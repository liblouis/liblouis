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

"""Liblouis Python ctypes bindings
These bindings allow you to use the liblouis braille translator and back-translator library from within Python.
This documentation is only a Python helper.
Please see the liblouis guide for more information.
@note: Back-translation is not currently supported.
@author: Michael Curran
@author: James Teh
@author: Eitan Isaacson
"""

from ctypes import *
import struct
import atexit

try:
    liblouis = cdll.liblouis
except OSError:
    liblouis = cdll.LoadLibrary("liblouis.so")

atexit.register(liblouis.lou_free)

liblouis.lou_version.restype = c_char_p

liblouis.lou_translateString.argtypes = (
    c_char_p, c_wchar_p, POINTER(c_int), c_wchar_p, 
         POINTER(c_int), POINTER(c_char), POINTER(c_char), c_int)

liblouis.lou_translate.argtypes = (
    c_char_p, c_wchar_p, POINTER(c_int), c_wchar_p, 
         POINTER(c_int), POINTER(c_char), POINTER(c_char), 
         POINTER(c_int), POINTER(c_int), POINTER(c_int), c_int)

def version():
    """Obtain version information for liblouis.
    @return: The version of liblouis, plus other information, such as
        the release date and perhaps notable changes.
    @rtype: str
    """
    return liblouis.lou_version()

def translate(tran_tables, inbuf, typeform=None,cursorPos=0, mode=0):
    """Translate a string of characters, providing position information.
    @param tran_tables: A list of translation tables.
        The first table in the list must be a full pathname, unless the tables are in the current directory.
    @type tran_tables: list of str
    @param inbuf: The string to translate.
    @type inbuf: unicode
    @param typeform: A list of typeform constants indicating the typeform for each position in inbuf,
        C{None} for no typeform information.
    @type typeform: list of int
    @param cursorPos: The position of the cursor in inbuf.
    @type cursorPos: int
    @param mode: The translation mode; add multiple values for a combined mode.
    @type mode: int
    @return: A tuple of: the translated string,
        a list of input positions for each position in the output,
        a list of output positions for each position in the input, and
        the position of the cursor in the output.
    @rtype: (unicode, list of int, list of int, int)
    @raise RuntimeError: If a complete translation could not be done.
    @see: lou_translate in the liblouis guide
    """
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
    """Translate a string of characters.
    @param tran_tables: A list of translation tables.
        The first table in the list must be a full pathname, unless the tables are in the current directory.
    @type tran_tables: list of str
    @param inbuf: The string to translate.
    @type inbuf: unicode
    @param typeform: A list of typeform constants indicating the typeform for each position in inbuf,
        C{None} for no typeform information.
    @type typeform: list of int
    @param mode: The translation mode; add multiple values for a combined mode.
    @type mode: int
    @return: The translated string.
    @rtype: unicode
    @raise RuntimeError: If a complete translation could not be done.
    @see: lou_translateString in the liblouis guide
    """
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

#{ Typeforms
plain_text = 0
italic = 1
underline = 2
bold = 4
computer_braille = 8

#{ Translation modes
noContractions = 1
compbrlAtCursor = 2
dotsIO = 4
comp8Dots =  8
#}

if __name__ == '__main__':
    # Just some common tests.
    print version()
    print translate(['../tables/en-us-g2.ctb'], u'Hello world!', cursorPos=5)
