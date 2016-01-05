#!/usr/bin/python
import os
import sys
import ctypes
import louis
import re

curDir = os.path.abspath(os.path.dirname(__file__))

testFile = os.path.abspath(os.path.join(curDir, sys.argv[1]))

dummyTable = (ctypes.c_char_p * 1)()
dummyTable[0] = os.path.join(curDir, 'tables', 'dummy')

inline_table_re = re.compile(r'^#inlinetable')

def table_resolver(fileListAddress, tableList, base):
    addr = (ctypes.POINTER(ctypes.c_char_p)*1).from_address(ctypes.addressof(fileListAddress.contents))
    if inline_table_re.match(tableList):
        # an inline table is being compiled
        addr[0] = dummyTable
        return
    tableList = tableList.split(',')
    if base:
        if not os.path.isfile(base):
            # compileLine("include ...") has been called, so base should become the test file
            base = testFile
    else:
        base = testFile
    fileList = (ctypes.c_char_p * len(tableList))()
    i = 0
    for table in tableList:
        fileList[i] = os.path.abspath(os.path.join(os.path.dirname(base), table))
        i = i + 1
    addr[0] = fileList

louis.liblouis.registerTableResolver2.argtypes = [ctypes.CFUNCTYPE(ctypes.c_void_p,
                                                                   ctypes.POINTER(ctypes.POINTER(ctypes.c_char_p)),
                                                                   ctypes.c_char_p,
                                                                   ctypes.c_char_p)]
# may not be garbage collected
_table_resolver = louis.liblouis.registerTableResolver2.argtypes[0](table_resolver)
louis.liblouis.registerTableResolver2(_table_resolver)

class Template:
    counter = 0
    def __init__(self, template):
        Template.counter = Template.counter + 1
        self.name = Template.counter
        self.lines = []
        for line in template.split('\n'):
            self.lines.append(Template.Line(line))
    def __iter__(self):
        return iter(self.lines)
    def __str__(self):
        return '\n'.join([(('[ ] ' if line.conditional else '    ') + str(line)) for line in self])
    
    class Line:
        line_re = re.compile(r'^(\[ \])? *(.*)$')
        def __init__(self, line):
            m = Template.Line.line_re.match(line)
            self.line = m.group(2)
            self.conditional = bool(m.group(1))
        def __str__(self):
            return self.line

class Table:
    def __init__(self, template, enabledLines):
        self.name = '#inlinetable<%s,%s>' % (template.name, ''.join([('x' if enabled else '.') for enabled in enabledLines]))
        self.lines = []
        enabledLinesIter = iter(enabledLines)
        for line in template:
            if line.conditional:
                try:
                    if enabledLinesIter.next():
                        self.lines.append(str(line))
                except StopIteration:
                    raise Exception('pattern %s can not be applied to template\n %s' % (enabledLines, template))
            else:
                self.lines.append(str(line))
        try:
            enabledLinesIter.next()
            raise Exception('pattern %s can not be applied to template\n %s' % (template, enabledLines))
        except StopIteration:
            pass
    def __iter__(self):
        return iter(self.lines)
    def __str__(self):
        return '\n'.join(self)

line_re = re.compile(r'^[ \t]*((?P<key>input|output|table|template|italic)[ \t]+(?P<val>.*?))?[ \t]*(#.*)?$')
typeform_re = re.compile(r'^[\.\+]+$')
table_pattern_re = re.compile(r'^\[([x ](\|[x ])*)\]$')

def main():
    curTemplate = None
    curTables = []
    curText = None
    curTypeform = None
    compiledTables = []
    failedTests = 0
    prevKey = None
    with open(sys.argv[1], 'r') as f:
        lines = iter(f.readlines())
        while True:
            try:
                line = lines.next()
            except StopIteration:
                break
            m = line_re.match(line)
            if not m:
                raise Exception('invalid syntax: %s' % line)
            key = m.group('key')
            if key:
                val = m.group('val')
                if key == 'input':
                    curText = val
                    curTypeform = None
                elif key == 'italic':
                    if not len(val) == len(curText):
                        raise Exception('length of input should match length of typeform')
                    if not typeform_re.match(val):
                        raise Exception()
                    curTypeform = [(louis.italic if x == '+' else louis.plain_text) for x in val]
                elif key == 'template':
                    if not val == "'''":
                        raise Exception()
                    template = []
                    while True:
                        try:
                            line = lines.next()
                        except StopIteration:
                            raise Exception('')
                        if line == "'''\n":
                            break
                        else:
                            template.append(line)
                    curTemplate = Template(''.join(template))
                elif key == 'table':
                    table = val
                    m = table_pattern_re.match(table)
                    if m:
                        if not curTemplate:
                            raise Exception()
                        enabledLines = [(x == 'x') for x in m.group(1).split('|')]
                        table = Table(curTemplate, enabledLines)
                        if not table.name in compiledTables:
                            for line in table:
                                louis.compileString([table.name], line)
                                compiledTables.append(table.name)
                    if not prevKey == 'table':
                        curTables = []
                    curTables.append(table)
                elif key == 'output':
                    expected = val
                    if not curTables:
                        raise Exception()
                    if not curText:
                        raise Exception()
                    for table in curTables:
                        if isinstance(table, Table):
                            actual = louis.translateString([table.name], curText, typeform=curTypeform)
                        else:
                            actual = louis.translateString([table], curText, typeform=curTypeform)
                        if not actual == expected:
                            print 'input     ' + curText
                            print 'table     ' + ( "'''\n%s'''" % table if isinstance(table, Table) else table )
                            print 'output    ' + actual
                            print 'expected  ' + expected
                            print ''
                            failedTests = failedTests + 1
                else:
                    raise Exception()
                prevKey = key
    if failedTests > 0:
        exit(1)

if  __name__ =='__main__':
    main()