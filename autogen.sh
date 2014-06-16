#!/bin/sh
#
# liblouis Braille Translation and Back-Translation 
# Library
#
#   Based on the Linux screenreader BRLTTY, copyright (C) 1999-2006 by
#   The BRLTTY Team
#
#   Copyright (C) 2004, 2005, 2006
#   ViewPlus Technologies, Inc. www.viewplus.com
#   and
#   abilitiessoft, Inc. www.abilitiessoft.com
#   All rights reserved
#
#   Copyright (C) 2006--2013 by The Liblouis Team
#
#   This file is free software; you can redistribute it and/or modify it
#   under the terms of the Lesser or Library GNU General Public License 
#   (LGPL) as published by the
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
# write to
#   the Free Software Foundation, 51 Franklin Street, Fifth Floor,
#   Boston, MA 02110-1301, USA.
#
# autogen.sh glue for liblouis
#
# Requires: automake 1.9, autoconf 2.57+
# Conflicts: autoconf 2.13
set -e

# Refresh GNU autotools toolchain.
echo Cleaning autotools files...
find -type d -name autom4te.cache -print0 | xargs -0 rm -rf \;
find -type f \( -name missing -o -name install-sh -o -name mkinstalldirs \
	-o -name depcomp -o -name ltmain.sh -o -name configure \
	-o -name config.sub -o -name config.guess -o -name config.h.in \
        -o -name mdate-sh -o -name texinfo.tex \
	-o -name Makefile.in -o -name aclocal.m4 \) -print0 | xargs -0 rm -f

echo Running autoreconf...
autoreconf --force --install

exit 0
