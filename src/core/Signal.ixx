module;

#include <algorithm>
#include <cstddef>
#include <functional>
#include <mutex>
#include <utility>
#include <vector>

export module Nandina.Core.Signal;

export namespace Nandina {
    // ── Connection ────────────────────────────────────────────────────────────
    // Represents a single registered slot. Call disconnect() to unsubscribe.
    // Move-only; safe to store and pass around.
    class Connection {
    public:
        Connection() = default;

        Connection(std::function<void()> disconnect_fn, std::function<bool()> connected_fn)
            : disconnect_fn_(std::move(disconnect_fn)), connected_fn_(std::move(connected_fn)) {}

        Connection(const Connection &) = delete;
        auto operator=(const Connection &) -> Connection& = delete;
        Connection(Connection &&) noexcept = default;
        auto operator=(Connection &&) noexcept -> Connection& = default;

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
        std::function<void()>   disconnect_fn_;
        std::function<bool()>   connected_fn_;
    };

    // ── ScopedConnection ─────────────────────────────────────────────────────
    // RAII wrapper around Connection. Automatically disconnects on destruction.
    // Use when a slot should live exactly as long as the owning object.
    //
    // Example:
    //   ScopedConnection sc{ signal.connect([&]{ ... }) };
    //   // disconnects automatically when sc goes out of scope
    class ScopedConnection {
    public:
        ScopedConnection() = default;
        explicit ScopedConnection(Connection conn) : conn_(std::move(conn)) {}

        ScopedConnection(const ScopedConnection&) = delete;
        auto operator=(const ScopedConnection&) -> ScopedConnection& = delete;
        ScopedConnection(ScopedConnection&&) noexcept = default;
        auto operator=(ScopedConnection&&) noexcept -> ScopedConnection& = default;

        ~ScopedConnection() { conn_.disconnect(); }

        auto disconnect() -> void { conn_.disconnect(); }
        [[nodiscard]] auto connected() const -> bool { return conn_.connected(); }

        /// Release ownership; the caller is responsible for managing the connection.
        auto release() -> Connection { return std::move(conn_); }

    private:
        Connection conn_;
    };

    // ── EventSignal<Args...> ─────────────────────────────────────────────────
    // Thread-safe event signal. Unrelated to Nandina.Reactive's State<T> system.
    //
    // Thread safety:
    //   - connect / connect_once / disconnect / clear: exclusive lock
    //   - emit: snapshot slots under exclusive lock, then invoke without lock
    //
    // Safe to call disconnect() from within a slot callback (no re-entrant lock).
    //
    // Example:
    //   EventSignal<int> sig;
    //   auto conn = sig.connect([](int v){ ... });
    //   sig.emit(42);        // calls all active slots
    //   conn.disconnect();   // unsubscribe
    //
    //   // One-shot: slot fires once then disconnects automatically
    //   auto once = sig.connect_once([](int v){ ... });
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

        /// Register a slot that automatically disconnects after firing once.
        auto connect_once(Slot slot) -> Connection {
            std::lock_guard lock(mutex_);
            const std::size_t id = next_id_++;
            slots_.push_back(SlotEntry{id, true, true, std::move(slot)});
            return make_connection(id);
        }

        auto emit(Args... args) -> void {
            // Snapshot active slots under lock; don't hold lock during callbacks.
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
            // NOTE: called while mutex_ is already held; do NOT lock inside lambdas.
            return Connection(
                [this, id]() { disconnect(id); },
                [this, id]() { return is_connected(id); }
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
} // export namespace Nandina

