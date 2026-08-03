/**
 * theme/design_system — 组件级解析与框架默认配方。
 *
 * 解析策略（迁移期）：
 *   1. 遗留平铺解析器（resolve_button_style 等）给出 base 语义，保证
 *      tone / treatment / size / state 的行为与现状一致；
 *   2. 应用 DesignSystem 配方书中的规则覆盖（后匹配者胜）；
 *   3. 组装为片段组合的解析结果（ResolvedButtonStyle 等）。
 *
 * 下一迁移步骤：把平铺解析器内部的语义（disabled 透明度、hover/pressed 覆盖、
 * size 高度等）搬进 default_design_system() 的 base + rules，使配方成为唯一事实来源。
 */

#include "design_system.hpp"

namespace nandina::theme
{
    namespace
    {
        /** 以当前外观构造 NanTheme 视图，复用遗留 token 解析。 */
        [[nodiscard]] auto theme_view(const DesignSystem& system, const ColorAppearance appearance)
            -> NanTheme {
            return NanTheme {system.tokens, system.palette(appearance)};
        }
    } // namespace

    /**
     * 解析 Button 配方。
     *
     * @param system     设计系统快照
     * @param appearance 当前外观
     * @param tone       语义色家族
     * @param treatment  视觉处理方式
     * @param size       尺寸档位
     * @param state      交互状态
     * @return 片段组合的解析结果
     */
    auto resolve_button(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const ButtonTone tone,
        const ButtonTreatment treatment,
        const ButtonSize size,
        const ButtonVisualState state
    ) -> ResolvedButtonStyle {
        auto flat = resolve_button_style(theme_view(system, appearance), tone, treatment, size, state);
        for (const auto& rule: system.components.button.rules) {
            if (!rule.selector.matches(tone, treatment, size, state)) {
                continue;
            }
            if (rule.container_fill) {
                flat.background = resolve_color(system, appearance, *rule.container_fill);
            }
            if (rule.container_border) {
                flat.border_color = resolve_color(system, appearance, *rule.container_border);
            }
            if (rule.container_border_width) {
                flat.border_width = resolve_scalar(system, appearance, *rule.container_border_width);
            }
            if (rule.container_radius) {
                flat.radius = resolve_scalar(system, appearance, *rule.container_radius);
            }
            if (rule.label_color) {
                flat.foreground = resolve_color(system, appearance, *rule.label_color);
            }
            if (rule.label_font_size) {
                flat.font_size = resolve_scalar(system, appearance, *rule.label_font_size);
            }
            if (rule.focus_ring_color) {
                flat.focus_ring_color = resolve_color(system, appearance, *rule.focus_ring_color);
            }
            if (rule.focus_ring_width) {
                flat.focus_ring_width = resolve_scalar(system, appearance, *rule.focus_ring_width);
            }
            if (rule.metrics_height) {
                flat.height = resolve_scalar(system, appearance, *rule.metrics_height);
            }
            if (rule.metrics_padding_x) {
                flat.padding_x = resolve_scalar(system, appearance, *rule.metrics_padding_x);
            }
        }
        return {
            .container = ResolvedBoxStyle {
                .fill = flat.background,
                .border = flat.border_color,
                .border_width = flat.border_width,
                .radius = flat.radius,
            },
            .label = ResolvedTypeStyle {.color = flat.foreground, .font_size = flat.font_size},
            .focus = ResolvedFocusRing {.color = flat.focus_ring_color, .width = flat.focus_ring_width},
            .metrics = ResolvedControlMetrics {
                .height = flat.height,
                .padding_x = flat.padding_x,
                .gap = 0.0F,
                .min_height = 0.0F,
                .box_size = 0.0F,
                .preferred_width = 0.0F,
            },
        };
    }

