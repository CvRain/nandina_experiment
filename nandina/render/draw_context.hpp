//
// Created by cvrain on 2026/7/2.
//
// DrawContext threads the drawing state down the tree during a draw traversal:
// the device, the clip stack, the current world transform, and inherited opacity.
//

#ifndef NANDINA_EXPERIMENT_DRAW_CONTEXT_HPP
#define NANDINA_EXPERIMENT_DRAW_CONTEXT_HPP

#include "../foundation/transform2d.hpp"
#include "clip_stack.hpp"
#include "render_device.hpp"

namespace nandina::scene
{
    class NanNode;
    class NanNode2D;
} // namespace nandina::scene

namespace nandina::render
{
    using foundation::NanRect;
    using foundation::NanTransform2D;

    class DrawContext {
    public:
        explicit DrawContext(IRenderDevice& device): device_(device), clip_(device) {}

        DrawContext(const DrawContext&) = delete;
        auto operator=(const DrawContext&) -> DrawContext& = delete;
        DrawContext(DrawContext&&) = delete;
        auto operator=(DrawContext&&) -> DrawContext& = delete;
        ~DrawContext() = default;

        [[nodiscard]] auto device() -> IRenderDevice& {
            return device_;
        }

        /// Current node's world transform (written by _propagate_draw on the way down).
        [[nodiscard]] auto world_transform() const -> const NanTransform2D& {
            return world_;
        }

        /// Inherited opacity in [0,1]: parent opacity x this node's opacity.
        [[nodiscard]] auto opacity() const -> float {
            return opacity_;
        }

        /// Clip stack (RAII push/pop).
        [[nodiscard]] auto clip() -> ClipStack& {
            return clip_;
        }

    private:
        // Scene traversal writes world_ / opacity_ as it descends the tree.
        friend class scene::NanNode;
        friend class scene::NanNode2D;

        IRenderDevice& device_;
        NanTransform2D world_;
        float opacity_ = 1.0F;
        ClipStack clip_;
    };

    /// Transform a local rect's four corners to world space and take the AABB.
    /// For rotated transforms this is the bounding box, not the exact quad.
    [[nodiscard]] inline auto
    world_bounds_from_local(const NanTransform2D& xf, const NanRect& local) -> NanRect {
        const NanPoint p0 = xf.transform_point(local.get_top_left());
        const NanPoint p1 = xf.transform_point(local.get_top_right());
        const NanPoint p2 = xf.transform_point(local.get_bottom_left());
        const NanPoint p3 = xf.transform_point(local.get_bottom_right());
        return NanRect::from_points(p0, p1).united(NanRect::from_points(p2, p3));
    }

} // namespace nandina::render

#endif // NANDINA_EXPERIMENT_DRAW_CONTEXT_HPP
