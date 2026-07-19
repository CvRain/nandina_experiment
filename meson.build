project(
    'nandina_experiment',
    'cpp',
    default_options: ['default_library=static', 'cpp_std=c++26'],
)

fs = import('fs')
python = import('python').find_installation('python3')

embed_binary = executable(
    'nandina_embed_binary',
    'tools/embed_binary.cpp',
    native: true,
)

# 使用 cmake subproject 集成第三方库
cmake = import('cmake')

# 配置 SDL 的 CMake 选项
sdl_options = cmake.subproject_options()
sdl_options.add_cmake_defines({'SDL_TESTS': 'OFF', 'SDL_STATIC': 'ON', 'SDL_SHARED': 'OFF'})

# 配置 spdlog 的 CMake选项
spdlog_options = cmake.subproject_options()
spdlog_options.add_cmake_defines({})

# 配置 raylib 的CMake选项
raylib_options = cmake.subproject_options()
raylib_options.add_cmake_defines({})

# 配置 utf8proc 的 CMake 选项
utf8proc_options = cmake.subproject_options()
utf8proc_options.add_cmake_defines({
    'BUILD_SHARED_LIBS': 'OFF',
    'UTF8PROC_INSTALL': 'OFF',
    'UTF8PROC_ENABLE_TESTING': 'OFF',
})

# 配置 subproject
sdl_subproj = cmake.subproject('SDL', options: sdl_options)
spdlog_subproj = cmake.subproject('spdlog', options: spdlog_options)
raylib_subproj = cmake.subproject('raylib', options: raylib_options)
utf8proc_subproj = cmake.subproject('utf8proc', options: utf8proc_options)
freetype_subproj = subproject(
    'freetype',
    default_options: ['tests=disabled', 'harfbuzz=disabled'],
)
harfbuzz_subproj = subproject(
    'harfbuzz',
    default_options: [
        'tests=disabled',
        'docs=disabled',
        'utilities=disabled',
        'introspection=disabled',
        'glib=disabled',
        'gobject=disabled',
        'cairo=disabled',
        'chafa=disabled',
        'png=disabled',
        'icu=disabled',
        'graphite2=disabled',
        'raster=disabled',
        'vector=disabled',
        'gpu=disabled',
        'subset=disabled',
        'freetype=enabled',
    ],
)
fribidi_subproj = subproject(
    'fribidi',
    default_options: [
        'default_library=static',
        'deprecated=false',
        'docs=false',
        'bin=false',
        'tests=false',
    ],
)

# 获取依赖（静态库）
sdl_dep = sdl_subproj.dependency('SDL3-static')
spdlog_dep = spdlog_subproj.dependency('spdlog')
raylib_dep = raylib_subproj.dependency('raylib')
utf8proc_dep = utf8proc_subproj.dependency('utf8proc')
freetype_dep = freetype_subproj.get_variable('freetype_dep')
harfbuzz_dep = harfbuzz_subproj.get_variable('libharfbuzz_dep')
fribidi_dep = fribidi_subproj.get_variable('libfribidi_dep')
physics2d_option = get_option('physics2d')
box2d_dep = disabler()
if not physics2d_option.disabled()
    box2d_options = cmake.subproject_options()
    box2d_options.add_cmake_defines({
        'BUILD_SHARED_LIBS': 'OFF',
        'BOX2D_SAMPLES': 'OFF',
        'BOX2D_BENCHMARKS': 'OFF',
        'BOX2D_DOCS': 'OFF',
        'BOX2D_UNIT_TESTS': 'OFF',
    })
    box2d_options.set_install(false)
    box2d_subproj = cmake.subproject(
        'box2d',
        options: box2d_options,
        required: physics2d_option,
    )
    if box2d_subproj.found()
        box2d_dep = box2d_subproj.dependency('box2d')
    endif
endif
sqlite3_dep = dependency(
    'sqlite3',
    version: '>=3.37',
    fallback: ['sqlite3', 'sqlite3_dep'],
    default_options: ['default_library=static'],
)
tomlplusplus_dep = dependency(
    'tomlplusplus',
    version: '>=3.4.0',
    fallback: ['tomlplusplus', 'tomlplusplus_dep'],
)
openssl_dep = dependency('openssl', version: '>=3.0')

# Catch2 是原生 meson 项目，直接作为 subproject 引入，用于单元测试。
# catch2_with_main 自带 Catch2 默认 main，测试文件无需自己写 main。
catch2_subproj = subproject('Catch2', default_options: ['install=false', 'tests=false'])
catch2_with_main_dep = catch2_subproj.get_variable('catch2_with_main_dep')

# 引入子项目
subdir('nandina')
nanres = executable(
    'nanres',
    ['tools/nanres_main.cpp', 'tools/resource_manifest.cpp', 'tools/resource_packager.cpp'],
    dependencies: [nandina_dep, tomlplusplus_dep, openssl_dep],
    install: true,
)
nanres_build_helper = [python, files('tools/nanres_build_helper.py')]
nanres_install_helper = [python, files('tools/nanres_install.py')]
subdir('example')
subdir('tests')
