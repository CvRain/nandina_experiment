# Recommended application example and optional diagnostics.

compact_todo_example_library = static_library(
    'nandina_compact_todo_components',
    files('compact_todo.cpp', 'compact_todo.hpp'),
    dependencies : [nandina_dep],
)
compact_todo_example_dep = declare_dependency(
    link_with : compact_todo_example_library,
    include_directories : include_directories('.'),
    dependencies : [nandina_dep],
)

nanres_policy = files('resources.toml')
resource_nanres = nandina_resource_toolchain['nanres']
resource_build_helper = nandina_resource_toolchain['build_helper']
resource_install_helper = nandina_resource_toolchain['install_helper']
nanres_package = custom_target(
    'nandina-example-resources',
    input: nanres_policy,
    output: 'resources',
    command: [
        resource_build_helper,
        'build',
        resource_nanres,
        nanres_policy,
        '@OUTPUT@',
    ],
    build_by_default: true,
    build_always_stale: true,
)
meson.add_install_script(
    resource_install_helper,
    nanres_package,
    nanres_policy,
    get_option('datadir'),
)

executable(
    'nandina_example',
    files('compact_main.cpp'),
    dependencies : [compact_todo_example_dep, raylib_dep],
    install : true,
)

if box2d_dep.found()
    executable(
        'nandina_physics_example',
        files('physics_main.cpp'),
        dependencies : [nandina_dep, raylib_dep],
        install : false,
    )
endif
