#!/usr/bin/perl -w
use strict;
$|++;

my $TEST_CORPUS_PREFIX = "$ENV{srcdir}/table_test_corpuses/test_";

my @test_corpuses;
my $table_counter = 0;
my $word_counter;
my $failed_counter;
my $failed = 0;

for my $test_corpus (glob "$TEST_CORPUS_PREFIX*") {
    my ($table) = ($test_corpus =~ /^$TEST_CORPUS_PREFIX(.*)$/);

    # ignore tables which are not vaild
    unless (system("lou_checktable $table > /dev/null 2>&1") == 0) {
	print "Table $table doesn't seem to be valid. Skipping it.\n";
	next;
    }
    
    open(CORPUS, "<", $test_corpus) or die "cannot open test corpus: $test_corpus\n";

    print "Testing $table...\n";

    $table_counter++;
    $word_counter = 0;
    $failed_counter = 0;
    while (<CORPUS>) {
	chomp;
	# ignore comment and empty lines
	next if (/^#.*/);
	next if (/^ *$/);
	my ($untranslated, $expected_translation) = split;
	$word_counter++;
	my $translation = `echo $untranslated | lou_translate -f $table`;
	chomp $translation;
	unless ($expected_translation eq $translation) {
	    print "Translation error: '$translation' expected '$expected_translation'\n";
	    $failed_counter++;
	    $failed = 1;
	}
    }
    print "Tested $word_counter words. ";
    if ($failed_counter == 0) {
	print "All translations are correct.\n";
    } else {
	print "$failed_counter words failed with a wrong translation.\n";
    }
    close(CORPUS);
}
print "Tested $table_counter tables.\n";

exit $failed;
