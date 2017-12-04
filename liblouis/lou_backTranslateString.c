/* liblouis Braille Translation and Back-Translation Library

   Based on the Linux screenreader BRLTTY, copyright (C) 1999-2006 by The
   BRLTTY Team

   Copyright (C) 2004, 2005, 2006 ViewPlus Technologies, Inc. www.viewplus.com
   Copyright (C) 2004, 2005, 2006 JJB Software, Inc. www.jjb-software.com

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
 * @brief Translate from braille
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"

static int
backTranslateString(const TranslationTableHeader *table, int *src, int srcmax, int *dest,
		int destmax, int mode, int currentPass, const widechar *currentInput,
		widechar *currentOutput, char *spacebuf, int *srcMapping, int *outputPositions,
		int *inputPositions, int *cursorPosition, int *cursorStatus,
		const TranslationTableRule **appliedRules, int *appliedRulesCount,
		int maxAppliedRules);
static int
makeCorrections(const TranslationTableHeader *table, int *src, int srcmax, int *dest,
		int destmax, int mode, int currentPass, const widechar *currentInput,
		widechar *currentOutput, int *srcMapping, int *outputPositions,
		int *inputPositions, int *cursorPosition, int *cursorStatus,
		const TranslationTableRule **appliedRules, int *appliedRulesCount,
		int maxAppliedRules);
static int
translatePass(const TranslationTableHeader *table, int *src, int srcmax, int *dest,
		int destmax, int mode, int currentPass, const widechar *currentInput,
		widechar *currentOutput, int *srcMapping, int *outputPositions,
		int *inputPositions, int *cursorPosition, int *cursorStatus,
		const TranslationTableRule **appliedRules, int *appliedRulesCount,
		int maxAppliedRules);
static void
passSelectRule(const TranslationTableHeader *table, int src, int srcmax, int currentPass,
		const widechar *currentInput, TranslationTableOpcode *currentOpcode,
		const TranslationTableRule **currentRule, int *passSrc,
		const widechar **passInstructions, int *passIC, int *startMatch,
		int *startReplace, int *endReplace);

int EXPORT_CALL
lou_backTranslateString(const char *tableList, const widechar *inbuf, int *inlen,
		widechar *outbuf, int *outlen, formtype *typeform, char *spacing, int modex) {
	return lou_backTranslate(tableList, inbuf, inlen, outbuf, outlen, typeform, spacing,
			NULL, NULL, NULL, modex);
}

int EXPORT_CALL
lou_backTranslate(const char *tableList, const widechar *inbuf, int *inlen,
		widechar *outbuf, int *outlen, formtype *typeform, char *spacing, int *outputPos,
		int *inputPos, int *cursorPos, int modex) {
	return _lou_backTranslateWithTracing(tableList, inbuf, inlen, outbuf, outlen,
			typeform, spacing, outputPos, inputPos, cursorPos, modex, NULL, NULL);
}

static int
doPasses(widechar *outbuf, const TranslationTableHeader *table, int *src, int *srcmax,
		int *dest, int destmax, int mode, const widechar *currentInput,
		widechar *passbuf1, widechar *passbuf2, char *spacebuf, int *srcMapping,
		int *outputPositions, int *inputPositions, int *cursorPosition, int *cursorStatus,
		const TranslationTableRule **appliedRules, int *appliedRulesCount,
		int maxAppliedRules) {
	int currentPass;
	widechar *currentOutput;
	int firstPass = table->numPasses;
	int lastPass = 1;

	if (table->corrections) lastPass -= 1;

	if (mode & pass1Only) {
		_lou_logMessage(LOG_WARN, "warning: pass1Only mode has been deprecated.");
		firstPass = 1;
		lastPass = 1;
	}

	currentPass = firstPass;

	while (1) {
		currentOutput = (currentPass == lastPass)
				? outbuf
				: (currentInput == passbuf1) ? passbuf2 : passbuf1;

		switch (currentPass) {
		case 1:
			if (!backTranslateString(table, src, *srcmax, dest, destmax, mode,
						currentPass, currentInput, currentOutput, spacebuf, srcMapping,
						outputPositions, inputPositions, cursorPosition, cursorStatus,
						appliedRules, appliedRulesCount, maxAppliedRules))
				return 0;
			break;

		case 0:
			if (!makeCorrections(table, src, *srcmax, dest, destmax, mode, currentPass,
						currentInput, currentOutput, srcMapping, outputPositions,
						inputPositions, cursorPosition, cursorStatus, appliedRules,
						appliedRulesCount, maxAppliedRules))
				return 0;
			break;

		default:
			if (!translatePass(table, src, *srcmax, dest, destmax, mode, currentPass,
						currentInput, currentOutput, srcMapping, outputPositions,
						inputPositions, cursorPosition, cursorStatus, appliedRules,
						appliedRulesCount, maxAppliedRules))
				return 0;
			break;
		}

		if (currentPass == lastPass) return 1;

		currentInput = currentOutput;
		*srcmax = *dest;
		currentPass -= 1;
	}
}

int EXPORT_CALL
_lou_backTranslateWithTracing(const char *tableList, const widechar *inbuf, int *inlen,
		widechar *outbuf, int *outlen, formtype *typeform, char *spacing, int *outputPos,
		int *inputPos, int *cursorPos, int modex, const TranslationTableRule **rules,
		int *rulesLen) {
	int srcmax;
	int src;
	int dest;
	int destmax;
	int mode;
	widechar *passbuf1 = NULL;
	widechar *passbuf2 = NULL;
	unsigned char *typebuf = NULL;
	char *spacebuf;
	int *srcMapping = NULL;
	int *outputPositions;
	int *inputPositions;
	int cursorPosition;
	int cursorStatus;
	const TranslationTableRule **appliedRules;
	int maxAppliedRules;
	int appliedRulesCount;
	int k;
	int goodTrans = 1;
	if (tableList == NULL || inbuf == NULL || inlen == NULL || outbuf == NULL ||
			outlen == NULL)
		return 0;
	const TranslationTableHeader *table = lou_getTable(tableList);
	if (table == NULL) return 0;
	srcmax = 0;
	while (srcmax < *inlen && inbuf[srcmax]) srcmax++;
	destmax = *outlen;
	typebuf = (unsigned char *)typeform;
	spacebuf = spacing;
	outputPositions = outputPos;
	if (outputPos != NULL)
		for (k = 0; k < srcmax; k++) outputPos[k] = -1;
	inputPositions = inputPos;
	if (cursorPos != NULL)
		cursorPosition = *cursorPos;
	else
		cursorPosition = -1;
	cursorStatus = 0;
	mode = modex;
	if (!(passbuf1 = _lou_allocMem(alloc_passbuf1, srcmax, destmax))) return 0;
	if (typebuf != NULL) memset(typebuf, '0', destmax);
	if (spacebuf != NULL) memset(spacebuf, '*', destmax);
	for (k = 0; k < srcmax; k++)
		if ((mode & dotsIO))
			passbuf1[k] = inbuf[k] | 0x8000;
		else
			passbuf1[k] = _lou_getDotsForChar(inbuf[k]);
	passbuf1[srcmax] = _lou_getDotsForChar(' ');
	if (!(srcMapping = _lou_allocMem(alloc_posMapping1, srcmax, destmax))) return 0;
	for (k = 0; k <= srcmax; k++) srcMapping[k] = k;
	srcMapping[srcmax] = srcmax;
	if ((!(mode & pass1Only)) && (table->numPasses > 1 || table->corrections)) {
		if (!(passbuf2 = _lou_allocMem(alloc_passbuf2, srcmax, destmax))) return 0;
	}
	appliedRulesCount = 0;
	if (rules != NULL && rulesLen != NULL) {
		appliedRules = rules;
		maxAppliedRules = *rulesLen;
	} else {
		appliedRules = NULL;
		maxAppliedRules = 0;
	}
	goodTrans = doPasses(outbuf, table, &src, &srcmax, &dest, destmax, mode, passbuf1,
			passbuf1, passbuf2, spacebuf, srcMapping, outputPositions, inputPositions,
			&cursorPosition, &cursorStatus, appliedRules, &appliedRulesCount,
			maxAppliedRules);
	if (src < *inlen) *inlen = srcMapping[src];
	*outlen = dest;
	if (outputPos != NULL) {
		int lastpos = 0;
		for (k = 0; k < *inlen; k++)
			if (outputPos[k] == -1)
				outputPos[k] = lastpos;
			else
				lastpos = outputPos[k];
	}
	if (cursorPos != NULL) *cursorPos = cursorPosition;
	if (rulesLen != NULL) *rulesLen = appliedRulesCount;
	return goodTrans;
}

static TranslationTableCharacter *
back_findCharOrDots(widechar c, int m, const TranslationTableHeader *table) {
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
		preva = (back_findCharOrDots(c, m, table))->attributes;
		prevc = c;
	}
	return ((preva & a) ? 1 : 0);
}

static int
compareDots(const widechar *address1, const widechar *address2, int count) {
	int k;
	if (!count) return 0;
	for (k = 0; k < count; k++)
		if (address1[k] != address2[k]) return 0;
	return 1;
}

static void
back_setBefore(const TranslationTableHeader *table, int dest, widechar *currentOutput,
		TranslationTableCharacterAttributes *beforeAttributes) {
	widechar before = (dest == 0) ? ' ' : currentOutput[dest - 1];
	*beforeAttributes = (back_findCharOrDots(before, 0, table))->attributes;
}

static void
back_setAfter(int length, const TranslationTableHeader *table, int src, int srcmax,
		const widechar *currentInput,
		TranslationTableCharacterAttributes *afterAttributes) {
	widechar after = (src + length < srcmax) ? currentInput[src + length] : ' ';
	*afterAttributes = (back_findCharOrDots(after, 1, table))->attributes;
}

static int
isBegWord(const TranslationTableHeader *table, int dest, widechar *currentOutput) {
	/* See if this is really the beginning of a word. Look at what has
	 * already been translated. */
	int k;
	if (dest == 0) return 1;
	for (k = dest - 1; k >= 0; k--) {
		const TranslationTableCharacter *ch =
				back_findCharOrDots(currentOutput[k], 0, table);
		if (ch->attributes & CTC_Space) break;
		if (ch->attributes & (CTC_Letter | CTC_Digit | CTC_Math | CTC_Sign)) return 0;
	}
	return 1;
}

