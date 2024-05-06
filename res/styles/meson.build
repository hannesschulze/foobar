sassc = find_program('sassc')

styles = ['default', 'monokai-pro', 'teatime']
style_targets = []

common_inputs = files(
  'partials/_base.scss',
  'partials/_notification.scss',
  'partials/_panel.scss',
  'partials/_launcher.scss',
  'partials/_control-center.scss',
  '_common.scss',
)

foreach style : styles
  style_targets += custom_target(
    style + '.scss',
    input: [style + '.scss'] + common_inputs,
    output: style + '.css',
    command: [sassc, '-a', '-M', '-t', 'compact', '@INPUT0@', '@OUTPUT@'],
  )
endforeach

styles_resource = gnome.compile_resources(
  'styles-resource',
  'styles.gresource.xml',
  source_dir: [
    meson.current_build_dir(),
    meson.current_source_dir(),
  ],
  dependencies: style_targets,
)
