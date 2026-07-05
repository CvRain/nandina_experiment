//
// Created by cvrain on 2026/7/2.
//
// Abstract render device. All coordinates are world coordinates (screen pixels,
// y down). Backends submit primitives to a concrete renderer (raylib / offscreen
// / recording test double). No raylib types appear in this header.
//

#ifndef NANDINA_EXPERIMENT_RENDER_DEVICE_HPP
#define NANDINA_EXPERIMENT_RENDER_DEVICE_HPP

#include "../foundation/geometry.hpp"
#include "../foundation/nandina_color.hpp"

#include <string_view>

namespace nandina::render
{
    using foundation::NanColor;
    using foundation::NanPoint;
    using foundation::NanRect;

    /// Abstract drawing device. Coordinates are world-space (screen pixels, y down).
    /// Implementations translate primitives into a concrete backend. Traversal-varying
    /// state (transform, clip, opacity) lives in DrawContext, not here — the device is
    /// a stateless "draw this primitive" executor plus frame/clip bracketing.
    class IRenderDevice {
    public:
        virtual ~IRenderDevice() = default;

        // ---- frame boundaries ----
        virtual void begin_frame() = 0;
        virtual void end_frame() = 0;

        /// Clear the whole framebuffer to a solid color (called at frame start).
        /// Default no-op so offscreen / recording devices need not implement it.
        virtual void clear(const NanColor& /*color*/) {}

        // ---- clipping (screen-space axis-aligned rect) ----
        /// Set the current clip rect; called by ClipStack, not by nodes directly.
        virtual void set_clip(const NanRect& screen_rect) = 0;
        virtual void clear_clip() = 0;

        // ---- primitives (world-space axis-aligned) ----
        virtual void draw_rect(const NanRect& rect, const NanColor& color) = 0;
        virtual void
        draw_rect_outline(const NanRect& rect, float thickness, const NanColor& color) = 0;
        virtual void
        draw_rounded_rect(const NanRect& rect, float radius, const NanColor& color) = 0;
        virtual void
        draw_line(const NanPoint& a, const NanPoint& b, float thickness, const NanColor& color) = 0;
        virtual void draw_circle(const NanPoint& center, float radius, const NanColor& color) = 0;

        /// Text uses a minimal "top-left anchor + backend default font" interface;
        /// full text layout is abstracted later.
        virtual void draw_text(
            std::string_view text,
            const NanPoint& pos,
            float font_size,
            const NanColor& color
        ) = 0;

        // ---- capability queries (let nodes pick a fallback path) ----
        [[nodiscard]] virtual auto supports_rounded_rect() const -> bool {
            return true;
        }
    };

} // namespace nandina::render

#endif // NANDINA_EXPERIMENT_RENDER_DEVICE_HPP
