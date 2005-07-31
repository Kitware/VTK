#!/usr/bin/env perl
# Time-stamp: <2002-11-22 16:23:16 barre>
#
# Extract VTK version and add it to documentation
#
# barre : Sebastien Barre <sebastien@barre.nom.fr>
#
# 0.3 (barre) :
#   - update to search for the version infos in a different file
#     (i.e. top CMakeLists.txt file instead of vtkVersion.h)
#   - --header becomes --revision_file and --version_file
#
# 0.25 (barre) :
#   - update useful links for Dart
#
# 0.24 (barre) :
#   - update useful links for new public.kitware.com structure
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
use File::Basename;
use Getopt::Long;
use strict;

my ($VERSION, $PROGNAME, $AUTHOR) = (0.3, $0, "Sebastien Barre");
$PROGNAME =~ s/^.*[\\\/]//;
print "$PROGNAME $VERSION, by $AUTHOR\n";

# -------------------------------------------------------------------------
# Defaults (add options as you want: "verbose" => 1 for default verbose mode)

my %default = 
  (
   version_file => "../../CMakeLists.txt",
   revison_file => "../../Common/vtkVersion.h",
   store => "doc_VTK_version.dox",
   to => "../../../VTK-doxygen"
  );

# -------------------------------------------------------------------------
# Parse options

my %args;
Getopt::Long::Configure("bundling");
GetOptions (\%args, "help", "revision_file=s", "version_file=s", "logo=s", "store=s", "to=s");

if (exists $args{"help"}) {
    print <<"EOT";
by $AUTHOR
Usage : $PROGNAME [--help] [--revision_file file] [--store file] [--to path]
  --help        : this message
  --revision_file file : use 'file' to find revision (default: $default{revision_file})
  --version_file file : use 'file' to find version info (default: $default{version_file})
  --logo file   : use 'file' as logo (default: $default{logo})
  --store file  : use 'file' to store version (default: $default{store})
  --to path     : use 'path' as destination directory (default: $default{to})

Example:
  $PROGNAME
EOT
    exit;
}

$args{"revision_file"} = $default{"revision_file"} if ! exists $args{"revision_file"};
$args{"version_file"} = $default{"version_file"} if ! exists $args{"version_file"};
$args{"logo"} = $default{"logo"} if ! exists $args{"logo"};
$args{"store"} = $default{"store"} if ! exists $args{"store"};
$args{"to"} = $default{"to"} if ! exists $args{"to"};
$args{"to"} =~ s/[\\\/]*$// if exists $args{"to"};

my $os_is_win = ($^O =~ m/(MSWin32|Cygwin)/i);
my $open_file_as_text = $os_is_win ? O_TEXT : 0;
my $start_time = time();

# -------------------------------------------------------------------------
# Try to get VTK version and revision

my ($major_version, $minor_version, $build_version, $revision, $date) = (undef, undef, undef);

sysopen(FILE, $args{"version_file"}, O_RDONLY|$open_file_as_text)
  or croak "$PROGNAME: unable to open $args{version_file}\n";

while (<FILE>) {
    if ($_ =~ /VTK_MAJOR_VERSION\s+(\d+)/) {
        $major_version = $1;
        print " major => $major_version\n";
    } elsif ($_ =~ /VTK_MINOR_VERSION\s+(\d+)/) {
        $minor_version = $1;
        print " minor => $minor_version\n";
    } elsif ($_ =~ /VTK_BUILD_VERSION\s+(\d+)/) {
        $build_version = $1;
        print " build => $build_version\n";
    }
}

close(FILE);

croak "$PROGNAME: unable to find version in " . $args{"version_file"} . "\n"
  if (!defined $major_version || !defined $minor_version || !defined $build_version);

sysopen(FILE, $args{"revision_file"}, O_RDONLY|$open_file_as_text)
  or croak "$PROGNAME: unable to open $args{revision_file}\n";

while (<FILE>) {
    if ($_ =~ /define\s+VTK_SOURCE_VERSION.*(.Revision:.*.?\$).*(.Date:.*?\$).*\"/) {
        $revision = $1;
        $date = $2;
        print " revision => $revision $date\n";
        last;
    }
}

close(FILE);

croak "$PROGNAME: unable to find revision/date in " . $args{"revision_file"} . "\n"
  if (!defined $revision || !defined $date);

# -------------------------------------------------------------------------
# Build documentation

my $destination_file = $args{"to"} . "/" . $args{"store"};
print "Building version documentation to ", $destination_file, "\n";

sysopen(DEST_FILE, 
        $destination_file, 
        O_WRONLY|O_TRUNC|O_CREAT|$open_file_as_text)
  or croak "$PROGNAME: unable to open destination file " . $destination_file . "\n";

print DEST_FILE 
  "/*! \@mainpage VTK $major_version.$minor_version.$build_version Documentation\n\n";

print DEST_FILE 
  "  \@image html " . basename($args{"logo"}) . "\n"
  if exists $args{"logo"} && -f $args{"logo"};

print DEST_FILE 
  "  $revision\n",
  "  $date\n",
  "  \@par Useful links:\n",
  "  \@li VTK Home: http://www.vtk.org\n",
  "  \@li VTK Users Mailing-list: http://public.kitware.com/mailman/listinfo/vtkusers\n",
  "  \@li VTK Developer Mailing List: http://www.vtk.org/mailman/listinfo/vtk-developers\n",
  "  \@li VTK FAQ: http://www.vtk.org/Wiki/VTK_FAQ\n",
  "  \@li VTK Wiki: http://www.vtk.org/Wiki/\n",
  "  \@li VTK Search: http://www.kitware.com/search.html\n",
  "  \@li VTK Dashboard: http://www.vtk.org/Testing/Dashboard/MostRecentResults-Nightly/Dashboard.html\n",
  "  \@li VTK-Doxygen scripts (Sebastien Barre): http://www.barre.nom.fr/vtk/doc/README\n",
  "  \@li Kitware Home: http://www.kitware.com\n",
  "  \@li Sebastien's VTK Links: http://www.barre.nom.fr/vtk/links.html\n",
  "  \@li Other Links: http://www.vtk.org/links.php\n",
  " ",
  "*/\n\n";

close(DEST_FILE);

print "Finished in ", time() - $start_time, " s.\n";
