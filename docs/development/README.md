# Nandina v3 Development Notes

This document is the tracked development entry point for the current C++ v3 line.
The older `dev-docs-*` directories are local reference material and are ignored by git.
Use this file as the concise current-state guide before changing runtime, widget, or layout code.

## Current Direction

Nandina v3 is a C++26 + raylib UI runtime built around a Godot-style 2D scene tree.
The lower layers behave like a small game engine: nodes, transforms, lifecycle, input routing, hit testing, draw traversal, and replaceable render devices.
The upper layers rebuild a desktop/frontend authoring surface: reactive state, semantic widgets, page routing, app-level theme, and explicit layout controls.

The current authority is the code under `nandina/` plus tests under `tests/`. Older v1/v2 material is useful for semantics and naming, but it is not the active contract.

The active release-closure authority is [`1.0_ACCEPTANCE.md`](1.0_ACCEPTANCE.md). It defines the
Linux 1.0 support profile, acceptance gates, and ordered closure units. Once the candidate is
accepted, [`1.0_PROMOTION_PLAN.md`](1.0_PROMOTION_PLAN.md) governs the history-preserving migration
to the official `/workspace/Cpp/NandinaUI` repository, the experiment-branch archive, and the 2.0
experiment/production split.

[`NANDINA_CLI_DISTRIBUTION.md`](NANDINA_CLI_DISTRIBUTION.md) defines the independent NandinaCLI
repository, layered source/mirror configuration, locked SDK resolution, verified cache, and SDK
artifact contract introduced by C2.1.

The 1.x desktop backend uses raylib for rendering, windowing, and input; raylib currently supplies its native desktop window through GLFW. SDL is intentionally not built or linked because no Nandina source consumes it and carrying a second platform stack increases clean-build cost. A Vulkan renderer plus SDL window backend may be evaluated after 2.0 if explicit graphics APIs, multi-window behavior, or platform requirements justify it. Any future backend must remain behind the existing window and render-device boundaries rather than becoming a dormant 1.x dependency.

## Current Development Approach

This section records the operative decisions behind the current 1.x line so later rounds do not
re-derive them. The detailed contracts live in the sections that follow; this is the "why", not the
"what".

- **One unit per round, docs first.** Each round lands one reviewable logical unit: record the design
  in the phase/roadmap doc, implement minimally, add focused tests, run the full suite, and commit
  with `type(scope): 中文` after user review. See `WORKFLOW.md` §1–§3 for the exact gates.
- **File-path loading first, nanres later.** Images and fonts load from filesystem paths
  (`IRenderDevice::load_texture_from_file`, `FontLoader::load_file`) with files copied beside the
  executable via `meson configure_file(copy)`. `nanres` packaging is deferred for the examples until
  the toolchain is a real consumer need; the full `nanres` pipeline (scan/validate/lock/pack/install,
  D1–D2) remains exported and tested for applications that want it.
- **Third-party libraries are vendored as git submodules, never re-implemented.** JSON uses
  nlohmann/json through a thin `foundation::parse_json` wrapper; TOML uses toml++; shaping uses
  FreeType/HarfBuzz/FriBidi/utf8proc. We do not hand-roll a parser or a shaping stack.
- **Typography is a family problem, not a widget flag.** `FontFamilyRegistry` owns logical
  families/faces/fallbacks; `FontLoader::load_file` imports any path; `register_face` appends
  regular/bold/italic variants resolved by weight then slant; `find_system_font` discovers paths in
  system font directories; widgets request a family per component/instance via `set_font_family`.
- **Animation is host-driven, not per-widget.** `AnimatedProperty<T>` + a single scene-tree
  `AnimationHost` drive tween/spring/keyframes (`motion::` DSL, `Group` combinators) with
  reduced-motion centralized in the host; widgets only own logical targets.
- **Render stays behind `IRenderDevice`.** New drawing (text glyph atlases, images, soft shadows,
  rounded clips) goes through the device abstraction; raylib types never leak into public headers.

## Architectural Constraints

- Prefer zero RTTI wiring in framework code. Use virtual capability hooks such as `as_node2d()`, `as_control()`, `layout_flex_factor()`, or event type tags before `dynamic_cast`-style solutions.
- RTTI is not forbidden absolutely, but any use must be local, safe, and justified by a better tradeoff than adding another framework hook.
- Keep raylib out of public interfaces. Backend-specific types belong in backend implementation files.
- Use `shared_ptr` for node ownership and `weak_ptr` for parent/hover/focus/delete-queue observation.
- Keep low-level widget APIs valid. `Button::create(...)`, `Label::create(...)`, direct construction, and explicit `add_child` composition remain supported.
- Do not make root controls secretly behave like `Column`, `Center`, or another concrete layout. Use explicit controls such as `Center`, `Padding`, `Column`, `Row`, `Flex`, and `Flow`.
- `NanControl` may keep transitional single-child fill behavior, but concrete page layout should be explicit.

## Design Philosophy

Nandina is deliberately layered rather than DSL-first. The imperative widget API is the runtime contract; higher-level authoring syntax must create, compose, bind, and return the same concrete widgets. A DSL must not introduce a second object model, renderer, state engine, or lifecycle. Developers must always be able to keep a widget reference, retrieve a node from the tree/router, call ordinary setters, or drop to primitives for custom behavior.

The intended stack is:

```text
Authoring DSL / builders
Declarative bindings, If, ForEach, keyed reconciliation
Semantic widgets and application style rules
Primitives + tokens
NanControl / NanNode scene tree
Layout, input, text, render device, platform window
```

Core principles:

- Keep the low-level imperative path complete and testable; the DSL is a thin authoring layer over it.
- Expose primitives for framework and advanced application work, semantic widgets for normal application work, and declarative composition for repetitive state/UI synchronization.
- Prefer one source of truth. `resources.toml` is the only human resource inventory; generated lock/package data and Meson wiring derive from it.
- Separate delivery from semantics: the resource layer knows font bytes, the font registry knows logical families/faces/fallbacks, and the style layer decides which family a component requests.
- Use tokens for theme-dependent values and literals for deliberate fixed overrides. Never write resolved theme values back into local component style.
- Inherit only properties whose semantics are naturally inherited, primarily typography, text color, direction, and locale. Background, border, padding, layout, shadow, and component variants remain local unless explicitly forced to inherit. Opacity is not a style property: it is the node-local `NanNode2D::local_opacity`.
- Theme changes recompute token-backed values; literal instance overrides remain unchanged. Child widgets with no local typography override follow the nearest inherited style, while non-inherited properties continue to use their own component defaults.
- Application code should describe state, UI projection, and user intent. Tree mutation, subscription lifetime, keyed reuse, dirty propagation, and post-layout work belong to the framework.
- Only the UI thread mutates widgets. Background work returns through a UI dispatcher and is cancelled with the owning scope.
- Accessibility semantics are a parallel tree capability, not metadata added after widgets and DSL are complete.
- Keep scene composition concepts orthogonal: a page owns navigation lifetime, a canvas layer owns a coordinate/composition boundary, a Control owns layout behavior, a physics world owns simulation, z-index orders siblings inside one canvas, and collision layer/mask filters physics interactions.

The DSL acceptance test is behavioral equivalence: an imperative page and its authored form expose the same concrete widget types, setters, binding lifetime, input/layout/semantics behavior, and teardown rules.

## Implemented Layers

| Layer | Current state |
| --- | --- |
| `foundation` | Geometry, color, color spaces, decomposed `NanTransform2D`, and the backend-neutral logging service. |
| `scene` | `NanNode`, `NanNode2D`, `NanSceneTree`, input events, focus/hover, deferred delete, `NanControl`. |
| `render` | `IRenderDevice`, `DrawContext`, `ClipStack`, raylib backend, analytic SDF UI primitives. |
| `reactive` | `Graph`, `Signal`, `Computed`, `Effect`, `EffectScope`, `ReactiveScope`, batching. |
| `resource` | Stable UUID/key identities, immutable handles, bounded streams, resource URIs/platform locations, prioritized manager, builtin/memory/directory/SQLite backends. |
| `text` | FreeType/HarfBuzz/FriBidi/utf8proc layout, fallback families, atlases, resource font loading, pipeline cache. |
| `theme` | Immutable DesignSystem snapshots, light/dark semantic palettes, tokens, recipes, typed overrides. |
| `widget` | Text/input controls, Button, Checkbox, Slider, Switch, ScrollView, Badge, and low-level layout controls. |
| `app` | `NanApplication`, `NanWindow`, `NanRouter`, `NanPage`, `NanStore`, app theme propagation. |

### Logging Contract

Framework and application code logs through `foundation/nan_logger.hpp` and never includes spdlog directly. `log::initialize(LoggerConfig)` configures the process root, level, and optional rotating file sink; `NanApplication` installs the default process configuration. `log::get("module.name")` returns a lightweight shared named handle, while free `log::info()`-style functions target the root logger. The public header exposes only standard C++ types, disabled levels avoid formatting work, backend failures do not interrupt application control flow, and `log::shutdown()` safely invalidates existing handles.

## Scene Composition And Physics Boundaries

Nandina must support ordinary application pages and lightweight 2D game scenes without forcing either organization onto the other. A page remains one scene tree and one router/reactive lifetime. It may contain multiple canvas subtrees, but canvas layers do not create another node model, lifecycle, renderer, or page stack.

The intended responsibilities are:

| Concept | Responsibility | Must not become |
| --- | --- | --- |
| `NanPage` | Navigation, keep-alive, page state/scope, and ownership of one scene root. | A render layer or physics world. |
| `CanvasLayer` | Independent canvas transform, visibility, layer-to-layer composition order, and input boundary. | A layout container, collision category, or entity store. |
| `NanNode2D` | Local/world transform, visibility, drawing, hit geometry, and ordinary scene lifecycle. | Automatically layout-managed or automatically physical. |
| `NanControl` | Measure/layout, focus, semantics, style, and UI interaction. | The mandatory base for sprites, bullets, particles, or rigid bodies. |
| `PhysicsWorld2D` | Fixed-step simulation, bodies/shapes, spatial queries, and contact/sensor events. | The scene tree, renderer, or application state store. |
| `z_index` | Stable draw and hit-test ordering among items in one canvas. | Cross-canvas order or collision filtering. |
| collision layer/mask | Box2D category/mask filtering for shapes. | Canvas count, draw order, or UI input priority. |

