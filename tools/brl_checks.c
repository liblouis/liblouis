/* liblouis Braille Translation and Back-Translation Library

   Copyright (C) 2008 Eitan Isaacson <eitan@ascender.com>
   Copyright (C) 2012 James Teh <jamie@nvaccess.org>
   Copyright (C) 2012 Bert Frees <bertfrees@gmail.com>
   Copyright (C) 2014 Mesar Hameed <mesar.hameed@gmail.com>
   Copyright (C) 2015 Mike Gray <mgray@aph.org>
   Copyright (C) 2010-2017 Swiss Library for the Blind, Visually Impaired and Print
   Disabled
   Copyright (C) 2016-2017 Davy Kager <mail@davykager.nl>

   Copying and distribution of this file, with or without modification,
   are permitted in any medium without royalty provided the copyright
   notice and this notice are preserved. This file is offered as-is,
   without any warranty.
*/

/**
 * @file
 * @brief Test helper functions
 */

#include <config.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblouis.h"
#include "internal.h"
#include "brl_checks.h"
#include "unistr.h"

void
print_int_array(const char *prefix, int *pos_list, int len) {
	int i;
	fprintf(stderr, "%s ", prefix);
	for (i = 0; i < len; i++) fprintf(stderr, "%d ", pos_list[i]);
	fprintf(stderr, "\n");
}

void
print_typeform(const formtype *typeform, int len) {
	int i;
	fprintf(stderr, "Typeform:  ");
	for (i = 0; i < len; i++) fprintf(stderr, "%hi", typeform[i]);
	fprintf(stderr, "\n");
}

void
print_widechars(widechar *buffer, int length) {
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

/* direction, 0=forward, otherwise backwards. If diagnostics is 1 then
 * print diagnostics in case where the translation is not as
 * expected */
int
check_base(const char *tableList, const char *input, const char *expected,
		optional_test_params in) {
	widechar *inbuf, *outbuf, *expectedbuf;
	int inlen = strlen(input);
	int outlen = inlen * 10;
	int expectedlen = strlen(expected);
	int i, retval = 0;
	int funcStatus = 0;
	formtype *typeformbuf = NULL;
	int *inputPos = NULL;
	int *outputPos = NULL;
	int cursorPos = 0;

	inbuf = malloc(sizeof(widechar) * inlen);
	outbuf = malloc(sizeof(widechar) * outlen);
	expectedbuf = malloc(sizeof(widechar) * expectedlen);
	if (in.typeform != NULL) {
		typeformbuf = malloc(outlen * sizeof(formtype));
		memcpy(typeformbuf, in.typeform, outlen * sizeof(formtype));
	}
	if (in.cursorPos >= 0) {
		cursorPos = in.cursorPos;
	}
	inlen = _lou_extParseChars(input, inbuf);
	if (!inlen) {
		fprintf(stderr, "Cannot parse input string.\n");
		retval = 1;
		goto fail;
	}
	if (in.expected_inputPos) {
		inputPos = malloc(sizeof(int) * outlen);
	}
	if (in.expected_outputPos) {
		outputPos = malloc(sizeof(int) * inlen);
	}
	if (in.direction == 0) {
		funcStatus = lou_translate(tableList, inbuf, &inlen, outbuf, &outlen, typeformbuf,
				NULL, outputPos, inputPos, &cursorPos, in.mode);
	} else {
		funcStatus = lou_backTranslate(tableList, inbuf, &inlen, outbuf, &outlen,
				typeformbuf, NULL, outputPos, inputPos, &cursorPos, in.mode);
	}
	if (!funcStatus) {
		fprintf(stderr, "Translation failed.\n");
		retval = 1;
		goto fail;
	}

	expectedlen = _lou_extParseChars(expected, expectedbuf);
	for (i = 0; i < outlen && i < expectedlen && expectedbuf[i] == outbuf[i]; i++)
		;
	if (i < outlen || i < expectedlen) {
		retval = 1;
		if (in.diagnostics) {
			outbuf[outlen] = 0;
			fprintf(stderr, "Input:    '%s'\n", input);
			/* Print the original typeform not the typeformbuf, as the
			 * latter has been modified by the translation and contains some
			 * information about outbuf */
			if (in.typeform != NULL) print_typeform(in.typeform, inlen);
			if (in.cursorPos >= 0) fprintf(stderr, "Cursor:   %d\n", in.cursorPos);
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
				fprintf(stderr,
						"Diff:     Expected '%.*s' but received '%.*s' in index %d\n",
						(int)expected_utf8_len, expected_utf8, (int)out_utf8_len,
						out_utf8, i);
			} else if (i < expectedlen) {
				fprintf(stderr,
						"Diff:     Expected '%.*s' but received nothing in index %d\n",
						(int)expected_utf8_len, expected_utf8, i);
			} else {
				fprintf(stderr,
						"Diff:     Expected nothing but received '%.*s' in index %d\n",
						(int)out_utf8_len, out_utf8, i);
			}
			free(expected_utf8);
			free(out_utf8);
		}
	}

	if (in.expected_inputPos) {
		int error_printed = 0;
		for (i = 0; i < outlen; i++) {
			if (in.expected_inputPos[i] != inputPos[i]) {
				if (!error_printed) {  // Print only once
					fprintf(stderr, "Input position failure:\n");
					error_printed = 1;
				}
				fprintf(stderr, "Expected %d, received %d in index %d\n",
						in.expected_inputPos[i], inputPos[i], i);
				retval = 1;
			}
		}
	}

	if (in.expected_outputPos) {
		int error_printed = 0;
		for (i = 0; i < inlen; i++) {
			if (in.expected_outputPos[i] != outputPos[i]) {
				if (!error_printed) {  // Print only once
					fprintf(stderr, "Output position failure:\n");
					error_printed = 1;
				}
				fprintf(stderr, "Expected %d, received %d in index %d\n",
						in.expected_outputPos[i], outputPos[i], i);
				retval = 1;
			}
		}
	}

	if ((in.expected_cursorPos >= 0) && (cursorPos != in.expected_cursorPos)) {
		fprintf(stderr, "Cursor position failure:\n");
		fprintf(stderr, "Initial:%d Expected:%d Actual:%d \n", in.cursorPos,
				in.expected_cursorPos, cursorPos);
		retval = 1;
	}

fail:
	free(inbuf);
	free(outbuf);
	free(expectedbuf);
	free(typeformbuf);
	free(inputPos);
	free(outputPos);

	return retval;
}

