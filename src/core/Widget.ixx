module;

#include <functional>
#include <memory>
#include <utility>
#include <vector>

export module Nandina.Core.Widget;

import Nandina.Core.Event;
import Nandina.Core.Signal;

export namespace Nandina {
    class Widget {
    public:
        using Child = std::unique_ptr<Widget>;

        virtual ~Widget() = default;

        auto add_child(Child child) -> Widget&;

        auto set_bounds(float x, float y, float width, float height) noexcept -> Widget&;

        [[nodiscard]] auto contains(float px, float py) const noexcept -> bool;

        auto dispatch_event(Event &event) -> bool;

        auto on_click(std::function<void()> handler) -> Connection;

        [[nodiscard]] auto is_dirty() const noexcept -> bool;

        auto mark_dirty() noexcept -> void;

        auto clear_dirty() noexcept -> void;

    protected:
        virtual auto handle_event(Event &event) -> bool;

        virtual auto on_click_event() -> void;

        [[nodiscard]] auto children() const noexcept -> const std::vector<Child>&;

    private:
        float x_ = 0.0f;
        float y_ = 0.0f;
        float width_ = 0.0f;
        float height_ = 0.0f;
        bool dirty_ = true;
        std::vector<Child> children_;
        Signal<> clicked_;
    };

    auto Widget::add_child(Child child) -> Widget& {
        children_.push_back(std::move(child));
        mark_dirty();
        return *this;
    }

    auto Widget::set_bounds(const float x, const float y, const float width, const float height) noexcept -> Widget& {
        x_ = x;
        y_ = y;
        width_ = width;
        height_ = height;
        mark_dirty();
        return *this;
    }

    auto Widget::contains(const float px, const float py) const noexcept -> bool {
        return px >= x_ && px < x_ + width_ && py >= y_ && py < y_ + height_;
    }

    auto Widget::dispatch_event(Event &event) -> bool {
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

    auto Widget::on_click(std::function<void()> handler) -> Connection {
        return clicked_.connect(std::move(handler));
    }

    auto Widget::is_dirty() const noexcept -> bool {
        return dirty_;
    }

    auto Widget::mark_dirty() noexcept -> void {
        dirty_ = true;
    }

    auto Widget::clear_dirty() noexcept -> void {
        dirty_ = false;
    }

    auto Widget::handle_event(Event &event) -> bool {
        if (event.type == EventType::click) {
            on_click_event();
            event.mark_handled();
            return true;
        }
        return false;
    }

    auto Widget::on_click_event() -> void {
        clicked_.emit();
    }

    auto Widget::children() const noexcept -> const std::vector<Child>& {
        return children_;
    }
}