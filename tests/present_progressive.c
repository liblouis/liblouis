/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2008 Eitan Isaacson <eitan@ascender.com>
Copyright (C) 2009 Swiss Library for the Blind, Visually Impaired and Print Disabled

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

/* Note that this test used to fail worse than it does now. The
   current situation isn't hugely critical, though probably still
   incorrect.

   There are two key portions of the string: the "ing" (which gets
   contracted to one character) and the double space at the end. When
   translated, you get: "greetings " -> "greet+s " Notice that the
   translation also contracts the double space into a single space.

   With regard to cursor position, compbrlAtCursor is set, which means
   that the word encompassed by the cursor will be uncontracted
   (computer braille). This means that if the cursor is anywhere
   within "greetings", the translated output will also be "greetings",
   so the cursor positions are identical up to the end of the s
   (position 8).

   It gets more interesting at position 9 (the first space). Now,
   greetings gets contracted, so the output cursor position becomes 7.
   Still correct so far.

   Position 10 (the second space) is the problem. Because
   compbrlAtCursor is set, the current word should probably be
   expanded. In this case, it is just a space. However, the two spaces
   are still compressed into one, even though the second should have
   been expanded. The translation has still contracted the second
   space, even though it should have stopped contracting at the
   cursor.

   See also the description in
   http://code.google.com/p/liblouis/issues/detail?id=4
*/

int
main (int argc, char **argv)
{
  const char *str2 = "greetings  ";
  const int expected_pos2[]={0,1,2,3,4,5,6,7,8,7,8};

  int result = check_cursor_pos(TRANSLATION_TABLE, str2, expected_pos2);

  lou_free();

  return result;
}
