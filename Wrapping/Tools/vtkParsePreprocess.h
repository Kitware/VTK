/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParsePreprocess.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright (c) 2010 David Gobbi.

  Contributed to the VisualizationToolkit by the author in June 2010
  under the terms of the Visualization Toolkit 2008 copyright.
-------------------------------------------------------------------------*/

/**
  This file provides subroutines to assist in preprocessing
  C/C++ header files.  It evaluates preprocessor directives
  and stores a list of all preprocessor macros.

  The preprocessing is done in-line while the file is being
  parsed.  Macros that are defined in the file are stored but
  are not automatically expanded.  The parser can query the
  macro definitions, expand them into plain text, or ask the
  preprocessor to evaluate them and return an integer result.

  The typical usage of this preprocessor is that the main
  parser will pass any lines that begin with '#' to the
  vtkParsePreprocess_HandleDirective() function, which will
  evaluate the line and provide a return code.  The return
  code will tell the main parser if a syntax error or macro
  lookup error occurred, and will also let the parser know
  if an #if or #else directive requires that the next block
  of code be skipped.
*/

#ifndef VTK_PARSE_PREPROCESS_H
#define VTK_PARSE_PREPROCESS_H

#include "vtkParseString.h"

/**
 * The preprocessor int type.  Use the compiler's longest int type.
 */
#if defined(_WIN32) && !defined(__MINGW32__) && !defined(__CYGWIN__)
typedef __int64 preproc_int_t;
typedef unsigned __int64 preproc_uint_t;
#else
typedef long long preproc_int_t;
typedef unsigned long long preproc_uint_t;
#endif

/**
 * Struct to describe a preprocessor symbol.
 */
typedef struct _MacroInfo
{
  const char    *Name;
  const char    *Definition;
  const char    *Comment; /* unused */
  int            NumberOfParameters; /* only if IsFunction == 1 */
  const char   **Parameters; /* symbols for parameters */
  int            IsFunction; /* this macro requires arguments */
  int            IsVariadic; /* this macro can take unlimited arguments */
  int            IsExternal; /* this macro is from an included file */
  int            IsExcluded; /* do not expand this macro */
} MacroInfo;

/**
 * Contains all symbols defined thus far (including those defined
 * in any included header files).
 */
typedef struct _PreprocessInfo
{
  const char    *FileName;         /* the file that is being parsed */
  MacroInfo   ***MacroHashTable;   /* hash table for macro lookup */
  int            NumberOfIncludeDirectories;
  const char   **IncludeDirectories;
  int            NumberOfIncludeFiles; /* all included files */
  const char   **IncludeFiles;
  StringCache   *Strings;          /* to aid string allocation */
  int            IsExternal;       /* label all macros as "external" */
  int            ConditionalDepth; /* internal state variable */
  int            ConditionalDone;  /* internal state variable */
} PreprocessInfo;

/**
 * Platforms.  Always choose native unless crosscompiling.
 */
enum _preproc_platform_t {
  VTK_PARSE_NATIVE = 0
};

/**
 * Directive return values.
 */
enum _preproc_return_t {
  VTK_PARSE_OK = 0,
  VTK_PARSE_SKIP = 1,            /* skip next block */
  VTK_PARSE_PREPROC_DOUBLE = 2,  /* encountered a double */
  VTK_PARSE_PREPROC_FLOAT = 3,   /* encountered a float */
  VTK_PARSE_PREPROC_STRING = 4,  /* encountered a string */
  VTK_PARSE_MACRO_UNDEFINED = 5, /* macro lookup failed */
  VTK_PARSE_MACRO_REDEFINED = 6, /* attempt to redefine a macro */
  VTK_PARSE_FILE_NOT_FOUND = 7,  /* include file not found */
  VTK_PARSE_FILE_OPEN_ERROR = 8, /* include file not readable */
  VTK_PARSE_FILE_READ_ERROR = 9, /* error during read */
  VTK_PARSE_MACRO_NUMARGS = 10,  /* wrong number of args to func macro */
  VTK_PARSE_SYNTAX_ERROR = 11    /* any and all syntax errors */
};

/**
 * Bitfield for fatal errors.
 */
