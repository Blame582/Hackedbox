# Makefile.am for Hackedbox - an X Window Manager
# Copyright (c) 2026 Kevin Day <blame582@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

AUTOMAKE_OPTIONS = foreign subdir-objects

SUBDIRS = data doc hbtk

DEFAULT_MENU = ~/.@PACKAGE@/menu
DEFAULT_STYLE = $(pkgdatadir)/styles/Default

AM_CPPFLAGS = \
	@SHAPE@ \
	@ORDEREDPSEUDO@ \
	@DEBUG@ \
	@TIMEDCACHE@ \
	-I$(top_srcdir)/hbtk/include \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/hbSettings/include \
	-I$(srcdir)/include \
	-I/usr/include/freetype2 \
	-DDEFAULTMENU=\"$(DEFAULT_MENU)\" \
	-DDEFAULTSTYLE=\"$(DEFAULT_STYLE)\"

bin_PROGRAMS = hackedbox hbsetbg

hackedbox_SOURCES = \
	src/BaseDisplay.cpp \
	src/BaseMenu.cpp \
	src/ClientMenu.cpp \
	src/Clock.cpp \
	src/Color.cpp \
	src/ConfigMenu.cpp \
	src/Font.cpp \
	src/GCCache.cpp \
	src/Hackedbox.cpp \
	src/IconMenu.cpp \
	src/Image.cpp \
	src/ImageControl.cpp \
	src/MenuManager.cpp \
	src/Netizen.cpp \
	src/RootMenu.cpp \
	src/Runbox.cpp \
	src/Screen.cpp \
	src/Texture.cpp \
	src/Timer.cpp \
	src/Util.cpp \
	src/Window.cpp \
	src/WindowMenu.cpp \
	src/Workspace.cpp \
	src/WorkspaceMenu.cpp \
	src/main.cpp \
	hbSettings/src/Settings.cpp \
	hbSettings/src/SWindow.cpp

hackedbox_LDADD = \
	$(top_builddir)/hbtk/libhbtk.a \
	-lX11 \
	-lXft \
	-lXext

hbsetbg_SOURCES = \
	src/HbSetBg.cpp

hbsetbg_CPPFLAGS = \
	$(AM_CPPFLAGS)

hbsetbg_LDADD = \
	$(top_builddir)/src/BaseDisplay.o \
	$(top_builddir)/src/Color.o \
	$(top_builddir)/src/GCCache.o \
	$(top_builddir)/src/Texture.o \
	$(top_builddir)/src/Timer.o \
	$(top_builddir)/src/Image.o \
	$(top_builddir)/src/ImageControl.o \
	$(top_builddir)/src/Util.o \
	-lImlib2 \
	-lX11 \
	-lXft \
	-lXext

MAINTAINERCLEANFILES = \
	aclocal.m4 \
	config.h.in \
	configure \
	Makefile.in

EXTRA_DIST = \
	bootstrap \
	DEPENDENCIES \
	README \
	COPYING \
	ChangeLog \
	ChangeLog.new \
	LICENSE \
	doc \
	hbtk/include/*.hpp \
	hbSettings/include/*.hpp \
	include/*.hpp

uninstall-local:
	-rmdir $(pkgdatadir)

distclean-local:
	rm -f *~ gmon.out .\#*
