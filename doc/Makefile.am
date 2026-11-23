
info_TEXINFOS = liblouis.texi

SUFFIXES                = .txt

.texi.txt:
	$(MAKEINFO) --plaintext $< -o $@

# Generate single-page HTML in addition to the default multi-page
html-local: liblouis-single.html

liblouis-single.html: liblouis.texi
	$(MAKEINFO) --html --no-headers --no-split -I $(srcdir) -o $@ $<

CLEANFILES = liblouis-single.html
