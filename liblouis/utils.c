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
reallocWrapper(void *address, size_t size) {
	if (!(address = realloc(address, size)) && size) _lou_outOfMemory();
	return address;
}

static char *
strdupWrapper(const char *string) {
	char *address = strdup(string);
	if (!address) _lou_outOfMemory();
	return address;
}

char *EXPORT_CALL
lou_getProgramPath(void) {
	char *path = NULL;
	HMODULE handle;

	if ((handle = GetModuleHandle(NULL))) {
		DWORD size = 0X80;
		char *buffer = NULL;

		while (1) {
			buffer = reallocWrapper(buffer, size <<= 1);

			{
				// As the "UNICODE" Windows define may have been set at compilation,
				// This call must be specifically GetModuleFilenameA as further code
				// expects it to be single byte chars.
				DWORD length = GetModuleFileNameA(handle, buffer, size);

				if (!length) {
					printf("GetModuleFileName\n");
					exit(3);
				}

				if (length < size) {
					buffer[length] = 0;
					path = strdupWrapper(buffer);

					while (length > 0)
						if (path[--length] == '\\') break;

					strncpy(path, path, length + 1);
					path[length + 1] = '\0';
					break;
				}
			}
		}

		free(buffer);
	} else {
		printf("GetModuleHandle\n");
		exit(3);
	}

	return path;
}
#endif
/* End of MS contribution */

int EXPORT_CALL
_lou_stringHash(const widechar *c) {
	/* hash function for strings */
	unsigned long int makeHash =
			(((unsigned long int)c[0] << 8) + (unsigned long int)c[1]) % HASHNUM;
	return (int)makeHash;
}

int EXPORT_CALL
_lou_charHash(widechar c) {
	unsigned long int makeHash = (unsigned long int)c % HASHNUM;
	return (int)makeHash;
}

char *EXPORT_CALL
_lou_showString(widechar const *chars, int length) {
	/* Translate a string of characters to the encoding used in character
	 * operands */
	int charPos;
	int bufPos = 0;
	static char scratchBuf[MAXSTRING];
	scratchBuf[bufPos++] = '\'';
	for (charPos = 0; charPos < length && bufPos < (MAXSTRING - 2); charPos++) {
		if (chars[charPos] >= 32 && chars[charPos] < 127)
			scratchBuf[bufPos++] = (char)chars[charPos];
		else {
			char hexbuf[20];
			int hexLength;
			char escapeLetter;

			int leadingZeros;
			int hexPos;
			hexLength = sprintf(hexbuf, "%x", chars[charPos]);
			switch (hexLength) {
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
			if ((bufPos + leadingZeros + hexLength + 4) >= (MAXSTRING - 2)) break;
			scratchBuf[bufPos++] = '\\';
			scratchBuf[bufPos++] = escapeLetter;
			for (hexPos = 0; hexPos < leadingZeros; hexPos++) scratchBuf[bufPos++] = '0';
			for (hexPos = 0; hexPos < hexLength; hexPos++)
				scratchBuf[bufPos++] = hexbuf[hexPos];
		}
	}
	scratchBuf[bufPos++] = '\'';
	scratchBuf[bufPos] = 0;
	return scratchBuf;
}

/**
 * Translate a sequence of dots to the encoding used in dots operands.
 */
char *EXPORT_CALL
_lou_showDots(widechar const *dots, int length) {
	int bufPos = 0;
	static char scratchBuf[MAXSTRING];
	for (int dotsPos = 0; dotsPos < length && bufPos < (MAXSTRING - 1); dotsPos++) {
		for (int mappingPos = 0; dotMapping[mappingPos].key; mappingPos++) {
			if ((dots[dotsPos] & dotMapping[mappingPos].key) &&
					(bufPos < (MAXSTRING - 1)))
				scratchBuf[bufPos++] = dotMapping[mappingPos].value;
		}
		if ((dots[dotsPos] == B16) && (bufPos < (MAXSTRING - 1)))
			scratchBuf[bufPos++] = '0';
		if ((dotsPos != length - 1) && (bufPos < (MAXSTRING - 1)))
			scratchBuf[bufPos++] = '-';
	}
	scratchBuf[bufPos] = 0;
	return scratchBuf;
}

/**
 * Mapping between character attribute and textual representation
 */
static const intCharTupple attributeMapping[] = {
	{ CTC_Space, 's' }, { CTC_Letter, 'l' }, { CTC_Digit, 'd' }, { CTC_Punctuation, 'p' },
	{ CTC_UpperCase, 'U' }, { CTC_LowerCase, 'u' }, { CTC_Math, 'm' }, { CTC_Sign, 'S' },
	{ CTC_LitDigit, 'D' }, { CTC_Class1, 'w' }, { CTC_Class2, 'x' }, { CTC_Class3, 'y' },
	{ CTC_Class4, 'z' }, { 0, 0 },
};

/**
 * Show attributes using the letters used after the $ in multipass
 * opcodes.
 */
char *EXPORT_CALL
_lou_showAttributes(TranslationTableCharacterAttributes a) {
	int bufPos = 0;
	static char scratchBuf[MAXSTRING];
	for (int mappingPos = 0; attributeMapping[mappingPos].key; mappingPos++) {
		if ((a & attributeMapping[mappingPos].key) && bufPos < (MAXSTRING - 1))
			scratchBuf[bufPos++] = attributeMapping[mappingPos].value;
	}
	scratchBuf[bufPos] = 0;
	return scratchBuf;
}

void EXPORT_CALL
_lou_outOfMemory(void) {
	_lou_logMessage(LOG_FATAL, "liblouis: Insufficient memory\n");
	exit(3);
}

#ifdef DEBUG
void EXPORT_CALL
_lou_debugHook(void) {
	char *hook = "debug hook";
	printf("%s\n", hook);
}
#endif
