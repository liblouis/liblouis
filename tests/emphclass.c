#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "louis.h"
#include "brl_checks.h"

static char** emph_classes = NULL;

static formtype *
typeform(const char* class, const char* fromString)
{
	int i;
	typeforms kind = plain_text;
	for (i = 0; emph_classes[i]; i++) {
		if (strcmp(class, emph_classes[i]) == 0) {
			switch (i) {
			case 0: kind = italic; break;
			case 1: kind = underline; break;
			case 2: kind = bold; break;
			case 3: kind = script; break;
			case 4: kind = trans_note; break;
			case 5: kind = trans_note_1; break;
			case 6: kind = trans_note_2; break;
			case 7: kind = trans_note_3; break;
			case 8: kind = trans_note_4; break;
			case 9: kind = trans_note_5; break;
			default:
				fprintf(stderr, "CODING ERROR\n");
				exit(1);
			}
			break;
		}
	}
	if (kind == plain_text)
		fprintf(stderr, "Warning: typeform '%s' was not declared\n", class);
	formtype *typeform = calloc(strlen(fromString), sizeof(formtype));
	update_typeform(fromString, typeform, kind);
	return typeform;
}

int
main (int argc, char **argv)
{
	int result = 0;
	const char* table;
	table = "emphclass_invalid_1.utb";
	if (lou_getTable(table)) {
		fprintf(stderr, "%s should be invalid\n", table);
		return 1;
	}
	table = "emphclass_invalid_2.utb";
	if (lou_getTable(table)) {
		fprintf(stderr, "%s should be invalid\n", table);
		return 1;
	}
	table = "emphclass_valid.utb";
	if (!lou_getTable(table)) {
		fprintf(stderr, "%s should be valid\n", table);
		return 1;
	}
	emph_classes = getEmphClasses(table);
	result |= check_translation(table,
	                            "foobar",
	                            typeform("foo", "+++++"),
	                            "~,foobar");
	lou_free();
	return result;
}
