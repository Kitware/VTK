#!/usr/bin/env perl
# Time-stamp: <2002-11-01 15:33:04 barre>
#
# Get author and contributors.
#
# barre : Sebastien Barre <sebastien@barre.nom.fr>
#
# 0.8 (barre) :
#   - Add --cvsweb and --cvsweb_suffix to report links to CVSweb logs
#
# 0.7 (barre) :
#   - Fix empty cached log file pb (regenerated)
#   - Add link from name to detailed description of contribution
#
# 0.6 (barre) :
#   - Change --history_img in order to create any contribution graph(s)
#
# 0.5 (barre) :
#   - Contribution graphs for the whole period (back to 1994).
#
# 0.4 (barre) :
#   - D'oh ! Month returned by localtime start with 0 (=january).
#
# 0.3 (barre) :
#   - add history feature + gnuplot fig
#
# 0.2 (barre) :
#   - now handles most files
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
use POSIX;
use strict;
use FileHandle;
use Time::Local;

my ($VERSION, $PROGNAME, $AUTHOR) = (0.8, $0, "Sebastien Barre");
$PROGNAME =~ s/^.*[\\\/]//;

# -------------------------------------------------------------------------
# Defaults  (add options as you want: "verbose" => 1 for default verbose mode)

my %default = 
  (
   dirs => ["../.."],
   cachedir => $ENV{TMP} . "/cache",
   class_group => '^(vtk[A-Z0-9][A-Za-z0-9]+)\.(?:c|cpp|cxx|h|mm)$',
   files_in => '(?:^hints|^README|\.(?:c|cmake|cpp|cxx|doc|h|html|in|java|mm|pl|py|tcl|txt))$',
   files_out => '(?:^vtkVersion\.\w+|^pkgIndex\.tcl|^vtkParse\.tab\.c|\.yy\.c)$',
   gnuplot_file => '../../../VTK-doxygen/contrib/history.plt',
   history_dir => '../../../VTK-doxygen/contrib',
   history_img => ['|lines|../../../VTK-doxygen/contrib/history.png',
                   '730|lines|../../../VTK-doxygen/contrib/history2y.png',
                   '180|linespoints|../../../VTK-doxygen/contrib/history6m.png'],
   history_max_nb => 12,
   lines_add => 1.0,
   lines_rem => 0.5,
   massive => 200,
   max_class_nb => 10,
   max_file_nb => 5,
   min_class => 0.02,
   min_file => 0.01,
   min_contrib => 0.05,
   min_gcontrib => 0.0001,
   relativeto => "../..",
   store => "doc_VTK_contributors.dox",
   to => "../../../VTK-doxygen"
  );

# -------------------------------------------------------------------------
# Parse options

my %args;
Getopt::Long::Configure("bundling");
GetOptions (\%args, "help", "verbose|v", "authors=s", "cachedir=s", "class_group=s", "cvsweb=s", "cvsweb_suffix=s", "files_in=s", "files_out=s", "gnuplot_file=s", "history_dir=s", "history_img=s@", "history_max_nb=i", "lines_add=f", "lines_rem=f", "massive=i", "max_class_nb=i", "max_file_nb=i", "min_class=f", "min_file=f", "min_contrib=f", "min_gcontrib=f", "relativeto=s", "store=s", "to=s");

print "$PROGNAME $VERSION, by $AUTHOR\n";

