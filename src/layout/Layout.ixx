module;
#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

export module Nandina.Layout;

import Nandina.Core;

export import Nandina.Layout.Flex;

export namespace Nandina {

    enum class Align : std::uint8_t {
        start,
        center,
        end,
        stretch,
        space_between,
        space_around,
    };

    namespace detail {
        [[nodiscard]] inline auto clamp_non_negative(float value) noexcept -> float {
            return std::max(0.0f, value);
        }

        [[nodiscard]] inline auto resolve_cross_extent(Align align, float available, float desired) noexcept -> float {
            if (align == Align::stretch) {
                return clamp_non_negative(available);
            }
            return std::min(clamp_non_negative(desired), clamp_non_negative(available));
        }

        [[nodiscard]] inline auto resolve_cross_position(float origin, float available, float extent, Align align) noexcept -> float {
            switch (align) {
            case Align::center:
            case Align::space_between:
            case Align::space_around:
                return origin + (available - extent) * 0.5f;
            case Align::end:
                return origin + (available - extent);
            case Align::start:
            case Align::stretch:
            default:
                return origin;
            }
        }

        struct MainAxisPlacement {
            float start_offset = 0.0f;
            float gap = 0.0f;
        };

        [[nodiscard]] inline auto resolve_main_axis_placement(Align justify,
                                                              int child_count,
                                                              float used_extent,
                                                              float available_extent,
                                                              float base_gap) noexcept -> MainAxisPlacement {
            const float free_space = clamp_non_negative(available_extent - used_extent);

            switch (justify) {
            case Align::end:
                return {.start_offset = free_space, .gap = base_gap};
            case Align::center:
                return {.start_offset = free_space * 0.5f, .gap = base_gap};
            case Align::space_between:
                if (child_count > 1) {
                    return {.start_offset = 0.0f, .gap = base_gap + free_space / static_cast<float>(child_count - 1)};
                }
                return {.start_offset = free_space * 0.5f, .gap = base_gap};
            case Align::space_around:
                if (child_count > 0) {
                    const float extra = free_space / static_cast<float>(child_count);
                    return {.start_offset = extra * 0.5f, .gap = base_gap + extra};
                }
                return {.start_offset = 0.0f, .gap = base_gap};
            case Align::start:
            case Align::stretch:
            default:
                return {.start_offset = 0.0f, .gap = base_gap};
            }
        }
    } // namespace detail

    // ── LayoutContainer ───────────────────────────────────────────────────────
    class LayoutContainer : public Component {
    public:
        auto set_bounds(float x, float y, float width, float height) noexcept -> Widget& override {
            Component::set_bounds(x, y, width, height);
            layout();
            return *this;
        }

        auto gap(float value) -> LayoutContainer& {
            gap_ = value;
            request_layout();
            return *this;
        }

        auto padding(float value) -> LayoutContainer& {
            padding_top_ = padding_right_ = padding_bottom_ = padding_left_ = value;
            request_layout();
            return *this;
        }

        auto padding(float h, float v) -> LayoutContainer& {
            padding_top_ = padding_bottom_ = v;
            padding_left_ = padding_right_ = h;
            request_layout();
            return *this;
        }

        auto padding(float left, float top, float right, float bottom) -> LayoutContainer& {
            padding_left_ = left;
            padding_top_ = top;
            padding_right_ = right;
            padding_bottom_ = bottom;
            request_layout();
            return *this;
        }

        auto padding_left(float value) -> LayoutContainer& {
            padding_left_ = value;
            request_layout();
            return *this;
        }

        auto padding_top(float value) -> LayoutContainer& {
            padding_top_ = value;
            request_layout();
            return *this;
        }

        auto padding_right(float value) -> LayoutContainer& {
            padding_right_ = value;
            request_layout();
            return *this;
        }

        auto padding_bottom(float value) -> LayoutContainer& {
            padding_bottom_ = value;
            request_layout();
            return *this;
        }

        auto align_items(Align value) -> LayoutContainer& {
            align_items_ = value;
            request_layout();
            return *this;
        }

        auto justify_content(Align value) -> LayoutContainer& {
            justify_content_ = value;
            request_layout();
            return *this;
        }

        auto add(std::unique_ptr<Widget> child) -> LayoutContainer& {
            add_child(std::move(child));
            request_layout();
            return *this;
        }

        virtual auto layout() -> void = 0;

    protected:
        auto request_layout() -> void {
            mark_dirty();
            if (width() > 0.0f || height() > 0.0f) {
                layout();
            }
        }

