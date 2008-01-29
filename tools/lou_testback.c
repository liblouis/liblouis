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
#define BUFSIZE 256

int
main ()
{
  widechar inbuf[BUFSIZE];
  widechar transbuf[BUFSIZE];
  int inlen;
  int translen;
  char table[128];
  int ch = 0;
  int namelen = 0;
  int k;
  while (1)
    {
      namelen = 0;
      printf ("Enter the name of a table, or q to quit.\n");
      while ((ch = getchar ()) != '\n')
	table[namelen++] = ch;
      if (namelen == 1 && *table == 'q')
	break;
      table[namelen] = 0;
      printf
	("Type something, press enter, and see if the back-translation is coreect.\n");
      printf ("A blank line goes back to try another table.\n");
      while (1)
	{
	  inlen = 0;
	  translen = BUFSIZE;
	  while ((ch = getchar ()) != '\n')
	    inbuf[inlen++] = ch;
	  if (inlen == 0)
	    break;
	  inbuf[inlen] = 0;
	  if (!lou_backTranslateString (table, inbuf, &inlen,
					transbuf, &translen, NULL, NULL, 0))
	    break;
	  transbuf[translen] = 0;
	  printf ("Back translation:\n");
	  for (k = 0; k < translen; k++)
	    printf ("%c", transbuf[k]);
	  printf ("\n");
	}
    }
  lou_free ();
  return 0;
}
