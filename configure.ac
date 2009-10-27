# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(liblouis, 1.7.0, john.boyer@abilitiessoft.com)
AC_CONFIG_SRCDIR([liblouis/lou_backTranslateString.c])
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_CONFIG_HEADER(liblouis/louiscfg.h)
# AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stddef.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([memset])

# This is for stuff that absolutely must end up in pyconfig.h.
# Please use pyport.h instead, if possible.
AH_TOP([
#ifndef LOUISCFG_H
#define LOUISCFG_H
])
AH_BOTTOM([
#endif /*LOUISCFG_H*/
])

# increment if the interface has additions, changes, removals.
LIBLOUIS_CURRENT=2

# increment any time the source changes; set to
#  0 if you increment CURRENT
LIBLOUIS_REVISION=5

# increment if any interfaces have been added; set to 0
#  if any interfaces have been changed or removed. removal has
#  precedence over adding, so set to 0 if both happened.
LIBLOUIS_AGE=2

AC_SUBST(LIBLOUIS_CURRENT)
AC_SUBST(LIBLOUIS_REVISION)
AC_SUBST(LIBLOUIS_AGE)

AC_ISC_POSIX
AC_HEADER_STDC
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL

AC_PATH_PROG(PKG_CONFIG, pkg-config)

AC_ARG_ENABLE(ucs4,
              AC_HELP_STRING(--enable-ucs4, Enable 4 byte-wide characters.),
              [],
              [enable_ucs4=no])

AH_TEMPLATE(WIDECHAR_SIZE,
  [Define as the size of the unicode type.])

case "$enable_ucs4" in
no) unicode_size="2"
      AC_DEFINE(WIDECHAR_SIZE,2)
      ;;
yes) unicode_size="4"
      AC_DEFINE(WIDECHAR_SIZE,4)
      ;;
esac

case $host in
  *mingw* | *cygwin*)
    CFLAGS="$CFLAGS -Wl,--add-stdcall-alias"
  ;;
esac

AC_CONFIG_FILES([Makefile
                 doc/Makefile
                 liblouis/Makefile
                 tables/Makefile
		         liblouis.pc
                 tests/Makefile
                 tests/table_test_corpuses/Makefile
				 python/Makefile
				 python/louis/Makefile
                 tools/Makefile])
AC_OUTPUT