        float gap_            = 0.0f;
        float padding_top_    = 0.0f;
        float padding_right_  = 0.0f;
        float padding_bottom_ = 0.0f;
        float padding_left_   = 0.0f;
        Align align_items_     = Align::start;
        Align justify_content_ = Align::start;
    };

    // ── Column ────────────────────────────────────────────────────────────────
    class Column final : public LayoutContainer {
    public:
        static auto Create() -> std::unique_ptr<Column> {
            return std::make_unique<Column>();
        }

        auto gap(float value) -> Column& {
            LayoutContainer::gap(value); return *this;
        }

        auto padding(float value) -> Column& {
            LayoutContainer::padding(value); return *this;
        }

        auto padding(float h, float v) -> Column& {
            LayoutContainer::padding(h, v); return *this;
        }

        auto padding(float left, float top, float right, float bottom) -> Column& {
            LayoutContainer::padding(left, top, right, bottom); return *this;
        }

        auto padding_left(float value) -> Column& {
            LayoutContainer::padding_left(value); return *this;
        }

        auto padding_top(float value) -> Column& {
            LayoutContainer::padding_top(value); return *this;
        }

        auto padding_right(float value) -> Column& {
            LayoutContainer::padding_right(value); return *this;
        }

        auto padding_bottom(float value) -> Column& {
            LayoutContainer::padding_bottom(value); return *this;
        }

        auto align_items(Align value) -> Column& {
            LayoutContainer::align_items(value); return *this;
        }

        auto justify_content(Align value) -> Column& {
            LayoutContainer::justify_content(value); return *this;
        }

        auto layout() -> void override {
            const float avail_w = detail::clamp_non_negative(width() - padding_left_ - padding_right_);
            const float avail_h = detail::clamp_non_negative(height() - padding_top_ - padding_bottom_);

            // ── Pass 1: sum fixed heights and total flex ──────────────────────
            float fixed_total = 0.0f;
            int   flex_total  = 0;
            int   child_count = 0;

            for_each_child([&](Widget& child) {
                const int f = child.flex_factor();
                if (f > 0) {
                    flex_total += f;
                } else {
                    fixed_total += child.height();
                }
                ++child_count;
            });

            const float gap_total = (child_count > 1)
                ? gap_ * static_cast<float>(child_count - 1) : 0.0f;
            const float remaining = detail::clamp_non_negative(avail_h - fixed_total - gap_total);
            const float flex_inv  = (flex_total > 0) ? 1.0f / static_cast<float>(flex_total) : 0.0f;

            const float used_h = fixed_total + gap_total + (flex_total > 0 ? remaining : 0.0f);
            const auto placement = detail::resolve_main_axis_placement(justify_content_, child_count, used_h, avail_h, gap_);

            // ── Pass 2: place children ────────────────────────────────────────
            float cursor_y = y() + padding_top_ + placement.start_offset;
            bool  first    = true;

            for_each_child([&](Widget& child) {
                if (!first) { cursor_y += placement.gap; }
                first = false;

                const int f = child.flex_factor();
                const float child_h = (f > 0 && flex_total > 0)
                    ? remaining * (static_cast<float>(f) * flex_inv)
                    : child.height();
                const float child_w = detail::resolve_cross_extent(align_items_, avail_w, child.width());
                const float child_x = detail::resolve_cross_position(x() + padding_left_, avail_w, child_w, align_items_);

                if (f > 0 && flex_total > 0) {
                    child.set_bounds(child_x, cursor_y, child_w, child_h);
                    cursor_y += child_h;
                } else {
                    child.set_bounds(child_x, cursor_y, child_w, child_h);
                    cursor_y += child_h;
                }
            });
        }
    };

    // ── Row ───────────────────────────────────────────────────────────────────
    class Row final : public LayoutContainer {
    public:
        static auto Create() -> std::unique_ptr<Row> {
            return std::make_unique<Row>();
        }

        auto gap(float value) -> Row& {
            LayoutContainer::gap(value); return *this;
        }

        auto padding(float value) -> Row& {
            LayoutContainer::padding(value); return *this;
        }

        auto padding(float h, float v) -> Row& {
            LayoutContainer::padding(h, v); return *this;
        }

        auto padding(float left, float top, float right, float bottom) -> Row& {
            LayoutContainer::padding(left, top, right, bottom); return *this;
        }

        auto padding_left(float value) -> Row& {
            LayoutContainer::padding_left(value); return *this;
        }

        auto padding_top(float value) -> Row& {
            LayoutContainer::padding_top(value); return *this;
        }

        auto padding_right(float value) -> Row& {
            LayoutContainer::padding_right(value); return *this;
        }