`CanvasLayer` follows the useful part of Godot's model: it is a non-spatial scene node that establishes a canvas boundary. Its child `NanNode2D` objects inherit the layer canvas transform instead of an arbitrary parent spatial transform. A layer may use world space (normally camera transformed), screen space (viewport coordinates, unaffected by the camera), or eventually a custom/offscreen viewport. Cross-layer `order` is separate from each child's `z_index`.

Do not impose a fixed "16 canvas layers per page" contract. Most application pages need one implicit screen-space canvas; a game page normally needs three to five explicit layers. Store layers in a small stable ordered collection and use a broad signed order range. Bit-count limits belong to collision category/mask fields, not scene composition.

Screen-space layers may declare a root `NanControl`; each such root is an explicit viewport layout boundary and receives tight viewport constraints. World-space layers are not walked by page layout. A Control intentionally placed in world space, such as a nameplate or health bar, uses explicit scene position/size or a dedicated world-UI adapter rather than becoming a page layout root.

Input starts at the highest visible layer and proceeds downward. The initial layer policy is `pass`, `block_below`, or `disabled`. Inside a layer, existing reverse draw/hit order and event consumption apply. Pointer coordinates are transformed from screen space through the selected canvas before hit testing. A modal overlay blocks lower HUD/world layers; a sparse HUD passes events through when no Control accepts them. Physics picking remains a separate world-space query after UI input has not consumed the event.

Visibility, processing, and input are separate switches. Hiding a layer removes it from paint and hit testing; pausing its process/physics work requires an explicit process mode. This allows a pause overlay to remain active while the game-world layer stops simulation.

A representative mixed page is:

```text
SpaceBattlePage
└── LayerStack
    ├── CanvasLayer(background, world-space)
    │   └── StarField
    ├── CanvasLayer(world, world-space)
    │   └── GameWorld
    │       ├── PhysicsWorld2D
    │       ├── PlayerShip
    │       ├── Bullets
    │       └── Asteroids
    ├── CanvasLayer(effects, world-space)
    │   └── Explosions
    ├── CanvasLayer(hud, screen-space)
    │   └── HudRoot : NanControl
    │       ├── ScoreLabel
    │       └── HealthBar
    └── CanvasLayer(overlay, screen-space, block_below)
        ├── PauseMenu
        └── GameOverDialog
```

`SpaceBattleState` owns reactive score, lives, and pause state. `GameWorld` mutates that state in response to gameplay/contact events; `HudRoot` binds to it and does not own or parent the player/physics nodes. The physics body is authoritative for a dynamic actor's position/rotation, and the visual `NanNode2D` synchronizes from it after stepping. Layout must not continuously overwrite dynamic-body transforms.

Box2D upstream is `https://github.com/erincatto/box2d.git`. Use the current Box2D 3.x C17 API through a small optional `physics2d` module. Keep MKS units internally, convert through a configured pixels-per-meter scale, step at a fixed frequency with an accumulator, read movement/contact/sensor events after `b2World_Step`, and defer world mutation requested during event delivery. Do not wrap the complete engine initially.

## Framework Capability Map

The application framework is evaluated across twelve connected responsibilities, with lightweight 2D simulation tracked as an optional supporting capability. “Usable” means the current example and tests exercise the main path; it does not imply platform-complete behavior.

| Responsibility | Current contract | Next gaps |
| --- | --- | --- |
| 1. Window/display surface | `NanWindow` creates a raylib-backed visible surface and render device. | Multi-window policy, DPI/display changes, offscreen surfaces. |
| 2. Event loop | A1a formalizes frame phases; A1b batches effects into one post-physics reactive wave. | UI task draining, reconcile/style phases, dirty-only paint. |
| 3. Input | Mouse/keyboard dispatch, hit testing, focus/hover, pointer editing. | Canvas-aware coordinate routing/input blocking, native IME, shortcuts, gestures, drag/drop. |
| 4. Object model | Concrete `NanNode`/`NanControl` objects with virtual capabilities and C++ setters. | Unified property/event surface without replacing ordinary setters. |
| 5. Widget tree | Shared-owned scene tree, weak observations, enter/exit/ready lifecycle, keep-alive page activate/deactivate. | Keyed reconciliation and declarative region ownership. |
| 6. Layout | Bottom-up measure/top-down layout, typed invalidation, root correctness boundary, bounded post-layout relayout, Row/Column/Flex/Wrap/Grid/Padding/Center/Expanded. | Screen-canvas layout roots, diagnostics, Anchor, richer intrinsic contracts. |
| 7. Paint/composition | Tree draw traversal, sibling z-order, clip stack, typed paint dirtiness, replaceable render device. | World/screen CanvasLayer boundaries, dirty-only paint, damage tracking, retained caches, animation phases. |
| 8. Text | FreeType/HarfBuzz/FriBidi/utf8proc, fallback faces, editing geometry, CJK package. | Native IME, UAX #14, emoji/color glyphs, rich text, per-widget family request. |
| 9. Style | `NanTheme`, tokens/palette, `NanStyle`, `ThemeManager`, `StyleDocument` (styles.toml), reference palettes, theme families, appearance preference. | Widget-level style overrides, richer variant/state rules. |
| 10. State binding | Signal/Computed/Effect/Scope, `Property<T>`, one-way bindings, `If`, keyed `ForEach`, free-function authoring factories. | General property binding coverage, automatic two-way bindings. |
| 11. Async | `UiDispatcher`, `BackgroundExecutor`, `CancellationToken`, `AsyncScope`. | Coroutine adapters, stale-result policy. |
| 12. Accessibility/delivery | R1-R10 resource delivery, install/portable layouts. | Semantic tree, keyboard navigation contract, platform accessibility and app packaging. |
| Supporting 2D simulation | Optional Box2D 3.x bridge, fixed physics phase, shape/contact events, and canvas/world isolation. | Interpolation polish, richer queries/shapes, joints, and debug draw. |

The Todo example is the acceptance surface for application authoring. Its keyed list, conditional empty state, bindings, post-layout scrolling, and reactive page visit counter use the framework contracts directly; A10 presents the same extracted components through imperative and DSL-authored pages, and A11 adds keep-alive page activate/deactivate lifecycle hooks.

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
- `FlexItem`: explicit basis/grow/shrink/min-max policy wrapper.
- `Padding`: single-child padding wrapper.
- `Center`: single-child centering wrapper.
- `Wrap`: automatic run-based wrapping layout.
- `Flow`: alias of `Wrap` for semantic flow layout use.
- `Grid`: fixed-column grid layout with row-by-row cell filling, equal-width columns, per-column/row gaps, and per-cell cross alignment.
- `ScrollView`: clipped single-child horizontal/vertical viewport.

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
- Grid layout is available; Anchor layout is deferred. `ScrollView` is the selected low-level viewport and currently omits scrollbar chrome and kinetic scrolling.
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
- Layout results expose source-byte/grapheme caret stops, line-local visual positions, point-to-source lookup, and logical upstream/downstream affinity. `EditableText` consumes this geometry for drawing, visual movement, pointer placement, selection, and grapheme-safe editing.
- System font discovery and dynamically packaged CJK fallback profiles remain resource-system work.

`TextOverflow::clip` preserves complete source and glyph geometry, constrains the reported layout size, and applies a real render clip around text drawing. This keeps glyph bearings, mark offsets, and shaped overhang inside the text box without discarding geometry needed by future editing.

## App Authoring State

The canonical examples are migrating to the single recommended `app::run<MainPage>()` entry. The
same typed page model starts a minimal one-page application and later participates in navigation,
parameters, stores, and page lifetime without changing `main()`. Functional root factories remain
compatibility and testing adapters, not a second application model.

The earlier paired imperative/DSL Todo was a delivery fixture for page parameters, keep-alive navigation, Store sharing, safe routed commands, and authoring equivalence. Those contracts now live in focused router, authoring, lifecycle, and compact-application tests, so the duplicate application and its test-only accessors are no longer shipped under `example/`.

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

The active post-Phase-7 plan is [Phase 8: Render Quality And Theme Evolution](PHASE8_RENDER_THEME_PLAN.md).
It prioritizes SDF/text clarity, generated reference palettes, true state overlays, and page-scope
lifetime closure before broad component expansion.

The text, clipping, editing, layout, interactive example, and R1-R10 resource-delivery line are complete. Application authoring foundations A1a-A13, the A14 developer-experience line, the A19 functional root runner, and the A20-A21 standard input controls are complete through the Settings reference application: scoped components, binding, theme tokens, concise conditional/keyed authoring, source-budget acceptance, boolean and continuous numeric selection, and a page-class-free single-window entry point. Canvas/physics work remains supporting infrastructure, not a second product-wide game-engine roadmap.

### Completed Milestones

| Milestone | Status | Delivered contract |
| --- | --- | --- |
| M1 Text capability | Complete through `65f3c81`. | Shared `TextStyle`/`TextPipeline` across Text, Label, Button, EditableText, and TextField. |
| M2 Text layout | Complete through `65f3c81`. | UTF-8/grapheme layout, FreeType/HarfBuzz, bidi, fallback runs, atlases, overflow, caret geometry. |
| M3 Clipping | Complete through `65f3c81`. | Control subtree render/hit-test clipping and real text pixel clipping. |
| M4 Single-line editing | Complete through `65f3c81`. | Selection, grapheme editing, visual bidi movement, pointer capture, states, submit, IME-ready state. |
| M5 Layout refinement | Complete through `65f3c81`. | Flex basis/grow/shrink/min-max, Wrap distribution/alignment, ScrollView. |
| M6 Todo validation | Complete through `65f3c81`. | Real keyboard/mouse editing, reactive list mutation, dynamic scrolling, resize-sensitive layout. |
| R0 Resource/font foundation | Complete in `65f3c81`. | Resource identities/handles/backends, SQLite runtime, FontLoader/families, render-device pipeline cache. |
| R1 Builtin/default font | Complete in `9b0933d`. | Read-only BuiltinBackend, embedded Caskaydia Cove font/OFL license, `fonts/default`, and `families/default-ui`. |
| R2 Resource URI/locator | Complete in `9b0933d`. | Strict `res`/`builtin`/`user`/`cache`/`file` URIs and deterministic Linux executable/XDG resource locations. |
| R3 Resource streams | Complete in `9b0933d`. | Bounded read/seek streams with stable metadata, independent ownership, snapshot/file implementations, and backend overlay lookup. |
| R4 `nanres` scan/validate | Complete in `9b0933d`. | Deterministic recursive scanner, ordered media detection, exclusions, unsafe-path diagnostics, and functional `init`/`scan`/`validate` CLI. |
| R5 Policy/lock manifest | Complete in `9b0933d`. | toml++ policy parsing, SHA-256 inventory, stable UUID move/change rules, revisions, stale validation, and atomic generated lock updates. |
| R6 SQLite package/sidecars | Complete in `9b0933d`. | Runtime-compatible SQLite packages, alias rows, policy/size-based BLOB selection, UUID-named external sidecars, atomic rebuilds, and fingerprint skips. |
| R7 Meson build/install | Complete in `9b0933d`, simplified after it. | Policy-only automatic scan/validate/package target, build-tree executable-relative output, datadir install helper, and user/system prefix layout tests. |
| R8 Application bootstrap | Complete in `9b0933d`. | Application-owned resource/font services, built-in bootstrap, locator-driven SQLite mounts, process config discovery, and PageContext service access. |
| R9 Window text pipeline | Complete in `9b0933d`. | Render-device-scoped default FontPipelineCache, scene-context inheritance, explicit override preservation, and ordered scene/GPU teardown. |
| R10 Cleanup/verification | Complete in `9b0933d`. | Removed temporary example resource/font setup and verified package, portable, prefix-install, and builtin-fallback modes. |
| A4 Declarative regions | Complete. | Imperative `IfRegion` and keyed `ForEach`, stable child movement, item scopes, and a Todo acceptance migration without whole-list refresh. |

