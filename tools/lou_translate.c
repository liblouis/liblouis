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
#define BUFSIZE 2048

int
main (int argc, char **argv)
{
  widechar inbuf[BUFSIZE];
  widechar transbuf[BUFSIZE];
  widechar outbuf[BUFSIZE];
  int inlen;
  int translen;
  int outlen;
  int ch = 0;
  int k;
  if (argc != 3)
    {
      fprintf (stderr, "Usage: translate -f|-b tablename\n");
      exit (1);
    }
  if (!(argv[1][0] == '-' && (argv[1][1] == 'f' || argv[1][1] == 'b')))
    {
      fprintf (stderr, "The first argument must be -f or -b.\n");
      exit (1);
    }
  if (argv[1][1] == 'f')
    while (1)
      {
	translen = BUFSIZE;
	inlen = 0;
	while ((ch = getchar ()) != '\n' && inlen < BUFSIZE)
	  inbuf[inlen++] = ch;
	if (ch == EOF)
	  break;
	inbuf[inlen] = 0;
	if (!lou_translateString (argv[2], inbuf, &inlen,
				  transbuf, &translen, NULL, NULL, 0))
	  break;
	transbuf[translen] = 0;
	for (k = 0; k < translen; k++)
	  printf ("%c", transbuf[k]);
	printf ("\n");
      }
  else
    while (1)
      {
	outlen = BUFSIZE;
	translen = 0;
	while ((ch = getchar ()) != '\n' && translen < BUFSIZE)
	  transbuf[translen++] = ch;
	if (ch == EOF)
	  break;
	transbuf[translen] = 0;
	if (!lou_backTranslateString (argv[2], transbuf, &translen,
				      outbuf, &outlen, NULL, NULL, 0))
	  break;
	for (k = 0; k < outlen; k++)
	  printf ("%c", outbuf[k]);
	printf ("\n");
      }
  lou_free ();
  return 0;
}
