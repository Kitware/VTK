Documenting VTK with doxygen
----------------------------

Sebastien BARRE (Time-stamp: <2000-11-29 15:45:21 barre>)

This file describes how to generate a doxygen-compliant documentation
featuring cross-references between classes and examples, index,
and accurate VTK version. 

Warning
-------

These scripts are now part of the VTK CVS and source distribution.
Nevertheless, new version of this file and the related scripts will be
mirrored at : 
         ftp://sic.sp2mi.univ-poitiers.fr/pub/barre/vtk/doc/

Think twice before letting doxygen eat your CPU cycles : Kitware is
now using these scripts to build an up-to-date doxygen-compliant
documentation every night, as part of the "nightly build"
process. Thus, you might browse a pre-built documentation here :
         http://www.visualizationtoolkit.org/vtk/quality/Doc/html/

...and download the whole documentation (vtkMan.tar.gz) in the usual
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

I assume that your VTK distribution is in a vtk/ directory and that
you do NOT want to modify it in any ways, but rather produce a
modified copy of the necessary files in a different location (here,
the ../vtk-dox/ directory). Hence, you might use this main vtk/
directory for CVS purposes (updating it from time to time), and rerun
the process described below to update your documentation (which is a
straightforward process once you get it).

I also assume that your doxygen configuration script, i.e. the script controlling how this VTK documentation will be created, is named 'doxyfile'. It is read by doxygen and contains directives like :

    PARAMETER = VALUE
 
Comment lines are allowed and start with '#'. Commenting a directive is a simple way to hide it to doxygen :

#   PARAMETER = VALUE

Do not forget to set the OUTPUT_DIRECTORY directive, which holds the location where the whole documentation will be stored. Example :

    OUTPUT_DIRECTORY = ../doc

The crucial INPUT directive controls which directory or files will be processed by doxygen to produce the documentation. Each directory/file is separated by a space. Whenever you will be requested to add a file name to INPUT, just add it to the end of the list. Example :

     INPUT = ../vtk-dox/common ../vtk-dox/contrib ../vtk-dox/graphics ../vtk-dox/imaging ../vtk-dox/patented ../vtk-dox/doc_class2example.dox

My own doxyfile might be found here :

ftp://sic.sp2mi.univ-poitiers.fr/pub/barre/vtk/doc/doxyfile

    WARNING : disable the HAVE_DOT directive if you have not installed
    graphviz : http://www.research.att.com/~north/graphviz/download.html

B. Download doxygen, make sure it runs 
   -----------------------------------

   http://www.stack.nl/~dimitri/doxygen


C1. Convert the VTK headers to doxygen format
    -----------------------------------------

     Use the conversion script : doc_header2doxygen.pl 

For example : 

     E:\src\vtk\vtk> perl doc_header2doxygen.pl --to ..\vtk-dox

     doc_header2doxygen.pl 0.7, by V. Roeim, S. Barre
     Converting...
     620 files converted in 8 s.

Meaning that :

     The '--to ..\vtk-dox' specifies that the script searches *headers*
     (*.h and so on) in the current directory, but will store the
     modified versions of these files in the vtk-dox/ directory. Hence,
     your vtk/ files are untouched. 
     
     Note that you might restrict the conversion to particuliar
     directories/files if you want to document a specific VTK part
     only : just add these as parameters (ex : perl
     doc_header2doxygen.pl -- to ..\vtk-dox contrib).

Update the doxygen configuration file (doxyfile) :

     Depending on the location of doxyfile, update the paths to the
     VTK components (directories) to document. My doxyfile is in the VTK
     directory (vtk/), as well as the Perl scripts, but the headers
     have been created in ../vtk-dox/, therefore :

     INPUT = ../vtk-dox/common ../vtk-dox/contrib ../vtk-dox/graphics ../vtk-dox/imaging ../vtk-dox/patented


C2. Extract VTK version
    -------------------

     Use the extraction script : doc_version.pl 

For example : 

     E:\src\vtk\vtk> perl doc_version --to ..\vtk-dox

     doc_version.pl 0.1, by S. Barre
     Building version documentation to ../vtk-dox/doc_version.dox...
     Finished in 0 s.

