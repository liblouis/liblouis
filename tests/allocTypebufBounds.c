/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2026 Fengxiaoxx <43958531+Fengxiaoxx@users.noreply.github.com>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include "liblouis.h"

#define INPUT_LENGTH 5000
#define OUTPUT_LENGTH 1024

/* _lou_translate() writes one entry per input character into its internal
 * formtype buffer (typebuf), but _lou_allocMem() sizes that buffer from
 * *outlen only: a first allocation holds max(*outlen, 1024) + 4 entries, so
 * a longer input writes past the end of the buffer. Translate an input
 * longer than the output buffer; with an AddressSanitizer build the
 * unpatched library reports a heap-buffer-overflow in _lou_translate(),
 * while the patched library truncates the output and returns success. */
int
main(int argc, char **argv) {
	int result = 0;
	int inlen = INPUT_LENGTH, outlen = OUTPUT_LENGTH, i;
	widechar *input = malloc(INPUT_LENGTH * sizeof(widechar));
	widechar *output = malloc(OUTPUT_LENGTH * sizeof(widechar));
	if (!input || !output) {
		printf("Out of memory\n");
		return 1;
	}

	for (i = 0; i < INPUT_LENGTH; i++) input[i] = 'a';

	if (!lou_translateString("tests/tables/typebuf-bounds.ctb", input, &inlen, output,
				&outlen, NULL, NULL, 0)) {
		printf("Translation failed\n");
		result = 1;
	} else if (inlen != OUTPUT_LENGTH || outlen != OUTPUT_LENGTH) {
		/* The output buffer is the limit: the translation must be truncated
		 * to OUTPUT_LENGTH characters, and exactly that many input
		 * characters must be consumed. */
		printf("Unexpected result: inlen=%d outlen=%d\n", inlen, outlen);
		result = 1;
	}

	free(input);
	free(output);
	lou_free();

	return result;
}
