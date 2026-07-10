//
// Widget-layer unit tests (Catch2 v3).
//
// Verifies the scene + reactive seam: a NanControl has size-based hit-testing and
// bounds; a BindableRect bound to signals updates its scene-node attributes when
// those signals change, and stops updating once unmounted (EffectScope cleared).
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "reactive/reactive.hpp"
#include "render/draw_context.hpp"
#include "render/render_device.hpp"
#include "scene/control.hpp"
#include "scene/scene_tree.hpp"
#include "widget/bindable_rect.hpp"
#include "widget/button.hpp"
#include "widget/label.hpp"
#include "widget/layout.hpp"
#include "widget/primitives/editable_text.hpp"
#include "widget/primitives/text.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

using namespace nandina;

namespace
{

/// Minimal recording device (rect fills only) for asserting draw output.
class RecordingDevice final : public render::IRenderDevice {
public:
    struct RectCall {
        foundation::NanRect rect;
        float alpha;
    };
    struct TextCall {
        std::string text;
        float alpha;
    };
    int line_count = 0;
    std::vector<RectCall> rects;
    std::vector<TextCall> texts;

    void begin_frame() override {}
    void end_frame() override {}
    void set_clip(const foundation::NanRect&) override {}
    void clear_clip() override {}
    void draw_rect(const foundation::NanRect& r, const foundation::NanColor& c) override {
        rects.push_back({r, c.alpha()});
    }
    void draw_rect_outline(const foundation::NanRect&, float, const foundation::NanColor&) override {}
    void draw_rounded_rect(const foundation::NanRect&, float, const foundation::NanColor&) override {}
    void draw_line(const foundation::NanPoint&, const foundation::NanPoint&, float,
                   const foundation::NanColor&) override {
        ++line_count;
    }
    void draw_circle(const foundation::NanPoint&, float, const foundation::NanColor&) override {}
    void draw_text(std::string_view text, const foundation::NanPoint&, float,
                   const foundation::NanColor& color) override {
        texts.push_back({std::string(text), color.alpha()});
    }
};

auto opaque_color(float light) -> foundation::NanColor {
    return foundation::NanColor::from(
        foundation::NanOklch{.light = light, .chroma = 0.1F, .hue = 120.0F, .alpha = 1.0F});
}

} // namespace

TEST_CASE("NanControl rect hit-test and bounds", "[widget][control]") {
    scene::NanControl c{foundation::NanSize(40, 20)};
    c.set_position(foundation::NanPoint(100, 50));

    SECTION("contains_point uses [0,w]x[0,h] local box") {
        REQUIRE(c.contains_point(foundation::NanPoint(0, 0)));
        REQUIRE(c.contains_point(foundation::NanPoint(40, 20)));
        REQUIRE(c.contains_point(foundation::NanPoint(20, 10)));
        REQUIRE_FALSE(c.contains_point(foundation::NanPoint(-1, 5)));
        REQUIRE_FALSE(c.contains_point(foundation::NanPoint(41, 5)));
    }

    SECTION("global_bounds reflects position + size") {
        const auto b = c.global_bounds();
        REQUIRE(b.get_left() == Catch::Approx(100.0F));
        REQUIRE(b.get_top() == Catch::Approx(50.0F));
        REQUIRE(b.get_width() == Catch::Approx(40.0F));
        REQUIRE(b.get_height() == Catch::Approx(20.0F));
    }
}

TEST_CASE("NanControl draws its background rect", "[widget][control][render]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto root = std::make_shared<scene::NanControl>(foundation::NanSize(10, 10));
    root->set_background(opaque_color(0.5F));
    tree.set_root(root);

    tree.draw(dev);
    REQUIRE(dev.rects.size() == 1);
    REQUIRE(dev.rects[0].rect.get_width() == Catch::Approx(10.0F));
}

