//
// Created by cvrain on 2026/6/30.
//

#ifndef NANDINA_EXPERIMENT_NODE2D_HPP
#define NANDINA_EXPERIMENT_NODE2D_HPP

#include "node.hpp"
#include "../foundation/transform2d.hpp"

namespace nandina::scene
{

/**
 * 2D scene node with a local transform (position, rotation, scale).
 *
 * Inherits from NanNode and adds:
 *   - A local transform relative to the parent node.
 *   - Visibility flag and z-index for draw ordering.
 *   - Global transform computation by walking the parent chain.
 *
 * This is the base class for all spatially-located 2D nodes
 * (sprites, labels, collision shapes, etc.).
 *
 * The global transform is lazily cached: the first call to
 * global_transform() after a local change recomputes by composing
 * the parent chain.  Subsequent calls return the cached value.
 * The cache is invalidated automatically when any transform mutator
 * is called (set_position, set_rotation, set_scale, etc.).
 */
class NanNode2D : public NanNode {
public:
    NanNode2D();

    // ---- local transform ----

    [[nodiscard]] auto transform() const -> const foundation::NanTransform2D&;
    void set_transform(const foundation::NanTransform2D& t);

    [[nodiscard]] auto position() const -> foundation::NanPoint;
    void set_position(foundation::NanPoint pos);

    [[nodiscard]] auto rotation() const -> float;
    void set_rotation(float radians);

    [[nodiscard]] auto scale() const -> foundation::NanPoint;
    void set_scale(foundation::NanPoint s);
    void set_scale(float sx, float sy);

    /// Translate by offset in local space.
    void translate(foundation::NanPoint offset);

    /// Rotate by radians (adds to current rotation).
    void rotate(float radians);

    /// Multiply current scale by factor.
    void apply_scale(foundation::NanPoint factor);

    // ---- global transform (world-space) ----

    /// World-space transform.  Lazily recomputed from the parent chain.
    [[nodiscard]] auto global_transform() const -> foundation::NanTransform2D;

    [[nodiscard]] auto global_position() const -> foundation::NanPoint;
    void set_global_position(foundation::NanPoint pos);

    [[nodiscard]] auto global_rotation() const -> float;

    /// Convert a point from local space to global (world) space.
    [[nodiscard]] auto to_global(foundation::NanPoint local_point) const -> foundation::NanPoint;

    /// Convert a point from global (world) space to local space.
    [[nodiscard]] auto to_local(foundation::NanPoint global_point) const -> foundation::NanPoint;

    /// World-space axis-aligned bounding rectangle of this node.
    /// Override in subclasses to return the actual interactive/visible area.
    /// Default: empty rect at global_position.
    [[nodiscard]] virtual auto global_bounds() const -> foundation::NanRect;

    // ---- visibility ----

    [[nodiscard]] auto visible() const -> bool;
    void set_visible(bool v);

    // ---- draw order ----

    /// z_index controls sibling draw order. Higher values draw on top.
    [[nodiscard]] auto z_index() const -> int;
    void set_z_index(int z);

    [[nodiscard]] auto z_index_hint() const -> int override { return z_index_; }

    [[nodiscard]] auto is_visible_in_tree() const -> bool override {
        return visible_ && NanNode::is_visible_in_tree();
    }

    // ---- hit testing ----

    /// Check whether a point in local space is inside this node.
    /// Override in subclasses to define the node's interactive area.
    [[nodiscard]] virtual auto contains_point(foundation::NanPoint local_point) const -> bool;

    // ---- lifecycle ----

    void on_draw() override;

protected:
    void on_enter_tree() override;
    void on_exit_tree() override;

    /// Invalidate this node's cached global transform.
    void _invalidate_global();

    /// Invalidate global transform on this node and all descendants.
    void _propagate_invalidate_global();

private:
    foundation::NanTransform2D transform_;
    bool visible_ = true;
    int z_index_ = 0;

    mutable foundation::NanTransform2D cached_global_;
    mutable bool global_invalid_ = true;
};

} // namespace nandina::scene

#endif // NANDINA_EXPERIMENT_NODE2D_HPP
