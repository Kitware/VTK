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
--------------------------------------------------------------------------*/

/**
 This file contains utility functions for loading and parsing
 a VTK hierarchy file.  The file contains entries like the
 following (one per line in the file):

 classname [ : superclass ] ; header.h ; vtkKit [; flags]

 For each typedef, the output file will have a line like this:

 name = &[2][3]* const type ; header.h ; vtkKit [; flags]

 For each enum, the output file will have:

 enumname : enum ; header.h ; vtkKit [; flags]

*/

#ifndef VTK_PARSE_HIERARCHY_H
#define VTK_PARSE_HIERARCHY_H

/* Need the ValueInfo struct for typedefs */
#include "vtkParseData.h"

/**
 * One entry from the hierarchy file.
 * It contains a class name, the superclasses, and the header file.
 */
typedef struct _HierarchyEntry
{
  const char  *Name;            /* the class or type name */
  const char  *HeaderFile;      /* header file the class is defined in */
  const char  *Module;          /* library the class is defined in */
  int          NumberOfTemplateParameters; /* number of template params */
  const char **TemplateParameters;
  const char **TemplateDefaults;
  int          NumberOfProperties;   /* number of properties */
  const char **Properties;
  int          NumberOfSuperClasses; /* number of superclasses */
  const char **SuperClasses;
  int         *SuperClassIndex; /* for internal use only */
  ValueInfo   *Typedef;         /* for typedefs */
  int         IsEnum;           /* this entry is for an enum type */
  int         IsTypedef;        /* this entry is for a typedef */
} HierarchyEntry;

/**
 * All the entries from a hierarchy file.
 */
typedef struct _HierarchyInfo
{
  int             NumberOfEntries;
  HierarchyEntry *Entries;
  StringCache    *Strings;
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
 * Return the entry for a class or type, or null if not found
 */
HierarchyEntry *vtkParseHierarchy_FindEntry(
  const HierarchyInfo *info, const char *classname);

/**
 * Get properties for the class.  Returns NULL if the property
 * is not set, and returns either an empty string or a value string
 * if the property is set. The properties supported are as follows:
 * "WRAP_EXCLUDE", "WRAP_SPECIAL", and "ABSTRACT"
 */
const char *vtkParseHierarchy_GetProperty(
  const HierarchyEntry *entry, const char *property);

/**
 * Check whether class is derived from baseclass.
 */
int vtkParseHierarchy_IsTypeOf(const HierarchyInfo *info,
  const HierarchyEntry *entry, const char *baseclass);

/**
 * Check whether class is derived from baseclass.  You must supply
 * the entry for the class (returned by FindEntry) as well as the
 * classname.  The classname can include template args in angle brackets.
 * The baseclass_with_args parameter, if not set to NULL, will be used
 * to return the name of base class with the template args in angle
 * brackets that classname was derived from.  If not set to NULL,
 * this should be freed with free() to avoid a memory leak.
 */
int vtkParseHierarchy_IsTypeOfTemplated(const HierarchyInfo *info,
  const HierarchyEntry *entry, const char *classname,
  const char *baseclass, const char **baseclass_with_args);

/**
 * Free the template args returned by IsTypeOfTemplated
 */
void vtkParseHierarchy_FreeTemplateArgs(int n, const char *args[]);

/**
 * Given a classname with template parameters, get the superclass name
 * with corresponding template parameters.  Returns null if 'i' is out
 * of range, i.e. greater than or equal to the number of superclasses.
 * The returned classname must be freed with "free()".
 */
const char *vtkParseHierarchy_TemplatedSuperClass(
  const HierarchyEntry *entry, const char *classname, int i);

/**
 * Expand all unrecognized types in a ValueInfo struct by
 * using the typedefs in the HierarchyInfo struct.
 */
int vtkParseHierarchy_ExpandTypedefsInValue(
  const HierarchyInfo *info, ValueInfo *data, StringCache *cache,
  const char *scope);

/**
 * Expand typedefs found in a name stored as a string.  The value
 * of "text" will be returned if no expansion occurred, else a new
 * string is returned that must be freed with "free()".
 */
const char *vtkParseHierarchy_ExpandTypedefsInName(
  const HierarchyInfo *info, const char *text, const char *scope);

/**
 * Check whether the named type is an enum type that appears either in
 * the HierarchyInfo or within the ClassInfo.  If the enum type is found,
 * its qualified name is returned, otherwise NULL is returned.
 */
const char *vtkParseHierarchy_QualifiedEnumName(
  HierarchyInfo *hinfo, ClassInfo *data, StringCache *cache,
  const char *name);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
