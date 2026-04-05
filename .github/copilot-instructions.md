# Nandina Workspace Guidelines

## Build And Validation

- Prefer the vcpkg presets for local development and verification: `cmake --preset debug-vcpkg` then `cmake --build --preset debug-vcpkg`.
- Use `release-vcpkg` or `ci` only when the task needs release-style validation.
- Run the app from `build/<preset>/bin/NandinaExperiment` when a UI change needs runtime verification.
- There is currently no CTest or unit-test target in this repository. Validate changes by building the relevant preset, and run the app for behavior changes.
- Do not add `-fmodules-ts` manually. This project relies on CMake 3.28+ and Ninja to inject module flags automatically.

## Architecture

- This project is built around C++26 module interface units (`.ixx`), not headers. Keep module names, file locations, and exports aligned.
- `src/main.cpp` is the executable entrypoint. Application wiring lives in `src/application_window.ixx`.
- `src/core/Core.ixx` is the umbrella module for widget tree, events, routing, components, and buttons. `src/layout/Layout.ixx` and `src/components/Label.ixx` build on top of core and reactive primitives.
- Reactive state is a first-class pattern here. Prefer `State`, `Effect`, and `EffectScope` for UI updates instead of ad hoc synchronization.
- If you add or reorder modules, update `MODULE_SOURCES` in `CMakeLists.txt` carefully. Dependency order matters for module scanning.

## Conventions

- Follow the existing fluent builder style used in `src/application_window.ixx`, `src/core/Button.ixx`, and layout/container APIs.
- Prefer composition over deep inheritance for UI behavior. Reuse layers, widgets, and reactive bindings before introducing new class hierarchies.
- Keep naming consistent with the codebase: classes and modules in PascalCase, methods in snake_case, factories as `Create()`, and ownership via `std::unique_ptr`.
- Preserve the current optional text-engine behavior. `FreeType` and `harfbuzz` are enabled only when found at configure time.

## Reference Docs

- See `README.md` for quick-start commands and active milestone status.
- See `docs/design.md` for system architecture and render/data-flow details.
- See `docs/reactive-design.md` for reactive internals and usage patterns.
- See `docs/development-plan.md` for roadmap context before implementing milestone-driven features.