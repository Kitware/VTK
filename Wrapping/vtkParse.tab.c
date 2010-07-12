
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
    vtkParse_AddPointerToArray(&stringArray, &numberOfChunks, cp);
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
#line 1169 "vtkParse.tab.c"

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
     OP_LSHIFT_EQ = 319,
     OP_RSHIFT_EQ = 320,
     OP_LSHIFT = 321,
     OP_RSHIFT = 322,
     OP_ARROW_POINTER = 323,
     OP_ARROW = 324,
     OP_INCR = 325,
     OP_DECR = 326,
     OP_PLUS_EQ = 327,
     OP_MINUS_EQ = 328,
     OP_TIMES_EQ = 329,
     OP_DIVIDE_EQ = 330,
     OP_REMAINDER_EQ = 331,
     OP_AND_EQ = 332,
     OP_OR_EQ = 333,
     OP_XOR_EQ = 334,
     OP_LOGIC_AND_EQ = 335,
     OP_LOGIC_OR_EQ = 336,
     OP_LOGIC_AND = 337,
     OP_LOGIC_OR = 338,
     OP_LOGIC_EQ = 339,
     OP_LOGIC_NEQ = 340,
     OP_LOGIC_LEQ = 341,
     OP_LOGIC_GEQ = 342,
     ELLIPSIS = 343,
     DOUBLE_COLON = 344,
     LP = 345,
     LA = 346,
     StdString = 347,
     UnicodeString = 348,
     IdType = 349,
     TypeInt8 = 350,
     TypeUInt8 = 351,
     TypeInt16 = 352,
     TypeUInt16 = 353,
     TypeInt32 = 354,
     TypeUInt32 = 355,
     TypeInt64 = 356,
     TypeUInt64 = 357,
     TypeFloat32 = 358,
     TypeFloat64 = 359,
     SetMacro = 360,
     GetMacro = 361,
     SetStringMacro = 362,
     GetStringMacro = 363,
     SetClampMacro = 364,
     SetObjectMacro = 365,
     GetObjectMacro = 366,
     BooleanMacro = 367,
     SetVector2Macro = 368,
     SetVector3Macro = 369,
     SetVector4Macro = 370,
     SetVector6Macro = 371,
     GetVector2Macro = 372,
     GetVector3Macro = 373,
     GetVector4Macro = 374,
     GetVector6Macro = 375,
     SetVectorMacro = 376,
     GetVectorMacro = 377,
     ViewportCoordinateMacro = 378,
     WorldCoordinateMacro = 379,
     TypeMacro = 380,
     VTK_BYTE_SWAP_DECL = 381
   };
#endif




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 1116 "vtkParse.y"

  const char   *str;
  unsigned int  integer;



