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

- Flex sizing intentionally covers basis/grow/shrink/min-max redistribution, not the complete CSS flexbox specification.
- No `space-around` or baseline alignment strategy yet; `space_between` supports linear children and per-run Wrap/Flow distribution.
- Grid and anchors are deferred; `ScrollView` is the selected low-level viewport and currently omits scrollbar chrome and kinetic scrolling.
- `Padding` does not model full content-box / border-box semantics.
- Layout dirty/cache invalidation is still coarse.
- Default `NanControl::on_layout()` direct-child behavior is transitional.

## Text And Label

The current text stack is a shared capability used by semantic text controls:

- `TextStyle` carries the common color, font size, `TextOverflow`, and `max_lines` inputs.
- `TextPipeline` carries a backend-neutral layout backend and optional glyph renderer.
- `Text` measures and draws from one `TextLayoutResult`, including lines, source ranges, glyph runs, baseline, effective font size, overflow, fallback font slots, and missing-glyph state.
- `Label` is the semantic text control and can bind to `Signal<std::string>` or `Computed<std::string>`.
- `Button` measures and draws through an internal `Text` primitive.
- `EditableText` and `TextField` forward the same pipeline to their value and placeholder text.
- The production path supports FreeType metrics/rasterization, HarfBuzz shaping, FriBidi ordering, utf8proc grapheme segmentation, fallback faces, glyph atlases, and alpha-texture rendering.

Text limitations:

- The deterministic fallback backend still uses estimated codepoint widths; portable shaped output uses the bundled Caskaydia/FreeType/HarfBuzz pipeline.
- `ellipsis` currently uses ASCII `...` rather than a configurable ellipsis glyph.
- Wrapping is grapheme-safe but still lacks a richer UAX #14 line-break opportunity layer.
- Layout results expose source-byte/grapheme caret stops, line-local visual positions, point-to-source lookup, and logical upstream/downstream affinity. `EditableText` does not yet consume this geometry for drawing and commands.
- System font discovery and dynamically packaged CJK fallback profiles remain resource-system work.

`TextOverflow::clip` preserves complete source and glyph geometry, constrains the reported layout size, and applies a real render clip around text drawing. This keeps glyph bearings, mark offsets, and shaped overhang inside the text box without discarding geometry needed by future editing.

## App Authoring State

The current example is a Todo page using:

- `NanApplication` with app-level `NanTheme`.
- `NanWindow` and `NanRouter`.
- `TodoPage : NanPageT<TodoPageParams>` with page-scoped reactive state.
- semantic `Label` and `Button` controls sharing one Caskaydia/HarfBuzz/glyph-atlas `TextPipeline`.
- bundled font resources as the reproducible default instead of hard-coded system font paths.
- explicit low-level layout composition for the application UI.

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

`TextPipeline` now carries the shared layout backend and optional glyph renderer. `Text`, `Label`, `Button`, `EditableText`, and `TextField` expose the same `set_text_pipeline()` / `text_pipeline()` protocol. Button forwards it to its internal text node, EditableText forwards it to its value text, and TextField applies it to both editable value and placeholder text. Existing text-node accessors remain available as low-level escape hatches.

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

Status: the core layout, shaped-rendering, and initial editing-geometry foundation is complete. Deterministic and HarfBuzz backends produce the same backend-neutral result contract, and measurement and drawing consume that result. FreeType, HarfBuzz, FriBidi, utf8proc, fallback font slots, glyph atlases, multiline layout, grapheme-safe overflow, and baseline/line metrics are integrated. Each line now carries visual caret stops at represented shaping-cluster boundaries, preserving duplicate logical offsets at bidi boundaries through upstream/downstream affinity. Source-byte-to-caret and layout-point-to-source queries are backend-neutral. Richer Unicode line-break opportunities and OpenType ligature-internal caret positions remain future work.

The FreeType foundation loads font faces and exposes pixel font metrics, glyph metrics, and grayscale glyph bitmaps. A CPU glyph atlas packs and caches those bitmaps, while `IRenderDevice` and the Raylib backend support alpha texture upload, revision updates, and tinted region drawing. `HarfBuzzTextLayoutBackend` emits glyph IDs, source clusters, advances, offsets, bidi ordering, and fallback font indices, and `GlyphRunRenderer` routes runs through the matching atlases. `Text` consumes the injected renderer end to end, while its default path remains the deterministic backend plus render-device text fallback.

