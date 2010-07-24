
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 15 "vtkParse.y"


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
void start_class(const char *classname, int is_struct);
void reject_class(const char *classname, int is_struct);
void end_class();
void output_function(void);
void reject_function(void);
void set_return(FunctionInfo *func, unsigned int type,
                const char *typeclass, int count);
void add_argument(FunctionInfo *func, unsigned int type,
                  const char *classname, int count);
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
const char **arrayDimensions;

/* clear the array counter */
void clearArray(void)
{
  numberOfDimensions = 0;
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



/* Line 189 of yacc.c  */
#line 1170 "vtkParse.tab.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     STRUCT = 258,
     CLASS = 259,
     PUBLIC = 260,
     PRIVATE = 261,
     PROTECTED = 262,
     VIRTUAL = 263,
     ID = 264,
     STRING_LITERAL = 265,
     INT_LITERAL = 266,
     HEX_LITERAL = 267,
     OCT_LITERAL = 268,
     FLOAT_LITERAL = 269,
     CHAR_LITERAL = 270,
     ZERO = 271,
     FLOAT = 272,
     DOUBLE = 273,
     LONG_DOUBLE = 274,
     INT = 275,
     UNSIGNED_INT = 276,
     SHORT = 277,
     UNSIGNED_SHORT = 278,
     LONG = 279,
     UNSIGNED_LONG = 280,
     LONG_LONG = 281,
     UNSIGNED_LONG_LONG = 282,
     INT64__ = 283,
     UNSIGNED_INT64__ = 284,
     CHAR = 285,
     SIGNED_CHAR = 286,
     UNSIGNED_CHAR = 287,
     VOID = 288,
     BOOL = 289,
     SSIZE_T = 290,
     SIZE_T = 291,
     OSTREAM = 292,
     ISTREAM = 293,
     ENUM = 294,
     UNION = 295,
     CLASS_REF = 296,
     OTHER = 297,
     CONST = 298,
     CONST_PTR = 299,
     CONST_EQUAL = 300,
     OPERATOR = 301,
     UNSIGNED = 302,
     SIGNED = 303,
     FRIEND = 304,
     INLINE = 305,
     MUTABLE = 306,
     TEMPLATE = 307,
     TYPENAME = 308,
     TYPEDEF = 309,
     NAMESPACE = 310,
     USING = 311,
     VTK_ID = 312,
     STATIC = 313,
     EXTERN = 314,
     VAR_FUNCTION = 315,
     VTK_LEGACY = 316,
     NEW = 317,
     DELETE = 318,
     EXPLICIT = 319,
     OP_LSHIFT_EQ = 320,
     OP_RSHIFT_EQ = 321,
     OP_LSHIFT = 322,
     OP_RSHIFT = 323,
     OP_ARROW_POINTER = 324,
     OP_ARROW = 325,
     OP_INCR = 326,
     OP_DECR = 327,
     OP_PLUS_EQ = 328,
     OP_MINUS_EQ = 329,
     OP_TIMES_EQ = 330,
     OP_DIVIDE_EQ = 331,
     OP_REMAINDER_EQ = 332,
     OP_AND_EQ = 333,
     OP_OR_EQ = 334,
     OP_XOR_EQ = 335,
     OP_LOGIC_AND_EQ = 336,
     OP_LOGIC_OR_EQ = 337,
     OP_LOGIC_AND = 338,
     OP_LOGIC_OR = 339,
     OP_LOGIC_EQ = 340,
     OP_LOGIC_NEQ = 341,
     OP_LOGIC_LEQ = 342,
     OP_LOGIC_GEQ = 343,
     ELLIPSIS = 344,
     DOUBLE_COLON = 345,
     LP = 346,
     LA = 347,
     StdString = 348,
     UnicodeString = 349,
     IdType = 350,
     TypeInt8 = 351,
     TypeUInt8 = 352,
     TypeInt16 = 353,
     TypeUInt16 = 354,
     TypeInt32 = 355,
     TypeUInt32 = 356,
     TypeInt64 = 357,
     TypeUInt64 = 358,
     TypeFloat32 = 359,
     TypeFloat64 = 360,
     SetMacro = 361,
     GetMacro = 362,
     SetStringMacro = 363,
     GetStringMacro = 364,
     SetClampMacro = 365,
     SetObjectMacro = 366,
     GetObjectMacro = 367,
     BooleanMacro = 368,
     SetVector2Macro = 369,
     SetVector3Macro = 370,
     SetVector4Macro = 371,
     SetVector6Macro = 372,
     GetVector2Macro = 373,
     GetVector3Macro = 374,
     GetVector4Macro = 375,
     GetVector6Macro = 376,
     SetVectorMacro = 377,
     GetVectorMacro = 378,
     ViewportCoordinateMacro = 379,
     WorldCoordinateMacro = 380,
     TypeMacro = 381,
     VTK_BYTE_SWAP_DECL = 382
   };
#endif




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 222 of yacc.c  */
#line 1117 "vtkParse.y"

  const char   *str;
  unsigned int  integer;



