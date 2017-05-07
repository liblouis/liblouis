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
backtranslate_noUndefinedDots_SOURCES = backtranslate_noUndefinedDots.c
backtranslate_partialTrans_SOURCES = backtranslate_noUndefinedDots.c
backtranslate_with_letsign_SOURCES = backtranslate_with_letsign.c
capitalization_SOURCES = capitalization.c
capitalized_word_SOURCES = default_table.h capitalized_word.c
checkTable_SOURCES = checkTable.c
compbrlAtCursor_SOURCES = compbrlAtCursor.c
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
squash_space_SOURCES = squash_space.c
typeform_SOURCES = typeform.c
typeform_for_emphclass_SOURCES = typeform_for_emphclass.c
uplow_with_unicode_SOURCES = uplow_with_unicode.c
check_metadata_SOURCES = check_metadata.c

# Note that this is not currently included in check_programs below
# otherwise make check would never complete.
infiniteTranslationLoop_SOURCES = infiniteTranslationLoop.c
noinst_PROGRAMS = infiniteTranslationLoop

program_TESTS =					\
	backtranslate				\
	backtranslate_noUndefinedDots		\
	backtranslate_partialTrans		\
	backtranslate_with_letsign		\
	capitalization				\
	capitalized_with_sentance		\
	capitalized_word			\
	checkTable				\
	check_metadata				\
	compbrlAtCursor				\
	emphclass				\
	en_gb_g1_italics			\
	findTable				\
	getTable				\
	hash_collision				\
	hyphenate_achena			\
	hyphenate_alderen			\
	hyphenate_straightforward		\
	hyphenate_xxx				\
	inpos					\
	inpos_compbrl				\
	inpos_match_replace			\
	lastworditalafter			\
	letterDefTest				\
	logging					\
	outpos					\
	pass0_typebuf				\
	pass1Only				\
	pass2					\
	pass2_inpos				\
	present_progressive			\
	resolve_table				\
	squash_space				\
	typeform				\
	typeform_for_emphclass			\
	uplow_with_unicode

check_PROGRAMS = $(program_TESTS)

dist_check_SCRIPTS =				\
	check_all_tables.pl			\
	check_endless_loop.pl			\
	check_included_tables.sh		\
	multiple_table_path.pl			\
	ueb_test_data.pl

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
	squash_space				\
	compbrlAtCursor

# en-ueb-symbols_harness.yaml defines the desired behaviour.
# Apparently the tables aren't quite there yet.
XFAIL_TESTS += yaml/en-ueb-symbols_harness.yaml

# FIXME: According to Bue the Danish yaml tests need to be regenerated
# from scratch. They produce numerous failures with his new tables
XFAIL_TESTS +=					\
	yaml/da-dk-g26-dictionary_harness.yaml	\
	yaml/da-dk-g28-dictionary_harness.yaml


