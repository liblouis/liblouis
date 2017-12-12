/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2012 Swiss Library for the Blind, Visually Impaired and Print Disabled
Copyright (C) 2012 Mesar Hameed <mesar.hameed@gmail.com>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "liblouis.h"
#include "brl_checks.h"

int main(int argc, char **argv)
{

  int result = 0;

  const char *text = "\\x280d\\x280e";  // "⠍⠎"
  const char *expected = "sm";  // "⠎⠍"

  result |= check("tests/tables/letterDefTest_letter.ctb", text, expected);
  result |= check("tests/tables/letterDefTest_lowercase.ctb", text, expected);
  //result |= check("tests/tables/letterDefTest_uplow.ctb", text, expected);
  result |= check("tests/tables/letterDefTest_uppercase.ctb", text, expected);

  lou_free();

  return result;
}
