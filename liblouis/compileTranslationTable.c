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

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "louis.h"
#include "config.h"

/* Contributed by Michel Such <michel.such@free.fr */
#ifdef _WIN32

/* Adapted from BRLTTY code (see sys_progs_wihdows.h) */

#include <shlobj.h>

static void
noMemory (void)
{
  printf ("Insufficient memory: %s", strerror (errno), "\n");
  exit (3);
}

void *
reallocWrapper (void *address, size_t size)
{
  if (!(address = realloc (address, size)) && size)
    noMemory ();
  return address;
}

char *
strdupWrapper (const char *string)
{
  char *address = strdup (string);
  if (!address)
    noMemory ();
  return address;
}

char *EXPORT_CALL
lou_getProgramPath (void)
{
  char *path = NULL;
  HMODULE handle;

  if ((handle = GetModuleHandle (NULL)))
    {
      size_t size = 0X80;
      char *buffer = NULL;

      while (1)
	{
	  buffer = reallocWrapper (buffer, size <<= 1);

	  {
	    DWORD length = GetModuleFileName (handle, buffer, size);

	    if (!length)
	      {
		printf ("GetModuleFileName\n");
		exit (3);
		3;
	      }

	    if (length < size)
	      {
		buffer[length] = 0;
		path = strdupWrapper (buffer);

		while (length > 0)
		  if (path[--length] == '\\')
		    break;

		strncpy (path, path, length + 1);
		path[length + 1] = '\0';
		break;
	      }
	  }
	}

      free (buffer);
    }
  else
    {
      printf ("GetModuleHandle\n");
      exit (3);
    }

  return path;
}

#define PATH_SEP ';'
#define DIR_SEP '\\'
#else
#define PATH_SEP ':'
#define DIR_SEP '/'
#endif
/* End of MS contribution */

static char tablePath[MAXSTRING];
static FILE *logFile = NULL;

void EXPORT_CALL
lou_logFile (char *fileName)
{
  if (logFile == NULL)
    logFile = stderr;
  logFile = fopen (fileName, "w");
  if (!logFile)
    {
      fprintf (stderr, "Cannot open log file %s\n", fileName);
      logFile = stderr;
    }
}

void EXPORT_CALL
lou_logPrint (char *format, ...)
{
#ifndef __SYMBIAN32__
  va_list argp;
  if (logFile == NULL)
    logFile = stderr;
  va_start (argp, format);
  vfprintf (logFile, format, argp);
  fprintf (logFile, "\n");
  va_end (argp);
#endif
}

static int
eqasc2uni (const unsigned char *a, const widechar * b, const int len)
{
  int k;
  for (k = 0; k < len; k++)
    if ((widechar) a[k] != b[k])
      return 0;
  return 1;
}

typedef struct
{
  widechar length;
  widechar chars[MAXSTRING];
}
CharsString;

static int errorCount;

static TranslationTableHeader *table;
static TranslationTableOffset tableSize;
static TranslationTableOffset tableUsed;

static const char *characterClassNames[] = {
  "space",
  "letter",
  "digit",
  "punctuation",
  "uppercase",
  "lowercase",
  "math",
  "sign",
  "litdigit",
  NULL
};

struct CharacterClass
{
  struct CharacterClass *next;
  TranslationTableCharacterAttributes attribute;
  widechar length;
  widechar name[1];
};
static struct CharacterClass *characterClasses;
static TranslationTableCharacterAttributes characterClassAttribute;

static const char *opcodeNames[CTO_None] = {
  "include",
  "locale",
  "undefined",
  "capsign",
  "begcaps",
  "lenbegcaps",
  "endcaps",
  "firstwordcaps",
  "lastwordbeforecaps",
  "lastwordaftercaps",
  "lencapsphrase",
  "letsign",
  "noletsignbefore",
  "noletsign",
  "noletsignafter",
  "numsign",
  "firstwordital",
  "italsign",
  "lastworditalbefore",
  "lastworditalafter",
  "begital",
  "firstletterital",
  "endital",
  "lastletterital",
  "singleletterital",
  "italword",
  "lenitalphrase",
  "firstwordbold",
  "boldsign",
  "lastwordboldbefore",
  "lastwordboldafter",
  "begbold",
  "firstletterbold",
  "endbold",
  "lastletterbold",
  "singleletterbold",
  "boldword",
  "lenboldphrase",
  "firstwordunder",
  "undersign",
  "lastwordunderbefore",
  "lastwordunderafter",
  "begunder",
  "firstletterunder",
  "endunder",
  "lastletterunder",
  "singleletterunder",
  "underword",
  "lenunderphrase",
  "begcomp",
  "compbegemph1",
  "compendemph1",
  "compbegemph2",
  "compendemph2",
  "compbegemph3",
  "compendemph3",
  "compcapsign",
  "compbegcaps",
  "compendcaps",
  "endcomp",
  "multind",
  "compdots",
  "comp6",
  "class",
  "after",
  "before",
  "noback",
  "nofor",
  "swapcc",
  "swapcd",
  "swapdd",
  "space",
  "digit",
  "punctuation",
  "math",
  "sign",
  "letter",
  "uppercase",
  "lowercase",
  "grouping",
  "uplow",
  "litdigit",
  "display",
  "replace",
  "context",
  "correct",
  "pass2",
  "pass3",
  "pass4",
  "repeated",
  "repword",
  "capsnocont",
  "always",
  "exactdots",
  "nocross",
  "syllable",
  "nocont",
  "compbrl",
  "literal",
  "largesign",
  "word",
  "partword",
  "joinnum",
  "joinword",
  "lowword",
  "contraction",
  "sufword",
  "prfword",
  "begword",
  "begmidword",
  "midword",
  "midendword",
  "endword",
  "prepunc",
  "postpunc",
  "begnum",
  "midnum",
  "endnum",
  "decpoint",
  "hyphen",
  "nobreak"
};
static short opcodeLengths[CTO_None] = { 0 };

typedef enum
{ noEncoding, bigEndian, littleEndian, ascii8 } EncodingType;


typedef struct
{
  const char *fileName;
  FILE *in;
  int lineNumber;
  EncodingType encoding;
  int status;
  int linepos;
  int checkencoding[2];
  widechar line[MAXSTRING];
}
FileInfo;

static char scratchBuf[MAXSTRING];

char *
showString (widechar const *chars, int length)
{
/*Translate a string of characters to the encoding used in character 
* operands */
  int charPos;
  int bufPos = 0;
  scratchBuf[bufPos++] = '\'';
  for (charPos = 0; charPos < length; charPos++)
    {
      if (chars[charPos] > 32 && chars[charPos] < 127)
	scratchBuf[bufPos++] = (char) chars[charPos];
      else
	{
	  char hexbuf[20];
	  int hexLength;
	  char escapeLetter;

	  int leadingZeros;
	  int hexPos;
	  hexLength = sprintf (hexbuf, "%x", chars[charPos]);
	  switch (hexLength)
	    {
	    case 1:
	    case 2:
	    case 3:
	    case 4:
	      escapeLetter = 'x';
	      leadingZeros = 4 - hexLength;
	      break;
	    case 5:
	      escapeLetter = 'y';
	      leadingZeros = 0;
	      break;
	    case 6:
	    case 7:
	    case 8:
	      escapeLetter = 'z';
	      leadingZeros = 8 - hexLength;
	      break;
	    default:
	      escapeLetter = '?';
	      leadingZeros = 0;
	      break;
	    }
	  if ((bufPos + leadingZeros + hexLength + 4) >= sizeof (scratchBuf))
	    break;
	  scratchBuf[bufPos++] = '\\';
	  scratchBuf[bufPos++] = escapeLetter;
	  for (hexPos = 0; hexPos < leadingZeros; hexPos++)
	    scratchBuf[bufPos++] = '0';
	  for (hexPos = 0; hexPos < hexLength; hexPos++)
	    scratchBuf[bufPos++] = hexbuf[hexPos];
	}
    }
  scratchBuf[bufPos++] = '\'';
  scratchBuf[bufPos] = 0;
  return scratchBuf;
}

char *
showDots (widechar const *dots, int length)
{
/* Translate a sequence of dots to the encoding used in dots operands. 
*/
  int bufPos = 0;
  int dotsPos;
  for (dotsPos = 0; bufPos < sizeof (scratchBuf) && dotsPos < length;
       dotsPos++)
    {
      if ((dots[dotsPos] & B1))
	scratchBuf[bufPos++] = '1';
      if ((dots[dotsPos] & B2))
	scratchBuf[bufPos++] = '2';
      if ((dots[dotsPos] & B3))
	scratchBuf[bufPos++] = '3';
      if ((dots[dotsPos] & B4))
	scratchBuf[bufPos++] = '4';
      if ((dots[dotsPos] & B5))
	scratchBuf[bufPos++] = '5';
      if ((dots[dotsPos] & B6))
	scratchBuf[bufPos++] = '6';
      if ((dots[dotsPos] & B7))
	scratchBuf[bufPos++] = '7';
      if ((dots[dotsPos] & B8))
	scratchBuf[bufPos++] = '8';
      if ((dots[dotsPos] & B9))
	scratchBuf[bufPos++] = '9';
      if ((dots[dotsPos] & B10))
	scratchBuf[bufPos++] = 'A';
      if ((dots[dotsPos] & B11))
	scratchBuf[bufPos++] = 'B';
      if ((dots[dotsPos] & B12))
	scratchBuf[bufPos++] = 'C';
      if ((dots[dotsPos] & B13))
	scratchBuf[bufPos++] = 'D';
      if ((dots[dotsPos] & B14))
	scratchBuf[bufPos++] = 'E';
      if ((dots[dotsPos] & B15))
	scratchBuf[bufPos++] = 'F';
      if ((dots[dotsPos] == B16))
	scratchBuf[bufPos++] = '0';
      if (dotsPos != length - 1)
	scratchBuf[bufPos++] = '-';
    }
  scratchBuf[bufPos] = 0;
  return &scratchBuf[0];
}

char *
showAttributes (TranslationTableCharacterAttributes a)
{
/* Show attributes using the letters used after the $ in multipass 
* opcodes. */
  int bufPos = 0;
  if ((a & CTC_Space))
    scratchBuf[bufPos++] = 's';
  if ((a & CTC_Letter))
    scratchBuf[bufPos++] = 'l';
  if ((a & CTC_Digit))
    scratchBuf[bufPos++] = 'd';
  if ((a & CTC_Punctuation))
    scratchBuf[bufPos++] = 'p';
  if ((a & CTC_UpperCase))
    scratchBuf[bufPos++] = 'U';
  if ((a & CTC_LowerCase))
    scratchBuf[bufPos++] = 'u';
  if ((a & CTC_Math))
    scratchBuf[bufPos++] = 'm';
  if ((a & CTC_Sign))
    scratchBuf[bufPos++] = 'S';
  if ((a & CTC_LitDigit))
    scratchBuf[bufPos++] = 'D';
  if ((a & CTC_Class1))
    scratchBuf[bufPos++] = 'w';
  if ((a & CTC_Class2))
    scratchBuf[bufPos++] = 'x';
  if ((a & CTC_Class3))
    scratchBuf[bufPos++] = 'y';
  if ((a & CTC_Class4))
    scratchBuf[bufPos++] = 'z';
  scratchBuf[bufPos] = 0;
  return scratchBuf;
}

static void compileError (FileInfo * nested, char *format, ...);

static int
getAChar (FileInfo * nested)
{
/*Read a big endian, little *ndian or ASCII 8 file and convert it to 
* 16- or 32-bit unsigned integers */
  int ch1 = 0, ch2 = 0;
  widechar character;
  if (nested->encoding == ascii8)
    if (nested->status == 2)
      {
	nested->status++;
	return nested->checkencoding[1];
      }
  while ((ch1 = fgetc (nested->in)) != EOF)
    {
      if (nested->status < 2)
	nested->checkencoding[nested->status] = ch1;
      nested->status++;
      if (nested->status == 2)
	{
	  if (nested->checkencoding[0] == 0xfe
	      && nested->checkencoding[1] == 0xff)
	    nested->encoding = bigEndian;
	  else if (nested->checkencoding[0] == 0xff
		   && nested->checkencoding[1] == 0xfe)
	    nested->encoding = littleEndian;
	  else if (nested->checkencoding[0] < 128
		   && nested->checkencoding[1] < 128)
	    {
	      nested->encoding = ascii8;
	      return nested->checkencoding[0];
	    }
	  else
	    {
	      compileError (nested,
			    "encoding is neither big-endian, little-endian nor ASCII 8.");
	      ch1 = EOF;
	      break;;
	    }
	  continue;
	}
      switch (nested->encoding)
	{
	case noEncoding:
	  break;
	case ascii8:
	  return ch1;
	  break;
	case bigEndian:
	  ch2 = fgetc (nested->in);
	  if (ch2 == EOF)
	    break;
	  character = (ch1 << 8) | ch2;
	  return (int) character;
	  break;
	case littleEndian:
	  ch2 = fgetc (nested->in);
	  if (ch2 == EOF)
	    break;
	  character = (ch2 << 8) | ch1;
	  return (int) character;
	  break;
	}
      if (ch1 == EOF || ch2 == EOF)
	break;
    }
  return EOF;
}

static int
getALine (FileInfo * nested)
{
/*Read a line of widechar's from an input file */
  int ch;
  int numchars = 0;
  while ((ch = getAChar (nested)) != EOF)
    {
      if (ch == 13)
	continue;
      if (ch == 10 || numchars >= MAXSTRING)
	break;
      nested->line[numchars++] = (widechar) ch;
    }
  nested->line[numchars] = 0;
  nested->linepos = 0;
  if (ch == EOF)
    return 0;
  nested->lineNumber++;
  return 1;
}

static int lastToken;
static int
getToken (FileInfo * nested, CharsString * result, const char *description)
{
/*Find the next string of contiguous nonblank characters. If this is the 
* last token on the line, return 2 instead of 1. */
  while (nested->line[nested->linepos] &&
	 (nested->line[nested->linepos] == 32
	  || nested->line[nested->linepos] == 9))
    nested->linepos++;
  result->length = 0;
  while (nested->line[nested->linepos] &&
	 !(nested->line[nested->linepos] == 32
	   || nested->line[nested->linepos] == 9))
    result->chars[result->length++] = nested->line[nested->linepos++];
  if (!result->length)
    {
      if (description)
	compileError (nested, "%s not specified.", description);
      return 0;
    }
  result->chars[result->length] = 0;
  while (nested->line[nested->linepos] &&
	 (nested->line[nested->linepos] == 32
	  || nested->line[nested->linepos] == 9))
    nested->linepos++;
  if (nested->line[nested->linepos] == 0)
    {
      lastToken = 1;
      return 2;
    }
  else
    {
      lastToken = 0;
      return 1;
    }
}

