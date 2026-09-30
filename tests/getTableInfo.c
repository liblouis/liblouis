/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2026 Fengxiaoxx <43958531+Fengxiaoxx@users.noreply.github.com>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblouis.h"
#include "internal.h"

static int pathTooLongErrors = 0;

static void
record_error(logLevels level, const char *message)
{
  if (level == LOU_LOG_ERROR
      && strcmp(message, "Resolved table path too long") == 0)
    pathTooLongErrors++;
  printf("%s\n", message);
}

static int
check_language(const char *table)
{
  char *value = lou_getTableInfo(table, "language");
  int result = value == NULL || strcmp(value, "en") != 0;
  if (result)
    printf("lou_getTableInfo() failed for a normal table\n");
  free(value);
  return result;
}

/* A resolved path of MAXSTRING bytes or longer does not fit in
 * fileName[MAXSTRING], including the terminating NUL. */
int
main(int argc, char **argv)
{
  int result = 0;
  size_t i;

  const char *table = "tables/en-us-g1.ctb";
  const char *prefix = "tables";
  const char *suffix = "/en-us-g1.ctb";
  const size_t padCount = MAXSTRING / 2;
  char *path = malloc(strlen(table) + 2 * padCount + 1);
  if (!path)
    return 1;

  lou_registerLogCallback(record_error);
  lou_setLogLevel(LOU_LOG_ERROR);

  /* Both paths name the same table, whose language metadata must exist. */
  result |= check_language(table);

  /* The "/." components keep the real path short while exceeding
   * MAXSTRING, below the resolver's MAX_TABLEFILE_SIZE limit. */
  strcpy(path, prefix);
  for (i = 0; i < padCount; i++) strcat(path, "/.");
  strcat(path, suffix);

  /* Overlong resolved path: must be rejected, not copied into the fixed
   * buffer. */
  char *value = lou_getTableInfo(path, "language");
  if (value)
    {
      printf("analyzeTable() accepted an overlong resolved path\n");
      free(value);
      result = 1;
    }

  if (pathTooLongErrors != 1)
    {
      printf("Expected one overlong resolved path error, got %d\n",
             pathTooLongErrors);
      result = 1;
    }

  /* The same table remains usable after the rejected call. */
  result |= check_language(table);

  free(path);
  lou_free();

  return result;
}