TEST_CASE("BindableRect: signal drives visibility across frames", "[widget][reactive]") {
    reactive::Graph g;
    reactive::Signal<bool> visible{g, true};

    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto rect = std::make_shared<widget::BindableRect>(g, foundation::NanSize(10, 10));
    rect->set_background(opaque_color(0.5F));
    rect->bind_visible(visible);
    tree.set_root(rect);  // on_ready fires bind() -> effect reads visible

    // Frame 1: visible == true, one draw.
    tree.draw(dev);
    REQUIRE(dev.rects.size() == 1);

    // Flip signal; effect re-runs and sets visible=false. Next frame: no draw.
    visible.set(false);
    dev.rects.clear();
    tree.draw(dev);
    REQUIRE(dev.rects.empty());

    // Flip back.
    visible.set(true);
    dev.rects.clear();
    tree.draw(dev);
    REQUIRE(dev.rects.size() == 1);
}

TEST_CASE("BindableRect: signal drives background color", "[widget][reactive]") {
    reactive::Graph g;
    reactive::Signal<foundation::NanColor> color{g, opaque_color(0.3F)};

    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto rect = std::make_shared<widget::BindableRect>(g, foundation::NanSize(10, 10));
    rect->bind_background(color);
    tree.set_root(rect);

    tree.draw(dev);
    REQUIRE(dev.rects.size() == 1);
    const float a0 = dev.rects[0].alpha;
    REQUIRE(a0 == Catch::Approx(1.0F));  // opaque

    // Change to a semi-transparent color; next frame reflects it.
    color.set(opaque_color(0.3F).with_alpha(0.5F));
    dev.rects.clear();
    tree.draw(dev);
    REQUIRE(dev.rects.size() == 1);
    REQUIRE(dev.rects[0].alpha == Catch::Approx(0.5F));
}

TEST_CASE("BindableRect: signal drives position", "[widget][reactive]") {
    reactive::Graph g;
    reactive::Signal<foundation::NanPoint> pos{g, foundation::NanPoint(0, 0)};

    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto rect = std::make_shared<widget::BindableRect>(g, foundation::NanSize(10, 10));
    rect->set_background(opaque_color(0.5F));
    rect->bind_position(pos);
    tree.set_root(rect);

    tree.draw(dev);
    REQUIRE(dev.rects[0].rect.get_left() == Catch::Approx(0.0F));

    pos.set(foundation::NanPoint(100, 25));
    dev.rects.clear();
    tree.draw(dev);
    REQUIRE(dev.rects[0].rect.get_left() == Catch::Approx(100.0F));
    REQUIRE(dev.rects[0].rect.get_top() == Catch::Approx(25.0F));
}

TEST_CASE("BindableRect: unmount clears subscriptions (no update after removal)", "[widget][reactive][lifecycle]") {
    reactive::Graph g;
    reactive::Signal<bool> visible{g, true};

    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto root = std::make_shared<scene::NanControl>(foundation::NanSize(200, 200));
    auto rect_owned = std::make_shared<widget::BindableRect>(g, foundation::NanSize(10, 10));
    auto* rect = rect_owned.get();
    rect->set_background(opaque_color(0.5F));
    rect->bind_visible(visible);
    root->add_child(rect_owned);
    tree.set_root(root);

    // Bound and reacting.
    visible.set(false);
    REQUIRE_FALSE(rect->visible());

    // Remove from tree: on_exit_tree clears the EffectScope.
    auto detached = root->remove_child(*rect);
    // Signal changes now must not touch the detached widget.
    REQUIRE_NOTHROW(visible.set(true));
    // Its visible flag stays at the last bound value (no effect re-ran).
    REQUIRE_FALSE(rect->visible());
}

