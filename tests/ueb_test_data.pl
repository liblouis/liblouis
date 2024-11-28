#!/usr/bin/perl

/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2016 Mike Gray, American Printing House for the Blind

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

@data = glob("$ENV{UEB_TEST_DATA_PATH}/*.txt");
$result = 0;

foreach $file (@data)
{
	chomp($file);

	@output = `./check_ueb_test_data -t tables/en-ueb-g2.ctb < $file`;
	if($? != 0)
	{
		$result = 1;
	}
	
	$last = $output[$#output];
	chomp($last);	
	$last =~ /^([0-9\.]+)%/;
	print("$file:  $1\n");
}

`rm -f pass.txt fail.txt output.txt`;
exit $result;

__END__