dist_yaml_TESTS =				\
	yaml/ar-ar-g1_harness.yaml		\
	yaml/ar-fa_harness.yaml			\
	yaml/begcaps_endcaps.yaml		\
	yaml/capsnocont.yaml			\
	yaml/chr-us-g1_harness.yaml		\
	yaml/da-dk-g08-back.yaml		\
	yaml/da-dk-g08.yaml			\
	yaml/da-dk-g16-dictionary_harness.yaml	\
	yaml/da-dk-g18-back.yaml		\
	yaml/da-dk-g18-dictionary_harness.yaml	\
	yaml/da-dk-g18.yaml			\
	yaml/da-dk-g26-dictionary_harness.yaml	\
	yaml/da-dk-g28-back.yaml		\
	yaml/da-dk-g28-dictionary_harness.yaml	\
	yaml/da-dk-g28l-back.yaml		\
	yaml/da-dk-g28l.yaml			\
	yaml/da-dk-g28.yaml			\
	yaml/el-backward.yaml			\
	yaml/el-forward.yaml			\
	yaml/emphasis-full.yaml			\
	yaml/emphasis-no-context.yaml		\
	yaml/en-gb-g1_harness.yaml		\
	yaml/en-GB-g2_backward.yaml		\
	yaml/en-GB-g2_harness.yaml		\
	yaml/en-ueb-02-stand_alone.yaml		\
	yaml/en-ueb-03-symbols.yaml		\
	yaml/en-ueb-05-grade_1_mode.yaml	\
	yaml/en-ueb-06-numeric_mode.yaml	\
	yaml/en-ueb-08-capitalization.yaml	\
	yaml/en-ueb-09-typeforms.yaml		\
	yaml/en-ueb-10-07-contractions.yaml	\
	yaml/en-ueb-10-13-contractions.yaml	\
	yaml/en-ueb-g1_backward.yaml		\
	yaml/en-ueb-g1_harness.yaml		\
	yaml/en-ueb-g2_backward.yaml		\
	yaml/en-ueb-g2_backward_no_dis.yaml	\
	yaml/en-ueb-g2-dictionary_harness.yaml	\
	yaml/en-ueb-math.yaml			\
	yaml/en-ueb-symbols_harness.yaml	\
	yaml/en-us-comp8-ext-back_harness.yaml	\
	yaml/en-us-comp8-ext-for_harness.yaml	\
	yaml/en-us-g2-dictionary_harness.yaml	\
	yaml/en-us-g2-end-emphasis_harness.yaml	\
	yaml/eo-g1_harness.yaml			\
	yaml/ethio-g1_harness.yaml		\
	yaml/example_test.yaml			\
	yaml/fa-ir-comp8-harness.yaml		\
	yaml/fa-ir-g1-harness.yaml		\
	yaml/fi_harness.yaml			\
	yaml/foo.yaml				\
	yaml/fr-bfu-g2_harness.yaml		\
	yaml/fr-fr-g1_backward.yaml		\
	yaml/fr-fr-g1_harness.yaml		\
	yaml/ga-g1_harness.yaml			\
	yaml/ga-g2_harness.yaml			\
	yaml/hr-8dots_harness.yaml		\
	yaml/hu-hu-comp8_backward.yaml		\
	yaml/hu-hu-comp8_harness.yaml		\
	yaml/hu-hu-g1_backward.yaml		\
	yaml/hu-hu-g1_harness.yaml		\
	yaml/iu-ca-g1_harness.yaml		\
	yaml/ko-2006-g2_harness.yaml		\
	yaml/ko-g2_harness.yaml			\
	yaml/letterDefTest_harness.yaml		\
	yaml/lt_harness.yaml			\
	yaml/mn-MN_harness.yaml			\
	yaml/multipass-backward.yaml		\
	yaml/multipass-forward.yaml		\
	yaml/multipass.yaml			\
	yaml/new_emph.yaml			\
	yaml/nl-g0_harness.yaml			\
	yaml/nl-NL-g0_harness.yaml		\
	yaml/no_8dot_harness.yaml		\
	yaml/nocross_harness.yaml		\
	yaml/no_g1_harness.yaml			\
	yaml/no_g2_harness.yaml			\
	yaml/no_harness.yaml			\
	yaml/noletsignafter.yaml		\
	yaml/pl-pl-comp8_harness.yaml		\
	yaml/sr-g1_harness.yaml			\
	yaml/ta-ta-g1_harness.yaml		\
	yaml/vi-g1_backward.yaml		\
	yaml/vi-g1_harness.yaml			\
	yaml/zh-chn.yaml				\
	yaml/issue-35.yaml				\
	yaml/issue-238.yaml

TEST_EXTENSIONS = .yaml
YAML_LOG_COMPILER =  $(top_builddir)/tools/lou_checkyaml

EXTRA_DIST = $(dist_yaml_TESTS)

XFAIL_TESTS += ueb_test_data.pl

TESTS =				\
	$(program_TESTS)	\
	$(dist_check_SCRIPTS)   \
	$(dist_yaml_TESTS)

AM_TESTS_ENVIRONMENT =								\
	LOUIS_TABLEPATH=$(abs_top_srcdir)					\
	UEB_TEST_DATA_PATH=$(top_srcdir)/tests/ueb_test_data			\
	PYTHONPATH=$(top_builddir)/python:$$PYTHONPATH				\
	LD_LIBRARY_PATH=$(top_builddir)/liblouis/.libs:$$LD_LIBRARY_PATH	\
	PATH=$(top_builddir)/tools:$$PATH
