EXTRA_DIST = __init__.py.in
CLEANFILES = __init__.py

all:
	source ../../liblouis/liblouis.la ; \
	sed "s/###LIBLOUIS_SONAME###/$$dlname/" \
		< $(srcdir)/__init__.py.in \
		> __init__.py