static void
compileError (FileInfo * nested, char *format, ...)
{
#ifndef __SYMBIAN32__
  char buffer[MAXSTRING];
  va_list arguments;
  va_start (arguments, format);
#ifdef _WIN32
  (void) _vsnprintf (buffer, sizeof (buffer), format, arguments);
#else
  (void) vsnprintf (buffer, sizeof (buffer), format, arguments);
#endif
  va_end (arguments);
  if (nested)
    lou_logPrint ("%s:%d: %s", nested->fileName, nested->lineNumber, buffer);
  else
    lou_logPrint ("%s", buffer);
  errorCount++;
#endif
}

static int
allocateSpaceInTable (FileInfo * nested, TranslationTableOffset * offset,
		      int count)
{
/* allocate memory for translation table and expand previously allocated 
* memory if necessary */
  int spaceNeeded = ((count + OFFSETSIZE - 1) / OFFSETSIZE) * OFFSETSIZE;
  TranslationTableOffset size = tableUsed + spaceNeeded;
  if (size > tableSize)
    {
      void *newTable;
      size += (size / OFFSETSIZE);
      newTable = realloc (table, size);
      if (!newTable)
	{
	  compileError (nested, "Not enough memory for translation table.");
	  return 0;
	}
      memset (((unsigned char *) newTable) + tableSize, 0, size - tableSize);
      table = (TranslationTableHeader *) newTable;
      tableSize = size;
    }
  if (offset != NULL)
    {
      *offset = (tableUsed - sizeof (*table)) / OFFSETSIZE;
      tableUsed += spaceNeeded;
    }
  return 1;
}

static int
reserveSpaceInTable (FileInfo * nested, int count)
{
  return (allocateSpaceInTable (nested, NULL, count));
}

static int
allocateHeader (FileInfo * nested)
{
/*Allocate memory for the table header and a guess on the number of 
* rules */
  const TranslationTableOffset startSize = 2 * sizeof (*table);
  if (table)
    return 1;
  tableUsed = sizeof (*table) + OFFSETSIZE;	/*So no offset is ever zero */
  if (!(table = malloc (startSize)))
    {
      compileError (nested, "Not enough memory");
      if (table != NULL)
	free (table);
      table = NULL;
      return 0;
    }
  memset (table, 0, startSize);
  tableSize = startSize;
  return 1;
}

int
stringHash (const widechar * c)
{
/*hash function for strings */
  unsigned long int makeHash = (((unsigned long int) c[0] << 8) +
				(unsigned long int) c[1]) % HASHNUM;
  return (int) makeHash;
}

int
charHash (widechar c)
{
  unsigned long int makeHash = (unsigned long int) c % HASHNUM;
  return (int) makeHash;
}

static TranslationTableCharacter *
compile_findCharOrDots (widechar c, int m)
{
/*Look up a character or dot pattern. If m is 0 look up a cearacter, 
* otherwise look up a dot pattern. Although the algorithms are almost 
* identical, different tables are needed for characters and dots because 
* of the possibility of conflicts.*/
  TranslationTableCharacter *character;
  TranslationTableOffset bucket;
  unsigned long int makeHash = (unsigned long int) c % HASHNUM;
  if (m == 0)
    bucket = table->characters[makeHash];
  else
    bucket = table->dots[makeHash];
  while (bucket)
    {
      character = (TranslationTableCharacter *) & table->ruleArea[bucket];
      if (character->realchar == c)
	return character;
      bucket = character->next;
    }
  return NULL;
}

static TranslationTableCharacter noChar = { 0, 0, 0, CTC_Space, 32, 32, 32 };
static TranslationTableCharacter noDots =
  { 0, 0, 0, CTC_Space, B16, B16, B16 };
static char *unknownDots (widechar dots);

static TranslationTableCharacter *
definedCharOrDots (FileInfo * nested, widechar c, int m)
{
  TranslationTableCharacter *notFound;
  TranslationTableCharacter *charOrDots = compile_findCharOrDots (c, m);
  if (charOrDots)
    return charOrDots;
  if (m == 0)
    {
      notFound = &noChar;
      compileError (nested,
		    "character %s should be defined at this point but is not",
		    showString (&c, 1));
    }
  else
    {
      notFound = &noDots;
      compileError (nested,
		    "cell %s should be defined at this point but is not",
		    unknownDots (c));
    }
  return notFound;
}

static TranslationTableCharacter *
addCharOrDots (FileInfo * nested, widechar c, int m)
{
/*See if a character or dot pattern is in the appropriate table. If not, 
* insert it. In either 
* case, return a pointer to it. */
  TranslationTableOffset bucket;
  TranslationTableCharacter *character;
  TranslationTableCharacter *oldchar;
  TranslationTableOffset offset;
  unsigned long int makeHash;
  if ((character = compile_findCharOrDots (c, m)))
    return character;
  if (!allocateSpaceInTable (nested, &offset, sizeof (*character)))
    return NULL;
  character = (TranslationTableCharacter *) & table->ruleArea[offset];
  memset (character, 0, sizeof (*character));
  character->realchar = c;
  makeHash = (unsigned long int) c % HASHNUM;
  if (m == 0)
    bucket = table->characters[makeHash];
  else
    bucket = table->dots[makeHash];
  if (!bucket)
    {
      if (m == 0)
	table->characters[makeHash] = offset;
      else
	table->dots[makeHash] = offset;
    }
  else
    {
      oldchar = (TranslationTableCharacter *) & table->ruleArea[bucket];
      while (oldchar->next)
	oldchar =
	  (TranslationTableCharacter *) & table->ruleArea[oldchar->next];
      oldchar->next = offset;
    }
  return character;
}

static CharOrDots *
getCharOrDots (widechar c, int m)
{
  CharOrDots *cdPtr;
  TranslationTableOffset bucket;
  unsigned long int makeHash = (unsigned long int) c % HASHNUM;
  if (m == 0)
    bucket = table->charToDots[makeHash];
  else
    bucket = table->dotsToChar[makeHash];
  while (bucket)
    {
      cdPtr = (CharOrDots *) & table->ruleArea[bucket];
      if (cdPtr->lookFor == c)
	return cdPtr;
      bucket = cdPtr->next;
    }
  return NULL;
}

widechar
getDotsForChar (widechar c)
{
  CharOrDots *cdPtr = getCharOrDots (c, 0);
  if (cdPtr)
    return cdPtr->found;
  return B16;
}

widechar
getCharFromDots (widechar d)
{
  CharOrDots *cdPtr = getCharOrDots (d, 1);
  if (cdPtr)
    return cdPtr->found;
  return ' ';
}

static int
putCharAndDots (FileInfo * nested, widechar c, widechar d)
{
  TranslationTableOffset bucket;
  CharOrDots *cdPtr;
  CharOrDots *oldcdPtr = NULL;
  TranslationTableOffset offset;
  unsigned long int makeHash;
  if (!(cdPtr = getCharOrDots (c, 0)))
    {
      if (!allocateSpaceInTable (nested, &offset, sizeof (*cdPtr)))
	return 0;
      cdPtr = (CharOrDots *) & table->ruleArea[offset];
      cdPtr->next = 0;
      cdPtr->lookFor = c;
      cdPtr->found = d;
      makeHash = (unsigned long int) c % HASHNUM;
      bucket = table->charToDots[makeHash];
      if (!bucket)
	table->charToDots[makeHash] = offset;
      else
	{
	  oldcdPtr = (CharOrDots *) & table->ruleArea[bucket];
	  while (oldcdPtr->next)
	    oldcdPtr = (CharOrDots *) & table->ruleArea[oldcdPtr->next];
	  oldcdPtr->next = offset;
	}
    }
  if (!(cdPtr = getCharOrDots (d, 1)))
    {
      if (!allocateSpaceInTable (nested, &offset, sizeof (*cdPtr)))
	return 0;
      cdPtr = (CharOrDots *) & table->ruleArea[offset];
      cdPtr->next = 0;
      cdPtr->lookFor = d;
      cdPtr->found = c;
      makeHash = (unsigned long int) d % HASHNUM;
      bucket = table->dotsToChar[makeHash];
      if (!bucket)
	table->dotsToChar[makeHash] = offset;
      else
	{
	  if (c == 0xb4)
	    oldcdPtr = (CharOrDots *) & table->ruleArea[bucket];
	  while (oldcdPtr->next)
	    oldcdPtr = (CharOrDots *) & table->ruleArea[oldcdPtr->next];
	  oldcdPtr->next = offset;
	}
    }
  return 1;
}

static char *
unknownDots (widechar dots)
{
/*Print out dot numbers */
  static char buffer[20];
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
  buffer[k] = 0;
  return buffer;
}

static TranslationTableOffset newRuleOffset = 0;
static TranslationTableRule *newRule = NULL;

static int
charactersDefined (FileInfo * nested)
{
/*Check that all characters are defined by character-definition 
* opcodes*/
  int noErrors = 1;
  int k;
  if ((newRule->opcode >= CTO_Space && newRule->opcode <= CTO_LitDigit)
      || newRule->opcode == CTO_SwapDd
      ||
      newRule->opcode == CTO_Replace || newRule->opcode == CTO_MultInd
      || newRule->opcode == CTO_Repeated ||
      ((newRule->opcode >= CTO_Context && newRule->opcode <=
	CTO_Pass4) && newRule->opcode != CTO_Correct))
    return 1;
  for (k = 0; k < newRule->charslen; k++)
    if (!compile_findCharOrDots (newRule->charsdots[k], 0))
      {
	compileError (nested, "Character %s is not defined", showString
		      (&newRule->charsdots[k], 1));
	noErrors = 0;
      }
  if (!(newRule->opcode == CTO_Correct || newRule->opcode ==
	CTO_NoBreak || newRule->opcode == CTO_SwapCc || newRule->opcode ==
	CTO_SwapCd))
    {
      for (k = newRule->charslen; k < newRule->charslen + newRule->dotslen;
	   k++)
	if (!compile_findCharOrDots (newRule->charsdots[k], 1))
	  {
	    compileError (nested, "Dot pattern %s is not defined.",
			  unknownDots (newRule->charsdots[k]));
	    noErrors = 0;
	  }
    }
  return noErrors;
}

static int noback = 0;
static int nofor = 0;

/*The following functions are 
called by addRule to handle various 
* cases.*/

static void
add_0_single (FileInfo * nested)
{
/*direction = 0, newRule->charslen = 1*/
  TranslationTableRule *currentRule;
  TranslationTableOffset *currentOffsetPtr;
  TranslationTableCharacter *character;
  int m = 0;
  if (newRule->opcode == CTO_CompDots || newRule->opcode == CTO_Comp6)
    return;
  if (newRule->opcode >= CTO_Pass2 && newRule->opcode <= CTO_Pass4)
    m = 1;
  character = definedCharOrDots (nested, newRule->charsdots[0], m);
  if (m != 1 && character->attributes & CTC_Letter && (newRule->opcode
						       ==
						       CTO_WholeWord
						       || newRule->opcode ==
						       CTO_LargeSign))
    {
      if (table->noLetsignCount < LETSIGNSIZE)
	table->noLetsign[table->noLetsignCount++] = newRule->charsdots[0];
    }
  if (newRule->opcode >= CTO_Space && newRule->opcode < CTO_UpLow)
    character->definitionRule = newRuleOffset;
  currentOffsetPtr = &character->otherRules;
  while (*currentOffsetPtr)
    {
      currentRule = (TranslationTableRule *)
	& table->ruleArea[*currentOffsetPtr];
      if (currentRule->charslen == 0)
	break;
      if (currentRule->opcode >= CTO_Space && currentRule->opcode < CTO_UpLow)
	if (!(newRule->opcode >= CTO_Space && newRule->opcode < CTO_UpLow))
	  break;
      currentOffsetPtr = &currentRule->charsnext;
    }
  newRule->charsnext = *currentOffsetPtr;
  *currentOffsetPtr = newRuleOffset;
}

static void
add_0_multiple (void)
{
/*direction = 0 newRule->charslen > 1*/
  TranslationTableRule *currentRule = NULL;
  TranslationTableOffset *currentOffsetPtr =
    &table->forRules[stringHash (&newRule->charsdots[0])];
  while (*currentOffsetPtr)
    {
      currentRule = (TranslationTableRule *)
	& table->ruleArea[*currentOffsetPtr];
      if (newRule->charslen > currentRule->charslen)
	break;
      if (newRule->charslen == currentRule->charslen)
	if ((currentRule->opcode == CTO_Always)
	    && (newRule->opcode != CTO_Always))
	  break;
      currentOffsetPtr = &currentRule->charsnext;
    }
  newRule->charsnext = *currentOffsetPtr;
  *currentOffsetPtr = newRuleOffset;
}

static void
add_1_single (FileInfo * nested)
{
/*direction = 1, newRule->dotslen = 1*/
  TranslationTableRule *currentRule;
  TranslationTableOffset *currentOffsetPtr;
  TranslationTableCharacter *dots;
  if (newRule->opcode == CTO_NoBreak || newRule->opcode == CTO_SwapCc ||
      (newRule->opcode >= CTO_Context
       &&
       newRule->opcode <= CTO_Pass4)
      || newRule->opcode == CTO_Repeated || (newRule->opcode == CTO_Always
					     && newRule->charslen == 1))
    return;			/*too ambiguous */
  dots = definedCharOrDots (nested, newRule->charsdots[newRule->charslen], 1);
  if (newRule->opcode >= CTO_Space && newRule->opcode < CTO_UpLow)
    dots->definitionRule = newRuleOffset;
  currentOffsetPtr = &dots->otherRules;
  while (*currentOffsetPtr)
    {
      currentRule = (TranslationTableRule *)
	& table->ruleArea[*currentOffsetPtr];
      if (newRule->charslen > currentRule->charslen ||
	  currentRule->dotslen == 0)
	break;
      if (currentRule->opcode >= CTO_Space && currentRule->opcode < CTO_UpLow)
	if (!(newRule->opcode >= CTO_Space && newRule->opcode < CTO_UpLow))
	  break;
      currentOffsetPtr = &currentRule->dotsnext;
    }
  newRule->dotsnext = *currentOffsetPtr;
  *currentOffsetPtr = newRuleOffset;
}

static void
add_1_multiple (void)
{
/*direction = 1, newRule->dotslen > 1*/
  TranslationTableRule *currentRule = NULL;
  TranslationTableOffset *currentOffsetPtr = &table->backRules[stringHash
							       (&newRule->
								charsdots
								[newRule->
								 charslen])];
  if (newRule->opcode == CTO_NoBreak || newRule->opcode == CTO_SwapCc ||
      (newRule->opcode >= CTO_Context && newRule->opcode <= CTO_Pass4))
    return;
  while (*currentOffsetPtr)
    {
      int currentLength;
      int newLength;
      currentRule = (TranslationTableRule *)
	& table->ruleArea[*currentOffsetPtr];
      currentLength = currentRule->dotslen + currentRule->charslen;
      newLength = newRule->dotslen + newRule->charslen;
      if (newLength > currentLength)
	break;
      if (currentLength == newLength)
	if ((currentRule->opcode == CTO_Always)
	    && (newRule->opcode != CTO_Always))
	  break;
      currentOffsetPtr = &currentRule->dotsnext;
    }
  newRule->dotsnext = *currentOffsetPtr;
  *currentOffsetPtr = newRuleOffset;
}