if (exists $args{"help"}) {
    print <<"EOT";
Usage : $PROGNAME [--help] [--verbose|-v] [--authors file] [--cachedir path] [--files_in string] [--files_out string] [--gnuplot_file string] [--history_dir string] [--history_img string] [--history_max_nb number] [--lines_add number] [--lines_rem number] [--massive number] [--max_class_nb number] [--max_file_nb number] [--min_class number] [--min_file number] [--min_contrib number] [--min_gcontrib number] [--store file] [--relativeto path] [--to path] [files|directories...]
  --help           : this message
  --verbose|-v     : verbose (display filenames while processing)
  --authors file   : use 'file' to read authors list
  --cachedir path  : use 'path' as cache directory for CVS logs (default: $default{cachedir})
  --files_in s     : accept only file names (without path) matching 's' (default: $default{files_in})
  --files_out s    : (then) reject file names (without path) matching 's' (default: $default{files_out})
  --gnuplot_file s : use 's' to store gnuplot command file (default: $default{gnuplot_file})
  --history_dir s  : history dir (default: $default{history_dir})
  --history_img d|s : create history image 's' graphing contribs up to 'd' ago (default: $default{history_img})
  --history_max_nb n : use at most 'n' authors in history pic (default: $default{history_max_nb})
  --lines_add n           : use 'n' as weight for added lines (default: $default{lines_add})
  --lines_rem n          : use 'n' as weight for removed lines (default: $default{lines_rem})
  --massive n      : use 'n' as minimum threshold for massive commits removal (default: $default{massive})
  --max_class_nb n : do not display more than 'n' classes by contributor (default: $default{max_class_nb})
  --max_file_nb n : do not display more than 'n' files by contributor (default: $default{max_file_nb})
  --min_class n    : display classes that represent more than 'n' % of author's contribution (default: $default{min_class})
  --min_file n    : display files that represent more than 'n' % of author's contribution (default: $default{min_file})
x  --min_contrib n  : display authors who represent more than 'n' % of classe's contribution (default: $default{min_contrib})
  --min_gcontrib n : display authors who represent more than 'n' % of total contribution (default: $default{min_gcontrib})
  --cvsweb s : use 's' as a base link to the CVSweb site
  --cvsweb_suffix s : use 's' as a suffix to the link to the CVSweb site
  --store file     : use 'file' to store doc (default: $default{store})
  --relativeto path: each file/directory to document is considered relative to 'path', where --to and --relativeto should be absolute (default: $default{relativeto})
  --to path        : use 'path' as destination directory (default: $default{to})

Example:
  $PROGNAME --to ../vtk-doxygen
  $PROGNAME contrib
EOT
    exit;
}

foreach my $option (
                    "cachedir",
                    "class_group",
                    "files_in",
                    "files_out",
                    "gnuplot_file",
                    "history_dir",
                    "history_img",
                    "history_max_nb",
                    "lines_add",
                    "lines_rem",
                    "massive",
                    "min_contrib",
                    "min_gcontrib",
                    "min_class",
                    "min_file",
                    "max_class_nb",
                    "max_file_nb",
                    "relativeto",
                    "store",
                    "to") {
    $args{$option} = $default{$option} 
      if ! exists $args{$option} && exists $default{$option};
}

$args{"verbose"} = 1 if exists $default{"verbose"};

$args{"cachedir"} =~ s/[\\\/]*$// if exists $args{"cachedir"};
$args{"history_dir"} =~ s/[\\\/]*$// if exists $args{"history_dir"};
$args{"relativeto"} =~ s/[\\\/]*$// if exists $args{"relativeto"};
$args{"to"} =~ s/[\\\/]*$// if exists $args{"to"};

my $os_is_win = ($^O =~ m/(MSWin32|Cygwin)/i);
my $open_file_as_text = $os_is_win ? O_TEXT : 0;

STDOUT->autoflush;
STDERR->autoflush;

my $start_time = time();

# -------------------------------------------------------------------------
# Read the authors list

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

my $authors_file_loaded = 0;

