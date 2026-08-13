# Phase 8: Render Quality And Theme Evolution

Phase 6 and Phase 7 established recipe-driven component styles, atomic DesignSystem snapshots,
light/dark appearance switching, shared painters, and the Switch end-to-end component path.
Phase 8 pauses component expansion long enough to stabilize rendering quality and state-layer
semantics, then uses new components to verify the resulting authoring template.

Work continues in one-reviewable-unit increments. Every unit must compile and pass the complete
test suite before review, and theme, widget, render, app, and example changes remain separate
whenever their dependency order allows it. The full iteration contract (docs-first discipline,
verification gates, commit format, and the frozen component template) is tracked in
`WORKFLOW.md`.

## Audit Summary

The current implementation follows the intended primitives + tokens architecture:

- DesignSystem base recipes and ordered rules are the component-style source of truth.
- ThemeManager atomically publishes immutable snapshots and selects light/dark semantic palettes.
- Button, Checkbox, Slider, TextField, and Switch reuse shared box and focus painters.
- Switch covers recipes, authoring, two-way binding, input, semantics, and headless tests.
- Router page frames already own ReactiveScope; page-local signal/computed/effect cleanup is not a
  missing feature anymore.

The audit also found the following defects or process drift:

- The SDF draw quad previously ended at the shape boundary, clipping the exterior half of the
  antialiasing transition. Centered outlines also lost their exterior half-width.
- The SDF smoothstep interval was about 1.5 pixels wide, making small controls look soft.
- A zero-length segment divided by zero in the shader instead of producing a round dot.
- A shader that loaded but failed uniform validation leaked its GPU object.
- Commit `7d73372` combined theme and widget work. Existing history will remain intact, but Phase 8
  returns to subsystem-sized commits.
- Several recent commits have useful titles but no body. New commits should record motivation,
  core changes, and verification so `git log` remains sufficient for later audits.

## Decisions

### D1: Rendering Quality Precedes More Components

Do not add business-facing widgets until SDF primitives and common text sizes have a repeatable
quality baseline. Otherwise the same backend defect appears as multiple component defects.

### D2: UI Edges Use Analytic SDF Antialiasing

MSAA remains an optional window capability, not a correctness requirement. Rounded rectangles,
circles, outlines, and segments use SDF coverage. Their draw quad must include the exterior AA band
and, for centered outlines, the exterior half-width.

### D3: Text Starts With Grayscale Antialiasing

The current FreeType 8-bit alpha atlas is the portable default. RGB subpixel rendering depends on
pixel order, compositor behavior, transparency, and display scale, so it is not enabled globally.
The next text-quality unit will first measure physical-pixel positioning, 1:1 texture filtering,
atlas size rounding, baseline placement, and 1x/1.25x/1.5x/2x scale behavior.

### D4: Reference Scales Are Authoring Inputs

Adding an unused `default_reference_palette()` would create duplicate defaults. The complete unit
must add the Skeleton-derived eleven-stop scales and a semantic palette generator:

```text
NanReferencePalette + PaletteVariantPolicy
                    -> make_color_scheme()
                    -> DesignSystem.light / DesignSystem.dark
```

Components continue consuming semantic colors only. A Material-like dark 400-tone choice is a
variant policy rather than another component resolver.

The built-in primary scale intentionally extends the warm Phase 7 brand color into eleven tones;
the other six scales use the documented Skeleton reference values. `PaletteVariantPolicy` selects
brand tones only (primary, secondary, and tertiary). Status colors retain their explicit 500/on-tone
mapping so a dark-brand preference cannot silently change success, warning, or error semantics.

### D5: Static State Overlay Precedes Ripple

First change StateLayerStyle from replacement fill colors to an independently painted overlay.
Button paints its base container, then the same-radius overlay, without mutating the base recipe.
Ripple follows in a separate unit with press origin, frame scheduling, rounded clipping,
cancellation, and reduced-motion behavior.

### D6: Verify Components From Low To High Interaction Risk

