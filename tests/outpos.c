/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2012 James Teh <jamie@nvaccess.org>

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
  const char* table = "en-us-g1.ctb";
  int result = 0;

  /* One input maps to one output. */
  const char* str1 = "ab";
  const int expected_outpos1[] = {0, 1};
  result |= check_outpos(table, str1, expected_outpos1);

  /* One input maps to two output. */
  const char* str2 = "1";
  const int expected_outpos2[] = {1};
  result |= check_outpos(table, str2, expected_outpos2);

  /* Similar to test 2, but multiple input. */
  const char* str3 = "1 2";
  const int expected_outpos3[] = {1, 2, 4};
  result |= check_outpos(table, str3, expected_outpos3);
  const char* str4 = "1 2 3";
  const int expected_outpos4[] = {1, 2, 4, 5, 7};
  result |= check_outpos(table, str4, expected_outpos4);

  lou_free();

  return result;
}
