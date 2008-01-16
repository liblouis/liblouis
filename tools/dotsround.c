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
  char inputbuf[BUFSIZE];
  widechar inbuf[BUFSIZE];
  widechar transbuf[BUFSIZE];
  widechar outbuf[BUFSIZE];
  int inlen;
  int translen;
  int outlen;
  char table[128];
  int namelen = 0;
  int realInlen;
  int k;
  while (1)
    {
      namelen = 0;
      printf ("Enter the name of a table, or q to quit.\n");
      fgets (table, sizeof (table), stdin);
      namelen = strlen (table) - 1;
      table[namelen] = 0;
      if (namelen == 1 && *table == 'q')
	break;
      printf
	("Type something, press enter, and see if the translation matches.\n");
      printf ("A blank line goes back to try another table.\n");
      while (1)
	{
	  inlen = 0;
	  translen = BUFSIZE;
	  outlen = BUFSIZE;
	  fgets (inputbuf, BUFSIZE - 2, stdin);
	  inlen = strlen (inputbuf) - 1;
	  inputbuf[inlen] = 0;
	  if (inlen == 0)
	    break;
	  for (realInlen = 0; realInlen < inlen; realInlen++)
	    inbuf[realInlen] = inputbuf[realInlen];
	  if (!lou_translateString (table, inbuf, &inlen, transbuf,
				    &translen, NULL, NULL, dotsIO))
	    break;
	  transbuf[translen] = 0;
	  printf ("Translation dot patterns:\n");
	  for (k = 0; k < translen; k++)
	    printf ("%x ", transbuf[k]);
	  printf ("\n");
	  lou_backTranslateString (table, transbuf, &translen,
				   outbuf, &outlen, NULL, NULL, dotsIO);
	  printf ("Back-translation:\n");
	  for (k = 0; k < outlen; k++)
	    printf ("%c", outbuf[k]);
	  printf ("\n");
	  if (outlen == realInlen)
	    if (memcmp (inbuf, outbuf, realInlen) == 0)
	      printf ("Perfect roundtrip!\n");
	}
    }
  lou_free ();
  return 0;
}
