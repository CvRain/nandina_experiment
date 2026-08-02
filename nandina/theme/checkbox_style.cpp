//
// theme/checkbox_style - Checkbox state resolver.
//

#include "checkbox_style.hpp"

namespace nandina::theme
{
    auto resolve_checkbox_style(
        const NanTheme& theme,
        const bool checked,
        const CheckboxVisualState state
    ) -> CheckboxStyle {
        const bool disabled = state == CheckboxVisualState::disabled;
        const bool hovered = state == CheckboxVisualState::hovered;
        const bool pressed = state == CheckboxVisualState::pressed;
        const bool focused = state == CheckboxVisualState::focused;
        const float alpha = disabled ? theme.tokens.opacity.disabled : 1.0F;
        const float interaction_alpha =
            pressed ? theme.tokens.opacity.pressed_overlay : theme.tokens.opacity.hover_overlay;
        auto background = checked ? theme.palette.primary : theme.palette.surface;
        if (!checked && (hovered || pressed)) {
            background = theme.palette.primary.with_alpha(interaction_alpha);
        }
        else {
            background = background.with_alpha(checked ? alpha : 0.0F);
        }
        return {
            .box_background = background,
            .check_color = theme.palette.on_primary.with_alpha(alpha),
            .border_color =
                (checked ? theme.palette.primary : theme.palette.outline).with_alpha(alpha),
            .foreground = theme.palette.on_surface.with_alpha(alpha),
            .focus_ring_color = theme.palette.focus_ring,
            .box_size = 20.0F,
            .gap = theme.tokens.spacing.sm,
            .min_height = 32.0F,
            .border_width = theme.tokens.border.thin,
            .radius = theme.tokens.radius.sm * 0.5F,
            .focus_ring_width = focused ? theme.tokens.border.focus_ring : 0.0F,
            .font_size = theme.tokens.typography.label_md,
        };
    }
} // namespace nandina::theme
