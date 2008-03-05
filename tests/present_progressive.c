#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "brl_checks.h"

int
main (int argc, char **argv)
{
        const char *str2 = "greetings  ";
        const int expected_pos2[]={0,1,2,3,4,5,6,7,8,7,8};

        return check_cursor_pos(str2, expected_pos2);      
}
