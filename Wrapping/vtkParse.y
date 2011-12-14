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
#include "vtkParsePreprocess.h"
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
FileInfo data;

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

/* helper functions */
void start_class(const char *classname, int is_struct_or_union);
void reject_class(const char *classname, int is_struct_or_union);
void end_class();
void output_function(void);
void reject_function(void);
void set_return(FunctionInfo *func, unsigned int type,
                const char *typeclass, int count);
void add_argument(FunctionInfo *func, unsigned int type,
                  const char *classname, int count);
void add_using(const char *name, int is_namespace);
void start_enum(const char *enumname);
void add_enum(const char *name, const char *value);
void end_enum();
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
 * then, vtkstrfree() can be called.
 */

size_t stringChunkPos = 0;
int numberOfChunks = 0;
char **stringArray = NULL;

/* allocate a string of n+1 bytes */
static char *vtkstralloc(size_t n)
{
  size_t chunk_size = 8176;
  size_t nextChunkPos;
  char *cp;

  // align next start position on an 8-byte boundary
  nextChunkPos = (((stringChunkPos + n + 8) | 7 ) - 7);

  if (numberOfChunks == 0 || nextChunkPos > chunk_size)
    {
    if (n + 1 > chunk_size)
      {
      chunk_size = n + 1;
      }
    cp = (char *)malloc(chunk_size);
    vtkParse_AddStringToArray((const char ***)&stringArray, &numberOfChunks,
                              cp);
    stringChunkPos = 0;
    nextChunkPos = (((n + 8) | 7) - 7);
    }

  cp = &stringArray[numberOfChunks-1][stringChunkPos];
  cp[0] = '\0';

  stringChunkPos = nextChunkPos;

  return cp;
}

/* free all allocated strings */
void vtkstrfree()
{
  int i;

  for (i = 0; i < numberOfChunks; i++)
    {
    free(stringArray[i]);
    }
  if (stringArray)
    {
    free(stringArray);
    }

  stringArray = NULL;
  numberOfChunks = 0;
}

