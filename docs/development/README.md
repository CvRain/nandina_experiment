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
- `LayoutAlignment::{start, center, end, stretch, space_between}`.
- gap and run gap.
- main/cross alignment.
- basic cross-axis stretch.
- remaining-space distribution to direct `Expanded` children.
- wrap relayout based on actual assigned bounds, not only prior measure constraints.

Current layout limitations:

- No basis/shrink/min-max policy comparable to full CSS flexbox.
- No `space-around` main-axis strategy yet; `space_between` currently targets fixed Flex/Row/Column children.
- No run stretch or per-child alignment in `Wrap` / `Flow`.
- No grid, anchors, or scroll viewport yet; controls now support child-subtree clipping.
- `Padding` does not model full content-box / border-box semantics.
- Layout dirty/cache invalidation is still coarse.
- Default `NanControl::on_layout()` direct-child behavior is transitional.

## Text And Label

The current text stack is intentionally minimal:

- `Text` supports text, color, font size, estimated measurement, `TextOverflow`, and `max_lines`.
- `Label` is the semantic text control and can bind to `Signal<std::string>` or `Computed<std::string>`.
- `Button` measures and draws through an internal `Text` primitive.

Text limitations:

- Width is estimated from character count.
- `ellipsis` uses ASCII `...`.
- `clip` truncates by estimated character count, not render clipping.
- `wrap` produces deterministic codepoint-based lines and draws them separately; word/grapheme-aware wrapping is not implemented yet.
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

## Development Roadmap

This roadmap merges the current code state, the useful parts of `dev-docs-*`, and the recent implementation decisions.
The main line should stay narrow: finish text/layout/clip foundations before adding many new semantic controls.

### Completed Foundation

The current baseline is:

1. `reactive::ReactiveScope` owns page-local `Signal`, `Computed`, and `Effect` lifetimes.
2. `PageContext::scope()` exposes the page frame scope.
3. `NanRouter` owns one scope per keep-alive page frame and clears it when the frame is popped or cleared.
4. `CounterPage` uses page scope and no longer manually disposes `Computed`.
5. Router/reactive tests cover scope teardown and page-frame cleanup.

This closes the first lifecycle risk. New work should not reintroduce page-local reactive objects that outlive their page scope.

### Main Line

#### M1. Text Capability Contract

Goal: make text a reusable primitive capability instead of logic copied into each text-bearing component.

Status: initial foundation landed. `TextStyle` is now the shared style value for `Text`, `Label` still layers on top of `Text`, and `Button` measures/draws through an internal `primitives::Text` instead of maintaining its own text measurement path.

Tasks:

1. Introduce `TextStyle` as a shared value object for text visual/layout inputs.
   It should start small: font size, color, overflow, max lines, and later font family, weight, line height, alignment, and baseline policy.
2. Move `Button` away from manual `text_.size() * font_size` measurement.
   Button should consume `primitives::Text` or a small `TextSlot` internally.
3. Keep `Label` as a semantic wrapper over `Text`, not a separate text system.
4. Give text-bearing controls a shared text styling path.
   Common setters such as `set_text(...)` can remain on semantic controls, but they should forward to their text primitive/slot.
5. Keep low-level escape hatches possible, for example `button->text_node()` or `button->text_slot()`, but do not make that the only normal API.

Boundary:

- `Text` owns UTF-8 text, style, measurement, layout result, and drawing.
- `Label` owns label semantics.
- `Button` owns press/click semantics and maps button state to a resolved text style.
- `TextField` / `EditableText` will own editing state later; ordinary `Text` should not become an editor.

#### M2. Text Layout Result

Goal: make measuring and drawing use the same computed layout data.

Status: the deterministic layout contract now produces independent UTF-8-safe lines for width wrapping and explicit newlines, and measure/draw consume the same line results. `ITextLayoutBackend` separates `Text` from the fallback codepoint-width algorithm and provides the integration boundary for a future FreeType/HarfBuzz backend. UTF-8 codepoint encoding, counting, boundary-safe truncation, and editing deletion are available; grapheme clusters, word-aware wrapping, shaping, and a richer baseline policy remain future work.

The FreeType foundation loads font faces and exposes pixel font metrics, glyph metrics, and grayscale glyph bitmaps. A CPU glyph atlas packs and caches those bitmaps, while `IRenderDevice` and the Raylib backend support alpha texture upload, revision updates, and tinted region drawing. `HarfBuzzTextLayoutBackend` emits glyph IDs, source clusters, advances, and offsets, and `GlyphRunRenderer` draws those runs through the atlas. `Text` can now consume an injected glyph renderer end to end, while its default path remains the deterministic backend plus render-device text fallback. Cluster-aware wrapping/ellipsis, fallback fonts, resource paths, and application-level font selection remain future work.

