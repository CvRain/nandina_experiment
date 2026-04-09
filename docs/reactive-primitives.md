# Reactive Primitives — Prop, StateList, and Signal

This document describes the three reactive primitives added in this milestone and how to use them.

---

## 1. `Prop<T>` — Unified property wrapper

**Module:** `Nandina.Reactive.Prop`  
**Re-exported by:** `Nandina.Reactive`

`Prop<T>` provides a single type that a component attribute can accept regardless of whether the value is static or reactive. This eliminates the need for separate `set_text()` / `bind_text()` pairs.

```cpp
import Nandina.Reactive;

using namespace Nandina;

// Static — value never changes
Prop<std::string> title{ std::string{"Hello, world!"} };

// Reactive — reads from a State<std::string>
State<std::string> name{"Nandina"};
Prop<std::string> greeting{ name };

// Access the current value (works for both)
auto v = greeting.get();          // -> const std::string&

// Query whether it's bound to a State
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

- `Prop<T>` holds a `std::variant<T, State<T>*>`. The reactive variant stores a **non-owning pointer** — the caller must ensure `State<T>` outlives the `Prop`.
- `get()` inside an `Effect` auto-registers the effect as an observer (reads from the bound State transparently).

---

## 2. `StateList<T>` — Observable vector

**Module:** `Nandina.Reactive.StateList`  
**Re-exported by:** `Nandina.Reactive`

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

---

## 3. Upgraded `EventSignal<>` — Thread safety, connect_once, ScopedConnection

**Module:** `Nandina.Core.Signal`  
**Re-exported by:** `Nandina.Reactive`

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

## Demo page

`DemoPage` in `example/application_window.ixx` demonstrates all three primitives at runtime:

- **Section A** — `Prop<std::string>`: a static label and a reactive label that updates when a `State<std::string>` changes (click "Toggle reactive text").
- **Section B** — `StateList<std::string>`: three item slots reflect push/update/remove operations (click "Push item", "Update [0]", "Remove last").
- **Section C** — `EventSignal<int>` + `ScopedConnection` + `connect_once`: clicking "Fire signal" emits an event; a persistent `ScopedConnection` updates the status label, while a fresh `connect_once` slot is registered before each emit to track one-shot receptions.

Run the application with:
```sh
cmake --preset debug-vcpkg && cmake --build --preset debug-vcpkg
./build/debug-vcpkg/bin/NandinaExperiment
```
