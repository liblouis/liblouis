INCLUDES =		\
	-I$(top_srcdir) -I$(top_srcdir)/liblouis

LDADD = $(top_builddir)/liblouis/liblouis.la

present_progressive_SOURCES =	\
	brl_checks.c		\
	brl_checks.h		\
	present_progressive.c

capitalized_word_SOURCES =		\
	brl_checks.c		\
	brl_checks.h		\
	capitalized_word.c

capitalized_with_sentance_SOURCES =	\
	brl_checks.c            \
    brl_checks.h            \
	capitalized_with_sentance.c

inpos_compbrl_SOURCES =\
	brl_checks.c            \
    brl_checks.h            \
	inpos_compbrl.c	

TESTS =	\
	present_progressive \
	capitalized_word \
	capitalized_with_sentance \
	inpos_compbrl

noinst_PROGRAMS = 		\
	present_progressive \
	capitalized_word \
	capitalized_with_sentance \
	inpos_compbrl
