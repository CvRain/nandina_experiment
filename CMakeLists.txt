cmake_minimum_required(VERSION 3.28)

project(NandinaExperiment
        VERSION 0.3.0
        LANGUAGES CXX
)

enable_testing()

# ── C++26, modules enabled ──────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)           # CMake scans & orders modules
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── GCC module flags ─────────────────────────────────────────────────────────
# CMake 3.28+ with Ninja injects -fmodules-ts automatically for module TUs.
# Do NOT add it manually — doing so causes duplicate flag + STL redefinition errors.

# ── Dependencies (pkg-config) ─────────────────────────────────────────────────
find_package(PkgConfig REQUIRED)
pkg_check_modules(THORVG REQUIRED IMPORTED_TARGET thorvg-1)
pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)

# ── Module interface sources ──────────────────────────────────────────────────
set(MODULE_SOURCES
        # ── Reactive primitives (single flat module, no sub-module diamonds) ──
        # Contains: Connection / ScopedConnection / EventSignal<>
        #           State<T> / ReadState / Computed / Effect / EffectScope
        #           Prop<T> / StateList<T> / ListChange / ListChangeKind
        src/reactive/Reactive.ixx

        # ── Geometry types (must precede Widget) ──────────────────────────────
        src/types/Position.ixx
        src/types/Size.ixx
        src/types/Rect.ixx
        src/types/Types.ixx

        # ── Core widget sub-modules (dependency order: leaves first) ──────────
        src/core/Color.ixx
        src/core/Event.ixx
        src/core/Widget.ixx           # depends on Color, Event, Types; no Reactive dep
        src/core/FreeWidget.ixx       # depends on Widget
        src/core/RenderLayer.ixx      # depends on Widget + Event
        src/core/Component.ixx        # depends on Widget + Reactive
        src/core/CompositeComponent.ixx
        src/core/Router.ixx
        src/core/Button.ixx
        src/core/Core.ixx             # umbrella: Reactive + all core sub-modules

        src/core/Window.ixx
        src/core/NanApplication.ixx

        src/components/Label.ixx

        src/layout/FlexLayout.ixx
        src/layout/LayoutBackend.ixx
        src/layout/YogaMapping.ixx
        src/layout/YogaAdapter.ixx
        src/layout/Layout.ixx

        src/style/Theme.ixx

        example/pages/DemoPage.ixx
        example/pages/CounterPage.ixx
        example/application_window.ixx
)

set(TEST_MODULE_SOURCES
        src/reactive/Reactive.ixx
        src/types/Position.ixx
        src/types/Size.ixx
        src/types/Rect.ixx
        src/types/Types.ixx
        src/core/Color.ixx
        src/core/Event.ixx
        src/core/Widget.ixx
        src/core/FreeWidget.ixx
        src/core/RenderLayer.ixx
        src/core/Component.ixx
        src/core/CompositeComponent.ixx
        src/core/Router.ixx
        src/core/Button.ixx
        src/core/Core.ixx
        src/components/Label.ixx
        src/layout/LayoutBackend.ixx
        src/layout/YogaMapping.ixx
        src/layout/YogaAdapter.ixx
)

# ── Executable ────────────────────────────────────────────────────────────────
add_executable(NandinaExperiment example/main.cpp)

target_link_libraries(NandinaExperiment PRIVATE
        PkgConfig::THORVG
        PkgConfig::SDL3
)


target_sources(NandinaExperiment
        PRIVATE FILE_SET CXX_MODULES FILES ${MODULE_SOURCES})

target_compile_options(NandinaExperiment PRIVATE
        -Wall -Wextra -Wpedantic
)

# ── Optional: FreeType + HarfBuzz text engine ─────────────────────────────────
find_package(Freetype QUIET)
find_package(harfbuzz QUIET)

if(Freetype_FOUND AND harfbuzz_FOUND)
    message(STATUS "Text engine: FreeType ${FREETYPE_VERSION_STRING} + HarfBuzz found — enabling NANDINA_TEXT_ENGINE")
    target_link_libraries(NandinaExperiment PRIVATE Freetype::Freetype harfbuzz::harfbuzz)
    target_compile_definitions(NandinaExperiment PRIVATE NANDINA_TEXT_ENGINE=1)
else()
    if(NOT Freetype_FOUND)
        message(STATUS "Text engine: FreeType not found — text rendering disabled")
    endif()
    if(NOT harfbuzz_FOUND)
        message(STATUS "Text engine: HarfBuzz not found — text rendering disabled")
    endif()
endif()


target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_26)

set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 26
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_executable(NandinaReactiveTests tests/reactive_smoke.cpp)

target_sources(NandinaReactiveTests
        PRIVATE tests/reactive_smoke.cpp)

target_sources(NandinaReactiveTests
        PRIVATE FILE_SET CXX_MODULES FILES
        ${TEST_MODULE_SOURCES})

target_compile_options(NandinaReactiveTests PRIVATE
        -Wall -Wextra -Wpedantic
)

target_compile_features(NandinaReactiveTests PRIVATE cxx_std_26)

set_target_properties(NandinaReactiveTests PROPERTIES
        CXX_STANDARD 26
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_test(NAME NandinaReactiveSmoke COMMAND ${CMAKE_BINARY_DIR}/bin/NandinaReactiveTests)

add_executable(NandinaLayoutMappingTests tests/layout_mapping_smoke.cpp)

target_sources(NandinaLayoutMappingTests
        PRIVATE tests/layout_mapping_smoke.cpp)

target_sources(NandinaLayoutMappingTests
        PRIVATE FILE_SET CXX_MODULES FILES
        ${TEST_MODULE_SOURCES})

target_compile_options(NandinaLayoutMappingTests PRIVATE
        -Wall -Wextra -Wpedantic
)

target_compile_features(NandinaLayoutMappingTests PRIVATE cxx_std_26)

set_target_properties(NandinaLayoutMappingTests PROPERTIES
        CXX_STANDARD 26
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_test(NAME NandinaLayoutMappingSmoke COMMAND ${CMAKE_BINARY_DIR}/bin/NandinaLayoutMappingTests)
