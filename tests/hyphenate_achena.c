/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2013 Mesar Hameed <mesar.hameed@gmail.com>
Copyright (C) 2014 Swiss Library for the Blind, Visually Impaired and Print Disabled

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "liblouis.h"
#include "brl_checks.h"

int main(int argc, char **argv)
{
  int ret = 0;
  char *tables = "tables/da-dk-g26.ctb";
  char *word = "achena";
  char * hyphens = calloc(8, sizeof(char));

  hyphens[0] = '0';
  hyphens[1] = '1';
  hyphens[2] = '0';
  hyphens[3] = '0';
  hyphens[4] = '1';
  hyphens[5] = '0';

  ret = check_hyphenation(tables, word, hyphens);
  assert(hyphens[6] == '\0');
  assert(hyphens[7] == '\0');
  free(hyphens);
  lou_free();
  return ret;
}
