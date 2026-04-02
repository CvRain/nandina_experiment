module;

#include <functional>
#include <memory>
#include <utility>
#include <vector>

export module Nandina.Core.Widget;

import Nandina.Core.Event;
import Nandina.Core.Signal;

export namespace Nandina {

    using WidgetPtr = std::unique_ptr<class Widget>;

    class Widget {
    public:
        using Child = std::unique_ptr<Widget>;

        virtual ~Widget() = default;

        auto add_child(Child child) -> Widget& {
            child->parent_ = this;
            children_.push_back(std::move(child));
            mark_dirty();
            return *this;
        }

        auto set_bounds(float x, float y, float width, float height) noexcept -> Widget& {
            x_ = x; y_ = y; width_ = width; height_ = height;
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto contains(float px, float py) const noexcept -> bool {
            return px >= x_ && px < x_ + width_ && py >= y_ && py < y_ + height_;
        }

        auto dispatch_event(Event& event) -> bool {
            for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
                if ((*it)->dispatch_event(event)) { return true; }
            }
            if (event.handled || !contains(event.x, event.y)) { return false; }
            return handle_event(event);
        }

        auto on_click(std::function<void()> handler) -> Connection {
            return clicked_.connect(std::move(handler));
        }

        [[nodiscard]] auto is_dirty()         const noexcept -> bool { return dirty_; }
        [[nodiscard]] auto has_dirty_child()  const noexcept -> bool { return has_dirty_child_; }
        [[nodiscard]] auto x()      const noexcept -> float { return x_; }
        [[nodiscard]] auto y()      const noexcept -> float { return y_; }
        [[nodiscard]] auto width()  const noexcept -> float { return width_; }
        [[nodiscard]] auto height() const noexcept -> float { return height_; }

        auto mark_dirty() noexcept -> void {
            dirty_ = true;
            if (parent_ && !parent_->has_dirty_child_) {
                parent_->bubble_dirty_child();
            }
        }

        auto clear_dirty() noexcept -> void {
            dirty_           = false;
            has_dirty_child_ = false;
        }

        auto for_each_child(std::function<void(Widget&)> fn) const -> void {
            for (const auto& child : children_) { fn(*child); }
        }

    protected:
        virtual auto handle_event(Event& event) -> bool {
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
        auto bubble_dirty_child() -> void {
            has_dirty_child_ = true;
            if (parent_ && !parent_->has_dirty_child_) {
                parent_->bubble_dirty_child();
            }
        }

        float x_ = 0.0f, y_ = 0.0f, width_ = 0.0f, height_ = 0.0f;
        bool dirty_           = true;
        bool has_dirty_child_ = false;
        Widget* parent_       = nullptr;
        std::vector<Child> children_;
        Signal<> clicked_;
    };

    class Component : public Widget {
    public:
        using Ptr = std::unique_ptr<Component>;
        virtual ~Component() = default;
    };

} // export namespace Nandina
