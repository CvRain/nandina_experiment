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
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

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

        [[nodiscard]] auto alpha_to_rgba(std::span<const std::uint8_t> alpha)
            -> std::vector<::Color> {
            std::vector<::Color> pixels;
            pixels.reserve(alpha.size());
            for (const auto value: alpha) {
                pixels.push_back(::Color {255, 255, 255, value});
            }
            return pixels;
        }

    } // namespace

    /// raylib-backed device. Stateless w.r.t. traversal; only frame/clip bracketing.
    class RaylibRenderDevice final: public IRenderDevice {
    public:
        ~RaylibRenderDevice() override {
            for (const auto& [handle, texture]: textures_) {
                (void)handle;
                UnloadTexture(texture);
            }
        }

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

        [[nodiscard]] auto supports_alpha_textures() const -> bool override {
            return true;
        }

        [[nodiscard]] auto create_alpha_texture(
            int width,
            int height,
            std::span<const std::uint8_t> alpha
        ) -> TextureHandle override {
            if (width <= 0 || height <= 0
                || alpha.size() != static_cast<std::size_t>(width * height))
            {
                return {};
            }

            auto rgba = alpha_to_rgba(alpha);
            Image image {
                .data = rgba.data(),
                .width = width,
                .height = height,
                .mipmaps = 1,
                .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
            };
            const auto texture = LoadTextureFromImage(image);
            if (texture.id == 0) {
                return {};
            }
            SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);

            const TextureHandle handle {.value = next_texture_handle_++};
            textures_.emplace(handle.value, texture);
            return handle;
        }

        void update_alpha_texture(
            TextureHandle handle,
            int width,
            int height,
            std::span<const std::uint8_t> alpha
        ) override {
            const auto found = textures_.find(handle.value);
            if (found == textures_.end() || found->second.width != width
                || found->second.height != height
                || alpha.size() != static_cast<std::size_t>(width * height))
            {
                return;
            }
            const auto rgba = alpha_to_rgba(alpha);
            UpdateTexture(found->second, rgba.data());
        }

        void destroy_texture(TextureHandle handle) override {
            const auto found = textures_.find(handle.value);
            if (found == textures_.end()) {
                return;
            }
            UnloadTexture(found->second);
            textures_.erase(found);
        }

        void draw_texture_region(
            TextureHandle handle,
            const NanRect& source,
            const NanRect& destination,
            const NanColor& tint
        ) override {
            const auto found = textures_.find(handle.value);
            if (found == textures_.end()) {
                return;
            }
            DrawTexturePro(
                found->second,
                to_rl(source),
                to_rl(destination),
                ::Vector2 {0.0F, 0.0F},
                0.0F,
                to_rl(tint)
            );
        }

    private:
        std::unordered_map<std::uint64_t, ::Texture2D> textures_;
        std::uint64_t next_texture_handle_ = 1;
    };

    auto make_raylib_device() -> std::unique_ptr<IRenderDevice> {
        return std::make_unique<RaylibRenderDevice>();
    }

} // namespace nandina::render
