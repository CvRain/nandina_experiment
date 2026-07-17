//
// Created by cvrain on 2026/6/30.
//

#include "scene_tree.hpp"
#include "../render/draw_context.hpp"
#include "control.hpp"
#include "canvas_layer.hpp"

#include <algorithm>
#include <numeric>

namespace nandina::scene
{

    namespace
    {

        [[nodiscard]] auto build_chain(NanNode* leaf) -> std::vector<NanNode*> {
            std::vector<NanNode*> chain;
            for (auto* node = leaf; node != nullptr; node = node->parent()) {
                chain.push_back(node);
            }
            return chain;
        }

        /// Produce a weak_ptr<NanNode2D> from a raw pointer to a tree-owned node.
        /// All nodes inside an active tree are owned by shared_ptr (root + children),
        /// so shared_from_this() is safe here; a null input yields an empty weak_ptr.
        [[nodiscard]] auto weak_2d(NanNode2D* node) -> std::weak_ptr<NanNode2D> {
            if (node == nullptr) {
                return {};
            }
            return std::static_pointer_cast<NanNode2D>(node->shared_from_this());
        }

    } // namespace

    NanSceneTree::NanSceneTree() = default;

    NanSceneTree::PhaseScope::PhaseScope(NanSceneTree& tree, const FramePhase phase):
        tree_(&tree), previous_(tree.phase_) {
        tree.phase_ = phase;
    }

    NanSceneTree::PhaseScope::~PhaseScope() {
        tree_->phase_ = previous_;
    }

    NanSceneTree::~NanSceneTree() {
        pointer_capture_.reset();
        _transition_hover(nullptr, has_mouse_pos_ ? last_mouse_pos_ : foundation::NanPoint {});
        _transition_focus(nullptr);
        _flush_deletes();
        if (root_) {
            root_->_propagate_exit_tree();
        }
    }

    auto NanSceneTree::root() const -> NanNode2D* {
        return root_.get();
    }

    auto NanSceneTree::phase() const -> FramePhase {
        return phase_;
    }

    auto NanSceneTree::enter_phase(const FramePhase phase) -> PhaseScope {
        return PhaseScope {*this, phase};
    }

    void NanSceneTree::begin_phase(const FramePhase phase) {
        phase_ = phase;
    }

    void NanSceneTree::end_phase() {
        phase_ = FramePhase::idle;
    }

    auto NanSceneTree::defers_tree_mutation() const -> bool {
        return phase_ == FramePhase::process || phase_ == FramePhase::layout
            || phase_ == FramePhase::post_layout || phase_ == FramePhase::paint;
    }

    void NanSceneTree::defer_tree_mutation(std::function<void()> mutation) {
        tree_mutations_.push_back(std::move(mutation));
    }

    void NanSceneTree::flush_tree_mutations() {
        auto pending = std::move(tree_mutations_);
        tree_mutations_.clear();
        for (auto& mutation: pending) {
            mutation();
        }
        _sync_hover_after_tree_change();
    }

    void NanSceneTree::post_layout(std::function<void()> action) {
        post_layout_actions_.push_back(std::move(action));
    }

    auto NanSceneTree::flush_post_layout_actions() -> bool {
        if (post_layout_actions_.empty()) {
            return false;
        }
        auto pending = std::move(post_layout_actions_);
        post_layout_actions_.clear();
        for (auto& action: pending) {
            action();
        }
        return true;
    }

    auto NanSceneTree::set_root(std::shared_ptr<NanNode2D> root) -> void {
        pointer_capture_.reset();
        _transition_hover(nullptr, has_mouse_pos_ ? last_mouse_pos_ : foundation::NanPoint {});
        _transition_focus(nullptr);

        if (root_) {
            root_->_propagate_exit_tree();
        }

        delete_queue_.clear();
        tree_mutations_.clear();
        post_layout_actions_.clear();
        root_ = std::move(root);

        if (root_) {
            root_->_propagate_enter_tree(this);
            root_->_propagate_ready();
        }

        _sync_hover_after_tree_change();
    }

    void NanSceneTree::set_default_text_pipeline(widget::primitives::TextPipeline pipeline) {
        if (pipeline.backend == nullptr) {
            throw std::invalid_argument("default text pipeline requires a layout backend");
        }
        default_text_pipeline_ = pipeline;
    }

    void NanSceneTree::clear_default_text_pipeline() {
        default_text_pipeline_.reset();
    }

