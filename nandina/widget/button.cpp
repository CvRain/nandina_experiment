//
// widget/button — first semantic control built from tone/treatment tokens.
//

#include "button.hpp"
#include "../render/draw_context.hpp"

#include <utility>

namespace nandina::widget
{

    Button::Button(std::string text, theme::NanTheme theme):
        text_(std::move(text)),
        theme_(theme) {
        apply_metrics();
    }

    auto Button::create(std::string text, theme::NanTheme theme) -> std::shared_ptr<Button> {
        return std::make_shared<Button>(std::move(text), theme);
    }

    void Button::set_text(std::string text) {
        text_ = std::move(text);
    }

    auto Button::text() const -> std::string_view {
        return text_;
    }

    void Button::set_theme(theme::NanTheme theme) {
        theme_ = theme;
        apply_metrics();
    }

    auto Button::theme_ref() const -> const theme::NanTheme& {
        return theme_;
    }

    void Button::set_tone(theme::ButtonTone tone) {
        tone_ = tone;
    }

    auto Button::tone() const -> theme::ButtonTone {
        return tone_;
    }

    void Button::set_treatment(theme::ButtonTreatment treatment) {
        treatment_ = treatment;
    }

    auto Button::treatment() const -> theme::ButtonTreatment {
        return treatment_;
    }

    void Button::set_button_size(theme::ButtonSize size) {
        size_ = size;
        apply_metrics();
    }

    auto Button::button_size() const -> theme::ButtonSize {
        return size_;
    }

    auto Button::visual_state() const -> theme::ButtonVisualState {
        if (disabled()) {
            return theme::ButtonVisualState::disabled;
        }
        if (pressed()) {
            return theme::ButtonVisualState::pressed;
        }
        if (hovered()) {
            return theme::ButtonVisualState::hovered;
        }
        if (focused()) {
            return theme::ButtonVisualState::focused;
        }
        return theme::ButtonVisualState::normal;
    }

    auto Button::resolved_style() const -> theme::ButtonStyle {
        return theme::resolve_button_style(theme_, tone_, treatment_, size_, visual_state());
    }

    void Button::on_draw(render::DrawContext& ctx) {
        const auto style = resolved_style();
        const auto world = render::world_bounds_from_local(ctx.world_transform(), local_rect());

        if (style.background.alpha() > 0.0F) {
            ctx.device().draw_rounded_rect(
                world,
                style.radius,
                style.background.with_alpha(style.background.alpha() * ctx.opacity())
            );
        }
        if (style.border_width > 0.0F && style.border_color.alpha() > 0.0F) {
            ctx.device().draw_rect_outline(
                world,
                style.border_width,
                style.border_color.with_alpha(style.border_color.alpha() * ctx.opacity())
            );
        }

        const float text_width = static_cast<float>(text_.size()) * style.font_size * 0.56F;
        const auto text_pos = foundation::NanPoint(
            world.get_left() + (world.get_width() - text_width) * 0.5F,
            world.get_top() + (world.get_height() - style.font_size) * 0.5F
        );
        ctx.device().draw_text(
            text_,
            text_pos,
            style.font_size,
            style.foreground.with_alpha(style.foreground.alpha() * ctx.opacity())
        );

        if (focused() && !disabled() && style.focus_ring_width > 0.0F) {
            const auto ring = world.expanded(style.focus_ring_width + 1.0F);
            ctx.device().draw_rect_outline(
                ring,
                style.focus_ring_width,
                style.focus_ring_color.with_alpha(style.focus_ring_color.alpha() * ctx.opacity())
            );
        }
    }

    void Button::on_pressable_state_changed() {}

    void Button::apply_metrics() {
        const auto style = theme::resolve_button_style(
            theme_,
            tone_,
            treatment_,
            size_,
            disabled() ? theme::ButtonVisualState::disabled : theme::ButtonVisualState::normal
        );
        const float text_width = static_cast<float>(text_.size()) * style.font_size * 0.56F;
        set_size(foundation::NanSize(text_width + style.padding_x * 2.0F, style.height));
    }

} // namespace nandina::widget
