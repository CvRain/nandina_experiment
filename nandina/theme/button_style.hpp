//
// theme/button_style — Button tone/treatment/size/state resolver.
//

#ifndef NANDINA_EXPERIMENT_THEME_BUTTON_STYLE_HPP
#define NANDINA_EXPERIMENT_THEME_BUTTON_STYLE_HPP

#include "theme.hpp"

namespace nandina::theme
{

    enum class ButtonTone {
        primary,
        secondary,
        neutral,
        danger,
    };

    enum class ButtonTreatment {
        filled,
        tonal,
        outlined,
        ghost,
        link,
    };

    enum class ButtonSize {
        small,
        medium,
        large,
    };

    enum class ButtonVisualState {
        normal,
        hovered,
        pressed,
        focused,
        disabled,
    };

    struct ButtonColorPair {
        NanColor accent;
        NanColor on_accent;
    };

    struct ButtonStyle {
        NanColor background;
        NanColor foreground;
        NanColor border_color;
        NanColor focus_ring_color;
        float border_width = 0.0F;
        float radius = 0.0F;
        float focus_ring_width = 0.0F;
        float height = 0.0F;
        float padding_x = 0.0F;
        float font_size = 0.0F;
    };

    [[nodiscard]] auto button_color_pair(const NanTheme& theme, ButtonTone tone) -> ButtonColorPair;

    [[nodiscard]] auto resolve_button_style(
        const NanTheme& theme,
        ButtonTone tone,
        ButtonTreatment treatment,
        ButtonSize size,
        ButtonVisualState state
    ) -> ButtonStyle;

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_BUTTON_STYLE_HPP
