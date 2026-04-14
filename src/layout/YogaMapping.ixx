module;

#include <cstdint>
#include <vector>

export module Nandina.Layout.YogaMapping;

import Nandina.Layout.Backend;

export namespace Nandina::layout_detail::yoga {

    enum class FlexDirection : std::uint8_t {
        column,
        column_reverse,
        row,
        row_reverse,
    };

    enum class Justify : std::uint8_t {
        flex_start,
        center,
        flex_end,
        space_between,
        space_around,
        space_evenly,
    };

    enum class Align : std::uint8_t {
        auto_value,
        flex_start,
        center,
        flex_end,
        stretch,
        baseline,
        space_between,
        space_around,
    };

    enum class PositionType : std::uint8_t {
        relative,
        absolute,
    };

    enum class Unit : std::uint8_t {
        undefined,
        point,
        percent,
        auto_value,
    };

    struct StyleValue {
        float value = 0.0f;
        Unit unit = Unit::undefined;

        [[nodiscard]] static auto undefined() noexcept -> StyleValue {
            return {};
        }

        [[nodiscard]] static auto point(float value) noexcept -> StyleValue {
            return {.value = value, .unit = Unit::point};
        }

        [[nodiscard]] static auto percent(float value) noexcept -> StyleValue {
            return {.value = value, .unit = Unit::percent};
        }

        [[nodiscard]] static auto auto_value() noexcept -> StyleValue {
            return {.value = 0.0f, .unit = Unit::auto_value};
        }

        [[nodiscard]] auto is_defined() const noexcept -> bool {
            return unit != Unit::undefined;
        }
    };

    struct Edges {
        StyleValue left{};
        StyleValue top{};
        StyleValue right{};
        StyleValue bottom{};
    };

    struct ContainerStyle {
        FlexDirection flex_direction = FlexDirection::column;
        Justify justify_content = Justify::flex_start;
        Align align_items = Align::stretch;
        StyleValue width{};
        StyleValue height{};
        StyleValue row_gap{};
        StyleValue column_gap{};
        Edges padding{};
    };

    struct ChildStyle {
        PositionType position_type = PositionType::relative;
        Align align_self = Align::auto_value;
        StyleValue width{};
        StyleValue height{};
        StyleValue flex_basis{};
        float flex_grow = 0.0f;
        float flex_shrink = 0.0f;
    };

    enum class MappingMode : std::uint8_t {
        direct_flex,
        stack_overlay,
    };

    enum class Directness : std::uint8_t {
        fully_direct,
        requires_adapter,
    };

    struct MappingResult {
        MappingMode mode = MappingMode::direct_flex;
        Directness directness = Directness::fully_direct;
        ContainerStyle container_style{};
        std::vector<ChildStyle> child_styles;
    };

    namespace detail {
        [[nodiscard]] inline auto map_justify(LayoutAlignment alignment) noexcept -> Justify {
            switch (alignment) {
            case LayoutAlignment::center:
                return Justify::center;
            case LayoutAlignment::end:
                return Justify::flex_end;
            case LayoutAlignment::space_between:
                return Justify::space_between;
            case LayoutAlignment::space_around:
                return Justify::space_around;
            case LayoutAlignment::start:
            case LayoutAlignment::stretch:
            default:
                return Justify::flex_start;
            }
        }

        [[nodiscard]] inline auto map_align(LayoutAlignment alignment) noexcept -> Align {
            switch (alignment) {
            case LayoutAlignment::center:
                return Align::center;
            case LayoutAlignment::end:
                return Align::flex_end;
            case LayoutAlignment::stretch:
                return Align::stretch;
            case LayoutAlignment::space_between:
                return Align::space_between;
            case LayoutAlignment::space_around:
                return Align::space_around;
            case LayoutAlignment::start:
            default:
                return Align::flex_start;
            }
        }
    } // namespace detail

    [[nodiscard]] inline auto map_request(const LayoutRequest& request) -> MappingResult {
        MappingResult result;
        result.container_style.padding = {
            .left = StyleValue::point(request.padding.left),
            .top = StyleValue::point(request.padding.top),
            .right = StyleValue::point(request.padding.right),
            .bottom = StyleValue::point(request.padding.bottom),
        };
        result.container_style.width = StyleValue::point(request.container_bounds.width());
        result.container_style.height = StyleValue::point(request.container_bounds.height());
        result.container_style.justify_content = detail::map_justify(request.main_alignment);
        result.container_style.align_items = detail::map_align(request.cross_alignment);

        switch (request.axis) {
        case LayoutAxis::column:
            result.container_style.flex_direction = FlexDirection::column;
            result.container_style.row_gap = StyleValue::point(request.gap);
            break;
        case LayoutAxis::row:
            result.container_style.flex_direction = FlexDirection::row;
            result.container_style.column_gap = StyleValue::point(request.gap);
            break;
        case LayoutAxis::stack:
            result.mode = MappingMode::stack_overlay;
            result.directness = Directness::requires_adapter;
            result.container_style.flex_direction = FlexDirection::column;
            break;
        }

        result.child_styles.reserve(request.children.size());

        for (const auto& child : request.children) {
            ChildStyle style;
            style.width = child.preferred_size.width() > 0.0f
                ? StyleValue::point(child.preferred_size.width())
                : StyleValue::undefined();
            style.height = child.preferred_size.height() > 0.0f
                ? StyleValue::point(child.preferred_size.height())
                : StyleValue::undefined();

            if (child.flex_factor > 0) {
                style.flex_grow = static_cast<float>(child.flex_factor);
                style.flex_shrink = 1.0f;
                style.flex_basis = StyleValue::point(0.0f);
            }

            if (request.axis == LayoutAxis::stack) {
                style.position_type = PositionType::absolute;
                style.align_self = detail::map_align(request.cross_alignment);
            }

            result.child_styles.push_back(style);
        }

        return result;
    }

} // namespace Nandina::layout_detail::yoga