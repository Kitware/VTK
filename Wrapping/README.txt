Notes on usage of LEX and YACC to generate VTK parse files
----------------------------------------------------------

LEX:
----
1. vtkParse.l is edited as necessary
2. LEX is run (on an SGI Irix 6.4 on 1/14/00 by W. Schroeder): lex vtkParse.l
3. LEX spits out lex.yy.c
4. Manually edit lex.yy.c:
The line:
FILE *yyin = {stdin}, *yyout = {stdout};
should be changed to:
FILE *yyin, *yyout;


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

