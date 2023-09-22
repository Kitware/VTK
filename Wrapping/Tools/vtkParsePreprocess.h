// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2010 David Gobbi
// SPDX-License-Identifier: BSD-3-Clause
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

#ifndef vtkParsePreprocess_h
#define vtkParsePreprocess_h

#include "vtkParseString.h"
#include "vtkParseSystem.h"
#include "vtkWrappingToolsModule.h"

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
typedef struct MacroInfo_
{
  const char* Name;
  const char* Definition;
  const char* Comment;     /* unused */
  int Ordinal;             /* gives order of definition */
  int NumberOfParameters;  /* only if IsFunction == 1 */
  const char** Parameters; /* symbols for parameters */
  int IsFunction;          /* this macro requires arguments */
  int IsVariadic;          /* this macro can take unlimited arguments */
  int IsExternal;          /* this macro is from an included file */
  int IsExcluded;          /* do not expand this macro */
} MacroInfo;

/**
 * Contains all symbols defined thus far (including those defined
 * in any included header files).
 */
typedef struct PreprocessInfo_
{
  const char* FileName;        /* the file that is being parsed */
  MacroInfo*** MacroHashTable; /* hash table for macro lookup */
  int NumberOfIncludeDirectories;
  const char** IncludeDirectories;
  int NumberOfIncludeFiles; /* all included files */
  const char** IncludeFiles;
  StringCache* Strings;     /* to aid string allocation */
  int IsExternal;           /* label all macros as "external" */
  int ConditionalDepth;     /* internal state variable */
  int ConditionalDone;      /* internal state variable */
  int MacroCounter;         /* for ordering macro definitions */
  int NumberOfMissingFiles; /* include files that cannot be found */
  const char** MissingFiles;
  SystemInfo* System; /* for caching the file system directory */
} PreprocessInfo;

/**
 * Platforms.  Always choose native unless crosscompiling.
 */
typedef enum preproc_platform_t_
{
  VTK_PARSE_NATIVE,
  VTK_PARSE_UNDEF
} preproc_platform_t;

/**
 * Search methods for include files.
 */
typedef enum preproc_search_t_
{
  VTK_PARSE_CURDIR_INCLUDE, /* look in current directory first */
  VTK_PARSE_SOURCE_INCLUDE, /* look in source directory first */
  VTK_PARSE_SYSTEM_INCLUDE  /* search system directories first */
} preproc_search_t;

/**
 * Directive return values.
 */
typedef enum preproc_return_t_
{
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
  VTK_PARSE_SYNTAX_ERROR = 11,   /* any and all syntax errors */
  VTK_PARSE_OUT_OF_MEMORY = 12   /* out-of-memory */
} preproc_return_t;

/**
 * Bitfield for fatal errors.
 */
#define VTK_PARSE_FATAL_ERROR 0xF8

