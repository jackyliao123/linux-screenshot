project('linux-screenshot', 'c')

cc = meson.get_compiler('c')

m_dep = cc.find_library('m', required : false)

gtk_dep = dependency('gtk+-3.0')
X11_dep = dependency('X11')
Xext_dep = dependency('xext')

executable(
  'screenshot',
  'main.c',
  dependencies: [
    gtk_dep,
    X11_dep,
    Xext_dep,
    m_dep,
  ]
)
