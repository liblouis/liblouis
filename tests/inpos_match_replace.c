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

  const char* txt = "Fussball-Vereinigung";

  const char* table = "tests/tables/inpos_match_replace.ctb";

  const char* brl = "FUSSBALL-V7EINIGUNG";

  const int inpos[] = {0,1,2,3,4,5,6,7,8,9,9,12,13,14,15,16,17,18,19};

  result |= check(table, txt, brl, .expected_inputPos=inpos);

  lou_free();

  return result;

}
