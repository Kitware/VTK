#!/usr/bin/env perl
# Time-stamp: <2002-01-29 22:39:16 barre>
#
# Get author and contributors.
#
# barre : Sebastien Barre <sebastien@barre.nom.fr>
#
# 0.1 (barre) :
#   - first release

use Carp;
use Cwd 'abs_path', 'cwd';
use Getopt::Long;
use Fcntl;
use File::Basename;
use File::Find;
use File::Path;
use strict;
use FileHandle;

my ($VERSION, $PROGNAME, $AUTHOR) = (0.1, $0, "Sebastien Barre");
$PROGNAME =~ s/^.*[\\\/]//;

# -------------------------------------------------------------------------
# Defaults  (add options as you want: "verbose" => 1 for default verbose mode)

my %default = 
  (
   dirs => ["../../Common", 
            "../../Filtering", 
            "../../Graphics", 
            "../../Hybrid", 
            "../../Imaging", 
            "../../IO", 
            "../../Parallel", 
            "../../Patented", 
            "../../Rendering"],
   authors => "authors.txt",
   cachedir => $ENV{TMP} . "/cache",
   in => 1.0,
   out => 0.5,
   massive => 200,
   max_class_nb => 10,
   min_class => 0.02,
   min_contrib => 0.05,
   min_gcontrib => 0.0001,
   relativeto => "",
   store => "doc_VTK_contributors.dox",
   to => "../../../VTK-doxygen"
  );

# -------------------------------------------------------------------------
# Parse options

my %args;
Getopt::Long::Configure("bundling");
GetOptions (\%args, "help", "verbose|v", "authors=s", "cachedir=s", "in=f", "out=f", "massive=i", "max_class_nb=i", "min_class=f", "min_contrib=f", "min_gcontrib=f", "relativeto=s", "store=s", "to=s");

print "$PROGNAME $VERSION, by $AUTHOR\n";

if (exists $args{"help"}) {
    print <<"EOT";
Usage : $PROGNAME [--help] [--verbose|-v] [--authors file] [--cachedir path] [--in number] [--out number] [--massive number] [--max_class_nb number] [--min_class number] [--min_contrib number] [--min_gcontrib number] [--store file] [--relativeto path] [--to path] [files|directories...]
  --help           : this message
  --verbose|-v     : verbose (display filenames while processing)
  --authors file   : use 'file' to read authors list (default: $default{authors})
  --cachedir path  : use 'path' as cache directory for CVS logs (default: $default{cachedir})
  --in n           : use 'n' as weight for added lines (default: $default{in})
  --out n          : use 'n' as weight for removed lines (default: $default{out})
  --massive n      : use 'n' as minimum threshold for massive commits removal (default: $default{massive})
  --max_class_nb n : do not display more than 'n' classes by contributor (default: $default{max_class_nb})
  --min_class n    : display classes that represent more than 'n' % of author's contribution (default: $default{min_class})
  --min_contrib n  : display authors who represent more than 'n' % of classe's contribution (default: $default{min_contrib})
  --min_gcontrib n : display authors who represent more than 'n' % of total contribution (default: $default{min_gcontrib})
  --store file     : use 'file' to store doc (default: $default{store})
  --relativeto path: each file/directory to document is considered relative to 'path', where --to and --relativeto should be absolute (default: $default{relativeto})
  --to path        : use 'path' as destination directory (default: $default{to})

Example:
  $PROGNAME --to ../vtk-doxygen
  $PROGNAME contrib
EOT
    exit;
}

$args{"verbose"} = 1 if exists $default{"verbose"};
$args{"authors"} = $default{"authors"} if ! exists $args{"authors"};
$args{"cachedir"} = $default{"cachedir"} if ! exists $args{"cachedir"};
$args{"cachedir"} =~ s/[\\\/]*$// if exists $args{"cachedir"};
$args{"in"} = $default{"in"} if ! exists $args{"in"};
$args{"out"} = $default{"out"} if ! exists $args{"out"};
$args{"massive"} = $default{"massive"} if ! exists $args{"massive"};
$args{"min_contrib"} = $default{"min_contrib"} 
  if ! exists $args{"min_contrib"};
$args{"min_gcontrib"} = $default{"min_gcontrib"} 
  if ! exists $args{"min_gcontrib"};
