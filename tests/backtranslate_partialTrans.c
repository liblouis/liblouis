/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2016 NV Access Limited

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
  char *normalExpected;
  char *partialExpected;
} test_s;

test_s tests[] = {
  {"ab", "about", "ab"}, /* word */
  {"7", "were", "("}, /* lowword */
  {"0", "was", "by"}, /* joinword */
  {"unnec", "unnecessary", "unnec"}, /* prfword */
  {"abb", "a;", "abb"}, /* midword */
  {"a4", "a.", "add"}, /* postpunc */
  NULL
};

int main(int argc, char **argv) {
  int result = 0;

  for (int i = 0; tests[i].input; i++) {
		result |= check(TRANSLATION_TABLE, tests[i].input, tests[i].normalExpected, .direction=1);
		result |= check(TRANSLATION_TABLE, tests[i].input, tests[i].partialExpected, .direction=1, .mode=partialTrans);
  }

  lou_free();
  return result;
}