if (exists $args{"authors"}) {
    if (!sysopen(AUTHORS_FILE, 
                 $args{"authors"}, 
                 O_RDONLY|$open_file_as_text)) {
        carp "$PROGNAME: unable to open authors list ". $args{"authors"}. "\n";
    } else {
        print "Reading the authors list from ". $args{"authors"} . "...\n";
        
        my @authors_file = <AUTHORS_FILE>;
        close(AUTHORS_FILE);

        $authors_file_loaded = 1;

        foreach my $line (@authors_file) {
            if ($line && $line =~ m/^\s*(.+?)\s*:\s*(.+?)\s*\(\s*(.*?)\s*\)/) {
                my @aliases = split(',', $1);
                my $author = trim_spaces(shift @aliases);
                $authors{$author}{'name'} = $2;
                $authors{$author}{'email'} = $3;
                foreach my $alias (@aliases) {
                    $authors_aliases{trim_spaces($alias)} = $author;
                }
                print " >> $author: ", $authors{$author}{'name'}, 
                " (", $authors{$author}{'email'}, ")\n" 
                  if  exists $args{"verbose"};
            }
        }

        print " => ", scalar keys %authors, " authors(s) read from ", basename($args{"authors"}), ".\n";
    }
}

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
    my ($author, $date, $time, $lines_add, $lines_rem) = 
      (undef, undef, undef, undef, undef);

    my @fields = split (';', $fields_line);

    if ($fields[0] =~ m/date:\s+(\d+-\d+-\d+)\s+(\d+:\d+:\d+)\s+\+0000$/) {
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
        ($lines_add, $lines_rem) = ($1, $2);
    }

    return ($revision, $date, $time, $author, $lines_add, $lines_rem, $message);
}

# -------------------------------------------------------------------------
# Collect all files and directories

# Avoid all CVS/ directories and third-party libs directories (identified
# by a .NoDartCoverage file)

push @ARGV, @{$default{dirs}} if !@ARGV;

print "Collecting...\n";
my %files;
foreach my $file (@ARGV) {
    if (-f $file) {
        $files{$file} = 1;
    } elsif (-d $file) {
        find sub { 
            if ($File::Find::dir =~ /\bCVS$/ || 
                -e '.NoDartCoverage') {
                $File::Find::prune = 1;
            } else {
                $files{$File::Find::name} = 1 if -f $_;
            }
        }, $file;
    }
}

# -------------------------------------------------------------------------
# Process files and get logs

print "Processing files and getting logs...\n";

my $intermediate_time = time();
my $nb_revisions = 0;

# %log_by_file_revision is indexed by file and revision 
# (i.e. $log_by_file_revision{$file_name}{$revision_number})
# {'date'}: revision's date
# {'author'}: revision's author
# {'lines_add'}: number of lines added for that revision
# {'lines_rem'}: number of lines removed for that revision

my %log_by_file_revision;

# %log_revision_by_signature_file is indexed by signature and file
# (i.e. $log_revision_by_signature_file{$signature}{$file_name})
# (hash) all revisions matching this signature and that file.
# The signature is made of the rev date, author and message.

my %log_revision_by_signature_file;

my @files_submitted = sort keys %files;
my $nb_file_submitted = scalar @files_submitted;
my $nb_file_fraction = ceil($nb_file_submitted / 10.0);

my %files_visited;

sub get_short_relative_name {
    my ($file_name, $relative_to) = @_;
    $file_name =~ s/^$relative_to//;
    return $file_name;
}

