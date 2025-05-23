# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.68)
AC_INIT([Liblouis], [3.33.0], [liblouis-liblouisxml@freelists.org], [liblouis], [http://www.liblouis.io])
AC_CONFIG_SRCDIR([liblouis/lou_backTranslateString.c])
AM_CONFIG_HEADER(liblouis/config.h)

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall dist-zip])

# Checks for programs.
AC_PROG_CC

# gnulib for the library
gl_EARLY
# gnulib for the tools
gl_tools_EARLY

gl_INIT
gl_tools_INIT

# Checks for more programs.
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_AR

# Check if the user wants to enable yaml tests
AC_ARG_WITH([yaml],
  [AS_HELP_STRING([--with-yaml], [enable YAML-based tests @<:@default=yes@:>@])],
  [with_yaml=$withval],
  [])

# Check if the user wants to enable coverage
AC_ARG_WITH([coverage],
    [AS_HELP_STRING([--with-coverage], [enable clang coverage in the fuzzer tests (also add coverage to lib sources) @<:@default=no@:>@])],
    [])

# Check if the user wants to compile fuzzer
AC_ARG_WITH([fuzzer],
    [AS_HELP_STRING([--with-fuzzer], [enable fuzzer for function lou_translateString @<:@default=no@:>@])],
    [])

# Checks for libraries.
if test "x$with_yaml" = xno; then
  AC_DEFINE([WITHOUT_YAML],[1],[Disable YAML tests])
else
  AC_CHECK_LIB([yaml], [yaml_parser_initialize])
fi

AM_CONDITIONAL([WITH_YAML], [test "x$ac_cv_lib_yaml_yaml_parser_initialize" = xyes -a "x$with_yaml" != xno])

# If libyaml is missing warn or die depending on --with-yaml
if test x$ac_cv_lib_yaml_yaml_parser_initialize != xyes; then
  case "$with_yaml" in
  yes) AC_MSG_ERROR([--with-yaml was given, but libyaml was not found]);;
  no) ;;
  *) AC_MSG_WARN([libyaml was not found. YAML tests will be skipped]);;
  esac
fi

# Check for coverage 
AM_CONDITIONAL(USE_COVERAGE, [test x"$with_coverage" = xyes])

# Check for fuzzer 
AC_MSG_CHECKING([whether to enable fuzzer])
AS_IF([test "$CC" != "clang" || test "$CXX" != "clang++"], [with_fuzzer=no])
AS_IF([test "x$with_fuzzer" = "xyes"],
      [
        AC_MSG_RESULT([yes])
        HOST_CPU="`$CC -print-target-triple | sed -e 's/-.*//'`"
        CFLAGS="$CFLAGS -fsanitize=fuzzer-no-link"
        LIBFUZZER_NO_MAIN="`$CC $CFLAGS -print-file-name=libclang_rt.fuzzer_no_main-$HOST_CPU.a` -lstdc++ -lm"
      ],
      AC_MSG_RESULT([no]))
AC_SUBST(LIBFUZZER_NO_MAIN)
AM_CONDITIONAL(USE_FUZZER, [test "x$with_fuzzer" = "xyes"])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stddef.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.
AC_FUNC_MEMCMP
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
LIBLOUIS_CURRENT=20

# increment any time the source changes; set to 0 if you increment
# CURRENT
LIBLOUIS_REVISION=21

# increment if any interfaces have been added; set to 0 if any
# interfaces have been changed or removed. removal has precedence over
# adding, so set to 0 if both happened.
LIBLOUIS_AGE=0

AC_SUBST(LIBLOUIS_CURRENT)
AC_SUBST(LIBLOUIS_REVISION)
AC_SUBST(LIBLOUIS_AGE)

# needed to build the liblouis.def
DLL_VERSION=`expr ${LIBLOUIS_CURRENT} - ${LIBLOUIS_AGE}`
AC_SUBST(DLL_VERSION)

AC_ISC_POSIX
AC_HEADER_STDC
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

# define an additional recursive make target to format the sources.
# Also check if clang-format is installed
AM_EXTRA_RECURSIVE_TARGETS([format-sources])
# prefer version 14 of clang-format as it is part of Ubuntu 22.04 LTS
AC_CHECK_PROGS([SOURCE_FORMATTER], [clang-format-14 clang-format])
AM_CONDITIONAL([HAVE_SOURCE_FORMATTER], [test x$SOURCE_FORMATTER = xclang-format -o x$SOURCE_FORMATTER = xclang-format-14])

# GNU help2man creates man pages from --help output; in many cases,
# this is sufficient, and obviates the need to maintain man pages
# separately. However, some developers do not have it so we do not
# make its use mandatory.
if test "x$cross_compiling" = xyes; then
  AC_MSG_WARN([cannot generate manual pages while cross compiling])
else
  AC_CHECK_PROG([HELP2MAN], [help2man], [help2man])
fi
AM_CONDITIONAL([HAVE_HELP2MAN], [test x$HELP2MAN = xhelp2man])

# Only run parts of the tests if we are cross compiling
AM_CONDITIONAL([CROSS_COMPILING], [test "x$cross_compiling" = xyes])

