/* -*- tab-width: 4; -*- */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "louis.h"

static const TranslationTableHeader *table;

extern void loadTable(const char *tableList) {
	table = lou_getTable(tableList);
}

extern int isLetter(widechar c) {
	static unsigned long int hash;
	static TranslationTableOffset offset;
	static TranslationTableCharacter *character;
	hash = (unsigned long int) c % HASHNUM;
	offset = table->characters[hash];
	while (offset) {
		character = (TranslationTableCharacter *)&table->ruleArea[offset];
		if (character->realchar == c)
			return character->attributes & CTC_Letter;
		offset = character->next; }
	return 0;
}

extern widechar toLowercase(widechar c) {
	static unsigned long int hash;
	static TranslationTableOffset offset;
	static TranslationTableCharacter *character;
	hash = (unsigned long int) c % HASHNUM;
	offset = table->characters[hash];
	while (offset) {
		character = (TranslationTableCharacter *)&table->ruleArea[offset];
		if (character->realchar == c)
			return character->lowercase;
		offset = character->next; }
	return c;
}

extern void toDotPattern(widechar *braille, char *pattern) {
	int length;
	widechar *dots;
	int i;
	for (length = 0; braille[length]; length++)
		;
	dots = (widechar *)malloc((length + 1) * sizeof(widechar));
	for (i = 0; i < length; i++)
		dots[i] = getDotsForChar(braille[i]);
	strcpy(pattern, showDots(dots, length));
	free(dots);
}

extern int printRule(TranslationTableRule *rule, widechar *rule_string) {
	int k, l;
	switch (rule->opcode) {
	case CTO_Context:
	case CTO_Correct:
	case CTO_SwapCd:
	case CTO_SwapDd:
	case CTO_Pass2:
	case CTO_Pass3:
	case CTO_Pass4:
		return 0;
	default:
		l = 0;
		const char *opcode = findOpcodeName(rule->opcode);
		for (k = 0; k < strlen(opcode); k++)
			rule_string[l++] = opcode[k];
		rule_string[l++] = ' ';
		for (k = 0; k < rule->charslen; k++)
			rule_string[l++] = rule->charsdots[k];
		rule_string[l++] = ' ';
		for (k = 0; k < rule->dotslen; k++)
			rule_string[l++] = getCharFromDots(rule->charsdots[rule->charslen + k]);
		rule_string[l++] = '\0';
		return 1; }
}

#define DEBUG 0

#if DEBUG
#define debug(fmt, ...) do {                                 \
  if (DEBUG)                                                 \
    printf("%*s" fmt "\n", debug_indent, "", ##__VA_ARGS__); \
} while(0)
#else
#define debug(fmt, ...)
#endif

