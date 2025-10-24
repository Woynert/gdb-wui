project('raygui', 'c')

raylib_dep = dependency('raylib')

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
