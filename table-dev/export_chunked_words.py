import argparse
from itertools import chain, izip_longest
from utils import *

def main():
    parser = argparse.ArgumentParser(description="Export chunked words in patgen format.")
    parser.add_argument('-d', '--dictionary', required=True, dest="DICTIONARY", help="dictionary file")
    args = parser.parse_args()
    conn, c = open_dictionary(args.DICTIONARY)
    c.execute("SELECT chunked_text FROM dictionary WHERE chunked_text IS NOT NULL")
    for chunked_text, in c.fetchall():
        text, hyphen_string = parse_chunks(chunked_text)
        println("".join([x for x in chain(*izip_longest(text, ["0" if x == "x" else "-" if x == "1" else None
                                                               for x in hyphen_string]))
                         if x is not None]))
    conn.close()

if __name__ == "__main__": main()