/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2012 Swiss Library for the Blind, Visually Impaired and Print Disabled
Copyright (C) 2013 Mesar Hameed <mesar.hameed@gmail.com>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "liblouis.h"
#include "brl_checks.h"

#undef TRANSLATION_TABLE
#define TRANSLATION_TABLE "tables/en-gb-g1.utb"

int
main(int argc, char **argv)
{

  int result = 0;

  const char *str      = "This is a Test";
  const char *typeform = "00000000000000";
  const char *expected = ",this is a ,test";

  result |= check(TRANSLATION_TABLE, str, expected, .typeform=convert_typeform(typeform));

  str      = "This is a Test in Italic.";
  typeform = "1111111111111111111111111";
  expected = "..,this is a ,test in ,italic4.'";

  result |= check(TRANSLATION_TABLE, str, expected, .typeform=convert_typeform(typeform));

  str      = "This is a Test";
  typeform = "00000111100000";
  expected = ",this .is .a ,test";

  result |= check(TRANSLATION_TABLE, str, expected, .typeform=convert_typeform(typeform));

  /* Test case requested here:
   * http://www.freelists.org/post/liblouis-liblouisxml/Mesar-here-are-some-test-possibilities */

  str      = "time and spirit";
  typeform = "111111111111111";
  expected = ".\"t .& ._s";

  result |= check(TRANSLATION_TABLE, str, expected, .typeform=convert_typeform(typeform));

  lou_free();
  return result;
}