Meaning that :

     The '--to ..\vtk-dox' specifies that the script searches the VTK
     version in the current directory (common/vtkVersion.h), but will
     store the related documentation part in the vtk-dox/ directory, in
     the ../vtk-dox/doc_version.dox file.

Update the doxygen configuration file (doxyfile) :

     Comment the PROJECT_NUMBER directive, you do not need it as you
     have just created a more accurate description based on the
     current vtkVersion.h.

    # PROJECT_NUMBER       =


C3. Generate the 'class to example' table 
    -------------------------------------

     Use the referencing script : doc_class2example.pl 

For example : 

     E:\src\vtk\vtk> perl doc_class2example.pl --to ..\vtk-dox

     doc_class2example.pl 0.5, by S. Barre
     Collecting files...
      => 896 file(s) collected in 3 s.
     Parsing files...
      => 896 file(s) parsed in 1 s.
     Eliminating some classes...
      => 1 class(es) eliminated (vtkCommand) in 0 s.
     Locating headers to update...
      => 394 found, 10 orphan class(es) removed (vtkImageShortReader, vtkImageAdaptiveFilter, vtkMINCReader, vtkImageXViewer, vtkImageMIPFilter, vtkImageConnectivity, vtkInteract, vtkImageMarkBoundary, vtkImageSubSampling, vtkImageRegion) in 1 s.
      Building documentation to ..\vtk-dox/doc_class2example.dox...
       => VTK 3.1.1, $Revision$, $Date$ (GMT)
       => 394 class(es) examplified by 896 file(s) on Sat Apr 15 18:32:23 2000
       => 3 parser(s) : [Python, C++, Tcl]
       => max limit is 20 example(s) per parser (8% over)
       => in 0 s.
      Updating headers...
       => 393 header(s) updated in 2 s.
      Finished in 7 s.

Meaning that :

     The '--to ..\vtk-dox' specifies that the script searches (and
     parses) the *examples* (896 here) in the current directory, but
     will use the *headers* (393 here) stored in the vtk-dox/ directory
     (which is good because they have just been created in the
     previous step). These headers will be updated to include a
     cross-link between the class and its examples. The script will
     store the documentation file (describing/listing all examples) in
     the ..\vtk-dox\doc_class2example.dox file. Your vtk/ files are still
     untouched.

Update the doxygen configuration file (doxyfile) :

     Add the relative path to doc_class2example.dox (here :
     ../vtk-dox/doc_class2example.dox) to the INPUT directive, so that
     doxygen might process it (see also section D)


C4. Build full-text index 
    ---------------------

     Use the indexing script : doc_index.pl 

For example : 

    E:\src\vtk\vtk>perl doc_index.pl --to ..\vtk-dox
    doc_index.pl 0.1, by S. Barre
    Reading stop-words...
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
    Building documentation to ../vtk-dox/doc_index.dox...
     => 620 file(s) indexed by 2556 word(s) on Sat May 27 18:36:52 2000
     => max limit is 10 xref(s) per word
     => in 7 s.

Meaning that :

     The '--to ..\vtk-dox' specifies that the script parses the VTK
     headers in the current directory, but will store the full-text
     index in the vtk-dox/ directory, in the ../vtk-dox/doc_index.dox file.

Update the doxygen configuration file (doxyfile) :

     Add the relative path to doc_index.dox (here :
     ../vtk-dox/doc_index.dox) to the INPUT directive, so that
     doxygen might process it (see also section D)


D. Run doxygen 
   -----------

Firt check that your doxygen configuration file is OK (doxyfile). The important directives are (for me) :

    PROJECT_NAME         = VTK
    # PROJECT_NUMBER     =
    OUTPUT_DIRECTORY     = ../doc
    INPUT                = ../vtk-dox/common ../vtk-dox/contrib ../vtk-dox/graphics ../vtk-dox/imaging ../vtk-dox/patented ../vtk-dox/doc_version.dox ../vtk-dox/doc_class2example.dox ../vtk-dox/doc_index.dox

Then run doxygen. Be *patient* :)

For example : 

     E:\src\vtk\vtk> doxygen

The documentation will be located in ../doc

You are done.


Just my 2 cents.

