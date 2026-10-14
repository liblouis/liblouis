/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2026 Fengxiaoxx <43958531+Fengxiaoxx@users.noreply.github.com>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <config.h>

#include <stdio.h>
#include "liblouis.h"

/* addRule() stores the new rule pointer in the caller's out-parameter before
 * the single-character and single-cell helpers call putChar()/putDots(),
 * which can reallocate the table; the helpers only refresh addRule()'s local
 * copy. compileRule() then executes `r->nocross = 1` through the stale
 * pointer. Compile enough `nocross always <char> 1` rules with fresh
 * characters into one table to force such a reallocation; with an
 * AddressSanitizer build the unpatched library reports a heap-use-after-free
 * write in compileRule(), while the patched library compiles all rules. */
int
main(int argc, char **argv) {
	int result = 0;
	int i;
	char rule[128];
	for (i = 0; i < 4000; i++) {
		int c = 0xE000 + i;
		unsigned char u8[3] = { (unsigned char)(0xE0 | (c >> 12)),
			(unsigned char)(0x80 | ((c >> 6) & 0x3F)),
			(unsigned char)(0x80 | (c & 0x3F)) };
		snprintf(rule, sizeof(rule), "nocross always %c%c%c 1", u8[0], u8[1],
				u8[2]);
		if (!lou_compileString("tests/tables/empty.ctb", rule)) {
			printf("Compiling rule %d failed\n", i);
			result = 1;
			break;
		}
	}
	lou_free();
	return result;
}
