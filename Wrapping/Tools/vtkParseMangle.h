/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParseMangle.h

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
 * This file contains utilities doing name mangling
 */

#ifndef vtkParseMangle_h
#define vtkParseMangle_h

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
