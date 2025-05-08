// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2010 David Gobbi
// SPDX-License-Identifier: BSD-3-Clause
/*
  Data structures used by vtkParse.
*/

#ifndef vtkParseData_h
#define vtkParseData_h

#include "vtkParseAttributes.h"
#include "vtkParseString.h"
#include "vtkParseType.h"
#include "vtkWrappingToolsModule.h"

/* legacy */
#ifndef VTK_PARSE_LEGACY_REMOVE
#define MAX_ARGS 20
#endif

/**
 * Access flags
 */
typedef enum parse_access_t_
{
  VTK_ACCESS_PUBLIC = 0,
  VTK_ACCESS_PROTECTED = 1,
  VTK_ACCESS_PRIVATE = 2
} parse_access_t;

/**
 * Comment type constants
 */
typedef enum parse_dox_t_
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
typedef enum parse_item_t_
{
  VTK_NAMESPACE_INFO = 1,
  VTK_CLASS_INFO = 2,
  VTK_STRUCT_INFO = 3,
  VTK_UNION_INFO = 4,
  VTK_ENUM_INFO = 5,
  VTK_FUNCTION_INFO = 6,
  VTK_VARIABLE_INFO = 7,
  VTK_CONSTANT_INFO = 8,
  VTK_TYPEDEF_INFO = 9,
  VTK_USING_INFO = 10
} parse_item_t;

/**
 * Marshalling code type constants
 */
typedef enum parse_marshal_t
{
  VTK_MARSHAL_NONE,
  VTK_MARSHAL_AUTO_MODE,
  VTK_MARSHAL_MANUAL_MODE
} parse_marshal_t;

/**
 * ItemInfo just contains an index
 */
typedef struct ItemInfo_
{
  parse_item_t Type;
  int Index;
} ItemInfo;

/* forward declarations */
struct ValueInfo_;
struct FunctionInfo_;
struct FileInfo_;
typedef struct ValueInfo_ ValueInfo;
typedef struct FunctionInfo_ FunctionInfo;
typedef struct FileInfo_ FileInfo;

/**
 * CommentInfo is for storing comments by category
 * This is for comments that cannot be immediately attached to an item,
 * for example class comments that come at the top of the header file
 * rather than immediately before the class that they document.
 */
typedef struct CommentInfo_
{
  parse_dox_t Type;
  const char* Comment;
  const char* Name;
} CommentInfo;

/**
 * TemplateInfo holds template definitions
 */
typedef struct TemplateInfo_
{
  int NumberOfParameters;
  ValueInfo** Parameters;
} TemplateInfo;

/**
 * ValueInfo is for typedefs, constants, variables,
 * function parameters, and return values
 *
 * Note that Dimensions is an array of char pointers, in
 * order to support dimensions that are sized according to
 * template parameter values or according to named constants.
 */
struct ValueInfo_
{
  parse_item_t ItemType;
  parse_access_t Access;
  const char* Name;
  const char* Comment;
  const char* Value;       /* for vars or default parameters values */
  unsigned int Attributes; /* as defined in vtkParseAttributes.h */
  unsigned int Type;       /* as defined in vtkParseType.h   */
  const char* Class;       /* classname for type */
  int Count;               /* total number of values, if known */
  const char* CountHint;   /* hint about how to get the count */
  int NumberOfDimensions;  /* dimensionality for arrays */
  const char** Dimensions; /* dimensions for arrays */
  FunctionInfo* Function;  /* for function pointer values */
  TemplateInfo* Template;  /* template parameters, or NULL */
  int IsStatic;            /* for class variables only */
  int IsEnum;              /* for constants only */
  int IsPack;              /* for pack expansions */
};

/**
 * FunctionInfo is for functions and methods
 */
