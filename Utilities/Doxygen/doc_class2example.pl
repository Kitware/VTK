#!/usr/bin/env perl
# Time-stamp: <2001-09-27 09:53:44 barre>
#
# Build cross-references between classes and examples
#
# barre : Sebastien Barre <sebastien@barre.nom.fr>
#
# 0.7 (barre) :
#   - update to match the new VTK 4.0 tree
#   - change default --dirs so that it can be launched from Utilities/Doxygen
#   - change default --to so that it can be launched from Utilities/Doxygen
#   - add Java support
#   - add --label str : use string as label in class page
#   - add --title str : use string as title in "Related Pages"
#   - add --dirmatch str : use string to match the directory name holding files
#   - add --unique str : use string as a unique page identifier (otherwise MD5)
#
# 0.6 (barre) :
#   - the "class to example" page is now split in different pages
#   - use --weight to increase or decrease the maximum weight of a page
#
# 0.55 (barre) :
#   - change default --to to '../vtk-doxygen' to comply with Kitware's doxyfile
#
# 0.54 (barre) :
#   - change doxygen command style from \ to @ to match javadoc, autodoc, etc.
#
# 0.53 (barre) :
#   - change default --to to '../vtk-dox'
#
# 0.52 (barre) :
#   - rename -l option to --link, add a parameter to preprend in front of
#     the example location/filename.
#
# 0.51 (barre) :
#   - fix O_TEXT flag problem
#   - switch to Unix CR/LF format
#
# 0.5 (barre) :
#   - removed version extraction feature (moved to another script)
#
# 0.4 (barre) :
#   - change (warning) default --to to '../vtk2' because I ruined my own
#     VTK distrib too many times :(
#   - change (warning) --to is now a path to a destination directory, and
#     no more a path to the destination doxygen file. The headers to update
#     will logically be searched here, while the examples to process are
#     searched in the directory where the script is launched
#
# 0.3 (barre)
#   - add '--parser' option to use specific parser only
#
# 0.2 (barre)
#   - use vtkVersion.h to fill the main page (@mainpage) with version/revision
#     => comment/remove PROJECT_NUMBER from your doxygen config file (doxyfile)
#   - add web URL to main page
#   - fix small "uninitialized variable" warning
#
# 0.1 (barre)
#   - first release (-l does not work for the moment)

use Carp;
use Data::Dumper;
use Digest::MD5  qw(md5 md5_hex md5_base64);
use Getopt::Long;
use Fcntl;
use File::Basename;
use File::Find;
use strict;

my ($VERSION, $PROGNAME, $AUTHOR) = (0.7, $0, "Sebastien Barre");
$PROGNAME =~ s/^.*[\\\/]//;
print "$PROGNAME $VERSION, by $AUTHOR\n";

# -------------------------------------------------------------------------
# Defaults (add options as you want : "verbose" => 1 for default verbose mode)

my %default = 
  (
   dirmatch => "^Examples\$",
   dirs => ["../.."],
   label => "Examples",
   limit => 20,
   store => "doc_class2examples.dox",
   title => "Class To Examples",
   unique => "e",
   to => "../../../VTK-doxygen",
   weight => 90000
  );

# -------------------------------------------------------------------------
# Matchers and parsers :
#
# $eliminate_matcher : regexp matching the names of the 'fake' classes 
#                      that shall be eliminated/ignored,
# %parsers           : hash defining each parser by associating
#                      parser_name => [filename_matcher, function_to_call].
#  $parser_name      : language identifier (Tcl, C++, Python, etc.),
#  $filename_matcher : regexp matching the filenames belonging to the language,
# @$function_to_call : function called when a filename is matched,
#                     - receives the contents of the file as a reference to an
#                       array of lines,
#                     - returns an array of the class names that have been 
#                       recognized in the example.

my $eliminate_matcher = '^vtkCommand$';

my %parsers = (
               "Tcl" => ['\.tcl$', \&parse],
               "C++" => ['\.cxx$', \&parse],
               "Java" => ['\.java$', \&parse],
               "Python" => ['\.py$', \&parse]
              );

# Parser (seems to work with all languages for the moment)

sub parse {
    my ($ref, %classes) = (shift, ());
    foreach my $line (@$ref) {
        if ($line =~ /\b(vtk[A-Z0-9][A-Za-z0-9]+)/) {
            $classes{$1}++;
        }
    }
    return keys %classes;
}

# -------------------------------------------------------------------------
# Parse options

my %args;
Getopt::Long::Configure("bundling");
GetOptions (\%args, "help", "verbose|v", "dirmatch=s", "label=s", "limit=i", "link=s", "parser=s@", "store=s", "title=s", "to=s", "unique=s", "weight=i");

