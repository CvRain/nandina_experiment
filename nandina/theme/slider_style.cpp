//
// theme/slider_style - Slider state resolver.
//

#include "slider_style.hpp"

namespace nandina::theme
{
    auto resolve_slider_style(const NanTheme& theme, const SliderVisualState state) -> SliderStyle {
        const bool disabled = state == SliderVisualState::disabled;
        const bool hovered = state == SliderVisualState::hovered;
        const bool dragging = state == SliderVisualState::dragging;
        const bool focused = state == SliderVisualState::focused;
        const float alpha = disabled ? theme.tokens.opacity.disabled : 1.0F;
        return {
            .inactive_track = theme.palette.outline_variant.with_alpha(alpha),
            .active_track = theme.palette.primary.with_alpha(alpha),
            .thumb = theme.palette.primary.with_alpha(alpha),
            .focus_ring = theme.palette.focus_ring.with_alpha(alpha),
            .preferred_width = 240.0F,
            .min_height = 32.0F,
            .track_height = 4.0F,
            .thumb_radius = dragging ? 11.0F : (hovered ? 10.0F : 9.0F),
            .focus_ring_width = focused ? theme.tokens.border.focus_ring : 0.0F,
        };
    }
} // namespace nandina::theme
