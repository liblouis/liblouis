#define TRANSLATION_TABLE "en-us-g2.ctb"

int check_outpos(const char *str, const int *expected_poslist);

/* Check if the cursor position is where you expect it to be after
   translating str. Return 0 if the translation is as expected and 1
   otherwise. */
int check_cursor_pos(const char *str, const int *expected_pos);

/* Check if a string is translated as expected. Return 0 if the
   translation is as expected and 1 otherwise. */
int check_translation(const char *tableList, const char *str,
		      const char *typeform, const char *expected);