Remaining M1-M6 follow-ups are deferred rather than blockers: UAX #14 line breaking, OpenType ligature-internal carets, native IME acquisition, clipboard/undo, scrollbar chrome, kinetic scrolling, Grid/Anchor, exact transformed polygon clipping, and accessibility bridges.

### Current Resource Runtime

`ResourceId` is a stable binary UUID, `ResourceKey` is a validated logical name, and immutable `ResourceHandle` snapshots own bytes independently of backend lifetime. `ResourceManager` mounts deterministic priority overlays and stops on backend errors rather than silently exposing a lower layer.

`BuiltinBackend` is a read-only process-shared lowest-priority source for framework resources. It embeds the Caskaydia Cove default font and its OFL license in `libnandina`, exposes stable `fonts/default` and license resources, and supports the `families/default-ui` registration contract without filesystem or system-font dependencies. `MemoryBackend` supports runtime/test overrides. `DirectoryBackend` consumes explicit caller-provided entries and returns owned file snapshots. Read-only `SQLiteBackend` supports canonical key/UUID lookup, aliases, BLOBs, relative external files, size checks, and schema identification through `application_id`/`user_version`. SQLite remains a private C API implementation; Meson prefers a compatible system package and provides a pinned checksum-verified static amalgamation fallback.

`FreeTypeFontFace` can retain a resource-backed memory face. `FontLoader` caches by `(ResourceId, face_index)`, `FontFamilyRegistry` resolves aliases/weight/slant/fallback order, and render-device-scoped `FontPipelineCache` owns HarfBuzz, per-face atlases/textures, and renderer bindings. The Todo example exercises this stack entirely through application/window bootstrap and scene inheritance.

### Active Resource Delivery Sequence

#### R1. BuiltinBackend And Default Font

Status: complete in `9b0933d`.

The licensed Caskaydia Cove regular font and OFL text are generated into `libnandina` at build time and exposed by the read-only process-shared `BuiltinBackend`. Stable `fonts/default` and `families/default-ui` contracts provide a portable default family while allowing higher-priority application mounts to override the logical font key. Automatic mounting and widget inheritance remain R8-R9 responsibilities.

#### R2. Resource URI And Platform Locator

Status: complete in `9b0933d`.

Add URI schemes without weakening stable logical keys:

- `res://` for read-only application resources.
- `builtin://` for forced framework resources.
- `user://` for writable application data.
- `cache://` for disposable data.
- `file://` for explicit filesystem access.

`ResourceUri` strictly parses canonical logical keys for `res`, `builtin`, `user`, and `cache`, while `file` requires an explicit absolute POSIX path. `PlatformResourceLocator` validates the application ID and executable path, then deterministically yields executable-relative, XDG user, XDG system, `/usr/local/share`, and `/usr/share` locations with duplicate removal. It also provides XDG user-data and cache write roots. macOS and Windows location providers remain required before those platforms are claimed as supported.

Runtime discovery uses the executable path, application ID, install prefix conventions, and platform locations rather than compiled absolute paths. Linux search order is executable-relative resources, `$XDG_DATA_HOME/<app-id>`, `~/.local/share/<app-id>` when unset, `$XDG_DATA_DIRS/<app-id>`, `/usr/local/share/<app-id>`, `/usr/share/<app-id>`, then builtins. Installation mode follows Meson's prefix; do not branch on whether the installer is root.

#### R3. Streamed Resources

Status: complete in `9b0933d`.

`ResourceStream` exposes stable ID/key/media/storage metadata, declared size, position, seekability, bounded read, and absolute seek. Memory, builtin, and SQLite BLOB streams retain immutable resource snapshots. Directory and SQLite external streams retain independent file handles, verify declared size when opened, stop reads at the declared boundary, and report premature EOF as an I/O error. Open streams remain valid after backend unmount/destruction. Snapshot and stream size limits are separate so explicitly large external resources can stream without weakening normal `find()` safeguards. Content-hash verification begins when R5 manifests provide authoritative hashes.

#### R4. `nanres` Scan And Validation CLI

Status: complete in `9b0933d`.

The resource tool is named `nanres`. Initial commands are:

```sh
nanres init
nanres scan
nanres validate
nanres pack
nanres watch
```

`nanres init` creates a minimal non-overwriting `resources.toml` starter. `nanres scan` emits a stable key-sorted line inventory and `nanres validate` runs the same checks without inventory output. The reusable scanner accepts arbitrary roots, key prefixes, glob excludes, explicit type rules, output paths, and hidden/symlink policy; it does not require fixed `fonts/images/video` directories. Type resolution applies explicit glob rules first, then file signatures, extensions, and finally `application/octet-stream`. Hidden/generated trees are excluded by default. Symlinks, case-normalization collisions, invalid logical keys, unavailable roots, filesystem failures, and output-package recursion produce deterministic diagnostics. `pack` and `watch` reserve their command names but fail explicitly until R5-R6 provide manifests and package output.

#### R5. Config And Generated Lock Manifest

Status: complete in `9b0933d`.

Human policy is separate from generated inventory:

- `resources.toml`: package ID, root directories/key prefixes, excludes, aliases, hidden/symlink policy, and ordered glob media/storage/streaming rules.
- `resources.lock.toml`: tool-owned format/package header plus UUID, canonical key, policy-relative source path, media type, size, SHA-256, storage decision, streaming flag, and revision for each resource.

Developers do not manually enumerate ordinary resources. `resources.toml` is the only human-maintained resource inventory. `nanres scan` preserves UUIDs by existing source and unique content-hash move detection, increments revision only when content changes, and atomically replaces the tool-owned `resources.lock.toml`. The Meson build helper runs validate on every build, automatically scans when the lock is missing or stale, then packages from the current lock. Adding, deleting, moving, or changing a file under a configured root therefore requires no Meson edit and no separate scan command. CI can still run `nanres validate` directly to require a committed current lock. Ambiguous hash moves, duplicate identities, alias/canonical collisions, missing alias targets, package mismatches, and normalized path/key collisions fail instead of silently changing identity. toml++ prefers the compatible system package and retains a wrap fallback; SHA-256 uses OpenSSL EVP in the `nanres` target rather than adding TOML or crypto dependencies to `libnandina`.

#### R6. SQLite Packaging And External Sidecars

Status: complete in `9b0933d`.

`nanres pack` requires a current validated lock and generates `resources.db` plus an `external/` sidecar directory under the configured `package_directory` (default `package`). The database schema, `application_id`, `user_version`, aliases, BLOB rows, and external paths are consumed directly by `SQLiteBackend`. Explicit storage rules win; `auto` embeds non-streaming resources up to `embed_threshold` (default 1 MiB) and keeps streaming, audio/video, and larger files external. Streaming plus explicit embedded storage is rejected. External files use stable UUID filenames rather than source paths, and every source SHA-256 is rechecked before packaging. A package-policy/lock fingerprint skips unchanged complete output; otherwise a complete temporary tree is transactionally built and swapped into place so stale sidecars are removed. Raw R4 command-line scans cannot write release packages.

#### R7. Meson Build And Install Integration

Status: complete in `9b0933d`; Meson authoring simplified in the current change.

The example previously built a SQLite resource package with an always-stale Meson custom target around `nanres_build_helper.py`. During the example restructure, resource packaging was removed from the example: each example is now a plain application without a project package, and `resources.toml`/`resources.lock.toml` plus the bundled CJK font are no longer shipped under `example/`. The `nanres` toolchain itself remains exported through `nandina_resource_toolchain` and continues to be exercised by the `nanres-cli`, `nanres-install`, `nanres-build-workflow`, and `nandina-subproject-fixture` tests.

Fully offline builds use compatible system dependencies or pre-populated Meson `subprojects/packagecache` archives for the pinned wraps. Configure/build never downloads Sarasa Gothic or other optional resource packs implicitly. Generated SQLite fallback source trees, build outputs, resource databases, package fingerprints, and lock-update temporaries remain outside source control; generated `resources.lock.toml` is the intentional exception and is committed as application inventory.

#### R8. NanApplication Resource Bootstrap

Status: complete in `9b0933d`.

`NanApplication` now owns `ResourceManager`, `FontLoader`, and `FontFamilyRegistry` for the process lifetime. It always mounts the process-shared built-in backend at priority -1000 and registers `families/default-ui`. `NanApplicationConfig` accepts an application ID, executable path, environment snapshot, and optional package filename; `for_process()` resolves `/proc/self/exe` and HOME/XDG variables on Linux. Configured startup uses `PlatformResourceLocator` order to mount each existing `<root>/resources.db` at descending priorities with sidecars rooted beside the database. Missing packages are skipped, while malformed discovered packages fail startup rather than exposing a lower overlay silently. `NanApplication`, application-created routers, and `PageContext` expose the same resource manager, font loader, and family registry. Direct low-level `NanRouter` construction remains valid without application services. The Todo example configures only `org.nandina.todo`.

#### R9. NanWindow Default Text Pipeline

Status: complete in `9b0933d`.

