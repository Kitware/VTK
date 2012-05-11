Documenting VTK with doxygen
----------------------------

Sebastien BARRE (Time-stamp: <2001-10-24 22:38:42 barre>)

This file describes how to generate a Doxygen-compliant documentation
featuring cross-references between classes and examples, a full-text index
and an accurate VTK version.

Note that this file describes how to run the scripts manually, which
can be tedious (to say the least). A batch file doc_makeall.sh is now
used to automate this process. It is part of the CMake/Build process
and is configured both though CMake and through its own parameter. It
should be pretty straightforward to understand and adapt.

Warning
-------

These scripts are now part of the VTK source distribution.

Think twice before letting Doxygen eat your CPU cycles: Kitware is
now using these scripts to build an up-to-date Doxygen-compliant
documentation every night as part of the "nightly build"
process. Thus, you might browse or download a pre-built documentation here:

         http://public.kitware.com/doc/

Nevertheless, Doxygen might still be useful to generate additional
PDF, Postscript or plain LaTeX output.

This package is made of he following Perl scripts:

- doc_header2doxygen.pl: convert the VTK headers to the Doxygen format
- doc_version.pl: extract the VTK version and add it to the documentation set
- doc_class2example.pl: build cross-references between classes and examples
- doc_index.pl: build the full-text index

Use the --help option to display the parameters that are allowed for each
script.

Thanks to Vetle Roeim and Jan Stifter who started the project :)


A. Preamble
   --------

I assume that your VTK distribution is in a 'VTK' directory and that
you do NOT want to modify it in any ways. You would rather create a
modified copy of the VTK files in a different location (here, the
'../VTK-doxygen' directory, assuming that you are in the 'VTK'
directory now). Hence, you might use this main 'VTK' directory for CVS
and development purposes (updating it from time to time) and rerun
the process described below to update your documentation (a
straightforward process once you get used to it).

The Perl scripts described in that document are stored in the
'VTK/Utilities/Doxygen' directory. You will also find a couple of
additional files like a VTK logo and a batch script automating the
whole conversion process. Thus, the directories that are of some
interest in that README are:

   VTK/ : the VTK repository
   VTK/Utilities/Doxygen : the Doxygen scripts and configuration files
   VTK-Doxygen/ : the intermediate VTK repository

The doxygen configuration script, i.e. the script controlling how this
VTK documentation is created, is named 'doxyfile'; it is also stored
in the 'VTK/Utilities/Doxygen' directory. It is read by doxygen and
lists option/value pairs like:

    PARAMETER = VALUE

Comment lines are allowed and start with '#'. Commenting a directive
is a simple way to hide it to doxygen:

#   PARAMETER = VALUE

Do not forget to set the OUTPUT_DIRECTORY directive specifying the
location where the whole documentation is to be stored. Example:

    OUTPUT_DIRECTORY     = ../../../doc

The crucial INPUT directive controls which directory or files are
processed by doxygen to produce the documentation. Each directory/file
is separated by a space. Whenever you will be requested to add a
filename to INPUT, just add it to the end of the list. Example:

    INPUT = ../../../VTK-doxygen/Common ../../../VTK-doxygen/Filtering ../../../VTK-doxygen/Graphics ../../../VTK-doxygen/Hybrid ../../../VTK-doxygen/Imaging ../../../VTK-doxygen/IO ../../../VTK-doxygen/Parallel ../../../VTK-doxygen/Patented ../../../VTK-doxygen/Rendering ../../../VTK-doxygen/doc_version.dox ../../../VTK-doxygen/doc_class2examples.dox ../../../VTK-doxygen/doc_class2tests.dox ../../../VTK-doxygen/doc_index.dox