/* Line 222 of yacc.c  */
#line 1467 "vtkParse.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 1479 "vtkParse.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)                \
      do                                        \
        {                                        \
          YYSIZE_T yyi;                                \
          for (yyi = 0; yyi < (Count); yyi++)        \
            (To)[yyi] = (From)[yyi];                \
        }                                        \
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                                \
    do                                                                        \
      {                                                                        \
        YYSIZE_T yynewbytes;                                                \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                        \
        Stack = &yyptr->Stack_alloc;                                        \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                                \
      }                                                                        \
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   5657

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  151
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  189
/* YYNRULES -- Number of rules.  */
#define YYNRULES  542
/* YYNRULES -- Number of states.  */
#define YYNSTATES  948

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   382

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   149,     2,     2,     2,   143,   144,     2,
     129,   130,   141,   139,   136,   138,   150,   142,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   135,   128,
     133,   137,   134,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   147,     2,   148,   146,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   131,   145,   132,   140,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     5,     9,    11,    15,    19,    21,
      23,    25,    27,    31,    36,    38,    41,    45,    48,    51,
      54,    58,    61,    63,    66,    71,    76,    78,    84,    85,
      92,    97,    98,   106,   107,   118,   119,   127,   128,   139,
     144,   145,   146,   150,   154,   156,   160,   164,   166,   168,
     170,   173,   175,   177,   180,   184,   188,   191,   195,   199,
     202,   208,   210,   212,   213,   216,   218,   222,   224,   227,
     230,   233,   235,   237,   239,   240,   247,   248,   254,   255,
     257,   261,   263,   267,   269,   271,   273,   275,   277,   279,
     281,   283,   285,   287,   288,   292,   293,   298,   299,   304,
     306,   308,   310,   312,   314,   316,   318,   320,   322,   324,
     326,   332,   337,   341,   344,   348,   352,   355,   357,   363,
     367,   372,   377,   382,   387,   391,   393,   397,   398,   404,
     406,   407,   412,   415,   418,   419,   423,   425,   427,   428,
     429,   433,   438,   443,   446,   450,   455,   461,   465,   470,
     477,   485,   491,   498,   501,   505,   508,   512,   516,   518,
     521,   524,   527,   531,   533,   536,   539,   543,   547,   549,
     552,   556,   557,   558,   567,   568,   572,   573,   574,   582,
     583,   587,   588,   591,   594,   596,   598,   602,   603,   609,
     610,   611,   621,   622,   626,   627,   633,   634,   638,   639,
     643,   648,   650,   651,   657,   658,   659,   662,   664,   666,
     667,   672,   673,   674,   680,   682,   684,   687,   688,   690,
     691,   695,   700,   705,   709,   712,   713,   716,   717,   718,
     723,   724,   727,   728,   732,   735,   736,   742,   745,   746,
     752,   754,   756,   758,   760,   762,   763,   764,   769,   771,
     773,   776,   778,   781,   782,   784,   786,   787,   789,   790,
     793,   794,   800,   801,   803,   804,   806,   808,   810,   812,
     814,   816,   818,   820,   823,   826,   830,   833,   836,   840,
     842,   845,   847,   850,   852,   855,   858,   860,   862,   864,
     866,   867,   871,   872,   878,   879,   885,   887,   888,   893,
     895,   897,   899,   901,   903,   905,   907,   909,   913,   917,
     919,   921,   923,   925,   927,   929,   931,   934,   936,   938,
     941,   943,   945,   947,   950,   953,   956,   959,   962,   965,
     967,   969,   971,   973,   975,   977,   979,   981,   983,   985,
     987,   989,   991,   993,   995,   997,   999,  1001,  1003,  1005,
    1007,  1009,  1011,  1013,  1015,  1017,  1019,  1021,  1023,  1025,
    1027,  1029,  1031,  1033,  1035,  1037,  1039,  1041,  1043,  1045,
    1047,  1049,  1050,  1057,  1058,  1060,  1061,  1062,  1067,  1069,
    1070,  1074,  1075,  1079,  1081,  1082,  1087,  1088,  1089,  1099,
    1101,  1104,  1106,  1108,  1110,  1112,  1114,  1116,  1118,  1120,
    1121,  1129,  1130,  1131,  1132,  1142,  1143,  1149,  1150,  1156,
    1157,  1158,  1169,  1170,  1178,  1179,  1180,  1181,  1191,  1198,
    1199,  1207,  1208,  1216,  1217,  1225,  1226,  1234,  1235,  1243,
    1244,  1252,  1253,  1261,  1262,  1270,  1271,  1281,  1282,  1292,
    1297,  1302,  1310,  1313,  1316,  1320,  1324,  1326,  1328,  1330,
    1332,  1334,  1336,  1338,  1340,  1342,  1344,  1346,  1348,  1350,
    1352,  1354,  1356,  1358,  1360,  1362,  1364,  1366,  1368,  1370,
    1372,  1374,  1376,  1378,  1380,  1382,  1384,  1386,  1388,  1390,
    1392,  1394,  1396,  1398,  1400,  1402,  1404,  1406,  1407,  1410,
    1411,  1414,  1416,  1418,  1420,  1422,  1424,  1426,  1428,  1430,
    1432,  1434,  1436,  1438,  1440,  1442,  1444,  1446,  1448,  1450,
    1452,  1454,  1456,  1458,  1460,  1462,  1464,  1466,  1468,  1470,
    1472,  1474,  1476,  1478,  1480,  1482,  1484,  1486,  1488,  1490,
    1492,  1494,  1496,  1498,  1500,  1502,  1504,  1506,  1508,  1512,
    1516,  1520,  1524
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     152,     0,    -1,    -1,    -1,   152,   153,   154,    -1,   245,
      -1,   170,   247,   128,    -1,   183,   247,   128,    -1,   184,
      -1,   156,    -1,   155,    -1,   188,    -1,   158,   247,   128,
      -1,   190,   158,   247,   128,    -1,    41,    -1,   207,   219,
      -1,   190,   207,   219,    -1,   206,   219,    -1,   201,   219,
      -1,   202,   219,    -1,   190,   201,   219,    -1,   199,   219,
      -1,   308,    -1,   287,   128,    -1,     9,   129,   333,   130,
      -1,    57,   129,   333,   130,    -1,   128,    -1,    59,    10,
     131,   152,   132,    -1,    -1,    55,   287,   157,   131,   152,
     132,    -1,    55,   131,   333,   132,    -1,    -1,     4,   272,
     159,   166,   131,   163,   132,    -1,    -1,     4,   272,   133,
     283,   134,   160,   166,   131,   163,   132,    -1,    -1,     3,
     272,   161,   166,   131,   163,   132,    -1,    -1,     3,   272,
     133,   283,   134,   162,   166,   131,   163,   132,    -1,     3,
     131,   333,   132,    -1,    -1,    -1,   163,   164,   165,    -1,
     163,   169,   135,    -1,   245,    -1,   170,   247,   128,    -1,
     183,   247,   128,    -1,   184,    -1,   188,    -1,   186,    -1,
      49,   186,    -1,   185,    -1,    41,    -1,   207,   219,    -1,
      49,   207,   219,    -1,   190,   207,   219,    -1,   204,   219,
      -1,    49,   204,   219,    -1,   190,   204,   219,    -1,   200,
     219,    -1,   127,   129,   333,   130,   128,    -1,   308,    -1,
     128,    -1,    -1,   135,   167,    -1,   168,    -1,   168,   136,
     167,    -1,   285,    -1,     6,   285,    -1,     7,   285,    -1,
       5,   285,    -1,     5,    -1,     6,    -1,     7,    -1,    -1,
      39,   272,   171,   131,   173,   132,    -1,    -1,    39,   172,
     131,   173,   132,    -1,    -1,   174,    -1,   174,   136,   173,
      -1,   272,    -1,   272,   137,   177,    -1,   176,    -1,   272,
      -1,   286,    -1,   280,    -1,    16,    -1,    11,    -1,    13,
      -1,    12,    -1,    15,    -1,   175,    -1,    -1,   181,   178,
     177,    -1,    -1,   175,   182,   179,   177,    -1,    -1,   129,
     180,   177,   130,    -1,   138,    -1,   139,    -1,   140,    -1,
     138,    -1,   139,    -1,   141,    -1,   142,    -1,   143,    -1,
     144,    -1,   145,    -1,   146,    -1,    40,   272,   131,   333,
     132,    -1,    40,   131,   333,   132,    -1,    56,   334,   128,
      -1,   190,   186,    -1,     4,   272,   187,    -1,     3,   272,
     187,    -1,     3,   187,    -1,   128,    -1,   131,   333,   132,
     334,   128,    -1,   135,   334,   128,    -1,   189,   275,   255,
     128,    -1,   189,   158,   241,   128,    -1,   189,   170,   241,
     128,    -1,   189,   183,   241,   128,    -1,   189,    60,   128,
      -1,    54,    -1,    52,   133,   134,    -1,    -1,    52,   133,
     191,   192,   134,    -1,   194,    -1,    -1,   194,   136,   193,
     192,    -1,   292,   197,    -1,   196,   197,    -1,    -1,   195,
     190,   197,    -1,     4,    -1,    53,    -1,    -1,    -1,   272,
     198,   242,    -1,    61,   129,   201,   130,    -1,    61,   129,
     204,   130,    -1,   273,   216,    -1,   273,   203,   216,    -1,
     287,    90,   140,   231,    -1,    50,   287,    90,   140,   231,
      -1,   287,    90,   224,    -1,    50,   287,    90,   224,    -1,
     287,    90,   287,    90,   140,   231,    -1,    50,   287,    90,
     287,    90,   140,   231,    -1,   287,    90,   287,    90,   224,
      -1,    50,   287,    90,   287,    90,   224,    -1,   287,    90,
      -1,   203,   287,    90,    -1,   140,   231,    -1,    50,   140,
     231,    -1,     8,   140,   231,    -1,   224,    -1,    50,   224,
      -1,   205,   224,    -1,   273,   216,    -1,     8,   273,   216,
      -1,    64,    -1,    50,    64,    -1,    64,    50,    -1,   287,
      90,   208,    -1,   273,   203,   211,    -1,   208,    -1,   273,
     211,    -1,     8,   275,   211,    -1,    -1,    -1,    46,   273,
     129,   209,   234,   130,   210,   218,    -1,    -1,   213,   212,
     218,    -1,    -1,    -1,    46,   331,   214,   129,   215,   234,
     130,    -1,    -1,   220,   217,   218,    -1,    -1,   137,    16,
      -1,    45,    16,    -1,    43,    -1,   128,    -1,   131,   333,
     132,    -1,    -1,   272,   129,   221,   234,   130,    -1,    -1,
      -1,   272,   133,   222,   283,   134,   129,   223,   234,   130,
      -1,    -1,   226,   225,   228,    -1,    -1,   272,   129,   227,
     234,   130,    -1,    -1,   135,   230,   229,    -1,    -1,   136,
     230,   229,    -1,   272,   129,   333,   130,    -1,   232,    -1,
      -1,   272,   129,   233,   234,   130,    -1,    -1,    -1,   235,
     236,    -1,    89,    -1,   238,    -1,    -1,   238,   136,   237,
     236,    -1,    -1,    -1,   239,   275,   253,   240,   242,    -1,
      60,    -1,   272,    -1,   288,   272,    -1,    -1,   243,    -1,
      -1,   137,   244,   295,    -1,   273,   246,   248,   128,    -1,
      58,    60,   248,   128,    -1,    60,   248,   128,    -1,   255,
     242,    -1,    -1,   250,   248,    -1,    -1,    -1,   248,   136,
     249,   250,    -1,    -1,   251,   246,    -1,    -1,   288,   252,
     246,    -1,   263,   265,    -1,    -1,   257,   261,   130,   254,
     259,    -1,   264,   265,    -1,    -1,   258,   262,   130,   256,
     259,    -1,   129,    -1,    91,    -1,    92,    -1,    91,    -1,
      92,    -1,    -1,    -1,   129,   260,   234,   130,    -1,   266,
      -1,   253,    -1,   288,   253,    -1,   255,    -1,   288,   255,
      -1,    -1,   264,    -1,   272,    -1,    -1,   266,    -1,    -1,
     267,   268,    -1,    -1,   270,   147,   269,   271,   148,    -1,
      -1,   268,    -1,    -1,   177,    -1,    57,    -1,     9,    -1,
      38,    -1,    37,    -1,    93,    -1,    94,    -1,   275,    -1,
      51,   275,    -1,    59,   275,    -1,    59,    10,   275,    -1,
      50,   275,    -1,   274,   275,    -1,    50,   274,   275,    -1,
      58,    -1,    58,    50,    -1,   276,    -1,   276,   288,    -1,
     278,    -1,   277,   278,    -1,   278,   277,    -1,    43,    -1,
     291,    -1,   280,    -1,   286,    -1,    -1,    53,   279,   285,
      -1,    -1,    57,   133,   281,   283,   134,    -1,    -1,     9,
     133,   282,   283,   134,    -1,   275,    -1,    -1,   275,   136,
     284,   283,    -1,    57,    -1,     9,    -1,    38,    -1,    37,
      -1,    93,    -1,    94,    -1,   280,    -1,   286,    -1,   287,
      90,   285,    -1,   280,    90,   285,    -1,     9,    -1,    57,
      -1,    38,    -1,    37,    -1,    93,    -1,    94,    -1,   144,
      -1,   289,   144,    -1,   289,    -1,   290,    -1,   289,   290,
      -1,   141,    -1,    44,    -1,   292,    -1,     4,   293,    -1,
       3,   293,    -1,    40,     9,    -1,    40,    57,    -1,    39,
       9,    -1,    39,    57,    -1,   294,    -1,   293,    -1,    93,
      -1,    94,    -1,    37,    -1,    38,    -1,     9,    -1,    57,
      -1,    33,    -1,    34,    -1,    35,    -1,    36,    -1,    96,
      -1,    97,    -1,    98,    -1,    99,    -1,   100,    -1,   101,
      -1,   102,    -1,   103,    -1,   104,    -1,   105,    -1,    95,
      -1,    17,    -1,    18,    -1,    19,    -1,    30,    -1,    31,
      -1,    32,    -1,    20,    -1,    21,    -1,    22,    -1,    23,
      -1,    24,    -1,    25,    -1,    26,    -1,    27,    -1,    28,
      -1,    29,    -1,    48,    -1,    47,    -1,   300,    -1,    -1,
     131,   296,   295,   298,   297,   132,    -1,    -1,   136,    -1,
      -1,    -1,   298,   136,   299,   295,    -1,   307,    -1,    -1,
     139,   301,   307,    -1,    -1,   138,   302,   307,    -1,   306,
      -1,    -1,   129,   303,   300,   130,    -1,    -1,    -1,     9,
     133,   304,   276,   134,   129,   305,   307,   130,    -1,    10,
      -1,   306,    10,    -1,    16,    -1,    11,    -1,    13,    -1,
      12,    -1,    14,    -1,    15,    -1,     9,    -1,    57,    -1,
      -1,   106,   129,   272,   136,   309,   275,   130,    -1,    -1,
      -1,    -1,   107,   129,   310,   272,   136,   311,   275,   312,
     130,    -1,    -1,   108,   129,   313,   272,   130,    -1,    -1,
     109,   129,   314,   272,   130,    -1,    -1,    -1,   110,   129,
     272,   136,   315,   291,   316,   136,   334,   130,    -1,    -1,
     111,   129,   272,   136,   317,   291,   130,    -1,    -1,    -1,
      -1,   112,   129,   318,   272,   136,   319,   291,   320,   130,
      -1,   113,   129,   272,   136,   291,   130,    -1,    -1,   114,
     129,   272,   136,   321,   291,   130,    -1,    -1,   118,   129,
     272,   136,   322,   291,   130,    -1,    -1,   115,   129,   272,
     136,   323,   291,   130,    -1,    -1,   119,   129,   272,   136,
     324,   291,   130,    -1,    -1,   116,   129,   272,   136,   325,
     291,   130,    -1,    -1,   120,   129,   272,   136,   326,   291,
     130,    -1,    -1,   117,   129,   272,   136,   327,   291,   130,
      -1,    -1,   121,   129,   272,   136,   328,   291,   130,    -1,
      -1,   122,   129,   272,   136,   329,   291,   136,    11,   130,
      -1,    -1,   123,   129,   272,   136,   330,   291,   136,    11,
     130,    -1,   124,   129,   272,   130,    -1,   125,   129,   272,
     130,    -1,   126,   129,   272,   136,   272,   297,   130,    -1,
     129,   130,    -1,   147,   148,    -1,    62,   147,   148,    -1,
      63,   147,   148,    -1,   332,    -1,   137,    -1,   141,    -1,
     142,    -1,   138,    -1,   139,    -1,   149,    -1,   140,    -1,
     136,    -1,   133,    -1,   134,    -1,   144,    -1,   145,    -1,
     146,    -1,   143,    -1,    62,    -1,    63,    -1,    65,    -1,
      66,    -1,    67,    -1,    68,    -1,    69,    -1,    70,    -1,
      73,    -1,    74,    -1,    75,    -1,    76,    -1,    77,    -1,
      71,    -1,    72,    -1,    78,    -1,    79,    -1,    80,    -1,
      81,    -1,    82,    -1,    83,    -1,    84,    -1,    85,    -1,
      86,    -1,    87,    -1,    88,    -1,    -1,   333,   335,    -1,
      -1,   334,   336,    -1,   128,    -1,   336,    -1,    42,    -1,
     337,    -1,   339,    -1,   338,    -1,    54,    -1,   332,    -1,
     135,    -1,   150,    -1,    90,    -1,     4,    -1,    52,    -1,
      38,    -1,    37,    -1,    93,    -1,    94,    -1,   294,    -1,
      13,    -1,    11,    -1,    12,    -1,    14,    -1,    15,    -1,
      10,    -1,    41,    -1,    43,    -1,    44,    -1,    45,    -1,
       3,    -1,    46,    -1,    58,    -1,    50,    -1,     8,    -1,
      39,    -1,    40,    -1,    53,    -1,    16,    -1,    60,    -1,
      89,    -1,     5,    -1,     7,    -1,     6,    -1,    55,    -1,
      56,    -1,    59,    -1,     9,    -1,    57,    -1,   131,   333,
     132,    -1,   147,   333,   148,    -1,   129,   333,   130,    -1,
      91,   333,   130,    -1,    92,   333,   130,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,  1261,  1261,  1262,  1261,  1266,  1267,  1268,  1269,  1270,
    1271,  1272,  1273,  1274,  1275,  1276,  1277,  1278,  1279,  1280,
    1281,  1282,  1283,  1284,  1285,  1286,  1287,  1293,  1299,  1299,
    1301,  1307,  1307,  1309,  1309,  1311,  1311,  1313,  1313,  1315,
    1318,  1320,  1319,  1322,  1325,  1326,  1327,  1328,  1329,  1330,
    1331,  1332,  1333,  1334,  1335,  1337,  1338,  1339,  1341,  1342,
    1343,  1344,  1345,  1347,  1347,  1349,  1349,  1351,  1352,  1353,
    1354,  1361,  1362,  1363,  1373,  1373,  1375,  1375,  1378,  1378,
    1378,  1380,  1381,  1383,  1384,  1384,  1384,  1386,  1386,  1386,
    1386,  1386,  1388,  1389,  1389,  1393,  1393,  1397,  1397,  1402,
    1402,  1403,  1405,  1405,  1406,  1406,  1407,  1407,  1408,  1408,
    1414,  1415,  1417,  1419,  1421,  1422,  1423,  1425,  1426,  1427,
    1433,  1456,  1457,  1458,  1459,  1461,  1467,  1468,  1468,  1474,
    1475,  1475,  1478,  1488,  1496,  1496,  1508,  1509,  1511,  1511,
    1511,  1518,  1520,  1526,  1528,  1529,  1530,  1531,  1532,  1533,
    1534,  1535,  1536,  1538,  1539,  1541,  1542,  1543,  1548,  1549,
    1550,  1557,  1558,  1566,  1566,  1566,  1568,  1569,  1572,  1573,
    1574,  1584,  1588,  1583,  1600,  1600,  1609,  1610,  1609,  1617,
    1617,  1626,  1627,  1636,  1646,  1652,  1652,  1655,  1654,  1659,
    1660,  1659,  1667,  1667,  1674,  1674,  1676,  1676,  1678,  1678,
    1680,  1682,  1691,  1691,  1697,  1697,  1697,  1700,  1701,  1702,
    1702,  1705,  1707,  1705,  1736,  1760,  1760,  1762,  1762,  1764,
    1764,  1771,  1772,  1773,  1775,  1786,  1787,  1789,  1790,  1790,
    1793,  1793,  1794,  1794,  1798,  1799,  1799,  1810,  1811,  1811,
    1821,  1822,  1824,  1827,  1829,  1832,  1833,  1833,  1835,  1838,
    1839,  1843,  1844,  1847,  1847,  1849,  1851,  1851,  1853,  1853,
    1855,  1855,  1857,  1857,  1859,  1860,  1866,  1867,  1868,  1869,
    1870,  1871,  1878,  1879,  1880,  1881,  1883,  1884,  1886,  1890,
    1891,  1893,  1894,  1896,  1897,  1898,  1900,  1902,  1903,  1905,
    1907,  1907,  1911,  1911,  1914,  1914,  1918,  1918,  1918,  1920,
    1921,  1922,  1923,  1924,  1925,  1926,  1927,  1929,  1934,  1940,
    1940,  1940,  1940,  1940,  1940,  1956,  1957,  1958,  1963,  1964,
    1976,  1977,  1980,  1981,  1982,  1983,  1984,  1985,  1986,  1989,
    1990,  1993,  1994,  1995,  1996,  1997,  1998,  2001,  2002,  2003,
    2004,  2005,  2006,  2007,  2008,  2009,  2010,  2011,  2012,  2013,
    2014,  2015,  2016,  2017,  2018,  2019,  2020,  2021,  2023,  2024,
    2026,  2027,  2029,  2030,  2032,  2033,  2035,  2036,  2038,  2039,
    2045,  2046,  2046,  2052,  2052,  2054,  2055,  2055,  2060,  2061,
    2061,  2062,  2062,  2066,  2067,  2067,  2068,  2070,  2068,  2090,
    2091,  2094,  2095,  2096,  2097,  2098,  2099,  2100,  2102,  2112,
    2112,  2121,  2122,  2122,  2121,  2130,  2130,  2139,  2139,  2147,
    2147,  2147,  2177,  2176,  2186,  2187,  2187,  2186,  2195,  2211,
    2211,  2216,  2216,  2221,  2221,  2226,  2226,  2231,  2231,  2236,
    2236,  2241,  2241,  2246,  2246,  2251,  2251,  2267,  2267,  2280,
    2313,  2347,  2400,  2401,  2402,  2403,  2404,  2406,  2407,  2407,
    2408,  2408,  2409,  2409,  2410,  2410,  2411,  2411,  2412,  2412,
    2413,  2414,  2415,  2416,  2417,  2418,  2419,  2420,  2421,  2422,
    2423,  2424,  2425,  2426,  2427,  2428,  2429,  2430,  2431,  2432,
    2433,  2434,  2435,  2436,  2437,  2438,  2439,  2445,  2445,  2446,
    2446,  2448,  2448,  2450,  2450,  2450,  2450,  2450,  2451,  2451,
    2451,  2451,  2451,  2451,  2452,  2452,  2452,  2452,  2452,  2453,
    2453,  2453,  2453,  2453,  2454,  2454,  2454,  2454,  2454,  2454,
    2455,  2455,  2455,  2455,  2455,  2455,  2455,  2456,  2456,  2456,
    2456,  2456,  2456,  2457,  2457,  2457,  2457,  2457,  2459,  2460,
    2461,  2461,  2461
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "STRUCT", "CLASS", "PUBLIC", "PRIVATE",
  "PROTECTED", "VIRTUAL", "ID", "STRING_LITERAL", "INT_LITERAL",
  "HEX_LITERAL", "OCT_LITERAL", "FLOAT_LITERAL", "CHAR_LITERAL", "ZERO",
  "FLOAT", "DOUBLE", "LONG_DOUBLE", "INT", "UNSIGNED_INT", "SHORT",
  "UNSIGNED_SHORT", "LONG", "UNSIGNED_LONG", "LONG_LONG",
  "UNSIGNED_LONG_LONG", "INT64__", "UNSIGNED_INT64__", "CHAR",
  "SIGNED_CHAR", "UNSIGNED_CHAR", "VOID", "BOOL", "SSIZE_T", "SIZE_T",
  "OSTREAM", "ISTREAM", "ENUM", "UNION", "CLASS_REF", "OTHER", "CONST",
  "CONST_PTR", "CONST_EQUAL", "OPERATOR", "UNSIGNED", "SIGNED", "FRIEND",
  "INLINE", "MUTABLE", "TEMPLATE", "TYPENAME", "TYPEDEF", "NAMESPACE",
  "USING", "VTK_ID", "STATIC", "EXTERN", "VAR_FUNCTION", "VTK_LEGACY",
  "NEW", "DELETE", "EXPLICIT", "OP_LSHIFT_EQ", "OP_RSHIFT_EQ", "OP_LSHIFT",
  "OP_RSHIFT", "OP_ARROW_POINTER", "OP_ARROW", "OP_INCR", "OP_DECR",
  "OP_PLUS_EQ", "OP_MINUS_EQ", "OP_TIMES_EQ", "OP_DIVIDE_EQ",
  "OP_REMAINDER_EQ", "OP_AND_EQ", "OP_OR_EQ", "OP_XOR_EQ",
  "OP_LOGIC_AND_EQ", "OP_LOGIC_OR_EQ", "OP_LOGIC_AND", "OP_LOGIC_OR",
  "OP_LOGIC_EQ", "OP_LOGIC_NEQ", "OP_LOGIC_LEQ", "OP_LOGIC_GEQ",
  "ELLIPSIS", "DOUBLE_COLON", "LP", "LA", "StdString", "UnicodeString",
  "IdType", "TypeInt8", "TypeUInt8", "TypeInt16", "TypeUInt16",
  "TypeInt32", "TypeUInt32", "TypeInt64", "TypeUInt64", "TypeFloat32",
  "TypeFloat64", "SetMacro", "GetMacro", "SetStringMacro",
  "GetStringMacro", "SetClampMacro", "SetObjectMacro", "GetObjectMacro",
  "BooleanMacro", "SetVector2Macro", "SetVector3Macro", "SetVector4Macro",
  "SetVector6Macro", "GetVector2Macro", "GetVector3Macro",
  "GetVector4Macro", "GetVector6Macro", "SetVectorMacro", "GetVectorMacro",
  "ViewportCoordinateMacro", "WorldCoordinateMacro", "TypeMacro",
  "VTK_BYTE_SWAP_DECL", "';'", "'('", "')'", "'{'", "'}'", "'<'", "'>'",
  "':'", "','", "'='", "'-'", "'+'", "'~'", "'*'", "'/'", "'%'", "'&'",
  "'|'", "'^'", "'['", "']'", "'!'", "'.'", "$accept", "strt", "$@1",
  "file_item", "extern", "namespace", "$@2", "class_def", "$@3", "$@4",
  "$@5", "$@6", "class_def_body", "$@7", "class_def_item",
  "optional_scope", "scope_list", "scope_list_item", "scope_type",
  "enum_def", "$@8", "$@9", "enum_list", "enum_item", "integer_value",
  "integer_literal", "integer_expression", "$@10", "$@11", "$@12",
  "math_unary_op", "math_binary_op", "union_def", "using",
  "template_internal_class", "internal_class", "internal_class_body",
  "type_def", "typedef_start", "template", "$@13", "template_args", "$@14",
  "template_arg", "$@15", "class_or_typename", "maybe_template_id", "$@16",
  "legacy_function", "legacy_method", "function", "scoped_method", "scope",
  "method", "explicit_mod", "scoped_operator", "operator",
  "typecast_op_func", "$@17", "$@18", "op_func", "$@19", "op_sig", "$@20",
  "$@21", "func", "$@22", "func_trailer", "func_body", "func_sig", "$@23",
  "$@24", "@25", "constructor", "$@26", "constructor_sig", "$@27",
  "maybe_initializers", "more_initializers", "initializer", "destructor",
  "destructor_sig", "$@28", "args_list", "$@29", "more_args", "$@30",
  "arg", "$@31", "$@32", "maybe_indirect_id", "maybe_var_assign",
  "var_assign", "$@33", "var", "var_id_maybe_assign", "maybe_vars",
  "maybe_other_vars", "$@34", "other_var", "$@35", "$@36",
  "maybe_complex_var_id", "$@37", "complex_var_id", "$@38",
  "p_or_lp_or_la", "lp_or_la", "maybe_array_or_args", "$@39",
  "maybe_indirect_maybe_var_id", "maybe_indirect_var_id", "maybe_var_id",
  "var_id", "maybe_var_array", "var_array", "$@40", "array", "$@41",
  "more_array", "array_size", "any_id", "storage_type", "static_mod",
  "type", "type_red", "const_mod", "type_red1", "$@42", "templated_id",
  "$@43", "$@44", "types", "$@45", "maybe_scoped_id", "scoped_id",
  "class_id", "type_indirection", "pointers", "pointer_or_const_pointer",
  "type_red2", "type_simple", "type_id", "type_primitive", "value", "$@46",
  "maybe_comma", "more_values", "$@47", "literal", "$@48", "$@49", "$@50",
  "$@51", "$@52", "string_literal", "literal2", "macro", "$@53", "$@54",
  "$@55", "$@56", "$@57", "$@58", "$@59", "$@60", "$@61", "$@62", "$@63",
  "$@64", "$@65", "$@66", "$@67", "$@68", "$@69", "$@70", "$@71", "$@72",
  "$@73", "$@74", "op_token", "op_token_no_delim", "maybe_other",
  "maybe_other_no_semi", "other_stuff", "other_stuff_no_semi", "braces",
  "brackets", "parens", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,    59,    40,
      41,   123,   125,    60,    62,    58,    44,    61,    45,    43,
     126,    42,    47,    37,    38,   124,    94,    91,    93,    33,
      46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   151,   152,   153,   152,   154,   154,   154,   154,   154,
     154,   154,   154,   154,   154,   154,   154,   154,   154,   154,
     154,   154,   154,   154,   154,   154,   154,   155,   157,   156,
     156,   159,   158,   160,   158,   161,   158,   162,   158,   158,
     163,   164,   163,   163,   165,   165,   165,   165,   165,   165,
     165,   165,   165,   165,   165,   165,   165,   165,   165,   165,
     165,   165,   165,   166,   166,   167,   167,   168,   168,   168,
     168,   169,   169,   169,   171,   170,   172,   170,   173,   173,
     173,   174,   174,   175,   175,   175,   175,   176,   176,   176,
     176,   176,   177,   178,   177,   179,   177,   180,   177,   181,
     181,   181,   182,   182,   182,   182,   182,   182,   182,   182,
     183,   183,   184,   185,   186,   186,   186,   187,   187,   187,
     188,   188,   188,   188,   188,   189,   190,   191,   190,   192,
     193,   192,   194,   194,   195,   194,   196,   196,   197,   198,
     197,   199,   200,   201,   202,   202,   202,   202,   202,   202,
     202,   202,   202,   203,   203,   204,   204,   204,   204,   204,
     204,   204,   204,   205,   205,   205,   206,   206,   207,   207,
     207,   209,   210,   208,   212,   211,   214,   215,   213,   217,
     216,   218,   218,   218,   218,   219,   219,   221,   220,   222,
     223,   220,   225,   224,   227,   226,   228,   228,   229,   229,
     230,   231,   233,   232,   234,   235,   234,   236,   236,   237,
     236,   239,   240,   238,   238,   241,   241,   242,   242,   244,
     243,   245,   245,   245,   246,   247,   247,   248,   249,   248,
     251,   250,   252,   250,   253,   254,   253,   255,   256,   255,
     257,   257,   257,   258,   258,   259,   260,   259,   259,   261,
     261,   262,   262,   263,   263,   264,   265,   265,   267,   266,
     269,   268,   270,   270,   271,   271,   272,   272,   272,   272,
     272,   272,   273,   273,   273,   273,   273,   273,   273,   274,
     274,   275,   275,   276,   276,   276,   277,   278,   278,   278,
     279,   278,   281,   280,   282,   280,   283,   284,   283,   285,
     285,   285,   285,   285,   285,   285,   285,   286,   286,   287,
     287,   287,   287,   287,   287,   288,   288,   288,   289,   289,
     290,   290,   291,   291,   291,   291,   291,   291,   291,   292,
     292,   293,   293,   293,   293,   293,   293,   294,   294,   294,
     294,   294,   294,   294,   294,   294,   294,   294,   294,   294,
     294,   294,   294,   294,   294,   294,   294,   294,   294,   294,
     294,   294,   294,   294,   294,   294,   294,   294,   294,   294,
     295,   296,   295,   297,   297,   298,   299,   298,   300,   301,
     300,   302,   300,   300,   303,   300,   304,   305,   300,   306,
     306,   307,   307,   307,   307,   307,   307,   307,   307,   309,
     308,   310,   311,   312,   308,   313,   308,   314,   308,   315,
     316,   308,   317,   308,   318,   319,   320,   308,   308,   321,
     308,   322,   308,   323,   308,   324,   308,   325,   308,   326,
     308,   327,   308,   328,   308,   329,   308,   330,   308,   308,
     308,   308,   331,   331,   331,   331,   331,   332,   332,   332,
     332,   332,   332,   332,   332,   332,   332,   332,   332,   332,
     332,   332,   332,   332,   332,   332,   332,   332,   332,   332,
     332,   332,   332,   332,   332,   332,   332,   332,   332,   332,
     332,   332,   332,   332,   332,   332,   332,   333,   333,   334,
     334,   335,   335,   336,   336,   336,   336,   336,   336,   336,
     336,   336,   336,   336,   336,   336,   336,   336,   336,   336,
     336,   336,   336,   336,   336,   336,   336,   336,   336,   336,
     336,   336,   336,   336,   336,   336,   336,   336,   336,   336,
     336,   336,   336,   336,   336,   336,   336,   336,   337,   338,
     339,   339,   339
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     0,     3,     1,     3,     3,     1,     1,
       1,     1,     3,     4,     1,     2,     3,     2,     2,     2,
       3,     2,     1,     2,     4,     4,     1,     5,     0,     6,
       4,     0,     7,     0,    10,     0,     7,     0,    10,     4,
       0,     0,     3,     3,     1,     3,     3,     1,     1,     1,
       2,     1,     1,     2,     3,     3,     2,     3,     3,     2,
       5,     1,     1,     0,     2,     1,     3,     1,     2,     2,
       2,     1,     1,     1,     0,     6,     0,     5,     0,     1,
       3,     1,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     0,     3,     0,     4,     0,     4,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       5,     4,     3,     2,     3,     3,     2,     1,     5,     3,
       4,     4,     4,     4,     3,     1,     3,     0,     5,     1,
       0,     4,     2,     2,     0,     3,     1,     1,     0,     0,
       3,     4,     4,     2,     3,     4,     5,     3,     4,     6,
       7,     5,     6,     2,     3,     2,     3,     3,     1,     2,
       2,     2,     3,     1,     2,     2,     3,     3,     1,     2,
       3,     0,     0,     8,     0,     3,     0,     0,     7,     0,
       3,     0,     2,     2,     1,     1,     3,     0,     5,     0,
       0,     9,     0,     3,     0,     5,     0,     3,     0,     3,
       4,     1,     0,     5,     0,     0,     2,     1,     1,     0,
       4,     0,     0,     5,     1,     1,     2,     0,     1,     0,
       3,     4,     4,     3,     2,     0,     2,     0,     0,     4,
       0,     2,     0,     3,     2,     0,     5,     2,     0,     5,
       1,     1,     1,     1,     1,     0,     0,     4,     1,     1,
       2,     1,     2,     0,     1,     1,     0,     1,     0,     2,
       0,     5,     0,     1,     0,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     2,     3,     2,     2,     3,     1,
       2,     1,     2,     1,     2,     2,     1,     1,     1,     1,
       0,     3,     0,     5,     0,     5,     1,     0,     4,     1,
       1,     1,     1,     1,     1,     1,     1,     3,     3,     1,
       1,     1,     1,     1,     1,     1,     2,     1,     1,     2,
       1,     1,     1,     2,     2,     2,     2,     2,     2,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     0,     6,     0,     1,     0,     0,     4,     1,     0,
       3,     0,     3,     1,     0,     4,     0,     0,     9,     1,
       2,     1,     1,     1,     1,     1,     1,     1,     1,     0,
       7,     0,     0,     0,     9,     0,     5,     0,     5,     0,
       0,    10,     0,     7,     0,     0,     0,     9,     6,     0,
       7,     0,     7,     0,     7,     0,     7,     0,     7,     0,
       7,     0,     7,     0,     7,     0,     9,     0,     9,     4,
       4,     7,     2,     2,     3,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     0,     2,     0,
       2,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     3,
       3,     3,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       2,     3,     1,     0,     0,     0,     0,   335,   352,   353,
     354,   358,   359,   360,   361,   362,   363,   364,   365,   366,
     367,   355,   356,   357,   337,   338,   339,   340,   333,   334,
      76,     0,    14,   286,     0,   369,   368,     0,     0,     0,
     290,   125,     0,   489,   336,   279,     0,   227,     0,   331,
     332,   351,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    26,     4,    10,     9,   230,   230,   230,
       8,    11,     0,     0,     0,     0,     0,     0,     0,   168,
       5,     0,     0,   272,   281,     0,   283,   288,   289,     0,
     287,   322,   330,   329,    22,   335,   333,   334,   336,   331,
     332,   487,    35,   324,    31,   323,     0,     0,   335,     0,
       0,   336,     0,     0,   487,   294,   327,   269,   268,   328,
     270,   271,     0,    74,   325,   326,   487,     0,     0,   279,
       0,     0,     0,   276,     0,   273,   127,     0,   309,   312,
     311,   310,   313,   314,   487,    28,     0,   487,   292,   280,
     227,     0,   274,     0,     0,     0,   401,   405,   407,     0,
       0,   414,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   321,   320,   315,     0,
     227,     0,   232,   317,   318,     0,     0,     0,     0,     0,
       0,     0,   230,     0,     0,     0,   185,   487,    21,    18,
      19,    17,    15,   267,   269,   268,     0,   266,   243,   244,
     270,   271,     0,   169,   174,   143,   179,   227,   217,     0,
     256,   255,     0,   277,   282,   284,   285,     0,     0,    23,
       0,     0,    63,     0,    63,   335,   333,   334,   336,   331,
     332,   327,   328,   325,   326,   170,     0,     0,     0,    78,
       0,     0,   487,     0,   171,   278,     0,   126,   134,   300,
     302,   301,   299,   303,   304,   305,   291,   306,     0,     0,
     519,   502,   530,   532,   531,   523,   536,   514,   510,   511,
     509,   512,   513,   527,   505,   504,   524,   525,   515,   493,
     516,   517,   518,   520,   522,   503,   526,   497,   533,   534,
     537,   521,   535,   528,   461,   462,   463,   464,   465,   466,
     467,   468,   474,   475,   469,   470,   471,   472,   473,   476,
     477,   478,   479,   480,   481,   482,   483,   484,   485,   486,
     529,   501,   487,   487,   506,   507,   112,   487,   487,   455,
     456,   499,   454,   447,   450,   451,   453,   448,   449,   460,
     457,   458,   459,   487,   452,   500,   508,   498,   490,   494,
     496,   495,     0,     0,     0,     2,   275,   223,   228,     0,
       0,   267,   266,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    12,   226,   231,   255,     0,   316,
     319,     6,     7,   124,     0,   215,     0,     0,     0,     0,
       0,    20,    16,     0,     0,   461,   462,     0,     0,   176,
     446,   167,   144,     0,   181,   181,     0,   219,   224,   218,
     251,     0,     0,   237,   257,   262,   187,   189,   153,   308,
     300,   302,   301,   299,   303,   304,     0,   166,   147,   192,
       0,   307,     0,   491,    39,   488,   492,   296,     0,     0,
       0,     0,     0,    24,     0,     0,    79,    81,    78,   111,
       0,   205,     0,   148,     0,   136,   137,     0,   129,     0,
     138,   138,    30,     2,     0,     0,     0,     0,     0,    25,
       0,   222,     3,   230,   141,   399,     0,     0,     0,   409,
     412,     0,     0,   419,   423,   427,   431,   421,   425,   429,
     433,   435,   437,   439,   440,     0,   233,   121,   216,   122,
     123,   120,    13,   186,     0,     0,   442,   443,     0,   154,
     184,     0,     0,   175,   180,   221,     0,   238,   252,   259,
       0,   205,     0,   145,   201,     0,   196,   194,     0,   297,
      37,     0,     0,     0,    64,    65,    67,    40,    33,    40,
     295,    77,    78,     0,     0,   110,     0,   211,   146,     0,
     128,   130,   138,   133,   139,   132,     3,   541,   542,   540,
     538,   539,   293,    27,   229,     0,   402,   406,   408,     0,
       0,   415,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   373,   444,   445,   177,   183,   182,   397,
     389,   392,   394,   393,   395,   396,   391,   398,   384,   371,
     381,   379,   220,   370,   383,   378,   245,   260,     0,     0,
     202,     0,   193,   205,     0,   151,     0,    63,    70,    68,
      69,     0,    41,    63,    41,    80,   267,    88,    90,    89,
      91,    87,   266,    97,    99,   100,   101,    92,    83,    82,
      93,    84,    86,    85,    75,   172,   214,   207,   206,   208,
       0,     0,   152,   134,   135,   217,    29,     0,     0,   410,
       0,     0,   418,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   374,     0,   205,   386,     0,     0,     0,
       0,   390,   246,   239,   248,   264,   188,     0,   205,   198,
       0,     0,   149,   298,     0,    66,    71,    72,    73,    36,
       0,     0,     0,    32,     0,   102,   103,   104,   105,   106,
     107,   108,   109,    95,     0,   181,   209,   253,   150,   131,
     140,   400,   403,     0,   413,   416,   420,   424,   428,   432,
     422,   426,   430,   434,     0,     0,   441,     0,     0,     0,
     375,   397,   382,   380,   205,   265,     0,   190,     0,     0,
     197,   487,   195,    40,     0,     0,     0,   335,   333,   334,
      52,     0,     0,   336,     0,   163,   331,   332,     0,    62,
       0,    42,   230,   230,    47,    51,    49,    48,     0,     0,
       0,     0,     0,   158,    44,     0,    61,    43,    40,     0,
       0,    94,   173,   211,   241,   242,   240,   212,   253,   256,
     254,     0,   489,     0,     0,     0,   178,     0,   385,   373,
       0,   261,   205,   203,   198,     0,    41,   117,   487,   489,
     116,     0,     0,     0,     0,   272,    50,     0,     0,     0,
     164,     0,   159,     0,   165,   487,   155,     0,     0,   113,
       0,     0,    59,    56,   160,    53,   161,    41,    98,    96,
     210,   217,   249,     0,   253,   234,   404,     0,   417,   436,
     438,     0,   376,     0,   247,     0,   199,   200,    38,     0,
       0,   115,   114,   157,   162,    57,    54,   156,     0,     0,
       0,     0,    45,    46,    58,    55,    34,   213,   235,   250,
     411,   387,     0,   372,   191,   489,   119,   142,     0,   245,
       0,   377,     0,    60,   236,     0,   118,   388
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     3,    84,    85,    86,   289,    87,   254,   663,
     252,   657,   662,   740,   811,   480,   574,   575,   741,    88,
     270,   142,   485,   486,   677,   678,   679,   754,   830,   744,
     680,   753,    89,    90,   815,   816,   860,    91,    92,    93,
     278,   497,   693,   498,   499,   500,   593,   695,    94,   819,
      95,    96,   232,   820,   821,    97,    98,    99,   491,   755,
     233,   444,   234,   548,   715,   235,   445,   553,   218,   236,
     561,   562,   852,   823,   566,   469,   653,   652,   790,   729,
     563,   564,   728,   586,   587,   688,   833,   689,   690,   891,
     424,   448,   449,   556,   100,   237,   199,   173,   513,   200,
     201,   418,   837,   939,   238,   646,   838,   239,   723,   784,
     893,   451,   839,   240,   453,   454,   455,   559,   725,   560,
     786,   470,   864,   102,   103,   104,   105,   106,   157,   107,
     383,   268,   478,   656,   471,   108,   133,   202,   203,   204,
     110,   111,   112,   113,   642,   718,   714,   849,   932,   643,
     720,   719,   717,   778,   940,   644,   645,   114,   605,   394,
     698,   841,   395,   396,   609,   763,   610,   399,   701,   843,
     613,   617,   614,   618,   615,   619,   616,   620,   621,   622,
     439,   377,   250,   166,   475,   476,   379,   380,   381
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -778
static const yytype_int16 yypact[] =
{
    -778,    37,  -778,  4463,   330,   726,  5207,   188,  -778,  -778,
    -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,
    -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,   -37,   -21,
     736,   429,  -778,  -778,  4692,  -778,  -778,  4795,  5207,    10,
    -778,  -778,   538,  -778,   236,    26,  4898,  -778,   -27,   128,
     135,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,
    -778,  -778,    28,    38,    48,   105,   131,   138,   169,   173,
     175,   183,   184,   185,   190,   195,   199,   201,   209,   211,
     229,   230,   231,  -778,  -778,  -778,  -778,    18,    18,    18,
    -778,  -778,  5001,  4589,   180,   180,   180,   180,   180,  -778,
    -778,   615,  5207,  -778,    79,  5310,   280,   122,  -778,   140,
    -778,  -778,  -778,  -778,  -778,   266,   347,   436,   456,   488,
     545,  -778,   237,  -778,   241,  -778,   751,   751,   -13,    71,
      91,   -11,   317,   285,  -778,  -778,   247,  -778,  -778,   248,
    -778,  -778,   249,  -778,   247,   248,  -778,   252,  4795,   338,
    5104,   260,  5207,  -778,   303,  -778,   264,   761,  -778,  -778,
    -778,  -778,  -778,  -778,  -778,  -778,  3134,  -778,  -778,  -778,
    -778,  4348,  -778,   -61,  4692,   795,  -778,  -778,  -778,   795,
     795,  -778,   795,   795,   795,   795,   795,   795,   795,   795,
     795,   795,   795,   795,   795,   795,  -778,  -778,  -778,   272,
    -778,   626,  -778,    88,  -778,   278,   281,   282,   165,   165,
     165,   626,    18,   180,   180,   524,  -778,  -778,  -778,  -778,
    -778,  -778,  -778,   321,   322,   325,  5508,   326,  -778,  -778,
     331,   337,   719,  -778,  -778,  -778,  -778,  -778,   283,   263,
     287,   116,   350,  -778,  -778,  -778,  -778,   761,   339,  -778,
     914,  5207,   310,  5207,   310,  -778,  -778,  -778,  -778,  -778,
    -778,  -778,  -778,  -778,  -778,  -778,   761,  1062,  5207,   795,
     315,  1210,  -778,  5207,  -778,  -778,   242,  -778,  5515,   -13,
     322,   325,   -11,   331,   337,   122,  -778,  -778,  1358,   319,
    -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,
    -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,
    -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,
    -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,
    -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,
    -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,
    -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,
    -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,
    -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,
    -778,  -778,  1506,  5207,   -58,  -778,  -778,  -778,  -778,   323,
     795,  -778,  -778,   332,   795,   795,   795,   333,   335,   795,
     345,   349,   351,   353,   354,   355,   358,   359,   380,   381,
     384,   327,   328,   385,  -778,   388,  -778,  -778,   626,  -778,
    -778,  -778,  -778,  -778,   400,  -778,   795,   402,   406,   407,
     408,  -778,  -778,   116,  1654,   304,   336,   416,   389,  -778,
    -778,  -778,  -778,   459,    13,    13,   167,  -778,  -778,  -778,
    -778,   420,   626,  -778,  -778,  -778,  -778,  -778,  -778,  -778,
     110,   -30,   102,   220,   124,   137,   795,  -778,  -778,  -778,
     422,  -778,   462,  -778,  -778,  -778,  -778,   423,   431,   520,
     435,   434,   441,  -778,   439,   442,   444,   445,   795,  -778,
    1802,   463,   795,  -778,   502,  -778,  -778,   460,   467,   546,
     795,   795,  -778,  -778,  1950,  2098,  2246,  2394,  2542,  -778,
     471,  -778,   465,    79,  -778,  -778,   472,   479,   480,  -778,
    -778,   490,  5413,  -778,  -778,  -778,  -778,  -778,  -778,  -778,
    -778,  -778,  -778,  -778,  -778,   795,  -778,  -778,  -778,  -778,
    -778,  -778,  -778,  -778,   485,   486,  -778,  -778,   498,  -778,
    -778,   620,   625,  -778,  -778,  -778,   491,  -778,  -778,   496,
     497,   463,  5207,  -778,  -778,   516,   513,  -778,   309,  -778,
    -778,   761,   761,   761,  -778,   518,  -778,  -778,  -778,  -778,
    -778,  -778,   795,   461,   517,  -778,   526,   166,  -778,   399,
    -778,  -778,   795,  -778,  -778,  -778,   528,  -778,  -778,  -778,
    -778,  -778,  -778,  -778,  -778,  5207,  -778,  -778,  -778,  5413,
    5413,  -778,   532,  5413,  5413,  5413,  5413,  5413,  5413,  5413,
    5413,  5413,  5413,   522,  -778,  -778,  -778,  -778,  -778,   533,
    -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,
    -778,  -778,  -778,  -778,   658,  -778,   115,  -778,   544,   543,
    -778,   795,  -778,   463,   795,  -778,  5207,   310,  -778,  -778,
    -778,   520,    67,   310,    78,  -778,   -13,  -778,  -778,  -778,
    -778,  -778,   -11,  -778,  -778,  -778,  -778,   697,  -778,  -778,
    -778,  -778,   122,  -778,  -778,  -778,  -778,  -778,  -778,   548,
    5207,   795,  -778,  5515,  -778,   283,  -778,   551,  5207,  -778,
     552,  5413,  -778,   555,   556,   559,   567,   568,   570,   573,
     574,   569,   575,  -778,   580,   463,  -778,   499,   491,   529,
     529,  -778,  -778,  -778,  -778,   461,  -778,   583,   463,   577,
     585,   592,  -778,  -778,   584,  -778,  -778,  -778,  -778,  -778,
    3726,   588,   598,  -778,   461,  -778,  -778,  -778,  -778,  -778,
    -778,  -778,  -778,  -778,   461,    13,  -778,   602,  -778,  -778,
    -778,  -778,  -778,   594,  -778,  -778,  -778,  -778,  -778,  -778,
    -778,  -778,  -778,  -778,   721,   727,  -778,   618,  5207,   619,
    -778,  -778,  -778,  -778,   463,  -778,   603,  -778,   622,   795,
    -778,  -778,  -778,  -778,   324,   726,  4100,   110,   -30,   102,
    -778,  3852,  4224,   220,   632,   705,   124,   137,   639,  -778,
     795,  -778,    18,    18,  -778,  -778,  -778,  -778,  3852,   180,
     180,   795,   180,  -778,  -778,   633,  -778,  -778,  -778,   642,
     461,  -778,  -778,   166,  -778,  -778,  -778,  -778,    72,   287,
    -778,   645,  -778,   647,   648,   650,  -778,   628,  -778,   646,
     651,  -778,   463,  -778,   577,  2690,    83,  -778,  -778,  -778,
    -778,   194,   194,   795,   795,   317,  -778,   180,   180,   524,
    -778,   795,  -778,  3976,  -778,  -778,  -778,   656,   659,  -778,
     180,   180,  -778,  -778,  -778,  -778,  -778,    89,  -778,  -778,
    -778,   283,  -778,   662,   602,  -778,  -778,  3282,  -778,  -778,
    -778,   667,   669,   670,  -778,   675,  -778,  -778,  -778,  2838,
    3430,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  4100,   677,
     795,  2986,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,
    -778,  -778,   491,  -778,  -778,  -778,  -778,  -778,   681,   115,
     529,  -778,  3578,  -778,  -778,   680,  -778,  -778
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -778,  -333,  -778,  -778,  -778,  -778,  -778,   155,  -778,  -778,
    -778,  -778,  -555,  -778,  -778,  -238,   150,  -778,  -778,   -75,
    -778,  -778,  -442,  -778,  -778,  -778,  -613,  -778,  -778,  -778,
    -778,  -778,   -73,    76,  -778,  -649,  -578,    82,  -778,  -464,
    -778,   130,  -778,  -778,  -778,  -778,  -438,  -778,  -778,  -778,
     -29,  -778,  -778,  -680,  -778,  -778,   -85,   576,  -778,  -778,
    -121,  -778,  -778,  -778,  -778,  -218,  -778,  -424,   -53,  -778,
    -778,  -778,  -778,  -217,  -778,  -778,  -778,  -778,   -23,    36,
    -458,  -778,  -778,  -520,  -778,     4,  -778,  -778,  -778,  -778,
      87,  -645,  -778,  -778,   106,  -178,   -76,  -102,  -778,   334,
    -778,  -778,  -670,  -778,  -182,  -778,  -778,  -778,   -91,  -778,
    -778,  -778,  -778,  -696,    11,  -624,  -778,  -778,  -778,  -778,
    -778,    -4,    -1,   -31,     1,    73,   743,   748,  -778,  -142,
    -778,  -778,  -228,  -778,  -108,  -129,    29,   -95,  -778,   653,
    -328,  -258,     0,  -163,  -682,  -778,     8,  -778,  -778,   141,
    -778,  -778,  -778,  -778,  -778,  -778,  -665,   121,  -778,  -778,
    -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,
    -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,  -778,
    -778,   636,  -116,  -777,  -778,  -156,  -778,  -778,  -778
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -375
static const yytype_int16 yytable[] =
{
     122,   124,   101,   376,   123,   125,   152,   132,   214,   244,
     378,   265,   205,   206,   442,   285,   482,   209,   267,   210,
     501,   554,   724,   416,   664,   481,   143,   147,   287,   429,
     271,   468,   109,   151,   588,   592,   780,     2,   153,   155,
     484,   648,   219,   220,   221,   222,   584,   172,   288,   286,
     760,   382,   512,  -312,   782,   783,   550,   450,   551,   493,
    -312,   840,   196,   595,   213,   897,   154,   387,   384,  -311,
     511,   165,   736,   737,   738,   388,   169,  -309,   388,  -310,
     261,   391,   910,   736,   737,   738,   170,   376,   736,   737,
     738,  -312,   215,   211,   736,   737,   738,   241,   415,  -269,
     263,   434,   174,   243,   376,   285,   285,  -311,   376,   137,
     138,   441,   785,   426,   426,   426,   196,   152,   287,   287,
     135,   867,   168,   196,   285,   376,   123,   125,   262,   392,
     242,   829,   196,   731,   285,   446,   430,   287,   880,   459,
     665,   831,   840,   156,   452,   389,  -225,   287,   264,   153,
     552,   172,   866,   275,   694,   510,   490,   175,   942,   197,
     431,   432,   198,   834,   835,   140,   141,   176,   892,   879,
     596,   393,   386,   390,   391,   397,   398,   177,   400,   401,
     402,   403,   404,   405,   406,   407,   408,   409,   410,   411,
     412,   413,  -311,   919,   612,   777,   732,   417,   840,   739,
    -309,   836,   137,   138,   425,   425,   425,   417,   788,   196,
     743,   433,   247,   197,  -313,   908,   198,   889,  -313,   376,
     197,   926,   392,   198,   929,  -314,   686,  -314,   433,   197,
     248,  -268,   419,   758,   178,   417,   504,   505,   856,  -267,
     536,   506,   507,   135,   722,   456,   927,   208,   212,   457,
     941,   460,   477,  -270,   477,   687,  -313,   508,   140,   141,
     179,   443,  -258,  -314,   850,   487,  -271,   180,   249,   477,
     558,   376,   391,   887,   386,   945,   818,   472,  -309,   461,
     462,   699,   700,   911,   912,   703,   704,   705,   706,   707,
     708,   709,   710,   711,   712,   555,   427,   428,   181,   463,
     137,   138,   182,   388,   183,   494,   197,   196,   216,   198,
    -310,   217,   184,   185,   186,   724,  -309,   134,   460,   187,
     392,   135,   857,    33,   188,   858,  -310,   376,   189,   859,
     190,   832,   905,   115,   649,   464,   465,   285,   191,   115,
     192,   376,   376,   376,   376,   376,   461,   462,   460,  -266,
     287,   655,   876,   168,   228,   229,   140,   141,   193,   194,
     195,   116,   117,   226,  -310,   167,   463,   116,   117,   168,
     251,   576,   692,   765,   253,   266,   461,   462,  -267,  -266,
     269,   118,   492,   272,   477,    34,   433,   118,   169,   274,
     516,   517,   518,   276,  -267,   521,   463,  -267,   277,  -267,
     414,  -267,   464,   465,   197,   913,   421,   198,   460,   422,
     423,  -309,  -312,   917,   417,  -311,  -310,   119,   120,   734,
     447,  -313,   538,   119,   120,   742,   285,  -314,   733,   285,
     285,   285,   464,   465,  -258,   501,   461,   462,   144,   287,
     458,   682,   287,   287,   287,   479,   488,   285,   417,   654,
     503,   544,   857,   514,   683,   858,   463,   533,   534,   859,
     287,   121,   565,   658,   659,   660,   137,   138,   515,   519,
     666,   520,   667,   668,   669,  -269,   670,   671,  -269,   466,
    -269,   522,  -269,   545,   487,   523,   145,   524,   565,   525,
     526,   527,   464,   465,   528,   529,   594,   594,   224,   225,
     629,   630,   631,   632,   633,   634,   635,   636,   629,   630,
     631,   632,   633,   634,   635,   636,   530,   531,   672,   285,
     532,   535,   140,   141,   388,   571,   572,   573,   537,   279,
     539,   623,   287,   391,   540,   541,   542,   547,   781,   691,
     631,   632,   633,   634,   635,   636,   546,   158,   637,   549,
     557,   567,   568,   576,   230,   231,   637,   280,   281,   569,
     146,   137,   138,   477,  -268,   570,   577,  -268,   578,  -268,
     226,  -268,   579,   580,   581,   159,   160,   282,   487,   681,
     582,   392,   583,   682,  -266,   872,   637,  -266,   594,  -266,
     673,  -266,   589,  -204,   590,   161,   683,   603,    39,   674,
     675,   676,   682,   591,   884,   602,   697,   886,   606,   607,
     608,   391,   682,   283,   284,   683,  -270,   140,   141,  -270,
     638,  -270,   639,  -270,   223,   683,   611,   626,   638,   640,
     641,   162,   163,   624,   625,   391,   627,   640,   641,   137,
     138,   628,   391,  -263,   647,   650,   914,   730,   651,   684,
     565,   886,   224,   225,   661,   822,   685,   477,   713,   392,
     696,   226,   702,   137,   138,   812,   716,   813,   721,   164,
     137,   138,   227,  -271,   726,   855,  -271,   727,  -271,   226,
    -271,   761,   764,   392,   756,   766,   767,   565,   682,   768,
     392,   757,   376,   834,   835,   140,   141,   769,   770,   762,
     771,   683,   886,   772,   773,   774,   228,   229,   230,   231,
     776,   775,   787,   789,   791,   793,   868,   228,   229,   140,
     141,   681,   792,   827,   228,   229,   140,   141,   223,   828,
     842,   836,   844,   881,   376,   115,   877,   878,   845,   825,
     681,   378,   909,   894,   265,   136,   376,   376,   846,   848,
     681,   851,   853,   417,   378,   874,   224,   225,   376,   921,
     255,   873,   901,   116,   117,   226,   882,   883,   875,   885,
     279,   152,   888,   137,   138,   896,   227,   898,   899,   376,
     900,   904,   902,   118,   922,   730,   378,   923,   256,   257,
     861,   862,   928,   139,   123,   125,   931,   865,   280,   281,
     869,  -374,   933,   153,   391,   934,   565,   937,   258,   943,
     947,   735,   230,   231,   915,   916,   814,   869,   282,   119,
     120,   241,   817,   759,   467,   854,   681,   924,   925,   140,
     141,   906,   137,   138,   417,   745,   746,   890,   747,   748,
     749,   750,   751,   752,   259,   260,   824,   604,   944,   246,
     895,   847,   392,   245,   283,   284,   420,   903,   779,   565,
     433,   826,   440,     0,     0,   433,     0,   565,     0,     0,
       0,     0,   920,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   140,   141,
     417,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   433,   290,   291,   292,
     293,   294,   295,   296,   297,   298,   299,   300,   301,   302,
     303,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   304,   305,   306,   307,   308,   309,   310,   311,   312,
     313,    35,    36,     0,   314,     0,   315,   316,   317,   318,
     319,   320,   321,   322,   323,     0,   324,   325,     0,   326,
     327,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   473,   357,     0,   358,   474,   359,   360,   361,
     362,   363,   364,   365,   366,   367,   368,   369,   370,   371,
     372,   373,     0,   374,   375,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,    35,
      36,     0,   314,     0,   315,   316,   317,   318,   319,   320,
     321,   322,   323,     0,   324,   325,     0,   326,   327,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     473,   357,   483,   358,     0,   359,   360,   361,   362,   363,
     364,   365,   366,   367,   368,   369,   370,   371,   372,   373,
       0,   374,   375,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   304,   305,   306,
     307,   308,   309,   310,   311,   312,   313,    35,    36,     0,
     314,     0,   315,   316,   317,   318,   319,   320,   321,   322,
     323,     0,   324,   325,     0,   326,   327,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   473,   357,
       0,   358,   489,   359,   360,   361,   362,   363,   364,   365,
     366,   367,   368,   369,   370,   371,   372,   373,     0,   374,
     375,   290,   291,   292,   293,   294,   295,   296,   297,   298,
     299,   300,   301,   302,   303,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,   304,   305,   306,   307,   308,
     309,   310,   311,   312,   313,    35,    36,     0,   314,     0,
     315,   316,   317,   318,   319,   320,   321,   322,   323,     0,
     324,   325,     0,   326,   327,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   473,   357,     0,   358,
     502,   359,   360,   361,   362,   363,   364,   365,   366,   367,
     368,   369,   370,   371,   372,   373,     0,   374,   375,   290,
     291,   292,   293,   294,   295,   296,   297,   298,   299,   300,
     301,   302,   303,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,   304,   305,   306,   307,   308,   309,   310,
     311,   312,   313,    35,    36,     0,   314,     0,   315,   316,
     317,   318,   319,   320,   321,   322,   323,     0,   324,   325,
       0,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   473,   357,   509,   358,     0,   359,
     360,   361,   362,   363,   364,   365,   366,   367,   368,   369,
     370,   371,   372,   373,     0,   374,   375,   290,   291,   292,
     293,   294,   295,   296,   297,   298,   299,   300,   301,   302,
     303,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   304,   305,   306,   307,   308,   309,   310,   311,   312,
     313,    35,    36,     0,   314,     0,   315,   316,   317,   318,
     319,   320,   321,   322,   323,     0,   324,   325,     0,   326,
     327,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   473,   357,     0,   358,   543,   359,   360,   361,
     362,   363,   364,   365,   366,   367,   368,   369,   370,   371,
     372,   373,     0,   374,   375,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,    35,
      36,     0,   314,     0,   315,   316,   317,   318,   319,   320,
     321,   322,   323,     0,   324,   325,     0,   326,   327,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     473,   357,     0,   358,   585,   359,   360,   361,   362,   363,
     364,   365,   366,   367,   368,   369,   370,   371,   372,   373,
       0,   374,   375,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   304,   305,   306,
     307,   308,   309,   310,   311,   312,   313,    35,    36,     0,
     314,     0,   315,   316,   317,   318,   319,   320,   321,   322,
     323,     0,   324,   325,     0,   326,   327,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   473,   357,
     597,   358,     0,   359,   360,   361,   362,   363,   364,   365,
     366,   367,   368,   369,   370,   371,   372,   373,     0,   374,
     375,   290,   291,   292,   293,   294,   295,   296,   297,   298,
     299,   300,   301,   302,   303,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,   304,   305,   306,   307,   308,
     309,   310,   311,   312,   313,    35,    36,     0,   314,     0,
     315,   316,   317,   318,   319,   320,   321,   322,   323,     0,
     324,   325,     0,   326,   327,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   473,   357,   598,   358,
       0,   359,   360,   361,   362,   363,   364,   365,   366,   367,
     368,   369,   370,   371,   372,   373,     0,   374,   375,   290,
     291,   292,   293,   294,   295,   296,   297,   298,   299,   300,
     301,   302,   303,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,   304,   305,   306,   307,   308,   309,   310,
     311,   312,   313,    35,    36,     0,   314,     0,   315,   316,
     317,   318,   319,   320,   321,   322,   323,     0,   324,   325,
       0,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   473,   357,   599,   358,     0,   359,
     360,   361,   362,   363,   364,   365,   366,   367,   368,   369,
     370,   371,   372,   373,     0,   374,   375,   290,   291,   292,
     293,   294,   295,   296,   297,   298,   299,   300,   301,   302,
     303,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   304,   305,   306,   307,   308,   309,   310,   311,   312,
     313,    35,    36,     0,   314,     0,   315,   316,   317,   318,
     319,   320,   321,   322,   323,     0,   324,   325,     0,   326,
     327,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   473,   357,     0,   358,   600,   359,   360,   361,
     362,   363,   364,   365,   366,   367,   368,   369,   370,   371,
     372,   373,     0,   374,   375,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,    35,
      36,     0,   314,     0,   315,   316,   317,   318,   319,   320,
     321,   322,   323,     0,   324,   325,     0,   326,   327,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     473,   357,     0,   358,     0,   359,   360,   361,   362,   363,
     364,   365,   366,   367,   368,   369,   370,   371,   372,   373,
     601,   374,   375,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   304,   305,   306,
     307,   308,   309,   310,   311,   312,   313,    35,    36,     0,
     314,     0,   315,   316,   317,   318,   319,   320,   321,   322,
     323,     0,   324,   325,     0,   326,   327,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   473,   357,
     907,   358,     0,   359,   360,   361,   362,   363,   364,   365,
     366,   367,   368,   369,   370,   371,   372,   373,     0,   374,
     375,   290,   291,   292,   293,   294,   295,   296,   297,   298,
     299,   300,   301,   302,   303,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,   304,   305,   306,   307,   308,
     309,   310,   311,   312,   313,    35,    36,     0,   314,     0,
     315,   316,   317,   318,   319,   320,   321,   322,   323,     0,
     324,   325,     0,   326,   327,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   473,   357,     0,   358,
     935,   359,   360,   361,   362,   363,   364,   365,   366,   367,
     368,   369,   370,   371,   372,   373,     0,   374,   375,   290,
     291,   292,   293,   294,   295,   296,   297,   298,   299,   300,
     301,   302,   303,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,   304,   305,   306,   307,   308,   309,   310,
     311,   312,   313,    35,    36,     0,   314,     0,   315,   316,
     317,   318,   319,   320,   321,   322,   323,     0,   324,   325,
       0,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   473,   357,   938,   358,     0,   359,
     360,   361,   362,   363,   364,   365,   366,   367,   368,   369,
     370,   371,   372,   373,     0,   374,   375,   290,   291,   292,
     293,   294,   295,   296,   297,   298,   299,   300,   301,   302,
     303,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   304,   305,   306,   307,   308,   309,   310,   311,   312,
     313,    35,    36,     0,   314,     0,   315,   316,   317,   318,
     319,   320,   321,   322,   323,     0,   324,   325,     0,   326,
     327,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   356,   357,     0,   358,     0,   359,   360,   361,
     362,   363,   364,   365,   366,   367,   368,   369,   370,   371,
     372,   373,     0,   374,   375,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,    35,
      36,     0,   314,     0,   315,   316,   317,   318,   319,   320,
     321,   322,   323,     0,   324,   325,     0,   326,   327,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   357,   930,   358,     0,   359,   360,   361,   362,   363,
     364,   365,   366,   367,   368,   369,   370,   371,   372,   373,
       0,   374,   375,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   304,   305,   306,
     307,   308,   309,   310,   311,   312,   313,    35,    36,     0,
     314,     0,   315,   316,   317,   318,   319,   320,   321,   322,
     323,     0,   324,   325,     0,   326,   327,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   936,   357,
       0,   358,     0,   359,   360,   361,   362,   363,   364,   365,
     366,   367,   368,   369,   370,   371,   372,   373,     0,   374,
     375,   290,   291,   292,   293,   294,   295,   296,   297,   298,
     299,   300,   301,   302,   303,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,   304,   305,   306,   307,   308,
     309,   310,   311,   312,   313,    35,    36,     0,   314,     0,
     315,   316,   317,   318,   319,   320,   321,   322,   323,     0,
     324,   325,     0,   326,   327,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   946,   357,     0,   358,
       0,   359,   360,   361,   362,   363,   364,   365,   366,   367,
     368,   369,   370,   371,   372,   373,     0,   374,   375,   794,
     795,     0,     0,     0,   796,   797,     0,     0,     0,     0,
       0,     0,     0,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,   798,   799,    30,    31,   800,     0,    33,
       0,     0,    34,    35,    36,   801,   802,    38,    39,    40,
      41,     0,    43,   803,    45,   150,    47,   804,     0,     0,
     805,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   806,
     807,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,   808,   809,   794,   795,     0,     0,     0,
     796,   797,     0,     0,     0,     0,   810,     0,     0,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,   798,
     799,   129,   130,     0,     0,    33,     0,     0,    34,    35,
      36,     0,   802,    38,     0,    40,     0,     0,     0,   803,
     149,   150,     0,     0,     0,     0,   805,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   806,   807,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   126,
     127,     0,     0,     0,   918,   797,     0,     0,     0,     0,
       0,     0,   810,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,   798,   799,   129,   130,     0,     0,    33,
       0,     0,     0,    35,    36,     0,   802,    38,     0,    40,
       0,     0,     0,   803,   149,   150,     0,     0,     0,     0,
     805,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   806,
     807,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   126,   127,     0,     0,     0,     0,   128,
       0,     0,     0,     0,     0,     0,   810,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,   129,
     130,     0,     0,    33,     0,     0,     0,    35,    36,     0,
     148,    38,     0,    40,     0,     0,     0,   131,   149,   150,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   126,   127,     0,
       0,     0,     0,   797,     0,     0,     0,     0,     0,     0,
     863,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   798,   799,   129,   130,     0,     0,    33,     0,     0,
       0,    35,    36,     0,     0,     0,     0,    40,     0,     0,
       0,   803,   149,     0,     0,     0,     0,     0,   870,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   806,   807,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   126,   127,     0,     0,     0,     0,   128,     0,     0,
       0,     0,     0,     0,   871,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,   129,   130,     0,
       0,    33,     0,     0,     0,    35,    36,     0,     0,     0,
       0,    40,     0,     0,     0,   131,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     4,     5,     0,     0,
       0,     6,     7,     0,     0,     0,     0,     0,     0,   385,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,     0,    33,     0,     0,    34,
      35,    36,     0,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
       0,    83,     4,     5,     0,     0,     0,     6,   128,     0,
       0,     0,     0,     0,     0,     0,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,   129,   130,
       0,     0,    33,     0,     0,    34,    35,    36,     0,   148,
      38,     0,    40,     0,     0,     0,   131,   149,   150,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,   126,   127,     0,     0,     0,
       0,   128,     0,     0,     0,     0,     0,     0,     0,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,   129,   130,     0,     0,    33,     0,     0,     0,    35,
      36,     0,   148,    38,     0,    40,     0,     0,     0,   131,
     149,   150,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,   126,   127,
       0,     0,     0,     0,   128,     0,     0,     0,     0,     0,
       0,     0,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,   129,   130,     0,     0,    33,     0,
       0,     0,    35,    36,     0,     0,     0,     0,    40,     0,
       0,     0,   131,   149,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,   126,   127,     0,     0,     0,     0,   128,   171,     0,
       0,     0,     0,     0,     0,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,   129,   130,     0,
       0,    33,     0,     0,     0,    35,    36,     0,     0,     0,
       0,    40,     0,     0,     0,   131,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,     4,     5,     0,     0,     0,     0,
     128,     0,     0,     0,     0,     0,     0,     0,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,     0,     0,    33,     0,     0,     0,    35,    36,
       0,     0,     0,     0,    40,     0,     0,     0,   131,     0,
       0,   207,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,   126,   127,     0,
       0,     0,     0,   128,   273,     0,     0,     0,     0,     0,
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,   129,   130,     0,     0,    33,     0,     0,
       0,    35,    36,     0,     0,     0,     0,    40,     0,     0,
       0,   131,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
     126,   127,     0,     0,     0,     0,   128,     0,     0,     0,
       0,     0,     0,     0,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,   129,   130,     0,     0,
      33,     0,     0,     0,    35,    36,     0,     0,     0,     0,
      40,     0,     0,     0,   131,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,   126,   127,     0,     0,     0,     0,   128,
       0,     0,     0,     0,     0,     0,     0,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,   129,
     130,     0,     0,     0,     0,     0,     0,    35,    36,     0,
       0,     0,     0,    40,     0,     0,     0,   131,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,   126,   127,     0,     0,
       0,     0,   255,     0,     0,     0,     0,     0,     0,     0,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
     256,   257,   129,   130,     0,     0,     0,     0,     0,     0,
      35,    36,     0,     0,     0,     0,     0,     0,     0,     0,
     258,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   259,   260,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,   495,
       0,     0,     0,     0,   255,     0,     0,     0,     0,     0,
       0,     0,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,   256,   257,     0,     0,     0,     0,     0,     0,
       0,     0,    35,    36,     0,     0,     0,     0,   496,     0,
     435,   436,   258,   326,   327,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   259,   260,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   437,     0,     0,
       0,   359,   360,     0,   362,   363,   364,   365,   366,   367,
     368,   369,   370,   371,   372,   438,     0,   374
};

static const yytype_int16 yycheck[] =
{
       4,     5,     3,   166,     4,     5,    37,     6,    93,   104,
     166,   132,    88,    89,   232,   157,   254,    92,   134,    92,
     278,   445,   646,   201,   579,   253,    30,    31,   157,   211,
     146,   248,     3,    34,   492,   499,   718,     0,    37,    38,
     268,   561,    95,    96,    97,    98,   488,    46,   164,   157,
     695,   167,   385,    90,   719,   720,    43,   239,    45,   276,
      90,   757,    44,   501,    93,   842,    37,   128,   170,    90,
     128,    42,     5,     6,     7,   136,    50,    90,   136,    90,
       9,     9,   859,     5,     6,     7,    60,   250,     5,     6,
       7,   128,    93,    92,     5,     6,     7,   101,   200,   129,
       9,   217,   129,   102,   267,   247,   248,   128,   271,    37,
      38,   232,   725,   208,   209,   210,    44,   148,   247,   248,
     133,   801,   133,    44,   266,   288,   126,   127,    57,    57,
     101,   744,    44,   653,   276,   237,   212,   266,   818,   247,
     582,   754,   838,   133,   239,   174,   128,   276,    57,   148,
     137,   150,   801,   152,   592,   383,   272,   129,   935,   141,
     213,   214,   144,    91,    92,    93,    94,   129,   838,   818,
     503,   175,   171,   174,     9,   179,   180,   129,   182,   183,
     184,   185,   186,   187,   188,   189,   190,   191,   192,   193,
     194,   195,    90,   873,   522,   715,   654,   201,   894,   132,
      90,   129,    37,    38,   208,   209,   210,   211,   728,    44,
     132,   215,    90,   141,    90,   132,   144,   830,    90,   382,
     141,   132,    57,   144,   894,    90,    60,    90,   232,   141,
      90,   129,   144,   691,   129,   239,   352,   353,   793,   129,
     418,   357,   358,   133,   129,   129,   891,    92,    93,   133,
     932,     9,   251,   129,   253,    89,   128,   373,    93,    94,
     129,   232,   147,   128,   784,   269,   129,   129,   128,   268,
     452,   434,     9,   828,   273,   940,   740,   248,    90,    37,
      38,   609,   610,   861,   862,   613,   614,   615,   616,   617,
     618,   619,   620,   621,   622,   128,   209,   210,   129,    57,
      37,    38,   129,   136,   129,   276,   141,    44,   128,   144,
      90,   131,   129,   129,   129,   939,   128,   129,     9,   129,
      57,   133,   128,    43,   129,   131,    90,   490,   129,   135,
     129,   755,   852,     9,   562,    93,    94,   479,   129,     9,
     129,   504,   505,   506,   507,   508,    37,    38,     9,   129,
     479,   568,   810,   133,    91,    92,    93,    94,   129,   129,
     129,    37,    38,    46,   128,   129,    57,    37,    38,   133,
     133,   479,   589,   701,   133,    90,    37,    38,   131,   131,
     131,    57,   140,   131,   383,    46,   390,    57,    50,   129,
     394,   395,   396,    90,   128,   399,    57,   131,   134,   133,
     128,   135,    93,    94,   141,   863,   128,   144,     9,   128,
     128,    90,    90,   871,   418,    90,    90,    93,    94,   657,
     137,    90,   426,    93,    94,   663,   568,    90,   656,   571,
     572,   573,    93,    94,   147,   693,    37,    38,     9,   568,
      90,   583,   571,   572,   573,   135,   131,   589,   452,   140,
     131,   147,   128,   130,   583,   131,    57,   130,   130,   135,
     589,   131,   466,   571,   572,   573,    37,    38,   136,   136,
       9,   136,    11,    12,    13,   128,    15,    16,   131,   140,
     133,   136,   135,   147,   488,   136,    57,   136,   492,   136,
     136,   136,    93,    94,   136,   136,   500,   501,    37,    38,
       9,    10,    11,    12,    13,    14,    15,    16,     9,    10,
      11,    12,    13,    14,    15,    16,   136,   136,    57,   661,
     136,   136,    93,    94,   136,     5,     6,     7,   128,     9,
     128,   535,   661,     9,   128,   128,   128,   148,     9,   140,
      11,    12,    13,    14,    15,    16,   130,     9,    57,    90,
     130,   129,    90,   661,    93,    94,    57,    37,    38,   136,
     131,    37,    38,   562,   128,   134,   131,   131,   134,   133,
      46,   135,   131,   134,   132,    37,    38,    57,   582,   583,
     136,    57,   137,   725,   128,   802,    57,   131,   592,   133,
     129,   135,    90,   130,   134,    57,   725,   132,    52,   138,
     139,   140,   744,   136,   821,   134,   605,   825,   136,   130,
     130,     9,   754,    93,    94,   744,   128,    93,    94,   131,
     129,   133,   131,   135,     9,   754,   136,   129,   129,   138,
     139,    93,    94,   148,   148,     9,    16,   138,   139,    37,
      38,    16,     9,   147,   147,   129,   864,   651,   135,   132,
     654,   869,    37,    38,   136,   740,   130,   656,   136,    57,
     132,    46,   130,    37,    38,   740,   133,   740,    10,   131,
      37,    38,    57,   128,   130,   791,   131,   134,   133,    46,
     135,   130,   130,    57,   136,   130,   130,   691,   830,   130,
      57,   690,   855,    91,    92,    93,    94,   130,   130,   698,
     130,   830,   920,   130,   130,   136,    91,    92,    93,    94,
     130,   136,   129,   136,   129,   131,   801,    91,    92,    93,
      94,   725,   130,   135,    91,    92,    93,    94,     9,   131,
     136,   129,    11,   818,   897,     9,   812,   813,    11,   740,
     744,   897,   858,   838,   865,     9,   909,   910,   130,   130,
     754,   148,   130,   757,   910,    50,    37,    38,   921,   875,
       9,   129,   134,    37,    38,    46,   819,   820,   129,   822,
       9,   802,   130,    37,    38,   130,    57,   130,   130,   942,
     130,   130,   136,    57,   128,   789,   942,   128,    37,    38,
     794,   795,   130,    57,   794,   795,   129,   796,    37,    38,
     801,   132,   132,   802,     9,   130,   810,   130,    57,   128,
     130,   661,    93,    94,   867,   868,   740,   818,    57,    93,
      94,   825,   740,   693,   248,   789,   830,   880,   881,    93,
      94,   854,    37,    38,   838,   138,   139,   833,   141,   142,
     143,   144,   145,   146,    93,    94,   740,   513,   939,   106,
     839,   778,    57,   105,    93,    94,   203,   849,   717,   863,
     864,   740,   226,    -1,    -1,   869,    -1,   871,    -1,    -1,
      -1,    -1,   873,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    93,    94,
     894,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   920,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    -1,    50,    -1,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    -1,    62,    63,    -1,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   128,   129,    -1,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,    -1,   149,   150,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    -1,    50,    -1,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    -1,    62,    63,    -1,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     128,   129,   130,   131,    -1,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
      -1,   149,   150,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    -1,
      50,    -1,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    -1,    62,    63,    -1,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   128,   129,
      -1,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,    -1,   149,
     150,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    -1,    50,    -1,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    -1,
      62,    63,    -1,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   128,   129,    -1,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   147,    -1,   149,   150,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    -1,    50,    -1,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    -1,    62,    63,
      -1,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   128,   129,   130,   131,    -1,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,    -1,   149,   150,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    -1,    50,    -1,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    -1,    62,    63,    -1,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   128,   129,    -1,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,    -1,   149,   150,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    -1,    50,    -1,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    -1,    62,    63,    -1,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     128,   129,    -1,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
      -1,   149,   150,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    -1,
      50,    -1,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    -1,    62,    63,    -1,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   128,   129,
     130,   131,    -1,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,    -1,   149,
     150,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    -1,    50,    -1,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    -1,
      62,    63,    -1,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   128,   129,   130,   131,
      -1,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   147,    -1,   149,   150,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    -1,    50,    -1,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    -1,    62,    63,
      -1,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   128,   129,   130,   131,    -1,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,    -1,   149,   150,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    -1,    50,    -1,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    -1,    62,    63,    -1,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   128,   129,    -1,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,    -1,   149,   150,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    -1,    50,    -1,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    -1,    62,    63,    -1,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     128,   129,    -1,   131,    -1,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    -1,
      50,    -1,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    -1,    62,    63,    -1,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   128,   129,
     130,   131,    -1,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,    -1,   149,
     150,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    -1,    50,    -1,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    -1,
      62,    63,    -1,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   128,   129,    -1,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   147,    -1,   149,   150,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    -1,    50,    -1,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    -1,    62,    63,
      -1,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   128,   129,   130,   131,    -1,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,    -1,   149,   150,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    -1,    50,    -1,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    -1,    62,    63,    -1,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   128,   129,    -1,   131,    -1,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,    -1,   149,   150,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    -1,    50,    -1,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    -1,    62,    63,    -1,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   129,   130,   131,    -1,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
      -1,   149,   150,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    -1,
      50,    -1,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    -1,    62,    63,    -1,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   128,   129,
      -1,   131,    -1,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,    -1,   149,
     150,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    -1,    50,    -1,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    -1,
      62,    63,    -1,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   128,   129,    -1,   131,
      -1,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   147,    -1,   149,   150,     3,
       4,    -1,    -1,    -1,     8,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    -1,    43,
      -1,    -1,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    -1,    56,    57,    58,    59,    60,    61,    -1,    -1,
      64,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,     3,     4,    -1,    -1,    -1,
       8,     9,    -1,    -1,    -1,    -1,   140,    -1,    -1,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    -1,    -1,    43,    -1,    -1,    46,    47,
      48,    -1,    50,    51,    -1,    53,    -1,    -1,    -1,    57,
      58,    59,    -1,    -1,    -1,    -1,    64,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,
       4,    -1,    -1,    -1,     8,     9,    -1,    -1,    -1,    -1,
      -1,    -1,   140,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    -1,    -1,    43,
      -1,    -1,    -1,    47,    48,    -1,    50,    51,    -1,    53,
      -1,    -1,    -1,    57,    58,    59,    -1,    -1,    -1,    -1,
      64,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     3,     4,    -1,    -1,    -1,    -1,     9,
      -1,    -1,    -1,    -1,    -1,    -1,   140,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    -1,    -1,    43,    -1,    -1,    -1,    47,    48,    -1,
      50,    51,    -1,    53,    -1,    -1,    -1,    57,    58,    59,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,    -1,
      -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,
     140,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    -1,    -1,    43,    -1,    -1,
      -1,    47,    48,    -1,    -1,    -1,    -1,    53,    -1,    -1,
      -1,    57,    58,    -1,    -1,    -1,    -1,    -1,    64,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     3,     4,    -1,    -1,    -1,    -1,     9,    -1,    -1,
      -1,    -1,    -1,    -1,   140,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    -1,
      -1,    43,    -1,    -1,    -1,    47,    48,    -1,    -1,    -1,
      -1,    53,    -1,    -1,    -1,    57,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     3,     4,    -1,    -1,
      -1,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,   131,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    -1,    43,    -1,    -1,    46,
      47,    48,    -1,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
      -1,   128,     3,     4,    -1,    -1,    -1,     8,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      -1,    -1,    43,    -1,    -1,    46,    47,    48,    -1,    50,
      51,    -1,    53,    -1,    -1,    -1,    57,    58,    59,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,     3,     4,    -1,    -1,    -1,
      -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    -1,    -1,    43,    -1,    -1,    -1,    47,
      48,    -1,    50,    51,    -1,    53,    -1,    -1,    -1,    57,
      58,    59,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,     3,     4,
      -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    -1,    -1,    43,    -1,
      -1,    -1,    47,    48,    -1,    -1,    -1,    -1,    53,    -1,
      -1,    -1,    57,    58,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,     3,     4,    -1,    -1,    -1,    -1,     9,    10,    -1,
      -1,    -1,    -1,    -1,    -1,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    -1,
      -1,    43,    -1,    -1,    -1,    47,    48,    -1,    -1,    -1,
      -1,    53,    -1,    -1,    -1,    57,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,     3,     4,    -1,    -1,    -1,    -1,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    -1,    -1,    43,    -1,    -1,    -1,    47,    48,
      -1,    -1,    -1,    -1,    53,    -1,    -1,    -1,    57,    -1,
      -1,    60,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,     3,     4,    -1,
      -1,    -1,    -1,     9,    10,    -1,    -1,    -1,    -1,    -1,
      -1,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    -1,    -1,    43,    -1,    -1,
      -1,    47,    48,    -1,    -1,    -1,    -1,    53,    -1,    -1,
      -1,    57,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
       3,     4,    -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    -1,    -1,
      43,    -1,    -1,    -1,    47,    48,    -1,    -1,    -1,    -1,
      53,    -1,    -1,    -1,    57,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,     3,     4,    -1,    -1,    -1,    -1,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    -1,    -1,    -1,    -1,    -1,    -1,    47,    48,    -1,
      -1,    -1,    -1,    53,    -1,    -1,    -1,    57,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,     3,     4,    -1,    -1,
      -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    -1,    -1,    -1,    -1,    -1,    -1,
      47,    48,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      57,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,     4,
      -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    47,    48,    -1,    -1,    -1,    -1,    53,    -1,
      62,    63,    57,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   129,    -1,    -1,
      -1,   133,   134,    -1,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   147,    -1,   149
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   152,     0,   153,     3,     4,     8,     9,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    43,    46,    47,    48,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   128,   154,   155,   156,   158,   170,   183,
     184,   188,   189,   190,   199,   201,   202,   206,   207,   208,
     245,   273,   274,   275,   276,   277,   278,   280,   286,   287,
     291,   292,   293,   294,   308,     9,    37,    38,    57,    93,
      94,   131,   272,   293,   272,   293,     3,     4,     9,    39,
      40,    57,   275,   287,   129,   133,     9,    37,    38,    57,
      93,    94,   172,   272,     9,    57,   131,   272,    50,    58,
      59,   273,   274,   275,   287,   275,   133,   279,     9,    37,
      38,    57,    93,    94,   131,   287,   334,   129,   133,    50,
      60,    10,   275,   248,   129,   129,   129,   129,   129,   129,
     129,   129,   129,   129,   129,   129,   129,   129,   129,   129,
     129,   129,   129,   129,   129,   129,    44,   141,   144,   247,
     250,   251,   288,   289,   290,   247,   247,    60,   158,   170,
     183,   275,   158,   201,   207,   273,   128,   131,   219,   219,
     219,   219,   219,     9,    37,    38,    46,    57,    91,    92,
      93,    94,   203,   211,   213,   216,   220,   246,   255,   258,
     264,   272,   287,   275,   288,   278,   277,    90,    90,   128,
     333,   133,   161,   133,   159,     9,    37,    38,    57,    93,
      94,     9,    57,     9,    57,   211,    90,   333,   282,   131,
     171,   333,   131,    10,   129,   275,    90,   134,   191,     9,
      37,    38,    57,    93,    94,   280,   285,   286,   333,   157,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    50,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    62,    63,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,   128,   129,   131,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   149,   150,   294,   332,   336,   337,
     338,   339,   333,   281,   248,   131,   275,   128,   136,   201,
     273,     9,    57,   272,   310,   313,   314,   272,   272,   318,
     272,   272,   272,   272,   272,   272,   272,   272,   272,   272,
     272,   272,   272,   272,   128,   248,   246,   272,   252,   144,
     290,   128,   128,   128,   241,   272,   288,   241,   241,   255,
     247,   219,   219,   272,   333,    62,    63,   129,   147,   331,
     332,   211,   216,   287,   212,   217,   248,   137,   242,   243,
     255,   262,   288,   265,   266,   267,   129,   133,    90,   285,
       9,    37,    38,    57,    93,    94,   140,   208,   224,   226,
     272,   285,   287,   128,   132,   335,   336,   275,   283,   135,
     166,   283,   166,   130,   283,   173,   174,   272,   131,   132,
     333,   209,   140,   224,   287,     4,    53,   192,   194,   195,
     196,   292,   132,   131,   333,   333,   333,   333,   333,   130,
     283,   128,   152,   249,   130,   136,   272,   272,   272,   136,
     136,   272,   136,   136,   136,   136,   136,   136,   136,   136,
     136,   136,   136,   130,   130,   136,   246,   128,   272,   128,
     128,   128,   128,   132,   147,   147,   130,   148,   214,    90,
      43,    45,   137,   218,   218,   128,   244,   130,   255,   268,
     270,   221,   222,   231,   232,   272,   225,   129,    90,   136,
     134,     5,     6,     7,   167,   168,   285,   131,   134,   131,
     134,   132,   136,   137,   173,   132,   234,   235,   231,    90,
     134,   136,   190,   197,   272,   197,   152,   130,   130,   130,
     132,   148,   134,   132,   250,   309,   136,   130,   130,   315,
     317,   136,   291,   321,   323,   325,   327,   322,   324,   326,
     328,   329,   330,   272,   148,   148,   129,    16,    16,     9,
      10,    11,    12,    13,    14,    15,    16,    57,   129,   131,
     138,   139,   295,   300,   306,   307,   256,   147,   234,   283,
     129,   135,   228,   227,   140,   224,   284,   162,   285,   285,
     285,   136,   163,   160,   163,   173,     9,    11,    12,    13,
      15,    16,    57,   129,   138,   139,   140,   175,   176,   177,
     181,   272,   280,   286,   132,   130,    60,    89,   236,   238,
     239,   140,   224,   193,   197,   198,   132,   275,   311,   291,
     291,   319,   130,   291,   291,   291,   291,   291,   291,   291,
     291,   291,   291,   136,   297,   215,   133,   303,   296,   302,
     301,    10,   129,   259,   266,   269,   130,   134,   233,   230,
     272,   234,   231,   283,   166,   167,     5,     6,     7,   132,
     164,   169,   166,   132,   180,   138,   139,   141,   142,   143,
     144,   145,   146,   182,   178,   210,   136,   275,   231,   192,
     242,   130,   275,   316,   130,   291,   130,   130,   130,   130,
     130,   130,   130,   130,   136,   136,   130,   234,   304,   300,
     295,     9,   307,   307,   260,   177,   271,   129,   234,   136,
     229,   129,   130,   131,     3,     4,     8,     9,    37,    38,
      41,    49,    50,    57,    61,    64,    93,    94,   127,   128,
     140,   165,   170,   183,   184,   185,   186,   188,   190,   200,
     204,   205,   207,   224,   245,   273,   308,   135,   131,   177,
     179,   177,   218,   237,    91,    92,   129,   253,   257,   263,
     264,   312,   136,   320,    11,    11,   130,   276,   130,   298,
     234,   148,   223,   130,   230,   333,   163,   128,   131,   135,
     187,   272,   272,   140,   273,   275,   186,   204,   207,   273,
      64,   140,   224,   129,    50,   129,   231,   247,   247,   186,
     204,   207,   219,   219,   224,   219,   216,   163,   130,   177,
     236,   240,   253,   261,   288,   265,   130,   334,   130,   130,
     130,   134,   136,   297,   130,   234,   229,   130,   132,   333,
     334,   187,   187,   231,   216,   219,   219,   231,     8,   204,
     273,   333,   128,   128,   219,   219,   132,   242,   130,   253,
     130,   129,   299,   132,   130,   132,   128,   130,   130,   254,
     305,   295,   334,   128,   259,   307,   128,   130
};

#define yyerrok                (yyerrstatus = 0)
#define yyclearin        (yychar = YYEMPTY)
#define YYEMPTY                (-2)
#define YYEOF                0

#define YYACCEPT        goto yyacceptlab
#define YYABORT                goto yyabortlab
#define YYERROR                goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL                goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                        \
do                                                                \
  if (yychar == YYEMPTY && yylen == 1)                                \
    {                                                                \
      yychar = (Token);                                                \
      yylval = (Value);                                                \
      yytoken = YYTRANSLATE (yychar);                                \
      YYPOPSTACK (1);                                                \
      goto yybackup;                                                \
    }                                                                \
  else                                                                \
    {                                                                \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                        \
    }                                                                \
while (YYID (0))


#define YYTERROR        1
#define YYERRCODE        256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                        \
      if (YYID (N))                                                    \
        {                                                                \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;        \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;                \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;        \
        }                                                                \
      else                                                                \
        {                                                                \
          (Current).first_line   = (Current).last_line   =                \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =                \
            YYRHSLOC (Rhs, 0).last_column;                                \
        }                                                                \
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)                        \
     fprintf (File, "%d.%d-%d.%d",                        \
              (Loc).first_line, (Loc).first_column,        \
              (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                                \
  if (yydebug)                                        \
    YYFPRINTF Args;                                \
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                          \
do {                                                                          \
  if (yydebug)                                                                  \
    {                                                                          \
      YYFPRINTF (stderr, "%s ", Title);                                          \
      yy_symbol_print (stderr,                                                  \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                                  \
    }                                                                          \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
        break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                                \
do {                                                                \
  if (yydebug)                                                        \
    yy_stack_print ((Bottom), (Top));                                \
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)                \
do {                                        \
  if (yydebug)                                \
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef        YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
         constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
                    + sizeof yyexpecting - 1
                    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
                       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
         YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
        if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
          {
            if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
              {
                yycount = 1;
                yysize = yysize0;
                yyformat[sizeof yyunexpected - 1] = '\0';
                break;
              }
            yyarg[yycount++] = yytname[yyx];
            yysize1 = yysize + yytnamerr (0, yytname[yyx]);
            yysize_overflow |= (yysize1 < yysize);
            yysize = yysize1;
            yyfmt = yystpcpy (yyfmt, yyprefix);
            yyprefix = yyor;
          }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
        return YYSIZE_MAXIMUM;

      if (yyresult)
        {
          /* Avoid sprintf, as that infringes on the user's name space.
             Don't have undefined behavior even if the translation
             produced a string with the wrong number of "%s"s.  */
          char *yyp = yyresult;
          int yyi = 0;
          while ((*yyp = *yyf) != '\0')
            {
              if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
                {
                  yyp += yytnamerr (yyp, yyarg[yyi++]);
                  yyf += 2;
                }
              else
                {
                  yyp++;
                  yyf++;
                }
            }
        }
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
        break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yystacksize);

        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 3:

/* Line 1455 of yacc.c  */
#line 1262 "vtkParse.y"
    { startSig(); clearTypeId(); clearTemplate(); closeComment(); }
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 1276 "vtkParse.y"
    { output_function(); }
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 1277 "vtkParse.y"
    { output_function(); }
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 1278 "vtkParse.y"
    { reject_function(); }
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 1279 "vtkParse.y"
    { output_function(); }
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 1280 "vtkParse.y"
    { reject_function(); }
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 1281 "vtkParse.y"
    { output_function(); }
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 1282 "vtkParse.y"
    { legacySig(); output_function(); }
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 1299 "vtkParse.y"
    { pushNamespace((yyvsp[(2) - (2)].str)); }
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 1300 "vtkParse.y"
    { popNamespace(); }
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 1307 "vtkParse.y"
    { start_class((yyvsp[(2) - (2)].str), 0); }
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 1308 "vtkParse.y"
    { end_class(); }
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 1309 "vtkParse.y"
    { reject_class((yyvsp[(2) - (5)].str), 0); }
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 1310 "vtkParse.y"
    { end_class(); }
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 1311 "vtkParse.y"
    { start_class((yyvsp[(2) - (2)].str), 1); }
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 1312 "vtkParse.y"
    { end_class(); }
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 1313 "vtkParse.y"
    { reject_class((yyvsp[(2) - (5)].str), 1); }
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 1314 "vtkParse.y"
    { end_class(); }
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 1320 "vtkParse.y"
    { startSig(); clearTypeId(); clearTemplate();  closeComment(); }
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 1334 "vtkParse.y"
    { output_function(); }
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 1335 "vtkParse.y"
    { ClassInfo *tmpc = currentClass;
     currentClass = NULL; reject_function(); currentClass = tmpc; }
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 1337 "vtkParse.y"
    { output_function(); }
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 1338 "vtkParse.y"
    { output_function(); }
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 1339 "vtkParse.y"
    { ClassInfo *tmpc = currentClass;
     currentClass = NULL; reject_function(); currentClass = tmpc; }
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 1341 "vtkParse.y"
    { output_function(); }
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 1342 "vtkParse.y"
    { legacySig(); output_function(); }
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 1355 "vtkParse.y"
    {
      vtkParse_AddStringToArray(&currentClass->SuperClasses,
                                &currentClass->NumberOfSuperClasses,
                                vtkstrdup((yyvsp[(2) - (2)].str)));
    }
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 1361 "vtkParse.y"
    {access_level = VTK_ACCESS_PUBLIC;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 1362 "vtkParse.y"
    {access_level = VTK_ACCESS_PROTECTED;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 1363 "vtkParse.y"
    {access_level = VTK_ACCESS_PRIVATE;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 1373 "vtkParse.y"
    {start_enum((yyvsp[(2) - (2)].str));}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 1374 "vtkParse.y"
    {end_enum();}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 1375 "vtkParse.y"
    {start_enum(NULL);}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 1376 "vtkParse.y"
    {end_enum();}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 1380 "vtkParse.y"
    {add_enum((yyvsp[(1) - (1)].str), NULL);}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 1381 "vtkParse.y"
    {add_enum((yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 1383 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 1388 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 1389 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 1390 "vtkParse.y"
    {
         (yyval.str) = vtkstrcat((yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));
       }
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 1393 "vtkParse.y"
    {postSig((yyvsp[(2) - (2)].str));}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 1394 "vtkParse.y"
    {
         (yyval.str) = vtkstrcat5((yyvsp[(1) - (4)].str), " ", (yyvsp[(2) - (4)].str), " ", (yyvsp[(4) - (4)].str));
       }
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 1397 "vtkParse.y"
    {postSig("(");}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 1398 "vtkParse.y"
    {
         (yyval.str) = vtkstrcat3("(", (yyvsp[(3) - (4)].str), ")");
       }
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 1402 "vtkParse.y"
    { (yyval.str) = "-"; }
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 1402 "vtkParse.y"
    { (yyval.str) = "+"; }
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 1403 "vtkParse.y"
    { (yyval.str) = "~"; }
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 1405 "vtkParse.y"
    { (yyval.str) = "-"; }
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 1405 "vtkParse.y"
    { (yyval.str) = "+"; }
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 1406 "vtkParse.y"
    { (yyval.str) = "*"; }
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 1406 "vtkParse.y"
    { (yyval.str) = "/"; }
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 1407 "vtkParse.y"
    { (yyval.str) = "%"; }
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 1407 "vtkParse.y"
    { (yyval.str) = "&"; }
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 1408 "vtkParse.y"
    { (yyval.str) = "|"; }
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 1408 "vtkParse.y"
    { (yyval.str) = "^"; }
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 1434 "vtkParse.y"
    {
      ValueInfo *item = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(item);
      item->ItemType = VTK_TYPEDEF_INFO;
      item->Access = access_level;

      handle_complex_type(item, (yyvsp[(2) - (4)].integer), (yyvsp[(3) - (4)].integer), getSig());

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
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 1456 "vtkParse.y"
    { }
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 1457 "vtkParse.y"
    { }
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 1458 "vtkParse.y"
    { }
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 1459 "vtkParse.y"
    { }
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 1461 "vtkParse.y"
    { }
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1467 "vtkParse.y"
    { postSig("template<> "); clearTypeId(); }
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1468 "vtkParse.y"
    { postSig("template<");
          clearTypeId(); startTemplate(); }
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1470 "vtkParse.y"
    { chopSig();
            if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
            postSig("> "); clearTypeId(); }
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1475 "vtkParse.y"
    { chopSig(); postSig(", "); clearTypeId(); }
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1479 "vtkParse.y"
    {
               TemplateArg *arg = (TemplateArg *)malloc(sizeof(TemplateArg));
               vtkParse_InitTemplateArg(arg);
               arg->Type = (yyvsp[(1) - (2)].integer);
               arg->Class = vtkstrdup(getTypeId());
               arg->Name = vtkstrdup(getVarName());
               arg->Value = vtkstrdup(getVarValue());
               vtkParse_AddArgumentToTemplate(currentTemplate, arg);
               }
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1489 "vtkParse.y"
    {
               TemplateArg *arg = (TemplateArg *)malloc(sizeof(TemplateArg));
               vtkParse_InitTemplateArg(arg);
               arg->Name = vtkstrdup(getVarName());
               arg->Value = vtkstrdup(getVarValue());
               vtkParse_AddArgumentToTemplate(currentTemplate, arg);
               }
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1496 "vtkParse.y"
    { pushTemplate(); }
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1497 "vtkParse.y"
    {
               TemplateArgs *newTemplate = currentTemplate;
               TemplateArg *arg = (TemplateArg *)malloc(sizeof(TemplateArg));
               vtkParse_InitTemplateArg(arg);
               popTemplate();
               arg->Template = newTemplate;
               arg->Name = vtkstrdup(getVarName());
               arg->Value = vtkstrdup(getVarValue());
               vtkParse_AddArgumentToTemplate(currentTemplate, arg);
               }
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1508 "vtkParse.y"
    {postSig("class ");}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1509 "vtkParse.y"
    {postSig("typename ");}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1511 "vtkParse.y"
    { setVarName((yyvsp[(1) - (1)].str)); }
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1541 "vtkParse.y"
    {openSig(); preSig("~"); closeSig();}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1542 "vtkParse.y"
    {openSig(); preSig("~"); closeSig();}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1544 "vtkParse.y"
    {
         openSig(); preSig("virtual ~"); closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 1551 "vtkParse.y"
    {
         openSig();
         preSig("explicit ");
         closeSig();
         currentFunction->IsExplicit = 1;
         }
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 1559 "vtkParse.y"
    {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 1575 "vtkParse.y"
    {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1584 "vtkParse.y"
    {
      postSig("(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1588 "vtkParse.y"
    { postSig(")"); }
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1589 "vtkParse.y"
    {
      (yyval.integer) = (yyvsp[(2) - (8)].integer);
      postSig(";");
      preSig("operator ");
      closeSig();
      currentFunction->IsOperator = 1;
      currentFunction->Name = "operator typecast";
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed operator", "operator typecast");
    }
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 1600 "vtkParse.y"
    { postSig(")"); }
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 1601 "vtkParse.y"
    {
      postSig(";");
      closeSig();
      currentFunction->Name = vtkstrdup((yyvsp[(1) - (3)].str));
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed operator", currentFunction->Name);
    }
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 1609 "vtkParse.y"
    {postSig((yyvsp[(2) - (2)].str));}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 1610 "vtkParse.y"
    {
      postSig("(");
      currentFunction->IsOperator = 1;
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1615 "vtkParse.y"
    { (yyval.str) = (yyvsp[(2) - (7)].str); }
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1617 "vtkParse.y"
    { postSig(")"); }
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 1618 "vtkParse.y"
    {
      postSig(";");
      closeSig();
      currentFunction->Name = vtkstrdup((yyvsp[(1) - (3)].str));
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 1628 "vtkParse.y"
    {
      postSig(" = 0");
      if (currentClass)
        {
        currentFunction->IsPureVirtual = 1;
        currentClass->IsAbstract = 1;
        }
    }
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 1637 "vtkParse.y"
    {
      postSig(" const = 0");
      currentFunction->IsConst = 1;
      if (currentClass)
        {
        currentFunction->IsPureVirtual = 1;
        currentClass->IsAbstract = 1;
        }
    }
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 1647 "vtkParse.y"
    {
      postSig(" const");
      currentFunction->IsConst = 1;
    }
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 1655 "vtkParse.y"
    {
      postSig("(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 1658 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (5)].str); }
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 1659 "vtkParse.y"
    {markSig(); postSig("<");}
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 1660 "vtkParse.y"
    {
      if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
      (yyval.str) = vtkstrcat((yyvsp[(1) - (6)].str), copySig());
    }
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 1665 "vtkParse.y"
    { (yyval.str) = (yyvsp[(7) - (9)].str); }
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 1667 "vtkParse.y"
    { postSig(");"); closeSig(); }
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 1668 "vtkParse.y"
    {
      currentFunction->Name = vtkstrdup((yyvsp[(1) - (3)].str));
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 1674 "vtkParse.y"
    { postSig("("); }
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 1683 "vtkParse.y"
    {
      postSig(");");
      closeSig();
      currentFunction->Name = vtkstrcat("~", (yyvsp[(1) - (1)].str));
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 1691 "vtkParse.y"
    { postSig("(");}
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 1697 "vtkParse.y"
    {clearTypeId();}
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 1700 "vtkParse.y"
    { currentFunction->IsVariadic = 1; postSig("..."); }
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 1701 "vtkParse.y"
    { clearTypeId(); }
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 1702 "vtkParse.y"
    { clearTypeId(); postSig(", "); }
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 1705 "vtkParse.y"
    { markSig(); }
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 1707 "vtkParse.y"
    {
      int i = currentFunction->NumberOfArguments;
      ValueInfo *arg = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(arg);

      handle_complex_type(arg, (yyvsp[(2) - (3)].integer), (yyvsp[(3) - (3)].integer), copySig());

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
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 1729 "vtkParse.y"
    {
      int i = currentFunction->NumberOfArguments-1;
      if (getVarValue())
        {
        currentFunction->Arguments[i]->Value = vtkstrdup(getVarValue());
        }
    }
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 1737 "vtkParse.y"
    {
      int i = currentFunction->NumberOfArguments;
      ValueInfo *arg = (ValueInfo *)malloc(sizeof(ValueInfo));

      vtkParse_InitValue(arg);

      markSig();
      postSig("void (*");
      postSig((yyvsp[(1) - (1)].str));
      postSig(")(void *) ");

      handle_function_type(arg, (yyvsp[(1) - (1)].str), copySig());

      if (i < MAX_ARGS)
        {
        currentFunction->ArgTypes[i] = arg->Type;
        currentFunction->ArgClasses[i] = arg->Class;
        currentFunction->ArgCounts[i] = arg->Count;
        }

      vtkParse_AddArgumentToFunction(currentFunction, arg);
    }
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 1762 "vtkParse.y"
    {clearVarValue();}
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 1764 "vtkParse.y"
    { postSig("="); clearVarValue();}
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 1765 "vtkParse.y"
    { setVarValue((yyvsp[(3) - (3)].str)); }
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 1776 "vtkParse.y"
    {
       unsigned int type = getStorageType();
       if (getVarValue() && ((type & VTK_PARSE_CONST) != 0) &&
           ((type & VTK_PARSE_INDIRECT) == 0) && getArrayNDims() == 0)
         {
         add_constant(getVarName(), getVarValue(),
                       (type & VTK_PARSE_UNQUALIFIED_TYPE), getTypeId(), 0);
         }
     }
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 1790 "vtkParse.y"
    {postSig(", ");}
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 1793 "vtkParse.y"
    { setStorageTypeIndirection(0); }
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 1794 "vtkParse.y"
    { setStorageTypeIndirection((yyvsp[(1) - (1)].integer)); }
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 1798 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 1799 "vtkParse.y"
    { postSig(")"); }
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 1801 "vtkParse.y"
    {
         unsigned int parens = add_indirection((yyvsp[(1) - (5)].integer), (yyvsp[(2) - (5)].integer));
         if ((yyvsp[(5) - (5)].integer) == VTK_PARSE_FUNCTION) {
           (yyval.integer) = (parens | VTK_PARSE_FUNCTION); }
         else if ((yyvsp[(5) - (5)].integer) == VTK_PARSE_ARRAY) {
           (yyval.integer) = add_indirection_to_array(parens); }
       }
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 1810 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 1811 "vtkParse.y"
    { postSig(")"); }
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 1813 "vtkParse.y"
    {
         unsigned int parens = add_indirection((yyvsp[(1) - (5)].integer), (yyvsp[(2) - (5)].integer));
         if ((yyvsp[(5) - (5)].integer) == VTK_PARSE_FUNCTION) {
           (yyval.integer) = (parens | VTK_PARSE_FUNCTION); }
         else if ((yyvsp[(5) - (5)].integer) == VTK_PARSE_ARRAY) {
           (yyval.integer) = add_indirection_to_array(parens); }
       }
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 1821 "vtkParse.y"
    { postSig("("); (yyval.integer) = 0; }
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 1822 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("*");
               (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 1824 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("&");
               (yyval.integer) = VTK_PARSE_REF; }
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 1827 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("*");
               (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 1829 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("&");
               (yyval.integer) = VTK_PARSE_REF; }
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 1832 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 1833 "vtkParse.y"
    { pushFunction(); postSig("("); }
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 1834 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_FUNCTION; postSig(")"); popFunction(); }
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 1835 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_ARRAY; }
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 1838 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 1840 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(1) - (2)].integer), (yyvsp[(2) - (2)].integer));}
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 1843 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 1845 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(1) - (2)].integer), (yyvsp[(2) - (2)].integer));}
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 1847 "vtkParse.y"
    {clearVarName(); chopSig();}
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 1849 "vtkParse.y"
    {setVarName((yyvsp[(1) - (1)].str));}
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 1851 "vtkParse.y"
    {clearArray();}
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 1853 "vtkParse.y"
    {clearArray();}
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 1855 "vtkParse.y"
    {postSig("[");}
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 1855 "vtkParse.y"
    {postSig("]");}
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 1859 "vtkParse.y"
    {pushArraySize("");}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 1860 "vtkParse.y"
    {pushArraySize((yyvsp[(1) - (1)].str));}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 1866 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 1867 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 1868 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 1869 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 1870 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 1871 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 1878 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer); setStorageType((yyval.integer));}
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 1879 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 1880 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 1882 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(3) - (3)].integer); setStorageType((yyval.integer));}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 1883 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 1884 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer));
      setStorageType((yyval.integer));}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 1886 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(2) - (3)].integer) | (yyvsp[(3) - (3)].integer));
      setStorageType((yyval.integer));}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 1890 "vtkParse.y"
    {postSig("static "); (yyval.integer) = VTK_PARSE_STATIC; }
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 1891 "vtkParse.y"
    {postSig("static "); (yyval.integer) = VTK_PARSE_STATIC; }
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 1893 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 1894 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer));}
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 1896 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 1897 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_CONST | (yyvsp[(2) - (2)].integer));}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 1898 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_CONST | (yyvsp[(1) - (2)].integer));}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 1900 "vtkParse.y"
    {postSig("const ");}
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 1904 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 1906 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 1907 "vtkParse.y"
    {postSig("typename ");}
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 1908 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(3) - (3)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 1911 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 1912 "vtkParse.y"
    {chopSig(); if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 294:

/* Line 1455 of yacc.c  */
#line 1914 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 295:

/* Line 1455 of yacc.c  */
#line 1915 "vtkParse.y"
    {chopSig(); if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 1918 "vtkParse.y"
    {postSig(", ");}
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 1920 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 1921 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 1922 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 302:

/* Line 1455 of yacc.c  */
#line 1923 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 303:

/* Line 1455 of yacc.c  */
#line 1924 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 304:

/* Line 1455 of yacc.c  */
#line 1925 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 307:

/* Line 1455 of yacc.c  */
#line 1930 "vtkParse.y"
    {
             (yyval.str) = vtkstrcat3((yyvsp[(1) - (3)].str), "::", (yyvsp[(3) - (3)].str));
             preScopeSig((yyvsp[(1) - (3)].str));
           }
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 1935 "vtkParse.y"
    {
             (yyval.str) = vtkstrcat3((yyvsp[(1) - (3)].str), "::", (yyvsp[(3) - (3)].str));
             preScopeSig("");
           }
    break;

  case 315:

/* Line 1455 of yacc.c  */
#line 1956 "vtkParse.y"
    { postSig("&"); (yyval.integer) = VTK_PARSE_REF;}
    break;

  case 316:

/* Line 1455 of yacc.c  */
#line 1957 "vtkParse.y"
    { postSig("&"); (yyval.integer) = ((yyvsp[(1) - (2)].integer) | VTK_PARSE_REF);}
    break;

  case 317:

/* Line 1455 of yacc.c  */
#line 1958 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 318:

/* Line 1455 of yacc.c  */
#line 1963 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 1965 "vtkParse.y"
    {
       unsigned int n;
       n = (((yyvsp[(1) - (2)].integer) << 2) | (yyvsp[(2) - (2)].integer));
       if ((n & VTK_PARSE_INDIRECT) != n)
         {
         n = VTK_PARSE_BAD_INDIRECT;
         }
      (yyval.integer) = n;
    }
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 1976 "vtkParse.y"
    { postSig("*"); (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 321:

/* Line 1455 of yacc.c  */
#line 1977 "vtkParse.y"
    { postSig("*const "); (yyval.integer) = VTK_PARSE_CONST_POINTER; }
    break;

  case 322:

/* Line 1455 of yacc.c  */
#line 1980 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 323:

/* Line 1455 of yacc.c  */
#line 1981 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); }
    break;

  case 324:

/* Line 1455 of yacc.c  */
#line 1982 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); }
    break;

  case 325:

/* Line 1455 of yacc.c  */
#line 1983 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 326:

/* Line 1455 of yacc.c  */
#line 1984 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 1985 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 1986 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 329:

/* Line 1455 of yacc.c  */
#line 1989 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 1990 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 331:

/* Line 1455 of yacc.c  */
#line 1993 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_STRING;}
    break;

  case 332:

/* Line 1455 of yacc.c  */
#line 1994 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNICODE_STRING;}
    break;

  case 333:

/* Line 1455 of yacc.c  */
#line 1995 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_OSTREAM; }
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 1996 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_ISTREAM; }
    break;

  case 335:

/* Line 1455 of yacc.c  */
#line 1997 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 336:

/* Line 1455 of yacc.c  */
#line 1998 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_OBJECT; }
    break;

  case 337:

/* Line 1455 of yacc.c  */
#line 2001 "vtkParse.y"
    { typeSig("void"); (yyval.integer) = VTK_PARSE_VOID;}
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 2002 "vtkParse.y"
    { typeSig("bool"); (yyval.integer) = VTK_PARSE_BOOL;}
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 2003 "vtkParse.y"
    { typeSig("ssize_t"); (yyval.integer) = VTK_PARSE_SSIZE_T;}
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 2004 "vtkParse.y"
    { typeSig("size_t"); (yyval.integer) = VTK_PARSE_SIZE_T;}
    break;

  case 341:

/* Line 1455 of yacc.c  */
#line 2005 "vtkParse.y"
    { typeSig("vtkTypeInt8"); (yyval.integer) = VTK_PARSE_INT8; }
    break;

  case 342:

/* Line 1455 of yacc.c  */
#line 2006 "vtkParse.y"
    { typeSig("vtkTypeUInt8"); (yyval.integer) = VTK_PARSE_UINT8; }
    break;

  case 343:

/* Line 1455 of yacc.c  */
#line 2007 "vtkParse.y"
    { typeSig("vtkTypeInt16"); (yyval.integer) = VTK_PARSE_INT16; }
    break;

  case 344:

/* Line 1455 of yacc.c  */
#line 2008 "vtkParse.y"
    { typeSig("vtkTypeUInt16"); (yyval.integer) = VTK_PARSE_UINT16; }
    break;

  case 345:

/* Line 1455 of yacc.c  */
#line 2009 "vtkParse.y"
    { typeSig("vtkTypeInt32"); (yyval.integer) = VTK_PARSE_INT32; }
    break;

  case 346:

/* Line 1455 of yacc.c  */
#line 2010 "vtkParse.y"
    { typeSig("vtkTypeUInt32"); (yyval.integer) = VTK_PARSE_UINT32; }
    break;

  case 347:

/* Line 1455 of yacc.c  */
#line 2011 "vtkParse.y"
    { typeSig("vtkTypeInt64"); (yyval.integer) = VTK_PARSE_INT64; }
    break;

  case 348:

/* Line 1455 of yacc.c  */
#line 2012 "vtkParse.y"
    { typeSig("vtkTypeUInt64"); (yyval.integer) = VTK_PARSE_UINT64; }
    break;

  case 349:

/* Line 1455 of yacc.c  */
#line 2013 "vtkParse.y"
    { typeSig("vtkTypeFloat32"); (yyval.integer) = VTK_PARSE_FLOAT32; }
    break;

  case 350:

/* Line 1455 of yacc.c  */
#line 2014 "vtkParse.y"
    { typeSig("vtkTypeFloat64"); (yyval.integer) = VTK_PARSE_FLOAT64; }
    break;

  case 351:

/* Line 1455 of yacc.c  */
#line 2015 "vtkParse.y"
    { typeSig("vtkIdType"); (yyval.integer) = VTK_PARSE_ID_TYPE;}
    break;

  case 352:

/* Line 1455 of yacc.c  */
#line 2016 "vtkParse.y"
    { typeSig("float"); (yyval.integer) = VTK_PARSE_FLOAT;}
    break;

  case 353:

/* Line 1455 of yacc.c  */
#line 2017 "vtkParse.y"
    { typeSig("double"); (yyval.integer) = VTK_PARSE_DOUBLE;}
    break;

  case 354:

/* Line 1455 of yacc.c  */
#line 2018 "vtkParse.y"
    { typeSig("long double"); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 355:

/* Line 1455 of yacc.c  */
#line 2019 "vtkParse.y"
    { typeSig("char"); (yyval.integer) = VTK_PARSE_CHAR;}
    break;

  case 356:

/* Line 1455 of yacc.c  */
#line 2020 "vtkParse.y"
    { typeSig("signed char"); (yyval.integer) = VTK_PARSE_SIGNED_CHAR;}
    break;

  case 357:

/* Line 1455 of yacc.c  */
#line 2022 "vtkParse.y"
    { typeSig("unsigned char"); (yyval.integer) = VTK_PARSE_UNSIGNED_CHAR;}
    break;

  case 358:

/* Line 1455 of yacc.c  */
#line 2023 "vtkParse.y"
    { typeSig("int"); (yyval.integer) = VTK_PARSE_INT;}
    break;

  case 359:

/* Line 1455 of yacc.c  */
#line 2025 "vtkParse.y"
    { typeSig("unsigned int"); (yyval.integer) = VTK_PARSE_UNSIGNED_INT;}
    break;

  case 360:

/* Line 1455 of yacc.c  */
#line 2026 "vtkParse.y"
    { typeSig("short"); (yyval.integer) = VTK_PARSE_SHORT;}
    break;

  case 361:

/* Line 1455 of yacc.c  */
#line 2028 "vtkParse.y"
    { typeSig("unsigned short"); (yyval.integer) = VTK_PARSE_UNSIGNED_SHORT;}
    break;

  case 362:

/* Line 1455 of yacc.c  */
#line 2029 "vtkParse.y"
    { typeSig("long"); (yyval.integer) = VTK_PARSE_LONG;}
    break;

  case 363:

/* Line 1455 of yacc.c  */
#line 2031 "vtkParse.y"
    { typeSig("unsigned long"); (yyval.integer) = VTK_PARSE_UNSIGNED_LONG;}
    break;

  case 364:

/* Line 1455 of yacc.c  */
#line 2032 "vtkParse.y"
    { typeSig("long long"); (yyval.integer) = VTK_PARSE_LONG_LONG;}
    break;

  case 365:

/* Line 1455 of yacc.c  */
#line 2034 "vtkParse.y"
    {typeSig("unsigned long long");(yyval.integer)=VTK_PARSE_UNSIGNED_LONG_LONG;}
    break;

  case 366:

/* Line 1455 of yacc.c  */
#line 2035 "vtkParse.y"
    { typeSig("__int64"); (yyval.integer) = VTK_PARSE___INT64;}
    break;

  case 367:

/* Line 1455 of yacc.c  */
#line 2037 "vtkParse.y"
    { typeSig("unsigned __int64"); (yyval.integer) = VTK_PARSE_UNSIGNED___INT64;}
    break;

  case 368:

/* Line 1455 of yacc.c  */
#line 2038 "vtkParse.y"
    { typeSig("int"); (yyval.integer) = VTK_PARSE_INT; }
    break;

  case 369:

/* Line 1455 of yacc.c  */
#line 2039 "vtkParse.y"
    { typeSig("unsigned int"); (yyval.integer) = VTK_PARSE_UNSIGNED_INT; }
    break;

  case 370:

/* Line 1455 of yacc.c  */
#line 2045 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 371:

/* Line 1455 of yacc.c  */
#line 2046 "vtkParse.y"
    { postSig("{ "); }
    break;

  case 372:

/* Line 1455 of yacc.c  */
#line 2047 "vtkParse.y"
    {
          postSig("}");
          (yyval.str) = vtkstrcat4("{ ", (yyvsp[(3) - (6)].str), (yyvsp[(4) - (6)].str), " }");
        }
    break;

  case 375:

/* Line 1455 of yacc.c  */
#line 2054 "vtkParse.y"
    {(yyval.str) = "";}
    break;

  case 376:

/* Line 1455 of yacc.c  */
#line 2055 "vtkParse.y"
    { postSig(", "); }
    break;

  case 377:

/* Line 1455 of yacc.c  */
#line 2056 "vtkParse.y"
    {
          (yyval.str) = vtkstrcat3((yyvsp[(1) - (4)].str), ", ", (yyvsp[(4) - (4)].str));
        }
    break;

  case 378:

/* Line 1455 of yacc.c  */
#line 2060 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 379:

/* Line 1455 of yacc.c  */
#line 2061 "vtkParse.y"
    {postSig("+");}
    break;

  case 380:

/* Line 1455 of yacc.c  */
#line 2061 "vtkParse.y"
    {(yyval.str) = (yyvsp[(3) - (3)].str);}
    break;

  case 381:

/* Line 1455 of yacc.c  */
#line 2062 "vtkParse.y"
    {postSig("-");}
    break;

  case 382:

/* Line 1455 of yacc.c  */
#line 2063 "vtkParse.y"
    {
             (yyval.str) = vtkstrcat("-", (yyvsp[(3) - (3)].str));
             }
    break;

  case 383:

/* Line 1455 of yacc.c  */
#line 2066 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 384:

/* Line 1455 of yacc.c  */
#line 2067 "vtkParse.y"
    {postSig("(");}
    break;

  case 385:

/* Line 1455 of yacc.c  */
#line 2067 "vtkParse.y"
    {postSig(")"); (yyval.str) = (yyvsp[(3) - (4)].str);}
    break;

  case 386:

/* Line 1455 of yacc.c  */
#line 2068 "vtkParse.y"
    {postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 387:

/* Line 1455 of yacc.c  */
#line 2070 "vtkParse.y"
    {
             chopSig();
             if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
             postSig(">(");
             }
    break;

  case 388:

/* Line 1455 of yacc.c  */
#line 2076 "vtkParse.y"
    {
             postSig(")");
             if (getTypeId()[strlen(getTypeId())-1] == '>')
               {
               (yyval.str) = vtkstrcat6(
                 (yyvsp[(1) - (9)].str), "<", getTypeId(), " >(", (yyvsp[(8) - (9)].str), ")");
               }
             else
               {
               (yyval.str) = vtkstrcat6(
                 (yyvsp[(1) - (9)].str), "<", getTypeId(), ">(", (yyvsp[(8) - (9)].str), ")");
               }
             }
    break;

  case 389:

/* Line 1455 of yacc.c  */
#line 2090 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 390:

/* Line 1455 of yacc.c  */
#line 2092 "vtkParse.y"
    { (yyval.str) = vtkstrcat((yyvsp[(1) - (2)].str), (yyvsp[(2) - (2)].str)); }
    break;

  case 391:

/* Line 1455 of yacc.c  */
#line 2094 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 392:

/* Line 1455 of yacc.c  */
#line 2095 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 393:

/* Line 1455 of yacc.c  */
#line 2096 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 394:

/* Line 1455 of yacc.c  */
#line 2097 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 395:

/* Line 1455 of yacc.c  */
#line 2098 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 396:

/* Line 1455 of yacc.c  */
#line 2099 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 397:

/* Line 1455 of yacc.c  */
#line 2100 "vtkParse.y"
    {(yyval.str) = vtkstrdup(add_const_scope((yyvsp[(1) - (1)].str)));
                postSig((yyvsp[(1) - (1)].str));}
    break;

  case 398:

/* Line 1455 of yacc.c  */
#line 2102 "vtkParse.y"
    {(yyval.str) = vtkstrdup(add_const_scope((yyvsp[(1) - (1)].str)));
                postSig((yyvsp[(1) - (1)].str));}
    break;

  case 399:

/* Line 1455 of yacc.c  */
#line 2112 "vtkParse.y"
    {preSig("void Set"); postSig("(");}
    break;

  case 400:

/* Line 1455 of yacc.c  */
#line 2113 "vtkParse.y"
    {
   postSig("a);");
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (7)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, (yyvsp[(6) - (7)].integer), getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 401:

/* Line 1455 of yacc.c  */
#line 2121 "vtkParse.y"
    {postSig("Get");}
    break;

  case 402:

/* Line 1455 of yacc.c  */
#line 2122 "vtkParse.y"
    {markSig();}
    break;

  case 403:

/* Line 1455 of yacc.c  */
#line 2122 "vtkParse.y"
    {swapSig();}
    break;

  case 404:

/* Line 1455 of yacc.c  */
#line 2123 "vtkParse.y"
    {
   postSig("();");
   currentFunction->Name = vtkstrcat("Get", (yyvsp[(4) - (9)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, (yyvsp[(7) - (9)].integer), getTypeId(), 0);
   output_function();
   }
    break;

  case 405:

/* Line 1455 of yacc.c  */
#line 2130 "vtkParse.y"
    {preSig("void Set");}
    break;

  case 406:

/* Line 1455 of yacc.c  */
#line 2131 "vtkParse.y"
    {
   postSig("(char *);");
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(4) - (5)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 407:

/* Line 1455 of yacc.c  */
#line 2139 "vtkParse.y"
    {preSig("char *Get");}
    break;

  case 408:

/* Line 1455 of yacc.c  */
#line 2140 "vtkParse.y"
    {
   postSig("();");
   currentFunction->Name = vtkstrcat("Get", (yyvsp[(4) - (5)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   output_function();
   }
    break;

  case 409:

/* Line 1455 of yacc.c  */
#line 2147 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 410:

/* Line 1455 of yacc.c  */
#line 2147 "vtkParse.y"
    {closeSig();}
    break;

  case 411:

/* Line 1455 of yacc.c  */
#line 2149 "vtkParse.y"
    {
   const char *typeText;
   chopSig();
   typeText = copySig();

   currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (10)].str));
   currentFunction->Signature =
     vtkstrcat5("void ", currentFunction->Name, "(", typeText, ");");
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, (yyvsp[(6) - (10)].integer), getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();

   currentFunction->Name = vtkstrcat3("Get", (yyvsp[(3) - (10)].str), "MinValue");
   currentFunction->Signature =
     vtkstrcat4(typeText, " ", currentFunction->Name, "();");
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, (yyvsp[(6) - (10)].integer), getTypeId(), 0);
   output_function();

   currentFunction->Name = vtkstrcat3("Get", (yyvsp[(3) - (10)].str), "MaxValue");
   currentFunction->Signature =
     vtkstrcat4(typeText, " ", currentFunction->Name, "();");
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, (yyvsp[(6) - (10)].integer), getTypeId(), 0);
   output_function();
   }
    break;

  case 412:

/* Line 1455 of yacc.c  */
#line 2177 "vtkParse.y"
    {preSig("void Set"); postSig("("); }
    break;

  case 413:

/* Line 1455 of yacc.c  */
#line 2178 "vtkParse.y"
    {
   postSig("*);");
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (7)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 414:

/* Line 1455 of yacc.c  */
#line 2186 "vtkParse.y"
    {postSig("*Get");}
    break;

  case 415:

/* Line 1455 of yacc.c  */
#line 2187 "vtkParse.y"
    {markSig();}
    break;

  case 416:

/* Line 1455 of yacc.c  */
#line 2187 "vtkParse.y"
    {swapSig();}
    break;

  case 417:

/* Line 1455 of yacc.c  */
#line 2188 "vtkParse.y"
    {
   postSig("();");
   currentFunction->Name = vtkstrcat("Get", (yyvsp[(4) - (9)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   output_function();
   }
    break;

  case 418:

/* Line 1455 of yacc.c  */
#line 2196 "vtkParse.y"
    {
   currentFunction->Name = vtkstrcat((yyvsp[(3) - (6)].str), "On");
   currentFunction->Comment = vtkstrdup(getComment());
   currentFunction->Signature =
     vtkstrcat3("void ", currentFunction->Name, "();");
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();

   currentFunction->Name = vtkstrcat((yyvsp[(3) - (6)].str), "Off");
   currentFunction->Comment = vtkstrdup(getComment());
   currentFunction->Signature =
     vtkstrcat3("void ", currentFunction->Name, "();");
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 419:

/* Line 1455 of yacc.c  */
#line 2211 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 420:

/* Line 1455 of yacc.c  */
#line 2212 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 2);
   }
    break;

  case 421:

/* Line 1455 of yacc.c  */
#line 2216 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 422:

/* Line 1455 of yacc.c  */
#line 2217 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 2);
   }
    break;

  case 423:

/* Line 1455 of yacc.c  */
#line 2221 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 424:

/* Line 1455 of yacc.c  */
#line 2222 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 3);
   }
    break;

  case 425:

/* Line 1455 of yacc.c  */
#line 2226 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 426:

/* Line 1455 of yacc.c  */
#line 2227 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 3);
   }
    break;

  case 427:

/* Line 1455 of yacc.c  */
#line 2231 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 428:

/* Line 1455 of yacc.c  */
#line 2232 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 4);
   }
    break;

  case 429:

/* Line 1455 of yacc.c  */
#line 2236 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 430:

/* Line 1455 of yacc.c  */
#line 2237 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 4);
   }
    break;

  case 431:

/* Line 1455 of yacc.c  */
#line 2241 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 432:

/* Line 1455 of yacc.c  */
#line 2242 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 6);
   }
    break;

  case 433:

/* Line 1455 of yacc.c  */
#line 2246 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 434:

/* Line 1455 of yacc.c  */
#line 2247 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 6);
   }
    break;

  case 435:

/* Line 1455 of yacc.c  */
#line 2251 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 436:

/* Line 1455 of yacc.c  */
#line 2253 "vtkParse.y"
    {
   const char *typeText;
   chopSig();
   typeText = copySig();
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (9)].str));
   currentFunction->Signature =
     vtkstrcat7("void ", currentFunction->Name, "(", typeText,
                " a[", (yyvsp[(8) - (9)].str), "]);");
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, (VTK_PARSE_POINTER | (yyvsp[(6) - (9)].integer)),
                getTypeId(), (int)strtol((yyvsp[(8) - (9)].str), NULL, 0));
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 437:

/* Line 1455 of yacc.c  */
#line 2267 "vtkParse.y"
    {startSig();}
    break;

  case 438:

/* Line 1455 of yacc.c  */
#line 2269 "vtkParse.y"
    {
   chopSig();
   currentFunction->Name = vtkstrcat("Get", (yyvsp[(3) - (9)].str));
   postSig(" *");
   postSig(currentFunction->Name);
   postSig("();");
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, (VTK_PARSE_POINTER | (yyvsp[(6) - (9)].integer)),
              getTypeId(), (int)strtol((yyvsp[(8) - (9)].str), NULL, 0));
   output_function();
   }
    break;

  case 439:

