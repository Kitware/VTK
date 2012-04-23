#!/usr/bin/env perl
# Time-stamp: <2001-11-21 17:40:48 barre>
#
# Build full-text index
#
# barre : Sebastien Barre <sebastien@barre.nom.fr>
#
# 0.22 (barre) :
#   - use common build_page_doc proc
#
# 0.21 (barre) :
#   - add --project name : project name, used to uniquify
#
# 0.2 (barre) :
#   - update to match the new VTK 4.0 tree
#   - change default --dirs so that it can be launched from Utilities/Doxygen
#   - change default --stop so that it can be launched from Utilities/Doxygen
#   - change default --to so that it can be launched from Utilities/Doxygen
#   - the "class to example" page is now split in different pages
#   - use --weight to increase or decrease the maximum weight of a page
#
# 0.16 (barre) :
#   - added 'parallel' to the default set of directories
#
# 0.15 (barre) :
#   - change default --to to '../vtk-doxygen' to comply with Kitware's doxyfile
#   - change default --stop to 'wrap/doc_index.stop' to comply with the source
#     tree structure.
#
# 0.14 (barre) :
#   - change doxygen command style from \ to @ to match javadoc, autodoc, etc.
#
# 0.13 (barre) :
#   - change default --to to '../vtk-dox'
#
# 0.12 (barre)
#   - change relevancy sorting : for each word, classes are sorted by presence
#     of the word in the class name, *then* by occurence of the word in the
#     class documentation, *then* by alphabetical order
#
# 0.11 (barre)
#   - fix O_TEXT flag problem
#   - switch to Unix CR/LF format
#
# 0.1 (barre)
#   - first release

use Carp;
use Getopt::Long;
use Fcntl;
use File::Find;
use strict;

my ($VERSION, $PROGNAME, $AUTHOR) = (0.22, $0, "Sebastien Barre");
$PROGNAME =~ s/^.*[\\\/]//;
print "$PROGNAME $VERSION, by $AUTHOR\n";

# -------------------------------------------------------------------------
# Defaults (add options as you want : "verbose" => 1 for default verbose mode)

my %default =
  (
   limit => 10,
   dirs => ["../../Charts",
            "../../Chemistry",
            "../../Common",
            "../../Filtering",
            "../../GenericFiltering",
            "../../GenericFiltering/Testing/Cxx",
            "../../Geovis",
            "../../Graphics",
            "../../GUISupport/MFC",
            "../../GUISupport/Qt",
            "../../Hybrid",
            "../../Imaging",
            "../../Infovis",
            "../../IO",
            "../../Parallel",
            "../../Rendering",
            "../../Views",
            "../../VolumeRendering",
            "../../Widgets"],
   project => "VTK",
   stop => "doc_index.stop",
   store => "doc_VTK_index.dox",
   to => "../../../VTK-doxygen",
   weight => 90000
  );

# -------------------------------------------------------------------------
# Parse options

my %args;
Getopt::Long::Configure("bundling");
GetOptions (\%args, "help", "verbose|v", "debug", "limit=i", "project=s", "stop=s", "store=s", "to=s", "weight=i");

if (exists $args{"help"}) {
    print <<"EOT";
Usage : $PROGNAME [--help] [--verbose|-v] [--limit n] [--stop file] [--store file] [--to path] [--weight n] [files|directories...]
  --help         : this message
  --verbose|-v   : verbose (display filenames while processing)
  --limit n      : limit the number of xrefs per word (default: $default{limit})
  --project name : project name, used to uniquify (default: $default{project})
  --stop file    : use 'file' to read stop-words (default: $default{stop})
  --store file   : use 'file' to store index (default: $default{store})
  --to path      : use 'path' as destination directory (default : $default{to})
  --weight n     : use 'n' as an approximation of the maximum page weight (default : $default{weight})

Example:
  $PROGNAME
EOT
    exit;
}

