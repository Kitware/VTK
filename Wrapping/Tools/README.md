# Wrapping Tools

The wrapping tools consist of executables that pull information from C++
header files, and produce wrapper code that allows the C++ interfaces to
be used from other programming languages (Python and Java).  One can think
of the wrappers as having a front-end that parses C++ header files, and a
back-end that produces language-specific glue code.

All of the code in this directory is C, rather than C++.  One might think
this is silly, since the front-end parses C++ .h files and the back-end
generates .cxx files.  The original reason for this is that the parser
uses lex and yacc, which are written in C and previously could not easily
be linked into C++ programs.


## The C++ Parser

### vtkParse

The header vtkParse.h provides a C API for the C++ parser that wrappers use
to read the VTK header files.  The parser consists of three critical pieces:
a preprocessor (see below), a lex-based lexical analyzer (lex.yy.c, generated
from vtkParse.l) and a bison-based glr parser (vtkParse.tab.c, generated from
vtkParse.y).  Instructions on rebuilding the parser are provided at the end
of this document.

### vtkParsePreprocess

This is a preprocessor that can run independently of the parser.  In general,
the parser does not recursively parse `#include` files, but it does
recursively preprocess them in order to gather all of the macro definitions
within them.

### vtkParseString

This provides low-level string handling routines that are used by the parser
and the preprocessor.  Most importantly, it contains a C++ tokenizer.  It also
contains a cache for storing strings (type names, etc.) that are
encountered during the parse.

### vtkParseSystem

This contains utilities for file system access.  One of its functionalities
is to manage a cache of where header files are located on the file system, so
that header file lookups can be done inexpensively even on slow file systems.

### vtkParseType

This is a header file that defines numerical constants that we use to identify
C++ types, type qualifiers, and specifiers.  These constants are used in the
vtkParseData data structures described below.

### vtkParseAttributes

This is a header file that defines numerical constants for wrapper-specific
attributes that can be added to declarations in the VTK header files.  For
example, `[[vtk::wrapexclude]]` and ``[[vtk::deprecated]]``.  These attribute
constants are stored in the vtkParseData data structures.

### vtkParseData

The data structures defined in vtkParseData.h are used for the output of the
parser.  This header provides data structures for namespaces, classes, methods,
typedefs, and for other entities that can be declared in a C++ file.  The
wrappers convert this data into wrapper code.


## Parser Utilities

### vtkParseExtras

This file provides routines for managing certain abstractions of the data that
is produced by the parser.  Most specifically, it provides facilities for
expanding typedefs and for instantiating templates.  Its code is not pretty.

### vtkParseMerge

This provides methods for dealing with method resolution order.  It defines
a data structure for managing a class along with all the classes it derives
from.  It is needed for managing tricky details relating to inheritance,
such as "using" declarations, overrides, virtual methods, etc.

### vtkParseMangle

The Python wrappers rely on name-mangling routines to convert C++ names into
names that can be used in Python.  The mangling is done according to the
rules of the IA64 ABI (this same mangling is used to convert C++ APIs into
C APIs)

### vtkParseHierarchy

A hierarchy file is a text file that lists information about all the types
defined in a VTK module.  The wrappers use these files to look up types from
names.  Through the use of vtkParseHierarchy, the wrappers can get detailed
information about a type even if the header file only contains a forward
reference, as long as the type is defined somewhere in another header.

### vtkParseMain

A common main() function for use by wrapper tool executables.  It provides a
standard set of command-line options as well as response-file handling.  It
also invokes the parser.


## Wrapper Utilities

### vtkWrap

This has functions that are common to the wrapper tools for all the wrapper
languages.  Unlikely vtkParse, it deals with the generation of code, rather
than the parsing of code.

### vtkWrapText

This has functions for automatically generating documentation from the
header files that are parsed.  It produces the Python docstrings.


## Python-Specific Utilities

These are named according to the pieces of wrapper code they produce.

* __vtkWrapPythonClass__ creates type objects for vtkObjectBase classes
* __vtkWrapPythonType__ creates type objects for other wrapped classes
* __vtkWrapPythonMethod__ for calling C++ methods from Python
* __vtkWrapPythonOverload__ maps a Python method to multiple C++ overloads
* __vtkWrapPythonMethodDef__ generates the method tables for wrapped classes
* __vtkWrapPythonTemplate__ for wrapping of C++ class templates
* __vtkWrapPythonNamespace__ for wrapping namespaces
* __vtkWrapPythonEnum__ creates type objects for enum types
* __vtkWrapPythonConstant__ adds C++ constants to Python classes, namespaces


## Python Wrapper Executables

### vtkWrapPython

This executable will parse the C++ declarations from a header file and
produce wrapper code that can be linked into a Python extension module.

### vtkWrapPythonInit

