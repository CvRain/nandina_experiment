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
#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
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

    // ─── 抗锯齿图元：SDF 着色器 ──────────────────────────────────────────────
    // raylib 的 2D 图元（DrawRectangleRounded* / DrawLineEx 等）不做抗锯齿，且本平台
    // 默认帧缓冲的 MSAA 实际未生效（斜线仍是硬阶梯）。改用 SDF 片段着色器做逐像素
    // 软边，统一覆盖圆角填充 / 圆角描边 / 矩形 / 圆 / 线段，圆角半径与线宽任意。
    // 顶点着色器沿用 raylib 约定（mvp / texture0 / colDiffuse），由 raylib 批量自动更新。

    constexpr const char* kAaVertexShader = R"GLSL(
#version 330 core
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;
uniform mat4 mvp;
out vec2 fragTexCoord;
out vec4 fragColor;
void main() {
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)GLSL";

    constexpr const char* kAaFragmentShader = R"GLSL(
#version 330 core
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec4 uRect;    // 目标矩形（raylib 屏幕坐标，y 向下）
uniform vec2 uRadius;  // x = 圆角半径；y = 线段 / 描边半宽
uniform vec4 uColor;   // 实心颜色（RGBA 0..1）
uniform int uMode;     // 0 = 填充（圆角矩形 / 矩形 / 圆）；1 = 描边；2 = 线段
uniform vec2 uA;       // 线段端点 A（模式 2）
uniform vec2 uB;       // 线段端点 B（模式 2）
out vec4 finalColor;