The first bundled-font build path uses the OFL-licensed Caskaydia Cove Regular from a pinned submodule. Meson option `bundled_fonts` copies the TTF and license into `example/res/fonts`, and integration tests load, shape, and rasterize the copied font. Sarasa Gothic SC remains the intended CJK default candidate, but its current regional release packages are large 7z archives; it should enter through a later font profile/download-cache design rather than slowing every initial configure.

The Todo example now creates its Caskaydia FreeType face, HarfBuzz backend, glyph atlas, atlas texture, and renderer after the window render device opens. `NanWindow::on_teardown()` releases page references and font GPU resources before the device closes. When `bundled_fonts` is disabled, the example keeps the deterministic/render-device fallback path. The build-directory font path is still injected directly into the example; a general resource locator is intentionally deferred.

The HarfBuzz backend applies width clipping, ellipsis, and wrapping at monotone grapheme cluster boundaries. Wrapped source ranges are reshaped per line so ligatures and contextual forms do not leak across line boundaries. FriBidi v1.0.16 resolves paragraph embedding levels once, then applies final line visual ordering, trailing whitespace, and mixed LTR/RTL runs to each wrapped line without losing paragraph context. RTL and mixed-bidi wrap/clip/ellipsis preserve logical UTF-8 source ranges while emitting glyphs in visual order. A richer Unicode line-break opportunity layer is still needed for word-aware wrapping.

`TextOverflow::scale` now measures the widest shaped paragraph, derives an effective pixel size, and reshapes the complete text with updated FreeType metrics, baseline, and line height. It does not scale a previously rasterized texture. A minimum one-pixel font size prevents zero-sized FreeType requests; content that still cannot fit at that size reports overflow.

`HarfBuzzTextLayoutBackend` accepts an ordered fallback face list. utf8proc v2.11.3 provides font-independent UAX #29 grapheme ranges; each complete grapheme is assigned to the first face covering its required codepoints and producing a shaped run without glyph zero. Variation sequences require an explicit variation glyph, while ZWJ candidates prefer the face producing the most compact valid shaped sequence. Adjacent ranges assigned to the same face are reshaped together, and each output glyph records its `font_index`. Layout results expose `missing_glyphs` when no face supports a grapheme. `GlyphRunRenderer` binds its atlas list to a specific HarfBuzz backend, validates every atlas face and texture pair, and synchronizes each used atlas once per line. Combined line metrics cover both the maximum font line height and the fallback chain's full ascent-to-descent extent. System font discovery, dynamic CJK packages, and user-configured fallback chains remain outside this low-level contract.

Tasks:

1. Introduce `TextLayoutInput` and `TextLayoutResult`.
2. Include measured size, laid-out lines, visible text ranges, effective font size, baseline information, and overflow result.
3. Replace ad-hoc `laid_out_text_` / `laid_out_font_size_` state with the layout result.
4. Keep the first backend simple and deterministic; exact shaping can come later.
5. Ensure `Button`, `Label`, and future text consumers all measure through this contract.

This is where the previous iterations failed hardest: text overflow must not be patched separately in Button, Label, Card, Field, and TextField.

#### M3. Container Clip / Overflow Contract

Goal: make parent-declared clipping a tree/rendering contract, not a local widget trick.

Status: initial rendering and hit-test contract complete. `NanControl` exposes `ControlOverflow::{visible, clip}`, and draw traversal pushes an intersected child clip through `ClipStack`. `TextOverflow::clip` preserves complete layout geometry and installs an RAII render clip for both fallback and shaped glyph drawing; `ellipsis` remains a separate truncating and reshaping policy. Scene hit testing now prunes descendant branches outside every clipped ancestor, using the same world-space AABB semantics as the current render scissor implementation. Mouse hover and click-derived focus therefore cannot target clipped descendants.

Note: `ControlOverflow` clips child scene nodes. Text-bearing semantic controls that draw composed internal text, such as `Button`, should expose their own text overflow API that forwards to `TextOverflow`.

Tasks:

1. Add explicit overflow policy to controls or a focused container primitive.
2. Support at least `visible` and `clip` first.
3. Use the existing `ClipStack` in draw traversal so children are clipped by parent policy.
4. Connect `TextOverflow::clip` to render clipping rather than string truncation only. Completed: layout retains complete source/glyph data while the constrained result box clips actual pixels.
5. Keep the path open for `ScrollView`, `Card`, `Dialog`, and popover-like containers.
6. Make ancestor `ControlOverflow::clip` intersections constrain descendant hit testing as well as drawing. Completed for direct hit tests, hover, and click-derived focus.

The next main-line task moves into M4 consumption of this geometry: replace `EditableText::caret_x()` estimation, align caret/deletion to grapheme boundaries, and add visual movement and pointer placement incrementally. Exact polygon clipping for rotated controls, stale hover invalidation after geometry changes, and focus-navigation visibility policy remain later refinements rather than blockers for editable text.

#### M4. EditableText / TextField

Goal: build input on top of the text layout contract, not beside it.

Status: the initial single-line editing milestone is complete. `EditableText` owns value, caret affinity, selection, read-only behavior, and IME-ready preedit state while reusing `Text` for layout and drawing. Caret drawing and pointer lookup consume backend-produced visual geometry. Caret clamping, Backspace, and Delete operate on UAX #29 grapheme boundaries; typing and deletion replace active selections. Left/Right traverse visual caret stops for LTR, RTL, and mixed-bidi layouts, Shift extends selection, Home/End select visual edges, and Ctrl+A selects all.

`TextField` composes the surface, placeholder, editable primitive, focus ring, and semantic state. It supports pointer placement and drag selection through SceneTree pointer capture, a clipped horizontal viewport that keeps the active caret visible, line-metric vertical centering, read-only/disabled/invalid states, `on_change`, and `on_submit`. The value uses `TextOverflow::clip` with complete off-screen geometry rather than ellipsis.

The current raylib adapter exposes committed UTF-8 input through `GetCharPressed()` but does not expose portable native composition updates or candidate-window placement. `TextComposition` therefore provides the backend-neutral preedit state contract for future SDL/native adapters; native IME acquisition remains platform integration work rather than widget-state work. Clipboard, undo/redo, word movement, password masking, multiline editing, caret blinking, and accessibility bridges are later features, not blockers for the initial M4 contract.

Tasks:

1. Add an `EditableText` primitive or internal control for caret, selection, IME/text input, and editing commands.
2. Build `TextField` as Surface + EditableText + placeholder Text + FocusRing + editing state machine.
3. Keep `TextField` responsible for value, placeholder, focus, read-only, disabled, invalid, on_change, and on_submit.
4. Keep Field/Form-level label/helper/error/required semantics outside `TextField`.

#### M5. Layout Refinement

Goal: refine the existing layout primitives after text and clip semantics are stable.

Status: initial refinement complete. Flex/Row/Column share one axis-neutral allocator. `LayoutFlexPolicy` is a zero-RTTI capability and `FlexItem` owns explicit basis, grow, shrink, and physical min/max limits; grow and scaled-basis shrink redistribute remaining space after items hit limits. Existing `Expanded` remains compatible through `layout_flex_factor()` and maps to zero basis plus weighted grow.

Wrap/Flow distribute main-axis space independently inside each run, honor `space_between` for run alignment, support stretched runs, and allow per-child cross-axis alignment overrides. Explicit gaps remain the minimum spacing used for intrinsic measurement and wrapping decisions.

`ScrollView` is the selected low-level viewport. It measures one child with an unbounded scroll axis, clamps programmatic and wheel-driven offsets, translates content locally, and relies on `ControlOverflow::clip` for rendering and hit testing. Wheel events hit-test their current screen position and bubble, allowing nested scroll views to hand off input at their limits. Grid and Anchor are explicitly deferred until real controls require track/span or edge-constraint contracts.

Tasks:

1. Add flex basis/shrink/min-max policy where needed. Completed through `LayoutFlexPolicy` and `FlexItem`.
2. Add main-axis spacing strategies such as `space_between` only when a real page/control needs them. Completed for linear layouts and Wrap runs.
3. Add run stretch and per-child alignment to `Wrap` / `Flow`. Completed.
4. Decide whether Grid, Anchor, or Scroll viewport enters the low-level widget set next. Completed: `ScrollView` entered; Grid and Anchor are deferred.
5. Keep Yoga or any third-party solver behind a backend boundary; it should not define the public widget API.

