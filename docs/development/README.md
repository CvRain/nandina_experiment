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
- Inherit only properties whose semantics are naturally inherited, primarily typography, text color, direction, locale, and opacity. Background, border, padding, layout, shadow, and component variants remain local unless explicitly forced to inherit.
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
| `render` | `IRenderDevice`, `DrawContext`, `ClipStack`, raylib backend factory. |
| `reactive` | `Graph`, `Signal`, `Computed`, `Effect`, `EffectScope`, `ReactiveScope`, batching. |
| `resource` | Stable UUID/key identities, immutable handles, bounded streams, resource URIs/platform locations, prioritized manager, builtin/memory/directory/SQLite backends. |
| `text` | FreeType/HarfBuzz/FriBidi/utf8proc layout, fallback families, atlases, resource font loading, pipeline cache. |
| `theme` | `NanTheme`, palette/tokens, button style resolver. |
| `widget` | Text, Label, Button, EditableText, TextField, ScrollView, and low-level layout controls. |
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
| 5. Widget tree | Shared-owned scene tree, weak observations, enter/exit/ready lifecycle. | Keyed reconciliation and declarative region ownership. |
| 6. Layout | Bottom-up measure/top-down layout, typed invalidation, root correctness boundary, and bounded post-layout relayout. | Screen-canvas layout roots, diagnostics, Grid/Anchor, richer intrinsic contracts. |
| 7. Paint/composition | Tree draw traversal, sibling z-order, clip stack, typed paint dirtiness, replaceable render device. | World/screen CanvasLayer boundaries, dirty-only paint, damage tracking, retained caches, animation phases. |
| 8. Text | FreeType/HarfBuzz/FriBidi/utf8proc, fallback faces, editing geometry, CJK package. | Native IME, UAX #14, emoji/color glyphs, rich text, per-widget family request. |
| 9. Style | `NanTheme`, tokens/palette, primitive and Button variants. | `NanStyle`, style context/cascade, ThemeManager, structured style files. |
| 10. State binding | Signal/Computed/Effect/Scope and limited Label binding. | General properties, automatic bindings, `If`, keyed `ForEach`, no manual refresh. |
| 11. Async | No complete application-facing model yet. | UI dispatcher, background tasks, cancellation, coroutine adapters, stale-result policy. |
| 12. Accessibility/delivery | R1-R10 resource delivery, install/portable layouts. | Semantic tree, keyboard navigation contract, platform accessibility and app packaging. |
| Supporting 2D simulation | Optional Box2D 3.x bridge, fixed physics phase, shape/contact events, and canvas/world isolation. | Interpolation polish, richer queries/shapes, joints, and debug draw. |

The current Todo page is intentionally the pressure test for the next abstraction layer. It still manually rebuilds list children, wires effects, marks layout dirty, and coordinates post-layout scrolling. Those operations prove the underlying runtime but are not the desired application-authoring surface.

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
- Layout results expose source-byte/grapheme caret stops, line-local visual positions, point-to-source lookup, and logical upstream/downstream affinity. `EditableText` consumes this geometry for drawing, visual movement, pointer placement, selection, and grapheme-safe editing.
- System font discovery and dynamically packaged CJK fallback profiles remain resource-system work.

`TextOverflow::clip` preserves complete source and glyph geometry, constrains the reported layout size, and applies a real render clip around text drawing. This keeps glyph bearings, mark offsets, and shaped overhang inside the text box without discarding geometry needed by future editing.

## App Authoring State

The current example is a Todo page using:

- `NanApplication` with app-level `NanTheme`.
- `NanWindow` and `NanRouter`.
- `TodoPage : NanPageT<NoParams>` with page-scoped reactive state.
- semantic `Label`, `Button`, and `TextField` controls sharing one resource-backed text pipeline.
- interactive add/complete/remove actions, a dynamic ScrollView list, and resize-sensitive Flex layout.
- automatic SQLite/builtin resource discovery and window-owned default font pipeline inheritance.
- explicit low-level layout composition for the application UI.

The Todo application configures only `org.nandina.todo`. Meson builds its SQLite package beside the executable, `NanApplication` discovers and mounts it, and `NanWindow` supplies the inherited default text pipeline. The page and widgets contain no backend paths, resource-service assembly, or per-control pipeline wiring.

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

