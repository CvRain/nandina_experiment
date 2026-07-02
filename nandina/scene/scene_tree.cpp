//
// Created by cvrain on 2026/6/30.
//

#include "scene_tree.hpp"

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

} // namespace

NanSceneTree::NanSceneTree() = default;

NanSceneTree::~NanSceneTree() {
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

auto NanSceneTree::set_root(std::unique_ptr<NanNode2D> root) -> void {
    _transition_hover(nullptr, has_mouse_pos_ ? last_mouse_pos_ : foundation::NanPoint {});
    _transition_focus(nullptr);

    if (root_) {
        root_->_propagate_exit_tree();
    }

    delete_queue_.clear();
    root_ = std::move(root);

    if (root_) {
        root_->_propagate_enter_tree(this);
        root_->_propagate_ready();
    }

    _sync_hover_after_tree_change();
}

auto NanSceneTree::process(const float dt) -> void {
    _flush_deletes();
    if (root_) {
        root_->_propagate_process(dt);
    }
}

auto NanSceneTree::draw() const -> void {
    if (root_) {
        root_->_propagate_draw();
    }
}

auto NanSceneTree::dispatch_mouse_button(const MouseButtonEvent& event) -> void {
    auto copy = event;
    auto* hit = hit_test(copy.screen_pos());
    if (!hit) {
        return;
    }

    set_focus(hit);
    _bubble_input(hit, copy);
}

auto NanSceneTree::dispatch_mouse_move(const MouseMoveEvent& event) -> void {
    has_mouse_pos_ = true;
    last_mouse_pos_ = event.screen_pos();

    auto* hit = hit_test(event.screen_pos());
    _transition_hover(hit, event.screen_pos());

    if (!hovered_node_) {
        return;
    }

    auto copy = event;
    _bubble_input(hovered_node_, copy);
}

auto NanSceneTree::hovered_node() const -> NanNode2D* {
    return hovered_node_;
}

auto NanSceneTree::dispatch_key(const KeyEvent& event) -> void {
    if (!focused_node_) {
        return;
    }

    auto copy = event;
    _bubble_input(focused_node_, copy);
}

auto NanSceneTree::focused_node() const -> NanNode2D* {
    return focused_node_;
}

auto NanSceneTree::set_focus(NanNode2D* node) -> void {
    if (node && (!node->is_inside_tree() || node->get_tree() != this || !node->is_visible_in_tree())) {
        return;
    }

    _transition_focus(node);
}

auto NanSceneTree::queue_delete(NanNode& node) -> void {
    for (const auto* queued : delete_queue_) {
        if (queued == &node || queued->is_ancestor_of(node)) {
            return;
        }
    }

    std::erase_if(delete_queue_, [&node](const NanNode* queued) {
        return node.is_ancestor_of(*queued);
    });

    delete_queue_.push_back(&node);
}

auto NanSceneTree::hit_test(const foundation::NanPoint world_point) const -> NanNode2D* {
    if (!root_) {
        return nullptr;
    }

    return _hit_test_node(root_.get(), world_point);
}

auto NanSceneTree::_hit_test_node(NanNode2D* node, const foundation::NanPoint world_point)
    -> NanNode2D* {
    if (!node || !node->is_visible_in_tree()) {
        return nullptr;
    }

    const auto count = node->child_count();
    if (count > 0) {
        std::vector<size_t> indices(count);
        std::iota(indices.begin(), indices.end(), static_cast<size_t>(0));

        std::ranges::stable_sort(indices, [node](const size_t a, const size_t b) {
            const auto* child_a = dynamic_cast<NanNode2D*>(node->get_child(a));
            const auto* child_b = dynamic_cast<NanNode2D*>(node->get_child(b));
            const int za = child_a ? child_a->z_index() : 0;
            const int zb = child_b ? child_b->z_index() : 0;
            if (za != zb) {
                return za > zb;
            }
            return a > b;
        });

        for (const auto idx : indices) {
            auto* child = dynamic_cast<NanNode2D*>(node->get_child(idx));
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

    if (const auto local_point = node->to_local(world_point); node->contains_point(local_point)) {
        return node;
    }

    return nullptr;
}

auto NanSceneTree::_bubble_input(NanNode* start, InputEvent& event, const NanNode* stop_exclusive) -> void {
    for (auto* node = start; node != nullptr && node != stop_exclusive; node = node->parent()) {
        if (node->on_input(event) || event.is_accepted()) {
            break;
        }
    }
}

auto NanSceneTree::_transition_hover(NanNode2D* next_hover, const foundation::NanPoint screen_pos) -> void {
    if (next_hover == hovered_node_) {
        return;
    }

    auto* old_hover = hovered_node_;
    hovered_node_ = next_hover;

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
    if (next_focus == focused_node_) {
        return;
    }

    auto* old_focus = focused_node_;
    focused_node_ = next_focus;

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
        hovered_node_ = nullptr;
        return;
    }

    _transition_hover(hit_test(last_mouse_pos_), last_mouse_pos_);
}

auto NanSceneTree::_hovered_is_inside(const NanNode& node) const -> bool {
    if (!hovered_node_) {
        return false;
    }

    return hovered_node_ == &node || node.is_ancestor_of(*hovered_node_);
}

auto NanSceneTree::_focused_is_inside(const NanNode& node) const -> bool {
    if (!focused_node_) {
        return false;
    }

    return focused_node_ == &node || node.is_ancestor_of(*focused_node_);
}

auto NanSceneTree::_flush_deletes() -> void {
    if (delete_queue_.empty()) {
        return;
    }

    for (auto* node : delete_queue_) {
        if (!node->is_inside_tree()) {
            continue;
        }

        if (_hovered_is_inside(*node)) {
            _transition_hover(nullptr, has_mouse_pos_ ? last_mouse_pos_ : foundation::NanPoint {});
        }
        if (_focused_is_inside(*node)) {
            _transition_focus(nullptr);
        }

        if (auto* parent = node->parent()) {
            parent->remove_and_delete(*node);
        } else if (node == root_.get()) {
            root_->_propagate_exit_tree();
            root_.reset();
        }
    }

    delete_queue_.clear();
    _sync_hover_after_tree_change();
}

} // namespace nandina::scene
