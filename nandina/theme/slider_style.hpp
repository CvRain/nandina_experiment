//
// theme/slider_style - Slider state resolver.
//

#ifndef NANDINA_EXPERIMENT_THEME_SLIDER_STYLE_HPP
#define NANDINA_EXPERIMENT_THEME_SLIDER_STYLE_HPP

#include "theme.hpp"

namespace nandina::theme
{
    enum class SliderVisualState {
        normal,
        hovered,
        dragging,
        focused,
        disabled,
    };

    struct SliderStyle {
        NanColor inactive_track;
        NanColor active_track;
        NanColor thumb;
        NanColor focus_ring;
        float preferred_width = 0.0F;
        float min_height = 0.0F;
        float track_height = 0.0F;
        float thumb_radius = 0.0F;
        float focus_ring_width = 0.0F;
    };

    [[nodiscard]] auto resolve_slider_style(const NanTheme& theme, SliderVisualState state)
        -> SliderStyle;
} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_SLIDER_STYLE_HPP
