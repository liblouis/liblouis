SUBDIRS = tables tablesWithMetadata yaml braille-specs fuzzing

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
attributeNames_SOURCES = attributeNames.c
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
charToFallbackDots_SOURCES = charToFallbackDots.c

program_TESTS =					\
	checkTable				\
	check_metadata				\
	emphclass				\
	attributeNames				\
	findTable				\
	getTable				\
	hash_collision				\
	hyphenate_xxx				\
	logging					\
	resolve_table				\
	typeform				\
	typeform_for_emphclass			\
	suggestChunks				\
	charToFallbackDots

check_PROGRAMS = $(program_TESTS)

dist_check_SCRIPTS =

# The test scripts aren't really set up to work in a cross compilation
if !CROSS_COMPILING
dist_check_SCRIPTS +=	 			\
	check_all_tables.pl			\
	check_endless_loop.pl			\
	multiple_table_path.pl
endif

dist_braille_specs_TESTS =				  \
	braille-specs/afr-za-g2.yaml			  \
	braille-specs/ar-ar-comp8.yaml			  \
	braille-specs/ar-ar-g1.yaml			  \
	braille-specs/ar-ar-g1_harness.yaml		  \
	braille-specs/ar-ar-g2.yaml			  \
	braille-specs/ba.yaml				  \
	braille-specs/bel.yaml				  \
	braille-specs/bg.yaml				  \
	braille-specs/bn.yaml				  \
	braille-specs/chr-us-g1_harness.yaml		  \
	braille-specs/cop.yaml				  \
	braille-specs/critical-apparatus.yaml		  \
	braille-specs/cs_harness.yaml			  \
	braille-specs/cs-comp8_harness.yaml		  \
	braille-specs/da-dk_1993.yaml			  \
	braille-specs/da-dk-6dot.yaml			  \
	braille-specs/da-dk-8dot.yaml			  \
	braille-specs/da-dk-g26-dictionary_harness_1993.yaml	  \
	braille-specs/da-dk-g26-dictionary_harness.yaml	  \
	braille-specs/da-dk-g28-dictionary_harness_1993.yaml	  \
	braille-specs/da-dk-g28-dictionary_harness.yaml	  \
	braille-specs/de-blista-dictionary.yaml	  	\
	braille-specs/de-comp6.yaml			  \
	braille-specs/de-de-comp8.yaml 			  \
	braille-specs/de-eurobrl6.yaml			  \
	braille-specs/de-g0-detailed-dictionary.yaml	  \
	braille-specs/de-g0-detailed-specs.yaml		  \
	braille-specs/de-g0.yaml			  \
	braille-specs/de-g1-detailed-dictionary.yaml	  \
	braille-specs/de-g1-detailed-specs.yaml		  \
	braille-specs/de-g1.yaml			  \
	braille-specs/de-g1-dictionary.yaml		  \
	braille-specs/de-g2.yaml			  \
	braille-specs/de-g2-sbs.yaml			  \
	braille-specs/de-g2-detailed-specs.yaml		  \
	braille-specs/de-g2-dictionary.yaml		  \
	braille-specs/el-backward.yaml			  \
	braille-specs/el-forward.yaml			  \
	braille-specs/en-g3.yaml		  \
	braille-specs/en-g3-dictionary.yaml		  \
	braille-specs/en-GB-g2.yaml			  \
	braille-specs/en-gb-g1_harness.yaml		  \
	braille-specs/en-nabcc.yaml			  \
	braille-specs/en-ueb.yaml	  		  \
	braille-specs/en-ueb-g1_backward.yaml		  \
	braille-specs/en-ueb-g1_harness.yaml		  \
	braille-specs/en-ueb-g2-dictionary_harness.yaml	  \
	braille-specs/en-ueb-g2_backward.yaml		  \
	braille-specs/en-ueb-g2_backward_no_dis.yaml	  \
	braille-specs/en-ueb-computer-code.yaml		  \
	braille-specs/en-ueb-math.yaml			  \
	braille-specs/en-ueb-symbols_harness.yaml	  \
	braille-specs/en-us-comp6.yaml			  \
	braille-specs/en-us-comp8-ext-back_harness.yaml	  \
	braille-specs/en-us-comp8-ext-for_harness.yaml	  \
	braille-specs/en-us-emphasis_harness.yaml	  \
	braille-specs/en-us-g1.yaml			  \
	braille-specs/en-us-g2.yaml			  \
	braille-specs/en-us-g2-dictionary_harness.yaml	  \
	braille-specs/eo-g1_harness.yaml		  \
	braille-specs/es-g0-g1.yaml			  \
	braille-specs/es-g2.yaml			  \
	braille-specs/ethio-g1_harness.yaml		  \
	braille-specs/fa-ir-comp8-harness.yaml		  \
	braille-specs/fa-ir-g1-harness.yaml		  \
	braille-specs/fi_harness.yaml			  \
	braille-specs/fil.yaml				  \
	braille-specs/fr-bfu-comp6.yaml			  \
	braille-specs/fr-bfu-comp8.yaml			  \
	braille-specs/fr-bfu-g2.yaml			  \
	braille-specs/fr-bfu-g2_harness.yaml		  \
	braille-specs/ga-g1_harness.yaml		  \
	braille-specs/ga-g2_harness.yaml		  \
	braille-specs/grc-international-common.yaml	  \
	braille-specs/grc-international-decomposed.yaml	  \
	braille-specs/grc-international-es.yaml	  \
	braille-specs/he-IL.yaml			  \
	braille-specs/hi_harness.yaml			  \
	braille-specs/hr-8dots_harness.yaml		  \
	braille-specs/hu-hu-comp8_harness.yaml		  \
	braille-specs/hu-hu-g1_harness.yaml		  \
	braille-specs/hu-hu-g1_braille_input_backward.yaml\
	braille-specs/hu-hu-g1_braille_input_forward.yaml \
	braille-specs/hu-hu-g1_dictionary_numbers.yaml		  \
	braille-specs/hu-hu-g1_dictionary_special_consonants.yaml		  \
	braille-specs/hu-hu-g2_harness.yaml 		  \
	braille-specs/hu-hu-g2_dictionary_numbers.yaml		  \
	braille-specs/ipa.yaml				  \
	braille-specs/it.yaml				  \
	braille-specs/iu-ca-g1_harness.yaml		  \
	braille-specs/ja-kantenji.yaml			  \
	braille-specs/ka.yaml				  \
	braille-specs/kk.yaml				  \
	braille-specs/kmr.yaml				  \
	braille-specs/ko-2006-g2_harness.yaml		  \
	braille-specs/ko-g2_harness.yaml		  \
	braille-specs/litdigits6Dots_backward.yaml	  \
	braille-specs/lg-ug-g1.yaml			  \
	braille-specs/lo.yaml				  \
	braille-specs/lt.yaml				  \
	braille-specs/lv_harness.yaml			  \
	braille-specs/mk.yaml				  \
	braille-specs/ml.yaml				  \
	braille-specs/mn-MN_harness.yaml		  \
	braille-specs/ms-my-g2.yaml			  \
	braille-specs/my-g2.yaml			  \
	braille-specs/nemeth.yaml			  \
	braille-specs/nl-comp8_harness.yaml		  \
	braille-specs/nl-g0_harness.yaml		  \
	braille-specs/no.yaml				  \
	braille-specs/ny-mw.yaml			  \
	braille-specs/pa.yaml				  \
	braille-specs/pl-g1.yaml			  \
	braille-specs/pl-pl-comp8_harness.yaml		  \
	braille-specs/pt.yaml		 		  \
	braille-specs/ro-g0.yaml			  \
	braille-specs/ru.yaml				  \
	braille-specs/rw-rw-g1.yaml			  \
	braille-specs/sah.yaml				  \
	braille-specs/sk-g1_harness.yaml		  \
	braille-specs/sl-g1.yaml			  \
	braille-specs/spaces.yaml			  \
	braille-specs/sr.yaml				  \
	braille-specs/st-g2.yaml			  \
	braille-specs/sv.yaml				  \
	braille-specs/sw-ke-dictionary.yaml		  \
	braille-specs/sw-ke.yaml			  \
	braille-specs/ta-ta-g1_harness.yaml		  \
	braille-specs/th.yaml				  \
	braille-specs/tr.yaml				  \
	braille-specs/tt.yaml				  \
	braille-specs/uk.yaml				  \
	braille-specs/ur-pk-g2.yaml			  \
	braille-specs/uz.yaml				  \
	braille-specs/ve-g2.yaml			  \
	braille-specs/vi.yaml				  \
	braille-specs/xh-g2.yaml			  \
	braille-specs/yi.yaml				  \
	braille-specs/zh-chn.yaml			  \
	braille-specs/zh-tw.yaml			  \
	braille-specs/zhcn-cbs.yaml			  \
	braille-specs/zhcn-g1.yaml			  \
	braille-specs/zhcn-g2.yaml

