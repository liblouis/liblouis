/* liblouis Braille Translation and Back-Translation Library

   Copyright (C) 2015, 2016 Christian Egli, Swiss Library for the Blind, Visually Impaired
   and Print Disabled
   Copyright (C) 2017 Bert Frees

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>
#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "internal.h"
#include "error.h"
#include "errno.h"
#include "progname.h"
#include "version-etc.h"
#include "brl_checks.h"

static const struct option longopts[] = {
	{ "help", no_argument, NULL, 'h' }, { "version", no_argument, NULL, 'v' },
	{ NULL, 0, NULL, 0 },
};

const char version_etc_copyright[] =
		"Copyright %s %d Swiss Library for the Blind, Visually Impaired and Print "
		"Disabled";

#define AUTHORS "Christian Egli"

#define DIRECTION_FORWARD 0
#define DIRECTION_BACKWARD 1
#define DIRECTION_BOTH 2
#define DIRECTION_DEFAULT DIRECTION_FORWARD

#define HYPHENATION_OFF 0
#define HYPHENATION_ON 1
#define HYPHENATION_DEFAULT HYPHENATION_OFF

static void
print_help(void) {
	printf("\
Usage: %s YAML_TEST_FILE\n",
			program_name);

	fputs("\
Run the tests defined in the YAML_TEST_FILE. Return 0 if all tests pass\n\
or 1 if any of the tests fail. The details of failing tests are printed\n\
to stderr.\n\n",
			stdout);

	fputs("\
  -h, --help          display this help and exit\n\
  -v, --version       display version information and exit\n",
			stdout);

	printf("\n");
	printf("Report bugs to %s.\n", PACKAGE_BUGREPORT);

#ifdef PACKAGE_PACKAGER_BUG_REPORTS
	printf("Report %s bugs to: %s\n", PACKAGE_PACKAGER, PACKAGE_PACKAGER_BUG_REPORTS);
#endif
#ifdef PACKAGE_URL
	printf("%s home page: <%s>\n", PACKAGE_NAME, PACKAGE_URL);
#endif
}

#define EXIT_SKIPPED 77

#ifdef HAVE_LIBYAML
#include <yaml.h>

const char *event_names[] = { "YAML_NO_EVENT", "YAML_STREAM_START_EVENT",
	"YAML_STREAM_END_EVENT", "YAML_DOCUMENT_START_EVENT", "YAML_DOCUMENT_END_EVENT",
	"YAML_ALIAS_EVENT", "YAML_SCALAR_EVENT", "YAML_SEQUENCE_START_EVENT",
	"YAML_SEQUENCE_END_EVENT", "YAML_MAPPING_START_EVENT", "YAML_MAPPING_END_EVENT" };
const char *encoding_names[] = { "YAML_ANY_ENCODING", "YAML_UTF8_ENCODING",
	"YAML_UTF16LE_ENCODING", "YAML_UTF16BE_ENCODING" };

const char *inline_table_prefix = "checkyaml_inline_";

char *file_name;

int errors = 0;
int count = 0;

static char const **emph_classes = NULL;

static void
simple_error(const char *msg, yaml_parser_t *parser, yaml_event_t *event) {
	error_at_line(EXIT_FAILURE, 0, file_name,
			event->start_mark.line ? event->start_mark.line + 1
								   : parser->problem_mark.line + 1,
			"%s", msg);
}

static void
yaml_parse_error(yaml_parser_t *parser) {
	error_at_line(EXIT_FAILURE, 0, file_name, parser->problem_mark.line + 1, "%s",
			parser->problem);
}

static void
yaml_error(yaml_event_type_t expected, yaml_event_t *event) {
	error_at_line(EXIT_FAILURE, 0, file_name, event->start_mark.line + 1,
			"Expected %s (actual %s)", event_names[expected], event_names[event->type]);
}

static char *
read_table_query(yaml_parser_t *parser, const char **table_file_name_check) {
	yaml_event_t event;
	char *query_as_string = malloc(sizeof(char) * MAXSTRING);
	char *p = query_as_string;
	query_as_string[0] = '\0';
	while (1) {
		if (!yaml_parser_parse(parser, &event)) yaml_error(YAML_SCALAR_EVENT, &event);
		if (event.type == YAML_SCALAR_EVENT) {

			// (temporary) feature to check whether the table query matches an expected
			// table
			if (!strcmp((const char *)event.data.scalar.value, "__assert-match")) {
				yaml_event_delete(&event);
				if (!yaml_parser_parse(parser, &event) ||
						(event.type != YAML_SCALAR_EVENT))
					yaml_error(YAML_SCALAR_EVENT, &event);
				*table_file_name_check = strdup((const char *)event.data.scalar.value);
				yaml_event_delete(&event);
			} else {
				if (query_as_string != p) strcat(p++, " ");
				strcat(p, (const char *)event.data.scalar.value);
				p += event.data.scalar.length;
				strcat(p++, ":");
				yaml_event_delete(&event);
				if (!yaml_parser_parse(parser, &event) ||
						(event.type != YAML_SCALAR_EVENT))
					yaml_error(YAML_SCALAR_EVENT, &event);
				strcat(p, (const char *)event.data.scalar.value);
				p += event.data.scalar.length;
				yaml_event_delete(&event);
			}
		} else if (event.type == YAML_MAPPING_END_EVENT) {
			yaml_event_delete(&event);
			break;
		} else
			yaml_error(YAML_SCALAR_EVENT, &event);
	}
	return query_as_string;
}

static char *
read_table(yaml_event_t *start_event, yaml_parser_t *parser, const char *display_table) {
	char *table = NULL;
	if (start_event->type != YAML_SCALAR_EVENT ||
			strcmp((const char *)start_event->data.scalar.value, "table"))
		return 0;

	table = malloc(sizeof(char) * MAXSTRING);
	table[0] = '\0';
	yaml_event_t event;
	if (!yaml_parser_parse(parser, &event) ||
			!(event.type == YAML_SEQUENCE_START_EVENT ||
					event.type == YAML_SCALAR_EVENT ||
					event.type == YAML_MAPPING_START_EVENT))
		error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
				"Expected %s, %s or %s (actual %s)",
				event_names[YAML_SEQUENCE_START_EVENT], event_names[YAML_SCALAR_EVENT],
				event_names[YAML_MAPPING_START_EVENT], event_names[event.type]);
	if (event.type == YAML_SEQUENCE_START_EVENT) {
		yaml_event_delete(&event);
		int done = 0;
		char *p = table;
		while (!done) {
			if (!yaml_parser_parse(parser, &event)) {
				yaml_parse_error(parser);
			}
			if (event.type == YAML_SEQUENCE_END_EVENT) {
				done = 1;
			} else if (event.type == YAML_SCALAR_EVENT) {
				if (table != p) strcat(p++, ",");
				strcat(p, (const char *)event.data.scalar.value);
				p += event.data.scalar.length;
			}
			yaml_event_delete(&event);
		}
		if (!lou_getTable(table))
			error_at_line(EXIT_FAILURE, 0, file_name, start_event->start_mark.line + 1,
					"Table %s not valid", table);
	} else if (event.type == YAML_SCALAR_EVENT) {
		yaml_char_t *p = event.data.scalar.value;
		if (*p)
			while (p[1]) p++;
		if (*p == 10 || *p == 13) {
			// If the scalar ends with a newline, assume it is a block
			// scalar, so treat as an inline table. (Is there a proper way
			// to check for a block scalar?)
			sprintf(table, "%s%d", inline_table_prefix, rand());
			p = event.data.scalar.value;
			yaml_char_t *line_start = p;
			int line_len = 0;
			while (*p) {
				if (*p == 10 || *p == 13) {
					char *line = strndup((const char *)line_start, line_len);
					lou_compileString(table, line);
					free(line);
					line_start = p + 1;
					line_len = 0;
				} else {
					line_len++;
				}
				p++;
			}
		} else {
			strcat(table, (const char *)event.data.scalar.value);
		}
		yaml_event_delete(&event);
	} else {  // event.type == YAML_MAPPING_START_EVENT
		char *query;
		const char *table_file_name_check = NULL;
		yaml_event_delete(&event);
		query = read_table_query(parser, &table_file_name_check);
		table = lou_findTable(query);
		free(query);
		if (!table)
			error_at_line(EXIT_FAILURE, 0, file_name, start_event->start_mark.line + 1,
					"Table query did not match a table");
		if (table_file_name_check) {
			const char *table_file_name = table;
			do {
				table_file_name++;
			} while (*table_file_name);
			while (table_file_name >= table && *table_file_name != '/' &&
					*table_file_name != '\\')
				table_file_name--;
			if (strcmp(table_file_name_check, table_file_name + 1))
				error_at_line(EXIT_FAILURE, 0, file_name,
						start_event->start_mark.line + 1,
						"Table query did not match expected table: expected '%s' but got "
						"'%s'",
						table_file_name_check, table_file_name + 1);
		}
	}
	if (display_table) {
		char *t = table;
		table = malloc(strlen(display_table) + 1 + strlen(t) + 1);
		strcpy(table, display_table);
		strcat(table, ",");
		strcat(table, t);
		free(t);
	}
	emph_classes = lou_getEmphClasses(table);  // get declared emphasis classes
	return table;
}

static void
read_flags(yaml_parser_t *parser, int *direction, int *hyphenation) {
	yaml_event_t event;
	int parse_error = 1;

	*direction = DIRECTION_DEFAULT;
	*hyphenation = HYPHENATION_DEFAULT;

	if (!yaml_parser_parse(parser, &event) || (event.type != YAML_MAPPING_START_EVENT))
		yaml_error(YAML_MAPPING_START_EVENT, &event);

	yaml_event_delete(&event);

	while ((parse_error = yaml_parser_parse(parser, &event)) &&
			(event.type == YAML_SCALAR_EVENT)) {
		if (!strcmp((const char *)event.data.scalar.value, "testmode")) {
			yaml_event_delete(&event);
			if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SCALAR_EVENT))
				yaml_error(YAML_SCALAR_EVENT, &event);
			if (!strcmp((const char *)event.data.scalar.value, "forward")) {
				*direction = DIRECTION_FORWARD;
			} else if (!strcmp((const char *)event.data.scalar.value, "backward")) {
				*direction = DIRECTION_BACKWARD;
			} else if (!strcmp((const char *)event.data.scalar.value, "bothDirections")) {
				*direction = DIRECTION_BOTH;
			} else if (!strcmp((const char *)event.data.scalar.value, "hyphenate")) {
				*hyphenation = HYPHENATION_ON;
			} else {
				error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
						"Testmode '%s' not supported\n", event.data.scalar.value);
			}
		} else {
			error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
					"Flag '%s' not supported\n", event.data.scalar.value);
		}
	}
	if (!parse_error) yaml_parse_error(parser);
	if (event.type != YAML_MAPPING_END_EVENT) yaml_error(YAML_MAPPING_END_EVENT, &event);
	yaml_event_delete(&event);
}

static int
read_xfail(yaml_parser_t *parser) {
	yaml_event_t event;
	/* assume xfail true if there is an xfail key */
	int xfail = 1;
	if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SCALAR_EVENT))
		yaml_error(YAML_SCALAR_EVENT, &event);
	if (!strcmp((const char *)event.data.scalar.value, "false") ||
			!strcmp((const char *)event.data.scalar.value, "off"))
		xfail = 0;
	yaml_event_delete(&event);
	return xfail;
}

