SUBDIRS = tables tablesWithMetadata doctests harness harnessSources

AM_CPPFLAGS = -I$(top_srcdir)/liblouis

LDADD = $(top_builddir)/liblouis/liblouis.la

capitalization_SOURCES =	\
	brl_checks.c		\
	brl_checks.h		\
	capitalization.c

present_progressive_SOURCES =	\
	brl_checks.c		\
	brl_checks.h		\
	present_progressive.c

capitalized_word_SOURCES =	\
	brl_checks.c		\
	brl_checks.h		\
	capitalized_word.c

capitalized_with_sentance_SOURCES =		\
	brl_checks.c				\
	brl_checks.h				\
	capitalized_with_sentance.c

inpos_compbrl_SOURCES =		\
	brl_checks.c            \
	brl_checks.h            \
	inpos_compbrl.c	

lastworditalafter_SOURCES =	\
	brl_checks.c            \
	brl_checks.h            \
	lastworditalafter.c	

en_gb_g1_italics_SOURCES =	\
	brl_checks.c            \
	brl_checks.h            \
	en_gb_g1_italics.c	

pass2_SOURCES =			\
	brl_checks.c		\
	brl_checks.h		\
	pass2.c	

pass2_inpos_SOURCES =		\
	brl_checks.c		\
	brl_checks.h		\
	pass2_inpos.c	

inpos_SOURCES =			\
	brl_checks.c		\
	brl_checks.h		\
	inpos.c	

inpos_match_replace_SOURCES =	\
	brl_checks.c		\
	brl_checks.h		\
	inpos_match_replace.c

letterDefTest_SOURCES = 	\
	brl_checks.c		\
	brl_checks.h		\
	letterDefTest.c

uplow_with_unicode_SOURCES = 	\
	brl_checks.c		\
	brl_checks.h		\
	uplow_with_unicode.c

squash_space_SOURCES =				\
	brl_checks.c				\
	brl_checks.h				\
	squash_space.c

hyphenate_achena_SOURCES  = 		\
	brl_checks.c			\
	brl_checks.h			\
	hyphenate_achena.c

hyphenate_alderen_SOURCES  = 		\
	brl_checks.c			\
	brl_checks.h			\
	hyphenate_alderen.c

hyphenate_straightforward_SOURCES  =		\
	brl_checks.c				\
	brl_checks.h				\
	hyphenate_straightforward.c

hyphenate_xxx_SOURCES =				\
	brl_checks.c				\
	brl_checks.h				\
	hyphenate_xxx.c

backtranslate_with_letsign_SOURCES =		\
	brl_checks.c				\
	brl_checks.h				\
	backtranslate_with_letsign.c

backtranslate_SOURCES =				\
	brl_checks.c				\
	brl_checks.h				\
	backtranslate.c

pass1Only_SOURCES =				\
	brl_checks.c				\
	brl_checks.h				\
	pass1Only.c

outpos_SOURCES =				\
	brl_checks.c				\
	brl_checks.h				\
	outpos.c	

getTable_SOURCES = getTable.c

typeform_SOURCES =				\
	typeform.c

pass0_typebuf_SOURCES =				\
	brl_checks.c				\
	brl_checks.h				\
	pass0_typebuf.c

hash_collision_SOURCES =			\
	brl_checks.c				\
	brl_checks.h				\
	hash_collision.c

resolve_table_SOURCES =				\
	resolve_table.c

logging_SOURCES =				\
	logging.c

findTable_SOURCES =				\
	findTable.c

emphclass_SOURCES =	\
	brl_checks.c		\
	brl_checks.h		\
	emphclass.c

check_yaml_SOURCES = 				\
	brl_checks.c				\
	brl_checks.h				\
	check_yaml.c

# Note that this is not currently included in check_programs below
# otherwise make check would never complete.
infiniteTranslationLoop_SOURCES =				\
	brl_checks.c				\
	brl_checks.h				\
	infiniteTranslationLoop.c

program_TESTS =					\
	capitalization                          \
	pass2					\
	pass2_inpos				\
	inpos					\
	inpos_match_replace			\
	uplow_with_unicode			\
	present_progressive			\
	capitalized_word			\
	capitalized_with_sentance		\
	inpos_compbrl				\
	lastworditalafter			\
	en_gb_g1_italics			\
	letterDefTest				\
	squash_space				\
	hyphenate_achena			\
	hyphenate_alderen			\
	hyphenate_straightforward		\
	hyphenate_xxx				\
	backtranslate_with_letsign 		\
	backtranslate 				\
	pass1Only				\
	outpos					\
	getTable				\
	typeform				\
	pass0_typebuf				\
	hash_collision                          \
	resolve_table                          	\
	logging                          	\
	findTable                          	\
	emphclass

