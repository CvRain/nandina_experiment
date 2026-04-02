module;

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

export module Nandina.Core;

export namespace Nandina {
    enum class EventType : std::uint8_t {
        none,
        mouse_move,
        mouse_button_press,
        mouse_button_release,
        click,
    };

    enum class MouseButton : std::uint8_t {
        none,
        left,
        middle,
        right,
    };

    struct Event {
        EventType type = EventType::none;
        MouseButton button = MouseButton::none;
        float x = 0.0f;
        float y = 0.0f;
        bool handled = false;

        auto mark_handled() noexcept -> void {
            handled = true;
        }
    };

    struct Color {
        std::uint8_t r = 0;
        std::uint8_t g = 0;
        std::uint8_t b = 0;
        std::uint8_t a = 255;
    };

    class Connection {
    public:
        Connection() = default;

        Connection(std::function<void()> disconnect_fn, std::function<bool()> connected_fn)
            : disconnect_fn_(std::move(disconnect_fn)), connected_fn_(std::move(connected_fn)) {
        }

        Connection(const Connection &) = delete;

        auto operator=(const Connection &) -> Connection& = delete;

        Connection(Connection &&) noexcept = default;

        auto operator=(Connection &&) noexcept -> Connection& = default;

        ~Connection() {
            disconnect();
        }

        auto disconnect() -> void {
            if (disconnect_fn_) {
                disconnect_fn_();
                disconnect_fn_ = {};
                connected_fn_ = {};
            }
        }

        [[nodiscard]] auto connected() const -> bool {
            return connected_fn_ && connected_fn_();
        }

    private:
        std::function<void()> disconnect_fn_;
        std::function<bool()> connected_fn_;
    };

    template<typename... Args>
    class Signal {
    public:
        using Slot = std::function<void(Args...)>;

        auto connect(Slot slot) -> Connection {
            const std::size_t id = next_id_++;
            slots_.push_back(SlotEntry{id, true, std::move(slot)});

            return Connection(
                [this, id]() {
                    disconnect(id);
                },
                [this, id]() {
                    return is_connected(id);
                }
            );
        }

        auto emit(Args... args) -> void {
            for (auto &entry : slots_) {
                if (!entry.active) {
                    continue;
                }
                entry.slot(args...);
            }
            cleanup();
        }

        auto clear() -> void {
            slots_.clear();
        }

    private:
        struct SlotEntry {
            std::size_t id = 0;
            bool active = false;
            Slot slot;
        };

        auto disconnect(const std::size_t id) -> void {
            for (auto &entry : slots_) {
                if (entry.id == id) {
                    entry.active = false;
                    break;
                }
            }
        }

        [[nodiscard]] auto is_connected(const std::size_t id) const -> bool {
            for (const auto &entry : slots_) {
                if (entry.id == id) {
                    return entry.active;
                }
            }
            return false;
        }

        auto cleanup() -> void {
            std::erase_if(slots_, [](const SlotEntry &entry) {
                return !entry.active;
            });
        }

        std::size_t next_id_ = 1;
        std::vector<SlotEntry> slots_;
    };

    class Widget {
    public:
        using Child = std::unique_ptr<Widget>;

        virtual ~Widget() = default;

        auto add_child(Child child) -> Widget& {
            children_.push_back(std::move(child));
            mark_dirty();
            return *this;
        }

        virtual auto set_bounds(const float x, const float y, const float width, const float height) noexcept -> Widget& {
            x_ = x;
            y_ = y;
            width_ = width;
            height_ = height;
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto contains(const float px, const float py) const noexcept -> bool {
            return px >= x_ && px < x_ + width_ && py >= y_ && py < y_ + height_;
        }

        auto dispatch_event(Event &event) -> bool {
            for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
                if ((*it)->dispatch_event(event)) {
                    return true;
                }
            }

            if (event.handled || !contains(event.x, event.y)) {
                return false;
            }

            return handle_event(event);
        }

        auto on_click(std::function<void()> handler) -> Connection {
            return clicked_.connect(std::move(handler));
        }

        [[nodiscard]] auto is_dirty() const noexcept -> bool {
            return dirty_;
        }

        auto mark_dirty() noexcept -> void {
            dirty_ = true;
        }

        auto clear_dirty() noexcept -> void {
            dirty_ = false;
        }

