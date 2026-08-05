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
        // 内嵌默认 = 亮色方案（对齐 Skeleton 参考，见 dev-docs-v3/phase7 Step 2）。
        // 品牌色两模式同值（500 档），明暗差异集中在中性色。

        // 背景 / 文本（surface 色阶）
        NanColor background = nan_color(1.0000F, 0.00F, 0.0F);       // surface-50
        NanColor on_background = nan_color(0.1776F, 0.00F, 0.0F);    // surface-950

        NanColor primary = nan_color(0.6803F, 0.12F, 39.30F);        // primary-500
        NanColor on_primary = nan_color(0.2949F, 0.11F, 32.48F);     // primary-950

        NanColor secondary = nan_color(0.4907F, 0.23F, 300.46F);     // secondary-500
        NanColor on_secondary = nan_color(0.8666F, 0.05F, 300.15F);  // secondary-50

        NanColor tertiary = nan_color(0.6454F, 0.26F, 2.48F);        // tertiary-500
        NanColor on_tertiary = nan_color(0.9073F, 0.08F, 328.92F);   // tertiary-50

        NanColor surface = nan_color(0.9067F, 0.00F, 0.0F);          // surface-100（轻微抬升）
        NanColor on_surface = nan_color(0.1776F, 0.00F, 0.0F);       // surface-950

        NanColor surface_variant = nan_color(0.8141F, 0.00F, 0.0F);  // surface-200
        NanColor on_surface_variant = nan_color(0.3867F, 0.00F, 0.0F); // surface-700

        NanColor outline = nan_color(0.5103F, 0.00F, 0.0F);          // surface-500
        NanColor outline_variant = nan_color(0.7155F, 0.00F, 0.0F);  // surface-300

        NanColor success = nan_color(0.8291F, 0.13F, 174.95F);       // success-500
        NanColor on_success = nan_color(0.2727F, 0.04F, 185.29F);    // success-950

        NanColor warning = nan_color(0.8246F, 0.14F, 76.71F);        // warning-500
        NanColor on_warning = nan_color(0.5169F, 0.13F, 51.44F);     // warning-950

        NanColor error = nan_color(0.6372F, 0.22F, 28.71F);          // error-500
        NanColor on_error = nan_color(0.8999F, 0.04F, 14.04F);       // error-50

        NanColor focus_ring = primary;
        NanColor selection = primary.with_alpha(0.32F);
    };

    // Compatibility name retained while callers migrate from palette terminology.
    using NanPalette = NanColorScheme;

    struct NanTheme {
        NanTokens tokens;
        NanColorScheme palette;
    };

    /** @return 框架默认亮色语义色板（即 NanColorScheme{} 内嵌默认）。 */
    [[nodiscard]] inline auto default_light_palette() -> NanColorScheme {
        return {};
    }

    /** @return 框架默认暗色语义色板（品牌色同亮色，中性色翻转）。 */
    [[nodiscard]] inline auto default_dark_palette() -> NanColorScheme {
        NanColorScheme palette;
        // 背景 / 文本翻转
        palette.background = nan_color(0.1776F, 0.00F, 0.0F);        // surface-950
        palette.on_background = nan_color(1.0000F, 0.00F, 0.0F);     // surface-50
        // 表面向亮抬升（暗色模式下「抬升」= 更亮）
        palette.surface = nan_color(0.2520F, 0.00F, 0.0F);           // surface-900
        palette.on_surface = nan_color(1.0000F, 0.00F, 0.0F);        // surface-50
        palette.surface_variant = nan_color(0.3211F, 0.00F, 0.0F);   // surface-800
        palette.on_surface_variant = nan_color(0.6167F, 0.00F, 0.0F); // surface-400
        // 边框
        palette.outline = nan_color(0.4495F, 0.00F, 0.0F);           // surface-600
        palette.outline_variant = nan_color(0.3211F, 0.00F, 0.0F);   // surface-800
        return palette;
    }

    [[nodiscard]] inline auto default_theme() -> NanTheme {
        return {};
    }

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_THEME_HPP
