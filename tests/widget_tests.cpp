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
#include "widget/text_field.hpp"
#include "widget/primitives/editable_text.hpp"
#include "widget/primitives/text.hpp"
#include "widget/scroll_view.hpp"

#include <memory>
#include <stdexcept>
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
        foundation::NanPoint position;
    };
    struct LineCall {
        foundation::NanPoint start;
        foundation::NanPoint end;
    };
    int line_count = 0;
    int outline_count = 0;
    int rounded_count = 0;
    int clip_clears = 0;
    std::vector<RectCall> rects;
    std::vector<TextCall> texts;
    std::vector<LineCall> lines;
    std::vector<foundation::NanRect> clips;

    void begin_frame() override {}
    void end_frame() override {}
    void set_clip(const foundation::NanRect& rect) override {
        clips.push_back(rect);
    }
    void clear_clip() override {
        ++clip_clears;
    }
    void draw_rect(const foundation::NanRect& r, const foundation::NanColor& c) override {
        rects.push_back({r, c.alpha()});
    }
    void draw_rect_outline(const foundation::NanRect&, float, const foundation::NanColor&) override {
        ++outline_count;
    }
    void draw_rounded_rect(const foundation::NanRect&, float, const foundation::NanColor&) override {
        ++rounded_count;
    }
    void draw_line(const foundation::NanPoint& start, const foundation::NanPoint& end, float,
                   const foundation::NanColor&) override {
        ++line_count;
        lines.push_back({.start = start, .end = end});
    }
    void draw_circle(const foundation::NanPoint&, float, const foundation::NanColor&) override {}
    void draw_text(std::string_view text, const foundation::NanPoint& position, float,
                   const foundation::NanColor& color) override {
        texts.push_back({std::string(text), color.alpha(), position});
    }
};

class FixedTextLayoutBackend final: public widget::primitives::ITextLayoutBackend {
public:
    mutable int calls = 0;

    [[nodiscard]] auto layout(widget::primitives::TextLayoutInput input) const
        -> widget::primitives::TextLayoutResult override {
        ++calls;
        return widget::primitives::TextLayoutResult {
            .size = foundation::NanSize(42.0F, 18.0F),
            .lines = {
                widget::primitives::TextLayoutLine {
                    .text_offset = 0,
                    .text_length = input.text.size(),
                    .visible_text = "backend",
                    .caret_stops = {
                        widget::primitives::TextCaretStop {
                            .source_offset = 0,
                            .x = 0.0F,
                        },
                        widget::primitives::TextCaretStop {
                            .source_offset = std::min<std::size_t>(1, input.text.size()),
                            .x = 5.0F,
                        },
                        widget::primitives::TextCaretStop {
                            .source_offset = input.text.size(),
                            .x = 42.0F,
                            .affinity = widget::primitives::TextAffinity::upstream,
                        },
                    },
                    .size = foundation::NanSize(42.0F, 18.0F),
                    .baseline = 12.0F,
                },
            },
            .font_size = 12.0F,
            .baseline = 12.0F,
            .overflowed = false,
        };
    }
};

class FixedTextLayoutRenderer final: public widget::primitives::ITextLayoutRenderer {
public:
    int draws = 0;

    void draw(
        const widget::primitives::TextLayoutResult&,
        render::DrawContext&,
        foundation::NanPoint,
        foundation::NanColor
    ) override {
        ++draws;
    }
};

class LayoutInvalidationProbe final: public scene::NanControl {
public:
    int layouts = 0;
    bool invalidate_once = false;

protected:
    void on_layout() override {
        ++layouts;
        if (invalidate_once) {
            invalidate_once = false;
            mark_layout_dirty();
        }
    }
};

