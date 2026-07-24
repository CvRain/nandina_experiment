//
// theme/theme — reference color scales, semantic color scheme, and token aggregate.
//

#ifndef NANDINA_EXPERIMENT_THEME_THEME_HPP
#define NANDINA_EXPERIMENT_THEME_THEME_HPP

#include "../foundation/nandina_color.hpp"
#include "tokens.hpp"

#include <array>
#include <cstddef>

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

    enum class ColorShade : std::size_t {
        shade_50,
        shade_100,
        shade_200,
        shade_300,
        shade_400,
        shade_500,
        shade_600,
        shade_700,
        shade_800,
        shade_900,
        shade_950,
    };

    struct NanColorScale {
        static constexpr std::size_t stop_count = 11;

        std::array<NanColor, stop_count> stops;

        [[nodiscard]] auto at(ColorShade shade) const noexcept -> const NanColor& {
            return stops[static_cast<std::size_t>(shade)];
        }

        [[nodiscard]] auto at(ColorShade shade) noexcept -> NanColor& {
            return stops[static_cast<std::size_t>(shade)];
        }
    };

    // Reference colors are authoring inputs. Components consume the resolved semantic scheme below.
    struct NanReferencePalette {
        NanColorScale primary;
        NanColorScale secondary;
        NanColorScale tertiary;
        NanColorScale neutral;
        NanColorScale success;
        NanColorScale warning;
        NanColorScale error;
    };

    struct NanColorScheme {
        NanColor background = nan_color(0.27F, 0.025F, 275.0F);
        NanColor on_background =
            NanColor::from(NanHexRgb {.red = 239, .green = 241, .blue = 250, .alpha = 255});

        NanColor primary = nan_color(0.62F, 0.18F, 250.0F);
        NanColor on_primary =
            NanColor::from(NanHexRgb {.red = 255, .green = 255, .blue = 255, .alpha = 255});

        NanColor secondary = nan_color(0.68F, 0.13F, 150.0F);
        NanColor on_secondary =
            NanColor::from(NanHexRgb {.red = 18, .green = 28, .blue = 24, .alpha = 255});

        NanColor tertiary = nan_color(0.68F, 0.16F, 330.0F);
        NanColor on_tertiary =
            NanColor::from(NanHexRgb {.red = 255, .green = 248, .blue = 252, .alpha = 255});

        NanColor surface = nan_color(0.33F, 0.03F, 275.0F);
        NanColor on_surface =
            NanColor::from(NanHexRgb {.red = 239, .green = 241, .blue = 250, .alpha = 255});

        NanColor surface_variant = nan_color(0.39F, 0.03F, 276.0F);
        NanColor on_surface_variant =
            NanColor::from(NanHexRgb {.red = 202, .green = 205, .blue = 222, .alpha = 255});

        NanColor outline = nan_color(0.66F, 0.02F, 275.0F);
        NanColor outline_variant = nan_color(0.48F, 0.02F, 275.0F);

        NanColor success = nan_color(0.72F, 0.13F, 175.0F);
        NanColor on_success = nan_color(0.22F, 0.04F, 175.0F);

        NanColor warning = nan_color(0.82F, 0.14F, 77.0F);
        NanColor on_warning = nan_color(0.28F, 0.06F, 65.0F);

        NanColor error = nan_color(0.62F, 0.18F, 25.0F);
        NanColor on_error =
            NanColor::from(NanHexRgb {.red = 255, .green = 250, .blue = 248, .alpha = 255});

        NanColor focus_ring = primary;
        NanColor selection = primary.with_alpha(0.32F);
    };

    // Compatibility name retained while callers migrate from palette terminology.
    using NanPalette = NanColorScheme;

    struct NanTheme {
        NanTokens tokens;
        NanColorScheme palette;
    };

    [[nodiscard]] inline auto default_theme() -> NanTheme {
        return {};
    }

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_THEME_HPP
