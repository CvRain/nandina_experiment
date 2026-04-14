#include <cassert>
#include <cstdint>
#include <vector>

import Nandina.Layout.Backend;
import Nandina.Layout.YogaAdapter;
import Nandina.Layout.YogaMapping;

namespace {

using namespace Nandina::layout_detail;
using namespace Nandina::layout_detail::yoga;

enum class OpKind : std::uint8_t {
    flex_direction,
    justify_content,
    align_items,
    align_self,
    position_type,
    width,
    height,
    flex_basis,
    gap,
    padding,
    flex_grow,
    flex_shrink,
};

struct RecordedOp {
    OpKind kind{};
    NodeHandle node = 0;
    FlexDirection flex_direction = FlexDirection::column;
    Justify justify = Justify::flex_start;
    Align align = Align::auto_value;
    PositionType position_type = PositionType::relative;
    Gutter gutter = Gutter::column;
    Edge edge = Edge::left;
    StyleValue value{};
    float scalar = 0.0f;
};

class RecordingWriter final : public YogaStyleWriter {
public:
    auto set_flex_direction(NodeHandle node, FlexDirection value) -> void override {
        ops.push_back({.kind = OpKind::flex_direction, .node = node, .flex_direction = value});
    }

    auto set_justify_content(NodeHandle node, Justify value) -> void override {
        ops.push_back({.kind = OpKind::justify_content, .node = node, .justify = value});
    }

    auto set_align_items(NodeHandle node, Align value) -> void override {
        ops.push_back({.kind = OpKind::align_items, .node = node, .align = value});
    }

    auto set_align_self(NodeHandle node, Align value) -> void override {
        ops.push_back({.kind = OpKind::align_self, .node = node, .align = value});
    }

    auto set_position_type(NodeHandle node, PositionType value) -> void override {
        ops.push_back({.kind = OpKind::position_type, .node = node, .position_type = value});
    }

    auto set_width(NodeHandle node, StyleValue value) -> void override {
        ops.push_back({.kind = OpKind::width, .node = node, .value = value});
    }

    auto set_height(NodeHandle node, StyleValue value) -> void override {
        ops.push_back({.kind = OpKind::height, .node = node, .value = value});
    }

    auto set_flex_basis(NodeHandle node, StyleValue value) -> void override {
        ops.push_back({.kind = OpKind::flex_basis, .node = node, .value = value});
    }

    auto set_gap(NodeHandle node, Gutter gutter, StyleValue value) -> void override {
        ops.push_back({.kind = OpKind::gap, .node = node, .gutter = gutter, .value = value});
    }

    auto set_padding(NodeHandle node, Edge edge, StyleValue value) -> void override {
        ops.push_back({.kind = OpKind::padding, .node = node, .edge = edge, .value = value});
    }

    auto set_flex_grow(NodeHandle node, float value) -> void override {
        ops.push_back({.kind = OpKind::flex_grow, .node = node, .scalar = value});
    }

    auto set_flex_shrink(NodeHandle node, float value) -> void override {
        ops.push_back({.kind = OpKind::flex_shrink, .node = node, .scalar = value});
    }

