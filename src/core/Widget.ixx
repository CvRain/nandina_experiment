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
import Nandina.Types.Position;
import Nandina.Types.Size;
import Nandina.Types.Rect;

export namespace Nandina {

    using WidgetPtr = std::unique_ptr<class Widget>;

    class Widget {
    public:
        using Child = std::unique_ptr<Widget>;

        virtual ~Widget() = default;

        auto set_hit_test_visible(bool value) noexcept -> Widget& {
            hit_test_visible_ = value;
            return *this;
        }

        [[nodiscard]] auto hit_test_visible() const noexcept -> bool {
            return hit_test_visible_;
        }

        auto layer(int index) noexcept -> Widget& {
            layer_ = index;
            return *this;
        }

        [[nodiscard]] auto layer() const noexcept -> int {
            return layer_;
        }

        auto add_child(Child child) -> Widget& {
            child->parent_ = this;
            children_.push_back(std::move(child));
            mark_dirty();
            return *this;
        }

        // Non-owning child reference. The caller is responsible for the child's
        // lifetime (e.g. Router owns its Pages while RouterView just references them).
        auto add_child_ref(Widget* child) -> Widget& {
            if (child) {
                child->parent_ = this;
                children_ref_.push_back(child);
                mark_dirty();
            }
            return *this;
        }

        // Clears all non-owning child references without destroying the widgets.
        auto clear_children_ref() -> void {
            for (auto* c : children_ref_) {
                if (c) { c->parent_ = nullptr; }
            }
            children_ref_.clear();
            mark_dirty();
        }

        virtual auto set_bounds(float x, float y, float width, float height) noexcept -> Widget& {
            x_ = x; y_ = y; width_ = width; height_ = height;
            mark_dirty();
            return *this;
        }

        // ── Geometric-type overloads ──────────────────────────────────────────
        auto set_bounds(const Rect& r) noexcept -> Widget& {
            return set_bounds(r.x(), r.y(), r.width(), r.height());
        }

        auto set_bounds(const Position& pos, const Size& size) noexcept -> Widget& {
            return set_bounds(pos.x(), pos.y(), size.width(), size.height());
        }

        auto set_position(const Position& pos) noexcept -> Widget& {
            return set_bounds(pos.x(), pos.y(), width_, height_);
        }

        auto set_size(const Size& size) noexcept -> Widget& {
            return set_bounds(x_, y_, size.width(), size.height());
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
            for (auto it = children_ref_.rbegin(); it != children_ref_.rend(); ++it) {
                if (*it && (*it)->dispatch_event(ev)) { return true; }
            }
            for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
                if ((*it)->dispatch_event(ev)) { return true; }
            }
            if (ev.handled || !hit_test_visible_ || !contains(ev.x, ev.y)) { return false; }
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
            for (auto* c : children_ref_) { if (c) { fn(*c); } }
        }

        [[nodiscard]] auto is_dirty()        const noexcept -> bool     { return dirty_; }
        [[nodiscard]] auto has_dirty_child() const noexcept -> bool     { return has_dirty_child_; }
        [[nodiscard]] auto x()               const noexcept -> float    { return x_; }
        [[nodiscard]] auto y()               const noexcept -> float    { return y_; }
        [[nodiscard]] auto width()           const noexcept -> float    { return width_; }
        [[nodiscard]] auto height()          const noexcept -> float    { return height_; }
        [[nodiscard]] auto background()      const noexcept -> Color    { return bg_color_; }
        [[nodiscard]] auto border_radius()   const noexcept -> float    { return border_radius_; }

        // ── Geometric readers ─────────────────────────────────────────────────
        [[nodiscard]] auto rect()     const noexcept -> Rect     { return {x_, y_, width_, height_}; }
        [[nodiscard]] auto position() const noexcept -> Position { return {x_, y_}; }
        [[nodiscard]] auto size()     const noexcept -> Size     { return {width_, height_}; }

        // ── Flex layout support ───────────────────────────────────────────────
        // Returns 0 for fixed-size widgets; Expanded / Spacer return their flex value.
        [[nodiscard]] virtual auto flex_factor() const noexcept -> int { return 0; }

    protected:
        // Geometric fields are protected so that FreeWidget (and other subclasses)
        // can directly update position/size without going through set_bounds().
        float x_      = 0.0f;
        float y_      = 0.0f;
        float width_  = 0.0f;
        float height_ = 0.0f;

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

        float  border_radius_ = 0.0f;
        bool   dirty_           = true;
        bool   has_dirty_child_ = false;
        bool   hit_test_visible_ = true;
        Widget* parent_         = nullptr;

        Color  bg_color_{255, 255, 255, 255};
        int    layer_ = -1;
        EventSignal<> clicked_;
        std::vector<Child>   children_;
        std::vector<Widget*> children_ref_;  // Non-owning references (e.g. for RouterView)
    };

} // export namespace Nandina
