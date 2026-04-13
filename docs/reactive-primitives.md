# Reactive Primitives — Prop, StateList, and Signal

This document describes the three reactive primitives added in this milestone and how to use them.

---

## 1. `Prop<T>` — Unified property wrapper

**Module:** `Nandina.Reactive`

`Prop<T>` provides a single type that a component attribute can accept regardless of whether the value is static or reactive. This eliminates the need for separate `set_text()` / `bind_text()` pairs and lets component props prefer read-only reactive input.

```cpp
import Nandina.Reactive;

using namespace Nandina;

// Static — value never changes
Prop<std::string> title{ std::string{"Hello, world!"} };

// Reactive — reads from a ReadState<std::string>
State<std::string> name{"Nandina"};
Prop<std::string> greeting{ name.as_read_only() };

// Access the current value (works for both)
auto v = greeting.get();          // -> const std::string&

// Query whether it's bound to a reactive source
greeting.is_reactive();           // -> bool

// Subscribe to changes
// - Reactive: returns a live Connection
// - Static:   returns a no-op Connection (connected() == false)
auto conn = greeting.on_change([](const std::string& v) {
    // ...
});

// Update the underlying State to trigger subscribers
name.set("World");
```

### Design notes

- `Prop<T>` holds a static value or a non-owning pointer to a reactive source derived from `State<T>` / `ReadState<T>`. The caller must ensure the owning `State<T>` outlives the `Prop`.
- `get()` inside an `Effect` auto-registers the effect as an observer (reads from the bound State transparently).
- Component props should prefer `Prop<T>` or `ReadState<T>` so child components consume read-only inputs by default.
- The reactive core now restores tracking context with RAII guards, so exceptions inside `Effect` / `Computed` callbacks do not leave the thread-local tracking state corrupted.

---

## 2. `StateList<T>` — Observable vector

**Module:** `Nandina.Reactive`

`StateList<T>` wraps a `std::vector<T>` and emits structured change events on every mutation.

### Change event types

| `ListChangeKind` | Meaningful fields |
|---|---|
| `Inserted` | `index`, `value` |
| `Removed` | `index` |
| `Updated` | `index`, `value` |
| `Moved` | `index` (new), `old_index` |
| `Reset` | _(none — read `items()`)_ |

### Mutations

```cpp
StateList<std::string> list;

list.push_back("A");              // Inserted{index=0, value="A"}
list.insert(0, "Z");              // Inserted{index=0, value="Z"}
list.update_at(1, "A2");          // Updated{index=1, value="A2"}
list.remove_at(0);                // Removed{index=0}
list.move_item(0, 1);             // Moved{old_index=0, index=1}
list.assign_all({...});           // Reset
list.clear();                     // Reset
```

### Subscriptions

```cpp
// Fine-grained: full ListChange<T> event
auto conn = list.on_change([](const ListChange<std::string>& ch) {
    if (ch.kind == ListChangeKind::Inserted) {
        add_widget(ch.index, ch.value.value());
    } else if (ch.kind == ListChangeKind::Removed) {
        remove_widget(ch.index);
    }
});

// Coarse-grained: just "something changed"
ScopedConnection sc{ list.watch([&]{ rebuild_all(); }) };
```

### Read access

```cpp
list.size();
list.empty();
list[0];
list.at(0);                       // bounds-checked
list.items();                     // const std::vector<T>&
for (const auto& item : list) { ... }
```

### `tracked_*` bridge helpers

`StateList<T>` intentionally keeps its structured change stream separate from the automatic `State<T>` / `Effect` dependency graph.

That separation is useful for list-heavy UI because it preserves precise mutation semantics:

- `Inserted` means you can mount one new row.
- `Removed` means you can unmount one row.
- `Moved` means you can reorder existing row widgets.
- `Updated` means you can refresh one row in place.

At the same time, some callers only need a coarse derived value such as:

- item count
- empty/non-empty state
- a filtered snapshot for summary text
- a simple "version changed" invalidation hook

For those cases, `StateList<T>` now exposes explicit bridge helpers:

```cpp
auto version = list.version();        // ReadState<std::size_t>
auto count = list.tracked_size();     // std::size_t, tracks coarse list version
auto empty = list.tracked_empty();    // bool, tracks coarse list version
auto items = list.tracked_items();    // const std::vector<T>&, tracks coarse list version
```

These helpers let a list participate in `Computed` / `Effect` recomputation without changing the core meaning of `StateList<T>`.

```cpp
State<std::string> search_text{""};
StateList<std::string> files;

Computed match_count{[&] {
    auto keyword = search_text();
    const auto& items = files.tracked_items();

    return std::ranges::count_if(items, [&](const std::string& file) {
        return keyword.empty() || file.contains(keyword);
    });
}};

Effect show_summary{[&] {
    std::println("matched files = {}", match_count());
}};
```

### When to use `on_change` vs `tracked_*`

Use `on_change` / `watch` when you need exact structure-aware UI updates:

