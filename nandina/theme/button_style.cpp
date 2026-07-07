//
// theme/button_style — Button tone/treatment/size/state resolver.
//

#include "button_style.hpp"

namespace nandina::theme
{
    namespace
    {
        [[nodiscard]] auto transparent(const NanColor& base) -> NanColor {
            return base.with_alpha(0.0F);
        }

        [[nodiscard]] auto state_tint_amount(const NanTheme& theme, ButtonVisualState state) -> float {
            switch (state) {
            case ButtonVisualState::hovered:
            case ButtonVisualState::focused:
                return theme.tokens.opacity.hover_overlay;
            case ButtonVisualState::pressed:
                return theme.tokens.opacity.pressed_overlay;
            case ButtonVisualState::normal:
            case ButtonVisualState::disabled:
                return 0.0F;
            }
            return 0.0F;
        }

        void apply_disabled(const NanTheme& theme, ButtonStyle& style) {
            const float alpha = theme.tokens.opacity.disabled;
            style.background = style.background.with_alpha(style.background.alpha() * alpha);
            style.foreground = style.foreground.with_alpha(style.foreground.alpha() * alpha);
            style.border_color = style.border_color.with_alpha(style.border_color.alpha() * alpha);
            style.focus_ring_color = style.focus_ring_color.with_alpha(0.0F);
        }
    } // namespace

    auto button_color_pair(const NanTheme& theme, ButtonTone tone) -> ButtonColorPair {
        switch (tone) {
        case ButtonTone::primary:
            return {.accent = theme.palette.primary, .on_accent = theme.palette.on_primary};
        case ButtonTone::secondary:
            return {.accent = theme.palette.secondary, .on_accent = theme.palette.on_secondary};
        case ButtonTone::neutral:
            return {.accent = theme.palette.surface_variant, .on_accent = theme.palette.on_surface_variant};
        case ButtonTone::danger:
            return {.accent = theme.palette.error, .on_accent = theme.palette.on_error};
        }
        return {.accent = theme.palette.primary, .on_accent = theme.palette.on_primary};
    }

    auto resolve_button_style(
        const NanTheme& theme,
        ButtonTone tone,
        ButtonTreatment treatment,
        ButtonSize size,
        ButtonVisualState state
    ) -> ButtonStyle {
        const auto pair = button_color_pair(theme, tone);
        ButtonStyle style {
            .background = transparent(theme.palette.surface),
            .foreground = pair.accent,
            .border_color = transparent(pair.accent),
            .focus_ring_color = pair.accent,
            .border_width = 0.0F,
            .radius = theme.tokens.radius.sm,
            .focus_ring_width = theme.tokens.border.focus_ring,
            .height = 36.0F,
            .padding_x = theme.tokens.spacing.md,
            .font_size = theme.tokens.typography.label_md,
        };

        switch (size) {
        case ButtonSize::small:
            style.height = 32.0F;
            style.padding_x = theme.tokens.spacing.sm;
            style.font_size = theme.tokens.typography.label_sm;
            break;
        case ButtonSize::medium:
            style.height = 40.0F;
            style.padding_x = theme.tokens.spacing.md;
            style.font_size = theme.tokens.typography.label_md;
            break;
        case ButtonSize::large:
            style.height = 48.0F;
            style.padding_x = theme.tokens.spacing.lg;
            style.font_size = theme.tokens.typography.label_lg;
            style.radius = theme.tokens.radius.md;
            break;
        }

        const float tint = state_tint_amount(theme, state);
        switch (treatment) {
        case ButtonTreatment::filled:
            style.background = pair.accent;
            style.foreground = pair.on_accent;
            style.border_color = transparent(pair.accent);
            if (tint > 0.0F) {
                style.background = pair.accent.mix(pair.on_accent, tint);
            }
            break;
        case ButtonTreatment::tonal:
            style.background = theme.palette.surface_variant.mix(pair.accent, 0.35F + tint);
            style.foreground = pair.accent;
            style.border_color = transparent(pair.accent);
            break;
        case ButtonTreatment::outlined:
            style.background = tint > 0.0F ? theme.palette.surface.mix(pair.accent, tint) : transparent(theme.palette.surface);
            style.foreground = pair.accent;
            style.border_color = pair.accent;
            style.border_width = theme.tokens.border.thin;
            break;
        case ButtonTreatment::ghost:
            style.background = tint > 0.0F ? theme.palette.surface.mix(pair.accent, tint) : transparent(theme.palette.surface);
            style.foreground = pair.accent;
            style.border_color = transparent(pair.accent);
            break;
        case ButtonTreatment::link:
            style.background = transparent(theme.palette.surface);
            style.foreground = pair.accent;
            style.border_color = transparent(pair.accent);
            style.padding_x = 0.0F;
            break;
        }

        if (state == ButtonVisualState::disabled) {
            apply_disabled(theme, style);
        }

        return style;
    }

} // namespace nandina::theme