check_PROGRAMS = $(program_TESTS) check_yaml

dist_check_SCRIPTS =		\
	check_all_tables.pl	\
	check_endless_loop.pl	\
	multiple_table_path.pl  \
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
	squash_space

# en-ueb-symbols_harness.yaml defines the desired behaviour.
# Apparently the tables aren't quite there yet.
XFAIL_TESTS += en-ueb-symbols_harness.yaml

dist_yaml_TESTS =					\
	yaml/ar-ar-g1_harness.yaml			\
	yaml/ar-fa_harness.yaml				\
	yaml/chr-us-g1_harness.yaml			\
	yaml/da-dk-g16-dictionary_harness.yaml		\
	yaml/da-dk-g18-dictionary_harness.yaml		\
	yaml/da-dk-g26-dictionary_harness.yaml		\
	yaml/da-dk-g28-dictionary_harness.yaml		\
	yaml/en-GB-g2_backward.yaml			\
	yaml/en-GB-g2_harness.yaml			\
	yaml/en-gb-g1_harness.yaml			\
	yaml/en-ueb-g1_harness.yaml			\
	yaml/en-ueb-g2-dictionary_harness.yaml		\
	yaml/en-ueb-symbols_harness.yaml		\
	yaml/en-us-g2-dictionary_harness.yaml		\
	yaml/en-us-g2-end-emphasis_harness.yaml		\
	yaml/eo-g1_harness.yaml				\
	yaml/ethio-g1_harness.yaml			\
	yaml/example_test.yaml				\
	yaml/fi_harness.yaml				\
	yaml/foo.yaml					\
	yaml/fr-bfu-g2_harness.yaml			\
	yaml/ga-g1_harness.yaml				\
	yaml/ga-g2_harness.yaml				\
	yaml/hr-8dots_harness.yaml			\
	yaml/hu-hu-comp8_backward.yaml			\
	yaml/hu-hu-comp8_harness.yaml			\
	yaml/hu-hu-g1_backward.yaml			\
	yaml/hu-hu-g1_harness.yaml			\
	yaml/iu-ca-g1_harness.yaml			\
	yaml/ko-2006-g2_harness.yaml			\
	yaml/ko-g2_harness.yaml				\
	yaml/letterDefTest_harness.yaml			\
	yaml/mn-MN_harness.yaml				\
	yaml/nl-BE-g0_harness.yaml			\
	yaml/nl-BE-g1_harness.yaml			\
	yaml/nl-NL-g0_harness.yaml			\
	yaml/no_harness.yaml				\
	yaml/no_g1_harness.yaml				\
	yaml/no_g2_harness.yaml				\
	yaml/no_8dot_harness.yaml			\
	yaml/nocross_harness.yaml			\
	yaml/pl-pl-comp8_harness.yaml			\
	yaml/sr-g1_harness.yaml				\
	yaml/ta-ta-g1_harness.yaml			\
	yaml/en-ueb-g1_harness.txt.bak.yaml		\
	yaml/en-ueb-g2-dictionary_harness.txt.bak.yaml	\
	yaml/en-us-g2-dictionary_harness.txt.bak.yaml	\
	yaml/en-us-g2-end-emphasis_harness.txt.bak.yaml	\
	yaml/ethio-g1_harness.txt.bak.yaml		\
	yaml/ko-2006-g2_harness.txt.bak.yaml		\
	yaml/ko-g2_harness.txt.bak.yaml			\
	yaml/nl-BE-g1_harness.txt.bak.yaml		\
	yaml/ta-ta-g1_harness.txt.bak.yaml

TEST_EXTENSIONS = .yaml
YAML_LOG_COMPILER = ./check_yaml

EXTRA_DIST = $(dist_yaml_TESTS)

XFAIL_TESTS += ueb_test_data.pl

TESTS =				\
	$(program_TESTS)	\
	$(dist_check_SCRIPTS)   \
	$(dist_yaml_TESTS)

if HAVE_PYTHON
if HAVE_UCS4
dist_emphasis_TESTS = new_caps_behavior
EXTRA_DIST += $(dist_emphasis_TESTS)
TESTS += $(dist_emphasis_TESTS)
endif
endif

TESTS_ENVIRONMENT =																	\
	LOUIS_TABLEPATH=$(top_srcdir)/tables,$(top_srcdir)/tests/tables,$(top_srcdir)/tests/tables/moreTables,$(top_srcdir)/tests/tablesWithMetadata,	\
	PYTHONPATH=$(HARNESS_DIR):$(top_builddir)/python:$$PYTHONPATH											\
	HARNESS_DIR=$(HARNESS_DIR)															\
	LD_LIBRARY_PATH=$(top_builddir)/liblouis/.libs:$$LD_LIBRARY_PATH										\
	PATH=$(top_builddir)/tools:$$PATH
