#include <raylib.h>
#include <spdlog/spdlog.h>

#include "foundation/nandina_color.hpp"

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
} // namespace

auto main() -> int {
    spdlog::info("Nandina experiment");

    constexpr int screen_width = 800;
    constexpr int screen_height = 600;

    InitWindow(screen_width, screen_height, "Nandina experiment");

    SetTargetFPS(60);

    const auto base_color = nandina::foundation::NanColor::from(
        nandina::foundation::NanOklch {
            .light = 0.62F,
            .chroma = 0.18F,
            .hue = 250.0F,
            .alpha = 1.0F,
        }
    );

    const auto light_color = base_color.lighten(0.15F);
    const auto dark_color = base_color.darken(0.18F);
    const auto accent_color = base_color.rotate_hue(95.0F).saturate(0.03F);

    const auto background_color = foundation::NanColor::from(
        foundation::NanHexRgb {.red = 48, .green = 52, .blue = 70, .alpha = 1}
    );

        while (not WindowShouldClose()) {
            BeginDrawing();
            ClearBackground(to_raylib_color(background_color));

            DrawRectangle(80, 120, 160, 280, to_raylib_color(dark_color));
            DrawRectangle(260, 120, 160, 280, to_raylib_color(base_color));
            DrawRectangle(440, 120, 160, 280, to_raylib_color(light_color));
            DrawRectangle(620, 120, 100, 280, to_raylib_color(accent_color));

            DrawText("NanColor OKLCH conversion demo", 80, 70, 24, BLACK);
            DrawText("dark", 80, 420, 20, BLACK);
            DrawText("base", 260, 420, 20, BLACK);
            DrawText("light", 440, 420, 20, BLACK);
            DrawText("hue", 620, 420, 20, BLACK);

            EndDrawing();
        }

    CloseWindow();
    return 0;
}
