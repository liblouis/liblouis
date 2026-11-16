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
  /* The "\\#" sequence starts the replacement operand with a literal '#';
   * reject its trailing lone backslash without underflowing ruleDots.length. */
  lou_checkTable("tests/tables/bad-replace.ctb");

  /* Reject a trailing lone backslash in the source operand. */
  lou_checkTable("tests/tables/bad-replace-source.ctb");

  lou_free();

  return 0;
}
