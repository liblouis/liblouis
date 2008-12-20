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
#include "louis.h"
#define BUFSIZE 256

static char inputBuffer[BUFSIZE];

static int
showDots (widechar * dots, int length)
{
  char buffer[4 * BUFSIZE];
  int bufPos = 0;
  int dotsPos;
  for (dotsPos = 0; bufPos < sizeof (buffer) && dotsPos < length; 
dotsPos++)
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
case 'h':
break;
case 'q':
	  exit (0);
	default:
	  printf ("Bad choice.\n");
	  break;
	}
    }
  while (inputBuffer[0] != 'r');
  return 1;
}

int
main (int argc, char **argv)
{
  const ContractionTableHeader *table;
  if (argc != 2)
    {
      fprintf (stderr, "Usage: lou_debug tablename\n");
      exit (1);
    }
  if (!(table = lou_getTable (argv[1])))
{
lou_free ();
return 1;
}
    printf ("No errors found.\n");
  lou_free ();
getCommands ();
  return 0;
}