static translationModes
read_mode(yaml_parser_t *parser) {
	yaml_event_t event;
	translationModes mode = 0;
	int parse_error = 1;

	if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SEQUENCE_START_EVENT))
		yaml_error(YAML_SEQUENCE_START_EVENT, &event);
	yaml_event_delete(&event);

	while ((parse_error = yaml_parser_parse(parser, &event)) &&
			(event.type == YAML_SCALAR_EVENT)) {
		if (!strcmp((const char *)event.data.scalar.value, "noContractions")) {
			mode |= noContractions;
		} else if (!strcmp((const char *)event.data.scalar.value, "compbrlAtCursor")) {
			mode |= compbrlAtCursor;
		} else if (!strcmp((const char *)event.data.scalar.value, "dotsIO")) {
			mode |= dotsIO;
		} else if (!strcmp((const char *)event.data.scalar.value, "compbrlLeftCursor")) {
			mode |= compbrlLeftCursor;
		} else if (!strcmp((const char *)event.data.scalar.value, "ucBrl")) {
			mode |= ucBrl;
		} else if (!strcmp((const char *)event.data.scalar.value, "noUndefinedDots")) {
			mode |= noUndefinedDots;
		} else if (!strcmp((const char *)event.data.scalar.value, "partialTrans")) {
			mode |= partialTrans;
		} else {
			error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
					"Mode '%s' not supported\n", event.data.scalar.value);
		}
		yaml_event_delete(&event);
	}
	if (!parse_error) yaml_parse_error(parser);
	if (event.type != YAML_SEQUENCE_END_EVENT)
		yaml_error(YAML_SEQUENCE_END_EVENT, &event);
	yaml_event_delete(&event);
	return mode;
}

