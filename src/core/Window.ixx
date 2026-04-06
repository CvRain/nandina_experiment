module;

#ifdef NANDINA_TEXT_ENGINE
#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb-ft.h>
#include <hb.h>
#endif

#include <SDL3/SDL.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <filesystem>
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

        ~NanWindow() {
    #ifdef NANDINA_TEXT_ENGINE
            shutdown_text_engine();
    #endif
        }

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

#ifdef NANDINA_TEXT_ENGINE
            if (layers_) {
                for (auto& layer : *layers_) {
                    if (layer.visible && layer.has_content()) {
                        render_text(layer.root.get());
                    }
                }
            }
#endif

            SDL_UpdateTexture(texture_.get(), nullptr, pixel_buffer_.data(),
                              width_ * static_cast<int>(sizeof(std::uint32_t)));
            SDL_RenderClear(renderer_.get());
            SDL_RenderTexture(renderer_.get(), texture_.get(), nullptr, nullptr);
#ifndef NANDINA_TEXT_ENGINE
            if (layers_) {
                for (auto& layer : *layers_) {
                    if (layer.visible && layer.has_content()) {
                        render_debug_text_overlay(layer.root.get());
                    }
                }
            }
#endif
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

#ifdef NANDINA_TEXT_ENGINE
        static auto find_default_font() -> std::string {
            constexpr std::array<std::string_view, 5> candidates{
                "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                "/usr/share/fonts/dejavu/DejaVuSans.ttf",
                "/usr/share/fonts/TTF/DejaVuSans.ttf",
                "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
                "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
            };

            for (const auto candidate : candidates) {
                if (std::filesystem::exists(candidate)) {
                    return std::string(candidate);
                }
            }

            return {};
        }

        auto initialize_text_engine() -> void {
            if (FT_Init_FreeType(&ft_library_) != 0) {
                throw std::runtime_error("FreeType initialization failed");
            }

            font_path_ = find_default_font();
            if (font_path_.empty()) {
                throw std::runtime_error("No default font found for NANDINA_TEXT_ENGINE");
            }

            if (FT_New_Face(ft_library_, font_path_.c_str(), 0, &ft_face_) != 0) {
                throw std::runtime_error(std::format("Unable to load font: {}", font_path_));
            }

            hb_font_ = hb_ft_font_create_referenced(ft_face_);
        }

        auto shutdown_text_engine() noexcept -> void {
            if (hb_font_) {
                hb_font_destroy(hb_font_);
                hb_font_ = nullptr;
            }
            if (ft_face_) {
                FT_Done_Face(ft_face_);
                ft_face_ = nullptr;
            }
            if (ft_library_) {
                FT_Done_FreeType(ft_library_);
                ft_library_ = nullptr;
            }
        }

        auto set_font_size(float font_size) -> void {
            const auto pixel_size = static_cast<unsigned int>(font_size < 1.0f ? 1.0f : font_size);
            if (current_font_size_ == pixel_size) {
                return;
            }

            FT_Set_Pixel_Sizes(ft_face_, 0, pixel_size);
            hb_ft_font_changed(hb_font_);
            current_font_size_ = pixel_size;
        }

        [[nodiscard]] auto blend_channel(std::uint8_t src, std::uint8_t dst, std::uint8_t src_alpha) const noexcept -> std::uint8_t {
            const auto value = (static_cast<unsigned int>(src) * src_alpha)
                             + (static_cast<unsigned int>(dst) * (255 - src_alpha));
            return static_cast<std::uint8_t>(value / 255);
        }

        auto blend_pixel(int x, int y, const Color& color, std::uint8_t coverage) -> void {
            if (x < 0 || y < 0 || x >= width_ || y >= height_ || coverage == 0) {
                return;
            }

            const auto src_alpha = static_cast<std::uint8_t>((static_cast<unsigned int>(color.a) * coverage) / 255);
            if (src_alpha == 0) {
                return;
            }

            auto& pixel = pixel_buffer_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x)];
            const auto dst_a = static_cast<std::uint8_t>((pixel >> 24) & 0xFFu);
            const auto dst_r = static_cast<std::uint8_t>((pixel >> 16) & 0xFFu);
            const auto dst_g = static_cast<std::uint8_t>((pixel >> 8) & 0xFFu);
            const auto dst_b = static_cast<std::uint8_t>(pixel & 0xFFu);

            const auto out_a = static_cast<std::uint8_t>(src_alpha + ((static_cast<unsigned int>(dst_a) * (255 - src_alpha)) / 255));
            const auto out_r = blend_channel(color.r, dst_r, src_alpha);
            const auto out_g = blend_channel(color.g, dst_g, src_alpha);
            const auto out_b = blend_channel(color.b, dst_b, src_alpha);

            pixel = (static_cast<std::uint32_t>(out_a) << 24)
                  | (static_cast<std::uint32_t>(out_r) << 16)
                  | (static_cast<std::uint32_t>(out_g) << 8)
                  | static_cast<std::uint32_t>(out_b);
        }

        auto render_text(Widget* widget) -> void {
            if (!widget) {
                return;
            }

            const auto text = widget->text_content();
            const auto font_size = widget->text_font_size();
            if (!text.empty() && font_size > 0.0f) {
                set_font_size(font_size);

                auto* buffer = hb_buffer_create();
                hb_buffer_add_utf8(buffer, text.data(), static_cast<int>(text.size()), 0, static_cast<int>(text.size()));
                hb_buffer_guess_segment_properties(buffer);
                hb_shape(hb_font_, buffer, nullptr, 0);

                unsigned int glyph_count = 0;
                auto* glyph_info = hb_buffer_get_glyph_infos(buffer, &glyph_count);
                auto* glyph_pos = hb_buffer_get_glyph_positions(buffer, &glyph_count);

                const auto bounds = widget->text_bounds();
                const auto metrics = ft_face_->size->metrics;
                const float ascender = static_cast<float>(metrics.ascender) / 64.0f;
                const float descender = static_cast<float>(metrics.descender) / 64.0f;
                const float line_height = ascender - descender;

                float total_advance = 0.0f;
                for (unsigned int index = 0; index < glyph_count; ++index) {
                    total_advance += static_cast<float>(glyph_pos[index].x_advance) / 64.0f;
                }

                float pen_x = bounds.x();
                if (widget->text_align() == TextAlign::center) {
                    pen_x = bounds.x() + ((bounds.width() - total_advance) * 0.5f);
                }

                const float baseline = bounds.y() + ((bounds.height() - line_height) * 0.5f) + ascender;
                const auto color = widget->text_color();

                for (unsigned int index = 0; index < glyph_count; ++index) {
                    pen_x += static_cast<float>(glyph_pos[index].x_offset) / 64.0f;
                    const float pen_y = baseline - (static_cast<float>(glyph_pos[index].y_offset) / 64.0f);

                    if (FT_Load_Glyph(ft_face_, glyph_info[index].codepoint, FT_LOAD_RENDER) == 0) {
                        const auto* glyph = ft_face_->glyph;
                        const auto& bitmap = glyph->bitmap;
                        const int bitmap_x = static_cast<int>(std::lround(pen_x)) + glyph->bitmap_left;
                        const int bitmap_y = static_cast<int>(std::lround(pen_y)) - glyph->bitmap_top;

                        for (unsigned int row = 0; row < bitmap.rows; ++row) {
                            for (unsigned int column = 0; column < bitmap.width; ++column) {
                                const auto coverage = bitmap.buffer[row * bitmap.pitch + column];
                                blend_pixel(bitmap_x + static_cast<int>(column),
                                            bitmap_y + static_cast<int>(row),
                                            color,
                                            coverage);
                            }
                        }
                    }

                    pen_x += static_cast<float>(glyph_pos[index].x_advance) / 64.0f;
                }

                hb_buffer_destroy(buffer);
            }

            widget->for_each_child([this](Widget& child) {
                render_text(&child);
            });
        }
#endif

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

        auto render_debug_text_overlay(Widget* widget) -> void {
            if (!widget) {
                return;
            }

            const auto text = widget->text_content();
            const auto font_size = widget->text_font_size();
            if (!text.empty() && font_size > 0.0f) {
                const auto bounds = widget->text_bounds();
                const auto color = widget->text_color();
                float draw_x = bounds.x();
                if (widget->text_align() == TextAlign::center) {
                    draw_x = bounds.center_x() - (static_cast<float>(text.size()) * 4.0f);
                }
                const float draw_y = bounds.center_y() - 4.0f;
                draw_debug_text(draw_x, draw_y, std::string(text), color.r, color.g, color.b, color.a);
            }

            widget->for_each_child([this](Widget& child) {
                render_debug_text_overlay(&child);
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
    #ifdef NANDINA_TEXT_ENGINE
        FT_Library ft_library_ = nullptr;
        FT_Face ft_face_ = nullptr;
        hb_font_t* hb_font_ = nullptr;
        unsigned int current_font_size_ = 0;
        std::string font_path_;
    #endif
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
    #ifdef NANDINA_TEXT_ENGINE
        initialize_text_engine();
    #endif
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
