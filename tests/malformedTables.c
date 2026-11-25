/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2026 Darren Carreras

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <config.h>

#include <stdio.h>
#include "liblouis.h"

/**
 * These are regression tests for malformed tables that used to make
 * Liblouis crash.
 */
int
main(int argc, char **argv)
{
  int result = 0;

  /* The "\\#" sequence starts the replacement operand with a literal '#';
   * reject its trailing lone backslash without underflowing ruleDots.length. */
  lou_checkTable("tests/tables/bad-replace.ctb");

  /* Reject a trailing lone backslash in the source operand. */
  lou_checkTable("tests/tables/bad-replace-source.ctb");

  /* A match or backmatch rule with a missing or invalid operand used to be
   * compiled with uninitialized operands (#2112). Loading such a rule from a
   * table file does not show this, because the table is rejected anyway, so
   * add the rules one at a time and check that each one is rejected. The
   * valid rules make sure that rules can be added to the table at all. */
  const char *table = "tests/tables/empty.ctb";
  const char *badMatchRules[] = {
    "match a 1",
    "backmatch a 1",
    "match \\q 1 2 3",
    "backmatch \\q 1 2 3"
  };
  if (!lou_compileString(table, "letter a 1") ||
      !lou_compileString(table, "match - a - 1"))
  {
    printf("Compiling valid rules into %s failed\n", table);
    result = 1;
  }
  for (size_t i = 0; i < sizeof(badMatchRules) / sizeof(badMatchRules[0]); i++)
  {
    if (lou_compileString(table, badMatchRules[i]))
    {
      printf("Compiling invalid rule '%s' succeeded, expected failure\n", badMatchRules[i]);
      result = 1;
    }
  }

  lou_free();

  return result;
}
