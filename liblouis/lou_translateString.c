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

/**
 * @file
 * @brief Translate to braille
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"

/* additional bits in typebuf */
#define SYLLABLE_MARKER_1 0x2000
#define SYLLABLE_MARKER_2 0x4000
#define CAPSEMPH 0x8000

#define EMPHASIS 0x3fff  // all typeform bits that can be used

/* bits for wordBuffer */
#define WORD_CHAR 0x00000001
#define WORD_RESET 0x00000002
#define WORD_STOP 0x00000004
#define WORD_WHOLE 0x00000008
#define LAST_WORD_AFTER 0x01000000

/* bits for emphasisBuffer */
#define CAPS_BEGIN 0x00000010
#define CAPS_END 0x00000020
#define CAPS_WORD 0x00000040
#define CAPS_SYMBOL 0x00000080
#define CAPS_EMPHASIS 0x000000f0
#define EMPHASIS_BEGIN 0x00000100
#define EMPHASIS_END 0x00000200
#define EMPHASIS_WORD 0x00000400
#define EMPHASIS_SYMBOL 0x00000800
#define EMPHASIS_MASK 0x00000f00
#define COMPBRL_BEGIN 0x10000000
#define COMPBRL_END 0x20000000

/* bits for transNoteBuffer */
#define TRANSNOTE_BEGIN 0x00000001
#define TRANSNOTE_END 0x00000002
#define TRANSNOTE_WORD 0x00000004
#define TRANSNOTE_SYMBOL 0x00000008
#define TRANSNOTE_MASK 0x0000000f

typedef struct {
	const widechar *chars;
	int length;
} InString;

typedef struct {
	widechar *chars;
	int maxlength;
	int length;
} OutString;

static int
putCharacter(widechar c, const TranslationTableHeader *table, int *pos, int mode,
		InString input, OutString *output, int *posMapping, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, const TranslationTableRule **transRule,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd);
static int
passDoTest(const TranslationTableHeader *table, int pos, InString input, int transOpcode,
		const TranslationTableRule *transRule, int *passCharDots,
		const widechar **passInstructions, int *passIC, int *startMatch,
		int *startReplace, int *endReplace, int *endMatch,
		TranslationTableRule **groupingRule, widechar *groupingOp);
static int
passDoAction(const TranslationTableHeader *table, int *pos, int mode, InString *input,
		OutString *output, int *posMapping, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, int transOpcode,
		const TranslationTableRule **transRule, int passCharDots,
		const widechar *passInstructions, int *passIC, int startMatch, int startReplace,
		int *endReplace, int endMatch, int *cursorPosition, int *cursorStatus,
		int compbrlStart, int compbrlEnd, TranslationTableRule *groupingRule,
		widechar groupingOp);

static const TranslationTableRule **appliedRules;
static int maxAppliedRules;
static int appliedRulesCount;

static TranslationTableCharacter *
findCharOrDots(widechar c, int m, const TranslationTableHeader *table) {
	/* Look up character or dot pattern in the appropriate
	 * table. */
	static TranslationTableCharacter noChar = { 0, 0, 0, CTC_Space, 32, 32, 32 };
	static TranslationTableCharacter noDots = { 0, 0, 0, CTC_Space, B16, B16, B16 };
	TranslationTableCharacter *notFound;
	TranslationTableCharacter *character;
	TranslationTableOffset bucket;
	unsigned long int makeHash = (unsigned long int)c % HASHNUM;
	if (m == 0) {
		bucket = table->characters[makeHash];
		notFound = &noChar;
	} else {
		bucket = table->dots[makeHash];
		notFound = &noDots;
	}
	while (bucket) {
		character = (TranslationTableCharacter *)&table->ruleArea[bucket];
		if (character->realchar == c) return character;
		bucket = character->next;
	}
	notFound->realchar = notFound->uppercase = notFound->lowercase = c;
	return notFound;
}

static int
checkAttr(const widechar c, const TranslationTableCharacterAttributes a, int m,
		const TranslationTableHeader *table) {
	static widechar prevc = 0;
	static TranslationTableCharacterAttributes preva = 0;
	if (c != prevc) {
		preva = (findCharOrDots(c, m, table))->attributes;
		prevc = c;
	}
	return ((preva & a) ? 1 : 0);
}

static int
checkAttr_safe(InString input, int pos, const TranslationTableCharacterAttributes a,
		int m, const TranslationTableHeader *table) {
	return ((pos < input.length) ? checkAttr(input.chars[pos], a, m, table) : 0);
}

static int
findForPassRule(const TranslationTableHeader *table, int pos, int currentPass,
		InString input, int *transOpcode, const TranslationTableRule **transRule,
		int *transCharslen, int *passCharDots, widechar const **passInstructions,
		int *passIC, int *startMatch, int *startReplace, int *endReplace, int *endMatch,
		TranslationTableRule **groupingRule, widechar *groupingOp) {
	int save_transCharslen = *transCharslen;
	const TranslationTableRule *save_transRule = *transRule;
	TranslationTableOpcode save_transOpcode = *transOpcode;
	TranslationTableOffset ruleOffset;
	ruleOffset = table->forPassRules[currentPass];
	*transCharslen = 0;
	while (ruleOffset) {
		*transRule = (TranslationTableRule *)&table->ruleArea[ruleOffset];
		*transOpcode = (*transRule)->opcode;
		if (passDoTest(table, pos, input, *transOpcode, *transRule, passCharDots,
					passInstructions, passIC, startMatch, startReplace, endReplace,
					endMatch, groupingRule, groupingOp))
			return 1;
		ruleOffset = (*transRule)->charsnext;
	}
	*transCharslen = save_transCharslen;
	*transRule = save_transRule;
	*transOpcode = save_transOpcode;
	return 0;
}

static int
compareChars(const widechar *address1, const widechar *address2, int count, int m,
		const TranslationTableHeader *table) {
	int k;
	if (!count) return 0;
	for (k = 0; k < count; k++)
		if ((findCharOrDots(address1[k], m, table))->lowercase !=
				(findCharOrDots(address2[k], m, table))->lowercase)
			return 0;
	return 1;
}

static int
makeCorrections(const TranslationTableHeader *table, int mode, InString *input,
		OutString *output, int *posMapping, formtype *typebuf,
		unsigned int *emphasisBuffer, unsigned int *transNoteBuffer, int *realInlen,
		int *posIncremented, int *cursorPosition, int *cursorStatus, int compbrlStart,
		int compbrlEnd) {
	int pos;
	int transOpcode;
	const TranslationTableRule *transRule;
	int transCharslen;
	int passCharDots;
	const widechar *passInstructions;
	int passIC; /* Instruction counter */
	int startMatch;
	int startReplace;
	int endReplace;
	int endMatch;
	TranslationTableRule *groupingRule;
	widechar groupingOp;
	if (!table->corrections) return 1;
	pos = 0;
	output->length = 0;
	*posIncremented = 1;
	_lou_resetPassVariables();
	while (pos < input->length) {
		int length = input->length - pos;
		const TranslationTableCharacter *character =
				findCharOrDots(input->chars[pos], 0, table);
		const TranslationTableCharacter *character2;
		int tryThis = 0;
		if (!findForPassRule(table, pos, 0, *input, &transOpcode, &transRule,
					&transCharslen, &passCharDots, &passInstructions, &passIC,
					&startMatch, &startReplace, &endReplace, &endMatch, &groupingRule,
					&groupingOp))
			while (tryThis < 3) {
				TranslationTableOffset ruleOffset = 0;
				unsigned long int makeHash = 0;
				switch (tryThis) {
				case 0:
					if (!(length >= 2)) break;
					makeHash = (unsigned long int)character->lowercase << 8;
					character2 = findCharOrDots(input->chars[pos + 1], 0, table);
					makeHash += (unsigned long int)character2->lowercase;
					makeHash %= HASHNUM;
					ruleOffset = table->forRules[makeHash];
					break;
				case 1:
					if (!(length >= 1)) break;
					length = 1;
					ruleOffset = character->otherRules;
					break;
				case 2: /* No rule found */
					transOpcode = CTO_Always;
					ruleOffset = 0;
					break;
				}
				while (ruleOffset) {
					transRule = (TranslationTableRule *)&table->ruleArea[ruleOffset];
					transOpcode = transRule->opcode;
					transCharslen = transRule->charslen;
					if (tryThis == 1 || (transCharslen <= length &&
												compareChars(&transRule->charsdots[0],
														&input->chars[pos], transCharslen,
														0, table))) {
						if (*posIncremented && transOpcode == CTO_Correct &&
								passDoTest(table, pos, *input, transOpcode, transRule,
										&passCharDots, &passInstructions, &passIC,
										&startMatch, &startReplace, &endReplace,
										&endMatch, &groupingRule, &groupingOp)) {
							tryThis = 4;
							break;
						}
					}
					ruleOffset = transRule->charsnext;
				}
				tryThis++;
			}
		*posIncremented = 1;

		switch (transOpcode) {
		case CTO_Always:
			if (output->length >= output->maxlength) goto failure;
			posMapping[output->length] = pos;
			output->chars[output->length++] = input->chars[pos++];
			break;
		case CTO_Correct:
			if (appliedRules != NULL && appliedRulesCount < maxAppliedRules)
				appliedRules[appliedRulesCount++] = transRule;
			if (!passDoAction(table, &pos, mode, input, output, posMapping,
						emphasisBuffer, transNoteBuffer, transOpcode, &transRule,
						passCharDots, passInstructions, &passIC, startMatch, startReplace,
						&endReplace, endMatch, cursorPosition, cursorStatus, compbrlStart,
						compbrlEnd, groupingRule, groupingOp))
				goto failure;
			if (endReplace == pos) *posIncremented = 0;
			pos = endReplace;
			break;
		default:
			break;
		}
	}

	{  // We have to transform typebuf accordingly
		int k;
		formtype *typebuf_temp;
		if ((typebuf_temp = malloc(output->length * sizeof(formtype))) == NULL)
			_lou_outOfMemory();
		for (k = 0; k < output->length; k++)
			// posMapping will never be < 0 but in theory it could
			if (posMapping[k] < 0)
				typebuf_temp[k] = typebuf[0];  // prepend to next
			else if (posMapping[k] >= input->length)
				typebuf_temp[k] = typebuf[input->length - 1];  // append to previous
			else
				typebuf_temp[k] = typebuf[posMapping[k]];
		memcpy(typebuf, typebuf_temp, output->length * sizeof(formtype));
		free(typebuf_temp);
	}

failure:
	*realInlen = pos;
	return 1;
}

static int
matchCurrentInput(InString input, int pos, const widechar *passInstructions, int passIC) {
	int k;
	int kk = pos;
	for (k = passIC + 2; k < passIC + 2 + passInstructions[passIC + 1]; k++)
		if (input.chars[kk] == ENDSEGMENT || passInstructions[k] != input.chars[kk++])
			return 0;
	return 1;
}

static int
swapTest(int swapIC, int *pos, const TranslationTableHeader *table, InString input,
		const widechar *passInstructions) {
	int p = *pos;
	TranslationTableOffset swapRuleOffset;
	TranslationTableRule *swapRule;
	swapRuleOffset = (passInstructions[swapIC + 1] << 16) | passInstructions[swapIC + 2];
	swapRule = (TranslationTableRule *)&table->ruleArea[swapRuleOffset];
	while (p - *pos < passInstructions[swapIC + 3]) {
		int test;
		if (swapRule->opcode == CTO_SwapDd) {
			for (test = 1; test < swapRule->charslen; test += 2) {
				if (input.chars[p] == swapRule->charsdots[test]) break;
			}
		} else {
			for (test = 0; test < swapRule->charslen; test++) {
				if (input.chars[p] == swapRule->charsdots[test]) break;
			}
		}
		if (test >= swapRule->charslen) return 0;
		p++;
	}
	if (passInstructions[swapIC + 3] == passInstructions[swapIC + 4]) {
		*pos = p;
		return 1;
	}
	while (p - *pos < passInstructions[swapIC + 4]) {
		int test;
		if (swapRule->opcode == CTO_SwapDd) {
			for (test = 1; test < swapRule->charslen; test += 2) {
				if (input.chars[p] == swapRule->charsdots[test]) break;
			}
		} else {
			for (test = 0; test < swapRule->charslen; test++) {
				if (input.chars[p] == swapRule->charsdots[test]) break;
			}
		}
		if (test >= swapRule->charslen) {
			*pos = p;
			return 1;
		}
		p++;
	}
	*pos = p;
	return 1;
}

static int
swapReplace(int start, int end, const TranslationTableHeader *table, InString input,
		OutString *output, int *posMapping, const widechar *passInstructions,
		int passIC) {
	TranslationTableOffset swapRuleOffset;
	TranslationTableRule *swapRule;
	widechar *replacements;
	int p;
	swapRuleOffset = (passInstructions[passIC + 1] << 16) | passInstructions[passIC + 2];
	swapRule = (TranslationTableRule *)&table->ruleArea[swapRuleOffset];
	replacements = &swapRule->charsdots[swapRule->charslen];
	for (p = start; p < end; p++) {
		int rep;
		int test;
		int k;
		for (test = 0; test < swapRule->charslen; test++)
			if (input.chars[p] == swapRule->charsdots[test]) break;
		if (test == swapRule->charslen) continue;
		k = 0;
		for (rep = 0; rep < test; rep++)
			if (swapRule->opcode == CTO_SwapCc)
				k++;
			else
				k += replacements[k];
		if (swapRule->opcode == CTO_SwapCc) {
			if ((output->length + 1) > output->maxlength) return 0;
			posMapping[output->length] = p;
			output->chars[output->length++] = replacements[k];
		} else {
			int l = replacements[k] - 1;
			int d = output->length + l;
			if (d > output->maxlength) return 0;
			while (--d >= output->length) posMapping[d] = p;
			memcpy(&output->chars[output->length], &replacements[k + 1],
					l * sizeof(*output->chars));
			output->length += l;
		}
	}
	return 1;
}

static int
replaceGrouping(const TranslationTableHeader *table, InString input, OutString *output,
		int *posMapping, int transOpcode, int passCharDots,
		const widechar *passInstructions, int passIC, int startReplace,
		TranslationTableRule *groupingRule, widechar groupingOp) {
	widechar startCharDots = groupingRule->charsdots[2 * passCharDots];
	widechar endCharDots = groupingRule->charsdots[2 * passCharDots + 1];
	widechar *curin = (widechar *)input.chars;
	int p;
	int level = 0;
	TranslationTableOffset replaceOffset =
			passInstructions[passIC + 1] << 16 | (passInstructions[passIC + 2] & 0xff);
	TranslationTableRule *replaceRule =
			(TranslationTableRule *)&table->ruleArea[replaceOffset];
	widechar replaceStart = replaceRule->charsdots[2 * passCharDots];
	widechar replaceEnd = replaceRule->charsdots[2 * passCharDots + 1];
	if (groupingOp == pass_groupstart) {
		curin[startReplace] = replaceStart;
		for (p = startReplace + 1; p < input.length; p++) {
			if (input.chars[p] == startCharDots) level--;
			if (input.chars[p] == endCharDots) level++;
			if (level == 1) break;
		}
		if (p == input.length) return 0;
		curin[p] = replaceEnd;
	} else {
		if (transOpcode == CTO_Context) {
			startCharDots = groupingRule->charsdots[2];
			endCharDots = groupingRule->charsdots[3];
			replaceStart = replaceRule->charsdots[2];
			replaceEnd = replaceRule->charsdots[3];
		}
		output->chars[output->length] = replaceEnd;
		for (p = output->length - 1; p >= 0; p--) {
			if (output->chars[p] == endCharDots) level--;
			if (output->chars[p] == startCharDots) level++;
			if (level == 1) break;
		}
		if (p < 0) return 0;
		output->chars[p] = replaceStart;
		output->length++;
	}
	return 1;
}

static int
removeGrouping(InString *input, OutString *output, int *posMapping, int passCharDots,
		int startReplace, int endReplace, TranslationTableRule *groupingRule,
		widechar groupingOp) {
	widechar startCharDots = groupingRule->charsdots[2 * passCharDots];
	widechar endCharDots = groupingRule->charsdots[2 * passCharDots + 1];
	widechar *curin = (widechar *)input->chars;
	int p;
	int level = 0;
	if (groupingOp == pass_groupstart) {
		for (p = startReplace + 1; p < input->length; p++) {
			if (input->chars[p] == startCharDots) level--;
			if (input->chars[p] == endCharDots) level++;
			if (level == 1) break;
		}
		if (p == input->length) return 0;
		p++;
		for (; p < input->length; p++) curin[p - 1] = curin[p];
		input->length--;
	} else {
		for (p = output->length - 1; p >= 0; p--) {
			if (output->chars[p] == endCharDots) level--;
			if (output->chars[p] == startCharDots) level++;
			if (level == 1) break;
		}
		if (p < 0) return 0;
		p++;
		for (; p < output->length; p++) output->chars[p - 1] = output->chars[p];
		output->length--;
	}
	return 1;
}

