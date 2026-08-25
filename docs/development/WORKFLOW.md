# Nandina Iteration Workflow

> Tracked iteration contract for the current C++ v3 line.
> Phase documents (e.g. `PHASE8_RENDER_THEME_PLAN.md`) define **what** each step
> delivers; this file defines **how** every logical unit is executed, verified, and
> committed.

## 1. Iteration Loop

Every round delivers exactly one reviewable logical unit. A unit may span multiple
subsystem commits (see §3), but it must compile, pass the full suite, and be
reviewable on its own.

1. **Docs first.** Record the design decision, acceptance criteria, and target step in
   the phase document / roadmap before writing code. If code and docs have drifted
   (for example a step is implemented but still marked "awaiting review"), close the
   doc gap first so the docs remain the single source of truth.
2. **Implement** the unit. Components follow the frozen template in §4.
3. **Verify** against the gates in §2.
4. **User review.** The user inspects the running effect and the diff.
5. **Commit** following §3 once review passes.
6. **Sync docs.** Mark the completed step/status together with how it was verified, in
   the same round that closes it; update the capability map when the change alters a
   capability.

## 2. Verification Gates

Every round must pass, before review:

```sh
meson compile -C buildDir
meson test -C buildDir --print-errorlogs
git diff --check
```

Resource or font changes additionally run the focused targets:

```sh
meson test -C buildDir resource font-resource --print-errorlogs
```

Interaction- or paint-heavy changes are also validated visually in the Settings
example (or the closest real application) in both appearances before commit.

## 3. Commit Discipline

- Message format: `type(scope): 中文描述` (for example `feat(widget): 增加按钮涟漪反馈`).
- The body must record **motivation / core changes / verification** so `git log`
  remains sufficient for later audits.
- Keep theme, widget, render, app, and example changes in separate commits whenever
  their dependency order allows it. A unit that must cross subsystems is split into
  dependency-ordered commits; a single commit never mixes unrelated changes. If a
  cross-subsystem unit is intentionally kept as one commit, say so and justify it in
  the body.
- Pre-1.0: remove obsolete interfaces directly instead of accumulating compatibility
  debt.

## 4. Component Template (frozen contract)

Every new component must complete the whole template before it is considered done:

- Recipe composed of base + explicit rules; no widget-local flat resolver.
- Reuse existing tokens, recipe fragments, and shared painters (box, focus ring,
  ripple); never copy rounded-corner/focus/text drawing.
- Tests for light/dark, disabled, interaction states, and typed override.
- Layout measurement and narrow-constraint tests (logical size, percent, fill,
  min/max, aspect ratio).
- Complete mouse / keyboard / focus behavior.
- Semantics role, label, and value/state.
- Authoring factory plus any required two-way binding; register a typed
  `ComponentTraits<T>` customization — never add a method to `BuildContext`.
- `RecordingDevice` coverage where draw order matters.
- The example exercises real use; it is not a component gallery.

Component order follows risk from low to high so the template stabilizes before
complex interaction lands: **Badge → Card → ProgressBar → RadioButton**. Badge and
Card validate surface/type composition, ProgressBar validates track/value state, and
RadioButton (mutual exclusion, keyboard navigation, form semantics) comes last. A new
order must be decided and recorded in the phase document before it is executed, not
silently reordered during implementation.

## 5. Lifetime And Ownership Rules

- Callbacks must never capture a temporary `BuildContext` (each `build()` creates a
  new one). Capture only objects that outlive the build: ThemeManager pointers,
  signals, nodes.
- A retained page root must not invoke callbacks after the page scope is cleared.
  BuildContext propagates the page scope generation token; NodeBuilder-installed
  callbacks check the weak token before entering application code.
- Page-owned reactive state (signals / computeds / effects) is cleaned up by the
  ReactiveScope owned by the page frame; never hand-clean it manually.

## 6. Doc And Roadmap Sync

- Update the phase document, this file, and the capability map in the same change
  that closes a step.
- Mark completed steps as complete **with the verification method used** (headless
  tests, screenshots, example interaction).
- Do not leave "awaiting review" markers on already-verified work; close them in the
  next round at the latest.

## 7. Current Position

Snapshot of the active milestone and step, refreshed each round so history stays
readable:

**Current state (1.x line)**: the 1.0 core (scene/reactive/widgets/theme/router/text/
accessibility) and the 1.1 authoring/animation/image/font work are landed; `tests/` is 41/41 green.
The resource-delivery line R0–R10 and developer-experience D1–D2 are complete (D2 now includes
runtime build-tree package lookup and `[[resources]]` per-resource overrides). Next up is D3
(`nandina` CLI + application template); i18n is deferred past 1.0. Detailed history follows:

