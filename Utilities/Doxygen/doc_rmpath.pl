#!/usr/bin/env perl
# Time-stamp: <2001-10-05 11:49:40 barre>
#
# Remove path to intermediate Doxygen dir from html doc
#
# barre : Sebastien Barre <sebastien@barre.nom.fr>
#
# 0.1 (barre)
#   - first release

use Carp;
use Cwd 'abs_path';
use Getopt::Long;
use Fcntl;
use File::Find;
use strict;

my ($VERSION, $PROGNAME, $AUTHOR) = (0.1, $0, "Sebastien Barre");
$PROGNAME =~ s/^.*[\\\/]//;
print "$PROGNAME $VERSION, by $AUTHOR\n";

# -------------------------------------------------------------------------
# Defaults (add options as you want : "verbose" => 1 for default verbose mode)

my %default = 
  (
   html => "../../../doc/html",
   to => "../../../VTK-doxygen"
  );

# -------------------------------------------------------------------------
# Parse options

my %args;
Getopt::Long::Configure("bundling");
GetOptions (\%args, "help", "verbose|v", "html=s", "to=s");

if (exists $args{"help"}) {
    print <<"EOT";
Usage : $PROGNAME [--help] [--verbose|-v] [--html path] [--to path]
  --help       : this message
  --verbose|-v : verbose (display filenames while processing)
  --html path  : 'path' the Doxygen generated HTML doc (default : $default{html})
  --to path    : 'path' to intermediate Doxygen dir (default : $default{to})

Example:
  $PROGNAME
EOT
    exit;
}

$args{"to"} = $default{"to"} if ! exists $args{"to"};
$args{"to"} =~ s/[\\\/]*$// if exists $args{"to"};
$args{"html"} = $default{"html"} if ! exists $args{"html"};
$args{"html"} =~ s/[\\\/]*$// if exists $args{"html"};

my $os_is_win = ($^O =~ m/(MSWin32|Cygwin)/i);
my $open_file_as_text = $os_is_win ? O_TEXT : 0;
my $start_time = time();

# -------------------------------------------------------------------------
# Collect all HTML files

print "Collecting HTML files in ", $args{"html"}, "\n";

my @files;
find sub { 
    push @files, $File::Find::name
      if -f $_ && $_ =~ /\.html$/;
}, $args{"html"};

print " => ", scalar @files, " file(s) collected in ", time() - $start_time, " s.\n";

# -------------------------------------------------------------------------
# Remove path 

my ($nb_files, $htmlpath) = (0, abs_path($args{"to"}) . '/');
undef $/;  # slurp mode

print "Removing $htmlpath and writing...\n";
my $intermediate_time = time();

foreach my $filename (@files) {

    print "  $filename\n" if exists $args{"verbose"};

    # Open the file, read it entirely

    sysopen(HTMLFILE, 
            $filename, 
            O_RDONLY|$open_file_as_text)
      or croak "$PROGNAME: unable to open $filename\n";
    my $html = <HTMLFILE>;
    close(HTMLFILE);
    
    # Remove all paths
   
    if ($html =~ s/$htmlpath//gms) {
        ++$nb_files;
        sysopen(HTMLFILE, 
                $filename, 
                O_WRONLY|O_TRUNC|O_CREAT|$open_file_as_text)
          or croak "$PROGNAME: unable to open destination file $filename\n";
        print HTMLFILE $html;
        close(HTMLFILE);
    }
}

print " => $nb_files file(s) processed and written in ", time() - $intermediate_time, " s.\n";
print "Finished in ", time() - $start_time, " s.\n";

