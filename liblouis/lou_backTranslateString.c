/* liblouis Braille Translation and Back-Translation 
Library

   Based on the Linux screenreader BRLTTY, copyright (C) 1999-2006 by
   The BRLTTY Team

   Copyright (C) 2004, 2005, 2006
   ViewPlus Technologies, Inc. www.viewplus.com
   and
   JJB Software, Inc. www.jjb-software.com
   All rights reserved

   This file is free software; you can redistribute it and/or modify it
   under the terms of the Lesser or Library GNU General Public License 
   as published by the
   Free Software Foundation; either version 3, or (at your option) any
   later version.

   This file is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
   Library GNU General Public License for more details.

   You should have received a copy of the Library GNU General Public 
   License along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Maintained by John J. Boyer john.boyer@jjb-software.com
   */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "louis.h"
#include "transcommon.ci"

static int backTranslateString (void);

int EXPORT_CALL
lou_backTranslateString (const char *trantab, const widechar
			 * inbuf,
			 int *inlen, widechar * outbuf, int *outlen, char
			 *typeform, char *spacing, int modex)
{
  return lou_backTranslate (trantab, inbuf, inlen, outbuf, outlen,
			    typeform, spacing, NULL, NULL, NULL, modex);
}

int EXPORT_CALL
lou_backTranslate (const char *trantab, const
		   widechar
		   * inbuf,
		   int *inlen, widechar * outbuf, int *outlen,
		   char *typeform, char *spacing, int
		   *outputPos, int *inputPos, int *cursorPos, int modex)
{
  int k;
  int goodTrans = 1;
  if ((modex & otherTrans))
    return other_backTranslate (trantab, inbuf,
				inlen, outbuf, outlen,
				typeform, spacing, outputPos, inputPos,
				cursorPos, modex);
  table = lou_getTable (trantab);
  if (table == NULL)
    return 0;
  srcmax = 0;
  while (srcmax < *inlen && inbuf[srcmax])
    srcmax++;
  destmax = *outlen;
  typebuf = (unsigned short *) typeform;
  destSpacing = spacing;
  outputPositions = outputPos;
  if (outputPos != NULL)
    for (k = 0; k < srcmax; k++)
      outputPos[k] = -1;
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
  if (destSpacing != NULL)
    memset (destSpacing, '*', destmax);
  for (k = 0; k < srcmax; k++)
    if ((mode & dotsIO))
      {
	widechar dots;
	dots = inbuf[k];
	if ((dots & 0xff00) == 0x2800)	/*Unicode braille */
	  {
	    dots = (dots & 0x00ff) | B16;
	    passbuf1[k] = getCharFromDots (dots);
	  }
	else
	  passbuf1[k] = inbuf[k] | B16;
      }
    else
      passbuf1[k] = getDotsForChar (inbuf[k]);
  passbuf1[srcmax] = getDotsForChar (' ');
  if (!(srcMapping = liblouis_allocMem (alloc_srcMapping, srcmax, destmax)))
    return 0;
  for (k = 0; k <= srcmax; k++)
    srcMapping[k] = k;
  srcMapping[srcmax] = srcmax;
  currentInput = passbuf1;
  if ((!(mode & pass1Only)) && (table->numPasses > 1 || table->corrections))
    {
      if (!(passbuf2 = liblouis_allocMem (alloc_passbuf2, srcmax, destmax)))
	return 0;
    }
  currentPass = table->numPasses;
  if ((mode & pass1Only))
    {
      currentOutput = outbuf;
      goodTrans = backTranslateString ();
    }
  else
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
    *inlen = srcMapping[src];
  *outlen = dest;
  if (outputPos != NULL)
    {
      int lastpos = 0;
      for (k = 0; k < *inlen; k++)
	if (outputPos[k] == -1)
	  outputPos[k] = lastpos;
	else
	  lastpos = outputPos[k];
    }
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
static int prevSrc;
static TranslationTableOpcode currentOpcode;
static TranslationTableOpcode prevOpcode;
static const TranslationTableRule *currentRule;

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
static TranslationTableCharacterAttributes beforeAttributes;
static TranslationTableCharacterAttributes afterAttributes;
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
      const TranslationTableCharacter *ch =
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
  const TranslationTableCharacter *dots;
  TranslationTableOffset testRuleOffset;
  TranslationTableRule *testRule;
  for (k = src + currentDotslen; k < srcmax; k++)
    {
      int postpuncFound = 0;
      int TranslationFound = 0;
      dots = findCharOrDots (currentInput[k], 1);
      testRuleOffset = dots->otherRules;
      if (dots->attributes & CTC_Space)
	break;
      if (dots->attributes & CTC_Letter)
	return 0;
      while (testRuleOffset)
	{
	  testRule =
	    (TranslationTableRule *) & table->ruleArea[testRuleOffset];
	  if (testRule->charslen > 1)
	    TranslationFound = 1;
	  if (testRule->opcode == CTO_PostPunc)
	    postpuncFound = 1;
	  if (testRule->opcode == CTO_Hyphen)
	    return 1;
	  testRuleOffset = testRule->dotsnext;
	}
      if (TranslationFound && !postpuncFound)
	return 0;
    }
  return 1;
}

