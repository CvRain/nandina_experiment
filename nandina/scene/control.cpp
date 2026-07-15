//
// Created by cvrain on 2026/7/3.
//

#include "control.hpp"
#include "../render/draw_context.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace nandina::scene
{

    namespace
    {
        [[nodiscard]] auto finite_or(float value, float fallback) -> float {
            return std::isfinite(value) ? value : fallback;
        }
    } // namespace

    auto LayoutConstraints::loose() -> LayoutConstraints {
        return {};
    }

    auto LayoutConstraints::tight(foundation::NanSize size) -> LayoutConstraints {
        return {
            .min_width = size.get_width(),
            .max_width = size.get_width(),
            .min_height = size.get_height(),
            .max_height = size.get_height(),
        };
    }

    auto LayoutConstraints::constrain(foundation::NanSize size) const -> foundation::NanSize {
        const float max_w = finite_or(max_width, std::max(size.get_width(), min_width));
        const float max_h = finite_or(max_height, std::max(size.get_height(), min_height));
        return foundation::NanSize(
            std::clamp(size.get_width(), min_width, std::max(min_width, max_w)),
            std::clamp(size.get_height(), min_height, std::max(min_height, max_h))
        );
    }

    auto LayoutConstraints::deflated(foundation::NanInsets insets) const -> LayoutConstraints {
        const float horizontal = insets.horizontal_sum();
        const float vertical = insets.vertical_sum();
        return {
            .min_width = std::max(0.0F, min_width - horizontal),
            .max_width =
                std::isfinite(max_width) ? std::max(0.0F, max_width - horizontal) : max_width,
            .min_height = std::max(0.0F, min_height - vertical),
            .max_height =
                std::isfinite(max_height) ? std::max(0.0F, max_height - vertical) : max_height,
        };
    }

    NanControl::NanControl(const foundation::NanSize& size): size_(size) {}

    auto NanControl::size() const -> foundation::NanSize {
        return size_;
    }

    void NanControl::set_size(foundation::NanSize size) {
        if (size_ == size) {
            return;
        }
        size_ = size;
        measured_size_ = size;
        if (auto* control_parent = parent() != nullptr ? parent()->as_control() : nullptr) {
            control_parent->mark_layout_dirty();
        }
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

    auto NanControl::measured_size() const -> foundation::NanSize {
        return measured_size_;
    }

    auto NanControl::last_layout_constraints() const -> LayoutConstraints {
        return last_layout_constraints_;
    }

    auto NanControl::layout_dirty() const -> bool {
        return layout_dirty_;
    }

    auto NanControl::mark_layout_dirty() -> void {
        if (layout_dirty_) {
            return;
        }
        layout_dirty_ = true;
        if (auto* control_parent = parent() != nullptr ? parent()->as_control() : nullptr) {
            control_parent->mark_layout_dirty();
        }
    }

    auto NanControl::clear_layout_dirty() -> void {
        layout_dirty_ = false;
    }

    auto NanControl::layout_flex_factor() const -> int {
        return 0;
    }

    auto NanControl::layout_flex_policy() const -> LayoutFlexPolicy {
        const auto factor = layout_flex_factor();
        return factor > 0 ? LayoutFlexPolicy {.basis = 0.0F, .grow = static_cast<float>(factor)}
                          : LayoutFlexPolicy {};
    }

    auto NanControl::measure_layout(LayoutConstraints constraints) -> foundation::NanSize {
        last_layout_constraints_ = constraints;
        measured_size_ = constraints.constrain(on_measure(constraints));
        return measured_size_;
    }

    auto NanControl::layout_to(foundation::NanRect rect) -> void {
        set_position(rect.get_top_left());
        set_size(rect.get_size());
        on_layout();
        clear_layout_dirty();
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

    void NanControl::set_overflow(const ControlOverflow overflow) {
        overflow_ = overflow;
    }

    auto NanControl::overflow() const -> ControlOverflow {
        return overflow_;
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

    auto NanControl::on_measure(LayoutConstraints constraints) -> foundation::NanSize {
        return constraints.constrain(size_);
    }

    auto NanControl::on_layout() -> void {
        std::vector<NanControl*> visible_children;
        for (std::size_t i = 0; i < child_count(); ++i) {
            auto* child = get_child(i) != nullptr ? get_child(i)->as_control() : nullptr;
            if (!child || !child->visible()) {
                continue;
            }
            visible_children.push_back(child);
        }

        if (visible_children.size() == 1) {
            auto* child = visible_children.front();
            (void)child->measure_layout(LayoutConstraints::tight(size()));
            child->layout_to(local_rect());
            return;
        }

        for (auto* child: visible_children) {
            const auto measured = child->measure_layout(
                LayoutConstraints {
                    .min_width = 0.0F,
                    .max_width = width(),
                    .min_height = 0.0F,
                    .max_height = height(),
                }
            );
            child->layout_to(foundation::NanRect::from_origin_size(child->position(), measured));
        }
    }

    auto NanControl::_push_child_clip(render::DrawContext& ctx) -> render::ClipStack::Guard {
        if (overflow_ != ControlOverflow::clip) {
            return {nullptr, false};
        }
        return ctx.clip().push(
            render::world_bounds_from_local(ctx.world_transform(), local_rect())
        );
    }

} // namespace nandina::scene
