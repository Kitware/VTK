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

/*
The purpose of this parser is to read C++ header files in order to
generate data structures that describe the C++ interface of a library,
one header file at a time.  As such, it is not a complete C++ parser.
It only parses what is relevant to the interface and skips the rest.

While the parser reads method definitions, type definitions, and
template definitions it generates a "signature" which is a string
that matches (apart from whitespace) the text that was parsed.

While parsing types, the parser creates an unsigned int that describes
the type as well as creating other data structures for arrays, function
pointers, etc.  The parse also creates a typeId string, which is either
a simple id that gives the class name or type name, or is "function" for
function pointer types, or "method" for method pointer types.
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

/* Map from the type anonymous_enumeration in vtkType.h to the
   VTK wrapping type system number for the type. */

#include "vtkParse.h"
#include "vtkParsePreprocess.h"
#include "vtkParseString.h"
#include "vtkParseData.h"
#include "vtkType.h"

static unsigned int vtkParseTypeMap[] =
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

/* the "preprocessor" */
PreprocessInfo preprocessor = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/* global variables */
FileInfo      *data;

int            NumberOfConcreteClasses = 0;
const char   **ConcreteClasses;

NamespaceInfo *currentNamespace = NULL;
ClassInfo     *currentClass = NULL;
FunctionInfo  *currentFunction = NULL;
TemplateArgs  *currentTemplate = NULL;

const char    *currentEnumName = 0;
const char    *currentEnumValue = 0;

int            parseDebug;
parse_access_t access_level = VTK_ACCESS_PUBLIC;
int            IgnoreBTX = 0;

/* functions from vtkParse.l */
void print_parser_error(const char *text, const char *cp, size_t n);

/* helper functions */
void start_class(const char *classname, int is_struct_or_union);
void reject_class(const char *classname, int is_struct_or_union);
void end_class();
void add_base_class(ClassInfo *cls, const char *name, int access_lev,
                    int is_virtual);
void output_friend_function(void);
void output_function(void);
void reject_function(void);
void set_return(FunctionInfo *func, unsigned int type,
                const char *typeclass, int count);
void add_argument(FunctionInfo *func, unsigned int type,
                  const char *classname, int count);
void add_template_arg(unsigned int datatype,
                      unsigned int extra, const char *funcSig);
void add_using(const char *name, int is_namespace);
void start_enum(const char *enumname);
void add_enum(const char *name, const char *value);
void end_enum();
unsigned int guess_constant_type(const char *value);
void add_constant(const char *name, const char *value,
                  unsigned int type, const char *typeclass, int global);
const char *add_const_scope(const char *name);
void prepend_scope(char *cp, const char *arg);
unsigned int add_indirection(unsigned int tval, unsigned int ptr);
unsigned int add_indirection_to_array(unsigned int ptr);
void handle_complex_type(ValueInfo *val, unsigned int datatype,
                         unsigned int extra, const char *funcSig);
void handle_function_type(ValueInfo *arg, const char *name,
                          const char *funcSig);
void outputSetVectorMacro(const char *var, unsigned int argType,
                          const char *typeText, int n);
void outputGetVectorMacro(const char *var, unsigned int argType,
                          const char *typeText, int n);

/*----------------------------------------------------------------
 * String utility methods
 *
 * Strings are centrally allocated and are const, and they are not
 * freed until the program exits.  If they need to be freed before
 * then, vtkParse_FreeStrings() can be called.
 */

/* duplicate the first n bytes of a string and terminate */
static const char *vtkstrndup(const char *in, size_t n)
{
  char *res = NULL;

  res = vtkParse_NewString(n);
  strncpy(res, in, n);
  res[n] = '\0';

  return res;
}

/* duplicate a string */
static const char *vtkstrdup(const char *in)
{
  if (in)
    {
    return vtkstrndup(in, strlen(in));
    }

  return in;
}

/* helper function for concatenating strings */
static const char *vtkstrncat(size_t n, const char **str)
{
  char *cp;
  size_t i;
  size_t j[8];
  size_t m = 0;

  for (i = 0; i < n; i++)
    {
    j[i] = 0;
    if (str[i])
      {
      j[i] = strlen(str[i]);
      m += j[i];
      }
    }
  cp = vtkParse_NewString(m);
  m = 0;
  for (i = 0; i < n; i++)
    {
    if (j[i])
      {
      strncpy(&cp[m], str[i], j[i]);
      m += j[i];
      }
    }
  cp[m] = '\0';

  return cp;
}

/* concatenate strings */
static const char *vtkstrcat(const char *str1, const char *str2)
{
  const char *cp[2];

  cp[0] = str1;
  cp[1] = str2;
  return vtkstrncat(2, cp);
}

static const char *vtkstrcat3(const char *str1, const char *str2,
                              const char *str3)
{
  const char *cp[3];

  cp[0] = str1;
  cp[1] = str2;
  cp[2] = str3;
  return vtkstrncat(3, cp);
}

static const char *vtkstrcat4(const char *str1, const char *str2,
                              const char *str3, const char *str4)
{
  const char *cp[4];

  cp[0] = str1;
  cp[1] = str2;
  cp[2] = str3;
  cp[3] = str4;
  return vtkstrncat(4, cp);
}

static const char *vtkstrcat5(const char *str1, const char *str2,
                              const char *str3, const char *str4,
                              const char *str5)
{
  const char *cp[5];

  cp[0] = str1;
  cp[1] = str2;
  cp[2] = str3;
  cp[3] = str4;
  cp[4] = str5;
  return vtkstrncat(5, cp);
}

static const char *vtkstrcat7(const char *str1, const char *str2,
                              const char *str3, const char *str4,
                              const char *str5, const char *str6,
                              const char *str7)
{
  const char *cp[7];

  cp[0] = str1;
  cp[1] = str2;
  cp[2] = str3;
  cp[3] = str4;
  cp[4] = str5;
  cp[5] = str6;
  cp[6] = str7;
  return vtkstrncat(7, cp);
}

/*----------------------------------------------------------------
 * Comments
 */

/* "private" variables */
char          *commentText = NULL;
size_t         commentLength = 0;
size_t         commentAllocatedLength = 0;
int            commentState = 0;

const char *getComment()
{
  if (commentState != 0)
    {
    return commentText;
    }
  return NULL;
}

void clearComment()
{
  commentLength = 0;
  if (commentText)
    {
    commentText[commentLength] = '\0';
    }
  commentState = 0;
}

void addCommentLine(const char *line, size_t n)
{
  if (commentState <= 0)
    {
    clearComment();
    return;
    }

  if (commentText == NULL)
    {
    commentAllocatedLength = n+80;
    commentText = (char *)malloc(commentAllocatedLength);
    commentLength = 0;
    commentText[0] = '\0';
    }
  else if (commentLength + n + 2 > commentAllocatedLength)
    {
    commentAllocatedLength = commentAllocatedLength + commentLength + n + 2;
    commentText = (char *)realloc(commentText, commentAllocatedLength);
    }

  if (n > 0)
    {
    memcpy(&commentText[commentLength], line, n);
    }
  commentLength += n;
  commentText[commentLength++] = '\n';
  commentText[commentLength] = '\0';
}

void closeComment()
{
  switch (commentState)
    {
    case 1:
      /* Make comment persist until a new comment starts */
      commentState = -1;
      break;
    case 2:
      data->Description = vtkstrdup(getComment());
      clearComment();
      break;
    case 3:
      data->SeeAlso = vtkstrdup(getComment());
      clearComment();
      break;
    case 4:
      data->Caveats = vtkstrdup(getComment());
      clearComment();
      break;
    }
}

void closeOrClearComment()
{
  if (commentState < 0)
    {
    clearComment();
    }
  else
    {
    closeComment();
    }
}

void setCommentState(int state)
{
  switch (state)
    {
    case 0:
      closeComment();
      break;
    default:
      closeComment();
      clearComment();
      break;
    }

  commentState = state;
}


/*----------------------------------------------------------------
 * Macros
 */

/* "private" variables */
const char *macroName = NULL;
int macroUsed = 0;
int macroEnded = 0;

const char *getMacro()
{
  if (macroUsed == 0)
    {
    macroUsed = macroEnded;
    return macroName;
    }
  return NULL;
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
    vtkParse_AddNamespaceToNamespace(oldNamespace, currentNamespace);
    }

  namespaceStack[namespaceDepth++] = oldNamespace;
}

/* leave the namespace */
void popNamespace()
{
  currentNamespace = namespaceStack[--namespaceDepth];
}


/*----------------------------------------------------------------
 * Classes
 *
 * operates on: currentClass, access_level
 */

/* "private" variables */
ClassInfo *classStack[10];
parse_access_t classAccessStack[10];
int classDepth = 0;

/* start an internal class definition */
void pushClass()
{
  classAccessStack[classDepth] = access_level;
  classStack[classDepth++] = currentClass;
}

