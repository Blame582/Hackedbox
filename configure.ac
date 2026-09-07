dnl configure.ac for Hackedbox
dnl Initialize autoconf and automake

AC_INIT([Hackedbox], [0.8.8])
AC_CONFIG_SRCDIR([src/Hackedbox.cpp])

AM_INIT_AUTOMAKE([no-define])

dnl Determine default prefix
test x$prefix = "xNONE" && prefix="$ac_default_prefix"

dnl Locate required external software
AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB
AC_PROG_INSTALL

AC_CHECK_PROGS([regex_cmd], [sed])
if test x$regex_cmd = "x"; then
  AC_MSG_ERROR([sed is required to build the default menu file.])
fi

dnl Check for system header files
AC_CHECK_HEADERS([
  ctype.h
  dirent.h
  fcntl.h
  libgen.h
  locale.h
  nl_types.h
  process.h
  signal.h
  stdarg.h
  stdio.h
  stdlib.h
  string.h
  time.h
  unistd.h
  sys/param.h
  sys/select.h
  sys/signal.h
  sys/stat.h
  sys/time.h
  sys/types.h
  sys/wait.h
])

dnl Check for existence of basename(), setlocale() and strftime()
AC_CHECK_FUNCS([
  basename
  getpid
  setlocale
  sigaction
  strftime
  strcasestr
  snprintf
  vsnprintf
  catopen
  catgets
  catclose
])

AC_CHECK_LIB(
  [gen],
  [basename],
  [
    AC_DEFINE([HAVE_BASENAME], [1], [Define if basename() is available in libgen.])
    LIBS="$LIBS -lgen"
  ]
)

AC_CHECK_LIB([nsl], [t_open], [LIBS="$LIBS -lnsl"])
AC_CHECK_LIB([socket], [socket], [LIBS="$LIBS -lsocket"])

dnl Check for X headers and libraries
AC_PATH_X
AC_PATH_XTRA

test x$no_x = "xyes" && AC_MSG_ERROR(
  [Hackedbox requires the X Window System libraries and headers.]
)

test x$x_includes = "x" && x_includes="/usr/include"
test x$x_libraries = "x" && x_libraries="/usr/lib"

CFLAGS="$CFLAGS $X_CFLAGS"
CXXFLAGS="$CXXFLAGS $X_CFLAGS"
LIBS="$LIBS $X_LIBS"
LDFLAGS="$LDFLAGS $LIBS $X_PRE_LIBS"

dnl Check for GCC
AC_MSG_CHECKING([for GCC])

if test x$GCC = "xyes"; then

  AC_MSG_RESULT([yes])

  CXXFLAGS="${CXXFLAGS} -Wall -W -pedantic"

else

  AC_MSG_RESULT([no, trying other compilers])

  AC_MSG_CHECKING([for MIPSpro])

  mips_pro_ver=`$CC -version 2>&1 | grep -i mipspro | cut -f4 -d ' '`

  if test "x$mips_pro_ver" = "x"; then

    AC_MSG_RESULT([no])

  else

    AC_MSG_RESULT([yes, version $mips_pro_ver.])

    AC_MSG_CHECKING([for -LANG:std in CXXFLAGS])

    lang_std_not_set=`echo $CXXFLAGS | grep "\-LANG:std"`

    if test "x$lang_std_not_set" = "x"; then

      AC_MSG_RESULT([not set, setting.])

      CXXFLAGS="${CXXFLAGS} -LANG:std"

    else

      AC_MSG_RESULT([already set.])

    fi
  fi
fi

dnl Check for required functions in -lX11
AC_CHECK_LIB(
  [X11],
  [XOpenDisplay],
  [LIBS="$LIBS -lX11"],
  [AC_MSG_ERROR([Could not find XOpenDisplay in -lX11.])]
)

dnl Check for required functions in -lXft
AC_CHECK_LIB(
  [Xft],
  [XftFontOpenName],
  [LIBS="$LIBS -lXft"],
  [AC_MSG_ERROR([Could not find XftFontOpenName in -lXft.])]
)

LIBS="$LIBS $X_EXTRA_LIBS"

dnl ------------------------------------------------------------
dnl XShape extension
dnl ------------------------------------------------------------

Xext_lib=""
SHAPE=""

AC_MSG_CHECKING(
  [whether to build support for the XShape extension]
)

AC_ARG_ENABLE(
  [shape],
  [AS_HELP_STRING([--enable-shape],
                  [enable support of the XShape extension @<:@default=yes@:>@])]
)

: ${enableval="yes"}

