/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2026 Anthony Tibbs

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <config.h>

#include <stdio.h>
#include "liblouis.h"
#include "brl_checks.h"

/**
 * When a check fails, the diagnostics print the first character that
 * differs. If the expected output is shorter than the received output,
 * there is no expected character at that index, and the diagnostics used
 * to read past the end of the expected buffer (issue #1357). The checks
 * below are meant to fail; the test passes if they report the failure
 * without crashing.
 */
int
main(int argc, char **argv)
{
  int result = 0;
  const char *table = "tables/en-us-g1.ctb";

  fprintf(stderr, "The following two checks are expected to fail:\n");

  /* Expected output is empty: nothing to compare the first received
   * character against. */
  if (check(table, "abc", "") != 1)
  {
    printf("Expected the check with an empty expected output to fail\n");
    result = 1;
  }

  /* Expected output is a prefix of the received output. */
  if (check(table, "abc", "ab") != 1)
  {
    printf("Expected the check with a too short expected output to fail\n");
    result = 1;
  }

  lou_free();

  return result;
}
