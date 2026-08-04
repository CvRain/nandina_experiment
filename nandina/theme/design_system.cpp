/**
 * theme/design_system — 组件级解析与框架默认配方。
 *
 * 解析策略：
 *   1. 从 `base` 配方出发，按 selector 顺序应用配方书规则（后匹配者胜）；
 *   2. 跨切面状态变换由解析器代码完成：Button 的 hover/focused/pressed 覆盖
 *      （tint）与 disabled 透明度，其余控件同理；
 *   3. 输出为片段组合的解析结果（ResolvedButtonStyle 等）。
 *
 * 遗留 NanStyle 规则经 ThemeManager::merge_style 转为配方规则，追加在配方书尾部，
 * 因此仍然生效（后匹配者胜）。tone / treatment / size / state 的语义全部来自
 * default_design_system() 的 base + rules。
 */

#include "design_system.hpp"

namespace nandina::theme
{
    namespace
    {
        /** ButtonRecipe → 解析后的片段组合（配方即事实来源）。 */
        [[nodiscard]] auto resolve_recipe(
            const DesignSystem& system,
            const ColorAppearance appearance,
            const ButtonRecipe& recipe
        ) -> ResolvedButtonStyle {
            return {
                .container = resolve(system, appearance, recipe.container),
                .label = resolve(system, appearance, recipe.label),
                .focus = resolve(system, appearance, recipe.focus),
                .metrics = resolve(system, appearance, recipe.metrics),
            };
        }

        /** CheckboxRecipe → 解析后的片段组合（配方即事实来源）。 */
        [[nodiscard]] auto resolve_recipe(
            const DesignSystem& system,
            const ColorAppearance appearance,
            const CheckboxRecipe& recipe
        ) -> ResolvedCheckboxStyle {
            return {
                .indicator = resolve(system, appearance, recipe.indicator),
                .check = resolve_color(system, appearance, recipe.check),
                .label = resolve(system, appearance, recipe.label),
                .focus = resolve(system, appearance, recipe.focus),
                .metrics = resolve(system, appearance, recipe.metrics),
            };
        }

        /** SliderRecipe → 解析后的片段组合。 */
        [[nodiscard]] auto resolve_recipe(
            const DesignSystem& system,
            const ColorAppearance appearance,
            const SliderRecipe& recipe
        ) -> ResolvedSliderStyle {
            return {
                .inactive_track = resolve(system, appearance, recipe.inactive_track),
                .active_track = resolve(system, appearance, recipe.active_track),
                .thumb = resolve(system, appearance, recipe.thumb),
                .focus = resolve(system, appearance, recipe.focus),
                .metrics = resolve(system, appearance, recipe.metrics),
            };
        }

        // ─── disabled 状态变换（跨切面，由解析器按 token 应用） ──────────────

        /** 按因子缩放已解析颜色的 alpha。 */
        void scale_alpha(NanColor& color, const float factor) {
            color = color.with_alpha(color.alpha() * factor);
        }

        void apply_checkbox_disabled(
            const DesignSystem& system,
            const ColorAppearance appearance,
            ResolvedCheckboxStyle& style
        ) {
            const float alpha = resolve_scalar(
                system,
                appearance,
                ThemeScalar::token(ScalarToken::opacity_disabled)
            );
            scale_alpha(style.indicator.fill, alpha);
            scale_alpha(style.indicator.border, alpha);
            scale_alpha(style.check, alpha);
            scale_alpha(style.label.color, alpha);
        }

        void apply_slider_disabled(
            const DesignSystem& system,
            const ColorAppearance appearance,
            ResolvedSliderStyle& style
        ) {
            const float alpha = resolve_scalar(
                system,
                appearance,
                ThemeScalar::token(ScalarToken::opacity_disabled)
            );
            scale_alpha(style.inactive_track.box.fill, alpha);
            scale_alpha(style.active_track.box.fill, alpha);
            scale_alpha(style.thumb.box.fill, alpha);
            scale_alpha(style.focus.color, alpha);
        }

