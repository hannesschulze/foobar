project('foobar', 'c', default_options: ['c_std=gnu11', 'warning_level=1'])

gnome = import('gnome')
glib_dep = dependency('glib-2.0', version: '>=2.78')
gio_dep = dependency('gio-2.0', version: '>=2.78')

deps = [
  glib_dep,
  gio_dep,
]

sources = []

sources += gnome.gdbus_codegen(
  'server',
  'dbus/com.github.hannesschulze.foobar.xml',
  namespace: 'Foobar',
  annotations: [
    ['com.github.hannesschulze.foobar.Server', 'org.gtk.GDBus.C.Name', 'Server'],
  ],
)

sources += gnome.gdbus_codegen(
  'notifications',
  'dbus/org.freedesktop.Notifications.xml',
  namespace: 'Foobar',
  annotations: [
    ['org.freedesktop.Notifications', 'org.gtk.GDBus.C.Name', 'Notifications'],
  ],
)

sources += gnome.gdbus_codegen(
  'upower',
  'dbus/org.freedesktop.UPower.xml',
  namespace: 'Foobar',
  annotations: [
    ['org.freedesktop.UPower.Device', 'org.gtk.GDBus.C.Name', 'UPowerDevice'],
  ],
)

sources += gnome.gdbus_codegen(
  'bluez',
  'dbus/org.bluez.xml',
  namespace: 'Foobar',
  annotations: [
    ['org.bluez.Adapter1', 'org.gtk.GDBus.C.Name', 'BluezAdapter'],
    ['org.bluez.Device1', 'org.gtk.GDBus.C.Name', 'BluezDevice'],
    ['org.bluez.AgentManager1', 'org.gtk.GDBus.C.Name', 'BluezAgentManager'],
  ],
)

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
