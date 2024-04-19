// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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
   * If you call this before calling vtkParse_ParseFile(), then the
   * only macros that are defined will be the ones specified on the
   * command-line (via "-D") or in the header files.  This is called
   * automatically if "-undef" is given on the command line.
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_UndefinePlatformMacros(void);

  /**
   * Use the preprocessor to read all macros from the provided header file.
   * This is called when "-imacros <file>" is given on the command line.
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_IncludeMacros(const char* filename);

  /**
   * Dump all defined macros to the specified file (stdout if NULL).
   * This is called when "-dM" is given on the command line.
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_DumpMacros(const char* filename);

  /**
   * Add an include directory, used by the "-I" command line option.
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_IncludeDirectory(const char* dirname);

  /**
   * Return the full path to a header file.  Results are cached for
   * efficiency.
   */
  VTKWRAPPINGTOOLS_EXPORT
  const char* vtkParse_FindIncludeFile(const char* filename);

  /**
   * Set the command name, for error reporting and diagnostics.
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_SetCommandName(const char* name);

  /**
   * Parse a header file and return a FileInfo, which must be freed by
   * calling vtkParse_Free().
   */
  VTKWRAPPINGTOOLS_EXPORT
  FileInfo* vtkParse_ParseFile(const char* filename, FILE* ifile, FILE* errfile);

  /**
   * Read a hints file and update the FileInfo.  Hints files are mostly
   * obsolete, as they have been replaced by inline hinting.
   */
  VTKWRAPPINGTOOLS_EXPORT
  int vtkParse_ReadHints(FileInfo* data, FILE* hfile, FILE* errfile);

  /**
   * Free the FileInfo struct returned by vtkParse_ParseFile().
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_Free(FileInfo* data);

  /**
   * Free any caches or buffers, call just before program exits.
   * This should only be called after vtkParse_Free() has been used
   * to free all FileInfo objects returned by vtkParse_ParseFile().
   * This function can safely be called multiple times.
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_FinalCleanup(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
/* VTK-HeaderTest-Exclude: vtkParse.h */