$args{"min_class"} = $default{"min_class"} 
  if ! exists $args{"min_class"};
$args{"max_class_nb"} = $default{"max_class_nb"} 
  if ! exists $args{"max_class_nb"};
$args{"relativeto"} = $default{"relativeto"} if ! exists $args{"relativeto"};
$args{"relativeto"} =~ s/[\\\/]*$// if exists $args{"relativeto"};
$args{"store"} = $default{"store"} if ! exists $args{"store"};
$args{"to"} = $default{"to"} if ! exists $args{"to"};
$args{"to"} =~ s/[\\\/]*$// if exists $args{"to"};

my $os_is_win = ($^O =~ m/(MSWin32|Cygwin)/i);
my $open_file_as_text = $os_is_win ? O_TEXT : 0;

STDOUT->autoflush;
STDERR->autoflush;

my $start_time = time();
    
# -------------------------------------------------------------------------
# Read the authors list

print "Reading the authors list from ". $args{"authors"} . "...\n";

sysopen(AUTHORS_FILE, 
        $args{"authors"}, 
        O_RDONLY|$open_file_as_text)
  or die "$PROGNAME: unable to open authors list ". $args{"authors"} . "\n";
my @authors_file = <AUTHORS_FILE>;
close(AUTHORS_FILE);

# %authors is indexed by login name (i.e. $authors{$name})
# {'name'}: full name of the user
# {'email'}: email of the user
#
# %authors_aliases is indexed by aliased login name
# $authors_aliases{$name} is the real login name for $name

my %authors;
my %authors_aliases;

sub trim_spaces {
    my @out = @_;
    for (@out) {
        s/^\s*//;
        s/\s*$//;
    }
    return wantarray ? @out : $out[0];
}

# Grab each author, line by line, setup aliases
# Example:
# will, schroede: Schroeder, Will (will.schroeder@kitware.com)

foreach my $line (@authors_file) {
    if ($line && $line =~ m/^\s*(.+?)\s*:\s*(.+?)\s*\(\s*(.*?)\s*\)/) {
        my @aliases = split(',', $1);
        my $author = trim_spaces(shift @aliases);
        $authors{$author}{'name'} = $2;
        $authors{$author}{'email'} = $3;
        foreach my $alias (@aliases) {
            $authors_aliases{trim_spaces($alias)} = $author;
        }
        print "  >> $author: ", $authors{$author}{'name'}, 
        " (", $authors{$author}{'email'}, ")\n" 
          if  exists $args{"verbose"};
    }
}

print " => ", scalar keys %authors, " authors(s) read from ", basename($args{"authors"}), ".\n";

# -------------------------------------------------------------------------
# Parse a revision
#
# Example:
# 1.2
# date: 2001/06/05 15:34:17;  author: barre;  state: Exp;  lines: +10 -3
# fix PrintSelf defects

sub parse_revision {
    my $ref = shift;

    $$ref =~ /([\d\.]*)\n(.*)\n(.*)/;

    my ($revision, $fields_line, $message) = ($1, $2, $3);
    my ($author, $date, $time, $lines_in, $lines_out) = 
      (undef, undef, undef, undef, undef);
    
    my @fields = split (';', $fields_line);

    if ($fields[0] =~ m/date:\s+(\d+\/\d+\/\d+)\s+(\d+:\d+:\d+)$/) {
        ($date, $time) = ($1, $2);
    } else {
        carp "Unable to find date of revision!\n";
    }
        
    if ($fields[1] =~ m/author:\s+(.+)$/) {
        $author = $1;
    } else {
        carp "Unable to find author of revision!\n";
    }
        
    if (exists $fields[3] && $fields[3] =~ m/lines:\s+\+(\d+)\s+\-(\d+)$/) {
        ($lines_in, $lines_out) = ($1, $2);
    }

    return ($revision, $date, $time, $author, $lines_in, $lines_out, $message);
}

# -------------------------------------------------------------------------
# Collect all files and directories

push @ARGV, @{$default{dirs}} if !@ARGV;

print "Collecting...\n";
my %files;
foreach my $file (@ARGV) {
    if (-f $file) {
        $files{$file} = 1;
    } elsif (-d $file) {
        find sub { $files{$File::Find::name} = 1; }, $file;
    }
}

# -------------------------------------------------------------------------
# Process files and get logs

print "Processing files and getting logs...\n";

