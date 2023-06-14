if USE_FUZZER

AM_CPPFLAGS =					\
	-fno-omit-frame-pointer 		\
	-I$(top_srcdir)/liblouis		\
	-I$(top_srcdir)/gnulib			\
	-I$(top_builddir)/gnulib		\
	-g -O2

LDADD =							\
	$(top_builddir)/liblouis/liblouis.la		\
	$(LIBFUZZER_NO_MAIN)

if USE_COVERAGE
AM_CPPFLAGS += -fprofile-instr-generate -fcoverage-mapping
endif

fuzz_translate_SOURCES = fuzz_translate.c
fuzz_backtranslate_SOURCES = fuzz_backtranslate.c
fuzz_translate_generic_SOURCES = fuzz_translate_generic.c
noinst_PROGRAMS = fuzz_translate fuzz_backtranslate fuzz_translate_generic

endif
