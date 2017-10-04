#!/usr/bin/env bash
CURDIR=$(cd $(dirname "$0") && pwd)
source <(cat sbs.cfg | sed 's/ *= */=/g')
echo auch | $CURDIR/../tools/lou_translate "$CURDIR/$BASE_TABLE,$CONTRACTIONS_TABLE,$PATTERNS_TABLE"
