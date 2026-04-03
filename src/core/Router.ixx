module;
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

export module Nandina.Core.Router;

import Nandina.Core.CompositeComponent;
import Nandina.Core.Component;
import Nandina.Core.Widget;
import Nandina.Reactive;

export namespace Nandina {

// ── Page ─────────────────────────────────────────────────────────────────────
// Full-screen component with navigation lifecycle hooks. All user page classes
// should inherit from Page.
//
// Usage:
//   class HomePage : public Nandina::Page {
//   public:
//       static auto Create() -> std::unique_ptr<HomePage> {
//           auto self = std::unique_ptr<HomePage>(new HomePage());
//           self->initialize();
//           return self;
//       }
//       auto build() -> WidgetPtr override { ... }
//       auto on_enter() -> void override { /* resume animations, etc. */ }
//   };
//
//   router.push<HomePage>();
// ─────────────────────────────────────────────────────────────────────────────
class Router;  // forward declaration within this module — no cross-module conflict

class Page : public CompositeComponent {
public:
    // Lifecycle hooks — called by Router on push/pop transitions.
    virtual auto on_enter() -> void {}
    virtual auto on_leave() -> void {}

    // Returns the Router that owns this page. Only valid after the page has
    // been pushed onto a Router stack.
    [[nodiscard]] auto router() noexcept -> Router* { return router_; }
    [[nodiscard]] auto router() const noexcept -> const Router* { return router_; }

    // Called by Router to inject the owning router pointer.
    auto set_router(Router* r) noexcept -> void { router_ = r; }

protected:
    Router* router_ = nullptr;
};


// ── Router ────────────────────────────────────────────────────────────────────
// Manages a stack of Page instances and drives navigation transitions.
//
// All page types must:
//   1. Inherit from Nandina::Page
//   2. Have a public default constructor (or a constructor accepting props)
//   3. Call initialize() after construction (Router handles this automatically)
//
// Example:
//   Nandina::Router router;
//   router.push<HomePage>();
//   router.push<DetailPage>(DetailProps{ .id = 42 });
//   router.pop();
// ─────────────────────────────────────────────────────────────────────────────
class Router {
public:
    Router() = default;

    // ── push (no-props page) ──────────────────────────────────────────────────
    template<typename PageType>
        requires std::is_base_of_v<Page, PageType>
    auto push() -> void {
        auto page = std::unique_ptr<PageType>(new PageType());
        push_impl(std::move(page));
    }

    // ── push (page with props, passed to constructor) ─────────────────────────
    template<typename PageType, typename Props>
        requires std::is_base_of_v<Page, PageType>
    auto push(Props props) -> void {
        auto page = std::unique_ptr<PageType>(new PageType(std::move(props)));
        push_impl(std::move(page));
    }

    // ── pop ───────────────────────────────────────────────────────────────────
    auto pop() -> void {
        if (stack_.empty()) { return; }
        stack_.back()->on_leave();
        stack_.pop_back();
        if (!stack_.empty()) {
            stack_.back()->on_enter();
        }
        version_.set(version_.get() + 1);
    }

    // ── replace (no-props) ────────────────────────────────────────────────────
    template<typename PageType>
        requires std::is_base_of_v<Page, PageType>
    auto replace() -> void {
        if (!stack_.empty()) {
            stack_.back()->on_leave();
            stack_.pop_back();
        }
        push<PageType>();
    }

    // ── replace (with props) ─────────────────────────────────────────────────
    template<typename PageType, typename Props>
        requires std::is_base_of_v<Page, PageType>
    auto replace(Props props) -> void {
        if (!stack_.empty()) {
            stack_.back()->on_leave();
            stack_.pop_back();
        }
        push<PageType>(std::move(props));
    }

    // Returns the current (top-of-stack) page, or nullptr if the stack is empty.
    [[nodiscard]] auto current() noexcept -> Page* {
        if (stack_.empty()) { return nullptr; }
        return stack_.back().get();
    }

    [[nodiscard]] auto current() const noexcept -> const Page* {
        if (stack_.empty()) { return nullptr; }
        return stack_.back().get();
    }

    [[nodiscard]] auto depth() const noexcept -> std::size_t {
        return stack_.size();
    }

    // RouterView subscribes to this to detect page changes.
    [[nodiscard]] auto version() const noexcept -> ReadState<int> {
        return version_.as_read_only();
    }

private:
    template<typename PageType>
    auto push_impl(std::unique_ptr<PageType> page) -> void {
        page->set_router(this);
        page->initialize();
        if (!stack_.empty()) {
            stack_.back()->on_leave();
        }
        page->on_enter();
        stack_.push_back(std::move(page));
        version_.set(version_.get() + 1);
    }

    std::vector<std::unique_ptr<Page>> stack_;
    State<int> version_{ 0 };
};


// ── RouterView ────────────────────────────────────────────────────────────────
// A Component that displays the Router's current page. When the Router's
// version State changes, RouterView automatically swaps the displayed page.
//
// RouterView does NOT own the pages — the Router does. It holds a non-owning
// reference via Widget::add_child_ref / clear_children_ref.
// ─────────────────────────────────────────────────────────────────────────────
class RouterView : public Component {
public:
    static auto Create(Router& router) -> std::unique_ptr<RouterView> {
        return std::unique_ptr<RouterView>(new RouterView(router));
    }

    auto set_bounds(float x, float y, float w, float h) noexcept -> Widget& override {
        Component::set_bounds(x, y, w, h);
        sync_page_bounds();
        return *this;
    }

private:
    explicit RouterView(Router& router) : router_(router) {
        set_background(0, 0, 0, 0);  // transparent — page renders itself

        // React to page-stack changes: swap the displayed page widget.
        scope_.add([this] {
            (void)router_.version()();  // subscribe to version changes
            mount_current_page();
        });
    }

    auto mount_current_page() -> void {
        // Release the previous non-owning reference without destroying the page.
        clear_children_ref();

        if (auto* page = router_.current()) {
            add_child_ref(page);
            sync_page_bounds();
        }
        mark_dirty();
    }

    auto sync_page_bounds() -> void {
        if (auto* page = router_.current()) {
            page->set_bounds(x(), y(), width(), height());
        }
    }

    Router& router_;
};

} // namespace Nandina