/* leave the internal class */
void popClass()
{
  currentClass = classStack[--classDepth];
  access_level = classAccessStack[classDepth];
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
int sigClosed = 0;
size_t sigMark[10];
size_t sigLength = 0;
size_t sigAllocatedLength = 0;
int sigMarkDepth = 0;
char *signature = NULL;

/* start a new signature */
void startSig()
{
  signature = NULL;
  sigLength = 0;
  sigAllocatedLength = 0;
  sigClosed = 0;
  sigMarkDepth = 0;
  sigMark[0] = 0;
}

/* get the signature */
const char *getSig()
{
  return signature;
}

/* get the signature length */
size_t getSigLength()
{
  return sigLength;
}

/* reset the sig to the specified length */
void resetSig(size_t n)
{
  if (n < sigLength)
    {
    sigLength = n;
    }
}

/* reallocate Signature if n chars cannot be appended */
void checkSigSize(size_t n)
{
  const char *ccp;

  if (sigAllocatedLength == 0)
    {
    sigLength = 0;
    sigAllocatedLength = 80 + n;
    signature = vtkParse_NewString(sigAllocatedLength);
    signature[0] = '\0';
    }
  else if (sigLength + n > sigAllocatedLength)
    {
    sigAllocatedLength += sigLength + n;
    ccp = signature;
    signature = vtkParse_NewString(sigAllocatedLength);
    strncpy(signature, ccp, sigLength);
    signature[sigLength] = '\0';
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
  if (!sigClosed)
    {
    size_t n = strlen(arg);
    checkSigSize(n);
    if (n > 0)
      {
      memmove(&signature[n], signature, sigLength);
      strncpy(signature, arg, n);
      sigLength += n;
      }
    signature[sigLength] = '\0';
    }
}

/* append text to the end of the signature */
void postSig(const char *arg)
{
  if (!sigClosed)
    {
    size_t n = strlen(arg);
    checkSigSize(n);
    if (n > 0)
      {
      strncpy(&signature[sigLength], arg, n);
      sigLength += n;
      }
    signature[sigLength] = '\0';
    }
}

/* set a mark in the signature for later operations */
void markSig()
{
  sigMark[sigMarkDepth] = 0;
  if (signature)
    {
    sigMark[sigMarkDepth] = sigLength;
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
  if (signature)
    {
    cp = &signature[sigMark[sigMarkDepth]];
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
  if (signature && sigMark[sigMarkDepth] > 0)
    {
    size_t i, m, n, nn;
    char c;
    char *cp;
    cp = signature;
    n = sigLength;
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
  if (signature)
    {
    size_t n = sigLength;
    if (n > 0 && signature[n-1] == ' ')
      {
      signature[n-1] = '\0';
      sigLength--;
      }
    }
}

/*----------------------------------------------------------------
 * Storage type for vars and functions
 */

/* "private" variables */
unsigned int storageType = 0;

/* clear the storage type */
void clearStorageType()
{
  storageType = 0;
}

/* save the storage type */
void setStorageType(unsigned int val)
{
  storageType = val;
}

/* modify the indirection (pointers, refs) in the storage type */
void setStorageTypeIndirection(unsigned int ind)
{
  storageType = (storageType & ~VTK_PARSE_INDIRECT);
  ind = (ind & VTK_PARSE_INDIRECT);
  storageType = (storageType | ind);
}

/* retrieve the storage type */
unsigned int getStorageType()
{
  return storageType;
}

/*----------------------------------------------------------------
 * Array information
 */

/* "private" variables */
int numberOfDimensions = 0;
const char **arrayDimensions = NULL;

/* clear the array counter */
void clearArray(void)
{
  numberOfDimensions = 0;
  arrayDimensions = NULL;
}

/* add another dimension */
void pushArraySize(const char *size)
{
  vtkParse_AddStringToArray(&arrayDimensions, &numberOfDimensions,
                            vtkstrdup(size));
}

/* add another dimension to the front */
void pushArrayFront(const char *size)
{
  int i;

  vtkParse_AddStringToArray(&arrayDimensions, &numberOfDimensions, 0);

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
const char **getArray()
{
  if (numberOfDimensions > 0)
    {
    return arrayDimensions;
    }
  return NULL;
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
 * Specifically for function pointers, the scope (i.e. class) that
 * the function is a method of.
 */

const char *pointerScopeStack[10];
int pointerScopeDepth = 0;

/* save the scope for scoped method pointers */
void scopeSig(const char *scope)
{
  if (scope && scope[0] != '\0')
    {
    postSig(scope);
    }
  else
    {
    scope = NULL;
    }
  pointerScopeStack[pointerScopeDepth++] = vtkstrdup(scope);
}

/* get the scope back */
const char *getScope()
{
  return pointerScopeStack[--pointerScopeDepth];
}

/*----------------------------------------------------------------
 * Function stack
 *
 * operates on: currentFunction
 */

/* "private" variables */
FunctionInfo *functionStack[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const char *functionVarNameStack[10];
const char *functionTypeIdStack[10];
int functionDepth = 0;

void pushFunction()
{
  functionStack[functionDepth] = currentFunction;
  currentFunction = (FunctionInfo *)malloc(sizeof(FunctionInfo));
  vtkParse_InitFunction(currentFunction);
  if (!functionStack[functionDepth])
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
  clearVarName();
  if (functionVarNameStack[functionDepth])
    {
    setVarName(functionVarNameStack[functionDepth]);
    }
  clearTypeId();
  if (functionTypeIdStack[functionDepth])
    {
    setTypeId(functionTypeIdStack[functionDepth]);
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
  size_t i, j, m, n;
  int depth;

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

  for (j = m; j > i; j--)
    {
    cp[j+n+1] = cp[j-1];
    }
  for (j = 0; j < n; j++)
    {
    cp[j+i] = arg[j];
    }
  cp[n+i] = ':'; cp[n+i+1] = ':';
  cp[m+n+2] = '\0';
}

/* expand a type by including pointers from another */
unsigned int add_indirection(unsigned int type1, unsigned int type2)
{
  unsigned int ptr1 = (type1 & VTK_PARSE_POINTER_MASK);
  unsigned int ptr2 = (type2 & VTK_PARSE_POINTER_MASK);
  unsigned int reverse = 0;
  unsigned int result;

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

/* There is only one array, so add any parenthetical indirection to it */
unsigned int add_indirection_to_array(unsigned int type)
{
  unsigned int ptrs = (type & VTK_PARSE_POINTER_MASK);
  unsigned int result = (type & ~VTK_PARSE_POINTER_MASK);
  unsigned int reverse = 0;

  if ((type & VTK_PARSE_INDIRECT) == VTK_PARSE_BAD_INDIRECT)
    {
    return (result | VTK_PARSE_BAD_INDIRECT);
    }

  while (ptrs)
    {
    reverse = ((reverse << 2) | (ptrs & VTK_PARSE_POINTER_LOWMASK));
    ptrs = ((ptrs >> 2) & VTK_PARSE_POINTER_MASK);
    }

  /* I know the reversal makes no difference, but it is still
   * nice to add the pointers in the correct order, just in
   * case const pointers are thrown into the mix. */
  while (reverse)
    {
    pushArrayFront("");
    reverse = ((reverse >> 2) & VTK_PARSE_POINTER_MASK);
    }

  return result;
}

%}
/*----------------------------------------------------------------
 * Start of yacc section
 */


/* The parser will shift/reduce values <str> or <integer>, where
   <str> is for IDs and <integer> is for types, modifiers, etc. */

%union{
  const char   *str;
  unsigned int  integer;
}

/* Lexical tokens defined in vtkParse.l */

/* various tokens that provide ID strings */
%token <str> ID
%token <str> VTK_ID
%token <str> QT_ID
%token <str> VAR_FUNCTION
%token <str> StdString
%token <str> UnicodeString
%token <str> OSTREAM
%token <str> ISTREAM

/* LP = "(*" or "(name::*"    and     LA = "(&" or "(name::&"
   These evaluate to an empty string or to "name::" as a string.
   Without these, the rules for declaring function pointers will
   produce errors because the parser cannot unambiguously choose
   between evaluating ID tokens as names via simple_id, versus
   evaluating ID tokens as types via type_id.  This construct forces
   the parser to evaluate the ID as a name, not as a type. */
%token <str> LP
%token <str> LA

/* literal tokens are provided as strings */
%token <str> STRING_LITERAL
%token <str> INT_LITERAL
%token <str> HEX_LITERAL
%token <str> OCT_LITERAL
%token <str> FLOAT_LITERAL
%token <str> CHAR_LITERAL
%token <str> ZERO

/* keywords (many unused keywords have been omitted) */
%token STRUCT
%token CLASS
%token UNION
%token ENUM
%token PUBLIC
%token PRIVATE
%token PROTECTED
%token CONST
%token MUTABLE
%token STATIC
%token VIRTUAL
%token EXPLICIT
%token INLINE
%token FRIEND
%token EXTERN
%token OPERATOR
%token TEMPLATE
%token THROW
%token TYPENAME
%token TYPEDEF
%token NAMESPACE
%token USING
%token NEW
%token DELETE
%token STATIC_CAST
%token DYNAMIC_CAST
%token CONST_CAST
%token REINTERPRET_CAST

/* operators */
%token OP_LSHIFT_EQ
%token OP_RSHIFT_EQ
%token OP_LSHIFT
%token OP_RSHIFT
%token OP_DOT_POINTER
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
%token OP_LOGIC_AND
%token OP_LOGIC_OR
%token OP_LOGIC_EQ
%token OP_LOGIC_NEQ
%token OP_LOGIC_LEQ
%token OP_LOGIC_GEQ
%token ELLIPSIS
%token DOUBLE_COLON

/* unrecognized character */
%token OTHER

/* types */
%token VOID
%token BOOL
%token FLOAT
%token DOUBLE
%token LONG_DOUBLE
%token INT
%token UNSIGNED_INT
%token SHORT
%token UNSIGNED_SHORT
%token LONG
%token UNSIGNED_LONG
%token LONG_LONG
%token UNSIGNED_LONG_LONG
%token INT64__
%token UNSIGNED_INT64__
%token CHAR
%token SIGNED_CHAR
%token UNSIGNED_CHAR
%token SIGNED
%token UNSIGNED

/* typedef types */
%token SSIZE_T
%token SIZE_T

/* VTK typedef types */
%token IdType
%token FloatType
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

/* VTK macros */
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

/* VTK special tokens */
%token VTK_BYTE_SWAP_DECL

%%
/*
 * Here is the start of the grammar
 */

strt:
  | strt
    {
      startSig();
      clearStorageType();
      clearTypeId();
      clearTemplate();
      closeComment();
    }
    file_item;

file_item:
    class_forward_decl
  | using
  | namespace
  | extern_section
  | type_def
  | variables
  | template_variable_initialization
  | enum_def maybe_variables ';'
  | class_def maybe_variables ';'
  | template class_def maybe_variables ';'
  | function func_body { output_function(); }
  | operator func_body { output_function(); }
  | template function func_body { output_function(); }
  | template operator func_body { output_function(); }
  | scoped_operator func_body { reject_function(); }
  | scoped_method func_body { reject_function(); }
  | template scoped_operator func_body { reject_function(); }
  | template scoped_method func_body { reject_function(); }
  | macro
  | any_id ';'
  | ';';


/*
 * extern section is parsed, but "extern" is ignored
 */

extern_section:
    EXTERN STRING_LITERAL '{' strt '}';

/*
 * Namespace is pushed and its body is parsed
 */

namespace:
    NAMESPACE class_id { pushNamespace($<str>2); }
    '{' strt '}' { popNamespace(); }
  | NAMESPACE '{' maybe_other '}';

/*
 * Class definitions
 */

class_forward_decl:
    CLASS any_id ';'
  | STRUCT any_id ';'
  | UNION any_id ';'
  | FRIEND CLASS any_id ';'
  | FRIEND STRUCT any_id ';'
  | FRIEND UNION any_id ';'
  | template CLASS any_id ';'
  | template STRUCT any_id ';'
  | template UNION any_id ';'
  | FRIEND template CLASS any_id ';'
  | FRIEND template STRUCT any_id ';'
  | FRIEND template UNION any_id ';';

class_def:
    CLASS any_id { start_class($<str>2, 0); }
    maybe_bases '{' class_def_body '}' { end_class(); }
  | STRUCT any_id { start_class($<str>2, 1); }
    maybe_bases '{' class_def_body '}' { end_class(); }
  | STRUCT '{' maybe_other '}'
  | UNION any_id { start_class($<str>2, 2); }
    maybe_bases '{' class_def_body '}' { end_class(); }
  | UNION '{' maybe_other '}';

class_def_body:
  | class_def_body
    {
      startSig();
      clearStorageType();
      clearTypeId();
      clearTemplate();
      closeComment();
    }
    class_def_item
  | class_def_body class_access ':';

class_access:
    PUBLIC { access_level = VTK_ACCESS_PUBLIC; }
  | PRIVATE { access_level = VTK_ACCESS_PRIVATE; }
  | PROTECTED { access_level = VTK_ACCESS_PROTECTED; };

class_def_item:
    class_forward_decl
  | using
  | type_def
  | variables
  | enum_def maybe_variables ';'
  | class_def maybe_variables ';'
  | template class_def maybe_variables ';'
  | FRIEND internal_class
  | FRIEND template_internal_class
  | FRIEND method func_body { output_friend_function(); }
  | method func_body { output_function(); }
  | template method func_body { output_function(); }
  | macro
  | VTK_BYTE_SWAP_DECL parens
  | any_id ';'
  | scope OPERATOR op_token ';'
  | ';';

maybe_bases:
  | ':' base_list;

base_list:
    base_list_item
  | base_list_item ',' base_list;

base_list_item:
    any_id
    { add_base_class(currentClass, $<str>1, access_level, 0); }
  | base_virtual base_access_opt any_id
    { add_base_class(currentClass, $<str>3, $<integer>2, $<integer>1); }
  | base_access base_virtual_opt any_id
    { add_base_class(currentClass, $<str>3, $<integer>1, $<integer>2); };

base_virtual:
    VIRTUAL { $<integer>$ = VTK_PARSE_VIRTUAL; };

base_virtual_opt:
    { $<integer>$ = 0; }
  | base_virtual { $<integer>$ = $<integer>1; };

base_access:
    PUBLIC { $<integer>$ = VTK_ACCESS_PUBLIC; }
  | PRIVATE { $<integer>$ = VTK_ACCESS_PRIVATE; }
  | PROTECTED { $<integer>$ = VTK_ACCESS_PROTECTED; };

base_access_opt:
    { $<integer>$ = access_level; }
  | base_access { $<integer>$ = $<integer>1; };

/*
 * Enums
 *
 * The values assigned to enum constants are handled as strings.
 * The text can be dropped into the generated .cxx file and evaluated there,
 * as long as all IDs are properly scoped.
 */

enum_def:
    ENUM class_id { start_enum($<str>2); } '{' enum_list '}' { end_enum(); }
  | ENUM { start_enum(NULL); } '{' enum_list '}' { end_enum(); };

enum_list:
  | enum_item
  | enum_item ',' enum_list;

enum_item:
    simple_id { add_enum($<str>1, NULL); }
  | simple_id '=' { postSig("="); markSig(); }
    const_expr { chopSig(); add_enum($<str>1, copySig()); };

/*
 * currently ignored items
 */

template_variable_initialization:
    template storage_type scope simple_id '=' maybe_other_no_semi ';';

template_internal_class:
    template internal_class;

internal_class:
    CLASS any_id internal_class_body
  | STRUCT any_id internal_class_body
  | STRUCT internal_class_body
  | UNION any_id internal_class_body
  | UNION internal_class_body;

internal_class_body:
    '{' maybe_other '}' maybe_other_no_semi ';'
  | ':' maybe_other_no_semi ';';

/*
 * Typedefs
 */

type_def:
    TYPEDEF type complex_typedef_id ';'
    {
      ValueInfo *item = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(item);
      item->ItemType = VTK_TYPEDEF_INFO;
      item->Access = access_level;

      handle_complex_type(item, $<integer>2, $<integer>3, getSig());

      if (getVarName())
        {
        item->Name = vtkstrdup(getVarName());
        }

      if (currentClass)
        {
        vtkParse_AddTypedefToClass(currentClass, item);
        }
      else
        {
        vtkParse_AddTypedefToNamespace(currentNamespace, item);
        }
    }
  | TYPEDEF class_def maybe_indirect_id ';'
  | TYPEDEF enum_def maybe_indirect_id ';'
  | TYPEDEF VAR_FUNCTION ';';

complex_typedef_id:
    var_decl
  | var_id '(' { pushFunction(); postSig("("); } parameter_list ')'
    maybe_func_const
    { $<integer>$ = VTK_PARSE_FUNCTION; postSig(")"); popFunction(); };


/*
 * The "using" keyword
 */

using:
    USING NAMESPACE any_id ';' { add_using($<str>3, 1); }
  | USING TYPENAME any_id ';'  { add_using($<str>3, 0); }
  | USING any_id ';' { add_using($<str>2, 0); }
  | USING scope OPERATOR op_token ';'
    { add_using(vtkstrcat3($<str>2, "operator", $<str>4), 0); }
  | USING scope_resolution scope OPERATOR op_token ';'
    { add_using(vtkstrcat4("::", $<str>3, "operator", $<str>5), 0); };


/*
 * Templates
 */

template:
    TEMPLATE '<' '>'
    { postSig("template<> "); clearTypeId(); }
  | TEMPLATE '<'
    { postSig("template<"); clearTypeId(); startTemplate(); }
    template_parameters '>'
    {
      chopSig();
      if (getSig()[getSigLength()-1] == '>') { postSig(" "); }
      postSig("> ");
      clearTypeId();
    };

template_parameters:
    template_parameter
  | template_parameter ',' { chopSig(); postSig(", "); clearTypeId(); }
    template_parameters;

template_parameter:
    { markSig(); }
    type_simple param_decl
    { add_template_arg($<integer>2, $<integer>3, copySig()); }
    maybe_template_default
  | { markSig(); }
    class_or_typename param_decl
    { add_template_arg(0, $<integer>3, copySig()); }
    maybe_template_default
  | { pushTemplate(); markSig(); }
    template param_decl
    {
      int i;
      TemplateArgs *newTemplate = currentTemplate;
      popTemplate();
      add_template_arg(0, $<integer>3, copySig());
      i = currentTemplate->NumberOfArguments-1;
      currentTemplate->Arguments[i]->Template = newTemplate;
    }
    maybe_template_default;

class_or_typename:
    CLASS { postSig("class "); }
  | TYPENAME { postSig("typename "); };

maybe_template_default:
  | template_default;

template_default:
    '=' { postSig("="); markSig(); }
    template_parameter_value
    {
      int i = currentTemplate->NumberOfArguments-1;
      TemplateArg *arg = currentTemplate->Arguments[i];
      chopSig();
      arg->Value = vtkstrdup(copySig());
    };

template_parameter_value:
    angle_bracket_pitem
  | template_parameter_value angle_bracket_pitem;


/*
 * Functions and Methods
 */

function:
    storage_type func;

scoped_method:
    storage_type scope func;
  | scope structor
  | storage_mods scope structor;

method:
    storage_type func
  | operator
  | structor
  | storage_mods structor;

scoped_operator:
    scope typecast_op_func
  | storage_type scope op_func;

operator:
    typecast_op_func
  | storage_type op_func;

typecast_op_func:
    OPERATOR { postSig("operator "); } storage_type '('
    {
      postSig("(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    parameter_list ')' { postSig(")"); } func_trailer
    {
      $<integer>$ = $<integer>3;
      closeSig();
      currentFunction->IsOperator = 1;
      currentFunction->Name = "operator typecast";
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed operator", "operator typecast");
    };

op_func:
    op_sig { postSig(")"); } func_trailer
    {
      closeSig();
      currentFunction->Name = vtkstrcat("operator", $<str>1);
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed operator", currentFunction->Name);
    };

op_sig:
    OPERATOR { postSig("operator"); } op_token { postSig($<str>3); } '('
    {
      postSig("(");
      currentFunction->IsOperator = 1;
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    parameter_list ')' { $<str>$ = $<str>3; };

func:
    func_sig func_trailer
    {
      closeSig();
      currentFunction->Name = vtkstrdup($<str>1);
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    };

func_trailer:
  | func_trailer throw_trailer
  | func_trailer const_trailer
  | func_trailer pure_trailer;

throw_trailer:
    THROW { postSig(" throw "); } parens_sig { chopSig(); };

const_trailer:
    CONST { postSig(" const"); currentFunction->IsConst = 1; };

pure_trailer:
    '=' ZERO
    {
      postSig(" = 0");
      currentFunction->IsPureVirtual = 1;
      if (currentClass) { currentClass->IsAbstract = 1; }
    };

func_body:
    '{' maybe_other '}'
  | ';';

func_sig:
    func_name '('
    {
      postSig("(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    parameter_list ')' { postSig(")"); };

func_name:
    simple_id
  | templated_id;


/*
 * Constructors and destructors are handled by the same rule
 */

structor:
    structor_sig { closeSig(); }
    maybe_initializers { openSig(); } func_trailer
    {
      closeSig();
      if (getStorageType() & VTK_PARSE_VIRTUAL)
        {
        currentFunction->IsVirtual = 1;
        }
      if (getStorageType() & VTK_PARSE_EXPLICIT)
        {
        currentFunction->IsExplicit = 1;
        }
      currentFunction->Name = vtkstrdup($<str>1);
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    };

structor_sig:
    func_name '(' { postSig("("); } parameter_list ')' { postSig(")"); };

maybe_initializers:
  | ':' initializer more_initializers;

more_initializers:
  | ',' initializer more_initializers;

initializer:
    any_id parens;

/*
 * Arguments
 */

parameter_list:
  | { clearTypeId(); } more_parameters;

more_parameters:
    ELLIPSIS { currentFunction->IsVariadic = 1; postSig("..."); }
  | parameter { clearTypeId(); }
  | parameter ',' { clearTypeId(); postSig(", "); } more_parameters;

parameter:
    { markSig(); }
    type param_decl
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

      vtkParse_AddArgumentToFunction(currentFunction, arg);
    }
    maybe_assign_value
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

      vtkParse_AddArgumentToFunction(currentFunction, arg);
    };

maybe_indirect_id:
    simple_id
  | type_indirection simple_id;

maybe_assign_value:
    { clearVarValue(); }
  | assign_value;

assign_value:
    '=' { postSig("="); clearVarValue(); markSig(); }
    const_expr { chopSig(); setVarValue(copySig()); };

/*
 * Variables
 */

variables:
    storage_type var_id_maybe_assign_value maybe_other_variables ';'
  | STATIC VAR_FUNCTION maybe_other_variables ';'
  | VAR_FUNCTION maybe_other_variables ';';

var_id_maybe_assign_value:
    var_decl maybe_assign_value
    {
      unsigned int type = getStorageType();
      ValueInfo *var = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(var);
      var->ItemType = VTK_VARIABLE_INFO;
      var->Access = access_level;

      handle_complex_type(var, type, $<integer>1, getSig());

      var->Name = vtkstrdup(getVarName());

      if (getVarValue())
        {
        var->Value = vtkstrdup(getVarValue());
        }

      if ((var->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_FUNCTION)
        {
        var->Type = var->Type;
        }

      /* Is this a constant? */
      if (((type & VTK_PARSE_CONST) != 0) && var->Value != NULL &&
          (((type & VTK_PARSE_INDIRECT) == 0) ||
           ((type & VTK_PARSE_INDIRECT) == VTK_PARSE_ARRAY)))
        {
        var->ItemType = VTK_CONSTANT_INFO;
        if (currentClass)
          {
          vtkParse_AddConstantToClass(currentClass, var);
          }
        else
          {
          vtkParse_AddConstantToNamespace(currentNamespace, var);
          }
        }
      /* This is a true variable i.e. not constant */
      else
        {
        if (currentClass)
          {
          vtkParse_AddVariableToClass(currentClass, var);
          }
        else
          {
          vtkParse_AddVariableToNamespace(currentNamespace, var);
          }
        }
    };

maybe_variables:
  | other_variable maybe_other_variables;

maybe_other_variables:
  | maybe_other_variables ',' { postSig(", "); } other_variable;

other_variable:
    { setStorageTypeIndirection(0); } var_id_maybe_assign_value
  | type_indirection
    { setStorageTypeIndirection($<integer>1); } var_id_maybe_assign_value;

/* for parameters, the var_id is optional */
param_decl:
    maybe_var_id maybe_array_decorator { $<integer>$ = 0; }
  | p_or_lp_or_la maybe_indirect_param_decl ')' { postSig(")"); }
    maybe_array_or_parameters
    {
      const char *scope = getScope();
      unsigned int parens = add_indirection($<integer>1, $<integer>2);
      if ($<integer>5 == VTK_PARSE_FUNCTION)
        {
        if (scope) { scope = vtkstrndup(scope, strlen(scope) - 2); }
        getFunction()->Class = scope;
        $<integer>$ = (parens | VTK_PARSE_FUNCTION);
        }
      else if ($<integer>5 == VTK_PARSE_ARRAY)
        {
        $<integer>$ = add_indirection_to_array(parens);
        }
    };

/* for variables, the var_id is mandatory */
var_decl:
    var_id maybe_array_decorator { $<integer>$ = 0; }
  | lp_or_la maybe_indirect_var_decl ')' { postSig(")"); }
    maybe_array_or_parameters
    {
      const char *scope = getScope();
      unsigned int parens = add_indirection($<integer>1, $<integer>2);
      if ($<integer>5 == VTK_PARSE_FUNCTION)
        {
        if (scope) { scope = vtkstrndup(scope, strlen(scope) - 2); }
        getFunction()->Class = scope;
        $<integer>$ = (parens | VTK_PARSE_FUNCTION);
        }
      else if ($<integer>5 == VTK_PARSE_ARRAY)
        {
        $<integer>$ = add_indirection_to_array(parens);
        }
    };

p_or_lp_or_la:
    '(' { postSig("("); scopeSig(""); $<integer>$ = 0; }
  | LP { postSig("("); scopeSig($<str>1); postSig("*");
         $<integer>$ = VTK_PARSE_POINTER; }
  | LA { postSig("("); scopeSig($<str>1); postSig("&");
         $<integer>$ = VTK_PARSE_REF; };

lp_or_la:
    LP { postSig("("); scopeSig($<str>1); postSig("*");
         $<integer>$ = VTK_PARSE_POINTER; }
  | LA { postSig("("); scopeSig($<str>1); postSig("&");
         $<integer>$ = VTK_PARSE_REF; };

maybe_func_const:
  | CONST { currentFunction->IsConst = 1; }
  | THROW parens;

maybe_array_or_parameters: { $<integer>$ = 0; }
  | '(' { pushFunction(); postSig("("); } parameter_list ')' maybe_func_const
    {
      $<integer>$ = VTK_PARSE_FUNCTION;
      postSig(")");
      popFunction();
    }
  | array_decorator { $<integer>$ = VTK_PARSE_ARRAY; };

maybe_indirect_param_decl:
    param_decl
  | type_indirection param_decl
    { $<integer>$ = add_indirection($<integer>1, $<integer>2); };

maybe_indirect_var_decl:
    var_decl
  | type_indirection var_decl
    { $<integer>$ = add_indirection($<integer>1, $<integer>2); };

maybe_var_id:
    { clearVarName(); chopSig(); }
  | var_id;

var_id:
    simple_id {setVarName($<str>1);};

maybe_array_decorator:
    { clearArray(); }
  | array_decorator;

array_decorator:
    { clearArray(); } array;

array:
    more_array '[' { postSig("["); } array_size ']' { postSig("]"); };

more_array:
  | array;

array_size:
    { pushArraySize(""); }
  | { markSig(); } const_expr { chopSig(); pushArraySize(copySig()); };

/*
 * simple_id evaluates to string and sigs itself, note that '~' is
 * considered part of the ID because this simplifies the handling of
 * destructor names, and since the parser doesn't do any math, there
 * is no conflict with the '~' operator.
 */

simple_id:
    VTK_ID { postSig($<str>1); }
  | QT_ID { postSig($<str>1); }
  | ID { postSig($<str>1); }
  | ISTREAM { postSig($<str>1); }
  | OSTREAM { postSig($<str>1); }
  | StdString { postSig($<str>1); }
  | UnicodeString { postSig($<str>1); }
  | '~' VTK_ID { $<str>$ = vtkstrcat("~",$<str>2); postSig($<str>$); }
  | '~' QT_ID { $<str>$ = vtkstrcat("~",$<str>2); postSig($<str>$); }
  | '~' ID { $<str>$ = vtkstrcat("~",$<str>2); postSig($<str>$); }
  | '~' ISTREAM { $<str>$ = vtkstrcat("~",$<str>2); postSig($<str>$); }
  | '~' OSTREAM { $<str>$ = vtkstrcat("~",$<str>2); postSig($<str>$); }
  | '~' StdString { $<str>$ = vtkstrcat("~",$<str>2); postSig($<str>$); }
  | '~' UnicodeString { $<str>$ = vtkstrcat("~",$<str>2); postSig($<str>$); }
  | SIZE_T { $<str>$ = "size_t"; postSig($<str>$); }
  | SSIZE_T { $<str>$ = "ssize_t"; postSig($<str>$); }
  | TypeInt8 { $<str>$ = "vtkTypeInt8"; postSig($<str>$); }
  | TypeUInt8 { $<str>$ = "vtkTypeUInt8"; postSig($<str>$); }
  | TypeInt16 { $<str>$ = "vtkTypeInt16"; postSig($<str>$); }
  | TypeUInt16 { $<str>$ = "vtkTypeUInt16"; postSig($<str>$); }
  | TypeInt32 { $<str>$ = "vtkTypeInt32"; postSig($<str>$); }
  | TypeUInt32 { $<str>$ = "vtkTypeUInt32"; postSig($<str>$); }
  | TypeInt64 { $<str>$ = "vtkTypeInt64"; postSig($<str>$); }
  | TypeUInt64 { $<str>$ = "vtkTypeUInt64"; postSig($<str>$); }
  | TypeFloat32 { $<str>$ = "vtkTypeFloat32"; postSig($<str>$); }
  | TypeFloat64 { $<str>$ = "vtkTypeFloat64"; postSig($<str>$); }
  | IdType { $<str>$ = "vtkIdType"; postSig($<str>$); }
  | FloatType { $<str>$ = "vtkFloatingPointType"; postSig($<str>$); };

/*
 * class_id does not have any side-effects, and is needes for some rules.
 */

class_id:
    ID
  | QT_ID
  | VTK_ID
  | ISTREAM
  | OSTREAM
  | StdString
  | UnicodeString;


/*
 * Modifiers
 */

storage_mods:
    storage_seq { setStorageType($<integer>1); };

storage_mod:
    MUTABLE { $<integer>$ = 0; }
  | INLINE { $<integer>$ = 0; }
  | EXTERN { $<integer>$ = 0; }
  | EXTERN STRING_LITERAL { $<integer>$ = 0; }
  | EXPLICIT { postSig("explicit "); $<integer>$ = VTK_PARSE_EXPLICIT; }
  | STATIC { postSig("static "); $<integer>$ = VTK_PARSE_STATIC; }
  | VIRTUAL { postSig("virtual "); $<integer>$ = VTK_PARSE_VIRTUAL; };

storage_seq:
    storage_mod
  | storage_seq storage_mod { $<integer>$ = ($<integer>1 | $<integer>2); };


/*
 * Types
 */

storage_type:
    type { setStorageType($<integer>$); }
  | storage_mods type
    {
      $<integer>$ = ($<integer>1 | $<integer>2);
      setStorageType($<integer>$);
    };

type:
    type_red
  | type_red type_indirection { $<integer>$ = ($<integer>1 | $<integer>2); };

type_red:
    type_red2
  | const_mod type_red2 { $<integer>$ = ($<integer>1 | $<integer>2); }
  | type_red2 const_mod { $<integer>$ = ($<integer>1 | $<integer>2); };

type_red2:
    type_simple
  | templated_id
    { postSig(" "); setTypeId($<str>1); $<integer>$ = VTK_PARSE_UNKNOWN; }
  | scoped_id
    { postSig(" "); setTypeId($<str>1); $<integer>$ = VTK_PARSE_UNKNOWN; }
  | TYPENAME { postSig("typename "); } any_id
    { postSig(" "); setTypeId($<str>3); $<integer>$ = VTK_PARSE_UNKNOWN; }
  | CLASS type_id { $<integer>$ = $<integer>2; }
  | STRUCT type_id { $<integer>$ = $<integer>2; }
  | UNION type_id { typeSig($<str>2); $<integer>$ = VTK_PARSE_UNKNOWN; }
  | ENUM type_id { typeSig($<str>2); $<integer>$ = VTK_PARSE_UNKNOWN; };

const_mod:
    CONST { postSig("const "); $<integer>$ = VTK_PARSE_CONST; };

templated_id:
    class_id '<' { markSig(); postSig($<str>1); postSig("<"); }
    angle_bracket_contents '>'
    {
      chopSig(); if (getSig()[getSigLength()-1] == '>') { postSig(" "); }
      postSig(">"); $<str>$ = vtkstrdup(copySig()); clearTypeId();
    };

any_id:
    simple_id
  | templated_id
  | scoped_id;

scoped_id:
    scope simple_id { $<str>$ = vtkstrcat($<str>1, $<str>2); }
  | scope templated_id { $<str>$ = vtkstrcat($<str>1, $<str>2); }
  | scope_resolution any_id { $<str>$ = vtkstrcat($<str>1, $<str>2); };

scope:
    class_id_sig scope_resolution
    { $<str>$ = vtkstrcat($<str>1, $<str>2); }
  | templated_id scope_resolution
    { $<str>$ = vtkstrcat($<str>1, $<str>2); }
  | scope class_id_sig scope_resolution
    { $<str>$ = vtkstrcat3($<str>1, $<str>2, $<str>3); }
  | scope templated_id scope_resolution
    { $<str>$ = vtkstrcat3($<str>1, $<str>2, $<str>3); }
  | scope TEMPLATE { postSig("template "); } templated_id scope_resolution
    { $<str>$ = vtkstrcat4($<str>1, "template ", $<str>4, $<str>5); };

class_id_sig:
    class_id { postSig($<str>1); };

scope_resolution:
    DOUBLE_COLON { $<str>$ = "::"; postSig($<str>$); };

type_simple:
    type_primitive
  | type_id;

type_id:
    StdString { typeSig($<str>1); $<integer>$ = VTK_PARSE_STRING; }
  | UnicodeString { typeSig($<str>1); $<integer>$ = VTK_PARSE_UNICODE_STRING;}
  | OSTREAM { typeSig($<str>1); $<integer>$ = VTK_PARSE_OSTREAM; }
  | ISTREAM { typeSig($<str>1); $<integer>$ = VTK_PARSE_ISTREAM; }
  | ID { typeSig($<str>1); $<integer>$ = VTK_PARSE_UNKNOWN; }
  | VTK_ID { typeSig($<str>1); $<integer>$ = VTK_PARSE_OBJECT; }
  | QT_ID { typeSig($<str>1); $<integer>$ = VTK_PARSE_QOBJECT; }
  | SSIZE_T { typeSig("ssize_t"); $<integer>$ = VTK_PARSE_SSIZE_T; }
  | SIZE_T { typeSig("size_t"); $<integer>$ = VTK_PARSE_SIZE_T; }
  | TypeInt8 { typeSig("vtkTypeInt8"); $<integer>$ = VTK_PARSE_INT8; }
  | TypeUInt8 { typeSig("vtkTypeUInt8"); $<integer>$ = VTK_PARSE_UINT8; }
  | TypeInt16 { typeSig("vtkTypeInt16"); $<integer>$ = VTK_PARSE_INT16; }
  | TypeUInt16 { typeSig("vtkTypeUInt16"); $<integer>$ = VTK_PARSE_UINT16; }
  | TypeInt32 { typeSig("vtkTypeInt32"); $<integer>$ = VTK_PARSE_INT32; }
  | TypeUInt32 { typeSig("vtkTypeUInt32"); $<integer>$ = VTK_PARSE_UINT32; }
  | TypeInt64 { typeSig("vtkTypeInt64"); $<integer>$ = VTK_PARSE_INT64; }
  | TypeUInt64 { typeSig("vtkTypeUInt64"); $<integer>$ = VTK_PARSE_UINT64; }
  | TypeFloat32 { typeSig("vtkTypeFloat32"); $<integer>$ = VTK_PARSE_FLOAT32; }
  | TypeFloat64 { typeSig("vtkTypeFloat64"); $<integer>$ = VTK_PARSE_FLOAT64; }
  | IdType { typeSig("vtkIdType"); $<integer>$ = VTK_PARSE_ID_TYPE; }
  | FloatType { typeSig("double"); $<integer>$ = VTK_PARSE_DOUBLE; };

type_primitive:
    VOID   { typeSig("void"); $<integer>$ = VTK_PARSE_VOID;}
  | BOOL { typeSig("bool"); $<integer>$ = VTK_PARSE_BOOL;}
  | FLOAT  { typeSig("float"); $<integer>$ = VTK_PARSE_FLOAT; }
  | DOUBLE { typeSig("double"); $<integer>$ = VTK_PARSE_DOUBLE; }
  | LONG_DOUBLE { typeSig("long double"); $<integer>$ = VTK_PARSE_UNKNOWN; }
  | CHAR   { typeSig("char"); $<integer>$ = VTK_PARSE_CHAR; }
  | SIGNED_CHAR { typeSig("signed char"); $<integer>$ = VTK_PARSE_SIGNED_CHAR;}
  | UNSIGNED_CHAR
    { typeSig("unsigned char"); $<integer>$ = VTK_PARSE_UNSIGNED_CHAR; }
  | INT    { typeSig("int"); $<integer>$ = VTK_PARSE_INT; }
  | UNSIGNED_INT
    { typeSig("unsigned int"); $<integer>$ = VTK_PARSE_UNSIGNED_INT; }
  | SHORT  { typeSig("short"); $<integer>$ = VTK_PARSE_SHORT; }
  | UNSIGNED_SHORT
    { typeSig("unsigned short"); $<integer>$ = VTK_PARSE_UNSIGNED_SHORT; }
  | LONG   { typeSig("long"); $<integer>$ = VTK_PARSE_LONG; }
  | UNSIGNED_LONG
    { typeSig("unsigned long"); $<integer>$ = VTK_PARSE_UNSIGNED_LONG; }
  | LONG_LONG   { typeSig("long long"); $<integer>$ = VTK_PARSE_LONG_LONG; }
  | UNSIGNED_LONG_LONG
    {typeSig("unsigned long long");$<integer>$=VTK_PARSE_UNSIGNED_LONG_LONG; }
  | INT64__ { typeSig("__int64"); $<integer>$ = VTK_PARSE___INT64; }
  | UNSIGNED_INT64__
    { typeSig("unsigned __int64"); $<integer>$ = VTK_PARSE_UNSIGNED___INT64; }
  | SIGNED { typeSig("int"); $<integer>$ = VTK_PARSE_INT; }
  | UNSIGNED { typeSig("unsigned int"); $<integer>$ = VTK_PARSE_UNSIGNED_INT;};


/*
 * Pointers and references
 */

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
  | pointers;

/* "VTK_BAD_INDIRECT" occurs when the bitfield fills up */

pointers:
    pointer_or_const_pointer
  | pointers pointer_or_const_pointer
    {
      unsigned int n;
      n = (($<integer>1 << 2) | $<integer>2);
      if ((n & VTK_PARSE_INDIRECT) != n)
        {
        n = VTK_PARSE_BAD_INDIRECT;
        }
     $<integer>$ = n;
   };

pointer_or_const_pointer:
    '*' { postSig("*"); $<integer>$ = VTK_PARSE_POINTER; }
  | '*' CONST { postSig("*const "); $<integer>$ = VTK_PARSE_CONST_POINTER; };

/*
 * VTK Macros
 */

macro:
  SetMacro '(' simple_id ',' {preSig("void Set"); postSig("(");} type ')'
   {
   postSig("a);");
   currentFunction->Macro = "vtkSetMacro";
   currentFunction->Name = vtkstrcat("Set", $<str>3);
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, $<integer>6, getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
| GetMacro '(' {postSig("Get");} simple_id ','
   {markSig();} type {swapSig();} ')'
   {
   postSig("();");
   currentFunction->Macro = "vtkGetMacro";
   currentFunction->Name = vtkstrcat("Get", $<str>4);
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, $<integer>7, getTypeId(), 0);
   output_function();
   }
| SetStringMacro '(' {preSig("void Set");} simple_id ')'
   {
   postSig("(char *);");
   currentFunction->Macro = "vtkSetStringMacro";
   currentFunction->Name = vtkstrcat("Set", $<str>4);
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
| GetStringMacro '(' {preSig("char *Get");} simple_id ')'
   {
   postSig("();");
   currentFunction->Macro = "vtkGetStringMacro";
   currentFunction->Name = vtkstrcat("Get", $<str>4);
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   output_function();
   }
| SetClampMacro '(' simple_id ',' {startSig(); markSig();} type_red {closeSig();}
     ',' maybe_other_no_semi ')'
   {
   const char *typeText;
   chopSig();
   typeText = copySig();

   currentFunction->Macro = "vtkSetClampMacro";
   currentFunction->Name = vtkstrcat("Set", $<str>3);
   currentFunction->Signature =
     vtkstrcat5("void ", currentFunction->Name, "(", typeText, ");");
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, $<integer>6, getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();

   currentFunction->Macro = "vtkSetClampMacro";
   currentFunction->Name = vtkstrcat3("Get", $<str>3, "MinValue");
   currentFunction->Signature =
     vtkstrcat4(typeText, " ", currentFunction->Name, "();");
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, $<integer>6, getTypeId(), 0);
   output_function();

   currentFunction->Macro = "vtkSetClampMacro";
   currentFunction->Name = vtkstrcat3("Get", $<str>3, "MaxValue");
   currentFunction->Signature =
     vtkstrcat4(typeText, " ", currentFunction->Name, "();");
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, $<integer>6, getTypeId(), 0);
   output_function();
   }
| SetObjectMacro '(' simple_id ','
  {preSig("void Set"); postSig("("); } type_red ')'
   {
   postSig("*);");
   currentFunction->Macro = "vtkSetObjectMacro";
   currentFunction->Name = vtkstrcat("Set", $<str>3);
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
| GetObjectMacro '(' {postSig("*Get");} simple_id ','
   {markSig();} type_red {swapSig();} ')'
   {
   postSig("();");
   currentFunction->Macro = "vtkGetObjectMacro";
   currentFunction->Name = vtkstrcat("Get", $<str>4);
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   output_function();
   }
| BooleanMacro '(' simple_id ',' type_red ')'
   {
   currentFunction->Macro = "vtkBooleanMacro";
   currentFunction->Name = vtkstrcat($<str>3, "On");
   currentFunction->Comment = vtkstrdup(getComment());
   currentFunction->Signature =
     vtkstrcat3("void ", currentFunction->Name, "();");
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();

   currentFunction->Macro = "vtkBooleanMacro";
   currentFunction->Name = vtkstrcat($<str>3, "Off");
   currentFunction->Comment = vtkstrdup(getComment());
   currentFunction->Signature =
     vtkstrcat3("void ", currentFunction->Name, "();");
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
| SetVector2Macro '(' simple_id ',' {startSig(); markSig();} type_red ')'
   {
   chopSig();
   outputSetVectorMacro($<str>3, $<integer>6, copySig(), 2);
   }
| GetVector2Macro '(' simple_id ',' {startSig(); markSig();} type_red ')'
   {
   chopSig();
   outputGetVectorMacro($<str>3, $<integer>6, copySig(), 2);
   }
| SetVector3Macro '(' simple_id ',' {startSig(); markSig();} type_red ')'
   {
   chopSig();
   outputSetVectorMacro($<str>3, $<integer>6, copySig(), 3);
   }
| GetVector3Macro  '(' simple_id ',' {startSig(); markSig();} type_red ')'
   {
   chopSig();
   outputGetVectorMacro($<str>3, $<integer>6, copySig(), 3);
   }
| SetVector4Macro '(' simple_id ',' {startSig(); markSig();} type_red ')'
   {
   chopSig();
   outputSetVectorMacro($<str>3, $<integer>6, copySig(), 4);
   }
| GetVector4Macro  '(' simple_id ',' {startSig(); markSig();} type_red ')'
   {
   chopSig();
   outputGetVectorMacro($<str>3, $<integer>6, copySig(), 4);
   }
| SetVector6Macro '(' simple_id ',' {startSig(); markSig();} type_red ')'
   {
   chopSig();
   outputSetVectorMacro($<str>3, $<integer>6, copySig(), 6);
   }
| GetVector6Macro  '(' simple_id ',' {startSig(); markSig();} type_red ')'
   {
   chopSig();
   outputGetVectorMacro($<str>3, $<integer>6, copySig(), 6);
   }
| SetVectorMacro  '(' simple_id ',' {startSig(); markSig();}
     type_red ',' INT_LITERAL ')'
   {
   const char *typeText;
   chopSig();
   typeText = copySig();
   currentFunction->Macro = "vtkSetVectorMacro";
   currentFunction->Name = vtkstrcat("Set", $<str>3);
   currentFunction->Signature =
     vtkstrcat7("void ", currentFunction->Name, "(", typeText,
                " a[", $<str>8, "]);");
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, (VTK_PARSE_POINTER | $<integer>6),
                getTypeId(), (int)strtol($<str>8, NULL, 0));
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
| GetVectorMacro  '(' simple_id ',' {startSig();}
     type_red ',' INT_LITERAL ')'
   {
   chopSig();
   currentFunction->Macro = "vtkGetVectorMacro";
   currentFunction->Name = vtkstrcat("Get", $<str>3);
   postSig(" *");
   postSig(currentFunction->Name);
   postSig("();");
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, (VTK_PARSE_POINTER | $<integer>6),
              getTypeId(), (int)strtol($<str>8, NULL, 0));
   output_function();
   }
| ViewportCoordinateMacro '(' simple_id ')'
   {
     currentFunction->Macro = "vtkViewportCoordinateMacro";
     currentFunction->Name = vtkstrcat3("Get", $<str>3, "Coordinate");
     currentFunction->Signature =
       vtkstrcat3("vtkCoordinate *", currentFunction->Name, "();");
     currentFunction->Comment = vtkstrdup(getComment());
     set_return(currentFunction, VTK_PARSE_OBJECT_PTR, "vtkCoordinate", 0);
     output_function();

     currentFunction->Macro = "vtkViewportCoordinateMacro";
     currentFunction->Name = vtkstrcat("Set", $<str>3);
     currentFunction->Signature =
       vtkstrcat3("void ", currentFunction->Name, "(double, double);");
     currentFunction->Comment = vtkstrdup(getComment());
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Macro = "vtkViewportCoordinateMacro";
     currentFunction->Name = vtkstrcat("Set", $<str>3);
     currentFunction->Signature =
       vtkstrcat3("void ", currentFunction->Name, "(double a[2]);");
     currentFunction->Comment = vtkstrdup(getComment());
     add_argument(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 2);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Macro = "vtkViewportCoordinateMacro";
     currentFunction->Name = vtkstrcat("Get", $<str>3);
     currentFunction->Signature =
       vtkstrcat3("double *", currentFunction->Name, "();");
     currentFunction->Comment = vtkstrdup(getComment());
     set_return(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 2);
     output_function();
   }
| WorldCoordinateMacro '(' simple_id ')'
   {
     currentFunction->Macro = "vtkWorldCoordinateMacro";
     currentFunction->Name = vtkstrcat3("Get", $<str>3, "Coordinate");
     currentFunction->Signature =
       vtkstrcat3("vtkCoordinate *", currentFunction->Name, "();");
     currentFunction->Comment = vtkstrdup(getComment());
     set_return(currentFunction, VTK_PARSE_OBJECT_PTR, "vtkCoordinate", 0);
     output_function();

     currentFunction->Macro = "vtkWorldCoordinateMacro";
     currentFunction->Name = vtkstrcat("Set", $<str>3);
     currentFunction->Signature =
       vtkstrcat3("void ", currentFunction->Name, "(double, double, double);");
     currentFunction->Comment = vtkstrdup(getComment());
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Macro = "vtkWorldCoordinateMacro";
     currentFunction->Name = vtkstrcat("Set", $<str>3);
     currentFunction->Signature =
       vtkstrcat3("void ", currentFunction->Name, "(double a[3]);");
     currentFunction->Comment = vtkstrdup(getComment());
     add_argument(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 3);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Macro = "vtkWorldCoordinateMacro";
     currentFunction->Name = vtkstrcat("Get", $<str>3);
     currentFunction->Signature =
       vtkstrcat3("double *", currentFunction->Name, "();");
     currentFunction->Comment = vtkstrdup(getComment());
     set_return(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 3);
     output_function();
   }
| TypeMacro '(' simple_id ',' simple_id maybe_comma ')'
   {
   int is_concrete = 0;
   int i;

   currentFunction->Macro = "vtkTypeMacro";
   currentFunction->Name = vtkstrdup("GetClassName");
   currentFunction->Signature = vtkstrdup("const char *GetClassName();");
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR),
              "char", 0);
   output_function();

   currentFunction->Macro = "vtkTypeMacro";
   currentFunction->Name = vtkstrdup("IsA");
   currentFunction->Signature = vtkstrdup("int IsA(const char *name);");
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR),
                "char", 0);
   set_return(currentFunction, VTK_PARSE_INT, "int", 0);
   output_function();

   currentFunction->Macro = "vtkTypeMacro";
   currentFunction->Name = vtkstrdup("NewInstance");
   currentFunction->Signature = vtkstrcat($<str>3, " *NewInstance();");
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_OBJECT_PTR, $<str>3, 0);
   output_function();

   for (i = 0; i < NumberOfConcreteClasses; i++)
     {
     if (strcmp(currentClass->Name, ConcreteClasses[i]) == 0)
       {
       is_concrete = 1;
       break;
       }
     }

   if ( is_concrete )
     {
     currentFunction->Macro = "vtkTypeMacro";
     currentFunction->Name = vtkstrdup("SafeDownCast");
     currentFunction->Signature =
       vtkstrcat($<str>3, " *SafeDownCast(vtkObject* o);");
     currentFunction->Comment = vtkstrdup(getComment());
     add_argument(currentFunction, VTK_PARSE_OBJECT_PTR, "vtkObject", 0);
     set_return(currentFunction, (VTK_PARSE_STATIC | VTK_PARSE_OBJECT_PTR),
                $<str>3, 0);
     output_function();
     }
   }
;

maybe_comma: | ',';

/*
 * Operators
 */

op_token:
    '(' ')' { $<str>$ = "()"; }
  | '[' ']' { $<str>$ = "[]"; }
  | NEW '[' ']' { $<str>$ = " new[]"; }
  | DELETE '[' ']' { $<str>$ = " delete[]"; }
  | '<' { $<str>$ = "<"; }
  | '>' { $<str>$ = ">"; }
  | ',' { $<str>$ = ","; }
  | '=' { $<str>$ = "="; }
  | op_token_no_delim;

op_token_no_delim:
    '%' { $<str>$ = "%"; }
  | '*' { $<str>$ = "*"; }
  | '/' { $<str>$ = "/"; }
  | '-' { $<str>$ = "-"; }
  | '+' { $<str>$ = "+"; }
  | '!' { $<str>$ = "!"; }
  | '~' { $<str>$ = "~"; }
  | '&' { $<str>$ = "&"; }
  | '|' { $<str>$ = "|"; }
  | '^' { $<str>$ = "^"; }
  | NEW { $<str>$ = " new"; }
  | DELETE { $<str>$ = " delete"; }
  | OP_LSHIFT_EQ { $<str>$ = "<<="; }
  | OP_RSHIFT_EQ { $<str>$ = ">>="; }
  | OP_LSHIFT { $<str>$ = "<<"; }
  | OP_RSHIFT { $<str>$ = ">>"; }
  | OP_DOT_POINTER { $<str>$ = ".*"; }
  | OP_ARROW_POINTER { $<str>$ = "->*"; }
  | OP_ARROW { $<str>$ = "->"; }
  | OP_PLUS_EQ { $<str>$ = "+="; }
  | OP_MINUS_EQ { $<str>$ = "-="; }
  | OP_TIMES_EQ { $<str>$ = "*="; }
  | OP_DIVIDE_EQ { $<str>$ = "/="; }
  | OP_REMAINDER_EQ { $<str>$ = "%="; }
  | OP_INCR { $<str>$ = "++"; }
  | OP_DECR { $<str>$ = "--"; }
  | OP_AND_EQ { $<str>$ = "&="; }
  | OP_OR_EQ { $<str>$ = "|="; }
  | OP_XOR_EQ { $<str>$ = "^="; }
  | OP_LOGIC_AND { $<str>$ = "&&"; }
  | OP_LOGIC_OR { $<str>$ = "||"; }
  | OP_LOGIC_EQ { $<str>$ = "=="; }
  | OP_LOGIC_NEQ { $<str>$ = "!="; }
  | OP_LOGIC_LEQ { $<str>$ = "<="; }
  | OP_LOGIC_GEQ { $<str>$ = ">="; };

keyword:
    TYPEDEF { $<str>$ = "typedef"; }
  | TYPENAME { $<str>$ = "typename"; }
  | CLASS { $<str>$ = "class"; }
  | STRUCT { $<str>$ = "struct"; }
  | UNION { $<str>$ = "union"; }
  | TEMPLATE { $<str>$ = "template"; }
  | PUBLIC { $<str>$ = "public"; }
  | PROTECTED { $<str>$ = "protected"; }
  | PRIVATE { $<str>$ = "private"; }
  | CONST { $<str>$ = "const"; }
  | STATIC { $<str>$ = "static"; }
  | INLINE { $<str>$ = "inline"; }
  | VIRTUAL { $<str>$ = "virtual"; }
  | EXTERN { $<str>$ = "extern"; }
  | NAMESPACE { $<str>$ = "namespace"; }
  | OPERATOR { $<str>$ = "operator"; }
  | ENUM { $<str>$ = "enum"; }
  | THROW { $<str>$ = "throw"; }
  | CONST_CAST { $<str>$ = "const_cast"; }
  | DYNAMIC_CAST { $<str>$ = "dynamic_cast"; }
  | STATIC_CAST { $<str>$ = "static_cast"; }
  | REINTERPRET_CAST { $<str>$ = "reinterpret_cast"; };

literal:
    OCT_LITERAL
  | INT_LITERAL
  | HEX_LITERAL
  | FLOAT_LITERAL
  | CHAR_LITERAL
  | STRING_LITERAL
  | ZERO;

/*
 * Constant expressions that evaluate to values
 */

const_expr:
    bracket_pitem
  | const_expr bracket_pitem;

common_bracket_item:
    brackets_sig
  | parens_sig
  | braces_sig
  | op_token_no_delim
    {
      if ((($<str>1)[0] == '+' || ($<str>1)[0] == '-' ||
           ($<str>1)[0] == '*' || ($<str>1)[0] == '&') &&
          ($<str>1)[1] == '\0')
        {
        int c1 = 0;
        size_t l;
        const char *cp;
        chopSig();
        cp = getSig();
        l = getSigLength();
        if (l != 0) { c1 = cp[l-1]; }
        if (c1 != 0 && c1 != '(' && c1 != '[' && c1 != '=')
          {
          postSig(" ");
          }
        postSig($<str>1);
        if ((c1 >= 'A' && c1 <= 'Z') || (c1 >= 'a' && c1 <= 'z') ||
            (c1 >= '0' && c1 <= '9') || c1 == '_' || c1 == '\'' ||
            c1 == '\"' || c1 == ')' || c1 == ']')
          {
          postSig(" ");
          }
        }
       else
        {
        postSig($<str>1);
        postSig(" ");
        }
    }
  | ':' { postSig(":"); postSig(" "); } | '.' { postSig("."); }
  | DOUBLE_COLON { chopSig(); postSig("::"); }
  | keyword { postSig($<str>1); postSig(" "); }
  | literal { postSig($<str>1); postSig(" "); }
  | type_primitive
  | type_id
    {
      int c1 = 0;
      size_t l;
      const char *cp;
      chopSig();
      cp = getSig();
      l = getSigLength();
      if (l != 0) { c1 = cp[l-1]; }
      while (((c1 >= 'A' && c1 <= 'Z') || (c1 >= 'a' && c1 <= 'z') ||
              (c1 >= '0' && c1 <= '9') || c1 == '_') && l != 0)
        {
        --l;
        c1 = cp[l-1];
        }
      if (l < 2 || cp[l-1] != ':' || cp[l-2] != ':')
        {
        cp = add_const_scope(&cp[l]);
        resetSig(l);
        postSig(cp);
        }
    };

any_bracket_contents:
  | any_bracket_contents any_bracket_item;

bracket_pitem: common_bracket_item
  | '<' { postSig("< "); }
  | '>' { postSig("> "); };

any_bracket_item: bracket_pitem
  | '=' { postSig("= "); }
  | ',' { chopSig(); postSig(", "); };

braces_item: any_bracket_item
  | ';' { chopSig(); postSig(";"); };

angle_bracket_contents:
  | angle_bracket_contents angle_bracket_item;

braces_contents:
  | braces_contents braces_item;

angle_bracket_pitem:
    angle_brackets_sig
  | common_bracket_item;

angle_bracket_item:
    angle_bracket_pitem
  | '=' { postSig("= "); }
  | ',' { chopSig(); postSig(", "); };

angle_brackets_sig:
    '<'
    {
      chopSig();
      if (getSig()[getSigLength()-1] == '<') { postSig(" "); }
      postSig("<");
    }
    angle_bracket_contents '>'
    {
      chopSig();
      if (getSig()[getSigLength()-1] == '>') { postSig(" "); }
      postSig("> ");
    };

brackets_sig:
    '[' { postSig("["); } any_bracket_contents ']'
    { chopSig(); postSig("] "); };

parens_sig:
    '(' { postSig("("); } any_bracket_contents ')'
    { chopSig(); postSig(") "); }
  | LP { postSig("("); postSig($<str>1); postSig("*"); }
    any_bracket_contents ')' { chopSig(); postSig(") "); }
  | LA { postSig("("); postSig($<str>1); postSig("&"); }
    any_bracket_contents ')' { chopSig(); postSig(") "); }

braces_sig:
    '{' { postSig("{ "); } braces_contents '}' { postSig("} "); };

/*
 * These just eat up stuff we don't care about, like function bodies
 */
maybe_other:
  | maybe_other other_stuff;

maybe_other_no_semi:
  | maybe_other_no_semi other_stuff_no_semi;

other_stuff:
    other_stuff_no_semi
  | ';';

other_stuff_no_semi:
    braces | parens | brackets
  | DOUBLE_COLON | op_token_no_delim | ':' | '.' | '<' | '>' | '=' | ','
  | keyword | literal | type_simple | VAR_FUNCTION
  | ELLIPSIS | OTHER;

braces:
  '{' maybe_other '}';

brackets:
  '[' maybe_other ']';

parens:
  lparen maybe_other ')';

lparen:
  '(' | LP | LA;

%%
#include <string.h>
#include "lex.yy.c"

/* check whether this is the class we are looking for */
void start_class(const char *classname, int is_struct_or_union)
{
  ClassInfo *outerClass = currentClass;
  pushClass();
  currentClass = (ClassInfo *)malloc(sizeof(ClassInfo));
  vtkParse_InitClass(currentClass);
  currentClass->Name = vtkstrdup(classname);
  if (is_struct_or_union == 1)
    {
    currentClass->ItemType = VTK_STRUCT_INFO;
    }
  if (is_struct_or_union == 2)
    {
    currentClass->ItemType = VTK_UNION_INFO;
    }

  if (classname[strlen(classname)-1] != '>')
    {
    if (outerClass)
      {
      vtkParse_AddClassToClass(outerClass, currentClass);
      }
    else
      {
      vtkParse_AddClassToNamespace(currentNamespace, currentClass);
      }
    }

  /* template information */
  if (currentTemplate)
    {
    currentClass->Template = currentTemplate;
    currentTemplate = NULL;
    }

  /* comment, if any */
  currentClass->Comment = vtkstrdup(getComment());

  access_level = VTK_ACCESS_PRIVATE;
  if (is_struct_or_union)
    {
    access_level = VTK_ACCESS_PUBLIC;
    }

  vtkParse_InitFunction(currentFunction);
  startSig();
  clearComment();
}

/* reject the class */
void reject_class(const char *classname, int is_struct_or_union)
{
  static ClassInfo static_class;

  pushClass();
  currentClass = &static_class;
  currentClass->Name = vtkstrdup(classname);
  vtkParse_InitClass(currentClass);

  access_level = VTK_ACCESS_PRIVATE;
  if (is_struct_or_union)
    {
    access_level = VTK_ACCESS_PUBLIC;
    }

  vtkParse_InitFunction(currentFunction);
  startSig();
  clearComment();
}

/* reached the end of a class definition */
void end_class()
{
  /* add default constructors */
  vtkParse_AddDefaultConstructors(currentClass);

  popClass();
}

/* add a base class to the specified class */
void add_base_class(ClassInfo *cls, const char *name, int al, int virt)
{
  if (al == VTK_ACCESS_PUBLIC && virt == 0)
    {
    vtkParse_AddStringToArray(&cls->SuperClasses,
                              &cls->NumberOfSuperClasses,
                              vtkstrdup(name));
    }
}

/* add a using declaration or directive */
void add_using(const char *name, int is_namespace)
{
  size_t i;
  UsingInfo *item;

  item = (UsingInfo *)malloc(sizeof(UsingInfo));
  vtkParse_InitUsing(item);
  if (is_namespace)
    {
    item->Name = NULL;
    item->Scope = vtkstrdup(name);
    }
  else
    {
    i = strlen(name);
    while (i > 0 && name[i-1] != ':') { i--; }
    item->Name = vtkstrdup(&name[i]);
    while (i > 0 && name[i-1] == ':') { i--; }
    item->Scope = vtkstrndup(name, i);
    }

  if (currentClass)
    {
    vtkParse_AddUsingToClass(currentClass, item);
    }
  else
    {
    vtkParse_AddUsingToNamespace(currentNamespace, item);
    }
}

/* start a new enum */
void start_enum(const char *name)
{
  static char text[256];
  EnumInfo *item;

  currentEnumName = "int";
  currentEnumValue = NULL;
  if (name)
    {
    strcpy(text, name);
    currentEnumName = text;
    item = (EnumInfo *)malloc(sizeof(EnumInfo));
    vtkParse_InitEnum(item);
    item->Name = vtkstrdup(name);
    item->Access = access_level;
    if (currentClass)
      {
      vtkParse_AddEnumToClass(currentClass, item);
      }
    else
      {
      vtkParse_AddEnumToNamespace(currentNamespace, item);
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
  static char text[2048];
  int i;
  long j;

  if (value)
    {
    strcpy(text, value);
    currentEnumValue = text;
    }
  else if (currentEnumValue)
    {
    i = strlen(text);
    while (i > 0 && text[i-1] >= '0' &&
           text[i-1] <= '9') { i--; }

    if (i == 0 || text[i-1] == ' ' ||
        (i > 1 && text[i-2] == ' ' &&
         (text[i-1] == '-' || text[i-1] == '+')))
      {
      if (i > 0 && text[i-1] != ' ')
        {
        i--;
        }
      j = (int)strtol(&text[i], NULL, 10);
      sprintf(&text[i], "%li", j+1);
      }
    else
      {
      i = strlen(text);
      strcpy(&text[i], " + 1");
      }
    }
  else
    {
    strcpy(text, "0");
    currentEnumValue = text;
    }

  add_constant(name, currentEnumValue, VTK_PARSE_INT, currentEnumName, 2);
}

/* for a macro constant, guess the constant type, doesn't do any math */
unsigned int guess_constant_type(const char *valstring)
{
  unsigned int valtype = 0;
  size_t k;
  int i;
  int is_name = 0;

  if (valstring == NULL || valstring[0] == '\0')
    {
    return 0;
    }

  if (valstring[0] < '0' || valstring[0] > '9')
    {
    k = 0;
    while ((valstring[k] >= '0' && valstring[k] <= '9') ||
           (valstring[k] >= 'a' && valstring[k] <= 'z') ||
           (valstring[k] >= 'A' && valstring[k] <= 'Z') ||
           valstring[k] == '_') { k++; }

    if (valstring[k] == '\0')
      {
      is_name = 1;
      }
    }

  if (strcmp(valstring, "true") == 0 || strcmp(valstring, "false") == 0)
    {
    return VTK_PARSE_BOOL;
    }

  if (valstring[0] == '\'')
    {
    return VTK_PARSE_CHAR;
    }

  if (strncmp(valstring, "VTK_TYPE_CAST(", 14) == 0 ||
      strncmp(valstring, "static_cast<", 12) == 0 ||
      strncmp(valstring, "const_cast<", 11) == 0 ||
      strncmp(valstring, "(", 1) == 0)
    {
    const char *cp;
    size_t n;
    int is_unsigned = 0;

    cp = &valstring[1];
    if (valstring[0] == 'c')
      {
      cp = &valstring[11];
      }
    else if (valstring[0] == 's')
      {
      cp = &valstring[12];
      }
    else if (valstring[0] == 'V')
      {
      cp = &valstring[14];
      }

    if (strncmp(cp, "unsigned ", 9) == 0)
      {
      is_unsigned = 1;
      cp += 9;
      }

    n = strlen(cp);
    for (k = 0; k < n && cp[k] != ',' &&
         cp[k] != '>' && cp[k] != ')'; k++) { ; };

    if (strncmp(cp, "long long", k) == 0)
      { valtype = VTK_PARSE_LONG_LONG; }
    else if (strncmp(cp, "__int64", k) == 0)
      { valtype = VTK_PARSE___INT64; }
    else if (strncmp(cp, "long", k) == 0)
      { valtype = VTK_PARSE_LONG; }
    else if (strncmp(cp, "short", k) == 0)
      { valtype = VTK_PARSE_SHORT; }
    else if (strncmp(cp, "signed char", k) == 0)
      { valtype = VTK_PARSE_SIGNED_CHAR; }
    else if (strncmp(cp, "char", k) == 0)
      { valtype = VTK_PARSE_CHAR; }
    else if (strncmp(cp, "int", k) == 0 ||
             strncmp(cp, "signed", k) == 0)
      { valtype = VTK_PARSE_INT; }
    else if (strncmp(cp, "float", k) == 0)
      { valtype = VTK_PARSE_FLOAT; }
    else if (strncmp(cp, "double", k) == 0)
      { valtype = VTK_PARSE_DOUBLE; }
    else if (strncmp(cp, "char *", k) == 0)
      { valtype = VTK_PARSE_CHAR_PTR; }

    if (is_unsigned)
      {
      if (valtype == 0) { valtype = VTK_PARSE_INT; }
      valtype = (valtype | VTK_PARSE_UNSIGNED);
      }

    if (valtype != 0)
      {
      return valtype;
      }
    }

  /* check the current scope */
  if (is_name)
    {
    NamespaceInfo *scope = currentNamespace;
    if (namespaceDepth > 0)
      {
      scope = namespaceStack[0];
      }

    for (i = 0; i < scope->NumberOfConstants; i++)
      {
      if (strcmp(scope->Constants[i]->Name, valstring) == 0)
        {
        return scope->Constants[i]->Type;
        }
      }
    }

  /* check for preprocessor macros */
  if (is_name)
    {
    MacroInfo *macro = vtkParsePreprocess_GetMacro(
      &preprocessor, valstring);

    if (macro && !macro->IsFunction)
      {
      return guess_constant_type(macro->Definition);
      }
    }

  /* fall back to the preprocessor to evaluate the constant */
    {
    preproc_int_t val;
    int is_unsigned;
    int result = vtkParsePreprocess_EvaluateExpression(
      &preprocessor, valstring, &val, &is_unsigned);

    if (result == VTK_PARSE_PREPROC_DOUBLE)
      {
      return VTK_PARSE_DOUBLE;
      }
    else if (result == VTK_PARSE_PREPROC_FLOAT)
      {
      return VTK_PARSE_FLOAT;
      }
    else if (result == VTK_PARSE_PREPROC_STRING)
      {
      return VTK_PARSE_CHAR_PTR;
      }
    else if (result == VTK_PARSE_OK)
      {
      if (is_unsigned)
        {
        if ((preproc_uint_t)val <= VTK_UNSIGNED_INT_MAX)
          {
          return VTK_PARSE_UNSIGNED_INT;
          }
        else
          {
#if defined(VTK_TYPE_USE_LONG_LONG)
          return VTK_PARSE_UNSIGNED_LONG_LONG;
#elif defined(VTK_TYPE_USE___INT64)
          return VTK_PARSE_UNSIGNED___INT64;
#else
          return VTK_PARSE_UNSIGNED_LONG;
#endif
          }
        }
      else
        {
        if (val >= VTK_INT_MIN && val <= VTK_INT_MAX)
          {
          return VTK_PARSE_INT;
          }
        else
          {
#if defined(VTK_TYPE_USE_LONG_LONG)
          return VTK_PARSE_LONG_LONG;
#elif defined(VTK_TYPE_USE___INT64)
          return VTK_PARSE___INT64;
#else
          return VTK_PARSE_LONG;
#endif
          }
        }
      }
    }

  return 0;
}

/* add a constant to the current class or namespace */
void add_constant(const char *name, const char *value,
                  unsigned int type, const char *typeclass, int flag)
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
    /* actually a macro, need to guess the type */
    ValueInfo **cptr = data->Contents->Constants;
    int n = data->Contents->NumberOfConstants;
    int i;

    con->Access = VTK_ACCESS_PUBLIC;
    if (con->Type == 0)
      {
      con->Type = guess_constant_type(con->Value);
      }

    for (i = 0; i < n; i++)
      {
      if (strcmp(cptr[i]->Name, con->Name) == 0)
        {
        break;
        }
      }

    if (i == n)
      {
      vtkParse_AddConstantToNamespace(data->Contents, con);
      }
    else
      {
      vtkParse_FreeValue(con);
      }
    }
  else if (currentClass)
    {
    con->Access = access_level;
    vtkParse_AddConstantToClass(currentClass, con);
    }
  else
    {
    con->Access = VTK_ACCESS_PUBLIC;
    vtkParse_AddConstantToNamespace(currentNamespace, con);
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

/* add a template argument to the current template */
void add_template_arg(
  unsigned int datatype, unsigned int extra, const char *funcSig)
{
  ValueInfo val;
  TemplateArg *arg = (TemplateArg *)malloc(sizeof(TemplateArg));
  vtkParse_InitTemplateArg(arg);
  vtkParse_InitValue(&val);
  handle_complex_type(&val, datatype, extra, funcSig);
  arg->Type = val.Type;
  arg->Class = val.Class;
  arg->Function = val.Function;
  arg->NumberOfDimensions = val.NumberOfDimensions;
  arg->Dimensions = val.Dimensions;
  if (getVarName())
    {
    arg->Name = vtkstrdup(getVarName());
    }
  vtkParse_AddArgumentToTemplate(currentTemplate, arg);
}

/* add an arg to a function */
void add_argument(FunctionInfo *func, unsigned int type,
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
    vtkParse_AddStringToArray(&arg->Dimensions, &arg->NumberOfDimensions,
                              vtkstrdup(text));
    }

  func->ArgTypes[i] = arg->Type;
  func->ArgClasses[i] = arg->Class;
  func->ArgCounts[i] = count;

  vtkParse_AddArgumentToFunction(func, arg);
}

/* set the return type for the current function */
void set_return(FunctionInfo *func, unsigned int type,
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
    vtkParse_AddStringToArray(&val->Dimensions, &val->NumberOfDimensions,
                              vtkstrdup(text));
    func->HaveHint = 1;
    }

  func->ReturnValue = val;
  func->ReturnType = val->Type;
  func->ReturnClass = val->Class;
  func->HintSize = count;
}

int count_from_dimensions(ValueInfo *val)
{
  int count, i, n;
  const char *cp;

  /* count is the product of the dimensions */
  count = 0;
  if (val->NumberOfDimensions)
    {
    count = 1;
    for (i = 0; i < val->NumberOfDimensions; i++)
      {
      n = 0;
      cp = val->Dimensions[i];
      if (cp[0] != '\0')
        {
        while (*cp != '\0' && *cp >= '0' && *cp <= '9') { cp++; }
        while (*cp != '\0' && (*cp == 'u' || *cp == 'l' ||
                               *cp == 'U' || *cp == 'L')) { cp++; }
        if (*cp == '\0')
          {
          n = (int)strtol(val->Dimensions[i], NULL, 0);
          }
        }
      count *= n;
      }
    }

  return count;
}

/* deal with types that include function pointers or arrays */
void handle_complex_type(
  ValueInfo *val, unsigned int datatype, unsigned int extra,
  const char *funcSig)
{
  FunctionInfo *func = 0;

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
    if (funcSig) { func->Signature = vtkstrdup(funcSig); }
    val->Function = func;

    /* the val type is whatever was inside the parentheses */
    clearTypeId();
    setTypeId(func->Class ? "method" : "function");
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
      datatype = add_indirection(datatype, VTK_PARSE_POINTER);
      }
    else
      {
      pushArrayFront("");
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
  clearArray();

  /* count is the product of the dimensions */
  val->Count = count_from_dimensions(val);
}

/* specifically handle a VAR_FUNCTION argument */
void handle_function_type(
  ValueInfo *arg, const char *name, const char *funcSig)
{
  FunctionInfo *func;
  size_t j;

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
  j = strlen(funcSig);
  while (j > 0 && funcSig[j-1] == ' ')
    {
    j--;
    }

  func->Signature = vtkstrndup(funcSig, j);
  arg->Function = func;
}


/* reject the function, do not output it */
void reject_function()
{
  vtkParse_InitFunction(currentFunction);
  startSig();
}

/* a simple routine that updates a few variables */
void output_function()
{
  const char *macro = getMacro();
  size_t n;
  int i, j;
  int match;

  /* reject template specializations */
  n = strlen(currentFunction->Name);
  if (currentFunction->Name[n-1] == '>')
    {
    /* make sure there is a matching angle bracket */
    while (n > 0 && currentFunction->Name[n-1] != '<') { n--; }
    if (n > 0)
      {
      reject_function();
      return;
      }
    }

  /* static */
  if (currentFunction->ReturnType & VTK_PARSE_STATIC)
    {
    currentFunction->IsStatic = 1;
    }

  /* virtual */
  if (currentFunction->ReturnType & VTK_PARSE_VIRTUAL)
    {
    currentFunction->IsVirtual = 1;
    }

  /* explicit */
  if (currentFunction->ReturnType & VTK_PARSE_EXPLICIT)
    {
    currentFunction->IsExplicit = 1;
    }

  /* the signature */
  if (!currentFunction->Signature)
    {
    currentFunction->Signature = getSig();
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

  /* is it defined in a legacy macro? */
  if (macro && strcmp(macro, "VTK_LEGACY") == 0)
    {
    currentFunction->IsLegacy = 1;
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
    if (currentFunction->NumberOfArguments != 2 ||
        currentFunction->Arguments[1]->Type != VTK_PARSE_VOID_PTR)
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

    currentFunction->Class = vtkstrdup(currentClass->Name);
    vtkParse_AddFunctionToClass(currentClass, currentFunction);

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
      vtkParse_AddFunctionToNamespace(currentNamespace, currentFunction);

      currentFunction = (FunctionInfo *)malloc(sizeof(FunctionInfo));
      }
    }

  vtkParse_InitFunction(currentFunction);
  startSig();
}

/* output a function that is not a method of the current class */
void output_friend_function()
{
  ClassInfo *tmpc = currentClass;
  currentClass = NULL;
  output_function();
  currentClass = tmpc;
}

void outputSetVectorMacro(const char *var, unsigned int argType,
                          const char *typeText, int n)
{
  static const char *mnames[] = {
    NULL, NULL,
    "vtkSetVector2Macro", "vtkSetVector3Macro", "vtkSetVector4Macro",
    NULL,
    "vtkSetVector6Macro",
    NULL };
  char ntext[32];
  int i, m;
  m = (n > 7 ? 0 : n);

  sprintf(ntext, "%i", n);

  currentFunction->Macro = mnames[m];
  currentFunction->Name = vtkstrcat("Set", var);
  startSig();
  postSig("void ");
  postSig(currentFunction->Name);
  postSig("(");
  postSig(typeText);
  for (i = 1; i < n; i++)
    {
    postSig(", ");
    postSig(typeText);
    }
  postSig(");");
  for (i = 0; i < n; i++)
    {
    add_argument(currentFunction, argType, getTypeId(), 0);
    }
  set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
  output_function();

  currentFunction->Macro = mnames[m];
  currentFunction->Name = vtkstrcat("Set", var);
  currentFunction->Signature =
    vtkstrcat7("void ", currentFunction->Name, "(", getTypeId(),
               " a[", ntext, "]);");
  add_argument(currentFunction, (VTK_PARSE_POINTER | argType),
               getTypeId(), n);
  set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
  output_function();
}

void outputGetVectorMacro(const char *var, unsigned int argType,
                          const char *typeText, int n)
{
  static const char *mnames[] = {
    NULL, NULL,
    "vtkGetVector2Macro", "vtkGetVector3Macro", "vtkGetVector4Macro",
    NULL,
    "vtkGetVector6Macro",
    NULL };
  int m;
  m = (n > 7 ? 0 : n);

  currentFunction->Macro = mnames[m];
  currentFunction->Name = vtkstrcat("Get", var);
  currentFunction->Signature =
    vtkstrcat4(typeText, " *", currentFunction->Name, "();");
  set_return(currentFunction, (VTK_PARSE_POINTER | argType), getTypeId(), n);
  output_function();
}

/* Set a flag to ignore BTX/ETX markers in the files */
void vtkParse_SetIgnoreBTX(int option)
{
  if (option)
    {
    IgnoreBTX = 1;
    }
  else
    {
    IgnoreBTX = 0;
    }
}

/* Parse a header file and return a FileInfo struct */
FileInfo *vtkParse_ParseFile(
  const char *filename, FILE *ifile, FILE *errfile)
{
  int i, j;
  int ret;
  FileInfo *file_info;
  char *main_class;
  const char **include_dirs;

  /* "data" is a global variable used by the parser */
  data = (FileInfo *)malloc(sizeof(FileInfo));
  vtkParse_InitFile(data);

  /* "preprocessor" is a global struct used by the parser */
  i = preprocessor.NumberOfIncludeDirectories;
  include_dirs = preprocessor.IncludeDirectories;
  preprocessor.NumberOfIncludeDirectories = 0;
  preprocessor.IncludeDirectories = NULL;
  vtkParsePreprocess_InitPreprocess(&preprocessor);
  vtkParsePreprocess_AddStandardMacros(&preprocessor, VTK_PARSE_NATIVE);
  preprocessor.FileName = vtkstrdup(filename);
  preprocessor.NumberOfIncludeDirectories = i;
  preprocessor.IncludeDirectories = include_dirs;
  /* should explicitly check for vtkConfigure.h, or even explicitly load it */
#ifdef VTK_USE_64BIT_IDS
  vtkParsePreprocess_AddMacro(&preprocessor, "VTK_USE_64BIT_IDS", NULL);
#endif

  data->FileName = vtkstrdup(filename);

  clearComment();

  namespaceDepth = 0;
  currentNamespace = (NamespaceInfo *)malloc(sizeof(NamespaceInfo));
  vtkParse_InitNamespace(currentNamespace);
  data->Contents = currentNamespace;

  templateDepth = 0;
  currentTemplate = NULL;

  currentFunction = (FunctionInfo *)malloc(sizeof(FunctionInfo));
  vtkParse_InitFunction(currentFunction);
  startSig();

  parseDebug = 0;
  if (getenv("DEBUG") != NULL)
    {
    parseDebug = 1;
    }

  yyset_in(ifile);
  yyset_out(errfile);
  ret = yyparse();

  if (ret)
    {
    print_parser_error("syntax error", NULL, 0);
    return NULL;
    }

  free(currentFunction);
  yylex_destroy();

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
      data->MainClass = currentNamespace->Classes[i];
      break;
      }
    }

  free(main_class);

  file_info = data;
  data = NULL;

  return file_info;
}

/* Read a hints file and update the FileInfo */
int vtkParse_ReadHints(FileInfo *file_info, FILE *hfile, FILE *errfile)
{
  char h_cls[512];
  char h_func[512];
  unsigned int h_type, type;
  int h_value;
  FunctionInfo *func_info;
  ClassInfo *class_info;
  NamespaceInfo *contents;
  int i, j;
  int lineno = 0;
  int n;

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
    type = ((h_type & VTK_PARSE_BASE_TYPE) |
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
              (type == ((func_info->ReturnType & ~VTK_PARSE_REF) &
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
              case VTK_PARSE_CHAR_PTR:
                {
                if (func_info->ReturnValue &&
                    func_info->ReturnValue->NumberOfDimensions == 0)
                  {
                  char text[64];
                  func_info->HaveHint = 1;
                  func_info->HintSize = h_value;
                  func_info->ReturnValue->Count = h_value;
                  sprintf(text, "%i", h_value);
                  vtkParse_AddStringToArray(
                    &func_info->ReturnValue->Dimensions,
                    &func_info->ReturnValue->NumberOfDimensions,
                    vtkstrdup(text));
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
  vtkParse_FreeFile(file_info);
  vtkParse_FreeStrings();
}

/* Set a property before parsing */
void vtkParse_SetClassProperty(
  const char *classname, const char *property)
{
   /* the only property recognized */
   if (strcmp(property, "concrete") == 0 ||
       strcmp(property, "CONCRETE") == 0 ||
       strcmp(property, "Concrete") == 0)
     {
     vtkParse_AddStringToArray(&ConcreteClasses,
                               &NumberOfConcreteClasses,
                               vtkstrdup(classname));
     }
}

/** Define a preprocessor macro. Function macros are not supported.  */
void vtkParse_DefineMacro(const char *name, const char *definition)
{
  vtkParsePreprocess_AddMacro(&preprocessor, name, definition);
}

/** Undefine a preprocessor macro.  */
void vtkParse_UndefineMacro(const char *name)
{
  vtkParsePreprocess_RemoveMacro(&preprocessor, name);
}

/** Add an include directory, for use with the "-I" option.  */
void vtkParse_IncludeDirectory(const char *dirname)
{
  vtkParsePreprocess_IncludeDirectory(&preprocessor, dirname);
}

/** Return the full path to a header file.  */
const char *vtkParse_FindIncludeFile(const char *filename)
{
  int val;
  return vtkParsePreprocess_FindIncludeFile(&preprocessor, filename, 0, &val);
}
