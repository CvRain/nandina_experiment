# Phase 8: Render Quality And Theme Evolution

Phase 6 and Phase 7 established recipe-driven component styles, atomic DesignSystem snapshots,
light/dark appearance switching, shared painters, and the Switch end-to-end component path.
Phase 8 pauses component expansion long enough to stabilize rendering quality and state-layer
semantics, then uses new components to verify the resulting authoring template.

Work continues in one-reviewable-unit increments. Every unit must compile and pass the complete
test suite before review, and theme, widget, render, app, and example changes remain separate
whenever their dependency order allows it.

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
risk is an externally retained page root invoking callbacks after its Page Frame has cleared page
signals. The architectural fix is a shared lifetime anchor for root and scope, or uniform callback
cleanup on unmount, with a regression test that retains a root across page pop.

## Delivery Sequence

### Step 0: SDF Coverage Repair

Status: implemented; awaiting visual review in the Settings example.

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

Status: implemented for the current 1x logical-pixel window path; awaiting visual review. The
Settings example already covers 16px body/control text, 18px section labels, and a 28px heading in
both appearances. A later DPI unit must introduce an explicit logical-to-physical scale and cache
glyphs at the corresponding physical size before claiming 1.25x/1.5x/2x support.

Suggested commit: `fix(text): 统一字形像素对齐与缩放策略`

### Step 2: Generate Semantic Palettes From Reference Scales

Land the eleven-stop reference data, variant policy, generator, consistency tests, and switch
`default_design_system()` to the generated light/dark palettes in one theme-only unit.

Suggested commit: `feat(theme): 从参考色阶生成默认语义调色板`

### Step 3: Paint A Real Static State Layer

Migrate Button from replacement fill semantics to an overlay and verify base/overlay/outline/focus
draw order with a recording render device.

Suggested commit: `refactor(theme): 将按钮状态色改为独立叠加层`

### Step 4: Ripple And Reduced Motion

Add animation state and scheduling separately from recipe colors. If this crosses render and widget
boundaries, split commits in compilable dependency order.

### Step 5: Normalize The Brand Theme Example

Demonstrate paired primary/on-primary colors for both appearances, contrast constraints, and an
optional tone policy instead of presenting a radius-only customization as the brand example.

### Step 6: Freeze The Component Template

Implement one component at a time and require recipe/rules, shared painters, light/dark and override
tests, constrained layout, pointer/keyboard/focus behavior, semantics, authoring, and any necessary
two-way binding. The example remains a real application, not a component gallery.

### Step 7: Close Page Root/Scope Lifetime

Keep the existing page-owned ReactiveScope. Address only the retained-root callback hazard and add
teardown tests; do not introduce a second reactive lifetime system.

## Deferred Work

- `styles.toml` font-family integration waits for the resource manager path to mature.
- RGB subpixel text waits for a display-scale and compositor policy.
- Vulkan/SDL platform work remains outside the pre-2.0 plan.
- Large component expansion waits for Steps 0 through 3.

## Phase Completion Criteria

- Settings has crisp SDF corners, outlines, lines, focus rings, and control indicators in both modes.
- Common text sizes and supported scale factors follow a documented, tested clarity policy.
- Default light/dark semantic palettes derive from one reference scale source.
- Button state feedback no longer rewrites the base fill, and ripple can be disabled independently.
- At least one new component completes the frozen acceptance template.
- Retaining a page root cannot expose callbacks to destroyed page reactive state.
- Every unit passes `meson compile -C buildDir` and `meson test -C buildDir` before review.
