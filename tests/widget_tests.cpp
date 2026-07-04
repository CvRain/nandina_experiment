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

#include <memory>
#include <string>
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
    std::vector<RectCall> rects;

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
                   const foundation::NanColor&) override {}
    void draw_circle(const foundation::NanPoint&, float, const foundation::NanColor&) override {}
    void draw_text(std::string_view, const foundation::NanPoint&, float,
                   const foundation::NanColor&) override {}
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