    auto NanSceneTree::default_text_pipeline() const -> const widget::primitives::TextPipeline* {
        return default_text_pipeline_ ? &*default_text_pipeline_ : nullptr;
    }

    auto NanSceneTree::process(const float dt) -> void {
        _flush_deletes();
        if (root_) {
            root_->_propagate_process(dt);
        }
    }

    auto NanSceneTree::_layout_root_once(const foundation::NanSize viewport_size) -> bool {
        if (auto* stack = root_ != nullptr ? root_->as_layer_stack() : nullptr;
            stack != nullptr) {
            bool laid_out = false;
            for (auto* layer: stack->layers_in_order()) {
                auto* control = layer->space() == CanvasSpace::screen ? layer->layout_root() : nullptr;
                if (control == nullptr
                    || (!control->layout_dirty() && control->size() == viewport_size)) {
                    continue;
                }
                (void)control->measure_layout(LayoutConstraints::tight(viewport_size));
                control->layout_to(foundation::NanRect::from_origin_size(
                    foundation::NanPoint::zero(), viewport_size
                ));
                laid_out = true;
            }
            return laid_out;
        }
        auto* control = root_ != nullptr ? root_->as_control() : nullptr;
        if (control == nullptr || (!control->layout_dirty() && control->size() == viewport_size)) {
            return false;
        }
        (void)control->measure_layout(LayoutConstraints::tight(viewport_size));
        control->layout_to(foundation::NanRect::from_origin_size(
            foundation::NanPoint::zero(), viewport_size
        ));
        return true;
    }

    auto NanSceneTree::layout_root(const foundation::NanSize viewport_size) -> std::size_t {
        std::size_t passes = 0;
        {
            auto phase = enter_phase(FramePhase::layout);
            if (_layout_root_once(viewport_size)) {
                ++passes;
            }
        }
        flush_tree_mutations();

        bool ran_actions = false;
        {
            auto phase = enter_phase(FramePhase::post_layout);
            ran_actions = flush_post_layout_actions();
        }
        flush_tree_mutations();

        if (ran_actions) {
            {
                auto phase = enter_phase(FramePhase::layout);
                if (_layout_root_once(viewport_size)) {
                    ++passes;
                }
            }
            flush_tree_mutations();
        }
        return passes;
    }

    auto NanSceneTree::draw(render::IRenderDevice& device) -> void {
        device.begin_frame();
        render::DrawContext ctx {device};
        render(ctx);
        device.end_frame();
        flush_tree_mutations();
    }

    auto NanSceneTree::render(render::DrawContext& ctx) -> void {
        auto phase = enter_phase(FramePhase::paint);
        if (root_) {
            root_->_propagate_draw(ctx);
        }
    }

    auto NanSceneTree::dispatch_mouse_button(const MouseButtonEvent& event) -> void {
        auto copy = event;
        auto* captured = pointer_capture_.lock().get();
        if (!_input_enabled_for(captured)) {
            pointer_capture_.reset();
            captured = nullptr;
        }
        auto* hit =
            !copy.is_pressed() && captured != nullptr ? captured : hit_test(copy.screen_pos());
        if (!hit) {
            return;
        }

        set_focus(_find_focus_target(hit));
        _bubble_input(hit, copy);
        if (!copy.is_pressed() && captured != nullptr) {
            pointer_capture_.reset();
        }
    }

    auto NanSceneTree::dispatch_mouse_move(const MouseMoveEvent& event) -> void {
        has_mouse_pos_ = true;
        last_mouse_pos_ = event.screen_pos();

        auto* hit = hit_test(event.screen_pos());
        _transition_hover(hit, event.screen_pos());

        auto* target = pointer_capture_.lock().get();
        if (!_input_enabled_for(target)) {
            pointer_capture_.reset();
            target = nullptr;
        }
        if (target == nullptr) {
            target = hovered_node_.lock().get();
        }
        if (!target) {
            return;
        }

        auto copy = event;
        _bubble_input(target, copy);
    }

    auto NanSceneTree::hovered_node() const -> NanNode2D* {
        return hovered_node_.lock().get();
    }

    auto NanSceneTree::dispatch_key(const KeyEvent& event) -> void {
        auto* focused = focused_node_.lock().get();
        if (!_input_enabled_for(focused)) {
            _transition_focus(nullptr);
            return;
        }

        auto copy = event;
        _bubble_input(focused, copy);
    }