my $intermediate_time = time();
my $nb_revisions = 0;

# %classes is indexed by class name (i.e. $classes{$class_name})
# {'files'}: (hash) all files describing that class (header, implem, etc.)
# {'authors'}: (hash) all authors (creators of rev 1.1) for that class

my %classes;

# %log_by_file_revision is indexed by file and revision 
# (i.e. $log_by_file_revision{$file_name}{$revision_number})
# {'date'}: revision's date
# {'author'}: revision's author
# {'lines_in'}: number of lines added for that revision
# {'lines_out'}: number of lines removed for that revision

my %log_by_file_revision;

# %log_revision_by_signature_file is indexed by signature and file
# (i.e. $log_revision_by_signature_file{$signature}{$file_name})
# (hash) all revisions matching this signature and that file.
# The signature is made of the rev date, author and message.

my %log_revision_by_signature_file;

mkpath($args{"cachedir"});
mkdir($args{"to"});

my @files_submitted = keys %files;
my $nb_file_submitted = scalar @files_submitted;
my $nb_file_fraction = int($nb_file_submitted / 10.0);
my %files_visited;
my $last_chdir = '';

foreach my $file_name (@files_submitted) {

    # Progress meter

    if ($nb_file_submitted % $nb_file_fraction == 0) {
        print " - Remaining files: $nb_file_submitted\n";
    }
    --$nb_file_submitted;
        
    # Check file syntax (vtk*) and get class name
    
    next if $file_name !~ m/(vtk([A-Z0-9])[A-Za-z0-9]+)\..+$/;
    my $class_name = $1;

    # Get CVS logs for different type of file (header, implementation)
    
    my $file_name_we = basename($file_name);
    $file_name_we =~ s/\.[^\.]+$/\./;
    my $file_name_abs_dir = abs_path(dirname($file_name));

    foreach my $extension ('h', 'cxx', 'mm') {
        
        # Keep track of files visited

        my $base = $file_name_we . $extension;
        my $name = $file_name_abs_dir . '/' . $base;
        next if exists $files_visited{$name} || ! -e $name;
        $files_visited{$name} = 1;

        print "     $base\n" if exists $args{"verbose"};

        $classes{$class_name}{'files'}{$name} = 1;

        # Get the CVS log for that file or grab it from the cache
        
        my $cache_name = $args{"cachedir"} . '/' . $base;
        my $output;

        my $old_slurp = $/;
        undef $/; # slurp mode  

        if (-e $cache_name && (stat $cache_name)[9] >= (stat $name)[9]) {
            sysopen(CACHE_FILE, 
                    $cache_name, 
                    O_RDONLY|$open_file_as_text)
              or croak "$PROGNAME: unable to open cache file $cache_name\n";
            $output = <CACHE_FILE>;
            close(CACHE_FILE);
        } else {
            print " >> [$file_name_abs_dir] cvs log $base\n" 
              if exists $args{"verbose"};
            my $current = cwd;
            chdir($file_name_abs_dir);
            $output = qx/cvs log $base/;
            chdir($current);
            sysopen(CACHE_FILE, 
                    $cache_name, 
                    O_WRONLY|O_TRUNC|O_CREAT|$open_file_as_text)
              or croak "$PROGNAME: unable to open cache file $cache_name\n";
            print CACHE_FILE $output;
            close(CACHE_FILE);
        }
        next if ! $output;
        $/ = $old_slurp;

        # Process revisions

        my @file_revisions = 
          split('----------------------------\nrevision ', $output);
        shift @file_revisions;

        # Store each revision (extract authors/creators from 1.1)

        foreach my $revision (@file_revisions) {

            my ($revision, $date, $time, $author, 
                $lines_in, $lines_out, $message) = parse_revision(\$revision);

            # Resolve author aliases and use full name

            $author = $authors_aliases{$author} 
              if exists $authors_aliases{$author};

            if (! exists $authors{$author}{'name'}) {
                carp "Unknown author $author in $name log!\n";
                next;
            }

            $author = $authors{$author}{'name'}; 

            $nb_revisions++;

            # Store date/author

            $log_by_file_revision{$name}{$revision}{'date'} = $date;
            $log_by_file_revision{$name}{$revision}{'author'} = $author;

            # Store lines added and removed. Load the file and count the
            # lines for rev 1.1

            if ($revision eq '1.1') {
                $classes{$class_name}{'authors'}{$author} = 1;

                sysopen(FILE, 
                        $name, 
                        O_RDONLY|$open_file_as_text)
                  or croak "$PROGNAME: unable to open file $name\n";
                my @lines = <FILE>;
                close(FILE);

                $log_by_file_revision{$name}{$revision}{'lines_in'} 
                  = scalar @lines;
                $log_by_file_revision{$name}{$revision}{'lines_out'} = 0;

            } else {
                $log_by_file_revision{$name}{$revision}{'lines_in'} 
                  = $lines_in;
                $log_by_file_revision{$name}{$revision}{'lines_out'} 
                  = $lines_out;

                $log_revision_by_signature_file
                  {"$date: $author: $message"}{$name}{$revision} = 1;
            }
        }
    }
}

