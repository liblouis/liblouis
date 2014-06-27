from ctypes import c_wchar, c_char_p, c_wchar_p
from louis import _loader, liblouis
import os
import sqlite3
import sys

assert liblouis.lou_charSize() == 4

liblouis_dev = _loader["liblouis-table-dev.so.0"]
liblouis_dev.loadTable.argtypes = (c_char_p,)
liblouis_dev.toLowercase.restype = c_wchar
liblouis_dev.toLowercase.argtypes = (c_wchar,)
liblouis_dev.suggestChunks.argtypes = (c_wchar_p,c_wchar_p)

def validate_chunks(chunked_text):
    in_nocross = False
    prev_c = None
    for c in chunked_text:
        if c == '[':
            if in_nocross or prev_c == '|':
                return False
            in_nocross = True
        elif c == ']':
            if not in_nocross:
                return False
            in_nocross = False
        elif c == '|':
            if in_nocross or prev_c == None or prev_c == ']' or prev_c == '|':
                return False
        prev_c = c
    if in_nocross or prev_c == '|':
        return False
    return True

def normalize_text(text):
    return "".join([liblouis_dev.toLowercase(c) for c in text])

def open_dictionary():
    dirname, _ = os.path.split(os.path.abspath(sys.argv[0]))
    dictionary = os.path.join(dirname, "dictionary.sqlite")
    conn = sqlite3.connect(dictionary)
    c = conn.cursor()
    return conn, c

def load_table(table):
    liblouis_dev.loadTable(table);

def hyphenate(table, text):
    return text