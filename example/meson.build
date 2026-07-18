# example 可执行文件

example_sources = files(
    'main.cpp',
)

nanres_policy = files('resources.toml')
nanres_package = custom_target(
    'nandina-example-resources',
    input: nanres_policy,
    output: 'resources',
    command: [
        nanres_build_helper,
        'build',
        nanres,
        nanres_policy,
        '@OUTPUT@',
    ],
    build_by_default: true,
    build_always_stale: true,
)
meson.add_install_script(
    nanres_install_helper,
    nanres_package,
    nanres_policy,
    get_option('datadir'),
)

executable('nandina_example',
    example_sources,
    dependencies : [nandina_dep,raylib_dep],
    install : true
)

if box2d_dep.found()
    executable(
        'nandina_physics_example',
        files('physics_main.cpp'),
        dependencies : [nandina_dep, raylib_dep],
        install : false,
    )
endif
