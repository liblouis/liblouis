/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2008 Eitan Isaacson <eitan@ascender.com>
Copyright (C) 2010, 2012, 2013 Swiss Library for the Blind, Visually Impaired and Print Disabled
Copyright (C) 2012 James Teh <jamie@nvaccess.org>
Copyright (C) 2014 Mesar Hameed <mesar.hameed@gmail.com>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#define TRANSLATION_TABLE "compress.cti,en-us-g2.ctb"

int check_inpos(const char *tableList, const char *str, const int *expected_poslist);

int check_outpos(const char *tableList, const char *str, const int *expected_poslist);

/* Check if the cursor position is where you expect it to be after
   translating str. Return 0 if the translation is as expected and 1
   otherwise. */
int check_cursor_pos(const char *str, const int *expected_pos);

/* Check if a string is translated as expected. Return 0 if the
   translation is as expected and 1 otherwise. */
int check_translation(const char *tableList, const char *str,
		      const char *typeform, const char *expected);

/* Check if a string is translated as expected. Return 0 if the
   translation is as expected and 1 otherwise. */
int check_translation_with_mode(const char *tableList, const char *str,
				const char *typeform, const char *expected, int mode);

/* Check if a string is backtranslated as expected. Return 0 if the
   backtranslation is as expected and 1 otherwise. */
int check_backtranslation(const char *tableList, const char *str,
		      const char *typeform, const char *expected);

/* Check if a string is backtranslated as expected. Return 0 if the
   backtranslation is as expected and 1 otherwise. */
int check_backtranslation_with_mode(const char *tableList, const char *str,
				const char *typeform, const char *expected, int mode);

/* Check if a string is hyphenated as expected. Return 0 if the
   hyphenation is as expected and 1 otherwise. */
int check_hyphenation(const char *tableList, const char *str, const char *expected);

/* Helper function to convert a typeform string of '0's, '1's, '2's etc.
   to the required format, which is an array of 0s, 1s, 2s, etc.
   For example, "0000011111000" is converted to {0,0,0,0,0,1,1,1,1,1,0,0,0}
   The caller is responsible for freeing the returned array. */
char * convert_typeform(const char* typeform_string);

