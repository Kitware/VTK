/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParseHierarchy.h

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
 This file contains utility functions for loading and parsing
 a VTK hierarchy file.  The file contains entries like the
 following (one per line in the file):

 classname [ : superclass ] ; header.h

 For each typedef, the output file will have a line like this:

 name = &[2][3]* const type ; header.h

 For each enum, the output file will have:

 enumname : int ; header.h

*/

#ifndef VTK_PARSE_HIERARCHY_H
#define VTK_PARSE_HIERARCHY_H

/**
 * One entry from the hierarchy file.
 * It contains a class name, the superclasses, and the header file.
 */
typedef struct _HierarchyEntry
{
  char  *ClassName;
  char  *HeaderFile;
  int    NumberOfSuperClasses;
  char **SuperClasses;
  int   *SuperClassIndex;
  int    NumberOfProperties;
  char **Properties;
} HierarchyEntry;

/**
 * All the entries from a hierarchy file.
 */
typedef struct _HierarchyInfo
{
  int             NumberOfClasses;
  HierarchyEntry *Classes;
} HierarchyInfo;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Read a hierarchy file into a HeirarchyInfo struct, or return NULL
 */
HierarchyInfo *vtkParseHierarchy_ReadFile(const char *filename);

/**
 * Free a HierarchyInfo struct
 */
void vtkParseHierarchy_Free(HierarchyInfo *info);

/**
 * Check whether a class is outside the heirarchy
 */
int vtkParseHierarchy_IsExtern(
  const HierarchyInfo *intp, const char *classname);

/**
 * Check whether class 1 is a subclass of class 2
 */
int vtkParseHierarchy_IsTypeOf(
  const HierarchyInfo *info, const char *subclass, const char *superclass);

/**
 * Get the header file for the specified class
 */
const char *vtkParseHierarchy_ClassHeader(
  const HierarchyInfo *info, const char *classname);

/**
 * Get the nth superclass for specified class, or return NULL.
 *
 * Calling with i=0 will give the first superclass or NULL.
 */
const char *vtkParseHierarchy_ClassSuperClass(
  const HierarchyInfo *info, const char *classname, int i);

/**
 * Get properties for the class.  Returns NULL if the property
 * is not set.  Only a few properties are supported so far:
 * "WRAP_EXCLUDE", "WRAP_SPECIAL", and "ABSTRACT"
 */
const char *vtkParseHierarchy_GetProperty(
  const HierarchyInfo *intp, const char *classname, const char *property);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