TEST_CASE("Label text follows computed state changed by buttons", "[widget][label][button][reactive]") {
    reactive::Graph graph;
    reactive::Signal<int> count {graph, 0};
    auto* text = reactive::make_computed(graph, [&] {
        return std::string("Count: ") + std::to_string(count.get());
    });

    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto root = std::make_shared<scene::NanControl>(foundation::NanSize(320.0F, 160.0F));

    auto label = std::make_shared<widget::Label>(graph);
    label->bind_text(*text);
    root->add_child(label);

    auto decrement = std::make_shared<widget::Button>("-1");
    decrement->set_position(foundation::NanPoint(0.0F, 48.0F));
    decrement->set_on_click([&] { count.update([](int& value) { --value; }); });
    root->add_child(decrement);

    auto increment = std::make_shared<widget::Button>("+1");
    increment->set_position(foundation::NanPoint(80.0F, 48.0F));
    increment->set_on_click([&] { count.update([](int& value) { ++value; }); });
    root->add_child(increment);

    tree.set_root(root);

    tree.draw(dev);
    REQUIRE_FALSE(dev.texts.empty());
    REQUIRE(dev.texts.front().text == "Count: 0");

    tree.dispatch_mouse_move(scene::MouseMoveEvent {
        foundation::NanPoint(90.0F, 58.0F),
        foundation::NanPoint::zero(),
    });
    tree.dispatch_mouse_button(scene::MouseButtonEvent {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        foundation::NanPoint(90.0F, 58.0F),
    });
    tree.dispatch_mouse_button(scene::MouseButtonEvent {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::release,
        foundation::NanPoint(90.0F, 58.0F),
    });

    dev.texts.clear();
    tree.draw(dev);
    REQUIRE(dev.texts.front().text == "Count: 1");

    tree.dispatch_mouse_move(scene::MouseMoveEvent {
        foundation::NanPoint(10.0F, 58.0F),
        foundation::NanPoint(-80.0F, 0.0F),
    });
    tree.dispatch_mouse_button(scene::MouseButtonEvent {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        foundation::NanPoint(10.0F, 58.0F),
    });
    tree.dispatch_mouse_button(scene::MouseButtonEvent {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::release,
        foundation::NanPoint(10.0F, 58.0F),
    });

    dev.texts.clear();
    tree.draw(dev);
    REQUIRE(dev.texts.front().text == "Count: 0");
}

TEST_CASE("Row Column and Padding arrange control children", "[widget][layout]") {
    auto a = std::make_shared<scene::NanControl>(foundation::NanSize(20.0F, 10.0F));
    auto b = std::make_shared<scene::NanControl>(foundation::NanSize(30.0F, 16.0F));
    auto c = std::make_shared<scene::NanControl>(foundation::NanSize(12.0F, 8.0F));

    auto row = widget::Row::create();
    row->set_gap(5.0F).add(a).add(b);

    REQUIRE(row->width() == Catch::Approx(55.0F));
    REQUIRE(row->height() == Catch::Approx(16.0F));
    REQUIRE(a->position().get_x() == Catch::Approx(0.0F));
    REQUIRE(b->position().get_x() == Catch::Approx(25.0F));

    auto column = widget::Column::create();
    column->set_gap(7.0F).add(row).add(c);

    REQUIRE(column->width() == Catch::Approx(55.0F));
    REQUIRE(column->height() == Catch::Approx(31.0F));
    REQUIRE(row->position().get_y() == Catch::Approx(0.0F));
    REQUIRE(c->position().get_y() == Catch::Approx(23.0F));

    auto padding = widget::Padding::create(foundation::NanInsets::symmetric(11.0F, 13.0F));
    padding->set_child(column);

    REQUIRE(column->position().get_x() == Catch::Approx(11.0F));
    REQUIRE(column->position().get_y() == Catch::Approx(13.0F));
    REQUIRE(padding->width() == Catch::Approx(77.0F));
    REQUIRE(padding->height() == Catch::Approx(57.0F));
}

TEST_CASE("Layout constraints remeasure a subtree for changing root bounds", "[widget][layout][responsive]") {
    auto root = std::make_shared<scene::NanControl>();
    auto text = std::make_shared<widget::primitives::Text>("a very long line of text");
    text->set_overflow(widget::primitives::TextOverflow::ellipsis);

    auto padding = widget::Padding::create(foundation::NanInsets::all(10.0F));
    padding->set_child(text);
    root->add_child(padding);

    (void)root->measure_layout(scene::LayoutConstraints::tight(foundation::NanSize(100.0F, 80.0F)));
    root->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 100.0F, 80.0F));

    REQUIRE(padding->width() <= 100.0F);
    REQUIRE(text->width() <= 80.0F);

    (void)root->measure_layout(scene::LayoutConstraints::tight(foundation::NanSize(320.0F, 80.0F)));
    root->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 320.0F, 80.0F));

    REQUIRE(padding->width() > 100.0F);
    REQUIRE(text->width() > 80.0F);
}