static int
parse_number(const char *number, const char *name, int file_line) {
	char *tail;
	errno = 0;

	int val = strtol(number, &tail, 0);
	if (errno != 0)
		error_at_line(EXIT_FAILURE, 0, file_name, file_line,
				"Not a valid %s '%s'. Must be a number\n", name, number);
	if (number == tail)
		error_at_line(EXIT_FAILURE, 0, file_name, file_line,
				"No digits found in %s '%s'. Must be a number\n", name, number);
	return val;
}

static int *
read_inPos(yaml_parser_t *parser, int translen) {
	int *pos = malloc(sizeof(int) * translen);
	int i = 0;
	yaml_event_t event;
	int parse_error = 1;

	if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SEQUENCE_START_EVENT))
		yaml_error(YAML_SEQUENCE_START_EVENT, &event);
	yaml_event_delete(&event);

	while ((parse_error = yaml_parser_parse(parser, &event)) &&
			(event.type == YAML_SCALAR_EVENT)) {
		if (i >= translen)
			error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
					"Too many input positions for translation of length %d.", translen);

		pos[i++] = parse_number((const char *)event.data.scalar.value, "input position",
				event.start_mark.line + 1);
		yaml_event_delete(&event);
	}
	if (i < translen)
		error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
				"Too few input positions (%i) for translation of length %i\n", i,
				translen);
	if (!parse_error) yaml_parse_error(parser);
	if (event.type != YAML_SEQUENCE_END_EVENT)
		yaml_error(YAML_SEQUENCE_END_EVENT, &event);
	yaml_event_delete(&event);
	return pos;
}

