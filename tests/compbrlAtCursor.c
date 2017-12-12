/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2017 Davy Kager

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <stdio.h>
#include <stdlib.h>
#include "liblouis.h"
#include "brl_checks.h"

int
main(int argc, char **argv)
{
  const char *tableList = "tables/unicode.dis,tables/en-us-comp6.ctb";
  const char *str = "én";
  const char *expected = "⠄⡳⠭⠴⠴⠑⠔⠄⠝";
  int result = 0;

  result |= check(tableList, str, expected, .cursorPos=0);
  result |= check(tableList, str, expected, .cursorPos=1);
  result |= check(tableList, str, expected, .mode=compbrlAtCursor, .cursorPos=0);
  result |= check(tableList, str, expected, .mode=compbrlAtCursor, .cursorPos=1);

  lou_free();
  return result;
}
