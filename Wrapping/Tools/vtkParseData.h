/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParseData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright (c) 2010 David Gobbi.

  Contributed to the VisualizationToolkit by the author in May 2010
  under the terms of the Visualization Toolkit 2008 copyright.
-------------------------------------------------------------------------*/

/*
  Data structures used by vtkParse.
*/

#ifndef vtkParseData_h
#define vtkParseData_h

#include "vtkParseType.h"
#include "vtkParseString.h"

/* legacy */
#ifndef VTK_PARSE_LEGACY_REMOVE
#define MAX_ARGS 20
#endif

/**
 * Access flags
 */
typedef enum _parse_access_t
{
  VTK_ACCESS_PUBLIC    = 0,
  VTK_ACCESS_PROTECTED = 1,
  VTK_ACCESS_PRIVATE   = 2
} parse_access_t;

/**
 * Comment type constants
 */
typedef enum _parse_dox_t
{
  DOX_COMMAND_OTHER = 0,
  DOX_COMMAND_DEF,
  DOX_COMMAND_CATEGORY,
  DOX_COMMAND_INTERFACE,
  DOX_COMMAND_PROTOCOL,
  DOX_COMMAND_CLASS,
  DOX_COMMAND_ENUM,
  DOX_COMMAND_STRUCT,
  DOX_COMMAND_UNION,
  DOX_COMMAND_NAMESPACE,
  DOX_COMMAND_TYPEDEF,
  DOX_COMMAND_FN,
  DOX_COMMAND_PROPERTY,
  DOX_COMMAND_VAR,
  DOX_COMMAND_NAME,
  DOX_COMMAND_DEFGROUP,
  DOX_COMMAND_ADDTOGROUP,
  DOX_COMMAND_WEAKGROUP,
  DOX_COMMAND_EXAMPLE,
  DOX_COMMAND_FILE,
  DOX_COMMAND_DIR,
  DOX_COMMAND_MAINPAGE,
  DOX_COMMAND_PAGE,
  DOX_COMMAND_SUBPAGE,
  DOX_COMMAND_INTERNAL,
  DOX_COMMAND_PACKAGE,
  DOX_COMMAND_PRIVATESECTION,
  DOX_COMMAND_PROTECTEDSECTION,
  DOX_COMMAND_PUBLICSECTION,
} parse_dox_t;

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
 * ItemInfo just contains an index
 */
typedef struct _ItemInfo
{
  parse_item_t   Type;
  int            Index;
} ItemInfo;

/* forward declarations */
struct _ValueInfo;
struct _FunctionInfo;
struct _FileInfo;
typedef struct _ValueInfo ValueInfo;
typedef struct _FunctionInfo FunctionInfo;
typedef struct _FileInfo FileInfo;

/**
 * CommentInfo is for storing comments by category
 * This is for comments that cannot be immediately attached to an item,
 * for example class comments that come at the top of the header file
 * rather than immediately before the class that they document.
 */
typedef struct _CommentInfo
{
  parse_dox_t Type;
  const char *Comment;
  const char *Name;
} CommentInfo;

/**
 * TemplateInfo holds template definitions
 */
typedef struct _TemplateInfo
{
  int            NumberOfParameters;
  ValueInfo    **Parameters;
} TemplateInfo;

/**
 * ValueInfo is for typedefs, constants, variables,
 * function parameters, and return values
 *
 * Note that Dimensions is an array of char pointers, in
 * order to support dimensions that are sized according to
 * template parameter values or according to named constants.
 */
struct _ValueInfo
{
  parse_item_t   ItemType;
  parse_access_t Access;
  const char    *Name;
  const char    *Comment;
  const char    *Value;      /* for vars or default paramter values */
  unsigned int   Type;       /* as defined in vtkParseType.h   */
  const char    *Class;      /* classname for type */
  int            Count;      /* total number of values, if known */
  const char    *CountHint;  /* hint about how to get the count */
  int            NumberOfDimensions; /* dimensionality for arrays */
  const char   **Dimensions; /* dimensions for arrays */
  FunctionInfo  *Function;  /* for function pointer values */
  TemplateInfo  *Template;   /* template parameters, or NULL */
  int            IsStatic;   /* for class variables only */
  int            IsEnum;     /* for constants only */
  int            IsPack;     /* for pack expansions */
};

/**
 * FunctionInfo is for functions and methods
 */
struct _FunctionInfo
{
  parse_item_t   ItemType;
  parse_access_t Access;
  const char    *Name;
  const char    *Comment;
  const char    *Class;       /* class name for methods */
  const char    *Signature;   /* function signature as text */
  TemplateInfo  *Template;    /* template parameters, or NULL */
  int            NumberOfParameters;
  ValueInfo    **Parameters;
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
  int            IsDeleted;   /* methods only */
  int            IsFinal;     /* methods only */
  int            IsExplicit;  /* constructors only */
#ifndef VTK_PARSE_LEGACY_REMOVE
  int            NumberOfArguments;   /* legacy */
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
#endif
};

/**
 * UsingInfo is for using directives
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
 * ClassInfo is for classes, structs, unions, and namespaces
 */
typedef struct _ClassInfo
{
  parse_item_t   ItemType;
  parse_access_t Access;
  const char    *Name;
  const char    *Comment;
  TemplateInfo  *Template;
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
  struct _ClassInfo **Enums;
  int            NumberOfTypedefs;
  ValueInfo    **Typedefs;
  int            NumberOfUsings;
  UsingInfo    **Usings;
  int            NumberOfNamespaces;
  struct _ClassInfo **Namespaces;
  int            NumberOfComments;
  CommentInfo  **Comments;
  int            IsAbstract;
  int            IsFinal;
  int            HasDelete;
} ClassInfo;

