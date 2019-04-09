SUBDIRS = tables tablesWithMetadata yaml braille-specs ueb_test_data

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

checkTable_SOURCES = checkTable.c
emphclass_SOURCES = emphclass.c
findTable_SOURCES = findTable.c
getTable_SOURCES = getTable.c
hash_collision_SOURCES = hash_collision.c
hyphenate_xxx_SOURCES = hyphenate_xxx.c
logging_SOURCES = logging.c
resolve_table_SOURCES = resolve_table.c
typeform_SOURCES = typeform.c
typeform_for_emphclass_SOURCES = typeform_for_emphclass.c
check_metadata_SOURCES = check_metadata.c
suggestChunks_SOURCES = suggestChunks.c

program_TESTS =					\
	checkTable				\
	check_metadata				\
	emphclass				\
	findTable				\
	getTable				\
	hash_collision				\
	hyphenate_xxx				\
	logging					\
	resolve_table				\
	typeform				\
	typeform_for_emphclass			\
	suggestChunks

check_PROGRAMS = $(program_TESTS)

dist_check_SCRIPTS =

# The test scripts aren't really set up to work in a cross compilation
if !CROSS_COMPILING
dist_check_SCRIPTS +=	 			\
	check_all_tables.pl			\
	check_endless_loop.pl			\
	multiple_table_path.pl			\
	ueb_test_data.pl
endif

dist_braille_specs_TESTS =				  \
	braille-specs/ar-ar-comp8.yaml			  \
	braille-specs/ar-ar-g1.yaml			  \
	braille-specs/ar-ar-g1_harness.yaml		  \
	braille-specs/ar-ar-g2.yaml			  \
	braille-specs/chr-us-g1_harness.yaml		  \
	braille-specs/cs_harness.yaml			  \
	braille-specs/cs-comp8_harness.yaml		  \
	braille-specs/da-dk-g08.yaml			  \
	braille-specs/da-dk-g16.yaml			  \
	braille-specs/da-dk-g16-dictionary_harness.yaml	  \
	braille-specs/da-dk-g16-lit.yaml		  \
	braille-specs/da-dk-g18-dictionary_harness.yaml	  \
	braille-specs/da-dk-g18.yaml			  \
	braille-specs/da-dk-g26.yaml			  \
	braille-specs/da-dk-g26-dictionary_harness.yaml	  \
	braille-specs/da-dk-g26-lit.yaml		  \
	braille-specs/da-dk-g26l.yaml			  \
	braille-specs/da-dk-g26l-lit.yaml		  \
	braille-specs/da-dk-g28-dictionary_harness.yaml	  \
	braille-specs/da-dk-g28.yaml			  \
	braille-specs/da-dk-g28l.yaml			  \
	braille-specs/de-ch-g1-dictionary.yaml		  \
	braille-specs/de-ch-g2-dictionary.yaml		  \
	braille-specs/de-ch-g1.yaml			  \
	braille-specs/el-backward.yaml			  \
	braille-specs/el-forward.yaml			  \
	braille-specs/en-GB-g2_backward.yaml		  \
	braille-specs/en-GB-g2_harness.yaml		  \
	braille-specs/en-gb-g1_harness.yaml		  \
	braille-specs/en-ueb-02-stand_alone.yaml	  \
	braille-specs/en-ueb-03-symbols.yaml		  \
	braille-specs/en-ueb-05-grade_1_mode.yaml	  \
	braille-specs/en-ueb-06-numeric_mode.yaml	  \
	braille-specs/en-ueb-08-capitalization.yaml	  \
	braille-specs/en-ueb-09-typeforms.yaml		  \
	braille-specs/en-ueb-10-07-contractions.yaml	  \
	braille-specs/en-ueb-10-13-contractions.yaml	  \
	braille-specs/en-ueb-g1_backward.yaml		  \
	braille-specs/en-ueb-g1_harness.yaml		  \
	braille-specs/en-ueb-g2-dictionary_harness.yaml	  \
	braille-specs/en-ueb-g2_backward.yaml		  \
	braille-specs/en-ueb-g2_backward_no_dis.yaml	  \
	braille-specs/en-ueb-math.yaml			  \
	braille-specs/en-ueb-symbols_harness.yaml	  \
	braille-specs/en-ueb-michael-robin.yaml		  \
	braille-specs/en-us-comp8-ext-back_harness.yaml	  \
	braille-specs/en-us-comp8-ext-for_harness.yaml	  \
	braille-specs/en-us-g2.yaml			  \
	braille-specs/en-us-g2-dictionary_harness.yaml	  \
	braille-specs/en-us-g2-end-emphasis_harness.yaml  \
	braille-specs/eo-g1_harness.yaml		  \
	braille-specs/es-g2.yaml			  \
	braille-specs/ethio-g1_harness.yaml		  \
	braille-specs/fa-ir-comp8-harness.yaml		  \
	braille-specs/fa-ir-g1-harness.yaml		  \
	braille-specs/fi_harness.yaml			  \
	braille-specs/fr-bfu-comp6.yaml			  \
	braille-specs/fr-bfu-comp8.yaml			  \
	braille-specs/fr-bfu-g2.yaml			  \
	braille-specs/fr-bfu-g2_harness.yaml		  \
	braille-specs/ga-g1_harness.yaml		  \
	braille-specs/ga-g2_harness.yaml		  \
	braille-specs/hi_harness.yaml			  \
	braille-specs/hr-8dots_harness.yaml		  \
	braille-specs/hu-hu-comp8_backward.yaml		  \
	braille-specs/hu-hu-comp8_harness.yaml		  \
	braille-specs/hu-hu-g1_backward.yaml		  \
	braille-specs/hu-hu-g1_harness.yaml		  \
	braille-specs/hu-hu-g2_harness.yaml \
	braille-specs/hu-hu-g2_backward.yaml \
	braille-specs/iu-ca-g1_harness.yaml		  \
	braille-specs/ko-2006-g2_harness.yaml		  \
	braille-specs/ko-g2_harness.yaml		  \
	braille-specs/litdigits6Dots_backward.yaml	  \
	braille-specs/lt_harness.yaml			  \
	braille-specs/lt-6dot_harness.yaml		  \
	braille-specs/lv_harness.yaml			  \
	braille-specs/mn-MN_harness.yaml		  \
	braille-specs/nl-NL-g0_harness.yaml		  \
	braille-specs/nl-g0_harness.yaml		  \
	braille-specs/no_8dot_harness.yaml		  \
	braille-specs/no_backward.yaml			  \
	braille-specs/no_g1_harness.yaml		  \
	braille-specs/no_g2_harness.yaml		  \
	braille-specs/no_harness.yaml			  \
	braille-specs/pl-pl-comp8_harness.yaml		  \
	braille-specs/Pl-Pl-g1_backward.yaml		  \
	braille-specs/sk-g1_harness.yaml		  \
	braille-specs/sl-g1.yaml			  \
	braille-specs/spaces.yaml			  \
	braille-specs/sr-g1_harness.yaml		  \
	braille-specs/ta-ta-g1_harness.yaml		  \
	braille-specs/uk.yaml				  \
	braille-specs/ur-pk-g2.yaml			  \
	braille-specs/vi-g1_backward.yaml		  \
	braille-specs/vi-g1_harness.yaml		  \
	braille-specs/zh-chn.yaml			  \
	braille-specs/zh-tw.yaml			  \
	braille-specs/zhcn-g1.yaml			  \
	braille-specs/zhcn-g2.yaml

