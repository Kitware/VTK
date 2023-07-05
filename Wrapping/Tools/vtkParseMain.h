// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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
*/

#ifndef vtkParseMain_h
#define vtkParseMain_h

#include "vtkParseData.h"
#include "vtkWrappingToolsModule.h"
#include <stdio.h>
#ifdef _WIN32
#include <stddef.h> /* for wchar_t */
#endif

/**
 * Options for the wrappers
 */
typedef struct OptionInfo_
{
  int NumberOfFiles;              /* the total number of file arguments */
  char** Files;                   /* all of the file arguments */
  char* InputFileName;            /* the first file argument */
  char* OutputFileName;           /* the second file, or the "-o" file */
  int NumberOfHintFileNames;      /* the total number of hints arguments */
  char** HintFileNames;           /* all of the hints arguments */
  int NumberOfHierarchyFileNames; /* the total number of types argument */
  char** HierarchyFileNames;      /* the file preceded by "--types" */
  int DumpMacros;                 /* dump macros to output */
} OptionInfo;

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * Return the options provided on the command line
   */
  VTKWRAPPINGTOOLS_EXPORT
  OptionInfo* vtkParse_GetCommandLineOptions(void);

  /**
   * The main function, parses the file and returns the result.
   */
  VTKWRAPPINGTOOLS_EXPORT
  FileInfo* vtkParse_Main(int argc, char* argv[]);

  /**
   * A main function that can take multiple input files.
   * It does not parse the files.  It will exit on error.
   */
  VTKWRAPPINGTOOLS_EXPORT
  StringCache* vtkParse_MainMulti(int argc, char* argv[]);

#ifdef _WIN32

  /**
   * Converts wmain args to utf8. This function can only be called once.
   * The caller is permitted to modify the returned argument array.
   */
  VTKWRAPPINGTOOLS_EXPORT
  char** vtkParse_WideArgsToUTF8(int argc, wchar_t* wargv[]);

#endif /* _WIN32 */

#ifdef __cplusplus
} /* extern "C" */
#endif

#if defined(_WIN32) && !defined(__MINGW32__)

/* This macro will define wmain() on Win32 and will handle conversion to UTF8 */
#define VTK_PARSE_MAIN(a, b)                                                                       \
  main_with_utf8_args(a, b);                                                                       \
  int wmain(int argc, wchar_t* wargv[])                                                            \
  {                                                                                                \
    char** argv = vtkParse_WideArgsToUTF8(argc, wargv);                                            \
    return main_with_utf8_args(argc, argv);                                                        \
  }                                                                                                \
  int main_with_utf8_args(a, b)

#else

#define VTK_PARSE_MAIN(a, b) main(a, b)

#endif

#endif
/* VTK-HeaderTest-Exclude: vtkParseMain.h */
