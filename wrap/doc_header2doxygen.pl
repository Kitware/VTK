#!/usr/bin/env perl
# Time-stamp: <2000-08-03 18:55:37 barre>
#
# Convert VTK headers to doxygen format
#
# roeim : Vetle Roeim <vetler@ifi.uio.no>
# barre : Sebastien Barre <barre@sic.sp2mi.univ-poitiers.fr>
#
# 0.74 (barre) :
#   - as doxygen now handles RCS/CVS tags of the form $word:text$, use them 
#
# 0.73 (barre) :
#   - change doxygen command style from \ to @ to match javadoc, autodoc, etc.
#
# 0.72 (barre) :
#   - change default --to to '../vtk-dox'
#
# 0.71 (barre) :
#   - fix O_TEXT flag problem
#   - switch to Unix CR/LF format
#
# 0.7 (barre) :
#   - change name
#   - remove -c option
#
# 0.6 (barre) :
#   - change (warning) default --to to '../vtk2' because I ruined my own
#     VTK distrib too many times :(
#   - add automatic creation of missing directory trees
#   - add check for current OS (if Windows, do not perform tests based 
#     on stat()/idev/ino features)
#
# 0.5 (barre) :
#   - better .SECTION handling
#   - add support for empty lines in documentation block
#   - fix problem with headers not corresponding to classes
#   - change name to doc_header2doxygen (removed vtk_)
#   - change '-s' (silent) to '-v' (verbose)
#   - add function description reformatting
#
# 0.4 (barre) :
#   - change /*! ... */ position upon request
#   - add 'Date:' support as @date
#   - add 'Version:' support as @version
#   - add 'Thanks:' support as @par Thanks
#
# 0.3 (barre) :
#   - fix various " // Description" spelling problems :)
#
# 0.2 (barre) :
#   - fix problem with classes with no brief documentation
#
# 0.1 (barre) :
#   - add Perl syntactic sugar, options...
#   - add standard output (filter) mode (-c)
#   - add silent mode (-s)
#   - add update mode, convert only if newer (-u)
#   - add conversion to another directory (--to)
#   - add '.SECTION Caveats' support as @warning
#   - add/fix '.SECTION whatever' support as @par
#   - add default directories to process
#
# 0.0 (roeim)
#   - first release
#
# Notes:
#   contrib/vtk32OffscreenRenderWindow.h : needs a documentation block
#   contrib/vtkSuperquadricSource.h : brief description span 2 lines :(


use Carp;
use Getopt::Long;
use Fcntl;
use File::Basename;
use File::Find;
use File::Path;
use Text::Wrap;
use strict;

my ($VERSION, $PROGNAME, $AUTHOR) = (0.74, $0, "V. Roeim, S. Barre");
$PROGNAME =~ s/^.*[\\\/]//;

# Defaults (add options as you want : "v" => 1 for default verbose mode)

my %default = 
  (
   to => "../vtk-dox",
   tmp_file => "doc_header2doxygen.tmp",
   dirs => ["common", "contrib", "graphics", "imaging", "patented"]
  );

# Parse options

my %args;
Getopt::Long::Configure("bundling");
GetOptions (\%args, "v", "u", "f", "temp=s", "to=s", "help|?");

if (exists $args{"help"}) {
    print <<"EOT";
$PROGNAME $VERSION, by $AUTHOR
Usage : $PROGNAME [--help|?] [-v] [-u] [-f] [--temp file] [--to path] [files|directories...]
  --help|?    : this message
  -v          : verbose (display filenames while processing)
  -u          : update, convert only if newer, requires --to
  -f          : force conversion for all files (overrides -u)
  --temp file : use 'file' as temporary file (default: $default{tmp_file})
  --to path   : use 'path' as destination directory (default: $default{to})

Example:
  $PROGNAME --to ../vtk-dox
  $PROGNAME contrib
EOT
    exit;
}

