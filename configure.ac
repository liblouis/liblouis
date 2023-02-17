# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.68)
AC_INIT([Liblouis], [3.24.0], [liblouis-liblouisxml@freelists.org], [liblouis], [http://www.liblouis.org])
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
if test "x$with_yaml" == xno; then
  AC_DEFINE([WITHOUT_YAML],[1],[Disable YAML tests])
else
  AC_CHECK_LIB([yaml], [yaml_parser_initialize])
fi

AM_CONDITIONAL([WITH_YAML], [test "x$ac_cv_lib_yaml_yaml_parser_initialize" == xyes -a "x$with_yaml" != xno])

# If libyaml is missing warn or die depending on --with-yaml
if test x$ac_cv_lib_yaml_yaml_parser_initialize != xyes; then
  case "$with_yaml" in
  yes) AC_MSG_ERROR([--with-yaml was given, but libyaml was not found]);;
  no) ;;
  *) AC_MSG_WARN([libyaml was not found. YAML tests will be skipped]);;
  esac
fi

AC_CHECK_LIB([xml2], [xmlParseFile])
if test "x$ac_cv_lib_xml2_xmlParseFile" == "xyes"; then
  AC_CHECK_LIB([xslt], [xsltInit])
  if test "x$ac_cv_lib_xslt_xsltInit" == "xyes"; then
    AC_CHECK_PROG([XSLT_CONFIG_FOUND], [xslt-config], [yes])
    if test "x${XSLT_CONFIG_FOUND}" = "xyes"; then
      CFLAGS="$CFLAGS `xslt-config --cflags`"
    fi
    AC_CHECK_HEADERS([libxml/parser.h])
    AC_CHECK_HEADERS([libxslt/transform.h])
  fi
fi

# Warn if libxslt is missing
if test "x$ac_cv_lib_xml2_xmlParseFile" == "xyes"       \
     -a "x$ac_cv_lib_xslt_xsltInit" == "xyes"           \
     -a "x$ac_cv_header_libxml_parser_h" == "xyes"      \
     -a "x$ac_cv_header_libxslt_transform_h" == "xyes"  ;
then
  :
else
  AC_DEFINE([SKIP_XSLT_TESTS],[1],[Disable tests that make use of XSLT])
  AC_MSG_WARN([libxslt was not found. Tests that make use of XSLT will be skipped])
fi

# Check for coverage 
AM_CONDITIONAL(USE_COVERAGE, [test x"$with_coverage" = xyes])

# Check for fuzzer 
AC_MSG_CHECKING([whether to enable fuzzer])
AS_IF([test "x$with_fuzzer" = "xyes" && test "$CC" = "clang" && test "$CXX" = "clang++"], AC_MSG_RESULT([yes]), AC_MSG_RESULT([no]))
AM_CONDITIONAL(USE_FUZZER, [test "x$with_fuzzer" = "xyes" && test "$CC" = "clang" && test "$CXX" = "clang++"])

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
LIBLOUIS_REVISION=12

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

# catch as many warnings as possible with the help of the gnulib manywarnings module
gl_MANYWARN_ALL_GCC([warnings])
# Set up the list of the pointless, undesired warnings.
nw=
nw="$nw -Wsystem-headers"       # Don't let system headers trigger warnings
nw="$nw -Wsuggest-attribute=pure"   # ignore recommendations to use
nw="$nw -Wsuggest-attribute=format" # attributes, most of these only
nw="$nw -Wsuggest-attribute=const"  # work in gcc
nw="$nw -Wsuggest-attribute=noreturn"
# Enable all GCC warnings not in this list.
gl_MANYWARN_COMPLEMENT([warnings], [$warnings], [$nw])
for w in $warnings; do
  gl_WARN_ADD([$w])
done

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

if test "x$enable_macros" == xyes; then
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

AC_CANONICAL_HOST
build_macosx=no
case $host_os in
  darwin*)
    build_macosx=yes
  ;;
