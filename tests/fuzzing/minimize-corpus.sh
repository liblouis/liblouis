#!/bin/bash

#
#   liblouis Braille Translation and Back-Translation Library
#
#   Copyright (C) 2022 Anna Stan, Nicolas Morel, Kalilou Mamadou Dramé
#
#   This file is part of liblouis.
#
#   liblouis is free software: you can redistribute it and/or modify it
#   under the terms of the GNU Lesser General Public License as published
#   by the Free Software Foundation, either version 2.1 of the License, or
#   (at your option) any later version.
#
#   liblouis is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#   Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public
#   License along with liblouis. If not, see <http://www.gnu.org/licenses/>.
#

if [[ $# -lt 1 ]]
then
  echo "Usage: $0 <corpus-dir>"
  echo "Usage: $0 --preserve-txt <corpus-dir> (minimize corpus but keep .txt files intact)"
  exit 1
fi

preserve_txt=0
if [[ "$1" == "--preserve-txt" ]]
then
  preserve_txt=1
  CORPUS_DIR=$2
else
  CORPUS_DIR=$1
fi 

ROOT_PWD=../../
export FUZZ_TABLE=$ROOT_PWD/tables/en-us-g2.ctb
FUZZER=$ROOT_PWD/tests/fuzzing/fuzz_translate

TMP_DIR=$(mktemp -d)
echo "Merging..."
`$FUZZER -merge=1 $TMP_DIR $CORPUS_DIR`
echo "Removing old files..."
if [[ $preserve_txt -eq 1 ]]
then
  echo " => Preserve .txt files"
  rm -rvf $(find $CORPUS_DIR | grep -vE "*.txt|$CORPUS_DIR") 2>/dev/null
else
  rm -rf $CORPUS_DIR/* 2>/dev/null
fi
cp $TMP_DIR/* $CORPUS_DIR 2>/dev/null
rm -rf $TMP_DIR
echo "Merging done !"
