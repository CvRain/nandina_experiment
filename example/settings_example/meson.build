# Settings 参考应用子项目：组件库 + 独立可执行文件。

settings_example_library = static_library(
    'nandina_settings_example_components',
    files('settings_example.cpp', 'settings_example.hpp', 'gallery_page.cpp', 'gallery_page.hpp'),
    dependencies: [nandina_dep],
)
settings_example_dep = declare_dependency(
    link_with: settings_example_library,
    include_directories: include_directories('.'),
    dependencies: [nandina_dep],
)

# 品牌图进入 nanres 包；build helper 同时写入可执行文件旁的 resource-location.json。
settings_resources = custom_target(
    'nandina-settings-resources',
    input: files('resources/resources.toml'),
    output: 'resources',
    command: [nanres_build_helper, 'build', nanres, '@INPUT@', '@OUTPUT@'],
    build_by_default: true,
    build_always_stale: true,
)
# 自定义字体仍保留文件路径加载；字体包迁移不属于 C3。
configure_file(
    input: 'assets/demo-font.ttf',
    output: 'demo-font.ttf',
    copy: true,
)

executable(
    'nandina_settings_example',
    files('settings_main.cpp'),
    dependencies: [settings_example_dep, raylib_dep],
    install: true,
)
