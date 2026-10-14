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

/* for_updatePositions() checks for a cursor sitting on the terminating NUL
 * by reading input->chars[*cursorPosition] before comparing the position.
 * The input length is already the NUL offset when the caller terminates the
 * buffer, so a caller buffer of exactly *inlen elements is read one widechar
 * past its end when the cursor is placed at the end of the input. With an
 * AddressSanitizer build the unpatched library reports a heap-buffer-overflow
 * read in for_updatePositions(), while the patched library compares positions
 * instead and keeps placing the cursor at the end of the output. */
int
main(int argc, char **argv) {
	int result = 0;
	int inlen = 1, outlen = 16, cursorPos = 1;
	widechar *input = malloc(1 * sizeof(widechar));
	widechar output[16];
	if (!input) {
		printf("Out of memory\n");
		return 1;
	}
	input[0] = 'a';
	if (!lou_translate("tests/tables/cursor-end.ctb", input, &inlen, output,
				&outlen, NULL, NULL, NULL, NULL, &cursorPos, 0)) {
		printf("Translation failed\n");
		result = 1;
	} else if (outlen != 1 || cursorPos != 1) {
		printf("Unexpected result: outlen=%d cursorPos=%d\n", outlen, cursorPos);
		result = 1;
	}
	/* The same placement for a terminated buffer must keep working. */
	{
		widechar terminated[2] = { 'a', 0 };
		inlen = 2;
		outlen = 16;
		cursorPos = 1;
		if (!lou_translate("tests/tables/cursor-end.ctb", terminated, &inlen,
					output, &outlen, NULL, NULL, NULL, NULL, &cursorPos, 0)) {
			printf("Terminated translation failed\n");
			result = 1;
		} else if (outlen != 1 || cursorPos != 1) {
			printf("Unexpected terminated result: outlen=%d cursorPos=%d\n",
					outlen, cursorPos);
			result = 1;
		}
	}
	free(input);
	lou_free();
	return result;
}
