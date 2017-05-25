/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2015 Bert Frees <bertfrees@gmail.com>

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
 * @brief Find translation tables
 */

#ifndef __FINDTABLE_H_
#define __FINDTABLE_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum { noEncoding, bigEndian, littleEndian, ascii8 } EncodingType;

typedef struct {
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

/**
 * Read a line of widechar's from an input file
 */
int EXPORT_CALL _lou_getALine(FileInfo *info);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FINDTABLE_H_ */
