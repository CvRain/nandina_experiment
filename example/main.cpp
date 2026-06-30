#include <raylib.h>
#include <spdlog/spdlog.h>

#include "foundation/geometry.hpp"
#include "foundation/nandina_color.hpp"
#include "foundation/transform2d.hpp"

using namespace nandina;

namespace
{
    [[nodiscard]] auto to_raylib_color(const nandina::foundation::NanColor& color) -> Color {
        const auto rgba = color.to<nandina::foundation::NanHexRgb>();
        return {
            rgba.red,
            rgba.green,
            rgba.blue,
            rgba.alpha,
        };
    }

    auto draw_rect_bounds(const foundation::NanRect& rect, Color color) -> void {
        DrawRectangleLines(
            static_cast<int>(rect.get_x()),
            static_cast<int>(rect.get_y()),
            static_cast<int>(rect.get_width()),
            static_cast<int>(rect.get_height()),
            color
        );
    }

    auto fill_rect(const foundation::NanRect& rect, Color color) -> void {
        DrawRectangle(
            static_cast<int>(rect.get_x()),
            static_cast<int>(rect.get_y()),
            static_cast<int>(rect.get_width()),
            static_cast<int>(rect.get_height()),
            color
        );
    }

} // namespace

auto main() -> int {
    spdlog::info("Nandina geometry demo");

    constexpr int screen_width = 1024;
    constexpr int screen_height = 640;

    InitWindow(screen_width, screen_height, "Nandina — Geometry Demo");
    SetTargetFPS(60);

    // ---- colors ----
    const auto primary = foundation::NanColor::from(
        foundation::NanOklch {.light = 0.62F, .chroma = 0.18F, .hue = 250.0F, .alpha = 1.0F}
    );
    const auto surface = foundation::NanColor::from(
        foundation::NanHexRgb {.red = 255, .green = 255, .blue = 255, .alpha = 1}
    );
    const auto bg = foundation::NanColor::from(
        foundation::NanHexRgb {.red = 239, .green = 241, .blue = 245, .alpha = 1}
    );
    const auto accent = primary.rotate_hue(95.0F).saturate(0.03F);
    const auto muted = primary.lighten(0.12F).desaturate(0.10F);

    const Color c_primary = to_raylib_color(primary);
    const Color c_surface = to_raylib_color(surface);
    const Color c_bg = to_raylib_color(bg);
    const Color c_accent = to_raylib_color(accent);
    const Color c_muted = to_raylib_color(muted);
    const Color c_text = {30, 30, 40, 255};
    const Color c_subtle = {120, 120, 140, 255};
    const Color c_border = {200, 205, 215, 255};

    // ---- layout demo: panel with padding containing two columns ----
    const auto panel_rect = foundation::NanRect::from_xywh(24, 52, 460, 540);
    const auto panel_padding = foundation::NanInsets::all(16);
    const auto content_rect = panel_padding.apply_to_rect(panel_rect);
    const auto col_gap = foundation::NanInsets::horizontal(12);

    const auto col_width = (content_rect.get_width() - col_gap.horizontal_sum()) / 2.0F;
    const auto col_left = foundation::NanRect::from_xywh(
        content_rect.get_x(),
        content_rect.get_y(),
        col_width,
        content_rect.get_height()
    );
    const auto col_right = foundation::NanRect::from_xywh(
        content_rect.get_x() + col_width + col_gap.get_left(),
        content_rect.get_y(),
        col_width,
        content_rect.get_height()
    );

    const auto row_height = 44.0F;
    const auto row_gap = 8.0F;
    const auto row_label_inset = foundation::NanInsets::only_left(8);

    // ---- vector math demo ----
    const auto pt_a = foundation::NanPoint(640, 180);
    const auto pt_b = foundation::NanPoint(880, 420);
    const auto pt_a_size = foundation::NanSize(12, 12);
    const auto pt_b_size = foundation::NanSize(16, 16);

    // ---- animation state ----
    float anim_t = 0.0F;
    float anim_dir = 1.0F;

    auto draw_row =
        [&](const foundation::NanRect& bounds, const char* label, const char* value, Color fill) {
            fill_rect(bounds, fill);
            draw_rect_bounds(bounds, c_border);
            const auto label_rect = row_label_inset.apply_to_rect(bounds);
            DrawText(
                label,
                static_cast<int>(label_rect.get_x()),
                static_cast<int>(label_rect.get_y() + (label_rect.get_height() - 14) / 2.0F),
                14,
                c_text
            );
            DrawText(
                value,
                static_cast<int>(label_rect.get_x() + 100),
                static_cast<int>(label_rect.get_y() + (label_rect.get_height() - 14) / 2.0F),
                14,
                c_subtle
            );
        };

        while (not WindowShouldClose()) {
            // -- animate lerp --
            anim_t += 0.008F * anim_dir;
                if (anim_t >= 1.0F || anim_t <= 0.0F) {
                    anim_dir = -anim_dir;
                    anim_t = std::clamp(anim_t, 0.0F, 1.0F);
                }

            const auto pt_lerp = foundation::NanPoint::lerp(pt_a, pt_b, anim_t);
            const auto pt_mid = foundation::NanPoint::lerp(pt_a, pt_b, 0.5F);
            const auto pt_dir = (pt_b - pt_a).normalized();

            BeginDrawing();
            ClearBackground(c_bg);

            // ===== TITLE =====
            DrawText("Nandina — Geometry Demo", 24, 16, 22, c_text);

            // ===== LEFT: Rect / Layout =====
            fill_rect(panel_rect, c_surface);
            draw_rect_bounds(panel_rect, c_border);

            // column headers
            DrawText(
                "NanSize",
                static_cast<int>(col_left.get_x() + 8),
                static_cast<int>(col_left.get_y() + 4),
                14,
                c_subtle
            );
            DrawText(
                "NanInsets",
                static_cast<int>(col_right.get_x() + 8),
                static_cast<int>(col_right.get_y() + 4),
                14,
                c_subtle
            );

            auto row_y = col_left.get_y() + 24;

            draw_row(
                foundation::NanRect::from_xywh(col_left.get_x(), row_y, col_width, row_height),
                "zero()",
                "(0, 0)",
                c_muted
            );
            row_y += row_height + row_gap;
            draw_row(
                foundation::NanRect::from_xywh(col_left.get_x(), row_y, col_width, row_height),
                "aspect_ratio",
                "0.714",
                c_muted
            );
            row_y += row_height + row_gap;
            draw_row(
                foundation::NanRect::from_xywh(col_left.get_x(), row_y, col_width, row_height),
                "area()",
                "28000",
                c_muted
            );
            row_y += row_height + row_gap;
            draw_row(
                foundation::NanRect::from_xywh(col_left.get_x(), row_y, col_width, row_height),
                "scaled(2.0)",
                "400x280",
                c_muted
            );
            row_y += row_height + row_gap;
            draw_row(
                foundation::NanRect::from_xywh(col_left.get_x(), row_y, col_width, row_height),
                "constrain",
                "250x80→250x80",
                c_muted
            );
            row_y += row_height + row_gap;
            draw_row(
                foundation::NanRect::from_xywh(col_left.get_x(), row_y, col_width, row_height),
                "max(a,b)",
                "comp-wise",
                c_muted
            );
            row_y += row_height + row_gap;
            draw_row(
                foundation::NanRect::from_xywh(col_left.get_x(), row_y, col_width, row_height),
                "fits_in()",
                "bool",
                c_muted
            );

            row_y = col_right.get_y() + 24;
            draw_row(
                foundation::NanRect::from_xywh(col_right.get_x(), row_y, col_width, row_height),
                "all(16)",
                "lrtb::16",
                c_muted
            );
            row_y += row_height + row_gap;
            draw_row(
                foundation::NanRect::from_xywh(col_right.get_x(), row_y, col_width, row_height),
                "symmetric",
                "h:12 v:8",
                c_muted
            );
            row_y += row_height + row_gap;
            draw_row(
                foundation::NanRect::from_xywh(col_right.get_x(), row_y, col_width, row_height),
                "only_left",
                "l:8 rtb:0",
                c_muted
            );
            row_y += row_height + row_gap;
            draw_row(
                foundation::NanRect::from_xywh(col_right.get_x(), row_y, col_width, row_height),
                "h_sum",
                "left+right",
                c_muted
            );
            row_y += row_height + row_gap;
            draw_row(
                foundation::NanRect::from_xywh(col_right.get_x(), row_y, col_width, row_height),
                "v_sum",
                "top+bottom",
                c_muted
            );
            row_y += row_height + row_gap;
            draw_row(
                foundation::NanRect::from_xywh(col_right.get_x(), row_y, col_width, row_height),
                "operator*",
                "scale 4 sides",
                c_muted
            );
            row_y += row_height + row_gap;
            draw_row(
                foundation::NanRect::from_xywh(col_right.get_x(), row_y, col_width, row_height),
                "lerp(a,b,t)",
                "interp",
                c_muted
            );

            // ===== RIGHT: Vector Math =====
            // draw the line between A and B
            DrawLine(
                static_cast<int>(pt_a.get_x()),
                static_cast<int>(pt_a.get_y()),
                static_cast<int>(pt_b.get_x()),
                static_cast<int>(pt_b.get_y()),
                {180, 190, 210, 255}
            );

            // draw anchor points
            DrawCircle(
                static_cast<int>(pt_a.get_x()),
                static_cast<int>(pt_a.get_y()),
                static_cast<int>(pt_a_size.get_width() / 2),
                c_primary
            );
            DrawCircle(
                static_cast<int>(pt_b.get_x()),
                static_cast<int>(pt_b.get_y()),
                static_cast<int>(pt_b_size.get_width() / 2),
                c_accent
            );

            // draw lerp point
            DrawCircle(
                static_cast<int>(pt_lerp.get_x()),
                static_cast<int>(pt_lerp.get_y()),
                8,
                {240, 80, 80, 255}
            );

            // labels
            DrawText(
                "A",
                static_cast<int>(pt_a.get_x()) - 20,
                static_cast<int>(pt_a.get_y()) - 20,
                14,
                c_text
            );
            DrawText(
                "B",
                static_cast<int>(pt_b.get_x()) + 8,
                static_cast<int>(pt_b.get_y()) + 8,
                14,
                c_text
            );
            DrawText(
                TextFormat("lerp(%.2f)", anim_t),
                static_cast<int>(pt_lerp.get_x()) + 12,
                static_cast<int>(pt_lerp.get_y()) - 8,
                14,
                c_text
            );

            // ===== Transform2D demo =====
            const auto t_parent = foundation::NanTransform2D(
                foundation::NanPoint(700, 200),
                anim_t * std::numbers::pi_v<float> * 2.0F,
                foundation::NanPoint(1.0F, 1.0F)
            );

            const auto t_child = foundation::NanTransform2D(
                foundation::NanPoint(60, 0),
                anim_t * std::numbers::pi_v<float>,
                foundation::NanPoint(0.6F, 0.6F)
            );

            const auto t_world = t_parent * t_child;

            // draw parent
            const auto parent_local = foundation::NanRect::from_xywh(-30, -20, 60, 40);
            const auto p_tl = t_parent.transform_point(parent_local.get_top_left());
            const auto p_size = foundation::NanSize(
                parent_local.get_width() * t_parent.scale_x(),
                parent_local.get_height() * t_parent.scale_y()
            );
            DrawRectanglePro(
                {p_tl.get_x(), p_tl.get_y(), p_size.get_width(), p_size.get_height()},
                {0, 0},
                t_parent.rotation() * 180.0F / std::numbers::pi_v<float>,
                c_primary
            );
            DrawText(
                "parent",
                static_cast<int>(p_tl.get_x() + p_size.get_width() / 2 - 22),
                static_cast<int>(p_tl.get_y() + p_size.get_height() / 2 - 8),
                12,
                BLACK
            );

            // draw child
            const auto child_local = foundation::NanRect::from_xywh(-16, -12, 32, 24);
            const auto c_tl = t_world.transform_point(child_local.get_top_left());
            const auto c_size = foundation::NanSize(
                child_local.get_width() * t_world.scale_x(),
                child_local.get_height() * t_world.scale_y()
            );
            DrawRectanglePro(
                {c_tl.get_x(), c_tl.get_y(), c_size.get_width(), c_size.get_height()},
                {0, 0},
                t_world.rotation() * 180.0F / std::numbers::pi_v<float>,
                c_accent
            );
            DrawText(
                "child",
                static_cast<int>(c_tl.get_x() + c_size.get_width() / 2 - 18),
                static_cast<int>(c_tl.get_y() + c_size.get_height() / 2 - 8),
                12,
                BLACK
            );

            // line from parent center to child center
            const auto p_center = t_parent.transform_point(foundation::NanPoint(0, 0));
            const auto c_center = t_world.transform_point(foundation::NanPoint(0, 0));
            DrawLine(
                static_cast<int>(p_center.get_x()),
                static_cast<int>(p_center.get_y()),
                static_cast<int>(c_center.get_x()),
                static_cast<int>(c_center.get_y()),
                {180, 190, 210, 180}
            );

            // Transform2D info panel
            const auto t_info_rect = foundation::NanRect::from_xywh(520, 310, 480, 60);
            fill_rect(t_info_rect, c_surface);
            draw_rect_bounds(t_info_rect, c_border);
            DrawText(
                "NanTransform2D — parent * child",
                static_cast<int>(t_info_rect.get_x() + 10),
                static_cast<int>(t_info_rect.get_y() + 6),
                13,
                c_text
            );
            DrawText(
                TextFormat(
                    "  compose: pos=(%.0f,%.0f) rot=%.2frad  scale=(%.1f,%.1f)",
                    t_world.position().get_x(),
                    t_world.position().get_y(),
                    t_world.rotation(),
                    t_world.scale_x(),
                    t_world.scale_y()
                ),
                static_cast<int>(t_info_rect.get_x() + 10),
                static_cast<int>(t_info_rect.get_y() + 26),
                12,
                c_subtle
            );
            DrawText(
                TextFormat(
                    "  inverse: pos=(%.0f,%.0f)",
                    t_world.inverse().position().get_x(),
                    t_world.inverse().position().get_y()
                ),
                static_cast<int>(t_info_rect.get_x() + 10),
                static_cast<int>(t_info_rect.get_y() + 42),
                12,
                c_subtle
            );

            // vector info panel
            const auto vec_info_rect = foundation::NanRect::from_xywh(520, 420, 480, 160);
            fill_rect(vec_info_rect, c_surface);
            draw_rect_bounds(vec_info_rect, c_border);

            const auto vec_info_pad = foundation::NanInsets::all(12);
            const auto vec_info_content = vec_info_pad.apply_to_rect(vec_info_rect);
            auto vx = vec_info_content.get_x();
            auto vy = vec_info_content.get_y();

            const auto ab_vec = pt_b - pt_a;
            DrawText(
                "NanPoint — Vector Math",
                static_cast<int>(vx),
                static_cast<int>(vy),
                14,
                c_text
            );
            vy += 22;
            DrawText(
                TextFormat(
                    "  A = (%.0f, %.0f)    B = (%.0f, %.0f)",
                    pt_a.get_x(),
                    pt_a.get_y(),
                    pt_b.get_x(),
                    pt_b.get_y()
                ),
                static_cast<int>(vx),
                static_cast<int>(vy),
                14,
                c_subtle
            );
            vy += 20;
            DrawText(
                TextFormat(
                    "  AB = (%.1f, %.1f)    len = %.1f    len^2 = %.1f",
                    ab_vec.get_x(),
                    ab_vec.get_y(),
                    ab_vec.length(),
                    ab_vec.length_squared()
                ),
                static_cast<int>(vx),
                static_cast<int>(vy),
                14,
                c_subtle
            );
            vy += 18;
            DrawText(
                TextFormat(
                    "  dir = (%.2f, %.2f)    dot(A,B) = %.1f    cross = %.1f",
                    pt_dir.get_x(),
                    pt_dir.get_y(),
                    pt_a.dot(pt_b),
                    pt_a.cross(pt_b)
                ),
                static_cast<int>(vx),
                static_cast<int>(vy),
                14,
                c_subtle
            );
            vy += 18;
            DrawText(
                TextFormat(
                    "  dist = %.1f    mid = (%.0f, %.0f)",
                    pt_a.distance(pt_b),
                    pt_mid.get_x(),
                    pt_mid.get_y()
                ),
                static_cast<int>(vx),
                static_cast<int>(vy),
                14,
                c_subtle
            );

            EndDrawing();
        }

    CloseWindow();
    return 0;
}