static int
doPassSearch(const TranslationTableHeader *table, InString input,
		const TranslationTableRule *transRule, int passCharDots, int pos,
		const widechar *passInstructions, int passIC, int *searchIC, int *searchPos,
		TranslationTableRule *groupingRule, widechar groupingOp) {
	int level = 0;
	int k, kk;
	int not = 0;
	TranslationTableOffset ruleOffset;
	TranslationTableRule *rule;
	TranslationTableCharacterAttributes attributes;
	while (pos < input.length) {
		*searchIC = passIC + 1;
		*searchPos = pos;
		while (*searchIC < transRule->dotslen) {
			int itsTrue = 1;
			if (*searchPos > input.length) return 0;
			switch (passInstructions[*searchIC]) {
			case pass_lookback:
				*searchPos -= passInstructions[*searchIC + 1];
				if (*searchPos < 0) {
					*searchPos = 0;
					itsTrue = 0;
				}
				*searchIC += 2;
				break;
			case pass_not:
				not = !not;
				(*searchIC)++;
				continue;
			case pass_string:
			case pass_dots:
				kk = *searchPos;
				for (k = *searchIC + 2;
						k < *searchIC + 2 + passInstructions[*searchIC + 1]; k++)
					if (input.chars[kk] == ENDSEGMENT ||
							passInstructions[k] != input.chars[kk++]) {
						itsTrue = 0;
						break;
					}
				*searchPos += passInstructions[*searchIC + 1];
				*searchIC += passInstructions[*searchIC + 1] + 2;
				break;
			case pass_startReplace:
				(*searchIC)++;
				break;
			case pass_endReplace:
				(*searchIC)++;
				break;
			case pass_attributes:
				attributes = (passInstructions[*searchIC + 1] << 16) |
						passInstructions[*searchIC + 2];
				for (k = 0; k < passInstructions[*searchIC + 3]; k++) {
					if (input.chars[*searchPos] == ENDSEGMENT)
						itsTrue = 0;
					else
						itsTrue = ((findCharOrDots(input.chars[(*searchPos)++],
											passCharDots,
											table)->attributes &
										   attributes)
										? 1
										: 0);
					if (!itsTrue) break;
				}
				if (itsTrue) {
					for (k = passInstructions[*searchIC + 3];
							k < passInstructions[*searchIC + 4]; k++) {
						if (input.chars[*searchPos] == ENDSEGMENT) {
							itsTrue = 0;
							break;
						}
						if (!(findCharOrDots(input.chars[*searchPos], passCharDots,
									  table)->attributes &
									attributes))
							break;
						(*searchPos)++;
					}
				}
				*searchIC += 5;
				break;
			case pass_groupstart:
			case pass_groupend:
				ruleOffset = (passInstructions[*searchIC + 1] << 16) |
						passInstructions[*searchIC + 2];
				rule = (TranslationTableRule *)&table->ruleArea[ruleOffset];
				if (passInstructions[*searchIC] == pass_groupstart)
					itsTrue =
							(input.chars[*searchPos] == rule->charsdots[2 * passCharDots])
							? 1
							: 0;
				else
					itsTrue = (input.chars[*searchPos] ==
									  rule->charsdots[2 * passCharDots + 1])
							? 1
							: 0;
				if (groupingRule != NULL && groupingOp == pass_groupstart &&
						rule == groupingRule) {
					if (input.chars[*searchPos] == rule->charsdots[2 * passCharDots])
						level--;
					else if (input.chars[*searchPos] ==
							rule->charsdots[2 * passCharDots + 1])
						level++;
				}
				(*searchPos)++;
				*searchIC += 3;
				break;
			case pass_swap:
				itsTrue = swapTest(*searchIC, searchPos, table, input, passInstructions);
				*searchIC += 5;
				break;
			case pass_endTest:
				if (itsTrue) {
					if ((groupingRule && level == 1) || !groupingRule) return 1;
				}
				*searchIC = transRule->dotslen;
				break;
			default:
				if (_lou_handlePassVariableTest(passInstructions, searchIC, &itsTrue))
					break;
				break;
			}
			if ((!not&&!itsTrue) || (not&&itsTrue)) break;
			not = 0;
		}
		pos++;
	}
	return 0;
}

static int
passDoTest(const TranslationTableHeader *table, int pos, InString input, int transOpcode,
		const TranslationTableRule *transRule, int *passCharDots,
		widechar const **passInstructions, int *passIC, int *startMatch,
		int *startReplace, int *endReplace, int *endMatch,
		TranslationTableRule **groupingRule, widechar *groupingOp) {
	int searchIC, searchPos;
	int k;
	int not = 0;
	TranslationTableOffset ruleOffset = 0;
	TranslationTableRule *rule = NULL;
	TranslationTableCharacterAttributes attributes = 0;
	*groupingRule = NULL;
	*startMatch = *endMatch = pos;
	*passInstructions = &transRule->charsdots[transRule->charslen];
	*passIC = 0;
	*startReplace = *endReplace = -1;
	if (transOpcode == CTO_Context || transOpcode == CTO_Correct)
		*passCharDots = 0;
	else
		*passCharDots = 1;
	while (*passIC < transRule->dotslen) {
		int itsTrue = 1;
		if (pos > input.length) return 0;
		switch ((*passInstructions)[*passIC]) {
		case pass_first:
			if (pos != 0) itsTrue = 0;
			(*passIC)++;
			break;
		case pass_last:
			if (pos != input.length) itsTrue = 0;
			(*passIC)++;
			break;
		case pass_lookback:
			pos -= (*passInstructions)[*passIC + 1];
			if (pos < 0) {
				searchPos = 0;
				itsTrue = 0;
			}
			*passIC += 2;
			break;
		case pass_not:
			not = !not;
			(*passIC)++;
			continue;
		case pass_string:
		case pass_dots:
			itsTrue = matchCurrentInput(input, pos, *passInstructions, *passIC);
			pos += (*passInstructions)[*passIC + 1];
			*passIC += (*passInstructions)[*passIC + 1] + 2;
			break;
		case pass_startReplace:
			*startReplace = pos;
			(*passIC)++;
			break;
		case pass_endReplace:
			*endReplace = pos;
			(*passIC)++;
			break;
		case pass_attributes:
			attributes = ((*passInstructions)[*passIC + 1] << 16) |
					(*passInstructions)[*passIC + 2];
			for (k = 0; k < (*passInstructions)[*passIC + 3]; k++) {
				if (pos >= input.length) {
					itsTrue = 0;
					break;
				}
				if (input.chars[pos] == ENDSEGMENT) {
					itsTrue = 0;
					break;
				}
				if (!(findCharOrDots(input.chars[pos], *passCharDots, table)->attributes &
							attributes)) {
					itsTrue = 0;
					break;
				}
				pos += 1;
			}
			if (itsTrue) {
				for (k = (*passInstructions)[*passIC + 3];
						k < (*passInstructions)[*passIC + 4] && pos < input.length; k++) {
					if (input.chars[pos] == ENDSEGMENT) {
						itsTrue = 0;
						break;
					}
					if (!(findCharOrDots(input.chars[pos], *passCharDots,
								  table)->attributes &
								attributes)) {
						break;
					}
					pos += 1;
				}
			}
			*passIC += 5;
			break;
		case pass_groupstart:
		case pass_groupend:
			ruleOffset = ((*passInstructions)[*passIC + 1] << 16) |
					(*passInstructions)[*passIC + 2];
			rule = (TranslationTableRule *)&table->ruleArea[ruleOffset];
			if (*passIC == 0 ||
					(*passIC > 0 &&
							(*passInstructions)[*passIC - 1] == pass_startReplace)) {
				*groupingRule = rule;
				*groupingOp = (*passInstructions)[*passIC];
			}
			if ((*passInstructions)[*passIC] == pass_groupstart)
				itsTrue =
						(input.chars[pos] == rule->charsdots[2 * *passCharDots]) ? 1 : 0;
			else
				itsTrue = (input.chars[pos] == rule->charsdots[2 * *passCharDots + 1])
						? 1
						: 0;
			pos++;
			*passIC += 3;
			break;
		case pass_swap:
			itsTrue = swapTest(*passIC, &pos, table, input, *passInstructions);
			*passIC += 5;
			break;
		case pass_search:
			itsTrue = doPassSearch(table, input, transRule, *passCharDots, pos,
					*passInstructions, *passIC, &searchIC, &searchPos, *groupingRule,
					*groupingOp);
			if ((!not&&!itsTrue) || (not&&itsTrue)) return 0;
			*passIC = searchIC;
			pos = searchPos;
		case pass_endTest:
			(*passIC)++;
			*endMatch = pos;
			if (*startReplace == -1) {
				*startReplace = *startMatch;
				*endReplace = *endMatch;
			}
			return 1;
			break;
		default:
			if (_lou_handlePassVariableTest(*passInstructions, passIC, &itsTrue)) break;
			return 0;
		}
		if ((!not&&!itsTrue) || (not&&itsTrue)) return 0;
		not = 0;
	}
	return 0;
}

static int
copyCharacters(int from, int to, const TranslationTableHeader *table, int *pos, int mode,
		InString input, OutString *output, int *posMapping, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, int transOpcode,
		const TranslationTableRule **transRule, int *cursorPosition, int *cursorStatus,
		int compbrlStart, int compbrlEnd) {
	if (transOpcode == CTO_Context) {
		while (from < to)
			if (!putCharacter(input.chars[from++], table, pos, mode, input, output,
						posMapping, emphasisBuffer, transNoteBuffer, transRule,
						cursorPosition, cursorStatus, compbrlStart, compbrlEnd))
				return 0;
	} else {
		if (to > from) {
			if ((output->length + to - from) > output->maxlength) return 0;
			while (to > from) {
				posMapping[output->length] = from;
				output->chars[output->length] = input.chars[from];
				output->length++;
				from++;
			}
		}
	}

	return 1;
}

static int
passDoAction(const TranslationTableHeader *table, int *pos, int mode, InString *input,
		OutString *output, int *posMapping, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, int transOpcode,
		const TranslationTableRule **transRule, int passCharDots,
		const widechar *passInstructions, int *passIC, int startMatch, int startReplace,
		int *endReplace, int endMatch, int *cursorPosition, int *cursorStatus,
		int compbrlStart, int compbrlEnd, TranslationTableRule *groupingRule,
		widechar groupingOp) {
	int k;
	TranslationTableOffset ruleOffset = 0;
	TranslationTableRule *rule = NULL;
	int destStartMatch = output->length;
	int destStartReplace;
	int origEndReplace = *endReplace;

	if (!copyCharacters(startMatch, startReplace, table, pos, mode, *input, output,
				posMapping, emphasisBuffer, transNoteBuffer, transOpcode, transRule,
				cursorPosition, cursorStatus, compbrlStart, compbrlEnd))
		return 0;
	destStartReplace = output->length;

	while (*passIC < (*transRule)->dotslen) switch (passInstructions[*passIC]) {
		case pass_string:
		case pass_dots:
			if ((output->length + passInstructions[*passIC + 1]) > output->maxlength)
				return 0;
			for (k = 0; k < passInstructions[*passIC + 1]; ++k)
				posMapping[output->length + k] = startReplace;
			memcpy(&output->chars[output->length], &passInstructions[*passIC + 2],
					passInstructions[*passIC + 1] * CHARSIZE);
			output->length += passInstructions[*passIC + 1];
			*passIC += passInstructions[*passIC + 1] + 2;
			break;
		case pass_groupstart:
			ruleOffset =
					(passInstructions[*passIC + 1] << 16) | passInstructions[*passIC + 2];
			rule = (TranslationTableRule *)&table->ruleArea[ruleOffset];
			posMapping[output->length] = startMatch;
			output->chars[output->length++] = rule->charsdots[2 * passCharDots];
			*passIC += 3;
			break;
		case pass_groupend:
			ruleOffset =
					(passInstructions[*passIC + 1] << 16) | passInstructions[*passIC + 2];
			rule = (TranslationTableRule *)&table->ruleArea[ruleOffset];
			posMapping[output->length] = startMatch;
			output->chars[output->length++] = rule->charsdots[2 * passCharDots + 1];
			*passIC += 3;
			break;
		case pass_swap:
			if (!swapReplace(startReplace, *endReplace, table, *input, output, posMapping,
						passInstructions, *passIC))
				return 0;
			*passIC += 3;
			break;
		case pass_groupreplace:
			if (!groupingRule ||
					!replaceGrouping(table, *input, output, posMapping, transOpcode,
							passCharDots, passInstructions, *passIC, startReplace,
							groupingRule, groupingOp))
				return 0;
			*passIC += 3;
			break;
		case pass_omit:
			if (groupingRule)
				removeGrouping(input, output, posMapping, passCharDots, startReplace,
						*endReplace, groupingRule, groupingOp);
			(*passIC)++;
			break;
		case pass_copy: {
			int count = destStartReplace - destStartMatch;
			if (count > 0) {
				memmove(&output->chars[destStartMatch], &output->chars[destStartReplace],
						count * sizeof(*output->chars));
				output->length -= count;
				destStartReplace = destStartMatch;
			}
		}

			if (!copyCharacters(startReplace, origEndReplace, table, pos, mode, *input,
						output, posMapping, emphasisBuffer, transNoteBuffer, transOpcode,
						transRule, cursorPosition, cursorStatus, compbrlStart,
						compbrlEnd))
				return 0;
			*endReplace = endMatch;
			(*passIC)++;
			break;
		default:
			if (_lou_handlePassVariableAction(passInstructions, passIC)) break;
			return 0;
		}
	return 1;
}

static void
passSelectRule(const TranslationTableHeader *table, int pos, int currentPass,
		InString input, int *transOpcode, const TranslationTableRule **transRule,
		int *transCharslen, int *passCharDots, widechar const **passInstructions,
		int *passIC, int *startMatch, int *startReplace, int *endReplace, int *endMatch,
		TranslationTableRule **groupingRule, widechar *groupingOp) {
	if (!findForPassRule(table, pos, currentPass, input, transOpcode, transRule,
				transCharslen, passCharDots, passInstructions, passIC, startMatch,
				startReplace, endReplace, endMatch, groupingRule, groupingOp)) {
		*transOpcode = CTO_Always;
	}
}

static int
translatePass(const TranslationTableHeader *table, int mode, int currentPass,
		InString *input, OutString *output, int *posMapping, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, int *posIncremented, int *cursorPosition,
		int *cursorStatus, int compbrlStart, int compbrlEnd) {
	int pos;
	int transOpcode;
	int prevTransOpcode;
	const TranslationTableRule *transRule;
	int transCharslen;
	int passCharDots;
	const widechar *passInstructions;
	int passIC; /* Instruction counter */
	int startMatch;
	int startReplace;
	int endReplace;
	int endMatch;
	TranslationTableRule *groupingRule;
	widechar groupingOp;
	prevTransOpcode = CTO_None;
	pos = output->length = 0;
	*posIncremented = 1;
	_lou_resetPassVariables();
	while (pos < input->length) { /* the main multipass translation loop */
		passSelectRule(table, pos, currentPass, *input, &transOpcode, &transRule,
				&transCharslen, &passCharDots, &passInstructions, &passIC, &startMatch,
				&startReplace, &endReplace, &endMatch, &groupingRule, &groupingOp);
		*posIncremented = 1;
		switch (transOpcode) {
		case CTO_Context:
		case CTO_Pass2:
		case CTO_Pass3:
		case CTO_Pass4:
			if (appliedRules != NULL && appliedRulesCount < maxAppliedRules)
				appliedRules[appliedRulesCount++] = transRule;
			if (!passDoAction(table, &pos, mode, input, output, posMapping,
						emphasisBuffer, transNoteBuffer, transOpcode, &transRule,
						passCharDots, passInstructions, &passIC, startMatch, startReplace,
						&endReplace, endMatch, cursorPosition, cursorStatus, compbrlStart,
						compbrlEnd, groupingRule, groupingOp))
				goto failure;
			if (endReplace == pos) *posIncremented = 0;
			pos = endReplace;
			break;
		case CTO_Always:
			if ((output->length + 1) > output->maxlength) goto failure;
			posMapping[output->length] = pos;
			output->chars[output->length++] = input->chars[pos++];
			break;
		default:
			goto failure;
		}
	}
	posMapping[output->length] = pos;
failure:
	if (pos < input->length) {
		while (checkAttr(input->chars[pos], CTC_Space, 1, table))
			if (++pos == input->length) break;
	}
	return 1;
}

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

static int
translateString(const TranslationTableHeader *table, int mode, int currentPass,
		InString *input, OutString *output, int *posMapping, formtype *typebuf,
		unsigned char *srcSpacing, unsigned char *destSpacing, unsigned int *wordBuffer,
		unsigned int *emphasisBuffer, unsigned int *transNoteBuffer, int haveEmphasis,
		int *realInlen, int *posIncremented, int *cursorPosition, int *cursorStatus,
		int compbrlStart, int compbrlEnd);

int EXPORT_CALL
lou_translateString(const char *tableList, const widechar *inbufx, int *inlen,
		widechar *outbuf, int *outlen, formtype *typeform, char *spacing, int mode) {
	return lou_translate(tableList, inbufx, inlen, outbuf, outlen, typeform, spacing,
			NULL, NULL, NULL, mode);
}

int EXPORT_CALL
lou_translate(const char *tableList, const widechar *inbufx, int *inlen, widechar *outbuf,
		int *outlen, formtype *typeform, char *spacing, int *outputPos, int *inputPos,
		int *cursorPos, int mode) {
	return _lou_translateWithTracing(tableList, inbufx, inlen, outbuf, outlen, typeform,
			spacing, outputPos, inputPos, cursorPos, mode, NULL, NULL);
}

