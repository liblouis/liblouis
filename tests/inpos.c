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

/*   input      aaabcd ef g
                012345 67 8
     pass0      a  bcddef g
                0  345567 8
     pass1      x   cddw  g
                0   4556  8
     pass2      zy    dw  g
                00    56  8
     pass3      zy    deeeg
                00    56668
*/

  int result = 0;

  const char* txt = "aaabcdefg";

  const char* table1 = "tests/tables/inpos_pass0.ctb";
  const char* table2 = "tests/tables/inpos_pass0.ctb,tests/tables/inpos_pass1.ctb";
  const char* table3 = "tests/tables/inpos_pass0.ctb,tests/tables/inpos_pass1.ctb,tests/tables/inpos_pass2.ctb";
  const char* table4 = "tests/tables/inpos_pass0.ctb,tests/tables/inpos_pass1.ctb,tests/tables/inpos_pass2.ctb,tests/tables/inpos_pass3.ctb";

  const char* brl1 = "abcddefg";
  const char* brl2 = "xcddwg";
  const char* brl3 = "zydwg";
  const char* brl4 = "zydeeeg";

  const int inpos1[] = {0,3,4,5,5,6,7,8};
  const int inpos2[] = {0,4,5,5,6,8};
  const int inpos3[] = {0,0,5,6,8};
  const int inpos4[] = {0,0,5,6,6,6,8};

  result |= check(table1, txt, brl1, .expected_inputPos=inpos1);
  result |= check(table2, txt, brl2, .expected_inputPos=inpos2);
  result |= check(table3, txt, brl3, .expected_inputPos=inpos3);
  result |= check(table4, txt, brl4, .expected_inputPos=inpos4);

  lou_free();

  return result;

}