The first bundled-font build path uses the OFL-licensed Caskaydia Cove Regular from a pinned submodule. Meson option `bundled_fonts` copies the TTF and license into `example/res/fonts`, and integration tests load, shape, and rasterize the copied font. Sarasa Gothic SC remains the intended CJK default candidate, but its current regional release packages are large 7z archives; it should enter through a later font profile/download-cache design rather than slowing every initial configure.

Tasks:

1. Introduce `TextLayoutInput` and `TextLayoutResult`.
2. Include measured size, laid-out lines, visible text ranges, effective font size, baseline information, and overflow result.
3. Replace ad-hoc `laid_out_text_` / `laid_out_font_size_` state with the layout result.
4. Keep the first backend simple and deterministic; exact shaping can come later.
5. Ensure `Button`, `Label`, and future text consumers all measure through this contract.

This is where the previous iterations failed hardest: text overflow must not be patched separately in Button, Label, Card, Field, and TextField.

#### M3. Container Clip / Overflow Contract

Goal: make parent-declared clipping a tree/rendering contract, not a local widget trick.

Status: initial tree-level contract landed. `NanControl` now exposes `ControlOverflow::{visible, clip}`, and draw traversal pushes a child clip through `ClipStack` when a control declares clipped overflow.

Note: `ControlOverflow` clips child scene nodes. Text-bearing semantic controls that draw composed internal text, such as `Button`, should expose their own text overflow API that forwards to `TextOverflow`.

Tasks:

1. Add explicit overflow policy to controls or a focused container primitive.
2. Support at least `visible` and `clip` first.
3. Use the existing `ClipStack` in draw traversal so children are clipped by parent policy.
4. Connect `TextOverflow::clip` to render clipping rather than string truncation only.
5. Keep the path open for `ScrollView`, `Card`, `Dialog`, and popover-like containers.

#### M4. EditableText / TextField

Goal: build input on top of the text layout contract, not beside it.

Status: initial `EditableText` primitive landed, and a thin semantic `TextField` shell is in progress. `EditableText` owns editable value/caret/focus handling and reuses `Text` for layout and drawing; `TextField` composes surface, placeholder, and editable text behavior.

Tasks:

1. Add an `EditableText` primitive or internal control for caret, selection, IME/text input, and editing commands.
2. Build `TextField` as Surface + EditableText + placeholder Text + FocusRing + editing state machine.
3. Keep `TextField` responsible for value, placeholder, focus, read-only, disabled, invalid, on_change, and on_submit.
4. Keep Field/Form-level label/helper/error/required semantics outside `TextField`.

#### M5. Layout Refinement

Goal: refine the existing layout primitives after text and clip semantics are stable.

Status: first refinement in progress. Flex/Row/Column now support `LayoutAlignment::space_between` for fixed-size children; flex basis/shrink and grid/scroll primitives remain future work.

Tasks:

1. Add flex basis/shrink/min-max policy where needed.
2. Add main-axis spacing strategies such as `space_between` only when a real page/control needs them.
3. Add run stretch and per-child alignment to `Wrap` / `Flow`.
4. Decide whether Grid, Anchor, or Scroll viewport enters the low-level widget set next.
5. Keep Yoga or any third-party solver behind a backend boundary; it should not define the public widget API.

### Side Tracks

These are useful, but should not interrupt the main text/layout/clip line unless a feature directly requires them.

#### Theme Context

Current `NanApplication` owns a `NanTheme` value. Runtime theme switching is not reactive yet.

Possible direction:

- Add `ThemeContext` or `Signal<NanTheme>`.
- Let widgets derive resolved styles through effects or scoped subscriptions.
- Make window background and existing page widgets refresh when theme changes.

#### Router Lifecycle

Router currently supports keep-alive page stack and visibility switching.

Possible direction:

- Add activate/deactivate hooks.
- Add pop/replace lifecycle callbacks.
- Add route history or deep-link serialization later.
- Add page transition animation after animation primitives exist.

#### Animation And Impact Effects

The long-term identity of v3 includes game-like interaction.

Possible direction:

- Add a small tween/animation primitive that can animate `NanNode2D` transform and control properties.
- Let Button click spawn ripple/impact nodes or events.
- Keep animation independent of Button so other components can reuse it.

#### Authoring DSL

Do this after low-level widgets are stable.

The low-level widget API remains valid. A DSL should sit above it, not replace it.
Do not block framework progress on a final authoring syntax.

## Verification

For normal framework changes run:

```sh
meson test -C buildDir
meson compile -C buildDir nandina_example
```

Before committing, inspect status and exclude unrelated generated files such as `firebase-debug.log`.