    /**
     * 解析 Checkbox 配方。
     *
     * @param system     设计系统快照
     * @param appearance 当前外观
     * @param checked    勾选状态（决定指示器 filled / outline）
     * @param state      交互状态
     * @return 片段组合的解析结果
     */
    auto resolve_checkbox(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const bool checked,
        const CheckboxVisualState state
    ) -> ResolvedCheckboxStyle {
        auto flat = resolve_checkbox_style(theme_view(system, appearance), checked, state);
        for (const auto& rule: system.components.checkbox.rules) {
            if (rule.checked && *rule.checked != checked) {
                continue;
            }
            if (rule.state && *rule.state != state) {
                continue;
            }
            if (rule.indicator_fill) {
                flat.box_background = resolve_color(system, appearance, *rule.indicator_fill);
            }
            if (rule.indicator_border) {
                flat.border_color = resolve_color(system, appearance, *rule.indicator_border);
            }
            if (rule.indicator_border_width) {
                flat.border_width = resolve_scalar(system, appearance, *rule.indicator_border_width);
            }
            if (rule.indicator_radius) {
                flat.radius = resolve_scalar(system, appearance, *rule.indicator_radius);
            }
            if (rule.label_color) {
                flat.foreground = resolve_color(system, appearance, *rule.label_color);
            }
            if (rule.label_font_size) {
                flat.font_size = resolve_scalar(system, appearance, *rule.label_font_size);
            }
            if (rule.focus_ring_color) {
                flat.focus_ring_color = resolve_color(system, appearance, *rule.focus_ring_color);
            }
            if (rule.focus_ring_width) {
                flat.focus_ring_width = resolve_scalar(system, appearance, *rule.focus_ring_width);
            }
            if (rule.metrics_gap) {
                flat.gap = resolve_scalar(system, appearance, *rule.metrics_gap);
            }
            if (rule.metrics_box_size) {
                flat.box_size = resolve_scalar(system, appearance, *rule.metrics_box_size);
            }
        }
        return {
            .indicator = ResolvedBoxStyle {
                .fill = flat.box_background,
                .border = flat.border_color,
                .border_width = flat.border_width,
                .radius = flat.radius,
            },
            .check = flat.check_color,
            .label = ResolvedTypeStyle {.color = flat.foreground, .font_size = flat.font_size},
            .focus = ResolvedFocusRing {.color = flat.focus_ring_color, .width = flat.focus_ring_width},
            .metrics = ResolvedControlMetrics {
                .height = 0.0F,
                .padding_x = 0.0F,
                .gap = flat.gap,
                .min_height = flat.min_height,
                .box_size = flat.box_size,
                .preferred_width = 0.0F,
            },
        };
    }

    /**
     * 解析 Slider 配方。
     *
     * @param system     设计系统快照
     * @param appearance 当前外观
     * @param state      交互状态
     * @return 片段组合的解析结果
     */
    auto resolve_slider(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const SliderVisualState state
    ) -> ResolvedSliderStyle {
        auto flat = resolve_slider_style(theme_view(system, appearance), state);
        for (const auto& rule: system.components.slider.rules) {
            if (rule.state && *rule.state != state) {
                continue;
            }
            if (rule.track_inactive_fill) {
                flat.inactive_track = resolve_color(system, appearance, *rule.track_inactive_fill);
            }
            if (rule.track_active_fill) {
                flat.active_track = resolve_color(system, appearance, *rule.track_active_fill);
            }
            if (rule.track_thickness) {
                flat.track_height = resolve_scalar(system, appearance, *rule.track_thickness);
            }
            if (rule.thumb_fill) {
                flat.thumb = resolve_color(system, appearance, *rule.thumb_fill);
            }
            if (rule.thumb_radius) {
                flat.thumb_radius = resolve_scalar(system, appearance, *rule.thumb_radius);
            }
            if (rule.focus_ring_color) {
                flat.focus_ring = resolve_color(system, appearance, *rule.focus_ring_color);
            }
            if (rule.focus_ring_width) {
                flat.focus_ring_width = resolve_scalar(system, appearance, *rule.focus_ring_width);
            }
        }
        return {
            .inactive_track = ResolvedTrackStyle {
                .box = ResolvedBoxStyle {
                    .fill = flat.inactive_track,
                    .border = flat.inactive_track,
                    .border_width = 0.0F,
                    .radius = flat.track_height * 0.5F, // pill 形轨道
                },
                .thickness = flat.track_height,
            },
            .active_track = ResolvedTrackStyle {
                .box = ResolvedBoxStyle {
                    .fill = flat.active_track,
                    .border = flat.active_track,
                    .border_width = 0.0F,
                    .radius = flat.track_height * 0.5F,
                },
                .thickness = flat.track_height,
            },
            .thumb = ResolvedThumbStyle {
                .box = ResolvedBoxStyle {
                    .fill = flat.thumb,
                    .border = flat.thumb,
                    .border_width = 0.0F,
                    .radius = flat.thumb_radius,
                },
            },
            .focus = ResolvedFocusRing {.color = flat.focus_ring, .width = flat.focus_ring_width},
            .metrics = ResolvedControlMetrics {
                .height = 0.0F,
                .padding_x = 0.0F,
                .gap = 0.0F,
                .min_height = flat.min_height,
                .box_size = 0.0F,
                .preferred_width = flat.preferred_width,
            },
        };
    }

