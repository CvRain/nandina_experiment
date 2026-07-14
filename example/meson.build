# example 可执行文件

example_sources = files(
    'main.cpp',
)

subdir('res/fonts')

example_cpp_args = []
if get_option('bundled_fonts').enabled()
    example_cpp_args += ['-DNANDINA_EXAMPLE_RESOURCE_DIR="' + bundled_font_dir + '"']
endif

executable('nandina_example',
    example_sources,
    cpp_args : example_cpp_args,
    dependencies : [nandina_dep,raylib_dep],
    install : true
)