        auto padding_bottom(float value) -> Row& {
            LayoutContainer::padding_bottom(value); return *this;
        }

        auto align_items(Align value) -> Row& {
            LayoutContainer::align_items(value); return *this;
        }

        auto justify_content(Align value) -> Row& {
            LayoutContainer::justify_content(value); return *this;
        }

        auto layout() -> void override {
            const float avail_w = detail::clamp_non_negative(width() - padding_left_ - padding_right_);
            const float avail_h = detail::clamp_non_negative(height() - padding_top_ - padding_bottom_);

            // ── Pass 1: sum fixed widths and total flex ───────────────────────
            float fixed_total = 0.0f;
            int   flex_total  = 0;
            int   child_count = 0;

            for_each_child([&](Widget& child) {
                const int f = child.flex_factor();
                if (f > 0) {
                    flex_total += f;
                } else {
                    fixed_total += child.width();
                }
                ++child_count;
            });

            const float gap_total = (child_count > 1)
                ? gap_ * static_cast<float>(child_count - 1) : 0.0f;
            const float remaining = detail::clamp_non_negative(avail_w - fixed_total - gap_total);
            const float flex_inv  = (flex_total > 0) ? 1.0f / static_cast<float>(flex_total) : 0.0f;

            const float used_w = fixed_total + gap_total + (flex_total > 0 ? remaining : 0.0f);
            const auto placement = detail::resolve_main_axis_placement(justify_content_, child_count, used_w, avail_w, gap_);

            // ── Pass 2: place children ────────────────────────────────────────
            float cursor_x = x() + padding_left_ + placement.start_offset;
            bool  first    = true;

            for_each_child([&](Widget& child) {
                if (!first) { cursor_x += placement.gap; }
                first = false;

                const int f = child.flex_factor();
                const float child_w = (f > 0 && flex_total > 0)
                    ? remaining * (static_cast<float>(f) * flex_inv)
                    : child.width();
                const float child_h = detail::resolve_cross_extent(align_items_, avail_h, child.height());
                const float child_y = detail::resolve_cross_position(y() + padding_top_, avail_h, child_h, align_items_);

                if (f > 0 && flex_total > 0) {
                    child.set_bounds(cursor_x, child_y, child_w, child_h);
                    cursor_x += child_w;
                } else {
                    child.set_bounds(cursor_x, child_y, child_w, child_h);
                    cursor_x += child_w;
                }
            });
        }
    };

    // ── Stack ─────────────────────────────────────────────────────────────────
    class Stack final : public LayoutContainer {
    public:
        static auto Create() -> std::unique_ptr<Stack> {
            return std::make_unique<Stack>();
        }

        auto gap(float value) -> Stack& {
            LayoutContainer::gap(value); return *this;
        }

        auto padding(float value) -> Stack& {
            LayoutContainer::padding(value); return *this;
        }

        auto padding(float h, float v) -> Stack& {
            LayoutContainer::padding(h, v); return *this;
        }

        auto padding(float left, float top, float right, float bottom) -> Stack& {
            LayoutContainer::padding(left, top, right, bottom); return *this;
        }

        auto padding_left(float value) -> Stack& {
            LayoutContainer::padding_left(value); return *this;
        }

        auto padding_top(float value) -> Stack& {
            LayoutContainer::padding_top(value); return *this;
        }

        auto padding_right(float value) -> Stack& {
            LayoutContainer::padding_right(value); return *this;
        }

        auto padding_bottom(float value) -> Stack& {
            LayoutContainer::padding_bottom(value); return *this;
        }

        auto align_items(Align value) -> Stack& {
            LayoutContainer::align_items(value); return *this;
        }

        auto justify_content(Align value) -> Stack& {
            LayoutContainer::justify_content(value); return *this;
        }

        auto layout() -> void override {
            const float cx = x() + padding_left_;
            const float cy = y() + padding_top_;
            const float cw = detail::clamp_non_negative(width()  - padding_left_ - padding_right_);
            const float ch = detail::clamp_non_negative(height() - padding_top_  - padding_bottom_);
            for_each_child([&](Widget& child) {
                const float child_w = detail::resolve_cross_extent(align_items_, cw, child.width());
                const float child_h = detail::resolve_cross_extent(justify_content_, ch, child.height());
                const float child_x = detail::resolve_cross_position(cx, cw, child_w, align_items_);
                const float child_y = detail::resolve_cross_position(cy, ch, child_h, justify_content_);
                child.set_bounds(child_x, child_y, child_w, child_h);
            });
        }
    };

} // export namespace Nandina