    /**
     * 解析 TextField 配方。
     *
     * @param system     设计系统快照
     * @param appearance 当前外观
     * @param state      位掩码交互状态（focused / disabled / invalid）
     * @return 片段组合的解析结果
     */
    auto resolve_text_field(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const TextFieldVisualState state
    ) -> ResolvedTextFieldStyle {
        auto flat = resolve_text_field_style(theme_view(system, appearance), state);
        for (const auto& rule: system.components.text_field.rules) {
            if (rule.state && !has_text_field_state(state, *rule.state)) {
                continue;
            }
            if (rule.container_fill) {
                flat.background = resolve_color(system, appearance, *rule.container_fill);
            }
            if (rule.container_border) {
                flat.border_color = resolve_color(system, appearance, *rule.container_border);
            }
            if (rule.container_border_width) {
                flat.border_width = resolve_scalar(system, appearance, *rule.container_border_width);
            }
            if (rule.container_radius) {
                flat.radius = resolve_scalar(system, appearance, *rule.container_radius);
            }
            if (rule.value_color) {
                flat.foreground = resolve_color(system, appearance, *rule.value_color);
            }
            if (rule.placeholder_color) {
                flat.placeholder = resolve_color(system, appearance, *rule.placeholder_color);
            }
            if (rule.selection_color) {
                flat.selection = resolve_color(system, appearance, *rule.selection_color);
            }
            if (rule.focus_ring_color) {
                flat.focus_ring_color = resolve_color(system, appearance, *rule.focus_ring_color);
            }
            if (rule.focus_ring_width) {
                flat.focus_ring_width = resolve_scalar(system, appearance, *rule.focus_ring_width);
            }
            if (rule.font_size) {
                flat.font_size = resolve_scalar(system, appearance, *rule.font_size);
            }
            if (rule.metrics_height) {
                flat.height = resolve_scalar(system, appearance, *rule.metrics_height);
            }
            if (rule.metrics_padding_x) {
                flat.padding_x = resolve_scalar(system, appearance, *rule.metrics_padding_x);
            }
        }
        return {
            .container = ResolvedBoxStyle {
                .fill = flat.background,
                .border = flat.border_color,
                .border_width = flat.border_width,
                .radius = flat.radius,
            },
            .value = ResolvedTypeStyle {.color = flat.foreground, .font_size = flat.font_size},
            .placeholder = ResolvedTypeStyle {.color = flat.placeholder, .font_size = flat.font_size},
            .selection = flat.selection,
            .focus = ResolvedFocusRing {.color = flat.focus_ring_color, .width = flat.focus_ring_width},
            .metrics = ResolvedControlMetrics {
                .height = flat.height,
                .padding_x = flat.padding_x,
                .gap = 0.0F,
                .min_height = 0.0F,
                .box_size = 0.0F,
                .preferred_width = 0.0F,
            },
        };
    }

