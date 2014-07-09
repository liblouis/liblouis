from ctypes import *
from itertools import takewhile, izip_longest, chain, tee
from louis import _loader, liblouis, outlenMultiplier                    , createStr
import re
import sqlite3
import sys

def exit_if_not(expression):
    if not expression:
        raise RuntimeError()

exit_if_not(liblouis.lou_charSize() == 4)

liblouis_dev = _loader["liblouis-table-dev.so.0"]

liblouis_dev.isLetter.argtypes = (c_wchar,)
liblouis_dev.toLowercase.argtypes = (c_wchar,)
liblouis_dev.toLowercase.restype = c_wchar

def println(line=""):
    sys.stdout.write(("%s\n" % line).encode("utf-8"))

def validate_chunks(chunked_text):
    return re.search(r"^([^)(|]+|\([^)(|][^)(|]+\))(\|?([^)(|]+|\([^)(|][^)(|]+\)))*$", chunked_text) != None

def print_chunks(text, hyphen_string):
    exit_if_not(len(hyphen_string) == len(text) - 1 and re.search("^[01x]+$", hyphen_string))
    chunked_text = []
    k = 0
    prev_c = None
    for c in hyphen_string:
        if c != prev_c and c == "0":
            chunked_text.append("(")
        chunked_text.append(text[k])
        if c != prev_c and prev_c == "0":
            chunked_text.append(")")
        if c == "1":
            chunked_text.append("|")
        prev_c = c
        k += 1
    chunked_text.append(text[k])
    if (prev_c == "0"):
        chunked_text.append(")")
    return "".join(chunked_text)

def parse_chunks(chunked_text):
    exit_if_not(validate_chunks(chunked_text))
    text, _ = read_text(chunked_text)
    hyphen_string = ["x"] * (len(text) - 1)
    k = 0
    for c in chunked_text:
        if c == "(":
            hyphen_string[k:] = ["0"] * (len(text) - 1 - k)
        elif c == ")":
            hyphen_string[k-1:] =  ["x"] * (len(text) - 1 - (k-1))
        elif c == "|":
            hyphen_string[k-1] = "1"
        else:
            k += 1
            if k > len(text):
                break
    return text, "".join(hyphen_string)

def read_text(maybe_chunked_text):
    if re.search("[)(|]", maybe_chunked_text) != None:
        text = re.sub("[)(|]", "", maybe_chunked_text)
        chunked_text = maybe_chunked_text
    else:
        text = maybe_chunked_text
        chunked_text = None
    return text, chunked_text

def compare_chunks(expected_hyphen_string, real_hyphen_string, text):
    exit_if_not(len(expected_hyphen_string) == len(text) - 1 and re.search("^[01x]+$", expected_hyphen_string))
    exit_if_not(len(real_hyphen_string) == len(text) - 1 and re.search("^[01]+$", real_hyphen_string))
    chunk_errors = "".join([x for x in chain(*izip_longest(text, map(lambda e, r: "*" if e in "1x" and r == "1" else
                                                                                  "." if r == "1" else
                                                                                  "-" if e == "1" else None,
                                                                     expected_hyphen_string, real_hyphen_string)))
                            if x is not None])
    return chunk_errors if re.search(r"[-\.]", chunk_errors) else None

def load_table(table):
    liblouis_dev.loadTable(table);

def is_letter(text):
    return all([liblouis_dev.isLetter(c) for c in text])

def to_lowercase(text):
    return "".join([liblouis_dev.toLowercase(c) for c in text])

def to_dot_pattern(braille):
    c_braille = create_unicode_buffer(braille)
    c_dots = create_string_buffer(9 * len(braille))
    liblouis_dev.toDotPattern(c_braille, c_dots)
    return c_dots.value

def hyphenate(table, text):
    c_text = create_unicode_buffer(text)
    c_text_len = c_int(len(text))
    c_hyphen_string = create_string_buffer(len(text) + 1)
    exit_if_not(liblouis.lou_hyphenate(table, c_text, c_text_len, c_hyphen_string, 0))
    return "".join(['1' if int(p) % 2 else '0' for p in c_hyphen_string.value[1:]])

def translate(table, text):
    c_text = create_unicode_buffer(text)
    c_text_len = c_int(len(text))
    braille_len = len(text) * outlenMultiplier
    c_braille = create_unicode_buffer(braille_len)
    c_braille_len = c_int(braille_len)
    max_rules = 16
    c_rules = (c_void_p * max_rules)()
    c_rules_len = c_int(max_rules)
    exit_if_not(liblouis.trace_translate(table, c_text, byref(c_text_len), c_braille, byref(c_braille_len),
                                         None, None, None, None, None, c_rules, byref(c_rules_len), 0))
    return c_braille.value, c_rules[0:c_rules_len.value]

def get_rule(c_rule_pointer):
    c_rule_string = create_unicode_buffer(u"", 128)
    if not liblouis_dev.printRule(cast(c_rule_pointer, c_void_p), c_rule_string):
        return None
    return tuple(c_rule_string.value.split(" "))

def suggest_chunks(text, braille):
    c_text = create_unicode_buffer(text)
    c_braille = create_unicode_buffer(braille)
    c_hyphen_string = create_string_buffer(len(text) + 1)
    if not liblouis_dev.suggestChunks(c_text, c_braille, c_hyphen_string):
        return None;
    hyphen_string = "".join(c_hyphen_string.value[1:len(c_hyphen_string.value)-1])
    assert len(hyphen_string) == len(text) - 1 and re.search("^[01x]+$", hyphen_string)
    return hyphen_string

def find_possible_matches(text, braille):
    c_text = create_unicode_buffer(text)
    c_braille = create_unicode_buffer(braille)
    max_rules = 16
    c_matches = [u""] * max_rules + [None]
    for i in range(0, max_rules):
        c_matches[i] = create_unicode_buffer(c_matches[i], 128)
        c_matches[i] = cast(c_matches[i], c_wchar_p)
    c_matches = (c_wchar_p * (max_rules + 1))(*c_matches)
    liblouis_dev.findPossibleMatches(c_text, c_braille, c_matches)
    return map(lambda x: tuple(x.split(" ")), takewhile(lambda x: x, c_matches))

def open_dictionary(dictionary):
    conn = sqlite3.connect(dictionary)
    c = conn.cursor()
    return conn, c

def filterfalse(predicate, iterable):
    return [x for x in iterable if not predicate(x)]

def partition(pred, iterable):
    t1, t2 = tee(iterable)
    return filterfalse(pred, t1), filter(pred, t2)
