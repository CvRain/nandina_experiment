#include <raylib.h>
#include <spdlog/spdlog.h>

#include <cmath>
#include <memory>
#include <numbers>

#include "foundation/geometry.hpp"
#include "foundation/nandina_color.hpp"
#include "scene/node.hpp"
#include "scene/node2d.hpp"
#include "scene/scene_tree.hpp"

using namespace nandina;

namespace
{
    [[nodiscard]] auto to_raylib(const foundation::NanColor& color) -> Color {
        const auto rgba = color.to<foundation::NanHexRgb>();
        return {static_cast<unsigned char>(rgba.red),
                static_cast<unsigned char>(rgba.green),
                static_cast<unsigned char>(rgba.blue),
                static_cast<unsigned char>(rgba.alpha)};
    }

    /// A drawable colored rectangle with a label.
    class RectNode : public scene::NanNode2D {
    public:
        RectNode(foundation::NanSize size, Color color, std::string label = {})
            : size_(size), color_(color), label_(std::move(label)) {}

        void on_draw() override {
            if (!visible()) return;

            const auto gt = global_transform();
            const auto w = size_.get_width() * gt.scale_x();
            const auto h = size_.get_height() * gt.scale_y();
            const auto pos = gt.position();

            DrawRectanglePro(
                {pos.get_x() - w / 2.0F, pos.get_y() - h / 2.0F, w, h},
                {w / 2.0F, h / 2.0F},
                gt.rotation() * 180.0F / std::numbers::pi_v<float>,
                color_);

            if (!label_.empty()) {
                const auto tw = static_cast<float>(MeasureText(label_.c_str(), 12));
                DrawText(label_.c_str(),
                         static_cast<int>(pos.get_x() - tw / 2.0F),
                         static_cast<int>(pos.get_y() - 6),
                         12, WHITE);
            }
        }

        [[nodiscard]] auto contains_point(foundation::NanPoint local_point) const -> bool override {
            const auto hw = size_.get_width() / 2.0F;
            const auto hh = size_.get_height() / 2.0F;
            return local_point.get_x() >= -hw && local_point.get_x() <= hw
                && local_point.get_y() >= -hh && local_point.get_y() <= hh;
        }

        void set_color(Color c) { color_ = c; }

    private:
        foundation::NanSize size_;
        Color color_;
        std::string label_;
    };

    /// Builds a deep chain that returns the leaf node for assertions.
    struct DeepChain {
        std::unique_ptr<RectNode> root;
        scene::NanNode2D* leaf = nullptr;
    };

    auto build_deep_chain(Color color, float dx, float dy) -> DeepChain {
        auto leaf = std::make_unique<RectNode>(foundation::NanSize(24, 16), color, "L4");
        auto* leaf_ptr = leaf.get();

        auto l3 = std::make_unique<RectNode>(foundation::NanSize(28, 20), color, "L3");
        l3->set_position(foundation::NanPoint(dx, dy));
        l3->add_child(std::move(leaf));

        auto l2 = std::make_unique<RectNode>(foundation::NanSize(32, 24), color, "L2");
        l2->set_position(foundation::NanPoint(dx, dy));
        l2->add_child(std::move(l3));

        auto l1 = std::make_unique<RectNode>(foundation::NanSize(36, 28), color, "L1");
        l1->set_position(foundation::NanPoint(dx, dy));
        l1->add_child(std::move(l2));

        auto root = std::make_unique<RectNode>(foundation::NanSize(40, 32), color, "root");
        root->set_name("deep_root");
        root->add_child(std::move(l1));

        return {std::move(root), leaf_ptr};
    }

    /// Draw text with background panel.
    void draw_panel(float x, float y, float w, float h, Color bg, Color border) {
        DrawRectangle(static_cast<int>(x), static_cast<int>(y),
                      static_cast<int>(w), static_cast<int>(h), bg);
        DrawRectangleLines(static_cast<int>(x), static_cast<int>(y),
                           static_cast<int>(w), static_cast<int>(h), border);
    }
} // namespace