/* Line 1455 of yacc.c  */
#line 2281 "vtkParse.y"
    {
     currentFunction->Name = vtkstrcat3("Get", (yyvsp[(3) - (4)].str), "Coordinate");
     currentFunction->Signature =
       vtkstrcat3("vtkCoordinate *", currentFunction->Name, "();");
     currentFunction->Comment = vtkstrdup(getComment());
     set_return(currentFunction, VTK_PARSE_OBJECT_PTR, "vtkCoordinate", 0);
     output_function();

     currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (4)].str));
     currentFunction->Signature =
       vtkstrcat3("void ", currentFunction->Name, "(double, double);");
     currentFunction->Comment = vtkstrdup(getComment());
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (4)].str));
     currentFunction->Signature =
       vtkstrcat3("void ", currentFunction->Name, "(double a[2]);");
     currentFunction->Comment = vtkstrdup(getComment());
     add_argument(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 2);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Name = vtkstrcat("Get", (yyvsp[(3) - (4)].str));
     currentFunction->Signature =
       vtkstrcat3("double *", currentFunction->Name, "();");
     currentFunction->Comment = vtkstrdup(getComment());
     set_return(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 2);
     output_function();
   }
    break;

  case 440:

/* Line 1455 of yacc.c  */
#line 2314 "vtkParse.y"
    {
     currentFunction->Name = vtkstrcat3("Get", (yyvsp[(3) - (4)].str), "Coordinate");
     currentFunction->Signature =
       vtkstrcat3("vtkCoordinate *", currentFunction->Name, "();");
     currentFunction->Comment = vtkstrdup(getComment());
     set_return(currentFunction, VTK_PARSE_OBJECT_PTR, "vtkCoordinate", 0);
     output_function();

     currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (4)].str));
     currentFunction->Signature =
       vtkstrcat3("void ", currentFunction->Name, "(double, double, double);");
     currentFunction->Comment = vtkstrdup(getComment());
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (4)].str));
     currentFunction->Signature =
       vtkstrcat3("void ", currentFunction->Name, "(double a[3]);");
     currentFunction->Comment = vtkstrdup(getComment());
     add_argument(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 3);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Name = vtkstrcat("Get", (yyvsp[(3) - (4)].str));
     currentFunction->Signature =
       vtkstrcat3("double *", currentFunction->Name, "();");
     currentFunction->Comment = vtkstrdup(getComment());
     set_return(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 3);
     output_function();
   }
    break;

  case 441:

