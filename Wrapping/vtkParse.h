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

#ifndef VTK_PARSE_H
#define VTK_PARSE_H

#include "vtkParseType.h"
#include <stdio.h>

/* legacy */
#define MAX_ARGS 20

/**
 * ItemType constants
 */
typedef enum _parse_item_t
{
  VTK_NAMESPACE_INFO = 1,
  VTK_CLASS_INFO     = 2,
  VTK_STRUCT_INFO    = 3,
  VTK_UNION_INFO     = 4,
  VTK_ENUM_INFO      = 5,
  VTK_FUNCTION_INFO  = 6,
  VTK_VARIABLE_INFO  = 7,
  VTK_CONSTANT_INFO  = 8,
  VTK_TYPEDEF_INFO   = 9
} parse_item_t;

/**
 * Access flags
 */
typedef enum _parse_access_t
{
  VTK_ACCESS_PUBLIC    = 0,
  VTK_ACCESS_PROTECTED = 1,
  VTK_ACCESS_PRIVATE   = 2
} parse_access_t;


struct _TemplateArgs;
struct _FunctionInfo;

/**
 * TemplateArgs holds one template arg
 */
typedef struct _TemplateArg
{
  unsigned int  Type;  /* is zero for "typename", "class", "template" */
  char         *Class; /* class name for type */
  char         *Name;  /* name of template arg */
  char         *Value; /* default value */
  struct _TemplateArgs *Template; /* for templated template args */
} TemplateArg;

/**
 * TemplateArgs holds template definitions
 */
typedef struct _TemplateArgs
{
  int           NumberOfArguments;
  TemplateArg **Arguments;
} TemplateArgs;

/**
 * ItemInfo is a "base class" for items in the header file
 */
typedef struct _ItemInfo
{
  parse_item_t   ItemType;
  parse_access_t Access;
  char          *Name;
  char          *Comment;
} ItemInfo;

/**
 * ValueInfo is for typedefs, constants, variables,
 * function arguments, and return values
 *
 * Note that Dimensions is an array of char pointers, in
 * order to support dimensions that are sized according to
 * template arg values or accoring to named constants.
 */
typedef struct _ValueInfo
{
  parse_item_t   ItemType;
  parse_access_t Access;
  char          *Name;
  char          *Comment;
  char          *Value;      /* for vars or default arg values */
  unsigned int   Type;       /* as defined in vtkParseType.h   */
  char          *Class;      /* classname for type */
  int            Count;      /* total number of values, if known */
  int            NumberOfDimensions; /* dimensionality for arrays */
  char         **Dimensions; /* dimensions for arrays */
  struct _FunctionInfo *Function;  /* for function pointer values */
  int            IsStatic;   /* for class variables only */
  int            IsEnum;     /* for constants only */
} ValueInfo;

/**
 * FunctionInfo is for functions and methods
 */
typedef struct _FunctionInfo
{
  parse_item_t   ItemType;
  parse_access_t Access;
  char          *Name;
  char          *Comment;
  char          *Signature;   /* function signature as text */
  TemplateArgs  *Template;    /* template args, or NULL */
  int            NumberOfArguments;
  ValueInfo    **Arguments;
  ValueInfo     *ReturnValue; /* NULL for constructors and destructors */
  int            IsOperator;
  int            IsVariadic;
  int            IsLegacy;    /* marked as a legacy method or function */
  int            IsStatic;    /* methods only */
  int            IsVirtual;   /* methods only */
  int            IsPureVirtual; /* methods only */
  int            IsConst;     /* methods only */
  unsigned int   ArgTypes[MAX_ARGS];  /* legacy */
  char          *ArgClasses[MAX_ARGS];/* legacy */
  int            ArgCounts[MAX_ARGS]; /* legacy */
  unsigned int   ReturnType;  /* legacy */
  char          *ReturnClass; /* legacy */
  int            HaveHint;    /* legacy */
  int            HintSize;    /* legacy */
  int            ArrayFailure;/* legacy */
  int            IsPublic;    /* legacy */
  int            IsProtected; /* legacy */
} FunctionInfo;

/**
 * EnumInfo is for enums
 * Constants are at the same level as the Enum, not inside it.
 */
typedef struct _EnumInfo
{
  parse_item_t   ItemType;
  parse_access_t Access;
  char          *Name;
  char          *Comment;
} EnumInfo;

/**
 * UnionInfo is for unions (not implemented yet)
 */
typedef struct _UnionInfo
{
  parse_item_t   ItemType;
  parse_access_t Access;
  char          *Name;
  char          *Comment;
  int            NumberOfMembers;
  ValueInfo    **Members;
} UnionInfo;

/**
 * ClassInfo is for classes and structs
 */
typedef struct _ClassInfo
{
  parse_item_t   ItemType;
  parse_access_t Access;
  char          *Name;
  char          *Comment;
  TemplateArgs  *Template;
  int            NumberOfSuperClasses;
  char         **SuperClasses;
  int            NumberOfItems;
  ItemInfo     **Items;
  int            NumberOfFunctions;
  FunctionInfo **Functions;
  int            NumberOfConstants;
  ValueInfo    **Constants;
  int            NumberOfEnums;
  EnumInfo     **Enums;
  int            NumberOfTypedefs;
  ValueInfo    **Typedefs;
  int            IsAbstract;
  int            HasDelete;
} ClassInfo;

/**
 * Namespace is for namespaces
 */
typedef struct _NamespaceInfo
{
  parse_item_t   ItemType;
  parse_access_t Access;
  char          *Name;  /* NULL for global namespace */
  char          *Comment;
  int            NumberOfItems;
  ItemInfo     **Items;
  int            NumberOfClasses;
  ClassInfo    **Classes;
  int            NumberOfFunctions;
  FunctionInfo **Functions;
  int            NumberOfConstants;
  ValueInfo    **Constants;
  int            NumberOfEnums;
  EnumInfo     **Enums;
  int            NumberOfTypedefs;
  ValueInfo    **Typedefs;
  int            NumberOfNamespaces;
  struct _NamespaceInfo **Namespaces;
} NamespaceInfo;

/**
 * FileInfo is for files
 */
typedef struct _FileInfo
{
  char *FileName;
  char *NameComment;
  char *Description;
  char *Caveats;
  char *SeeAlso;

  ClassInfo *MainClass;
  NamespaceInfo *Contents;
} FileInfo;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Add a hint about a class that will be parsed.  The only
 * acknowledged hint is "concrete", which if set to "true"
 * is a promise that the class is instantiable.
 */
void vtkParse_SetClassProperty(
  const char *classname, const char *property);

/**
 * Parse a header file and return a FileInfo struct
 */
FileInfo *vtkParse_ParseFile(
  const char *filename, FILE *ifile, FILE *errfile);

/**
 * Read a hints file and update the FileInfo
 */
int vtkParse_ReadHints(FileInfo *data, FILE *hfile, FILE *errfile);

/**
 * Free the FileInfo struct returned by vtkParse_ParseFile()
 */
void vtkParse_Free(FileInfo *data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
