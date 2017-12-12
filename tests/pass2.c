/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2012 Swiss Library for the Blind, Visually Impaired and Print Disabled

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "liblouis.h"
#include "brl_checks.h"

int
main(int argc, char **argv)
{

  const char* table = "tests/tables/pass2.ctb";
  int result = 0;

  /* First check a plain word to see if the table works */
  result = check(table, "Rene", "rene");

  /* then try a word which uses pass2 */
  result |= check(table, "Reno", "ren'o");

  lou_free();

  return result;
}
