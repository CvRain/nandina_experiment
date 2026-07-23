//
// theme/text_field_style — TextField state resolver.
//

#include "text_field_style.hpp"

namespace nandina::theme
{
    auto resolve_text_field_style(const NanTheme& theme, const TextFieldVisualState state)
        -> TextFieldStyle {
        const bool disabled = has_text_field_state(state, TextFieldVisualState::disabled);
        const bool invalid = has_text_field_state(state, TextFieldVisualState::invalid);
        const bool focused = has_text_field_state(state, TextFieldVisualState::focused);
        const float alpha = disabled ? theme.tokens.opacity.disabled : 1.0F;
        const auto border = invalid ? theme.palette.error : theme.palette.outline_variant;
        const auto focus = invalid ? theme.palette.error : theme.palette.primary;
        return {
            .background = theme.palette.surface_variant.with_alpha(alpha),
            .foreground = theme.palette.on_surface.with_alpha(alpha),
            .placeholder = theme.palette.on_surface_variant.with_alpha(0.72F * alpha),
            .border_color = border.with_alpha(alpha),
            .focus_ring_color = focus.with_alpha(focused || invalid ? 1.0F : 0.0F),
            .selection = theme.palette.primary.with_alpha(0.32F),
            .border_width = theme.tokens.border.thin,
            .radius = theme.tokens.radius.sm,
            .focus_ring_width = focused ? theme.tokens.border.focus_ring : 0.0F,
            .height = 40.0F,
            .padding_x = theme.tokens.spacing.md,
            .font_size = theme.tokens.typography.label_md,
        };
    }

} // namespace nandina::theme
