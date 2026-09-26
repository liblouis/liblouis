/* liblouis Braille Translation and Back-Translation Library

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

/* Check that a nofor or noback prefix on contraction, nocont or
   compbrl produces a compiler warning, and that the same opcodes
   without a prefix do not (issue #2093). */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include "liblouis.h"

static int noforWarnings = 0;
static int nobackWarnings = 0;
static int otherWarnings = 0;

static void
count_warnings(logLevels level, const char *message)
{
  if (level != LOU_LOG_WARN || strstr(message, "warnings issued"))
    return;
  if (strstr(message, "nofor contraction rule is never used"))
    noforWarnings++;
  else if (strstr(message, "noback is redundant: nocont")
	   || strstr(message, "noback is redundant: compbrl"))
    nobackWarnings++;
  else
    {
      printf("Unexpected warning: %s\n", message);
      otherWarnings++;
    }
}

int
main(int argc, char **argv)
{
  const char *table = "tests/tables/dotless-direction.ctb";
  int result = 0;

  lou_registerLogCallback(count_warnings);
  lou_setLogLevel(LOU_LOG_WARN);

  if (lou_checkTable(table) == 0)
    {
      printf("Compiling %s failed, expected success\n", table);
      result = 1;
    }
  if (noforWarnings != 1)
    {
      printf("Expected 1 nofor warning, got %d\n", noforWarnings);
      result = 1;
    }
  if (nobackWarnings != 2)
    {
      printf("Expected 2 noback warnings, got %d\n", nobackWarnings);
      result = 1;
    }
  if (otherWarnings != 0)
    result = 1;

  lou_free();
  return result;
}