static int *
read_outPos(yaml_parser_t *parser, int wrdlen, int translen) {
	int *pos = malloc(sizeof(int) * wrdlen);
	int i = 0;
	yaml_event_t event;
	int parse_error = 1;

	if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SEQUENCE_START_EVENT))
		yaml_error(YAML_SEQUENCE_START_EVENT, &event);
	yaml_event_delete(&event);

	while ((parse_error = yaml_parser_parse(parser, &event)) &&
			(event.type == YAML_SCALAR_EVENT)) {
		if (i >= wrdlen)
			error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
					"Too many output positions for input string of length %d.", translen);

		pos[i++] = parse_number((const char *)event.data.scalar.value, "output position",
				event.start_mark.line + 1);
		yaml_event_delete(&event);
	}
	if (i < wrdlen)
		error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
				"Too few output positions (%i) for input string of length %i\n", i,
				wrdlen);
	if (!parse_error) yaml_parse_error(parser);
	if (event.type != YAML_SEQUENCE_END_EVENT)
		yaml_error(YAML_SEQUENCE_END_EVENT, &event);
	yaml_event_delete(&event);
	return pos;
}

static void
read_cursorPos(yaml_parser_t *parser, int *cursorPos, int *expected_cursorPos, int wrdlen,
		int translen) {
	yaml_event_t event;

	if (!yaml_parser_parse(parser, &event) ||
			!(event.type == YAML_SEQUENCE_START_EVENT || event.type == YAML_SCALAR_EVENT))
		error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
				"Expected %s or %s (actual %s)", event_names[YAML_SEQUENCE_START_EVENT],
				event_names[YAML_SCALAR_EVENT], event_names[event.type]);

	if (event.type == YAML_SEQUENCE_START_EVENT) {
		/* it's a sequence: read the two cursor positions (before and after) */
		yaml_event_delete(&event);
		if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SCALAR_EVENT))
			yaml_error(YAML_SCALAR_EVENT, &event);
		*cursorPos = parse_number((const char *)event.data.scalar.value,
				"cursor position", event.start_mark.line + 1);
		if ((0 > *cursorPos) || (*cursorPos >= wrdlen))
			error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
					"Cursor position (%i) outside of input string of length %i\n",
					*cursorPos, wrdlen);
		yaml_event_delete(&event);
		if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SCALAR_EVENT))
			error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
					"Too few cursor positions, 2 are expected (before and after)\n");
		*expected_cursorPos = parse_number((const char *)event.data.scalar.value,
				"expected cursor position", event.start_mark.line + 1);
		if ((0 > *expected_cursorPos) || (*expected_cursorPos >= wrdlen))
			error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
					"Expected cursor position (%i) outside of output string of length "
					"%i\n",
					*expected_cursorPos, translen);
		yaml_event_delete(&event);
		if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SEQUENCE_END_EVENT))
			error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
					"Too many cursor positions, only 2 are expected (before and "
					"after)\n");
		yaml_event_delete(&event);
	} else {  // YAML_SCALAR_EVENT
		/* it's just a single value: just read the initial cursor position */
		*cursorPos = parse_number((const char *)event.data.scalar.value,
				"cursor position before", event.start_mark.line + 1);
		if ((0 > *cursorPos) || (*cursorPos >= wrdlen))
			error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
					"Cursor position (%i) outside of input string of length %i\n",
					*cursorPos, wrdlen);
		*expected_cursorPos = -1;
		yaml_event_delete(&event);
	}
}