# This test hangs when configured without --enable-ucs4
if HAVE_UCS4
dist_braille_specs_TESTS +=				  \
	braille-specs/hu-hu-g1-hyph_harness.yaml
endif

dist_yaml_TESTS =				\
	yaml/back_cont_then_punc.yaml		\
	yaml/begcaps_endcaps.yaml		\
	yaml/before_begmidword.yaml		\
	yaml/broken_equals_operand.yaml		\
	yaml/capitalization.yaml		\
	yaml/capsnocont.yaml			\
	yaml/capsword_for_back.yaml		\
	yaml/compbrlAtCursor_with_equals.yaml	\
	yaml/computer_braille.yaml		\
	yaml/emphasis-full.yaml			\
	yaml/emphasis-no-context.yaml		\
	yaml/example_test.yaml			\
	yaml/foo.yaml				\
	yaml/hyphenation_nocross_harness.yaml	\
	yaml/inpos_outpos.yaml			\
	yaml/issue-238.yaml			\
	yaml/issue-35.yaml			\
	yaml/issue-384.yaml			\
	yaml/issue-400.yaml			\
	yaml/issue-401.yaml			\
	yaml/issue-406.yaml			\
	yaml/issue-412.yaml			\
	yaml/issue-479.yaml			\
	yaml/issue-498.yaml			\
	yaml/issue-537.yaml			\
	yaml/issue-583.yaml			\
	yaml/issue-584.yaml			\
	yaml/issue-615.yaml			\
	yaml/issue-77.yaml			\
	yaml/letterDefTest_harness.yaml		\
	yaml/multipass-backward.yaml		\
	yaml/multipass-forward.yaml		\
	yaml/multipass.yaml			\
	yaml/multipass-negation.yaml		\
	yaml/multipass-vs-match.yaml		\
	yaml/new_emph.yaml			\
	yaml/noletsignafter.yaml		\
	yaml/noUndefinedDots.yaml		\
	yaml/partialTrans.yaml			\
	yaml/pass0_typebuf.yaml			\
	yaml/present_progressive.yaml		\
	yaml/squash_space.yaml			\
	yaml/syllable.yaml

YAML_LOG_COMPILER_SCRIPT = yaml-log-compiler.sh
TEST_EXTENSIONS = .yaml
YAML_LOG_COMPILER = $(top_srcdir)/tests/$(YAML_LOG_COMPILER_SCRIPT)
LOG_COMPILE = $(WINE)

EXTRA_DIST = $(dist_yaml_TESTS) $(dist_braille_specs_TESTS) $(YAML_LOG_COMPILER_SCRIPT)

XFAIL_TESTS =					\
	ueb_test_data.pl

TESTS =				     \
	$(program_TESTS)	     \
	$(dist_check_SCRIPTS)	     \
	$(dist_braille_specs_TESTS)  \
	$(dist_yaml_TESTS)

AM_TESTS_ENVIRONMENT =								\
	LOUIS_TABLEPATH=$(abs_top_srcdir)					\
	UEB_TEST_DATA_PATH=$(top_srcdir)/tests/ueb_test_data			\
	LD_LIBRARY_PATH=$(top_builddir)/liblouis/.libs:$$LD_LIBRARY_PATH	\
	PATH=$(top_builddir)/tools:$$PATH					\
	WINE=$(WINE)								\
	EXEEXT=$(EXEEXT)
