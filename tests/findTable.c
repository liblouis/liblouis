#include <config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "liblouis.h"
#include "louis.h"

int
main(int argc, char **argv)
{
  int success = 0;
  char * match;
  const char * tables[] = {"tablesWithMetadata/foo","tablesWithMetadata/bar",NULL};
  lou_setLogLevel(LOG_DEBUG);
  lou_indexTables(tables);
  match = lou_findTable("id:foo");
  success |= (!match || (strstr(match, "tablesWithMetadata/foo") == NULL));
  match = lou_findTable("language:en");
  success |= (!match || (strstr(match, "tablesWithMetadata/bar") == NULL));

  lou_free();
  return success;
}