/* Helper function to convert a typeform string of '0's, '1's, '2's etc.
 * to the required format, which is an array of 0s, 1s, 2s, etc.
 * For example, "0000011111000" is converted to {0,0,0,0,0,1,1,1,1,1,0,0,0}
 * The caller is responsible for freeing the returned array. */
formtype *
convert_typeform(const char *typeform_string) {
	int len = strlen(typeform_string);
	formtype *typeform = malloc(len * sizeof(formtype));
	int i;
	for (i = 0; i < len; i++) typeform[i] = typeform_string[i] - '0';
	return typeform;
}

void
update_typeform(const char *typeform_string, formtype *typeform, const typeforms kind) {
	int len = strlen(typeform_string);
	int i;
	for (i = 0; i < len; i++)
		if (typeform_string[i] != ' ') typeform[i] |= kind;
}

int
check_cursor_pos(const char *tableList, const char *str, const int *expected_pos) {
	widechar *inbuf;
	widechar *outbuf;
	int *inpos, *outpos;
	int inlen = strlen(str);
	int outlen = inlen;
	int cursor_pos;
	int i, retval = 0;

	inbuf = malloc(sizeof(widechar) * inlen);
	outbuf = malloc(sizeof(widechar) * inlen);
	inpos = malloc(sizeof(int) * inlen);
	outpos = malloc(sizeof(int) * inlen);
	inlen = _lou_extParseChars(str, inbuf);

	for (i = 0; i < inlen; i++) {
		cursor_pos = i;
		if (!lou_translate(tableList, inbuf, &inlen, outbuf, &outlen, NULL, NULL, NULL,
					NULL, &cursor_pos, compbrlAtCursor)) {
			fprintf(stderr, "Translation failed.\n");
			retval = 1;
			goto fail;
		}
		if (expected_pos[i] != cursor_pos) {
			if (!retval)  // Print only once
				fprintf(stderr, "Cursorpos failure:\n");
			fprintf(stderr,
					"string='%s' cursor=%d ('%c') expected=%d received=%d ('%c')\n", str,
					i, str[i], expected_pos[i], cursor_pos, (char)outbuf[cursor_pos]);
			retval = 1;
		}
	}

fail:
	free(inbuf);
	free(outbuf);
	free(inpos);
	free(outpos);
	return retval;
}

/* Check if a string is hyphenated as expected. Return 0 if the
 * hyphenation is as expected and 1 otherwise. */
int
check_hyphenation(const char *tableList, const char *str, const char *expected) {
	widechar *inbuf;
	char *hyphens = NULL;
	int inlen = strlen(str);
	int retval = 0;

	inbuf = malloc(sizeof(widechar) * inlen);
	inlen = _lou_extParseChars(str, inbuf);
	if (!inlen) {
		fprintf(stderr, "Cannot parse input string.\n");
		retval = 1;
		goto fail;
	}
	hyphens = calloc(inlen + 1, sizeof(char));

	if (!lou_hyphenate(tableList, inbuf, inlen, hyphens, 0)) {
		fprintf(stderr, "Hyphenation failed.\n");
		retval = 1;
		goto fail;
	}

	if (strcmp(expected, hyphens)) {
		fprintf(stderr, "Input:    '%s'\n", str);
		fprintf(stderr, "Expected: '%s'\n", expected);
		fprintf(stderr, "Received: '%s'\n", hyphens);
		retval = 1;
	}

fail:
	free(inbuf);
	free(hyphens);
	return retval;
}