class PaintMutationProbe final: public scene::NanControl {
public:
    void on_draw(render::DrawContext&) override {
        set_size(foundation::NanSize(80.0F, 40.0F));
        mark_layout_dirty();
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

TEST_CASE("scene text context is inherited without overriding explicit pipelines", "[widget][text][pipeline]") {
    reactive::Graph graph;
    FixedTextLayoutBackend inherited_backend;
    FixedTextLayoutRenderer inherited_renderer;
    FixedTextLayoutBackend explicit_backend;
    FixedTextLayoutRenderer explicit_renderer;
    scene::NanSceneTree tree;
    tree.set_default_text_pipeline({
        .backend = &inherited_backend,
        .renderer = &inherited_renderer,
    });

    auto root = std::make_shared<scene::NanControl>();
    auto label = widget::Label::create(graph, "label");
    auto button = widget::Button::create("button");
    auto field = widget::TextField::create("value", "placeholder");
    auto explicit_text = std::make_shared<widget::primitives::Text>("explicit");
    explicit_text->set_text_pipeline({
        .backend = &explicit_backend,
        .renderer = &explicit_renderer,
    });
    root->add_child(label);
    root->add_child(button);
    root->add_child(field);
    root->add_child(explicit_text);
    tree.set_root(root);

    REQUIRE(label->text_pipeline().backend == &inherited_backend);
    REQUIRE(label->text_pipeline().renderer == &inherited_renderer);
    REQUIRE(button->text_pipeline().backend == &inherited_backend);
    REQUIRE(button->text_pipeline().renderer == &inherited_renderer);
    REQUIRE(field->text_pipeline().backend == &inherited_backend);
    REQUIRE(field->placeholder_text().text_pipeline().renderer == &inherited_renderer);
    REQUIRE(explicit_text->text_pipeline().backend == &explicit_backend);
    REQUIRE(explicit_text->text_pipeline().renderer == &explicit_renderer);

    auto dynamic = widget::Label::create(graph, "dynamic");
    root->add_child(dynamic);
    REQUIRE(dynamic->text_pipeline().backend == &inherited_backend);
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

TEST_CASE("Text setter and property share mutation and dirty behavior", "[widget][text][property]") {
    auto text = std::make_shared<widget::primitives::Text>("initial");
    text->clear_layout_dirty();
    int changes = 0;
    auto subscription = text->text_property().changed().subscribe(
        [&changes](const std::string&) { ++changes; }
    );

    text->set_text("setter");
    REQUIRE(text->text() == "setter");
    REQUIRE(text->layout_dirty());
    REQUIRE(changes == 1);

    text->clear_layout_dirty();
    REQUIRE(text->text_property().set("property"));
    REQUIRE(text->text() == "property");
    REQUIRE(text->layout_dirty());
    REQUIRE(changes == 2);

    REQUIRE_FALSE(text->text_property().set("property"));
    REQUIRE(changes == 2);
}

TEST_CASE("Label binding follows tree lifetime and may be replaced", "[widget][label][property]") {
    reactive::Graph graph;
    reactive::Signal<std::string> first {graph, "first"};
    reactive::Signal<std::string> second {graph, "second"};
    auto label = widget::Label::create(graph);
    label->bind_text(first);

    scene::NanSceneTree tree;
    tree.set_root(label);
    REQUIRE(label->text() == "first");

    first.set("mounted");
    REQUIRE(label->text() == "mounted");

    label->bind_text(second);
    REQUIRE(label->text() == "second");
    first.set("ignored");
    REQUIRE(label->text() == "second");

    tree.set_root(nullptr);
    second.set("while detached");
    REQUIRE(label->text() == "second");
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

TEST_CASE("layout dirtiness crosses non-control ancestors", "[widget][layout][scheduler]") {
    auto root = std::make_shared<scene::NanControl>();
    auto bridge = std::make_shared<scene::NanNode2D>();
    auto leaf = std::make_shared<scene::NanControl>();
    bridge->add_child(leaf);
    root->add_child(bridge);
    root->clear_layout_dirty();
    leaf->clear_layout_dirty();

    leaf->mark_layout_dirty();
    REQUIRE(leaf->layout_dirty());
    REQUIRE(root->layout_dirty());
}

TEST_CASE("layout invalidation raised during layout is preserved", "[widget][layout][scheduler]") {
    auto root = std::make_shared<LayoutInvalidationProbe>();
    root->invalidate_once = true;
    scene::NanSceneTree tree;
    tree.set_root(root);

    REQUIRE(tree.layout_root(foundation::NanSize(120.0F, 80.0F)) == 1);
    REQUIRE(root->layouts == 1);
    REQUIRE(root->layout_dirty());

    REQUIRE(tree.layout_root(foundation::NanSize(120.0F, 80.0F)) == 1);
    REQUIRE(root->layouts == 2);
    REQUIRE_FALSE(root->layout_dirty());
}

TEST_CASE("post-layout action may request one additional layout pass", "[widget][layout][scheduler]") {
    auto root = std::make_shared<LayoutInvalidationProbe>();
    scene::NanSceneTree tree;
    tree.set_root(root);
    tree.post_layout([root] { root->mark_layout_dirty(); });

    REQUIRE(tree.layout_root(foundation::NanSize(120.0F, 80.0F)) == 2);
    REQUIRE(root->layouts == 2);
    REQUIRE_FALSE(root->layout_dirty());
}

TEST_CASE("paint mutation affects layout on the next frame", "[widget][layout][scheduler]") {
    RecordingDevice dev;
    auto root = std::make_shared<PaintMutationProbe>();
    scene::NanSceneTree tree;
    tree.set_root(root);

    REQUIRE(tree.layout_root(foundation::NanSize(120.0F, 80.0F)) == 1);
    REQUIRE_FALSE(root->layout_dirty());
    tree.draw(dev);
    REQUIRE(root->layout_dirty());

    REQUIRE(tree.layout_root(foundation::NanSize(120.0F, 80.0F)) == 1);
    REQUIRE_FALSE(root->layout_dirty());
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
    REQUIRE(text->layout_result().lines.front().text_length == text->text().size());
    REQUIRE(text->layout_result().lines.front().visible_text == text->text());
    REQUIRE(text->layout_result().lines.front().size.get_width() > text->width());
    REQUIRE(dev.texts.size() == 1);
    REQUIRE(dev.texts.front().text == text->text());
    REQUIRE(dev.texts[0].alpha == Catch::Approx(0.5F));
    REQUIRE(dev.clips.size() == 1);
    REQUIRE(dev.clips.front().get_width() == Catch::Approx(40.0F));
    REQUIRE(dev.clip_clears == 1);
}

TEST_CASE("Text clip intersects and restores an ancestor clip", "[widget][text][clip]") {
    RecordingDevice dev;
    render::DrawContext context(dev);
    widget::primitives::Text text("abcdef");
    text.set_style(widget::primitives::TextStyle {
        .font_size = 20.0F,
        .overflow = widget::primitives::TextOverflow::clip,
        .max_lines = 1,
    });
    (void)text.measure_layout(scene::LayoutConstraints {
        .min_width = 0.0F,
        .max_width = 40.0F,
        .min_height = 0.0F,
        .max_height = 80.0F,
    });

    {
        auto ancestor = context.clip().push(
            foundation::NanRect::from_xywh(10.0F, 10.0F, 25.0F, 30.0F)
        );
        text.draw_at(context, foundation::NanPoint(20.0F, 5.0F));

        REQUIRE(context.clip().depth() == 1);
        REQUIRE(dev.clips.size() == 3);
        REQUIRE(dev.clips[1].get_left() == Catch::Approx(20.0F));
        REQUIRE(dev.clips[1].get_top() == Catch::Approx(10.0F));
        REQUIRE(dev.clips[1].get_width() == Catch::Approx(15.0F));
        REQUIRE(dev.clips[2].get_left() == Catch::Approx(10.0F));
        REQUIRE(dev.clips[2].get_width() == Catch::Approx(25.0F));
    }

    REQUIRE(context.clip().depth() == 0);
    REQUIRE(dev.clip_clears == 1);
}

TEST_CASE("Text layout backend controls measurement and draw output", "[widget][text][backend]") {
    FixedTextLayoutBackend backend;
    auto text = std::make_shared<widget::primitives::Text>("source", backend);
    RecordingDevice dev;
    scene::NanSceneTree tree;
    tree.set_root(text);

    (void)text->measure_layout(scene::LayoutConstraints::loose());
    tree.draw(dev);

    REQUIRE(&text->layout_backend() == &backend);
    REQUIRE(backend.calls >= 2);
    REQUIRE(text->width() == Catch::Approx(42.0F));
    REQUIRE(text->height() == Catch::Approx(18.0F));
    REQUIRE(text->layout_result().lines.front().visible_text == "backend");
    REQUIRE(dev.texts.size() == 1);
    REQUIRE(dev.texts.front().text == "backend");
    REQUIRE(dev.texts.front().position.get_y() == Catch::Approx(0.0F));
}

TEST_CASE("Text consumers forward one shared text pipeline", "[widget][text][pipeline]") {
    FixedTextLayoutBackend backend;
    FixedTextLayoutRenderer renderer;
    const widget::primitives::TextPipeline pipeline {
        .backend = &backend,
        .renderer = &renderer,
    };

    reactive::Graph graph;
    auto label = widget::Label::create(graph, "label");
    auto button = widget::Button::create("button");
    widget::primitives::EditableText editable("value");
    widget::TextField field("", "placeholder");

    label->set_text_pipeline(pipeline);
    button->set_text_pipeline(pipeline);
    editable.set_text_pipeline(pipeline);
    field.set_text_pipeline(pipeline);

    REQUIRE(label->text_pipeline().backend == &backend);
    REQUIRE(label->text_pipeline().renderer == &renderer);
    REQUIRE(button->text_pipeline().backend == &backend);
    REQUIRE(button->text_node().text_pipeline().renderer == &renderer);
    REQUIRE(editable.text_pipeline().backend == &backend);
    REQUIRE(editable.text_node().text_pipeline().renderer == &renderer);
    REQUIRE(field.text_pipeline().backend == &backend);
    REQUIRE(field.editable_text().text_node().text_pipeline().renderer == &renderer);
    REQUIRE(field.placeholder_text().text_pipeline().backend == &backend);
    REQUIRE(field.placeholder_text().text_pipeline().renderer == &renderer);

    REQUIRE_THROWS_AS(
        label->set_text_pipeline(widget::primitives::TextPipeline {.backend = nullptr}),
        std::invalid_argument
    );
}

TEST_CASE("Button draws through its forwarded text renderer", "[widget][button][text][pipeline]") {
    FixedTextLayoutBackend backend;
    FixedTextLayoutRenderer renderer;
    auto button = widget::Button::create("button");
    button->set_text_pipeline({.backend = &backend, .renderer = &renderer});

    RecordingDevice dev;
    scene::NanSceneTree tree;
    tree.set_root(button);
    tree.draw(dev);

    REQUIRE(renderer.draws == 1);
    REQUIRE(dev.texts.empty());
}

TEST_CASE("TextField draws placeholder and value through one text pipeline", "[widget][text-field][pipeline]") {
    FixedTextLayoutBackend backend;
    FixedTextLayoutRenderer renderer;
    auto field = std::make_shared<widget::TextField>("", "placeholder");
    field->set_text_pipeline({.backend = &backend, .renderer = &renderer});

    RecordingDevice dev;
    scene::NanSceneTree tree;
    tree.set_root(field);
    tree.draw(dev);
    REQUIRE(renderer.draws == 1);
    REQUIRE(dev.texts.empty());

    field->set_value("value");
    tree.draw(dev);
    REQUIRE(renderer.draws == 2);
    REQUIRE(dev.texts.empty());
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

TEST_CASE("Text overflow preserves UTF-8 codepoint boundaries", "[widget][text][utf8]") {
    auto text = std::make_shared<widget::primitives::Text>("中文测试字");
    text->set_style(widget::primitives::TextStyle {
        .color = opaque_color(0.8F),
        .font_size = 10.0F,
        .overflow = widget::primitives::TextOverflow::ellipsis,
        .max_lines = 1,
    });

    (void)text->measure_layout(scene::LayoutConstraints {
        .min_width = 0.0F,
        .max_width = 23.0F,
        .min_height = 0.0F,
        .max_height = 80.0F,
    });

    const auto& line = text->layout_result().lines.front();
    REQUIRE(text->layout_result().overflowed);
    REQUIRE(line.text_length == std::string("中").size());
    REQUIRE(line.visible_text == "中...");
}

TEST_CASE("deterministic caret geometry follows grapheme boundaries", "[widget][text][caret]") {
    constexpr std::string_view source = "a\xCC\x81" "b";
    widget::primitives::Text text {std::string(source)};
    text.set_style(widget::primitives::TextStyle {
        .font_size = 10.0F,
        .overflow = widget::primitives::TextOverflow::clip,
        .max_lines = 1,
    });
    (void)text.measure_layout(scene::LayoutConstraints {
        .min_width = 0.0F,
        .max_width = 6.0F,
        .min_height = 0.0F,
        .max_height = 40.0F,
    });

    const auto& layout = text.layout_result();
    const auto& line = layout.lines.front();
    REQUIRE(line.caret_stops.size() == 3);
    REQUIRE(line.caret_stops[0].source_offset == 0);
    REQUIRE(line.caret_stops[1].source_offset == 3);
    REQUIRE(line.caret_stops[2].source_offset == source.size());
    REQUIRE(line.caret_for_source(1).source_offset == 0);
    REQUIRE(line.caret_for_source(3).x == Catch::Approx(5.6F));
    REQUIRE(line.caret_stops.back().x > layout.size.get_width());
    REQUIRE(line.caret_for_x(-100.0F).source_offset == 0);
    REQUIRE(line.caret_for_x(100.0F).source_offset == source.size());
    REQUIRE(layout.caret_for_point(foundation::NanPoint(5.7F, 0.0F)).source_offset == 3);
}

TEST_CASE("empty text layout exposes one caret stop", "[widget][text][caret]") {
    widget::primitives::Text text;
    const auto& line = text.layout_result().lines.front();
    REQUIRE(line.caret_stops.size() == 1);
    REQUIRE(line.caret_stops.front().source_offset == 0);
    REQUIRE(line.caret_stops.front().x == Catch::Approx(0.0F));
}

TEST_CASE("Text wrap produces and draws actual UTF-8 lines", "[widget][text][wrap]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto text = std::make_shared<widget::primitives::Text>("A中文B测试");
    text->set_style(widget::primitives::TextStyle {
        .color = opaque_color(0.8F),
        .font_size = 10.0F,
        .overflow = widget::primitives::TextOverflow::wrap,
        .max_lines = 3,
    });

    (void)text->measure_layout(scene::LayoutConstraints {
        .min_width = 0.0F,
        .max_width = 17.0F,
        .min_height = 0.0F,
        .max_height = 80.0F,
    });
    tree.set_root(text);
    tree.draw(dev);

    const auto& layout = text->layout_result();
    REQUIRE(layout.lines.size() == 2);
    REQUIRE_FALSE(layout.overflowed);
    REQUIRE(layout.lines[0].visible_text == "A中文");
    REQUIRE(layout.lines[0].text_offset == 0);
    REQUIRE(layout.lines[0].text_length == std::string("A中文").size());
    REQUIRE(layout.lines[1].visible_text == "B测试");
    REQUIRE(layout.lines[1].text_offset == std::string("A中文").size());
    REQUIRE(layout.size.get_height() == Catch::Approx(24.0F));
    REQUIRE(dev.texts.size() == 2);
    REQUIRE(dev.texts[0].text == "A中文");
    REQUIRE(dev.texts[1].text == "B测试");
    REQUIRE(dev.texts[1].position.get_y() - dev.texts[0].position.get_y() == Catch::Approx(12.0F));
}

TEST_CASE("Text wrap honors explicit newlines and max lines", "[widget][text][wrap]") {
    auto text = std::make_shared<widget::primitives::Text>("first\nsecond\nthird");
    text->set_style(widget::primitives::TextStyle {
        .color = opaque_color(0.8F),
        .font_size = 10.0F,
        .overflow = widget::primitives::TextOverflow::wrap,
        .max_lines = 2,
    });

    (void)text->measure_layout(scene::LayoutConstraints::loose());

    const auto& layout = text->layout_result();
    REQUIRE(layout.lines.size() == 2);
    REQUIRE(layout.overflowed);
    REQUIRE(layout.lines[0].visible_text == "first");
    REQUIRE(layout.lines[1].visible_text == "second");
    REQUIRE(layout.lines[1].text_offset == std::string("first\n").size());
    REQUIRE(layout.size.get_height() == Catch::Approx(24.0F));
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

TEST_CASE("EditableText caret and backspace preserve UTF-8 boundaries", "[widget][editable-text][utf8]") {
    auto edit = std::make_shared<widget::primitives::EditableText>("A中🙂");
    scene::NanSceneTree tree;
    tree.set_root(edit);
    tree.set_focus(edit.get());

    REQUIRE(edit->caret() == std::string("A中🙂").size());
    edit->set_caret(2);
    REQUIRE(edit->caret() == 1);
    edit->set_caret(edit->value().size());

    tree.dispatch_key(scene::KeyEvent(259, scene::KeyEvent::Action::press));
    REQUIRE(edit->value() == "A中");
    REQUIRE(edit->caret() == std::string("A中").size());

    tree.dispatch_key(scene::KeyEvent(259, scene::KeyEvent::Action::press));
    REQUIRE(edit->value() == "A");
    REQUIRE(edit->caret() == 1);
}

TEST_CASE("EditableText deletes complete grapheme clusters", "[widget][editable-text][grapheme]") {
    constexpr std::string_view combined = "a\xCC\x81" "b";
    auto edit = std::make_shared<widget::primitives::EditableText>(std::string(combined));
    scene::NanSceneTree tree;
    tree.set_root(edit);
    tree.set_focus(edit.get());

    edit->set_caret(2);
    REQUIRE(edit->caret() == 0);
    edit->set_caret(edit->value().size());
    tree.dispatch_key(scene::KeyEvent(259, scene::KeyEvent::Action::press));
    REQUIRE(edit->value() == "a\xCC\x81");
    REQUIRE(edit->caret() == 3);

    tree.dispatch_key(scene::KeyEvent(259, scene::KeyEvent::Action::press));
    REQUIRE(edit->value().empty());
    REQUIRE(edit->caret() == 0);

    edit->set_value(std::string(combined));
    edit->set_caret(0);
    tree.dispatch_key(scene::KeyEvent(261, scene::KeyEvent::Action::press));
    REQUIRE(edit->value() == "b");
    REQUIRE(edit->caret() == 0);
}

TEST_CASE("EditableText moves through visual caret stops", "[widget][editable-text][caret]") {
    auto edit = std::make_shared<widget::primitives::EditableText>("abc");
    scene::NanSceneTree tree;
    tree.set_root(edit);
    tree.set_focus(edit.get());

    tree.dispatch_key(scene::KeyEvent(263, scene::KeyEvent::Action::press));
    REQUIRE(edit->caret() == 2);
    tree.dispatch_key(scene::KeyEvent(263, scene::KeyEvent::Action::press));
    REQUIRE(edit->caret() == 1);
    tree.dispatch_key(scene::KeyEvent(262, scene::KeyEvent::Action::press));
    REQUIRE(edit->caret() == 2);
    tree.dispatch_key(scene::KeyEvent(268, scene::KeyEvent::Action::press));
    REQUIRE(edit->caret() == 0);
    tree.dispatch_key(scene::KeyEvent(269, scene::KeyEvent::Action::press));
    REQUIRE(edit->caret() == 3);
}

TEST_CASE("EditableText draws caret from layout geometry", "[widget][editable-text][caret]") {
    FixedTextLayoutBackend backend;
    auto edit = std::make_shared<widget::primitives::EditableText>("abc");
    edit->set_text_pipeline({.backend = &backend});
    edit->set_caret(1);

    RecordingDevice dev;
    scene::NanSceneTree tree;
    tree.set_root(edit);
    tree.set_focus(edit.get());
    tree.draw(dev);

    REQUIRE(dev.lines.size() == 1);
    REQUIRE(dev.lines.front().start.get_x() == Catch::Approx(5.0F));
    REQUIRE(dev.lines.front().end.get_x() == Catch::Approx(5.0F));
}

TEST_CASE("EditableText selection replaces and deletes once", "[widget][editable-text][selection]") {
    auto edit = std::make_shared<widget::primitives::EditableText>("abc");
    std::vector<std::string> changes;
    edit->set_on_change([&](std::string_view value) { changes.emplace_back(value); });
    scene::NanSceneTree tree;
    tree.set_root(edit);
    tree.set_focus(edit.get());

    scene::KeyModifiers shift {.shift = true};
    tree.dispatch_key(scene::KeyEvent(263, scene::KeyEvent::Action::press, shift));
    REQUIRE(edit->has_selection());
    REQUIRE(edit->selection().anchor == 3);
    REQUIRE(edit->selection().focus == 2);
    tree.dispatch_text_input(scene::TextInputEvent("X"));
    REQUIRE(edit->value() == "abX");
    REQUIRE(changes.size() == 1);

    scene::KeyModifiers ctrl {.ctrl = true};
    tree.dispatch_key(scene::KeyEvent(65, scene::KeyEvent::Action::press, ctrl));
    REQUIRE(edit->selection().anchor == 0);
    REQUIRE(edit->selection().focus == 3);
    tree.dispatch_key(scene::KeyEvent(261, scene::KeyEvent::Action::press));
    REQUIRE(edit->value().empty());
    REQUIRE(changes.size() == 2);
}

TEST_CASE("EditableText read-only and composition state preserve value", "[widget][editable-text][state]") {
    auto edit = std::make_shared<widget::primitives::EditableText>("value");
    edit->set_read_only(true);
    edit->set_composition(widget::primitives::TextComposition {
        .text = "preedit",
        .selection_start = 2,
        .selection_end = 4,
    });
    scene::NanSceneTree tree;
    tree.set_root(edit);
    tree.set_focus(edit.get());

    tree.dispatch_text_input(scene::TextInputEvent("x"));
    tree.dispatch_key(scene::KeyEvent(259, scene::KeyEvent::Action::press));
    REQUIRE(edit->value() == "value");
    REQUIRE(edit->composition().has_value());
    REQUIRE(edit->composition()->selection_end == 4);
    edit->clear_composition();
    REQUIRE_FALSE(edit->composition().has_value());
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

TEST_CASE("TextField draws placeholder through semantic shell", "[widget][text-field]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto field = std::make_shared<widget::TextField>("", "Name");
    tree.set_root(field);

    tree.draw(dev);

    REQUIRE(field->value().empty());
    REQUIRE(field->placeholder() == "Name");
    REQUIRE(dev.rounded_count == 1);
    REQUIRE(dev.outline_count == 1);
    REQUIRE(dev.texts.size() == 1);
    REQUIRE(dev.texts.front().text == "Name");
}

TEST_CASE("TextField forwards text editing and change callback", "[widget][text-field]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto field = std::make_shared<widget::TextField>("", "Name");
    std::vector<std::string> changes;
    field->set_on_change([&](std::string_view value) { changes.emplace_back(value); });
    tree.set_root(field);
    tree.set_focus(field.get());

    tree.dispatch_text_input(scene::TextInputEvent("A"));
    REQUIRE(field->value() == "A");
    REQUIRE(changes.size() == 1);
    REQUIRE(changes.back() == "A");

    tree.draw(dev);
    REQUIRE(dev.texts.size() == 1);
    REQUIRE(dev.texts.front().text == "A");
    REQUIRE(dev.line_count == 1);
}

TEST_CASE("TextField pointer selection uses capture outside bounds", "[widget][text-field][selection]") {
    auto field = std::make_shared<widget::TextField>("abcdef", "");
    field->layout_to(foundation::NanRect::from_xywh(20.0F, 20.0F, 100.0F, 40.0F));
    scene::NanSceneTree tree;
    tree.set_root(field);

    tree.dispatch_mouse_button(scene::MouseButtonEvent(
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        foundation::NanPoint(34.0F, 40.0F)
    ));
    REQUIRE(tree.pointer_capture() == field.get());
    tree.dispatch_mouse_move(scene::MouseMoveEvent(
        foundation::NanPoint(200.0F, 40.0F), foundation::NanPoint(166.0F, 0.0F)
    ));
    REQUIRE(field->editable_text().has_selection());
    REQUIRE(field->editable_text().selection().focus == field->value().size());
    tree.dispatch_mouse_button(scene::MouseButtonEvent(
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::release,
        foundation::NanPoint(200.0F, 40.0F)
    ));
    REQUIRE(tree.pointer_capture() == nullptr);
}

TEST_CASE("TextField exposes semantic states and submit", "[widget][text-field][state]") {
    auto field = std::make_shared<widget::TextField>("value", "");
    std::vector<std::string> submissions;
    field->set_on_submit([&](std::string_view value) { submissions.emplace_back(value); });
    field->set_read_only(true);
    field->set_invalid(true);
    REQUIRE(field->read_only());
    REQUIRE(field->invalid());

    scene::NanSceneTree tree;
    tree.set_root(field);
    tree.set_focus(field.get());
    tree.dispatch_key(scene::KeyEvent(257, scene::KeyEvent::Action::press));
    REQUIRE(submissions == std::vector<std::string> {"value"});

    field->set_disabled(true);
    REQUIRE(field->disabled());
    REQUIRE_FALSE(field->is_focusable());
    REQUIRE(tree.focused_node() == nullptr);
    tree.dispatch_text_input(scene::TextInputEvent("x"));
    REQUIRE(field->value() == "value");

    RecordingDevice dev;
    tree.draw(dev);
    REQUIRE(dev.line_count == 0);
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

TEST_CASE("Row and Column distribute main-axis space between children", "[widget][layout][alignment]") {
    auto a = std::make_shared<scene::NanControl>(foundation::NanSize(20.0F, 10.0F));
    auto b = std::make_shared<scene::NanControl>(foundation::NanSize(30.0F, 10.0F));
    auto c = std::make_shared<scene::NanControl>(foundation::NanSize(10.0F, 10.0F));

    auto row = widget::Row::create();
    row->set_gap(5.0F)
        .set_main_alignment(widget::LayoutAlignment::space_between)
        .add(a)
        .add(b)
        .add(c);

    (void)row->measure_layout(scene::LayoutConstraints::tight(foundation::NanSize(120.0F, 20.0F)));
    row->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 120.0F, 20.0F));

    REQUIRE(a->position().get_x() == Catch::Approx(0.0F));
    REQUIRE(b->position().get_x() == Catch::Approx(50.0F));
    REQUIRE(c->position().get_x() == Catch::Approx(110.0F));

    auto top = std::make_shared<scene::NanControl>(foundation::NanSize(10.0F, 20.0F));
    auto bottom = std::make_shared<scene::NanControl>(foundation::NanSize(10.0F, 30.0F));
    auto column = widget::Column::create();
    column->set_gap(5.0F)
        .set_main_alignment(widget::LayoutAlignment::space_between)
        .add(top)
        .add(bottom);

    (void)column->measure_layout(scene::LayoutConstraints::tight(foundation::NanSize(40.0F, 100.0F)));
    column->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 40.0F, 100.0F));

    REQUIRE(top->position().get_y() == Catch::Approx(0.0F));
    REQUIRE(bottom->position().get_y() == Catch::Approx(70.0F));
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

TEST_CASE("Flex distributes main-axis space between fixed children", "[widget][layout][flex]") {
    auto a = std::make_shared<scene::NanControl>(foundation::NanSize(20.0F, 10.0F));
    auto b = std::make_shared<scene::NanControl>(foundation::NanSize(20.0F, 10.0F));
    auto c = std::make_shared<scene::NanControl>(foundation::NanSize(20.0F, 10.0F));

    auto flex = widget::Flex::create(widget::LayoutAxis::horizontal);
    flex->set_gap(4.0F)
        .set_main_alignment(widget::LayoutAlignment::space_between)
        .add(a)
        .add(b)
        .add(c);

    (void)flex->measure_layout(scene::LayoutConstraints::tight(foundation::NanSize(120.0F, 20.0F)));
    flex->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 120.0F, 20.0F));

    REQUIRE(a->position().get_x() == Catch::Approx(0.0F));
    REQUIRE(b->position().get_x() == Catch::Approx(50.0F));
    REQUIRE(c->position().get_x() == Catch::Approx(100.0F));
}

TEST_CASE("FlexItem grows and redistributes at max limits", "[widget][layout][flex]") {
    auto a_child = std::make_shared<scene::NanControl>(foundation::NanSize(10.0F, 10.0F));
    auto b_child = std::make_shared<scene::NanControl>(foundation::NanSize(10.0F, 10.0F));
    auto a = widget::FlexItem::create(scene::LayoutFlexPolicy {
        .basis = 20.0F,
        .grow = 1.0F,
        .limits = {.max_width = 30.0F},
    });
    auto b = widget::FlexItem::create(scene::LayoutFlexPolicy {.basis = 20.0F, .grow = 1.0F});
    a->set_child(a_child);
    b->set_child(b_child);
    auto row = widget::Row::create();
    row->add(a).add(b);

    (void)row->measure_layout(scene::LayoutConstraints::tight(foundation::NanSize(90.0F, 20.0F)));
    row->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 90.0F, 20.0F));

    REQUIRE(a->width() == Catch::Approx(30.0F));
    REQUIRE(b->width() == Catch::Approx(60.0F));
    REQUIRE(a_child->size() == a->size());
    REQUIRE(b_child->size() == b->size());
}

TEST_CASE("FlexItem shrinks by scaled basis and respects minima", "[widget][layout][flex]") {
    auto a = widget::FlexItem::create(scene::LayoutFlexPolicy {
        .basis = 100.0F,
        .shrink = 1.0F,
        .limits = {.min_width = 70.0F},
    });
    auto b = widget::FlexItem::create(scene::LayoutFlexPolicy {
        .basis = 100.0F,
        .shrink = 1.0F,
        .limits = {.min_width = 20.0F},
    });
    a->set_child(std::make_shared<scene::NanControl>(foundation::NanSize(100.0F, 10.0F)));
    b->set_child(std::make_shared<scene::NanControl>(foundation::NanSize(100.0F, 10.0F)));
    auto row = widget::Row::create();
    row->add(a).add(b);

    (void)row->measure_layout(scene::LayoutConstraints::tight(foundation::NanSize(120.0F, 20.0F)));
    row->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 120.0F, 20.0F));

    REQUIRE(a->width() == Catch::Approx(70.0F));
    REQUIRE(b->width() == Catch::Approx(50.0F));
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

