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
#include <string.h>
#include <stdlib.h>
#include "liblouis.h"
#include "louis.h"
#define BUFSIZE 256

static char inputBuffer[BUFSIZE];
static const TranslationTableHeader *validTable = NULL;
static unsigned int mode;
static char table[BUFSIZE];

static int
getInput (void)
{
  int inputLength;
  inputBuffer[0] = 0;
  fgets (inputBuffer, sizeof (inputBuffer), stdin);
  inputLength = strlen (inputBuffer) - 1;
  if (inputLength < 0)		/*EOF on script */
    exit (0);
  inputBuffer[inputLength] = 0;
  return inputLength;
}

static void
paramLetters (void)
{
  printf ("Press one of the letters in parentheses, then enter.\n");
  printf ("(t)able, tr(a)nslated, (u)ntranslated, (r)un, (h)elp, (q)uit\n");
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
	      printf ("Enter the name of a table or a list: ");
	      getInput ();
	      strcpy (table, inputBuffer);
	      validTable = lou_getTable (table);
	      if (validTable != NULL && validTable->hyphenStatesArray == 0)
		{
		  printf ("No hyphenation table.\n");
		  validTable = NULL;
		}
	    }
	  while (validTable == NULL);
	  break;
	case 'a':
	  mode = 1;
	  break;
	case 'u':
	  mode = 0;
	  break;
	case 'r':
	  if (validTable == NULL)
	    {
	      printf ("You must enter a valid table name or list.\n");
	      inputBuffer[0] = 0;
	    }
	  break;
	case 'h':
	  printf ("Commands: action\n");
	  printf ("(t)able: Enter a table name or list\n");
	  printf ("(r)un: run the hyphenation test loop\n");
	  printf ("tr(a)nslated: translated input\n");
	  printf ("(u)ntranslated: untranslated input\n");
	  printf ("(h)elp: print this page\n");
	  printf ("(q)uit: leave the program\n");
	  printf ("\n");
	  paramLetters ();
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
main ()
{
  widechar inbuf[BUFSIZE];
  char hyphens[BUFSIZE];
  int inlen;
  int k;
  validTable = NULL;
  mode = 0;
  while (1)
    {
      getCommands ();
      printf ("Type something, press enter, and view the results.\n");
      printf ("A blank line returns to command entry.\n");
      while (1)
	{
	  inlen = getInput ();
	  if (inlen == 0)
	    break;
	  for (k = 0; k < inlen; k++)
	    inbuf[k] = inputBuffer[k];
	  if (!lou_hyphenate (table, inbuf, inlen, hyphens, mode))
	    {
	      printf ("Hyphenation error\n");
	      continue;
	    }
	  printf ("Hyphenation mask: %s\n", hyphens);
	  printf ("Hyphenated word: ");
	  for (k = 0; k < inlen; k++)
	    {
	      if (hyphens[k] == '1')
		printf ("-");
	      printf ("%c", inbuf[k]);
	    }
	  printf ("\n");
	}
    }
  lou_free ();
  return 0;
}
