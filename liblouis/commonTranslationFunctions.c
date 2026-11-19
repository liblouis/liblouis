/* liblouis Braille Translation and Back-Translation Library

   Based on the Linux screenreader BRLTTY, copyright (C) 1999-2006 by The
   BRLTTY Team

   Copyright (C) 2004, 2005, 2006 ViewPlus Technologies, Inc. www.viewplus.com
   Copyright (C) 2004, 2005, 2006 JJB Software, Inc. www.jjb-software.com
   Copyright (C) 2016 Mike Gray, American Printing House for the Blind
   Copyright (C) 2016 Davy Kager, Dedicon

   This file is part of liblouis.

   liblouis is free software: you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as published
   by the Free Software Foundation, either version 2.1 of the License, or
   (at your option) any later version.

   liblouis is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with liblouis. If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"

#include <string.h>
#include "internal.h"

static int passVariables[NUMVAR];

void EXPORT_CALL
_lou_resetPassVariables(void) {
	memset(passVariables, 0, sizeof(passVariables[0]) * NUMVAR);
}

int EXPORT_CALL
_lou_handlePassVariableTest(const widechar *instructions, int *IC, int *itsTrue) {
	switch (instructions[*IC]) {
	case pass_eq:
		if (passVariables[instructions[*IC + 1]] != instructions[*IC + 2]) *itsTrue = 0;
		*IC += 3;
		return 1;

	case pass_lt:
		if (passVariables[instructions[*IC + 1]] >= instructions[*IC + 2]) *itsTrue = 0;
		*IC += 3;
		return 1;

	case pass_gt:
		if (passVariables[instructions[*IC + 1]] <= instructions[*IC + 2]) *itsTrue = 0;
		*IC += 3;
		return 1;

	case pass_lteq:
		if (passVariables[instructions[*IC + 1]] > instructions[*IC + 2]) *itsTrue = 0;
		*IC += 3;
		return 1;

	case pass_gteq:
		if (passVariables[instructions[*IC + 1]] < instructions[*IC + 2]) *itsTrue = 0;
		*IC += 3;
		return 1;

	default:
		return 0;
	}
}

int EXPORT_CALL
_lou_handlePassVariableAction(const widechar *instructions, int *IC) {
	switch (instructions[*IC]) {
	case pass_eq:
		passVariables[instructions[*IC + 1]] = instructions[*IC + 2];
		*IC += 3;
		return 1;

	case pass_hyphen:
		passVariables[instructions[*IC + 1]] -= 1;
		if (passVariables[instructions[*IC + 1]] < 0)
			passVariables[instructions[*IC + 1]] = 0;
		*IC += 2;
		return 1;

	case pass_plus:
		passVariables[instructions[*IC + 1]] += 1;
		*IC += 2;
		return 1;

	default:
		return 0;
	}
}

static const TranslationTableCharacter *
findChar(widechar c, const TranslationTableHeader *table) {
	static TranslationTableCharacter notFound = { NULL, -1, 0, 0, 0, CTC_Space, 0, 0, 32,
		0, 0 };
	unsigned long int makeHash = _lou_charHash(c);
	TranslationTableOffset bucket = table->characters[makeHash];
	while (bucket) {
		TranslationTableCharacter *character =
				(TranslationTableCharacter *)&table->ruleArea[bucket];
		if (character->value == c) return character;
		bucket = character->next;
	}
	notFound.value = c;
	return &notFound;
}

static int
hasAttr(widechar c, TranslationTableCharacterAttributes attributes,
		const TranslationTableHeader *table) {
	return (findChar(c, table)->attributes & attributes) != 0;
}

/* The lowercase counterpart of c according to the table (same as toLowercase in
 * the translation engines), or c itself if it has none. */
static widechar
lowercaseOf(widechar value, const TranslationTableHeader *table) {
	const TranslationTableCharacter *character = findChar(value, table);
	if (character->mode & CTC_UpperCase) {
		const TranslationTableCharacter *c = character;
		if (c->basechar) c = (TranslationTableCharacter *)&table->ruleArea[c->basechar];
		while (1) {
			if ((c->mode & (character->mode & ~CTC_UpperCase)) ==
					(character->mode & ~CTC_UpperCase))
				return c->value;
			if (!c->linked) break;
			c = (TranslationTableCharacter *)&table->ruleArea[c->linked];
		}
	}
	return value;
}

/* Is the character sequence starting at chars[pos] preceded by a valid start of a
 * standing-alone sequence: the start of the string, a space or a seqdelimiter
 * character, possibly followed by seqbeforechars characters? */
int EXPORT_CALL
_lou_inSequenceBefore(
		const TranslationTableHeader *table, const widechar *chars, int pos) {
	int i;
	for (i = pos - 1; i >= 0; i--) {
		if (hasAttr(chars[i], CTC_SeqBefore, table)) continue;
		return hasAttr(chars[i], CTC_Space | CTC_SeqDelimiter, table);
	}
	return 1;
}

/* Is the character sequence that ends just before chars[start] followed by a
 * valid end of a standing-alone sequence: seqafterchars characters and
 * seqafterpattern strings, then the end of the string, a space or a seqdelimiter
 * character? chars[start .. length - 1] is examined. */
int EXPORT_CALL
_lou_inSequenceAfter(const TranslationTableHeader *table, const widechar *chars,
		int start, int length) {
	int i;
	for (i = start; i < length; i++) {
		/* The patterns are stored one after the other, each terminated by a 0. The
		 * first one that matches here is skipped. Case is ignored, so that "'s"
		 * also matches the "'S" of a word in capitals. */
		int s = 0;
		while (s < table->seqPatternsCount) {
			int len = 0;
			int j;
			while (s + len < table->seqPatternsCount && table->seqPatterns[s + len])
				len++;
			for (j = 0; j < len && i + j < length; j++)
				if (lowercaseOf(chars[i + j], table) !=
						lowercaseOf(table->seqPatterns[s + j], table))
					break;
			if (len > 0 && j == len) {
				if (i + len >= length) return 1;
				i += len;
				break;
			}
			s += len + 1;
		}
		if (hasAttr(chars[i], CTC_SeqAfter, table)) continue;
		return hasAttr(chars[i], CTC_Space | CTC_SeqDelimiter, table);
	}
	return 1;
}
