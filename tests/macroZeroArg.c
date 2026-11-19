/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2026 Fengxiaoxx <43958531+Fengxiaoxx@users.noreply.github.com>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <config.h>

#include <stdio.h>
#include "liblouis.h"

int
main(void) {
	int ok = lou_checkTable("tests/tables/macro-zero.ctb");
	lou_free();
	if (ok) {
		fprintf(stderr, "Macro argument $0 was accepted\n");
		return 1;
	}
	return 0;
}
