#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "brl_checks.h"

int
main (int argc, char **argv)
{
  /* First check italics with the English table */
  const char *str      = "He said it wasn't always working as expected.";
  const char *typeform = "110000000000000000111111111111110000000000000";
  const char *expected = ".,he sd x wasn't .alw .\"w+ z expect$4";
  
  int result1 = check_translation(TRANSLATION_TABLE, str, typeform, expected);

  /* Then check the German table */
  str      = "Er sagte es funktioniere nicht immer wie erwartet.";
  typeform = "11000000000011111111111111111100000000000000000000";
  expected = "_ER SAGTE ES __FUNKTION0RE NI4T', IMMER W0 ERWARTET.";

  int result2 = check_translation("de-ch-g1.ctb", str, typeform, expected);

  return result1 || result2;
}
