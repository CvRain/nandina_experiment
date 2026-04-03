module;

#include <algorithm>

export module Nandina.Types.Size;

export namespace Nandina {
    class Size {
    public:
        constexpr Size(float width = 0.0f, float height = 0.0f) noexcept;

        constexpr auto operator==(const Size &other) const noexcept -> bool = default;

        auto set_width(float value) noexcept -> Size&;

        auto set_height(float value) noexcept -> Size&;

        [[nodiscard]] auto width() const noexcept -> float;

        [[nodiscard]] auto height() const noexcept -> float;

        [[nodiscard]] auto area() const noexcept -> float;

        [[nodiscard]] auto is_empty() const noexcept -> bool;

        [[nodiscard]] auto bounded_to(const Size &other) const noexcept -> Size;

        [[nodiscard]] auto expanded_to(const Size &other) const noexcept -> Size;

        // ── Semantic factory methods ──────────────────────────────────────────
        // Returns a Size that signals "fill the parent container" (both axes -1).
        [[nodiscard]] static auto fill() noexcept -> Size { return {-1.0f, -1.0f}; }

        // Returns a Size with explicit width and height.
        [[nodiscard]] static auto fixed(float w, float h) noexcept -> Size { return {w, h}; }

        // Returns a square Size with equal width and height.
        [[nodiscard]] static auto square(float s) noexcept -> Size { return {s, s}; }

        // Returns true if this Size represents a "fill parent" marker.
        [[nodiscard]] auto is_fill() const noexcept -> bool {
            return width_ < 0.0f || height_ < 0.0f;
        }

    private:
        float width_ = 0.0f;
        float height_ = 0.0f;
    };

    constexpr Size::Size(const float width, const float height) noexcept
        : width_(width), height_(height) {
    }

    auto Size::set_width(const float value) noexcept -> Size& {
        width_ = value;
        return *this;
    }

    auto Size::set_height(const float value) noexcept -> Size& {
        height_ = value;
        return *this;
    }

    auto Size::width() const noexcept -> float {
        return width_;
    }

    auto Size::height() const noexcept -> float {
        return height_;
    }

    auto Size::area() const noexcept -> float {
        return width_ * height_;
    }

    auto Size::is_empty() const noexcept -> bool {
        return width_ <= 0.0f || height_ <= 0.0f;
    }

    auto Size::bounded_to(const Size &other) const noexcept -> Size {
        return Size(std::min(width_, other.width_), std::min(height_, other.height_));
    }

    auto Size::expanded_to(const Size &other) const noexcept -> Size {
        return Size(std::max(width_, other.width_), std::max(height_, other.height_));
    }
}