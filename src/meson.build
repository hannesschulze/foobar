foobar_sources = files(
  'main.c',
  'application.c',
  'utils.c',
  'panel.c',
  'launcher.c',
  'launcher-item.c',
  'control-center.c',
  'notification-area.c',
)

subdir('dbus')
subdir('services')
subdir('widgets')

executable(
  meson.project_name(),
  styles_resource,
  icons_resource,
  foobar_sources,
  include_directories: include_directories('.'),
  dependencies: [
    glib_dep,
    gio_dep,
    gio_unix_dep,
    gtk_dep,
    json_dep,
    nm_dep,
    m_dep,
    layershell_dep,
    gvc_dep,
  ],
  install: true,
  c_args: [
    '-Wall',
    '-Werror',
    '-Wswitch-enum',
    '-Wswitch-default',
    '-Wunused',
    '-Wuninitialized',
  ],
)
