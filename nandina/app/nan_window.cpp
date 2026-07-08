//
// Created by cvrain on 2026/7/4.
//
// raylib-backed implementation of NanWindow. raylib is included ONLY here.
//

#include "nan_window.hpp"
#include "nan_application.hpp"

#include "../render/backends/raylib_device.hpp"
#include "../render/draw_context.hpp"
#include "../scene/control.hpp"
#include "../scene/input_event.hpp"

#include <raylib.h>
#include <spdlog/spdlog.h>

#include <utility>

namespace nandina::app
{
    namespace
    {

        /// Snapshot raylib modifier-key state into a KeyModifiers.
        auto current_modifiers() -> scene::KeyModifiers {
            return scene::KeyModifiers {
                .shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT),
                .ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL),
                .alt = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT),
                .super = IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER),
            };
        }

        void dispatch_mouse_button(
            scene::NanSceneTree& tree,
            foundation::NanPoint pos,
            scene::MouseButtonEvent::Button button,
            int rl_button
        ) {
            const auto mods = current_modifiers();
            if (IsMouseButtonPressed(rl_button)) {
                scene::MouseButtonEvent
                    ev {button, scene::MouseButtonEvent::Action::press, pos, mods};
                tree.dispatch_mouse_button(ev);
            }
            if (IsMouseButtonReleased(rl_button)) {
                scene::MouseButtonEvent
                    ev {button, scene::MouseButtonEvent::Action::release, pos, mods};
                tree.dispatch_mouse_button(ev);
            }
        }

    } // namespace

    NanWindow::NanWindow(NanApplication& app, WindowConfig config):
        app_(app),
        config_(std::move(config)) {}

    NanWindow::~NanWindow() {
        // 若 run 未显式 close (异常路径), 兜底关闭窗口。
        if (opened_ && IsWindowReady()) {
            close();
        }
    }

    void NanWindow::set_content(std::shared_ptr<scene::NanNode2D> root) {
        tree_.set_root(std::move(root));
    }

    auto NanWindow::use_router() -> NanRouter& {
        router_ = std::make_unique<NanRouter>(
            app_.graph(),
            app_.theme(),
            app_.store_base(),
            app_.store_type_key()
        );
        set_content(router_->host());
        return *router_;
    }

    auto NanWindow::graph() -> reactive::Graph& {
        return app_.graph();
    }

    auto NanWindow::theme() const -> const theme::NanTheme& {
        return app_.theme();
    }

    void NanWindow::open() {
        if (opened_) {
            return;
        }

        unsigned int flags = 0;
        if (config_.msaa) {
            flags |= FLAG_MSAA_4X_HINT;
        }
        if (config_.vsync) {
            flags |= FLAG_VSYNC_HINT;
        }
        if (config_.resizable) {
            flags |= FLAG_WINDOW_RESIZABLE;
        }
        if (!config_.decorated) {
            flags |= FLAG_WINDOW_UNDECORATED;
        }
        SetConfigFlags(flags);

        InitWindow(config_.width, config_.height, config_.title.c_str());
        SetTargetFPS(config_.target_fps);

        device_ = render::make_raylib_device();
        opened_ = true;

        spdlog::info(
            "NanWindow: opened {}x{} \"{}\"",
            config_.width,
            config_.height,
            config_.title
        );
        on_setup();
    }

    auto NanWindow::should_close() const -> bool {
        return WindowShouldClose();
    }

    void NanWindow::poll_and_dispatch_input() {
        const auto mx = static_cast<float>(GetMouseX());
        const auto my = static_cast<float>(GetMouseY());
        const foundation::NanPoint pos {mx, my};

        // Mouse move (only when actually moved).
        if (!has_mouse_) {
            last_mouse_x_ = mx;
            last_mouse_y_ = my;
            has_mouse_ = true;
        }
        if (mx != last_mouse_x_ || my != last_mouse_y_) {
            scene::MouseMoveEvent ev {
                pos,
                foundation::NanPoint {mx - last_mouse_x_, my - last_mouse_y_}
            };
            tree_.dispatch_mouse_move(ev);
            last_mouse_x_ = mx;
            last_mouse_y_ = my;
        }

        // Mouse buttons.
        dispatch_mouse_button(tree_, pos, scene::MouseButtonEvent::Button::left, MOUSE_BUTTON_LEFT);
        dispatch_mouse_button(
            tree_,
            pos,
            scene::MouseButtonEvent::Button::right,
            MOUSE_BUTTON_RIGHT
        );
        dispatch_mouse_button(
            tree_,
            pos,
            scene::MouseButtonEvent::Button::middle,
            MOUSE_BUTTON_MIDDLE
        );

        // Mouse wheel.
        if (const float wheel = GetMouseWheelMove(); wheel != 0.0F) {
            scene::MouseWheelEvent ev {
                pos,
                foundation::NanPoint {0.0F, wheel},
                current_modifiers()
            };
            tree_.dispatch_mouse_wheel(ev);
        }

        // Keyboard: drain the pressed-key queue.
        for (int key = GetKeyPressed(); key != 0; key = GetKeyPressed()) {
            scene::KeyEvent ev {key, scene::KeyEvent::Action::press, current_modifiers()};
            tree_.dispatch_key(ev);
        }

        // Text input: drain the char queue (already composed unicode codepoints).
        for (int codepoint = GetCharPressed(); codepoint != 0; codepoint = GetCharPressed()) {
            if (codepoint >= 32) {
                tree_.dispatch_text_input(
                    scene::TextInputEvent {std::string(1, static_cast<char>(codepoint))}
                );
            }
        }
    }

    void NanWindow::tick() {
        const float dt = GetFrameTime();

        poll_and_dispatch_input();
        tree_.process(dt);
        on_frame(dt);

        if (auto* root = dynamic_cast<scene::NanControl*>(tree_.root())) {
            const auto window_size = foundation::NanSize(
                static_cast<float>(GetScreenWidth()),
                static_cast<float>(GetScreenHeight())
            );
            if (root->layout_dirty() || root->size() != window_size) {
                (void)root->measure_layout(scene::LayoutConstraints::tight(window_size));
                root->layout_to(foundation::NanRect::from_xywh(
                    0.0F,
                    0.0F,
                    window_size.get_width(),
                    window_size.get_height()
                ));
            }
        }

        device_->begin_frame();
        device_->clear(config_.background);
        {
            render::DrawContext ctx {*device_};
            tree_.render(ctx);
        }
        device_->end_frame();
    }

    void NanWindow::close() {
        if (!opened_) {
            return;
        }
        // 释放场景树 (触发 widget 卸载, 回访 graph) 后再释放设备、关窗口。
        device_.reset();
        CloseWindow();
        opened_ = false;
        spdlog::info("NanWindow: closed");
    }

} // namespace nandina::app
