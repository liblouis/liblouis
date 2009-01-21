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

#ifndef __LIBLOUIS_H_
#define __LIBLOUIS_H_
#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

#include "louiscfg.h"

#if WIDECHAR_SIZE == 2
#define widechar unsigned short int
#else
#define widechar unsigned int
#endif

  typedef enum
  {
    plain_text = 0,
    italic = 1,
    underline = 2,
    bold = 4,
    computer_braille = 8
  } typeforms;
#define comp_emph_1 italic
#define comp_emph_2 underline
#define comp_emph_3 bold

  typedef enum
  {
    noContractions = 1,
    compbrlAtCursor = 2,
    dotsIO = 4,
    comp8Dots = 8,
    pass1Only = 16
  } translationModes;

char *lou_version ();

  int lou_translateString
    (const char *trantab,
     const widechar *inbuf,
     int *inlen,
     widechar * outbuf,
     int *outlen, char *typeform, char *spacing, int mode);

  int lou_translate (const char *trantab, const widechar
		     *inbuf,
		     int *inlen, widechar * outbuf, int *outlen,
		     char *typeform, char *spacing, int *outputPos, int 
*inputPos, int *cursorPos, int mode);
int lou_hyphenate (const char *trantab, const widechar
	       *inbuf,
      int inlen, char *hyphens, int mode);

   int lou_backTranslateString (const char *trantab,
			       const widechar *inbuf,
			       int *inlen,
			       widechar * outbuf,
			       int *outlen, char *typeform, char
			       *spacing, int mode);

  int lou_backTranslate (const char *trantab, const widechar
			 *inbuf,
			 int *inlen, widechar * outbuf, int *outlen, 
char *typeform, char *spacing, int
			 *outputPos, int *inputPos, int *cursorPos, int
			 mode);
  void lou_logPrint (char *format, ...);
/* prints error messages to a file */

  void lou_logFile (char *filename);
/* Specifies the name of the file to be used by lou_logPrint. If it is 
* not used, this file is stderr*/

  int lou_readCharFromFile (const char *fileName, int *mode);
/*Read a character from a file, whether big-encian, little-endian or 
* ASCII8, and return it as an integer. EOF at end of file. Mode = 1 on 
* first call, any other value thereafter*/

  void *lou_getTable (const char *trantab);
/* This function checks a table for errors. If none are found it loads 
* the table into memory and returns a pointer to it. if errors are found 
* it returns a null pointer. It is called by _ou_translateString and 
* lou_backTranslateString and also by functions in liblouisxml
*/

  void lou_free (void);
/* This function should be called at the end of 
* the application to free all memory allocated by liblouis. */

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				/*LibLOUIS_H_ */
