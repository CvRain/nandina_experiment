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
| `resource` | Stable UUID/key identities, immutable handles, bounded streams, resource URIs/platform locations, prioritized manager, builtin/memory/directory/SQLite backends. |
| `text` | FreeType/HarfBuzz/FriBidi/utf8proc layout, fallback families, atlases, resource font loading, pipeline cache. |
| `theme` | `NanTheme`, palette/tokens, button style resolver. |
| `widget` | Text, Label, Button, EditableText, TextField, ScrollView, and low-level layout controls. |
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

The text, clipping, editing, layout, and interactive-example foundations are complete. The active main line is resource delivery: make the existing runtime backends and font pipeline automatic, portable, packageable, and low-boilerplate for application authors.

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
| R1 Builtin/default font | Complete in the current worktree. | Read-only BuiltinBackend, embedded Caskaydia Cove font/OFL license, `fonts/default`, and `families/default-ui`. |
| R2 Resource URI/locator | Complete in the current worktree. | Strict `res`/`builtin`/`user`/`cache`/`file` URIs and deterministic Linux executable/XDG resource locations. |
| R3 Resource streams | Complete in the current worktree. | Bounded read/seek streams with stable metadata, independent ownership, snapshot/file implementations, and backend overlay lookup. |
| R4 `nanres` scan/validate | Complete in the current worktree. | Deterministic recursive scanner, ordered media detection, exclusions, unsafe-path diagnostics, and functional `init`/`scan`/`validate` CLI. |
| R5 Policy/lock manifest | Complete in the current worktree. | toml++ policy parsing, SHA-256 inventory, stable UUID move/change rules, revisions, stale validation, and atomic generated lock updates. |
| R6 SQLite package/sidecars | Complete in the current worktree. | Runtime-compatible SQLite packages, alias rows, policy/size-based BLOB selection, UUID-named external sidecars, atomic rebuilds, and fingerprint skips. |
| R7 Meson build/install | Complete in the current worktree. | Manifest/source-aware validate/package targets, build-tree executable-relative output, datadir install helper, and user/system prefix layout tests. |
| R8 Application bootstrap | Complete in the current worktree. | Application-owned resource/font services, built-in bootstrap, locator-driven SQLite mounts, process config discovery, and PageContext service access. |
| R9 Window text pipeline | Complete in the current worktree. | Render-device-scoped default FontPipelineCache, scene-context inheritance, explicit override preservation, and ordered scene/GPU teardown. |
| R10 Cleanup/verification | Complete in the current worktree. | Removed temporary example resource/font setup and verified package, portable, prefix-install, and builtin-fallback modes. |

Remaining M1-M6 follow-ups are deferred rather than blockers: UAX #14 line breaking, OpenType ligature-internal carets, native IME acquisition, clipboard/undo, scrollbar chrome, kinetic scrolling, Grid/Anchor, exact transformed polygon clipping, and accessibility bridges.

### Current Resource Runtime

`ResourceId` is a stable binary UUID, `ResourceKey` is a validated logical name, and immutable `ResourceHandle` snapshots own bytes independently of backend lifetime. `ResourceManager` mounts deterministic priority overlays and stops on backend errors rather than silently exposing a lower layer.

`BuiltinBackend` is a read-only process-shared lowest-priority source for framework resources. It embeds the Caskaydia Cove default font and its OFL license in `libnandina`, exposes stable `fonts/default` and license resources, and supports the `families/default-ui` registration contract without filesystem or system-font dependencies. `MemoryBackend` supports runtime/test overrides. `DirectoryBackend` consumes explicit caller-provided entries and returns owned file snapshots. Read-only `SQLiteBackend` supports canonical key/UUID lookup, aliases, BLOBs, relative external files, size checks, and schema identification through `application_id`/`user_version`. SQLite remains a private C API implementation; Meson prefers a compatible system package and provides a pinned checksum-verified static amalgamation fallback.

`FreeTypeFontFace` can retain a resource-backed memory face. `FontLoader` caches by `(ResourceId, face_index)`, `FontFamilyRegistry` resolves aliases/weight/slant/fallback order, and render-device-scoped `FontPipelineCache` owns HarfBuzz, per-face atlases/textures, and renderer bindings. The Todo example exercises this stack entirely through application/window bootstrap and scene inheritance.

### Active Resource Delivery Sequence

#### R1. BuiltinBackend And Default Font

Status: complete in the current worktree.

The licensed Caskaydia Cove regular font and OFL text are generated into `libnandina` at build time and exposed by the read-only process-shared `BuiltinBackend`. Stable `fonts/default` and `families/default-ui` contracts provide a portable default family while allowing higher-priority application mounts to override the logical font key. Automatic mounting and widget inheritance remain R8-R9 responsibilities.

