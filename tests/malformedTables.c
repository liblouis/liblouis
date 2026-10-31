/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2026 Darren Carreras

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include "liblouis.h"

/**
 * These are regression tests for malformed tables that used to make
 * Liblouis crash.
 */

static int recursive_inclusion_errors = 0;

static void
count_recursive_inclusion_errors(logLevels level, const char *message)
{
  if (level == LOU_LOG_ERROR && strstr(message, "Recursive inclusion"))
    recursive_inclusion_errors++;
  fprintf(stderr, "%s\n", message);
}

/* Compiling the table must fail with a recursive inclusion error. */
static int
expect_recursive_inclusion_error(const char *table)
{
  recursive_inclusion_errors = 0;
  if (lou_checkTable(table) != 0)
  {
    printf("Compiling %s succeeded, expected failure\n", table);
    return 1;
  }
  if (recursive_inclusion_errors != 1)
  {
    printf("Compiling %s gave %d recursive inclusion errors, expected 1\n",
           table, recursive_inclusion_errors);
    return 1;
  }
  return 0;
}

int
main(int argc, char **argv)
{
  int result = 0;

  /* The "\\#" sequence starts the replacement operand with a literal '#';
   * reject its trailing lone backslash without underflowing ruleDots.length. */
  lou_checkTable("tests/tables/bad-replace.ctb");

  /* Reject a trailing lone backslash in the source operand. */
  lou_checkTable("tests/tables/bad-replace-source.ctb");

  /* A file that includes itself, directly or through another file, used to
   * recurse until the stack overflowed. */
  lou_registerLogCallback(count_recursive_inclusion_errors);
  result |= expect_recursive_inclusion_error("tests/tables/recursive-include-self.ctb");
  result |= expect_recursive_inclusion_error("tests/tables/recursive-include-1.ctb");
  lou_registerLogCallback(NULL);

  lou_free();

  return result;
}
