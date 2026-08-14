//
// theme/builtin_themes - framework built-in theme families.
//

#include "builtin_themes.hpp"

#include "theme_manager.hpp"

#include <utility>

namespace nandina::theme
{
    auto default_theme_families() -> std::vector<ThemeFamilyDefinition> {
        // ─── butter：黄油卡片 + Catppuccin（暖奶油中性 + 琥珀主色 + 暖粉辅色）───
        // 色阶按 50 → 950（浅 → 深）命名；950 档 = base，实现「On Accent = Base」。
        // 主色/辅色以 Catppuccin Peach/Rosewater、Flamingo/Maroon 为参考做暖调微调。
        auto reference = default_reference_palette();
        reference.neutral = nan_color_scale({
            .shade_50 = 0xFAF6EC,  // 奶白（背景）
            .shade_100 = 0xF2EAD9, // 米黄（表面）
            .shade_200 = 0xE8DCC4, // 浅米（表面变体）
            .shade_300 = 0xD9C8A8, // 米
            .shade_400 = 0xB3A184, // 暖灰
            .shade_500 = 0x96856A, // 暖灰（outline）
            .shade_600 = 0x7A6B54, // 暖灰深
            .shade_700 = 0x5A4F3E, // 暖深灰（次文本）
            .shade_800 = 0x3F372B, // 深暖
            .shade_900 = 0x2A241B, // 近黑暖（暗表面）
            .shade_950 = 0x1C1811, // 最深暖（文字 / base）
        });
        reference.primary = nan_color_scale({
            .shade_50 = 0xFFF6E2,  // 奶油
            .shade_100 = 0xFFEBC4, // 浅黄
            .shade_200 = 0xFFD997,
            .shade_300 = 0xFCC36C, // 暗色品牌（Peach 提亮）
            .shade_400 = 0xF5A845,
            .shade_500 = 0xE78D28, // 亮色品牌（琥珀）
            .shade_600 = 0xC97217,
            .shade_700 = 0xA15710,
            .shade_800 = 0x7A3E0A,
            .shade_900 = 0x522804,
            .shade_950 = 0x1C1811, // on-primary（= base）
        });
        reference.tertiary = nan_color_scale({
            .shade_50 = 0xF9E4E0,
            .shade_100 = 0xF4CCCC,
            .shade_200 = 0xEBA5AE,
            .shade_300 = 0xDD7F8A, // 暗色辅色（Flamingo 提亮）
            .shade_400 = 0xD45A68,
            .shade_500 = 0xC23A4E, // 亮色辅色（Maroon）
            .shade_600 = 0xA62A3C,
            .shade_700 = 0x841F2F,
            .shade_800 = 0x61151F,
            .shade_900 = 0x3E0C12,
            .shade_950 = 0x24060A,
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
            .policy =
                {
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
