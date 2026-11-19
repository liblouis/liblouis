/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2014 Swiss Library for the Blind, Visually Impaired and Print Disabled

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "liblouis.h"
#include "resolve_table.h"

#define ASSERT(test)  \
  do {                \
    if (!(test))      \
      result = 1;     \
  } while(0)          \

static char log_buffer[8192];
static int log_buffer_pos = 0;

static void
log_to_buffer(logLevels level, const char *message)
{
  if (log_buffer_pos + strlen(message) + 2 > sizeof(log_buffer)) return;
  log_buffer_pos += sprintf(&log_buffer[log_buffer_pos], "%s\n", message);
}

static void
clear_log(void)
{
  log_buffer[0] = '\0';
  log_buffer_pos = 0;
}

static int
log_contains(const char *expected)
{
  if (strstr(log_buffer, expected)) return 1;
  printf("Expected log to contain \"%s\" but got:\n%s", expected, log_buffer);
  return 0;
}

int
main(int argc, char **argv)
{
    
  /* ====================================================================== *
   * `-- resolve_table                                                      *
   *     |-- dir_1                                                          *
   *     |   |-- dir_1.1                                                    *
   *     |   |   `-- table_1.1.1                                            *
   *     |   |-- table_1.1                                                  *
   *     |   |-- table_1.2      ------------------------------              *
   *     |   `-- table_1.3 --> | include dir_1.1/table_1.1.1 |              *
   *     |-- dir_2             `-----------------------------               *
   *     |   |-- table_2.1      ------------------                          *
   *     |   `-- table_2.2 --> | include table_1 |                          *
   *     |-- table_1           `-----------------                           *
   *     |-- table_2      ------------------                                *
   *     |-- table_3 --> | include table_2 |                                *
   *     |               `-----------------                                 *
   *     |                --------------------------                        *
   *     |-- table_4 --> | include dir_1/table_1.3 |                        *
   *     |               `-------------------------                         *
   *     |                --------------------------                        *
   *     |-- table_5 --> | include dir_2/table_2.2 |                        *
   *     |               `-------------------------                         *
   *     |                ----------------------                            *
   *     `-- table_6 --> | dir_1.1/table_1.1.1 |                            *
   *                     `---------------------                             *
   * ====================================================================== */
  
  int result = 0;

  // this test relies on being in the test dir, so that it can test
  // finding tables by relative path
  if (chdir(TEST_SRC_DIR)) return 1;

  // Full path
  setenv ("LOUIS_TABLEPATH", "", 1);
  ASSERT (lou_getTable ("tables/resolve_table/table_1"));
  
  // File name not on LOUIS_TABLEPATH: the error names the directory the
  // relative path was resolved against (see issue #317)
  {
    char cwd[4096];
    char expected[4096 + 64];
    lou_registerLogCallback (log_to_buffer);
    clear_log ();
    ASSERT (!lou_getTable ("table_1"));
    ASSERT (log_contains ("Cannot resolve table 'table_1'"));
    ASSERT (getcwd (cwd, sizeof(cwd)));
    sprintf (expected,
	     "Relative table paths are resolved against the current directory: %s",
	     cwd);
    ASSERT (log_contains (expected));

    // ... but not for an absolute path
    clear_log ();
    ASSERT (!lou_getTable ("/nonexistent/table_1"));
    ASSERT (log_contains ("Cannot resolve table '/nonexistent/table_1'"));
    ASSERT (!strstr (log_buffer, "Relative table paths"));
    lou_registerLogCallback (NULL);
  }
  
  // File name on LOUIS_TABLEPATH
  setenv ("LOUIS_TABLEPATH", "tables/resolve_table", 1);
  ASSERT (lou_getTable ("table_1"));
  
  // First is full path, second is in same directory
  setenv ("LOUIS_TABLEPATH", "", 1);
  ASSERT (lou_getTable ("tables/resolve_table/table_1,"
			"table_2"));
  
  // First is full path, second is not in same directory
  ASSERT (!lou_getTable ("tables/resolve_table/table_1,"
			 "table_1.1.1"));
  
  // Two full paths
  ASSERT (lou_getTable ("tables/resolve_table/dir_1/table_1.1,"
			"tables/resolve_table/dir_2/table_2.1"));
  
  // First is full path, second is on LOUIS_TABLEPATH, third is in same
  // directory as first
  setenv ("LOUIS_TABLEPATH", "tables/resolve_table/dir_2", 1);
  ASSERT (lou_getTable ("tables/resolve_table/dir_1/table_1.1,"
			"table_2.1,"
			"table_1.2"));
  
  // First is full path, second is in subdirectory
  setenv ("LOUIS_TABLEPATH", "", 1);
  ASSERT (lou_getTable ("tables/resolve_table/table_1,"
			"dir_1/table_1.1"));
  
  // Two file names in different directories, but both on LOUIS_TABLEPATH
  setenv ("LOUIS_TABLEPATH", "tables/resolve_table/dir_1,"
			     "tables/resolve_table/dir_2", 1);
  ASSERT (lou_getTable ("table_1.2,"
			"table_2.1"));
  
  // First is file name on LOUIS_TABLEPATH, second is full path, third is in
  // same directory as second
  setenv ("LOUIS_TABLEPATH", "tables/resolve_table", 1);
  ASSERT (!lou_getTable ("table_1,"
			 "tables/resolve_table/dir_1/table_1.1,"
			 "table_1.2"));
  
  // Full path, include table in same directory
  setenv ("LOUIS_TABLEPATH", "", 1);
  ASSERT (lou_getTable ("tables/resolve_table/table_3"));
  
  // Full path, include table in subdirectory
  ASSERT (lou_getTable ("tables/resolve_table/dir_1/table_1.3"));
  
  // Full path, include table in subdirectory, from there include table in
  // sub-subdirectory
  ASSERT (lou_getTable ("tables/resolve_table/table_4"));
  
  // Full path, include table in subdirectory, from there include table in
  // first directory
  ASSERT (!lou_getTable ("tables/resolve_table/table_5"));
  
  // Full path, include table in subdirectory, from there include table on
  // LOUIS_TABLEPATH
  setenv ("LOUIS_TABLEPATH", "tables/resolve_table", 1);
  ASSERT (lou_getTable ("tables/resolve_table/table_5"));
  
  // Full path, include table in subdirectory of LOUIS_TABLEPATH
  setenv ("LOUIS_TABLEPATH", "tables/resolve_table/dir_1", 1);
  ASSERT (lou_getTable ("tables/resolve_table/table_6"));

  // Overlong LOUIS_TABLEPATH: the search path cannot be constructed, but
  // this must not crash, and full paths must still resolve (see issue #1999)
  {
    char longPath[4096];
    memset (longPath, 'a', sizeof(longPath) - 1);
    longPath[sizeof(longPath) - 1] = '\0';
    setenv ("LOUIS_TABLEPATH", longPath, 1);
    lou_free();  // clear the table cache so resolution actually runs
    ASSERT (!lou_getTable ("table_1"));
    ASSERT (lou_getTable ("tables/resolve_table/table_1"));
  }

  lou_free();

  return result;
  
}