static void
read_typeform_string(yaml_parser_t *parser, formtype *typeform, typeforms kind, int len) {
	yaml_event_t event;
	int typeform_len;

	if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SCALAR_EVENT))
		yaml_error(YAML_SCALAR_EVENT, &event);
	typeform_len = strlen((const char *)event.data.scalar.value);
	if (typeform_len != len)
		error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
				"Too many or too typeforms (%i) for word of length %i\n", typeform_len,
				len);
	update_typeform((const char *)event.data.scalar.value, typeform, kind);
	yaml_event_delete(&event);
}

static formtype *
read_typeforms(yaml_parser_t *parser, int len) {
	yaml_event_t event;
	formtype *typeform = calloc(len, sizeof(formtype));
	int parse_error = 1;

	if (!yaml_parser_parse(parser, &event) || (event.type != YAML_MAPPING_START_EVENT))
		yaml_error(YAML_MAPPING_START_EVENT, &event);
	yaml_event_delete(&event);

	while ((parse_error = yaml_parser_parse(parser, &event)) &&
			(event.type == YAML_SCALAR_EVENT)) {
		if (strcmp((const char *)event.data.scalar.value, "computer_braille") == 0) {
			yaml_event_delete(&event);
			read_typeform_string(parser, typeform, computer_braille, len);
		} else if (strcmp((const char *)event.data.scalar.value, "no_translate") == 0) {
			yaml_event_delete(&event);
			read_typeform_string(parser, typeform, no_translate, len);
		} else if (strcmp((const char *)event.data.scalar.value, "no_contract") == 0) {
			yaml_event_delete(&event);
			read_typeform_string(parser, typeform, no_contract, len);
		} else {
			int i;
			typeforms kind = plain_text;
			for (i = 0; emph_classes[i]; i++) {
				if (strcmp((const char *)event.data.scalar.value, emph_classes[i]) == 0) {
					yaml_event_delete(&event);
					kind = italic << i;
					if (kind > emph_10)
						error_at_line(EXIT_FAILURE, 0, file_name,
								event.start_mark.line + 1,
								"Typeform '%s' was not declared\n",
								event.data.scalar.value);
					read_typeform_string(parser, typeform, kind, len);
					break;
				}
			}
		}
	}
	if (!parse_error) yaml_parse_error(parser);

	if (event.type != YAML_MAPPING_END_EVENT) yaml_error(YAML_MAPPING_END_EVENT, &event);
	yaml_event_delete(&event);
	return typeform;
}

static void
read_options(yaml_parser_t *parser, int direction, int wordLen, int translationLen,
		int *xfail, translationModes *mode, formtype **typeform, int **inPos,
		int **outPos, int *cursorPos, int *cursorOutPos, int *maxOutputLen,
		int *realInputLen) {
	yaml_event_t event;
	char *option_name;
	int parse_error = 1;

	*mode = 0;
	*xfail = 0;
	*typeform = NULL;
	*inPos = NULL;
	*outPos = NULL;

	while ((parse_error = yaml_parser_parse(parser, &event)) &&
			(event.type == YAML_SCALAR_EVENT)) {
		option_name =
				strndup((const char *)event.data.scalar.value, event.data.scalar.length);

		if (!strcmp(option_name, "xfail")) {
			yaml_event_delete(&event);
			*xfail = read_xfail(parser);
		} else if (!strcmp(option_name, "mode")) {
			yaml_event_delete(&event);
			*mode = read_mode(parser);
		} else if (!strcmp(option_name, "typeform")) {
			if (direction != 0) {
				error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
						"typeforms only supported with testmode 'forward'\n");
			}
			yaml_event_delete(&event);
			*typeform = read_typeforms(parser, wordLen);
		} else if (!strcmp(option_name, "inputPos")) {
			yaml_event_delete(&event);
			*inPos = read_inPos(parser, translationLen);
		} else if (!strcmp(option_name, "outputPos")) {
			yaml_event_delete(&event);
			*outPos = read_outPos(parser, wordLen, translationLen);
		} else if (!strcmp(option_name, "cursorPos")) {
			if (direction == 2) {
				error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
						"cursorPos not supported with testmode 'bothDirections'\n");
			}
			yaml_event_delete(&event);
			read_cursorPos(parser, cursorPos, cursorOutPos, wordLen, translationLen);
		} else if (!strcmp(option_name, "maxOutputLength")) {
			if (direction == 2) {
				error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
						"maxOutputLength not supported with testmode 'bothDirections'\n");
			}
			yaml_event_delete(&event);
			if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SCALAR_EVENT))
				yaml_error(YAML_SCALAR_EVENT, &event);
			*maxOutputLen = parse_number((const char *)event.data.scalar.value,
					"Maximum output length", event.start_mark.line + 1);
			if (*maxOutputLen <= 0)
				error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
						"Maximum output length (%i) must be a positive number\n",
						*maxOutputLen);
			if (*maxOutputLen < translationLen)
				error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
						"Expected translation length (%i) must not exceed maximum output "
						"length (%i)\n",
						translationLen, *maxOutputLen);
			yaml_event_delete(&event);
		} else if (!strcmp(option_name, "realInputLength")) {
			yaml_event_delete(&event);
			if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SCALAR_EVENT))
				yaml_error(YAML_SCALAR_EVENT, &event);
			*realInputLen = parse_number((const char *)event.data.scalar.value,
					"Real input length", event.start_mark.line + 1);
			if (*realInputLen < 0)
				error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
						"Real input length (%i) must not be a negative number\n",
						*realInputLen);
			if (*realInputLen > wordLen)
				error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
						"Real input length (%i) must not exceed total input "
						"length (%i)\n",
						*realInputLen, wordLen);
			yaml_event_delete(&event);
		} else {
			error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
					"Unsupported option %s", option_name);
		}
		free(option_name);
	}
	if (!parse_error) yaml_parse_error(parser);
	if (event.type != YAML_MAPPING_END_EVENT) yaml_error(YAML_MAPPING_END_EVENT, &event);
	yaml_event_delete(&event);
}

