icons_resource = gnome.compile_resources(
  'icons-resource',
  'icons.gresource.xml',
  source_dir: [
    meson.current_build_dir(),
    meson.current_source_dir(),
  ],
  dependencies: style_targets,
  c_name: 'foobar_icons',
  export: true,
  extra_args: [ '--manual-register' ],
)