#ifdef __cplusplus
extern "C"
{
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
  VTKWRAPPINGTOOLS_EXPORT
  int vtkParsePreprocess_HandleDirective(PreprocessInfo* info, const char* directive);

  /**
   * Evaluate a preprocessor expression, providing an integer result
   * in "val", and whether it is unsigned in "is_unsigned".  A return
   * value of VTK_PARSE_OK means that no errors occurred, while
   * VTK_PREPROC_DOUBLE, VTK_PREPROC_FLOAT, and VTK_PREPROC_STRING
   * indicate that the preprocessor encountered a non-integer value.
   * Error return values are VTK_PARSE_MACRO_UNDEFINED and
   * VTK_PARSE_SYNTAX_ERRORS.  Undefined macros evaluate to zero.
   */
  VTKWRAPPINGTOOLS_EXPORT
  int vtkParsePreprocess_EvaluateExpression(
    PreprocessInfo* info, const char* text, preproc_int_t* val, int* is_unsigned);

  /**
   * Add all standard preprocessor symbols. Use VTK_PARSE_NATIVE for
   * the platform to add the same macros as the native compiler.  For
   * cross-compiling, use VTK_PARSE_UNDEF and then define the macros
   * for the target platform.
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParsePreprocess_AddStandardMacros(PreprocessInfo* info, preproc_platform_t platform);

  /**
   * Add a preprocessor symbol, including a definition.  Return
   * values are VTK_PARSE_OK and VTK_PARSE_MACRO_REDEFINED.
   */
  VTKWRAPPINGTOOLS_EXPORT
  int vtkParsePreprocess_AddMacro(PreprocessInfo* info, const char* name, const char* definition);

  /**
   * Remove a preprocessor symbol.  Return values are VTK_PARSE_OK
   * and VTK_PARSE_MACRO_UNDEFINED.
   */
  VTKWRAPPINGTOOLS_EXPORT
  int vtkParsePreprocess_RemoveMacro(PreprocessInfo* info, const char* name);

  /**
   * Go through macros in order of definition.
   * Pass NULL to start.  Will return NULL when done.
   */
  VTKWRAPPINGTOOLS_EXPORT
  MacroInfo* vtkParsePreprocess_NextMacro(PreprocessInfo* info, MacroInfo* macro);

  /**
   * Return a preprocessor symbol struct, or NULL if not found.
   */
  VTKWRAPPINGTOOLS_EXPORT
  MacroInfo* vtkParsePreprocess_GetMacro(PreprocessInfo* info, const char* name);

  /**
   * Expand a macro.  A function macro must be given an argstring
   * with args in parentheses, otherwise the argstring can be NULL.
   * returns NULL if the wrong number of arguments were given.
   */
  VTKWRAPPINGTOOLS_EXPORT
  const char* vtkParsePreprocess_ExpandMacro(
    PreprocessInfo* info, MacroInfo* macro, const char* argstring);

  /**
   * Free an expanded macro
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParsePreprocess_FreeMacroExpansion(
    const PreprocessInfo* info, const MacroInfo* macro, const char* text);

  /**
   * Fully process a string with the preprocessor, and
   * return a new string or NULL if a fatal error occurred.
   */
  VTKWRAPPINGTOOLS_EXPORT
  const char* vtkParsePreprocess_ProcessString(PreprocessInfo* info, const char* text);

  /**
   * Free a processed string.  Only call this method if
   * the string returned by ProcessString is different from
   * the original string, because ProcessString will just
   * return the original string if no processing was needed.
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParsePreprocess_FreeProcessedString(const PreprocessInfo* info, const char* text);

  /**
   * Add an include directory.  The directories that were added
   * first will be searched first.
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParsePreprocess_IncludeDirectory(PreprocessInfo* info, const char* name);

  /**
   * Find an include file in the path.  If order is VTK_PARSE_SYSTEM_INCLUDE,
   * then the current directory is ignored unless it is explicitly in the
   * search path.  A null return value indicates that the file was not found.
   * If already_loaded is set, then the file was already loaded.  This
   * preprocessor never loads the same file twice.
   */
  VTKWRAPPINGTOOLS_EXPORT
  const char* vtkParsePreprocess_FindIncludeFile(
    PreprocessInfo* info, const char* filename, preproc_search_t order, int* already_loaded);

  /**
   * Process a file as if included from a source file.  The return value
   * will be VTK_PARSE_OK if no errors occurred.
   */
  VTKWRAPPINGTOOLS_EXPORT
  int vtkParsePreprocess_IncludeFile(
    PreprocessInfo* info, const char* filename, preproc_search_t order);

  /**
   * Initialize a preprocessor symbol struct.
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParsePreprocess_InitMacro(MacroInfo* symbol);

  /**
   * Free a preprocessor macro struct
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParsePreprocess_FreeMacro(MacroInfo* macro);

  /**
   * Initialize a preprocessor struct.
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParsePreprocess_Init(PreprocessInfo* info, const char* filename);

  /**
   * Free a preprocessor struct and its contents;
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParsePreprocess_Free(PreprocessInfo* info);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
/* VTK-HeaderTest-Exclude: vtkParsePreprocess.h */