static int
isEndWord(const TranslationTableHeader *table, int src, int srcmax, int mode,
		const widechar *currentInput, int currentDotslen) {
	if (mode & partialTrans) return 0;
	/* See if this is really the end of a word. */
	int k;
	const TranslationTableCharacter *dots;
	TranslationTableOffset testRuleOffset;
	TranslationTableRule *testRule;
	for (k = src + currentDotslen; k < srcmax; k++) {
		int postpuncFound = 0;
		int TranslationFound = 0;
		dots = back_findCharOrDots(currentInput[k], 1, table);
		testRuleOffset = dots->otherRules;
		if (dots->attributes & CTC_Space) break;
		if (dots->attributes & CTC_Letter) return 0;
		while (testRuleOffset) {
			testRule = (TranslationTableRule *)&table->ruleArea[testRuleOffset];
			/* #360: Don't treat begword/midword as definite translations here
			 * because we don't know whether they apply yet. Subsequent
			 * input will allow us to determine whether the word continues.
			 */
			if (testRule->charslen > 1 && testRule->opcode != CTO_BegWord &&
					testRule->opcode != CTO_MidWord)
				TranslationFound = 1;
			if (testRule->opcode == CTO_PostPunc) postpuncFound = 1;
			if (testRule->opcode == CTO_Hyphen) return 1;
			testRuleOffset = testRule->dotsnext;
		}
		if (TranslationFound && !postpuncFound) return 0;
	}
	return 1;
}
static int
findBrailleIndicatorRule(TranslationTableOffset offset,
		const TranslationTableHeader *table, int *currentDotslen,
		TranslationTableOpcode *currentOpcode, const TranslationTableRule **currentRule) {
	if (!offset) return 0;
	*currentRule = (TranslationTableRule *)&table->ruleArea[offset];
	*currentOpcode = (*currentRule)->opcode;
	*currentDotslen = (*currentRule)->dotslen;
	return 1;
}

static int
handleMultind(const TranslationTableHeader *table, int *currentDotslen,
		TranslationTableOpcode *currentOpcode, const TranslationTableRule **currentRule,
		int *doingMultind, const TranslationTableRule *multindRule) {
	/* Handle multille braille indicators */
	int found = 0;
	if (!*doingMultind) return 0;
	switch (multindRule->charsdots[multindRule->charslen - *doingMultind]) {
	case CTO_CapsLetterRule:  // FIXME: make sure this works
		found = findBrailleIndicatorRule(table->emphRules[capsRule][letterOffset], table,
				currentDotslen, currentOpcode, currentRule);
		break;
	// NOTE:  following fixme is based on the names at the time of
	//        commit f22f91eb510cb4eef33dfb4950a297235dd2f9f1.
	// FIXME: the next two opcodes were begcaps/endcaps,
	//        and they were aliased to opcodes capsword/capswordstop.
	//        However, the table attributes they use are
	//        table->beginCapitalSign and table->endCapitalSign.
	//        These are actually compiled with firstlettercaps/lastlettercaps.
	//        Which to use here?
	case CTO_BegCapsWordRule:
		found = findBrailleIndicatorRule(table->emphRules[capsRule][begWordOffset], table,
				currentDotslen, currentOpcode, currentRule);
		break;
	case CTO_EndCapsWordRule:
		found = findBrailleIndicatorRule(table->emphRules[capsRule][endWordOffset], table,
				currentDotslen, currentOpcode, currentRule);
		break;
	case CTO_LetterSign:
		found = findBrailleIndicatorRule(
				table->letterSign, table, currentDotslen, currentOpcode, currentRule);
		break;
	case CTO_NoContractSign:
		found = findBrailleIndicatorRule(
				table->noContractSign, table, currentDotslen, currentOpcode, currentRule);
		break;
	case CTO_NumberSign:
		found = findBrailleIndicatorRule(
				table->numberSign, table, currentDotslen, currentOpcode, currentRule);
		break;
	case CTO_EndEmph1PhraseBeforeRule:
		found = findBrailleIndicatorRule(
				table->emphRules[emph1Rule][endPhraseBeforeOffset], table, currentDotslen,
				currentOpcode, currentRule);
		break;
	case CTO_BegEmph1Rule:
		found = findBrailleIndicatorRule(table->emphRules[emph1Rule][begOffset], table,
				currentDotslen, currentOpcode, currentRule);
		break;
	case CTO_EndEmph1Rule:
		found = findBrailleIndicatorRule(table->emphRules[emph1Rule][endOffset], table,
				currentDotslen, currentOpcode, currentRule);
		break;
	case CTO_EndEmph2PhraseBeforeRule:
		found = findBrailleIndicatorRule(
				table->emphRules[emph2Rule][endPhraseBeforeOffset], table, currentDotslen,
				currentOpcode, currentRule);
		break;
	case CTO_BegEmph2Rule:
		found = findBrailleIndicatorRule(table->emphRules[emph2Rule][begOffset], table,
				currentDotslen, currentOpcode, currentRule);
		break;
	case CTO_EndEmph2Rule:
		found = findBrailleIndicatorRule(table->emphRules[emph2Rule][endOffset], table,
				currentDotslen, currentOpcode, currentRule);
		break;
	case CTO_EndEmph3PhraseBeforeRule:
		found = findBrailleIndicatorRule(
				table->emphRules[emph3Rule][endPhraseBeforeOffset], table, currentDotslen,
				currentOpcode, currentRule);
		break;
	case CTO_BegEmph3Rule:
		found = findBrailleIndicatorRule(table->emphRules[emph3Rule][begOffset], table,
				currentDotslen, currentOpcode, currentRule);
		break;
	case CTO_EndEmph3Rule:
		found = findBrailleIndicatorRule(table->emphRules[emph3Rule][endOffset], table,
				currentDotslen, currentOpcode, currentRule);
		break;
	case CTO_BegComp:
		found = findBrailleIndicatorRule(
				table->begComp, table, currentDotslen, currentOpcode, currentRule);
		break;
	case CTO_EndComp:
		found = findBrailleIndicatorRule(
				table->endComp, table, currentDotslen, currentOpcode, currentRule);
		break;
	default:
		found = 0;
		break;
	}
	(*doingMultind)--;
	return found;
}

