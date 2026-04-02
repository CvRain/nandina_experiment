module;

#include <cstdint>

export module Nandina.Core.Event;

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
}