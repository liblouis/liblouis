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

  int result = 0;

  const char *str1 = "gross";
  const char *str2 = "gro\\x00df";

  const char *expected = "g^";

  result |= check("tests/tables/uplow_with_unicode.ctb", str1, expected);
  result |= check("tests/tables/uplow_with_unicode.ctb", str2, expected);

  result |= check("tests/tables/lowercase_with_unicode.ctb", str1, expected);
  result |= check("tests/tables/lowercase_with_unicode.ctb", str2, expected);

  lou_free();

  return result;
}