#### M6. Todo Workflow Validation

Goal: validate M4 editing and M5 layout under a real mouse, keyboard, reactive-update, scrolling, and window-resize workflow.

Status: interactive validation page landed in the Todo example. The page owns a scoped `Signal<std::vector<TodoItem>>`, keeps its `TextField` and `ScrollView` stable, and rebuilds only the rendered task-list snapshot after reactive changes. Enter or the Add button submits non-empty tasks, completion and removal callbacks address stable item IDs, and the input remains focused across submission. Empty submissions expose the field's invalid state.

The list rebuild is staged by a page-scoped effect and applied from `on_process()` rather than synchronously inside button callbacks. This avoids destroying a pressed control while its callback is still executing. New-item scrolling is applied one frame after replacement so `ScrollView` has completed layout and exposes the new maximum offset.

The responsive page uses `Expanded`, `FlexItem` grow/shrink/min-width policy, clipped labels, and a vertical `ScrollView`. Window resize continues through the normal tight root measure/layout pass. All text-bearing controls, including dynamically created task rows and buttons, reuse the window-owned bundled-font `TextPipeline`.

M6 is an application validation milestone, not a new widget abstraction. Its next purpose is collecting real interaction feedback on selection, focus, narrow-window behavior, nested clipping, and dynamic scroll updates before starting native input integration or advanced layout work.

#### Resource Foundation

Status: initial storage-neutral resource contract landed. `ResourceId` is a stable binary UUID, `ResourceKey` is a validated human-readable logical name, and immutable `ResourceHandle` snapshots own their bytes independently of backend lifetime. `ResourceManager` mounts ordered backends with deterministic priority overrides and stops on backend errors rather than silently exposing a lower layer.

`MemoryBackend` supports registration, replacement, aliases, and test/runtime overrides. `DirectoryBackend` uses an explicit manifest so IDs remain stable across file moves and returns owned file snapshots. `SQLiteBackend` is a read-only runtime package backend supporting canonical key/UUID lookup, aliases, embedded BLOBs, and relative external files with declared-size validation. SQLite details remain private to its implementation; package creation and migrations are intentionally separate tooling concerns.

SQLite uses Meson's dependency fallback model. A compatible system SQLite is preferred, while a pinned WrapDB amalgamation is downloaded and checksum-verified when the host has no development package, including normal Windows source/subproject builds. This keeps `<sqlite3.h>` private to `sqlite_backend.cpp` without making SQLite installation a framework consumer prerequisite. Fully offline builds must pre-populate Meson's package cache or vendor the wrap source archive alongside the wrap metadata.

The schema is identified by SQLite `application_id` plus `user_version`. Components must consume logical resource or font-family IDs rather than paths, UUID literals, SQLite rows, or backend handles.

Resource-backed font loading is now available. `FreeTypeFontFace` retains an immutable `ResourceHandle` and creates an `FT_New_Memory_Face`, so shaping and rasterization remain valid after manager unmount, backend destruction, or source-file removal. `FontLoader` resolves logical keys through `ResourceManager` and weakly caches faces by `(ResourceId, face_index)`; aliases deduplicate naturally, while same-ID hot replacement requires explicit invalidation for subsequent loads.

`FontFamilyRegistry` provides application-defined family IDs, aliases, weight/slant matching, family-specific fallback order, global default fallback, cycle detection, and ordered resolution into shared faces for `HarfBuzzTextLayoutBackend`. ResourceManager and FontLoader remain CPU-only.

`FontPipeline` is the render-device-scoped owner for the HarfBuzz backend, one CPU atlas and device texture per resolved face, ordered renderer bindings, and the non-owning `TextPipeline` view consumed by widgets. `FontPipelineCache` weakly reuses that complete owner by family request and atlas configuration; clearing widgets and pipeline handles before device teardown preserves the existing GPU lifetime contract.

The Todo example now mounts its bundled resource directory, resolves `fonts/default` through `families/ui`, and obtains its shared pipeline from `FontPipelineCache`. The former compile-time absolute font-face construction and manual backend/atlas/texture/renderer ownership have been removed. Packaged CJK fallback profiles, SQLite package generation, and color emoji rendering remain the next milestones.

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
