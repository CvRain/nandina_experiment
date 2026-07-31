//
// widget/primitives/surface — visual surface primitive.
//

#include "surface.hpp"
#include "../../render/draw_context.hpp"
#include "../../theme/theme_manager.hpp"

namespace nandina::widget::primitives
{

    void Surface::set_fill(foundation::NanColor color) {
        theme_fill_.reset();
        fill_ = color;
        mark_dirty(scene::DirtyFlags::paint);
    }

    void Surface::set_fill(theme::ThemeColor color) {
        theme_fill_ = std::move(color);
        fill_ = theme::resolve_theme_color(theme::default_theme(), *theme_fill_);
        mark_dirty(scene::DirtyFlags::paint);
    }

    void Surface::clear_fill() {
        fill_.reset();
        theme_fill_.reset();
        mark_dirty(scene::DirtyFlags::paint);
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
        theme_border_.reset();
        border_color_ = color;
        border_width_ = width < 0.0F ? 0.0F : width;
        mark_dirty(scene::DirtyFlags::paint);
    }

    void Surface::set_border(theme::ThemeColor color, const float width) {
        theme_border_ = std::move(color);
        border_color_ = theme::resolve_theme_color(theme::default_theme(), *theme_border_);
        border_width_ = width < 0.0F ? 0.0F : width;
        mark_dirty(scene::DirtyFlags::paint);
    }

    void Surface::clear_border() {
        border_color_.reset();
        theme_border_.reset();
        border_width_ = 0.0F;
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto Surface::border_color() const -> const std::optional<foundation::NanColor>& {
        return border_color_;
    }

    auto Surface::border_width() const -> float {
        return border_width_;
    }

    void Surface::on_theme_changed(const theme::ThemeManager& manager) {
        if (theme_fill_) {
            fill_ = theme::resolve_theme_color(manager.theme(), *theme_fill_);
        }
        if (theme_border_) {
            border_color_ = theme::resolve_theme_color(manager.theme(), *theme_border_);
        }
        mark_dirty(scene::DirtyFlags::paint);
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
