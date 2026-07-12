# 单元测试 (Catch2 v3, 无窗口)

foundation_tests = executable(
    'foundation_tests',
    files('foundation_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('foundation', foundation_tests)

font_test_path = meson.project_source_root() / 'subprojects' / 'harfbuzz' / 'test' / 'api' / 'fonts' / 'Inconsolata-Regular.ab.ttf'
font_tests = executable(
    'font_tests',
    files('font_tests.cpp'),
    cpp_args : ['-DNANDINA_TEST_FONT_PATH="' + font_test_path + '"'],
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('font', font_tests)

if get_option('bundled_fonts').enabled()
    bundled_font_path = (
        meson.project_build_root()
        / 'example'
        / 'res'
        / 'fonts'
        / 'CaskaydiaCove-Regular.ttf'
    )
    bundled_font_tests = executable(
        'bundled_font_tests',
        files('bundled_font_tests.cpp'),
        cpp_args : ['-DNANDINA_BUNDLED_FONT_PATH="' + bundled_font_path + '"'],
        dependencies : [nandina_dep, catch2_with_main_dep],
    )
    test('bundled-font', bundled_font_tests, depends: bundled_font_targets)
endif

scene_tests = executable(
    'scene_tests',
    files('scene_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('scene', scene_tests)

render_tests = executable(
    'render_tests',
    files('render_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('render', render_tests)

reactive_tests = executable(
    'reactive_tests',
    files('reactive_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('reactive', reactive_tests)

widget_tests = executable(
    'widget_tests',
    files('widget_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('widget', widget_tests)

router_tests = executable(
    'router_tests',
    files('router_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('router', router_tests)

button_tests = executable(
    'button_tests',
    files('button_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('button', button_tests)
