//
// Created by cvrain on 2026/7/3.
//

#include "control.hpp"
#include "../render/draw_context.hpp"

namespace nandina::scene
{

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
