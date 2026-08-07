//
// theme/theme — reference color scales, semantic color scheme, and token aggregate.
//

#ifndef NANDINA_EXPERIMENT_THEME_THEME_HPP
#define NANDINA_EXPERIMENT_THEME_THEME_HPP

#include "../foundation/nandina_color.hpp"
#include "appearance.hpp"
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

    /** 从参考色阶生成语义色时的 tone 选择。 */
    struct PaletteVariantPolicy {
        ColorShade light_brand = ColorShade::shade_500;
        ColorShade dark_brand = ColorShade::shade_500;

        /** Material 风格可选项：暗色外观使用更亮的 400 档品牌色。 */
        [[nodiscard]] static constexpr auto material_dark_tone() -> PaletteVariantPolicy {
            return {.light_brand = ColorShade::shade_500, .dark_brand = ColorShade::shade_400};
        }
    };

    struct NanColorScheme {
        NanColorScheme();

        NanColor background;
        NanColor on_background;
        NanColor primary;
        NanColor on_primary;
        NanColor secondary;
        NanColor on_secondary;
        NanColor tertiary;
        NanColor on_tertiary;
        NanColor surface;
        NanColor on_surface;
        NanColor surface_variant;
        NanColor on_surface_variant;
        NanColor outline;
        NanColor outline_variant;
        NanColor success;
        NanColor on_success;
        NanColor warning;
        NanColor on_warning;
        NanColor error;
        NanColor on_error;
        NanColor focus_ring;
        NanColor selection;

    private:
        struct GeneratedTag {};

        NanColorScheme(
            const NanReferencePalette& reference,
            ColorAppearance appearance,
            PaletteVariantPolicy policy,
            GeneratedTag
        );

        friend auto make_color_scheme(
            const NanReferencePalette& reference,
            ColorAppearance appearance,
            PaletteVariantPolicy policy
        ) -> NanColorScheme;
    };

    // Compatibility name retained while callers migrate from palette terminology.
    using NanPalette = NanColorScheme;

    struct NanTheme {
        NanTokens tokens;
        NanColorScheme palette;
    };

    /** @return 框架内置的七组 11 档参考色阶。 */
    [[nodiscard]] auto default_reference_palette() -> NanReferencePalette;

    /** 由参考色阶和外观策略生成组件消费的语义色板。 */
    [[nodiscard]] auto make_color_scheme(
        const NanReferencePalette& reference,
        ColorAppearance appearance,
        PaletteVariantPolicy policy = {}
    ) -> NanColorScheme;

    [[nodiscard]] auto default_light_palette() -> NanColorScheme;
    [[nodiscard]] auto default_dark_palette() -> NanColorScheme;

    [[nodiscard]] auto default_theme() -> NanTheme;

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_THEME_HPP
