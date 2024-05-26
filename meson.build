project('foobar', 'c', default_options: ['c_std=gnu11', 'warning_level=1'])

gnome = import('gnome')

glib_dep = dependency('glib-2.0', version: '>=2.78')
gio_dep = dependency('gio-2.0', version: '>=2.78')
gio_unix_dep = dependency('gio-unix-2.0', version: '>=2.78')
gtk_dep = dependency('gtk4', version: '>=4.12')
json_dep = dependency('json-glib-1.0', version: '>=1.8')
nm_dep = dependency('libnm', version: '>=1.44')
m_dep = meson.get_compiler('c').find_library('m', required : false)
layershell_dep = dependency('gtk4-layer-shell', required: false)
gvc_dep = dependency('gvc', required: false)
if not layershell_dep.found()
  layershell_proj = subproject('gtk4-layer-shell')
  layershell_dep = layershell_proj.get_variable('gtk_layer_shell')
endif
if not gvc_dep.found()
  gvc_proj = subproject('gvc')
  gvc_dep = gvc_proj.get_variable('libgvc_dep')
endif

subdir('res')
subdir('src')

install_man('pub/foobar.1')
