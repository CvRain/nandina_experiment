# nandina 库

# 收集所有源文件
nandina_sources = files('nandina_greet.cpp')

# 收集所有头文件
nandina_headers = files('nandina_greet.hpp')

# 创建静态库
nandina_lib = static_library(
    'nandina',
    [nandina_sources, nandina_headers],
    dependencies: [sdl_dep, spdlog_dep, raylib_dep],
    install: true,
)

# 创建依赖对象，供 example 使用
nandina_dep = declare_dependency(
    link_with: nandina_lib,
    include_directories: include_directories('.'),
    dependencies: [sdl_dep, spdlog_dep, raylib_dep],
)