static int
back_passDoTest(const TranslationTableHeader *table, int src, int srcmax,
		const widechar *currentInput, TranslationTableOpcode currentOpcode,
		const TranslationTableRule *currentRule, int *passSrc,
		const widechar **passInstructions, int *passIC, int *startMatch,
		int *startReplace, int *endReplace);
static int
back_passDoAction(const TranslationTableHeader *table, int src, int srcmax, int *dest,
		int destmax, int mode, const widechar *currentInput, widechar *currentOutput,
		int *srcMapping, int *outputPositions, int *inputPositions, int *cursorPosition,
		int *cursorStatus, int *nextUpper, int allUpper, int allUpperPhrase,
		TranslationTableOpcode currentOpcode, const TranslationTableRule *currentRule,
		int passSrc, const widechar *passInstructions, int passIC, int startMatch,
		int startReplace, int *endReplace);

static int
findBackPassRule(const TranslationTableHeader *table, int src, int srcmax,
		int currentPass, const widechar *currentInput,
		TranslationTableOpcode *currentOpcode, const TranslationTableRule **currentRule,
		int *passSrc, const widechar **passInstructions, int *passIC, int *startMatch,
		int *startReplace, int *endReplace) {
	TranslationTableOffset ruleOffset;
	ruleOffset = table->backPassRules[currentPass];

	while (ruleOffset) {
		*currentRule = (TranslationTableRule *)&table->ruleArea[ruleOffset];
		*currentOpcode = (*currentRule)->opcode;

		switch (*currentOpcode) {
		case CTO_Correct:
			if (currentPass != 0) goto NEXT_RULE;
			break;
		case CTO_Context:
			if (currentPass != 1) goto NEXT_RULE;
			break;
		case CTO_Pass2:
			if (currentPass != 2) goto NEXT_RULE;
			break;
		case CTO_Pass3:
			if (currentPass != 3) goto NEXT_RULE;
			break;
		case CTO_Pass4:
			if (currentPass != 4) goto NEXT_RULE;
			break;
		default:
			goto NEXT_RULE;
		}

		if (back_passDoTest(table, src, srcmax, currentInput, *currentOpcode,
					*currentRule, passSrc, passInstructions, passIC, startMatch,
					startReplace, endReplace))
			return 1;

	NEXT_RULE:
		ruleOffset = (*currentRule)->dotsnext;
	}

	return 0;
}