if test x$enableval = "xyes"; then

  AC_MSG_RESULT([yes])

  AC_CHECK_LIB(
    [Xext],
    [XShapeCombineShape],

    AC_MSG_CHECKING(
      [for X11/extensions/shape.h]
    )

    AC_LINK_IFELSE(
      [
        AC_LANG_PROGRAM(
          [[
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
          ]],
          [[
long foo = ShapeSet;
(void) foo;
          ]]
        )
      ],
      [
        AC_MSG_RESULT([yes])
        SHAPE="-DSHAPE"
        Xext_lib="-lXext"
      ],
      [
        AC_MSG_RESULT([no])
      ]
    )
  )

else

  AC_MSG_RESULT([no])

fi

AC_SUBST([SHAPE])

LIBS="$LIBS $Xext_lib"


dnl ------------------------------------------------------------
dnl Ordered 8bpp dithering
dnl ------------------------------------------------------------

ORDEREDPSEUDO=""

AC_MSG_CHECKING(
  [whether to include Pseudocolor ordered dithering code]
)

AC_ARG_ENABLE(
  [ordered-pseudo],

  [AS_HELP_STRING([--enable-ordered-pseudo],
                  [include code for ordered pseudocolor (8bpp) dithering @<:@default=no@:>@])],

  if test x$enableval = "xyes"; then

    AC_MSG_RESULT([yes])
    ORDEREDPSEUDO="-DORDEREDPSEUDO"

  else

    AC_MSG_RESULT([no])

  fi,

  AC_MSG_RESULT([no])
)

AC_SUBST([ORDEREDPSEUDO])

dnl ------------------------------------------------------------
dnl Debugging
dnl ------------------------------------------------------------

DEBUG=""

AC_MSG_CHECKING(
  [whether to include verbose debugging code]
)

AC_ARG_ENABLE(
  [debug],

  [AS_HELP_STRING([--enable-debug],
                  [include verbose debugging code @<:@default=no@:>@])],

  if test x$enableval = "xyes"; then

    AC_MSG_RESULT([yes])

    if test x$GCC = "xyes"; then

      DEBUG="-DDEBUG -fno-inline -g"

    else

      DEBUG="-DDEBUG"

    fi

  else

    AC_MSG_RESULT([no])
    DEBUG="-DNDEBUG"

  fi,

  AC_MSG_RESULT([no])
  DEBUG="-DNDEBUG"
)

AC_SUBST([DEBUG])

dnl ------------------------------------------------------------
dnl Timed pixmap cache
dnl ------------------------------------------------------------

TIMEDCACHE=""

AC_MSG_CHECKING(
  [whether to use the new timed pixmap cache]
)

AC_ARG_ENABLE(
  [timed-cache],

  [AS_HELP_STRING([--enable-timed-cache],
                  [use new timed pixmap cache @<:@default=yes@:>@])],

  if test x$enableval = "xyes"; then

    AC_MSG_RESULT([yes])
    TIMEDCACHE="-DTIMEDCACHE"

  else

    AC_MSG_RESULT([no])

  fi,

  AC_MSG_RESULT([yes])
  TIMEDCACHE="-DTIMEDCACHE"
)

AC_SUBST([TIMEDCACHE])

dnl Determine if maintainer portions of the Makefiles should be included
AM_MAINTAINER_MODE

dnl ------------------------------------------------------------
dnl Configuration header
dnl ------------------------------------------------------------

AC_CONFIG_HEADERS([config.h])

dnl ------------------------------------------------------------
dnl Output files
dnl ------------------------------------------------------------

AC_CONFIG_FILES([
  Makefile
  hbtk/Makefile
  data/Makefile
  data/menu
  data/styles/Makefile
  data/styles/Default
  data/backgrounds/Makefile
  doc/hackedbox.1
  doc/Makefile
  version.h
])

AC_OUTPUT

dnl ------------------------------------------------------------
dnl Configuration summary
dnl ------------------------------------------------------------

AC_MSG_RESULT([])
AC_MSG_RESULT([***************************************************])
AC_MSG_RESULT([])
AC_MSG_RESULT([$PACKAGE version $VERSION configured successfully.])
AC_MSG_RESULT([])
AC_MSG_RESULT([Using '$prefix' for installation.])
AC_MSG_RESULT([Using '$CXX' for C++ compiler.])
AC_MSG_RESULT([Building with '$CXXFLAGS' for C++ compiler flags.])
AC_MSG_RESULT([Building with '$LIBS' for linker flags.])
AC_MSG_RESULT([])
AC_MSG_RESULT([***************************************************])
