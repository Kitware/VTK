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

#include "vtkParseData.h"
#include <stddef.h>

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
 * Examples are "name", "::name", "name<arg>", "name::name2",
 * "::name::name2<arg1,arg2>".
 */
size_t vtkParse_NameLength(const char *text);

/**
 * Skip over a name, including any template arguments, but stopping
 * if a '::' is encoutered.  Return the number of characters.
 * Examples are "name" and "name<arg>"
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
 * Returns the number of characters consumed.
 */
size_t vtkParse_ValueInfoFromString(
  ValueInfo *val, StringCache *cache, const char *text);

/**
 * Generate a declaration string from a ValueInfo struct.  If the
 * "nf" arg is set, the returned string must be freed.
 * Only simple text strings are supported, e.g. "const T **".
 * The variable or typedef name, if present, is ignored.
 */
const char *vtkParse_ValueInfoToString(ValueInfo *val, int *nf);

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
  ValueInfo *valinfo, StringCache *cache,
  int n, const char *name[], const char *val[],
  ValueInfo *typedefinfo[]);

/**
 * Wherever one of the specified names exists inside a Value or inside
 * a Dimension size, replace it with the corresponding val string.
 * This is used to replace constants with their values.
 */
void vtkParse_ExpandValues(
  ValueInfo *valinfo, StringCache *cache,
  int n, const char *name[], const char *val[]);

/**
 * Search and replace, return the initial string if no replacements
 * occurred, else return a new string allocated with malloc. */
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
  ClassInfo *data, StringCache *cache, int n, const char *args[]);

/**
 * Instantiate a function or class method template by substituting the
 * provided arguments for the template parameters.  If "n" is less than
 * the number of template parameters, then default parameter values
 * (if present) will be used.  If an error occurs, the error will be
 * printed to stderr and NULL will be returned.
 */
void vtkParse_IntantiateFunctionTemplate(
  FunctionInfo *data, int n, const char *args[]);

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
