liblouisincludedir = $(includedir)/liblouis

# Don't include liblouis.h in dist, this will break subdir builds
# when dist is configured with a different ucs-option than the build,
# i.e. ucs2 dist and ucs4 build and vice versa.
nodist_liblouisinclude_HEADERS = \
	liblouis.h

lib_LTLIBRARIES = liblouis.la

AM_CPPFLAGS = 						\
	$(WARN_CFLAGS)					\
	-DTABLESDIR=\""$(datadir)"/liblouis/tables\"	\
	-I$(top_srcdir)/gnulib				\
	-I$(top_builddir)/gnulib

liblouis_la_LIBADD = $(top_builddir)/gnulib/libgnu.la

liblouis_la_LDFLAGS =	\
	-version-info $(LIBLOUIS_CURRENT):$(LIBLOUIS_REVISION):$(LIBLOUIS_AGE) -no-undefined \
	$(LTLIBINTL)

if HAVE_LD_OUTPUT_DEF
liblouis_la_LDFLAGS += -Wl,--output-def,liblouis-$(DLL_VERSION).def
defexecdir = $(bindir)
defexec_DATA = liblouis-$(DLL_VERSION).def
DISTCLEANFILES = $(defexec_DATA)
endif

liblouis_la_SOURCES =				\
	internal.h					\
	compileTranslationTable.c		\
	lou_translateString.c			\
	lou_backTranslateString.c		\
	commonTranslationFunctions.c		\
	metadata.c 				\
	pattern.c				\
	logging.c				\
	utils.c				\
	maketable.c

# Don't include liblouis.h in dist, this will break subdir builds
# when dist is configured with a different ucs-option than the build,
# i.e. ucs2 dist and ucs4 build and vice versa.
nodist_liblouis_la_SOURCES = \
	liblouis.h

if HAVE_SOURCE_FORMATTER
format-sources-local:
	for file in $(SOURCES); do $(SOURCE_FORMATTER) -i $$file; done
	$(SOURCE_FORMATTER) -i liblouis.h.in
endif

# Rust
RUST_SRC =					\
	$(top_srcdir)/Cargo.toml		\
	$(top_srcdir)/louis_rust/Cargo.toml

CARGO_TARGET_DIR=@abs_top_builddir@/target
RUST_LIB=@abs_top_builddir@/target/@RUST_TARGET_SUBDIR@/liblouis_rust.la

$(RUST_LIB): $(RUST_SRC)
	+cd $(top_srcdir)/louis_rust &&						\
	CARGO_TARGET_DIR=$(CARGO_TARGET_DIR)					\
	$(CARGO) --locked build $(CARGO_VERBOSE) $(CARGO_TARGET_ARGS) $(CARGO_RELEASE_ARGS)

liblouis_la_LIBADD += $(RUST_LIB)

clean-local:
	cd $(top_srcdir)/louis_rust && \
	CARGO_TARGET_DIR=$(CARGO_TARGET_DIR) $(CARGO) clean
