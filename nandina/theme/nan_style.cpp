//
// theme/nan_style — component rules backed by theme tokens or literals.
//

#include "nan_style.hpp"

namespace nandina::theme
{
    auto resolve_theme_color(const NanTheme& theme, const ThemeColor& value) -> NanColor {
        if (const auto* literal = std::get_if<NanColor>(&value.value())) {
            return *literal;
        }
        switch (std::get<ColorToken>(value.value())) {
            case ColorToken::primary:
                return theme.palette.primary;
            case ColorToken::on_primary:
                return theme.palette.on_primary;
            case ColorToken::secondary:
                return theme.palette.secondary;
            case ColorToken::on_secondary:
                return theme.palette.on_secondary;
            case ColorToken::surface:
                return theme.palette.surface;
            case ColorToken::on_surface:
                return theme.palette.on_surface;
            case ColorToken::surface_variant:
                return theme.palette.surface_variant;
            case ColorToken::on_surface_variant:
                return theme.palette.on_surface_variant;
            case ColorToken::outline:
                return theme.palette.outline;
            case ColorToken::outline_variant:
                return theme.palette.outline_variant;
            case ColorToken::error:
                return theme.palette.error;
            case ColorToken::on_error:
                return theme.palette.on_error;
        }
        return theme.palette.primary;
    }

    auto resolve_theme_scalar(const NanTheme& theme, const ThemeScalar& value) -> float {
        if (const auto* literal = std::get_if<float>(&value.value())) {
            return *literal;
        }
        switch (std::get<ScalarToken>(value.value())) {
            case ScalarToken::spacing_xs:
                return theme.tokens.spacing.xs;
            case ScalarToken::spacing_sm:
                return theme.tokens.spacing.sm;
            case ScalarToken::spacing_md:
                return theme.tokens.spacing.md;
            case ScalarToken::spacing_lg:
                return theme.tokens.spacing.lg;
            case ScalarToken::spacing_xl:
                return theme.tokens.spacing.xl;
            case ScalarToken::radius_sm:
                return theme.tokens.radius.sm;
            case ScalarToken::radius_md:
                return theme.tokens.radius.md;
            case ScalarToken::radius_lg:
                return theme.tokens.radius.lg;
            case ScalarToken::radius_full:
                return theme.tokens.radius.full;
            case ScalarToken::border_thin:
                return theme.tokens.border.thin;
            case ScalarToken::border_medium:
                return theme.tokens.border.medium;
            case ScalarToken::border_focus_ring:
                return theme.tokens.border.focus_ring;
            case ScalarToken::opacity_disabled:
                return theme.tokens.opacity.disabled;
            case ScalarToken::opacity_hover_overlay:
                return theme.tokens.opacity.hover_overlay;
            case ScalarToken::opacity_pressed_overlay:
                return theme.tokens.opacity.pressed_overlay;
            case ScalarToken::typography_label_sm:
                return theme.tokens.typography.label_sm;
            case ScalarToken::typography_label_md:
                return theme.tokens.typography.label_md;
            case ScalarToken::typography_label_lg:
                return theme.tokens.typography.label_lg;
        }
        return 0.0F;
    }

    auto ButtonRuleSelector::matches(
        const ButtonTone current_tone,
        const ButtonTreatment current_treatment,
        const ButtonSize current_size,
        const ButtonVisualState current_state
    ) const noexcept -> bool {
        return (!tone || *tone == current_tone) && (!treatment || *treatment == current_treatment)
            && (!size || *size == current_size) && (!state || *state == current_state);
    }

    void NanStyle::add_button_rule(ButtonStyleRule rule) {
        button_rules_.push_back(std::move(rule));
    }

    auto NanStyle::button_rules() const noexcept -> const std::vector<ButtonStyleRule>& {
        return button_rules_;
    }

    auto NanStyle::resolve_button(
        const NanTheme& theme,
        const ButtonTone tone,
        const ButtonTreatment treatment,
        const ButtonSize size,
        const ButtonVisualState state
    ) const -> ButtonStyle {
        auto resolved = resolve_button_style(theme, tone, treatment, size, state);
        for (const auto& rule: button_rules_) {
            if (!rule.selector.matches(tone, treatment, size, state)) {
                continue;
            }
            if (rule.background)
                resolved.background = resolve_theme_color(theme, *rule.background);
            if (rule.foreground)
                resolved.foreground = resolve_theme_color(theme, *rule.foreground);
            if (rule.border_color)
                resolved.border_color = resolve_theme_color(theme, *rule.border_color);
            if (rule.focus_ring_color) {
                resolved.focus_ring_color = resolve_theme_color(theme, *rule.focus_ring_color);
            }
            if (rule.border_width)
                resolved.border_width = resolve_theme_scalar(theme, *rule.border_width);
            if (rule.radius)
                resolved.radius = resolve_theme_scalar(theme, *rule.radius);
            if (rule.focus_ring_width) {
                resolved.focus_ring_width = resolve_theme_scalar(theme, *rule.focus_ring_width);
            }
            if (rule.height)
                resolved.height = resolve_theme_scalar(theme, *rule.height);
            if (rule.padding_x)
                resolved.padding_x = resolve_theme_scalar(theme, *rule.padding_x);
            if (rule.font_size)
                resolved.font_size = resolve_theme_scalar(theme, *rule.font_size);
        }
        return resolved;
    }

    void NanStyle::add_text_field_rule(TextFieldStyleRule rule) {
        text_field_rules_.push_back(std::move(rule));
    }

    auto NanStyle::text_field_rules() const noexcept -> const std::vector<TextFieldStyleRule>& {
        return text_field_rules_;
    }

    auto NanStyle::resolve_text_field(const NanTheme& theme, const TextFieldVisualState state) const
        -> TextFieldStyle {
        auto resolved = resolve_text_field_style(theme, state);
        for (const auto& rule: text_field_rules_) {
            if (rule.state && !has_text_field_state(state, *rule.state))
                continue;
            if (rule.background)
                resolved.background = resolve_theme_color(theme, *rule.background);
            if (rule.foreground)
                resolved.foreground = resolve_theme_color(theme, *rule.foreground);
            if (rule.placeholder)
                resolved.placeholder = resolve_theme_color(theme, *rule.placeholder);
            if (rule.border_color)
                resolved.border_color = resolve_theme_color(theme, *rule.border_color);
            if (rule.focus_ring_color)
                resolved.focus_ring_color = resolve_theme_color(theme, *rule.focus_ring_color);
            if (rule.selection)
                resolved.selection = resolve_theme_color(theme, *rule.selection);
            if (rule.border_width)
                resolved.border_width = resolve_theme_scalar(theme, *rule.border_width);
            if (rule.radius)
                resolved.radius = resolve_theme_scalar(theme, *rule.radius);
            if (rule.focus_ring_width)
                resolved.focus_ring_width = resolve_theme_scalar(theme, *rule.focus_ring_width);
            if (rule.height)
                resolved.height = resolve_theme_scalar(theme, *rule.height);
            if (rule.padding_x)
                resolved.padding_x = resolve_theme_scalar(theme, *rule.padding_x);
            if (rule.font_size)
                resolved.font_size = resolve_theme_scalar(theme, *rule.font_size);
        }
        return resolved;
    }

    auto default_style() -> std::shared_ptr<const NanStyle> {
        static const auto style = std::make_shared<const NanStyle>();
        return style;
    }

} // namespace nandina::theme
