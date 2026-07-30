# A14 Developer Experience Contract

## Purpose

Nandina keeps its retained scene graph and imperative widget API as an advanced, stable foundation, but ordinary application code must not be required to orchestrate that foundation. The 1.0 authoring layer is responsible for graph access, component scopes, theme propagation, safe navigation scheduling, keyed child lifetime, and routine post-layout work.

This document is an acceptance contract. The target examples intentionally describe APIs that will be delivered after A14; they are not all available yet. Each later authoring milestone must move the real Todo example toward these examples without creating a second renderer or widget object model.

## A14 Runnable Minimum

A normal routed application no longer needs a one-off `NanWindow` subclass. `run_page()` is the first delivered part of this contract:

```cpp
auto main() -> int {
    app::NanApplication application(
        app::NanApplicationConfig::for_process("org.example.hello")
    );
    return application.run_page<HomePage>(
        app::WindowConfig {.title = "Hello", .width = 640, .height = 400}
    );
}
```

Applications only inherit `NanWindow` when they need advanced window-level frame, setup, or teardown hooks.

## 1.0 Minimum Window Target

The final common-case entry point should not require a page class either:

```cpp
auto main() -> int {
    return app::run(
        {.id = "org.example.hello", .window = {.title = "Hello"}},
        [](ui::BuildContext& ui) { return ui.label("Hello, Nandina!"); }
    );
}
```

The application runner owns the application, default window, root page, UI context, and shutdown order. Explicit `NanApplication`, `NanWindow`, and `NanPageT` remain available for applications that need their lifetime boundaries.

## Imperative Todo Target

The imperative form uses concrete retained widgets and setters. It does not receive or forward a `Graph`, `NanTheme`, `ReactiveScope`, or `UiDispatcher`. Collection coordination belongs to a named `TodoTasks` component, so page code stays flat and resembles Qt's Model/View split:

| Todo role | Nandina responsibility | Qt-style analogy |
| --- | --- | --- |
| `TodoStore` | owns items and mutations | model |
| `TodoTasks` | binds the model and coordinates keyed rows | view / delegate host |
| `TodoRow` | renders one item and emits user intent | delegate/editor |
| `TodoPage` | composes controls and handles intents | controller/presenter |

```cpp
using TodoId = std::uint64_t;

class TodoTasks final: public ui::ListView<TodoItem> {
public:
    reactive::Event<TodoId> toggle_requested;
    reactive::Event<TodoId> remove_requested;

    void set_model(TodoStore::Items& items) {
        bind_items(items, &TodoItem::id);
    }
};

class TodoPage final: public ui::Page<TodoPageParams> {
public:
    auto build(ui::BuildContext& ui) -> ui::View override {
        auto input = ui.text_field({.placeholder = "添加一个任务"});
        auto tasks = ui.make<TodoTasks>();

        tasks->set_model(store().items);
        tasks->toggle_requested.subscribe([this](TodoId id) { toggle_task(id); });
        tasks->remove_requested.subscribe([this](TodoId id) { remove_task(id); });

        auto add = ui.button("添加");
        add->on_click([this, input] { add_task(input->value()); });

        auto root = ui.column({.gap = 10, .padding = 16});
        root->add(ui.label("待办事项").style("title"));
        root->add(ui.row(input.expand(), add));
        root->add(tasks->expand());
        input->request_focus();
        return root;
    }

private:
    void add_task(std::string_view title) { store().add(title); }
    void toggle_task(TodoId id) { store().toggle(id); }
    void remove_task(TodoId id) { store().remove(id); }
};
```

`TodoTasks` contains the row delegate. Its row buttons emit `toggle_requested` and `remove_requested`; they do not capture the page or mutate the store. The page only binds the model and connects three named intents. The event subscriptions, item scopes, and delayed focus/scroll requests are owned by the page/component scope and disconnected automatically when the page is dropped.

### What `bind_items` means

`bind_items(store().items, &TodoItem::id)` does not pass a snapshot to the widget and does not define button behavior:

- `store().items` is a read-only reactive source containing `std::vector<TodoItem>`; changes schedule a list synchronization.
- `&TodoItem::id` is the stable identity extractor. It lets the list reuse an existing row when an item moves or its title/status changes.
- The list component owns row creation, update, reorder, and teardown. The model owns data and business operations.

The current low-level `ForEach::bind(source)` plus key/create/update callbacks remains available for framework and advanced component authors. It is not the recommended application-facing spelling.

## DSL Todo Target

The DSL form creates the same concrete controls and uses the same ownership model. It is composition syntax, not a second runtime:

```cpp
auto TodoPage::build(ui::BuildContext& ui) -> ui::View {
    using namespace ui::dsl;
    return column({.gap = 10, .padding = 16},
        label("待办事项").style("title"),
        label(computed([this] { return store().summary(); })).tone(Tone::muted),
        row({.gap = 8},
            text_field(draft, {.placeholder = "添加一个任务"}).expand().autofocus(),
            button("添加").on_click([this] { store().add(std::exchange(draft, {})); })
        ),
        when(store().items.empty(), label("暂无任务").tone(Tone::muted)),
        scroll_view(for_each(store().items, &TodoItem::id, [this](const TodoItem& item) {
            return todo_row(item,
                [this, id = item.id] { store().toggle(id); },
                [this, id = item.id] { store().remove(id); });
        })).expand()
    );
}
```

Bindings accept values, `State`, `Signal`, and `Computed` without widget-specific `bind_*` plumbing. Theme tokens and semantic tones remain live across theme changes without component overrides.

## Acceptance Budget

The canonical Todo application must eventually satisfy all of these limits:

- bootstrap and first-page launch: at most 30 non-blank source lines;
- Todo data model and operations: at most 100 non-blank source lines;
- one complete authoring form: at most 150 non-blank source lines;
- complete single-form Todo, excluding theme data: at most 300 non-blank source lines;
- imperative and DSL forms share the same store, row component, behavior, and tests;
- ordinary page code does not include `scene_tree.hpp`, `frame_scheduler.hpp`, `ui_dispatcher.hpp`, or `reactive/scope.hpp`;
- ordinary page code does not override `on_process`, `on_layout`, `on_draw`, or `on_theme_changed`;
- navigation invoked by an event is safe without explicit task/post-layout scheduling;
- focus, scroll-to-end, keyed list teardown, and binding cleanup are expressed as intent.

The existing paired Todo remains a low-level equivalence and regression fixture while the compact Todo becomes the recommended application example.

## Delivery Order

1. A14 establishes this contract, line budgets, forbidden dependencies, and `run_page()`.
2. A15 introduces typed list-model adapters and named commands first, then an explicit `BuildContext` carrying ambient build services, context-aware widget factories, safe router commands, and focus/layout intents.
3. A16 introduces the component/page ownership model and automatic reactive/lifecycle cleanup.
4. A17 completes bindable widget properties and theme-token propagation.
5. A18 adds concise conditional and keyed collection authoring over the existing `IfRegion` and `ForEach` runtime.

Each stage must shorten the canonical example and retain direct access to the existing concrete widgets for advanced use.
