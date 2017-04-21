/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2016 Bert Frees <bertfrees@gmail.com>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "liblouis.h"

static int errorCount = 0;

static void
log_and_count_errors(logLevels level, const char *message)
{
  switch(level)
    {
    case LOG_ERROR:
      errorCount++;
      printf("\n  ERROR >> %s\n", message);
      break;
    default:
      printf("%s", message);
    }
}

int
main(int argc, char **argv)
{
  lou_registerLogCallback(log_and_count_errors);
  lou_setLogLevel(LOG_DEBUG);
  errorCount = 0;
  lou_findTable("bo:gus"); // because lou_indexTables hasn't been called, this
			   // command will implicitly index all tables on the
			   // table path, and produce errors when it
			   // encounters invalid metadata statements.
  return errorCount;
}