The text, clipping, editing, layout, interactive example, and R1-R10 resource-delivery line are complete. Application authoring foundations A1a/A1b, the A2 property core, the minimal A3 canvas/physics boundary, and A4 declarative regions are implemented. The active main line is A5 UI dispatch and async scope, followed by style/theme context, accessibility, and a thin DSL over the same imperative widgets. Canvas/physics work is supporting infrastructure, not a second product-wide game-engine roadmap.

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

The example declares one always-stale Meson custom target around `nanres_build_helper.py`. Its only source input is `resources.toml`; the helper discovers all resource files, maintains the lock, and uses the package fingerprint to make unchanged builds cheap. This avoids duplicating package ID, lock path, and every resource filename in `meson.build`. The complete package is produced beside the executable at `buildDir/example/resources`. `nanres_install.py` reads the package ID from the same policy and copies the database/sidecar tree to `get_option('datadir')/<app-id>`. On Linux, `--prefix=/usr` therefore installs under `/usr/share/<app-id>`, while `--prefix=$HOME/.local` installs under `~/.local/share/<app-id>` without checking whether the installer is root. Tests add a resource after initial packaging and verify that a second build updates both lock and SQLite package without any build-file change.

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

### Simplified Chinese Example Fallback

The Todo example includes Sarasa Fixed SC Regular 1.0.40 under OFL 1.1. Only the 24.9 MB regular face is retained from the verified `SarasaFixedSC-TTF-1.0.40.7z` release archive; the archive and unused weights are not committed. `source.toml` records the upstream URL, version, archive SHA-256, extracted-font SHA-256, and license. The font is exposed as `fonts/fallback/zh-cn`, packaged as an external sidecar, and registered as `families/zh-cn` after `families/default-ui`. It is not marked streaming because current FreeType loading requires an immutable snapshot. The Todo interface and initial data contain mixed Simplified Chinese/Latin text, and tests verify actual `中`/`文` glyph coverage. Configure/build perform no network download; without the project package, optional registration is a no-op and the built-in default remains available.

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
- Enabling `physics2d` also builds `nandina_physics_example`, an interactive LayerStack demo with a world canvas, screen-space HUD, static ramps, dynamic circles, and a sensor zone. It reuses ordinary `NanNode2D` visuals and the same scene lifecycle rather than adding a physics renderer.
- `PhysicsBody2D`/shape definitions wrap opaque Box2D 3.x IDs and bind simulation transforms to ordinary `NanNode2D` visuals by composition, not inheritance from Box2D types.
- First shapes are box/polygon and circle, plus sensor/contact begin/end events and collision category/mask filtering.
- Body/shape creation and destruction requested during stepping/event dispatch are committed at a physics safe point.

To build and run the optional visual physics example:

```bash
git submodule update --init subprojects/box2d
meson setup buildPhysics -Dphysics2d=enabled
meson compile -C buildPhysics nandina_physics_example
./buildPhysics/example/nandina_physics_example
```

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

Only the UI thread may mutate widgets. Add a UI dispatcher, background task execution, cancellation token, and page/widget-owned `AsyncScope`. Completion returns through the event-loop task phase and is discarded if the owner is gone or a newer generation superseded it. Coroutine syntax may wrap this model later; it is not the underlying contract.

### A6. Font Requests And Style Context

Replace the scene-wide fixed default `TextPipeline` assumption with a window font-resolution context backed by `FontFamilyRegistry` and `FontPipelineCache`. Extend text style/request with logical family, weight, and slant. Add imperative controls such as `set_font_family()`, `set_font_weight()`, and `set_font()`; explicit low-level pipelines remain a supported override.

Introduce four-state style values: unset, inherit, initial, and explicit value. Typography, text color, locale/direction, and opacity inherit by default. Background, border, radius, padding, layout, shadow, and component variants do not.

### A7. NanStyle And ThemeManager

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

### A8. Accessibility Semantics

Add a semantics capability/tree with role, label, value, state, actions, focus, and bounds. Semantic widgets provide defaults; composition decides whether to expose, merge, or hide primitive children. Dirty semantics update in the formal tick. Platform AT-SPI/UI Automation/NSAccessibility adapters come after the internal contract is stable.

### A9. Thin Authoring DSL

Only after A1-A8 are usable, add builders/DSL for composition and binding. DSL expressions return or expose concrete widgets and call the same setters/properties/events as imperative code. No DSL-only node, lifecycle, renderer, style, or state path is allowed. Layer and physics authoring helpers must also return the same concrete `CanvasLayer`, `PhysicsWorld2D`, body, and shape objects used imperatively.