$args{"debug"} = $default{"debug"} if exists $default{"debug"};
$args{"verbose"} = 1 if exists $default{"verbose"};
$args{"limit"} = $default{"limit"} if ! exists $args{"limit"};
$args{"project"} = $default{"project"} if ! exists $args{"project"};
$args{"stop"} = $default{"stop"} if ! exists $args{"stop"};
$args{"store"} = $default{"store"} if ! exists $args{"store"};
$args{"to"} = $default{"to"} if ! exists $args{"to"};
$args{"to"} =~ s/[\\\/]*$// if exists $args{"to"};
$args{"weight"} = $default{"weight"} if ! exists $args{"weight"};

my $os_is_win = ($^O =~ m/(MSWin32|Cygwin)/i);
my $open_file_as_text = $os_is_win ? O_TEXT : 0;
my $start_time = time();

# -------------------------------------------------------------------------
# Read the stop-words

print "Reading stop-words from $args{stop}...\n";

sysopen(STOPFILE, $args{"stop"}, O_RDONLY|$open_file_as_text)
  or die "$PROGNAME: unable to open stop words list $args{stop}\n";
my @stop_file = <STOPFILE>;
close(HEADERFILE);

my %stop_words;
foreach my $stop_word (@stop_file) {
    if ($stop_word && $stop_word !~ m/^\s*#/) {
        chop $stop_word;
        $stop_words{$stop_word} = 1;
    }
}

print " => ", scalar keys %stop_words, " stop-word(s) read.\n";

# -------------------------------------------------------------------------
# Collect all files and directories

print "Collecting files...\n";

push @ARGV, @{$default{dirs}} if !@ARGV;

my @files;
foreach my $file (@ARGV) {
    if (-f $file) {
        push @files, $file;
    } elsif (-d $file) {
        find sub { push @files, $File::Find::name; }, $file;
    }
}

# -------------------------------------------------------------------------
# Index files corresponding to headers

print "Indexing...\n";
my $intermediate_time = time();

# %index associates a word with a class name and reports how many times that
# word was found in that comment.
# Example: $index{"contour"}{"vtkMarchingCubes"} = 2

my %index;
keys(%index) = 7000;
my %group;

my $nb_files = 0;
undef $/;  # slurp mode

foreach my $source (@files) {

    # Skip what is not a vtk header

    next if $source !~ /(vtk[^\\\/]*)\.h\Z/;
    my $class = $1;

    ++$nb_files;
    print "  $source\n" if exists $args{"verbose"};

    # Open the file, read it entirely

    sysopen(HEADERFILE, $source, O_RDONLY|$open_file_as_text)
      or croak "$PROGNAME: unable to open $source\n";
    my $headerfile = <HEADERFILE>;
    close(HEADERFILE);

    # Grab all comments then skip the first one (preamble and copyright stuff)

    my @comments = $headerfile =~ m/(?:\/\*(.+?)\*\/|\/\/(.+?)$)/gms;
    shift @comments;

    # Grab (and count) each word in each comment and associate it with
    # the class name

    foreach my $comment (@comments) {
        next if ! $comment;
        my @words = $comment =~ m/\b([A-Za-z][A-Za-z-]*[A-Za-z]+)\b/gms;
        foreach my $word (@words) {
            $index{$word}{$class}++ if $word;
        }
    }
}

my @words = keys %index;
print " => ", scalar @words, " word(s) grabbed in $nb_files file(s) in ", time() - $intermediate_time, " s.\n";

# -------------------------------------------------------------------------
# Remove some words

print "Removing...\n";

my $nb_removed = 0;
foreach my $word (@words) {
    my ($len, $lcw, $ucw) = (length($word), lc($word), uc($word));
    if ($len <= 2 ||                         # too small
        ($len == 3 && $ucw ne $word) ||      # small and not an accronym
        (ucfirst($word) ne ucfirst($lcw) && $ucw ne $word) || # mixed case
        $word =~ m/^vtk/ ||                  # VTK function/class
        exists $stop_words{lc($word)}) {     # found in stop-words
        delete $index{$word};
        ++$nb_removed;
    }
}

print " => $nb_removed word(s) removed.\n";

# -------------------------------------------------------------------------
# Group some words

print "Grouping...\n";

