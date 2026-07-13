//
// Created by cvrain on 2026/6/30.
//

#ifndef NANDINA_EXPERIMENT_SCENE_TREE_HPP
#define NANDINA_EXPERIMENT_SCENE_TREE_HPP

#include "input_event.hpp"
#include "node2d.hpp"

#include <memory>
#include <vector>

namespace nandina::render
{
    class IRenderDevice;
} // namespace nandina::render

namespace nandina::scene
{

    /**
     * Root container and driver for the node tree.
     *
     * Owns the root node and orchestrates per-frame traversal:
     *   - process(dt): top-down, calls on_process on every node.
     *   - draw():      top-down depth-first, calls on_draw on every visible node.
     *
     * Also provides front-to-back hit-testing, hover tracking, and focus dispatch.
     * Hit-testing may use a world-space bounds coarse pass before local-space fine tests.
     */
    class NanSceneTree {
    public:
        NanSceneTree();
        ~NanSceneTree();

        NanSceneTree(const NanSceneTree&) = delete;
        auto operator=(const NanSceneTree&) -> NanSceneTree& = delete;

        // ---- root ----

        /// The root node, or nullptr if none set.
        [[nodiscard]] auto root() const -> NanNode2D*;

        /**
         * Set the root node. Ownership transfers to the tree.
         * The previous root (and all its descendants) is destroyed.
         */
        auto set_root(std::shared_ptr<NanNode2D> root) -> void;

        // ---- per-frame traversal ----

        /// Call on_process(dt) on every node (top-down).
        /// Flushes any pending queue_delete() requests before processing.
        auto process(float dt) -> void;

        /// Draw every visible node (top-down, depth-first) via the given device.
        /// Brackets the frame with device.begin_frame()/end_frame() and threads a
        /// DrawContext (world transform + opacity + clip) through the traversal.
        /// Use this when the scene owns the whole frame.
        auto draw(render::IRenderDevice& device) -> void;

        /// Render the scene into an already-open frame using the given context.
        /// Does NOT call begin_frame/end_frame — the caller owns the frame. Useful
        /// when app chrome is drawn in the same frame as the scene.
        auto render(render::DrawContext& ctx) -> void;

        // ---- input dispatch ----

        /// Dispatch a mouse-button event to the deepest hit node and bubble to root.
        auto dispatch_mouse_button(const MouseButtonEvent& event) -> void;

        /// Dispatch mouse movement, maintaining hover target enter/leave/move semantics.
        auto dispatch_mouse_move(const MouseMoveEvent& event) -> void;

        /// Dispatch a scroll-wheel event to the hovered node and bubble toward root.
        /// If nothing is hovered, hit-tests at the event position first.
        auto dispatch_mouse_wheel(const MouseWheelEvent& event) -> void;

        /// Current deepest hovered node, or nullptr if none.
        [[nodiscard]] auto hovered_node() const -> NanNode2D*;

        /// Dispatch a key event to the focused node and bubble toward the root.
        auto dispatch_key(const KeyEvent& event) -> void;

        /// Dispatch committed text input to the focused node and bubble toward root.
        auto dispatch_text_input(const TextInputEvent& event) -> void;

        void set_pointer_capture(NanNode2D* node);
        void release_pointer_capture(NanNode2D* node = nullptr);
        [[nodiscard]] auto pointer_capture() const -> NanNode2D*;

        /// Focused node, or nullptr if none.
        [[nodiscard]] auto focused_node() const -> NanNode2D*;

        /// Assign focus to a node inside this tree, or clear focus with nullptr.
        auto set_focus(NanNode2D* node) -> void;

        /// Move focus forward through the tree's focusable nodes.
        auto focus_next() -> void;

        /// Move focus backward through the tree's focusable nodes.
        auto focus_previous() -> void;

        // ---- deferred deletion ----

        /**
         * Queue a node for safe deletion at the start of the next process() call.
         * Safe to call from within on_process / on_draw — the node is not destroyed
         * until the current traversal completes.
         *
         * If an ancestor is already queued, this call is a no-op.
         */
        auto queue_delete(NanNode& node) -> void;

        // ---- hit testing ----

        /**
         * Find the deepest visible node that contains `world_point`.
         * Walks children front-to-back (higher z_index first), recursively.
         * A clipped control prunes descendants outside its world-space clip bounds.
         */
        [[nodiscard]] auto hit_test(foundation::NanPoint world_point) const -> NanNode2D*;

    private:
        static auto _hit_test_node(NanNode2D* node, foundation::NanPoint world_point) -> NanNode2D*;
        static void _collect_focusable_nodes(NanNode* node, std::vector<NanNode2D*>& out);

        auto
        _bubble_input(NanNode* start, InputEvent& event, const NanNode* stop_exclusive = nullptr)
            -> void;
        [[nodiscard]] auto _find_focus_target(NanNode* start) const -> NanNode2D*;
        auto _transition_hover(NanNode2D* next_hover, foundation::NanPoint screen_pos) -> void;
        auto _transition_focus(NanNode2D* next_focus) -> void;
        auto _sync_hover_after_tree_change() -> void;
        auto _hovered_is_inside(const NanNode& node) const -> bool;
        auto _focused_is_inside(const NanNode& node) const -> bool;
        auto _flush_deletes() -> void;

        std::shared_ptr<NanNode2D> root_;
        // Deferred-delete requests and interaction targets are weak: if a node is
        // destroyed by another path before we act on it, the weak_ptr simply expires
        // instead of dangling.
        std::vector<std::weak_ptr<NanNode>> delete_queue_;
        std::weak_ptr<NanNode2D> hovered_node_;
        std::weak_ptr<NanNode2D> focused_node_;
        std::weak_ptr<NanNode2D> pointer_capture_;
        foundation::NanPoint last_mouse_pos_ {};
        bool has_mouse_pos_ = false;
    };

} // namespace nandina::scene

#endif // NANDINA_EXPERIMENT_SCENE_TREE_HPP
