/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2026 Bhavya H <bhavya@labs.digiscrypt.com>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

/* A cursor at the end of the input (cursorPos == inlen) must not be looked up in
 * outputPos, which only has inlen elements. The slot after those elements is
 * filled with a guard value that the library must never return. With the cursor
 * at the end, passing outputPos must give the same cursor as passing NULL. */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include "liblouis.h"

#define GUARD -1234

static int
check(const char *table, const widechar *input, int backward) {
	widechar outbuf[64];
	int inlen, outlen, cursorPos, expected;
	int *outputPos;
	int ok;

	inlen = 3;
	outlen = 64;
	expected = 3;
	if (backward)
		ok = lou_backTranslate(table, input, &inlen, outbuf, &outlen, NULL, NULL, NULL,
				NULL, &expected, 0);
	else
		ok = lou_translate(table, input, &inlen, outbuf, &outlen, NULL, NULL, NULL, NULL,
				&expected, 0);
	if (!ok) {
		printf("FAIL: translation without outputPos failed\n");
		return 1;
	}

	outputPos = malloc((3 + 1) * sizeof(int));
	outputPos[3] = GUARD;
	inlen = 3;
	outlen = 64;
	cursorPos = 3;
	if (backward)
		ok = lou_backTranslate(table, input, &inlen, outbuf, &outlen, NULL, NULL,
				outputPos, NULL, &cursorPos, 0);
	else
		ok = lou_translate(table, input, &inlen, outbuf, &outlen, NULL, NULL, outputPos,
				NULL, &cursorPos, 0);
	free(outputPos);
	if (!ok) {
		printf("FAIL: translation with outputPos failed\n");
		return 1;
	}
	if (cursorPos != expected) {
		printf("FAIL: %s cursor at end of input: expected %d, got %d\n",
				backward ? "backward" : "forward", expected, cursorPos);
		return 1;
	}
	return 0;
}

int
main(int argc, char **argv) {
	const char *table = "tables/en-ueb-g1.ctb";
	const widechar text[] = { 'a', 'b', 'c', 0 };
	const widechar braille[] = { 0x2801, 0x2803, 0x2809, 0 };
	int result = 0;

	result |= check(table, text, 0);
	result |= check(table, braille, 1);
	lou_free();
	return result;
}