    auto NanSceneTree::dispatch_mouse_wheel(const MouseWheelEvent& event) -> void {
        auto* target = hit_test(event.screen_pos());
        if (!target) {
            return;
        }

        auto copy = event;
        _bubble_input(target, copy);
    }

    auto NanSceneTree::dispatch_text_input(const TextInputEvent& event) -> void {
        auto* focused = focused_node_.lock().get();
        if (!_input_enabled_for(focused)) {
            _transition_focus(nullptr);
            return;
        }

        auto copy = event;
        _bubble_input(focused, copy);
    }

    auto NanSceneTree::focused_node() const -> NanNode2D* {
        return focused_node_.lock().get();
    }

    void NanSceneTree::set_pointer_capture(NanNode2D* node) {
        if (node == nullptr || !node->is_inside_tree() || node->get_tree() != this) {
            return;
        }
        if (!_input_enabled_for(node)) {
            return;
        }
        pointer_capture_ = weak_2d(node);
    }

    void NanSceneTree::release_pointer_capture(NanNode2D* node) {
        auto* captured = pointer_capture_.lock().get();
        if (node == nullptr || captured == node) {
            pointer_capture_.reset();
        }
    }

    auto NanSceneTree::pointer_capture() const -> NanNode2D* {
        return pointer_capture_.lock().get();
    }

    auto NanSceneTree::set_focus(NanNode2D* node) -> void {
        if (node
            && (!node->is_inside_tree() || node->get_tree() != this || !node->is_visible_in_tree()
                || !node->is_focusable() || !_input_enabled_for(node)))
        {
            return;
        }

        _transition_focus(node);
    }

    auto NanSceneTree::focus_next() -> void {
        if (!root_) {
            return;
        }

        std::vector<NanNode2D*> nodes;
        _collect_focusable_nodes(root_.get(), nodes);
        if (nodes.empty()) {
            _transition_focus(nullptr);
            return;
        }

        auto* focused = focused_node_.lock().get();
        if (!focused) {
            _transition_focus(nodes.front());
            return;
        }

        const auto it = std::ranges::find(nodes, focused);
        if (it == nodes.end() || std::next(it) == nodes.end()) {
            _transition_focus(nodes.front());
            return;
        }

        _transition_focus(*std::next(it));
    }

    auto NanSceneTree::focus_previous() -> void {
        if (!root_) {
            return;
        }

        std::vector<NanNode2D*> nodes;
        _collect_focusable_nodes(root_.get(), nodes);
        if (nodes.empty()) {
            _transition_focus(nullptr);
            return;
        }

        auto* focused = focused_node_.lock().get();
        if (!focused) {
            _transition_focus(nodes.back());
            return;
        }

        const auto it = std::ranges::find(nodes, focused);
        if (it == nodes.end() || it == nodes.begin()) {
            _transition_focus(nodes.back());
            return;
        }

        _transition_focus(*std::prev(it));
    }

    auto NanSceneTree::queue_delete(NanNode& node) -> void {
        for (const auto& queued_weak: delete_queue_) {
            auto queued = queued_weak.lock();
            if (!queued) {
                continue;
            }
            if (queued.get() == &node || queued->is_ancestor_of(node)) {
                return;
            }
        }

        std::erase_if(delete_queue_, [&node](const std::weak_ptr<NanNode>& queued_weak) {
            auto queued = queued_weak.lock();
            return !queued || node.is_ancestor_of(*queued);
        });

        delete_queue_.push_back(node.weak_from_this());
    }

    void NanSceneTree::flush_deferred_deletes() {
        _flush_deletes();
    }

