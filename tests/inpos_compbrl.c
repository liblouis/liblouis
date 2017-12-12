/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2008 Eitan Isaacson <eitan@ascender.com>
Copyright (C) 2008 James Teh <jamie@jantrid.net>
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
#include "default_table.h"

int
main (int argc, char **argv)
{
  const char *str1 = "user@example.com";
  const char *expected = "_+user@example.com_:";
  const int expected_inpos[] = 
    {0,0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,15,15};

  int result = check(TRANSLATION_TABLE, str1, expected, .expected_inputPos=expected_inpos);

  lou_free();

  return result;
}
