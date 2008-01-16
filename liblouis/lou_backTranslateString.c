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

static const ContractionTableHeader *table;	/*translation table */
static int src, srcmax;
static int dest, destmax;
static int mode;
static int currentPass = 1;
static widechar *currentInput;
static widechar *passbuf1 = NULL;
static widechar *passbuf2 = NULL;
static widechar *currentOutput;
static unsigned char *typebuf = NULL;
static char *spacebuf;
static int backTranslateString (void);
static int makeCorrections (void);
static int translatePass (void);

static int *outputPositions;
static int *inputPositions;
static int cursorPosition;
static int cursorStatus;

int
lou_backTranslateString (const char *const trantab, const widechar
			 * const inbuf,
			 int *inlen, widechar * outbuf, int *outlen, char
			 *typeform, char *spacing, int modex)
{
  return lou_backTranslate (trantab, inbuf, inlen, outbuf, outlen,
			    typeform, spacing, NULL, NULL, NULL, modex);
}

int
lou_backTranslate (const char *const trantab, const
		   widechar
		   * const inbuf,
		   int *inlen, widechar * outbuf, int *outlen,
		   char *typeform, char *spacing, int
		   *outputPos, int *inputPos, int *cursorPos, int modex)
{
  int k;
  int goodTrans = 1;
  table = lou_getTable (trantab);
  if (table == NULL)
    return 0;
  srcmax = 0;
  while (srcmax < *inlen && inbuf[srcmax])
    srcmax++;
  destmax = *outlen;
  typebuf = (unsigned char *) typeform;
  spacebuf = spacing;
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
  if (typebuf != NULL)
    memset (typebuf, '0', destmax);
  if (spacebuf != NULL)
    memset (spacebuf, '*', destmax);
  for (k = 0; k < srcmax; k++)
    if ((mode & dotsIO))
      passbuf1[k] = inbuf[k] | 0x8000;
    else
      passbuf1[k] = getDotsForChar (inbuf[k]);
  passbuf1[srcmax] = getDotsForChar (' ');
  currentInput = passbuf1;
  if (table->numPasses > 1 || table->corrections)
    {
      if (!(passbuf2 = liblouis_allocMem (alloc_passbuf2, srcmax, destmax)))
	return 0;
    }
  currentPass = table->numPasses;
  switch (table->numPasses + (table->corrections << 3))
    {
    case 1:
      currentOutput = outbuf;
      goodTrans = backTranslateString ();
      break;
    case 2:
      currentOutput = passbuf2;
      goodTrans = translatePass ();
      if (!goodTrans)
	break;
      currentPass--;
      srcmax = dest;
      currentInput = passbuf2;
      currentOutput = outbuf;
      goodTrans = backTranslateString ();
      break;
    case 3:
      currentOutput = passbuf2;
      goodTrans = translatePass ();
      if (!goodTrans)
	break;
      currentPass--;
      srcmax = dest;
      currentInput = passbuf2;
      currentOutput = passbuf1;
      goodTrans = translatePass ();
      if (!goodTrans)
	break;
      currentInput = passbuf1;
      currentOutput = outbuf;
      currentPass--;
      srcmax = src;
      goodTrans = backTranslateString ();
      break;
    case 4:
      currentOutput = passbuf2;
      goodTrans = translatePass ();
      if (!goodTrans)
	break;
      currentPass--;
      srcmax = dest;
      currentInput = passbuf2;
      currentOutput = passbuf1;
      goodTrans = translatePass ();
      if (!goodTrans)
	break;
      currentInput = passbuf1;
      currentOutput = passbuf2;
      srcmax = dest;
      currentPass--;
      goodTrans = translatePass ();
      if (!goodTrans)
	break;
      currentInput = passbuf2;
      currentOutput = outbuf;
      currentPass--;
      srcmax = dest;
      goodTrans = backTranslateString ();
      break;
    case 9:
      currentOutput = passbuf2;
      goodTrans = backTranslateString ();
      if (!goodTrans)
	break;
      currentInput = passbuf2;
      currentOutput = outbuf;
      currentPass--;
      srcmax = dest;
      goodTrans = makeCorrections ();
      break;
    case 10:
      currentOutput = passbuf2;
      goodTrans = translatePass ();
      if (!goodTrans)
	break;
      currentPass--;
      srcmax = dest;
      currentInput = passbuf2;
      currentOutput = passbuf1;
      goodTrans = backTranslateString ();
      if (!goodTrans)
	break;
      currentInput = passbuf1;
      currentOutput = outbuf;
      currentPass--;
      srcmax = dest;
      goodTrans = makeCorrections ();
      break;
    case 11:
      currentOutput = passbuf2;
      goodTrans = translatePass ();
      if (!goodTrans)
	break;
      currentPass--;
      srcmax = dest;
      currentInput = passbuf2;
      currentOutput = passbuf1;
      goodTrans = translatePass ();
      if (!goodTrans)
	break;
      currentInput = passbuf1;
      currentOutput = passbuf2;
      currentPass--;
      srcmax = dest;
      goodTrans = backTranslateString ();
      if (!goodTrans)
	break;
      currentInput = passbuf2;
      currentOutput = outbuf;
      currentPass--;
      srcmax = dest;
      goodTrans = makeCorrections ();
      break;
    case 12:
      currentOutput = passbuf2;
      goodTrans = translatePass ();
      if (!goodTrans)
	break;
      currentPass--;
      srcmax = dest;
      currentInput = passbuf2;
      currentOutput = passbuf1;
      goodTrans = translatePass ();
      if (!goodTrans)
	break;
      currentInput = passbuf1;
      currentOutput = passbuf2;
      srcmax = dest;
      currentPass--;
      goodTrans = translatePass ();
      if (!goodTrans)
	break;
      currentInput = passbuf2;
      currentOutput = passbuf1;
      currentPass--;
      srcmax = dest;
      goodTrans = backTranslateString ();
      if (!goodTrans)
	break;
      currentInput = passbuf1;
      currentOutput = outbuf;
      currentPass--;
      srcmax = dest;
      goodTrans = makeCorrections ();
      break;
    default:
      break;
    }
  if (src < *inlen)
    *inlen = src;
  *outlen = dest;
  if (cursorPos != NULL)
    *cursorPos = cursorPosition;
  return goodTrans;
}