TEST_CASE("Text size changes mark ancestor layout dirty", "[widget][layout][text]") {
    auto row = widget::Row::create();
    auto text = std::make_shared<widget::primitives::Text>("small");
    row->add(text);

    row->clear_layout_dirty();
    text->clear_layout_dirty();

    text->set_text("a much larger label");
    REQUIRE(text->layout_dirty());
    REQUIRE(row->layout_dirty());

    row->relayout();
    REQUIRE_FALSE(row->layout_dirty());
    REQUIRE(row->width() == Catch::Approx(text->width()));
}

TEST_CASE("TextStyle updates text measurement and drawing style", "[widget][text]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto text = std::make_shared<widget::primitives::Text>("abcdef");
    text->set_style(widget::primitives::TextStyle {
        .color = opaque_color(0.7F).with_alpha(0.5F),
        .font_size = 20.0F,
        .overflow = widget::primitives::TextOverflow::clip,
        .max_lines = 1,
    });
    (void)text->measure_layout(scene::LayoutConstraints {
        .min_width = 0.0F,
        .max_width = 40.0F,
        .min_height = 0.0F,
        .max_height = 80.0F,
    });
    tree.set_root(text);

    tree.draw(dev);

    REQUIRE(text->font_size() == Catch::Approx(20.0F));
    REQUIRE(text->overflow() == widget::primitives::TextOverflow::clip);
    REQUIRE(text->width() <= 40.0F);
    REQUIRE(dev.texts.size() == 1);
    REQUIRE(dev.texts[0].alpha == Catch::Approx(0.5F));
}

TEST_CASE("Text exposes a layout result shared by measure and draw", "[widget][text][layout]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto text = std::make_shared<widget::primitives::Text>("abcdefghi");
    text->set_style(widget::primitives::TextStyle {
        .color = opaque_color(0.8F),
        .font_size = 10.0F,
        .overflow = widget::primitives::TextOverflow::ellipsis,
        .max_lines = 1,
    });

    (void)text->measure_layout(scene::LayoutConstraints {
        .min_width = 0.0F,
        .max_width = 28.0F,
        .min_height = 0.0F,
        .max_height = 80.0F,
    });
    tree.set_root(text);
    tree.draw(dev);

    const auto& layout = text->layout_result();
    REQUIRE(layout.overflowed);
    REQUIRE(layout.font_size == Catch::Approx(10.0F));
    REQUIRE(layout.size.get_width() <= 28.0F);
    REQUIRE(layout.lines.size() == 1);
    REQUIRE(layout.lines.front().visible_text == dev.texts.front().text);
    REQUIRE(layout.lines.front().visible_text.ends_with("..."));
}

TEST_CASE("EditableText accepts focused text input and backspace", "[widget][editable-text]") {
    auto edit = std::make_shared<widget::primitives::EditableText>("A");
    std::vector<std::string> changes;
    edit->set_on_change([&](std::string_view value) { changes.emplace_back(value); });

    scene::NanSceneTree tree;
    tree.set_root(edit);
    tree.dispatch_text_input(scene::TextInputEvent("x"));
    REQUIRE(edit->value() == "A");

    tree.set_focus(edit.get());
    tree.dispatch_text_input(scene::TextInputEvent("x"));
    REQUIRE(edit->value() == "Ax");
    REQUIRE(edit->caret() == 2);
    REQUIRE(changes.back() == "Ax");

    tree.dispatch_key(scene::KeyEvent(259, scene::KeyEvent::Action::press));
    REQUIRE(edit->value() == "A");
    REQUIRE(edit->caret() == 1);
    REQUIRE(changes.back() == "A");
}

TEST_CASE("EditableText draws text and focused caret", "[widget][editable-text]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto edit = std::make_shared<widget::primitives::EditableText>("Edit");
    edit->set_style(widget::primitives::TextStyle {
        .color = opaque_color(0.8F),
        .font_size = 16.0F,
        .overflow = widget::primitives::TextOverflow::ellipsis,
        .max_lines = 1,
    });
    tree.set_root(edit);
    tree.set_focus(edit.get());

    tree.draw(dev);

    REQUIRE(dev.texts.size() == 1);
    REQUIRE(dev.texts.front().text == "Edit");
    REQUIRE(dev.line_count == 1);
}

