bundled_font_targets = []

if get_option('bundled_fonts').enabled()
    caskaydia_regular_source = (
        meson.project_source_root()
        / 'subprojects'
        / 'caskaydia-cove'
        / 'fonts'
        / 'ttf'
        / 'CaskaydiaCove-Regular.ttf'
    )
    caskaydia_license_source = (
        meson.project_source_root()
        / 'subprojects'
        / 'caskaydia-cove'
        / 'OFL.txt'
    )

    cmake_copy = find_program('cmake')
    caskaydia_font_target = custom_target(
        'copy-caskaydia-regular',
        output: 'CaskaydiaCove-Regular.ttf',
        command: [cmake_copy, '-E', 'copy_if_different', caskaydia_regular_source, '@OUTPUT@'],
        build_by_default: true,
    )
    caskaydia_license_target = custom_target(
        'copy-caskaydia-license',
        output: 'OFL-CaskaydiaCove.txt',
        command: [cmake_copy, '-E', 'copy_if_different', caskaydia_license_source, '@OUTPUT@'],
        build_by_default: true,
    )
    bundled_font_targets += [caskaydia_font_target, caskaydia_license_target]
endif