    /** @return 框架默认 Button 配方（token 引用，随 palette 变体解析）。 */
    auto default_button_recipe() -> ButtonRecipe {
        return {
            .container = BoxStyle {
                .fill = ThemeColor::token(ColorToken::primary),
                .border = ThemeColor::token(ColorToken::outline),
                .border_width = ThemeScalar::token(ScalarToken::border_thin),
                .radius = ThemeScalar::token(ScalarToken::radius_md),
            },
            .label = TypeStyle {
                .color = ThemeColor::token(ColorToken::on_primary),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_md),
            },
            .focus = FocusRingStyle {
                .color = ThemeColor::token(ColorToken::focus_ring),
                .width = ThemeScalar::token(ScalarToken::border_focus_ring),
            },
            .metrics = ControlMetrics {
                .height = ThemeScalar::literal(36.0F),
                .padding_x = ThemeScalar::literal(16.0F),
                .gap = ThemeScalar::token(ScalarToken::spacing_sm),
                .min_height = ThemeScalar::literal(32.0F),
                .box_size = ThemeScalar::literal(20.0F),
                .preferred_width = ThemeScalar::literal(240.0F),
            },
        };
    }

    /** @return 框架默认 Checkbox 配方。 */
    auto default_checkbox_recipe() -> CheckboxRecipe {
        return {
            .indicator = BoxStyle {
                .fill = ThemeColor::token(ColorToken::surface),
                .border = ThemeColor::token(ColorToken::outline),
                .border_width = ThemeScalar::token(ScalarToken::border_thin),
                .radius = ThemeScalar::literal(5.0F), // 与现状 radius.sm * 0.5 一致
            },
            .label = TypeStyle {
                .color = ThemeColor::token(ColorToken::on_surface),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_md),
            },
            .focus = FocusRingStyle {
                .color = ThemeColor::token(ColorToken::focus_ring),
                .width = ThemeScalar::token(ScalarToken::border_focus_ring),
            },
            .metrics = ControlMetrics {
                .height = ThemeScalar::literal(0.0F),
                .padding_x = ThemeScalar::literal(0.0F),
                .gap = ThemeScalar::token(ScalarToken::spacing_sm),
                .min_height = ThemeScalar::literal(32.0F),
                .box_size = ThemeScalar::literal(20.0F),
                .preferred_width = ThemeScalar::literal(0.0F),
            },
        };
    }

    /** @return 框架默认 Slider 配方。 */
    auto default_slider_recipe() -> SliderRecipe {
        return {
            .inactive_track = TrackStyle {
                .box = BoxStyle {
                    .fill = ThemeColor::token(ColorToken::outline_variant),
                    .border = ThemeColor::token(ColorToken::outline_variant),
                    .border_width = ThemeScalar::literal(0.0F),
                    .radius = ThemeScalar::token(ScalarToken::radius_full),
                },
                .thickness = ThemeScalar::literal(4.0F),
            },
            .active_track = TrackStyle {
                .box = BoxStyle {
                    .fill = ThemeColor::token(ColorToken::primary),
                    .border = ThemeColor::token(ColorToken::primary),
                    .border_width = ThemeScalar::literal(0.0F),
                    .radius = ThemeScalar::token(ScalarToken::radius_full),
                },
                .thickness = ThemeScalar::literal(4.0F),
            },
            .thumb = ThumbStyle {
                .box = BoxStyle {
                    .fill = ThemeColor::token(ColorToken::primary),
                    .border = ThemeColor::token(ColorToken::primary),
                    .border_width = ThemeScalar::literal(0.0F),
                    .radius = ThemeScalar::token(ScalarToken::radius_full),
                },
            },
            .focus = FocusRingStyle {
                .color = ThemeColor::token(ColorToken::focus_ring),
                .width = ThemeScalar::token(ScalarToken::border_focus_ring),
            },
            .metrics = ControlMetrics {
                .height = ThemeScalar::literal(0.0F),
                .padding_x = ThemeScalar::literal(0.0F),
                .gap = ThemeScalar::literal(0.0F),
                .min_height = ThemeScalar::literal(32.0F),
                .box_size = ThemeScalar::literal(0.0F),
                .preferred_width = ThemeScalar::literal(240.0F),
            },
        };
    }

    /** @return 框架默认 TextField 配方。 */
    auto default_text_field_recipe() -> TextFieldRecipe {
        return {
            .container = BoxStyle {
                .fill = ThemeColor::token(ColorToken::surface_variant),
                .border = ThemeColor::token(ColorToken::outline),
                .border_width = ThemeScalar::token(ScalarToken::border_thin),
                .radius = ThemeScalar::token(ScalarToken::radius_md),
            },
            .value = TypeStyle {
                .color = ThemeColor::token(ColorToken::on_surface),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_md),
            },
            .placeholder = TypeStyle {
                .color = ThemeColor::token(ColorToken::on_surface_variant),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_md),
            },
            .selection = ThemeColor::token(ColorToken::selection),
            .focus = FocusRingStyle {
                .color = ThemeColor::token(ColorToken::focus_ring),
                .width = ThemeScalar::token(ScalarToken::border_focus_ring),
            },
            .metrics = ControlMetrics {
                .height = ThemeScalar::literal(36.0F),
                .padding_x = ThemeScalar::literal(12.0F),
                .gap = ThemeScalar::literal(0.0F),
                .min_height = ThemeScalar::literal(32.0F),
                .box_size = ThemeScalar::literal(0.0F),
                .preferred_width = ThemeScalar::literal(0.0F),
            },
        };
    }

    /**
     * 框架默认设计系统。
     *
     * 片段携带无默认构造的 ThemeValue（没有 "unset" 态），因此整棵树只能通过
     * 聚合初始化构建——本函数就是那个种子。品牌主题从拷贝出发修改字段后原子提交。
     *
     * @return 与遗留 default_theme() 语义对齐的默认快照
     */
    auto default_design_system() -> DesignSystem {
        return DesignSystem {
            .tokens = NanTokens {},
            // NOTE: 真实的 light/dark palette 属于迁移期工作；当前两者都取自遗留
            // 默认方案，保证结构今天即可使用。
            .light = default_theme().palette,
            .dark = default_theme().palette,
            .typography = TypographyRoles {
                .label_sm = TypeStyle {
                    .color = ThemeColor::token(ColorToken::on_surface),
                    .font_size = ThemeScalar::token(ScalarToken::typography_label_sm),
                },
                .label_md = TypeStyle {
                    .color = ThemeColor::token(ColorToken::on_surface),
                    .font_size = ThemeScalar::token(ScalarToken::typography_label_md),
                },
                .label_lg = TypeStyle {
                    .color = ThemeColor::token(ColorToken::on_surface),
                    .font_size = ThemeScalar::token(ScalarToken::typography_label_lg),
                },
            },
            .components = ComponentRecipes {
                .button = ButtonRecipes {.base = default_button_recipe(), .rules = {}},
                .checkbox = CheckboxRecipes {.base = default_checkbox_recipe(), .rules = {}},
                .slider = SliderRecipes {.base = default_slider_recipe(), .rules = {}},
                .text_field = TextFieldRecipes {.base = default_text_field_recipe(), .rules = {}},
            },
        };
    }

    /**
     * 从遗留 NanTheme 构建 DesignSystem：tokens + 单调色板（light/dark 同值），
     * 配方沿用 default_design_system() 的框架默认。
     */
    auto design_system_from_theme(const NanTheme& theme) -> DesignSystem {
        auto system = default_design_system();
        system.tokens = theme.tokens;
        system.light = theme.palette;
        system.dark = theme.palette;
        return system;
    }

} // namespace nandina::theme