After render-device creation and before `on_setup()`, `NanWindow` resolves the application default family and owns the render-device-scoped `FontPipelineCache`, `FontPipeline`, and backend-neutral `TextPipeline`. `NanSceneTree` carries that default pipeline context. Nodes receive it when entering the tree through a zero-RTTI virtual capability; Text/Label, Button, EditableText, and TextField inherit it, including dynamically added subtrees and internal text primitives. An explicit `set_text_pipeline()`, layout backend, or renderer remains authoritative and is never overwritten by context inheritance. Close clears router frames and the scene root, removes the tree context, then releases FontPipeline, cache, and finally the render device, so no page/widget retains raw renderer pointers after GPU text resources are destroyed.

#### R10. Example Cleanup And Install Validation

Status: complete in `9b0933d`.

The Todo example no longer contains `NANDINA_EXAMPLE_RESOURCE_DIR`, manual DirectoryBackend/resource/font services, `TodoPageParams::text_pipeline`, repetitive `set_text_pipeline()`, or manual resource teardown. The obsolete `bundled_fonts` Meson option, copy targets, and copy-specific font test are removed. R4 explicit scanning still covers loose development trees, while the application path uses the validated R7 SQLite package. Automated probes verify the generated package through `SQLiteBackend`, executable-relative portable layout, user/system datadir installation, and built-in font fallback when no optional/project package is present. Sarasa Gothic remains optional and is not downloaded or required for configure, build, startup, or tests.

Note: the later example restructure removed the example's SQLite package and the packaged-application probes (`nanres-meson-package`, `nanres-chinese-font-package`, `r10-layout`, and the packaged-executable case in `application-resource`) together with the bundled CJK font; the `nanres` toolchain remains covered by the CLI, install, and build-workflow tests.

### Simplified Chinese Example Fallback

The example previously bundled Sarasa Fixed SC Regular 1.0.40 under OFL 1.1 as an external `fonts/fallback/zh-cn` package with a committed `source.toml` inventory. During the example restructure the bundled CJK font pack and its `resources.toml`/`resources.lock.toml` were removed from `example/`; examples now rely on the built-in default family. Without a project package, optional CJK registration is a no-op and the built-in default remains available.

### Deferred Resource And Font Work

- Define monochrome emoji fallback, then a separate color glyph/bitmap renderer path.
- Add project hot reload after identity, lock manifest, and pipeline invalidation semantics are stable.
- Add system font discovery only as an explicit developer/application feature; it is not part of default portability.

## Next Development Line: Application Authoring

Resource delivery is complete. The next main line raises the application-facing abstraction without hiding or replacing the imperative widget runtime. Implement in dependency order; do not start by designing attractive DSL syntax.

### A1. Runtime Tick And Dirty Contract

Status: A1a runtime contract and A1b tick-level reactive wave implemented; task draining, style/semantics consumers, local layout boundaries, and dirty-only paint remain. A4 reconciliation runs in the reactive wave before layout.

Formalize one UI tick:

```text
collect platform events
dispatch input
drain UI-thread tasks
flush batched reactive updates
reconcile declarative regions
resolve styles
measure/layout dirty roots
run post-layout actions
paint dirty regions
present
dispose deferred objects
```

Introduce typed dirty flags for style, measure, layout, paint, and semantics. Setters mark the minimum required flags; application code must not call `mark_layout_dirty()` to synchronize normal widget state. Tree mutation during layout/paint is deferred to a safe phase.

Current A1 decisions:

- Scalar/property mutation is synchronous. Input callbacks and later callbacks in the same dispatch can read the new value; reactive work, layout, and paint later in the same tick observe it.
- One reactive flush wave runs each effect at most once. Self-invalidation is retained for the next flush rather than looping in the current wave, and a flush has a hard cascade limit for diagnostics.
- `add_child` and destructive removal requested during process/layout/post-layout/paint are queued until traversal completes. `remove_child`, whose contract returns ownership synchronously, rejects use during those phases; callers use `remove_and_delete` or `queue_delete` when ownership is not needed. Single-child widgets use one deferred `replace_child` transaction so replacing content never exposes an intermediate detached state. Deferred deletes flush at tree-commit and frame-end disposal safe points.
- Post-layout actions are a formal queue. They may dirty layout and receive one additional root layout pass in the same tick; actions queued while draining and further invalidation remain for the next tick instead of creating an unbounded loop. Replacing the scene root discards callbacks owned by the previous frame/root.
- A setter called during paint updates the stored value immediately. Work whose phase has already passed, including layout or already-painted content, is visible in the next tick; paint never restarts midway.
- The window root is the current correctness-preserving layout boundary. Dirty measure/layout flags propagate through the full ancestor chain, including non-Control nodes, to prevent lost invalidation. Local layout roots are deferred until containers can declare stable constraint boundaries; replaying arbitrary subtrees with stale parent constraints is not valid.

The tick implemented by `NanWindow` is:

```text
input
process / on_frame
commit deferred tree mutations
physics
flush one reactive effect wave
layout root
post-layout actions
optional second layout root pass
paint / present
commit paint-time tree mutations for the next tick
```

`NanWindow` opens a deferred-effect scope before input and commits it in the reactive phase after
physics. Signal and Property values remain synchronously readable throughout the tick, while effects
invalidated by input, process, or physics are deduplicated into that phase. A self-invalidating effect
remains queued for the next tick. Graphs used outside a window retain their existing synchronous flush
behavior, and explicit nested `batch()` calls cannot flush ahead of the enclosing tick.

The Todo example now uses the formal post-layout queue for scroll-to-end instead of a hand-written extra-frame flag.

### A2. Property And Binding Core

Status: core implemented for `Property<T>`, read-only observation, disconnectable events, scoped one-way binding, and `Signal<T>::update(fn)`. `Text`/`Label` are the representative migrated path; broader widget property coverage remains.

Add `Property<T>`, read-only observation, and events while preserving ordinary setters. Both paths must converge on the same mutation/dirty logic:

```cpp
label->set_text("ready");
label->text_property().bind(scope, status);
```

Bindings activate/deactivate with the widget/page scope, batch updates within one tick, and cannot outlive captured application state. Add `Signal<T>::update(fn)` or an equivalent transaction API so state mutation does not require read-copy-set boilerplate.

Current A2 decisions:

- `Property<T>` stores one authoritative value. Ordinary setters delegate to it, and property writes call the same apply callback, dirty propagation, and change event path.
- `ReadProperty<T>` exposes value reads and observation without mutation. `Event<Args...>` supports multiple RAII subscriptions and safe disconnect during dispatch.
- One-way bindings are owned by an `EffectScope`; replacing a binding disposes the previous effect, and leaving the tree disposes both the effect and its source-capturing binding description. A detached widget must be bound again when it is mounted again, so it cannot retain a dangling reference after page state is destroyed.
- Binding sources are structural: any source exposing `get()` with a compatible value type works, including `Signal<T>` and `Computed<T>`. There is no parallel widget-specific binding engine.
- Binding callbacks follow the Graph flush policy: synchronous for standalone graphs and one deferred reactive wave per `NanWindow` tick.

### A3. Canvas Layers And Minimal Physics Bridge

Status: A3a canvas core, A3b1 world/body lifecycle, and A3b2 box/circle shapes with touch events implemented; joints, polygon shapes, richer hit data, and debug draw remain pending.

Add `LayerStack` and `CanvasLayer` as imperative scene objects before expanding rendering or physics features. Preserve one page tree and one lifecycle while allowing independent world/screen coordinate spaces:

- Stable cross-layer ordering, visibility, and `pass`/`block_below`/`disabled` input policy.
- Screen-to-canvas coordinate conversion for hit testing and world-space picking.
- Screen-space root-Control layout boundaries; world-space layers never enter page layout automatically.
- Layer-local child `z_index`; no fixed layer-count contract and no reuse of canvas order for physics filtering.
- One default screen canvas keeps existing ordinary pages source-compatible.

Current A3a decisions:

- Existing page roots remain the implicit single screen canvas. Multi-canvas pages opt in by returning a `LayerStack` and add children through `add_layer()`.
- `CanvasLayer` currently derives from `NanNode2D` to preserve the existing `NanSceneTree` root/edge contract and overrides draw traversal as a canvas boundary. Its inherited transform is the same authoritative canvas transform; layer ordering remains explicit through `order`/`set_order`. A future generic `NanNode` root may remove this compatibility inheritance without changing layer behavior.
- Layer order is stable and independent of child insertion order and layer-local `z_index`. Canvas transforms are included in global bounds, hit testing, and screen/canvas conversion.
- Canvas and nested node transforms follow the existing decomposed translate/rotate/scale model. A non-uniformly scaled canvas combined with nested rotation would require shear, which `NanTransform2D` cannot represent exactly and is not currently a supported precision contract.
- Only a screen layer's declared layout root receives viewport constraints. World-layer Controls retain explicit scene geometry.
- Input searches visible layers front-to-back; `block_below` stops lower-layer picking even when the blocking layer has no hit, while `disabled` skips the layer.
- The optional Meson feature is `-Dphysics2d=enabled`. `subprojects/box2d` is a Git submodule pinned by the repository gitlink to Box2D v3.1.1 commit `8c661469c9507d3ad6fbd2fea3f1aa71669c2fe3`. Meson mounts that local CMake project directly, disables its install targets, and never downloads Box2D through a wrap.

Add Box2D as an optional Meson dependency/subproject from `https://github.com/erincatto/box2d.git` and expose only a narrow `physics2d` bridge:

- `PhysicsWorld2D` owns `b2WorldId`, fixed-step accumulation, pixels-per-meter conversion, the formal physics phase, and a stable per-step touch-event snapshot. `PhysicsBody2D` wraps opaque Box2D body IDs, owns box/circle shapes, applies density/material/filter settings, and can bind an existing `NanNode2D` visual; dynamic bodies drive visuals after a step, while static/kinematic bodies read their transforms from scene state before a step.
- Sensor/contact events are pulled from Box2D after each fixed step and exposed as framework-owned shape references. A handler may request body/shape destruction while events are being delivered; mutation is committed after event dispatch and before the next fixed step. Box2D transient event pointers and IDs are never exposed as the application contract.
- Enabling `physics2d` currently ships no example application; the physics bridge is exercised by the `physics2d` unit tests instead.
- `PhysicsBody2D`/shape definitions wrap opaque Box2D 3.x IDs and bind simulation transforms to ordinary `NanNode2D` visuals by composition, not inheritance from Box2D types.
- First shapes are box/polygon and circle, plus sensor/contact begin/end events and collision category/mask filtering.
- Body/shape creation and destruction requested during stepping/event dispatch are committed at a physics safe point.

The optional physics feature is enabled with `-Dphysics2d=enabled`; it is validated by the `physics2d` unit tests rather than a separate example application.