/**
 * EnumInfo is for enums
 * For scoped enums, the constants are in the enum itself, but for
 * standard enums, the constants are at the same level as the enum.
 */
typedef struct _ClassInfo EnumInfo;

/**
 * Namespace is for namespaces
 */
typedef struct _ClassInfo NamespaceInfo;

/**
 * FileInfo is for header files
 */
struct _FileInfo
{
  const char *FileName;
  const char *NameComment;
  const char *Description;
  const char *Caveats;
  const char *SeeAlso;

  int NumberOfIncludes;
  struct _FileInfo **Includes;
  ClassInfo *MainClass;
  NamespaceInfo *Contents;
  StringCache *Strings;
};


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializer methods
 */
/*@{*/
void vtkParse_InitFile(FileInfo *file_info);
void vtkParse_InitNamespace(NamespaceInfo *namespace_info);
void vtkParse_InitClass(ClassInfo *cls);
void vtkParse_InitFunction(FunctionInfo *func);
void vtkParse_InitValue(ValueInfo *val);
void vtkParse_InitEnum(EnumInfo *item);
void vtkParse_InitUsing(UsingInfo *item);
void vtkParse_InitTemplate(TemplateInfo *arg);
void vtkParse_InitComment(CommentInfo *arg);
/*@}*/

/**
 * Copy methods
 *
 * Strings are not deep-copied, they are assumed to be persistent.
 */
/*@{*/
void vtkParse_CopyNamespace(NamespaceInfo *data, const NamespaceInfo *orig);
void vtkParse_CopyClass(ClassInfo *data, const ClassInfo *orig);
void vtkParse_CopyFunction(FunctionInfo *data, const FunctionInfo *orig);
void vtkParse_CopyValue(ValueInfo *data, const ValueInfo *orig);
void vtkParse_CopyEnum(EnumInfo *data, const EnumInfo *orig);
void vtkParse_CopyUsing(UsingInfo *data, const UsingInfo *orig);
void vtkParse_CopyTemplate(TemplateInfo *data, const TemplateInfo *orig);
void vtkParse_CopyComment(CommentInfo *data, const CommentInfo *orig);
/*@}*/

/**
 * Free methods
 *
 * Strings are not freed, they are assumed to be persistent.
 */
/*@{*/
void vtkParse_FreeFile(FileInfo *file_info);
void vtkParse_FreeNamespace(NamespaceInfo *namespace_info);
void vtkParse_FreeClass(ClassInfo *cls);
void vtkParse_FreeFunction(FunctionInfo *func);
void vtkParse_FreeValue(ValueInfo *val);
void vtkParse_FreeEnum(EnumInfo *item);
void vtkParse_FreeUsing(UsingInfo *item);
void vtkParse_FreeTemplate(TemplateInfo *arg);
void vtkParse_FreeComment(CommentInfo *arg);
/*@}*/


/**
 * Add a string to an array of strings, grow array as necessary.
 */
void vtkParse_AddStringToArray(
  const char ***valueArray, int *count, const char *value);

/**
 * Expand the Item array for classes and namespaces.
 */
void vtkParse_AddItemToArray(
  ItemInfo **valueArray, int *count, parse_item_t type, int idx);


/**
 * Add various items to the structs.
 */
/*@{*/
void vtkParse_AddIncludeToFile(FileInfo *info, FileInfo *item);
void vtkParse_AddClassToClass(ClassInfo *info, ClassInfo *item);
void vtkParse_AddFunctionToClass(ClassInfo *info, FunctionInfo *item);
void vtkParse_AddEnumToClass(ClassInfo *info, EnumInfo *item);
void vtkParse_AddConstantToClass(ClassInfo *info, ValueInfo *item);
void vtkParse_AddVariableToClass(ClassInfo *info, ValueInfo *item);
void vtkParse_AddTypedefToClass(ClassInfo *info, ValueInfo *item);
void vtkParse_AddUsingToClass(ClassInfo *info, UsingInfo *item);
void vtkParse_AddCommentToClass(ClassInfo *info, CommentInfo *item);
void vtkParse_AddNamespaceToNamespace(NamespaceInfo *info,NamespaceInfo *item);
void vtkParse_AddClassToNamespace(NamespaceInfo *info, ClassInfo *item);
void vtkParse_AddFunctionToNamespace(NamespaceInfo *info, FunctionInfo *item);
void vtkParse_AddEnumToNamespace(NamespaceInfo *info, EnumInfo *item);
void vtkParse_AddConstantToNamespace(NamespaceInfo *info, ValueInfo *item);
void vtkParse_AddVariableToNamespace(NamespaceInfo *info, ValueInfo *item);
void vtkParse_AddTypedefToNamespace(NamespaceInfo *info, ValueInfo *item);
void vtkParse_AddUsingToNamespace(NamespaceInfo *info, UsingInfo *item);
void vtkParse_AddCommentToNamespace(NamespaceInfo *info, CommentInfo *item);
void vtkParse_AddParameterToFunction(FunctionInfo *info, ValueInfo *item);
void vtkParse_AddParameterToTemplate(TemplateInfo *info, ValueInfo *item);
/*@}*/

/**
 * Add default constructors to a class if they do not already exist
 */
void vtkParse_AddDefaultConstructors(ClassInfo *data, StringCache *cache);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
