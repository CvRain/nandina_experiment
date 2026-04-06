module;

#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

export module Nandina.Core.Signal;

export namespace Nandina {
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

    // Event-based signal. Unrelated to Nandina.Reactive's State<T> system.
    template<typename... Args>
    class EventSignal {
    public:
        using Slot = std::function<void(Args...)>;

        auto connect(Slot slot) -> Connection {
            const std::size_t id = next_id_++;
            slots_.push_back(SlotEntry{id, true, std::move(slot)});
            return Connection(
                [this, id]() { disconnect(id); },
                [this, id]() { return is_connected(id); }
            );
        }

        auto emit(Args... args) -> void {
            for (auto& entry : slots_) {
                if (entry.active) { entry.slot(args...); }
            }
            cleanup();
        }

        auto clear() -> void { slots_.clear(); }

    private:
        struct SlotEntry {
            std::size_t id   = 0;
            bool        active = false;
            Slot        slot;
        };

        auto disconnect(std::size_t id) -> void {
            for (auto& e : slots_) {
                if (e.id == id) { e.active = false; break; }
            }
        }

        [[nodiscard]] auto is_connected(std::size_t id) const -> bool {
            for (const auto& e : slots_) {
                if (e.id == id) { return e.active; }
            }
            return false;
        }

        auto cleanup() -> void {
            std::erase_if(slots_, [](const SlotEntry& e) { return !e.active; });
        }

        std::size_t            next_id_ = 1;
        std::vector<SlotEntry> slots_;
    };
} // export namespace Nandina