This will produce the PyInit entry point for a Python extension module,
as well as code for loading all the dependent modules.  The .cxx file
produced by vtkWrapPythonInit is linked together to the .cxx files that
are produced by vtkWrapPython to create the module.


## Java Wrapper Executables

* __vtkWrapJava__ produces C++ wrapper code that uses the JNI
* __vtkParseJava__ produces Java code that sits on top of the C++ code


## Other Executables

### vtkWrapHierarchy

This will slurp up all the header files in a VTK module and produce a
"hierarchy.txt" file that provides information about all of the types that
are defined in that module.  In other words, it provides a summary of the
module's contents.  The Python and Java wrapper executables rely on these
"hierarchy.txt" files in order to look up types by name.


## Rebuilding the Parser

The code for the C++ parser is generated from the files vtkParse.l and
vtkParse.y with the classic compiler-generator tools lex and yacc (or,
more specifically, with their modern incarnations flex and bison).  These
tools are readily available on macOS and Linux systems, and they can be
installed (with some difficulty) on Windows systems.

The C code that flex and bison generate is not styled according to VTK
standards, and must be cleaned up in order to compile without warnings and
in order to satisfy VTK's git hooks and style checks.

### vtkParse.l

The file vtkParse.l contains regular expressions for tokenizing a C++
header file.  It is used to generate the file lex.yy.c, which is directly
included (i.e. as a C file) by the main parser file, vtkParse.tab.c.

To generate lex.yy.c from vtkParse.l, use the following steps.

1. Get a copy of flex, version 2.6.4 or later
2. Run `flex --nodefault --noline -olex.yy.c vtkParse.l`
3. In an editor, remove blank lines from the top and bottom of lex.yy.c
4. Replace all tabs with two spaces (e.g. `:%s/\t/  /g` in vi)
5. Remove spaces from the ends of lines (e.g. `:%s/  *$//` in vi)
6. Remove `struct yy_trans_info`, which is used nowhere in the code
7. Add the following code at line 23 (after "`end standard C headers`")

    #ifndef __cplusplus
    extern int isatty(int);
    #endif /* __cplusplus */

Finally, if you have clang-format installed, you can use it to re-style
the code.

### vtkParse.y

The file vtkParse.y contains the rules for parsing a C++ header file.
Many of the rules in this file have the same names as in the description
of the grammar in the official ISO standard.  The file vtkParse.y is
used to generate the file vtkParse.tab.c, which contains the parser.

1. Get a copy of bison 3.2.3 or later, it has a yacc-compatible front end.
2. Run `bison --no-lines -b vtkParse vtkParse.y`, to generate vtkParse.tab.c
3. In an editor, replace every `static inline` in vtkParse.tab.c with `static`
4. Replace `#if ! defined lint || defined __GNUC__` with `#if 1`
5. remove `YY_ATTRIBUTE_UNUSED` from `yyfillin`, `yyfill`, and `yynormal`
6. comment out the `break;` after `return yyreportAmbiguity`
7. replace `(1-yyrhslen)` with `(1-(int)yyrhslen)`
8. replace `sizeof yynewStates[0]` and `sizeof yyset->yystates[0]` with `sizeof (yyGLRState*)`
9. replace `sizeof yynewLookaheadNeeds[0]` and `sizeof yyset->yylookaheadNeeds[0]` with `sizeof (yybool)`
10. replace `sizeof yynewItems[0]` and `sizeof yystackp->yynextFree[0]` with `sizeof (yyGLRStackItem)`

If you are familiar with "diff" and "patch" and if you have clang-format,
you can automate these code changes as follows.  For this, you must use
exactly version 3.2.3 of bison to ensure that the code that is produced
is as similar as possible to what is currently in the VTK repository.

1. Run bison (as above) on the vtkParse.y from the master branch
2. Use clang-format-8 to re-style vtkParse.tab.c to match VTK code style
3. Use "git diff -R vtkParse.tab.c" to produce a patch file

If done correctly, this will produce a patch file that contains all the
changes above (steps 3 through 9 in the original list).  Load the patch
file into a text editor to verify that this is so, and remove any superfluous
changes from the patch file.

Then, switch to your new vtkParse.y (the one you have modified).  Repeat
steps 1 and 2 (generate vtkParse.tab.c and reformat it with clang-format).
Now you can apply the patch file to automate the original steps 3 through 9.
Note that as you continue to edit vtkParse.y and regenerate vtkParse.tab.c,
you can continue to use the same patch.  Just remember to run clang-format
every time that you run bison.

### Debugging the Parser

When bison is run, it should not report any shift/reduce or reduce/reduce
warnings.  If modifications to the rules cause these warnings to occur,
you can run bison with the `--debug` and `--verbose` options:

    bison --debug --verbose -b vtkParse vtkParse.y

This will cause bison to produce a file called "vtkParse.output" that
will show which rules conflict with other rules.
