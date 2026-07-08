//
// theme/theme — semantic palette + token aggregate.
//

#ifndef NANDINA_EXPERIMENT_THEME_THEME_HPP
#define NANDINA_EXPERIMENT_THEME_THEME_HPP

#include "../foundation/nandina_color.hpp"
#include "tokens.hpp"

namespace nandina::theme
{
    using foundation::NanColor;
    using foundation::NanHexRgb;
    using foundation::NanOklch;

    [[nodiscard]] inline auto nan_color(float light, float chroma, float hue, float alpha = 1.0F)
        -> NanColor {
        return NanColor::from(
            NanOklch {.light = light, .chroma = chroma, .hue = hue, .alpha = alpha}
        );
    }

    struct NanPalette {
        NanColor primary = nan_color(0.62F, 0.18F, 250.0F);
        NanColor on_primary =
            NanColor::from(NanHexRgb {.red = 255, .green = 255, .blue = 255, .alpha = 255});

        NanColor secondary = nan_color(0.68F, 0.13F, 150.0F);
        NanColor on_secondary =
            NanColor::from(NanHexRgb {.red = 18, .green = 28, .blue = 24, .alpha = 255});

        NanColor surface = nan_color(0.33F, 0.03F, 275.0F);
        NanColor on_surface =
            NanColor::from(NanHexRgb {.red = 239, .green = 241, .blue = 250, .alpha = 255});

        NanColor surface_variant = nan_color(0.39F, 0.03F, 276.0F);
        NanColor on_surface_variant =
            NanColor::from(NanHexRgb {.red = 202, .green = 205, .blue = 222, .alpha = 255});

        NanColor outline = nan_color(0.66F, 0.02F, 275.0F);
        NanColor outline_variant = nan_color(0.48F, 0.02F, 275.0F);

        NanColor error = nan_color(0.62F, 0.18F, 25.0F);
        NanColor on_error =
            NanColor::from(NanHexRgb {.red = 255, .green = 250, .blue = 248, .alpha = 255});
    };

    struct NanTheme {
        NanTokens tokens;
        NanPalette palette;
    };

    [[nodiscard]] inline auto default_theme() -> NanTheme {
        return {};
    }

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_THEME_HPP
