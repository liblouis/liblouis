/* liblouis Braille Translation and Back-Translation Library

   Based on the Linux screenreader BRLTTY, copyright (C) 1999-2006 by
   The BRLTTY Team

   Copyright (C) 2004, 2005, 2006
   ViewPlus Technologies, Inc. www.viewplus.com
   and
   JJB Software, Inc. www.jjb-software.com
   All rights reserved

   This file is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

In addition to the permissions and restrictions contained in the GNU
General Public License (GPL), the copyright holders grant two explicit
permissions and impose one explicit restriction. The permissions are:

1) Using, copying, merging, publishing, distributing, sublicensing, 
   and/or selling copies of this software that are either compiled or loaded 
   as part of and/or linked into other code is not bound by the GPL.

2) Modifying copies of this software as needed in order to facilitate 
   compiling and/or linking with other code is not bound by the GPL.

The restriction is:

3. The translation, semantic-action and configuration tables that are 
   read at run-time are considered part of this code and are under the terms 
   of the GPL. Any changes to these tables and any additional tables that are 
   created for use by this code must be made publicly available.

All other uses, including modifications not required for compiling or linking 
and distribution of code which is not linked into a combined executable, are 
bound by the terms of the GPL.

   This file is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Maintained by John J. Boyer john.boyer@jjb-software.com
   */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "louis.h"

static const ContractionTableHeader *table;	/*translation table 
						 */
static int src, srcmax;
static int dest, destmax;
static int mode;
static int currentPass = 1;
static const widechar *currentInput;
static widechar *passbuf1 = NULL;
static widechar *passbuf2 = NULL;
static widechar *currentOutput;
static unsigned short *typebuf = NULL;
static unsigned char *srcSpacing = NULL;
static unsigned char *destSpacing = NULL;
static int haveTypeforms = 0;
static int makeCorrections (void);
static int markSyllables (void);
static int translateString (void);
static int for_translatePass (void);

static int *outputPositions;
static int *inputPositions;
static int cursorPosition;
static int cursorStatus = 0;

int
lou_translateString (const char *const trantab, const widechar
		     * const inbufx,
		     int *inlen, widechar * outbuf, int *outlen, char
		     *typeform, char *spacing, int mode)
{
  return lou_translate (trantab, inbufx, inlen, outbuf, outlen, typeform,
			spacing, NULL, NULL, NULL, mode);
}

int
lou_translate (const char *const trantab, const widechar
	       * const inbufx,
	       int *inlen, widechar * outbuf, int *outlen,
	       char *typeform, char *spacing, int *outputPos,
	       int *inputPos, int *cursorPos, int modex)
{
  int k;
  int goodTrans = 1;
  table = lou_getTable (trantab);
  if (table == NULL || *inlen < 0 || *outlen < 0)
    return 0;
  currentInput = (widechar *) inbufx;
  srcmax = 0;
  while (srcmax < *inlen && currentInput[srcmax])
    srcmax++;
  destmax = *outlen;
  haveTypeforms = 0;
  if (!(typeform == NULL || *typeform == 'x') || table->syllables ||
      table->firstWordCaps)
    {
      if (!(typebuf = liblouis_allocMem (alloc_typebuf, srcmax, destmax)))
	return 0;
      if (!(typeform == NULL || *typeform == 'X'))
	{
	  for (k = 0; k < srcmax; k++)
	    if ((typebuf[k] = typeform[k] & 0x0f))
	      haveTypeforms = 1;
	}
      else
	memset (typebuf, 0, srcmax * sizeof (unsigned short));
    }
  if (!(spacing == NULL || *spacing == 'X'))
    srcSpacing = (unsigned char *) spacing;
  outputPositions = outputPos;
  inputPositions = inputPos;
  if (cursorPos != NULL)
    cursorPosition = *cursorPos;
  else
    cursorPosition = -1;
  cursorStatus = 0;
  mode = modex;
  if (!(passbuf1 = liblouis_allocMem (alloc_passbuf1, srcmax, destmax)))
    return 0;
  if (table->numPasses > 1 || table->corrections)
    {
      if (!(passbuf2 = liblouis_allocMem (alloc_passbuf2, srcmax, destmax)))
	return 0;
    }
  if (srcSpacing != NULL)
    {
      if (!(destSpacing = liblouis_allocMem (alloc_destSpacing, srcmax,
					     destmax)))
	goodTrans = 0;
      else
	memset (destSpacing, '*', destmax);
    }
  currentPass = 0;
  while (currentPass <= table->numPasses && goodTrans)
    {
      switch (currentPass)
	{
	case 0:
	  if (table->corrections)
	    {
	      currentOutput = passbuf2;
	      goodTrans = makeCorrections ();
	      currentInput = passbuf2;
	      srcmax = dest;
	    }
	  break;
	case 1:
	  currentOutput = passbuf1;
	  goodTrans = translateString ();
	  break;
	case 2:
	  srcmax = dest;
	  currentInput = passbuf1;
	  currentOutput = passbuf2;
	  goodTrans = for_translatePass ();
	  break;
	case 3:
	  srcmax = dest;
	  currentInput = passbuf2;
	  currentOutput = passbuf1;
	  goodTrans = for_translatePass ();
	  break;
	case 4:
	  srcmax = dest;
	  currentInput = passbuf1;
	  currentOutput = passbuf2;
	  goodTrans = for_translatePass ();
	  break;
	default:
	  break;
	}
      currentPass++;
    }
  if (goodTrans)
    {
      for (k = 0; k < dest; k++)
	{
	  if (typeform != NULL)
	    {
	      if ((currentOutput[k] & (B7 | B8)))
		typeform[k] = '8';
	      else
		typeform[k] = '0';
	    }
	  if ((mode & dotsIO))
	    outbuf[k] = currentOutput[k];
	  else
	    outbuf[k] = getCharFromDots (currentOutput[k]);
	}
      if (src < *inlen)
	*inlen = src;
      *outlen = dest;
    }
  if (destSpacing != NULL)
    {
      memcpy (srcSpacing, destSpacing, srcmax);
    }
  if (cursorPos != NULL)
    *cursorPos = cursorPosition;
  return goodTrans;
}

static int transCharslen;	/*length of current find string */
static ContractionTableOpcode transOpcode;
static ContractionTableOpcode indicOpcode;
static ContractionTableOpcode prevTransOpcode;
static const ContractionTableRule *transRule;
static const ContractionTableRule *indicRule;
static int dontContract = 0;
static int passVariables[NUMVAR];
static int passSrc;
static widechar const *passInstructions;
static int passIC;		/*Instruction counter */
static int startMatch;
static int endMatch;
static int startReplace;
static int endReplace;

static ContractionTableCharacter *findCharOrDots (widechar c, int m);
static int doCompbrl (void);

static int
for_updatePositions (const widechar * outChars, int inLength, int outLength)
{
  int k;
  if ((dest + outLength) > destmax || (src + inLength) > srcmax)
    return 0;
  memcpy (&currentOutput[dest], outChars, outLength * CHARSIZE);
  if (!cursorStatus && cursorPosition >= 0)
    {
      if (cursorPosition >= src && cursorPosition < (src + inLength))
	{
	  cursorPosition = dest + outLength / 2;
	  cursorStatus = 1;
	  if ((mode & compbrlAtCursor) &&
	      !((findCharOrDots (currentOutput[cursorPosition], 1))->
		attributes & CTC_Space))
	    {
	      cursorStatus = 2;
	      doCompbrl ();
	      cursorStatus = 1;
	      return 1;
	    }
	}
    }
  if (inputPositions != NULL || outputPositions != NULL)
    {
      if (outLength <= inLength)
	{
	  for (k = 0; k < outLength; k++)
	    {
	      if (inputPositions != NULL)
		inputPositions[dest + k] = src + k;
	      if (outputPositions != NULL)
		outputPositions[src + k] = dest + k;
	    }
	  for (k = outLength; k < inLength; k++)
	    if (outputPositions != NULL)
	      outputPositions[src + k] = dest + outLength - 1;
	}
      else
	{
	  for (k = 0; k < inLength; k++)
	    {
	      if (inputPositions != NULL)
		inputPositions[dest + k] = src + k;
	      if (outputPositions != NULL)
		outputPositions[src + k] = dest + k;
	    }
	  for (k = inLength; k < outLength; k++)
	    if (inputPositions != NULL)
	      inputPositions[dest + k] = src + inLength - 1;
	}
    }
  dest += outLength;
  return 1;
}

