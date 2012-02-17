#!/usr/bin/env perl
# Time-stamp: <2011-01-16 21:15:53 barre>
#
# Convert VTK headers to doxygen format
#
# roeim : Vetle Roeim <vetler@ifi.uio.no>
# barre : Sebastien Barre <sebastien@barre.nom.fr>
#
# 0.9 (barre) :
#   - add --conds          : add \cond...\endcond around public:, private:, protected:
#
# 0.83 (barre) :
#   - add --stdout          : print converted file to standard output
#
# 0.82 (barre) :
#   - add --relativeto path : each file/directory to document is considered
#     relative to 'path', where --to and --relativeto should be absolute
#
# 0.81 (barre) :
#   - fix pb if both --to and path to the file to document were absolute
#   - remove warning when date or revision not found
#
# 0.8 (barre) :
#   - update to match the new VTK 4.0 tree
#   - change default --dirs so that it can be launched from Utilities/Doxygen
#   - change default --to so that it can be launched from Utilities/Doxygen
#   - handle more .SECTION syntax
#   - add group support (at last)
#
# 0.76 (barre) :
#   - add 'parallel' to the default set of directories
#
# 0.75 (barre) :
#   - change default --to to '../vtk-doxygen' to comply with Kitware's doxyfile
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
#   - first release (thanks to V. Roeim !)


use Carp;
use Cwd 'abs_path';
use Getopt::Long;
use Fcntl;
use File::Basename;
use File::Find;
use File::Path;
use Text::Wrap;
use strict;

my ($VERSION, $PROGNAME, $AUTHOR) = (0.9, $0, "Sebastien Barre et al.");
$PROGNAME =~ s/^.*[\\\/]//;

# -------------------------------------------------------------------------
# Defaults  (add options as you want: "verbose" => 1 for default verbose mode)

my %default =
  (
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
            "../../TextAnalysis",
            "../../Views",
            "../../VolumeRendering",
            "../../Widgets"],
   relativeto => "",
   temp => "doc_header2doxygen.tmp",
   to => "../../../VTK-doxygen"
  );

# -------------------------------------------------------------------------
# Parse options

my %args;
Getopt::Long::Configure("bundling");
GetOptions (\%args, "help", "verbose|v", "update|u", "conds|c", "force|f", "temp=s", "to=s", "stdout", "relativeto=s");

print "$PROGNAME $VERSION, by $AUTHOR\n" if ! exists $args{"stdout"};

if (exists $args{"help"}) {
    print <<"EOT";
Usage : $PROGNAME [--help] [--verbose|-v] [--update|-u] [--conds|-c] [--force|-f] [--temp file] [--to path] [--relativeto path] [files|directories...]
  --help            : this message
  --verbose|-v      : verbose (display filenames while processing)
  --update|-u       : update (convert only if newer, requires --to)
  --force|-f        : force conversion for all files (overrides --update)
  --stdout          : print converted file to standard output
  --temp file       : use 'file' as temporary file (default: $default{temp})
  --to path         : use 'path' as destination directory (default: $default{to})
  --relativeto path : each file/directory to document is considered relative to 'path', where --to and --relativeto should be absolute (default: $default{relativeto})
  --conds|-c        : use \cond sections around public, protected, private

Example:
  $PROGNAME --to ../vtk-doxygen
  $PROGNAME contrib
EOT
    exit;
}

$args{"verbose"} = 1 if exists $default{"verbose"};
$args{"update"} = 1 if exists $default{"update"};
$args{"conds"} = 1 if exists $default{"conds"};
$args{"force"} = 1 if exists $default{"force"};
$args{"temp"} = $default{temp} if ! exists $args{"temp"};
$args{"to"} = $default{"to"} if ! exists $args{"to"};
$args{"to"} =~ s/[\\\/]*$// if exists $args{"to"};
$args{"relativeto"} = $default{"relativeto"} if ! exists $args{"relativeto"};
$args{"relativeto"} =~ s/[\\\/]*$// if exists $args{"relativeto"};

croak "$PROGNAME: --update requires --to\n" 
  if exists $args{"update"} && ! exists $args{"to"};

my $os_is_win = ($^O =~ m/(MSWin32|Cygwin)/i);
my $open_file_as_text = $os_is_win ? O_TEXT : 0;
my $start_time = time();
    