static char currentTypeform = plain_text;
static int nextUpper = 0;
static int allUpper = 0;
static int itsANumber = 0;
static int itsALetter = 0;
static int itsCompbrl = 0;
static int currentCharslen;
static int currentDotslen;	/*length of current find string */
static int previousSrc;
static ContractionTableOpcode currentOpcode;
static ContractionTableOpcode previousOpcode;
static const ContractionTableRule *currentRule;	/*pointer to current rule in 
						   table */
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
  notFound->realchar = notFound->uppercase = notFound->lowercase = c;
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
compareDots (const widechar * address1, const widechar * address2, int count)
{
  int k;
  if (!count)
    return 0;
  for (k = 0; k < count; k++)
    if (address1[k] != address2[k])
      return 0;
  return 1;
}

static widechar before, after;
static ContractionTableCharacterAttributes beforeAttributes;
static ContractionTableCharacterAttributes afterAttributes;
static void
back_setBefore (void)
{
  before = (dest == 0) ? ' ' : currentOutput[dest - 1];
  beforeAttributes = (findCharOrDots (before, 0))->attributes;
}

static void
back_setAfter (int length)
{
  after = (src + length < srcmax) ? currentInput[src + length] : ' ';
  afterAttributes = (findCharOrDots (after, 1))->attributes;
}


