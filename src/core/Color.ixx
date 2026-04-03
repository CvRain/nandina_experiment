module;

#include <cstdint>

export module Nandina.Core.Color;

export namespace Nandina {
    struct Color {
        std::uint8_t r = 0;
        std::uint8_t g = 0;
        std::uint8_t b = 0;
        std::uint8_t a = 255;

        constexpr auto operator==(const Color&) const noexcept -> bool = default;
    };
}