static void
back_selectRule(const TranslationTableHeader *table, int src, int srcmax, int dest,
		int mode, const widechar *currentInput, widechar *currentOutput, int itsANumber,
		int itsALetter, int *currentDotslen, TranslationTableOpcode *currentOpcode,
		const TranslationTableRule **currentRule, TranslationTableOpcode previousOpcode,
		int *doingMultind, const TranslationTableRule **multindRule,
		TranslationTableCharacterAttributes beforeAttributes, int *passSrc,
		const widechar **passInstructions, int *passIC, int *startMatch,
		int *startReplace, int *endReplace) {
	/* check for valid back-translations */
	int length = srcmax - src;
	TranslationTableOffset ruleOffset = 0;
	static TranslationTableRule pseudoRule = { 0 };
	unsigned long int makeHash = 0;
	const TranslationTableCharacter *dots =
			back_findCharOrDots(currentInput[src], 1, table);
	int tryThis;
	if (handleMultind(table, currentDotslen, currentOpcode, currentRule, doingMultind,
				*multindRule))
		return;
	for (tryThis = 0; tryThis < 3; tryThis++) {
		switch (tryThis) {
		case 0:
			if (length < 2 || (itsANumber && (dots->attributes & CTC_LitDigit))) break;
			/* Hash function optimized for backward translation */
			makeHash = (unsigned long int)dots->realchar << 8;
			makeHash += (unsigned long int)(back_findCharOrDots(
													currentInput[src + 1], 1, table))
								->realchar;
			makeHash %= HASHNUM;
			ruleOffset = table->backRules[makeHash];
			break;
		case 1:
			if (!(length >= 1)) break;
			length = 1;
			ruleOffset = dots->otherRules;
			break;
		case 2: /* No rule found */
			*currentRule = &pseudoRule;
			*currentOpcode = pseudoRule.opcode = CTO_None;
			*currentDotslen = pseudoRule.dotslen = 1;
			pseudoRule.charsdots[0] = currentInput[src];
			pseudoRule.charslen = 0;
			return;
			break;
		}
		while (ruleOffset) {
			const widechar *currentDots;
			*currentRule = (TranslationTableRule *)&table->ruleArea[ruleOffset];
			*currentOpcode = (*currentRule)->opcode;
			if (*currentOpcode == CTO_Context) {
				currentDots = &(*currentRule)->charsdots[0];
				*currentDotslen = (*currentRule)->charslen;
			} else {
				currentDots = &(*currentRule)->charsdots[(*currentRule)->charslen];
				*currentDotslen = (*currentRule)->dotslen;
			}
			if (((*currentDotslen <= length) &&
						compareDots(&currentInput[src], currentDots, *currentDotslen))) {
				TranslationTableCharacterAttributes afterAttributes;
				/* check this rule */
				back_setAfter(*currentDotslen, table, src, srcmax, currentInput,
						&afterAttributes);
				if ((!((*currentRule)->after & ~CTC_EmpMatch) ||
							(beforeAttributes & (*currentRule)->after)) &&
						(!((*currentRule)->before & ~CTC_EmpMatch) ||
								(afterAttributes & (*currentRule)->before))) {
					switch (*currentOpcode) { /* check validity of this Translation */
					case CTO_Context:
						if (back_passDoTest(table, src, srcmax, currentInput,
									*currentOpcode, *currentRule, passSrc,
									passInstructions, passIC, startMatch, startReplace,
									endReplace))
							return;
						break;
					case CTO_Space:
					case CTO_Digit:
					case CTO_Letter:
					case CTO_UpperCase:
					case CTO_LowerCase:
					case CTO_Punctuation:
					case CTO_Math:
					case CTO_Sign:
					case CTO_ExactDots:
					case CTO_NoCross:
					case CTO_Repeated:
					case CTO_Replace:
					case CTO_Hyphen:
						return;
					case CTO_LitDigit:
						if (itsANumber) return;
						break;
					case CTO_CapsLetterRule:
					case CTO_BegCapsRule:
					case CTO_EndCapsRule:
					case CTO_BegCapsWordRule:
					case CTO_EndCapsWordRule:
					case CTO_BegEmph1Rule:
					case CTO_EndEmph1Rule:
					case CTO_BegEmph2Rule:
					case CTO_EndEmph2Rule:
					case CTO_BegEmph3Rule:
					case CTO_EndEmph3Rule:
					case CTO_NumberRule:
					case CTO_BegCompRule:
					case CTO_EndCompRule:
						return;
					case CTO_LetterRule:
					case CTO_NoContractRule:
						// BF: This is just a heuristic test. During forward translation,
						// the
						// nocontractsign is inserted either when in numeric mode and the
						// next
						// character is not numeric (CTC_Digit | CTC_LitDigit |
						// CTC_NumericMode),
						// or when a "contraction" rule is matched and the characters are
						// preceded and followed by space or punctuation (CTC_Space |
						// CTC_Punctuation).
						if (!(beforeAttributes & CTC_Letter) &&
								(afterAttributes & (CTC_Letter | CTC_Sign)))
							return;
						break;
					case CTO_MultInd:
						*doingMultind = *currentDotslen;
						*multindRule = *currentRule;
						if (handleMultind(table, currentDotslen, currentOpcode,
									currentRule, doingMultind, *multindRule))
							return;
						break;
					case CTO_LargeSign:
						return;
					case CTO_WholeWord:
						if (mode & partialTrans) break;
						if (itsALetter || itsANumber) break;
					case CTO_Contraction:
						if ((beforeAttributes & (CTC_Space | CTC_Punctuation)) &&
								((afterAttributes & CTC_Space) ||
										isEndWord(table, src, srcmax, mode, currentInput,
												*currentDotslen)))
							return;
						break;
					case CTO_LowWord:
						if (mode & partialTrans) break;
						if ((beforeAttributes & CTC_Space) &&
								(afterAttributes & CTC_Space) &&
								(previousOpcode != CTO_JoinableWord))
							return;
						break;
					case CTO_JoinNum:
					case CTO_JoinableWord:
						if ((beforeAttributes & (CTC_Space | CTC_Punctuation)) &&
								(!(afterAttributes & CTC_Space) || mode & partialTrans))
							return;
						break;
					case CTO_SuffixableWord:
						if (beforeAttributes & (CTC_Space | CTC_Punctuation)) return;
						break;
					case CTO_PrefixableWord:
						if ((beforeAttributes &
									(CTC_Space | CTC_Letter | CTC_Punctuation)) &&
								isEndWord(table, src, srcmax, mode, currentInput,
										*currentDotslen))
							return;
						break;
					case CTO_BegWord:
						if ((beforeAttributes & (CTC_Space | CTC_Punctuation)) &&
								(!isEndWord(table, src, srcmax, mode, currentInput,
										*currentDotslen)))
							return;
						break;
					case CTO_BegMidWord:
						if ((beforeAttributes &
									(CTC_Letter | CTC_Space | CTC_Punctuation)) &&
								(!isEndWord(table, src, srcmax, mode, currentInput,
										*currentDotslen)))
							return;
						break;
					case CTO_PartWord:
						if (!(beforeAttributes & CTC_LitDigit) &&
								(beforeAttributes & CTC_Letter ||
										!isEndWord(table, src, srcmax, mode, currentInput,
												*currentDotslen)))
							return;
						break;
					case CTO_MidWord:
						if (beforeAttributes & CTC_Letter &&
								!isEndWord(table, src, srcmax, mode, currentInput,
										*currentDotslen))
							return;
						break;
					case CTO_MidEndWord:
						if ((beforeAttributes & CTC_Letter)) return;
						break;
					case CTO_EndWord:
						if ((beforeAttributes & CTC_Letter) &&
								isEndWord(table, src, srcmax, mode, currentInput,
										*currentDotslen))
							return;
						break;
					case CTO_BegNum:
						if (beforeAttributes & (CTC_Space | CTC_Punctuation) &&
								(afterAttributes & (CTC_LitDigit | CTC_Sign)))
							return;
						break;
					case CTO_MidNum:
						if (beforeAttributes & CTC_Digit &&
								afterAttributes & CTC_LitDigit)
							return;
						break;
					case CTO_EndNum:
						if (itsANumber && !(afterAttributes & CTC_LitDigit)) return;
						break;
					case CTO_DecPoint:
						if (afterAttributes & (CTC_Digit | CTC_LitDigit)) return;
						break;
					case CTO_PrePunc:
						if (isBegWord(table, dest, currentOutput)) return;
						break;

					case CTO_PostPunc:
						if (isEndWord(table, src, srcmax, mode, currentInput,
									*currentDotslen))
							return;
						break;
					case CTO_Always:
						if ((beforeAttributes & CTC_LitDigit) &&
								(afterAttributes & CTC_LitDigit) &&
								(*currentRule)->charslen > 1)
							break;
						return;

					case CTO_BackMatch: {
						widechar *patterns, *pattern;

						// if(dontContract || (mode & noContractions))
						//	break;
						// if(checkEmphasisChange(0))
						//	break;

						patterns = (widechar *)&table->ruleArea[(*currentRule)->patterns];

						/* check before pattern */
						pattern = &patterns[1];
						if (!_lou_pattern_check(
									currentInput, src - 1, -1, -1, pattern, table))
							break;

						/* check after pattern */
						pattern = &patterns[patterns[0]];
						if (!_lou_pattern_check(currentInput,
									src + (*currentRule)->dotslen, srcmax, 1, pattern,
									table))
							break;

						return;
					}
					default:
						break;
					}
				}
			} /* Done with checking this rule */
			ruleOffset = (*currentRule)->dotsnext;
		}
	}
}

static int
putchars(const widechar *chars, int count, const TranslationTableHeader *table, int *dest,
		int destmax, widechar *currentOutput, int *nextUpper, int allUpper,
		int allUpperPhrase) {
	int k = 0;
	if (!count || (*dest + count) > destmax) return 0;
	if (*nextUpper) {
		currentOutput[(*dest)++] = (back_findCharOrDots(chars[k++], 0, table))->uppercase;
		*nextUpper = 0;
	}
	if (!allUpper && !allUpperPhrase) {
		memcpy(&currentOutput[*dest], &chars[k], CHARSIZE * (count - k));
		*dest += count - k;
	} else
		for (; k < count; k++)
			currentOutput[(*dest)++] =
					(back_findCharOrDots(chars[k], 0, table))->uppercase;
	return 1;
}