static int
isBegWord (void)
{
/*See if this is really the beginning of a word. Look at what has 
* already been translated. */
  int k;
  if (dest == 0)
    return 1;
  for (k = dest - 1; k >= 0; k--)
    {
      const ContractionTableCharacter *ch =
	findCharOrDots (currentOutput[k], 0);
      if (ch->attributes & CTC_Space)
	break;
      if (ch->attributes & (CTC_Letter | CTC_Digit | CTC_Math | CTC_Sign))
	return 0;
    }
  return 1;
}

static int
isEndWord (void)
{
/*See if this is really the end of a word. */
  int k;
  const ContractionTableCharacter *dots;
  ContractionTableOffset testRuleOffset;
  ContractionTableRule *testRule;
  for (k = src + currentDotslen; k < srcmax; k++)
    {
      int postpuncFound = 0;
      int contractionFound = 0;
      dots = findCharOrDots (currentInput[k], 1);
      testRuleOffset = dots->otherRules;
      if (dots->attributes & CTC_Space)
	break;
      if (dots->attributes & CTC_Letter)
	return 0;
      while (testRuleOffset)
	{
	  testRule =
	    (ContractionTableRule *) & table->ruleArea[testRuleOffset];
	  if (testRule->charslen > 1)
	    contractionFound = 1;
	  if (testRule->opcode == CTO_PostPunc)
	    postpuncFound = 1;
	  if (testRule->opcode == CTO_Hyphen)
	    return 1;
	  testRuleOffset = testRule->dotsnext;
	}
      if (contractionFound && !postpuncFound)
	return 0;
    }
  return 1;
}

static int
findBrailleIndicatorRule (ContractionTableOffset offset)
{
  if (!offset)
    return 0;
  currentRule = (ContractionTableRule *) & table->ruleArea[offset];
  currentOpcode = currentRule->opcode;
  currentDotslen = currentRule->dotslen;
  return 1;
}

static int doingMultind = 0;
static const ContractionTableRule *multindRule;

static int
handleMultind (void)
{
/*Handle multille braille indicators*/
  int found = 0;
  if (!doingMultind)
    return 0;
  switch (multindRule->charsdots[multindRule->charslen - doingMultind])
    {
    case CTO_CapitalSign:
      found = findBrailleIndicatorRule (table->capitalSign);
      break;
    case CTO_BeginCapitalSign:
      found = findBrailleIndicatorRule (table->beginCapitalSign);
      break;
    case CTO_EndCapitalSign:
      found = findBrailleIndicatorRule (table->endCapitalSign);
      break;
    case CTO_LetterSign:
      found = findBrailleIndicatorRule (table->letterSign);
      break;
    case CTO_NumberSign:
      found = findBrailleIndicatorRule (table->numberSign);
      break;
    case CTO_LastWordItalBefore:
      found = findBrailleIndicatorRule (table->lastWordItalBefore);
      break;
    case CTO_BegItal:
      found = findBrailleIndicatorRule (table->firstLetterItal);
      break;
    case CTO_LastLetterItal:
      found = findBrailleIndicatorRule (table->lastLetterItal);
      break;
    case CTO_LastWordBoldBefore:
      found = findBrailleIndicatorRule (table->lastWordBoldBefore);
      break;
    case CTO_FirstLetterBold:
      found = findBrailleIndicatorRule (table->firstLetterBold);
      break;
    case CTO_LastLetterBold:
      found = findBrailleIndicatorRule (table->lastLetterBold);
      break;
    case CTO_LastWordUnderBefore:
      found = findBrailleIndicatorRule (table->lastWordUnderBefore);
      break;
    case CTO_FirstLetterUnder:
      found = findBrailleIndicatorRule (table->firstLetterUnder);
      break;
    case CTO_EndUnder:
      found = findBrailleIndicatorRule (table->lastLetterUnder);
      break;
    case CTO_BegComp:
      found = findBrailleIndicatorRule (table->begComp);
      break;
    case CTO_EndComp:
      found = findBrailleIndicatorRule (table->endComp);
      break;
    default:
      found = 0;
      break;
    }
  doingMultind--;
  return found;
}