sub transfer_keys {
    my ($rfrom, $rto) = @_;
    foreach my $key (keys %$rfrom) {
        $rto->{$key} += $rfrom->{$key};
    }
}

@words = sort keys %index;
my $nb_grouped = 0;

foreach my $word (@words) {

    my ($len, $lcw, $similar) = (length($word), lc($word));
    my (@similars, %verbs, %exts) = ((), (), ());

    # Now first try to get a list of words similar to the current one

    # Lowercase form ?

    if ($word ne $lcw) {
        push @similars, $lcw;
    }

    # Singular form ?

    if ($word =~ m/s$/i) {
        $similar = substr($word, 0, $len - 1);
        push @similars, lc($similar), $similar;
    }

    # Singular form ? (dashes -> dash)

    if ($word =~ m/[hsx]es$/i) {
        $similar = substr($word, 0, $len - 2);
        push @similars, lc($similar), $similar;

    # Singular form ? (leaves -> leaf)

    } elsif ($word =~ m/ves$/i) {
        $similar = substr($word, 0, $len - 3) . 'f';
        push @similars, lc($similar), $similar;
    }

    # Colour -> color

    if ($word =~ m/our$/i) {
        $similar = substr($word, 0, $len - 2) . 'r';
        push @similars, lc($similar), $similar;
    }

    # Thick -> thickness

    if ($word =~ m/ness$/i) {
        $similar = substr($word, 0, $len - 4);
        push @similars, lc($similar), $similar;
    }

    # Explicitly -> explicit

    if ($word =~ m/ly$/i) {
        $similar = substr($word, 0, $len - 2);
        push @similars, lc($similar), $similar;
    }

    # Accuracy -> accurate

    if ($word =~ m/acy$/i) {
        ($similar = $word) =~ s/cy$/te/i;
        push @similars, lc($similar), $similar;
    }

    # Rounded, rounding -> round

    if ($word =~ m/.{4,}(ed|ing|ten)$/i) {
        $exts{$1} = 1;
        ($similar = $word) =~ s/(ed|ing|ten)$//i;
        $verbs{$similar} = 1;
    }

    # Not try to see if it's not a verb (and keep its "extension")

    # Mapped, mapping -> map

    if ($word =~ m/.{3,}[bglmnpt](ed|ing)$/i) {
        $exts{$1} = 1;
        ($similar = $word) =~ s/[bglmnpt](ed|ing)$//i;
        $verbs{$similar} = 1;
    }

    # Applied -> apply

    if ($word =~ m/.{3,}(ied)$/i) {
        $exts{$1} = 1;
        ($similar = $word) =~ s/ied$/y/i;
        $verbs{$similar} = 1;
    }

    # Description -> descript

    if ($word =~ m/.{4,}[ts](ion)s?$/i) {
        $exts{$1} = 1;
        ($similar = $word) =~ s/ions?$//i;
        $verbs{$similar} = 1;
    }

    # Now we have a list of verb and extension, try to associate each verb
    # with these extensions that were not found and build a list of similar
    # "words" by concatenating both.

    my @verbs = keys %verbs;
    if (@verbs) {
        my %try = ("" => 1,
                   "e" => 1,
                   "ed" => 1, "ied" => 1,
                   "es" => 1,
                   "ing" => 1,
                   "ion" => 1,
                   "s" => 1,
                   "ten" => 1);
        foreach my $ext (sort keys %exts) {
            delete $try{$ext};
        }
        foreach my $verb (@verbs) {
            my $lcverb = lc $verb;
            print " -> ", $lcverb, "\n" if exists $args{"debug"};
            foreach my $ext (sort keys %try) {
                print "    +> ", $lcverb . $ext, "\n" if exists $args{"debug"};
                push @similars, $lcverb . $ext, $verb . $ext;
            }
        }
    }

    # Browse each similar word. It it already exists in the index then group
    # the current word with it and remove the word from the index.

    foreach $similar (@similars) {
        if (exists $index{$similar}) {
            print "- grouping $word with $similar\n" if exists $args{"debug"};
            transfer_keys(\%{$index{$word}}, \%{$index{$similar}});
            delete $index{$word};
            $group{$similar}{$word}++;
            transfer_keys(\%{$group{$word}}, \%{$group{$similar}});
            delete $group{$word};
            ++$nb_grouped;
            last;
        }
    }
}