static void
makeRuleChain (TranslationTableOffset * offsetPtr)
{
  TranslationTableRule *currentRule;
  TranslationTableRule *prevRule = NULL;
  while (*offsetPtr)
    {
      currentRule = (TranslationTableRule *) & table->ruleArea[*offsetPtr];
      if (prevRule != NULL && newRule->after > currentRule->after)
	{
	  prevRule->charsnext = newRuleOffset;
	  newRule->charsnext = *offsetPtr;
	  return;
	}
      prevRule = currentRule;
      offsetPtr = &currentRule->charsnext;
    }
  newRule->charsnext = *offsetPtr;
  *offsetPtr = newRuleOffset;
}

static int
addPassRule (FileInfo * nested)
{
  TranslationTableOffset *offsetPtr;
  switch (newRule->opcode)
    {
    case CTO_Correct:
      offsetPtr = &table->attribOrSwapRules[0];
      break;
    case CTO_Context:
      offsetPtr = &table->attribOrSwapRules[1];
      break;
    case CTO_Pass2:
      offsetPtr = &table->attribOrSwapRules[2];
      break;
    case CTO_Pass3:
      offsetPtr = &table->attribOrSwapRules[3];
      break;
    case CTO_Pass4:
      offsetPtr = &table->attribOrSwapRules[4];
      break;
    default:
      return 0;
    }
  makeRuleChain (offsetPtr);
  return 1;
}

static int
  addRule
  (FileInfo * nested,
   TranslationTableOpcode opcode,
   CharsString * ruleChars,
   CharsString * ruleDots,
   TranslationTableCharacterAttributes after,
   TranslationTableCharacterAttributes before)
{
/*Add a rule to the table, using the hash function to find the start of 
* chains and chaining both the chars and dots strings */
  int ruleSize = sizeof (TranslationTableRule) - (DEFAULTRULESIZE * CHARSIZE);
  int direction = 0;		/*0 = forward translation; 1 = bacward */
  if (ruleChars)
    ruleSize += CHARSIZE * ruleChars->length;
  if (ruleDots)
    ruleSize += CHARSIZE * ruleDots->length;
  if (!allocateSpaceInTable (nested, &newRuleOffset, ruleSize))
    return 0;
  newRule = (TranslationTableRule *) & table->ruleArea[newRuleOffset];
  newRule->opcode = opcode;
  newRule->after = after;
  newRule->before = before;
  if (ruleChars)
    memcpy (&newRule->charsdots[0], &ruleChars->chars[0],
	    CHARSIZE * (newRule->charslen = ruleChars->length));
  else
    newRule->charslen = 0;
  if (ruleDots)
    memcpy (&newRule->charsdots[newRule->charslen],
	    &ruleDots->chars[0], CHARSIZE * (newRule->dotslen =
					     ruleDots->length));
  else
    newRule->dotslen = 0;
  if (!charactersDefined (nested))
    return 0;

  /*link new rule into table. */
  if (opcode == CTO_SwapCc || opcode == CTO_SwapCd || opcode == CTO_SwapDd)
    return 1;
  if (opcode >= CTO_Context && opcode <= CTO_Pass4 && newRule->charslen == 0)
    return addPassRule (nested);
  if (newRule->charslen == 0 || nofor)
    direction = 1;
  while (direction < 2)
    {
      if (direction == 0 && newRule->charslen == 1)
	add_0_single (nested);
      else if (direction == 0 && newRule->charslen > 1)
	add_0_multiple ();
      else if (direction == 1 && newRule->dotslen == 1 && !noback)
	add_1_single (nested);
      else if (direction == 1 && newRule->dotslen > 1 && !noback)
	add_1_multiple ();
      else
	{
	}
      direction++;
      if (newRule->dotslen == 0)
	direction = 2;
    }
  return 1;
}

static const struct CharacterClass *
findCharacterClass (const CharsString * name)
{
/*Find a character class, whether predefined or user-defined */
  const struct CharacterClass *class = characterClasses;
  while (class)
    {
      if ((name->length == class->length) &&
	  (memcmp (&name->chars[0], class->name, CHARSIZE *
		   name->length) == 0))
	return class;
      class = class->next;
    }
  return NULL;
}

static struct CharacterClass *
addCharacterClass (FileInfo * nested, const widechar * name, int length)
{
/*Define a character class, Whether predefined or user-defined */
  struct CharacterClass *class;
  if (characterClassAttribute)
    {
      if ((class = malloc (sizeof (*class) + CHARSIZE * (length - 1))))
	{
	  memset (class, 0, sizeof (*class));
	  memcpy (class->name, name, CHARSIZE * (class->length = length));
	  class->attribute = characterClassAttribute;
	  characterClassAttribute <<= 1;
	  class->next = characterClasses;
	  characterClasses = class;
	  return class;
	}
    }
  compileError (nested, "character class table overflow.");
  return NULL;
}

static void
deallocateCharacterClasses (void)
{
  while (characterClasses)
    {
      struct CharacterClass *class = characterClasses;
      characterClasses = characterClasses->next;
      if (class)
	free (class);
    }
}

static int
allocateCharacterClasses (void)
{
/*Allocate memory for predifined character classes */
  int k = 0;
  characterClasses = NULL;
  characterClassAttribute = 1;
  while (characterClassNames[k])
    {
      widechar wname[MAXSTRING];
      int length = strlen (characterClassNames[k]);
      int kk;
      for (kk = 0; kk < length; kk++)
	wname[kk] = (widechar) characterClassNames[k][kk];
      if (!addCharacterClass (NULL, wname, length))
	{
	  deallocateCharacterClasses ();
	  return 0;
	}
      k++;
    }
  return 1;
}

static TranslationTableOpcode
getOpcode (FileInfo * nested, const CharsString * token)
{
  static TranslationTableOpcode lastOpcode = 0;
  TranslationTableOpcode opcode = lastOpcode;

  do
    {
      if (token->length == opcodeLengths[opcode])
	if (eqasc2uni ((unsigned char *) opcodeNames[opcode],
		       &token->chars[0], token->length))
	  {
	    lastOpcode = opcode;
	    return opcode;
	  }
      opcode++;
      if (opcode >= CTO_None)
	opcode = 0;
    }
  while (opcode != lastOpcode);
  compileError (nested, "opcode %s not defined.", showString
		(&token->chars[0], token->length));
  return CTO_None;
}

TranslationTableOpcode
findOpcodeNumber (const char *toFind)
{
/* Used by tools such as lou_debug */
  static TranslationTableOpcode lastOpcode = 0;
  TranslationTableOpcode opcode = lastOpcode;
  int length = strlen (toFind);
  do
    {
      if (length == opcodeLengths[opcode] && strcasecmp (toFind,
							 opcodeNames[opcode])
	  == 0)
	{
	  lastOpcode = opcode;
	  return opcode;
	}
      opcode++;
      if (opcode >= CTO_None)
	opcode = 0;
    }
  while (opcode != lastOpcode);
  return CTO_None;
}

const char *
findOpcodeName (TranslationTableOpcode opcode)
{
/* Used by tools such as lou_debug */
  if (opcode < 0 || opcode >= CTO_None)
    {
      sprintf (scratchBuf, "%d", opcode);
      return scratchBuf;
    }
  return opcodeNames[opcode];
}

static widechar
hexValue (FileInfo * nested, const widechar * digits, int length)
{
  int k;
  unsigned int binaryValue = 0;
  for (k = 0; k < length; k++)
    {
      unsigned int hexDigit = 0;
      if (digits[k] >= '0' && digits[k] <= '9')
	hexDigit = digits[k] - '0';
      else if (digits[k] >= 'a' && digits[k] <= 'f')
	hexDigit = digits[k] - 'a' + 10;
      else if (digits[k] >= 'A' && digits[k] <= 'F')
	hexDigit = digits[k] - 'A' + 10;
      else
	{
	  compileError (nested, "invalid %d-digit hexadecimal number",
			length);
	  return (widechar) 0xffffffff;
	}
      binaryValue |= hexDigit << (4 * (length - 1 - k));
    }
  return (widechar) binaryValue;
}

static int
parseChars (FileInfo * nested, CharsString * result, CharsString * token)
{
/*interpret ruleChars string */
  int count = 0;
  int index;
  for (index = 0; index < token->length; index++)
    {
      widechar character = token->chars[index];
      if (character == '\\')
	{			/* escape sequence */
	  int ok = 0;
	  if (++index < token->length)
	    {
	      switch (character = token->chars[index])
		{
		case '\\':
		  ok = 1;
		  break;
		case 'e':
		  character = 0x1b;
		  ok = 1;
		  break;
		case 'f':
		  character = '\f';
		  ok = 1;
		  break;
		case 'n':
		  character = '\n';
		  ok = 1;
		  break;
		case 'r':
		  character = '\r';
		  ok = 1;
		  break;
		case 's':
		  character = ' ';
		  ok = 1;
		  break;
		case 't':
		  character = '\t';
		  ok = 1;
		  break;
		case 'v':
		  character = '\v';
		  ok = 1;
		  break;
		case 'w':
		  character = ENDSEGMENT;
		  ok = 1;
		  break;
		case 'X':
		case 'x':
		  if (token->length - index > 4)
		    {
		      character =
			hexValue (nested, &token->chars[index + 1], 4);
		      index += 4;
		      ok = 1;
		    }
		  break;
		case 'y':
		case 'Y':
		  if (CHARSIZE == 2)
		    {
		    not32:
		      compileError (nested,
				    "liblouis has not been compiled for 32-bit Unicode");
		      break;
		    }
		  if (token->length - index > 5)
		    {
		      character =
			hexValue (nested, &token->chars[index + 1], 5);
		      index += 5;
		      ok = 1;
		    }
		  break;
		case 'z':
		case 'Z':
		  if (CHARSIZE == 2)
		    goto not32;
		  if (token->length - index > 8)
		    {
		      character =
			hexValue (nested, &token->chars[index + 1], 8);
		      index += 8;
		      ok = 1;
		    }
		  break;
		}
	    }
	  if (!ok)
	    {
	      index++;
	      compileError (nested, "invalid escape sequence.");
	      return 0;
	    }
	}
      result->chars[count++] = character;
    }
  result->length = count;
  return 1;
}

int
extParseChars (const char *inString, widechar * outString)
{
/* Parse external character strings */
  CharsString wideIn;
  CharsString result;
  int k;
  for (k = 0; inString[k] && k < MAXSTRING; k++)
    wideIn.chars[k] = inString[k];
  wideIn.chars[k] = 0;
  wideIn.length = k;
  parseChars (NULL, &result, &wideIn);
  if (errorCount)
    {
      errorCount = 0;
      return 0;
    }
  for (k = 0; k < result.length; k++)
    outString[k] = result.chars[k];
  outString[k] = 0;
  return 1;
}

static int
parseDots (FileInfo * nested, CharsString * cells, const CharsString * token)
{
/*get dot patterns */
  widechar cell = 0;		/*assembly place for dots */
  int cellCount = 0;
  int index;
  int start = 0;

  for (index = 0; index < token->length; index++)
    {
      int started = index != start;
      widechar character = token->chars[index];
      switch (character)
	{			/*or dots to make up Braille cell */
	  {
	    int dot;
	case '1':
	    dot = B1;
	    goto haveDot;
	case '2':
	    dot = B2;
	    goto haveDot;
	case '3':
	    dot = B3;
	    goto haveDot;
	case '4':
	    dot = B4;
	    goto haveDot;
	case '5':
	    dot = B5;
	    goto haveDot;
	case '6':
	    dot = B6;
	    goto haveDot;
	case '7':
	    dot = B7;
	    goto haveDot;
	case '8':
	    dot = B8;
	    goto haveDot;
	case '9':
	    dot = B9;
	    goto haveDot;
	case 'a':
	case 'A':
	    dot = B10;
	    goto haveDot;
	case 'b':
	case 'B':
	    dot = B11;
	    goto haveDot;
	case 'c':
	case 'C':
	    dot = B12;
	    goto haveDot;
	case 'd':
	case 'D':
	    dot = B13;
	    goto haveDot;
	case 'e':
	case 'E':
	    dot = B14;
	    goto haveDot;
	case 'f':
	case 'F':
	    dot = B15;
	  haveDot:
	    if (started && !cell)
	      goto invalid;
	    if (cell & dot)
	      {
		compileError (nested, "dot specified more than once.");
		return 0;
	      }
	    cell |= dot;
	    break;
	  }
	case '0':		/*blank */
	  if (started)
	    goto invalid;
	  break;
	case '-':		/*got all dots for this cell */
	  if (!started)
	    {
	      compileError (nested, "missing cell specification.");
	      return 0;
	    }
	  cells->chars[cellCount++] = cell | B16;
	  cell = 0;
	  start = index + 1;
	  break;
	default:
	invalid:
	  compileError (nested, "invalid dot number %s.", showString
			(&character, 1));
	  return 0;
	}
    }
  if (index == start)
    {
      compileError (nested, "missing cell specification.");
      return 0;
    }
  cells->chars[cellCount++] = cell | B16;	/*last cell */
  cells->length = cellCount;
  return 1;
}

int
extParseDots (const char *inString, widechar * outString)
{
/* Parse external dot patterns */
  CharsString wideIn;
  CharsString result;
  int k;
  for (k = 0; inString[k] && k < MAXSTRING; k++)
    wideIn.chars[k] = inString[k];
  wideIn.chars[k] = 0;
  wideIn.length = k;
  parseDots (NULL, &result, &wideIn);
  if (errorCount)
    {
      errorCount = 0;
      return 0;
    }
  for (k = 0; k < result.length; k++)
    outString[k] = result.chars[k];
  outString[k] = 0;
  return 1;
}

static int
getCharacters (FileInfo * nested, CharsString * characters)
{
/*Get ruleChars string */
  CharsString token;
  if (getToken (nested, &token, "characters"))
    if (parseChars (nested, characters, &token))
      return 1;
  return 0;
}

static int
getRuleCharsText (FileInfo * nested, CharsString * ruleChars)
{

  CharsString token;
  if (getToken (nested, &token, "Characters operand"))
    if (parseChars (nested, ruleChars, &token))
      return 1;
  return 0;
}

static int
getRuleDotsText (FileInfo * nested, CharsString * ruleDots)
{
  CharsString token;
  if (getToken (nested, &token, "characters"))
    if (parseChars (nested, ruleDots, &token))
      return 1;
  return 0;
}