static int
hyphenate (const widechar * word, int wordSize, char *hyphens)
{
  widechar prepWord[MAXSTRING];
  int i, j, k;
  int stateNum;
  widechar ch;
  HyphenationState *statesArray = (HyphenationState *)
    & table->ruleArea[table->hyphenStatesArray];
  HyphenationState *currentState;
  HyphenationTrans *transitionsArray;
  char *hyphenPattern;
  int patternOffset;
  if (!table->hyphenStatesArray || (wordSize + 3) > MAXSTRING)
    return 0;
  j = 0;
  prepWord[j++] = '.';
  for (i = 0; i < wordSize; i++)
    prepWord[j++] = (findCharOrDots (word[i], 0))->lowercase;
  prepWord[j++] = '.';
  prepWord[j] = 0;
  for (i = 0; i < j; i++)
    hyphens[i] = '0';
  /* now, run the finite state machine */
  stateNum = 0;
  for (i = 0; i < j; i++)
    {
      ch = prepWord[i];
      while (1)
	{
	  if (stateNum == 0xffff)
	    {
	      stateNum = 0;
	      goto nextLetter;
	    }
	  currentState = &statesArray[stateNum];
	  if (currentState->trans.offset)
	    {
	      transitionsArray = (HyphenationTrans *) &
		table->ruleArea[currentState->trans.offset];
	      for (k = 0; k < currentState->numTrans; k++)
		if (transitionsArray[k].ch == ch)
		  {
		    stateNum = transitionsArray[k].newState;
		    goto stateFound;
		  }
	    }
	  stateNum = currentState->fallbackState;
	}
    stateFound:
      currentState = &statesArray[stateNum];
      if (currentState->hyphenPattern)
	{
	  hyphenPattern =
	    (char *) &table->ruleArea[currentState->hyphenPattern];
	  patternOffset = i + 1 - strlen (hyphenPattern);
	  for (k = 0; hyphenPattern[k]; k++)
	    if (hyphens[patternOffset + k] < hyphenPattern[k])
	      hyphens[patternOffset + k] = hyphenPattern[k];
	}
    nextLetter:;
    }
  hyphens[wordSize] = 0;
  return 1;
}

static ContractionTableCharacter *
findCharOrDots (widechar c, int m)
{
/*Look up character or dot pattern in the appropriate  
* table. */
  static ContractionTableCharacter noChar =
    { 0, 0, 0, CTC_Space, 32, 32, 32 };
  static ContractionTableCharacter noDots =
    { 0, 0, 0, CTC_Space, B16, B16, B16 };
  ContractionTableCharacter *notFound;
  ContractionTableCharacter *character;
  ContractionTableOffset bucket;
  unsigned long int makeHash = (unsigned long int) c % HASHNUM;
  if (m == 0)
    {
      bucket = table->characters[makeHash];
      notFound = &noChar;
    }
  else
    {
      bucket = table->dots[makeHash];
      notFound = &noDots;
    }
  while (bucket)
    {
      character = (ContractionTableCharacter *) & table->ruleArea[bucket];
      if (character->realchar == c)
	return character;
      bucket = character->next;
    }
  return notFound;
}

static int
checkAttr (const widechar c, const ContractionTableCharacterAttributes
	   a, int m)
{
  static widechar prevc = 0;
  static ContractionTableCharacterAttributes preva = 0;
  if (c != prevc)
    {
      preva = (findCharOrDots (c, m))->attributes;
      prevc = c;
    }
  return ((preva & a) ? 1 : 0);
}

static int
syllableBreak (void)
{
  int wordStart;
  int wordEnd;
  int k;
  char hyphens[MAXSTRING];
  for (wordStart = src; wordStart >= 0; wordStart--)
    if (!((findCharOrDots (currentInput[wordStart], 0))->attributes &
	  CTC_Letter))
      {
	wordStart++;
	break;
      }
  if (wordStart < 0)
    wordStart = 0;
  for (wordEnd = src; wordEnd < srcmax; wordEnd++)
    if (!((findCharOrDots (currentInput[wordEnd], 0))->attributes &
	  CTC_Letter))
      {
	wordEnd--;
	break;
      }
  if (!hyphenate (&currentInput[wordStart], wordEnd - wordStart, hyphens))
    return 0;
/* If the number at the beginning of the syllable is odd or all 
* numbers are even there is no syllable break. Otherwise there is.*/
  k = src - wordStart;
  if (hyphens[k] & 1)
    return 0;
  k++;
  for (; k < (src - wordStart + transCharslen); k++)
    if (hyphens[k] & 1)
      return 1;
  return 0;
}

static int
compareChars (const widechar * address1, const widechar * address2, int
	      count, int m)
{
  int k;
  if (!count)
    return 0;
  for (k = 0; k < count; k++)
    if ((findCharOrDots (address1[k], m))->lowercase != (findCharOrDots
							 (address2[k],
							  m))->lowercase)
      return 0;
  return 1;
}

static ContractionTableCharacter *curCharDef;
static widechar before, after;
static ContractionTableCharacterAttributes beforeAttributes;
static ContractionTableCharacterAttributes afterAttributes;
static void
setBefore (void)
{
  before = (src == 0) ? ' ' : currentInput[src - 1];
  beforeAttributes = (findCharOrDots (before, 0))->attributes;
}

static void
setAfter (int length)
{
  after = (src + length < srcmax) ? currentInput[src + length] : ' ';
  afterAttributes = (findCharOrDots (after, 0))->attributes;
}

static int prevTypeform = plain_text;
static int prevSrc = 0;
static ContractionTableRule pseudoRule = { 0 };

static int
findBrailleIndicatorRule (ContractionTableOffset offset)
{
  if (!offset)
    return 0;
  indicRule = (ContractionTableRule *) & table->ruleArea[offset];
  indicOpcode = indicRule->opcode;
  return 1;
}

static typeforms prevType = plain_text;
static typeforms curType = plain_text;

typedef enum
{
  firstWord,
  lastWordBefore,
  lastWordAfter,
  firstLetter,
  lastLetter,
  singleLetter,
  word,
  lenPhrase
} emphCodes;

static int wordsMarked = 0;
static int finishEmphasis = 0;
static int wordCount = 0;
static int lastWord = 0;
static int startType = -1;
static int endType = 0;

static void
markWords (const ContractionTableOffset * const offset)
{
/*Mark the beginnings of words*/
  int numWords = 0;
  int k;
  wordsMarked = 1;
  numWords = offset[lenPhrase];
  if (!numWords)
    numWords = 4;
  if (wordCount < numWords)
    {
      for (k = src; k < endType; k++)
	if (!checkAttr (currentInput[k - 1], CTC_Letter | CTC_Digit, 0) &&
	    checkAttr (currentInput[k], CTC_Digit | CTC_Letter, 0))
	  typebuf[k] |= 0x10;
    }
  else
    {
      int firstWord = 1;
      int lastWord = src;
      for (k = src; k < endType; k++)
	{
	  if (!checkAttr (currentInput[k - 1], CTC_Letter | CTC_Digit, 0)
	      && checkAttr (currentInput[k], CTC_Digit | CTC_Letter, 0))
	    {
	      if (firstWord)
		{
		  typebuf[k] |= 0x20;
		  firstWord = 0;
		}
	      else
		lastWord = k;
	    }
	}
      typebuf[lastWord] |= 0x10;
    }
}

static int
insertMarks (void)
{
/*Insert italic, bold, etc. marks before words*/
  int typeMark;
  int ruleFound = 0;
  if (!wordsMarked || !haveTypeforms)
    return 1;
  typeMark = (typebuf[src] >> 4) & 0x0f;
  if (!typeMark)
    return 1;
  switch (typebuf[src] & 0x0f)
    {
    case italic:
      if (typeMark == 2)
	ruleFound = findBrailleIndicatorRule (table->firstWordItal);
      else
	ruleFound = findBrailleIndicatorRule (table->lastWordItalBefore);
      break;
    case bold:
      if (typeMark == 2)
	ruleFound = findBrailleIndicatorRule (table->firstWordBold);
      else
	ruleFound = findBrailleIndicatorRule (table->lastWordBoldBefore);
      break;
    case underline:
      if (typeMark == 2)
	ruleFound = findBrailleIndicatorRule (table->firstWordUnder);
      else
	ruleFound = findBrailleIndicatorRule (table->lastWordUnderBefore);
      break;
    default:
      ruleFound = 0;
      break;
    }
  if (ruleFound)
    {
      if (!for_updatePositions
	  (&indicRule->charsdots[0], 0, indicRule->dotslen))
	return 0;
    }
  return 1;
}

