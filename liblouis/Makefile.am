INCLUDES =		\
	-I$(top_srcdir)

liblouisincludedir = $(includedir)/liblouis

liblouisinclude_HEADERS = liblouis.h

lib_LTLIBRARIES = liblouis.la

liblouis_la_LDFLAGS =	\
	-version-info $(LIBLOUIS_CURRENT):$(LIBLOUIS_REVISION):$(LIBLOUIS_AGE) -no-undefined

liblouis_la_SOURCES =			\
	lou_backTranslateString.c	\
	compileTranslationTable.c	\
	louis.h						\
	liblouis.h					\
	lou_translateString.c
