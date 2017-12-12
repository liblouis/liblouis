/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2014 Swiss Library for the Blind, Visually Impaired and Print Disabled

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "liblouis.h"
#include "default_table.h"
#include "brl_checks.h"

typedef struct test {
  char *input;
  char *expected;
} test_s;

test_s tests[] = {
  {",k5", "Ken"},
  {",m5", "Men"},
  {",m>k", "Mark"},
  {",f5", "Fen"},
  {",l5", "Len"},
  {",t5", "Ten"},
  NULL
};

int main(int argc, char **argv) {
  int result = 0;

  for (int i = 0; tests[i].input; i++)
    result |= check(TRANSLATION_TABLE, tests[i].input, tests[i].expected, .direction=1);

  lou_free();
  return result;
}
