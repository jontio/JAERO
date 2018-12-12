#!/usr/bin/perl -w
#
# This file is a part of libacars
#
# Copyright (c) 2018 Tomasz Lemiech <szpajder@gmail.com>

sub usage {
	print STDERR <<EOF
This script extracts a given list variable(s) from a Makefile.am and prints them
in a format suitable for inclusion into CMakeLists.txt

Usage: am2cmake.pl <Makefile.am_file_name> <list_variable_name1> [<list_variable_name2> [...]]
EOF
;
	exit 1;
}

@ARGV < 2 and usage;
open(IN, '<' . shift @ARGV) or die "open(): $!\n";

my $vars = {};
my $line;
my $curvar;
my @unwrapped;
map { $vars->{$_} = []; } @ARGV;

my $content;
{
	local $/;
	$content = <IN>;
}
close(IN);
$content =~ s/\\\n//sg;
for $line(split(/\n/, $content)) {
	if($line =~ /^(\S+)\s*\+=(.*)/) {
		exists($vars->{$1}) and push @{$vars->{$1}}, grep { $_ ne '' } split(/\s+/, $2);
	} elsif($line =~ /^(\S+)\s*=(.*)/) {
		exists($vars->{$1}) and $vars->{$1} = [ grep { $_ ne '' } split(/\s+/, $2) ];
	}
}

for my $k(sort keys %$vars) {
	if(scalar @{$vars->{$k}} > 0) {
		print "set($k\n", map( { "\t$_\n" } @{$vars->{$k}}), ")\n\n";
	}
}
