foobar_sources += gnome.gdbus_codegen(
  'server',
  'com.github.hannesschulze.foobar.xml',
  namespace: 'Foobar',
  annotations: [
    ['com.github.hannesschulze.foobar.Server', 'org.gtk.GDBus.C.Name', 'Server'],
  ],
)

foobar_sources += gnome.gdbus_codegen(
  'notifications',
  'org.freedesktop.Notifications.xml',
  namespace: 'Foobar',
  annotations: [
    ['org.freedesktop.Notifications', 'org.gtk.GDBus.C.Name', 'Notifications'],
  ],
)

foobar_sources += gnome.gdbus_codegen(
  'upower',
  'org.freedesktop.UPower.xml',
  namespace: 'Foobar',
  annotations: [
    ['org.freedesktop.UPower.Device', 'org.gtk.GDBus.C.Name', 'UPowerDevice'],
  ],
)

foobar_sources += gnome.gdbus_codegen(
  'bluez',
  'org.bluez.xml',
  namespace: 'Foobar',
  annotations: [
    ['org.bluez.Adapter1', 'org.gtk.GDBus.C.Name', 'BluezAdapter'],
    ['org.bluez.Device1', 'org.gtk.GDBus.C.Name', 'BluezDevice'],
    ['org.bluez.AgentManager1', 'org.gtk.GDBus.C.Name', 'BluezAgentManager'],
  ],
)