TEST_CASE("Wrap distributes space independently inside each run", "[widget][layout][wrap]") {
    auto a = std::make_shared<scene::NanControl>(foundation::NanSize(40.0F, 10.0F));
    auto b = std::make_shared<scene::NanControl>(foundation::NanSize(30.0F, 20.0F));
    auto c = std::make_shared<scene::NanControl>(foundation::NanSize(50.0F, 12.0F));
    auto d = std::make_shared<scene::NanControl>(foundation::NanSize(20.0F, 8.0F));

    auto wrap = widget::Wrap::create();
    wrap->set_gap(5.0F)
        .set_run_gap(7.0F)
        .set_main_alignment(widget::LayoutAlignment::space_between)
        .add(a)
        .add(b)
        .add(c)
        .add(d);

    (void)wrap->measure_layout(scene::LayoutConstraints::tight(foundation::NanSize(80.0F, 64.0F)));
    wrap->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 80.0F, 64.0F));

    REQUIRE(a->position().get_x() == Catch::Approx(0.0F));
    REQUIRE(b->position().get_x() == Catch::Approx(50.0F));
    REQUIRE(c->position().get_x() == Catch::Approx(0.0F));
    REQUIRE(d->position().get_x() == Catch::Approx(60.0F));
    REQUIRE(c->position().get_y() == Catch::Approx(27.0F));
}

