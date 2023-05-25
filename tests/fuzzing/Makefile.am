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
noinst_PROGRAMS = fuzz_translate

endif