For a fresh checkout, `git clone --recurse-submodules` initializes Box2D together with the other vendored dependencies. Updating Box2D is an explicit repository change: checkout the reviewed upstream commit inside `subprojects/box2d`, then commit the changed gitlink in this repository.

- Dynamic bodies drive node transforms; static/kinematic bodies may be driven explicitly from scene state before a step. No two-way transform feedback loop is allowed.

Integrate a `physics` phase after process/tree commit and before layout/paint. Physics uses a fixed timestep independent of render `dt`; movement/contact events update application state before reactive/layout work. A small headless fixture must prove deterministic stepping, deferred body destruction, sensor/contact delivery, unit conversion, and isolation from HUD layout.

Acceptance scene: a minimal space-battle fixture has a world layer containing a movable body, bullets, and asteroid sensors plus a screen HUD layer bound to score/lives. Camera2D, offscreen viewports, interpolation polish, joints beyond immediate sample needs, editor tooling, particles, audio, navigation, and ECS remain deferred.

### A4. Declarative Regions And Keyed Reconciliation

Status: complete.

Implement low-level imperative objects for `If` and keyed `ForEach` before adding DSL wrappers. `ForEach` owns a key-to-node map, reuses unchanged nodes, moves nodes without recreating them, destroys removed scopes, and preserves focus/edit state. First version need not virtualize.

Todo success criteria: no `clear_children()` refresh, no hand-written synchronization effect, no explicit layout invalidation, and no recreated row for an unchanged key.

`NanNode::insert_child()` and `move_child()` provide stable sibling ordering without lifecycle churn. Layout containers read the concrete scene-child order directly instead of maintaining a parallel item list. `ForEach` validates duplicate keys before mutation, owns one node and `ReactiveScope` per key, detaches removed nodes before disposing their scopes, and reorders retained nodes without disturbing focus. `IfRegion` gives each active branch the same scoped lifetime contract.

The Todo page now binds a page-scoped status `Computed`, projects tasks through a concrete `TodoRow` keyed by ID, uses `IfRegion` for its empty state, and schedules scroll-to-end directly through `post_layout()`. It contains no list synchronization effect, whole-list child replacement, or application-level `mark_layout_dirty()` call.

### A5. UI Dispatcher And Async Scope

Status: complete.

Only the UI thread may mutate widgets. `UiDispatcher` captures that thread, accepts cross-thread posts, and drains a stable queue snapshot in the event-loop task phase before process/reactive/layout work. `BackgroundExecutor` owns a bounded worker pool, while `CancellationToken` provides cooperative cancellation without exposing the executor implementation.

`AsyncScope::run()` starts a latest-wins generation and returns its `std::expected` result through the UI dispatcher. Starting a newer generation cancels the previous token; clearing or destroying the scope invalidates queued completions. Application-created router frames own a page `AsyncScope`, expose it through `PageContext`, and clear it after detaching the page root. Widgets or application services can own additional scopes over the same application dispatcher/executor when they need independent concurrent operation slots. Coroutine syntax may wrap this model later; it is not the underlying contract.

### A6. Font Requests And Style Context

Status: complete.

Replace the scene-wide fixed default `TextPipeline` assumption with a window font-resolution context backed by `FontFamilyRegistry` and `FontPipelineCache`. Extend text style/request with logical family, weight, and slant. Add imperative controls such as `set_font_family()`, `set_font_weight()`, and `set_font()`; explicit low-level pipelines remain a supported override.

Introduce four-state style values: unset, inherit, initial, and explicit value. Typography, text color, and locale/direction inherit by default. Background, border, radius, padding, layout, shadow, and component variants do not. Opacity is not a style property: it is the node-local `NanNode2D::local_opacity`.

`StyleContext` is stored on every scene node and resolves against the nearest parent into a `ResolvedStyleContext`. Changes propagate immediately through attached or detached subtrees; detaching a node clears inherited results, while `initial` cuts a single property back to its framework default. Text primitives consume inherited font requests, font size, and color unless an instance setter or complete `TextStyle` has supplied an explicit override. Opacity threads through `DrawContext` as `effective = parent effective × node local_opacity` (each node multiplies once), keeping the renderer-independent subtree-fade contract without style-level double-counting.

### A7. NanStyle And ThemeManager

Status: complete.

Keep the shadcn-like primitives + tokens + semantic variant model. `NanStyle` maps component type/variant/state to token references and can be subclassed for application-specific rule algorithms. `ThemeManager` owns named light/dark/high-contrast token sets and a revision; switching themes marks style roots dirty.

Resolution order:

```text
framework primitive defaults
current application theme tokens
NanStyle component/variant rules
nearest inherited StyleContext
subtree overrides
component instance overrides
```

Token references re-resolve on theme changes. Literal instance values remain fixed. A child with no typography override follows its nearest parent context; its background/layout continue to use its own component rule. A child may request `initial` to ignore inherited text style or `inherit` to force inheritance.

Structured `styles.toml` is a data authoring form for tokens, named themes, font-family declarations, and ordinary component mappings. It must compile to the same `NanStyle`/ThemeManager objects used by C++ configuration, not create a separate style engine.

`ThemeManager` owns named `NanTheme` values, the active `NanStyle`, and a monotonic revision. Scene trees observe that revision and recursively notify mounted widgets; Button, TextField, Label, Router, and newly built pages therefore see the current theme without rebuilding the scene. Token-backed rules resolve again for every revision, while literal rule values and explicit widget themes remain fixed. `NanStyle` provides ordered, wildcard-capable Button and TextField rules and virtual typed resolvers for application-specific algorithms. TextField state uses composable focused/disabled/invalid flags instead of losing one state when another is active.

`StyleDocument` parses C++-independent theme data and installs it into the same runtime objects. Font families are registered in two phases before fallback/default links are applied, allowing forward references inside one document. Applications may call `NanApplication::load_styles()` before opening their window, or parse/apply a document explicitly:

```toml
active_theme = "dark"

[themes.dark.palette]
primary = [0.31, 0.12, 250.0, 1.0]

[themes.dark.tokens.spacing]
xl = 33.0

[[styles.button]]
treatment = "ghost"
background = "$palette.primary"
padding_x = "$tokens.spacing.xl"
radius = 12.0

[[styles.text_field]]
state = "focused"
border_color = "$palette.primary"

[[fonts.family]]
name = "families/application-ui"
default = true
faces = [{ resource = "fonts/application-ui/medium", weight = 500 }]
```

### A7b. Reference Palettes And Appearance

Status: complete.

Stabilize the theme model before A9 can freeze it into authoring helpers. Theme colors have three distinct layers: complete 50-950 `NanColorScale` values form reusable `NanReferencePalette` authoring inputs; each concrete light or dark `NanTheme` owns a resolved `NanColorScheme`; component variants and interaction states continue to live in `NanStyle`. Runtime widgets therefore depend on semantic names such as `background`, `primary`, `success`, `warning`, `focus_ring`, and `selection`, never on a raw shade number.

`ThemeManager` groups named concrete themes into families. A family supplies light and dark variants, while `ThemePreference` selects system, forced light, or forced dark appearance. Platform adapters report operating-system changes through `set_system_appearance()`; the cross-platform theme core does not poll native APIs. Revisions are published only when the effective concrete theme changes.

`styles.toml` preserves direct semantic color arrays and additionally accepts reference palettes and families:

```toml
active_family = "application"
appearance = "system"

[palettes.brand.primary]
"50" = [0.98, 0.01, 250.0]
"100" = [0.93, 0.02, 250.0]
"200" = [0.87, 0.04, 250.0]
"300" = [0.80, 0.06, 250.0]
"400" = [0.72, 0.09, 250.0]
"500" = [0.64, 0.12, 250.0]
"600" = [0.56, 0.12, 250.0]
"700" = [0.48, 0.11, 250.0]
"800" = [0.39, 0.09, 250.0]
"900" = [0.30, 0.07, 250.0]
"950" = [0.21, 0.05, 250.0]

[themes.application-light.palette]
primary = "$palettes.brand.primary.500"

[themes.application-dark.palette]
primary = "$palettes.brand.primary.300"

[theme_families.application]
light = "application-light"
dark = "application-dark"
```

The current text pipeline intentionally limits typography tokens to supported properties. User text scaling belongs to accessibility/application preferences and will multiply resolved typography rather than becoming a fixed color-theme value. Non-circular corner shapes remain deferred until the renderer can draw them consistently.

### A8. Accessibility Semantics

Status: complete.

`NanNode` owns a stable semantics id and exposes platform-independent properties containing role, label, value, hint, state, actions, focusability, and bounds. Widgets provide defaults: text exposes static text, Button exposes activation/focus, and TextField exposes editable value plus focused/disabled/read-only/invalid state. Application components can replace these defaults with `set_semantics_override()`.

Composition is explicit through `automatic`, `expose`, `merge_descendants`, and `hidden`. Nodes without their own semantics transparently hoist semantic descendants; merging produces one accessible element while retaining descendant labels, state, actions, and bounds; hiding prunes the complete subtree. `NanSceneTree` rebuilds its immutable semantics snapshot only when dirty, after layout in the formal `semantics` frame phase, so adapters always observe current geometry. Semantic actions are routed back to the source node through the same focus and widget behavior used by ordinary input.

The internal contract deliberately has no AT-SPI, UI Automation, or NSAccessibility dependency. Those platform adapters consume `semantics_tree()` and call `perform_semantics_action()` after the snapshot model is stable.

### A9. Thin Authoring DSL

Status: complete.

`widget::authoring::NodeBuilder<T>` is the v1 stable composition helper around `std::shared_ptr<T>`. `make<T>()` constructs a concrete object, while `from()` accepts the result of an existing factory such as `CanvasLayer::create()`, `ForEach::create()`, or `PhysicsWorld2D::create()`. `configure()` invokes ordinary setters, property bindings, and event APIs on `T&`; `children()` and `child()` forward to the existing `add()` and `set_child()` contracts. `expose()` and `build()` return the original concrete shared pointer rather than a wrapper node. Free-function factories (`row()`, `column()`, `label()`, `button()`, `padding()`, etc.) are thin aliases for `make<T>(args...)` documented in A12.

```cpp
std::shared_ptr<widget::Button> save;
auto form = widget::authoring::make<widget::Column>()
    .configure([](widget::Column& value) {
        value.set_gap(8.0F).set_cross_alignment(widget::LayoutAlignment::stretch);
    })
    .children(
        widget::authoring::make<widget::Label>(graph, "Settings"),
        widget::authoring::make<widget::Button>("Save")
            .configure([](widget::Button& value) {
                value.set_treatment(theme::ButtonTreatment::outlined);
            })
            .expose(save)
    )
    .build();
```