static int find_matching_rules(widechar *text,
                               int text_len,
                               widechar *braille,
                               int braille_len,
                               char *data) {
	unsigned long int hash;
	TranslationTableOffset offset;
	TranslationTableRule *rule;
	TranslationTableCharacter *character;
	char *data_save;
	int hash_len, k;
#if DEBUG
	static int initial_text_len = 0;
	int debug_indent = 0;
	if (initial_text_len == 0) {
		initial_text_len = text_len;
		for (k = 0; k < text_len; k++)
			printf("%c", text[k]);
		printf(" <=> ");
		for (k = 0; k < braille_len; k++)
			printf("%c", braille[k]);
		printf("\n"); }
	else
		debug_indent = initial_text_len - text_len;
#endif
	
	/* finish */
	if (text_len == 0 && braille_len == 0) {
		if (data[-1] != '1')
			return 0;
		data[-1] = '$';
		return 1; }
	
	/* save data */
	data_save = (char *)malloc(text_len * sizeof(char));
	memcpy(data_save, data, text_len);
	
	/* iterate over rules */
	for (hash_len = 2; hash_len >= 1; hash_len--) {
		offset = 0;
		switch (hash_len) {
		case 2:
			if (text_len < 2)
				break;
			hash = (unsigned long int) toLowercase(text[0]) << 8;
			hash += (unsigned long int) toLowercase(text[1]);
			hash %= HASHNUM;
			offset = table->forRules[hash];
			break;
		case 1:
			hash = (unsigned long int) text[0] % HASHNUM;
			offset = table->characters[hash];
			while (offset) {
				character = (TranslationTableCharacter *)&table->ruleArea[offset];
				if (character->realchar == text[0]) {
					offset = character->otherRules;
					break; }
				else
					offset = character->next; }}
		while (offset) {
			rule = (TranslationTableRule *)&table->ruleArea[offset];
#if DEBUG
			widechar print_string[128];
			printRule(rule, print_string);
			printf("%*s=> ", debug_indent, "");
			for (k = 0; print_string[k]; k++)
				printf("%c", print_string[k]);
			printf("\n");
#endif
			
			/* select rule */
			if (rule->charslen == 0 || rule->dotslen == 0)
				goto next_rule;
			if (rule->charslen > text_len)
				goto next_rule;
			switch (rule->opcode) {
			case CTO_WholeWord:
				if (data[-1] == '^' && rule->charslen == text_len)
					break;
				goto next_rule;
			case CTO_BegWord:
				if (data[-1] == '^')
					break;
				goto next_rule;
			case CTO_EndWord:
				if (rule->charslen == text_len)
					break;
				goto next_rule;
			case CTO_Syllable_:
				if (data[-1] != '1' && data[-1] != '^')
					goto next_rule;
			case CTO_NoCross:
				for (k = 0; k < rule->charslen - 1; k++)
					if (data[k + 1] == '>')
						goto next_rule;
				break;
			case CTO_Letter:
			case CTO_UpperCase:
			case CTO_LowerCase:
			case CTO_Punctuation:
				break;
			default:
				goto next_rule; }
			for (k = 0; k < rule->charslen; k++)
				if (rule->charsdots[k] != text[k])
					goto next_rule;
			debug("** rule selected **");
			
			/* inhibit rule */
			if (rule->dotslen > braille_len
			    || rule->charslen == text_len && rule->dotslen < braille_len
			    || rule->dotslen == braille_len && rule->charslen < text_len)
				goto inhibit;
			for (k = 0; k < rule->dotslen; k++)
				if (getCharFromDots(rule->charsdots[rule->charslen + k]) != braille[k])
					goto inhibit;
			
			/* fill data */
			switch (rule->opcode) {
			case CTO_NoCross:
			case CTO_Syllable_: // deferred: see success
				break;
			default:
				k = 0;
				while (k < rule->charslen - 1) {
					if (data[k + 1] == '>' || data[k + 1] == '|') {
						data[k++] = '1';
						memset(&data[k], '-', text_len - k);
						break; }
					else if (data[k] == 's')
						data[k++] = '0';
					else
						data[k++] = 'x'; }}
			if (rule->opcode == CTO_Syllable_ || data[rule->charslen] == '>' || data[rule->charslen] == '|') {
				data[rule->charslen - 1] = '1';
				memset(&data[rule->charslen], '-', text_len - rule->charslen); }
			else if (data[rule->charslen - 1] == '|')
				data[rule->charslen - 1] = '0';
			else
				data[rule->charslen - 1] = 'x';
			debug("%s", data);
			
			/* recur */
			switch (data[rule->charslen - 1]) {
			case 'x':
				data[rule->charslen - 1] = '0';
				debug("%s", data);
				if (find_matching_rules(&text[rule->charslen], text_len - rule->charslen,
				                        &braille[rule->dotslen], braille_len - rule->dotslen,
				                        &data[rule->charslen])) {
					char *data_tmp = (char *)malloc((text_len - rule->charslen) * sizeof(char));
					memcpy(data_tmp, &data[rule->charslen - 1], text_len - rule->charslen + 1);
					data[rule->charslen - 1] = '1';
					debug("%s", data);
					memset(&data[rule->charslen], '-', text_len - rule->charslen);
					if (find_matching_rules(&text[rule->charslen], text_len - rule->charslen,
					                        &braille[rule->dotslen], braille_len - rule->dotslen,
					                        &data[rule->charslen])
					    && memcmp(&data_tmp[1], &data[rule->charslen], text_len - rule->charslen) == 0)
						data[rule->charslen - 1] = 'x';
					else
						memcpy(&data[rule->charslen - 1], data_tmp, text_len - rule->charslen + 1);
					debug("%s", data);
					free(data_tmp);
					goto success; }
				else {
					data[rule->charslen - 1] = '1';
					memset(&data[rule->charslen], '-', text_len - rule->charslen);
					debug("%s", data); }
			case '0':
			case '1':
				if (find_matching_rules(&text[rule->charslen], text_len - rule->charslen,
				                        &braille[rule->dotslen], braille_len - rule->dotslen,
				                        &data[rule->charslen]))
					goto success; }
			
		  inhibit:
			debug("** rule inhibited **");
			switch (rule->opcode) {
			case CTO_NoCross:
				if (rule->charslen < 2)
					goto abort;
				data[rule->charslen - 1] = '>';
				debug("%s", data);
				goto next_rule;
			case CTO_Syllable_:
				data[rule->charslen - 1] = '|';
				debug("%s", data);
				goto next_rule;
			default:
				goto abort; }
			
		  success:
			/* fill data (deferred) */
			switch (rule->opcode) {
			case CTO_NoCross:
			case CTO_Syllable_:
				memset(data, '0', rule->charslen - 1);
				debug("%s", data); }
			return 1;
			
		  next_rule:
			offset = rule->charsnext; }}
	
  abort:
	/* restore data */
	memcpy(data, data_save, text_len);
	free(data_save);
	debug("** abort **");
	return 0;
}