static int
back_updatePositions(const widechar *outChars, int inLength, int outLength,
		const TranslationTableHeader *table, int src, int srcmax, int *dest, int destmax,
		widechar *currentOutput, int *srcMapping, int *outputPositions,
		int *inputPositions, int *cursorPosition, int *cursorStatus, int *nextUpper,
		int allUpper, int allUpperPhrase) {
	int k;
	if ((*dest + outLength) > destmax || (src + inLength) > srcmax) return 0;
	if (!*cursorStatus && *cursorPosition >= src && *cursorPosition < (src + inLength)) {
		*cursorPosition = *dest + outLength / 2;
		*cursorStatus = 1;
	}
	if (inputPositions != NULL || outputPositions != NULL) {
		if (outLength <= inLength) {
			for (k = 0; k < outLength; k++) {
				if (inputPositions != NULL)
					inputPositions[*dest + k] = srcMapping[src + k];
				if (outputPositions != NULL)
					outputPositions[srcMapping[src + k]] = *dest + k;
			}
			for (k = outLength; k < inLength; k++)
				if (outputPositions != NULL)
					outputPositions[srcMapping[src + k]] = *dest + outLength - 1;
		} else {
			for (k = 0; k < inLength; k++) {
				if (inputPositions != NULL)
					inputPositions[*dest + k] = srcMapping[src + k];
				if (outputPositions != NULL)
					outputPositions[srcMapping[src + k]] = *dest + k;
			}
			for (k = inLength; k < outLength; k++)
				if (inputPositions != NULL)
					inputPositions[*dest + k] = srcMapping[src + inLength - 1];
		}
	}
	return putchars(outChars, outLength, table, dest, destmax, currentOutput, nextUpper,
			allUpper, allUpperPhrase);
}

static int
undefinedDots(widechar dots, int *dest, int destmax, int mode, widechar *currentOutput) {
	if (mode & noUndefinedDots) return 1;
	/* Print out dot numbers */
	widechar buffer[20];
	int k = 1;
	buffer[0] = '\\';
	if ((dots & B1)) buffer[k++] = '1';
	if ((dots & B2)) buffer[k++] = '2';
	if ((dots & B3)) buffer[k++] = '3';
	if ((dots & B4)) buffer[k++] = '4';
	if ((dots & B5)) buffer[k++] = '5';
	if ((dots & B6)) buffer[k++] = '6';
	if ((dots & B7)) buffer[k++] = '7';
	if ((dots & B8)) buffer[k++] = '8';
	if ((dots & B9)) buffer[k++] = '9';
	if ((dots & B10)) buffer[k++] = 'A';
	if ((dots & B11)) buffer[k++] = 'B';
	if ((dots & B12)) buffer[k++] = 'C';
	if ((dots & B13)) buffer[k++] = 'D';
	if ((dots & B14)) buffer[k++] = 'E';
	if ((dots & B15)) buffer[k++] = 'F';
	buffer[k++] = '/';
	if ((*dest + k) > destmax) return 0;
	memcpy(&currentOutput[*dest], buffer, k * CHARSIZE);
	*dest += k;
	return 1;
}

static int
putCharacter(widechar dots, const TranslationTableHeader *table, int src, int srcmax,
		int *dest, int destmax, int mode, widechar *currentOutput, int *srcMapping,
		int *outputPositions, int *inputPositions, int *cursorPosition, int *cursorStatus,
		int *nextUpper, int allUpper, int allUpperPhrase) {
	/* Output character(s) corresponding to a Unicode braille Character */
	TranslationTableOffset offset = (back_findCharOrDots(dots, 1, table))->definitionRule;
	if (offset) {
		widechar c;
		const TranslationTableRule *rule =
				(TranslationTableRule *)&table->ruleArea[offset];
		if (rule->charslen)
			return back_updatePositions(&rule->charsdots[0], rule->dotslen,
					rule->charslen, table, src, srcmax, dest, destmax, currentOutput,
					srcMapping, outputPositions, inputPositions, cursorPosition,
					cursorStatus, nextUpper, allUpper, allUpperPhrase);
		c = _lou_getCharFromDots(dots);
		return back_updatePositions(&c, 1, 1, table, src, srcmax, dest, destmax,
				currentOutput, srcMapping, outputPositions, inputPositions,
				cursorPosition, cursorStatus, nextUpper, allUpper, allUpperPhrase);
	}
	return undefinedDots(dots, dest, destmax, mode, currentOutput);
}

static int
putCharacters(const widechar *characters, int count, const TranslationTableHeader *table,
		int src, int srcmax, int *dest, int destmax, int mode, widechar *currentOutput,
		int *srcMapping, int *outputPositions, int *inputPositions, int *cursorPosition,
		int *cursorStatus, int *nextUpper, int allUpper, int allUpperPhrase) {
	int k;
	for (k = 0; k < count; k++)
		if (!putCharacter(characters[k], table, src, srcmax, dest, destmax, mode,
					currentOutput, srcMapping, outputPositions, inputPositions,
					cursorPosition, cursorStatus, nextUpper, allUpper, allUpperPhrase))
			return 0;
	return 1;
}

static int
insertSpace(const TranslationTableHeader *table, int src, int srcmax, int *dest,
		int destmax, widechar *currentOutput, char *spacebuf, int *srcMapping,
		int *outputPositions, int *inputPositions, int *cursorPosition, int *cursorStatus,
		int *nextUpper, int allUpper, int allUpperPhrase) {
	widechar c = ' ';
	if (!back_updatePositions(&c, 1, 1, table, src, srcmax, dest, destmax, currentOutput,
				srcMapping, outputPositions, inputPositions, cursorPosition, cursorStatus,
				nextUpper, allUpper, allUpperPhrase))
		return 0;
	if (spacebuf) spacebuf[*dest - 1] = '1';
	return 1;
}

static int
compareChars(const widechar *address1, const widechar *address2, int count, int m,
		const TranslationTableHeader *table) {
	int k;
	if (!count) return 0;
	for (k = 0; k < count; k++)
		if ((back_findCharOrDots(address1[k], m, table))->lowercase !=
				(back_findCharOrDots(address2[k], m, table))->lowercase)
			return 0;
	return 1;
}

