module;

#include <cmath>

export module Nandina.Types.Position;

export namespace Nandina {
    class Position {
    public:
        constexpr Position(float pos_x = 0.0f, float pos_y = 0.0f, float pos_z = 0.0f) noexcept;

        constexpr auto operator==(const Position &other) const noexcept -> bool = default;

        auto set_x(float value) noexcept -> Position&;

        auto set_y(float value) noexcept -> Position&;

        auto set_z(float value) noexcept -> Position&;

        [[nodiscard]] auto x() const noexcept -> float;

        [[nodiscard]] auto y() const noexcept -> float;

        [[nodiscard]] auto z() const noexcept -> float;

        [[nodiscard]] auto distance_to_origin() const noexcept -> float;

    private:
        float x_ = 0.0f;
        float y_ = 0.0f;
        float z_ = 0.0f;
    };

    constexpr Position::Position(const float pos_x, const float pos_y, const float pos_z) noexcept
        : x_(pos_x), y_(pos_y), z_(pos_z) {
    }

    auto Position::set_x(const float value) noexcept -> Position& {
        x_ = value;
        return *this;
    }

    auto Position::set_y(const float value) noexcept -> Position& {
        y_ = value;
        return *this;
    }

    auto Position::set_z(const float value) noexcept -> Position& {
        z_ = value;
        return *this;
    }

    auto Position::x() const noexcept -> float {
        return x_;
    }

    auto Position::y() const noexcept -> float {
        return y_;
    }

    auto Position::z() const noexcept -> float {
        return z_;
    }

    auto Position::distance_to_origin() const noexcept -> float {
        return std::sqrt(x_ * x_ + y_ * y_ + z_ * z_);
    }
}