int EXPORT_CALL
_lou_translateWithTracing(const char *tableList, const widechar *inbufx, int *inlen,
		widechar *outbuf, int *outlen, formtype *typeform, char *spacing, int *outputPos,
		int *inputPos, int *cursorPos, int mode, const TranslationTableRule **rules,
		int *rulesLen) {
	// int i;
	// for(i = 0; i < *inlen; i++)
	// {
	// 	outbuf[i] = inbufx[i];
	// 	if(inputPos)
	// 		inputPos[i] = i;
	// 	if(outputPos)
	// 		outputPos[i] = i;
	// }
	// *inlen = i;
	// *outlen = i;
	// return 1;
	const TranslationTableHeader *table;
	InString input;
	OutString output;
	widechar *passbuf1;
	widechar *passbuf2;
	// posMapping contains position mapping info between the initial input and the output
	// of the current pass. It is 1 longer than the output. The values are monotonically
	// increasing and can range between -1 and the output length. At the end the position
	// info is passed to the user as an inputPos and outputPos array. inputPos has the
	// length of the final output and has values ranging from 0 to outlen-1. outputPos has
	// the length of the initial input and has values ranging from 0 to inlen-1.
	int *posMapping;
	int *posMapping1;
	int *posMapping2;
	int *posMapping3;
	formtype *typebuf;
	unsigned char *srcSpacing;
	unsigned char *destSpacing;
	unsigned int *wordBuffer;
	unsigned int *emphasisBuffer;
	unsigned int *transNoteBuffer;
	int cursorPosition;
	int cursorStatus;
	int haveEmphasis;
	int compbrlStart;
	int compbrlEnd;
	int k;
	int goodTrans = 1;
	int realInlen;
	int posIncremented;
	if (tableList == NULL || inbufx == NULL || inlen == NULL || outbuf == NULL ||
			outlen == NULL)
		return 0;
	_lou_logMessage(
			LOG_ALL, "Performing translation: tableList=%s, inlen=%d", tableList, *inlen);
	_lou_logWidecharBuf(LOG_ALL, "Inbuf=", inbufx, *inlen);

	table = lou_getTable(tableList);
	if (table == NULL || *inlen < 0 || *outlen < 0) return 0;
	k = 0;
	while (k < *inlen && inbufx[k]) k++;
	input = (InString){ inbufx, k };
	haveEmphasis = 0;
	if (!(typebuf = _lou_allocMem(alloc_typebuf, input.length, *outlen))) return 0;
	if (typeform != NULL) {
		for (k = 0; k < input.length; k++) {
			typebuf[k] = typeform[k];
			if (typebuf[k] & EMPHASIS) haveEmphasis = 1;
		}
	} else
		memset(typebuf, 0, input.length * sizeof(formtype));

	if ((wordBuffer = _lou_allocMem(alloc_wordBuffer, input.length, *outlen)))
		memset(wordBuffer, 0, (input.length + 4) * sizeof(unsigned int));
	else
		return 0;
	if ((emphasisBuffer = _lou_allocMem(alloc_emphasisBuffer, input.length, *outlen)))
		memset(emphasisBuffer, 0, (input.length + 4) * sizeof(unsigned int));
	else
		return 0;
	if ((transNoteBuffer = _lou_allocMem(alloc_transNoteBuffer, input.length, *outlen)))
		memset(transNoteBuffer, 0, (input.length + 4) * sizeof(unsigned int));
	else
		return 0;

	if (!(spacing == NULL || *spacing == 'X'))
		srcSpacing = (unsigned char *)spacing;
	else
		srcSpacing = NULL;
	if (outputPos != NULL)
		for (k = 0; k < input.length; k++) outputPos[k] = -1;
	if (cursorPos != NULL && *cursorPos >= 0) {
		cursorStatus = 0;
		cursorPosition = *cursorPos;
		if ((mode & (compbrlAtCursor | compbrlLeftCursor))) {
			compbrlStart = cursorPosition;
			if (checkAttr(input.chars[compbrlStart], CTC_Space, 0, table))
				compbrlEnd = compbrlStart + 1;
			else {
				while (compbrlStart >= 0 &&
						!checkAttr(input.chars[compbrlStart], CTC_Space, 0, table))
					compbrlStart--;
				compbrlStart++;
				compbrlEnd = cursorPosition;
				if (!(mode & compbrlLeftCursor))
					while (compbrlEnd < input.length &&
							!checkAttr(input.chars[compbrlEnd], CTC_Space, 0, table))
						compbrlEnd++;
			}
		}
	} else {
		cursorPosition = -1;
		cursorStatus = 1; /* so it won't check cursor position */
	}
	// FIXME: reallocate passbuf and posMapping buffers in between passes because the
	// intermediary strings may be longer than the in- and outputs.
	if (!(passbuf1 = _lou_allocMem(alloc_passbuf1, input.length, *outlen))) return 0;
	if (!(posMapping1 = _lou_allocMem(alloc_posMapping1, input.length, *outlen)))
		return 0;
	if ((!(mode & pass1Only)) && (table->numPasses > 1 || table->corrections)) {
		if (!(passbuf2 = _lou_allocMem(alloc_passbuf2, input.length, *outlen))) return 0;
		if (!(posMapping2 = _lou_allocMem(alloc_posMapping2, input.length, *outlen)))
			return 0;
		if (!(posMapping3 = _lou_allocMem(alloc_posMapping3, input.length, *outlen)))
			return 0;
	}
	if (srcSpacing != NULL) {
		if (!(destSpacing = _lou_allocMem(alloc_destSpacing, input.length, *outlen)))
			goodTrans = 0;
		else
			memset(destSpacing, '*', *outlen);
	} else
		destSpacing = NULL;
	appliedRulesCount = 0;
	if (rules != NULL && rulesLen != NULL) {
		appliedRules = rules;
		maxAppliedRules = *rulesLen;
	} else {
		appliedRules = NULL;
		maxAppliedRules = 0;
	}
	output = (OutString){ passbuf1, *outlen };
	posMapping = posMapping1;
	if ((mode & pass1Only)) {
		goodTrans = translateString(table, mode, 1, &input, &output, posMapping, typebuf,
				srcSpacing, destSpacing, wordBuffer, emphasisBuffer, transNoteBuffer,
				haveEmphasis, &realInlen, &posIncremented, &cursorPosition, &cursorStatus,
				compbrlStart, compbrlEnd);
		posMapping[output.length] = input.length;
	} else {
		int currentPass = table->corrections ? 0 : 1;
		int *passPosMapping = posMapping;
		while (1) {
			switch (currentPass) {
			case 0:
				goodTrans = makeCorrections(table, mode, &input, &output, passPosMapping,
						typebuf, emphasisBuffer, transNoteBuffer, &realInlen,
						&posIncremented, &cursorPosition, &cursorStatus, compbrlStart,
						compbrlEnd);
				break;
			case 1: {
				// if table->corrections, realInlen is set by makeCorrections
				int *pRealInlen;
				pRealInlen = table->corrections ? NULL : &realInlen;
				goodTrans = translateString(table, mode, currentPass, &input, &output,
						passPosMapping, typebuf, srcSpacing, destSpacing, wordBuffer,
						emphasisBuffer, transNoteBuffer, haveEmphasis, pRealInlen,
						&posIncremented, &cursorPosition, &cursorStatus, compbrlStart,
						compbrlEnd);
				break;
			}
			default:
				goodTrans = translatePass(table, mode, currentPass, &input, &output,
						passPosMapping, emphasisBuffer, transNoteBuffer, &posIncremented,
						&cursorPosition, &cursorStatus, compbrlStart, compbrlEnd);
				break;
			}
			passPosMapping[output.length] = input.length;
			if (passPosMapping == posMapping) {
				passPosMapping = posMapping2;
			} else {
				int *prevPosMapping = posMapping3;
				memcpy((int *)prevPosMapping, posMapping,
						(output.length + 1) * sizeof(int));
				for (k = 0; k <= output.length; k++)
					if (passPosMapping[k] < 0)
						posMapping[k] = prevPosMapping[0];
					else
						posMapping[k] = prevPosMapping[passPosMapping[k]];
			}
			currentPass++;
			if (currentPass <= table->numPasses && goodTrans) {
				widechar *tmp = passbuf1;
				passbuf1 = passbuf2;
				passbuf2 = tmp;
				input = (InString){ output.chars, output.length };
				output = (OutString){ passbuf1, *outlen };
				continue;
			}
			break;
		}
	}
	if (goodTrans) {
		for (k = 0; k < output.length; k++) {
			if (typeform != NULL) {
				if ((output.chars[k] & (B7 | B8)))
					typeform[k] = '8';
				else
					typeform[k] = '0';
			}
			if ((mode & dotsIO)) {
				if ((mode & ucBrl))
					outbuf[k] = ((output.chars[k] & 0xff) | 0x2800);
				else
					outbuf[k] = output.chars[k];
			} else
				outbuf[k] = _lou_getCharFromDots(output.chars[k]);
		}
		*inlen = realInlen;
		*outlen = output.length;
		// Compute inputPos and outputPos from posMapping. The value at the last index of
		// posMapping is currectly not used.
		if (inputPos != NULL) {
			for (k = 0; k < *outlen; k++)
				if (posMapping[k] < 0)
					inputPos[k] = 0;
				else if (posMapping[k] > *inlen - 1)
					inputPos[k] = *inlen - 1;
				else
					inputPos[k] = posMapping[k];
		}
		if (outputPos != NULL) {
			int inpos = -1;
			int outpos = -1;
			for (k = 0; k < *outlen; k++)
				if (posMapping[k] > inpos) {
					while (inpos < posMapping[k]) {
						if (inpos >= 0 && inpos < *inlen)
							outputPos[inpos] = outpos < 0 ? 0 : outpos;
						inpos++;
					}
					outpos = k;
				}
			while (inpos < *inlen) outputPos[inpos++] = outpos;
		}
	}
	if (destSpacing != NULL) {
		memcpy(srcSpacing, destSpacing, input.length);
		srcSpacing[input.length] = 0;
	}
	if (cursorPos != NULL && *cursorPos != -1) {
		if (outputPos != NULL)
			*cursorPos = outputPos[*cursorPos];
		else
			*cursorPos = cursorPosition;
	}
	if (rulesLen != NULL) *rulesLen = appliedRulesCount;
	_lou_logMessage(LOG_ALL, "Translation complete: outlen=%d", *outlen);
	_lou_logWidecharBuf(LOG_ALL, "Outbuf=", (const widechar *)outbuf, *outlen);

	return goodTrans;
}

int EXPORT_CALL
lou_translatePrehyphenated(const char *tableList, const widechar *inbufx, int *inlen,
		widechar *outbuf, int *outlen, formtype *typeform, char *spacing, int *outputPos,
		int *inputPos, int *cursorPos, char *inputHyphens, char *outputHyphens,
		int mode) {
	int rv = 1;
	int *alloc_inputPos = NULL;
	if (inputHyphens != NULL) {
		if (outputHyphens == NULL) return 0;
		if (inputPos == NULL) {
			if ((alloc_inputPos = malloc(*outlen * sizeof(int))) == NULL)
				_lou_outOfMemory();
			inputPos = alloc_inputPos;
		}
	}
	if (lou_translate(tableList, inbufx, inlen, outbuf, outlen, typeform, spacing,
				outputPos, inputPos, cursorPos, mode)) {
		if (inputHyphens != NULL) {
			int inpos = 0;
			int outpos;
			for (outpos = 0; outpos < *outlen; outpos++) {
				int new_inpos = inputPos[outpos];
				if (new_inpos < inpos) {
					rv = 0;
					break;
				}
				if (new_inpos > inpos)
					outputHyphens[outpos] = inputHyphens[new_inpos];
				else
					outputHyphens[outpos] = '0';
				inpos = new_inpos;
			}
		}
	}
	if (alloc_inputPos != NULL) free(alloc_inputPos);
	return rv;
}

static int
hyphenate(const widechar *word, int wordSize, char *hyphens,
		const TranslationTableHeader *table) {
	widechar *prepWord;
	int i, k, limit;
	int stateNum;
	widechar ch;
	HyphenationState *statesArray =
			(HyphenationState *)&table->ruleArea[table->hyphenStatesArray];
	HyphenationState *currentState;
	HyphenationTrans *transitionsArray;
	char *hyphenPattern;
	int patternOffset;
	if (!table->hyphenStatesArray || (wordSize + 3) > MAXSTRING) return 0;
	prepWord = (widechar *)calloc(wordSize + 3, sizeof(widechar));
	/* prepWord is of the format ".hello."
	 * hyphens is the length of the word "hello" "00000" */
	prepWord[0] = '.';
	for (i = 0; i < wordSize; i++) {
		prepWord[i + 1] = (findCharOrDots(word[i], 0, table))->lowercase;
		hyphens[i] = '0';
	}
	prepWord[wordSize + 1] = '.';

	/* now, run the finite state machine */
	stateNum = 0;

	// we need to walk all of ".hello."
	for (i = 0; i < wordSize + 2; i++) {
		ch = prepWord[i];
		while (1) {
			if (stateNum == 0xffff) {
				stateNum = 0;
				goto nextLetter;
			}
			currentState = &statesArray[stateNum];
			if (currentState->trans.offset) {
				transitionsArray =
						(HyphenationTrans *)&table->ruleArea[currentState->trans.offset];
				for (k = 0; k < currentState->numTrans; k++) {
					if (transitionsArray[k].ch == ch) {
						stateNum = transitionsArray[k].newState;
						goto stateFound;
					}
				}
			}
			stateNum = currentState->fallbackState;
		}
	stateFound:
		currentState = &statesArray[stateNum];
		if (currentState->hyphenPattern) {
			hyphenPattern = (char *)&table->ruleArea[currentState->hyphenPattern];
			patternOffset = i + 1 - (int)strlen(hyphenPattern);

			/* Need to ensure that we don't overrun hyphens,
			 * in some cases hyphenPattern is longer than the remaining letters,
			 * and if we write out all of it we would have overshot our buffer. */
			limit = MIN((int)strlen(hyphenPattern), wordSize - patternOffset);
			for (k = 0; k < limit; k++) {
				if (hyphens[patternOffset + k] < hyphenPattern[k])
					hyphens[patternOffset + k] = hyphenPattern[k];
			}
		}
	nextLetter:;
	}
	hyphens[wordSize] = 0;
	free(prepWord);
	return 1;
}

static int
doCompTrans(int start, int end, const TranslationTableHeader *table, int *pos, int mode,
		InString input, OutString *output, int *posMapping, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, const TranslationTableRule **transRule,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd);

