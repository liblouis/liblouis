/* liblouis Braille Translation and Back-Translation Library

   Copyright (C) 2014 ViewPlus Technologies, Inc. www.viewplus.com
   and the liblouis team. http://liblouis.org
   All rights reserved

   This file is free software; you can redistribute it and/or modify
   it under the terms of the Lesser or Library GNU General Public
   License as published by the Free Software Foundation; either
   version 3, or (at your option) any later version.

   This file is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   Library GNU General Public License for more details.

   You should have received a copy of the Library GNU General Public
   License along with this program; see the file COPYING. If not,
   write to the Free Software Foundation, 51 Franklin Street, Fifth
   Floor, Boston, MA 02110-1301, USA.
   */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblouis.h"
#include "louis.h"
#include "brl_checks.h"
#include "unistr.h"

int check_with_mode(const char *tableList, const char *str,
                    const formtype *typeform, const char *expected, int mode,
                    int direction, int diagnostics);

void print_int_array(const char *prefix, int *pos_list, int len) {
  int i;
  fprintf(stderr, "%s ", prefix);
  for (i = 0; i < len; i++) fprintf(stderr, "%d ", pos_list[i]);
  fprintf(stderr, "\n");
}

void print_typeform(const formtype *typeform, int len) {
  int i;
  fprintf(stderr, "Typeform:  ");
  for (i = 0; i < len; i++) fprintf(stderr, "%hi", typeform[i]);
  fprintf(stderr, "\n");
}

void print_widechars(widechar *buffer, int length) {
  uint8_t *result_buf;
  size_t result_len;

#ifdef WIDECHARS_ARE_UCS4
  result_buf = u32_to_u8(buffer, length, NULL, &result_len);
#else
  result_buf = u16_to_u8(buffer, length, NULL, &result_len);
#endif
  fprintf(stderr, "%.*s", (int)result_len, result_buf);
  free(result_buf);
}

/* Helper function to convert a typeform string of '0's, '1's, '2's etc.
   to the required format, which is an array of 0s, 1s, 2s, etc.
   For example, "0000011111000" is converted to {0,0,0,0,0,1,1,1,1,1,0,0,0}
   The caller is responsible for freeing the returned array. */
formtype *convert_typeform(const char *typeform_string) {
  int len = strlen(typeform_string);
  formtype *typeform = malloc(len * sizeof(formtype));
  int i;
  for (i = 0; i < len; i++) typeform[i] = typeform_string[i] - '0';
  return typeform;
}

void update_typeform(const char *typeform_string, formtype *typeform,
                     const typeforms kind) {
  int len = strlen(typeform_string);
  int i;
  for (i = 0; i < len; i++)
    if (typeform_string[i] != ' ') typeform[i] |= kind;
}

/* Check if a string is translated as expected. Return 0 if the
   translation is as expected and 1 otherwise. */
int check_translation(const char *tableList, const char *str,
                      const formtype *typeform, const char *expected) {
  return check_translation_with_mode(tableList, str, typeform, expected, 0);
}

/* Check if a string is translated as expected. Return 0 if the
   translation is as expected and 1 otherwise. */
int check_translation_with_mode(const char *tableList, const char *str,
                                const formtype *typeform, const char *expected,
                                int mode) {
  return check_with_mode(tableList, str, typeform, expected, mode, 0, 1);
}

/* Check if a string is backtranslated as expected. Return 0 if the
   backtranslation is as expected and 1 otherwise. */
int check_backtranslation(const char *tableList, const char *str,
                          const formtype *typeform, const char *expected) {
  return check_backtranslation_with_mode(tableList, str, typeform, expected, 0);
}

/* Check if a string is backtranslated as expected. Return 0 if the
   backtranslation is as expected and 1 otherwise. */
int check_backtranslation_with_mode(const char *tableList, const char *str,
                                    const formtype *typeform,
                                    const char *expected, int mode) {
  return check_with_mode(tableList, str, typeform, expected, mode, 1, 1);
}

/* direction, 0=forward, otherwise backwards. If diagnostics is 1 then
   print diagnostics in case where the translation is not as
   expected */
