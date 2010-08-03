
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
     QT_ID = 348,
     StdString = 349,
     UnicodeString = 350,
     IdType = 351,
     TypeInt8 = 352,
     TypeUInt8 = 353,
     TypeInt16 = 354,
     TypeUInt16 = 355,
     TypeInt32 = 356,
     TypeUInt32 = 357,
     TypeInt64 = 358,
     TypeUInt64 = 359,
     TypeFloat32 = 360,
     TypeFloat64 = 361,
     SetMacro = 362,
     GetMacro = 363,
     SetStringMacro = 364,
     GetStringMacro = 365,
     SetClampMacro = 366,
     SetObjectMacro = 367,
     GetObjectMacro = 368,
     BooleanMacro = 369,
     SetVector2Macro = 370,
     SetVector3Macro = 371,
     SetVector4Macro = 372,
     SetVector6Macro = 373,
     GetVector2Macro = 374,
     GetVector3Macro = 375,
     GetVector4Macro = 376,
     GetVector6Macro = 377,
     SetVectorMacro = 378,
     GetVectorMacro = 379,
     ViewportCoordinateMacro = 380,
     WorldCoordinateMacro = 381,
     TypeMacro = 382,
     VTK_BYTE_SWAP_DECL = 383
   };
#endif
/* Tokens.  */
#define STRUCT 258
#define CLASS 259
#define PUBLIC 260
#define PRIVATE 261
#define PROTECTED 262
#define VIRTUAL 263
#define ID 264
#define STRING_LITERAL 265
#define INT_LITERAL 266
#define HEX_LITERAL 267
#define OCT_LITERAL 268
#define FLOAT_LITERAL 269
#define CHAR_LITERAL 270
#define ZERO 271
#define FLOAT 272
#define DOUBLE 273
#define LONG_DOUBLE 274
#define INT 275
#define UNSIGNED_INT 276
#define SHORT 277
#define UNSIGNED_SHORT 278
#define LONG 279
#define UNSIGNED_LONG 280
#define LONG_LONG 281
#define UNSIGNED_LONG_LONG 282
#define INT64__ 283
#define UNSIGNED_INT64__ 284
#define CHAR 285
#define SIGNED_CHAR 286
#define UNSIGNED_CHAR 287
#define VOID 288
#define BOOL 289
#define SSIZE_T 290
#define SIZE_T 291
#define OSTREAM 292
#define ISTREAM 293
#define ENUM 294
#define UNION 295
#define CLASS_REF 296
#define OTHER 297
#define CONST 298
#define CONST_PTR 299
#define CONST_EQUAL 300
#define OPERATOR 301
#define UNSIGNED 302
#define SIGNED 303
#define FRIEND 304
#define INLINE 305
#define MUTABLE 306
#define TEMPLATE 307
#define TYPENAME 308
#define TYPEDEF 309
#define NAMESPACE 310
#define USING 311
#define VTK_ID 312
#define STATIC 313
#define EXTERN 314
#define VAR_FUNCTION 315
#define VTK_LEGACY 316
#define NEW 317
#define DELETE 318
#define EXPLICIT 319
#define OP_LSHIFT_EQ 320
#define OP_RSHIFT_EQ 321
#define OP_LSHIFT 322
#define OP_RSHIFT 323
#define OP_ARROW_POINTER 324
#define OP_ARROW 325
#define OP_INCR 326
#define OP_DECR 327
#define OP_PLUS_EQ 328
#define OP_MINUS_EQ 329
#define OP_TIMES_EQ 330
#define OP_DIVIDE_EQ 331
#define OP_REMAINDER_EQ 332
#define OP_AND_EQ 333
#define OP_OR_EQ 334
#define OP_XOR_EQ 335
#define OP_LOGIC_AND_EQ 336
#define OP_LOGIC_OR_EQ 337
#define OP_LOGIC_AND 338
#define OP_LOGIC_OR 339
#define OP_LOGIC_EQ 340
#define OP_LOGIC_NEQ 341
#define OP_LOGIC_LEQ 342
#define OP_LOGIC_GEQ 343
#define ELLIPSIS 344
#define DOUBLE_COLON 345
#define LP 346
#define LA 347
#define QT_ID 348
#define StdString 349
#define UnicodeString 350
#define IdType 351
#define TypeInt8 352
#define TypeUInt8 353
#define TypeInt16 354
#define TypeUInt16 355
#define TypeInt32 356
#define TypeUInt32 357
#define TypeInt64 358
#define TypeUInt64 359
#define TypeFloat32 360
#define TypeFloat64 361
#define SetMacro 362
#define GetMacro 363
#define SetStringMacro 364
#define GetStringMacro 365
#define SetClampMacro 366
#define SetObjectMacro 367
#define GetObjectMacro 368
#define BooleanMacro 369
#define SetVector2Macro 370
#define SetVector3Macro 371
#define SetVector4Macro 372
#define SetVector6Macro 373
#define GetVector2Macro 374
#define GetVector3Macro 375
#define GetVector4Macro 376
#define GetVector6Macro 377
#define SetVectorMacro 378
#define GetVectorMacro 379
#define ViewportCoordinateMacro 380
#define WorldCoordinateMacro 381
#define TypeMacro 382
#define VTK_BYTE_SWAP_DECL 383




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 1117 "vtkParse.y"

  const char   *str;
  unsigned int  integer;