After the foundations, use `Badge -> Card -> ProgressBar -> RadioButton`. Badge and Card validate
surface/type composition, ProgressBar validates track/value state, and RadioButton deliberately
comes last because it also requires group selection, keyboard navigation, and form semantics.

### D7: BuildContext Remains A Lightweight Handle

BuildContext contains non-owning Graph, ReactiveScope, and ThemeManager pointers. Copying it by value
is cheap and supports derived regions; converting the facade itself to shared ownership does not fix
a lambda that captured a local BuildContext by reference.

Long-lived callbacks capture only required stable services or page state. The remaining lifetime
risk was an externally retained page root invoking callbacks after its Page Frame had cleared page
signals. The selected fix gives each ReactiveScope generation a weak lifetime token and makes
BuildContext-authored callbacks check that token before entering application code. Clearing the page
scope invalidates the old generation before destroying signals, while a reused scope publishes a new
token. A retained-root regression verifies callbacks are inert after page pop without extending the
page or its reactive state.

### D8: Layout Fractions, Viewport Scale, And DPI Are Separate

Responsive Flex/Flow/Grid layout continues to consume a logical viewport. A child that occupies a
fraction of its parent belongs to layout constraints (`FractionallySizedBox` / `AspectRatio`) and
must not stretch its text or border geometry. Scaling an entire fixed-design interface is instead a
uniform logical-to-screen viewport transform with contain/cover and anchoring; drawing, clipping,
hit testing, pointer coordinates, and semantics must share its inverse mapping.

DPI scale is a third value: physical framebuffer pixels per screen-space unit. It controls SDF pixel
width and the physical size of cached glyphs. User accessibility scale multiplies typography and
control metrics before layout. These values must remain explicit rather than being collapsed into a
single node transform.

Component sizing uses one typed constraints model in both imperative and authoring APIs:

```cpp
control.set_width(240);                    // logical UI units
control.set_width(scene::percent(50));     // 50% of the finite parent constraint
control.set_width(scene::fill);            // all available width
control.set_width(scene::content);         // intrinsic content width
control.set_min_width(120);
control.set_max_width(480);
control.set_aspect_ratio(16.0F / 9.0F);

ui.make<widget::Button>("Save")
    .width(percent(50))
    .min_width(120)
    .aspect_ratio(16.0F / 9.0F);
```

Plain numbers deliberately mean logical UI units, so application code does not need a `_dp`
suffix. Strings such as `"50%"` are reserved for external style/configuration parsers; the C++ API
remains compile-time typed. A percentage reserves a fraction of the parent's finite available
axis, while `Expanded` / `FlexItem` distributes remaining main-axis space. On an unbounded axis a
percentage or `fill` falls back to intrinsic content sizing instead of manufacturing infinity.
QML-style arbitrary `parent.width / 2` property dependencies are not the default layout mechanism,
which keeps measure dependencies acyclic and diagnostics local to the layout tree.

Typography does not derive from a component's percentage width. Ordinary responsive layouts keep
the selected typography role stable while containers reflow; optional responsive type ramps may
switch roles at explicit breakpoints. Fixed-design viewport scaling multiplies component geometry,
font raster size, radii, borders, and spacing together. User accessibility/interface scale is
applied to typography and control metrics before layout, so larger text can legitimately trigger
remeasurement instead of being stretched after layout. Direct `font_size(...)` remains a local
literal override; relative `em`-style units can be added later for inherited typography, without
introducing a dependency on parent component width.

## Delivery Sequence

### Step 0: SDF Coverage Repair

Status: complete. Verified with `[render][sdf]` unit tests covering quad bounds, outline inset and
edge snapping, and zero-length segments, plus zoomed visual checks of corners, borders, checkboxes,
switches, sliders, and focus rings in both Settings appearances.

- Separate shape bounds from the expanded draw quad.
- Preserve a one-pixel exterior AA band and an outline's exterior half-width.
- Tighten analytic coverage to approximately one physical pixel.
- Treat zero-length segments as round dots.
- Release a shader that fails uniform validation.
- Unit-test fill, outline, and zero-length segment bounds.
- Treat component bounds as the border's outer edge and inset the SDF centerline by half its width,
  so a 1px border occupies one physical pixel row instead of two half-transparent rows.