foreach my $file_name (@files_submitted) {

    # print "   ? $file_name\n" if exists $args{"verbose"};

    # Progress meter

    if ($nb_file_submitted % $nb_file_fraction == 0) {
        print " - Remaining files: $nb_file_submitted\n";
    }
    --$nb_file_submitted;
        
    # Reject file if not matching pattern or already visited

    my $file_name_basename = basename($file_name);

    next if ($file_name_basename !~ m/$args{"files_in"}/ ||
             $file_name_basename =~ m/$args{"files_out"}/ ||
             exists $files_visited{$file_name});

    print "     $file_name\n" if exists $args{"verbose"};

    # Get the CVS log for that file or grab it from the cache
        
    my $cache_name = $args{"cachedir"} . 
      get_short_relative_name($file_name, $args{"relativeto"}) . '.log';
    my $output;

    my $old_slurp = $/;
    undef $/;                           # slurp mode
    
    # Use the cache if it exists, is not older than file, and not empty

    if (-e $cache_name && 
        (stat $cache_name)[9] >= (stat $file_name)[9] &&
        (stat $cache_name)[7]) {
        sysopen(CACHE_FILE, 
                $cache_name, 
                O_RDONLY|$open_file_as_text)
          or croak "$PROGNAME: unable to open cache file $cache_name\n";
        $output = <CACHE_FILE>;
        close(CACHE_FILE);
    } else {
        my $file_name_dir = dirname($file_name);
        print " >>  $file_name_dir: cvs log -b $file_name_basename\n" 
          if exists $args{"verbose"};
        my $current = cwd;
        chdir($file_name_dir);
        $output = qx/cvs log -b $file_name_basename/;
        chdir($current);
        mkpath(dirname($cache_name));
        sysopen(CACHE_FILE, 
                $cache_name, 
                O_WRONLY|O_TRUNC|O_CREAT|$open_file_as_text)
          or croak "$PROGNAME: unable to open cache file $cache_name\n";
        print CACHE_FILE $output;
        close(CACHE_FILE);
    }

    $/ = $old_slurp;

    if (! $output) {
        carp " >> Empty output from CVS log -b $file_name\n";
        next;
    }

    $files_visited{$file_name} = 1;

    # Process revisions

    my @file_revisions = 
      split('----------------------------\nrevision ', $output);
    shift @file_revisions;

    # Store each revision

    my $lines_added = 0;

    foreach my $revision (@file_revisions) {
        
        my ($revision, $date, $time, $author, 
            $lines_add, $lines_rem, $message) = parse_revision(\$revision);

        # Resolve author aliases and use full name
        
        $author = $authors_aliases{$author} 
          if exists $authors_aliases{$author};
        
        $nb_revisions++;
        
        # Store date/author
        
        $log_by_file_revision{$file_name}{$revision}{'date'} = $date;
        $log_by_file_revision{$file_name}{$revision}{'author'} = $author;

        # Store lines added and removed. Load the file and count the
        # lines for rev 1.1
        
        if ($revision ne '1.1') {
            $log_by_file_revision{$file_name}{$revision}{'lines_add'} 
              = $lines_add;
            $log_by_file_revision{$file_name}{$revision}{'lines_rem'} 
              = $lines_rem;

            $lines_added += $lines_add - $lines_rem;

            $log_revision_by_signature_file
              {"$date: $author: $message"}{$file_name}{$revision} = 1;
        }
    }

    # Now get the number of lines for 1.1 (not stored in the log)
    # Read the current file, add all changes

    sysopen(FILE, 
            $file_name, 
            O_RDONLY|$open_file_as_text)
      or croak "$PROGNAME: unable to open file $file_name\n";

    my @lines = <FILE>;
    close(FILE);
 
    $log_by_file_revision{$file_name}{'1.1'}{'lines_add'} 
      = (scalar @lines) - $lines_added;
    if ($log_by_file_revision{$file_name}{'1.1'}{'lines_add'} < 0) {
        print " >> $file_name: ", $log_by_file_revision{$file_name}{'1.1'}{'lines_add'}, " lines !\n";
    }
    $log_by_file_revision{$file_name}{'1.1'}{'lines_rem'} = 0;
}

print " => $nb_revisions revision(s) stored from ", scalar keys %files_visited, " file(s) in ", time() - $intermediate_time, " s. \n";

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
        print " >> Removed: (" . scalar @files . ")\n     " . 
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

print " => ", $nb_revisions - $nb_removed, " revision(s) kept ($nb_removed revision(s) removed) in ", time() - $intermediate_time, " s. \n";

# -------------------------------------------------------------------------
# Compute the contributions

print "Contributing contributions (sum(added * ", $args{"lines_add"}, " + removed * ", $args{"lines_rem"}, "))...\n";

# %contribution_by_author_file is indexed by author and file name
# (i.e $contribution_by_author_file{$author}{$file_name})
# stores the total contribution of this author for that specific file.

my %contribution_by_author_file;

# %contribution_by_author_date is indexed by author and date
# (i.e $contribution_by_author_date{$author}{$date})
# stores the total *accumulative* contribution of this author up to that date.
# (computed it 2 steps)

my %contribution_by_author_date;

# %contribution_by_author_class is indexed by author and class name
# (i.e $contribution_by_author_class{$author}{$class_name})
# stores the total contribution of this author for that specific class.

my %contribution_by_author_class;

