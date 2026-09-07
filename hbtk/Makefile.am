# Makefile.am for HbTK
# Hackedbox Toolkit

AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = \
  -I$(srcdir)/include \
  -I/usr/include/freetype2

noinst_LIBRARIES = libhbtk.a

libhbtk_a_SOURCES = \
  src/Application.cpp \
  src/Button.cpp \
  src/Colors.cpp \
  src/Dialog.cpp \
  src/Entry.cpp \
  src/Event.cpp \
  src/EventDispatcher.cpp \
  src/Frame.cpp \
  src/Geometry.cpp \
  src/MainWindow.cpp \
  src/Module.cpp \
  src/Painter.cpp \
  src/Splitter.cpp \
  src/Style.cpp \
  src/StyleEngine.cpp \
  src/TerminalWidget.cpp \
  src/TitleBar.cpp \
  src/Widget.cpp \
  src/WidgetContainer.cpp

EXTRA_DIST = \
  include/*.hpp
