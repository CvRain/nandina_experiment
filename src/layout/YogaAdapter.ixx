module;

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

export module Nandina.Layout.YogaAdapter;

import Nandina.Layout.YogaMapping;

export namespace Nandina::layout_detail::yoga {

    using NodeHandle = std::uintptr_t;

    enum class Edge : std::uint8_t {
        left,
        top,
        right,
        bottom,
    };

    enum class Gutter : std::uint8_t {
        column,
        row,
    };

    class YogaStyleWriter {
    public:
        virtual ~YogaStyleWriter() = default;

        virtual auto set_flex_direction(NodeHandle node, FlexDirection value) -> void = 0;
        virtual auto set_justify_content(NodeHandle node, Justify value) -> void = 0;
        virtual auto set_align_items(NodeHandle node, Align value) -> void = 0;
        virtual auto set_align_self(NodeHandle node, Align value) -> void = 0;
        virtual auto set_position_type(NodeHandle node, PositionType value) -> void = 0;
        virtual auto set_width(NodeHandle node, StyleValue value) -> void = 0;
        virtual auto set_height(NodeHandle node, StyleValue value) -> void = 0;
        virtual auto set_flex_basis(NodeHandle node, StyleValue value) -> void = 0;
        virtual auto set_gap(NodeHandle node, Gutter gutter, StyleValue value) -> void = 0;
        virtual auto set_padding(NodeHandle node, Edge edge, StyleValue value) -> void = 0;
        virtual auto set_flex_grow(NodeHandle node, float value) -> void = 0;
        virtual auto set_flex_shrink(NodeHandle node, float value) -> void = 0;
    };

    namespace detail {
        inline auto emit_container_style(const ContainerStyle& style,
                                         YogaStyleWriter& writer,
                                         NodeHandle node) -> void {
            writer.set_flex_direction(node, style.flex_direction);
            writer.set_justify_content(node, style.justify_content);
            writer.set_align_items(node, style.align_items);

            if (style.width.is_defined()) {
                writer.set_width(node, style.width);
            }
            if (style.height.is_defined()) {
                writer.set_height(node, style.height);
            }
            if (style.row_gap.is_defined()) {
                writer.set_gap(node, Gutter::row, style.row_gap);
            }
            if (style.column_gap.is_defined()) {
                writer.set_gap(node, Gutter::column, style.column_gap);
            }
            if (style.padding.left.is_defined()) {
                writer.set_padding(node, Edge::left, style.padding.left);
            }
            if (style.padding.top.is_defined()) {
                writer.set_padding(node, Edge::top, style.padding.top);
            }
            if (style.padding.right.is_defined()) {
                writer.set_padding(node, Edge::right, style.padding.right);
            }
            if (style.padding.bottom.is_defined()) {
                writer.set_padding(node, Edge::bottom, style.padding.bottom);
            }
        }

        inline auto emit_child_style(const ChildStyle& style,
                                     YogaStyleWriter& writer,
                                     NodeHandle node) -> void {
            writer.set_position_type(node, style.position_type);

            if (style.align_self != Align::auto_value) {
                writer.set_align_self(node, style.align_self);
            }
            if (style.width.is_defined()) {
                writer.set_width(node, style.width);
            }
            if (style.height.is_defined()) {
                writer.set_height(node, style.height);
            }
            if (style.flex_basis.is_defined()) {
                writer.set_flex_basis(node, style.flex_basis);
            }
            if (style.flex_grow != 0.0f) {
                writer.set_flex_grow(node, style.flex_grow);
            }
            if (style.flex_shrink != 0.0f) {
                writer.set_flex_shrink(node, style.flex_shrink);
            }
        }
    } // namespace detail

    inline auto apply_mapping(const MappingResult& mapping,
                              YogaStyleWriter& writer,
                              NodeHandle root,
                              std::span<const NodeHandle> children) -> void {
        detail::emit_container_style(mapping.container_style, writer, root);

        const auto count = std::min(children.size(), mapping.child_styles.size());
        for (std::size_t index = 0; index < count; ++index) {
            detail::emit_child_style(mapping.child_styles[index], writer, children[index]);
        }
    }

} // namespace Nandina::layout_detail::yoga