struct FunctionInfo_
{
  parse_item_t ItemType;
  parse_access_t Access;
  const char* Name;
  const char* Comment;
  const char* Class;      /* class name for methods */
  const char* Signature;  /* function signature as text */
  TemplateInfo* Template; /* template parameters, or NULL */
  int NumberOfParameters;
  ValueInfo** Parameters;
  ValueInfo* ReturnValue;          /* NULL for constructors and destructors */
  const char* MarshalPropertyName; /* optionally marshalled for the given property name */
  const char* MarshalExcludeReason;
  int NumberOfPreconds;
  const char** Preconds;         /* preconditions */
  const char* Macro;             /* the macro that defined this function */
  const char* SizeHint;          /* hint the size e.g. for operator[] */
  const char* DeprecatedReason;  /* reason for deprecation, or NULL */
  const char* DeprecatedVersion; /* version of deprecation, or NULL */
  int IsOperator;
  int IsVariadic;
  int IsExcluded;        /* marked as excluded from wrapping */
  int IsPropExcluded;    /* exclude from consideration as a property getset method */
  int IsDeprecated;      /* method or function has been deprecated */
  int IsStatic;          /* methods only */
  int IsVirtual;         /* methods only */
  int IsPureVirtual;     /* methods only */
  int IsConst;           /* methods only */
  int IsDeleted;         /* methods only */
  int IsFinal;           /* methods only */
  int IsOverride;        /* methods only */
  int IsMarshalExcluded; /* methods only */
  int IsExplicit;        /* constructors only */
#ifndef VTK_PARSE_LEGACY_REMOVE
  int NumberOfArguments;            /* legacy */
  unsigned int ArgTypes[MAX_ARGS];  /* legacy */
  const char* ArgClasses[MAX_ARGS]; /* legacy */
  int ArgCounts[MAX_ARGS];          /* legacy */
  unsigned int ReturnType;          /* legacy */
  const char* ReturnClass;          /* legacy */
  int HaveHint;                     /* legacy */
  int HintSize;                     /* legacy */
  int ArrayFailure;                 /* legacy */
  int IsPublic;                     /* legacy */
  int IsProtected;                  /* legacy */
  int IsLegacy;                     /* legacy */
#endif
};

/**
 * UsingInfo is for using directives
 */
typedef struct UsingInfo_
{
  parse_item_t ItemType;
  parse_access_t Access;
  const char* Name; /* null for using whole namespace */
  const char* Comment;
  const char* Scope; /* the namespace or class */
} UsingInfo;

/**
 * ClassInfo is for classes, structs, unions, and namespaces
 */
typedef struct ClassInfo_
{
  const char* Name;
  const char* Comment;
  TemplateInfo* Template;
  const char** SuperClasses;
  ItemInfo* Items;
  struct ClassInfo_** Classes;
  FunctionInfo** Functions;
  ValueInfo** Constants;
  ValueInfo** Variables;
  struct ClassInfo_** Enums;
  ValueInfo** Typedefs;
  UsingInfo** Usings;
  struct ClassInfo_** Namespaces;
  CommentInfo** Comments;
  const char* DeprecatedReason;
  const char* DeprecatedVersion;
  parse_item_t ItemType;
  parse_access_t Access;
  parse_marshal_t MarshalType;
  int NumberOfSuperClasses;
  int NumberOfItems;
  int NumberOfClasses;
  int NumberOfFunctions;
  int NumberOfConstants;
  int NumberOfVariables;
  int NumberOfEnums;
  int NumberOfTypedefs;
  int NumberOfUsings;
  int NumberOfNamespaces;
  int NumberOfComments;
  int IsAbstract;
  int IsFinal;
  int HasDelete;
  int IsExcluded;
  int IsDeprecated;
} ClassInfo;

/**
 * EnumInfo is for enums
 * For scoped enums, the constants are in the enum itself, but for
 * standard enums, the constants are at the same level as the enum.
 */
typedef struct ClassInfo_ EnumInfo;

/**
 * Namespace is for namespaces
 */
typedef struct ClassInfo_ NamespaceInfo;

/**
 * FileInfo is for header files
 */
struct FileInfo_
{
  const char* FileName;
  const char* NameComment;
  const char* Description;
  const char* Caveats;
  const char* SeeAlso;

