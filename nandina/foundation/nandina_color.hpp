//
// Created by cvrain on 2026/6/27.
//

#ifndef NANDINA_EXPERIMENT_NANDINA_COLOR_HPP
#define NANDINA_EXPERIMENT_NANDINA_COLOR_HPP

#include "nandina_color_space.hpp"

namespace nandina::foundation
{
    class NanColor {
    public:
        constexpr NanColor() = default;

        explicit NanColor(const NanOklch& color):
            color_ {
                nan_clamp01(color.light),
                std::max(0.0F, color.chroma),
                nan_normalize_degrees(color.hue),
                nan_clamp01(color.alpha),
            } {}

        template<NanColorSpace ColorSpace>
        [[nodiscard]] static auto from(const ColorSpace& color) -> NanColor {
            return NanColor {NanColorConverter<std::remove_cvref_t<ColorSpace>>::to_oklch(color)};
        }

        template<NanColorSpace ColorSpace>
        [[nodiscard]] auto to() const -> ColorSpace {
            return NanColorConverter<std::remove_cvref_t<ColorSpace>>::from_oklch(color_);
        }

        [[nodiscard]] constexpr auto oklch() const -> NanOklch;

        [[nodiscard]] constexpr auto alpha() const -> float;

        [[nodiscard]] auto with_alpha(float alpha) const -> NanColor;

        [[nodiscard]] auto lighten(float amount) const -> NanColor;

        [[nodiscard]] auto darken(float amount) const -> NanColor;

        [[nodiscard]] auto saturate(float amount) const -> NanColor;

        [[nodiscard]] auto desaturate(float amount) const -> NanColor;

        [[nodiscard]] auto rotate_hue(float degrees) const -> NanColor;

        [[nodiscard]] auto clamped() const -> NanColor;

        [[nodiscard]] auto mix(const NanColor& other, float amount) const -> NanColor;

    private:
        NanOklch color_ {};
    };
} // namespace nandina::foundation

#endif // NANDINA_EXPERIMENT_NANDINA_COLOR_HPP
