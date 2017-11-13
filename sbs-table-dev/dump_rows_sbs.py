import argparse
from utils import *

def main():
    parser = argparse.ArgumentParser(description="Dump dictionary to CSV format.")
    parser.add_argument('-d', '--dictionary', required=True, dest="DICTIONARY", help="dictionary file")
    args = parser.parse_args()
    conn, c = open_dictionary(args.DICTIONARY)
    c.execute("SELECT text,braille FROM dictionary WHERE braille IS NOT NULL")
    for text, braille in c.fetchall():
        println("*\t%s\t%s\t" % (text, braille))

if __name__ == "__main__": main()
