#include <raylib.h>
#include <spdlog/spdlog.h>

#include <cmath>
#include <memory>
#include <numbers>
#include <string>

#include "foundation/geometry.hpp"
#include "foundation/nandina_color.hpp"
#include "scene/input_event.hpp"
#include "scene/node2d.hpp"
#include "scene/scene_tree.hpp"

using namespace nandina;

namespace
{

[[nodiscard]] auto to_raylib(const foundation::NanColor& color) -> Color {
    const auto rgba = color.to<foundation::NanHexRgb>();
    return {
        static_cast<unsigned char>(rgba.red),
        static_cast<unsigned char>(rgba.green),
        static_cast<unsigned char>(rgba.blue),
        static_cast<unsigned char>(rgba.alpha)
    };
}

[[nodiscard]] auto lighten(Color color, int amount) -> Color {
    auto bump = [amount](unsigned char channel) -> unsigned char {
        const int value = static_cast<int>(channel) + amount;
        return static_cast<unsigned char>(value > 255 ? 255 : value);
    };

    return {bump(color.r), bump(color.g), bump(color.b), color.a};
}

[[nodiscard]] auto lerp_color(Color a, Color b, float t) -> Color {
    const auto lerp = [t](unsigned char from, unsigned char to) -> unsigned char {
        return static_cast<unsigned char>(from + (to - from) * t);
    };

    return {
        lerp(a.r, b.r),
        lerp(a.g, b.g),
        lerp(a.b, b.b),
        lerp(a.a, b.a)
    };
}

class GroupNode final : public scene::NanNode2D {
public:
    [[nodiscard]] auto contains_point(foundation::NanPoint /*local_point*/) const -> bool override {
        return false;
    }
};

class CardNode final : public scene::NanNode2D {
public:
    CardNode(
        foundation::NanSize size,
        Color base_color,
        std::string label,
        bool interactive = true
    )
        : size_(size),
          base_color_(base_color),
          hover_color_(lighten(base_color, 22)),
          toggled_color_(lighten(base_color, 56)),
          hover_toggled_color_(lighten(base_color, 80)),
          label_(std::move(label)),
          interactive_(interactive) {}

    [[nodiscard]] auto contains_point(foundation::NanPoint local_point) const -> bool override {
        const auto hw = size_.get_width() / 2.0F;
        const auto hh = size_.get_height() / 2.0F;
        return local_point.get_x() >= -hw && local_point.get_x() <= hw
            && local_point.get_y() >= -hh && local_point.get_y() <= hh;
    }

    auto on_input(scene::InputEvent& event) -> bool override {
        if (!interactive_) {
            return false;
        }

        if (dynamic_cast<const scene::MouseEnterEvent*>(&event)) {
            hovered_ = true;
            return false;
        }

        if (dynamic_cast<const scene::MouseLeaveEvent*>(&event)) {
            hovered_ = false;
            return false;
        }

        if (const auto* button = dynamic_cast<const scene::MouseButtonEvent*>(&event)) {
            if (button->button() == scene::MouseButtonEvent::Button::left && button->is_pressed()) {
                toggled_ = !toggled_;
                event.accept();
                spdlog::info("click: {}", name());
                return true;
            }
        }

        return false;
    }