static int
findBrailleIndicatorRule (TranslationTableOffset offset)
{
  if (!offset)
    return 0;
  currentRule = (TranslationTableRule *) & table->ruleArea[offset];
  currentOpcode = currentRule->opcode;
  currentDotslen = currentRule->dotslen;
  return 1;
}

static int doingMultind = 0;
static const TranslationTableRule *multindRule;

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

static void
back_selectRule (void)
{
/*check for valid back-translations */
  int length = srcmax - src;
  TranslationTableOffset ruleOffset = 0;
  static TranslationTableRule pseudoRule = { 0 };
  unsigned long int makeHash = 0;
  const TranslationTableCharacter *dots =
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
		    (TranslationTableRule *) & table->ruleArea[ruleOffset];
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
	    (TranslationTableRule *) & table->ruleArea[ruleOffset];
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
		    {		/*check validity of this Translation */
		    case CTO_Space:
		    case CTO_Digit:
		    case CTO_Letter:
		    case CTO_UpperCase:
		    case CTO_LowerCase:
		    case CTO_Punctuation:
		    case CTO_Math:
		    case CTO_Sign:
		    case CTO_Always:
		    case CTO_ExactDots:
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
			    CTC_Letter) && (afterAttributes & CTC_Letter))
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
			  (prevOpcode != CTO_JoinableWord))
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
		      if (beforeAttributes & CTC_Letter || !isEndWord ())
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
  if (!cursorStatus && cursorPosition >= src &&
      cursorPosition < (src + inLength))
    {
      cursorPosition = dest + outLength / 2;
      cursorStatus = 1;
    }
  if (inputPositions != NULL || outputPositions != NULL)
    {
      if (outLength <= inLength)
	{
	  for (k = 0; k < outLength; k++)
	    {
	      if (inputPositions != NULL)
		inputPositions[dest + k] = srcMapping[src + k];
	      if (outputPositions != NULL)
		outputPositions[srcMapping[src + k]] = dest + k;
	    }
	  for (k = outLength; k < inLength; k++)
	    if (outputPositions != NULL)
	      outputPositions[srcMapping[src + k]] = dest + outLength - 1;
	}
      else
	{
	  for (k = 0; k < inLength; k++)
	    {
	      if (inputPositions != NULL)
		inputPositions[dest + k] = srcMapping[src + k];
	      if (outputPositions != NULL)
		outputPositions[srcMapping[src + k]] = dest + k;
	    }
	  for (k = inLength; k < outLength; k++)
	    if (inputPositions != NULL)
	      inputPositions[dest + k] = srcMapping[src + inLength - 1];
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
  TranslationTableOffset offset = (findCharOrDots (dots, 0))->definitionRule;
  if (offset)
    {
      widechar c;
      const TranslationTableRule *rule = (TranslationTableRule
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
  widechar c = ' ';
  if (!back_updatePositions (&c, 1, 1))
    return 0;
  if (destSpacing)
    destSpacing[dest - 1] = '1';
  return 1;
}

static int
backTranslateString (void)
{
/*Back translation */
  int srcword = 0;
  int destword = 0;		/* last word translated */
  nextUpper = allUpper = itsANumber = itsALetter = itsCompbrl = 0;
  prevOpcode = CTO_None;
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
	  if (prevOpcode == CTO_LargeSign)
	    if (!insertSpace ())
	      goto failure;
	  break;
	case CTO_CapitalRule:
	  nextUpper = 1;
	  src += currentDotslen;
	  continue;
	case CTO_BeginCapitalRule:
	  allUpper = 1;
	  src += currentDotslen;
	  continue;
	case CTO_EndCapitalRule:
	  allUpper = 0;
	  src += currentDotslen;
	  continue;
	case CTO_LetterRule:
	  itsALetter = 1;
	  itsANumber = 0;
	  src += currentDotslen;
	  continue;
	case CTO_NumberRule:
	  itsANumber = 1;
	  src += currentDotslen;
	  continue;
	case CTO_FirstLetterItalRule:
	  currentTypeform = italic;
	  src += currentDotslen;
	  continue;
	case CTO_LastLetterItalRule:
	  currentTypeform = plain_text;
	  src += currentDotslen;
	  continue;
	case CTO_FirstLetterBoldRule:
	  currentTypeform = bold;
	  src += currentDotslen;
	  continue;
	case CTO_LastLetterBoldRule:
	  currentTypeform = plain_text;
	  src += currentDotslen;
	  continue;
	case CTO_FirstLetterUnderRule:
	  currentTypeform = underline;
	  src += currentDotslen;
	  continue;
	case CTO_LastLetterUnderRule:
	  currentTypeform = plain_text;
	  src += currentDotslen;
	  continue;
	case CTO_BegCompRule:
	  itsCompbrl = 1;
	  currentTypeform = computer_braille;
	  src += currentDotslen;
	  continue;
	case CTO_EndCompRule:
	  itsCompbrl = 0;
	  currentTypeform = plain_text;
	  src += currentDotslen;
	  continue;
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
	prevOpcode = currentOpcode;
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