    auto NanSceneTree::hit_test(const foundation::NanPoint world_point) const -> NanNode2D* {
        if (!root_) {
            return nullptr;
        }

        if (auto* stack = root_->as_layer_stack(); stack != nullptr) {
            for (auto* layer: stack->layers_in_order(true)) {
                if (!layer->is_visible_in_tree() || layer->input_mode() == LayerInputMode::disabled) {
                    continue;
                }
                const auto child_count = layer->child_count();
                if (child_count > 0) {
                    std::vector<std::size_t> indices(child_count);
                    std::iota(indices.begin(), indices.end(), static_cast<std::size_t>(0));
                    std::ranges::stable_sort(indices, [layer](const std::size_t a, const std::size_t b) {
                        const auto* lhs = layer->get_child(a);
                        const auto* rhs = layer->get_child(b);
                        return (lhs != nullptr ? lhs->z_index_hint() : 0)
                            > (rhs != nullptr ? rhs->z_index_hint() : 0);
                    });
                    for (const auto index: indices) {
                        auto* child = layer->get_child(index);
                        auto* node = child != nullptr ? child->as_node2d() : nullptr;
                        if (auto* hit = _hit_test_node(node, world_point); hit != nullptr) {
                            return hit;
                        }
                    }
                }
                if (layer->input_mode() == LayerInputMode::block_below) {
                    return nullptr;
                }
            }
            return nullptr;
        }

        return _hit_test_node(root_.get(), world_point);
    }

    auto NanSceneTree::_hit_test_node(NanNode2D* node, const foundation::NanPoint world_point)
        -> NanNode2D* {
        if (!node || !node->is_visible_in_tree()) {
            return nullptr;
        }

        bool visit_children = true;
        if (const auto* control = node->as_control();
            control != nullptr && control->overflow() == ControlOverflow::clip)
        {
            const auto clip_bounds = control->global_bounds();
            visit_children = clip_bounds.is_valid() && clip_bounds.contains_point(world_point);
        }

        const auto count = node->child_count();
        if (visit_children && count > 0) {
            std::vector<size_t> indices(count);
            std::iota(indices.begin(), indices.end(), static_cast<size_t>(0));

            std::ranges::stable_sort(indices, [node](const size_t a, const size_t b) {
                const auto* raw_a = node->get_child(a);
                const auto* raw_b = node->get_child(b);
                const auto* child_a = raw_a ? raw_a->as_node2d() : nullptr;
                const auto* child_b = raw_b ? raw_b->as_node2d() : nullptr;
                const int za = child_a ? child_a->z_index() : 0;
                const int zb = child_b ? child_b->z_index() : 0;
                if (za != zb) {
                    return za > zb;
                }
                return a > b;
            });

            for (const auto idx: indices) {
                auto* raw_child = node->get_child(idx);
                auto* child = raw_child ? raw_child->as_node2d() : nullptr;
                if (!child) {
                    continue;
                }

                if (auto* hit = _hit_test_node(child, world_point)) {
                    return hit;
                }
            }
        }

        const auto bounds = node->global_bounds();
        if (bounds.is_valid() && !bounds.contains_point(world_point)) {
            return nullptr;
        }

        if (const auto local_point = node->to_local(world_point); node->contains_point(local_point))
        {
            return node;
        }

        return nullptr;
    }

    void NanSceneTree::_collect_focusable_nodes(NanNode* node, std::vector<NanNode2D*>& out) {
        if (!node) {
            return;
        }

        if (auto* stack = node->as_layer_stack(); stack != nullptr) {
            for (auto* layer: stack->layers_in_order()) {
                if (layer->is_visible_in_tree() && layer->input_mode() != LayerInputMode::disabled) {
                    _collect_focusable_nodes(layer, out);
                }
            }
            return;
        }
        if (auto* layer = node->as_canvas_layer();
            layer != nullptr
            && (!layer->is_visible_in_tree() || layer->input_mode() == LayerInputMode::disabled)) {
            return;
        }

        if (auto* node_2d = node->as_node2d();
            node_2d && node_2d->is_visible_in_tree() && node_2d->is_focusable())
        {
            out.push_back(node_2d);
        }

        for (size_t i = 0; i < node->child_count(); ++i) {
            _collect_focusable_nodes(node->get_child(i), out);
        }
    }

    auto
    NanSceneTree::_bubble_input(NanNode* start, InputEvent& event, const NanNode* stop_exclusive)
        -> void {
        for (auto* node = start; node != nullptr && node != stop_exclusive; node = node->parent()) {
            if (node->on_input(event) || event.is_accepted()) {
                break;
            }
        }
    }

    auto NanSceneTree::_find_focus_target(NanNode* start) const -> NanNode2D* {
        for (auto* node = start; node != nullptr; node = node->parent()) {
            auto* node_2d = node->as_node2d();
            if (node_2d && node_2d->is_visible_in_tree() && node_2d->is_focusable()) {
                return node_2d;
            }
        }

        return nullptr;
    }