    void on_draw() override {
        const auto gt = global_transform();
        const auto pos = gt.position();
        const auto w = size_.get_width() * gt.scale_x();
        const auto h = size_.get_height() * gt.scale_y();
        const auto angle = gt.rotation() * 180.0F / std::numbers::pi_v<float>;

        const auto fill = current_fill_color();
        const auto shadow = lerp_color(fill, BLACK, 0.28F);
        const auto text = hovered_ ? BLACK : Color {32, 34, 42, 255};

        DrawRectanglePro(
            {pos.get_x() - w / 2.0F + 4.0F, pos.get_y() - h / 2.0F + 5.0F, w, h},
            {w / 2.0F, h / 2.0F},
            angle,
            Color {shadow.r, shadow.g, shadow.b, 42}
        );
        DrawRectanglePro(
            {pos.get_x() - w / 2.0F, pos.get_y() - h / 2.0F, w, h},
            {w / 2.0F, h / 2.0F},
            angle,
            fill
        );

        if (!label_.empty()) {
            const auto font_size = static_cast<int>(h < 28.0F ? 11.0F : 12.0F);
            const auto tw = static_cast<float>(MeasureText(label_.c_str(), font_size));
            DrawText(
                label_.c_str(),
                static_cast<int>(pos.get_x() - tw / 2.0F),
                static_cast<int>(pos.get_y() - font_size / 2.0F),
                font_size,
                text
            );
        }
    }

private:
    [[nodiscard]] auto current_fill_color() const -> Color {
        if (toggled_ && hovered_) {
            return hover_toggled_color_;
        }
        if (toggled_) {
            return toggled_color_;
        }
        if (hovered_) {
            return hover_color_;
        }
        return base_color_;
    }

