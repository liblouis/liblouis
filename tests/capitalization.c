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
	
	rst |= check_translation(table, "Abc", NULL, "+abc");
	rst |= check_translation(table, "ABC", NULL, "[abc");
	rst |= check_translation(table, "AbcXyz", NULL, "+abc+xyz");
	rst |= check_translation(table, "ABcXYz", NULL, "[ab]c[xy]z");
	rst |= check_translation(table, "ABC-XYZ", NULL, "[abc-[xyz");
	rst |= check_translation(table, "ABC XYZ ABC", NULL, "<abc xyz abc>");
	rst |= check_translation(table, "ABC-XYZ-ABC", NULL, "[abc-[xyz-[abc");
	rst |= check_translation(table, "ABC XYZ-ABC", NULL, "[abc [xyz-[abc");
	rst |= check_translation(table, "ABC XYZ ABC   ", NULL, "<abc xyz abc>   ");
	rst |= check_translation(table, "ABC XYZ ABC XYZ", NULL, "<abc xyz abc xyz>");
	rst |= check_translation(table, "ABC-XYZ-ABC-XYZ", NULL, "[abc-[xyz-[abc-[xyz");
	rst |= check_translation(table, "ABC XYZ-ABC-XYZ", NULL, "[abc [xyz-[abc-[xyz");
	rst |= check_translation(table, "ABC XYZ ABC-XYZ", NULL, "<abc xyz abc-xyz>");
	rst |= check_translation(table, "A B C", NULL, "<a b c>");
	rst |= check_translation(table, "A B C ", NULL, "<a b c> ");
	rst |= check_translation(table, "A-B-C", NULL, "+a-+b-+c");
	rst |= check_translation(table, "AB-X-BC", NULL, "[ab-+x-[bc");
	
	return rst;      
}