static int
for_updatePositions(const widechar *outChars, int inLength, int outLength, int shift,
		const TranslationTableHeader *table, int *pos, int mode, InString input,
		OutString *output, int *posMapping, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, const TranslationTableRule **transRule,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd) {
	int k;
	if ((output->length + outLength) > output->maxlength ||
			(*pos + inLength) > input.length)
		return 0;
	memcpy(&output->chars[output->length], outChars, outLength * CHARSIZE);
	if (!*cursorStatus) {
		if ((mode & (compbrlAtCursor | compbrlLeftCursor))) {
			if (*pos >= compbrlStart) {
				*cursorStatus = 2;
				return doCompTrans(compbrlStart, compbrlEnd, table, pos, mode, input,
						output, posMapping, emphasisBuffer, transNoteBuffer, transRule,
						cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
			}
		} else if (*cursorPosition >= *pos && *cursorPosition < (*pos + inLength)) {
			*cursorPosition = output->length;
			*cursorStatus = 1;
		} else if (input.chars[*cursorPosition] == 0 &&
				*cursorPosition == (*pos + inLength)) {
			*cursorPosition = output->length + outLength / 2 + 1;
			*cursorStatus = 1;
		}
	} else if (*cursorStatus == 2 && *cursorPosition == *pos)
		*cursorPosition = output->length;
	if (outLength <= inLength) {
		for (k = 0; k < outLength; k++) posMapping[output->length + k] = *pos + shift;
	} else {
		for (k = 0; k < inLength; k++) posMapping[output->length + k] = *pos + shift;
		for (k = inLength; k < outLength; k++)
			posMapping[output->length + k] = *pos + shift;
	}
	output->length += outLength;
	return 1;
}

static int
syllableBreak(
		const TranslationTableHeader *table, int pos, InString input, int transCharslen) {
	int wordStart = 0;
	int wordEnd = 0;
	int wordSize = 0;
	int k = 0;
	char *hyphens = NULL;
	for (wordStart = pos; wordStart >= 0; wordStart--)
		if (!((findCharOrDots(input.chars[wordStart], 0, table))->attributes &
					CTC_Letter)) {
			wordStart++;
			break;
		}
	if (wordStart < 0) wordStart = 0;
	for (wordEnd = pos; wordEnd < input.length; wordEnd++)
		if (!((findCharOrDots(input.chars[wordEnd], 0, table))->attributes &
					CTC_Letter)) {
			wordEnd--;
			break;
		}
	if (wordEnd == input.length) wordEnd--;
	/* At this stage wordStart is the 0 based index of the first letter in the word,
	 * wordEnd is the 0 based index of the last letter in the word.
	 * example: "hello" wordstart=0, wordEnd=4. */
	wordSize = wordEnd - wordStart + 1;
	hyphens = (char *)calloc(wordSize + 1, sizeof(char));
	if (!hyphenate(&input.chars[wordStart], wordSize, hyphens, table)) {
		free(hyphens);
		return 0;
	}
	for (k = pos - wordStart + 1; k < (pos - wordStart + transCharslen); k++)
		if (hyphens[k] & 1) {
			free(hyphens);
			return 1;
		}
	free(hyphens);
	return 0;
}

static void
setBefore(const TranslationTableHeader *table, int pos, InString input,
		TranslationTableCharacterAttributes *beforeAttributes) {
	widechar before;
	if (pos >= 2 && input.chars[pos - 1] == ENDSEGMENT)
		before = input.chars[pos - 2];
	else
		before = (pos == 0) ? ' ' : input.chars[pos - 1];
	*beforeAttributes = (findCharOrDots(before, 0, table))->attributes;
}

static void
setAfter(int length, const TranslationTableHeader *table, int pos, InString input,
		TranslationTableCharacterAttributes *afterAttributes) {
	widechar after;
	if ((pos + length + 2) < input.length && input.chars[pos + 1] == ENDSEGMENT)
		after = input.chars[pos + 2];
	else
		after = (pos + length < input.length) ? input.chars[pos + length] : ' ';
	*afterAttributes = (findCharOrDots(after, 0, table))->attributes;
}

static int
brailleIndicatorDefined(TranslationTableOffset offset,
		const TranslationTableHeader *table, const TranslationTableRule **indicRule) {
	if (!offset) return 0;
	*indicRule = (TranslationTableRule *)&table->ruleArea[offset];
	return 1;
}

static int
validMatch(const TranslationTableHeader *table, int pos, InString input,
		formtype *typebuf, const TranslationTableRule *transRule, int transCharslen) {
	/* Analyze the typeform parameter and also check for capitalization */
	TranslationTableCharacter *inputChar;
	TranslationTableCharacter *ruleChar;
	TranslationTableCharacterAttributes prevAttr = 0;
	int k;
	int kk = 0;
	if (!transCharslen) return 0;
	for (k = pos; k < pos + transCharslen; k++) {
		if (input.chars[k] == ENDSEGMENT) {
			if (k == pos && transCharslen == 1)
				return 1;
			else
				return 0;
		}
		inputChar = findCharOrDots(input.chars[k], 0, table);
		if (k == pos) prevAttr = inputChar->attributes;
		ruleChar = findCharOrDots(transRule->charsdots[kk++], 0, table);
		if ((inputChar->lowercase != ruleChar->lowercase)) return 0;
		if (typebuf != NULL && (typebuf[pos] & CAPSEMPH) == 0 &&
				(typebuf[k] | typebuf[pos]) != typebuf[pos])
			return 0;
		if (inputChar->attributes != CTC_Letter) {
			if (k != (pos + 1) && (prevAttr & CTC_Letter) &&
					(inputChar->attributes & CTC_Letter) &&
					((inputChar->attributes &
							 (CTC_LowerCase | CTC_UpperCase | CTC_Letter)) !=
							(prevAttr & (CTC_LowerCase | CTC_UpperCase | CTC_Letter))))
				return 0;
		}
		prevAttr = inputChar->attributes;
	}
	return 1;
}

static int
doCompEmph(const TranslationTableHeader *table, int *pos, int mode, InString input,
		OutString *output, int *posMapping, formtype *typebuf,
		unsigned int *emphasisBuffer, unsigned int *transNoteBuffer,
		const TranslationTableRule **transRule, int *cursorPosition, int *cursorStatus,
		int compbrlStart, int compbrlEnd) {
	int endEmph;
	for (endEmph = *pos; (typebuf[endEmph] & computer_braille) && endEmph <= input.length;
			endEmph++)
		;
	return doCompTrans(*pos, endEmph, table, pos, mode, input, output, posMapping,
			emphasisBuffer, transNoteBuffer, transRule, cursorPosition, cursorStatus,
			compbrlStart, compbrlEnd);
}

static int
insertBrailleIndicators(int finish, const TranslationTableHeader *table, int *pos,
		int mode, InString input, OutString *output, int *posMapping, formtype *typebuf,
		int haveEmphasis, unsigned int *emphasisBuffer, unsigned int *transNoteBuffer,
		int transOpcode, int prevTransOpcode, const TranslationTableRule **transRule,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd,
		TranslationTableCharacterAttributes beforeAttributes, int *prevType, int *curType,
		int *prevTypeform, int prevPos) {
	/* Insert braille indicators such as letter, number, etc. */
	typedef enum {
		checkNothing,
		checkBeginTypeform,
		checkEndTypeform,
		checkNumber,
		checkLetter
	} checkThis;
	checkThis checkWhat = checkNothing;
	int ok = 0;
	int k;
	{
		if (*pos == prevPos && !finish) return 1;
		if (*pos != prevPos) {
			if (haveEmphasis && (typebuf[*pos] & EMPHASIS) != *prevTypeform) {
				*prevType = *prevTypeform & EMPHASIS;
				*curType = typebuf[*pos] & EMPHASIS;
				checkWhat = checkEndTypeform;
			} else if (!finish)
				checkWhat = checkNothing;
			else
				checkWhat = checkNumber;
		}
		if (finish == 1) checkWhat = checkNumber;
	}
	do {
		const TranslationTableRule *indicRule;
		ok = 0;
		switch (checkWhat) {
		case checkNothing:
			ok = 0;
			break;
		case checkBeginTypeform:
			if (haveEmphasis) {
				ok = 0;
				*curType = 0;
			}
			if (*curType == plain_text) {
				if (!finish)
					checkWhat = checkNothing;
				else
					checkWhat = checkNumber;
			}
			break;
		case checkEndTypeform:
			if (haveEmphasis) {
				ok = 0;
				*prevType = plain_text;
			}
			if (*prevType == plain_text) {
				checkWhat = checkBeginTypeform;
				*prevTypeform = typebuf[*pos] & EMPHASIS;
			}
			break;
		case checkNumber:
			if (brailleIndicatorDefined(table->numberSign, table, &indicRule) &&
					checkAttr_safe(input, *pos, CTC_Digit, 0, table) &&
					(prevTransOpcode == CTO_ExactDots ||
							!(beforeAttributes & CTC_Digit)) &&
					prevTransOpcode != CTO_MidNum) {
				ok = !table->usesNumericMode;
				checkWhat = checkNothing;
			} else
				checkWhat = checkLetter;
			break;
		case checkLetter:
			if (!brailleIndicatorDefined(table->letterSign, table, &indicRule)) {
				ok = 0;
				checkWhat = checkNothing;
				break;
			}
			if (transOpcode == CTO_Contraction) {
				ok = 1;
				checkWhat = checkNothing;
				break;
			}
			if ((checkAttr_safe(input, *pos, CTC_Letter, 0, table) &&
						!(beforeAttributes & CTC_Letter)) &&
					(!checkAttr_safe(input, *pos + 1, CTC_Letter, 0, table) ||
							(beforeAttributes & CTC_Digit))) {
				ok = 1;
				if (*pos > 0)
					for (k = 0; k < table->noLetsignBeforeCount; k++)
						if (input.chars[*pos - 1] == table->noLetsignBefore[k]) {
							ok = 0;
							break;
						}
				for (k = 0; k < table->noLetsignCount; k++)
					if (input.chars[*pos] == table->noLetsign[k]) {
						ok = 0;
						break;
					}
				if ((*pos + 1) < input.length)
					for (k = 0; k < table->noLetsignAfterCount; k++)
						if (input.chars[*pos + 1] == table->noLetsignAfter[k]) {
							ok = 0;
							break;
						}
			}
			checkWhat = checkNothing;
			break;

		default:
			ok = 0;
			checkWhat = checkNothing;
			break;
		}
		if (ok && indicRule != NULL) {
			if (!for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, 0,
						table, pos, mode, input, output, posMapping, emphasisBuffer,
						transNoteBuffer, transRule, cursorPosition, cursorStatus,
						compbrlStart, compbrlEnd))
				return 0;
			if (*cursorStatus == 2) checkWhat = checkNothing;
		}
	} while (checkWhat != checkNothing);
	return 1;
}

static int
onlyLettersBehind(const TranslationTableHeader *table, int pos, InString input,
		TranslationTableCharacterAttributes beforeAttributes) {
	/* Actually, spaces, then letters */
	int k;
	if (!(beforeAttributes & CTC_Space)) return 0;
	for (k = pos - 2; k >= 0; k--) {
		TranslationTableCharacterAttributes attr =
				(findCharOrDots(input.chars[k], 0, table))->attributes;
		if ((attr & CTC_Space)) continue;
		if ((attr & CTC_Letter))
			return 1;
		else
			return 0;
	}
	return 1;
}

static int
onlyLettersAhead(const TranslationTableHeader *table, int pos, InString input,
		int transCharslen, TranslationTableCharacterAttributes afterAttributes) {
	/* Actullly, spaces, then letters */
	int k;
	if (!(afterAttributes & CTC_Space)) return 0;
	for (k = pos + transCharslen + 1; k < input.length; k++) {
		TranslationTableCharacterAttributes attr =
				(findCharOrDots(input.chars[k], 0, table))->attributes;
		if ((attr & CTC_Space)) continue;
		if ((attr & (CTC_Letter | CTC_LitDigit)))
			return 1;
		else
			return 0;
	}
	return 0;
}

static int
noCompbrlAhead(const TranslationTableHeader *table, int pos, int mode, InString input,
		int transOpcode, int transCharslen, int cursorPosition) {
	int start = pos + transCharslen;
	int end;
	int p;
	if (start >= input.length) return 1;
	while (start < input.length && checkAttr(input.chars[start], CTC_Space, 0, table))
		start++;
	if (start == input.length ||
			(transOpcode == CTO_JoinableWord &&
					(!checkAttr(input.chars[start], CTC_Letter | CTC_Digit, 0, table) ||
							!checkAttr(input.chars[start - 1], CTC_Space, 0, table))))
		return 1;
	end = start;
	while (end < input.length && !checkAttr(input.chars[end], CTC_Space, 0, table)) end++;
	if ((mode & (compbrlAtCursor | compbrlLeftCursor)) && cursorPosition >= start &&
			cursorPosition < end)
		return 0;
	/* Look ahead for rules with CTO_CompBrl */
	for (p = start; p < end; p++) {
		int length = input.length - p;
		int tryThis;
		const TranslationTableCharacter *character1;
		const TranslationTableCharacter *character2;
		int k;
		character1 = findCharOrDots(input.chars[p], 0, table);
		for (tryThis = 0; tryThis < 2; tryThis++) {
			TranslationTableOffset ruleOffset = 0;
			TranslationTableRule *testRule;
			unsigned long int makeHash = 0;
			switch (tryThis) {
			case 0:
				if (!(length >= 2)) break;
				/* Hash function optimized for forward translation */
				makeHash = (unsigned long int)character1->lowercase << 8;
				character2 = findCharOrDots(input.chars[p + 1], 0, table);
				makeHash += (unsigned long int)character2->lowercase;
				makeHash %= HASHNUM;
				ruleOffset = table->forRules[makeHash];
				break;
			case 1:
				if (!(length >= 1)) break;
				length = 1;
				ruleOffset = character1->otherRules;
				break;
			}
			while (ruleOffset) {
				testRule = (TranslationTableRule *)&table->ruleArea[ruleOffset];
				for (k = 0; k < testRule->charslen; k++) {
					character1 = findCharOrDots(testRule->charsdots[k], 0, table);
					character2 = findCharOrDots(input.chars[p + k], 0, table);
					if (character1->lowercase != character2->lowercase) break;
				}
				if (tryThis == 1 || k == testRule->charslen) {
					if (testRule->opcode == CTO_CompBrl ||
							testRule->opcode == CTO_Literal)
						return 0;
				}
				ruleOffset = testRule->charsnext;
			}
		}
	}
	return 1;
}

static int
isRepeatedWord(const TranslationTableHeader *table, int pos, InString input,
		int transCharslen, const widechar **repwordStart, int *repwordLength) {
	int start;
	if (pos == 0 || !checkAttr(input.chars[pos - 1], CTC_Letter, 0, table)) return 0;
	if ((pos + transCharslen) >= input.length ||
			!checkAttr(input.chars[pos + transCharslen], CTC_Letter, 0, table))
		return 0;
	for (start = pos - 2;
			start >= 0 && checkAttr(input.chars[start], CTC_Letter, 0, table); start--)
		;
	start++;
	*repwordStart = &input.chars[start];
	*repwordLength = pos - start;
	if (compareChars(*repwordStart, &input.chars[pos + transCharslen], *repwordLength, 0,
				table))
		return 1;
	return 0;
}

static int
checkEmphasisChange(const int skip, int pos, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, const TranslationTableRule *transRule) {
	int i;
	for (i = pos + (skip + 1); i < pos + transRule->charslen; i++)
		if (emphasisBuffer[i] || transNoteBuffer[i]) return 1;
	return 0;
}

static int
inSequence(const TranslationTableHeader *table, int pos, InString input,
		const TranslationTableRule *transRule) {
	int i, j, s, match;
	// TODO: all caps words
	// const TranslationTableCharacter *c = NULL;

	/* check before sequence */
	for (i = pos - 1; i >= 0; i--) {
		if (checkAttr(input.chars[i], CTC_SeqBefore, 0, table)) continue;
		if (!(checkAttr(input.chars[i], CTC_Space | CTC_SeqDelimiter, 0, table)))
			return 0;
		break;
	}

	/* check after sequence */
	for (i = pos + transRule->charslen; i < input.length; i++) {
		/* check sequence after patterns */
		if (table->seqPatternsCount) {
			match = 0;
			for (j = i, s = 0; j <= input.length && s < table->seqPatternsCount;
					j++, s++) {
				/* matching */
				if (match == 1) {
					if (table->seqPatterns[s]) {
						if (input.chars[j] == table->seqPatterns[s])
							match = 1;
						else {
							match = -1;
							j = i - 1;
						}
					}

					/* found match */
					else {
						/* pattern at end of input */
						if (j >= input.length) return 1;

						i = j;
						break;
					}
				}

				/* looking for match */
				else if (match == 0) {
					if (table->seqPatterns[s]) {
						if (input.chars[j] == table->seqPatterns[s])
							match = 1;
						else {
							match = -1;
							j = i - 1;
						}
					}
				}

				/* next pattarn */
				else if (match == -1) {
					if (!table->seqPatterns[s]) {
						match = 0;
						j = i - 1;
					}
				}
			}
		}

		if (checkAttr(input.chars[i], CTC_SeqAfter, 0, table)) continue;
		if (!(checkAttr(input.chars[i], CTC_Space | CTC_SeqDelimiter, 0, table)))
			return 0;
		break;
	}

	return 1;
}

