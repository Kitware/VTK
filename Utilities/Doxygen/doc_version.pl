#!/usr/bin/env perl
# Time-stamp: <2001-10-17 16:30:58 barre>
#
# Extract VTK version and add it to documentation
#
# barre : Sebastien Barre <sebastien@barre.nom.fr>
#
# 0.23 (barre) :
#   - update useful links
#
# 0.22 (barre) :
#   - add more (useful) links to various VTK documentation ressources
#
# 0.21 (barre) :
#   - no more --logo defaults
#
# 0.2 (barre) :
#   - update to match the new VTK 4.0 tree
#   - change default --header so that it can be launched from Utilities/Doxygen
#   - change default --to so that it can be launched from Utilities/Doxygen
#   - add --logo file : use 'file' as logo
#
# 0.16 (barre) :
#   - change default --to to '../vtk-doxygen' to comply with Kitware
#   - update VTK home page URL.
#
# 0.15 (barre) :
#   - fix RCS/CVS tags problem (regexp replacement when this file is in a CVS)
#
# 0.14 (barre) :
#   - as doxygen now handles RCS/CVS tags of the form $word:text$, use them 
#
# 0.13 (barre) :
#   - change doxygen command style from \ to @ to match javadoc, autodoc, etc.
#
# 0.12 (barre) :
#   - change default --to to '../vtk-dox'
#
# 0.11 (barre)
#   - fix O_TEXT flag problem
#   - switch to Unix CR/LF format
#
# 0.1 (barre)
#   - initial release

use Carp;
use Fcntl;
use Getopt::Long;
use strict;

my ($VERSION, $PROGNAME, $AUTHOR) = (0.23, $0, "Sebastien Barre");
$PROGNAME =~ s/^.*[\\\/]//;
print "$PROGNAME $VERSION, by $AUTHOR\n";

# -------------------------------------------------------------------------
# Defaults (add options as you want: "verbose" => 1 for default verbose mode)

my %default = 
  (
   header => "../../Common/vtkVersion.h",
   store => "doc_VTK_version.dox",
   to => "../../../VTK-doxygen"
  );

# -------------------------------------------------------------------------
# Parse options

my %args;
Getopt::Long::Configure("bundling");
GetOptions (\%args, "help", "header=s", "logo=s", "store=s", "to=s");

if (exists $args{"help"}) {
    print <<"EOT";
by $AUTHOR
Usage : $PROGNAME [--help] [--header file] [--store file] [--to path]
  --help        : this message
  --header file : use 'file' to find version (default: $default{header})
  --logo file   : use 'file' as logo (default: $default{logo})
  --store file  : use 'file' to store version (default: $default{store})
  --to path     : use 'path' as destination directory (default: $default{to})

Example:
  $PROGNAME
EOT
    exit;
}

$args{"header"} = $default{"header"} if ! exists $args{"header"};
$args{"logo"} = $default{"logo"} if ! exists $args{"logo"};
$args{"store"} = $default{"store"} if ! exists $args{"store"};
$args{"to"} = $default{"to"} if ! exists $args{"to"};
$args{"to"} =~ s/[\\\/]*$// if exists $args{"to"};

my $os_is_win = ($^O =~ m/(MSWin32|Cygwin)/i);
my $open_file_as_text = $os_is_win ? O_TEXT : 0;
my $start_time = time();
    
# -------------------------------------------------------------------------
# Try to get VTK version from a header

my ($version, $revision, $date) = (undef, undef, undef);

sysopen(FILE, $args{"header"}, O_RDONLY|$open_file_as_text)
  or croak "$PROGNAME: unable to open $args{header}\n";

while (<FILE>) {
    if ($_ =~ /define\s+VTK_VERSION\s+\"(.*)\"/) {
        $version = $1;
        print " => $version\n";
    } elsif ($_ =~ /define\s+VTK_SOURCE_VERSION.*(.Revision:.*.?\$).*(.Date:.*?\$).*\"/) {
        $revision = $1;
        $date = $2;
        print " => $revision $date\n";
        last;
    }
}

close(FILE);

croak "$PROGNAME: unable to find version/date in " . $args{"header"} . "\n"
  if (!defined $version || !defined $revision || !defined $date);

# -------------------------------------------------------------------------
# Build documentation

my $destination_file = $args{"to"} . "/" . $args{"store"};
print "Building version documentation to ", $destination_file, "\n";

sysopen(DEST_FILE, 
        $destination_file, 
        O_WRONLY|O_TRUNC|O_CREAT|$open_file_as_text)
  or croak "$PROGNAME: unable to open destination file " . $destination_file . "\n";

print DEST_FILE 
  "/*! \@mainpage VTK $version Documentation\n\n";

print DEST_FILE 
  "  \@image html " . $args{"logo"} . "\n"
  if exists $args{"logo"} && -f $args{"logo"};

print DEST_FILE 
  "  $revision\n",
  "  $date\n",
  "  \@par Useful links:\n",
  "  \@li VTK Home: http://public.kitware.com/VTK\n",
  "  \@li VTK Mailing-list: http://public.kitware.com/mailman/listinfo/vtkusers\n",
  "  \@li VTK FAQ: http://public.kitware.com/cgi-bin/vtkfaq\n",
  "  \@li VTK Search: http://www.kitware.com/search.html\n",
  "  \@li VTK Dashboard: http://public.kitware.com/vtkhtml/Testing/HTML/TestingResults/Dashboard/MostRecentResults-Nightly/Dashboard.html",
  "  \@li VTK-Doxygen scripts (Sebastien Barre): http://www.barre.nom.fr/vtk/doc/README\n",
  "  \@li Kitware Home: http://www.kitware.com\n",
  "  \@li Sebastien's VTK Links: http://www.barre.nom.fr/vtk/links.html\n",
  "  \@li Other Links: http://public.kitware.com/VTK/otherLinks.html\n",
  " ",
  "*/\n\n";

close(DEST_FILE);

print "Finished in ", time() - $start_time, " s.\n";
