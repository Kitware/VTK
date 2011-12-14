/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParseExtras.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright (c) 2011 David Gobbi.

  Contributed to the VisualizationToolkit by the author in May 2011
  under the terms of the Visualization Toolkit 2008 copyright.
-------------------------------------------------------------------------*/

/**
 * This file contains extra utilities for parsing and wrapping.
 */

#ifndef VTK_PARSE_EXTRAS_H
#define VTK_PARSE_EXTRAS_H

#include "vtkParse.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Skip over a sequence of characters that begin with an alphabetic
 * character or an underscore, and include only alphanumeric
 * characters or underscores. Return the number of characters.
 */
size_t vtkParse_IdentifierLength(const char *text);

/**
 * Skip over a name, including any namespace prefixes and
 * any template arguments.  Return the number of characters.
 */
size_t vtkParse_NameLength(const char *text);

/**
 * Skip over a name, including any template parameters, but stopping
 * if a '::' is encoutered.  Return the number of characters.
 */
size_t vtkParse_UnscopedNameLength(const char *text);

/**
 * Skip over a literal, which may be a number, a char in single
 * quotes, a string in double quotes, or a name, or a name followed
 * by arguments in parentheses.
 */
size_t vtkParse_LiteralLength(const char *text);

/**
 * Get a type from a type name, and return the number of characters used.
 * If the "classname" argument is not NULL, then it is used to return
 * the short name for the type, e.g. "long int" becomes "long", while
 * typedef names and class names are returned unchanged.  If "const"
 * appears in the type name, then the const bit flag is set for the
 * type, but "const" will not appear in the returned classname.
 */
size_t vtkParse_BasicTypeFromString(
  const char *text, unsigned int *type,
  const char **classname, size_t *classname_len);

/**
 * Generate a ValueInfo by parsing the type from the provided text.
 * Only simple text strings are supported, e.g. "const T **".
 */
void vtkParse_ValueInfoFromString(ValueInfo *val, const char *text);

/**
 * Expand a typedef within a variable, parameter, or typedef declaration.
 * The expansion is done in-place.
 */
void vtkParse_ExpandTypedef(ValueInfo *valinfo, ValueInfo *typedefinfo);

/**
 * Expand any unrecognized types within a variable, parameter, or typedef
 * that match any of the supplied typedefs. The expansion is done in-place.
 */
void vtkParse_ExpandTypedefs(
  ValueInfo *valinfo, int n, const char *name[], const char *val[],
  ValueInfo *typedefinfo[]);

/**
 * Wherever one of the specified names exists inside a Value or inside
 * a Dimension size, replace it with the corresponding val string.
 * This is used to replace constants with their values.
 */
void vtkParse_ExpandValues(
  ValueInfo *valinfo, int n, const char *name[], const char *val[]);

/**
 * Search for all occurences of "name" and replace with the corresponding
 * "val", return the initial string if no replacements occurred, otherwise
 * return a new string allocated with malloc.
 */
const char *vtkParse_StringReplace(
  const char *str1, int n, const char *name[], const char *val[]);

/**
 * Extract the class name and template args from a templated
 * class type ID.  Returns the full number of characters that
 * were consumed during the decomposition.
 */
size_t vtkParse_DecomposeTemplatedType(
  const char *text, const char **classname,
  int n, const char ***args, const char *defaults[]);

/**
 * Free the list of strings returned by ExtractTemplateArgs.
 */
void vtkParse_FreeTemplateDecomposition(
  const char *classname, int n, const char **args);

/**
 * Instantiate a class template by substituting the provided arguments
 * for the template parameters. If "n" is less than the number of template
 * parameters, then default parameter values (if present) will be used.
 * If an error occurs, the error will be printed to stderr and NULL will
 * be returned.
 */
void vtkParse_InstantiateClassTemplate(
  ClassInfo *data, int n, const char *args[]);

/**
 * Instantiate a function or class method template by substituting the
 * provided arguments for the template parameters.  If "n" is less than
 * the number of template parameters, then default parameter values
 * (if present) will be used.  If an error occurs, the error will be
 * printed to stderr and NULL will be returned.
 */
void vtkParse_IntantiateFunctionTemplate(
  FunctionInfo *data, int n, const char *args);

/**
 * Generate a mangled name for a type, use gcc ia64 ABI.
 * The result is placed in new_name, which must be large enough
 * to accept the result.
 */
size_t vtkParse_MangledTypeName(const char *name, char *new_name);

/**
 * Generate a mangled name for a literal.  Only handles decimal
 * integer literals.  It guesses type from suffix "u", "ul",
 * "ull", "l", "ll" so only certain types are supported.
 */
size_t vtkParse_MangledLiteral(const char *name, char *new_name);

/**
 * Get a zero-terminated array of the types in vtkTemplateMacro.
 */
const char **vtkParse_GetTemplateMacroTypes();

/**
 * Get a zero-terminated array of the types in vtkArray.
 */
const char **vtkParse_GetArrayTypes();


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