my $available_parser = join(", ", keys %parsers);

if (exists $args{"help"}) {
    print <<"EOT";
by $AUTHOR
Usage : $PROGNAME [--help] [--verbose|-v] [--dirmatch string] [--label string] [--limit n] [--link path] [--parser name] [--store file] [--title string] [--to path] [--weight n] [directories...]
  --help         : this message
  --verbose|-v   : verbose (display filenames/classes while processing)
  --dirmatch str : use string to match the directory name holding files (default: $default{dirmatch})
  --label str    : use string as label in class page (default: $default{label})
  --limit n      : limit the number of examples per parser type (default: $default{limit})
  --link path    : link to example files (and prepend path)
  --title str    : use string as title in "Related Pages" (default: $default{title})
  --parser name  : use specific parser only (available : $available_parser)
  --store file   : use 'file' to store xrefs (default: $default{store})
  --to path      : use 'path' as destination directory (default : $default{to})
  --unique str   : use string as a unique page identifier (otherwise MD5) (default : $default{unique})
  --weight n     : use 'n' as an approximation of the maximum page weight (default : $default{weight})

Example:
  $PROGNAME --verbose
  $PROGNAME --dirmatch "^Testing$" --label "Tests" --title "Class To Tests" --store "doc_class2tests.dox" --unique "t"
EOT
    exit;
}

$args{"verbose"} = 1 if exists $default{"verbose"};
$args{"dirmatch"} = $default{"dirmatch"} if ! exists $args{"dirmatch"};
$args{"label"} = $default{"label"} if ! exists $args{"label"};
$args{"limit"} = $default{"limit"} if ! exists $args{"limit"};
$args{"link"} = $default{"link"} if ! exists $args{"link"} && exists $default{"link"};
$args{"link"} =~ s/[\\\/]*$// if exists $args{"link"};
$args{"store"} = $default{"store"} if ! exists $args{"store"};
$args{"title"} = $default{"title"} if ! exists $args{"title"};
$args{"to"} = $default{"to"} if ! exists $args{"to"};
$args{"to"} =~ s/[\\\/]*$// if exists $args{"to"};
$args{"unique"} = $default{"unique"} if ! exists $args{"unique"};
$args{"weight"} = $default{"weight"} if ! exists $args{"weight"};

# Select parsers

my @parsers;
if (exists $args{"parser"}) {
    foreach my $parser (@{$args{"parser"}}) {
        if (exists $parsers{$parser}) {
            push @parsers, $parser;
        } else {
            carp "$PROGNAME: unknown parser : $parser\n";
        }
    }
} else {
    @parsers = keys %parsers;
}
    
my $os_is_win = ($^O =~ m/(MSWin32|Cygwin)/i);
my $open_file_as_text = $os_is_win ? O_TEXT : 0;
my $start_time = time();

# -------------------------------------------------------------------------
# Collect unique non-intersecting (top) examples' directories recursively
# Save current path (pwd) to avoid the ::prune bug

print "Collecting files...\n";

push @ARGV, @{$default{"dirs"}} if !@ARGV;
my (%seen, @dirs);

my $cwd = Cwd::cwd();

foreach my $file (@ARGV) {
    find sub { 
        if (-d $_ && 
            $_ ne "CVS" && 
            basename($File::Find::name) =~ m/$args{"dirmatch"}/i) {
            # my ($dev, $ino) = stat $_;
            push @dirs, $File::Find::name;
            # if ! $seen{$dev, $ino}++;
            # once we are in a "examples directory", don't descend anymore
            $File::Find::prune = 1;
        }
    }, $file;
}

chdir($cwd);

# -------------------------------------------------------------------------
# Collect files within directories recursively

my @files;
foreach my $dir (@dirs) {
    find sub {
        push @files, $File::Find::name if -f $_;
    }, $dir;
}

# -------------------------------------------------------------------------
# Select unique files matching available parsers

my @parsable;
foreach my $parser (@parsers) {
    foreach my $file (@files) {
        if ($file =~ m/$parsers{$parser}->[0]/) {
            # my ($dev, $ino) = stat $file;
            push @parsable, $file;
            # if ! $seen{$dev, $ino}++;
        }
    }
}

@parsable = sort @parsable;

print " => ", scalar @parsable, " file(s) collected in ", time() - $start_time, " s.\n";

# -------------------------------------------------------------------------
# Parse files and build xref