TEST_CASE("Wrap distributes spare cross-axis space between runs", "[widget][layout][wrap]") {
    auto a = std::make_shared<scene::NanControl>(foundation::NanSize(40.0F, 10.0F));
    auto b = std::make_shared<scene::NanControl>(foundation::NanSize(40.0F, 20.0F));

    auto wrap = widget::Wrap::create();
    wrap->set_run_gap(5.0F)
        .set_run_alignment(widget::LayoutAlignment::space_between)
        .add(a)
        .add(b);

    (void)wrap->measure_layout(scene::LayoutConstraints::tight(foundation::NanSize(40.0F, 80.0F)));
    wrap->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 40.0F, 80.0F));

    REQUIRE(a->position().get_y() == Catch::Approx(0.0F));
    REQUIRE(b->position().get_y() == Catch::Approx(60.0F));
}

TEST_CASE("Wrap stretches runs and honors child cross alignment", "[widget][layout][wrap]") {
    auto a = std::make_shared<scene::NanControl>(foundation::NanSize(40.0F, 10.0F));
    auto b = std::make_shared<scene::NanControl>(foundation::NanSize(40.0F, 20.0F));
    auto wrap = widget::Wrap::create();
    wrap->set_run_gap(5.0F)
        .set_run_alignment(widget::LayoutAlignment::stretch)
        .add(a, widget::LayoutAlignment::end)
        .add(b, widget::LayoutAlignment::stretch);

    (void)wrap->measure_layout(scene::LayoutConstraints::tight(foundation::NanSize(40.0F, 80.0F)));
    wrap->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 40.0F, 80.0F));

    REQUIRE(a->position().get_y() == Catch::Approx(22.5F));
    REQUIRE(a->height() == Catch::Approx(10.0F));
    REQUIRE(b->position().get_y() == Catch::Approx(37.5F));
    REQUIRE(b->height() == Catch::Approx(42.5F));
}

