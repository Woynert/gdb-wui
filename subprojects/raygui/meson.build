project('raygui', 'c')

# Invoke raylib using cmake, might wanna wrap raylib into meson.
cmake = import('cmake')
raylib_opts = cmake.subproject_options()
raylib_opts.add_cmake_defines({'PLATFORM': 'Desktop'})
raylib_opts.add_cmake_defines({'BUILD_EXAMPLES': false})
raylib_proj = cmake.subproject('raylib', options: raylib_opts)
raylib_dep = raylib_proj.dependency('raylib')

raygui_includes = include_directories('./raygui/src')
raygui_lib = library('raygui',
  './raygui_implementation.c',
  dependencies: [ raylib_dep ],
  include_directories: raygui_includes,
  install: true
)
raygui_dep = declare_dependency(
  include_directories: raygui_includes,
  link_with: raygui_lib
)
