//
// Created by cvrain on 2026/7/3.
//
// scene/control — Control-like 布局节点 (Node2D 之上的 size / 矩形语义)
//
// NanControl 在 NanNode2D 的 transform 之上引入「尺寸」概念: 一个以局部原点为
// 左上角、大小为 size 的矩形 [0,0,w,h]。它提供:
//   - 矩形命中测试 (contains_point 覆写为 [0,w]x[0,h] 判定);
//   - world_bounds (局部矩形四角变换到世界取 AABB, 供 hit-test 粗判);
//   - 可选背景色绘制 (on_draw 填充 local_rect)。
//
// 这一层是纯 scene 语义, **不依赖 reactive / raylib**, 保持内核独立。
// 数据驱动 (signal → 属性) 由 widget 层的 NanReactiveControl 在其之上完成。
//
// 坐标约定: 局部原点 = 控件左上角 (对齐 Qt/GTK/前端的 box 习惯), 与 CardNode
// 早期「中心原点」不同 —— Control 采用左上角, 便于后续 anchor / padding 布局。
//

#ifndef NANDINA_EXPERIMENT_CONTROL_HPP
#define NANDINA_EXPERIMENT_CONTROL_HPP

#include "../foundation/geometry.hpp"
#include "../foundation/nandina_color.hpp"
#include "frame_scheduler.hpp"
#include "node2d.hpp"

#include <limits>
#include <optional>

namespace nandina::scene
{

    enum class ControlOverflow {
        visible,
        clip,
    };

    struct LayoutConstraints {
        float min_width = 0.0F;
        float max_width = std::numeric_limits<float>::infinity();
        float min_height = 0.0F;
        float max_height = std::numeric_limits<float>::infinity();

        [[nodiscard]] static auto loose() -> LayoutConstraints;
        [[nodiscard]] static auto tight(foundation::NanSize size) -> LayoutConstraints;
        [[nodiscard]] auto constrain(foundation::NanSize size) const -> foundation::NanSize;
        [[nodiscard]] auto deflated(foundation::NanInsets insets) const -> LayoutConstraints;
    };

    struct LayoutFlexPolicy {
        std::optional<float> basis;
        float grow = 0.0F;
        float shrink = 0.0F;
        LayoutConstraints limits {};
    };

    /// 带尺寸的 2D 控件基类。局部矩形为 [0,0,size.w,size.h] (原点左上角)。
    class NanControl: public NanNode2D {
    public:
        NanControl() = default;
        explicit NanControl(const foundation::NanSize& size);

        // ---- size ----

        [[nodiscard]] auto size() const -> foundation::NanSize;
        auto set_size(foundation::NanSize size) -> void;

        [[nodiscard]] auto width() const -> float;
        [[nodiscard]] auto height() const -> float;

        /// 局部空间矩形 [0,0,w,h]。
        [[nodiscard]] auto local_rect() const -> foundation::NanRect;

        // ---- layout protocol ----

        [[nodiscard]] auto measured_size() const -> foundation::NanSize;
        [[nodiscard]] auto last_layout_constraints() const -> LayoutConstraints;
        [[nodiscard]] auto layout_dirty() const -> bool;
        [[nodiscard]] auto dirty_flags() const -> DirtyFlags;
        [[nodiscard]] auto is_dirty(DirtyFlags flags) const -> bool;
        auto mark_dirty(DirtyFlags flags) -> void;
        auto clear_dirty(DirtyFlags flags) -> void;
        auto mark_layout_dirty() -> void;
        auto clear_layout_dirty() -> void;
        [[nodiscard]] virtual auto layout_flex_factor() const -> int;
        [[nodiscard]] virtual auto layout_flex_policy() const -> LayoutFlexPolicy;

        [[nodiscard]] auto measure_layout(LayoutConstraints constraints) -> foundation::NanSize;
        auto layout_to(foundation::NanRect rect) -> void;

        // ---- background (可选) ----

        /// 设置背景填充色; 未设置时 on_draw 不绘制背景 (透明容器)。
        auto set_background(foundation::NanColor color) -> void;
        auto clear_background() -> void;

        [[nodiscard]] auto background() const -> const std::optional<foundation::NanColor>&;

        auto set_overflow(ControlOverflow overflow) -> void;
        [[nodiscard]] auto overflow() const -> ControlOverflow;

        // ---- overrides ----

        /// 矩形命中: 局部点落在 [0,w]x[0,h] 内。
        [[nodiscard]] auto contains_point(foundation::NanPoint local_point) const -> bool override;

        /// 世界空间 AABB: 局部矩形四角变换取包围盒 (旋转时为 AABB)。
        [[nodiscard]] auto global_bounds() const -> foundation::NanRect override;

        /// 默认绘制: 若设置了背景色, 填充世界空间矩形 (乘继承 opacity)。
        auto on_draw(render::DrawContext& ctx) -> void override;

        [[nodiscard]] auto as_control() -> NanControl* override {
            return this;
        }
        [[nodiscard]] auto as_control() const -> const NanControl* override {
            return this;
        }

    protected:
        [[nodiscard]] virtual auto on_measure(LayoutConstraints constraints) -> foundation::NanSize;
        virtual auto on_layout() -> void;

        [[nodiscard]] auto _push_child_clip(render::DrawContext& ctx)
            -> render::ClipStack::Guard override;

    private:
        foundation::NanSize size_ {};
        foundation::NanSize measured_size_ {};
        LayoutConstraints last_layout_constraints_ {};
        DirtyFlags dirty_flags_ = layout_dirty_flags | DirtyFlags::paint | DirtyFlags::semantics;
        std::optional<foundation::NanColor> background_;
        ControlOverflow overflow_ = ControlOverflow::visible;
    };

} // namespace nandina::scene

#endif // NANDINA_EXPERIMENT_CONTROL_HPP