extern int suggestChunks(widechar *text, widechar *braille, char *hyphen_string) {
	int text_len, braille_len;
	for (text_len = 0; text[text_len]; text_len++)
		;
	for (braille_len = 0; braille[braille_len]; braille_len++)
		;
	if (text_len == 0 || braille_len == 0)
		return 0;
	hyphen_string[0] = '^';
	hyphen_string[text_len + 1] = '\0';
	memset(&hyphen_string[1], '-', text_len);
	return find_matching_rules(text, text_len, braille, braille_len, &hyphen_string[1]);
}

extern void findRelevantRules(widechar *text, widechar **rules_str) {
	int text_len, rules_len;
	unsigned long int hash;
	TranslationTableOffset offset;
	TranslationTableCharacter *character;
	TranslationTableRule *rule;
	TranslationTableRule **rules;
	int hash_len, k, l, m, n;
	for (text_len = 0; text[text_len]; text_len++)
		;
	for (rules_len = 0; rules_str[rules_len]; rules_len++)
		;
	rules = (TranslationTableRule **)malloc((rules_len + 1) * sizeof(TranslationTableRule *));
	m = n = 0;
	while (text[n]) {
		for (hash_len = 2; hash_len >= 1; hash_len--) {
			offset = 0;
			switch (hash_len) {
			case 2:
				if (text_len - n < 2)
					break;
				hash = (unsigned long int) toLowercase(text[n]) << 8;
				hash += (unsigned long int) toLowercase(text[n+1]);
				hash %= HASHNUM;
				offset = table->forRules[hash];
				break;
			case 1:
				hash = (unsigned long int) text[n] % HASHNUM;
				offset = table->characters[hash];
				while (offset) {
					character = (TranslationTableCharacter *)&table->ruleArea[offset];
					if (character->realchar == text[0]) {
						offset = character->otherRules;
						break; }
					else
						offset = character->next; }}
			while (offset) {
				rule = (TranslationTableRule *)&table->ruleArea[offset];
				switch (rule->opcode) {
				case CTO_WholeWord:
				case CTO_BegWord:
				case CTO_EndWord:
				case CTO_NoCross:
				case CTO_Syllable_:
					break;
				default:
					goto next_rule; }
				if (rule->charslen == 0
				    || rule->dotslen == 0
				    || rule->charslen > text_len - n)
					goto next_rule;
				for (k = 0; k < rule->charslen; k++)
					if (rule->charsdots[k] != text[n+k])
						goto next_rule;
				rules[m++] = rule;
				if (m == rules_len)
					goto finish;
			  next_rule:
				offset = rule->charsnext; }}
		n++; }
  finish:
	rules_str[m--] = NULL;
	for (; m >= 0; m--)
		printRule(rules[m], rules_str[m]);
	free(rules);
}