int check_with_mode(const char *tableList, const char *str,
                    const formtype *typeform, const char *expected, int mode,
                    int direction, int diagnostics) {
  widechar *inbuf, *outbuf, *expectedbuf;
  int inlen;
  int outlen;
  int i, rv = 0;
  int funcStatus = 0;

  formtype *typeformbuf = NULL;

  int expectedlen = strlen(expected);

  inlen = strlen(str);
  outlen = inlen * 10;
  inbuf = malloc(sizeof(widechar) * inlen);
  outbuf = malloc(sizeof(widechar) * outlen);
  expectedbuf = malloc(sizeof(widechar) * expectedlen);
  if (typeform != NULL) {
    typeformbuf = malloc(outlen * sizeof(formtype));
    memcpy(typeformbuf, typeform, outlen * sizeof(formtype));
  }
  inlen = extParseChars(str, inbuf);
  if (!inlen) {
    fprintf(stderr, "Cannot parse input string.\n");
    return 1;
  }
  if (direction == 0) {
    funcStatus = lou_translate(tableList, inbuf, &inlen, outbuf, &outlen,
                               typeformbuf, NULL, NULL, NULL, NULL, mode);
  } else {
    funcStatus = lou_backTranslate(tableList, inbuf, &inlen, outbuf, &outlen,
                                   typeformbuf, NULL, NULL, NULL, NULL, mode);
  }
  if (!funcStatus) {
    fprintf(stderr, "Translation failed.\n");
    return 1;
  }

  expectedlen = extParseChars(expected, expectedbuf);
  for (i = 0; i < outlen && i < expectedlen && expectedbuf[i] == outbuf[i]; i++)
    ;
  if (i < outlen || i < expectedlen) {
    rv = 1;
    if (diagnostics) {
      outbuf[outlen] = 0;
      fprintf(stderr, "Input:    '%s'\n", str);
      /* Print the original typeform not the typeformbuf, as the
         latter has been modified by the translation and contains some
         information about outbuf */
      if (typeform != NULL) print_typeform(typeform, inlen);
      fprintf(stderr, "Expected: '%s' (length %d)\n", expected, expectedlen);
      fprintf(stderr, "Received: '");
      print_widechars(outbuf, outlen);
      fprintf(stderr, "' (length %d)\n", outlen);

      uint8_t *expected_utf8;
      uint8_t *out_utf8;
      size_t expected_utf8_len;
      size_t out_utf8_len;
#ifdef WIDECHARS_ARE_UCS4
      expected_utf8 = u32_to_u8(&expectedbuf[i], 1, NULL, &expected_utf8_len);
      out_utf8 = u32_to_u8(&outbuf[i], 1, NULL, &out_utf8_len);
#else
      expected_utf8 = u16_to_u8(&expectedbuf[i], 1, NULL, &expected_utf8_len);
      out_utf8 = u16_to_u8(&outbuf[i], 1, NULL, &out_utf8_len);
#endif

      if (i < outlen && i < expectedlen) {
	fprintf(stderr, "Diff: Expected '%.*s' but received '%.*s' in index %d\n",
	        (int)expected_utf8_len, expected_utf8, (int)out_utf8_len, out_utf8, i);
      } else if (i < expectedlen) {
	fprintf(stderr,
	        "Diff: Expected '%.*s' but received nothing in index %d\n",
	        (int)expected_utf8_len, expected_utf8, i);
      } else {
	fprintf(stderr,
	        "Diff: Expected nothing but received '%.*s' in index %d\n",
	        (int)out_utf8_len, out_utf8, i);
      }
      free(expected_utf8);
      free(out_utf8);
    }
  }

  free(inbuf);
  free(outbuf);
  free(expectedbuf);
  free(typeformbuf);
  return rv;
}

int check_inpos(const char *tableList, const char *str,
                const int *expected_poslist) {
  widechar *inbuf;
  widechar *outbuf;
  int *inpos;
  int inlen;
  int outlen;
  int i, rv = 0;

  inlen = strlen(str) * 2;
  outlen = inlen;
  inbuf = malloc(sizeof(widechar) * inlen);
  outbuf = malloc(sizeof(widechar) * outlen);
  inpos = malloc(sizeof(int) * inlen);
  for (i = 0; i < inlen; i++) {
    inbuf[i] = str[i];
  }
  if (!lou_translate(tableList, inbuf, &inlen, outbuf, &outlen, NULL, NULL, NULL,
		     inpos, NULL, 0)) {
    fprintf(stderr, "Translation failed.\n");
    return 1;
  }
  for (i = 0; i < outlen; i++) {
    if (expected_poslist[i] != inpos[i]) {
      rv = 1;
      fprintf(stderr, "Expected %d, received %d in index %d\n",
              expected_poslist[i], inpos[i], i);
    }
  }

  free(inbuf);
  free(outbuf);
  free(inpos);
  return rv;
}