- Snap all four outline edges independently to the logical pixel grid before the half-width inset;
  text-derived fractional widths can otherwise leave only vertical edges between pixel columns.

Suggested commit: `fix(render): 修正SDF图元软边裁剪与模糊`

### Step 1: Text Clarity Baseline

Add a repeatable diagnostic surface for common sizes and scales before changing hinting, pixel
snapping, atlas filtering, or DPI behavior.

The first 1x baseline keeps FreeType grayscale AA and HarfBuzz subpixel advances, while using the
rasterized bitmap's `bitmap_left/bitmap_top`, snapping only the final bitmap origin, and sampling
the atlas with point filtering. This avoids applying GPU interpolation to an already-antialiased
glyph without changing shaping, caret, wrapping, or layout widths. Fractional/high-DPI scaling
remains a separate policy and must not silently stretch the 1x atlas.

Status: complete for the current 1x logical-pixel window path. The Settings example covers 16px
body/control text, 18px section labels, and a 28px heading in both appearances. The explicit
logical-to-physical scale and physical-size glyph caching previously required before claiming
1.25x/1.5x/2x support is delivered by Step 4C.

Suggested commit: `fix(text): 统一字形像素对齐与缩放策略`

### Step 2: Generate Semantic Palettes From Reference Scales

Land the eleven-stop reference data, variant policy, generator, consistency tests, and switch
`default_design_system()` to the generated light/dark palettes in one theme-only unit.

Status: complete. Verified with `[theme][palette]` tests for reference scales, generator mapping,
variant policy, and legacy construction compatibility. `default_reference_palette()` is now the
single source for seven eleven-stop OKLCH scales, and `make_color_scheme()` derives both default appearances.
The default policy keeps brand colors at 500 in both appearances to preserve Phase 7 visuals;
`material_dark_tone()` is an opt-in policy that selects 400 for dark primary/secondary/tertiary.
`NanColorScheme{}` remains equivalent to the generated light scheme for detached-widget and legacy
API compatibility. Reference scales remain build-time authoring data and do not enlarge immutable
DesignSystem snapshots consumed by widgets.

Suggested commit: `feat(theme): 从参考色阶生成默认语义调色板`

### Step 3: Paint A Real Static State Layer

Migrate Button from replacement fill semantics to an overlay and verify base/overlay/outline/focus
draw order with a recording render device.

Status: complete. Verified with `[widget][button][state-layer]` recording-device tests asserting
base → overlay → outline/content → focus draw order, plus disabled and link no-overlay cases.
Button resolution now preserves `container.fill` for
every interactive state and exposes the active state-layer color separately. Filled buttons overlay
`on_accent`; tonal, outlined, and ghost buttons overlay `accent`, using the shared hover/pressed
opacity tokens. Drawing order is base fill, same-radius state overlay, outline, content, then focus
ring. Link state layers remain transparent, and disabled buttons do not paint an overlay.

Suggested commit: `refactor(theme): 将按钮状态色改为独立叠加层`

### Step 4A: Define Fixed-Design Viewport Mapping

Status: complete. Verified with `[app][viewport]` unit tests for contain/cover anchoring, input
round-tripping, and invalid-space rejection. `ViewportScalePolicy` defines a fixed logical design
size, contain/cover behavior, and two-axis anchoring. The resulting `ViewportMapping` provides one
uniform transform, content bounds, and inverse screen-to-logical conversion. It is pure geometry:
no window backend, scene tree, or font cache behavior changes in this unit.

### Step 4B: Add Typed Component Sizing

Status: complete. Verified with headless geometry assertions at two window sizes plus the
resizable-window Settings check (50% Save width). `NanControl` owns a typed size specification
shared by imperative calls and `NodeBuilder`: logical fixed values, parent percentages, fill/content,
min/max limits, and aspect ratio. Resolution occurs in the common measurement protocol, so existing
widgets do not duplicate sizing code. Percent and fill require a finite parent axis and otherwise
retain intrinsic measurement. Flex remains a separate remaining-space policy. The Settings Save
action uses a constrained 50% width, pairing headless geometry assertions at two window sizes with
a directly observable resizable-window check.

