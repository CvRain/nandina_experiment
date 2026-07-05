//
// Created by cvrain on 2026/7/2.
//
// raylib backend for IRenderDevice. raylib is included ONLY here; all
// nandina<->raylib type conversions live in the anonymous namespace below.
//

#include "raylib_device.hpp"

#include "../../foundation/nandina_color_space.hpp"

#include <raylib.h>

#include <algorithm>
#include <memory>
#include <string>

namespace nandina::render
{
    namespace
    {

        auto to_rl(const NanPoint& p) -> ::Vector2 {
            return {p.get_x(), p.get_y()};
        }

        auto to_rl(const NanColor& c) -> ::Color {
            const auto hex = c.to<foundation::NanHexRgb>();
            return {hex.red, hex.green, hex.blue, hex.alpha};
        }

        auto to_rl(const NanRect& r) -> ::Rectangle {
            return {r.get_left(), r.get_top(), r.get_width(), r.get_height()};
        }

    } // namespace

    /// raylib-backed device. Stateless w.r.t. traversal; only frame/clip bracketing.
    class RaylibRenderDevice final: public IRenderDevice {
    public:
        void begin_frame() override {
            BeginDrawing();
        }
        void end_frame() override {
            EndDrawing();
        }

        void clear(const NanColor& c) override {
            ClearBackground(to_rl(c));
        }

        void set_clip(const NanRect& r) override {
            // raylib scissor doesn't nest (each call overrides). That's fine: the
            // ClipStack always submits the already-intersected top rect.
            if (!r.is_valid()) {
                // Fully clipped: a zero-area scissor produces no visible pixels.
                BeginScissorMode(0, 0, 0, 0);
                return;
            }
            BeginScissorMode(
                static_cast<int>(r.get_left()),
                static_cast<int>(r.get_top()),
                static_cast<int>(r.get_width()),
                static_cast<int>(r.get_height())
            );
        }

        void clear_clip() override {
            EndScissorMode();
        }

        void draw_rect(const NanRect& r, const NanColor& c) override {
            DrawRectangleRec(to_rl(r), to_rl(c));
        }

        void draw_rect_outline(const NanRect& r, float thickness, const NanColor& c) override {
            DrawRectangleLinesEx(to_rl(r), thickness, to_rl(c));
        }

        void draw_rounded_rect(const NanRect& r, float radius, const NanColor& c) override {
            const float shortest = std::min(r.get_width(), r.get_height());
            const float roundness =
                shortest > 0.0F ? std::clamp(radius * 2.0F / shortest, 0.0F, 1.0F) : 0.0F;
            DrawRectangleRounded(to_rl(r), roundness, 8, to_rl(c));
        }

        void draw_line(
            const NanPoint& a,
            const NanPoint& b,
            float thickness,
            const NanColor& c
        ) override {
            DrawLineEx(to_rl(a), to_rl(b), thickness, to_rl(c));
        }

        void draw_circle(const NanPoint& center, float radius, const NanColor& c) override {
            DrawCircleV(to_rl(center), radius, to_rl(c));
        }

        void draw_text(
            std::string_view text,
            const NanPoint& pos,
            float font_size,
            const NanColor& c
        ) override {
            const std::string owned(text);
            DrawText(
                owned.c_str(),
                static_cast<int>(pos.get_x()),
                static_cast<int>(pos.get_y()),
                static_cast<int>(font_size),
                to_rl(c)
            );
        }
    };

    auto make_raylib_device() -> std::unique_ptr<IRenderDevice> {
        return std::make_unique<RaylibRenderDevice>();
    }

} // namespace nandina::render