TEST_CASE("Row alignment positions children inside assigned bounds", "[widget][layout][alignment]") {
    auto a = std::make_shared<scene::NanControl>(foundation::NanSize(20.0F, 10.0F));
    auto b = std::make_shared<scene::NanControl>(foundation::NanSize(30.0F, 14.0F));

    auto row = widget::Row::create();
    row->set_gap(10.0F)
        .set_main_alignment(widget::LayoutAlignment::center)
        .set_cross_alignment(widget::LayoutAlignment::end)
        .add(a)
        .add(b);

    (void)row->measure_layout(scene::LayoutConstraints::tight(foundation::NanSize(120.0F, 40.0F)));
    row->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 120.0F, 40.0F));

    REQUIRE(a->position().get_x() == Catch::Approx(30.0F));
    REQUIRE(a->position().get_y() == Catch::Approx(30.0F));
    REQUIRE(b->position().get_x() == Catch::Approx(60.0F));
    REQUIRE(b->position().get_y() == Catch::Approx(26.0F));
}

TEST_CASE("Column cross stretch expands children to assigned width", "[widget][layout][alignment]") {
    auto a = std::make_shared<scene::NanControl>(foundation::NanSize(20.0F, 10.0F));
    auto b = std::make_shared<scene::NanControl>(foundation::NanSize(30.0F, 14.0F));

    auto column = widget::Column::create();
    column->set_gap(6.0F)
        .set_main_alignment(widget::LayoutAlignment::end)
        .set_cross_alignment(widget::LayoutAlignment::stretch)
        .add(a)
        .add(b);

    (void)column->measure_layout(scene::LayoutConstraints::tight(foundation::NanSize(90.0F, 60.0F)));
    column->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 90.0F, 60.0F));

    REQUIRE(a->position().get_y() == Catch::Approx(30.0F));
    REQUIRE(a->width() == Catch::Approx(90.0F));
    REQUIRE(b->position().get_y() == Catch::Approx(46.0F));
    REQUIRE(b->width() == Catch::Approx(90.0F));
}

TEST_CASE("Center positions a single child in assigned bounds", "[widget][layout][center]") {
    auto child = std::make_shared<scene::NanControl>(foundation::NanSize(40.0F, 20.0F));
    auto center = widget::Center::create();
    center->set_child(child);

    (void)center->measure_layout(scene::LayoutConstraints::tight(foundation::NanSize(120.0F, 80.0F)));
    center->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 120.0F, 80.0F));

    REQUIRE(child->position().get_x() == Catch::Approx(40.0F));
    REQUIRE(child->position().get_y() == Catch::Approx(30.0F));
    REQUIRE(child->width() == Catch::Approx(40.0F));
    REQUIRE(child->height() == Catch::Approx(20.0F));
}

TEST_CASE("NanControl single child layout fills parent bounds", "[widget][layout][root]") {
    auto root = std::make_shared<scene::NanControl>();
    auto child = std::make_shared<scene::NanControl>(foundation::NanSize(40.0F, 20.0F));
    root->add_child(child);

    (void)root->measure_layout(scene::LayoutConstraints::tight(foundation::NanSize(200.0F, 120.0F)));
    root->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 200.0F, 120.0F));

    REQUIRE(child->position().get_x() == Catch::Approx(0.0F));
    REQUIRE(child->position().get_y() == Catch::Approx(0.0F));
    REQUIRE(child->width() == Catch::Approx(200.0F));
    REQUIRE(child->height() == Catch::Approx(120.0F));
}