static int passVariables[NUMVAR];
static int passSrc;
static const widechar const *passInstructions;
static int passIC;		/*Instruction counter */
static int startMatch;
static int endMatch;
static int startReplace;
static int endReplace;

static int back_passDoTest (void);
static int back_passDoAction (void);

static int
findAttribOrSwapRules (void)
{
  ContractionTableOffset ruleOffset;
  if (src == previousSrc)
    return 0;
  ruleOffset = table->attribOrSwapRules[currentPass];
  currentCharslen = 0;
  while (ruleOffset)
    {
      currentRule = (ContractionTableRule *) & table->ruleArea[ruleOffset];
      currentOpcode = currentRule->opcode;
      if (back_passDoTest ())
	return 1;
      ruleOffset = currentRule->charsnext;
    }
  return 0;
}

static void
back_selectRule (void)
{
/*check for valid bcak-translations */
  int length = srcmax - src;
  ContractionTableOffset ruleOffset = 0;
  static ContractionTableRule pseudoRule = { 0 };
  unsigned long int makeHash = 0;
  const ContractionTableCharacter *dots =
    findCharOrDots (currentInput[src], 1);
  int tryThis;
  if (handleMultind ())
    return;
  for (tryThis = 0; tryThis < 3; tryThis++)
    {
      switch (tryThis)
	{
	case 0:
	  if (length < 2 || (itsANumber && (dots->attributes & CTC_LitDigit)))
	    break;
	  /*Hash function optimized for backward translation */
	  makeHash = (unsigned long int) dots->lowercase << 8;
	  makeHash += (unsigned long int) (findCharOrDots
					   (currentInput[src + 1],
					    1))->lowercase;
	  makeHash %= HASHNUM;
	  ruleOffset = table->backRules[makeHash];
	  break;
	case 1:
	  if (!(length >= 1))
	    break;
	  length = 1;
	  ruleOffset = dots->otherRules;
	  if (itsANumber)
	    {
	      while (ruleOffset)
		{
		  currentRule =
		    (ContractionTableRule *) & table->ruleArea[ruleOffset];
		  if (currentRule->opcode == CTO_LitDigit)
		    {
		      currentOpcode = currentRule->opcode;
		      currentDotslen = currentRule->dotslen;
		      return;
		    }
		  ruleOffset = currentRule->dotsnext;
		}
	      ruleOffset = dots->otherRules;
	    }
	  break;
	case 2:		/*No rule found */
	  currentRule = &pseudoRule;
	  currentOpcode = pseudoRule.opcode = CTO_None;
	  currentDotslen = pseudoRule.dotslen = 1;
	  pseudoRule.charsdots[0] = currentInput[src];
	  pseudoRule.charslen = 0;
	  return;
	  break;
	}
      while (ruleOffset)
	{
	  currentRule =
	    (ContractionTableRule *) & table->ruleArea[ruleOffset];
	  currentOpcode = currentRule->opcode;
	  currentDotslen = currentRule->dotslen;
	  if (((currentDotslen <= length) &&
	       compareDots (&currentInput[src],
			    &currentRule->charsdots[currentRule->charslen],
			    currentDotslen)))
	    {
	      /* check this rule */
	      back_setAfter (currentDotslen);
	      if ((!currentRule->after || (beforeAttributes
					   & currentRule->after)) &&
		  (!currentRule->before || (afterAttributes
					    & currentRule->before)))
		{
		  switch (currentOpcode)
		    {		/*check validity of this contraction */
		    case CTO_Space:
		    case CTO_Digit:
		    case CTO_Letter:
		    case CTO_UpperCase:
		    case CTO_LowerCase:
		    case CTO_Punctuation:
		    case CTO_Math:
		    case CTO_Sign:
		    case CTO_Always:
		    case CTO_NoCross:
		    case CTO_Repeated:
		    case CTO_Replace:
		    case CTO_Hyphen:
		      return;
		    case CTO_LitDigit:
		      if (itsANumber)
			return;
		      break;
		    case CTO_CapitalRule:
		    case CTO_BeginCapitalRule:
		    case CTO_EndCapitalRule:
		    case CTO_FirstLetterItalRule:
		    case CTO_LastLetterItalRule:
		    case CTO_LastWordBoldBeforeRule:
		    case CTO_LastLetterBoldRule:
		    case CTO_FirstLetterUnderRule:
		    case CTO_LastLetterUnderRule:
		    case CTO_NumberRule:
		    case CTO_BegCompRule:
		    case CTO_EndCompRule:
		      return;
		    case CTO_LetterRule:
		      if (!(beforeAttributes &
			    CTO_Letter) && (afterAttributes & CTC_Letter))
			return;
		      break;
		    case CTO_MultInd:
		      doingMultind = currentDotslen;
		      multindRule = currentRule;
		      if (handleMultind ())
			return;
		      break;
		    case CTO_LargeSign:
		      return;
		    case CTO_WholeWord:
		      if (itsALetter)
			break;
		    case CTO_Contraction:
		      if ((beforeAttributes & (CTC_Space | CTC_Punctuation))
			  && ((afterAttributes & CTC_Space) || isEndWord ()))
			return;
		      break;
		    case CTO_LowWord:
		      if ((beforeAttributes & CTC_Space) && (afterAttributes
							     & CTC_Space) &&
			  (previousOpcode != CTO_JoinableWord))
			return;
		      break;
		    case CTO_JoinNum:
		    case CTO_JoinableWord:
		      if ((beforeAttributes & (CTC_Space |
					       CTC_Punctuation))
			  && !((afterAttributes & CTC_Space)))
			return;
		      break;
		    case CTO_SuffixableWord:
		      if ((beforeAttributes & (CTC_Space | CTC_Punctuation))
			  &&
			  ((afterAttributes & (CTC_Space | CTC_Letter))
			   || isEndWord ()))
			return;
		      break;
		    case CTO_PrefixableWord:
		      if ((beforeAttributes & (CTC_Space | CTC_Letter |
					       CTC_Punctuation))
			  && isEndWord ())
			return;
		      break;
		    case CTO_BegWord:
		      if ((beforeAttributes & (CTC_Space | CTC_Punctuation))
			  && (!isEndWord ()))
			return;
		      break;
		    case CTO_BegMidWord:
		      if ((beforeAttributes & (CTC_Letter | CTC_Space |
					       CTC_Punctuation))
			  && (!isEndWord ()))
			return;
		      break;
		    case CTO_PartWord:
		      if (beforeAttributes & CTC_Letter || !isEndWord 
())
			return;
		      break;
		    case CTO_MidWord:
		      if (beforeAttributes & CTC_Letter && !isEndWord ())
			return;
		      break;
		    case CTO_MidEndWord:
		      if ((beforeAttributes & CTC_Letter))
			return;
		      break;
		    case CTO_EndWord:
		      if ((beforeAttributes & CTC_Letter) && isEndWord ())
			return;
		      break;
		    case CTO_BegNum:
		      if (beforeAttributes & (CTC_Space | CTC_Punctuation)
			  && (afterAttributes & (CTC_LitDigit | CTC_Sign)))
			return;
		      break;
		    case CTO_MidNum:
		      if (beforeAttributes & CTC_Digit &&
			  afterAttributes & CTC_LitDigit)
			return;
		      break;
		    case CTO_EndNum:
		      if (itsANumber && !(afterAttributes & CTC_LitDigit))
			return;
		      break;
		    case CTO_DecPoint:
		      if (afterAttributes & (CTC_Digit | CTC_LitDigit))
			return;
		      break;
		    case CTO_PrePunc:
		      if (isBegWord ())
			return;
		      break;

		    case CTO_PostPunc:
		      if (isEndWord ())
			return;
		      break;
		    default:
		      break;
		    }
		}
	    }			/*Done with checking this rule */
	  ruleOffset = currentRule->dotsnext;
	}
    }
}

