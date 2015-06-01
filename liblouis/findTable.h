/* liblouis Braille Translation and Back-Translation Library

   Based on the Linux screenreader BRLTTY, copyright (C) 1999-2006 by
   The BRLTTY Team

   Copyright (C) 2015 Bert Frees <bertfrees@gmail.com>

   This file is free software; you can redistribute it and/or modify
   it under the terms of the Lesser or Library GNU General Public
   License as published by the Free Software Foundation; either
   version 3, or (at your option) any later version.

   This file is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   Library GNU General Public License for more details.

   You should have received a copy of the Library GNU General Public
   License along with this program; see the file COPYING. If not,
   write to the Free Software Foundation, 51 Franklin Street, Fifth
   Floor, Boston, MA 02110-1301, USA.

   */

#ifndef __FINDTABLE_H_
#define __FINDTABLE_H_

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

  typedef enum { noEncoding, bigEndian, littleEndian, ascii8 } EncodingType;

  typedef struct
  {
    const char *fileName;
    FILE *in;
    int lineNumber;
    EncodingType encoding;
    int status;
    int linelen;
    int linepos;
    int checkencoding[2];
    widechar line[MAXSTRING];
  } FileInfo;

  /* Read a line of widechar's from an input file */
  int getALine (FileInfo * info);

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				/* __LOUIS_H */