    auto
    NanSceneTree::_transition_hover(NanNode2D* next_hover, const foundation::NanPoint screen_pos)
        -> void {
        auto* old_hover = hovered_node_.lock().get();
        if (next_hover == old_hover) {
            return;
        }

        hovered_node_ = weak_2d(next_hover);

        const auto old_chain = build_chain(old_hover);
        const auto new_chain = build_chain(next_hover);

        size_t old_idx = old_chain.size();
        size_t new_idx = new_chain.size();
        while (old_idx > 0 && new_idx > 0 && old_chain[old_idx - 1] == new_chain[new_idx - 1]) {
            --old_idx;
            --new_idx;
        }

        const auto* common_ancestor = (old_idx < old_chain.size()) ? old_chain[old_idx] : nullptr;

        if (old_hover) {
            MouseLeaveEvent leave_event(screen_pos);
            _bubble_input(old_hover, leave_event, common_ancestor);
        }

        if (next_hover) {
            MouseEnterEvent enter_event(screen_pos);
            _bubble_input(next_hover, enter_event, common_ancestor);
        }
    }

    auto NanSceneTree::_transition_focus(NanNode2D* next_focus) -> void {
        auto* old_focus = focused_node_.lock().get();
        if (next_focus == old_focus) {
            return;
        }

        focused_node_ = weak_2d(next_focus);

        const auto old_chain = build_chain(old_focus);
        const auto new_chain = build_chain(next_focus);

        size_t old_idx = old_chain.size();
        size_t new_idx = new_chain.size();
        while (old_idx > 0 && new_idx > 0 && old_chain[old_idx - 1] == new_chain[new_idx - 1]) {
            --old_idx;
            --new_idx;
        }

        const auto* common_ancestor = (old_idx < old_chain.size()) ? old_chain[old_idx] : nullptr;

        if (old_focus) {
            FocusLeaveEvent leave_event;
            _bubble_input(old_focus, leave_event, common_ancestor);
        }

        if (next_focus) {
            FocusEnterEvent enter_event;
            _bubble_input(next_focus, enter_event, common_ancestor);
        }
    }

    auto NanSceneTree::_sync_hover_after_tree_change() -> void {
        if (!has_mouse_pos_) {
            hovered_node_.reset();
            return;
        }

        _transition_hover(hit_test(last_mouse_pos_), last_mouse_pos_);
    }

    auto NanSceneTree::_hovered_is_inside(const NanNode& node) const -> bool {
        auto* hovered = hovered_node_.lock().get();
        if (!hovered) {
            return false;
        }

        return hovered == &node || node.is_ancestor_of(*hovered);
    }

    auto NanSceneTree::_focused_is_inside(const NanNode& node) const -> bool {
        auto* focused = focused_node_.lock().get();
        if (!focused) {
            return false;
        }

        return focused == &node || node.is_ancestor_of(*focused);
    }

    auto NanSceneTree::_input_enabled_for(const NanNode* node) -> bool {
        if (node == nullptr || !node->is_visible_in_tree()) {
            return false;
        }
        for (auto* current = node; current != nullptr; current = current->parent()) {
            if (const auto* layer = current->as_canvas_layer();
                layer != nullptr && layer->input_mode() == LayerInputMode::disabled) {
                return false;
            }
        }
        return true;
    }

    auto NanSceneTree::_flush_deletes() -> void {
        if (delete_queue_.empty()) {
            return;
        }

        // Move out and clear first: destroying a node can trigger callbacks that
        // re-enter queue_delete; those requests belong to the next flush.
        auto pending = std::move(delete_queue_);
        delete_queue_.clear();

        for (const auto& node_weak: pending) {
            auto node_shared = node_weak.lock();
            if (!node_shared) {
                continue; // Already destroyed by another path — weak_ptr expired.
            }
            auto* node = node_shared.get();

            if (!node->is_inside_tree()) {
                continue;
            }

            if (_hovered_is_inside(*node)) {
                _transition_hover(
                    nullptr,
                    has_mouse_pos_ ? last_mouse_pos_ : foundation::NanPoint {}
                );
            }
            if (_focused_is_inside(*node)) {
                _transition_focus(nullptr);
            }

            if (auto* parent = node->parent()) {
                parent->remove_and_delete(*node);
            }
            else if (node == root_.get()) {
                root_->_propagate_exit_tree();
                root_.reset();
            }
        }

        _sync_hover_after_tree_change();
    }

} // namespace nandina::scene
