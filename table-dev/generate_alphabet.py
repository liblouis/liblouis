import argparse
from itertools import groupby
from utils import *

def main():
    parser = argparse.ArgumentParser(description="Generate translate file for patgen.")
    parser.add_argument('-d', '--dictionary', required=True, dest="DICTIONARY",
                        help="dictionary file")
    parser.add_argument('-t', '--table', required=True, dest="TABLE",
                        help="translation table to be used for converting letters to lowercase")
    args = parser.parse_args()
    load_table(args.TABLE)
    conn, c = open_dictionary(args.DICTIONARY)
    c.execute("SELECT text FROM dictionary WHERE chunked_text IS NOT NULL")
    alphabet = ""
    for text, in c.fetchall():
        alphabet = "".join(set(alphabet + text))
    print("")
    for k, g in groupby(sorted(alphabet, key=to_lowercase), to_lowercase):
        g = list(g)
        g.remove(k)
        println(" " + " ".join([k] + g))
    conn.close()

if __name__ == "__main__": main()