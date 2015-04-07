/* liblouis Braille Translation and Back-Translation Library

   Based on the Linux screenreader BRLTTY, copyright (C) 1999-2006 by
   The BRLTTY Team

   Copyright (C) 2004, 2005, 2006
   ViewPlus Technologies, Inc. www.viewplus.com
   and
   abilitiessoft, Inc. www.abilitiessoft.com
   All rights reserved

   This file is part of Liblouis.

   Liblouis is free software: you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   Liblouis is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with Liblouis. If not, see
   <http://www.gnu.org/licenses/>.

   Maintained by John J. Boyer john.boyer@abilitiessoft.com
   */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "louis.h"
#include "transcommon.ci"

#define MIN(a,b) (((a)<(b))?(a):(b))

static int translateString ();
static int compbrlStart = 0;
static int compbrlEnd = 0;

static int numericMode = 0;

int EXPORT_CALL
lou_translateString (const char *tableList, const widechar
		     * inbufx,
		     int *inlen, widechar * outbuf, int *outlen, 
		     formtype
		     *typeform, char *spacing, int mode)
{
  return
    lou_translate (tableList, inbufx, inlen, outbuf, outlen, typeform,
		   spacing, NULL, NULL, NULL, mode);
}

int EXPORT_CALL
lou_translate (const char *tableList, const widechar
	       * inbufx,
	       int *inlen, widechar * outbuf, int *outlen,
	       formtype *typeform, char *spacing, int *outputPos,
	       int *inputPos, int *cursorPos, int modex)
{
  return trace_translate (tableList, inbufx, inlen, outbuf, outlen,
			  typeform, spacing, outputPos, inputPos, cursorPos,
			  NULL, NULL, modex);
}

int
trace_translate (const char *tableList, const widechar * inbufx,
		 int *inlen, widechar * outbuf, int *outlen,
		 formtype *typeform, char *spacing, int *outputPos,
		 int *inputPos, int *cursorPos,
		 const TranslationTableRule ** rules, int *rulesLen,
		 int modex)
{
  int k;
  int goodTrans = 1;
  if (tableList == NULL || inbufx == NULL || inlen == NULL || outbuf ==
      NULL || outlen == NULL)
    return 0;
  logMessage(LOG_DEBUG, "Performing translation: tableList=%s, inlen=%d", tableList, *inlen);
  logWidecharBuf(LOG_DEBUG, "Inbuf=", inbufx, *inlen);
  if ((modex & otherTrans))
    return other_translate (tableList, inbufx,
			    inlen, outbuf, outlen,
			    typeform, spacing, outputPos, inputPos, cursorPos,
			    modex);
  table = lou_getTable (tableList);
  if (table == NULL || *inlen < 0 || *outlen < 0)
    return 0;
  currentInput = (widechar *) inbufx;
  srcmax = 0;
  while (srcmax < *inlen && currentInput[srcmax])
    srcmax++;
  destmax = *outlen;
  haveEmphasis = 0;
  if (!(typebuf = liblouis_allocMem (alloc_typebuf, srcmax, destmax)))
    return 0;
  if (typeform != NULL)
    {
      for (k = 0; k < srcmax; k++)
		{
			typebuf[k] = typeform[k];
			if(typebuf[k] & EMPHASIS)
				haveEmphasis = 1;
		}
    }
  else
    memset (typebuf, 0, srcmax * sizeof (unsigned short));
	
	if(wordBuffer = liblouis_allocMem(alloc_wordBuffer, srcmax, destmax))
		memset(wordBuffer, 0, (srcmax + 4) * sizeof(unsigned int));
	else
		return 0;
	if(emphasisBuffer = liblouis_allocMem(alloc_emphasisBuffer, srcmax, destmax))
		memset(emphasisBuffer, 0, (srcmax + 4) * sizeof(unsigned int));
	else
		return 0;
	if(transNoteBuffer = liblouis_allocMem(alloc_transNoteBuffer, srcmax, destmax))
		memset(transNoteBuffer, 0, (srcmax + 4) * sizeof(unsigned int));
	else
		return 0;
		
  if (!(spacing == NULL || *spacing == 'X'))
    srcSpacing = (unsigned char *) spacing;
  outputPositions = outputPos;
  if (outputPos != NULL)
    for (k = 0; k < srcmax; k++)
      outputPos[k] = -1;
  inputPositions = inputPos;
  mode = modex;
  if (cursorPos != NULL && *cursorPos >= 0)
    {
      cursorStatus = 0;
      cursorPosition = *cursorPos;
      if ((mode & (compbrlAtCursor | compbrlLeftCursor)))
	{
	  compbrlStart = cursorPosition;
	  if (checkAttr (currentInput[compbrlStart], CTC_Space, 0))
	    compbrlEnd = compbrlStart + 1;
	  else
	    {
	      while (compbrlStart >= 0 && !checkAttr
		     (currentInput[compbrlStart], CTC_Space, 0))
		compbrlStart--;
	      compbrlStart++;
	      compbrlEnd = cursorPosition;
	      if (!(mode & compbrlLeftCursor))
		while (compbrlEnd < srcmax && !checkAttr
		       (currentInput[compbrlEnd], CTC_Space, 0))
		  compbrlEnd++;
	    }
	}
    }
  else
    {
      cursorPosition = -1;
      cursorStatus = 1;		/*so it won't check cursor position */
    }
  if (!(passbuf1 = liblouis_allocMem (alloc_passbuf1, srcmax, destmax)))
    return 0;
  if (!(srcMapping = liblouis_allocMem (alloc_srcMapping, srcmax, destmax)))
    return 0;
  if (!
      (prevSrcMapping =
       liblouis_allocMem (alloc_prevSrcMapping, srcmax, destmax)))
    return 0;
  for (k = 0; k <= srcmax; k++)
    srcMapping[k] = k;
  srcMapping[srcmax] = srcmax;
  if ((!(mode & pass1Only)) && (table->numPasses > 1 || table->corrections))
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
  appliedRulesCount = 0;
  if (rules != NULL && rulesLen != NULL)
    {
      appliedRules = rules;
      maxAppliedRules = *rulesLen;
    }
  else
    {
      appliedRules = NULL;
      maxAppliedRules = 0;
    }
  currentPass = 0;
  if ((mode & pass1Only))
    {
      currentOutput = passbuf1;
      memcpy (prevSrcMapping, srcMapping, destmax * sizeof (int));
      goodTrans = translateString ();
      currentPass = 5;		/*Certainly > table->numPasses */
    }
  while (currentPass <= table->numPasses && goodTrans)
    {
      memcpy (prevSrcMapping, srcMapping, destmax * sizeof (int));
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
	  goodTrans = translatePass ();
	  break;
	case 3:
	  srcmax = dest;
	  currentInput = passbuf2;
	  currentOutput = passbuf1;
	  goodTrans = translatePass ();
	  break;
	case 4:
	  srcmax = dest;
	  currentInput = passbuf1;
	  currentOutput = passbuf2;
	  goodTrans = translatePass ();
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
	    {
	      if ((mode & ucBrl))
		outbuf[k] = ((currentOutput[k] & 0xff) | 0x2800);
	      else
		outbuf[k] = currentOutput[k];
	    }
	  else
	    outbuf[k] = getCharFromDots (currentOutput[k]);
	}
      *inlen = realInlen;
      *outlen = dest;
      if (inputPositions != NULL)
	memcpy (inputPositions, srcMapping, dest * sizeof (int));
      if (outputPos != NULL)
	{
	  int lastpos = 0;
	  for (k = 0; k < *inlen; k++)
	    if (outputPos[k] == -1)
	      outputPos[k] = lastpos;
	    else
	      lastpos = outputPos[k];
	}
    }
  if (destSpacing != NULL)
    {
      memcpy (srcSpacing, destSpacing, srcmax);
      srcSpacing[srcmax] = 0;
    }
  if (cursorPos != NULL && *cursorPos != -1)
    {
      if (outputPos != NULL)
	*cursorPos = outputPos[*cursorPos];
      else
	*cursorPos = cursorPosition;
    }
  if (rulesLen != NULL)
    *rulesLen = appliedRulesCount;
  logMessage(LOG_DEBUG, "Translation complete: outlen=%d", *outlen);
  logWidecharBuf(LOG_DEBUG, "Outbuf=", (const widechar *)outbuf, *outlen);
  return goodTrans;
}

