//
// widget/text_field — semantic single-line text input shell.
//

#include "text_field.hpp"

#include "../render/draw_context.hpp"
#include "../scene/input_event.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace nandina::widget
{

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
    }

    auto TextField::value() const -> std::string_view {
        return edit_.value();
    }

    void TextField::set_placeholder(std::string placeholder) {
        placeholder_.set_text(std::move(placeholder));
        mark_layout_dirty();
    }

    auto TextField::placeholder() const -> std::string_view {
        return placeholder_.text();
    }

    void TextField::set_theme(theme::NanTheme theme) {
        theme_ = theme;
        apply_theme();
        mark_layout_dirty();
    }

    auto TextField::theme_ref() const -> const theme::NanTheme& {
        return theme_;
    }

    void TextField::set_on_change(std::function<void(std::string_view)> callback) {
        on_change_ = std::move(callback);
    }

    auto TextField::editable_text() -> primitives::EditableText& {
        return edit_;
    }

    auto TextField::editable_text() const -> const primitives::EditableText& {
        return edit_;
    }

    auto TextField::is_focusable() const -> bool {
        return true;
    }

    auto TextField::on_input(scene::InputEvent& event) -> bool {
        if (event.type() == scene::EventType::mouse_button) {
            auto& mouse = static_cast<scene::MouseButtonEvent&>(event);
            if (mouse.button() == scene::MouseButtonEvent::Button::left && mouse.is_pressed()) {
                edit_.set_caret(edit_.value().size());
                event.accept();
                return true;
            }
        }
        return edit_.on_input(event);
    }

    void TextField::on_draw(render::DrawContext& ctx) {
        const auto world = render::world_bounds_from_local(ctx.world_transform(), local_rect());
        surface_.set_size(size());
        surface_.on_draw(ctx);

        const auto origin = content_origin(world);
        if (edit_.value().empty() && !placeholder_.text().empty()) {
            placeholder_.draw_at(ctx, origin);
            return;
        }
        edit_.draw_at(ctx, origin);
    }

    auto TextField::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto content = content_constraints(constraints);
        const auto edit_size = edit_.measure_layout(content);
        const auto placeholder_size = placeholder_.measure_layout(content);
        const float natural_width = std::max(edit_size.get_width(), placeholder_size.get_width())
            + padding_x_ * 2.0F;
        const auto measured = constraints.constrain(foundation::NanSize(natural_width, height_));
        set_size(measured);
        return measured;
    }

    void TextField::apply_theme() {
        padding_x_ = theme_.tokens.spacing.md;
        height_ = 40.0F;

        surface_.set_fill(theme_.palette.surface_variant);
        surface_.set_radius(theme_.tokens.radius.sm);
        surface_.set_border(theme_.palette.outline_variant, theme_.tokens.border.thin);

        const primitives::TextStyle value_style {
            .color = theme_.palette.on_surface,
            .font_size = theme_.tokens.typography.label_md,
            .overflow = primitives::TextOverflow::ellipsis,
            .max_lines = 1,
        };
        const primitives::TextStyle placeholder_style {
            .color = theme_.palette.on_surface_variant.with_alpha(0.72F),
            .font_size = theme_.tokens.typography.label_md,
            .overflow = primitives::TextOverflow::ellipsis,
            .max_lines = 1,
        };
        edit_.set_style(value_style);
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

    auto TextField::content_origin(foundation::NanRect world) const -> foundation::NanPoint {
        return foundation::NanPoint(
            world.get_left() + padding_x_,
            world.get_top() + std::max(0.0F, (world.get_height() - edit_.text_node().laid_out_font_size()) * 0.5F)
        );
    }

} // namespace nandina::widget