        /** TextFieldRecipe → 解析后的片段组合。 */
        [[nodiscard]] auto resolve_recipe(
            const DesignSystem& system,
            const ColorAppearance appearance,
            const TextFieldRecipe& recipe
        ) -> ResolvedTextFieldStyle {
            return {
                .container = resolve(system, appearance, recipe.container),
                .value = resolve(system, appearance, recipe.value),
                .placeholder = resolve(system, appearance, recipe.placeholder),
                .selection = resolve_color(system, appearance, recipe.selection),
                .focus = resolve(system, appearance, recipe.focus),
                .metrics = resolve(system, appearance, recipe.metrics),
            };
        }

        void apply_text_field_disabled(
            const DesignSystem& system,
            const ColorAppearance appearance,
            ResolvedTextFieldStyle& style
        ) {
            const float alpha = resolve_scalar(
                system,
                appearance,
                ThemeScalar::token(ScalarToken::opacity_disabled)
            );
            scale_alpha(style.container.fill, alpha);
            scale_alpha(style.container.border, alpha);
            scale_alpha(style.value.color, alpha);
            scale_alpha(style.placeholder.color, alpha);
        }

        /**
         * Button 的 hover / focused / pressed 覆盖变换（跨切面，由解析器按 treatment 应用）。
         *
         * 配方书的 treatment 规则描述 normal 态语义；交互态在此基础上叠加深浅不一的
         * 覆盖色，各 treatment 的混合语义与遗留平铺解析器一致：
         *   filled    accent.mix(on_accent, tint)
         *   tonal     surface_variant.mix(accent, 0.35 + tint)
         *   outlined  surface.mix(accent, tint)
         *   ghost     surface.mix(accent, tint)
         *   link      不变
         */
        void apply_button_tint(
            const DesignSystem& system,
            const ColorAppearance appearance,
            ResolvedButtonStyle& style,
            const ButtonTone tone,
            const ButtonTreatment treatment,
            const ButtonVisualState state
        ) {
            float amount = 0.0F;
            if (state == ButtonVisualState::hovered || state == ButtonVisualState::focused) {
                amount = resolve_scalar(
                    system,
                    appearance,
                    ThemeScalar::token(ScalarToken::opacity_hover_overlay)
                );
            }
            else if (state == ButtonVisualState::pressed) {
                amount = resolve_scalar(
                    system,
                    appearance,
                    ThemeScalar::token(ScalarToken::opacity_pressed_overlay)
                );
            }
            if (amount <= 0.0F) {
                return;
            }

            const auto [accent, on_accent] = button_accent(system.palette(appearance), tone);
            const auto surface = resolve_color(system, appearance, ThemeColor::token(ColorToken::surface));
            const auto surface_variant =
                resolve_color(system, appearance, ThemeColor::token(ColorToken::surface_variant));
            switch (treatment) {
                case ButtonTreatment::filled:
                    style.container.fill = accent.mix(on_accent, amount);
                    break;
                case ButtonTreatment::tonal:
                    style.container.fill = surface_variant.mix(accent, 0.35F + amount);
                    break;
                case ButtonTreatment::outlined:
                case ButtonTreatment::ghost:
                    style.container.fill = surface.mix(accent, amount);
                    break;
                case ButtonTreatment::link:
                    break;
            }
        }

        /** Button disabled 变换：全部颜色 ×opacity.disabled，焦点环隐去。 */
        void apply_button_disabled(
            const DesignSystem& system,
            const ColorAppearance appearance,
            ResolvedButtonStyle& style
        ) {
            const float alpha = resolve_scalar(
                system,
                appearance,
                ThemeScalar::token(ScalarToken::opacity_disabled)
            );
            scale_alpha(style.container.fill, alpha);
            scale_alpha(style.container.border, alpha);
            scale_alpha(style.label.color, alpha);
            style.focus.color = style.focus.color.with_alpha(0.0F);
        }
    } // namespace