#### R2. Resource URI And Platform Locator

Status: complete in the current worktree.

Add URI schemes without weakening stable logical keys:

- `res://` for read-only application resources.
- `builtin://` for forced framework resources.
- `user://` for writable application data.
- `cache://` for disposable data.
- `file://` for explicit filesystem access.

`ResourceUri` strictly parses canonical logical keys for `res`, `builtin`, `user`, and `cache`, while `file` requires an explicit absolute POSIX path. `PlatformResourceLocator` validates the application ID and executable path, then deterministically yields executable-relative, XDG user, XDG system, `/usr/local/share`, and `/usr/share` locations with duplicate removal. It also provides XDG user-data and cache write roots. macOS and Windows location providers remain required before those platforms are claimed as supported.

Runtime discovery uses the executable path, application ID, install prefix conventions, and platform locations rather than compiled absolute paths. Linux search order is executable-relative resources, `$XDG_DATA_HOME/<app-id>`, `~/.local/share/<app-id>` when unset, `$XDG_DATA_DIRS/<app-id>`, `/usr/local/share/<app-id>`, `/usr/share/<app-id>`, then builtins. Installation mode follows Meson's prefix; do not branch on whether the installer is root.

#### R3. Streamed Resources

Status: complete in the current worktree.

`ResourceStream` exposes stable ID/key/media/storage metadata, declared size, position, seekability, bounded read, and absolute seek. Memory, builtin, and SQLite BLOB streams retain immutable resource snapshots. Directory and SQLite external streams retain independent file handles, verify declared size when opened, stop reads at the declared boundary, and report premature EOF as an I/O error. Open streams remain valid after backend unmount/destruction. Snapshot and stream size limits are separate so explicitly large external resources can stream without weakening normal `find()` safeguards. Content-hash verification begins when R5 manifests provide authoritative hashes.

#### R4. `nanres` Scan And Validation CLI

Status: complete in the current worktree.

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

Status: complete in the current worktree.

Human policy is separate from generated inventory:

- `resources.toml`: package ID, root directories/key prefixes, excludes, aliases, hidden/symlink policy, and ordered glob media/storage/streaming rules.
- `resources.lock.toml`: tool-owned format/package header plus UUID, canonical key, policy-relative source path, media type, size, SHA-256, storage decision, streaming flag, and revision for each resource.

Developers do not manually enumerate ordinary resources. Without explicit R4 command-line scan overrides, `nanres scan` reads `resources.toml`, scans and hashes resources, preserves UUIDs by existing source and unique content-hash move detection, increments revision only when content changes, and atomically replaces `resources.lock.toml`. Only genuinely new resources receive IDs. `nanres validate` regenerates the inventory in memory and fails when the lock is missing or stale without modifying it. Ambiguous hash moves, duplicate identities, alias/canonical collisions, missing alias targets, package mismatches, and normalized path/key collisions fail instead of silently changing identity. toml++ prefers the compatible system package and retains a wrap fallback; SHA-256 uses OpenSSL EVP in the `nanres` target rather than adding TOML or crypto dependencies to `libnandina`.

#### R6. SQLite Packaging And External Sidecars

Status: complete in the current worktree.

`nanres pack` requires a current validated lock and generates `resources.db` plus an `external/` sidecar directory under the configured `package_directory` (default `package`). The database schema, `application_id`, `user_version`, aliases, BLOB rows, and external paths are consumed directly by `SQLiteBackend`. Explicit storage rules win; `auto` embeds non-streaming resources up to `embed_threshold` (default 1 MiB) and keeps streaming, audio/video, and larger files external. Streaming plus explicit embedded storage is rejected. External files use stable UUID filenames rather than source paths, and every source SHA-256 is rechecked before packaging. A package-policy/lock fingerprint skips unchanged complete output; otherwise a complete temporary tree is transactionally built and swapped into place so stale sidecars are removed. Raw R4 command-line scans cannot write release packages.

#### R7. Meson Build And Install Integration

Status: complete in the current worktree.

The example declares short Meson custom targets around the reusable `nanres_build_helper.py` and `nanres_install.py` scripts. Policy, generated lock, and explicitly enumerated source files are target inputs; `nanres validate` runs before `pack`, and the complete package is produced beside the executable at `buildDir/example/resources`. The source list is explicit because Meson cannot discover arbitrary TOML inventory dependencies at configure time; `nanres` still verifies every SHA-256 before packaging. The install script copies the complete database/sidecar tree to `get_option('datadir')/<app-id>`. On Linux, `--prefix=/usr` therefore installs under `/usr/share/<app-id>`, while `--prefix=$HOME/.local` installs under `~/.local/share/<app-id>` without checking whether the installer is root. Tests exercise both prefix layouts and open the Meson-generated package through `SQLiteBackend`.

