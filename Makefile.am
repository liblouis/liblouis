SUBDIRS = gnulib liblouis tools tables man tests python windows

# only build the documentation if we have makeinfo 5
if HAVE_MAKEINFO_5
SUBDIRS += doc
endif

ACLOCAL_AMFLAGS = -I m4 -I gnulib/m4 -I tools/gnulib/m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = liblouis.pc

EXTRA_DIST = liblouis.pc README.windows HACKING

.PHONY: distwin32 distwin64

distwin32 :
	docker build -f Dockerfile.win32 -t liblouis/win32 .
	time=$$(date +'%Y%m%d-%H%M%S') && \
	docker create --name="tmp_$$time" liblouis/win32 && \
	docker cp "tmp_$$time":/usr/src/liblouis/liblouis.zip liblouis-$(VERSION)-win32.zip && \
	docker rm tmp_$$time

distwin64 :
	docker build -f Dockerfile.win64 -t liblouis/win64 .
	time=$$(date +'%Y%m%d-%H%M%S') && \
	docker create --name="tmp_$$time" liblouis/win64 && \
	docker cp "tmp_$$time":/usr/src/liblouis/liblouis.zip liblouis-$(VERSION)-win64.zip && \
	docker rm tmp_$$time

# .dockerignore
.dockerignore : .gitignore
	sed -e 's|^\([^/#]\)|# \1|' -e 's|^/||' -e 's|\+|\\+|g' $< >$@

