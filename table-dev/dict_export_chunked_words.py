from utils import open_dictionary

def print_chunks(chunked_text):
    str_list = []
    in_nocross = False
    prev_c = None
    for c in chunked_text:
        if c == '[':
            if in_nocross or prev_c == '|':
                exit(1)
            in_nocross = True
            if prev_c != None:
                str_list.append('-')
        elif c == ']':
            if not in_nocross:
                exit(1)
            in_nocross = False
        elif c == '|':
            if in_nocross or prev_c == None or prev_c == ']' or prev_c == '|':
                exit(1)
            str_list.append('-')
        else:
            if prev_c == ']':
                str_list.append('-')
            elif not in_nocross and prev_c != None and prev_c != '|':
                str_list.append('0')
            str_list.append(c)
        prev_c = c
    if in_nocross or prev_c == '|':
        exit(1)
    print(''.join(str_list))

def main():
    conn, c = open_dictionary()
    c.execute("SELECT chunked_text FROM dictionary WHERE chunked_text IS NOT NULL")
    for chunked_text, in c.fetchall():
        print_chunks(chunked_text)
    conn.close()

if __name__ == "__main__": main()