### Step 4C: Integrate Window Scale And DPI

Add an opt-in fixed-design policy to WindowConfig, then apply the same mapping to root layout,
DrawContext, clipping, input, and semantics. Introduce explicit framebuffer/DPI scale and build font
pipelines at the corresponding physical size before claiming non-1x text support. Default windows
remain responsive and behavior-compatible.

The render foundation is complete: `DrawContext` carries separate
logical-to-screen and screen-to-physical factors, preserves an initial viewport transform through
CanvasLayer traversal, and shared box/focus painters scale logical radii, borders, gaps, and ring
widths. Glyph layout remains in logical units, while the atlas rasterizes at logical font size ×
viewport scale × DPI; bitmap destinations divide only by DPI to return to screen space. Baselines,
advances, offsets, fallback drawing, line progression, and text clips convert through the viewport
factor. No WindowConfig behavior changes yet; component-internal geometry must join this contract
before enabling a non-1x viewport in the example.

The component integration follows three invariants learned from the archived Zig implementation:

1. measurement, text constraints, and theme tokens remain logical units;
2. world/screen geometry is never fed back into `measure_layout()`;
3. internal points and lengths convert only while submitting paint commands and never write scaled
   values back to node bounds.

Button, Checkbox, Switch, and Slider now follow these rules. In particular Button no longer uses its
already-scaled world width as a text measurement constraint, which previously made a future non-1x
paint mutate logical text layout. TextField/EditableText remains a separate unit because caret,
selection, scroll offset, hit testing, and IME geometry must change together.

#### TextField/EditableText closure

The text-editing pair now completes the same contract. Caret and selection geometry scales from
logical caret stops at paint time; their x offsets, heights, and caret thickness never enter text
measurement. TextField keeps padding and horizontal scroll in logical units, converts them only for
the viewport clip and draw origin, and centers placeholder/value baselines using the scaled line
height. Pointer hit testing remains logical through `to_local()`, so the future window-level inverse
viewport mapping has one owner and cannot apply the scale twice. A 2x headless regression covers
selection width/height and caret position/thickness.

The optional `WindowConfig::viewport` now activates this mapping at the window boundary: root layout
uses the design size, `DrawContext` receives the anchored logical-to-screen transform and framebuffer
scale, and mouse positions/deltas are inverted before scene dispatch. Leaving the option unset keeps
the existing responsive-to-window path unchanged.

Semantic snapshots now receive the same root transform, so accessibility bounds remain aligned with
the painted controls under contain/cover scaling while the default identity transform preserves
logical window behavior.

Status: complete. Verified with `[app][viewport]` mapping tests, the TextField/EditableText 2x
headless regression (selection geometry and caret stops), and the default responsive path, which
remains behavior-compatible when `WindowConfig::viewport` is unset.

### Step 5: Ripple And Reduced Motion

Status: complete.

Step 5A establishes the policy boundary before any component animation: `NanMotionTokens` carries
short/medium/long durations, while `ThemeManager` resolves `system/full/reduced` motion preference
and publishes a revision only when the effective reduced-motion value changes.

Step 5B adds ripple color and duration to the Button recipe and typed override path. Button records
only the pointer-local impact origin and normalized progress; `RipplePainter` owns eased expansion,
farthest-corner geometry, fade-out, and opacity composition. The render-device contract exposes a
circle intersected with a rounded rectangle, implemented analytically by the raylib SDF backend, so
the effect follows the same radius as the container without manipulating `DrawContext` clipping.

Attached Buttons consume the effective `ThemeManager::reduced_motion()` value. Changing that policy
or disabling a Button cancels an in-flight ripple, and reduced motion prevents new pointer ripples
without changing keyboard or semantic activation. Settings binds its visible preference directly to
the manager, providing both an application example and a headless integration test.

### Step 6: Normalize The Brand Theme Example

Demonstrate paired primary/on-primary colors for both appearances, contrast constraints, and an
optional tone policy instead of presenting a radius-only customization as the brand example.

