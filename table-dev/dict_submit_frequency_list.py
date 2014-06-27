import argparse
import csv
import fileinput
import re
from utils import load_table, normalize_text, open_dictionary

class Reader:
    def __init__(self, files, encoding):
        self.reader = csv.reader(fileinput.FileInput(files, openhook=fileinput.hook_encoded(encoding)), delimiter='\t')
    def __iter__(self):
        return self
    def next(self):
        frequency, text = [unicode(s, "utf-8") for s in self.reader.next()]
        return {'text': normalize_text(text),
                'frequency': int(frequency)}

def main():
    parser = argparse.ArgumentParser(description='Submit frequency list to dictionary.')
    parser.add_argument('FILE', nargs='*', default=['-'], help="TSV file with frequencies in first column and words second column")
    parser.add_argument('-t', '--table', required=True, help="translation table to be used for converting words to lowercase", dest="TABLE")
    args = parser.parse_args()
    load_table(args.TABLE)
    conn, c = open_dictionary()
    reader = Reader(args.FILE, "utf-8")
    c.execute("CREATE TABLE IF NOT EXISTS dictionary (text TEXT PRIMARY KEY, braille TEXT, chunked_text TEXT, frequency INTEGER)")
    c.execute("UPDATE dictionary SET frequency = 0")
    for row in reader:
        c.execute("INSERT OR IGNORE INTO dictionary VALUES (:text,'',0,:frequency)", row)
        if c.lastrowid == 0:
            c.execute("SELECT frequency FROM dictionary where text=:text", row)
            row['frequency'] += c.fetchone()[0]
            c.execute("UPDATE dictionary SET frequency=:frequency WHERE text=:text", row)
    conn.commit()
    conn.close()

if __name__ == "__main__": main()