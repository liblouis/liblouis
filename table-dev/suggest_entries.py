import argparse
from louis import translate, hyphenate
import re
import sys
from utils import open_dictionary

def compare_chunks(expected_chunked_text, real_hyphen_positions):
    str_list = []
    in_nocross = False
    prev_c = None
    errors = False
    k = 0
    for c in expected_chunked_text:
        if c == '[':
            if in_nocross or prev_c == '|':
                exit(1)
            in_nocross = True
            if prev_c != None:
                if real_hyphen_positions[k] == '0':
                    str_list.append('-')
                    errors = True
                else:
                    str_list.append('*')
        elif c == ']':
            if not in_nocross:
                exit(1)
            in_nocross = False
        elif c == '|':
            if in_nocross or prev_c == None or prev_c == ']' or prev_c == '|':
                exit(1)
            if real_hyphen_positions[k] == '0':
                str_list.append('-')
                errors = True
            else:
                str_list.append('*')
        else:
            if prev_c == ']':
                if real_hyphen_positions[k] == '0':
                    str_list.append('-')
                    errors = True
                else:
                    str_list.append('*')
            elif prev_c == '|' or prev_c == '[':
                pass
            elif prev_c == None:
                if real_hyphen_positions[k] != '0':
                    exit(1)
            else:
                if real_hyphen_positions[k] != '0':
                    str_list.append('.')
                    if in_nocross:
                        errors = True
            str_list.append(c)
            k += 1
        prev_c = c
    if in_nocross or prev_c == '|':
        exit(1)
    if len(real_hyphen_positions) != k:
        exit(1)
    if errors:
        return ''.join(str_list)
    else:
        return None

def main():
    parser = argparse.ArgumentParser(description='Suggest dictionary entries to work on')
    parser.add_argument('-t', '--table', required=True, help="translation table", dest="TABLE")
    parser.add_argument('-n', type=int, default=10, help="maximal number of entries to suggest", dest="N")
    args = parser.parse_args()
    conn, c = open_dictionary()
    c.execute("SELECT text,braille,chunked_text FROM dictionary WHERE braille IS NOT NULL OR chunked_text IS NOT NULL ORDER BY frequency DESC")
    n = 0
    for text, braille, chunked_text in c.fetchall():
        problems = []
        if braille:
            real_braille = translate(args.TABLE.split(","), text)[0]
            if real_braille != braille:
                problems.append("wrong braille: %s" % real_braille)
        if chunked_text:
            hyphen_positions = hyphenate(args.TABLE.split(","), text)
            chunk_errors = compare_chunks(chunked_text, hyphen_positions)
            if chunk_errors:
                problems.append("wrong chunks: %s" % chunk_errors)
        if problems:
            print("#\t%s\t%s\t# %s" % (chunked_text or text, braille or "", " # ".join(problems)))
            n += 1
            if n >= args.N:
                break
    conn.close()

if __name__ == "__main__": main()