# %xref is a hash : xref{$class}{$parser}{$file}
# Ex: xref{"vtkPoints"}{"Tcl"}{"examplesTcl/foo.tcl"}
#     xref{"vtkPoints"}{"Tcl"}{"graphics/examplesTcl/bar.tcl"}
#     xref{"vtkNormals"}{"C++"}{"test.cxx"}

# %shorter_filename is a hash associating a filename and its shorter 
# counterpart where any leading component matching the name of a directory
# being browsed have been removed.

my %shorter_filename;

print "Parsing files...\n";

my $intermediate_time = time();
my %xref;

foreach my $file (@parsable) {

    # Read entire file into an array of lines

    sysopen(FILE, $file, O_RDONLY|$open_file_as_text)
      or croak "$PROGNAME: unable to open $file\n";
    my @file = <FILE>;
    close(FILE);

    # Submit the contents of the file to the corresponding parser

    foreach my $parser (@parsers) {
        if ($file =~ m/$parsers{$parser}->[0]/) {
            my @classes = $parsers{$parser}->[1]->(\@file);
            printf("%7s: %2d | ", $parser, scalar @classes)
              if exists $args{"verbose"};
            if (@classes) {
                $shorter_filename{$file} = $file;
                # print "(", join(", ", sort @classes), ") " if @classes;
                foreach my $class (@classes) {
                    $xref{$class}{$parser}{$file}++;
                }
            }
        }
    }
    print "=> ", $file, "\n" if exists $args{"verbose"};
}

foreach my $file (keys %shorter_filename) {
    foreach my $dir (@ARGV) {
        last if $shorter_filename{$file} =~ s/$dir//;
    }
}

print " => ", scalar @parsable, " file(s) parsed in ", time() - $intermediate_time, " s.\n";

# -------------------------------------------------------------------------
# Eliminate some 'fake' classes

print "Eliminating some classes...\n";

$intermediate_time = time();
my @eliminated = ();

foreach my $class (keys %xref) {
    if ($class =~ m/$eliminate_matcher/) {
        print "   $class\n" if exists $args{"verbose"};
        delete($xref{$class});
        push @eliminated, $class;
    }
}

print " => ", scalar @eliminated, " class(es) eliminated (", join(", ", @eliminated), ") in ", time() - $intermediate_time, " s.\n";

# -------------------------------------------------------------------------
# We need to locate the C++ header of each class to add a link from
# that header to the example page. We also remove orphan classes with
# no headers.

# %headers is a hash associating a class name to the location of its header.
#   ex: $headers{"vtkPoints"} = "Common/vtkPoints.h"
# %headers_not_found is a hash associating a class name to a flag that is
#   true (1) if the location of its header has not been found.

print "Locating headers to update...\n";

$intermediate_time = time();
my (%headers, %headers_not_found);

# Let's state that no headers have been found at the moment

foreach (keys %xref) {
    $headers_not_found{$_} = 1;
}
my $headers_not_found_nb = scalar keys %headers_not_found;

$cwd = Cwd::cwd();

find sub { 
    if ($headers_not_found_nb == 0) {
        # All headers have been found, let's stop descending the tree
        $File::Find::prune = 1;

    } elsif (-f $_ && $_ =~ /^(vtk[A-Z0-9][A-Za-z0-9]+)\.h$/) {
        # A class header has been found, let's check if it matches one pf
        # the class in xref
        my $class = $1;
        if (exists $headers_not_found{$class}) {
            print "   $class : $File::Find::name\n" if exists $args{"verbose"};
            $headers{$class} = $File::Find::name;
            $headers_not_found_nb--;
            delete($headers_not_found{$class});
        }
    }
}, $args{"to"};

chdir($cwd);

# Collect these classes that have not been associated to a header and
# remove them from xref.

my @still_not_found = keys %headers_not_found;
foreach my $not_found (@still_not_found) {
    print "   $not_found : not found (removed)\n" if exists $args{"verbose"};
    delete($xref{$not_found});
}

print " => ", scalar keys %headers, " found, ", scalar @still_not_found, " orphan class(es) removed (", join(", ", @still_not_found), ") in ", time() - $intermediate_time, " s.\n";

# -------------------------------------------------------------------------
# Build classes documentation and alphabetical section(s) weight(s)

print "Building classes doc and alphabetical section(s) weight(s)...\n";

$intermediate_time = time();

# @classes is the array holding the names of the classes to document
# %classes_doc is a hash associating a class name to its Doxygen doc (string)

my @classes = sort keys %xref;
my %classes_doc;

# $nb_of_parsers is the nb of times a parser has found something for a class.
# $nb_of_parsers_over_file_limit is the nb of times the nb of examples found 
# for a class (by a parser) was over the allowed number of examples per parser.

