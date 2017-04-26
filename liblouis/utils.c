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
 * @brief Internal API of liblouis
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
reallocWrapper (void *address, size_t size)
{
  if (!(address = realloc (address, size)) && size)
    outOfMemory ();
  return address;
}

static char *
strdupWrapper (const char *string)
{
  char *address = strdup (string);
  if (!address)
    outOfMemory ();
  return address;
}

char *EXPORT_CALL
lou_getProgramPath ()
{
  char *path = NULL;
  HMODULE handle;

  if ((handle = GetModuleHandle (NULL)))
    {
      DWORD size = 0X80;
      char *buffer = NULL;

      while (1)
	{
	  buffer = reallocWrapper (buffer, size <<= 1);

	  {
		// As the "UNICODE" Windows define may have been set at compilation,
		// This call must be specifically GetModuleFilenameA as further code expects it to be single byte chars.
		DWORD length = GetModuleFileNameA (handle, buffer, size);

	    if (!length)
	      {
		printf ("GetModuleFileName\n");
		exit (3);
	      }

	    if (length < size)
	      {
		buffer[length] = 0;
		path = strdupWrapper (buffer);

		while (length > 0)
		  if (path[--length] == '\\')
		    break;

		strncpy (path, path, length + 1);
		path[length + 1] = '\0';
		break;
	      }
	  }
	}

      free (buffer);
    }
  else
    {
      printf ("GetModuleHandle\n");
      exit (3);
    }

  return path;
}
#endif
/* End of MS contribution */

int
stringHash (const widechar * c)
{
  /*hash function for strings */
  unsigned long int makeHash = (((unsigned long int) c[0] << 8) +
				(unsigned long int) c[1]) % HASHNUM;
  return (int) makeHash;
}

int
charHash (widechar c)
{
  unsigned long int makeHash = (unsigned long int) c % HASHNUM;
  return (int) makeHash;
}

void
outOfMemory ()
{
  logMessage(LOG_FATAL, "liblouis: Insufficient memory\n");
  exit (3);
}

void
debugHook ()
{
  char *hook = "debug hook";
  printf ("%s\n", hook);
}
