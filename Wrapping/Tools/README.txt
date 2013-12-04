Notes on usage of LEX and YACC to generate VTK parse files
----------------------------------------------------------

The contents of this directory are used to generate a C library called
vtkWrappingTools, which provides utility functions for parsing C++ header
files.  The core of the wrapping tools is a parser that is built using
the classic compiler-generator tools lex and yacc.  These tools are
available on OS X and on most linux systems.


LEX:
----

The file vtkParse.l contains regular expressions for tokenizing a C++
header file.  It is used to generate the file lex.yy.c, which is directly
included (i.e. as a C file) by the main parser file, vtkParse.tab.c.

To generate lex.yy.c from vtkParse.l, use the following steps:

1. Get a copy of flex, version 2.5.35 or later
2. Run flex --nodefault -olex.yy.c vtkParse.l
3. Edit the file lex.yy.c as follows, to eliminate compiler warnings and
   to make it pass the git commit hook tests:
a) Convert tabs to 8 spaces, e.g. :%s/\t/        /g
b) Remove extra whitespace from the ends of lines, e.g. :%s/  *$//
c) Remove blank lines at the beginning and end of the file
d) Replace "int yyl;" with "yy_size_t yyl;", e.g. :%s/int yyl;/yy_size_t yyl;/
e) Remove any instances of the "register" keyword.

Some known warnings with recent flex/gcc:

   - Add the following code if not already present to avoid warnings about
     isatty being used without a declaration:
         #ifndef __cplusplus
         extern int isatty(int);
         #endif /* __cplusplus */
   - Change 'int i;' to 'yy_size_t i;' in yy_scan_bytes (line ~3700).
   - Add text after "@param line_number" (line ~3505) since doxygen
     does not permit empty @param paragraphs (clang -Wdocumentation).
     upstream bug: <https://sourceforge.net/p/flex/bugs/158/>

Step "d" removes a potential signed/unsigned comparison compiler
warning.  It might not be necessary in later versions of flex.


YACC:
-----

The file vtkParse.y contains the rules for parsing a C++ header file.
Many of the rules in this file have the same names as in description
of the grammar in the official ISO standard.  The file vtkParse.y is
used to generate the file vtkParse.tab.c, which contains the parser.

1. Get a copy of bison 2.3 or later, it has a yacc-compatible front end.
2. Run yacc -b vtkParse vtkParse.y, it will generate vtkParse.tab.c
3. Edit the file vtkParse.tab.c as follows, to eliminate compiler warnings
   and to make it pass the git commit hook tests:
a) Convert tabs to 8 spaces, e.g. :%s/\t/        /g
b) Remove extra whitespace from the ends of lines, e.g. :%s/  *$//
c) Remove blank lines at the beginning and end of the file
d) Replace all instances of "static inline" with "static".

When yacc is run, it should not report any shift/reduce or reduce/reduce
warnings.  If modifications to the rules cause these warnings to occur,
you can run yacc with the --debug and --verbose options:
 yacc --debug --verbose -b vtkParse vtkParse.y
This will cause yacc to produce a file called "vtkParse.output" that
will show which rules conflict with other rules.

The rules in vtkParse.y avoid most of the ambiguities that are present
in the official C++ grammar by only parsing declarative statements.
Non-declarative statements, such the contents of function bodies, are
simply ignored using the rule "ignore_items".  Constant expressions,
which appear in declarative statements as default argument values or
enum values, are parsed by the rule "constant_expression" which simply
copies the expression into a string without attempting to evaluate it.