int check_outpos(const char *tableList, const char *str,
                 const int *expected_poslist) {
  widechar *inbuf;
  widechar *outbuf;
  int *inpos, *outpos;
  int origInlen, inlen;
  int outlen;
  int i, rv = 0;

  origInlen = inlen = strlen(str);
  outlen = inlen * 2;
  inbuf = malloc(sizeof(widechar) * inlen);
  outbuf = malloc(sizeof(widechar) * outlen);
  /* outputPos can be affected by inputPos, so pass inputPos as well. */
  inpos = malloc(sizeof(int) * outlen);
  outpos = malloc(sizeof(int) * inlen);
  for (i = 0; i < inlen; i++) {
    inbuf[i] = str[i];
  }
  if (!lou_translate(tableList, inbuf, &inlen, outbuf, &outlen, NULL, NULL, outpos,
		     inpos, NULL, 0)) {
    fprintf(stderr, "Translation failed.\n");
    return 1;
  }
  if (inlen != origInlen) {
    fprintf(stderr, "original inlen %d and returned inlen %d differ\n",
            origInlen, inlen);
  }

  for (i = 0; i < inlen; i++) {
    if (expected_poslist[i] != outpos[i]) {
      rv = 1;
      fprintf(stderr, "Expected %d, received %d in index %d\n",
              expected_poslist[i], outpos[i], i);
    }
  }

  free(inbuf);
  free(outbuf);
  free(inpos);
  free(outpos);
  return rv;
}

int check_cursor_pos(const char *tableList, const char *str,
                     const int *expected_pos) {
  widechar *inbuf;
  widechar *outbuf;
  int *inpos, *outpos;
  int inlen;
  int outlen;
  int cursor_pos;
  int i, rv = 0;

  inlen = strlen(str);
  outlen = inlen;
  inbuf = malloc(sizeof(widechar) * inlen);
  outbuf = malloc(sizeof(widechar) * inlen);
  inpos = malloc(sizeof(int) * inlen);
  outpos = malloc(sizeof(int) * inlen);

  inlen = extParseChars(str, inbuf);

  for (i = 0; i < inlen; i++) {
    cursor_pos = i;
    if (!lou_translate(tableList, inbuf, &inlen, outbuf, &outlen, NULL, NULL, NULL,
		       NULL, &cursor_pos, compbrlAtCursor)) {
      fprintf(stderr, "Translation failed.\n");
      return 1;
    }
    if (expected_pos[i] != cursor_pos) {
      rv = 1;
      fprintf(stderr,
              "string='%s' cursor=%d ('%c') expected=%d received=%d ('%c')\n",
              str, i, str[i], expected_pos[i], cursor_pos,
              (char)outbuf[cursor_pos]);
    }
  }

  free(inbuf);
  free(outbuf);
  free(inpos);
  free(outpos);

  return rv;
}

/* Check if a string is hyphenated as expected. Return 0 if the
   hyphenation is as expected and 1 otherwise. */
int check_hyphenation(const char *tableList, const char *str,
                      const char *expected) {
  widechar *inbuf;
  char *hyphens;
  int inlen;
  int rv = 0;

  inlen = strlen(str);
  inbuf = malloc(sizeof(widechar) * inlen);
  inlen = extParseChars(str, inbuf);
  if (!inlen) {
    fprintf(stderr, "Cannot parse input string.\n");
    return 1;
  }
  hyphens = calloc(inlen + 1, sizeof(char));

  if (!lou_hyphenate(tableList, inbuf, inlen, hyphens, 0)) {
    fprintf(stderr, "Hyphenation failed.\n");
    return 1;
  }

  if (strcmp(expected, hyphens)) {
    fprintf(stderr, "Input:    '%s'\n", str);
    fprintf(stderr, "Expected: '%s'\n", expected);
    fprintf(stderr, "Received: '%s'\n", hyphens);
    rv = 1;
  }

  free(inbuf);
  free(hyphens);
  lou_free();
  return rv;
}