- Phase 8 Steps 0–8 are complete (see PHASE8_RENDER_THEME_PLAN.md). The full D6
  component order — Badge, Card, ProgressBar, RadioButton — is now complete
  against the template frozen in §4.
- Typed sizing was extended per D9: `set_min_*`/`set_max_*` accept percentage
  lengths, and `Button::set_font_size(percent)` resolves as a fraction of the
  control's own final height. Commits `6d6b74b`..`1514ded` cover scene, widget,
  authoring, and example layers with imperative + authoring tests.
- ProgressBar is a determinate, non-interactive control (recipe/rule resolution,
  disabled alpha transform, `progress_bar` semantics role, one-way `Signal<float>`
  authoring, recording-device paint coverage, and a real click-to-advance use in
  base_window).
- RadioButton closes the D6 order: a `RadioGroup` coordinating object enforces
  mutual exclusion and arrow-key roving focus, with a `radio` semantics role and
  the Settings appearance section (System/Light/Dark) as real use.
- The `butter` built-in theme family (warm cream + Catppuccin) is landed via
  `theme/builtin_themes.{hpp,cpp}` and `ThemeManager::register_theme_family`
  (full-snapshot families). Settings now selects it with `activate_family("butter")`
  instead of a hardcoded brand palette.
- `fluent` (Fluent 2 / Windows: cool grays + blue accent, sharp 4/8/12 radii) and
  `material` (Material 3: tonal neutrals + purple/pink, rounded 8/12/16 radii)
  families landed. `PaletteVariantPolicy` gained `light_on_brand`/`dark_on_brand`
  so `on_primary` flips with appearance for mid/dark accents; Settings adds a
  Theme-family Select (butter/fluent/material) driving `activate_family`.
- Soft shadows landed: `IRenderDevice::draw_rounded_rect_shadow` (SDF mode 4 soft
  falloff), `ShadowPainter`, `ShadowStyle` on `CardRecipe`, and a butter card
  shadow rule. Default Card stays flat; butter gets warm-clay elevation.
- Tabs landed: a horizontal tab bar with single selection, arrow-key roving focus,
  underline indicator, `tab` semantics role, and `Signal<int>` two-way authoring;
  Settings shows a General/Appearance/About tabs section.
- Tooltip landed: a hover-triggered floating bubble (primary fill + on-primary text)
  wrapping a trigger control, with delay/placement/override and a `tooltip`
  semantics role; Settings wraps the Reset button.
- Select landed: a single-choice dropdown with a popup option list drawn on a
  raised z-order, keyboard open/navigate/select/escape, `combobox` semantics, and
  `Signal<int>` two-way authoring; Settings adds a language select.
- Basic widgets landed: Divider (solid/dashed/double patterns, `separator`),
  Avatar (UTF-8-safe initials), and Chip (removable pill with `removed` event);
  Settings shows them in a Basics row inside a scrollable viewport.
- The router is now exercised by the example: `ShellPage` hosts a persistent
  sidebar and a **nested `NanRouter`** (real QML-StackView-style content stack),
  with section pages (General/Appearance/Components/About) backed by a shared
  `SettingsStore`, plus a parameterized `DetailPage` demonstrating push/pop.
  The bootstrap migrated to `use_store` + `run_page<ShellPage>`; the example
  budget test now permits `NanPageT`/`route_key` (raised to 700 lines).
- Dialog/Modal landed: a modal overlay (`scrim` + centered panel + title +
  content) with `dialog` semantics, Escape/scrim-click dismissal (configurable),
  a within-panel Tab focus trap, and full-screen positioning via a new `Stack`
  overlay layout. The example wires a "Reset → confirm dialog" flow; Settings
  Reset now pops the dialog before committing.
- System font discovery + CJK fallback landed: `text/system_fonts` scans
  cross-platform font directories for a known CJK face (Noto Sans CJK / Source
  Han Sans / Sarasa / PingFang / Microsoft YaHei / WenQuanYi …), mounts it as a
  resource, and registers it as the default fallback so CJK glyphs (中文/日本語)
  no longer render as tofu. NanApplication wires it up automatically and degrades
  silently when absent.
- Keyboard/focus pass complete: Chip (the last interactive control lacking it)
  is now focusable when removable, activates on Enter/Space and removes on
  Delete/Backspace, draws a focus ring via a new `focus` field in its recipe,
  and reports focused semantics. All interactive widgets now have keyboard
  access (Button/Checkbox/Switch/Radio via `Pressable`; Slider/Select/Tabs
  native; Dialog traps Tab).