#define VTK_PARSE_FATAL_ERROR 0xF8

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handle a preprocessor directive.  Return value VTK_PARSE_OK
 * means that no errors occurred, while VTK_PARSE_SKIP means that
 * a conditional directive was encountered and the next code
 * block should be skipped.  The preprocessor has an internal state
 * machine that keeps track of conditional if/else/endif directives.
 * All other return values indicate errors, and it is up to the
 * parser to decide which errors are fatal.  The preprocessor
 * only considers syntax errors and I/O errors to be fatal.
 */
int vtkParsePreprocess_HandleDirective(
  PreprocessInfo *info, const char *directive);

/**
 * Evaluate a preprocessor expression, providing an integer result
 * in "val", and whether it is unsigned in "is_unsigned".  A return
 * value of VTK_PARSE_OK means that no errors occurred, while
 * VTK_PREPROC_DOUBLE, VTK_PREPROC_FLOAT, and VTK_PREPROC_STRING
 * indicate that the preprocessor encountered a non-integer value.
 * Error return values are VTK_PARSE_MACRO_UNDEFINED and
 * VTK_PARSE_SYNTAX_ERRORS.  Undefined macros evaluate to zero.
 */
int vtkParsePreprocess_EvaluateExpression(
  PreprocessInfo *info, const char *text,
  preproc_int_t *val, int *is_unsigned);

/**
 * Add all standard preprocessor symbols. Use VTK_PARSE_NATIVE
 * as the platform.  In the future, other platform specifiers
 * might be added to allow crosscompiling.
 */
void vtkParsePreprocess_AddStandardMacros(
  PreprocessInfo *info, int platform);

/**
 * Add a preprocessor symbol, including a definition.  Return
 * values are VTK_PARSE_OK and VTK_PARSE_MACRO_REDEFINED.
 */
int vtkParsePreprocess_AddMacro(
  PreprocessInfo *info, const char *name, const char *definition);

/**
 * Remove a preprocessor symbol.  Return values are VTK_PARSE_OK
 * and VTK_PARSE_MACRO_UNDEFINED.
 */
int vtkParsePreprocess_RemoveMacro(
  PreprocessInfo *info, const char *name);

/**
 * Return a preprocessor symbol struct, or NULL if not found.
 */
MacroInfo *vtkParsePreprocess_GetMacro(
  PreprocessInfo *info, const char *name);

/**
 * Expand a macro.  A function macro must be given an argstring
 * with args in parentheses, otherwise the argstring can be NULL.
 * returns NULL if the wrong number of arguments were given.
 */
const char *vtkParsePreprocess_ExpandMacro(
  PreprocessInfo *info, MacroInfo *macro, const char *argstring);

/**
 * Free an expanded macro
 */
void vtkParsePreprocess_FreeMacroExpansion(
  PreprocessInfo *info, MacroInfo *macro, const char *text);

/**
 * Fully process a string with the preprocessor, and
 * return a new string or NULL if a fatal error occurred.
 */
const char *vtkParsePreprocess_ProcessString(
  PreprocessInfo *info, const char *text);

/**
 * Free a processed string.  Only call this method if
 * the string returned by ProcessString is different from
 * the original string, because ProcessString will just
 * return the original string if no processing was needed.
 */
void vtkParsePreprocess_FreeProcessedString(
  PreprocessInfo *info, const char *text);

/**
 * Add an include directory.  The directories that were added
 * first will be searched first.
 */
void vtkParsePreprocess_IncludeDirectory(
  PreprocessInfo *info, const char *name);

/**
 * Find an include file in the path.  If system_first is set, then
 * the current directory is ignored unless it is explicitly in the
 * path.  A null return value indicates that the file was not found.
 * If already_loaded is set, then the file was already loaded.  This
 * preprocessor never loads the same file twice.
 */
const char *vtkParsePreprocess_FindIncludeFile(
  PreprocessInfo *info, const char *filename, int system_first,
  int *already_loaded);

/**
 * Initialize a preprocessor symbol struct.
 */
void vtkParsePreprocess_InitMacro(MacroInfo *symbol);

/**
 * Free a preprocessor macro struct
 */
void vtkParsePreprocess_FreeMacro(MacroInfo *macro);

/**
 * Initialize a preprocessor struct.
 */
void vtkParsePreprocess_Init(
  PreprocessInfo *info, const char *filename);

/**
 * Free a preprocessor struct and its contents;
 */
void vtkParsePreprocess_Free(PreprocessInfo *info);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
