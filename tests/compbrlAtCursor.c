// https://github.com/liblouis/liblouis/issues/78

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "louis.h"

void
print_widechars(widechar * buf, int len)
{
  int i;
  for (i = 0; i < len; i++)
    printf("%c", buf[i]);
}

int
check_translation(const char *table, const char *input, int cursorPos, int mode, const char *expected)
{
  widechar *inbuf;
  widechar *outbuf;
  int inlen;
  int outlen;
  int i, rv = 0;
  int expectedlen = strlen(expected);
  inlen = strlen(input);
  outlen = inlen * 10;
  inbuf = malloc(sizeof(widechar) * inlen);
  outbuf = malloc(sizeof(widechar) * outlen);
  inlen = extParseChars(input, inbuf);
  if (!inlen)
    {
      printf("Cannot parse input string.\n");
      return 1;
    }
  if (!lou_translate(table, inbuf, &inlen, outbuf, &outlen,
		     NULL, NULL, NULL, NULL, &cursorPos, mode))
    {
      printf("Translation failed.\n");
      return 1;
    }
  for (i = 0; i < outlen && i < expectedlen && expected[i] == outbuf[i]; i++);
  if (i < outlen || i < expectedlen)
    {
      rv = 1;
      outbuf[outlen] = 0;
      printf("Input: '%s'\n", input);
      printf("Expected: '%s'\n", expected);
      printf("Received: '");
      print_widechars(outbuf, outlen);
      printf("'\n");
      if (i < outlen && i < expectedlen)
	{
	  printf("Diff: Expected '%c' but recieved '%c' in index %d\n",
		 expected[i], outbuf[i], i);
	}
      else if (i < expectedlen)
	{
	  printf("Diff: Expected '%c' but recieved nothing in index %d\n",
		 expected[i], i);
	}
      else
	{
	  printf("Diff: Expected nothing but recieved '%c' in index %d\n",
		 outbuf[i], i);
	}
    }
  free(inbuf);
  free(outbuf);
  lou_free();
  return rv;
}

int
main (int argc, char **argv)
{
  const char* table = "en-us-comp8.ctb,braille-patterns.cti";
  const char* input = "héllo";
  const char* expected = "h'\\x00e9'llo";
  translationModes mode = compbrlAtCursor;
  int result = 0;
  result |= check_translation(table, input, 0, mode, expected);
  result |= check_translation(table, input, 1, mode, expected);
  return result;
}