static int
validMatch ()
{
/*Analyze the typeform parameter and also check for capitalization*/
  ContractionTableCharacter *currentInputChar;
  ContractionTableCharacter *ruleChar;
  ContractionTableCharacterAttributes prevAttr = 0;
  int k;
  int kk = 0;
  widechar mask;
  if (!transCharslen)
    return 0;
  switch (transOpcode)
    {
    case CTO_WholeWord:
    case CTO_PrefixableWord:
    case CTO_SuffixableWord:
    case CTO_JoinableWord:
    case CTO_LowWord:
      mask = 0x000f;
      break;
    default:
      mask = 0xff0f;
      break;
    }
  for (k = src; k < src + transCharslen; k++)
    {
      currentInputChar = findCharOrDots (currentInput[k], 0);
      if (k == src)
	prevAttr = currentInputChar->attributes;
      ruleChar = findCharOrDots (transRule->charsdots[kk++], 0);
      if ((currentInputChar->lowercase != ruleChar->lowercase)
	  || (typebuf != NULL && (typebuf[k] & mask) !=
	      (typebuf[src] & mask))
	  || (k != (src + 1) && (currentInputChar->attributes &
				 (CTC_LowerCase
				  |
				  CTC_UpperCase))
	      && (prevAttr & (CTC_LowerCase | CTC_UpperCase))
	      && currentInputChar->attributes != prevAttr))
	return 0;
      prevAttr = currentInputChar->attributes;
    }
  return 1;
}

static int for_passDoTest (void);
static int for_passDoAction (void);

static int
findAttribOrSwapRules (void)
{
  int save_transCharslen = transCharslen;
  const ContractionTableRule *save_transRule = transRule;
  ContractionTableOpcode save_transOpcode = transOpcode;
  ContractionTableOffset ruleOffset;
  ruleOffset = table->attribOrSwapRules[currentPass];
  transCharslen = 0;
  while (ruleOffset)
    {
      transRule = (ContractionTableRule *) & table->ruleArea[ruleOffset];
      transOpcode = transRule->opcode;
      if (for_passDoTest ())
	return 1;
      ruleOffset = transRule->charsnext;
    }
  transCharslen = save_transCharslen;
  transRule = save_transRule;
  transOpcode = save_transOpcode;
  return 0;
}

static int
checkMultCaps (void)
{
  int k;
  for (k = 0; k < table->lenBeginCaps; k++)
    if (!checkAttr (currentInput[src + k], CTC_UpperCase, 0))
      return 0;
  return 1;
}

static int prevPrevType = 0;
static int nextType = 0;
static ContractionTableCharacterAttributes prevPrevAttr = 0;

static int
beginEmphasis (const ContractionTableOffset * const offset)
{
  if (src != startType)
    {
      wordCount = finishEmphasis = wordsMarked = 0;
      startType = lastWord = src;
      for (endType = src; endType < srcmax; endType++)
	{
	  if ((typebuf[endType] & 0x0f) != curType)
	    break;
	  if (checkAttr (currentInput[endType - 1], CTC_Space, 0)
	      && !checkAttr (currentInput[endType], CTC_Space, 0))
	    {
	      lastWord = endType;
	      wordCount++;
	    }
	}
    }
  if ((beforeAttributes & CTC_Letter) && (endType - startType) ==
      1 && findBrailleIndicatorRule (offset[singleLetter]))
    return 1;
  else
    if ((beforeAttributes & CTC_Letter) && findBrailleIndicatorRule
	(offset[firstLetter]))
    return 1;
  else if (findBrailleIndicatorRule (offset[lastWordBefore]))
    {
      markWords (offset);
      return 0;
    }
  else
    return (findBrailleIndicatorRule (offset[firstWord]));
  return 0;
}

static int
endEmphasis (const ContractionTableOffset * const offset)
{
  if (wordsMarked)
    return 0;
  if (prevPrevType != prevType && nextType != prevType &&
      findBrailleIndicatorRule (offset[singleLetter]))
    return 0;
  else
    if ((finishEmphasis || (src < srcmax && ((findCharOrDots
					      (currentInput[src + 1],
					       0))->attributes &
					     CTC_Letter)))
	&& findBrailleIndicatorRule (offset[lastLetter]))
    return 1;
  else
    return (findBrailleIndicatorRule (offset[lastWordAfter]));
  return 0;
}

static int doCompTrans (int start, int end);

static int
doCompEmph (void)
{
  int endEmph;
  for (endEmph = src; (typebuf[endEmph] & computer_braille) && endEmph
       <= srcmax; endEmph++);
  return doCompTrans (src, endEmph);
}