    /**
     * 解析 Button 配方。
     *
     * 流程：base → 配方书规则（treatment / size / tone / state，后匹配者胜）→
     * hover/focused/pressed 覆盖变换 → disabled 变换。
     *
     * @param system     设计系统快照
     * @param appearance 当前外观
     * @param tone       语义色家族（accent 引用与 tint 变换依赖）
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
        auto style = resolve_recipe(system, appearance, system.components.button.base);
        for (const auto& rule: system.components.button.rules) {
            if (rule.selector.matches(tone, treatment, size, state)) {
                apply_rule(system, appearance, style, rule, tone);
            }
        }
        apply_button_tint(system, appearance, style, tone, treatment, state);
        if (state == ButtonVisualState::disabled) {
            apply_button_disabled(system, appearance, style);
        }
        return style;
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
        auto style = resolve_recipe(system, appearance, system.components.checkbox.base);
        for (const auto& rule: system.components.checkbox.rules) {
            if ((rule.checked && *rule.checked != checked) || (rule.state && *rule.state != state)) {
                continue;
            }
            apply_rule(system, appearance, style, rule);
        }
        if (state == CheckboxVisualState::disabled) {
            apply_checkbox_disabled(system, appearance, style);
        }
        return style;
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
        auto style = resolve_recipe(system, appearance, system.components.slider.base);
        for (const auto& rule: system.components.slider.rules) {
            if (rule.state && *rule.state != state) {
                continue;
            }
            apply_rule(system, appearance, style, rule);
        }
        if (state == SliderVisualState::disabled) {
            apply_slider_disabled(system, appearance, style);
        }
        return style;
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
        auto style = resolve_recipe(system, appearance, system.components.text_field.base);
        for (const auto& rule: system.components.text_field.rules) {
            if (rule.state && !has_text_field_state(state, *rule.state)) {
                continue;
            }
            apply_rule(system, appearance, style, rule);
        }
        if (has_text_field_state(state, TextFieldVisualState::disabled)) {
            apply_text_field_disabled(system, appearance, style);
        }
        return style;
    }

    // ─── apply_rule：把配方规则应用到已解析的配方（widget set_override 复用） ───

    void apply_rule(
        const DesignSystem& system,
        const ColorAppearance appearance,
        ResolvedButtonStyle& style,
        const ButtonRecipeRule& rule,
        const ButtonTone tone
    ) {
        if (rule.container_fill)
            style.container.fill = resolve_color(system, appearance, *rule.container_fill, tone);
        if (rule.container_border)
            style.container.border = resolve_color(system, appearance, *rule.container_border, tone);
        if (rule.container_border_width) {
            style.container.border_width =
                resolve_scalar(system, appearance, *rule.container_border_width);
        }
        if (rule.container_radius)
            style.container.radius = resolve_scalar(system, appearance, *rule.container_radius);
        if (rule.label_color)
            style.label.color = resolve_color(system, appearance, *rule.label_color, tone);
        if (rule.label_font_size)
            style.label.font_size = resolve_scalar(system, appearance, *rule.label_font_size);
        if (rule.focus_ring_color)
            style.focus.color = resolve_color(system, appearance, *rule.focus_ring_color, tone);
        if (rule.focus_ring_width)
            style.focus.width = resolve_scalar(system, appearance, *rule.focus_ring_width);
        if (rule.metrics_height)
            style.metrics.height = resolve_scalar(system, appearance, *rule.metrics_height);
        if (rule.metrics_padding_x)
            style.metrics.padding_x = resolve_scalar(system, appearance, *rule.metrics_padding_x);
    }

    void apply_rule(
        const DesignSystem& system,
        const ColorAppearance appearance,
        ResolvedCheckboxStyle& style,
        const CheckboxRecipeRule& rule
    ) {
        if (rule.indicator_fill)
            style.indicator.fill = resolve_color(system, appearance, *rule.indicator_fill);
        if (rule.indicator_border)
            style.indicator.border = resolve_color(system, appearance, *rule.indicator_border);
        if (rule.indicator_border_width) {
            style.indicator.border_width =
                resolve_scalar(system, appearance, *rule.indicator_border_width);
        }
        if (rule.indicator_radius)
            style.indicator.radius = resolve_scalar(system, appearance, *rule.indicator_radius);
        if (rule.label_color)
            style.label.color = resolve_color(system, appearance, *rule.label_color);
        if (rule.label_font_size)
            style.label.font_size = resolve_scalar(system, appearance, *rule.label_font_size);
        if (rule.focus_ring_color)
            style.focus.color = resolve_color(system, appearance, *rule.focus_ring_color);
        if (rule.focus_ring_width)
            style.focus.width = resolve_scalar(system, appearance, *rule.focus_ring_width);
        if (rule.metrics_gap)
            style.metrics.gap = resolve_scalar(system, appearance, *rule.metrics_gap);
        if (rule.metrics_box_size)
            style.metrics.box_size = resolve_scalar(system, appearance, *rule.metrics_box_size);
    }

    void apply_rule(
        const DesignSystem& system,
        const ColorAppearance appearance,
        ResolvedSliderStyle& style,
        const SliderRecipeRule& rule
    ) {
        if (rule.track_inactive_fill) {
            style.inactive_track.box.fill =
                resolve_color(system, appearance, *rule.track_inactive_fill);
        }
        if (rule.track_active_fill) {
            style.active_track.box.fill = resolve_color(system, appearance, *rule.track_active_fill);
        }
        if (rule.track_thickness) {
            style.inactive_track.thickness = resolve_scalar(system, appearance, *rule.track_thickness);
            style.active_track.thickness = resolve_scalar(system, appearance, *rule.track_thickness);
        }
        if (rule.thumb_fill)
            style.thumb.box.fill = resolve_color(system, appearance, *rule.thumb_fill);
        if (rule.thumb_radius)
            style.thumb.box.radius = resolve_scalar(system, appearance, *rule.thumb_radius);
        if (rule.focus_ring_color)
            style.focus.color = resolve_color(system, appearance, *rule.focus_ring_color);
        if (rule.focus_ring_width)
            style.focus.width = resolve_scalar(system, appearance, *rule.focus_ring_width);
    }

    void apply_rule(
        const DesignSystem& system,
        const ColorAppearance appearance,
        ResolvedTextFieldStyle& style,
        const TextFieldRecipeRule& rule
    ) {
        if (rule.container_fill)
            style.container.fill = resolve_color(system, appearance, *rule.container_fill);
        if (rule.container_border)
            style.container.border = resolve_color(system, appearance, *rule.container_border);
        if (rule.container_border_width) {
            style.container.border_width =
                resolve_scalar(system, appearance, *rule.container_border_width);
        }
        if (rule.container_radius)
            style.container.radius = resolve_scalar(system, appearance, *rule.container_radius);
        if (rule.value_color)
            style.value.color = resolve_color(system, appearance, *rule.value_color);
        if (rule.placeholder_color)
            style.placeholder.color = resolve_color(system, appearance, *rule.placeholder_color);
        if (rule.selection_color)
            style.selection = resolve_color(system, appearance, *rule.selection_color);
        if (rule.focus_ring_color)
            style.focus.color = resolve_color(system, appearance, *rule.focus_ring_color);
        if (rule.focus_ring_width)
            style.focus.width = resolve_scalar(system, appearance, *rule.focus_ring_width);
        if (rule.font_size) {
            style.value.font_size = resolve_scalar(system, appearance, *rule.font_size);
            style.placeholder.font_size = resolve_scalar(system, appearance, *rule.font_size);
        }
        if (rule.metrics_height)
            style.metrics.height = resolve_scalar(system, appearance, *rule.metrics_height);
        if (rule.metrics_padding_x)
            style.metrics.padding_x = resolve_scalar(system, appearance, *rule.metrics_padding_x);
    }

    /** @return 框架默认 Button 配方（normal 态通用语义；treatment/size 由规则覆盖）。 */
    auto default_button_recipe() -> ButtonRecipe {
        return {
            .container = BoxStyle {
                .fill = ThemeColor::transparent(ColorToken::surface),
                .border = ThemeColor::transparent(accent_ref),
                .border_width = ThemeScalar::literal(0.0F),
                .radius = ThemeScalar::token(ScalarToken::radius_sm),
            },
            .label = TypeStyle {
                .color = ThemeColor::accent(),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_md),
            },
            .focus = FocusRingStyle {
                .color = ThemeColor::token(ColorToken::focus_ring),
                .width = ThemeScalar::token(ScalarToken::border_focus_ring),
            },
            .metrics = ControlMetrics {
                .height = ThemeScalar::literal(36.0F),
                .padding_x = ThemeScalar::token(ScalarToken::spacing_md),
                .gap = ThemeScalar::literal(0.0F),
                .min_height = ThemeScalar::literal(32.0F),
                .box_size = ThemeScalar::literal(0.0F),
                .preferred_width = ThemeScalar::literal(0.0F),
            },
        };
    }

    /** @return 框架默认 Checkbox 配方（未勾选：透明指示器 + outline 边框）。 */
    auto default_checkbox_recipe() -> CheckboxRecipe {
        return {
            .indicator = BoxStyle {
                .fill = ThemeColor::transparent(ColorToken::surface),
                .border = ThemeColor::token(ColorToken::outline),
                .border_width = ThemeScalar::token(ScalarToken::border_thin),
                .radius = ThemeScalar::literal(5.0F), // 与现状 radius.sm * 0.5 一致
            },
            .check = ThemeColor::token(ColorToken::on_primary),
            .label = TypeStyle {
                .color = ThemeColor::token(ColorToken::on_surface),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_md),
            },
            .focus = FocusRingStyle {
                .color = ThemeColor::token(ColorToken::focus_ring),
                .width = ThemeScalar::literal(0.0F), // focused 规则按需开启
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
                    .radius = ThemeScalar::literal(9.0F), // dragging 11 / hovered 10 由规则覆盖
                },
            },
            .focus = FocusRingStyle {
                .color = ThemeColor::token(ColorToken::focus_ring),
                .width = ThemeScalar::literal(0.0F), // focused 规则按需开启
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

    /** @return 框架默认 TextField 配方（normal 状态；focused/invalid 由规则覆盖）。 */
    auto default_text_field_recipe() -> TextFieldRecipe {
        return {
            .container = BoxStyle {
                .fill = ThemeColor::token(ColorToken::surface_variant),
                .border = ThemeColor::token(ColorToken::outline_variant),
                .border_width = ThemeScalar::token(ScalarToken::border_thin),
                .radius = ThemeScalar::token(ScalarToken::radius_sm),
            },
            .value = TypeStyle {
                .color = ThemeColor::token(ColorToken::on_surface),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_md),
            },
            .placeholder = TypeStyle {
                .color = ThemeColor::with_alpha(
                    ColorToken::on_surface_variant,
                    ThemeScalar::literal(0.72F)
                ),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_md),
            },
            .selection = ThemeColor::token(ColorToken::selection),
            .focus = FocusRingStyle {
                .color = ThemeColor::token(ColorToken::focus_ring),
                .width = ThemeScalar::literal(0.0F), // focused 规则按需开启
            },
            .metrics = ControlMetrics {
                .height = ThemeScalar::literal(40.0F),
                .padding_x = ThemeScalar::token(ScalarToken::spacing_md),
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
                .button = ButtonRecipes {
                    .base = default_button_recipe(),
                    .rules = {
                        // 尺寸档位（对齐遗留解析器：small 32 / medium 40 / large 48）
                        ButtonRecipeRule {
                            .selector = {.size = ButtonSize::small},
                            .label_font_size = ThemeScalar::token(ScalarToken::typography_label_sm),
                            .metrics_height = ThemeScalar::literal(32.0F),
                            .metrics_padding_x = ThemeScalar::token(ScalarToken::spacing_sm),
                        },
                        ButtonRecipeRule {
                            .selector = {.size = ButtonSize::medium},
                            .label_font_size = ThemeScalar::token(ScalarToken::typography_label_md),
                            .metrics_height = ThemeScalar::literal(40.0F),
                            .metrics_padding_x = ThemeScalar::token(ScalarToken::spacing_md),
                        },
                        ButtonRecipeRule {
                            .selector = {.size = ButtonSize::large},
                            .container_radius = ThemeScalar::token(ScalarToken::radius_md),
                            .label_font_size = ThemeScalar::token(ScalarToken::typography_label_lg),
                            .metrics_height = ThemeScalar::literal(48.0F),
                            .metrics_padding_x = ThemeScalar::token(ScalarToken::spacing_lg),
                        },
                        // 视觉处理方式（normal 态语义；hover/pressed 覆盖由解析器变换处理）。
                        // accent / on_accent 引用当前 tone，随解析时的 tone 解析。
                        ButtonRecipeRule {
                            .selector = {.treatment = ButtonTreatment::filled},
                            .container_fill = ThemeColor::accent(),
                            .container_border = ThemeColor::transparent(accent_ref),
                            .label_color = ThemeColor::on_accent(),
                        },
                        ButtonRecipeRule {
                            .selector = {.treatment = ButtonTreatment::tonal},
                            .container_fill = ThemeColor::mix(
                                ColorOperand {ColorToken::surface_variant},
                                accent_ref,
                                ThemeScalar::literal(0.35F)
                            ),
                            .container_border = ThemeColor::transparent(accent_ref),
                            .label_color = ThemeColor::accent(),
                        },
                        ButtonRecipeRule {
                            .selector = {.treatment = ButtonTreatment::outlined},
                            .container_fill = ThemeColor::transparent(ColorToken::surface),
                            .container_border = ThemeColor::accent(),
                            .container_border_width = ThemeScalar::token(ScalarToken::border_thin),
                            .label_color = ThemeColor::accent(),
                        },
                        ButtonRecipeRule {
                            .selector = {.treatment = ButtonTreatment::ghost},
                            .container_fill = ThemeColor::transparent(ColorToken::surface),
                            .container_border = ThemeColor::transparent(accent_ref),
                            .label_color = ThemeColor::accent(),
                        },
                        ButtonRecipeRule {
                            .selector = {.treatment = ButtonTreatment::link},
                            .container_fill = ThemeColor::transparent(ColorToken::surface),
                            .container_border = ThemeColor::transparent(accent_ref),
                            .label_color = ThemeColor::accent(),
                            .metrics_padding_x = ThemeScalar::literal(0.0F),
                        },
                    },
                },
                .checkbox = CheckboxRecipes {
                    .base = default_checkbox_recipe(),
                    .rules = {
                        CheckboxRecipeRule {
                            .checked = true,
                            .indicator_fill = ThemeColor::token(ColorToken::primary),
                            .indicator_border = ThemeColor::token(ColorToken::primary),
                        },
                        CheckboxRecipeRule {
                            .checked = false,
                            .state = CheckboxVisualState::hovered,
                            .indicator_fill = ThemeColor::with_alpha(
                                ColorToken::primary,
                                ThemeScalar::token(ScalarToken::opacity_hover_overlay)
                            ),
                        },
                        CheckboxRecipeRule {
                            .checked = false,
                            .state = CheckboxVisualState::pressed,
                            .indicator_fill = ThemeColor::with_alpha(
                                ColorToken::primary,
                                ThemeScalar::token(ScalarToken::opacity_pressed_overlay)
                            ),
                        },
                        CheckboxRecipeRule {
                            .state = CheckboxVisualState::focused,
                            .focus_ring_width =
                                ThemeScalar::token(ScalarToken::border_focus_ring),
                        },
                    },
                },
                .slider = SliderRecipes {
                    .base = default_slider_recipe(),
                    .rules = {
                        SliderRecipeRule {
                            .state = SliderVisualState::dragging,
                            .thumb_radius = ThemeScalar::literal(11.0F),
                        },
                        SliderRecipeRule {
                            .state = SliderVisualState::hovered,
                            .thumb_radius = ThemeScalar::literal(10.0F),
                        },
                        SliderRecipeRule {
                            .state = SliderVisualState::focused,
                            .focus_ring_width =
                                ThemeScalar::token(ScalarToken::border_focus_ring),
                        },
                    },
                },
                .text_field = TextFieldRecipes {
                    .base = default_text_field_recipe(),
                    .rules = {
                        TextFieldRecipeRule {
                            .state = TextFieldVisualState::focused,
                            .focus_ring_width =
                                ThemeScalar::token(ScalarToken::border_focus_ring),
                        },
                        TextFieldRecipeRule {
                            .state = TextFieldVisualState::invalid,
                            .container_border = ThemeColor::token(ColorToken::error),
                            .focus_ring_color = ThemeColor::token(ColorToken::error),
                        },
                    },
                },
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
