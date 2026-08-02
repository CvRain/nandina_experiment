//
// theme/checkbox_style - Checkbox state resolver.
//

#ifndef NANDINA_EXPERIMENT_THEME_CHECKBOX_STYLE_HPP
#define NANDINA_EXPERIMENT_THEME_CHECKBOX_STYLE_HPP

#include "theme.hpp"

namespace nandina::theme
{
    enum class CheckboxVisualState {
        normal,
        hovered,
        pressed,
        focused,
        disabled,
    };

    struct CheckboxStyle {
        NanColor box_background;
        NanColor check_color;
        NanColor border_color;
        NanColor foreground;
        NanColor focus_ring_color;
        float box_size = 0.0F;
        float gap = 0.0F;
        float min_height = 0.0F;
        float border_width = 0.0F;
        float radius = 0.0F;
        float focus_ring_width = 0.0F;
        float font_size = 0.0F;
    };

    [[nodiscard]] auto
    resolve_checkbox_style(const NanTheme& theme, bool checked, CheckboxVisualState state)
        -> CheckboxStyle;
} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_CHECKBOX_STYLE_HPP
