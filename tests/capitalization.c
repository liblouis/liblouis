/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2015 Mike Gray <mgray@aph.org>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "liblouis.h"
#include "brl_checks.h"

const char* table = "tests/tables/capitalization.ctb";

int
main (int argc, char **argv)
{
	int rst = 0;
	
	rst |= check(table, "Abc", "+abc");
	rst |= check(table, "ABC", "[abc");
	rst |= check(table, "AbcXyz", "+abc+xyz");
	rst |= check(table, "ABcXYz", "[ab]c[xy]z");
	rst |= check(table, "ABC-XYZ", "[abc-[xyz");
	rst |= check(table, "ABC XYZ ABC", "<abc xyz abc>");
	rst |= check(table, "ABC-XYZ-ABC", "[abc-[xyz-[abc");
	rst |= check(table, "ABC XYZ-ABC", "[abc [xyz-[abc");
	rst |= check(table, "ABC XYZ ABC   ", "<abc xyz abc>   ");
	rst |= check(table, "ABC XYZ ABC XYZ", "<abc xyz abc xyz>");
	rst |= check(table, "ABC-XYZ-ABC-XYZ", "[abc-[xyz-[abc-[xyz");
	rst |= check(table, "ABC XYZ-ABC-XYZ", "[abc [xyz-[abc-[xyz");
	rst |= check(table, "ABC XYZ ABC-XYZ", "<abc xyz abc-xyz>");
	rst |= check(table, "A B C", "<a b c>");
	rst |= check(table, "A B C ", "<a b c> ");
	rst |= check(table, "A-B-C", "+a-+b-+c");
	rst |= check(table, "AB-X-BC", "[ab-+x-[bc");
	
	return rst;      
}
