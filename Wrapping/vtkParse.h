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
  VTK_TYPEDEF_INFO   = 9,
  VTK_USING_INFO     = 10
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
 * TemplateArg holds one template arg
 */
typedef struct _TemplateArg
{
  unsigned int  Type;  /* is zero for "typename", "class", "template" */
  const char   *Class; /* class name for type */
  const char   *Name;  /* name of template arg */
  const char   *Value; /* default value */
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
  const char    *Name;
  const char    *Comment;
  const char    *Value;      /* for vars or default arg values */
  unsigned int   Type;       /* as defined in vtkParseType.h   */
  const char    *Class;      /* classname for type */
  int            Count;      /* total number of values, if known */
  const char    *CountHint;  /* hint about how to get the count */
  int            NumberOfDimensions; /* dimensionality for arrays */
  const char   **Dimensions; /* dimensions for arrays */
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
  const char    *Name;
  const char    *Comment;
  const char    *Class;       /* class name for methods */
  const char    *Signature;   /* function signature as text */
  TemplateArgs  *Template;    /* template args, or NULL */
  int            NumberOfArguments;
  ValueInfo    **Arguments;
  ValueInfo     *ReturnValue; /* NULL for constructors and destructors */
  const char    *Macro;       /* the macro that defined this function */
  const char    *SizeHint;    /* hint the size e.g. for operator[] */
  int            IsOperator;
  int            IsVariadic;
  int            IsLegacy;    /* marked as a legacy method or function */
  int            IsStatic;    /* methods only */
  int            IsVirtual;   /* methods only */
  int            IsPureVirtual; /* methods only */
  int            IsConst;     /* methods only */
  int            IsExplicit;  /* constructors only */
  unsigned int   ArgTypes[MAX_ARGS];  /* legacy */
  const char    *ArgClasses[MAX_ARGS];/* legacy */
  int            ArgCounts[MAX_ARGS]; /* legacy */
  unsigned int   ReturnType;  /* legacy */
  const char    *ReturnClass; /* legacy */
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
  const char    *Name;
  const char    *Comment;
} EnumInfo;

/**
 * UsingInfo is for using directives (not implemented yet)
 */
typedef struct _UsingInfo
{
  parse_item_t   ItemType;
  parse_access_t Access;
  const char    *Name;     /* null for using whole namespace */
  const char    *Comment;
  const char    *Scope;    /* the namespace or class */
} UsingInfo;

/**
 * ItemInfo just contains an index
 */
typedef struct _ItemInfo
{
  parse_item_t   Type;
  int            Index;
} ItemInfo;

/**
 * ClassInfo is for classes, structs, and unions
 */
typedef struct _ClassInfo
{
  parse_item_t   ItemType;
  parse_access_t Access;
  const char    *Name;
  const char    *Comment;
  TemplateArgs  *Template;
  int            NumberOfSuperClasses;
  const char   **SuperClasses;
  int            NumberOfItems;
  ItemInfo      *Items;
  int            NumberOfClasses;
  struct _ClassInfo **Classes;
  int            NumberOfFunctions;
  FunctionInfo **Functions;
  int            NumberOfConstants;
  ValueInfo    **Constants;
  int            NumberOfVariables;
  ValueInfo    **Variables;
  int            NumberOfEnums;
  EnumInfo     **Enums;
  int            NumberOfTypedefs;
  ValueInfo    **Typedefs;
  int            NumberOfUsings;
  UsingInfo    **Usings;
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
  const char    *Name;  /* NULL for global namespace */
  const char    *Comment;
  int            NumberOfItems;
  ItemInfo      *Items;
  int            NumberOfClasses;
  ClassInfo    **Classes;
  int            NumberOfFunctions;
  FunctionInfo **Functions;
  int            NumberOfConstants;
  ValueInfo    **Constants;
  int            NumberOfVariables;
  ValueInfo    **Variables;
  int            NumberOfEnums;
  EnumInfo     **Enums;
  int            NumberOfTypedefs;
  ValueInfo    **Typedefs;
  int            NumberOfUsings;
  UsingInfo    **Usings;
  int            NumberOfNamespaces;
  struct _NamespaceInfo **Namespaces;
} NamespaceInfo;

/**
 * FileInfo is for files
 */
typedef struct _FileInfo
{
  const char *FileName;
  const char *NameComment;
  const char *Description;
  const char *Caveats;
  const char *SeeAlso;

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
 * Define a preprocessor macro. Function macros are not supported.
 */
void vtkParse_DefineMacro(const char *name, const char *definition);

/**
 * Undefine a preprocessor macro.
 */
void vtkParse_UndefineMacro(const char *name);

/**
 * Add an include directory, for use with the "-I" option.
 */
void vtkParse_IncludeDirectory(const char *dirname);

/**
 * Return the full path to a header file.
 */
const char *vtkParse_FindIncludeFile(const char *filename);

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
