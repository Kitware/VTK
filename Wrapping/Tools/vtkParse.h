/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParse.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
  This is the header file for vtkParse.tab.c, which is generated
  from vtkParse.y with the "yacc" compiler-compiler.
*/

#ifndef vtkParse_h
#define vtkParse_h

#include "vtkParseData.h"
#include "vtkParseType.h"
#include "vtkWrappingToolsModule.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * Define a preprocessor macro. Function macros are not supported.
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_DefineMacro(const char* name, const char* definition);

  /**
   * Undefine a preprocessor macro.
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_UndefineMacro(const char* name);

  /**
   * Do not pre-define any macros related to the system or platform.
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_UndefinePlatformMacros(void);

  /**
   * Read macros from the provided header file.
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_IncludeMacros(const char* filename);

  /**
   * Dump macros to the specified file (stdout if NULL).
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_DumpMacros(const char* filename);

  /**
   * Add an include directory, for use with the "-I" option.
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_IncludeDirectory(const char* dirname);

  /**
   * Return the full path to a header file.
   */
  VTKWRAPPINGTOOLS_EXPORT
  const char* vtkParse_FindIncludeFile(const char* filename);

  /**
   * Set the command name, for error reporting and diagnostics.
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_SetCommandName(const char* name);

  /**
   * Parse a header file and return a FileInfo struct
   */
  VTKWRAPPINGTOOLS_EXPORT
  FileInfo* vtkParse_ParseFile(const char* filename, FILE* ifile, FILE* errfile);

  /**
   * Read a hints file and update the FileInfo
   */
  VTKWRAPPINGTOOLS_EXPORT
  int vtkParse_ReadHints(FileInfo* data, FILE* hfile, FILE* errfile);

  /**
   * Free the FileInfo struct returned by vtkParse_ParseFile()
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_Free(FileInfo* data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
/* VTK-HeaderTest-Exclude: vtkParse.h */
