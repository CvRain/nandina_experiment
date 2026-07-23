//
// widget/button — first semantic control built from tone/treatment tokens.
//

#include "button.hpp"
#include "../render/draw_context.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace nandina::widget
{

    namespace
    {
        [[nodiscard]] auto near(float lhs, float rhs) -> bool {
            return std::abs(lhs - rhs) <= foundation::nan_epsilon;
        }

        [[nodiscard]] auto same_color(foundation::NanColor lhs, foundation::NanColor rhs) -> bool {
            const auto a = lhs.oklch();
            const auto b = rhs.oklch();
            return near(a.light, b.light) && near(a.chroma, b.chroma) && near(a.hue, b.hue)
                && near(a.alpha, b.alpha);
        }

        [[nodiscard]] auto
        same_text_style(const primitives::TextStyle& lhs, const primitives::TextStyle& rhs)
            -> bool {
            return same_color(lhs.color, rhs.color) && near(lhs.font_size, rhs.font_size)
                && lhs.overflow == rhs.overflow && lhs.max_lines == rhs.max_lines;
        }
    } // namespace

    Button::Button(std::string text, theme::NanTheme theme): text_(std::move(text)), theme_(theme) {
        apply_metrics();
    }

    auto Button::create(std::string text, theme::NanTheme theme) -> std::shared_ptr<Button> {
        return std::make_shared<Button>(std::move(text), theme);
    }

    void Button::set_text(std::string text) {
        text_.set_text(std::move(text));
        mark_layout_dirty();
        apply_metrics();
    }

    auto Button::text() const -> std::string_view {
        return text_.text();
    }

    auto Button::text_node() -> primitives::Text& {
        return text_;
    }

    auto Button::text_node() const -> const primitives::Text& {
        return text_;
    }

    void Button::set_text_pipeline(primitives::TextPipeline pipeline) {
        text_.set_text_pipeline(pipeline);
        mark_layout_dirty();
        apply_metrics();
    }

    auto Button::text_pipeline() const -> primitives::TextPipeline {
        return text_.text_pipeline();
    }

    void Button::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        text_.apply_default_text_pipeline(pipeline);
        mark_layout_dirty();
    }

    void Button::apply_font_context(text::FontPipelineCache& context) {
        text_.apply_font_context(context);
        mark_layout_dirty();
    }

    void Button::set_font(text::FontRequest request) {
        text_.set_font(std::move(request));
        mark_layout_dirty();
    }

    void Button::set_font_family(resource::ResourceKey family) {
        text_.set_font_family(std::move(family));
        mark_layout_dirty();
    }

    void Button::set_font_weight(const int weight) {
        text_.set_font_weight(weight);
        mark_layout_dirty();
    }

    void Button::set_font_slant(const text::FontSlant slant) {
        text_.set_font_slant(slant);
        mark_layout_dirty();
    }

    void Button::set_text_overflow(primitives::TextOverflow overflow) {
        text_overflow_ = overflow;
        mark_layout_dirty();
        apply_metrics();
    }

    auto Button::text_overflow() const -> primitives::TextOverflow {
        return text_overflow_;
    }

    void Button::set_theme(theme::NanTheme theme) {
        theme_ = theme;
        mark_layout_dirty();
        apply_metrics();
    }

    auto Button::theme_ref() const -> const theme::NanTheme& {
        return theme_;
    }

    void Button::set_tone(theme::ButtonTone tone) {
        tone_ = tone;
        mark_layout_dirty();
        apply_metrics();
    }

    auto Button::tone() const -> theme::ButtonTone {
        return tone_;
    }

    void Button::set_treatment(theme::ButtonTreatment treatment) {
        treatment_ = treatment;
        mark_layout_dirty();
        apply_metrics();
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

        apply_text_style(visual_state());
        const float content_width = std::max(0.0F, world.get_width() - style.padding_x * 2.0F);
        (void)text_.measure_layout(
            scene::LayoutConstraints {
                .min_width = 0.0F,
                .max_width = content_width,
                .min_height = 0.0F,
                .max_height = style.height,
            }
        );
        const float text_width = text_.measured_text_width();
        const auto text_pos = foundation::NanPoint(
            world.get_left() + (world.get_width() - text_width) * 0.5F,
            world.get_top() + (world.get_height() - text_.laid_out_font_size()) * 0.5F
        );
        text_.draw_at(ctx, text_pos);

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

    auto Button::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto style = theme::resolve_button_style(
            theme_,
            tone_,
            treatment_,
            size_,
            disabled() ? theme::ButtonVisualState::disabled : theme::ButtonVisualState::normal
        );
        apply_text_style(
            disabled() ? theme::ButtonVisualState::disabled : theme::ButtonVisualState::normal
        );
        const float max_text_width = std::isfinite(constraints.max_width)
            ? std::max(0.0F, constraints.max_width - style.padding_x * 2.0F)
            : constraints.max_width;
        (void)text_.measure_layout(
            scene::LayoutConstraints {
                .min_width = 0.0F,
                .max_width = max_text_width,
                .min_height = 0.0F,
                .max_height = style.height,
            }
        );
        return constraints.constrain(
            foundation::NanSize(text_.width() + style.padding_x * 2.0F, style.height)
        );
    }

    void Button::apply_metrics() {
        const auto style = theme::resolve_button_style(
            theme_,
            tone_,
            treatment_,
            size_,
            disabled() ? theme::ButtonVisualState::disabled : theme::ButtonVisualState::normal
        );
        apply_text_style(
            disabled() ? theme::ButtonVisualState::disabled : theme::ButtonVisualState::normal
        );
        (void)text_.measure_layout(scene::LayoutConstraints::loose());
        set_size(foundation::NanSize(text_.width() + style.padding_x * 2.0F, style.height));
    }

    void Button::apply_text_style(theme::ButtonVisualState state) {
        const auto style = theme::resolve_button_style(theme_, tone_, treatment_, size_, state);
        const primitives::TextStyle text_style {
            .color = style.foreground,
            .font_size = style.font_size,
            .font = text_.font(),
            .overflow = text_overflow_,
            .max_lines = 1,
        };
        if (same_text_style(text_.style(), text_style)) {
            return;
        }
        text_.set_style(text_style);
    }

} // namespace nandina::widget
