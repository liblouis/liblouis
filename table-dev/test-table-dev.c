#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "louis.h"

extern void loadTable(const char *tableList);
extern widechar toLowercase(widechar c);
extern int suggestChunks(widechar *text, widechar *braille);

static int check_widechar_string(char *expected, widechar* string) {
	int expected_len = strlen(expected);
	int string_len;
	int k;
	for (string_len = 0; string[string_len]; string_len++)
		;
	if (expected_len != string_len)
		return 1;
	for (k = 0; k < string_len; k++)
		if (expected[k] != string[k])
			return 1;
	return 0;
}

static int test_toLowercase() {
	widechar upper = 'A';
	widechar lower = 'a';
	return lower != toLowercase(upper);
}

static int test_suggestChunks() {
	widechar text[] = {'f', 'o', 'o', 'b', 'a', 'r', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	widechar braille[] = {'f', 'u', 'b', 'r', 0};
	if (!suggestChunks(text, braille))
		return 1;
	return check_widechar_string("[foo][bar]", text);
}

int main(int argc, char **argv) {
	loadTable("test-table.ctb");
	if (test_toLowercase())
		return 1;
	if (test_suggestChunks())
		return 1;
	return 0;
}
