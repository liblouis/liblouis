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
#include "brl_checks.h"

#define TRANSLATION_TABLE "tables/en-ueb-g1.ctb"
#define UNDEFINED_DOTS "_"

int main(int argc, char **argv) {
  int result = 0;

  result |= check_backtranslation(TRANSLATION_TABLE, UNDEFINED_DOTS, NULL, "\\\\456/");
  result |= check_backtranslation_with_mode(TRANSLATION_TABLE, UNDEFINED_DOTS, NULL, "", noUndefinedDots);

  lou_free();
  return result;
}