print " => $nb_revisions revision(s) stored for ", scalar keys %classes, " class(es) from ", scalar keys %files_visited, " file(s) in ", time() - $intermediate_time, " s. \n";

# -------------------------------------------------------------------------
# Remove massive commits (copyright changes for example)

print "Removing massive commits (> " . $args{"massive"} . ") ...\n";

$intermediate_time = time();
my $nb_removed = 0;

# Remove these changes that have been commited to more than 'n' files
# in the same day by the same author with same log message (= signature)

foreach my $signature (sort { (scalar keys %{$log_revision_by_signature_file{$b}}) <=> (scalar keys %{$log_revision_by_signature_file{$a}}) } 
                       keys %log_revision_by_signature_file) {
    my @files = keys %{$log_revision_by_signature_file{$signature}};

    if (scalar @files > $args{"massive"}) {
        print "  >> Removed: (" . scalar @files . ")\n     " . 
          substr($signature, 0, 130) . "...\n\n" if exists $args{"verbose"};

        foreach my $name (@files) {
            foreach my $revision 
              (keys %{$log_revision_by_signature_file{$signature}{$name}}) {
                ++$nb_removed;
                delete $log_by_file_revision{$name}{$revision};
            }
        }
    }
}

print " => $nb_removed revision(s) removed out of $nb_revisions in ", time() - $intermediate_time, " s. \n";

# -------------------------------------------------------------------------
# Find contributors for each class, update header

print "Finding contributors > ", int($args{"min_contrib"} * 100.0), "% (sum(in * ", $args{"in"}, " + out * ", $args{"out"}, "))...\n";

# %contribution_by_author_class is indexed by author and class name
# (i.e $contribution_by_author_class{$author}{$class_name})
# stores the total contribution of this author for that specific class.

my %contribution_by_author_class;

# %last_contribution_date_by_author is indexed by author
# (i.e $last_contribution_date_by_author{$author})
# stores the last contribution date for that author.

my %last_contribution_date_by_author;

my $class_name;
my @classes_names = sort { $b cmp $a } keys %classes;

$intermediate_time = time();
my $nb_updated = 0;

# For each class, grab the revisions of all files describing this class
# and update the contribution of the revisions' authors for that class
# given the number of lines added and removed. Also update the last
# contribution date.

