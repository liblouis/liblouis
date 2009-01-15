# Liblouis Python ctypes bindings
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., Franklin Street, Fifth Floor,
# Boston MA  02110-1301 USA.

# Authors: Michael Curran, James Teh and Eitan Isaacson

from ctypes import *
import struct
import atexit

try:
    liblouis = cdll.liblouis
except OSError:
    liblouis = cdll.LoadLibrary("liblouis.so")

atexit.register(liblouis.lou_free)

liblouis.lou_version.restype = c_char_p

liblouis.lou_translateString.argtypes = \
    (c_char_p, c_wchar_p, POINTER(c_int), c_wchar_p, \
         POINTER(c_int), POINTER(c_char), POINTER(c_char), c_int)

liblouis.lou_translate.argtypes = \
    (c_char_p, c_wchar_p, POINTER(c_int), c_wchar_p, \
         POINTER(c_int), POINTER(c_char), POINTER(c_char), \
         POINTER(c_int), POINTER(c_int), POINTER(c_int), c_int)

version = liblouis.lou_version

def translate(tran_tables, inbuf, typeform=None,cursorPos=0, mode=0):
    tablesString = ",".join([str(x) for x in tran_tables])
    inbuf = unicode(inbuf)
    if typeform:
        typeform = struct.pack('B'*len(typeform),*typeform)
    else:
        typeform = None
    inlen = c_int(len(inbuf))
    outlen = c_int(inlen.value*2)
    outbuf = create_unicode_buffer(outlen.value)
    inPos = (c_int*outlen.value)()
    outPos = (c_int*inlen.value)()
    cursorPos = c_int(cursorPos)
    if not liblouis.lou_translate(tablesString, inbuf, byref(inlen), 
                                  outbuf, byref(outlen),  typeform, 
                                  None, outPos, inPos, byref(cursorPos), mode):
        raise RuntimeError("can't translate: tables %s, inbuf %s, typeform %s, cursorPos %s, mode %s"%(tran_tables, inbuf, typeform, cursorPos, mode))
    return outbuf.value, inPos[:outlen.value], outPos[:inlen.value], cursorPos.value

def translateString(tran_tables, inbuf, typeform = None, mode = 0):
    tablesString = ",".join([str(x) for x in tran_tables])
    inbuf = unicode(inbuf)
    if typeform:
        typeform = struct.pack('B'*len(typeform), *typeform)
    else:
        typeform = None
    inlen = c_int(len(inbuf))
    outlen = c_int(inlen.value*2)
    outbuf = create_unicode_buffer(outlen.value)
    if not liblouis.lou_translateString(tablesString, inbuf, byref(inlen), 
                                        outbuf, byref(outlen),  typeform, 
                                        None, mode):
        raise RuntimeError("can't translate: tables %s, inbuf %s, typeform %s, mode %s"%(tran_tables, inbuf, typeform, mode))
    return outbuf.value

# typeforms
plain_text = 0
italic = 1
underline = 2
bold = 4
computer_braille = 8

# translationModes
noContractions = 1
compbrlAtCursor = 2
dotsIO = 4
comp8Dots =  8

if __name__ == '__main__':
    # Just some common tests.
    print version()
    print translate(['../tables/en-us-g2.ctb'], u'Hello world!', cursorPos=5)
    
