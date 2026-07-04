#include <raylib.h>
#include <spdlog/spdlog.h>

#include "utils.hpp"
#include <cmath>
#include <memory>
#include <numbers>
#include <string>

#include "foundation/geometry.hpp"
#include "foundation/nandina_color.hpp"
#include "render/backends/raylib_device.hpp"
#include "render/draw_context.hpp"
#include "scene/input_event.hpp"
#include "scene/node2d.hpp"
#include "scene/scene_tree.hpp"

using namespace nandina;

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
    constexpr Color c_chain = {100, 160, 220, 255};
    constexpr Color c_overlap_a = {220, 120, 80, 255};
    constexpr Color c_overlap_b = {80, 180, 120, 255};
    constexpr Color c_overlap_c = {140, 100, 210, 255};
    constexpr Color c_text = {30, 30, 40, 255};
    constexpr Color c_subtle = {110, 115, 130, 255};
    constexpr Color c_panel = {255, 255, 255, 255};
    constexpr Color c_border = {200, 205, 215, 255};
    constexpr Color c_hit = {240, 70, 70, 255};

    auto device = render::make_raylib_device();
    auto tree = scene::NanSceneTree();
    auto scene_root = std::make_shared<GroupNode>();
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

    auto stack_rig =
        build_stack_rig(foundation::NanPoint(980, 146), c_overlap_a, c_overlap_b, c_overlap_c);
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
            tree.dispatch_mouse_move(
                scene::MouseMoveEvent(mouse, foundation::NanPoint(mx - prev_mx, my - prev_my))
            );
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

        if (IsKeyPressed(KEY_TAB)) {
            if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
                tree.focus_previous();
            }
            else {
                tree.focus_next();
            }
        }

        if (IsKeyPressed(KEY_ENTER)) {
            tree.dispatch_key(scene::KeyEvent(KEY_ENTER, scene::KeyEvent::Action::press));
        }
        if (IsKeyPressed(KEY_SPACE)) {
            tree.dispatch_key(scene::KeyEvent(KEY_SPACE, scene::KeyEvent::Action::press));
        }

        if (const float wheel = GetMouseWheelMove(); wheel != 0.0F) {
            tree.dispatch_mouse_wheel(
                scene::MouseWheelEvent(mouse, foundation::NanPoint(0, wheel))
            );
        }

        for (int codepoint = GetCharPressed(); codepoint != 0; codepoint = GetCharPressed()) {
            if (codepoint >= 32) {
                tree.dispatch_text_input(
                    scene::TextInputEvent(std::string(1, static_cast<char>(codepoint)))
                );
            }
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            tree.set_focus(nullptr);
        }

        if (transform_present && IsKeyPressed(KEY_DELETE) && transform_root) {
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
            12,
            48,
            340,
            240,
            "Transform Rig",
            "Single SceneTree: parent rotation + child scale + clickable leaf node.",
            c_text,
            c_subtle,
            c_panel,
            c_border
        );
        draw_section(
            370,
            48,
            340,
            240,
            "Hierarchy Chain",
            "Two deep chains verify global position accumulation across nested nodes.",
            c_text,
            c_subtle,
            c_panel,
            c_border
        );
        draw_section(
            728,
            48,
            360,
            240,
            "Interaction Stack",
            "Hover tracks the topmost hit node. Click focuses it. Enter/Space activates focus.",
            c_text,
            c_subtle,
            c_panel,
            c_border
        );
        draw_panel(12, 304, 1076, 152, c_panel, c_border);

        DrawText("Nandina SceneTree Workspace", 24, 14, 22, c_text);

        {
            render::DrawContext ctx {*device};
            tree.render(ctx);
        }

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
        }
        else {
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
        }
        else {
            DrawText("hovered: (none)", 738, 252, 13, c_subtle);
        }
        DrawText(
            "Try click to focus, Tab to cycle focus, Enter/Space to toggle the focused card.",
            738,
            268,
            12,
            c_subtle
        );

        DrawText("Runtime Status", 24, 314, 14, c_text);
        DrawText(
            "Current progress: lifecycle + transform cascade + deferred delete + hover target tracking + click bubbling.",
            24,
            334,
            12,
            c_subtle
        );
        DrawText(
            "Click assigns focus. Tab rotates focus across interactive nodes. Delete queues removal of the left transform rig.",
            24,
            350,
            12,
            c_subtle
        );
        DrawText(
            "R rebuilds the transform rig. Escape clears focus. This scene can grow into animation, clip, Control-like layout, and richer input.",
            24,
            366,
            12,
            c_subtle
        );
        DrawText(
            TextFormat(
                "hover=%s  focus=%s",
                tree.hovered_node() ? tree.hovered_node()->name().data() : "-",
                tree.focused_node() ? tree.focused_node()->name().data() : "-"
            ),
            24,
            382,
            12,
            c_subtle
        );
        DrawText(
            TextFormat(
                "mouse=(%.0f, %.0f)  transform-rig=%s",
                mouse.get_x(),
                mouse.get_y(),
                transform_present ? "alive" : "removed"
            ),
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
