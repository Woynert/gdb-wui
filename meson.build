project('raylib-game', 'c', 'cpp')

extra_args = ['-O']

# general deps

cc = meson.get_compiler('c')
math_dep = cc.find_library('m', required : false)
raylib_dep = dependency('raylib')

raygui_proj = subproject('raygui',
  default_options: [ 'default_library=shared' ])
raygui_dep = raygui_proj.get_variable('raygui_dep')

# artifacts

gdbwui_exe = executable(
  'gdbwui'
  ,'src/main.c'
  ,dependencies : [
    raylib_dep,
    math_dep,
    raygui_dep
  ]
  ,c_args : [
    '-Wall',
    '-Werror',
    '-Werror=incompatible-pointer-types',
    '-Wuninitialized',
    '-Wmaybe-uninitialized',
    '-Wmissing-field-initializers',

    '-Wextra',
    '-Wpedantic',
    '-Wnull-dereference',
    # The following should be included in -Wextra, but including them anyway
    '-Wshadow',
    '-Wconversion',
    '-Wno-language-extension-token',
    '-Werror=incompatible-pointer-types',
    '-Wc++-compat',

    '-Wno-type-limits',
    '-Wno-error=float-equal',
    '-Wno-error=unused-parameter',
    '-Wno-error=unused-function',
    '-Wno-error=unused-variable',
    '-Wno-unused-function'
  ]
)
