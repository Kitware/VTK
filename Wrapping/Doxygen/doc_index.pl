#!/usr/bin/env perl
# Time-stamp: <2000-08-02 14:38:38 barre>
#
# Build full-text index 
#
# barre : Sebastien Barre <barre@sic.sp2mi.univ-poitiers.fr>
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

my ($VERSION, $PROGNAME, $AUTHOR) = (0.14, $0, "S. Barre");
$PROGNAME =~ s/^.*[\\\/]//;

# Defaults (add options as you want : "v" => 1 for default verbose mode)

my %default = 
  (
#   debug => 1,
   limit => 10,
   dirs => ["common", "contrib", "graphics", "imaging", "patented"],
   stop => "doc_index.stop",
   store => "doc_index.dox",
   to => "../vtk-dox"
  );

# Parse options

my %args;
Getopt::Long::Configure("bundling");
GetOptions (\%args, "v", "limit=i", "stop=s", "store=s", "to=s", "help|?");

if (exists $args{"help"}) {
    print <<"EOT";
$PROGNAME $VERSION, by $AUTHOR
Usage : $PROGNAME [--help|?] [-v] [--limit n] [--stop file] [--store file] [--to path] [files|directories...]
  --help|?     : this message
  -v           : verbose (display filenames while processing)
  --limit n    : limit the number of xrefs per word (default: $default{limit})
  --stop file  : use 'file' to read stop-words (default: $default{stop})
  --store file : use 'file' to store index (default: $default{store})
  --to path    : use 'path' as destination directory (default : $default{to})

Example:
  $PROGNAME --to ../vtk-dox
EOT
    exit;
}

$args{"debug"} = 1 if exists $default{"debug"};
$args{"v"} = 1 if exists $default{"v"};
$args{"limit"} = $default{"limit"} if ! exists $args{"limit"};
$args{"stop"} = $default{"stop"} if ! exists $args{"stop"};
$args{"store"} = $default{"store"} if ! exists $args{"store"};
$args{"to"} = $default{"to"} if ! exists $args{"to"};
$args{"to"} =~ s/[\\\/]*$// if exists $args{"to"};

print "$PROGNAME $VERSION, by $AUTHOR\n";

my $start_time = time();

my $os_is_win = ($^O =~ m/(MSWin32|Cygwin)/i);
my $open_file_as_text = $os_is_win ? O_TEXT : 0;

# Read the stop-words

print "Reading stop-words...\n";

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

print scalar keys %stop_words, " word(s) read.\n";

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

# Index files corresponding to headers

print "Indexing...\n";

my %index;
keys(%index) = 7000;
my %group;

my $nb_files = 0;
undef $/;  # slurp mode

foreach my $source (@files) {
    if ($source =~ /(vtk[^\\\/]*)\.h\Z/) {
        my $class = $1;

        ++$nb_files;
	print "  $source\n" if exists $args{"v"};

	sysopen(HEADERFILE, $source, O_RDONLY|$open_file_as_text)
	  or croak "$PROGNAME: unable to open $source\n";
        my $headerfile = <HEADERFILE>;
	close(HEADERFILE);

        # Parse comments, skip first one (= preamble and copyright stuff)

        my @comments = $headerfile =~ m/(?:\/\*(.+?)\*\/|\/\/(.+?)$)/gms;
        shift @comments;
        foreach my $comment (@comments) {
            if ($comment) {
                my @words = $comment =~ m/\b([A-Za-z][A-Za-z-]+)\b/gms;
                foreach my $word (@words) {
                    if ($word) {
                        $index{$word}{$class}++;
                    }
                }
            }
        }
    }
}

my @words = keys %index;
print scalar @words, " word(s) grabbed in $nb_files file(s).\n";

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

