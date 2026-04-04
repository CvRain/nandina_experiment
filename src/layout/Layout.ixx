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

    // ── LayoutContainer ───────────────────────────────────────────────────────
    class LayoutContainer : public Component {
    public:
        auto gap(float value) -> LayoutContainer& {
            gap_ = value; mark_dirty(); return *this;
        }

        auto padding(float value) -> LayoutContainer& {
            padding_top_ = padding_right_ = padding_bottom_ = padding_left_ = value;
            mark_dirty(); return *this;
        }

        auto padding(float h, float v) -> LayoutContainer& {
            padding_top_ = padding_bottom_ = v;
            padding_left_ = padding_right_ = h;
            mark_dirty(); return *this;
        }

        auto align_items(Align value) -> LayoutContainer& {
            align_items_ = value; mark_dirty(); return *this;
        }

        auto justify_content(Align value) -> LayoutContainer& {
            justify_content_ = value; mark_dirty(); return *this;
        }

        auto add(std::unique_ptr<Widget> child) -> LayoutContainer& {
            add_child(std::move(child));
            return *this;
        }

        virtual auto layout() -> void = 0;

    protected:
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

        auto align_items(Align value) -> Column& {
            LayoutContainer::align_items(value); return *this;
        }

        auto justify_content(Align value) -> Column& {
            LayoutContainer::justify_content(value); return *this;
        }

        auto layout() -> void override {
            const float avail_h = height() - padding_top_ - padding_bottom_;

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
            const float remaining = avail_h - fixed_total - gap_total;

            // ── Pass 2: place children ────────────────────────────────────────
            float cursor_y = y() + padding_top_;
            bool  first    = true;

            for_each_child([&](Widget& child) {
                if (!first) { cursor_y += gap_; }
                first = false;

                const int f = child.flex_factor();
                if (f > 0 && flex_total > 0) {
                    const float alloc_h = remaining * (static_cast<float>(f) / static_cast<float>(flex_total));
                    child.set_bounds(x() + padding_left_, cursor_y,
                                     width() - padding_left_ - padding_right_,
                                     alloc_h);
                    cursor_y += alloc_h;
                } else {
                    child.set_bounds(x() + padding_left_, cursor_y,
                                     width() - padding_left_ - padding_right_,
                                     child.height());
                    cursor_y += child.height();
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

        auto align_items(Align value) -> Row& {
            LayoutContainer::align_items(value); return *this;
        }

        auto justify_content(Align value) -> Row& {
            LayoutContainer::justify_content(value); return *this;
        }

        auto layout() -> void override {
            const float avail_w = width() - padding_left_ - padding_right_;

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
            const float remaining = avail_w - fixed_total - gap_total;

            // ── Pass 2: place children ────────────────────────────────────────
            float cursor_x = x() + padding_left_;
            bool  first    = true;

            for_each_child([&](Widget& child) {
                if (!first) { cursor_x += gap_; }
                first = false;

                const int f = child.flex_factor();
                if (f > 0 && flex_total > 0) {
                    const float alloc_w = remaining * (static_cast<float>(f) / static_cast<float>(flex_total));
                    child.set_bounds(cursor_x, y() + padding_top_,
                                     alloc_w,
                                     height() - padding_top_ - padding_bottom_);
                    cursor_x += alloc_w;
                } else {
                    child.set_bounds(cursor_x, y() + padding_top_,
                                     child.width(),
                                     height() - padding_top_ - padding_bottom_);
                    cursor_x += child.width();
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

        auto layout() -> void override {
            const float cx = x() + padding_left_;
            const float cy = y() + padding_top_;
            const float cw = width()  - padding_left_ - padding_right_;
            const float ch = height() - padding_top_  - padding_bottom_;
            for_each_child([&](Widget& child) {
                child.set_bounds(cx, cy, cw, ch);
            });
        }
    };

} // export namespace Nandina
