foobar_sources = files(
  'application.c',
  'utils.c',
  'panel.c',
  'launcher.c',
  'launcher-item.c',
  'control-center.c',
  'notification-area.c',
)
foobar_tests = {}

subdir('dbus')
subdir('services')
subdir('widgets')

c_args = [
  '-Wall',
  '-Werror',
  '-Wswitch-enum',
  '-Wswitch-default',
  '-Wunused',
  '-Wuninitialized',
]

deps = [
  glib_dep,
  gio_dep,
  gio_unix_dep,
  gtk_dep,
  json_dep,
  gmp_dep,
  nm_dep,
  m_dep,
  layershell_dep,
  gvc_dep,
]

libfoobar = static_library(
  meson.project_name(),
  foobar_sources,
  include_directories: include_directories('.'),
  dependencies: deps,
  install: false,
  c_args: c_args
)

libfoobar_dep = declare_dependency(
  link_with: libfoobar,
  include_directories: include_directories('.'),
  dependencies: deps,
)

executable(
  meson.project_name(),
  'main.c',
  styles_resource,
  icons_resource,
  dependencies: [ libfoobar_dep ],
  install: true,
  c_args: c_args
)

if mutest_dep.found()
  foreach name, sources : foobar_tests
    test(
      name,
      executable(
        'foobar-test-' + name,
        sources,
        dependencies: [ libfoobar_dep, mutest_dep ],
      ),
    )
  endforeach
endif
