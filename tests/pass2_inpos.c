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

  /* First check a plain word to see if inpos handling generally
     works */
  const char* str1 = "Rene";
  const int expected_inpos1[] = {0,1,2,3,3};

  result |= check(table, str1, "rene", .expected_inputPos=expected_inpos1);

  /* then try a word which uses pass2 and makes the output longer */
  const char* str2 = "Reno";
  const int expected_inpos2[] = {0,1,2,3,3};

  result |= check(table, str2, "ren'o", .expected_inputPos=expected_inpos2);

  /* finally try a word also uses pass2, deletes a char from the
     output and essentially shortens the output */
  const char* str3 = "xRen";
  const int expected_inpos3[] = {1,2,3};

  result |= check(table, str3, "ren", .expected_inputPos=expected_inpos3);

  lou_free();

  return result;

}
