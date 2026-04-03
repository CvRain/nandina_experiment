module;

#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

export module Nandina.Core.Widget;

import Nandina.Core.Color;
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

        virtual auto set_bounds(float x, float y, float width, float height) noexcept -> Widget& {
            x_ = x; y_ = y; width_ = width; height_ = height;
            mark_dirty();
            return *this;
        }

        auto set_background(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                            std::uint8_t a = 255) noexcept -> Widget& {
            bg_color_ = {r, g, b, a};
            mark_dirty();
            return *this;
        }

        auto set_border_radius(float r) noexcept -> Widget& {
            border_radius_ = r;
            mark_dirty();
            return *this;
        }

        auto on_click(std::function<void()> handler) -> Connection {
            return clicked_.connect(std::move(handler));
        }

        auto dispatch_event(Event& ev) -> bool {
            for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
                if ((*it)->dispatch_event(ev)) { return true; }
            }
            if (ev.handled || !contains(ev.x, ev.y)) { return false; }
            return handle_event(ev);
        }

        [[nodiscard]] auto contains(float px, float py) const noexcept -> bool {
            return px >= x_ && px < x_ + width_ && py >= y_ && py < y_ + height_;
        }

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
            for (const auto& c : children_) { fn(*c); }
        }

        [[nodiscard]] auto is_dirty()        const noexcept -> bool  { return dirty_; }
        [[nodiscard]] auto has_dirty_child() const noexcept -> bool  { return has_dirty_child_; }
        [[nodiscard]] auto x()               const noexcept -> float { return x_; }
        [[nodiscard]] auto y()               const noexcept -> float { return y_; }
        [[nodiscard]] auto width()           const noexcept -> float { return width_; }
        [[nodiscard]] auto height()          const noexcept -> float { return height_; }
        [[nodiscard]] auto background()      const noexcept -> Color { return bg_color_; }
        [[nodiscard]] auto border_radius()   const noexcept -> float { return border_radius_; }

    protected:
        virtual auto handle_event(Event& ev) -> bool {
            if (ev.type == EventType::click) {
                clicked_.emit();
                ev.mark_handled();
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

        float  x_             = 0.0f;
        float  y_             = 0.0f;
        float  width_         = 0.0f;
        float  height_        = 0.0f;
        float  border_radius_ = 0.0f;
        bool   dirty_           = true;
        bool   has_dirty_child_ = false;
        Widget* parent_         = nullptr;

        Color  bg_color_{255, 255, 255, 255};
        EventSignal<> clicked_;
        std::vector<Child> children_;
    };

} // export namespace Nandina
