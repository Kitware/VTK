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
 * vtkParseMain.h is the main() function for the wrapper executables.
 *
 * "Usage: vtkWrap [options] input_file output_file"
 * "  --concrete      force concrete class"
 * "  --abstract      force abstract class"
 * "  --vtkobject     vtkObjectBase-derived class"
 * "  --special       non-vtkObjectBase class"
 * "  --hints <file>  hints file"
 * "  --types <file>  type hierarchy file"
 * "  -I <dir>        add an include directory"
 * "  -D <macro>      define a preprocessor macro"
 *
 * After parsing the options, main() will call
 * extern void vtkParseOutput(FILE *ofile, FileInfo *data)
 *
 * The main() function will call exit(1) on error, and
 * vtkParseOutput() should do the same.
*/

#ifndef VTK_PARSE_MAIN_H
#define VTK_PARSE_MAIN_H

/**
 * Options for the wrappers
 */
typedef struct _OptionInfo
{
  char         *InputFileName;
  char         *OutputFileName;
  char         *HintFileName;
  char         *HierarchyFileName;
  int           IsVTKObject;
  int           IsSpecialObject;
  int           IsConcrete;
  int           IsAbstract;
} OptionInfo;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Return the options provided on the command line
 */
OptionInfo *vtkParse_GetCommandLineOptions();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
