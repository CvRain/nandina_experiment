//
// Created by cvrain on 2026/7/1.
//

#ifndef NANDINA_EXPERIMENT_INPUT_EVENT_HPP
#define NANDINA_EXPERIMENT_INPUT_EVENT_HPP

#include "../foundation/geometry.hpp"

namespace nandina::scene
{

/// Base class for all input events.
class InputEvent {
public:
    virtual ~InputEvent() = default;

    /// True if a node has consumed this event (stops bubbling).
    [[nodiscard]] auto is_accepted() const -> bool {
        return accepted_;
    }

    /// Mark this event as consumed (no further propagation).
    void accept() {
        accepted_ = true;
    }

private:
    bool accepted_ = false;
};

/// Base class for pointer events that carry a screen-space position.
class MouseEvent : public InputEvent {
public:
    explicit MouseEvent(foundation::NanPoint screen_pos)
        : screen_pos_(screen_pos) {}

    [[nodiscard]] auto screen_pos() const -> foundation::NanPoint {
        return screen_pos_;
    }

private:
    foundation::NanPoint screen_pos_;
};

/// Mouse button press or release.
class MouseButtonEvent final : public MouseEvent {
public:
    enum class Button { left, middle, right };
    enum class Action { press, release };

    MouseButtonEvent(Button button, Action action, foundation::NanPoint screen_pos)
        : MouseEvent(screen_pos), button_(button), action_(action) {}

    [[nodiscard]] auto button() const -> Button {
        return button_;
    }

    [[nodiscard]] auto action() const -> Action {
        return action_;
    }

    [[nodiscard]] auto is_pressed() const -> bool {
        return action_ == Action::press;
    }

private:
    Button button_;
    Action action_;
};

/// Mouse movement while remaining over the active hover target.
class MouseMoveEvent final : public MouseEvent {
public:
    MouseMoveEvent(foundation::NanPoint screen_pos, foundation::NanPoint delta)
        : MouseEvent(screen_pos), delta_(delta) {}

    [[nodiscard]] auto delta() const -> foundation::NanPoint {
        return delta_;
    }

private:
    foundation::NanPoint delta_;
};

/// Mouse entered a node's hover chain.
class MouseEnterEvent final : public MouseEvent {
public:
    explicit MouseEnterEvent(foundation::NanPoint screen_pos)
        : MouseEvent(screen_pos) {}
};

/// Mouse left a node's hover chain.
class MouseLeaveEvent final : public MouseEvent {
public:
    explicit MouseLeaveEvent(foundation::NanPoint screen_pos)
        : MouseEvent(screen_pos) {}
};

/// Base class for keyboard input.
class KeyEvent : public InputEvent {
public:
    enum class Action { press, release };

    KeyEvent(int keycode, Action action)
        : keycode_(keycode), action_(action) {}

    [[nodiscard]] auto keycode() const -> int {
        return keycode_;
    }

    [[nodiscard]] auto action() const -> Action {
        return action_;
    }

    [[nodiscard]] auto is_pressed() const -> bool {
        return action_ == Action::press;
    }

private:
    int keycode_;
    Action action_;
};

/// Focus entered a node's focus chain.
class FocusEnterEvent final : public InputEvent {};

/// Focus left a node's focus chain.
class FocusLeaveEvent final : public InputEvent {};

} // namespace nandina::scene

#endif // NANDINA_EXPERIMENT_INPUT_EVENT_HPP
