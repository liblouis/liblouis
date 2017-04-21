/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2015 Bert Frees
 
This file is part of liblouis.

liblouis is free software: you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

liblouis is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with liblouis. If not, see <http://www.gnu.org/licenses/>.

*/

/**
 * @file
 * @brief Find translation tables
 */

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include "louis.h"
#include "findTable.h"

/* =============================== LIST =================================== */

typedef struct List
{
  void * head;
  void (* free)(void *);
  struct List * tail;
} List;

/**
 * Returns a list with the element `x' added to `list'. Returns a sorted list
 * if `cmp' is not NULL and if `list' is also sorted. New elements replace
 * existing ones if they are equal according to `cmp'. If `cmp' is NULL,
 * elements are simply prepended to the list. The function `dup' is used to
 * duplicate elements before they are added to the list. If `free' function is
 * used to free elements when they are removed from the list. The returned
 * list must be freed by the caller.
 */
static List *
list_conj(List * list,
	  void * x,
	  int (* cmp)(void *, void *),
	  void * (* dup)(void *),
	  void (* free)(void *))
{
  if (!list)
    {
      list = malloc(sizeof(List));
      list->head = dup ? dup(x) : x;
      list->free = free;
      list->tail = NULL;
      return list;
    }
  else if (!cmp)
    {
      List * l = malloc(sizeof(List));
      l->head = dup ? dup(x) : x;
      l->free = free;
      l->tail = list;
      return l;
    }
  else
    {
      List * l1 = list;
      List * l2 = NULL;
      while (l1)
	{
	  int c = cmp(l1->head, x);
	  if (c > 0)
	    break;
	  else if (c < 0)
	    {
	      l2 = l1;
	      l1 = l2->tail;
	    }
	  else
	    {
	      if (x != l1->head && !dup && free)
		free(x);
	      return list;
	    }
	}
      List * l3 = malloc(sizeof(List));
      l3->head = dup? dup(x) : x;
      l3->free = free;
      l3->tail = l1;
      if (!l2)
	list = l3;
      else
	l2->tail = l3;
      return list;
    }
}

/**
 * Free an instance of type List.
 */
static void
list_free(List * list)
{
  if (list)
    {
      if (list->free)
	list->free(list->head);
      list_free(list->tail);
      free(list);
    }
}

/**
 * Sort a list based on a comparison function.
 */
static List *
list_sort(List * list, int (* cmp)(void *, void *))
{
  List * newList = NULL;
  List * l;
  for (l = list; l; l = l->tail)
    {
      newList = list_conj(newList, l->head, cmp, NULL, l->free);
      l->free = NULL;
    }
  list_free(list);
  return newList;
}

/**
 * Get the size of a list.
 */
static int
list_size(List * list)
{
  int len = 0;
  List * l;
  for (l = list; l; l = l->tail)
    len++;
  return len;
}

/**
 * Convert a list into a NULL terminated array.
 */
static void **
list_toArray(List * list, void * (* dup)(void *))
{
  void ** array;
  List * l;
  int i;
  array = malloc((1 + list_size(list)) * sizeof(void *));
  i = 0;
  for (l = list; l; l = l->tail)
    array[i++] = dup ? dup(l->head) : l->head;
  array[i] = NULL;
  return array;
}

/* ============================== FEATURE ================================= */

typedef struct
{
  char * key;
  char * val;
} Feature;

typedef struct
{
  Feature feature;
  int importance;
} FeatureWithImportance;

typedef struct
{
  char * name;
  List * features;
} TableMeta;

/**
 * Create an instance of type Feature.
 * The `key' and `val' strings are duplicated. Leaving out the `val'
 * argument results in a value of "yes".
 */
static Feature
feature_new(const char * key, const char * val)
{
  static char * YES = "yes";
  Feature f;
  f.key = strdup(key);
  f.val = strdup(val ? val : YES);
  return f;
}

/**
 * Free an instance of type Feature
 */
static void
feature_free(Feature * f)
{
  if (f)
    {
      free(f->key);
      free(f->val);
      free(f);
    }
}

