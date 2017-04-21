/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2012 James Teh <jamie@nvaccess.org>
Copyright (C) 2014 Mesar Hameed <mesar.hameed@gmail.com>
Copyright (C) 2015 Mike Gray <mgray@aph.org>
Copyright (C) 2010-2016 Swiss Library for the Blind, Visually Impaired and Print Disabled
Copyright (C) 2016-2017 Davy Kager <mail@davykager.nl>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

/* Functionality to check a translation. This is mostly needed for the
   tests in ../tests but it is also needed for lou_checkyaml. So this
   functionality is packaged up in what automake calls a convenience
   library, a lib that is solely built at compile time but never
   installed. */

int check_inpos(const char *tableList, const char *str, const int *expected_poslist);

int check_outpos(const char *tableList, const char *str, const int *expected_poslist);

/* Check if the cursor position is where you expect it to be after
   translating str. Return 0 if the translation is as expected and 1
   otherwise. */
int check_cursor_pos(const char *tableList, const char *str, const int *expected_pos);

/* Check if a string is translated as expected. Return 0 if the
   translation is as expected and 1 otherwise. */
int check_translation(const char *tableList, const char *str,
                      const formtype *typeform, const char *expected);

/* Check if a string is translated as expected. Return 0 if the
   translation is as expected and 1 otherwise. */
int check_translation_with_mode(const char *tableList, const char *str,
                                const formtype *typeform, const char *expected,
                                int mode);

/* Check if a string is translated as expected. Return 0 if the
   translation is as expected and 1 otherwise. */
int check_translation_with_cursorpos(const char *tableList, const char *str,
                                     const formtype *typeform,
                                     const char *expected,
                                     const int *cursorPos);

/* Check if a string is translated as expected. Return 0 if the
   translation is as expected and 1 otherwise. */
int check_translation_full(const char *tableList, const char *str,
                           const formtype *typeform, const char *expected,
                           int mode, const int *cursorPos);

/* Check if a string is backtranslated as expected. Return 0 if the
   backtranslation is as expected and 1 otherwise. */
int check_backtranslation(const char *tableList, const char *str,
                          const formtype *typeform, const char *expected);

/* Check if a string is backtranslated as expected. Return 0 if the
   backtranslation is as expected and 1 otherwise. */
int check_backtranslation_with_mode(const char *tableList, const char *str,
                                    const formtype *typeform,
                                    const char *expected, int mode);

/* Check if a string is backtranslated as expected. Return 0 if the
   backtranslation is as expected and 1 otherwise. */
int check_backtranslation_with_cursorpos(const char *tableList, const char *str,
                                         const formtype *typeform,
                                         const char *expected,
                                         const int *cursorPos);

/* Check if a string is backtranslated as expected. Return 0 if the
   backtranslation is as expected and 1 otherwise. */
int check_backtranslation_full(const char *tableList, const char *str,
                               const formtype *typeform, const char *expected,
                               int mode, const int *cursorPos);

/* Check if a string is translated as expected for the given direction
   (0 = forward, backward otherwise). Return 0 if the translation is
   as expected and 1 otherwise. Print diagnostic output on failure if
   diagnostics is not 0. */
int check_full(const char *tableList, const char *str,
               const formtype *typeform, const char *expected,
               int mode, const int *cursorPos, int direction, int diagnostics);

/* Check if a string is hyphenated as expected. Return 0 if the
   hyphenation is as expected and 1 otherwise. */
int check_hyphenation(const char *tableList, const char *str, const char *expected);

/* Helper function to convert a typeform string of '0's, '1's, '2's etc.
   to the required format, which is an array of 0s, 1s, 2s, etc.
   For example, "0000011111000" is converted to {0,0,0,0,0,1,1,1,1,1,0,0,0}
   The caller is responsible for freeing the returned array. */
formtype * convert_typeform(const char* typeform_string);

void update_typeform(const char* typeform_string, formtype* typeform, typeforms kind);
