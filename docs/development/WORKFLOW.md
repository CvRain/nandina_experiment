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

- Phase 8 Step 6 committed (`000ae6c` / `930fa44` / `371699f`): contrast helpers
  and the Catppuccin Settings brand theme (Latte light / Mocha dark, Rosewater
  light primary, Peach dark primary, 12/16/24 radii, 2–3px borders).
- Phase 8 Step 7 complete: the component template is frozen here in §4. Badge
  and Card are the first two components completed against it (recipe/rule
  resolution, shared painters, override survival, constrained layout, static
  semantics, real use in Settings).
- Test suite 29/29 green.
- Next: continue the D6 order with ProgressBar, then RadioButton. Planned
  follow-ups: built-in ThemeManager theme families (fluent/material/creamy)
  and soft shadows for claymorphism elevation.