static int
insertBrailleIndicators (int finish)
{
/*Insert braille indicators such as italic, bold, capital, 
* letter, number, etc.*/
  typedef enum
  {
    checkNothing,
    checkBeginTypeform,
    checkEndTypeform,
    checkNumber,
    checkLetter,
    checkBeginMultCaps,
    checkEndMultCaps,
    checkSingleCap,
    checkAll
  } checkThis;
  checkThis checkWhat = checkNothing;
  int ok = 0;
  int k;
  if (finish == 2)
    {
      while (dest > 0 && (currentOutput[dest - 1] == 0 ||
			  currentOutput[dest - 1] == B16))
	dest--;
      finishEmphasis = 1;
      prevType = prevPrevType;
      curType = plain_text;
      checkWhat = checkEndTypeform;
    }
  else
    {
      if (src == prevSrc && !finish)
	return 1;
      if (src != prevSrc)
	{
	  if (haveTypeforms && src < srcmax)
	    nextType = typebuf[src + 1] & 0x0f;
	  else
	    nextType = plain_text;
	  if (src > 2)
	    {
	      if (haveTypeforms)
		prevPrevType = typebuf[src - 2] & 0x0f;
	      else
		prevPrevType = plain_text;
	      prevPrevAttr =
		(findCharOrDots (currentInput[src - 2], 0))->attributes;
	    }
	  else
	    {
	      prevPrevType = plain_text;
	      prevPrevAttr = CTC_Space;
	    }
	  if (haveTypeforms && (typebuf[src] & 0x0f) != prevTypeform)
	    {
	      prevType = prevTypeform & 0x0f;
	      curType = typebuf[src] & 0x0f;
	      checkWhat = checkEndTypeform;
	    }
	  else if (!finish)
	    checkWhat = checkNothing;
	  else
	    checkWhat = checkNumber;
	}
      if (finish == 1)
	checkWhat = checkNumber;
    }
  do
    {
      ok = 0;
      switch (checkWhat)
	{
	case checkNothing:
	  ok = 0;
	  break;
	case checkBeginTypeform:
	  if (haveTypeforms)
	    switch (curType)
	      {
	      case plain_text:
		ok = 0;
		break;
	      case italic:
		ok = beginEmphasis (&table->firstWordItal);
		curType = 0;
		break;
	      case bold:
		ok = beginEmphasis (&table->firstWordBold);
		curType = 0;
		break;
	      case underline:
		ok = beginEmphasis (&table->firstWordUnder);
		curType = 0;
		break;
	      case computer_braille:
		ok = 0;
		doCompEmph ();
		curType = 0;
		break;
	      case italic + underline:
		ok = beginEmphasis (&table->firstWordUnder);
		curType -= underline;
		break;
	      case italic + bold:
		ok = beginEmphasis (&table->firstWordBold);
		curType -= bold;
		break;
	      case italic + computer_braille:
		ok = 0;
		doCompEmph ();
		curType -= computer_braille;
		break;
	      case underline + bold:
		beginEmphasis (&table->firstWordBold);
		curType -= bold;
		break;
	      case underline + computer_braille:
		ok = 0;
		doCompEmph ();
		curType -= computer_braille;
		break;
	      case bold + computer_braille:
		ok = 0;
		doCompEmph ();
		curType -= computer_braille;
		break;
	      default:
		ok = 0;
		curType = 0;
		break;
	      }
	  if (!curType)
	    {
	      if (!finish)
		checkWhat = checkNothing;
	      else
		checkWhat = checkNumber;
	    }
	  break;
	case checkEndTypeform:
	  if (haveTypeforms)
	    switch (prevType)
	      {
	      case plain_text:
		ok = 0;
		break;
	      case italic:
		ok = endEmphasis (&table->firstWordItal);
		prevType = 0;
		break;
	      case bold:
		ok = endEmphasis (&table->firstWordBold);
		prevType = 0;
		break;
	      case underline:
		ok = endEmphasis (&table->firstWordUnder);
		prevType = 0;
		break;
	      case computer_braille:
		ok = 0;
		prevType = 0;
		break;
	      case italic + underline:
		ok = endEmphasis (&table->firstWordUnder);
		prevType -= underline;
		break;
	      case italic + bold:
		ok = endEmphasis (&table->firstWordBold);
		prevType -= bold;
		break;
	      case italic + computer_braille:
		ok = 1;
		prevType -= computer_braille;
		break;
	      case underline + bold:
		ok = endEmphasis (&table->firstWordBold);
		prevType -= bold;
		break;
	      case underline + computer_braille:
		ok = 0;
		prevType -= computer_braille;
		break;
	      case bold + computer_braille:
		ok = endEmphasis (&table->firstWordBold);
		prevType -= bold;
		break;
	      default:
		ok = 0;
		prevType = 0;
		break;
	      }
	  if (!prevType)
	    {
	      checkWhat = checkBeginTypeform;
	      prevTypeform = typebuf[src] & 0x0f;
	    }
	  break;
	case checkNumber:
	  if (findBrailleIndicatorRule
	      (table->numberSign) &&
	      checkAttr (currentInput[src], CTC_Digit, 0) &&
	      !(beforeAttributes & CTC_Digit)
	      && prevTransOpcode != CTO_MidNum)
	    {
	      ok = 1;
	      checkWhat = checkNothing;
	    }
	  else
	    checkWhat = checkLetter;
	  break;
	case checkLetter:
	  if (!findBrailleIndicatorRule (table->letterSign))
	    {
	      ok = 0;
	      checkWhat = checkBeginMultCaps;
	      break;
	    }
	  if (transOpcode == CTO_Contraction)
	    {
	      ok = 1;
	      checkWhat = checkBeginMultCaps;
	      break;
	    }
	  if ((checkAttr (currentInput[src], CTC_Letter, 0)
	       && !(beforeAttributes & CTC_Letter))
	      && (!checkAttr (currentInput[src + 1], CTC_Letter, 0)
		  || (beforeAttributes & CTC_Digit)))
	    {
	      ok = 1;
	      if (src > 0)
		for (k = 0; k < table->noLetsignBeforeCount; k++)
		  if (currentInput[src - 1] == table->noLetsignBefore[k])
		    {
		      ok = 0;
		      break;
		    }
	      for (k = 0; k < table->noLetsignCount; k++)
		if (currentInput[src] == table->noLetsign[k])
		  {
		    ok = 0;
		    break;
		  }
	      if ((src + 1) < srcmax)
		for (k = 0; k < table->noLetsignAfterCount; k++)
		  if (currentInput[src + 1] == table->noLetsignAfter[k])
		    {
		      ok = 0;
		      break;
		    }
	    }
	  checkWhat = checkBeginMultCaps;
	  break;
	case checkBeginMultCaps:
	  if (findBrailleIndicatorRule (table->beginCapitalSign) &&
	      !(beforeAttributes & CTC_UpperCase) && checkMultCaps ())
	    {
	      ok = 1;
	      if (table->capsNoCont)
		dontContract = 1;
	      checkWhat = checkNothing;
	    }
	  else
	    checkWhat = checkSingleCap;
	  break;
	case checkEndMultCaps:
	  if (findBrailleIndicatorRule (table->endCapitalSign) &&
	      (prevPrevAttr & CTC_UpperCase)
	      && (beforeAttributes & CTC_UpperCase)
	      && checkAttr (currentInput[src], CTC_LowerCase, 0))
	    {
	      ok = 1;
	      if (table->capsNoCont)
		dontContract = 0;
	    }
	  checkWhat = checkNothing;
	  break;
	case checkSingleCap:
	  if (findBrailleIndicatorRule (table->capitalSign) && src < srcmax
	      && checkAttr (currentInput[src], CTC_UpperCase, 0) &&
	      !(beforeAttributes & CTC_UpperCase))
	    {
	      ok = 1;
	      checkWhat = checkNothing;
	    }
	  checkWhat = checkEndMultCaps;
	  break;
	default:
	  ok = 0;
	  checkWhat = checkNothing;
	  break;
	}
      if (ok && indicRule != NULL)
	{
	  if (!for_updatePositions
	      (&indicRule->charsdots[0], 0, indicRule->dotslen))
	    return 0;
	}
    }
  while (checkWhat != checkNothing);
  finishEmphasis = 0;
  return 1;
}