There is no DSL-owned node, binding scope, lifecycle callback, renderer, style resolver, or state store. The builder disappears after composition; scene ownership and teardown remain unchanged. The generic `from()` path also keeps canvas and optional physics authoring on their existing concrete APIs instead of introducing parallel helper types.

The A9 acceptance tests pair imperative and authored construction and verify equivalent concrete widget access, mutation, binding lifetime, keyed reuse, layout, input, style, semantics, and teardown. A10 originally added paired Todo forms as an integration fixture; that duplicate application was retired after A19 while the focused equivalence tests remain.

### A10. Todo Refactor And Component Extraction

Status: complete; historical integration fixture retired after A19.

The A10 delivery fixture split Todo into semantic header, composer, list, row, and empty-state components backed by one application-owned Store. Its imperative page composed concrete controls with setters and `add()`, while its DSL page composed the same types through `widget::authoring`; neither page had a separate state, binding, style, reconciliation, or lifecycle path.

The fixture validated strongly typed route parameters, a reactive visit counter, shared Store state, keep-alive restoration, and traversal-safe `request_*` navigation. Dedicated router and lifecycle tests now retain those guarantees without keeping a second Todo application or exposing its internal widgets solely for tests.

### A11. Page Activate/Deactivate Lifecycle

Status: complete.

`NanPage` gains two virtual lifecycle hooks:

- `on_activate(PageContext&)` — called when a keep-alive page becomes the visible top of the router stack. Fires on initial push and again every time a `pop`/`pop_to` restores the page.
- `on_deactivate(PageContext&)` — called when a keep-alive page is hidden by another page pushing on top, or when the page is popped/dropped from the stack.

The router tracks a per-frame `active` flag and constructs a `PageContext` for the frame from its stored scope, async scope, and the router's shared services. Deactivation fires before activation on the same transition (`deactivate(old) → activate(new)`).

The former paired Todo used `on_activate` to drive a reactive visit signal, proving that a restored keep-alive page observes fresh state rather than a static route snapshot. Router lifecycle tests now cover this contract directly.

Tests cover: initial push activation, push-on-top deactivation, pop reactivation, pop_to reactivation, and clear-then-deactivate. All router tests (13 cases, 77 assertions) pass.

### A12. Authoring Factory Functions

Status: complete.

`widget::authoring` gains free-function factories that are semantic aliases for `make<T>(args...)`. They do not introduce a new object model, renderer, or lifecycle — they return the same `NodeBuilder<T>` and produce the same concrete `shared_ptr<T>` as `make<T>()`.

Layout factories:

- `row()`, `column()`, `flex(axis)`
- `padding(insets)`, `center()`, `expanded(flex)`
- `flex_item(policy)`, `scroll_view(axis)`

Control factories:

- `label(graph, text, theme)`, `button(text, theme)`, `text_field(value, placeholder, theme)`

Theme parameters default to `default_theme()`.

Usage before and after:

```cpp
// Before (A9)
auto page = widget::authoring::make<widget::Column>()
    .configure([](widget::Column& c) { c.set_gap(10.0F); })
    .children(
        widget::authoring::make<widget::Label>(graph, "Overview"),
        widget::authoring::make<widget::Button>("Run")
    )
    .build();

// After (A12)
using namespace widget::authoring;
auto page = column()
    .configure([](widget::Column& c) { c.set_gap(10.0F); })
    .children(label(graph, "Overview"), button("Run"))
    .build();
```

The former DSL Todo page used these factories during delivery. Authoring tests continue to cover type identity, layout equivalence with imperative construction, nested tree composition, and `expose`/`configure` on factory-produced builders. The `make<T>()` entry point remains valid and is the underlying mechanism; the factories are thin inline wrappers.

The A9 section previously marked `NodeBuilder<T>` as "temporary" — this label is removed. `NodeBuilder<T>` together with the free-function factories is the v1 stable authoring API.

### A13. Grid Layout

Status: complete.

`widget::Grid` is a grid layout container. Children fill cells row by row, left to right, with a fixed column count. Each column receives an equal fraction of available main-axis space after gaps; row heights are the maximum measured height in that row. Children are measured with loose cross-axis constraints so cell alignment (`start`, `center`, `end`, `stretch`) takes effect.

API:

- `Grid(columns)` / `Grid::create(columns)` — construct with column count (default 2)
- `add(child)`, `set_columns(n)`, `set_column_gap(g)`, `set_row_gap(g)`, `set_gap(cg, rg)`
- `set_cross_alignment(align)` — vertical alignment of children within their row cells

Authoring factory: `grid(columns)` returns `NodeBuilder<Grid>`.

Layout protocol:

- `on_measure`: measures each child with cell-width constraints, computes column widths as per-column maxima, row heights as per-row maxima, returns constrained total size.
- `on_layout`: measures children with loose cross-axis constraints, positions each child within its cell according to `cross_alignment`, assigns final rects.

Tests cover: type identity, row/column ordering with 2 columns, single-column stacking, zero-child safety, and cross-axis alignment (5 new test cases).

The deferred "richer Grid/Anchor layout" item is partially addressed; Anchor layout remains deferred.

### A14. Developer Experience Contract

Status: complete; the target contract, compact reference application, and source budgets are implemented.

The [A14 developer experience contract](A14_DEVELOPER_EXPERIENCE.md) defines the ideal minimum-window, imperative Todo, and DSL Todo forms. It also establishes measurable line budgets and forbids ordinary page code from directly coordinating scene phases, dispatch queues, reactive scopes, per-frame rendering, and theme propagation.

`NanApplication::run_page<PageT>()` creates the ordinary routed window, pushes the typed initial page, and enters the existing application loop. A19 additionally provides functional root runners for single-page applications, so the compact reference no longer declares a page class merely to implement `route_key()` and `build()`. Explicit page and window subclasses remain the advanced path for named routes and custom frame/setup/teardown behavior.

The earlier paired and compact Todo fixtures have been retired after acceptance. Their 23-line bootstrap and 215 total non-blank line result remains recorded in the A14 contract. The current `nandina_settings_example` Settings interface uses the same authoring layer with a 23-line bootstrap and 122 total non-blank lines while expanding the standard control surface.

### A15. Typed List Model And Commands

Status: complete; list models, named commands, explicit build context, safe router commands, and post-layout intents are implemented.

`widget::ListDataModelSource<Source, Item>` is a structural protocol: a source only needs a tracked `get()` returning `const std::vector<Item>&`; it does not inherit a framework model base. `widget::ListView<Item, Key, NodeT>` is a thin application-facing shell over the existing `ForEach` keyed reconciliation runtime and exposes `set_model(source)` without moving row creation, reuse, reorder, or teardown into application pages.

The Todo example now has a `TodoTasks` component backed by `ListView`. Its rows emit typed toggle/remove intents, while the page connects named handlers to `TodoStore`. Model synchronization and business commands are therefore separate, and the same source protocol can later support read-only, paged, remote, and tree-specific adapters without imposing CRUD inheritance.

`widget::BuildContext` is a lightweight, non-owning bundle of the current `Graph`, `ReactiveScope`, and `ThemeManager`. `PageContext::ui()` creates the page context, while `with_scope()` derives an item or conditional-region context without changing the page-wide graph and theme services. Its context-aware factories reuse the stable `NodeBuilder<T>` API and always read the currently active theme when constructing a widget; no global or thread-local authoring state is introduced.

The A15 Todo fixture accepted `BuildContext` instead of separately threading graph and theme arguments. `ListView` row factories derive a context from the row-owned scope, preserving keyed row cleanup and establishing the same construction contract for later ownership work. Routing, stores, and dispatch remain explicit `PageContext` services rather than becoming widget dependencies.

`NanRouter::request_push()`, `request_replace()`, `request_pop()`, and `request_pop_to()` are traversal-safe commands layered over the immediate router API. Commands execute in the next UI task phase and carry a router lifetime guard, so destroying a router invalidates its queued work. `NanNode2D::request_focus()` and `ScrollView::request_scroll_to_end()` similarly express post-layout intent, including before mounting, without exposing `NanSceneTree` scheduling to application components. The Todo example now uses these APIs and no longer includes `scene_tree.hpp`, receives `UiDispatcher`, or overrides `on_ready()` for routine focus.

### A16. Component And Page Ownership

Status: complete; page, component, conditional, and keyed scopes use automatic cleanup.

`BuildContext::make<Component>(args...)` constructs a concrete custom widget with a derived `ReactiveScope`. The scope shares the page graph and theme services, remains alive for exactly as long as the component's `shared_ptr` control block, and clears subscriptions and reactive work before destroying the component. It preserves normal `shared_from_this()` behavior and returns the same stable `NodeBuilder<Component>` used by ordinary authoring factories.

`BuildContext::signal()`, `computed()`, `effect()`, and `connect()` register work in the current page, component, item, or conditional scope. `ReactiveScope::connect()` owns the returned event subscription and disconnects it before effects, computed values, and signals are torn down. The Todo components are now created through `ui.make<T>()`; page-to-list commands use `ui.connect()` and no component stores manual `Subscription` members.

The low-level `BuildContext::scope()` and `with_scope()` APIs remain available for framework regions and advanced integrations. `PageContext::ui()` supplies the router-owned page scope, custom components derive their own scope through `ui.make<T>()`, and conditional/keyed regions derive scopes through `with_scope()`. Lifecycle tests cover component destruction, routed page removal, keyed row removal, and conditional branch replacement so subscriptions are cleared before their captured nodes and state are destroyed.

### A17. Bindable Properties And Theme Tokens

Status: complete for the v1 authoring contract through typed component traits.

`BuildContext::bind(target, setter, source)` connects any tracked source exposing `get()` to an ordinary concrete-widget setter. The current page, component, item, or conditional scope owns the effect, and the binding retains only a weak widget reference. `Signal`, read-only signal views, `Computed`, and compatible properties therefore share one binding path while detached controls are not kept alive by reactive work. `ui.make<widget::Label>(source)` and `ui.make<widget::Button>(source)` are concise forms over this same contract; literal values use the same typed construction path and ordinary setters.

`ui.make<widget::TextField>(signal, placeholder)` adds the v1 two-way string binding: source changes call the normal `TextField::set_value()` path, while committed user edits are published through `TextField::value_changed()` and written back to the signal. The existing callback setters remain available for commands and validation, and no control-owned reactive scope is added.

