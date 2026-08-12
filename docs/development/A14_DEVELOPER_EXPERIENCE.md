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

## 1.0 Minimum Window Result

The 1.0 recommended entry point always starts from a typed page. A one-page program and a routed
application therefore use the same model:

```cpp
class MainPage final: public app::Page<> {
public:
    auto build(widget::BuildContext& ui) -> widget::View override {
        return ui.make<widget::Label>("Hello, Nandina!").build();
    }
};

auto main() -> int {
    return app::run<MainPage>({
        .id = "org.example.hello",
        .window = {.title = "Hello"},
    });
}
```

`Page<Params>` hides route type plumbing and adapts the framework-owned `PageContext` to the one
required `build(BuildContext&)` override. Adding navigation or parameterized pages does not change
`main()` or migrate the root to another runtime. Lambda root factories remain compatibility/test
helpers; tutorials and examples recommend `app::run<MainPage>()`. `NanApplication`, `NanWindow`,
`NanPageT`, and `NanRouter` remain advanced framework APIs rather than competing entry choices.

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
        auto input = ui.make<ui::TextField>("", "添加一个任务").build();
        auto tasks = ui.make<TodoTasks>().build();

        tasks->set_model(store().items);
        ui.connect(tasks->toggle_requested, [this](TodoId id) { toggle_task(id); });
        ui.connect(tasks->remove_requested, [this](TodoId id) { remove_task(id); });

        auto add = ui.make<ui::Button>("添加").build();
        add->on_click([this, input] { add_task(input->value()); });

        auto input_row = ui.make<ui::Row>().gap(8).build();
        input_row->add(input);
        input_row->add(add);

        auto root = ui.make<ui::Column>().gap(10).build();
        root->add(ui.make<ui::Label>("待办事项").font_size(24).build());
        root->add(input_row);
        root->add(tasks);
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

The DSL form uses the same `ui.make<T>()` customization and concrete controls. A builder becomes
composition syntax through `.children()`, `.child()`, bindings and constrained modifiers; it is not
a second runtime or a catalog of methods added to `BuildContext`:

```cpp
auto TodoPage::build(ui::BuildContext& ui) -> ui::View {
    auto& summary = ui.computed([this] { return store().summary(); });
    auto empty = ui.when(store().items.empty(), [](ui::BuildContext branch) {
        return branch.make<ui::Label>("暂无任务")
            .color_token(theme::ColorToken::muted_foreground);
    });
    auto tasks = ui.for_each(
        store().items,
        &TodoItem::id,
        [this](ui::BuildContext row, const TodoItem& item) {
            return row.make<TodoRow>(item)
                .configure([this, row](TodoRow& view) {
                    row.connect(view.toggle_requested, [this](TodoId id) { store().toggle(id); });
                    row.connect(view.remove_requested, [this](TodoId id) { store().remove(id); });
                });
        }
    );

    return ui.make<ui::Column>()
        .gap(10)
        .children(
            ui.make<ui::Label>("待办事项").font_size(24),
            ui.make<ui::Label>(summary),
            ui.make<ui::Row>().gap(8).children(
                ui.make<ui::TextField>(draft, "添加一个任务").width(ui::fill).autofocus(),
                ui.make<ui::Button>("添加").on_click(
                    [this] { store().add(std::exchange(draft, {})); }
                )
            ),
            empty,
            ui.make<ui::ScrollView>().child(tasks).height(ui::fill)
        );
}
```

Bindings accept values, `State`, `Signal`, and `Computed` without widget-specific `bind_*` plumbing. Theme tokens and semantic tones remain live across theme changes without component overrides.

`make<T>()` resolves component construction through the customization declared beside `T`. For
example, `ComponentTraits<Label>` injects the graph and active theme, while
`ComponentTraits<TextField>` additionally recognizes a string signal and installs its two-way
binding. Layout types use the same path. Adding `Badge` therefore adds its widget, recipe, traits,
tests and example usage, but no `BuildContext::badge()` declaration.

