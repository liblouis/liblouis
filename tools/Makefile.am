SUBDIRS = gnulib

AM_CPPFLAGS =					\
	-I$(top_srcdir)/liblouis		\
	-I$(top_srcdir)/tools/gnulib		\
	-I$(top_builddir)/tools/gnulib

lou_checkyaml_CPPFLAGS = $(AM_CPPFLAGS) $(YAML_CFLAGS)

LDADD =							\
	$(top_builddir)/liblouis/liblouis.la		\
	$(top_builddir)/tools/gnulib/libgnutools.la	\
	libbrlcheck.la		\
	$(LTLIBINTL)

# libbrlcheck is a convenience library that contains functionality to
# check a translation. This is mostly needed for the tests in ../tests
# but it is also needed for lou_checkyaml. So this functionality is
# packaged up in what automake calls a convenience library, a lib that
# is solely built at compile time but never installed.
noinst_LTLIBRARIES = libbrlcheck.la
libbrlcheck_la_SOURCES =			\
	brl_checks.h				\
	brl_checks.c

# libbrlcheck depends on the liblouis shared lib. Apparently the
# following is needed to make sure the symbols are resolved from the
# liblouis library. Otherwise we get missing symbol errors when
# compiling the test suite, which depends on libbrlcheck.
libbrlcheck_la_LIBADD =					\
	$(top_builddir)/liblouis/liblouis.la		\
	$(top_builddir)/tools/gnulib/libgnutools.la

lou_checkyaml_LDADD = $(LDADD) $(YAML_LIBS)

bin_PROGRAMS =					\
	lou_allround				\
	lou_checkhyphens			\
	lou_checktable				\
	lou_debug				\
	lou_translate				\
	lou_trace				\
	lou_compare

# install lou_checkyaml only if libyaml is found
if WITH_YAML
bin_PROGRAMS += lou_checkyaml
else
noinst_PROGRAMS = lou_checkyaml
endif

lou_allround_SOURCES = lou_allround.c
lou_checkhyphens_SOURCES = lou_checkhyphens.c
lou_checktable_SOURCES = lou_checktable.c
lou_debug_SOURCES = lou_debug.c
lou_translate_SOURCES =	lou_translate.c
lou_trace_SOURCES = lou_trace.c
lou_compare_SOURCES = lou_compare.c
lou_checkyaml_SOURCES =	lou_checkyaml.c