my ($nb_of_parsers, $nb_of_parsers_over_file_limit) = (0, 0);

# $indent is the indentation string
# %sections_classes is a hash associating a section (alphabetical letter) to
# an array of classes belonging to that section.
#   Ex: $sections_classes{"P"} => ("vtkPoints", "vtkProduct")
# %sections_weight is a hash associating a section to its weight (the sum of 
# the weights of each class belonging to that section).
# @sections is the array holding the name of all sections

my $indent = "    ";
my (%sections_classes, %sections_weight, @sections);

# $navbar is the Doxygen string describing the sections' navigation bar

my $navbar;

# $prefix is a unique prefix that should be append to each link

my $prefix = "c2_";
if (exists $args{"unique"}) {
    $prefix .= $args{"unique"};
} else {
    $prefix .= md5_hex($args{"label"} . $args{"title"});
}

# Browse each class

foreach my $class (@classes) {

    # Build doc (anchor, class name, example files sorted by parser and name)

    my @temp;
    push @temp, "\@anchor ${prefix}_$class", "$class";
    
    foreach my $parser (sort keys %{$xref{$class}}) {
        $nb_of_parsers++;
        push @temp, "  - $parser";
        my $count = 0;
        # @files if the set of files found for that class by that parser
        my @files = sort keys %{$xref{$class}{$parser}};
        $nb_of_parsers_over_file_limit++ if (scalar @files) > $args{"limit"};

        foreach my $file (@files) {
            last if ++$count > $args{"limit"};
            if (exists $args{"link"}) {
                push @temp, 
                '    - @htmlonly <TT><A href="' . $args{"link"} . '/' . 
                  $file . '">@endhtmlonly ' . $shorter_filename{$file} . 
                    '@htmlonly</A></TT> @endhtmlonly';
            } else {
                push @temp, "    - \@c $shorter_filename{$file}";
            }
        }
    }

    $classes_doc{$class} = $indent . join("\n$indent", @temp) . "\n";

    # Update section(s) and section(s) weight(s)

    $class =~ /^vtk(\w)/;
    my $section = $1;
    push @{$sections_classes{$section}}, $class;
    $sections_weight{$section} += length($classes_doc{$class});

    print " => ", $class, "\n" if exists $args{"verbose"};
}

print " => ", scalar @classes, " classes(s) documented in ", time() - $intermediate_time, " s.\n";

@sections = sort keys %sections_classes;

# Build the navbar

my @temp;
foreach my $section (@sections) {
    push @temp, "\@ref ${prefix}_section_$section \"$section\"";
}
$navbar = "$indent\@par Navigation: \n$indent\[" . join(" | ", @temp) . "]\n";

# Add the (approximate) weight of the (header + navbar) to each section

my $total_weight = 0;
my $header_weight = length($indent) + 24 + length($navbar);

foreach my $section (@sections) {
    $sections_weight{$section} += $header_weight;
    $total_weight += $sections_weight{$section};
}

if (exists $args{"verbose"}) {
    foreach my $section (@sections) {
        printf("\t- %s : %6d\n", $section, $sections_weight{$section});
    }
}

print " => total weight is $total_weight in ", scalar @sections, " section(s) (mean is ", int($total_weight / scalar @sections), ")\n";

# -------------------------------------------------------------------------
# Compute the alphabetical groups by joining sections depending on weights

print "Computing alphabetical group(s)/page(s)...\n";

# %groups is a hash associating a group id (int) to an array of sections names
# belonging to that group.
#   Ex: $groups{"0"} => ("A", "B", "C")
# %groups_weight is a hash associating a group id to its weight (the sum of 
# the weights of each section belonging to that group).

my (%groups, %groups_weight);

my $groupid = 0;

# Remove a section one by one, and put it in a group until the group if full,
# then create a next group, etc., until the sections are exhausted.

my @sections_temp = @sections;
while (@sections_temp) {
    $groups_weight{$groupid} = $sections_weight{$sections_temp[0]};
    push @{$groups{$groupid}}, shift @sections_temp;
    while (@sections_temp && 
           ($groups_weight{$groupid} + $sections_weight{$sections_temp[0]}) 
           <= $args{"weight"}) {
        $groups_weight{$groupid} += $sections_weight{$sections_temp[0]};
        push @{$groups{$groupid}}, shift @sections_temp;
    }
    $groupid++;
}

if (exists $args{"verbose"}) {
    foreach my $groupid (sort {$a <=> $b} keys %groups) {
        printf("\t- %02d (weight: %7d) : %s\n", $groupid, 
               $groups_weight{$groupid}, join(", ", @{$groups{$groupid}}));
    }
}

