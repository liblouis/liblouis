/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2024 liblouis contributors

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

/* Test backtranslation typeform/emphasis detection.
 * This tests that emphasis indicators in braille are correctly
 * recognized and the typeform buffer is populated during backtranslation.
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "liblouis.h"
#include "internal.h"

static int errors = 0;

static void
print_typeform(const char *name, const formtype *typeform, int len) {
	int i;
	printf("  %s typeform: [", name);
	for (i = 0; i < len; i++) {
		printf("%d", typeform[i]);
		if (i < len - 1) printf(", ");
	}
	printf("]\n");
}

static int
test_backtranslation_typeform(const char *table, const widechar *input, int inlen,
		const char *expected_output, const formtype *expected_typeform,
		const char *test_name) {
	widechar outbuf[256];
	formtype typeform[256];
	int outlen = 256;
	int input_len = inlen;
	int i;
	int success = 1;

	memset(outbuf, 0, sizeof(outbuf));
	memset(typeform, 0, sizeof(typeform));

	if (!lou_backTranslate(table, input, &input_len, outbuf, &outlen, typeform, NULL,
				NULL, NULL, NULL, 0)) {
		printf("FAIL: %s - lou_backTranslate failed\n", test_name);
		errors++;
		return 0;
	}

	/* Check output text */
	char output_str[256];
	for (i = 0; i < outlen && i < 255; i++) {
		output_str[i] = (char)outbuf[i];
	}
	output_str[outlen] = '\0';

	if (strcmp(output_str, expected_output) != 0) {
		printf("FAIL: %s - output mismatch\n", test_name);
		printf("  Expected: '%s'\n", expected_output);
		printf("  Got:      '%s'\n", output_str);
		success = 0;
	}

	/* Check typeform */
	int typeform_match = 1;
	for (i = 0; i < outlen; i++) {
		if (typeform[i] != expected_typeform[i]) {
			typeform_match = 0;
			break;
		}
	}

	if (!typeform_match) {
		printf("FAIL: %s - typeform mismatch\n", test_name);
		print_typeform("Expected", expected_typeform, outlen);
		print_typeform("Got", typeform, outlen);
		success = 0;
	}

	if (success) {
		printf("PASS: %s\n", test_name);
	} else {
		errors++;
	}

	return success;
}