static int
getRuleDotsPattern (FileInfo * nested, CharsString * ruleDots)
{
/*Interpret the dets operand */
  CharsString token;
  if (getToken (nested, &token, "Dots operand"))
    {
      if (token.length == 1 && token.chars[0] == '=')
	{
	  ruleDots->length = 0;
	  return 1;
	}
      if (parseDots (nested, ruleDots, &token))
	return 1;
    }
  return 0;
}

static int
getCharacterClass (FileInfo * nested, const struct CharacterClass **class)
{
  CharsString token;
  if (getToken (nested, &token, "character class name"))
    {
      if ((*class = findCharacterClass (&token)))
	return 1;
      compileError (nested, "character class not defined.");
    }
  return 0;
}

static int compileFile (const char *fileName);

static int
includeFile (FileInfo * nested, CharsString * includedFile)
{
/*Implement include opcode*/
  int k;
  char includeThis[MAXSTRING];
  for (k = 0; k < includedFile->length; k++)
    includeThis[k] = (char) includedFile->chars[k];
  includeThis[k] = 0;
  return compileFile (includeThis);
}

struct RuleName
{
  struct RuleName *next;
  TranslationTableOffset ruleOffset;
  widechar length;
  widechar name[1];
};
static struct RuleName *ruleNames = NULL;
static TranslationTableOffset
findRuleName (const CharsString * name)
{
  const struct RuleName *nameRule = ruleNames;
  while (nameRule)
    {
      if ((name->length == nameRule->length) &&
	  (memcmp (&name->chars[0], nameRule->name, CHARSIZE *
		   name->length) == 0))
	return nameRule->ruleOffset;
      nameRule = nameRule->next;
    }
  return 0;
}

static int
addRuleName (FileInfo * nested, CharsString * name)
{
  int k;
  struct RuleName *nameRule;
  if (!(nameRule = malloc (sizeof (*nameRule) + CHARSIZE *
			   (name->length - 1))))
    {
      compileError (nested, "not enough memory");
      return 0;
    }
  memset (nameRule, 0, sizeof (*nameRule));
  for (k = 0; k < name->length; k++)
    {
      TranslationTableCharacter *ch = definedCharOrDots
	(nested, name->chars[k],
	 0);
      if (!(ch->attributes & CTC_Letter))
	{
	  compileError (nested, "a name may contain only letters");
	  return 0;
	}
      nameRule->name[k] = name->chars[k];
    }
  nameRule->length = name->length;
  nameRule->ruleOffset = newRuleOffset;
  nameRule->next = ruleNames;
  ruleNames = nameRule;
  return 1;
}

static void
deallocateRuleNames (void)
{
  while (ruleNames)
    {
      struct RuleName *nameRule = ruleNames;
      ruleNames = ruleNames->next;
      if (nameRule)
	free (nameRule);
    }
}

static int
compileSwapDots (FileInfo * nested, CharsString * source, CharsString * dest)
{
  int k = 0;
  int kk;
  CharsString dotsSource;
  CharsString dotsDest;
  dest->length = 0;
  dotsSource.length = 0;
  while (k <= source->length)
    {
      if (source->chars[k] != ',' && k != source->length)
	dotsSource.chars[dotsSource.length++] = source->chars[k];
      else
	{
	  if (!parseDots (nested, &dotsDest, &dotsSource))
	    return 0;
	  dest->chars[dest->length++] = dotsDest.length + 1;
	  for (kk = 0; kk < dotsDest.length; kk++)
	    dest->chars[dest->length++] = dotsDest.chars[kk];
	  dotsSource.length = 0;
	}
      k++;
    }
  return 1;
}

static int
compileSwap (FileInfo * nested, TranslationTableOpcode opcode)
{
  CharsString ruleChars;
  CharsString ruleDots;
  CharsString name;
  CharsString matches;
  CharsString replacements;
  if (!getToken (nested, &name, "name operand"))
    return 0;
  if (!getToken (nested, &matches, "matches operand"))
    return 0;
  if (!getToken (nested, &replacements, "replacements operand"))
    return 0;
  if (opcode == CTO_SwapCc || opcode == CTO_SwapCd)
    {
      if (!parseChars (nested, &ruleChars, &matches))
	return 0;
    }
  else
    {
      if (!compileSwapDots (nested, &matches, &ruleChars))
	return 0;
    }
  if (opcode == CTO_SwapCc)
    {
      if (!parseChars (nested, &ruleDots, &replacements))
	return 0;
    }
  else
    {
      if (!compileSwapDots (nested, &replacements, &ruleDots))
	return 0;
    }
  if (!addRule (nested, opcode, &ruleChars, &ruleDots, 0, 0))
    return 0;
  if (!addRuleName (nested, &name))
    return 0;
  return 1;
}

static int
compilePassAttributes (FileInfo * nested, widechar * source,
		       TranslationTableCharacterAttributes * dest)
{
  int k = 1;
  int more = 1;
  if (source[0] != pass_attributes)
    return -1;
  *dest = 0;
  while (more)
    {
      switch (source[k])
	{
	case pass_any:
	  *dest = 0xffffffff;
	  break;
	case pass_digit:
	  *dest |= CTC_Digit;
	  break;
	case pass_litDigit:
	  *dest |= CTC_LitDigit;
	  break;
	case pass_letter:
	  *dest |= CTC_Letter;
	  break;
	case pass_math:
	  *dest |= CTC_Math;
	  break;
	case pass_punctuation:
	  *dest |= CTC_Punctuation;
	  break;
	case pass_sign:
	  *dest |= CTC_Sign;
	  break;
	case pass_space:
	  *dest |= CTC_Space;
	  break;
	case pass_uppercase:
	  *dest |= CTC_UpperCase;
	  break;
	case pass_lowercase:
	  *dest |= CTC_LowerCase;
	  break;
	case pass_class1:
	  *dest |= CTC_Class1;
	  break;
	case pass_class2:
	  *dest |= CTC_Class2;
	  break;
	case pass_class3:
	  *dest |= CTC_Class3;
	  break;
	case pass_class4:
	  *dest |= CTC_Class4;
	  break;
	default:
	  more = 0;
	  break;
	}
      if (more)
	k++;
    }
  if (!*dest)
    compileError (nested, "Missing attribute");
  return k;
}

static int
compilePassDots (FileInfo * nested, widechar * source, CharsString * dest)
{
  int k = 1;
  int kk = 0;
  CharsString sourceDots;
  if (*source != pass_dots)
    return -1;
  while (source[k] == '-' || (source[k] >= '0' && source[k] <= '9') ||
	 ((source[k] | 32) >= 'a' && (source[k] | 32) <= 'f'))
    sourceDots.chars[kk++] = source[k++];
  sourceDots.length = kk;
  if (!parseDots (nested, dest, &sourceDots))
    return -1;
  return k;
}

static int
compileContextChars (FileInfo * nested, widechar * source, CharsString * dest)
{
  int k = 1;
  int kk = 0;
  CharsString sourceChars;
  if (*source != pass_string)
    return -1;
  while (1)
    {
      if (!source[k])
	break;
      if (source[k] == '\"')
	{
	  if (source[k - 1] == '\\' && source[k - 2] != '\\')
	    kk--;
	  else
	    break;
	}
      sourceChars.chars[kk++] = source[k++];
    }
  sourceChars.chars[kk] = 0;
  sourceChars.length = kk;
  k++;
  if (!parseChars (nested, dest, &sourceChars))
    return -1;
  return k;
}

static int
getNumber (widechar * source, widechar * dest)
{
/*Convert a string of wide character digits to an integer*/
  int k = 0;
  *dest = 0;
  while (source[k] >= '0' && source[k] <= '9')
    *dest = 10 * *dest + (source[k++] - '0');
  return k;
}

static int
compilePassOpcode (FileInfo * nested, TranslationTableOpcode opcode)
{
/*Compile the operands of a pass opcode */
  CharsString ruleChars;
  CharsString ruleDots;
  TranslationTableCharacterAttributes after = 0;
  TranslationTableCharacterAttributes before = 0;
  CharsString test;
  CharsString action;
  widechar passSubOp;
  int returned;
  CharsString holdString;
  const struct CharacterClass *class;
  TranslationTableOffset ruleOffset;
  TranslationTableRule *rule;
  TranslationTableCharacterAttributes attributes = 0;
  widechar *passInstructions = ruleDots.chars;
  int passIC = 0;		/*Instruction counter */
  int k = 0;
  int kk;
  widechar holdNumber;
  if (!getToken (nested, &test, "Multipass opcode, test part"))
    return 0;
  if (!getToken (nested, &action, "multipass opcode, action part"))
    return 0;

/*Compile test part*/
  ruleChars.length = 0;
  while (k < test.length)
    switch ((passSubOp = test.chars[k]))
      {
      case pass_lookback:
	passInstructions[passIC++] = pass_lookback;
	k++;
	k += getNumber (&test.chars[k], &holdNumber);
	if (holdNumber == 0)
	  holdNumber = 1;
	passInstructions[passIC++] = holdNumber;
	break;
      case pass_not:
	passInstructions[passIC++] = pass_not;
	k++;
	break;
      case pass_first:
	passInstructions[passIC++] = pass_first;
	k++;
	break;
      case pass_last:
	passInstructions[passIC++] = pass_last;
	k++;
	break;
      case pass_search:
	passInstructions[passIC++] = pass_search;
	k++;
	break;
      case pass_string:
	if (opcode != CTO_Context && opcode != CTO_Correct)
	  {
	    compileError (nested,
			  "Character strings can only be used with the context and correct opcodes.");
	    return 0;
	  }
	passInstructions[passIC++] = pass_string;
	returned = compileContextChars (nested, &test.chars[k], &holdString);
	goto testDoCharsDots;
      case pass_dots:
	passInstructions[passIC++] = pass_dots;
	returned = compilePassDots (nested, &test.chars[k], &holdString);
      testDoCharsDots:
	if (returned == -1)
	  return 0;
	k += returned;
	passInstructions[passIC++] = holdString.length;
	for (kk = 0; kk < holdString.length; kk++)
	  passInstructions[passIC++] = holdString.chars[kk];
	break;
      case pass_startReplace:
	passInstructions[passIC++] = pass_startReplace;
	k++;
	break;
      case pass_endReplace:
	passInstructions[passIC++] = pass_endReplace;
	k++;
	break;
      case pass_variable:
	k++;
	k += getNumber (&test.chars[k], &holdNumber);
	switch (test.chars[k])
	  {
	  case pass_eq:
	    passInstructions[passIC++] = pass_eq;
	    goto doComp;
	  case pass_lt:
	    if (test.chars[k + 1] == pass_eq)
	      {
		k++;
		passInstructions[passIC++] = pass_lteq;
	      }
	    else
	      passInstructions[passIC++] = pass_lt;
	    goto doComp;
	  case pass_gt:
	    if (test.chars[k + 1] == pass_eq)
	      {
		k++;
		passInstructions[passIC++] = pass_gteq;
	      }
	    else
	      passInstructions[passIC++] = pass_gt;
	  doComp:
	    passInstructions[passIC++] = holdNumber;
	    k++;
	    k += getNumber (&test.chars[k], &passInstructions[passIC++]);
	    break;
	  default:
	    compileError (nested, "incorrect comparison operator");
	    return 0;
	  }
	break;
      case pass_attributes:
	k += compilePassAttributes (nested, &test.chars[k], &attributes);
      insertAttributes:
	passInstructions[passIC++] = pass_attributes;
	passInstructions[passIC++] = attributes << 16;
	passInstructions[passIC++] = attributes & 0xffff;
      getRange:
	if (test.chars[k] == pass_until)
	  {
	    k++;
	    passInstructions[passIC++] = 1;
	    passInstructions[passIC++] = 32;
	    break;
	  }
	k += getNumber (&test.chars[k], &holdNumber);
	if (holdNumber == 0)
	  {
	    holdNumber = passInstructions[passIC++] = 1;
	    passInstructions[passIC++] = 1;	/*This is not an error */
	    break;
	  }
	passInstructions[passIC++] = holdNumber;
	if (test.chars[k] != pass_hyphen)
	  {
	    passInstructions[passIC++] = holdNumber;
	    break;
	  }
	k++;
	k += getNumber (&test.chars[k], &holdNumber);
	if (holdNumber == 0)
	  {
	    compileError (nested, "invalid range");
	    return 0;
	  }
	passInstructions[passIC++] = holdNumber;
	break;
      case pass_groupstart:
      case pass_groupend:
	k++;
	holdString.length = 0;
	while (((definedCharOrDots (nested, test.chars[k],
				    0))->attributes & CTC_Letter))
	  holdString.chars[holdString.length++] = test.chars[k++];
	ruleOffset = findRuleName (&holdString);
	if (ruleOffset)
	  rule = (TranslationTableRule *) & table->ruleArea[ruleOffset];
	if (rule && rule->opcode == CTO_Grouping)
	  {
	    passInstructions[passIC++] = passSubOp;
	    passInstructions[passIC++] = ruleOffset >> 16;
	    passInstructions[passIC++] = ruleOffset & 0xffff;
	    break;
	  }
	else
	  {
	    compileError (nested, "%s is not a grouping name",
			  showString (&holdString.chars[0],
				      holdString.length));
	    return 0;
	  }
	break;
      case pass_swap:
	k++;
	holdString.length = 0;
	while (((definedCharOrDots (nested, test.chars[k],
				    0))->attributes & CTC_Letter))
	  holdString.chars[holdString.length++] = test.chars[k++];
	if ((class = findCharacterClass (&holdString)))
	  {
	    attributes = class->attribute;
	    goto insertAttributes;
	  }
	ruleOffset = findRuleName (&holdString);
	if (ruleOffset)
	  rule = (TranslationTableRule *) & table->ruleArea[ruleOffset];
	if (rule
	    && (rule->opcode == CTO_SwapCc || rule->opcode == CTO_SwapCd
		|| rule->opcode == CTO_SwapDd))
	  {
	    passInstructions[passIC++] = pass_swap;
	    passInstructions[passIC++] = ruleOffset >> 16;
	    passInstructions[passIC++] = ruleOffset & 0xffff;
	    goto getRange;
	  }
	compileError (nested, "%s is neither a class name nor a swap name.",
		      showString (&holdString.chars[0], holdString.length));
	return 0;
      default:
	compileError (nested, "incorrect operator '%c' in test part",
		      test.chars[k]);
	return 0;
      }
  passInstructions[passIC++] = pass_endTest;

/*Compile action part*/
  k = 0;
  while (k < action.length)
    switch ((passSubOp = action.chars[k]))
      {
      case pass_string:
	if (opcode != CTO_Correct)
	  {
	    compileError (nested,
			  "Character strings can only be used with the ccorrect opcode.");
	    return 0;
	  }
	passInstructions[passIC++] = pass_string;
	returned = compileContextChars (nested, &action.chars[k],
					&holdString);
	goto actionDoCharsDots;
      case pass_dots:
	if (opcode == CTO_Correct)
	  {
	    compileError (nested,
			  "Dot patterns cannot be used with the correct opcode.");
	    return 0;
	  }
	returned = compilePassDots (nested, &action.chars[k], &holdString);
	passInstructions[passIC++] = pass_dots;
      actionDoCharsDots:
	if (returned == -1)
	  return 0;
	k += returned;
	passInstructions[passIC++] = holdString.length;
	for (kk = 0; kk < holdString.length; kk++)
	  passInstructions[passIC++] = holdString.chars[kk];
	break;
      case pass_variable:
	k++;
	k += getNumber (&action.chars[k], &holdNumber);
	switch (action.chars[k])
	  {
	  case pass_eq:
	    passInstructions[passIC++] = pass_eq;
	    passInstructions[passIC++] = holdNumber;
	    k++;
	    k += getNumber (&action.chars[k], &passInstructions[passIC++]);
	    break;
	  case pass_plus:
	  case pass_hyphen:
	    passInstructions[passIC++] = action.chars[k];
	    passInstructions[passIC++] = holdNumber;
	    break;
	  default:
	    compileError (nested,
			  "incorrect variable operator in action part");
	    return 0;
	  }
	break;
      case pass_copy:
	passInstructions[passIC++] = pass_copy;
	k++;
	break;
      case pass_omit:
	passInstructions[passIC++] = pass_omit;
	k++;
	break;
      case pass_groupreplace:
      case pass_groupstart:
      case pass_groupend:
	k++;
	holdString.length = 0;
	while (((definedCharOrDots (nested, action.chars[k],
				    0))->attributes & CTC_Letter))
	  holdString.chars[holdString.length++] = action.chars[k++];
	ruleOffset = findRuleName (&holdString);
	if (ruleOffset)
	  rule = (TranslationTableRule *) & table->ruleArea[ruleOffset];
	if (rule && rule->opcode == CTO_Grouping)
	  {
	    passInstructions[passIC++] = passSubOp;
	    passInstructions[passIC++] = ruleOffset >> 16;
	    passInstructions[passIC++] = ruleOffset & 0xffff;
	    break;
	  }
	compileError (nested, "%s is not a grouping name",
		      showString (&holdString.chars[0], holdString.length));
	return 0;
      case pass_swap:
	k++;
	holdString.length = 0;
	while (action.chars[k] && ((definedCharOrDots (nested,
						       action.chars[k],
						       0))->
				   attributes & (CTC_Letter | CTC_Digit)))
	  holdString.chars[holdString.length++] = action.chars[k++];
	ruleOffset = findRuleName (&holdString);
	if (ruleOffset)
	  rule = (TranslationTableRule *) & table->ruleArea[ruleOffset];
	if (rule
	    && (rule->opcode == CTO_SwapCc || rule->opcode == CTO_SwapCd
		|| rule->opcode == CTO_SwapDd))
	  {
	    passInstructions[passIC++] = pass_swap;
	    passInstructions[passIC++] = ruleOffset >> 16;
	    passInstructions[passIC++] = ruleOffset & 0xffff;
	    break;
	  }
	compileError (nested, "%s is not a swap name.",
		      showString (&holdString.chars[0], holdString.length));
	return 0;
	break;
      default:
	compileError (nested, "incorrect operator in action part");
	return 0;
	break;
      }
  ruleDots.length = passIC;
  passIC = 0;
  while (passIC < ruleDots.length)
    {
      int start = 0;
      switch (passInstructions[passIC])
	{
	case pass_string:
	case pass_dots:
	case pass_attributes:
	case pass_swap:
	  start = 1;
	  break;
	case pass_groupstart:
	case pass_groupend:
/*	  if (!((passIC == 0
	       || passInstructions[passIC - 1] == pass_startReplace)
	      && (passInstructions[passIC + 3] == pass_endReplace
		  || passInstructions[passIC + 3] == pass_endTest)))
	    {
	      compileError (nested,
			    "grouping symbols must stand alone between replacement markers");
	      return 0;
	    }
*/
	  start = 1;
	  break;
	case pass_eq:
	case pass_lt:
	case pass_gt:
	case pass_lteq:
	case pass_gteq:
	  passIC += 3;
	  break;
	case pass_lookback:
	  passIC += 2;
	  break;
	case pass_not:
	case pass_startReplace:
	case pass_endReplace:
	  passIC++;
	  break;
	default:
	  compileError (nested,
			"Test part must contain characters/dots, attributes or class/swap.");
	  return 0;
	}
      if (start)
	break;
    }
  switch (passInstructions[passIC])
    {
    case pass_string:
    case pass_dots:
      for (k = 0; k < passInstructions[passIC + 1]; k++)
	ruleChars.chars[k] = passInstructions[passIC + 2 + k];
      ruleChars.length = k;
      break;
    case pass_attributes:
    case pass_groupstart:
    case pass_groupend:
    case pass_swap:
      after = passInstructions[passIC + 1];
      break;
    default:
      break;
    }
  if (!addRule (nested, opcode, &ruleChars, &ruleDots, after, before))
    return 0;
  return 1;
}

