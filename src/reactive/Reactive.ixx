// Nandina.Reactive — all reactive primitives in a single module unit.
//
// Consolidating into one TU avoids the GCC-15 diamond-import bug that arises
// when the same module BMI is reachable through multiple re-export paths.
//
// Public API:
//   ── Signals ──────────────────────────────────────────────────────────────
//   Connection          — RAII handle to a registered slot; call disconnect()
//   ScopedConnection    — auto-disconnects on destruction
//   EventSignal<Args…>  — thread-safe multi-slot signal
//
//   ── Reactive state ───────────────────────────────────────────────────────
//   State<T>            — mutable reactive value; reading inside Effect auto-tracks
//   ReadState<T>        — read-only view of a State<T>
//   Computed<F>         — lazily cached derived value, invalidated by State changes
//   Effect              — re-runs an invocable whenever its State dependencies change
//   EffectScope         — owns a set of Effects; clear() disposes all at once
//
//   ── Properties ───────────────────────────────────────────────────────────
//   Prop<T>             — static or reactive property; uniform get()/on_change() API
//
//   ── Observable collections ───────────────────────────────────────────────
//   ListChangeKind      — Inserted / Removed / Updated / Moved / Reset
//   ListChange<T>       — per-mutation descriptor emitted by StateList
//   StateList<T>        — observable std::vector with fine- and coarse-grained events
module;

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

export module Nandina.Reactive;

export namespace Nandina {

// ═════════════════════════════════════════════════════════════════════════════
// §1  Signals  (Connection / ScopedConnection / EventSignal)
// ═════════════════════════════════════════════════════════════════════════════

// ── Connection ────────────────────────────────────────────────────────────────
// Represents a single registered slot. Call disconnect() to unsubscribe.
// Move-only; safe to store and pass around.
class Connection {
public:
    Connection() noexcept {}

    Connection(std::function<void()> disconnect_fn, std::function<bool()> connected_fn)
        : disconnect_fn_(std::move(disconnect_fn)), connected_fn_(std::move(connected_fn)) {}

    Connection(const Connection&) = delete;
    auto operator=(const Connection&) -> Connection& = delete;

    Connection(Connection&& other) noexcept
        : disconnect_fn_(std::move(other.disconnect_fn_))
        , connected_fn_(std::move(other.connected_fn_)) {}

    auto operator=(Connection&& other) noexcept -> Connection& {
        if (this != &other) {
            disconnect_fn_ = std::move(other.disconnect_fn_);
            connected_fn_  = std::move(other.connected_fn_);
        }
        return *this;
    }

    ~Connection() {}

    auto disconnect() -> void {
        if (disconnect_fn_) {
            disconnect_fn_();
            disconnect_fn_ = {};
            connected_fn_  = {};
        }
    }

    [[nodiscard]] auto connected() const -> bool {
        return connected_fn_ && connected_fn_();
    }

private:
    std::function<void()> disconnect_fn_;
    std::function<bool()> connected_fn_;
};

// ── ScopedConnection ──────────────────────────────────────────────────────────
// RAII wrapper around Connection. Automatically disconnects on destruction.
// Use when a slot should live exactly as long as the owning object.
//
// Example:
//   ScopedConnection sc{ signal.connect([&]{ … }) };
//   // disconnects automatically when sc goes out of scope
class ScopedConnection {
public:
    ScopedConnection() noexcept {}
    explicit ScopedConnection(Connection conn) : conn_(std::move(conn)) {}

    ScopedConnection(const ScopedConnection&) = delete;
    auto operator=(const ScopedConnection&) -> ScopedConnection& = delete;

    ScopedConnection(ScopedConnection&& other) noexcept : conn_(std::move(other.conn_)) {}

    auto operator=(ScopedConnection&& other) noexcept -> ScopedConnection& {
        if (this != &other) { conn_ = std::move(other.conn_); }
        return *this;
    }

    ~ScopedConnection() { conn_.disconnect(); }

    auto disconnect() -> void { conn_.disconnect(); }
    [[nodiscard]] auto connected() const -> bool { return conn_.connected(); }

    /// Release ownership; the caller is responsible for managing the connection.
    auto release() -> Connection { return std::move(conn_); }

private:
    Connection conn_;
};

// ── EventSignal<Args…> ────────────────────────────────────────────────────────
// Thread-safe event signal.
//
// Thread safety:
//   connect / connect_once / disconnect / clear — exclusive lock
//   emit — snapshots slots under lock, then invokes without lock
//
// Safe to call disconnect() from within a slot callback (no re-entrant lock).
//
// Example:
//   EventSignal<int> sig;
//   auto conn = sig.connect([](int v){ … });
//   sig.emit(42);       // calls all active slots
//   conn.disconnect();  // unsubscribe
//
//   // One-shot: fires once then disconnects automatically
//   auto once = sig.connect_once([](int v){ … });
template<typename... Args>
class EventSignal {
public:
    using Slot = std::function<void(Args...)>;

