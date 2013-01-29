#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "liblouis.h"
#include "louis.h"

static int compare_widechars(const widechar* buf1, const widechar* buf2, int len) {
    int i;
    for (i = 0; i < len; i++)
        if (buf1[i] != buf2[i]) return 1;
    return 0;
}

static int compare_chars(const char* buf1, const char* buf2, int len) {
    int i;
    for (i = 0; i < len; i++)
        if (buf1[i] != buf2[i]) return 1;
    return 0;
}

static void print_widechars(const widechar* buf, int len) {
    int i;
    for (i = 0; i < len; i++)
        printf("%c", buf[i]);
}

static void print_chars(const char* buf, int len) {
    int i;
    for (i = 0; i < len; i++)
        printf("%c", buf[i]);
}

#define fail() { \
    result = 1;  \
    goto free;   \
}

#define fail_if(condition, ...) { \
    if (condition) {              \
        printf(__VA_ARGS__);      \
        printf("\n");             \
        fail(); }                 \
}

static int check_hyphenation(const char* table, const char* text) {

    int result = 0;
    int textlen = strlen(text);
    int brllen1 = textlen * 3;
    int brllen2 = textlen * 3;
    widechar* textbuf = malloc(textlen * sizeof(widechar));
    widechar* brlbuf1 = malloc(brllen1 * sizeof(widechar));
    widechar* brlbuf2 = malloc(brllen2 * sizeof(widechar));
    char* texthyph = malloc(textlen * sizeof(char));
    char* brlhyph1 = malloc(brllen1 * sizeof(char));
    char* brlhyph2 = malloc(brllen2 * sizeof(char));
    textlen = extParseChars(text, textbuf);

    /* the easy way */
    fail_if(
        !lou_hyphenate(table, textbuf, textlen, texthyph, 0),
        "Hyphenation failed!");
    fail_if(
        !lou_translatePrehyphenated(table, textbuf, &textlen, brlbuf1, &brllen1, NULL, NULL, NULL, NULL, NULL, texthyph, brlhyph1, 0),
        "Translation failed!");

    /* the hard way */
    fail_if(
        !lou_translate(table, textbuf, &textlen, brlbuf2, &brllen2, NULL, NULL, NULL, NULL, NULL, 0),
        "Translation failed!");
    fail_if(
        !lou_hyphenate(table, brlbuf2, brllen2, brlhyph2, 1),
        "Hyphenation failed!");

    /* compare results */
    fail_if(
        brllen1 != brllen2,
        "Output lengths not equal: %d vs. %d", brllen1, brllen2);

    if (compare_widechars(brlbuf1, brlbuf2, brllen1)) {
        printf("Braille outputs not equal: ");
        print_widechars(brlbuf1, brllen1);
        printf(" vs. ");
        print_widechars(brlbuf2, brllen2);
        printf("\n");
        fail(); }

    if (compare_chars(brlhyph1, brlhyph2, brllen1)) {
        printf("Hyphenation points not equal: ");
        print_chars(brlhyph1, brllen1);
        printf(" vs. ");
        print_chars(brlhyph2, brllen2);
        printf("\n");
        fail(); }

    free:
        free(textbuf);
        free(brlbuf1);
        free(brlbuf2);
        free(texthyph);
        free(brlhyph1);
        free(brlhyph2);

    return result;
}

int main(int argc, char **argv) {
    int result = 0;
    result = result | check_hyphenation("en-us-g1.utb,hyph_en_US.dic", "SomeExampleToBeHyphenated");
    return result;
}
