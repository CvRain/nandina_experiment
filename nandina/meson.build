# nandina 库

module_fundation = files(
    'foundation/nandina_color.hpp',
    'foundation/nandina_color.cpp',
    'foundation/nandina_color_space.hpp',
    'foundation/nandina_color_space.cpp',
    'foundation/geometry.hpp',
    'foundation/geometry.cpp',
    'foundation/transform2d.hpp',
    'foundation/transform2d.cpp',
)

module_scene = files(
    'scene/input_event.hpp',
    'scene/node.hpp',
    'scene/node.cpp',
    'scene/node2d.hpp',
    'scene/node2d.cpp',
    'scene/scene_tree.hpp',
    'scene/scene_tree.cpp',
)

module_render = files(
    'render/render_device.hpp',
    'render/clip_stack.hpp',
    'render/draw_context.hpp',
    'render/backends/raylib_device.hpp',
    'render/backends/raylib_device.cpp',
)

# 创建静态库
nandina_lib = static_library(
    'nandina',
    [module_fundation, module_scene, module_render],
    dependencies : [sdl_dep, spdlog_dep, raylib_dep],
    install : true,
)

# 创建依赖对象，供 example 使用
nandina_dep = declare_dependency(
    link_with : nandina_lib,
    include_directories : include_directories('.'),
    dependencies : [sdl_dep, spdlog_dep, raylib_dep],
)