- Text vertical alignment fixed: widgets now center text by its measured line
  height (`Text::measured_text_height()`, ascent+descent) instead of the em
  size (`laid_out_font_size()`), removing the "slightly low" offset across
  Button/Chip/Tabs/Select/Tooltip/etc. TextField already used line height.
- Tween/animation system landed (minimal 1.0 seed): `animation/easing.hpp` +
  `animation/tween.hpp` (generic `Tween<T>`, reduced-motion aware). Tabs
  underline slides between tabs and Dialog scrim/panel fade in on open. The
  declarative, per-property QML-style system (`AnimatedProperty<T>` + `Behavior`)
  is specced in `docs/development/ANIMATION.md` and deferred to 1.1 (needs
  per-node opacity + behavior config).
- Declarative animation 1.1 started with its first review unit: shared typed visual
  property paths use `part + field` identities (`visual::label.color`,
  `visual::container.radius`) instead of per-component property catalogs. Text and
  Box primitives own field behavior, components expose only their standard parts,
  unsupported combinations fail concepts at compile time, and BuildContext can bind
  tracked sources through the same path protocol.
- Declarative animation unit 2 adds typed `Behavior<T>` and `AnimatedProperty<T>` as
  host-independent value objects. Logical targets now differ from presentation values;
  disabled/zero-duration policies jump, active transitions continuously retarget from
  their current value, and `NanColor` interpolates in OKLCH over the shortest hue arc.
  Button was audited alongside this step: fixed font-size writes now share the label
  presentation entry, instance visual values outrank inherited StyleContext, font-only
  context changes are detected, recipe overrides refresh detached metrics, and interaction
  state invalidates layout/paint/semantics.
- Declarative animation unit 3 adds one `AnimationHost` per `NanSceneTree` and an explicit
  `animation` frame phase between reactive commit and layout. The Host stores only active,
  property-identity-deduplicated tracks, advances from caller-provided `dt`, propagates exact
  paint/layout/semantics dirty flags only when presentation values change, and completes plus
  cancels tracks synchronously when owners exit the tree. Manual-clock, retarget, cross-tree,
  clear, parent layout propagation, and keep-alive detach regressions are covered headlessly.
  Builder `.behavior(path, spec)` and endpoint-to-Host wiring are layered on this contract.
- Declarative animation unit 4 adds reusable owner-aware property endpoints and makes
  BuildContext-authored builders carry their current reactive scope. `.bind(path, source)` and
  `.behavior(path, spec)` now converge with ordinary setters on one logical target; Text and Box
  primitives expose the same protocol, invalid part/field combinations remain compile-time
  failures, detached initialization does not spuriously animate, and behavior replacement or
  override clearing synchronously reconciles Host tracks. Per-node opacity is the next review unit;
  endpoint-wide reduced-motion enforcement remains with the later Label/Button visual fixture.
- Declarative animation unit 5 adds `NanNode2D::local_opacity` and removes the redundant
  `StyleContext.opacity` inherited property that double-multiplied alpha down the tree. Effective
  opacity is now `parent effective × node local opacity`, each node multiplying exactly once via a
  `NanNode::local_opacity()` virtual hook (base opaque, Node2D overrides); `set_local_opacity`
  validates and clamps to [0,1] then marks only paint dirty through `NanNode::mark_paint_dirty()`
  (nearest Control ancestor). Opacity no longer affects visibility, input, or semantics. The old
  "0.5 → 0.25" inheritance test was replaced with parent-child / deep-nesting / sibling-isolation /
  context-restore draw assertions; 40/40 green.
- Declarative animation unit 6 wires global reduced-motion into `AnimationHost`: it reads
  `tree_->theme_manager()->reduced_motion()`, jumps new targets without registering a track, and
  clears in-flight tracks when the policy toggles on; toggling off resumes normal animation. No
  per-widget reduced-motion wiring is needed. The Label color / Button container radius vertical
  slice (OKLCH interpolation + radius transition through `.bind/.behavior`) was already exercised
  and now additionally proves the reduced-motion path; 40/40 green.
- Declarative animation unit 7 migrates Tabs and Dialog off hand-written `Tween` ticks. Tabs
  `indicator_x_/indicator_width_` and Dialog `fade_` are now `AnimatedProperty<float>` driven by the
  scene `AnimationHost`; Tabs drops `on_process` entirely. Dialog gains an
  `opening/opened/closing/closed` state machine: `close()` fades out and only hides + fires
  `on_close_` after the fade completes, and content children fade with the panel by overriding
  `Dialog::local_opacity()` (`NanNode2D::local_opacity() × fade_.value()`), so title/scrim/panel no
  longer hand-multiply alpha. A minimal Dialog `on_process` only transitions the state machine (no
  tween ticking). Reduced-motion stays centralized in the Host. Tests now advance via
  `AnimationHost` and assert the deferred-close contract; 40/40 green.