/* see http://stackoverflow.com/questions/5117393/utf-8-strings-length-in-linux-c */
static int
my_strlen_utf8_c(char *s) {
	int i = 0, j = 0;
	while (s[i]) {
		if ((s[i] & 0xc0) != 0x80) j++;
		i++;
	}
	return j;
}

/*
 * String parsing is also done later in check_base. At this point we
 * only need it to compute the actual string length in order to be
 * able to provide error messages when parsing typeform and position arrays.
 */
static int
parsed_strlen(char *s) {
	widechar *buf;
	int len, maxlen;
	maxlen = my_strlen_utf8_c(s);
	buf = malloc(sizeof(widechar) * maxlen);
	len = _lou_extParseChars(s, buf);
	free(buf);
	return len;
}

static void
read_test(yaml_parser_t *parser, char **tables, int direction, int hyphenation) {
	yaml_event_t event;
	char *description = NULL;
	char *word;
	char *translation;
	int xfail = 0;
	translationModes mode = 0;
	formtype *typeform = NULL;
	int *inPos = NULL;
	int *outPos = NULL;
	int cursorPos = -1;
	int cursorOutPos = -1;
	int maxOutputLen = -1;
	int realInputLen = -1;

	if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SCALAR_EVENT))
		simple_error("Word expected", parser, &event);

	word = strndup((const char *)event.data.scalar.value, event.data.scalar.length);
	yaml_event_delete(&event);

	if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SCALAR_EVENT))
		simple_error("Translation expected", parser, &event);

	translation =
			strndup((const char *)event.data.scalar.value, event.data.scalar.length);
	yaml_event_delete(&event);

	if (!yaml_parser_parse(parser, &event)) yaml_parse_error(parser);

	/* Handle an optional description */
	if (event.type == YAML_SCALAR_EVENT) {
		description = word;
		word = translation;
		translation =
				strndup((const char *)event.data.scalar.value, event.data.scalar.length);
		yaml_event_delete(&event);

		if (!yaml_parser_parse(parser, &event)) yaml_parse_error(parser);
	}

	if (event.type == YAML_MAPPING_START_EVENT) {
		yaml_event_delete(&event);
		read_options(parser, direction, parsed_strlen(word), parsed_strlen(translation),
				&xfail, &mode, &typeform, &inPos, &outPos, &cursorPos, &cursorOutPos,
				&maxOutputLen, &realInputLen);

		if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SEQUENCE_END_EVENT))
			yaml_error(YAML_SEQUENCE_END_EVENT, &event);
	} else if (event.type != YAML_SEQUENCE_END_EVENT) {
		error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
				"Expected %s or %s (actual %s)", event_names[YAML_MAPPING_START_EVENT],
				event_names[YAML_SEQUENCE_END_EVENT], event_names[event.type]);
	}

	int result = 0;
	char **table = tables;
	while (*table) {
		if (hyphenation == HYPHENATION_ON) {
			result |= check_hyphenation(*table, word, translation);
		} else {
			// FIXME: Note that the typeform array was constructed using the
			// emphasis classes mapping of the last compiled table. This
			// means that if we are testing multiple tables at the same time
			// they must have the same mapping (i.e. the emphasis classes
			// must be defined in the same order).
			result |= check(*table, word, translation, .typeform = typeform, .mode = mode,
					.expected_inputPos = inPos, .expected_outputPos = outPos,
					.cursorPos = cursorPos, .expected_cursorPos = cursorOutPos,
					.max_outlen = maxOutputLen, .real_inlen = realInputLen,
					.direction = direction, .diagnostics = !xfail);
		}
		table++;
	}
	if (xfail != result) {
		if (description) fprintf(stderr, "%s\n", description);
		error_at_line(0, 0, file_name, event.start_mark.line + 1,
				(xfail ? "Unexpected Pass" : "Failure"));
		errors++;
	}
	yaml_event_delete(&event);
	count++;

	free(description);
	free(word);
	free(translation);
	free(typeform);
	free(inPos);
	free(outPos);
}

