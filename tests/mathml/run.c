#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "unistr.h"
#include "liblouis.h"
#include "internal.h"
#ifndef SKIP_XSLT_TESTS
#include <libxml/parser.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#endif

#define EXIT_SKIPPED 77

int
main(int argc, char **argv)
{
#ifdef SKIP_XSLT_TESTS
	fprintf(stderr, "Skipping tests for %s as libyaml was not found\n", argv[2]);
	return EXIT_SKIPPED;
#else

	char *mathmlFile;
	xmlDocPtr mathmlDoc;
	xsltStylesheetPtr stylesheet;
	xmlDocPtr linearMathDoc;
	xmlChar *linearMath;
	uint8_t *braille = NULL;
	char *expectedBrailleFile = NULL;
	int ret = 0;

	// parse MathML input
	const char *testDir = argv[2];
	const char *mathmlFileName = "input.xml";
	mathmlFile = malloc(sizeof(char) * (strlen(testDir) + strlen(mathmlFileName) + 2));
	sprintf(mathmlFile, "%s%c%s", testDir, DIR_SEP, mathmlFileName);
	mathmlDoc = xmlParseFile(mathmlFile);

	// transform to linear math notation
	const xmlChar *stylesheetFile = (const xmlChar *)"mathml/mathml.xsl";
	stylesheet = xsltParseStylesheetFile(stylesheetFile);
	linearMathDoc = xsltApplyStylesheet(stylesheet, mathmlDoc, NULL);
	int linearMathLen;
	if (xsltSaveResultToString(&linearMath, &linearMathLen, linearMathDoc, stylesheet) < 0) {
		fprintf(stderr, "Transformation from MathML to linear math notation failed\n");
		ret = 1;
		goto cleanup;
	}

	// translate to braille
	widechar linearMathBuf[MAXSTRING];
	int linearMathBufLen = _lou_extParseChars((char *)linearMath, linearMathBuf);
	const char *table = argv[1];
	const char *displayTable = "tables/unicode.dis";
	widechar brailleBuf[MAXSTRING];
	int brailleBufLen = MAXSTRING;
	if (!_lou_translate(table, displayTable, linearMathBuf, &linearMathBufLen, brailleBuf, &brailleBufLen,
	                    NULL, NULL, NULL, NULL, NULL, 0, NULL, NULL)) {
		fprintf(stderr, "Translation from linear math notation to braille failed\n");
		ret = 1;
		goto cleanup;
	}
	size_t brailleLen;
#ifdef WIDECHARS_ARE_UCS4
	braille = u32_to_u8(brailleBuf, brailleBufLen, NULL, &brailleLen);
#else
	braille = u16_to_u8(brailleBuf, brailleBufLen, NULL, &brailleLen);
#endif

	// compare with expected braille
	const char *expectedBrailleFileName = "expected.txt";
	expectedBrailleFile = malloc(sizeof(char) * (strlen(testDir) + strlen(expectedBrailleFileName) + 2));
	sprintf(expectedBrailleFile, "%s%c%s", testDir, DIR_SEP, expectedBrailleFileName);
	FILE *expectedBrailleFilePtr;
	if ((expectedBrailleFilePtr = fopen(expectedBrailleFile, "r")) == NULL) {
		fprintf(stderr, "Can not read file: %s\n", expectedBrailleFile);
		ret = 1;
		goto cleanup;
	}
	char expectedBrailleUTF8[MAXSTRING];
	int k = 0;
	if ((expectedBrailleUTF8[k++] = fgetc(expectedBrailleFilePtr)) == '\357'
	    && (expectedBrailleUTF8[k++] = fgetc(expectedBrailleFilePtr)) == '\273'
	    && (expectedBrailleUTF8[k++] = fgetc(expectedBrailleFilePtr)) == '\277') {
		// skip bom
		k = 0;
	}
	while (1) {
		char c = expectedBrailleUTF8[k - 1];
		if (c == '\n' || c == EOF) {
			expectedBrailleUTF8[k - 1] = '\0';
			break;
		}
		if (k == MAXSTRING) {
			fprintf(stderr, "File is too long: %s\n", expectedBrailleFile);
			ret = 1;
			goto cleanup;
		}
		expectedBrailleUTF8[k++] = fgetc(expectedBrailleFilePtr);
	}
	fclose(expectedBrailleFilePtr);
	char brailleUTF8[MAXSTRING];
	snprintf(brailleUTF8, MAXSTRING, "%.*s", (int)brailleLen, braille);
	if (strcmp(expectedBrailleUTF8, brailleUTF8)) {
		fprintf(stderr, "Linear math: %s\n", linearMath);
		fprintf(stderr, "Braille differs:\n");
		fprintf(stderr, "- expected: %s\n", expectedBrailleUTF8);
		fprintf(stderr, "- got:      %s\n", brailleUTF8);
		ret = 1;
		goto cleanup;
	}

	cleanup:
	free(mathmlFile);
	xmlFreeDoc(mathmlDoc);
	xsltFreeStylesheet(stylesheet);
	xmlFreeDoc(linearMathDoc);
	free(linearMath);
	xsltCleanupGlobals();
	xmlCleanupParser();
	lou_free();
	free(braille);
	free(expectedBrailleFile);
	return ret;

#endif
}
