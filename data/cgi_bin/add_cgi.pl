#!/usr/bin/perl

use CGI;

# Create a new CGI object
my $cgi = CGI->new;

# Get the values of the two numbers to add from the query string
# if query string case
# print $ENV{QUERY_STRING};

# else if post put case
my $num1 = $cgi->param('num1');
my $num2 = $cgi->param('num2');

# print "num1 : $num1 \n";
# print "num2 : $num2 \n";
# Convert the numbers to integers

if ($num1 eq "" || $num2 eq "") {
	if (exists $ENV{QUERY_STRING}) {

		$query = $ENV{'QUERY_STRING'};
		@list = split( /\&/, $query);
		foreach (@list) {
			($var, $val) = split(/=/);
			$val =~ s/\'//g;
			$val =~ s/\+/ /g;
			$val =~ s/%(\w\w)/sprintf("%c", hex($1))/ge;
			if ($var eq "num1") {
				if ($val ne ""){
					$num1 = int($val);
				}
			} elsif ($var eq "num2") {
				if ($val ne ""){
					$num2 = int($val);
				}
			}
		}
		if ($num1 eq "" || $num2 eq "") {
			print $cgi->header,
			  $cgi->start_html("Addition Result"),
			  $cgi->center($cgi->h1("Addition Result"), $cgi->p("num1 or num2 does not exist.")),
			  $cgi->end_html;
			exit;
		}else {
			my $sum = $num1 + $num2;

			print $cgi->header,
			$cgi->start_html("Addition Result"),
			$cgi->center($cgi->h1("Addition Result"), $cgi->p("The sum of $num1 and $num2 is $sum.")),
			$cgi->end_html;
			exit;
		}
	}else{
		print $cgi->header,
		  $cgi->start_html("Addition Result"),
		  $cgi->center($cgi->h1("Addition Result"), $cgi->p("num1 and num2 does not exist.")),
		  $cgi->end_html;
		exit;
	}
}else{
	$num1 = int($num1);
	$num2 = int($num2);
	# Add the numbers
	my $sum = $num1 + $num2;


	# Print the HTML response
	print $cgi->header,
		$cgi->start_html("Addition Result"),
		$cgi->center($cgi->h1("Addition Result"), $cgi->p("The sum of $num1 and $num2 is $sum.")),
		$cgi->end_html;
}
