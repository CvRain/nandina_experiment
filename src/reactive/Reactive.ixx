module;
#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

export module Nandina.Reactive;

export namespace Nandina {

    // ── Forward declarations ──────────────────────────────────────────────────
    template<typename T> class State;
    template<typename T> class ReadState;

    // ── Internal detail ───────────────────────────────────────────────────────
    namespace detail {
        inline thread_local std::function<void()>* current_invalidator = nullptr;

        struct ObserverEntry {
            std::size_t id       = 0;
            bool        active   = true;
            std::function<void()> invalidate;
        };
    } // namespace detail

    // ── State<T> ──────────────────────────────────────────────────────────────
    template<typename T>
    class State {
    public:
        explicit State(T initial_value)
            : value_(std::move(initial_value)) {}

        State(const State&)            = delete;
        auto operator=(const State&)   = delete;
        State(State&&) noexcept        = default;
        auto operator=(State&&) noexcept -> State& = default;

        /// Read value and auto-register current Effect as observer.
        [[nodiscard]] auto operator()() const -> const T& {
            track_access();
            return value_;
        }

        /// Alias for operator()().
        [[nodiscard]] auto get() const -> const T& {
            return (*this)();
        }

        /// Write new value; notifies observers if value changed.
        auto set(T new_value) -> void {
            if (!(value_ == new_value)) {
                value_ = std::move(new_value);
                notify();
            }
        }

        /// Returns a read-only view of this State.
        [[nodiscard]] auto as_read_only() const -> ReadState<T>;

    private:
        auto track_access() const -> void {
            if (detail::current_invalidator) {
                observers_.push_back({
                    next_id_++,
                    true,
                    *detail::current_invalidator
                });
            }
        }

        auto notify() -> void {
            // Move observers out so Effects can re-register during re-run
            auto snapshot = std::move(observers_);
            observers_ = {};
            for (auto& entry : snapshot) {
                if (entry.active && entry.invalidate) {
                    entry.invalidate();
                }
            }
        }

        T value_;
        mutable std::vector<detail::ObserverEntry> observers_;
        mutable std::size_t next_id_ = 1;
    };

    // ── ReadState<T> ──────────────────────────────────────────────────────────
    template<typename T>
    class ReadState {
    public:
        explicit ReadState(const State<T>* source)
            : source_(source) {
            assert(source != nullptr);
        }

        ReadState(const ReadState&)          = delete;
        auto operator=(const ReadState&)     = delete;
        ReadState(ReadState&&) noexcept      = default;
        auto operator=(ReadState&&) noexcept -> ReadState& = default;

        [[nodiscard]] auto operator()() const -> const T& {
            return (*source_)();
        }

        [[nodiscard]] auto get() const -> const T& {
            return (*this)();
        }

    private:
        const State<T>* source_;
    };

    // ── State<T>::as_read_only() implementation ───────────────────────────────
    template<typename T>
    auto State<T>::as_read_only() const -> ReadState<T> {
        return ReadState<T>{ this };
    }

    // ── Computed<F> ───────────────────────────────────────────────────────────
    template<typename F>
        requires std::invocable<F>
    class Computed {
    public:
        using ValueType = std::invoke_result_t<F>;

        explicit Computed(F compute_fn)
            : compute_fn_(std::move(compute_fn)), stale_(true) {}

        Computed(const Computed&)          = delete;
        auto operator=(const Computed&)    = delete;
        Computed(Computed&&) noexcept      = default;
        auto operator=(Computed&&) noexcept -> Computed& = default;

        [[nodiscard]] auto operator()() const -> const ValueType& {
            if (stale_) {
                recompute();
            }
            return cached_;
        }

    private:
        auto recompute() const -> void {
            stale_ = false;
            auto invalidator = std::function<void()>{ [this]{ stale_ = true; } };
            auto* prev = detail::current_invalidator;
            detail::current_invalidator = &invalidator;
            cached_ = compute_fn_();
            detail::current_invalidator = prev;
        }

        F                  compute_fn_;
        mutable ValueType  cached_{};
        mutable bool       stale_;
    };

    template<typename F>
    Computed(F) -> Computed<F>;

    // ── Effect ────────────────────────────────────────────────────────────────
    class Effect {
    public:
        template<typename F>
            requires std::invocable<F>
        explicit Effect(F&& fn)
            : fn_(std::forward<F>(fn)) {
            run();
        }

        Effect(const Effect&)          = delete;
        auto operator=(const Effect&)  = delete;

        Effect(Effect&& other) noexcept
            : fn_(std::move(other.fn_)),
              active_(other.active_),
              self_invalidator_(std::move(other.self_invalidator_)) {
            other.active_ = false;
        }

        auto operator=(Effect&& other) noexcept -> Effect& {
            if (this != &other) {
                fn_               = std::move(other.fn_);
                active_           = other.active_;
                self_invalidator_ = std::move(other.self_invalidator_);
                other.active_     = false;
            }
            return *this;
        }

        ~Effect() {
            active_ = false;
        }

    private:
        auto run() -> void {
            if (!active_) { return; }
            self_invalidator_ = [this]{ run(); };
            auto* prev = detail::current_invalidator;
            detail::current_invalidator = &self_invalidator_;
            fn_();
            detail::current_invalidator = prev;
        }

        std::function<void()> fn_;
        bool                  active_ = true;
        std::function<void()> self_invalidator_;
    };

    // ── EffectScope ───────────────────────────────────────────────────────────
    class EffectScope {
    public:
        template<typename F>
            requires std::invocable<F>
        auto add(F&& fn) -> void {
            effects_.emplace_back(std::forward<F>(fn));
        }

        auto clear() -> void {
            effects_.clear();
        }

        [[nodiscard]] auto size() const noexcept -> std::size_t {
            return effects_.size();
        }

    private:
        std::vector<Effect> effects_;
    };

} // export namespace Nandina