/* duplicate the first n bytes of a string and terminate */
static const char *vtkstrndup(const char *in, size_t n)
{
  char *res = NULL;

  res = vtkstralloc(n);
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
  cp = vtkstralloc(m);
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

static const char *vtkstrcat6(const char *str1, const char *str2,
                              const char *str3, const char *str4,
                              const char *str5, const char *str6)
{
  const char *cp[6];

  cp[0] = str1;
  cp[1] = str2;
  cp[2] = str3;
  cp[3] = str4;
  cp[4] = str5;
  cp[5] = str6;
  return vtkstrncat(6, cp);
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
      data.Description = vtkstrdup(getComment());
      clearComment();
      break;
    case 3:
      data.SeeAlso = vtkstrdup(getComment());
      clearComment();
      break;
    case 4:
      data.Caveats = vtkstrdup(getComment());
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

/* reallocate Signature if n chars cannot be appended */
void checkSigSize(size_t n)
{
  const char *ccp;

  if (sigAllocatedLength == 0)
    {
    sigAllocatedLength = 80 + n;
    signature = vtkstralloc(sigAllocatedLength);
    signature[0] = '\0';
    }
  else if (sigLength + n > sigAllocatedLength)
    {
    sigAllocatedLength += sigLength + n;
    ccp = signature;
    signature = vtkstralloc(sigAllocatedLength);
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
  size_t n;

  n = strlen(arg);

  if (!signature)
    {
    checkSigSize(n);
    strncpy(signature, arg, n);
    signature[n] = '\0';
    sigLength = n;
    }
  else if (!sigClosed && n > 0)
    {
    checkSigSize(n);
    memmove(&signature[n], signature, sigLength);
    strncpy(signature, arg, n);
    sigLength += n;
    signature[sigLength] = '\0';
    }
}

/* append text to the end of the signature */
void postSig(const char *arg)
{
  size_t n;

  n = strlen(arg);

  if (!signature)
    {
    checkSigSize(n);
    strncpy(signature, arg, n);
    signature[n] = '\0';
    sigLength = n;
    }
  else if (!sigClosed)
    {
    checkSigSize(n);
    strncpy(&signature[sigLength], arg, n);
    sigLength += n;
    signature[sigLength] = '\0';
    }
}

/* prepend a scope:: to the ID at the end of the signature */
void preScopeSig(const char *arg)
{
  size_t n;

  n = strlen(arg);

  if (!signature)
    {
    checkSigSize(n);
    strncpy(signature, arg, n);
    signature[n] = '\0';
    sigLength = n;
    }
  else if (!sigClosed)
    {
    checkSigSize(n+2);
    prepend_scope(signature, arg);
    sigLength = strlen(signature);
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

/* mark this signature as legacy */
void legacySig(void)
{
  currentFunction->IsLegacy = 1;
}

/*----------------------------------------------------------------
 * Storage type for vars and functions
 */

/* "private" variables */
unsigned int storageType = 0;

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


/* The parser will shift/reduce values <str> or <integer> */

%union{
  const char   *str;
  unsigned int  integer;
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
%token VOID
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
%token EXPLICIT
%token STATIC_CAST
%token DYNAMIC_CAST
%token CONST_CAST
%token REINTERPRET_CAST
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
%token <str> QT_ID

/* type tokens */
%token <str> StdString
%token <str> UnicodeString
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
%token VTK_BYTE_SWAP_DECL

%%
/*
 * Here is the start of the grammer
 */

strt: | strt
        { startSig(); clearTypeId(); clearTemplate(); closeComment(); }
        file_item;

file_item:
     var
   | enum_def maybe_vars ';'
   | using
   | namespace
   | extern
   | type_def
   | class_def maybe_vars ';'
   | template class_def maybe_vars ';'
   | CLASS_REF
   | operator func_body { output_function(); }
   | template operator func_body { output_function(); }
   | scoped_operator func_body { reject_function(); }
   | function func_body { output_function(); }
   | scoped_method func_body { reject_function(); }
   | template function func_body { output_function(); }
   | legacy_function func_body { legacySig(); output_function(); }
   | macro
   | class_id ';'
   | ID '(' maybe_other ')'
   | VTK_ID '(' maybe_other ')'
   | QT_ID '(' maybe_other ')'
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
  | CLASS any_id '<' template_params '>' { reject_class($<str>2, 0); }
    optional_scope '{' class_def_body '}' { end_class(); }
  | STRUCT any_id { start_class($<str>2, 1); }
    optional_scope '{' class_def_body '}' { end_class(); }
  | STRUCT any_id '<' template_params '>' { reject_class($<str>2, 1); }
    optional_scope '{' class_def_body '}' { end_class(); }
  | STRUCT '{' maybe_other '}'
  | UNION any_id { start_class($<str>2, 2); }
    optional_scope '{' class_def_body '}' { end_class(); }
  | UNION any_id '<' template_params '>' { reject_class($<str>2, 2); }
    optional_scope '{' class_def_body '}' { end_class(); }
  | UNION '{' maybe_other '}';

class_def_body:
    | class_def_body
      { startSig(); clearTypeId(); clearTemplate();  closeComment(); }
      class_def_item;
    | class_def_body scope_type ':';

class_def_item:
     var
   | enum_def maybe_vars ';'
   | using
   | type_def
   | class_def maybe_vars ';'
   | template class_def maybe_vars ';'
   | FRIEND internal_class
   | FRIEND template_internal_class
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
   | ';';

optional_scope: | ':' scope_list;

scope_list: scope_list_item | scope_list_item ',' scope_list;

scope_list_item: maybe_scoped_id
  | PRIVATE maybe_scoped_id
  | PROTECTED maybe_scoped_id
  | PUBLIC maybe_scoped_id
    {
      vtkParse_AddStringToArray(&currentClass->SuperClasses,
                                &currentClass->NumberOfSuperClasses,
                                vtkstrdup($<str>2));
    };

scope_type: PUBLIC {access_level = VTK_ACCESS_PUBLIC;}
          | PRIVATE {access_level = VTK_ACCESS_PRIVATE;}
          | PROTECTED {access_level = VTK_ACCESS_PROTECTED;};

/*
 * Enums
 *
 * The values assigned to enum constants are handled as strings.
 * The text can be dropped into the generated .cxx file and evaluated there,
 * as long as all IDs are properly scoped.
 */

enum_def: ENUM any_id {start_enum($<str>2);} '{' enum_list '}'
   {end_enum();}
 | ENUM {start_enum(NULL);} '{' enum_list '}'
   {end_enum();};

enum_list: | enum_item | enum_item ',' enum_list;

enum_item: any_id {add_enum($<str>1, NULL);}
         | any_id '=' integer_expression {add_enum($<str>1, $<str>3);};

integer_value: integer_literal {postSig($<str>1);}
             | any_id {$<str>$ = vtkstrdup(add_const_scope($<str>1));}
             | scoped_id | templated_id;

integer_literal: ZERO | INT_LITERAL | OCT_LITERAL | HEX_LITERAL | CHAR_LITERAL;

integer_expression: integer_value { $<str>$ = $<str>1; }
     | math_unary_op {postSig($<str>1);} integer_expression
       {
         $<str>$ = vtkstrcat($<str>1, $<str>3);
       }
     | integer_value math_binary_op {postSig($<str>2);} integer_expression
       {
         $<str>$ = vtkstrcat5($<str>1, " ", $<str>2, " ", $<str>4);
       }
     | '(' {postSig("(");} integer_expression ')'
       {
         $<str>$ = vtkstrcat3("(", $<str>3, ")");
       };

math_unary_op:   '-' { $<str>$ = "-"; } | '+' { $<str>$ = "+"; }
               | '~' { $<str>$ = "~"; };

math_binary_op:  '-' { $<str>$ = "-"; } | '+' { $<str>$ = "+"; }
               | '*' { $<str>$ = "*"; } | '/' { $<str>$ = "/"; }
               | '%' { $<str>$ = "%"; } | '&' { $<str>$ = "&"; }
               | '|' { $<str>$ = "|"; } | '^' { $<str>$ = "^"; }
               | OP_LSHIFT { $<str>$ = ">>"; }
               | OP_RSHIFT { $<str>$ = "<<"; };

/*
 * currently ignored items
 */

template_internal_class: template internal_class;

internal_class: CLASS any_id internal_class_body
              | STRUCT any_id internal_class_body
              | STRUCT internal_class_body
              | UNION any_id internal_class_body
              | UNION internal_class_body;

internal_class_body: ';'
    | '{' maybe_other '}' maybe_other_no_semi ';'
    | ':' maybe_other_no_semi ';';

/*
 * Typedefs
 */

type_def: typedef_start type complex_var_id ';'
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
 | typedef_start class_def maybe_indirect_id ';' { }
 | typedef_start enum_def maybe_indirect_id ';' { }
 | typedef_start VAR_FUNCTION ';' { };

typedef_start: TYPEDEF { };


/*
 * The "using" keyword
 */

using: USING NAMESPACE maybe_scoped_id ';' { add_using($<str>3, 1); }
     | USING TYPENAME maybe_scoped_id ';'  { add_using($<str>3, 0); }
     | USING maybe_scoped_id ';' { add_using($<str>2, 0); };


/*
 * Templates
 */

template: TEMPLATE '<' '>' { postSig("template<> "); clearTypeId(); }
        | TEMPLATE '<' { postSig("template<");
          clearTypeId(); startTemplate(); }
          template_args '>' { chopSig();
            if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
            postSig("> "); clearTypeId(); };

template_args: template_arg
             | template_arg ',' { chopSig(); postSig(", "); clearTypeId(); }
               template_args;

template_arg: type_simple maybe_template_id
               {
               TemplateArg *arg = (TemplateArg *)malloc(sizeof(TemplateArg));
               vtkParse_InitTemplateArg(arg);
               arg->Type = $<integer>1;
               arg->Class = vtkstrdup(getTypeId());
               arg->Name = vtkstrdup(getVarName());
               arg->Value = vtkstrdup(getVarValue());
               vtkParse_AddArgumentToTemplate(currentTemplate, arg);
               }
            | class_or_typename maybe_template_id
               {
               TemplateArg *arg = (TemplateArg *)malloc(sizeof(TemplateArg));
               vtkParse_InitTemplateArg(arg);
               arg->Name = vtkstrdup(getVarName());
               arg->Value = vtkstrdup(getVarValue());
               vtkParse_AddArgumentToTemplate(currentTemplate, arg);
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
               vtkParse_AddArgumentToTemplate(currentTemplate, arg);
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
      | explicit_mod constructor
         {
         openSig();
         preSig("explicit ");
         closeSig();
         currentFunction->IsExplicit = 1;
         }
      | storage_type func
      | VIRTUAL storage_type func
         {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->IsVirtual = 1;
         };

explicit_mod: EXPLICIT | INLINE EXPLICIT | EXPLICIT INLINE;

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
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed operator", "operator typecast");
    };

op_func: op_sig { postSig(")"); } func_trailer
    {
      postSig(";");
      closeSig();
      currentFunction->Name = vtkstrdup($<str>1);
      currentFunction->Comment = vtkstrdup(getComment());
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
      currentFunction->Name = vtkstrdup($<str>1);
      currentFunction->Comment = vtkstrdup(getComment());
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
  | any_id '<' {markSig(); postSig("<");} template_params '>' '('
    {
      if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
      $<str>$ = vtkstrcat($<str>1, copySig());
    } args_list ')' { $<str>$ = $<str>7; };

constructor: constructor_sig { postSig(");"); closeSig(); } maybe_initializers
    {
      currentFunction->Name = vtkstrdup($<str>1);
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    };

constructor_sig: any_id '(' { postSig("("); } args_list ')';

maybe_initializers: | ':' initializer more_initializers;

more_initializers: | ',' initializer more_initializers;

initializer: maybe_scoped_id '(' maybe_other ')';

destructor: destructor_sig
    {
      postSig(");");
      closeSig();
      currentFunction->Name = vtkstrcat("~", $<str>1);
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    };

destructor_sig: any_id '(' { postSig("(");} args_list ')';

/*
 * Arguments
 */

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

      vtkParse_AddArgumentToFunction(currentFunction, arg);
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

      vtkParse_AddArgumentToFunction(currentFunction, arg);
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

maybe_vars:
     | other_var maybe_other_vars;

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
       {
         const char *scope = getScope();
         unsigned int parens = add_indirection($<integer>1, $<integer>2);
         if ($<integer>5 == VTK_PARSE_FUNCTION) {
           if (scope) { scope = vtkstrndup(scope, strlen(scope) - 2); }
           getFunction()->Class = scope;
           $<integer>$ = (parens | VTK_PARSE_FUNCTION); }
         else if ($<integer>5 == VTK_PARSE_ARRAY) {
           $<integer>$ = add_indirection_to_array(parens); }
       };

/* for vars, the var_id is mandatory */
complex_var_id: var_id maybe_var_array { $<integer>$ = 0; }
     | lp_or_la maybe_indirect_var_id ')' { postSig(")"); }
       maybe_array_or_args
       {
         const char *scope = getScope();
         unsigned int parens = add_indirection($<integer>1, $<integer>2);
         if ($<integer>5 == VTK_PARSE_FUNCTION) {
           if (scope) { scope = vtkstrndup(scope, strlen(scope) - 2); }
           getFunction()->Class = scope;
           $<integer>$ = (parens | VTK_PARSE_FUNCTION); }
         else if ($<integer>5 == VTK_PARSE_ARRAY) {
           $<integer>$ = add_indirection_to_array(parens); }
       };

p_or_lp_or_la: '(' { postSig("("); scopeSig(""); $<integer>$ = 0; }
        | LP { postSig("("); scopeSig($<str>1); postSig("*");
               $<integer>$ = VTK_PARSE_POINTER; }
        | LA { postSig("("); scopeSig($<str>1); postSig("&");
               $<integer>$ = VTK_PARSE_REF; };

lp_or_la: LP { postSig("("); scopeSig($<str>1); postSig("*");
               $<integer>$ = VTK_PARSE_POINTER; }
        | LA { postSig("("); scopeSig($<str>1); postSig("&");
               $<integer>$ = VTK_PARSE_REF; };

maybe_func_const: | CONST { currentFunction->IsConst = 1; };

maybe_array_or_args: { $<integer>$ = 0; }
   | '(' { pushFunction(); postSig("("); } args_list ')' maybe_func_const
     { $<integer>$ = VTK_PARSE_FUNCTION; postSig(")"); popFunction(); }
   | var_array { $<integer>$ = VTK_PARSE_ARRAY; };

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
     | QT_ID {postSig($<str>1);}
     | ID {postSig($<str>1);}
     | ISTREAM {postSig($<str>1);}
     | OSTREAM {postSig($<str>1);}
     | StdString {postSig($<str>1);}
     | UnicodeString {postSig($<str>1);}
     | sized_type_id {postSig($<str>1);}
     | special_type_id {postSig($<str>1);};

sized_type_id:
       TypeInt8 {$<str>$ = "vtkTypeInt8";}
     | TypeUInt8 {$<str>$ = "vtkTypeUInt8";}
     | TypeInt16 {$<str>$ = "vtkTypeInt16";}
     | TypeUInt16 {$<str>$ = "vtkTypeUInt16";}
     | TypeInt32 {$<str>$ = "vtkTypeInt32";}
     | TypeUInt32 {$<str>$ = "vtkTypeUInt32";}
     | TypeInt64 {$<str>$ = "vtkTypeInt64";}
     | TypeUInt64 {$<str>$ = "vtkTypeUInt64";}
     | TypeFloat32 {$<str>$ = "vtkTypeFloat32";}
     | TypeFloat64 {$<str>$ = "vtkTypeFloat64";};

special_type_id:
       IdType {$<str>$ = "vtkIdType";}
     | FloatType {$<str>$ = "vtkFloatingPointType";};

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
   VTK_ID '<' { markSig(); postSig($<str>1); postSig("<");} template_params '>'
     {chopSig(); if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">"); $<str>$ = vtkstrdup(copySig()); clearTypeId();}
 | ID '<' { markSig(); postSig($<str>1); postSig("<");} template_params '>'
     {chopSig(); if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">"); $<str>$ = vtkstrdup(copySig()); clearTypeId();}
 | QT_ID '<' { markSig(); postSig($<str>1); postSig("<");} template_params '>'
     {chopSig(); if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">"); $<str>$ = vtkstrdup(copySig()); clearTypeId();};

template_params: template_param
               | template_param ',' {chopSig(); postSig(", ");}
                 template_params;

template_param: type | integer_literal {postSig($<str>1);};

maybe_scoped_id: VTK_ID {postSig($<str>1);}
               | ID {postSig($<str>1);}
               | QT_ID {postSig($<str>1);}
               | ISTREAM {postSig($<str>1);}
               | OSTREAM {postSig($<str>1);}
               | StdString {postSig($<str>1);}
               | UnicodeString {postSig($<str>1);}
               | templated_id
               | scoped_id;

scoped_id: class_id DOUBLE_COLON maybe_scoped_id
           {
             $<str>$ = vtkstrcat3($<str>1, "::", $<str>3);
             preScopeSig($<str>1);
           }
         | templated_id DOUBLE_COLON maybe_scoped_id
           {
             $<str>$ = vtkstrcat3($<str>1, "::", $<str>3);
             preScopeSig("");
           };

class_id: ID | QT_ID | VTK_ID | ISTREAM | OSTREAM
        | StdString | UnicodeString;


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
  | CONST_PTR { postSig("*const "); $<integer>$ = VTK_PARSE_CONST_POINTER; };

type_red2:
   type_simple { $<integer>$ = $<integer>1;}
 | CLASS type_id { $<integer>$ = $<integer>2; }
 | STRUCT type_id { $<integer>$ = $<integer>2; }
 | UNION type_id { typeSig($<str>2); $<integer>$ = VTK_PARSE_UNKNOWN; }
 | ENUM type_id { typeSig($<str>2); $<integer>$ = VTK_PARSE_UNKNOWN; };

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
 | QT_ID { typeSig($<str>1); $<integer>$ = VTK_PARSE_QOBJECT; };

type_primitive:
  VOID   { typeSig("void"); $<integer>$ = VTK_PARSE_VOID;} |
  BOOL { typeSig("bool"); $<integer>$ = VTK_PARSE_BOOL;} |
  SSIZE_T { typeSig("ssize_t"); $<integer>$ = VTK_PARSE_SSIZE_T;} |
  SIZE_T { typeSig("size_t"); $<integer>$ = VTK_PARSE_SIZE_T;} |
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
  IdType { typeSig("vtkIdType"); $<integer>$ = VTK_PARSE_ID_TYPE;} |
  FloatType { typeSig("double"); $<integer>$ = VTK_PARSE_DOUBLE;} |
  FLOAT  { typeSig("float"); $<integer>$ = VTK_PARSE_FLOAT;} |
  DOUBLE { typeSig("double"); $<integer>$ = VTK_PARSE_DOUBLE;} |
  LONG_DOUBLE { typeSig("long double"); $<integer>$ = VTK_PARSE_UNKNOWN;} |
  CHAR   { typeSig("char"); $<integer>$ = VTK_PARSE_CHAR;} |
  SIGNED_CHAR { typeSig("signed char"); $<integer>$ = VTK_PARSE_SIGNED_CHAR;} |
  UNSIGNED_CHAR
    { typeSig("unsigned char"); $<integer>$ = VTK_PARSE_UNSIGNED_CHAR;} |
  INT    { typeSig("int"); $<integer>$ = VTK_PARSE_INT;} |
  UNSIGNED_INT
    { typeSig("unsigned int"); $<integer>$ = VTK_PARSE_UNSIGNED_INT;} |
  SHORT  { typeSig("short"); $<integer>$ = VTK_PARSE_SHORT;} |
  UNSIGNED_SHORT
    { typeSig("unsigned short"); $<integer>$ = VTK_PARSE_UNSIGNED_SHORT;} |
  LONG   { typeSig("long"); $<integer>$ = VTK_PARSE_LONG;} |
  UNSIGNED_LONG
    { typeSig("unsigned long"); $<integer>$ = VTK_PARSE_UNSIGNED_LONG;} |
  LONG_LONG   { typeSig("long long"); $<integer>$ = VTK_PARSE_LONG_LONG;} |
  UNSIGNED_LONG_LONG
    {typeSig("unsigned long long");$<integer>$=VTK_PARSE_UNSIGNED_LONG_LONG;} |
  INT64__ { typeSig("__int64"); $<integer>$ = VTK_PARSE___INT64;} |
  UNSIGNED_INT64__
    { typeSig("unsigned __int64"); $<integer>$ = VTK_PARSE_UNSIGNED___INT64;} |
  SIGNED { typeSig("int"); $<integer>$ = VTK_PARSE_INT; }; |
  UNSIGNED { typeSig("unsigned int"); $<integer>$ = VTK_PARSE_UNSIGNED_INT; };

/*
 * Values
 */

value: literal { $<str>$ = $<str>1; }
         | '{' { postSig("{ "); } value more_values maybe_comma '}'
        {
          postSig("}");
          $<str>$ = vtkstrcat4("{ ", $<str>3, $<str>4, " }");
        };

maybe_comma: | ',';

more_values: {$<str>$ = "";}
      | more_values ',' { postSig(", "); } value
        {
          $<str>$ = vtkstrcat3($<str>1, ", ", $<str>4);
        };

literal:  literal2 {$<str>$ = $<str>1;}
          | '+' {postSig("+");} literal2 {$<str>$ = $<str>3;}
          | '-' {postSig("-");} literal2
             {
             $<str>$ = vtkstrcat("-", $<str>3);
             }
          | string_literal {$<str>$ = $<str>1; postSig($<str>1);}
          | '(' {postSig("(");} literal ')' {postSig(")"); $<str>$ = $<str>3;}
          | any_cast '<' {postSig($<str>1); postSig("<");}
            type_red '>' '('
             {
             chopSig();
             if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
             postSig(">(");
             }
            literal ')'
             {
             postSig(")");
             if (getTypeId()[strlen(getTypeId())-1] == '>')
               {
               $<str>$ = vtkstrcat6(
                 $<str>1, "<", getTypeId(), " >(", $<str>8, ")");
               }
             else
               {
               $<str>$ = vtkstrcat6(
                 $<str>1, "<", getTypeId(), ">(", $<str>8, ")");
               }
             };

any_cast: STATIC_CAST { $<str>$ = "static_cast"; }
        | CONST_CAST { $<str>$ = "const_cast"; }
        | DYNAMIC_CAST { $<str>$ = "dynamic_cast"; }
        | REINTERPRET_CAST { $<str>$ = "reinterpret_cast"; };

string_literal: STRING_LITERAL {$<str>$ = $<str>1;}
              | string_literal STRING_LITERAL
                { $<str>$ = vtkstrcat($<str>1, $<str>2); };

literal2:   ZERO {$<str>$ = $<str>1; postSig($<str>1);}
          | INT_LITERAL {$<str>$ = $<str>1; postSig($<str>1);}
          | OCT_LITERAL {$<str>$ = $<str>1; postSig($<str>1);}
          | HEX_LITERAL {$<str>$ = $<str>1; postSig($<str>1);}
          | FLOAT_LITERAL {$<str>$ = $<str>1; postSig($<str>1);}
          | CHAR_LITERAL {$<str>$ = $<str>1; postSig($<str>1);}
          | maybe_scoped_id
            { $<str>$ = vtkstrdup(add_const_scope($<str>1)); };

/*
 * VTK Macros
 */



macro:
  SetMacro '(' any_id ',' {preSig("void Set"); postSig("(");} type ')'
   {
   postSig("a);");
   currentFunction->Macro = "vtkSetMacro";
   currentFunction->Name = vtkstrcat("Set", $<str>3);
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, $<integer>6, getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
| GetMacro '(' {postSig("Get");} any_id ','
   {markSig();} type {swapSig();} ')'
   {
   postSig("();");
   currentFunction->Macro = "vtkGetMacro";
   currentFunction->Name = vtkstrcat("Get", $<str>4);
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, $<integer>7, getTypeId(), 0);
   output_function();
   }
| SetStringMacro '(' {preSig("void Set");} any_id ')'
   {
   postSig("(char *);");
   currentFunction->Macro = "vtkSetStringMacro";
   currentFunction->Name = vtkstrcat("Set", $<str>4);
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
| GetStringMacro '(' {preSig("char *Get");} any_id ')'
   {
   postSig("();");
   currentFunction->Macro = "vtkGetStringMacro";
   currentFunction->Name = vtkstrcat("Get", $<str>4);
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   output_function();
   }
| SetClampMacro '(' any_id ',' {startSig(); markSig();} type_red2 {closeSig();}
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
| SetObjectMacro '(' any_id ','
  {preSig("void Set"); postSig("("); } type_red2 ')'
   {
   postSig("*);");
   currentFunction->Macro = "vtkSetObjectMacro";
   currentFunction->Name = vtkstrcat("Set", $<str>3);
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
| GetObjectMacro '(' {postSig("*Get");} any_id ','
   {markSig();} type_red2 {swapSig();} ')'
   {
   postSig("();");
   currentFunction->Macro = "vtkGetObjectMacro";
   currentFunction->Name = vtkstrcat("Get", $<str>4);
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   output_function();
   }
| BooleanMacro '(' any_id ',' type_red2 ')'
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
| GetVectorMacro  '(' any_id ',' {startSig();}
     type_red2 ',' INT_LITERAL ')'
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
| ViewportCoordinateMacro '(' any_id ')'
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
| WorldCoordinateMacro '(' any_id ')'
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
| TypeMacro '(' any_id ',' any_id maybe_comma ')'
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
   | NAMESPACE | USING | EXTERN | ID | VTK_ID | QT_ID
   | CONST_CAST | DYNAMIC_CAST | STATIC_CAST | REINTERPRET_CAST;

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

void vtkParse_CopyTemplateArgs(TemplateArgs *args, const TemplateArgs *orig)
{
  unsigned long i, n;

  n = orig->NumberOfArguments;
  args->NumberOfArguments = n;
  args->Arguments = (TemplateArg **)malloc(n*sizeof(TemplateArg *));

  for (i = 0; i < n; i++)
    {
    args->Arguments[i] = (TemplateArg *)malloc(sizeof(TemplateArg));
    vtkParse_CopyTemplateArg(args->Arguments[i], orig->Arguments[i]);
    }
}

void vtkParse_InitTemplateArg(TemplateArg *arg)
{
  arg->Template = NULL;
  arg->Type = 0;
  arg->Class = NULL;
  arg->Name = NULL;
  arg->Value = NULL;
}

void vtkParse_CopyTemplateArg(TemplateArg *arg, const TemplateArg *orig)
{
  arg->Template = NULL;

  if (orig->Template)
    {
    arg->Template = (TemplateArgs *)malloc(sizeof(TemplateArgs));
    vtkParse_CopyTemplateArgs(arg->Template, orig->Template);
    }

  arg->Type = orig->Type;
  arg->Class = orig->Class;
  arg->Name = orig->Name;
  arg->Value = orig->Value;
}

/* initialize the structure */
void vtkParse_InitFunction(FunctionInfo *func)
{
  unsigned long i;

  func->ItemType = VTK_FUNCTION_INFO;
  func->Access = VTK_ACCESS_PUBLIC;
  func->Name = NULL;
  func->Comment = NULL;
  func->Class = NULL;
  func->Signature = NULL;
  func->Template = NULL;
  func->NumberOfArguments = 0;
  func->ReturnValue = NULL;
  func->Macro = NULL;
  func->SizeHint = NULL;
  func->IsStatic = 0;
  func->IsVirtual = 0;
  func->IsPureVirtual = 0;
  func->IsOperator = 0;
  func->IsVariadic = 0;
  func->IsConst = 0;
  func->IsExplicit = 0;

  /* everything below here is legacy information, *
   * maintained only for backwards compatibility  */
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

void vtkParse_CopyFunction(FunctionInfo *func, const FunctionInfo *orig)
{
  unsigned long i, n;

  func->ItemType = orig->ItemType;
  func->Access = orig->Access;
  func->Name = orig->Name;
  func->Comment = orig->Comment;
  func->Class = orig->Class;
  func->Signature = orig->Signature;
  func->Template = NULL;

  if (orig->Template)
    {
    func->Template = (TemplateArgs *)malloc(sizeof(TemplateArgs));
    vtkParse_CopyTemplateArgs(func->Template, orig->Template);
    }

  n = orig->NumberOfArguments;
  func->NumberOfArguments = n;
  if (n)
    {
    func->Arguments = (ValueInfo **)malloc(n*sizeof(ValueInfo *));
    for (i = 0; i < n; i++)
      {
      func->Arguments[i] = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_CopyValue(func->Arguments[i], orig->Arguments[i]);
      }
    }

  func->ReturnValue = NULL;
  if (orig->ReturnValue)
    {
    func->ReturnValue = (ValueInfo *)malloc(sizeof(ValueInfo));
    vtkParse_CopyValue(func->ReturnValue, orig->ReturnValue);
    }

  func->Macro = orig->Macro;
  func->SizeHint = orig->SizeHint;
  func->IsStatic = orig->IsStatic;
  func->IsVirtual = orig->IsVirtual;
  func->IsPureVirtual = orig->IsPureVirtual;
  func->IsOperator = orig->IsOperator;
  func->IsVariadic = orig->IsVariadic;
  func->IsConst = orig->IsConst;
  func->IsExplicit = orig->IsExplicit;

  /* everything below here is legacy information, *
   * maintained only for backwards compatibility  */
  func->ReturnType = orig->ReturnType;
  func->ReturnClass = orig->ReturnClass;
  func->HaveHint = orig->HaveHint;
  func->HintSize = orig->HintSize;
  func->IsLegacy = orig->IsLegacy;
  func->ArrayFailure = orig->ArrayFailure;
  func->IsPublic = orig->IsPublic;
  func->IsProtected = orig->IsProtected;

  for (i = 0; i < MAX_ARGS; i++)
    {
    func->ArgTypes[i] = orig->ArgTypes[i];
    func->ArgClasses[i] = orig->ArgClasses[i];
    func->ArgCounts[i] = orig->ArgCounts[i];
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
  val->CountHint = NULL;
  val->NumberOfDimensions = 0;
  val->Function = NULL;
  val->IsStatic = 0;
  val->IsEnum = 0;
}

void vtkParse_CopyValue(ValueInfo *val, const ValueInfo *orig)
{
  unsigned long i, n;

  val->ItemType = orig->ItemType;
  val->Access = orig->Access;
  val->Name = orig->Name;
  val->Comment = orig->Comment;
  val->Value = orig->Value;
  val->Type = orig->Type;
  val->Class = orig->Class;
  val->Count = orig->Count;
  val->CountHint = orig->CountHint;

  n = orig->NumberOfDimensions;
  val->NumberOfDimensions = n;
  if (n)
    {
    val->Dimensions = (const char **)malloc(n*sizeof(char *));
    for (i = 0; i < n; i++)
      {
      val->Dimensions[i] = orig->Dimensions[i];
      }
    }

  val->Function = NULL;
  if (orig->Function)
    {
    val->Function = (FunctionInfo *)malloc(sizeof(FunctionInfo));
    vtkParse_CopyFunction(val->Function, orig->Function);
    }

  val->IsStatic = orig->IsStatic;
  val->IsEnum = orig->IsEnum;
}

/* initialize the structure */
void vtkParse_InitEnum(EnumInfo *item)
{
  item->ItemType = VTK_ENUM_INFO;
  item->Access = VTK_ACCESS_PUBLIC;
  item->Name = NULL;
  item->Comment = NULL;
}

void vtkParse_CopyEnum(EnumInfo *item, const EnumInfo *orig)
{
  item->ItemType = orig->ItemType;
  item->Access = orig->Access;
  item->Name = orig->Name;
  item->Comment = orig->Comment;
}

/* initialize the structure */
void vtkParse_InitUsing(UsingInfo *item)
{
  item->ItemType = VTK_USING_INFO;
  item->Access = VTK_ACCESS_PUBLIC;
  item->Name = NULL;
  item->Comment = NULL;
  item->Scope = NULL;
}

void vtkParse_CopyUsing(UsingInfo *item, const UsingInfo *orig)
{
  item->ItemType = orig->ItemType;
  item->Access = orig->Access;
  item->Name = orig->Name;
  item->Comment = orig->Comment;
  item->Scope = orig->Scope;
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
  cls->NumberOfClasses = 0;
  cls->NumberOfFunctions = 0;
  cls->NumberOfConstants = 0;
  cls->NumberOfVariables = 0;
  cls->NumberOfEnums = 0;
  cls->NumberOfTypedefs = 0;
  cls->NumberOfUsings = 0;
  cls->IsAbstract = 0;
  cls->HasDelete = 0;
}

void vtkParse_CopyClass(ClassInfo *cls, const ClassInfo *orig)
{
  unsigned long i, n;

  cls->ItemType = orig->ItemType;
  cls->Access = orig->Access;
  cls->Name = orig->Name;
  cls->Comment = orig->Comment;
  cls->Template = NULL;

  if (orig->Template)
    {
    cls->Template = (TemplateArgs *)malloc(sizeof(TemplateArgs));
    vtkParse_CopyTemplateArgs(cls->Template, orig->Template);
    }

  n = orig->NumberOfSuperClasses;
  cls->NumberOfSuperClasses = n;
  if (n)
    {
    cls->SuperClasses = (const char **)malloc(n*sizeof(char *));
    for (i = 0; i < n; i++)
      {
      cls->SuperClasses[i] = orig->SuperClasses[i];
      }
    }

  n = orig->NumberOfItems;
  cls->NumberOfItems = n;
  if (n)
    {
    cls->Items = (ItemInfo *)malloc(n*sizeof(ItemInfo));
    for (i = 0; i < n; i++)
      {
      cls->Items[i].Type = orig->Items[i].Type;
      cls->Items[i].Index = orig->Items[i].Index;
      }
    }

  n = orig->NumberOfClasses;
  cls->NumberOfClasses = n;
  if (n)
    {
    cls->Classes = (ClassInfo **)malloc(n*sizeof(ClassInfo *));
    for (i = 0; i < n; i++)
      {
      cls->Classes[i] = (ClassInfo *)malloc(sizeof(ClassInfo));
      vtkParse_CopyClass(cls->Classes[i], orig->Classes[i]);
      }
    }

  n = orig->NumberOfFunctions;
  cls->NumberOfFunctions = n;
  if (n)
    {
    cls->Functions = (FunctionInfo **)malloc(n*sizeof(FunctionInfo *));
    for (i = 0; i < n; i++)
      {
      cls->Functions[i] = (FunctionInfo *)malloc(sizeof(FunctionInfo));
      vtkParse_CopyFunction(cls->Functions[i], orig->Functions[i]);
      }
    }

  n = orig->NumberOfConstants;
  cls->NumberOfConstants = n;
  if (n)
    {
    cls->Constants = (ValueInfo **)malloc(n*sizeof(ValueInfo *));
    for (i = 0; i < n; i++)
      {
      cls->Constants[i] = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_CopyValue(cls->Constants[i], orig->Constants[i]);
      }
    }

  n = orig->NumberOfVariables;
  cls->NumberOfVariables = n;
  if (n)
    {
    cls->Variables = (ValueInfo **)malloc(n*sizeof(ValueInfo *));
    for (i = 0; i < n; i++)
      {
      cls->Variables[i] = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_CopyValue(cls->Variables[i], orig->Variables[i]);
      }
    }

  n = orig->NumberOfEnums;
  cls->NumberOfEnums = n;
  if (n)
    {
    cls->Enums = (EnumInfo **)malloc(n*sizeof(EnumInfo *));
    for (i = 0; i < n; i++)
      {
      cls->Enums[i] = (EnumInfo *)malloc(sizeof(EnumInfo));
      vtkParse_CopyEnum(cls->Enums[i], orig->Enums[i]);
      }
    }

  n = orig->NumberOfTypedefs;
  cls->NumberOfTypedefs = n;
  if (n)
    {
    cls->Typedefs = (ValueInfo **)malloc(n*sizeof(ValueInfo *));
    for (i = 0; i < n; i++)
      {
      cls->Typedefs[i] = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_CopyValue(cls->Typedefs[i], orig->Typedefs[i]);
      }
    }

  n = orig->NumberOfUsings;
  cls->NumberOfUsings = n;
  if (n)
    {
    cls->Usings = (UsingInfo **)malloc(n*sizeof(UsingInfo *));
    for (i = 0; i < n; i++)
      {
      cls->Usings[i] = (UsingInfo *)malloc(sizeof(UsingInfo));
      vtkParse_CopyUsing(cls->Usings[i], orig->Usings[i]);
      }
    }

  cls->IsAbstract = orig->IsAbstract;
  cls->HasDelete = orig->HasDelete;
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
  name_info->NumberOfVariables = 0;
  name_info->NumberOfEnums = 0;
  name_info->NumberOfTypedefs = 0;
  name_info->NumberOfUsings = 0;
  name_info->NumberOfNamespaces = 0;
}

void vtkParse_CopyNamespace(NamespaceInfo *ninfo, const NamespaceInfo *orig)
{
  unsigned long i, n;

  /* namespace info */
  ninfo->ItemType = orig->ItemType;
  ninfo->Access = orig->Access;
  ninfo->Name = orig->Name;
  ninfo->Comment = orig->Comment;

  n = orig->NumberOfItems;
  ninfo->NumberOfItems = n;
  if (n)
    {
    ninfo->Items = (ItemInfo *)malloc(n*sizeof(ItemInfo));
    for (i = 0; i < n; i++)
      {
      ninfo->Items[i].Type = orig->Items[i].Type;
      ninfo->Items[i].Index = orig->Items[i].Index;
      }
    }

  n = orig->NumberOfClasses;
  ninfo->NumberOfClasses = n;
  if (n)
    {
    ninfo->Classes = (ClassInfo **)malloc(n*sizeof(ClassInfo *));
    for (i = 0; i < n; i++)
      {
      ninfo->Classes[i] = (ClassInfo *)malloc(sizeof(ClassInfo));
      vtkParse_CopyClass(ninfo->Classes[i], orig->Classes[i]);
      }
    }

  n = orig->NumberOfFunctions;
  ninfo->NumberOfFunctions = n;
  if (n)
    {
    ninfo->Functions = (FunctionInfo **)malloc(n*sizeof(FunctionInfo *));
    for (i = 0; i < n; i++)
      {
      ninfo->Functions[i] = (FunctionInfo *)malloc(sizeof(FunctionInfo));
      vtkParse_CopyFunction(ninfo->Functions[i], orig->Functions[i]);
      }
    }

  n = orig->NumberOfConstants;
  ninfo->NumberOfConstants = n;
  if (n)
    {
    ninfo->Constants = (ValueInfo **)malloc(n*sizeof(ValueInfo *));
    for (i = 0; i < n; i++)
      {
      ninfo->Constants[i] = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_CopyValue(ninfo->Constants[i], orig->Constants[i]);
      }
    }

  n = orig->NumberOfVariables;
  ninfo->NumberOfVariables = n;
  if (n)
    {
    ninfo->Variables = (ValueInfo **)malloc(n*sizeof(ValueInfo *));
    for (i = 0; i < n; i++)
      {
      ninfo->Variables[i] = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_CopyValue(ninfo->Variables[i], orig->Variables[i]);
      }
    }

  n = orig->NumberOfEnums;
  ninfo->NumberOfEnums = n;
  if (n)
    {
    ninfo->Enums = (EnumInfo **)malloc(n*sizeof(EnumInfo *));
    for (i = 0; i < n; i++)
      {
      ninfo->Enums[i] = (EnumInfo *)malloc(sizeof(EnumInfo));
      vtkParse_CopyEnum(ninfo->Enums[i], orig->Enums[i]);
      }
    }

  n = orig->NumberOfTypedefs;
  ninfo->NumberOfTypedefs = n;
  if (n)
    {
    ninfo->Typedefs = (ValueInfo **)malloc(n*sizeof(ValueInfo *));
    for (i = 0; i < n; i++)
      {
      ninfo->Typedefs[i] = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_CopyValue(ninfo->Typedefs[i], orig->Typedefs[i]);
      }
    }

  n = orig->NumberOfUsings;
  ninfo->NumberOfUsings = n;
  if (n)
    {
    ninfo->Usings = (UsingInfo **)malloc(n*sizeof(UsingInfo *));
    for (i = 0; i < n; i++)
      {
      ninfo->Usings[i] = (UsingInfo *)malloc(sizeof(UsingInfo));
      vtkParse_CopyUsing(ninfo->Usings[i], orig->Usings[i]);
      }
    }
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

void vtkParse_FreeTemplateArgs(TemplateArgs *template_info)
{
  int j, m;

  m = template_info->NumberOfArguments;
  for (j = 0; j < m; j++)
    {
    if (template_info->Arguments[j]->Template)
      {
      vtkParse_FreeTemplateArgs(template_info->Arguments[j]->Template);
      }
    free(template_info->Arguments[j]);
    }

  free(template_info);
}

void vtkParse_FreeFunction(FunctionInfo *function_info);

void vtkParse_FreeValue(ValueInfo *value_info)
{
  if (value_info->NumberOfDimensions)
    {
    free((char **)value_info->Dimensions);
    }
  if (value_info->Function)
    {
    vtkParse_FreeFunction(value_info->Function);
    }

  free(value_info);
}

void vtkParse_FreeEnum(EnumInfo *enum_info)
{
  free(enum_info);
}

void vtkParse_FreeUsing(UsingInfo *using_info)
{
  free(using_info);
}

void vtkParse_FreeFunction(FunctionInfo *function_info)
{
  int j, m;

  if (function_info->Template)
    {
    vtkParse_FreeTemplateArgs(function_info->Template);
    }

  m = function_info->NumberOfArguments;
  for (j = 0; j < m; j++) { vtkParse_FreeValue(function_info->Arguments[j]); }
  if (m > 0) { free(function_info->Arguments); }

  if (function_info->ReturnValue)
    {
    vtkParse_FreeValue(function_info->ReturnValue);
    }

  free(function_info);
}

void vtkParse_FreeClass(ClassInfo *class_info)
{
  int j, m;

  if (class_info->Template) { vtkParse_FreeTemplateArgs(class_info->Template); }

  m = class_info->NumberOfSuperClasses;
  if (m > 0) { free((char **)class_info->SuperClasses); }

  m = class_info->NumberOfClasses;
  for (j = 0; j < m; j++) { vtkParse_FreeClass(class_info->Classes[j]); }
  if (m > 0) { free(class_info->Classes); }

  m = class_info->NumberOfFunctions;
  for (j = 0; j < m; j++) { vtkParse_FreeFunction(class_info->Functions[j]); }
  if (m > 0) { free(class_info->Functions); }

  m = class_info->NumberOfConstants;
  for (j = 0; j < m; j++) { vtkParse_FreeValue(class_info->Constants[j]); }
  if (m > 0) { free(class_info->Constants); }

  m = class_info->NumberOfVariables;
  for (j = 0; j < m; j++) { vtkParse_FreeValue(class_info->Variables[j]); }
  if (m > 0) { free(class_info->Variables); }

  m = class_info->NumberOfEnums;
  for (j = 0; j < m; j++) { vtkParse_FreeEnum(class_info->Enums[j]); }
  if (m > 0) { free(class_info->Enums); }

  m = class_info->NumberOfTypedefs;
  for (j = 0; j < m; j++) { vtkParse_FreeValue(class_info->Typedefs[j]); }
  if (m > 0) { free(class_info->Typedefs); }

  m = class_info->NumberOfUsings;
  for (j = 0; j < m; j++) { vtkParse_FreeUsing(class_info->Usings[j]); }
  if (m > 0) { free(class_info->Usings); }

  if (class_info->NumberOfItems > 0) { free(class_info->Items); }

  free(class_info);
}

void vtkParse_FreeNamespace(NamespaceInfo *namespace_info)
{
  int j, m;

  m = namespace_info->NumberOfClasses;
  for (j = 0; j < m; j++) { vtkParse_FreeClass(namespace_info->Classes[j]); }
  if (m > 0) { free(namespace_info->Classes); }

  m = namespace_info->NumberOfFunctions;
  for (j=0; j<m; j++) { vtkParse_FreeFunction(namespace_info->Functions[j]); }
  if (m > 0) { free(namespace_info->Functions); }

  m = namespace_info->NumberOfConstants;
  for (j=0; j<m; j++) { vtkParse_FreeValue(namespace_info->Constants[j]); }
  if (m > 0) { free(namespace_info->Constants); }

  m = namespace_info->NumberOfVariables;
  for (j=0; j<m; j++) { vtkParse_FreeValue(namespace_info->Variables[j]); }
  if (m > 0) { free(namespace_info->Variables); }

  m = namespace_info->NumberOfEnums;
  for (j = 0; j < m; j++) { vtkParse_FreeEnum(namespace_info->Enums[j]); }
  if (m > 0) { free(namespace_info->Enums); }

  m = namespace_info->NumberOfTypedefs;
  for (j = 0; j < m; j++) { vtkParse_FreeValue(namespace_info->Typedefs[j]); }
  if (m > 0) { free(namespace_info->Typedefs); }

  m = namespace_info->NumberOfUsings;
  for (j = 0; j < m; j++) { vtkParse_FreeUsing(namespace_info->Usings[j]); }
  if (m > 0) { free(namespace_info->Usings); }

  m = namespace_info->NumberOfNamespaces;
  for (j=0; j<m; j++) {vtkParse_FreeNamespace(namespace_info->Namespaces[j]);}
  if (m > 0) { free(namespace_info->Namespaces); }

  free(namespace_info);
}

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

  if (outerClass)
    {
    vtkParse_AddClassToClass(outerClass, currentClass);
    }
  else
    {
    vtkParse_AddClassToNamespace(currentNamespace, currentClass);
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

/* for a macro constant, guess the constant type */
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
    con->Access = VTK_ACCESS_PUBLIC;
    if (con->Type == 0)
      {
      con->Type = guess_constant_type(con->Value);
      }
    vtkParse_AddConstantToNamespace(data.Contents, con);
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

/* deal with types that include function pointers or arrays */
void handle_complex_type(
  ValueInfo *val, unsigned int datatype, unsigned int extra,
  const char *funcSig)
{
  FunctionInfo *func = 0;
  int i, n;
  const char *cp;

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
          n = (int)strtol(val->Dimensions[i], NULL, 0);
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

/* This method is used for extending dynamic arrays in a progression of
 * powers of two.  If "n" reaches a power of two, then the array size is
 * doubled so that "n" can be safely incremented. */
static void *array_size_check(
  void *arraymem, size_t size, int n)
{
  /* if empty, alloc for the first time */
  if (n == 0)
    {
    return malloc(size);
    }
  /* if count is power of two, reallocate with double size */
  else if ((n & (n-1)) == 0)
    {
    return realloc(arraymem, (n << 1)*size);
    }

  /* no reallocation, just return the original array */
  return arraymem;
}

/* Utility method to add a pointer to an array */
void vtkParse_AddPointerToArray(
  void *valueArray, int *count, const void *value)
{
  void **values = *(void ***)valueArray;
  int n = *count;

  values = (void **)array_size_check(values, sizeof(void *), n);

  values[n++] = (void *)value;
  *count = n;
  *(void ***)valueArray = values;
}

/*
 * There is a lot of repetition here, but all the code is written
 * out explicitly to avoid the use of macros or typecasts.  The
 * use of macros for generic programming makes code harder to debug,
 * and the use of C typecasts for anything but void* and char* breaks
 * the C99 standard.
 */

/* Utility method to add an item to an array */
void vtkParse_AddItemToArray(
  ItemInfo **valueArray, int *count,
  parse_item_t type, int idx)
{
  size_t n = *count;
  ItemInfo *values = *valueArray;

  values = (ItemInfo *)array_size_check(values, sizeof(ItemInfo), n);

  values[n].Type = type;
  values[n].Index = idx;
  *count = n+1;
  *valueArray = values;
}

/* Add a ClassInfo to a ClassInfo */
void vtkParse_AddClassToClass(ClassInfo *info, ClassInfo *item)
{
  vtkParse_AddItemToArray(&info->Items, &info->NumberOfItems,
    item->ItemType, info->NumberOfClasses);
  info->Classes = (ClassInfo **)array_size_check(
    info->Classes, sizeof(ClassInfo *), info->NumberOfClasses);
  info->Classes[info->NumberOfClasses++] = item;
}

/* Add a FunctionInfo to a ClassInfo */
void vtkParse_AddFunctionToClass(ClassInfo *info, FunctionInfo *item)
{
  vtkParse_AddItemToArray(&info->Items, &info->NumberOfItems,
    item->ItemType, info->NumberOfFunctions);
  info->Functions = (FunctionInfo **)array_size_check(
    info->Functions, sizeof(FunctionInfo *), info->NumberOfFunctions);
  info->Functions[info->NumberOfFunctions++] = item;
}

/* Add a EnumInfo to a ClassInfo */
void vtkParse_AddEnumToClass(ClassInfo *info, EnumInfo *item)
{
  vtkParse_AddItemToArray(&info->Items, &info->NumberOfItems,
    item->ItemType, info->NumberOfEnums);
  info->Enums = (EnumInfo **)array_size_check(
    info->Enums, sizeof(EnumInfo *), info->NumberOfEnums);
  info->Enums[info->NumberOfEnums++] = item;
}

/* Add a Constant ValueInfo to a ClassInfo */
void vtkParse_AddConstantToClass(ClassInfo *info, ValueInfo *item)
{
  vtkParse_AddItemToArray(&info->Items, &info->NumberOfItems,
    item->ItemType, info->NumberOfConstants);
  info->Constants = (ValueInfo **)array_size_check(
    info->Constants, sizeof(ValueInfo *), info->NumberOfConstants);
  info->Constants[info->NumberOfConstants++] = item;
}

/* Add a Variable ValueInfo to a ClassInfo */
void vtkParse_AddVariableToClass(ClassInfo *info, ValueInfo *item)
{
  vtkParse_AddItemToArray(&info->Items, &info->NumberOfItems,
    item->ItemType, info->NumberOfVariables);
  info->Variables = (ValueInfo **)array_size_check(
    info->Variables, sizeof(ValueInfo *), info->NumberOfVariables);
  info->Variables[info->NumberOfVariables++] = item;
}

/* Add a Typedef ValueInfo to a ClassInfo */
void vtkParse_AddTypedefToClass(ClassInfo *info, ValueInfo *item)
{
  vtkParse_AddItemToArray(&info->Items, &info->NumberOfItems,
    item->ItemType, info->NumberOfTypedefs);
  info->Typedefs = (ValueInfo **)array_size_check(
    info->Typedefs, sizeof(ValueInfo *), info->NumberOfTypedefs);
  info->Typedefs[info->NumberOfTypedefs++] = item;
}

/* Add a UsingInfo to a ClassInfo */
void vtkParse_AddUsingToClass(ClassInfo *info, UsingInfo *item)
{
  vtkParse_AddItemToArray(&info->Items, &info->NumberOfItems,
    item->ItemType, info->NumberOfUsings);
  info->Usings = (UsingInfo **)array_size_check(
    info->Usings, sizeof(UsingInfo *), info->NumberOfUsings);
  info->Usings[info->NumberOfUsings++] = item;
}


/* Add a NamespaceInfo to a NamespaceInfo */
void vtkParse_AddNamespaceToNamespace(NamespaceInfo *info, NamespaceInfo *item)
{
  vtkParse_AddItemToArray(&info->Items, &info->NumberOfItems,
    item->ItemType, info->NumberOfNamespaces);
  info->Namespaces = (NamespaceInfo **)array_size_check(
    info->Namespaces, sizeof(NamespaceInfo *), info->NumberOfNamespaces);
  info->Namespaces[info->NumberOfNamespaces++] = item;
}

/* Add a ClassInfo to a NamespaceInfo */
void vtkParse_AddClassToNamespace(NamespaceInfo *info, ClassInfo *item)
{
  vtkParse_AddItemToArray(&info->Items, &info->NumberOfItems,
    item->ItemType, info->NumberOfClasses);
  info->Classes = (ClassInfo **)array_size_check(
    info->Classes, sizeof(ClassInfo *), info->NumberOfClasses);
  info->Classes[info->NumberOfClasses++] = item;
}

/* Add a FunctionInfo to a NamespaceInfo */
void vtkParse_AddFunctionToNamespace(NamespaceInfo *info, FunctionInfo *item)
{
  vtkParse_AddItemToArray(&info->Items, &info->NumberOfItems,
    item->ItemType, info->NumberOfFunctions);
  info->Functions = (FunctionInfo **)array_size_check(
    info->Functions, sizeof(FunctionInfo *), info->NumberOfFunctions);
  info->Functions[info->NumberOfFunctions++] = item;
}

/* Add a EnumInfo to a NamespaceInfo */
void vtkParse_AddEnumToNamespace(NamespaceInfo *info, EnumInfo *item)
{
  vtkParse_AddItemToArray(&info->Items, &info->NumberOfItems,
    item->ItemType, info->NumberOfEnums);
  info->Enums = (EnumInfo **)array_size_check(
    info->Enums, sizeof(EnumInfo *), info->NumberOfEnums);
  info->Enums[info->NumberOfEnums++] = item;
}

/* Add a Constant ValueInfo to a NamespaceInfo */
void vtkParse_AddConstantToNamespace(NamespaceInfo *info, ValueInfo *item)
{
  vtkParse_AddItemToArray(&info->Items, &info->NumberOfItems,
    item->ItemType, info->NumberOfConstants);
  info->Constants = (ValueInfo **)array_size_check(
    info->Constants, sizeof(ValueInfo *), info->NumberOfConstants);
  info->Constants[info->NumberOfConstants++] = item;
}

/* Add a Variable ValueInfo to a NamespaceInfo */
void vtkParse_AddVariableToNamespace(NamespaceInfo *info, ValueInfo *item)
{
  vtkParse_AddItemToArray(&info->Items, &info->NumberOfItems,
    item->ItemType, info->NumberOfVariables);
  info->Variables = (ValueInfo **)array_size_check(
    info->Variables, sizeof(ValueInfo *), info->NumberOfVariables);
  info->Variables[info->NumberOfVariables++] = item;
}

/* Add a Typedef ValueInfo to a NamespaceInfo */
void vtkParse_AddTypedefToNamespace(NamespaceInfo *info, ValueInfo *item)
{
  vtkParse_AddItemToArray(&info->Items, &info->NumberOfItems,
    item->ItemType, info->NumberOfTypedefs);
  info->Typedefs = (ValueInfo **)array_size_check(
    info->Typedefs, sizeof(ValueInfo *), info->NumberOfTypedefs);
  info->Typedefs[info->NumberOfTypedefs++] = item;
}

/* Add a UsingInfo to a NamespaceInfo */
void vtkParse_AddUsingToNamespace(NamespaceInfo *info, UsingInfo *item)
{
  vtkParse_AddItemToArray(&info->Items, &info->NumberOfItems,
    item->ItemType, info->NumberOfUsings);
  info->Usings = (UsingInfo **)array_size_check(
    info->Usings, sizeof(UsingInfo *), info->NumberOfUsings);
  info->Usings[info->NumberOfUsings++] = item;
}


/* Add a Argument ValueInfo to a FunctionInfo */
void vtkParse_AddArgumentToFunction(FunctionInfo *info, ValueInfo *item)
{
  info->Arguments = (ValueInfo **)array_size_check(
    info->Arguments, sizeof(ValueInfo *), info->NumberOfArguments);
  info->Arguments[info->NumberOfArguments++] = item;
}


/* Add a TemplateArg to a TemplateArgs */
void vtkParse_AddArgumentToTemplate(TemplateArgs *info, TemplateArg *item)
{
  info->Arguments = (TemplateArg **)array_size_check(
    info->Arguments, sizeof(TemplateArg *), info->NumberOfArguments);
  info->Arguments[info->NumberOfArguments++] = item;
}


/* Utility method to add a const char pointer to an array */
void vtkParse_AddStringToArray(
  const char ***valueArray, int *count, const char *value)
{
  *valueArray = (const char **)array_size_check(
    (char **)*valueArray, sizeof(const char *), *count);

  (*valueArray)[(*count)++] = value;
}

/* duplicate the first n bytes of a string and terminate */
const char *vtkParse_DuplicateString(const char *cp, size_t n)
{
  char *res = NULL;

  res = vtkstralloc(n);
  strncpy(res, cp, n);
  res[n] = '\0';

  return res;
}

/* Add default constructors if they do not already exist */
void vtkParse_AddDefaultConstructors(ClassInfo *cls)
{
  FunctionInfo *func;
  ValueInfo *arg;
  size_t k;
  int i, n;
  int default_constructor = 1;
  int copy_constructor = 1;
  char *tname;
  const char *ccname;

  if (cls == NULL || cls->Name == NULL)
    {
    return;
    }

  n = cls->NumberOfFunctions;
  for (i = 0; i < n; i++)
    {
    func = cls->Functions[i];
    if (func->Name && strcmp(func->Name, cls->Name) == 0)
      {
      default_constructor = 0;

      if (func->NumberOfArguments == 1)
        {
        arg = func->Arguments[0];
        if (arg->Class &&
            strcmp(arg->Class, cls->Name) == 0 &&
            (arg->Type & VTK_PARSE_POINTER_MASK) == 0)
          {
          copy_constructor = 0;
          }
        }
      }
    }

  if (default_constructor)
    {
    func = (FunctionInfo *)malloc(sizeof(FunctionInfo));
    vtkParse_InitFunction(func);
    func->Class = vtkstrdup(cls->Name);
    func->Name = vtkstrdup(cls->Name);
    func->Signature = vtkstrcat(cls->Name, "()");
    vtkParse_AddFunctionToClass(cls, func);
    }

  if (copy_constructor)
    {
    if (cls->Template)
      {
      /* specialize the name */
      n = cls->Template->NumberOfArguments;

      k = strlen(cls->Name) + 2;
      for (i = 0; i < n; i++)
        {
        k += strlen(cls->Template->Arguments[i]->Name) + 2;
        }
      tname = vtkstralloc(k);
      strcpy(tname, cls->Name);
      k = strlen(tname);
      tname[k++] = '<';
      for (i = 0; i < n; i++)
        {
        strcpy(&tname[k], cls->Template->Arguments[i]->Name);
        k += strlen(cls->Template->Arguments[i]->Name);
        if (i+1 < n)
          {
          tname[k++] = ',';
          tname[k++] = ' ';
          }
        }
      tname[k++] = '>';
      tname[k] = '\0';
      ccname = tname;
      }
    else
      {
      ccname = vtkstrdup(cls->Name);
      }

    func = (FunctionInfo *)malloc(sizeof(FunctionInfo));
    vtkParse_InitFunction(func);
    func->Class = vtkstrdup(cls->Name);
    func->Name = vtkstrdup(cls->Name);
    func->Signature = vtkstrcat4(cls->Name, "(const &", ccname, ")");
    arg = (ValueInfo *)malloc(sizeof(ValueInfo));
    vtkParse_InitValue(arg);
    arg->Type = (VTK_PARSE_OBJECT_REF | VTK_PARSE_CONST);
    arg->Class = ccname;
    vtkParse_AddArgumentToFunction(func, arg);
    vtkParse_AddFunctionToClass(cls, func);
    }
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
  int lineno;
  int ret;
  FileInfo *file_info;
  char *main_class;
  const char **include_dirs;

  vtkParse_InitFile(&data);

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
  vtkParsePreprocess_AddMacro(&preprocessor, "VTK_USE_64BIT_IDS", "1");
#endif

  data.FileName = vtkstrdup(filename);

  clearComment();

  namespaceDepth = 0;
  currentNamespace = (NamespaceInfo *)malloc(sizeof(NamespaceInfo));
  vtkParse_InitNamespace(currentNamespace);
  data.Contents = currentNamespace;

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
  vtkParse_FreeNamespace(file_info->Contents);
  file_info->Contents = NULL;
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

/** Simple utility for mapping VTK types to VTK_PARSE types */
unsigned int vtkParse_MapType(int vtktype)
{
  if (vtktype > 0 && vtktype <= VTK_UNICODE_STRING)
    {
    return vtkParseTypeMap[vtktype];
    }
  return 0;
}