        [[nodiscard]] auto x()      const noexcept -> float { return x_; }
        [[nodiscard]] auto y()      const noexcept -> float { return y_; }
        [[nodiscard]] auto width()  const noexcept -> float { return width_; }
        [[nodiscard]] auto height() const noexcept -> float { return height_; }

        [[nodiscard]] auto background()    const noexcept -> Color { return bg_color_; }
        [[nodiscard]] auto border_radius() const noexcept -> float { return border_radius_; }

        auto set_background(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) noexcept -> Widget& {
            bg_color_ = {r, g, b, a};
            mark_dirty();
            return *this;
        }

        auto set_border_radius(const float r) noexcept -> Widget& {
            border_radius_ = r;
            mark_dirty();
            return *this;
        }

        auto for_each_child(std::function<void(Widget &)> fn) const -> void {
            for (const auto &child : children_) {
                fn(*child);
            }
        }

    protected:
        virtual auto handle_event(Event &event) -> bool {
            if (event.type == EventType::click) {
                clicked_.emit();
                event.mark_handled();
                return true;
            }
            return false;
        }

        [[nodiscard]] auto children() const noexcept -> const std::vector<Child>& {
            return children_;
        }

    private:
        float x_ = 0.0f;
        float y_ = 0.0f;
        float width_ = 0.0f;
        float height_ = 0.0f;
        bool dirty_ = true;
        std::vector<Child> children_;
        Signal<> clicked_;
        Color bg_color_{255, 255, 255, 255};
        float border_radius_ = 0.0f;
    };

    class Component : public Widget {
    public:
        using Ptr = std::unique_ptr<Component>;
        virtual ~Component() = default;
    };

    class RectangleComponent final : public Component {
    public:
        static auto Create() -> std::unique_ptr<RectangleComponent> {
            return std::make_unique<RectangleComponent>();
        }

        auto color(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) -> RectangleComponent& {
            set_background(r, g, b, a);
            return *this;
        }

        auto radius(float r) -> RectangleComponent& {
            set_border_radius(r);
            return *this;
        }
    };

    class FocusComponent final : public Component {
    public:
        static auto Create() -> std::unique_ptr<FocusComponent> {
            return std::make_unique<FocusComponent>();
        }

        auto focused(bool value) -> FocusComponent& {
            focused_ = value;
            if (focused_) {
                set_background(59, 130, 246, 90);
            } else {
                set_background(0, 0, 0, 0);
            }
            return *this;
        }

    private:
        bool focused_ = false;
    };

    class Button final : public Component {
    public:
        static auto Create() -> std::unique_ptr<Button> {
            return std::unique_ptr<Button>(new Button());
        }

        auto text(std::string value) -> Button& {
            text_ = std::move(value);
            return *this;
        }

        auto set_bounds(const float x, const float y, const float width, const float height) noexcept -> Widget& override {
            Component::set_bounds(x, y, width, height);
            if (focus_layer_) {
                focus_layer_->set_bounds(x - 2.0f, y - 2.0f, width + 4.0f, height + 4.0f);
            }
            if (rect_layer_) {
                rect_layer_->set_bounds(x, y, width, height);
            }
            return *this;
        }

    protected:
        auto handle_event(Event &event) -> bool override {
            if (event.type == EventType::mouse_button_press && contains(event.x, event.y)) {
                pressed_ = true;
                if (rect_layer_) {
                    rect_layer_->color(29, 78, 216);
                }
                event.mark_handled();
                return true;
            }

            if (event.type == EventType::mouse_button_release && pressed_) {
                pressed_ = false;
                if (rect_layer_) {
                    rect_layer_->color(37, 99, 235);
                }
                event.mark_handled();
                return true;
            }

            if (event.type == EventType::click && contains(event.x, event.y)) {
                if (focus_layer_) {
                    focus_layer_->focused(true);
                }
                return Component::handle_event(event);
            }

            return false;
        }

    private:
        Button() {
            set_background(0, 0, 0, 0);

            auto focus = FocusComponent::Create();
            focus_layer_ = focus.get();
            focus_layer_->focused(false);
            add_child(std::move(focus));

            auto rect = RectangleComponent::Create();
            rect_layer_ = rect.get();
            rect_layer_->color(37, 99, 235).radius(8.0f);
            add_child(std::move(rect));
        }

        std::string text_;
        bool pressed_ = false;
        FocusComponent *focus_layer_ = nullptr;
        RectangleComponent *rect_layer_ = nullptr;
    };
}