static int
compileBrailleIndicator (FileInfo * nested, char *ermsg,
			 TranslationTableOpcode opcode,
			 TranslationTableOffset * rule)
{
  CharsString token;
  CharsString cells;
  if (getToken (nested, &token, ermsg))
    if (parseDots (nested, &cells, &token))
      if (!addRule (nested, opcode, NULL, &cells, 0, 0))
	return 0;
  *rule = newRuleOffset;
  return 1;
}

static int
compileNumber (FileInfo * nested)
{
  CharsString token;
  widechar dest;
  if (!getToken (nested, &token, "number"))
    return 0;
  getNumber (&token.chars[0], &dest);
  if (!(dest > 0))
    {
      compileError (nested, "a nonzero positive number is required");
      return 0;
    }
  return dest;
}

static int
compileGrouping (FileInfo * nested)
{
  int k;
  CharsString name;
  CharsString groupChars;
  CharsString groupDots;
  CharsString dotsParsed;
  TranslationTableCharacter *charsDotsPtr;
  widechar endChar;
  widechar endDots;
  if (!getToken (nested, &name, "name operand"))
    return 0;
  if (!getRuleCharsText (nested, &groupChars))
    return 0;
  if (!getToken (nested, &groupDots, "dots operand"))
    return 0;
  for (k = 0; k < groupDots.length && groupDots.chars[k] != ','; k++);
  if (k == groupDots.length)
    {
      compileError (nested,
		    "Dots operand must consist of two cells separated by a comma");
      return 0;
    }
  groupDots.chars[k] = '-';
  if (!parseDots (nested, &dotsParsed, &groupDots))
    return 0;
  if (groupChars.length != 2 || dotsParsed.length != 2)
    {
      compileError (nested,
		    "two Unicode characters and two cells separated by a comma are needed.");
      return 0;
    }
  charsDotsPtr = addCharOrDots (nested, groupChars.chars[0], 0);
  charsDotsPtr->attributes |= CTC_Math;
  charsDotsPtr->uppercase = charsDotsPtr->realchar;
  charsDotsPtr->lowercase = charsDotsPtr->realchar;
  charsDotsPtr = addCharOrDots (nested, groupChars.chars[1], 0);
  charsDotsPtr->attributes |= CTC_Math;
  charsDotsPtr->uppercase = charsDotsPtr->realchar;
  charsDotsPtr->lowercase = charsDotsPtr->realchar;
  charsDotsPtr = addCharOrDots (nested, dotsParsed.chars[0], 1);
  charsDotsPtr->attributes |= CTC_Math;
  charsDotsPtr->uppercase = charsDotsPtr->realchar;
  charsDotsPtr->lowercase = charsDotsPtr->realchar;
  charsDotsPtr = addCharOrDots (nested, dotsParsed.chars[1], 1);
  charsDotsPtr->attributes |= CTC_Math;
  charsDotsPtr->uppercase = charsDotsPtr->realchar;
  charsDotsPtr->lowercase = charsDotsPtr->realchar;
  if (!addRule (nested, CTO_Grouping, &groupChars, &dotsParsed, 0, 0))
    return 0;
  if (!addRuleName (nested, &name))
    return 0;
  putCharAndDots (nested, groupChars.chars[0], dotsParsed.chars[0]);
  putCharAndDots (nested, groupChars.chars[1], dotsParsed.chars[1]);
  endChar = groupChars.chars[1];
  endDots = dotsParsed.chars[1];
  groupChars.length = dotsParsed.length = 1;
  if (!addRule (nested, CTO_Math, &groupChars, &dotsParsed, 0, 0))
    return 0;
  groupChars.chars[0] = endChar;
  dotsParsed.chars[0] = endDots;
  if (!addRule (nested, CTO_Math, &groupChars, &dotsParsed, 0, 0))
    return 0;
  return 1;
}

static int
compileUplow (FileInfo * nested)
{
  int k;
  TranslationTableCharacter *upperChar;
  TranslationTableCharacter *lowerChar;
  TranslationTableCharacter *upperCell = NULL;
  TranslationTableCharacter *lowerCell = NULL;
  CharsString ruleChars;
  CharsString ruleDots;
  CharsString upperDots;
  CharsString lowerDots;
  int haveLowerDots = 0;
  TranslationTableCharacterAttributes attr;
  if (!getRuleCharsText (nested, &ruleChars))
    return 0;
  if (!getToken (nested, &ruleDots, "dots operand"))
    return 0;
  for (k = 0; k < ruleDots.length && ruleDots.chars[k] != ','; k++);
  if (k == ruleDots.length)
    {
      if (!parseDots (nested, &upperDots, &ruleDots))
	return 0;
      lowerDots.length = upperDots.length;
      for (k = 0; k < upperDots.length; k++)
	lowerDots.chars[k] = upperDots.chars[k];
      lowerDots.chars[k] = 0;
    }
  else
    {
      haveLowerDots = ruleDots.length;
      ruleDots.length = k;
      if (!parseDots (nested, &upperDots, &ruleDots))
	return 0;
      ruleDots.length = 0;
      k++;
      for (; k < haveLowerDots; k++)
	ruleDots.chars[ruleDots.length++] = ruleDots.chars[k];
      if (!parseDots (nested, &lowerDots, &ruleDots))
	return 0;
    }
  if (ruleChars.length != 2 || upperDots.length < 1)
    {
      compileError (nested,
		    "Exactly two Unicode characters and at least one cell are required.");
      return 0;
    }
  if (haveLowerDots && lowerDots.length < 1)
    {
      compileError (nested, "at least one cell is required after the comma.");
      return 0;
    }
  upperChar = addCharOrDots (nested, ruleChars.chars[0], 0);
  upperChar->attributes |= CTC_Letter | CTC_UpperCase;
  upperChar->uppercase = ruleChars.chars[0];
  upperChar->lowercase = ruleChars.chars[1];
  lowerChar = addCharOrDots (nested, ruleChars.chars[1], 0);
  lowerChar->attributes |= CTC_Letter | CTC_LowerCase;
  lowerChar->uppercase = ruleChars.chars[0];
  lowerChar->lowercase = ruleChars.chars[1];
  for (k = 0; k < upperDots.length; k++)
    if (!compile_findCharOrDots (upperDots.chars[k], 1))
      {
	attr = CTC_Letter | CTC_UpperCase;
	upperCell = addCharOrDots (nested, upperDots.chars[k], 1);
	if (upperDots.length != 1)
	  attr = CTC_Space;
	upperCell->attributes |= attr;
	upperCell->uppercase = upperCell->realchar;
      }
  if (haveLowerDots)
    {
      for (k = 0; k < lowerDots.length; k++)
	if (!compile_findCharOrDots (lowerDots.chars[k], 1))
	  {
	    attr = CTC_Letter | CTC_LowerCase;
	    lowerCell = addCharOrDots (nested, lowerDots.chars[k], 1);
	    if (lowerDots.length != 1)
	      attr = CTC_Space;
	    lowerCell->attributes |= attr;
	    lowerCell->lowercase = lowerCell->realchar;
	  }
    }
  else if (upperCell != NULL && upperDots.length == 1)
    upperCell->attributes |= CTC_LowerCase;
  if (lowerDots.length == 1)
    putCharAndDots (nested, ruleChars.chars[1], lowerDots.chars[0]);
  if (upperCell != NULL)
    upperCell->lowercase = lowerDots.chars[0];
  if (lowerCell != NULL)
    lowerCell->uppercase = upperDots.chars[0];
  if (upperDots.length == 1)
    putCharAndDots (nested, ruleChars.chars[0], upperDots.chars[0]);
  ruleChars.length = 1;
  ruleChars.chars[2] = ruleChars.chars[0];
  ruleChars.chars[0] = ruleChars.chars[1];
  if (!addRule (nested, CTO_LowerCase, &ruleChars, &lowerDots, 0, 0))
    return 0;
  ruleChars.chars[0] = ruleChars.chars[2];
  if (!addRule (nested, CTO_UpperCase, &ruleChars, &upperDots, 0, 0))
    return 0;
  return 1;
}

/*Functions for compiling hyphenation tables*/

typedef struct			/*hyphenation dictionary: finite state machine */
{
  int numStates;
  HyphenationState *states;
} HyphenDict;

#define DEFAULTSTATE 0xffff
#define HYPHENHASHSIZE 8191

typedef struct
{
  void *next;
  CharsString *key;
  int val;
} HyphenHashEntry;

typedef struct
{
  HyphenHashEntry *entries[HYPHENHASHSIZE];
} HyphenHashTab;

/* a hash function from ASU - adapted from Gtk+ */
static unsigned int
hyphenStringHash (const CharsString * s)
{
  int k;
  unsigned int h = 0, g;
  for (k = 0; k < s->length; k++)
    {
      h = (h << 4) + s->chars[k];
      if ((g = h & 0xf0000000))
	{
	  h = h ^ (g >> 24);
	  h = h ^ g;
	}
    }
  return h;
}

static HyphenHashTab *
hyphenHashNew (void)
{
  HyphenHashTab *hashTab;
  hashTab = malloc (sizeof (HyphenHashTab));
  memset (hashTab, 0, sizeof (HyphenHashTab));
  return hashTab;
}

static void
hyphenHashFree (HyphenHashTab * hashTab)
{
  int i;
  HyphenHashEntry *e, *next;
  for (i = 0; i < HYPHENHASHSIZE; i++)
    for (e = hashTab->entries[i]; e; e = next)
      {
	next = e->next;
	free (e->key);
	free (e);
      }
  free (hashTab);
}

