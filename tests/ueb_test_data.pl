#!/usr/bin/perl

@data = glob("ueb_test_data/*");
$result = 0;

foreach $file (@data)
{
	chomp($file);
	
	@output = `../tools/lou_compare -t en-ueb-g2.ctb < $file`;
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