static int
putchars (const widechar * chars, int count)
{
  int k = 0;
  if (!count || (dest + count) > destmax)
    return 0;
  if (nextUpper)
    {
      currentOutput[dest++] = (findCharOrDots (chars[k++], 0))->uppercase;
      nextUpper = 0;
    }
  if (!allUpper)
    {
      memcpy (&currentOutput[dest], &chars[k], CHARSIZE * (count - k));
      dest += count - k;
    }
  else
    for (; k < count; k++)
      currentOutput[dest++] = (findCharOrDots (chars[k], 0))->uppercase;
  return 1;
}

static int
back_updatePositions (const widechar * outChars, int inLength, int outLength)
{
  int k;
  if ((dest + outLength) > destmax || (src + inLength) > srcmax)
    return 0;
  if (cursorPosition >= 0)
    {
      if (cursorPosition >= src && cursorPosition < (src + inLength))
	cursorPosition = dest + outLength / 2;
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
  return putchars (outChars, outLength);
}

static int
undefinedDots (widechar dots)
{
/*Print out dot numbers */
  widechar buffer[20];
  int k = 1;
  buffer[0] = '\\';
  if ((dots & B1))
    buffer[k++] = '1';
  if ((dots & B2))
    buffer[k++] = '2';
  if ((dots & B3))
    buffer[k++] = '3';
  if ((dots & B4))
    buffer[k++] = '4';
  if ((dots & B5))
    buffer[k++] = '5';
  if ((dots & B6))
    buffer[k++] = '6';
  if ((dots & B7))
    buffer[k++] = '7';
  if ((dots & B8))
    buffer[k++] = '8';
  if ((dots & B9))
    buffer[k++] = '9';
  if ((dots & B10))
    buffer[k++] = 'A';
  if ((dots & B11))
    buffer[k++] = 'B';
  if ((dots & B12))
    buffer[k++] = 'C';
  if ((dots & B13))
    buffer[k++] = 'D';
  if ((dots & B14))
    buffer[k++] = 'E';
  if ((dots & B15))
    buffer[k++] = 'F';
  buffer[k++] = '/';
  if ((dest + k) > destmax)
    return 0;
  memcpy (&currentOutput[dest], buffer, k * CHARSIZE);
  dest += k;
  return 1;
}

static int
putCharacter (widechar dots)
{
/*Output character(s) corresponding to a Unicode braille Character*/
  ContractionTableOffset offset = (findCharOrDots (dots, 0))->definitionRule;
  if (offset)
    {
      widechar c;
      const ContractionTableRule *rule = (ContractionTableRule
					  *) & table->ruleArea[offset];
      if (rule->charslen)
	return back_updatePositions (&rule->charsdots[0],
				     rule->dotslen, rule->charslen);
      c = getCharFromDots (dots);
      return back_updatePositions (&c, 1, 1);
    }
  return undefinedDots (dots);
}

static int
putCharacters (const widechar * characters, int count)
{
  int k;
  for (k = 0; k < count; k++)
    if (!putCharacter (characters[k]))
      return 0;
  return 1;
}

static int
insertSpace (void)
{
  ContractionTableRule *spaceRule =
    (ContractionTableRule *) & table->ruleArea[(findCharOrDots
						(B16, 1))->definitionRule];
  if (!spaceRule->charslen)
    return 1;
  if (!back_updatePositions (&spaceRule->charsdots[0],
			     spaceRule->dotslen, spaceRule->charslen))
    return 0;
  if (spacebuf)
    spacebuf[dest - 1] = '1';
  return 1;
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
		currentOpcode = CTO_Always;
		ruleOffset = 0;
		break;
	      }
	    while (ruleOffset)
	      {
		currentRule =
		  (ContractionTableRule *) & table->ruleArea[ruleOffset];
		currentOpcode = currentRule->opcode;
		currentCharslen = currentRule->charslen;
		if (tryThis == 1 || (currentCharslen <= length &&
				     compareChars (&currentRule->
						   charsdots[0],
						   &currentInput[src],
						   currentCharslen, 0)))
		  {
		    if (currentOpcode == CTO_Correct && back_passDoTest ())
		      {
			tryThis = 4;
			break;
		      }
		  }
		ruleOffset = currentRule->charsnext;
	      }
	    tryThis++;
	  }
      switch (currentOpcode)
	{
	case CTO_Always:
	  if (dest >= destmax)
	    goto failure;
	  currentOutput[dest++] = currentInput[src++];
	  break;
	case CTO_Correct:
	  if (!back_passDoAction ())
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
backTranslateString (void)
{
/*Back translation */
  int srcword = 0;
  int destword = 0;		/* last word translated */
  nextUpper = allUpper = itsANumber = itsALetter = itsCompbrl = 0;
  previousOpcode = CTO_None;
  src = dest = 0;
  while (src < srcmax)
    {
/*the main translation loop */
      back_setBefore ();
      back_selectRule ();
      /* processing before replacement */
      switch (currentOpcode)
	{
	case CTO_Hyphen:
	  if (isEndWord ())
	    itsANumber = 0;
	  break;
	case CTO_LargeSign:
	  if (previousOpcode == CTO_LargeSign)
	    if (!insertSpace ())
	      goto failure;
	  break;
	case CTO_CapitalRule:
	  nextUpper = 1;
	  src += currentDotslen;
	  continue;
	  break;
	case CTO_BeginCapitalRule:
	  allUpper = 1;
	  src += currentDotslen;
	  continue;
	  break;
	case CTO_EndCapitalRule:
	  allUpper = 0;
	  src += currentDotslen;
	  continue;
	  break;
	case CTO_LetterRule:
	  itsALetter = 1;
	  itsANumber = 0;
	  src += currentDotslen;
	  continue;
	  break;
	case CTO_NumberRule:
	  itsANumber = 1;
	  src += currentDotslen;
	  continue;
	  break;
	case CTO_FirstLetterItalRule:
	  currentTypeform = italic;
	  src += currentDotslen;
	  continue;
	  break;
	case CTO_LastLetterItalRule:
	  currentTypeform = plain_text;
	  src += currentDotslen;
	  continue;
	  break;
	case CTO_FirstLetterBoldRule:
	  currentTypeform = bold;
	  src += currentDotslen;
	  continue;
	  break;
	case CTO_LastLetterBoldRule:
	  currentTypeform = plain_text;
	  src += currentDotslen;
	  continue;
	  break;
	case CTO_FirstLetterUnderRule:
	  currentTypeform = underline;
	  src += currentDotslen;
	  continue;
	  break;
	case CTO_LastLetterUnderRule:
	  currentTypeform = plain_text;
	  src += currentDotslen;
	  continue;
	  break;
	case CTO_BegCompRule:
	  itsCompbrl = 1;
	  currentTypeform = computer_braille;
	  src += currentDotslen;
	  continue;
	  break;
	case CTO_EndCompRule:
	  itsCompbrl = 0;
	  currentTypeform = plain_text;
	  src += currentDotslen;
	  continue;
	  break;

	default:
	  break;
	}

      /* replacement processing */
      switch (currentOpcode)
	{
	case CTO_Replace:
	  src += currentDotslen;
	  if (!putCharacters
	      (&currentRule->charsdots[0], currentRule->charslen))
	    goto failure;
	  break;
	case CTO_None:
	  if (!undefinedDots (currentInput[src]))
	    goto failure;
	  src++;
	  break;
	case CTO_BegNum:
	  itsANumber = 1;
	  goto insertChars;
	case CTO_EndNum:
	  itsANumber = 0;
	  goto insertChars;
	case CTO_Space:
	  itsANumber = allUpper = nextUpper = 0;
	default:
	insertChars:
	  if (currentRule->charslen)
	    {
	      if (!back_updatePositions
		  (&currentRule->charsdots[0],
		   currentRule->dotslen, currentRule->charslen))
		goto failure;
	      src += currentDotslen;
	    }
	  else
	    {
	      int srclim = src + currentDotslen;
	      while (1)
		{
		  if (!putCharacter (currentInput[src]))
		    goto failure;
		  if (++src == srclim)
		    break;
		}
	    }
	}

      /* processing after replacement */
      switch (currentOpcode)
	{
	case CTO_JoinNum:
	case CTO_JoinableWord:
	  if (!insertSpace ())
	    goto failure;
	  break;
	default:
	  break;
	}
      if (((src > 0) && checkAttr (currentInput[src - 1], CTC_Space, 1)
	   && (currentOpcode != CTO_JoinableWord)))
	{
	  srcword = src;
	  destword = dest;
	}
      if ((currentOpcode >= CTO_Always && currentOpcode <= CTO_None) ||
	  (currentOpcode >= CTO_Digit && currentOpcode <= CTO_LitDigit))
	previousOpcode = currentOpcode;
    }				/*end of translation loop */
failure:

  if (destword != 0 && src < srcmax && !checkAttr (currentInput[src],
						   CTC_Space, 1))
    {
      src = srcword;
      dest = destword;
    }
  if (src < srcmax)
    {
      while (checkAttr (currentInput[src], CTC_Space, 1))
	if (++src == srcmax)
	  break;
    }
  return 1;
}				/*translation completed */

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
back_passDoTest ()
{
  int k;
  int m;
  int not = 0;
  ContractionTableCharacterAttributes attributes;
  passSrc = src;
  passInstructions = &currentRule->charsdots[currentCharslen];
  passIC = 0;
  startMatch = passSrc;
  startReplace = -1;
  if (currentOpcode == CTO_Correct)
    m = 0;
  else
    m = 1;
  while (passIC < currentRule->dotslen)
    {
      int itsTrue = 1;
      if (passSrc > srcmax)
	return 0;
      switch (passInstructions[passIC])
	{
	case pass_lookback:
	  passSrc -= passInstructions[passIC + 1];
	  if (passSrc < -1)
	    passSrc = -1;
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
back_passDoAction (void)
{
  int k;
  if ((dest + startReplace - startMatch) > destmax)
    return 0;
  for (k = startMatch; k < startReplace; k++)
    currentOutput[dest++] = currentInput[k];
  while (passIC < currentRule->dotslen)
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
  for (k = 0; k < currentCharslen; k++)
    if (currentRule->charsdots[k] != currentInput[kk++])
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
	  currentOpcode = CTO_Always;
	  return;
	  break;
	}
      while (ruleOffset)
	{
	  currentRule =
	    (ContractionTableRule *) & table->ruleArea[ruleOffset];
	  currentOpcode = currentRule->opcode;
	  currentCharslen = currentRule->charslen;
	  if (tryThis == 1 || ((currentCharslen <= length) && checkDots ()))
/* check this rule */
	    switch (currentOpcode)
	      {			/*check validity of this contraction */
	      case CTO_Pass2:
		if (currentPass != 2)
		  break;
		if (!back_passDoTest ())
		  break;
		return;
	      case CTO_Pass3:
		if (currentPass != 3)
		  break;
		if (!back_passDoTest ())
		  break;
		return;
	      case CTO_Pass4:
		if (currentPass != 4)
		  break;
		if (!back_passDoTest ())
		  break;
		return;
	      default:
		break;
	      }
	  ruleOffset = currentRule->charsnext;
	}
    }
  return;
}

static int
translatePass (void)
{
  int k;
  previousOpcode = CTO_None;
  src = dest = 0;
  for (k = 0; k < NUMVAR; k++)
    passVariables[k] = 0;
  while (src < srcmax)
    {				/*the main multipass translation loop */
      for_passSelectRule ();
      switch (currentOpcode)
	{
	case CTO_Pass2:
	case CTO_Pass3:
	case CTO_Pass4:
	  if (!back_passDoAction ())
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