    auto connect(Slot slot) -> Connection {
        std::lock_guard lock(mutex_);
        const std::size_t id = next_id_++;
        slots_.push_back(SlotEntry{id, true, false, std::move(slot)});
        return make_connection(id);
    }

    auto connect_once(Slot slot) -> Connection {
        std::lock_guard lock(mutex_);
        const std::size_t id = next_id_++;
        slots_.push_back(SlotEntry{id, true, true, std::move(slot)});
        return make_connection(id);
    }

    auto emit(Args... args) -> void {
        std::vector<SlotEntry> snapshot;
        {
            std::lock_guard lock(mutex_);
            snapshot = slots_;
        }

        std::vector<std::size_t> once_ids;
        for (auto& entry : snapshot) {
            if (entry.active) {
                entry.slot(args...);
                if (entry.once) { once_ids.push_back(entry.id); }
            }
        }

        {
            std::lock_guard lock(mutex_);
            for (auto id : once_ids) { disconnect_locked(id); }
            cleanup_locked();
        }
    }

    auto clear() -> void {
        std::lock_guard lock(mutex_);
        slots_.clear();
    }

private:
    struct SlotEntry {
        std::size_t id     = 0;
        bool        active = false;
        bool        once   = false;
        Slot        slot;
    };

    auto make_connection(std::size_t id) -> Connection {
        return Connection(
            [this, id]{ disconnect(id); },
            [this, id]{ return is_connected(id); }
        );
    }

    auto disconnect(std::size_t id) -> void {
        std::lock_guard lock(mutex_);
        disconnect_locked(id);
    }

    auto disconnect_locked(std::size_t id) -> void {
        for (auto& e : slots_) {
            if (e.id == id) { e.active = false; break; }
        }
    }

    [[nodiscard]] auto is_connected(std::size_t id) const -> bool {
        std::lock_guard lock(mutex_);
        for (const auto& e : slots_) {
            if (e.id == id) { return e.active; }
        }
        return false;
    }

    auto cleanup_locked() -> void {
        std::erase_if(slots_, [](const SlotEntry& e) { return !e.active; });
    }

    mutable std::mutex     mutex_;
    std::size_t            next_id_ = 1;
    std::vector<SlotEntry> slots_;
};

// ═════════════════════════════════════════════════════════════════════════════
// §2  Reactive state  (State / ReadState / Computed / Effect / EffectScope)
// ═════════════════════════════════════════════════════════════════════════════

// Forward declarations required by State<T>::as_read_only()
template<typename T> class State;
template<typename T> class ReadState;

namespace detail {
    struct TrackingContext {
        std::size_t id = 0;
        std::function<void()>* invalidate = nullptr;
    };

    inline thread_local TrackingContext* current_tracking_context = nullptr;

    class TrackingContextGuard {
    public:
        explicit TrackingContextGuard(TrackingContext& context) noexcept
            : previous_(current_tracking_context) {
            current_tracking_context = &context;
        }

        TrackingContextGuard(const TrackingContextGuard&) = delete;
        auto operator=(const TrackingContextGuard&) -> TrackingContextGuard& = delete;

        ~TrackingContextGuard() {
            current_tracking_context = previous_;
        }

    private:
        TrackingContext* previous_ = nullptr;
    };

    inline auto next_tracking_id() -> std::size_t {
        static std::atomic_size_t next_id{1};
        return next_id.fetch_add(1, std::memory_order_relaxed);
    }

    struct ObserverEntry {
        std::size_t id       = 0;
        bool        active   = true;
        std::function<void()> invalidate;
    };

    inline auto has_observer_id(const std::vector<ObserverEntry>& observers, std::size_t id) -> bool {
        for (const auto& observer : observers) {
            if (observer.active && observer.id == id) {
                return true;
            }
        }
        return false;
    }

    class PendingObserverRestore {
    public:
        PendingObserverRestore(std::vector<ObserverEntry>& observers,
                               const std::vector<ObserverEntry>& snapshot,
                               std::size_t& next_index) noexcept
            : observers_(observers), snapshot_(snapshot), next_index_(next_index) {}

        PendingObserverRestore(const PendingObserverRestore&) = delete;
        auto operator=(const PendingObserverRestore&) -> PendingObserverRestore& = delete;