static void
for_selectRule(const TranslationTableHeader *table, int pos, OutString output, int mode,
		InString input, formtype *typebuf, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, int *transOpcode, int prevTransOpcode,
		const TranslationTableRule **transRule, int *transCharslen, int *passCharDots,
		widechar const **passInstructions, int *passIC, int *startMatch,
		int *startReplace, int *endReplace, int *endMatch, int posIncremented,
		int cursorPosition, const widechar **repwordStart, int *repwordLength,
		int dontContract, int compbrlStart, int compbrlEnd,
		TranslationTableCharacterAttributes beforeAttributes,
		TranslationTableCharacter **curCharDef, TranslationTableRule **groupingRule,
		widechar *groupingOp) {
	/* check for valid Translations. Return value is in transRule. */
	static TranslationTableRule pseudoRule = { 0 };
	int length = input.length - pos;
	int tryThis;
	const TranslationTableCharacter *character2;
	int k;
	TranslationTableOffset ruleOffset = 0;
	*curCharDef = findCharOrDots(input.chars[pos], 0, table);
	for (tryThis = 0; tryThis < 3; tryThis++) {
		unsigned long int makeHash = 0;
		switch (tryThis) {
		case 0:
			if (!(length >= 2)) break;
			/* Hash function optimized for forward translation */
			makeHash = (unsigned long int)(*curCharDef)->lowercase << 8;
			character2 = findCharOrDots(input.chars[pos + 1], 0, table);
			makeHash += (unsigned long int)character2->lowercase;
			makeHash %= HASHNUM;
			ruleOffset = table->forRules[makeHash];
			break;
		case 1:
			if (!(length >= 1)) break;
			length = 1;
			ruleOffset = (*curCharDef)->otherRules;
			break;
		case 2: /* No rule found */
			*transRule = &pseudoRule;
			*transOpcode = pseudoRule.opcode = CTO_None;
			*transCharslen = pseudoRule.charslen = 1;
			pseudoRule.charsdots[0] = input.chars[pos];
			pseudoRule.dotslen = 0;
			return;
			break;
		}
		while (ruleOffset) {
			*transRule = (TranslationTableRule *)&table->ruleArea[ruleOffset];
			*transOpcode = (*transRule)->opcode;
			*transCharslen = (*transRule)->charslen;
			if (tryThis == 1 ||
					((*transCharslen <= length) && validMatch(table, pos, input, typebuf,
														   *transRule, *transCharslen))) {
				TranslationTableCharacterAttributes afterAttributes;
				/* check before emphasis match */
				if ((*transRule)->before & CTC_EmpMatch) {
					if (emphasisBuffer[pos] || transNoteBuffer[pos]) break;
				}

				/* check after emphasis match */
				if ((*transRule)->after & CTC_EmpMatch) {
					if (emphasisBuffer[pos + *transCharslen] ||
							transNoteBuffer[pos + *transCharslen])
						break;
				}

				/* check this rule */
				setAfter(*transCharslen, table, pos, input, &afterAttributes);
				if ((!((*transRule)->after & ~CTC_EmpMatch) ||
							(beforeAttributes & (*transRule)->after)) &&
						(!((*transRule)->before & ~CTC_EmpMatch) ||
								(afterAttributes & (*transRule)->before)))
					switch (*transOpcode) { /* check validity of this Translation */
					case CTO_Space:
					case CTO_Letter:
					case CTO_UpperCase:
					case CTO_LowerCase:
					case CTO_Digit:
					case CTO_LitDigit:
					case CTO_Punctuation:
					case CTO_Math:
					case CTO_Sign:
					case CTO_Hyphen:
					case CTO_Replace:
					case CTO_CompBrl:
					case CTO_Literal:
						return;
					case CTO_Repeated:
						if ((mode & (compbrlAtCursor | compbrlLeftCursor)) &&
								pos >= compbrlStart && pos <= compbrlEnd)
							break;
						return;
					case CTO_RepWord:
						if (dontContract || (mode & noContractions)) break;
						if (isRepeatedWord(table, pos, input, *transCharslen,
									repwordStart, repwordLength))
							return;
						break;
					case CTO_NoCont:
						if (dontContract || (mode & noContractions)) break;
						return;
					case CTO_Syllable:
						*transOpcode = CTO_Always;
					case CTO_Always:
						if (checkEmphasisChange(
									0, pos, emphasisBuffer, transNoteBuffer, *transRule))
							break;
						if (dontContract || (mode & noContractions)) break;
						return;
					case CTO_ExactDots:
						return;
					case CTO_NoCross:
						if (dontContract || (mode & noContractions)) break;
						if (syllableBreak(table, pos, input, *transCharslen)) break;
						return;
					case CTO_Context:
						if (!posIncremented ||
								!passDoTest(table, pos, input, *transOpcode, *transRule,
										passCharDots, passInstructions, passIC,
										startMatch, startReplace, endReplace, endMatch,
										groupingRule, groupingOp))
							break;
						return;
					case CTO_LargeSign:
						if (dontContract || (mode & noContractions)) break;
						if (!((beforeAttributes & (CTC_Space | CTC_Punctuation)) ||
									onlyLettersBehind(
											table, pos, input, beforeAttributes)) ||
								!((afterAttributes & CTC_Space) ||
										prevTransOpcode == CTO_LargeSign) ||
								(afterAttributes & CTC_Letter) ||
								!noCompbrlAhead(table, pos, mode, input, *transOpcode,
										*transCharslen, cursorPosition))
							*transOpcode = CTO_Always;
						return;
					case CTO_WholeWord:
						if (dontContract || (mode & noContractions)) break;
						if (checkEmphasisChange(
									0, pos, emphasisBuffer, transNoteBuffer, *transRule))
							break;
					case CTO_Contraction:
						if (table->usesSequences) {
							if (inSequence(table, pos, input, *transRule)) return;
						} else {
							if ((beforeAttributes & (CTC_Space | CTC_Punctuation)) &&
									(afterAttributes & (CTC_Space | CTC_Punctuation)))
								return;
						}
						break;
					case CTO_PartWord:
						if (dontContract || (mode & noContractions)) break;
						if ((beforeAttributes & CTC_Letter) ||
								(afterAttributes & CTC_Letter))
							return;
						break;
					case CTO_JoinNum:
						if (dontContract || (mode & noContractions)) break;
						if ((beforeAttributes & (CTC_Space | CTC_Punctuation)) &&
								(afterAttributes & CTC_Space) &&
								(output.length + (*transRule)->dotslen <
										output.maxlength)) {
							int p = pos + *transCharslen + 1;
							while (p < input.length) {
								if (!checkAttr(input.chars[p], CTC_Space, 0, table)) {
									if (checkAttr(input.chars[p], CTC_Digit, 0, table))
										return;
									break;
								}
								p++;
							}
						}
						break;
					case CTO_LowWord:
						if (dontContract || (mode & noContractions)) break;
						if ((beforeAttributes & CTC_Space) &&
								(afterAttributes & CTC_Space) &&
								(prevTransOpcode != CTO_JoinableWord))
							return;
						break;
					case CTO_JoinableWord:
						if (dontContract || (mode & noContractions)) break;
						if (beforeAttributes & (CTC_Space | CTC_Punctuation) &&
								onlyLettersAhead(table, pos, input, *transCharslen,
										afterAttributes) &&
								noCompbrlAhead(table, pos, mode, input, *transOpcode,
										*transCharslen, cursorPosition))
							return;
						break;
					case CTO_SuffixableWord:
						if (dontContract || (mode & noContractions)) break;
						if ((beforeAttributes & (CTC_Space | CTC_Punctuation)) &&
								(afterAttributes &
										(CTC_Space | CTC_Letter | CTC_Punctuation)))
							return;
						break;
					case CTO_PrefixableWord:
						if (dontContract || (mode & noContractions)) break;
						if ((beforeAttributes &
									(CTC_Space | CTC_Letter | CTC_Punctuation)) &&
								(afterAttributes & (CTC_Space | CTC_Punctuation)))
							return;
						break;
					case CTO_BegWord:
						if (dontContract || (mode & noContractions)) break;
						if ((beforeAttributes & (CTC_Space | CTC_Punctuation)) &&
								(afterAttributes & CTC_Letter))
							return;
						break;
					case CTO_BegMidWord:
						if (dontContract || (mode & noContractions)) break;
						if ((beforeAttributes &
									(CTC_Letter | CTC_Space | CTC_Punctuation)) &&
								(afterAttributes & CTC_Letter))
							return;
						break;
					case CTO_MidWord:
						if (dontContract || (mode & noContractions)) break;
						if (beforeAttributes & CTC_Letter && afterAttributes & CTC_Letter)
							return;
						break;
					case CTO_MidEndWord:
						if (dontContract || (mode & noContractions)) break;
						if (beforeAttributes & CTC_Letter &&
								afterAttributes &
										(CTC_Letter | CTC_Space | CTC_Punctuation))
							return;
						break;
					case CTO_EndWord:
						if (dontContract || (mode & noContractions)) break;
						if (beforeAttributes & CTC_Letter &&
								afterAttributes & (CTC_Space | CTC_Punctuation))
							return;
						break;
					case CTO_BegNum:
						if (beforeAttributes & (CTC_Space | CTC_Punctuation) &&
								afterAttributes & CTC_Digit)
							return;
						break;
					case CTO_MidNum:
						if (prevTransOpcode != CTO_ExactDots &&
								beforeAttributes & CTC_Digit &&
								afterAttributes & CTC_Digit)
							return;
						break;
					case CTO_EndNum:
						if (beforeAttributes & CTC_Digit &&
								prevTransOpcode != CTO_ExactDots)
							return;
						break;
					case CTO_DecPoint:
						if (!(afterAttributes & CTC_Digit)) break;
						if (beforeAttributes & CTC_Digit) *transOpcode = CTO_MidNum;
						return;
					case CTO_PrePunc:
						if (!checkAttr(input.chars[pos], CTC_Punctuation, 0, table) ||
								(pos > 0 && checkAttr(input.chars[pos - 1], CTC_Letter, 0,
													table)))
							break;
						for (k = pos + *transCharslen; k < input.length; k++) {
							if (checkAttr(input.chars[k], (CTC_Letter | CTC_Digit), 0,
										table))
								return;
							if (checkAttr(input.chars[k], CTC_Space, 0, table)) break;
						}
						break;
					case CTO_PostPunc:
						if (!checkAttr(input.chars[pos], CTC_Punctuation, 0, table) ||
								(pos < (input.length - 1) &&
										checkAttr(input.chars[pos + 1], CTC_Letter, 0,
												table)))
							break;
						for (k = pos; k >= 0; k--) {
							if (checkAttr(input.chars[k], (CTC_Letter | CTC_Digit), 0,
										table))
								return;
							if (checkAttr(input.chars[k], CTC_Space, 0, table)) break;
						}
						break;

					case CTO_Match: {
						widechar *patterns, *pattern;

						if (dontContract || (mode & noContractions)) break;
						if (checkEmphasisChange(
									0, pos, emphasisBuffer, transNoteBuffer, *transRule))
							break;

						patterns = (widechar *)&table->ruleArea[(*transRule)->patterns];

						/* check before pattern */
						pattern = &patterns[1];
						if (!_lou_pattern_check(
									input.chars, pos - 1, -1, -1, pattern, table))
							break;

						/* check after pattern */
						pattern = &patterns[patterns[0]];
						if (!_lou_pattern_check(input.chars, pos + (*transRule)->charslen,
									input.length, 1, pattern, table))
							break;

						return;
					}

					default:
						break;
					}
			}
			/* Done with checking this rule */
			ruleOffset = (*transRule)->charsnext;
		}
	}
}

static int
undefinedCharacter(widechar c, const TranslationTableHeader *table, int *pos, int mode,
		InString input, OutString *output, int *posMapping, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, const TranslationTableRule **transRule,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd) {
	/* Display an undefined character in the output buffer */
	int k;
	char *display;
	widechar displayDots[20];
	if (table->undefined) {
		TranslationTableRule *rule =
				(TranslationTableRule *)&table->ruleArea[table->undefined];
		if (!for_updatePositions(&rule->charsdots[rule->charslen], rule->charslen,
					rule->dotslen, 0, table, pos, mode, input, output, posMapping,
					emphasisBuffer, transNoteBuffer, transRule, cursorPosition,
					cursorStatus, compbrlStart, compbrlEnd))
			return 0;
		return 1;
	}
	display = _lou_showString(&c, 1);
	for (k = 0; k < (int)strlen(display); k++)
		displayDots[k] = _lou_getDotsForChar(display[k]);
	if (!for_updatePositions(displayDots, 1, (int)strlen(display), 0, table, pos, mode,
				input, output, posMapping, emphasisBuffer, transNoteBuffer, transRule,
				cursorPosition, cursorStatus, compbrlStart, compbrlEnd))
		return 0;
	return 1;
}

static int
putCharacter(widechar character, const TranslationTableHeader *table, int *pos, int mode,
		InString input, OutString *output, int *posMapping, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, const TranslationTableRule **transRule,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd) {
	/* Insert the dots equivalent of a character into the output buffer */
	const TranslationTableRule *rule = NULL;
	TranslationTableCharacter *chardef = NULL;
	TranslationTableOffset offset;
	widechar d;
	if (*cursorStatus == 2) return 1;
	chardef = (findCharOrDots(character, 0, table));
	if ((chardef->attributes & CTC_Letter) && (chardef->attributes & CTC_UpperCase))
		chardef = findCharOrDots(chardef->lowercase, 0, table);
	// TODO: for_selectRule and this function screw up Digit and LitDigit
	// NOTE: removed Litdigit from tables.
	// if(!chardef->otherRules)
	offset = chardef->definitionRule;
	// else
	//{
	//	offset = chardef->otherRules;
	//	rule = (TranslationTableRule *)&table->ruleArea[offset];
	//	while(rule->charsnext && rule->charsnext != chardef->definitionRule)
	//	{
	//		rule = (TranslationTableRule *)&table->ruleArea[offset];
	//		if(rule->charsnext)
	//			offset = rule->charsnext;
	//	}
	//}
	if (offset) {
		rule = (TranslationTableRule *)&table->ruleArea[offset];
		if (rule->dotslen)
			return for_updatePositions(&rule->charsdots[1], 1, rule->dotslen, 0, table,
					pos, mode, input, output, posMapping, emphasisBuffer, transNoteBuffer,
					transRule, cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
		d = _lou_getDotsForChar(character);
		return for_updatePositions(&d, 1, 1, 0, table, pos, mode, input, output,
				posMapping, emphasisBuffer, transNoteBuffer, transRule, cursorPosition,
				cursorStatus, compbrlStart, compbrlEnd);
	}
	return undefinedCharacter(character, table, pos, mode, input, output, posMapping,
			emphasisBuffer, transNoteBuffer, transRule, cursorPosition, cursorStatus,
			compbrlStart, compbrlEnd);
}

static int
putCharacters(const widechar *characters, int count, const TranslationTableHeader *table,
		int *pos, int mode, InString input, OutString *output, int *posMapping,
		unsigned int *emphasisBuffer, unsigned int *transNoteBuffer,
		const TranslationTableRule **transRule, int *cursorPosition, int *cursorStatus,
		int compbrlStart, int compbrlEnd) {
	/* Insert the dot equivalents of a series of characters in the output
	 * buffer */
	int k;
	for (k = 0; k < count; k++)
		if (!putCharacter(characters[k], table, pos, mode, input, output, posMapping,
					emphasisBuffer, transNoteBuffer, transRule, cursorPosition,
					cursorStatus, compbrlStart, compbrlEnd))
			return 0;
	return 1;
}

static int
doCompbrl(const TranslationTableHeader *table, int *pos, int mode, InString input,
		OutString *output, int *posMapping, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, const TranslationTableRule **transRule,
		int *cursorPosition, int *cursorStatus, int destword, int srcword,
		int compbrlStart, int compbrlEnd) {
	/* Handle strings containing substrings defined by the compbrl opcode */
	int stringStart, stringEnd;
	if (checkAttr(input.chars[*pos], CTC_Space, 0, table)) return 1;
	if (destword) {
		*pos = srcword;
		output->length = destword;
	} else {
		*pos = 0;
		output->length = 0;
	}
	for (stringStart = *pos; stringStart >= 0; stringStart--)
		if (checkAttr(input.chars[stringStart], CTC_Space, 0, table)) break;
	stringStart++;
	for (stringEnd = *pos; stringEnd < input.length; stringEnd++)
		if (checkAttr(input.chars[stringEnd], CTC_Space, 0, table)) break;
	return doCompTrans(stringStart, stringEnd, table, pos, mode, input, output,
			posMapping, emphasisBuffer, transNoteBuffer, transRule, cursorPosition,
			cursorStatus, compbrlStart, compbrlEnd);
}

static int
putCompChar(widechar character, const TranslationTableHeader *table, int *pos, int mode,
		InString input, OutString *output, int *posMapping, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, const TranslationTableRule **transRule,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd) {
	/* Insert the dots equivalent of a character into the output buffer */
	widechar d;
	TranslationTableOffset offset = (findCharOrDots(character, 0, table))->definitionRule;
	if (offset) {
		const TranslationTableRule *rule =
				(TranslationTableRule *)&table->ruleArea[offset];
		if (rule->dotslen)
			return for_updatePositions(&rule->charsdots[1], 1, rule->dotslen, 0, table,
					pos, mode, input, output, posMapping, emphasisBuffer, transNoteBuffer,
					transRule, cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
		d = _lou_getDotsForChar(character);
		return for_updatePositions(&d, 1, 1, 0, table, pos, mode, input, output,
				posMapping, emphasisBuffer, transNoteBuffer, transRule, cursorPosition,
				cursorStatus, compbrlStart, compbrlEnd);
	}
	return undefinedCharacter(character, table, pos, mode, input, output, posMapping,
			emphasisBuffer, transNoteBuffer, transRule, cursorPosition, cursorStatus,
			compbrlStart, compbrlEnd);
}

static int
doCompTrans(int start, int end, const TranslationTableHeader *table, int *pos, int mode,
		InString input, OutString *output, int *posMapping, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, const TranslationTableRule **transRule,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd) {
	const TranslationTableRule *indicRule;
	int k;
	int haveEndsegment = 0;
	if (*cursorStatus != 2 && brailleIndicatorDefined(table->begComp, table, &indicRule))
		if (!for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, 0,
					table, pos, mode, input, output, posMapping, emphasisBuffer,
					transNoteBuffer, transRule, cursorPosition, cursorStatus,
					compbrlStart, compbrlEnd))
			return 0;
	for (k = start; k < end; k++) {
		TranslationTableOffset compdots = 0;
		/* HACK: computer braille is one-to-one so it
		 * can't have any emphasis indicators.
		 * A better solution is to treat computer braille as its own mode. */
		emphasisBuffer[k] = 0;
		transNoteBuffer[k] = 0;
		if (input.chars[k] == ENDSEGMENT) {
			haveEndsegment = 1;
			continue;
		}
		*pos = k;
		if (input.chars[k] < 256) compdots = table->compdotsPattern[input.chars[k]];
		if (compdots != 0) {
			*transRule = (TranslationTableRule *)&table->ruleArea[compdots];
			if (!for_updatePositions(&(*transRule)->charsdots[(*transRule)->charslen],
						(*transRule)->charslen, (*transRule)->dotslen, 0, table, pos,
						mode, input, output, posMapping, emphasisBuffer, transNoteBuffer,
						transRule, cursorPosition, cursorStatus, compbrlStart,
						compbrlEnd))
				return 0;
		} else if (!putCompChar(input.chars[k], table, pos, mode, input, output,
						   posMapping, emphasisBuffer, transNoteBuffer, transRule,
						   cursorPosition, cursorStatus, compbrlStart, compbrlEnd))
			return 0;
	}
	if (*cursorStatus != 2 && brailleIndicatorDefined(table->endComp, table, &indicRule))
		if (!for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, 0,
					table, pos, mode, input, output, posMapping, emphasisBuffer,
					transNoteBuffer, transRule, cursorPosition, cursorStatus,
					compbrlStart, compbrlEnd))
			return 0;
	*pos = end;
	if (haveEndsegment) {
		widechar endSegment = ENDSEGMENT;
		if (!for_updatePositions(&endSegment, 0, 1, 0, table, pos, mode, input, output,
					posMapping, emphasisBuffer, transNoteBuffer, transRule,
					cursorPosition, cursorStatus, compbrlStart, compbrlEnd))
			return 0;
	}
	return 1;
}

static int
doNocont(const TranslationTableHeader *table, int *pos, OutString *output, int mode,
		InString input, int destword, int srcword, int *dontContract) {
	/* Handle strings containing substrings defined by the nocont opcode */
	if (checkAttr(input.chars[*pos], CTC_Space, 0, table) || *dontContract ||
			(mode & noContractions))
		return 1;
	if (destword) {
		*pos = srcword;
		output->length = destword;
	} else {
		*pos = 0;
		output->length = 0;
	}
	*dontContract = 1;
	return 1;
}

static int
markSyllables(const TranslationTableHeader *table, InString input, formtype *typebuf,
		int *transOpcode, const TranslationTableRule **transRule, int *transCharslen) {
	int pos;
	int k;
	int currentMark = 0;
	int const syllable_marks[] = { SYLLABLE_MARKER_1, SYLLABLE_MARKER_2 };
	int syllable_mark_selector = 0;

	if (typebuf == NULL || !table->syllables) return 1;
	pos = 0;
	while (pos < input.length) { /* the main multipass translation loop */
		int length = input.length - pos;
		const TranslationTableCharacter *character =
				findCharOrDots(input.chars[pos], 0, table);
		const TranslationTableCharacter *character2;
		int tryThis = 0;
		while (tryThis < 3) {
			TranslationTableOffset ruleOffset = 0;
			unsigned long int makeHash = 0;
			switch (tryThis) {
			case 0:
				if (!(length >= 2)) break;
				makeHash = (unsigned long int)character->lowercase << 8;
				// memory overflow when pos == input.length - 1
				character2 = findCharOrDots(input.chars[pos + 1], 0, table);
				makeHash += (unsigned long int)character2->lowercase;
				makeHash %= HASHNUM;
				ruleOffset = table->forRules[makeHash];
				break;
			case 1:
				if (!(length >= 1)) break;
				length = 1;
				ruleOffset = character->otherRules;
				break;
			case 2: /* No rule found */
				*transOpcode = CTO_Always;
				ruleOffset = 0;
				break;
			}
			while (ruleOffset) {
				*transRule = (TranslationTableRule *)&table->ruleArea[ruleOffset];
				*transOpcode = (*transRule)->opcode;
				*transCharslen = (*transRule)->charslen;
				if (tryThis == 1 ||
						(*transCharslen <= length &&
								compareChars(&(*transRule)->charsdots[0],
										&input.chars[pos], *transCharslen, 0, table))) {
					if (*transOpcode == CTO_Syllable) {
						tryThis = 4;
						break;
					}
				}
				ruleOffset = (*transRule)->charsnext;
			}
			tryThis++;
		}
		switch (*transOpcode) {
		case CTO_Always:
			if (pos >= input.length) return 0;
			typebuf[pos++] |= currentMark;
			break;
		case CTO_Syllable:
			/* cycle between SYLLABLE_MARKER_1 and SYLLABLE_MARKER_2 so
			 * we can distinguinsh two consequtive syllables */
			currentMark = syllable_marks[syllable_mark_selector];
			syllable_mark_selector = (syllable_mark_selector + 1) % 2;

			if ((pos + *transCharslen) > input.length) return 0;
			for (k = 0; k < *transCharslen; k++) typebuf[pos++] |= currentMark;
			break;
		default:
			break;
		}
	}
	return 1;
}

