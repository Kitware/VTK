/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParseMain.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 vtkParseMain.h provides argument parsing for the wrapper executables.

 Usage: vtkWrap [options] infile ...

 -D <macro[=def]>  add a macro definition
 -U <macro>        cancel a macro definition
 -I <dir>          add an include directory
 -o <file>         specify the output file
 @<file>           read arguments from a file
 --help            print a help message and exit
 --version         print the VTK version number and exit
 --concrete        force concrete class
 --abstract        force abstract class
 --vtkobject       wrap a vtkObjectBase-derived class
 --special         wrap a non-vtkObjectBase class
 --hints <file>    hints file
 --types <file>    type hierarchy file

 Notes:

 1) The "-o" option is needed when there are multiple input files.
    Otherwise, the output file can be given after the input file.

 2) The "@file" option allows arguments to be stored in a file,
    instead of given on the command line.  The use of such a file
    is sometimes necessary to avoid overflowing the 8191-character
    command-line limit on Windows.  If the file is not found, then
    "@file" will be passed as as a command-line parameter.

 3) The options "--vtkobject" and "--special" are ignored if the
    "--types" option is used.
*/

#ifndef VTK_PARSE_MAIN_H
#define VTK_PARSE_MAIN_H

#include "vtkParseData.h"
#include <stdio.h>

/**
 * Options for the wrappers
 */
typedef struct _OptionInfo
{
  int           NumberOfFiles;     /* the total number of file arguments */
  char        **Files;             /* all of the file arguments */
  char         *InputFileName;     /* the first file argument */
  char         *OutputFileName;    /* the second file, or the "-o" file */
  char         *HintFileName;      /* the file preceded by "--hints" */
  char         *HierarchyFileName; /* the file preceded by "--types" */
  int           IsVTKObject;       /* set when "--vtkobject" is set */
  int           IsSpecialObject;   /* set when "--special" is set */
  int           IsConcrete;        /* set when "--concrete" is set */
  int           IsAbstract;        /* set when "--abstract" is set */
} OptionInfo;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Return the options provided on the command line
 */
OptionInfo *vtkParse_GetCommandLineOptions();

/**
 * The main function, parses the file and returns the result.
 */
FileInfo *vtkParse_Main(int argc, char *argv[]);

/**
 * A main function that can take multiple input files.
 * It does not parse the files.  It will exit on error.
 */
void vtkParse_MainMulti(int argc, char *argv[]);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
