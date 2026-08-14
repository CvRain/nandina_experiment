//
// theme/builtin_themes - framework built-in theme families.
//

#include "builtin_themes.hpp"

#include "theme_manager.hpp"

#include <array>
#include <cstdint>
#include <utility>

namespace nandina::theme
{
    namespace
    {
        [[nodiscard]] auto hex_color(const std::uint32_t value) -> NanColor {
            return NanColor::from(NanHexRgb {
                .red = static_cast<std::uint8_t>(value >> 16),
                .green = static_cast<std::uint8_t>(value >> 8),
                .blue = static_cast<std::uint8_t>(value),
                .alpha = 255,
            });
        }

        [[nodiscard]] auto scale(const std::array<std::uint32_t, 11>& values)
            -> std::array<NanColor, 11> {
            std::array<NanColor, 11> result {};
            for (std::size_t i = 0; i < 11; ++i) {
                result[i] = hex_color(values[i]);
            }
            return result;
        }
    } // namespace

    auto default_theme_families() -> std::vector<ThemeFamilyDefinition> {
        // ─── butter：黄油卡片 + Catppuccin（暖奶油中性 + 琥珀主色 + 暖粉辅色）───
        // 中性 ramp 浅端取奶白/米黄，深端取暖棕（「On Accent = Base」取 950 档）。
        // 主色/辅色以 Catppuccin Peach/Rosewater、Flamingo/Maroon 为参考，按暖调微调。
        auto reference = default_reference_palette();
        reference.neutral.stops = scale({
            0xFAF6EC, 0xF2EAD9, 0xE8DCC4, 0xD9C8A8, 0xB3A184,
            0x96856A, 0x7A6B54, 0x5A4F3E, 0x3F372B, 0x2A241B, 0x1C1811,
        });
        reference.primary.stops = scale({
            0xFFF6E2, 0xFFEBC4, 0xFFD997, 0xFCC36C, 0xF5A845,
            0xE78D28, 0xC97217, 0xA15710, 0x7A3E0A, 0x522804, 0x1C1811,
        });
        reference.tertiary.stops = scale({
            0xF9E4E0, 0xF4CCCC, 0xEBA5AE, 0xDD7F8A, 0xD45A68,
            0xC23A4E, 0xA62A3C, 0x841F2F, 0x61151F, 0x3E0C12, 0x24060A,
        });

        NanTokens butter_tokens {};
        butter_tokens.radius.sm = 12.0F;
        butter_tokens.radius.md = 16.0F;
        butter_tokens.radius.lg = 24.0F;
        butter_tokens.border.thin = 2.0F;
        butter_tokens.border.medium = 3.0F;

        std::vector<ThemeFamilyDefinition> families;
        families.push_back(ThemeFamilyDefinition {
            .name = "butter",
            .reference = std::move(reference),
            .policy = {
                .light_brand = ColorShade::shade_500,
                .dark_brand = ColorShade::shade_300,
            },
            .tokens = butter_tokens,
        });
        return families;
    }

    auto build_family_design_system(const ThemeFamilyDefinition& family) -> DesignSystem {
        auto system = default_design_system();
        system.tokens = family.tokens;
        system.light = make_color_scheme(family.reference, ColorAppearance::light, family.policy);
        system.dark = make_color_scheme(family.reference, ColorAppearance::dark, family.policy);
        return system;
    }

    void register_default_theme_families(ThemeManager& manager) {
        for (const auto& family: default_theme_families()) {
            manager.register_theme_family(family.name, build_family_design_system(family));
        }
    }
} // namespace nandina::theme