auto main() -> int {
    spdlog::info("Nandina — verification demo");

    constexpr int W = 1100;
    constexpr int H = 700;
    InitWindow(W, H, "Nandina — Scene Tree Verification");
    SetTargetFPS(60);

    const auto primary = foundation::NanColor::from(
        foundation::NanOklch{.light = 0.62F, .chroma = 0.18F, .hue = 250.0F, .alpha = 1.0F});
    const auto accent = primary.rotate_hue(95.0F).saturate(0.03F);
    const auto bg = foundation::NanColor::from(
        foundation::NanHexRgb{.red = 239, .green = 241, .blue = 245, .alpha = 1});

    const Color c_bg = to_raylib(bg);
    const Color c_primary = to_raylib(primary);
    const Color c_accent = to_raylib(accent);
    const Color c_chain = {100, 160, 220, 255};
    const Color c_overlap_a = {220, 120, 80, 200};
    const Color c_overlap_b = {80, 180, 120, 200};
    const Color c_overlap_c = {140, 100, 210, 200};
    const Color c_text = {30, 30, 40, 255};
    const Color c_subtle = {110, 115, 130, 255};
    const Color c_panel = {255, 255, 255, 255};
    const Color c_border = {200, 205, 215, 255};
    const Color c_hit = {240, 70, 70, 255};

    // ========== TEST 1: Transform Cascade ==========
    // Panel (rotates) → Card (scales) → Button (stationary relative to Card)
    auto tree1 = scene::NanSceneTree();

    auto panel = std::make_unique<RectNode>(foundation::NanSize(140, 100), c_primary, "Panel");
    panel->set_name("panel");
    panel->set_position(foundation::NanPoint(200, 140));

    auto card = std::make_unique<RectNode>(foundation::NanSize(80, 60), c_accent, "Card");
    card->set_name("card");
    card->set_position(foundation::NanPoint(0, 0));

    auto button = std::make_unique<RectNode>(foundation::NanSize(40, 24), Color{220, 180, 80, 255}, "Btn");
    button->set_name("button");
    button->set_position(foundation::NanPoint(0, 0));

    card->add_child(std::move(button));
    panel->add_child(std::move(card));
    tree1.set_root(std::move(panel));

    // ========== TEST 6: Deep Chain Global Position ==========
    auto tree6 = scene::NanSceneTree();
    auto [chain1_root, leaf1_ptr] = build_deep_chain(c_chain, 8.0F, 12.0F);
    auto [chain2_root, leaf2_ptr] = build_deep_chain(c_accent, 14.0F, -8.0F);
    chain2_root->set_position(foundation::NanPoint(160, 0));

    auto t6_root = std::make_unique<RectNode>(foundation::NanSize(220, 120), c_panel, "Tree6");
    t6_root->set_position(foundation::NanPoint(420, 140));
    t6_root->add_child(std::move(chain1_root));
    t6_root->add_child(std::move(chain2_root));
    tree6.set_root(std::move(t6_root));

    // ========== TEST 7: Z-Index Occlusion ==========
    auto tree7 = scene::NanSceneTree();

    auto z_root = std::make_unique<RectNode>(foundation::NanSize(180, 120), c_panel, "overlap");
    z_root->set_name("z_root");
    z_root->set_position(foundation::NanPoint(1000, 140));

    auto z_low = std::make_unique<RectNode>(foundation::NanSize(80, 80), c_overlap_a, "z=0");
    z_low->set_name("z_low");
    z_low->set_position(foundation::NanPoint(-30, -20));

    auto z_mid = std::make_unique<RectNode>(foundation::NanSize(80, 80), c_overlap_b, "z=5");
    z_mid->set_name("z_mid");
    z_mid->set_position(foundation::NanPoint(20, -30));
    z_mid->set_z_index(5);

    auto z_high = std::make_unique<RectNode>(foundation::NanSize(80, 80), c_overlap_c, "z=10");
    z_high->set_name("z_high");
    z_high->set_position(foundation::NanPoint(10, 20));
    z_high->set_z_index(10);

    z_root->add_child(std::move(z_low));
    z_root->add_child(std::move(z_mid));
    z_root->add_child(std::move(z_high));
    tree7.set_root(std::move(z_root));

    // ---- animation ----
    float time = 0.0F;
    const scene::NanNode2D* hit1 = nullptr;
    const scene::NanNode2D* hit7 = nullptr;

    while (not WindowShouldClose()) {
        const auto dt = GetFrameTime();
        time += dt;

        // Animate test 1: panel rotates and scales
        if (auto* p = static_cast<RectNode*>(tree1.root())) {
            p->set_rotation(time * 0.8F);
            const auto s = 1.0F + std::sin(time * 1.2F) * 0.15F;
            p->set_scale(s, s);
        }

        // Hit tests
        const auto mouse = foundation::NanPoint(
            static_cast<float>(GetMouseX()),
            static_cast<float>(GetMouseY()));
        hit1 = tree1.hit_test(mouse);
        hit7 = tree7.hit_test(mouse);

        tree1.process(dt);
        tree6.process(dt);
        tree7.process(dt);

        // ---- draw ----
        BeginDrawing();
        ClearBackground(c_bg);

        DrawText("Scene Tree Verification", 24, 14, 22, c_text);

        // ---- Test 1 ----
        draw_panel(12, 48, 340, 240, c_panel, c_border);
        DrawText("Test 1: Transform Cascade", 22, 54, 14, c_text);
        DrawText("Panel rotates+scales. Button stays centered on Card.", 22, 72, 12, c_subtle);
        tree1.draw();

        // Test 1 info
        if (auto* p = static_cast<RectNode*>(tree1.root())) {
            auto* card_n = static_cast<scene::NanNode2D*>(p->get_child(0));
            auto* btn_n = card_n ? static_cast<scene::NanNode2D*>(card_n->get_child(0)) : nullptr;
            const auto gp = btn_n ? btn_n->global_position() : foundation::NanPoint(0, 0);
            DrawText(TextFormat("panel rot=%.2f  scale=(%.2f,%.2f)",
                p->rotation(), p->scale().get_x(), p->scale().get_y()),
                22, 256, 12, c_subtle);
            DrawText(TextFormat("button global=(%.0f,%.0f)",
                gp.get_x(), gp.get_y()),
                22, 270, 12, c_subtle);
        }

        // ---- Test 6 ----
        draw_panel(370, 48, 340, 240, c_panel, c_border);
        DrawText("Test 6: Deep Chain Global Pos", 380, 54, 14, c_text);
        DrawText("5-layer chains. Leaf position = sum of all offsets.", 380, 72, 12, c_subtle);
        tree6.draw();

        // Show expected vs actual global positions
        if (tree6.root()) {
            const auto gp1 = leaf1_ptr->global_position();
            const auto gp2 = leaf2_ptr->global_position();
            // chain1: root_pos(420,140) + L1(8,12) + L2(8,12) + L3(8,12) + L4(8,12) = (452,188)
            const float ex1_x = 420 + 8*4;
            const float ex1_y = 140 + 12*4;
            // chain2: root_pos(420,140) + offset(160,0) + L1(14,-8)*4 = (636,108)
            const float ex2_x = 420 + 160 + 14*4;
            const float ex2_y = 140 + 0 + (-8)*4;

            DrawText(TextFormat("chain1 leaf: global=(%.0f,%.0f)  expected=(%.0f,%.0f)",
                gp1.get_x(), gp1.get_y(), ex1_x, ex1_y),
                380, 252, 12, std::abs(gp1.get_x() - ex1_x) < 1 ? c_accent : c_hit);
            DrawText(TextFormat("chain2 leaf: global=(%.0f,%.0f)  expected=(%.0f,%.0f)",
                gp2.get_x(), gp2.get_y(), ex2_x, ex2_y),
                380, 266, 12, std::abs(gp2.get_x() - ex2_x) < 1 ? c_accent : c_hit);
        }

        // ---- Test 7 ----
        draw_panel(720, 48, 360, 240, c_panel, c_border);
        DrawText("Test 7: Z-Index Occlusion + Hit", 730, 54, 14, c_text);
        DrawText("3 overlapping rects: z=0(green) z=5(blue) z=10(purple)", 730, 72, 12, c_subtle);
        DrawText("Move mouse over overlaps — topmost node is hit.", 730, 86, 12, c_subtle);
        tree7.draw();

        if (hit7) {
            DrawText(TextFormat("hit: %s  (z=%d)",
                hit7->name().data(), static_cast<const scene::NanNode2D*>(hit7)->z_index()),
                730, 258, 13, c_hit);
        } else {
            DrawText("hit: (none)", 730, 258, 13, c_subtle);
        }

        // ---- Legend / lifecycle log ----
        draw_panel(12, 300, 1068, 140, c_panel, c_border);
        DrawText("Lifecycle + queue_delete verification", 22, 306, 14, c_text);
        DrawText("on_enter_tree: top-down (parent first).  on_ready: bottom-up (children first).", 22, 324, 12, c_subtle);
        DrawText("Press SPACE to queue_delete the rotating Panel — it will be removed at next frame start.", 22, 340, 12, c_subtle);
        DrawText("Press R to recreate the Panel subtree (verifies enter→ready lifecycle on re-add).", 22, 356, 12, c_subtle);

        // Handle keyboard
        static bool panel_deleted = false;
        if (IsKeyPressed(KEY_SPACE) && !panel_deleted) {
            if (auto* p = tree1.root()) {
                tree1.queue_delete(*p);
                spdlog::info("queue_delete: panel queued for deletion");
                panel_deleted = true;
            }
        }
        if (IsKeyPressed(KEY_R)) {
            auto new_panel = std::make_unique<RectNode>(foundation::NanSize(140, 100), c_primary, "Panel");
            new_panel->set_name("panel");

            auto new_card = std::make_unique<RectNode>(foundation::NanSize(80, 60), c_accent, "Card");
            new_card->set_name("card");
            new_card->set_position(foundation::NanPoint(0, 0));

            auto new_button = std::make_unique<RectNode>(foundation::NanSize(40, 24), Color{220, 180, 80, 255}, "Btn");
            new_button->set_name("button");
            new_button->set_position(foundation::NanPoint(0, 0));
            new_card->add_child(std::move(new_button));
            new_panel->add_child(std::move(new_card));
            tree1.set_root(std::move(new_panel));
            spdlog::info("Panel recreated — on_enter_tree + on_ready should fire");
            panel_deleted = false;
        }

        auto state = panel_deleted ? "DELETED (press R)" : "alive";
        DrawText(TextFormat("Panel state: %s", state), 22, 420, 13, panel_deleted ? c_hit : c_accent);

        // Global hit info
        DrawText(TextFormat("Mouse=(%.0f,%.0f)  Test1 hit=%s  Test7 hit=%s",
            mouse.get_x(), mouse.get_y(),
            hit1 ? hit1->name().data() : "-",
            hit7 ? hit7->name().data() : "-"),
            22, 440, 12, c_subtle);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