while (@classes_names) {
    $class_name = pop @classes_names;

    my %class_contributors;

    foreach my $name (keys %{$classes{$class_name}{'files'}}) {
        foreach my $revision (keys %{$log_by_file_revision{$name}}) {

            my $author = $log_by_file_revision{$name}{$revision}{'author'};

            $class_contributors{$author} = 1;

            $contribution_by_author_class{$author}{$class_name} += 
              $log_by_file_revision{$name}{$revision}{'lines_in'} * 
                $args{"in"} + 
                  $log_by_file_revision{$name}{$revision}{'lines_out'} * 
                    $args{"out"};

            $last_contribution_date_by_author{$author} = 
              $log_by_file_revision{$name}{$revision}{'date'} 
                if (! exists $last_contribution_date_by_author{$author} || 
                    $log_by_file_revision{$name}{$revision}{'date'} gt  
                    $last_contribution_date_by_author{$author});
        }
    }

    # Sort the contributors for that class according to their
    # respective contributions. 

    my @class_contributors_sorted = 
      sort {$contribution_by_author_class{$b}{$class_name} <=> 
              $contribution_by_author_class{$a}{$class_name}}
        keys %class_contributors;

    # Compute the total contribution for that class

    my $total_class_contribution;
    foreach my $class_contributor (@class_contributors_sorted) {
        $total_class_contribution += 
          $contribution_by_author_class{$class_contributor}{$class_name};
    }

    # Find the class header name

    my $source_header_name = undef;
    foreach my $name (keys %{$classes{$class_name}{'files'}}) {
        if ($name =~ m/\.h$/) {
            $source_header_name = $name;
            last;
        }
    }
    next if ! defined $source_header_name;

    # Figure out the name of the already-converted-to-doxygen header
    # file using --to and --relativeto destination file now

    my $header;

    # If source has absolute path, just use the basename (unless a
    # relativeto path has been set) otherwise remove the ../ component
    # before the source filename, so that it might be appended to the
    # "to" directory.

    if ($source_header_name =~ m/^(\/|[a-zA-W]\:[\/\\])/) {
        if ($args{"relativeto"}) {
            my ($dir, $absrel) = (abs_path(dirname($source_header_name)), 
                                  abs_path($args{"relativeto"}));
            $dir =~ s/$absrel//;
            $header = $args{"to"} . $dir . '/' . basename($source_header_name);
        } else {
            $header = $args{"to"} . '/' . basename($source_header_name);
        }
    } else {
        $source_header_name =~ s/^(\.\.[\/\\])*//;
        $header = $args{"to"} . '/' . $source_header_name;
    }

    next if ! -e $header;

    # Read that header

    my $old_slurp = $/;
    undef $/; # slurp mode  

    if (!sysopen(HEADERFILE, 
                 $header, 
                 O_RDONLY|$open_file_as_text)) {
        carp "$PROGNAME: unable to open $header\n";
        next;
    }

    my $headerfile = <HEADERFILE>;
    close(HEADERFILE);
    
    $/ = $old_slurp;

    # Search for the documentation block (@class ...)

    if ($headerfile !~ /(.*\/\*\!\s+)(\@class\s.+?)(\*\/.*)/gms) {
        carp  "$PROGNAME: no documentation block in $header ! (skipping)\n";
        next;
    }
    my ($pre, $block, $post) = ($1, $2, $3);

    # Create new doc section, insert it into block
    
    my $preamble = "    \@par      Created by:\n";

    my $doc = $preamble . "                - " . 
      join("\n                - ", 
           sort keys %{$classes{$class_name}{'authors'}}) . "\n";

    $doc .= "\n    \@par      Contributed by (if > " . int(100.0 * $args{"min_contrib"}) . "%):\n";

    foreach my $class_contributor (@class_contributors_sorted) {
        my $ratio = 
          $contribution_by_author_class{$class_contributor}{$class_name} / 
            $total_class_contribution;
        last if $ratio < $args{"min_contrib"};
        $doc .= "                - $class_contributor (" . int($ratio * 100.0) . "%)\n";
    }

    if ($block !~ s/($preamble.+?)(\s*\@par|\z)/$doc$2/gms) {
        $block .= "\n$doc";
    }

    # Write new header

    if (!sysopen(HEADERFILE, $header, 
                 O_WRONLY|O_TRUNC|O_CREAT|$open_file_as_text)) {
        carp "$PROGNAME: unable to open $header\n";
        next;
    }

    print HEADERFILE $pre . $block . $post;
    close(HEADERFILE);

    print " >> Updating $header\n" if exists $args{"verbose"};

    $nb_updated++;
}

print " => $nb_updated header(s) updated in ", time() - $intermediate_time, " s. \n";

# -------------------------------------------------------------------------
# Build the page summary documentation

# $indent is the indentation string

my $indent = "    ";

# $header is the Doxygen string summarizing what has been documented as well
# as the credits.

my $header;
my (@summary, @credits);

push @summary, 
  "  - " . (scalar keys %authors) . " authors(s) read from " . basename($args{"authors"});

push @summary,
  "  - $nb_revisions CVS revision(s) processed for " . (scalar keys %classes) . " class(es) from " . (scalar keys %files_visited) . " file(s)";


push @summary, 
  "  - $nb_removed revision(s) removed from 'massive commits' > " . $args{"massive"} . " changes per date/author/message";

push @summary, 
  "  - revision's contribution is " . $args{"in"} . " * (number of lines added) + " . $args{"out"} . " * (number of lines removed)";