```cpp
auto conn = files.on_change([&](const ListChange<FileNode>& ch) {
    switch (ch.kind) {
    case ListChangeKind::Inserted:
        list_view->insert_row(ch.index, make_row(*ch.value));
        break;
    case ListChangeKind::Removed:
        list_view->remove_row(ch.index);
        break;
    case ListChangeKind::Updated:
        list_view->update_row(ch.index, *ch.value);
        break;
    case ListChangeKind::Moved:
        list_view->move_row(*ch.old_index, ch.index);
        break;
    case ListChangeKind::Reset:
        list_view->rebuild_all(files.items());
        break;
    }
});
```

Use `tracked_*` when you only need a coarse derived value and are comfortable rerunning the whole derived computation:

```cpp
Computed empty_hint{[&] {
    return files.tracked_empty() ? std::string{"This folder is empty"}
                                 : std::string{""};
}};
```

Recommended rule:

- Use `State<T>` for scalar reactive state.
- Use `StateList<T>` for structure-aware collections.
- Use `tracked_*` only as an explicit bridge from collection state back into the scalar reactive graph.

---

## 3. Upgraded `EventSignal<>` — Thread safety, connect_once, ScopedConnection

**Module:** `Nandina.Reactive`

### Thread safety

`EventSignal<>` is now thread-safe:

- `connect` / `connect_once` / `disconnect` / `clear` — protected by a `std::mutex`.
- `emit` — snapshots the active slot list under a lock, then invokes callbacks **without** holding the lock. This prevents deadlocks when a callback itself calls `connect` or `disconnect`.

### `connect_once`

A slot registered with `connect_once` fires exactly once and is automatically disconnected afterward.

```cpp
EventSignal<int> sig;

// Fires once, then auto-disconnects
auto conn = sig.connect_once([](int v) {
    std::println("got {} (will not fire again)", v);
});

sig.emit(1);  // slot fires, auto-disconnected
sig.emit(2);  // slot does NOT fire
```

### `ScopedConnection`

`ScopedConnection` is an RAII wrapper around `Connection`. When it goes out of scope (or is destroyed), it automatically calls `disconnect()`.

```cpp
class MyComponent {
    ScopedConnection on_change_;

    void setup(EventSignal<int>& sig) {
        on_change_ = ScopedConnection{ sig.connect([this](int v){ update(v); }) };
        // disconnects automatically when MyComponent is destroyed
    }
};
```

You can also use it to limit a subscription to a specific scope:

```cpp
{
    ScopedConnection sc{ sig.connect([](int v){ ... }) };
    sig.emit(42);  // slot fires
    // sc goes out of scope — slot disconnects
}
sig.emit(99);  // slot does NOT fire
```

### `State<T>::on_change`

In addition to the Effect-based reactive system, `State<T>` now exposes a direct subscription API. Unlike Effects, `on_change` subscriptions are **persistent** (they are not cleared on each notification).

```cpp
State<int> counter{0};

auto conn = counter.on_change([](const int& v) {
    std::println("counter changed to {}", v);
});

counter.set(1);   // -> "counter changed to 1"
counter.set(1);   // no-op: same value
counter.set(2);   // -> "counter changed to 2"

conn.disconnect();
counter.set(3);   // (silent — connection was disconnected)
```

---

## 4. Minimal batch updates

### Problem

Multiple consecutive `State::set()` calls can cause the same `Effect` to rerun multiple times and briefly observe intermediate UI state.

```cpp
selected_id.set("file-a");
preview_name.set("file-a");
preview_loading.set(true);

// A dependent effect may run 3 times.
```

### API shape

```cpp
batch([&] {
    selected_id.set("file-a");
    preview_name.set("file-a");
    preview_loading.set(true);
});
```

The current semantics are deliberately minimal:

- `set()` inside `batch(...)` marks observers dirty but does not immediately rerun effects.
- At the end of the outer batch, dirty invalidators are deduplicated and flushed once.
- Effects observe the final consistent state, not intermediate states.
- Exceptions raised during flush are rethrown after the batch performs the required cleanup.
- This is closer to a UI commit/batch than a full database transaction with rollback.

### Pseudo example: file manager selection

```cpp
batch([&] {
    selected_entry_id.set("report-final.txt");
    breadcrumb_text.set("/docs/report-final.txt");
    status_text.set("Loading preview...");
    preview_visible.set(true);
});
```

Without batching, four updates may cause repeated recomputation. With batching, the UI sees one coherent commit.

### Current scope

The current implementation intentionally stays narrow:

- Batch `State<T>` observer flushing.
- Deduplicate `Effect` and `Computed` invalidator reruns.
- Do not try to merge or fold multiple `StateList<T>` diffs in the same batch.

That keeps the model simple while solving the most common UI consistency problem.

---

## Demo page

`DemoPage` in `example/pages/DemoPage.ixx` demonstrates a realistic list-selection flow at runtime:

- `StateList<int>` drives a small list view.
- `tracked_*` helpers feed summary and selected-item derived text.
- `ReadState<T>` / `Prop<T>` drive labels without exposing writable state to child components.
- `batch(...)` is used when adding a new item so selection, status text, and next value update as one commit.

Run the application with:
```sh
cmake --preset debug-vcpkg && cmake --build --preset debug-vcpkg
./build/debug-vcpkg/bin/NandinaExperiment
```