static void
for_selectRule (void)
{
/*check for valid contractions. Return value is in transRule. */
  int length = srcmax - src;
  int tryThis;
  const ContractionTableCharacter *character2;
  int k;
  curCharDef = findCharOrDots (currentInput[src], 0);
  for (tryThis = 0; tryThis < 3; tryThis++)
    {
      ContractionTableOffset ruleOffset = 0;
      unsigned long int makeHash = 0;
      switch (tryThis)
	{
	case 0:
	  if (!(length >= 2))
	    break;
	  /*Hash function optimized for forward translation */
	  makeHash = (unsigned long int) curCharDef->lowercase << 8;
	  character2 = findCharOrDots (currentInput[src + 1], 0);
	  makeHash += (unsigned long int) character2->lowercase;
	  makeHash %= HASHNUM;
	  ruleOffset = table->forRules[makeHash];
	  break;
	case 1:
	  if (!(length >= 1))
	    break;
	  length = 1;
	  ruleOffset = curCharDef->otherRules;
	  break;
	case 2:		/*No rule found */
	  transRule = &pseudoRule;
	  transOpcode = pseudoRule.opcode = CTO_None;
	  transCharslen = pseudoRule.charslen = 1;
	  pseudoRule.charsdots[0] = currentInput[src];
	  pseudoRule.dotslen = 0;
	  return;
	  break;
	}
      while (ruleOffset)
	{
	  transRule = (ContractionTableRule *) & table->ruleArea[ruleOffset];
	  transOpcode = transRule->opcode;
	  transCharslen = transRule->charslen;
	  if (tryThis == 1 || ((transCharslen <= length) && validMatch ()))
	    {
	      /* check this rule */
	      setAfter (transCharslen);
	      if ((!transRule->after || (beforeAttributes
					 & transRule->after)) &&
		  (!transRule->before || (afterAttributes
					  & transRule->before)))
		switch (transOpcode)
		  {		/*check validity of this contraction */
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
		  case CTO_Repeated:
		  case CTO_Replace:
		  case CTO_NoCont:
		  case CTO_CompBrl:
		  case CTO_Literal:
		    return;
		  case CTO_Syllable:
		    transOpcode = CTO_Always;
		  case CTO_Always:
		    if (dontContract || (mode & noContractions))
		      break;
		    return;
		  case CTO_NoCross:
		    if (syllableBreak ())
		      break;
		    return;
		  case CTO_Context:
		    if (for_passDoTest ())
		      return;
		    break;
		  case CTO_LargeSign:
		    if (dontContract || (mode & noContractions))
		      break;
		    if (!(beforeAttributes & (CTC_Space |
					      CTC_Punctuation))
			|| !(afterAttributes & CTC_Space))
		      transOpcode = CTO_Always;
		    return;
		  case CTO_WholeWord:
		    if (dontContract || (mode & noContractions))
		      break;
		  case CTO_Contraction:
		    if ((beforeAttributes & (CTC_Space | CTC_Punctuation))
			&& (afterAttributes & (CTC_Space | CTC_Punctuation)))
		      return;
		    break;
		  case CTO_PartWord:
		    if (dontContract || (mode & noContractions))
		      break;
		    if ((beforeAttributes & CTC_Letter)
			|| (afterAttributes & CTC_Letter))
		      return;
		    break;
		  case CTO_JoinNum:
		    if (dontContract || (mode & noContractions))
		      break;
		    if ((beforeAttributes & (CTC_Space | CTC_Punctuation))
			&&
			(afterAttributes & CTC_Space) &&
			(dest + transRule->dotslen < destmax))
		      {
			int cursrc = src + transCharslen + 1;
			while (cursrc < srcmax)
			  {
			    if (!checkAttr
				(currentInput[cursrc], CTC_Space, 0))
			      {
				if (checkAttr
				    (currentInput[cursrc], CTC_Digit, 0))
				  return;
				break;
			      }
			    cursrc++;
			  }
		      }
		    break;
		  case CTO_LowWord:
		    if (dontContract || (mode & noContractions))
		      break;
		    if ((beforeAttributes & CTC_Space)
			&& (afterAttributes & CTC_Space)
			&& (prevTransOpcode != CTO_JoinableWord))
		      return;
		    break;
		  case CTO_JoinableWord:
		    if (dontContract || (mode & noContractions))
		      break;
		    if ((beforeAttributes & (CTC_Space | CTC_Punctuation))
			&&
			(afterAttributes & CTC_Space) &&
			(dest + transRule->dotslen < destmax))
		      {
			int cursrc = src + transCharslen + 1;
			while (cursrc < srcmax)
			  {
			    if (!checkAttr
				(currentInput[cursrc], CTC_Space, 0))
			      {
				if (checkAttr
				    (currentInput[cursrc], CTC_Letter, 0))
				  return;
				break;
			      }
			    cursrc++;
			  }
		      }
		    break;
		  case CTO_SuffixableWord:
		    if (dontContract || (mode & noContractions))
		      break;
		    if ((beforeAttributes & (CTC_Space | CTC_Punctuation))
			&& (afterAttributes &
			    (CTC_Space | CTC_Letter | CTC_Punctuation)))
		      return;
		    break;
		  case CTO_PrefixableWord:
		    if (dontContract || (mode & noContractions))
		      break;
		    if ((beforeAttributes &
			 (CTC_Space | CTC_Letter | CTC_Punctuation))
			&& (afterAttributes & (CTC_Space | CTC_Punctuation)))
		      return;
		    break;
		  case CTO_BegWord:
		    if (dontContract || (mode & noContractions))
		      break;
		    if ((beforeAttributes & (CTC_Space | CTC_Punctuation))
			&& (afterAttributes & CTC_Letter))
		      return;
		    break;
		  case CTO_BegMidWord:
		    if (dontContract || (mode & noContractions))
		      break;
		    if ((beforeAttributes &
			 (CTC_Letter | CTC_Space | CTC_Punctuation))
			&& (afterAttributes & CTC_Letter))
		      return;
		    break;
		  case CTO_MidWord:
		    if (dontContract || (mode & noContractions))
		      break;
		    if (beforeAttributes & CTC_Letter
			&& afterAttributes & CTC_Letter)
		      return;
		    break;
		  case CTO_MidEndWord:
		    if (dontContract || (mode & noContractions))
		      break;
		    if (beforeAttributes & CTC_Letter
			&& afterAttributes & (CTC_Letter | CTC_Space |
					      CTC_Punctuation))
		      return;
		    break;
		  case CTO_EndWord:
		    if (dontContract || (mode & noContractions))
		      break;
		    if (beforeAttributes & CTC_Letter
			&& afterAttributes & (CTC_Space | CTC_Punctuation))
		      return;
		    break;
		  case CTO_BegNum:
		    if (beforeAttributes & (CTC_Space | CTC_Punctuation)
			&& afterAttributes & CTC_Digit)
		      return;
		    break;
		  case CTO_MidNum:
		    if (beforeAttributes & CTC_Digit
			&& afterAttributes & CTC_Digit)
		      return;
		    break;
		  case CTO_EndNum:
		    if (beforeAttributes & CTC_Digit &&
			afterAttributes & (CTC_Space | CTC_Punctuation))
		      return;
		    break;
		  case CTO_DecPoint:
		    if (!(afterAttributes & CTC_Digit))
		      break;
		    if (beforeAttributes & CTC_Digit)
		      transOpcode = CTO_MidNum;
		    return;
		  case CTO_PrePunc:
		    if (!checkAttr (currentInput[src], CTC_Punctuation, 0)
			|| (src > 0
			    && checkAttr (currentInput[src - 1], CTC_Letter,
					  0)))
		      break;
		    for (k = src + transCharslen; k < srcmax; k++)
		      {
			if (checkAttr
			    (currentInput[k], (CTC_Letter | CTC_Digit), 0))
			  return;
			if (checkAttr (currentInput[k], CTC_Space, 0))
			  break;
		      }
		    break;
		  case CTO_PostPunc:
		    if (!checkAttr (currentInput[src], CTC_Punctuation, 0)
			|| (src < (srcmax - 1)
			    && checkAttr (currentInput[src + 1], CTC_Letter,
					  0)))
		      break;
		    for (k = src; k >= 0; k--)
		      {
			if (checkAttr
			    (currentInput[k], (CTC_Letter | CTC_Digit), 0))
			  return;
			if (checkAttr (currentInput[k], CTC_Space, 0))
			  break;
		      }
		    break;
		  default:
		    break;
		  }
	    }
/*Done with checking this rule */
	  ruleOffset = transRule->charsnext;
	}
    }
}

static int
undefinedCharacter (widechar c)
{
/*Display an undefined character in the output buffer*/
  int k;
  char *display = showString (&c, 1);
  if ((dest + strlen (display)) > destmax)
    return 0;
  for (k = 0; k < strlen (display); k++)
    currentOutput[dest++] = getDotsForChar (display[k]);
  return 1;
}

static int
putCharacter (widechar character)
{
/*Insert the dots equivalent of a character into the output buffer */
  ContractionTableCharacter *chardef;
  ContractionTableOffset offset;
  chardef = (findCharOrDots (character, 0));
  if ((chardef->attributes & CTC_Letter) && (chardef->attributes &
					     CTC_UpperCase))
    chardef = findCharOrDots (chardef->lowercase, 0);
  offset = chardef->definitionRule;
  if (offset)
    {
      const ContractionTableRule *rule = (ContractionTableRule *)
	& table->ruleArea[offset];
      if (rule->dotslen)
	return for_updatePositions (&rule->charsdots[1], 1, rule->dotslen);
      {
	widechar d = getDotsForChar (character);
	return for_updatePositions (&d, 1, 1);
      }
    }
  return undefinedCharacter (character);
}

static int
putCharacters (const widechar * characters, int count)
{
/*Insert the dot equivalents of a series of characters in the output 
* buffer */
  int k;
  for (k = 0; k < count; k++)
    if (!putCharacter (characters[k]))
      return 0;
  return 1;
}

static int destword;
static int srcword;

static int
doCompbrl (void)
{
/*Handle strings containing substrings defined by the compbrl opcode*/
  int stringEnd;
  if (checkAttr (currentInput[src], CTC_Space, 0))
    return 1;
  if (destword)
    {
      src = srcword;
      dest = destword;
    }
  else
    {
      src = 0;
      dest = 0;
    }
  for (stringEnd = src; stringEnd < srcmax; stringEnd++)
    if (checkAttr (currentInput[stringEnd], CTC_Space, 0))
      break;
  return (doCompTrans (src, stringEnd));
}

static int
putCompChar (widechar character)
{
/*Insert the dots equivalent of a character into the output buffer */
  ContractionTableOffset offset = (findCharOrDots
				   (character, 0))->definitionRule;
  if (offset)
    {
      const ContractionTableRule *rule = (ContractionTableRule *)
	& table->ruleArea[offset];
      if (rule->dotslen)
	return for_updatePositions (&rule->charsdots[1], 1, rule->dotslen);
      {
	widechar d = getDotsForChar (character);
	return for_updatePositions (&d, 1, 1);
      }
    }
  return undefinedCharacter (character);
}

static int
doCompTrans (int start, int end)
{
  int k;
  if (cursorStatus != 2 && findBrailleIndicatorRule (table->begComp))
    if (!for_updatePositions
	(&indicRule->charsdots[0], 0, indicRule->dotslen))
      return 0;
  for (k = start; k < end; k++)
    {
      ContractionTableOffset compdots = 0;
      if (currentInput[k] < 256)
	compdots = table->compdotsPattern[currentInput[k]];
      if (compdots != 0)
	{
	  transRule = (ContractionTableRule *) & table->ruleArea[compdots];
	  if (!for_updatePositions
	      (&transRule->charsdots[transRule->charslen],
	       transRule->charslen, transRule->dotslen))
	    return 0;
	}
      else if (!putCompChar (currentInput[k]))
	return 0;
    }
  if (cursorStatus != 2 && findBrailleIndicatorRule (table->endComp))
    if (!for_updatePositions
	(&indicRule->charsdots[0], 0, indicRule->dotslen))
      return 0;
  src = end;
  return 1;
}

