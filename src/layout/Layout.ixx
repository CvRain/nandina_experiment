module;
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

export module Nandina.Layout;

import Nandina.Core;
import Nandina.Layout.Backend;
import Nandina.Layout.YogaMapping;
import Nandina.Types;

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

    class LayoutContainer;

    namespace detail {
        [[nodiscard]] inline auto clamp_non_negative(float value) noexcept -> float {
            return value < 0.0f ? 0.0f : value;
        }

        [[nodiscard]] inline auto preferred_width(const Widget& widget) noexcept -> float {
            const float current = widget.width();
            if (current > 0.0f) {
                return current;
            }
            return clamp_non_negative(widget.preferred_size().width());
        }

        [[nodiscard]] inline auto preferred_height(const Widget& widget) noexcept -> float {
            const float current = widget.height();
            if (current > 0.0f) {
                return current;
            }
            return clamp_non_negative(widget.preferred_size().height());
        }

        [[nodiscard]] inline auto map_alignment(Align align) noexcept -> layout_detail::LayoutAlignment {
            switch (align) {
            case Align::center:
                return layout_detail::LayoutAlignment::center;
            case Align::end:
                return layout_detail::LayoutAlignment::end;
            case Align::stretch:
                return layout_detail::LayoutAlignment::stretch;
            case Align::space_between:
                return layout_detail::LayoutAlignment::space_between;
            case Align::space_around:
                return layout_detail::LayoutAlignment::space_around;
            case Align::start:
            default:
                return layout_detail::LayoutAlignment::start;
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

        [[nodiscard]] auto gap_value() const noexcept -> float { return gap_; }
        [[nodiscard]] auto padding_left_value() const noexcept -> float { return padding_left_; }
        [[nodiscard]] auto padding_top_value() const noexcept -> float { return padding_top_; }
        [[nodiscard]] auto padding_right_value() const noexcept -> float { return padding_right_; }
        [[nodiscard]] auto padding_bottom_value() const noexcept -> float { return padding_bottom_; }
        [[nodiscard]] auto align_items_value() const noexcept -> Align { return align_items_; }
        [[nodiscard]] auto justify_content_value() const noexcept -> Align { return justify_content_; }

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

    namespace detail {
        [[nodiscard]] inline auto collect_layout_specs(const LayoutContainer& container) -> std::vector<layout_detail::LayoutChildSpec> {
            std::vector<layout_detail::LayoutChildSpec> specs;
            container.for_each_child([&](Widget& child) {
                specs.push_back({
                    .preferred_size = {preferred_width(child), preferred_height(child)},
                    .flex_factor = child.flex_factor(),
                });
            });
            return specs;
        }

        [[nodiscard]] inline auto collect_layout_targets(const LayoutContainer& container) -> std::vector<Widget*> {
            std::vector<Widget*> targets;
            container.for_each_child([&](Widget& child) {
                targets.push_back(&child);
            });
            return targets;
        }

        [[nodiscard]] inline auto build_layout_request(const LayoutContainer& container,
                                                       layout_detail::LayoutAxis axis) -> layout_detail::LayoutRequest {
            layout_detail::LayoutRequest request;
            request.axis = axis;
            request.container_bounds = {container.x(), container.y(), container.width(), container.height()};
            request.padding = {
                .left = container.padding_left_value(),
                .top = container.padding_top_value(),
                .right = container.padding_right_value(),
                .bottom = container.padding_bottom_value(),
            };
            request.gap = container.gap_value();
            request.cross_alignment = map_alignment(container.align_items_value());
            request.main_alignment = map_alignment(container.justify_content_value());
            request.children = collect_layout_specs(container);

            return request;
        }

        [[nodiscard]] inline auto prepare_yoga_mapping(const LayoutContainer& container,
                                                       layout_detail::LayoutAxis axis) -> layout_detail::yoga::MappingResult {
            return layout_detail::yoga::map_request(build_layout_request(container, axis));
        }

        inline auto apply_backend(LayoutContainer& container, layout_detail::LayoutAxis axis) -> void {
            auto targets = collect_layout_targets(container);
            auto request = build_layout_request(container, axis);

            auto frames = layout_detail::default_layout_backend().compute(request);

            const auto count = std::min(targets.size(), frames.size());
            for (std::size_t index = 0; index < count; ++index) {
                auto* widget = targets[index];
                const auto& frame = frames[index];
                if (widget) {
                    widget->set_bounds(frame.x(), frame.y(), frame.width(), frame.height());
                }
            }
        }
    } // namespace detail

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
            detail::apply_backend(*this, layout_detail::LayoutAxis::column);
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
            detail::apply_backend(*this, layout_detail::LayoutAxis::row);
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
            detail::apply_backend(*this, layout_detail::LayoutAxis::stack);
        }
    };

} // export namespace Nandina
