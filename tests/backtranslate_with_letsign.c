/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2014 Mesar Hameed <mesar.hameed@gmail.com>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "liblouis.h"
#include "brl_checks.h"

int main(int argc, char **argv) {
  int result = 0;
  const char *tbl = "en-us-g2.ctb";
  const char *input = "but b can ";
  const char *expected = "b ;b c ";

  result |= check_translation(tbl, input, NULL, expected);
  result |= check_backtranslation(tbl, expected, NULL, input);

  return result;
}