Maintain paired tests/pages:

- imperative construction;
- authored construction;
- equivalent concrete widget access and mutation;
- equivalent binding lifetime, keyed reuse, layout, input, style, semantics, and teardown.

### A10. Todo Refactor And Component Extraction

Split Todo into semantic components (`TodoHeader`, `TodoComposer`, `TodoList`, `TodoRow`, `TodoEmptyState`) and first rewrite it using the new imperative Property/ForEach/Style APIs. Then add a second DSL authoring form over the same components. The example becomes the acceptance test that normal application code describes state projection and intent rather than framework refresh mechanics.

### Deferred After Authoring Core

- Router activate/deactivate/replace lifecycle, history, deep links, and transitions.
- Tween/animation primitives and reusable impact/ripple effects.
- Camera2D, offscreen/custom viewports, physics interpolation polish, broader Box2D joints, and advanced spatial queries.
- Sprite/shape convenience nodes, particles, audio, navigation, scene serialization, and ECS remain optional future game-facing work rather than requirements for the application framework.
- Virtualized lists, richer Grid/Anchor layout, and retained render caches.
- Native IME, clipboard/undo, UAX #14, emoji and rich text.
- System font discovery as an explicit application feature.

## Development Workflow

### Developer Experience Roadmap

The application-facing resource workflow is a separate delivery line from the runtime architecture. The target experience is that an application can include Nandina as a Meson subproject, keep a small human-authored resource rule file, and get resource validation, stable locks, packaging, development lookup, and installation from a normal `meson compile`.

#### D1. Meson Subproject Export

Status: initial non-cross export implemented.

Export a stable `nandina_resource_toolchain` Meson dictionary from the Nandina subproject: the `nanres` executable, build helper, install helper, and resource build template. A clean external fixture consumes these values through `subproject('nandina')`, creates a package and lock, and does not copy Nandina's internal `meson.build` files. The current export is validated for native builds. Splitting `nanres` and its dependencies into a build-machine executable for cross compilation remains a D4 requirement and must be completed before claiming cross-build support.

#### D2. Convention-Driven Resources

Status: initial convention mode and build-tree package metadata implemented; runtime metadata consumption and per-resource overrides remain.

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

The resource build helper now writes a generated `resource-location.json` beside `resources.db`. It records the package ID, build-tree package root, and database filename. This file is development metadata only: it is generated, must not be hand-edited or committed, and its absolute build path must never be embedded into a release binary. Runtime metadata consumption is the next D2 increment; existing executable-relative and install-prefix lookup remains authoritative until then.

Do not replace the manifest with Lua. Resource identity and build inputs must remain statically inspectable, deterministic, cacheable, IDE-editable, and safe in cross builds. A future Lua or Python script may be an explicit asset generator whose declared outputs enter the normal scan root; it must not become the resource inventory, identity, or lifecycle engine.

#### D3. Application Template And `nandina` CLI

Status: planned after D2.

`nanres` remains focused on scanning, validation, lock management, and package creation. A separate `nandina` command owns project-level actions such as `new`, `build`, `run`, `doctor`, and high-level resource edits. The first template should provide Hello World, the default `resources/` layout, a minimal manifest, and a Meson subproject declaration. Todo and Physics Canvas templates follow once the exported API is stable.

#### D4. Distribution And CI

Status: planned after D3.

Document recursive submodule checkout, test clean application builds, offline/default builds, native host-tool builds for cross compilation, executable-relative/package-prefix lookup, deterministic lock/package regeneration, and system/user installation trees. Nandina-as-Git-submodule is a supported development mode; a release archive/wrap remains a future distribution option, not a second resource model.

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
meson test -C buildDir
meson compile -C buildDir nandina_example
```

Before committing, inspect status and exclude unrelated generated files such as `firebase-debug.log`.

For resource/font changes, also run the focused targets:

```sh
meson test -C buildDir resource font-resource --print-errorlogs
```

SQLite dependency changes must validate both the normal system-dependency path and an isolated configure using `--force-fallback-for=sqlite3`. Once R4-R7 land, CI should additionally run `nanres validate`, verify deterministic lock/pack regeneration, launch from an executable-relative package layout, and test Linux user-prefix and system-prefix install trees.
