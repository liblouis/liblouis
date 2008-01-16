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

#ifndef __LOUIS_H_
#define __LOUIS_H_

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

#include "liblouis.h"

#define NUMSWAPS 50
#define NUMVAR 50
#define LETSIGNSIZE 16
#define CHARSIZE sizeof(widechar)
#define DEFAULTRULESIZE 50
/*Definitions of braille dots*/
#define B1 0X01
#define B2 0X02
#define B3 0X04
#define B4 0X08
#define B5 0X10
#define B6 0X20
#define B7 0X40
#define B8 0X80
#define B9 0X100
#define B10 0X200
#define B11 0X400
#define B12 0X800
#define B13 0X1000
#define B14 0X2000
#define B15 0X4000
#define B16 0X8000

/*HASHNUM must be prime */
#define HASHNUM 1123

#define MAXSTRING 256

  typedef unsigned int ContractionTableOffset;
#define OFFSETSIZE sizeof (ContractionTableOffset)

  typedef enum
  {
    CTC_Space = 0X01,
    CTC_Letter = 0X02,
    CTC_Digit = 0X04,
    CTC_Punctuation = 0X08,
    CTC_UpperCase = 0X10,
    CTC_LowerCase = 0X20,
    CTC_Math = 0X40,
    CTC_Sign = 0X80,
    CTC_LitDigit = 0X100,
    CTC_Undefined = 0x200
  } ContractionTableCharacterAttribute;

  typedef enum
  {
    pass_lookback = '_',
    pass_string = '\"',
    pass_dots = '@',
    pass_omit = '?',
    pass_startReplace = '[',
    pass_endReplace = ']',
    pass_variable = '#',
    pass_not = '!',
    pass_digit = 'd',
    pass_litDigit = 'D',
    pass_letter = 'l',
    pass_math = 'm',
    pass_punctuation = 'p',
    pass_sign = 'S',
    pass_space = 's',
    pass_uppercase = 'U',
    pass_lowercase = 'u',
    pass_attributes = '$',
    pass_swap = '%',
    pass_hyphen = '-',
    pass_until = '.',
    pass_eq = '=',
    pass_lt = '<',
    pass_gt = '>',
    pass_lteq = 130,
    pass_gteq = 131,
    pass_endTest = 32,
    pass_plus = '+',
    pass_copy = '*',
  }
  pass_Codes;

  typedef unsigned int ContractionTableCharacterAttributes;

  typedef struct
  {
    ContractionTableOffset next;
    ContractionTableOffset definitionRule;
    ContractionTableOffset otherRules;
    ContractionTableCharacterAttributes attributes;
    widechar realchar;
    widechar uppercase;
    widechar lowercase;
#if UNICODEBITS == 16
    widechar padding;
#endif
  } ContractionTableCharacter;

  typedef enum
  {				/*Op codes */
    CTO_IncludeFile,
    CTO_Locale,			/*Deprecated, do not use */
    CTO_CapitalSign,
    CTO_BeginCapitalSign,
    CTO_LenBegcaps,
    CTO_EndCapitalSign,
    CTO_FirstWordCaps,
    CTO_LastWordCapsAfter,
    CTO_LenCapsPhrase,
    CTO_LetterSign,
    CTO_NoLetsignBefore,
    CTO_NoLetsign,
    CTO_NoLetsignAfter,
    CTO_NumberSign,
    CTO_FirstWordItal,
    CTO_ItalSign,
    CTO_LastWordItalBefore,
    CTO_LastWordItalAfter,
    CTO_BegItal,
    CTO_FirstLetterItal,
    CTO_EndItal,
    CTO_LastLetterItal,
    CTO_SingleLetterItal,
    CTO_ItalWord,
    CTO_LenItalPhrase,
    CTO_FirstWordBold,
    CTO_BoldSign,
    CTO_LastWordBoldBefore,
    CTO_LastWordBoldAfter,
    CTO_BegBold,
    CTO_FirstLetterBold,
    CTO_EndBold,
    CTO_LastLetterBold,
    CTO_SingleLetterBold,
    CTO_BoldWord,
    CTO_LenBoldPhrase,
    CTO_FirstWordUnder,
    CTO_UnderSign,
    CTO_LastWordUnderBefore,
    CTO_LastWordUnderAfter,
    CTO_BegUnder,
    CTO_FirstLetterUnder,
    CTO_EndUnder,
    CTO_LastLetterUnder,
    CTO_SingleLetterUnder,
    CTO_UnderWord,
    CTO_LenUnderPhrase,
    CTO_BegComp,
    CTO_CompBegEmph1,
    CTO_CompEndEmph1,
    CTO_CompBegEmph2,
    CTO_CompEndEmph2,
    CTO_CompBegEmph3,
    CTO_CompEndEmph3,
    CTO_CompCapSign,
    CTO_CompBegCaps,
    CTO_CompEndCaps,
    CTO_EndComp,
    CTO_MultInd,
    CTO_CompDots,
    CTO_Comp6,
    CTO_Class,			/*define a character class */
    CTO_After,			/*only match if after character in class */
    CTO_Before,			/*only match if before character in class               30      */
    CTO_SwapCd,
    CTO_SwapDd,
    CTO_Space,
    CTO_Digit,
    CTO_Punctuation,
    CTO_Math,
    CTO_Sign,
    CTO_Letter,
    CTO_UpperCase,
    CTO_LowerCase,
    CTO_UpLow,
    CTO_LitDigit,
    CTO_Display,
    CTO_Replace,		/*replace this string with another */
    CTO_Context,
    CTO_Correct,
    CTO_Pass2,
    CTO_Pass3,
    CTO_Pass4,
    CTO_Repeated,		/*take just the first, i.e. multiple blanks     */
    CTO_CapsNoCont,
    CTO_Always,			/*always use this contraction                                           9       */
    CTO_NoCross,
    CTO_Syllable,
    CTO_NoCont,
    CTO_CompBrl,
    CTO_Literal,
    CTO_LargeSign,		/*and, for, the with a */
    CTO_WholeWord,		/*whole word contraction */
    CTO_PartWord,
    CTO_JoinNum,
    CTO_JoinableWord,		/*to, by, into */
    CTO_LowWord,		/*enough, were, was, etc. */
    CTO_Contraction,		/*multiletter word sign that needs letsign */
    CTO_SuffixableWord,		/*whole word or beginning of word */
    CTO_PrefixableWord,		/*whole word or end of word */
    CTO_BegWord,		/*beginning of word only */
    CTO_BegMidWord,		/*beginning or middle of word */
    CTO_MidWord,		/*middle of word only                                                                           20      */
    CTO_MidEndWord,		/*middle or end of word */
    CTO_EndWord,		/*end of word only */
    CTO_PrePunc,		/*punctuation in string at beginning of word */
    CTO_PostPunc,		/*punctuation in string at end of word */
    CTO_BegNum,			/*beginning of number */
    CTO_MidNum,			/*middle of number, e.g., decimal point */
    CTO_EndNum,			/*end of number */
    CTO_DecPoint,
    CTO_Hyphen,
    CTO_None,
/*Internal opcodes */
    CTO_CapitalRule,
    CTO_BeginCapitalRule,	//-             40
    CTO_EndCapitalRule,
    CTO_FirstWordCapsRule,
    CTO_LastWordCapsAfterRule,
    CTO_LetterRule,
    CTO_NumberRule,
    CTO_FirstWordItalRule,
    CTO_LastWordItalBeforeRule,
    CTO_LastWordItalAfterRule,
    CTO_FirstLetterItalRule,
    CTO_LastLetterItalRule,
    CTO_SingleLetterItalRule,
    CTO_ItalWordRule,
    CTO_FirstWordBoldRule,
    CTO_LastWordBoldBeforeRule,
    CTO_LastWordBoldAfterRule,
    CTO_FirstLetterBoldRule,
    CTO_LastLetterBoldRule,
    CTO_SingleLetterBoldRule,
    CTO_BoldWordRule,
    CTO_FirstWordUnderRule,
    CTO_LastWordUnderBeforeRule,
    CTO_LastWordUnderAfterRule,
    CTO_FirstLetterUnderRule,
    CTO_LastLetterUnderRule,
    CTO_SingleLetterUnderRule,
    CTO_UnderWordRule,
    CTO_BegCompRule,
    CTO_CompBegEmph1Rule,
    CTO_CompEndEmph1Rule,
    CTO_CompBegEmph2Rule,
    CTO_CompEndEmrh2Rule,
    CTO_CompBegEmph3Rule,
    CTO_CompEndEmph3Rule,
    CTO_CompCapSignRule,
    CTO_CompBegCapsRule,
    CTO_CompEndCapsRule,
    CTO_EndCompRule,
    CTO_CapsNoContRule,
    CTO_All			//-             44
  } ContractionTableOpcode;

  typedef struct
  {
    ContractionTableOffset charsnext;	/*next chars entry */
    ContractionTableOffset dotsnext;	/*next dots entry */
    ContractionTableCharacterAttributes after;	/*character types which must foollow */
    ContractionTableCharacterAttributes before;	/*character types which must 
						   precede */
    ContractionTableOpcode opcode;	/*rule for testing validity of replacement */
    short charslen;		/*length of string to be replaced */
    short dotslen;		/*length of replacement string */
    widechar charsdots[DEFAULTRULESIZE];	/*find and replacement 
						   strings */
  } ContractionTableRule;

  typedef struct		/*state transition */
  {
    widechar ch;
    widechar newState;
  } HyphenationTrans;

  typedef union
  {
    HyphenationTrans *pointer;
    ContractionTableOffset offset;
  } PointOff;

  typedef struct		/*one state */
  {
    PointOff trans;
    ContractionTableOffset hyphenPattern;
    widechar fallbackState;
    widechar numTrans;
  } HyphenationState;

  /*Contraction table header */
  typedef struct
  {				/*translation table */
    ContractionTableOffset capitalSign;	/*capitalization sign */
    ContractionTableOffset beginCapitalSign;	/*begin capitals sign */
    ContractionTableOffset lenBeginCaps;
    ContractionTableOffset endCapitalSign;	/*end capitals sign */
    ContractionTableOffset firstWordCaps;
    ContractionTableOffset lastWordCapsAfter;
    ContractionTableOffset lenCapsPhrase;
    ContractionTableOffset letterSign;
    ContractionTableOffset numberSign;	/*number sign */
    /*Do not change the order of the following emphasis opcodes */
    ContractionTableOffset firstWordItal;
    ContractionTableOffset lastWordItalBefore;
    ContractionTableOffset lastWordItalAfter;
    ContractionTableOffset firstLetterItal;
    ContractionTableOffset lastLetterItal;
    ContractionTableOffset singleLetterItal;
    ContractionTableOffset italWord;
    ContractionTableOffset lenItalPhrase;
    ContractionTableOffset firstWordBold;
    ContractionTableOffset lastWordBoldBefore;
    ContractionTableOffset lastWordBoldAfter;
    ContractionTableOffset firstLetterBold;
    ContractionTableOffset lastLetterBold;
    ContractionTableOffset singleLetterBold;
    ContractionTableOffset boldWord;
    ContractionTableOffset lenBoldPhrase;
    ContractionTableOffset firstWordUnder;
    ContractionTableOffset lastWordUnderBefore;
    ContractionTableOffset lastWordUnderAfter;
    ContractionTableOffset firstLetterUnder;
    ContractionTableOffset lastLetterUnder;
    ContractionTableOffset singleLetterUnder;
    ContractionTableOffset underWord;
    ContractionTableOffset lenUnderPhrase;
    ContractionTableOffset begComp;
    ContractionTableOffset compBegEmph1;
    ContractionTableOffset compEndEmph1;
    ContractionTableOffset compBegEmph2;
    ContractionTableOffset compEndEmph2;
    ContractionTableOffset compBegEmph3;
    ContractionTableOffset compEndEmph3;
    ContractionTableOffset compCapSign;
    ContractionTableOffset compBegCaps;
    ContractionTableOffset compEndCaps;
    ContractionTableOffset endComp;
    ContractionTableOffset capsNoCont;
    ContractionTableOffset numPasses;
    ContractionTableOffset corrections;
    ContractionTableOffset syllables;
    ContractionTableOffset hyphenStatesArray;
    widechar noLetsignBefore[LETSIGNSIZE];
    ContractionTableOffset noLetsignBeforeCount;
    widechar noLetsign[LETSIGNSIZE];
    ContractionTableOffset noLetsignCount;
    widechar noLetsignAfter[LETSIGNSIZE];
    ContractionTableOffset noLetsignAfterCount;
    ContractionTableOffset characters[HASHNUM];	/*Character 
						   definitions */
    ContractionTableOffset dots[HASHNUM];	/*Dot definitions */
    ContractionTableOffset charToDots[HASHNUM];
    ContractionTableOffset dotsToChar[HASHNUM];
    ContractionTableOffset compdotsPattern[256];
    ContractionTableOffset swapDefinitions[NUMSWAPS];
    ContractionTableOffset attribOrSwapRules[5];
    ContractionTableOffset forRules[HASHNUM];	/*chains of forward rules */
    ContractionTableOffset backRules[HASHNUM];	/*Chains of backward rules */
    ContractionTableOffset ruleArea[1];	/*Space for storing all 
					   rules and values */
  } ContractionTableHeader;
  typedef enum
  {
    alloc_typebuf,
    alloc_destSpacing,
    alloc_passbuf1,
    alloc_passbuf2
  } AllocBuf;

  widechar getDotsForChar (widechar c);
  widechar getCharFromDots (widechar d);
  void *liblouis_allocMem (AllocBuf buffer, int srcmax, int destmax);
  void *get_table (const char *name);
  char *showString (widechar const *chars, int length);

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				/* __LOUIS_H */