`Label::set_color_token()` and the `Surface` theme-color fill/border overloads retain semantic `ColorToken` references rather than resolved snapshots. Attached scene trees already receive every `ThemeManager` revision, so these values are re-resolved automatically alongside Button and TextField styles. The Todo components now use context-owned text bindings and semantic colors and no longer override theme callbacks merely to refresh labels, rows, or the page background. The older `Label::bind_text()` entry point remains source-compatible but is no longer the recommended authoring path.

### A18. Conditional And Keyed Collection Authoring

Status: complete for the v1 authoring API and compact-reference acceptance.

`BuildContext::when(source, when_true[, when_false])` constructs the existing `IfRegion<scene::NanControl>`, binds the tracked condition, and passes each branch factory a context derived from that branch's runtime-owned scope. Application code no longer passes a `Graph`, handles a `ReactiveScope`, or sequences `create()` and `bind()`. Branch factories return ordinary `NodeBuilder<T>` or `shared_ptr<T>` values, so conditional authoring continues to materialize concrete controls without a parallel view type.

`BuildContext::for_each(source, key, create[, update])` infers the item, key, and concrete row types, creates the existing typed `ListView<Item, Key, Node>`, binds the structural vector source, and passes row factories their keyed item context. Stable keys still preserve node identity, focus, edit state, and ordering; removed rows still clear their subscriptions and reactive work before destruction. The optional update callback remains separate from creation so retained rows receive changed model values without rebuilding their component.

The A18 Todo fixture spelled its empty state and keyed rows through these two APIs; the compact canonical example retains the same coverage. The low-level `IfRegion::create()` / `bind()` and `ForEach` / `ListView` constructors remain available for framework internals and advanced users who need explicit scope or runtime control.

`NodeBuilder` also forwards the common authoring modifiers used by the compact reference: click and submit handlers, button tone/treatment, layout gap/alignment, label font/color tokens, scroll wheel step, and autofocus intent. Each modifier is constrained by the concrete widget's existing setter and stores no parallel property state.

The compact Todo acceptance fixture deliberately had no workspace facade, event-forwarding component, page-owned widget registry, lifecycle override, or test-only accessor. Its headless test exercised real controls before the fixture was retired. Broader routing, keep-alive, parameter, and concrete-widget equivalence remain covered by focused framework tests.

### A19. Functional Root Application Runner

Status: complete for ordinary single-window applications.

`NanApplication::run(window, factory)` creates the same configured window and router frame as `run_page()`, but adapts a root factory through an internal `NanPageT`. A factory may accept `PageContext&` when it needs the installed Store, router, resources, or other page services, or accept only `BuildContext&` for a self-contained view. It may return either a concrete-node `shared_ptr` or the existing `NodeBuilder<T>`; no new view object or renderer is introduced.

The free `app::run({.id, .window}, factory)` is the minimum process entry point and owns `NanApplication` construction and shutdown. Applications that install a Store or configure resource and theme services first retain an explicit `NanApplication`, then call the member runner. The current Settings example uses the free form; applications with an installed Store use the equivalent member form without reintroducing a page subclass.

The internal functional frame has the fixed route key `root`. Multiple named routes, typed route parameters, keep-alive navigation, and lifecycle activation hooks continue to use explicit `NanPageT` classes; the functional runner removes single-page ceremony rather than weakening the router contract.

### A20. Checkbox And Settings Reference

Status: complete for boolean selection and two-way authoring.

`widget::Checkbox` is a concrete `Pressable` control with a label, boolean checked state, disabled/focus/hover/press visual states, theme-token-backed colors and metrics, inherited text pipeline, keyboard Space/Enter activation, and platform-independent checkbox semantics. User activation updates the stored value and emits `checked_changed`; programmatic `set_checked()` is silent so external state synchronization cannot create callback loops.

`ui.make<widget::Checkbox>(signal, label)` binds any page/component-owned `Signal<bool>` in both directions through `ComponentTraits<Checkbox>`, using the existing weak setter binding and scope-owned event subscription. Literal construction through `authoring::make<Checkbox>()`, `.checked()`, and `.on_change()` remains a thin adapter over the same concrete control.

The canonical `nandina_settings_example` is now a Settings interface rather than another Todo variant. It starts through free `app::run`, owns profile and preference signals in the root scope, and exercises TextField, Checkbox, conditional content, reactive labels, commands, focus, theme tokens, layout, and semantics. Its headless test activates real semantic controls and validates save/reset behavior without test-only component accessors. A20 established a 23-line bootstrap and 122-line application snapshot; the continuing source-budget test keeps the bootstrap below 30 lines, the complete application below 220 lines, and forbids page/window/frame plumbing.

### A21. Slider And Continuous Value Binding

Status: complete for bounded continuous numeric input.

`widget::Slider` provides a labelled accessibility identity, finite minimum/maximum range, positive step quantization, silent programmatic `set_value()`, and user-originated `value_changed Event<float>`. Pointer presses update immediately and capture subsequent dragging outside the control; keyboard arrows, Home/End, disabled and focus states, and semantic `set_value`, `increment`, and `decrement` actions share the same normalized update path.

`ui.make<widget::Slider>(signal, label, minimum, maximum, step)` adds the float counterpart to Checkbox and TextField two-way authoring through `ComponentTraits<Slider>`. Scope-owned subscriptions publish user changes to the signal, weak setter effects reflect source changes back into the concrete control, and silent setters prevent feedback loops. Literal `authoring::make<Slider>()` construction and `.value()`, `.range()`, `.step()`, and `.on_change()` modifiers remain available without a reactive source.

The Settings reference now uses Slider for an interface-scale preference and derives both its local label and overall preference summary from the bound signal. Save/reset semantics and the actual Slider accessibility actions are covered headlessly. The canonical example is now 140 non-blank lines including its 23-line bootstrap, while still containing no page, window, frame, or manual reactive-scope plumbing.

### A22. Extensible Component Authoring

Status: complete for the built-in control traits and public header split.

`BuildContext` remains a lightweight service handle rather than an ever-growing component catalog.
The generic construction path is `ui.make<widget::Button>("Save").on_click(save).build()`.
Each component provides a typed `ComponentTraits<T>` customization in the built-in traits layer.
That customization adapts context, theme, and defaults into `NodeBuilder<T>`;
adding a component does not require editing `BuildContext`. Runtime `std::type_index` registries are
reserved for diagnostics and future plugins, while traits and concepts provide the typed path
without depending on unstable C++ static reflection. Public headers are layered into core,
controls and data boundaries, keeping a future module migration straightforward.

The first slice introduces the incomplete `ComponentTraits<T>` customization point and routes
`BuildContext::make<Label/Button>()` through built-in specializations. String signals retain weak
setter bindings, while context-aware application components continue using the scoped-constructor
fallback. `base_window` exercises `make<Button>(signal)` visually. Checkbox and Switch form the
second slice: both literal construction and `Signal<bool>` two-way binding live in traits, and
Settings exercises the new path for notification, diagnostics, and reduced-motion preferences.

TextField and Slider form the third slice. Their traits own `Signal<std::string>` and `Signal<float>`
two-way synchronization respectively, including slider epsilon feedback suppression. Settings now
constructs every interactive input through `make<T>()`.

The named component factories have now been removed during pre-1.0 iteration:
`BuildContext::label/button/checkbox/switch_control/slider/text_field` and their matching
`authoring::*` component functions no longer exist. Application code uses `ui.make<T>()`; low-level
tests and framework composition use `authoring::make<T>()`. Layout factories and structural
`when()/for_each()` remain because they express layout/data behavior rather than a component catalog.

Public authoring includes now follow the same boundary. `widget/build_context.hpp` exposes core
build services, layout/data authoring, and the scoped custom-component fallback without including
every concrete control. Application code that constructs built-ins includes `widget/controls.hpp`,
which deliberately adds the built-in controls and their `ComponentTraits<T>` specializations.
Framework and low-level control code may instead include only the concrete widget and generic
`widget/authoring.hpp`. A dedicated translation-unit test keeps custom `BuildContext::make<T>()`
usable with the core header alone, preventing the built-in catalog from leaking back into it.

### A23. Page Callback Lifetime Closure

Status: complete for callbacks installed through BuildContext authoring.

Each `ReactiveScope` generation owns a lifetime token. `BuildContext` preserves the page token when
deriving component, conditional, and keyed-item contexts, and builders returned by `ui.make<T>()`
guard `on_click`, `on_submit`, and `on_change` handlers with its weak token. Page pop invalidates the
token before subscriptions, effects, computed values, and signals are destroyed, so an externally
retained page root cannot re-enter callbacks that captured page state. Clearing and reusing a scope
publishes a fresh generation for newly authored controls while old controls remain inert.

This closes the retained-root hazard without making the lightweight BuildContext owning, retaining
an inactive page, or introducing a root/page ownership cycle. Low-level `authoring::make<T>()` stays
unguarded by design because it has no page scope; advanced direct setter/configure code remains
responsible for the lifetime of what it captures.

### A24. Motion Policy Foundation

Status: complete for shared tokens and effective preference resolution; component animation follows
as a separate review unit.

`NanTokens::motion` defines reusable short, medium, and long durations in seconds. `ThemeManager`
owns a `system/full/reduced` application preference plus the current platform reduced-motion value,
and exposes one effective `reduced_motion()` decision. Observers receive a revision only when that
effective decision changes, so attached components can cancel or complete animation without a
parallel settings channel. Button ripple now consumes this policy rather than adding a
Button-specific accessibility flag.

### A25. Button Ripple Feedback

Status: complete.

Button pointer presses start a recipe-driven ripple from the local press position. A shared
`RipplePainter` expands to the farthest container corner with cubic ease-out and fades beneath the
outline, label, and focus ring. Rounded clipping is an explicit render-device primitive; the raylib
backend evaluates the circle/rounded-rectangle intersection in the existing SDF shader, while
recording backends can inspect the semantic draw call without depending on GPU output.

The animation duration comes from `NanMotionTokens`, and attached Buttons observe the effective
application motion preference through `ThemeManager`. Reduced motion and disabled state cancel an
active ripple and suppress later pointer ripples. The Settings example's motion checkbox controls
that real policy rather than maintaining a visual-only preference.

### Deferred After Authoring Core

- Router history, deep links, replace semantics, and page transitions.
- General tween/animation primitives and adoption of shared impact feedback by later controls.
- Camera2D, offscreen/custom viewports, physics interpolation polish, broader Box2D joints, and advanced spatial queries.
- Sprite/shape convenience nodes, particles, audio, navigation, scene serialization, and ECS remain optional future game-facing work rather than requirements for the application framework.
- Virtualized lists, richer Anchor layout, and retained render caches.
- Native IME, clipboard/undo, UAX #14, emoji and rich text.
- System font discovery as an explicit application feature.

