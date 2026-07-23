//
// theme/text_field_style — TextField state resolver.
//

#ifndef NANDINA_EXPERIMENT_THEME_TEXT_FIELD_STYLE_HPP
#define NANDINA_EXPERIMENT_THEME_TEXT_FIELD_STYLE_HPP

#include "theme.hpp"

namespace nandina::theme
{
    enum class TextFieldVisualState : unsigned char {
        normal = 0,
        focused = 1 << 0,
        disabled = 1 << 1,
        invalid = 1 << 2,
    };

    [[nodiscard]] constexpr auto operator|(TextFieldVisualState lhs, TextFieldVisualState rhs)
        -> TextFieldVisualState {
        return static_cast<TextFieldVisualState>(
            static_cast<unsigned char>(lhs) | static_cast<unsigned char>(rhs)
        );
    }

    [[nodiscard]] constexpr auto
    has_text_field_state(TextFieldVisualState value, TextFieldVisualState state) noexcept -> bool {
        if (state == TextFieldVisualState::normal)
            return value == state;
        return (static_cast<unsigned char>(value) & static_cast<unsigned char>(state))
            == static_cast<unsigned char>(state);
    }

    struct TextFieldStyle {
        NanColor background;
        NanColor foreground;
        NanColor placeholder;
        NanColor border_color;
        NanColor focus_ring_color;
        NanColor selection;
        float border_width = 0.0F;
        float radius = 0.0F;
        float focus_ring_width = 0.0F;
        float height = 0.0F;
        float padding_x = 0.0F;
        float font_size = 0.0F;
    };

    [[nodiscard]] auto resolve_text_field_style(const NanTheme& theme, TextFieldVisualState state)
        -> TextFieldStyle;

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_TEXT_FIELD_STYLE_HPP