TEST_CASE("ScrollView clamps offsets and translates content", "[widget][scroll]") {
    auto content = std::make_shared<scene::NanControl>(foundation::NanSize(50.0F, 200.0F));
    auto scroll = widget::ScrollView::create(widget::ScrollAxis::vertical);
    scroll->set_child(content);
    (void)scroll->measure_layout(scene::LayoutConstraints::tight(foundation::NanSize(100.0F, 80.0F)));
    scroll->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 100.0F, 80.0F));

    REQUIRE(scroll->overflow() == scene::ControlOverflow::clip);
    REQUIRE(scroll->content_size() == foundation::NanSize(100.0F, 200.0F));
    REQUIRE(scroll->maximum_scroll_offset() == foundation::NanPoint(0.0F, 120.0F));
    scroll->set_scroll_offset(foundation::NanPoint(20.0F, 500.0F));
    REQUIRE(scroll->scroll_offset() == foundation::NanPoint(0.0F, 120.0F));
    REQUIRE(content->position() == foundation::NanPoint(0.0F, -120.0F));
}

TEST_CASE("ScrollView replaces content safely during process", "[widget][scroll][scheduler]") {
    auto scroll = widget::ScrollView::create(widget::ScrollAxis::vertical);
    auto initial = std::make_shared<scene::NanControl>(foundation::NanSize(100.0F, 100.0F));
    auto first = std::make_shared<scene::NanControl>(foundation::NanSize(100.0F, 200.0F));
    auto final = std::make_shared<scene::NanControl>(foundation::NanSize(100.0F, 300.0F));
    scroll->set_child(initial);
    scene::NanSceneTree tree;
    tree.set_root(scroll);

    {
        auto phase = tree.enter_phase(scene::FramePhase::process);
        REQUIRE_NOTHROW(scroll->set_child(first));
        REQUIRE_NOTHROW(scroll->set_child(final));
        REQUIRE(scroll->child() == final.get());
        REQUIRE(scroll->child_count() == 1);
    }

    tree.flush_tree_mutations();
    REQUIRE(scroll->child() == final.get());
    REQUIRE(scroll->child_count() == 1);
    REQUIRE(scroll->get_child(0) == final.get());
    REQUIRE_FALSE(initial->is_inside_tree());
    REQUIRE_FALSE(first->is_inside_tree());
    REQUIRE(final->is_inside_tree());
}

TEST_CASE("ScrollView consumes wheel only when offset changes", "[widget][scroll][input]") {
    auto content = std::make_shared<scene::NanControl>(foundation::NanSize(100.0F, 200.0F));
    auto scroll = widget::ScrollView::create();
    scroll->set_child(content).set_wheel_step(25.0F);
    (void)scroll->measure_layout(scene::LayoutConstraints::tight(foundation::NanSize(100.0F, 80.0F)));
    scroll->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 100.0F, 80.0F));

    scene::MouseWheelEvent down(
        foundation::NanPoint(10.0F, 10.0F), foundation::NanPoint(0.0F, -1.0F)
    );
    REQUIRE(scroll->on_input(down));
    REQUIRE(down.is_accepted());
    REQUIRE(scroll->scroll_offset().get_y() == Catch::Approx(25.0F));

    scroll->set_scroll_offset(scroll->maximum_scroll_offset());
    scene::MouseWheelEvent at_end(
        foundation::NanPoint(10.0F, 10.0F), foundation::NanPoint(0.0F, -1.0F)
    );
    REQUIRE_FALSE(scroll->on_input(at_end));
    REQUIRE_FALSE(at_end.is_accepted());
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
