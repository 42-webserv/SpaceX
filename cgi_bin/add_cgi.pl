#!/usr/bin/perl

use CGI;

# Create a new CGI object
my $cgi = CGI->new;

# Get the values of the two numbers to add from the query string
# if query string case
# print $ENV{QUERY_STRING};

# else if post put case
my $num1 = $cgi->param("num1");
my $num2 = $cgi->param("num2");

@param_names = $cgi->param();
foreach $p (@param_names) {
  $value = $cgi->param($p);
  print "Param $p = $value<p>\n";
}

print "init : @names\n";
print "num1 : $num1 \n";
print "num2 : $num2 \n";
# Convert the numbers to integers
$num1 = int($num1);
$num2 = int($num2);
# Add the numbers
my $sum = $num1 + $num2;

print "num1 : $num1 \n";
print "num2 : $num2 \n";
print "hi : $query \n";

print "\n---\n";


# Print the HTML response
print $cgi->header,
      $cgi->start_html("Addition Result"),
	  $cgi->center($cgi->h1("Addition Result"), $cgi->p("The sum of $num1 and $num2 is $sum.")),
      $cgi->end_html;