/* assumes that key is not already present! */
static void
hyphenHashInsert (HyphenHashTab * hashTab, const CharsString * key, int val)
{
  int i, j;
  HyphenHashEntry *e;
  i = hyphenStringHash (key) % HYPHENHASHSIZE;
  e = malloc (sizeof (HyphenHashEntry));
  e->next = hashTab->entries[i];
  e->key = malloc ((key->length + 1) * CHARSIZE);
  e->key->length = key->length;
  for (j = 0; j < key->length; j++)
    e->key->chars[j] = key->chars[j];
  e->val = val;
  hashTab->entries[i] = e;
}

/* return val if found, otherwise DEFAULTSTATE */
static int
hyphenHashLookup (HyphenHashTab * hashTab, const CharsString * key)
{
  int i, j;
  HyphenHashEntry *e;
  if (key->length == 0)
    return 0;
  i = hyphenStringHash (key) % HYPHENHASHSIZE;
  for (e = hashTab->entries[i]; e; e = e->next)
    {
      if (key->length != e->key->length)
	continue;
      for (j = 0; j < key->length; j++)
	if (key->chars[j] != e->key->chars[j])
	  break;
      if (j == key->length)
	return e->val;
    }
  return DEFAULTSTATE;
}

static int
hyphenGetNewState (HyphenDict * dict, HyphenHashTab * hashTab, const
		   CharsString * string)
{
  hyphenHashInsert (hashTab, string, dict->numStates);
  /* predicate is true if dict->numStates is a power of two */
  if (!(dict->numStates & (dict->numStates - 1)))
    dict->states = realloc (dict->states,
			    (dict->numStates << 1) *
			    sizeof (HyphenationState));
  dict->states[dict->numStates].hyphenPattern = 0;
  dict->states[dict->numStates].fallbackState = DEFAULTSTATE;
  dict->states[dict->numStates].numTrans = 0;
  dict->states[dict->numStates].trans.pointer = NULL;
  return dict->numStates++;
}

/* add a transition from state1 to state2 through ch - assumes that the
   transition does not already exist */
static void
hyphenAddTrans (HyphenDict * dict, int state1, int state2, widechar ch)
{
  int numTrans;
  numTrans = dict->states[state1].numTrans;
  if (numTrans == 0)
    dict->states[state1].trans.pointer = malloc (sizeof (HyphenationTrans));
  else if (!(numTrans & (numTrans - 1)))
    dict->states[state1].trans.pointer = realloc
      (dict->states[state1].trans.pointer,
       (numTrans << 1) * sizeof (HyphenationTrans));
  dict->states[state1].trans.pointer[numTrans].ch = ch;
  dict->states[state1].trans.pointer[numTrans].newState = state2;
  dict->states[state1].numTrans++;
}

static int
compileHyphenation (FileInfo * nested, CharsString * encoding)
{
  CharsString hyph;
  HyphenationTrans *holdPointer;
  HyphenHashTab *hashTab;
  CharsString word;
  char pattern[MAXSTRING];
  unsigned int stateNum = 0, lastState = 0;
  int i, j, k = encoding->length;
  widechar ch;
  int found;
  HyphenHashEntry *e;
  HyphenDict dict;
  TranslationTableOffset holdOffset;
  /*Set aside enough space for hyphenation states and transitions in 
   * translation table. Must be done before anything else*/
  reserveSpaceInTable (nested, 250000);
  hashTab = hyphenHashNew ();
  dict.numStates = 1;
  dict.states = malloc (sizeof (HyphenationState));
  dict.states[0].hyphenPattern = 0;
  dict.states[0].fallbackState = DEFAULTSTATE;
  dict.states[0].numTrans = 0;
  dict.states[0].trans.pointer = NULL;
  do
    {
      if (!getToken (nested, &hyph, NULL))
	continue;
      if (hyph.length == 0 || hyph.chars[0] == '#' || hyph.chars[0] ==
	  '%' || hyph.chars[0] == '<')
	continue;		/*comment */
      for (i = 0; i < hyph.length; i++)
	definedCharOrDots (nested, hyph.chars[i], 0);
      j = 0;
      pattern[j] = '0';
      for (i = 0; i < hyph.length; i++)
	{
	  if (hyph.chars[i] >= '0' && hyph.chars[i] <= '9')
	    pattern[j] = (char) hyph.chars[i];
	  else
	    {
	      word.chars[j] = hyph.chars[i];
	      pattern[++j] = '0';
	    }
	}
      word.chars[j] = 0;
      word.length = j;
      pattern[j + 1] = 0;
      for (i = 0; pattern[i] == '0'; i++);
      found = hyphenHashLookup (hashTab, &word);
      if (found != DEFAULTSTATE)
	stateNum = found;
      else
	stateNum = hyphenGetNewState (&dict, hashTab, &word);
      k = j + 2 - i;
      if (k > 0)
	{
	  allocateSpaceInTable (nested, &dict.states[stateNum].hyphenPattern,
				k);
	  memcpy (&table->ruleArea[dict.states[stateNum].hyphenPattern],
		  &pattern[i], k);
	}
      /* now, put in the prefix transitions */
      while (found == DEFAULTSTATE)
	{
	  lastState = stateNum;
	  ch = word.chars[word.length-- - 1];
	  found = hyphenHashLookup (hashTab, &word);
	  if (found != DEFAULTSTATE)
	    stateNum = found;
	  else
	    stateNum = hyphenGetNewState (&dict, hashTab, &word);
	  hyphenAddTrans (&dict, stateNum, lastState, ch);
	}
    }
  while (getALine (nested));
  /* put in the fallback states */
  for (i = 0; i < HYPHENHASHSIZE; i++)
    {
      for (e = hashTab->entries[i]; e; e = e->next)
	{
	  for (j = 1; j <= e->key->length; j++)
	    {
	      word.length = 0;
	      for (k = j; k < e->key->length; k++)
		word.chars[word.length++] = e->key->chars[k];
	      stateNum = hyphenHashLookup (hashTab, &word);
	      if (stateNum != DEFAULTSTATE)
		break;
	    }
	  if (e->val)
	    dict.states[e->val].fallbackState = stateNum;
	}
    }
  hyphenHashFree (hashTab);
/*Transfer hyphenation information to table*/
  for (i = 0; i < dict.numStates; i++)
    {
      if (dict.states[i].numTrans == 0)
	dict.states[i].trans.offset = 0;
      else
	{
	  holdPointer = dict.states[i].trans.pointer;
	  allocateSpaceInTable (nested,
				&dict.states[i].trans.offset,
				dict.states[i].numTrans *
				sizeof (HyphenationTrans));
	  memcpy (&table->ruleArea[dict.states[i].trans.offset],
		  holdPointer,
		  dict.states[i].numTrans * sizeof (HyphenationTrans));
	  free (holdPointer);
	}
    }
  allocateSpaceInTable (nested,
			&holdOffset, dict.numStates *
			sizeof (HyphenationState));
  table->hyphenStatesArray = holdOffset;
  /* Prevents segmentajion fault if table is reallocated */
  memcpy (&table->ruleArea[table->hyphenStatesArray], &dict.states[0],
	  dict.numStates * sizeof (HyphenationState));
  free (dict.states);
  return 1;
}

static int
compileNoBreak (FileInfo * nested)
{
  int k;
  CharsString ruleDots;
  CharsString otherDots;
  CharsString dotsBefore;
  CharsString dotsAfter;
  int haveDotsAfter = 0;
  if (!getToken (nested, &ruleDots, "dots operand"))
    return 0;
  for (k = 0; k < ruleDots.length && ruleDots.chars[k] != ','; k++);
  if (k == ruleDots.length)
    {
      if (!parseDots (nested, &dotsBefore, &ruleDots))
	return 0;
      dotsAfter.length = dotsBefore.length;
      for (k = 0; k < dotsBefore.length; k++)
	dotsAfter.chars[k] = dotsBefore.chars[k];
      dotsAfter.chars[k] = 0;
    }
  else
    {
      haveDotsAfter = ruleDots.length;
      ruleDots.length = k;
      if (!parseDots (nested, &dotsBefore, &ruleDots))
	return 0;
      otherDots.length = 0;
      k++;
      for (; k < haveDotsAfter; k++)
	otherDots.chars[otherDots.length++] = ruleDots.chars[k];
      if (!parseDots (nested, &dotsAfter, &otherDots))
	return 0;
    }
  for (k = 0; k < dotsBefore.length; k++)
    dotsBefore.chars[k] = getCharFromDots (dotsBefore.chars[k]);
  for (k = 0; k < dotsAfter.length; k++)
    dotsAfter.chars[k] = getCharFromDots (dotsAfter.chars[k]);
  if (!addRule (nested, CTO_NoBreak, &dotsBefore, &dotsAfter, 0, 0))
    return 0;
  table->noBreak = newRuleOffset;
  return 1;
}

