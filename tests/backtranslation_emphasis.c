/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2025 Benedict Carling <hello@paigebraille.com>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

/* Minimal test that exercises lou_backTranslate with and without a typeform buffer.
 * Full emphasis/typeform coverage lives in backtranslation_emphasis.yaml. */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "liblouis.h"

int
main(int argc, char **argv)
{
	const char *table = "tables/en-ueb-g1.ctb";
	int result = 0;

	/* "hello" in UEB - no emphasis indicators */
	widechar input[] = { 0x2813, 0x2811, 0x2807, 0x2807, 0x2815 };
	widechar outbuf[256];
	formtype typeform[256];
	int inlen = 5;
	int outlen = 256;
	int i;

	memset(outbuf, 0, sizeof(outbuf));
	memset(typeform, 0, sizeof(typeform));

	if (!lou_backTranslate(table, input, &inlen, outbuf, &outlen, typeform, NULL,
				NULL, NULL, NULL, 0)) {
		printf("FAIL: lou_backTranslate returned failure\n");
		result = 1;
		goto cleanup;
	}

	/* Check output text */
	{
		const char *expected = "hello";
		char output_str[256];
		for (i = 0; i < outlen && i < 255; i++)
			output_str[i] = (char)outbuf[i];
		output_str[i] = '\0';

		if (strcmp(output_str, expected) != 0) {
			printf("FAIL: expected '%s', got '%s'\n", expected, output_str);
			result = 1;
		}
	}

	/* Verify typeform buffer is all zeros (no emphasis) */
	for (i = 0; i < outlen; i++) {
		if (typeform[i] != 0) {
			printf("FAIL: typeform[%d] = %d, expected 0\n", i, typeform[i]);
			result = 1;
			break;
		}
	}

	if (result == 0)
		printf("PASS: backtranslation with typeform buffer\n");

	/* Emphasis indicators stand for no text, so the output must not depend on
	 * whether a typeform buffer is passed (issue #2183). Italic "word" in UEB. */
	{
		widechar emph[] = { 0x2828, 0x2802, 0x283a, 0x2815, 0x2817, 0x2819, 0x2828,
			0x2804 };
		widechar withbuf[256], without[256];
		int withlen = 256, withoutlen = 256;
		memset(typeform, 0, sizeof(typeform));
		inlen = 8;
		if (!lou_backTranslate(table, emph, &inlen, withbuf, &withlen, typeform, NULL,
					NULL, NULL, NULL, 0)) {
			printf("FAIL: lou_backTranslate with typeform buffer returned failure\n");
			result = 1;
			goto cleanup;
		}
		inlen = 8;
		if (!lou_backTranslate(table, emph, &inlen, without, &withoutlen, NULL, NULL,
					NULL, NULL, NULL, 0)) {
			printf("FAIL: lou_backTranslate without typeform buffer returned failure\n");
			result = 1;
			goto cleanup;
		}
		if (withlen != 4 || withoutlen != 4 ||
				memcmp(withbuf, without, 4 * sizeof(widechar)) != 0) {
			printf("FAIL: output differs with (%d chars) and without (%d chars) a "
				   "typeform buffer\n",
					withlen, withoutlen);
			result = 1;
		}
		for (i = 0; i < 4; i++) {
			if (typeform[i] == 0 || typeform[i] != typeform[0]) {
				printf("FAIL: typeform[%d] = %d, expected the same emphasis on every "
					   "character\n",
						i, typeform[i]);
				result = 1;
				break;
			}
		}
		if (result == 0)
			printf("PASS: same output with and without typeform buffer\n");
	}

cleanup:
	lou_free();
	return result;
}
