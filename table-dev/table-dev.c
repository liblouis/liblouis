#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "louis.h"

static const TranslationTableHeader *table;

extern void loadTable(const char *tableList) {
	table = lou_getTable(tableList);
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

static int find_matching_rules(widechar *text,
                               int text_len,
                               widechar *braille,
                               int braille_len,
                               TranslationTableRule **rules) {
	unsigned long int hash;
	TranslationTableOffset offset;
	TranslationTableRule *rule;
	TranslationTableCharacter *character;
	int try, k, dobreak;
	for (try = 1; try <= 2; try++) {
		offset = 0;
		switch (try) {
		case 1:
			if (text_len < 2)
				break;
			hash = (unsigned long int) toLowercase(text[0]) << 8;
			hash += (unsigned long int) toLowercase(text[1]);
			hash %= HASHNUM;
			offset = table->forRules[hash];
			break;
		case 2:
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
			switch (rule->opcode) {
			case CTO_Letter:
			case CTO_UpperCase:
			case CTO_LowerCase:
			case CTO_NoCross:
				if (rule->charslen == 0
				    || rule->dotslen == 0
				    || rule->charslen > text_len
				    || rule->dotslen > braille_len
				    || rule->charslen == text_len && rule->dotslen < braille_len
				    || rule->dotslen == braille_len && rule->charslen < text_len)
					break;
				dobreak = 0;
				for (k = 0; k < rule->dotslen; k++)
					if (getCharFromDots(rule->charsdots[rule->charslen + k]) != braille[k]) {
						dobreak = 1;
						break; }
				if (dobreak) break;
				if (text_len == rule->charslen &&
				    braille_len == rule->dotslen) {
					rules[0] = rule;
					rules[1] = NULL;
					return 1; }
				if (find_matching_rules(&text[rule->charslen], text_len - rule->charslen,
				                        &braille[rule->dotslen], braille_len - rule->dotslen,
				                        &rules[1])) {
					rules[0] = rule;
					return 1; }}
			offset = rule->charsnext; }}
	return 0;
}

extern int suggestChunks(widechar *text, widechar *braille) {
	int text_len, braille_len;
	TranslationTableRule **rules;
	TranslationTableRule *rule;
	int k, l, m;
	for (text_len = 0; text[text_len]; text_len++)
		;
	for (braille_len = 0; braille[braille_len]; braille_len++)
		;
	if (text_len == 0 || braille_len == 0)
		return 0;
	rules = (TranslationTableRule **)malloc((text_len + 1) * sizeof(TranslationTableRule *));
	if (find_matching_rules(text, text_len, braille, braille_len, rules)) {
		k = l = m = 0;
		for (rule = rules[m++]; rule; rule = rules[m++]) {
			if (rule->opcode == CTO_NoCross)
				text[k++] = '[';
			for (l = 0; l < rule->charslen; l++)
				text[k++] = rule->charsdots[l];
			if (rule->opcode == CTO_NoCross)
				text[k++] = ']'; }
		text[k++] = 0;
		free(rules);
		return 1; }
	free(rules);
	return 0;
}
