SUBDIRS = table_test_corpuses tables

AM_CPPFLAGS = -I$(top_srcdir)/liblouis

LDADD = $(top_builddir)/liblouis/liblouis.la

present_progressive_SOURCES =	\
	brl_checks.c		\
	brl_checks.h		\
	present_progressive.c

capitalized_word_SOURCES =	\
	brl_checks.c		\
	brl_checks.h		\
	capitalized_word.c

capitalized_with_sentance_SOURCES =	\
	brl_checks.c            \
	brl_checks.h            \
	capitalized_with_sentance.c

inpos_compbrl_SOURCES =		\
	brl_checks.c            \
	brl_checks.h            \
	inpos_compbrl.c	

lastworditalafter_SOURCES =	\
	brl_checks.c            \
	brl_checks.h            \
	lastworditalafter.c	

check_PROGRAMS = 		\
	present_progressive 	\
	capitalized_word 	\
	capitalized_with_sentance \
	inpos_compbrl		\
	lastworditalafter

dist_check_SCRIPTS = \
	check_all_tables.pl \
	check_endless_loop.pl \
	check_tables_against_corpus.pl \
	multiple_table_path.pl

# The present_progressive should not really fail but for the moment
# the issue doesn't seem to be critical enough that anybody has enough
# interest to fix it.
XFAIL_TESTS = \
	present_progressive \
	lastworditalafter

TESTS =	\
	$(check_PROGRAMS) \
	$(dist_check_SCRIPTS)

TESTS_ENVIRONMENT = LOUIS_TABLEPATH=$(top_srcdir)/tables,$(top_srcdir)/tests/tables PATH=../tools:$$PATH
