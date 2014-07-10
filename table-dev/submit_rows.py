import argparse
import csv
import fileinput
from utils import *

class UTF8Recoder:
    def __init__(self, reader):
        self.reader = reader
    def __iter__(self):
        return self
    def next(self):
        return self.reader.next().encode("utf-8")
        
class Reader:
    def __init__(self, files, encoding):
        self.reader = csv.reader(UTF8Recoder(fileinput.FileInput(files, openhook=fileinput.hook_encoded(encoding))),
                                 delimiter='\t', quoting=csv.QUOTE_NONE)
    def __iter__(self):
        return self
    def next(self):
        row = [unicode(s, "utf-8") for s in self.reader.next()]
        if not row or row[0].startswith("#"):
            return self.next()
        maybe_chunked_text, braille = row[1:3]
        maybe_chunked_text = to_lowercase(maybe_chunked_text)
        text, chunked_text = read_text(maybe_chunked_text)
        braille = braille if braille != "" else None
        exit_if_not(not chunked_text or validate_chunks(chunked_text))
        return {'text': text,
                'braille': braille,
                'chunked_text': chunked_text}

def main():
    parser = argparse.ArgumentParser(description="Submit entries to dictionary.")
    parser.add_argument('FILE', nargs='*', default=['-'], help="TSV file with entries")
    parser.add_argument('-d', '--dictionary', required=True, dest="DICTIONARY",
                        help="dictionary file")
    parser.add_argument('-t', '--table', required=True, dest="TABLE",
                        help="translation table to be used for converting words to lowercase")
    args = parser.parse_args()
    load_table(args.TABLE)
    conn, c = open_dictionary(args.DICTIONARY)
    reader = Reader(args.FILE, "utf-8")
    c.execute("CREATE TABLE IF NOT EXISTS dictionary (text TEXT PRIMARY KEY, braille TEXT, chunked_text TEXT, frequency INTEGER)")
    for row in reader:
        c.execute("INSERT OR IGNORE INTO dictionary VALUES (:text,:braille,:chunked_text,0)", row)
        if c.lastrowid == 0:
            c.execute("UPDATE dictionary SET braille=:braille,chunked_text=:chunked_text WHERE text=:text", row)
    conn.commit()
    conn.close()

if __name__ == "__main__": main()