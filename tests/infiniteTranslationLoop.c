/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2014 Mesar Hameed <mesar.hameed@gmail.com>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "liblouis.h"
#include "brl_checks.h"

/* Demonstrates infinite translation loop.
*  Originally reported at: http://www.freelists.org/post/liblouis-liblouisxml/Translating-com-with-UEBCg2ctb-causes-infinite-loop
*/

int main(int argc, char **argv)
{
  int result = 0;
  const char *text = "---.com";
  const char *expected = "";

  result |= check_translation("tables/UEBC-g2.ctb", text, NULL, expected);

  lou_free();

  return result;
}
