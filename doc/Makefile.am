docdir = $(datadir)/liblouis/doc

doc_DATA = \
	liblouis-guide.html \
	liblouis-guide.txt

EXTRA_DIST = \
	liblouis-guide.html \
	liblouis-guide.txt

info_TEXINFOS = liblouis-guide.texi

# generate one big html file
AM_MAKEINFOHTMLFLAGS = --no-headers --no-split

SUFFIXES                = .txt

.texi.txt:
	$(MAKEINFO) --plaintext $< -o $@