static void
resolveEmphasisWords(unsigned int *buffer, const unsigned int bit_begin,
		const unsigned int bit_end, const unsigned int bit_word,
		const unsigned int bit_symbol, InString input, unsigned int *wordBuffer) {
	int in_word = 0, in_emp = 0, word_start = -1, word_whole = 0, word_stop;
	int i;

	for (i = 0; i < input.length; i++) {
		// TODO: give each emphasis its own whole word bit?
		/* clear out previous whole word markings */
		wordBuffer[i] &= ~WORD_WHOLE;

		/* check if at beginning of emphasis */
		if (!in_emp)
			if (buffer[i] & bit_begin) {
				in_emp = 1;
				buffer[i] &= ~bit_begin;

				/* emphasis started inside word */
				if (in_word) {
					word_start = i;
					word_whole = 0;
				}

				/* emphasis started on space */
				if (!(wordBuffer[i] & WORD_CHAR)) word_start = -1;
			}

		/* check if at end of emphasis */
		if (in_emp)
			if (buffer[i] & bit_end) {
				in_emp = 0;
				buffer[i] &= ~bit_end;

				if (in_word && word_start >= 0) {
					/* check if emphasis ended inside a word */
					word_stop = bit_end | bit_word;
					if (wordBuffer[i] & WORD_CHAR)
						word_whole = 0;
					else
						word_stop = 0;

					/* if whole word is one symbol,
					 * turn it into a symbol */
					if (bit_symbol && word_start + 1 == i)
						buffer[word_start] |= bit_symbol;
					else {
						buffer[word_start] |= bit_word;
						buffer[i] |= word_stop;
					}
					wordBuffer[word_start] |= word_whole;
				}
			}

		/* check if at beginning of word */
		if (!in_word)
			if (wordBuffer[i] & WORD_CHAR) {
				in_word = 1;
				if (in_emp) {
					word_whole = WORD_WHOLE;
					word_start = i;
				}
			}

		/* check if at end of word */
		if (in_word)
			if (!(wordBuffer[i] & WORD_CHAR)) {
				/* made it through whole word */
				if (in_emp && word_start >= 0) {
					/* if word is one symbol,
					 * turn it into a symbol */
					if (bit_symbol && word_start + 1 == i)
						buffer[word_start] |= bit_symbol;
					else
						buffer[word_start] |= bit_word;
					wordBuffer[word_start] |= word_whole;
				}

				in_word = 0;
				word_whole = 0;
				word_start = -1;
			}
	}

	/* clean up end */
	if (in_emp) {
		buffer[i] &= ~bit_end;

		if (in_word)
			if (word_start >= 0) {
				/* if word is one symbol,
				 * turn it into a symbol */
				if (bit_symbol && word_start + 1 == i)
					buffer[word_start] |= bit_symbol;
				else
					buffer[word_start] |= bit_word;
				wordBuffer[word_start] |= word_whole;
			}
	}
}

static void
convertToPassage(const int pass_start, const int pass_end, const int word_start,
		unsigned int *buffer, const EmphRuleNumber emphRule, const unsigned int bit_begin,
		const unsigned int bit_end, const unsigned int bit_word,
		const unsigned int bit_symbol, const TranslationTableHeader *table,
		unsigned int *wordBuffer) {
	int i;
	const TranslationTableRule *indicRule;

	for (i = pass_start; i <= pass_end; i++)
		if (wordBuffer[i] & WORD_WHOLE) {
			buffer[i] &= ~(bit_symbol | bit_word);
			wordBuffer[i] &= ~WORD_WHOLE;
		}

	buffer[pass_start] |= bit_begin;
	if (brailleIndicatorDefined(
				table->emphRules[emphRule][endOffset], table, &indicRule) ||
			brailleIndicatorDefined(
					table->emphRules[emphRule][endPhraseAfterOffset], table, &indicRule))
		buffer[pass_end] |= bit_end;
	else if (brailleIndicatorDefined(table->emphRules[emphRule][endPhraseBeforeOffset],
					 table, &indicRule))
		buffer[word_start] |= bit_end;
}

static void
resolveEmphasisPassages(unsigned int *buffer, const EmphRuleNumber emphRule,
		const unsigned int bit_begin, const unsigned int bit_end,
		const unsigned int bit_word, const unsigned int bit_symbol,
		const TranslationTableHeader *table, InString input, unsigned int *wordBuffer) {
	unsigned int word_cnt = 0;
	int pass_start = -1, pass_end = -1, word_start = -1, in_word = 0, in_pass = 0;
	int i;

	for (i = 0; i < input.length; i++) {
		/* check if at beginning of word */
		if (!in_word)
			if (wordBuffer[i] & WORD_CHAR) {
				in_word = 1;
				if (wordBuffer[i] & WORD_WHOLE) {
					if (!in_pass) {
						in_pass = 1;
						pass_start = i;
						pass_end = -1;
						word_cnt = 1;
					} else
						word_cnt++;
					word_start = i;
					continue;
				} else if (in_pass) {
					if (word_cnt >= table->emphRules[emphRule][lenPhraseOffset])
						if (pass_end >= 0) {
							convertToPassage(pass_start, pass_end, word_start, buffer,
									emphRule, bit_begin, bit_end, bit_word, bit_symbol,
									table, wordBuffer);
						}
					in_pass = 0;
				}
			}

		/* check if at end of word */
		if (in_word)
			if (!(wordBuffer[i] & WORD_CHAR)) {
				in_word = 0;
				if (in_pass) pass_end = i;
			}

		if (in_pass)
			if (buffer[i] & bit_begin || buffer[i] & bit_end || buffer[i] & bit_word ||
					buffer[i] & bit_symbol) {
				if (word_cnt >= table->emphRules[emphRule][lenPhraseOffset])
					if (pass_end >= 0) {
						convertToPassage(pass_start, pass_end, word_start, buffer,
								emphRule, bit_begin, bit_end, bit_word, bit_symbol, table,
								wordBuffer);
					}
				in_pass = 0;
			}
	}

	if (in_pass) {
		if (word_cnt >= table->emphRules[emphRule][lenPhraseOffset]) {
			if (pass_end >= 0) {
				if (in_word) {
					convertToPassage(pass_start, i, word_start, buffer, emphRule,
							bit_begin, bit_end, bit_word, bit_symbol, table, wordBuffer);
				} else {
					convertToPassage(pass_start, pass_end, word_start, buffer, emphRule,
							bit_begin, bit_end, bit_word, bit_symbol, table, wordBuffer);
				}
			}
		}
	}
}

static void
resolveEmphasisSingleSymbols(unsigned int *buffer, const unsigned int bit_begin,
		const unsigned int bit_end, const unsigned int bit_symbol, InString input) {
	int i;

	for (i = 0; i < input.length; i++) {
		if (buffer[i] & bit_begin)
			if (buffer[i + 1] & bit_end) {
				buffer[i] &= ~bit_begin;
				buffer[i + 1] &= ~bit_end;
				buffer[i] |= bit_symbol;
			}
	}
}

static void
resolveEmphasisAllCapsSymbols(unsigned int *buffer, formtype *typebuf, InString input) {
	/* Marks every caps letter with CAPS_SYMBOL.
	 * Used in the special case where capsnocont has been defined and capsword has not
	 * been defined. */

	const unsigned int bit_begin = CAPS_BEGIN;
	const unsigned int bit_end = CAPS_END;
	const unsigned int bit_symbol = CAPS_SYMBOL;

	int inEmphasis = 0, i;

	for (i = 0; i < input.length; i++) {
		if (buffer[i] & bit_end) {
			inEmphasis = 0;
			buffer[i] &= ~bit_end;
		} else {
			if (buffer[i] & bit_begin) {
				buffer[i] &= ~bit_begin;
				inEmphasis = 1;
			}
			if (inEmphasis) {
				if (typebuf[i] & CAPSEMPH)
					/* Only mark if actually a capital letter (don't mark spaces or
					 * punctuation). */
					buffer[i] |= bit_symbol;
			} /* In emphasis */
		}	 /* Not bit_end */
	}
}

static void
resolveEmphasisResets(unsigned int *buffer, const unsigned int bit_begin,
		const unsigned int bit_end, const unsigned int bit_word,
		const unsigned int bit_symbol, const TranslationTableHeader *table,
		InString input, unsigned int *wordBuffer) {
	int in_word = 0, in_pass = 0, word_start = -1, word_reset = 0, orig_reset = -1,
		letter_cnt = 0;
	int i, j;

	for (i = 0; i < input.length; i++) {
		if (in_pass)
			if (buffer[i] & bit_end) in_pass = 0;

		if (!in_pass) {
			if (buffer[i] & bit_begin)
				in_pass = 1;
			else {
				if (!in_word) {
					if (buffer[i] & bit_word) {
						/* deal with case when reset
						 * was at beginning of word */
						if (wordBuffer[i] & WORD_RESET ||
								!checkAttr(input.chars[i], CTC_Letter, 0, table)) {
							/* not just one reset by itself */
							if (wordBuffer[i + 1] & WORD_CHAR) {
								buffer[i + 1] |= bit_word;
								if (wordBuffer[i] & WORD_WHOLE)
									wordBuffer[i + 1] |= WORD_WHOLE;
							}
							buffer[i] &= ~bit_word;
							wordBuffer[i] &= ~WORD_WHOLE;

							/* if reset is a letter, make it a symbol */
							if (checkAttr(input.chars[i], CTC_Letter, 0, table))
								buffer[i] |= bit_symbol;

							continue;
						}

						in_word = 1;
						word_start = i;
						letter_cnt = 0;
						word_reset = 0;
					}

					/* it is possible for a character to have been
					 * marked as a symbol when it should not be one */
					else if (buffer[i] & bit_symbol) {
						if (wordBuffer[i] & WORD_RESET ||
								!checkAttr(input.chars[i], CTC_Letter, 0, table))
							buffer[i] &= ~bit_symbol;
					}
				}

				if (in_word) {

					/* at end of word */
					if (!(wordBuffer[i] & WORD_CHAR) ||
							(buffer[i] & bit_word && buffer[i] & bit_end)) {
						in_word = 0;

						/* check if symbol */
						if (bit_symbol && letter_cnt == 1) {
							buffer[word_start] |= bit_symbol;
							buffer[word_start] &= ~bit_word;
							wordBuffer[word_start] &= ~WORD_WHOLE;
							buffer[i] &= ~(bit_end | bit_word);
						}

						/* if word ended on a reset or last char was a reset,
						 * get rid of end bits */
						if (word_reset || wordBuffer[i] & WORD_RESET ||
								!checkAttr(input.chars[i], CTC_Letter, 0, table))
							buffer[i] &= ~(bit_end | bit_word);

						/* if word ended when it began, get rid of all bits */
						if (i == word_start) {
							wordBuffer[word_start] &= ~WORD_WHOLE;
							buffer[i] &= ~(bit_end | bit_word);
						}
						orig_reset = -1;
					} else {
						/* hit reset */
						if (wordBuffer[i] & WORD_RESET ||
								!checkAttr(input.chars[i], CTC_Letter, 0, table)) {
							if (!checkAttr(input.chars[i], CTC_Letter, 0, table)) {
								if (checkAttr(input.chars[i], CTC_CapsMode, 0, table)) {
									/* chars marked as not resetting */
									orig_reset = i;
									continue;
								} else if (orig_reset >= 0) {
									/* invalid no reset sequence */
									for (j = orig_reset; j < i; j++)
										buffer[j] &= ~bit_word;
									// word_reset = 1;
									orig_reset = -1;
								}
							}

							/* check if symbol is not already resetting */
							if (bit_symbol && letter_cnt == 1) {
								buffer[word_start] |= bit_symbol;
								buffer[word_start] &= ~bit_word;
								wordBuffer[word_start] &= ~WORD_WHOLE;
								// buffer[i] &= ~(bit_end | WORD_STOP);
							}

							/* if reset is a letter, make it the new word_start */
							if (checkAttr(input.chars[i], CTC_Letter, 0, table)) {
								word_reset = 0;
								word_start = i;
								letter_cnt = 1;
								buffer[i] |= bit_word;
							} else
								word_reset = 1;

							continue;
						}

						if (word_reset) {
							word_reset = 0;
							word_start = i;
							letter_cnt = 0;
							buffer[i] |= bit_word;
						}

						letter_cnt++;
					}
				}
			}
		}
	}

	/* clean up end */
	if (in_word) {
		/* check if symbol */
		if (bit_symbol && letter_cnt == 1) {
			buffer[word_start] |= bit_symbol;
			buffer[word_start] &= ~bit_word;
			wordBuffer[word_start] &= ~WORD_WHOLE;
			buffer[i] &= ~(bit_end | bit_word);
		}

		if (word_reset) buffer[i] &= ~(bit_end | bit_word);
	}
}

static void
markEmphases(const TranslationTableHeader *table, InString input, formtype *typebuf,
		unsigned int *wordBuffer, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, int haveEmphasis, int *cursorPosition,
		int *cursorStatus, int compbrlStart, int compbrlEnd) {
	/* Relies on the order of typeforms emph_1..emph_10. */
	int caps_start = -1, last_caps = -1, caps_cnt = 0;
	int emph_start[10] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
	int i, j;

	for (i = 0; i < input.length; i++) {
		if (!checkAttr(input.chars[i], CTC_Space, 0, table)) {
			wordBuffer[i] |= WORD_CHAR;
		} else if (caps_cnt > 0) {
			last_caps = i;
			caps_cnt = 0;
		}

		if (checkAttr(input.chars[i], CTC_UpperCase, 0, table)) {
			if (caps_start < 0) caps_start = i;
			caps_cnt++;
		} else if (caps_start >= 0) {
			/* caps should keep going until this */
			if (checkAttr(input.chars[i], CTC_Letter, 0, table) &&
					checkAttr(input.chars[i], CTC_LowerCase, 0, table)) {
				emphasisBuffer[caps_start] |= CAPS_BEGIN;
				if (caps_cnt > 0)
					emphasisBuffer[i] |= CAPS_END;
				else
					emphasisBuffer[last_caps] |= CAPS_END;
				caps_start = -1;
				last_caps = -1;
				caps_cnt = 0;
			}
		}

		if (!haveEmphasis) continue;

		for (j = 0; j < 10; j++) {
			if (typebuf[i] & (italic << j)) {
				if (emph_start[j] < 0) emph_start[j] = i;
			} else if (emph_start[j] >= 0) {
				if (j < 5) {
					emphasisBuffer[emph_start[j]] |= EMPHASIS_BEGIN << (j * 4);
					emphasisBuffer[i] |= EMPHASIS_END << (j * 4);
				} else {
					transNoteBuffer[emph_start[j]] |= TRANSNOTE_BEGIN << ((j - 5) * 4);
					transNoteBuffer[i] |= TRANSNOTE_END << ((j - 5) * 4);
				}
				emph_start[j] = -1;
			}
		}
	}

	/* clean up input.length */
	if (caps_start >= 0) {
		emphasisBuffer[caps_start] |= CAPS_BEGIN;
		if (caps_cnt > 0)
			emphasisBuffer[input.length] |= CAPS_END;
		else
			emphasisBuffer[last_caps] |= CAPS_END;
	}

	if (haveEmphasis) {
		for (j = 0; j < 10; j++) {
			if (emph_start[j] >= 0) {
				if (j < 5) {
					emphasisBuffer[emph_start[j]] |= EMPHASIS_BEGIN << (j * 4);
					emphasisBuffer[input.length] |= EMPHASIS_END << (j * 4);
				} else {
					transNoteBuffer[emph_start[j]] |= TRANSNOTE_BEGIN << ((j - 5) * 4);
					transNoteBuffer[input.length] |= TRANSNOTE_END << ((j - 5) * 4);
				}
			}
		}
	}

	/* Handle capsnocont */
	if (table->capsNoCont) {
		int inCaps = 0;
		for (i = 0; i < input.length; i++) {
			if (emphasisBuffer[i] & CAPS_END) {
				inCaps = 0;
			} else {
				if ((emphasisBuffer[i] & CAPS_BEGIN) &&
						!(emphasisBuffer[i + 1] & CAPS_END))
					inCaps = 1;
				if (inCaps) typebuf[i] |= no_contract;
			}
		}
	}

	if (table->emphRules[capsRule][begWordOffset]) {
		resolveEmphasisWords(emphasisBuffer, CAPS_BEGIN, CAPS_END, CAPS_WORD, CAPS_SYMBOL,
				input, wordBuffer);
		if (table->emphRules[capsRule][lenPhraseOffset])
			resolveEmphasisPassages(emphasisBuffer, capsRule, CAPS_BEGIN, CAPS_END,
					CAPS_WORD, CAPS_SYMBOL, table, input, wordBuffer);
		resolveEmphasisResets(emphasisBuffer, CAPS_BEGIN, CAPS_END, CAPS_WORD,
				CAPS_SYMBOL, table, input, wordBuffer);
	} else if (table->emphRules[capsRule][letterOffset]) {
		if (table->capsNoCont) /* capsnocont and no capsword */
			resolveEmphasisAllCapsSymbols(emphasisBuffer, typebuf, input);
		else
			resolveEmphasisSingleSymbols(
					emphasisBuffer, CAPS_BEGIN, CAPS_END, CAPS_SYMBOL, input);
	}

	if (!haveEmphasis) return;

	for (j = 0; j < 5; j++) {
		if (table->emphRules[emph1Rule + j][begWordOffset]) {
			resolveEmphasisWords(emphasisBuffer, EMPHASIS_BEGIN << (j * 4),
					EMPHASIS_END << (j * 4), EMPHASIS_WORD << (j * 4),
					EMPHASIS_SYMBOL << (j * 4), input, wordBuffer);
			if (table->emphRules[emph1Rule + j][lenPhraseOffset])
				resolveEmphasisPassages(emphasisBuffer, emph1Rule + j,
						EMPHASIS_BEGIN << (j * 4), EMPHASIS_END << (j * 4),
						EMPHASIS_WORD << (j * 4), EMPHASIS_SYMBOL << (j * 4), table,
						input, wordBuffer);
		} else if (table->emphRules[emph1Rule + j][letterOffset])
			resolveEmphasisSingleSymbols(emphasisBuffer, EMPHASIS_BEGIN << (j * 4),
					EMPHASIS_END << (j * 4), EMPHASIS_SYMBOL << (j * 4), input);
	}
	for (j = 0; j < 5; j++) {
		if (table->emphRules[emph6Rule + j][begWordOffset]) {
			resolveEmphasisWords(transNoteBuffer, TRANSNOTE_BEGIN << (j * 4),
					TRANSNOTE_END << (j * 4), TRANSNOTE_WORD << (j * 4),
					TRANSNOTE_SYMBOL << (j * 4), input, wordBuffer);
			if (table->emphRules[emph6Rule + j][lenPhraseOffset])
				resolveEmphasisPassages(transNoteBuffer, j + 6,
						TRANSNOTE_BEGIN << (j * 4), TRANSNOTE_END << (j * 4),
						TRANSNOTE_WORD << (j * 4), TRANSNOTE_SYMBOL << (j * 4), table,
						input, wordBuffer);
		} else if (table->emphRules[emph6Rule + j][letterOffset])
			resolveEmphasisSingleSymbols(transNoteBuffer, TRANSNOTE_BEGIN << (j * 4),
					TRANSNOTE_END << (j * 4), TRANSNOTE_SYMBOL << (j * 4), input);
	}
}

