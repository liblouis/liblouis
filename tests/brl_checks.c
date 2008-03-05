#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "liblouis.h"

#define TRANTAB "../tables/en-us-g2.ctb"

void
print_int_array(const char *prefix, int* pos_list, int len)
{
		int i;
		printf("%s ", prefix);
		for (i=0; i<len; i++)
				printf("%d ", pos_list[i]);
		printf("\n");
}

void
print_widechars(widechar *buf, int len)
{
		int i;
		for (i=0; i<len; i++)
				printf("%c", buf[i]);
		printf("\n");	
}

int
check_outpos(const char *str, const int *expected_poslist)
{
        widechar *inbuf;
        widechar *outbuf;
		int *inpos, *outpos;
        int inlen;
        int outlen;
        int cursor_pos;
        int i, rv=0;

        inlen = strlen(str)*2;
        outlen = inlen;
        inbuf = malloc(sizeof(widechar)*inlen);
        outbuf = malloc(sizeof(widechar)*outlen);
		inpos = malloc(sizeof(int)*inlen);
		outpos = malloc(sizeof(int)*inlen);
        for (i=0;i<inlen;i++) 
        {
                inbuf[i] = str[i];
        }
		lou_translate(TRANTAB, inbuf, &inlen, outbuf, &outlen, 
					  NULL, NULL, outpos, inpos,
					  NULL, 0);
		for (i=0;i<outlen;i++)
		{
				if (expected_poslist[i] != inpos[i]) {
						rv = 1;
						printf("Expected %d, recieved %d in index %d\n",
							   expected_poslist[i], inpos[i], i);
				}
		}

        free(inbuf);
        free(outbuf);
		free(inpos);
		free(outpos);
		lou_free();
		return rv;

}

int 
check_cursor_pos(const char *str, const int *expected_pos)
{
        widechar *inbuf;
        widechar *outbuf;
		int *inpos, *outpos;
        int orig_inlen;
        int outlen;
        int cursor_pos;
        int i, rv=0;

        orig_inlen = strlen(str);
        outlen = orig_inlen;
        inbuf = malloc(sizeof(widechar)*orig_inlen);
        outbuf = malloc(sizeof(widechar)*orig_inlen);
		inpos = malloc(sizeof(int)*orig_inlen);
		outpos = malloc(sizeof(int)*orig_inlen);
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
						printf("string='%s' cursor=%d ('%c') expected=%d \
recieved=%d ('%c')\n", 
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
