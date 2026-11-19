/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2026 Anthony Tibbs <anthony@tibbs.ca>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

/* Regression test for issue #2172.
 *
 * lou_backTranslate() reads its input up to the first NUL character, and the
 * caller passes the input buffer capacity in *inlen. The internal position
 * maps are sized from the NUL-terminated length (and *outlen), but the
 * position merging in multi-pass tables and the inputPos/outputPos loops used
 * to be bounded by the original *inlen. With *inlen larger than the maps, this
 * read and wrote past the ends of the maps (visible under AddressSanitizer),
 * and the returned *inlen did not reflect the length actually consumed.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblouis.h"

static int
check(const char *table, int inlen, int outlen, int withPos) {
	const char *text = "abc";
	int textlen = (int)strlen(text);
	int result = 0;
	int k;
	widechar *inbuf = calloc(inlen, sizeof(widechar));
	widechar *outbuf = calloc(outlen, sizeof(widechar));
	int *outputPos = calloc(inlen, sizeof(int));
	int *inputPos = calloc(outlen, sizeof(int));
	int inlenUsed = inlen;
	int outlenUsed = outlen;

	for (k = 0; k < textlen; k++) inbuf[k] = text[k];

	if (!lou_backTranslate(table, inbuf, &inlenUsed, outbuf, &outlenUsed, NULL, NULL,
				withPos ? outputPos : NULL, withPos ? inputPos : NULL, NULL, 0)) {
		printf("FAIL: %s inlen=%d outlen=%d: lou_backTranslate returned failure\n", table,
				inlen, outlen);
		result = 1;
		goto cleanup;
	}
	if (inlenUsed != textlen) {
		printf("FAIL: %s inlen=%d outlen=%d: expected *inlen %d, got %d\n", table, inlen,
				outlen, textlen, inlenUsed);
		result = 1;
	}
	if (outlenUsed != textlen) {
		printf("FAIL: %s inlen=%d outlen=%d: expected *outlen %d, got %d\n", table, inlen,
				outlen, textlen, outlenUsed);
		result = 1;
	} else {
		for (k = 0; k < textlen; k++)
			if (outbuf[k] != (widechar)text[k]) {
				printf("FAIL: %s inlen=%d outlen=%d: unexpected output\n", table, inlen,
						outlen);
				result = 1;
				break;
			}
	}

cleanup:
	free(inbuf);
	free(outbuf);
	free(outputPos);
	free(inputPos);
	return result;
}

int
main(int argc, char **argv) {
	int result = 0;
	/* multi-pass table (pass2 rules), exact shape from the report */
	result |= check("tables/en-us-g2.ctb", 1028, 1024, 0);
	result |= check("tables/en-us-g2.ctb", 1028, 1024, 1);
	/* larger gap between *inlen and the map size */
	result |= check("tables/en-us-g2.ctb", 5000, 1024, 1);
	/* single-pass table: only the inputPos/outputPos loops were affected */
	result |= check("tables/en-us-comp8.ctb", 5000, 1024, 1);
	/* sanity: *inlen no larger than the input */
	result |= check("tables/en-us-g2.ctb", 3, 1024, 1);
	lou_free();
	return result;
}