        ~PendingObserverRestore() {
            if (committed_) {
                return;
            }

            for (std::size_t index = next_index_; index < snapshot_.size(); ++index) {
                const auto& observer = snapshot_[index];
                if (!observer.active || has_observer_id(observers_, observer.id)) {
                    continue;
                }
                observers_.push_back(observer);
            }
        }

        auto commit() noexcept -> void {
            committed_ = true;
        }

    private:
        std::vector<ObserverEntry>& observers_;
        const std::vector<ObserverEntry>& snapshot_;
        std::size_t& next_index_;
        bool committed_ = false;
    };
} // namespace detail

// ── State<T> ──────────────────────────────────────────────────────────────────
// Reactive value holder. Reading inside an Effect auto-registers it as an
// observer; writing notifies all observers.
//
// State is intentionally non-movable after construction (ReadState stores a
// raw pointer to it). Declare as a named member, not a temporary.
//
// Subscribe from outside the Effect system:
//   auto conn = state.on_change([](const T& v){ … });
template<typename T>
class State {
public:
    explicit State(T initial_value) : value_(std::move(initial_value)) {}

    State(const State&)          = delete;
    auto operator=(const State&) = delete;
    State(State&&)               = delete;
    auto operator=(State&&)      = delete;

    /// Read value and auto-register the current Effect as an observer.
    [[nodiscard]] auto operator()() const -> const T& {
        track_access();
        return value_;
    }

    [[nodiscard]] auto get() const -> const T& { return (*this)(); }

    /// Write new value; notifies observers only when the value actually changed.
    auto set(T new_value) -> void {
        if (!(value_ == new_value)) {
            value_ = std::move(new_value);
            notify();
        }
    }

    /// Persistent subscription (not tied to the Effect tracking system).
    auto on_change(std::function<void(const T&)> callback) -> Connection {
        return change_signal_.connect(std::move(callback));
    }

    [[nodiscard]] auto as_read_only() const -> ReadState<T>;

private:
    auto track_access() const -> void {
        if (!detail::current_tracking_context || !detail::current_tracking_context->invalidate) {
            return;
        }
        for (const auto& obs : observers_) {
            if (obs.active && obs.id == detail::current_tracking_context->id) { return; }
        }
        observers_.push_back({
            detail::current_tracking_context->id,
            true,
            *detail::current_tracking_context->invalidate
        });
    }

    auto notify() -> void {
        auto snapshot = std::move(observers_);
        observers_    = {};
        std::size_t next_observer = 0;
        detail::PendingObserverRestore restore_guard{observers_, snapshot, next_observer};

        for (; next_observer < snapshot.size(); ++next_observer) {
            auto& entry = snapshot[next_observer];
            if (entry.active && entry.invalidate) { entry.invalidate(); }
        }

        restore_guard.commit();
        change_signal_.emit(value_);
    }

    T value_;
    mutable std::vector<detail::ObserverEntry> observers_;
    EventSignal<const T&> change_signal_;
};

// ── ReadState<T> ──────────────────────────────────────────────────────────────
// Read-only view of a State<T>. Reading auto-registers the current Effect.
template<typename T>
class ReadState {
public:
    explicit ReadState(const State<T>* source) : source_(source) {
        assert(source != nullptr);
    }

    ReadState(const ReadState&)          = delete;
    auto operator=(const ReadState&)     = delete;
    ReadState(ReadState&&) noexcept      = default;
    auto operator=(ReadState&&) noexcept -> ReadState& = default;

    [[nodiscard]] auto operator()() const -> const T& { return (*source_)(); }
    [[nodiscard]] auto get() const -> const T&        { return (*this)(); }

private:
    const State<T>* source_;
};

template<typename T>
auto State<T>::as_read_only() const -> ReadState<T> { return ReadState<T>{this}; }

// ── Computed<F> ───────────────────────────────────────────────────────────────
// Lazily cached derived value. Re-evaluates when any State it read is modified.
template<typename F>
    requires std::invocable<F>
class Computed {
public:
    using ValueType = std::invoke_result_t<F>;

    explicit Computed(F compute_fn)
        : compute_fn_(std::move(compute_fn)), stale_(true),
          observer_id_(detail::next_tracking_id()) {}

    Computed(const Computed&)          = delete;
    auto operator=(const Computed&)    = delete;
    Computed(Computed&&)               = delete;
    auto operator=(Computed&&)         = delete;

