#!/usr/bin/perl
use warnings;
use strict;
use Cwd 'abs_path';
use File::Temp 'tempdir';
$|++;

# Compile a table with deeply nested includes while the stack size is limited.
#
# Every level of nested includes adds stack frames of the table compiler. The
# frames used to be so large that a chain of includes a few levels deep
# overflowed the stack of applications that call liblouis from a thread with a
# small stack, such as the JVM (https://github.com/liblouis/liblouis/issues/1288).
#
# Copyright (C) 2026 by Anthony Tibbs
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved. This file is offered as-is,
# without any warranty.

my $depth = 50;
my $stack_kb = 1024;

my $abs_top_srcdir = abs_path("$ENV{srcdir}/..");

# Generate the chain of tables: "$depth.ctb" includes "($depth-1).ctb", which
# includes "($depth-2).ctb", and so on down to "1.ctb".
my $dir = tempdir(CLEANUP => 1);
open(my $fh, '>', "$dir/1.ctb") or die "Cannot write $dir/1.ctb: $!\n";
print $fh "include latinLetterDef6Dots.uti\n";
print $fh "space \\s 0\n";
close $fh;
for my $i (2 .. $depth) {
    open($fh, '>', "$dir/$i.ctb") or die "Cannot write $dir/$i.ctb: $!\n";
    print $fh "include " . ($i - 1) . ".ctb\n";
    close $fh;
}

$ENV{"LOUIS_TABLEPATH"} = join(",", "$abs_top_srcdir/tables", $dir);

my $table = "$depth.ctb";

# The stack size limit (in kilobytes) is set with the ulimit builtin of the shell.
system("sh", "-c", "ulimit -s $stack_kb && exec ../tools/lou_checktable $table --quiet") == 0
    or die "lou_checktable on a table with $depth nested includes failed with a stack of $stack_kb KB\n";
