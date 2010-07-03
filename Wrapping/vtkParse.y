/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParse.y

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
%{

/*

This file must be translated to C and modified to build everywhere.

Run yacc like this:

  yacc -b vtkParse vtkParse.y

Modify vtkParse.tab.c:
  - convert TABs to spaces (eight per tab)
  - remove spaces from ends of lines, s/ *$//g
  - remove the "goto yyerrlab1;" that appears right before yyerrlab1:
  - remove the #defined constants that appear right after the anonymous_enums

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define yyerror(a) fprintf(stderr,"%s\n",a)
#define yywrap() 1

/* Make sure yacc-generated code knows we have included stdlib.h.  */
#ifndef _STDLIB_H
# define _STDLIB_H
#endif
#define YYINCLUDED_STDLIB_H

/* Borland and MSVC do not define __STDC__ properly. */
#if !defined(__STDC__)
# if (defined(_MSC_VER) && _MSC_VER >= 1200) || defined(__BORLANDC__)
#  define __STDC__ 1
# endif
#endif

/* Disable warnings in generated code. */
#if defined(_MSC_VER)
# pragma warning (disable: 4127) /* conditional expression is constant */
# pragma warning (disable: 4244) /* conversion to smaller integer type */
#endif
#if defined(__BORLANDC__)
# pragma warn -8004 /* assigned a value that is never used */
# pragma warn -8008 /* conditional is always true */
# pragma warn -8066 /* unreachable code */
#endif

/* Map from the type anonymous_enumeration in vtkType.h to the VTK wrapping type
   system number for the type. */

#include "vtkParse.h"
#include "vtkParseInternal.h"
#include "vtkType.h"

static int vtkParseTypeMap[] =
  {
  VTK_PARSE_VOID,               /* VTK_VOID                0 */
  0,                            /* VTK_BIT                 1 */
  VTK_PARSE_CHAR,               /* VTK_CHAR                2 */
  VTK_PARSE_UNSIGNED_CHAR,      /* VTK_UNSIGNED_CHAR       3 */
  VTK_PARSE_SHORT,              /* VTK_SHORT               4 */
  VTK_PARSE_UNSIGNED_SHORT,     /* VTK_UNSIGNED_SHORT      5 */
  VTK_PARSE_INT,                /* VTK_INT                 6 */
  VTK_PARSE_UNSIGNED_INT,       /* VTK_UNSIGNED_INT        7 */
  VTK_PARSE_LONG,               /* VTK_LONG                8 */
  VTK_PARSE_UNSIGNED_LONG,      /* VTK_UNSIGNED_LONG       9 */
  VTK_PARSE_FLOAT,              /* VTK_FLOAT              10 */
  VTK_PARSE_DOUBLE,             /* VTK_DOUBLE             11 */
  VTK_PARSE_ID_TYPE,            /* VTK_ID_TYPE            12 */
  VTK_PARSE_STRING,             /* VTK_STRING             13 */
  0,                            /* VTK_OPAQUE             14 */
  VTK_PARSE_SIGNED_CHAR,        /* VTK_SIGNED_CHAR        15 */
  VTK_PARSE_LONG_LONG,          /* VTK_LONG_LONG          16 */
  VTK_PARSE_UNSIGNED_LONG_LONG, /* VTK_UNSIGNED_LONG_LONG 17 */
  VTK_PARSE___INT64,            /* VTK___INT64            18 */
  VTK_PARSE_UNSIGNED___INT64,   /* VTK_UNSIGNED___INT64   19 */
  0,                            /* VTK_VARIANT            20 */
  0,                            /* VTK_OBJECT             21 */
  VTK_PARSE_UNICODE_STRING      /* VTK_UNICODE_STRING     22 */
  };

/* Define some constants to simplify references to the table lookup in
   the type_primitive production rule code.  */
#define VTK_PARSE_INT8 vtkParseTypeMap[VTK_TYPE_INT8]
#define VTK_PARSE_UINT8 vtkParseTypeMap[VTK_TYPE_UINT8]
#define VTK_PARSE_INT16 vtkParseTypeMap[VTK_TYPE_INT16]
#define VTK_PARSE_UINT16 vtkParseTypeMap[VTK_TYPE_UINT16]
#define VTK_PARSE_INT32 vtkParseTypeMap[VTK_TYPE_INT32]
#define VTK_PARSE_UINT32 vtkParseTypeMap[VTK_TYPE_UINT32]
#define VTK_PARSE_INT64 vtkParseTypeMap[VTK_TYPE_INT64]
#define VTK_PARSE_UINT64 vtkParseTypeMap[VTK_TYPE_UINT64]
#define VTK_PARSE_FLOAT32 vtkParseTypeMap[VTK_TYPE_FLOAT32]
#define VTK_PARSE_FLOAT64 vtkParseTypeMap[VTK_TYPE_FLOAT64]

#define vtkParseDebug(s1, s2) \
  if ( parseDebug ) { fprintf(stderr, "   %s %s\n", s1, s2); }

/* the tokenizer */
int yylex(void);

/* global variables */
FileInfo data;

NamespaceInfo *currentNamespace = NULL;
ClassInfo *currentClass = NULL;
FunctionInfo *currentFunction = NULL;
TemplateArgs *currentTemplate = NULL;

char *currentEnumName = 0;
char *currentEnumValue = 0;

int parseDebug;
char temps[2048];
parse_access_t access_level = VTK_ACCESS_PUBLIC;
int  is_concrete;
int  HaveComment;
char CommentText[50000];
int CommentState;

/* helper functions */
void start_class(const char *classname, int is_struct);
void reject_class(const char *classname, int is_struct);
void end_class();
void output_function(void);
void reject_function(void);
void set_return(FunctionInfo *func, int type,
                const char *typeclass, int count);
void add_argument(FunctionInfo *func, int type,
                  const char *classname, int count);
void start_enum(const char *enumname);
void add_enum(const char *name, const char *value);
void end_enum();
void add_constant(const char *name, const char *value,
                  int type, const char *typeclass, int global);
const char *add_const_scope(const char *name);
void prepend_scope(char *cp, const char *arg);
int add_indirection(int tval, int ptr);
void handle_complex_type(ValueInfo *val, int datatype,
                         int extra, const char *funcSig);
void handle_function_type(ValueInfo *arg, const char *name,
                          const char *funcSig);

void outputSetVectorMacro(
  const char *var, int argType, const char *typeText, int n);
void outputGetVectorMacro(
  const char *var, int argType, const char *typeText, int n);

/* duplicate a string */
char *vtkstrdup(const char *in)
{
  char *res = NULL;
  if (in)
    {
    res = (char *)malloc(strlen(in)+1);
    strcpy(res,in);
    }
  return res;
}

/*----------------------------------------------------------------
 * Namespaces
 *
 * operates on: currentNamespace
 */

/* "private" variables */
NamespaceInfo *namespaceStack[10];
int namespaceDepth = 0;

/* enter a namespace */
void pushNamespace(const char *name)
{
  int i;
  NamespaceInfo *oldNamespace = currentNamespace;

  for (i = 0; i < oldNamespace->NumberOfNamespaces; i++)
    {
    /* see if the namespace already exists */
    if (strcmp(name, oldNamespace->Namespaces[i]->Name) == 0)
      {
      currentNamespace = oldNamespace->Namespaces[i];
      }
    }

  /* create a new namespace */
  if (i == oldNamespace->NumberOfNamespaces)
    {
    currentNamespace = (NamespaceInfo *)malloc(sizeof(NamespaceInfo));
    vtkParse_InitNamespace(currentNamespace);
    currentNamespace->Name = vtkstrdup(name);
    vtkParse_AddItemMacro(oldNamespace, Namespaces, currentNamespace);
    }

  namespaceStack[namespaceDepth++] = oldNamespace;
}

/* leave the namespace */
void popNamespace()
{
  currentNamespace = namespaceStack[--namespaceDepth];
}

/*----------------------------------------------------------------
 * Templates
 *
 * operates on: currentTemplate
 */

/* "private" variables */
TemplateArgs *templateStack[10];
int templateDepth = 0;

/* begin a template */
void startTemplate()
{
  currentTemplate = (TemplateArgs *)malloc(sizeof(TemplateArgs));
  vtkParse_InitTemplateArgs(currentTemplate);
}

/* clear a template, if set */
void clearTemplate()
{
  if (currentTemplate)
    {
    free(currentTemplate);
    }
  currentTemplate = NULL;
}

/* push the template onto the stack, and start a new one */
void pushTemplate()
{
  templateStack[templateDepth++] = currentTemplate;
  startTemplate();
}

/* pop a template off the stack */
void popTemplate()
{
  currentTemplate = templateStack[--templateDepth];
}

/*----------------------------------------------------------------
 * Function signatures
 *
 * operates on: currentFunction
 */

/* "private" variables */
int sigClosed;
size_t sigMark[10];
size_t sigMarkDepth = 0;
unsigned int sigAllocatedLength;

/* start a new signature */
void startSig()
{
  if (currentFunction->Signature)
    {
    free(currentFunction->Signature);
    currentFunction->Signature = NULL;
    }

  sigAllocatedLength = 0;
  sigClosed = 0;
  sigMarkDepth = 0;
  sigMark[0] = 0;
}

/* reallocate Signature if "arg" cannot be appended */
void checkSigSize(const char *arg)
{
  if (strlen(currentFunction->Signature) + strlen(arg) + 5 >
      sigAllocatedLength)
    {
    currentFunction->Signature = (char *)
      realloc(currentFunction->Signature, sigAllocatedLength*2);
    sigAllocatedLength = sigAllocatedLength*2;
    }
}

/* close the signature, i.e. allow no more additions to it */
void closeSig()
{
  sigClosed = 1;
}

/* re-open the signature */
void openSig()
{
  sigClosed = 0;
}

/* insert text at the beginning of the signature */
void preSig(const char *arg)
{
  if (!currentFunction->Signature)
    {
    currentFunction->Signature = (char*)malloc(2048);
    sigAllocatedLength = 2048;
    strcpy(currentFunction->Signature, arg);
    }
  else if (!sigClosed)
    {
    size_t m, n;
    char *cp;
    checkSigSize(arg);
    cp = currentFunction->Signature;
    m = strlen(cp);
    n = strlen(arg);
    memmove(&cp[n], cp, m+1);
    strncpy(cp, arg, n);
    }
}

/* append text to the end of the signature */
void postSig(const char *arg)
{
  if (!currentFunction->Signature)
    {
    currentFunction->Signature = (char*)malloc(2048);
    sigAllocatedLength = 2048;
    strcpy(currentFunction->Signature, arg);
    }
  else if (!sigClosed)
    {
    size_t m, n;
    char *cp;
    checkSigSize(arg);
    cp = currentFunction->Signature;
    m = strlen(cp);
    n = strlen(arg);
    strncpy(&cp[m], arg, n+1);
    }
}

/* prepend a scope:: to the ID at the end of the signature */
void preScopeSig(const char *arg)
{
  if (!currentFunction->Signature)
    {
    currentFunction->Signature = (char*)malloc(2048);
    sigAllocatedLength = 2048;
    strcpy(currentFunction->Signature, arg);
    }
  else if (!sigClosed)
    {
    checkSigSize(arg);
    prepend_scope(currentFunction->Signature, arg);
    }
}

/* set a mark in the signature for later operations */
void markSig()
{
  sigMark[sigMarkDepth] = 0;
  if (currentFunction->Signature)
    {
    sigMark[sigMarkDepth] = strlen(currentFunction->Signature);
    }
  sigMarkDepth++;
}

/* get the contents of the sig from the mark, and clear the mark */
const char *copySig()
{
  const char *cp = NULL;
  if (sigMarkDepth > 0)
    {
    sigMarkDepth--;
    }
  if (currentFunction->Signature)
    {
    cp = &currentFunction->Signature[sigMark[sigMarkDepth]];
    }
  return cp;
}

/* swap the signature text using the mark as the radix */
void swapSig()
{
  if (sigMarkDepth > 0)
    {
    sigMarkDepth--;
    }
  if (currentFunction->Signature && sigMark[sigMarkDepth] > 0)
    {
    size_t i, m, n, nn;
    char c;
    char *cp;
    cp = currentFunction->Signature;
    n = strlen(cp);
    m = sigMark[sigMarkDepth];
    nn = m/2;
    for (i = 0; i < nn; i++)
      {
      c = cp[i]; cp[i] = cp[m-i-1]; cp[m-i-1] = c;
      }
    nn = (n-m)/2;
    for (i = 0; i < nn; i++)
      {
      c = cp[i+m]; cp[i+m] = cp[n-i-1]; cp[n-i-1] = c;
      }
    nn = n/2;
    for (i = 0; i < nn; i++)
      {
      c = cp[i]; cp[i] = cp[n-i-1]; cp[n-i-1] = c;
      }
    }
}

/* chop the last space from the signature */
void chopSig(void)
{
  if (currentFunction->Signature)
    {
    size_t n = strlen(currentFunction->Signature);
    if (n > 0 && currentFunction->Signature[n-1] == ' ')
      {
      currentFunction->Signature[n-1] = '\0';
      }
    }
}

/* mark this signature as legacy */
void legacySig(void)
{
  currentFunction->IsLegacy = 1;
}

/*----------------------------------------------------------------
 * Storage type for vars and functions
 */

/* "private" variables */
int storageType = 0;

/* save the storage type */
void setStorageType(int val)
{
  storageType = val;
}

/* modify the indirection (pointers, refs) in the storage type */
void setStorageTypeIndirection(int ind)
{
  storageType = (storageType & ~VTK_PARSE_INDIRECT);
  ind = (ind & VTK_PARSE_INDIRECT);
  storageType = (storageType | ind);
}

/* retrieve the storage type */
int getStorageType()
{
  return storageType;
}

/*----------------------------------------------------------------
 * Array information
 */

/* "private" variables */
int numberOfDimensions = 0;
char **arrayDimensions;

/* clear the array counter */
void clearArray(void)
{
  numberOfDimensions = 0;
}

/* add another dimension */
void pushArraySize(const char *size)
{
  vtkParse_AddPointerToArray(&arrayDimensions, &numberOfDimensions,
                             vtkstrdup(size));
}

/* add another dimension to the front */
void pushArrayFront(const char *size)
{
  int i;

  vtkParse_AddPointerToArray(&arrayDimensions, &numberOfDimensions, 0);

  for (i = numberOfDimensions-1; i > 0; i--)
    {
    arrayDimensions[i] = arrayDimensions[i-1];
    }

  arrayDimensions[0] = vtkstrdup(size);
}

/* get the number of dimensions */
int getArrayNDims()
{
  return numberOfDimensions;
}

/* get the whole array */
char **getArray()
{
  return arrayDimensions;
}

/*----------------------------------------------------------------
 * Variables and Arguments
 */

/* "private" variables */
char *currentVarName = 0;
char *currentVarValue = 0;
char *currentId = 0;

/* clear the var Id */
void clearVarName(void)
{
  currentVarName = NULL;
}

/* set the var Id */
void setVarName(const char *text)
{
  static char static_text[2048];
  currentVarName = static_text;
  strcpy(static_text, text);
}

/* return the var id */
const char *getVarName()
{
  return currentVarName;
}

/* variable value -------------- */

/* clear the var value */
void clearVarValue(void)
{
  currentVarValue = NULL;
}

/* set the var value */
void setVarValue(const char *text)
{
  static char static_text[2048];
  currentVarValue = static_text;
  strcpy(static_text, text);
}

/* return the var value */
const char *getVarValue()
{
  return currentVarValue;
}

/* variable type -------------- */

/* clear the current Id */
void clearTypeId(void)
{
  currentId = NULL;
}

/* set the current Id, it is sticky until cleared */
void setTypeId(const char *text)
{
  static char static_text[2048];
  if (currentId == NULL)
    {
    currentId = static_text;
    strcpy(static_text, text);
    }
}

/* set the signature and type together */
void typeSig(const char *text)
{
  size_t n;

  postSig(text);
  postSig(" ");

  if (currentId == 0)
    {
    setTypeId(text);
    }
  else if ((currentId[0] == 'u' && strcmp(currentId, "unsigned") == 0) ||
           (currentId[0] == 's' && strcmp(currentId, "signed") == 0))
    {
    n = strlen(currentId);
    currentId[n] = ' ';
    strcpy(&currentId[n+1], text);
    }
}

/* return the current Id */
const char *getTypeId()
{
  return currentId;
}

/*----------------------------------------------------------------
 * Function stack
 *
 * operates on: currentFunction
 */

/* "private" variables */
FunctionInfo *functionStack[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
char *functionVarNameStack[10];
char *functionTypeIdStack[10];
int functionDepth = 0;

void pushFunction()
{
  functionStack[functionDepth] = currentFunction;
  currentFunction = (FunctionInfo *)malloc(sizeof(FunctionInfo));
  vtkParse_InitFunction(currentFunction);
  if (functionStack[functionDepth])
    {
    currentFunction->Signature = functionStack[functionDepth]->Signature;
    }
  else
    {
    startSig();
    }
  functionVarNameStack[functionDepth] = vtkstrdup(getVarName());
  functionTypeIdStack[functionDepth] = vtkstrdup(getTypeId());
  clearVarName();
  clearTypeId();
  functionDepth++;
  functionStack[functionDepth] = 0;
}

void popFunction()
{
  FunctionInfo *newFunction = currentFunction;

  --functionDepth;
  currentFunction = functionStack[functionDepth];
  if (currentFunction)
    {
    currentFunction->Signature = newFunction->Signature;
    }
  newFunction->Signature = NULL;
  clearVarName();
  if (functionVarNameStack[functionDepth])
    {
    setVarName(functionVarNameStack[functionDepth]);
    free(functionVarNameStack[functionDepth]);
    }
  clearTypeId();
  if (functionTypeIdStack[functionDepth])
    {
    setTypeId(functionTypeIdStack[functionDepth]);
    free(functionTypeIdStack[functionDepth]);
    }

  functionStack[functionDepth+1] = newFunction;
}

FunctionInfo *getFunction()
{
  return functionStack[functionDepth+1];
}

/*----------------------------------------------------------------
 * Utility methods
 */

/* prepend a scope:: to a name */
void prepend_scope(char *cp, const char *arg)
{
  size_t i, m, n, depth;

  m = strlen(cp);
  n = strlen(arg);
  i = m;
  while (i > 0 &&
         ((cp[i-1] >= 'a' && cp[i-1] <= 'z') ||
          (cp[i-1] >= 'A' && cp[i-1] <= 'Z') ||
          (cp[i-1] >= '0' && cp[i-1] <= '9') ||
          cp[i-1] == '_' || cp[i-1] == ':' ||
          cp[i-1] == '>'))
    {
    i--;
    if (cp[i] == '>')
      {
      depth = 1;
      while (i > 0)
        {
        i--;
        if (cp[i] == '<')
          {
          if (--depth == 0)
            {
            break;
            }
          }
        if (cp[i] == '>')
          {
          depth++;
          }
        }
      }
    }

  memmove(&cp[i+n+2], &cp[i], m+1);
  strncpy(&cp[i], arg, n);
  strncpy(&cp[i+n], "::", 2);
}

/* expand a type by including pointers from another */
int add_indirection(int type1, int type2)
{
  int ptr1 = (type1 & VTK_PARSE_POINTER_MASK);
  int ptr2 = (type2 & VTK_PARSE_POINTER_MASK);
  int reverse = 0;
  int result;

  /* one of type1 or type2 will only have VTK_PARSE_INDIRECT, but
   * we don't know which one. */
  result = ((type1 & ~VTK_PARSE_POINTER_MASK) |
            (type2 & ~VTK_PARSE_POINTER_MASK));

  while (ptr2)
    {
    reverse = ((reverse << 2) | (ptr2 & VTK_PARSE_POINTER_LOWMASK));
    ptr2 = ((ptr2 >> 2) & VTK_PARSE_POINTER_MASK);
    }

  while (reverse)
   {
   ptr1 = ((ptr1 << 2) | (reverse & VTK_PARSE_POINTER_LOWMASK));
   reverse = ((reverse >> 2) & VTK_PARSE_POINTER_MASK);

   /* make sure we don't exceed the VTK_PARSE_POINTER bitfield */
   if ((ptr1 & VTK_PARSE_POINTER_MASK) != ptr1)
     {
     ptr1 = VTK_PARSE_BAD_INDIRECT;
     break;
     }
   }

  return (ptr1 | result);
}

%}
/*----------------------------------------------------------------
 * Start of yacc section
 */


/* The parser will shift/reduce values <str> or <integer> */

%union{
  char *str;
  int   integer;
}

/* Lexical tokens defined in in vtkParse.l */

%token STRUCT
%token CLASS
%token PUBLIC
%token PRIVATE
%token PROTECTED
%token VIRTUAL
%token <str> ID
%token <str> STRING_LITERAL
%token <str> INT_LITERAL
%token <str> HEX_LITERAL
%token <str> OCT_LITERAL
%token <str> FLOAT_LITERAL
%token <str> CHAR_LITERAL
%token <str> ZERO
%token INT
%token FLOAT
%token SHORT
%token LONG
%token LONG_LONG
%token INT64__
%token DOUBLE
%token VOID
%token CHAR
%token SIGNED_CHAR
%token BOOL
%token SSIZE_T
%token SIZE_T
%token <str> OSTREAM
%token <str> ISTREAM
%token ENUM
%token UNION
%token CLASS_REF
%token OTHER
%token CONST
%token CONST_PTR
%token CONST_EQUAL
%token OPERATOR
%token UNSIGNED
%token SIGNED
%token FRIEND
%token INLINE
%token MUTABLE
%token TEMPLATE
%token TYPENAME
%token TYPEDEF
%token NAMESPACE
%token USING
%token <str> VTK_ID
%token STATIC
%token EXTERN
%token <str> VAR_FUNCTION
%token VTK_LEGACY
%token NEW
%token DELETE
%token OP_LSHIFT_EQ
%token OP_RSHIFT_EQ
%token OP_LSHIFT
%token OP_RSHIFT
%token OP_ARROW_POINTER
%token OP_ARROW
%token OP_INCR
%token OP_DECR
%token OP_PLUS_EQ
%token OP_MINUS_EQ
%token OP_TIMES_EQ
%token OP_DIVIDE_EQ
%token OP_REMAINDER_EQ
%token OP_AND_EQ
%token OP_OR_EQ
%token OP_XOR_EQ
%token OP_LOGIC_AND_EQ
%token OP_LOGIC_OR_EQ
%token OP_LOGIC_AND
%token OP_LOGIC_OR
%token OP_LOGIC_EQ
%token OP_LOGIC_NEQ
%token OP_LOGIC_LEQ
%token OP_LOGIC_GEQ
%token ELLIPSIS
%token DOUBLE_COLON
%token <str> LP
%token <str> LA

/* type tokens */
%token <str> StdString
%token <str> UnicodeString
%token IdType
%token TypeInt8
%token TypeUInt8
%token TypeInt16
%token TypeUInt16
%token TypeInt32
%token TypeUInt32
%token TypeInt64
%token TypeUInt64
%token TypeFloat32
%token TypeFloat64

/* macro tokens */
%token SetMacro
%token GetMacro
%token SetStringMacro
%token GetStringMacro
%token SetClampMacro
%token SetObjectMacro
%token GetObjectMacro
%token BooleanMacro
%token SetVector2Macro
%token SetVector3Macro
%token SetVector4Macro
%token SetVector6Macro
%token GetVector2Macro
%token GetVector3Macro
%token GetVector4Macro
%token GetVector6Macro
%token SetVectorMacro
%token GetVectorMacro
%token ViewportCoordinateMacro
%token WorldCoordinateMacro
%token TypeMacro

/* special tokens */
%token VTK_CONSTANT_DEF
%token VTK_BYTE_SWAP_DECL

%%
/*
 * Here is the start of the grammer
 */

strt: | strt
        { startSig(); clearTypeId(); clearTemplate(); }
        file_item;

file_item:
     var
   | anonymous_enum
   | named_enum
   | anonymous_union
   | named_union
   | anonymous_struct
   | using
   | namespace
   | extern
   | typedef
   | class_def
   | template class_def
   | CLASS_REF
   | operator func_body { output_function(); }
   | template operator func_body { output_function(); }
   | scoped_operator func_body { reject_function(); }
   | function func_body { output_function(); }
   | scoped_method func_body { reject_function(); }
   | template function func_body { output_function(); }
   | legacy_function func_body { legacySig(); output_function(); }
   | macro
   | vtk_constant_def
   | class_id ';'
   | ID '(' maybe_other ')'
   | VTK_ID '(' maybe_other ')'
   | ';';

/*
 * extern section is parsed, but "extern" is ignored
 */

extern: EXTERN STRING_LITERAL '{' strt '}';

/*
 * Namespace is pushed and its body is parsed
 */

namespace: NAMESPACE class_id { pushNamespace($<str>2); }
            '{' strt '}' { popNamespace(); };
         | NAMESPACE '{' maybe_other '}';

/*
 * Class definitions
 */

class_def: CLASS any_id { start_class($<str>2, 0); }
    optional_scope '{' class_def_body '}' { end_class(); }
  | CLASS any_id '<' types '>' { reject_class($<str>2, 0); }
    optional_scope '{' class_def_body '}' { end_class(); }
  | STRUCT any_id { start_class($<str>2, 1); }
    optional_scope '{' class_def_body '}' { end_class(); }
  | STRUCT any_id '<' types '>' { reject_class($<str>2, 1); }
    optional_scope '{' class_def_body '}' { end_class(); };

class_def_body: | class_def_body
                  { startSig(); clearTypeId(); clearTemplate(); }
                  class_def_item;

class_def_item: scope_type ':'
   | var
   | anonymous_enum
   | named_enum
   | anonymous_union
   | named_union
   | anonymous_struct
   | using
   | typedef
   | internal_class
   | FRIEND internal_class
   | template_internal_class
   | CLASS_REF
   | operator func_body { output_function(); }
   | FRIEND operator func_body { ClassInfo *tmpc = currentClass;
     currentClass = NULL; output_function(); currentClass = tmpc; }
   | template operator func_body { output_function(); }
   | method func_body { output_function(); }
   | FRIEND method func_body { ClassInfo *tmpc = currentClass;
     currentClass = NULL; output_function(); currentClass = tmpc; }
   | template method func_body { output_function(); }
   | legacy_method func_body { legacySig(); output_function(); }
   | VTK_BYTE_SWAP_DECL '(' maybe_other ')' ';'
   | macro
   | vtk_constant_def
   | ';';

optional_scope: | ':' scope_list;

scope_list: scope_list_item | scope_list_item ',' scope_list;

scope_list_item: maybe_scoped_id
  | PRIVATE maybe_scoped_id
  | PROTECTED maybe_scoped_id
  | PUBLIC maybe_scoped_id
    {
      vtkParse_AddPointerToArray(&currentClass->SuperClasses,
                                 &currentClass->NumberOfSuperClasses,
                                 vtkstrdup($<str>2));
    };

scope_type: PUBLIC {access_level = VTK_ACCESS_PUBLIC;}
          | PRIVATE {access_level = VTK_ACCESS_PROTECTED;}
          | PROTECTED {access_level = VTK_ACCESS_PRIVATE;};

/*
 * Enums
 *
 * The values assigned to enum constants are handled as strings.
 * The text can be dropped into the generated .cxx file and evaluated there,
 * as long as all IDs are properly scoped.
 */

named_enum: ENUM any_id {start_enum($<str>2);} '{' enum_list '}'
   {end_enum();} maybe_other_no_semi ';';

anonymous_enum: ENUM {start_enum(NULL);} '{' enum_list '}'
   {end_enum();} maybe_other_no_semi ';';

enum_list: | enum_item | enum_item ',' enum_list;

enum_item: any_id {add_enum($<str>1, NULL);}
         | any_id '=' integer_expression {add_enum($<str>1, $<str>3);};

integer_value: integer_literal {postSig($<str>1);}
             | any_id | scoped_id | templated_id;

integer_literal: ZERO | INT_LITERAL | OCT_LITERAL | HEX_LITERAL | CHAR_LITERAL;

integer_expression: integer_value { $<str>$ = $<str>1; }
     | math_unary_op {postSig($<str>1);} integer_expression
       {
         $<str>$ = (char *)malloc(strlen($<str>1) + strlen($<str>3) + 1);
         sprintf($<str>$, "%s%s", $<str>1, $<str>3);
       }
     | integer_value math_binary_op {postSig($<str>2);} integer_expression
       {
         $<str>$ = (char *)malloc(strlen($<str>1) + strlen($<str>2) +
                                  strlen($<str>4) + 3);
         sprintf($<str>$, "%s %s %s", $<str>1, $<str>2, $<str>4);
       }
     | '(' {postSig("(");} integer_expression ')'
       {
         postSig(")");
         $<str>$ = (char *)malloc(strlen($<str>3) + 3);
         sprintf($<str>$, "(%s)", $<str>3);
       };

math_unary_op:   '-' { $<str>$ = "-"; } | '+' { $<str>$ = "+"; }
               | '~' { $<str>$ = "~"; };

math_binary_op:  '-' { $<str>$ = "-"; } | '+' { $<str>$ = "+"; }
               | '*' { $<str>$ = "*"; } | '/' { $<str>$ = "/"; }
               | '%' { $<str>$ = "%"; } | '&' { $<str>$ = "&"; }
               | '|' { $<str>$ = "|"; } | '^' { $<str>$ = "^"; };

/*
 * currently ignored items
 */

anonymous_struct: STRUCT '{' maybe_other '}' maybe_other_no_semi ';';

named_union: UNION any_id '{' maybe_other '}' maybe_other_no_semi ';';

anonymous_union: UNION '{' maybe_other '}' maybe_other_no_semi ';';

using: USING maybe_other_no_semi ';';

template_internal_class: template internal_class;

internal_class: CLASS any_id internal_class_body
              | STRUCT any_id internal_class_body;

internal_class_body: ';'
    | '{' maybe_other '}' ';'
    | ':' maybe_other_no_semi ';';

typedef: TYPEDEF type var_id maybe_var_array';'
 | TYPEDEF CLASS any_id '{' maybe_other '}' maybe_indirect_id ';'
 | TYPEDEF STRUCT any_id '{' maybe_other '}' maybe_indirect_id ';'
 | TYPEDEF type LA maybe_indirect_id ')' maybe_var_array ';'
 | TYPEDEF type LP maybe_indirect_id ')' maybe_var_array ';'
 | TYPEDEF type LA maybe_indirect_id ')' '(' ignore_args_list ')' ';'
 | TYPEDEF type LP maybe_indirect_id ')' '(' ignore_args_list ')' ';'
 | TYPEDEF anonymous_enum
 | TYPEDEF named_enum
 | TYPEDEF anonymous_union
 | TYPEDEF named_union
 | TYPEDEF anonymous_struct
 | TYPEDEF VAR_FUNCTION ';';

/*
 * Templates
 */

template: TEMPLATE '<' '>' { postSig("template<> "); clearTypeId(); }
        | TEMPLATE '<' { postSig("template<");
          clearTypeId(); startTemplate(); }
          template_args '>' { postSig("> "); clearTypeId(); };

template_args: template_arg
             | template_arg ',' { postSig(", "); clearTypeId(); }
               template_args;

template_arg: type_simple maybe_template_id
               {
               TemplateArg *arg = (TemplateArg *)malloc(sizeof(TemplateArg));
               vtkParse_InitTemplateArg(arg);
               arg->Type = $<integer>1;
               arg->Class = vtkstrdup(getTypeId());
               arg->Name = vtkstrdup(getVarName());
               arg->Value = vtkstrdup(getVarValue());
               vtkParse_AddItemMacro2(currentTemplate, Arguments, arg);
               }
            | class_or_typename maybe_template_id
               {
               TemplateArg *arg = (TemplateArg *)malloc(sizeof(TemplateArg));
               vtkParse_InitTemplateArg(arg);
               arg->Name = vtkstrdup(getVarName());
               arg->Value = vtkstrdup(getVarValue());
               vtkParse_AddItemMacro2(currentTemplate, Arguments, arg);
               }
            | { pushTemplate(); } template maybe_template_id
               {
               TemplateArgs *newTemplate = currentTemplate;
               TemplateArg *arg = (TemplateArg *)malloc(sizeof(TemplateArg));
               vtkParse_InitTemplateArg(arg);
               popTemplate();
               arg->Template = newTemplate;
               arg->Name = vtkstrdup(getVarName());
               arg->Value = vtkstrdup(getVarValue());
               vtkParse_AddItemMacro2(currentTemplate, Arguments, arg);
               };

class_or_typename: CLASS {postSig("class ");}
                 | TYPENAME {postSig("typename ");};

maybe_template_id: | any_id { setVarName($<str>1); }
                     maybe_var_assign;

/*
 * Items inside vtkLegacy macro are parsed and then rejected
 */

legacy_function: VTK_LEGACY '(' function ')' ;

legacy_method: VTK_LEGACY '(' method ')' ;

/*
 * Functions and Methods
 */

function: storage_type func;

scoped_method: storage_type scope func
      | class_id DOUBLE_COLON '~' destructor
      | INLINE class_id DOUBLE_COLON '~' destructor
      | class_id DOUBLE_COLON constructor
      | INLINE class_id DOUBLE_COLON constructor
      | class_id DOUBLE_COLON class_id DOUBLE_COLON '~' destructor
      | INLINE class_id DOUBLE_COLON class_id DOUBLE_COLON '~' destructor
      | class_id DOUBLE_COLON class_id DOUBLE_COLON constructor
      | INLINE class_id DOUBLE_COLON class_id DOUBLE_COLON constructor;

scope: class_id DOUBLE_COLON
      | scope class_id DOUBLE_COLON;

method: '~' destructor {openSig(); preSig("~"); closeSig();}
      | INLINE '~' destructor {openSig(); preSig("~"); closeSig();}
      | VIRTUAL '~' destructor
         {
         openSig(); preSig("virtual ~"); closeSig();
         currentFunction->IsVirtual = 1;
         }
      | constructor
      | INLINE constructor
      | storage_type func
      | VIRTUAL storage_type func
         {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->IsVirtual = 1;
         };

scoped_operator: class_id DOUBLE_COLON typecast_op_func
      | storage_type scope op_func;

operator:
        typecast_op_func
      | storage_type op_func
      | VIRTUAL type op_func
         {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->IsVirtual = 1;
         };

typecast_op_func:
  OPERATOR storage_type '('
    {
      postSig("(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
  args_list ')' { postSig(")"); } func_trailer
    {
      $<integer>$ = $<integer>2;
      postSig(";");
      preSig("operator ");
      closeSig();
      currentFunction->IsOperator = 1;
      currentFunction->Name = "operator typecast";
      if (HaveComment)
        {
        currentFunction->Comment = vtkstrdup(CommentText);
        }
      vtkParseDebug("Parsed operator", "operator typecast");
    };

op_func: op_sig { postSig(")"); } func_trailer
    {
      postSig(";");
      closeSig();
      currentFunction->Name = $<str>1;
      if (HaveComment)
        {
        currentFunction->Comment = vtkstrdup(CommentText);
        }
      vtkParseDebug("Parsed operator", currentFunction->Name);
    };

op_sig: OPERATOR op_token {postSig($<str>2);} '('
    {
      postSig("(");
      currentFunction->IsOperator = 1;
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    args_list ')' { $<str>$ = $<str>2; };

func: func_sig { postSig(")"); } func_trailer
    {
      postSig(";");
      closeSig();
      currentFunction->Name = $<str>1;
      if (HaveComment)
        {
        currentFunction->Comment = vtkstrdup(CommentText);
        }
      vtkParseDebug("Parsed func", currentFunction->Name);
    };

func_trailer:
  | '=' ZERO
    {
      postSig(" = 0");
      if (currentClass)
        {
        currentFunction->IsPureVirtual = 1;
        currentClass->IsAbstract = 1;
        }
    }
 | CONST_EQUAL ZERO
    {
      postSig(" const = 0");
      currentFunction->IsConst = 1;
      if (currentClass)
        {
        currentFunction->IsPureVirtual = 1;
        currentClass->IsAbstract = 1;
        }
    }
 | CONST
    {
      postSig(" const");
      currentFunction->IsConst = 1;
    };

func_body: ';' | '{' maybe_other '}';

func_sig: any_id '('
    {
      postSig("(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    } args_list ')' { $<str>$ = $<str>1; }
  | any_id '<' {markSig(); postSig("<");} types '>' '('
    {
      const char *cp;
      postSig(">(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
      cp = copySig();
      $<str>$ = (char *)malloc(strlen($<str>1) + strlen(cp) + 1);
      sprintf($<str>$, "%s%s", $<str>1, cp);
    } args_list ')' { $<str>$ = $<str>7; };

constructor: constructor_sig { postSig(");"); closeSig(); } maybe_initializers
    {
      currentFunction->Name = $<str>1;
      if (HaveComment)
        {
        currentFunction->Comment = vtkstrdup(CommentText);
        }
      vtkParseDebug("Parsed func", currentFunction->Name);
    };

constructor_sig: any_id '(' { postSig("("); } args_list ')';

maybe_initializers: | ':' initializer more_initializers;

more_initializers: | ',' initializer more_initializers;

initializer: any_id '(' maybe_other ')';

destructor: destructor_sig
    {
      postSig(");");
      closeSig();
      currentFunction->Name = (char *)malloc(strlen($<str>1) + 2);
      currentFunction->Name[0] = '~';
      strcpy(&currentFunction->Name[1], $<str>1);
      if (HaveComment)
        {
        currentFunction->Comment = vtkstrdup(CommentText);
        }
      vtkParseDebug("Parsed func", currentFunction->Name);
    };

destructor_sig: any_id '(' { postSig("(");} args_list ')';

/*
 * Arguments
 */

ignore_args_list: | ignore_more_args;

ignore_more_args: ELLIPSIS { postSig("..."); }
  | ignore_arg | ignore_arg ',' { postSig(", "); } ignore_more_args;

ignore_arg:
    type maybe_complex_var_id maybe_var_assign
  | VAR_FUNCTION
    { postSig("void (*"); postSig($<str>1); postSig(")(void *) "); };


args_list: | {clearTypeId();} more_args;

more_args:
    ELLIPSIS { currentFunction->IsVariadic = 1; postSig("..."); }
  | arg { clearTypeId(); }
  | arg ',' { clearTypeId(); postSig(", "); } more_args;

arg:
    { markSig(); }
    type maybe_complex_var_id
    {
      int i = currentFunction->NumberOfArguments;
      ValueInfo *arg = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(arg);

      handle_complex_type(arg, $<integer>2, $<integer>3, copySig());

      if (i < MAX_ARGS)
        {
        currentFunction->ArgTypes[i] = arg->Type;
        currentFunction->ArgClasses[i] = arg->Class;
        currentFunction->ArgCounts[i] = arg->Count;
        }

      if (getVarName())
        {
        arg->Name = vtkstrdup(getVarName());
        }

      vtkParse_AddItemMacro2(currentFunction, Arguments, arg);
    }
    maybe_var_assign
    {
      int i = currentFunction->NumberOfArguments-1;
      if (getVarValue())
        {
        currentFunction->Arguments[i]->Value = vtkstrdup(getVarValue());
        }
    }
  | VAR_FUNCTION
    {
      int i = currentFunction->NumberOfArguments;
      ValueInfo *arg = (ValueInfo *)malloc(sizeof(ValueInfo));

      vtkParse_InitValue(arg);

      markSig();
      postSig("void (*");
      postSig($<str>1);
      postSig(")(void *) ");

      handle_function_type(arg, $<str>1, copySig());

      if (i < MAX_ARGS)
        {
        currentFunction->ArgTypes[i] = arg->Type;
        currentFunction->ArgClasses[i] = arg->Class;
        currentFunction->ArgCounts[i] = arg->Count;
        }

      vtkParse_AddItemMacro2(currentFunction, Arguments, arg);
    };

maybe_indirect_id: any_id | type_indirection any_id;

maybe_var_assign: {clearVarValue();} | var_assign;

var_assign: '=' { postSig("="); clearVarValue();}
            value { setVarValue($<str>3); };

/*
 * Variables
 */

var:   storage_type var_id_maybe_assign maybe_other_vars ';'
     | STATIC VAR_FUNCTION maybe_other_vars ';'
     | VAR_FUNCTION maybe_other_vars ';';

var_id_maybe_assign: complex_var_id maybe_var_assign
     {
       int type = getStorageType();
       if (getVarValue() && ((type & VTK_PARSE_CONST) != 0) &&
           ((type & VTK_PARSE_INDIRECT) == 0) && getArrayNDims() == 0)
         {
         add_constant(getVarName(), getVarValue(),
                       (type & VTK_PARSE_UNQUALIFIED_TYPE), getTypeId(), 0);
         }
     };

maybe_other_vars:
     | maybe_other_vars ',' {postSig(", ");} other_var;

other_var:
       { setStorageTypeIndirection(0); } var_id_maybe_assign
     | type_indirection { setStorageTypeIndirection($<integer>1); }
       var_id_maybe_assign;

/* for args, the var_id is optional */
maybe_complex_var_id: maybe_var_id maybe_var_array { $<integer>$ = 0; }
     | p_or_lp_or_la maybe_indirect_maybe_var_id ')' { postSig(")"); }
       maybe_array_or_args
       { $<integer>$ = add_indirection($<integer>5,
                       add_indirection($<integer>1, $<integer>2)); };

/* for vars, the var_id is mandatory */
complex_var_id: var_id maybe_var_array { $<integer>$ = 0; }
     | lp_or_la maybe_indirect_var_id ')' { postSig(")"); }
       maybe_array_or_args
       { $<integer>$ = add_indirection($<integer>5,
                       add_indirection($<integer>1, $<integer>2)); };

p_or_lp_or_la: '(' { postSig("("); $<integer>$ = 0; }
        | LP { postSig("("); postSig($<str>1); postSig("*");
               $<integer>$ = VTK_PARSE_POINTER; }
        | LA { postSig("("); postSig($<str>1); postSig("&");
               $<integer>$ = VTK_PARSE_REF; };

lp_or_la: LP { postSig("("); postSig($<str>1); postSig("*");
               $<integer>$ = VTK_PARSE_POINTER; }
        | LA { postSig("("); postSig($<str>1); postSig("&");
               $<integer>$ = VTK_PARSE_REF; };

maybe_array_or_args: { $<integer>$ = 0; }
   | '(' { pushFunction(); postSig("("); } args_list ')'
     { $<integer>$ = VTK_PARSE_FUNCTION; postSig(")"); popFunction(); }
   | var_array { $<integer>$ = 0; };

maybe_indirect_maybe_var_id:
       maybe_complex_var_id { $<integer>$ = $<integer>1; }
     | type_indirection maybe_complex_var_id
       { $<integer>$ = add_indirection($<integer>1, $<integer>2);};

maybe_indirect_var_id:
       complex_var_id { $<integer>$ = $<integer>1; }
     | type_indirection complex_var_id
       { $<integer>$ = add_indirection($<integer>1, $<integer>2);};

maybe_var_id: {clearVarName(); chopSig();} | var_id;

var_id: any_id {setVarName($<str>1);};

maybe_var_array: {clearArray();} | var_array;

var_array: {clearArray();} array;

array: more_array '[' {postSig("[");} array_size ']' {postSig("]");};

more_array: | array;

array_size: {pushArraySize("");}
    | integer_expression {pushArraySize($<str>1);};

/*
 * Any Id, evaluates to string and sigs itself
 */

any_id: VTK_ID {postSig($<str>1);}
     | ID {postSig($<str>1);}
     | ISTREAM {postSig($<str>1);}
     | OSTREAM {postSig($<str>1);}
     | StdString {postSig($<str>1);}
     | UnicodeString {postSig($<str>1);};

/*
 * Types
 */

storage_type:
      type {$<integer>$ = $<integer>1; setStorageType($<integer>$);}
    | MUTABLE type {$<integer>$ = $<integer>2; setStorageType($<integer>$);}
    | EXTERN type {$<integer>$ = $<integer>2; setStorageType($<integer>$);}
    | EXTERN STRING_LITERAL type
      {$<integer>$ = $<integer>3; setStorageType($<integer>$);}
    | INLINE type {$<integer>$ = $<integer>2; setStorageType($<integer>$);}
    | static_mod type {$<integer>$ = ($<integer>1 | $<integer>2);
      setStorageType($<integer>$);}
    | INLINE static_mod type {$<integer>$ = ($<integer>2 | $<integer>3);
      setStorageType($<integer>$);};

static_mod:
      STATIC {postSig("static "); $<integer>$ = VTK_PARSE_STATIC; }
    | STATIC INLINE {postSig("static "); $<integer>$ = VTK_PARSE_STATIC; };

type: type_red {$<integer>$ = $<integer>1;}
    | type_red type_indirection {$<integer>$ = ($<integer>1 | $<integer>2);};

type_red: type_red1 {$<integer>$ = $<integer>1;}
    | const_mod type_red1 {$<integer>$ = (VTK_PARSE_CONST | $<integer>2);}
    | type_red1 const_mod {$<integer>$ = (VTK_PARSE_CONST | $<integer>1);};

const_mod: CONST {postSig("const ");};

type_red1: type_red2
    | templated_id
      {postSig(" "); setTypeId($<str>1); $<integer>$ = VTK_PARSE_UNKNOWN;}
    | scoped_id
      {postSig(" "); setTypeId($<str>1); $<integer>$ = VTK_PARSE_UNKNOWN;}
    | TYPENAME {postSig("typename ");} maybe_scoped_id
      {postSig(" "); setTypeId($<str>3); $<integer>$ = VTK_PARSE_UNKNOWN;};

templated_id:
   VTK_ID '<' { markSig(); postSig($<str>1); postSig("<");} types '>'
     {chopSig(); postSig(">"); $<str>$ = vtkstrdup(copySig()); clearTypeId();}
 | ID '<' { markSig(); postSig($<str>1); postSig("<");} types '>'
     {chopSig(); postSig(">"); $<str>$ = vtkstrdup(copySig()); clearTypeId();};

types: type | type ',' {postSig(", ");} types;

maybe_scoped_id: VTK_ID {postSig($<str>1);}
               | ID {postSig($<str>1);}
               | ISTREAM {postSig($<str>1);}
               | OSTREAM {postSig($<str>1);}
               | StdString {postSig($<str>1);}
               | UnicodeString {postSig($<str>1);}
               | templated_id
               | scoped_id;

scoped_id: class_id DOUBLE_COLON maybe_scoped_id
           {
             $<str>$ = (char *)malloc(strlen($<str>1)+strlen($<str>3)+3);
             sprintf($<str>$, "%s::%s", $<str>1, $<str>3);
             preScopeSig($<str>1);
           }
         | templated_id DOUBLE_COLON maybe_scoped_id
           {
             $<str>$ = (char *)malloc(strlen($<str>1)+strlen($<str>3)+3);
             sprintf($<str>$, "%s::%s", $<str>1, $<str>3);
             preScopeSig("");
           };

class_id: ID | VTK_ID | ISTREAM | OSTREAM | StdString | UnicodeString;


/* &          is VTK_PARSE_REF
   *          is VTK_PARSE_POINTER
   *&         is VTK_PARSE_POINTER_REF
   **         is VTK_PARSE_POINTER_POINTER
   **&        is VTK_PARSE_POINTER_POINTER_REF
   *const     is VTK_PARSE_CONST_POINTER
   *const&    is VTK_PARSE_CONST_POINTER_REF
   *const*    is VTK_PARSE_POINTER_CONST_POINTER
   everything else is VTK_PARSE_BAD_INDIRECT,
   unless the VTK_PARSE_INDIRECT bitfield is expanded.
   */

type_indirection:
    '&' { postSig("&"); $<integer>$ = VTK_PARSE_REF;}
  | pointers '&' { postSig("&"); $<integer>$ = ($<integer>1 | VTK_PARSE_REF);}
  | pointers { $<integer>$ = $<integer>1; };

/* "VTK_BAD_INDIRECT" occurs when the bitfield fills up */

pointers:
    pointer_or_const_pointer { $<integer>$ = $<integer>1; }
  | pointers pointer_or_const_pointer
     {
       int n;
       n = (($<integer>1 << 2) | $<integer>2);
       if ((n & VTK_PARSE_INDIRECT) != n)
         {
         n = VTK_PARSE_BAD_INDIRECT;
         }
      $<integer>$ = n;
    };

pointer_or_const_pointer:
   '*' { postSig("*"); $<integer>$ = VTK_PARSE_POINTER; }
  | CONST_PTR { postSig("*const "); $<integer>$ = VTK_PARSE_CONST_POINTER; };

type_red2:
   type_simple { $<integer>$ = $<integer>1;}
 | CLASS type_id { $<integer>$ = $<integer>2; }
 | STRUCT type_id { $<integer>$ = $<integer>2; }
 | UNION ID { typeSig($<str>2); $<integer>$ = VTK_PARSE_UNKNOWN; }
 | UNION VTK_ID { typeSig($<str>2); $<integer>$ = VTK_PARSE_UNKNOWN; }
 | ENUM ID { typeSig($<str>2); $<integer>$ = VTK_PARSE_UNKNOWN; }
 | ENUM VTK_ID { typeSig($<str>2); $<integer>$ = VTK_PARSE_UNKNOWN; };

type_simple:
   type_primitive { $<integer>$ = $<integer>1;}
 | type_id { $<integer>$ = $<integer>1;};

type_id:
   StdString { typeSig($<str>1); $<integer>$ = VTK_PARSE_STRING;}
 | UnicodeString { typeSig($<str>1); $<integer>$ = VTK_PARSE_UNICODE_STRING;}
 | OSTREAM { typeSig($<str>1); $<integer>$ = VTK_PARSE_OSTREAM; }
 | ISTREAM { typeSig($<str>1); $<integer>$ = VTK_PARSE_ISTREAM; }
 | ID { typeSig($<str>1); $<integer>$ = VTK_PARSE_UNKNOWN; }
 | VTK_ID { typeSig($<str>1); $<integer>$ = VTK_PARSE_OBJECT; }

type_primitive:
  VOID   { typeSig("void"); $<integer>$ = VTK_PARSE_VOID;} |
  FLOAT  { typeSig("float"); $<integer>$ = VTK_PARSE_FLOAT;} |
  DOUBLE { typeSig("double"); $<integer>$ = VTK_PARSE_DOUBLE;} |
  BOOL { typeSig("bool"); $<integer>$ = VTK_PARSE_BOOL;} |
  SSIZE_T { typeSig("ssize_t"); $<integer>$ = VTK_PARSE_SSIZE_T;} |
  SIZE_T { typeSig("size_t"); $<integer>$ = VTK_PARSE_SIZE_T;} |
  SIGNED_CHAR {typeSig("signed char"); $<integer>$ = VTK_PARSE_SIGNED_CHAR;} |
  TypeInt8 { typeSig("vtkTypeInt8"); $<integer>$ = VTK_PARSE_INT8; } |
  TypeUInt8 { typeSig("vtkTypeUInt8"); $<integer>$ = VTK_PARSE_UINT8; } |
  TypeInt16 { typeSig("vtkTypeInt16"); $<integer>$ = VTK_PARSE_INT16; } |
  TypeUInt16 { typeSig("vtkTypeUInt16"); $<integer>$ = VTK_PARSE_UINT16; } |
  TypeInt32 { typeSig("vtkTypeInt32"); $<integer>$ = VTK_PARSE_INT32; } |
  TypeUInt32 { typeSig("vtkTypeUInt32"); $<integer>$ = VTK_PARSE_UINT32; } |
  TypeInt64 { typeSig("vtkTypeInt64"); $<integer>$ = VTK_PARSE_INT64; } |
  TypeUInt64 { typeSig("vtkTypeUInt64"); $<integer>$ = VTK_PARSE_UINT64; } |
  TypeFloat32 { typeSig("vtkTypeFloat32"); $<integer>$ = VTK_PARSE_FLOAT32; } |
  TypeFloat64 { typeSig("vtkTypeFloat64"); $<integer>$ = VTK_PARSE_FLOAT64; } |
  SIGNED {typeSig("signed");} type_integer { $<integer>$ = $<integer>3;}; |
  UNSIGNED {typeSig("unsigned");}
   type_integer { $<integer>$ = (VTK_PARSE_UNSIGNED | $<integer>3);} |
  type_integer { $<integer>$ = $<integer>1;};

type_integer:
  CHAR   { typeSig("char"); $<integer>$ = VTK_PARSE_CHAR;} |
  INT    { typeSig("int"); $<integer>$ = VTK_PARSE_INT;} |
  SHORT  { typeSig("short"); $<integer>$ = VTK_PARSE_SHORT;} |
  LONG   { typeSig("long"); $<integer>$ = VTK_PARSE_LONG;} |
  IdType { typeSig("vtkIdType"); $<integer>$ = VTK_PARSE_ID_TYPE;} |
  LONG_LONG { typeSig("long long"); $<integer>$ = VTK_PARSE_LONG_LONG;} |
  INT64__ { typeSig("__int64"); $<integer>$ = VTK_PARSE___INT64;};

/*
 * Values
 */

value: literal { $<str>$ = $<str>1; }
         | '{' { postSig("{ "); } value more_values maybe_comma '}'
        {
          char *cp;
          postSig("}");
          cp = (char *)malloc(strlen($<str>3) + strlen($<str>4) + 5);
          sprintf(cp, "{ %s%s }", $<str>3, $<str>4);
          $<str>$ = cp;
        };

maybe_comma: | ',';

more_values: {$<str>$ = vtkstrdup("");}
      | more_values ',' { postSig(", "); } value
        {
          char *cp;
          cp = (char *)malloc(strlen($<str>1) + strlen($<str>4) + 3);
          sprintf(cp, "%s, %s", $<str>1, $<str>4);
          $<str>$ = cp;
        };

literal:  literal2 {$<str>$ = $<str>1;}
          | '+' {postSig("+");} literal2 {$<str>$ = $<str>3;}
          | '-' {postSig("-");} literal2 {
             $<str>$ = (char *)malloc(strlen($<str>3)+2);
             sprintf($<str>$, "-%s", $<str>3); }
          | string_literal {$<str>$ = $<str>1; postSig($<str>1);}
          | '(' {postSig("(");} literal ')' {postSig(")"); $<str>$ = $<str>3;}
          | ID '<' {postSig($<str>1); postSig("<");}
            type_red '>' '(' {postSig("<(");} literal2 ')'
            {
            postSig(")");
            $<str>$ = (char *)malloc(strlen($<str>1) + strlen(getTypeId()) +
                                     strlen($<str>8) + 5);
            sprintf($<str>$, "%s<%s>(%s)", $<str>1, getTypeId(), $<str>8);
            };

string_literal: STRING_LITERAL {$<str>$ = $<str>1;}
              | string_literal STRING_LITERAL {
                size_t i = strlen($<str>1);
                char *cp = (char *)malloc(i + strlen($<str>2) + 1);
                strcpy(cp, $<str>1);
                strcpy(&cp[i], $<str>2);
                $<str>$ = cp; };

literal2:   ZERO {$<str>$ = $<str>1; postSig($<str>1);}
          | INT_LITERAL {$<str>$ = $<str>1; postSig($<str>1);}
          | OCT_LITERAL {$<str>$ = $<str>1; postSig($<str>1);}
          | HEX_LITERAL {$<str>$ = $<str>1; postSig($<str>1);}
          | FLOAT_LITERAL {$<str>$ = $<str>1; postSig($<str>1);}
          | CHAR_LITERAL {$<str>$ = $<str>1; postSig($<str>1);}
          | ID {$<str>$ = vtkstrdup(add_const_scope($<str>1));
                postSig($<str>1);}
          | VTK_ID {$<str>$ = vtkstrdup(add_const_scope($<str>1));
                postSig($<str>1);};

/*
 * VTK Macros
 */



macro:
  SetMacro '(' any_id ',' {preSig("void Set"); postSig("(");} type ')'
   {
   postSig("a);");
   sprintf(temps,"Set%s",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   add_argument(currentFunction, $<integer>6, getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
| GetMacro '(' {postSig("Get");} any_id ','
   {markSig();} type {swapSig();} ')'
   {
   postSig("();");
   sprintf(temps,"Get%s",$<str>4);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   set_return(currentFunction, $<integer>7, getTypeId(), 0);
   output_function();
   }
| SetStringMacro '(' {preSig("void Set");} any_id ')'
   {
   postSig("(char *);");
   sprintf(temps,"Set%s",$<str>4);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   add_argument(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
| GetStringMacro '(' {preSig("char *Get");} any_id ')'
   {
   postSig("();");
   sprintf(temps,"Get%s",$<str>4);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   set_return(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   output_function();
   }
| SetClampMacro '(' any_id ',' {startSig(); markSig();} type_red2 {closeSig();}
     ',' maybe_other_no_semi ')'
   {
   char *local;
   chopSig();
   local = vtkstrdup(copySig());
   sprintf(currentFunction->Signature,"void Set%s(%s);",$<str>3,local);
   sprintf(temps,"Set%s",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   add_argument(currentFunction, $<integer>6, getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%sGet%sMinValue();",local,$<str>3);
   sprintf(temps,"Get%sMinValue",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   set_return(currentFunction, $<integer>6, getTypeId(), 0);
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%sGet%sMaxValue();",local,$<str>3);
   sprintf(temps,"Get%sMaxValue",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   set_return(currentFunction, $<integer>6, getTypeId(), 0);
   output_function();
   free(local);
   }
| SetObjectMacro '(' any_id ','
  {preSig("void Set"); postSig("("); } type_red2 ')'
   {
   postSig("*);");
   sprintf(temps,"Set%s",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   add_argument(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
| GetObjectMacro '(' {postSig("*Get");} any_id ','
   {markSig();} type_red2 {swapSig();} ')'
   {
   postSig("();");
   sprintf(temps,"Get%s",$<str>4);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   set_return(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   output_function();
   }
| BooleanMacro '(' any_id ',' type_red2 ')'
   {
   sprintf(temps,"%sOn",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   startSig();
   postSig("void ");
   postSig(temps);
   postSig("();");
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();

   sprintf(temps,"%sOff",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   startSig();
   postSig("void ");
   postSig(temps);
   postSig("();");
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
| SetVector2Macro '(' any_id ',' {startSig(); markSig();} type_red2 ')'
   {
   chopSig();
   outputSetVectorMacro($<str>3, $<integer>6, copySig(), 2);
   }
| GetVector2Macro '(' any_id ',' {startSig(); markSig();} type_red2 ')'
   {
   chopSig();
   outputGetVectorMacro($<str>3, $<integer>6, copySig(), 2);
   }
| SetVector3Macro '(' any_id ',' {startSig(); markSig();} type_red2 ')'
   {
   chopSig();
   outputSetVectorMacro($<str>3, $<integer>6, copySig(), 3);
   }
| GetVector3Macro  '(' any_id ',' {startSig(); markSig();} type_red2 ')'
   {
   chopSig();
   outputGetVectorMacro($<str>3, $<integer>6, copySig(), 3);
   }
| SetVector4Macro '(' any_id ',' {startSig(); markSig();} type_red2 ')'
   {
   chopSig();
   outputSetVectorMacro($<str>3, $<integer>6, copySig(), 4);
   }
| GetVector4Macro  '(' any_id ',' {startSig(); markSig();} type_red2 ')'
   {
   chopSig();
   outputGetVectorMacro($<str>3, $<integer>6, copySig(), 4);
   }
| SetVector6Macro '(' any_id ',' {startSig(); markSig();} type_red2 ')'
   {
   chopSig();
   outputSetVectorMacro($<str>3, $<integer>6, copySig(), 6);
   }
| GetVector6Macro  '(' any_id ',' {startSig(); markSig();} type_red2 ')'
   {
   chopSig();
   outputGetVectorMacro($<str>3, $<integer>6, copySig(), 6);
   }
| SetVectorMacro  '(' any_id ',' {startSig(); markSig();}
     type_red2 ',' INT_LITERAL ')'
   {
   char *local;
   chopSig();
   local = vtkstrdup(copySig());
   sprintf(currentFunction->Signature,"void Set%s(%s a[%s]);",
           $<str>3, local, $<str>8);
   sprintf(temps,"Set%s",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   add_argument(currentFunction, (VTK_PARSE_POINTER | $<integer>6),
                getTypeId(), atol($<str>8));
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   free(local);
   }
| GetVectorMacro  '(' any_id ',' {startSig(); markSig();}
     type_red2 ',' INT_LITERAL ')'
   {
   char *local;
   chopSig();
   local = vtkstrdup(copySig());
   sprintf(currentFunction->Signature,"%s *Get%s();", local, $<str>3);
   sprintf(temps,"Get%s",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   set_return(currentFunction, (VTK_PARSE_POINTER | $<integer>6),
              getTypeId(), atol($<str>8));
   output_function();
   free(local);
   }
| ViewportCoordinateMacro '(' any_id ')'
   {
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate();",
             $<str>3);

     sprintf(temps,"Get%sCoordinate",$<str>3);
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     set_return(currentFunction, VTK_PARSE_OBJECT_PTR, "vtkCoordinate", 0);
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s(double, double);",
             $<str>3);
     sprintf(temps,"Set%s",$<str>3);
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s(double a[2]);",
             $<str>3);
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     add_argument(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 2);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s();", $<str>3);
     sprintf(temps,"Get%s",$<str>3);
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     set_return(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 2);
     output_function();
   }
| WorldCoordinateMacro '(' any_id ')'
   {
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate();",
             $<str>3);

     sprintf(temps,"Get%sCoordinate",$<str>3);
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     set_return(currentFunction, VTK_PARSE_OBJECT_PTR, "vtkCoordinate", 0);
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,
             "void Set%s(double, double, double);",
             $<str>3);
     sprintf(temps,"Set%s",$<str>3);
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s(double a[3]);",
             $<str>3);
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     add_argument(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 3);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s();", $<str>3);
     sprintf(temps,"Get%s",$<str>3);
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     set_return(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 3);
     output_function();
   }
| TypeMacro '(' any_id ',' any_id ')'
   {
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "const char *GetClassName();");
   sprintf(temps,"GetClassName");
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   set_return(currentFunction, (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR),
              "char", 0);
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "int IsA(const char *name);");
   sprintf(temps,"IsA");
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   add_argument(currentFunction, (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR),
                "char", 0);
   set_return(currentFunction, VTK_PARSE_INT, "int", 0);
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "%s *NewInstance();", $<str>3);
   sprintf(temps,"NewInstance");
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   set_return(currentFunction, VTK_PARSE_OBJECT_PTR, $<str>3, 0);
   output_function();

   if ( is_concrete )
     {
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature, "%s *SafeDownCast(vtkObject* o);",
             $<str>3);
     sprintf(temps,"SafeDownCast");
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     add_argument(currentFunction, VTK_PARSE_OBJECT_PTR, "vtkObject", 0);
     set_return(currentFunction, (VTK_PARSE_STATIC | VTK_PARSE_OBJECT_PTR),
                $<str>3, 0);
     output_function();
     }
   }
| TypeMacro '(' any_id ',' any_id ',' ')'
   {
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "const char *GetClassName();");
   sprintf(temps,"GetClassName");
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   set_return(currentFunction, (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR),
              "char", 0);
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,
           "int IsA(const char *name);");
   sprintf(temps,"IsA");
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   add_argument(currentFunction, (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR),
                "char", 0);
   set_return(currentFunction, VTK_PARSE_INT, "int", 0);
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "%s *NewInstance();", $<str>3);
   sprintf(temps,"NewInstance");
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   set_return(currentFunction, VTK_PARSE_OBJECT_PTR, $<str>3, 0);
   output_function();

   if ( is_concrete )
     {
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature, "%s *SafeDownCast(vtkObject* o);",
             $<str>3);
     sprintf(temps,"SafeDownCast");
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     add_argument(currentFunction, VTK_PARSE_OBJECT_PTR, "vtkObject", 0);
     set_return(currentFunction, (VTK_PARSE_STATIC | VTK_PARSE_OBJECT_PTR),
                $<str>3, 0);
     output_function();
     }
   }
;

/*
 * Operators
 */

op_token: '(' ')' { $<str>$ = "operator()"; }
        | '[' ']' { $<str>$ = "operator[]"; }
        | NEW '[' ']' { $<str>$ = "operator new[]"; }
        | DELETE '[' ']' { $<str>$ = "operator delete[]"; }
        | op_token_no_delim;

op_token_no_delim: '=' { $<str>$ = "operator="; }
   | '*' { $<str>$ = "operator*"; } | '/' { $<str>$ = "operator/"; }
   | '-' { $<str>$ = "operator-"; } | '+' { $<str>$ = "operator+"; }
   | '!' { $<str>$ = "operator!"; } | '~' { $<str>$ = "operator~"; }
   | ',' { $<str>$ = "operator,"; } | '<' { $<str>$ = "operator<"; }
   | '>' { $<str>$ = "operator>"; } | '&' { $<str>$ = "operator&"; }
   | '|' { $<str>$ = "operator|"; } | '^' { $<str>$ = "operator^"; }
   | '%' { $<str>$ = "operator%"; }
   | NEW { $<str>$ = "operator new"; }
   | DELETE { $<str>$ = "operator delete"; }
   | OP_LSHIFT_EQ { $<str>$ = "operator<<="; }
   | OP_RSHIFT_EQ { $<str>$ = "operator>>="; }
   | OP_LSHIFT { $<str>$ = "operator<<"; }
   | OP_RSHIFT { $<str>$ = "operator>>"; }
   | OP_ARROW_POINTER { $<str>$ = "operator->*"; }
   | OP_ARROW { $<str>$ = "operator->"; }
   | OP_PLUS_EQ { $<str>$ = "operator+="; }
   | OP_MINUS_EQ { $<str>$ = "operator-="; }
   | OP_TIMES_EQ { $<str>$ = "operator*="; }
   | OP_DIVIDE_EQ { $<str>$ = "operator/="; }
   | OP_REMAINDER_EQ { $<str>$ = "operator%="; }
   | OP_INCR { $<str>$ = "operator++"; }
   | OP_DECR { $<str>$ = "operator--"; }
   | OP_AND_EQ { $<str>$ = "operator&="; }
   | OP_OR_EQ { $<str>$ = "operator|="; }
   | OP_XOR_EQ { $<str>$ = "operator^="; }
   | OP_LOGIC_AND_EQ {$<str>$ = "operator&&=";}
   | OP_LOGIC_OR_EQ {$<str>$ = "operator||=";}
   | OP_LOGIC_AND { $<str>$ = "operator&&"; }
   | OP_LOGIC_OR { $<str>$ = "operator||"; }
   | OP_LOGIC_EQ { $<str>$ = "operator=="; }
   | OP_LOGIC_NEQ { $<str>$ = "operator!="; }
   | OP_LOGIC_LEQ { $<str>$ = "operator<="; }
   | OP_LOGIC_GEQ { $<str>$ = "operator>="; };

/*
 * "VTK_CONSTANT some_value"
 */

vtk_constant_def: VTK_CONSTANT_DEF
  {
  static char name[256];
  static char value[256];
  size_t i = 0;
  char *cp = $<str>1;
  while ((*cp >= 'a' && *cp <= 'z') ||
         (*cp >= 'A' && *cp <= 'Z') ||
         (*cp >= '0' && *cp <= '9') ||
         *cp == '_') { name[i++] = *cp++; }
  name[i] = '\0';
  while (*cp == ' ' || *cp == '\t') { cp++; }
  strcpy(value, cp);
  i = strlen(value);
  while (i > 0 && (value[i-1] == '\n' || value[i-1] == '\r' ||
                   value[i-1] == '\t' || value[i-1] == ' ')) { i--; }
  value[i] = '\0';
  add_constant(name, value, 0, NULL, 1);
  };

/*
 * These just eat up misc garbage
 */
maybe_other : | maybe_other other_stuff;
maybe_other_no_semi : | maybe_other_no_semi other_stuff_no_semi;

other_stuff : ';' | other_stuff_no_semi;

other_stuff_no_semi : OTHER | braces | parens | brackets | TYPEDEF
   | op_token_no_delim | ':' | '.' | DOUBLE_COLON | CLASS | TEMPLATE
   | ISTREAM | OSTREAM | StdString | UnicodeString | type_primitive
   | OCT_LITERAL | INT_LITERAL | HEX_LITERAL | FLOAT_LITERAL | CHAR_LITERAL
   | STRING_LITERAL | CLASS_REF | CONST | CONST_PTR | CONST_EQUAL | STRUCT
   | OPERATOR | STATIC | INLINE | VIRTUAL | ENUM | UNION | TYPENAME
   | ZERO | VAR_FUNCTION | ELLIPSIS | PUBLIC | PROTECTED | PRIVATE
   | NAMESPACE | USING | EXTERN | ID | VTK_ID | vtk_constant_def;

braces: '{' maybe_other '}';
brackets: '[' maybe_other ']';
parens: '(' maybe_other ')' | LP maybe_other ')' | LA maybe_other ')';

%%
#include <string.h>
#include "lex.yy.c"

/* initialize the structure */
void vtkParse_InitTemplateArgs(TemplateArgs *args)
{
  args->NumberOfArguments = 0;
}

void vtkParse_InitTemplateArg(TemplateArg *arg)
{
  arg->Template = NULL;
  arg->Type = 0;
  arg->Class = NULL;
  arg->Name = NULL;
  arg->Value = NULL;
}

/* initialize the structure */
void vtkParse_InitFunction(FunctionInfo *func)
{
  int i;

  func->ItemType = VTK_FUNCTION_INFO;
  func->Access = VTK_ACCESS_PUBLIC;
  func->Name = NULL;
  func->Comment = NULL;
  func->Signature = NULL;
  func->Template = NULL;
  func->NumberOfArguments = 0;
  func->ReturnValue = NULL;
  func->IsStatic = 0;
  func->IsVirtual = 0;
  func->IsPureVirtual = 0;
  func->IsOperator = 0;
  func->IsVariadic = 0;
  func->IsConst = 0;
  func->ReturnType = VTK_PARSE_VOID;
  func->ReturnClass = NULL;
  func->HaveHint = 0;
  func->HintSize = 0;
  func->IsLegacy = 0;
  func->ArrayFailure = 0;
  func->IsPublic = 0;
  func->IsProtected = 0;

  for (i = 0; i < MAX_ARGS; i++)
    {
    func->ArgTypes[i] = 0;
    func->ArgClasses[i] = 0;
    func->ArgCounts[i] = 0;
    }
}

/* initialize the structure */
void vtkParse_InitValue(ValueInfo *val)
{
  val->ItemType = VTK_VARIABLE_INFO;
  val->Access = VTK_ACCESS_PUBLIC;
  val->Name = NULL;
  val->Comment = NULL;
  val->Value = NULL;
  val->Type = 0;
  val->Class = NULL;
  val->Count = 0;
  val->NumberOfDimensions = 0;
  val->Function = NULL;
  val->IsStatic = 0;
  val->IsEnum = 0;
}

/* initialize the structure */
void vtkParse_InitEnum(EnumInfo *item)
{
  item->ItemType = VTK_ENUM_INFO;
  item->Access = VTK_ACCESS_PUBLIC;
  item->Name = NULL;
  item->Comment = NULL;
}

/* initialize the structure */
void vtkParse_InitClass(ClassInfo *cls)
{
  cls->ItemType = VTK_CLASS_INFO;
  cls->Access = VTK_ACCESS_PUBLIC;
  cls->Name = NULL;
  cls->Comment = NULL;
  cls->Template = NULL;
  cls->NumberOfSuperClasses = 0;
  cls->NumberOfItems = 0;
  cls->NumberOfFunctions = 0;
  cls->NumberOfEnums = 0;
  cls->NumberOfConstants = 0;
  cls->IsAbstract = 0;
  cls->HasDelete = 0;
}

/* initialize the structure */
void vtkParse_InitNamespace(NamespaceInfo *name_info)
{
  /* namespace info */
  name_info->ItemType = VTK_NAMESPACE_INFO;
  name_info->Access = VTK_ACCESS_PUBLIC;
  name_info->Name = NULL;
  name_info->Comment = NULL;
  name_info->NumberOfItems = 0;
  name_info->NumberOfClasses = 0;
  name_info->NumberOfFunctions = 0;
  name_info->NumberOfConstants = 0;
  name_info->NumberOfEnums = 0;
  name_info->NumberOfNamespaces = 0;
}

void vtkParse_InitFile(FileInfo *file_info)
{
  /* file info */
  file_info->FileName = NULL;
  file_info->NameComment = NULL;
  file_info->Description = NULL;
  file_info->Caveats = NULL;
  file_info->SeeAlso = NULL;

  file_info->MainClass = NULL;
  file_info->Contents = NULL;
}

void FreeNamespace(NamespaceInfo *namespace_info)
{
  /* big memory leak here, strings aren't freed */
  ClassInfo *class_info;
  FunctionInfo *func_info;
  ValueInfo *const_info;
  EnumInfo *enum_info;

  int i, j, n, m;

  n = namespace_info->NumberOfClasses;
  for (i = 0; i < n; i++)
    {
    class_info = namespace_info->Classes[i];

    m = class_info->NumberOfSuperClasses;
    if (m > 0)
      {
      free(class_info->SuperClasses);
      }

    m = class_info->NumberOfFunctions;
    for (j = 0; j < m; j++)
      {
      func_info = class_info->Functions[j];
      free(func_info);
      }
    if (m > 0)
      {
      free(class_info->Functions);
      }

    m = class_info->NumberOfConstants;
    for (j = 0; j < m; j++)
      {
      const_info = class_info->Constants[j];
      free(const_info);
      }
    if (m > 0)
      {
      free(class_info->Constants);
      }

    m = class_info->NumberOfEnums;
    for (j = 0; j < m; j++)
      {
      enum_info = class_info->Enums[j];
      free(enum_info);
      }
    if (m > 0)
      {
      free(class_info->Enums);
      }

    if (class_info->NumberOfItems > 0)
      {
      free(class_info->Items);
      }

    free(class_info);
    }

  m = namespace_info->NumberOfFunctions;
  for (j = 0; j < m; j++)
    {
    func_info = namespace_info->Functions[j];
    free(func_info);
    }
  if (m > 0)
    {
    free(namespace_info->Functions);
    }

  m = namespace_info->NumberOfConstants;
  for (j = 0; j < m; j++)
    {
    const_info = namespace_info->Constants[j];
    free(const_info);
    }
  if (m > 0)
    {
    free(namespace_info->Constants);
    }

  m = namespace_info->NumberOfEnums;
  for (j = 0; j < m; j++)
    {
    enum_info = namespace_info->Enums[j];
    free(enum_info);
    }
  if (m > 0)
    {
    free(namespace_info->Enums);
    }

  m = namespace_info->NumberOfNamespaces;
  for (i = 0; i < m; i++)
    {
    FreeNamespace(namespace_info->Namespaces[i]);
    }

  if (namespace_info->NumberOfItems > 0)
    {
    free(namespace_info->Items);
    }

  free(namespace_info);
}

/* check whether this is the class we are looking for */
void start_class(const char *classname, int is_struct)
{
  currentClass = (ClassInfo *)malloc(sizeof(ClassInfo));
  vtkParse_InitClass(currentClass);
  currentClass->Name = vtkstrdup(classname);
  if (is_struct)
    {
    currentClass->ItemType = VTK_STRUCT_INFO;
    }
  vtkParse_AddItemMacro(currentNamespace, Classes, currentClass);

  /* template information */
  if (currentTemplate)
    {
    currentClass->Template = currentTemplate;
    currentTemplate = NULL;
    }

  access_level = VTK_ACCESS_PRIVATE;
  if (is_struct)
    {
    access_level = VTK_ACCESS_PUBLIC;
    }

  vtkParse_InitFunction(currentFunction);
}

/* reject the class */
void reject_class(const char *classname, int is_struct)
{
  static ClassInfo static_class;

  currentClass = &static_class;
  currentClass->Name = vtkstrdup(classname);
  vtkParse_InitClass(currentClass);

  access_level = VTK_ACCESS_PRIVATE;
  if (is_struct)
    {
    access_level = VTK_ACCESS_PUBLIC;
    }

  vtkParse_InitFunction(currentFunction);
}

/* reached the end of a class definition */
void end_class()
{
  currentClass = NULL;
  access_level = VTK_ACCESS_PUBLIC;
}

/* start a new enum */
void start_enum(const char *name)
{
  static char text[256];
  EnumInfo *item;

  currentEnumName = NULL;
  currentEnumValue = NULL;
  if (name)
    {
    currentEnumName = text;
    strcpy(currentEnumName, name);
    item = (EnumInfo *)malloc(sizeof(EnumInfo));
    vtkParse_InitEnum(item);
    item->Name = vtkstrdup(name);
    item->Access = access_level;
    if (currentClass)
      {
      vtkParse_AddItemMacro(currentClass, Enums, item);
      }
    else
      {
      vtkParse_AddItemMacro(currentNamespace, Enums, item);
      }
    }
}

/* finish the enum */
void end_enum()
{
  currentEnumName = NULL;
  currentEnumValue = NULL;
}

/* add a constant to the enum */
void add_enum(const char *name, const char *value)
{
  static char text[256];
  int i, j;

  if (value)
    {
    currentEnumValue = text;
    strcpy(currentEnumValue, value);
    }
  else if (currentEnumValue)
    {
    i = strlen(currentEnumValue);
    while (i > 0 && currentEnumValue[i-1] >= '0' &&
           currentEnumValue[i-1] <= '9') { i--; }

    if (i == 0 || currentEnumValue[i-1] == ' ' ||
        (i > 1 && currentEnumValue[i-2] == ' ' &&
         (currentEnumValue[i-1] == '-' || currentEnumValue[i-1] == '+')))
      {
      if (i > 0 && currentEnumValue[i-1] != ' ')
        {
        i--;
        }
      j = atol(&currentEnumValue[i]);
      sprintf(&currentEnumValue[i], "%i", j+1);
      }
    else
      {
      i = strlen(currentEnumValue);
      strcpy(&currentEnumValue[i], " + 1");
      }
    }
  else
    {
    currentEnumValue = text;
    strcpy(currentEnumValue, "0");
    }

  add_constant(name, currentEnumValue, VTK_PARSE_INT, currentEnumName, 2);
}

/* add a constant to the current class or namespace */
void add_constant(const char *name, const char *value,
                  int type, const char *typeclass, int flag)
{
  ValueInfo *con = (ValueInfo *)malloc(sizeof(ValueInfo));
  vtkParse_InitValue(con);
  con->ItemType = VTK_CONSTANT_INFO;
  con->Name = vtkstrdup(name);
  con->Value = vtkstrdup(value);
  con->Type = type;
  if (typeclass)
    {
    con->Class = vtkstrdup(typeclass);
    }
  if (flag == 2)
    {
    con->IsEnum = 1;
    }

  if (flag == 1)
    {
    con->Access = VTK_ACCESS_PUBLIC;
    vtkParse_AddItemMacro(data.Contents, Constants, con);
    }
  else if (currentClass)
    {
    con->Access = access_level;
    vtkParse_AddItemMacro(currentClass, Constants, con);
    }
  else
    {
    con->Access = VTK_ACCESS_PUBLIC;
    vtkParse_AddItemMacro(currentNamespace, Constants, con);
    }
}

/* if the name is a const in this namespace, the scope it */
const char *add_const_scope(const char *name)
{
  static char text[256];
  NamespaceInfo *scope = currentNamespace;
  int i, j;
  int addscope = 0;

  strcpy(text, name);

  if (currentClass)
    {
    for (j = 0; j < currentClass->NumberOfConstants; j++)
      {
      if (strcmp(currentClass->Constants[j]->Name, text) == 0)
        {
        prepend_scope(text, currentClass->Name);
        addscope = 1;
        }
      }
    }
  i = namespaceDepth;
  while (scope && scope->Name)
    {
    if (addscope)
      {
      prepend_scope(text, scope->Name);
      }
    else
      {
      for (j = 0; j < scope->NumberOfConstants; j++)
        {
        if (strcmp(scope->Constants[j]->Name, text) == 0)
          {
          prepend_scope(text, scope->Name);
          addscope = 1;
          }
        }
      }

    scope = 0;
    if (i > 0)
      {
      scope = namespaceStack[--i];
      }
    }

  return text;
}

/* add an arg to a function */
void add_argument(FunctionInfo *func, int type,
                  const char *typeclass, int count)
{
  int i = func->NumberOfArguments;
  char text[64];
  ValueInfo *arg = (ValueInfo *)malloc(sizeof(ValueInfo));
  vtkParse_InitValue(arg);

  arg->Type = type;
  if (typeclass)
    {
    arg->Class = vtkstrdup(typeclass);
    }

  if (count)
    {
    arg->Count = count;
    sprintf(text, "%i", count);
    vtkParse_AddItemMacro2(arg, Dimensions, vtkstrdup(text));
    }

  func->ArgTypes[i] = arg->Type;
  func->ArgClasses[i] = arg->Class;
  func->ArgCounts[i] = count;

  vtkParse_AddItemMacro2(func, Arguments, arg);
}

/* set the return type for the current function */
void set_return(FunctionInfo *func, int type,
                const char *typeclass, int count)
{
  char text[64];
  ValueInfo *val = (ValueInfo *)malloc(sizeof(ValueInfo));

  vtkParse_InitValue(val);
  val->Type = type;
  if (typeclass)
    {
    val->Class = vtkstrdup(typeclass);
    }

  if (count)
    {
    val->Count = count;
    sprintf(text, "%i", count);
    vtkParse_AddItemMacro2(val, Dimensions, vtkstrdup(text));
    func->HaveHint = 1;
    }

  func->ReturnValue = val;
  func->ReturnType = val->Type;
  func->ReturnClass = val->Class;
  func->HintSize = count;
}

/* deal with types that include function pointers or arrays */
void handle_complex_type(
  ValueInfo *val, int datatype, int extra, const char *funcSig)
{
  FunctionInfo *func = 0;
  int i, n;
  char *cp;

  /* if "extra" was set, parentheses were involved */
  if ((extra & VTK_PARSE_BASE_TYPE) == VTK_PARSE_FUNCTION)
    {
    /* the current type becomes the function return type */
    func = getFunction();
    func->ReturnValue = (ValueInfo *)malloc(sizeof(ValueInfo));
    vtkParse_InitValue(func->ReturnValue);
    func->ReturnValue->Type = datatype;
    func->ReturnValue->Class = vtkstrdup(getTypeId());
    func->ReturnType = func->ReturnValue->Type;
    func->ReturnClass = func->ReturnValue->Class;
    func->Signature = vtkstrdup(funcSig);
    val->Function = func;

    /* the val type is whatever was inside the parentheses */
    clearTypeId();
    setTypeId("function");
    datatype = (extra & VTK_PARSE_UNQUALIFIED_TYPE);
    }
  else if ((extra & VTK_PARSE_INDIRECT) == VTK_PARSE_BAD_INDIRECT)
    {
    datatype = (datatype | VTK_PARSE_BAD_INDIRECT);
    }
  else if ((extra & VTK_PARSE_INDIRECT) != 0)
    {
    extra = (extra & VTK_PARSE_INDIRECT);

    if ((extra & VTK_PARSE_REF) != 0)
      {
      datatype = (datatype | VTK_PARSE_REF);
      extra = (extra & ~VTK_PARSE_REF);
      }

    if (extra != 0 && getArrayNDims() > 0)
      {
      /* pointer represents an unsized array bracket */
      datatype = add_indirection(datatype, VTK_PARSE_ARRAY);
      extra = ((extra >> 2) & VTK_PARSE_POINTER_MASK);
      }

    datatype = add_indirection(datatype, extra);
    }

  if (getArrayNDims() == 1)
    {
    if ((datatype & VTK_PARSE_POINTER_LOWMASK) != VTK_PARSE_ARRAY)
      {
      /* turn the first set of brackets into a pointer */
      if (getArrayNDims() == 1)
        {
        datatype = add_indirection(datatype, VTK_PARSE_POINTER);
        }
      }
    }
  else if (getArrayNDims() > 1)
    {
    if ((datatype & VTK_PARSE_POINTER_LOWMASK) != VTK_PARSE_ARRAY)
      {
      /* turn the first set of brackets into a pointer */
      datatype = add_indirection(datatype, VTK_PARSE_ARRAY);
      }
    else
      {
      pushArrayFront("");
      }
    }

  /* get the data type */
  val->Type = datatype;
  val->Class = vtkstrdup(getTypeId());

  /* copy contents of all brackets to the ArgDimensions */
  val->NumberOfDimensions = getArrayNDims();
  val->Dimensions = getArray();

  /* count is the product of the dimensions */
  val->Count = 0;
  if (val->NumberOfDimensions)
    {
    val->Count = 1;
    for (i = 0; i < val->NumberOfDimensions; i++)
      {
      n = 0;
      cp = val->Dimensions[i];
      if (cp[0] != '\0')
        {
        while (*cp != '\0' && *cp >= '0' && *cp <= '9') { cp++; }
        if (*cp == '\0')
          {
          n = (int)atol(val->Dimensions[i]);
          }
        }
      val->Count *= n;
      }
    }
}

/* specifically handle a VAR_FUNCTION argument */
void handle_function_type(
  ValueInfo *arg, const char *name, const char *funcSig)
{
  FunctionInfo *func;
  int j;

  arg->Type = VTK_PARSE_FUNCTION;
  arg->Class = vtkstrdup("function");

  if (name && name[0] != '\0')
    {
    arg->Name = vtkstrdup(name);
    }

  func = (FunctionInfo *)malloc(sizeof(FunctionInfo));
  vtkParse_InitFunction(func);
  add_argument(func, VTK_PARSE_VOID_PTR, "void", 0);
  set_return(func, VTK_PARSE_VOID, "void", 0);
  func->Signature = vtkstrdup(funcSig);
  j = strlen(func->Signature);
  while (j > 0 && func->Signature[j-1] == ' ')
    {
    func->Signature[j-1] = '\0';
    }

  arg->Function = func;
}


/* reject the function, do not output it */
void reject_function()
{
  vtkParse_InitFunction(currentFunction);
}

/* a simple routine that updates a few variables */
void output_function()
{
  int i, j, match;

  /* reject template specializations */
  if (currentFunction->Name[strlen(currentFunction->Name)-1] == '>')
    {
    reject_function();
    return;
    }

  /* static */
  if (currentFunction->ReturnType & VTK_PARSE_STATIC)
    {
    currentFunction->IsStatic = 1;
    }

  /* template information */
  if (currentTemplate)
    {
    currentFunction->Template = currentTemplate;
    currentTemplate = NULL;
    }

  /* a void argument is the same as no arguments */
  if (currentFunction->NumberOfArguments == 1 &&
      (currentFunction->Arguments[0]->Type & VTK_PARSE_UNQUALIFIED_TYPE) ==
      VTK_PARSE_VOID)
    {
    currentFunction->NumberOfArguments = 0;
    }

  /* if return type is void, set return class to void */
  if (currentFunction->ReturnClass == NULL &&
      (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE) ==
       VTK_PARSE_VOID)
    {
    currentFunction->ReturnClass = vtkstrdup("void");
    }

  /* set public, protected */
  if (currentClass)
    {
    currentFunction->Access = access_level;
    /* set legacy flags */
    currentFunction->IsPublic = (access_level == VTK_ACCESS_PUBLIC);
    currentFunction->IsProtected = (access_level == VTK_ACCESS_PROTECTED);
    }
  else
    {
    currentFunction->Access = VTK_ACCESS_PUBLIC;
    /* set legacy flags */
    currentFunction->IsPublic = 1;
    currentFunction->IsProtected = 0;
    }

  /* look for legacy VAR FUNCTIONS */
  if (currentFunction->NumberOfArguments
      && (currentFunction->Arguments[0]->Type == VTK_PARSE_FUNCTION))
    {
    if (currentFunction->NumberOfArguments == 2)
      {
      currentFunction->NumberOfArguments = 1;
      }
    else
      {
      currentFunction->ArrayFailure = 1;
      }
    }

  /* check for too many arguments */
  if (currentFunction->NumberOfArguments > MAX_ARGS)
    {
    currentFunction->ArrayFailure = 1;
    }

  /* also legacy: tell old wrappers that multi-dimensional arrays are bad */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    ValueInfo *arg = currentFunction->Arguments[i];
    if ((arg->Type & VTK_PARSE_POINTER_MASK) != 0)
      {
      if (((arg->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_FUNCTION) ||
          ((arg->Type & VTK_PARSE_INDIRECT) == VTK_PARSE_BAD_INDIRECT) ||
          ((arg->Type & VTK_PARSE_POINTER_LOWMASK) != VTK_PARSE_POINTER))
       {
       currentFunction->ArrayFailure = 1;
       }
      }
    }

  if (currentClass)
    {
    /* is it a delete function */
    if (currentFunction->Name && !strcmp("Delete",currentFunction->Name))
      {
      currentClass->HasDelete = 1;
      }

    vtkParse_AddItemMacro(currentClass, Functions, currentFunction);

    currentFunction = (FunctionInfo *)malloc(sizeof(FunctionInfo));
    }
  else
    {
    /* make sure this function isn't a repeat */
    match = 0;
    for (i = 0; i < currentNamespace->NumberOfFunctions; i++)
      {
      if (currentNamespace->Functions[i]->Name &&
          strcmp(currentNamespace->Functions[i]->Name,
                 currentFunction->Name) == 0)
        {
        if (currentNamespace->Functions[i]->NumberOfArguments ==
            currentFunction->NumberOfArguments)
          {
          for (j = 0; j < currentFunction->NumberOfArguments; j++)
            {
            if (currentNamespace->Functions[i]->Arguments[j]->Type ==
                currentFunction->Arguments[j]->Type)
              {
              if (currentFunction->Arguments[j]->Type == VTK_PARSE_OBJECT &&
                  strcmp(currentNamespace->Functions[i]->Arguments[j]->Class,
                         currentFunction->Arguments[j]->Class) == 0)
                {
                break;
                }
              }
            }
          if (j == currentFunction->NumberOfArguments)
            {
            match = 1;
            break;
            }
          }
        }
      }

    if (!match)
      {
      vtkParse_AddItemMacro(currentNamespace, Functions, currentFunction);

      currentFunction = (FunctionInfo *)malloc(sizeof(FunctionInfo));
      }
    }

  vtkParse_InitFunction(currentFunction);
}

void outputSetVectorMacro(
  const char *var, int argType, const char *typeText, int n)
{
  char *local = vtkstrdup(typeText);
  char *name;
  int i;

  sprintf(temps,"Set%s", var);
  name = vtkstrdup(temps);

  sprintf(currentFunction->Signature, "void Set%s(%s", var, local);
  for (i = 1; i < n; i++)
    {
    postSig(", ");
    postSig(local);
    }
  postSig(");");
  currentFunction->Name = name;
  for (i = 0; i < n; i++)
    {
    add_argument(currentFunction, argType, getTypeId(), 0);
    }
  set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
  output_function();

  currentFunction->Signature = (char *)malloc(2048);
  sigAllocatedLength = 2048;
  sprintf(currentFunction->Signature, "void Set%s(%s a[%i]);",
          var, local, n);
  currentFunction->Name = name;
  add_argument(currentFunction, (VTK_PARSE_POINTER | argType),
               getTypeId(), n);
  set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
  output_function();

  free(local);
}

void outputGetVectorMacro(
  const char *var, int argType, const char *typeText, int n)
{
  char *local = vtkstrdup(typeText);

  sprintf(currentFunction->Signature, "%s *Get%s();", local, var);
  sprintf(temps, "Get%s", var);
  currentFunction->Name = vtkstrdup(temps);
  set_return(currentFunction, (VTK_PARSE_POINTER | argType), getTypeId(), n);
  output_function();

  free(local);
}

/* Parse a header file and return a FileInfo struct */
void vtkParse_AddPointerToArray(void *valueArray, int *count, void *value)
{
  void **values = *(void ***)valueArray;
  int n = *count;

  /* if empty, alloc for the first time */
  if (n == 0)
    {
    values = (void **)malloc(1*sizeof(void*));
    }
  /* if count is power of two, reallocate with double size */
  else if ((n & (n-1)) == 0)
    {
    values = (void **)realloc(values, (n << 1)*sizeof(void*));
    }

  values[n++] = value;
  *count = n;
  *(void ***)valueArray = values;
}

/* Parse a header file and return a FileInfo struct */
FileInfo *vtkParse_ParseFile(
  const char *filename, int concrete, FILE *ifile, FILE *errfile)
{
  int i, j, lineno;
  int ret;
  FileInfo *file_info;
  char *main_class;

  vtkParse_InitFile(&data);

  data.FileName = vtkstrdup(filename);
  is_concrete = concrete;

  CommentState = 0;

  namespaceDepth = 0;
  currentNamespace = (NamespaceInfo *)malloc(sizeof(NamespaceInfo));
  vtkParse_InitNamespace(currentNamespace);
  data.Contents = currentNamespace;

  templateDepth = 0;
  currentTemplate = NULL;

  currentFunction = (FunctionInfo *)malloc(sizeof(FunctionInfo));
  vtkParse_InitFunction(currentFunction);

  parseDebug = 0;
  if (getenv("DEBUG") != NULL)
    {
    parseDebug = 1;
    }

  yyset_in(ifile);
  yyset_out(errfile);
  ret = yyparse();
  lineno = yyget_lineno();
  yylex_destroy();

  free(currentFunction);

  if (ret)
    {
    fprintf(errfile,
            "*** SYNTAX ERROR found in parsing the header file %s "
            "before line %d ***\n",
            filename, lineno);
    return NULL;
    }

  /* The main class name should match the file name */
  i = strlen(filename);
  j = i;
  while (i > 0)
    {
    --i;
    if (filename[i] == '.')
      {
      j = i;
      }
    if (filename[i] == '/' || filename[i] == '\\')
      {
      i++;
      break;
      }
    }
  main_class = (char *)malloc(j-i+1);
  strncpy(main_class, &filename[i], j-i);
  main_class[j-i] = '\0';

  /* special treatment of the main class in the file */
  for (i = 0; i < currentNamespace->NumberOfClasses; i++)
    {
    if (strcmp(currentNamespace->Classes[i]->Name, main_class) == 0)
      {
      data.MainClass = currentNamespace->Classes[i];
      break;
      }
    }

  free(main_class);

  file_info = (FileInfo *)malloc(sizeof(FileInfo));
  memcpy(file_info, &data, sizeof(FileInfo));

  return file_info;
}

/* Read a hints file and update the FileInfo */
int vtkParse_ReadHints(FileInfo *file_info, FILE *hfile, FILE *errfile)
{
  char h_cls[80];
  char h_func[80];
  unsigned int h_type;
  int  h_value;
  FunctionInfo *func_info;
  ClassInfo *class_info;
  NamespaceInfo *contents;
  int i, j, n;
  int lineno = 0;

  contents = file_info->Contents;

  /* read each hint line in succession */
  while ((n = fscanf(hfile,"%s %s %x %i", h_cls, h_func, &h_type, &h_value))
         != EOF)
    {
    lineno++;
    if (n < 4)
      {
      fprintf(errfile, "Wrapping: error parsing hints file line %i\n", lineno);
      exit(1);
      }

    /* erase "ref" and qualifiers from hint type */
    h_type = ((h_type & VTK_PARSE_BASE_TYPE) |
              (h_type & VTK_PARSE_POINTER_LOWMASK));

    /* find the matching class */
    for (i = 0; i < contents->NumberOfClasses; i++)
      {
      class_info = contents->Classes[i];

      if (strcmp(h_cls, class_info->Name) == 0)
        {
        /* find the matching function */
        for (j = 0; j < class_info->NumberOfFunctions; j++)
          {
          func_info = class_info->Functions[j];

          if (func_info->HaveHint == 0 && func_info->Name &&
              (strcmp(h_func, func_info->Name) == 0) &&
              ((int)h_type == ((func_info->ReturnType & ~VTK_PARSE_REF) &
                               VTK_PARSE_UNQUALIFIED_TYPE)))
            {
            /* types that hints are accepted for */
            switch (func_info->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE)
              {
              case VTK_PARSE_FLOAT_PTR:
              case VTK_PARSE_VOID_PTR:
              case VTK_PARSE_DOUBLE_PTR:
              case VTK_PARSE_ID_TYPE_PTR:
              case VTK_PARSE_LONG_LONG_PTR:
              case VTK_PARSE_UNSIGNED_LONG_LONG_PTR:
              case VTK_PARSE___INT64_PTR:
              case VTK_PARSE_UNSIGNED___INT64_PTR:
              case VTK_PARSE_INT_PTR:
              case VTK_PARSE_UNSIGNED_INT_PTR:
              case VTK_PARSE_SHORT_PTR:
              case VTK_PARSE_UNSIGNED_SHORT_PTR:
              case VTK_PARSE_LONG_PTR:
              case VTK_PARSE_UNSIGNED_LONG_PTR:
              case VTK_PARSE_SIGNED_CHAR_PTR:
              case VTK_PARSE_UNSIGNED_CHAR_PTR:
                {
                if (func_info->ReturnValue &&
                    func_info->ReturnValue->NumberOfDimensions == 0)
                  {
                  char text[64];
                  func_info->HaveHint = 1;
                  func_info->HintSize = h_value;
                  func_info->ReturnValue->Count = h_value;
                  sprintf(text, "%i", h_value);
                  vtkParse_AddItemMacro2(func_info->ReturnValue,
                                         Dimensions, vtkstrdup(text));
                  }
                break;
                }
              default:
                {
                fprintf(errfile,
                        "Wrapping: unhandled hint type %#x\n", h_type);
                }
              }
            }
          }
        }
      }
    }

  return 1;
}

/* Free the FileInfo struct returned by vtkParse_ParseFile() */
void vtkParse_Free(FileInfo *file_info)
{
  FreeNamespace(file_info->Contents);
  file_info->Contents = NULL;
}

