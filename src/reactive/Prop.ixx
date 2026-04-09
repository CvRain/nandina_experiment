module;
#include <functional>
#include <utility>
#include <variant>

export module Nandina.Reactive.Prop;

import Nandina.Core.Signal;
import Nandina.Reactive.State;

export namespace Nandina {

    // ── Prop<T> ───────────────────────────────────────────────────────────────
    // Uniform property wrapper for component attributes. A Prop<T> can hold
    // either a static compile-time value or a reactive reference to a State<T>.
    //
    // Usage:
    //   // Static value — value never changes after construction
    //   Prop<std::string> title{"Hello"};
    //
    //   // Reactive — reads and subscribes to a State<std::string>
    //   State<std::string> name{"world"};
    //   Prop<std::string> title{name};
    //
    //   title.get();          // current value
    //   title.is_reactive();  // true if bound to State
    //
    //   // Subscribe to changes (no-op connection returned for static props)
    //   auto conn = title.on_change([](const std::string& v){ ... });
    template<typename T>
    class Prop {
    public:
        // ── Construction ─────────────────────────────────────────────────────

        /// Construct from a static value. on_change() will return a no-op Connection.
        explicit Prop(T value) : storage_(std::move(value)) {}

        /// Construct from a reactive State reference. The Prop does not own the
        /// State; the caller must ensure the State outlives the Prop.
        explicit Prop(State<T>& state) : storage_(&state) {}

        Prop(const Prop&)            = delete;
        auto operator=(const Prop&)  = delete;
        Prop(Prop&&) noexcept        = default;
        auto operator=(Prop&&) noexcept -> Prop& = default;

        // ── Accessors ────────────────────────────────────────────────────────

        /// Returns the current value. For reactive props, reads from the bound State
        /// and auto-registers the current Effect as an observer (if any).
        [[nodiscard]] auto get() const -> const T& {
            if (const auto* state_ptr = std::get_if<State<T>*>(&storage_)) {
                return (*state_ptr)->get();
            }
            return std::get<T>(storage_);
        }

        /// Returns true if this Prop is bound to a State<T>.
        [[nodiscard]] auto is_reactive() const -> bool {
            return std::holds_alternative<State<T>*>(storage_);
        }

        // ── Subscription ─────────────────────────────────────────────────────

        /// Subscribe to value changes.
        ///   - Reactive prop: callback is invoked whenever the bound State changes.
        ///     Returns a live Connection.
        ///   - Static prop:   callback is never invoked. Returns a default (no-op)
        ///     Connection that reports connected() == false.
        [[nodiscard]] auto on_change(std::function<void(const T&)> callback) -> Connection {
            if (auto* state_ptr = std::get_if<State<T>*>(&storage_)) {
                return (*state_ptr)->on_change(std::move(callback));
            }
            return Connection{};  // no-op for static props
        }

    private:
        std::variant<T, State<T>*> storage_;
    };

} // export namespace Nandina