static int
makeCorrections(const TranslationTableHeader *table, int *src, int srcmax, int *dest,
		int destmax, int mode, int currentPass, const widechar *currentInput,
		widechar *currentOutput, int *srcMapping, int *outputPositions,
		int *inputPositions, int *cursorPosition, int *cursorStatus,
		const TranslationTableRule **appliedRules, int *appliedRulesCount,
		int maxAppliedRules) {
	int nextUpper = 0;
	int allUpper = 0;
	int allUpperPhrase = 0;
	if (!table->corrections) return 1;
	*src = 0;
	*dest = 0;
	_lou_resetPassVariables();
	while (*src < srcmax) {
		TranslationTableOpcode currentOpcode;
		const TranslationTableRule *currentRule; /* pointer to current rule in table */
		int passSrc;
		const widechar *passInstructions;
		int passIC; /* Instruction counter */
		int startMatch;
		int startReplace;
		int endReplace;
		int length = srcmax - *src;
		const TranslationTableCharacter *character =
				back_findCharOrDots(currentInput[*src], 0, table);
		const TranslationTableCharacter *character2;
		int tryThis = 0;
		if (!findBackPassRule(table, *src, srcmax, currentPass, currentInput,
					&currentOpcode, &currentRule, &passSrc, &passInstructions, &passIC,
					&startMatch, &startReplace, &endReplace))
			while (tryThis < 3) {
				TranslationTableOffset ruleOffset = 0;
				unsigned long int makeHash = 0;
				switch (tryThis) {
				case 0:
					if (!(length >= 2)) break;
					makeHash = (unsigned long int)character->lowercase << 8;
					character2 = back_findCharOrDots(currentInput[*src + 1], 0, table);
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
					currentOpcode = CTO_Always;
					ruleOffset = 0;
					break;
				}
				while (ruleOffset) {
					currentRule = (TranslationTableRule *)&table->ruleArea[ruleOffset];
					currentOpcode = currentRule->opcode;
					int currentCharslen = currentRule->charslen;
					if (tryThis == 1 || (currentCharslen <= length &&
												compareChars(&currentRule->charsdots[0],
														&currentInput[*src],
														currentCharslen, 0, table))) {
						if (currentOpcode == CTO_Correct &&
								back_passDoTest(table, *src, srcmax, currentInput,
										currentOpcode, currentRule, &passSrc,
										&passInstructions, &passIC, &startMatch,
										&startReplace, &endReplace)) {
							tryThis = 4;
							break;
						}
					}
					ruleOffset = currentRule->dotsnext;
				}
				tryThis++;
			}
		switch (currentOpcode) {
		case CTO_Always:
			if (*dest >= destmax) goto failure;
			srcMapping[*dest] = srcMapping[*src];
			currentOutput[(*dest)++] = currentInput[(*src)++];
			break;
		case CTO_Correct:
			if (appliedRules != NULL && *appliedRulesCount < maxAppliedRules)
				appliedRules[(*appliedRulesCount)++] = currentRule;
			if (!back_passDoAction(table, *src, srcmax, dest, destmax, mode, currentInput,
						currentOutput, srcMapping, outputPositions, inputPositions,
						cursorPosition, cursorStatus, &nextUpper, allUpper,
						allUpperPhrase, currentOpcode, currentRule, passSrc,
						passInstructions, passIC, startMatch, startReplace, &endReplace))
				goto failure;
			*src = endReplace;
			break;
		default:
			break;
		}
	}
failure:
	return 1;
}

static int
backTranslateString(const TranslationTableHeader *table, int *src, int srcmax, int *dest,
		int destmax, int mode, int currentPass, const widechar *currentInput,
		widechar *currentOutput, char *spacebuf, int *srcMapping, int *outputPositions,
		int *inputPositions, int *cursorPosition, int *cursorStatus,
		const TranslationTableRule **appliedRules, int *appliedRulesCount,
		int maxAppliedRules) {
	int nextUpper;
	int allUpper;
	int allUpperPhrase;
	int itsANumber;
	int itsALetter;
	/* Back translation */
	int srcword = 0;
	int destword = 0; /* last word translated */
	TranslationTableOpcode previousOpcode;
	int doingMultind = 0;
	const TranslationTableRule *multindRule;
	_lou_resetPassVariables();
	translation_direction = 0;
	nextUpper = allUpper = allUpperPhrase = itsANumber = itsALetter = 0;
	previousOpcode = CTO_None;
	*src = *dest = 0;
	while (*src < srcmax) {
		/* the main translation loop */
		int currentDotslen; /* length of current find string */
		TranslationTableOpcode currentOpcode;
		const TranslationTableRule *currentRule; /* pointer to current rule in table */
		TranslationTableCharacterAttributes beforeAttributes;
		int passSrc;
		const widechar *passInstructions;
		int passIC; /* Instruction counter */
		int startMatch;
		int startReplace;
		int endReplace;
		back_setBefore(table, *dest, currentOutput, &beforeAttributes);
		back_selectRule(table, *src, srcmax, *dest, mode, currentInput, currentOutput,
				itsANumber, itsALetter, &currentDotslen, &currentOpcode, &currentRule,
				previousOpcode, &doingMultind, &multindRule, beforeAttributes, &passSrc,
				&passInstructions, &passIC, &startMatch, &startReplace, &endReplace);
		if (appliedRules != NULL && *appliedRulesCount < maxAppliedRules)
			appliedRules[(*appliedRulesCount)++] = currentRule;
		/* processing before replacement */
		switch (currentOpcode) {
		case CTO_Hyphen:
			itsANumber = 0;
			break;
		case CTO_LargeSign:
			if (previousOpcode == CTO_LargeSign)
				if (!insertSpace(table, *src, srcmax, dest, destmax, currentOutput,
							spacebuf, srcMapping, outputPositions, inputPositions,
							cursorPosition, cursorStatus, &nextUpper, allUpper,
							allUpperPhrase))
					goto failure;
			break;
		case CTO_CapsLetterRule:
			nextUpper = 1;
			*src += currentDotslen;
			continue;
			break;
		case CTO_BegCapsWordRule:
			allUpper = 1;
			*src += currentDotslen;
			continue;
			break;
		case CTO_BegCapsRule:
			allUpperPhrase = 1;
			*src += currentDotslen;
			continue;
			break;
		case CTO_EndCapsWordRule:
			allUpper = 0;
			*src += currentDotslen;
			continue;
			break;
		case CTO_EndCapsRule:
			allUpperPhrase = 0;
			*src += currentDotslen;
			continue;
			break;
		case CTO_LetterRule:
		case CTO_NoContractRule:
			itsALetter = 1;
			itsANumber = 0;
			*src += currentDotslen;
			continue;
			break;
		case CTO_NumberRule:
			itsANumber = 1;
			*src += currentDotslen;
			continue;
			break;
		case CTO_BegEmph1Rule:
			*src += currentDotslen;
			continue;
			break;
		case CTO_BegEmph2Rule:
			*src += currentDotslen;
			continue;
			break;
		case CTO_BegEmph3Rule:
			*src += currentDotslen;
			continue;
			break;
		case CTO_EndEmph1Rule:
		case CTO_EndEmph2Rule:
		case CTO_EndEmph3Rule:
			*src += currentDotslen;
			continue;
			break;
		case CTO_BegCompRule:
			*src += currentDotslen;
			continue;
			break;
		case CTO_EndCompRule:
			*src += currentDotslen;
			continue;
			break;

		default:
			break;
		}

		/* replacement processing */
		switch (currentOpcode) {
		case CTO_Context:
			if (!back_passDoAction(table, *src, srcmax, dest, destmax, mode, currentInput,
						currentOutput, srcMapping, outputPositions, inputPositions,
						cursorPosition, cursorStatus, &nextUpper, allUpper,
						allUpperPhrase, currentOpcode, currentRule, passSrc,
						passInstructions, passIC, startMatch, startReplace, &endReplace))
				return 0;
			*src = endReplace;
			break;
		case CTO_Replace:
			*src += currentDotslen;
			if (!putCharacters(&currentRule->charsdots[0], currentRule->charslen, table,
						*src, srcmax, dest, destmax, mode, currentOutput, srcMapping,
						outputPositions, inputPositions, cursorPosition, cursorStatus,
						&nextUpper, allUpper, allUpperPhrase))
				goto failure;
			break;
		case CTO_None:
			if (!undefinedDots(currentInput[*src], dest, destmax, mode, currentOutput))
				goto failure;
			(*src)++;
			break;
		case CTO_BegNum:
			itsANumber = 1;
			goto insertChars;
		case CTO_EndNum:
			itsANumber = 0;
			goto insertChars;
		case CTO_Space:
			itsALetter = itsANumber = allUpper = nextUpper = 0;
			goto insertChars;
		default:
		insertChars:
			if (currentRule->charslen) {
				if (!back_updatePositions(&currentRule->charsdots[0],
							currentRule->dotslen, currentRule->charslen, table, *src,
							srcmax, dest, destmax, currentOutput, srcMapping,
							outputPositions, inputPositions, cursorPosition, cursorStatus,
							&nextUpper, allUpper, allUpperPhrase))
					goto failure;
				*src += currentDotslen;
			} else {
				int srclim = *src + currentDotslen;
				while (1) {
					if (!putCharacter(currentInput[*src], table, *src, srcmax, dest,
								destmax, mode, currentOutput, srcMapping, outputPositions,
								inputPositions, cursorPosition, cursorStatus, &nextUpper,
								allUpper, allUpperPhrase))
						goto failure;
					if (++(*src) == srclim) break;
				}
			}
		}

		/* processing after replacement */
		switch (currentOpcode) {
		case CTO_JoinNum:
		case CTO_JoinableWord:
			if (!insertSpace(table, *src, srcmax, dest, destmax, currentOutput, spacebuf,
						srcMapping, outputPositions, inputPositions, cursorPosition,
						cursorStatus, &nextUpper, allUpper, allUpperPhrase))
				goto failure;
			break;
		default:
			passSelectRule(table, *src, srcmax, currentPass, currentInput, &currentOpcode,
					&currentRule, &passSrc, &passInstructions, &passIC, &startMatch,
					&startReplace, &endReplace);
			if (currentOpcode == CTO_Context) {
				back_passDoAction(table, *src, srcmax, dest, destmax, mode, currentInput,
						currentOutput, srcMapping, outputPositions, inputPositions,
						cursorPosition, cursorStatus, &nextUpper, allUpper,
						allUpperPhrase, currentOpcode, currentRule, passSrc,
						passInstructions, passIC, startMatch, startReplace, &endReplace);
				*src = endReplace;
			}
			break;
		}
		if (((*src > 0) && checkAttr(currentInput[*src - 1], CTC_Space, 1, table) &&
					(currentOpcode != CTO_JoinableWord))) {
			srcword = *src;
			destword = *dest;
		}
		if ((currentOpcode >= CTO_Always && currentOpcode <= CTO_None) ||
				(currentOpcode >= CTO_Digit && currentOpcode <= CTO_LitDigit))
			previousOpcode = currentOpcode;
	} /* end of translation loop */
failure:

	if (destword != 0 && *src < srcmax &&
			!checkAttr(currentInput[*src], CTC_Space, 1, table)) {
		*src = srcword;
		*dest = destword;
	}
	if (*src < srcmax) {
		while (checkAttr(currentInput[*src], CTC_Space, 1, table))
			if (++(*src) == srcmax) break;
	}
	return 1;
} /* translation completed */

