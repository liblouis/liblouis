/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2012 Swiss Library for the Blind, Visually Impaired and Print Disabled
Copyright (C) 2013 Mesar Hameed <mesar.hameed@gmail.com>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include "liblouis.h"
#include "brl_checks.h"

int main(int argc, char **argv)
{
  int result = check_hyphenation("en-us-g1.ctb,hyph_en_US.dic", "straightforward", "010000001001000");
  lou_free();
  return result;
}