WARNING: disable the HAVE_DOT directive if you have not installed graphviz
(http://www.research.att.com/~north/graphviz/download.html).


B.  Download doxygen, make sure it runs
    -----------------------------------

    http://www.doxygen.org


C0. Go to the VTK/Utilities/Doxygen directory
    -----------------------------------------

    You are supposed to run all scripts as well as Doxygen from this
    directory.

For example:

    D:\src\kitware\vtk\VTK> cd Utilities\Doxygen
    D:\src\kitware\vtk\VTK\Utilities\Doxygen>


C1. Convert the VTK headers to Doxygen format
    -----------------------------------------

    Use the conversion script: doc_header2doxygen.pl

For example:

    D:\src\kitware\vtk\VTK\Utilities\Doxygen> perl doc_header2doxygen.pl --to ../../../VTK-doxygen
    doc_header2doxygen.pl 0.8, by Sebastien Barre et al.
    Collecting...
    Converting...
     => 729 files converted in 6 s.
    Finished in 7 s.

Meaning that:

    The optional '--to ../../../VTK-doxygen' parameter specifies that the
    script stores the modified C++ *headers* (*.h files)  in the
    '../../../VTK-doxygen' directory (here: 'D:\src\kitware\vtk\VTK-doxygen',
    as I issued the command from 'D:\src\kitware\vtk\VTK\Utilities\Doxygen').
    The files located in 'VTK' remain untouched. The script browses
    the directories located two levels up (i.e. ../../) automatically.

    Use --help to display the default value associated to --to.

    Note that you might restrict the conversion to a set of particuliar
    directories or files if you want to document a specific VTK part only:
    just add these parts as parameters. Example:

    D:\src\kitware\vtk\VTK\Utilities\Doxygen> perl doc_header2doxygen.pl --to ../../../VTK-doxygen ../../Hybrid

Update the Doxygen configuration file ('doxyfile'):

    Depending on the location from where you are going to run Doxygen
    (here: 'D:\src\kitware\vtk\VTK\Utilities\Doxygen') update or add
    the paths to the VTK components/directories that you want to
    document. Since the headers have been created in
    '../../../VTK-doxygen', the following values will be fine:

    INPUT = ../../../VTK-doxygen/Common ../../../VTK-doxygen/Filtering ../../../VTK-doxygen/Graphics ../../../VTK-doxygen/Hybrid ../../../VTK-doxygen/Imaging ../../../VTK-doxygen/IO ../../../VTK-doxygen/Parallel ../../../VTK-doxygen/Patented ../../../VTK-doxygen/Rendering


C2. Extract VTK version
    -------------------

    Use the extraction script: doc_version.pl

For example:

    D:\src\kitware\vtk\VTK\Utilities\Doxygen> perl doc_version.pl --logo "vtk-logo.gif" --to ../../../VTK-doxygen
    doc_version.pl 0.22, by Sebastien Barre
     => 4.0.0
     => $Revision$ $Date$
    Building version documentation to ../../../VTK-doxygen/doc_version.dox
    Finished in 0 s.

Meaning that:

    The optional '--to ../../../VTK-doxygen' parameter specifies that
    the script stores the related documentation part in the
    '../../../VTK-doxygen' directory (more precisely in the
    '../../../VTK-doxygen/doc_version.dox' file). The VTK logo is
    incorporated by using the --logo option. The script searchs for
    the VTK version in the '../../Common/vtkVersion.h'. Use --help to
    check how to override these default values.

Update the Doxygen configuration file ('doxyfile'):

    Comment the PROJECT_NUMBER directive: you do not need it anymore
    as you have just created a more accurate description based on the
    current vtkVersion.h.

    # PROJECT_NUMBER       =


C3. Generate the 'class to examples' pages
    --------------------------------------

    Use the referencing script: doc_class2example.pl

For example:

    D:\src\kitware\vtk\VTK\Utilities\Doxygen> perl doc_class2example.pl --link http://public.kitware.com/cgi-bin/cvsweb.cgi/~checkout~/VTK/Utilities/Doxygen --to ../../../VTK-doxygen
    doc_class2example.pl 0.7, by Sebastien Barre
    Collecting files...
     => 17 file(s) collected in 2 s.
    Parsing files...
     => 17 file(s) parsed in 0 s.
    Eliminating some classes...
     => 1 class(es) eliminated (vtkCommand) in 0 s.
    Locating headers to update...
     => 33 found, 8 orphan class(es) removed (vtkIOJava, vtkRenderingJava, vtkCommonJava, vtkImagingJava, vtkGraphicsJava, vtkInteract, vtkTkWidget, vtkFilteringJava) in 0 s.
    Building classes doc and alphabetical section(s) weight(s)...
     => 33 classes(s) documented in 0 s.
     => total weight is 38604 in 13 section(s) (mean is 2969)
    Computing alphabetical group(s)/page(s)...
     => max weight is 90000 per group/page, but a section can not be divided
     => 1 group(s) for 13 section(s)
    Building pages header...
    Writing documentation to ../../../VTK-doxygen/doc_class2examples.dox...
      - 33 class(es) in 17 file(s) from directories matching @c ^Examples$ on Mon Oct  1 10:53:04 2001
      - 4 parser(s) : [Java, Python, C++, Tcl]
      - at most 20 file(s) per parser (0% over)
     => in 0 s.
    Updating headers...
     => 32 header(s) updated in 0 s.
Finished in 2 s.

Meaning that :

    The optional '--to ../../../VTK-doxygen' parameter specifies that
    the script searches (and parses) the Python, Tcl, Java or C++
    *examples* (17 here) in the directories two level up (i.e. ../../
    and below) but updates the C++ class *headers* (32 here) stored in
    the '../../../VTK-doxygen' directory (which is good because they
    have just been created in step C1). These headers are updated to
    include cross-links between the class and the examples using
    it. The script stores the documentation file listing all examples
    in the '../../../VTK-doxygen/doc_class2example.dox' file. Once
    again, the files located in 'VTK' remain untouched. The --link
    option is a path or a URL that is prepended to each example's name
    to provide a physical link to that example (here, the CVSweb).

    Use --help to check how to override these default values. More
    specifically you can override the --dirmatch option (a regexp) so
    that the parsers will only look into specific directories. The
    default value "^Examples\$" constrains the script to any
    directory which name is 'Example'; --dirmatch "^Testing$" will
    actually make the script parse all examples found in the 'Testing'
    directories (the --label, --title and --unique option should be changed
    appropriately, check 'doc_makeall.bat' for example).

Update the Doxygen configuration file ('doxyfile'):

    Add the relative path to 'doc_class2example.dox' (here :
    '../../../VTK-doxygen/doc_class2example.dox') to the INPUT
    directive, so that Doxygen might process it (see also section D).


C4. Build full-text index
    ---------------------

    Use the indexing script: doc_index.pl

For example:

    D:\src\kitware\vtk\VTK\Utilities\Doxygen> perl doc_index.pl --to ../../../VTK-doxygen
    doc_index.pl 0.2, by Sebastien Barre
    Reading stop-words from doc_index.stop...
     => 724 stop-word(s) read.
    Collecting files...
    Indexing...
     => 8174 word(s) grabbed in 729 file(s) in 2 s.
    Removing...
     => 3126 word(s) removed.
    Grouping...
     => 2075 word(s) grouped.
    Normalizing...
     => normalized to lowercase.
    Building indexes doc and alphabetical section(s) weight(s)...
     => 2973 words(s) documented in 2 s.
     => total weight is 400800 in 47 section(s) (mean is 8527)
    Computing alphabetical group(s)/page(s)...
     => max weight is 90000 per group/page, but a section can not be divided
     => 6 group(s) for 47 section(s)
    Building pages header...
    Writing documentation to ../../../VTK-doxygen/doc_index.dox...
      - 729 file(s) indexed by 2973 word(s) on Mon Oct  1 11:13:44 2001
      - max limit is 10 xref(s) per word
    Finished in 5 s.

Meaning that:

    The optional '--to ../../../VTK-doxygen' parameter specifies that
    the script stores the full-text index in the
    '../../../VTK-doxygen' directory (more precisely in the
    '../../../VTK-doxygen/doc_index.dox' file).

    The optional --stop parameter can be used to specify that the
    script should read its stop-words from a different file (default
    is 'doc_index.stop').  This file is a list of trivial words (one
    per line) that won't be indexed.

Update the Doxygen configuration file ('doxyfile'):

    Add the relative path to 'doc_index.dox' (here :
    '../../../VTK-doxygen/doc_index.dox') to the INPUT directive, so that
    Doxygen might process it (see also section D)


D.  Run doxygen
    -----------

    Check that your Doxygen configuration file is OK
    ('doxyfile'). The important directives are:

    PROJECT_NAME         = VTK
    # PROJECT_NUMBER     =
    OUTPUT_DIRECTORY     = ../../../doc
    INPUT = ../../../VTK-doxygen/Common ../../../VTK-doxygen/Filtering ../../../VTK-doxygen/Graphics ../../../VTK-doxygen/Hybrid ../../../VTK-doxygen/Imaging ../../../VTK-doxygen/IO ../../../VTK-doxygen/Parallel ../../../VTK-doxygen/Patented ../../../VTK-doxygen/Rendering ../../../VTK-doxygen/doc_version.dox ../../../VTK-doxygen/doc_class2examples.dox ../../../VTK-doxygen/doc_class2tests.dox ../../../VTK-doxygen/doc_index.dox

    Then run doxygen. Be *patient* :)

For example:

    D:\src\kitware\vtk\VTK\Utilities\Doxygen> doxygen

    The documentation will be located in the '../../../doc'
    directory. If that directory does not exist, doxygen will create it,
    *unless* the its parent directory does not exist either.

    You are done.
    Browse the HTML documentation starting from:
    doc/html/index.html


Just my 2 cents.

Sebastien BARRE
