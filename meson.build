project('nandina_experiment', 'cpp',
        default_options : ['default_library=static',
                           'cpp_std=c++26'
        ]
)

# 根据编译器类型确定子目录名
compiler_id = meson.get_compiler('cpp').get_id()
if compiler_id == 'gcc'
  compiler_dir = 'gcc'
elif compiler_id == 'clang'
  compiler_dir = 'clang'
elif compiler_id == 'msvc'
  compiler_dir = 'msvc'
elif compiler_id == 'clang-cl'
  compiler_dir = 'clang-cl'
else
  compiler_dir = compiler_id
endif

# 根据构建类型确定子目录
buildtype = get_option('buildtype')
if buildtype == 'debug'
  buildtype_dir = 'debug'
elif buildtype == 'release'
  buildtype_dir = 'release'
elif buildtype == 'debugoptimized'
  buildtype_dir = 'debugoptimized'
elif buildtype == 'minsize'
  buildtype_dir = 'minsize'
else
  buildtype_dir = buildtype
endif

# 根据操作系统确定子目录
host_system = host_machine.system()
if host_system == 'windows'
  os_dir = 'windows'
elif host_system == 'linux'
  os_dir = 'linux'
elif host_system == 'darwin'
  os_dir = 'macos'
else
  os_dir = host_system
endif

# 构建输出目录: builddir/[os]-[compiler]-[buildtype]/
base_build_dir = join_paths('builddir', os_dir + '-' + compiler_dir + '-' + buildtype_dir)

# 设置构建目标的名称前缀和安装目录
message('Build output directory: ' + base_build_dir)

# 使用 cmake subproject 集成 SDL
cmake = import('cmake')

# 配置 SDL 的 CMake 选项
sdl_options = cmake.subproject_options()
sdl_options.add_cmake_defines({ 'SDL_TESTS' : 'OFF', 'SDL_STATIC' : 'ON', 'SDL_SHARED' : 'OFF' })

# 配置 spdlog 的 CMake选项
spdlog_options = cmake.subproject_options()
spdlog_options.add_cmake_defines({})

# 配置 subproject
sdl_subproj = cmake.subproject('SDL', options : sdl_options)
spdlog_subproj = cmake.subproject('spdlog', options : spdlog_options)

# 获取依赖（静态库）
sdl_dep = sdl_subproj.dependency('SDL3-static')
spdlog_dep = spdlog_subproj.dependency('spdlog')

# 引入子项目
subdir('nandina')
subdir('example')