static int
doNocont (void)
{
/*Handle strings containing substrings defined by the nocont opcode*/
  if (checkAttr (currentInput[src], CTC_Space, 0) || dontContract
      || (mode & noContractions))
    return 1;
  if (destword)
    {
      src = srcword;
      dest = destword;
    }
  else
    {
      src = 0;
      dest = 0;
    }
  dontContract = 1;
  return 1;
}

static int
makeCorrections (void)
{
  int k;
  if (!table->corrections)
    return 1;
  src = 0;
  dest = 0;
  for (k = 0; k < NUMVAR; k++)
    passVariables[k] = 0;
  while (src < srcmax)
    {
      int length = srcmax - src;
      const ContractionTableCharacter *character = findCharOrDots
	(currentInput[src], 0);
      const ContractionTableCharacter *character2;
      int tryThis = 0;
      if (!findAttribOrSwapRules ())
	while (tryThis < 3)
	  {
	    ContractionTableOffset ruleOffset = 0;
	    unsigned long int makeHash = 0;
	    switch (tryThis)
	      {
	      case 0:
		if (!(length >= 2))
		  break;
		makeHash = (unsigned long int) character->lowercase << 8;
		character2 = findCharOrDots (currentInput[src + 1], 0);
		makeHash += (unsigned long int) character2->lowercase;
		makeHash %= HASHNUM;
		ruleOffset = table->forRules[makeHash];
		break;
	      case 1:
		if (!(length >= 1))
		  break;
		length = 1;
		ruleOffset = character->otherRules;
		break;
	      case 2:		/*No rule found */
		transOpcode = CTO_Always;
		ruleOffset = 0;
		break;
	      }
	    while (ruleOffset)
	      {
		transRule =
		  (ContractionTableRule *) & table->ruleArea[ruleOffset];
		transOpcode = transRule->opcode;
		transCharslen = transRule->charslen;
		if (tryThis == 1 || (transCharslen <= length &&
				     compareChars (&transRule->
						   charsdots[0],
						   &currentInput[src],
						   transCharslen, 0)))
		  {
		    if (transOpcode == CTO_Correct && for_passDoTest ())
		      {
			tryThis = 4;
			break;
		      }
		  }
		ruleOffset = transRule->charsnext;
	      }
	    tryThis++;
	  }
      switch (transOpcode)
	{
	case CTO_Always:
	  if (dest >= destmax)
	    goto failure;
	  currentOutput[dest++] = currentInput[src++];
	  break;
	case CTO_Correct:
	  if (!for_passDoAction ())
	    goto failure;
	  src = endReplace;
	  break;
	default:
	  break;
	}
    }
failure:
  return 1;
}

static int
markSyllables (void)
{
  int k;
  int bitConfig = 0;
  if (typebuf == NULL || !table->syllables)
    return 1;
  src = 0;
  while (src < srcmax)
    {				/*the main multipass translation loop */
      int length = srcmax - src;
      const ContractionTableCharacter *character = findCharOrDots
	(currentInput[src], 0);
      const ContractionTableCharacter *character2;
      int tryThis = 0;
      while (tryThis < 3)
	{
	  ContractionTableOffset ruleOffset = 0;
	  unsigned long int makeHash = 0;
	  switch (tryThis)
	    {
	    case 0:
	      if (!(length >= 2))
		break;
	      makeHash = (unsigned long int) character->lowercase << 8;
	      character2 = findCharOrDots (currentInput[src + 1], 0);
	      makeHash += (unsigned long int) character2->lowercase;
	      makeHash %= HASHNUM;
	      ruleOffset = table->forRules[makeHash];
	      break;
	    case 1:
	      if (!(length >= 1))
		break;
	      length = 1;
	      ruleOffset = character->otherRules;
	      break;
	    case 2:		/*No rule found */
	      transOpcode = CTO_Always;
	      ruleOffset = 0;
	      break;
	    }
	  while (ruleOffset)
	    {
	      transRule =
		(ContractionTableRule *) & table->ruleArea[ruleOffset];
	      transOpcode = transRule->opcode;
	      transCharslen = transRule->charslen;
	      if (tryThis == 1 || (transCharslen <= length &&
				   compareChars (&transRule->
						 charsdots[0],
						 &currentInput[src],
						 transCharslen, 0)))
		{
		  if (transOpcode == CTO_Syllable)
		    {
		      tryThis = 4;
		      break;
		    }
		}
	      ruleOffset = transRule->charsnext;
	    }
	  tryThis++;
	}
      switch (transOpcode)
	{
	case CTO_Always:
	  if (src >= srcmax)
	    return 0;
	  if (typebuf != NULL)
	    typebuf[src++] |= (bitConfig & 0xf) << 8;
	  break;
	case CTO_Syllable:
	  bitConfig++;
	  if ((src + transCharslen) > srcmax)
	    return 0;
	  for (k = 0; k < transCharslen; k++)
	    typebuf[src++] |= (bitConfig & 0xf) << 8;
	  break;
	default:
	  break;
	}
    }
  return 1;
}

static int
translateString (void)
{
/*Main translation routine */
  int k;
  markSyllables ();
  srcword = 0;
  destword = 0;			/* last word translated */
  prevTransOpcode = CTO_None;
  cursorStatus = wordsMarked = 0;
  prevType = prevPrevType = curType = nextType = prevTypeform = plain_text;
  startType = prevSrc = -1;
  src = dest = 0;
  for (k = 0; k < NUMVAR; k++)
    passVariables[k] = 0;
  if (typebuf && table->firstWordCaps)
    for (k = 0; k < srcmax; k++)
      if (checkAttr (currentInput[k], CTC_UpperCase, 0))
	typebuf[k] |= 0x1000;	/*set capital emphasis */
  while (src < srcmax)
    {				/*the main translation loop */
      setBefore ();
      if (!insertBrailleIndicators (0))
	goto failure;
      if (src >= srcmax)
	break;
      if (!insertMarks ())
	goto failure;
      for_selectRule ();
      prevSrc = src;
      switch (transOpcode)	/*Rules that pre-empt context and swap */
	{
	case CTO_CompBrl:
	case CTO_Literal:
	  if (!doCompbrl ())
	    goto failure;
	  continue;
	default:
	  break;
	}
      if (!insertBrailleIndicators (1))
	goto failure;
      if (findAttribOrSwapRules ())
	switch (transOpcode)
	  {
	  case CTO_Context:
	    if (!for_passDoAction ())
	      goto failure;
	    src = endReplace;
	    continue;
	  default:
	    break;
	  }

/*Processing before replacement*/
      switch (transOpcode)
	{
	case CTO_EndNum:
	  if (table->letterSign && checkAttr (currentInput[src],
						     CTC_Letter, 0))
	    dest--;
	  break;
	case CTO_Repeated:
	case CTO_Space:
	  dontContract = 0;
	  break;
	case CTO_LargeSign:
	  if (prevTransOpcode == CTO_LargeSign)
	    if (dest > 0 && checkAttr (currentOutput[dest - 1], CTC_Space, 1))
	      dest--;
	  break;
	case CTO_DecPoint:
	  if (table->numberSign)
	    {
	      ContractionTableRule *numRule = (ContractionTableRule *)
		& table->ruleArea[table->numberSign];
	      if (!for_updatePositions
		  (&numRule->charsdots[numRule->charslen],
		   numRule->charslen, numRule->dotslen))
		goto failure;
	    }
	  transOpcode = CTO_MidNum;
	  break;
	case CTO_NoCont:
	  if (!dontContract)
	    doNocont ();
	  continue;
	default:
	  break;
	}			/*end of action */

      /* replacement processing */
      switch (transOpcode)
	{
	case CTO_Replace:
	  src += transCharslen;
	  if (!putCharacters
	      (&transRule->charsdots[transCharslen], transRule->dotslen))
	    goto failure;
	  break;
	case CTO_None:
	  if (!undefinedCharacter (currentInput[src]))
	    goto failure;
	  src++;
	  break;
	case CTO_UpperCase:
	  if (transRule->dotslen == 1)
	    {
	      putCharacter (curCharDef->lowercase);
	      src++;
	      break;
	    }
	default:
	  if (transRule->dotslen)
	    {
	      if (!for_updatePositions
		  (&transRule->charsdots[transCharslen],
		   transCharslen, transRule->dotslen))
		goto failure;
	    }
	  else
	    {
	      for (k = src; k < (src + transCharslen); k++)
		{
		  if (!putCharacter (currentInput[k]))
		    goto failure;
		}
	    }
	  src += transCharslen;
	  break;
	}

      /* processing after replacement */
      switch (transOpcode)
	{
	case CTO_Repeated:
	  {
	    int srclim = srcmax - transCharslen;
	    while ((src <= srclim)
		   && compareChars (&transRule->charsdots[0],
				    &currentInput[src], transCharslen, 0))
	      {
		src += transCharslen;
	      }
	    break;
	  }
	case CTO_JoinNum:
	case CTO_JoinableWord:
	  while ((src < srcmax)
		 && checkAttr (currentInput[src], CTC_Space, 0))
	    src++;
	  break;
	default:
	  break;
	}
      if (((src > 0) && checkAttr (currentInput[src - 1], CTC_Space, 0)
	   && (transOpcode != CTO_JoinableWord)))
	{
	  srcword = src;
	  destword = dest;
	}
      if (srcSpacing != NULL && srcSpacing[src] >= '0' && srcSpacing[src] <=
	  '9')
	destSpacing[dest] = srcSpacing[src];
      if ((transOpcode >= CTO_Always && transOpcode <= CTO_None) ||
	  (transOpcode >= CTO_Digit && transOpcode <= CTO_LitDigit))
	prevTransOpcode = transOpcode;
    }				/*end of translation loop */
  if (haveTypeforms && !wordsMarked && prevPrevType != plain_text)
    insertBrailleIndicators (2);
failure:
  if (destword != 0 && src < srcmax
      && !checkAttr (currentInput[src], CTC_Space, 0))
    {
      src = srcword;
      dest = destword;
    }
  if (src < srcmax)
    {
      while (checkAttr (currentInput[src], CTC_Space, 0))
	if (++src == srcmax)
	  break;
    }
  return 1;
}				/*first pass translation completed */

