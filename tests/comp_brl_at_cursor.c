#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "liblouis.h"

#define TRANTAB "../tables/en-us-g2.ctb"

int 
check_cursor_pos(const char *str, const int *expected_pos)
{
        widechar *inbuf;
        widechar *outbuf;
        int orig_inlen;
        int outlen;
        int cursor_pos;
        int i, rv=0;

        orig_inlen = strlen(str);
        outlen = orig_inlen;
        inbuf = malloc(sizeof(widechar)*orig_inlen);
        outbuf = malloc(sizeof(widechar)*orig_inlen);
        for (i=0;i<orig_inlen;i++) 
        {
                inbuf[i] = str[i];
        }
        
        for (i = 0; i < orig_inlen; i++) 
        {
				int inlen = orig_inlen;
                cursor_pos = i;
                lou_translate(TRANTAB, inbuf, &inlen, outbuf, &outlen, 
                              NULL, NULL, NULL, NULL,
                              &cursor_pos, compbrlAtCursor);
				if (expected_pos[i] != cursor_pos) {
						rv = 1;
						printf(
								"string='%s' cursor=%d ('%c') expected=%d recieved=%d ('%c')\n", 
							   str, i, str[i], 
								expected_pos[i], 
								cursor_pos,
								(char) outbuf[cursor_pos]);
								}
        }
        
        free(inbuf);
        free(outbuf);
		lou_free();

		return rv;
}
