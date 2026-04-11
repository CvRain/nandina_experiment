#include <cassert>
#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

import Nandina.Reactive;
import Nandina.Components.Label;

namespace {

// ── Core tracking and invalidation semantics ───────────────────────────────

auto test_effect_deduplicates_reads() -> void {
    Nandina::State<int> value{0};
    int runs = 0;

    Nandina::Effect effect{[&] {
        ++runs;
        (void)value();
        (void)value();
    }};

    assert(runs == 1);
    value.set(1);
    assert(runs == 2);
}

auto test_batch_deduplicates_effect_reruns() -> void {
    Nandina::State<int> left{1};
    Nandina::State<int> right{2};
    int runs = 0;
    int snapshot = 0;

    Nandina::Effect effect{[&] {
        ++runs;
        snapshot = left() + right();
    }};

    assert(runs == 1);
    assert(snapshot == 3);

    Nandina::batch([&] {
        left.set(10);
        right.set(20);
    });

    assert(runs == 2);
    assert(snapshot == 30);
}

auto test_nested_batch_flushes_once_at_outer_boundary() -> void {
    Nandina::State<int> value{0};
    int runs = 0;

    Nandina::Effect effect{[&] {
        ++runs;
        (void)value();
    }};

    assert(runs == 1);

    Nandina::batch([&] {
        value.set(1);
        Nandina::batch([&] {
            value.set(2);
            value.set(3);
        });
        assert(runs == 1);
    });

    assert(runs == 2);
    assert(value.get() == 3);
}

auto test_state_notify_restores_observers_after_exception() -> void {
    Nandina::State<int> value{0};
    bool should_throw = true;
    int throwing_runs = 0;
    int stable_runs = 0;

    Nandina::EffectScope scope;
    scope.add([&] {
        ++throwing_runs;
        (void)value();
        if (should_throw && value.get() == 1) {
            throw std::runtime_error{"effect failure"};
        }
    });
    scope.add([&] {
        ++stable_runs;
        (void)value();
    });

    assert(throwing_runs == 1);
    assert(stable_runs == 1);

    try {
        value.set(1);
        assert(false && "expected effect exception");
    } catch (const std::runtime_error&) {
    }

    should_throw = false;
    value.set(2);

    assert(throwing_runs == 3);
    assert(stable_runs == 2);
}

// ── Derived state semantics ────────────────────────────────────────────────

auto test_computed_recomputes_lazily() -> void {
    Nandina::State<int> left{2};
    Nandina::State<int> right{3};
    int recomputes = 0;

    Nandina::Computed sum{[&] {
        ++recomputes;
        return left() + right();
    }};

    assert(sum() == 5);
    assert(sum() == 5);
    assert(recomputes == 1);

    left.set(10);
    assert(sum() == 13);
    assert(recomputes == 2);
}

auto test_computed_retries_after_exception() -> void {
    Nandina::State<int> value{0};
    bool should_throw = true;
    int recomputes = 0;

    Nandina::Computed doubled{[&] {
        ++recomputes;
        if (value() == 1 && should_throw) {
            throw std::runtime_error{"computed failure"};
        }
        return value() * 2;
    }};

    assert(doubled() == 0);
    value.set(1);

    try {
        (void)doubled();
        assert(false && "expected computed exception");
    } catch (const std::runtime_error&) {
    }

    should_throw = false;
    assert(doubled() == 2);
    assert(recomputes == 3);
}

auto test_read_state_is_copyable_and_tracks() -> void {
    Nandina::State<int> value{5};
    Nandina::ReadState<int> read_value = value.as_read_only();
    auto read_value_copy = read_value;
    int runs = 0;

    Nandina::Effect effect{[&] {
        ++runs;
        assert(read_value_copy() == value());
    }};

    assert(runs == 1);
    value.set(9);
    assert(runs == 2);
}

// ── Read-only input model semantics ────────────────────────────────────────

auto test_prop_supports_static_and_read_only_sources() -> void {
    const Nandina::Prop<std::string> static_prop{std::string{"static"}};
    bool static_called = false;
    auto static_conn = static_prop.on_change([&](const std::string&) {
        static_called = true;
    });

    assert(static_prop.get() == "static");
    assert(!static_prop.is_reactive());
    assert(!static_conn.connected());
    assert(!static_called);

    Nandina::State<std::string> name{"alpha"};
    auto read_name = name.as_read_only();
    const Nandina::Prop<std::string> reactive_prop{read_name};
    auto copied_prop = reactive_prop;
    int hits = 0;

    auto reactive_conn = copied_prop.on_change([&](const std::string& value) {
        ++hits;
        assert(value == "beta");
    });

    assert(copied_prop.is_reactive());
    assert(copied_prop.get() == "alpha");
    name.set("beta");
    assert(hits == 1);
    assert(reactive_conn.connected());
}

auto test_prop_disconnect_stops_notifications() -> void {
    Nandina::State<std::string> text{"alpha"};
    Nandina::Prop<std::string> prop{text.as_read_only()};
    int hits = 0;

    auto conn = prop.on_change([&](const std::string&) {
        ++hits;
    });

    text.set("beta");
    assert(hits == 1);

    conn.disconnect();
    text.set("gamma");
    assert(hits == 1);
}

auto test_label_create_with_static_prop_sets_initial_text() -> void {
    auto label = Nandina::Label::Create(Nandina::Prop<std::string>{std::string{"hello"}});

    assert(label->text_content() == std::string_view{"hello"});
}

auto test_label_bind_text_from_read_state_updates_text() -> void {
    Nandina::State<std::string> text{"alpha"};
    auto label = Nandina::Label::Create(Nandina::Prop<std::string>{text.as_read_only()});

    assert(label->text_content() == std::string_view{"alpha"});
    text.set("beta");
    assert(label->text_content() == std::string_view{"beta"});
}

auto test_label_bind_text_with_read_state_formatter_updates_text() -> void {
    Nandina::State<int> count{2};
    auto label = Nandina::Label::Create();
    label->bind_text(count.as_read_only(), [](int value) {
        return std::to_string(value * 10);
    });

    assert(label->text_content() == std::string_view{"20"});
    count.set(3);
    assert(label->text_content() == std::string_view{"30"});
}

// ── Direct subscription semantics ──────────────────────────────────────────

auto test_event_signal_continues_cleanup_and_rethrows() -> void {
    Nandina::EventSignal<int> signal;
    std::vector<int> visited;

    auto persistent = signal.connect([&](int value) {
        visited.push_back(value);
    });

    signal.connect_once([&](int) {
        visited.push_back(99);
        throw std::runtime_error{"boom"};
    });

    auto trailing = signal.connect([&](int value) {
        visited.push_back(value * 10);
    });

    try {
        signal.emit(7);
        assert(false && "expected signal exception");
    } catch (const std::runtime_error&) {
    }

    assert((visited == std::vector<int>{7, 99, 70}));
    visited.clear();

    signal.emit(8);
    assert((visited == std::vector<int>{8, 80}));
    assert(persistent.connected());
    assert(trailing.connected());
}

// ── Observable collection semantics ────────────────────────────────────────

auto test_state_list_bridge_tracks_effects() -> void {
    Nandina::StateList<std::string> list;
    int runs = 0;
    std::string snapshot;

    Nandina::Effect effect{[&] {
        ++runs;
        snapshot.clear();
        for (const auto& item : list.tracked_items()) {
            snapshot += item;
        }
    }};

    assert(runs == 1);
    assert(snapshot.empty());

    list.push_back("A");
    assert(runs == 2);
    assert(snapshot == "A");

    list.push_back("B");
    assert(runs == 3);
    assert(snapshot == "AB");

    list.update_at(0, "Z");
    assert(runs == 4);
    assert(snapshot == "ZB");
}

auto test_state_list_emits_fine_and_coarse_notifications() -> void {
    Nandina::StateList<std::string> list;
    std::vector<Nandina::ListChangeKind> changes;
    int watch_hits = 0;

    auto change_conn = list.on_change([&](const Nandina::ListChange<std::string>& change) {
        changes.push_back(change.kind);
    });
    auto watch_conn = list.watch([&] {
        ++watch_hits;
    });

    list.push_back("A");
    list.update_at(0, "B");
    list.remove_at(0);

    assert((changes == std::vector<Nandina::ListChangeKind>{
        Nandina::ListChangeKind::Inserted,
        Nandina::ListChangeKind::Updated,
        Nandina::ListChangeKind::Removed
    }));
    assert(watch_hits == 3);
    assert(change_conn.connected());
    assert(watch_conn.connected());
}

} // namespace

auto main() -> int {
    // Tracking, recomputation, and exception safety
    test_effect_deduplicates_reads();
    test_batch_deduplicates_effect_reruns();
    test_nested_batch_flushes_once_at_outer_boundary();
    test_state_notify_restores_observers_after_exception();
    test_computed_recomputes_lazily();
    test_computed_retries_after_exception();

    // Read-only input model
    test_read_state_is_copyable_and_tracks();
    test_prop_supports_static_and_read_only_sources();
    test_prop_disconnect_stops_notifications();
    test_label_create_with_static_prop_sets_initial_text();
    test_label_bind_text_from_read_state_updates_text();
    test_label_bind_text_with_read_state_formatter_updates_text();

    // Direct subscriptions and collection notifications
    test_event_signal_continues_cleanup_and_rethrows();
    test_state_list_bridge_tracks_effects();
    test_state_list_emits_fine_and_coarse_notifications();
    return 0;
}