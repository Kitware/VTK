#!/usr/bin/env perl
# Time-stamp: <2001-08-17 14:09:42 barre>
#
# Extract VTK version and add it to documentation
#
# barre : Sebastien Barre <sebastien@barre.nom.fr>
#
# 0.16 (barre) :
#   - change default --to to '../vtk-doxygen' to comply with Kitware's doxyfile.
#   - updated VTK home page URL.
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

my ($VERSION, $PROGNAME, $AUTHOR) = (0.16, $0, "Sebastien Barre");
$PROGNAME =~ s/^.*[\\\/]//;
print "$PROGNAME $VERSION, by $AUTHOR\n";

# -------------------------------------------------------------------------
# Defaults (add options as you want : "v" => 1 for default verbose mode)

my %default = 
  (
   to => "../vtk-doxygen",
   store => "doc_version.dox",
   header => "common/vtkVersion.h"
  );

# -------------------------------------------------------------------------
# Parse options

my %args;
Getopt::Long::Configure("bundling");
GetOptions (\%args, "header=s", "store=s", "to=s", "help|?");

if (exists $args{"help"}) {
    print <<"EOT";
$PROGNAME $VERSION
by $AUTHOR
Usage : $PROGNAME [--help|?] [--header file] [--store file] [--to path]
  --help|?      : this message
  --header file : use 'file' to find version (default: $default{header})
  --store file  : use 'file' to store version (default: $default{store})
  --to path     : use 'path' as destination directory (default: $default{to})

Example:
  $PROGNAME
EOT
    exit;
}

$args{"header"} = $default{"header"} if ! exists $args{"header"};
$args{"store"} = $default{"store"} if ! exists $args{"store"};
$args{"to"} = $default{"to"} if ! exists $args{"to"};
$args{"to"} =~ s/[\\\/]*$// if exists $args{"to"};

my $os_is_win = ($^O =~ m/(MSWin32|Cygwin)/i);
my $open_file_as_text = $os_is_win ? O_TEXT : 0;
my $start_time = time();
    
# -------------------------------------------------------------------------
# Try to get VTK version from vtkVersion.h

my ($version, $revision, $date) = (undef, undef, undef);

sysopen(FILE, $args{"header"}, O_RDONLY|$open_file_as_text)
  or croak "$PROGNAME: unable to open $args{header}\n";

while (<FILE>) {
    if ($_ =~ /define\s+VTK_VERSION\s+\"(.*)\"/) {
        $version = $1;
    } elsif ($_ =~ /define\s+VTK_SOURCE_VERSION.*(.Revision:.*.?\$).*(.Date:.*?\$).*\"/) {
        $revision = $1;
        $date = $2;
        print "$revision $date\n";
        last;
    }
}

close(FILE);

croak "$PROGNAME: unable to find version/date in " . $args{"header"} . "\n"
  if (!defined $version || !defined $revision || !defined $date);

# -------------------------------------------------------------------------
# Build documentation

my $destination_file = $args{"to"} . "/" . $args{"store"};
print "Building version documentation to ", $destination_file, "...\n";

sysopen(DEST_FILE, $destination_file, O_WRONLY|O_TRUNC|O_CREAT|$open_file_as_text)
  or croak "$PROGNAME: unable to open destination file " . $destination_file . "\n";

print DEST_FILE 
  "/*! \@mainpage VTK $version Documentation\n\n",
  "  $revision\n",
  "  $date\n",
  "  \@sa VTK home page (Kitware): http://public.kitware.com\n",
  "  \@sa DOC project (Sebastien Barre): http://www.barre.nom.fr/vtk/doc/README\n",
  "*/\n\n";

close(DEST_FILE);

print "Finished in ", time() - $start_time, " s.\n";