esac
AM_CONDITIONAL([OSX], [test "$build_macosx" = "yes"])

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
	tests/mathml/Makefile
	tests/mathml/ueb/Makefile
	tests/mathml/ueb/alg_3_2_1_1.test/Makefile
	tests/mathml/ueb/alg_3_2_1_2.test/Makefile
	tests/mathml/ueb/alg_3_2_2.test/Makefile
	tests/mathml/ueb/alg_3_2_3.test/Makefile
	tests/mathml/ueb/alg_3_2_4.test/Makefile
	tests/mathml/ueb/alg_3_2_5.test/Makefile
	tests/mathml/ueb/arrow_over_12.test/Makefile
	tests/mathml/ueb/arrow_under_12.test/Makefile
	tests/mathml/ueb/bana_2_1.test/Makefile
	tests/mathml/ueb/bana_5_1.test/Makefile
	tests/mathml/ueb/bana_5_2.test/Makefile
	tests/mathml/ueb/bana_5_3.test/Makefile
	tests/mathml/ueb/bana_5_4.test/Makefile
	tests/mathml/ueb/bana_5_5.test/Makefile
	tests/mathml/ueb/bana_5a_1.test/Makefile
	tests/mathml/ueb/bana_5a_1_baseline.test/Makefile
	tests/mathml/ueb/bana_5a_2.test/Makefile
	tests/mathml/ueb/bana_5a_2_mtext.test/Makefile
	tests/mathml/ueb/bana_5a_3.test/Makefile
	tests/mathml/ueb/bana_6_1.test/Makefile
	tests/mathml/ueb/bar_12_2_1.test/Makefile
	tests/mathml/ueb/bar_12_2_2.test/Makefile
	tests/mathml/ueb/bar_menclose_12_1_2.test/Makefile
	tests/mathml/ueb/bar_over_12_1_1.test/Makefile
	tests/mathml/ueb/bar_under_12_1_2.test/Makefile
	tests/mathml/ueb/binomial_14_3_3_2.test/Makefile
	tests/mathml/ueb/bold_2_7_1.test/Makefile
	tests/mathml/ueb/bold_2_7_2.test/Makefile
	tests/mathml/ueb/bold_2_7_3.test/Makefile
	tests/mathml/ueb/cap_1_6_1.test/Makefile
	tests/mathml/ueb/cap_1_6_1_separate.test/Makefile
	tests/mathml/ueb/cap_1_6_2.test/Makefile
	tests/mathml/ueb/cap_1_6_4.test/Makefile
	tests/mathml/ueb/chem_16_2_10.test/Makefile
	tests/mathml/ueb/chem_16_2_11.test/Makefile
	tests/mathml/ueb/chem_16_2_13.test/Makefile
	tests/mathml/ueb/chem_16_2_8.test/Makefile
	tests/mathml/ueb/chem_16_2_9.test/Makefile
	tests/mathml/ueb/contractions_1.test/Makefile
	tests/mathml/ueb/contractions_2.test/Makefile
	tests/mathml/ueb/contractions_3.test/Makefile
	tests/mathml/ueb/contractions_4.test/Makefile
	tests/mathml/ueb/contractions_5.test/Makefile
	tests/mathml/ueb/dot_12_1_4.test/Makefile
	tests/mathml/ueb/dot_12_1_5.test/Makefile
	tests/mathml/ueb/dot_12_1_6_double.test/Makefile
	tests/mathml/ueb/dot_12_1_6_single.test/Makefile
	tests/mathml/ueb/example_11_5_1_2.test/Makefile
	tests/mathml/ueb/example_11_5_1_3.test/Makefile
	tests/mathml/ueb/example_11_5_1_4.test/Makefile
	tests/mathml/ueb/example_11_5_2.test/Makefile
	tests/mathml/ueb/example_11_5_3.test/Makefile
	tests/mathml/ueb/example_11_5_4.test/Makefile
	tests/mathml/ueb/example_11_5_5_2.test/Makefile
	tests/mathml/ueb/example_11_5_5_3.test/Makefile
	tests/mathml/ueb/example_11_5_6.test/Makefile
	tests/mathml/ueb/example_11_6.test/Makefile
	tests/mathml/ueb/example_11_6_math_variant.test/Makefile
	tests/mathml/ueb/example_3_4_1.test/Makefile
	tests/mathml/ueb/example_3_4_2.test/Makefile
	tests/mathml/ueb/expr_3_1_1.test/Makefile
	tests/mathml/ueb/expr_3_1_10.test/Makefile
	tests/mathml/ueb/expr_3_1_2.test/Makefile
	tests/mathml/ueb/expr_3_1_3.test/Makefile
	tests/mathml/ueb/expr_3_1_6.test/Makefile
	tests/mathml/ueb/expr_3_1_7.test/Makefile
	tests/mathml/ueb/expr_3_1_8.test/Makefile
	tests/mathml/ueb/expr_3_1_9.test/Makefile
	tests/mathml/ueb/expr_3_1_9_wiris.test/Makefile
	tests/mathml/ueb/fraction_6_1_1.test/Makefile
	tests/mathml/ueb/fraction_6_1_2.test/Makefile
	tests/mathml/ueb/fraction_6_2_1.test/Makefile
	tests/mathml/ueb/fraction_6_2_2.test/Makefile
	tests/mathml/ueb/fraction_6_2_2_unicode_frac.test/Makefile
	tests/mathml/ueb/fraction_6_3_1.test/Makefile
	tests/mathml/ueb/fraction_6_4_1.test/Makefile
	tests/mathml/ueb/fraction_6_4_2.test/Makefile
	tests/mathml/ueb/fraction_6_4_3.test/Makefile
	tests/mathml/ueb/fraction_6_4_4.test/Makefile
	tests/mathml/ueb/fraction_6_4_5.test/Makefile
	tests/mathml/ueb/fraction_6_4_6.test/Makefile
	tests/mathml/ueb/grade1_1_7_1.test/Makefile
	tests/mathml/ueb/grade1_1_7_2.test/Makefile
	tests/mathml/ueb/grade1_1_7_3_1.test/Makefile
	tests/mathml/ueb/grade1_1_7_3_2.test/Makefile
	tests/mathml/ueb/grade1_1_7_4.test/Makefile
	tests/mathml/ueb/hat_12_1_7.test/Makefile
	tests/mathml/ueb/lim_7_9_2.test/Makefile
	tests/mathml/ueb/msub_7_4_3.test/Makefile
	tests/mathml/ueb/msubsup_7_7_1.test/Makefile
	tests/mathml/ueb/msubsup_7_7_2.test/Makefile
	tests/mathml/ueb/msup_7_3_11.test/Makefile
	tests/mathml/ueb/msup_7_3_2.test/Makefile
	tests/mathml/ueb/msup_7_3_3.test/Makefile
	tests/mathml/ueb/msup_7_3_4.test/Makefile
	tests/mathml/ueb/msup_7_3_6.test/Makefile
	tests/mathml/ueb/msup_7_3_7.test/Makefile
	tests/mathml/ueb/msup_7_4_1.test/Makefile
	tests/mathml/ueb/msup_7_4_2.test/Makefile
	tests/mathml/ueb/msup_7_5_1.test/Makefile
	tests/mathml/ueb/msup_7_5_3.test/Makefile
	tests/mathml/ueb/msup_7_6_2.test/Makefile
	tests/mathml/ueb/number_2_1_2.test/Makefile
	tests/mathml/ueb/number_2_1_3.test/Makefile
	tests/mathml/ueb/number_2_2_1.test/Makefile
	tests/mathml/ueb/number_2_2_2.test/Makefile
	tests/mathml/ueb/number_2_2_3.test/Makefile
	tests/mathml/ueb/number_space_after.test/Makefile
	tests/mathml/ueb/number_space_before.test/Makefile
	tests/mathml/ueb/number_space_before_and_after.test/Makefile
	tests/mathml/ueb/pre_msubsup_7_8_1.test/Makefile
	tests/mathml/ueb/pre_sup_7_8_2.test/Makefile
	tests/mathml/ueb/ratio_3_1_11.test/Makefile
	tests/mathml/ueb/ratio_3_1_12.test/Makefile
	tests/mathml/ueb/ratio_3_2_6.test/Makefile
	tests/mathml/ueb/roman_numeral_2_6_1.test/Makefile
	tests/mathml/ueb/roman_numeral_2_6_2.test/Makefile
	tests/mathml/ueb/roman_numeral_2_6_3.test/Makefile
	tests/mathml/ueb/root_8_2_1.test/Makefile
	tests/mathml/ueb/root_8_2_2.test/Makefile
	tests/mathml/ueb/root_8_2_3.test/Makefile
	tests/mathml/ueb/root_8_2_4.test/Makefile
	tests/mathml/ueb/root_letter_base.test/Makefile
	tests/mathml/ueb/set_10_1.test/Makefile
	tests/mathml/ueb/set_10_3.test/Makefile
	tests/mathml/ueb/set_10_4.test/Makefile
	tests/mathml/ueb/set_10_5.test/Makefile
	tests/mathml/ueb/set_10_6.test/Makefile
	tests/mathml/ueb/shape_14_1_1_1.test/Makefile
	tests/mathml/ueb/shape_14_1_2_1.test/Makefile
	tests/mathml/ueb/shape_14_1_2_2.test/Makefile
	tests/mathml/ueb/signs_2_10_16.test/Makefile
	tests/mathml/ueb/signs_2_10_2.test/Makefile
	tests/mathml/ueb/signs_2_10_5.test/Makefile
	tests/mathml/ueb/signs_2_10_8.test/Makefile
	tests/mathml/ueb/spacing_9_3_1_1.test/Makefile
	tests/mathml/ueb/spacing_9_3_1_2.test/Makefile
	tests/mathml/ueb/spacing_9_3_1_3.test/Makefile
	tests/mathml/ueb/spacing_9_3_2_1.test/Makefile
	tests/mathml/ueb/spacing_9_3_2_2.test/Makefile
	tests/mathml/ueb/spacing_9_3_2_3.test/Makefile
	tests/mathml/ueb/spacing_9_3_2_4.test/Makefile
	tests/mathml/ueb/spacing_9_3_2_5.test/Makefile
	tests/mathml/ueb/spacing_9_3_3_1.test/Makefile
	tests/mathml/ueb/spacing_9_3_3_2.test/Makefile
	tests/mathml/ueb/spacing_9_3_3_3.test/Makefile
	tests/mathml/ueb/spacing_9_3_3_4.test/Makefile
	tests/mathml/ueb/spacing_9_3_3_5.test/Makefile
	tests/mathml/ueb/spacing_9_3_3_6.test/Makefile
	tests/mathml/ueb/sqrt_8_1_1.test/Makefile
	tests/mathml/ueb/sqrt_8_1_2.test/Makefile
	tests/mathml/ueb/sqrt_8_1_3.test/Makefile
	tests/mathml/ueb/sqrt_8_1_4.test/Makefile
	tests/mathml/ueb/stat_9_7_2.test/Makefile
	tests/mathml/ueb/sum_7_9_1.test/Makefile
	tests/mathml/ueb/text_9_7_1.test/Makefile
	tests/mathml/ueb/text_9_7_1_mtext.test/Makefile
	tests/mathml/ueb/time_2_4_1.test/Makefile
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
	tools/Makefile
	tools/gnulib/Makefile
	tools/lou_maketable.d/Makefile
	tools/lou_maketable.d/lou_maketable
	gnulib/Makefile])
AC_OUTPUT