/* Line 1455 of yacc.c  */
#line 2348 "vtkParse.y"
    {
   int is_concrete = 0;
   int i;

   currentFunction->Name = vtkstrdup("GetClassName");
   currentFunction->Signature = vtkstrdup("const char *GetClassName();");
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR),
              "char", 0);
   output_function();

   currentFunction->Name = vtkstrdup("IsA");
   currentFunction->Signature = vtkstrdup("int IsA(const char *name);");
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR),
                "char", 0);
   set_return(currentFunction, VTK_PARSE_INT, "int", 0);
   output_function();

   currentFunction->Name = vtkstrdup("NewInstance");
   currentFunction->Signature = vtkstrcat((yyvsp[(3) - (7)].str), " *NewInstance();");
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_OBJECT_PTR, (yyvsp[(3) - (7)].str), 0);
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
     currentFunction->Name = vtkstrdup("SafeDownCast");
     currentFunction->Signature =
       vtkstrcat((yyvsp[(3) - (7)].str), " *SafeDownCast(vtkObject* o);");
     currentFunction->Comment = vtkstrdup(getComment());
     add_argument(currentFunction, VTK_PARSE_OBJECT_PTR, "vtkObject", 0);
     set_return(currentFunction, (VTK_PARSE_STATIC | VTK_PARSE_OBJECT_PTR),
                (yyvsp[(3) - (7)].str), 0);
     output_function();
     }
   }
    break;

  case 442:

