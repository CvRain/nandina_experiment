# Nandina v3 Development Notes

This document is the tracked development entry point for the current C++ v3 line.
The older `dev-docs-*` directories are local reference material and are ignored by git.
Use this file as the concise current-state guide before changing runtime, widget, or layout code.

## Current Direction

Nandina v3 is a C++26 + raylib UI runtime built around a Godot-style 2D scene tree.
The lower layers behave like a small game engine: nodes, transforms, lifecycle, input routing, hit testing, draw traversal, and replaceable render devices.
The upper layers rebuild a desktop/frontend authoring surface: reactive state, semantic widgets, page routing, app-level theme, and explicit layout controls.

The current authority is the code under `nandina/` plus tests under `tests/`. Older v1/v2 material is useful for semantics and naming, but it is not the active contract.

## Architectural Constraints

- Prefer zero RTTI wiring in framework code. Use virtual capability hooks such as `as_node2d()`, `as_control()`, `layout_flex_factor()`, or event type tags before `dynamic_cast`-style solutions.
- RTTI is not forbidden absolutely, but any use must be local, safe, and justified by a better tradeoff than adding another framework hook.
- Keep raylib out of public interfaces. Backend-specific types belong in backend implementation files.
- Use `shared_ptr` for node ownership and `weak_ptr` for parent/hover/focus/delete-queue observation.
- Keep low-level widget APIs valid. `Button::create(...)`, `Label::create(...)`, direct construction, and explicit `add_child` composition remain supported.
- Do not make root controls secretly behave like `Column`, `Center`, or another concrete layout. Use explicit controls such as `Center`, `Padding`, `Column`, `Row`, `Flex`, and `Flow`.
- `NanControl` may keep transitional single-child fill behavior, but concrete page layout should be explicit.

## Implemented Layers

| Layer | Current state |
| --- | --- |
| `foundation` | Geometry, color, color spaces, and decomposed `NanTransform2D`. |
| `scene` | `NanNode`, `NanNode2D`, `NanSceneTree`, input events, focus/hover, deferred delete, `NanControl`. |
| `render` | `IRenderDevice`, `DrawContext`, `ClipStack`, raylib backend factory. |
| `reactive` | `Graph`, `Signal`, `Computed`, `Effect`, `EffectScope`, `ReactiveScope`, batching. |
| `theme` | `NanTheme`, palette/tokens, button style resolver. |
| `widget` | Text/Label/Button primitives and low-level layout controls. |
| `app` | `NanApplication`, `NanWindow`, `NanRouter`, `NanPage`, `NanStore`, app theme propagation. |

## Layout System

The current layout protocol is based on a measure/layout pair:

- `scene::LayoutConstraints` carries min/max width/height.
- `NanControl::measure_layout(...)` measures bottom-up.
- `NanControl::layout_to(...)` assigns final rects top-down.
- `mark_layout_dirty()` propagates from child controls to ancestor controls.
- `NanWindow` lays out the root control to the current window size each frame.
- `NanRouter` uses a host control that fills the visible page root.

Implemented layout controls:

- `Row` and `Column`: convenience linear layouts.
- `Flex`: generic horizontal/vertical linear layout.
- `Expanded`: single-child flex wrapper; direct `Expanded` children receive remaining main-axis space.
- `Padding`: single-child padding wrapper.
- `Center`: single-child centering wrapper.
- `Wrap`: automatic run-based wrapping layout.
- `Flow`: alias of `Wrap` for semantic flow layout use.

Current layout capabilities:

- `LayoutAxis::{horizontal, vertical}`.
- `LayoutAlignment::{start, center, end, stretch}`.
- gap and run gap.
- main/cross alignment.
- basic cross-axis stretch.
- remaining-space distribution to direct `Expanded` children.
- wrap relayout based on actual assigned bounds, not only prior measure constraints.

Current layout limitations:

- No basis/shrink/min-max policy comparable to full CSS flexbox.
- No `space-between` / `space-around` main-axis strategies yet.
- No run stretch or per-child alignment in `Wrap` / `Flow`.
- No grid, anchors, scroll viewport, or clipping container yet.
- `Padding` does not model full content-box / border-box semantics.
- Layout dirty/cache invalidation is still coarse.
- Default `NanControl::on_layout()` direct-child behavior is transitional.

## Text And Label

The current text stack is intentionally minimal:

- `Text` supports text, color, font size, estimated measurement, `TextOverflow`, and `max_lines`.
- `Label` is the semantic text control and can bind to `Signal<std::string>` or `Computed<std::string>`.
- `Button` still draws text through `IRenderDevice::draw_text` using estimated width.

Text limitations:

- Width is estimated from character count.
- `ellipsis` uses ASCII `...`.
- `clip` truncates by estimated character count, not render clipping.
- `wrap` affects measurement but is not real multi-line text rendering.
- No shaping, kerning, bidi, CJK, emoji, baseline model, glyph runs, or font cache yet.

Text overflow should become a first-class layout result, not a late patch inside individual widgets.

## App Authoring State

The current example is a standard counter page using:

- `NanApplication` with app-level `NanTheme`.
- `NanWindow` and `NanRouter`.
- `CounterPage : NanPageT<NoParams>`.
- page-scoped `Signal<int>` plus `Computed<std::string>`.
- `Label` bound to computed text.
- `Button` controls for decrement/increment/reset.
- explicit `Center -> Padding -> Column -> Flow` layout.

Example code is not sacred. It can be refactored when it helps validate framework APIs, unless a task explicitly asks to preserve it.

## Reactive Lifetime

Page-local reactive state is owned by `reactive::ReactiveScope`.
`PageContext::scope()` returns the scope for the page frame currently being built.
The router keeps that scope alive for as long as the page frame is mounted or kept alive.

`ReactiveScope` can own:

- `Signal<T>` values through `scope.signal<T>(...)` or `scope.signal_value(...)`.
- `Computed<T>` values through `scope.computed(fn)`.
- `Effect` callbacks through `scope.effect(fn)`.

Router frame teardown detaches the page root first, allowing widgets to run `on_exit_tree()`, then clears the page scope.
This prevents page-local computed/effect callbacks from surviving the page object that they may capture.

## Recommended Next Steps

1. Push text layout forward: real font metrics, layout result, single/multi-line drawing, and overflow behavior.
2. Add container clipping/overflow contract using the existing `ClipStack` and draw traversal.
3. Refine layout semantics: basis/shrink/min-max policy, main-axis spacing strategies, run stretch, and per-child alignment.
4. Add more semantic controls only after text/layout behavior is stable enough to support them.
5. Build authoring DSL later as a layer above the low-level widget API, not as a replacement for it.

## Verification

For normal framework changes run:

```sh
meson test -C buildDir
meson compile -C buildDir nandina_example
```

Before committing, inspect status and exclude unrelated generated files such as `firebase-debug.log`.
