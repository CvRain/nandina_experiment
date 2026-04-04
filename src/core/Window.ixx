module;

#include <SDL3/SDL.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <thorvg-1/thorvg.h>
#include <vector>

export module Nandina.Window;

import Nandina.Core;

export namespace Nandina {
    class NanWindow {
    public:
        struct WindowDeleter {
            void operator()(SDL_Window *w) const noexcept { if (w) SDL_DestroyWindow(w); }
        };

        struct RendererDeleter {
            void operator()(SDL_Renderer *r) const noexcept { if (r) SDL_DestroyRenderer(r); }
        };

        struct TextureDeleter {
            void operator()(SDL_Texture *t) const noexcept { if (t) SDL_DestroyTexture(t); }
        };

        class Builder {
        public:
            auto set_title(std::string_view title) -> Builder& {
                title_ = title;
                return *this;
            }

            auto set_width(int w) -> Builder& {
                width_ = w;
                return *this;
            }

            auto set_height(int h) -> Builder& {
                height_ = h;
                return *this;
            }

            [[nodiscard]] auto build() const -> NanWindow { return NanWindow{title_, width_, height_}; }

        private:
            std::string title_ = "Nandina Window";
            int width_ = 800;
            int height_ = 600;
        };

        NanWindow(const NanWindow &) = delete;

        auto operator=(const NanWindow &) -> NanWindow& = delete;

        NanWindow(NanWindow &&) noexcept = default;

        auto operator=(NanWindow &&) noexcept -> NanWindow& = default;

        auto set_layers(std::array<RenderLayer, 16>& layers) -> void {
            layers_ = &layers;
        }

        // Returns true when the application should quit
        [[nodiscard]] auto process_events() -> bool {
            SDL_Event sdl_event{};
            while (SDL_PollEvent(&sdl_event)) {
                if (sdl_event.type == SDL_EVENT_QUIT) {
                    return true;
                }
                if (layers_) {
                    translate_and_dispatch(sdl_event);
                }
            }
            return false;
        }

        auto present_frame() -> void {
            canvas_->remove(nullptr); // clear all paints from previous frame

            if (layers_) {
                // Render layers in order 0 → 15 (higher index renders on top)
                for (auto& layer : *layers_) {
                    if (layer.visible && layer.has_content()) {
                        render_widget(*layer.root);
                    }
                }
            } else {
                auto *bg = tvg::Shape::gen();
                bg->appendRect(0, 0, static_cast<float>(width_), static_cast<float>(height_), 0, 0);
                bg->fill(255, 255, 255, 255);
                canvas_->add(bg);
            }

            if (canvas_->draw(true) == tvg::Result::Success) {
                // true = clear buffer before draw
                canvas_->sync();
            }

            SDL_UpdateTexture(texture_.get(), nullptr, pixel_buffer_.data(),
                              width_ * static_cast<int>(sizeof(std::uint32_t)));
            SDL_RenderClear(renderer_.get());
            SDL_RenderTexture(renderer_.get(), texture_.get(), nullptr, nullptr);
            SDL_RenderPresent(renderer_.get());
        }

        // Convenience: block until the window is closed
        auto run() -> void {
            while (!process_events()) {
                present_frame();
            }
        }

    private:
        NanWindow(std::string_view title, int width, int height);

        static auto translate_button(std::uint8_t sdl_btn) noexcept -> MouseButton {
            switch (sdl_btn) {
                case SDL_BUTTON_LEFT: return MouseButton::left;
                case SDL_BUTTON_MIDDLE: return MouseButton::middle;
                case SDL_BUTTON_RIGHT: return MouseButton::right;
                default: return MouseButton::none;
            }
        }

        // Translate an SDL_Event to a Nandina Event.
        // Returns an Event with type==none for unhandled SDL event types.
        auto translate_event(const SDL_Event& sdl_event) -> Event {
            if (sdl_event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                press_x_ = sdl_event.button.x;
                press_y_ = sdl_event.button.y;
                pressed_ = true;
                return Event{
                    EventType::mouse_button_press,
                    translate_button(sdl_event.button.button),
                    press_x_, press_y_
                };
            }
            if (sdl_event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                const float rx = sdl_event.button.x;
                const float ry = sdl_event.button.y;
                // We need to dispatch both release and click; handle that in
                // translate_and_dispatch by synthesizing the click separately.
                pending_click_x_ = rx;
                pending_click_y_ = ry;
                pending_click_btn_ = sdl_event.button.button;
                pending_click_ = pressed_;
                pressed_ = false;
                return Event{
                    EventType::mouse_button_release,
                    translate_button(sdl_event.button.button),
                    rx, ry
                };
            }
            if (sdl_event.type == SDL_EVENT_MOUSE_MOTION) {
                return Event{
                    EventType::mouse_move, MouseButton::none,
                    sdl_event.motion.x, sdl_event.motion.y
                };
            }
            return Event{ EventType::none, MouseButton::none, 0.0f, 0.0f };
        }

