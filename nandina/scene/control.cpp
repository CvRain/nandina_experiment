//
// Created by cvrain on 2026/7/3.
//

#include "control.hpp"
#include "../render/draw_context.hpp"

namespace nandina::scene
{

    NanControl::NanControl(const foundation::NanSize& size): size_(size) {}

    auto NanControl::size() const -> foundation::NanSize {
        return size_;
    }

    void NanControl::set_size(foundation::NanSize size) {
        size_ = size;
    }

    auto NanControl::width() const -> float {
        return size_.get_width();
    }

    auto NanControl::height() const -> float {
        return size_.get_height();
    }

    auto NanControl::local_rect() const -> foundation::NanRect {
        return foundation::NanRect::from_xywh(0.0F, 0.0F, size_.get_width(), size_.get_height());
    }

    void NanControl::set_background(foundation::NanColor color) {
        background_ = color;
    }

    void NanControl::clear_background() {
        background_.reset();
    }

    auto NanControl::background() const -> const std::optional<foundation::NanColor>& {
        return background_;
    }

    bool NanControl::contains_point(foundation::NanPoint local_point) const {
        return local_point.get_x() >= 0.0F && local_point.get_x() <= size_.get_width()
            && local_point.get_y() >= 0.0F && local_point.get_y() <= size_.get_height();
    }
    auto NanControl::global_bounds() const -> foundation::NanRect {
        return render::world_bounds_from_local(global_transform(), local_rect());
    }

    void NanControl::on_draw(render::DrawContext& ctx) {
        if (!background_.has_value()) {
            return;
        }
        const auto world = render::world_bounds_from_local(ctx.world_transform(), local_rect());
        const auto color = background_->with_alpha(background_->alpha() * ctx.opacity());
        ctx.device().draw_rect(world, color);
    }

} // namespace nandina::scene
