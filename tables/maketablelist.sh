#! /bin/sh

#  Copyright (C) 2009, 2010 Christian Egli
#  Copyright (C) 2016 Bert Frees

# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved. This file is offered as-is,
# without any warranty.

# Use this script to regenerate Makefile.am if you must. I recommend
# against it for the reasons explained in
# http://www.gnu.org/software/hello/manual/automake/Wildcards.html.
# It's easy to pick up some spurious files that you did not mean to
# distribute.
 
OUTFILE=Makefile.am.new
COMMAND="ls | grep -v Makefile | grep -v README | grep -v maketablelist.sh | grep -v '.*~$' | grep -v '.in$' | sort -df | sed -e 's/$/ \\\\/' -e 's/^/	/' -e '\$s/\\\\$//'"

cat <<EOF >$OUTFILE
# generate the list of tables as follows:
# $ ${COMMAND}
table_files = \\
EOF

eval "$COMMAND" >> $OUTFILE

cat <<'EOF' >>$OUTFILE

comma=,
fuzzing_yml = $(top_srcdir)/.github/workflows/fuzzing.yml
$(fuzzing_yml): $(srcdir)/Makefile.am
	[ ! -w $(fuzzing_yml) ] || \
	perl -0777 -i -pe \
	     's~(table:).*?(steps:)~\1'"$$(printf '\n          - %s' $(filter %.utb %.ctb,$(table_files)))"'\n    \2~is' \
	     $@

all-local: $(fuzzing_yml)

# tables that use macros need to be expanded
BUILT_SOURCES = nl-chardefs.uti nl-NL-g0.utb
CLEANFILES = nl-chardefs.uti nl-NL-g0.utb
nl-chardefs.uti nl-NL-g0.utb: % : %.in Makefile
	m4 -G $< > $@

tablesdir = $(datadir)/liblouis/tables
tables_DATA = $(table_files)
EXTRA_DIST = $(table_files) \
	nl-chardefs.uti.in \
	nl-NL-g0.utb.in
EOF

