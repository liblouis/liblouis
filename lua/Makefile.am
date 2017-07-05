lib_LTLIBRARIES = liblua-louis.la

AM_CPPFLAGS =					\
	-I$(top_srcdir)/liblouis		\
	-I$(top_srcdir)/tools/gnulib		\
	-I$(top_builddir)/tools/gnulib

liblua_louis_la_LDFLAGS = $(LTLIBINTL)
liblua_louis_la_LIBADD =				\
	$(top_builddir)/liblouis/liblouis.la		\
	$(top_builddir)/tools/gnulib/libgnutools.la	\
	$(LTLIBINTL)

liblua_louis_la_SOURCES = lua_louis.c