int EXPORT_CALL
lou_translatePrehyphenated (const char *tableList,
			    const widechar * inbufx, int *inlen,
			    widechar * outbuf, int *outlen,
			    formtype *typeform, char *spacing,
			    int *outputPos, int *inputPos, int *cursorPos,
			    char *inputHyphens, char *outputHyphens,
			    int modex)
{
  int rv = 1;
  int *alloc_inputPos = NULL;
  if (inputHyphens != NULL)
    {
      if (outputHyphens == NULL)
	return 0;
      if (inputPos == NULL)
	{
	  if ((alloc_inputPos = malloc (*outlen * sizeof (int))) == NULL)
	    outOfMemory ();
	  inputPos = alloc_inputPos;
	}
    }
  if (lou_translate (tableList, inbufx, inlen, outbuf, outlen, typeform,
		     spacing, outputPos, inputPos, cursorPos, modex))
    {
      if (inputHyphens != NULL)
	{
	  int inpos = 0;
	  int outpos;
	  for (outpos = 0; outpos < *outlen; outpos++)
	    {
	      int new_inpos = inputPos[outpos];
	      if (new_inpos < inpos)
		{
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
  if (alloc_inputPos != NULL)
    free (alloc_inputPos);
  return rv;
}

static TranslationTableOpcode indicOpcode;
static const TranslationTableRule *indicRule;
static int dontContract = 0;

static int
hyphenate (const widechar * word, int wordSize, char *hyphens)
{
  widechar *prepWord;
  int i, k, limit;
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
  prepWord = (widechar *) calloc (wordSize + 3, sizeof (widechar));
  /* prepWord is of the format ".hello."
   * hyphens is the length of the word "hello" "00000" */
  prepWord[0] = '.';
  for (i = 0; i < wordSize; i++)
    {
      prepWord[i + 1] = (findCharOrDots (word[i], 0))->lowercase;
      hyphens[i] = '0';
    }
  prepWord[wordSize + 1] = '.';

  /* now, run the finite state machine */
  stateNum = 0;

  // we need to walk all of ".hello."
  for (i = 0; i < wordSize + 2; i++)
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
		{
		  if (transitionsArray[k].ch == ch)
		    {
		      stateNum = transitionsArray[k].newState;
		      goto stateFound;
		    }
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

	  /* Need to ensure that we don't overrun hyphens,
	   * in some cases hyphenPattern is longer than the remaining letters,
	   * and if we write out all of it we would have overshot our buffer. */
	  limit = MIN (strlen (hyphenPattern), wordSize - patternOffset);
	  for (k = 0; k < limit; k++)
	    {
	      if (hyphens[patternOffset + k] < hyphenPattern[k])
		hyphens[patternOffset + k] = hyphenPattern[k];
	    }
	}
    nextLetter:;
    }
  hyphens[wordSize] = 0;
  free (prepWord);
  return 1;
}

static int destword;
static int srcword;
static int doCompTrans (int start, int end);

static int
for_updatePositions (const widechar * outChars, int inLength, int outLength)
{
  int k;
  if ((dest + outLength) > destmax || (src + inLength) > srcmax)
    return 0;
  memcpy (&currentOutput[dest], outChars, outLength * CHARSIZE);
  if (!cursorStatus)
    {
      if ((mode & (compbrlAtCursor | compbrlLeftCursor)))
	{
	  if (src >= compbrlStart)
	    {
	      cursorStatus = 2;
	      return (doCompTrans (compbrlStart, compbrlEnd));
	    }
	}
      else if (cursorPosition >= src && cursorPosition < (src + inLength))
	{
	  cursorPosition = dest;
	  cursorStatus = 1;
	}
      else if (currentInput[cursorPosition] == 0 &&
	       cursorPosition == (src + inLength))
	{
	  cursorPosition = dest + outLength / 2 + 1;
	  cursorStatus = 1;
	}
    }
  else if (cursorStatus == 2 && cursorPosition == src)
    cursorPosition = dest;
  if (inputPositions != NULL || outputPositions != NULL)
    {
      if (outLength <= inLength)
	{
	  for (k = 0; k < outLength; k++)
	    {
	      if (inputPositions != NULL)
		srcMapping[dest + k] = prevSrcMapping[src];
	      if (outputPositions != NULL)
		outputPositions[prevSrcMapping[src + k]] = dest;
	    }
	  for (k = outLength; k < inLength; k++)
	    if (outputPositions != NULL)
	      outputPositions[prevSrcMapping[src + k]] = dest;
	}
      else
	{
	  for (k = 0; k < inLength; k++)
	    {
	      if (inputPositions != NULL)
		srcMapping[dest + k] = prevSrcMapping[src];
	      if (outputPositions != NULL)
		outputPositions[prevSrcMapping[src + k]] = dest;
	    }
	  for (k = inLength; k < outLength; k++)
	    if (inputPositions != NULL)
	      srcMapping[dest + k] = prevSrcMapping[src];
	}
    }
  dest += outLength;
  return 1;
}

static int
syllableBreak ()
{
  int wordStart = 0;
  int wordEnd = 0;
  int wordSize = 0;
  int k = 0;
  char *hyphens = NULL;
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
  if (wordEnd == srcmax)
    wordEnd--;
  /* At this stage wordStart is the 0 based index of the first letter in the word,
   * wordEnd is the 0 based index of the last letter in the word.
   * example: "hello" wordstart=0, wordEnd=4. */
  wordSize = wordEnd - wordStart + 1;
  hyphens = (char *) calloc (wordSize + 1, sizeof (char));
  if (!hyphenate (&currentInput[wordStart], wordSize, hyphens))
    {
      free (hyphens);
      return 0;
    }
  for (k = src - wordStart + 1; k < (src - wordStart + transCharslen); k++)
    if (hyphens[k] & 1)
      {
	free (hyphens);
	return 1;
      }
  free (hyphens);
  return 0;
}

static TranslationTableCharacter *curCharDef;
static widechar before, after;
static TranslationTableCharacterAttributes beforeAttributes;
static TranslationTableCharacterAttributes afterAttributes;
static void
setBefore ()
{
  if (src >= 2 && currentInput[src - 1] == ENDSEGMENT)
    before = currentInput[src - 2];
  else
    before = (src == 0) ? ' ' : currentInput[src - 1];
  beforeAttributes = (findCharOrDots (before, 0))->attributes;
}

static void
setAfter (int length)
{
  if ((src + length + 2) < srcmax && currentInput[src + 1] == ENDSEGMENT)
    after = currentInput[src + 2];
  else
    after = (src + length < srcmax) ? currentInput[src + length] : ' ';
  afterAttributes = (findCharOrDots (after, 0))->attributes;
}

static int prevTypeform = plain_text;
static int prevSrc = 0;
static TranslationTableRule pseudoRule = {
  0
};

static int
brailleIndicatorDefined (TranslationTableOffset offset)
{
  if (!offset)
    return 0;
  indicRule = (TranslationTableRule *) & table->ruleArea[offset];
  indicOpcode = indicRule->opcode;
  return 1;
}

static int prevType = plain_text;
static int curType = plain_text;

typedef enum
{
  firstWord,
  lastWordBefore,
  lastWordAfter,
  firstLetter,
  lastLetter,
  singleLetter,
  word,
  wordStop,
  lenPhrase
} emphCodes;

static int
validMatch ()
{
/*Analyze the typeform parameter and also check for capitalization*/
  TranslationTableCharacter *currentInputChar;
  TranslationTableCharacter *ruleChar;
  TranslationTableCharacterAttributes prevAttr = 0;
  int k;
  int kk = 0;
  if (!transCharslen)
    return 0;
  for (k = src; k < src + transCharslen; k++)
    {
      if (currentInput[k] == ENDSEGMENT)
	{
	  if (k == src && transCharslen == 1)
	    return 1;
	  else
	    return 0;
	}
      currentInputChar = findCharOrDots (currentInput[k], 0);
      if (k == src)
	prevAttr = currentInputChar->attributes;
      ruleChar = findCharOrDots (transRule->charsdots[kk++], 0);
      if ((currentInputChar->lowercase != ruleChar->lowercase))
	return 0;
      if (typebuf != NULL && (typebuf[src] & capsemph) == 0 &&
	  (typebuf[k] | typebuf[src]) != (typebuf[src]))
	return 0;
      if (currentInputChar->attributes != CTC_Letter)
	{
	  if (k != (src + 1) && (prevAttr &
				 CTC_Letter)
	      && (currentInputChar->attributes & CTC_Letter)
	      &&
	      ((currentInputChar->
		attributes & (CTC_LowerCase | CTC_UpperCase |
			      CTC_Letter)) !=
	       (prevAttr & (CTC_LowerCase | CTC_UpperCase | CTC_Letter))))
	    return 0;
	}
      prevAttr = currentInputChar->attributes;
    }
  return 1;
}

static int prevPrevType = 0;
static int nextType = 0;
static TranslationTableCharacterAttributes prevPrevAttr = 0;

static int
doCompEmph ()
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
    {
      if (src == prevSrc && !finish)
	return 1;
      if (src != prevSrc)
	{
	  if (haveEmphasis && src < srcmax - 1)
	    nextType = typebuf[src + 1] & EMPHASIS;
	  else
	    nextType = plain_text;
	  if (src > 2)
	    {
	      if (haveEmphasis)
		prevPrevType = typebuf[src - 2] & EMPHASIS;
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
	  if (haveEmphasis && (typebuf[src] & EMPHASIS) != prevTypeform)
	    {
	      prevType = prevTypeform & EMPHASIS;
	      curType = typebuf[src] & EMPHASIS;
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
	  if (haveEmphasis)
	    switch (curType)
	      {
	      case plain_text:
		ok = 0;
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
	  if (haveEmphasis)
	    switch (prevType)
	      {
	      case plain_text:
		ok = 0;
		break;
		
		default:
		ok = 0;
		prevType = 0;
		break;
	      }
	  if (prevType == plain_text)
	    {
	      checkWhat = checkBeginTypeform;
	      prevTypeform = typebuf[src] & EMPHASIS;
	    }
	  break;
	case checkNumber:
//	  if (brailleIndicatorDefined
//	      (table->numberSign) &&
//	      checkAttr (currentInput[src], CTC_Digit, 0) &&
//	      (prevTransOpcode == CTO_ExactDots
//	       || !(beforeAttributes & CTC_Digit))
//	      && prevTransOpcode != CTO_MidNum)
//	    {
//	      ok = 1;
//	      checkWhat = checkNothing;
//	    }
//	  else
	    checkWhat = checkLetter;
	  break;
	case checkLetter:
	  if (!brailleIndicatorDefined (table->letterSign))
	    {
	      ok = 0;
	      checkWhat = checkBeginMultCaps;
	      break;
	    }
	  if (transOpcode == CTO_Contraction)
	    {
	      ok = 0;//1;
	      checkWhat = checkBeginMultCaps;
	      break;
	    }
	  if ((checkAttr (currentInput[src], CTC_Letter, 0)
	       && !(beforeAttributes & CTC_Letter))
	      && (!checkAttr (currentInput[src + 1], CTC_Letter, 0)
		))//  || (beforeAttributes & CTC_Digit)))
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
	case checkEndMultCaps:
	case checkSingleCap:
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
	  if (cursorStatus == 2)
	    checkWhat = checkNothing;
	}
    }
  while (checkWhat != checkNothing);
  return 1;
}

static int
onlyLettersBehind ()
{
  /* Actually, spaces, then letters */
  int k;
  if (!(beforeAttributes & CTC_Space))
    return 0;
  for (k = src - 2; k >= 0; k--)
    {
      TranslationTableCharacterAttributes attr = (findCharOrDots
						  (currentInput[k],
						   0))->attributes;
      if ((attr & CTC_Space))
	continue;
      if ((attr & CTC_Letter))
	return 1;
      else
	return 0;
    }
  return 1;
}

static int
onlyLettersAhead ()
{
  /* Actullly, spaces, then letters */
  int k;
  if (!(afterAttributes & CTC_Space))
    return 0;
  for (k = src + transCharslen + 1; k < srcmax; k++)
    {
      TranslationTableCharacterAttributes attr = (findCharOrDots
						  (currentInput[k],
						   0))->attributes;
      if ((attr & CTC_Space))
	continue;
      if ((attr & (CTC_Letter | CTC_LitDigit)))
	return 1;
      else
	return 0;
    }
  return 0;
}

static int
noCompbrlAhead ()
{
  int start = src + transCharslen;
  int end;
  int curSrc;
  if (start >= srcmax)
    return 1;
  while (start < srcmax && checkAttr (currentInput[start], CTC_Space, 0))
    start++;
  if (start == srcmax || (transOpcode == CTO_JoinableWord && (!checkAttr
							      (currentInput
							       [start],
							       CTC_Letter |
							       CTC_Digit, 0)
							      ||
							      !checkAttr
							      (currentInput
							       [start - 1],
							       CTC_Space,
							       0))))
    return 1;
  end = start;
  while (end < srcmax && !checkAttr (currentInput[end], CTC_Space, 0))
    end++;
  if ((mode & (compbrlAtCursor | compbrlLeftCursor)) && cursorPosition
      >= start && cursorPosition < end)
    return 0;
  /* Look ahead for rules with CTO_CompBrl */
  for (curSrc = start; curSrc < end; curSrc++)
    {
      int length = srcmax - curSrc;
      int tryThis;
      const TranslationTableCharacter *character1;
      const TranslationTableCharacter *character2;
      int k;
      character1 = findCharOrDots (currentInput[curSrc], 0);
      for (tryThis = 0; tryThis < 2; tryThis++)
	{
	  TranslationTableOffset ruleOffset = 0;
	  TranslationTableRule *testRule;
	  unsigned long int makeHash = 0;
	  switch (tryThis)
	    {
	    case 0:
	      if (!(length >= 2))
		break;
	      /*Hash function optimized for forward translation */
	      makeHash = (unsigned long int) character1->lowercase << 8;
	      character2 = findCharOrDots (currentInput[curSrc + 1], 0);
	      makeHash += (unsigned long int) character2->lowercase;
	      makeHash %= HASHNUM;
	      ruleOffset = table->forRules[makeHash];
	      break;
	    case 1:
	      if (!(length >= 1))
		break;
	      length = 1;
	      ruleOffset = character1->otherRules;
	      break;
	    }
	  while (ruleOffset)
	    {
	      testRule =
		(TranslationTableRule *) & table->ruleArea[ruleOffset];
	      for (k = 0; k < testRule->charslen; k++)
		{
		  character1 = findCharOrDots (testRule->charsdots[k], 0);
		  character2 = findCharOrDots (currentInput[curSrc + k], 0);
		  if (character1->lowercase != character2->lowercase)
		    break;
		}
	      if (tryThis == 1 || k == testRule->charslen)
		{
		  if (testRule->opcode == CTO_CompBrl
		      || testRule->opcode == CTO_Literal)
		    return 0;
		}
	      ruleOffset = testRule->charsnext;
	    }
	}
    }
  return 1;
}

static widechar const *repwordStart;
static int repwordLength;
static int
isRepeatedWord ()
{
  int start;
  if (src == 0 || !checkAttr (currentInput[src - 1], CTC_Letter, 0))
    return 0;
  if ((src + transCharslen) >= srcmax || !checkAttr (currentInput[src +
								  transCharslen],
						     CTC_Letter, 0))
    return 0;
  for (start = src - 2;
       start >= 0 && checkAttr (currentInput[start], CTC_Letter, 0); start--);
  start++;
  repwordStart = &currentInput[start];
  repwordLength = src - start;
  if (compareChars (repwordStart, &currentInput[src
						+ transCharslen],
		    repwordLength, 0))
    return 1;
  return 0;
}

static int
checkEmphasisChange(const int skip)
{
	int r, i;	
	r = 0;
	for(i = src + (skip + 1); i < src + transRule->charslen; i++)
	if(emphasisBuffer[i] || transNoteBuffer[i])
	{
		r = 1;
		break;
	}
	return r;
}

static int
inSequence()
{
	int i, j, s, match;
	//TODO:  all caps words
	//const TranslationTableCharacter *c = NULL;
	
	/*   check before sequence   */
	for(i = src - 1; i >= 0; i--)
	{
		if(checkAttr(currentInput[i], CTC_SeqBefore, 0))
			continue;
		if(!(checkAttr(currentInput[i], CTC_Space | CTC_SeqDelimiter, 0)))
			return 0;
		break;
	}
	
	/*   check after sequence   */
	for(i = src + transRule->charslen; i < srcmax; i++)
	{
		/*   check sequence after patterns   */
		if(table->seqPatternsCount)
		{
			match = 0;
			for(j = i, s = 0; j <= srcmax && s < table->seqPatternsCount; j++, s++)
			{
				/*   matching   */
				if(match == 1)
				{
					if(table->seqPatterns[s])
					{
						if(currentInput[j] == table->seqPatterns[s])
							match = 1;
						else
						{
							match = -1;
							j = i - 1;
						}
					}
					
					/*   found match   */
					else
					{
						/*   pattern at end of input   */
						if(j >= srcmax)
							return 1;
							
						i = j;
						break;
					}
				}
				
				/*   looking for match   */
				else if(match == 0)
				{
					if(table->seqPatterns[s])
					{
						if(currentInput[j] == table->seqPatterns[s])
							match = 1;
						else
						{
							match = -1;
							j = i - 1;
						}
					}
				}
				
				/*   next pattarn   */
				else if(match == -1)
				{
					if(!table->seqPatterns[s])
					{
						match = 0;
						j = i - 1;
					}
				}
			}
		}
		
		if(checkAttr(currentInput[i], CTC_SeqAfter, 0))
			continue;
		if(!(checkAttr(currentInput[i], CTC_Space | CTC_SeqDelimiter, 0)))
			return 0;
		break;
	}
	
	return 1;
}

static void
for_selectRule ()
{
/*check for valid Translations. Return value is in transRule. */
  int length = srcmax - src;
  int tryThis;
  const TranslationTableCharacter *character2;
  int k;
	TranslationTableOffset ruleOffset = 0;
  curCharDef = findCharOrDots (currentInput[src], 0);
  for (tryThis = 0; tryThis < 3; tryThis++)
    {
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
			if(curCharDef->attributes & CTC_UpperCase)
				curCharDef = findCharOrDots(curCharDef->lowercase, 0);
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
	  transRule = (TranslationTableRule *) & table->ruleArea[ruleOffset];
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
		  {		/*check validity of this Translation */
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
		    if ((mode & (compbrlAtCursor | compbrlLeftCursor))
			&& src >= compbrlStart && src <= compbrlEnd)
		      break;
		    return;
		  case CTO_RepWord:
		    if (dontContract || (mode & noContractions))
		      break;
		    if (isRepeatedWord ())
		      return;
		    break;
		  case CTO_NoCont:
		    if (dontContract || (mode & noContractions))
		      break;
		    return;
		  case CTO_Syllable:
		    transOpcode = CTO_Always;
		  case CTO_Always:
					if(checkEmphasisChange(0))
						break;
		    if (dontContract || (mode & noContractions))
		      break;
		    return;
		  case CTO_ExactDots:
		    return;
		  case CTO_NoCross:
		    if (syllableBreak ())
		      break;
		    return;
		  case CTO_Context:
		    if (!srcIncremented || !passDoTest ())
		      break;
		    return;
		  case CTO_LargeSign:
		    if (dontContract || (mode & noContractions))
		      break;
		    if (!((beforeAttributes & (CTC_Space
					       | CTC_Punctuation))
			  || onlyLettersBehind ())
			|| !((afterAttributes & CTC_Space)
			     || prevTransOpcode == CTO_LargeSign)
			|| (afterAttributes & CTC_Letter)
			|| !noCompbrlAhead ())
		      transOpcode = CTO_Always;
		    return;
		  case CTO_WholeWord:
		    if (dontContract || (mode & noContractions))
		      break;
					if(checkEmphasisChange(0))
						break;
		  case CTO_Contraction:
					if(table->usesSequences)
					{
						if(inSequence())
							return;
					}
					else
					{
		    if ((beforeAttributes & (CTC_Space | CTC_Punctuation))
			&& (afterAttributes & (CTC_Space | CTC_Punctuation)))
		      return;
					}
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
		    if (beforeAttributes & (CTC_Space | CTC_Punctuation)
			&& onlyLettersAhead () && noCompbrlAhead ())
		      return;
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
		    if (prevTransOpcode != CTO_ExactDots
			&& beforeAttributes & CTC_Digit
			&& afterAttributes & CTC_Digit)
		      return;
		    break;
		  case CTO_EndNum:
		    if (beforeAttributes & CTC_Digit &&
			prevTransOpcode != CTO_ExactDots)
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
  char *display;
  widechar displayDots[20];
  if (table->undefined)
    {
      TranslationTableRule *transRule = (TranslationTableRule *)
	& table->ruleArea[table->undefined];
      if (!for_updatePositions
	  (&transRule->charsdots[transRule->charslen],
	   transRule->charslen, transRule->dotslen))
	return 0;
      return 1;
    }
  display = showString (&c, 1);
  for (k = 0; k < strlen (display); k++)
    displayDots[k] = getDotsForChar (display[k]);
  if (!for_updatePositions (displayDots, 1, strlen(display)))
    return 0;
  return 1;
}

static int
putCharacter (widechar character)
{
/*Insert the dots equivalent of a character into the output buffer */
	const TranslationTableRule *rule = NULL;
  TranslationTableCharacter *chardef = NULL;
  TranslationTableOffset offset;
  if (cursorStatus == 2)
    return 1;
  chardef = (findCharOrDots (character, 0));
  if ((chardef->attributes & CTC_Letter) && (chardef->attributes &
					     CTC_UpperCase))
    chardef = findCharOrDots (chardef->lowercase, 0);
	//TODO:  for_selectRule and this function screw up Digit and LitDigit
	//NOTE:  removed Litdigit from tables.
	//if(!chardef->otherRules)
		offset = chardef->definitionRule;
	//else
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
  if (offset)
    {
      rule = (TranslationTableRule *)
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

static int
doCompbrl ()
{
/*Handle strings containing substrings defined by the compbrl opcode*/
  int stringStart, stringEnd;
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
  for (stringStart = src; stringStart >= 0; stringStart--)
    if (checkAttr (currentInput[stringStart], CTC_Space, 0))
      break;
  stringStart++;
  for (stringEnd = src; stringEnd < srcmax; stringEnd++)
    if (checkAttr (currentInput[stringEnd], CTC_Space, 0))
      break;
  return (doCompTrans (stringStart, stringEnd));
}

static int
putCompChar (widechar character)
{
/*Insert the dots equivalent of a character into the output buffer */
  TranslationTableOffset offset = (findCharOrDots
				   (character, 0))->definitionRule;
  if (offset)
    {
      const TranslationTableRule *rule = (TranslationTableRule *)
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
  int haveEndsegment = 0;
  if (cursorStatus != 2 && brailleIndicatorDefined (table->begComp))
    if (!for_updatePositions
	(&indicRule->charsdots[0], 0, indicRule->dotslen))
      return 0;
  for (k = start; k < end; k++)
    {
      TranslationTableOffset compdots = 0;
      if (currentInput[k] == ENDSEGMENT)
	{
	  haveEndsegment = 1;
	  continue;
	}
      src = k;
      if (currentInput[k] < 256)
	compdots = table->compdotsPattern[currentInput[k]];
      if (compdots != 0)
	{
	  transRule = (TranslationTableRule *) & table->ruleArea[compdots];
	  if (!for_updatePositions
	      (&transRule->charsdots[transRule->charslen],
	       transRule->charslen, transRule->dotslen))
	    return 0;
	}
      else if (!putCompChar (currentInput[k]))
	return 0;
    }
  if (cursorStatus != 2 && brailleIndicatorDefined (table->endComp))
    if (!for_updatePositions
	(&indicRule->charsdots[0], 0, indicRule->dotslen))
      return 0;
  src = end;
  if (haveEndsegment)
    {
      widechar endSegment = ENDSEGMENT;
      if (!for_updatePositions (&endSegment, 0, 1))
	return 0;
    }
  return 1;
}

static int
doNocont ()
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
markSyllables ()
{
  int k;
  int syllableMarker = 0;
  int currentMark = 0;
  if (typebuf == NULL || !table->syllables)
    return 1;
  src = 0;
  while (src < srcmax)
    {				/*the main multipass translation loop */
      int length = srcmax - src;
      const TranslationTableCharacter *character = findCharOrDots
	(currentInput[src], 0);
      const TranslationTableCharacter *character2;
      int tryThis = 0;
      while (tryThis < 3)
	{
	  TranslationTableOffset ruleOffset = 0;
	  unsigned long int makeHash = 0;
	  switch (tryThis)
	    {
	    case 0:
	      if (!(length >= 2))
		break;
	      makeHash = (unsigned long int) character->lowercase << 8;
		  //memory overflow when src == srcmax - 1
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
		(TranslationTableRule *) & table->ruleArea[ruleOffset];
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
	    typebuf[src++] |= currentMark;
	  break;
	case CTO_Syllable:
	  syllableMarker++;
	  if (syllableMarker > 3)
	    syllableMarker = 1;
	  currentMark = syllableMarker << 13;
	  /*The syllable marker is bits 13 and 14 of typebuf. */
	  if ((src + transCharslen) > srcmax)
	    return 0;
	  for (k = 0; k < transCharslen; k++)
	    typebuf[src++] |= currentMark;
	  break;
	default:
	  break;
	}
    }
  return 1;
}

static void
resolveEmphasisWords(
	int *buffer,
	const TranslationTableOffset *offset,
	const unsigned int bit_begin,
	const unsigned int bit_end,
	const unsigned int bit_word,
	const unsigned int bit_symbol)
{
	int in_word = 0, in_emp = 0, word_start = -1, word_whole = 0, word_stop;
	int i;
	
	for(i = 0; i < srcmax; i++)
	{			
		//TODO:  give each enphasis its own whole word bit?
		/*   clear out previous whole word markings   */
		wordBuffer[i] &= ~WORD_WHOLE;
		
		if(typebuf[i] & passage_break)
		{
			if(in_emp && in_word)
			
			/*   if whole word is one symbol,
				 turn it into a symbol   */
			if(word_start >= 0)
			{
				if(word_start + 1 == i)
					buffer[word_start] |= bit_symbol;
				else
					buffer[word_start] |= bit_word;
				wordBuffer[word_start] |= word_whole;
			}
			
			in_word = 0;
			word_whole = 0;
			word_start = -1;
		}
	
		/*   check if at beginning of emphasis   */
		if(!in_emp)
		if(buffer[i] & bit_begin)
		{
			in_emp = 1;
			buffer[i] &= ~bit_begin;
			
			/*   emphasis started inside word   */
			if(in_word)
			{
				word_start = i;
				word_whole = 0;
			}
		}
		
		/*   check if at end of emphasis   */
		if(in_emp)
		if(buffer[i] & bit_end)
		{
			in_emp = 0;
			buffer[i] &= ~bit_end;
						
			if(in_word && word_start >= 0)
			{				
				/*   check if emphasis ended inside a word   */
				word_stop = bit_end | bit_word;
				if(i < srcmax && wordBuffer[i] & WORD_CHAR)
					word_whole = 0;
				else
					word_stop = 0;
						
				/*   if whole word is one symbol,
					 turn it into a symbol   */
				if(word_start + 1 == i)
					buffer[word_start] |= bit_symbol;
				else
				{
					buffer[word_start] |= bit_word;
					buffer[i] |= word_stop;
				}
				wordBuffer[word_start] |= word_whole;
			}
		}
		
		/*   check if at beginning of word   */
		if(!in_word)
		if(wordBuffer[i] & WORD_CHAR)
		{
			in_word = 1;
			if(in_emp)
			{
				word_whole = WORD_WHOLE;
				word_start = i;
			}
		}
		
		/*   check if at end of word   */
		if(in_word)
		if(i == srcmax || !(wordBuffer[i] & WORD_CHAR))
		{			
			/*   made it through whole word   */
			if(in_emp && word_start >= 0)
			{			
				/*   if word is one symbol,
				     turn it into a symbol   */
				if(word_start + 1 == i)
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
	
	/*   clean up end   */
	if(in_word && in_emp)
	{
		buffer[i] &= ~bit_end;
		
		if(word_start >= 0)
		{		
			/*   if word is one symbol,
				 turn it into a symbol   */
			if(word_start + 1 == i)
					buffer[word_start] |= bit_symbol;
			else
				buffer[word_start] |= bit_word;
			wordBuffer[word_start] |= word_whole;
		}
	}
}

static void
convertToPassage(
	const int pass_start,
	const int pass_end,
	const int word_start,
	int *buffer,
	const TranslationTableOffset *offset,
	const unsigned int bit_begin,
	const unsigned int bit_end,
	const unsigned int bit_word,
	const unsigned int bit_symbol)
{
	int i;
	
	for(i = pass_start; i <= pass_end; i++)
	if(wordBuffer[i] & WORD_WHOLE)
	{
		buffer[i] &= ~(bit_symbol | bit_word);
		wordBuffer[i] &= ~WORD_WHOLE;
	}
	
	buffer[pass_start] |= bit_begin;
	if(   brailleIndicatorDefined(offset[lastLetter])
	   || brailleIndicatorDefined(offset[lastWordAfter]))
		buffer[pass_end] |= bit_end;
	else if(brailleIndicatorDefined(offset[lastWordBefore]))
		buffer[word_start] |= bit_end;
}

static void
resolveEmphasisPassages(
	int *buffer,
	const TranslationTableOffset *offset,
	const unsigned int bit_begin,
	const unsigned int bit_end,
	const unsigned int bit_word,
	const unsigned int bit_symbol)
{
	int word_cnt = 0, pass_start = -1, pass_end = -1, in_word = 0, in_pass = 0;
	int word_start = -1;
	int i;
	
	if(!offset[lenPhrase])
		return;
	
	for(i = 0; i < srcmax; i++)
	{		
		/*   hit passage_break   */
		if(typebuf[i] & passage_break)
		{
			if(in_pass)
			{
				if(!in_word)
				{
					if(word_cnt >= offset[lenPhrase])
					if(pass_end >= 0)
					{
						convertToPassage(
							pass_start, pass_end, word_start, buffer, offset,
							bit_begin, bit_end, bit_word, bit_symbol);
					}
				}
				else
				{
					if(word_cnt >= offset[lenPhrase])
					if(pass_end >= 0)
					{
						convertToPassage(
							pass_start, i, word_start, buffer, offset,
							bit_begin, bit_end, bit_word, bit_symbol);
					}
				}
			}
			in_pass = 0;
			in_word = 0;
		}
		
		/*   check if at beginning of word   */
		if(!in_word)
		if(wordBuffer[i] & WORD_CHAR)
		{
			in_word = 1;
			if(wordBuffer[i] & WORD_WHOLE)
			{
				if(!in_pass)
				{
					in_pass = 1;
					pass_start = i;
					pass_end = -1;
					word_cnt = 1;
				}
				else
					word_cnt++;
				word_start = i;
				continue;
			}
			else if(in_pass)
			{
				if(word_cnt >= offset[lenPhrase])
				if(pass_end >= 0)
				{
					convertToPassage(
						pass_start, pass_end, word_start, buffer, offset,
						bit_begin, bit_end, bit_word, bit_symbol);
				}
				in_pass = 0;
			}
		}
		
		/*   check if at end of word   */
		if(in_word)
		if(i == srcmax || !(wordBuffer[i] & WORD_CHAR))
		{
			in_word = 0;
			if(in_pass)
				pass_end = i;
		}
		
		if(in_pass)
		if(   buffer[i] & bit_begin
		   || buffer[i] & bit_end
		   || buffer[i] & bit_word
		   || buffer[i] & bit_symbol)
		{
			if(word_cnt >= offset[lenPhrase])
			if(pass_end >= 0)
			{
				convertToPassage(
					pass_start, pass_end, word_start, buffer, offset,
					bit_begin, bit_end, bit_word, bit_symbol);
			}
			in_pass = 0;
		}	
	}
	
	if(in_pass)
	{
		if(word_cnt >= offset[lenPhrase])
		if(pass_end >= 0)
		{
			convertToPassage(
				pass_start, i, word_start, buffer, offset,
				bit_begin, bit_end, bit_word, bit_symbol);
		}
	}
}

static void
resolveEmphasisResets(
	int *buffer,
	const TranslationTableOffset *offset,
	const unsigned int bit_begin,
	const unsigned int bit_end,
	const unsigned int bit_word,
	const unsigned int bit_symbol)
{
	int in_word = 0, in_pass = 0, word_start = -1, word_reset = 0, letter_cnt;
	int i;
	
	for(i = 0; i < srcmax; i++)
	{
		if(in_pass)
		if(buffer[i] & bit_end)
			in_pass = 0;
		
		if(!in_pass)
		if(buffer[i] & bit_begin)
		{
			in_pass = 1;
			
			//TODO:  passage break in middle of word
			/*   passage break means no more word_reset   */
			if(typebuf[i] & passage_break)
				word_reset = 0;
		}
		else
		{		
			if(!in_word)
			if(buffer[i] & bit_word)
			{
				/*   deal with case when reset
				     was at beginning of word   */
				if(wordBuffer[i] & WORD_RESET || !checkAttr(currentInput[i], CTC_Letter, 0))
				{
					/*   not just one reset by itself   */
					if(wordBuffer[i + 1] & WORD_CHAR)
					{
						buffer[i + 1] |= bit_word;
						if(wordBuffer[i] & WORD_WHOLE)
							wordBuffer[i + 1] |= WORD_WHOLE;
					}					
					buffer[i] &= ~bit_word;
					wordBuffer[i] &= ~WORD_WHOLE;
					
					/*   if reset is a letter, make it a symbol   */
					if(checkAttr(currentInput[i], CTC_Letter, 0))
						buffer[i] |= bit_symbol;
					
					continue;
				}
				
				in_word = 1;
				word_start = i;
				letter_cnt = 0;
				word_reset = 0;
			}
			
			/*   it is possible for a character to have been
			     marked as a symbol when it should not be one   */
			else if(buffer[i] & bit_symbol)
			if(wordBuffer[i] & WORD_RESET || !checkAttr(currentInput[i], CTC_Letter, 0))
				buffer[i] &= ~bit_symbol;
			
			if(in_word)
			
			/*   at end of word   */
			if(i == srcmax || !(wordBuffer[i] & WORD_CHAR)
			   || (buffer[i] & bit_word && buffer[i] & bit_end))
			{
				in_word = 0;
				
				/*   check if symbol   */
				if(letter_cnt == 1)
				{
					buffer[word_start] |= bit_symbol;
					buffer[word_start] &= ~bit_word;
					wordBuffer[word_start] &= ~WORD_WHOLE;
					buffer[i] &= ~(bit_end | bit_word);
				}
				
				/*   if word ended on a reset or last char was a reset,
				     get rid of end bits   */
				if(word_reset || wordBuffer[i] & WORD_RESET || !checkAttr(currentInput[i], CTC_Letter, 0))
					buffer[i] &= ~(bit_end | bit_word);
				
				/*   if word ended when it began, get rid of all bits   */
				if(i == word_start)
				{
					buffer[word_start] &= ~bit_word;
					wordBuffer[word_start] &= ~WORD_WHOLE;
					buffer[i] &= ~(bit_end | bit_word);					
				}
			}
			else
			{
				/*   hit reset   */
				if(wordBuffer[i] & WORD_RESET || !checkAttr(currentInput[i], CTC_Letter, 0))
				{
					/*   check if symbol if not already reseting   */
					if(letter_cnt == 1)
					{
						buffer[word_start] |= bit_symbol;
						buffer[word_start] &= ~bit_word;
						wordBuffer[word_start] &= ~WORD_WHOLE;
						//buffer[i] &= ~(bit_end | WORD_STOP);
					}
					
					/*   if reset is a letter, make it the new word_start   */
					if(checkAttr(currentInput[i], CTC_Letter, 0))
					{
						word_reset = 0;	
						word_start = i;
						letter_cnt = 1;
						buffer[i] |= bit_word;
					}
					else
						word_reset = 1;		
						
					continue;
				}
				
				if(word_reset)
				{
					word_reset = 0;
					word_start = i;
					letter_cnt = 0;
					buffer[i] |= bit_word;
				}		
				
				letter_cnt++;
			}	
		}
	}
	
	/*   clean up end   */		
	if(in_word)
	{		
		/*   check if symbol   */
		if(letter_cnt == 1)
		{
			buffer[word_start] |= bit_symbol;
			buffer[word_start] &= ~bit_word;
			wordBuffer[word_start] &= ~WORD_WHOLE;
			buffer[i] &= ~(bit_end | bit_word);
		}
		
		if(word_reset)
			buffer[i] &= ~(bit_end | bit_word);
	}
}

static void
markEmphases()
{
	const TranslationTableOffset *offset;
	int caps_start = -1;
	int under_start = -1;
	int bold_start = -1;
	int italic_start = -1;
	int script_start = -1;
	int tnote_start = -1;
	int trans_start[5] = {-1,-1,-1,-1,-1}, trans_bit;
	int i, j;
		
	for(i = 0; i < srcmax; i++)
	{
		if(!checkAttr(currentInput[i], CTC_Space, 0))
			wordBuffer[i] |= WORD_CHAR;
			
		if(   typebuf[i] & word_reset
		)//   || checkAttr(currentInput[i], CTC_WordReset, 0))
			wordBuffer[i] |= WORD_RESET;
		
	
		if(checkAttr(currentInput[i], CTC_UpperCase, 0))
		{
			if(caps_start < 0)
				caps_start = i;
		}
		else if(caps_start >= 0)
		{
			/*   caps should keep going until this   */
			if(   checkAttr(currentInput[i], CTC_Letter, 0)
			   && checkAttr(currentInput[i], CTC_LowerCase, 0))
			{
				emphasisBuffer[caps_start] |= CAPS_BEGIN;
				emphasisBuffer[i] |= CAPS_END;
				caps_start = -1;
			}
		}
		
		if(!haveEmphasis)
			continue;
	
		if(typebuf[i] & underline)
		{
			if(under_start < 0)
				under_start = i;
		}
		else if(under_start >= 0)
		{
			emphasisBuffer[under_start] |= UNDER_BEGIN;
			emphasisBuffer[i] |= UNDER_END;
			under_start = -1;
		}

		if(typebuf[i] & bold)
		{
			if(bold_start < 0)
				bold_start = i;
		}
		else if(bold_start >= 0)
		{
			emphasisBuffer[bold_start] |= BOLD_BEGIN;
			emphasisBuffer[i] |= BOLD_END;
			bold_start = -1;
		}

		if(typebuf[i] & italic)
		{
			if(italic_start < 0)
				italic_start = i;
		}
		else if(italic_start >= 0)
		{
			emphasisBuffer[italic_start] |= ITALIC_BEGIN;
			emphasisBuffer[i] |= ITALIC_END;
			italic_start = -1;
		}

		if(typebuf[i] & script)
		{
			if(script_start < 0)
				script_start = i;
		}
		else if(script_start >= 0)
		{
			emphasisBuffer[script_start] |= SCRIPT_BEGIN;
			emphasisBuffer[i] |= SCRIPT_END;
			script_start = -1;
		}

		if(typebuf[i] & trans_note)
		{
			if(tnote_start < 0)
				tnote_start = i;
		}
		else if(tnote_start >= 0)
		{
			emphasisBuffer[tnote_start] |= TNOTE_BEGIN;
			emphasisBuffer[i] |= TNOTE_END;
			tnote_start = -1;
		}
		
		trans_bit = trans_note_1;
		for(j = 0; j < 5; j++)
		{		
			if(typebuf[i] & trans_bit)
			{
				if(trans_start[j] < 0)
					trans_start[j] = i;
			}
			else if(trans_start[j] >= 0)
			{
				transNoteBuffer[trans_start[j]] |= (TRANSNOTE_BEGIN << (j * 4));
				transNoteBuffer[i] |= (TRANSNOTE_END << (j * 4));
				trans_start[j] = -1;
			}
			trans_bit <<= 1;
		}
	}
	
	/*   clean up srcmax   */
	if(caps_start >= 0)
	{
		emphasisBuffer[caps_start] |= CAPS_BEGIN;
		emphasisBuffer[srcmax] |= CAPS_END;
	}

	if(haveEmphasis)
	{	
		if(under_start >= 0)
		{
			emphasisBuffer[under_start] |= UNDER_BEGIN;
			emphasisBuffer[srcmax] |= UNDER_END;
		}
		if(bold_start >= 0)
		{
			emphasisBuffer[bold_start] |= BOLD_BEGIN;
			emphasisBuffer[srcmax] |= BOLD_END;
		}
		if(italic_start >= 0)
		{
			emphasisBuffer[italic_start] |= ITALIC_BEGIN;
			emphasisBuffer[srcmax] |= ITALIC_END;
		}
		if(script_start >= 0)
		{
			emphasisBuffer[script_start] |= SCRIPT_BEGIN;
			emphasisBuffer[srcmax] |= SCRIPT_END;
		}
		if(tnote_start >= 0)
		{
			emphasisBuffer[tnote_start] |= TNOTE_BEGIN;
			emphasisBuffer[srcmax] |= SCRIPT_END;
		}
	}
	
	resolveEmphasisWords(emphasisBuffer, &table->firstWordCaps,
	                     CAPS_BEGIN, CAPS_END, CAPS_WORD, CAPS_SYMBOL);
	resolveEmphasisPassages(emphasisBuffer, &table->firstWordCaps,
	                        CAPS_BEGIN, CAPS_END, CAPS_WORD, CAPS_SYMBOL);
	resolveEmphasisResets(emphasisBuffer, &table->firstWordCaps,
	                      CAPS_BEGIN, CAPS_END, CAPS_WORD, CAPS_SYMBOL);
	if(!haveEmphasis)
		return;
		
	resolveEmphasisWords(emphasisBuffer, &table->firstWordUnder,
	                     UNDER_BEGIN, UNDER_END, UNDER_WORD, UNDER_SYMBOL);
	resolveEmphasisPassages(emphasisBuffer, &table->firstWordUnder,
	                        UNDER_BEGIN, UNDER_END, UNDER_WORD, UNDER_SYMBOL);
					
	resolveEmphasisWords(emphasisBuffer, &table->firstWordBold,
	                     BOLD_BEGIN, BOLD_END, BOLD_WORD, BOLD_SYMBOL);
	resolveEmphasisPassages(emphasisBuffer, &table->firstWordBold,
	                        BOLD_BEGIN, BOLD_END, BOLD_WORD, BOLD_SYMBOL);
					
	resolveEmphasisWords(emphasisBuffer, &table->firstWordItal,
	                     ITALIC_BEGIN, ITALIC_END, ITALIC_WORD, ITALIC_SYMBOL);
	resolveEmphasisPassages(emphasisBuffer, &table->firstWordItal,
	                        ITALIC_BEGIN, ITALIC_END, ITALIC_WORD, ITALIC_SYMBOL);
					
	resolveEmphasisWords(emphasisBuffer, &table->firstWordScript,
	                     SCRIPT_BEGIN, SCRIPT_END, SCRIPT_WORD, SCRIPT_SYMBOL);
	resolveEmphasisPassages(emphasisBuffer, &table->firstWordScript,
	                        SCRIPT_BEGIN, SCRIPT_END, SCRIPT_WORD, SCRIPT_SYMBOL);
					
	resolveEmphasisWords(emphasisBuffer, &table->firstWordTransNote,
	                     TNOTE_BEGIN, TNOTE_END, TNOTE_WORD, TNOTE_SYMBOL);
	resolveEmphasisPassages(emphasisBuffer, &table->firstWordTransNote,
	                        TNOTE_BEGIN, TNOTE_END, TNOTE_WORD, TNOTE_SYMBOL);

	for(i = 0; i < 5; i++)
	{
		switch(i)
		{
		case 0:  offset = &table->firstWordTrans1; break;
		case 1:  offset = &table->firstWordTrans2; break;
		case 2:  offset = &table->firstWordTrans3; break;
		case 3:  offset = &table->firstWordTrans4; break;
		case 4:  offset = &table->firstWordTrans5; break;
		}
		resolveEmphasisWords(
			transNoteBuffer,
			offset,
			TRANSNOTE_BEGIN << (i * 4),
			TRANSNOTE_END << (i * 4),
			TRANSNOTE_WORD << (i * 4),
			TRANSNOTE_SYMBOL << (i * 4));
		resolveEmphasisPassages(
			transNoteBuffer,
			offset,
			TRANSNOTE_BEGIN << (i * 4),
			TRANSNOTE_END << (i * 4),
			TRANSNOTE_WORD << (i * 4),
			TRANSNOTE_SYMBOL << (i * 4));
	}
}

static void
insertEmphasis(
	const int *buffer,
	const int at,
	const TranslationTableOffset *offset,
	const unsigned int bit_begin,
	const unsigned int bit_end,
	const unsigned int bit_word,
	const unsigned int bit_symbol)
{	
	if(buffer[at] & bit_symbol)
	{
		if(brailleIndicatorDefined(offset[singleLetter]))
			for_updatePositions(
				&indicRule->charsdots[0], 0, indicRule->dotslen);
	}
	
	if(buffer[at] & bit_word
	//   && !(buffer[at] & bit_begin)
	   && !(buffer[at] & bit_end))
	{
		if(brailleIndicatorDefined(offset[word]))
			for_updatePositions(
				&indicRule->charsdots[0], 0, indicRule->dotslen);
	}
	
	if(buffer[at] & bit_begin)
	{
		if(brailleIndicatorDefined(offset[firstWord]))
			for_updatePositions(
				&indicRule->charsdots[0], 0, indicRule->dotslen);
		else if(brailleIndicatorDefined(offset[firstLetter]))
			for_updatePositions(
				&indicRule->charsdots[0], 0, indicRule->dotslen);
	}
}

static void
insertEmphasisEnd(
	const int *buffer,
	const int at,
	const TranslationTableOffset *offset,
	const unsigned int bit_end,
	const unsigned int bit_word)
{
	if(buffer[at] & bit_end)
	{
		if(buffer[at] & bit_word)
		{
			if(brailleIndicatorDefined(offset[wordStop]))
				for_updatePositions(
					&indicRule->charsdots[0], 0, indicRule->dotslen);
		}
		else
		{
			if(brailleIndicatorDefined(offset[lastLetter]))
				for_updatePositions(
					&indicRule->charsdots[0], 0, indicRule->dotslen);
			else if(brailleIndicatorDefined(offset[lastWordAfter]))
				for_updatePositions(
					&indicRule->charsdots[0], 0, indicRule->dotslen);
			else if(brailleIndicatorDefined(offset[lastWordBefore]))
				for_updatePositions(
					&indicRule->charsdots[0], 0, indicRule->dotslen);
		}
	}
}

static int
endCount(
	const int *buffer,
	const int at,
	const unsigned int bit_end,
	const unsigned int bit_begin,
	const unsigned int bit_word)
{
	if(!(buffer[at] & bit_end))
		return 0;
	int i, cnt = 1;	
	for(i = at - 1; i >= 0; i--)
	if(buffer[i] & bit_begin || buffer[i] & bit_word)
		break;
	else
		cnt++;
	return cnt;
}

static void
insertEmphasesAt(const int at)
{
	const TranslationTableOffset *offset;
	int mask;
	int type_counts[10];
	int i, j, max;
	
	/*   simple case   */
	if(!haveEmphasis)
	{
		/*   insert graded 1 mode indicator   */
		if(transOpcode == CTO_Contraction)
		if(brailleIndicatorDefined(table->noContractSign))
			for_updatePositions(
				&indicRule->charsdots[0], 0, indicRule->dotslen);

		if(emphasisBuffer[at] & CAPS_EMPHASIS)
		{
			insertEmphasis(
				emphasisBuffer, at, &table->firstWordCaps,
				CAPS_BEGIN, CAPS_END, CAPS_WORD, CAPS_SYMBOL);
			insertEmphasisEnd(
				emphasisBuffer, at, &table->firstWordCaps, CAPS_END, CAPS_WORD);
		}
		return;
	}
	
	/*   The order of inserting the end symbols must be the reverse
	     of the insertions of the begin symbols so that they will
	     nest properly when multiple emphases start and end at
	     the same place   */
#define CAPS_COUNT     0
#define UNDER_COUNT    1
#define BOLD_COUNT     2
#define ITALIC_COUNT   3
#define SCRIPT_COUNT   4
#define TRANS1_COUNT   5
#define TRANS2_COUNT   6
#define TRANS3_COUNT   7
#define TRANS4_COUNT   8
#define TRANS5_COUNT   9
	
	type_counts[CAPS_COUNT] 
		= endCount(emphasisBuffer, at, CAPS_END, CAPS_BEGIN, CAPS_WORD);
	type_counts[BOLD_COUNT]
		= endCount(emphasisBuffer, at, BOLD_END, BOLD_BEGIN, BOLD_WORD);
	type_counts[UNDER_COUNT]
		= endCount(emphasisBuffer, at, UNDER_END, UNDER_BEGIN, UNDER_WORD);
	type_counts[ITALIC_COUNT]
		= endCount(emphasisBuffer, at, ITALIC_END, ITALIC_BEGIN, ITALIC_WORD);
	type_counts[SCRIPT_COUNT]
		= endCount(emphasisBuffer, at, SCRIPT_END, SCRIPT_BEGIN, SCRIPT_WORD);
	type_counts[TRANS1_COUNT]
		= endCount(transNoteBuffer, at, TRANSNOTE_END, TRANSNOTE_BEGIN, TRANSNOTE_WORD);
	type_counts[TRANS2_COUNT]
		= endCount(transNoteBuffer, at, TRANSNOTE_END << 4, TRANSNOTE_BEGIN << 4, TRANSNOTE_WORD << 4);
	type_counts[TRANS3_COUNT]
		= endCount(transNoteBuffer, at, TRANSNOTE_END << 8, TRANSNOTE_BEGIN << 8, TRANSNOTE_WORD << 8);
	type_counts[TRANS4_COUNT]
		= endCount(transNoteBuffer, at, TRANSNOTE_END << 12, TRANSNOTE_BEGIN << 12, TRANSNOTE_WORD << 12);
	type_counts[TRANS5_COUNT]
		= endCount(transNoteBuffer, at, TRANSNOTE_END << 16, TRANSNOTE_BEGIN << 16, TRANSNOTE_WORD << 16);
	
	for(i = 0; i < 8; i++)
	{
		max = 0;
		for(j = 1; j < 9; j++)
		if(type_counts[max] < type_counts[j])
			max = j;
		if(!type_counts[max])
			break;
		type_counts[max] = 0;
		switch(max)
		{	
		case CAPS_COUNT:		
			insertEmphasisEnd(
				emphasisBuffer, at, &table->firstWordCaps, CAPS_END, CAPS_WORD);
			break;			
		case ITALIC_COUNT:
			insertEmphasisEnd(
				emphasisBuffer, at, &table->firstWordItal, ITALIC_END, ITALIC_WORD);
			break;
		case UNDER_COUNT:
			insertEmphasisEnd(
				emphasisBuffer, at, &table->firstWordUnder, UNDER_END, UNDER_WORD);
			break;
		case BOLD_COUNT:
			insertEmphasisEnd(
				emphasisBuffer, at, &table->firstWordBold, BOLD_END, BOLD_WORD);
			break;
		case SCRIPT_COUNT:
			insertEmphasisEnd(
				emphasisBuffer, at, &table->firstWordScript, SCRIPT_END, SCRIPT_WORD);
			break;
		case TRANS1_COUNT:
			insertEmphasisEnd(
				transNoteBuffer, at, &table->firstWordTrans1, TRANSNOTE_END, TRANSNOTE_END);
			break;
		case TRANS2_COUNT:
			insertEmphasisEnd(
				transNoteBuffer, at, &table->firstWordTrans2, TRANSNOTE_END << 4, TRANSNOTE_END << 4);
			break;
		case TRANS3_COUNT:
			insertEmphasisEnd(
				transNoteBuffer, at, &table->firstWordTrans3, TRANSNOTE_END << 8, TRANSNOTE_END << 8);
			break;
		case TRANS4_COUNT:
			insertEmphasisEnd(
				transNoteBuffer, at, &table->firstWordTrans4, TRANSNOTE_END << 12, TRANSNOTE_END << 12);
			break;
		case TRANS5_COUNT:
			insertEmphasisEnd(
				transNoteBuffer, at, &table->firstWordTrans5, TRANSNOTE_END << 16, TRANSNOTE_END << 16);
			break;
		}
	}
	
	for(i = 4; i >= 0; i--)
	{
		mask = TRANSNOTE_MASK << (i * 4);
		if(transNoteBuffer[at] & mask)
		{
			switch(i)
			{
			case 0:  offset = &table->firstWordTrans1; break;
			case 1:  offset = &table->firstWordTrans2; break;
			case 2:  offset = &table->firstWordTrans3; break;
			case 3:  offset = &table->firstWordTrans4; break;
			case 4:  offset = &table->firstWordTrans5; break;
			}
			insertEmphasis(
				transNoteBuffer, at, offset,
				TRANSNOTE_BEGIN << (i * 4),
				TRANSNOTE_END << (i * 4),
				TRANSNOTE_WORD << (i * 4),
				TRANSNOTE_SYMBOL << (i * 4));
		}
	}
	if(emphasisBuffer[at] & SCRIPT_EMPHASIS)
		insertEmphasis(
			emphasisBuffer, at, &table->firstWordScript,
			SCRIPT_BEGIN, SCRIPT_END, SCRIPT_WORD, SCRIPT_SYMBOL);
	if(emphasisBuffer[at] & ITALIC_EMPHASIS)
		insertEmphasis(
			emphasisBuffer, at, &table->firstWordItal,
			ITALIC_BEGIN, ITALIC_END, ITALIC_WORD, ITALIC_SYMBOL);
	if(emphasisBuffer[at] & BOLD_EMPHASIS)
		insertEmphasis(
			emphasisBuffer, at, &table->firstWordBold,
			BOLD_BEGIN, BOLD_END, BOLD_WORD, BOLD_SYMBOL);			
	if(emphasisBuffer[at] & UNDER_EMPHASIS)
		insertEmphasis(
			emphasisBuffer, at, &table->firstWordUnder,
			UNDER_BEGIN, UNDER_END, UNDER_WORD, UNDER_SYMBOL);
	
	/*   insert graded 1 mode indicator   */
	if(transOpcode == CTO_Contraction)
	if(brailleIndicatorDefined(table->noContractSign))
		for_updatePositions(
			&indicRule->charsdots[0], 0, indicRule->dotslen);

	/*   insert capitaliztion last so it will be closest to word   */
	if(emphasisBuffer[at] & CAPS_EMPHASIS)
		insertEmphasis(
			emphasisBuffer, at, &table->firstWordCaps,
			CAPS_BEGIN, CAPS_END, CAPS_WORD, CAPS_SYMBOL);

}

static int pre_src;

static void
insertEmphases()
{
	int at;
	
	for(at = pre_src; at <= src; at++)
		insertEmphasesAt(at);
	
	pre_src = src + 1;
}

static void
checkNumericMode()
{
	int i;
	
	if(!brailleIndicatorDefined(table->numberSign))
		return;
	
	/*   not in numeric mode   */
	if(!numericMode)
	{
		if(checkAttr(currentInput[src], CTC_Digit | CTC_LitDigit, 0))
		{
			numericMode = 1;
			dontContract = 1;
			for_updatePositions(
				&indicRule->charsdots[0], 0, indicRule->dotslen);
		}
		else if(checkAttr(currentInput[src], CTC_NumericMode, 0))
		{
			for(i = src + 1; i < srcmax; i++)
			{
				if(checkAttr(currentInput[i], CTC_Digit | CTC_LitDigit, 0))
				{
					numericMode = 1;
					for_updatePositions(
						&indicRule->charsdots[0], 0, indicRule->dotslen);
					break;
				}
				else if(!checkAttr(currentInput[i], CTC_NumericMode, 0))
					break;
			}
		}
	}
	
	/*   in numeric mode   */
	else
	{
		if(!checkAttr(currentInput[src], CTC_Digit | CTC_LitDigit | CTC_NumericMode, 0))
		{
			numericMode = 0;
			if(brailleIndicatorDefined(table->noContractSign))
			if(checkAttr(currentInput[src], CTC_NumericNoContract, 0))
					for_updatePositions(
						&indicRule->charsdots[0], 0, indicRule->dotslen);
		}
	}
}

static int
translateString ()
{
/*Main translation routine */
  int k;
  markSyllables ();
	numericMode = 0;
  srcword = 0;
  destword = 0;        		/* last word translated */
  dontContract = 0;
  prevTransOpcode = CTO_None;
  prevType = prevPrevType = curType = nextType = prevTypeform = plain_text;
  prevSrc = -1;
  src = dest = 0;
  srcIncremented = 1;
	pre_src = 0;
  memset (passVariables, 0, sizeof(int) * NUMVAR);
  if (typebuf && table->capitalSign)
    for (k = 0; k < srcmax; k++)
      if (checkAttr (currentInput[k], CTC_UpperCase, 0))
        typebuf[k] |= capsemph;
		
	markEmphases();

  while (src < srcmax)
    {        			/*the main translation loop */
      setBefore ();
      if (!insertBrailleIndicators (0))
        goto failure;
      if (src >= srcmax)
        break;

		//insertEmphases();

      for_selectRule ();
      if (appliedRules != NULL && appliedRulesCount < maxAppliedRules)
        appliedRules[appliedRulesCount++] = transRule;
      srcIncremented = 1;
      prevSrc = src;
      switch (transOpcode)        /*Rules that pre-empt context and swap */
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

//		if(transOpcode == CTO_Contraction)
//		if(brailleIndicatorDefined(table->noContractSign))
//		if(!for_updatePositions(
//			&indicRule->charsdots[0], 0, indicRule->dotslen))
//			goto failure;
		insertEmphases();
		checkNumericMode();

      if (transOpcode == CTO_Context || findAttribOrSwapRules ())
        switch (transOpcode)
          {
          case CTO_Context:
            if (appliedRules != NULL && appliedRulesCount < maxAppliedRules)
              appliedRules[appliedRulesCount++] = transRule;
            if (!passDoAction ())
              goto failure;
            if (endReplace == src)
              srcIncremented = 0;
            src = endReplace;
            continue;
          default:
            break;
          }

/*Processing before replacement*/

		/*   check if leaving no contraction (grade 1) mode   */
		if(checkAttr(currentInput[src], CTC_SeqDelimiter | CTC_Space, 0))
			dontContract = 0;
			
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
          {
            int hasEndSegment = 0;
            while (dest > 0 && checkAttr (currentOutput[dest - 1], CTC_Space, 1))
            {
              if (currentOutput[dest - 1] == ENDSEGMENT)
              {
                hasEndSegment = 1;
              }
              dest--;
            }
            if (hasEndSegment != 0)
            {
              currentOutput[dest] = 0xffff;
              dest++;
            }
          }
          break;
        case CTO_DecPoint:
//          if (table->numberSign)
//            {
//              TranslationTableRule *numRule = (TranslationTableRule *)
//        	& table->ruleArea[table->numberSign];
//              if (!for_updatePositions
//        	  (&numRule->charsdots[numRule->charslen],
//        	   numRule->charslen, numRule->dotslen))
//        	goto failure;
//            }
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
          /* Only needs special handling if not within compbrl and
           *the table defines a capital sign. */
          if (!
              (mode & (compbrlAtCursor | compbrlLeftCursor) && src >=
               compbrlStart
               && src <= compbrlEnd) && (transRule->dotslen == 1
        				 && table->capitalSign))
            {
              putCharacter (curCharDef->lowercase);
              src++;
              break;
            }
//		case CTO_Contraction:
//		
//			if(brailleIndicatorDefined(table->noContractSign))
//			if(!for_updatePositions(
//				&indicRule->charsdots[0], 0, indicRule->dotslen))
//				goto failure;
			
        default:
          if (cursorStatus == 2)
            cursorStatus = 1;
          else
            {
              if (transRule->dotslen)
        	{
        	  if (!for_updatePositions
        	      (&transRule->charsdots[transCharslen],
        	       transCharslen, transRule->dotslen))
        	    goto failure;
        	}
              else
        	{
        	  for (k = 0; k < transCharslen; k++)
        	    {
        	      if (!putCharacter (currentInput[src]))
        		goto failure;
        	      src++;
        	    }
        	}
              if (cursorStatus == 2)
        	cursorStatus = 1;
              else if (transRule->dotslen)
        	src += transCharslen;
            }
          break;
        }

      /* processing after replacement */
      switch (transOpcode)
        {
        case CTO_Repeated:
          {
            /* Skip repeated characters. */
            int srclim = srcmax - transCharslen;
            if (mode & (compbrlAtCursor | compbrlLeftCursor) &&
        	compbrlStart < srclim)
              /* Don't skip characters from compbrlStart onwards. */
              srclim = compbrlStart - 1;
            while ((src <= srclim)
        	   && compareChars (&transRule->charsdots[0],
        			    &currentInput[src], transCharslen, 0))
              {
        	/* Map skipped input positions to the previous output position. */
        	if (outputPositions != NULL)
        	  {
        	    int tcc;
        	    for (tcc = 0; tcc < transCharslen; tcc++)
        	      outputPositions[prevSrcMapping[src + tcc]] = dest - 1;
        	  }
        	if (!cursorStatus && src <= cursorPosition
        	    && cursorPosition < src + transCharslen)
        	  {
        	    cursorStatus = 1;
        	    cursorPosition = dest - 1;
        	  }
        	src += transCharslen;
              }
            break;
          }
        case CTO_RepWord:
          {
            /* Skip repeated characters. */
            int srclim = srcmax - transCharslen;
            if (mode & (compbrlAtCursor | compbrlLeftCursor) &&
        	compbrlStart < srclim)
              /* Don't skip characters from compbrlStart onwards. */
              srclim = compbrlStart - 1;
            while ((src <= srclim)
        	   && compareChars (repwordStart,
        			    &currentInput[src], repwordLength, 0))
              {
        	/* Map skipped input positions to the previous output position. */
        	if (outputPositions != NULL)
        	  {
        	    int tcc;
        	    for (tcc = 0; tcc < transCharslen; tcc++)
        	      outputPositions[prevSrcMapping[src + tcc]] = dest - 1;
        	  }
        	if (!cursorStatus && src <= cursorPosition
        	    && cursorPosition < src + transCharslen)
        	  {
        	    cursorStatus = 1;
        	    cursorPosition = dest - 1;
        	  }
        	src += repwordLength + transCharslen;
              }
            src -= transCharslen;
            break;
          }
        case CTO_JoinNum:
        case CTO_JoinableWord:
          while (src < srcmax
        	 && checkAttr (currentInput[src], CTC_Space, 0) &&
        	 currentInput[src] != ENDSEGMENT)
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
    }        	
	
	transOpcode = CTO_Space;
	insertEmphases();
		
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
  realInlen = src;
  return 1;
}        			/*first pass translation completed */

int EXPORT_CALL
lou_hyphenate (const char *tableList, const widechar
	       * inbuf, int inlen, char *hyphens, int mode)
{
#define HYPHSTRING 100
  widechar workingBuffer[HYPHSTRING];
  int k, kk;
  int wordStart;
  int wordEnd;
  table = lou_getTable (tableList);
  if (table == NULL || inbuf == NULL || hyphens
      == NULL || table->hyphenStatesArray == 0 || inlen >= HYPHSTRING)
    return 0;
  if (mode != 0)
    {
      k = inlen;
      kk = HYPHSTRING;
      if (!lou_backTranslate (tableList, inbuf, &k,
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
  for (k = wordStart; k <= wordEnd; k++)
    {
      TranslationTableCharacter *c = findCharOrDots (workingBuffer[k], 0);
      if (!(c->attributes & CTC_Letter))
	return 0;
    }
  if (!hyphenate
      (&workingBuffer[wordStart], wordEnd - wordStart + 1,
       &hyphens[wordStart]))
    return 0;
  for (k = 0; k <= wordStart; k++)
    hyphens[k] = '0';
  if (mode != 0)
    {
      widechar workingBuffer2[HYPHSTRING];
      int outputPos[HYPHSTRING];
      char hyphens2[HYPHSTRING];
      kk = wordEnd - wordStart + 1;
      k = HYPHSTRING;
      if (!lou_translate (tableList, &workingBuffer[wordStart], &kk,
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
	if (!table->noBreak || hyphens2[kk] == '0')
	  hyphens[kk] = hyphens2[kk];
	else
	  {
	    TranslationTableRule *noBreakRule = (TranslationTableRule *)
	      & table->ruleArea[table->noBreak];
	    int kkk;
	    if (kk > 0)
	      for (kkk = 0; kkk < noBreakRule->charslen; kkk++)
		if (workingBuffer2[kk - 1] == noBreakRule->charsdots[kkk])
		  {
		    hyphens[kk] = '0';
		    break;
		  }
	    for (kkk = 0; kkk < noBreakRule->dotslen; kkk++);
	    if (workingBuffer2[kk] ==
		noBreakRule->charsdots[noBreakRule->charslen + kkk])
	      {
		hyphens[kk] = '0';
		break;
	      }
	  }
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
lou_dotsToChar (const char *tableList, widechar * inbuf, widechar * outbuf,
		int length, int mode)
{
  int k;
  widechar dots;
  if (tableList == NULL || inbuf == NULL || outbuf == NULL)
    return 0;
  if ((mode & otherTrans))
    return other_dotsToChar (tableList, inbuf, outbuf, length, mode);
  table = lou_getTable (tableList);
  if (table == NULL || length <= 0)
    return 0;
  for (k = 0; k < length; k++)
    {
      dots = inbuf[k];
      if (!(dots & B16) && (dots & 0xff00) == 0x2800)	/*Unicode braille */
	dots = (dots & 0x00ff) | B16;
      outbuf[k] = getCharFromDots (dots);
    }
  return 1;
}

int EXPORT_CALL
lou_charToDots (const char *tableList, const widechar * inbuf, widechar *
		outbuf, int length, int mode)
{
  int k;
  if (tableList == NULL || inbuf == NULL || outbuf == NULL)
    return 0;
  if ((mode & otherTrans))
    return other_charToDots (tableList, inbuf, outbuf, length, mode);

  table = lou_getTable (tableList);
  if (table == NULL || length <= 0)
    return 0;
  for (k = 0; k < length; k++)
    if ((mode & ucBrl))
      outbuf[k] = ((getDotsForChar (inbuf[k]) & 0xff) | 0x2800);
    else
      outbuf[k] = getDotsForChar (inbuf[k]);
  return 1;
}
