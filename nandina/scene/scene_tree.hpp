//
// Created by cvrain on 2026/6/30.
//

#ifndef NANDINA_EXPERIMENT_SCENE_TREE_HPP
#define NANDINA_EXPERIMENT_SCENE_TREE_HPP

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
 *   - draw():      bottom-up depth-first, calls on_draw on every visible node.
 *
 * Also provides hit-testing by walking the tree from front to back.
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
     * Set the root node.  Ownership transfers to the tree.
     * The previous root (and all its descendants) is destroyed.
     */
    void set_root(std::unique_ptr<NanNode2D> root);

    // ---- per-frame traversal ----

    /// Call on_process(dt) on every node (top-down).
    /// Flushes any pending queue_delete() requests before processing.
    void process(float dt);

    /// Call on_draw() on every visible node (top-down, depth-first).
    void draw();

    // ---- deferred deletion ----

    /**
     * Queue a node for safe deletion at the start of the next process() call.
     * Safe to call from within on_process / on_draw — the node is not destroyed
     * until the current traversal completes.
     *
     * If an ancestor is already queued, this call is a no-op (the ancestor's
     * deletion will cascade).
     */
    void queue_delete(NanNode& node);

    // ---- hit testing ----

    /**
     * Find the deepest visible node that contains `world_point`.
     * Walks children front-to-back (higher z_index first), recursively.
     * @return The hit node, or nullptr if nothing was hit.
     */
    [[nodiscard]] auto hit_test(foundation::NanPoint world_point) const -> NanNode2D*;

private:
    /// Recursive hit test helper.
    static auto _hit_test_node(NanNode2D* node, foundation::NanPoint world_point) -> NanNode2D*;

    /// Destroy all nodes queued for deletion.
    void _flush_deletes();

    std::unique_ptr<NanNode2D> root_;
    std::vector<NanNode*> delete_queue_;
};

} // namespace nandina::scene

#endif // NANDINA_EXPERIMENT_SCENE_TREE_HPP
