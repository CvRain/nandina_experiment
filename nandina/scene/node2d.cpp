//
// Created by cvrain on 2026/6/30.
//

#include "node2d.hpp"

namespace nandina::scene
{

NanNode2D::NanNode2D() = default;

// ---- local transform (all mutators invalidate global cache) ----

auto NanNode2D::transform() const -> const foundation::NanTransform2D& {
    return transform_;
}

void NanNode2D::set_transform(const foundation::NanTransform2D& t) {
    transform_ = t;
    _propagate_invalidate_global();
}

auto NanNode2D::position() const -> foundation::NanPoint {
    return transform_.position();
}

void NanNode2D::set_position(const foundation::NanPoint pos) {
    transform_.set_position(pos);
    _propagate_invalidate_global();
}

auto NanNode2D::rotation() const -> float {
    return transform_.rotation();
}

void NanNode2D::set_rotation(const float radians) {
    transform_.set_rotation(radians);
    _propagate_invalidate_global();
}

auto NanNode2D::scale() const -> foundation::NanPoint {
    return transform_.scale();
}

void NanNode2D::set_scale(const foundation::NanPoint s) {
    transform_.set_scale(s);
    _propagate_invalidate_global();
}

void NanNode2D::set_scale(const float sx, const float sy) {
    transform_.set_scale_xy(sx, sy);
    _propagate_invalidate_global();
}

void NanNode2D::translate(const foundation::NanPoint offset) {
    transform_.translate(offset);
    _propagate_invalidate_global();
}

void NanNode2D::rotate(const float radians) {
    transform_.rotate(radians);
    _propagate_invalidate_global();
}

void NanNode2D::apply_scale(const foundation::NanPoint factor) {
    transform_.scale_by_xy(factor.get_x(), factor.get_y());
    _propagate_invalidate_global();
}

// ---- cached global transform ----

auto NanNode2D::global_transform() const -> foundation::NanTransform2D {
    if (global_invalid_) {
        cached_global_ = transform_;
        for (auto* p = dynamic_cast<const NanNode2D*>(parent()); p != nullptr;
             p = dynamic_cast<const NanNode2D*>(p->parent())) {
            cached_global_ = p->transform_ * cached_global_;
        }
        global_invalid_ = false;
    }
    return cached_global_;
}

auto NanNode2D::global_position() const -> foundation::NanPoint {
    return global_transform().position();
}

void NanNode2D::set_global_position(const foundation::NanPoint pos) {
    if (auto* p = dynamic_cast<NanNode2D*>(parent())) {
        const auto parent_inv = p->global_transform().inverse();
        set_position(parent_inv.transform_point(pos));
    } else {
        set_position(pos);
    }
}

auto NanNode2D::global_rotation() const -> float {
    auto rot = rotation();
    for (auto* p = dynamic_cast<const NanNode2D*>(parent()); p != nullptr;
         p = dynamic_cast<const NanNode2D*>(p->parent())) {
        rot += p->rotation();
    }
    return rot;
}

auto NanNode2D::to_global(const foundation::NanPoint local_point) const -> foundation::NanPoint {
    return global_transform().transform_point(local_point);
}

auto NanNode2D::to_local(const foundation::NanPoint global_point) const -> foundation::NanPoint {
    return global_transform().inverse_transform_point(global_point);
}

auto NanNode2D::global_bounds() const -> foundation::NanRect {
    const auto pos = global_position();
    return foundation::NanRect::from_xywh(pos.get_x(), pos.get_y(), 0, 0);
}

// ---- visibility ----

auto NanNode2D::visible() const -> bool {
    return visible_;
}

void NanNode2D::set_visible(const bool v) {
    visible_ = v;
}

// ---- draw order ----

auto NanNode2D::z_index() const -> int {
    return z_index_;
}

void NanNode2D::set_z_index(const int z) {
    z_index_ = z;
}

// ---- hit testing ----

auto NanNode2D::contains_point(foundation::NanPoint /*local_point*/) const -> bool {
    return false;
}

// ---- cache invalidation ----

void NanNode2D::_invalidate_global() {
    global_invalid_ = true;
}

void NanNode2D::_propagate_invalidate_global() {
    global_invalid_ = true;
    for (size_t i = 0; i < child_count(); ++i) {
        if (auto* child = dynamic_cast<NanNode2D*>(get_child(i))) {
            child->_propagate_invalidate_global();
        }
    }
}

// ---- lifecycle ----

void NanNode2D::on_enter_tree() {
    NanNode::on_enter_tree();
    global_invalid_ = true;  // Force recompute now that we have a parent chain.
}

void NanNode2D::on_exit_tree() {
    NanNode::on_exit_tree();
}

void NanNode2D::on_draw() {
    NanNode::on_draw();
}

} // namespace nandina::scene
