module;
#include <SDL3/SDL.h>
#include <string>
#include <string_view>
#include <memory>
#include <stdexcept>
#include <format>
#include <print>
#include <thorvg-1/thorvg.h>

export module Nandina.Window;

import Nandina.Types; // 假设 NanRect 在这里

export namespace Nandina {
    class NanWindow {
    public:
        // C++ 智能指针定制删除器
        struct WindowDeleter {
            void operator()(SDL_Window *w) const noexcept { if (w) SDL_DestroyWindow(w); }
        };

        struct RendererDeleter {
            void operator()(SDL_Renderer *r) const noexcept { if (r) SDL_DestroyRenderer(r); }
        };

        struct TextureDeleter {
            void operator()(SDL_Texture *t) const noexcept { if (t) SDL_DestroyTexture(t); }
        };

        // 内部类：构建者模式
        class Builder {
        public:
            [[nodiscard]] auto set_title(std::string_view title) -> Builder&;

            [[nodiscard]] auto set_width(int width) -> Builder&;

            [[nodiscard]] auto set_height(int height) -> Builder&;

            [[nodiscard]] auto build() const -> NanWindow;

        private:
            std::string title_ = "Nandina Window"; // 使用 string 存储，防止 string_view 悬空
            int width_ = 800;
            int height_ = 600;
        };

        // --- NanWindow 的主体接口 ---

        // 禁用拷贝 (防止指针共享灾难)
        NanWindow(const NanWindow &) = delete;

        auto operator=(const NanWindow &) -> NanWindow& = delete;

        // 启用默认移动语义 (unique_ptr 支持移动，完美转移所有权)
        NanWindow(NanWindow &&) noexcept = default;

        auto operator=(NanWindow &&) noexcept -> NanWindow& = default;

        void run() const;

    private:
        // 私有构造函数，只允许 Builder 调用
        NanWindow(std::string_view title, int width, int height);

        int width_ = 0;
        int height_ = 0;

        std::unique_ptr<SDL_Window, WindowDeleter> window_;
        std::unique_ptr<SDL_Renderer, RendererDeleter> renderer_;
        std::unique_ptr<SDL_Texture, TextureDeleter> texture_;

        std::vector<uint32_t> pixel_buffer_;
        std::unique_ptr<tvg::SwCanvas> canvas_;
    };

    auto NanWindow::Builder::set_title(std::string_view title) -> Builder& {
        title_ = title;
        return *this;
    }

    auto NanWindow::Builder::set_width(const int width) -> Builder& {
        width_ = width;
        return *this;
    }

    auto NanWindow::Builder::set_height(const int height) -> Builder& {
        height_ = height;
        return *this;
    }

    auto NanWindow::Builder::build() const -> NanWindow {
        return {title_, width_, height_};
    }

    void NanWindow::run() const {
        SDL_Event event{};
        bool keep_running = true;

        while (keep_running) {
            if (SDL_WaitEventTimeout(&event, 16)) {
                if (event.type == SDL_EVENT_QUIT) {
                    keep_running = false;
                }
            }

            //window background
            const auto shape = tvg::Shape::gen();
            shape->appendRect(0, 0, width_, height_, 0, 0); // x, y, w, h, rx, ry
            shape->fill(255, 255, 255, 255);
            shape->strokeJoin(tvg::StrokeJoin::Round);

            // 将图形推入画布并执行绘制
            canvas_->add(shape);
            canvas_->draw();
            if (canvas_->draw() == tvg::Result::Success) {
                canvas_->sync(); // 必须同步，确保像素已经写入 pixel_buffer_
            }

            // ================= 2. SDL 渲染阶段 =================
            // 将内存中的像素数据更新到 SDL_Texture 中
            SDL_UpdateTexture(texture_.get(), nullptr, pixel_buffer_.data(), width_ * sizeof(uint32_t));

            SDL_RenderClear(renderer_.get());
            SDL_RenderTexture(renderer_.get(), texture_.get(), nullptr, nullptr);
            SDL_RenderPresent(renderer_.get());


        }
    }

    NanWindow::NanWindow(std::string_view title, const int width, const int height)
        : width_(width), height_(height) {
        SDL_Window *raw_window = nullptr;
        SDL_Renderer *raw_renderer = nullptr;

        if (not SDL_CreateWindowAndRenderer(title.data(), width, height, 0, &raw_window, &raw_renderer)) {
            throw std::runtime_error(std::format("Window creation failed: {}", SDL_GetError()));
        }

        // 移交所有权给智能指针，从此无需操心内存泄漏
        window_.reset(raw_window);
        renderer_.reset(raw_renderer);

        // 创建桥梁 Texture：格式为 ARGB8888，流模式（允许频繁更新像素）
        SDL_Texture *raw_texture = SDL_CreateTexture(
            renderer_.get(),
            SDL_PIXELFORMAT_ARGB8888, // SDL 使用 ARGB
            SDL_TEXTUREACCESS_STREAMING,
            width, height);
        texture_.reset(raw_texture);

        // 初始化像素缓冲区并绑定给 ThorVG
        pixel_buffer_.resize(width * height);
        canvas_.reset(tvg::SwCanvas::gen());

        // 核心绑定：告诉 ThorVG 将画面绘制到我们的 pixel_buffer_ 中
        if (canvas_->target(pixel_buffer_.data(), width, width, height, tvg::ColorSpace::ARGB8888) !=
            tvg::Result::Success) {
            throw std::runtime_error("ThorVG Canvas target binding failed!");
        }

        std::println("Window '{}' initialized with ThorVG engine.", title);
    }
}
