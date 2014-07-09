import argparse
from utils import *

def main():
    parser = argparse.ArgumentParser(description="Suggest dictionary rows and liblouis rules")
    parser.add_argument('-d', '--dictionary', required=True, dest="DICTIONARY",
                        help="dictionary file")
    parser.add_argument('-t', '--table', required=True, dest="TABLE",
                        help="translation table including hyphenation patterns")
    parser.add_argument('-n', type=int, default=10, dest="MAX_PROBLEMS",
                        help="maximal number of suggestions")
    parser.add_argument('-x', type=bool, default=False, dest="AUTO_CHUNK",
                        help="don't ask for updating chunks")
    args = parser.parse_args()
    println("# -*- tab-width: 30; -*-")
    conn, c = open_dictionary(args.DICTIONARY)
    c.execute("SELECT text,braille,chunked_text FROM dictionary WHERE braille IS NOT NULL OR chunked_text IS NOT NULL ORDER BY frequency DESC")
    load_table(args.TABLE)
    problems = 0
    auto_chunked = 0
    correct = 0
    for text, braille, chunked_text in c.fetchall():
        problem = False
        comments = []
        suggest_rows = []
        suggest_rules = []
        chunk_errors = None
        if braille:
            real_braille, applied_rules = translate(args.TABLE, text)
            if real_braille != braille:
                problem = True
                comments.append("wrong braille\t\t" + real_braille)
                if chunked_text:
                    _, expected_hyphen_string = parse_chunks(chunked_text)
                    real_hyphen_string = hyphenate(args.TABLE, text)
                    chunk_errors = compare_chunks(expected_hyphen_string, real_hyphen_string, text)
                    if chunk_errors:
                        comments.append("wrong chunks\t" + chunk_errors)
                if not is_letter(text):
                    comments.append("word doesn't consist of all letters")
                    suggest_rows.append({"text": text, "braille": braille})
                    suggest_rules.append({"opcode": "word", "text": text, "braille": braille})
                elif not chunk_errors:
                    suggested_hyphen_string = suggest_chunks(text, braille)
                    if suggested_hyphen_string:
                        if not chunked_text:
                            expected_hyphen_string = "x" * (len(text) - 1)
                            real_hyphen_string = hyphenate(args.TABLE, text)
                        chunk_errors = compare_chunks(suggested_hyphen_string, real_hyphen_string, text)
                    if suggested_hyphen_string and suggested_hyphen_string != expected_hyphen_string and chunk_errors:
                        suggested_chunked_text = print_chunks(text, suggested_hyphen_string)
                        if (args.AUTO_CHUNK):
                            c.execute("UPDATE dictionary SET chunked_text=:chunked_text WHERE text=:text",
                                      {"text": text, "chunked_text": suggested_chunked_text})
                            problem = False
                            auto_chunked += 1
                        else:
                            suggest_rows.append({"text": suggested_chunked_text, "braille": braille})
                            comments.append("wrong chunks\t" + chunk_errors)
                    else:
                        comments.append("applied rules")
                        applied_rules = [get_rule(x) for x in applied_rules if x is not None]
                        for rule in applied_rules:
                            comments.append("> " + "\t".join(rule))
                        suggest_rules.append({"opcode": "word", "text": text, "braille": braille})
                        other_possible_matches = set(find_possible_matches(text, braille)) - set(applied_rules)
                        if other_possible_matches:
                            comments.append("other possible matches")
                            for rule in other_possible_matches:
                                comments.append("> " + "\t".join(rule))
        if problem:
            println("# >>>\t%s\t%s" % (chunked_text or text, braille or ""))
            for comment in comments:
                println("# | " + comment)
            println("# |__")
            if suggest_rows:
                for row in suggest_rows:
                    println("#\t%(text)s\t%(braille)s" % row)
            if suggest_rules:
                for rule in suggest_rules:
                    println("# %(opcode)s\t%(text)s\t%(braille)s" % rule)
            println()
            problems += 1
            if problems >= args.MAX_PROBLEMS:
                break
        else:
            correct += 1
    if correct > 0:
        println("### %d words translated correctly" % (correct - auto_chunked))
    if auto_chunked > 0:
        println("### %d words automatically chunked" % auto_chunked)
        conn.commit()
    conn.close()

if __name__ == "__main__": main()