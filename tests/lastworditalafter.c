/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2010 Swiss Library for the Blind, Visually Impaired and Print Disabled

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "liblouis.h"
#include "brl_checks.h"
#include "default_table.h"

int
main(int argc, char **argv)
{

  int result = 0;

  /* First check italics with the English table */
  const char *str      = "He said it wasn't always working as expected.";
  const formtype typeform[] = {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0};
  const char *expected = ".,he sd x wasn't .alw .\"w+ z expect$4";

  result |= check(TRANSLATION_TABLE, str, expected, .typeform=typeform);

  /* Then check a test table that defines lastworditalafter */
  str      = "Er sagte es funktioniere nicht immer wie erwartet.";
  const formtype typeform2[] = {1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  expected = "_ER SAGTE ES __FUNKTION0RE NI4T', IMMER W0 ERWARTET.";

  result |= check("tables/de-ch-g1.ctb", str, expected, .typeform=typeform2);

  lou_free();

  return result;
}
