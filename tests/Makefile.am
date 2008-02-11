INCLUDES =		\
	-I$(top_srcdir) -I$(top_srcdir)/liblouis

LDADD = $(top_builddir)/liblouis/liblouis.la

present_progressive_SOURCES =	\
	comp_brl_at_cursor.c		\
	comp_brl_at_cursor.h		\
	present_progressive.c

capitalized_word_SOURCES =		\
	comp_brl_at_cursor.c		\
	comp_brl_at_cursor.h		\
	capitalized_word.c

TESTS =	\
	present_progressive \
	capitalized_word

noinst_PROGRAMS = 		\
	present_progressive \
	capitalized_word