Fully offline builds use compatible system dependencies or pre-populated Meson `subprojects/packagecache` archives for the pinned wraps. Configure/build never downloads Sarasa Gothic or other optional resource packs implicitly. Generated SQLite fallback source trees, build outputs, resource databases, package fingerprints, and lock-update temporaries remain outside source control; generated `resources.lock.toml` is the intentional exception and is committed as application inventory.

#### R8. NanApplication Resource Bootstrap

Status: complete in the current worktree.

`NanApplication` now owns `ResourceManager`, `FontLoader`, and `FontFamilyRegistry` for the process lifetime. It always mounts the process-shared built-in backend at priority -1000 and registers `families/default-ui`. `NanApplicationConfig` accepts an application ID, executable path, environment snapshot, and optional package filename; `for_process()` resolves `/proc/self/exe` and HOME/XDG variables on Linux. Configured startup uses `PlatformResourceLocator` order to mount each existing `<root>/resources.db` at descending priorities with sidecars rooted beside the database. Missing packages are skipped, while malformed discovered packages fail startup rather than exposing a lower overlay silently. `NanApplication`, application-created routers, and `PageContext` expose the same resource manager, font loader, and family registry. Direct low-level `NanRouter` construction remains valid without application services. The Todo example configures only `org.nandina.todo`.

#### R9. NanWindow Default Text Pipeline

Status: complete in the current worktree.

After render-device creation and before `on_setup()`, `NanWindow` resolves the application default family and owns the render-device-scoped `FontPipelineCache`, `FontPipeline`, and backend-neutral `TextPipeline`. `NanSceneTree` carries that default pipeline context. Nodes receive it when entering the tree through a zero-RTTI virtual capability; Text/Label, Button, EditableText, and TextField inherit it, including dynamically added subtrees and internal text primitives. An explicit `set_text_pipeline()`, layout backend, or renderer remains authoritative and is never overwritten by context inheritance. Close clears router frames and the scene root, removes the tree context, then releases FontPipeline, cache, and finally the render device, so no page/widget retains raw renderer pointers after GPU text resources are destroyed.

#### R10. Example Cleanup And Install Validation

Status: complete in the current worktree.

The Todo example no longer contains `NANDINA_EXAMPLE_RESOURCE_DIR`, manual DirectoryBackend/resource/font services, `TodoPageParams::text_pipeline`, repetitive `set_text_pipeline()`, or manual resource teardown. The obsolete `bundled_fonts` Meson option, copy targets, and copy-specific font test are removed. R4 explicit scanning still covers loose development trees, while the application path uses the validated R7 SQLite package. Automated probes verify the generated package through `SQLiteBackend`, executable-relative portable layout, user/system datadir installation, and built-in font fallback when no optional/project package is present. Sarasa Gothic remains optional and is not downloaded or required for configure, build, startup, or tests.

### Simplified Chinese Example Fallback

The Todo example includes Sarasa Fixed SC Regular 1.0.40 under OFL 1.1. Only the 24.9 MB regular face is retained from the verified `SarasaFixedSC-TTF-1.0.40.7z` release archive; the archive and unused weights are not committed. `source.toml` records the upstream URL, version, archive SHA-256, extracted-font SHA-256, and license. The font is exposed as `fonts/fallback/zh-cn`, packaged as an external sidecar, and registered as `families/zh-cn` after `families/default-ui`. It is not marked streaming because current FreeType loading requires an immutable snapshot. The Todo interface and initial data contain mixed Simplified Chinese/Latin text, and tests verify actual `中`/`文` glyph coverage. Configure/build perform no network download; without the project package, optional registration is a no-op and the built-in default remains available.

### Deferred Resource And Font Work

- Define monochrome emoji fallback, then a separate color glyph/bitmap renderer path.
- Add project hot reload after identity, lock manifest, and pipeline invalidation semantics are stable.
- Add system font discovery only as an explicit developer/application feature; it is not part of default portability.

### Side Tracks

These are useful, but should not interrupt the active resource delivery sequence unless a feature directly requires them.

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

For resource/font changes, also run the focused targets:

```sh
meson test -C buildDir resource font-resource --print-errorlogs
```

SQLite dependency changes must validate both the normal system-dependency path and an isolated configure using `--force-fallback-for=sqlite3`. Once R4-R7 land, CI should additionally run `nanres validate`, verify deterministic lock/pack regeneration, launch from an executable-relative package layout, and test Linux user-prefix and system-prefix install trees.
