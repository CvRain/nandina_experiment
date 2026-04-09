module;
#include <cstddef>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

export module Nandina.Reactive.StateList;

import Nandina.Core.Signal;

export namespace Nandina {

    // ── ListChangeKind ────────────────────────────────────────────────────────
    enum class ListChangeKind {
        Inserted,   ///< One element was inserted at a given index
        Removed,    ///< One element was removed from a given index
        Updated,    ///< One element was replaced in-place
        Moved,      ///< One element was moved from old_index to index
        Reset,      ///< The entire list was replaced (assign_all / clear)
    };

    // ── ListChange<T> ─────────────────────────────────────────────────────────
    // Describes a single mutation to a StateList<T>.
    // Fields that are meaningful depend on `kind`:
    //
    //   Inserted:  index, value
    //   Removed:   index
    //   Updated:   index, value
    //   Moved:     index (new), old_index
    //   Reset:     (no extra fields; read the full list via StateList::items())
    template<typename T>
    struct ListChange {
        ListChangeKind    kind      = ListChangeKind::Reset;
        std::size_t       index     = 0;
        std::size_t       old_index = 0;          ///< only for Moved
        std::optional<T>  value     = std::nullopt; ///< for Inserted / Updated
    };

    // ── StateList<T> ─────────────────────────────────────────────────────────
    // An observable std::vector<T>. Mutations emit fine-grained ListChange<T>
    // events as well as a coarse-grained "something changed" notification.
    //
    // Example:
    //   StateList<std::string> list;
    //
    //   // Fine-grained subscription (per-operation events)
    //   auto conn = list.on_change([](const ListChange<std::string>& change){
    //       if (change.kind == ListChangeKind::Inserted) { ... }
    //   });
    //
    //   // Coarse-grained subscription (just "list changed")
    //   auto wconn = list.watch([](){ rebuild_ui(); });
    //
    //   list.push_back("hello");   // emits Inserted{index=0, value="hello"}
    //   list.remove_at(0);         // emits Removed{index=0}
    template<typename T>
    class StateList {
    public:
        StateList() = default;

        StateList(const StateList&) = delete;
        auto operator=(const StateList&) = delete;
        StateList(StateList&&) = delete;
        auto operator=(StateList&&) = delete;

        // ── Read access ───────────────────────────────────────────────────────

        [[nodiscard]] auto size() const noexcept -> std::size_t { return items_.size(); }
        [[nodiscard]] auto empty() const noexcept -> bool       { return items_.empty(); }

        [[nodiscard]] auto operator[](std::size_t i) const -> const T& { return items_[i]; }
        [[nodiscard]] auto at(std::size_t i) const -> const T&          { return items_.at(i); }

        [[nodiscard]] auto begin()  const noexcept { return items_.begin(); }
        [[nodiscard]] auto end()    const noexcept { return items_.end(); }
        [[nodiscard]] auto cbegin() const noexcept { return items_.cbegin(); }
        [[nodiscard]] auto cend()   const noexcept { return items_.cend(); }

        /// Direct read-only access to the underlying vector.
        [[nodiscard]] auto items() const noexcept -> const std::vector<T>& { return items_; }

        // ── Mutations ─────────────────────────────────────────────────────────

        /// Append an element at the end.
        auto push_back(T value) -> void {
            const std::size_t idx = items_.size();
            items_.push_back(std::move(value));
            emit({ ListChangeKind::Inserted, idx, 0, items_.back() });
        }

        /// Insert an element at position `index`. Elements after it shift right.
        auto insert(std::size_t index, T value) -> void {
            if (index > items_.size()) { index = items_.size(); }
            items_.insert(items_.begin() + static_cast<std::ptrdiff_t>(index), std::move(value));
            emit({ ListChangeKind::Inserted, index, 0, items_[index] });
        }

        /// Remove the element at `index`. Elements after it shift left.
        auto remove_at(std::size_t index) -> void {
            if (index >= items_.size()) { return; }
            items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(index));
            emit({ ListChangeKind::Removed, index, 0, std::nullopt });
        }

        /// Replace the element at `index` with `value`.
        auto update_at(std::size_t index, T value) -> void {
            if (index >= items_.size()) { return; }
            items_[index] = std::move(value);
            emit({ ListChangeKind::Updated, index, 0, items_[index] });
        }

        /// Move element from `from_index` to `to_index`.
        auto move_item(std::size_t from_index, std::size_t to_index) -> void {
            if (from_index == to_index || from_index >= items_.size()) { return; }
            if (to_index >= items_.size()) { to_index = items_.size() - 1; }
            T tmp = std::move(items_[from_index]);
            items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(from_index));
            items_.insert(items_.begin() + static_cast<std::ptrdiff_t>(to_index), std::move(tmp));
            emit({ ListChangeKind::Moved, to_index, from_index, std::nullopt });
        }

        /// Replace the entire list with `new_items`.
        auto assign_all(std::vector<T> new_items) -> void {
            items_ = std::move(new_items);
            emit({ ListChangeKind::Reset, 0, 0, std::nullopt });
        }

        /// Remove all elements.
        auto clear() -> void {
            items_.clear();
            emit({ ListChangeKind::Reset, 0, 0, std::nullopt });
        }

        // ── Subscription ─────────────────────────────────────────────────────

        /// Fine-grained subscription: callback receives a ListChange<T> describing
        /// the exact mutation. Returns a Connection; call disconnect() to unsubscribe.
        auto on_change(std::function<void(const ListChange<T>&)> callback) -> Connection {
            return change_signal_.connect(std::move(callback));
        }

        /// Coarse-grained subscription: callback is invoked on ANY mutation (no
        /// details). Useful when you just need to know "the list changed".
        auto watch(std::function<void()> callback) -> Connection {
            return watch_signal_.connect(std::move(callback));
        }

    private:
        auto emit(ListChange<T> change) -> void {
            change_signal_.emit(change);
            watch_signal_.emit();
        }

        std::vector<T>                  items_;
        EventSignal<const ListChange<T>&> change_signal_;
        EventSignal<>                   watch_signal_;
    };

} // export namespace Nandina
