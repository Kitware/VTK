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
  a C/C++ header file.  It evaluates preprocessor directives
  and stores a list of all preprocessor symbols.

  The preprocessing is meant to be done in-line while the file
  is being parsed, rather than being done ahead of time.  The
  macros stored but not expanded.  Macro-like macros can be
  evaluated using integer math, function-like macros cannot
  be evaluated.

  Since macro expansion is not done, things like concatenation
  and conversion to strings are not possible.  The #include
  directive cannot contain any macros.

  The typical usage of this preprocessor is that the tokenizer
  of the main parser will pass it any preprocessor directives,
  i.e. any lines that begin with '#'.  These lines will be
  evaluated by the preprocessor.  Conditional directives
  will provide a "0" or "1" return value.  All directives are
  passed as raw text.  The text can include any comments and
  escaped newlines, and need not be null-terminated.  The
  preprocessor will automatically stop at the first non-escaped
  newline or null character.

  No checks are done for recursively-defined macros.  If they
  occur, the preprocessor will crash.
*/

#ifndef VTK_PARSE_PREPROCESS_H
#define VTK_PARSE_PREPROCESS_H

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
  int            NumberOfArguments; /* if IsFunction (unused) */
  const char   **Arguments;  /* symbols for arguments (unused) */
  int            IsFunction; /* is a function macro */
  int            IsExternal; /* was from an included file */
} MacroInfo;

/**
 * Contains all symbols defined thus far (including those defined
 * in any included header files).
 */
typedef struct _PreprocessInfo
{
  const char    *FileName;
  int            NumberOfMacros;
  MacroInfo    **Macros;
  int            NumberOfIncludeDirectories;
  const char   **IncludeDirectories;
  int            NumberOfIncludeFiles;
  const char   **IncludeFiles;
  int            IsExternal;
  int            ConditionalDepth;
  int            ConditionalDone;
} PreprocessInfo;

/**
 * Platforms.  Always choose native unless crosscompiling.
 */
enum _preproc_platform_t {
  VTK_PARSE_NATIVE = 0,
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
  VTK_PARSE_SYNTAX_ERROR = 10    /* any and all syntax errors */
};

/**
 * Bitfield for fatal errors.
 */
#define VTK_PARSE_FATAL_ERROR 0xF8

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handle a preprocessor directive.  A return value of 1 means
 * "skip all code until the next directive", a return value of 0
 * indicates a successful evaluation, and any other return value
 * indicates an error.  The preprocessor has an internal state
 * machine that keeps track of conditional if/else/endif directives.
 * The directive string must end either with a non-escaped newline
 * or with a null.
 */
int vtkParsePreprocess_HandleDirective(
  PreprocessInfo *info, const char *directive);

/**
 * Evaluate a preprocessor expression, providing an integer result
 * in "val", and whether it is unsigned in "is_unsigned".  A return
 * value of 0 means that no errors occurred.
 */
int vtkParsePreprocess_EvaluateExpression(
  PreprocessInfo *info, const char *text,
  preproc_int_t *val, int *is_unsigned);

/**
 * Add all standard preprocessor symbols. Use VTK_PARSE_NATIVE
 * as the platform unless you are crosscompiling.
 */
void vtkParsePreprocess_AddStandardMacros(
  PreprocessInfo *info, int platform);

/**
 * Add a preprocessor symbol, including a definition.  A non-zero
 * return value means that the symbol was already present.
 */
int vtkParsePreprocess_AddMacro(
  PreprocessInfo *info, const char *name, const char *definition);

/**
 * Remove a preprocessor symbol.  A non-zero return value means that
 * the symbol was not present.
 */
int vtkParsePreprocess_RemoveMacro(
  PreprocessInfo *info, const char *name);

/**
 * Return a preprocessor symbol struct, or NULL if not found.
 */
MacroInfo *vtkParsePreprocess_GetMacro(
  PreprocessInfo *info, const char *name);

/**
 * Add an include directory.  The directories that were added
 * first will be searched first.
 */
void vtkParsePreprocess_IncludeDirectory(
  PreprocessInfo *info, const char *name);

/**
 * Find an include file in the path.  If system_first is set, then
 * the current directory is ignored unless it is explicitly in the path.
 * A null return value indicates that the file was not found.
 * If already_loaded is set, then the file was already loaded.
 */
const char *vtkParsePreprocess_FindIncludeFile(
  PreprocessInfo *info, const char *filename, int system_first,
  int *already_loaded);

/**
 * Initialize a preprocessor symbol struct.
 */
void vtkParsePreprocess_InitMacro(MacroInfo *symbol);

/**
 * Initialize a preprocessor struct.
 */
void vtkParsePreprocess_InitPreprocess(PreprocessInfo *info);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
