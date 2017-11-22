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

static int
putCharacter(widechar c, const TranslationTableHeader *table, int *src, int *dest,
		int srcmax, int destmax, int mode, const widechar *currentInput,
		widechar *currentOutput, int *srcMapping, int *prevSrcMapping,
		unsigned int *emphasisBuffer, unsigned int *transNoteBuffer,
		const TranslationTableRule **transRule, int *inputPositions, int *outputPositions,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd);
static int
passDoTest(const TranslationTableHeader *table, int src, int srcmax,
		const widechar *currentInput, int transOpcode,
		const TranslationTableRule *transRule, int *passCharDots, int *passSrc,
		const widechar **passInstructions, int *passIC, int *startMatch,
		int *startReplace, int *endReplace, TranslationTableRule **groupingRule,
		widechar *groupingOp);
static int
passDoAction(const TranslationTableHeader *table, int *src, int *dest, int *srcmax,
		int destmax, int mode, const widechar *currentInput, widechar *currentOutput,
		int *srcMapping, int *prevSrcMapping, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, int transOpcode,
		const TranslationTableRule **transRule, int passCharDots, int passSrc,
		const widechar *passInstructions, int startMatch, int startReplace, int *passIC,
		int *endReplace, int *inputPositions, int *outputPositions, int *cursorPosition,
		int *cursorStatus, int compbrlStart, int compbrlEnd,
		TranslationTableRule *groupingRule, widechar groupingOp);

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
checkAttr_safe(const widechar *currentInput, int src,
		const TranslationTableCharacterAttributes a, int m,
		const TranslationTableHeader *table, int srcmax) {
	return ((src < srcmax) ? checkAttr(currentInput[src], a, m, table) : 0);
}

static int
findForPassRule(const TranslationTableHeader *table, int src, int srcmax, int currentPass,
		const widechar *currentInput, int *transOpcode,
		const TranslationTableRule **transRule, int *transCharslen, int *passCharDots,
		int *passSrc, widechar const **passInstructions, int *passIC, int *startMatch,
		int *startReplace, int *endReplace, TranslationTableRule **groupingRule,
		widechar *groupingOp) {
	int save_transCharslen = *transCharslen;
	const TranslationTableRule *save_transRule = *transRule;
	TranslationTableOpcode save_transOpcode = *transOpcode;
	TranslationTableOffset ruleOffset;
	ruleOffset = table->forPassRules[currentPass];
	*transCharslen = 0;
	while (ruleOffset) {
		*transRule = (TranslationTableRule *)&table->ruleArea[ruleOffset];
		*transOpcode = (*transRule)->opcode;
		if (passDoTest(table, src, srcmax, currentInput, *transOpcode, *transRule,
					passCharDots, passSrc, passInstructions, passIC, startMatch,
					startReplace, endReplace, groupingRule, groupingOp))
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
makeCorrections(const TranslationTableHeader *table, int *dest, int *srcmax, int destmax,
		int mode, const widechar *currentInput, widechar *currentOutput, int *srcMapping,
		int *prevSrcMapping, formtype *typebuf, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, int *realInlen, int *srcIncremented,
		int *inputPositions, int *outputPositions, int *cursorPosition, int *cursorStatus,
		int compbrlStart, int compbrlEnd) {
	int src;
	int transOpcode;
	const TranslationTableRule *transRule;
	int transCharslen;
	int passCharDots;
	int passSrc;
	const widechar *passInstructions;
	int passIC; /* Instruction counter */
	int startMatch;
	int startReplace;
	int endReplace;
	TranslationTableRule *groupingRule;
	widechar groupingOp;
	if (!table->corrections) return 1;
	src = 0;
	*dest = 0;
	*srcIncremented = 1;
	_lou_resetPassVariables();
	while (src < *srcmax) {
		int length = *srcmax - src;
		const TranslationTableCharacter *character =
				findCharOrDots(currentInput[src], 0, table);
		const TranslationTableCharacter *character2;
		int tryThis = 0;
		if (!findForPassRule(table, src, *srcmax, 0, currentInput, &transOpcode,
					&transRule, &transCharslen, &passCharDots, &passSrc,
					&passInstructions, &passIC, &startMatch, &startReplace, &endReplace,
					&groupingRule, &groupingOp))
			while (tryThis < 3) {
				TranslationTableOffset ruleOffset = 0;
				unsigned long int makeHash = 0;
				switch (tryThis) {
				case 0:
					if (!(length >= 2)) break;
					makeHash = (unsigned long int)character->lowercase << 8;
					character2 = findCharOrDots(currentInput[src + 1], 0, table);
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
														&currentInput[src], transCharslen,
														0, table))) {
						if (*srcIncremented && transOpcode == CTO_Correct &&
								passDoTest(table, src, *srcmax, currentInput, transOpcode,
										transRule, &passCharDots, &passSrc,
										&passInstructions, &passIC, &startMatch,
										&startReplace, &endReplace, &groupingRule,
										&groupingOp)) {
							tryThis = 4;
							break;
						}
					}
					ruleOffset = transRule->charsnext;
				}
				tryThis++;
			}
		*srcIncremented = 1;

		switch (transOpcode) {
		case CTO_Always:
			if (*dest >= destmax) goto failure;
			srcMapping[*dest] = prevSrcMapping[src];
			currentOutput[(*dest)++] = currentInput[src++];
			break;
		case CTO_Correct:
			if (appliedRules != NULL && appliedRulesCount < maxAppliedRules)
				appliedRules[appliedRulesCount++] = transRule;
			if (!passDoAction(table, &src, dest, srcmax, destmax, mode, currentInput,
						currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
						transNoteBuffer, transOpcode, &transRule, passCharDots, passSrc,
						passInstructions, startMatch, startReplace, &passIC, &endReplace,
						inputPositions, outputPositions, cursorPosition, cursorStatus,
						compbrlStart, compbrlEnd, groupingRule, groupingOp))
				goto failure;
			if (endReplace == src) *srcIncremented = 0;
			src = endReplace;
			break;
		default:
			break;
		}
	}

	{  // We have to transform typebuf accordingly
		int pos;
		formtype *typebuf_temp;
		if ((typebuf_temp = malloc(*dest * sizeof(formtype))) == NULL) _lou_outOfMemory();
		for (pos = 0; pos < *dest; pos++) typebuf_temp[pos] = typebuf[srcMapping[pos]];
		memcpy(typebuf, typebuf_temp, *dest * sizeof(formtype));
		free(typebuf_temp);
	}

failure:
	*realInlen = src;
	return 1;
}

static int
matchCurrentInput(const widechar *currentInput, int passSrc,
		const widechar *passInstructions, int passIC) {
	int k;
	int kk = passSrc;
	for (k = passIC + 2; k < passIC + 2 + passInstructions[passIC + 1]; k++)
		if (currentInput[kk] == ENDSEGMENT || passInstructions[k] != currentInput[kk++])
			return 0;
	return 1;
}

static int
swapTest(int swapIC, int *callSrc, const TranslationTableHeader *table,
		const widechar *currentInput, const widechar *passInstructions) {
	int curLen;
	int curTest;
	int curSrc = *callSrc;
	TranslationTableOffset swapRuleOffset;
	TranslationTableRule *swapRule;
	swapRuleOffset = (passInstructions[swapIC + 1] << 16) | passInstructions[swapIC + 2];
	swapRule = (TranslationTableRule *)&table->ruleArea[swapRuleOffset];
	for (curLen = 0; curLen < passInstructions[swapIC + 3]; curLen++) {
		if (swapRule->opcode == CTO_SwapDd) {
			for (curTest = 1; curTest < swapRule->charslen; curTest += 2) {
				if (currentInput[curSrc] == swapRule->charsdots[curTest]) break;
			}
		} else {
			for (curTest = 0; curTest < swapRule->charslen; curTest++) {
				if (currentInput[curSrc] == swapRule->charsdots[curTest]) break;
			}
		}
		if (curTest >= swapRule->charslen) return 0;
		curSrc++;
	}
	if (passInstructions[swapIC + 3] == passInstructions[swapIC + 4]) {
		*callSrc = curSrc;
		return 1;
	}
	while (curLen < passInstructions[swapIC + 4]) {
		if (swapRule->opcode == CTO_SwapDd) {
			for (curTest = 1; curTest < swapRule->charslen; curTest += 2) {
				if (currentInput[curSrc] == swapRule->charsdots[curTest]) break;
			}
		} else {
			for (curTest = 0; curTest < swapRule->charslen; curTest++) {
				if (currentInput[curSrc] == swapRule->charsdots[curTest]) break;
			}
		}
		if (curTest >= swapRule->charslen) {
			*callSrc = curSrc;
			return 1;
		}
		curSrc++;
		curLen++;
	}
	*callSrc = curSrc;
	return 1;
}

static int
swapReplace(int start, int end, const TranslationTableHeader *table, int *dest,
		int destmax, const widechar *currentInput, widechar *currentOutput,
		int *srcMapping, int *prevSrcMapping, const widechar *passInstructions,
		int passIC) {
	TranslationTableOffset swapRuleOffset;
	TranslationTableRule *swapRule;
	widechar *replacements;
	int curRep;
	int curPos;
	int curTest;
	int curSrc;
	swapRuleOffset = (passInstructions[passIC + 1] << 16) | passInstructions[passIC + 2];
	swapRule = (TranslationTableRule *)&table->ruleArea[swapRuleOffset];
	replacements = &swapRule->charsdots[swapRule->charslen];
	for (curSrc = start; curSrc < end; curSrc++) {
		for (curTest = 0; curTest < swapRule->charslen; curTest++)
			if (currentInput[curSrc] == swapRule->charsdots[curTest]) break;
		if (curTest == swapRule->charslen) continue;
		curPos = 0;
		for (curRep = 0; curRep < curTest; curRep++)
			if (swapRule->opcode == CTO_SwapCc)
				curPos++;
			else
				curPos += replacements[curPos];
		if (swapRule->opcode == CTO_SwapCc) {
			if ((*dest + 1) > destmax) return 0;
			srcMapping[*dest] = prevSrcMapping[curSrc];
			currentOutput[(*dest)++] = replacements[curPos];
		} else {
			int l = replacements[curPos] - 1;
			int d = *dest + l;
			if (d > destmax) return 0;
			while (--d >= *dest) srcMapping[d] = prevSrcMapping[curSrc];
			memcpy(&currentOutput[*dest], &replacements[curPos + 1],
					l * sizeof(*currentOutput));
			*dest += l;
		}
	}
	return 1;
}

static int
replaceGrouping(const TranslationTableHeader *table, int *dest, int srcmax,
		const widechar *currentInput, widechar *currentOutput, int *srcMapping,
		int *prevSrcMapping, int transOpcode, int passCharDots,
		const widechar *passInstructions, int passIC, int startReplace,
		TranslationTableRule *groupingRule, widechar groupingOp) {
	widechar startCharDots = groupingRule->charsdots[2 * passCharDots];
	widechar endCharDots = groupingRule->charsdots[2 * passCharDots + 1];
	widechar *curin = (widechar *)currentInput;
	int curPos;
	int level = 0;
	TranslationTableOffset replaceOffset =
			passInstructions[passIC + 1] << 16 | (passInstructions[passIC + 2] & 0xff);
	TranslationTableRule *replaceRule =
			(TranslationTableRule *)&table->ruleArea[replaceOffset];
	widechar replaceStart = replaceRule->charsdots[2 * passCharDots];
	widechar replaceEnd = replaceRule->charsdots[2 * passCharDots + 1];
	if (groupingOp == pass_groupstart) {
		curin[startReplace] = replaceStart;
		for (curPos = startReplace + 1; curPos < srcmax; curPos++) {
			if (currentInput[curPos] == startCharDots) level--;
			if (currentInput[curPos] == endCharDots) level++;
			if (level == 1) break;
		}
		if (curPos == srcmax) return 0;
		curin[curPos] = replaceEnd;
	} else {
		if (transOpcode == CTO_Context) {
			startCharDots = groupingRule->charsdots[2];
			endCharDots = groupingRule->charsdots[3];
			replaceStart = replaceRule->charsdots[2];
			replaceEnd = replaceRule->charsdots[3];
		}
		currentOutput[*dest] = replaceEnd;
		for (curPos = *dest - 1; curPos >= 0; curPos--) {
			if (currentOutput[curPos] == endCharDots) level--;
			if (currentOutput[curPos] == startCharDots) level++;
			if (level == 1) break;
		}
		if (curPos < 0) return 0;
		currentOutput[curPos] = replaceStart;
		(*dest)++;
	}
	return 1;
}

static int
removeGrouping(int *dest, int *srcmax, const widechar *currentInput,
		widechar *currentOutput, int *srcMapping, int *prevSrcMapping, int passCharDots,
		int startReplace, int endReplace, TranslationTableRule *groupingRule,
		widechar groupingOp) {
	widechar startCharDots = groupingRule->charsdots[2 * passCharDots];
	widechar endCharDots = groupingRule->charsdots[2 * passCharDots + 1];
	widechar *curin = (widechar *)currentInput;
	int curPos;
	int level = 0;
	if (groupingOp == pass_groupstart) {
		for (curPos = startReplace + 1; curPos < *srcmax; curPos++) {
			if (currentInput[curPos] == startCharDots) level--;
			if (currentInput[curPos] == endCharDots) level++;
			if (level == 1) break;
		}
		if (curPos == *srcmax) return 0;
		curPos++;
		for (; curPos < *srcmax; curPos++) curin[curPos - 1] = curin[curPos];
		(*srcmax)--;
	} else {
		for (curPos = *dest - 1; curPos >= 0; curPos--) {
			if (currentOutput[curPos] == endCharDots) level--;
			if (currentOutput[curPos] == startCharDots) level++;
			if (level == 1) break;
		}
		if (curPos < 0) return 0;
		curPos++;
		for (; curPos < *dest; curPos++)
			currentOutput[curPos - 1] = currentOutput[curPos];
		(*dest)--;
	}
	return 1;
}

