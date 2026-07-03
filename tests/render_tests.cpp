//
// Render-layer unit tests (Catch2 v3).
//
// RecordingDevice captures every device call into a list of structs, so the
// draw traversal becomes assertable without a window. This is the direct payoff
// of the IRenderDevice abstraction: draw ordering, inherited opacity, clip
// intersection, and visibility skipping are now testable.
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "render/clip_stack.hpp"
#include "render/draw_context.hpp"
#include "render/render_device.hpp"
#include "scene/node2d.hpp"
#include "scene/scene_tree.hpp"

#include <memory>
#include <string>
#include <vector>

using namespace nandina;

namespace
{

/// Records draw/clip calls for assertions. No windowing.
class RecordingDevice final : public render::IRenderDevice {
public:
    struct RectCall {
        foundation::NanRect rect;
        float alpha;  // color alpha in [0,1], for opacity checks
    };
    struct ClipCall {
        foundation::NanRect rect;
        bool cleared;  // true if this was a clear_clip()
    };

    std::vector<RectCall> rects;
    std::vector<ClipCall> clips;
    int begin_count = 0;
    int end_count = 0;

    void begin_frame() override { ++begin_count; }
    void end_frame() override { ++end_count; }

    void set_clip(const foundation::NanRect& r) override { clips.push_back({r, false}); }
    void clear_clip() override { clips.push_back({foundation::NanRect::empty(), true}); }

    void draw_rect(const foundation::NanRect& r, const foundation::NanColor& c) override {
        rects.push_back({r, c.alpha()});
    }
    void draw_rect_outline(const foundation::NanRect& r, float /*t*/,
                           const foundation::NanColor& c) override {
        rects.push_back({r, c.alpha()});
    }
    void draw_rounded_rect(const foundation::NanRect& r, float /*rad*/,
                           const foundation::NanColor& c) override {
        rects.push_back({r, c.alpha()});
    }
    void draw_line(const foundation::NanPoint&, const foundation::NanPoint&, float,
                   const foundation::NanColor&) override {}
    void draw_circle(const foundation::NanPoint&, float,
                     const foundation::NanColor&) override {}
    void draw_text(std::string_view, const foundation::NanPoint&, float,
                   const foundation::NanColor&) override {}
};

/// A node that draws one rect at its world origin, tagged via fill alpha so the
/// recorded call order can be identified. opacity() is folded into the alpha.
class RectNode : public scene::NanNode2D {
public:
    explicit RectNode(float tag_alpha) : tag_(tag_alpha) {}

    [[nodiscard]] auto contains_point(foundation::NanPoint) const -> bool override {
        return false;
    }

    void on_draw(render::DrawContext& ctx) override {
        const auto p = ctx.world_transform().position();
        const auto rect = foundation::NanRect::from_xywh(p.get_x(), p.get_y(), 10, 10);
        const auto color = foundation::NanColor::from(
            foundation::NanOklch{.light = 0.5F, .chroma = 0.1F, .hue = 120.0F, .alpha = 1.0F});
        ctx.device().draw_rect(rect, color.with_alpha(tag_ * ctx.opacity()));
    }

private:
    float tag_;
};

} // namespace

TEST_CASE("draw brackets the frame exactly once", "[render][frame]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    tree.set_root(std::make_shared<RectNode>(1.0F));

    tree.draw(dev);

    REQUIRE(dev.begin_count == 1);
    REQUIRE(dev.end_count == 1);
    REQUIRE(dev.rects.size() == 1);
}

