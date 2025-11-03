project('mickstr', 'c')

cc = meson.get_compiler('c')
math_dep = cc.find_library('m', required : false)

mickstr_includes = include_directories('./mickstr')
mickstr_lib = library('mickstr',
  './mickstr_implementation.c',
  dependencies: math_dep,
  include_directories: mickstr_includes,
  install: true,
  c_args: [ '-Wall', '-Wextra' ]
)
mickstr_dep = declare_dependency(
  include_directories: mickstr_includes,
  link_with: mickstr_lib
)
