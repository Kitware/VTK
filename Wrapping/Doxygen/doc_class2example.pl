#!/usr/bin/env perl
# Time-stamp: <2001-06-28 02:21:12 barre>
#
# Build cross-references between classes and examples
#
# barre : Sebastien Barre <barre@sic.sp2mi.univ-poitiers.fr>
#
# 0.55 (barre) :
#   - change default --to to '../vtk-doxygen' to comply with Kitware's doxyfile.
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
use Getopt::Long;
use Fcntl;
use File::Basename;
use File::Find;
use strict;

my ($VERSION, $PROGNAME, $AUTHOR) = (0.55, $0, "Sebastien Barre");
$PROGNAME =~ s/^.*[\\\/]//;

# Defaults (add options as you want : "v" => 1 for default verbose mode)

my %default = 
  (
   dirs => ["."],
   limit => 20,
   store => "doc_class2example.dox",
   to => "../vtk-doxygen"
  );

# Matchers and parsers
# dir_matcher : shall match directory names containing example files
# eliminate_matcher : shall match 'fake' class names to be eliminated
# parsers : parser_name => [filename_matcher, function_to_call]
#            function_to_call receives the contents of the file as a reference
#            to an array of lines and returns an array of class names

my $dir_matcher = '(^|[\\\/])examples';
my $eliminate_matcher = '^vtkCommand$';
my %parsers = (
               "Tcl" => ['\.tcl$', \&parse_tcl],
               "C++" => ['\.cxx$', \&parse_tcl],
               "Python" => ['\.py$', \&parse_python]
              );

# Parse options

my %args;
Getopt::Long::Configure("bundling");
GetOptions (\%args, "v", "limit=i", "link=s", "parser=s@", "store=s", "to=s", "help|?");

my $available_parser = join(", ", keys %parsers);

if (exists $args{"help"}) {
    print <<"EOT";
$PROGNAME $VERSION
by $AUTHOR
Usage : $PROGNAME [--help|?] [-v] [--limit n] [--link path] [--parser name] [--store file] [--to path] [directories...]
  --help|?      : this message
  -v            : verbose (display filenames/classes while processing)
  --limit n     : limit the number of examples per parser type (default: $default{limit})
  --link path   : link to example files (and prepend path)
  --parser name : use specific parser only (available : $available_parser)
  --store file  : use 'file' to store xrefs (default: $default{store})
  --to path     : use 'path' as destination directory (default : $default{to})

Example:
  $PROGNAME -v --link ../../vtk
  $PROGNAME --parser tcl --parser python
EOT
    exit;
}

$args{"v"} = 1 if exists $default{"v"};
$args{"limit"} = $default{"limit"} if ! exists $args{"limit"};
$args{"link"} = $default{"link"} if ! exists $args{"link"} && exists $default{"link"};
$args{"link"} =~ s/[\\\/]*$// if exists $args{"link"};
$args{"store"} = $default{"store"} if ! exists $args{"store"};
$args{"to"} = $default{"to"} if ! exists $args{"to"};
$args{"to"} =~ s/[\\\/]*$// if exists $args{"to"};

print "$PROGNAME $VERSION, by $AUTHOR\n";

my $os_is_win = ($^O =~ m/(MSWin32|Cygwin)/i);
my $open_file_as_text = $os_is_win ? O_TEXT : 0;
    
# Tcl parser (seems to work with C++ too for the moment)

sub parse_tcl {
    my ($ref, %classes) = (shift, ());
    foreach my $line (@$ref) {
        if ($line =~ /^\s*(vtk[A-Z0-9][A-Za-z0-9]+)\s/) {
            $classes{$1}++;
        }
    }
    return keys %classes;
}

# Python parser

sub parse_python {
    my ($ref, %classes) = (shift, ());
    foreach my $line (@$ref) {
        if ($line =~ /=\s*(vtk[A-Z0-9][A-Za-z0-9]+)\(\)\s*$/) {
            $classes{$1}++;
        }
    }
    return keys %classes;
}

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

my $start_time = time();
print "Collecting files...\n";

# Collect unique non-intersecting (top) directories recursively
# Save current path (pwd) to avoid the ::prune bug

