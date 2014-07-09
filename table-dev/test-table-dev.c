#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "louis.h"

extern void loadTable(const char *tableList);
extern widechar toLowercase(widechar c);
extern int suggestChunks(widechar *text, widechar *braille, char *hyphen_string);

static int test_toLowercase() {
	widechar upper = 'A';
	widechar lower = 'a';
	return lower != toLowercase(upper);
}

static int test_suggestChunks() {
	widechar text[] = {'f', 'o', 'o', 'b', 'a', 'r', '\0'};
	widechar braille[] = {'f', 'u', 'b', 'r', '\0'};
	char hyphen_string[8];
	if (!suggestChunks(text, braille, hyphen_string))
		return 1;
	return strcmp("^00x00$", hyphen_string);
}

int main(int argc, char **argv) {
	loadTable("test-table.ctb");
	if (test_toLowercase())
		return 1;
	if (test_suggestChunks())
		return 1;
	return 0;
}
