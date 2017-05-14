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
 * @brief Common utility functions
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include "internal.h"
#include "config.h"

/* Contributed by Michel Such <michel.such@free.fr> */
#ifdef _WIN32

/* Adapted from BRLTTY code (see sys_progs_wihdows.h) */

#include <shlobj.h>

static void *
reallocWrapper (void *address, size_t size)
{
  if (!(address = realloc (address, size)) && size)
    _lou_outOfMemory ();
  return address;
}

static char *
strdupWrapper (const char *string)
{
  char *address = strdup (string);
  if (!address)
    _lou_outOfMemory ();
  return address;
}

char *EXPORT_CALL
lou_getProgramPath ()
{
  char *path = NULL;
  HMODULE handle;

  if ((handle = GetModuleHandle (NULL)))
    {
      DWORD size = 0X80;
      char *buffer = NULL;

      while (1)
	{
	  buffer = reallocWrapper (buffer, size <<= 1);

	  {
		// As the "UNICODE" Windows define may have been set at compilation,
		// This call must be specifically GetModuleFilenameA as further code expects it to be single byte chars.
		DWORD length = GetModuleFileNameA (handle, buffer, size);

	    if (!length)
	      {
		printf ("GetModuleFileName\n");
		exit (3);
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
#endif
/* End of MS contribution */

int EXPORT_CALL
_lou_stringHash (const widechar * c)
{
  /*hash function for strings */
  unsigned long int makeHash = (((unsigned long int) c[0] << 8) +
				(unsigned long int) c[1]) % HASHNUM;
  return (int) makeHash;
}

int EXPORT_CALL
_lou_charHash (widechar c)
{
  unsigned long int makeHash = (unsigned long int) c % HASHNUM;
  return (int) makeHash;
}

char *EXPORT_CALL
_lou_showString (widechar const *chars, int length)
{
  /*Translate a string of characters to the encoding used in character 
  * operands */
  int charPos;
  int bufPos = 0;
  static char scratchBuf[MAXSTRING];
  scratchBuf[bufPos++] = '\'';
  for (charPos = 0; charPos < length; charPos++)
    {
      if (chars[charPos] >= 32 && chars[charPos] < 127)
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

char *EXPORT_CALL
_lou_showDots (widechar const *dots, int length)
{
  /* Translate a sequence of dots to the encoding used in dots operands. 
  */
  int bufPos = 0;
  int dotsPos;
  static char scratchBuf[MAXSTRING];
  for (dotsPos = 0; bufPos < sizeof (scratchBuf) && dotsPos < length;
       dotsPos++)
    {
      if (dots[dotsPos] & B1)
	scratchBuf[bufPos++] = '1';
      if (dots[dotsPos] & B2)
	scratchBuf[bufPos++] = '2';
      if (dots[dotsPos] & B3)
	scratchBuf[bufPos++] = '3';
      if (dots[dotsPos] & B4)
	scratchBuf[bufPos++] = '4';
      if (dots[dotsPos] & B5)
	scratchBuf[bufPos++] = '5';
      if (dots[dotsPos] & B6)
	scratchBuf[bufPos++] = '6';
      if (dots[dotsPos] & B7)
	scratchBuf[bufPos++] = '7';
      if (dots[dotsPos] & B8)
	scratchBuf[bufPos++] = '8';
      if (dots[dotsPos] & B9)
	scratchBuf[bufPos++] = '9';
      if (dots[dotsPos] & B10)
	scratchBuf[bufPos++] = 'A';
      if (dots[dotsPos] & B11)
	scratchBuf[bufPos++] = 'B';
      if (dots[dotsPos] & B12)
	scratchBuf[bufPos++] = 'C';
      if (dots[dotsPos] & B13)
	scratchBuf[bufPos++] = 'D';
      if (dots[dotsPos] & B14)
	scratchBuf[bufPos++] = 'E';
      if (dots[dotsPos] & B15)
	scratchBuf[bufPos++] = 'F';
      if (dots[dotsPos] == B16)
	scratchBuf[bufPos++] = '0';
      if (dotsPos != length - 1)
	scratchBuf[bufPos++] = '-';
    }
  scratchBuf[bufPos] = 0;
  return scratchBuf;
}

char *EXPORT_CALL
_lou_showAttributes (TranslationTableCharacterAttributes a)
{
  /* Show attributes using the letters used after the $ in multipass 
  * opcodes. */
  int bufPos = 0;
  static char scratchBuf[MAXSTRING];
  if (a & CTC_Space)
    scratchBuf[bufPos++] = 's';
  if (a & CTC_Letter)
    scratchBuf[bufPos++] = 'l';
  if (a & CTC_Digit)
    scratchBuf[bufPos++] = 'd';
  if (a & CTC_Punctuation)
    scratchBuf[bufPos++] = 'p';
  if (a & CTC_UpperCase)
    scratchBuf[bufPos++] = 'U';
  if (a & CTC_LowerCase)
    scratchBuf[bufPos++] = 'u';
  if (a & CTC_Math)
    scratchBuf[bufPos++] = 'm';
  if (a & CTC_Sign)
    scratchBuf[bufPos++] = 'S';
  if (a & CTC_LitDigit)
    scratchBuf[bufPos++] = 'D';
  if (a & CTC_Class1)
    scratchBuf[bufPos++] = 'w';
  if (a & CTC_Class2)
    scratchBuf[bufPos++] = 'x';
  if (a & CTC_Class3)
    scratchBuf[bufPos++] = 'y';
  if (a & CTC_Class4)
    scratchBuf[bufPos++] = 'z';
  scratchBuf[bufPos] = 0;
  return scratchBuf;
}

void EXPORT_CALL
_lou_outOfMemory ()
{
  _lou_logMessage(LOG_FATAL, "liblouis: Insufficient memory\n");
  exit (3);
}

#ifdef DEBUG
void EXPORT_CALL
_lou_debugHook ()
{
  char *hook = "debug hook";
  printf ("%s\n", hook);
}
#endif