/* ======================================================================== */

/**
 * Sort features based on their keys.
 */
static int
cmpKeys(Feature * f1, Feature * f2)
{
  return strcmp(f1->key, f2->key);
}

/**
 * Compute the match quotient of the features in a query against the features in a table's metadata.
 *
 * The features are assumed to be sorted and to have no duplicate
 * keys. The query's features must be of type FeatureWithImportance.
 * How a feature contributes to the match quotient depends on its
 * importance, on whether the feature is undefined, defined with the
 * same value (positive match), or defined with a different value
 * (negative match), and on the `fuzzy' argument. If the `fuzzy'
 * argument evaluates to true, negative matches and undefined features
 * get a lower penalty.
 */
static int
matchFeatureLists(const List * query, const List * tableFeatures, int fuzzy)
{
  static int POS_MATCH = 10;
  static int NEG_MATCH = -100;
  static int UNDEFINED = -20;
  static int EXTRA = -1;
  static int POS_MATCH_FUZZY = 10;
  static int NEG_MATCH_FUZZY = -25;
  static int UNDEFINED_FUZZY = -5;
  static int EXTRA_FUZZY = -1;
  int posMatch, negMatch, undefined, extra;
  if (!fuzzy)
    {
      posMatch = POS_MATCH;
      negMatch = NEG_MATCH;
      undefined = UNDEFINED;
      extra = EXTRA;
    }
  else
    {
      posMatch = POS_MATCH_FUZZY;
      negMatch = NEG_MATCH_FUZZY;
      undefined = UNDEFINED_FUZZY;
      extra = EXTRA_FUZZY;
    }
  int quotient = 0;
  const List * l1 = query;
  const List * l2 = tableFeatures;
  while (1)
    {
      if (!l1)
	{
	  if (!l2)
	    break;
	  quotient += extra;
	  l2 = l2->tail;
	}
      else if (!l2)
	{
	  quotient += undefined;
	  l1 = l1->tail;
	}
      else
	{
	  int cmp = cmpKeys(l1->head, l2->head);
	  if (cmp < 0)
	    {
	      quotient += undefined;
	      l1 = l1->tail;
	    }
	  else if (cmp > 0)
	    {
	      quotient += extra;
	      l2 = l2->tail;
	    }
	  else
	    {
	      if (strcmp(((Feature *)l1->head)->val, ((Feature *)l2->head)->val) == 0)
		quotient += posMatch;
	      else
		quotient += negMatch;
	      l1 = l1->tail;
	      l2 = l2->tail;
	    }
	}
    }
  return quotient;
}

/**
 * Return true if a character matches [0-9A-Za-z_-]
 */
static int
isIdentChar(char c)
{
  return (c >= '0' && c <= '9')
      || (c >= 'A' && c <= 'Z')
      || (c >= 'a' && c <= 'z')
      || c == '-'
      || c == '_';
}

/**
 * Parse a table query into a list of features. Features defined first get a
 * higher importance.
 */
static List *
parseQuery(const char * query)
{
  List * features = NULL;
  const char * key = NULL;
  const char * val = NULL;
  size_t keySize = 0;
  size_t valSize = 0;
  const char * c;
  int pos = 0;
  while (1)
    {
      c = &query[pos++];
      if ((*c == ' ') || (*c == '\t') || (*c == '\n') | (*c == '\0'))
	{
	  if (key)
	    {
	      char * k = strndup(key, keySize);
	      char * v = val ? strndup(val, valSize) : NULL;
	      FeatureWithImportance f = { feature_new(k, v), 0 };
	      logMessage(LOG_DEBUG, "Query has feature '%s:%s'", f.feature.key, f.feature.val);
	      features = list_conj(features, memcpy(malloc(sizeof(f)), &f, sizeof(f)),
				   NULL, NULL, (void (*)(void *))feature_free);
	      free(k);
	      free(v);
	      key = val = NULL;
	      keySize = valSize = 0;
	    }
	  if (*c == '\0')
	    break;
	}
      else if (*c == ':')
	{
	  if (!key || val)
	    goto compile_error;
	  else
	    {
	      c = &query[pos++];
	      if (isIdentChar(*c))
		{
		  val = c;
		  valSize = 1;
		}
	      else
		goto compile_error;
	    }
	}
      else if (isIdentChar(*c))
	{
	  if (val)
	    valSize++;
	  else if (key)
	    keySize++;
	  else
	    {
	      key = c;
	      keySize = 1;
	    }
	}
      else
	goto compile_error;
    }
  int k = 1;
  List * l;
  for (l = features; l; l = l->tail)
    {
      FeatureWithImportance * f = l->head;
      f->importance = k++;
    }
  return list_sort(features, (int (*)(void *, void *))cmpKeys);
 compile_error:
  logMessage(LOG_ERROR, "Unexpected character '%c' at position %d", c, pos);
  list_free(features);
  return NULL;
}

