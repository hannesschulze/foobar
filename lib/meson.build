project('foobar', 'c', default_options: ['c_std=gnu11', 'warning_level=1'])

gnome = import('gnome')
glib_dep = dependency('glib-2.0', version: '>=2.78')
gio_dep = dependency('gio-2.0', version: '>=2.78')

deps = [
  glib_dep,
  gio_dep,
]

sources = []
subdir('dbus')
subdir('res')

c_args = [
  '-Wall',
  '-Werror',
  '-Wswitch-enum',
  '-Wswitch-default',
  '-Wunused',
  '-Wuninitialized',
]

static_library(
  'foobar_static',
  sources,
  dependencies: deps,
  install: false,
  c_args: c_args
)

libfoobar = shared_library(
  'foobar',
  sources,
  dependencies: deps,
  install: false,
  c_args: c_args
)

libfoobar_gir = gnome.generate_gir(
  libfoobar,
  sources: sources,
  namespace: 'Foobar',
  link_with: libfoobar,
  nsversion: '1.0.0',
  identifier_prefix: 'Foobar',
  symbol_prefix: 'foobar',
  export_packages: 'foobar',
  dependencies: deps,
  includes: [ 'GObject-2.0', 'Gio-2.0' ],
  install: false,
  extra_args: [ '--fallback-library-path=' ]
)
