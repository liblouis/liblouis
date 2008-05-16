/* liblouis Braille Translation and Back-Translation Library

   Based on BRLTTY, copyright (C) 1999-2006 by
   The BRLTTY Team

   Copyright (C) 2004, 2005, 2006
   ViewPlus Technologies, Inc. www.viewplus.com
   and
   JJB Software, Inc. www.jjb-software.com

   This file is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   In addition to the permissions in the GNU General Public License, the
   copyright holders give you unlimited permission to link the
   compiled version of this file into combinations with other programs,
   and to distribute those combinations without any restriction coming
   from the use of this file.  (The General Public License restrictions
   do apply in other respects; for example, they cover modification of
   the file, and distribution when not linked into a combine
   executable.)

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
#include <string.h>
#include <stdlib.h>
#include "liblouis.h"
#include "louis.h"
#define BUFSIZE 256

static int
showDots (widechar * dots)
{
  char buffer[4 * BUFSIZE];
  int bufPos = 0;
  int dotsPos;
  for (dotsPos = 0; bufPos < sizeof (buffer) && dots[dotsPos]; dotsPos++)
    {
      if ((dots[dotsPos] & B1))
	buffer[bufPos++] = '1';
      if ((dots[dotsPos] & B2))
	buffer[bufPos++] = '2';
      if ((dots[dotsPos] & B3))
	buffer[bufPos++] = '3';
      if ((dots[dotsPos] & B4))
	buffer[bufPos++] = '4';
      if ((dots[dotsPos] & B5))
	buffer[bufPos++] = '5';
      if ((dots[dotsPos] & B6))
	buffer[bufPos++] = '6';
      if ((dots[dotsPos] & B7))
	buffer[bufPos++] = '7';
      if ((dots[dotsPos] & B8))
	buffer[bufPos++] = '8';
      if ((dots[dotsPos] & B9))
	buffer[bufPos++] = '9';
      if ((dots[dotsPos] & B10))
	buffer[bufPos++] = 'A';
      if ((dots[dotsPos] & B11))
	buffer[bufPos++] = 'B';
      if ((dots[dotsPos] & B12))
	buffer[bufPos++] = 'C';
      if ((dots[dotsPos] & B13))
	buffer[bufPos++] = 'D';
      if ((dots[dotsPos] & B14))
	buffer[bufPos++] = 'E';
      if ((dots[dotsPos] & B15))
	buffer[bufPos++] = 'F';

      if ((dots[dotsPos] == B16))
	buffer[bufPos++] = '0';
      buffer[bufPos++] = ' ';
    }
  buffer[bufPos] = 0;
  return printf ("%s\n", buffer);
}

static char inputBuffer[BUFSIZE];
static void *validTable = NULL;
static int forwardOnly = 0;
static int backOnly = 0;
static int showPositions = 0;
static int minimalist = 0;
static int enteredCursorPos;
static unsigned int mode;
static char table[BUFSIZE];
static char emphasis[BUFSIZE];
static char spacing[BUFSIZE];
static char enteredEmphasis[BUFSIZE];
static char enteredSpacing[BUFSIZE];

static int
getInput (void)
{
  int namelen;
  fgets (inputBuffer, sizeof (inputBuffer), stdin);
  namelen = strlen (inputBuffer) - 1;
  inputBuffer[namelen] = 0;
  return namelen;
}

static int
getYN (void)
{
  printf ("? y/n: ");
  getInput ();
  if (inputBuffer[0] == 'y')
    return 1;
  return 0;
}

static void
paramLetters (void)
{
  printf ("Press one of the letters in parentheses, then enter.\n");
  printf
    ("(t)able, (r)un, (m)ode, (c)ursor, (e)mphasis, (s)pacing, (h)elp,\n");
  printf
    ("(q)uit, (f)orward-only, (b)ack-only, show-(p)ositions m(i)nimal.\n");
}

static int
getCommands (void)
{
  paramLetters ();
  do
    {
      printf ("Command: ");
      getInput ();
      switch (inputBuffer[0])
	{
	case 0:
	  break;
	case 't':
	  do
	    {
	      printf ("Enter the name of a table: ");
	      getInput ();
	      strcpy (table, inputBuffer);
	    }
	  while ((validTable = lou_getTable (table)) == NULL);
	  break;
	case 'r':
	  if (validTable == NULL)
	    {
	      printf ("You must enter a valid table name.\n");
	      inputBuffer[0] = 0;
	    }
	  break;
	case 'm':
	  printf ("Reset mode");
	  if (getYN ())
	    mode = 0;
	  printf ("No contractions");
	  mode |= getYN ();
	  printf ("Computer braille at cursor");
	  mode |= 2 * getYN ();
	  printf ("Dots input and output");
	  mode |= 4 * getYN ();
	  printf ("8-dot computer braille");
	  mode |= 8 * getYN ();
	  break;
	case 'c':
	  printf ("Enter a cursor position: ");
	  getInput ();
	  enteredCursorPos = atoi (inputBuffer);
	  if (enteredCursorPos < -1 || enteredCursorPos > BUFSIZE)
	    {
	      printf ("Cursor position must be from -1 to %d.\n", BUFSIZE);
	      enteredCursorPos = -1;
	    }
	  break;
	case 'e':
	  printf ("(Enter an x to cancel emphasis.)\n");
	  printf ("Enter an emphasis string: ");
	  getInput ();
	  strcpy (emphasis, inputBuffer);
	  break;
	case 's':
	  printf ("(Enter an x to cancel spacing.)\n");
	  printf ("Enter a spacing string: ");
	  getInput ();
	  strcpy (spacing, inputBuffer);
	  break;
	case 'h':
	  printf ("Commands: action\n");
	  printf ("(t)able: Enter a table name\n");
	  printf ("(r)un: run the translation/back-translation loop\n");
	  printf ("(m)ode: Enter a mode parameter\n");
	  printf ("(c)ursor: Enter a cursor position\n");
	  printf ("(e)mphasis: Enter an emphasis string\n");
	  printf ("(s)pacing: Enter a spacing string\n");
	  printf ("(h)elp: print this page\n");
	  printf ("(q)uit: leave the program\n");
	  printf ("(f)orward-only: do only forward translation\n");
	  printf ("(b)ack-only: do only back-translation\n");
	  printf ("show-(p)ositions: show input and output positions\n");
	  printf
	    ("m(i)nimal: test translator and back-translator with minimal parameters\n");
	  printf ("\n");
	  paramLetters ();
	  break;
	case 'q':
	  exit (0);
	case 'f':
	  printf ("Do only forward translation");
	  forwardOnly = getYN ();
	  break;
	case 'b':
	  printf ("Do only backward translation");
	  backOnly = getYN ();
	  break;
	case 'p':
	  printf ("Show input and output positions");
	  showPositions = getYN ();
	  break;
	case 'i':
	  printf
	    ("Test translation/back-translation loop with minimal parameters");
	  minimalist = getYN ();
	  break;
	default:
	  printf ("Bad choice.\n");
	  break;
	}
      if (forwardOnly && backOnly)
	printf
	  ("You cannot specify both forward-only and backward-only translation.\n");
    }
  while (inputBuffer[0] != 'r');
  return 1;
}

int
main ()
{
  widechar inbuf[BUFSIZE];
  widechar transbuf[BUFSIZE];
  widechar outbuf[BUFSIZE];
  int outputPos[BUFSIZE];
  int inputPos[BUFSIZE];
  int inlen;
  int translen;
  int outlen;
  int cursorPos;
  int realInlen = 0;
  int k;
  validTable = NULL;
  enteredCursorPos = -1;
  mode = 0;
  while (1)
    {
      getCommands ();
      printf ("Type something, press enter, and view the results.\n");
      printf ("A blank line returns to command entry.\n");
      if (minimalist)
	while (1)
	  {
	    translen = BUFSIZE;
	    outlen = BUFSIZE;
	    inlen = getInput ();
	    if (inlen == 0)
	      break;
	    for (realInlen = 0; realInlen < inlen; realInlen++)
	      inbuf[realInlen] = inputBuffer[realInlen];
	    if (!lou_translateString (table, inbuf, &inlen, transbuf,
				      &translen, NULL, NULL, 0))
	      break;
	    transbuf[translen] = 0;
	    printf ("Translation:\n");
	    for (k = 0; k < translen; k++)
	      printf ("%c", transbuf[k]);
	    printf ("\n");
	    lou_backTranslateString (table, transbuf, &translen,
				     outbuf, &outlen, NULL, NULL, 0);
	    printf ("Back-translation:\n");
	    for (k = 0; k < outlen; k++)
	      printf ("%c", outbuf[k]);
	    printf ("\n");
	    if (outlen == realInlen)
	      {
		for (k = 0; k < realInlen; k++)
		  if (inbuf[k] != outbuf[k])
		    break;
		if (k == realInlen)
		  printf ("Perfect roundtrip!\n");
	      }
	  }
      else
	while (1)
	  {
	    strcpy (emphasis, enteredEmphasis);
	    strcpy (spacing, enteredSpacing);
	    cursorPos = enteredCursorPos;
	    inlen = getInput ();
	    if (inlen == 0)
	      break;
	    outlen = BUFSIZE;
	    if (backOnly)
	      {
		for (translen = 0; translen < inlen; translen++)
		  transbuf[translen] = inputBuffer[translen];
	      }
	    else
	      {
		translen = BUFSIZE;
		for (realInlen = 0; realInlen < inlen; realInlen++)
		  inbuf[realInlen] = inputBuffer[realInlen];
		if (!lou_translate (table, inbuf, &inlen, transbuf,
				    &translen, emphasis, spacing,
				    &outputPos[0], &inputPos[0], &cursorPos,
				    mode))
		  break;
		transbuf[translen] = 0;
		if (mode & dotsIO)
		  {
		    printf ("Translation dot patterns:\n");
		    showDots (transbuf);
		  }
		else
		  {
		    printf ("Translation:\n");
		    for (k = 0; k < translen; k++)
		      printf ("%c", transbuf[k]);
		    printf ("\n");
		  }
	      }
	    if (cursorPos != -1)
	      printf ("Cursor position: %d\n", cursorPos);
	    if (enteredEmphasis[0])
	      printf ("Returned emphasis: %s\n", emphasis);
	    if (enteredSpacing[0])
	      printf ("Returned spacing: %s\n", spacing);
	    if (showPositions)
	      {
		printf ("Output positions:\n");
		for (k = 0; k < inlen; k++)
		  printf ("%d ", outputPos[k]);
		printf ("\n");
		printf ("Input positions:\n");
		for (k = 0; k < translen; k++)
		  printf ("%d ", inputPos[k]);
		printf ("\n");
	      }
	    if (!forwardOnly)
	      {
		if (!lou_backTranslate (table, transbuf, &translen,
					outbuf, &outlen,
					emphasis, spacing, &outputPos[0],
					&inputPos[0], &cursorPos, mode))
		  break;
		printf ("Back-translation:\n");
		for (k = 0; k < outlen; k++)
		  printf ("%c", outbuf[k]);
		printf ("\n");
		if (cursorPos != -1)
		  printf ("Cursor position: %d\n", cursorPos);
		if (enteredEmphasis[0])
		  printf ("Returned emphasis: %s\n", emphasis);
		if (enteredSpacing[0])
		  printf ("Returned spacing: %s\n", spacing);
		if (showPositions)
		  {
		    printf ("Output positions:\n");
		    for (k = 0; k < inlen; k++)
		      printf ("%d ", outputPos[k]);
		    printf ("\n");
		    printf ("Input positions:\n");
		    for (k = 0; k < translen; k++)
		      printf ("%d ", inputPos[k]);
		    printf ("\n");
		  }
	      }
	    if (!(forwardOnly || backOnly))
	      {
		if (outlen == realInlen)
		  {
		    for (k = 0; k < realInlen; k++)
		      if (inbuf[k] != outbuf[k])
			break;
		    if (k == realInlen)
		      printf ("Perfect roundtrip!\n");
		  }
	      }
	  }
    }
  lou_free ();
  return 0;
}