- Declarative animation unit 8 (combinators) adds `animation::Group` with `parallel` /
  `sequential` / `stagger`: each clip type-erases one `AnimatedProperty<T>` target + `Behavior<T>`
  and a `ready(elapsed)` trigger predicate (all-immediate / previous-finished / fixed-interval).
  `AnimationHost::run(owner, group)` hosts the Group as a single track on the same clock, reduced
  motion, and owner-cancel semantics; the Group is passed by value and owned via shared_ptr so a
  local Group can never outlive its track, and it is move-only (sequential's ready predicate points
  at its neighbour clip, which survives move but not copy). Router transition (page-exit lifecycle
  preservation) builds on this next. Tests cover parallel firing, sequential ordering, stagger
  intervals, jump-on-finish, and cancel-on-owner-exit; 40/40 green.
- Declarative animation unit 9 (spring) adds `SpringSpec` (stiffness/damping/mass, rejecting
  invalid parameters) and `Spring<T>` (semi-implicit Euler integration, settle detection,
  velocity-preserving retarget, `finish` jump). `AnimatedProperty<T>` gains `set_spring` /
  `clear_spring`, mutually exclusive with `Behavior`, enabled only for floating-point types via a
  lazy `SpringMemberSelector` so `NanColor` never instantiates the constrained `Spring`. Spring
  reuses the same Host/clock/cancel/retarget (no second scheduler). Tests cover overshoot + settle,
  retarget continuity, finish, invalid specs, and behavior/spring exclusivity; 40/40 green.
- Declarative animation unit 9 (keyframes) adds `Keyframe<T>` + `Keyframes<T>` (time-value pairs,
  strictly increasing, first at 0; interpolated via the shared `lerp` so arithmetic and `NanColor`
  both work, with the interpolated result cached so `value()` returns a stable reference).
  `AnimatedProperty<T>` gains `set_keyframes` / `clear_keyframes`, mutually exclusive with
  `Behavior`/`Spring`, and `set_target` clears keyframes back to tween/spring. Tests cover
  interpolation, finish-at-last-frame, invalid inputs, and exclusivity; 40/40 green.
- Declarative animation unit 8 (router transition) adds opt-in page transitions
  (`NanRouter::set_transition_enabled`, default off = instant). When enabled, each page root is
  wrapped in a `PageFrame` (`AnimatedProperty<float> opacity` + `local_opacity()` override); push
  fades in and pop/replace fades out. The replaced page moves to an `exiting_` list so its
  scope/async stay alive during the fade-out, and `PageHost::on_process` polls for completion before
  clearing the lifecycle, deferring the tree detach to `tree_commit` (since `remove_child` cannot run
  in the process phase). With transitions off the router behaves exactly as before. Tests cover
  fade-in, deferred teardown on pop, and immediate teardown when disabled; 40/40 green.
- Declarative animation (motion DSL) adds `animation/motion.hpp`: `motion::tween(duration).easing(...)`
  builds a `Behavior<T>` and `motion::spring().stiffness(...).damping(...)` builds a `SpringSpec`, with
  `motion::ease_*` named curves. `NodeBuilder` gains `.behavior(path, TweenSpec)` and
  `.spring(path, SpringSpec)` (spring only for floating-point paths, gated by `property::Springable`);
  spring is threaded through `PropertyEndpoint` / box+text presentations / `property::set_spring`.
  `SpringSpec` fields became private with `stiffness()/damping()/mass()` getter+fluent-setter overloads.
  The Settings example's Motion section now demonstrates both tween and spring; 40/40 green.
- Image/texture subsystem (Stage 1–3) adds RGBA image loading on the existing texture abstraction:
  `IRenderDevice::load_texture_from_file(path, ImageLoadOptions)` + `texture_size(handle)` (default
  no-op; raylib backend via `LoadImage → ImageCrop/ImageResize/ImageColorTint → LoadTextureFromImage`
  + bilinear filter), plus `widget::Image` (`NanControl`) with lazy load, natural size, `tint ×
  per-node opacity`, `source_rect` crop, `stretch/contain/cover` scaling + contain alignment, and
  `set_load_options` preprocessing. Loading is file-path based; nanres stays deferred. The Settings
  About page shows a brand image (`assets/nandina_logo.png` copied beside the executable via meson
  `configure_file`). Tests cover load-once, rects, crop, contain/cover/alignment, failed-load
  fallback, opacity, and load-options passthrough; 41/41 green.