/* Multipass translation */

static int
matchcurrentInput(const widechar *currentInput, int passSrc,
		const widechar *passInstructions, int passIC) {
	int k;
	int kk = passSrc;
	for (k = passIC + 2; k < passIC + 2 + passInstructions[passIC + 1]; k++)
		if (passInstructions[k] != currentInput[kk++]) return 0;
	return 1;
}

static int
back_swapTest(const TranslationTableHeader *table, const widechar *currentInput,
		int *passSrc, const widechar *passInstructions, int passIC) {
	int curLen;
	int curTest;
	int curSrc = *passSrc;
	TranslationTableOffset swapRuleOffset;
	TranslationTableRule *swapRule;
	swapRuleOffset = (passInstructions[passIC + 1] << 16) | passInstructions[passIC + 2];
	swapRule = (TranslationTableRule *)&table->ruleArea[swapRuleOffset];
	for (curLen = 0; curLen < passInstructions[passIC] + 3; curLen++) {
		for (curTest = 0; curTest < swapRule->charslen; curTest++) {
			if (currentInput[curSrc] == swapRule->charsdots[curTest]) break;
		}
		if (curTest == swapRule->charslen) return 0;
		curSrc++;
	}
	if (passInstructions[passIC + 2] == passInstructions[passIC + 3]) {
		*passSrc = curSrc;
		return 1;
	}
	while (curLen < passInstructions[passIC + 4]) {
		for (curTest = 0; curTest < swapRule->charslen; curTest++) {
			if (currentInput[curSrc] != swapRule->charsdots[curTest]) break;
		}
		if (curTest < swapRule->charslen)
			if (curTest < swapRule->charslen) {
				*passSrc = curSrc;
				return 1;
			}
		curSrc++;
		curLen++;
	}
	*passSrc = curSrc;
	return 1;
}

static int
back_swapReplace(int startSrc, int maxLen, const TranslationTableHeader *table, int *dest,
		int destmax, const widechar *currentInput, widechar *currentOutput,
		int *srcMapping, const widechar *passInstructions, int passIC) {
	TranslationTableOffset swapRuleOffset;
	TranslationTableRule *swapRule;
	widechar *replacements;
	int curRep;
	int curPos;
	int lastPos = 0;
	int lastRep = 0;
	int curTest;
	int curSrc = startSrc;
	swapRuleOffset = (passInstructions[passIC + 1] << 16) | passInstructions[passIC + 2];
	swapRule = (TranslationTableRule *)&table->ruleArea[swapRuleOffset];
	replacements = &swapRule->charsdots[swapRule->charslen];
	while (curSrc < maxLen) {
		for (curTest = 0; curTest < swapRule->charslen; curTest++) {
			if (currentInput[curSrc] == swapRule->charsdots[curTest]) break;
		}
		if (curTest == swapRule->charslen) return curSrc;
		if (curTest >= lastRep) {
			curPos = lastPos;
			curRep = lastRep;
		} else {
			curPos = 0;
			curRep = 0;
		}
		while (curPos < swapRule->dotslen) {
			if (curRep == curTest) {
				int k;
				if ((*dest + replacements[curPos] - 1) >= destmax) return 0;
				for (k = *dest + replacements[curPos] - 2; k >= *dest; --k)
					srcMapping[k] = srcMapping[curSrc];
				memcpy(&currentOutput[*dest], &replacements[curPos + 1],
						(replacements[curPos] - 1) * CHARSIZE);
				*dest += replacements[curPos] - 1;
				lastPos = curPos;
				lastRep = curRep;
				break;
			}
			curRep++;
			curPos += replacements[curPos];
		}
		curSrc++;
	}
	return curSrc;
}

static int
back_passDoTest(const TranslationTableHeader *table, int src, int srcmax,
		const widechar *currentInput, TranslationTableOpcode currentOpcode,
		const TranslationTableRule *currentRule, int *passSrc,
		const widechar **passInstructions, int *passIC, int *startMatch,
		int *startReplace, int *endReplace) {
	int k;
	int m;
	int not = 0;
	TranslationTableCharacterAttributes attributes;
	*passSrc = src;
	*passInstructions = &currentRule->charsdots[currentRule->charslen];
	*passIC = 0;
	*startMatch = *passSrc;
	*startReplace = -1;
	if (currentOpcode == CTO_Correct)
		m = 0;
	else
		m = 1;
	while (*passIC < currentRule->dotslen) {
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
				*passSrc = 0;
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
					matchcurrentInput(currentInput, *passSrc, *passInstructions, *passIC);
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
				if (!(back_findCharOrDots(currentInput[*passSrc], m, table)->attributes &
							attributes)) {
					itsTrue = 0;
					break;
				}
				(*passSrc)++;
			}
			if (itsTrue) {
				for (k = (*passInstructions)[*passIC + 3];
						k < (*passInstructions)[*passIC + 4] && *passSrc < srcmax; k++) {
					if (!(back_findCharOrDots(currentInput[*passSrc], m,
								  table)->attributes &
								attributes))
						break;
					(*passSrc)++;
				}
			}
			*passIC += 5;
			break;
		case pass_swap:
			itsTrue = back_swapTest(
					table, currentInput, passSrc, *passInstructions, *passIC);
			*passIC += 5;
			break;
		case pass_endTest: {
			int endMatch;
			(*passIC)++;
			endMatch = *passSrc;
			if (*startReplace == -1) {
				*startReplace = *startMatch;
				*endReplace = endMatch;
			}
			return 1;
			break;
		}
		default:
			if (_lou_handlePassVariableTest(*passInstructions, passIC, &itsTrue)) break;
			return 0;
		}
		if ((!not&&!itsTrue) || (not&&itsTrue)) return 0;
		not = 0;
	}
	return 1;
}

