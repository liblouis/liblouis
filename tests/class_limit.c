// see issue https://github.com/liblouis/liblouis/issues/105

#include <stdlib.h>
#include "liblouis.h"

int
main(int argc, char **argv)
{
  return (lou_getTable("tables/class_limit") == NULL);
}
