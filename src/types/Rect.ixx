module;

export module Nandina.Types.Rect;

export namespace Nandina {
    class Rect {
    public:
        constexpr Rect() noexcept = default;

        constexpr Rect(float x, float y, float width, float height) noexcept;

        constexpr auto operator==(const Rect &other) const noexcept -> bool = default;

        auto set_x(float value) noexcept -> Rect&;

        auto set_y(float value) noexcept -> Rect&;

        auto set_width(float value) noexcept -> Rect&;

        auto set_height(float value) noexcept -> Rect&;

        auto move_to(float x, float y) noexcept -> Rect&;

        auto translate(float dx, float dy) noexcept -> Rect&;

        [[nodiscard]] auto x() const noexcept -> float;

        [[nodiscard]] auto y() const noexcept -> float;

        [[nodiscard]] auto width() const noexcept -> float;

        [[nodiscard]] auto height() const noexcept -> float;

        [[nodiscard]] auto left() const noexcept -> float;

        [[nodiscard]] auto right() const noexcept -> float;

        [[nodiscard]] auto top() const noexcept -> float;

        [[nodiscard]] auto bottom() const noexcept -> float;

        [[nodiscard]] auto center_x() const noexcept -> float;

        [[nodiscard]] auto center_y() const noexcept -> float;

        [[nodiscard]] auto is_empty() const noexcept -> bool;

        [[nodiscard]] auto contains(float px, float py) const noexcept -> bool;

        [[nodiscard]] auto intersects(const Rect &other) const noexcept -> bool;

        [[nodiscard]] auto intersection(const Rect &other) const noexcept -> Rect;

    private:
        float x_ = 0.0f;
        float y_ = 0.0f;
        float width_ = 0.0f;
        float height_ = 0.0f;
    };

    constexpr Rect::Rect(const float x, const float y, const float width, const float height) noexcept
        : x_(x), y_(y), width_(width), height_(height) {
    }

    auto Rect::set_x(const float value) noexcept -> Rect& {
        x_ = value;
        return *this;
    }

    auto Rect::set_y(const float value) noexcept -> Rect& {
        y_ = value;
        return *this;
    }

    auto Rect::set_width(const float value) noexcept -> Rect& {
        width_ = value;
        return *this;
    }

    auto Rect::set_height(const float value) noexcept -> Rect& {
        height_ = value;
        return *this;
    }

    auto Rect::move_to(const float x, const float y) noexcept -> Rect& {
        x_ = x;
        y_ = y;
        return *this;
    }

    auto Rect::translate(const float dx, const float dy) noexcept -> Rect& {
        x_ += dx;
        y_ += dy;
        return *this;
    }

    auto Rect::x() const noexcept -> float {
        return x_;
    }

    auto Rect::y() const noexcept -> float {
        return y_;
    }

    auto Rect::width() const noexcept -> float {
        return width_;
    }

    auto Rect::height() const noexcept -> float {
        return height_;
    }

    auto Rect::left() const noexcept -> float {
        return x();
    }

    auto Rect::right() const noexcept -> float {
        return x() + width();
    }

    auto Rect::top() const noexcept -> float {
        return y();
    }

    auto Rect::bottom() const noexcept -> float {
        return y() + height();
    }

    auto Rect::center_x() const noexcept -> float {
        return x_ + width_ * 0.5f;
    }

    auto Rect::center_y() const noexcept -> float {
        return y_ + height_ * 0.5f;
    }

    auto Rect::is_empty() const noexcept -> bool {
        return width_ <= 0.0f || height_ <= 0.0f;
    }

    auto Rect::contains(const float px, const float py) const noexcept -> bool {
        return px >= left() && px < right() && py >= top() && py < bottom();
    }

    auto Rect::intersects(const Rect &other) const noexcept -> bool {
        if (is_empty() || other.is_empty()) {
            return false;
        }
        return !(right() <= other.left() || other.right() <= left() || bottom() <= other.top() || other.bottom() <= top());
    }

    auto Rect::intersection(const Rect &other) const noexcept -> Rect {
        const float new_left = left() > other.left() ? left() : other.left();
        const float new_top = top() > other.top() ? top() : other.top();
        const float new_right = right() < other.right() ? right() : other.right();
        const float new_bottom = bottom() < other.bottom() ? bottom() : other.bottom();

        const float new_width = new_right - new_left;
        const float new_height = new_bottom - new_top;
        if (new_width <= 0.0f || new_height <= 0.0f) {
            return Rect();
        }
        return Rect(new_left, new_top, new_width, new_height);
    }
}