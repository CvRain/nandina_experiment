//
// Created by cvrain on 2026/6/30.
//

#ifndef NANDINA_EXPERIMENT_NODE_HPP
#define NANDINA_EXPERIMENT_NODE_HPP

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::scene
{

class InputEvent;
class NanSceneTree;

/**
 * Base class for all nodes in the scene tree.
 *
 * Nodes form a tree: each node owns zero or more children via unique_ptr.
 * The tree root is owned by NanSceneTree.  When a node is destroyed all
 * descendants are recursively destroyed.
 *
 * Lifecycle callbacks (override in subclasses):
 *   on_enter_tree()  — top-down, called when the node enters the active tree
 *   on_ready()       — bottom-up, called after ALL descendants have entered
 *   on_exit_tree()   — called before the node leaves the active tree
 *   on_process(dt)   — top-down, every frame (dt in seconds)
 *   on_draw()        — top-down, parent drawn before children
 *
 * Lifecycle order on add_child to an in-tree parent:
 *   1. child.on_enter_tree
 *   2. grandchild.on_enter_tree (recursively)
 *   3. grandchild.on_ready
 *   4. child.on_ready
 *
 * @note Nodes are non-copyable and non-movable while inside the tree.
 *       Ownership is managed through std::unique_ptr.
 */
class NanNode {
    friend class NanSceneTree;
public:
    NanNode();
    virtual ~NanNode();

    NanNode(const NanNode&) = delete;
    auto operator=(const NanNode&) -> NanNode& = delete;
    NanNode(NanNode&&) = delete;
    auto operator=(NanNode&&) -> NanNode& = delete;

    // ---- hierarchy ----

    /// Parent node, or nullptr if this is the root or detached.
    [[nodiscard]] auto parent() const -> NanNode*;

    /// Number of direct children.
    [[nodiscard]] auto child_count() const -> size_t;

    /// Get child by index [0, child_count).
    [[nodiscard]] auto get_child(size_t index) const -> NanNode*;

    /// True if this node is an ancestor of `other`.
    [[nodiscard]] auto is_ancestor_of(const NanNode& other) const -> bool;

    // ---- tree membership ----

    /// True while this node is part of an active scene tree.
    [[nodiscard]] auto is_inside_tree() const -> bool;

    /// The scene tree this node belongs to, or nullptr.
    [[nodiscard]] auto get_tree() const -> NanSceneTree*;

    // ---- child management ----

    /**
     * Add a child node.  Ownership transfers to this node.
     * @return A reference to the added child (non-owning).
     *
     * @pre child is not null and not already owned by another node.
     * @throws std::runtime_error if child is null, already has a parent,
     *         or would mix NanNode/NanNode2D on the same parent-child edge.
     */
    auto add_child(std::unique_ptr<NanNode> child) -> NanNode&;

    /**
     * Remove a child node and return ownership to the caller.
     * @return The removed child as unique_ptr, or nullptr if not found.
     */
    auto remove_child(NanNode& child) -> std::unique_ptr<NanNode>;

    /// Remove and immediately destroy a child.
    void remove_and_delete(NanNode& child);

    // ---- name ----

    [[nodiscard]] auto name() const -> std::string_view;
    void set_name(std::string name);

    // ---- lifecycle (override in subclasses) ----

    /// Called when the node enters the tree (top-down: parent before children).
    virtual void on_enter_tree();

    /// Called after all descendants' on_enter_tree have completed (bottom-up).
    virtual void on_ready();

    /// Called before the node (and all descendants) leave the tree.
    virtual void on_exit_tree();

    /**
     * Handle an input event routed to this node via hit-testing.
     * Return true if the event was consumed (stops bubbling to parent).
     * Default: false (event continues to parent).
     */
    virtual auto on_input(InputEvent& event) -> bool;

    /// Called every frame.  dt is the elapsed time in seconds.
    virtual void on_process(float dt);

    /// Called during draw traversal (top-down: parent drawn before children).
    virtual void on_draw();

    /// Hint for sibling draw/hit-test ordering (higher = on top).
    /// Override in subclasses that have a z-ordering concept.
    [[nodiscard]] virtual auto z_index_hint() const -> int;

    /// True if this node should be drawn / hit-tested in the active tree.
    /// A false return means this node AND all descendants are skipped.
    [[nodiscard]] virtual auto is_visible_in_tree() const -> bool;

protected:
    /// Internal: set the owning scene tree (called by NanSceneTree).
    void _set_tree(NanSceneTree* tree);

    /// Internal: propagate enter_tree to this node and all descendants.
    void _propagate_enter_tree(NanSceneTree* tree);

    /// Internal: propagate ready to all descendants then this node (bottom-up).
    void _propagate_ready();

    /// Internal: propagate exit_tree to this node and all descendants.
    void _propagate_exit_tree();

    /// Internal: process this node then recursively process children.
    void _propagate_process(float dt);

    /// Internal: draw this node then recursively draw children (top-down).
    void _propagate_draw();

private:
    NanNode* parent_ = nullptr;
    std::vector<std::unique_ptr<NanNode>> children_;
    NanSceneTree* tree_ = nullptr;
    std::string name_;
};

} // namespace nandina::scene

#endif // NANDINA_EXPERIMENT_NODE_HPP