if HAVE_UCS4
# These tests only work when configured with --enable-ucs4
dist_braille_specs_TESTS +=				\
	braille-specs/akk.yaml				\
	braille-specs/akk-borger.yaml			\
	braille-specs/cuneiform-transliterated.yaml	\
	braille-specs/grc-international-composed.yaml	\
	braille-specs/hbo.yaml				\
	braille-specs/hu-hu-g1-hyph_harness.yaml	\
	braille-specs/ja-rokutenkanji.yaml		\
	braille-specs/syc.yaml				\
	braille-specs/uga.yaml
endif

dist_yaml_TESTS =				\
	yaml/attribute.yaml			\
	yaml/back_cont_then_punc.yaml		\
	yaml/begcaps_endcaps.yaml		\
	yaml/before_begmidword.yaml		\
	yaml/broken_equals_operand.yaml		\
	yaml/capitalization.yaml		\
	yaml/capsnocont.yaml			\
	yaml/capsword.yaml			\
	yaml/case-sensitivity.yaml		\
	yaml/compbrlAtCursor_with_equals.yaml	\
	yaml/computer_braille.yaml		\
	yaml/emphasis.yaml			\
	yaml/example_test.yaml			\
	yaml/face-with-tears-of-joy.yaml        \
	yaml/issue-1863-basechar-finalize.yaml	\
	yaml/hyphenation.yaml			\
	yaml/hyphenation_nocross_harness.yaml	\
	yaml/inpos_outpos.yaml			\
	yaml/input-length.yaml			\
	yaml/issue-332.yaml			\
	yaml/issue-479.yaml			\
	yaml/issue-615.yaml			\
	yaml/issue-963.yaml			\
	yaml/letterDefTest_harness.yaml		\
	yaml/match-vs-always.yaml		\
	yaml/multipass-backward.yaml		\
	yaml/multipass-forward.yaml		\
	yaml/multipass.yaml			\
	yaml/multipass-negation.yaml		\
	yaml/multipass-vs-match.yaml		\
	yaml/new_emph.yaml			\
	yaml/nonumsign.yaml			\
	yaml/numericmode.yaml			\
	yaml/partialTrans.yaml			\
	yaml/pass0_typebuf.yaml			\
	yaml/precedence.yaml			\
	yaml/present_progressive.yaml		\
	yaml/repword.yaml			\
	yaml/squash_space.yaml			\
	yaml/sufword_prfword.yaml		\
	yaml/syllable.yaml			\
	yaml/undefined.yaml			\
	yaml/various-regression-tests.yaml

