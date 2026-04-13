module;
#include <memory>

export module Nandina.Layout.Flex;

import Nandina.Core.Component;
import Nandina.Core.Widget;
import Nandina.Types.Size;

export namespace Nandina {
    namespace detail {
        inline auto relayout_if_ready(Widget& widget) -> void {
            widget.mark_dirty();
            if (widget.width() > 0.0f || widget.height() > 0.0f) {
                widget.set_bounds(widget.x(), widget.y(), widget.width(), widget.height());
            }
        }
    } // namespace detail

    // ── Spacer ────────────────────────────────────────────────────────────────────
    // Elastic blank space in a Row/Column. The flex value determines how many
    // parts of the remaining space this Spacer claims.
    class Spacer final : public Component {
    public:
        static auto Create(int flex = 1) -> std::unique_ptr<Spacer> {
            auto s = std::make_unique<Spacer>();
            s->flex_ = flex;
            s->set_background(0, 0, 0, 0);
            return s;
        }

        [[nodiscard]] auto flex_factor() const noexcept -> int override { return flex_; }

    private:
        int flex_ = 1;
    };

    /**
     * Wraps a child widget and claims flex shares of the remaining main-axis space.
     *
     * 包裹子控件，并占据剩余主轴空间中的一部分以实现弹性布局。
     */
    class Expanded final : public Component {
    public:
        static auto Create(const int flex = 1) -> std::unique_ptr<Expanded> {
            auto e = std::unique_ptr<Expanded>(new Expanded(flex));
            e->set_background(0, 0, 0, 0);
            return e;
        }

        auto child(std::unique_ptr<Widget> w) -> Expanded& {
            add_child(std::move(w));
            return *this;
        }

        [[nodiscard]] auto flex_factor() const noexcept -> int override { return flex_; }

    private:
        explicit Expanded(const int flex) : flex_(flex) {
        }

        int flex_ = 1;
    };

    /**
     * Fixed-size container. If fixed_w_/fixed_h_ > 0 they override the bounds
     * provided by the parent layout.
     *
     * 固定尺寸容器。如果 fixed_w_ 或 fixed_h_ 大于 0，则会覆盖边界限制。
     * provided by the parent layout.
     */
    class SizedBox final : public Component {
    public:
        static auto Create() -> std::unique_ptr<SizedBox> {
            return std::make_unique<SizedBox>();
        }

        auto width(float w) -> SizedBox& {
            fixed_w_ = w;
            detail::relayout_if_ready(*this);
            return *this;
        }

        auto height(float h) -> SizedBox& {
            fixed_h_ = h;
            detail::relayout_if_ready(*this);
            return *this;
        }

        auto size(const Size &s) -> SizedBox& {
            fixed_w_ = s.width();
            fixed_h_ = s.height();
            detail::relayout_if_ready(*this);
            return *this;
        }

        auto child(std::unique_ptr<Widget> w) -> SizedBox& {
            add_child(std::move(w));
            return *this;
        }

        auto set_bounds(float x, float y, float w, float h) noexcept -> Widget& override {
            const float actual_w = fixed_w_ > 0.0f ? fixed_w_ : w;
            const float actual_h = fixed_h_ > 0.0f ? fixed_h_ : h;
            Component::set_bounds(x, y, actual_w, actual_h);
            for_each_child([&](Widget &c) {
                c.set_bounds(x, y, actual_w, actual_h);
            });
            return *this;
        }

    private:
        float fixed_w_ = 0.0f;
        float fixed_h_ = 0.0f;
    };

    // ── Center ────────────────────────────────────────────────────────────────────
    // Centers its single child within its own bounds (child keeps its own size).
    class Center final : public Component {
    public:
        using CenterComponentPtr = std::unique_ptr<Center>;

        static auto Create() -> CenterComponentPtr {
            return std::make_unique<Center>();
        }

        auto child(std::unique_ptr<Widget> w) -> Center& {
            add_child(std::move(w));
            return *this;
        }

        auto set_bounds(float x, float y, float w, float h) noexcept -> Widget& override {
            Component::set_bounds(x, y, w, h);
            for_each_child([&](Widget &c) {
                const float cx = x + (w - c.width()) * 0.5f;
                const float cy = y + (h - c.height()) * 0.5f;
                c.set_bounds(cx, cy, c.width(), c.height());
            });
            return *this;
        }
    };

    // ── Padding ───────────────────────────────────────────────────────────────────
    // Adds inner padding around a child widget.
    class Padding final : public Component {
    public:
        static auto Create() -> std::unique_ptr<Padding> {
            return std::make_unique<Padding>();
        }

        auto padding(float value) -> Padding& {
            top_ = right_ = bottom_ = left_ = value;
            detail::relayout_if_ready(*this);
            return *this;
        }

        auto padding(float horizontal, float vertical) -> Padding& {
            left_ = right_ = horizontal;
            top_ = bottom_ = vertical;
            detail::relayout_if_ready(*this);
            return *this;
        }

        auto padding(float left, float top, float right, float bottom) -> Padding& {
            left_ = left;
            top_ = top;
            right_ = right;
            bottom_ = bottom;
            detail::relayout_if_ready(*this);
            return *this;
        }

        auto padding_left(float value) -> Padding& {
            left_ = value;
            detail::relayout_if_ready(*this);
            return *this;
        }

        auto padding_top(float value) -> Padding& {
            top_ = value;
            detail::relayout_if_ready(*this);
            return *this;
        }

        auto padding_right(float value) -> Padding& {
            right_ = value;
            detail::relayout_if_ready(*this);
            return *this;
        }

        auto padding_bottom(float value) -> Padding& {
            bottom_ = value;
            detail::relayout_if_ready(*this);
            return *this;
        }

        auto all(float v) -> Padding& {
            return padding(v);
        }

        auto horizontal(float v) -> Padding& {
            left_ = right_ = v;
            detail::relayout_if_ready(*this);
            return *this;
        }

        auto vertical(float v) -> Padding& {
            top_ = bottom_ = v;
            detail::relayout_if_ready(*this);
            return *this;
        }

        auto top(float v) -> Padding& {
            return padding_top(v);
        }

        auto right(float v) -> Padding& {
            return padding_right(v);
        }

        auto bottom(float v) -> Padding& {
            return padding_bottom(v);
        }

        auto left(float v) -> Padding& {
            return padding_left(v);
        }

        auto child(std::unique_ptr<Widget> w) -> Padding& {
            add_child(std::move(w));
            return *this;
        }

        auto set_bounds(float x, float y, float w, float h) noexcept -> Widget& override {
            Component::set_bounds(x, y, w, h);
            for_each_child([&](Widget &c) {
                c.set_bounds(x + left_, y + top_,
                             w - left_ - right_,
                             h - top_ - bottom_);
            });
            return *this;
        }

    private:
        float top_ = 0.0f;
        float right_ = 0.0f;
        float bottom_ = 0.0f;
        float left_ = 0.0f;
    };
} // namespace Nandina
