#!/usr/bin/perl
use warnings;
use strict;
$|++;

# Test a specific table with lou_checktable which causes an endless loop.
#
# Copyright (C) 2010 by Swiss Library for the Blind, Visually Impaired and Print Disabled
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.

# look for the table that causes the endless loop in the tables dir
# which is local to the tests dir. The Makefile passes this path via
# an env var named LOUIS_TEST_TABLEPATH.
$ENV{LOUIS_TABLEPATH} = $ENV{LOUIS_TEST_TABLEPATH};

my $table="loop.ctb";

$SIG{ALRM} = sub { die "lou_checktable on $table stuck in endless loop\n" };

alarm 5;
system("lou_checktable $table") == 0 
    or die "lou_checktable on $table failed\n";
alarm 0;
