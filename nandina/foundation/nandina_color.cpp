//
// Created by cvrain on 2026/6/27.
//

#include "nandina_color.hpp"

#include <cmath>

namespace nandina::foundation
{
    constexpr auto NanColor::oklch() const -> NanOklch {
        return this->color_;
    }

    constexpr auto NanColor::alpha() const -> float {
        return color_.alpha;
    }

    auto NanColor::with_alpha(const float alpha) const -> NanColor {
        return NanColor {{
            color_.light,
            color_.chroma,
            color_.hue,
            nan_clamp01(alpha),
        }};
    }

    auto NanColor::lighten(const float amount) const -> NanColor {
        return NanColor {{
            nan_clamp01(color_.light + amount),
            color_.chroma,
            color_.hue,
            color_.alpha,
        }};
    }

    auto NanColor::darken(const float amount) const -> NanColor {
        return lighten(-amount);
    }

    auto NanColor::saturate(const float amount) const -> NanColor {
        return NanColor {{
            color_.light,
            std::max(0.0F, color_.chroma + amount),
            color_.hue,
            color_.alpha,
        }};
    }

    auto NanColor::desaturate(const float amount) const -> NanColor {
        return saturate(-amount);
    }

    auto NanColor::rotate_hue(const float degrees) const -> NanColor {
        return NanColor {{
            color_.light,
            color_.chroma,
            nan_normalize_degrees(color_.hue + degrees),
            color_.alpha,
        }};
    }

    auto NanColor::clamped() const -> NanColor {
        return NanColor {NanColorConverter<NanOklch>::to_oklch(color_)};
    }

    auto NanColor::mix(const NanColor& other, float amount) const -> NanColor {
        const auto clamped_amount = nan_clamp01(amount);
        return NanColor {{
            nan_lerp(color_.light, other.color_.light, clamped_amount),
            nan_lerp(color_.chroma, other.color_.chroma, clamped_amount),
            nan_lerp(color_.hue, other.color_.hue, clamped_amount),
            nan_lerp(color_.alpha, other.color_.alpha, clamped_amount),
        }};
    }
} // namespace nandina::foundation