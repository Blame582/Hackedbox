# Makefile.am for Hackedbox Settings
# Copyright (c) 2026 Kevin Day <blame582@gmail.com>

noinst_LIBRARIES = libhbsettings.a

libhbsettings_a_SOURCES = \
	src/Settings.cpp \
	src/SWindow.cpp

libhbsettings_a_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/hbtk/include \
	-I$(srcdir)/include

libhbsettings_a_LIBADD = \
	$(top_builddir)/hbtk/libhbtk.a
