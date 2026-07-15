# example 可执行文件

example_sources = files(
    'main.cpp',
)

nanres_resource_name = 'nandina-example-resources'
nanres_application_id = 'org.nandina.todo'
nanres_policy = files('resources.toml')
nanres_lock = files('resources.lock.toml')
nanres_resource_sources = files(
    '../nandina/assets/default_font/default-ui.ttf',
    '../nandina/assets/default_font/default-ui-license.txt',
    'res/fonts/zh-cn/sarasa-fixed-sc-regular.ttf',
    'res/fonts/zh-cn/ofl-sarasa-gothic.txt',
    'res/fonts/zh-cn/source.toml',
)
nanres_output_directory = 'resources'
nanres_validate = custom_target(
    nanres_resource_name + '-validate',
    input: [nanres_policy, nanres_lock, nanres_resource_sources],
    output: nanres_resource_name + '-validate.stamp',
    command: [
        nanres_build_helper,
        'validate',
        nanres,
        nanres_policy,
        nanres_lock,
        '@OUTPUT@',
    ],
    build_by_default: true,
)
nanres_package = custom_target(
    nanres_resource_name + '-package',
    input: [nanres_policy, nanres_lock, nanres_resource_sources],
    output: nanres_output_directory,
    command: [
        nanres_build_helper,
        'pack',
        nanres,
        nanres_policy,
        nanres_lock,
        '@OUTPUT@',
    ],
    depends: nanres_validate,
    build_by_default: true,
)
meson.add_install_script(
    nanres_install_helper,
    nanres_package,
    get_option('datadir') / nanres_application_id,
)

executable('nandina_example',
    example_sources,
    dependencies : [nandina_dep,raylib_dep],
    install : true
)