/* Line 1455 of yacc.c  */
#line 2400 "vtkParse.y"
    { (yyval.str) = "operator()"; }
    break;

  case 443:

/* Line 1455 of yacc.c  */
#line 2401 "vtkParse.y"
    { (yyval.str) = "operator[]"; }
    break;

  case 444:

/* Line 1455 of yacc.c  */
#line 2402 "vtkParse.y"
    { (yyval.str) = "operator new[]"; }
    break;

  case 445:

/* Line 1455 of yacc.c  */
#line 2403 "vtkParse.y"
    { (yyval.str) = "operator delete[]"; }
    break;

  case 447:

/* Line 1455 of yacc.c  */
#line 2406 "vtkParse.y"
    { (yyval.str) = "operator="; }
    break;

  case 448:

/* Line 1455 of yacc.c  */
#line 2407 "vtkParse.y"
    { (yyval.str) = "operator*"; }
    break;

  case 449:

/* Line 1455 of yacc.c  */
#line 2407 "vtkParse.y"
    { (yyval.str) = "operator/"; }
    break;

  case 450:

/* Line 1455 of yacc.c  */
#line 2408 "vtkParse.y"
    { (yyval.str) = "operator-"; }
    break;

  case 451:

/* Line 1455 of yacc.c  */
#line 2408 "vtkParse.y"
    { (yyval.str) = "operator+"; }
    break;

  case 452:

/* Line 1455 of yacc.c  */
#line 2409 "vtkParse.y"
    { (yyval.str) = "operator!"; }
    break;

  case 453:

/* Line 1455 of yacc.c  */
#line 2409 "vtkParse.y"
    { (yyval.str) = "operator~"; }
    break;

  case 454:

/* Line 1455 of yacc.c  */
#line 2410 "vtkParse.y"
    { (yyval.str) = "operator,"; }
    break;

  case 455:

/* Line 1455 of yacc.c  */
#line 2410 "vtkParse.y"
    { (yyval.str) = "operator<"; }
    break;

  case 456:

/* Line 1455 of yacc.c  */
#line 2411 "vtkParse.y"
    { (yyval.str) = "operator>"; }
    break;

  case 457:

/* Line 1455 of yacc.c  */
#line 2411 "vtkParse.y"
    { (yyval.str) = "operator&"; }
    break;

  case 458:

/* Line 1455 of yacc.c  */
#line 2412 "vtkParse.y"
    { (yyval.str) = "operator|"; }
    break;

  case 459:

/* Line 1455 of yacc.c  */
#line 2412 "vtkParse.y"
    { (yyval.str) = "operator^"; }
    break;

  case 460:

/* Line 1455 of yacc.c  */
#line 2413 "vtkParse.y"
    { (yyval.str) = "operator%"; }
    break;

  case 461:

/* Line 1455 of yacc.c  */
#line 2414 "vtkParse.y"
    { (yyval.str) = "operator new"; }
    break;

  case 462:

/* Line 1455 of yacc.c  */
#line 2415 "vtkParse.y"
    { (yyval.str) = "operator delete"; }
    break;

  case 463:

/* Line 1455 of yacc.c  */
#line 2416 "vtkParse.y"
    { (yyval.str) = "operator<<="; }
    break;

  case 464:

/* Line 1455 of yacc.c  */
#line 2417 "vtkParse.y"
    { (yyval.str) = "operator>>="; }
    break;

  case 465:

/* Line 1455 of yacc.c  */
#line 2418 "vtkParse.y"
    { (yyval.str) = "operator<<"; }
    break;

  case 466:

/* Line 1455 of yacc.c  */
#line 2419 "vtkParse.y"
    { (yyval.str) = "operator>>"; }
    break;

  case 467:

/* Line 1455 of yacc.c  */
#line 2420 "vtkParse.y"
    { (yyval.str) = "operator->*"; }
    break;

  case 468:

/* Line 1455 of yacc.c  */
#line 2421 "vtkParse.y"
    { (yyval.str) = "operator->"; }
    break;

  case 469:

/* Line 1455 of yacc.c  */
#line 2422 "vtkParse.y"
    { (yyval.str) = "operator+="; }
    break;

  case 470:

