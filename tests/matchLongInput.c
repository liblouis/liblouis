/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2026 Anthony Tibbs

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include "liblouis.h"
#include "brl_checks.h"

/**
 * The pattern matcher behind the match opcode recurses once per loop
 * iteration. A match rule whose pattern loops over letters used to
 * overflow the stack when the input contained a long run of letters.
 */
int
main(int argc, char **argv)
{
  const char *table = "tests/tables/match-loop.ctb";
  int result = 0;
  /* Sanity check: the match rule applies to short input. */
  result |= check(table, "aa.aa", "aa'aa");
  /* A run of letters far longer than any sane stack can hold frames
   * for. The match rule is allowed not to apply, but translation must
   * succeed without crashing. */
  int n = 500000;
  widechar *in = malloc((2 * n + 1) * sizeof(widechar));
  widechar *out = malloc((2 * n + 1) * sizeof(widechar));
  int inlen = 2 * n + 1;
  int outlen = 2 * n + 1;
  for (int i = 0; i < n; i++) in[i] = 'a';
  in[n] = '.';
  for (int i = 0; i < n; i++) in[n + 1 + i] = 'a';
  if (!lou_translateString(table, in, &inlen, out, &outlen, NULL, NULL, 0)) {
    fprintf(stderr, "translation of long input failed\n");
    result = 1;
  } else if (outlen != 2 * n + 1 || out[0] != 'a' || out[2 * n] != 'a') {
    fprintf(stderr, "unexpected translation of long input\n");
    result = 1;
  }
  free(in);
  free(out);
  lou_free();
  return result;
}