print " => $nb_grouped word(s) grouped.\n";

# -------------------------------------------------------------------------
# Normalize to lowercase except if all uppercase

print "Normalizing...\n";

@words = keys %index;
foreach my $word (@words) {
    my $lcw = lc $word;

    # Normalize word to lowercase

    if ($word ne uc($word) && $word ne $lcw) {
        transfer_keys(\%{$index{$word}}, \%{$index{$lcw}});
        delete $index{$word};
        transfer_keys(\%{$group{$word}}, \%{$group{$lcw}});
        delete $group{$word};
    }

    # Normalize group to lowercase

    if (exists $group{$word}) {
        foreach my $gword (keys %{$group{$word}}) {
            my $lcgw = lc $gword;
            if ($gword ne uc($gword) && $gword ne $lcgw) {
                $group{$word}{$lcgw} = $group{$word}{$gword};
                delete $group{$word}{$gword};
            }
        }
        delete $group{$word}{$word};
    }
}

print " => normalized to lowercase.\n";

# -------------------------------------------------------------------------
# Build the page summary documentation

# $indent is the indentation string

my $indent = "    ";

# $header is the Doxygen string summarizing what has been documented as well
# as the credits.

my $header;
my (@summary, @credits);

push @summary,
  "  - $nb_files file(s) indexed by " . scalar @words . " word(s) on " .
  localtime(),
  "  - max limit is " . $args{"limit"} . " xref(s) per word";

push @credits,
  "\@version $VERSION",
  "\@author \@c $PROGNAME, by $AUTHOR";

$header = $indent . join("\n$indent", @summary) .
  "\n\n$indent" . join("\n$indent", @credits) . "\n\n";

# -------------------------------------------------------------------------
# Index to class

print "Building page doc...\n";

# @words is the array of words to document

my @words = sort keys %index;

# $prefix is a unique prefix that is appended to each link

my $prefix = "idx_" . $args{"project"};
$prefix = lc($prefix);

# word_section_name returns the short string describing a word section

sub word_section_name {
    my ($word) = @_;
    my @group = sort keys %{$group{$word}};
    $word .= " (" . join(", ", @group) . ")" if @group;
    return $word;
}

# word_section_doc returns the doxygen doc for a word

sub word_section_doc {
    my ($word) = @_;
    my @xrefs = sort { (($b =~ m/$word/i) <=> ($a =~ m/$word/i)) || ($index{$word}{$b} <=> $index{$word}{$a}) || ($a cmp $b)} (keys %{$index{$word}});
    my @xrefs_lim;
    my $count = 0;
    foreach my $xref (@xrefs) {
        last if ++$count > $args{"limit"};
        push @xrefs_lim, $xref . " (" . $index{$word}{$xref} . ")";
    }
    my $string = "  - " . join(", ", @xrefs_lim);
    $string .= ", [...]" if scalar keys %{$index{$word}} > $args{"limit"};
    return $string . "\n";
}

# word_section_alpha returns the single alpha char corresponding to that
# word's section.

sub word_section_alpha {
    my ($word) = @_;
    $word =~ /^(\w)/;
    return $1;
}

my $page_doc = build_page_doc($indent,
                              "Full-text Index",
                              \@words,
                              $prefix,
                              \&word_section_name,
                              \&word_section_doc,
                              \&word_section_alpha,
                              $header,
                              "",
                              $args{"to"} . "/" . $args{"store"});

print join("\n", @summary), "\n";
print "Finished in ", time() - $start_time, " s.\n";

# -------------------------------------------------------------------------

