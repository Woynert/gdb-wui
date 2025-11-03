project('raylib-game', 'c', 'cpp')

extra_args = ['-O']
my_extra_args = [
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
  '-Wno-error=stringop-truncation',
  '-Wno-unused-function'
]

# general deps

cc = meson.get_compiler('c')
math_dep = cc.find_library('m', required : false)
raylib_dep = dependency('raylib')

raygui_proj = subproject('raygui',
  default_options: [ 'default_library=shared' ])
raygui_dep = raygui_proj.get_variable('raygui_dep')

cwalk_proj = subproject('cwalk',
  default_options: [ 'default_library=shared' ])
cwalk_dep = cwalk_proj.get_variable('cwalk_dep')

mickstr_proj = subproject('mickstr',
  default_options: [ 'default_library=shared' ])
mickstr_dep = mickstr_proj.get_variable('mickstr_dep')

x11_dep = dependency('x11')
xcb_dep = dependency('xcb')
xcb_util_dep = dependency('xcb-util')
xcb_keysyms_dep = dependency('xcb-keysyms')

# artifacts

gdbwui_exe = executable(
  'gdbwui'
  ,'src/main.c'
  ,dependencies : [
    raylib_dep,
    math_dep,
    raygui_dep
  ]
  ,c_args : my_extra_args
)

scroll_exe = executable(
  'scroll'
  ,'src/scroll.c'
  ,dependencies : [
    #raylib_dep,
    #math_dep,
    #raygui_dep
  ]
  ,c_args : my_extra_args
)

file_list_exe = executable(
  'file_list'
  ,'src/file_list.c'
  ,dependencies : [
    raylib_dep,
    math_dep,
    raygui_dep,
    cwalk_dep,
    mickstr_dep
  ]
  ,c_args : my_extra_args
)

#itinywm_exe = executable(
  #'itinywm'
  #,'src/lab/ittywm.c'
  #,dependencies : [
    #math_dep,
    #xcb_dep,
    #xcb_util_dep,
  #]
  ##,c_args : my_extra_args
#)

#tinywm_exe = executable(
  #'tinywm'
  #,'src/lab/tinywm.c'
  #,dependencies : [
    #math_dep,
    #x11_dep
  #]
  ##,c_args : my_extra_args
#)

#xwm_exe = executable(
  #'xwm'
  #,'src/lab/xwm/xwm.c'
  #,dependencies : [
    #math_dep,
    #xcb_dep,
    ##xcb_util_dep,
    #xcb_keysyms_dep
  #]
  ##,c_args : my_extra_args
#)

#-I/nix/store/94pix4pzamq3svx4jxjj2sgrvafn29b8-libxcb-1.17.0-dev/include

