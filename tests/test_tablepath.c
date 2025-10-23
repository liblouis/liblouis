#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "liblouis/internal.h"

int main(void) {
    /* create a very long env var (> 8k) to exercise path construction */
    size_t len = 9000;
    char *buf = malloc(len + 1);
    if (!buf) return 2;
    memset(buf, 'A', len);
    buf[len] = '\0';
    setenv("LOUIS_TABLEPATH", buf, 1);
    free(buf);

    char *p = _lou_getTablePath();
    if (!p) {
        printf("_lou_getTablePath returned NULL\n");
        return 1;
    }
    printf("OK: length=%zu\n", strlen(p));
    /* print prefix */
    for (int i = 0; i < 80 && p[i]; i++) putchar(p[i]);
    putchar('\n');
    free(p);
    return 0;
}
