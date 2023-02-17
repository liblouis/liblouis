SUBDIRS = ueb

AM_CPPFLAGS =				\
	$(WARN_CFLAGS)			\
	-I$(top_srcdir)/liblouis	\
	-I$(top_srcdir)/tools/gnulib

run_CPPFLAGS = $(AM_CPPFLAGS) $(XSLT_CFLAGS)

LDADD =							\
	$(top_builddir)/liblouis/liblouis.la		\
	$(top_builddir)/tools/gnulib/libgnutools.la	\
	$(LTLIBINTL)

run_LDADD = $(LDADD) $(XSLT_LIBS)

run_SOURCES = run.c

noinst_PROGRAMS = run

EXTRA_DIST = mathml.xsl