$args{"v"} = 1 if exists $default{"v"};
$args{"u"} = 1 if exists $default{"u"};
$args{"f"} = 1 if exists $default{"f"};
$args{"temp"} = $default{tmp_file} if ! exists $args{"temp"};
$args{"to"} = $default{"to"} if ! exists $args{"to"};
$args{"to"} =~ s/[\\\/]*$// if exists $args{"to"};
croak "$PROGNAME: -u requires --to\n" 
  if exists $args{"u"} && ! exists $args{"to"};

print "$PROGNAME $VERSION, by $AUTHOR\n";

my $os_is_win = ($^O =~ m/(MSWin32|Cygwin)/i);
my $open_file_as_text = $os_is_win ? O_TEXT : 0;
    
# Collect all files and directories

push @ARGV, @{$default{dirs}} if !@ARGV;

my @files;
foreach my $file (@ARGV) {
    if (-f $file) {
        push @files, $file;
    } elsif (-d $file) {
        find sub { push @files, $File::Find::name; }, $file;
    }
}

# Process files corresponding to headers

print "Converting...\n";
my ($start_time, $nb_file) = (time(), 0);

foreach my $source (@files) {
    
    if ($source =~ /vtk[^\\\/]*\.h\Z/) {

        # Update mode : skip if not newer than destination

        if (exists $args{"u"} && ! exists $args{"f"}) {
            my $dest = $args{"to"} . '/' . $source;
            next if -e $dest && (stat $source)[9] < (stat $dest)[9];
        }
        
        ++$nb_file;
	print "  $source\n" if exists $args{"v"};

	sysopen(HEADERFILE, $source, O_RDONLY|$open_file_as_text)
	  or croak "$PROGNAME: unable to open $source\n";
        my @headerfile = <HEADERFILE>;
	close(HEADERFILE);

        my ($date, $revision) = ("", "");
        my @converted = ();
        my @thanks = ();

        # Read until the beginning of the documentation block
        # Extract 'Date', 'Version', 'Thanks' sections

        my ($line, $prev_line);
	while ($line = shift @headerfile) {
            last if $line =~ /\/\/ \.NAME/;
            if ($line =~ /^\s*Date:\s*(.*)$/) {
                $date = $1;
            } elsif ($line =~ /^\s*Version:\s*(.*)$/) {
                $revision = $1;
            } elsif ($line =~ /^\s*Thanks:\s*(.*)$/) {
                push @thanks, "             ", $1, "\n";
                while ($line = shift @headerfile) {
                    last if $line =~ /^\s*$/;
                    $line =~ s/^(\s*)//;
                    push @thanks, "             ", $line;
                }
                push @converted, $line;
            } else {
                push @converted, $line;
            }
        }

        # Process the documentation block

        if (defined($line) && $line =~ /\/\/ \.NAME (\w*)( \- (.*))?/) {

            # Insert class description, date, revision, thanks

            my ($class_name, $class_description) = ($1, $3);
            $class_name =~ s/\.h//;
		
            push @converted, "/*! \@class   $class_name\n";
            push @converted, "    \@brief   $class_description\n"
              if $class_description;
            # WARNING : need a blank line between RCS tags and previous dox tag
            if ($date) {
                push @converted, "\n    $date\n";
            } else {
                carp "$PROGNAME: could not parse 'Date:' in $source\n";
            }
            if ($revision) {
                push @converted, "\n" if (!$date);
                push @converted, "    $revision\n";
            } else {
                carp "$PROGNAME: could not parse 'Version:' in $source\n";
            }
            push @converted, "    \@par     Thanks:\n", @thanks if @thanks;

            my $tag = "";

            # Read until the end of the documentation block
            # Translate 'See Also', 'Caveats' and whatever section

            my $empty_prev = 0;
            while ($line = shift @headerfile) {
                last if $line =~ /^\#/;
		    
                if ($line =~ /^\/\/ \.SECTION Description/i) {
                    ($tag, $empty_prev) = ("", 1);
                    push @converted, "\n";
                }
                elsif ($line =~ /^\/\/ \.SECTION See Also/i) {
                    ($tag, $empty_prev) = ("", 0);
                    push @converted, "    \@sa      ";
                }
                elsif ($line =~ /^\/\/ \.SECTION Caveats/i) {
                    ($tag, $empty_prev) = ("\@warning", 1);
                }
                elsif ($line =~ /^\/\/ \.SECTION (\w*)/) {
                    ($tag, $empty_prev) =  ("\@par " . $1 . ":", 1);
                }

                # Starts with '//', we are still within the block,
                # remove '//' for non empty lines, 
                # eventually put/duplicate .SECTION tag
                
                elsif ($line =~ /^\/\/(.*)/) {
                    my $remaining = $1;
                    if ($remaining =~ /\S/) {
                        $line =~ s/\/\///;
                        push @converted, "    $tag\n" 
                          if $tag ne "" && $empty_prev;
                        push @converted, $line;
                        $empty_prev = 0;
                    } else {
                        push @converted, "\n";
                        $empty_prev = 1;
                    }    
                } else {
                    # Does not starts with //, but still within block or just
                    # before the end (#). Probably an empty line.
                    # Hack : let's have a look at the next line, if it begins
                    # with //, then the current line is included (was a space).
                    
                    if (my $next_line = shift @headerfile) {
                        push @converted, $line if $next_line =~ /^\/\//;
                        unshift @headerfile, $next_line;
                    }
                }
            }

            # Dump the end of the block

            push @converted, "*/\n\n", $line;
        }

        # Read until the end of the header, translate description of functions
        # (locate a normal C++ comment containing 'Description:')

	while ($line = shift @headerfile) {
        
	    if ($line =~ /^(\s*)\/\/\s*De(s|c)(s|c)?ription/) {
		
		my $spaces = $1;
                $Text::Wrap::columns = 76;

		# While there are still lines beginning with '//',
                # save description, trim spaces and reformat paragraph

                my @desc = ();
		while ($line = shift @headerfile) {
                    last if $line !~ /^\s*\/\//;
                    chop $line;
		    $line =~ s/^\s*\/\/\s*//;
		    $line =~ s/\s*$//;
		    push @desc, $line;
		}
		push @converted, wrap("$spaces/*! ", "$spaces    ", @desc), " */\n" if @desc;
	    } 
            
            push @converted, $line;
	}
	
        # Write the converted header to its destination

        my $dest;
        if (! exists $args{"to"}) {
            $dest = $args{"temp"};
        } else {
            $dest = $args{"to"} . '/' . $source;
            if (!$os_is_win) {
                my ($i_dev, $i_ino) = stat $source;
                my ($o_dev, $o_ino) = stat $dest;
                croak "$PROGNAME: sorry, $source and $dest are the same file\n"
                  if ($i_dev == $o_dev && $i_ino == $o_ino);
            }
        }
        
        if (!sysopen(DEST_FILE, $dest, O_WRONLY|O_TRUNC|O_CREAT|$open_file_as_text)) {
            my $dir = dirname($dest);
            mkpath($dir);
            sysopen(DEST_FILE, $dest, O_WRONLY|O_TRUNC|O_CREAT|$open_file_as_text)
              or croak "$PROGNAME: unable to open destination file $dest\n";
        }
        print DEST_FILE @converted;
        close(DEST_FILE);
        
        if (! exists $args{"to"}) {
            unlink($source)
              or carp "$PROGNAME: unable to delete original file $source\n";
            rename($args{"temp"}, $source)
              or carp "$PROGNAME: unable to rename ", $args{"temp"}, " to $source\n";
        }
    }
}

print "$nb_file files converted in ", time() - $start_time, " s. \n"
  if ! exists $args{"c"};