# Yaml tests that require ucs4
if HAVE_UCS4
dist_yaml_TESTS +=				\
	yaml/face-with-tears-of-joy-ucs4.yaml
endif

# Tests for macros
if ENABLE_MACROS
dist_yaml_TESTS +=				\
	yaml/macro.yaml
endif

YAML_LOG_COMPILER_SCRIPT = yaml-log-compiler.sh
TEST_EXTENSIONS = .yaml
YAML_LOG_COMPILER = $(top_srcdir)/tests/$(YAML_LOG_COMPILER_SCRIPT)
LOG_COMPILE = $(WINE)

EXTRA_DIST = $(dist_yaml_TESTS) $(dist_braille_specs_TESTS) $(YAML_LOG_COMPILER_SCRIPT)

TESTS =				     \
	$(program_TESTS)	     \
	$(dist_check_SCRIPTS)	     \
	$(dist_braille_specs_TESTS)  \
	$(dist_yaml_TESTS)

AM_TESTS_ENVIRONMENT =								\
	LOUIS_TABLEPATH=$(abs_top_srcdir)					\
	LD_LIBRARY_PATH=$(top_builddir)/liblouis/.libs:$$LD_LIBRARY_PATH	\
	PATH=$(top_builddir)/tools:$$PATH					\
	WINEPATH=$(top_builddir)/tools						\
	WINE=$(WINE)								\
	EXEEXT=$(EXEEXT)
