/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2024 liblouis contributors

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

/* Minimal test that exercises lou_backTranslate with a typeform buffer.
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

cleanup:
	lou_free();
	return result;
}
