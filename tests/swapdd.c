/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2026 Baris Ari

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <config.h>

#include <stdio.h>
#include "liblouis.h"

int
main(void) {
	const struct {
		const char *rule;
		int valid;
	} tests[] = {
		/* Reject multi-cell search patterns, wherever they occur in the list. */
		{ "swapdd test 1-2 12", 0 },
		{ "swapdd test 1-2,3,4 12,13,14", 0 },
		{ "swapdd test 1,2-3,4 12,13,14", 0 },
		{ "swapdd test 1,2,3-4 12,13,14", 0 },
		{ "swapdd test 0-1 12", 0 },
		{ "noback swapdd test 1-2 12", 0 },
		{ "nofor swapdd test 1-2 12", 0 },
		/* A single cell can contain several dots, or be blank. */
		{ "swapdd test 12345678 1", 1 },
		{ "swapdd test 0,1,12 1,12,14", 1 },
		/* Multi-cell replacements remain valid for both dot-producing opcodes. */
		{ "swapdd test 1,12,14 1-2,12-3,14-5-6", 1 },
		{ "swapcd test abc 1-2,12-3,14-5-6", 1 },
		{ "swapcc test ABC abc", 1 },
	};
	int result = 0;
	for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		if (!lou_compileString("tests/tables/empty.ctb", "# initialize table")) {
			fprintf(stderr, "Could not initialize the test table\n");
			lou_free();
			return 1;
		}
		int valid = lou_compileString("tests/tables/empty.ctb", tests[i].rule);
		if (valid != tests[i].valid) {
			fprintf(stderr, "%s: expected compilation to %s\n", tests[i].rule,
					tests[i].valid ? "succeed" : "fail");
			result = 1;
		}
		lou_free();
	}
	return result;
}
