module;

#include <SDL3/SDL.h>
#include <format>
#include <thorvg.h>

export module Nandian.Application;

export namespace Nandina {
    class NanApplication {
    public:
        NanApplication(const NanApplication &) = delete;

        NanApplication& operator=(const NanApplication &) = delete;

        NanApplication();

        ~NanApplication();
    };

    NanApplication::NanApplication() {
        // 初始化SDL
        if (not SDL_Init(SDL_INIT_VIDEO)) {
            throw std::runtime_error(std::format("SDL_Init failed: {}", SDL_GetError()));
        }

        //初始化Thorvg
        if (tvg::Initializer::init(4) != tvg::Result::Success) {
            throw std::runtime_error("ThorVG Initialization failed!");
        }
    }

    NanApplication::~NanApplication() {
        tvg::Initializer::term();
        SDL_Quit();
    }
}
