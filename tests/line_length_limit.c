// see issue https://github.com/liblouis/liblouis/issues/106

#include <stdlib.h>
#include "liblouis.h"

int
main(int argc, char **argv)
{
  return (lou_getTable("tables/line_length_limit") == NULL);
}