static int
compileRule (FileInfo * nested)
{
  int ok = 1;
  CharsString token;
  TranslationTableOpcode opcode;
  CharsString ruleChars;
  CharsString ruleDots;
  CharsString cells;
  CharsString scratchPad;
  TranslationTableCharacterAttributes after = 0;
  TranslationTableCharacterAttributes before = 0;
  int k;

  noback = nofor = 0;
doOpcode:
  if (!getToken (nested, &token, NULL))
    return 1;			/*blank line */
  if (token.chars[0] == '#' || token.chars[0] == '<')
    return 1;			/*comment */
  if (nested->lineNumber == 1 && token.chars[0] == 'I' && token.chars[1]
      == 'S' && token.chars[2] == 'O')
    {
      compileHyphenation (nested, &token);
      return 1;
    }
  opcode = getOpcode (nested, &token);
  switch (opcode)
    {				/*Carry out operations */
    case CTO_None:
      break;
    case CTO_IncludeFile:
      {
	CharsString includedFile;
	if (getToken (nested, &token, "include file name"))
	  if (parseChars (nested, &includedFile, &token))
	    if (!includeFile (nested, &includedFile))
	      ok = 0;
	break;
      }
    case CTO_Locale:
      break;
    case CTO_Undefined:
      ok = compileBrailleIndicator (nested,
				    "undefined character opcode",
				    CTO_Undefined, &table->undefined);
      break;
    case CTO_CapitalSign:
      ok = compileBrailleIndicator (nested,
				    "capital sign", CTO_CapitalRule,
				    &table->capitalSign);
      break;
    case CTO_BeginCapitalSign:
      ok = compileBrailleIndicator (nested,
				    "begin capital sign",
				    CTO_BeginCapitalRule,
				    &table->beginCapitalSign);
      break;
    case CTO_LenBegcaps:
      ok = table->lenBeginCaps = compileNumber (nested);
      break;
    case CTO_EndCapitalSign:
      ok = compileBrailleIndicator (nested,
				    "end capitals sign",
				    CTO_EndCapitalRule,
				    &table->endCapitalSign);
      break;
    case CTO_FirstWordCaps:
      ok = compileBrailleIndicator (nested,
				    "first word capital sign",
				    CTO_FirstWordCapsRule,
				    &table->firstWordCaps);
      break;
    case CTO_LastWordCapsBefore:
      ok = compileBrailleIndicator (nested,
				    "capital sign before last word",
				    CTO_LastWordCapsBeforeRule,
				    &table->lastWordCapsBefore);
      break;
    case CTO_LastWordCapsAfter:
      ok = compileBrailleIndicator (nested,
				    "capital sign after last word",
				    CTO_LastWordCapsAfterRule,
				    &table->lastWordCapsAfter);
      break;
    case CTO_LenCapsPhrase:
      ok = table->lenCapsPhrase = compileNumber (nested);
      break;
    case CTO_LetterSign:
      ok = compileBrailleIndicator (nested,
				    "letter sign", CTO_LetterRule,
				    &table->letterSign);
      break;
    case CTO_NoLetsignBefore:
      if (getRuleCharsText (nested, &ruleChars))
	{
	  if ((table->noLetsignBeforeCount + ruleChars.length) > LETSIGNSIZE)
	    {
	      compileError (nested, "More than %d characters", LETSIGNSIZE);
	      ok = 0;
	      break;
	    }
	  for (k = 0; k < ruleChars.length; k++)
	    table->noLetsignBefore[table->noLetsignBeforeCount++] =
	      ruleChars.chars[k];
	}
      break;
    case CTO_NoLetsign:
      if (getRuleCharsText (nested, &ruleChars))
	{
	  if ((table->noLetsignCount + ruleChars.length) > LETSIGNSIZE)
	    {
	      compileError (nested, "More than %d characters", LETSIGNSIZE);
	      ok = 0;
	      break;
	    }
	  for (k = 0; k < ruleChars.length; k++)
	    table->noLetsign[table->noLetsignCount++] = ruleChars.chars[k];
	}
      break;
    case CTO_NoLetsignAfter:
      if (getRuleCharsText (nested, &ruleChars))
	{
	  if ((table->noLetsignAfterCount + ruleChars.length) > LETSIGNSIZE)
	    {
	      compileError (nested, "More than %d characters", LETSIGNSIZE);
	      ok = 0;
	      break;
	    }
	  for (k = 0; k < ruleChars.length; k++)
	    table->noLetsignAfter[table->noLetsignAfterCount++] =
	      ruleChars.chars[k];
	}
      break;
    case CTO_NumberSign:
      ok = compileBrailleIndicator (nested,
				    "number sign", CTO_NumberRule,
				    &table->numberSign);
      break;
    case CTO_FirstWordItal:
      ok = compileBrailleIndicator (nested,
				    "first word italic",
				    CTO_FirstWordItalRule,
				    &table->firstWordItal);
      break;
    case CTO_ItalSign:
    case CTO_LastWordItalBefore:
      ok = compileBrailleIndicator
	(nested, "first word italic before",
	 CTO_LastWordItalBeforeRule, &table->lastWordItalBefore);
      break;
    case CTO_LastWordItalAfter:
      ok =
	compileBrailleIndicator (nested, "last word italic after",
				 CTO_LastWordItalAfterRule,
				 &table->lastWordItalAfter);
      break;
    case CTO_BegItal:
    case CTO_FirstLetterItal:
      ok = compileBrailleIndicator
	(nested, "first letter italic", CTO_FirstLetterItalRule,
	 &table->firstLetterItal);
      break;
    case CTO_EndItal:
    case CTO_LastLetterItal:
      ok = compileBrailleIndicator (nested,
				    "last letter italic",
				    CTO_LastLetterItalRule,
				    &table->lastLetterItal);
      break;
    case CTO_SingleLetterItal:
      ok = compileBrailleIndicator (nested,
				    "single letter italic",
				    CTO_SingleLetterItalRule,
				    &table->singleLetterItal);
      break;
    case CTO_ItalWord:
      ok = compileBrailleIndicator (nested,
				    "italic word",
				    CTO_ItalWordRule, &table->italWord);
      break;
    case CTO_LenItalPhrase:
      ok = table->lenItalPhrase = compileNumber (nested);
      break;
    case CTO_FirstWordBold:
      ok = compileBrailleIndicator
	(nested, "first word bold", CTO_FirstWordBoldRule,
	 &table->firstWordBold);
      break;
    case CTO_BoldSign:
    case CTO_LastWordBoldBefore:
      ok = compileBrailleIndicator (nested,
				    "last word bold before",
				    CTO_LastWordBoldBeforeRule,
				    &table->lastWordBoldBefore);
      break;
    case CTO_LastWordBoldAfter:
      ok = compileBrailleIndicator (nested,
				    "last word bold after",
				    CTO_LastWordBoldAfterRule,
				    &table->lastWordBoldAfter);
      break;
    case CTO_BegBold:
    case CTO_FirstLetterBold:
      ok = compileBrailleIndicator (nested,
				    "first  letter bold",
				    CTO_FirstLetterBoldRule,
				    &table->firstLetterBold);
      break;
    case CTO_EndBold:
    case CTO_LastLetterBold:
      ok =
	compileBrailleIndicator (nested, "last letter bold",
				 CTO_LastLetterBoldRule,
				 &table->lastLetterBold);
      break;
    case CTO_SingleLetterBold:
      ok = compileBrailleIndicator (nested,
				    "single  letter bold",
				    CTO_SingleLetterBoldRule,
				    &table->singleLetterBold);
      break;
    case CTO_BoldWord:
      ok = compileBrailleIndicator (nested,
				    "bold word",
				    CTO_BoldWordRule, &table->boldWord);
      break;
    case CTO_LenBoldPhrase:
      ok = table->lenBoldPhrase = compileNumber (nested);
      break;
    case CTO_FirstWordUnder:
      ok = compileBrailleIndicator (nested,
				    "first word  underline",
				    CTO_FirstWordUnderRule,
				    &table->firstWordUnder);
      break;
    case CTO_UnderSign:
    case CTO_LastWordUnderBefore:
      ok =
	compileBrailleIndicator (nested, "last word underline before",
				 CTO_LastWordUnderBeforeRule,
				 &table->lastWordUnderBefore);
      break;
    case CTO_LastWordUnderAfter:
      ok = compileBrailleIndicator (nested,
				    "last  word underline after",
				    CTO_LastWordUnderAfterRule,
				    &table->lastWordUnderAfter);
      break;
    case CTO_BegUnder:
    case CTO_FirstLetterUnder:
      ok = compileBrailleIndicator (nested,
				    "first letter underline",
				    CTO_FirstLetterUnderRule,
				    &table->firstLetterUnder);
      break;
    case CTO_EndUnder:
    case CTO_LastLetterUnder:
      ok = compileBrailleIndicator (nested, "last letter underline",
				    CTO_LastLetterUnderRule,
				    &table->lastLetterUnder);
      break;
    case CTO_SingleLetterUnder:
      ok = compileBrailleIndicator (nested,
				    "single letter underline",
				    CTO_SingleLetterUnderRule,
				    &table->singleLetterUnder);
      break;
    case CTO_UnderWord:
      ok = compileBrailleIndicator (nested,
				    "underlined word",
				    CTO_UnderWordRule, &table->underWord);
      break;
    case CTO_LenUnderPhrase:
      ok = table->lenUnderPhrase = compileNumber (nested);
      break;
    case CTO_BegComp:
      ok = compileBrailleIndicator (nested, "begin computer braille",
				    CTO_BegCompRule, &table->begComp);
      break;
    case CTO_EndComp:
      ok = compileBrailleIndicator (nested,
				    "end computer braslle",
				    CTO_EndCompRule, &table->endComp);
      break;

    case CTO_Syllable:
      table->syllables = 1;
    case CTO_Always:
    case CTO_NoCross:
    case CTO_LargeSign:
    case CTO_WholeWord:
    case CTO_PartWord:
    case CTO_JoinNum:
    case CTO_JoinableWord:
    case CTO_LowWord:
    case CTO_SuffixableWord:
    case CTO_PrefixableWord:
    case CTO_BegWord:
    case CTO_BegMidWord:
    case CTO_MidWord:
    case CTO_MidEndWord:
    case CTO_EndWord:
    case CTO_PrePunc:
    case CTO_PostPunc:
    case CTO_BegNum:
    case CTO_MidNum:
    case CTO_EndNum:
    case CTO_Repeated:
    case CTO_RepWord:
      if (getRuleCharsText (nested, &ruleChars))
	if (getRuleDotsPattern (nested, &ruleDots))
	  if (!addRule (nested, opcode, &ruleChars, &ruleDots, after, before))
	    ok = 0;
      break;

    case CTO_CompDots:
    case CTO_Comp6:
      if (!getRuleCharsText (nested, &ruleChars))
	return 0;
      if (ruleChars.length != 1 || ruleChars.chars[0] > 255)
	{
	  compileError (nested,
			"first operand must be 1 character and < 256");
	  return 0;
	}
      if (!getRuleDotsPattern (nested, &ruleDots))
	return 0;
      if (!addRule (nested, opcode, &ruleChars, &ruleDots, after, before))
	ok = 0;
      table->compdotsPattern[ruleChars.chars[0]] = newRuleOffset;
      break;

    case CTO_ExactDots:
      if (!getRuleCharsText (nested, &ruleChars))
	return 0;
      if (ruleChars.chars[0] != '@')
	{
	  compileError (nested, "The operand must begin with an at sign (@)");
	  return 0;
	}
      for (k = 1; k < ruleChars.length; k++)
	scratchPad.chars[k - 1] = ruleChars.chars[k];
      scratchPad.length = ruleChars.length - 1;
      if (!parseDots (nested, &ruleDots, &scratchPad))
	return 0;
      if (!addRule (nested, opcode, &ruleChars, &ruleDots, before, after))
	ok = 0;
      break;
    case CTO_CapsNoCont:
      ruleChars.length = 1;
      ruleChars.chars[0] = 'a';
      if (!addRule (nested, CTO_CapsNoContRule, &ruleChars, NULL,
		    after, before))
	ok = 0;
      table->capsNoCont = newRuleOffset;
      break;

    case CTO_Replace:
      if (getRuleCharsText (nested, &ruleChars))
	{
	  if (lastToken)
	    ruleDots.length = ruleDots.chars[0] = 0;
	  else
	    {
	      getRuleDotsText (nested, &ruleDots);
	      if (ruleDots.chars[0] == '#')
		ruleDots.length = ruleDots.chars[0] = 0;
	      else if (ruleDots.chars[0] == '\\' && ruleDots.chars[1] == '#')
		memcpy (&ruleDots.chars[0], &ruleDots.chars[1],
			ruleDots.length-- * CHARSIZE);
	    }
	}
      for (k = 0; k < ruleChars.length; k++)
	addCharOrDots (nested, ruleChars.chars[k], 0);
      for (k = 0; k < ruleDots.length; k++)
	addCharOrDots (nested, ruleDots.chars[k], 0);
      if (!addRule (nested, opcode, &ruleChars, &ruleDots, after, before))
	ok = 0;
      break;

    case CTO_Pass2:
      if (table->numPasses < 2)
	table->numPasses = 2;
      goto doPass;
    case CTO_Pass3:
      if (table->numPasses < 3)
	table->numPasses = 3;
      goto doPass;
    case CTO_Pass4:
      if (table->numPasses < 4)
	table->numPasses = 4;
    doPass:
    case CTO_Context:
      if (!compilePassOpcode (nested, opcode))
	ok = 0;
      break;
    case CTO_Correct:
      if (!compilePassOpcode (nested, opcode))
	ok = 0;
      table->corrections = 1;
      break;
    case CTO_Contraction:
    case CTO_NoCont:
    case CTO_CompBrl:
    case CTO_Literal:
      if (getRuleCharsText (nested, &ruleChars))
	if (!addRule (nested, opcode, &ruleChars, NULL, after, before))
	  ok = 0;
      break;

    case CTO_MultInd:
      {
	int lastToken;
	ruleChars.length = 0;
	if (getToken (nested, &token, "multiple braille indicators") &&
	    parseDots (nested, &cells, &token))
	  {
	    while ((lastToken = getToken (nested, &token, "multind opcodes")))
	      {
		opcode = getOpcode (nested, &token);
		if (opcode >= CTO_CapitalSign && opcode < CTO_MultInd)
		  ruleChars.chars[ruleChars.length++] = (widechar) opcode;
		else
		  {
		    compileError (nested, "Not a braille indicator opcode.");
		    ok = 0;
		  }
		if (lastToken == 2)
		  break;
	      }
	  }
	else
	  ok = 0;
	if (!addRule (nested, CTO_MultInd, &ruleChars, &cells, after, before))
	  ok = 0;
	break;
      }

    case CTO_Class:
      {
	CharsString characters;
	const struct CharacterClass *class;
	if (!characterClasses)
	  {
	    if (!allocateCharacterClasses ())
	      ok = 0;
	  }
	if (getToken (nested, &token, "character class name"))
	  {
	    if ((class = findCharacterClass (&token)))
	      {
		compileError (nested, "character class already defined.");
	      }
	    else if ((class = addCharacterClass (nested,
						 &token.chars[0],
						 token.length)))
	      {
		if (getCharacters (nested, &characters))
		  {
		    int index;
		    for (index = 0; index < characters.length; ++index)
		      {
			TranslationTableRule *defRule;
			TranslationTableCharacter *character =
			  definedCharOrDots
			  (nested, characters.chars[index], 0);
			character->attributes |= class->attribute;
			defRule = (TranslationTableRule *)
			  & table->ruleArea[character->definitionRule];
			if (defRule->dotslen == 1)
			  {
			    character = definedCharOrDots
			      (nested, defRule->charsdots[defRule->charslen],
			       1);
			    character->attributes |= class->attribute;
			  }
		      }
		  }
	      }
	  }
	break;
      }

      {
	TranslationTableCharacterAttributes *attributes;
	const struct CharacterClass *class;

    case CTO_After:
	attributes = &after;
	goto doClass;
    case CTO_Before:
	attributes = &before;
      doClass:

	if (!characterClasses)
	  {
	    if (!allocateCharacterClasses ())
	      ok = 0;
	  }
	if (getCharacterClass (nested, &class))
	  {
	    *attributes |= class->attribute;
	    goto doOpcode;
	  }
	break;
      }
    case CTO_NoBack:
      noback = 1;
      goto doOpcode;
    case CTO_NoFor:
      nofor = 1;
      goto doOpcode;
    case CTO_SwapCc:
    case CTO_SwapCd:
    case CTO_SwapDd:
      if (!compileSwap (nested, opcode))
	ok = 0;
      break;
    case CTO_Hyphen:
    case CTO_DecPoint:
      if (getRuleCharsText (nested, &ruleChars))
	if (getRuleDotsPattern (nested, &ruleDots))
	  {
	    if (ruleChars.length != 1 || ruleDots.length < 1)
	      {
		compileError (nested,
			      "One Unicode character and at least one cell are required.");
		ok = 0;
	      }
	    if (!addRule
		(nested, opcode, &ruleChars, &ruleDots, after, before))
	      ok = 0;
	  }
      break;

      {
	TranslationTableCharacterAttributes attributes = 0;
    case CTO_Space:
	attributes = CTC_Space;
	goto doChar;
    case CTO_Digit:
	attributes = CTC_Digit;
	goto doChar;
    case CTO_LitDigit:
	attributes = CTC_LitDigit;
	goto doChar;
    case CTO_Punctuation:
	attributes = CTC_Punctuation;
	goto doChar;
    case CTO_Math:
	attributes = CTC_Math;
	goto doChar;
    case CTO_Sign:
	attributes = CTC_Sign;
	goto doChar;
    case CTO_Letter:
	attributes = CTC_Letter;
	goto doChar;
    case CTO_UpperCase:
	attributes = CTC_UpperCase;
	goto doChar;
    case CTO_LowerCase:
	attributes = CTC_LowerCase;
      doChar:
	if ((attributes & (CTC_UpperCase | CTC_LowerCase)))
	  attributes |= CTC_Letter;
	if (getRuleCharsText (nested, &ruleChars))
	  if (getRuleDotsPattern (nested, &ruleDots))
	    {
	      if (ruleChars.length != 1 || ruleDots.length < 1)
		{
		  compileError (nested,
				"Exactly one Unicode character and at least one cell are required.");
		  ok = 0;
		}
	      {
		TranslationTableCharacter
		  * character = addCharOrDots (nested, ruleChars.chars[0], 0);
		TranslationTableCharacter *cell;
		character->attributes |= attributes;
		if (!(attributes & CTC_Letter))
		  character->uppercase = character->lowercase =
		    character->realchar;
		if (ruleDots.length == 1 && (cell = compile_findCharOrDots
					     (ruleDots.chars[0], 1)))
		  cell->attributes |= attributes;
		else
		  {
		    for (k = 0; k < ruleDots.length; k++)
		      if (!compile_findCharOrDots (ruleDots.chars[k], 1))
			{
			  TranslationTableCharacterAttributes attr =
			    attributes;
			  TranslationTableCharacter *cell =
			    addCharOrDots (nested, ruleDots.chars[k], 1);
			  if (ruleDots.length != 1)
			    attr = CTC_Space;
			  cell->attributes |= attr;
			  cell->uppercase = cell->lowercase = cell->realchar;
			}
		  }
		if (!addRule
		    (nested, opcode, &ruleChars, &ruleDots, after, before))
		  ok = 0;
		if (ruleDots.length == 1)
		  putCharAndDots (nested, ruleChars.chars[0],
				  ruleDots.chars[0]);
	      }
	    }
	break;
      }

    case CTO_NoBreak:
      ok = compileNoBreak (nested);
      break;
    case CTO_Grouping:
      ok = compileGrouping (nested);
      break;
    case CTO_UpLow:
      ok = compileUplow (nested);
      break;

    case CTO_Display:
      if (getRuleCharsText (nested, &ruleChars))
	if (getRuleDotsPattern (nested, &ruleDots))
	  {
	    if (ruleChars.length != 1 || ruleDots.length != 1)
	      {
		compileError (nested,
			      "Exactly one character and one cell are required.");
		ok = 0;
	      }
	    putCharAndDots (nested, ruleChars.chars[0], ruleDots.chars[0]);
	  }
      break;

    default:
      compileError (nested, "unimplemented opcode.");
      break;
    }
  return ok;
}

int EXPORT_CALL
lou_readCharFromFile (const char *fileName, int *mode)
{
/*Read a character from a file, whether big-endian, little-endian or 
* ASCII8*/
  int ch;
  static FileInfo nested;
  if (*mode == 1)
    {
      *mode = 0;
      nested.fileName = fileName;
      nested.encoding = noEncoding;
      nested.status = 0;
      nested.lineNumber = 0;
      if (!(nested.in = fopen (nested.fileName, "r")))
	{
	  lou_logPrint ("Cannot open file '%s'", nested.fileName);
	  *mode = 1;
	  return EOF;
	}
    }
  if (nested.in == NULL)
    {
      *mode = 1;
      return EOF;
    }
  ch = getAChar (&nested);
  if (ch == EOF)
    {
      fclose (nested.in);
      nested.in = NULL;
      *mode = 1;
    }
  return ch;
}