# %classes is indexed by class name (i.e. $classes{$class_name})
# {'files'}: (hash) all files describing that class (header, implem, etc.)
# {'creators'}: (hash) all creators (authors of rev 1.1) for that class
# {'contributors'}: (hash) all contributors for that class

my %classes;

# %not_class_file_by_author is indexed by author
# (i.e $not_class_file_by_author{$author})
# (hash) files contributed by author but not part of a class group.

my %not_class_file_by_author;

$intermediate_time = time();

# Browse each file, each revision and use contribution
 
foreach my $file_name (keys %files_visited) {

    # Check if file is part of a class group
    
    my $class_name = undef;
    if (basename($file_name) =~ m/$args{"class_group"}/) {
        $class_name = $1;
        print " >> $class_name: $file_name\n" if exists $args{"verbose"};
        $classes{$class_name}{'files'}{$file_name} = 1;
        $classes{$class_name}{'creators'}{$log_by_file_revision{$file_name}{'1.1'}{'author'}} = 1;
    }

    foreach my $revision (keys %{$log_by_file_revision{$file_name}}) {

        my $author = $log_by_file_revision{$file_name}{$revision}{'author'};

        # Store contribution

        my $revision_contribution = 
          $log_by_file_revision{$file_name}{$revision}{'lines_add'} * 
            $args{"lines_add"} + 
              $log_by_file_revision{$file_name}{$revision}{'lines_rem'} * 
                $args{"lines_rem"};

        $contribution_by_author_file{$author}{$file_name} += 
          $revision_contribution;

        $contribution_by_author_date{$author}{$log_by_file_revision{$file_name}{$revision}{'date'}} += 
          $revision_contribution;

        # File is part of a class, store the whole class contributors
        # and contributions

        if (defined($class_name)) {

            $classes{$class_name}{'contributors'}{$author} = 1;

            $contribution_by_author_class{$author}{$class_name} += 
              $revision_contribution;
        } else {
            $not_class_file_by_author{$author}{$file_name} = 1;
        }
    }
}

print " => computed in ", time() - $intermediate_time, " s. \n";

# -------------------------------------------------------------------------
# Find contributors for each class, update header

print "Finding contributors > ", int($args{"min_contrib"} * 100.0), " and updating class headers...\n";

mkpath($args{"to"});

$intermediate_time = time();
my $nb_updated = 0;

my $class_name;
my @classes_names = sort { $b cmp $a } keys %classes;