push @summary, 
  "  - Tcl, Python, Java or Perl scripts as well as examples and tests are not taken into account";

push @credits, 
  "\@version $VERSION",
  "\@author \@c $PROGNAME, by $AUTHOR";

$header = $indent . join("\n$indent", @summary) . "\n\n" .
  $indent . join("\n$indent", @credits) . "\n\n";

# -------------------------------------------------------------------------
# Build documentation

my $destination_file = $args{"to"} . "/" . $args{"store"};
print "Building documentation to ", $destination_file, "\n";

sysopen(DEST_FILE, 
        $destination_file, 
        O_WRONLY|O_TRUNC|O_CREAT|$open_file_as_text)
  or croak "$PROGNAME: unable to open destination file $destination_file\n";

print DEST_FILE 
  "/*! \@page contributors Contributors\n\n$header"; 

print DEST_FILE 
  "\n$indent\@section contributors_alphabetical Contributors (by alphabetical order)\n\n";

# %contribution_by_author is indexed by author
# (i.e $contribution_by_author{$author})
# stores the total contribution of this author for all files processed so far.

my %contribution_by_author;
my $total_contribution;

foreach my $contributor (sort keys %contribution_by_author_class) {
    print DEST_FILE "$indent - $contributor\n";

    foreach my $class_name 
      (keys %{$contribution_by_author_class{$contributor}}) {
        $contribution_by_author{$contributor} += 
          $contribution_by_author_class{$contributor}{$class_name};
    }
    $total_contribution += $contribution_by_author{$contributor};
}

print DEST_FILE 
  "\n$indent\@section contributors_decreasing Contributors (by decreasing order of global contribution)\n";

print DEST_FILE 
  "\n$indent\@note Contributions lower than " . int($args{"min_gcontrib"} * 10000.0) / 100.0 . "% are not listed\n\n";

my @contributors_sorted = 
  sort {$contribution_by_author{$b} <=> $contribution_by_author{$a}} 
  keys %contribution_by_author;

foreach my $contributor (@contributors_sorted) {

    my $c_ratio = $contribution_by_author{$contributor} / $total_contribution;
    last if $c_ratio < $args{"min_gcontrib"};
    
    print DEST_FILE "$indent -# $contributor\n";
}

print DEST_FILE 
  "\n$indent\@section contributors_detailed Detailed contributions (by decreasing order)\n";

print DEST_FILE 
  "\n$indent\@note Contributions lower than " . int($args{"min_gcontrib"} * 10000.0) / 100.0 . "% are not listed\n";

print DEST_FILE 
  "\n$indent\@note Details are: % of global contribution, last date of contribution and first ", $args{"max_class_nb"}, " contributed classes > ", int($args{"min_class"} * 100.0), "% of author's total contribution\n\n";

foreach my $contributor (@contributors_sorted) {

    # Yes, you can have 0 contrib:
    # vtkPiecewiseFunction.cxx:date: 1999/11/03 18:11:39;  
    # author: tpan;  state: Exp; lines: +0 -0

    next if $contribution_by_author{$contributor} == 0;

    my $c_ratio = $contribution_by_author{$contributor} / $total_contribution;
    last if $c_ratio < $args{"min_gcontrib"};

    my @ok; 

    my @classes_sorted = 
      sort {$contribution_by_author_class{$contributor}{$b} <=> 
              $contribution_by_author_class{$contributor}{$a}}
        keys %{$contribution_by_author_class{$contributor}};
    
    foreach my $class_name (@classes_sorted) {
        last if scalar @ok > $args{"max_class_nb"};
        my $ratio = 
          $contribution_by_author_class{$contributor}{$class_name} / 
            $contribution_by_author{$contributor};
        last if $ratio < $args{"min_class"};
        push @ok, "$class_name (" . int($ratio * 100.0) . "%)";
    }
    
    print DEST_FILE 
      "$indent -# $contributor:\n",
      "$indent   - \@b ", int(10000.0 * $c_ratio) / 100, "%\n",
      "$indent   - ", $last_contribution_date_by_author{$contributor}, "\n",
      "$indent   - ", join(", ", @ok), "\n";
}

print DEST_FILE "\n*/\n\n";

# -------------------------------------------------------------------------

print "Finished in ", time() - $start_time, " s.\n";