/*Multipass translation*/

static int
matchcurrentInput (void)
{
  int k;
  int kk = passSrc;
  for (k = passIC + 2; k < passIC + 2 + passInstructions[passIC + 1]; k++)
    if (passInstructions[k] != currentInput[kk++])
      return 0;
  return 1;
}

static int
swapTest (void)
{
  int curLen;
  int curTest;
  int curSrc = passSrc;
  ContractionTableOffset swapRuleOffset;
  ContractionTableRule *swapRule;
  swapRuleOffset =
    (passInstructions[passIC + 1] << 16) | passInstructions[passIC + 2];
  swapRule = (ContractionTableRule *) & table->ruleArea[swapRuleOffset];
  for (curLen = 0; curLen < passInstructions[passIC] + 3; curLen++)
    {
      for (curTest = 0; curTest < swapRule->charslen; curTest++)
	{
	  if (currentInput[curSrc] == swapRule->charsdots[curTest])
	    break;
	}
      if (curTest == swapRule->charslen)
	return 0;
      curSrc++;
    }
  if (passInstructions[passIC + 2] == passInstructions[passIC + 3])
    {
      passSrc = curSrc;
      return 1;
    }
  while (curLen < passInstructions[passIC + 4])
    {
      for (curTest = 0; curTest < swapRule->charslen; curTest++)
	{
	  if (currentInput[curSrc] != swapRule->charsdots[curTest])
	    break;
	}
      if (curTest < swapRule->charslen)
	if (curTest < swapRule->charslen)
	  {
	    passSrc = curSrc;
	    return 1;
	  }
      curSrc++;
      curLen++;
    }
  passSrc = curSrc;
  return 1;
}

