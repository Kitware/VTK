Documenting VTK with doxygen
----------------------------

Sebastien BARRE (Time-stamp: <2001-06-28 03:03:03 barre>)

This file describes how to generate a doxygen-compliant documentation
featuring cross-references between classes and examples, index,
and accurate VTK version. 

Warning
-------

These scripts are now part of the VTK CVS and source distribution.

Think twice before letting doxygen eat your CPU cycles : Kitware is
now using these scripts to build an up-to-date doxygen-compliant
documentation every night, as part of the "nightly build"
process. Thus, you might browse a pre-built documentation here :

         http://www.visualizationtoolkit.org/vtk/quality/Doc/html/

...and download the whole documentation (vtkMan.tar.gz) from the usual
nightly mirrors : 

        http://www.kitware.com/vtkhtml/vtkdata/Nightly.html
        ftp://public.kitware.com/pub/vtk/nightly/vtkMan.tar.gz

Nevertheless, doxygen might still be useful to generate additional
PDF, Postscript, plain LaTeX or Windows Compressed HTML output.

The package is made of :

- doc_header2doxygen.pl : convert VTK headers to doxygen format
- doc_version.pl : extract VTK version and add it to documentation
- doc_class2example.pl : build cross-references between classes and examples
- doc_index.pl : build full-text index 

Use the --help option to display the allowed parameters for each
script.

Thanks to Vetle Roeim and Jan Stifter who started the project :)


A. Preamble
   --------

I assume that your VTK distribution is in a 'vtk' directory and that
you do NOT want to modify it in any ways. You'd but rather create a
modified copy of the VTK files in a different location (here,
the '../vtk-doxygen' directory). Hence, you might use this main 'vtk'
directory for CVS purposes (updating it from time to time), and rerun
the process described below to update your documentation (which is a
straightforward process once you get it).

I assume that the Perl scripts described below are stored in the 'vtk/wrap'
directory, which is where they usually are in the source distribution.

I also assume that your doxygen configuration script, i.e. the script
controlling how this VTK documentation will be created, is named
'doxyfile'. I assume that it is also stored in the 'vtk/wrap' directory.

It is read by doxygen and contains directives like :

    PARAMETER = VALUE
 
Comment lines are allowed and start with '#'. Commenting a directive
is a simple way to hide it to doxygen :

#   PARAMETER = VALUE

Do not forget to set the OUTPUT_DIRECTORY directive, which holds the
location where the whole documentation will be stored. Example :

    OUTPUT_DIRECTORY = ../html/Doc

