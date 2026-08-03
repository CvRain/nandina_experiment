//
// widget/button — first semantic control built from tone/treatment tokens.
//

#include "button.hpp"
#include "primitives/box_painter.hpp"
#include "primitives/focus_ring_painter.hpp"
#include "../render/draw_context.hpp"
#include "../theme/theme_manager.hpp"

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

    Button::Button(std::string text, theme::NanTheme theme): text_(std::move(text)) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        apply_metrics();
    }

    auto Button::create(std::string text, theme::NanTheme theme) -> std::shared_ptr<Button> {
        return std::make_shared<Button>(std::move(text), theme);
    }

    void Button::set_text(std::string text) {
        text_.set_text(std::move(text));
        mark_layout_dirty();
        apply_metrics();
        mark_semantics_dirty();
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

    void Button::on_style_context_changed(const theme::ResolvedStyleContext& /*context*/) {
        mark_layout_dirty();
        apply_text_style(visual_state());
    }

    void Button::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        mark_layout_dirty();
        apply_metrics();
    }

    void Button::set_font(text::FontRequest request) {
        font_explicit_ = true;
        text_.set_font(std::move(request));
        mark_layout_dirty();
    }

    void Button::set_font_family(resource::ResourceKey family) {
        font_explicit_ = true;
        text_.set_font_family(std::move(family));
        mark_layout_dirty();
    }

    void Button::set_font_weight(const int weight) {
        font_explicit_ = true;
        text_.set_font_weight(weight);
        mark_layout_dirty();
    }

    void Button::set_font_slant(const text::FontSlant slant) {
        font_explicit_ = true;
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
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        mark_layout_dirty();
        apply_metrics();
    }

    auto Button::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Button::set_override(theme::ButtonRecipeRule rule) {
        override_ = std::move(rule);
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
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

    auto Button::resolved_style() const -> theme::ResolvedButtonStyle {
        auto style = theme::resolve_button(
            *system_, appearance_, tone_, treatment_, size_, visual_state()
        );
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void Button::on_draw(render::DrawContext& ctx) {
        const auto style = resolved_style();
        const auto world = render::world_bounds_from_local(ctx.world_transform(), local_rect());
        const float opacity = ctx.opacity();

        primitives::BoxPainter::paint(ctx, world, style.container, opacity);

        apply_text_style(visual_state());
        const float content_width =
            std::max(0.0F, world.get_width() - style.metrics.padding_x * 2.0F);
        (void)text_.measure_layout(
            scene::LayoutConstraints {
                .min_width = 0.0F,
                .max_width = content_width,
                .min_height = 0.0F,
                .max_height = style.metrics.height,
            }
        );
        const float text_width = text_.measured_text_width();
        const auto text_pos = foundation::NanPoint(
            world.get_left() + (world.get_width() - text_width) * 0.5F,
            world.get_top() + (world.get_height() - text_.laid_out_font_size()) * 0.5F
        );
        text_.draw_at(ctx, text_pos);

        if (focused() && !disabled() && style.focus.width > 0.0F) {
            primitives::FocusRingPainter::paint(ctx, world, style.focus, opacity);
        }
    }

    void Button::on_pressable_state_changed() {
        mark_semantics_dirty();
    }

    auto Button::semantics_properties() const -> semantics::Properties {
        return {
            .role = semantics::Role::button,
            .label = std::string(text()),
            .state = {
                .focusable = !disabled(),
                .focused = focused(),
                .disabled = disabled(),
            },
            .actions = disabled() ? semantics::Action::none
                                  : semantics::Action::activate | semantics::Action::focus,
        };
    }

    auto Button::on_semantics_action(const semantics::ActionRequest& request) -> bool {
        if (request.action != semantics::Action::activate || disabled()) {
            return false;
        }
        activate();
        return true;
    }

    auto Button::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto state = disabled() ? theme::ButtonVisualState::disabled
                                      : theme::ButtonVisualState::normal;
        const auto style =
            theme::resolve_button(*system_, appearance_, tone_, treatment_, size_, state);
        apply_text_style(
            disabled() ? theme::ButtonVisualState::disabled : theme::ButtonVisualState::normal
        );
        const float max_text_width = std::isfinite(constraints.max_width)
            ? std::max(0.0F, constraints.max_width - style.metrics.padding_x * 2.0F)
            : constraints.max_width;
        (void)text_.measure_layout(
            scene::LayoutConstraints {
                .min_width = 0.0F,
                .max_width = max_text_width,
                .min_height = 0.0F,
                .max_height = style.metrics.height,
            }
        );
        return constraints.constrain(foundation::NanSize(
            text_.width() + style.metrics.padding_x * 2.0F,
            style.metrics.height
        ));
    }

    void Button::apply_metrics() {
        const auto state = disabled() ? theme::ButtonVisualState::disabled
                                      : theme::ButtonVisualState::normal;
        const auto style =
            theme::resolve_button(*system_, appearance_, tone_, treatment_, size_, state);
        apply_text_style(
            disabled() ? theme::ButtonVisualState::disabled : theme::ButtonVisualState::normal
        );
        (void)text_.measure_layout(scene::LayoutConstraints::loose());
        set_size(foundation::NanSize(
            text_.width() + style.metrics.padding_x * 2.0F,
            style.metrics.height
        ));
    }

    void Button::apply_text_style(theme::ButtonVisualState state) {
        const auto style =
            theme::resolve_button(*system_, appearance_, tone_, treatment_, size_, state);
        const auto& context = resolved_style_context();
        const primitives::TextStyle text_style {
            .color = context.text_color_from_context ? context.text_color : style.label.color,
            .font_size =
                context.font_size_from_context ? context.font_size : style.label.font_size,
            .font = context.font_from_context && !font_explicit_ ? context.font : text_.font(),
            .overflow = text_overflow_,
            .max_lines = 1,
        };
        if (same_text_style(text_.style(), text_style)) {
            return;
        }
        text_.set_style(text_style);
    }

} // namespace nandina::widget
