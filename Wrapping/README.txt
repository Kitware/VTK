Notes on usage of LEX and YACC to generate VTK parse files
----------------------------------------------------------

LEX:
----
1. vtkParse.l is edited as necessary
2. LEX is run (should be flex. Last tested on flex version 2.5.4 on Cygwin, 
               but it should work on Linux or on anything that uses flex)
3. LEX spits out lex.yy.c
4. Remove line:
   #include <unistd.h>
   Suggested method is to run:

   perl -ne "s/\t/   /g; next if /unistd/; print" lex.yy.c > lex.yy.c.new
   mv lex.yy.c.new lex.yy.c

   This removes the line and replaces tabs with spaces.

YACC:
-----
1. vtkParse.y is edited as necessary
2. yacc is run: "yacc -b vtkParse vtkParse.y" and spits out vtkParse.tab.c
   (Note: yacc was run RedHat Linux 5.2 1/14/00 by W. Schroeder)
3. Build vtkHTML.exe, vtkWrapTcl.exe, vtkParseJava.exe,vtkWrapJava.exe, 
   vtkWrapPython.exe on the PC and check them in.
4. Check in vtkParse.l, lex.yy.c, vtkParse.y, vtkParse.tab.c


Important Note on YACC:
-----------------------

   Do not use GNU Bison on vtkParse.y.  New versions won't even parse
   the file and old ones will create compile problems on various
   platforms.  Use a recent byacc instead.  Byacc is the Berkeley LALR
   parser generator.  Red Hat Linux apparently installs byacc by
   default.