    [[nodiscard]] auto operator()() const -> const ValueType& {
        if (stale_) { recompute(); }
        return cached_;
    }

private:
    auto recompute() const -> void {
        auto invalidator = std::function<void()>{ [this]{ stale_ = true; } };
        detail::TrackingContext context{observer_id_, &invalidator};
        detail::TrackingContextGuard guard{context};

        auto recomputed_value = compute_fn_();
        cached_ = std::move(recomputed_value);
        stale_ = false;
    }

    F                  compute_fn_;
    mutable ValueType  cached_{};
    mutable bool       stale_;
    std::size_t        observer_id_;
};

template<typename F>
Computed(F) -> Computed<F>;

// ── Effect ────────────────────────────────────────────────────────────────────
// Runs an invocable immediately and re-runs it whenever any State read inside
// it changes. Stops tracking on destruction.
class Effect {
public:
    template<typename F>
        requires std::invocable<F>
    explicit Effect(F&& fn)
        : fn_(std::forward<F>(fn)), observer_id_(detail::next_tracking_id()) {
        run();
    }

    Effect(const Effect&)         = delete;
    auto operator=(const Effect&) = delete;
    Effect(Effect&&)              = delete;
    auto operator=(Effect&&)      = delete;

    ~Effect() { active_ = false; }

private:
    auto run() -> void {
        if (!active_) { return; }
        self_invalidator_ = [this]{ run(); };
        detail::TrackingContext context{observer_id_, &self_invalidator_};
        detail::TrackingContextGuard guard{context};
        fn_();
    }

    std::function<void()> fn_;
    bool                  active_ = true;
    std::function<void()> self_invalidator_;
    std::size_t           observer_id_ = 0;
};

// ── EffectScope ───────────────────────────────────────────────────────────────
// Owns a collection of Effects. clear() disposes all at once (e.g., on widget
// destruction). Components hold an EffectScope as a member for lifetime management.
class EffectScope {
public:
    template<typename F>
        requires std::invocable<F>
    auto add(F&& fn) -> void {
        effects_.push_back(std::make_unique<Effect>(std::forward<F>(fn)));
    }

    auto clear() -> void { effects_.clear(); }

    [[nodiscard]] auto size() const noexcept -> std::size_t { return effects_.size(); }

private:
    std::vector<std::unique_ptr<Effect>> effects_;
};

// ═════════════════════════════════════════════════════════════════════════════
// §3  Prop<T>  (static-or-reactive property wrapper)
// ═════════════════════════════════════════════════════════════════════════════

// ── Prop<T> ───────────────────────────────────────────────────────────────────
// Uniform attribute type for components: holds either a compile-time constant
// or a live reference to a State<T>, presenting the same get()/on_change() API.
//
// Example:
//   Prop<std::string> title{"Hello"};          // static
//   State<std::string> name{"world"};
//   Prop<std::string> greeting{name};           // reactive
//
//   greeting.get();          // current value
//   greeting.is_reactive();  // true
//   auto conn = greeting.on_change([](const std::string& v){ … });
template<typename T>
class Prop {
public:
    explicit Prop(T value)       : storage_(std::move(value)) {}
    explicit Prop(State<T>& state) : storage_(&state) {}

    Prop(const Prop&)            = delete;
    auto operator=(const Prop&)  = delete;
    Prop(Prop&&) noexcept        = default;
    auto operator=(Prop&&) noexcept -> Prop& = default;

    [[nodiscard]] auto get() const -> const T& {
        if (const auto* s = std::get_if<State<T>*>(&storage_)) { return (*s)->get(); }
        return std::get<T>(storage_);
    }

    [[nodiscard]] auto is_reactive() const -> bool {
        return std::holds_alternative<State<T>*>(storage_);
    }

