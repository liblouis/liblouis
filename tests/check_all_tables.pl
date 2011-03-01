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
    if (my $pid = fork) {
	waitpid($pid, 0);
	if ($?) {
	    print STDERR "lou_checktable on $table failed or timed out\n";
	    $fail = 1;
	}
    } else {
	die "cannot fork: $!" unless defined($pid);
	alarm $timeout;
	exec ("lou_checktable $table 2> /dev/null");
	die "Exec of lou_checktable failed: $!";
    }
}

exit $fail;