# -------------------------------------------------------------------------
# Collect all files and directories

push @ARGV, @{$default{dirs}} if !@ARGV;

print "Collecting...\n" if ! exists $args{"stdout"};
my @files;
foreach my $file (@ARGV) {
    if (-f $file) {
        push @files, $file;
    } elsif (-d $file) {
        find sub { push @files, $File::Find::name; }, $file;
    }
}

# -------------------------------------------------------------------------
# Process files corresponding to headers

print "Converting...\n" if ! exists $args{"stdout"};
my $intermediate_time = time();
my $nb_file = 0;

foreach my $source (@files) {
    
    next if $source !~ /vtk[^\\\/]*\.h\Z/;

    # Figure out destination file now

    my $dest;
    if (! exists $args{"to"}) {
        $dest = $args{"temp"};
    } else {
        # if source has absolute path, just use the basename, unless a
        # relativeto path has been set
        if ($source =~ m/^(\/|[a-zA-W]\:[\/\\])/) {
            if ($args{"relativeto"}) {
                my ($dir, $absrel) = (abs_path(dirname($source)), 
                                      abs_path($args{"relativeto"}));
                $dir =~ s/$absrel//;
                $dest = $args{"to"} . $dir . '/' . basename($source);
            } else {
                $dest = $args{"to"} . '/' . basename($source);
            }
        } else {
            my $source2 = $source;
            # let's remove the ../ component before the source filename, so 
            # that it might be appended to the "to" directory
            $source2 =~ s/^(\.\.[\/\\])*//;
            $dest = $args{"to"} . '/' . $source2;
        }
        # Ensure both source and target are different
        if (!$os_is_win) {
            my ($i_dev, $i_ino) = stat $source;
            my ($o_dev, $o_ino) = stat $dest;
            croak "$PROGNAME: sorry, $source and $dest are the same file\n"
              if ($i_dev == $o_dev && $i_ino == $o_ino);
        }
    }

    # Update mode : skip the file if it is not newer than the 
    # previously converted target
    
    if (exists $args{"update"} && ! exists $args{"force"}) {
        next if -e $dest && (stat $source)[9] < (stat $dest)[9];
    }
    
    ++$nb_file;
    print "  $source\n" if exists $args{"verbose"};

    # Open file, feed it entirely to an array

    sysopen(HEADERFILE, $source, O_RDONLY|$open_file_as_text)
      or croak "$PROGNAME: unable to open $source\n";
    my @headerfile = <HEADERFILE>;
    close(HEADERFILE);
    
    my ($date, $revision) = ("", "");
    my @converted = ();
    my @thanks = ();
    
    # Parse the file until the beginning of the documentation block
    # is found. The copyright and disclaimer sections are parsed to 
    # extract the 'Date', 'Version' and 'Thanks' values.
    
    my $line;
    while ($line = shift @headerfile) {

        # Quit if the beginning of the documentation block has been reached. 
        # It is supposed to start with:
        # // .NAME vtkFooBar - foo bar class

        last if $line =~ /\/\/ \.NAME/;

        # Date. Example:
        # Date:      $Date$

        if ($line =~ /^\s*Date:\s*(.*)$/) {
            $date = $1;

        # Version. Example:
        # Version:   $Revision$

        } elsif ($line =~ /^\s*Version:\s*(.*)$/) {
            $revision = $1;

        # Thanks (maybe multi-lines). Example:
        # Thanks:    Thanks to Sebastien Barre who developed this class.
         
        } elsif ($line =~ /^\s*Thanks:\s*(.*)$/) {
            push @thanks, "             ", $1, "\n";
            # Handle multi-line thanks
            while ($line = shift @headerfile) {
                last if $line =~ /^\s*$/;
                $line =~ s/^(\s*)//;
                push @thanks, "             ", $line;
            }
            push @converted, $line;

        # Everything else goes to the converted file

        } else {
            push @converted, $line;
        }
    }
    
    # Process the documentation block
    # Extract the name of the class and its short description
    # // .NAME vtkFooBar - foo bar class
    
    if (defined($line) && $line =~ /\/\/ \.NAME (\w*)( \- (.*))?/) {
        
        my ($class_name, $class_short_description) = ($1, $3);
        $class_name =~ s/\.h//;
        
        # Insert class description, date, revision, thanks
        
        push @converted, "/*! \@class   $class_name\n";
        push @converted, "    \@brief   $class_short_description\n"
          if $class_short_description;

        if ($date) {
            push @converted, "\n    $date\n";
        }

        # WARNING : need a blank line between RCS tags and previous dox tag

        if ($revision) {
            push @converted, "\n" if (!$date);
            push @converted, "    $revision\n";
        }

        # Do not add thanks anymore. Will be done externally.
        # push @converted, "    \@par     Thanks:\n", @thanks if @thanks;
        
        # Read until the end of the documentation block is reached
        # Translate 'See Also', 'Caveats' and whatever .SECTION
        # As of 24 sep 2001, there are:
        #      137	// .SECTION Caveats
        #        1	// .SECTION Credits
        #      702	// .SECTION Description
        #        3	// .SECTION Note
        #        1	// .SECTION note
        #      329	// .SECTION See Also
        #        4	// .SECTION See also
        #       70	// .SECTION see also
        #        1	// .SECTION Warning
        # find . -name vtk\*.h -exec grep "\.SECTION" {} \; | sort | uniq -c
        # Let's provide support for bugs too:
        #         	// .SECTION Bug
        #         	// .SECTION Bugs
        #         	// .SECTION Todo

        my ($tag, $inblock) = ("", 0);
        while ($line = shift @headerfile) {
            
            # Quit if the end of the documentation block has been reached. 
            # Let'say that it is supposed to end as soon as the usual
            # inclusion directives are found, for example:
            # #ifndef __vtkAbstractTransform_h
            # #define __vtkAbstractTransform_h
            
            last if $line =~ /^\#/;
            
            # Process and recognize a .SECTION command and convert it to
            # the corresponding doxygen tag ($tag)

            if ($line =~ /^\/\/\s+\.SECTION\s+(.+)\s*$/i) {
                
                my $type = $1;

                # Bugs (@bugs). Starts with:
                # // .SECTION Bug
                # // .SECTION Bugs
            
                if ($type =~ /Bugs?/i) {
                    $tag = "\@bug";
                }

                # Caveats or Warnings (@warning). Starts with:
                # // .SECTION Caveats
                # // .SECTION Warning
                # // .SECTION Warnings
            
                elsif ($type =~ /(Caveats|Warnings?)/i) {
                    $tag = "\@warning";
                }

                # Description. Starts with:
                # // .SECTION Description
            
                elsif ($type =~ /Description/i) {
                    $tag = "";
                    push @converted, "\n";
                }

                # Note (@attention). Starts with:
                # // .SECTION Note

                elsif ($type =~ /Note/i) {
                    $tag = "\@attention";
                }

                # See also (@sa). Starts with:
                # // .SECTION See Also
                
                elsif ($type =~ /See Also/i) {
                    $tag = "\@sa";
                }

                # Todo (@todo). Starts with:
                # // .SECTION Todo

                elsif ($type =~ /Todo/i) {
                    $tag = "\@todo";
                }

                # Any other .SECTION (@par). Starts with:
                # // .SECTION whatever
                
                else {
                    $tag =  "\@par " . $type . ":";
                }

                $inblock = 0;
            }
            
            # If the line starts with '//', we are still within the tag block.
            # Remove '//' for non empty lines, eventually put or duplicate 
            # the tag name if an empty comment is found (meaning that a new 
            # 'paragraph' is requested but with the same tag type)
            # Example:
            #  // .SECTION Caveats
            #  // blabla1q
            #  // blabla1b
            #  //
            #  // blabla2
            # Gets translated into:
            #      @warning
            #   blabla1q
            #   blabla1b
            #
            #      @warning
            #   blabla2
            
            elsif ($line =~ /^\/\/(.*)/) {
                my $remaining = $1;
                if ($remaining =~ /\S/) {
                    push @converted, "    $tag\n" 
                      if $tag ne "" && ! $inblock;
                    push @converted, $remaining, "\n";
                    $inblock = 1;
                } else {
                    push @converted, "\n";
                    $inblock = 0;
                }    
            } else {
                # Does not starts with // but still within block or just
                # before the end (#). Probably an empty line.
                # Hack : let's have a look at the next line... if it begins
                # with // then the current line is included (was a space).
                
                if (my $next_line = shift @headerfile) {
                    push @converted, $line if $next_line =~ /^\/\//;
                    unshift @headerfile, $next_line;
                }
            }
        }
        
        # Close the doxygen documentation block describing the class
        
        push @converted, "*/\n\n", $line;
    }
    
    # Read until the end of the header and translate the description of
    # each function provided that it is located in a C++ comment
    # containing the 'Description:' keyword.
    # Example:
    #  // Description:
    #  // Construct with automatic computation of divisions, averaging
    #  // 25 points per bucket.
    #  static vtkPointLocator2D *New();

    my $in_section = "";

    while ($line = shift @headerfile) {
        
        # Track the public:, protected: and private: sections and put them
        # between \cond... \endcond so that they can be removed from the
        # documentation conditionally. Add them to ENABLED_SECTION
        # to show them.
        # IMPORTANT: *no* spaces are allowed between the beginning of the
        # line and the qualifier. This is mandatory to solve issues
        # with nested class definitions, since it is non-trivial to
        # track the fact that we are leaving a class definition to
        # re-enter the parent class definition, etc.

        if (exists $args{"conds"}) {
            if ($line =~ /^(public|protected|private):/) {
                if ($in_section ne "") {
                    push @converted, "// \@endcond\n";
                }
                $in_section = $1;
                push @converted, "// \@cond section_$in_section\n";
            }
        }

        if ($line =~ /^(\s*)\/\/\s*De(s|c)(s|c)?ription/) {
            
            my $indent = $1;
            $Text::Wrap::columns = 76;
            
            # While there are still lines beginning with '//' append them to
            # the function's description and trim spaces.
            
            my @description = ();
            while ($line = shift @headerfile) {
                last if $line !~ /^\s*\/\//;
                chop $line;
                $line =~ s/^\s*\/\/\s*//;
                $line =~ s/\s*$//;
                push @description, $line;
            }

            # While there are non-empty lines or a new Descrption line add
            # these lines to the list of declarations
            # (and/or inline definitions) pertaining to the same description.

            my @declarations = ();
            while ($line && $line =~ /\s*\S/) {
                push @declarations, $line;
                # terminate if we encounter another Description line
                # that doesn't have a leading empty line
                if ($headerfile[0] !~ /^(\s*)\/\/\s*De(s|c)(s|c)?ription/ ) {
                    $line = shift @headerfile;
                }else{
                    $line = "";
                }
            }

            # If there is more than one declaration or at least a macro,
            # enclose in a group (actually a single multiline declaration will
            # be enclosed too, but who cares :)...

            my $enclose =
              (scalar @declarations > 1 || $declarations[0] =~ /vtk.+Macro/);
            
            push @converted, "$indent//@\{\n" if $enclose;
            push @converted, 
            wrap("$indent/*! ", "$indent    ", @description), " */\n"
              if @description;
            push @converted, @declarations;
            push @converted, "$indent//@\}\n" if $enclose;
        }
        
        push @converted, $line;
    }
    
    if (exists $args{"conds"}) {
        if ($in_section ne "") {
            push @converted, "// \@endcond";
        }
    }

    # Write the converted header to its destination
    # or to standard output.

    if (exists $args{"stdout"}) {

        print  @converted;

    } else {

        # Open the target and create the missing directory if any

        if (!sysopen(DEST_FILE, 
                     $dest, 
                     O_WRONLY|O_TRUNC|O_CREAT|$open_file_as_text)) {
            my $dir = dirname($dest);
            mkpath($dir);
            sysopen(DEST_FILE, 
                    $dest, 
                    O_WRONLY|O_TRUNC|O_CREAT|$open_file_as_text)
              or croak "$PROGNAME: unable to open destination file $dest\n";
        }
        print DEST_FILE @converted;
        close(DEST_FILE);
        
        # If in-place conversion was requested, remove source and rename target
        # (or temp file) to source

        if (! exists $args{"to"}) {
            unlink($source)
              or carp "$PROGNAME: unable to delete original file $source\n";
            rename($args{"temp"}, $source)
              or carp "$PROGNAME: unable to rename ", $args{"temp"}, " to $source\n";
        }
    }
}

if (! exists $args{"stdout"}) {
    print " => $nb_file files converted in ", time() - $intermediate_time, " s. \n";
    print "Finished in ", time() - $start_time, " s.\n";
}