    foundation::NanSize size_;
    Color base_color_;
    Color hover_color_;
    Color toggled_color_;
    Color hover_toggled_color_;
    std::string label_;
    bool interactive_ = true;
    bool hovered_ = false;
    bool toggled_ = false;
};

struct DeepChain {
    std::unique_ptr<CardNode> root;
    scene::NanNode2D* leaf = nullptr;
};

struct TransformRig {
    std::unique_ptr<GroupNode> root;
    GroupNode* spinner = nullptr;
    CardNode* shell = nullptr;
    scene::NanNode2D* button = nullptr;
    GroupNode* root_node = nullptr;
};

struct ChainRig {
    std::unique_ptr<GroupNode> root;
    scene::NanNode2D* leaf_a = nullptr;
    scene::NanNode2D* leaf_b = nullptr;
};

struct StackRig {
    std::unique_ptr<GroupNode> root;
};

auto build_deep_chain(Color color, float dx, float dy, const char* prefix) -> DeepChain {
    auto leaf = std::make_unique<CardNode>(foundation::NanSize(24, 16), color, "L4", false);
    leaf->set_name(std::string(prefix) + "_leaf");
    leaf->set_position(foundation::NanPoint(dx, dy));
    auto* leaf_ptr = leaf.get();

    auto l3 = std::make_unique<CardNode>(foundation::NanSize(28, 20), color, "L3", false);
    l3->set_position(foundation::NanPoint(dx, dy));
    l3->add_child(std::move(leaf));

    auto l2 = std::make_unique<CardNode>(foundation::NanSize(32, 24), color, "L2", false);
    l2->set_position(foundation::NanPoint(dx, dy));
    l2->add_child(std::move(l3));

    auto l1 = std::make_unique<CardNode>(foundation::NanSize(36, 28), color, "L1", false);
    l1->set_position(foundation::NanPoint(dx, dy));
    l1->add_child(std::move(l2));

    auto root = std::make_unique<CardNode>(foundation::NanSize(40, 32), color, "Root", false);
    root->set_name(std::string(prefix) + "_root");
    root->add_child(std::move(l1));

    return {std::move(root), leaf_ptr};
}

auto build_transform_rig(foundation::NanPoint center, Color primary, Color accent) -> TransformRig {
    TransformRig rig;

    auto root = std::make_unique<GroupNode>();
    rig.root_node = root.get();
    rig.root_node->set_name("transform_rig");
    rig.root_node->set_position(center);

    auto spinner = std::make_unique<GroupNode>();
    rig.spinner = spinner.get();
    rig.spinner->set_name("transform_spinner");

    auto shell = std::make_unique<CardNode>(foundation::NanSize(148, 104), primary, "Panel", false);
    rig.shell = shell.get();
    rig.shell->set_name("panel_shell");

    auto card = std::make_unique<CardNode>(foundation::NanSize(86, 62), accent, "Card", false);
    card->set_name("panel_card");
    card->set_position(foundation::NanPoint(0, 0));

    auto button = std::make_unique<CardNode>(
        foundation::NanSize(46, 28),
        Color {220, 180, 80, 255},
        "Node",
        true
    );
    rig.button = button.get();
    rig.button->set_name("panel_button");
    rig.button->set_position(foundation::NanPoint(0, 0));

    card->add_child(std::move(button));
    shell->add_child(std::move(card));
    spinner->add_child(std::move(shell));
    root->add_child(std::move(spinner));
    rig.root = std::move(root);

    return rig;
}

auto build_chain_rig(foundation::NanPoint center, Color chain_color, Color accent) -> ChainRig {
    ChainRig rig;

    auto root = std::make_unique<GroupNode>();
    root->set_name("chain_rig");
    root->set_position(center);

    auto [chain_a, leaf_a] = build_deep_chain(chain_color, 8.0F, 12.0F, "chain_a");
    auto [chain_b, leaf_b] = build_deep_chain(accent, 14.0F, -8.0F, "chain_b");
    chain_b->set_position(foundation::NanPoint(160, 0));

    rig.leaf_a = leaf_a;
    rig.leaf_b = leaf_b;

    root->add_child(std::move(chain_a));
    root->add_child(std::move(chain_b));
    rig.root = std::move(root);

    return rig;
}

auto build_stack_rig(foundation::NanPoint center, Color low, Color mid, Color high) -> StackRig {
    StackRig rig;

    auto root = std::make_unique<GroupNode>();
    root->set_name("stack_rig");
    root->set_position(center);

    auto low_card = std::make_unique<CardNode>(foundation::NanSize(82, 82), low, "z=0", true);
    low_card->set_name("stack_low");
    low_card->set_position(foundation::NanPoint(-30, -20));

    auto mid_card = std::make_unique<CardNode>(foundation::NanSize(82, 82), mid, "z=5", true);
    mid_card->set_name("stack_mid");
    mid_card->set_position(foundation::NanPoint(20, -30));
    mid_card->set_z_index(5);

    auto high_card = std::make_unique<CardNode>(foundation::NanSize(82, 82), high, "z=10", true);
    high_card->set_name("stack_high");
    high_card->set_position(foundation::NanPoint(10, 20));
    high_card->set_z_index(10);

    root->add_child(std::move(low_card));
    root->add_child(std::move(mid_card));
    root->add_child(std::move(high_card));
    rig.root = std::move(root);

    return rig;
}

void draw_panel(float x, float y, float w, float h, Color bg, Color border) {
    DrawRectangle(
        static_cast<int>(x),
        static_cast<int>(y),
        static_cast<int>(w),
        static_cast<int>(h),
        bg
    );
    DrawRectangleLines(
        static_cast<int>(x),
        static_cast<int>(y),
        static_cast<int>(w),
        static_cast<int>(h),
        border
    );
}

void draw_section(float x, float y, float w, float h, const char* title, const char* subtitle, Color text, Color subtle, Color bg, Color border) {
    draw_panel(x, y, w, h, bg, border);
    DrawText(title, static_cast<int>(x + 10), static_cast<int>(y + 8), 14, text);
    DrawText(subtitle, static_cast<int>(x + 10), static_cast<int>(y + 26), 12, subtle);
}

} // namespace