float sdRoundRect(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + vec2(r);
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

float sdSegment(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

void main() {
    vec2 p = uRect.xy + fragTexCoord * uRect.zw;
    float alpha;
    if (uMode == 2) {
        float d = sdSegment(p, uA, uB);
        float aa = fwidth(d) * 0.75;
        alpha = 1.0 - smoothstep(uRadius.y - aa, uRadius.y + aa, d);
    }
    else {
        vec2 center = uRect.xy + uRect.zw * 0.5;
        float sd = sdRoundRect(p - center, uRect.zw * 0.5, uRadius.x);
        float aa = fwidth(sd) * 0.75;
        if (uMode == 1) {
            alpha = 1.0 - smoothstep(uRadius.y - aa, uRadius.y + aa, abs(sd));
        }
        else {
            alpha = 1.0 - smoothstep(-aa, aa, sd);
        }
    }
    finalColor = vec4(uColor.rgb, uColor.a * alpha);
}
)GLSL";

    /// raylib-backed device. Stateless w.r.t. traversal; only frame/clip bracketing.
    class RaylibRenderDevice final: public IRenderDevice {
    public:
        ~RaylibRenderDevice() override {
            for (const auto& [handle, texture]: textures_) {
                (void)handle;
                UnloadTexture(texture);
            }
            if (aa_shader_.id != 0) {
                UnloadShader(aa_shader_);
            }
            if (aa_white_tex_.id != 0) {
                UnloadTexture(aa_white_tex_);
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
            if (r.get_width() <= 0.0F || r.get_height() <= 0.0F) {
                return;
            }
            draw_aa(r, 0.0F, 0.0F, 0, to_rl(c));
        }

        void draw_rect_outline(const NanRect& r, float thickness, const NanColor& c) override {
            draw_aa(r, 0.0F, thickness * 0.5F, 1, to_rl(c));
        }

        void draw_rounded_rect_outline(
            const NanRect& r,
            float radius,
            float thickness,
            const NanColor& c
        ) override {
            const float corner =
                std::clamp(radius, 0.0F, std::min(r.get_width(), r.get_height()) * 0.5F);
            draw_aa(r, corner, thickness * 0.5F, 1, to_rl(c));
        }

        void draw_rounded_rect(const NanRect& r, float radius, const NanColor& c) override {
            if (r.get_width() <= 0.0F || r.get_height() <= 0.0F) {
                return;
            }
            const float corner =
                std::clamp(radius, 0.0F, std::min(r.get_width(), r.get_height()) * 0.5F);
            draw_aa(r, corner, 0.0F, 0, to_rl(c));
        }

        void draw_line(
            const NanPoint& a,
            const NanPoint& b,
            float thickness,
            const NanColor& c
        ) override {
            if (thickness <= 0.0F) {
                return;
            }
            const float hw = thickness * 0.5F;
            const float min_x = std::min(a.get_x(), b.get_x()) - hw;
            const float max_x = std::max(a.get_x(), b.get_x()) + hw;
            const float min_y = std::min(a.get_y(), b.get_y()) - hw;
            const float max_y = std::max(a.get_y(), b.get_y()) + hw;
            draw_aa(
                foundation::NanRect::from_xywh(min_x, min_y, max_x - min_x, max_y - min_y),
                0.0F,
                hw,
                2,
                to_rl(c),
                a,
                b
            );
        }

        void draw_circle(const NanPoint& center, float radius, const NanColor& c) override {
            if (radius <= 0.0F) {
                return;
            }
            draw_aa(
                foundation::NanRect::from_xywh(
                    center.get_x() - radius,
                    center.get_y() - radius,
                    radius * 2.0F,
                    radius * 2.0F
                ),
                radius,
                0.0F,
                0,
                to_rl(c)
            );
        }

        /// 懒加载 SDF 抗锯齿着色器与 1x1 白纹理。
        void ensure_aa() {
            if (aa_ready_) {
                return;
            }
            aa_ready_ = true;
            // 注意：LoadShader() 接收的是文件路径；这里必须用 LoadShaderFromMemory()。
            aa_shader_ = LoadShaderFromMemory(kAaVertexShader, kAaFragmentShader);
            if (aa_shader_.id == 0) {
                return;
            }
            aa_loc_rect_ = GetShaderLocation(aa_shader_, "uRect");
            aa_loc_radius_ = GetShaderLocation(aa_shader_, "uRadius");
            aa_loc_color_ = GetShaderLocation(aa_shader_, "uColor");
            aa_loc_mode_ = GetShaderLocation(aa_shader_, "uMode");
            aa_loc_a_ = GetShaderLocation(aa_shader_, "uA");
            aa_loc_b_ = GetShaderLocation(aa_shader_, "uB");
            // 自定义 uniform 找不到 = 回退到了默认着色器（加载失败），标记不可用。
            if (aa_loc_rect_ < 0 || aa_loc_radius_ < 0 || aa_loc_color_ < 0 || aa_loc_mode_ < 0) {
                aa_shader_.id = 0;
                return;
            }
            const std::uint8_t white[4] = {255, 255, 255, 255};
            Image image {
                .data = const_cast<std::uint8_t*>(white),
                .width = 1,
                .height = 1,
                .mipmaps = 1,
                .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
            };
            aa_white_tex_ = LoadTextureFromImage(image);
        }

        /// 着色器不可用时的回退：沿用 raylib 原生无抗锯齿图元。
        void draw_aa_fallback(
            const NanRect& rect,
            const float radius,
            const float half_width,
            const int mode,
            const ::Color& color,
            const std::optional<NanPoint>& line_a,
            const std::optional<NanPoint>& line_b
        ) {
            const float shortest = std::min(rect.get_width(), rect.get_height());
            const float corner = std::clamp(radius, 0.0F, shortest * 0.5F);
            const float roundness = shortest > 0.0F ? corner * 2.0F / shortest : 0.0F;
            if (mode == 2) {
                DrawLineEx(to_rl(*line_a), to_rl(*line_b), half_width * 2.0F, color);
            }
            else if (mode == 1) {
                DrawRectangleRoundedLinesEx(
                    to_rl(rect),
                    roundness,
                    16,
                    half_width * 2.0F,
                    color
                );
            }
            else if (corner > 0.0F) {
                DrawRectangleRounded(to_rl(rect), roundness, 16, color);
            }
            else {
                DrawRectangleRec(to_rl(rect), color);
            }
        }

        /// 用 SDF 着色器绘制一个抗锯齿图元（覆盖矩形 / 圆角矩形 / 圆 / 描边 / 线段）。
        void draw_aa(
            const NanRect& rect,
            const float radius,
            const float half_width,
            const int mode,
            const ::Color& color,
            const std::optional<NanPoint>& line_a = std::nullopt,
            const std::optional<NanPoint>& line_b = std::nullopt
        ) {
            ensure_aa();
            if (aa_shader_.id == 0 || aa_white_tex_.id == 0) {
                draw_aa_fallback(rect, radius, half_width, mode, color, line_a, line_b);
                return;
            }
            const ::Rectangle rl = to_rl(rect);
            const ::Vector4 rgba = ColorNormalize(color);
            const float rect_v[4] = {rl.x, rl.y, rl.width, rl.height};
            const float radius_v[2] = {radius, half_width};
            const float color_v[4] = {rgba.x, rgba.y, rgba.z, rgba.w};
            BeginShaderMode(aa_shader_);
            if (aa_loc_rect_ >= 0) {
                SetShaderValue(aa_shader_, aa_loc_rect_, rect_v, SHADER_UNIFORM_VEC4);
            }
            if (aa_loc_radius_ >= 0) {
                SetShaderValue(aa_shader_, aa_loc_radius_, radius_v, SHADER_UNIFORM_VEC2);
            }
            if (aa_loc_color_ >= 0) {
                SetShaderValue(aa_shader_, aa_loc_color_, color_v, SHADER_UNIFORM_VEC4);
            }
            if (aa_loc_mode_ >= 0) {
                SetShaderValue(aa_shader_, aa_loc_mode_, &mode, SHADER_UNIFORM_INT);
            }
            if (aa_loc_a_ >= 0 && line_a) {
                const float a_v[2] = {line_a->get_x(), line_a->get_y()};
                SetShaderValue(aa_shader_, aa_loc_a_, a_v, SHADER_UNIFORM_VEC2);
            }
            if (aa_loc_b_ >= 0 && line_b) {
                const float b_v[2] = {line_b->get_x(), line_b->get_y()};
                SetShaderValue(aa_shader_, aa_loc_b_, b_v, SHADER_UNIFORM_VEC2);
            }
            DrawTexturePro(
                aa_white_tex_,
                {0.0F, 0.0F, 1.0F, 1.0F},
                rl,
                {0.0F, 0.0F},
                0.0F,
                WHITE
            );
            EndShaderMode();
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

        [[nodiscard]] auto
        create_alpha_texture(int width, int height, std::span<const std::uint8_t> alpha)
            -> TextureHandle override {
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
        /// SDF 抗锯齿着色器与 1x1 白纹理（懒加载）。
        ::Shader aa_shader_ {};
        ::Texture2D aa_white_tex_ {};
        int aa_loc_rect_ = -1;
        int aa_loc_radius_ = -1;
        int aa_loc_color_ = -1;
        int aa_loc_mode_ = -1;
        int aa_loc_a_ = -1;
        int aa_loc_b_ = -1;
        bool aa_ready_ = false;
    };

    auto make_raylib_device() -> std::unique_ptr<IRenderDevice> {
        return std::make_unique<RaylibRenderDevice>();
    }

} // namespace nandina::render