The crucial INPUT directive controls which directory or files will be
processed by doxygen to produce the documentation. Each directory/file
is separated by a space. Whenever you will be requested to add a file
name to INPUT, just add it to the end of the list. Example :

     INPUT = ../vtk-doxygen/common ../vtk-doxygen/contrib ../vtk-doxygen/graphics ../vtk-doxygen/imaging ../vtk-doxygen/patented ../vtk-doxygen/doc_class2example.dox

    WARNING : disable the HAVE_DOT directive if you have not installed
    graphviz (http://www.research.att.com/~north/graphviz/download.html).


B. Download doxygen, make sure it runs 
   -----------------------------------

   http://www.stack.nl/~dimitri/doxygen


C1. Convert the VTK headers to doxygen format
    -----------------------------------------

     Use the conversion script : doc_header2doxygen.pl 

For example : 

     E:\src\vtk\vtk> perl wrap\doc_header2doxygen.pl --to ..\vtk-doxygen

     doc_header2doxygen.pl 0.75, by Sebastien Barre et al.
     Converting...
     620 files converted in 8 s.

Meaning that :

     The optional '--to ..\vtk-doxygen' parameter specifies that the
     script searches for C++ *headers* (*.h files) in the current
     directory (and below) but stores the modified headers in the
     '..\vtk-doxygen' directory (here: E:\src\vtk\vtk-doxygen, as I
     issued the command from E:\src\vtk\vtk). The files located in
     'vtk' remain untouched.

     Use --help to display the default value associated to --to.
     
     Note that you might restrict the conversion to a set of particuliar
     directories/files if you want to document a specific VTK part only :
     just add these parts as parameters.
       Ex : perl wrap\doc_header2doxygen.pl --to ..\vtk-doxygen contrib

Update the doxygen configuration file (wrap\doxyfile) :

     Depending on the location from where you are going to run doxygen
     (here: E:\src\vtk\vtk), update the paths to the VTK
     components (directories) to document. Since the headers have been
     created in '../vtk-doxygen', the following values will be fine :

     INPUT = ../vtk-doxygen/common ../vtk-doxygen/contrib ../vtk-doxygen/graphics ../vtk-doxygen/imaging ../vtk-doxygen/patented


C2. Extract VTK version
    -------------------

     Use the extraction script : doc_version.pl 

For example : 

     E:\src\vtk\vtk> perl wrap\doc_version.pl --to ..\vtk-doxygen

     doc_version.pl 0.16, by Sebastien Barre
     Building version documentation to ../vtk-doxygen/doc_version.dox...
     Finished in 0 s.

Meaning that :

     The optional '--to ..\vtk-doxygen' parameter specifies that the
     script searchs for the VTK version in the current directory
     (common/vtkVersion.h) but stores the related documentation part
     in the '../vtk-doxygen' directory (here, in the
     ../vtk-doxygen/doc_version.dox file).

Update the doxygen configuration file (doxyfile) :

     Comment the PROJECT_NUMBER directive: you do not need it anymore
     as you have just created a more accurate description based on the
     current vtkVersion.h.

    # PROJECT_NUMBER       =


C3. Generate the 'class to example' table 
    -------------------------------------

     Use the referencing script : doc_class2example.pl 

For example : 

     E:\src\vtk\vtk> perl wrap\doc_class2example.pl --to ..\vtk-doxygen

     doc_class2example.pl 0.55, by Sebastien Barre
     Collecting files...
      => 896 file(s) collected in 3 s.
     Parsing files...
      => 896 file(s) parsed in 1 s.
     Eliminating some classes...
      => 1 class(es) eliminated (vtkCommand) in 0 s.
     Locating headers to update...
      => 394 found, 10 orphan class(es) removed (vtkImageShortReader, vtkImageAdaptiveFilter, vtkMINCReader, vtkImageXViewer, vtkImageMIPFilter, vtkImageConnectivity, vtkInteract, vtkImageMarkBoundary, vtkImageSubSampling, vtkImageRegion) in 1 s.
      Building documentation to ..\vtk-doxygen/doc_class2example.dox...
       => VTK 3.1.1, $Revision$, $Date$ (GMT)
       => 394 class(es) examplified by 896 file(s) on Sat Apr 15 18:32:23 2000
       => 3 parser(s) : [Python, C++, Tcl]
       => max limit is 20 example(s) per parser (8% over)
       => in 0 s.
      Updating headers...
       => 393 header(s) updated in 2 s.
      Finished in 7 s.

Meaning that :

     The optional '--to ..\vtk-doxygen' parameter specifies that the
     script searches (and parses) the *examples* (896 here) in the
     current directory (and below) but updates the *headers* (393
     here) stored in the '..\vtk-doxygen' directory (which is good
     because they have just been created in the previous step). These
     headers are updated to include a cross-link between the class and
     the examples using it. The script stores the documentation file
     describing/listing all examples in the
     ..\vtk-doxygen\doc_class2example.dox file. Once again, the files
     located in 'vtk' remain untouched.

Update the doxygen configuration file (doxyfile) :

     Add the relative path to doc_class2example.dox (here :
     ../vtk-doxygen/doc_class2example.dox) to the INPUT directive, so that
     doxygen might process it (see also section D)


C4. Build full-text index 
    ---------------------

     Use the indexing script : doc_index.pl 

For example : 

    E:\src\vtk\vtk>perl wrap\doc_index.pl --stop wrap\doc_index.stop --to ..\vtk-doxygen
    doc_index.pl 0.15, by Sebastien Barre
    Reading stop-words from wrap\doc_index.stop...
    849 word(s) read.
    Collecting files...
    Indexing...
    6875 word(s) grabbed in 620 file(s).
    Removing...
    2734 word(s) removed.
    Grouping...
    1585 word(s) grouped.
    Normalizing...
    normalized to lowercase.
    Building documentation to ../vtk-doxygen/doc_index.dox...
     => 620 file(s) indexed by 2556 word(s) on Sat May 27 18:36:52 2000
     => max limit is 10 xref(s) per word
     => in 7 s.

Meaning that :

     The optional '--to ..\vtk-doxygen' parameter specifies that the
     script parses the VTK headers in the current directory (and
     below) but stores the full-text index in the '..\vtk-doxygen'
     directory, in the ../vtk-doxygen/doc_index.dox file.

     The optional '--stop wrap\doc_index.stop' parameter specifies
     that the script reads its stop-words from a file named
     'wrap\doc_index.stop'.  This file is a list of trivial words (one
     per line) that won't be indexed.

Update the doxygen configuration file (doxyfile) :

     Add the relative path to doc_index.dox (here :
     ../vtk-doxygen/doc_index.dox) to the INPUT directive, so that
     doxygen might process it (see also section D)


D. Run doxygen 
   -----------

Firt check that your doxygen configuration file is OK (doxyfile). The important directives are (for me) :

    PROJECT_NAME         = VTK
    # PROJECT_NUMBER     =
    OUTPUT_DIRECTORY = ../html/Doc
    INPUT                = ../vtk-doxygen/common ../vtk-doxygen/contrib ../vtk-doxygen/graphics ../vtk-doxygen/imaging ../vtk-doxygen/patented ../vtk-doxygen/doc_version.dox ../vtk-doxygen/doc_class2example.dox ../vtk-doxygen/doc_index.dox

Then run doxygen. Be *patient* :)

For example : 

     E:\src\vtk\vtk> doxygen wrap\doxyfile

The 'wrap/doxyfile' parameter is the name of your doxygen configuration file.

The documentation will be located in the '../html/Doc' directory. If
that directory does not exist, doxygen will create it, *unless* the
intermediate directories do not exist either. In my case, I had to
create the '../html' directory manually so that doxygen could create
'Doc' inside '../html'.

You are done.


Just my 2 cents.