static int
doPassSearch(const TranslationTableHeader *table, int srcmax,
		const widechar *currentInput, const TranslationTableRule *transRule,
		int passCharDots, int passSrc, const widechar *passInstructions, int passIC,
		int *searchIC, int *searchSrc, TranslationTableRule *groupingRule,
		widechar groupingOp) {
	int level = 0;
	int k, kk;
	int not = 0;
	TranslationTableOffset ruleOffset;
	TranslationTableRule *rule;
	TranslationTableCharacterAttributes attributes;
	int stepper = passSrc;
	while (stepper < srcmax) {
		*searchIC = passIC + 1;
		*searchSrc = stepper;
		while (*searchIC < transRule->dotslen) {
			int itsTrue = 1;
			if (*searchSrc > srcmax) return 0;
			switch (passInstructions[*searchIC]) {
			case pass_lookback:
				*searchSrc -= passInstructions[*searchIC + 1];
				if (*searchSrc < 0) {
					*searchSrc = 0;
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
				kk = *searchSrc;
				for (k = *searchIC + 2;
						k < *searchIC + 2 + passInstructions[*searchIC + 1]; k++)
					if (currentInput[kk] == ENDSEGMENT ||
							passInstructions[k] != currentInput[kk++]) {
						itsTrue = 0;
						break;
					}
				*searchSrc += passInstructions[*searchIC + 1];
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
					if (currentInput[*searchSrc] == ENDSEGMENT)
						itsTrue = 0;
					else
						itsTrue = ((findCharOrDots(currentInput[(*searchSrc)++],
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
						if (currentInput[*searchSrc] == ENDSEGMENT) {
							itsTrue = 0;
							break;
						}
						if (!(findCharOrDots(currentInput[*searchSrc], passCharDots,
									  table)->attributes &
									attributes))
							break;
						(*searchSrc)++;
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
					itsTrue = (currentInput[*searchSrc] ==
									  rule->charsdots[2 * passCharDots])
							? 1
							: 0;
				else
					itsTrue = (currentInput[*searchSrc] ==
									  rule->charsdots[2 * passCharDots + 1])
							? 1
							: 0;
				if (groupingRule != NULL && groupingOp == pass_groupstart &&
						rule == groupingRule) {
					if (currentInput[*searchSrc] == rule->charsdots[2 * passCharDots])
						level--;
					else if (currentInput[*searchSrc] ==
							rule->charsdots[2 * passCharDots + 1])
						level++;
				}
				(*searchSrc)++;
				*searchIC += 3;
				break;
			case pass_swap:
				itsTrue = swapTest(
						*searchIC, searchSrc, table, currentInput, passInstructions);
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
		stepper++;
	}
	return 0;
}

static int
passDoTest(const TranslationTableHeader *table, int src, int srcmax,
		const widechar *currentInput, int transOpcode,
		const TranslationTableRule *transRule, int *passCharDots, int *passSrc,
		widechar const **passInstructions, int *passIC, int *startMatch,
		int *startReplace, int *endReplace, TranslationTableRule **groupingRule,
		widechar *groupingOp) {
	int endMatch;
	int searchIC, searchSrc;
	int k;
	int not = 0;
	TranslationTableOffset ruleOffset = 0;
	TranslationTableRule *rule = NULL;
	TranslationTableCharacterAttributes attributes = 0;
	*groupingRule = NULL;
	*passSrc = src;
	*passInstructions = &transRule->charsdots[transRule->charslen];
	*passIC = 0;
	*startMatch = endMatch = *passSrc;
	*startReplace = *endReplace = -1;
	if (transOpcode == CTO_Context || transOpcode == CTO_Correct)
		*passCharDots = 0;
	else
		*passCharDots = 1;
	while (*passIC < transRule->dotslen) {
		int itsTrue = 1;
		if (*passSrc > srcmax) return 0;
		switch ((*passInstructions)[*passIC]) {
		case pass_first:
			if (*passSrc != 0) itsTrue = 0;
			(*passIC)++;
			break;
		case pass_last:
			if (*passSrc != srcmax) itsTrue = 0;
			(*passIC)++;
			break;
		case pass_lookback:
			*passSrc -= (*passInstructions)[*passIC + 1];
			if (*passSrc < 0) {
				searchSrc = 0;
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
			itsTrue =
					matchCurrentInput(currentInput, *passSrc, *passInstructions, *passIC);
			*passSrc += (*passInstructions)[*passIC + 1];
			*passIC += (*passInstructions)[*passIC + 1] + 2;
			break;
		case pass_startReplace:
			*startReplace = *passSrc;
			(*passIC)++;
			break;
		case pass_endReplace:
			*endReplace = *passSrc;
			(*passIC)++;
			break;
		case pass_attributes:
			attributes = ((*passInstructions)[*passIC + 1] << 16) |
					(*passInstructions)[*passIC + 2];
			for (k = 0; k < (*passInstructions)[*passIC + 3]; k++) {
				if (*passSrc >= srcmax) {
					itsTrue = 0;
					break;
				}
				if (currentInput[*passSrc] == ENDSEGMENT) {
					itsTrue = 0;
					break;
				}
				if (!(findCharOrDots(currentInput[*passSrc], *passCharDots,
							  table)->attributes &
							attributes)) {
					itsTrue = 0;
					break;
				}
				*passSrc += 1;
			}
			if (itsTrue) {
				for (k = (*passInstructions)[*passIC + 3];
						k < (*passInstructions)[*passIC + 4] && *passSrc < srcmax; k++) {
					if (currentInput[*passSrc] == ENDSEGMENT) {
						itsTrue = 0;
						break;
					}
					if (!(findCharOrDots(currentInput[*passSrc], *passCharDots,
								  table)->attributes &
								attributes)) {
						break;
					}
					*passSrc += 1;
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
				itsTrue = (currentInput[*passSrc] == rule->charsdots[2 * *passCharDots])
						? 1
						: 0;
			else
				itsTrue =
						(currentInput[*passSrc] == rule->charsdots[2 * *passCharDots + 1])
						? 1
						: 0;
			(*passSrc)++;
			*passIC += 3;
			break;
		case pass_swap:
			itsTrue = swapTest(*passIC, passSrc, table, currentInput, *passInstructions);
			*passIC += 5;
			break;
		case pass_search:
			itsTrue = doPassSearch(table, srcmax, currentInput, transRule, *passCharDots,
					*passSrc, *passInstructions, *passIC, &searchIC, &searchSrc,
					*groupingRule, *groupingOp);
			if ((!not&&!itsTrue) || (not&&itsTrue)) return 0;
			*passIC = searchIC;
			*passSrc = searchSrc;
		case pass_endTest:
			(*passIC)++;
			endMatch = *passSrc;
			if (*startReplace == -1) {
				*startReplace = *startMatch;
				*endReplace = endMatch;
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
copyCharacters(int from, int to, const TranslationTableHeader *table, int *src, int *dest,
		int srcmax, int destmax, int mode, const widechar *currentInput,
		widechar *currentOutput, int *srcMapping, int *prevSrcMapping,
		unsigned int *emphasisBuffer, unsigned int *transNoteBuffer, int transOpcode,
		const TranslationTableRule **transRule, int *inputPositions, int *outputPositions,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd) {
	if (transOpcode == CTO_Context) {
		while (from < to)
			if (!putCharacter(currentInput[from++], table, src, dest, srcmax, destmax,
						mode, currentInput, currentOutput, srcMapping, prevSrcMapping,
						emphasisBuffer, transNoteBuffer, transRule, inputPositions,
						outputPositions, cursorPosition, cursorStatus, compbrlStart,
						compbrlEnd))
				return 0;
	} else {
		int count = to - from;

		if (count > 0) {
			if ((*dest + count) > destmax) return 0;

			memmove(&srcMapping[*dest], &prevSrcMapping[from],
					count * sizeof(*srcMapping));
			memcpy(&currentOutput[*dest], &currentInput[from],
					count * sizeof(*currentOutput));
			*dest += count;
		}
	}

	return 1;
}

static int
passDoAction(const TranslationTableHeader *table, int *src, int *dest, int *srcmax,
		int destmax, int mode, const widechar *currentInput, widechar *currentOutput,
		int *srcMapping, int *prevSrcMapping, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, int transOpcode,
		const TranslationTableRule **transRule, int passCharDots, int passSrc,
		const widechar *passInstructions, int startMatch, int startReplace, int *passIC,
		int *endReplace, int *inputPositions, int *outputPositions, int *cursorPosition,
		int *cursorStatus, int compbrlStart, int compbrlEnd,
		TranslationTableRule *groupingRule, widechar groupingOp) {
	int k;
	TranslationTableOffset ruleOffset = 0;
	TranslationTableRule *rule = NULL;

	int srcInitial = startMatch;
	int srcStart = startReplace;
	int srcEnd = *endReplace;
	int destInitial = *dest;
	int destStart;

	if (!copyCharacters(srcInitial, srcStart, table, src, dest, *srcmax, destmax, mode,
				currentInput, currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
				transNoteBuffer, transOpcode, transRule, inputPositions, outputPositions,
				cursorPosition, cursorStatus, compbrlStart, compbrlEnd))
		return 0;
	destStart = *dest;

	while (*passIC < (*transRule)->dotslen) switch (passInstructions[*passIC]) {
		case pass_string:
		case pass_dots:
			if ((*dest + passInstructions[*passIC + 1]) > destmax) return 0;
			for (k = 0; k < passInstructions[*passIC + 1]; ++k)
				srcMapping[*dest + k] = prevSrcMapping[startReplace];
			memcpy(&currentOutput[*dest], &passInstructions[*passIC + 2],
					passInstructions[*passIC + 1] * CHARSIZE);
			*dest += passInstructions[*passIC + 1];
			*passIC += passInstructions[*passIC + 1] + 2;
			break;
		case pass_groupstart:
			ruleOffset =
					(passInstructions[*passIC + 1] << 16) | passInstructions[*passIC + 2];
			rule = (TranslationTableRule *)&table->ruleArea[ruleOffset];
			srcMapping[*dest] = prevSrcMapping[startMatch];
			currentOutput[(*dest)++] = rule->charsdots[2 * passCharDots];
			*passIC += 3;
			break;
		case pass_groupend:
			ruleOffset =
					(passInstructions[*passIC + 1] << 16) | passInstructions[*passIC + 2];
			rule = (TranslationTableRule *)&table->ruleArea[ruleOffset];
			srcMapping[*dest] = prevSrcMapping[startMatch];
			currentOutput[(*dest)++] = rule->charsdots[2 * passCharDots + 1];
			*passIC += 3;
			break;
		case pass_swap:
			if (!swapReplace(startReplace, *endReplace, table, dest, destmax,
						currentInput, currentOutput, srcMapping, prevSrcMapping,
						passInstructions, *passIC))
				return 0;
			*passIC += 3;
			break;
		case pass_groupreplace:
			if (!groupingRule ||
					!replaceGrouping(table, dest, *srcmax, currentInput, currentOutput,
							srcMapping, prevSrcMapping, transOpcode, passCharDots,
							passInstructions, *passIC, startReplace, groupingRule,
							groupingOp))
				return 0;
			*passIC += 3;
			break;
		case pass_omit:
			if (groupingRule)
				removeGrouping(dest, srcmax, currentInput, currentOutput, srcMapping,
						prevSrcMapping, passCharDots, startReplace, *endReplace,
						groupingRule, groupingOp);
			(*passIC)++;
			break;
		case pass_copy: {
			int count = destStart - destInitial;

			if (count > 0) {
				memmove(&currentOutput[destInitial], &currentOutput[destStart],
						count * sizeof(*currentOutput));
				*dest -= count;
				destStart = destInitial;
			}
		}

			if (!copyCharacters(srcStart, srcEnd, table, src, dest, *srcmax, destmax,
						mode, currentInput, currentOutput, srcMapping, prevSrcMapping,
						emphasisBuffer, transNoteBuffer, transOpcode, transRule,
						inputPositions, outputPositions, cursorPosition, cursorStatus,
						compbrlStart, compbrlEnd))
				return 0;
			*endReplace = passSrc;
			(*passIC)++;
			break;
		default:
			if (_lou_handlePassVariableAction(passInstructions, passIC)) break;
			return 0;
		}
	return 1;
}

static void
passSelectRule(const TranslationTableHeader *table, int src, int srcmax, int currentPass,
		const widechar *currentInput, int *transOpcode,
		const TranslationTableRule **transRule, int *transCharslen, int *passCharDots,
		int *passSrc, widechar const **passInstructions, int *passIC, int *startMatch,
		int *startReplace, int *endReplace, TranslationTableRule **groupingRule,
		widechar *groupingOp) {
	if (!findForPassRule(table, src, srcmax, currentPass, currentInput, transOpcode,
				transRule, transCharslen, passCharDots, passSrc, passInstructions, passIC,
				startMatch, startReplace, endReplace, groupingRule, groupingOp)) {
		*transOpcode = CTO_Always;
	}
}

static int
translatePass(const TranslationTableHeader *table, int *dest, int *srcmax, int destmax,
		int mode, int currentPass, const widechar *currentInput, widechar *currentOutput,
		int *srcMapping, int *prevSrcMapping, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, int *srcIncremented, int *inputPositions,
		int *outputPositions, int *cursorPosition, int *cursorStatus, int compbrlStart,
		int compbrlEnd) {
	int src;
	int transOpcode;
	int prevTransOpcode;
	const TranslationTableRule *transRule;
	int transCharslen;
	int passCharDots;
	int passSrc;
	const widechar *passInstructions;
	int passIC; /* Instruction counter */
	int startMatch;
	int startReplace;
	int endReplace;
	TranslationTableRule *groupingRule;
	widechar groupingOp;
	prevTransOpcode = CTO_None;
	src = *dest = 0;
	*srcIncremented = 1;
	_lou_resetPassVariables();
	while (src < *srcmax) { /* the main multipass translation loop */
		passSelectRule(table, src, *srcmax, currentPass, currentInput, &transOpcode,
				&transRule, &transCharslen, &passCharDots, &passSrc, &passInstructions,
				&passIC, &startMatch, &startReplace, &endReplace, &groupingRule,
				&groupingOp);
		*srcIncremented = 1;
		switch (transOpcode) {
		case CTO_Context:
		case CTO_Pass2:
		case CTO_Pass3:
		case CTO_Pass4:
			if (appliedRules != NULL && appliedRulesCount < maxAppliedRules)
				appliedRules[appliedRulesCount++] = transRule;
			if (!passDoAction(table, &src, dest, srcmax, destmax, mode, currentInput,
						currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
						transNoteBuffer, transOpcode, &transRule, passCharDots, passSrc,
						passInstructions, startMatch, startReplace, &passIC, &endReplace,
						inputPositions, outputPositions, cursorPosition, cursorStatus,
						compbrlStart, compbrlEnd, groupingRule, groupingOp))
				goto failure;
			if (endReplace == src) *srcIncremented = 0;
			src = endReplace;
			break;
		case CTO_Always:
			if ((*dest + 1) > destmax) goto failure;
			srcMapping[*dest] = prevSrcMapping[src];
			currentOutput[(*dest)++] = currentInput[src++];
			break;
		default:
			goto failure;
		}
	}
	srcMapping[*dest] = prevSrcMapping[src];
failure:
	if (src < *srcmax) {
		while (checkAttr(currentInput[src], CTC_Space, 1, table))
			if (++src == *srcmax) break;
	}
	return 1;
}

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

static int
translateString(const TranslationTableHeader *table, int *dest, int *srcmax, int destmax,
		int mode, int currentPass, const widechar *currentInput, widechar *currentOutput,
		int *srcMapping, int *prevSrcMapping, formtype *typebuf,
		unsigned char *srcSpacing, unsigned char *destSpacing, unsigned int *wordBuffer,
		unsigned int *emphasisBuffer, unsigned int *transNoteBuffer, int haveEmphasis,
		int *realInlen, int *srcIncremented, int *inputPositions, int *outputPositions,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd);

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
	int srcmax;
	int destmax;
	int dest;
	const widechar *currentInput;
	widechar *currentOutput;
	widechar *passbuf1;
	widechar *passbuf2;
	int *srcMapping;
	int *prevSrcMapping;
	formtype *typebuf;
	unsigned char *srcSpacing;
	unsigned char *destSpacing;
	unsigned int *wordBuffer;
	unsigned int *emphasisBuffer;
	unsigned int *transNoteBuffer;
	int *inputPositions;
	int *outputPositions;
	int cursorPosition;
	int cursorStatus;
	int haveEmphasis;
	int compbrlStart;
	int compbrlEnd;
	int k;
	int goodTrans = 1;
	int realInlen;
	int srcIncremented;
	if (tableList == NULL || inbufx == NULL || inlen == NULL || outbuf == NULL ||
			outlen == NULL)
		return 0;
	_lou_logMessage(
			LOG_ALL, "Performing translation: tableList=%s, inlen=%d", tableList, *inlen);
	_lou_logWidecharBuf(LOG_ALL, "Inbuf=", inbufx, *inlen);

	table = lou_getTable(tableList);
	if (table == NULL || *inlen < 0 || *outlen < 0) return 0;
	currentInput = (widechar *)inbufx;
	srcmax = 0;
	while (srcmax < *inlen && currentInput[srcmax]) srcmax++;
	destmax = *outlen;
	haveEmphasis = 0;
	if (!(typebuf = _lou_allocMem(alloc_typebuf, srcmax, destmax))) return 0;
	if (typeform != NULL) {
		for (k = 0; k < srcmax; k++) {
			typebuf[k] = typeform[k];
			if (typebuf[k] & EMPHASIS) haveEmphasis = 1;
		}
	} else
		memset(typebuf, 0, srcmax * sizeof(formtype));

	if ((wordBuffer = _lou_allocMem(alloc_wordBuffer, srcmax, destmax)))
		memset(wordBuffer, 0, (srcmax + 4) * sizeof(unsigned int));
	else
		return 0;
	if ((emphasisBuffer = _lou_allocMem(alloc_emphasisBuffer, srcmax, destmax)))
		memset(emphasisBuffer, 0, (srcmax + 4) * sizeof(unsigned int));
	else
		return 0;
	if ((transNoteBuffer = _lou_allocMem(alloc_transNoteBuffer, srcmax, destmax)))
		memset(transNoteBuffer, 0, (srcmax + 4) * sizeof(unsigned int));
	else
		return 0;

	if (!(spacing == NULL || *spacing == 'X'))
		srcSpacing = (unsigned char *)spacing;
	else
		srcSpacing = NULL;
	outputPositions = outputPos;
	if (outputPos != NULL)
		for (k = 0; k < srcmax; k++) outputPos[k] = -1;
	inputPositions = inputPos;
	if (cursorPos != NULL && *cursorPos >= 0) {
		cursorStatus = 0;
		cursorPosition = *cursorPos;
		if ((mode & (compbrlAtCursor | compbrlLeftCursor))) {
			compbrlStart = cursorPosition;
			if (checkAttr(currentInput[compbrlStart], CTC_Space, 0, table))
				compbrlEnd = compbrlStart + 1;
			else {
				while (compbrlStart >= 0 &&
						!checkAttr(currentInput[compbrlStart], CTC_Space, 0, table))
					compbrlStart--;
				compbrlStart++;
				compbrlEnd = cursorPosition;
				if (!(mode & compbrlLeftCursor))
					while (compbrlEnd < srcmax &&
							!checkAttr(currentInput[compbrlEnd], CTC_Space, 0, table))
						compbrlEnd++;
			}
		}
	} else {
		cursorPosition = -1;
		cursorStatus = 1; /* so it won't check cursor position */
	}
	if (!(passbuf1 = _lou_allocMem(alloc_passbuf1, srcmax, destmax))) return 0;
	if (!(srcMapping = _lou_allocMem(alloc_srcMapping, srcmax, destmax))) return 0;
	if (!(prevSrcMapping = _lou_allocMem(alloc_prevSrcMapping, srcmax, destmax)))
		return 0;
	for (k = 0; k <= srcmax; k++) srcMapping[k] = k;
	srcMapping[srcmax] = srcmax;
	if ((!(mode & pass1Only)) && (table->numPasses > 1 || table->corrections)) {
		if (!(passbuf2 = _lou_allocMem(alloc_passbuf2, srcmax, destmax))) return 0;
	}
	if (srcSpacing != NULL) {
		if (!(destSpacing = _lou_allocMem(alloc_destSpacing, srcmax, destmax)))
			goodTrans = 0;
		else
			memset(destSpacing, '*', destmax);
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
	int currentPass = 0;
	if ((mode & pass1Only)) {
		currentOutput = passbuf1;
		memcpy(prevSrcMapping, srcMapping, destmax * sizeof(int));
		goodTrans = translateString(table, &dest, &srcmax, destmax, mode, 1,
				currentInput, currentOutput, srcMapping, prevSrcMapping, typebuf,
				srcSpacing, destSpacing, wordBuffer, emphasisBuffer, transNoteBuffer,
				haveEmphasis, &realInlen, &srcIncremented, inputPositions,
				outputPositions, &cursorPosition, &cursorStatus, compbrlStart,
				compbrlEnd);
		currentPass = 5; /* Certainly > table->numPasses */
	}
	while (currentPass <= table->numPasses && goodTrans) {
		memcpy(prevSrcMapping, srcMapping, destmax * sizeof(int));
		switch (currentPass) {
		case 0:
			if (table->corrections) {
				currentOutput = passbuf2;
				goodTrans = makeCorrections(table, &dest, &srcmax, destmax, mode,
						currentInput, currentOutput, srcMapping, prevSrcMapping, typebuf,
						emphasisBuffer, transNoteBuffer, &realInlen, &srcIncremented,
						inputPositions, outputPositions, &cursorPosition, &cursorStatus,
						compbrlStart, compbrlEnd);
				currentInput = passbuf2;
				srcmax = dest;
			}
			break;
		case 1: {
			// if table->corrections, realInlen is set by makeCorrections
			int *pRealInlen;
			pRealInlen = table->corrections ? NULL : &realInlen;
			currentOutput = passbuf1;
			goodTrans = translateString(table, &dest, &srcmax, destmax, mode, currentPass,
					currentInput, currentOutput, srcMapping, prevSrcMapping, typebuf,
					srcSpacing, destSpacing, wordBuffer, emphasisBuffer, transNoteBuffer,
					haveEmphasis, pRealInlen, &srcIncremented, inputPositions,
					outputPositions, &cursorPosition, &cursorStatus, compbrlStart,
					compbrlEnd);
			break;
		}
		case 2:
			srcmax = dest;
			currentInput = passbuf1;
			currentOutput = passbuf2;
			goodTrans = translatePass(table, &dest, &srcmax, destmax, mode, currentPass,
					currentInput, currentOutput, srcMapping, prevSrcMapping,
					emphasisBuffer, transNoteBuffer, &srcIncremented, inputPositions,
					outputPositions, &cursorPosition, &cursorStatus, compbrlStart,
					compbrlEnd);
			break;
		case 3:
			srcmax = dest;
			currentInput = passbuf2;
			currentOutput = passbuf1;
			goodTrans = translatePass(table, &dest, &srcmax, destmax, mode, currentPass,
					currentInput, currentOutput, srcMapping, prevSrcMapping,
					emphasisBuffer, transNoteBuffer, &srcIncremented, inputPositions,
					outputPositions, &cursorPosition, &cursorStatus, compbrlStart,
					compbrlEnd);
			break;
		case 4:
			srcmax = dest;
			currentInput = passbuf1;
			currentOutput = passbuf2;
			goodTrans = translatePass(table, &dest, &srcmax, destmax, mode, currentPass,
					currentInput, currentOutput, srcMapping, prevSrcMapping,
					emphasisBuffer, transNoteBuffer, &srcIncremented, inputPositions,
					outputPositions, &cursorPosition, &cursorStatus, compbrlStart,
					compbrlEnd);
			break;
		default:
			break;
		}
		currentPass++;
	}
	if (goodTrans) {
		for (k = 0; k < dest; k++) {
			if (typeform != NULL) {
				if ((currentOutput[k] & (B7 | B8)))
					typeform[k] = '8';
				else
					typeform[k] = '0';
			}
			if ((mode & dotsIO)) {
				if ((mode & ucBrl))
					outbuf[k] = ((currentOutput[k] & 0xff) | 0x2800);
				else
					outbuf[k] = currentOutput[k];
			} else
				outbuf[k] = _lou_getCharFromDots(currentOutput[k]);
		}
		*inlen = realInlen;
		*outlen = dest;
		if (inputPositions != NULL)
			memcpy(inputPositions, srcMapping, dest * sizeof(int));
		if (outputPos != NULL) {
			int lastpos = 0;
			for (k = 0; k < *inlen; k++)
				if (outputPos[k] == -1)
					outputPos[k] = lastpos;
				else
					lastpos = outputPos[k];
		}
	}
	if (destSpacing != NULL) {
		memcpy(srcSpacing, destSpacing, srcmax);
		srcSpacing[srcmax] = 0;
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
doCompTrans(int start, int end, const TranslationTableHeader *table, int *src, int *dest,
		int srcmax, int destmax, int mode, const widechar *currentInput,
		widechar *currentOutput, int *srcMapping, int *prevSrcMapping,
		unsigned int *emphasisBuffer, unsigned int *transNoteBuffer,
		const TranslationTableRule **transRule, int *inputPositions, int *outputPositions,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd);

static int
for_updatePositions(const widechar *outChars, int inLength, int outLength, int shift,
		const TranslationTableHeader *table, int *src, int *dest, int srcmax, int destmax,
		int mode, const widechar *currentInput, widechar *currentOutput, int *srcMapping,
		int *prevSrcMapping, unsigned int *emphasisBuffer, unsigned int *transNoteBuffer,
		const TranslationTableRule **transRule, int *inputPositions, int *outputPositions,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd) {
	int k;
	if ((*dest + outLength) > destmax || (*src + inLength) > srcmax) return 0;
	memcpy(&currentOutput[*dest], outChars, outLength * CHARSIZE);
	if (!*cursorStatus) {
		if ((mode & (compbrlAtCursor | compbrlLeftCursor))) {
			if (*src >= compbrlStart) {
				*cursorStatus = 2;
				return doCompTrans(compbrlStart, compbrlEnd, table, src, dest, srcmax,
						destmax, mode, currentInput, currentOutput, srcMapping,
						prevSrcMapping, emphasisBuffer, transNoteBuffer, transRule,
						inputPositions, outputPositions, cursorPosition, cursorStatus,
						compbrlStart, compbrlEnd);
			}
		} else if (*cursorPosition >= *src && *cursorPosition < (*src + inLength)) {
			*cursorPosition = *dest;
			*cursorStatus = 1;
		} else if (currentInput[*cursorPosition] == 0 &&
				*cursorPosition == (*src + inLength)) {
			*cursorPosition = *dest + outLength / 2 + 1;
			*cursorStatus = 1;
		}
	} else if (*cursorStatus == 2 && *cursorPosition == *src)
		*cursorPosition = *dest;
	if (inputPositions != NULL || outputPositions != NULL) {
		if (outLength <= inLength) {
			for (k = 0; k < outLength; k++) {
				if (inputPositions != NULL)
					srcMapping[*dest + k] = prevSrcMapping[*src] + shift;
				if (outputPositions != NULL)
					outputPositions[prevSrcMapping[*src + k]] = *dest;
			}
			for (k = outLength; k < inLength; k++)
				if (outputPositions != NULL)
					outputPositions[prevSrcMapping[*src + k]] = *dest;
		} else {
			for (k = 0; k < inLength; k++) {
				if (inputPositions != NULL)
					srcMapping[*dest + k] = prevSrcMapping[*src] + shift;
				if (outputPositions != NULL)
					outputPositions[prevSrcMapping[*src + k]] = *dest;
			}
			for (k = inLength; k < outLength; k++)
				if (inputPositions != NULL)
					srcMapping[*dest + k] = prevSrcMapping[*src] + shift;
		}
	}
	*dest += outLength;
	return 1;
}

static int
syllableBreak(const TranslationTableHeader *table, int src, int srcmax,
		const widechar *currentInput, int transCharslen) {
	int wordStart = 0;
	int wordEnd = 0;
	int wordSize = 0;
	int k = 0;
	char *hyphens = NULL;
	for (wordStart = src; wordStart >= 0; wordStart--)
		if (!((findCharOrDots(currentInput[wordStart], 0, table))->attributes &
					CTC_Letter)) {
			wordStart++;
			break;
		}
	if (wordStart < 0) wordStart = 0;
	for (wordEnd = src; wordEnd < srcmax; wordEnd++)
		if (!((findCharOrDots(currentInput[wordEnd], 0, table))->attributes &
					CTC_Letter)) {
			wordEnd--;
			break;
		}
	if (wordEnd == srcmax) wordEnd--;
	/* At this stage wordStart is the 0 based index of the first letter in the word,
	 * wordEnd is the 0 based index of the last letter in the word.
	 * example: "hello" wordstart=0, wordEnd=4. */
	wordSize = wordEnd - wordStart + 1;
	hyphens = (char *)calloc(wordSize + 1, sizeof(char));
	if (!hyphenate(&currentInput[wordStart], wordSize, hyphens, table)) {
		free(hyphens);
		return 0;
	}
	for (k = src - wordStart + 1; k < (src - wordStart + transCharslen); k++)
		if (hyphens[k] & 1) {
			free(hyphens);
			return 1;
		}
	free(hyphens);
	return 0;
}

static void
setBefore(const TranslationTableHeader *table, int src, const widechar *currentInput,
		TranslationTableCharacterAttributes *beforeAttributes) {
	widechar before;
	if (src >= 2 && currentInput[src - 1] == ENDSEGMENT)
		before = currentInput[src - 2];
	else
		before = (src == 0) ? ' ' : currentInput[src - 1];
	*beforeAttributes = (findCharOrDots(before, 0, table))->attributes;
}

static void
setAfter(int length, const TranslationTableHeader *table, int src, int srcmax,
		const widechar *currentInput,
		TranslationTableCharacterAttributes *afterAttributes) {
	widechar after;
	if ((src + length + 2) < srcmax && currentInput[src + 1] == ENDSEGMENT)
		after = currentInput[src + 2];
	else
		after = (src + length < srcmax) ? currentInput[src + length] : ' ';
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
validMatch(const TranslationTableHeader *table, int src, const widechar *currentInput,
		formtype *typebuf, const TranslationTableRule *transRule, int transCharslen) {
	/* Analyze the typeform parameter and also check for capitalization */
	TranslationTableCharacter *currentInputChar;
	TranslationTableCharacter *ruleChar;
	TranslationTableCharacterAttributes prevAttr = 0;
	int k;
	int kk = 0;
	if (!transCharslen) return 0;
	for (k = src; k < src + transCharslen; k++) {
		if (currentInput[k] == ENDSEGMENT) {
			if (k == src && transCharslen == 1)
				return 1;
			else
				return 0;
		}
		currentInputChar = findCharOrDots(currentInput[k], 0, table);
		if (k == src) prevAttr = currentInputChar->attributes;
		ruleChar = findCharOrDots(transRule->charsdots[kk++], 0, table);
		if ((currentInputChar->lowercase != ruleChar->lowercase)) return 0;
		if (typebuf != NULL && (typebuf[src] & CAPSEMPH) == 0 &&
				(typebuf[k] | typebuf[src]) != typebuf[src])
			return 0;
		if (currentInputChar->attributes != CTC_Letter) {
			if (k != (src + 1) && (prevAttr & CTC_Letter) &&
					(currentInputChar->attributes & CTC_Letter) &&
					((currentInputChar->attributes &
							 (CTC_LowerCase | CTC_UpperCase | CTC_Letter)) !=
							(prevAttr & (CTC_LowerCase | CTC_UpperCase | CTC_Letter))))
				return 0;
		}
		prevAttr = currentInputChar->attributes;
	}
	return 1;
}

static int
doCompEmph(const TranslationTableHeader *table, int *src, int *dest, int srcmax,
		int destmax, int mode, const widechar *currentInput, widechar *currentOutput,
		int *srcMapping, int *prevSrcMapping, formtype *typebuf,
		unsigned int *emphasisBuffer, unsigned int *transNoteBuffer,
		const TranslationTableRule **transRule, int *inputPositions, int *outputPositions,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd) {
	int endEmph;
	for (endEmph = *src; (typebuf[endEmph] & computer_braille) && endEmph <= srcmax;
			endEmph++)
		;
	return doCompTrans(*src, endEmph, table, src, dest, srcmax, destmax, mode,
			currentInput, currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
			transNoteBuffer, transRule, inputPositions, outputPositions, cursorPosition,
			cursorStatus, compbrlStart, compbrlEnd);
}

static int
insertBrailleIndicators(int finish, const TranslationTableHeader *table, int *src,
		int *dest, int srcmax, int destmax, int mode, const widechar *currentInput,
		widechar *currentOutput, int *srcMapping, int *prevSrcMapping, formtype *typebuf,
		int haveEmphasis, unsigned int *emphasisBuffer, unsigned int *transNoteBuffer,
		int transOpcode, int prevTransOpcode, const TranslationTableRule **transRule,
		int *inputPositions, int *outputPositions, int *cursorPosition, int *cursorStatus,
		int compbrlStart, int compbrlEnd,
		TranslationTableCharacterAttributes beforeAttributes, int *prevType, int *curType,
		int *prevTypeform, int prevSrc) {
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
		if (*src == prevSrc && !finish) return 1;
		if (*src != prevSrc) {
			if (haveEmphasis && (typebuf[*src] & EMPHASIS) != *prevTypeform) {
				*prevType = *prevTypeform & EMPHASIS;
				*curType = typebuf[*src] & EMPHASIS;
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
				*prevTypeform = typebuf[*src] & EMPHASIS;
			}
			break;
		case checkNumber:
			if (brailleIndicatorDefined(table->numberSign, table, &indicRule) &&
					checkAttr_safe(currentInput, *src, CTC_Digit, 0, table, srcmax) &&
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
			if ((checkAttr_safe(currentInput, *src, CTC_Letter, 0, table, srcmax) &&
						!(beforeAttributes & CTC_Letter)) &&
					(!checkAttr_safe(
							 currentInput, *src + 1, CTC_Letter, 0, table, srcmax) ||
							(beforeAttributes & CTC_Digit))) {
				ok = 1;
				if (*src > 0)
					for (k = 0; k < table->noLetsignBeforeCount; k++)
						if (currentInput[*src - 1] == table->noLetsignBefore[k]) {
							ok = 0;
							break;
						}
				for (k = 0; k < table->noLetsignCount; k++)
					if (currentInput[*src] == table->noLetsign[k]) {
						ok = 0;
						break;
					}
				if ((*src + 1) < srcmax)
					for (k = 0; k < table->noLetsignAfterCount; k++)
						if (currentInput[*src + 1] == table->noLetsignAfter[k]) {
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
						table, src, dest, srcmax, destmax, mode, currentInput,
						currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
						transNoteBuffer, transRule, inputPositions, outputPositions,
						cursorPosition, cursorStatus, compbrlStart, compbrlEnd))
				return 0;
			if (*cursorStatus == 2) checkWhat = checkNothing;
		}
	} while (checkWhat != checkNothing);
	return 1;
}

static int
onlyLettersBehind(const TranslationTableHeader *table, int src,
		const widechar *currentInput,
		TranslationTableCharacterAttributes beforeAttributes) {
	/* Actually, spaces, then letters */
	int k;
	if (!(beforeAttributes & CTC_Space)) return 0;
	for (k = src - 2; k >= 0; k--) {
		TranslationTableCharacterAttributes attr =
				(findCharOrDots(currentInput[k], 0, table))->attributes;
		if ((attr & CTC_Space)) continue;
		if ((attr & CTC_Letter))
			return 1;
		else
			return 0;
	}
	return 1;
}

static int
onlyLettersAhead(const TranslationTableHeader *table, int src, int srcmax,
		const widechar *currentInput, int transCharslen,
		TranslationTableCharacterAttributes afterAttributes) {
	/* Actullly, spaces, then letters */
	int k;
	if (!(afterAttributes & CTC_Space)) return 0;
	for (k = src + transCharslen + 1; k < srcmax; k++) {
		TranslationTableCharacterAttributes attr =
				(findCharOrDots(currentInput[k], 0, table))->attributes;
		if ((attr & CTC_Space)) continue;
		if ((attr & (CTC_Letter | CTC_LitDigit)))
			return 1;
		else
			return 0;
	}
	return 0;
}

static int
noCompbrlAhead(const TranslationTableHeader *table, int src, int srcmax, int mode,
		const widechar *currentInput, int transOpcode, int transCharslen,
		int cursorPosition) {
	int start = src + transCharslen;
	int end;
	int curSrc;
	if (start >= srcmax) return 1;
	while (start < srcmax && checkAttr(currentInput[start], CTC_Space, 0, table)) start++;
	if (start == srcmax ||
			(transOpcode == CTO_JoinableWord &&
					(!checkAttr(currentInput[start], CTC_Letter | CTC_Digit, 0, table) ||
							!checkAttr(currentInput[start - 1], CTC_Space, 0, table))))
		return 1;
	end = start;
	while (end < srcmax && !checkAttr(currentInput[end], CTC_Space, 0, table)) end++;
	if ((mode & (compbrlAtCursor | compbrlLeftCursor)) && cursorPosition >= start &&
			cursorPosition < end)
		return 0;
	/* Look ahead for rules with CTO_CompBrl */
	for (curSrc = start; curSrc < end; curSrc++) {
		int length = srcmax - curSrc;
		int tryThis;
		const TranslationTableCharacter *character1;
		const TranslationTableCharacter *character2;
		int k;
		character1 = findCharOrDots(currentInput[curSrc], 0, table);
		for (tryThis = 0; tryThis < 2; tryThis++) {
			TranslationTableOffset ruleOffset = 0;
			TranslationTableRule *testRule;
			unsigned long int makeHash = 0;
			switch (tryThis) {
			case 0:
				if (!(length >= 2)) break;
				/* Hash function optimized for forward translation */
				makeHash = (unsigned long int)character1->lowercase << 8;
				character2 = findCharOrDots(currentInput[curSrc + 1], 0, table);
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
					character2 = findCharOrDots(currentInput[curSrc + k], 0, table);
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
isRepeatedWord(const TranslationTableHeader *table, int src, int srcmax,
		const widechar *currentInput, int transCharslen, const widechar **repwordStart,
		int *repwordLength) {
	int start;
	if (src == 0 || !checkAttr(currentInput[src - 1], CTC_Letter, 0, table)) return 0;
	if ((src + transCharslen) >= srcmax ||
			!checkAttr(currentInput[src + transCharslen], CTC_Letter, 0, table))
		return 0;
	for (start = src - 2;
			start >= 0 && checkAttr(currentInput[start], CTC_Letter, 0, table); start--)
		;
	start++;
	*repwordStart = &currentInput[start];
	*repwordLength = src - start;
	if (compareChars(*repwordStart, &currentInput[src + transCharslen], *repwordLength, 0,
				table))
		return 1;
	return 0;
}

static int
checkEmphasisChange(const int skip, int src, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, const TranslationTableRule *transRule) {
	int i;
	for (i = src + (skip + 1); i < src + transRule->charslen; i++)
		if (emphasisBuffer[i] || transNoteBuffer[i]) return 1;
	return 0;
}

static int
inSequence(const TranslationTableHeader *table, int src, int srcmax,
		const widechar *currentInput, const TranslationTableRule *transRule) {
	int i, j, s, match;
	// TODO: all caps words
	// const TranslationTableCharacter *c = NULL;

	/* check before sequence */
	for (i = src - 1; i >= 0; i--) {
		if (checkAttr(currentInput[i], CTC_SeqBefore, 0, table)) continue;
		if (!(checkAttr(currentInput[i], CTC_Space | CTC_SeqDelimiter, 0, table)))
			return 0;
		break;
	}

	/* check after sequence */
	for (i = src + transRule->charslen; i < srcmax; i++) {
		/* check sequence after patterns */
		if (table->seqPatternsCount) {
			match = 0;
			for (j = i, s = 0; j <= srcmax && s < table->seqPatternsCount; j++, s++) {
				/* matching */
				if (match == 1) {
					if (table->seqPatterns[s]) {
						if (currentInput[j] == table->seqPatterns[s])
							match = 1;
						else {
							match = -1;
							j = i - 1;
						}
					}

					/* found match */
					else {
						/* pattern at end of input */
						if (j >= srcmax) return 1;

						i = j;
						break;
					}
				}

				/* looking for match */
				else if (match == 0) {
					if (table->seqPatterns[s]) {
						if (currentInput[j] == table->seqPatterns[s])
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

		if (checkAttr(currentInput[i], CTC_SeqAfter, 0, table)) continue;
		if (!(checkAttr(currentInput[i], CTC_Space | CTC_SeqDelimiter, 0, table)))
			return 0;
		break;
	}

	return 1;
}

static void
for_selectRule(const TranslationTableHeader *table, int src, int dest, int srcmax,
		int destmax, int mode, const widechar *currentInput, formtype *typebuf,
		unsigned int *emphasisBuffer, unsigned int *transNoteBuffer, int *transOpcode,
		int prevTransOpcode, const TranslationTableRule **transRule, int *transCharslen,
		int *passCharDots, int *passSrc, widechar const **passInstructions, int *passIC,
		int *startMatch, int *startReplace, int *endReplace, int srcIncremented,
		int cursorPosition, const widechar **repwordStart, int *repwordLength,
		int dontContract, int compbrlStart, int compbrlEnd,
		TranslationTableCharacterAttributes beforeAttributes,
		TranslationTableCharacter **curCharDef, TranslationTableRule **groupingRule,
		widechar *groupingOp) {
	/* check for valid Translations. Return value is in transRule. */
	static TranslationTableRule pseudoRule = { 0 };
	int length = srcmax - src;
	int tryThis;
	const TranslationTableCharacter *character2;
	int k;
	TranslationTableOffset ruleOffset = 0;
	*curCharDef = findCharOrDots(currentInput[src], 0, table);
	for (tryThis = 0; tryThis < 3; tryThis++) {
		unsigned long int makeHash = 0;
		switch (tryThis) {
		case 0:
			if (!(length >= 2)) break;
			/* Hash function optimized for forward translation */
			makeHash = (unsigned long int)(*curCharDef)->lowercase << 8;
			character2 = findCharOrDots(currentInput[src + 1], 0, table);
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
			pseudoRule.charsdots[0] = currentInput[src];
			pseudoRule.dotslen = 0;
			return;
			break;
		}
		while (ruleOffset) {
			*transRule = (TranslationTableRule *)&table->ruleArea[ruleOffset];
			*transOpcode = (*transRule)->opcode;
			*transCharslen = (*transRule)->charslen;
			if (tryThis == 1 || ((*transCharslen <= length) &&
										validMatch(table, src, currentInput, typebuf,
												*transRule, *transCharslen))) {
				TranslationTableCharacterAttributes afterAttributes;
				/* check before emphasis match */
				if ((*transRule)->before & CTC_EmpMatch) {
					if (emphasisBuffer[src] || transNoteBuffer[src]) break;
				}

				/* check after emphasis match */
				if ((*transRule)->after & CTC_EmpMatch) {
					if (emphasisBuffer[src + *transCharslen] ||
							transNoteBuffer[src + *transCharslen])
						break;
				}

				/* check this rule */
				setAfter(*transCharslen, table, src, srcmax, currentInput,
						&afterAttributes);
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
								src >= compbrlStart && src <= compbrlEnd)
							break;
						return;
					case CTO_RepWord:
						if (dontContract || (mode & noContractions)) break;
						if (isRepeatedWord(table, src, srcmax, currentInput,
									*transCharslen, repwordStart, repwordLength))
							return;
						break;
					case CTO_NoCont:
						if (dontContract || (mode & noContractions)) break;
						return;
					case CTO_Syllable:
						*transOpcode = CTO_Always;
					case CTO_Always:
						if (checkEmphasisChange(
									0, src, emphasisBuffer, transNoteBuffer, *transRule))
							break;
						if (dontContract || (mode & noContractions)) break;
						return;
					case CTO_ExactDots:
						return;
					case CTO_NoCross:
						if (dontContract || (mode & noContractions)) break;
						if (syllableBreak(
									table, src, srcmax, currentInput, *transCharslen))
							break;
						return;
					case CTO_Context:
						if (!srcIncremented ||
								!passDoTest(table, src, srcmax, currentInput,
										*transOpcode, *transRule, passCharDots, passSrc,
										passInstructions, passIC, startMatch,
										startReplace, endReplace, groupingRule,
										groupingOp))
							break;
						return;
					case CTO_LargeSign:
						if (dontContract || (mode & noContractions)) break;
						if (!((beforeAttributes & (CTC_Space | CTC_Punctuation)) ||
									onlyLettersBehind(table, src, currentInput,
											beforeAttributes)) ||
								!((afterAttributes & CTC_Space) ||
										prevTransOpcode == CTO_LargeSign) ||
								(afterAttributes & CTC_Letter) ||
								!noCompbrlAhead(table, src, srcmax, mode, currentInput,
										*transOpcode, *transCharslen, cursorPosition))
							*transOpcode = CTO_Always;
						return;
					case CTO_WholeWord:
						if (dontContract || (mode & noContractions)) break;
						if (checkEmphasisChange(
									0, src, emphasisBuffer, transNoteBuffer, *transRule))
							break;
					case CTO_Contraction:
						if (table->usesSequences) {
							if (inSequence(table, src, srcmax, currentInput, *transRule))
								return;
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
								(dest + (*transRule)->dotslen < destmax)) {
							int cursrc = src + *transCharslen + 1;
							while (cursrc < srcmax) {
								if (!checkAttr(
											currentInput[cursrc], CTC_Space, 0, table)) {
									if (checkAttr(currentInput[cursrc], CTC_Digit, 0,
												table))
										return;
									break;
								}
								cursrc++;
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
								onlyLettersAhead(table, src, srcmax, currentInput,
										*transCharslen, afterAttributes) &&
								noCompbrlAhead(table, src, srcmax, mode, currentInput,
										*transOpcode, *transCharslen, cursorPosition))
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
						if (!checkAttr(currentInput[src], CTC_Punctuation, 0, table) ||
								(src > 0 && checkAttr(currentInput[src - 1], CTC_Letter,
													0, table)))
							break;
						for (k = src + *transCharslen; k < srcmax; k++) {
							if (checkAttr(currentInput[k], (CTC_Letter | CTC_Digit), 0,
										table))
								return;
							if (checkAttr(currentInput[k], CTC_Space, 0, table)) break;
						}
						break;
					case CTO_PostPunc:
						if (!checkAttr(currentInput[src], CTC_Punctuation, 0, table) ||
								(src < (srcmax - 1) && checkAttr(currentInput[src + 1],
															   CTC_Letter, 0, table)))
							break;
						for (k = src; k >= 0; k--) {
							if (checkAttr(currentInput[k], (CTC_Letter | CTC_Digit), 0,
										table))
								return;
							if (checkAttr(currentInput[k], CTC_Space, 0, table)) break;
						}
						break;

					case CTO_Match: {
						widechar *patterns, *pattern;

						if (dontContract || (mode & noContractions)) break;
						if (checkEmphasisChange(
									0, src, emphasisBuffer, transNoteBuffer, *transRule))
							break;

						patterns = (widechar *)&table->ruleArea[(*transRule)->patterns];

						/* check before pattern */
						pattern = &patterns[1];
						if (!_lou_pattern_check(
									currentInput, src - 1, -1, -1, pattern, table))
							break;

						/* check after pattern */
						pattern = &patterns[patterns[0]];
						if (!_lou_pattern_check(currentInput,
									src + (*transRule)->charslen, srcmax, 1, pattern,
									table))
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
undefinedCharacter(widechar c, const TranslationTableHeader *table, int *src, int *dest,
		int srcmax, int destmax, int mode, const widechar *currentInput,
		widechar *currentOutput, int *srcMapping, int *prevSrcMapping,
		unsigned int *emphasisBuffer, unsigned int *transNoteBuffer,
		const TranslationTableRule **transRule, int *inputPositions, int *outputPositions,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd) {
	/* Display an undefined character in the output buffer */
	int k;
	char *display;
	widechar displayDots[20];
	if (table->undefined) {
		TranslationTableRule *rule =
				(TranslationTableRule *)&table->ruleArea[table->undefined];
		if (!for_updatePositions(&rule->charsdots[rule->charslen], rule->charslen,
					rule->dotslen, 0, table, src, dest, srcmax, destmax, mode,
					currentInput, currentOutput, srcMapping, prevSrcMapping,
					emphasisBuffer, transNoteBuffer, transRule, inputPositions,
					outputPositions, cursorPosition, cursorStatus, compbrlStart,
					compbrlEnd))
			return 0;
		return 1;
	}
	display = _lou_showString(&c, 1);
	for (k = 0; k < (int)strlen(display); k++)
		displayDots[k] = _lou_getDotsForChar(display[k]);
	if (!for_updatePositions(displayDots, 1, (int)strlen(display), 0, table, src, dest,
				srcmax, destmax, mode, currentInput, currentOutput, srcMapping,
				prevSrcMapping, emphasisBuffer, transNoteBuffer, transRule,
				inputPositions, outputPositions, cursorPosition, cursorStatus,
				compbrlStart, compbrlEnd))
		return 0;
	return 1;
}

static int
putCharacter(widechar character, const TranslationTableHeader *table, int *src, int *dest,
		int srcmax, int destmax, int mode, const widechar *currentInput,
		widechar *currentOutput, int *srcMapping, int *prevSrcMapping,
		unsigned int *emphasisBuffer, unsigned int *transNoteBuffer,
		const TranslationTableRule **transRule, int *inputPositions, int *outputPositions,
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
					src, dest, srcmax, destmax, mode, currentInput, currentOutput,
					srcMapping, prevSrcMapping, emphasisBuffer, transNoteBuffer,
					transRule, inputPositions, outputPositions, cursorPosition,
					cursorStatus, compbrlStart, compbrlEnd);
		d = _lou_getDotsForChar(character);
		return for_updatePositions(&d, 1, 1, 0, table, src, dest, srcmax, destmax, mode,
				currentInput, currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
				transNoteBuffer, transRule, inputPositions, outputPositions,
				cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
	}
	return undefinedCharacter(character, table, src, dest, srcmax, destmax, mode,
			currentInput, currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
			transNoteBuffer, transRule, inputPositions, outputPositions, cursorPosition,
			cursorStatus, compbrlStart, compbrlEnd);
}

static int
putCharacters(const widechar *characters, int count, const TranslationTableHeader *table,
		int *src, int *dest, int srcmax, int destmax, int mode,
		const widechar *currentInput, widechar *currentOutput, int *srcMapping,
		int *prevSrcMapping, unsigned int *emphasisBuffer, unsigned int *transNoteBuffer,
		const TranslationTableRule **transRule, int *inputPositions, int *outputPositions,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd) {
	/* Insert the dot equivalents of a series of characters in the output
	 * buffer */
	int k;
	for (k = 0; k < count; k++)
		if (!putCharacter(characters[k], table, src, dest, srcmax, destmax, mode,
					currentInput, currentOutput, srcMapping, prevSrcMapping,
					emphasisBuffer, transNoteBuffer, transRule, inputPositions,
					outputPositions, cursorPosition, cursorStatus, compbrlStart,
					compbrlEnd))
			return 0;
	return 1;
}

static int
doCompbrl(const TranslationTableHeader *table, int *src, int *dest, int srcmax,
		int destmax, int mode, const widechar *currentInput, widechar *currentOutput,
		int *srcMapping, int *prevSrcMapping, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, const TranslationTableRule **transRule,
		int *inputPositions, int *outputPositions, int *cursorPosition, int *cursorStatus,
		int destword, int srcword, int compbrlStart, int compbrlEnd) {
	/* Handle strings containing substrings defined by the compbrl opcode */
	int stringStart, stringEnd;
	if (checkAttr(currentInput[*src], CTC_Space, 0, table)) return 1;
	if (destword) {
		*src = srcword;
		*dest = destword;
	} else {
		*src = 0;
		*dest = 0;
	}
	for (stringStart = *src; stringStart >= 0; stringStart--)
		if (checkAttr(currentInput[stringStart], CTC_Space, 0, table)) break;
	stringStart++;
	for (stringEnd = *src; stringEnd < srcmax; stringEnd++)
		if (checkAttr(currentInput[stringEnd], CTC_Space, 0, table)) break;
	return doCompTrans(stringStart, stringEnd, table, src, dest, srcmax, destmax, mode,
			currentInput, currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
			transNoteBuffer, transRule, inputPositions, outputPositions, cursorPosition,
			cursorStatus, compbrlStart, compbrlEnd);
}

static int
putCompChar(widechar character, const TranslationTableHeader *table, int *src, int *dest,
		int srcmax, int destmax, int mode, const widechar *currentInput,
		widechar *currentOutput, int *srcMapping, int *prevSrcMapping,
		unsigned int *emphasisBuffer, unsigned int *transNoteBuffer,
		const TranslationTableRule **transRule, int *inputPositions, int *outputPositions,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd) {
	/* Insert the dots equivalent of a character into the output buffer */
	widechar d;
	TranslationTableOffset offset = (findCharOrDots(character, 0, table))->definitionRule;
	if (offset) {
		const TranslationTableRule *rule =
				(TranslationTableRule *)&table->ruleArea[offset];
		if (rule->dotslen)
			return for_updatePositions(&rule->charsdots[1], 1, rule->dotslen, 0, table,
					src, dest, srcmax, destmax, mode, currentInput, currentOutput,
					srcMapping, prevSrcMapping, emphasisBuffer, transNoteBuffer,
					transRule, inputPositions, outputPositions, cursorPosition,
					cursorStatus, compbrlStart, compbrlEnd);
		d = _lou_getDotsForChar(character);
		return for_updatePositions(&d, 1, 1, 0, table, src, dest, srcmax, destmax, mode,
				currentInput, currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
				transNoteBuffer, transRule, inputPositions, outputPositions,
				cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
	}
	return undefinedCharacter(character, table, src, dest, srcmax, destmax, mode,
			currentInput, currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
			transNoteBuffer, transRule, inputPositions, outputPositions, cursorPosition,
			cursorStatus, compbrlStart, compbrlEnd);
}

static int
doCompTrans(int start, int end, const TranslationTableHeader *table, int *src, int *dest,
		int srcmax, int destmax, int mode, const widechar *currentInput,
		widechar *currentOutput, int *srcMapping, int *prevSrcMapping,
		unsigned int *emphasisBuffer, unsigned int *transNoteBuffer,
		const TranslationTableRule **transRule, int *inputPositions, int *outputPositions,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd) {
	const TranslationTableRule *indicRule;
	int k;
	int haveEndsegment = 0;
	if (*cursorStatus != 2 && brailleIndicatorDefined(table->begComp, table, &indicRule))
		if (!for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, 0,
					table, src, dest, srcmax, destmax, mode, currentInput, currentOutput,
					srcMapping, prevSrcMapping, emphasisBuffer, transNoteBuffer,
					transRule, inputPositions, outputPositions, cursorPosition,
					cursorStatus, compbrlStart, compbrlEnd))
			return 0;
	for (k = start; k < end; k++) {
		TranslationTableOffset compdots = 0;
		/* HACK: computer braille is one-to-one so it
		 * can't have any emphasis indicators.
		 * A better solution is to treat computer braille as its own mode. */
		emphasisBuffer[k] = 0;
		transNoteBuffer[k] = 0;
		if (currentInput[k] == ENDSEGMENT) {
			haveEndsegment = 1;
			continue;
		}
		*src = k;
		if (currentInput[k] < 256) compdots = table->compdotsPattern[currentInput[k]];
		if (compdots != 0) {
			*transRule = (TranslationTableRule *)&table->ruleArea[compdots];
			if (!for_updatePositions(&(*transRule)->charsdots[(*transRule)->charslen],
						(*transRule)->charslen, (*transRule)->dotslen, 0, table, src,
						dest, srcmax, destmax, mode, currentInput, currentOutput,
						srcMapping, prevSrcMapping, emphasisBuffer, transNoteBuffer,
						transRule, inputPositions, outputPositions, cursorPosition,
						cursorStatus, compbrlStart, compbrlEnd))
				return 0;
		} else if (!putCompChar(currentInput[k], table, src, dest, srcmax, destmax, mode,
						   currentInput, currentOutput, srcMapping, prevSrcMapping,
						   emphasisBuffer, transNoteBuffer, transRule, inputPositions,
						   outputPositions, cursorPosition, cursorStatus, compbrlStart,
						   compbrlEnd))
			return 0;
	}
	if (*cursorStatus != 2 && brailleIndicatorDefined(table->endComp, table, &indicRule))
		if (!for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, 0,
					table, src, dest, srcmax, destmax, mode, currentInput, currentOutput,
					srcMapping, prevSrcMapping, emphasisBuffer, transNoteBuffer,
					transRule, inputPositions, outputPositions, cursorPosition,
					cursorStatus, compbrlStart, compbrlEnd))
			return 0;
	*src = end;
	if (haveEndsegment) {
		widechar endSegment = ENDSEGMENT;
		if (!for_updatePositions(&endSegment, 0, 1, 0, table, src, dest, srcmax, destmax,
					mode, currentInput, currentOutput, srcMapping, prevSrcMapping,
					emphasisBuffer, transNoteBuffer, transRule, inputPositions,
					outputPositions, cursorPosition, cursorStatus, compbrlStart,
					compbrlEnd))
			return 0;
	}
	return 1;
}

static int
doNocont(const TranslationTableHeader *table, int *src, int *dest, int mode,
		const widechar *currentInput, int destword, int srcword, int *dontContract) {
	/* Handle strings containing substrings defined by the nocont opcode */
	if (checkAttr(currentInput[*src], CTC_Space, 0, table) || *dontContract ||
			(mode & noContractions))
		return 1;
	if (destword) {
		*src = srcword;
		*dest = destword;
	} else {
		*src = 0;
		*dest = 0;
	}
	*dontContract = 1;
	return 1;
}

static int
markSyllables(const TranslationTableHeader *table, int srcmax,
		const widechar *currentInput, formtype *typebuf, int *transOpcode,
		const TranslationTableRule **transRule, int *transCharslen) {
	int src;
	int k;
	int currentMark = 0;
	int const syllable_marks[] = { SYLLABLE_MARKER_1, SYLLABLE_MARKER_2 };
	int syllable_mark_selector = 0;

	if (typebuf == NULL || !table->syllables) return 1;
	src = 0;
	while (src < srcmax) { /* the main multipass translation loop */
		int length = srcmax - src;
		const TranslationTableCharacter *character =
				findCharOrDots(currentInput[src], 0, table);
		const TranslationTableCharacter *character2;
		int tryThis = 0;
		while (tryThis < 3) {
			TranslationTableOffset ruleOffset = 0;
			unsigned long int makeHash = 0;
			switch (tryThis) {
			case 0:
				if (!(length >= 2)) break;
				makeHash = (unsigned long int)character->lowercase << 8;
				// memory overflow when src == srcmax - 1
				character2 = findCharOrDots(currentInput[src + 1], 0, table);
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
										&currentInput[src], *transCharslen, 0, table))) {
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
			if (src >= srcmax) return 0;
			typebuf[src++] |= currentMark;
			break;
		case CTO_Syllable:
			/* cycle between SYLLABLE_MARKER_1 and SYLLABLE_MARKER_2 so
			 * we can distinguinsh two consequtive syllables */
			currentMark = syllable_marks[syllable_mark_selector];
			syllable_mark_selector = (syllable_mark_selector + 1) % 2;

			if ((src + *transCharslen) > srcmax) return 0;
			for (k = 0; k < *transCharslen; k++) typebuf[src++] |= currentMark;
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
		const unsigned int bit_symbol, int srcmax, unsigned int *wordBuffer) {
	int in_word = 0, in_emp = 0, word_start = -1, word_whole = 0, word_stop;
	int i;

	for (i = 0; i < srcmax; i++) {
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
		unsigned int *wordBuffer, int *inputPositions, int *outputPositions,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd) {
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
		const TranslationTableHeader *table, int srcmax, unsigned int *wordBuffer,
		int *inputPositions, int *outputPositions, int *cursorPosition, int *cursorStatus,
		int compbrlStart, int compbrlEnd) {
	unsigned int word_cnt = 0;
	int pass_start = -1, pass_end = -1, word_start = -1, in_word = 0, in_pass = 0;
	int i;

	for (i = 0; i < srcmax; i++) {
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
									table, wordBuffer, inputPositions, outputPositions,
									cursorPosition, cursorStatus, compbrlStart,
									compbrlEnd);
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
								wordBuffer, inputPositions, outputPositions,
								cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
					}
				in_pass = 0;
			}
	}

	if (in_pass) {
		if (word_cnt >= table->emphRules[emphRule][lenPhraseOffset]) {
			if (pass_end >= 0) {
				if (in_word) {
					convertToPassage(pass_start, i, word_start, buffer, emphRule,
							bit_begin, bit_end, bit_word, bit_symbol, table, wordBuffer,
							inputPositions, outputPositions, cursorPosition, cursorStatus,
							compbrlStart, compbrlEnd);
				} else {
					convertToPassage(pass_start, pass_end, word_start, buffer, emphRule,
							bit_begin, bit_end, bit_word, bit_symbol, table, wordBuffer,
							inputPositions, outputPositions, cursorPosition, cursorStatus,
							compbrlStart, compbrlEnd);
				}
			}
		}
	}
}

static void
resolveEmphasisSingleSymbols(unsigned int *buffer, const unsigned int bit_begin,
		const unsigned int bit_end, const unsigned int bit_symbol, int srcmax) {
	int i;

	for (i = 0; i < srcmax; i++) {
		if (buffer[i] & bit_begin)
			if (buffer[i + 1] & bit_end) {
				buffer[i] &= ~bit_begin;
				buffer[i + 1] &= ~bit_end;
				buffer[i] |= bit_symbol;
			}
	}
}

static void
resolveEmphasisAllCapsSymbols(unsigned int *buffer, formtype *typebuf, int srcmax) {
	/* Marks every caps letter with CAPS_SYMBOL.
	 * Used in the special case where capsnocont has been defined and capsword has not
	 * been defined. */

	const unsigned int bit_begin = CAPS_BEGIN;
	const unsigned int bit_end = CAPS_END;
	const unsigned int bit_symbol = CAPS_SYMBOL;

	int inEmphasis = 0, i;

	for (i = 0; i < srcmax; i++) {
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
		const unsigned int bit_symbol, const TranslationTableHeader *table, int srcmax,
		const widechar *currentInput, unsigned int *wordBuffer) {
	int in_word = 0, in_pass = 0, word_start = -1, word_reset = 0, orig_reset = -1,
		letter_cnt = 0;
	int i, j;

	for (i = 0; i < srcmax; i++) {
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
								!checkAttr(currentInput[i], CTC_Letter, 0, table)) {
							/* not just one reset by itself */
							if (wordBuffer[i + 1] & WORD_CHAR) {
								buffer[i + 1] |= bit_word;
								if (wordBuffer[i] & WORD_WHOLE)
									wordBuffer[i + 1] |= WORD_WHOLE;
							}
							buffer[i] &= ~bit_word;
							wordBuffer[i] &= ~WORD_WHOLE;

							/* if reset is a letter, make it a symbol */
							if (checkAttr(currentInput[i], CTC_Letter, 0, table))
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
								!checkAttr(currentInput[i], CTC_Letter, 0, table))
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
								!checkAttr(currentInput[i], CTC_Letter, 0, table))
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
								!checkAttr(currentInput[i], CTC_Letter, 0, table)) {
							if (!checkAttr(currentInput[i], CTC_Letter, 0, table)) {
								if (checkAttr(currentInput[i], CTC_CapsMode, 0, table)) {
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
							if (checkAttr(currentInput[i], CTC_Letter, 0, table)) {
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
markEmphases(const TranslationTableHeader *table, int srcmax,
		const widechar *currentInput, formtype *typebuf, unsigned int *wordBuffer,
		unsigned int *emphasisBuffer, unsigned int *transNoteBuffer, int haveEmphasis,
		int *inputPositions, int *outputPositions, int *cursorPosition, int *cursorStatus,
		int compbrlStart, int compbrlEnd) {
	/* Relies on the order of typeforms emph_1..emph_10. */
	int caps_start = -1, last_caps = -1, caps_cnt = 0;
	int emph_start[10] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
	int i, j;

	for (i = 0; i < srcmax; i++) {
		if (!checkAttr(currentInput[i], CTC_Space, 0, table)) {
			wordBuffer[i] |= WORD_CHAR;
		} else if (caps_cnt > 0) {
			last_caps = i;
			caps_cnt = 0;
		}

		if (checkAttr(currentInput[i], CTC_UpperCase, 0, table)) {
			if (caps_start < 0) caps_start = i;
			caps_cnt++;
		} else if (caps_start >= 0) {
			/* caps should keep going until this */
			if (checkAttr(currentInput[i], CTC_Letter, 0, table) &&
					checkAttr(currentInput[i], CTC_LowerCase, 0, table)) {
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

	/* clean up srcmax */
	if (caps_start >= 0) {
		emphasisBuffer[caps_start] |= CAPS_BEGIN;
		if (caps_cnt > 0)
			emphasisBuffer[srcmax] |= CAPS_END;
		else
			emphasisBuffer[last_caps] |= CAPS_END;
	}

	if (haveEmphasis) {
		for (j = 0; j < 10; j++) {
			if (emph_start[j] >= 0) {
				if (j < 5) {
					emphasisBuffer[emph_start[j]] |= EMPHASIS_BEGIN << (j * 4);
					emphasisBuffer[srcmax] |= EMPHASIS_END << (j * 4);
				} else {
					transNoteBuffer[emph_start[j]] |= TRANSNOTE_BEGIN << ((j - 5) * 4);
					transNoteBuffer[srcmax] |= TRANSNOTE_END << ((j - 5) * 4);
				}
			}
		}
	}

	/* Handle capsnocont */
	if (table->capsNoCont) {
		int inCaps = 0;
		for (i = 0; i < srcmax; i++) {
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
				srcmax, wordBuffer);
		if (table->emphRules[capsRule][lenPhraseOffset])
			resolveEmphasisPassages(emphasisBuffer, capsRule, CAPS_BEGIN, CAPS_END,
					CAPS_WORD, CAPS_SYMBOL, table, srcmax, wordBuffer, inputPositions,
					outputPositions, cursorPosition, cursorStatus, compbrlStart,
					compbrlEnd);
		resolveEmphasisResets(emphasisBuffer, CAPS_BEGIN, CAPS_END, CAPS_WORD,
				CAPS_SYMBOL, table, srcmax, currentInput, wordBuffer);
	} else if (table->emphRules[capsRule][letterOffset]) {
		if (table->capsNoCont) /* capsnocont and no capsword */
			resolveEmphasisAllCapsSymbols(emphasisBuffer, typebuf, srcmax);
		else
			resolveEmphasisSingleSymbols(
					emphasisBuffer, CAPS_BEGIN, CAPS_END, CAPS_SYMBOL, srcmax);
	}

	if (!haveEmphasis) return;

	for (j = 0; j < 5; j++) {
		if (table->emphRules[emph1Rule + j][begWordOffset]) {
			resolveEmphasisWords(emphasisBuffer, EMPHASIS_BEGIN << (j * 4),
					EMPHASIS_END << (j * 4), EMPHASIS_WORD << (j * 4),
					EMPHASIS_SYMBOL << (j * 4), srcmax, wordBuffer);
			if (table->emphRules[emph1Rule + j][lenPhraseOffset])
				resolveEmphasisPassages(emphasisBuffer, emph1Rule + j,
						EMPHASIS_BEGIN << (j * 4), EMPHASIS_END << (j * 4),
						EMPHASIS_WORD << (j * 4), EMPHASIS_SYMBOL << (j * 4), table,
						srcmax, wordBuffer, inputPositions, outputPositions,
						cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
		} else if (table->emphRules[emph1Rule + j][letterOffset])
			resolveEmphasisSingleSymbols(emphasisBuffer, EMPHASIS_BEGIN << (j * 4),
					EMPHASIS_END << (j * 4), EMPHASIS_SYMBOL << (j * 4), srcmax);
	}
	for (j = 0; j < 5; j++) {
		if (table->emphRules[emph6Rule + j][begWordOffset]) {
			resolveEmphasisWords(transNoteBuffer, TRANSNOTE_BEGIN << (j * 4),
					TRANSNOTE_END << (j * 4), TRANSNOTE_WORD << (j * 4),
					TRANSNOTE_SYMBOL << (j * 4), srcmax, wordBuffer);
			if (table->emphRules[emph6Rule + j][lenPhraseOffset])
				resolveEmphasisPassages(transNoteBuffer, j + 6,
						TRANSNOTE_BEGIN << (j * 4), TRANSNOTE_END << (j * 4),
						TRANSNOTE_WORD << (j * 4), TRANSNOTE_SYMBOL << (j * 4), table,
						srcmax, wordBuffer, inputPositions, outputPositions,
						cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
		} else if (table->emphRules[emph6Rule + j][letterOffset])
			resolveEmphasisSingleSymbols(transNoteBuffer, TRANSNOTE_BEGIN << (j * 4),
					TRANSNOTE_END << (j * 4), TRANSNOTE_SYMBOL << (j * 4), srcmax);
	}
}

static void
insertEmphasisSymbol(unsigned int *buffer, const int at, const EmphRuleNumber emphRule,
		const unsigned int bit_symbol, const TranslationTableHeader *table, int *src,
		int *dest, int srcmax, int destmax, int mode, const widechar *currentInput,
		widechar *currentOutput, int *srcMapping, int *prevSrcMapping,
		unsigned int *emphasisBuffer, unsigned int *transNoteBuffer,
		const TranslationTableRule **transRule, int *inputPositions, int *outputPositions,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd) {
	if (buffer[at] & bit_symbol) {
		const TranslationTableRule *indicRule;
		if (brailleIndicatorDefined(
					table->emphRules[emphRule][letterOffset], table, &indicRule))
			for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, 0, table,
					src, dest, srcmax, destmax, mode, currentInput, currentOutput,
					srcMapping, prevSrcMapping, emphasisBuffer, transNoteBuffer,
					transRule, inputPositions, outputPositions, cursorPosition,
					cursorStatus, compbrlStart, compbrlEnd);
	}
}

static void
insertEmphasisBegin(unsigned int *buffer, const int at, const EmphRuleNumber emphRule,
		const unsigned int bit_begin, const unsigned int bit_end,
		const unsigned int bit_word, const TranslationTableHeader *table, int *src,
		int *dest, int srcmax, int destmax, int mode, const widechar *currentInput,
		widechar *currentOutput, int *srcMapping, int *prevSrcMapping,
		unsigned int *emphasisBuffer, unsigned int *transNoteBuffer,
		const TranslationTableRule **transRule, int *inputPositions, int *outputPositions,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd) {
	const TranslationTableRule *indicRule;
	if (buffer[at] & bit_begin) {
		if (brailleIndicatorDefined(
					table->emphRules[emphRule][begPhraseOffset], table, &indicRule))
			for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, 0, table,
					src, dest, srcmax, destmax, mode, currentInput, currentOutput,
					srcMapping, prevSrcMapping, emphasisBuffer, transNoteBuffer,
					transRule, inputPositions, outputPositions, cursorPosition,
					cursorStatus, compbrlStart, compbrlEnd);
		else if (brailleIndicatorDefined(
						 table->emphRules[emphRule][begOffset], table, &indicRule))
			for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, 0, table,
					src, dest, srcmax, destmax, mode, currentInput, currentOutput,
					srcMapping, prevSrcMapping, emphasisBuffer, transNoteBuffer,
					transRule, inputPositions, outputPositions, cursorPosition,
					cursorStatus, compbrlStart, compbrlEnd);
	}

	if (buffer[at] & bit_word
			// && !(buffer[at] & bit_begin)
			&& !(buffer[at] & bit_end)) {
		if (brailleIndicatorDefined(
					table->emphRules[emphRule][begWordOffset], table, &indicRule))
			for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, 0, table,
					src, dest, srcmax, destmax, mode, currentInput, currentOutput,
					srcMapping, prevSrcMapping, emphasisBuffer, transNoteBuffer,
					transRule, inputPositions, outputPositions, cursorPosition,
					cursorStatus, compbrlStart, compbrlEnd);
	}
}

static void
insertEmphasisEnd(unsigned int *buffer, const int at, const EmphRuleNumber emphRule,
		const unsigned int bit_end, const unsigned int bit_word,
		const TranslationTableHeader *table, int *src, int *dest, int srcmax, int destmax,
		int mode, const widechar *currentInput, widechar *currentOutput, int *srcMapping,
		int *prevSrcMapping, unsigned int *emphasisBuffer, unsigned int *transNoteBuffer,
		const TranslationTableRule **transRule, int *inputPositions, int *outputPositions,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd) {
	if (buffer[at] & bit_end) {
		const TranslationTableRule *indicRule;
		if (buffer[at] & bit_word) {
			if (brailleIndicatorDefined(
						table->emphRules[emphRule][endWordOffset], table, &indicRule))
				for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, -1,
						table, src, dest, srcmax, destmax, mode, currentInput,
						currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
						transNoteBuffer, transRule, inputPositions, outputPositions,
						cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
		} else {
			if (brailleIndicatorDefined(
						table->emphRules[emphRule][endOffset], table, &indicRule))
				for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, -1,
						table, src, dest, srcmax, destmax, mode, currentInput,
						currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
						transNoteBuffer, transRule, inputPositions, outputPositions,
						cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
			else if (brailleIndicatorDefined(
							 table->emphRules[emphRule][endPhraseAfterOffset], table,
							 &indicRule))
				for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, -1,
						table, src, dest, srcmax, destmax, mode, currentInput,
						currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
						transNoteBuffer, transRule, inputPositions, outputPositions,
						cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
			else if (brailleIndicatorDefined(
							 table->emphRules[emphRule][endPhraseBeforeOffset], table,
							 &indicRule))
				for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, 0,
						table, src, dest, srcmax, destmax, mode, currentInput,
						currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
						transNoteBuffer, transRule, inputPositions, outputPositions,
						cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
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
		const TranslationTableHeader *table, int srcmax, const widechar *currentInput) {
	if (buffer[at] & bit_begin) {
		int i, cnt = 1;
		for (i = at + 1; i < srcmax; i++)
			if (buffer[i] & bit_end)
				break;
			else
				cnt++;
		return cnt;
	} else if (buffer[at] & bit_word) {
		int i, cnt = 1;
		for (i = at + 1; i < srcmax; i++)
			if (buffer[i] & bit_end) break;
			// TODO: WORD_RESET?
			else if (checkAttr(currentInput[i], CTC_SeqDelimiter | CTC_Space, 0, table))
				break;
			else
				cnt++;
		return cnt;
	}
	return 0;
}

static void
insertEmphasesAt(const int at, const TranslationTableHeader *table, int *src, int *dest,
		int srcmax, int destmax, int mode, const widechar *currentInput,
		widechar *currentOutput, int *srcMapping, int *prevSrcMapping,
		unsigned int *emphasisBuffer, unsigned int *transNoteBuffer, int haveEmphasis,
		int transOpcode, const TranslationTableRule **transRule, int *inputPositions,
		int *outputPositions, int *cursorPosition, int *cursorStatus, int compbrlStart,
		int compbrlEnd) {
	int type_counts[10];
	int i, j, min, max;

	/* simple case */
	if (!haveEmphasis) {
		/* insert graded 1 mode indicator */
		if (transOpcode == CTO_Contraction) {
			const TranslationTableRule *indicRule;
			if (brailleIndicatorDefined(table->noContractSign, table, &indicRule))
				for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, 0,
						table, src, dest, srcmax, destmax, mode, currentInput,
						currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
						transNoteBuffer, transRule, inputPositions, outputPositions,
						cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
		}

		if (emphasisBuffer[at] & CAPS_EMPHASIS) {
			insertEmphasisEnd(emphasisBuffer, at, capsRule, CAPS_END, CAPS_WORD, table,
					src, dest, srcmax, destmax, mode, currentInput, currentOutput,
					srcMapping, prevSrcMapping, emphasisBuffer, transNoteBuffer,
					transRule, inputPositions, outputPositions, cursorPosition,
					cursorStatus, compbrlStart, compbrlEnd);
			insertEmphasisBegin(emphasisBuffer, at, capsRule, CAPS_BEGIN, CAPS_END,
					CAPS_WORD, table, src, dest, srcmax, destmax, mode, currentInput,
					currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
					transNoteBuffer, transRule, inputPositions, outputPositions,
					cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
			insertEmphasisSymbol(emphasisBuffer, at, capsRule, CAPS_SYMBOL, table, src,
					dest, srcmax, destmax, mode, currentInput, currentOutput, srcMapping,
					prevSrcMapping, emphasisBuffer, transNoteBuffer, transRule,
					inputPositions, outputPositions, cursorPosition, cursorStatus,
					compbrlStart, compbrlEnd);
		}
		return;
	}

	/* The order of inserting the end symbols must be the reverse
	 * of the insertions of the begin symbols so that they will
	 * nest properly when multiple emphases start and end at
	 * the same place */
	// TODO: ordering with partial word using bit_word and bit_end

	if (emphasisBuffer[at] & CAPS_EMPHASIS)
		insertEmphasisEnd(emphasisBuffer, at, capsRule, CAPS_END, CAPS_WORD, table, src,
				dest, srcmax, destmax, mode, currentInput, currentOutput, srcMapping,
				prevSrcMapping, emphasisBuffer, transNoteBuffer, transRule,
				inputPositions, outputPositions, cursorPosition, cursorStatus,
				compbrlStart, compbrlEnd);

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
					EMPHASIS_END << (min * 4), EMPHASIS_WORD << (min * 4), table, src,
					dest, srcmax, destmax, mode, currentInput, currentOutput, srcMapping,
					prevSrcMapping, emphasisBuffer, transNoteBuffer, transRule,
					inputPositions, outputPositions, cursorPosition, cursorStatus,
					compbrlStart, compbrlEnd);
		else
			insertEmphasisEnd(transNoteBuffer, at, emph1Rule + min,
					TRANSNOTE_END << ((min - 5) * 4), TRANSNOTE_WORD << ((min - 5) * 4),
					table, src, dest, srcmax, destmax, mode, currentInput, currentOutput,
					srcMapping, prevSrcMapping, emphasisBuffer, transNoteBuffer,
					transRule, inputPositions, outputPositions, cursorPosition,
					cursorStatus, compbrlStart, compbrlEnd);
	}

	/* begin and word bits */
	for (i = 0; i < 10; i++)
		if (i < 5)
			type_counts[i] = beginCount(emphasisBuffer, at, EMPHASIS_END << (i * 4),
					EMPHASIS_BEGIN << (i * 4), EMPHASIS_WORD << (i * 4), table, srcmax,
					currentInput);
		else
			type_counts[i] = beginCount(transNoteBuffer, at,
					TRANSNOTE_END << ((i - 5) * 4), TRANSNOTE_BEGIN << ((i - 5) * 4),
					TRANSNOTE_WORD << ((i - 5) * 4), table, srcmax, currentInput);

	for (i = 9; i >= 0; i--) {
		max = 9;
		for (j = 9; j >= 0; j--)
			if (type_counts[max] < type_counts[j]) max = j;
		if (!type_counts[max]) break;
		type_counts[max] = 0;
		if (max >= 5)
			insertEmphasisBegin(transNoteBuffer, at, emph1Rule + max,
					TRANSNOTE_BEGIN << ((max - 5) * 4), TRANSNOTE_END << ((max - 5) * 4),
					TRANSNOTE_WORD << ((max - 5) * 4), table, src, dest, srcmax, destmax,
					mode, currentInput, currentOutput, srcMapping, prevSrcMapping,
					emphasisBuffer, transNoteBuffer, transRule, inputPositions,
					outputPositions, cursorPosition, cursorStatus, compbrlStart,
					compbrlEnd);
		else
			insertEmphasisBegin(emphasisBuffer, at, emph1Rule + max,
					EMPHASIS_BEGIN << (max * 4), EMPHASIS_END << (max * 4),
					EMPHASIS_WORD << (max * 4), table, src, dest, srcmax, destmax, mode,
					currentInput, currentOutput, srcMapping, prevSrcMapping,
					emphasisBuffer, transNoteBuffer, transRule, inputPositions,
					outputPositions, cursorPosition, cursorStatus, compbrlStart,
					compbrlEnd);
	}

	/* symbol bits */
	for (i = 4; i >= 0; i--)
		if (transNoteBuffer[at] & (TRANSNOTE_MASK << (i * 4)))
			insertEmphasisSymbol(transNoteBuffer, at, emph6Rule + i,
					TRANSNOTE_SYMBOL << (i * 4), table, src, dest, srcmax, destmax, mode,
					currentInput, currentOutput, srcMapping, prevSrcMapping,
					emphasisBuffer, transNoteBuffer, transRule, inputPositions,
					outputPositions, cursorPosition, cursorStatus, compbrlStart,
					compbrlEnd);
	for (i = 4; i >= 0; i--)
		if (emphasisBuffer[at] & (EMPHASIS_MASK << (i * 4)))
			insertEmphasisSymbol(emphasisBuffer, at, emph1Rule + i,
					EMPHASIS_SYMBOL << (i * 4), table, src, dest, srcmax, destmax, mode,
					currentInput, currentOutput, srcMapping, prevSrcMapping,
					emphasisBuffer, transNoteBuffer, transRule, inputPositions,
					outputPositions, cursorPosition, cursorStatus, compbrlStart,
					compbrlEnd);

	/* insert graded 1 mode indicator */
	if (transOpcode == CTO_Contraction) {
		const TranslationTableRule *indicRule;
		if (brailleIndicatorDefined(table->noContractSign, table, &indicRule))
			for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, 0, table,
					src, dest, srcmax, destmax, mode, currentInput, currentOutput,
					srcMapping, prevSrcMapping, emphasisBuffer, transNoteBuffer,
					transRule, inputPositions, outputPositions, cursorPosition,
					cursorStatus, compbrlStart, compbrlEnd);
	}

	/* insert capitalization last so it will be closest to word */
	if (emphasisBuffer[at] & CAPS_EMPHASIS) {
		insertEmphasisBegin(emphasisBuffer, at, capsRule, CAPS_BEGIN, CAPS_END, CAPS_WORD,
				table, src, dest, srcmax, destmax, mode, currentInput, currentOutput,
				srcMapping, prevSrcMapping, emphasisBuffer, transNoteBuffer, transRule,
				inputPositions, outputPositions, cursorPosition, cursorStatus,
				compbrlStart, compbrlEnd);
		insertEmphasisSymbol(emphasisBuffer, at, capsRule, CAPS_SYMBOL, table, src, dest,
				srcmax, destmax, mode, currentInput, currentOutput, srcMapping,
				prevSrcMapping, emphasisBuffer, transNoteBuffer, transRule,
				inputPositions, outputPositions, cursorPosition, cursorStatus,
				compbrlStart, compbrlEnd);
	}
}

static void
insertEmphases(const TranslationTableHeader *table, int *src, int *dest, int srcmax,
		int destmax, int mode, const widechar *currentInput, widechar *currentOutput,
		int *srcMapping, int *prevSrcMapping, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, int haveEmphasis, int transOpcode,
		const TranslationTableRule **transRule, int *inputPositions, int *outputPositions,
		int *cursorPosition, int *cursorStatus, int *pre_src, int compbrlStart,
		int compbrlEnd) {
	int at;

	for (at = *pre_src; at <= *src; at++)
		insertEmphasesAt(at, table, src, dest, srcmax, destmax, mode, currentInput,
				currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
				transNoteBuffer, haveEmphasis, transOpcode, transRule, inputPositions,
				outputPositions, cursorPosition, cursorStatus, compbrlStart, compbrlEnd);

	*pre_src = *src + 1;
}

static void
checkNumericMode(const TranslationTableHeader *table, int *src, int *dest, int srcmax,
		int destmax, int mode, const widechar *currentInput, widechar *currentOutput,
		int *srcMapping, int *prevSrcMapping, unsigned int *emphasisBuffer,
		unsigned int *transNoteBuffer, const TranslationTableRule **transRule,
		int *inputPositions, int *outputPositions, int *cursorPosition, int *cursorStatus,
		int *dontContract, int compbrlStart, int compbrlEnd, int *numericMode) {
	int i;
	const TranslationTableRule *indicRule;
	if (!brailleIndicatorDefined(table->numberSign, table, &indicRule)) return;

	/* not in numeric mode */
	if (!*numericMode) {
		if (checkAttr(currentInput[*src], CTC_Digit | CTC_LitDigit, 0, table)) {
			*numericMode = 1;
			*dontContract = 1;
			for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen, 0, table,
					src, dest, srcmax, destmax, mode, currentInput, currentOutput,
					srcMapping, prevSrcMapping, emphasisBuffer, transNoteBuffer,
					transRule, inputPositions, outputPositions, cursorPosition,
					cursorStatus, compbrlStart, compbrlEnd);
		} else if (checkAttr(currentInput[*src], CTC_NumericMode, 0, table)) {
			for (i = *src + 1; i < srcmax; i++) {
				if (checkAttr(currentInput[i], CTC_Digit | CTC_LitDigit, 0, table)) {
					*numericMode = 1;
					for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen,
							0, table, src, dest, srcmax, destmax, mode, currentInput,
							currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
							transNoteBuffer, transRule, inputPositions, outputPositions,
							cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
					break;
				} else if (!checkAttr(currentInput[i], CTC_NumericMode, 0, table))
					break;
			}
		}
	}

	/* in numeric mode */
	else {
		if (!checkAttr(currentInput[*src], CTC_Digit | CTC_LitDigit | CTC_NumericMode, 0,
					table)) {
			*numericMode = 0;
			if (brailleIndicatorDefined(table->noContractSign, table, &indicRule))
				if (checkAttr(currentInput[*src], CTC_NumericNoContract, 0, table))
					for_updatePositions(&indicRule->charsdots[0], 0, indicRule->dotslen,
							0, table, src, dest, srcmax, destmax, mode, currentInput,
							currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
							transNoteBuffer, transRule, inputPositions, outputPositions,
							cursorPosition, cursorStatus, compbrlStart, compbrlEnd);
		}
	}
}

static int
translateString(const TranslationTableHeader *table, int *dest, int *srcmax, int destmax,
		int mode, int currentPass, const widechar *currentInput, widechar *currentOutput,
		int *srcMapping, int *prevSrcMapping, formtype *typebuf,
		unsigned char *srcSpacing, unsigned char *destSpacing, unsigned int *wordBuffer,
		unsigned int *emphasisBuffer, unsigned int *transNoteBuffer, int haveEmphasis,
		int *realInlen, int *srcIncremented, int *inputPositions, int *outputPositions,
		int *cursorPosition, int *cursorStatus, int compbrlStart, int compbrlEnd) {
	int src;
	int transOpcode;
	int prevTransOpcode;
	const TranslationTableRule *transRule;
	int transCharslen;
	int passCharDots;
	int passSrc;
	const widechar *passInstructions;
	int passIC; /* Instruction counter */
	int startMatch;
	int startReplace;
	int endReplace;
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
	int prevSrc;
	/* Main translation routine */
	int k;
	translation_direction = 1;
	markSyllables(table, *srcmax, currentInput, typebuf, &transOpcode, &transRule,
			&transCharslen);
	numericMode = 0;
	srcword = 0;
	destword = 0; /* last word translated */
	dontContract = 0;
	prevTransOpcode = CTO_None;
	prevType = curType = prevTypeform = plain_text;
	prevSrc = -1;
	src = *dest = 0;
	*srcIncremented = 1;
	pre_src = 0;
	_lou_resetPassVariables();
	if (typebuf && table->emphRules[capsRule][letterOffset])
		for (k = 0; k < *srcmax; k++)
			if (checkAttr(currentInput[k], CTC_UpperCase, 0, table))
				typebuf[k] |= CAPSEMPH;

	markEmphases(table, *srcmax, currentInput, typebuf, wordBuffer, emphasisBuffer,
			transNoteBuffer, haveEmphasis, inputPositions, outputPositions,
			cursorPosition, cursorStatus, compbrlStart, compbrlEnd);

	while (src < *srcmax) { /* the main translation loop */
		TranslationTableCharacterAttributes beforeAttributes;
		setBefore(table, src, currentInput, &beforeAttributes);
		if (!insertBrailleIndicators(0, table, &src, dest, *srcmax, destmax, mode,
					currentInput, currentOutput, srcMapping, prevSrcMapping, typebuf,
					haveEmphasis, emphasisBuffer, transNoteBuffer, transOpcode,
					prevTransOpcode, &transRule, inputPositions, outputPositions,
					cursorPosition, cursorStatus, compbrlStart, compbrlEnd,
					beforeAttributes, &prevType, &curType, &prevTypeform, prevSrc))
			goto failure;
		if (src >= *srcmax) break;

		// insertEmphases();
		if (!dontContract) dontContract = typebuf[src] & no_contract;
		if (typebuf[src] & no_translate) {
			widechar c = _lou_getDotsForChar(currentInput[src]);
			if (currentInput[src] < 32 || currentInput[src] > 126) goto failure;
			if (!for_updatePositions(&c, 1, 1, 0, table, &src, dest, *srcmax, destmax,
						mode, currentInput, currentOutput, srcMapping, prevSrcMapping,
						emphasisBuffer, transNoteBuffer, &transRule, inputPositions,
						outputPositions, cursorPosition, cursorStatus, compbrlStart,
						compbrlEnd))
				goto failure;
			src++;
			/* because we don't call insertEmphasis */
			pre_src = src;
			continue;
		}
		for_selectRule(table, src, *dest, *srcmax, destmax, mode, currentInput, typebuf,
				emphasisBuffer, transNoteBuffer, &transOpcode, prevTransOpcode,
				&transRule, &transCharslen, &passCharDots, &passSrc, &passInstructions,
				&passIC, &startMatch, &startReplace, &endReplace, *srcIncremented,
				*cursorPosition, &repwordStart, &repwordLength, dontContract,
				compbrlStart, compbrlEnd, beforeAttributes, &curCharDef, &groupingRule,
				&groupingOp);

		if (transOpcode != CTO_Context)
			if (appliedRules != NULL && appliedRulesCount < maxAppliedRules)
				appliedRules[appliedRulesCount++] = transRule;
		*srcIncremented = 1;
		prevSrc = src;
		switch (transOpcode) /* Rules that pre-empt context and swap */
		{
		case CTO_CompBrl:
		case CTO_Literal:
			if (!doCompbrl(table, &src, dest, *srcmax, destmax, mode, currentInput,
						currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
						transNoteBuffer, &transRule, inputPositions, outputPositions,
						cursorPosition, cursorStatus, destword, srcword, compbrlStart,
						compbrlEnd))
				goto failure;
			continue;
		default:
			break;
		}
		if (!insertBrailleIndicators(1, table, &src, dest, *srcmax, destmax, mode,
					currentInput, currentOutput, srcMapping, prevSrcMapping, typebuf,
					haveEmphasis, emphasisBuffer, transNoteBuffer, transOpcode,
					prevTransOpcode, &transRule, inputPositions, outputPositions,
					cursorPosition, cursorStatus, compbrlStart, compbrlEnd,
					beforeAttributes, &prevType, &curType, &prevTypeform, prevSrc))
			goto failure;

		//		if(transOpcode == CTO_Contraction)
		//		if(brailleIndicatorDefined(table->noContractSign))
		//		if(!for_updatePositions(
		//			&indicRule->charsdots[0], 0, indicRule->dotslen, 0))
		//			goto failure;
		insertEmphases(table, &src, dest, *srcmax, destmax, mode, currentInput,
				currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
				transNoteBuffer, haveEmphasis, transOpcode, &transRule, inputPositions,
				outputPositions, cursorPosition, cursorStatus, &pre_src, compbrlStart,
				compbrlEnd);
		if (table->usesNumericMode)
			checkNumericMode(table, &src, dest, *srcmax, destmax, mode, currentInput,
					currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
					transNoteBuffer, &transRule, inputPositions, outputPositions,
					cursorPosition, cursorStatus, &dontContract, compbrlStart, compbrlEnd,
					&numericMode);

		if (transOpcode == CTO_Context ||
				findForPassRule(table, src, *srcmax, currentPass, currentInput,
						&transOpcode, &transRule, &transCharslen, &passCharDots, &passSrc,
						&passInstructions, &passIC, &startMatch, &startReplace,
						&endReplace, &groupingRule, &groupingOp))
			switch (transOpcode) {
			case CTO_Context:
				if (appliedRules != NULL && appliedRulesCount < maxAppliedRules)
					appliedRules[appliedRulesCount++] = transRule;
				if (!passDoAction(table, &src, dest, srcmax, destmax, mode, currentInput,
							currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
							transNoteBuffer, transOpcode, &transRule, passCharDots,
							passSrc, passInstructions, startMatch, startReplace, &passIC,
							&endReplace, inputPositions, outputPositions, cursorPosition,
							cursorStatus, compbrlStart, compbrlEnd, groupingRule,
							groupingOp))
					goto failure;
				if (endReplace == src) *srcIncremented = 0;
				src = endReplace;
				continue;
			default:
				break;
			}

		/* Processing before replacement */

		/* check if leaving no contraction (grade 1) mode */
		if (checkAttr(currentInput[src], CTC_SeqDelimiter | CTC_Space, 0, table))
			dontContract = 0;

		switch (transOpcode) {
		case CTO_EndNum:
			if (table->letterSign && checkAttr(currentInput[src], CTC_Letter, 0, table))
				(*dest)--;
			break;
		case CTO_Repeated:
		case CTO_Space:
			dontContract = 0;
			break;
		case CTO_LargeSign:
			if (prevTransOpcode == CTO_LargeSign) {
				int hasEndSegment = 0;
				while (*dest > 0 &&
						checkAttr(currentOutput[*dest - 1], CTC_Space, 1, table)) {
					if (currentOutput[*dest - 1] == ENDSEGMENT) {
						hasEndSegment = 1;
					}
					(*dest)--;
				}
				if (hasEndSegment != 0) {
					currentOutput[*dest] = 0xffff;
					(*dest)++;
				}
			}
			break;
		case CTO_DecPoint:
			if (!table->usesNumericMode && table->numberSign) {
				TranslationTableRule *numRule =
						(TranslationTableRule *)&table->ruleArea[table->numberSign];
				if (!for_updatePositions(&numRule->charsdots[numRule->charslen],
							numRule->charslen, numRule->dotslen, 0, table, &src, dest,
							*srcmax, destmax, mode, currentInput, currentOutput,
							srcMapping, prevSrcMapping, emphasisBuffer, transNoteBuffer,
							&transRule, inputPositions, outputPositions, cursorPosition,
							cursorStatus, compbrlStart, compbrlEnd))
					goto failure;
			}
			transOpcode = CTO_MidNum;
			break;
		case CTO_NoCont:
			if (!dontContract)
				doNocont(table, &src, dest, mode, currentInput, destword, srcword,
						&dontContract);
			continue;
		default:
			break;
		} /* end of action */

		/* replacement processing */
		switch (transOpcode) {
		case CTO_Replace:
			src += transCharslen;
			if (!putCharacters(&transRule->charsdots[transCharslen], transRule->dotslen,
						table, &src, dest, *srcmax, destmax, mode, currentInput,
						currentOutput, srcMapping, prevSrcMapping, emphasisBuffer,
						transNoteBuffer, &transRule, inputPositions, outputPositions,
						cursorPosition, cursorStatus, compbrlStart, compbrlEnd))
				goto failure;
			break;
		case CTO_None:
			if (!undefinedCharacter(currentInput[src], table, &src, dest, *srcmax,
						destmax, mode, currentInput, currentOutput, srcMapping,
						prevSrcMapping, emphasisBuffer, transNoteBuffer, &transRule,
						inputPositions, outputPositions, cursorPosition, cursorStatus,
						compbrlStart, compbrlEnd))
				goto failure;
			src++;
			break;
		case CTO_UpperCase:
			/* Only needs special handling if not within compbrl and
			 * the table defines a capital sign. */
			if (!(mode & (compbrlAtCursor | compbrlLeftCursor) && src >= compbrlStart &&
						src <= compbrlEnd) &&
					(transRule->dotslen == 1 &&
							table->emphRules[capsRule][letterOffset])) {
				putCharacter(curCharDef->lowercase, table, &src, dest, *srcmax, destmax,
						mode, currentInput, currentOutput, srcMapping, prevSrcMapping,
						emphasisBuffer, transNoteBuffer, &transRule, inputPositions,
						outputPositions, cursorPosition, cursorStatus, compbrlStart,
						compbrlEnd);
				src++;
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
								transCharslen, transRule->dotslen, 0, table, &src, dest,
								*srcmax, destmax, mode, currentInput, currentOutput,
								srcMapping, prevSrcMapping, emphasisBuffer,
								transNoteBuffer, &transRule, inputPositions,
								outputPositions, cursorPosition, cursorStatus,
								compbrlStart, compbrlEnd))
						goto failure;
				} else {
					for (k = 0; k < transCharslen; k++) {
						if (!putCharacter(currentInput[src], table, &src, dest, *srcmax,
									destmax, mode, currentInput, currentOutput,
									srcMapping, prevSrcMapping, emphasisBuffer,
									transNoteBuffer, &transRule, inputPositions,
									outputPositions, cursorPosition, cursorStatus,
									compbrlStart, compbrlEnd))
							goto failure;
						src++;
					}
				}
				if (*cursorStatus == 2)
					*cursorStatus = 1;
				else if (transRule->dotslen)
					src += transCharslen;
			}
			break;
		}

		/* processing after replacement */
		switch (transOpcode) {
		case CTO_Repeated: {
			/* Skip repeated characters. */
			int srclim = *srcmax - transCharslen;
			if (mode & (compbrlAtCursor | compbrlLeftCursor) && compbrlStart < srclim)
				/* Don't skip characters from compbrlStart onwards. */
				srclim = compbrlStart - 1;
			while ((src <= srclim) &&
					compareChars(&transRule->charsdots[0], &currentInput[src],
							transCharslen, 0, table)) {
				/* Map skipped input positions to the previous output position. */
				if (outputPositions != NULL) {
					int tcc;
					for (tcc = 0; tcc < transCharslen; tcc++)
						outputPositions[prevSrcMapping[src + tcc]] = *dest - 1;
				}
				if (!*cursorStatus && src <= *cursorPosition &&
						*cursorPosition < src + transCharslen) {
					*cursorStatus = 1;
					*cursorPosition = *dest - 1;
				}
				src += transCharslen;
			}
			break;
		}
		case CTO_RepWord: {
			/* Skip repeated characters. */
			int srclim = *srcmax - transCharslen;
			if (mode & (compbrlAtCursor | compbrlLeftCursor) && compbrlStart < srclim)
				/* Don't skip characters from compbrlStart onwards. */
				srclim = compbrlStart - 1;
			while ((src <= srclim) && compareChars(repwordStart, &currentInput[src],
											  repwordLength, 0, table)) {
				/* Map skipped input positions to the previous output position. */
				if (outputPositions != NULL) {
					int tcc;
					for (tcc = 0; tcc < transCharslen; tcc++)
						outputPositions[prevSrcMapping[src + tcc]] = *dest - 1;
				}
				if (!*cursorStatus && src <= *cursorPosition &&
						*cursorPosition < src + transCharslen) {
					*cursorStatus = 1;
					*cursorPosition = *dest - 1;
				}
				src += repwordLength + transCharslen;
			}
			src -= transCharslen;
			break;
		}
		case CTO_JoinNum:
		case CTO_JoinableWord:
			while (src < *srcmax && checkAttr(currentInput[src], CTC_Space, 0, table) &&
					currentInput[src] != ENDSEGMENT)
				src++;
			break;
		default:
			break;
		}
		if (((src > 0) && checkAttr(currentInput[src - 1], CTC_Space, 0, table) &&
					(transOpcode != CTO_JoinableWord))) {
			srcword = src;
			destword = *dest;
		}
		if (srcSpacing != NULL && srcSpacing[src] >= '0' && srcSpacing[src] <= '9')
			destSpacing[*dest] = srcSpacing[src];
		if ((transOpcode >= CTO_Always && transOpcode <= CTO_None) ||
				(transOpcode >= CTO_Digit && transOpcode <= CTO_LitDigit))
			prevTransOpcode = transOpcode;
	}

	transOpcode = CTO_Space;
	insertEmphases(table, &src, dest, *srcmax, destmax, mode, currentInput, currentOutput,
			srcMapping, prevSrcMapping, emphasisBuffer, transNoteBuffer, haveEmphasis,
			transOpcode, &transRule, inputPositions, outputPositions, cursorPosition,
			cursorStatus, &pre_src, compbrlStart, compbrlEnd);

failure:
	if (destword != 0 && src < *srcmax &&
			!checkAttr(currentInput[src], CTC_Space, 0, table)) {
		src = srcword;
		*dest = destword;
	}
	if (src < *srcmax) {
		while (checkAttr(currentInput[src], CTC_Space, 0, table))
			if (++src == *srcmax) break;
	}
	if (realInlen) *realInlen = src;
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