TEST_CASE("Row distributes remaining width across Expanded children", "[widget][layout][expanded]") {
    auto fixed = std::make_shared<scene::NanControl>(foundation::NanSize(30.0F, 10.0F));
    auto first_child = std::make_shared<scene::NanControl>(foundation::NanSize(5.0F, 10.0F));
    auto second_child = std::make_shared<scene::NanControl>(foundation::NanSize(5.0F, 10.0F));
    auto first = widget::Expanded::create(1);
    auto second = widget::Expanded::create(2);
    first->set_child(first_child);
    second->set_child(second_child);

    auto row = widget::Row::create();
    row->set_gap(5.0F).add(fixed).add(first).add(second);

    (void)row->measure_layout(scene::LayoutConstraints::tight(foundation::NanSize(155.0F, 20.0F)));
    row->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 155.0F, 20.0F));

    REQUIRE(fixed->width() == Catch::Approx(30.0F));
    REQUIRE(first->position().get_x() == Catch::Approx(35.0F));
    REQUIRE(first->width() == Catch::Approx(38.333F).margin(0.01F));
    REQUIRE(second->position().get_x() == Catch::Approx(78.333F).margin(0.01F));
    REQUIRE(second->width() == Catch::Approx(76.666F).margin(0.01F));
    REQUIRE(first_child->width() == Catch::Approx(first->width()));
    REQUIRE(second_child->width() == Catch::Approx(second->width()));
}

TEST_CASE("Column distributes remaining height across Expanded children", "[widget][layout][expanded]") {
    auto fixed = std::make_shared<scene::NanControl>(foundation::NanSize(20.0F, 12.0F));
    auto expanded_child = std::make_shared<scene::NanControl>(foundation::NanSize(20.0F, 5.0F));
    auto expanded = widget::Expanded::create();
    expanded->set_child(expanded_child);

    auto column = widget::Column::create();
    column->set_gap(8.0F).add(fixed).add(expanded);

    (void)column->measure_layout(scene::LayoutConstraints::tight(foundation::NanSize(80.0F, 100.0F)));
    column->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 80.0F, 100.0F));

    REQUIRE(fixed->height() == Catch::Approx(12.0F));
    REQUIRE(expanded->position().get_y() == Catch::Approx(20.0F));
    REQUIRE(expanded->height() == Catch::Approx(80.0F));
    REQUIRE(expanded_child->height() == Catch::Approx(80.0F));
}

TEST_CASE("Flex supports both axes and Expanded children", "[widget][layout][flex]") {
    auto fixed = std::make_shared<scene::NanControl>(foundation::NanSize(24.0F, 10.0F));
    auto expanded_child = std::make_shared<scene::NanControl>(foundation::NanSize(4.0F, 4.0F));
    auto expanded = widget::Expanded::create(2);
    expanded->set_child(expanded_child);

    auto flex = widget::Flex::create(widget::LayoutAxis::horizontal);
    flex->set_gap(6.0F)
        .set_cross_alignment(widget::LayoutAlignment::stretch)
        .add(fixed)
        .add(expanded);

    (void)flex->measure_layout(scene::LayoutConstraints::tight(foundation::NanSize(120.0F, 30.0F)));
    flex->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 120.0F, 30.0F));

    REQUIRE(fixed->position().get_x() == Catch::Approx(0.0F));
    REQUIRE(fixed->height() == Catch::Approx(30.0F));
    REQUIRE(expanded->position().get_x() == Catch::Approx(30.0F));
    REQUIRE(expanded->width() == Catch::Approx(90.0F));
    REQUIRE(expanded_child->width() == Catch::Approx(90.0F));

    auto vertical_fixed = std::make_shared<scene::NanControl>(foundation::NanSize(24.0F, 10.0F));
    auto vertical_expanded_child = std::make_shared<scene::NanControl>(foundation::NanSize(4.0F, 4.0F));
    auto vertical_expanded = widget::Expanded::create(2);
    vertical_expanded->set_child(vertical_expanded_child);

    auto vertical = widget::Flex::create(widget::LayoutAxis::vertical);
    vertical->set_gap(6.0F)
        .set_cross_alignment(widget::LayoutAlignment::stretch)
        .add(vertical_fixed)
        .add(vertical_expanded);

    (void)vertical->measure_layout(scene::LayoutConstraints::tight(foundation::NanSize(80.0F, 140.0F)));
    vertical->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 80.0F, 140.0F));

    REQUIRE(vertical_fixed->position().get_y() == Catch::Approx(0.0F));
    REQUIRE(vertical_fixed->width() == Catch::Approx(80.0F));
    REQUIRE(vertical_expanded->position().get_y() == Catch::Approx(16.0F));
    REQUIRE(vertical_expanded->height() == Catch::Approx(124.0F));
    REQUIRE(vertical_expanded_child->height() == Catch::Approx(124.0F));
}