static void
insertEmphasisSymbol(unsigned int *buffer, const int at, const EmphRuleNumber emphRule,
		const unsigned int bit_symbol, const TranslationTableHeader *table, int *pos,
		int mode, InString input, OutString *output, int *posMapping,
		unsigned int *emphasisBuffer, unsigned int *transNoteBuffer,
		const TranslationTableRule **transRule, int *cursorPosition, int *cursorStatus,
		int compbrlStart, int compbrlEnd) {
	if (buffer[at] & bit_symbol) {
		const TranslationTableRule *indicRule;
		if (brailleIndicatorDefined(
					table->emphRules[emphRule][letterOffset], table, &indicRule))
			for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, 0, table,
					pos, mode, input, output, posMapping, emphasisBuffer, transNoteBuffer,
					transRule, cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
	}
}

static void
insertEmphasisBegin(unsigned int *buffer, const int at, const EmphRuleNumber emphRule,
		const unsigned int bit_begin, const unsigned int bit_end,
		const unsigned int bit_word, const TranslationTableHeader *table, int *pos,
		int mode, InString input, OutString *output, int *posMapping,
		unsigned int *emphasisBuffer, unsigned int *transNoteBuffer,
		const TranslationTableRule **transRule, int *cursorPosition, int *cursorStatus,
		int compbrlStart, int compbrlEnd) {
	const TranslationTableRule *indicRule;
	if (buffer[at] & bit_begin) {
		if (brailleIndicatorDefined(
					table->emphRules[emphRule][begPhraseOffset], table, &indicRule))
			for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, 0, table,
					pos, mode, input, output, posMapping, emphasisBuffer, transNoteBuffer,
					transRule, cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
		else if (brailleIndicatorDefined(
						 table->emphRules[emphRule][begOffset], table, &indicRule))
			for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, 0, table,
					pos, mode, input, output, posMapping, emphasisBuffer, transNoteBuffer,
					transRule, cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
	}

	if (buffer[at] & bit_word
			// && !(buffer[at] & bit_begin)
			&& !(buffer[at] & bit_end)) {
		if (brailleIndicatorDefined(
					table->emphRules[emphRule][begWordOffset], table, &indicRule))
			for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, 0, table,
					pos, mode, input, output, posMapping, emphasisBuffer, transNoteBuffer,
					transRule, cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
	}
}

static void
insertEmphasisEnd(unsigned int *buffer, const int at, const EmphRuleNumber emphRule,
		const unsigned int bit_end, const unsigned int bit_word,
		const TranslationTableHeader *table, int *pos, int mode, InString input,
		OutString *output, int *posMapping, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, const TranslationTableRule **transRule,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd) {
	if (buffer[at] & bit_end) {
		const TranslationTableRule *indicRule;
		if (buffer[at] & bit_word) {
			if (brailleIndicatorDefined(
						table->emphRules[emphRule][endWordOffset], table, &indicRule))
				for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, -1,
						table, pos, mode, input, output, posMapping, emphasisBuffer,
						transNoteBuffer, transRule, cursorPosition, cursorStatus,
						compbrlStart, compbrlEnd);
		} else {
			if (brailleIndicatorDefined(
						table->emphRules[emphRule][endOffset], table, &indicRule))
				for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, -1,
						table, pos, mode, input, output, posMapping, emphasisBuffer,
						transNoteBuffer, transRule, cursorPosition, cursorStatus,
						compbrlStart, compbrlEnd);
			else if (brailleIndicatorDefined(
							 table->emphRules[emphRule][endPhraseAfterOffset], table,
							 &indicRule))
				for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, -1,
						table, pos, mode, input, output, posMapping, emphasisBuffer,
						transNoteBuffer, transRule, cursorPosition, cursorStatus,
						compbrlStart, compbrlEnd);
			else if (brailleIndicatorDefined(
							 table->emphRules[emphRule][endPhraseBeforeOffset], table,
							 &indicRule))
				for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, 0,
						table, pos, mode, input, output, posMapping, emphasisBuffer,
						transNoteBuffer, transRule, cursorPosition, cursorStatus,
						compbrlStart, compbrlEnd);
		}
	}
}

static int
endCount(unsigned int *buffer, const int at, const unsigned int bit_end,
		const unsigned int bit_begin, const unsigned int bit_word) {
	int i, cnt = 1;
	if (!(buffer[at] & bit_end)) return 0;
	for (i = at - 1; i >= 0; i--)
		if (buffer[i] & bit_begin || buffer[i] & bit_word)
			break;
		else
			cnt++;
	return cnt;
}

static int
beginCount(unsigned int *buffer, const int at, const unsigned int bit_end,
		const unsigned int bit_begin, const unsigned int bit_word,
		const TranslationTableHeader *table, InString input) {
	if (buffer[at] & bit_begin) {
		int i, cnt = 1;
		for (i = at + 1; i < input.length; i++)
			if (buffer[i] & bit_end)
				break;
			else
				cnt++;
		return cnt;
	} else if (buffer[at] & bit_word) {
		int i, cnt = 1;
		for (i = at + 1; i < input.length; i++)
			if (buffer[i] & bit_end) break;
			// TODO: WORD_RESET?
			else if (checkAttr(input.chars[i], CTC_SeqDelimiter | CTC_Space, 0, table))
				break;
			else
				cnt++;
		return cnt;
	}
	return 0;
}

