import argparse
from ctypes import create_unicode_buffer
import sys
from utils import liblouis_dev, load_table

def main():
    parser = argparse.ArgumentParser(description='Suggest how a word should be chunked in order to obtain the given translation')
    parser.add_argument('TEXT', help="word to be chunked")
    parser.add_argument('BRAILLE', help="expected braille translation")
    parser.add_argument('-t', '--table', required=True, help="translation table (should not contain any hyphenation patterns)", dest="TABLE")
    args = parser.parse_args()
    load_table(args.TABLE)
    text = create_unicode_buffer(args.TEXT, len(args.TEXT) * 2)
    braille = create_unicode_buffer(args.BRAILLE)
    if not liblouis_dev.suggestChunks(text, braille):
        return 1;
    print("#\t%s\t%s" % (text.value, args.BRAILLE))

if __name__ == "__main__": main()