/* Line 1455 of yacc.c  */
#line 2423 "vtkParse.y"
    { (yyval.str) = "operator-="; }
    break;

  case 471:

/* Line 1455 of yacc.c  */
#line 2424 "vtkParse.y"
    { (yyval.str) = "operator*="; }
    break;

  case 472:

/* Line 1455 of yacc.c  */
#line 2425 "vtkParse.y"
    { (yyval.str) = "operator/="; }
    break;

  case 473:

/* Line 1455 of yacc.c  */
#line 2426 "vtkParse.y"
    { (yyval.str) = "operator%="; }
    break;

  case 474:

/* Line 1455 of yacc.c  */
#line 2427 "vtkParse.y"
    { (yyval.str) = "operator++"; }
    break;

  case 475:

/* Line 1455 of yacc.c  */
#line 2428 "vtkParse.y"
    { (yyval.str) = "operator--"; }
    break;

  case 476:

/* Line 1455 of yacc.c  */
#line 2429 "vtkParse.y"
    { (yyval.str) = "operator&="; }
    break;

  case 477:

/* Line 1455 of yacc.c  */
#line 2430 "vtkParse.y"
    { (yyval.str) = "operator|="; }
    break;

  case 478:

/* Line 1455 of yacc.c  */
#line 2431 "vtkParse.y"
    { (yyval.str) = "operator^="; }
    break;

  case 479:

/* Line 1455 of yacc.c  */
#line 2432 "vtkParse.y"
    {(yyval.str) = "operator&&=";}
    break;

  case 480:

/* Line 1455 of yacc.c  */
#line 2433 "vtkParse.y"
    {(yyval.str) = "operator||=";}
    break;

  case 481:

/* Line 1455 of yacc.c  */
#line 2434 "vtkParse.y"
    { (yyval.str) = "operator&&"; }
    break;

  case 482:

/* Line 1455 of yacc.c  */
#line 2435 "vtkParse.y"
    { (yyval.str) = "operator||"; }
    break;

  case 483:

/* Line 1455 of yacc.c  */
#line 2436 "vtkParse.y"
    { (yyval.str) = "operator=="; }
    break;

  case 484:

/* Line 1455 of yacc.c  */
#line 2437 "vtkParse.y"
    { (yyval.str) = "operator!="; }
    break;

  case 485:

/* Line 1455 of yacc.c  */
#line 2438 "vtkParse.y"
    { (yyval.str) = "operator<="; }
    break;

  case 486:

/* Line 1455 of yacc.c  */
#line 2439 "vtkParse.y"
    { (yyval.str) = "operator>="; }
    break;



/* Line 1455 of yacc.c  */
#line 7501 "vtkParse.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
        YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
        if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
          {
            YYSIZE_T yyalloc = 2 * yysize;
            if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
              yyalloc = YYSTACK_ALLOC_MAXIMUM;
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yyalloc);
            if (yymsg)
              yymsg_alloc = yyalloc;
            else
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
              }
          }

        if (0 < yysize && yysize <= yymsg_alloc)
          {
            (void) yysyntax_error (yymsg, yystate, yychar);
            yyerror (yymsg);
          }
        else
          {
            yyerror (YY_("syntax error"));
            if (yysize != 0)
              goto yyexhaustedlab;
          }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;        /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
                 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 2463 "vtkParse.y"

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
  func->IsExplicit = 0;
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
  cls->NumberOfClasses = 0;
  cls->NumberOfFunctions = 0;
  cls->NumberOfConstants = 0;
  cls->NumberOfVariables = 0;
  cls->NumberOfEnums = 0;
  cls->NumberOfUnions = 0;
  cls->NumberOfTypedefs = 0;
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
  name_info->NumberOfVariables = 0;
  name_info->NumberOfEnums = 0;
  name_info->NumberOfUnions = 0;
  name_info->NumberOfTypedefs = 0;
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

void vtkParse_FreeTemplate(TemplateArgs *template_info)
{
  int j, m;

  m = template_info->NumberOfArguments;
  for (j = 0; j < m; j++)
    {
    if (template_info->Arguments[j]->Template)
      {
      vtkParse_FreeTemplate(template_info->Arguments[j]->Template);
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

void vtkParse_FreeUnion(UnionInfo *union_info)
{
  int j, m;

  m = union_info->NumberOfMembers;
  for (j = 0; j < m; j++) { vtkParse_FreeValue(union_info->Members[j]); }
  if (m > 0) { free(union_info->Members); }

  free(union_info);
}

void vtkParse_FreeEnum(EnumInfo *enum_info)
{
  free(enum_info);
}

void vtkParse_FreeFunction(FunctionInfo *function_info)
{
  int j, m;

  if (function_info->Template)
    {
    vtkParse_FreeTemplate(function_info->Template);
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

  if (class_info->Template) { vtkParse_FreeTemplate(class_info->Template); }

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

  m = class_info->NumberOfUnions;
  for (j = 0; j < m; j++) { vtkParse_FreeUnion(class_info->Unions[j]); }
  if (m > 0) { free(class_info->Unions); }

  m = class_info->NumberOfTypedefs;
  for (j = 0; j < m; j++) { vtkParse_FreeValue(class_info->Typedefs[j]); }
  if (m > 0) { free(class_info->Typedefs); }

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
  for (j=0; j<m; j++) {vtkParse_FreeFunction(namespace_info->Functions[j]);}
  if (m > 0) { free(namespace_info->Functions); }

  m = namespace_info->NumberOfConstants;
  for (j = 0; j < m; j++) { vtkParse_FreeValue(namespace_info->Constants[j]); }
  if (m > 0) { free(namespace_info->Constants); }

  m = namespace_info->NumberOfVariables;
  for (j = 0; j < m; j++) { vtkParse_FreeValue(namespace_info->Variables[j]); }
  if (m > 0) { free(namespace_info->Variables); }

  m = namespace_info->NumberOfEnums;
  for (j = 0; j < m; j++) { vtkParse_FreeEnum(namespace_info->Enums[j]); }
  if (m > 0) { free(namespace_info->Enums); }

  m = namespace_info->NumberOfUnions;
  for (j = 0; j < m; j++) { vtkParse_FreeUnion(namespace_info->Unions[j]); }
  if (m > 0) { free(namespace_info->Unions); }

  m = namespace_info->NumberOfTypedefs;
  for (j = 0; j < m; j++) { vtkParse_FreeValue(namespace_info->Typedefs[j]); }
  if (m > 0) { free(namespace_info->Typedefs); }

  m = namespace_info->NumberOfNamespaces;
  for (j=0; j<m; j++) {vtkParse_FreeNamespace(namespace_info->Namespaces[j]);}
  if (m > 0) { free(namespace_info->Namespaces); }

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
  vtkParse_AddClassToNamespace(currentNamespace, currentClass);

  /* template information */
  if (currentTemplate)
    {
    currentClass->Template = currentTemplate;
    currentTemplate = NULL;
    }

  /* comment, if any */
  currentClass->Comment = vtkstrdup(getComment());

  access_level = VTK_ACCESS_PRIVATE;
  if (is_struct)
    {
    access_level = VTK_ACCESS_PUBLIC;
    }

  vtkParse_InitFunction(currentFunction);
  startSig();
  clearComment();
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
  startSig();
  clearComment();
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
  int i;
  char ntext[32];

  sprintf(ntext, "%i", n);

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

/* Add a UnionInfo to a ClassInfo */
void vtkParse_AddUnionToClass(ClassInfo *info, UnionInfo *item)
{
  vtkParse_AddItemToArray(&info->Items, &info->NumberOfItems,
    item->ItemType, info->NumberOfUnions);
  info->Unions = (UnionInfo **)array_size_check(
    info->Unions, sizeof(UnionInfo *), info->NumberOfUnions);
  info->Unions[info->NumberOfUnions++] = item;
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

/* Add a UnionInfo to a NamespaceInfo */
void vtkParse_AddUnionToNamespace(NamespaceInfo *info, UnionInfo *item)
{
  vtkParse_AddItemToArray(&info->Items, &info->NumberOfItems,
    item->ItemType, info->NumberOfUnions);
  info->Unions = (UnionInfo **)array_size_check(
    info->Unions, sizeof(UnionInfo *), info->NumberOfUnions);
  info->Unions[info->NumberOfUnions++] = item;
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

  vtkParse_InitFile(&data);

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
