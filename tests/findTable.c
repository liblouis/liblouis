#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "louis.h"

int
main(int argc, char **argv)
{
  int success = 0;
  char * match;
  const char * tables[] = {"tablesWithMetadata/foo","tablesWithMetadata/bar"};
  lou_setLogLevel(LOG_DEBUG);
  lou_indexTables(tables);
  match = lou_findTable("id:foo");
  success |= (!match || strcmp(match, "tablesWithMetadata/foo"));
  match = lou_findTable("language:en");
  success |= (!match || strcmp(match, "tablesWithMetadata/bar"));
  return success;
}
