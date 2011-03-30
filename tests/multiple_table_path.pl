#!/usr/bin/perl
use warnings;
use strict;
use Cwd 'abs_path';
$|++;

# Test a specific table with lou_checktable which causes an endless loop.
#
# Copyright (C) 2011 by Swiss Library for the Blind, Visually Impaired and Print Disabled
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.

my @tables = (
    # a global table
    "en-us-g2.ctb",
    # a local table
    "loop.ctb",
    # a table with a relative path
    "$ENV{srcdir}/tables/loop.ctb",
    # a list of tables with a relative path
    "$ENV{srcdir}/../tables/en-us-g2.ctb,en-us-g1.ctb",
    # a table with an absolute path
    abs_path("$ENV{srcdir}/tables/loop.ctb"),
    # a list of tables with an absolute path
    abs_path("$ENV{srcdir}/../tables/en-us-g2.ctb") . ",en-us-g1.ctb"
    );

# ensure existing tables are found
foreach my $table (@tables) {
    system("lou_checktable $table --quiet") == 0 or die;
}

# ensure a non-existing table is not found
system("lou_checktable nonexistent.ctb 2> /dev/null") != 0 or die;