print " => max weight is ", $args{"weight"}, " per group/page, but a section can not be divided\n";
print " => ", scalar  keys %groups, " group(s) for ", scalar @sections, " section(s)\n";

# -------------------------------------------------------------------------
# Build pages header (summary, version)

print "Building pages header...\n";

# $header is the Doxygen string summarizing what has been documented as well
# as the credits.

my $header;
my (@summary, @credits);

push @summary, 
  "  - " . scalar @classes . " class(es) in " . 
  scalar @parsable . " file(s) from directories matching \@c " . $args{"dirmatch"} . " on " . localtime();

push @summary,
  "  - " . scalar @parsers . " parser(s) : [" . join(", ", @parsers) . "]";

push @summary, 
  "  - at most " . $args{"limit"} . " file(s) per parser (" . 
  int(($nb_of_parsers_over_file_limit / ($nb_of_parsers + 0.01)) * 100) . 
  "% over)";

push @credits, 
  "\@version $VERSION",
  "\@author \@c $PROGNAME, by $AUTHOR";

$header = $indent . join("\n$indent", @summary) . 
  "\n\n$indent" . join("\n$indent", @credits) . "\n\n";

# -------------------------------------------------------------------------
# Write documentation

my $destination_file = $args{"to"} . "/" . $args{"store"};
print "Writing documentation to ", $destination_file, "...\n";

$intermediate_time = time();

sysopen(DEST_FILE, 
        $destination_file, 
        O_WRONLY|O_TRUNC|O_CREAT|$open_file_as_text)
  or croak "$PROGNAME: unable to open destination file $destination_file\n";

# Browse each group, each section in this group, each class in this section

foreach my $groupid (sort {$a <=> $b} keys %groups) {

    my $fromto = $groups{$groupid}[0];
    $fromto .= ".." . $groups{$groupid}[scalar @{$groups{$groupid}} - 1]
      if scalar @{$groups{$groupid}} > 1;

    print DEST_FILE 
     "/*! \@page page_${prefix}_$groupid " . $args{"title"} . " ($fromto)\n\n$header"; 

    foreach my $section (@{$groups{$groupid}}) {
        print DEST_FILE 
          "\n$indent\@section ${prefix}_section_$section $section\n\n$navbar\n";

        foreach my $class (@{$sections_classes{$section}}) {
            print DEST_FILE $classes_doc{$class}, "\n";
        }
        print "\t- $section\n" if exists $args{"verbose"};
    }

    print DEST_FILE "*/\n\n";
}

close(DEST_FILE);

print join("\n", @summary), "\n => in ", time() - $intermediate_time, " s.\n";

# -------------------------------------------------------------------------
# Update class headers (add a link from the class header to the example page).
# Do not update if the section is already there.

print "Updating headers...\n";

$intermediate_time = time();
my $updated_nb = 0;

foreach my $class (@classes) {

    print "   $class => " . $headers{$class} . "\n" if exists $args{"verbose"};

    sysopen(HEADER, $headers{$class}, O_RDONLY|$open_file_as_text)
      or croak "$PROGNAME: unable to open " . $headers{$class} . "\n";
    
    # Search for the documentation block (@class ...)

    my @dest = ();
    my $line;
    while ($line = <HEADER>) {
        push @dest, $line;
        last if $line =~ /\s\@class\s+$class/;
    }

    # Search for the end of the block (*/), and check if the xref is not
    # already there (@par      $args{"label"})

    if (defined $line) {
        while ($line = <HEADER>) {
            last if $line =~ /^\*\// || $line =~ /^\s*\@par\s+$args{"label"}:\s*$/;
            push @dest, $line;
        }

        # If the example section was not found, insert the xref to the
        # example page, read the rest of the file, and overwrite the
        # header

        if (defined $line && $line =~ /^\*\//) {
            push @dest, "\n    \@par      " . $args{"label"} . ":\n",
                    "              \@ref ${prefix}_$class \"$class (" . $args{"label"} . ")\"\n",
                        $line;
            while ($line = <HEADER>) {
                push @dest, $line;
            }
            close(HEADER);
            sysopen(HEADER, $headers{$class}, 
                    O_WRONLY|O_TRUNC|O_CREAT|$open_file_as_text)
              or croak "$PROGNAME: unable to open " . $headers{$class} . "\n";
            print HEADER @dest;
            $updated_nb++;
        } 
    }
    close(HEADER);
}

print " => $updated_nb header(s) updated in ", time() - $intermediate_time, " s.\n";

print "Finished in ", time() - $start_time, " s.\n";