auto main() -> int {
    spdlog::info("Nandina — SceneTree workspace demo");

    constexpr int window_width = 1100;
    constexpr int window_height = 700;
    InitWindow(window_width, window_height, "Nandina — SceneTree Workspace");
    SetTargetFPS(60);

    const auto primary = foundation::NanColor::from(
        foundation::NanOklch {.light = 0.62F, .chroma = 0.18F, .hue = 250.0F, .alpha = 1.0F}
    );
    const auto accent = primary.rotate_hue(95.0F).saturate(0.03F);
    const auto background = foundation::NanColor::from(
        foundation::NanHexRgb {.red = 239, .green = 241, .blue = 245, .alpha = 1}
    );

    const Color c_bg = to_raylib(background);
    const Color c_primary = to_raylib(primary);
    const Color c_accent = to_raylib(accent);
    const Color c_chain = {100, 160, 220, 255};
    const Color c_overlap_a = {220, 120, 80, 255};
    const Color c_overlap_b = {80, 180, 120, 255};
    const Color c_overlap_c = {140, 100, 210, 255};
    const Color c_text = {30, 30, 40, 255};
    const Color c_subtle = {110, 115, 130, 255};
    const Color c_panel = {255, 255, 255, 255};
    const Color c_border = {200, 205, 215, 255};
    const Color c_hit = {240, 70, 70, 255};

    auto tree = scene::NanSceneTree();
    auto scene_root = std::make_unique<GroupNode>();
    scene_root->set_name("scene_root");
    auto* scene_root_ptr = scene_root.get();
    tree.set_root(std::move(scene_root));

    auto transform_rig = build_transform_rig(foundation::NanPoint(182, 166), c_primary, c_accent);
    auto* transform_root = transform_rig.root_node;
    auto* transform_spinner = transform_rig.spinner;
    auto* transform_shell = transform_rig.shell;
    auto* transform_button = transform_rig.button;
    scene_root_ptr->add_child(std::move(transform_rig.root));
    bool transform_present = true;

    auto chain_rig = build_chain_rig(foundation::NanPoint(430, 140), c_chain, c_accent);
    auto* chain_leaf_a = chain_rig.leaf_a;
    auto* chain_leaf_b = chain_rig.leaf_b;
    scene_root_ptr->add_child(std::move(chain_rig.root));

    auto stack_rig = build_stack_rig(
        foundation::NanPoint(980, 146),
        c_overlap_a,
        c_overlap_b,
        c_overlap_c
    );
    scene_root_ptr->add_child(std::move(stack_rig.root));

    float time = 0.0F;
    float shell_scale_phase = 0.0F;

    while (!WindowShouldClose()) {
        const auto dt = GetFrameTime();
        time += dt;
        shell_scale_phase += dt * 1.2F;

        const auto mx = static_cast<float>(GetMouseX());
        const auto my = static_cast<float>(GetMouseY());
        const auto mouse = foundation::NanPoint(mx, my);

        static auto prev_mx = mx;
        static auto prev_my = my;
        if (mx != prev_mx || my != prev_my) {
            tree.dispatch_mouse_move(scene::MouseMoveEvent(mouse, foundation::NanPoint(mx - prev_mx, my - prev_my)));
            prev_mx = mx;
            prev_my = my;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            tree.dispatch_mouse_button(
                scene::MouseButtonEvent(
                    scene::MouseButtonEvent::Button::left,
                    scene::MouseButtonEvent::Action::press,
                    mouse
                )
            );
        }

        if (transform_present && IsKeyPressed(KEY_SPACE) && transform_root) {
            tree.queue_delete(*transform_root);
            transform_present = false;
            transform_root = nullptr;
            transform_spinner = nullptr;
            transform_shell = nullptr;
            transform_button = nullptr;
        }

        if (!transform_present && IsKeyPressed(KEY_R)) {
            auto rebuilt = build_transform_rig(foundation::NanPoint(182, 166), c_primary, c_accent);
            transform_root = rebuilt.root_node;
            transform_spinner = rebuilt.spinner;
            transform_shell = rebuilt.shell;
            transform_button = rebuilt.button;
            scene_root_ptr->add_child(std::move(rebuilt.root));
            transform_present = true;
        }

        if (transform_spinner) {
            transform_spinner->set_rotation(time * 0.8F);
        }
        if (transform_shell) {
            const auto scale = 1.0F + std::sin(shell_scale_phase) * 0.14F;
            transform_shell->set_scale(scale, scale);
        }

        tree.process(dt);

        BeginDrawing();
        ClearBackground(c_bg);

        draw_section(
            12, 48, 340, 240,
            "Transform Rig",
            "Single SceneTree: parent rotation + child scale + clickable leaf node.",
            c_text, c_subtle, c_panel, c_border
        );
        draw_section(
            370, 48, 340, 240,
            "Hierarchy Chain",
            "Two deep chains verify global position accumulation across nested nodes.",
            c_text, c_subtle, c_panel, c_border
        );
        draw_section(
            728, 48, 360, 240,
            "Interaction Stack",
            "Hover tracks the topmost hit node. Click toggles the active card state.",
            c_text, c_subtle, c_panel, c_border
        );
        draw_panel(12, 304, 1076, 152, c_panel, c_border);

        DrawText("Nandina SceneTree Workspace", 24, 14, 22, c_text);

        tree.draw();

        if (transform_spinner && transform_button) {
            const auto gp = transform_button->global_position();
            DrawText(
                TextFormat(
                    "spinner rot=%.2f  shell scale=(%.2f, %.2f)",
                    transform_spinner->rotation(),
                    transform_shell ? transform_shell->scale().get_x() : 1.0F,
                    transform_shell ? transform_shell->scale().get_y() : 1.0F
                ),
                22,
                252,
                12,
                c_subtle
            );
            DrawText(
                TextFormat("button global=(%.0f, %.0f)", gp.get_x(), gp.get_y()),
                22,
                266,
                12,
                c_subtle
            );
        } else {
            DrawText("Transform rig removed. Press R to rebuild it.", 22, 252, 12, c_hit);
        }

        if (chain_leaf_a && chain_leaf_b) {
            const auto gp1 = chain_leaf_a->global_position();
            const auto gp2 = chain_leaf_b->global_position();
            const float ex1_x = 430 + 8 * 4;
            const float ex1_y = 140 + 12 * 4;
            const float ex2_x = 430 + 160 + 14 * 4;
            const float ex2_y = 140 + (-8) * 4;

            DrawText(
                TextFormat(
                    "chain A leaf: global=(%.0f, %.0f)  expected=(%.0f, %.0f)",
                    gp1.get_x(),
                    gp1.get_y(),
                    ex1_x,
                    ex1_y
                ),
                380,
                252,
                12,
                std::abs(gp1.get_x() - ex1_x) < 1.0F ? c_accent : c_hit
            );
            DrawText(
                TextFormat(
                    "chain B leaf: global=(%.0f, %.0f)  expected=(%.0f, %.0f)",
                    gp2.get_x(),
                    gp2.get_y(),
                    ex2_x,
                    ex2_y
                ),
                380,
                266,
                12,
                std::abs(gp2.get_x() - ex2_x) < 1.0F ? c_accent : c_hit
            );
        }

        if (auto* hovered = tree.hovered_node()) {
            DrawText(
                TextFormat("hovered: %s  (z=%d)", hovered->name().data(), hovered->z_index()),
                738,
                252,
                13,
                c_hit
            );
        } else {
            DrawText("hovered: (none)", 738, 252, 13, c_subtle);
        }
        DrawText("Try moving across overlaps and clicking the topmost card.", 738, 268, 12, c_subtle);

        DrawText("Runtime Status", 24, 314, 14, c_text);
        DrawText(
            "Current progress: lifecycle + transform cascade + deferred delete + hover target tracking + click bubbling.",
            24,
            334,
            12,
            c_subtle
        );
        DrawText(
            "SPACE queues deletion of the left transform rig. R rebuilds it through add_child() on the live tree root.",
            24,
            350,
            12,
            c_subtle
        );
        DrawText(
            "This scene is intended to grow into later phases: animation, clip stack, Control-like layout nodes, and richer input.",
            24,
            366,
            12,
            c_subtle
        );
        DrawText(
            TextFormat("mouse=(%.0f, %.0f)  transform-rig=%s", mouse.get_x(), mouse.get_y(), transform_present ? "alive" : "removed"),
            24,
            412,
            13,
            transform_present ? c_accent : c_hit
        );

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