static int
copyCharacters(int from, int to, const TranslationTableHeader *table, int src, int srcmax,
		int *dest, int destmax, int mode, const widechar *currentInput,
		widechar *currentOutput, int *srcMapping, int *outputPositions,
		int *inputPositions, int *cursorPosition, int *cursorStatus, int *nextUpper,
		int allUpper, int allUpperPhrase, TranslationTableOpcode currentOpcode) {
	if (currentOpcode == CTO_Context) {
		while (from < to)
			if (!putCharacter(currentInput[from++], table, src, srcmax, dest, destmax,
						mode, currentOutput, srcMapping, outputPositions, inputPositions,
						cursorPosition, cursorStatus, nextUpper, allUpper,
						allUpperPhrase))
				return 0;
	} else {
		int count = to - from;

		if (count > 0) {
			if ((*dest + count) > destmax) return 0;

			memmove(&srcMapping[*dest], &srcMapping[from], count * sizeof(*srcMapping));
			memcpy(&currentOutput[*dest], &currentInput[from],
					count * sizeof(*currentOutput));
			*dest += count;
		}
	}

	return 1;
}

static int
back_passDoAction(const TranslationTableHeader *table, int src, int srcmax, int *dest,
		int destmax, int mode, const widechar *currentInput, widechar *currentOutput,
		int *srcMapping, int *outputPositions, int *inputPositions, int *cursorPosition,
		int *cursorStatus, int *nextUpper, int allUpper, int allUpperPhrase,
		TranslationTableOpcode currentOpcode, const TranslationTableRule *currentRule,
		int passSrc, const widechar *passInstructions, int passIC, int startMatch,
		int startReplace, int *endReplace) {
	int k;

	int srcInitial = startMatch;
	int srcStart = startReplace;
	int srcEnd = *endReplace;
	int destInitial = *dest;
	int destStart;

	if (!copyCharacters(srcInitial, srcStart, table, src, srcmax, dest, destmax, mode,
				currentInput, currentOutput, srcMapping, outputPositions, inputPositions,
				cursorPosition, cursorStatus, nextUpper, allUpper, allUpperPhrase,
				currentOpcode))
		return 0;
	destStart = *dest;

	while (passIC < currentRule->dotslen) switch (passInstructions[passIC]) {
		case pass_string:
		case pass_dots:
			if ((*dest + passInstructions[passIC + 1]) > destmax) return 0;
			for (k = 0; k < passInstructions[passIC + 1]; ++k)
				srcMapping[*dest + k] = startMatch;
			memcpy(&currentOutput[*dest], &passInstructions[passIC + 2],
					passInstructions[passIC + 1] * sizeof(*currentOutput));
			*dest += passInstructions[passIC + 1];
			passIC += passInstructions[passIC + 1] + 2;
			break;
		case pass_swap:
			if (!back_swapReplace(startReplace, *endReplace - startReplace, table, dest,
						destmax, currentInput, currentOutput, srcMapping,
						passInstructions, passIC))
				return 0;
			passIC += 3;
			break;
		case pass_omit:
			passIC++;
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

			if (!copyCharacters(srcStart, srcEnd, table, src, srcmax, dest, destmax, mode,
						currentInput, currentOutput, srcMapping, outputPositions,
						inputPositions, cursorPosition, cursorStatus, nextUpper, allUpper,
						allUpperPhrase, currentOpcode))
				return 0;
			*endReplace = passSrc;
			passIC++;
			break;
		default:
			if (_lou_handlePassVariableAction(passInstructions, &passIC)) break;
			return 0;
		}
	return 1;
}

static void
passSelectRule(const TranslationTableHeader *table, int src, int srcmax, int currentPass,
		const widechar *currentInput, TranslationTableOpcode *currentOpcode,
		const TranslationTableRule **currentRule, int *passSrc,
		const widechar **passInstructions, int *passIC, int *startMatch,
		int *startReplace, int *endReplace) {
	if (!findBackPassRule(table, src, srcmax, currentPass, currentInput, currentOpcode,
				currentRule, passSrc, passInstructions, passIC, startMatch, startReplace,
				endReplace)) {
		*currentOpcode = CTO_Always;
	}
}

static int
translatePass(const TranslationTableHeader *table, int *src, int srcmax, int *dest,
		int destmax, int mode, int currentPass, const widechar *currentInput,
		widechar *currentOutput, int *srcMapping, int *outputPositions,
		int *inputPositions, int *cursorPosition, int *cursorStatus,
		const TranslationTableRule **appliedRules, int *appliedRulesCount,
		int maxAppliedRules) {
	int nextUpper = 0;
	int allUpper = 0;
	int allUpperPhrase = 0;
	*src = *dest = 0;
	_lou_resetPassVariables();
	while (*src < srcmax) { /* the main multipass translation loop */
		TranslationTableOpcode currentOpcode;
		const TranslationTableRule *currentRule; /* pointer to current rule in table */
		int passSrc;
		const widechar *passInstructions;
		int passIC; /* Instruction counter */
		int startMatch;
		int startReplace;
		int endReplace;
		passSelectRule(table, *src, srcmax, currentPass, currentInput, &currentOpcode,
				&currentRule, &passSrc, &passInstructions, &passIC, &startMatch,
				&startReplace, &endReplace);
		switch (currentOpcode) {
		case CTO_Pass2:
		case CTO_Pass3:
		case CTO_Pass4:
			if (appliedRules != NULL && *appliedRulesCount < maxAppliedRules)
				appliedRules[(*appliedRulesCount)++] = currentRule;
			if (!back_passDoAction(table, *src, srcmax, dest, destmax, mode, currentInput,
						currentOutput, srcMapping, outputPositions, inputPositions,
						cursorPosition, cursorStatus, &nextUpper, allUpper,
						allUpperPhrase, currentOpcode, currentRule, passSrc,
						passInstructions, passIC, startMatch, startReplace, &endReplace))
				goto failure;
			*src = endReplace;
			break;
		case CTO_Always:
			if ((*dest + 1) > destmax) goto failure;
			srcMapping[*dest] = srcMapping[*src];
			currentOutput[(*dest)++] = currentInput[(*src)++];
			break;
		default:
			goto failure;
		}
	}
	srcMapping[*dest] = srcMapping[*src];
failure:
	if (*src < srcmax) {
		while (checkAttr(currentInput[*src], CTC_Space, 1, table))
			if (++(*src) == srcmax) break;
	}
	return 1;
}