print "$nb_removed word(s) removed.\n";

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

    # lowercase form ?
    if ($word ne $lcw) {
        push @similars, $lcw;
    }
    
    # singular form ?
    if ($word =~ m/s$/i) {
        $similar = substr($word, 0, $len - 1);
        push @similars, lc($similar), $similar;
    }

    # singular form ? (dashes -> dash)
    if ($word =~ m/[hsx]es$/i) {
        $similar = substr($word, 0, $len - 2);
        push @similars, lc($similar), $similar;
    # singular form ? (leaves -> leaf)
    } elsif ($word =~ m/ves$/i) {
        $similar = substr($word, 0, $len - 3) . 'f';
        push @similars, lc($similar), $similar;
    }

    # colour -> color
    if ($word =~ m/our$/i) {
        $similar = substr($word, 0, $len - 2) . 'r';
        push @similars, lc($similar), $similar;
    }

    # thick -> thickness
    if ($word =~ m/ness$/i) {
        $similar = substr($word, 0, $len - 4);
        push @similars, lc($similar), $similar;
    }

    # explicitly -> explicit
    if ($word =~ m/ly$/i) {
        $similar = substr($word, 0, $len - 2);
        push @similars, lc($similar), $similar;
    }

    # accuracy -> accurate
    if ($word =~ m/acy$/i) {
        ($similar = $word) =~ s/cy$/te/i;
        push @similars, lc($similar), $similar;
    }

    # rounded, rounding -> round
    if ($word =~ m/.{4,}(ed|ing|ten)$/i) {
        $exts{$1} = 1;
        ($similar = $word) =~ s/(ed|ing|ten)$//i;
        $verbs{$similar} = 1;
    }

    # mapped, mapping -> map
    if ($word =~ m/.{3,}[bglmnpt](ed|ing)$/i) {
        $exts{$1} = 1;
        ($similar = $word) =~ s/[bglmnpt](ed|ing)$//i;
        $verbs{$similar} = 1;
    }

    # applied -> apply
    if ($word =~ m/.{3,}(ied)$/i) {
        $exts{$1} = 1;
        ($similar = $word) =~ s/ied$/y/i;
        $verbs{$similar} = 1;
    }

    # description -> descript
    if ($word =~ m/.{4,}[ts](ion)s?$/i) {
        $exts{$1} = 1;
        ($similar = $word) =~ s/ions?$//i;
        $verbs{$similar} = 1;
    }

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

print "$nb_grouped word(s) grouped.\n";

# Normalize to lowercase except if all uppercase

print "Normalizing...\n";

@words = keys %index;
foreach my $word (@words) {
    my $lcw = lc $word;

    # Normalize word
    if ($word ne uc($word) && $word ne $lcw) {
        transfer_keys(\%{$index{$word}}, \%{$index{$lcw}});
        delete $index{$word};
        transfer_keys(\%{$group{$word}}, \%{$group{$lcw}});
        delete $group{$word};
    }

    # Normalize group
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

print "normalized to lowercase.\n";

# Build documentation

my $destination_file = $args{"to"} . "/" . $args{"store"};
print "Building documentation to ", $destination_file, "...\n";

my (@nav_bar, @body) = ((), ());
my ($prev_section, $ident) = ("_", "    ");

@words = sort keys %index;
foreach my $word (@words) {

    # Check alphabetical section, update nav bar

    $word =~ /^(\w)/;
    my $section = $1;
    my $section_link = (uc($section) eq $section ? 'u_' : 'l_') . $section;
    if ($section ne $prev_section) {
        push @nav_bar, "\@ref idx_section_$section_link \"$section\"";
        push @body, "\n", $ident, "\@section idx_section_$section_link $section\n";
        $prev_section = $section;
    }

    # Add word and group

    push @body, $ident, $word;
    my @group = sort keys %{$group{$word}};
    push @body, " (", join(", ", @group), ")" if (@group);
    push @body, "\n";

    # Add xrefs to classes, sorted by relevancy
    
    my @xrefs = sort { (($b =~ m/$word/i) <=> ($a =~ m/$word/i)) || ($index{$word}{$b} <=> $index{$word}{$a}) || ($a cmp $b)} (keys %{$index{$word}});
    my @xrefs_lim;
    my $count = 0;
    foreach my $xref (@xrefs) {
        last if ++$count > $args{"limit"};
        push @xrefs_lim, $xref . " (" . $index{$word}{$xref} . ")";
    }
    push @body, $ident, "  - ", join(", ", @xrefs_lim);
    push @body, ", [...]"
      if (scalar keys %{$index{$word}} > $args{"limit"});
    push @body, "\n\n";
}

my @summary;
push @summary, "$nb_files file(s) indexed by " . scalar @words . " word(s) on " . localtime();
push @summary, "max limit is " . $args{"limit"} . " xref(s) per word";

# Write documentation

sysopen(DEST_FILE, $destination_file, O_WRONLY|O_TRUNC|O_CREAT|$open_file_as_text)
  or croak "$PROGNAME: unable to open destination file " . $destination_file . "\n";

print DEST_FILE 
  "/*! \@page page_idx Full-text Index\n\n",
  $ident, "  - ", join("\n" . $ident . "  - ", @summary), "\n\n",
  $ident, "\@version $VERSION\n",
  $ident, "\@author \@c $PROGNAME, by $AUTHOR\n",
  $ident, "\@par Navigation:\n",
  $ident, "[", join(" | ", @nav_bar), "]\n", 
  @body, 
  "*/";

close(DEST_FILE);
print " => ", join("\n => ", @summary), "\n => in ", time() - $start_time, " s. \n";
