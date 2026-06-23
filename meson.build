#project('raylib-game', 'c', 'cpp')
project('raylib-game', 'c')
  #default_options: [
    ## 'b_ndebug=false',

    ## enable ASAN on clang
    #'b_sanitize=address' ,
    #'b_lundef=false',
  #],
#)

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
  #'-Wc++-compat',

  '-Wno-type-limits',
  '-Wno-error=float-equal',
  '-Wno-error=unused-parameter',
  '-Wno-error=unused-function',
  '-Wno-error=unused-variable',
  '-Wno-error=stringop-truncation',
  '-Wno-unused-function',

  # sanitizer
  #'-fsanitize=address',
  #'-lundef=false',
]

add_project_arguments([
  '-fsanitize=address,undefined',
  '-fno-omit-frame-pointer',
  '-g',
  '-O1', # O1 supposedly better than O0 or O2.
], language:'c')
add_project_link_arguments([
  '-fsanitize=address,undefined',
], language:'c')

# strbuf

add_global_arguments('-DSTRBUF_PROVIDE_PRINTF', language: 'c')

# General dependencies

cc = meson.get_compiler('c')
math_dep = cc.find_library('m', required : false)

cwalk_proj = subproject('cwalk',
  default_options: [ 'default_library=shared' ])
cwalk_dep = cwalk_proj.get_variable('cwalk_dep')

mickstr_proj = subproject('mickstr',
  default_options: [ 'default_library=shared' ])
mickstr_dep = mickstr_proj.get_variable('mickstr_dep')

cmake = import('cmake')
raylib_opts = cmake.subproject_options()
raylib_opts.add_cmake_defines({'PLATFORM': 'Desktop'})
raylib_opts.add_cmake_defines({'BUILD_EXAMPLES': false})
raylib_proj = cmake.subproject('raylib', options: raylib_opts)
raylib_dep = raylib_proj.dependency('raylib')

raygui_proj = subproject('raygui',
  default_options: [ 'default_library=static' ])
raygui_dep = raygui_proj.get_variable('raygui_dep')

gl_dep = dependency('gl') # Only needed to shut up clang LSP.

# Artifacts

core_includes = include_directories('./src', './src/standalone')

gdbwui_exe = executable(
  'gdbwui'
  ,'src/main.c'
  ,'src/standalone/gdb_woy_api_imp.c'
  ,include_directories : core_includes
  ,dependencies : [
    raylib_dep,
    math_dep,
    raygui_dep,
    mickstr_dep,
    gl_dep
  ]
  ,c_args : my_extra_args
)

#scroll_exe = executable(
  #'scroll'
  #,'src/scroll.c'
  #,dependencies : [
    #mickstr_dep,
    #math_dep
  #]
  #,c_args : my_extra_args
#)

file_list_exe = executable(
  'file_list'
  ,'src/demos/file_list.c'
  ,include_directories : core_includes
  ,dependencies : [
    raylib_dep,
    math_dep,
    raygui_dep,
    cwalk_dep,
    mickstr_dep
  ]
  ,c_args : my_extra_args
)

tree_exe = executable(
  'tree'
  ,'src/demos/tree.c'
  ,include_directories : core_includes
  ,dependencies : [
    math_dep,
    mickstr_dep
  ]
  ,c_args : my_extra_args
)

#ring_exe = executable(
  #'ring'
  #,'src/containers/fiforing_test.c'
  #,dependencies : [
    #math_dep,
    #mickstr_dep
  #]
  #,c_args : my_extra_args
#)