sub build_page_doc {

    # $indent is the indentation string
    # $rwords is a reference to the array of words to document
    # $prefix is a unique prefix that is appended to each link
    # word_section_name returns the short string describing a word section
    # word_section_doc returns the doxygen doc for a word
    # word_section_alpha returns the single alpha char corresponding to that
    # word's section.
    # $header is the Doxygen string summarizing what has been documented as
    # well as the credits.
    # $footer is a Doxygen string appended to each the resulting page
    # $destination_file is the name of the file where this page should be
    # written to.

    my ($ident, $title, $rwords, $prefix, $rword_section_name, $rword_section_doc, $rword_section_alpha, $header, $footer, $destination_file) = @_;

    # %words_doc is a hash associating a word to its Doxygen doc (string)

    my %words_doc;

    # %sections_words is a hash associating a section (alphabetical letter) to
    # an array of words belonging to that section.
    #   Ex: $sections_words{"C"} => ("contour", "cut")
    # %sections_weight is a hash associating a section to its weight (the sum
    # of the weights of each word belonging to that section).
    # @sections is the array holding the name of all sections

    my (%sections_words, %sections_weight, @sections);

    # $navbar is the Doxygen string describing the sections' navigation bar

    my $navbar;

    my $intermediate_time = time();

    # Browse each word

    foreach my $word (@$rwords) {

        my @temp;
        push @temp, &$rword_section_name($word), &$rword_section_doc($word);
        $words_doc{$word} = $indent . join("\n$indent", @temp) . "\n";

        # Update section(s) and section(s) weight(s)

        my $section = &$rword_section_alpha($word);
        push @{$sections_words{$section}}, $word;
        $sections_weight{$section} += length($words_doc{$word});

        print " => ", $word, "\n" if exists $args{"verbose"};
    }

    print " => ", scalar @$rwords, " words(s) documented in ", time() - $intermediate_time, " s.\n";

    @sections = sort keys %sections_words;

    # Build the navbar

    my @temp;
    foreach my $section (@sections) {
        push @temp, "\@ref ${prefix}_section_$section \"$section\"";
    }
    $navbar = "$indent\@par Navigation: \n$indent\[" .
      join(" | ", @temp) . "]\n";

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

    # Compute the alphabetical groups by joining sections depending on weights

    print "Computing alphabetical group(s)/page(s)...\n";

    # %groups is a hash associating a group id (int) to an array of sections
    # namesbelonging to that group.
    #   Ex: $groups{"0"} => ("A", "B", "C")
    # %groups_weight is a hash associating a group id to its weight (the sum
    # of the weights of each section belonging to that group).

    my (%groups, %groups_weight);

    my $groupid = 0;

    # Remove a section one by one, and put it in a group until the group if
    # full,then create a next group, etc., until the sections are exhausted.

    my @sections_temp = @sections;
    while (@sections_temp) {
        $groups_weight{$groupid} = $sections_weight{$sections_temp[0]};
        push @{$groups{$groupid}}, shift @sections_temp;
        while (@sections_temp &&
               ($groups_weight{$groupid} +$sections_weight{$sections_temp[0]})
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

    # Build documentation page
    # Browse each group, each section in this group, each word in this section

    my $page_doc;

    foreach my $groupid (sort {$a <=> $b} keys %groups) {

        my $fromto = $groups{$groupid}[0];
        $fromto .= ".." . $groups{$groupid}[scalar @{$groups{$groupid}} - 1]
          if scalar @{$groups{$groupid}} > 1;

        $page_doc .=
          "/*! \@page ${prefix}_$groupid $title ($fromto)\n\n$header";

        foreach my $section (@{$groups{$groupid}}) {
            $page_doc .=
              "\n$indent\@section ${prefix}_section_$section $section\n\n$navbar\n";
            foreach my $word (@{$sections_words{$section}}) {
                $page_doc .= $words_doc{$word}, "\n";
            }
            print "\t- $section\n" if exists $args{"verbose"};
        }
        $page_doc .= "$footer\n*/\n\n";
    }

    print "Writing documentation to ", $destination_file, "...\n";

    $intermediate_time = time();

    sysopen(DEST_FILE,
            $destination_file,
            O_WRONLY|O_TRUNC|O_CREAT|$open_file_as_text)
     or croak "$PROGNAME: unable to open destination file $destination_file\n";
    print DEST_FILE $page_doc;
    close(DEST_FILE);

    print " => written in ", time() - $intermediate_time, " s.\n";
}