static void
read_tests(yaml_parser_t *parser, char **tables, int direction, int hyphenation) {
	yaml_event_t event;
	if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SEQUENCE_START_EVENT))
		yaml_error(YAML_SEQUENCE_START_EVENT, &event);

	yaml_event_delete(&event);

	int done = 0;
	while (!done) {
		if (!yaml_parser_parse(parser, &event)) {
			yaml_parse_error(parser);
		}
		if (event.type == YAML_SEQUENCE_END_EVENT) {
			done = 1;
			yaml_event_delete(&event);
		} else if (event.type == YAML_SEQUENCE_START_EVENT) {
			yaml_event_delete(&event);
			read_test(parser, tables, direction, hyphenation);
		} else {
			error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
					"Expected %s or %s (actual %s)", event_names[YAML_SEQUENCE_END_EVENT],
					event_names[YAML_SEQUENCE_START_EVENT], event_names[event.type]);
		}
	}
}

/*
 * This custom table resolver handles magic table names that represent
 * inline tables.
 */
static char **
customTableResolver(const char *tableList, const char *base) {
	static char *dummy_table[1];
	if (strncmp(tableList, inline_table_prefix, strlen(inline_table_prefix)) == 0)
		return dummy_table;
	return _lou_defaultTableResolver(tableList, base);
}

#endif  // HAVE_LIBYAML