        auto translate_and_dispatch(const SDL_Event &sdl_event) -> void {
            Event ev = translate_event(sdl_event);
            if (ev.type == EventType::none) { return; }

            // Dispatch from highest layer to lowest; stop at modal or handled.
            for (int i = static_cast<int>(layers_->size()) - 1; i >= 0; --i) {
                auto& layer = (*layers_)[i];
                if (!layer.visible || !layer.has_content()) { continue; }

                layer.root->dispatch_event(ev);

                if (ev.handled) { break; }
                if (layer.modal) { break; }
            }

            // Synthesize click after a mouse-up if released close to press point.
            if (ev.type == EventType::mouse_button_release && pending_click_) {
                pending_click_ = false;
                const float dx = pending_click_x_ - press_x_;
                const float dy = pending_click_y_ - press_y_;
                if (dx * dx + dy * dy < 256.0f) {
                    Event ev_click{
                        EventType::click,
                        translate_button(pending_click_btn_),
                        pending_click_x_, pending_click_y_
                    };
                    for (int i = static_cast<int>(layers_->size()) - 1; i >= 0; --i) {
                        auto& layer = (*layers_)[i];
                        if (!layer.visible || !layer.has_content()) { continue; }
                        layer.root->dispatch_event(ev_click);
                        if (ev_click.handled) { break; }
                        if (layer.modal) { break; }
                    }
                }
            }
        }

        auto render_widget(Widget &widget) -> void {
            const auto c = widget.background();
            const float r = widget.border_radius();
            auto *shape = tvg::Shape::gen();
            shape->appendRect(widget.x(), widget.y(),
                              widget.width(), widget.height(), r, r);
            shape->fill(c.r, c.g, c.b, c.a);
            canvas_->add(shape);
            widget.clear_dirty();
            widget.for_each_child([this](Widget &child) {
                render_widget(child);
            });
        }

        int width_ = 0;
        int height_ = 0;
        bool pressed_ = false;
        float press_x_ = 0.0f;
        float press_y_ = 0.0f;
        bool pending_click_ = false;
        float pending_click_x_ = 0.0f;
        float pending_click_y_ = 0.0f;
        std::uint8_t pending_click_btn_ = 0;

        std::array<RenderLayer, 16>* layers_ = nullptr;
        std::unique_ptr<SDL_Window, WindowDeleter> window_;
        std::unique_ptr<SDL_Renderer, RendererDeleter> renderer_;
        std::unique_ptr<SDL_Texture, TextureDeleter> texture_;
        std::vector<std::uint32_t> pixel_buffer_;
        std::unique_ptr<tvg::SwCanvas> canvas_;
    };

    NanWindow::NanWindow(std::string_view title, const int width, const int height)
        : width_(width), height_(height) {
        SDL_Window *raw_window = nullptr;
        SDL_Renderer *raw_renderer = nullptr;

        if (!SDL_CreateWindowAndRenderer(title.data(), width, height, 0, &raw_window, &raw_renderer)) {
            throw std::runtime_error(std::format("Window creation failed: {}", SDL_GetError()));
        }
        window_.reset(raw_window);
        renderer_.reset(raw_renderer);

        auto *raw_texture = SDL_CreateTexture(renderer_.get(),
                                              SDL_PIXELFORMAT_ARGB8888,
                                              SDL_TEXTUREACCESS_STREAMING,
                                              width, height);
        if (!raw_texture) {
            throw std::runtime_error(std::format("Texture creation failed: {}", SDL_GetError()));
        }
        texture_.reset(raw_texture);

        pixel_buffer_.resize(static_cast<std::size_t>(width * height));
        canvas_.reset(tvg::SwCanvas::gen());
        if (canvas_->target(pixel_buffer_.data(), width, width, height,
                            tvg::ColorSpace::ARGB8888) != tvg::Result::Success) {
            throw std::runtime_error("ThorVG canvas binding failed!");
        }
        std::println("[NanWindow] '{}' ({}x{}) initialized.", title, width, height);
    }

    // Inheritable facade: users subclass this like Qt's QMainWindow pattern.
    class WindowController {
    public:
        virtual ~WindowController() = default;

        auto exec() -> int {
            auto [w, h] = initial_size();
            auto window = NanWindow::Builder{}
                    .set_title(title())
                    .set_width(w)
                    .set_height(h)
                    .build();

            setup_layers(layers_);
            window.set_layers(layers_);
            on_start();

            while (!window.process_events()) {
                on_frame();
                window.present_frame();
            }

            on_shutdown();
            return 0;
        }

    protected:
        // ── Semantic layer accessors (mirrors Godot's conventional layer numbers)
        auto world_layer()   -> RenderLayer& { return layers_[0]; }  // FreeWidget / world canvas
        auto ui_layer()      -> RenderLayer& { return layers_[1]; }  // main UI, RouterView
        auto overlay_layer() -> RenderLayer& { return layers_[2]; }  // Popover/Toast/Dock
        auto modal_layer()   -> RenderLayer& { return layers_[3]; }  // Dialog (modal)
        auto layer(int i)    -> RenderLayer& { return layers_[i]; }  // arbitrary layer

        // Override this to configure all render layers.
        // Default: calls the legacy build_root() and places the result in ui_layer().
        virtual auto setup_layers(std::array<RenderLayer, 16>& layers) -> void {
            auto root = build_root();
            if (root) {
                layers[1].root = std::move(root);
            }
        }

        // Legacy entry-point kept for backward compatibility.
        // Override build_root() if you do not need multi-layer support.
        virtual auto build_root() -> std::unique_ptr<Widget> { return nullptr; }

        [[nodiscard]] virtual auto title() const -> std::string_view {
            return "Nandina Window";
        }

        [[nodiscard]] virtual auto initial_size() const -> std::pair<int, int> {
            return {800, 600};
        }

        virtual auto on_start()    -> void {}
        virtual auto on_frame()    -> void {}
        virtual auto on_shutdown() -> void {}

        std::array<RenderLayer, 16> layers_;
    };
}
