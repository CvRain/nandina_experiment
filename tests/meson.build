# 单元测试 (Catch2 v3, 无窗口)

foundation_tests = executable(
    'foundation_tests',
    files('foundation_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('foundation', foundation_tests)

font_test_path = meson.project_source_root() / 'subprojects' / 'harfbuzz' / 'test' / 'api' / 'fonts' / 'Inconsolata-Regular.ab.ttf'
rtl_font_test_path = meson.project_source_root() / 'subprojects' / 'harfbuzz' / 'test' / 'shape' / 'data' / 'text-rendering-tests' / 'fonts' / 'TestShapeAran.ttf'
arabic_mark_font_test_path = meson.project_source_root() / 'subprojects' / 'harfbuzz' / 'test' / 'shape' / 'data' / 'in-house' / 'fonts' / '65984dfce552a785f564422aadf4715fa07795ad.ttf'
font_tests = executable(
    'font_tests',
    files('font_tests.cpp'),
    cpp_args : [
        '-DNANDINA_TEST_FONT_PATH="' + font_test_path + '"',
        '-DNANDINA_RTL_TEST_FONT_PATH="' + rtl_font_test_path + '"',
        '-DNANDINA_ARABIC_MARK_TEST_FONT_PATH="' + arabic_mark_font_test_path + '"',
    ],
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('font', font_tests)

scene_tests = executable(
    'scene_tests',
    files('scene_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('scene', scene_tests)

if box2d_dep.found()
    physics2d_tests = executable(
        'physics2d_tests',
        files('physics2d_tests.cpp'),
        dependencies : [nandina_dep, catch2_with_main_dep],
    )
    test('physics2d', physics2d_tests)
endif

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

authoring_tests = executable(
    'authoring_tests',
    files('authoring_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('authoring', authoring_tests)

settings_example_tests = executable(
    'settings_example_tests',
    files('settings_example_tests.cpp'),
    dependencies : [settings_example_dep, catch2_with_main_dep],
)
test('settings-example', settings_example_tests)

test(
    'example-source-budget',
    python,
    args: [files('example_source_budget_test.py'), meson.project_source_root()],
)

router_tests = executable(
    'router_tests',
    files('router_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('router', router_tests)

async_tests = executable(
    'async_tests',
    files('async_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('async', async_tests)

viewport_scaling_tests = executable(
    'viewport_scaling_tests',
    files('viewport_scaling_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('viewport-scaling', viewport_scaling_tests)

button_tests = executable(
    'button_tests',
    files('button_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('button', button_tests)

theme_design_system_tests = executable(
    'theme_design_system_tests',
    files('theme_design_system_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('theme-design-system', theme_design_system_tests)

checkbox_tests = executable(
    'checkbox_tests',
    files('checkbox_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('checkbox', checkbox_tests)

slider_tests = executable(
    'slider_tests',
    files('slider_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('slider', slider_tests)

switch_tests = executable(
    'switch_tests',
    files('switch_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('switch', switch_tests)

resource_tests = executable(
    'resource_tests',
    files('resource_tests.cpp'),
    dependencies : [nandina_dep, sqlite3_dep, catch2_with_main_dep],
)
test('resource', resource_tests)

nanres_cli_test = find_program('nanres_cli_test.py')
nanres_package_probe = executable(
    'nanres_package_probe',
    files('nanres_package_probe.cpp'),
    dependencies: [nandina_dep, sqlite3_dep],
)
test(
    'nanres-cli',
    nanres_cli_test,
    args: [nanres, nanres_package_probe],
)
nanres_install_test = find_program('nanres_install_test.py')
test(
    'nanres-install',
    nanres_install_test,
    args: [meson.project_source_root() / 'tools' / 'nanres_install.py'],
)
nanres_build_workflow_test = find_program('nanres_build_workflow_test.py')
test(
    'nanres-build-workflow',
    nanres_build_workflow_test,
    args: [
        meson.project_source_root() / 'tools' / 'nanres_build_helper.py',
        nanres,
    ],
)
nandina_subproject_fixture_test = find_program('nandina_subproject_fixture_test.py')
test(
    'nandina-subproject-fixture',
    nandina_subproject_fixture_test,
    args: [meson.project_source_root()],
    timeout: 300,
)
font_resource_tests = executable(
    'font_resource_tests',
    files('font_resource_tests.cpp'),
    cpp_args : ['-DNANDINA_TEST_FONT_PATH="' + font_test_path + '"'],
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('font-resource', font_resource_tests)

application_resource_tests = executable(
    'application_resource_tests',
    files('application_resource_tests.cpp'),
    dependencies: [nandina_dep, catch2_with_main_dep],
)
test('application-resource', application_resource_tests)
