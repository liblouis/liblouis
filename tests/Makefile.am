SUBDIRS = tables tablesWithMetadata doctests yaml ueb_test_data

AM_CPPFLAGS =					\
	-I$(top_srcdir)/liblouis		\
	-I$(top_srcdir)/gnulib			\
	-I$(top_srcdir)/tools			\
	-I$(top_builddir)/gnulib		\
	-I$(top_builddir)/tools

LDADD =							\
	$(top_builddir)/liblouis/liblouis.la		\
	$(top_builddir)/gnulib/libgnu.la		\
	$(top_builddir)/tools/libbrlcheck.la		\
	$(top_builddir)/tools/gnulib/libgnutools.la	\
	$(LTLIBINTL)

backtranslate_SOURCES = backtranslate.c
backtranslate_with_letsign_SOURCES = backtranslate_with_letsign.c
capitalization_SOURCES = capitalization.c
capitalized_word_SOURCES = default_table.h capitalized_word.c
checkTable_SOURCES = checkTable.c
emphclass_SOURCES = emphclass.c
en_gb_g1_italics_SOURCES = en_gb_g1_italics.c
findTable_SOURCES = findTable.c
getTable_SOURCES = getTable.c
hash_collision_SOURCES = hash_collision.c
hyphenate_achena_SOURCES = hyphenate_achena.c
hyphenate_alderen_SOURCES = hyphenate_alderen.c
hyphenate_straightforward_SOURCES = hyphenate_straightforward.c
hyphenate_xxx_SOURCES = hyphenate_xxx.c
inpos_SOURCES = inpos.c
inpos_compbrl_SOURCES = default_table.h inpos_compbrl.c
inpos_match_replace_SOURCES = inpos_match_replace.c
lastworditalafter_SOURCES = default_table.h lastworditalafter.c
letterDefTest_SOURCES = letterDefTest.c
logging_SOURCES = logging.c
outpos_SOURCES = outpos.c
pass0_typebuf_SOURCES = pass0_typebuf.c
pass1Only_SOURCES = pass1Only.c
pass2_SOURCES = pass2.c
pass2_inpos_SOURCES = pass2_inpos.c
present_progressive_SOURCES = default_table.h present_progressive.c
resolve_table_SOURCES = resolve_table.c
deep_include_SOURCES = deep_include.c
squash_space_SOURCES = squash_space.c
typeform_SOURCES = typeform.c
typeform_for_emphclass_SOURCES = typeform_for_emphclass.c
uplow_with_unicode_SOURCES = uplow_with_unicode.c

# Note that this is not currently included in check_programs below
# otherwise make check would never complete.
infiniteTranslationLoop_SOURCES = infiniteTranslationLoop.c

program_TESTS =	deep_include

check_PROGRAMS = $(program_TESTS)

dist_check_SCRIPTS =

# if we have Python and liblouis is configured with ucs4 then we can
# invoke the python based tests
if HAVE_PYTHON
if HAVE_UCS4
dist_check_SCRIPTS += \
	check_doctests.py
endif
endif

# The present_progressive should not really fail but for the moment
# the issue doesn't seem to be critical enough that anybody has enough
# interest to fix it.
XFAIL_TESTS =					\
	present_progressive			\
	lastworditalafter			\
	en_gb_g1_italics			\
	squash_space

# en-ueb-symbols_harness.yaml defines the desired behaviour.
# Apparently the tables aren't quite there yet.
XFAIL_TESTS += yaml/en-ueb-symbols_harness.yaml

# FIXME: According to Bue the Danish yaml tests need to be regenerated
# from scratch. They produce numerous failures with his new tables
XFAIL_TESTS +=					\
	yaml/da-dk-g26-dictionary_harness.yaml	\
	yaml/da-dk-g28-dictionary_harness.yaml

dist_yaml_TESTS =

TEST_EXTENSIONS = .yaml
YAML_LOG_COMPILER =  $(top_builddir)/tools/lou_checkyaml

EXTRA_DIST = $(dist_yaml_TESTS)

XFAIL_TESTS += ueb_test_data.pl

TESTS =				\
	$(program_TESTS)	\
	$(dist_check_SCRIPTS)   \
	$(dist_yaml_TESTS)

TESTS_ENVIRONMENT =																						\
	LOUIS_TABLEPATH=$(top_srcdir)/tables,$(top_srcdir)/tests/tables,$(top_srcdir)/tests/tables/moreTables,$(top_srcdir)/tests/tablesWithMetadata,$(top_srcdir)/tests/tables/emphclass,	\
	UEB_TEST_DATA_PATH=$(top_srcdir)/tests/ueb_test_data																	\
	PYTHONPATH=$(top_builddir)/python:$$PYTHONPATH																		\
	LD_LIBRARY_PATH=$(top_builddir)/liblouis/.libs:$$LD_LIBRARY_PATH															\
	PATH=$(top_builddir)/tools:$$PATH