push @ARGV, @{$default{"dirs"}} if !@ARGV;
my (%seen, @dirs);
my $cwd = Cwd::cwd();
foreach my $file (@ARGV) {
    find sub { 
        if (-d $_ && $_ ne "CVS" && $File::Find::name =~ m/$dir_matcher/) {
            my ($dev, $ino) = stat $_;
            push @dirs, $File::Find::name;
# if ! $seen{$dev, $ino}++;
            $File::Find::prune = 1;
        }
    }, $file;
}
chdir($cwd);

# Collect files within directories recursively

my @files;
foreach my $dir (@dirs) {
    find sub {
        push @files, $File::Find::name if -f $_;
    }, $dir;
}

# Select unique files matching available parsers

my @parsable;
foreach my $parser (@parsers) {
    foreach my $file (@files) {
        if ($file =~ m/$parsers{$parser}->[0]/) {
            my ($dev, $ino) = stat $file;
            push @parsable, $file;
# if ! $seen{$dev, $ino}++;
        }
    }
}

@parsable = sort @parsable;

print " => ", scalar @parsable, " file(s) collected in ", time() - $start_time, " s.\n";
my $intermediate_time = time();

# Parse files and build xref

my %xref;
print "Parsing files...\n";
foreach my $file (@parsable) {

    sysopen(FILE, $file, O_RDONLY|$open_file_as_text)
      or croak "$PROGNAME: unable to open $file\n";
    my @file = <FILE>;
    close(FILE);

    foreach my $parser (@parsers) {
        if ($file =~ m/$parsers{$parser}->[0]/) {
            my @classes = $parsers{$parser}->[1]->(\@file);
            printf("%7s: %2d | ", $parser, scalar @classes)
              if exists $args{"v"};
            if (@classes) {
                # print "(", join(", ", sort @classes), ") " if @classes;
                foreach my $class (@classes) {
                    $xref{$class}{$parser}{$file}++;
                }
            }
        }
    }
    print "=> ", $file, "\n" if exists $args{"v"};
}

print " => ", scalar @parsable, " file(s) parsed in ", time() - $intermediate_time, " s.\n";
$intermediate_time = time();

# Eliminate some 'fake' classes

print "Eliminating some classes...\n";
my @eliminated = ();
foreach my $class (keys %xref) {
    if ($class =~ m/$eliminate_matcher/ && exists $xref{$class}) {
        print "   $class\n" if exists $args{"v"};
        delete($xref{$class});
        push @eliminated, $class;
    }
}

print " => ", scalar @eliminated, " class(es) eliminated (", join(", ", @eliminated), ") in ", time() - $intermediate_time, " s.\n";
$intermediate_time = time();

# We need the headers to add a link from the class header to the example page
# Locate headers and remove orphan classes with no headers

my (%headers, %headers_not_found);
foreach (keys %xref) {
    $headers_not_found{$_} = 1;
}
my $headers_not_found_nb = scalar keys %headers_not_found;

print "Locating headers to update...\n";
$cwd = Cwd::cwd();
find sub { 
    if ($headers_not_found_nb == 0) {
        $File::Find::prune = 1;
    } elsif (-f $_ && $_ =~ /^(vtk[A-Z0-9][A-Za-z0-9]+)\.h$/) {
        my $class = $1;
        if (exists $headers_not_found{$class}) {
            print "   $class : $File::Find::name\n" if exists $args{"v"};
            $headers{$class} = $File::Find::name;
            $headers_not_found_nb--;
            delete($headers_not_found{$class});
        }
    }
}, $args{"to"};
chdir($cwd);

my @still_not_found = keys %headers_not_found;
foreach my $not_found (@still_not_found) {
    print "   $not_found : not found (removed)\n" if exists $args{"v"};
    delete($xref{$not_found});
}

print " => ", scalar keys %headers, " found, ", scalar @still_not_found, " orphan class(es) removed (", join(", ", @still_not_found), ") in ", time() - $intermediate_time, " s.\n";
$intermediate_time = time();

# Build documentation

my $destination_file = $args{"to"} . "/" . $args{"store"};
print "Building documentation to ", $destination_file, "...\n";

my (@classes, @nav_bar, @body) = (sort keys %xref, (), ());
my ($prev_section, $ident, $nb_proc, $nb_limited) = ("_", "    ", 0, 0);

