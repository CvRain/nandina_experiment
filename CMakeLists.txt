cmake_minimum_required(VERSION 3.28)

project(NandinaExperiment
        VERSION 1.0.0
        LANGUAGES CXX
)

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
        src/core/Core.ixx
        src/core/Event.ixx
        src/core/Signal.ixx
        src/core/Widget.ixx
        src/core/Window.ixx

        src/style/Theme.ixx

        src/types/Position.ixx
        src/types/Rect.ixx
        src/types/Size.ixx

        src/NanApplication.ixx
)

# ── Executable ────────────────────────────────────────────────────────────────
add_executable(NandinaExperiment src/main.cpp)

target_link_libraries(NandinaExperiment PRIVATE
        PkgConfig::THORVG
        PkgConfig::SDL3
)


target_sources(NandinaExperiment
        PRIVATE FILE_SET CXX_MODULES FILES ${MODULE_SOURCES})

target_compile_options(NandinaExperiment PRIVATE
        -Wall -Wextra -Wpedantic
)


target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_26)

set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 26
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
