# Settings 参考应用子项目：组件库 + 独立可执行文件。

settings_example_library = static_library(
    'nandina_settings_example_components',
    files('settings_example.cpp', 'settings_example.hpp'),
    dependencies: [nandina_dep],
)
settings_example_dep = declare_dependency(
    link_with: settings_example_library,
    include_directories: include_directories('.'),
    dependencies: [nandina_dep],
)

# 把示例图片与自定义字体复制到可执行文件同目录，供运行时按相对路径加载
# （nanres 集成延后；图片/字体统一使用输出目录相对路径）。
configure_file(
    input: 'assets/nandina_logo.png',
    output: 'nandina_logo.png',
    copy: true,
)
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