while (@classes_names) {
    $class_name = pop @classes_names;

    # Sort the contributors for that class according to their
    # respective contributions. 

    my @class_contributors_sorted = 
      sort {$contribution_by_author_class{$b}{$class_name} <=> 
              $contribution_by_author_class{$a}{$class_name}}
        keys %{$classes{$class_name}{'contributors'}};

    # Compute the total contribution for that class

    my $total_class_contribution;
    foreach my $class_contributor (@class_contributors_sorted) {
        $total_class_contribution += 
          $contribution_by_author_class{$class_contributor}{$class_name};
    }

    # Find the class header name

    my $source_header_name = undef;
    foreach my $file_name (keys %{$classes{$class_name}{'files'}}) {
        if ($file_name =~ m/\.h$/) {
            $source_header_name = $file_name;
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
           sort map(exists $authors{$_}{'name'} ? $authors{$_}{'name'} : $_, keys %{$classes{$class_name}{'creators'}})) . "\n";

    $doc .= "\n    \@par      CVS contributions (if > " . int(100.0 * $args{"min_contrib"}) . "%):\n";

    foreach my $class_contributor (@class_contributors_sorted) {
        my $ratio = 
          $contribution_by_author_class{$class_contributor}{$class_name} / 
            $total_class_contribution;
        last if $ratio < $args{"min_contrib"};
        $doc .= "                - " . (exists $authors{$class_contributor}{'name'} ? $authors{$class_contributor}{'name'} : $class_contributor) . " (" . int($ratio * 100.0) . "%)\n";
    }

    if (exists $args{"cvsweb"}) {
        $doc .= "\n    \@par      CVS logs (CVSweb):\n";
        foreach my $file (keys %{$classes{$class_name}{'files'}}) {
            my $shortname = get_short_relative_name($file, $args{"relativeto"});
            my ($base, $dir, $ext) = fileparse($shortname, '\..*');
            $doc .= '                - @htmlonly<A href="' . $args{"cvsweb"} 
              . $shortname;
            $doc .= $args{"cvsweb_suffix"} if exists $args{"cvsweb_suffix"};
            $doc .= '">@endhtmlonly @c ' .  $ext 
              . ' @htmlonly</A>@endhtmlonly (@c ' . $shortname . ")\n";
        }
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
  "  - " . (scalar keys %contribution_by_author_file) . " authors(s) reported";

push @summary, 
  "  - Files matching \@c " . $args{"files_in"} . " and not matching \@c ". $args{"files_out"} . " are taken into account";

push @summary,
  "  - $nb_revisions CVS revision(s) processed from " . (scalar keys %files_visited) . " matching file(s)";

push @summary, 
  "  - " . ($nb_revisions - $nb_removed) . " revision(s) kept ($nb_removed revision(s) removed from 'massive commits' > " . $args{"massive"} . " changes per date/author/message)";

push @summary, 
  "  - revision's contribution is " . $args{"lines_add"} . " * (number of lines added) + " . $args{"lines_rem"} . " * (number of lines removed)";

push @summary,
  "  - " . (scalar keys %classes) . " class(es) found (matching \@c " . $args{"class_group"} . ")";

push @credits, 
  "\@version $VERSION",
  "\@author \@c $PROGNAME, by $AUTHOR";

$header = $indent . join("\n$indent", @summary) . "\n\n" .
  $indent . join("\n$indent", @credits) . "\n\n";

# -------------------------------------------------------------------------
# Update contributions stats

# %contribution_by_author is indexed by author
# (i.e $contribution_by_author{$author})
# stores the total contribution of this author for all files processed so far.

my %contribution_by_author;

# %contribution_by_date is indexed by date
# (i.e $contribution_by_date{$date})
# stores the total *accumulative* contribution up to that date 
# for all files processed so far.

my %contribution_by_date;

foreach my $contributor (keys %contribution_by_author_file) {
    
    foreach my $file_name 
      (keys %{$contribution_by_author_file{$contributor}}) {
        $contribution_by_author{$contributor} += 
          $contribution_by_author_file{$contributor}{$file_name};
    }

    my $accumulate = 0;

    foreach my $date 
      (sort {$a cmp $b} keys %{$contribution_by_author_date{$contributor}}) {
          $contribution_by_date{$date} += 
            $contribution_by_author_date{$contributor}{$date};
          $accumulate += $contribution_by_author_date{$contributor}{$date};
          $contribution_by_author_date{$contributor}{$date} = $accumulate;
      }
}

my $accumulate = 0;

foreach my $date 
  (sort {$a cmp $b} keys %contribution_by_date) {
      $accumulate += $contribution_by_date{$date};
      $contribution_by_date{$date} = $accumulate;
  }

my @all_contribution_dates = sort {$a cmp $b} keys %contribution_by_date;

my $last_contribution_date = $all_contribution_dates[$#all_contribution_dates];
my $first_contribution_date = $all_contribution_dates[0];
my $total_contribution = $contribution_by_date{$last_contribution_date};

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

sub compare_by_author_name {
    my $a_name = exists $authors{$a}{'name'} ? $authors{$a}{'name'} : $a;
    my $b_name = exists $authors{$b}{'name'} ? $authors{$b}{'name'} : $b;
    return lc $a_name cmp lc $b_name;
}

foreach my $contributor (sort compare_by_author_name 
                         keys %contribution_by_author_file) {

    my $c_ratio = $contribution_by_author{$contributor} / $total_contribution;

    my $name = exists $authors{$contributor}{'name'} 
      ? $authors{$contributor}{'name'} : $contributor;

    if ($c_ratio < $args{"min_gcontrib"}) {
        print DEST_FILE "$indent - $name\n";
    } else {
        print DEST_FILE "$indent - \@ref $contributor \"$name\"\n";
    }
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

    my $name = exists $authors{$contributor}{'name'} 
      ? $authors{$contributor}{'name'} : $contributor;

    print DEST_FILE "$indent -# \@ref $contributor \"$name\"\n";
}

foreach my $history (@{$args{"history_img"}}) {
    my ($delta, $style, $filename) = split(/\|/, $history);
    print DEST_FILE 
      "\n$indent\@image html " . basename($filename) . "\n";
}

print DEST_FILE 
  "\n$indent\@section contributors_detailed Detailed contributions (by decreasing order)\n";

print DEST_FILE 
  "\n$indent\@note Contributions lower than " . int($args{"min_gcontrib"} * 10000.0) / 100.0 . "% are not listed\n";

print DEST_FILE 
  "\n$indent\@note Details are:",
  "\n$indent       - % of global contribution at $last_contribution_date, % at date of last contribution, % at date of first contribution",
  "\n$indent       - first ", $args{"max_class_nb"}, " contributed classes > ", int($args{"min_class"} * 100.0), "% of author's total contribution",
  "\n$indent       - first ", $args{"max_file_nb"}, " contributed (non-class) files > ", int($args{"min_file"} * 100.0), "% of author's total contribution",
  "\n\n";

foreach my $contributor (@contributors_sorted) {

    # Yes, you can have 0 contrib:
    # vtkPiecewiseFunction.cxx:date: 1999/11/03 18:11:39;  
    # author: tpan;  state: Exp; lines: +0 -0

    next if $contribution_by_author{$contributor} == 0;

    my $c_ratio = $contribution_by_author{$contributor} / $total_contribution;
    last if $c_ratio < $args{"min_gcontrib"};

    # Classes

    my @ok_classes; 

    my @classes_sorted = 
      sort {$contribution_by_author_class{$contributor}{$b} <=> 
              $contribution_by_author_class{$contributor}{$a}}
        keys %{$contribution_by_author_class{$contributor}};
    
    foreach my $class_name (@classes_sorted) {
        last if scalar @ok_classes > $args{"max_class_nb"};
        my $ratio = 
          $contribution_by_author_class{$contributor}{$class_name} / 
            $contribution_by_author{$contributor};
        last if $ratio < $args{"min_class"};
        push @ok_classes, "$class_name (" . int($ratio * 100.0) . "%)";
    }
 
    # Files

    my @ok_files; 

    my @files_sorted = 
      sort {$contribution_by_author_file{$contributor}{$b} <=> 
              $contribution_by_author_file{$contributor}{$a}}
        keys %{$not_class_file_by_author{$contributor}};
 
    foreach my $file_name (@files_sorted) {
        next if basename($file_name) =~ m/$args{"class_group"}/;
        last if scalar @ok_files > $args{"max_file_nb"};
        my $ratio = 
          $contribution_by_author_file{$contributor}{$file_name} / 
            $contribution_by_author{$contributor};
        last if $ratio < $args{"min_file"};
        push @ok_files, get_short_relative_name($file_name, $args{"relativeto"}) . " (" . int($ratio * 100.0) . "%)";
    }

    my $name = exists $authors{$contributor}{'name'} 
      ? $authors{$contributor}{'name'} . " ($contributor)" : $contributor;

    print DEST_FILE "$indent -# \@anchor $contributor \@b $name:\n";

    my @contribution_dates = sort { $a cmp $b } 
      keys %{$contribution_by_author_date{$contributor}};

    my $last_contrib_date = $contribution_dates[$#contribution_dates];
    my $first_contrib_date = $contribution_dates[0];

    print DEST_FILE 
      "$indent   - \@b ", int(10000.0 * $c_ratio) / 100, "% ($last_contribution_date), ", int(10000.0 * ($contribution_by_author_date{$contributor}{$last_contrib_date} / $contribution_by_date{$last_contrib_date})) / 100, "% ($last_contrib_date), ", int(10000.0 * ($contribution_by_author_date{$contributor}{$first_contrib_date} / $contribution_by_date{$first_contrib_date})) / 100, "% ($first_contrib_date)\n";

    print DEST_FILE 
      "$indent   - ", (scalar @classes_sorted), " class(es): ", join(", ", @ok_classes), "...\n" if @classes_sorted;

    print DEST_FILE 
      "$indent   - ", (scalar @files_sorted), " file(s): ", join(", ", @ok_files), "...\n" if @files_sorted;
}

print DEST_FILE "\n*/\n\n";

# -------------------------------------------------------------------------
# Create history files

print "Creating history files in ", $args{"history_dir"}, "\n";

mkpath($args{"history_dir"});

foreach my $contributor (@contributors_sorted) {
    my $history_name = $args{"history_dir"} . "/$contributor.dat";

    sysopen(HISTORY_FILE, 
            $history_name, 
            O_WRONLY|O_TRUNC|O_CREAT|$open_file_as_text)
      or croak "$PROGNAME: unable to open history file $history_name\n";

    foreach my $date 
      (sort {$a cmp $b} keys %{$contribution_by_author_date{$contributor}}) {
          print HISTORY_FILE $date . ' ' . ($contribution_by_author_date{$contributor}{$date} / $contribution_by_date{$date}) * 100.0 . "\n";
      }

    close(HISTORY_FILE);
}

# -------------------------------------------------------------------------
# Create gnuplot file

print "Creating gnuplot command file\n", $args{"gnuplot_file"}, "\n";

my ($year, $month, $mday) = split('-', $last_contribution_date);

my $last_contribution_sec = timelocal(0, 0, 0, $mday, $month -1 , $year-1900); 

my $history_dir_abs = abs_path($args{"history_dir"});

my @plots;
foreach my $contributor (@contributors_sorted) {
    last if scalar @plots >= $args{"history_max_nb"};
    my $history_filename = $history_dir_abs . "/$contributor.dat";
    push @plots, "\"$history_filename\" using 1:2 title \"" . (exists $authors{$contributor}{'name'} ? $authors{$contributor}{'name'} : $contributor) . "\"" if -e $history_filename;
}

my $plot = 'plot ' . join(', ', @plots);

sysopen(GNUPLOT_FILE, 
        $args{"gnuplot_file"}, 
        O_WRONLY|O_TRUNC|O_CREAT|$open_file_as_text)
  or croak "$PROGNAME: unable to open gnuplot command file " . $args{"gnuplot_file"} . "\n";

print GNUPLOT_FILE <<EOT;
set grid

set xdata time
set timefmt "%Y/%m/%d"
set format x "%Y/%m"
set xlabel "Time (Year/Month)"

set ylabel "Contribution (%)"

set key bottom outside below Left title 'Legend:' nobox

set mytics 5

set size 1.2,1.4

set terminal png small

set timestamp top

EOT

my $date_from;
foreach my $history (@{$args{"history_img"}}) {
    my ($delta, $style, $filename) = split(/\|/, $history);
    if ($delta eq '') {
        print GNUPLOT_FILE "# Whole period\n\n";
        $date_from = $first_contribution_date;
        print GNUPLOT_FILE "set title \"Contributions over time (whole period: $date_from to $last_contribution_date)\"\n\n";
    } else {
        print GNUPLOT_FILE "# Back $delta days\n\n";
        ($mday, $month, $year) = 
          (localtime($last_contribution_sec - $delta * 24 * 60 * 60))[3, 4, 5];
        $date_from = sprintf("%04d/%02d/%02d", $year+1900, $month + 1, $mday);
        print GNUPLOT_FILE "set title \"Contributions over time ($delta days ago: $date_from to $last_contribution_date)\"\n\n";
    }
    print GNUPLOT_FILE "set xrange [\"$date_from\":]\n\n";
    print GNUPLOT_FILE "set output \"" . 
      abs_path(dirname($filename)) . '/' . basename($filename) . "\"\n\n";
    print GNUPLOT_FILE "set data style $style\n\n";
    print GNUPLOT_FILE "$plot\n\n";
    if ($plot ne 'replot') {
        $plot = 'replot';
    }
}

close(GNUPLOT_FILE);

# -------------------------------------------------------------------------

print "Finished in ", time() - $start_time, " s.\n";