static int
swapReplace (int startSrc, int maxLen)
{
  ContractionTableOffset swapRuleOffset;
  ContractionTableRule *swapRule;
  widechar *replacements;
  int curRep;
  int curPos;
  int lastPos = 0;
  int lastRep = 0;
  int curTest;
  int curSrc = startSrc;
  swapRuleOffset =
    (passInstructions[passIC + 1] << 16) | passInstructions[passIC + 2];
  swapRule = (ContractionTableRule *) & table->ruleArea[swapRuleOffset];
  replacements = &swapRule->charsdots[swapRule->charslen];
  while (curSrc < maxLen)
    {
      for (curTest = 0; curTest < swapRule->charslen; curTest++)
	{
	  if (currentInput[curSrc] == swapRule->charsdots[curTest])
	    break;
	}
      if (curTest == swapRule->charslen)
	return curSrc;
      if (curTest >= lastRep)
	{
	  curPos = lastPos;
	  curRep = lastRep;
	}
      else
	{
	  curPos = 0;
	  curRep = 0;
	}
      while (curPos < swapRule->dotslen)
	{
	  if (curRep == curTest)
	    {
	      if ((dest + replacements[curPos] - 1) >= destmax)
		return 0;
	      memcpy (&currentOutput[dest], &replacements[curPos + 1],
		      (replacements[curPos] - 1) * CHARSIZE);
	      dest += replacements[curPos] - 1;
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
for_passDoTest (void)
{
  int k;
  int m;
  int not = 0;
  ContractionTableCharacterAttributes attributes;
  passSrc = src;
  passInstructions = &transRule->charsdots[transCharslen];
  passIC = 0;
  startMatch = endMatch = passSrc;
  startReplace = endReplace = -1;
  if (transOpcode == CTO_Context || transOpcode == CTO_Correct)
    m = 0;
  else
    m = 1;
  while (passIC < transRule->dotslen)
    {
      int itsTrue = 1;
      if (passSrc > srcmax)
	return 0;
      switch (passInstructions[passIC])
	{
	case pass_lookback:
	  passSrc -= passInstructions[passIC + 1];
	  if (passSrc < 0)
	    passSrc = 0;
	  passIC += 2;
	  break;
	case pass_not:
	  not = 1;
	  passIC++;
	  continue;
	case pass_string:
	case pass_dots:
	  itsTrue = matchcurrentInput ();
	  passSrc += passInstructions[passIC + 1];
	  passIC += passInstructions[passIC + 1] + 2;
	  break;
	case pass_startReplace:
	  startReplace = passSrc;
	  passIC++;
	  break;
	case pass_endReplace:
	  endReplace = passSrc;
	  passIC++;
	  break;
	case pass_attributes:
	  attributes = (passInstructions[passIC + 1] << 16) |
	    passInstructions[passIC + 2];
	  for (k = 0; k < passInstructions[passIC + 3]; k++)
	    itsTrue =
	      (((findCharOrDots (currentInput[passSrc++], m)->
		 attributes & attributes)) ? 1 : 0);
	  if (itsTrue)
	    for (k = passInstructions[passIC + 3]; k <
		 passInstructions[passIC + 4]; k++)
	      {
		if (!
		    (findCharOrDots (currentInput[passSrc], 1)->
		     attributes & attributes))
		  break;
		passSrc++;
	      }
	  passIC += 5;
	  break;
	case pass_swap:
	  itsTrue = swapTest ();
	  passIC += 5;
	  break;
	case pass_eq:
	  if (passVariables[passInstructions[passIC + 1]] !=
	      passInstructions[passIC + 2])
	    itsTrue = 0;
	  passIC += 3;
	  break;
	case pass_lt:
	  if (passVariables[passInstructions[passIC + 1]] >=
	      passInstructions[passIC + 2])
	    itsTrue = 0;
	  passIC += 3;
	  break;
	case pass_gt:
	  if (passVariables[passInstructions[passIC + 1]] <=
	      passInstructions[passIC + 2])
	    itsTrue = 0;
	  passIC += 3;
	  break;
	case pass_lteq:
	  if (passVariables[passInstructions[passIC + 1]] >
	      passInstructions[passIC + 2])
	    itsTrue = 0;
	  passIC += 3;
	  break;
	case pass_gteq:
	  if (passVariables[passInstructions[passIC + 1]] <
	      passInstructions[passIC + 2])
	    itsTrue = 0;
	  passIC += 3;
	  break;
	case pass_endTest:
	  passIC++;
	  endMatch = passSrc;
	  if (startReplace == -1)
	    {
	      startReplace = startMatch;
	      endReplace = endMatch;
	    }
	  return 1;
	  break;
	default:
	  return 0;
	}
      if ((!not && !itsTrue) || (not && itsTrue))
	return 0;
      not = 0;
    }
  return 1;
}

static int
for_passDoAction (void)
{
  int k;
  if ((dest + startReplace - startMatch) > destmax)
    return 0;
  for (k = startMatch; k < startReplace; k++)
    if (transOpcode == CTO_Context)
      {
	if (!putCharacter (currentInput[k]))
	  return 0;
      }
    else
      currentOutput[dest++] = currentInput[k];
  while (passIC < transRule->dotslen)
    switch (passInstructions[passIC])
      {
      case pass_string:
      case pass_dots:
	if ((dest + passInstructions[passIC + 1]) > destmax)
	  return 0;
	memcpy (&currentOutput[dest], &passInstructions[passIC + 2],
		passInstructions[passIC + 1] * CHARSIZE);
	dest += passInstructions[passIC + 1];
	passIC += passInstructions[passIC + 1] + 2;
	break;
      case pass_eq:
	passVariables[passInstructions[passIC + 1]] =
	  passInstructions[passIC + 2];
	passIC += 3;
	break;
      case pass_hyphen:
	passVariables[passInstructions[passIC + 1]]--;
	if (passVariables[passInstructions[passIC + 1]] < 0)
	  passVariables[passInstructions[passIC + 1]] = 0;
	passIC += 2;
	break;
      case pass_plus:
	passVariables[passInstructions[passIC + 1]]++;
	passIC += 2;
	break;
      case pass_swap:
	if (!swapReplace (startReplace, endReplace - startReplace))
	  return 0;
	passIC += 3;
	break;
      case pass_omit:
	passIC++;
	break;
      case pass_copy:
	dest -= startReplace - startMatch;
	k = endReplace - startReplace;
	if ((dest + k) > destmax)
	  return 0;
	memcpy (&currentOutput[dest], &currentInput[startReplace],
		k * CHARSIZE);
	dest += k;
	passIC++;
	endReplace = passSrc;
	break;
      default:
	return 0;
      }
  return 1;
}

static int
checkDots (void)
{
  int k;
  int kk = src;
  for (k = 0; k < transCharslen; k++)
    if (transRule->charsdots[k] != currentInput[kk++])
      return 0;
  return 1;
}

static void
for_passSelectRule (void)
{
  int length = srcmax - src;
  const ContractionTableCharacter *dots;
  const ContractionTableCharacter *dots2;
  int tryThis;
  ContractionTableOffset ruleOffset = 0;
  unsigned long int makeHash = 0;
  if (findAttribOrSwapRules ())
    return;
  dots = findCharOrDots (currentInput[src], 1);
  for (tryThis = 0; tryThis < 3; tryThis++)
    {
      switch (tryThis)
	{
	case 0:
	  if (!(length >= 2))
	    break;
/*Hash function optimized for forward translation */
	  makeHash = (unsigned long int) dots->lowercase << 8;
	  dots2 = findCharOrDots (currentInput[src + 1], 1);
	  makeHash += (unsigned long int) dots2->lowercase;
	  makeHash %= HASHNUM;
	  ruleOffset = table->forRules[makeHash];
	  break;
	case 1:
	  if (!(length >= 1))
	    break;
	  length = 1;
	  ruleOffset = dots->otherRules;
	  break;
	case 2:		/*No rule found */
	  transOpcode = CTO_Always;
	  return;
	  break;
	}
      while (ruleOffset)
	{
	  transRule = (ContractionTableRule *) & table->ruleArea[ruleOffset];
	  transOpcode = transRule->opcode;
	  transCharslen = transRule->charslen;
	  if (tryThis == 1 || ((transCharslen <= length) && checkDots ()))
/* check this rule */
	    switch (transOpcode)
	      {			/*check validity of this contraction */
	      case CTO_Pass2:
		if (currentPass != 2)
		  break;
		if (!for_passDoTest ())
		  break;
		return;
	      case CTO_Pass3:
		if (currentPass != 3)
		  break;
		if (!for_passDoTest ())
		  break;
		return;
	      case CTO_Pass4:
		if (currentPass != 4)
		  break;
		if (!for_passDoTest ())
		  break;
		return;
	      default:
		break;
	      }
	  ruleOffset = transRule->charsnext;
	}
    }
  return;
}

static int
for_translatePass (void)
{
  int k;
  prevTransOpcode = CTO_None;
  src = dest = 0;
  for (k = 0; k < NUMVAR; k++)
    passVariables[k] = 0;
  while (src < srcmax)
    {				/*the main multipass translation loop */
      for_passSelectRule ();
      switch (transOpcode)
	{
	case CTO_Context:
	case CTO_Pass2:
	case CTO_Pass3:
	case CTO_Pass4:
	  if (!for_passDoAction ())
	    goto failure;
	  src = endReplace;
	  break;
	case CTO_Always:
	  if ((dest + 1) > destmax)
	    goto failure;
	  currentOutput[dest++] = currentInput[src++];
	  break;
	default:
	  goto failure;
	}
    }
failure:
  if (src < srcmax)
    {
      while (checkAttr (currentInput[src], CTC_Space, 1))
	if (++src == srcmax)
	  break;
    }
  return 1;
}

int
lou_hyphenate (const char *const trantab, const widechar
	       * const inbuf, int inlen, char *hyphens, int mode)
{
#define HYPHSTRING 100
  widechar workingBuffer[HYPHSTRING];
  int k, kk;
  int wordStart;
  int wordEnd;
  table = lou_getTable (trantab);
  if (table == NULL || inlen >= HYPHSTRING)
    return 0;
  if (mode != 0)
    {
      k = inlen;
      kk = HYPHSTRING;
      if (!lou_backTranslate (trantab, inbuf, &k,
			      &workingBuffer[0],
			      &kk, NULL, NULL, NULL, NULL, NULL, 0))
	return 0;
    }
  else
    {
      memcpy (&workingBuffer[0], inbuf, CHARSIZE * inlen);
      kk = inlen;
    }
  for (wordStart = 0; wordStart < kk; wordStart++)
    if (((findCharOrDots (workingBuffer[wordStart], 0))->attributes &
	 CTC_Letter))
      break;
  if (wordStart == kk)
    return 0;
  for (wordEnd = kk - 1; wordEnd >= 0; wordEnd--)
    if (((findCharOrDots (workingBuffer[wordEnd], 0))->attributes &
	 CTC_Letter))
      break;
  if ((wordEnd - wordStart) < 5)
    return 0;
  for (k = wordStart; k <= wordEnd; k++)
    {
      ContractionTableCharacter *c = findCharOrDots (workingBuffer[k], 0);
      if (!(c->attributes & CTC_Letter))
	return 0;
    }
  if (!hyphenate
      (&workingBuffer[wordStart], wordEnd - wordStart, &hyphens[wordStart]))
    return 0;
  for (k = 0; k < wordStart; k++)
    hyphens[k] = '0';
  if (mode != 0)
    {
      widechar workingBuffer2[HYPHSTRING];
      int outputPos[HYPHSTRING];
      char hyphens2[HYPHSTRING];
      kk = wordEnd - wordStart;
      k = HYPHSTRING;
      if (!lou_translate (trantab, &workingBuffer[wordStart], &kk,
			  &workingBuffer2[0], &k, NULL,
			  NULL, &outputPos[0], NULL, NULL, 0))
	return 0;
      for (kk = 0; kk < k; kk++)
	{
	  int hyphPos = outputPos[kk];
	  if (hyphPos > k || hyphPos < 0)
	    break;
	  if (hyphens[wordStart + kk] & 1)
	    hyphens2[hyphPos] = '1';
	  else
	    hyphens2[hyphPos] = '0';
	}
      for (kk = wordStart; kk < wordStart + k; kk++)
	hyphens[kk] = hyphens2[kk];
      return 1;
    }
  for (k = 0; k < inlen; k++)
    if (hyphens[k] & 1)
      hyphens[k] = '1';
    else
      hyphens[k] = '0';
  return 1;
}
