project('foobar', 'c', default_options: ['c_std=gnu11', 'warning_level=1'])

gnome = import('gnome')

glib_dep = dependency('glib-2.0', version: '>=2.78')
gio_dep = dependency('gio-2.0', version: '>=2.78')
gio_unix_dep = dependency('gio-unix-2.0', version: '>=2.78')
gtk_dep = dependency('gtk4', version: '>=4.12')
json_dep = dependency('json-glib-1.0', version: '>=1.8')
nm_dep = dependency('libnm', version: '>=1.44')
gmp_dep = dependency('gmp', version: '>=6.3')
m_dep = meson.get_compiler('c').find_library('m', required : false)
layershell_dep = dependency(
  'gtk4-layer-shell',
  fallback: [ 'gtk4-layer-shell', 'gtk_layer_shell' ],
  default_options: [ 'default_library=static' ],
)
gvc_dep = dependency(
  'gvc',
  fallback: [ 'gvc', 'libgvc_dep' ],
  default_options: [ 'default_library=static' ],
)
mutest_dep = dependency(
  'mutest',
  fallback: [ 'mutest', 'mutest_dep' ],
  default_options: [ 'static=true' ],
  required: false,
)

subdir('res')
subdir('src')

install_man('pub/foobar.1')