    std::vector<RecordedOp> ops;
};

auto test_row_mapping_preserves_direct_flex_semantics() -> void {
    LayoutRequest request;
    request.axis = LayoutAxis::row;
    request.container_bounds = {10.0f, 20.0f, 300.0f, 120.0f};
    request.padding = {.left = 12.0f, .top = 8.0f, .right = 6.0f, .bottom = 4.0f};
    request.gap = 16.0f;
    request.cross_alignment = LayoutAlignment::stretch;
    request.main_alignment = LayoutAlignment::space_between;
    request.children = {
        {.preferred_size = {80.0f, 40.0f}, .flex_factor = 0},
        {.preferred_size = {60.0f, 40.0f}, .flex_factor = 1},
    };

    const auto mapping = map_request(request);

    assert(mapping.mode == MappingMode::direct_flex);
    assert(mapping.directness == Directness::fully_direct);
    assert(mapping.container_style.flex_direction == FlexDirection::row);
    assert(mapping.container_style.justify_content == Justify::space_between);
    assert(mapping.container_style.align_items == Align::stretch);
    assert(mapping.container_style.width.unit == Unit::point);
    assert(mapping.container_style.width.value == 300.0f);
    assert(mapping.container_style.column_gap.unit == Unit::point);
    assert(mapping.container_style.column_gap.value == 16.0f);
    assert(mapping.child_styles.size() == 2);
    assert(mapping.child_styles[0].position_type == PositionType::relative);
    assert(mapping.child_styles[1].flex_grow == 1.0f);
    assert(mapping.child_styles[1].flex_shrink == 1.0f);
    assert(mapping.child_styles[1].flex_basis.unit == Unit::point);
    assert(mapping.child_styles[1].flex_basis.value == 0.0f);
}

auto test_stack_mapping_marks_overlay_adapter_boundary() -> void {
    LayoutRequest request;
    request.axis = LayoutAxis::stack;
    request.container_bounds = {0.0f, 0.0f, 200.0f, 100.0f};
    request.padding = {.left = 4.0f, .top = 5.0f, .right = 6.0f, .bottom = 7.0f};
    request.cross_alignment = LayoutAlignment::center;
    request.main_alignment = LayoutAlignment::end;
    request.children = {
        {.preferred_size = {50.0f, 30.0f}, .flex_factor = 0},
    };

    const auto mapping = map_request(request);

    assert(mapping.mode == MappingMode::stack_overlay);
    assert(mapping.directness == Directness::requires_adapter);
    assert(mapping.child_styles.size() == 1);
    assert(mapping.child_styles[0].position_type == PositionType::absolute);
    assert(mapping.child_styles[0].align_self == Align::center);
}

auto test_adapter_emits_expected_style_calls() -> void {
    LayoutRequest request;
    request.axis = LayoutAxis::row;
    request.container_bounds = {0.0f, 0.0f, 320.0f, 180.0f};
    request.padding = {.left = 10.0f, .top = 11.0f, .right = 12.0f, .bottom = 13.0f};
    request.gap = 24.0f;
    request.cross_alignment = LayoutAlignment::center;
    request.main_alignment = LayoutAlignment::center;
    request.children = {
        {.preferred_size = {90.0f, 50.0f}, .flex_factor = 0},
        {.preferred_size = {0.0f, 40.0f}, .flex_factor = 2},
    };

    const auto mapping = map_request(request);
    RecordingWriter writer;
    const std::vector<NodeHandle> children{101, 102};

    apply_mapping(mapping, writer, 100, children);

    assert(!writer.ops.empty());
    assert(writer.ops[0].kind == OpKind::flex_direction);
    assert(writer.ops[0].node == 100);
    assert(writer.ops[0].flex_direction == FlexDirection::row);

    bool saw_gap = false;
    bool saw_left_padding = false;
    bool saw_child_position = false;
    bool saw_child_flex = false;

    for (const auto& op : writer.ops) {
        if (op.kind == OpKind::gap && op.node == 100 && op.gutter == Gutter::column) {
            saw_gap = true;
            assert(op.value.unit == Unit::point);
            assert(op.value.value == 24.0f);
        }
        if (op.kind == OpKind::padding && op.node == 100 && op.edge == Edge::left) {
            saw_left_padding = true;
            assert(op.value.unit == Unit::point);
            assert(op.value.value == 10.0f);
        }
        if (op.kind == OpKind::position_type && op.node == 101) {
            saw_child_position = true;
            assert(op.position_type == PositionType::relative);
        }
        if (op.kind == OpKind::flex_grow && op.node == 102) {
            saw_child_flex = true;
            assert(op.scalar == 2.0f);
        }
    }

    assert(saw_gap);
    assert(saw_left_padding);
    assert(saw_child_position);
    assert(saw_child_flex);
}

} // namespace

auto main() -> int {
    test_row_mapping_preserves_direct_flex_semantics();
    test_stack_mapping_marks_overlay_adapter_boundary();
    test_adapter_emits_expected_style_calls();
    return 0;
}