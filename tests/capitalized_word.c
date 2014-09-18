/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2009 Eitan Isaacson <eitan@ascender.com>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "brl_checks.h"

int
main (int argc, char **argv)
{
        const char *str1 = "World ";
        const int expected_pos1[]={0,1,2,3,4,3};

        return check_cursor_pos(str1, expected_pos1);        
}