Status: implemented; awaiting user visual review. Settings applies the official Catppuccin
palette (MIT): Latte neutrals for light (`#eff1f5` base) and Mocha neutrals for dark (`#1e1e2e`
base), with Peach as the warm-orange primary (`#fe640b` light / `#fab387` dark via the tone
policy) and a Flamingo/Maroon tertiary scale. `on_primary` follows the style-guide "On Accent =
Base" rule (Mocha base), keeping AA text contrast in both appearances. Card-style radii
(12/16/24) and 2–3px borders are expressed as token overrides; soft shadows are deferred until
the render device has a shadow primitive. The page showcases primary, coral, variant, and
outline semantic colors (the on-primary pairing is demonstrated by the filled Save button)
while keeping theme switching and automatic refresh. `foundation/contrast.hpp` adds WCAG
relative luminance and contrast ratio helpers, and the settings tests assert AA text contrast
for primary/on_primary, surface/on_surface, and background/on_background in both appearances.

### Step 7: Freeze The Component Template

Implement one component at a time and require recipe/rules, shared painters, light/dark and override
tests, constrained layout, pointer/keyboard/focus behavior, semantics, authoring, and any necessary
two-way binding. The example remains a real application, not a component gallery.

The authoring entry point is generalized by A22. New components must register a typed
`ComponentTraits<T>` construction customization and must not require adding another method to
`BuildContext`. The pre-1.0 named component factories have been removed. Built-in traits are exposed
through `widget/controls.hpp`, while `widget/build_context.hpp` stays independent of the concrete
control catalog, keeping the component acceptance template strict without making the context header
monolithic.

### Step 8: Close Page Root/Scope Lifetime

Complete. The existing page-owned ReactiveScope now exposes a generation lifetime token;
BuildContext propagates the page token through derived component/region contexts, and NodeBuilder
guards application callbacks installed through `on_click`, `on_submit`, and `on_change`. Router and
authoring teardown tests cover both retained roots after pop and scope reuse. No second reactive
lifetime system or page/root ownership cycle is introduced.

## Deferred Work

- `styles.toml` font-family integration waits for the resource manager path to mature.
- RGB subpixel text waits for a display-scale and compositor policy.
- Vulkan/SDL platform work remains outside the pre-2.0 plan.
- Large component expansion waits for Steps 0 through 3.
- Soft UI shadows (claymorphism double shadow stacks) wait for a render-device
  shadow primitive with SDF soft-edge falloff, so themes like the Settings brand
  example can add elevation without a per-widget texture pass.
- Built-in theme families: the Settings brand theme currently lives in the
  example. Pre-1.0 it should move into a ThemeManager-managed catalog (named
  built-in schemes such as fluent/material/creamy) so applications select one
  coherent platform tone by name and override it per brand, instead of copying
  palette code out of the example.

## Build Dependency Policy

Third-party libraries maintained as repository dependencies use pinned Git submodules. Prefer an
upstream CMake project when one is available, then let Meson consume that CMake target through its
CMake subproject adapter. This keeps one vendored source tree usable by the current Meson build and
a future native CMake package instead of coupling consumers to a Meson wrap layout.

`tomlplusplus` is pinned to upstream `v3.4.0` under `subprojects/tomlplusplus`. The build uses its
header-only `tomlplusplus::tomlplusplus` target directly and no longer probes a system package or
downloads a WrapDB archive. A fresh checkout therefore requires `git submodule update --init
--recursive`, consistently with the project's other embedded dependencies.

## Phase Completion Criteria

- Settings has crisp SDF corners, outlines, lines, focus rings, and control indicators in both modes.
- Common text sizes and supported scale factors follow a documented, tested clarity policy.
- Default light/dark semantic palettes derive from one reference scale source.
- Button state feedback no longer rewrites the base fill, and ripple can be disabled independently.
- At least one new component completes the frozen acceptance template.
- Retaining a page root cannot expose callbacks to destroyed page reactive state.
- Every unit passes `meson compile -C buildDir` and `meson test -C buildDir` before review.
