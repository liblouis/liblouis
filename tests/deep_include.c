#include <config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "liblouis.h"
#include "resolve_table.h"

int
main(int argc, char **argv)
{
  int result = 0;
  if (!lou_getTable ("deep_include/15"))
	  result = 1;
  lou_free();
  return result;
}
