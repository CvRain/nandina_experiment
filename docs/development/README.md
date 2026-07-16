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

The DSL acceptance test is behavioral equivalence: an imperative page and its authored form expose the same concrete widget types, setters, binding lifetime, input/layout/semantics behavior, and teardown rules.

## Implemented Layers

| Layer | Current state |
| --- | --- |
| `foundation` | Geometry, color, color spaces, and decomposed `NanTransform2D`. |
| `scene` | `NanNode`, `NanNode2D`, `NanSceneTree`, input events, focus/hover, deferred delete, `NanControl`. |
| `render` | `IRenderDevice`, `DrawContext`, `ClipStack`, raylib backend factory. |
| `reactive` | `Graph`, `Signal`, `Computed`, `Effect`, `EffectScope`, `ReactiveScope`, batching. |
| `resource` | Stable UUID/key identities, immutable handles, bounded streams, resource URIs/platform locations, prioritized manager, builtin/memory/directory/SQLite backends. |
| `text` | FreeType/HarfBuzz/FriBidi/utf8proc layout, fallback families, atlases, resource font loading, pipeline cache. |
| `theme` | `NanTheme`, palette/tokens, button style resolver. |
| `widget` | Text, Label, Button, EditableText, TextField, ScrollView, and low-level layout controls. |
| `app` | `NanApplication`, `NanWindow`, `NanRouter`, `NanPage`, `NanStore`, app theme propagation. |

## Framework Capability Map

The framework is evaluated across twelve connected responsibilities. “Usable” means the current example and tests exercise the main path; it does not imply platform-complete behavior.

| Responsibility | Current contract | Next gaps |
| --- | --- | --- |
| 1. Window/display surface | `NanWindow` creates a raylib-backed visible surface and render device. | Multi-window policy, DPI/display changes, offscreen surfaces. |
| 2. Event loop | A1a formalizes input/process/tree-commit/layout/post-layout/paint/dispose phases. | UI task draining, tick-level reactive batching, reconcile/style phases, dirty-only paint. |
| 3. Input | Mouse/keyboard dispatch, hit testing, focus/hover, pointer editing. | Capture contract, native IME, shortcuts, gestures, drag/drop. |
| 4. Object model | Concrete `NanNode`/`NanControl` objects with virtual capabilities and C++ setters. | Unified property/event surface without replacing ordinary setters. |
| 5. Widget tree | Shared-owned scene tree, weak observations, enter/exit/ready lifecycle. | Keyed reconciliation and declarative region ownership. |
| 6. Layout | Bottom-up measure/top-down layout, typed invalidation, root correctness boundary, and bounded post-layout relayout. | Explicit layout boundaries, diagnostics, Grid/Anchor, richer intrinsic contracts. |
| 7. Paint/composition | Tree draw traversal, clip stack, typed paint dirtiness, replaceable render device. | Dirty-only paint, damage tracking, retained layers/caches, animation phases. |
| 8. Text | FreeType/HarfBuzz/FriBidi/utf8proc, fallback faces, editing geometry, CJK package. | Native IME, UAX #14, emoji/color glyphs, rich text, per-widget family request. |
| 9. Style | `NanTheme`, tokens/palette, primitive and Button variants. | `NanStyle`, style context/cascade, ThemeManager, structured style files. |
| 10. State binding | Signal/Computed/Effect/Scope and limited Label binding. | General properties, automatic bindings, `If`, keyed `ForEach`, no manual refresh. |
| 11. Async | No complete application-facing model yet. | UI dispatcher, background tasks, cancellation, coroutine adapters, stale-result policy. |
| 12. Accessibility/delivery | R1-R10 resource delivery, install/portable layouts. | Semantic tree, keyboard navigation contract, platform accessibility and app packaging. |

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

The text, clipping, editing, layout, interactive example, and R1-R10 resource-delivery line are complete. The active main line is application authoring: formalize runtime scheduling and invalidation, then add property binding, keyed reconciliation, async scope, style/theme context, accessibility, and finally a thin DSL over the same imperative widgets.

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

Status: A1a runtime contract implemented; task draining, tick-level reactive batching, style/semantics consumers, local layout boundaries, and dirty-only paint remain.

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

Current A1a decisions:

- Scalar/property mutation is synchronous. Input callbacks and later callbacks in the same dispatch can read the new value; reactive work, layout, and paint later in the same tick observe it.
- One reactive flush wave runs each effect at most once. Self-invalidation is retained for the next flush rather than looping in the current wave, and a flush has a hard cascade limit for diagnostics.
- `add_child` and destructive removal requested during process/layout/post-layout/paint are queued until traversal completes. `remove_child`, whose contract returns ownership synchronously, rejects use during those phases; callers use `remove_and_delete` or `queue_delete` when ownership is not needed. Single-child widgets use one deferred `replace_child` transaction so replacing content never exposes an intermediate detached state. Deferred deletes flush at tree-commit and frame-end disposal safe points.
- Post-layout actions are a formal queue. They may dirty layout and receive one additional root layout pass in the same tick; actions queued while draining and further invalidation remain for the next tick instead of creating an unbounded loop. Replacing the scene root discards callbacks owned by the previous frame/root.
- A setter called during paint updates the stored value immediately. Work whose phase has already passed, including layout or already-painted content, is visible in the next tick; paint never restarts midway.
- The window root is the current correctness-preserving layout boundary. Dirty measure/layout flags propagate through the full ancestor chain, including non-Control nodes, to prevent lost invalidation. Local layout roots are deferred until containers can declare stable constraint boundaries; replaying arbitrary subtrees with stale parent constraints is not valid.

The A1a tick implemented by `NanWindow` is:

```text
input
process / on_frame
commit deferred tree mutations
layout root
post-layout actions
optional second layout root pass
paint / present
commit paint-time tree mutations for the next tick
```

The Todo example now uses the formal post-layout queue for scroll-to-end instead of a hand-written extra-frame flag.

### A2. Property And Binding Core

Add `Property<T>`, read-only observation, and events while preserving ordinary setters. Both paths must converge on the same mutation/dirty logic:

```cpp
label->set_text("ready");
label->text_property().bind(scope, status);
```

Bindings activate/deactivate with the widget/page scope, batch updates within one tick, and cannot outlive captured application state. Add `Signal<T>::update(fn)` or an equivalent transaction API so state mutation does not require read-copy-set boilerplate.

### A3. Declarative Regions And Keyed Reconciliation

Implement low-level imperative objects for `If` and keyed `ForEach` before adding DSL wrappers. `ForEach` owns a key-to-node map, reuses unchanged nodes, moves nodes without recreating them, destroys removed scopes, and preserves focus/edit state. First version need not virtualize.

Todo success criteria: no `clear_children()` refresh, no hand-written synchronization effect, no explicit layout invalidation, and no recreated row for an unchanged key.

### A4. UI Dispatcher And Async Scope

Only the UI thread may mutate widgets. Add a UI dispatcher, background task execution, cancellation token, and page/widget-owned `AsyncScope`. Completion returns through the event-loop task phase and is discarded if the owner is gone or a newer generation superseded it. Coroutine syntax may wrap this model later; it is not the underlying contract.

### A5. Font Requests And Style Context

Replace the scene-wide fixed default `TextPipeline` assumption with a window font-resolution context backed by `FontFamilyRegistry` and `FontPipelineCache`. Extend text style/request with logical family, weight, and slant. Add imperative controls such as `set_font_family()`, `set_font_weight()`, and `set_font()`; explicit low-level pipelines remain a supported override.

Introduce four-state style values: unset, inherit, initial, and explicit value. Typography, text color, locale/direction, and opacity inherit by default. Background, border, radius, padding, layout, shadow, and component variants do not.

### A6. NanStyle And ThemeManager

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

### A7. Accessibility Semantics

Add a semantics capability/tree with role, label, value, state, actions, focus, and bounds. Semantic widgets provide defaults; composition decides whether to expose, merge, or hide primitive children. Dirty semantics update in the formal tick. Platform AT-SPI/UI Automation/NSAccessibility adapters come after the internal contract is stable.

### A8. Thin Authoring DSL

Only after A1-A7 are usable, add builders/DSL for composition and binding. DSL expressions return or expose concrete widgets and call the same setters/properties/events as imperative code. No DSL-only node, lifecycle, renderer, style, or state path is allowed.

Maintain paired tests/pages:

- imperative construction;
- authored construction;
- equivalent concrete widget access and mutation;
- equivalent binding lifetime, keyed reuse, layout, input, style, semantics, and teardown.

### A9. Todo Refactor And Component Extraction

Split Todo into semantic components (`TodoHeader`, `TodoComposer`, `TodoList`, `TodoRow`, `TodoEmptyState`) and first rewrite it using the new imperative Property/ForEach/Style APIs. Then add a second DSL authoring form over the same components. The example becomes the acceptance test that normal application code describes state projection and intent rather than framework refresh mechanics.

### Deferred After Authoring Core

- Router activate/deactivate/replace lifecycle, history, deep links, and transitions.
- Tween/animation primitives and reusable impact/ripple effects.
- Virtualized lists, richer Grid/Anchor layout, retained composition layers.
- Native IME, clipboard/undo, UAX #14, emoji and rich text.
- System font discovery as an explicit application feature.

## Development Workflow

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
- Which dirty flags does each property change?
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
