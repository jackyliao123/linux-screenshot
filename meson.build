project(
  'linux-screenshot',
  'c',
  default_options: [
    'c_std=c11',
  ],
)

cc = meson.get_compiler('c')

m_dep = cc.find_library('m', required : false)

gtk_dep = dependency('gtk+-3.0')
X11_dep = dependency('X11')
Xext_dep = dependency('xext')
Xrandr_dep = dependency('xrandr')

src_files = files(
  'main.c',
  'display_X11.c',
  'overlay.c',
  'tooltip.c',
  'selection.c',
  'geometry.c',
)

executable(
  'screenshot',
  src_files,
  dependencies: [
    gtk_dep,
    X11_dep,
    Xext_dep,
    Xrandr_dep,
    m_dep,
  ],
  install: true,
)
