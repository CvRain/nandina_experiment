//
// widget/primitives/surface — visual surface primitive.
//

#include "surface.hpp"
#include "../../render/draw_context.hpp"

namespace nandina::widget::primitives
{

    void Surface::set_fill(foundation::NanColor color) {
        fill_ = color;
    }

    void Surface::clear_fill() {
        fill_.reset();
    }

    auto Surface::fill() const -> const std::optional<foundation::NanColor>& {
        return fill_;
    }

    void Surface::set_radius(float radius) {
        radius_ = radius < 0.0F ? 0.0F : radius;
    }

    auto Surface::radius() const -> float {
        return radius_;
    }

    void Surface::set_border(foundation::NanColor color, float width) {
        border_color_ = color;
        border_width_ = width < 0.0F ? 0.0F : width;
    }

    void Surface::clear_border() {
        border_color_.reset();
        border_width_ = 0.0F;
    }

    auto Surface::border_color() const -> const std::optional<foundation::NanColor>& {
        return border_color_;
    }

    auto Surface::border_width() const -> float {
        return border_width_;
    }

    void Surface::on_draw(render::DrawContext& ctx) {
        const auto world = render::world_bounds_from_local(ctx.world_transform(), local_rect());
        if (fill_.has_value() && fill_->alpha() > 0.0F) {
            const auto color = fill_->with_alpha(fill_->alpha() * ctx.opacity());
            if (radius_ > 0.0F && ctx.device().supports_rounded_rect()) {
                ctx.device().draw_rounded_rect(world, radius_, color);
            }
            else {
                ctx.device().draw_rect(world, color);
            }
        }
        if (border_color_.has_value() && border_width_ > 0.0F && border_color_->alpha() > 0.0F) {
            ctx.device().draw_rect_outline(
                world,
                border_width_,
                border_color_->with_alpha(border_color_->alpha() * ctx.opacity())
            );
        }
    }

} // namespace nandina::widget::primitives