foreach my $class (@classes) {

    # Check alphabetical section, update nav bar

    $class =~ /^vtk(\w)/;
    my $section = $1;
    if ($section ne $prev_section) {
        push @nav_bar, "\@ref ex_section_$section \"$section\"";
        push @body, "\n", $ident, "\@section ex_section_$section $section\n";
        $prev_section = $section;
    }

    # Add anchor and class name

    push @body, "\n", $ident, "\@anchor ex_$class\n";
    push @body, $ident, "$class\n";

    # Add example files, sorted by parser and name

    foreach my $parser (sort keys %{$xref{$class}}) {
        push @body, $ident, "  - $parser\n";
        my @files = sort keys %{$xref{$class}{$parser}};
        printf("%7s: %2d | ", $parser, scalar @files) if exists $args{"v"};
        ++$nb_limited if @files > $args{"limit"};
        ++$nb_proc;
        my $count = 0;
        foreach my $file (@files) {
            last if ++$count > $args{"limit"};
            if (exists $args{"link"}) {
                push @body, $ident, '    - @htmlonly <TT><A href="' . $args{"link"} . '/' . $file . '">@endhtmlonly ' . $file . '@htmlonly</A></TT> @endhtmlonly' . "\n";
            } else {
                push @body, $ident, "    - $file\n";
            }
        }
    }
    print "=> ", $class, "\n" if exists $args{"v"};
}

my @summary;
push @summary, scalar @classes . " class(es) examplified by " . scalar @parsable . " file(s) on " . localtime();
push @summary, scalar @parsers . " parser(s) : [" . join(", ", @parsers) . "]";
push @summary, "max limit is " . $args{"limit"} . " example(s) per parser (" . int(($nb_limited / ($nb_proc + 0.01)) * 100) . "% over)";

# Write documentation

sysopen(DEST_FILE, $destination_file, O_WRONLY|O_TRUNC|O_CREAT|$open_file_as_text)
  or croak "$PROGNAME: unable to open destination file " . $destination_file . "\n";

print DEST_FILE 
  "/*! \@page page_ex Examples\n\n",
  $ident, "  - ", join("\n" . $ident . "  - ", @summary), "\n\n",
  $ident, "\@version $VERSION\n",
  $ident, "\@author \@c $PROGNAME, by $AUTHOR\n",
  $ident, "\@par Navigation:\n",
  $ident, "[", join(" | ", @nav_bar), "]\n", 
  @body, 
  "*/";

close(DEST_FILE);
print " => ", join("\n => ", @summary), "\n => in ", time() - $intermediate_time, " s.\n";
$intermediate_time = time();

# Update headers
# Do not update if 'Examples' section is already there

print "Updating headers...\n";

my $updated_nb = 0;
foreach my $class (@classes) {

    print "   $class => " . $headers{$class} . "\n" if exists $args{"v"};
    sysopen(HEADER, $headers{$class}, O_RDONLY|$open_file_as_text)
      or croak "$PROGNAME: unable to open " . $headers{$class} . "\n";
    
    # Search for documentation block 

    my @dest = ();
    my $line;
    while ($line = <HEADER>) {
        push @dest, $line;
        last if $line =~ /\s\@class\s+$class/;
    }

    # Search for end of block, and check if ref not already there

    if (defined $line) {
        while ($line = <HEADER>) {
            last if $line =~ /^\*\// || $line =~ /^\s*\@par\s+Examples:\s*$/;
            push @dest, $line;
        }

        # Insert reference to example page, read rest, and overwrite

        if (defined $line && $line =~ /^\*\//) {
            push @dest, "\n    \@par      Examples:\n",
                    "              \@ref ex_$class \"$class (examples)\"\n",
                        $line;
            while ($line = <HEADER>) {
                push @dest, $line;
            }
            close(HEADER);
            sysopen(HEADER, $headers{$class}, O_WRONLY|O_TRUNC|O_CREAT|$open_file_as_text)
              or croak "$PROGNAME: unable to open " . $headers{$class} . "\n";
            print HEADER @dest;
            $updated_nb++;
        } 
    }
    close(HEADER);
}

print " => $updated_nb header(s) updated in ", time() - $intermediate_time, " s.\n";
$intermediate_time = time();

print "Finished in ", time() - $start_time, " s.\n";