static void
insertEmphasesAt(const int at, const TranslationTableHeader *table, int *pos, int mode,
		InString input, OutString *output, int *posMapping, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, int haveEmphasis, int transOpcode,
		const TranslationTableRule **transRule, int *cursorPosition, int *cursorStatus,
		int compbrlStart, int compbrlEnd) {
	int type_counts[10];
	int i, j, min, max;

	/* simple case */
	if (!haveEmphasis) {
		/* insert graded 1 mode indicator */
		if (transOpcode == CTO_Contraction) {
			const TranslationTableRule *indicRule;
			if (brailleIndicatorDefined(table->noContractSign, table, &indicRule))
				for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, 0,
						table, pos, mode, input, output, posMapping, emphasisBuffer,
						transNoteBuffer, transRule, cursorPosition, cursorStatus,
						compbrlStart, compbrlEnd);
		}

		if (emphasisBuffer[at] & CAPS_EMPHASIS) {
			insertEmphasisEnd(emphasisBuffer, at, capsRule, CAPS_END, CAPS_WORD, table,
					pos, mode, input, output, posMapping, emphasisBuffer, transNoteBuffer,
					transRule, cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
			insertEmphasisBegin(emphasisBuffer, at, capsRule, CAPS_BEGIN, CAPS_END,
					CAPS_WORD, table, pos, mode, input, output, posMapping,
					emphasisBuffer, transNoteBuffer, transRule, cursorPosition,
					cursorStatus, compbrlStart, compbrlEnd);
			insertEmphasisSymbol(emphasisBuffer, at, capsRule, CAPS_SYMBOL, table, pos,
					mode, input, output, posMapping, emphasisBuffer, transNoteBuffer,
					transRule, cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
		}
		return;
	}

	/* The order of inserting the end symbols must be the reverse
	 * of the insertions of the begin symbols so that they will
	 * nest properly when multiple emphases start and end at
	 * the same place */
	// TODO: ordering with partial word using bit_word and bit_end

	if (emphasisBuffer[at] & CAPS_EMPHASIS)
		insertEmphasisEnd(emphasisBuffer, at, capsRule, CAPS_END, CAPS_WORD, table, pos,
				mode, input, output, posMapping, emphasisBuffer, transNoteBuffer,
				transRule, cursorPosition, cursorStatus, compbrlStart, compbrlEnd);

	/* end bits */
	for (i = 0; i < 10; i++)
		if (i < 5)
			type_counts[i] = endCount(emphasisBuffer, at, EMPHASIS_END << (i * 4),
					EMPHASIS_BEGIN << (i * 4), EMPHASIS_WORD << (i * 4));
		else
			type_counts[i] = endCount(transNoteBuffer, at, TRANSNOTE_END << ((i - 5) * 4),
					TRANSNOTE_BEGIN << ((i - 5) * 4), TRANSNOTE_WORD << ((i - 5) * 4));

	for (i = 0; i < 10; i++) {
		min = -1;
		for (j = 0; j < 10; j++)
			if (type_counts[j] > 0)
				if (min < 0 || type_counts[j] < type_counts[min]) min = j;
		if (min < 0) break;
		type_counts[min] = 0;
		if (min < 5)
			insertEmphasisEnd(emphasisBuffer, at, emph1Rule + min,
					EMPHASIS_END << (min * 4), EMPHASIS_WORD << (min * 4), table, pos,
					mode, input, output, posMapping, emphasisBuffer, transNoteBuffer,
					transRule, cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
		else
			insertEmphasisEnd(transNoteBuffer, at, emph1Rule + min,
					TRANSNOTE_END << ((min - 5) * 4), TRANSNOTE_WORD << ((min - 5) * 4),
					table, pos, mode, input, output, posMapping, emphasisBuffer,
					transNoteBuffer, transRule, cursorPosition, cursorStatus,
					compbrlStart, compbrlEnd);
	}

	/* begin and word bits */
	for (i = 0; i < 10; i++)
		if (i < 5)
			type_counts[i] = beginCount(emphasisBuffer, at, EMPHASIS_END << (i * 4),
					EMPHASIS_BEGIN << (i * 4), EMPHASIS_WORD << (i * 4), table, input);
		else
			type_counts[i] = beginCount(transNoteBuffer, at,
					TRANSNOTE_END << ((i - 5) * 4), TRANSNOTE_BEGIN << ((i - 5) * 4),
					TRANSNOTE_WORD << ((i - 5) * 4), table, input);

	for (i = 9; i >= 0; i--) {
		max = 9;
		for (j = 9; j >= 0; j--)
			if (type_counts[max] < type_counts[j]) max = j;
		if (!type_counts[max]) break;
		type_counts[max] = 0;
		if (max >= 5)
			insertEmphasisBegin(transNoteBuffer, at, emph1Rule + max,
					TRANSNOTE_BEGIN << ((max - 5) * 4), TRANSNOTE_END << ((max - 5) * 4),
					TRANSNOTE_WORD << ((max - 5) * 4), table, pos, mode, input, output,
					posMapping, emphasisBuffer, transNoteBuffer, transRule,
					cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
		else
			insertEmphasisBegin(emphasisBuffer, at, emph1Rule + max,
					EMPHASIS_BEGIN << (max * 4), EMPHASIS_END << (max * 4),
					EMPHASIS_WORD << (max * 4), table, pos, mode, input, output,
					posMapping, emphasisBuffer, transNoteBuffer, transRule,
					cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
	}

	/* symbol bits */
	for (i = 4; i >= 0; i--)
		if (transNoteBuffer[at] & (TRANSNOTE_MASK << (i * 4)))
			insertEmphasisSymbol(transNoteBuffer, at, emph6Rule + i,
					TRANSNOTE_SYMBOL << (i * 4), table, pos, mode, input, output,
					posMapping, emphasisBuffer, transNoteBuffer, transRule,
					cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
	for (i = 4; i >= 0; i--)
		if (emphasisBuffer[at] & (EMPHASIS_MASK << (i * 4)))
			insertEmphasisSymbol(emphasisBuffer, at, emph1Rule + i,
					EMPHASIS_SYMBOL << (i * 4), table, pos, mode, input, output,
					posMapping, emphasisBuffer, transNoteBuffer, transRule,
					cursorPosition, cursorStatus, compbrlStart, compbrlEnd);

	/* insert graded 1 mode indicator */
	if (transOpcode == CTO_Contraction) {
		const TranslationTableRule *indicRule;
		if (brailleIndicatorDefined(table->noContractSign, table, &indicRule))
			for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, 0, table,
					pos, mode, input, output, posMapping, emphasisBuffer, transNoteBuffer,
					transRule, cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
	}

	/* insert capitalization last so it will be closest to word */
	if (emphasisBuffer[at] & CAPS_EMPHASIS) {
		insertEmphasisBegin(emphasisBuffer, at, capsRule, CAPS_BEGIN, CAPS_END, CAPS_WORD,
				table, pos, mode, input, output, posMapping, emphasisBuffer,
				transNoteBuffer, transRule, cursorPosition, cursorStatus, compbrlStart,
				compbrlEnd);
		insertEmphasisSymbol(emphasisBuffer, at, capsRule, CAPS_SYMBOL, table, pos, mode,
				input, output, posMapping, emphasisBuffer, transNoteBuffer, transRule,
				cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
	}
}

static void
insertEmphases(const TranslationTableHeader *table, int *pos, int mode, InString input,
		OutString *output, int *posMapping, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, int haveEmphasis, int transOpcode,
		const TranslationTableRule **transRule, int *cursorPosition, int *cursorStatus,
		int *pre_src, int compbrlStart, int compbrlEnd) {
	int at;

	for (at = *pre_src; at <= *pos; at++)
		insertEmphasesAt(at, table, pos, mode, input, output, posMapping, emphasisBuffer,
				transNoteBuffer, haveEmphasis, transOpcode, transRule, cursorPosition,
				cursorStatus, compbrlStart, compbrlEnd);

	*pre_src = *pos + 1;
}

static void
checkNumericMode(const TranslationTableHeader *table, int *pos, int mode, InString input,
		OutString *output, int *posMapping, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, const TranslationTableRule **transRule,
		int *cursorPosition, int *cursorStatus, int *dontContract, int compbrlStart,
		int compbrlEnd, int *numericMode) {
	int i;
	const TranslationTableRule *indicRule;
	if (!brailleIndicatorDefined(table->numberSign, table, &indicRule)) return;

	/* not in numeric mode */
	if (!*numericMode) {
		if (checkAttr(input.chars[*pos], CTC_Digit | CTC_LitDigit, 0, table)) {
			*numericMode = 1;
			*dontContract = 1;
			for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, 0, table,
					pos, mode, input, output, posMapping, emphasisBuffer, transNoteBuffer,
					transRule, cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
		} else if (checkAttr(input.chars[*pos], CTC_NumericMode, 0, table)) {
			for (i = *pos + 1; i < input.length; i++) {
				if (checkAttr(input.chars[i], CTC_Digit | CTC_LitDigit, 0, table)) {
					*numericMode = 1;
					for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen,
							0, table, pos, mode, input, output, posMapping,
							emphasisBuffer, transNoteBuffer, transRule, cursorPosition,
							cursorStatus, compbrlStart, compbrlEnd);
					break;
				} else if (!checkAttr(input.chars[i], CTC_NumericMode, 0, table))
					break;
			}
		}
	}

	/* in numeric mode */
	else {
		if (!checkAttr(input.chars[*pos], CTC_Digit | CTC_LitDigit | CTC_NumericMode, 0,
					table)) {
			*numericMode = 0;
			if (brailleIndicatorDefined(table->noContractSign, table, &indicRule))
				if (checkAttr(input.chars[*pos], CTC_NumericNoContract, 0, table))
					for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen,
							0, table, pos, mode, input, output, posMapping,
							emphasisBuffer, transNoteBuffer, transRule, cursorPosition,
							cursorStatus, compbrlStart, compbrlEnd);
		}
	}
}

static int
translateString(const TranslationTableHeader *table, int mode, int currentPass,
		InString *input, OutString *output, int *posMapping, formtype *typebuf,
		unsigned char *srcSpacing, unsigned char *destSpacing, unsigned int *wordBuffer,
		unsigned int *emphasisBuffer, unsigned int *transNoteBuffer, int haveEmphasis,
		int *realInlen, int *posIncremented, int *cursorPosition, int *cursorStatus,
		int compbrlStart, int compbrlEnd) {
	int pos;
	int transOpcode;
	int prevTransOpcode;
	const TranslationTableRule *transRule;
	int transCharslen;
	int passCharDots;
	const widechar *passInstructions;
	int passIC; /* Instruction counter */
	int startMatch;
	int startReplace;
	int endReplace;
	int endMatch;
	TranslationTableRule *groupingRule;
	widechar groupingOp;
	int numericMode;
	int dontContract;
	int destword;
	int srcword;
	int pre_src;
	TranslationTableCharacter *curCharDef;
	const widechar *repwordStart;
	int repwordLength;
	int curType;
	int prevType;
	int prevTypeform;
	int prevPos;
	/* Main translation routine */
	int k;
	translation_direction = 1;
	markSyllables(table, *input, typebuf, &transOpcode, &transRule, &transCharslen);
	numericMode = 0;
	srcword = 0;
	destword = 0; /* last word translated */
	dontContract = 0;
	prevTransOpcode = CTO_None;
	prevType = curType = prevTypeform = plain_text;
	prevPos = -1;
	pos = output->length = 0;
	*posIncremented = 1;
	pre_src = 0;
	_lou_resetPassVariables();
	if (typebuf && table->emphRules[capsRule][letterOffset])
		for (k = 0; k < input->length; k++)
			if (checkAttr(input->chars[k], CTC_UpperCase, 0, table))
				typebuf[k] |= CAPSEMPH;

	markEmphases(table, *input, typebuf, wordBuffer, emphasisBuffer, transNoteBuffer,
			haveEmphasis, cursorPosition, cursorStatus, compbrlStart, compbrlEnd);

	while (pos < input->length) { /* the main translation loop */
		TranslationTableCharacterAttributes beforeAttributes;
		setBefore(table, pos, *input, &beforeAttributes);
		if (!insertBrailleIndicators(0, table, &pos, mode, *input, output, posMapping,
					typebuf, haveEmphasis, emphasisBuffer, transNoteBuffer, transOpcode,
					prevTransOpcode, &transRule, cursorPosition, cursorStatus,
					compbrlStart, compbrlEnd, beforeAttributes, &prevType, &curType,
					&prevTypeform, prevPos))
			goto failure;
		if (pos >= input->length) break;

		// insertEmphases();
		if (!dontContract) dontContract = typebuf[pos] & no_contract;
		if (typebuf[pos] & no_translate) {
			widechar c = _lou_getDotsForChar(input->chars[pos]);
			if (input->chars[pos] < 32 || input->chars[pos] > 126) goto failure;
			if (!for_updatePositions(&c, 1, 1, 0, table, &pos, mode, *input, output,
						posMapping, emphasisBuffer, transNoteBuffer, &transRule,
						cursorPosition, cursorStatus, compbrlStart, compbrlEnd))
				goto failure;
			pos++;
			/* because we don't call insertEmphasis */
			pre_src = pos;
			continue;
		}
		for_selectRule(table, pos, *output, mode, *input, typebuf, emphasisBuffer,
				transNoteBuffer, &transOpcode, prevTransOpcode, &transRule,
				&transCharslen, &passCharDots, &passInstructions, &passIC, &startMatch,
				&startReplace, &endReplace, &endMatch, *posIncremented, *cursorPosition,
				&repwordStart, &repwordLength, dontContract, compbrlStart, compbrlEnd,
				beforeAttributes, &curCharDef, &groupingRule, &groupingOp);

		if (transOpcode != CTO_Context)
			if (appliedRules != NULL && appliedRulesCount < maxAppliedRules)
				appliedRules[appliedRulesCount++] = transRule;
		*posIncremented = 1;
		prevPos = pos;
		switch (transOpcode) /* Rules that pre-empt context and swap */
		{
		case CTO_CompBrl:
		case CTO_Literal:
			if (!doCompbrl(table, &pos, mode, *input, output, posMapping, emphasisBuffer,
						transNoteBuffer, &transRule, cursorPosition, cursorStatus,
						destword, srcword, compbrlStart, compbrlEnd))
				goto failure;
			continue;
		default:
			break;
		}
		if (!insertBrailleIndicators(1, table, &pos, mode, *input, output, posMapping,
					typebuf, haveEmphasis, emphasisBuffer, transNoteBuffer, transOpcode,
					prevTransOpcode, &transRule, cursorPosition, cursorStatus,
					compbrlStart, compbrlEnd, beforeAttributes, &prevType, &curType,
					&prevTypeform, prevPos))
			goto failure;

		//		if(transOpcode == CTO_Contraction)
		//		if(brailleIndicatorDefined(table->noContractSign))
		//		if(!for_updatePositions(
		//			&indicRule->charsdots[0], 0, indicRule->dotslen, 0))
		//			goto failure;
		insertEmphases(table, &pos, mode, *input, output, posMapping, emphasisBuffer,
				transNoteBuffer, haveEmphasis, transOpcode, &transRule, cursorPosition,
				cursorStatus, &pre_src, compbrlStart, compbrlEnd);
		if (table->usesNumericMode)
			checkNumericMode(table, &pos, mode, *input, output, posMapping,
					emphasisBuffer, transNoteBuffer, &transRule, cursorPosition,
					cursorStatus, &dontContract, compbrlStart, compbrlEnd, &numericMode);

		if (transOpcode == CTO_Context ||
				findForPassRule(table, pos, currentPass, *input, &transOpcode, &transRule,
						&transCharslen, &passCharDots, &passInstructions, &passIC,
						&startMatch, &startReplace, &endReplace, &endMatch, &groupingRule,
						&groupingOp))
			switch (transOpcode) {
			case CTO_Context:
				if (appliedRules != NULL && appliedRulesCount < maxAppliedRules)
					appliedRules[appliedRulesCount++] = transRule;
				if (!passDoAction(table, &pos, mode, input, output, posMapping,
							emphasisBuffer, transNoteBuffer, transOpcode, &transRule,
							passCharDots, passInstructions, &passIC, startMatch,
							startReplace, &endReplace, endMatch, cursorPosition,
							cursorStatus, compbrlStart, compbrlEnd, groupingRule,
							groupingOp))
					goto failure;
				if (endReplace == pos) *posIncremented = 0;
				pos = endReplace;
				continue;
			default:
				break;
			}

		/* Processing before replacement */

		/* check if leaving no contraction (grade 1) mode */
		if (checkAttr(input->chars[pos], CTC_SeqDelimiter | CTC_Space, 0, table))
			dontContract = 0;

		switch (transOpcode) {
		case CTO_EndNum:
			if (table->letterSign && checkAttr(input->chars[pos], CTC_Letter, 0, table))
				output->length--;
			break;
		case CTO_Repeated:
		case CTO_Space:
			dontContract = 0;
			break;
		case CTO_LargeSign:
			if (prevTransOpcode == CTO_LargeSign) {
				int hasEndSegment = 0;
				while (output->length > 0 && checkAttr(output->chars[output->length - 1],
													 CTC_Space, 1, table)) {
					if (output->chars[output->length - 1] == ENDSEGMENT) {
						hasEndSegment = 1;
					}
					output->length--;
				}
				if (hasEndSegment != 0) {
					output->chars[output->length] = 0xffff;
					output->length++;
				}
			}
			break;
		case CTO_DecPoint:
			if (!table->usesNumericMode && table->numberSign) {
				TranslationTableRule *numRule =
						(TranslationTableRule *)&table->ruleArea[table->numberSign];
				if (!for_updatePositions(&numRule->charsdots[numRule->charslen],
							numRule->charslen, numRule->dotslen, 0, table, &pos, mode,
							*input, output, posMapping, emphasisBuffer, transNoteBuffer,
							&transRule, cursorPosition, cursorStatus, compbrlStart,
							compbrlEnd))
					goto failure;
			}
			transOpcode = CTO_MidNum;
			break;
		case CTO_NoCont:
			if (!dontContract)
				doNocont(table, &pos, output, mode, *input, destword, srcword,
						&dontContract);
			continue;
		default:
			break;
		} /* end of action */

		/* replacement processing */
		switch (transOpcode) {
		case CTO_Replace:
			pos += transCharslen;
			if (!putCharacters(&transRule->charsdots[transCharslen], transRule->dotslen,
						table, &pos, mode, *input, output, posMapping, emphasisBuffer,
						transNoteBuffer, &transRule, cursorPosition, cursorStatus,
						compbrlStart, compbrlEnd))
				goto failure;
			break;
		case CTO_None:
			if (!undefinedCharacter(input->chars[pos], table, &pos, mode, *input, output,
						posMapping, emphasisBuffer, transNoteBuffer, &transRule,
						cursorPosition, cursorStatus, compbrlStart, compbrlEnd))
				goto failure;
			pos++;
			break;
		case CTO_UpperCase:
			/* Only needs special handling if not within compbrl and
			 * the table defines a capital sign. */
			if (!(mode & (compbrlAtCursor | compbrlLeftCursor) && pos >= compbrlStart &&
						pos <= compbrlEnd) &&
					(transRule->dotslen == 1 &&
							table->emphRules[capsRule][letterOffset])) {
				putCharacter(curCharDef->lowercase, table, &pos, mode, *input, output,
						posMapping, emphasisBuffer, transNoteBuffer, &transRule,
						cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
				pos++;
				break;
			}
		//		case CTO_Contraction:
		//
		//			if(brailleIndicatorDefined(table->noContractSign))
		//			if(!for_updatePositions(
		//				&indicRule->charsdots[0], 0, indicRule->dotslen, 0))
		//				goto failure;

		default:
			if (*cursorStatus == 2)
				*cursorStatus = 1;
			else {
				if (transRule->dotslen) {
					if (!for_updatePositions(&transRule->charsdots[transCharslen],
								transCharslen, transRule->dotslen, 0, table, &pos, mode,
								*input, output, posMapping, emphasisBuffer,
								transNoteBuffer, &transRule, cursorPosition, cursorStatus,
								compbrlStart, compbrlEnd))
						goto failure;
				} else {
					for (k = 0; k < transCharslen; k++) {
						if (!putCharacter(input->chars[pos], table, &pos, mode, *input,
									output, posMapping, emphasisBuffer, transNoteBuffer,
									&transRule, cursorPosition, cursorStatus,
									compbrlStart, compbrlEnd))
							goto failure;
						pos++;
					}
				}
				if (*cursorStatus == 2)
					*cursorStatus = 1;
				else if (transRule->dotslen)
					pos += transCharslen;
			}
			break;
		}

		/* processing after replacement */
		switch (transOpcode) {
		case CTO_Repeated: {
			/* Skip repeated characters. */
			int srclim = input->length - transCharslen;
			if (mode & (compbrlAtCursor | compbrlLeftCursor) && compbrlStart < srclim)
				/* Don't skip characters from compbrlStart onwards. */
				srclim = compbrlStart - 1;
			while ((pos <= srclim) &&
					compareChars(&transRule->charsdots[0], &input->chars[pos],
							transCharslen, 0, table)) {
				/* Map skipped input positions to the previous output position. */
				/* if (posMapping.outputPositions != NULL) { */
				/* 	int tcc; */
				/* 	for (tcc = 0; tcc < transCharslen; tcc++) */
				/* 		posMapping.outputPositions[posMapping.prev[pos + tcc]] = */
				/* 				output->length - 1; */
				/* } */
				if (!*cursorStatus && pos <= *cursorPosition &&
						*cursorPosition < pos + transCharslen) {
					*cursorStatus = 1;
					*cursorPosition = output->length - 1;
				}
				pos += transCharslen;
			}
			break;
		}
		case CTO_RepWord: {
			/* Skip repeated characters. */
			int srclim = input->length - transCharslen;
			if (mode & (compbrlAtCursor | compbrlLeftCursor) && compbrlStart < srclim)
				/* Don't skip characters from compbrlStart onwards. */
				srclim = compbrlStart - 1;
			while ((pos <= srclim) && compareChars(repwordStart, &input->chars[pos],
											  repwordLength, 0, table)) {
				/* Map skipped input positions to the previous output position. */
				/* if (posMapping.outputPositions != NULL) { */
				/* 	int tcc; */
				/* 	for (tcc = 0; tcc < transCharslen; tcc++) */
				/* 		posMapping.outputPositions[posMapping.prev[pos + tcc]] = */
				/* 				output->length - 1; */
				/* } */
				if (!*cursorStatus && pos <= *cursorPosition &&
						*cursorPosition < pos + transCharslen) {
					*cursorStatus = 1;
					*cursorPosition = output->length - 1;
				}
				pos += repwordLength + transCharslen;
			}
			pos -= transCharslen;
			break;
		}
		case CTO_JoinNum:
		case CTO_JoinableWord:
			while (pos < input->length &&
					checkAttr(input->chars[pos], CTC_Space, 0, table) &&
					input->chars[pos] != ENDSEGMENT)
				pos++;
			break;
		default:
			break;
		}
		if (((pos > 0) && checkAttr(input->chars[pos - 1], CTC_Space, 0, table) &&
					(transOpcode != CTO_JoinableWord))) {
			srcword = pos;
			destword = output->length;
		}
		if (srcSpacing != NULL && srcSpacing[pos] >= '0' && srcSpacing[pos] <= '9')
			destSpacing[output->length] = srcSpacing[pos];
		if ((transOpcode >= CTO_Always && transOpcode <= CTO_None) ||
				(transOpcode >= CTO_Digit && transOpcode <= CTO_LitDigit))
			prevTransOpcode = transOpcode;
	}

	transOpcode = CTO_Space;
	insertEmphases(table, &pos, mode, *input, output, posMapping, emphasisBuffer,
			transNoteBuffer, haveEmphasis, transOpcode, &transRule, cursorPosition,
			cursorStatus, &pre_src, compbrlStart, compbrlEnd);

failure:
	if (destword != 0 && pos < input->length &&
			!checkAttr(input->chars[pos], CTC_Space, 0, table)) {
		pos = srcword;
		output->length = destword;
	}
	if (pos < input->length) {
		while (checkAttr(input->chars[pos], CTC_Space, 0, table))
			if (++pos == input->length) break;
	}
	if (realInlen) *realInlen = pos;
	return 1;
} /* first pass translation completed */

int EXPORT_CALL
lou_hyphenate(const char *tableList, const widechar *inbuf, int inlen, char *hyphens,
		int mode) {
#define HYPHSTRING 100
	const TranslationTableHeader *table;
	widechar workingBuffer[HYPHSTRING];
	int k, kk;
	int wordStart;
	int wordEnd;
	table = lou_getTable(tableList);
	if (table == NULL || inbuf == NULL || hyphens == NULL ||
			table->hyphenStatesArray == 0 || inlen >= HYPHSTRING)
		return 0;
	if (mode != 0) {
		k = inlen;
		kk = HYPHSTRING;
		if (!lou_backTranslate(tableList, inbuf, &k, &workingBuffer[0], &kk, NULL, NULL,
					NULL, NULL, NULL, 0))
			return 0;
	} else {
		memcpy(&workingBuffer[0], inbuf, CHARSIZE * inlen);
		kk = inlen;
	}
	for (wordStart = 0; wordStart < kk; wordStart++)
		if (((findCharOrDots(workingBuffer[wordStart], 0, table))->attributes &
					CTC_Letter))
			break;
	if (wordStart == kk) return 0;
	for (wordEnd = kk - 1; wordEnd >= 0; wordEnd--)
		if (((findCharOrDots(workingBuffer[wordEnd], 0, table))->attributes & CTC_Letter))
			break;
	for (k = wordStart; k <= wordEnd; k++) {
		TranslationTableCharacter *c = findCharOrDots(workingBuffer[k], 0, table);
		if (!(c->attributes & CTC_Letter)) return 0;
	}
	if (!hyphenate(&workingBuffer[wordStart], wordEnd - wordStart + 1,
				&hyphens[wordStart], table))
		return 0;
	for (k = 0; k <= wordStart; k++) hyphens[k] = '0';
	if (mode != 0) {
		widechar workingBuffer2[HYPHSTRING];
		int outputPos[HYPHSTRING];
		char hyphens2[HYPHSTRING];
		kk = wordEnd - wordStart + 1;
		k = HYPHSTRING;
		if (!lou_translate(tableList, &workingBuffer[wordStart], &kk, &workingBuffer2[0],
					&k, NULL, NULL, &outputPos[0], NULL, NULL, 0))
			return 0;
		for (kk = 0; kk < k; kk++) {
			int hyphPos = outputPos[kk];
			if (hyphPos > k || hyphPos < 0) break;
			if (hyphens[wordStart + kk] & 1)
				hyphens2[hyphPos] = '1';
			else
				hyphens2[hyphPos] = '0';
		}
		for (kk = wordStart; kk < wordStart + k; kk++)
			if (hyphens2[kk] == '0') hyphens[kk] = hyphens2[kk];
	}
	for (k = 0; k < inlen; k++)
		if (hyphens[k] & 1)
			hyphens[k] = '1';
		else
			hyphens[k] = '0';
	hyphens[inlen] = 0;
	return 1;
}

int EXPORT_CALL
lou_dotsToChar(
		const char *tableList, widechar *inbuf, widechar *outbuf, int length, int mode) {
	const TranslationTableHeader *table;
	int k;
	widechar dots;
	if (tableList == NULL || inbuf == NULL || outbuf == NULL) return 0;

	table = lou_getTable(tableList);
	if (table == NULL || length <= 0) return 0;
	for (k = 0; k < length; k++) {
		dots = inbuf[k];
		if (!(dots & B16) && (dots & 0xff00) == 0x2800) /* Unicode braille */
			dots = (dots & 0x00ff) | B16;
		outbuf[k] = _lou_getCharFromDots(dots);
	}
	return 1;
}

int EXPORT_CALL
lou_charToDots(const char *tableList, const widechar *inbuf, widechar *outbuf, int length,
		int mode) {
	const TranslationTableHeader *table;
	int k;
	if (tableList == NULL || inbuf == NULL || outbuf == NULL) return 0;

	table = lou_getTable(tableList);
	if (table == NULL || length <= 0) return 0;
	for (k = 0; k < length; k++)
		if ((mode & ucBrl))
			outbuf[k] = ((_lou_getDotsForChar(inbuf[k]) & 0xff) | 0x2800);
		else
			outbuf[k] = _lou_getDotsForChar(inbuf[k]);
	return 1;
}
