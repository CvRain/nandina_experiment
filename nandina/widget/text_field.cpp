//
// widget/text_field — semantic single-line text input shell.
//

#include "text_field.hpp"

#include "../render/draw_context.hpp"
#include "../scene/input_event.hpp"
#include "../scene/scene_tree.hpp"
#include "../theme/theme_manager.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace nandina::widget
{
    namespace
    {
        constexpr int key_enter = 257;
        constexpr int key_kp_enter = 335;
        constexpr float caret_width = 1.0F;
    } // namespace

    TextField::TextField(std::string value, std::string placeholder, theme::NanTheme theme):
        edit_(std::move(value)),
        placeholder_(std::move(placeholder)),
        theme_(theme) {
        edit_.set_on_change([this](std::string_view value) {
            if (on_change_) {
                on_change_(value);
            }
        });
        apply_theme();
        (void)measure_layout(scene::LayoutConstraints::loose());
    }

    auto TextField::create(std::string value, std::string placeholder, theme::NanTheme theme)
        -> std::shared_ptr<TextField> {
        return std::make_shared<TextField>(std::move(value), std::move(placeholder), theme);
    }

    void TextField::set_value(std::string value) {
        edit_.set_value(std::move(value));
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto TextField::value() const -> std::string_view {
        return edit_.value();
    }

    void TextField::set_placeholder(std::string placeholder) {
        placeholder_.set_text(std::move(placeholder));
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto TextField::placeholder() const -> std::string_view {
        return placeholder_.text();
    }

    void TextField::set_theme(theme::NanTheme theme) {
        theme_ = theme;
        theme_explicit_ = true;
        apply_theme();
        mark_layout_dirty();
    }

    auto TextField::theme_ref() const -> const theme::NanTheme& {
        return theme_;
    }

    void TextField::set_on_change(std::function<void(std::string_view)> callback) {
        on_change_ = std::move(callback);
    }

    void TextField::set_on_submit(std::function<void(std::string_view)> callback) {
        on_submit_ = std::move(callback);
    }

    void TextField::set_read_only(const bool value) {
        read_only_ = value;
        edit_.set_read_only(value);
        mark_semantics_dirty();
    }
    auto TextField::read_only() const -> bool {
        return read_only_;
    }

    void TextField::set_disabled(const bool value) {
        disabled_ = value;
        dragging_ = false;
        if (disabled_ && is_inside_tree() && get_tree()->focused_node() == this) {
            get_tree()->set_focus(nullptr);
        }
        apply_theme();
        mark_layout_dirty();
        mark_semantics_dirty();
    }
    auto TextField::disabled() const -> bool {
        return disabled_;
    }

    void TextField::set_invalid(const bool value) {
        invalid_ = value;
        apply_theme();
        mark_semantics_dirty();
    }
    auto TextField::invalid() const -> bool {
        return invalid_;
    }

    auto TextField::visual_state() const -> theme::TextFieldVisualState {
        auto state = theme::TextFieldVisualState::normal;
        if (focused_) state = state | theme::TextFieldVisualState::focused;
        if (disabled_) state = state | theme::TextFieldVisualState::disabled;
        if (invalid_) state = state | theme::TextFieldVisualState::invalid;
        return state;
    }

    auto TextField::resolved_style() const -> theme::TextFieldStyle {
        if (theme_manager_ != nullptr) {
            return theme_manager_->style().resolve_text_field(theme_, visual_state());
        }
        return theme::resolve_text_field_style(theme_, visual_state());
    }

    auto TextField::editable_text() -> primitives::EditableText& {
        return edit_;
    }

    auto TextField::editable_text() const -> const primitives::EditableText& {
        return edit_;
    }

    auto TextField::placeholder_text() -> primitives::Text& {
        return placeholder_;
    }

    auto TextField::placeholder_text() const -> const primitives::Text& {
        return placeholder_;
    }

    void TextField::set_text_pipeline(primitives::TextPipeline pipeline) {
        edit_.set_text_pipeline(pipeline);
        placeholder_.set_text_pipeline(pipeline);
        mark_layout_dirty();
        (void)measure_layout(last_layout_constraints());
    }

    auto TextField::text_pipeline() const -> primitives::TextPipeline {
        return edit_.text_pipeline();
    }

    void TextField::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        edit_.apply_default_text_pipeline(pipeline);
        placeholder_.apply_default_text_pipeline(pipeline);
        mark_layout_dirty();
    }

    void TextField::apply_font_context(text::FontPipelineCache& context) {
        edit_.apply_font_context(context);
        placeholder_.apply_font_context(context);
        mark_layout_dirty();
    }

    void TextField::on_style_context_changed(const theme::ResolvedStyleContext& /*context*/) {
        apply_theme();
        mark_layout_dirty();
    }

    void TextField::on_theme_changed(const theme::ThemeManager& manager) {
        theme_manager_ = &manager;
        if (!theme_explicit_) {
            theme_ = manager.theme();
        }
        apply_theme();
        mark_layout_dirty();
    }

    void TextField::on_theme_context_removed() {
        theme_manager_ = nullptr;
    }

    void TextField::set_font(text::FontRequest request) {
        font_explicit_ = true;
        edit_.text_node().set_font(request);
        placeholder_.set_font(std::move(request));
        mark_layout_dirty();
    }

    void TextField::set_font_family(resource::ResourceKey family) {
        font_explicit_ = true;
        edit_.text_node().set_font_family(family);
        placeholder_.set_font_family(std::move(family));
        mark_layout_dirty();
    }

    void TextField::set_font_weight(const int weight) {
        font_explicit_ = true;
        edit_.text_node().set_font_weight(weight);
        placeholder_.set_font_weight(weight);
        mark_layout_dirty();
    }

    void TextField::set_font_slant(const text::FontSlant slant) {
        font_explicit_ = true;
        edit_.text_node().set_font_slant(slant);
        placeholder_.set_font_slant(slant);
        mark_layout_dirty();
    }

    auto TextField::is_focusable() const -> bool {
        return !disabled_;
    }

    auto TextField::on_input(scene::InputEvent& event) -> bool {
        if (event.type() == scene::EventType::focus_enter) {
            focused_ = !disabled_;
            apply_theme();
            mark_semantics_dirty();
            return edit_.on_input(event);
        }
        else if (event.type() == scene::EventType::focus_leave) {
            focused_ = false;
            dragging_ = false;
            edit_.clear_composition();
            apply_theme();
            mark_semantics_dirty();
            return edit_.on_input(event);
        }
        if (disabled_) {
            return false;
        }
        if (event.type() == scene::EventType::mouse_button) {
            auto& mouse = static_cast<scene::MouseButtonEvent&>(event);
            if (mouse.button() == scene::MouseButtonEvent::Button::left && mouse.is_pressed()) {
                place_caret(mouse.screen_pos(), mouse.modifiers().shift);
                dragging_ = true;
                get_tree()->set_pointer_capture(this);
                event.accept();
                return true;
            }
            if (mouse.button() == scene::MouseButtonEvent::Button::left) {
                dragging_ = false;
                get_tree()->release_pointer_capture(this);
                event.accept();
                return true;
            }
        }
        if (event.type() == scene::EventType::mouse_move && dragging_) {
            place_caret(static_cast<scene::MouseMoveEvent&>(event).screen_pos(), true);
            event.accept();
            return true;
        }
        if (event.type() == scene::EventType::key) {
            const auto& key = static_cast<scene::KeyEvent&>(event);
            if (key.is_pressed() && (key.keycode() == key_enter || key.keycode() == key_kp_enter)) {
                if (on_submit_) {
                    on_submit_(value());
                }
                event.accept();
                return true;
            }
        }
        return edit_.on_input(event);
    }

    void TextField::on_draw(render::DrawContext& ctx) {
        const auto world = render::world_bounds_from_local(ctx.world_transform(), local_rect());
        const auto style = resolved_style();
        surface_.set_fill(style.background);
        surface_.set_border(style.border_color, style.border_width);
        surface_.set_radius(style.radius);
        if (style.focus_ring_width > 0.0F && style.focus_ring_color.alpha() > 0.0F) {
            ctx.device().draw_rect_outline(
                world.expanded(style.focus_ring_width),
                style.focus_ring_width,
                style.focus_ring_color.with_alpha(style.focus_ring_color.alpha() * ctx.opacity())
            );
        }
        surface_.set_size(size());
        surface_.on_draw(ctx);

        const float viewport_width = std::max(0.0F, world.get_width() - padding_x_ * 2.0F);
        update_scroll(viewport_width);
        const auto viewport = foundation::NanRect::from_xywh(
            world.get_left() + padding_x_,
            world.get_top(),
            viewport_width,
            world.get_height()
        );
        auto clip = ctx.clip().push(viewport);
        if (edit_.value().empty() && !placeholder_.text().empty()) {
            placeholder_.draw_at(
                ctx,
                line_origin(world, placeholder_.layout_result(), viewport.get_left())
            );
            return;
        }
        edit_.draw_at(
            ctx,
            line_origin(world, edit_.text_node().layout_result(), viewport.get_left() - scroll_x_)
        );
    }

    auto TextField::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto content = content_constraints(constraints);
        const auto edit_size = edit_.measure_layout(
            scene::LayoutConstraints {
                .min_width = 0.0F,
                .max_width = std::numeric_limits<float>::infinity(),
                .min_height = 0.0F,
                .max_height = height_,
            }
        );
        const auto placeholder_size = placeholder_.measure_layout(content);
        const float natural_width =
            std::max(edit_size.get_width(), placeholder_size.get_width()) + padding_x_ * 2.0F;
        const auto measured = constraints.constrain(foundation::NanSize(natural_width, height_));
        set_size(measured);
        return measured;
    }

    auto TextField::semantics_properties() const -> semantics::Properties {
        auto actions = disabled_ ? semantics::Action::none : semantics::Action::focus;
        if (!disabled_ && !read_only_) {
            actions |= semantics::Action::set_value;
        }
        return {
            .role = semantics::Role::text_field,
            .label = std::string(placeholder()),
            .value = std::string(value()),
            .state = {
                .focusable = !disabled_,
                .focused = focused_,
                .disabled = disabled_,
                .read_only = read_only_,
                .invalid = invalid_,
            },
            .actions = actions,
        };
    }

    auto TextField::on_semantics_action(const semantics::ActionRequest& request) -> bool {
        if (request.action != semantics::Action::set_value || disabled_ || read_only_) {
            return false;
        }
        set_value(request.value);
        return true;
    }

    void TextField::apply_theme() {
        const auto style = resolved_style();
        padding_x_ = style.padding_x;
        height_ = style.height;

        surface_.set_fill(style.background);
        surface_.set_radius(style.radius);
        surface_.set_border(style.border_color, style.border_width);

        const auto& context = resolved_style_context();
        const auto font = context.font_from_context && !font_explicit_ ? context.font
                                                                       : edit_.style().font;
        const auto font_size = context.font_size_from_context
            ? context.font_size
            : style.font_size;
        const auto text_color = context.text_color_from_context ? context.text_color
                                                                 : style.foreground;
        const primitives::TextStyle value_style {
            .color = text_color,
            .font_size = font_size,
            .font = font,
            .overflow = primitives::TextOverflow::clip,
            .max_lines = 1,
        };
        const primitives::TextStyle placeholder_style {
            .color = context.text_color_from_context ? context.text_color : style.placeholder,
            .font_size = font_size,
            .font = font,
            .overflow = primitives::TextOverflow::ellipsis,
            .max_lines = 1,
        };
        edit_.set_style(value_style);
        edit_.set_selection_color(style.selection);
        placeholder_.set_style(placeholder_style);
    }

    auto TextField::content_constraints(scene::LayoutConstraints constraints) const
        -> scene::LayoutConstraints {
        return {
            .min_width = 0.0F,
            .max_width = std::isfinite(constraints.max_width)
                ? std::max(0.0F, constraints.max_width - padding_x_ * 2.0F)
                : constraints.max_width,
            .min_height = 0.0F,
            .max_height = height_,
        };
    }

    void TextField::place_caret(const foundation::NanPoint screen_point, const bool extend) {
        const auto local = to_local(screen_point);
        const auto stop = edit_.text_node().layout_result().caret_for_point(
            foundation::NanPoint(local.get_x() - padding_x_ + scroll_x_, 0.0F)
        );
        if (extend) {
            auto selection = edit_.selection();
            selection.focus = stop.source_offset;
            selection.focus_affinity = stop.affinity;
            edit_.set_selection(selection);
        }
        else {
            edit_.set_caret(stop.source_offset, stop.affinity);
        }
    }

    void TextField::update_scroll(const float viewport_width) {
        const auto& layout = edit_.text_node().layout_result();
        if (layout.lines.empty()) {
            scroll_x_ = 0.0F;
            return;
        }
        const auto caret =
            layout.lines.front().caret_for_source(edit_.caret(), edit_.caret_affinity());
        if (caret.x < scroll_x_) {
            scroll_x_ = caret.x;
        }
        if (caret.x > scroll_x_ + viewport_width - caret_width) {
            scroll_x_ = caret.x - viewport_width + caret_width;
        }
        const float max_scroll =
            std::max(0.0F, layout.lines.front().size.get_width() - viewport_width);
        scroll_x_ = std::clamp(scroll_x_, 0.0F, max_scroll);
    }

    auto TextField::line_origin(
        const foundation::NanRect world,
        const primitives::TextLayoutResult& layout,
        const float x
    ) const -> foundation::NanPoint {
        const float line_height =
            layout.lines.empty() ? layout.font_size : layout.lines.front().size.get_height();
        return foundation::NanPoint(
            x,
            world.get_top() + std::max(0.0F, (world.get_height() - line_height) * 0.5F)
        );
    }

} // namespace nandina::widget
