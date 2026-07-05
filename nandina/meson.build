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

module_reactive = files(
    'reactive/graph.hpp',
    'reactive/graph.cpp',
    'reactive/signal.hpp',
    'reactive/computed.hpp',
    'reactive/effect.hpp',
    'reactive/batch.hpp',
    'reactive/reactive.hpp',
)

module_scene = files(
    'scene/input_event.hpp',
    'scene/node.hpp',
    'scene/node.cpp',
    'scene/node2d.hpp',
    'scene/node2d.cpp',
    'scene/scene_tree.hpp',
    'scene/scene_tree.cpp',
    'scene/control.hpp',
    'scene/control.cpp',
)

module_render = files(
    'render/render_device.hpp',
    'render/clip_stack.hpp',
    'render/draw_context.hpp',
    'render/backends/raylib_device.hpp',
    'render/backends/raylib_device.cpp',
)

module_widget = files(
    'widget/reactive_control.hpp',
    'widget/bindable_rect.hpp',
)

module_app = files(
    'app/window_config.hpp',
    'app/nan_application.hpp',
    'app/nan_application.cpp',
    'app/nan_window.hpp',
    'app/nan_window.cpp',
)

# 创建静态库
nandina_lib = static_library(
    'nandina',
    [module_fundation, module_reactive, module_scene, module_render, module_widget, module_app],
    dependencies : [sdl_dep, spdlog_dep, raylib_dep],
    install : true,
)

# 创建依赖对象，供 example 使用
nandina_dep = declare_dependency(
    link_with : nandina_lib,
    include_directories : include_directories('.'),
    dependencies : [sdl_dep, spdlog_dep, raylib_dep],
)
