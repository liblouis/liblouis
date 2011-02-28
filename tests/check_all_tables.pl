#!/usr/bin/perl
use warnings;
use strict;
$|++;

# Test all tables with lou_checktable.
#
# Copyright (C) 2010 by Swiss Library for the Blind, Visually Impaired and Print Disabled
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.

my $fail = 0;
# some tables are quite big and take some time to check, so keep the timeout reasonably long
my $timeout = 120; # seconds
my $tablesdir = $ENV{LOUIS_TABLEPATH};

# get all the tables from the tables directory
my @tables = glob("$tablesdir/*.[cu]tb $tablesdir/*.cti $tablesdir/*.dis");
# filter tables that only work when included inside others
@tables = grep(!/countries.cti|compress.ctb|corrections.ctb|core.[cu]tb/, @tables);

foreach my $table (@tables) {
    eval {
	local $SIG{ALRM} = sub { print STDERR "lou_checktable on $table stuck in endless loop\n"; die };
	alarm $timeout;
	unless (system("lou_checktable $table 2> /dev/null") == 0) {
	    print STDERR "lou_checktable on $table failed\n";
	    die;
	}
	alarm 0;
    };
    $fail = 1 if ($@);
}

exit $fail;
