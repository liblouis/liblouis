noinst_LTLIBRARIES = liblouis-table-dev.la

liblouis_table_dev_la_SOURCES = table-dev.c

liblouis_table_dev_la_LDFLAGS = -rpath /nowhere

liblouis_table_dev_la_LIBADD = $(top_builddir)/liblouis/liblouis.la

check_PROGRAMS = test-table-dev

test_table_dev_SOURCES = test-table-dev.c

test_table_dev_LDADD = $(top_builddir)/table-dev/liblouis-table-dev.la

TESTS =	$(check_PROGRAMS)

TESTS_ENVIRONMENT =	LD_LIBRARY_PATH=$(top_srcdir)/table-dev/.libs
