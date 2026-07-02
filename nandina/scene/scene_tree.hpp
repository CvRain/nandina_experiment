//
// Created by cvrain on 2026/6/30.
//

#ifndef NANDINA_EXPERIMENT_SCENE_TREE_HPP
#define NANDINA_EXPERIMENT_SCENE_TREE_HPP

#include "input_event.hpp"
#include "node2d.hpp"

#include <memory>
#include <vector>

namespace nandina::scene
{

/**
 * Root container and driver for the node tree.
 *
 * Owns the root node and orchestrates per-frame traversal:
 *   - process(dt): top-down, calls on_process on every node.
 *   - draw():      top-down depth-first, calls on_draw on every visible node.
 *
 * Also provides front-to-back hit-testing and hover-target tracking.
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
    auto set_root(std::unique_ptr<NanNode2D> root) -> void;

    // ---- per-frame traversal ----

    /// Call on_process(dt) on every node (top-down).
    /// Flushes any pending queue_delete() requests before processing.
    auto process(float dt) -> void;

    /// Call on_draw() on every visible node (top-down, depth-first).
    auto draw() const -> void;

    // ---- input dispatch ----

    /// Dispatch a mouse-button event to the deepest hit node and bubble to root.
    auto dispatch_mouse_button(const MouseButtonEvent& event) -> void;

    /// Dispatch mouse movement, maintaining hover target enter/leave/move semantics.
    auto dispatch_mouse_move(const MouseMoveEvent& event) -> void;

    /// Current deepest hovered node, or nullptr if none.
    [[nodiscard]] auto hovered_node() const -> NanNode2D*;

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
     */
    [[nodiscard]] auto hit_test(foundation::NanPoint world_point) const -> NanNode2D*;

private:
    static auto _hit_test_node(NanNode2D* node, foundation::NanPoint world_point) -> NanNode2D*;

    auto _bubble_input(NanNode* start, InputEvent& event, const NanNode* stop_exclusive = nullptr) -> void;
    auto _transition_hover(NanNode2D* next_hover, foundation::NanPoint screen_pos) -> void;
    auto _sync_hover_after_tree_change() -> void;
    auto _hovered_is_inside(const NanNode& node) const -> bool;
    auto _flush_deletes() -> void;

    std::unique_ptr<NanNode2D> root_;
    std::vector<NanNode*> delete_queue_;
    NanNode2D* hovered_node_ = nullptr;
    foundation::NanPoint last_mouse_pos_ {};
    bool has_mouse_pos_ = false;
};

} // namespace nandina::scene

#endif // NANDINA_EXPERIMENT_SCENE_TREE_HPP
