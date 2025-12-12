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

	/* Test 10: Nested emphasis - bold word inside italic phrase */
	{
		/* Italic phrase containing a bold word: "hi BOLD there"
		 * where BOLD is both italic (from phrase) and bold (from word)
		 * Expected typeform: italic=1, bold=4, both=5 */
		widechar input[] = {
			0x2828, 0x2836,	/* dots 46-2356 = begemphphrase italic */
			0x2813,			/* h */
			0x280a,			/* i */
			0x2800,			/* space */
			0x2818, 0x2802,	/* dots 45-2 = begemphword bold */
			0x2803,			/* b */
			0x2815,			/* o */
			0x2807,			/* l */
			0x2819,			/* d */
			0x2818, 0x2804,	/* dots 45-3 = endemphword bold */
			0x2800,			/* space */
			0x281e,			/* t */
			0x2813,			/* h */
			0x2811,			/* e */
			0x2817,			/* r */
			0x2811,			/* e */
			0x2828, 0x2804	/* dots 46-3 = endemphphrase italic */
		};
		/* hi=italic(1), space=italic(1), bold=italic+bold(5), space=italic(1), there=italic(1) */
		formtype expected_typeform[] = { 1, 1, 1, 5, 5, 5, 5, 1, 1, 1, 1, 1, 1 };
		test_backtranslation_typeform(table, input, 21, "hi bold there", expected_typeform,
				"Nested emphasis - bold word inside italic phrase");
	}

	/* Test 11: Multiple word emphasis - both italic and bold on same word */
	{
		/* Word with both italic and bold indicators: "test"
		 * begemphword italic + begemphword bold ... endemphword bold + endemphword italic */
		widechar input[] = {
			0x2828, 0x2802,	/* dots 46-2 = begemphword italic */
			0x2818, 0x2802,	/* dots 45-2 = begemphword bold */
			0x281e,			/* t */
			0x2811,			/* e */
			0x280e,			/* s */
			0x281e,			/* t */
			0x2818, 0x2804,	/* dots 45-3 = endemphword bold */
			0x2828, 0x2804	/* dots 46-3 = endemphword italic */
		};
		/* All chars should be italic(1) + bold(4) = 5 */
		formtype expected_typeform[] = { 5, 5, 5, 5 };
		test_backtranslation_typeform(table, input, 12, "test", expected_typeform,
				"Multiple word emphasis - italic and bold on same word");
	}

	/* Test 12: Letter emphasis inside word emphasis */
	{
		/* Word italic with one letter also underlined: "test" where 'e' is also underlined
		 * begemphword italic, t, underline-letter e, s, t, endemphword italic */
		widechar input[] = {
			0x2828, 0x2802,	/* dots 46-2 = begemphword italic */
			0x281e,			/* t */
			0x2838, 0x2806,	/* dots 456-23 = emphletter underline */
			0x2811,			/* e */
			0x280e,			/* s */
			0x281e,			/* t */
			0x2828, 0x2804	/* dots 46-3 = endemphword italic */
		};
		/* t=italic(1), e=italic+underline(3), s=italic(1), t=italic(1) */
		formtype expected_typeform[] = { 1, 3, 1, 1 };
		test_backtranslation_typeform(table, input, 10, "test", expected_typeform,
				"Letter emphasis inside word emphasis");
	}

	/* Test 13: Multiple phrase emphasis - italic and bold phrases overlapping */
	{
		/* Bold phrase containing start of italic phrase: "ab cd"
		 * Bold phrase covers "ab c", italic phrase covers "cd"
		 *
		 * Note: In UEB, endemphphrase and endemphword use the same dot pattern (e.g., 45-3
		 * for bold). When backtranslating, the implementation may match endemphword first,
		 * which only clears activeWordEmphasis (not activePhraseEmphasis). This means
		 * the phrase emphasis continues until explicitly ended. This is a known limitation
		 * when phrase and word terminators share the same dots.
		 *
		 * Current behavior: 'd' remains bold+italic (5) because endemphphrase bold
		 * is interpreted as endemphword bold (which has no effect since no word bold
		 * was started).
		 */
		widechar input[] = {
			0x2818, 0x2836,	/* dots 45-2356 = begemphphrase bold */
			0x2801,			/* a */
			0x2803,			/* b */
			0x2800,			/* space */
			0x2828, 0x2836,	/* dots 46-2356 = begemphphrase italic */
			0x2809,			/* c */
			0x2818, 0x2804,	/* dots 45-3 = endemphphrase bold (matched as endemphword) */
			0x2819,			/* d */
			0x2828, 0x2804	/* dots 46-3 = endemphphrase italic */
		};
		/* Due to the limitation above, 'd' gets bold+italic(5) instead of italic(1) */
		formtype expected_typeform[] = { 4, 4, 4, 5, 5 };
		test_backtranslation_typeform(table, input, 13, "ab cd", expected_typeform,
				"Overlapping phrase emphasis - bold then italic (known limitation)");
	}

	/* Test 14: Three simultaneous emphasis types */
	{
		/* Single letter with italic + underline + bold all active
		 * Letter indicators for all three on 't', followed by plain "est" */
		widechar input[] = {
			0x2828, 0x2806,	/* dots 46-23 = emphletter italic */
			0x2838, 0x2806,	/* dots 456-23 = emphletter underline */
			0x2818, 0x2806,	/* dots 45-23 = emphletter bold */
			0x281e,			/* t */
			0x2811,			/* e */
			0x280e,			/* s */
			0x281e			/* t */
		};
		/* t=italic+underline+bold(7), e=0, s=0, t=0 */
		formtype expected_typeform[] = { 7, 0, 0, 0 };
		test_backtranslation_typeform(table, input, 10, "test", expected_typeform,
				"Three simultaneous letter emphasis types");
	}

	/* Test 15: Word emphasis ending mid-word with different emphasis starting */
	{
		/* Tests transition from italic word emphasis to bold word emphasis
		 * on the same continuous text without spaces */
		widechar input[] = {
			0x2828, 0x2802,	/* dots 46-2 = begemphword italic */
			0x2801,			/* a */
			0x2803,			/* b */
			0x2828, 0x2804,	/* dots 46-3 = endemphword italic */
			0x2818, 0x2802,	/* dots 45-2 = begemphword bold */
			0x2809,			/* c */
			0x2819,			/* d */
			0x2818, 0x2804	/* dots 45-3 = endemphword bold */
		};
		/* ab=italic, cd=bold */
		formtype expected_typeform[] = { 1, 1, 4, 4 };
		test_backtranslation_typeform(table, input, 12, "abcd", expected_typeform,
				"Emphasis type transition within word");
	}

	/* Test 16: Punctuation within word emphasis */
	{
		/* Word italic on "test" followed by punctuation, then terminator
		 * Punctuation after emphasized word retains emphasis until terminator */
		widechar input[] = {
			0x2828, 0x2802,	/* dots 46-2 = begemphword italic */
			0x281e,			/* t */
			0x2811,			/* e */
			0x280e,			/* s */
			0x281e,			/* t */
			0x2816,			/* ! (exclamation mark, dots 235) */
			0x2828, 0x2804	/* dots 46-3 = endemphword italic */
		};
		/* all characters including punctuation get italic */
		formtype expected_typeform[] = { 1, 1, 1, 1, 1 };
		test_backtranslation_typeform(table, input, 10, "test!", expected_typeform,
				"Punctuation at end of word emphasis");
	}

	/* Test 17: Word emphasis at end of input (no terminator, no space) */
	{
		/* begemphword italic + "test" + [END OF INPUT]
		 * Emphasis should apply even without explicit terminator */
		widechar input[] = {
			0x2828, 0x2802,	/* dots 46-2 = begemphword italic */
			0x281e,			/* t */
			0x2811,			/* e */
			0x280e,			/* s */
			0x281e			/* t */
		};
		/* All characters get italic even without terminator */
		formtype expected_typeform[] = { 1, 1, 1, 1 };
		test_backtranslation_typeform(table, input, 6, "test", expected_typeform,
				"Word emphasis at end of input");
	}

	/* Test 18: Phrase emphasis not terminated at end of input */
	{
		/* begemphphrase italic + "hello" + [END OF INPUT]
		 * Phrase emphasis should apply even without explicit terminator */
		widechar input[] = {
			0x2828, 0x2836,	/* dots 46-2356 = begemphphrase italic */
			0x2813,			/* h */
			0x2811,			/* e */
			0x2807,			/* l */
			0x2807,			/* l */
			0x2815			/* o */
		};
		/* All characters get italic even without terminator */
		formtype expected_typeform[] = { 1, 1, 1, 1, 1 };
		test_backtranslation_typeform(table, input, 7, "hello", expected_typeform,
				"Phrase emphasis at end of input");
	}

	/* Test 19: Letter emphasis indicator at end of input (no following character) */
	{
		/* "a" + emphletter italic + [END OF INPUT]
		 * Should not crash; 'a' should be plain */
		widechar input[] = {
			0x2801,			/* a */
			0x2828, 0x2806	/* dots 46-23 = emphletter italic (nothing follows) */
		};
		/* 'a' is plain; emphasis indicator has no character to apply to */
		formtype expected_typeform[] = { 0 };
		test_backtranslation_typeform(table, input, 3, "a", expected_typeform,
				"Letter emphasis at end of input");
	}

	/* Test 20: Emphasis on single character word */
	{
		/* Letter italic + "a" + space + plain "bc"
		 * Single char with letter emphasis, then plain text */
		widechar input[] = {
			0x2828, 0x2806,	/* dots 46-23 = emphletter italic */
			0x2801,			/* a */
			0x2800,			/* space */
			0x2803,			/* b */
			0x2809			/* c */
		};
		/* a=italic, space and bc=plain */
		formtype expected_typeform[] = { 1, 0, 0, 0 };
		test_backtranslation_typeform(table, input, 6, "a bc", expected_typeform,
				"Single character letter emphasis");
	}

	/* Test 21: Letter emphasis followed by word emphasis on same word */
	{
		/* emphletter italic on 't', then begemphword bold on "est"
		 * Result: "test" where 't' is italic, "est" is bold */
		widechar input[] = {
			0x2828, 0x2806,	/* dots 46-23 = emphletter italic */
			0x281e,			/* t */
			0x2818, 0x2802,	/* dots 45-2 = begemphword bold */
			0x2811,			/* e */
			0x280e,			/* s */
			0x281e,			/* t */
			0x2818, 0x2804	/* dots 45-3 = endemphword bold */
		};
		/* t=italic(1), est=bold(4) */
		formtype expected_typeform[] = { 1, 4, 4, 4 };
		test_backtranslation_typeform(table, input, 10, "test", expected_typeform,
				"Letter then word emphasis");
	}

	/* Test 22: Emphasis indicators only (no content) */
	{
		/* begemphword italic + endemphword (no actual content between)
		 * Should produce empty output with no typeform */
		widechar input[] = {
			0x2828, 0x2802,	/* dots 46-2 = begemphword italic */
			0x2828, 0x2804	/* dots 46-3 = endemphword italic */
		};
		formtype expected_typeform[] = { 0 }; /* dummy, output should be empty */
		/* This tests that empty emphasis blocks don't crash */
		test_backtranslation_typeform(table, input, 4, "", expected_typeform,
				"Emphasis indicators only (no content)");
	}

	/* Test 23: Phrase emphasis across multiple words with punctuation */
	{
		/* Italic phrase: "hi, there!" with comma and exclamation */
		widechar input[] = {
			0x2828, 0x2836,	/* dots 46-2356 = begemphphrase italic */
			0x2813,			/* h */
			0x280a,			/* i */
			0x2802,			/* , (comma) */
			0x2800,			/* space */
			0x281e,			/* t */
			0x2813,			/* h */
			0x2811,			/* e */
			0x2817,			/* r */
			0x2811,			/* e */
			0x2816,			/* ! */
			0x2828, 0x2804	/* dots 46-3 = endemphphrase italic */
		};
		/* All characters including punctuation get italic */
		formtype expected_typeform[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
		test_backtranslation_typeform(table, input, 14, "hi, there!", expected_typeform,
				"Phrase emphasis with punctuation");
	}

	/* Test 24: Word emphasis terminated by space (no explicit terminator)
	 * This is the key test for implicit word boundary termination.
	 * begemphword + "test" + space + "abc" should result in:
	 * "test" with emphasis, space with emphasis (part of word boundary), "abc" plain */
	{
		widechar input[] = {
			0x2828, 0x2802,	/* dots 46-2 = begemphword italic */
			0x281e,			/* t */
			0x2811,			/* e */
			0x280e,			/* s */
			0x281e,			/* t */
			0x2800,			/* space - should clear word emphasis */
			0x2801,			/* a */
			0x2803,			/* b */
			0x2809			/* c */
		};
		/* test=italic, space=italic (word boundary), abc=plain */
		formtype expected_typeform[] = { 1, 1, 1, 1, 1, 0, 0, 0 };
		test_backtranslation_typeform(table, input, 10, "test abc", expected_typeform,
				"Word emphasis terminated by space (implicit)");
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
