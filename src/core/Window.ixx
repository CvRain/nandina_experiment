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
import Nandina.Components.Label;

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
            if (layers_) {
                for (auto& layer : *layers_) {
                    if (layer.visible && layer.has_content()) {
                        render_text_overlay(*layer.root);
                    }
                }
            }
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

        auto render_text_overlay(Widget& widget) -> void {
            if (auto* label = dynamic_cast<Label*>(&widget)) {
                draw_debug_text(widget.x(), widget.y(),
                                label->get_text(),
                                label->text_r(), label->text_g(), label->text_b(), label->text_a());
            } else if (auto* button = dynamic_cast<Button*>(&widget)) {
                draw_debug_text(widget.x() + 12.0f,
                                widget.y() + (widget.height() * 0.5f) - 4.0f,
                                button->get_text(),
                                255, 255, 255, 255);
            }

            widget.for_each_child([this](Widget& child) {
                render_text_overlay(child);
            });
        }

        auto draw_debug_text(float x, float y, const std::string& text,
                             std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) -> void {
            if (text.empty()) {
                return;
            }
            SDL_SetRenderDrawColor(renderer_.get(), r, g, b, a);
            SDL_RenderDebugText(renderer_.get(), x, y, text.c_str());
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
            window_width_ = w;
            window_height_ = h;
            auto window = NanWindow::Builder{}
                    .set_title(title())
                    .set_width(w)
                    .set_height(h)
                    .build();

            reset_layers();
            setup();
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
        virtual auto setup() -> void {
            setup_layers(layers_);
        }

        // ── Semantic layer accessors (mirrors Godot's conventional layer numbers)
        auto world_layer()   -> RenderLayer& { return layers_[0]; }  // FreeWidget / world canvas
        auto ui_layer()      -> RenderLayer& { return layers_[1]; }  // main UI, RouterView
        auto overlay_layer() -> RenderLayer& { return layers_[2]; }  // Popover/Toast/Dock
        auto modal_layer()   -> RenderLayer& { return layers_[3]; }  // Dialog (modal)
        auto layer(int i)    -> RenderLayer& { return layers_[i]; }  // arbitrary layer

        [[nodiscard]] auto window_width() const noexcept -> int { return window_width_; }
        [[nodiscard]] auto window_height() const noexcept -> int { return window_height_; }

        auto set_background(std::unique_ptr<Widget> widget) -> Widget& {
            return set_layer_root(0, std::move(widget));
        }

        auto set_content(std::unique_ptr<Widget> widget) -> Widget& {
            return set_layer_root(1, std::move(widget));
        }

        auto set_overlay(std::unique_ptr<Widget> widget) -> Widget& {
            return set_layer_root(2, std::move(widget));
        }

        auto add_child(std::unique_ptr<Widget> widget) -> Widget& {
            const int target_layer = normalize_layer_index(widget ? widget->layer() : 1);
            return add_to_layer(target_layer, std::move(widget));
        }

        auto add_to_layer(int index, std::unique_ptr<Widget> widget) -> Widget& {
            auto* mounted = layers_[normalize_layer_index(index)].add(std::move(widget));
            return *mounted;
        }

        auto set_background_color(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                                  std::uint8_t a = 255) -> Widget& {
            auto bg = std::make_unique<FreeWidget>();
            bg->move_to(0.0f, 0.0f)
              .resize(static_cast<float>(window_width_), static_cast<float>(window_height_))
              .set_background(r, g, b, a);
            return set_background(std::move(bg));
        }

        auto set_layer_visible(int index, bool visible) -> void {
            layers_[normalize_layer_index(index)].visible = visible;
        }

        auto set_layer_modal(int index, bool modal) -> void {
            layers_[normalize_layer_index(index)].modal = modal;
        }

        // Override this to configure all render layers.
        // Default: calls the legacy build_root() and places the result in ui_layer().
        virtual auto setup_layers(std::array<RenderLayer, 16>&) -> void {
            auto root = build_root();
            if (root) {
                set_content(std::move(root));
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

    private:
        static constexpr int default_content_layer_ = 1;

        auto reset_layers() -> void {
            for (std::size_t i = 0; i < layers_.size(); ++i) {
                layers_[i].index = static_cast<int>(i);
                layers_[i].modal = false;
                layers_[i].visible = true;
                layers_[i].root.reset();
            }
        }

        [[nodiscard]] static auto normalize_layer_index(int index) noexcept -> int {
            if (index < 0 || index >= 16) {
                return default_content_layer_;
            }
            return index;
        }

        auto set_layer_root(int index, std::unique_ptr<Widget> widget) -> Widget& {
            auto& target = layers_[normalize_layer_index(index)];
            target.root = std::move(widget);
            return *target.root;
        }

        int window_width_ = 800;
        int window_height_ = 600;
    };
}
