SUBDIRS = gnulib liblouis tools tables man tests python windows

# only build the documentation if we have makeinfo 5
if HAVE_MAKEINFO_5
SUBDIRS += doc
endif

ACLOCAL_AMFLAGS = -I m4 -I gnulib/m4 -I tools/gnulib/m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = liblouis.pc

EXTRA_DIST = liblouis.pc README.windows HACKING