/* Line 214 of yacc.c  */
#line 1464 "vtkParse.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 1476 "vtkParse.tab.c"

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
#define YYLAST   5619

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  150
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  188
/* YYNRULES -- Number of rules.  */
#define YYNRULES  538
/* YYNRULES -- Number of states.  */
#define YYNSTATES  943

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   381

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   148,     2,     2,     2,   142,   143,     2,
     128,   129,   140,   138,   135,   137,   149,   141,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   134,   127,
     132,   136,   133,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   146,     2,   147,   145,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   130,   144,   131,   139,     2,     2,     2,
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
     125,   126
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
     521,   524,   528,   532,   536,   538,   541,   545,   546,   547,
     556,   557,   561,   562,   563,   571,   572,   576,   577,   580,
     583,   585,   587,   591,   592,   598,   599,   600,   610,   611,
     615,   616,   622,   623,   627,   628,   632,   637,   639,   640,
     646,   647,   648,   651,   653,   655,   656,   661,   662,   663,
     669,   671,   673,   676,   677,   679,   680,   684,   689,   694,
     698,   701,   702,   705,   706,   707,   712,   713,   716,   717,
     721,   724,   725,   731,   734,   735,   741,   743,   745,   747,
     749,   751,   752,   753,   758,   760,   762,   765,   767,   770,
     771,   773,   775,   776,   778,   779,   782,   783,   789,   790,
     792,   793,   795,   797,   799,   801,   803,   805,   807,   809,
     812,   815,   819,   822,   825,   829,   831,   834,   836,   839,
     841,   844,   847,   849,   851,   853,   855,   856,   860,   861,
     867,   868,   874,   876,   877,   882,   884,   886,   888,   890,
     892,   894,   896,   898,   902,   906,   908,   910,   912,   914,
     916,   918,   920,   923,   925,   927,   930,   932,   934,   936,
     939,   942,   945,   948,   951,   954,   956,   958,   960,   962,
     964,   966,   968,   970,   972,   974,   976,   978,   980,   982,
     984,   986,   988,   990,   992,   994,   996,   998,  1000,  1002,
    1004,  1006,  1008,  1010,  1012,  1014,  1016,  1018,  1020,  1022,
    1024,  1026,  1028,  1030,  1032,  1034,  1036,  1038,  1039,  1046,
    1047,  1049,  1050,  1051,  1056,  1058,  1059,  1063,  1064,  1068,
    1070,  1071,  1076,  1077,  1078,  1088,  1090,  1093,  1095,  1097,
    1099,  1101,  1103,  1105,  1107,  1109,  1110,  1118,  1119,  1120,
    1121,  1131,  1132,  1138,  1139,  1145,  1146,  1147,  1158,  1159,
    1167,  1168,  1169,  1170,  1180,  1187,  1188,  1196,  1197,  1205,
    1206,  1214,  1215,  1223,  1224,  1232,  1233,  1241,  1242,  1250,
    1251,  1259,  1260,  1270,  1271,  1281,  1286,  1291,  1299,  1302,
    1305,  1309,  1313,  1315,  1317,  1319,  1321,  1323,  1325,  1327,
    1329,  1331,  1333,  1335,  1337,  1339,  1341,  1343,  1345,  1347,
    1349,  1351,  1353,  1355,  1357,  1359,  1361,  1363,  1365,  1367,
    1369,  1371,  1373,  1375,  1377,  1379,  1381,  1383,  1385,  1387,
    1389,  1391,  1393,  1395,  1396,  1399,  1400,  1403,  1405,  1407,
    1409,  1411,  1413,  1415,  1417,  1419,  1421,  1423,  1425,  1427,
    1429,  1431,  1433,  1435,  1437,  1439,  1441,  1443,  1445,  1447,
    1449,  1451,  1453,  1455,  1457,  1459,  1461,  1463,  1465,  1467,
    1469,  1471,  1473,  1475,  1477,  1479,  1481,  1483,  1485,  1487,
    1489,  1491,  1493,  1495,  1497,  1501,  1505,  1509,  1513
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     151,     0,    -1,    -1,    -1,   151,   152,   153,    -1,   243,
      -1,   169,   245,   127,    -1,   182,   245,   127,    -1,   183,
      -1,   155,    -1,   154,    -1,   187,    -1,   157,   245,   127,
      -1,   189,   157,   245,   127,    -1,    41,    -1,   205,   217,
      -1,   189,   205,   217,    -1,   204,   217,    -1,   200,   217,
      -1,   201,   217,    -1,   189,   200,   217,    -1,   198,   217,
      -1,   306,    -1,   285,   127,    -1,     9,   128,   331,   129,
      -1,    57,   128,   331,   129,    -1,   127,    -1,    59,    10,
     130,   151,   131,    -1,    -1,    55,   285,   156,   130,   151,
     131,    -1,    55,   130,   331,   131,    -1,    -1,     4,   270,
     158,   165,   130,   162,   131,    -1,    -1,     4,   270,   132,
     281,   133,   159,   165,   130,   162,   131,    -1,    -1,     3,
     270,   160,   165,   130,   162,   131,    -1,    -1,     3,   270,
     132,   281,   133,   161,   165,   130,   162,   131,    -1,     3,
     130,   331,   131,    -1,    -1,    -1,   162,   163,   164,    -1,
     162,   168,   134,    -1,   243,    -1,   169,   245,   127,    -1,
     182,   245,   127,    -1,   183,    -1,   187,    -1,   185,    -1,
      49,   185,    -1,   184,    -1,    41,    -1,   205,   217,    -1,
      49,   205,   217,    -1,   189,   205,   217,    -1,   203,   217,
      -1,    49,   203,   217,    -1,   189,   203,   217,    -1,   199,
     217,    -1,   126,   128,   331,   129,   127,    -1,   306,    -1,
     127,    -1,    -1,   134,   166,    -1,   167,    -1,   167,   135,
     166,    -1,   283,    -1,     6,   283,    -1,     7,   283,    -1,
       5,   283,    -1,     5,    -1,     6,    -1,     7,    -1,    -1,
      39,   270,   170,   130,   172,   131,    -1,    -1,    39,   171,
     130,   172,   131,    -1,    -1,   173,    -1,   173,   135,   172,
      -1,   270,    -1,   270,   136,   176,    -1,   175,    -1,   270,
      -1,   284,    -1,   278,    -1,    16,    -1,    11,    -1,    13,
      -1,    12,    -1,    15,    -1,   174,    -1,    -1,   180,   177,
     176,    -1,    -1,   174,   181,   178,   176,    -1,    -1,   128,
     179,   176,   129,    -1,   137,    -1,   138,    -1,   139,    -1,
     137,    -1,   138,    -1,   140,    -1,   141,    -1,   142,    -1,
     143,    -1,   144,    -1,   145,    -1,    40,   270,   130,   331,
     131,    -1,    40,   130,   331,   131,    -1,    56,   332,   127,
      -1,   189,   185,    -1,     4,   270,   186,    -1,     3,   270,
     186,    -1,     3,   186,    -1,   127,    -1,   130,   331,   131,
     332,   127,    -1,   134,   332,   127,    -1,   188,   273,   253,
     127,    -1,   188,   157,   239,   127,    -1,   188,   169,   239,
     127,    -1,   188,   182,   239,   127,    -1,   188,    60,   127,
      -1,    54,    -1,    52,   132,   133,    -1,    -1,    52,   132,
     190,   191,   133,    -1,   193,    -1,    -1,   193,   135,   192,
     191,    -1,   290,   196,    -1,   195,   196,    -1,    -1,   194,
     189,   196,    -1,     4,    -1,    53,    -1,    -1,    -1,   270,
     197,   240,    -1,    61,   128,   200,   129,    -1,    61,   128,
     203,   129,    -1,   271,   214,    -1,   271,   202,   214,    -1,
     285,    89,   139,   229,    -1,    50,   285,    89,   139,   229,
      -1,   285,    89,   222,    -1,    50,   285,    89,   222,    -1,
     285,    89,   285,    89,   139,   229,    -1,    50,   285,    89,
     285,    89,   139,   229,    -1,   285,    89,   285,    89,   222,
      -1,    50,   285,    89,   285,    89,   222,    -1,   285,    89,
      -1,   202,   285,    89,    -1,   139,   229,    -1,    50,   139,
     229,    -1,     8,   139,   229,    -1,   222,    -1,    50,   222,
      -1,   271,   214,    -1,     8,   271,   214,    -1,   285,    89,
     206,    -1,   271,   202,   209,    -1,   206,    -1,   271,   209,
      -1,     8,   273,   209,    -1,    -1,    -1,    46,   271,   128,
     207,   232,   129,   208,   216,    -1,    -1,   211,   210,   216,
      -1,    -1,    -1,    46,   329,   212,   128,   213,   232,   129,
      -1,    -1,   218,   215,   216,    -1,    -1,   136,    16,    -1,
      45,    16,    -1,    43,    -1,   127,    -1,   130,   331,   131,
      -1,    -1,   270,   128,   219,   232,   129,    -1,    -1,    -1,
     270,   132,   220,   281,   133,   128,   221,   232,   129,    -1,
      -1,   224,   223,   226,    -1,    -1,   270,   128,   225,   232,
     129,    -1,    -1,   134,   228,   227,    -1,    -1,   135,   228,
     227,    -1,   270,   128,   331,   129,    -1,   230,    -1,    -1,
     270,   128,   231,   232,   129,    -1,    -1,    -1,   233,   234,
      -1,    88,    -1,   236,    -1,    -1,   236,   135,   235,   234,
      -1,    -1,    -1,   237,   273,   251,   238,   240,    -1,    60,
      -1,   270,    -1,   286,   270,    -1,    -1,   241,    -1,    -1,
     136,   242,   293,    -1,   271,   244,   246,   127,    -1,    58,
      60,   246,   127,    -1,    60,   246,   127,    -1,   253,   240,
      -1,    -1,   248,   246,    -1,    -1,    -1,   246,   135,   247,
     248,    -1,    -1,   249,   244,    -1,    -1,   286,   250,   244,
      -1,   261,   263,    -1,    -1,   255,   259,   129,   252,   257,
      -1,   262,   263,    -1,    -1,   256,   260,   129,   254,   257,
      -1,   128,    -1,    90,    -1,    91,    -1,    90,    -1,    91,
      -1,    -1,    -1,   128,   258,   232,   129,    -1,   264,    -1,
     251,    -1,   286,   251,    -1,   253,    -1,   286,   253,    -1,
      -1,   262,    -1,   270,    -1,    -1,   264,    -1,    -1,   265,
     266,    -1,    -1,   268,   146,   267,   269,   147,    -1,    -1,
     266,    -1,    -1,   176,    -1,    57,    -1,     9,    -1,    38,
      -1,    37,    -1,    92,    -1,    93,    -1,   273,    -1,    51,
     273,    -1,    59,   273,    -1,    59,    10,   273,    -1,    50,
     273,    -1,   272,   273,    -1,    50,   272,   273,    -1,    58,
      -1,    58,    50,    -1,   274,    -1,   274,   286,    -1,   276,
      -1,   275,   276,    -1,   276,   275,    -1,    43,    -1,   289,
      -1,   278,    -1,   284,    -1,    -1,    53,   277,   283,    -1,
      -1,    57,   132,   279,   281,   133,    -1,    -1,     9,   132,
     280,   281,   133,    -1,   273,    -1,    -1,   273,   135,   282,
     281,    -1,    57,    -1,     9,    -1,    38,    -1,    37,    -1,
      92,    -1,    93,    -1,   278,    -1,   284,    -1,   285,    89,
     283,    -1,   278,    89,   283,    -1,     9,    -1,    57,    -1,
      38,    -1,    37,    -1,    92,    -1,    93,    -1,   143,    -1,
     287,   143,    -1,   287,    -1,   288,    -1,   287,   288,    -1,
     140,    -1,    44,    -1,   290,    -1,     4,   291,    -1,     3,
     291,    -1,    40,     9,    -1,    40,    57,    -1,    39,     9,
      -1,    39,    57,    -1,   292,    -1,   291,    -1,    92,    -1,
      93,    -1,    37,    -1,    38,    -1,     9,    -1,    57,    -1,
      33,    -1,    34,    -1,    35,    -1,    36,    -1,    95,    -1,
      96,    -1,    97,    -1,    98,    -1,    99,    -1,   100,    -1,
     101,    -1,   102,    -1,   103,    -1,   104,    -1,    94,    -1,
      17,    -1,    18,    -1,    19,    -1,    30,    -1,    31,    -1,
      32,    -1,    20,    -1,    21,    -1,    22,    -1,    23,    -1,
      24,    -1,    25,    -1,    26,    -1,    27,    -1,    28,    -1,
      29,    -1,    48,    -1,    47,    -1,   298,    -1,    -1,   130,
     294,   293,   296,   295,   131,    -1,    -1,   135,    -1,    -1,
      -1,   296,   135,   297,   293,    -1,   305,    -1,    -1,   138,
     299,   305,    -1,    -1,   137,   300,   305,    -1,   304,    -1,
      -1,   128,   301,   298,   129,    -1,    -1,    -1,     9,   132,
     302,   274,   133,   128,   303,   305,   129,    -1,    10,    -1,
     304,    10,    -1,    16,    -1,    11,    -1,    13,    -1,    12,
      -1,    14,    -1,    15,    -1,     9,    -1,    57,    -1,    -1,
     105,   128,   270,   135,   307,   273,   129,    -1,    -1,    -1,
      -1,   106,   128,   308,   270,   135,   309,   273,   310,   129,
      -1,    -1,   107,   128,   311,   270,   129,    -1,    -1,   108,
     128,   312,   270,   129,    -1,    -1,    -1,   109,   128,   270,
     135,   313,   289,   314,   135,   332,   129,    -1,    -1,   110,
     128,   270,   135,   315,   289,   129,    -1,    -1,    -1,    -1,
     111,   128,   316,   270,   135,   317,   289,   318,   129,    -1,
     112,   128,   270,   135,   289,   129,    -1,    -1,   113,   128,
     270,   135,   319,   289,   129,    -1,    -1,   117,   128,   270,
     135,   320,   289,   129,    -1,    -1,   114,   128,   270,   135,
     321,   289,   129,    -1,    -1,   118,   128,   270,   135,   322,
     289,   129,    -1,    -1,   115,   128,   270,   135,   323,   289,
     129,    -1,    -1,   119,   128,   270,   135,   324,   289,   129,
      -1,    -1,   116,   128,   270,   135,   325,   289,   129,    -1,
      -1,   120,   128,   270,   135,   326,   289,   129,    -1,    -1,
     121,   128,   270,   135,   327,   289,   135,    11,   129,    -1,
      -1,   122,   128,   270,   135,   328,   289,   135,    11,   129,
      -1,   123,   128,   270,   129,    -1,   124,   128,   270,   129,
      -1,   125,   128,   270,   135,   270,   295,   129,    -1,   128,
     129,    -1,   146,   147,    -1,    62,   146,   147,    -1,    63,
     146,   147,    -1,   330,    -1,   136,    -1,   140,    -1,   141,
      -1,   137,    -1,   138,    -1,   148,    -1,   139,    -1,   135,
      -1,   132,    -1,   133,    -1,   143,    -1,   144,    -1,   145,
      -1,   142,    -1,    62,    -1,    63,    -1,    64,    -1,    65,
      -1,    66,    -1,    67,    -1,    68,    -1,    69,    -1,    72,
      -1,    73,    -1,    74,    -1,    75,    -1,    76,    -1,    70,
      -1,    71,    -1,    77,    -1,    78,    -1,    79,    -1,    80,
      -1,    81,    -1,    82,    -1,    83,    -1,    84,    -1,    85,
      -1,    86,    -1,    87,    -1,    -1,   331,   333,    -1,    -1,
     332,   334,    -1,   127,    -1,   334,    -1,    42,    -1,   335,
      -1,   337,    -1,   336,    -1,    54,    -1,   330,    -1,   134,
      -1,   149,    -1,    89,    -1,     4,    -1,    52,    -1,    38,
      -1,    37,    -1,    92,    -1,    93,    -1,   292,    -1,    13,
      -1,    11,    -1,    12,    -1,    14,    -1,    15,    -1,    10,
      -1,    41,    -1,    43,    -1,    44,    -1,    45,    -1,     3,
      -1,    46,    -1,    58,    -1,    50,    -1,     8,    -1,    39,
      -1,    40,    -1,    53,    -1,    16,    -1,    60,    -1,    88,
      -1,     5,    -1,     7,    -1,     6,    -1,    55,    -1,    56,
      -1,    59,    -1,     9,    -1,    57,    -1,   130,   331,   131,
      -1,   146,   331,   147,    -1,   128,   331,   129,    -1,    90,
     331,   129,    -1,    91,   331,   129,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,  1259,  1259,  1260,  1259,  1264,  1265,  1266,  1267,  1268,
    1269,  1270,  1271,  1272,  1273,  1274,  1275,  1276,  1277,  1278,
    1279,  1280,  1281,  1282,  1283,  1284,  1285,  1291,  1297,  1297,
    1299,  1305,  1305,  1307,  1307,  1309,  1309,  1311,  1311,  1313,
    1316,  1318,  1317,  1320,  1323,  1324,  1325,  1326,  1327,  1328,
    1329,  1330,  1331,  1332,  1333,  1335,  1336,  1337,  1339,  1340,
    1341,  1342,  1343,  1345,  1345,  1347,  1347,  1349,  1350,  1351,
    1352,  1359,  1360,  1361,  1371,  1371,  1373,  1373,  1376,  1376,
    1376,  1378,  1379,  1381,  1382,  1382,  1382,  1384,  1384,  1384,
    1384,  1384,  1386,  1387,  1387,  1391,  1391,  1395,  1395,  1400,
    1400,  1401,  1403,  1403,  1404,  1404,  1405,  1405,  1406,  1406,
    1412,  1413,  1415,  1417,  1419,  1420,  1421,  1423,  1424,  1425,
    1431,  1454,  1455,  1456,  1457,  1459,  1465,  1466,  1466,  1472,
    1473,  1473,  1476,  1486,  1494,  1494,  1506,  1507,  1509,  1509,
    1509,  1516,  1518,  1524,  1526,  1527,  1528,  1529,  1530,  1531,
    1532,  1533,  1534,  1536,  1537,  1539,  1540,  1541,  1546,  1547,
    1548,  1549,  1557,  1558,  1561,  1562,  1563,  1573,  1577,  1572,
    1589,  1589,  1598,  1599,  1598,  1606,  1606,  1615,  1616,  1625,
    1635,  1641,  1641,  1644,  1643,  1648,  1649,  1648,  1656,  1656,
    1663,  1663,  1665,  1665,  1667,  1667,  1669,  1671,  1680,  1680,
    1686,  1686,  1686,  1689,  1690,  1691,  1691,  1694,  1696,  1694,
    1725,  1749,  1749,  1751,  1751,  1753,  1753,  1760,  1761,  1762,
    1764,  1775,  1776,  1778,  1779,  1779,  1782,  1782,  1783,  1783,
    1787,  1788,  1788,  1799,  1800,  1800,  1810,  1811,  1813,  1816,
    1818,  1821,  1822,  1822,  1824,  1827,  1828,  1832,  1833,  1836,
    1836,  1838,  1840,  1840,  1842,  1842,  1844,  1844,  1846,  1846,
    1848,  1849,  1855,  1856,  1857,  1858,  1859,  1860,  1867,  1868,
    1869,  1870,  1872,  1873,  1875,  1879,  1880,  1882,  1883,  1885,
    1886,  1887,  1889,  1891,  1892,  1894,  1896,  1896,  1900,  1900,
    1903,  1903,  1907,  1907,  1907,  1909,  1910,  1911,  1912,  1913,
    1914,  1915,  1916,  1918,  1923,  1929,  1929,  1929,  1929,  1929,
    1929,  1945,  1946,  1947,  1952,  1953,  1965,  1966,  1969,  1970,
    1971,  1972,  1973,  1974,  1975,  1978,  1979,  1982,  1983,  1984,
    1985,  1986,  1987,  1990,  1991,  1992,  1993,  1994,  1995,  1996,
    1997,  1998,  1999,  2000,  2001,  2002,  2003,  2004,  2005,  2006,
    2007,  2008,  2009,  2010,  2012,  2013,  2015,  2016,  2018,  2019,
    2021,  2022,  2024,  2025,  2027,  2028,  2034,  2035,  2035,  2041,
    2041,  2043,  2044,  2044,  2049,  2050,  2050,  2051,  2051,  2055,
    2056,  2056,  2057,  2059,  2057,  2079,  2080,  2083,  2084,  2085,
    2086,  2087,  2088,  2089,  2091,  2101,  2101,  2110,  2111,  2111,
    2110,  2119,  2119,  2128,  2128,  2136,  2136,  2136,  2166,  2165,
    2175,  2176,  2176,  2175,  2184,  2200,  2200,  2205,  2205,  2210,
    2210,  2215,  2215,  2220,  2220,  2225,  2225,  2230,  2230,  2235,
    2235,  2240,  2240,  2256,  2256,  2269,  2302,  2336,  2389,  2390,
    2391,  2392,  2393,  2395,  2396,  2396,  2397,  2397,  2398,  2398,
    2399,  2399,  2400,  2400,  2401,  2401,  2402,  2403,  2404,  2405,
    2406,  2407,  2408,  2409,  2410,  2411,  2412,  2413,  2414,  2415,
    2416,  2417,  2418,  2419,  2420,  2421,  2422,  2423,  2424,  2425,
    2426,  2427,  2428,  2434,  2434,  2435,  2435,  2437,  2437,  2439,
    2439,  2439,  2439,  2439,  2440,  2440,  2440,  2440,  2440,  2440,
    2441,  2441,  2441,  2441,  2441,  2442,  2442,  2442,  2442,  2442,
    2443,  2443,  2443,  2443,  2443,  2443,  2444,  2444,  2444,  2444,
    2444,  2444,  2444,  2445,  2445,  2445,  2445,  2445,  2445,  2446,
    2446,  2446,  2446,  2446,  2448,  2449,  2450,  2450,  2450
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
  "NEW", "DELETE", "OP_LSHIFT_EQ", "OP_RSHIFT_EQ", "OP_LSHIFT",
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
  "method", "scoped_operator", "operator", "typecast_op_func", "$@17",
  "$@18", "op_func", "$@19", "op_sig", "$@20", "$@21", "func", "$@22",
  "func_trailer", "func_body", "func_sig", "$@23", "$@24", "@25",
  "constructor", "$@26", "constructor_sig", "$@27", "maybe_initializers",
  "more_initializers", "initializer", "destructor", "destructor_sig",
  "$@28", "args_list", "$@29", "more_args", "$@30", "arg", "$@31", "$@32",
  "maybe_indirect_id", "maybe_var_assign", "var_assign", "$@33", "var",
  "var_id_maybe_assign", "maybe_vars", "maybe_other_vars", "$@34",
  "other_var", "$@35", "$@36", "maybe_complex_var_id", "$@37",
  "complex_var_id", "$@38", "p_or_lp_or_la", "lp_or_la",
  "maybe_array_or_args", "$@39", "maybe_indirect_maybe_var_id",
  "maybe_indirect_var_id", "maybe_var_id", "var_id", "maybe_var_array",
  "var_array", "$@40", "array", "$@41", "more_array", "array_size",
  "any_id", "storage_type", "static_mod", "type", "type_red", "const_mod",
  "type_red1", "$@42", "templated_id", "$@43", "$@44", "types", "$@45",
  "maybe_scoped_id", "scoped_id", "class_id", "type_indirection",
  "pointers", "pointer_or_const_pointer", "type_red2", "type_simple",
  "type_id", "type_primitive", "value", "$@46", "maybe_comma",
  "more_values", "$@47", "literal", "$@48", "$@49", "$@50", "$@51", "$@52",
  "string_literal", "literal2", "macro", "$@53", "$@54", "$@55", "$@56",
  "$@57", "$@58", "$@59", "$@60", "$@61", "$@62", "$@63", "$@64", "$@65",
  "$@66", "$@67", "$@68", "$@69", "$@70", "$@71", "$@72", "$@73", "$@74",
  "op_token", "op_token_no_delim", "maybe_other", "maybe_other_no_semi",
  "other_stuff", "other_stuff_no_semi", "braces", "brackets", "parens", 0
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
     375,   376,   377,   378,   379,   380,   381,    59,    40,    41,
     123,   125,    60,    62,    58,    44,    61,    45,    43,   126,
      42,    47,    37,    38,   124,    94,    91,    93,    33,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   150,   151,   152,   151,   153,   153,   153,   153,   153,
     153,   153,   153,   153,   153,   153,   153,   153,   153,   153,
     153,   153,   153,   153,   153,   153,   153,   154,   156,   155,
     155,   158,   157,   159,   157,   160,   157,   161,   157,   157,
     162,   163,   162,   162,   164,   164,   164,   164,   164,   164,
     164,   164,   164,   164,   164,   164,   164,   164,   164,   164,
     164,   164,   164,   165,   165,   166,   166,   167,   167,   167,
     167,   168,   168,   168,   170,   169,   171,   169,   172,   172,
     172,   173,   173,   174,   174,   174,   174,   175,   175,   175,
     175,   175,   176,   177,   176,   178,   176,   179,   176,   180,
     180,   180,   181,   181,   181,   181,   181,   181,   181,   181,
     182,   182,   183,   184,   185,   185,   185,   186,   186,   186,
     187,   187,   187,   187,   187,   188,   189,   190,   189,   191,
     192,   191,   193,   193,   194,   193,   195,   195,   196,   197,
     196,   198,   199,   200,   201,   201,   201,   201,   201,   201,
     201,   201,   201,   202,   202,   203,   203,   203,   203,   203,
     203,   203,   204,   204,   205,   205,   205,   207,   208,   206,
     210,   209,   212,   213,   211,   215,   214,   216,   216,   216,
     216,   217,   217,   219,   218,   220,   221,   218,   223,   222,
     225,   224,   226,   226,   227,   227,   228,   229,   231,   230,
     232,   233,   232,   234,   234,   235,   234,   237,   238,   236,
     236,   239,   239,   240,   240,   242,   241,   243,   243,   243,
     244,   245,   245,   246,   247,   246,   249,   248,   250,   248,
     251,   252,   251,   253,   254,   253,   255,   255,   255,   256,
     256,   257,   258,   257,   257,   259,   259,   260,   260,   261,
     261,   262,   263,   263,   265,   264,   267,   266,   268,   268,
     269,   269,   270,   270,   270,   270,   270,   270,   271,   271,
     271,   271,   271,   271,   271,   272,   272,   273,   273,   274,
     274,   274,   275,   276,   276,   276,   277,   276,   279,   278,
     280,   278,   281,   282,   281,   283,   283,   283,   283,   283,
     283,   283,   283,   284,   284,   285,   285,   285,   285,   285,
     285,   286,   286,   286,   287,   287,   288,   288,   289,   289,
     289,   289,   289,   289,   289,   290,   290,   291,   291,   291,
     291,   291,   291,   292,   292,   292,   292,   292,   292,   292,
     292,   292,   292,   292,   292,   292,   292,   292,   292,   292,
     292,   292,   292,   292,   292,   292,   292,   292,   292,   292,
     292,   292,   292,   292,   292,   292,   293,   294,   293,   295,
     295,   296,   297,   296,   298,   299,   298,   300,   298,   298,
     301,   298,   302,   303,   298,   304,   304,   305,   305,   305,
     305,   305,   305,   305,   305,   307,   306,   308,   309,   310,
     306,   311,   306,   312,   306,   313,   314,   306,   315,   306,
     316,   317,   318,   306,   306,   319,   306,   320,   306,   321,
     306,   322,   306,   323,   306,   324,   306,   325,   306,   326,
     306,   327,   306,   328,   306,   306,   306,   306,   329,   329,
     329,   329,   329,   330,   330,   330,   330,   330,   330,   330,
     330,   330,   330,   330,   330,   330,   330,   330,   330,   330,
     330,   330,   330,   330,   330,   330,   330,   330,   330,   330,
     330,   330,   330,   330,   330,   330,   330,   330,   330,   330,
     330,   330,   330,   331,   331,   332,   332,   333,   333,   334,
     334,   334,   334,   334,   334,   334,   334,   334,   334,   334,
     334,   334,   334,   334,   334,   334,   334,   334,   334,   334,
     334,   334,   334,   334,   334,   334,   334,   334,   334,   334,
     334,   334,   334,   334,   334,   334,   334,   334,   334,   334,
     334,   334,   334,   334,   335,   336,   337,   337,   337
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
       2,     3,     3,     3,     1,     2,     3,     0,     0,     8,
       0,     3,     0,     0,     7,     0,     3,     0,     2,     2,
       1,     1,     3,     0,     5,     0,     0,     9,     0,     3,
       0,     5,     0,     3,     0,     3,     4,     1,     0,     5,
       0,     0,     2,     1,     1,     0,     4,     0,     0,     5,
       1,     1,     2,     0,     1,     0,     3,     4,     4,     3,
       2,     0,     2,     0,     0,     4,     0,     2,     0,     3,
       2,     0,     5,     2,     0,     5,     1,     1,     1,     1,
       1,     0,     0,     4,     1,     1,     2,     1,     2,     0,
       1,     1,     0,     1,     0,     2,     0,     5,     0,     1,
       0,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       2,     3,     2,     2,     3,     1,     2,     1,     2,     1,
       2,     2,     1,     1,     1,     1,     0,     3,     0,     5,
       0,     5,     1,     0,     4,     1,     1,     1,     1,     1,
       1,     1,     1,     3,     3,     1,     1,     1,     1,     1,
       1,     1,     2,     1,     1,     2,     1,     1,     1,     2,
       2,     2,     2,     2,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     0,     6,     0,
       1,     0,     0,     4,     1,     0,     3,     0,     3,     1,
       0,     4,     0,     0,     9,     1,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     0,     7,     0,     0,     0,
       9,     0,     5,     0,     5,     0,     0,    10,     0,     7,
       0,     0,     0,     9,     6,     0,     7,     0,     7,     0,
       7,     0,     7,     0,     7,     0,     7,     0,     7,     0,
       7,     0,     9,     0,     9,     4,     4,     7,     2,     2,
       3,     3,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     0,     2,     0,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     3,     3,     3,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       2,     3,     1,     0,     0,     0,     0,   331,   348,   349,
     350,   354,   355,   356,   357,   358,   359,   360,   361,   362,
     363,   351,   352,   353,   333,   334,   335,   336,   329,   330,
      76,     0,    14,   282,     0,   365,   364,     0,     0,     0,
     286,   125,     0,   485,   332,   275,     0,   223,     0,   327,
     328,   347,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    26,     4,    10,     9,   226,   226,   226,
       8,    11,     0,     0,     0,     0,     0,     0,     0,   164,
       5,     0,     0,   268,   277,     0,   279,   284,   285,     0,
     283,   318,   326,   325,    22,   331,   329,   330,   332,   327,
     328,   483,    35,   320,    31,   319,     0,     0,   331,     0,
       0,   332,     0,     0,   483,   290,   323,   265,   264,   324,
     266,   267,     0,    74,   321,   322,   483,     0,     0,   275,
       0,     0,     0,   272,     0,   269,   127,     0,   305,   308,
     307,   306,   309,   310,   483,    28,     0,   483,   288,   276,
     223,     0,   270,     0,     0,     0,   397,   401,   403,     0,
       0,   410,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   317,   316,   311,     0,
     223,     0,   228,   313,   314,     0,     0,     0,     0,     0,
       0,     0,   226,     0,     0,     0,   181,   483,    21,    18,
      19,    17,    15,   263,   265,   264,     0,   262,   239,   240,
     266,   267,     0,   165,   170,   143,   175,   223,   213,     0,
     252,   251,     0,   273,   278,   280,   281,     0,     0,    23,
       0,     0,    63,     0,    63,   331,   329,   330,   332,   327,
     328,   323,   324,   321,   322,   166,     0,     0,     0,    78,
       0,     0,   483,     0,   167,   274,     0,   126,   134,   296,
     298,   297,   295,   299,   300,   301,   287,   302,     0,     0,
     515,   498,   526,   528,   527,   519,   532,   510,   506,   507,
     505,   508,   509,   523,   501,   500,   520,   521,   511,   489,
     512,   513,   514,   516,   518,   499,   522,   493,   529,   530,
     533,   517,   531,   524,   457,   458,   459,   460,   461,   462,
     463,   464,   470,   471,   465,   466,   467,   468,   469,   472,
     473,   474,   475,   476,   477,   478,   479,   480,   481,   482,
     525,   497,   483,   483,   502,   503,   112,   483,   483,   451,
     452,   495,   450,   443,   446,   447,   449,   444,   445,   456,
     453,   454,   455,   483,   448,   496,   504,   494,   486,   490,
     492,   491,     0,     0,     0,     2,   271,   219,   224,     0,
       0,   263,   262,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    12,   222,   227,   251,     0,   312,
     315,     6,     7,   124,     0,   211,     0,     0,     0,     0,
       0,    20,    16,     0,     0,   457,   458,     0,     0,   172,
     442,   163,   144,     0,   177,   177,     0,   215,   220,   214,
     247,     0,     0,   233,   253,   258,   183,   185,   153,   304,
     296,   298,   297,   295,   299,   300,     0,   162,   147,   188,
       0,   303,     0,   487,    39,   484,   488,   292,     0,     0,
       0,     0,     0,    24,     0,     0,    79,    81,    78,   111,
       0,   201,     0,   148,     0,   136,   137,     0,   129,     0,
     138,   138,    30,     2,     0,     0,     0,     0,     0,    25,
       0,   218,     3,   226,   141,   395,     0,     0,     0,   405,
     408,     0,     0,   415,   419,   423,   427,   417,   421,   425,
     429,   431,   433,   435,   436,     0,   229,   121,   212,   122,
     123,   120,    13,   182,     0,     0,   438,   439,     0,   154,
     180,     0,     0,   171,   176,   217,     0,   234,   248,   255,
       0,   201,     0,   145,   197,     0,   192,   190,     0,   293,
      37,     0,     0,     0,    64,    65,    67,    40,    33,    40,
     291,    77,    78,     0,     0,   110,     0,   207,   146,     0,
     128,   130,   138,   133,   139,   132,     3,   537,   538,   536,
     534,   535,   289,    27,   225,     0,   398,   402,   404,     0,
       0,   411,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   369,   440,   441,   173,   179,   178,   393,
     385,   388,   390,   389,   391,   392,   387,   394,   380,   367,
     377,   375,   216,   366,   379,   374,   241,   256,     0,     0,
     198,     0,   189,   201,     0,   151,     0,    63,    70,    68,
      69,     0,    41,    63,    41,    80,   263,    88,    90,    89,
      91,    87,   262,    97,    99,   100,   101,    92,    83,    82,
      93,    84,    86,    85,    75,   168,   210,   203,   202,   204,
       0,     0,   152,   134,   135,   213,    29,     0,     0,   406,
       0,     0,   414,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   370,     0,   201,   382,     0,     0,     0,
       0,   386,   242,   235,   244,   260,   184,     0,   201,   194,
       0,     0,   149,   294,     0,    66,    71,    72,    73,    36,
       0,     0,     0,    32,     0,   102,   103,   104,   105,   106,
     107,   108,   109,    95,     0,   177,   205,   249,   150,   131,
     140,   396,   399,     0,   409,   412,   416,   420,   424,   428,
     418,   422,   426,   430,     0,     0,   437,     0,     0,     0,
     371,   393,   378,   376,   201,   261,     0,   186,     0,     0,
     193,   483,   191,    40,     0,     0,     0,   331,   329,   330,
      52,     0,     0,   332,     0,   327,   328,     0,    62,     0,
      42,   226,   226,    47,    51,    49,    48,     0,     0,     0,
       0,   158,    44,     0,    61,    43,    40,     0,     0,    94,
     169,   207,   237,   238,   236,   208,   249,   252,   250,     0,
     485,     0,     0,     0,   174,     0,   381,   369,     0,   257,
     201,   199,   194,     0,    41,   117,   483,   485,   116,     0,
       0,     0,     0,   268,    50,     0,     0,     0,     0,   159,
       0,   483,   155,     0,     0,   113,     0,     0,    59,    56,
      53,   160,    41,    98,    96,   206,   213,   245,     0,   249,
     230,   400,     0,   413,   432,   434,     0,   372,     0,   243,
       0,   195,   196,    38,     0,     0,   115,   114,   157,   161,
      57,    54,   156,     0,     0,     0,     0,    45,    46,    58,
      55,    34,   209,   231,   246,   407,   383,     0,   368,   187,
     485,   119,   142,     0,   241,     0,   373,     0,    60,   232,
       0,   118,   384
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     3,    84,    85,    86,   289,    87,   254,   663,
     252,   657,   662,   740,   810,   480,   574,   575,   741,    88,
     270,   142,   485,   486,   677,   678,   679,   754,   828,   744,
     680,   753,    89,    90,   814,   815,   858,    91,    92,    93,
     278,   497,   693,   498,   499,   500,   593,   695,    94,   818,
      95,    96,   232,   819,    97,    98,    99,   491,   755,   233,
     444,   234,   548,   715,   235,   445,   553,   218,   236,   561,
     562,   850,   821,   566,   469,   653,   652,   790,   729,   563,
     564,   728,   586,   587,   688,   831,   689,   690,   886,   424,
     448,   449,   556,   100,   237,   199,   173,   513,   200,   201,
     418,   835,   934,   238,   646,   836,   239,   723,   784,   888,
     451,   837,   240,   453,   454,   455,   559,   725,   560,   786,
     470,   862,   102,   103,   104,   105,   106,   157,   107,   383,
     268,   478,   656,   471,   108,   133,   202,   203,   204,   110,
     111,   112,   113,   642,   718,   714,   847,   927,   643,   720,
     719,   717,   778,   935,   644,   645,   114,   605,   394,   698,
     839,   395,   396,   609,   763,   610,   399,   701,   841,   613,
     617,   614,   618,   615,   619,   616,   620,   621,   622,   439,
     377,   250,   166,   475,   476,   379,   380,   381
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -784
static const yytype_int16 yypact[] =
{
    -784,    76,  -784,  4433,   523,   811,  5170,   191,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,    18,    39,
     813,   544,  -784,  -784,  4660,  -784,  -784,  4762,  5170,   -21,
    -784,  -784,   567,  -784,   250,   102,  4864,  -784,   -29,   129,
     158,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,    -5,    40,    42,    91,   112,   113,   127,   133,
     134,   136,   143,   145,   180,   187,   188,   193,   204,   205,
     208,   209,   214,  -784,  -784,  -784,  -784,    15,    15,    15,
    -784,  -784,  4966,  4558,    83,    83,    83,    83,    83,  -784,
    -784,   680,  5170,  -784,    34,  5272,   100,   171,  -784,   174,
    -784,  -784,  -784,  -784,  -784,   195,   253,   266,   323,   359,
     403,  -784,   213,  -784,   216,  -784,   816,   816,   -31,    48,
      56,   -23,   257,   261,  -784,  -784,   221,  -784,  -784,   238,
    -784,  -784,   241,  -784,   221,   238,  -784,   243,  4762,   331,
    5068,   260,  5170,  -784,   312,  -784,   270,  3859,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  3114,  -784,  -784,  -784,
    -784,  4319,  -784,   118,  4660,  3899,  -784,  -784,  -784,  3899,
    3899,  -784,  3899,  3899,  3899,  3899,  3899,  3899,  3899,  3899,
    3899,  3899,  3899,  3899,  3899,  3899,  -784,  -784,  -784,   275,
    -784,   803,  -784,    74,  -784,   277,   278,   283,   332,   332,
     332,   803,    15,    83,    83,   631,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,   322,   326,   328,  5471,   329,  -784,  -784,
     330,   334,   809,  -784,  -784,  -784,  -784,  -784,   284,   273,
     280,   203,   338,  -784,  -784,  -784,  -784,  3859,   110,  -784,
     909,  5170,   287,  5170,   287,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  3859,  1056,  5170,  3899,
     301,  1203,  -784,  5170,  -784,  -784,   315,  -784,  5475,   -31,
     326,   328,   -23,   330,   334,   171,  -784,  -784,  1350,   310,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  1497,  5170,   132,  -784,  -784,  -784,  -784,   320,
    3899,  -784,  -784,   309,  3899,  3899,  3899,   316,   317,  3899,
     335,   339,   343,   344,   345,   346,   347,   350,   355,   357,
     360,   336,   340,   379,  -784,   380,  -784,  -784,   803,  -784,
    -784,  -784,  -784,  -784,   367,  -784,  3899,   389,   390,   393,
     394,  -784,  -784,   203,  1644,   327,   378,   396,   311,  -784,
    -784,  -784,  -784,   377,    65,    65,   177,  -784,  -784,  -784,
    -784,   399,   803,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
     -12,    71,   144,   147,   161,   162,  3899,  -784,  -784,  -784,
     401,  -784,   445,  -784,  -784,  -784,  -784,   407,   405,   769,
     413,   411,   415,  -784,   414,   417,   424,   416,  3899,  -784,
    1791,   421,  3899,  -784,   475,  -784,  -784,   433,   435,   519,
    3899,  3899,  -784,  -784,  1938,  2085,  2232,  2379,  2526,  -784,
     439,  -784,   442,    34,  -784,  -784,   448,   455,   456,  -784,
    -784,   460,  5374,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  3899,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,   427,   450,  -784,  -784,   468,  -784,
    -784,   561,   582,  -784,  -784,  -784,   489,  -784,  -784,   453,
     457,   421,  5170,  -784,  -784,   474,   473,  -784,   337,  -784,
    -784,  3859,  3859,  3859,  -784,   477,  -784,  -784,  -784,  -784,
    -784,  -784,  3899,   430,   478,  -784,   479,     7,  -784,   426,
    -784,  -784,  3899,  -784,  -784,  -784,   483,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  5170,  -784,  -784,  -784,  5374,
    5374,  -784,   491,  5374,  5374,  5374,  5374,  5374,  5374,  5374,
    5374,  5374,  5374,   486,  -784,  -784,  -784,  -784,  -784,   490,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,   608,  -784,   -24,  -784,   494,   495,
    -784,  3899,  -784,   421,  3899,  -784,  5170,   287,  -784,  -784,
    -784,   769,    63,   287,    67,  -784,   -31,  -784,  -784,  -784,
    -784,  -784,   -23,  -784,  -784,  -784,  -784,   449,  -784,  -784,
    -784,  -784,   171,  -784,  -784,  -784,  -784,  -784,  -784,   496,
    5170,  3899,  -784,  5475,  -784,   284,  -784,   501,  5170,  -784,
     504,  5374,  -784,   509,   512,   514,   517,   520,   522,   525,
     526,   521,   527,  -784,   532,   421,  -784,   497,   489,   211,
     211,  -784,  -784,  -784,  -784,   430,  -784,   535,   421,   529,
     538,   542,  -784,  -784,   537,  -784,  -784,  -784,  -784,  -784,
    3702,   539,   545,  -784,   430,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,   430,    65,  -784,   671,  -784,  -784,
    -784,  -784,  -784,   543,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,   661,   668,  -784,   552,  5170,   553,
    -784,  -784,  -784,  -784,   421,  -784,   536,  -784,   555,  3899,
    -784,  -784,  -784,  -784,   518,   811,  4073,   -12,    71,   144,
    -784,  3827,  4196,   147,   557,   161,   162,   558,  -784,  3899,
    -784,    15,    15,  -784,  -784,  -784,  -784,  3827,    83,    83,
      83,  -784,  -784,   799,  -784,  -784,  -784,   565,   430,  -784,
    -784,     7,  -784,  -784,  -784,  -784,   269,   280,  -784,   566,
    -784,   569,   571,   572,  -784,   563,  -784,   570,   573,  -784,
     421,  -784,   529,  2673,    78,  -784,  -784,  -784,  -784,   175,
     175,  3899,  3899,   257,  -784,    83,    83,   631,  3899,  -784,
    3950,  -784,  -784,   577,   579,  -784,    83,    83,  -784,  -784,
    -784,  -784,    81,  -784,  -784,  -784,   284,  -784,   578,   671,
    -784,  -784,  3261,  -784,  -784,  -784,   562,   580,   583,  -784,
     581,  -784,  -784,  -784,  2820,  3408,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  4073,   584,  3899,  2967,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,   489,  -784,  -784,
    -784,  -784,  -784,   588,   -24,   211,  -784,  3555,  -784,  -784,
     590,  -784,  -784
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -784,  -342,  -784,  -784,  -784,  -784,  -784,    -3,  -784,  -784,
    -784,  -784,  -545,  -784,  -784,  -229,    59,  -784,  -784,   -82,
    -784,  -784,  -432,  -784,  -784,  -784,  -663,  -784,  -784,  -784,
    -784,  -784,   -75,   -15,  -784,  -719,  -664,   -13,  -784,  -457,
    -784,    36,  -784,  -784,  -784,  -784,  -455,  -784,  -784,  -784,
     -33,  -784,  -784,  -746,  -784,   -70,   482,  -784,  -784,  -117,
    -784,  -784,  -784,  -784,  -223,  -784,  -421,   -84,  -784,  -784,
    -784,  -784,  -240,  -784,  -784,  -784,  -784,  -119,   -51,  -462,
    -784,  -784,  -507,  -784,   -83,  -784,  -784,  -784,  -784,    21,
    -647,  -784,  -784,     9,  -161,   -68,  -106,  -784,   239,  -784,
    -784,  -783,  -784,  -176,  -784,  -784,  -784,  -183,  -784,  -784,
    -784,  -784,  -696,   -80,  -614,  -784,  -784,  -784,  -784,  -784,
      -4,    -1,   -34,     1,   -20,   653,   660,  -784,  -112,  -784,
    -784,  -224,  -784,  -135,   -32,    38,   -76,  -784,   564,  -322,
    -260,     0,  -150,  -681,  -784,   -78,  -784,  -784,    62,  -784,
    -784,  -784,  -784,  -784,  -784,  -669,    43,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
     554,  -115,  -761,  -784,  -160,  -784,  -784,  -784
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -371
static const yytype_int16 yytable[] =
{
     122,   124,   101,   152,   123,   125,   378,   132,   468,   442,
     209,   219,   220,   221,   222,   265,   376,   210,   501,   267,
     205,   206,   286,   214,   554,   482,   143,   147,   244,   481,
     588,   271,   724,   151,   664,   429,   493,   780,   153,   155,
     416,   109,   592,   512,   484,   285,   595,   172,   760,   288,
     782,   783,   382,   887,   648,   865,   584,   261,  -305,   196,
     213,   838,   785,   450,   384,   263,  -306,   686,   736,   737,
     738,   876,   736,   737,   738,   154,     2,  -305,   196,   892,
     165,   827,   864,   736,   737,   738,   736,   737,   738,   208,
     212,   829,   215,   211,   415,   687,   905,   241,   875,   174,
     376,   135,   434,   243,   722,   262,   924,  -308,   550,   168,
     551,   156,   459,   264,   152,   441,  -263,   376,   196,   460,
     135,   376,  -254,   175,   914,   287,   123,   125,  -307,   431,
     432,   446,   426,   426,   426,   285,   285,   694,   376,   242,
     838,   389,  -221,    33,   430,  -308,   731,   461,   462,   153,
     665,   172,   169,   275,   285,   197,    34,   490,   198,   510,
    -308,   596,   170,   452,   285,   884,  -307,   463,   176,   937,
     177,   393,   386,   390,   197,   397,   398,   198,   400,   401,
     402,   403,   404,   405,   406,   407,   408,   409,   410,   411,
     412,   413,   732,   838,   739,   906,   907,   417,   743,  -265,
     612,   552,   464,   465,   425,   425,   425,   417,   777,   903,
     216,   433,   921,   217,   197,   287,   287,   419,  -309,   178,
     781,   788,   631,   632,   633,   634,   635,   636,   433,   758,
     427,   428,   376,  -307,   287,   417,  -306,   504,   505,   922,
     179,   180,   506,   507,   287,   387,   936,  -310,   854,   466,
    -309,  -310,   477,   388,   477,   181,  -309,   536,   508,   511,
     247,   182,   183,   248,   184,   487,   940,   388,   637,   477,
     443,   185,  -264,   186,   386,  -262,   558,   848,   391,   168,
    -305,   882,   391,   817,   376,  -310,   472,   699,   700,  -266,
    -267,   703,   704,   705,   706,   707,   708,   709,   710,   711,
     712,   249,   855,   226,   555,   856,   137,   138,   187,   857,
     137,   138,   388,   196,   494,   188,   189,   196,  -305,   134,
     724,   190,  -263,   135,   460,  -263,   392,  -263,   655,  -263,
     392,   456,   191,   192,   830,   457,   193,   194,   649,  -306,
     376,   391,   195,   900,   576,   251,   460,   872,   253,   692,
     266,  -263,   461,   462,   376,   376,   376,   376,   376,   832,
     833,   140,   141,   228,   229,   140,   141,   285,  -262,   137,
     138,   269,   463,   272,   461,   462,   196,  -306,   167,   765,
    -265,   169,   168,  -265,   477,  -265,   433,  -265,   274,   392,
     516,   517,   518,  -264,   463,   521,  -264,   834,  -264,   908,
    -264,   276,   414,   277,   421,   422,   912,   464,   465,   197,
     423,  -305,   198,   197,   417,  -308,   198,  -307,  -306,  -309,
     447,   479,   538,  -310,   140,   141,  -254,   458,   734,   464,
     465,   488,   733,   501,   742,   460,   658,   659,   660,   666,
     503,   667,   668,   669,   515,   670,   671,   287,   417,   514,
    -262,   519,   520,  -262,   492,  -262,   285,  -262,   547,   285,
     285,   285,   565,   461,   462,   533,   549,   224,   225,   534,
     522,   682,   197,   544,   523,   198,   654,   285,   524,   525,
     526,   527,   528,   463,   487,   529,  -266,   672,   565,  -266,
     530,  -266,   531,  -266,   537,   532,   594,   594,   629,   630,
     631,   632,   633,   634,   635,   636,   629,   630,   631,   632,
     633,   634,   635,   636,   535,   388,   539,   540,   464,   465,
     541,   542,   230,   231,   545,   546,   576,   115,   557,   567,
    -267,   623,   115,  -267,   568,  -267,   287,  -267,   570,   287,
     287,   287,   569,   577,   578,   579,   637,   580,   581,   285,
    -200,   683,   583,   144,   637,   116,   117,   287,   673,   582,
     116,   117,   869,   477,   589,   691,   590,   674,   675,   676,
     591,    39,   602,   603,   624,   118,   158,   627,   487,   681,
     118,   137,   138,   606,   607,   608,   745,   746,   594,   747,
     748,   749,   750,   751,   752,   611,   626,   625,   628,  -259,
     881,   145,   650,   647,   159,   160,   697,   651,   685,   684,
     119,   120,   661,   682,   696,   119,   120,   638,   721,   639,
     702,   713,   716,   726,   161,   638,   640,   641,   727,   287,
     761,   756,   682,   764,   640,   641,   140,   141,   766,   909,
     391,   767,   682,   768,   881,   855,   769,   730,   856,   770,
     565,   771,   857,   121,   772,   773,   774,   477,   811,   162,
     163,   776,   775,   787,   789,   812,   791,   793,   137,   138,
     820,   792,   842,   825,   146,   826,   853,   226,   840,   843,
     391,   844,   846,   849,   851,   870,   871,   565,   392,   223,
     926,   757,   881,   683,   883,   891,   896,   164,   893,   762,
     894,   895,   899,   376,   917,   897,   918,   923,   137,   138,
     929,  -370,   683,   932,   928,   938,   682,   224,   225,   942,
     735,   681,   683,   140,   141,   813,   226,   816,   392,   759,
     467,   866,   378,   901,   878,   879,   880,   227,   852,   823,
     681,   904,   376,   873,   874,   378,   265,   877,   885,   822,
     681,   939,   604,   417,   376,   376,   916,   890,   845,   246,
     889,   832,   833,   140,   141,   245,   376,   420,   152,   898,
     228,   229,   230,   231,   571,   572,   573,   378,   279,   779,
     440,   910,   911,   824,     0,   730,     0,   376,     0,     0,
     859,   860,   919,   920,   123,   125,   683,   863,     0,   834,
     867,     0,     0,   153,     0,   565,   280,   281,   391,     0,
       0,     0,   391,     0,     0,     0,   867,     0,   223,   241,
     115,     0,   136,     0,   681,   255,   282,     0,     0,     0,
       0,     0,   417,     0,     0,     0,   137,   138,     0,     0,
     137,   138,     0,     0,     0,   226,   224,   225,   116,   117,
     137,   138,     0,   256,   257,   226,   392,   565,   433,     0,
     392,   283,   284,   433,   565,     0,   227,     0,   118,   915,
     139,     0,     0,   258,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   417,     0,     0,     0,   228,
     229,   140,   141,   228,   229,   140,   141,     0,     0,     0,
       0,   230,   231,   119,   120,   140,   141,     0,   259,   260,
       0,   433,   290,   291,   292,   293,   294,   295,   296,   297,
     298,   299,   300,   301,   302,   303,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,   304,   305,   306,   307,
     308,   309,   310,   311,   312,   313,    35,    36,     0,   314,
       0,   315,   316,   317,   318,   319,   320,   321,   322,   323,
       0,   324,   325,   326,   327,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   473,   357,     0,   358,
     474,   359,   360,   361,   362,   363,   364,   365,   366,   367,
     368,   369,   370,   371,   372,   373,     0,   374,   375,   290,
     291,   292,   293,   294,   295,   296,   297,   298,   299,   300,
     301,   302,   303,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,   304,   305,   306,   307,   308,   309,   310,
     311,   312,   313,    35,    36,     0,   314,     0,   315,   316,
     317,   318,   319,   320,   321,   322,   323,     0,   324,   325,
     326,   327,   328,   329,   330,   331,   332,   333,   334,   335,
     336,   337,   338,   339,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   473,   357,   483,   358,     0,   359,   360,
     361,   362,   363,   364,   365,   366,   367,   368,   369,   370,
     371,   372,   373,     0,   374,   375,   290,   291,   292,   293,
     294,   295,   296,   297,   298,   299,   300,   301,   302,   303,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
     304,   305,   306,   307,   308,   309,   310,   311,   312,   313,
      35,    36,     0,   314,     0,   315,   316,   317,   318,   319,
     320,   321,   322,   323,     0,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     473,   357,     0,   358,   489,   359,   360,   361,   362,   363,
     364,   365,   366,   367,   368,   369,   370,   371,   372,   373,
       0,   374,   375,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   304,   305,   306,
     307,   308,   309,   310,   311,   312,   313,    35,    36,     0,
     314,     0,   315,   316,   317,   318,   319,   320,   321,   322,
     323,     0,   324,   325,   326,   327,   328,   329,   330,   331,
     332,   333,   334,   335,   336,   337,   338,   339,   340,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   473,   357,     0,
     358,   502,   359,   360,   361,   362,   363,   364,   365,   366,
     367,   368,   369,   370,   371,   372,   373,     0,   374,   375,
     290,   291,   292,   293,   294,   295,   296,   297,   298,   299,
     300,   301,   302,   303,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,   304,   305,   306,   307,   308,   309,
     310,   311,   312,   313,    35,    36,     0,   314,     0,   315,
     316,   317,   318,   319,   320,   321,   322,   323,     0,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
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
     319,   320,   321,   322,   323,     0,   324,   325,   326,   327,
     328,   329,   330,   331,   332,   333,   334,   335,   336,   337,
     338,   339,   340,   341,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,   355,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   473,   357,     0,   358,   543,   359,   360,   361,   362,
     363,   364,   365,   366,   367,   368,   369,   370,   371,   372,
     373,     0,   374,   375,   290,   291,   292,   293,   294,   295,
     296,   297,   298,   299,   300,   301,   302,   303,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,   304,   305,
     306,   307,   308,   309,   310,   311,   312,   313,    35,    36,
       0,   314,     0,   315,   316,   317,   318,   319,   320,   321,
     322,   323,     0,   324,   325,   326,   327,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   473,   357,
       0,   358,   585,   359,   360,   361,   362,   363,   364,   365,
     366,   367,   368,   369,   370,   371,   372,   373,     0,   374,
     375,   290,   291,   292,   293,   294,   295,   296,   297,   298,
     299,   300,   301,   302,   303,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,   304,   305,   306,   307,   308,
     309,   310,   311,   312,   313,    35,    36,     0,   314,     0,
     315,   316,   317,   318,   319,   320,   321,   322,   323,     0,
     324,   325,   326,   327,   328,   329,   330,   331,   332,   333,
     334,   335,   336,   337,   338,   339,   340,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   473,   357,   597,   358,     0,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   368,
     369,   370,   371,   372,   373,     0,   374,   375,   290,   291,
     292,   293,   294,   295,   296,   297,   298,   299,   300,   301,
     302,   303,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,   304,   305,   306,   307,   308,   309,   310,   311,
     312,   313,    35,    36,     0,   314,     0,   315,   316,   317,
     318,   319,   320,   321,   322,   323,     0,   324,   325,   326,
     327,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   473,   357,   598,   358,     0,   359,   360,   361,
     362,   363,   364,   365,   366,   367,   368,   369,   370,   371,
     372,   373,     0,   374,   375,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,    35,
      36,     0,   314,     0,   315,   316,   317,   318,   319,   320,
     321,   322,   323,     0,   324,   325,   326,   327,   328,   329,
     330,   331,   332,   333,   334,   335,   336,   337,   338,   339,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,   355,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   473,
     357,   599,   358,     0,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,     0,
     374,   375,   290,   291,   292,   293,   294,   295,   296,   297,
     298,   299,   300,   301,   302,   303,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,   304,   305,   306,   307,
     308,   309,   310,   311,   312,   313,    35,    36,     0,   314,
       0,   315,   316,   317,   318,   319,   320,   321,   322,   323,
       0,   324,   325,   326,   327,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   473,   357,     0,   358,
     600,   359,   360,   361,   362,   363,   364,   365,   366,   367,
     368,   369,   370,   371,   372,   373,     0,   374,   375,   290,
     291,   292,   293,   294,   295,   296,   297,   298,   299,   300,
     301,   302,   303,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,   304,   305,   306,   307,   308,   309,   310,
     311,   312,   313,    35,    36,     0,   314,     0,   315,   316,
     317,   318,   319,   320,   321,   322,   323,     0,   324,   325,
     326,   327,   328,   329,   330,   331,   332,   333,   334,   335,
     336,   337,   338,   339,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   473,   357,     0,   358,     0,   359,   360,
     361,   362,   363,   364,   365,   366,   367,   368,   369,   370,
     371,   372,   373,   601,   374,   375,   290,   291,   292,   293,
     294,   295,   296,   297,   298,   299,   300,   301,   302,   303,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
     304,   305,   306,   307,   308,   309,   310,   311,   312,   313,
      35,    36,     0,   314,     0,   315,   316,   317,   318,   319,
     320,   321,   322,   323,     0,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     473,   357,   902,   358,     0,   359,   360,   361,   362,   363,
     364,   365,   366,   367,   368,   369,   370,   371,   372,   373,
       0,   374,   375,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   304,   305,   306,
     307,   308,   309,   310,   311,   312,   313,    35,    36,     0,
     314,     0,   315,   316,   317,   318,   319,   320,   321,   322,
     323,     0,   324,   325,   326,   327,   328,   329,   330,   331,
     332,   333,   334,   335,   336,   337,   338,   339,   340,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   473,   357,     0,
     358,   930,   359,   360,   361,   362,   363,   364,   365,   366,
     367,   368,   369,   370,   371,   372,   373,     0,   374,   375,
     290,   291,   292,   293,   294,   295,   296,   297,   298,   299,
     300,   301,   302,   303,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,   304,   305,   306,   307,   308,   309,
     310,   311,   312,   313,    35,    36,     0,   314,     0,   315,
     316,   317,   318,   319,   320,   321,   322,   323,     0,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   473,   357,   933,   358,     0,   359,
     360,   361,   362,   363,   364,   365,   366,   367,   368,   369,
     370,   371,   372,   373,     0,   374,   375,   290,   291,   292,
     293,   294,   295,   296,   297,   298,   299,   300,   301,   302,
     303,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   304,   305,   306,   307,   308,   309,   310,   311,   312,
     313,    35,    36,     0,   314,     0,   315,   316,   317,   318,
     319,   320,   321,   322,   323,     0,   324,   325,   326,   327,
     328,   329,   330,   331,   332,   333,   334,   335,   336,   337,
     338,   339,   340,   341,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,   355,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   356,   357,     0,   358,     0,   359,   360,   361,   362,
     363,   364,   365,   366,   367,   368,   369,   370,   371,   372,
     373,     0,   374,   375,   290,   291,   292,   293,   294,   295,
     296,   297,   298,   299,   300,   301,   302,   303,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,   304,   305,
     306,   307,   308,   309,   310,   311,   312,   313,    35,    36,
       0,   314,     0,   315,   316,   317,   318,   319,   320,   321,
     322,   323,     0,   324,   325,   326,   327,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   357,
     925,   358,     0,   359,   360,   361,   362,   363,   364,   365,
     366,   367,   368,   369,   370,   371,   372,   373,     0,   374,
     375,   290,   291,   292,   293,   294,   295,   296,   297,   298,
     299,   300,   301,   302,   303,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,   304,   305,   306,   307,   308,
     309,   310,   311,   312,   313,    35,    36,     0,   314,     0,
     315,   316,   317,   318,   319,   320,   321,   322,   323,     0,
     324,   325,   326,   327,   328,   329,   330,   331,   332,   333,
     334,   335,   336,   337,   338,   339,   340,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   931,   357,     0,   358,     0,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   368,
     369,   370,   371,   372,   373,     0,   374,   375,   290,   291,
     292,   293,   294,   295,   296,   297,   298,   299,   300,   301,
     302,   303,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,   304,   305,   306,   307,   308,   309,   310,   311,
     312,   313,    35,    36,     0,   314,     0,   315,   316,   317,
     318,   319,   320,   321,   322,   323,     0,   324,   325,   326,
     327,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   941,   357,     0,   358,     0,   359,   360,   361,
     362,   363,   364,   365,   366,   367,   368,   369,   370,   371,
     372,   373,     0,   374,   375,   794,   795,     0,     0,     0,
     796,   797,     0,     0,     0,     0,     0,     0,     0,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,   798,
     799,    30,    31,   800,     0,    33,     0,     0,    34,    35,
      36,   801,   802,    38,    39,    40,    41,     0,    43,   803,
      45,   150,    47,   804,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   805,   806,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,   807,   808,
     794,   795,     0,     0,     0,   796,   797,     0,     0,     0,
       0,   809,     0,     0,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,   798,   799,   129,   130,   279,     0,
      33,     0,     0,    34,    35,    36,     0,   802,    38,     0,
      40,     0,     0,     0,   803,   149,   150,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   280,   281,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   391,     0,
       0,     0,     0,     0,     0,     0,   282,     0,     0,   805,
     806,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,     0,     0,     0,     0,   137,   138,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   283,   284,   126,   127,     0,   392,     0,   913,   797,
       0,     0,     0,     0,     0,     0,   809,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   798,   799,   129,
     130,   140,   141,    33,     0,     0,     0,    35,    36,     0,
     802,    38,     0,    40,     0,     0,     0,   803,   149,   150,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   805,   806,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   126,   127,     0,     0,
       0,     0,   128,     0,     0,     0,     0,     0,     0,   809,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,   129,   130,     0,     0,    33,     0,     0,     0,
      35,    36,     0,   148,    38,     0,    40,     0,     0,     0,
     131,   149,   150,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   126,
     127,     0,     0,     0,     0,   797,     0,     0,     0,     0,
       0,     0,   861,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,   798,   799,   129,   130,     0,     0,    33,
       0,     0,     0,    35,    36,     0,     0,     0,     0,    40,
       0,     0,     0,   803,   149,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   805,   806,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   126,   127,     0,     0,     0,     0,   128,     0,
       0,     0,     0,     0,     0,   868,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,   129,   130,
       0,     0,    33,     0,     0,     0,    35,    36,     0,     0,
       0,     0,    40,     0,     0,     0,   131,     0,     0,     0,
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
       0,     0,     0,     0,     0,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,     0,
      83,     4,     5,     0,     0,     0,     6,   128,     0,     0,
       0,     0,     0,     0,     0,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,   129,   130,     0,
       0,    33,     0,     0,    34,    35,    36,     0,   148,    38,
       0,    40,     0,     0,     0,   131,   149,   150,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,   126,   127,     0,     0,     0,     0,   128,
       0,     0,     0,     0,     0,     0,     0,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,   129,
     130,     0,     0,    33,     0,     0,     0,    35,    36,     0,
     148,    38,     0,    40,     0,     0,     0,   131,   149,   150,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,   126,   127,     0,     0,     0,
       0,   128,     0,     0,     0,     0,     0,     0,     0,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,   129,   130,     0,     0,    33,     0,     0,     0,    35,
      36,     0,     0,     0,     0,    40,     0,     0,     0,   131,
     149,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,   126,   127,     0,
       0,     0,     0,   128,   171,     0,     0,     0,     0,     0,
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,   129,   130,     0,     0,    33,     0,     0,
       0,    35,    36,     0,     0,     0,     0,    40,     0,     0,
       0,   131,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,     4,
       5,     0,     0,     0,     0,   128,     0,     0,     0,     0,
       0,     0,     0,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,     0,     0,    33,
       0,     0,     0,    35,    36,     0,     0,     0,     0,    40,
       0,     0,     0,   131,     0,     0,   207,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,   126,   127,     0,     0,     0,     0,   128,   273,     0,
       0,     0,     0,     0,     0,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,   129,   130,     0,
       0,    33,     0,     0,     0,    35,    36,     0,     0,     0,
       0,    40,     0,     0,     0,   131,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,   126,   127,     0,     0,     0,     0,   128,
       0,     0,     0,     0,     0,     0,     0,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,   129,
     130,     0,     0,    33,     0,     0,     0,    35,    36,     0,
       0,     0,     0,    40,     0,     0,     0,   131,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,   126,   127,     0,     0,     0,
       0,   128,     0,     0,     0,     0,     0,     0,     0,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,   129,   130,     0,     0,     0,     0,     0,     0,    35,
      36,     0,     0,     0,     0,    40,     0,     0,     0,   131,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,   126,   127,     0,
       0,     0,     0,   255,     0,     0,     0,     0,     0,     0,
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   256,   257,   129,   130,     0,     0,     0,     0,     0,
       0,    35,    36,     0,     0,     0,     0,     0,     0,     0,
       0,   258,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   259,   260,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,   495,
       0,     0,     0,     0,   255,     0,     0,     0,     0,     0,
       0,     0,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,   256,   257,     0,     0,     0,     0,     0,     0,
       0,     0,    35,    36,     0,     0,     0,     0,   496,     0,
       0,     0,   258,   435,   436,   326,   327,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,     0,
       0,     0,     0,     0,     0,     0,     0,   259,   260,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   437,
       0,     0,     0,   359,   360,     0,   362,   363,   364,   365,
     366,   367,   368,   369,   370,   371,   372,   438,     0,   374
};

static const yytype_int16 yycheck[] =
{
       4,     5,     3,    37,     4,     5,   166,     6,   248,   232,
      92,    95,    96,    97,    98,   132,   166,    92,   278,   134,
      88,    89,   157,    93,   445,   254,    30,    31,   104,   253,
     492,   146,   646,    34,   579,   211,   276,   718,    37,    38,
     201,     3,   499,   385,   268,   157,   501,    46,   695,   164,
     719,   720,   167,   836,   561,   801,   488,     9,    89,    44,
      93,   757,   725,   239,   170,     9,    89,    60,     5,     6,
       7,   817,     5,     6,     7,    37,     0,    89,    44,   840,
      42,   744,   801,     5,     6,     7,     5,     6,     7,    92,
      93,   754,    93,    92,   200,    88,   857,   101,   817,   128,
     250,   132,   217,   102,   128,    57,   889,    89,    43,   132,
      45,   132,   247,    57,   148,   232,   128,   267,    44,     9,
     132,   271,   146,   128,   870,   157,   126,   127,    89,   213,
     214,   237,   208,   209,   210,   247,   248,   592,   288,   101,
     836,   174,   127,    43,   212,   127,   653,    37,    38,   148,
     582,   150,    50,   152,   266,   140,    46,   272,   143,   383,
      89,   503,    60,   239,   276,   828,   127,    57,   128,   930,
     128,   175,   171,   174,   140,   179,   180,   143,   182,   183,
     184,   185,   186,   187,   188,   189,   190,   191,   192,   193,
     194,   195,   654,   889,   131,   859,   860,   201,   131,   128,
     522,   136,    92,    93,   208,   209,   210,   211,   715,   131,
     127,   215,   131,   130,   140,   247,   248,   143,    89,   128,
       9,   728,    11,    12,    13,    14,    15,    16,   232,   691,
     209,   210,   382,    89,   266,   239,    89,   352,   353,   886,
     128,   128,   357,   358,   276,   127,   927,    89,   793,   139,
      89,    89,   251,   135,   253,   128,   127,   418,   373,   127,
      89,   128,   128,    89,   128,   269,   935,   135,    57,   268,
     232,   128,   128,   128,   273,   128,   452,   784,     9,   132,
      89,   826,     9,   740,   434,   127,   248,   609,   610,   128,
     128,   613,   614,   615,   616,   617,   618,   619,   620,   621,
     622,   127,   127,    46,   127,   130,    37,    38,   128,   134,
      37,    38,   135,    44,   276,   128,   128,    44,   127,   128,
     934,   128,   127,   132,     9,   130,    57,   132,   568,   134,
      57,   128,   128,   128,   755,   132,   128,   128,   562,    89,
     490,     9,   128,   850,   479,   132,     9,   809,   132,   589,
      89,   130,    37,    38,   504,   505,   506,   507,   508,    90,
      91,    92,    93,    90,    91,    92,    93,   479,   130,    37,
      38,   130,    57,   130,    37,    38,    44,   127,   128,   701,
     127,    50,   132,   130,   383,   132,   390,   134,   128,    57,
     394,   395,   396,   127,    57,   399,   130,   128,   132,   861,
     134,    89,   127,   133,   127,   127,   868,    92,    93,   140,
     127,    89,   143,   140,   418,    89,   143,    89,    89,    89,
     136,   134,   426,    89,    92,    93,   146,    89,   657,    92,
      93,   130,   656,   693,   663,     9,   571,   572,   573,     9,
     130,    11,    12,    13,   135,    15,    16,   479,   452,   129,
     127,   135,   135,   130,   139,   132,   568,   134,   147,   571,
     572,   573,   466,    37,    38,   129,    89,    37,    38,   129,
     135,   583,   140,   146,   135,   143,   139,   589,   135,   135,
     135,   135,   135,    57,   488,   135,   127,    57,   492,   130,
     135,   132,   135,   134,   127,   135,   500,   501,     9,    10,
      11,    12,    13,    14,    15,    16,     9,    10,    11,    12,
      13,    14,    15,    16,   135,   135,   127,   127,    92,    93,
     127,   127,    92,    93,   146,   129,   661,     9,   129,   128,
     127,   535,     9,   130,    89,   132,   568,   134,   133,   571,
     572,   573,   135,   130,   133,   130,    57,   133,   131,   661,
     129,   583,   136,     9,    57,    37,    38,   589,   128,   135,
      37,    38,   802,   562,    89,   139,   133,   137,   138,   139,
     135,    52,   133,   131,   147,    57,     9,    16,   582,   583,
      57,    37,    38,   135,   129,   129,   137,   138,   592,   140,
     141,   142,   143,   144,   145,   135,   128,   147,    16,   146,
     823,    57,   128,   146,    37,    38,   605,   134,   129,   131,
      92,    93,   135,   725,   131,    92,    93,   128,    10,   130,
     129,   135,   132,   129,    57,   128,   137,   138,   133,   661,
     129,   135,   744,   129,   137,   138,    92,    93,   129,   862,
       9,   129,   754,   129,   867,   127,   129,   651,   130,   129,
     654,   129,   134,   130,   129,   129,   135,   656,   740,    92,
      93,   129,   135,   128,   135,   740,   128,   130,    37,    38,
     740,   129,    11,   134,   130,   130,   791,    46,   135,    11,
       9,   129,   129,   147,   129,   128,   128,   691,    57,     9,
     128,   690,   915,   725,   129,   129,   133,   130,   129,   698,
     129,   129,   129,   853,   127,   135,   127,   129,    37,    38,
     129,   131,   744,   129,   131,   127,   828,    37,    38,   129,
     661,   725,   754,    92,    93,   740,    46,   740,    57,   693,
     248,   801,   892,   852,   818,   819,   820,    57,   789,   740,
     744,   856,   892,   811,   812,   905,   863,   817,   831,   740,
     754,   934,   513,   757,   904,   905,   871,   837,   778,   106,
     836,    90,    91,    92,    93,   105,   916,   203,   802,   847,
      90,    91,    92,    93,     5,     6,     7,   937,     9,   717,
     226,   865,   866,   740,    -1,   789,    -1,   937,    -1,    -1,
     794,   795,   876,   877,   794,   795,   828,   796,    -1,   128,
     801,    -1,    -1,   802,    -1,   809,    37,    38,     9,    -1,
      -1,    -1,     9,    -1,    -1,    -1,   817,    -1,     9,   823,
       9,    -1,     9,    -1,   828,     9,    57,    -1,    -1,    -1,
      -1,    -1,   836,    -1,    -1,    -1,    37,    38,    -1,    -1,
      37,    38,    -1,    -1,    -1,    46,    37,    38,    37,    38,
      37,    38,    -1,    37,    38,    46,    57,   861,   862,    -1,
      57,    92,    93,   867,   868,    -1,    57,    -1,    57,   870,
      57,    -1,    -1,    57,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   889,    -1,    -1,    -1,    90,
      91,    92,    93,    90,    91,    92,    93,    -1,    -1,    -1,
      -1,    92,    93,    92,    93,    92,    93,    -1,    92,    93,
      -1,   915,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    -1,    50,
      -1,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      -1,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   127,   128,    -1,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,    -1,   148,   149,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    -1,    50,    -1,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    -1,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   127,   128,   129,   130,    -1,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,    -1,   148,   149,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    -1,    50,    -1,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    -1,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     127,   128,    -1,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
      -1,   148,   149,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    -1,
      50,    -1,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    -1,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   127,   128,    -1,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,    -1,   148,   149,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    -1,    50,    -1,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    -1,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   127,   128,   129,   130,    -1,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,    -1,   148,   149,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    -1,    50,    -1,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    -1,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   127,   128,    -1,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,    -1,   148,   149,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      -1,    50,    -1,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    -1,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   127,   128,
      -1,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,    -1,   148,
     149,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    -1,    50,    -1,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    -1,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   127,   128,   129,   130,    -1,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,    -1,   148,   149,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    -1,    50,    -1,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    -1,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   127,   128,   129,   130,    -1,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,    -1,   148,   149,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    -1,    50,    -1,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    -1,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   127,
     128,   129,   130,    -1,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,    -1,
     148,   149,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    -1,    50,
      -1,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      -1,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   127,   128,    -1,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,    -1,   148,   149,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    -1,    50,    -1,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    -1,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   127,   128,    -1,   130,    -1,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   148,   149,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    -1,    50,    -1,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    -1,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     127,   128,   129,   130,    -1,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
      -1,   148,   149,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    -1,
      50,    -1,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    -1,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   127,   128,    -1,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,    -1,   148,   149,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    -1,    50,    -1,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    -1,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   127,   128,   129,   130,    -1,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,    -1,   148,   149,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    -1,    50,    -1,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    -1,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   127,   128,    -1,   130,    -1,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,    -1,   148,   149,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      -1,    50,    -1,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    -1,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   128,
     129,   130,    -1,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,    -1,   148,
     149,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    -1,    50,    -1,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    -1,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   127,   128,    -1,   130,    -1,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,    -1,   148,   149,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    -1,    50,    -1,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    -1,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   127,   128,    -1,   130,    -1,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,    -1,   148,   149,     3,     4,    -1,    -1,    -1,
       8,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    -1,    43,    -1,    -1,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    -1,    56,    57,
      58,    59,    60,    61,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
       3,     4,    -1,    -1,    -1,     8,     9,    -1,    -1,    -1,
      -1,   139,    -1,    -1,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,     9,    -1,
      43,    -1,    -1,    46,    47,    48,    -1,    50,    51,    -1,
      53,    -1,    -1,    -1,    57,    58,    59,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    37,    38,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    57,    -1,    -1,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,    -1,    -1,    -1,    -1,    37,    38,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    92,    93,     3,     4,    -1,    57,    -1,     8,     9,
      -1,    -1,    -1,    -1,    -1,    -1,   139,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    92,    93,    43,    -1,    -1,    -1,    47,    48,    -1,
      50,    51,    -1,    53,    -1,    -1,    -1,    57,    58,    59,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     3,     4,    -1,    -1,
      -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,   139,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    -1,    -1,    43,    -1,    -1,    -1,
      47,    48,    -1,    50,    51,    -1,    53,    -1,    -1,    -1,
      57,    58,    59,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,
       4,    -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,
      -1,    -1,   139,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    -1,    -1,    43,
      -1,    -1,    -1,    47,    48,    -1,    -1,    -1,    -1,    53,
      -1,    -1,    -1,    57,    58,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     3,     4,    -1,    -1,    -1,    -1,     9,    -1,
      -1,    -1,    -1,    -1,    -1,   139,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      -1,    -1,    43,    -1,    -1,    -1,    47,    48,    -1,    -1,
      -1,    -1,    53,    -1,    -1,    -1,    57,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     3,     4,    -1,    -1,
      -1,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,   130,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    -1,    43,    -1,    -1,    46,
      47,    48,    -1,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,    -1,
     127,     3,     4,    -1,    -1,    -1,     8,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    -1,
      -1,    43,    -1,    -1,    46,    47,    48,    -1,    50,    51,
      -1,    53,    -1,    -1,    -1,    57,    58,    59,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,     3,     4,    -1,    -1,    -1,    -1,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    -1,    -1,    43,    -1,    -1,    -1,    47,    48,    -1,
      50,    51,    -1,    53,    -1,    -1,    -1,    57,    58,    59,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,     3,     4,    -1,    -1,    -1,
      -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    -1,    -1,    43,    -1,    -1,    -1,    47,
      48,    -1,    -1,    -1,    -1,    53,    -1,    -1,    -1,    57,
      58,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,     3,     4,    -1,
      -1,    -1,    -1,     9,    10,    -1,    -1,    -1,    -1,    -1,
      -1,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    -1,    -1,    43,    -1,    -1,
      -1,    47,    48,    -1,    -1,    -1,    -1,    53,    -1,    -1,
      -1,    57,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,     3,
       4,    -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    -1,    -1,    43,
      -1,    -1,    -1,    47,    48,    -1,    -1,    -1,    -1,    53,
      -1,    -1,    -1,    57,    -1,    -1,    60,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,     3,     4,    -1,    -1,    -1,    -1,     9,    10,    -1,
      -1,    -1,    -1,    -1,    -1,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    -1,
      -1,    43,    -1,    -1,    -1,    47,    48,    -1,    -1,    -1,
      -1,    53,    -1,    -1,    -1,    57,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,     3,     4,    -1,    -1,    -1,    -1,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    -1,    -1,    43,    -1,    -1,    -1,    47,    48,    -1,
      -1,    -1,    -1,    53,    -1,    -1,    -1,    57,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,     3,     4,    -1,    -1,    -1,
      -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    -1,    -1,    -1,    -1,    -1,    -1,    47,
      48,    -1,    -1,    -1,    -1,    53,    -1,    -1,    -1,    57,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,     3,     4,    -1,
      -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    -1,    -1,    -1,    -1,    -1,
      -1,    47,    48,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    57,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,     4,
      -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    47,    48,    -1,    -1,    -1,    -1,    53,    -1,
      -1,    -1,    57,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   128,
      -1,    -1,    -1,   132,   133,    -1,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,    -1,   148
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   151,     0,   152,     3,     4,     8,     9,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    43,    46,    47,    48,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   127,   153,   154,   155,   157,   169,   182,
     183,   187,   188,   189,   198,   200,   201,   204,   205,   206,
     243,   271,   272,   273,   274,   275,   276,   278,   284,   285,
     289,   290,   291,   292,   306,     9,    37,    38,    57,    92,
      93,   130,   270,   291,   270,   291,     3,     4,     9,    39,
      40,    57,   273,   285,   128,   132,     9,    37,    38,    57,
      92,    93,   171,   270,     9,    57,   130,   270,    50,    58,
      59,   271,   272,   273,   285,   273,   132,   277,     9,    37,
      38,    57,    92,    93,   130,   285,   332,   128,   132,    50,
      60,    10,   273,   246,   128,   128,   128,   128,   128,   128,
     128,   128,   128,   128,   128,   128,   128,   128,   128,   128,
     128,   128,   128,   128,   128,   128,    44,   140,   143,   245,
     248,   249,   286,   287,   288,   245,   245,    60,   157,   169,
     182,   273,   157,   200,   205,   271,   127,   130,   217,   217,
     217,   217,   217,     9,    37,    38,    46,    57,    90,    91,
      92,    93,   202,   209,   211,   214,   218,   244,   253,   256,
     262,   270,   285,   273,   286,   276,   275,    89,    89,   127,
     331,   132,   160,   132,   158,     9,    37,    38,    57,    92,
      93,     9,    57,     9,    57,   209,    89,   331,   280,   130,
     170,   331,   130,    10,   128,   273,    89,   133,   190,     9,
      37,    38,    57,    92,    93,   278,   283,   284,   331,   156,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    50,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,   127,   128,   130,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   148,   149,   292,   330,   334,   335,
     336,   337,   331,   279,   246,   130,   273,   127,   135,   200,
     271,     9,    57,   270,   308,   311,   312,   270,   270,   316,
     270,   270,   270,   270,   270,   270,   270,   270,   270,   270,
     270,   270,   270,   270,   127,   246,   244,   270,   250,   143,
     288,   127,   127,   127,   239,   270,   286,   239,   239,   253,
     245,   217,   217,   270,   331,    62,    63,   128,   146,   329,
     330,   209,   214,   285,   210,   215,   246,   136,   240,   241,
     253,   260,   286,   263,   264,   265,   128,   132,    89,   283,
       9,    37,    38,    57,    92,    93,   139,   206,   222,   224,
     270,   283,   285,   127,   131,   333,   334,   273,   281,   134,
     165,   281,   165,   129,   281,   172,   173,   270,   130,   131,
     331,   207,   139,   222,   285,     4,    53,   191,   193,   194,
     195,   290,   131,   130,   331,   331,   331,   331,   331,   129,
     281,   127,   151,   247,   129,   135,   270,   270,   270,   135,
     135,   270,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   129,   129,   135,   244,   127,   270,   127,
     127,   127,   127,   131,   146,   146,   129,   147,   212,    89,
      43,    45,   136,   216,   216,   127,   242,   129,   253,   266,
     268,   219,   220,   229,   230,   270,   223,   128,    89,   135,
     133,     5,     6,     7,   166,   167,   283,   130,   133,   130,
     133,   131,   135,   136,   172,   131,   232,   233,   229,    89,
     133,   135,   189,   196,   270,   196,   151,   129,   129,   129,
     131,   147,   133,   131,   248,   307,   135,   129,   129,   313,
     315,   135,   289,   319,   321,   323,   325,   320,   322,   324,
     326,   327,   328,   270,   147,   147,   128,    16,    16,     9,
      10,    11,    12,    13,    14,    15,    16,    57,   128,   130,
     137,   138,   293,   298,   304,   305,   254,   146,   232,   281,
     128,   134,   226,   225,   139,   222,   282,   161,   283,   283,
     283,   135,   162,   159,   162,   172,     9,    11,    12,    13,
      15,    16,    57,   128,   137,   138,   139,   174,   175,   176,
     180,   270,   278,   284,   131,   129,    60,    88,   234,   236,
     237,   139,   222,   192,   196,   197,   131,   273,   309,   289,
     289,   317,   129,   289,   289,   289,   289,   289,   289,   289,
     289,   289,   289,   135,   295,   213,   132,   301,   294,   300,
     299,    10,   128,   257,   264,   267,   129,   133,   231,   228,
     270,   232,   229,   281,   165,   166,     5,     6,     7,   131,
     163,   168,   165,   131,   179,   137,   138,   140,   141,   142,
     143,   144,   145,   181,   177,   208,   135,   273,   229,   191,
     240,   129,   273,   314,   129,   289,   129,   129,   129,   129,
     129,   129,   129,   129,   135,   135,   129,   232,   302,   298,
     293,     9,   305,   305,   258,   176,   269,   128,   232,   135,
     227,   128,   129,   130,     3,     4,     8,     9,    37,    38,
      41,    49,    50,    57,    61,    92,    93,   126,   127,   139,
     164,   169,   182,   183,   184,   185,   187,   189,   199,   203,
     205,   222,   243,   271,   306,   134,   130,   176,   178,   176,
     216,   235,    90,    91,   128,   251,   255,   261,   262,   310,
     135,   318,    11,    11,   129,   274,   129,   296,   232,   147,
     221,   129,   228,   331,   162,   127,   130,   134,   186,   270,
     270,   139,   271,   273,   185,   203,   205,   271,   139,   222,
     128,   128,   229,   245,   245,   185,   203,   205,   217,   217,
     217,   214,   162,   129,   176,   234,   238,   251,   259,   286,
     263,   129,   332,   129,   129,   129,   133,   135,   295,   129,
     232,   227,   129,   131,   331,   332,   186,   186,   229,   214,
     217,   217,   229,     8,   203,   271,   331,   127,   127,   217,
     217,   131,   240,   129,   251,   129,   128,   297,   131,   129,
     131,   127,   129,   129,   252,   303,   293,   332,   127,   257,
     305,   127,   129
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
#line 1260 "vtkParse.y"
    { startSig(); clearTypeId(); clearTemplate(); closeComment(); }
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 1274 "vtkParse.y"
    { output_function(); }
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 1275 "vtkParse.y"
    { output_function(); }
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 1276 "vtkParse.y"
    { reject_function(); }
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 1277 "vtkParse.y"
    { output_function(); }
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 1278 "vtkParse.y"
    { reject_function(); }
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 1279 "vtkParse.y"
    { output_function(); }
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 1280 "vtkParse.y"
    { legacySig(); output_function(); }
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 1297 "vtkParse.y"
    { pushNamespace((yyvsp[(2) - (2)].str)); }
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 1298 "vtkParse.y"
    { popNamespace(); }
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 1305 "vtkParse.y"
    { start_class((yyvsp[(2) - (2)].str), 0); }
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 1306 "vtkParse.y"
    { end_class(); }
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 1307 "vtkParse.y"
    { reject_class((yyvsp[(2) - (5)].str), 0); }
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 1308 "vtkParse.y"
    { end_class(); }
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 1309 "vtkParse.y"
    { start_class((yyvsp[(2) - (2)].str), 1); }
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 1310 "vtkParse.y"
    { end_class(); }
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 1311 "vtkParse.y"
    { reject_class((yyvsp[(2) - (5)].str), 1); }
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 1312 "vtkParse.y"
    { end_class(); }
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 1318 "vtkParse.y"
    { startSig(); clearTypeId(); clearTemplate();  closeComment(); }
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 1332 "vtkParse.y"
    { output_function(); }
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 1333 "vtkParse.y"
    { ClassInfo *tmpc = currentClass;
     currentClass = NULL; output_function(); currentClass = tmpc; }
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 1335 "vtkParse.y"
    { output_function(); }
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 1336 "vtkParse.y"
    { output_function(); }
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 1337 "vtkParse.y"
    { ClassInfo *tmpc = currentClass;
     currentClass = NULL; output_function(); currentClass = tmpc; }
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 1339 "vtkParse.y"
    { output_function(); }
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 1340 "vtkParse.y"
    { legacySig(); output_function(); }
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 1353 "vtkParse.y"
    {
      vtkParse_AddStringToArray(&currentClass->SuperClasses,
                                &currentClass->NumberOfSuperClasses,
                                vtkstrdup((yyvsp[(2) - (2)].str)));
    }
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 1359 "vtkParse.y"
    {access_level = VTK_ACCESS_PUBLIC;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 1360 "vtkParse.y"
    {access_level = VTK_ACCESS_PROTECTED;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 1361 "vtkParse.y"
    {access_level = VTK_ACCESS_PRIVATE;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 1371 "vtkParse.y"
    {start_enum((yyvsp[(2) - (2)].str));}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 1372 "vtkParse.y"
    {end_enum();}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 1373 "vtkParse.y"
    {start_enum(NULL);}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 1374 "vtkParse.y"
    {end_enum();}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 1378 "vtkParse.y"
    {add_enum((yyvsp[(1) - (1)].str), NULL);}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 1379 "vtkParse.y"
    {add_enum((yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 1381 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 1386 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 1387 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 1388 "vtkParse.y"
    {
         (yyval.str) = vtkstrcat((yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));
       }
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 1391 "vtkParse.y"
    {postSig((yyvsp[(2) - (2)].str));}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 1392 "vtkParse.y"
    {
         (yyval.str) = vtkstrcat5((yyvsp[(1) - (4)].str), " ", (yyvsp[(2) - (4)].str), " ", (yyvsp[(4) - (4)].str));
       }
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 1395 "vtkParse.y"
    {postSig("(");}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 1396 "vtkParse.y"
    {
         (yyval.str) = vtkstrcat3("(", (yyvsp[(3) - (4)].str), ")");
       }
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 1400 "vtkParse.y"
    { (yyval.str) = "-"; }
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 1400 "vtkParse.y"
    { (yyval.str) = "+"; }
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 1401 "vtkParse.y"
    { (yyval.str) = "~"; }
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 1403 "vtkParse.y"
    { (yyval.str) = "-"; }
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 1403 "vtkParse.y"
    { (yyval.str) = "+"; }
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 1404 "vtkParse.y"
    { (yyval.str) = "*"; }
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 1404 "vtkParse.y"
    { (yyval.str) = "/"; }
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 1405 "vtkParse.y"
    { (yyval.str) = "%"; }
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 1405 "vtkParse.y"
    { (yyval.str) = "&"; }
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 1406 "vtkParse.y"
    { (yyval.str) = "|"; }
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 1406 "vtkParse.y"
    { (yyval.str) = "^"; }
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 1432 "vtkParse.y"
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
        vtkParse_AddItemMacro(currentClass, Typedefs, item);
        }
      else
        {
        vtkParse_AddItemMacro(currentNamespace, Typedefs, item);
        }
    }
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 1454 "vtkParse.y"
    { }
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 1455 "vtkParse.y"
    { }
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 1456 "vtkParse.y"
    { }
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 1457 "vtkParse.y"
    { }
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 1459 "vtkParse.y"
    { }
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1465 "vtkParse.y"
    { postSig("template<> "); clearTypeId(); }
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1466 "vtkParse.y"
    { postSig("template<");
          clearTypeId(); startTemplate(); }
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1468 "vtkParse.y"
    { chopSig();
            if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
            postSig("> "); clearTypeId(); }
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1473 "vtkParse.y"
    { chopSig(); postSig(", "); clearTypeId(); }
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1477 "vtkParse.y"
    {
               TemplateArg *arg = (TemplateArg *)malloc(sizeof(TemplateArg));
               vtkParse_InitTemplateArg(arg);
               arg->Type = (yyvsp[(1) - (2)].integer);
               arg->Class = vtkstrdup(getTypeId());
               arg->Name = vtkstrdup(getVarName());
               arg->Value = vtkstrdup(getVarValue());
               vtkParse_AddItemMacro2(currentTemplate, Arguments, arg);
               }
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1487 "vtkParse.y"
    {
               TemplateArg *arg = (TemplateArg *)malloc(sizeof(TemplateArg));
               vtkParse_InitTemplateArg(arg);
               arg->Name = vtkstrdup(getVarName());
               arg->Value = vtkstrdup(getVarValue());
               vtkParse_AddItemMacro2(currentTemplate, Arguments, arg);
               }
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1494 "vtkParse.y"
    { pushTemplate(); }
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1495 "vtkParse.y"
    {
               TemplateArgs *newTemplate = currentTemplate;
               TemplateArg *arg = (TemplateArg *)malloc(sizeof(TemplateArg));
               vtkParse_InitTemplateArg(arg);
               popTemplate();
               arg->Template = newTemplate;
               arg->Name = vtkstrdup(getVarName());
               arg->Value = vtkstrdup(getVarValue());
               vtkParse_AddItemMacro2(currentTemplate, Arguments, arg);
               }
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1506 "vtkParse.y"
    {postSig("class ");}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1507 "vtkParse.y"
    {postSig("typename ");}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1509 "vtkParse.y"
    { setVarName((yyvsp[(1) - (1)].str)); }
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1539 "vtkParse.y"
    {openSig(); preSig("~"); closeSig();}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1540 "vtkParse.y"
    {openSig(); preSig("~"); closeSig();}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1542 "vtkParse.y"
    {
         openSig(); preSig("virtual ~"); closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 1550 "vtkParse.y"
    {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 1564 "vtkParse.y"
    {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 1573 "vtkParse.y"
    {
      postSig("(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1577 "vtkParse.y"
    { postSig(")"); }
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 1578 "vtkParse.y"
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

  case 170:

/* Line 1455 of yacc.c  */
#line 1589 "vtkParse.y"
    { postSig(")"); }
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1590 "vtkParse.y"
    {
      postSig(";");
      closeSig();
      currentFunction->Name = vtkstrdup((yyvsp[(1) - (3)].str));
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed operator", currentFunction->Name);
    }
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1598 "vtkParse.y"
    {postSig((yyvsp[(2) - (2)].str));}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1599 "vtkParse.y"
    {
      postSig("(");
      currentFunction->IsOperator = 1;
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 1604 "vtkParse.y"
    { (yyval.str) = (yyvsp[(2) - (7)].str); }
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 1606 "vtkParse.y"
    { postSig(")"); }
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 1607 "vtkParse.y"
    {
      postSig(";");
      closeSig();
      currentFunction->Name = vtkstrdup((yyvsp[(1) - (3)].str));
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1617 "vtkParse.y"
    {
      postSig(" = 0");
      if (currentClass)
        {
        currentFunction->IsPureVirtual = 1;
        currentClass->IsAbstract = 1;
        }
    }
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1626 "vtkParse.y"
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

  case 180:

/* Line 1455 of yacc.c  */
#line 1636 "vtkParse.y"
    {
      postSig(" const");
      currentFunction->IsConst = 1;
    }
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 1644 "vtkParse.y"
    {
      postSig("(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 1647 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (5)].str); }
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 1648 "vtkParse.y"
    {markSig(); postSig("<");}
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 1649 "vtkParse.y"
    {
      if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
      (yyval.str) = vtkstrcat((yyvsp[(1) - (6)].str), copySig());
    }
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 1654 "vtkParse.y"
    { (yyval.str) = (yyvsp[(7) - (9)].str); }
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 1656 "vtkParse.y"
    { postSig(");"); closeSig(); }
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 1657 "vtkParse.y"
    {
      currentFunction->Name = vtkstrdup((yyvsp[(1) - (3)].str));
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 1663 "vtkParse.y"
    { postSig("("); }
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 1672 "vtkParse.y"
    {
      postSig(");");
      closeSig();
      currentFunction->Name = vtkstrcat("~", (yyvsp[(1) - (1)].str));
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 1680 "vtkParse.y"
    { postSig("(");}
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 1686 "vtkParse.y"
    {clearTypeId();}
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 1689 "vtkParse.y"
    { currentFunction->IsVariadic = 1; postSig("..."); }
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 1690 "vtkParse.y"
    { clearTypeId(); }
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 1691 "vtkParse.y"
    { clearTypeId(); postSig(", "); }
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 1694 "vtkParse.y"
    { markSig(); }
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 1696 "vtkParse.y"
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

      vtkParse_AddItemMacro2(currentFunction, Arguments, arg);
    }
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 1718 "vtkParse.y"
    {
      int i = currentFunction->NumberOfArguments-1;
      if (getVarValue())
        {
        currentFunction->Arguments[i]->Value = vtkstrdup(getVarValue());
        }
    }
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 1726 "vtkParse.y"
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

      vtkParse_AddItemMacro2(currentFunction, Arguments, arg);
    }
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 1751 "vtkParse.y"
    {clearVarValue();}
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 1753 "vtkParse.y"
    { postSig("="); clearVarValue();}
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 1754 "vtkParse.y"
    { setVarValue((yyvsp[(3) - (3)].str)); }
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 1765 "vtkParse.y"
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

  case 224:

/* Line 1455 of yacc.c  */
#line 1779 "vtkParse.y"
    {postSig(", ");}
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 1782 "vtkParse.y"
    { setStorageTypeIndirection(0); }
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 1783 "vtkParse.y"
    { setStorageTypeIndirection((yyvsp[(1) - (1)].integer)); }
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 1787 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 1788 "vtkParse.y"
    { postSig(")"); }
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 1790 "vtkParse.y"
    {
         unsigned int parens = add_indirection((yyvsp[(1) - (5)].integer), (yyvsp[(2) - (5)].integer));
         if ((yyvsp[(5) - (5)].integer) == VTK_PARSE_FUNCTION) {
           (yyval.integer) = (parens | VTK_PARSE_FUNCTION); }
         else if ((yyvsp[(5) - (5)].integer) == VTK_PARSE_ARRAY) {
           (yyval.integer) = add_indirection_to_array(parens); }
       }
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 1799 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 1800 "vtkParse.y"
    { postSig(")"); }
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 1802 "vtkParse.y"
    {
         unsigned int parens = add_indirection((yyvsp[(1) - (5)].integer), (yyvsp[(2) - (5)].integer));
         if ((yyvsp[(5) - (5)].integer) == VTK_PARSE_FUNCTION) {
           (yyval.integer) = (parens | VTK_PARSE_FUNCTION); }
         else if ((yyvsp[(5) - (5)].integer) == VTK_PARSE_ARRAY) {
           (yyval.integer) = add_indirection_to_array(parens); }
       }
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 1810 "vtkParse.y"
    { postSig("("); (yyval.integer) = 0; }
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 1811 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("*");
               (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 1813 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("&");
               (yyval.integer) = VTK_PARSE_REF; }
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 1816 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("*");
               (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 1818 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("&");
               (yyval.integer) = VTK_PARSE_REF; }
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 1821 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 1822 "vtkParse.y"
    { pushFunction(); postSig("("); }
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 1823 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_FUNCTION; postSig(")"); popFunction(); }
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 1824 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_ARRAY; }
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 1827 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 1829 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(1) - (2)].integer), (yyvsp[(2) - (2)].integer));}
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 1832 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 1834 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(1) - (2)].integer), (yyvsp[(2) - (2)].integer));}
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 1836 "vtkParse.y"
    {clearVarName(); chopSig();}
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 1838 "vtkParse.y"
    {setVarName((yyvsp[(1) - (1)].str));}
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 1840 "vtkParse.y"
    {clearArray();}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 1842 "vtkParse.y"
    {clearArray();}
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 1844 "vtkParse.y"
    {postSig("[");}
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 1844 "vtkParse.y"
    {postSig("]");}
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 1848 "vtkParse.y"
    {pushArraySize("");}
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 1849 "vtkParse.y"
    {pushArraySize((yyvsp[(1) - (1)].str));}
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 1855 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 1856 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 1857 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 1858 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 1859 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 1860 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 1867 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer); setStorageType((yyval.integer));}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 1868 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 1869 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 1871 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(3) - (3)].integer); setStorageType((yyval.integer));}
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 1872 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 1873 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer));
      setStorageType((yyval.integer));}
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 1875 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(2) - (3)].integer) | (yyvsp[(3) - (3)].integer));
      setStorageType((yyval.integer));}
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 1879 "vtkParse.y"
    {postSig("static "); (yyval.integer) = VTK_PARSE_STATIC; }
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 1880 "vtkParse.y"
    {postSig("static "); (yyval.integer) = VTK_PARSE_STATIC; }
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 1882 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 1883 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer));}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 1885 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 1886 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_CONST | (yyvsp[(2) - (2)].integer));}
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 1887 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_CONST | (yyvsp[(1) - (2)].integer));}
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 1889 "vtkParse.y"
    {postSig("const ");}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 1893 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 1895 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 1896 "vtkParse.y"
    {postSig("typename ");}
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 1897 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(3) - (3)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 1900 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 1901 "vtkParse.y"
    {chopSig(); if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 1903 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 1904 "vtkParse.y"
    {chopSig(); if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 1907 "vtkParse.y"
    {postSig(", ");}
    break;

  case 295:

/* Line 1455 of yacc.c  */
#line 1909 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 296:

/* Line 1455 of yacc.c  */
#line 1910 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 1911 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 1912 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 1913 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 1914 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 303:

/* Line 1455 of yacc.c  */
#line 1919 "vtkParse.y"
    {
             (yyval.str) = vtkstrcat3((yyvsp[(1) - (3)].str), "::", (yyvsp[(3) - (3)].str));
             preScopeSig((yyvsp[(1) - (3)].str));
           }
    break;

  case 304:

/* Line 1455 of yacc.c  */
#line 1924 "vtkParse.y"
    {
             (yyval.str) = vtkstrcat3((yyvsp[(1) - (3)].str), "::", (yyvsp[(3) - (3)].str));
             preScopeSig("");
           }
    break;

  case 311:

/* Line 1455 of yacc.c  */
#line 1945 "vtkParse.y"
    { postSig("&"); (yyval.integer) = VTK_PARSE_REF;}
    break;

  case 312:

/* Line 1455 of yacc.c  */
#line 1946 "vtkParse.y"
    { postSig("&"); (yyval.integer) = ((yyvsp[(1) - (2)].integer) | VTK_PARSE_REF);}
    break;

  case 313:

/* Line 1455 of yacc.c  */
#line 1947 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 314:

/* Line 1455 of yacc.c  */
#line 1952 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 315:

/* Line 1455 of yacc.c  */
#line 1954 "vtkParse.y"
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

  case 316:

/* Line 1455 of yacc.c  */
#line 1965 "vtkParse.y"
    { postSig("*"); (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 317:

/* Line 1455 of yacc.c  */
#line 1966 "vtkParse.y"
    { postSig("*const "); (yyval.integer) = VTK_PARSE_CONST_POINTER; }
    break;

  case 318:

/* Line 1455 of yacc.c  */
#line 1969 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 1970 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); }
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 1971 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); }
    break;

  case 321:

/* Line 1455 of yacc.c  */
#line 1972 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 322:

/* Line 1455 of yacc.c  */
#line 1973 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 323:

/* Line 1455 of yacc.c  */
#line 1974 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 324:

/* Line 1455 of yacc.c  */
#line 1975 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 325:

/* Line 1455 of yacc.c  */
#line 1978 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 326:

/* Line 1455 of yacc.c  */
#line 1979 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 1982 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_STRING;}
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 1983 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNICODE_STRING;}
    break;

  case 329:

/* Line 1455 of yacc.c  */
#line 1984 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_OSTREAM; }
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 1985 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_ISTREAM; }
    break;

  case 331:

/* Line 1455 of yacc.c  */
#line 1986 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 332:

/* Line 1455 of yacc.c  */
#line 1987 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_OBJECT; }
    break;

  case 333:

/* Line 1455 of yacc.c  */
#line 1990 "vtkParse.y"
    { typeSig("void"); (yyval.integer) = VTK_PARSE_VOID;}
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 1991 "vtkParse.y"
    { typeSig("bool"); (yyval.integer) = VTK_PARSE_BOOL;}
    break;

  case 335:

/* Line 1455 of yacc.c  */
#line 1992 "vtkParse.y"
    { typeSig("ssize_t"); (yyval.integer) = VTK_PARSE_SSIZE_T;}
    break;

  case 336:

/* Line 1455 of yacc.c  */
#line 1993 "vtkParse.y"
    { typeSig("size_t"); (yyval.integer) = VTK_PARSE_SIZE_T;}
    break;

  case 337:

/* Line 1455 of yacc.c  */
#line 1994 "vtkParse.y"
    { typeSig("vtkTypeInt8"); (yyval.integer) = VTK_PARSE_INT8; }
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 1995 "vtkParse.y"
    { typeSig("vtkTypeUInt8"); (yyval.integer) = VTK_PARSE_UINT8; }
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 1996 "vtkParse.y"
    { typeSig("vtkTypeInt16"); (yyval.integer) = VTK_PARSE_INT16; }
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 1997 "vtkParse.y"
    { typeSig("vtkTypeUInt16"); (yyval.integer) = VTK_PARSE_UINT16; }
    break;

  case 341:

/* Line 1455 of yacc.c  */
#line 1998 "vtkParse.y"
    { typeSig("vtkTypeInt32"); (yyval.integer) = VTK_PARSE_INT32; }
    break;

  case 342:

/* Line 1455 of yacc.c  */
#line 1999 "vtkParse.y"
    { typeSig("vtkTypeUInt32"); (yyval.integer) = VTK_PARSE_UINT32; }
    break;

  case 343:

/* Line 1455 of yacc.c  */
#line 2000 "vtkParse.y"
    { typeSig("vtkTypeInt64"); (yyval.integer) = VTK_PARSE_INT64; }
    break;

  case 344:

/* Line 1455 of yacc.c  */
#line 2001 "vtkParse.y"
    { typeSig("vtkTypeUInt64"); (yyval.integer) = VTK_PARSE_UINT64; }
    break;

  case 345:

/* Line 1455 of yacc.c  */
#line 2002 "vtkParse.y"
    { typeSig("vtkTypeFloat32"); (yyval.integer) = VTK_PARSE_FLOAT32; }
    break;

  case 346:

/* Line 1455 of yacc.c  */
#line 2003 "vtkParse.y"
    { typeSig("vtkTypeFloat64"); (yyval.integer) = VTK_PARSE_FLOAT64; }
    break;

  case 347:

/* Line 1455 of yacc.c  */
#line 2004 "vtkParse.y"
    { typeSig("vtkIdType"); (yyval.integer) = VTK_PARSE_ID_TYPE;}
    break;

  case 348:

/* Line 1455 of yacc.c  */
#line 2005 "vtkParse.y"
    { typeSig("float"); (yyval.integer) = VTK_PARSE_FLOAT;}
    break;

  case 349:

/* Line 1455 of yacc.c  */
#line 2006 "vtkParse.y"
    { typeSig("double"); (yyval.integer) = VTK_PARSE_DOUBLE;}
    break;

  case 350:

/* Line 1455 of yacc.c  */
#line 2007 "vtkParse.y"
    { typeSig("long double"); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 351:

/* Line 1455 of yacc.c  */
#line 2008 "vtkParse.y"
    { typeSig("char"); (yyval.integer) = VTK_PARSE_CHAR;}
    break;

  case 352:

/* Line 1455 of yacc.c  */
#line 2009 "vtkParse.y"
    { typeSig("signed char"); (yyval.integer) = VTK_PARSE_SIGNED_CHAR;}
    break;

  case 353:

/* Line 1455 of yacc.c  */
#line 2011 "vtkParse.y"
    { typeSig("unsigned char"); (yyval.integer) = VTK_PARSE_UNSIGNED_CHAR;}
    break;

  case 354:

/* Line 1455 of yacc.c  */
#line 2012 "vtkParse.y"
    { typeSig("int"); (yyval.integer) = VTK_PARSE_INT;}
    break;

  case 355:

/* Line 1455 of yacc.c  */
#line 2014 "vtkParse.y"
    { typeSig("unsigned int"); (yyval.integer) = VTK_PARSE_UNSIGNED_INT;}
    break;

  case 356:

/* Line 1455 of yacc.c  */
#line 2015 "vtkParse.y"
    { typeSig("short"); (yyval.integer) = VTK_PARSE_SHORT;}
    break;

  case 357:

/* Line 1455 of yacc.c  */
#line 2017 "vtkParse.y"
    { typeSig("unsigned short"); (yyval.integer) = VTK_PARSE_UNSIGNED_SHORT;}
    break;

  case 358:

/* Line 1455 of yacc.c  */
#line 2018 "vtkParse.y"
    { typeSig("long"); (yyval.integer) = VTK_PARSE_LONG;}
    break;

  case 359:

/* Line 1455 of yacc.c  */
#line 2020 "vtkParse.y"
    { typeSig("unsigned long"); (yyval.integer) = VTK_PARSE_UNSIGNED_LONG;}
    break;

  case 360:

/* Line 1455 of yacc.c  */
#line 2021 "vtkParse.y"
    { typeSig("long long"); (yyval.integer) = VTK_PARSE_LONG_LONG;}
    break;

  case 361:

/* Line 1455 of yacc.c  */
#line 2023 "vtkParse.y"
    {typeSig("unsigned long long");(yyval.integer)=VTK_PARSE_UNSIGNED_LONG_LONG;}
    break;

  case 362:

/* Line 1455 of yacc.c  */
#line 2024 "vtkParse.y"
    { typeSig("__int64"); (yyval.integer) = VTK_PARSE___INT64;}
    break;

  case 363:

/* Line 1455 of yacc.c  */
#line 2026 "vtkParse.y"
    { typeSig("unsigned __int64"); (yyval.integer) = VTK_PARSE_UNSIGNED___INT64;}
    break;

  case 364:

/* Line 1455 of yacc.c  */
#line 2027 "vtkParse.y"
    { typeSig("int"); (yyval.integer) = VTK_PARSE_INT; }
    break;

  case 365:

/* Line 1455 of yacc.c  */
#line 2028 "vtkParse.y"
    { typeSig("unsigned int"); (yyval.integer) = VTK_PARSE_UNSIGNED_INT; }
    break;

  case 366:

/* Line 1455 of yacc.c  */
#line 2034 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 367:

/* Line 1455 of yacc.c  */
#line 2035 "vtkParse.y"
    { postSig("{ "); }
    break;

  case 368:

/* Line 1455 of yacc.c  */
#line 2036 "vtkParse.y"
    {
          postSig("}");
          (yyval.str) = vtkstrcat4("{ ", (yyvsp[(3) - (6)].str), (yyvsp[(4) - (6)].str), " }");
        }
    break;

  case 371:

/* Line 1455 of yacc.c  */
#line 2043 "vtkParse.y"
    {(yyval.str) = "";}
    break;

  case 372:

/* Line 1455 of yacc.c  */
#line 2044 "vtkParse.y"
    { postSig(", "); }
    break;

  case 373:

/* Line 1455 of yacc.c  */
#line 2045 "vtkParse.y"
    {
          (yyval.str) = vtkstrcat3((yyvsp[(1) - (4)].str), ", ", (yyvsp[(4) - (4)].str));
        }
    break;

  case 374:

/* Line 1455 of yacc.c  */
#line 2049 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 375:

/* Line 1455 of yacc.c  */
#line 2050 "vtkParse.y"
    {postSig("+");}
    break;

  case 376:

/* Line 1455 of yacc.c  */
#line 2050 "vtkParse.y"
    {(yyval.str) = (yyvsp[(3) - (3)].str);}
    break;

  case 377:

/* Line 1455 of yacc.c  */
#line 2051 "vtkParse.y"
    {postSig("-");}
    break;

  case 378:

/* Line 1455 of yacc.c  */
#line 2052 "vtkParse.y"
    {
             (yyval.str) = vtkstrcat("-", (yyvsp[(3) - (3)].str));
             }
    break;

  case 379:

/* Line 1455 of yacc.c  */
#line 2055 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 380:

/* Line 1455 of yacc.c  */
#line 2056 "vtkParse.y"
    {postSig("(");}
    break;

  case 381:

/* Line 1455 of yacc.c  */
#line 2056 "vtkParse.y"
    {postSig(")"); (yyval.str) = (yyvsp[(3) - (4)].str);}
    break;

  case 382:

/* Line 1455 of yacc.c  */
#line 2057 "vtkParse.y"
    {postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 383:

/* Line 1455 of yacc.c  */
#line 2059 "vtkParse.y"
    {
             chopSig();
             if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
             postSig(">(");
             }
    break;

  case 384:

/* Line 1455 of yacc.c  */
#line 2065 "vtkParse.y"
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

  case 385:

/* Line 1455 of yacc.c  */
#line 2079 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 386:

/* Line 1455 of yacc.c  */
#line 2081 "vtkParse.y"
    { (yyval.str) = vtkstrcat((yyvsp[(1) - (2)].str), (yyvsp[(2) - (2)].str)); }
    break;

  case 387:

/* Line 1455 of yacc.c  */
#line 2083 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 388:

/* Line 1455 of yacc.c  */
#line 2084 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 389:

/* Line 1455 of yacc.c  */
#line 2085 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 390:

/* Line 1455 of yacc.c  */
#line 2086 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 391:

/* Line 1455 of yacc.c  */
#line 2087 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 392:

/* Line 1455 of yacc.c  */
#line 2088 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 393:

/* Line 1455 of yacc.c  */
#line 2089 "vtkParse.y"
    {(yyval.str) = vtkstrdup(add_const_scope((yyvsp[(1) - (1)].str)));
                postSig((yyvsp[(1) - (1)].str));}
    break;

  case 394:

/* Line 1455 of yacc.c  */
#line 2091 "vtkParse.y"
    {(yyval.str) = vtkstrdup(add_const_scope((yyvsp[(1) - (1)].str)));
                postSig((yyvsp[(1) - (1)].str));}
    break;

  case 395:

/* Line 1455 of yacc.c  */
#line 2101 "vtkParse.y"
    {preSig("void Set"); postSig("(");}
    break;

  case 396:

/* Line 1455 of yacc.c  */
#line 2102 "vtkParse.y"
    {
   postSig("a);");
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (7)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, (yyvsp[(6) - (7)].integer), getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 397:

/* Line 1455 of yacc.c  */
#line 2110 "vtkParse.y"
    {postSig("Get");}
    break;

  case 398:

/* Line 1455 of yacc.c  */
#line 2111 "vtkParse.y"
    {markSig();}
    break;

  case 399:

/* Line 1455 of yacc.c  */
#line 2111 "vtkParse.y"
    {swapSig();}
    break;

  case 400:

/* Line 1455 of yacc.c  */
#line 2112 "vtkParse.y"
    {
   postSig("();");
   currentFunction->Name = vtkstrcat("Get", (yyvsp[(4) - (9)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, (yyvsp[(7) - (9)].integer), getTypeId(), 0);
   output_function();
   }
    break;

  case 401:

/* Line 1455 of yacc.c  */
#line 2119 "vtkParse.y"
    {preSig("void Set");}
    break;

  case 402:

/* Line 1455 of yacc.c  */
#line 2120 "vtkParse.y"
    {
   postSig("(char *);");
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(4) - (5)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 403:

/* Line 1455 of yacc.c  */
#line 2128 "vtkParse.y"
    {preSig("char *Get");}
    break;

  case 404:

/* Line 1455 of yacc.c  */
#line 2129 "vtkParse.y"
    {
   postSig("();");
   currentFunction->Name = vtkstrcat("Get", (yyvsp[(4) - (5)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   output_function();
   }
    break;

  case 405:

/* Line 1455 of yacc.c  */
#line 2136 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 406:

/* Line 1455 of yacc.c  */
#line 2136 "vtkParse.y"
    {closeSig();}
    break;

  case 407:

/* Line 1455 of yacc.c  */
#line 2138 "vtkParse.y"
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

  case 408:

/* Line 1455 of yacc.c  */
#line 2166 "vtkParse.y"
    {preSig("void Set"); postSig("("); }
    break;

  case 409:

/* Line 1455 of yacc.c  */
#line 2167 "vtkParse.y"
    {
   postSig("*);");
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (7)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 410:

/* Line 1455 of yacc.c  */
#line 2175 "vtkParse.y"
    {postSig("*Get");}
    break;

  case 411:

/* Line 1455 of yacc.c  */
#line 2176 "vtkParse.y"
    {markSig();}
    break;

  case 412:

/* Line 1455 of yacc.c  */
#line 2176 "vtkParse.y"
    {swapSig();}
    break;

  case 413:

/* Line 1455 of yacc.c  */
#line 2177 "vtkParse.y"
    {
   postSig("();");
   currentFunction->Name = vtkstrcat("Get", (yyvsp[(4) - (9)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   output_function();
   }
    break;

  case 414:

/* Line 1455 of yacc.c  */
#line 2185 "vtkParse.y"
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

  case 415:

/* Line 1455 of yacc.c  */
#line 2200 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 416:

/* Line 1455 of yacc.c  */
#line 2201 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 2);
   }
    break;

  case 417:

/* Line 1455 of yacc.c  */
#line 2205 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 418:

/* Line 1455 of yacc.c  */
#line 2206 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 2);
   }
    break;

  case 419:

/* Line 1455 of yacc.c  */
#line 2210 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 420:

/* Line 1455 of yacc.c  */
#line 2211 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 3);
   }
    break;

  case 421:

/* Line 1455 of yacc.c  */
#line 2215 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 422:

/* Line 1455 of yacc.c  */
#line 2216 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 3);
   }
    break;

  case 423:

/* Line 1455 of yacc.c  */
#line 2220 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 424:

/* Line 1455 of yacc.c  */
#line 2221 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 4);
   }
    break;

  case 425:

/* Line 1455 of yacc.c  */
#line 2225 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 426:

/* Line 1455 of yacc.c  */
#line 2226 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 4);
   }
    break;

  case 427:

/* Line 1455 of yacc.c  */
#line 2230 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 428:

/* Line 1455 of yacc.c  */
#line 2231 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 6);
   }
    break;

  case 429:

/* Line 1455 of yacc.c  */
#line 2235 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 430:

/* Line 1455 of yacc.c  */
#line 2236 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 6);
   }
    break;

  case 431:

/* Line 1455 of yacc.c  */
#line 2240 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 432:

/* Line 1455 of yacc.c  */
#line 2242 "vtkParse.y"
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

  case 433:

/* Line 1455 of yacc.c  */
#line 2256 "vtkParse.y"
    {startSig();}
    break;

  case 434:

/* Line 1455 of yacc.c  */
#line 2258 "vtkParse.y"
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

  case 435:

/* Line 1455 of yacc.c  */
#line 2270 "vtkParse.y"
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

  case 436:

/* Line 1455 of yacc.c  */
#line 2303 "vtkParse.y"
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

  case 437:

/* Line 1455 of yacc.c  */
#line 2337 "vtkParse.y"
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

  case 438:

/* Line 1455 of yacc.c  */
#line 2389 "vtkParse.y"
    { (yyval.str) = "operator()"; }
    break;

  case 439:

/* Line 1455 of yacc.c  */
#line 2390 "vtkParse.y"
    { (yyval.str) = "operator[]"; }
    break;

  case 440:

/* Line 1455 of yacc.c  */
#line 2391 "vtkParse.y"
    { (yyval.str) = "operator new[]"; }
    break;

  case 441:

/* Line 1455 of yacc.c  */
#line 2392 "vtkParse.y"
    { (yyval.str) = "operator delete[]"; }
    break;

  case 443:

/* Line 1455 of yacc.c  */
#line 2395 "vtkParse.y"
    { (yyval.str) = "operator="; }
    break;

  case 444:

/* Line 1455 of yacc.c  */
#line 2396 "vtkParse.y"
    { (yyval.str) = "operator*"; }
    break;

  case 445:

/* Line 1455 of yacc.c  */
#line 2396 "vtkParse.y"
    { (yyval.str) = "operator/"; }
    break;

  case 446:

/* Line 1455 of yacc.c  */
#line 2397 "vtkParse.y"
    { (yyval.str) = "operator-"; }
    break;

  case 447:

/* Line 1455 of yacc.c  */
#line 2397 "vtkParse.y"
    { (yyval.str) = "operator+"; }
    break;

  case 448:

/* Line 1455 of yacc.c  */
#line 2398 "vtkParse.y"
    { (yyval.str) = "operator!"; }
    break;

  case 449:

/* Line 1455 of yacc.c  */
#line 2398 "vtkParse.y"
    { (yyval.str) = "operator~"; }
    break;

  case 450:

/* Line 1455 of yacc.c  */
#line 2399 "vtkParse.y"
    { (yyval.str) = "operator,"; }
    break;

  case 451:

/* Line 1455 of yacc.c  */
#line 2399 "vtkParse.y"
    { (yyval.str) = "operator<"; }
    break;

  case 452:

/* Line 1455 of yacc.c  */
#line 2400 "vtkParse.y"
    { (yyval.str) = "operator>"; }
    break;

  case 453:

/* Line 1455 of yacc.c  */
#line 2400 "vtkParse.y"
    { (yyval.str) = "operator&"; }
    break;

  case 454:

/* Line 1455 of yacc.c  */
#line 2401 "vtkParse.y"
    { (yyval.str) = "operator|"; }
    break;

  case 455:

/* Line 1455 of yacc.c  */
#line 2401 "vtkParse.y"
    { (yyval.str) = "operator^"; }
    break;

  case 456:

/* Line 1455 of yacc.c  */
#line 2402 "vtkParse.y"
    { (yyval.str) = "operator%"; }
    break;

  case 457:

/* Line 1455 of yacc.c  */
#line 2403 "vtkParse.y"
    { (yyval.str) = "operator new"; }
    break;

  case 458:

/* Line 1455 of yacc.c  */
#line 2404 "vtkParse.y"
    { (yyval.str) = "operator delete"; }
    break;

  case 459:

/* Line 1455 of yacc.c  */
#line 2405 "vtkParse.y"
    { (yyval.str) = "operator<<="; }
    break;

  case 460:

/* Line 1455 of yacc.c  */
#line 2406 "vtkParse.y"
    { (yyval.str) = "operator>>="; }
    break;

  case 461:

/* Line 1455 of yacc.c  */
#line 2407 "vtkParse.y"
    { (yyval.str) = "operator<<"; }
    break;

  case 462:

/* Line 1455 of yacc.c  */
#line 2408 "vtkParse.y"
    { (yyval.str) = "operator>>"; }
    break;

  case 463:

/* Line 1455 of yacc.c  */
#line 2409 "vtkParse.y"
    { (yyval.str) = "operator->*"; }
    break;

  case 464:

/* Line 1455 of yacc.c  */
#line 2410 "vtkParse.y"
    { (yyval.str) = "operator->"; }
    break;

  case 465:

/* Line 1455 of yacc.c  */
#line 2411 "vtkParse.y"
    { (yyval.str) = "operator+="; }
    break;

  case 466:

/* Line 1455 of yacc.c  */
#line 2412 "vtkParse.y"
    { (yyval.str) = "operator-="; }
    break;

  case 467:

/* Line 1455 of yacc.c  */
#line 2413 "vtkParse.y"
    { (yyval.str) = "operator*="; }
    break;

  case 468:

/* Line 1455 of yacc.c  */
#line 2414 "vtkParse.y"
    { (yyval.str) = "operator/="; }
    break;

  case 469:

/* Line 1455 of yacc.c  */
#line 2415 "vtkParse.y"
    { (yyval.str) = "operator%="; }
    break;

  case 470:

/* Line 1455 of yacc.c  */
#line 2416 "vtkParse.y"
    { (yyval.str) = "operator++"; }
    break;

  case 471:

/* Line 1455 of yacc.c  */
#line 2417 "vtkParse.y"
    { (yyval.str) = "operator--"; }
    break;

  case 472:

/* Line 1455 of yacc.c  */
#line 2418 "vtkParse.y"
    { (yyval.str) = "operator&="; }
    break;

  case 473:

/* Line 1455 of yacc.c  */
#line 2419 "vtkParse.y"
    { (yyval.str) = "operator|="; }
    break;

  case 474:

/* Line 1455 of yacc.c  */
#line 2420 "vtkParse.y"
    { (yyval.str) = "operator^="; }
    break;

  case 475:

/* Line 1455 of yacc.c  */
#line 2421 "vtkParse.y"
    {(yyval.str) = "operator&&=";}
    break;

  case 476:

/* Line 1455 of yacc.c  */
#line 2422 "vtkParse.y"
    {(yyval.str) = "operator||=";}
    break;

  case 477:

/* Line 1455 of yacc.c  */
#line 2423 "vtkParse.y"
    { (yyval.str) = "operator&&"; }
    break;

  case 478:

/* Line 1455 of yacc.c  */
#line 2424 "vtkParse.y"
    { (yyval.str) = "operator||"; }
    break;

  case 479:

/* Line 1455 of yacc.c  */
#line 2425 "vtkParse.y"
    { (yyval.str) = "operator=="; }
    break;

  case 480:

/* Line 1455 of yacc.c  */
#line 2426 "vtkParse.y"
    { (yyval.str) = "operator!="; }
    break;

  case 481:

/* Line 1455 of yacc.c  */
#line 2427 "vtkParse.y"
    { (yyval.str) = "operator<="; }
    break;

  case 482:

/* Line 1455 of yacc.c  */
#line 2428 "vtkParse.y"
    { (yyval.str) = "operator>="; }
    break;



/* Line 1455 of yacc.c  */
#line 7471 "vtkParse.tab.c"
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
#line 2452 "vtkParse.y"

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
  name_info->NumberOfEnums = 0;
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
      free((char **)class_info->SuperClasses);
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

  vtkParse_AddItemMacro2(func, Arguments, arg);
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
  int i, j;
  int match;

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

/* Utility method to add a pointer to an array */
void vtkParse_AddPointerToArray(
  void *valueArray, int *count, const void *value)
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

  values[n++] = (void *)value;
  *count = n;
  *(void ***)valueArray = values;
}

/* Utility method to add a const char pointer to an array */
void vtkParse_AddStringToArray(
  const char ***valueArray, int *count, const char *value)
{
  const char **values = *valueArray;
  int n = *count;

  /* if empty, alloc for the first time */
  if (n == 0)
    {
    values = (const char **)malloc(1*sizeof(void*));
    }
  /* if count is power of two, reallocate with double size */
  else if ((n & (n-1)) == 0)
    {
    values = (const char **)realloc((char **)values, (n << 1)*sizeof(char*));
    }

  values[n++] = value;
  *count = n;
  *valueArray = values;
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
  FreeNamespace(file_info->Contents);
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