/* Line 214 of yacc.c  */
#line 1469 "vtkParse.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 1481 "vtkParse.tab.c"

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
#define YYLAST   5866

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  152
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  190
/* YYNRULES -- Number of rules.  */
#define YYNRULES  553
/* YYNRULES -- Number of states.  */
#define YYNSTATES  972

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   383

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   150,     2,     2,     2,   144,   145,     2,
     130,   131,   142,   140,   137,   139,   151,   143,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   136,   129,
     134,   138,   135,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   148,     2,   149,   147,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   132,   146,   133,   141,     2,     2,     2,
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
     125,   126,   127,   128
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
    1130,  1132,  1134,  1136,  1138,  1140,  1142,  1144,  1146,  1148,
    1149,  1157,  1158,  1159,  1160,  1170,  1171,  1177,  1178,  1184,
    1185,  1186,  1197,  1198,  1206,  1207,  1208,  1209,  1219,  1226,
    1227,  1235,  1236,  1244,  1245,  1253,  1254,  1262,  1263,  1271,
    1272,  1280,  1281,  1289,  1290,  1298,  1299,  1309,  1310,  1320,
    1325,  1330,  1338,  1341,  1344,  1348,  1352,  1354,  1356,  1358,
    1360,  1362,  1364,  1366,  1368,  1370,  1372,  1374,  1376,  1378,
    1380,  1382,  1384,  1386,  1388,  1390,  1392,  1394,  1396,  1398,
    1400,  1402,  1404,  1406,  1408,  1410,  1412,  1414,  1416,  1418,
    1420,  1422,  1424,  1426,  1428,  1430,  1432,  1434,  1435,  1438,
    1439,  1442,  1444,  1446,  1448,  1450,  1452,  1454,  1456,  1458,
    1460,  1462,  1464,  1466,  1468,  1470,  1472,  1474,  1476,  1478,
    1480,  1482,  1484,  1486,  1488,  1490,  1492,  1494,  1496,  1498,
    1500,  1502,  1504,  1506,  1508,  1510,  1512,  1514,  1516,  1518,
    1520,  1522,  1524,  1526,  1528,  1530,  1532,  1534,  1536,  1538,
    1542,  1546,  1550,  1554
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     153,     0,    -1,    -1,    -1,   153,   154,   155,    -1,   246,
      -1,   171,   248,   129,    -1,   184,   248,   129,    -1,   185,
      -1,   157,    -1,   156,    -1,   189,    -1,   159,   248,   129,
      -1,   191,   159,   248,   129,    -1,    41,    -1,   208,   220,
      -1,   191,   208,   220,    -1,   207,   220,    -1,   202,   220,
      -1,   203,   220,    -1,   191,   202,   220,    -1,   200,   220,
      -1,   310,    -1,   289,   129,    -1,     9,   130,   335,   131,
      -1,    57,   130,   335,   131,    -1,    93,   130,   335,   131,
      -1,   129,    -1,    59,    10,   132,   153,   133,    -1,    -1,
      55,   289,   158,   132,   153,   133,    -1,    55,   132,   335,
     133,    -1,    -1,     4,   273,   160,   167,   132,   164,   133,
      -1,    -1,     4,   273,   134,   285,   135,   161,   167,   132,
     164,   133,    -1,    -1,     3,   273,   162,   167,   132,   164,
     133,    -1,    -1,     3,   273,   134,   285,   135,   163,   167,
     132,   164,   133,    -1,     3,   132,   335,   133,    -1,    -1,
      -1,   164,   165,   166,    -1,   164,   170,   136,    -1,   246,
      -1,   171,   248,   129,    -1,   184,   248,   129,    -1,   185,
      -1,   189,    -1,   187,    -1,    49,   187,    -1,   186,    -1,
      41,    -1,   208,   220,    -1,    49,   208,   220,    -1,   191,
     208,   220,    -1,   205,   220,    -1,    49,   205,   220,    -1,
     191,   205,   220,    -1,   201,   220,    -1,   128,   130,   335,
     131,   129,    -1,   310,    -1,   129,    -1,    -1,   136,   168,
      -1,   169,    -1,   169,   137,   168,    -1,   287,    -1,     6,
     287,    -1,     7,   287,    -1,     5,   287,    -1,     5,    -1,
       6,    -1,     7,    -1,    -1,    39,   273,   172,   132,   174,
     133,    -1,    -1,    39,   173,   132,   174,   133,    -1,    -1,
     175,    -1,   175,   137,   174,    -1,   273,    -1,   273,   138,
     178,    -1,   177,    -1,   273,    -1,   288,    -1,   281,    -1,
      16,    -1,    11,    -1,    13,    -1,    12,    -1,    15,    -1,
     176,    -1,    -1,   182,   179,   178,    -1,    -1,   176,   183,
     180,   178,    -1,    -1,   130,   181,   178,   131,    -1,   139,
      -1,   140,    -1,   141,    -1,   139,    -1,   140,    -1,   142,
      -1,   143,    -1,   144,    -1,   145,    -1,   146,    -1,   147,
      -1,    40,   273,   132,   335,   133,    -1,    40,   132,   335,
     133,    -1,    56,   336,   129,    -1,   191,   187,    -1,     4,
     273,   188,    -1,     3,   273,   188,    -1,     3,   188,    -1,
     129,    -1,   132,   335,   133,   336,   129,    -1,   136,   336,
     129,    -1,   190,   276,   256,   129,    -1,   190,   159,   242,
     129,    -1,   190,   171,   242,   129,    -1,   190,   184,   242,
     129,    -1,   190,    60,   129,    -1,    54,    -1,    52,   134,
     135,    -1,    -1,    52,   134,   192,   193,   135,    -1,   195,
      -1,    -1,   195,   137,   194,   193,    -1,   294,   198,    -1,
     197,   198,    -1,    -1,   196,   191,   198,    -1,     4,    -1,
      53,    -1,    -1,    -1,   273,   199,   243,    -1,    61,   130,
     202,   131,    -1,    61,   130,   205,   131,    -1,   274,   217,
      -1,   274,   204,   217,    -1,   289,    90,   141,   232,    -1,
      50,   289,    90,   141,   232,    -1,   289,    90,   225,    -1,
      50,   289,    90,   225,    -1,   289,    90,   289,    90,   141,
     232,    -1,    50,   289,    90,   289,    90,   141,   232,    -1,
     289,    90,   289,    90,   225,    -1,    50,   289,    90,   289,
      90,   225,    -1,   289,    90,    -1,   204,   289,    90,    -1,
     141,   232,    -1,    50,   141,   232,    -1,     8,   141,   232,
      -1,   225,    -1,    50,   225,    -1,   206,   225,    -1,   274,
     217,    -1,     8,   274,   217,    -1,    64,    -1,    50,    64,
      -1,    64,    50,    -1,   289,    90,   209,    -1,   274,   204,
     212,    -1,   209,    -1,   274,   212,    -1,     8,   276,   212,
      -1,    -1,    -1,    46,   274,   130,   210,   235,   131,   211,
     219,    -1,    -1,   214,   213,   219,    -1,    -1,    -1,    46,
     333,   215,   130,   216,   235,   131,    -1,    -1,   221,   218,
     219,    -1,    -1,   138,    16,    -1,    45,    16,    -1,    43,
      -1,   129,    -1,   132,   335,   133,    -1,    -1,   273,   130,
     222,   235,   131,    -1,    -1,    -1,   273,   134,   223,   285,
     135,   130,   224,   235,   131,    -1,    -1,   227,   226,   229,
      -1,    -1,   273,   130,   228,   235,   131,    -1,    -1,   136,
     231,   230,    -1,    -1,   137,   231,   230,    -1,   273,   130,
     335,   131,    -1,   233,    -1,    -1,   273,   130,   234,   235,
     131,    -1,    -1,    -1,   236,   237,    -1,    89,    -1,   239,
      -1,    -1,   239,   137,   238,   237,    -1,    -1,    -1,   240,
     276,   254,   241,   243,    -1,    60,    -1,   273,    -1,   290,
     273,    -1,    -1,   244,    -1,    -1,   138,   245,   297,    -1,
     274,   247,   249,   129,    -1,    58,    60,   249,   129,    -1,
      60,   249,   129,    -1,   256,   243,    -1,    -1,   251,   249,
      -1,    -1,    -1,   249,   137,   250,   251,    -1,    -1,   252,
     247,    -1,    -1,   290,   253,   247,    -1,   264,   266,    -1,
      -1,   258,   262,   131,   255,   260,    -1,   265,   266,    -1,
      -1,   259,   263,   131,   257,   260,    -1,   130,    -1,    91,
      -1,    92,    -1,    91,    -1,    92,    -1,    -1,    -1,   130,
     261,   235,   131,    -1,   267,    -1,   254,    -1,   290,   254,
      -1,   256,    -1,   290,   256,    -1,    -1,   265,    -1,   273,
      -1,    -1,   267,    -1,    -1,   268,   269,    -1,    -1,   271,
     148,   270,   272,   149,    -1,    -1,   269,    -1,    -1,   178,
      -1,    57,    -1,    93,    -1,     9,    -1,    38,    -1,    37,
      -1,    94,    -1,    95,    -1,   276,    -1,    51,   276,    -1,
      59,   276,    -1,    59,    10,   276,    -1,    50,   276,    -1,
     275,   276,    -1,    50,   275,   276,    -1,    58,    -1,    58,
      50,    -1,   277,    -1,   277,   290,    -1,   279,    -1,   278,
     279,    -1,   279,   278,    -1,    43,    -1,   293,    -1,   281,
      -1,   288,    -1,    -1,    53,   280,   287,    -1,    -1,    57,
     134,   282,   285,   135,    -1,    -1,     9,   134,   283,   285,
     135,    -1,    -1,    93,   134,   284,   285,   135,    -1,   276,
      -1,    -1,   276,   137,   286,   285,    -1,    57,    -1,     9,
      -1,    93,    -1,    38,    -1,    37,    -1,    94,    -1,    95,
      -1,   281,    -1,   288,    -1,   289,    90,   287,    -1,   281,
      90,   287,    -1,     9,    -1,    93,    -1,    57,    -1,    38,
      -1,    37,    -1,    94,    -1,    95,    -1,   145,    -1,   291,
     145,    -1,   291,    -1,   292,    -1,   291,   292,    -1,   142,
      -1,    44,    -1,   294,    -1,     4,   295,    -1,     3,   295,
      -1,    40,     9,    -1,    40,    57,    -1,    40,    93,    -1,
      39,     9,    -1,    39,    57,    -1,    39,    93,    -1,   296,
      -1,   295,    -1,    94,    -1,    95,    -1,    37,    -1,    38,
      -1,     9,    -1,    57,    -1,    93,    -1,    33,    -1,    34,
      -1,    35,    -1,    36,    -1,    97,    -1,    98,    -1,    99,
      -1,   100,    -1,   101,    -1,   102,    -1,   103,    -1,   104,
      -1,   105,    -1,   106,    -1,    96,    -1,    17,    -1,    18,
      -1,    19,    -1,    30,    -1,    31,    -1,    32,    -1,    20,
      -1,    21,    -1,    22,    -1,    23,    -1,    24,    -1,    25,
      -1,    26,    -1,    27,    -1,    28,    -1,    29,    -1,    48,
      -1,    47,    -1,   302,    -1,    -1,   132,   298,   297,   300,
     299,   133,    -1,    -1,   137,    -1,    -1,    -1,   300,   137,
     301,   297,    -1,   309,    -1,    -1,   140,   303,   309,    -1,
      -1,   139,   304,   309,    -1,   308,    -1,    -1,   130,   305,
     302,   131,    -1,    -1,    -1,     9,   134,   306,   277,   135,
     130,   307,   309,   131,    -1,    10,    -1,   308,    10,    -1,
      16,    -1,    11,    -1,    13,    -1,    12,    -1,    14,    -1,
      15,    -1,     9,    -1,    57,    -1,    93,    -1,    -1,   107,
     130,   273,   137,   311,   276,   131,    -1,    -1,    -1,    -1,
     108,   130,   312,   273,   137,   313,   276,   314,   131,    -1,
      -1,   109,   130,   315,   273,   131,    -1,    -1,   110,   130,
     316,   273,   131,    -1,    -1,    -1,   111,   130,   273,   137,
     317,   293,   318,   137,   336,   131,    -1,    -1,   112,   130,
     273,   137,   319,   293,   131,    -1,    -1,    -1,    -1,   113,
     130,   320,   273,   137,   321,   293,   322,   131,    -1,   114,
     130,   273,   137,   293,   131,    -1,    -1,   115,   130,   273,
     137,   323,   293,   131,    -1,    -1,   119,   130,   273,   137,
     324,   293,   131,    -1,    -1,   116,   130,   273,   137,   325,
     293,   131,    -1,    -1,   120,   130,   273,   137,   326,   293,
     131,    -1,    -1,   117,   130,   273,   137,   327,   293,   131,
      -1,    -1,   121,   130,   273,   137,   328,   293,   131,    -1,
      -1,   118,   130,   273,   137,   329,   293,   131,    -1,    -1,
     122,   130,   273,   137,   330,   293,   131,    -1,    -1,   123,
     130,   273,   137,   331,   293,   137,    11,   131,    -1,    -1,
     124,   130,   273,   137,   332,   293,   137,    11,   131,    -1,
     125,   130,   273,   131,    -1,   126,   130,   273,   131,    -1,
     127,   130,   273,   137,   273,   299,   131,    -1,   130,   131,
      -1,   148,   149,    -1,    62,   148,   149,    -1,    63,   148,
     149,    -1,   334,    -1,   138,    -1,   142,    -1,   143,    -1,
     139,    -1,   140,    -1,   150,    -1,   141,    -1,   137,    -1,
     134,    -1,   135,    -1,   145,    -1,   146,    -1,   147,    -1,
     144,    -1,    62,    -1,    63,    -1,    65,    -1,    66,    -1,
      67,    -1,    68,    -1,    69,    -1,    70,    -1,    73,    -1,
      74,    -1,    75,    -1,    76,    -1,    77,    -1,    71,    -1,
      72,    -1,    78,    -1,    79,    -1,    80,    -1,    81,    -1,
      82,    -1,    83,    -1,    84,    -1,    85,    -1,    86,    -1,
      87,    -1,    88,    -1,    -1,   335,   337,    -1,    -1,   336,
     338,    -1,   129,    -1,   338,    -1,    42,    -1,   339,    -1,
     341,    -1,   340,    -1,    54,    -1,   334,    -1,   136,    -1,
     151,    -1,    90,    -1,     4,    -1,    52,    -1,    38,    -1,
      37,    -1,    94,    -1,    95,    -1,   296,    -1,    13,    -1,
      11,    -1,    12,    -1,    14,    -1,    15,    -1,    10,    -1,
      41,    -1,    43,    -1,    44,    -1,    45,    -1,     3,    -1,
      46,    -1,    58,    -1,    50,    -1,     8,    -1,    39,    -1,
      40,    -1,    53,    -1,    16,    -1,    60,    -1,    89,    -1,
       5,    -1,     7,    -1,     6,    -1,    55,    -1,    56,    -1,
      59,    -1,     9,    -1,    57,    -1,    93,    -1,   132,   335,
     133,    -1,   148,   335,   149,    -1,   130,   335,   131,    -1,
      91,   335,   131,    -1,    92,   335,   131,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,  1262,  1262,  1263,  1262,  1267,  1268,  1269,  1270,  1271,
    1272,  1273,  1274,  1275,  1276,  1277,  1278,  1279,  1280,  1281,
    1282,  1283,  1284,  1285,  1286,  1287,  1288,  1289,  1295,  1301,
    1301,  1303,  1309,  1309,  1311,  1311,  1313,  1313,  1315,  1315,
    1317,  1320,  1322,  1321,  1324,  1327,  1328,  1329,  1330,  1331,
    1332,  1333,  1334,  1335,  1336,  1337,  1339,  1340,  1341,  1343,
    1344,  1345,  1346,  1347,  1349,  1349,  1351,  1351,  1353,  1354,
    1355,  1356,  1363,  1364,  1365,  1375,  1375,  1377,  1377,  1380,
    1380,  1380,  1382,  1383,  1385,  1386,  1387,  1387,  1389,  1389,
    1389,  1389,  1389,  1391,  1392,  1392,  1396,  1396,  1400,  1400,
    1405,  1405,  1406,  1408,  1408,  1409,  1409,  1410,  1410,  1411,
    1411,  1417,  1418,  1420,  1422,  1424,  1425,  1426,  1428,  1429,
    1430,  1436,  1459,  1460,  1461,  1462,  1464,  1470,  1471,  1471,
    1477,  1478,  1478,  1481,  1491,  1499,  1499,  1511,  1512,  1514,
    1514,  1514,  1521,  1523,  1529,  1531,  1532,  1533,  1534,  1535,
    1536,  1537,  1538,  1539,  1541,  1542,  1544,  1545,  1546,  1551,
    1552,  1553,  1560,  1561,  1569,  1569,  1569,  1571,  1572,  1575,
    1576,  1577,  1587,  1591,  1586,  1603,  1603,  1612,  1613,  1612,
    1620,  1620,  1629,  1630,  1639,  1649,  1655,  1655,  1658,  1657,
    1662,  1663,  1662,  1670,  1670,  1677,  1677,  1679,  1679,  1681,
    1681,  1683,  1685,  1694,  1694,  1700,  1700,  1700,  1703,  1704,
    1705,  1705,  1708,  1710,  1708,  1739,  1763,  1763,  1765,  1765,
    1767,  1767,  1774,  1775,  1776,  1778,  1789,  1790,  1792,  1793,
    1793,  1796,  1796,  1797,  1797,  1801,  1802,  1802,  1813,  1814,
    1814,  1824,  1825,  1827,  1830,  1832,  1835,  1836,  1836,  1838,
    1841,  1842,  1846,  1847,  1850,  1850,  1852,  1854,  1854,  1856,
    1856,  1858,  1858,  1860,  1860,  1862,  1863,  1869,  1870,  1871,
    1872,  1873,  1874,  1875,  1882,  1883,  1884,  1885,  1887,  1888,
    1890,  1894,  1895,  1897,  1898,  1900,  1901,  1902,  1904,  1906,
    1907,  1909,  1911,  1911,  1915,  1915,  1918,  1918,  1921,  1921,
    1925,  1925,  1925,  1927,  1928,  1929,  1930,  1931,  1932,  1933,
    1934,  1935,  1937,  1942,  1948,  1948,  1948,  1948,  1948,  1948,
    1948,  1964,  1965,  1966,  1971,  1972,  1984,  1985,  1988,  1989,
    1990,  1991,  1992,  1993,  1994,  1995,  1996,  1999,  2000,  2003,
    2004,  2005,  2006,  2007,  2008,  2009,  2012,  2013,  2014,  2015,
    2016,  2017,  2018,  2019,  2020,  2021,  2022,  2023,  2024,  2025,
    2026,  2027,  2028,  2029,  2030,  2031,  2032,  2034,  2035,  2037,
    2038,  2040,  2041,  2043,  2044,  2046,  2047,  2049,  2050,  2056,
    2057,  2057,  2063,  2063,  2065,  2066,  2066,  2071,  2072,  2072,
    2073,  2073,  2077,  2078,  2078,  2079,  2081,  2079,  2101,  2102,
    2105,  2106,  2107,  2108,  2109,  2110,  2111,  2113,  2115,  2125,
    2125,  2134,  2135,  2135,  2134,  2143,  2143,  2152,  2152,  2160,
    2160,  2160,  2190,  2189,  2199,  2200,  2200,  2199,  2208,  2224,
    2224,  2229,  2229,  2234,  2234,  2239,  2239,  2244,  2244,  2249,
    2249,  2254,  2254,  2259,  2259,  2264,  2264,  2280,  2280,  2293,
    2326,  2360,  2413,  2414,  2415,  2416,  2417,  2419,  2420,  2420,
    2421,  2421,  2422,  2422,  2423,  2423,  2424,  2424,  2425,  2425,
    2426,  2427,  2428,  2429,  2430,  2431,  2432,  2433,  2434,  2435,
    2436,  2437,  2438,  2439,  2440,  2441,  2442,  2443,  2444,  2445,
    2446,  2447,  2448,  2449,  2450,  2451,  2452,  2458,  2458,  2459,
    2459,  2461,  2461,  2463,  2463,  2463,  2463,  2463,  2464,  2464,
    2464,  2464,  2464,  2464,  2465,  2465,  2465,  2465,  2465,  2466,
    2466,  2466,  2466,  2466,  2467,  2467,  2467,  2467,  2467,  2467,
    2468,  2468,  2468,  2468,  2468,  2468,  2468,  2469,  2469,  2469,
    2469,  2469,  2469,  2470,  2470,  2470,  2470,  2470,  2470,  2472,
    2473,  2474,  2474,  2474
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
  "$@52", "$@53", "string_literal", "literal2", "macro", "$@54", "$@55",
  "$@56", "$@57", "$@58", "$@59", "$@60", "$@61", "$@62", "$@63", "$@64",
  "$@65", "$@66", "$@67", "$@68", "$@69", "$@70", "$@71", "$@72", "$@73",
  "$@74", "$@75", "op_token", "op_token_no_delim", "maybe_other",
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
     375,   376,   377,   378,   379,   380,   381,   382,   383,    59,
      40,    41,   123,   125,    60,    62,    58,    44,    61,    45,
      43,   126,    42,    47,    37,    38,   124,    94,    91,    93,
      33,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   152,   153,   154,   153,   155,   155,   155,   155,   155,
     155,   155,   155,   155,   155,   155,   155,   155,   155,   155,
     155,   155,   155,   155,   155,   155,   155,   155,   156,   158,
     157,   157,   160,   159,   161,   159,   162,   159,   163,   159,
     159,   164,   165,   164,   164,   166,   166,   166,   166,   166,
     166,   166,   166,   166,   166,   166,   166,   166,   166,   166,
     166,   166,   166,   166,   167,   167,   168,   168,   169,   169,
     169,   169,   170,   170,   170,   172,   171,   173,   171,   174,
     174,   174,   175,   175,   176,   176,   176,   176,   177,   177,
     177,   177,   177,   178,   179,   178,   180,   178,   181,   178,
     182,   182,   182,   183,   183,   183,   183,   183,   183,   183,
     183,   184,   184,   185,   186,   187,   187,   187,   188,   188,
     188,   189,   189,   189,   189,   189,   190,   191,   192,   191,
     193,   194,   193,   195,   195,   196,   195,   197,   197,   198,
     199,   198,   200,   201,   202,   203,   203,   203,   203,   203,
     203,   203,   203,   203,   204,   204,   205,   205,   205,   205,
     205,   205,   205,   205,   206,   206,   206,   207,   207,   208,
     208,   208,   210,   211,   209,   213,   212,   215,   216,   214,
     218,   217,   219,   219,   219,   219,   220,   220,   222,   221,
     223,   224,   221,   226,   225,   228,   227,   229,   229,   230,
     230,   231,   232,   234,   233,   235,   236,   235,   237,   237,
     238,   237,   240,   241,   239,   239,   242,   242,   243,   243,
     245,   244,   246,   246,   246,   247,   248,   248,   249,   250,
     249,   252,   251,   253,   251,   254,   255,   254,   256,   257,
     256,   258,   258,   258,   259,   259,   260,   261,   260,   260,
     262,   262,   263,   263,   264,   264,   265,   266,   266,   268,
     267,   270,   269,   271,   271,   272,   272,   273,   273,   273,
     273,   273,   273,   273,   274,   274,   274,   274,   274,   274,
     274,   275,   275,   276,   276,   277,   277,   277,   278,   279,
     279,   279,   280,   279,   282,   281,   283,   281,   284,   281,
     285,   286,   285,   287,   287,   287,   287,   287,   287,   287,
     287,   287,   288,   288,   289,   289,   289,   289,   289,   289,
     289,   290,   290,   290,   291,   291,   292,   292,   293,   293,
     293,   293,   293,   293,   293,   293,   293,   294,   294,   295,
     295,   295,   295,   295,   295,   295,   296,   296,   296,   296,
     296,   296,   296,   296,   296,   296,   296,   296,   296,   296,
     296,   296,   296,   296,   296,   296,   296,   296,   296,   296,
     296,   296,   296,   296,   296,   296,   296,   296,   296,   297,
     298,   297,   299,   299,   300,   301,   300,   302,   303,   302,
     304,   302,   302,   305,   302,   306,   307,   302,   308,   308,
     309,   309,   309,   309,   309,   309,   309,   309,   309,   311,
     310,   312,   313,   314,   310,   315,   310,   316,   310,   317,
     318,   310,   319,   310,   320,   321,   322,   310,   310,   323,
     310,   324,   310,   325,   310,   326,   310,   327,   310,   328,
     310,   329,   310,   330,   310,   331,   310,   332,   310,   310,
     310,   310,   333,   333,   333,   333,   333,   334,   334,   334,
     334,   334,   334,   334,   334,   334,   334,   334,   334,   334,
     334,   334,   334,   334,   334,   334,   334,   334,   334,   334,
     334,   334,   334,   334,   334,   334,   334,   334,   334,   334,
     334,   334,   334,   334,   334,   334,   334,   335,   335,   336,
     336,   337,   337,   338,   338,   338,   338,   338,   338,   338,
     338,   338,   338,   338,   338,   338,   338,   338,   338,   338,
     338,   338,   338,   338,   338,   338,   338,   338,   338,   338,
     338,   338,   338,   338,   338,   338,   338,   338,   338,   338,
     338,   338,   338,   338,   338,   338,   338,   338,   338,   339,
     340,   341,   341,   341
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
       0,     3,     1,     0,     4,     0,     0,     9,     1,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     0,
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
       1,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       3,     3,     3,     3
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
     292,   126,     0,   499,   344,   281,     0,   228,     0,   345,
     339,   340,   360,   350,   351,   352,   353,   354,   355,   356,
     357,   358,   359,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    27,     4,    10,     9,   231,   231,
     231,     8,    11,     0,     0,     0,     0,     0,     0,     0,
     169,     5,     0,     0,   274,   283,     0,   285,   290,   291,
       0,   289,   328,   338,   337,    22,   343,   341,   342,   344,
     345,   339,   340,   497,    36,   330,    32,   329,     0,     0,
     343,     0,     0,   344,   345,     0,     0,   497,   296,   334,
     271,   270,   335,   336,   272,   273,     0,    75,   331,   332,
     333,   497,     0,     0,   281,     0,     0,     0,   278,     0,
     275,   128,     0,   314,   318,   317,   316,   315,   319,   320,
     497,    29,     0,   497,   294,   282,   228,     0,   276,     0,
       0,   497,   298,     0,   411,   415,   417,     0,     0,   424,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   327,   326,   321,     0,   228,     0,
     233,   323,   324,     0,     0,     0,     0,     0,     0,     0,
     231,     0,     0,     0,   186,   497,    21,    18,    19,    17,
      15,   269,   271,   270,     0,   267,   244,   245,   268,   272,
     273,     0,   170,   175,   144,   180,   228,   218,     0,   257,
     256,     0,   279,   284,   286,   287,     0,     0,    23,     0,
       0,    64,     0,    64,   343,   341,   342,   344,   345,   339,
     340,   334,   335,   336,   331,   332,   333,   171,     0,     0,
       0,    79,     0,     0,   497,     0,   172,   280,     0,   127,
     135,   304,   307,   306,   303,   305,   308,   309,   310,   293,
     311,     0,     0,   529,   512,   540,   542,   541,   533,   546,
     524,   520,   521,   519,   522,   523,   537,   515,   514,   534,
     535,   525,   503,   526,   527,   528,   530,   532,   513,   536,
     507,   543,   544,   547,   531,   545,   538,   471,   472,   473,
     474,   475,   476,   477,   478,   484,   485,   479,   480,   481,
     482,   483,   486,   487,   488,   489,   490,   491,   492,   493,
     494,   495,   496,   539,   511,   497,   497,   548,   516,   517,
     113,   497,   497,   465,   466,   509,   464,   457,   460,   461,
     463,   458,   459,   470,   467,   468,   469,   497,   462,   510,
     518,   508,   500,   504,   506,   505,     0,     0,     0,     2,
     277,   224,   229,     0,     0,     0,     0,   269,   267,   268,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    12,   227,   232,   256,     0,   322,   325,     6,     7,
     125,     0,   216,     0,     0,     0,     0,     0,    20,    16,
       0,     0,   471,   472,     0,     0,   177,   456,   168,   145,
       0,   182,   182,     0,   220,   225,   219,   252,     0,     0,
     238,   258,   263,   188,   190,   154,   313,   304,   307,   306,
     303,   305,   308,   309,     0,   167,   148,   193,     0,   312,
       0,   501,    40,   498,   502,   300,     0,     0,     0,     0,
       0,    24,     0,     0,    80,    82,    79,   112,     0,   206,
       0,   149,     0,   137,   138,     0,   130,     0,   139,   139,
      31,     2,     0,     0,     0,     0,     0,    25,     0,   223,
       3,   231,   142,    26,     0,   409,     0,     0,     0,   419,
     422,     0,     0,   429,   433,   437,   441,   431,   435,   439,
     443,   445,   447,   449,   450,     0,   234,   122,   217,   123,
     124,   121,    13,   187,     0,     0,   452,   453,     0,   155,
     185,     0,     0,   176,   181,   222,     0,   239,   253,   260,
       0,   206,     0,   146,   202,     0,   197,   195,     0,   301,
      38,     0,     0,     0,    65,    66,    68,    41,    34,    41,
     297,    78,    79,     0,     0,   111,     0,   212,   147,     0,
     129,   131,   139,   134,   140,   133,     3,   552,   553,   551,
     549,   550,   295,    28,   230,   299,     0,   412,   416,   418,
       0,     0,   425,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   382,   454,   455,   178,   184,   183,
     406,   398,   401,   403,   402,   404,   405,   400,   407,   408,
     393,   380,   390,   388,   221,   379,   392,   387,   246,   261,
       0,     0,   203,     0,   194,   206,     0,   152,     0,    64,
      71,    69,    70,     0,    42,    64,    42,    81,   269,    89,
      91,    90,    92,    88,   267,   268,    98,   100,   101,   102,
      93,    84,    83,    94,    85,    87,    86,    76,   173,   215,
     208,   207,   209,     0,     0,   153,   135,   136,   218,    30,
       0,     0,   420,     0,     0,   428,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   383,     0,   206,   395,
       0,     0,     0,     0,   399,   247,   240,   249,   265,   189,
       0,   206,   199,     0,     0,   150,   302,     0,    67,    72,
      73,    74,    37,     0,     0,     0,    33,     0,   103,   104,
     105,   106,   107,   108,   109,   110,    96,     0,   182,   210,
     254,   151,   132,   141,   410,   413,     0,   423,   426,   430,
     434,   438,   442,   432,   436,   440,   444,     0,     0,   451,
       0,     0,     0,   384,   406,   391,   389,   206,   266,     0,
     191,     0,     0,   198,   497,   196,    41,     0,     0,     0,
     343,   341,   342,    53,     0,     0,   344,     0,   164,   345,
     339,   340,     0,    63,     0,    43,   231,   231,    48,    52,
      50,    49,     0,     0,     0,     0,     0,   159,    45,     0,
      62,    44,    41,     0,     0,    95,   174,   212,   242,   243,
     241,   213,   254,   257,   255,     0,   499,     0,     0,     0,
     179,     0,   394,   382,     0,   262,   206,   204,   199,     0,
      42,   118,   497,   499,   117,     0,     0,     0,     0,   274,
      51,     0,     0,     0,   165,     0,   160,     0,   166,   497,
     156,     0,     0,   114,     0,     0,    60,    57,   161,    54,
     162,    42,    99,    97,   211,   218,   250,     0,   254,   235,
     414,     0,   427,   446,   448,     0,   385,     0,   248,     0,
     200,   201,    39,     0,     0,   116,   115,   158,   163,    58,
      55,   157,     0,     0,     0,     0,    46,    47,    59,    56,
      35,   214,   236,   251,   421,   396,     0,   381,   192,   499,
     120,   143,     0,   246,     0,   386,     0,    61,   237,     0,
     119,   397
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     3,    85,    86,    87,   302,    88,   263,   685,
     261,   679,   684,   763,   835,   498,   594,   595,   764,    89,
     282,   146,   503,   504,   700,   701,   702,   777,   854,   767,
     703,   776,    90,    91,   839,   840,   884,    92,    93,    94,
     290,   515,   716,   516,   517,   518,   613,   718,    95,   843,
      96,    97,   241,   844,   845,    98,    99,   100,   509,   778,
     242,   461,   243,   568,   738,   244,   462,   573,   226,   245,
     581,   582,   876,   847,   586,   487,   675,   674,   813,   752,
     583,   584,   751,   606,   607,   711,   857,   712,   713,   915,
     441,   465,   466,   576,   101,   246,   207,   179,   531,   208,
     209,   435,   861,   963,   247,   668,   862,   248,   746,   807,
     917,   468,   863,   249,   470,   471,   472,   579,   748,   580,
     809,   488,   888,   103,   104,   105,   106,   107,   162,   108,
     397,   280,   406,   496,   678,   489,   109,   136,   210,   211,
     212,   111,   112,   113,   114,   664,   741,   737,   873,   956,
     665,   743,   742,   740,   801,   964,   666,   667,   115,   626,
     411,   721,   865,   412,   413,   630,   786,   631,   416,   724,
     867,   634,   638,   635,   639,   636,   640,   637,   641,   642,
     643,   456,   391,   259,   172,   493,   494,   393,   394,   395
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -808
static const yytype_int16 yypact[] =
{
    -808,    86,  -808,  4661,   219,   817,  5412,   139,  -808,  -808,
    -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,
    -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,    92,   112,
     835,   507,  -808,  -808,  4892,  -808,  -808,  4996,  5412,   -37,
    -808,  -808,   552,  -808,   253,    11,  5100,  -808,   -23,   290,
     136,   143,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,
    -808,  -808,  -808,   -18,    -9,    23,    44,    87,   110,   158,
     175,   185,   190,   202,   208,   210,   231,   233,   242,   244,
     254,   256,   263,   264,  -808,  -808,  -808,  -808,    28,    28,
      28,  -808,  -808,  5204,  4788,   121,   121,   121,   121,   121,
    -808,  -808,   785,  5412,  -808,    19,  5516,   106,    68,  -808,
     145,  -808,  -808,  -808,  -808,  -808,   296,   371,   444,   450,
     502,   520,   549,  -808,   132,  -808,   239,  -808,   844,   844,
     -15,    53,    58,    13,    41,   298,   305,  -808,  -808,   265,
    -808,  -808,   266,   267,  -808,  -808,   272,  -808,   265,   266,
     267,  -808,   274,  4996,   360,  5308,   281,  5412,  -808,   323,
    -808,   282,  4077,  -808,  -808,  -808,  -808,  -808,  -808,  -808,
    -808,  -808,  3322,  -808,  -808,  -808,  -808,  4545,  -808,   138,
    4892,  -808,  -808,  4202,  -808,  -808,  -808,  4202,  4202,  -808,
    4202,  4202,  4202,  4202,  4202,  4202,  4202,  4202,  4202,  4202,
    4202,  4202,  4202,  4202,  -808,  -808,  -808,   287,  -808,   633,
    -808,    65,  -808,   289,   292,   293,   308,   308,   308,   633,
      28,   121,   121,   273,  -808,  -808,  -808,  -808,  -808,  -808,
    -808,   343,   344,   346,  5716,   347,  -808,  -808,   353,   355,
     356,   657,  -808,  -808,  -808,  -808,  -808,   304,   426,   300,
     197,   361,  -808,  -808,  -808,  -808,  4077,    39,  -808,   938,
    5412,   316,  5412,   316,  -808,  -808,  -808,  -808,  -808,  -808,
    -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  4077,  1087,
    5412,  4202,   322,  1236,  -808,  5412,  -808,  -808,   214,  -808,
    5723,   -15,   344,   346,    13,    41,   355,   356,    68,  -808,
    -808,  1385,   325,  -808,  -808,  -808,  -808,  -808,  -808,  -808,
    -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,
    -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,
    -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,
    -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,
    -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,
    -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,
    -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,
    -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,
    -808,  -808,  -808,  -808,  -808,  -808,  1534,  5412,   187,  -808,
    -808,  -808,  -808,   324,  4202,  1683,  5412,  -808,  -808,  -808,
     329,  4202,  4202,  4202,   330,   334,  4202,   336,   337,   341,
     342,   345,   349,   364,   372,   374,   375,   376,   331,   350,
     385,  -808,   386,  -808,  -808,   633,  -808,  -808,  -808,  -808,
    -808,   332,  -808,  4202,   379,   399,   414,   417,  -808,  -808,
     197,  1832,   400,   401,   358,   403,  -808,  -808,  -808,  -808,
     464,    71,    71,   189,  -808,  -808,  -808,  -808,   359,   633,
    -808,  -808,  -808,  -808,  -808,  -808,  -808,    35,   -12,    14,
     156,   199,    88,   130,  4202,  -808,  -808,  -808,   427,  -808,
     469,  -808,  -808,  -808,  -808,   423,   428,   829,   430,   434,
     433,  -808,   435,   441,   429,   439,  4202,  -808,  1981,   452,
    4202,  -808,   495,  -808,  -808,   456,   455,   541,  4202,  4202,
    -808,  -808,  2130,  2279,  2428,  2577,  2726,  -808,   462,  -808,
     470,    19,  -808,  -808,   471,  -808,   467,   474,   479,  -808,
    -808,   477,  5620,  -808,  -808,  -808,  -808,  -808,  -808,  -808,
    -808,  -808,  -808,  -808,  -808,  4202,  -808,  -808,  -808,  -808,
    -808,  -808,  -808,  -808,   475,   480,  -808,  -808,   496,  -808,
    -808,   612,   619,  -808,  -808,  -808,   776,  -808,  -808,   492,
     493,   452,  5412,  -808,  -808,   513,   508,  -808,   431,  -808,
    -808,  4077,  4077,  4077,  -808,   516,  -808,  -808,  -808,  -808,
    -808,  -808,  4202,   518,   517,  -808,   524,    31,  -808,   447,
    -808,  -808,  4202,  -808,  -808,  -808,   527,  -808,  -808,  -808,
    -808,  -808,  -808,  -808,  -808,  -808,  5412,  -808,  -808,  -808,
    5620,  5620,  -808,   533,  5620,  5620,  5620,  5620,  5620,  5620,
    5620,  5620,  5620,  5620,   528,  -808,  -808,  -808,  -808,  -808,
     534,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,
    -808,  -808,  -808,  -808,  -808,  -808,   663,  -808,    95,  -808,
     543,   540,  -808,  4202,  -808,   452,  4202,  -808,  5412,   316,
    -808,  -808,  -808,   829,    52,   316,    75,  -808,   -15,  -808,
    -808,  -808,  -808,  -808,    13,    41,  -808,  -808,  -808,  -808,
     352,  -808,  -808,  -808,  -808,    68,  -808,  -808,  -808,  -808,
    -808,  -808,   539,  5412,  4202,  -808,  5723,  -808,   304,  -808,
     551,  5412,  -808,   555,  5620,  -808,   557,   558,   560,   562,
     565,   567,   569,   570,   568,   571,  -808,   573,   452,  -808,
     606,   776,  3867,  3867,  -808,  -808,  -808,  -808,   518,  -808,
     572,   452,   578,   576,   585,  -808,  -808,   575,  -808,  -808,
    -808,  -808,  -808,  3918,   581,   577,  -808,   518,  -808,  -808,
    -808,  -808,  -808,  -808,  -808,  -808,  -808,   518,    71,  -808,
     297,  -808,  -808,  -808,  -808,  -808,   582,  -808,  -808,  -808,
    -808,  -808,  -808,  -808,  -808,  -808,  -808,   711,   712,  -808,
     598,  5412,   599,  -808,  -808,  -808,  -808,   452,  -808,   583,
    -808,   600,  4202,  -808,  -808,  -808,  -808,   501,   817,  4295,
      35,   -12,    14,  -808,  4045,  4420,   156,   603,   684,   199,
      88,   130,   605,  -808,  4202,  -808,    28,    28,  -808,  -808,
    -808,  -808,  4045,   121,   121,  4202,   121,  -808,  -808,   802,
    -808,  -808,  -808,   608,   518,  -808,  -808,    31,  -808,  -808,
    -808,  -808,   284,   300,  -808,   609,  -808,   610,   611,   616,
    -808,   602,  -808,   617,   618,  -808,   452,  -808,   578,  2875,
      83,  -808,  -808,  -808,  -808,   102,   102,  4202,  4202,   298,
    -808,   121,   121,   273,  -808,  4202,  -808,  4170,  -808,  -808,
    -808,   614,   624,  -808,   121,   121,  -808,  -808,  -808,  -808,
    -808,    94,  -808,  -808,  -808,   304,  -808,   626,   297,  -808,
    -808,  3471,  -808,  -808,  -808,   630,   629,   642,  -808,   646,
    -808,  -808,  -808,  3024,  3620,  -808,  -808,  -808,  -808,  -808,
    -808,  -808,  4295,   647,  4202,  3173,  -808,  -808,  -808,  -808,
    -808,  -808,  -808,  -808,  -808,  -808,   776,  -808,  -808,  -808,
    -808,  -808,   651,    95,  3867,  -808,  3769,  -808,  -808,   650,
    -808,  -808
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -808,  -355,  -808,  -808,  -808,  -808,  -808,   243,  -808,  -808,
    -808,  -808,  -569,  -808,  -808,  -241,   100,  -808,  -808,   -84,
    -808,  -808,  -457,  -808,  -808,  -808,  -683,  -808,  -808,  -808,
    -808,  -808,   -83,    21,  -808,  -579,  -538,    30,  -808,  -476,
    -808,    79,  -808,  -808,  -808,  -808,  -449,  -808,  -808,  -808,
     -42,  -808,  -808,  -755,  -808,  -808,   -76,   542,  -808,  -808,
    -119,  -808,  -808,  -808,  -808,  -226,  -808,  -439,   -85,  -808,
    -808,  -808,  -808,  -238,  -808,  -808,  -808,  -808,   -81,   -10,
    -472,  -808,  -808,  -527,  -808,   -54,  -808,  -808,  -808,  -808,
     152,  -679,  -808,  -808,    42,  -177,   -65,  -103,  -808,   278,
    -808,  -808,  -807,  -808,  -188,  -808,  -808,  -808,  -159,  -808,
    -808,  -808,  -808,  -712,   -53,  -640,  -808,  -808,  -808,  -808,
    -808,    -4,     1,   -29,    -1,    16,   705,   715,  -808,  -116,
    -808,  -808,  -808,  -229,  -808,  -133,   -56,    37,   -88,  -808,
     621,  -339,  -269,    -2,  -166,  -694,  -808,   -46,  -808,  -808,
      89,  -808,  -808,  -808,  -808,  -808,  -808,  -700,    74,  -808,
    -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,
    -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,  -808,
    -808,  -808,   594,  -117,  -800,  -808,  -165,  -808,  -808,  -808
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -384
static const yytype_int16 yytable[] =
{
     124,   126,   125,   127,   102,   135,   390,   392,   157,   217,
     218,   227,   228,   229,   230,   459,   277,   253,   222,   486,
     279,   519,   500,   574,   213,   214,   147,   152,   747,   299,
     686,   446,   433,   499,   283,   156,   158,   160,   608,   783,
     110,   612,   805,   806,   530,   178,   298,   803,   477,   604,
     511,   502,   221,   301,   670,   916,   396,   759,   760,   761,
     467,   175,   271,   204,   405,   808,   921,   274,   864,   891,
     615,   176,   204,   398,   159,  -314,   478,   479,  -318,   171,
     759,   760,   761,   934,   853,    34,     2,   904,   759,   760,
     761,   709,   219,   390,   855,   223,   480,   161,   250,   759,
     760,   761,   252,  -316,  -317,   432,   300,   180,   451,   204,
     272,   953,   183,   390,   570,   275,   571,   390,  -271,   138,
     710,   184,   458,   476,   157,  -314,   125,   127,   443,   443,
     443,  -315,   481,   482,   483,   390,   448,   449,   403,   251,
     298,   298,   943,   463,  -270,   687,   273,   174,   754,    33,
     864,   276,   158,   185,   178,   447,   287,  -226,   256,   966,
     469,   205,   298,   717,   206,  -269,   616,   508,   528,   138,
     205,   913,   298,   206,   186,   182,   400,   534,  -319,   410,
     484,   404,  -318,   414,   415,   762,   417,   418,   419,   420,
     421,   422,   423,   424,   425,   426,   427,   428,   429,   430,
     300,   300,  -317,   633,   755,   434,   864,   205,   766,   572,
     436,   800,   442,   442,   442,   434,   932,   187,  -272,   450,
    -320,  -318,   300,   477,   811,   745,  -319,   950,   116,  -314,
     390,   881,   300,  -320,   882,   257,   951,   450,   883,   390,
     188,  -317,   781,  -259,   434,   890,  -316,   880,   522,   523,
     224,   478,   479,   225,   524,   525,   117,   118,   556,   495,
    -273,   495,   965,   903,   969,  -319,   260,   401,  -314,   137,
     526,   480,  -320,   138,   258,   402,   119,   505,   460,   495,
     874,   578,   407,   911,   400,   390,  -267,   842,   189,  -315,
     174,   722,   723,   407,   490,   726,   727,   728,   729,   730,
     731,   732,   733,   734,   735,   190,   407,   481,   482,   483,
     140,   141,   120,   121,   122,   191,   529,   407,   575,   234,
     192,   140,   141,   747,   402,   512,   402,   473,   204,  -268,
     408,   474,   193,   182,   140,   141,   216,   220,   194,   856,
     195,   408,   390,  -316,   234,   140,   141,   935,   936,   929,
     677,   123,   204,   671,   408,   510,   390,   390,   390,   390,
     390,   196,   900,   197,   596,   408,   409,   144,   145,   444,
     445,   715,   198,   262,   199,   858,   859,   409,   144,   145,
    -315,   298,  -316,   173,   200,   788,   201,   174,   858,   859,
     409,   144,   145,   202,   203,   278,   495,  -269,  -267,  -268,
     450,   409,   144,   145,   281,   495,   284,   536,   537,   538,
     175,   286,   541,   288,   860,   937,   431,   289,   438,  -315,
     181,   439,   440,   941,   182,  -269,   205,   860,  -269,   206,
    -269,   434,  -269,  -314,  -318,   407,  -317,  -316,   757,   558,
     477,   300,   464,  -315,   765,  -319,  -320,   519,  -259,   756,
     205,   475,   497,   206,   506,   532,   477,   521,   680,   681,
     682,   557,   553,   140,   141,   434,   535,   539,   478,   479,
     204,   540,   298,   542,   543,   298,   298,   298,   544,   545,
     585,   554,   546,   408,   478,   479,   547,   705,   480,   566,
     577,   768,   769,   298,   770,   771,   772,   773,   774,   775,
    -271,   548,   505,  -271,   480,  -271,   585,  -271,   559,   549,
     116,   550,   551,   552,   614,   614,   148,   236,   237,   409,
     144,   145,   555,   402,   481,   482,   483,   688,   560,   689,
     690,   691,   300,   692,   693,   300,   300,   300,   117,   118,
     481,   482,   483,   561,   140,   141,   562,   706,   564,   565,
     596,   644,   567,   300,   569,   232,   233,   587,   119,   588,
     589,   163,   597,   590,   149,   599,   602,   298,   205,   598,
     600,   206,   676,  -270,   601,   694,  -270,   603,  -270,  -267,
    -270,   495,  -267,  -205,  -267,   609,  -267,   896,   714,   164,
     165,   610,   611,    39,   120,   121,   122,   622,   505,   704,
     150,   144,   145,   623,   627,   628,   625,   908,   614,   166,
     629,   695,   239,   240,   632,   650,   651,   652,   653,   654,
     655,   656,   657,   910,   645,   720,   647,   300,   648,   646,
     881,  -268,   705,   882,  -268,   649,  -268,   883,  -268,   151,
    -264,   669,   407,   672,   673,   167,   168,   169,   696,  -272,
     707,   705,  -272,   683,  -272,   708,  -272,   697,   698,   699,
     719,   705,   938,   658,   725,   736,   231,   910,   739,   753,
     140,   141,   585,   744,   749,   750,   779,   495,  -273,   836,
     837,  -273,   784,  -273,   170,  -273,   787,   846,   789,   790,
     408,   791,   706,   792,   232,   233,   793,   879,   794,   659,
     795,   796,   810,   234,   799,   797,   814,   816,   798,   852,
     585,   706,   780,   390,   235,   812,   815,   851,   910,   866,
     785,   706,   868,   869,   236,   237,   409,   144,   145,   870,
     872,   877,   875,   897,   898,   899,   660,   925,   705,   912,
     920,   922,   923,   946,   704,   662,   663,   924,   892,   928,
     238,   239,   240,   947,   926,   390,   392,   952,   906,   907,
     955,   909,  -383,   704,   849,   933,   905,   390,   390,   392,
     277,   901,   902,   704,   918,   957,   434,   958,   961,   390,
     967,   971,   945,   758,   838,   650,   651,   652,   653,   654,
     655,   656,   657,   841,   231,   782,   157,   930,   706,   485,
     390,   392,   878,   914,   968,   848,   939,   940,   753,   624,
     919,   407,   255,   885,   886,   125,   127,   871,   889,   948,
     949,   254,   232,   233,   158,   893,   116,   927,   457,   802,
     585,   234,   437,   658,   591,   592,   593,   850,   291,   140,
     141,     0,   235,   893,   139,   250,     0,     0,   234,     0,
     704,     0,     0,   264,   117,   118,     0,     0,   434,   408,
       0,     0,     0,     0,     0,     0,   292,   293,     0,   659,
       0,     0,   140,   141,   119,     0,   236,   237,   238,   239,
     240,   265,   266,   585,   450,     0,   294,     0,     0,   450,
       0,   585,   142,   236,   237,   409,   144,   145,   944,     0,
       0,   267,     0,     0,     0,     0,   660,     0,   661,     0,
     120,   121,   122,     0,   434,   662,   663,     0,     0,     0,
       0,     0,   295,   296,   297,     0,     0,     0,   143,   144,
     145,     0,     0,     0,     0,     0,     0,   268,   269,   270,
     450,   303,   304,   305,   306,   307,   308,   309,   310,   311,
     312,   313,   314,   315,   316,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,   317,   318,   319,   320,   321,
     322,   323,   324,   325,   326,    35,    36,     0,   327,     0,
     328,   329,   330,   331,   332,   333,   334,   335,   336,     0,
     337,   338,     0,   339,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,   362,   363,   364,   365,
     366,   367,   368,   369,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   491,   371,     0,
     372,   492,   373,   374,   375,   376,   377,   378,   379,   380,
     381,   382,   383,   384,   385,   386,   387,     0,   388,   389,
     303,   304,   305,   306,   307,   308,   309,   310,   311,   312,
     313,   314,   315,   316,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,   317,   318,   319,   320,   321,   322,
     323,   324,   325,   326,    35,    36,     0,   327,     0,   328,
     329,   330,   331,   332,   333,   334,   335,   336,     0,   337,
     338,     0,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,   356,
     357,   358,   359,   360,   361,   362,   363,   364,   365,   366,
     367,   368,   369,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   491,   371,   501,   372,
       0,   373,   374,   375,   376,   377,   378,   379,   380,   381,
     382,   383,   384,   385,   386,   387,     0,   388,   389,   303,
     304,   305,   306,   307,   308,   309,   310,   311,   312,   313,
     314,   315,   316,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,   317,   318,   319,   320,   321,   322,   323,
     324,   325,   326,    35,    36,     0,   327,     0,   328,   329,
     330,   331,   332,   333,   334,   335,   336,     0,   337,   338,
       0,   339,   340,   341,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,   355,   356,   357,
     358,   359,   360,   361,   362,   363,   364,   365,   366,   367,
     368,   369,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   491,   371,     0,   372,   507,
     373,   374,   375,   376,   377,   378,   379,   380,   381,   382,
     383,   384,   385,   386,   387,     0,   388,   389,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,    35,    36,     0,   327,     0,   328,   329,   330,
     331,   332,   333,   334,   335,   336,     0,   337,   338,     0,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   368,
     369,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   491,   371,     0,   372,   520,   373,
     374,   375,   376,   377,   378,   379,   380,   381,   382,   383,
     384,   385,   386,   387,     0,   388,   389,   303,   304,   305,
     306,   307,   308,   309,   310,   311,   312,   313,   314,   315,
     316,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   317,   318,   319,   320,   321,   322,   323,   324,   325,
     326,    35,    36,     0,   327,     0,   328,   329,   330,   331,
     332,   333,   334,   335,   336,     0,   337,   338,     0,   339,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,   355,   356,   357,   358,   359,
     360,   361,   362,   363,   364,   365,   366,   367,   368,   369,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   491,   371,   527,   372,     0,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,     0,   388,   389,   303,   304,   305,   306,
     307,   308,   309,   310,   311,   312,   313,   314,   315,   316,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
     317,   318,   319,   320,   321,   322,   323,   324,   325,   326,
      35,    36,     0,   327,     0,   328,   329,   330,   331,   332,
     333,   334,   335,   336,     0,   337,   338,     0,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,   356,   357,   358,   359,   360,
     361,   362,   363,   364,   365,   366,   367,   368,   369,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   491,   371,   533,   372,     0,   373,   374,   375,
     376,   377,   378,   379,   380,   381,   382,   383,   384,   385,
     386,   387,     0,   388,   389,   303,   304,   305,   306,   307,
     308,   309,   310,   311,   312,   313,   314,   315,   316,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,   317,
     318,   319,   320,   321,   322,   323,   324,   325,   326,    35,
      36,     0,   327,     0,   328,   329,   330,   331,   332,   333,
     334,   335,   336,     0,   337,   338,     0,   339,   340,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,   356,   357,   358,   359,   360,   361,
     362,   363,   364,   365,   366,   367,   368,   369,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   491,   371,     0,   372,   563,   373,   374,   375,   376,
     377,   378,   379,   380,   381,   382,   383,   384,   385,   386,
     387,     0,   388,   389,   303,   304,   305,   306,   307,   308,
     309,   310,   311,   312,   313,   314,   315,   316,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,   317,   318,
     319,   320,   321,   322,   323,   324,   325,   326,    35,    36,
       0,   327,     0,   328,   329,   330,   331,   332,   333,   334,
     335,   336,     0,   337,   338,     0,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,   356,   357,   358,   359,   360,   361,   362,
     363,   364,   365,   366,   367,   368,   369,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     491,   371,     0,   372,   605,   373,   374,   375,   376,   377,
     378,   379,   380,   381,   382,   383,   384,   385,   386,   387,
       0,   388,   389,   303,   304,   305,   306,   307,   308,   309,
     310,   311,   312,   313,   314,   315,   316,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   317,   318,   319,
     320,   321,   322,   323,   324,   325,   326,    35,    36,     0,
     327,     0,   328,   329,   330,   331,   332,   333,   334,   335,
     336,     0,   337,   338,     0,   339,   340,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,   356,   357,   358,   359,   360,   361,   362,   363,
     364,   365,   366,   367,   368,   369,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   491,
     371,   617,   372,     0,   373,   374,   375,   376,   377,   378,
     379,   380,   381,   382,   383,   384,   385,   386,   387,     0,
     388,   389,   303,   304,   305,   306,   307,   308,   309,   310,
     311,   312,   313,   314,   315,   316,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,   317,   318,   319,   320,
     321,   322,   323,   324,   325,   326,    35,    36,     0,   327,
       0,   328,   329,   330,   331,   332,   333,   334,   335,   336,
       0,   337,   338,     0,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   491,   371,
     618,   372,     0,   373,   374,   375,   376,   377,   378,   379,
     380,   381,   382,   383,   384,   385,   386,   387,     0,   388,
     389,   303,   304,   305,   306,   307,   308,   309,   310,   311,
     312,   313,   314,   315,   316,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,   317,   318,   319,   320,   321,
     322,   323,   324,   325,   326,    35,    36,     0,   327,     0,
     328,   329,   330,   331,   332,   333,   334,   335,   336,     0,
     337,   338,     0,   339,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,   362,   363,   364,   365,
     366,   367,   368,   369,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   491,   371,   619,
     372,     0,   373,   374,   375,   376,   377,   378,   379,   380,
     381,   382,   383,   384,   385,   386,   387,     0,   388,   389,
     303,   304,   305,   306,   307,   308,   309,   310,   311,   312,
     313,   314,   315,   316,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,   317,   318,   319,   320,   321,   322,
     323,   324,   325,   326,    35,    36,     0,   327,     0,   328,
     329,   330,   331,   332,   333,   334,   335,   336,     0,   337,
     338,     0,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,   356,
     357,   358,   359,   360,   361,   362,   363,   364,   365,   366,
     367,   368,   369,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   491,   371,     0,   372,
     620,   373,   374,   375,   376,   377,   378,   379,   380,   381,
     382,   383,   384,   385,   386,   387,     0,   388,   389,   303,
     304,   305,   306,   307,   308,   309,   310,   311,   312,   313,
     314,   315,   316,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,   317,   318,   319,   320,   321,   322,   323,
     324,   325,   326,    35,    36,     0,   327,     0,   328,   329,
     330,   331,   332,   333,   334,   335,   336,     0,   337,   338,
       0,   339,   340,   341,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,   355,   356,   357,
     358,   359,   360,   361,   362,   363,   364,   365,   366,   367,
     368,   369,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   491,   371,     0,   372,     0,
     373,   374,   375,   376,   377,   378,   379,   380,   381,   382,
     383,   384,   385,   386,   387,   621,   388,   389,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,    35,    36,     0,   327,     0,   328,   329,   330,
     331,   332,   333,   334,   335,   336,     0,   337,   338,     0,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   368,
     369,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   491,   371,   931,   372,     0,   373,
     374,   375,   376,   377,   378,   379,   380,   381,   382,   383,
     384,   385,   386,   387,     0,   388,   389,   303,   304,   305,
     306,   307,   308,   309,   310,   311,   312,   313,   314,   315,
     316,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   317,   318,   319,   320,   321,   322,   323,   324,   325,
     326,    35,    36,     0,   327,     0,   328,   329,   330,   331,
     332,   333,   334,   335,   336,     0,   337,   338,     0,   339,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,   355,   356,   357,   358,   359,
     360,   361,   362,   363,   364,   365,   366,   367,   368,   369,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   491,   371,     0,   372,   959,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,     0,   388,   389,   303,   304,   305,   306,
     307,   308,   309,   310,   311,   312,   313,   314,   315,   316,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
     317,   318,   319,   320,   321,   322,   323,   324,   325,   326,
      35,    36,     0,   327,     0,   328,   329,   330,   331,   332,
     333,   334,   335,   336,     0,   337,   338,     0,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,   356,   357,   358,   359,   360,
     361,   362,   363,   364,   365,   366,   367,   368,   369,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   491,   371,   962,   372,     0,   373,   374,   375,
     376,   377,   378,   379,   380,   381,   382,   383,   384,   385,
     386,   387,     0,   388,   389,   303,   304,   305,   306,   307,
     308,   309,   310,   311,   312,   313,   314,   315,   316,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,   317,
     318,   319,   320,   321,   322,   323,   324,   325,   326,    35,
      36,     0,   327,     0,   328,   329,   330,   331,   332,   333,
     334,   335,   336,     0,   337,   338,     0,   339,   340,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,   356,   357,   358,   359,   360,   361,
     362,   363,   364,   365,   366,   367,   368,   369,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   370,   371,     0,   372,     0,   373,   374,   375,   376,
     377,   378,   379,   380,   381,   382,   383,   384,   385,   386,
     387,     0,   388,   389,   303,   304,   305,   306,   307,   308,
     309,   310,   311,   312,   313,   314,   315,   316,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,   317,   318,
     319,   320,   321,   322,   323,   324,   325,   326,    35,    36,
       0,   327,     0,   328,   329,   330,   331,   332,   333,   334,
     335,   336,     0,   337,   338,     0,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,   356,   357,   358,   359,   360,   361,   362,
     363,   364,   365,   366,   367,   368,   369,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   371,   954,   372,     0,   373,   374,   375,   376,   377,
     378,   379,   380,   381,   382,   383,   384,   385,   386,   387,
       0,   388,   389,   303,   304,   305,   306,   307,   308,   309,
     310,   311,   312,   313,   314,   315,   316,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   317,   318,   319,
     320,   321,   322,   323,   324,   325,   326,    35,    36,     0,
     327,     0,   328,   329,   330,   331,   332,   333,   334,   335,
     336,     0,   337,   338,     0,   339,   340,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,   356,   357,   358,   359,   360,   361,   362,   363,
     364,   365,   366,   367,   368,   369,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   960,
     371,     0,   372,     0,   373,   374,   375,   376,   377,   378,
     379,   380,   381,   382,   383,   384,   385,   386,   387,     0,
     388,   389,   303,   304,   305,   306,   307,   308,   309,   310,
     311,   312,   313,   314,   315,   316,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,   317,   318,   319,   320,
     321,   322,   323,   324,   325,   326,    35,    36,     0,   327,
       0,   328,   329,   330,   331,   332,   333,   334,   335,   336,
       0,   337,   338,     0,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,   804,     0,   652,   653,
     654,   655,   656,   657,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   970,   371,
       0,   372,     0,   373,   374,   375,   376,   377,   378,   379,
     380,   381,   382,   383,   384,   385,   386,   387,     0,   388,
     389,   817,   818,     0,   658,     0,   819,   820,     0,     0,
       0,     0,     0,     0,     0,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,   821,   822,    30,    31,   823,
     659,    33,     0,     0,    34,    35,    36,   824,   825,    38,
      39,    40,    41,     0,    43,   826,    45,   155,    47,   827,
       0,     0,   828,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   829,   830,   831,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,   832,   833,   817,   818,
       0,     0,     0,   819,   820,     0,     0,     0,     0,   834,
       0,     0,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,   821,   822,   131,   132,   291,     0,    33,     0,
       0,    34,    35,    36,     0,   825,    38,     0,    40,     0,
       0,     0,   826,   154,   155,     0,     0,     0,     0,   828,
       0,     0,     0,     0,   292,   293,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   294,     0,     0,     0,   829,   830,
     831,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     295,   296,   297,   128,   129,     0,     0,     0,   942,   820,
       0,     0,     0,     0,     0,     0,   834,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   821,   822,   131,
     132,   407,     0,    33,     0,     0,     0,    35,    36,     0,
     825,    38,     0,    40,     0,     0,     0,   826,   154,   155,
       0,     0,     0,     0,   828,     0,     0,     0,     0,   140,
     141,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   408,
       0,     0,     0,   829,   830,   831,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   409,   144,   145,   128,   129,
       0,     0,     0,     0,   130,     0,     0,     0,     0,     0,
       0,   834,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,   131,   132,     0,     0,    33,     0,
       0,     0,    35,    36,     0,   153,    38,     0,    40,     0,
       0,     0,   133,   154,   155,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   134,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   128,   129,     0,     0,     0,     0,   820,
       0,     0,     0,     0,     0,     0,   887,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   821,   822,   131,
     132,     0,     0,    33,     0,     0,     0,    35,    36,     0,
       0,     0,     0,    40,     0,     0,     0,   826,   154,     0,
       0,     0,     0,     0,   894,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   829,   830,   831,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   128,   129,
       0,     0,     0,     0,   130,     0,     0,     0,     0,     0,
       0,   895,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,   131,   132,     0,     0,    33,     0,
       0,     0,    35,    36,     0,     0,     0,     0,    40,     0,
       0,     0,   133,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   134,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     4,     5,     0,     0,     0,     6,
       7,     0,     0,     0,     0,     0,     0,   399,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,     0,    33,     0,     0,    34,    35,    36,
       0,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,     0,
      84,     4,     5,     0,     0,     0,     6,   130,     0,     0,
       0,     0,     0,     0,     0,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,   131,   132,     0,
       0,    33,     0,     0,    34,    35,    36,     0,   153,    38,
       0,    40,     0,     0,     0,   133,   154,   155,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   134,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,   128,   129,     0,     0,     0,
       0,   130,     0,     0,     0,     0,     0,     0,     0,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,   131,   132,     0,     0,    33,     0,     0,     0,    35,
      36,     0,   153,    38,     0,    40,     0,     0,     0,   133,
     154,   155,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   134,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,   128,
     129,     0,     0,     0,     0,   130,     0,     0,     0,     0,
       0,     0,     0,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,   131,   132,     0,     0,    33,
       0,     0,     0,    35,    36,     0,     0,     0,     0,    40,
       0,     0,     0,   133,   154,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   134,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,   128,   129,     0,     0,     0,     0,   130,
     177,     0,     0,     0,     0,     0,     0,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,   131,
     132,     0,     0,    33,     0,     0,     0,    35,    36,     0,
       0,     0,     0,    40,     0,     0,     0,   133,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   134,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,     4,     5,     0,
       0,     0,     0,   130,     0,     0,     0,     0,     0,     0,
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,     0,     0,    33,     0,     0,
       0,    35,    36,     0,     0,     0,     0,    40,     0,     0,
       0,   133,     0,     0,   215,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   134,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,   128,   129,     0,     0,     0,     0,   130,   285,     0,
       0,     0,     0,     0,     0,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,   131,   132,     0,
       0,    33,     0,     0,     0,    35,    36,     0,     0,     0,
       0,    40,     0,     0,     0,   133,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   134,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,   128,   129,     0,     0,     0,
       0,   130,     0,     0,     0,     0,     0,     0,     0,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,   131,   132,     0,     0,    33,     0,     0,     0,    35,
      36,     0,     0,     0,     0,    40,     0,     0,     0,   133,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   134,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,   128,
     129,     0,     0,     0,     0,   130,     0,     0,     0,     0,
       0,     0,     0,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,   131,   132,     0,     0,     0,
       0,     0,     0,    35,    36,     0,     0,     0,     0,    40,
       0,     0,     0,   133,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   134,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,   128,   129,     0,     0,     0,     0,   264,
       0,     0,     0,     0,     0,     0,     0,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   265,   266,   131,
     132,     0,     0,     0,     0,     0,     0,    35,    36,     0,
       0,     0,     0,     0,     0,     0,     0,   267,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   268,   269,   270,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,   513,     0,     0,
       0,     0,   264,     0,     0,     0,     0,     0,     0,     0,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
     265,   266,     0,     0,     0,     0,     0,     0,     0,     0,
      35,    36,     0,     0,     0,     0,   514,     0,   452,   453,
     267,   339,   340,   341,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,   355,   356,   357,
     358,   359,   360,   361,   362,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   268,   269,   270,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   454,     0,     0,     0,
     373,   374,     0,   376,   377,   378,   379,   380,   381,   382,
     383,   384,   385,   386,   455,     0,   388
};

static const yytype_int16 yycheck[] =
{
       4,     5,     4,     5,     3,     6,   172,   172,    37,    93,
      93,    96,    97,    98,    99,   241,   135,   105,    94,   257,
     137,   290,   263,   462,    89,    90,    30,    31,   668,   162,
     599,   219,   209,   262,   151,    34,    37,    38,   510,   718,
       3,   517,   742,   743,   399,    46,   162,   741,     9,   506,
     288,   280,    94,   170,   581,   862,   173,     5,     6,     7,
     248,    50,     9,    44,   181,   748,   866,     9,   780,   824,
     519,    60,    44,   176,    37,    90,    37,    38,    90,    42,
       5,     6,     7,   883,   767,    46,     0,   842,     5,     6,
       7,    60,    93,   259,   777,    94,    57,   134,   102,     5,
       6,     7,   103,    90,    90,   208,   162,   130,   225,    44,
      57,   918,   130,   279,    43,    57,    45,   283,   130,   134,
      89,   130,   241,   256,   153,    90,   128,   129,   216,   217,
     218,    90,    93,    94,    95,   301,   221,   222,   180,   102,
     256,   257,   897,   246,   130,   602,    93,   134,   675,    43,
     862,    93,   153,   130,   155,   220,   157,   129,    90,   959,
     248,   142,   278,   612,   145,   130,   521,   284,   397,   134,
     142,   854,   288,   145,   130,   134,   177,   406,    90,   183,
     141,   180,    90,   187,   188,   133,   190,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,   201,   202,   203,
     256,   257,    90,   542,   676,   209,   918,   142,   133,   138,
     145,   738,   216,   217,   218,   219,   133,   130,   130,   223,
      90,   129,   278,     9,   751,   130,    90,   133,     9,    90,
     396,   129,   288,    90,   132,    90,   915,   241,   136,   405,
     130,   129,   714,   148,   248,   824,    90,   816,   365,   366,
     129,    37,    38,   132,   371,   372,    37,    38,   435,   260,
     130,   262,   956,   842,   964,   129,   134,   129,   129,   130,
     387,    57,   129,   134,   129,   137,    57,   281,   241,   280,
     807,   469,     9,   852,   285,   451,   130,   763,   130,    90,
     134,   630,   631,     9,   257,   634,   635,   636,   637,   638,
     639,   640,   641,   642,   643,   130,     9,    93,    94,    95,
      37,    38,    93,    94,    95,   130,   129,     9,   129,    46,
     130,    37,    38,   963,   137,   288,   137,   130,    44,   130,
      57,   134,   130,   134,    37,    38,    93,    94,   130,   778,
     130,    57,   508,    90,    46,    37,    38,   885,   886,   876,
     588,   132,    44,   582,    57,   141,   522,   523,   524,   525,
     526,   130,   834,   130,   497,    57,    93,    94,    95,   217,
     218,   609,   130,   134,   130,    91,    92,    93,    94,    95,
      90,   497,   129,   130,   130,   724,   130,   134,    91,    92,
      93,    94,    95,   130,   130,    90,   397,   132,   132,   132,
     404,    93,    94,    95,   132,   406,   132,   411,   412,   413,
      50,   130,   416,    90,   130,   887,   129,   135,   129,   129,
     130,   129,   129,   895,   134,   129,   142,   130,   132,   145,
     134,   435,   136,    90,    90,     9,    90,    90,   679,   443,
       9,   497,   138,    90,   685,    90,    90,   716,   148,   678,
     142,    90,   136,   145,   132,   131,     9,   132,   591,   592,
     593,   129,   131,    37,    38,   469,   137,   137,    37,    38,
      44,   137,   588,   137,   137,   591,   592,   593,   137,   137,
     484,   131,   137,    57,    37,    38,   137,   603,    57,   131,
     131,   139,   140,   609,   142,   143,   144,   145,   146,   147,
     129,   137,   506,   132,    57,   134,   510,   136,   129,   137,
       9,   137,   137,   137,   518,   519,     9,    91,    92,    93,
      94,    95,   137,   137,    93,    94,    95,     9,   129,    11,
      12,    13,   588,    15,    16,   591,   592,   593,    37,    38,
      93,    94,    95,   129,    37,    38,   129,   603,   148,   148,
     683,   555,   149,   609,    90,    37,    38,   130,    57,    90,
     137,     9,   132,   135,    57,   132,   137,   683,   142,   135,
     135,   145,   141,   129,   133,    57,   132,   138,   134,   129,
     136,   582,   132,   131,   134,    90,   136,   825,   141,    37,
      38,   135,   137,    52,    93,    94,    95,   135,   602,   603,
      93,    94,    95,   133,   137,   131,   135,   845,   612,    57,
     131,    93,    94,    95,   137,     9,    10,    11,    12,    13,
      14,    15,    16,   849,   149,   626,   130,   683,    16,   149,
     129,   129,   748,   132,   132,    16,   134,   136,   136,   132,
     148,   148,     9,   130,   136,    93,    94,    95,   130,   129,
     133,   767,   132,   137,   134,   131,   136,   139,   140,   141,
     133,   777,   888,    57,   131,   137,     9,   893,   134,   673,
      37,    38,   676,    10,   131,   135,   137,   678,   129,   763,
     763,   132,   131,   134,   132,   136,   131,   763,   131,   131,
      57,   131,   748,   131,    37,    38,   131,   814,   131,    93,
     131,   131,   130,    46,   131,   137,   130,   132,   137,   132,
     714,   767,   713,   879,    57,   137,   131,   136,   944,   137,
     721,   777,    11,    11,    91,    92,    93,    94,    95,   131,
     131,   131,   149,   130,    50,   130,   130,   135,   854,   131,
     131,   131,   131,   129,   748,   139,   140,   131,   824,   131,
      93,    94,    95,   129,   137,   921,   921,   131,   843,   844,
     130,   846,   133,   767,   763,   882,   842,   933,   934,   934,
     889,   836,   837,   777,   862,   133,   780,   131,   131,   945,
     129,   131,   899,   683,   763,     9,    10,    11,    12,    13,
      14,    15,    16,   763,     9,   716,   825,   878,   854,   257,
     966,   966,   812,   857,   963,   763,   891,   892,   812,   531,
     863,     9,   107,   817,   818,   817,   818,   801,   819,   904,
     905,   106,    37,    38,   825,   824,     9,   873,   234,   740,
     834,    46,   211,    57,     5,     6,     7,   763,     9,    37,
      38,    -1,    57,   842,     9,   849,    -1,    -1,    46,    -1,
     854,    -1,    -1,     9,    37,    38,    -1,    -1,   862,    57,
      -1,    -1,    -1,    -1,    -1,    -1,    37,    38,    -1,    93,
      -1,    -1,    37,    38,    57,    -1,    91,    92,    93,    94,
      95,    37,    38,   887,   888,    -1,    57,    -1,    -1,   893,
      -1,   895,    57,    91,    92,    93,    94,    95,   897,    -1,
      -1,    57,    -1,    -1,    -1,    -1,   130,    -1,   132,    -1,
      93,    94,    95,    -1,   918,   139,   140,    -1,    -1,    -1,
      -1,    -1,    93,    94,    95,    -1,    -1,    -1,    93,    94,
      95,    -1,    -1,    -1,    -1,    -1,    -1,    93,    94,    95,
     944,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    -1,    50,    -1,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    -1,
      62,    63,    -1,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   129,   130,    -1,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   147,   148,    -1,   150,   151,
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
     103,   104,   105,   106,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   129,   130,   131,   132,
      -1,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   147,   148,    -1,   150,   151,     3,
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
     104,   105,   106,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   129,   130,    -1,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   148,    -1,   150,   151,     3,     4,
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
     105,   106,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   129,   130,    -1,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,    -1,   150,   151,     3,     4,     5,
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
     106,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   129,   130,   131,   132,    -1,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   148,    -1,   150,   151,     3,     4,     5,     6,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   129,   130,   131,   132,    -1,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     147,   148,    -1,   150,   151,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    -1,    50,    -1,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    -1,    62,    63,    -1,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   129,   130,    -1,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,    -1,   150,   151,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      -1,    50,    -1,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    -1,    62,    63,    -1,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     129,   130,    -1,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
      -1,   150,   151,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    -1,
      50,    -1,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    -1,    62,    63,    -1,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   129,
     130,   131,   132,    -1,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,    -1,
     150,   151,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    -1,    50,
      -1,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      -1,    62,    63,    -1,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   129,   130,
     131,   132,    -1,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,   148,    -1,   150,
     151,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    -1,    50,    -1,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    -1,
      62,    63,    -1,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   129,   130,   131,
     132,    -1,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   147,   148,    -1,   150,   151,
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
     103,   104,   105,   106,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   129,   130,    -1,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   147,   148,    -1,   150,   151,     3,
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
     104,   105,   106,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   129,   130,    -1,   132,    -1,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   148,   149,   150,   151,     3,     4,
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
     105,   106,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   129,   130,   131,   132,    -1,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,    -1,   150,   151,     3,     4,     5,
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
     106,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   129,   130,    -1,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   148,    -1,   150,   151,     3,     4,     5,     6,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   129,   130,   131,   132,    -1,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     147,   148,    -1,   150,   151,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    -1,    50,    -1,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    -1,    62,    63,    -1,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   129,   130,    -1,   132,    -1,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,    -1,   150,   151,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      -1,    50,    -1,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    -1,    62,    63,    -1,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   130,   131,   132,    -1,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
      -1,   150,   151,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    -1,
      50,    -1,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    -1,    62,    63,    -1,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   129,
     130,    -1,   132,    -1,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,    -1,
     150,   151,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    -1,    50,
      -1,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      -1,    62,    63,    -1,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,     9,    -1,    11,    12,
      13,    14,    15,    16,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   129,   130,
      -1,   132,    -1,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,   148,    -1,   150,
     151,     3,     4,    -1,    57,    -1,     8,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      93,    43,    -1,    -1,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    -1,    56,    57,    58,    59,    60,    61,
      -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,   128,   129,     3,     4,
      -1,    -1,    -1,     8,     9,    -1,    -1,    -1,    -1,   141,
      -1,    -1,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,     9,    -1,    43,    -1,
      -1,    46,    47,    48,    -1,    50,    51,    -1,    53,    -1,
      -1,    -1,    57,    58,    59,    -1,    -1,    -1,    -1,    64,
      -1,    -1,    -1,    -1,    37,    38,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    57,    -1,    -1,    -1,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      93,    94,    95,     3,     4,    -1,    -1,    -1,     8,     9,
      -1,    -1,    -1,    -1,    -1,    -1,   141,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,     9,    -1,    43,    -1,    -1,    -1,    47,    48,    -1,
      50,    51,    -1,    53,    -1,    -1,    -1,    57,    58,    59,
      -1,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    37,
      38,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    57,
      -1,    -1,    -1,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    93,    94,    95,     3,     4,
      -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,
      -1,   141,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    -1,    -1,    43,    -1,
      -1,    -1,    47,    48,    -1,    50,    51,    -1,    53,    -1,
      -1,    -1,    57,    58,    59,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     3,     4,    -1,    -1,    -1,    -1,     9,
      -1,    -1,    -1,    -1,    -1,    -1,   141,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    -1,    -1,    43,    -1,    -1,    -1,    47,    48,    -1,
      -1,    -1,    -1,    53,    -1,    -1,    -1,    57,    58,    -1,
      -1,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,
      -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,
      -1,   141,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    -1,    -1,    43,    -1,
      -1,    -1,    47,    48,    -1,    -1,    -1,    -1,    53,    -1,
      -1,    -1,    57,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     3,     4,    -1,    -1,    -1,     8,
       9,    -1,    -1,    -1,    -1,    -1,    -1,   132,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    -1,    43,    -1,    -1,    46,    47,    48,
      -1,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,    -1,
     129,     3,     4,    -1,    -1,    -1,     8,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    -1,
      -1,    43,    -1,    -1,    46,    47,    48,    -1,    50,    51,
      -1,    53,    -1,    -1,    -1,    57,    58,    59,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,     3,     4,    -1,    -1,    -1,
      -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    -1,    -1,    43,    -1,    -1,    -1,    47,
      48,    -1,    50,    51,    -1,    53,    -1,    -1,    -1,    57,
      58,    59,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,     3,
       4,    -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    -1,    -1,    43,
      -1,    -1,    -1,    47,    48,    -1,    -1,    -1,    -1,    53,
      -1,    -1,    -1,    57,    58,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,     3,     4,    -1,    -1,    -1,    -1,     9,
      10,    -1,    -1,    -1,    -1,    -1,    -1,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    -1,    -1,    43,    -1,    -1,    -1,    47,    48,    -1,
      -1,    -1,    -1,    53,    -1,    -1,    -1,    57,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,     3,     4,    -1,
      -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    -1,    -1,    43,    -1,    -1,
      -1,    47,    48,    -1,    -1,    -1,    -1,    53,    -1,    -1,
      -1,    57,    -1,    -1,    60,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,     3,     4,    -1,    -1,    -1,    -1,     9,    10,    -1,
      -1,    -1,    -1,    -1,    -1,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    -1,
      -1,    43,    -1,    -1,    -1,    47,    48,    -1,    -1,    -1,
      -1,    53,    -1,    -1,    -1,    57,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,     3,     4,    -1,    -1,    -1,
      -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    -1,    -1,    43,    -1,    -1,    -1,    47,
      48,    -1,    -1,    -1,    -1,    53,    -1,    -1,    -1,    57,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,     3,
       4,    -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    -1,    -1,    -1,
      -1,    -1,    -1,    47,    48,    -1,    -1,    -1,    -1,    53,
      -1,    -1,    -1,    57,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,     3,     4,    -1,    -1,    -1,    -1,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    -1,    -1,    -1,    -1,    -1,    -1,    47,    48,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    57,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,     4,    -1,    -1,
      -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      47,    48,    -1,    -1,    -1,    -1,    53,    -1,    62,    63,
      57,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   130,    -1,    -1,    -1,
     134,   135,    -1,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   148,    -1,   150
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   153,     0,   154,     3,     4,     8,     9,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    43,    46,    47,    48,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   129,   155,   156,   157,   159,   171,
     184,   185,   189,   190,   191,   200,   202,   203,   207,   208,
     209,   246,   274,   275,   276,   277,   278,   279,   281,   288,
     289,   293,   294,   295,   296,   310,     9,    37,    38,    57,
      93,    94,    95,   132,   273,   295,   273,   295,     3,     4,
       9,    39,    40,    57,    93,   276,   289,   130,   134,     9,
      37,    38,    57,    93,    94,    95,   173,   273,     9,    57,
      93,   132,   273,    50,    58,    59,   274,   275,   276,   289,
     276,   134,   280,     9,    37,    38,    57,    93,    94,    95,
     132,   289,   336,   130,   134,    50,    60,    10,   276,   249,
     130,   130,   134,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,    44,   142,   145,   248,   251,   252,
     290,   291,   292,   248,   248,    60,   159,   171,   184,   276,
     159,   202,   208,   274,   129,   132,   220,   220,   220,   220,
     220,     9,    37,    38,    46,    57,    91,    92,    93,    94,
      95,   204,   212,   214,   217,   221,   247,   256,   259,   265,
     273,   289,   276,   290,   279,   278,    90,    90,   129,   335,
     134,   162,   134,   160,     9,    37,    38,    57,    93,    94,
      95,     9,    57,    93,     9,    57,    93,   212,    90,   335,
     283,   132,   172,   335,   132,    10,   130,   276,    90,   135,
     192,     9,    37,    38,    57,    93,    94,    95,   281,   287,
     288,   335,   158,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    50,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    62,    63,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
     129,   130,   132,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,   148,   150,   151,
     296,   334,   338,   339,   340,   341,   335,   282,   249,   132,
     276,   129,   137,   202,   274,   335,   284,     9,    57,    93,
     273,   312,   315,   316,   273,   273,   320,   273,   273,   273,
     273,   273,   273,   273,   273,   273,   273,   273,   273,   273,
     273,   129,   249,   247,   273,   253,   145,   292,   129,   129,
     129,   242,   273,   290,   242,   242,   256,   248,   220,   220,
     273,   335,    62,    63,   130,   148,   333,   334,   212,   217,
     289,   213,   218,   249,   138,   243,   244,   256,   263,   290,
     266,   267,   268,   130,   134,    90,   287,     9,    37,    38,
      57,    93,    94,    95,   141,   209,   225,   227,   273,   287,
     289,   129,   133,   337,   338,   276,   285,   136,   167,   285,
     167,   131,   285,   174,   175,   273,   132,   133,   335,   210,
     141,   225,   289,     4,    53,   193,   195,   196,   197,   294,
     133,   132,   335,   335,   335,   335,   335,   131,   285,   129,
     153,   250,   131,   131,   285,   137,   273,   273,   273,   137,
     137,   273,   137,   137,   137,   137,   137,   137,   137,   137,
     137,   137,   137,   131,   131,   137,   247,   129,   273,   129,
     129,   129,   129,   133,   148,   148,   131,   149,   215,    90,
      43,    45,   138,   219,   219,   129,   245,   131,   256,   269,
     271,   222,   223,   232,   233,   273,   226,   130,    90,   137,
     135,     5,     6,     7,   168,   169,   287,   132,   135,   132,
     135,   133,   137,   138,   174,   133,   235,   236,   232,    90,
     135,   137,   191,   198,   273,   198,   153,   131,   131,   131,
     133,   149,   135,   133,   251,   135,   311,   137,   131,   131,
     317,   319,   137,   293,   323,   325,   327,   329,   324,   326,
     328,   330,   331,   332,   273,   149,   149,   130,    16,    16,
       9,    10,    11,    12,    13,    14,    15,    16,    57,    93,
     130,   132,   139,   140,   297,   302,   308,   309,   257,   148,
     235,   285,   130,   136,   229,   228,   141,   225,   286,   163,
     287,   287,   287,   137,   164,   161,   164,   174,     9,    11,
      12,    13,    15,    16,    57,    93,   130,   139,   140,   141,
     176,   177,   178,   182,   273,   281,   288,   133,   131,    60,
      89,   237,   239,   240,   141,   225,   194,   198,   199,   133,
     276,   313,   293,   293,   321,   131,   293,   293,   293,   293,
     293,   293,   293,   293,   293,   293,   137,   299,   216,   134,
     305,   298,   304,   303,    10,   130,   260,   267,   270,   131,
     135,   234,   231,   273,   235,   232,   285,   167,   168,     5,
       6,     7,   133,   165,   170,   167,   133,   181,   139,   140,
     142,   143,   144,   145,   146,   147,   183,   179,   211,   137,
     276,   232,   193,   243,   131,   276,   318,   131,   293,   131,
     131,   131,   131,   131,   131,   131,   131,   137,   137,   131,
     235,   306,   302,   297,     9,   309,   309,   261,   178,   272,
     130,   235,   137,   230,   130,   131,   132,     3,     4,     8,
       9,    37,    38,    41,    49,    50,    57,    61,    64,    93,
      94,    95,   128,   129,   141,   166,   171,   184,   185,   186,
     187,   189,   191,   201,   205,   206,   208,   225,   246,   274,
     310,   136,   132,   178,   180,   178,   219,   238,    91,    92,
     130,   254,   258,   264,   265,   314,   137,   322,    11,    11,
     131,   277,   131,   300,   235,   149,   224,   131,   231,   335,
     164,   129,   132,   136,   188,   273,   273,   141,   274,   276,
     187,   205,   208,   274,    64,   141,   225,   130,    50,   130,
     232,   248,   248,   187,   205,   208,   220,   220,   225,   220,
     217,   164,   131,   178,   237,   241,   254,   262,   290,   266,
     131,   336,   131,   131,   131,   135,   137,   299,   131,   235,
     230,   131,   133,   335,   336,   188,   188,   232,   217,   220,
     220,   232,     8,   205,   274,   335,   129,   129,   220,   220,
     133,   243,   131,   254,   131,   130,   301,   133,   131,   133,
     129,   131,   131,   255,   307,   297,   336,   129,   260,   309,
     129,   131
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
#line 1263 "vtkParse.y"
    { startSig(); clearTypeId(); clearTemplate(); closeComment(); }
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 1277 "vtkParse.y"
    { output_function(); }
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 1278 "vtkParse.y"
    { output_function(); }
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 1279 "vtkParse.y"
    { reject_function(); }
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 1280 "vtkParse.y"
    { output_function(); }
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 1281 "vtkParse.y"
    { reject_function(); }
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 1282 "vtkParse.y"
    { output_function(); }
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 1283 "vtkParse.y"
    { legacySig(); output_function(); }
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 1301 "vtkParse.y"
    { pushNamespace((yyvsp[(2) - (2)].str)); }
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 1302 "vtkParse.y"
    { popNamespace(); }
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 1309 "vtkParse.y"
    { start_class((yyvsp[(2) - (2)].str), 0); }
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 1310 "vtkParse.y"
    { end_class(); }
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 1311 "vtkParse.y"
    { reject_class((yyvsp[(2) - (5)].str), 0); }
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 1312 "vtkParse.y"
    { end_class(); }
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 1313 "vtkParse.y"
    { start_class((yyvsp[(2) - (2)].str), 1); }
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 1314 "vtkParse.y"
    { end_class(); }
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 1315 "vtkParse.y"
    { reject_class((yyvsp[(2) - (5)].str), 1); }
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 1316 "vtkParse.y"
    { end_class(); }
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 1322 "vtkParse.y"
    { startSig(); clearTypeId(); clearTemplate();  closeComment(); }
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 1336 "vtkParse.y"
    { output_function(); }
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 1337 "vtkParse.y"
    { ClassInfo *tmpc = currentClass;
     currentClass = NULL; reject_function(); currentClass = tmpc; }
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 1339 "vtkParse.y"
    { output_function(); }
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 1340 "vtkParse.y"
    { output_function(); }
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 1341 "vtkParse.y"
    { ClassInfo *tmpc = currentClass;
     currentClass = NULL; reject_function(); currentClass = tmpc; }
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 1343 "vtkParse.y"
    { output_function(); }
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 1344 "vtkParse.y"
    { legacySig(); output_function(); }
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 1357 "vtkParse.y"
    {
      vtkParse_AddStringToArray(&currentClass->SuperClasses,
                                &currentClass->NumberOfSuperClasses,
                                vtkstrdup((yyvsp[(2) - (2)].str)));
    }
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 1363 "vtkParse.y"
    {access_level = VTK_ACCESS_PUBLIC;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 1364 "vtkParse.y"
    {access_level = VTK_ACCESS_PROTECTED;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 1365 "vtkParse.y"
    {access_level = VTK_ACCESS_PRIVATE;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 1375 "vtkParse.y"
    {start_enum((yyvsp[(2) - (2)].str));}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 1376 "vtkParse.y"
    {end_enum();}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 1377 "vtkParse.y"
    {start_enum(NULL);}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 1378 "vtkParse.y"
    {end_enum();}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 1382 "vtkParse.y"
    {add_enum((yyvsp[(1) - (1)].str), NULL);}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 1383 "vtkParse.y"
    {add_enum((yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 1385 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 1386 "vtkParse.y"
    {(yyval.str) = vtkstrdup(add_const_scope((yyvsp[(1) - (1)].str)));}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 1391 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 1392 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 1393 "vtkParse.y"
    {
         (yyval.str) = vtkstrcat((yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));
       }
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 1396 "vtkParse.y"
    {postSig((yyvsp[(2) - (2)].str));}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 1397 "vtkParse.y"
    {
         (yyval.str) = vtkstrcat5((yyvsp[(1) - (4)].str), " ", (yyvsp[(2) - (4)].str), " ", (yyvsp[(4) - (4)].str));
       }
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 1400 "vtkParse.y"
    {postSig("(");}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 1401 "vtkParse.y"
    {
         (yyval.str) = vtkstrcat3("(", (yyvsp[(3) - (4)].str), ")");
       }
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 1405 "vtkParse.y"
    { (yyval.str) = "-"; }
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 1405 "vtkParse.y"
    { (yyval.str) = "+"; }
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 1406 "vtkParse.y"
    { (yyval.str) = "~"; }
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 1408 "vtkParse.y"
    { (yyval.str) = "-"; }
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 1408 "vtkParse.y"
    { (yyval.str) = "+"; }
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 1409 "vtkParse.y"
    { (yyval.str) = "*"; }
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 1409 "vtkParse.y"
    { (yyval.str) = "/"; }
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 1410 "vtkParse.y"
    { (yyval.str) = "%"; }
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 1410 "vtkParse.y"
    { (yyval.str) = "&"; }
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 1411 "vtkParse.y"
    { (yyval.str) = "|"; }
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 1411 "vtkParse.y"
    { (yyval.str) = "^"; }
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 1437 "vtkParse.y"
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
#line 1459 "vtkParse.y"
    { }
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 1460 "vtkParse.y"
    { }
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 1461 "vtkParse.y"
    { }
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 1462 "vtkParse.y"
    { }
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1464 "vtkParse.y"
    { }
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1470 "vtkParse.y"
    { postSig("template<> "); clearTypeId(); }
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1471 "vtkParse.y"
    { postSig("template<");
          clearTypeId(); startTemplate(); }
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1473 "vtkParse.y"
    { chopSig();
            if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
            postSig("> "); clearTypeId(); }
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1478 "vtkParse.y"
    { chopSig(); postSig(", "); clearTypeId(); }
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1482 "vtkParse.y"
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
#line 1492 "vtkParse.y"
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
#line 1499 "vtkParse.y"
    { pushTemplate(); }
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1500 "vtkParse.y"
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
#line 1511 "vtkParse.y"
    {postSig("class ");}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1512 "vtkParse.y"
    {postSig("typename ");}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1514 "vtkParse.y"
    { setVarName((yyvsp[(1) - (1)].str)); }
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1544 "vtkParse.y"
    {openSig(); preSig("~"); closeSig();}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1545 "vtkParse.y"
    {openSig(); preSig("~"); closeSig();}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 1547 "vtkParse.y"
    {
         openSig(); preSig("virtual ~"); closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 1554 "vtkParse.y"
    {
         openSig();
         preSig("explicit ");
         closeSig();
         currentFunction->IsExplicit = 1;
         }
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1562 "vtkParse.y"
    {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1578 "vtkParse.y"
    {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1587 "vtkParse.y"
    {
      postSig("(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1591 "vtkParse.y"
    { postSig(")"); }
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 1592 "vtkParse.y"
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
#line 1603 "vtkParse.y"
    { postSig(")"); }
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 1604 "vtkParse.y"
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
#line 1612 "vtkParse.y"
    {postSig((yyvsp[(2) - (2)].str));}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1613 "vtkParse.y"
    {
      postSig("(");
      currentFunction->IsOperator = 1;
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1618 "vtkParse.y"
    { (yyval.str) = (yyvsp[(2) - (7)].str); }
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 1620 "vtkParse.y"
    { postSig(")"); }
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 1621 "vtkParse.y"
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
#line 1631 "vtkParse.y"
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
#line 1640 "vtkParse.y"
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
#line 1650 "vtkParse.y"
    {
      postSig(" const");
      currentFunction->IsConst = 1;
    }
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 1658 "vtkParse.y"
    {
      postSig("(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 1661 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (5)].str); }
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 1662 "vtkParse.y"
    {markSig(); postSig("<");}
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 1663 "vtkParse.y"
    {
      if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
      (yyval.str) = vtkstrcat((yyvsp[(1) - (6)].str), copySig());
    }
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 1668 "vtkParse.y"
    { (yyval.str) = (yyvsp[(7) - (9)].str); }
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 1670 "vtkParse.y"
    { postSig(");"); closeSig(); }
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 1671 "vtkParse.y"
    {
      currentFunction->Name = vtkstrdup((yyvsp[(1) - (3)].str));
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 1677 "vtkParse.y"
    { postSig("("); }
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 1686 "vtkParse.y"
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
#line 1694 "vtkParse.y"
    { postSig("(");}
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 1700 "vtkParse.y"
    {clearTypeId();}
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 1703 "vtkParse.y"
    { currentFunction->IsVariadic = 1; postSig("..."); }
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 1704 "vtkParse.y"
    { clearTypeId(); }
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 1705 "vtkParse.y"
    { clearTypeId(); postSig(", "); }
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 1708 "vtkParse.y"
    { markSig(); }
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 1710 "vtkParse.y"
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
#line 1732 "vtkParse.y"
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
#line 1740 "vtkParse.y"
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
#line 1765 "vtkParse.y"
    {clearVarValue();}
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 1767 "vtkParse.y"
    { postSig("="); clearVarValue();}
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 1768 "vtkParse.y"
    { setVarValue((yyvsp[(3) - (3)].str)); }
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 1779 "vtkParse.y"
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
#line 1793 "vtkParse.y"
    {postSig(", ");}
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 1796 "vtkParse.y"
    { setStorageTypeIndirection(0); }
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 1797 "vtkParse.y"
    { setStorageTypeIndirection((yyvsp[(1) - (1)].integer)); }
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 1801 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 1802 "vtkParse.y"
    { postSig(")"); }
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 1804 "vtkParse.y"
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
#line 1813 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 1814 "vtkParse.y"
    { postSig(")"); }
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 1816 "vtkParse.y"
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
#line 1824 "vtkParse.y"
    { postSig("("); (yyval.integer) = 0; }
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 1825 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("*");
               (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 1827 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("&");
               (yyval.integer) = VTK_PARSE_REF; }
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 1830 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("*");
               (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 1832 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("&");
               (yyval.integer) = VTK_PARSE_REF; }
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 1835 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 1836 "vtkParse.y"
    { pushFunction(); postSig("("); }
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 1837 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_FUNCTION; postSig(")"); popFunction(); }
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 1838 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_ARRAY; }
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 1841 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 1843 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(1) - (2)].integer), (yyvsp[(2) - (2)].integer));}
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 1846 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 1848 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(1) - (2)].integer), (yyvsp[(2) - (2)].integer));}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 1850 "vtkParse.y"
    {clearVarName(); chopSig();}
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 1852 "vtkParse.y"
    {setVarName((yyvsp[(1) - (1)].str));}
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 1854 "vtkParse.y"
    {clearArray();}
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 1856 "vtkParse.y"
    {clearArray();}
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 1858 "vtkParse.y"
    {postSig("[");}
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 1858 "vtkParse.y"
    {postSig("]");}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 1862 "vtkParse.y"
    {pushArraySize("");}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 1863 "vtkParse.y"
    {pushArraySize((yyvsp[(1) - (1)].str));}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 1869 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 1870 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 1871 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 1872 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 1873 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 1874 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 1875 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 1882 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer); setStorageType((yyval.integer));}
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 1883 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 1884 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 1886 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(3) - (3)].integer); setStorageType((yyval.integer));}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 1887 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 1888 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer));
      setStorageType((yyval.integer));}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 1890 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(2) - (3)].integer) | (yyvsp[(3) - (3)].integer));
      setStorageType((yyval.integer));}
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 1894 "vtkParse.y"
    {postSig("static "); (yyval.integer) = VTK_PARSE_STATIC; }
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 1895 "vtkParse.y"
    {postSig("static "); (yyval.integer) = VTK_PARSE_STATIC; }
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 1897 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 1898 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer));}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 1900 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 1901 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_CONST | (yyvsp[(2) - (2)].integer));}
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 1902 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_CONST | (yyvsp[(1) - (2)].integer));}
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 1904 "vtkParse.y"
    {postSig("const ");}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 1908 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 1910 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 1911 "vtkParse.y"
    {postSig("typename ");}
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 1912 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(3) - (3)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 294:

/* Line 1455 of yacc.c  */
#line 1915 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 295:

/* Line 1455 of yacc.c  */
#line 1916 "vtkParse.y"
    {chopSig(); if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 296:

/* Line 1455 of yacc.c  */
#line 1918 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 1919 "vtkParse.y"
    {chopSig(); if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 1921 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 1922 "vtkParse.y"
    {chopSig(); if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 1925 "vtkParse.y"
    {postSig(", ");}
    break;

  case 303:

/* Line 1455 of yacc.c  */
#line 1927 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 304:

/* Line 1455 of yacc.c  */
#line 1928 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 305:

/* Line 1455 of yacc.c  */
#line 1929 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 1930 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 307:

/* Line 1455 of yacc.c  */
#line 1931 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 1932 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 309:

/* Line 1455 of yacc.c  */
#line 1933 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 312:

/* Line 1455 of yacc.c  */
#line 1938 "vtkParse.y"
    {
             (yyval.str) = vtkstrcat3((yyvsp[(1) - (3)].str), "::", (yyvsp[(3) - (3)].str));
             preScopeSig((yyvsp[(1) - (3)].str));
           }
    break;

  case 313:

/* Line 1455 of yacc.c  */
#line 1943 "vtkParse.y"
    {
             (yyval.str) = vtkstrcat3((yyvsp[(1) - (3)].str), "::", (yyvsp[(3) - (3)].str));
             preScopeSig("");
           }
    break;

  case 321:

/* Line 1455 of yacc.c  */
#line 1964 "vtkParse.y"
    { postSig("&"); (yyval.integer) = VTK_PARSE_REF;}
    break;

  case 322:

/* Line 1455 of yacc.c  */
#line 1965 "vtkParse.y"
    { postSig("&"); (yyval.integer) = ((yyvsp[(1) - (2)].integer) | VTK_PARSE_REF);}
    break;

  case 323:

/* Line 1455 of yacc.c  */
#line 1966 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 324:

/* Line 1455 of yacc.c  */
#line 1971 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 325:

/* Line 1455 of yacc.c  */
#line 1973 "vtkParse.y"
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
#line 1984 "vtkParse.y"
    { postSig("*"); (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 1985 "vtkParse.y"
    { postSig("*const "); (yyval.integer) = VTK_PARSE_CONST_POINTER; }
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 1988 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 329:

/* Line 1455 of yacc.c  */
#line 1989 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); }
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 1990 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); }
    break;

  case 331:

/* Line 1455 of yacc.c  */
#line 1991 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 332:

/* Line 1455 of yacc.c  */
#line 1992 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 333:

/* Line 1455 of yacc.c  */
#line 1993 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 1994 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 335:

/* Line 1455 of yacc.c  */
#line 1995 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 336:

/* Line 1455 of yacc.c  */
#line 1996 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 337:

/* Line 1455 of yacc.c  */
#line 1999 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 2000 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 2003 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_STRING;}
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 2004 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNICODE_STRING;}
    break;

  case 341:

/* Line 1455 of yacc.c  */
#line 2005 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_OSTREAM; }
    break;

  case 342:

/* Line 1455 of yacc.c  */
#line 2006 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_ISTREAM; }
    break;

  case 343:

/* Line 1455 of yacc.c  */
#line 2007 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 344:

/* Line 1455 of yacc.c  */
#line 2008 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_OBJECT; }
    break;

  case 345:

/* Line 1455 of yacc.c  */
#line 2009 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_QOBJECT; }
    break;

  case 346:

/* Line 1455 of yacc.c  */
#line 2012 "vtkParse.y"
    { typeSig("void"); (yyval.integer) = VTK_PARSE_VOID;}
    break;

  case 347:

/* Line 1455 of yacc.c  */
#line 2013 "vtkParse.y"
    { typeSig("bool"); (yyval.integer) = VTK_PARSE_BOOL;}
    break;

  case 348:

/* Line 1455 of yacc.c  */
#line 2014 "vtkParse.y"
    { typeSig("ssize_t"); (yyval.integer) = VTK_PARSE_SSIZE_T;}
    break;

  case 349:

/* Line 1455 of yacc.c  */
#line 2015 "vtkParse.y"
    { typeSig("size_t"); (yyval.integer) = VTK_PARSE_SIZE_T;}
    break;

  case 350:

/* Line 1455 of yacc.c  */
#line 2016 "vtkParse.y"
    { typeSig("vtkTypeInt8"); (yyval.integer) = VTK_PARSE_INT8; }
    break;

  case 351:

/* Line 1455 of yacc.c  */
#line 2017 "vtkParse.y"
    { typeSig("vtkTypeUInt8"); (yyval.integer) = VTK_PARSE_UINT8; }
    break;

  case 352:

/* Line 1455 of yacc.c  */
#line 2018 "vtkParse.y"
    { typeSig("vtkTypeInt16"); (yyval.integer) = VTK_PARSE_INT16; }
    break;

  case 353:

/* Line 1455 of yacc.c  */
#line 2019 "vtkParse.y"
    { typeSig("vtkTypeUInt16"); (yyval.integer) = VTK_PARSE_UINT16; }
    break;

  case 354:

/* Line 1455 of yacc.c  */
#line 2020 "vtkParse.y"
    { typeSig("vtkTypeInt32"); (yyval.integer) = VTK_PARSE_INT32; }
    break;

  case 355:

/* Line 1455 of yacc.c  */
#line 2021 "vtkParse.y"
    { typeSig("vtkTypeUInt32"); (yyval.integer) = VTK_PARSE_UINT32; }
    break;

  case 356:

/* Line 1455 of yacc.c  */
#line 2022 "vtkParse.y"
    { typeSig("vtkTypeInt64"); (yyval.integer) = VTK_PARSE_INT64; }
    break;

  case 357:

/* Line 1455 of yacc.c  */
#line 2023 "vtkParse.y"
    { typeSig("vtkTypeUInt64"); (yyval.integer) = VTK_PARSE_UINT64; }
    break;

  case 358:

/* Line 1455 of yacc.c  */
#line 2024 "vtkParse.y"
    { typeSig("vtkTypeFloat32"); (yyval.integer) = VTK_PARSE_FLOAT32; }
    break;

  case 359:

/* Line 1455 of yacc.c  */
#line 2025 "vtkParse.y"
    { typeSig("vtkTypeFloat64"); (yyval.integer) = VTK_PARSE_FLOAT64; }
    break;

  case 360:

/* Line 1455 of yacc.c  */
#line 2026 "vtkParse.y"
    { typeSig("vtkIdType"); (yyval.integer) = VTK_PARSE_ID_TYPE;}
    break;

  case 361:

/* Line 1455 of yacc.c  */
#line 2027 "vtkParse.y"
    { typeSig("float"); (yyval.integer) = VTK_PARSE_FLOAT;}
    break;

  case 362:

/* Line 1455 of yacc.c  */
#line 2028 "vtkParse.y"
    { typeSig("double"); (yyval.integer) = VTK_PARSE_DOUBLE;}
    break;

  case 363:

/* Line 1455 of yacc.c  */
#line 2029 "vtkParse.y"
    { typeSig("long double"); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 364:

/* Line 1455 of yacc.c  */
#line 2030 "vtkParse.y"
    { typeSig("char"); (yyval.integer) = VTK_PARSE_CHAR;}
    break;

  case 365:

/* Line 1455 of yacc.c  */
#line 2031 "vtkParse.y"
    { typeSig("signed char"); (yyval.integer) = VTK_PARSE_SIGNED_CHAR;}
    break;

  case 366:

/* Line 1455 of yacc.c  */
#line 2033 "vtkParse.y"
    { typeSig("unsigned char"); (yyval.integer) = VTK_PARSE_UNSIGNED_CHAR;}
    break;

  case 367:

/* Line 1455 of yacc.c  */
#line 2034 "vtkParse.y"
    { typeSig("int"); (yyval.integer) = VTK_PARSE_INT;}
    break;

  case 368:

/* Line 1455 of yacc.c  */
#line 2036 "vtkParse.y"
    { typeSig("unsigned int"); (yyval.integer) = VTK_PARSE_UNSIGNED_INT;}
    break;

  case 369:

/* Line 1455 of yacc.c  */
#line 2037 "vtkParse.y"
    { typeSig("short"); (yyval.integer) = VTK_PARSE_SHORT;}
    break;

  case 370:

/* Line 1455 of yacc.c  */
#line 2039 "vtkParse.y"
    { typeSig("unsigned short"); (yyval.integer) = VTK_PARSE_UNSIGNED_SHORT;}
    break;

  case 371:

/* Line 1455 of yacc.c  */
#line 2040 "vtkParse.y"
    { typeSig("long"); (yyval.integer) = VTK_PARSE_LONG;}
    break;

  case 372:

/* Line 1455 of yacc.c  */
#line 2042 "vtkParse.y"
    { typeSig("unsigned long"); (yyval.integer) = VTK_PARSE_UNSIGNED_LONG;}
    break;

  case 373:

/* Line 1455 of yacc.c  */
#line 2043 "vtkParse.y"
    { typeSig("long long"); (yyval.integer) = VTK_PARSE_LONG_LONG;}
    break;

  case 374:

/* Line 1455 of yacc.c  */
#line 2045 "vtkParse.y"
    {typeSig("unsigned long long");(yyval.integer)=VTK_PARSE_UNSIGNED_LONG_LONG;}
    break;

  case 375:

/* Line 1455 of yacc.c  */
#line 2046 "vtkParse.y"
    { typeSig("__int64"); (yyval.integer) = VTK_PARSE___INT64;}
    break;

  case 376:

/* Line 1455 of yacc.c  */
#line 2048 "vtkParse.y"
    { typeSig("unsigned __int64"); (yyval.integer) = VTK_PARSE_UNSIGNED___INT64;}
    break;

  case 377:

/* Line 1455 of yacc.c  */
#line 2049 "vtkParse.y"
    { typeSig("int"); (yyval.integer) = VTK_PARSE_INT; }
    break;

  case 378:

/* Line 1455 of yacc.c  */
#line 2050 "vtkParse.y"
    { typeSig("unsigned int"); (yyval.integer) = VTK_PARSE_UNSIGNED_INT; }
    break;

  case 379:

/* Line 1455 of yacc.c  */
#line 2056 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 380:

/* Line 1455 of yacc.c  */
#line 2057 "vtkParse.y"
    { postSig("{ "); }
    break;

  case 381:

/* Line 1455 of yacc.c  */
#line 2058 "vtkParse.y"
    {
          postSig("}");
          (yyval.str) = vtkstrcat4("{ ", (yyvsp[(3) - (6)].str), (yyvsp[(4) - (6)].str), " }");
        }
    break;

  case 384:

/* Line 1455 of yacc.c  */
#line 2065 "vtkParse.y"
    {(yyval.str) = "";}
    break;

  case 385:

/* Line 1455 of yacc.c  */
#line 2066 "vtkParse.y"
    { postSig(", "); }
    break;

  case 386:

/* Line 1455 of yacc.c  */
#line 2067 "vtkParse.y"
    {
          (yyval.str) = vtkstrcat3((yyvsp[(1) - (4)].str), ", ", (yyvsp[(4) - (4)].str));
        }
    break;

  case 387:

/* Line 1455 of yacc.c  */
#line 2071 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 388:

/* Line 1455 of yacc.c  */
#line 2072 "vtkParse.y"
    {postSig("+");}
    break;

  case 389:

/* Line 1455 of yacc.c  */
#line 2072 "vtkParse.y"
    {(yyval.str) = (yyvsp[(3) - (3)].str);}
    break;

  case 390:

/* Line 1455 of yacc.c  */
#line 2073 "vtkParse.y"
    {postSig("-");}
    break;

  case 391:

/* Line 1455 of yacc.c  */
#line 2074 "vtkParse.y"
    {
             (yyval.str) = vtkstrcat("-", (yyvsp[(3) - (3)].str));
             }
    break;

  case 392:

/* Line 1455 of yacc.c  */
#line 2077 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 393:

/* Line 1455 of yacc.c  */
#line 2078 "vtkParse.y"
    {postSig("(");}
    break;

  case 394:

/* Line 1455 of yacc.c  */
#line 2078 "vtkParse.y"
    {postSig(")"); (yyval.str) = (yyvsp[(3) - (4)].str);}
    break;

  case 395:

/* Line 1455 of yacc.c  */
#line 2079 "vtkParse.y"
    {postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 396:

/* Line 1455 of yacc.c  */
#line 2081 "vtkParse.y"
    {
             chopSig();
             if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
             postSig(">(");
             }
    break;

  case 397:

/* Line 1455 of yacc.c  */
#line 2087 "vtkParse.y"
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
#line 2101 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 399:

/* Line 1455 of yacc.c  */
#line 2103 "vtkParse.y"
    { (yyval.str) = vtkstrcat((yyvsp[(1) - (2)].str), (yyvsp[(2) - (2)].str)); }
    break;

  case 400:

/* Line 1455 of yacc.c  */
#line 2105 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 401:

/* Line 1455 of yacc.c  */
#line 2106 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 402:

/* Line 1455 of yacc.c  */
#line 2107 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 403:

/* Line 1455 of yacc.c  */
#line 2108 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 404:

/* Line 1455 of yacc.c  */
#line 2109 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 405:

/* Line 1455 of yacc.c  */
#line 2110 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 406:

/* Line 1455 of yacc.c  */
#line 2111 "vtkParse.y"
    {(yyval.str) = vtkstrdup(add_const_scope((yyvsp[(1) - (1)].str)));
                postSig((yyvsp[(1) - (1)].str));}
    break;

  case 407:

/* Line 1455 of yacc.c  */
#line 2113 "vtkParse.y"
    {(yyval.str) = vtkstrdup(add_const_scope((yyvsp[(1) - (1)].str)));
                postSig((yyvsp[(1) - (1)].str));}
    break;

  case 408:

/* Line 1455 of yacc.c  */
#line 2115 "vtkParse.y"
    {(yyval.str) = vtkstrdup(add_const_scope((yyvsp[(1) - (1)].str)));
                postSig((yyvsp[(1) - (1)].str));}
    break;

  case 409:

/* Line 1455 of yacc.c  */
#line 2125 "vtkParse.y"
    {preSig("void Set"); postSig("(");}
    break;

  case 410:

/* Line 1455 of yacc.c  */
#line 2126 "vtkParse.y"
    {
   postSig("a);");
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (7)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, (yyvsp[(6) - (7)].integer), getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 411:

/* Line 1455 of yacc.c  */
#line 2134 "vtkParse.y"
    {postSig("Get");}
    break;

  case 412:

/* Line 1455 of yacc.c  */
#line 2135 "vtkParse.y"
    {markSig();}
    break;

  case 413:

/* Line 1455 of yacc.c  */
#line 2135 "vtkParse.y"
    {swapSig();}
    break;

  case 414:

/* Line 1455 of yacc.c  */
#line 2136 "vtkParse.y"
    {
   postSig("();");
   currentFunction->Name = vtkstrcat("Get", (yyvsp[(4) - (9)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, (yyvsp[(7) - (9)].integer), getTypeId(), 0);
   output_function();
   }
    break;

  case 415:

/* Line 1455 of yacc.c  */
#line 2143 "vtkParse.y"
    {preSig("void Set");}
    break;

  case 416:

/* Line 1455 of yacc.c  */
#line 2144 "vtkParse.y"
    {
   postSig("(char *);");
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(4) - (5)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 417:

/* Line 1455 of yacc.c  */
#line 2152 "vtkParse.y"
    {preSig("char *Get");}
    break;

  case 418:

/* Line 1455 of yacc.c  */
#line 2153 "vtkParse.y"
    {
   postSig("();");
   currentFunction->Name = vtkstrcat("Get", (yyvsp[(4) - (5)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   output_function();
   }
    break;

  case 419:

/* Line 1455 of yacc.c  */
#line 2160 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 420:

/* Line 1455 of yacc.c  */
#line 2160 "vtkParse.y"
    {closeSig();}
    break;

  case 421:

/* Line 1455 of yacc.c  */
#line 2162 "vtkParse.y"
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

  case 422:

/* Line 1455 of yacc.c  */
#line 2190 "vtkParse.y"
    {preSig("void Set"); postSig("("); }
    break;

  case 423:

/* Line 1455 of yacc.c  */
#line 2191 "vtkParse.y"
    {
   postSig("*);");
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (7)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 424:

/* Line 1455 of yacc.c  */
#line 2199 "vtkParse.y"
    {postSig("*Get");}
    break;

  case 425:

/* Line 1455 of yacc.c  */
#line 2200 "vtkParse.y"
    {markSig();}
    break;

  case 426:

/* Line 1455 of yacc.c  */
#line 2200 "vtkParse.y"
    {swapSig();}
    break;

  case 427:

/* Line 1455 of yacc.c  */
#line 2201 "vtkParse.y"
    {
   postSig("();");
   currentFunction->Name = vtkstrcat("Get", (yyvsp[(4) - (9)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   output_function();
   }
    break;

  case 428:

/* Line 1455 of yacc.c  */
#line 2209 "vtkParse.y"
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

  case 429:

/* Line 1455 of yacc.c  */
#line 2224 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 430:

/* Line 1455 of yacc.c  */
#line 2225 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 2);
   }
    break;

  case 431:

/* Line 1455 of yacc.c  */
#line 2229 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 432:

/* Line 1455 of yacc.c  */
#line 2230 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 2);
   }
    break;

  case 433:

/* Line 1455 of yacc.c  */
#line 2234 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 434:

/* Line 1455 of yacc.c  */
#line 2235 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 3);
   }
    break;

  case 435:

/* Line 1455 of yacc.c  */
#line 2239 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 436:

/* Line 1455 of yacc.c  */
#line 2240 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 3);
   }
    break;

  case 437:

/* Line 1455 of yacc.c  */
#line 2244 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 438:

/* Line 1455 of yacc.c  */
#line 2245 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 4);
   }
    break;

  case 439:

/* Line 1455 of yacc.c  */
#line 2249 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 440:

/* Line 1455 of yacc.c  */
#line 2250 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 4);
   }
    break;

  case 441:

/* Line 1455 of yacc.c  */
#line 2254 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 442:

/* Line 1455 of yacc.c  */
#line 2255 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 6);
   }
    break;

  case 443:

/* Line 1455 of yacc.c  */
#line 2259 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 444:

/* Line 1455 of yacc.c  */
#line 2260 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 6);
   }
    break;

  case 445:

/* Line 1455 of yacc.c  */
#line 2264 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 446:

/* Line 1455 of yacc.c  */
#line 2266 "vtkParse.y"
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

  case 447:

/* Line 1455 of yacc.c  */
#line 2280 "vtkParse.y"
    {startSig();}
    break;

  case 448:

/* Line 1455 of yacc.c  */
#line 2282 "vtkParse.y"
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

  case 449:

/* Line 1455 of yacc.c  */
#line 2294 "vtkParse.y"
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

  case 450:

/* Line 1455 of yacc.c  */
#line 2327 "vtkParse.y"
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

  case 451:

/* Line 1455 of yacc.c  */
#line 2361 "vtkParse.y"
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

  case 452:

/* Line 1455 of yacc.c  */
#line 2413 "vtkParse.y"
    { (yyval.str) = "operator()"; }
    break;

  case 453:

/* Line 1455 of yacc.c  */
#line 2414 "vtkParse.y"
    { (yyval.str) = "operator[]"; }
    break;

  case 454:

/* Line 1455 of yacc.c  */
#line 2415 "vtkParse.y"
    { (yyval.str) = "operator new[]"; }
    break;

  case 455:

/* Line 1455 of yacc.c  */
#line 2416 "vtkParse.y"
    { (yyval.str) = "operator delete[]"; }
    break;

  case 457:

/* Line 1455 of yacc.c  */
#line 2419 "vtkParse.y"
    { (yyval.str) = "operator="; }
    break;

  case 458:

/* Line 1455 of yacc.c  */
#line 2420 "vtkParse.y"
    { (yyval.str) = "operator*"; }
    break;

  case 459:

/* Line 1455 of yacc.c  */
#line 2420 "vtkParse.y"
    { (yyval.str) = "operator/"; }
    break;

  case 460:

/* Line 1455 of yacc.c  */
#line 2421 "vtkParse.y"
    { (yyval.str) = "operator-"; }
    break;

  case 461:

/* Line 1455 of yacc.c  */
#line 2421 "vtkParse.y"
    { (yyval.str) = "operator+"; }
    break;

  case 462:

/* Line 1455 of yacc.c  */
#line 2422 "vtkParse.y"
    { (yyval.str) = "operator!"; }
    break;

  case 463:

/* Line 1455 of yacc.c  */
#line 2422 "vtkParse.y"
    { (yyval.str) = "operator~"; }
    break;

  case 464:

/* Line 1455 of yacc.c  */
#line 2423 "vtkParse.y"
    { (yyval.str) = "operator,"; }
    break;

  case 465:

/* Line 1455 of yacc.c  */
#line 2423 "vtkParse.y"
    { (yyval.str) = "operator<"; }
    break;

  case 466:

/* Line 1455 of yacc.c  */
#line 2424 "vtkParse.y"
    { (yyval.str) = "operator>"; }
    break;

  case 467:

/* Line 1455 of yacc.c  */
#line 2424 "vtkParse.y"
    { (yyval.str) = "operator&"; }
    break;

  case 468:

/* Line 1455 of yacc.c  */
#line 2425 "vtkParse.y"
    { (yyval.str) = "operator|"; }
    break;

  case 469:

/* Line 1455 of yacc.c  */
#line 2425 "vtkParse.y"
    { (yyval.str) = "operator^"; }
    break;

  case 470:

/* Line 1455 of yacc.c  */
#line 2426 "vtkParse.y"
    { (yyval.str) = "operator%"; }
    break;

  case 471:

/* Line 1455 of yacc.c  */
#line 2427 "vtkParse.y"
    { (yyval.str) = "operator new"; }
    break;

  case 472:

/* Line 1455 of yacc.c  */
#line 2428 "vtkParse.y"
    { (yyval.str) = "operator delete"; }
    break;

  case 473:

/* Line 1455 of yacc.c  */
#line 2429 "vtkParse.y"
    { (yyval.str) = "operator<<="; }
    break;

  case 474:

/* Line 1455 of yacc.c  */
#line 2430 "vtkParse.y"
    { (yyval.str) = "operator>>="; }
    break;

  case 475:

/* Line 1455 of yacc.c  */
#line 2431 "vtkParse.y"
    { (yyval.str) = "operator<<"; }
    break;

  case 476:

/* Line 1455 of yacc.c  */
#line 2432 "vtkParse.y"
    { (yyval.str) = "operator>>"; }
    break;

  case 477:

/* Line 1455 of yacc.c  */
#line 2433 "vtkParse.y"
    { (yyval.str) = "operator->*"; }
    break;

  case 478:

/* Line 1455 of yacc.c  */
#line 2434 "vtkParse.y"
    { (yyval.str) = "operator->"; }
    break;

  case 479:

/* Line 1455 of yacc.c  */
#line 2435 "vtkParse.y"
    { (yyval.str) = "operator+="; }
    break;

  case 480:

/* Line 1455 of yacc.c  */
#line 2436 "vtkParse.y"
    { (yyval.str) = "operator-="; }
    break;

  case 481:

/* Line 1455 of yacc.c  */
#line 2437 "vtkParse.y"
    { (yyval.str) = "operator*="; }
    break;

  case 482:

/* Line 1455 of yacc.c  */
#line 2438 "vtkParse.y"
    { (yyval.str) = "operator/="; }
    break;

  case 483:

/* Line 1455 of yacc.c  */
#line 2439 "vtkParse.y"
    { (yyval.str) = "operator%="; }
    break;

  case 484:

/* Line 1455 of yacc.c  */
#line 2440 "vtkParse.y"
    { (yyval.str) = "operator++"; }
    break;

  case 485:

/* Line 1455 of yacc.c  */
#line 2441 "vtkParse.y"
    { (yyval.str) = "operator--"; }
    break;

  case 486:

/* Line 1455 of yacc.c  */
#line 2442 "vtkParse.y"
    { (yyval.str) = "operator&="; }
    break;

  case 487:

/* Line 1455 of yacc.c  */
#line 2443 "vtkParse.y"
    { (yyval.str) = "operator|="; }
    break;

  case 488:

/* Line 1455 of yacc.c  */
#line 2444 "vtkParse.y"
    { (yyval.str) = "operator^="; }
    break;

  case 489:

/* Line 1455 of yacc.c  */
#line 2445 "vtkParse.y"
    {(yyval.str) = "operator&&=";}
    break;

  case 490:

/* Line 1455 of yacc.c  */
#line 2446 "vtkParse.y"
    {(yyval.str) = "operator||=";}
    break;

  case 491:

/* Line 1455 of yacc.c  */
#line 2447 "vtkParse.y"
    { (yyval.str) = "operator&&"; }
    break;

  case 492:

/* Line 1455 of yacc.c  */
#line 2448 "vtkParse.y"
    { (yyval.str) = "operator||"; }
    break;

  case 493:

/* Line 1455 of yacc.c  */
#line 2449 "vtkParse.y"
    { (yyval.str) = "operator=="; }
    break;

  case 494:

/* Line 1455 of yacc.c  */
#line 2450 "vtkParse.y"
    { (yyval.str) = "operator!="; }
    break;

  case 495:

/* Line 1455 of yacc.c  */
#line 2451 "vtkParse.y"
    { (yyval.str) = "operator<="; }
    break;

  case 496:

/* Line 1455 of yacc.c  */
#line 2452 "vtkParse.y"
    { (yyval.str) = "operator>="; }
    break;



/* Line 1455 of yacc.c  */
#line 7626 "vtkParse.tab.c"
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
  goto yyerrlab1;


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
#line 2476 "vtkParse.y"

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