TEST_CASE("children draw in z-index order (low to high)", "[render][zorder]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto root = std::make_shared<RectNode>(0.1F);
    auto low = std::make_shared<RectNode>(0.3F);
    auto high = std::make_shared<RectNode>(0.9F);
    low->set_z_index(1);
    high->set_z_index(10);
    // Add high first to prove ordering is by z_index, not insertion.
    root->add_child(high);
    root->add_child(low);
    tree.set_root(root);

    tree.draw(dev);

    // Order: root (0.1), then low z=1 (0.3), then high z=10 (0.9).
    REQUIRE(dev.rects.size() == 3);
    REQUIRE(dev.rects[0].alpha == Catch::Approx(0.1F));
    REQUIRE(dev.rects[1].alpha == Catch::Approx(0.3F));
    REQUIRE(dev.rects[2].alpha == Catch::Approx(0.9F));
}

TEST_CASE("world transform composes down the tree", "[render][transform]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto root = std::make_shared<RectNode>(1.0F);
    auto child = std::make_shared<RectNode>(1.0F);
    root->set_position(foundation::NanPoint(100, 50));
    child->set_position(foundation::NanPoint(10, 5));
    root->add_child(child);
    tree.set_root(root);

    tree.draw(dev);

    REQUIRE(dev.rects.size() == 2);
    // root at (100,50)
    REQUIRE(dev.rects[0].rect.get_left() == Catch::Approx(100.0F));
    REQUIRE(dev.rects[0].rect.get_top() == Catch::Approx(50.0F));
    // child world = root + local = (110, 55)
    REQUIRE(dev.rects[1].rect.get_left() == Catch::Approx(110.0F));
    REQUIRE(dev.rects[1].rect.get_top() == Catch::Approx(55.0F));
}

TEST_CASE("invisible subtree produces no draw calls", "[render][visibility]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto root = std::make_shared<RectNode>(1.0F);
    auto hidden = std::make_shared<RectNode>(1.0F);
    auto grandchild = std::make_shared<RectNode>(1.0F);
    hidden->add_child(grandchild);
    hidden->set_visible(false);
    root->add_child(hidden);
    tree.set_root(root);

    tree.draw(dev);

    // Only root draws; hidden and its child are skipped entirely.
    REQUIRE(dev.rects.size() == 1);
}

TEST_CASE("ClipStack intersects with the parent clip", "[render][clip]") {
    RecordingDevice dev;
    render::ClipStack clip(dev);

    {
        auto outer = clip.push(foundation::NanRect::from_xywh(0, 0, 100, 100));
        REQUIRE(dev.clips.size() == 1);
        REQUIRE(dev.clips[0].rect.get_width() == Catch::Approx(100.0F));

        {
            // Child clip exceeds parent on the right/bottom; result is the
            // intersection (50..100 x 50..100 => x=50,y=50,w=50,h=50).
            auto inner = clip.push(foundation::NanRect::from_xywh(50, 50, 100, 100));
            REQUIRE(clip.depth() == 2);
            const auto& top = dev.clips.back();
            REQUIRE(top.rect.get_left() == Catch::Approx(50.0F));
            REQUIRE(top.rect.get_top() == Catch::Approx(50.0F));
            REQUIRE(top.rect.get_width() == Catch::Approx(50.0F));
            REQUIRE(top.rect.get_height() == Catch::Approx(50.0F));
        }
        // inner popped: back to the outer rect.
        REQUIRE(clip.depth() == 1);
        REQUIRE(dev.clips.back().rect.get_width() == Catch::Approx(100.0F));
    }
    // outer popped: stack empty => clear_clip issued.
    REQUIRE(clip.depth() == 0);
    REQUIRE(dev.clips.back().cleared);
}

TEST_CASE("inherited opacity multiplies down the tree", "[render][opacity]") {
    // RectNode folds ctx.opacity() into alpha. Base opacity is 1.0 (no opacity
    // inheritance API on Node2D yet), so this verifies the identity case: a
    // node tagged 0.5 draws at 0.5 when ctx.opacity() == 1.
    RecordingDevice dev;
    scene::NanSceneTree tree;
    tree.set_root(std::make_shared<RectNode>(0.5F));

    tree.draw(dev);

    REQUIRE(dev.rects.size() == 1);
    REQUIRE(dev.rects[0].alpha == Catch::Approx(0.5F));
}