- Slider value label: `widget::Slider` gains an optional value label (`set_show_value_label`) that renders
  the current `numeric_text(value)` above the track using a shared `primitives::Text` (inherited font,
  `on_surface_variant` color when not from style context). Opt-in so existing layout is unchanged;
  enabling it adds label height to `on_measure` and pushes the track down in `on_draw`. The Settings
  General page opts in for the interface-scale slider. Tests cover opt-in height increase, value-text
  tracking, and idempotence; 41/41 green.
- Custom font import (file-path basis, nanres deferred just like images):
  `FontLoader::load_file(path, face_index=0)` loads a `FreeTypeFontFace` straight from a filesystem
  path (no resource system), and `FontFamilyRegistry::register_face(family, face, weight, slant)`
  registers that loaded face directly (non-null face required) via a new `FontFaceSpec::direct_face`
  member that `resolve` short-circuits (skipping the resource `load`/dedup path). The Settings example
  registers `assets/demo-font.ttf` (an Inconsolata copy) as the `"demo"` family in `ShellPage::build`
  and renders an About-page label with `set_font_family("demo")`; registration is guarded by
  `PageContext::has_resource_services()` so the headless settings tests (no font services) don't throw.
  Tests cover file load + missing-file error and registry resolve returning the same direct face;
  41/41 green.
- Multi-font import: `register_face` now appends a face to an existing family (instead of rejecting a
  duplicate), so a family can hold regular/bold/italic variants loaded as separate files, and `resolve`
  picks the closest face by weight then slant. Tests register regular (400) + bold (700) + italic (400
  italic) under one family and assert each request resolves to the matching face; 41/41 green.
- Per-component font assignment: `text::find_system_font(file_hint)` locates a font file by name
  substring in the system font directories (so devs can import from `/usr/share/fonts`), then
  `load_file(path)` + `register_face(family, ...)` registers it, and `set_font_family` assigns it per
  component/instance (Button/Label/TextField all expose it; a Card title is just a child Label). The
  Settings About page registers `serif`/`sans`/`mono` families from system fonts and shows a serif
  title, sans body, mono code line, and two buttons on different families. Tests cover
  `find_system_font_in` (substring match, absent, empty hint); 41/41 green.
- Glyph-overhang clip fix: a clipped Text (`overflow = clip`, e.g. a `TextField` value) clipped to
  its measured advance width, which cut 1–2 px off the last glyph's ink when a full-width CJK glyph
  was mixed with proportional narrow latin/digits (the glyph atlas snaps bitmaps to the physical
  pixel grid, so the last glyph's right ink can exceed its `x_advance`). `glyph_overhang_allowance`
  (`max(2px, 8% × font_size)`) now widens the right edge of both the `Text::draw_at` clip and the
  `TextField` viewport clip so the last glyph renders fully. Tests cover the widened clip and updated
  the widget clip assertion; 41/41 green.
- D2 runtime metadata consumption: `NanApplication` now reads a `resource-location.json` at each
  scanned resource root and mounts the pointed build-tree package at that root's priority, falling
  back to the direct `<root>/resources.db` when no metadata is present (release/install). This wires
  the dev flow "put assets → meson compile → application resolves the build-tree package" without a
  source-tree copy or manual sync. Infrastructure: nlohmann/json vendored as a git submodule with a
  thin `foundation::parse_json` wrapper, plus `resource::read_build_location_metadata`. Tests cover
  the JSON wrapper, metadata parse/missing/
  malformed, and a NanApplication that resolves a package reachable only through the metadata;
  41/41 green.
- D2 per-resource overrides: `resources.toml` gains `[[resources]]` entries keyed by canonical
  `ResourceKey` (`key`, plus optional `media_type` / `storage` / `streaming`) that override the glob
  `[[rules]]` and signature/extension detection for one resource. `build_resource_lock` applies glob
  rules first, then the per-key override (media_type/storage/streaming), so the lock and package
  honor the explicit choice. `nanres-cli` tests cover a media_type + storage override and reject an
  invalid key; 41/41 green. D2 is now complete; D3 (`nandina` CLI + template) is next.
- Test suite 41/41 green.
- Next (planned follow-ups, 1.1): D3 `nandina` CLI + application template; i18n language packs
  (deferred past 1.0); keyframes/ColorSpace DSL sugar + component motion slots; image 9-patch/atlas
  + audio (raylib audio) as separate media milestones.