    /// Reactive prop: returns a live Connection.
    /// Static prop: returns a no-op Connection (connected() == false).
    [[nodiscard]] auto on_change(std::function<void(const T&)> callback) -> Connection {
        if (auto* s = std::get_if<State<T>*>(&storage_)) {
            return (*s)->on_change(std::move(callback));
        }
        return Connection{};
    }

private:
    std::variant<T, State<T>*> storage_;
};

// ═════════════════════════════════════════════════════════════════════════════
// §4  StateList<T>  (observable vector)
// ═════════════════════════════════════════════════════════════════════════════

// ── ListChangeKind ────────────────────────────────────────────────────────────
enum class ListChangeKind {
    Inserted,   ///< One element inserted at index
    Removed,    ///< One element removed from index
    Updated,    ///< One element replaced in-place at index
    Moved,      ///< Element moved from old_index to index
    Reset,      ///< Entire list replaced (assign_all / clear)
};

// ── ListChange<T> ─────────────────────────────────────────────────────────────
// Describes a single mutation to a StateList<T>.
//   Inserted: index, value
//   Removed:  index
//   Updated:  index, value
//   Moved:    index (new), old_index
//   Reset:    (read full list via StateList::items())
template<typename T>
struct ListChange {
    ListChangeKind   kind      = ListChangeKind::Reset;
    std::size_t      index     = 0;
    std::size_t      old_index = 0;
    std::optional<T> value     = std::nullopt;
};

// ── StateList<T> ──────────────────────────────────────────────────────────────
// Observable std::vector<T>. Mutations emit fine-grained ListChange<T> events
// and a coarse-grained "something changed" notification.
//
// Example:
//   StateList<std::string> list;
//   auto conn = list.on_change([](const ListChange<std::string>& c){ … });
//   auto wconn = list.watch([](){ rebuild_ui(); });
//   list.push_back("hello");   // emits Inserted{index=0, value="hello"}
template<typename T>
class StateList {
public:
    StateList() = default;

    StateList(const StateList&) = delete;
    auto operator=(const StateList&) = delete;
    StateList(StateList&&) = delete;
    auto operator=(StateList&&) = delete;

    // ── Read access ───────────────────────────────────────────────────────────
    [[nodiscard]] auto size()  const noexcept -> std::size_t { return items_.size(); }
    [[nodiscard]] auto empty() const noexcept -> bool        { return items_.empty(); }

    [[nodiscard]] auto operator[](std::size_t i) const -> const T& { return items_[i]; }
    [[nodiscard]] auto at(std::size_t i)          const -> const T& { return items_.at(i); }

    [[nodiscard]] auto begin()  const noexcept { return items_.begin(); }
    [[nodiscard]] auto end()    const noexcept { return items_.end(); }
    [[nodiscard]] auto cbegin() const noexcept { return items_.cbegin(); }
    [[nodiscard]] auto cend()   const noexcept { return items_.cend(); }

    [[nodiscard]] auto items() const noexcept -> const std::vector<T>& { return items_; }

    // ── Mutations ─────────────────────────────────────────────────────────────
    auto push_back(T value) -> void {
        const std::size_t idx = items_.size();
        items_.push_back(std::move(value));
        emit({ListChangeKind::Inserted, idx, 0, items_.back()});
    }

    auto insert(std::size_t index, T value) -> void {
        if (index > items_.size()) { index = items_.size(); }
        items_.insert(items_.begin() + static_cast<std::ptrdiff_t>(index), std::move(value));
        emit({ListChangeKind::Inserted, index, 0, items_[index]});
    }

    auto remove_at(std::size_t index) -> void {
        if (index >= items_.size()) { return; }
        items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(index));
        emit({ListChangeKind::Removed, index, 0, std::nullopt});
    }

    auto update_at(std::size_t index, T value) -> void {
        if (index >= items_.size()) { return; }
        items_[index] = std::move(value);
        emit({ListChangeKind::Updated, index, 0, items_[index]});
    }

    auto move_item(std::size_t from_index, std::size_t to_index) -> void {
        if (from_index == to_index || from_index >= items_.size()) { return; }
        if (to_index >= items_.size()) { to_index = items_.size() - 1; }
        T tmp = std::move(items_[from_index]);
        items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(from_index));
        items_.insert(items_.begin() + static_cast<std::ptrdiff_t>(to_index), std::move(tmp));
        emit({ListChangeKind::Moved, to_index, from_index, std::nullopt});
    }

    auto assign_all(std::vector<T> new_items) -> void {
        items_ = std::move(new_items);
        emit({ListChangeKind::Reset, 0, 0, std::nullopt});
    }

    auto clear() -> void {
        items_.clear();
        emit({ListChangeKind::Reset, 0, 0, std::nullopt});
    }

    // ── Subscription ─────────────────────────────────────────────────────────
    /// Fine-grained: callback receives a ListChange<T> on every mutation.
    auto on_change(std::function<void(const ListChange<T>&)> callback) -> Connection {
        return change_signal_.connect(std::move(callback));
    }

    /// Coarse-grained: fires on ANY mutation, no details.
    auto watch(std::function<void()> callback) -> Connection {
        return watch_signal_.connect(std::move(callback));
    }

private:
    auto emit(ListChange<T> change) -> void {
        change_signal_.emit(change);
        watch_signal_.emit();
    }

    std::vector<T>                     items_;
    EventSignal<const ListChange<T>&>  change_signal_;
    EventSignal<>                      watch_signal_;
};

} // export namespace Nandina
