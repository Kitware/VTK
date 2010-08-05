
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
                  unsigned int type, const char *typeclass, int flag);
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
     STATIC_CAST = 320,
     DYNAMIC_CAST = 321,
     CONST_CAST = 322,
     REINTERPRET_CAST = 323,
     OP_LSHIFT_EQ = 324,
     OP_RSHIFT_EQ = 325,
     OP_LSHIFT = 326,
     OP_RSHIFT = 327,
     OP_ARROW_POINTER = 328,
     OP_ARROW = 329,
     OP_INCR = 330,
     OP_DECR = 331,
     OP_PLUS_EQ = 332,
     OP_MINUS_EQ = 333,
     OP_TIMES_EQ = 334,
     OP_DIVIDE_EQ = 335,
     OP_REMAINDER_EQ = 336,
     OP_AND_EQ = 337,
     OP_OR_EQ = 338,
     OP_XOR_EQ = 339,
     OP_LOGIC_AND_EQ = 340,
     OP_LOGIC_OR_EQ = 341,
     OP_LOGIC_AND = 342,
     OP_LOGIC_OR = 343,
     OP_LOGIC_EQ = 344,
     OP_LOGIC_NEQ = 345,
     OP_LOGIC_LEQ = 346,
     OP_LOGIC_GEQ = 347,
     ELLIPSIS = 348,
     DOUBLE_COLON = 349,
     LP = 350,
     LA = 351,
     QT_ID = 352,
     StdString = 353,
     UnicodeString = 354,
     IdType = 355,
     TypeInt8 = 356,
     TypeUInt8 = 357,
     TypeInt16 = 358,
     TypeUInt16 = 359,
     TypeInt32 = 360,
     TypeUInt32 = 361,
     TypeInt64 = 362,
     TypeUInt64 = 363,
     TypeFloat32 = 364,
     TypeFloat64 = 365,
     SetMacro = 366,
     GetMacro = 367,
     SetStringMacro = 368,
     GetStringMacro = 369,
     SetClampMacro = 370,
     SetObjectMacro = 371,
     GetObjectMacro = 372,
     BooleanMacro = 373,
     SetVector2Macro = 374,
     SetVector3Macro = 375,
     SetVector4Macro = 376,
     SetVector6Macro = 377,
     GetVector2Macro = 378,
     GetVector3Macro = 379,
     GetVector4Macro = 380,
     GetVector6Macro = 381,
     SetVectorMacro = 382,
     GetVectorMacro = 383,
     ViewportCoordinateMacro = 384,
     WorldCoordinateMacro = 385,
     TypeMacro = 386,
     VTK_BYTE_SWAP_DECL = 387
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
#line 1477 "vtkParse.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 1489 "vtkParse.tab.c"

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
#define YYLAST   6089

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  156
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  191
/* YYNRULES -- Number of rules.  */
#define YYNRULES  559
/* YYNRULES -- Number of states.  */
#define YYNSTATES  978

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   387

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   154,     2,     2,     2,   148,   149,     2,
     134,   135,   146,   144,   141,   143,   155,   147,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   140,   133,
     138,   142,   139,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   152,     2,   153,   151,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   136,   150,   137,   145,     2,     2,     2,
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
     125,   126,   127,   128,   129,   130,   131,   132
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     5,     9,    11,    15,    19,    21,
      23,    25,    27,    31,    36,    38,    41,    45,    48,    51,
      54,    58,    61,    63,    66,    71,    76,    81,    83,    89,
      90,    97,   102,   103,   111,   112,   123,   124,   132,   133,
     144,   149,   150,   151,   155,   159,   161,   165,   169,   171,
     173,   175,   178,   180,   182,   185,   189,   193,   196,   200,
     204,   207,   213,   215,   217,   218,   221,   223,   227,   229,
     232,   235,   238,   240,   242,   244,   245,   252,   253,   259,
     260,   262,   266,   268,   272,   274,   276,   278,   280,   282,
     284,   286,   288,   290,   292,   293,   297,   298,   303,   304,
     309,   311,   313,   315,   317,   319,   321,   323,   325,   327,
     329,   331,   337,   342,   346,   349,   353,   357,   360,   362,
     368,   372,   377,   382,   387,   392,   396,   398,   402,   403,
     409,   411,   412,   417,   420,   423,   424,   428,   430,   432,
     433,   434,   438,   443,   448,   451,   455,   460,   466,   470,
     475,   482,   490,   496,   503,   506,   510,   513,   517,   521,
     523,   526,   529,   532,   536,   538,   541,   544,   548,   552,
     554,   557,   561,   562,   563,   572,   573,   577,   578,   579,
     587,   588,   592,   593,   596,   599,   601,   603,   607,   608,
     614,   615,   616,   626,   627,   631,   632,   638,   639,   643,
     644,   648,   653,   655,   656,   662,   663,   664,   667,   669,
     671,   672,   677,   678,   679,   685,   687,   689,   692,   693,
     695,   696,   700,   705,   710,   714,   717,   718,   721,   722,
     723,   728,   729,   732,   733,   737,   740,   741,   747,   750,
     751,   757,   759,   761,   763,   765,   767,   768,   769,   774,
     776,   778,   781,   783,   786,   787,   789,   791,   792,   794,
     795,   798,   799,   805,   806,   808,   809,   811,   813,   815,
     817,   819,   821,   823,   825,   827,   830,   833,   837,   840,
     843,   847,   849,   852,   854,   857,   859,   862,   865,   867,
     869,   871,   873,   874,   878,   879,   885,   886,   892,   893,
     899,   901,   902,   907,   909,   911,   913,   915,   917,   919,
     921,   923,   925,   929,   933,   935,   937,   939,   941,   943,
     945,   947,   949,   952,   954,   956,   959,   961,   963,   965,
     968,   971,   974,   977,   980,   983,   986,   989,   991,   993,
     995,   997,   999,  1001,  1003,  1005,  1007,  1009,  1011,  1013,
    1015,  1017,  1019,  1021,  1023,  1025,  1027,  1029,  1031,  1033,
    1035,  1037,  1039,  1041,  1043,  1045,  1047,  1049,  1051,  1053,
    1055,  1057,  1059,  1061,  1063,  1065,  1067,  1069,  1071,  1073,
    1075,  1076,  1083,  1084,  1086,  1087,  1088,  1093,  1095,  1096,
    1100,  1101,  1105,  1107,  1108,  1113,  1114,  1115,  1125,  1127,
    1129,  1131,  1133,  1135,  1138,  1140,  1142,  1144,  1146,  1148,
    1150,  1152,  1153,  1161,  1162,  1163,  1164,  1174,  1175,  1181,
    1182,  1188,  1189,  1190,  1201,  1202,  1210,  1211,  1212,  1213,
    1223,  1230,  1231,  1239,  1240,  1248,  1249,  1257,  1258,  1266,
    1267,  1275,  1276,  1284,  1285,  1293,  1294,  1302,  1303,  1313,
    1314,  1324,  1329,  1334,  1342,  1345,  1348,  1352,  1356,  1358,
    1360,  1362,  1364,  1366,  1368,  1370,  1372,  1374,  1376,  1378,
    1380,  1382,  1384,  1386,  1388,  1390,  1392,  1394,  1396,  1398,
    1400,  1402,  1404,  1406,  1408,  1410,  1412,  1414,  1416,  1418,
    1420,  1422,  1424,  1426,  1428,  1430,  1432,  1434,  1436,  1438,
    1439,  1442,  1443,  1446,  1448,  1450,  1452,  1454,  1456,  1458,
    1460,  1462,  1464,  1466,  1468,  1470,  1472,  1474,  1476,  1478,
    1480,  1482,  1484,  1486,  1488,  1490,  1492,  1494,  1496,  1498,
    1500,  1502,  1504,  1506,  1508,  1510,  1512,  1514,  1516,  1518,
    1520,  1522,  1524,  1526,  1528,  1530,  1532,  1534,  1536,  1538,
    1540,  1542,  1544,  1546,  1548,  1550,  1554,  1558,  1562,  1566
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     157,     0,    -1,    -1,    -1,   157,   158,   159,    -1,   250,
      -1,   175,   252,   133,    -1,   188,   252,   133,    -1,   189,
      -1,   161,    -1,   160,    -1,   193,    -1,   163,   252,   133,
      -1,   195,   163,   252,   133,    -1,    41,    -1,   212,   224,
      -1,   195,   212,   224,    -1,   211,   224,    -1,   206,   224,
      -1,   207,   224,    -1,   195,   206,   224,    -1,   204,   224,
      -1,   315,    -1,   293,   133,    -1,     9,   134,   340,   135,
      -1,    57,   134,   340,   135,    -1,    97,   134,   340,   135,
      -1,   133,    -1,    59,    10,   136,   157,   137,    -1,    -1,
      55,   293,   162,   136,   157,   137,    -1,    55,   136,   340,
     137,    -1,    -1,     4,   277,   164,   171,   136,   168,   137,
      -1,    -1,     4,   277,   138,   289,   139,   165,   171,   136,
     168,   137,    -1,    -1,     3,   277,   166,   171,   136,   168,
     137,    -1,    -1,     3,   277,   138,   289,   139,   167,   171,
     136,   168,   137,    -1,     3,   136,   340,   137,    -1,    -1,
      -1,   168,   169,   170,    -1,   168,   174,   140,    -1,   250,
      -1,   175,   252,   133,    -1,   188,   252,   133,    -1,   189,
      -1,   193,    -1,   191,    -1,    49,   191,    -1,   190,    -1,
      41,    -1,   212,   224,    -1,    49,   212,   224,    -1,   195,
     212,   224,    -1,   209,   224,    -1,    49,   209,   224,    -1,
     195,   209,   224,    -1,   205,   224,    -1,   132,   134,   340,
     135,   133,    -1,   315,    -1,   133,    -1,    -1,   140,   172,
      -1,   173,    -1,   173,   141,   172,    -1,   291,    -1,     6,
     291,    -1,     7,   291,    -1,     5,   291,    -1,     5,    -1,
       6,    -1,     7,    -1,    -1,    39,   277,   176,   136,   178,
     137,    -1,    -1,    39,   177,   136,   178,   137,    -1,    -1,
     179,    -1,   179,   141,   178,    -1,   277,    -1,   277,   142,
     182,    -1,   181,    -1,   277,    -1,   292,    -1,   285,    -1,
      16,    -1,    11,    -1,    13,    -1,    12,    -1,    15,    -1,
     180,    -1,    -1,   186,   183,   182,    -1,    -1,   180,   187,
     184,   182,    -1,    -1,   134,   185,   182,   135,    -1,   143,
      -1,   144,    -1,   145,    -1,   143,    -1,   144,    -1,   146,
      -1,   147,    -1,   148,    -1,   149,    -1,   150,    -1,   151,
      -1,    40,   277,   136,   340,   137,    -1,    40,   136,   340,
     137,    -1,    56,   341,   133,    -1,   195,   191,    -1,     4,
     277,   192,    -1,     3,   277,   192,    -1,     3,   192,    -1,
     133,    -1,   136,   340,   137,   341,   133,    -1,   140,   341,
     133,    -1,   194,   280,   260,   133,    -1,   194,   163,   246,
     133,    -1,   194,   175,   246,   133,    -1,   194,   188,   246,
     133,    -1,   194,    60,   133,    -1,    54,    -1,    52,   138,
     139,    -1,    -1,    52,   138,   196,   197,   139,    -1,   199,
      -1,    -1,   199,   141,   198,   197,    -1,   298,   202,    -1,
     201,   202,    -1,    -1,   200,   195,   202,    -1,     4,    -1,
      53,    -1,    -1,    -1,   277,   203,   247,    -1,    61,   134,
     206,   135,    -1,    61,   134,   209,   135,    -1,   278,   221,
      -1,   278,   208,   221,    -1,   293,    94,   145,   236,    -1,
      50,   293,    94,   145,   236,    -1,   293,    94,   229,    -1,
      50,   293,    94,   229,    -1,   293,    94,   293,    94,   145,
     236,    -1,    50,   293,    94,   293,    94,   145,   236,    -1,
     293,    94,   293,    94,   229,    -1,    50,   293,    94,   293,
      94,   229,    -1,   293,    94,    -1,   208,   293,    94,    -1,
     145,   236,    -1,    50,   145,   236,    -1,     8,   145,   236,
      -1,   229,    -1,    50,   229,    -1,   210,   229,    -1,   278,
     221,    -1,     8,   278,   221,    -1,    64,    -1,    50,    64,
      -1,    64,    50,    -1,   293,    94,   213,    -1,   278,   208,
     216,    -1,   213,    -1,   278,   216,    -1,     8,   280,   216,
      -1,    -1,    -1,    46,   278,   134,   214,   239,   135,   215,
     223,    -1,    -1,   218,   217,   223,    -1,    -1,    -1,    46,
     338,   219,   134,   220,   239,   135,    -1,    -1,   225,   222,
     223,    -1,    -1,   142,    16,    -1,    45,    16,    -1,    43,
      -1,   133,    -1,   136,   340,   137,    -1,    -1,   277,   134,
     226,   239,   135,    -1,    -1,    -1,   277,   138,   227,   289,
     139,   134,   228,   239,   135,    -1,    -1,   231,   230,   233,
      -1,    -1,   277,   134,   232,   239,   135,    -1,    -1,   140,
     235,   234,    -1,    -1,   141,   235,   234,    -1,   277,   134,
     340,   135,    -1,   237,    -1,    -1,   277,   134,   238,   239,
     135,    -1,    -1,    -1,   240,   241,    -1,    93,    -1,   243,
      -1,    -1,   243,   141,   242,   241,    -1,    -1,    -1,   244,
     280,   258,   245,   247,    -1,    60,    -1,   277,    -1,   294,
     277,    -1,    -1,   248,    -1,    -1,   142,   249,   301,    -1,
     278,   251,   253,   133,    -1,    58,    60,   253,   133,    -1,
      60,   253,   133,    -1,   260,   247,    -1,    -1,   255,   253,
      -1,    -1,    -1,   253,   141,   254,   255,    -1,    -1,   256,
     251,    -1,    -1,   294,   257,   251,    -1,   268,   270,    -1,
      -1,   262,   266,   135,   259,   264,    -1,   269,   270,    -1,
      -1,   263,   267,   135,   261,   264,    -1,   134,    -1,    95,
      -1,    96,    -1,    95,    -1,    96,    -1,    -1,    -1,   134,
     265,   239,   135,    -1,   271,    -1,   258,    -1,   294,   258,
      -1,   260,    -1,   294,   260,    -1,    -1,   269,    -1,   277,
      -1,    -1,   271,    -1,    -1,   272,   273,    -1,    -1,   275,
     152,   274,   276,   153,    -1,    -1,   273,    -1,    -1,   182,
      -1,    57,    -1,    97,    -1,     9,    -1,    38,    -1,    37,
      -1,    98,    -1,    99,    -1,   280,    -1,    51,   280,    -1,
      59,   280,    -1,    59,    10,   280,    -1,    50,   280,    -1,
     279,   280,    -1,    50,   279,   280,    -1,    58,    -1,    58,
      50,    -1,   281,    -1,   281,   294,    -1,   283,    -1,   282,
     283,    -1,   283,   282,    -1,    43,    -1,   297,    -1,   285,
      -1,   292,    -1,    -1,    53,   284,   291,    -1,    -1,    57,
     138,   286,   289,   139,    -1,    -1,     9,   138,   287,   289,
     139,    -1,    -1,    97,   138,   288,   289,   139,    -1,   280,
      -1,    -1,   280,   141,   290,   289,    -1,    57,    -1,     9,
      -1,    97,    -1,    38,    -1,    37,    -1,    98,    -1,    99,
      -1,   285,    -1,   292,    -1,   293,    94,   291,    -1,   285,
      94,   291,    -1,     9,    -1,    97,    -1,    57,    -1,    38,
      -1,    37,    -1,    98,    -1,    99,    -1,   149,    -1,   295,
     149,    -1,   295,    -1,   296,    -1,   295,   296,    -1,   146,
      -1,    44,    -1,   298,    -1,     4,   299,    -1,     3,   299,
      -1,    40,     9,    -1,    40,    57,    -1,    40,    97,    -1,
      39,     9,    -1,    39,    57,    -1,    39,    97,    -1,   300,
      -1,   299,    -1,    98,    -1,    99,    -1,    37,    -1,    38,
      -1,     9,    -1,    57,    -1,    97,    -1,    33,    -1,    34,
      -1,    35,    -1,    36,    -1,   101,    -1,   102,    -1,   103,
      -1,   104,    -1,   105,    -1,   106,    -1,   107,    -1,   108,
      -1,   109,    -1,   110,    -1,   100,    -1,    17,    -1,    18,
      -1,    19,    -1,    30,    -1,    31,    -1,    32,    -1,    20,
      -1,    21,    -1,    22,    -1,    23,    -1,    24,    -1,    25,
      -1,    26,    -1,    27,    -1,    28,    -1,    29,    -1,    48,
      -1,    47,    -1,   306,    -1,    -1,   136,   302,   301,   304,
     303,   137,    -1,    -1,   141,    -1,    -1,    -1,   304,   141,
     305,   301,    -1,   314,    -1,    -1,   144,   307,   314,    -1,
      -1,   143,   308,   314,    -1,   313,    -1,    -1,   134,   309,
     306,   135,    -1,    -1,    -1,   312,   138,   310,   281,   139,
     134,   311,   314,   135,    -1,    65,    -1,    67,    -1,    66,
      -1,    68,    -1,    10,    -1,   313,    10,    -1,    16,    -1,
      11,    -1,    13,    -1,    12,    -1,    14,    -1,    15,    -1,
     291,    -1,    -1,   111,   134,   277,   141,   316,   280,   135,
      -1,    -1,    -1,    -1,   112,   134,   317,   277,   141,   318,
     280,   319,   135,    -1,    -1,   113,   134,   320,   277,   135,
      -1,    -1,   114,   134,   321,   277,   135,    -1,    -1,    -1,
     115,   134,   277,   141,   322,   297,   323,   141,   341,   135,
      -1,    -1,   116,   134,   277,   141,   324,   297,   135,    -1,
      -1,    -1,    -1,   117,   134,   325,   277,   141,   326,   297,
     327,   135,    -1,   118,   134,   277,   141,   297,   135,    -1,
      -1,   119,   134,   277,   141,   328,   297,   135,    -1,    -1,
     123,   134,   277,   141,   329,   297,   135,    -1,    -1,   120,
     134,   277,   141,   330,   297,   135,    -1,    -1,   124,   134,
     277,   141,   331,   297,   135,    -1,    -1,   121,   134,   277,
     141,   332,   297,   135,    -1,    -1,   125,   134,   277,   141,
     333,   297,   135,    -1,    -1,   122,   134,   277,   141,   334,
     297,   135,    -1,    -1,   126,   134,   277,   141,   335,   297,
     135,    -1,    -1,   127,   134,   277,   141,   336,   297,   141,
      11,   135,    -1,    -1,   128,   134,   277,   141,   337,   297,
     141,    11,   135,    -1,   129,   134,   277,   135,    -1,   130,
     134,   277,   135,    -1,   131,   134,   277,   141,   277,   303,
     135,    -1,   134,   135,    -1,   152,   153,    -1,    62,   152,
     153,    -1,    63,   152,   153,    -1,   339,    -1,   142,    -1,
     146,    -1,   147,    -1,   143,    -1,   144,    -1,   154,    -1,
     145,    -1,   141,    -1,   138,    -1,   139,    -1,   149,    -1,
     150,    -1,   151,    -1,   148,    -1,    62,    -1,    63,    -1,
      69,    -1,    70,    -1,    71,    -1,    72,    -1,    73,    -1,
      74,    -1,    77,    -1,    78,    -1,    79,    -1,    80,    -1,
      81,    -1,    75,    -1,    76,    -1,    82,    -1,    83,    -1,
      84,    -1,    85,    -1,    86,    -1,    87,    -1,    88,    -1,
      89,    -1,    90,    -1,    91,    -1,    92,    -1,    -1,   340,
     342,    -1,    -1,   341,   343,    -1,   133,    -1,   343,    -1,
      42,    -1,   344,    -1,   346,    -1,   345,    -1,    54,    -1,
     339,    -1,   140,    -1,   155,    -1,    94,    -1,     4,    -1,
      52,    -1,    38,    -1,    37,    -1,    98,    -1,    99,    -1,
     300,    -1,    13,    -1,    11,    -1,    12,    -1,    14,    -1,
      15,    -1,    10,    -1,    41,    -1,    43,    -1,    44,    -1,
      45,    -1,     3,    -1,    46,    -1,    58,    -1,    50,    -1,
       8,    -1,    39,    -1,    40,    -1,    53,    -1,    16,    -1,
      60,    -1,    93,    -1,     5,    -1,     7,    -1,     6,    -1,
      55,    -1,    56,    -1,    59,    -1,     9,    -1,    57,    -1,
      97,    -1,    67,    -1,    66,    -1,    65,    -1,    68,    -1,
     136,   340,   137,    -1,   152,   340,   153,    -1,   134,   340,
     135,    -1,    95,   340,   135,    -1,    96,   340,   135,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,  1266,  1266,  1267,  1266,  1271,  1272,  1273,  1274,  1275,
    1276,  1277,  1278,  1279,  1280,  1281,  1282,  1283,  1284,  1285,
    1286,  1287,  1288,  1289,  1290,  1291,  1292,  1293,  1299,  1305,
    1305,  1307,  1313,  1313,  1315,  1315,  1317,  1317,  1319,  1319,
    1321,  1324,  1326,  1325,  1328,  1331,  1332,  1333,  1334,  1335,
    1336,  1337,  1338,  1339,  1340,  1341,  1343,  1344,  1345,  1347,
    1348,  1349,  1350,  1351,  1353,  1353,  1355,  1355,  1357,  1358,
    1359,  1360,  1367,  1368,  1369,  1379,  1379,  1381,  1381,  1384,
    1384,  1384,  1386,  1387,  1389,  1390,  1391,  1391,  1393,  1393,
    1393,  1393,  1393,  1395,  1396,  1396,  1400,  1400,  1404,  1404,
    1409,  1409,  1410,  1412,  1412,  1413,  1413,  1414,  1414,  1415,
    1415,  1421,  1422,  1424,  1426,  1428,  1429,  1430,  1432,  1433,
    1434,  1440,  1463,  1464,  1465,  1466,  1468,  1474,  1475,  1475,
    1481,  1482,  1482,  1485,  1495,  1503,  1503,  1515,  1516,  1518,
    1518,  1518,  1525,  1527,  1533,  1535,  1536,  1537,  1538,  1539,
    1540,  1541,  1542,  1543,  1545,  1546,  1548,  1549,  1550,  1555,
    1556,  1557,  1564,  1565,  1573,  1573,  1573,  1575,  1576,  1579,
    1580,  1581,  1591,  1595,  1590,  1607,  1607,  1616,  1617,  1616,
    1624,  1624,  1633,  1634,  1643,  1653,  1659,  1659,  1662,  1661,
    1666,  1667,  1666,  1674,  1674,  1681,  1681,  1683,  1683,  1685,
    1685,  1687,  1689,  1698,  1698,  1704,  1704,  1704,  1707,  1708,
    1709,  1709,  1712,  1714,  1712,  1743,  1767,  1767,  1769,  1769,
    1771,  1771,  1778,  1779,  1780,  1782,  1793,  1794,  1796,  1797,
    1797,  1800,  1800,  1801,  1801,  1805,  1806,  1806,  1817,  1818,
    1818,  1828,  1829,  1831,  1834,  1836,  1839,  1840,  1840,  1842,
    1845,  1846,  1850,  1851,  1854,  1854,  1856,  1858,  1858,  1860,
    1860,  1862,  1862,  1864,  1864,  1866,  1867,  1873,  1874,  1875,
    1876,  1877,  1878,  1879,  1886,  1887,  1888,  1889,  1891,  1892,
    1894,  1898,  1899,  1901,  1902,  1904,  1905,  1906,  1908,  1910,
    1911,  1913,  1915,  1915,  1919,  1919,  1922,  1922,  1925,  1925,
    1929,  1929,  1929,  1931,  1932,  1933,  1934,  1935,  1936,  1937,
    1938,  1939,  1941,  1946,  1952,  1952,  1952,  1952,  1952,  1952,
    1952,  1968,  1969,  1970,  1975,  1976,  1988,  1989,  1992,  1993,
    1994,  1995,  1996,  1997,  1998,  1999,  2000,  2003,  2004,  2007,
    2008,  2009,  2010,  2011,  2012,  2013,  2016,  2017,  2018,  2019,
    2020,  2021,  2022,  2023,  2024,  2025,  2026,  2027,  2028,  2029,
    2030,  2031,  2032,  2033,  2034,  2035,  2036,  2038,  2039,  2041,
    2042,  2044,  2045,  2047,  2048,  2050,  2051,  2053,  2054,  2060,
    2061,  2061,  2067,  2067,  2069,  2070,  2070,  2075,  2076,  2076,
    2077,  2077,  2081,  2082,  2082,  2083,  2085,  2083,  2105,  2106,
    2107,  2108,  2110,  2111,  2114,  2115,  2116,  2117,  2118,  2119,
    2120,  2131,  2131,  2140,  2141,  2141,  2140,  2149,  2149,  2158,
    2158,  2166,  2166,  2166,  2196,  2195,  2205,  2206,  2206,  2205,
    2214,  2230,  2230,  2235,  2235,  2240,  2240,  2245,  2245,  2250,
    2250,  2255,  2255,  2260,  2260,  2265,  2265,  2270,  2270,  2286,
    2286,  2299,  2332,  2366,  2419,  2420,  2421,  2422,  2423,  2425,
    2426,  2426,  2427,  2427,  2428,  2428,  2429,  2429,  2430,  2430,
    2431,  2431,  2432,  2433,  2434,  2435,  2436,  2437,  2438,  2439,
    2440,  2441,  2442,  2443,  2444,  2445,  2446,  2447,  2448,  2449,
    2450,  2451,  2452,  2453,  2454,  2455,  2456,  2457,  2458,  2464,
    2464,  2465,  2465,  2467,  2467,  2469,  2469,  2469,  2469,  2469,
    2470,  2470,  2470,  2470,  2470,  2470,  2471,  2471,  2471,  2471,
    2471,  2472,  2472,  2472,  2472,  2472,  2473,  2473,  2473,  2473,
    2473,  2473,  2474,  2474,  2474,  2474,  2474,  2474,  2474,  2475,
    2475,  2475,  2475,  2475,  2475,  2476,  2476,  2476,  2476,  2476,
    2476,  2477,  2477,  2477,  2477,  2479,  2480,  2481,  2481,  2481
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
  "NEW", "DELETE", "EXPLICIT", "STATIC_CAST", "DYNAMIC_CAST", "CONST_CAST",
  "REINTERPRET_CAST", "OP_LSHIFT_EQ", "OP_RSHIFT_EQ", "OP_LSHIFT",
  "OP_RSHIFT", "OP_ARROW_POINTER", "OP_ARROW", "OP_INCR", "OP_DECR",
  "OP_PLUS_EQ", "OP_MINUS_EQ", "OP_TIMES_EQ", "OP_DIVIDE_EQ",
  "OP_REMAINDER_EQ", "OP_AND_EQ", "OP_OR_EQ", "OP_XOR_EQ",
  "OP_LOGIC_AND_EQ", "OP_LOGIC_OR_EQ", "OP_LOGIC_AND", "OP_LOGIC_OR",
  "OP_LOGIC_EQ", "OP_LOGIC_NEQ", "OP_LOGIC_LEQ", "OP_LOGIC_GEQ",
  "ELLIPSIS", "DOUBLE_COLON", "LP", "LA", "QT_ID", "StdString",
  "UnicodeString", "IdType", "TypeInt8", "TypeUInt8", "TypeInt16",
  "TypeUInt16", "TypeInt32", "TypeUInt32", "TypeInt64", "TypeUInt64",
  "TypeFloat32", "TypeFloat64", "SetMacro", "GetMacro", "SetStringMacro",
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
  "$@43", "$@44", "$@45", "types", "$@46", "maybe_scoped_id", "scoped_id",
  "class_id", "type_indirection", "pointers", "pointer_or_const_pointer",
  "type_red2", "type_simple", "type_id", "type_primitive", "value", "$@47",
  "maybe_comma", "more_values", "$@48", "literal", "$@49", "$@50", "$@51",
  "$@52", "$@53", "any_cast", "string_literal", "literal2", "macro",
  "$@54", "$@55", "$@56", "$@57", "$@58", "$@59", "$@60", "$@61", "$@62",
  "$@63", "$@64", "$@65", "$@66", "$@67", "$@68", "$@69", "$@70", "$@71",
  "$@72", "$@73", "$@74", "$@75", "op_token", "op_token_no_delim",
  "maybe_other", "maybe_other_no_semi", "other_stuff",
  "other_stuff_no_semi", "braces", "brackets", "parens", 0
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
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,    59,    40,    41,   123,   125,    60,    62,
      58,    44,    61,    45,    43,   126,    42,    47,    37,    38,
     124,    94,    91,    93,    33,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   156,   157,   158,   157,   159,   159,   159,   159,   159,
     159,   159,   159,   159,   159,   159,   159,   159,   159,   159,
     159,   159,   159,   159,   159,   159,   159,   159,   160,   162,
     161,   161,   164,   163,   165,   163,   166,   163,   167,   163,
     163,   168,   169,   168,   168,   170,   170,   170,   170,   170,
     170,   170,   170,   170,   170,   170,   170,   170,   170,   170,
     170,   170,   170,   170,   171,   171,   172,   172,   173,   173,
     173,   173,   174,   174,   174,   176,   175,   177,   175,   178,
     178,   178,   179,   179,   180,   180,   180,   180,   181,   181,
     181,   181,   181,   182,   183,   182,   184,   182,   185,   182,
     186,   186,   186,   187,   187,   187,   187,   187,   187,   187,
     187,   188,   188,   189,   190,   191,   191,   191,   192,   192,
     192,   193,   193,   193,   193,   193,   194,   195,   196,   195,
     197,   198,   197,   199,   199,   200,   199,   201,   201,   202,
     203,   202,   204,   205,   206,   207,   207,   207,   207,   207,
     207,   207,   207,   207,   208,   208,   209,   209,   209,   209,
     209,   209,   209,   209,   210,   210,   210,   211,   211,   212,
     212,   212,   214,   215,   213,   217,   216,   219,   220,   218,
     222,   221,   223,   223,   223,   223,   224,   224,   226,   225,
     227,   228,   225,   230,   229,   232,   231,   233,   233,   234,
     234,   235,   236,   238,   237,   239,   240,   239,   241,   241,
     242,   241,   244,   245,   243,   243,   246,   246,   247,   247,
     249,   248,   250,   250,   250,   251,   252,   252,   253,   254,
     253,   256,   255,   257,   255,   258,   259,   258,   260,   261,
     260,   262,   262,   262,   263,   263,   264,   265,   264,   264,
     266,   266,   267,   267,   268,   268,   269,   270,   270,   272,
     271,   274,   273,   275,   275,   276,   276,   277,   277,   277,
     277,   277,   277,   277,   278,   278,   278,   278,   278,   278,
     278,   279,   279,   280,   280,   281,   281,   281,   282,   283,
     283,   283,   284,   283,   286,   285,   287,   285,   288,   285,
     289,   290,   289,   291,   291,   291,   291,   291,   291,   291,
     291,   291,   292,   292,   293,   293,   293,   293,   293,   293,
     293,   294,   294,   294,   295,   295,   296,   296,   297,   297,
     297,   297,   297,   297,   297,   297,   297,   298,   298,   299,
     299,   299,   299,   299,   299,   299,   300,   300,   300,   300,
     300,   300,   300,   300,   300,   300,   300,   300,   300,   300,
     300,   300,   300,   300,   300,   300,   300,   300,   300,   300,
     300,   300,   300,   300,   300,   300,   300,   300,   300,   301,
     302,   301,   303,   303,   304,   305,   304,   306,   307,   306,
     308,   306,   306,   309,   306,   310,   311,   306,   312,   312,
     312,   312,   313,   313,   314,   314,   314,   314,   314,   314,
     314,   316,   315,   317,   318,   319,   315,   320,   315,   321,
     315,   322,   323,   315,   324,   315,   325,   326,   327,   315,
     315,   328,   315,   329,   315,   330,   315,   331,   315,   332,
     315,   333,   315,   334,   315,   335,   315,   336,   315,   337,
     315,   315,   315,   315,   338,   338,   338,   338,   338,   339,
     339,   339,   339,   339,   339,   339,   339,   339,   339,   339,
     339,   339,   339,   339,   339,   339,   339,   339,   339,   339,
     339,   339,   339,   339,   339,   339,   339,   339,   339,   339,
     339,   339,   339,   339,   339,   339,   339,   339,   339,   340,
     340,   341,   341,   342,   342,   343,   343,   343,   343,   343,
     343,   343,   343,   343,   343,   343,   343,   343,   343,   343,
     343,   343,   343,   343,   343,   343,   343,   343,   343,   343,
     343,   343,   343,   343,   343,   343,   343,   343,   343,   343,
     343,   343,   343,   343,   343,   343,   343,   343,   343,   343,
     343,   343,   343,   343,   343,   344,   345,   346,   346,   346
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     0,     3,     1,     3,     3,     1,     1,
       1,     1,     3,     4,     1,     2,     3,     2,     2,     2,
       3,     2,     1,     2,     4,     4,     4,     1,     5,     0,
       6,     4,     0,     7,     0,    10,     0,     7,     0,    10,
       4,     0,     0,     3,     3,     1,     3,     3,     1,     1,
       1,     2,     1,     1,     2,     3,     3,     2,     3,     3,
       2,     5,     1,     1,     0,     2,     1,     3,     1,     2,
       2,     2,     1,     1,     1,     0,     6,     0,     5,     0,
       1,     3,     1,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     0,     3,     0,     4,     0,     4,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     5,     4,     3,     2,     3,     3,     2,     1,     5,
       3,     4,     4,     4,     4,     3,     1,     3,     0,     5,
       1,     0,     4,     2,     2,     0,     3,     1,     1,     0,
       0,     3,     4,     4,     2,     3,     4,     5,     3,     4,
       6,     7,     5,     6,     2,     3,     2,     3,     3,     1,
       2,     2,     2,     3,     1,     2,     2,     3,     3,     1,
       2,     3,     0,     0,     8,     0,     3,     0,     0,     7,
       0,     3,     0,     2,     2,     1,     1,     3,     0,     5,
       0,     0,     9,     0,     3,     0,     5,     0,     3,     0,
       3,     4,     1,     0,     5,     0,     0,     2,     1,     1,
       0,     4,     0,     0,     5,     1,     1,     2,     0,     1,
       0,     3,     4,     4,     3,     2,     0,     2,     0,     0,
       4,     0,     2,     0,     3,     2,     0,     5,     2,     0,
       5,     1,     1,     1,     1,     1,     0,     0,     4,     1,
       1,     2,     1,     2,     0,     1,     1,     0,     1,     0,
       2,     0,     5,     0,     1,     0,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     2,     3,     2,     2,
       3,     1,     2,     1,     2,     1,     2,     2,     1,     1,
       1,     1,     0,     3,     0,     5,     0,     5,     0,     5,
       1,     0,     4,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     1,     1,     2,     1,     1,     1,     2,
       2,     2,     2,     2,     2,     2,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       0,     6,     0,     1,     0,     0,     4,     1,     0,     3,
       0,     3,     1,     0,     4,     0,     0,     9,     1,     1,
       1,     1,     1,     2,     1,     1,     1,     1,     1,     1,
       1,     0,     7,     0,     0,     0,     9,     0,     5,     0,
       5,     0,     0,    10,     0,     7,     0,     0,     0,     9,
       6,     0,     7,     0,     7,     0,     7,     0,     7,     0,
       7,     0,     7,     0,     7,     0,     7,     0,     9,     0,
       9,     4,     4,     7,     2,     2,     3,     3,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     0,
       2,     0,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     3,     3,     3,     3,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       2,     3,     1,     0,     0,     0,     0,   343,   361,   362,
     363,   367,   368,   369,   370,   371,   372,   373,   374,   375,
     376,   364,   365,   366,   346,   347,   348,   349,   341,   342,
      77,     0,    14,   288,     0,   378,   377,     0,     0,     0,
     292,   126,     0,   501,   344,   281,     0,   228,     0,   345,
     339,   340,   360,   350,   351,   352,   353,   354,   355,   356,
     357,   358,   359,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    27,     4,    10,     9,   231,   231,
     231,     8,    11,     0,     0,     0,     0,     0,     0,     0,
     169,     5,     0,     0,   274,   283,     0,   285,   290,   291,
       0,   289,   328,   338,   337,    22,   343,   341,   342,   344,
     345,   339,   340,   499,    36,   330,    32,   329,     0,     0,
     343,     0,     0,   344,   345,     0,     0,   499,   296,   334,
     271,   270,   335,   336,   272,   273,     0,    75,   331,   332,
     333,   499,     0,     0,   281,     0,     0,     0,   278,     0,
     275,   128,     0,   314,   318,   317,   316,   315,   319,   320,
     499,    29,     0,   499,   294,   282,   228,     0,   276,     0,
       0,   499,   298,     0,   413,   417,   419,     0,     0,   426,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   327,   326,   321,     0,   228,     0,
     233,   323,   324,     0,     0,     0,     0,     0,     0,     0,
     231,     0,     0,     0,   186,   499,    21,    18,    19,    17,
      15,   269,   271,   270,     0,   267,   244,   245,   268,   272,
     273,     0,   170,   175,   144,   180,   228,   218,     0,   257,
     256,     0,   279,   284,   286,   287,     0,     0,    23,     0,
       0,    64,     0,    64,   343,   341,   342,   344,   345,   339,
     340,   334,   335,   336,   331,   332,   333,   171,     0,     0,
       0,    79,     0,     0,   499,     0,   172,   280,     0,   127,
     135,   304,   307,   306,   303,   305,   308,   309,   310,   293,
     311,     0,     0,   531,   514,   542,   544,   543,   535,   548,
     526,   522,   523,   521,   524,   525,   539,   517,   516,   536,
     537,   527,   505,   528,   529,   530,   532,   534,   515,   538,
     509,   545,   546,   549,   533,   547,   540,   473,   474,   553,
     552,   551,   554,   475,   476,   477,   478,   479,   480,   486,
     487,   481,   482,   483,   484,   485,   488,   489,   490,   491,
     492,   493,   494,   495,   496,   497,   498,   541,   513,   499,
     499,   550,   518,   519,   113,   499,   499,   467,   468,   511,
     466,   459,   462,   463,   465,   460,   461,   472,   469,   470,
     471,   499,   464,   512,   520,   510,   502,   506,   508,   507,
       0,     0,     0,     2,   277,   224,   229,     0,     0,     0,
       0,   269,   267,   268,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    12,   227,   232,   256,     0,
     322,   325,     6,     7,   125,     0,   216,     0,     0,     0,
       0,     0,    20,    16,     0,     0,   473,   474,     0,     0,
     177,   458,   168,   145,     0,   182,   182,     0,   220,   225,
     219,   252,     0,     0,   238,   258,   263,   188,   190,   154,
     313,   304,   307,   306,   303,   305,   308,   309,     0,   167,
     148,   193,     0,   312,     0,   503,    40,   500,   504,   300,
       0,     0,     0,     0,     0,    24,     0,     0,    80,    82,
      79,   112,     0,   206,     0,   149,     0,   137,   138,     0,
     130,     0,   139,   139,    31,     2,     0,     0,     0,     0,
       0,    25,     0,   223,     3,   231,   142,    26,     0,   411,
       0,     0,     0,   421,   424,     0,     0,   431,   435,   439,
     443,   433,   437,   441,   445,   447,   449,   451,   452,     0,
     234,   122,   217,   123,   124,   121,    13,   187,     0,     0,
     454,   455,     0,   155,   185,     0,     0,   176,   181,   222,
       0,   239,   253,   260,     0,   206,     0,   146,   202,     0,
     197,   195,     0,   301,    38,     0,     0,     0,    65,    66,
      68,    41,    34,    41,   297,    78,    79,     0,     0,   111,
       0,   212,   147,     0,   129,   131,   139,   134,   140,   133,
       3,   558,   559,   557,   555,   556,   295,    28,   230,   299,
       0,   414,   418,   420,     0,     0,   427,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   382,   456,
     457,   178,   184,   183,   402,   405,   407,   406,   408,   409,
     404,   398,   400,   399,   401,   393,   380,   390,   388,   410,
     221,   379,     0,   392,   387,   246,   261,     0,     0,   203,
       0,   194,   206,     0,   152,     0,    64,    71,    69,    70,
       0,    42,    64,    42,    81,   269,    89,    91,    90,    92,
      88,   267,   268,    98,   100,   101,   102,    93,    84,    83,
      94,    85,    87,    86,    76,   173,   215,   208,   207,   209,
       0,     0,   153,   135,   136,   218,    30,     0,     0,   422,
       0,     0,   430,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   383,     0,   206,     0,     0,     0,     0,
     395,   403,   247,   240,   249,   265,   189,     0,   206,   199,
       0,     0,   150,   302,     0,    67,    72,    73,    74,    37,
       0,     0,     0,    33,     0,   103,   104,   105,   106,   107,
     108,   109,   110,    96,     0,   182,   210,   254,   151,   132,
     141,   412,   415,     0,   425,   428,   432,   436,   440,   444,
     434,   438,   442,   446,     0,     0,   453,     0,     0,   384,
     391,   389,     0,   206,   266,     0,   191,     0,     0,   198,
     499,   196,    41,     0,     0,     0,   343,   341,   342,    53,
       0,     0,   344,     0,   164,   345,   339,   340,     0,    63,
       0,    43,   231,   231,    48,    52,    50,    49,     0,     0,
       0,     0,     0,   159,    45,     0,    62,    44,    41,     0,
       0,    95,   174,   212,   242,   243,   241,   213,   254,   257,
     255,     0,   501,     0,     0,     0,   179,   394,   382,     0,
       0,   262,   206,   204,   199,     0,    42,   118,   499,   501,
     117,     0,     0,     0,     0,   274,    51,     0,     0,     0,
     165,     0,   160,     0,   166,   499,   156,     0,     0,   114,
       0,     0,    60,    57,   161,    54,   162,    42,    99,    97,
     211,   218,   250,     0,   254,   235,   416,     0,   429,   448,
     450,   385,     0,     0,   248,     0,   200,   201,    39,     0,
       0,   116,   115,   158,   163,    58,    55,   157,     0,     0,
       0,     0,    46,    47,    59,    56,    35,   214,   236,   251,
     423,     0,   381,   396,   192,   501,   120,   143,     0,   246,
     386,     0,     0,    61,   237,     0,   119,   397
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     3,    85,    86,    87,   302,    88,   263,   692,
     261,   686,   691,   770,   841,   502,   598,   599,   771,    89,
     282,   146,   507,   508,   707,   708,   709,   784,   860,   774,
     710,   783,    90,    91,   845,   846,   890,    92,    93,    94,
     290,   519,   723,   520,   521,   522,   617,   725,    95,   849,
      96,    97,   241,   850,   851,    98,    99,   100,   513,   785,
     242,   465,   243,   572,   745,   244,   466,   577,   226,   245,
     585,   586,   882,   853,   590,   491,   682,   681,   819,   759,
     587,   588,   758,   610,   611,   718,   863,   719,   720,   921,
     445,   469,   470,   580,   101,   246,   207,   179,   535,   208,
     209,   439,   867,   969,   247,   675,   868,   248,   753,   813,
     923,   472,   869,   249,   474,   475,   476,   583,   755,   584,
     815,   492,   894,   103,   104,   105,   106,   107,   162,   108,
     401,   280,   410,   500,   685,   669,   109,   136,   210,   211,
     212,   111,   112,   113,   114,   670,   747,   744,   878,   961,
     671,   749,   748,   746,   812,   971,   672,   673,   674,   115,
     630,   415,   728,   871,   416,   417,   634,   793,   635,   420,
     731,   873,   638,   642,   639,   643,   640,   644,   641,   645,
     646,   647,   460,   395,   259,   172,   497,   498,   397,   398,
     399
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -809
static const yytype_int16 yypact[] =
{
    -809,    60,  -809,  4844,   550,   820,  5623,   -19,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,    26,    49,
     845,   600,  -809,  -809,  5083,  -809,  -809,  5191,  5623,   -72,
    -809,  -809,   617,  -809,   199,    37,  5299,  -809,   -46,   201,
     145,   148,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,   -17,    36,    46,    90,    95,   121,   146,
     151,   165,   177,   179,   190,   196,   204,   207,   209,   218,
     219,   226,   230,   233,  -809,  -809,  -809,  -809,    14,    14,
      14,  -809,  -809,  5407,  4975,   -26,   -26,   -26,   -26,   -26,
    -809,  -809,   626,  5623,  -809,    18,  5731,   327,   283,  -809,
     156,  -809,  -809,  -809,  -809,  -809,   120,   223,   253,   259,
     265,   282,   325,  -809,   -30,  -809,   238,  -809,  4167,  4167,
     -20,    48,    56,   -15,    80,   332,   286,  -809,  -809,   245,
    -809,  -809,   251,   252,  -809,  -809,   254,  -809,   245,   251,
     252,  -809,   260,  5191,   352,  5515,   276,  5623,  -809,   320,
    -809,   278,  4221,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  3392,  -809,  -809,  -809,  -809,  4724,  -809,    92,
    5083,  -809,  -809,  4296,  -809,  -809,  -809,  4296,  4296,  -809,
    4296,  4296,  4296,  4296,  4296,  4296,  4296,  4296,  4296,  4296,
    4296,  4296,  4296,  4296,  -809,  -809,  -809,   290,  -809,   651,
    -809,    19,  -809,   292,   293,   295,   166,   166,   166,   651,
      14,   -26,   -26,   309,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,   337,   338,   339,  5935,   340,  -809,  -809,   343,   348,
     350,   824,  -809,  -809,  -809,  -809,  -809,   277,   429,   294,
     114,   353,  -809,  -809,  -809,  -809,  4221,   285,  -809,   944,
    5623,   310,  5623,   310,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  4221,  1097,
    5623,  4296,   313,  1250,  -809,  5623,  -809,  -809,   450,  -809,
    5946,   -20,   338,   339,   -15,    80,   348,   350,   283,  -809,
    -809,  1403,   324,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    1556,  5623,    93,  -809,  -809,  -809,  -809,   333,  4296,  1709,
    5623,  -809,  -809,  -809,   329,  4296,  4296,  4296,   330,   336,
    4296,   341,   344,   349,   351,   354,   355,   357,   358,   361,
     362,   363,   359,   370,   367,  -809,   368,  -809,  -809,   651,
    -809,  -809,  -809,  -809,  -809,   345,  -809,  4296,   356,   360,
     388,   389,  -809,  -809,   114,  1862,   322,   371,   394,   377,
    -809,  -809,  -809,  -809,   438,    16,    16,    94,  -809,  -809,
    -809,  -809,   398,   651,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,   136,    83,   134,   182,   202,   137,   149,  4296,  -809,
    -809,  -809,   401,  -809,   443,  -809,  -809,  -809,  -809,   397,
     403,   749,   407,   405,   409,  -809,   411,   414,   412,   404,
    4296,  -809,  2015,   419,  4296,  -809,   462,  -809,  -809,   424,
     417,   513,  4296,  4296,  -809,  -809,  2168,  2321,  2474,  2627,
    2780,  -809,   427,  -809,   431,    18,  -809,  -809,   430,  -809,
     435,   447,   451,  -809,  -809,   448,  5839,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  4296,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,   441,   445,
    -809,  -809,   436,  -809,  -809,   580,   584,  -809,  -809,  -809,
    4578,  -809,  -809,   449,   452,   419,  5623,  -809,  -809,   476,
     471,  -809,   463,  -809,  -809,  4221,  4221,  4221,  -809,   472,
    -809,  -809,  -809,  -809,  -809,  -809,  4296,   568,   477,  -809,
     480,    23,  -809,   474,  -809,  -809,  4296,  -809,  -809,  -809,
     479,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    5623,  -809,  -809,  -809,  5839,  5839,  -809,   485,  5839,  5839,
    5839,  5839,  5839,  5839,  5839,  5839,  5839,  5839,   481,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,   483,   613,  -809,   -40,  -809,   489,   488,  -809,
    4296,  -809,   419,  4296,  -809,  5623,   310,  -809,  -809,  -809,
     749,    65,   310,    71,  -809,   -20,  -809,  -809,  -809,  -809,
    -809,   -15,    80,  -809,  -809,  -809,  -809,   725,  -809,  -809,
    -809,  -809,   283,  -809,  -809,  -809,  -809,  -809,  -809,   495,
    5623,  4296,  -809,  5946,  -809,   277,  -809,   505,  5623,  -809,
     506,  5839,  -809,   507,   509,   510,   511,   515,   516,   517,
     518,   520,   521,  -809,   524,   419,  4596,  4578,   828,   828,
    -809,  -809,  -809,  -809,  -809,   568,  -809,   522,   419,   528,
     536,   538,  -809,  -809,   535,  -809,  -809,  -809,  -809,  -809,
    4004,   537,   539,  -809,   568,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,   568,    16,  -809,  4759,  -809,  -809,
    -809,  -809,  -809,   540,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,   667,   669,  -809,   555,   559,  -809,
    -809,  -809,  5623,   419,  -809,   532,  -809,   560,  4296,  -809,
    -809,  -809,  -809,   229,   820,  4393,   136,    83,   134,  -809,
    4135,  4522,   182,   562,   653,   202,   137,   149,   567,  -809,
    4296,  -809,    14,    14,  -809,  -809,  -809,  -809,  4135,   -26,
     -26,  4296,   -26,  -809,  -809,   841,  -809,  -809,  -809,   570,
     568,  -809,  -809,    23,  -809,  -809,  -809,  -809,   418,   294,
    -809,   571,  -809,   572,   574,   575,  -809,  -809,   577,   581,
     591,  -809,   419,  -809,   528,  2933,    79,  -809,  -809,  -809,
    -809,   181,   181,  4296,  4296,   332,  -809,   -26,   -26,   309,
    -809,  4296,  -809,  4264,  -809,  -809,  -809,   595,   596,  -809,
     -26,   -26,  -809,  -809,  -809,  -809,  -809,    84,  -809,  -809,
    -809,   277,  -809,   597,  4759,  -809,  -809,  3545,  -809,  -809,
    -809,   594,   598,   599,  -809,   602,  -809,  -809,  -809,  3086,
    3698,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  4393,   604,
    4296,  3239,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  4578,  -809,  -809,  -809,  -809,  -809,  -809,   601,   -40,
    -809,   828,  3851,  -809,  -809,   605,  -809,  -809
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -809,  -354,  -809,  -809,  -809,  -809,  -809,   197,  -809,  -809,
    -809,  -809,  -571,  -809,  -809,  -238,    51,  -809,  -809,   -78,
    -809,  -809,  -457,  -809,  -809,  -809,  -675,  -809,  -809,  -809,
    -809,  -809,   -77,   -27,  -809,  -709,  -543,   -25,  -809,  -488,
    -809,    29,  -809,  -809,  -809,  -809,  -468,  -809,  -809,  -809,
     -38,  -809,  -809,  -737,  -809,  -809,   -70,   500,  -809,  -809,
    -107,  -809,  -809,  -809,  -809,  -212,  -809,  -456,   -85,  -809,
    -809,  -809,  -809,  -234,  -809,  -809,  -809,  -809,  -142,   -57,
    -472,  -809,  -809,  -538,  -809,  -100,  -809,  -809,  -809,  -809,
     133,  -685,  -809,  -809,     7,  -170,   -69,   -73,  -809,   231,
    -809,  -809,  -786,  -809,  -181,  -809,  -809,  -809,  -201,  -809,
    -809,  -809,  -809,  -718,   -90,  -644,  -809,  -809,  -809,  -809,
    -809,    -4,     1,   -28,    -1,   -43,   675,   683,  -809,  -156,
    -809,  -809,  -809,  -232,  -809,   -56,  -116,    31,   -87,  -809,
     579,  -337,  -271,    -2,  -155,  -704,  -809,   -86,  -809,  -809,
      45,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -698,    24,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,   561,  -129,  -808,  -809,  -165,  -809,  -809,
    -809
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -384
static const yytype_int16 yytable[] =
{
     124,   126,   125,   127,   102,   135,   298,   396,   279,   157,
     578,   227,   228,   229,   230,   217,   218,   394,   253,   523,
     213,   214,   283,   490,   222,   504,   147,   152,   277,   463,
     503,   754,   693,   616,   110,   156,   158,   160,   450,   437,
     790,   301,   612,   809,   400,   178,   300,   677,   506,   534,
     810,   811,   409,   608,   515,   619,   221,   271,   204,   574,
       2,   575,   204,   204,   927,   274,   161,   471,   159,   870,
     766,   767,   768,   171,  -314,  -314,   766,   767,   768,  -316,
     814,   940,   922,   716,   766,   767,   768,   175,   180,   766,
     767,   768,   219,   897,   752,   223,   455,   176,   250,   859,
     298,   298,   252,   402,   394,   272,   299,   224,   260,   861,
     225,   910,  -259,   275,  -314,   137,   717,   183,   138,   138,
    -318,   896,   298,   174,   394,   157,   125,   127,   394,   447,
     447,   447,   298,   251,   462,   436,   452,   453,   959,   909,
     300,   300,   407,  -317,   761,   273,   394,  -226,   724,   694,
     870,   451,   158,   276,   178,   512,   287,   972,   576,  -318,
     205,   473,   300,   206,   205,   205,   949,   206,   440,   532,
     184,   620,   300,   467,  -315,   411,   404,  -318,   538,   414,
     185,   408,  -317,   418,   419,   919,   421,   422,   423,   424,
     425,   426,   427,   428,   429,   430,   431,   432,   433,   434,
     480,   493,   769,   140,   141,   438,   870,   807,   773,   637,
     204,   762,   446,   446,   446,   438,   938,  -271,   182,   454,
     817,   956,   493,   412,   186,   405,   533,   579,  -317,   187,
    -314,  -319,   493,   406,   406,   406,   957,   454,   116,  -319,
     526,   527,  -320,  -320,   438,   394,   528,   529,   477,   788,
     257,   886,   478,  -269,   394,   188,  -269,   970,  -269,   499,
    -269,   499,   530,   413,   144,   145,   117,   118,  -270,   560,
    -269,  -272,   464,   975,   138,   880,  -316,   509,  -319,   499,
     189,  -320,   848,  -273,   404,   190,   119,   917,   494,   258,
     216,   220,   582,  -316,   481,  -315,  -315,   729,   730,   191,
     394,   733,   734,   735,   736,   737,   738,   739,   740,   741,
     742,   192,   205,   193,   887,   206,  -267,   888,   411,   516,
     174,   889,   482,   483,   194,   754,   120,   121,   122,   862,
     195,    34,  -316,   173,  -315,   181,  -268,   174,   196,   182,
     182,   197,   484,   198,   935,   298,   140,   141,   941,   942,
     448,   449,   199,   200,   678,   234,  -271,   394,   684,  -271,
     201,  -271,   887,  -271,   202,   888,   412,   203,   906,   889,
      33,   394,   394,   394,   394,   394,   262,   256,   234,   722,
     278,  -269,   485,   486,   487,   300,  -270,  -267,  -268,  -270,
     281,  -270,  -267,  -270,   795,  -267,   284,  -267,  -268,  -267,
     499,  -268,   175,  -268,   454,  -268,   413,   144,   145,   499,
     286,   540,   541,   542,   288,  -272,   545,   289,  -272,   468,
    -272,   943,  -272,   435,   298,   442,   443,   411,   444,   947,
     488,  -314,  -318,  -317,  -316,   438,   298,  -315,   411,   298,
     298,   298,  -319,   562,  -320,   600,  -259,   479,   764,   510,
     501,   712,   523,   763,   772,   140,   141,   298,  -273,   481,
     525,  -273,   204,  -273,   300,  -273,   140,   141,   536,   438,
     539,   543,   481,   204,   568,   412,   300,   544,   561,   300,
     300,   300,   546,   481,   589,   547,   412,   482,   483,   563,
     548,   713,   549,   564,   557,   550,   551,   300,   552,   553,
     482,   483,   554,   555,   556,   558,   509,   484,   559,   406,
     589,   482,   483,   864,   865,   413,   144,   145,   618,   618,
     484,   565,   566,   569,   236,   237,   413,   144,   145,   570,
     571,   484,   573,   581,   298,   591,   493,   592,   593,   687,
     688,   689,   594,   601,   602,   603,   607,   485,   486,   487,
     604,   605,   866,   606,  -205,   648,   613,   493,   615,   116,
     485,   486,   487,   614,   205,    39,   626,   206,   627,   629,
     651,   485,   486,   487,   300,   205,   631,   695,   206,   696,
     697,   698,   632,   699,   700,   499,   633,   117,   118,   636,
     298,   298,   298,   298,   649,   514,   652,   902,   650,   712,
     653,  -264,   509,   711,   676,   232,   233,   119,   683,   148,
     679,   680,   618,   690,   714,   715,   726,   914,   712,   721,
     732,   750,   743,   751,   756,   701,   163,   757,   712,   727,
     300,   300,   300,   300,   600,   231,   786,   140,   141,   713,
     791,   794,   796,   916,   797,   798,   799,   120,   121,   122,
     800,   801,   802,   803,   164,   165,   816,   149,   713,   806,
     411,   804,   805,   232,   233,   702,   239,   240,   713,   818,
     820,   822,   234,   821,   166,   858,   760,   857,   874,   589,
     875,   872,   944,   235,   499,   881,   123,   916,   140,   141,
     876,   885,   842,   843,   877,   883,   903,   150,   144,   145,
     852,   905,   703,   904,   712,   918,   926,   928,   412,   929,
     930,   704,   705,   706,   167,   168,   169,   589,   931,   787,
     933,   236,   237,   238,   239,   240,   934,   792,   952,   953,
     394,  -383,   958,   963,   973,   962,   151,   964,   916,   967,
     977,   765,   936,   844,   713,   847,   236,   237,   413,   144,
     145,   711,   789,   170,   595,   596,   597,   489,   291,   939,
     898,   884,   396,   920,   912,   913,   628,   915,   974,   879,
     711,   855,   394,   907,   908,   396,   951,   854,   911,   925,
     711,   924,   255,   438,   394,   394,   292,   293,   277,   254,
     441,   808,   932,     0,   856,   461,   394,     0,     0,     0,
       0,     0,     0,   157,     0,   298,   294,   396,     0,     0,
       0,     0,   945,   946,   760,   298,     0,   394,     0,   891,
     892,   125,   127,     0,   895,   954,   955,     0,     0,   116,
     158,   899,     0,   231,     0,     0,   589,   291,     0,   655,
     656,   657,   658,   659,   660,   300,   295,   296,   297,   899,
     411,   250,     0,     0,   139,   300,   711,   117,   118,     0,
       0,   232,   233,     0,   438,   292,   293,     0,   775,   776,
     234,   777,   778,   779,   780,   781,   782,   119,   140,   141,
       0,   235,   140,   141,     0,   294,     0,   234,     0,   589,
     454,     0,     0,     0,     0,   454,     0,   589,   412,     0,
       0,     0,   142,     0,   950,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   120,   121,   122,
     438,   238,   239,   240,     0,   295,   296,   297,     0,     0,
       0,     0,     0,     0,     0,     0,   236,   237,   413,   144,
     145,     0,   143,   144,   145,     0,   454,   303,   304,   305,
     306,   307,   308,   309,   310,   311,   312,   313,   314,   315,
     316,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   317,   318,   319,   320,   321,   322,   323,   324,   325,
     326,    35,    36,     0,   327,     0,   328,   329,   330,   331,
     332,   333,   334,   335,   336,     0,   337,   338,     0,   339,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,   355,   356,   357,   358,   359,
     360,   361,   362,   363,   364,   365,   366,   367,   368,   369,
     370,   371,   372,   373,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   495,   375,     0,
     376,   496,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391,     0,   392,   393,
     303,   304,   305,   306,   307,   308,   309,   310,   311,   312,
     313,   314,   315,   316,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,   317,   318,   319,   320,   321,   322,
     323,   324,   325,   326,    35,    36,     0,   327,     0,   328,
     329,   330,   331,   332,   333,   334,   335,   336,     0,   337,
     338,     0,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,   356,
     357,   358,   359,   360,   361,   362,   363,   364,   365,   366,
     367,   368,   369,   370,   371,   372,   373,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     495,   375,   505,   376,     0,   377,   378,   379,   380,   381,
     382,   383,   384,   385,   386,   387,   388,   389,   390,   391,
       0,   392,   393,   303,   304,   305,   306,   307,   308,   309,
     310,   311,   312,   313,   314,   315,   316,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   317,   318,   319,
     320,   321,   322,   323,   324,   325,   326,    35,    36,     0,
     327,     0,   328,   329,   330,   331,   332,   333,   334,   335,
     336,     0,   337,   338,     0,   339,   340,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,   356,   357,   358,   359,   360,   361,   362,   363,
     364,   365,   366,   367,   368,   369,   370,   371,   372,   373,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   495,   375,     0,   376,   511,   377,   378,
     379,   380,   381,   382,   383,   384,   385,   386,   387,   388,
     389,   390,   391,     0,   392,   393,   303,   304,   305,   306,
     307,   308,   309,   310,   311,   312,   313,   314,   315,   316,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
     317,   318,   319,   320,   321,   322,   323,   324,   325,   326,
      35,    36,     0,   327,     0,   328,   329,   330,   331,   332,
     333,   334,   335,   336,     0,   337,   338,     0,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,   356,   357,   358,   359,   360,
     361,   362,   363,   364,   365,   366,   367,   368,   369,   370,
     371,   372,   373,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   495,   375,     0,   376,
     524,   377,   378,   379,   380,   381,   382,   383,   384,   385,
     386,   387,   388,   389,   390,   391,     0,   392,   393,   303,
     304,   305,   306,   307,   308,   309,   310,   311,   312,   313,
     314,   315,   316,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,   317,   318,   319,   320,   321,   322,   323,
     324,   325,   326,    35,    36,     0,   327,     0,   328,   329,
     330,   331,   332,   333,   334,   335,   336,     0,   337,   338,
       0,   339,   340,   341,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,   355,   356,   357,
     358,   359,   360,   361,   362,   363,   364,   365,   366,   367,
     368,   369,   370,   371,   372,   373,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   495,
     375,   531,   376,     0,   377,   378,   379,   380,   381,   382,
     383,   384,   385,   386,   387,   388,   389,   390,   391,     0,
     392,   393,   303,   304,   305,   306,   307,   308,   309,   310,
     311,   312,   313,   314,   315,   316,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,   317,   318,   319,   320,
     321,   322,   323,   324,   325,   326,    35,    36,     0,   327,
       0,   328,   329,   330,   331,   332,   333,   334,   335,   336,
       0,   337,   338,     0,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   495,   375,   537,   376,     0,   377,   378,   379,
     380,   381,   382,   383,   384,   385,   386,   387,   388,   389,
     390,   391,     0,   392,   393,   303,   304,   305,   306,   307,
     308,   309,   310,   311,   312,   313,   314,   315,   316,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,   317,
     318,   319,   320,   321,   322,   323,   324,   325,   326,    35,
      36,     0,   327,     0,   328,   329,   330,   331,   332,   333,
     334,   335,   336,     0,   337,   338,     0,   339,   340,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,   356,   357,   358,   359,   360,   361,
     362,   363,   364,   365,   366,   367,   368,   369,   370,   371,
     372,   373,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   495,   375,     0,   376,   567,
     377,   378,   379,   380,   381,   382,   383,   384,   385,   386,
     387,   388,   389,   390,   391,     0,   392,   393,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,    35,    36,     0,   327,     0,   328,   329,   330,
     331,   332,   333,   334,   335,   336,     0,   337,   338,     0,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   368,
     369,   370,   371,   372,   373,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   495,   375,
       0,   376,   609,   377,   378,   379,   380,   381,   382,   383,
     384,   385,   386,   387,   388,   389,   390,   391,     0,   392,
     393,   303,   304,   305,   306,   307,   308,   309,   310,   311,
     312,   313,   314,   315,   316,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,   317,   318,   319,   320,   321,
     322,   323,   324,   325,   326,    35,    36,     0,   327,     0,
     328,   329,   330,   331,   332,   333,   334,   335,   336,     0,
     337,   338,     0,   339,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,   362,   363,   364,   365,
     366,   367,   368,   369,   370,   371,   372,   373,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   495,   375,   621,   376,     0,   377,   378,   379,   380,
     381,   382,   383,   384,   385,   386,   387,   388,   389,   390,
     391,     0,   392,   393,   303,   304,   305,   306,   307,   308,
     309,   310,   311,   312,   313,   314,   315,   316,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,   317,   318,
     319,   320,   321,   322,   323,   324,   325,   326,    35,    36,
       0,   327,     0,   328,   329,   330,   331,   332,   333,   334,
     335,   336,     0,   337,   338,     0,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,   356,   357,   358,   359,   360,   361,   362,
     363,   364,   365,   366,   367,   368,   369,   370,   371,   372,
     373,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   495,   375,   622,   376,     0,   377,
     378,   379,   380,   381,   382,   383,   384,   385,   386,   387,
     388,   389,   390,   391,     0,   392,   393,   303,   304,   305,
     306,   307,   308,   309,   310,   311,   312,   313,   314,   315,
     316,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   317,   318,   319,   320,   321,   322,   323,   324,   325,
     326,    35,    36,     0,   327,     0,   328,   329,   330,   331,
     332,   333,   334,   335,   336,     0,   337,   338,     0,   339,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,   355,   356,   357,   358,   359,
     360,   361,   362,   363,   364,   365,   366,   367,   368,   369,
     370,   371,   372,   373,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   495,   375,   623,
     376,     0,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391,     0,   392,   393,
     303,   304,   305,   306,   307,   308,   309,   310,   311,   312,
     313,   314,   315,   316,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,   317,   318,   319,   320,   321,   322,
     323,   324,   325,   326,    35,    36,     0,   327,     0,   328,
     329,   330,   331,   332,   333,   334,   335,   336,     0,   337,
     338,     0,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,   356,
     357,   358,   359,   360,   361,   362,   363,   364,   365,   366,
     367,   368,   369,   370,   371,   372,   373,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     495,   375,     0,   376,   624,   377,   378,   379,   380,   381,
     382,   383,   384,   385,   386,   387,   388,   389,   390,   391,
       0,   392,   393,   303,   304,   305,   306,   307,   308,   309,
     310,   311,   312,   313,   314,   315,   316,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   317,   318,   319,
     320,   321,   322,   323,   324,   325,   326,    35,    36,     0,
     327,     0,   328,   329,   330,   331,   332,   333,   334,   335,
     336,     0,   337,   338,     0,   339,   340,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,   356,   357,   358,   359,   360,   361,   362,   363,
     364,   365,   366,   367,   368,   369,   370,   371,   372,   373,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   495,   375,     0,   376,     0,   377,   378,
     379,   380,   381,   382,   383,   384,   385,   386,   387,   388,
     389,   390,   391,   625,   392,   393,   303,   304,   305,   306,
     307,   308,   309,   310,   311,   312,   313,   314,   315,   316,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
     317,   318,   319,   320,   321,   322,   323,   324,   325,   326,
      35,    36,     0,   327,     0,   328,   329,   330,   331,   332,
     333,   334,   335,   336,     0,   337,   338,     0,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,   356,   357,   358,   359,   360,
     361,   362,   363,   364,   365,   366,   367,   368,   369,   370,
     371,   372,   373,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   495,   375,   937,   376,
       0,   377,   378,   379,   380,   381,   382,   383,   384,   385,
     386,   387,   388,   389,   390,   391,     0,   392,   393,   303,
     304,   305,   306,   307,   308,   309,   310,   311,   312,   313,
     314,   315,   316,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,   317,   318,   319,   320,   321,   322,   323,
     324,   325,   326,    35,    36,     0,   327,     0,   328,   329,
     330,   331,   332,   333,   334,   335,   336,     0,   337,   338,
       0,   339,   340,   341,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,   355,   356,   357,
     358,   359,   360,   361,   362,   363,   364,   365,   366,   367,
     368,   369,   370,   371,   372,   373,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   495,
     375,     0,   376,   965,   377,   378,   379,   380,   381,   382,
     383,   384,   385,   386,   387,   388,   389,   390,   391,     0,
     392,   393,   303,   304,   305,   306,   307,   308,   309,   310,
     311,   312,   313,   314,   315,   316,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,   317,   318,   319,   320,
     321,   322,   323,   324,   325,   326,    35,    36,     0,   327,
       0,   328,   329,   330,   331,   332,   333,   334,   335,   336,
       0,   337,   338,     0,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   495,   375,   968,   376,     0,   377,   378,   379,
     380,   381,   382,   383,   384,   385,   386,   387,   388,   389,
     390,   391,     0,   392,   393,   303,   304,   305,   306,   307,
     308,   309,   310,   311,   312,   313,   314,   315,   316,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,   317,
     318,   319,   320,   321,   322,   323,   324,   325,   326,    35,
      36,     0,   327,     0,   328,   329,   330,   331,   332,   333,
     334,   335,   336,     0,   337,   338,     0,   339,   340,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,   356,   357,   358,   359,   360,   361,
     362,   363,   364,   365,   366,   367,   368,   369,   370,   371,
     372,   373,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   374,   375,     0,   376,     0,
     377,   378,   379,   380,   381,   382,   383,   384,   385,   386,
     387,   388,   389,   390,   391,     0,   392,   393,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,    35,    36,     0,   327,     0,   328,   329,   330,
     331,   332,   333,   334,   335,   336,     0,   337,   338,     0,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   368,
     369,   370,   371,   372,   373,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   375,
     960,   376,     0,   377,   378,   379,   380,   381,   382,   383,
     384,   385,   386,   387,   388,   389,   390,   391,     0,   392,
     393,   303,   304,   305,   306,   307,   308,   309,   310,   311,
     312,   313,   314,   315,   316,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,   317,   318,   319,   320,   321,
     322,   323,   324,   325,   326,    35,    36,     0,   327,     0,
     328,   329,   330,   331,   332,   333,   334,   335,   336,     0,
     337,   338,     0,   339,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,   362,   363,   364,   365,
     366,   367,   368,   369,   370,   371,   372,   373,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   966,   375,     0,   376,     0,   377,   378,   379,   380,
     381,   382,   383,   384,   385,   386,   387,   388,   389,   390,
     391,     0,   392,   393,   303,   304,   305,   306,   307,   308,
     309,   310,   311,   312,   313,   314,   315,   316,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,   317,   318,
     319,   320,   321,   322,   323,   324,   325,   326,    35,    36,
       0,   327,     0,   328,   329,   330,   331,   332,   333,   334,
     335,   336,     0,   337,   338,     0,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,   356,   357,   358,   359,   360,   361,   362,
     363,   364,   365,   366,   367,   368,   369,   370,   371,   372,
     373,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   976,   375,     0,   376,     0,   377,
     378,   379,   380,   381,   382,   383,   384,   385,   386,   387,
     388,   389,   390,   391,     0,   392,   393,   823,   824,     0,
       0,     0,   825,   826,     0,     0,     0,     0,     0,     0,
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   827,   828,    30,    31,   829,     0,    33,     0,     0,
      34,    35,    36,   830,   831,    38,    39,    40,    41,     0,
      43,   832,    45,   155,    47,   833,     0,     0,   834,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   835,   836,   837,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,   838,   839,   823,   824,
       0,     0,     0,   825,   826,     0,     0,     0,     0,   840,
       0,     0,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,   827,   828,   131,   132,   264,     0,    33,     0,
       0,    34,    35,    36,     0,   831,    38,     0,    40,     0,
       0,     0,   832,   154,   155,     0,     0,     0,     0,   834,
       0,     0,     0,     0,   265,   266,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   267,     0,     0,     0,     0,     0,
     291,     0,   835,   836,   837,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   292,   293,
       0,     0,     0,     0,   268,   269,   270,   128,   129,     0,
       0,     0,   948,   826,     0,     0,     0,     0,   294,     0,
     840,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   827,   828,   131,   132,   411,     0,    33,     0,     0,
       0,    35,    36,     0,   831,    38,     0,    40,   295,   296,
     297,   832,   154,   155,     0,     0,     0,     0,   834,     0,
       0,     0,     0,   140,   141,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   412,     0,     0,     0,     0,     0,     0,
       0,   835,   836,   837,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   413,   144,   145,   128,   129,     0,     0,
       0,     0,   130,     0,     0,     0,     0,     0,     0,   840,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,   131,   132,     0,     0,    33,     0,     0,     0,
      35,    36,     0,   153,    38,     0,    40,     0,     0,     0,
     133,   154,   155,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     134,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   128,   129,     0,     0,     0,
       0,   826,     0,     0,     0,     0,     0,     0,   893,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,   827,
     828,   131,   132,     0,     0,    33,     0,     0,     0,    35,
      36,     0,     0,     0,     0,    40,     0,     0,     0,   832,
     154,     0,     0,     0,     0,     0,   900,   291,   654,   655,
     656,   657,   658,   659,   660,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   291,   654,   655,   656,   657,
     658,   659,   660,     0,     0,   292,   293,     0,     0,   835,
     836,   837,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,   292,   293,   294,     0,     0,     0,     0,
       0,     0,     0,   661,   662,   663,   664,     0,     0,     0,
       0,     0,     0,   294,     0,     0,     0,     0,     0,     0,
       0,   661,   662,   663,   664,     0,     0,   901,     0,     0,
       0,     0,     0,     0,     0,   295,   296,   297,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   295,   296,   297,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   665,     0,   666,     0,     0,     0,     0,     0,
       0,   667,   668,     0,     0,     0,     0,   128,   129,     0,
     665,     0,     0,   130,     0,     0,     0,     0,     0,   667,
     668,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,   131,   132,     0,     0,    33,   411,     0,
       0,    35,    36,     0,     0,     0,     0,    40,     0,     0,
       0,   133,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   140,   141,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   412,     0,     0,     0,
       0,   134,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     4,     5,     0,
       0,     0,     6,     7,   864,   865,   413,   144,   145,     0,
     403,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,     0,    33,     0,     0,
      34,    35,    36,   866,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,     0,    84,     4,     5,
       0,     0,     0,     6,   130,     0,     0,     0,     0,     0,
       0,     0,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,   131,   132,     0,     0,    33,     0,
       0,    34,    35,    36,     0,   153,    38,     0,    40,     0,
       0,     0,   133,   154,   155,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   134,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,   128,   129,     0,     0,
       0,     0,   130,     0,     0,     0,     0,     0,     0,     0,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,   131,   132,     0,     0,    33,     0,     0,     0,
      35,    36,     0,   153,    38,     0,    40,     0,     0,     0,
     133,   154,   155,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     134,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,   128,   129,     0,     0,     0,     0,
     130,     0,     0,     0,     0,     0,     0,     0,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
     131,   132,     0,     0,    33,     0,     0,     0,    35,    36,
       0,     0,     0,     0,    40,     0,     0,     0,   133,   154,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   134,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,   128,   129,     0,     0,     0,     0,   130,   177,
       0,     0,     0,     0,     0,     0,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,   131,   132,
       0,     0,    33,     0,     0,     0,    35,    36,     0,     0,
       0,     0,    40,     0,     0,     0,   133,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   134,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
       4,     5,     0,     0,     0,     0,   130,     0,     0,     0,
       0,     0,     0,     0,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,     0,     0,
      33,     0,     0,     0,    35,    36,     0,     0,     0,     0,
      40,     0,     0,     0,   133,     0,     0,   215,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   134,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,   128,   129,
       0,     0,     0,     0,   130,   285,     0,     0,     0,     0,
       0,     0,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,   131,   132,     0,     0,    33,     0,
       0,     0,    35,    36,     0,     0,     0,     0,    40,     0,
       0,     0,   133,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   134,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,   128,   129,     0,     0,
       0,     0,   130,     0,     0,     0,     0,     0,     0,     0,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,   131,   132,     0,     0,    33,     0,     0,     0,
      35,    36,     0,     0,     0,     0,    40,     0,     0,     0,
     133,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     134,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,   128,   129,     0,     0,     0,     0,
     130,     0,     0,     0,     0,     0,     0,     0,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
     131,   132,     0,     0,     0,     0,     0,     0,    35,    36,
       0,     0,     0,     0,    40,     0,     0,     0,   133,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   134,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,   128,   129,     0,     0,     0,     0,   264,     0,
       0,     0,     0,     0,     0,     0,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,   265,   266,   131,   132,
       0,     0,     0,     0,     0,     0,    35,    36,     0,     0,
       0,     0,     0,     0,     0,     0,   267,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   268,   269,   270,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
     517,     0,     0,     0,     0,   264,     0,     0,     0,     0,
       0,     0,     0,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,   265,   266,     0,     0,     0,     0,     0,
       0,     0,     0,    35,    36,     0,     0,   456,   457,   518,
       0,     0,     0,   267,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   268,   269,   270,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   458,
       0,     0,     0,   377,   378,     0,   380,   381,   382,   383,
     384,   385,   386,   387,   388,   389,   390,   459,     0,   392
};

static const yytype_int16 yycheck[] =
{
       4,     5,     4,     5,     3,     6,   162,   172,   137,    37,
     466,    96,    97,    98,    99,    93,    93,   172,   105,   290,
      89,    90,   151,   257,    94,   263,    30,    31,   135,   241,
     262,   675,   603,   521,     3,    34,    37,    38,   219,   209,
     725,   170,   514,   747,   173,    46,   162,   585,   280,   403,
     748,   749,   181,   510,   288,   523,    94,     9,    44,    43,
       0,    45,    44,    44,   872,     9,   138,   248,    37,   787,
       5,     6,     7,    42,    94,    94,     5,     6,     7,    94,
     755,   889,   868,    60,     5,     6,     7,    50,   134,     5,
       6,     7,    93,   830,   134,    94,   225,    60,   102,   774,
     256,   257,   103,   176,   259,    57,   162,   133,   138,   784,
     136,   848,   152,    57,   133,   134,    93,   134,   138,   138,
      94,   830,   278,   138,   279,   153,   128,   129,   283,   216,
     217,   218,   288,   102,   241,   208,   221,   222,   924,   848,
     256,   257,   180,    94,   682,    97,   301,   133,   616,   606,
     868,   220,   153,    97,   155,   284,   157,   965,   142,   133,
     146,   248,   278,   149,   146,   146,   903,   149,   149,   401,
     134,   525,   288,   246,    94,     9,   177,    94,   410,   183,
     134,   180,   133,   187,   188,   860,   190,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,   201,   202,   203,
     256,   257,   137,    37,    38,   209,   924,   745,   137,   546,
      44,   683,   216,   217,   218,   219,   137,   134,   138,   223,
     758,   137,   278,    57,   134,   133,   133,   133,    94,   134,
      94,    94,   288,   141,   141,   141,   921,   241,     9,    94,
     369,   370,    94,    94,   248,   400,   375,   376,   134,   721,
      94,   822,   138,   133,   409,   134,   136,   961,   138,   260,
     140,   262,   391,    97,    98,    99,    37,    38,   134,   439,
     134,   134,   241,   971,   138,   813,    94,   281,   133,   280,
     134,   133,   770,   134,   285,   134,    57,   858,   257,   133,
      93,    94,   473,    94,     9,    94,    94,   634,   635,   134,
     455,   638,   639,   640,   641,   642,   643,   644,   645,   646,
     647,   134,   146,   134,   133,   149,   134,   136,     9,   288,
     138,   140,    37,    38,   134,   969,    97,    98,    99,   785,
     134,    46,   133,   134,   133,   134,   134,   138,   134,   138,
     138,   134,    57,   134,   882,   501,    37,    38,   891,   892,
     217,   218,   134,   134,   586,    46,   133,   512,   592,   136,
     134,   138,   133,   140,   134,   136,    57,   134,   840,   140,
      43,   526,   527,   528,   529,   530,   138,    94,    46,   613,
      94,   136,    97,    98,    99,   501,   133,   136,   136,   136,
     136,   138,   133,   140,   731,   136,   136,   138,   133,   140,
     401,   136,    50,   138,   408,   140,    97,    98,    99,   410,
     134,   415,   416,   417,    94,   133,   420,   139,   136,   142,
     138,   893,   140,   133,   580,   133,   133,     9,   133,   901,
     145,    94,    94,    94,    94,   439,   592,    94,     9,   595,
     596,   597,    94,   447,    94,   501,   152,    94,   686,   136,
     140,   607,   723,   685,   692,    37,    38,   613,   133,     9,
     136,   136,    44,   138,   580,   140,    37,    38,   135,   473,
     141,   141,     9,    44,   152,    57,   592,   141,   133,   595,
     596,   597,   141,     9,   488,   141,    57,    37,    38,   133,
     141,   607,   141,   133,   135,   141,   141,   613,   141,   141,
      37,    38,   141,   141,   141,   135,   510,    57,   141,   141,
     514,    37,    38,    95,    96,    97,    98,    99,   522,   523,
      57,   133,   133,   152,    95,    96,    97,    98,    99,   135,
     153,    57,    94,   135,   690,   134,   592,    94,   141,   595,
     596,   597,   139,   136,   139,   136,   142,    97,    98,    99,
     139,   137,   134,   141,   135,   559,    94,   613,   141,     9,
      97,    98,    99,   139,   146,    52,   139,   149,   137,   139,
     134,    97,    98,    99,   690,   146,   141,     9,   149,    11,
      12,    13,   135,    15,    16,   586,   135,    37,    38,   141,
     746,   747,   748,   749,   153,   145,    16,   831,   153,   755,
      16,   152,   606,   607,   152,    37,    38,    57,   145,     9,
     134,   140,   616,   141,   137,   135,   137,   851,   774,   145,
     135,   138,   141,    10,   135,    57,     9,   139,   784,   630,
     746,   747,   748,   749,   690,     9,   141,    37,    38,   755,
     135,   135,   135,   855,   135,   135,   135,    97,    98,    99,
     135,   135,   135,   135,    37,    38,   134,    57,   774,   135,
       9,   141,   141,    37,    38,    97,    98,    99,   784,   141,
     134,   136,    46,   135,    57,   136,   680,   140,    11,   683,
      11,   141,   894,    57,   685,   153,   136,   899,    37,    38,
     135,   820,   770,   770,   135,   135,   134,    97,    98,    99,
     770,   134,   134,    50,   860,   135,   135,   135,    57,   135,
     135,   143,   144,   145,    97,    98,    99,   721,   141,   720,
     139,    95,    96,    97,    98,    99,   135,   728,   133,   133,
     885,   137,   135,   134,   133,   137,   136,   135,   950,   135,
     135,   690,   884,   770,   860,   770,    95,    96,    97,    98,
      99,   755,   723,   136,     5,     6,     7,   257,     9,   888,
     830,   818,   927,   863,   849,   850,   535,   852,   969,   812,
     774,   770,   927,   842,   843,   940,   905,   770,   848,   869,
     784,   868,   107,   787,   939,   940,    37,    38,   895,   106,
     211,   746,   878,    -1,   770,   234,   951,    -1,    -1,    -1,
      -1,    -1,    -1,   831,    -1,   961,    57,   972,    -1,    -1,
      -1,    -1,   897,   898,   818,   971,    -1,   972,    -1,   823,
     824,   823,   824,    -1,   825,   910,   911,    -1,    -1,     9,
     831,   830,    -1,     9,    -1,    -1,   840,     9,    -1,    11,
      12,    13,    14,    15,    16,   961,    97,    98,    99,   848,
       9,   855,    -1,    -1,     9,   971,   860,    37,    38,    -1,
      -1,    37,    38,    -1,   868,    37,    38,    -1,   143,   144,
      46,   146,   147,   148,   149,   150,   151,    57,    37,    38,
      -1,    57,    37,    38,    -1,    57,    -1,    46,    -1,   893,
     894,    -1,    -1,    -1,    -1,   899,    -1,   901,    57,    -1,
      -1,    -1,    57,    -1,   903,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    97,    98,    99,
     924,    97,    98,    99,    -1,    97,    98,    99,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    95,    96,    97,    98,
      99,    -1,    97,    98,    99,    -1,   950,     3,     4,     5,
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
     106,   107,   108,   109,   110,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   133,   134,    -1,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,    -1,   154,   155,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    -1,    50,    -1,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    -1,    62,
      63,    -1,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     133,   134,   135,   136,    -1,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   147,   148,   149,   150,   151,   152,
      -1,   154,   155,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    -1,
      50,    -1,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    -1,    62,    63,    -1,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   133,   134,    -1,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   152,    -1,   154,   155,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    -1,    50,    -1,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    -1,    62,    63,    -1,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   133,   134,    -1,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     147,   148,   149,   150,   151,   152,    -1,   154,   155,     3,
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
     104,   105,   106,   107,   108,   109,   110,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   133,
     134,   135,   136,    -1,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   148,   149,   150,   151,   152,    -1,
     154,   155,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    -1,    50,
      -1,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      -1,    62,    63,    -1,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   133,   134,   135,   136,    -1,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,   148,   149,   150,
     151,   152,    -1,   154,   155,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    -1,    50,    -1,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    -1,    62,    63,    -1,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   133,   134,    -1,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,    -1,   154,   155,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    -1,    50,    -1,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    -1,    62,    63,    -1,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   133,   134,
      -1,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,    -1,   154,
     155,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    -1,    50,    -1,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    -1,
      62,    63,    -1,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   133,   134,   135,   136,    -1,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   147,   148,   149,   150,   151,
     152,    -1,   154,   155,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      -1,    50,    -1,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    -1,    62,    63,    -1,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   133,   134,   135,   136,    -1,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
     149,   150,   151,   152,    -1,   154,   155,     3,     4,     5,
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
     106,   107,   108,   109,   110,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   133,   134,   135,
     136,    -1,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,    -1,   154,   155,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    -1,    50,    -1,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    -1,    62,
      63,    -1,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     133,   134,    -1,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   147,   148,   149,   150,   151,   152,
      -1,   154,   155,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    -1,
      50,    -1,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    -1,    62,    63,    -1,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   133,   134,    -1,   136,    -1,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   152,   153,   154,   155,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    -1,    50,    -1,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    -1,    62,    63,    -1,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   133,   134,   135,   136,
      -1,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     147,   148,   149,   150,   151,   152,    -1,   154,   155,     3,
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
     104,   105,   106,   107,   108,   109,   110,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   133,
     134,    -1,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   148,   149,   150,   151,   152,    -1,
     154,   155,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    -1,    50,
      -1,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      -1,    62,    63,    -1,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   133,   134,   135,   136,    -1,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,   148,   149,   150,
     151,   152,    -1,   154,   155,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    -1,    50,    -1,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    -1,    62,    63,    -1,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   133,   134,    -1,   136,    -1,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,    -1,   154,   155,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    -1,    50,    -1,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    -1,    62,    63,    -1,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   134,
     135,   136,    -1,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,    -1,   154,
     155,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    -1,    50,    -1,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    -1,
      62,    63,    -1,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   133,   134,    -1,   136,    -1,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   147,   148,   149,   150,   151,
     152,    -1,   154,   155,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      -1,    50,    -1,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    -1,    62,    63,    -1,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   133,   134,    -1,   136,    -1,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
     149,   150,   151,   152,    -1,   154,   155,     3,     4,    -1,
      -1,    -1,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    -1,    43,    -1,    -1,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    -1,
      56,    57,    58,    59,    60,    61,    -1,    -1,    64,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,     3,     4,
      -1,    -1,    -1,     8,     9,    -1,    -1,    -1,    -1,   145,
      -1,    -1,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,     9,    -1,    43,    -1,
      -1,    46,    47,    48,    -1,    50,    51,    -1,    53,    -1,
      -1,    -1,    57,    58,    59,    -1,    -1,    -1,    -1,    64,
      -1,    -1,    -1,    -1,    37,    38,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    57,    -1,    -1,    -1,    -1,    -1,
       9,    -1,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    37,    38,
      -1,    -1,    -1,    -1,    97,    98,    99,     3,     4,    -1,
      -1,    -1,     8,     9,    -1,    -1,    -1,    -1,    57,    -1,
     145,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,     9,    -1,    43,    -1,    -1,
      -1,    47,    48,    -1,    50,    51,    -1,    53,    97,    98,
      99,    57,    58,    59,    -1,    -1,    -1,    -1,    64,    -1,
      -1,    -1,    -1,    37,    38,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    57,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    97,    98,    99,     3,     4,    -1,    -1,
      -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,   145,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    -1,    -1,    43,    -1,    -1,    -1,
      47,    48,    -1,    50,    51,    -1,    53,    -1,    -1,    -1,
      57,    58,    59,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     3,     4,    -1,    -1,    -1,
      -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,   145,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    -1,    -1,    43,    -1,    -1,    -1,    47,
      48,    -1,    -1,    -1,    -1,    53,    -1,    -1,    -1,    57,
      58,    -1,    -1,    -1,    -1,    -1,    64,     9,    10,    11,
      12,    13,    14,    15,    16,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     9,    10,    11,    12,    13,
      14,    15,    16,    -1,    -1,    37,    38,    -1,    -1,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,    37,    38,    57,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    65,    66,    67,    68,    -1,    -1,    -1,
      -1,    -1,    -1,    57,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    66,    67,    68,    -1,    -1,   145,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    97,    98,    99,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    97,    98,    99,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   134,    -1,   136,    -1,    -1,    -1,    -1,    -1,
      -1,   143,   144,    -1,    -1,    -1,    -1,     3,     4,    -1,
     134,    -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,   143,
     144,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    -1,    -1,    43,     9,    -1,
      -1,    47,    48,    -1,    -1,    -1,    -1,    53,    -1,    -1,
      -1,    57,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    37,    38,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    57,    -1,    -1,    -1,
      -1,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,    -1,
      -1,    -1,     8,     9,    95,    96,    97,    98,    99,    -1,
     136,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    -1,    43,    -1,    -1,
      46,    47,    48,   134,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,   128,   129,   130,   131,    -1,   133,     3,     4,
      -1,    -1,    -1,     8,     9,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    -1,    -1,    43,    -1,
      -1,    46,    47,    48,    -1,    50,    51,    -1,    53,    -1,
      -1,    -1,    57,    58,    59,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,     3,     4,    -1,    -1,
      -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    -1,    -1,    43,    -1,    -1,    -1,
      47,    48,    -1,    50,    51,    -1,    53,    -1,    -1,    -1,
      57,    58,    59,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,     3,     4,    -1,    -1,    -1,    -1,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    -1,    -1,    43,    -1,    -1,    -1,    47,    48,
      -1,    -1,    -1,    -1,    53,    -1,    -1,    -1,    57,    58,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,     3,     4,    -1,    -1,    -1,    -1,     9,    10,
      -1,    -1,    -1,    -1,    -1,    -1,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      -1,    -1,    43,    -1,    -1,    -1,    47,    48,    -1,    -1,
      -1,    -1,    53,    -1,    -1,    -1,    57,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
       3,     4,    -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    -1,    -1,
      43,    -1,    -1,    -1,    47,    48,    -1,    -1,    -1,    -1,
      53,    -1,    -1,    -1,    57,    -1,    -1,    60,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,     3,     4,
      -1,    -1,    -1,    -1,     9,    10,    -1,    -1,    -1,    -1,
      -1,    -1,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    -1,    -1,    43,    -1,
      -1,    -1,    47,    48,    -1,    -1,    -1,    -1,    53,    -1,
      -1,    -1,    57,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,     3,     4,    -1,    -1,
      -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    -1,    -1,    43,    -1,    -1,    -1,
      47,    48,    -1,    -1,    -1,    -1,    53,    -1,    -1,    -1,
      57,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,     3,     4,    -1,    -1,    -1,    -1,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    -1,    -1,    -1,    -1,    -1,    -1,    47,    48,
      -1,    -1,    -1,    -1,    53,    -1,    -1,    -1,    57,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,     3,     4,    -1,    -1,    -1,    -1,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      -1,    -1,    -1,    -1,    -1,    -1,    47,    48,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    57,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
       4,    -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    47,    48,    -1,    -1,    62,    63,    53,
      -1,    -1,    -1,    57,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   134,
      -1,    -1,    -1,   138,   139,    -1,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,    -1,   154
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   157,     0,   158,     3,     4,     8,     9,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    43,    46,    47,    48,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   133,   159,   160,   161,   163,   175,
     188,   189,   193,   194,   195,   204,   206,   207,   211,   212,
     213,   250,   278,   279,   280,   281,   282,   283,   285,   292,
     293,   297,   298,   299,   300,   315,     9,    37,    38,    57,
      97,    98,    99,   136,   277,   299,   277,   299,     3,     4,
       9,    39,    40,    57,    97,   280,   293,   134,   138,     9,
      37,    38,    57,    97,    98,    99,   177,   277,     9,    57,
      97,   136,   277,    50,    58,    59,   278,   279,   280,   293,
     280,   138,   284,     9,    37,    38,    57,    97,    98,    99,
     136,   293,   341,   134,   138,    50,    60,    10,   280,   253,
     134,   134,   138,   134,   134,   134,   134,   134,   134,   134,
     134,   134,   134,   134,   134,   134,   134,   134,   134,   134,
     134,   134,   134,   134,    44,   146,   149,   252,   255,   256,
     294,   295,   296,   252,   252,    60,   163,   175,   188,   280,
     163,   206,   212,   278,   133,   136,   224,   224,   224,   224,
     224,     9,    37,    38,    46,    57,    95,    96,    97,    98,
      99,   208,   216,   218,   221,   225,   251,   260,   263,   269,
     277,   293,   280,   294,   283,   282,    94,    94,   133,   340,
     138,   166,   138,   164,     9,    37,    38,    57,    97,    98,
      99,     9,    57,    97,     9,    57,    97,   216,    94,   340,
     287,   136,   176,   340,   136,    10,   134,   280,    94,   139,
     196,     9,    37,    38,    57,    97,    98,    99,   285,   291,
     292,   340,   162,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    50,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    62,    63,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   133,   134,   136,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,   148,   149,   150,
     151,   152,   154,   155,   300,   339,   343,   344,   345,   346,
     340,   286,   253,   136,   280,   133,   141,   206,   278,   340,
     288,     9,    57,    97,   277,   317,   320,   321,   277,   277,
     325,   277,   277,   277,   277,   277,   277,   277,   277,   277,
     277,   277,   277,   277,   277,   133,   253,   251,   277,   257,
     149,   296,   133,   133,   133,   246,   277,   294,   246,   246,
     260,   252,   224,   224,   277,   340,    62,    63,   134,   152,
     338,   339,   216,   221,   293,   217,   222,   253,   142,   247,
     248,   260,   267,   294,   270,   271,   272,   134,   138,    94,
     291,     9,    37,    38,    57,    97,    98,    99,   145,   213,
     229,   231,   277,   291,   293,   133,   137,   342,   343,   280,
     289,   140,   171,   289,   171,   135,   289,   178,   179,   277,
     136,   137,   340,   214,   145,   229,   293,     4,    53,   197,
     199,   200,   201,   298,   137,   136,   340,   340,   340,   340,
     340,   135,   289,   133,   157,   254,   135,   135,   289,   141,
     277,   277,   277,   141,   141,   277,   141,   141,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   135,   135,   141,
     251,   133,   277,   133,   133,   133,   133,   137,   152,   152,
     135,   153,   219,    94,    43,    45,   142,   223,   223,   133,
     249,   135,   260,   273,   275,   226,   227,   236,   237,   277,
     230,   134,    94,   141,   139,     5,     6,     7,   172,   173,
     291,   136,   139,   136,   139,   137,   141,   142,   178,   137,
     239,   240,   236,    94,   139,   141,   195,   202,   277,   202,
     157,   135,   135,   135,   137,   153,   139,   137,   255,   139,
     316,   141,   135,   135,   322,   324,   141,   297,   328,   330,
     332,   334,   329,   331,   333,   335,   336,   337,   277,   153,
     153,   134,    16,    16,    10,    11,    12,    13,    14,    15,
      16,    65,    66,    67,    68,   134,   136,   143,   144,   291,
     301,   306,   312,   313,   314,   261,   152,   239,   289,   134,
     140,   233,   232,   145,   229,   290,   167,   291,   291,   291,
     141,   168,   165,   168,   178,     9,    11,    12,    13,    15,
      16,    57,    97,   134,   143,   144,   145,   180,   181,   182,
     186,   277,   285,   292,   137,   135,    60,    93,   241,   243,
     244,   145,   229,   198,   202,   203,   137,   280,   318,   297,
     297,   326,   135,   297,   297,   297,   297,   297,   297,   297,
     297,   297,   297,   141,   303,   220,   309,   302,   308,   307,
     138,    10,   134,   264,   271,   274,   135,   139,   238,   235,
     277,   239,   236,   289,   171,   172,     5,     6,     7,   137,
     169,   174,   171,   137,   185,   143,   144,   146,   147,   148,
     149,   150,   151,   187,   183,   215,   141,   280,   236,   197,
     247,   135,   280,   323,   135,   297,   135,   135,   135,   135,
     135,   135,   135,   135,   141,   141,   135,   239,   306,   301,
     314,   314,   310,   265,   182,   276,   134,   239,   141,   234,
     134,   135,   136,     3,     4,     8,     9,    37,    38,    41,
      49,    50,    57,    61,    64,    97,    98,    99,   132,   133,
     145,   170,   175,   188,   189,   190,   191,   193,   195,   205,
     209,   210,   212,   229,   250,   278,   315,   140,   136,   182,
     184,   182,   223,   242,    95,    96,   134,   258,   262,   268,
     269,   319,   141,   327,    11,    11,   135,   135,   304,   281,
     239,   153,   228,   135,   235,   340,   168,   133,   136,   140,
     192,   277,   277,   145,   278,   280,   191,   209,   212,   278,
      64,   145,   229,   134,    50,   134,   236,   252,   252,   191,
     209,   212,   224,   224,   229,   224,   221,   168,   135,   182,
     241,   245,   258,   266,   294,   270,   135,   341,   135,   135,
     135,   141,   303,   139,   135,   239,   234,   135,   137,   340,
     341,   192,   192,   236,   221,   224,   224,   236,     8,   209,
     278,   340,   133,   133,   224,   224,   137,   247,   135,   258,
     135,   305,   137,   134,   135,   137,   133,   135,   135,   259,
     301,   311,   341,   133,   264,   314,   133,   135
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
#line 1267 "vtkParse.y"
    { startSig(); clearTypeId(); clearTemplate(); closeComment(); }
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 1281 "vtkParse.y"
    { output_function(); }
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 1282 "vtkParse.y"
    { output_function(); }
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 1283 "vtkParse.y"
    { reject_function(); }
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 1284 "vtkParse.y"
    { output_function(); }
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 1285 "vtkParse.y"
    { reject_function(); }
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 1286 "vtkParse.y"
    { output_function(); }
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 1287 "vtkParse.y"
    { legacySig(); output_function(); }
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 1305 "vtkParse.y"
    { pushNamespace((yyvsp[(2) - (2)].str)); }
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 1306 "vtkParse.y"
    { popNamespace(); }
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 1313 "vtkParse.y"
    { start_class((yyvsp[(2) - (2)].str), 0); }
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 1314 "vtkParse.y"
    { end_class(); }
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 1315 "vtkParse.y"
    { reject_class((yyvsp[(2) - (5)].str), 0); }
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 1316 "vtkParse.y"
    { end_class(); }
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 1317 "vtkParse.y"
    { start_class((yyvsp[(2) - (2)].str), 1); }
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 1318 "vtkParse.y"
    { end_class(); }
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 1319 "vtkParse.y"
    { reject_class((yyvsp[(2) - (5)].str), 1); }
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 1320 "vtkParse.y"
    { end_class(); }
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 1326 "vtkParse.y"
    { startSig(); clearTypeId(); clearTemplate();  closeComment(); }
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 1340 "vtkParse.y"
    { output_function(); }
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 1341 "vtkParse.y"
    { ClassInfo *tmpc = currentClass;
     currentClass = NULL; reject_function(); currentClass = tmpc; }
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 1343 "vtkParse.y"
    { output_function(); }
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 1344 "vtkParse.y"
    { output_function(); }
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 1345 "vtkParse.y"
    { ClassInfo *tmpc = currentClass;
     currentClass = NULL; reject_function(); currentClass = tmpc; }
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 1347 "vtkParse.y"
    { output_function(); }
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 1348 "vtkParse.y"
    { legacySig(); output_function(); }
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 1361 "vtkParse.y"
    {
      vtkParse_AddStringToArray(&currentClass->SuperClasses,
                                &currentClass->NumberOfSuperClasses,
                                vtkstrdup((yyvsp[(2) - (2)].str)));
    }
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 1367 "vtkParse.y"
    {access_level = VTK_ACCESS_PUBLIC;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 1368 "vtkParse.y"
    {access_level = VTK_ACCESS_PROTECTED;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 1369 "vtkParse.y"
    {access_level = VTK_ACCESS_PRIVATE;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 1379 "vtkParse.y"
    {start_enum((yyvsp[(2) - (2)].str));}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 1380 "vtkParse.y"
    {end_enum();}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 1381 "vtkParse.y"
    {start_enum(NULL);}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 1382 "vtkParse.y"
    {end_enum();}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 1386 "vtkParse.y"
    {add_enum((yyvsp[(1) - (1)].str), NULL);}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 1387 "vtkParse.y"
    {add_enum((yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 1389 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 1390 "vtkParse.y"
    {(yyval.str) = vtkstrdup(add_const_scope((yyvsp[(1) - (1)].str)));}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 1395 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 1396 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 1397 "vtkParse.y"
    {
         (yyval.str) = vtkstrcat((yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));
       }
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 1400 "vtkParse.y"
    {postSig((yyvsp[(2) - (2)].str));}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 1401 "vtkParse.y"
    {
         (yyval.str) = vtkstrcat5((yyvsp[(1) - (4)].str), " ", (yyvsp[(2) - (4)].str), " ", (yyvsp[(4) - (4)].str));
       }
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 1404 "vtkParse.y"
    {postSig("(");}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 1405 "vtkParse.y"
    {
         (yyval.str) = vtkstrcat3("(", (yyvsp[(3) - (4)].str), ")");
       }
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 1409 "vtkParse.y"
    { (yyval.str) = "-"; }
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 1409 "vtkParse.y"
    { (yyval.str) = "+"; }
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 1410 "vtkParse.y"
    { (yyval.str) = "~"; }
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 1412 "vtkParse.y"
    { (yyval.str) = "-"; }
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 1412 "vtkParse.y"
    { (yyval.str) = "+"; }
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 1413 "vtkParse.y"
    { (yyval.str) = "*"; }
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 1413 "vtkParse.y"
    { (yyval.str) = "/"; }
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 1414 "vtkParse.y"
    { (yyval.str) = "%"; }
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 1414 "vtkParse.y"
    { (yyval.str) = "&"; }
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 1415 "vtkParse.y"
    { (yyval.str) = "|"; }
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 1415 "vtkParse.y"
    { (yyval.str) = "^"; }
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 1441 "vtkParse.y"
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

  case 122:

/* Line 1455 of yacc.c  */
#line 1463 "vtkParse.y"
    { }
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 1464 "vtkParse.y"
    { }
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 1465 "vtkParse.y"
    { }
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 1466 "vtkParse.y"
    { }
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1468 "vtkParse.y"
    { }
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1474 "vtkParse.y"
    { postSig("template<> "); clearTypeId(); }
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1475 "vtkParse.y"
    { postSig("template<");
          clearTypeId(); startTemplate(); }
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1477 "vtkParse.y"
    { chopSig();
            if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
            postSig("> "); clearTypeId(); }
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1482 "vtkParse.y"
    { chopSig(); postSig(", "); clearTypeId(); }
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1486 "vtkParse.y"
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

  case 134:

/* Line 1455 of yacc.c  */
#line 1496 "vtkParse.y"
    {
               TemplateArg *arg = (TemplateArg *)malloc(sizeof(TemplateArg));
               vtkParse_InitTemplateArg(arg);
               arg->Name = vtkstrdup(getVarName());
               arg->Value = vtkstrdup(getVarValue());
               vtkParse_AddArgumentToTemplate(currentTemplate, arg);
               }
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1503 "vtkParse.y"
    { pushTemplate(); }
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1504 "vtkParse.y"
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

  case 137:

/* Line 1455 of yacc.c  */
#line 1515 "vtkParse.y"
    {postSig("class ");}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1516 "vtkParse.y"
    {postSig("typename ");}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1518 "vtkParse.y"
    { setVarName((yyvsp[(1) - (1)].str)); }
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1548 "vtkParse.y"
    {openSig(); preSig("~"); closeSig();}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1549 "vtkParse.y"
    {openSig(); preSig("~"); closeSig();}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 1551 "vtkParse.y"
    {
         openSig(); preSig("virtual ~"); closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 1558 "vtkParse.y"
    {
         openSig();
         preSig("explicit ");
         closeSig();
         currentFunction->IsExplicit = 1;
         }
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1566 "vtkParse.y"
    {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1582 "vtkParse.y"
    {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1591 "vtkParse.y"
    {
      postSig("(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1595 "vtkParse.y"
    { postSig(")"); }
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 1596 "vtkParse.y"
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

  case 175:

/* Line 1455 of yacc.c  */
#line 1607 "vtkParse.y"
    { postSig(")"); }
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 1608 "vtkParse.y"
    {
      postSig(";");
      closeSig();
      currentFunction->Name = vtkstrdup((yyvsp[(1) - (3)].str));
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed operator", currentFunction->Name);
    }
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 1616 "vtkParse.y"
    {postSig((yyvsp[(2) - (2)].str));}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1617 "vtkParse.y"
    {
      postSig("(");
      currentFunction->IsOperator = 1;
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1622 "vtkParse.y"
    { (yyval.str) = (yyvsp[(2) - (7)].str); }
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 1624 "vtkParse.y"
    { postSig(")"); }
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 1625 "vtkParse.y"
    {
      postSig(";");
      closeSig();
      currentFunction->Name = vtkstrdup((yyvsp[(1) - (3)].str));
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 1635 "vtkParse.y"
    {
      postSig(" = 0");
      if (currentClass)
        {
        currentFunction->IsPureVirtual = 1;
        currentClass->IsAbstract = 1;
        }
    }
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 1644 "vtkParse.y"
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

  case 185:

/* Line 1455 of yacc.c  */
#line 1654 "vtkParse.y"
    {
      postSig(" const");
      currentFunction->IsConst = 1;
    }
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 1662 "vtkParse.y"
    {
      postSig("(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 1665 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (5)].str); }
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 1666 "vtkParse.y"
    {markSig(); postSig("<");}
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 1667 "vtkParse.y"
    {
      if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
      (yyval.str) = vtkstrcat((yyvsp[(1) - (6)].str), copySig());
    }
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 1672 "vtkParse.y"
    { (yyval.str) = (yyvsp[(7) - (9)].str); }
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 1674 "vtkParse.y"
    { postSig(");"); closeSig(); }
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 1675 "vtkParse.y"
    {
      currentFunction->Name = vtkstrdup((yyvsp[(1) - (3)].str));
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 1681 "vtkParse.y"
    { postSig("("); }
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 1690 "vtkParse.y"
    {
      postSig(");");
      closeSig();
      currentFunction->Name = vtkstrcat("~", (yyvsp[(1) - (1)].str));
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 1698 "vtkParse.y"
    { postSig("(");}
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 1704 "vtkParse.y"
    {clearTypeId();}
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 1707 "vtkParse.y"
    { currentFunction->IsVariadic = 1; postSig("..."); }
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 1708 "vtkParse.y"
    { clearTypeId(); }
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 1709 "vtkParse.y"
    { clearTypeId(); postSig(", "); }
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 1712 "vtkParse.y"
    { markSig(); }
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 1714 "vtkParse.y"
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

  case 214:

/* Line 1455 of yacc.c  */
#line 1736 "vtkParse.y"
    {
      int i = currentFunction->NumberOfArguments-1;
      if (getVarValue())
        {
        currentFunction->Arguments[i]->Value = vtkstrdup(getVarValue());
        }
    }
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 1744 "vtkParse.y"
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

  case 218:

/* Line 1455 of yacc.c  */
#line 1769 "vtkParse.y"
    {clearVarValue();}
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 1771 "vtkParse.y"
    { postSig("="); clearVarValue();}
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 1772 "vtkParse.y"
    { setVarValue((yyvsp[(3) - (3)].str)); }
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 1783 "vtkParse.y"
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

  case 229:

/* Line 1455 of yacc.c  */
#line 1797 "vtkParse.y"
    {postSig(", ");}
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 1800 "vtkParse.y"
    { setStorageTypeIndirection(0); }
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 1801 "vtkParse.y"
    { setStorageTypeIndirection((yyvsp[(1) - (1)].integer)); }
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 1805 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 1806 "vtkParse.y"
    { postSig(")"); }
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 1808 "vtkParse.y"
    {
         unsigned int parens = add_indirection((yyvsp[(1) - (5)].integer), (yyvsp[(2) - (5)].integer));
         if ((yyvsp[(5) - (5)].integer) == VTK_PARSE_FUNCTION) {
           (yyval.integer) = (parens | VTK_PARSE_FUNCTION); }
         else if ((yyvsp[(5) - (5)].integer) == VTK_PARSE_ARRAY) {
           (yyval.integer) = add_indirection_to_array(parens); }
       }
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 1817 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 1818 "vtkParse.y"
    { postSig(")"); }
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 1820 "vtkParse.y"
    {
         unsigned int parens = add_indirection((yyvsp[(1) - (5)].integer), (yyvsp[(2) - (5)].integer));
         if ((yyvsp[(5) - (5)].integer) == VTK_PARSE_FUNCTION) {
           (yyval.integer) = (parens | VTK_PARSE_FUNCTION); }
         else if ((yyvsp[(5) - (5)].integer) == VTK_PARSE_ARRAY) {
           (yyval.integer) = add_indirection_to_array(parens); }
       }
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 1828 "vtkParse.y"
    { postSig("("); (yyval.integer) = 0; }
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 1829 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("*");
               (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 1831 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("&");
               (yyval.integer) = VTK_PARSE_REF; }
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 1834 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("*");
               (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 1836 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("&");
               (yyval.integer) = VTK_PARSE_REF; }
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 1839 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 1840 "vtkParse.y"
    { pushFunction(); postSig("("); }
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 1841 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_FUNCTION; postSig(")"); popFunction(); }
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 1842 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_ARRAY; }
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 1845 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 1847 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(1) - (2)].integer), (yyvsp[(2) - (2)].integer));}
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 1850 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 1852 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(1) - (2)].integer), (yyvsp[(2) - (2)].integer));}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 1854 "vtkParse.y"
    {clearVarName(); chopSig();}
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 1856 "vtkParse.y"
    {setVarName((yyvsp[(1) - (1)].str));}
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 1858 "vtkParse.y"
    {clearArray();}
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 1860 "vtkParse.y"
    {clearArray();}
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 1862 "vtkParse.y"
    {postSig("[");}
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 1862 "vtkParse.y"
    {postSig("]");}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 1866 "vtkParse.y"
    {pushArraySize("");}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 1867 "vtkParse.y"
    {pushArraySize((yyvsp[(1) - (1)].str));}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 1873 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 1874 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 1875 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 1876 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 1877 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 1878 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 1879 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 1886 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer); setStorageType((yyval.integer));}
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 1887 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 1888 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 1890 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(3) - (3)].integer); setStorageType((yyval.integer));}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 1891 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 1892 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer));
      setStorageType((yyval.integer));}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 1894 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(2) - (3)].integer) | (yyvsp[(3) - (3)].integer));
      setStorageType((yyval.integer));}
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 1898 "vtkParse.y"
    {postSig("static "); (yyval.integer) = VTK_PARSE_STATIC; }
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 1899 "vtkParse.y"
    {postSig("static "); (yyval.integer) = VTK_PARSE_STATIC; }
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 1901 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 1902 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer));}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 1904 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 1905 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_CONST | (yyvsp[(2) - (2)].integer));}
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 1906 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_CONST | (yyvsp[(1) - (2)].integer));}
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 1908 "vtkParse.y"
    {postSig("const ");}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 1912 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 1914 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 1915 "vtkParse.y"
    {postSig("typename ");}
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 1916 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(3) - (3)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 294:

/* Line 1455 of yacc.c  */
#line 1919 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 295:

/* Line 1455 of yacc.c  */
#line 1920 "vtkParse.y"
    {chopSig(); if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 296:

/* Line 1455 of yacc.c  */
#line 1922 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 1923 "vtkParse.y"
    {chopSig(); if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 1925 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 1926 "vtkParse.y"
    {chopSig(); if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 1929 "vtkParse.y"
    {postSig(", ");}
    break;

  case 303:

/* Line 1455 of yacc.c  */
#line 1931 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 304:

/* Line 1455 of yacc.c  */
#line 1932 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 305:

/* Line 1455 of yacc.c  */
#line 1933 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 1934 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 307:

/* Line 1455 of yacc.c  */
#line 1935 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 1936 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 309:

/* Line 1455 of yacc.c  */
#line 1937 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 312:

/* Line 1455 of yacc.c  */
#line 1942 "vtkParse.y"
    {
             (yyval.str) = vtkstrcat3((yyvsp[(1) - (3)].str), "::", (yyvsp[(3) - (3)].str));
             preScopeSig((yyvsp[(1) - (3)].str));
           }
    break;

  case 313:

/* Line 1455 of yacc.c  */
#line 1947 "vtkParse.y"
    {
             (yyval.str) = vtkstrcat3((yyvsp[(1) - (3)].str), "::", (yyvsp[(3) - (3)].str));
             preScopeSig("");
           }
    break;

  case 321:

/* Line 1455 of yacc.c  */
#line 1968 "vtkParse.y"
    { postSig("&"); (yyval.integer) = VTK_PARSE_REF;}
    break;

  case 322:

/* Line 1455 of yacc.c  */
#line 1969 "vtkParse.y"
    { postSig("&"); (yyval.integer) = ((yyvsp[(1) - (2)].integer) | VTK_PARSE_REF);}
    break;

  case 323:

/* Line 1455 of yacc.c  */
#line 1970 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 324:

/* Line 1455 of yacc.c  */
#line 1975 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 325:

/* Line 1455 of yacc.c  */
#line 1977 "vtkParse.y"
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

  case 326:

/* Line 1455 of yacc.c  */
#line 1988 "vtkParse.y"
    { postSig("*"); (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 1989 "vtkParse.y"
    { postSig("*const "); (yyval.integer) = VTK_PARSE_CONST_POINTER; }
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 1992 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 329:

/* Line 1455 of yacc.c  */
#line 1993 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); }
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 1994 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); }
    break;

  case 331:

/* Line 1455 of yacc.c  */
#line 1995 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 332:

/* Line 1455 of yacc.c  */
#line 1996 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 333:

/* Line 1455 of yacc.c  */
#line 1997 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 1998 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 335:

/* Line 1455 of yacc.c  */
#line 1999 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 336:

/* Line 1455 of yacc.c  */
#line 2000 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 337:

/* Line 1455 of yacc.c  */
#line 2003 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 2004 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 2007 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_STRING;}
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 2008 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNICODE_STRING;}
    break;

  case 341:

/* Line 1455 of yacc.c  */
#line 2009 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_OSTREAM; }
    break;

  case 342:

/* Line 1455 of yacc.c  */
#line 2010 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_ISTREAM; }
    break;

  case 343:

/* Line 1455 of yacc.c  */
#line 2011 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 344:

/* Line 1455 of yacc.c  */
#line 2012 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_OBJECT; }
    break;

  case 345:

/* Line 1455 of yacc.c  */
#line 2013 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_QOBJECT; }
    break;

  case 346:

/* Line 1455 of yacc.c  */
#line 2016 "vtkParse.y"
    { typeSig("void"); (yyval.integer) = VTK_PARSE_VOID;}
    break;

  case 347:

/* Line 1455 of yacc.c  */
#line 2017 "vtkParse.y"
    { typeSig("bool"); (yyval.integer) = VTK_PARSE_BOOL;}
    break;

  case 348:

/* Line 1455 of yacc.c  */
#line 2018 "vtkParse.y"
    { typeSig("ssize_t"); (yyval.integer) = VTK_PARSE_SSIZE_T;}
    break;

  case 349:

/* Line 1455 of yacc.c  */
#line 2019 "vtkParse.y"
    { typeSig("size_t"); (yyval.integer) = VTK_PARSE_SIZE_T;}
    break;

  case 350:

/* Line 1455 of yacc.c  */
#line 2020 "vtkParse.y"
    { typeSig("vtkTypeInt8"); (yyval.integer) = VTK_PARSE_INT8; }
    break;

  case 351:

/* Line 1455 of yacc.c  */
#line 2021 "vtkParse.y"
    { typeSig("vtkTypeUInt8"); (yyval.integer) = VTK_PARSE_UINT8; }
    break;

  case 352:

/* Line 1455 of yacc.c  */
#line 2022 "vtkParse.y"
    { typeSig("vtkTypeInt16"); (yyval.integer) = VTK_PARSE_INT16; }
    break;

  case 353:

/* Line 1455 of yacc.c  */
#line 2023 "vtkParse.y"
    { typeSig("vtkTypeUInt16"); (yyval.integer) = VTK_PARSE_UINT16; }
    break;

  case 354:

/* Line 1455 of yacc.c  */
#line 2024 "vtkParse.y"
    { typeSig("vtkTypeInt32"); (yyval.integer) = VTK_PARSE_INT32; }
    break;

  case 355:

/* Line 1455 of yacc.c  */
#line 2025 "vtkParse.y"
    { typeSig("vtkTypeUInt32"); (yyval.integer) = VTK_PARSE_UINT32; }
    break;

  case 356:

/* Line 1455 of yacc.c  */
#line 2026 "vtkParse.y"
    { typeSig("vtkTypeInt64"); (yyval.integer) = VTK_PARSE_INT64; }
    break;

  case 357:

/* Line 1455 of yacc.c  */
#line 2027 "vtkParse.y"
    { typeSig("vtkTypeUInt64"); (yyval.integer) = VTK_PARSE_UINT64; }
    break;

  case 358:

/* Line 1455 of yacc.c  */
#line 2028 "vtkParse.y"
    { typeSig("vtkTypeFloat32"); (yyval.integer) = VTK_PARSE_FLOAT32; }
    break;

  case 359:

/* Line 1455 of yacc.c  */
#line 2029 "vtkParse.y"
    { typeSig("vtkTypeFloat64"); (yyval.integer) = VTK_PARSE_FLOAT64; }
    break;

  case 360:

/* Line 1455 of yacc.c  */
#line 2030 "vtkParse.y"
    { typeSig("vtkIdType"); (yyval.integer) = VTK_PARSE_ID_TYPE;}
    break;

  case 361:

/* Line 1455 of yacc.c  */
#line 2031 "vtkParse.y"
    { typeSig("float"); (yyval.integer) = VTK_PARSE_FLOAT;}
    break;

  case 362:

/* Line 1455 of yacc.c  */
#line 2032 "vtkParse.y"
    { typeSig("double"); (yyval.integer) = VTK_PARSE_DOUBLE;}
    break;

  case 363:

/* Line 1455 of yacc.c  */
#line 2033 "vtkParse.y"
    { typeSig("long double"); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 364:

/* Line 1455 of yacc.c  */
#line 2034 "vtkParse.y"
    { typeSig("char"); (yyval.integer) = VTK_PARSE_CHAR;}
    break;

  case 365:

/* Line 1455 of yacc.c  */
#line 2035 "vtkParse.y"
    { typeSig("signed char"); (yyval.integer) = VTK_PARSE_SIGNED_CHAR;}
    break;

  case 366:

/* Line 1455 of yacc.c  */
#line 2037 "vtkParse.y"
    { typeSig("unsigned char"); (yyval.integer) = VTK_PARSE_UNSIGNED_CHAR;}
    break;

  case 367:

/* Line 1455 of yacc.c  */
#line 2038 "vtkParse.y"
    { typeSig("int"); (yyval.integer) = VTK_PARSE_INT;}
    break;

  case 368:

/* Line 1455 of yacc.c  */
#line 2040 "vtkParse.y"
    { typeSig("unsigned int"); (yyval.integer) = VTK_PARSE_UNSIGNED_INT;}
    break;

  case 369:

/* Line 1455 of yacc.c  */
#line 2041 "vtkParse.y"
    { typeSig("short"); (yyval.integer) = VTK_PARSE_SHORT;}
    break;

  case 370:

/* Line 1455 of yacc.c  */
#line 2043 "vtkParse.y"
    { typeSig("unsigned short"); (yyval.integer) = VTK_PARSE_UNSIGNED_SHORT;}
    break;

  case 371:

/* Line 1455 of yacc.c  */
#line 2044 "vtkParse.y"
    { typeSig("long"); (yyval.integer) = VTK_PARSE_LONG;}
    break;

  case 372:

/* Line 1455 of yacc.c  */
#line 2046 "vtkParse.y"
    { typeSig("unsigned long"); (yyval.integer) = VTK_PARSE_UNSIGNED_LONG;}
    break;

  case 373:

/* Line 1455 of yacc.c  */
#line 2047 "vtkParse.y"
    { typeSig("long long"); (yyval.integer) = VTK_PARSE_LONG_LONG;}
    break;

  case 374:

/* Line 1455 of yacc.c  */
#line 2049 "vtkParse.y"
    {typeSig("unsigned long long");(yyval.integer)=VTK_PARSE_UNSIGNED_LONG_LONG;}
    break;

  case 375:

/* Line 1455 of yacc.c  */
#line 2050 "vtkParse.y"
    { typeSig("__int64"); (yyval.integer) = VTK_PARSE___INT64;}
    break;

  case 376:

/* Line 1455 of yacc.c  */
#line 2052 "vtkParse.y"
    { typeSig("unsigned __int64"); (yyval.integer) = VTK_PARSE_UNSIGNED___INT64;}
    break;

  case 377:

/* Line 1455 of yacc.c  */
#line 2053 "vtkParse.y"
    { typeSig("int"); (yyval.integer) = VTK_PARSE_INT; }
    break;

  case 378:

/* Line 1455 of yacc.c  */
#line 2054 "vtkParse.y"
    { typeSig("unsigned int"); (yyval.integer) = VTK_PARSE_UNSIGNED_INT; }
    break;

  case 379:

/* Line 1455 of yacc.c  */
#line 2060 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 380:

/* Line 1455 of yacc.c  */
#line 2061 "vtkParse.y"
    { postSig("{ "); }
    break;

  case 381:

/* Line 1455 of yacc.c  */
#line 2062 "vtkParse.y"
    {
          postSig("}");
          (yyval.str) = vtkstrcat4("{ ", (yyvsp[(3) - (6)].str), (yyvsp[(4) - (6)].str), " }");
        }
    break;

  case 384:

/* Line 1455 of yacc.c  */
#line 2069 "vtkParse.y"
    {(yyval.str) = "";}
    break;

  case 385:

/* Line 1455 of yacc.c  */
#line 2070 "vtkParse.y"
    { postSig(", "); }
    break;

  case 386:

/* Line 1455 of yacc.c  */
#line 2071 "vtkParse.y"
    {
          (yyval.str) = vtkstrcat3((yyvsp[(1) - (4)].str), ", ", (yyvsp[(4) - (4)].str));
        }
    break;

  case 387:

/* Line 1455 of yacc.c  */
#line 2075 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 388:

/* Line 1455 of yacc.c  */
#line 2076 "vtkParse.y"
    {postSig("+");}
    break;

  case 389:

/* Line 1455 of yacc.c  */
#line 2076 "vtkParse.y"
    {(yyval.str) = (yyvsp[(3) - (3)].str);}
    break;

  case 390:

/* Line 1455 of yacc.c  */
#line 2077 "vtkParse.y"
    {postSig("-");}
    break;

  case 391:

/* Line 1455 of yacc.c  */
#line 2078 "vtkParse.y"
    {
             (yyval.str) = vtkstrcat("-", (yyvsp[(3) - (3)].str));
             }
    break;

  case 392:

/* Line 1455 of yacc.c  */
#line 2081 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 393:

/* Line 1455 of yacc.c  */
#line 2082 "vtkParse.y"
    {postSig("(");}
    break;

  case 394:

/* Line 1455 of yacc.c  */
#line 2082 "vtkParse.y"
    {postSig(")"); (yyval.str) = (yyvsp[(3) - (4)].str);}
    break;

  case 395:

/* Line 1455 of yacc.c  */
#line 2083 "vtkParse.y"
    {postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 396:

/* Line 1455 of yacc.c  */
#line 2085 "vtkParse.y"
    {
             chopSig();
             if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
             postSig(">(");
             }
    break;

  case 397:

/* Line 1455 of yacc.c  */
#line 2091 "vtkParse.y"
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

  case 398:

/* Line 1455 of yacc.c  */
#line 2105 "vtkParse.y"
    { (yyval.str) = "static_cast"; }
    break;

  case 399:

/* Line 1455 of yacc.c  */
#line 2106 "vtkParse.y"
    { (yyval.str) = "const_cast"; }
    break;

  case 400:

/* Line 1455 of yacc.c  */
#line 2107 "vtkParse.y"
    { (yyval.str) = "dynamic_cast"; }
    break;

  case 401:

/* Line 1455 of yacc.c  */
#line 2108 "vtkParse.y"
    { (yyval.str) = "reinterpret_cast"; }
    break;

  case 402:

/* Line 1455 of yacc.c  */
#line 2110 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 403:

/* Line 1455 of yacc.c  */
#line 2112 "vtkParse.y"
    { (yyval.str) = vtkstrcat((yyvsp[(1) - (2)].str), (yyvsp[(2) - (2)].str)); }
    break;

  case 404:

/* Line 1455 of yacc.c  */
#line 2114 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 405:

/* Line 1455 of yacc.c  */
#line 2115 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 406:

/* Line 1455 of yacc.c  */
#line 2116 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 407:

/* Line 1455 of yacc.c  */
#line 2117 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 408:

/* Line 1455 of yacc.c  */
#line 2118 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 409:

/* Line 1455 of yacc.c  */
#line 2119 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 410:

/* Line 1455 of yacc.c  */
#line 2121 "vtkParse.y"
    { (yyval.str) = vtkstrdup(add_const_scope((yyvsp[(1) - (1)].str)));
              postSig((yyvsp[(1) - (1)].str)); }
    break;

  case 411:

/* Line 1455 of yacc.c  */
#line 2131 "vtkParse.y"
    {preSig("void Set"); postSig("(");}
    break;

  case 412:

/* Line 1455 of yacc.c  */
#line 2132 "vtkParse.y"
    {
   postSig("a);");
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (7)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, (yyvsp[(6) - (7)].integer), getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 413:

/* Line 1455 of yacc.c  */
#line 2140 "vtkParse.y"
    {postSig("Get");}
    break;

  case 414:

/* Line 1455 of yacc.c  */
#line 2141 "vtkParse.y"
    {markSig();}
    break;

  case 415:

/* Line 1455 of yacc.c  */
#line 2141 "vtkParse.y"
    {swapSig();}
    break;

  case 416:

/* Line 1455 of yacc.c  */
#line 2142 "vtkParse.y"
    {
   postSig("();");
   currentFunction->Name = vtkstrcat("Get", (yyvsp[(4) - (9)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, (yyvsp[(7) - (9)].integer), getTypeId(), 0);
   output_function();
   }
    break;

  case 417:

/* Line 1455 of yacc.c  */
#line 2149 "vtkParse.y"
    {preSig("void Set");}
    break;

  case 418:

/* Line 1455 of yacc.c  */
#line 2150 "vtkParse.y"
    {
   postSig("(char *);");
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(4) - (5)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 419:

/* Line 1455 of yacc.c  */
#line 2158 "vtkParse.y"
    {preSig("char *Get");}
    break;

  case 420:

/* Line 1455 of yacc.c  */
#line 2159 "vtkParse.y"
    {
   postSig("();");
   currentFunction->Name = vtkstrcat("Get", (yyvsp[(4) - (5)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   output_function();
   }
    break;

  case 421:

/* Line 1455 of yacc.c  */
#line 2166 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 422:

/* Line 1455 of yacc.c  */
#line 2166 "vtkParse.y"
    {closeSig();}
    break;

  case 423:

/* Line 1455 of yacc.c  */
#line 2168 "vtkParse.y"
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

  case 424:

/* Line 1455 of yacc.c  */
#line 2196 "vtkParse.y"
    {preSig("void Set"); postSig("("); }
    break;

  case 425:

/* Line 1455 of yacc.c  */
#line 2197 "vtkParse.y"
    {
   postSig("*);");
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (7)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 426:

/* Line 1455 of yacc.c  */
#line 2205 "vtkParse.y"
    {postSig("*Get");}
    break;

  case 427:

/* Line 1455 of yacc.c  */
#line 2206 "vtkParse.y"
    {markSig();}
    break;

  case 428:

/* Line 1455 of yacc.c  */
#line 2206 "vtkParse.y"
    {swapSig();}
    break;

  case 429:

/* Line 1455 of yacc.c  */
#line 2207 "vtkParse.y"
    {
   postSig("();");
   currentFunction->Name = vtkstrcat("Get", (yyvsp[(4) - (9)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   output_function();
   }
    break;

  case 430:

/* Line 1455 of yacc.c  */
#line 2215 "vtkParse.y"
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

  case 431:

/* Line 1455 of yacc.c  */
#line 2230 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 432:

/* Line 1455 of yacc.c  */
#line 2231 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 2);
   }
    break;

  case 433:

/* Line 1455 of yacc.c  */
#line 2235 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 434:

/* Line 1455 of yacc.c  */
#line 2236 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 2);
   }
    break;

  case 435:

/* Line 1455 of yacc.c  */
#line 2240 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 436:

/* Line 1455 of yacc.c  */
#line 2241 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 3);
   }
    break;

  case 437:

/* Line 1455 of yacc.c  */
#line 2245 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 438:

/* Line 1455 of yacc.c  */
#line 2246 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 3);
   }
    break;

  case 439:

/* Line 1455 of yacc.c  */
#line 2250 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 440:

/* Line 1455 of yacc.c  */
#line 2251 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 4);
   }
    break;

  case 441:

/* Line 1455 of yacc.c  */
#line 2255 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 442:

/* Line 1455 of yacc.c  */
#line 2256 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 4);
   }
    break;

  case 443:

/* Line 1455 of yacc.c  */
#line 2260 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 444:

/* Line 1455 of yacc.c  */
#line 2261 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 6);
   }
    break;

  case 445:

/* Line 1455 of yacc.c  */
#line 2265 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 446:

/* Line 1455 of yacc.c  */
#line 2266 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 6);
   }
    break;

  case 447:

/* Line 1455 of yacc.c  */
#line 2270 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 448:

/* Line 1455 of yacc.c  */
#line 2272 "vtkParse.y"
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

  case 449:

/* Line 1455 of yacc.c  */
#line 2286 "vtkParse.y"
    {startSig();}
    break;

  case 450:

/* Line 1455 of yacc.c  */
#line 2288 "vtkParse.y"
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

  case 451:

/* Line 1455 of yacc.c  */
#line 2300 "vtkParse.y"
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

  case 452:

/* Line 1455 of yacc.c  */
#line 2333 "vtkParse.y"
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

  case 453:

/* Line 1455 of yacc.c  */
#line 2367 "vtkParse.y"
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

  case 454:

/* Line 1455 of yacc.c  */
#line 2419 "vtkParse.y"
    { (yyval.str) = "operator()"; }
    break;

  case 455:

/* Line 1455 of yacc.c  */
#line 2420 "vtkParse.y"
    { (yyval.str) = "operator[]"; }
    break;

  case 456:

/* Line 1455 of yacc.c  */
#line 2421 "vtkParse.y"
    { (yyval.str) = "operator new[]"; }
    break;

  case 457:

/* Line 1455 of yacc.c  */
#line 2422 "vtkParse.y"
    { (yyval.str) = "operator delete[]"; }
    break;

  case 459:

/* Line 1455 of yacc.c  */
#line 2425 "vtkParse.y"
    { (yyval.str) = "operator="; }
    break;

  case 460:

/* Line 1455 of yacc.c  */
#line 2426 "vtkParse.y"
    { (yyval.str) = "operator*"; }
    break;

  case 461:

/* Line 1455 of yacc.c  */
#line 2426 "vtkParse.y"
    { (yyval.str) = "operator/"; }
    break;

  case 462:

/* Line 1455 of yacc.c  */
#line 2427 "vtkParse.y"
    { (yyval.str) = "operator-"; }
    break;

  case 463:

/* Line 1455 of yacc.c  */
#line 2427 "vtkParse.y"
    { (yyval.str) = "operator+"; }
    break;

  case 464:

/* Line 1455 of yacc.c  */
#line 2428 "vtkParse.y"
    { (yyval.str) = "operator!"; }
    break;

  case 465:

/* Line 1455 of yacc.c  */
#line 2428 "vtkParse.y"
    { (yyval.str) = "operator~"; }
    break;

  case 466:

/* Line 1455 of yacc.c  */
#line 2429 "vtkParse.y"
    { (yyval.str) = "operator,"; }
    break;

  case 467:

/* Line 1455 of yacc.c  */
#line 2429 "vtkParse.y"
    { (yyval.str) = "operator<"; }
    break;

  case 468:

/* Line 1455 of yacc.c  */
#line 2430 "vtkParse.y"
    { (yyval.str) = "operator>"; }
    break;

  case 469:

/* Line 1455 of yacc.c  */
#line 2430 "vtkParse.y"
    { (yyval.str) = "operator&"; }
    break;

  case 470:

/* Line 1455 of yacc.c  */
#line 2431 "vtkParse.y"
    { (yyval.str) = "operator|"; }
    break;

  case 471:

/* Line 1455 of yacc.c  */
#line 2431 "vtkParse.y"
    { (yyval.str) = "operator^"; }
    break;

  case 472:

/* Line 1455 of yacc.c  */
#line 2432 "vtkParse.y"
    { (yyval.str) = "operator%"; }
    break;

  case 473:

/* Line 1455 of yacc.c  */
#line 2433 "vtkParse.y"
    { (yyval.str) = "operator new"; }
    break;

  case 474:

/* Line 1455 of yacc.c  */
#line 2434 "vtkParse.y"
    { (yyval.str) = "operator delete"; }
    break;

  case 475:

/* Line 1455 of yacc.c  */
#line 2435 "vtkParse.y"
    { (yyval.str) = "operator<<="; }
    break;

  case 476:

/* Line 1455 of yacc.c  */
#line 2436 "vtkParse.y"
    { (yyval.str) = "operator>>="; }
    break;

  case 477:

/* Line 1455 of yacc.c  */
#line 2437 "vtkParse.y"
    { (yyval.str) = "operator<<"; }
    break;

  case 478:

/* Line 1455 of yacc.c  */
#line 2438 "vtkParse.y"
    { (yyval.str) = "operator>>"; }
    break;

  case 479:

/* Line 1455 of yacc.c  */
#line 2439 "vtkParse.y"
    { (yyval.str) = "operator->*"; }
    break;

  case 480:

/* Line 1455 of yacc.c  */
#line 2440 "vtkParse.y"
    { (yyval.str) = "operator->"; }
    break;

  case 481:

/* Line 1455 of yacc.c  */
#line 2441 "vtkParse.y"
    { (yyval.str) = "operator+="; }
    break;

  case 482:

/* Line 1455 of yacc.c  */
#line 2442 "vtkParse.y"
    { (yyval.str) = "operator-="; }
    break;

  case 483:

/* Line 1455 of yacc.c  */
#line 2443 "vtkParse.y"
    { (yyval.str) = "operator*="; }
    break;

  case 484:

/* Line 1455 of yacc.c  */
#line 2444 "vtkParse.y"
    { (yyval.str) = "operator/="; }
    break;

  case 485:

/* Line 1455 of yacc.c  */
#line 2445 "vtkParse.y"
    { (yyval.str) = "operator%="; }
    break;

  case 486:

/* Line 1455 of yacc.c  */
#line 2446 "vtkParse.y"
    { (yyval.str) = "operator++"; }
    break;

  case 487:

/* Line 1455 of yacc.c  */
#line 2447 "vtkParse.y"
    { (yyval.str) = "operator--"; }
    break;

  case 488:

/* Line 1455 of yacc.c  */
#line 2448 "vtkParse.y"
    { (yyval.str) = "operator&="; }
    break;

  case 489:

/* Line 1455 of yacc.c  */
#line 2449 "vtkParse.y"
    { (yyval.str) = "operator|="; }
    break;

  case 490:

/* Line 1455 of yacc.c  */
#line 2450 "vtkParse.y"
    { (yyval.str) = "operator^="; }
    break;

  case 491:

/* Line 1455 of yacc.c  */
#line 2451 "vtkParse.y"
    {(yyval.str) = "operator&&=";}
    break;

  case 492:

/* Line 1455 of yacc.c  */
#line 2452 "vtkParse.y"
    {(yyval.str) = "operator||=";}
    break;

  case 493:

/* Line 1455 of yacc.c  */
#line 2453 "vtkParse.y"
    { (yyval.str) = "operator&&"; }
    break;

  case 494:

/* Line 1455 of yacc.c  */
#line 2454 "vtkParse.y"
    { (yyval.str) = "operator||"; }
    break;

  case 495:

/* Line 1455 of yacc.c  */
#line 2455 "vtkParse.y"
    { (yyval.str) = "operator=="; }
    break;

  case 496:

/* Line 1455 of yacc.c  */
#line 2456 "vtkParse.y"
    { (yyval.str) = "operator!="; }
    break;

  case 497:

/* Line 1455 of yacc.c  */
#line 2457 "vtkParse.y"
    { (yyval.str) = "operator<="; }
    break;

  case 498:

/* Line 1455 of yacc.c  */
#line 2458 "vtkParse.y"
    { (yyval.str) = "operator>="; }
    break;



/* Line 1455 of yacc.c  */
#line 7694 "vtkParse.tab.c"
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
#line 2483 "vtkParse.y"

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

/* for a macro constant, guess the constant type, doesn't do any math */
unsigned int guess_constant_type(const char *valstring)
{
  unsigned int valtype = 0;
  size_t k;
  int i;

  if (valstring == NULL || valstring[0] == '\0')
    {
    valtype = 0;
    }
  else if (strcmp(valstring, "true") == 0 || strcmp(valstring, "false") == 0)
    {
    valtype = VTK_PARSE_BOOL;
    }
  else if (valstring[0] == '_' ||
           (valstring[0] >= 'a' && valstring[0] <= 'z') ||
           (valstring[0] >= 'A' && valstring[0] <= 'Z'))
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
        valtype = scope->Constants[i]->Type;
        }
      }
    }
  else if (valstring[0] == '\"')
    {
    valtype = VTK_PARSE_CHAR_PTR;
    }
  else if (valstring[0] == '\'')
    {
    valtype = VTK_PARSE_CHAR;
    }
  else if (valstring[0] == '-' || valstring[0] == '+' ||
           (valstring[0] >= '0' && valstring[0] <= '9'))
    {
    k = 0;
    if (valstring[0] == '-' || valstring[0] == '+')
      {
      k++;
      while (valstring[k] == ' ' || valstring[k] == '\t') { k++; }
      }
    if (valstring[k] >= '0' && valstring[k] <= '9')
      {
     /* guess "int" first */
      valtype = VTK_PARSE_INT;

      if (valstring[k+1] == 'x' || valstring[k+1] == 'X')
        {
        k += 2;
        while ((valstring[k] >= '0' && valstring[k] <= '9') ||
               (valstring[k] >= 'a' && valstring[k] <= 'f') ||
               (valstring[k] >= 'A' && valstring[k] <= 'F'))
          {
          k++;
          }
        }
      else
        {
        while ((valstring[k] >= '0' && valstring[k] <= '9') ||
               valstring[k] == '.' ||
               valstring[k] == 'e' || valstring[k] == 'E' ||
               valstring[k] == '-' || valstring[k] == '+')
          {
          if (valstring[k] == '.' ||
              valstring[k] == 'e' || valstring[k] == 'E')
            {
            valtype = VTK_PARSE_DOUBLE;
            }
          k++;
          }
        }

      /* look for type suffixes */
      if (valtype == VTK_PARSE_DOUBLE)
        {
        if (valstring[k] == 'f')
          {
          valtype = VTK_PARSE_FLOAT;
          }
        }
      else
        {
        while (valstring[k] != '\0')
          {
          if (valstring[k] == 'u' || valstring[k] == 'U')
            {
            valtype = (valtype | VTK_PARSE_UNSIGNED);
            }
          else if (valstring[k] == 'l' || valstring[k] == 'L')
            {
            if (valstring[k+1] == 'l' || valstring[k+1] == 'L')
              {
              k++;
              valtype = ((valtype & ~VTK_PARSE_UNSIGNED) | VTK_PARSE_LONG_LONG);
              }
            else
              {
              valtype = ((valtype & ~VTK_PARSE_UNSIGNED) | VTK_PARSE_LONG);
              }
            }
          else if (valstring[k] == 'i' && valstring[k+1] == '6' &&
                   valstring[k+2] == '4')
            {
            k += 2;
            valtype = ((valtype & ~VTK_PARSE_UNSIGNED) | VTK_PARSE___INT64);
            }
          k++;
          }
        }
      }
    }

  return valtype;
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