  int NumberOfIncludes;
  struct FileInfo_** Includes;
  ClassInfo* MainClass;
  NamespaceInfo* Contents;
  StringCache* Strings;
};

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * Initializer methods
   */
  /*@{*/
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_InitFile(FileInfo* file_info);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_InitNamespace(NamespaceInfo* namespace_info);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_InitClass(ClassInfo* cls);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_InitFunction(FunctionInfo* func);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_InitValue(ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_InitEnum(EnumInfo* item);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_InitUsing(UsingInfo* item);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_InitTemplate(TemplateInfo* arg);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_InitComment(CommentInfo* arg);
  /*@}*/

  /**
   * Copy methods
   *
   * Strings are not deep-copied, they are assumed to be persistent.
   */
  /*@{*/
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_CopyNamespace(NamespaceInfo* data, const NamespaceInfo* orig);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_CopyClass(ClassInfo* data, const ClassInfo* orig);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_CopyFunction(FunctionInfo* data, const FunctionInfo* orig);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_CopyValue(ValueInfo* data, const ValueInfo* orig);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_CopyEnum(EnumInfo* data, const EnumInfo* orig);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_CopyUsing(UsingInfo* data, const UsingInfo* orig);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_CopyTemplate(TemplateInfo* data, const TemplateInfo* orig);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_CopyComment(CommentInfo* data, const CommentInfo* orig);
  /*@}*/

  /**
   * Free methods
   *
   * Strings are not freed, they are assumed to be persistent.
   */
  /*@{*/
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_FreeFile(FileInfo* file_info);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_FreeNamespace(NamespaceInfo* namespace_info);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_FreeClass(ClassInfo* cls);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_FreeFunction(FunctionInfo* func);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_FreeValue(ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_FreeEnum(EnumInfo* item);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_FreeUsing(UsingInfo* item);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_FreeTemplate(TemplateInfo* arg);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_FreeComment(CommentInfo* arg);
  /*@}*/

  /**
   * Add a string to an array of strings, grow array as necessary.
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_AddStringToArray(const char*** valueArray, int* count, const char* value);

  /**
   * Expand the Item array for classes and namespaces.
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_AddItemToArray(ItemInfo** valueArray, int* count, parse_item_t type, int idx);

  /**
   * Add various items to the structs.
   */
  /*@{*/
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_AddIncludeToFile(FileInfo* info, FileInfo* item);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_AddClassToClass(ClassInfo* info, ClassInfo* item);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_AddFunctionToClass(ClassInfo* info, FunctionInfo* item);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_AddEnumToClass(ClassInfo* info, EnumInfo* item);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_AddConstantToClass(ClassInfo* info, ValueInfo* item);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_AddVariableToClass(ClassInfo* info, ValueInfo* item);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_AddTypedefToClass(ClassInfo* info, ValueInfo* item);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_AddUsingToClass(ClassInfo* info, UsingInfo* item);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_AddCommentToClass(ClassInfo* info, CommentInfo* item);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_AddNamespaceToNamespace(NamespaceInfo* info, NamespaceInfo* item);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_AddClassToNamespace(NamespaceInfo* info, ClassInfo* item);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_AddFunctionToNamespace(NamespaceInfo* info, FunctionInfo* item);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_AddEnumToNamespace(NamespaceInfo* info, EnumInfo* item);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_AddConstantToNamespace(NamespaceInfo* info, ValueInfo* item);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_AddVariableToNamespace(NamespaceInfo* info, ValueInfo* item);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_AddTypedefToNamespace(NamespaceInfo* info, ValueInfo* item);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_AddUsingToNamespace(NamespaceInfo* info, UsingInfo* item);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_AddCommentToNamespace(NamespaceInfo* info, CommentInfo* item);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_AddParameterToFunction(FunctionInfo* info, ValueInfo* item);
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_AddParameterToTemplate(TemplateInfo* info, ValueInfo* item);
  /*@}*/

  /**
   * Add default constructors to a class if they do not already exist
   */
  void vtkParse_AddDefaultConstructors(ClassInfo* cls, StringCache* cache);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
/* VTK-HeaderTest-Exclude: vtkParseData.h */
