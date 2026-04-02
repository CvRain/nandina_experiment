module;

export module Nandina.Types;

export namespace Nandina {
    class NanPos {
    public:
        explicit NanPos(float pos_x, float pos_y);

        explicit NanPos(const NanPos &pos);

        auto operator=(const NanPos &pos) -> NanPos&;

        auto set_pos_x(int x) -> NanPos&;

        auto set_pos_y(int y) -> NanPos&;

        [[nodiscard]] auto get_pos_x() const -> float;

        [[nodiscard]] auto get_pos_y() const -> float;

    private:
        float x = 0;
        float y = 0;
    };

    class NanSize {
    public:
    private:
        float width = 0;
        float height = 0;
    };

    class NanRect {
    public:
        explicit NanRect(int x = 0, int y = 0, int w = 0, int h = 0);

        auto operator=(const NanRect &rect) -> NanRect&;

        NanRect(const NanRect &rect);

        auto set_pos_x(int x) -> NanRect&;

        auto set_pos_y(int y) -> NanRect&;

        auto set_width(int w) -> NanRect&;

        auto set_height(int h) -> NanRect&;

        [[nodiscard]] auto get_pos_x() const -> int;

        [[nodiscard]] auto get_pos_y() const -> int;

        [[nodiscard]] auto get_width() const -> int;

        [[nodiscard]] auto get_height() const -> int;

    private:
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
    };


    NanPos::NanPos(const float pos_x, const float pos_y)
        : x(pos_x), y(pos_y) {
    }

    NanPos::NanPos(const NanPos &pos)
        : x(pos.get_pos_x()), y(pos.get_pos_y()) {
    }

    auto NanPos::operator=(const NanPos &pos) -> NanPos& {
        this->x = pos.get_pos_x();
        this->y = pos.get_pos_y();
        return *this;
    }

    auto NanPos::set_pos_x(const int x) -> NanPos & {
        this->x = x;
        return *this;
    }

    auto NanPos::set_pos_y(const int y) -> NanPos & {
        this->y = y;
        return *this;
    }


    auto NanPos::get_pos_x() const -> float {
        return this->x;
    }

    auto NanPos::get_pos_y() const -> float {
        return this->y;
    }

    NanRect::NanRect(const int x, const int y, const int w, const int h)
        : x(x), y(y), w(w), h(h) {
    }

    auto NanRect::operator=(const NanRect &rect) -> NanRect& = default;

    NanRect::NanRect(const NanRect &rect)
        : x(rect.get_pos_x()), y(rect.get_pos_y()), w(rect.get_width()), h(rect.get_height()) {
    }

    auto NanRect::set_pos_x(const int x) -> NanRect& {
        this->x = x;
        return *this;
    }

    auto NanRect::set_pos_y(const int y) -> NanRect& {
        this->y = y;
        return *this;
    }

    auto NanRect::set_width(const int w) -> NanRect& {
        this->w = w;
        return *this;
    }

    auto NanRect::set_height(const int h) -> NanRect& {
        this->h = h;
        return *this;
    }

    auto NanRect::get_pos_x() const -> int {
        return this->x;
    }

    auto NanRect::get_pos_y() const -> int {
        return this->y;
    }

    auto NanRect::get_width() const -> int {
        return this->w;
    }

    auto NanRect::get_height() const -> int {
        return this->h;
    }
}