/**
 * Convert a widechar string to a normal string.
 */
static char *
widestrToStr(const widechar * str, size_t n)
{
  char * result = malloc((1 + n) * sizeof(char));
  int k;
  for (k = 0; k < n; k++)
    result[k] = str[k];
  result[k] = '\0';
  return result;
}

/**
 * Extract a list of features from a table.
 */
static List *
analyzeTable(const char * table)
{
  static char fileName[MAXSTRING];
  char ** resolved;
  List * features = NULL;
  FileInfo info;
  int k;
  resolved = resolveTable(table, NULL);
  if (resolved == NULL)
    {
      logMessage(LOG_ERROR, "Cannot resolve table '%s'", table);
      return NULL;
    }
  sprintf(fileName, "%s", *resolved);
  k = 0;
  for (k = 0; resolved[k]; k++)
    free(resolved[k]);
  free(resolved);
  if (k > 1)
    {
      logMessage(LOG_ERROR, "Table '%s' resolves to more than one file", table);
      return NULL;
    }
  info.fileName = fileName;
  info.encoding = noEncoding;
  info.status = 0;
  info.lineNumber = 0;
  if ((info.in = fopen(info.fileName, "rb")))
    {
      while (getALine(&info))
	{
	  if (info.linelen == 0);
	  else if (info.line[0] == '#')
	    {
	      if (info.linelen >= 2 && info.line[1] == '+')
		{
		  widechar * key = NULL;
		  widechar * val = NULL;
		  size_t keySize = 0;
		  size_t valSize = 0;
		  info.linepos = 2;
		  if (info.linepos < info.linelen && isIdentChar(info.line[info.linepos]))
		    {
		      key = &info.line[info.linepos];
		      keySize = 1;
		      info.linepos++;
		      while (info.linepos < info.linelen && isIdentChar(info.line[info.linepos]))
			{
			  keySize++;
			  info.linepos++;
			}
		      if (info.linepos < info.linelen && info.line[info.linepos] == ':')
			{
			  info.linepos++;
			  while (info.linepos < info.linelen
				 && (info.line[info.linepos] == ' ' || info.line[info.linepos] == '\t'))
			    info.linepos++;
			  if (info.linepos < info.linelen && isIdentChar(info.line[info.linepos]))
			    {
			      val = &info.line[info.linepos];
			      valSize = 1;
			      info.linepos++;
			      while (info.linepos < info.linelen && isIdentChar(info.line[info.linepos]))
				{
				  valSize++;
				  info.linepos++;
				}
			    }
			  else
			    goto compile_error;
			}
		      if (info.linepos == info.linelen)
			{
			  char * k = widestrToStr(key, keySize);
			  char * v = val ? widestrToStr(val, valSize) : NULL;
			  Feature f = feature_new(k, v);
			  logMessage(LOG_DEBUG, "Table has feature '%s:%s'", f.key, f.val);
			  features = list_conj(features, memcpy(malloc(sizeof(f)), &f, sizeof(f)),
					       NULL, NULL, (void (*)(void *))feature_free);
			  free(k);
			  free(v);
			}
		      else
			goto compile_error;
		    }
		  else
		    goto compile_error;
		}
	    }
	  else
	    break;
	}
      fclose(info.in);
    }
  else
    logMessage (LOG_ERROR, "Cannot open table '%s'", info.fileName);
  return list_sort(features, (int (*)(void *, void *))cmpKeys);
 compile_error:
  if (info.linepos < info.linelen)
    logMessage(LOG_ERROR, "Unexpected character '%c' on line %d, column %d",
	       info.line[info.linepos],
	       info.lineNumber,
	       info.linepos);
  else
    logMessage(LOG_ERROR, "Unexpected newline on line %d", info.lineNumber);
  list_free(features);
  return NULL;
}

