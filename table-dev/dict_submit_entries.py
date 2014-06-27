import argparse
import csv
import fileinput
import re
import sys
from utils import normalize_text, validate_chunks, open_dictionary, load_table

class Reader:
    def __init__(self, files, encoding):
        self.reader = csv.reader(fileinput.FileInput(files, openhook=fileinput.hook_encoded(encoding)), delimiter='\t')
    def __iter__(self):
        return self
    def next(self):
        is_comment, text, braille = [unicode(s, "utf-8") for s in self.reader.next()][:3]
        if is_comment == "#":
            return self.next()
        if is_comment != "":
            exit(1)
        chunked = re.search("[][|]", text) != None
        if chunked:
            chunked_text = text
            text = re.sub("[][|]", "", chunked_text)
            if not validate_chunks(chunked_text):
                exit(1)
        return {'text': normalize_text(text),
                'braille': braille if braille != "" else None,
                'chunked_text': chunked_text if chunked else None}

def main():
    parser = argparse.ArgumentParser(description='Submit entries to dictionary.')
    parser.add_argument('FILE', nargs='*', default=['-'], help="TSV file with entries")
    parser.add_argument('-t', '--table', required=True, help="translation table to be used for converting words to lowercase", dest="TABLE")
    args = parser.parse_args()
    load_table(args.TABLE)
    conn, c = open_dictionary()
    reader = Reader(args.FILE, "utf-8")
    c.execute("CREATE TABLE IF NOT EXISTS dictionary (text TEXT PRIMARY KEY, braille TEXT, chunked_text TEXT, frequency INTEGER)")
    for row in reader:
        c.execute("INSERT OR IGNORE INTO dictionary VALUES (:text,:braille,:chunked_text,0)", row)
        if c.lastrowid == 0:
            c.execute("UPDATE dictionary SET braille=:braille,chunked_text=:chunked_text WHERE text=:text", row)
    conn.commit()
    conn.close()

if __name__ == "__main__": main()