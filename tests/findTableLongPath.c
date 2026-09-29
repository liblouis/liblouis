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

#ifdef _WIN32

int
main(int argc, char **argv)
{
  printf("Skipped: this test needs a table path longer than MAXSTRING\n");
  return 77;
}

#else /* !_WIN32 */

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define DEEP_LENGTH (MAXSTRING - 3)

static int reportedError = 0;

static void EXPORT_CALL
check_log(logLevels level, const char *message)
{
  if (level == LOU_LOG_ERROR) reportedError = 1;
  fprintf(stderr, "%s\n", message);
}

/* Create base + "/compNNN/..." until the path is DEEP_LENGTH bytes long.
 * Returns 0 on success, 1 if the file system does not allow a path that
 * long (e.g. PATH_MAX < DEEP_LENGTH), -1 on other errors. On failure,
 * path still names the deepest directory successfully created. */
static int
make_deep_dir(char *path, size_t pathSize, const char *base)
{
  size_t len = strlen(base);
  int n = 0;
  if (len + 1 >= pathSize || DEEP_LENGTH >= pathSize) return -1;
  strcpy(path, base);
  while (len + 8 <= DEEP_LENGTH)
    {
      size_t parentLength = len;
      len += (size_t)sprintf(path + len, "/comp%03d", n++);
      if (mkdir(path, 0700) != 0)
        {
          int error = errno;
          path[parentLength] = '\0';
          return error == ENAMETOOLONG ? 1 : -1;
        }
    }
  if (len < DEEP_LENGTH)
    {
      char name[16];
      size_t parentLength = len;
      size_t i;
      size_t count = DEEP_LENGTH - len - 1;
      if (count >= sizeof(name)) return -1;
      for (i = 0; i < count; i++) name[i] = 'd';
      name[count] = '\0';
      len += (size_t)sprintf(path + len, "/%s", name);
      if (mkdir(path, 0700) != 0)
        {
          int error = errno;
          path[parentLength] = '\0';
          return error == ENAMETOOLONG ? 1 : -1;
        }
    }
  return 0;
}

/* Remove the directories created by make_deep_dir, deepest first. They must
 * be empty. */
static int
remove_deep_dir(char *path, size_t baseLength)
{
  while (strlen(path) > baseLength)
    {
      if (rmdir(path) != 0) return -1;
      *strrchr(path, '/') = '\0';
    }
  return 0;
}

static int
write_table(const char *path)
{
  FILE *f = fopen(path, "w");
  if (!f) return -1;
  fprintf(f, "#+language: en\n");
  fclose(f);
  return 0;
}

int
main(int argc, char **argv)
{
  int result = 0;
  int deep;
  char shortDir[] = "/tmp/louis-findtable-XXXXXX";
  char longBase[] = "/tmp/louis-findtable-XXXXXX";
  char shortTable[4096];
  char longTable[4096] = "";
  char longDir[4096] = "";
  char *match;

  if (!mkdtemp(shortDir))
    {
      printf("Could not create a temporary directory\n");
      return 1;
    }
  if (!mkdtemp(longBase))
    {
      printf("Could not create a temporary directory\n");
      rmdir(shortDir);
      return 1;
    }
  sprintf(shortTable, "%s/t.ctb", shortDir);
  if (write_table(shortTable) != 0)
    {
      printf("Could not write %s\n", shortTable);
      result = 1;
      goto cleanup;
    }

  /* Control: a table on a normal table path is found. */
  if (setenv("LOUIS_TABLEPATH", shortDir, 1) != 0)
    {
      result = 1;
      goto cleanup;
    }
  lou_setLogLevel(LOU_LOG_INFO);
  lou_free(); /* drop the table index so that it is rebuilt */
  match = lou_findTable("language:en");
  if (match == NULL || strstr(match, shortDir) == NULL)
    {
      printf("The table in %s was not found\n", shortDir);
      result = 1;
    }
  free(match);

  /* An entry of a directory of DEEP_LENGTH bytes must be skipped, not
   * copied into the fixed fileName[MAXSTRING] buffer. */
  deep = make_deep_dir(longDir, sizeof(longDir), longBase);
  if (deep == 1)
    {
      printf("Skipped: the file system does not allow a %d byte path\n", DEEP_LENGTH);
      if (result == 0) result = 77;
    }
  else if (deep != 0)
    {
      printf("Could not create a directory under %s\n", longBase);
      result = 1;
    }
  else
    {
      sprintf(longTable, "%s/somefile.ctb", longDir);
      if (write_table(longTable) != 0)
        {
          printf("Could not write %s\n", longTable);
          result = 1;
        }
      else
        {
          if (setenv("LOUIS_TABLEPATH", longDir, 1) != 0)
            {
              result = 1;
              goto cleanup;
            }
          lou_free();
          lou_registerLogCallback(check_log);
          match = lou_findTable("language:en");
          if (match != NULL)
            {
              printf("A table with an overlong path was indexed\n");
              free(match);
              result = 1;
            }
          if (!reportedError)
            {
              printf("No error was reported for the overlong table path\n");
              result = 1;
            }
        }
    }

cleanup:
  if (longTable[0] && unlink(longTable) != 0 && errno != ENOENT) result = 1;
  if (longDir[0] && remove_deep_dir(longDir, strlen(longBase)) != 0) result = 1;
  if (unlink(shortTable) != 0 && errno != ENOENT) result = 1;
  if (rmdir(shortDir) != 0) result = 1;
  if (rmdir(longBase) != 0) result = 1;
  lou_registerLogCallback(NULL);
  lou_free();

  return result;
}

#endif /* !_WIN32 */