int
main(int argc, char **argv) {
	const char *table = "tables/en-ueb-g1.ctb";

	printf("Testing backtranslation emphasis detection...\n\n");

	/* Test 1: Plain text - no emphasis */
	{
		/* "hello" in UEB */
		widechar input[] = { 0x2813, 0x2811, 0x2807, 0x2807, 0x2815 }; /* h e l l o */
		formtype expected_typeform[] = { 0, 0, 0, 0, 0 };
		test_backtranslation_typeform(table, input, 5, "hello", expected_typeform,
				"Plain text - no emphasis");
	}

	/* Test 2: Single letter italic (dots 46, 23 prefix) */
	{
		/* italic 't' followed by "est" -> "test" with first char italic */
		/* dots 46-23 = emphletter italic, then t e s t */
		widechar input[] = { 0x2828, 0x2806, /* dots 46, 23 = italic letter indicator */
			0x281e,						   /* t */
			0x2811,						   /* e */
			0x280e,						   /* s */
			0x281e						   /* t */
		};
		formtype expected_typeform[] = { 1, 0, 0, 0 }; /* only first char italic */
		test_backtranslation_typeform(table, input, 6, "test", expected_typeform,
				"Single letter italic");
	}

	/* Test 3: Word italic (dots 46, 2 prefix, dots 46, 3 suffix) */
	{
		/* begemphword italic = dots 46-2, endemphword italic = dots 46-3 */
		/* "word" with word italic */
		widechar input[] = { 0x2828, 0x2802, /* dots 46-2 = begemphword italic */
			0x283a,						   /* w */
			0x2815,						   /* o */
			0x2817,						   /* r */
			0x2819,						   /* d */
			0x2828, 0x2804				   /* dots 46-3 = endemphword italic */
		};
		formtype expected_typeform[] = { 1, 1, 1, 1 }; /* all chars italic */
		test_backtranslation_typeform(table, input, 8, "word", expected_typeform,
				"Word italic");
	}

	/* Test 4: Single letter bold (dots 45, 23 prefix) */
	{
		/* bold 't' followed by "est" -> "test" with first char bold */
		/* dots 45-23 = emphletter bold, then t e s t */
		widechar input[] = { 0x2818, 0x2806, /* dots 45, 23 = bold letter indicator */
			0x281e,						   /* t */
			0x2811,						   /* e */
			0x280e,						   /* s */
			0x281e						   /* t */
		};
		formtype expected_typeform[] = { 4, 0, 0, 0 }; /* only first char bold (emph_3=4) */
		test_backtranslation_typeform(table, input, 6, "test", expected_typeform,
				"Single letter bold");
	}

	/* Test 5: Word bold (dots 45-2 prefix, dots 45-3 suffix) */
	{
		/* begemphword bold = dots 45-2, endemphword bold = dots 45-3 */
		widechar input[] = { 0x2818, 0x2802, /* dots 45-2 = begemphword bold */
			0x283a,						   /* w */
			0x2815,						   /* o */
			0x2817,						   /* r */
			0x2819,						   /* d */
			0x2818, 0x2804				   /* dots 45-3 = endemphword bold */
		};
		formtype expected_typeform[] = { 4, 4, 4, 4 }; /* all chars bold */
		test_backtranslation_typeform(table, input, 8, "word", expected_typeform,
				"Word bold");
	}

	/* Test 6: Mixed text - some italic, some plain */
	{
		/* "a" plain, "b" italic letter, "c" plain -> "abc" */
		widechar input[] = { 0x2801,		 /* a */
			0x2828, 0x2806,					 /* italic letter indicator */
			0x2803,							 /* b */
			0x2809							 /* c */
		};
		formtype expected_typeform[] = { 0, 1, 0 }; /* a=0, b=1, c=0 */
		test_backtranslation_typeform(table, input, 5, "abc", expected_typeform,
				"Mixed text - some italic");
	}

	/* Test 7: Phrase italic (dots 46-2356 prefix, dots 46-3 suffix) */
	{
		/* begemphphrase italic = dots 46-2356, endemphphrase italic = dots 46-3 */
		/* "hi there" with phrase italic */
		widechar input[] = { 0x2828, 0x2836, /* dots 46-2356 = begemphphrase italic */
			0x2813,						   /* h */
			0x280a,						   /* i */
			0x2800,						   /* space */
			0x281e,						   /* t */
			0x2813,						   /* h */
			0x2811,						   /* e */
			0x2817,						   /* r */
			0x2811,						   /* e */
			0x2828, 0x2804				   /* dots 46-3 = endemphphrase italic */
		};
		formtype expected_typeform[] = { 1, 1, 1, 1, 1, 1, 1, 1 }; /* all chars italic */
		test_backtranslation_typeform(table, input, 12, "hi there", expected_typeform,
				"Phrase italic");
	}

	/* Test 8: Single letter underline (dots 456-23 prefix) */
	{
		/* underline 't' followed by "est" -> "test" with first char underline */
		/* dots 456-23 = emphletter underline, then t e s t */
		widechar input[] = { 0x2838, 0x2806, /* dots 456, 23 = underline letter indicator */
			0x281e,						   /* t */
			0x2811,						   /* e */
			0x280e,						   /* s */
			0x281e						   /* t */
		};
		formtype expected_typeform[] = { 2, 0, 0, 0 }; /* only first char underline (emph_2=2) */
		test_backtranslation_typeform(table, input, 6, "test", expected_typeform,
				"Single letter underline");
	}

	/* Test 9: Word emphasis with explicit terminator */
	{
		/* Word italic for "hi" with explicit endemphword, then space and "there" plain */
		widechar input[] = { 0x2828, 0x2802, /* dots 46-2 = begemphword italic */
			0x2813,						   /* h */
			0x280a,						   /* i */
			0x2828, 0x2804,				   /* dots 46-3 = endemphword italic */
			0x2800,						   /* space */
			0x281e,						   /* t */
			0x2813,						   /* h */
			0x2811,						   /* e */
			0x2817,						   /* r */
			0x2811						   /* e */
		};
		formtype expected_typeform[] = { 1, 1, 0, 0, 0, 0, 0, 0 }; /* hi italic, space and there plain */
		test_backtranslation_typeform(table, input, 12, "hi there", expected_typeform,
				"Word emphasis with explicit terminator");
	}

	lou_free();

	printf("\n");
	if (errors == 0) {
		printf("All tests passed!\n");
		return 0;
	} else {
		printf("%d test(s) failed.\n", errors);
		return 1;
	}
}