int
main(int argc, char *argv[]) {
	int optc;

	set_program_name(argv[0]);

	while ((optc = getopt_long(argc, argv, "hv", longopts, NULL)) != -1) switch (optc) {
		/* --help and --version exit immediately, per GNU coding standards.  */
		case 'v':
			version_etc(
					stdout, program_name, PACKAGE_NAME, VERSION, AUTHORS, (char *)NULL);
			exit(EXIT_SUCCESS);
			break;
		case 'h':
			print_help();
			exit(EXIT_SUCCESS);
			break;
		default:
			fprintf(stderr, "Try `%s --help' for more information.\n", program_name);
			exit(EXIT_FAILURE);
			break;
		}

	if (optind != argc - 1) {
		/* Print error message and exit.  */
		if (optind < argc - 1)
			fprintf(stderr, "%s: extra operand: %s\n", program_name, argv[optind + 1]);
		else
			fprintf(stderr, "%s: no YAML test file specified\n", program_name);

		fprintf(stderr, "Try `%s --help' for more information.\n", program_name);
		exit(EXIT_FAILURE);
	}

#ifdef WITHOUT_YAML
	fprintf(stderr,
			"Skipping tests for %s as yaml was disabled in configure with "
			"--without-yaml\n",
			argv[1]);
	return EXIT_SKIPPED;
#else
#ifndef HAVE_LIBYAML
	fprintf(stderr, "Skipping tests for %s as libyaml was not found\n", argv[1]);
	return EXIT_SKIPPED;
#endif  // not HAVE_LIBYAML
#endif  // WITHOUT_YAML

#ifndef WITHOUT_YAML
#ifdef HAVE_LIBYAML

	FILE *file;
	yaml_parser_t parser;
	yaml_event_t event;

	file_name = argv[1];
	file = fopen(file_name, "rb");
	if (!file) {
		fprintf(stderr, "%s: file not found: %s\n", program_name, file_name);
		exit(3);
	}

	char *dir_name = strdup(file_name);
	int i = strlen(dir_name);
	while (i > 0) {
		if (dir_name[i - 1] == '/' || dir_name[i - 1] == '\\') {
			i--;
			break;
		}
		i--;
	}
	dir_name[i] = '\0';
	// FIXME: problem with this is that
	// LOUIS_TABLEPATH=$(top_srcdir)/tables,... does not work anymore because
	// $(top_srcdir) == .. (not an absolute path)
	if (i > 0)
		if (chdir(dir_name))
			error(EXIT_FAILURE, EIO, "Cannot change directory to %s", dir_name);

	// register custom table resolver
	lou_registerTableResolver(&customTableResolver);

	assert(yaml_parser_initialize(&parser));

	yaml_parser_set_input_file(&parser, file);

	if (!yaml_parser_parse(&parser, &event) || (event.type != YAML_STREAM_START_EVENT)) {
		yaml_error(YAML_STREAM_START_EVENT, &event);
	}

	if (event.data.stream_start.encoding != YAML_UTF8_ENCODING)
		error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
				"UTF-8 encoding expected (actual %s)",
				encoding_names[event.data.stream_start.encoding]);
	yaml_event_delete(&event);

	if (!yaml_parser_parse(&parser, &event) ||
			(event.type != YAML_DOCUMENT_START_EVENT)) {
		yaml_error(YAML_DOCUMENT_START_EVENT, &event);
	}
	yaml_event_delete(&event);

	if (!yaml_parser_parse(&parser, &event) || (event.type != YAML_MAPPING_START_EVENT)) {
		yaml_error(YAML_MAPPING_START_EVENT, &event);
	}
	yaml_event_delete(&event);

	int has_next;
	has_next = yaml_parser_parse(&parser, &event);

	const char *display_table = NULL;
	if (has_next && event.type == YAML_SCALAR_EVENT &&
			!strcmp((const char *)event.data.scalar.value, "display")) {
		yaml_event_delete(&event);
		if (!yaml_parser_parse(&parser, &event) || event.type != YAML_SCALAR_EVENT)
			yaml_error(YAML_SCALAR_EVENT, &event);
		display_table =
				strndup((const char *)event.data.scalar.value, event.data.scalar.length);
		yaml_event_delete(&event);
		has_next = yaml_parser_parse(&parser, &event);
	}

	if (!has_next) simple_error("table expected", &parser, &event);

	int MAXTABLES = 10;
	char *tables[MAXTABLES + 1];
	while ((tables[0] = read_table(&event, &parser, display_table))) {
		yaml_event_delete(&event);
		int k = 1;
		while (1) {
			if (!yaml_parser_parse(&parser, &event))
				error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
						"Expected table or %s (actual %s)",
						event_names[YAML_SCALAR_EVENT], event_names[event.type]);
			if ((tables[k++] = read_table(&event, &parser, display_table))) {
				if (k == MAXTABLES) exit(EXIT_FAILURE);
				yaml_event_delete(&event);
			} else
				break;
		}

		if (event.type != YAML_SCALAR_EVENT) yaml_error(YAML_SCALAR_EVENT, &event);

		int haveRunTests = 0;
		while (1) {
			int direction = DIRECTION_DEFAULT;
			int hyphenation = HYPHENATION_DEFAULT;
			if (!strcmp((const char *)event.data.scalar.value, "flags")) {
				yaml_event_delete(&event);
				read_flags(&parser, &direction, &hyphenation);

				if (!yaml_parser_parse(&parser, &event) ||
						(event.type != YAML_SCALAR_EVENT) ||
						strcmp((const char *)event.data.scalar.value, "tests")) {
					simple_error("tests expected", &parser, &event);
				}
				yaml_event_delete(&event);
				read_tests(&parser, tables, direction, hyphenation);
				haveRunTests = 1;

			} else if (!strcmp((const char *)event.data.scalar.value, "tests")) {
				yaml_event_delete(&event);
				read_tests(&parser, tables, direction, hyphenation);
				haveRunTests = 1;
			} else {
				if (haveRunTests) {
					break;
				} else {
					simple_error("flags or tests expected", &parser, &event);
				}
			}
			if (!yaml_parser_parse(&parser, &event))
				error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
						"Expected table, flags, tests or %s (actual %s)",
						event_names[YAML_MAPPING_END_EVENT], event_names[event.type]);
			if (event.type != YAML_SCALAR_EVENT) break;
		}

		char **p = tables;
		while (*p) free(*(p++));
	}
	if (event.type != YAML_MAPPING_END_EVENT) yaml_error(YAML_MAPPING_END_EVENT, &event);
	yaml_event_delete(&event);

	if (!yaml_parser_parse(&parser, &event) || (event.type != YAML_DOCUMENT_END_EVENT)) {
		yaml_error(YAML_DOCUMENT_END_EVENT, &event);
	}
	yaml_event_delete(&event);

	if (!yaml_parser_parse(&parser, &event) || (event.type != YAML_STREAM_END_EVENT)) {
		yaml_error(YAML_STREAM_END_EVENT, &event);
	}
	yaml_event_delete(&event);

	yaml_parser_delete(&parser);

	free(emph_classes);
	lou_free();

	assert(!fclose(file));

	printf("%s (%d tests, %d failure%s)\n", (errors ? "FAILURE" : "SUCCESS"), count,
			errors, ((errors != 1) ? "s" : ""));

	return errors ? 1 : 0;

#endif  // HAVE_LIBYAML
#endif  // not WITHOUT_YAML
}
