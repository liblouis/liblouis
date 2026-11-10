/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2026 Fengxiaoxx <43958531+Fengxiaoxx@users.noreply.github.com>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <config.h>

#include <stdio.h>
#include "liblouis.h"

/* addRule() must return a valid rule pointer even if putChar()/putDots()
 * reallocates the table, because compileRule() uses it to set nocross.
 * Compile 4000 distinct rules to force reallocation in both UCS-2 and UCS-4
 * builds; the unpatched library reports a use-after-free under ASan. */
int
main(int argc, char **argv) {
	int result = 0;
	int i;
	char rule[64];
	/* Use distinct private-use characters, starting at U+E000. */
	for (i = 0; i < 4000; i++) {
		snprintf(rule, sizeof(rule), "nocross always \\x%04x 1", 0xE000 + i);
		if (!lou_compileString("tests/tables/empty.ctb", rule)) {
			printf("Compiling rule %d failed\n", i);
			result = 1;
			break;
		}
	}
	lou_free();
	return result;
}