# Check for makeinfo version >= 5, required for building documentation.
have_makeinfo_5=false
AC_PROG_SED
AC_CHECK_PROG([MAKEINFO_FOUND], [makeinfo], [yes])
if test x"${MAKEINFO_FOUND}" = xyes
then
  MAKEINFO_VERSION_REQ=5
  AC_MSG_CHECKING([for makeinfo version >= $MAKEINFO_VERSION_REQ])
  MAKEINFO_VERSION=`makeinfo --version | sed -ne 's/^\(makeinfo\|texi2any\) .* \([[0-9]][[0-9]]*\)\.[[0-9]][[0-9]]*.*$/\2/p'`
  if test x$MAKEINFO_VERSION = x -o 0$MAKEINFO_VERSION -lt $MAKEINFO_VERSION_REQ
  then
    AC_MSG_RESULT([no])
    AC_MSG_WARN([Program 'makeinfo' version >= $MAKEINFO_VERSION_REQ is required. Documentation will not be built.])
  else
    AC_MSG_RESULT([yes])
    have_makeinfo_5=true
  fi
else
  AC_MSG_WARN([Missing program 'makeinfo', Documentation will not be built. Please install you you want documentation or refer to online resources.])
fi
AM_CONDITIONAL([HAVE_MAKEINFO_5], [test x$have_makeinfo_5 = xtrue])

# Check if the user wants to enable macros.
# Macros are an experimental feature and need to be enabled
# explicitely. They are a helpful tool for refactoring, but for now we
# don't want to expose it to users yet.
AC_ARG_ENABLE([macros],
  [AS_HELP_STRING([--enable-macros], [enable macros @<:@default=no@:>@])],
  [enable_macros=$enableval],
  [])

if test "x$enable_macros" = xyes; then
   AC_DEFINE([ENABLE_MACROS],[1],[Enable macros])
fi
AM_CONDITIONAL([ENABLE_MACROS], [test "x$enable_macros" = xyes])

AC_ARG_ENABLE(ucs4,
              AC_HELP_STRING(--enable-ucs4, enable 4 byte-wide characters),
              [AC_DEFINE([WIDECHARS_ARE_UCS4], [1], [Define if widechars are ucs4])],
              [enable_ucs4=no])

AC_MSG_CHECKING([whether 4 byte-wide characters should be supported])
AC_MSG_RESULT($enable_ucs4)

case "$enable_ucs4" in
yes) WIDECHAR_TYPE='unsigned int';;
*) WIDECHAR_TYPE='unsigned short int';;
esac
AC_SUBST(WIDECHAR_TYPE)
AM_CONDITIONAL([HAVE_UCS4], [test x$enable_ucs4 = xyes])

case $host in
  *mingw* | *cygwin*)
    CFLAGS="$CFLAGS -Wl,--add-stdcall-alias"
    LDFLAGS="$LDFLAGS -avoid-version"
  ;;
esac

# add an --enable-macos-universal-binary flag that when building for
# mac create a universal x86_64 / arm64 binary
AC_ARG_ENABLE([macos-universal-binary],
    [AS_HELP_STRING([--enable-macos-universal-binary],
            [Create an universal x86_64+arm64 binary when building for macos)])],,
    [enable_macos_universal_binary=no])

# Determine if we're building for macos/darwin by checking for macos &
# darwin in the build triple.
AC_CANONICAL_HOST
build_macosx=no
case $host_os in
  *macos*)
    build_macosx=yes ;;
  *darwin*)
    build_macosx=yes ;;
esac
AM_CONDITIONAL([OSX], [test "$build_macosx" = "yes"])

# If building for Mac we will pass x86_64 or arm64 to the compiler
# based on user options or target type.
AS_IF([test "$build_macosx" = "yes"],[
  clang_arch_x86_64="no"
  clang_arch_arm64="no"

  # If building a universal binary ask clang for both
  AS_IF([test "$enable_macos_universal_binary" = "yes"],[
      clang_arch_x86_64="yes"
      clang_arch_arm64="yes"
  ],[
    # If building for x86_64 ask for that
    AS_IF([test "$host_cpu" = "x86_64"],[
      clang_arch_x86_64="yes"
    ])
    # If building for arm64, ask for that
    AS_IF([test "$host_cpu" = "aarch64"],[
      clang_arch_arm64="yes"
    ])
  ])
  # set the defines for our makefile
  AM_CONDITIONAL([CLANG_ARCH_X86_64], [test "$clang_arch_x86_64" = "yes"])
  AM_CONDITIONAL([CLANG_ARCH_ARM64], [test "$clang_arch_arm64" = "yes"])
], [
  # Just set these to false. They need to be present in the else
  # clause to keep automake happy.
  AM_CONDITIONAL([CLANG_ARCH_X86_64], [false])
  AM_CONDITIONAL([CLANG_ARCH_ARM64], [false])
])

AC_CONFIG_FILES([
	Makefile
	doc/Makefile
	man/Makefile
	liblouis/Makefile
	liblouis/liblouis.h
	windows/Makefile
	windows/include/Makefile
	windows/utils/Makefile
	tables/Makefile
	liblouis.pc
	tests/Makefile
	tests/braille-specs/Makefile
	tests/fuzzing/Makefile
	tests/resolve_table.h
	tests/tables/Makefile
	tests/tables/emphclass/Makefile
	tests/tables/attribute/Makefile
	tests/tables/moreTables/Makefile
	tests/tables/resolve_table/Makefile
	tests/tables/resolve_table/dir_1/Makefile
	tests/tables/resolve_table/dir_1/dir_1.1/Makefile
	tests/tables/resolve_table/dir_2/Makefile
	tests/tablesWithMetadata/Makefile
	tests/ueb_test_data/Makefile
	tests/yaml/Makefile
	python/Makefile
	python/setup.py
	python/louis/Makefile
	python/examples/Makefile
	python/tests/Makefile
	tools/Makefile
	tools/gnulib/Makefile
	tools/lou_maketable.d/Makefile
	tools/lou_maketable.d/lou_maketable
	gnulib/Makefile])
AC_OUTPUT
