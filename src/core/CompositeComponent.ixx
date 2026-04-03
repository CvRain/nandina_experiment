module;
#include <memory>
#include <utility>

export module Nandina.Core.CompositeComponent;

import Nandina.Core.Component;
import Nandina.Core.Widget;

export namespace Nandina {

// ── CompositeComponent<Props> ─────────────────────────────────────────────────
// Base class for reusable components that compose their UI in a build() method.
//
// build() is called once during initialize(). Reactive updates are driven by
// Effects registered via effect().
//
// Usage pattern:
//   class MyWidget : public CompositeComponent<void> {
//   public:
//       static auto Create() -> std::unique_ptr<MyWidget> {
//           auto self = std::unique_ptr<MyWidget>(new MyWidget());
//           self->initialize();  // calls build() after construction
//           return self;
//       }
//       auto build() -> WidgetPtr override { ... }
//   };
//
// Props variant:
//   class ProcessWidget : public CompositeComponent<void> {
//   public:
//       explicit ProcessWidget(std::string title) : title_(std::move(title)) {}
//       // ... same Create() pattern
//   private:
//       std::string title_;
//   };
// ─────────────────────────────────────────────────────────────────────────────
class CompositeComponent : public Component {
public:
    // Called by the framework (or by a derived class's static Create() factory)
    // after the object has been fully constructed so that virtual dispatch
    // inside build() resolves correctly. Must be called exactly once.
    auto initialize() -> void {
        auto root = build();
        if (root) {
            add_child(std::move(root));
        }
    }

    // Subclasses implement this to return the initial widget tree.
    // Called once by initialize(). Use effect() inside build() to register
    // reactive updates.
    virtual auto build() -> WidgetPtr = 0;

protected:
    // Registers a reactive Effect scoped to this component's lifetime.
    // Typically called inside build() to bind State to widget properties.
    template<typename F>
    auto effect(F&& fn) -> void {
        scope_.add(std::forward<F>(fn));
    }
};

} // namespace Nandina