## Iteration Workflow

Nandina develops in one-reviewable-unit increments. The complete iteration contract —
docs-first discipline, verification gates, commit discipline, the frozen component
template, and lifetime rules — lives in [`WORKFLOW.md`](WORKFLOW.md). Every round:

1. Close any doc/status drift before writing code.
2. Implement one logical unit; components follow the frozen template.
3. Pass `meson compile -C buildDir`, `meson test -C buildDir --print-errorlogs`, and
   `git diff --check`.
4. Have the user review the running effect and the diff.
5. Commit with `type(scope): 中文描述` and a body covering motivation / core changes /
   verification, splitting theme/widget/render/app/example changes when dependency
   order allows.
6. Sync the phase doc, capability map, and this file in the same change.

## Development Workflow

### Developer Experience Roadmap

The application-facing resource workflow is a separate delivery line from the runtime architecture. The target experience is that an application can include Nandina as a Meson subproject, keep a small human-authored resource rule file, and get resource validation, stable locks, packaging, development lookup, and installation from a normal `meson compile`.

#### D1. Meson Subproject Export

Status: initial non-cross export implemented.

Export a stable `nandina_resource_toolchain` Meson dictionary from the Nandina subproject: the `nanres` executable, build helper, install helper, and resource build template. A clean external fixture consumes these values through `subproject('nandina')`, creates a package and lock, and does not copy Nandina's internal `meson.build` files. The current export is validated for native builds. Splitting `nanres` and its dependencies into a build-machine executable for cross compilation remains a D4 requirement and must be completed before claiming cross-build support.

#### D2. Convention-Driven Resources

Status: complete — convention mode, build-tree package metadata, runtime metadata consumption, and
per-resource overrides are implemented.

The normal application layout is:

```text
resources/
├── resources.toml
└── assets/
    ├── images/
    ├── fonts/
    └── data/
```

`resources.toml` remains the only manually maintained inventory, but it becomes a small rule file rather than a list of every file. The minimum formal manifest is now `package = "org.example.app"`; without explicit `[[roots]]`, `source = "assets"` is used, and if `source` is omitted the same `assets` convention applies. Existing `package_id` and explicit roots remain compatible for projects that need aliases, bundled framework assets, or special storage rules. `resources.lock.toml` remains generated and committed: it records the solved UUID, normalized source path, hash, type, and storage decisions.

`nanres init` creates this minimal manifest and an empty `assets/` directory. A new project can therefore put files under `assets/` and run `nanres scan` without writing a root table. Meson consumers use the same convention through the D1 exported resource toolchain.

The intended flow is:

```text
put files under resources/assets/
→ meson compile
→ nanres validates/scans/updates lock/packages
→ the application resolves the build-tree package
```

No source-tree copying or manual package synchronization is required. A build metadata file may point development runtime lookup at the package in the build tree; release lookup remains executable-relative and install-prefix based.

The resource build helper now writes a generated `resource-location.json` beside `resources.db`. It records the package ID, build-tree package root, and database filename. This file is development metadata only: it is generated, must not be hand-edited or committed, and its absolute build path must never be embedded into a release binary. Runtime metadata consumption is now wired in: `NanApplication` reads a `resource-location.json` at each scanned resource root (nlohmann/json is vendored as a git submodule and wrapped by `foundation/json.hpp`'s `parse_json`) and mounts the pointed build-tree package at that root's priority, falling back to the direct `<root>/resources.db` when no metadata file is present (release/install).

Per-resource overrides are expressed as explicit `[[resources]]` entries keyed by logical key; they win over glob `[[rules]]` and over signature/extension media-type detection:

```toml
embed_threshold = 1048576

[[resources]]
key = "assets/logo.png"
media_type = "image/png"   # 覆盖自动检测
storage = "embedded"       # auto | embedded | external
streaming = false
```

`storage` and `streaming` participate in the same lock/package decisions as glob rules; `media_type` overrides the scanned type in `resources.lock.toml`. A `[[resources]]` key must be a canonical `ResourceKey`, must not repeat, and only needs the fields the project wants to override.

Do not replace the manifest with Lua. Resource identity and build inputs must remain statically inspectable, deterministic, cacheable, IDE-editable, and safe in cross builds. A future Lua or Python script may be an explicit asset generator whose declared outputs enter the normal scan root; it must not become the resource inventory, identity, or lifecycle engine.

#### D3. Application Template And `nandina` CLI

Status: the C2 local-source workflow is complete (`new` + `build` + `run` + `doctor` +
`--version`/`--help`). C2.1 now owns the sibling `NandinaCLI` repository. Commits `3716bbd`,
`a6722f9`, `742c02d`, `3f33ba7`, and `71f27f4` implement the independent C++20 build,
registry/archive provider, layered and CLI-editable sources/mirrors, size/SHA-256 cache,
provider-neutral lock generation, migrated `build`/`run`/`doctor`, Git forks pinned to exact
commits, explicit path development mode, Ed25519-signed indexes, and platform-selected
process/config/cache/path backends. C2.1's code contract is complete under
`NANDINA_CLI_DISTRIBUTION.md`; native Windows/macOS validation remains a D4 support gate and the
built-in official release key is provisioned during C6.

`nanres` remains focused on scanning, validation, lock management, and package creation. A separate `nandina` command owns project-level actions such as `new`, `build`, `run`, `doctor`, and high-level resource edits. The first template provides Hello World, the default `resources/` layout, a minimal manifest, and a Meson subproject declaration. Todo and Physics Canvas templates follow once the exported API is stable.

`nandina new <path> [--package <package-id>] [--nandina-source <path>]` scaffolds the application
files: `meson.build`
(`subproject('nandina')` + the D1 `nandina_resource_toolchain` custom target + an executable),
`src/main.cpp` (a minimal `app::run<MainPage>` program), `resources/resources.toml`
(`package = ...`), `resources/assets/.gitkeep`, `.nandina/target`, and `.gitignore`. The binary lives
in `tools/` so its build output
(`buildDir/tools/nandina`) does not collide with the `nandina/` library directory. By default the
generated project links the Nandina source configured into the CLI; `--nandina-source` selects an
explicit checkout. Generation happens in a sibling staging directory and is published only after
all files plus the source link exist.

`nandina build [path] [--build-dir <path>]` configures a missing build directory and then compiles
it; later calls reuse the Meson configuration. `nandina run [path] [--build-dir <path>]
[--no-build] [-- <args...>]` uses that same directory, resolves the executable through
`.nandina/target`, forwards application arguments without a shell, and returns the application's
exit code. `nandina doctor [path]` checks Meson >= 1.3, Ninja >= 1.10, CMake, the selected C++
compiler, Python 3, pkg-config, Git, OpenSSL >= 3.0, and a real C++26 compile/link probe. With a
project path it also validates the resource manifest, CLI target metadata, Nandina source link or
wrap, and recursively checked-out Nandina dependencies.

The release compiler baseline is GCC 16+ or Clang 21+ with a matching standard library that passes
the C++26 probe (`std::expected`, `std::move_only_function`, and `std::println`). `nandina-cli`
executes `new -> build -> doctor -> run --no-build`, verifies incremental build-directory reuse,
the generated resource package/lock/metadata, argument forwarding, and child exit-code propagation,
and covers default/explicit source plus invalid input paths. A pinned official wrap/submodule
belongs to C6/C8.

#### D4. Distribution And CI

Status: planned after D3.

Document recursive submodule checkout, test clean application builds, offline/default builds, native host-tool builds for cross compilation, executable-relative/package-prefix lookup, deterministic lock/package regeneration, and system/user installation trees. Nandina-as-Git-submodule is a supported development mode; a release archive/wrap remains a future distribution option, not a second resource model.

For 1.0, D4 owns the CI/sanitizer gates, release metadata, SDK bundle/index publishing, and
source/install distribution verification. The source-resolution contract itself is frozen in C2.1
so later platform and release work consumes one stable model. The accepted tree is released only
after the official-repository promotion and re-verification defined by `1.0_PROMOTION_PLAN.md`.

Resource configuration decisions:

- Keep TOML as deterministic declaration data; improve its schema and defaults before adding another language.
- Human intent belongs in `resources.toml`; solved inventory belongs in `resources.lock.toml`; runtime delivery belongs in SQLite packages.
- Default conventions are overridable rules, not hard-coded runtime behavior.
- `nandina` and `nanres` have separate responsibilities: project workflow versus resource solving/delivery.

For each roadmap item:

1. Write or update the low-level contract and invariants before authoring syntax.
2. Implement the imperative API and focused tests first.
3. Integrate lifecycle, dirty flags, teardown, and thread ownership explicitly.
4. Exercise the API in Todo or a minimal fixture without DSL.
5. Add declarative/DSL wrappers only when they are mechanical adapters over the tested imperative path.
6. Add equivalence tests so convenience APIs cannot diverge into a second runtime.
7. Update this document’s capability map, limitations, and active milestone in the same change.

Review questions for every abstraction:

- What concrete node/widget exists at runtime?
- Who owns it and its subscriptions/tasks?
- Which event-loop phase may mutate it?
- Which coordinate space/canvas owns it, and how are screen points transformed for input?
- Which dirty flags does each property change?
- If physics participates, is scene state or the physics body authoritative for each transform, and when is synchronization allowed?
- How does explicit imperative mutation interact with bindings and style cascade?
- What is inherited, token-backed, literal, or initial?
- What semantics node and actions does it expose?
- Can an advanced developer retrieve and modify the concrete object directly?

## Verification

For normal framework changes run:

```sh
meson compile -C buildDir
meson test -C buildDir --print-errorlogs
git diff --check
meson compile -C buildDir nandina_settings_example
```

Before committing, inspect status and exclude unrelated generated files such as
`firebase-debug.log`. The full iteration contract (docs-first discipline, commit
discipline, component template) is in [`WORKFLOW.md`](WORKFLOW.md).

For resource/font changes, also run the focused targets:

```sh
meson test -C buildDir resource font-resource --print-errorlogs
```

SQLite dependency changes must validate both the normal system-dependency path and an isolated configure using `--force-fallback-for=sqlite3`. Once R4-R7 land, CI should additionally run `nanres validate`, verify deterministic lock/pack regeneration, launch from an executable-relative package layout, and test Linux user-prefix and system-prefix install trees.
