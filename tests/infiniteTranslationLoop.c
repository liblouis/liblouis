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

  result |= check_translation("UEBC-g2.ctb", text, NULL, expected);
  return result;
}