static int fileCount = 0;

static int
compileFile (const char *fileName)
{
/*Compile a table file */
  FileInfo nested;
  char completePath[MAXSTRING];
  fileCount++;
  strcpy (completePath, tablePath);
  strcat (completePath, fileName);
  nested.fileName = fileName;
  nested.encoding = noEncoding;
  nested.status = 0;
  nested.lineNumber = 0;
  if ((nested.in = fopen (completePath, "r")))
    {
      while (getALine (&nested))
	compileRule (&nested);
      fclose (nested.in);
    }
  else
    {
      if (fileCount > 1)
	lou_logPrint ("Cannot open table '%s'", nested.fileName);
      errorCount++;
      return 0;
    }
  return 1;
}

static int
compileString (const char *inString)
{
  int k;
  FileInfo nested;
  nested.fileName = inString;
  nested.encoding = noEncoding;
  nested.lineNumber = 1;
  nested.status = 0;
  nested.linepos = 0;
  for (k = 0; inString[k]; k++)
    nested.line[k] = inString[k];
  nested.line[k] = 0;
  return compileRule (&nested);
}

static int
makeDoubleRule (TranslationTableOpcode opcode, TranslationTableOffset
		* singleRule, TranslationTableOffset * doubleRule)
{
  CharsString dots;
  TranslationTableRule *rule;
  if (!*singleRule || *doubleRule)
    return 1;
  rule = (TranslationTableRule *) & table->ruleArea[*singleRule];
  memcpy (dots.chars, &rule->charsdots[0], rule->dotslen * CHARSIZE);
  memcpy (&dots.chars[rule->dotslen], &rule->charsdots[0], rule->dotslen *
	  CHARSIZE);
  dots.length = 2 * rule->dotslen;
  if (!addRule (NULL, opcode, NULL, &dots, 0, 0))
    return 0;
  *doubleRule = newRuleOffset;
  return 1;
}

static int
setDefaults (void)
{
  if (!table->lenBeginCaps)
    table->lenBeginCaps = 2;
  makeDoubleRule (CTO_FirstWordItal, &table->lastWordItalBefore,
		  &table->firstWordItal);
  if (!table->lenItalPhrase)
    table->lenItalPhrase = 4;
  makeDoubleRule (CTO_FirstWordBold, &table->lastWordBoldBefore,
		  &table->firstWordBold);
  if (!table->lenBoldPhrase)
    table->lenBoldPhrase = 4;
  makeDoubleRule (CTO_FirstWordUnder, &table->lastWordUnderBefore,
		  &table->firstWordUnder);
  if (!table->lenUnderPhrase)
    table->lenUnderPhrase = 4;
  if (table->numPasses == 0)
    table->numPasses = 1;
  return 1;
}

static char *
doLang2table (const char *tableList)
{
  static char newList[MAXSTRING];
  int k;
  char buffer[MAXSTRING];
  FILE *l2t;
  char *langCode;
  int langCodeLen;
  if (tableList == NULL || *tableList == 0)
    return NULL;
  strcpy (newList, tableList);
  for (k = strlen (newList) - 1; k >= 0 && newList[k] != '='; k--);
  if (newList[k] != '=')
    return newList;
  fileCount = 1;
  errorCount = 1;
  newList[k] = 0;
  strcpy (buffer, newList);
  langCode = &newList[k + 1];
  langCodeLen = strlen (langCode);
  strcat (buffer, "lang2table");
  l2t = fopen (buffer, "r");
  if (l2t == NULL)
    return NULL;
  while ((fgets (buffer, sizeof (buffer) - 2, l2t)))
    {
      int bufLen = strlen (buffer);
      char *codeInFile;
      int codeInFileLen;
      char *tableInFile;
      for (k = 0; buffer[k] < 32; k++);
      if (buffer[k] == '#' || buffer[k] < 32)
	continue;
      codeInFile = &buffer[k];
      codeInFileLen = k;
      while (buffer[k] > 32)
	k++;
      codeInFileLen = k - codeInFileLen;
      codeInFile[codeInFileLen] = 0;
      if (!
	  (codeInFileLen == langCodeLen
	   && strcasecmp (langCode, codeInFile) == 0))
	continue;
      while (buffer[k] < 32)
	k++;
      tableInFile = &buffer[k];
      while (buffer[k] > 32)
	k++;
      buffer[k] = 0;
      strcat (newList, tableInFile);
      fclose (l2t);
      fileCount = 0;
      errorCount = 0;
      return newList;
    }
  fclose (l2t);
  return NULL;
}

static void *
compileTranslationTable (const char *tl)
{
/*compile source tables into a table in memory */
  const char *tableList;
  int k;
  char mainTable[MAXSTRING];
  char subTable[MAXSTRING];
  int listLength;
  int currentListPos = 0;
  errorCount = 0;
  fileCount = 0;
  table = NULL;
  characterClasses = NULL;
  ruleNames = NULL;
  tableList = doLang2table (tl);
  if (tableList == NULL)
    return NULL;
  if (!opcodeLengths[0])
    {
      TranslationTableOpcode opcode;
      for (opcode = 0; opcode < CTO_None; opcode++)
	opcodeLengths[opcode] = strlen (opcodeNames[opcode]);
    }
  allocateHeader (NULL);
  /*Compile things that are necesary for the proper operation of 
     liblouis or liblouisxml */
  compileString ("space \\s 0");
  compileString ("noback sign \\x0000 0");
  compileString ("space \\x00a0 a unbreakable space");
  compileString ("space \\x001b 1b escape");
  compileString ("space \\xffff 123456789abcdef ENDSEGMENT");
  listLength = strlen (tableList);
  for (k = currentListPos; k < listLength; k++)
    if (tableList[k] == ',')
      break;
  if (k == listLength)
    {				/* Only one file */
      strcpy (tablePath, tableList);
      for (k = strlen (tablePath); k >= 0; k--)
	if (tablePath[k] == '\\' || tablePath[k] == '/')
	  break;
      strcpy (mainTable, &tablePath[k + 1]);
      tablePath[++k] = 0;
      if (!compileFile (mainTable))
	goto cleanup;
    }
  else
    {				/* Compile a list of files */
      currentListPos = k + 1;
      strncpy (tablePath, tableList, k);
      tablePath[k] = 0;
      for (k = strlen (tablePath); k >= 0; k--)
	if (tablePath[k] == '\\' || tablePath[k] == '/')
	  break;
      strcpy (mainTable, &tablePath[k + 1]);
      tablePath[++k] = 0;
      if (!compileFile (mainTable))
	goto cleanup;
      while (currentListPos < listLength)
	{
	  for (k = currentListPos; k < listLength; k++)
	    if (tableList[k] == ',')
	      break;
	  strncpy (subTable, &tableList[currentListPos], k - currentListPos);
	  subTable[k - currentListPos] = 0;
	  if (!compileFile (subTable))
	    goto cleanup;
	  currentListPos = k + 1;
	}
    }
/*Clean up after compiling files*/
cleanup:
  if (characterClasses)
    deallocateCharacterClasses ();
  if (ruleNames)
    deallocateRuleNames ();
  if (!errorCount)
    {
      setDefaults ();
      table->tableSize = tableSize;
      table->bytesUsed = tableUsed;
    }
  else
    {
      if (!(errorCount == 1 && fileCount == 1))
	lou_logPrint ("%d errors found.", errorCount);
      if (table)
	free (table);
      table = NULL;
    }
  return (void *) table;
}

typedef struct
{
  void *next;
  void *table;
  int tableListLength;
  char tableList[1];
} ChainEntry;
static ChainEntry *tableChain = NULL;
static ChainEntry *lastTrans = NULL;
static void *
getTable (const char *tableList)
{
/*Keep track of which tables have already been compiled */
  int tableListLen;
  ChainEntry *currentEntry = NULL;
  ChainEntry *lastEntry = NULL;
  void *newTable;
  if (tableList == NULL || *tableList == 0)
    return NULL;
  tableListLen = strlen (tableList);
  /*See if this is the last table used. */
  if (lastTrans != NULL)
    if (tableListLen == lastTrans->tableListLength && (memcmp
						       (&lastTrans->
							tableList
							[0],
							tableList,
							tableListLen)) == 0)
      return (table = lastTrans->table);
/*See if Table has already been compiled*/
  currentEntry = tableChain;
  while (currentEntry != NULL)
    {
      if (tableListLen == currentEntry->tableListLength && (memcmp
							    (&currentEntry->
							     tableList
							     [0],
							     tableList,
							     tableListLen))
	  == 0)
	{
	  lastTrans = currentEntry;
	  return (table = currentEntry->table);
	}
      lastEntry = currentEntry;
      currentEntry = currentEntry->next;
    }
  if ((newTable = compileTranslationTable (tableList)))
    {
      /*Add a new entry to the table chain. */
      int entrySize = sizeof (ChainEntry) + tableListLen;
      ChainEntry *newEntry = malloc (entrySize);
      if (tableChain == NULL)
	tableChain = newEntry;
      else
	lastEntry->next = newEntry;
      newEntry->next = NULL;
      newEntry->table = newTable;
      newEntry->tableListLength = tableListLen;
      memcpy (&newEntry->tableList[0], tableList, tableListLen);
      lastTrans = newEntry;
      return newEntry->table;
    }
  return NULL;
}

void *EXPORT_CALL
lou_getTable (const char *tableList)
{
/* Search paths for tables and keep track of compiled tables. */
  void *table;
  char *ch;
  char pathEnd[2];
  char trialPath[MAXSTRING];
  if (tableList == NULL || tableList[0] == 0)
    return NULL;
  pathEnd[0] = DIR_SEP;
  pathEnd[1] = 0;
  /* See if table is on environment path LOUIS_TABLEPATH */
  ch = getenv ("LOUIS_TABLEPATH");
  if (ch)
    {
      int pathLength;
      strcpy (trialPath, ch);
      /* Make sure path ends with \ or / etc. */
      pathLength = strlen (trialPath);
      if (trialPath[pathLength - 1] != DIR_SEP)
	strcat (trialPath, pathEnd);
      strcat (trialPath, tableList);
      table = getTable (trialPath);
    }
  else
    {
      /* See if table in current directory or on a path in 
       * the table name*/
      table = getTable (tableList);
      if (!table && errorCount == 1 && fileCount == 1)
	/* See if table on installed path. */
	{
#ifdef _WIN32
	  strcpy (trialPath, lou_getProgramPath ());
	  strcat (trialPath, "\\share\\liblouss\\tables\\");
#else
	  strcpy (trialPath, TABLESDIR);
	  strcat (trialPath, pathEnd);
#endif
	  strcat (trialPath, tableList);
	  table = getTable (trialPath);
	}
    }
  if (!table && errorCount == 1 && fileCount == 1)
    lou_logPrint ("Cannot find %s", trialPath);
  return table;
}

static unsigned char *destSpacing = NULL;
static int sizeDestSpacing = 0;
static unsigned short *typebuf = NULL;
static int sizeTypebuf = 0;
static widechar *passbuf1 = NULL;
static int sizePassbuf1 = 0;
static widechar *passbuf2 = NULL;
static int sizePassbuf2 = 0;
static int *srcMapping = NULL;
static int sizeSrcMapping = 0;
void *
liblouis_allocMem (AllocBuf buffer, int srcmax, int destmax)
{
  if (srcmax < 1024)
    srcmax = 1024;
  if (destmax < 1024)
    destmax = 1024;
  switch (buffer)
    {
    case alloc_typebuf:
      if (destmax > sizeTypebuf)
	{
	  if (typebuf != NULL)
	    free (typebuf);
	  typebuf = malloc ((destmax + 4) * sizeof (unsigned short));
	  sizeTypebuf = destmax;
	}
      return typebuf;
    case alloc_destSpacing:
      if (destmax > sizeDestSpacing)
	{
	  if (destSpacing != NULL)
	    free (destSpacing);
	  destSpacing = malloc (destmax + 4);
	  sizeDestSpacing = destmax;
	}
      return destSpacing;
    case alloc_passbuf1:
      if (destmax > sizePassbuf1)
	{
	  if (passbuf1 != NULL)
	    free (passbuf1);
	  passbuf1 = malloc ((destmax + 4) * CHARSIZE);
	  sizePassbuf1 = destmax;
	}
      return passbuf1;
    case alloc_passbuf2:
      if (destmax > sizePassbuf2)
	{
	  if (passbuf2 != NULL)
	    free (passbuf2);
	  passbuf2 = malloc ((destmax + 4) * CHARSIZE);
	  sizePassbuf2 = destmax;
	}
      return passbuf2;
    case alloc_srcMapping:
      {
	int mapSize;
	if (srcmax >= destmax)
	  mapSize = srcmax;
	else
	  mapSize = destmax;
	if (mapSize > sizeSrcMapping)
	  {
	    if (srcMapping != NULL)
	      free (srcMapping);
	    srcMapping = malloc ((mapSize + 4) * sizeof (int));
	    sizeSrcMapping = mapSize;
	  }
      }
      return srcMapping;
    default:
      return NULL;
    }
}

void EXPORT_CALL
lou_free (void)
{
  ChainEntry *currentEntry;
  ChainEntry *previousEntry;
  if (logFile != NULL)
    fclose (logFile);
  if (tableChain == NULL)
    return;
  currentEntry = tableChain;
  while (currentEntry)
    {
      free (currentEntry->table);
      previousEntry = currentEntry;
      currentEntry = currentEntry->next;
      free (previousEntry);
    }
  tableChain = NULL;
  lastTrans = NULL;
  if (typebuf != NULL)
    free (typebuf);
  typebuf = NULL;
  sizeTypebuf = 0;
  if (destSpacing != NULL)
    free (destSpacing);
  destSpacing = NULL;
  sizeDestSpacing = 0;
  if (passbuf1 != NULL)
    free (passbuf1);
  passbuf1 = NULL;
  sizePassbuf1 = 0;
  if (passbuf2 != NULL)
    free (passbuf2);
  passbuf2 = NULL;
  sizePassbuf2 = 0;
  if (srcMapping != NULL)
    free (srcMapping);
  srcMapping = NULL;
  sizeSrcMapping = 0;
  opcodeLengths[0] = 0;
}

char *EXPORT_CALL
lou_version ()
{
  static char *version = PACKAGE_VERSION;
  return version;
}

int EXPORT_CALL
lou_charSize (void)
{
  return CHARSIZE;
}

int EXPORT_CALL
lou_compileString (const char *tableList, const char *inString)
{
  if (!lou_getTable (tableList))
    return 0;
  return compileString (inString);
}
