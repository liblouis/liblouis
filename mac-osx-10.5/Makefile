# liblouisxml Braille Transcription Library
#
#   This file may contain code borrowed from the Linux screenreader
#   BRLTTY, copyright (C) 1999-2006 by
#   the BRLTTY Team
#
#   Copyright (C) 2004, 2005, 2006, 2008
#   ViewPlus Technologies, Inc. www.viewplus.com
#   and
#   JJB Software, Inc. www.jjb-software.com
#   All rights reserved
#
#   This file is free software; you can redistribute it and/or modify it
#   under the terms of the Lesser or Library GNU General Public License 
#   as published by the
#   Free Software Foundation; either version 3, or (at your option) any
#   later version.
#
#   This file is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
#   Library GNU General Public License for more details.
# 
#   You should have received a copy of the Library GNU General Public 
#   License along with this program; see the file COPYING.  If not, 
#   write to
#   the Free Software Foundation, 51 Franklin Street, Fifth Floor,
#   Boston, MA 02110-1301, USA.
#
#    Maintained by John J. Boyer john.boyer@jjb-software.com
#
c_opt =  -c -Wall -O2 -fno-common 
includes = -I/usr/include/libxml2 -I/usr/include/liblouis
paths = -DLIBLOUIS_TABLES_PATH=\"/usr/share/liblouis/tables\" \
-DLBX_PATH=\"/usr/share/liblouisxml/lbx_files\"
libs = /usr/lib/libxml2.dylib /usr/lib/liblouis.dylib
objects = transcriber.o transcribe_computerCode.o \
liblouisxml.o \
semantics.o\
examine_document.o transcribe_document.o transcribe_paragraph.o \
transcribe_chemistry.o transcribe_graphic.o transcribe_music.o \
transcribe_math.o readconfig.o change_table.o paths.o makeContents.o \
transcribe_cdataSection.o

all : sem_names.h liblouisxml.dylib xml2brl
sem_names.h : sem_enum.h make_sem_names
	./make_sem_names
make_sem_names : make_sem_names.c
	gcc -O2 -Wall -o make_sem_names make_sem_names.c
liblouisxml.dylib : liblouisxml.h louisxml.h transcriber.h \
sem_enum.h sem_names.h $(objects) 
	gcc -dynamiclib $(objects) $(libs) -lc \
	-Wl,-install_name,liblouisxml.dylib \
	-o liblouisxml.dylib
liblouisxml.o : liblouisxml.c louisxml.h transcriber.h
	gcc $(c_opt) liblouisxml.c $(includes)
examine_document.o : examine_document.c louisxml.h sem_enum.h
	gcc $(c_opt) examine_document.c $(includes)
transcribe_document.o : transcribe_document.c louisxml.h sem_enum.h
	gcc $(c_opt) transcribe_document.c $(includes)
transcribe_paragraph.o : transcribe_paragraph.c louisxml.h \
transcriber.h sem_enum.h
	gcc $(c_opt) transcribe_paragraph.c $(includes)
change_table.o : change_table.c louisxml.h \
transcriber.h sem_enum.h
	gcc $(c_opt) change_table.c $(includes)
transcribe_math.o : transcribe_math.c louisxml.h \
transcriber.h sem_enum.h
	gcc $(c_opt) transcribe_math.c $(includes)
transcribe_computerCode.o : transcribe_computerCode.c louisxml.h \
transcriber.h sem_enum.h
	gcc $(c_opt) transcribe_computerCode.c $(includes)
transcribe_chemistry.o : transcribe_chemistry.c louisxml.h \
transcriber.h sem_enum.h
	gcc $(c_opt) transcribe_chemistry.c $(includes)
transcribe_music.o : transcribe_music.c louisxml.h\
transcriber.h sem_enum.h
	gcc $(c_opt) transcribe_music.c $(includes)
transcribe_graphic.o : transcribe_graphic.c louisxml.h\
transcriber.h sem_enum.h
	gcc $(c_opt) transcribe_graphic.c $(includes)
transcribe_cdataSection.o : transcribe_cdataSection.c louisxml.h \
transcriber.h sem_enum.h
	gcc $(c_opt) transcribe_cdataSection.c $(includes)
makeContents.o : makeContents.c louisxml.h \
transcriber.h sem_enum.h
	gcc $(c_opt) makeContents.c $(includes)
transcriber.o : transcriber.c louisxml.h\
transcriber.h sem_enum.h
	gcc $(c_opt) transcriber.c $(includes) 
semantics.o : semantics.c louisxml.h sem_enum.h sem_names.h
	gcc $(c_opt) semantics.c $(includes)
paths.o : paths.c louisxml.h
	gcc $(c_opt) paths.c $(includes) $(paths)
readconfig.o : readconfig.c louisxml.h sem_enum.h \
transcriber.h
	gcc $(c_opt) readconfig.c $(includes)
xml2brl : xml2brl.c liblouisxml.h louisxml.h
	gcc -o xml2brl -O2 -Wall xml2brl.c liblouisxml.dylib \
	$(libs) $(includes)

clean :
	-rm liblouisxml.dylib *.o xml2brl