static List * tableIndex = NULL;

void EXPORT_CALL
lou_indexTables(const char ** tables)
{
  const char ** table;
  list_free(tableIndex);
  tableIndex = NULL;
  for (table = tables; *table; table++)
    {
      logMessage(LOG_DEBUG, "Analyzing table %s", *table);
      List * features = analyzeTable(*table);
      if (features)
	{
	  TableMeta m = { strdup(*table), features };
	  tableIndex = list_conj(tableIndex, memcpy(malloc(sizeof(m)), &m, sizeof(m)), NULL, NULL, free);
	}
    }
  if (!tableIndex)
    logMessage(LOG_WARN, "No tables were indexed");
}

#ifdef _WIN32
#define DIR_SEP '\\'
#else
#define DIR_SEP '/'
#endif

/**
 * Returns the list of files found on searchPath, where searchPath is a
 * comma-separated list of directories.
 */
static List *
listFiles(char * searchPath)
{
  List * list = NULL;
  char * dirName;
  DIR * dir;
  struct dirent * file;
  static char fileName[MAXSTRING];
  struct stat info;
  int pos = 0;
  int n;
  while (1)
    {
      for (n = 0; searchPath[pos + n] != '\0' && searchPath[pos + n] != ','; n++);
      dirName = strndup(&searchPath[pos], n);
      if ((dir = opendir(dirName)))
	{
	  while ((file = readdir(dir)))
	    {
	      sprintf(fileName, "%s%c%s", dirName, DIR_SEP, file->d_name);
	      if (stat(fileName, &info) == 0 && !(info.st_mode & S_IFDIR))
		{
		  list = list_conj(list, strdup(fileName), NULL, NULL, free);
		}
	    }
	  closedir(dir);
	}
      else
	{
	  logMessage(LOG_WARN, "%s is not a directory", dirName);
	}
      free(dirName);
      pos += n;
      if (searchPath[pos] == '\0')
	break;
      else
	pos++;
    }
  return list;
}

char * EXPORT_CALL
lou_findTable(const char * query)
{
  if (!tableIndex)
    {
      char * searchPath;
      List * tables;
      const char ** tablesArray;
      logMessage(LOG_WARN, "Tables have not been indexed yet. Indexing LOUIS_TABLEPATH.");
      searchPath = getTablePath();
      tables = listFiles(searchPath);
      tablesArray = (const char **)list_toArray(tables, NULL);
      lou_indexTables(tablesArray);
      free(searchPath);
      list_free(tables);
      free(tablesArray);
    }
  List * queryFeatures = parseQuery(query);
  int bestQuotient = 0;
  char * bestMatch = NULL;
  List * l;
  for (l = tableIndex; l; l = l->tail)
    {
      TableMeta * table = l->head;
      int q = matchFeatureLists(queryFeatures, table->features, 0);
      if (q > bestQuotient)
	{
	  bestQuotient = q;
	  bestMatch = strdup(table->name);
	}
    }
  if (bestMatch)
     {
       logMessage(LOG_INFO, "Best match: %s (%d)", bestMatch, bestQuotient);
       return bestMatch;
     }
  else
    {
      logMessage(LOG_INFO, "No table could be found for query '%s'", query);
      return NULL;
    }
}
