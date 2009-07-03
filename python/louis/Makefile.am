EXTRA_DIST = __init__.py.in
CLEANFILES = __init__.py

LIBLOUIS_SONAME = $(shell source ../../liblouis/liblouis.la ; echo $$dlname)

all:
	source ../../liblouis/liblouis.la ; \
	sed "s/###LIBLOUIS_SONAME###/$$dlname/" \
		< $(srcdir)/__init__.py.in \
		> __init__.py