TEST_CASE("Wrap flows horizontal children into multiple runs", "[widget][layout][wrap]") {
    auto a = std::make_shared<scene::NanControl>(foundation::NanSize(40.0F, 10.0F));
    auto b = std::make_shared<scene::NanControl>(foundation::NanSize(30.0F, 20.0F));
    auto c = std::make_shared<scene::NanControl>(foundation::NanSize(50.0F, 12.0F));
    auto d = std::make_shared<scene::NanControl>(foundation::NanSize(20.0F, 8.0F));

    auto wrap = widget::Wrap::create();
    wrap->set_gap(5.0F).set_run_gap(7.0F).add(a).add(b).add(c).add(d);

    (void)wrap->measure_layout(scene::LayoutConstraints::tight(foundation::NanSize(80.0F, 64.0F)));
    wrap->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 80.0F, 64.0F));

    REQUIRE(a->position().get_x() == Catch::Approx(0.0F));
    REQUIRE(a->position().get_y() == Catch::Approx(0.0F));
    REQUIRE(b->position().get_x() == Catch::Approx(45.0F));
    REQUIRE(b->position().get_y() == Catch::Approx(0.0F));
    REQUIRE(c->position().get_x() == Catch::Approx(0.0F));
    REQUIRE(c->position().get_y() == Catch::Approx(27.0F));
    REQUIRE(d->position().get_x() == Catch::Approx(55.0F));
    REQUIRE(d->position().get_y() == Catch::Approx(27.0F));
}

TEST_CASE("Flow wraps again when assigned bounds shrink", "[widget][layout][wrap]") {
    auto a = std::make_shared<scene::NanControl>(foundation::NanSize(40.0F, 10.0F));
    auto b = std::make_shared<scene::NanControl>(foundation::NanSize(40.0F, 10.0F));
    auto c = std::make_shared<scene::NanControl>(foundation::NanSize(40.0F, 10.0F));

    auto flow = widget::Flow::create();
    flow->set_gap(4.0F).set_run_gap(6.0F).add(a).add(b).add(c);

    (void)flow->measure_layout(scene::LayoutConstraints::tight(foundation::NanSize(132.0F, 40.0F)));
    flow->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 132.0F, 40.0F));
    REQUIRE(c->position().get_y() == Catch::Approx(0.0F));

    flow->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 84.0F, 60.0F));
    REQUIRE(a->position().get_y() == Catch::Approx(0.0F));
    REQUIRE(b->position().get_y() == Catch::Approx(0.0F));
    REQUIRE(c->position().get_x() == Catch::Approx(0.0F));
    REQUIRE(c->position().get_y() == Catch::Approx(16.0F));
}

TEST_CASE("Wrap supports vertical axis and run alignment", "[widget][layout][wrap]") {
    auto a = std::make_shared<scene::NanControl>(foundation::NanSize(20.0F, 35.0F));
    auto b = std::make_shared<scene::NanControl>(foundation::NanSize(30.0F, 20.0F));
    auto c = std::make_shared<scene::NanControl>(foundation::NanSize(18.0F, 30.0F));

    auto wrap = widget::Wrap::create(widget::LayoutAxis::vertical);
    wrap->set_gap(5.0F)
        .set_run_gap(10.0F)
        .set_main_alignment(widget::LayoutAlignment::center)
        .set_cross_alignment(widget::LayoutAlignment::end)
        .set_run_alignment(widget::LayoutAlignment::center)
        .add(a)
        .add(b)
        .add(c);

    (void)wrap->measure_layout(scene::LayoutConstraints::tight(foundation::NanSize(100.0F, 60.0F)));
    wrap->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 100.0F, 60.0F));

    REQUIRE(a->position().get_x() == Catch::Approx(31.0F));
    REQUIRE(a->position().get_y() == Catch::Approx(0.0F));
    REQUIRE(b->position().get_x() == Catch::Approx(21.0F));
    REQUIRE(b->position().get_y() == Catch::Approx(40.0F));
    REQUIRE(c->position().get_x() == Catch::Approx(61.0F));
    REQUIRE(c->position().get_y() == Catch::Approx(15.0F));
}