The include boundary mirrors that responsibility split. Ordinary pages that construct built-in
controls include `widget/controls.hpp`; it exposes `BuildContext`, the built-in control types, and
their typed traits as one application-facing entry. Custom component and framework code can include
only `widget/build_context.hpp`, which retains layout/data composition and scoped construction but
does not transitively parse the complete built-in control catalog. Low-level code may pair an
individual component header with `widget/authoring.hpp`. These layers are also the intended source
boundaries for the later C++ modules migration.

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

The paired imperative/DSL Todo served as the delivery fixture through A18. After A19 acceptance, its reusable contracts remain covered by focused authoring, router, lifecycle, and compact-application tests rather than a second shipped Todo implementation.

## Delivery Order

1. A14 establishes this contract, line budgets, forbidden dependencies, and `run_page()`.
2. A15 introduces typed list-model adapters and named commands first, then an explicit `BuildContext` carrying ambient build services, context-aware widget factories, safe router commands, and focus/layout intents.
3. A16 introduces the component/page ownership model and automatic reactive/lifecycle cleanup.
4. A17 completes generic setter binding, two-way text-field state, and live theme-token propagation.
5. A18 adds `BuildContext::when()` and `for_each()` as concise conditional and keyed collection authoring over the existing `IfRegion` and `ForEach` runtime.
6. A22 generalizes component construction through typed `make<T>()` customization and domain-layered public headers, so adding a component does not require editing `BuildContext`.
7. A23 closes retained-root callback lifetime with ReactiveScope generation tokens propagated through `BuildContext` authoring.

A22 starts with `Label` and `Button`: their traits inject the current graph/theme and preserve live
string bindings. Context-aware custom components retain the scoped constructor fallback. Other
controls migrate one family at a time, with focused tests and an example use before compatibility
factory methods can be retired.

The boolean-selection slice migrates `Checkbox` and `Switch` together because they share the same
silent setter plus user-originated event contract. Their traits own two-way `Signal<bool>` wiring;
Settings provides the corresponding visual and interaction check.

The value-input slice migrates `TextField` and `Slider`, preserving text edit publication, silent
source updates, bounded float configuration, and epsilon loop prevention. At this point all
interactive inputs in Settings use `make<T>()`, providing one visual acceptance surface for A22.

Because the project is still pre-1.0, the replaced named component factories are removed rather than
deprecated. This prevents two authoring spellings from diverging and makes new components available
exclusively through their traits. Generic low-level `authoring::make<T>()` remains available.

Each stage must shorten the canonical example and retain direct access to the existing concrete widgets for advanced use.

The A14 authoring API delivery order is complete through A18. The compact recommended Todo keeps ordinary application code focused on its Store, row component, page composition, and bootstrap. The earlier paired low-level fixture was retired after the same imperative/DSL equivalence, page-parameter, and lifecycle contracts were established in focused framework tests.

## Compact Reference Acceptance Snapshot

At A14/A19 acceptance, the compact Todo established the following counts using all non-blank source lines, including includes and declarations. The fixture was retired when A20 moved the canonical example to a Settings interface; these numbers remain the authoring-layer acceptance record rather than a promise that historical Todo files stay in the repository.

| Budget | Result | Limit |
| --- | ---: | ---: |
| Bootstrap (`compact_main.cpp`) | 23 | 30 |
| Data model and operations | 50 | 100 |
| Complete authoring form | 108 | 150 |
| Complete single-form Todo | 215 | 300 |

The compact root factory directly composed `Signal`, `Computed`, two-way text input, `when()`, keyed `for_each()`, focus and scroll intents, and concrete widgets. It did not declare a page class or route key; include scene-tree, dispatcher, frame-scheduler, or reactive-scope headers; override frame, layout, drawing, or theme lifecycle; or expose test-only widget getters. Those authoring contracts remain covered by focused framework tests and the current canonical example.
