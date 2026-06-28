project(
    'nandina_experiment',
    'cpp',
    default_options: ['default_library=static', 'cpp_std=c++26'],
)

# 使用 cmake subproject 集成第三方库
cmake = import('cmake')

# 配置 SDL 的 CMake 选项
sdl_options = cmake.subproject_options()
sdl_options.add_cmake_defines({'SDL_TESTS': 'OFF', 'SDL_STATIC': 'ON', 'SDL_SHARED': 'OFF'})

# 配置 spdlog 的 CMake选项
spdlog_options = cmake.subproject_options()
spdlog_options.add_cmake_defines({})

# 配置 raylib 的CMake选项
raylib_options = cmake.subproject_options()
raylib_options.add_cmake_defines({})

# 配置 subproject
sdl_subproj = cmake.subproject('SDL', options: sdl_options)
spdlog_subproj = cmake.subproject('spdlog', options: spdlog_options)
raylib_subproj = cmake.subproject('raylib', options: raylib_options)

# 获取依赖（静态库）
sdl_dep = sdl_subproj.dependency('SDL3-static')
spdlog_dep = spdlog_subproj.dependency('spdlog')
raylib_dep = raylib_subproj.dependency('raylib')

# 引入子项目
subdir('nandina')
subdir('example')
