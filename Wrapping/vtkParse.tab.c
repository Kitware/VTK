
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



/* Line 189 of yacc.c  */
#line 1235 "vtkParse.tab.c"

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
     FloatType = 356,
     TypeInt8 = 357,
     TypeUInt8 = 358,
     TypeInt16 = 359,
     TypeUInt16 = 360,
     TypeInt32 = 361,
     TypeUInt32 = 362,
     TypeInt64 = 363,
     TypeUInt64 = 364,
     TypeFloat32 = 365,
     TypeFloat64 = 366,
     SetMacro = 367,
     GetMacro = 368,
     SetStringMacro = 369,
     GetStringMacro = 370,
     SetClampMacro = 371,
     SetObjectMacro = 372,
     GetObjectMacro = 373,
     BooleanMacro = 374,
     SetVector2Macro = 375,
     SetVector3Macro = 376,
     SetVector4Macro = 377,
     SetVector6Macro = 378,
     GetVector2Macro = 379,
     GetVector3Macro = 380,
     GetVector4Macro = 381,
     GetVector6Macro = 382,
     SetVectorMacro = 383,
     GetVectorMacro = 384,
     ViewportCoordinateMacro = 385,
     WorldCoordinateMacro = 386,
     TypeMacro = 387,
     VTK_BYTE_SWAP_DECL = 388
   };
#endif




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 1182 "vtkParse.y"

  const char   *str;
  unsigned int  integer;



/* Line 214 of yacc.c  */
#line 1544 "vtkParse.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 1556 "vtkParse.tab.c"

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
#define YYLAST   7055

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  157
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  196
/* YYNRULES -- Number of rules.  */
#define YYNRULES  581
/* YYNRULES -- Number of states.  */
#define YYNSTATES  1021

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   388

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   155,     2,     2,     2,   149,   150,     2,
     135,   136,   147,   145,   142,   144,   156,   148,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   141,   134,
     139,   143,   140,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   153,     2,   154,   152,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   137,   151,   138,   146,     2,     2,     2,
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
     125,   126,   127,   128,   129,   130,   131,   132,   133
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     5,     9,    11,    15,    17,    19,
      21,    23,    27,    32,    34,    37,    41,    44,    47,    50,
      54,    57,    59,    62,    67,    72,    77,    79,    85,    86,
      93,    98,    99,   107,   108,   119,   120,   128,   129,   140,
     145,   146,   154,   155,   166,   171,   172,   173,   177,   181,
     183,   187,   189,   191,   195,   200,   203,   206,   208,   211,
     215,   219,   222,   226,   230,   233,   239,   241,   243,   244,
     247,   249,   253,   255,   258,   261,   264,   266,   268,   270,
     271,   278,   279,   285,   286,   288,   292,   294,   298,   300,
     302,   304,   306,   308,   310,   312,   314,   316,   318,   319,
     323,   324,   329,   330,   335,   337,   339,   341,   343,   345,
     347,   349,   351,   353,   355,   357,   359,   361,   364,   368,
     372,   375,   379,   382,   384,   390,   394,   399,   404,   409,
     413,   415,   420,   425,   429,   433,   434,   440,   442,   443,
     448,   451,   454,   455,   459,   461,   463,   464,   465,   469,
     474,   479,   482,   486,   491,   497,   501,   506,   513,   521,
     527,   534,   537,   541,   544,   548,   552,   554,   557,   560,
     563,   567,   569,   572,   575,   579,   583,   585,   588,   592,
     593,   594,   603,   604,   608,   609,   610,   618,   619,   623,
     624,   627,   630,   632,   634,   638,   639,   645,   646,   647,
     657,   658,   662,   663,   669,   670,   674,   675,   679,   684,
     686,   687,   693,   694,   695,   698,   700,   702,   703,   708,
     709,   710,   716,   718,   720,   723,   724,   726,   727,   731,
     736,   741,   745,   748,   749,   752,   753,   754,   759,   760,
     763,   764,   768,   771,   772,   778,   781,   782,   788,   790,
     792,   794,   796,   798,   799,   801,   802,   803,   809,   811,
     813,   816,   818,   821,   822,   824,   826,   827,   829,   830,
     833,   834,   840,   841,   843,   844,   846,   848,   850,   852,
     854,   856,   858,   860,   862,   864,   866,   868,   870,   872,
     874,   876,   878,   880,   882,   884,   886,   888,   890,   893,
     896,   900,   903,   906,   910,   912,   915,   917,   920,   922,
     925,   928,   930,   932,   934,   936,   937,   941,   942,   948,
     949,   955,   956,   962,   964,   965,   970,   972,   974,   976,
     978,   980,   982,   984,   986,   988,   990,   992,   996,  1000,
    1002,  1004,  1006,  1008,  1010,  1012,  1014,  1016,  1019,  1021,
    1023,  1026,  1028,  1030,  1032,  1035,  1038,  1041,  1044,  1046,
    1048,  1050,  1052,  1054,  1056,  1058,  1060,  1062,  1064,  1066,
    1068,  1070,  1072,  1074,  1076,  1078,  1080,  1082,  1084,  1086,
    1088,  1090,  1092,  1094,  1096,  1098,  1100,  1102,  1104,  1106,
    1108,  1110,  1112,  1114,  1116,  1118,  1120,  1122,  1124,  1126,
    1128,  1130,  1132,  1133,  1140,  1141,  1143,  1144,  1145,  1150,
    1152,  1153,  1157,  1158,  1162,  1164,  1165,  1170,  1171,  1172,
    1182,  1184,  1186,  1188,  1190,  1192,  1195,  1197,  1199,  1201,
    1203,  1205,  1207,  1209,  1210,  1218,  1219,  1220,  1221,  1231,
    1232,  1238,  1239,  1245,  1246,  1247,  1258,  1259,  1267,  1268,
    1269,  1270,  1280,  1287,  1288,  1296,  1297,  1305,  1306,  1314,
    1315,  1323,  1324,  1332,  1333,  1341,  1342,  1350,  1351,  1359,
    1360,  1370,  1371,  1381,  1386,  1391,  1399,  1402,  1405,  1409,
    1413,  1415,  1417,  1419,  1421,  1423,  1425,  1427,  1429,  1431,
    1433,  1435,  1437,  1439,  1441,  1443,  1445,  1447,  1449,  1451,
    1453,  1455,  1457,  1459,  1461,  1463,  1465,  1467,  1469,  1471,
    1473,  1475,  1477,  1479,  1481,  1483,  1485,  1487,  1489,  1491,
    1493,  1495,  1496,  1499,  1500,  1503,  1505,  1507,  1509,  1511,
    1513,  1515,  1517,  1519,  1521,  1523,  1525,  1527,  1529,  1531,
    1533,  1535,  1537,  1539,  1541,  1543,  1545,  1547,  1549,  1551,
    1553,  1555,  1557,  1559,  1561,  1563,  1565,  1567,  1569,  1571,
    1573,  1575,  1577,  1579,  1581,  1583,  1585,  1587,  1589,  1591,
    1593,  1595,  1597,  1599,  1601,  1603,  1605,  1607,  1611,  1615,
    1619,  1623
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     158,     0,    -1,    -1,    -1,   158,   159,   160,    -1,   252,
      -1,   178,   254,   134,    -1,   196,    -1,   162,    -1,   161,
      -1,   194,    -1,   164,   254,   134,    -1,   197,   164,   254,
     134,    -1,    41,    -1,   214,   226,    -1,   197,   214,   226,
      -1,   213,   226,    -1,   208,   226,    -1,   209,   226,    -1,
     197,   208,   226,    -1,   206,   226,    -1,   321,    -1,   299,
     134,    -1,     9,   135,   346,   136,    -1,    57,   135,   346,
     136,    -1,    97,   135,   346,   136,    -1,   134,    -1,    59,
      10,   137,   158,   138,    -1,    -1,    55,   299,   163,   137,
     158,   138,    -1,    55,   137,   346,   138,    -1,    -1,     4,
     280,   165,   174,   137,   171,   138,    -1,    -1,     4,   280,
     139,   294,   140,   166,   174,   137,   171,   138,    -1,    -1,
       3,   280,   167,   174,   137,   171,   138,    -1,    -1,     3,
     280,   139,   294,   140,   168,   174,   137,   171,   138,    -1,
       3,   137,   346,   138,    -1,    -1,    40,   280,   169,   174,
     137,   171,   138,    -1,    -1,    40,   280,   139,   294,   140,
     170,   174,   137,   171,   138,    -1,    40,   137,   346,   138,
      -1,    -1,    -1,   171,   172,   173,    -1,   171,   177,   141,
      -1,   252,    -1,   178,   254,   134,    -1,   196,    -1,   194,
      -1,   164,   254,   134,    -1,   197,   164,   254,   134,    -1,
      49,   192,    -1,    49,   191,    -1,    41,    -1,   214,   226,
      -1,    49,   214,   226,    -1,   197,   214,   226,    -1,   211,
     226,    -1,    49,   211,   226,    -1,   197,   211,   226,    -1,
     207,   226,    -1,   133,   135,   346,   136,   134,    -1,   321,
      -1,   134,    -1,    -1,   141,   175,    -1,   176,    -1,   176,
     142,   175,    -1,   297,    -1,     6,   297,    -1,     7,   297,
      -1,     5,   297,    -1,     5,    -1,     6,    -1,     7,    -1,
      -1,    39,   280,   179,   137,   181,   138,    -1,    -1,    39,
     180,   137,   181,   138,    -1,    -1,   182,    -1,   182,   142,
     181,    -1,   280,    -1,   280,   143,   185,    -1,   184,    -1,
     280,    -1,   298,    -1,   290,    -1,    16,    -1,    11,    -1,
      13,    -1,    12,    -1,    15,    -1,   183,    -1,    -1,   189,
     186,   185,    -1,    -1,   183,   190,   187,   185,    -1,    -1,
     135,   188,   185,   136,    -1,   144,    -1,   145,    -1,   146,
      -1,   144,    -1,   145,    -1,   147,    -1,   148,    -1,   149,
      -1,   150,    -1,   151,    -1,   152,    -1,    71,    -1,    72,
      -1,   197,   192,    -1,     4,   280,   193,    -1,     3,   280,
     193,    -1,     3,   193,    -1,    40,   280,   193,    -1,    40,
     193,    -1,   134,    -1,   137,   346,   138,   347,   134,    -1,
     141,   347,   134,    -1,   195,   285,   262,   134,    -1,   195,
     164,   248,   134,    -1,   195,   178,   248,   134,    -1,   195,
      60,   134,    -1,    54,    -1,    56,    55,   297,   134,    -1,
      56,    53,   297,   134,    -1,    56,   297,   134,    -1,    52,
     139,   140,    -1,    -1,    52,   139,   198,   199,   140,    -1,
     201,    -1,    -1,   201,   142,   200,   199,    -1,   304,   204,
      -1,   203,   204,    -1,    -1,   202,   197,   204,    -1,     4,
      -1,    53,    -1,    -1,    -1,   280,   205,   249,    -1,    61,
     135,   208,   136,    -1,    61,   135,   211,   136,    -1,   283,
     223,    -1,   283,   210,   223,    -1,   299,    94,   146,   238,
      -1,    50,   299,    94,   146,   238,    -1,   299,    94,   231,
      -1,    50,   299,    94,   231,    -1,   299,    94,   299,    94,
     146,   238,    -1,    50,   299,    94,   299,    94,   146,   238,
      -1,   299,    94,   299,    94,   231,    -1,    50,   299,    94,
     299,    94,   231,    -1,   299,    94,    -1,   210,   299,    94,
      -1,   146,   238,    -1,    50,   146,   238,    -1,     8,   146,
     238,    -1,   231,    -1,    50,   231,    -1,   212,   231,    -1,
     283,   223,    -1,     8,   283,   223,    -1,    64,    -1,    50,
      64,    -1,    64,    50,    -1,   299,    94,   215,    -1,   283,
     210,   218,    -1,   215,    -1,   283,   218,    -1,     8,   285,
     218,    -1,    -1,    -1,    46,   283,   135,   216,   241,   136,
     217,   225,    -1,    -1,   220,   219,   225,    -1,    -1,    -1,
      46,   344,   221,   135,   222,   241,   136,    -1,    -1,   227,
     224,   225,    -1,    -1,   143,    16,    -1,    45,    16,    -1,
      43,    -1,   134,    -1,   137,   346,   138,    -1,    -1,   280,
     135,   228,   241,   136,    -1,    -1,    -1,   280,   139,   229,
     294,   140,   135,   230,   241,   136,    -1,    -1,   233,   232,
     235,    -1,    -1,   280,   135,   234,   241,   136,    -1,    -1,
     141,   237,   236,    -1,    -1,   142,   237,   236,    -1,   297,
     135,   346,   136,    -1,   239,    -1,    -1,   280,   135,   240,
     241,   136,    -1,    -1,    -1,   242,   243,    -1,    93,    -1,
     245,    -1,    -1,   245,   142,   244,   243,    -1,    -1,    -1,
     246,   285,   260,   247,   249,    -1,    60,    -1,   280,    -1,
     300,   280,    -1,    -1,   250,    -1,    -1,   143,   251,   307,
      -1,   283,   253,   255,   134,    -1,    58,    60,   255,   134,
      -1,    60,   255,   134,    -1,   262,   249,    -1,    -1,   257,
     255,    -1,    -1,    -1,   255,   142,   256,   257,    -1,    -1,
     258,   253,    -1,    -1,   300,   259,   253,    -1,   271,   273,
      -1,    -1,   264,   269,   136,   261,   267,    -1,   272,   273,
      -1,    -1,   265,   270,   136,   263,   267,    -1,   135,    -1,
      95,    -1,    96,    -1,    95,    -1,    96,    -1,    -1,    43,
      -1,    -1,    -1,   135,   268,   241,   136,   266,    -1,   274,
      -1,   260,    -1,   300,   260,    -1,   262,    -1,   300,   262,
      -1,    -1,   272,    -1,   280,    -1,    -1,   274,    -1,    -1,
     275,   276,    -1,    -1,   278,   153,   277,   279,   154,    -1,
      -1,   276,    -1,    -1,   185,    -1,    57,    -1,    97,    -1,
       9,    -1,    38,    -1,    37,    -1,    98,    -1,    99,    -1,
     281,    -1,   282,    -1,   102,    -1,   103,    -1,   104,    -1,
     105,    -1,   106,    -1,   107,    -1,   108,    -1,   109,    -1,
     110,    -1,   111,    -1,   100,    -1,   101,    -1,   285,    -1,
      51,   285,    -1,    59,   285,    -1,    59,    10,   285,    -1,
      50,   285,    -1,   284,   285,    -1,    50,   284,   285,    -1,
      58,    -1,    58,    50,    -1,   286,    -1,   286,   300,    -1,
     288,    -1,   287,   288,    -1,   288,   287,    -1,    43,    -1,
     303,    -1,   290,    -1,   298,    -1,    -1,    53,   289,   297,
      -1,    -1,    57,   139,   291,   294,   140,    -1,    -1,     9,
     139,   292,   294,   140,    -1,    -1,    97,   139,   293,   294,
     140,    -1,   296,    -1,    -1,   296,   142,   295,   294,    -1,
     285,    -1,   184,    -1,    57,    -1,     9,    -1,    97,    -1,
      38,    -1,    37,    -1,    98,    -1,    99,    -1,   290,    -1,
     298,    -1,   299,    94,   297,    -1,   290,    94,   297,    -1,
       9,    -1,    97,    -1,    57,    -1,    38,    -1,    37,    -1,
      98,    -1,    99,    -1,   150,    -1,   301,   150,    -1,   301,
      -1,   302,    -1,   301,   302,    -1,   147,    -1,    44,    -1,
     304,    -1,     4,   305,    -1,     3,   305,    -1,    40,   305,
      -1,    39,   305,    -1,   306,    -1,   305,    -1,    98,    -1,
      99,    -1,    37,    -1,    38,    -1,     9,    -1,    57,    -1,
      97,    -1,    33,    -1,    34,    -1,    35,    -1,    36,    -1,
     102,    -1,   103,    -1,   104,    -1,   105,    -1,   106,    -1,
     107,    -1,   108,    -1,   109,    -1,   110,    -1,   111,    -1,
     100,    -1,   101,    -1,    17,    -1,    18,    -1,    19,    -1,
      30,    -1,    31,    -1,    32,    -1,    20,    -1,    21,    -1,
      22,    -1,    23,    -1,    24,    -1,    25,    -1,    26,    -1,
      27,    -1,    28,    -1,    29,    -1,    48,    -1,    47,    -1,
     312,    -1,    -1,   137,   308,   307,   310,   309,   138,    -1,
      -1,   142,    -1,    -1,    -1,   310,   142,   311,   307,    -1,
     320,    -1,    -1,   145,   313,   320,    -1,    -1,   144,   314,
     320,    -1,   319,    -1,    -1,   135,   315,   312,   136,    -1,
      -1,    -1,   318,   139,   316,   286,   140,   135,   317,   312,
     136,    -1,    65,    -1,    67,    -1,    66,    -1,    68,    -1,
      10,    -1,   319,    10,    -1,    16,    -1,    11,    -1,    13,
      -1,    12,    -1,    14,    -1,    15,    -1,   297,    -1,    -1,
     112,   135,   280,   142,   322,   285,   136,    -1,    -1,    -1,
      -1,   113,   135,   323,   280,   142,   324,   285,   325,   136,
      -1,    -1,   114,   135,   326,   280,   136,    -1,    -1,   115,
     135,   327,   280,   136,    -1,    -1,    -1,   116,   135,   280,
     142,   328,   303,   329,   142,   347,   136,    -1,    -1,   117,
     135,   280,   142,   330,   303,   136,    -1,    -1,    -1,    -1,
     118,   135,   331,   280,   142,   332,   303,   333,   136,    -1,
     119,   135,   280,   142,   303,   136,    -1,    -1,   120,   135,
     280,   142,   334,   303,   136,    -1,    -1,   124,   135,   280,
     142,   335,   303,   136,    -1,    -1,   121,   135,   280,   142,
     336,   303,   136,    -1,    -1,   125,   135,   280,   142,   337,
     303,   136,    -1,    -1,   122,   135,   280,   142,   338,   303,
     136,    -1,    -1,   126,   135,   280,   142,   339,   303,   136,
      -1,    -1,   123,   135,   280,   142,   340,   303,   136,    -1,
      -1,   127,   135,   280,   142,   341,   303,   136,    -1,    -1,
     128,   135,   280,   142,   342,   303,   142,    11,   136,    -1,
      -1,   129,   135,   280,   142,   343,   303,   142,    11,   136,
      -1,   130,   135,   280,   136,    -1,   131,   135,   280,   136,
      -1,   132,   135,   280,   142,   280,   309,   136,    -1,   135,
     136,    -1,   153,   154,    -1,    62,   153,   154,    -1,    63,
     153,   154,    -1,   345,    -1,   143,    -1,   147,    -1,   148,
      -1,   144,    -1,   145,    -1,   155,    -1,   146,    -1,   142,
      -1,   139,    -1,   140,    -1,   150,    -1,   151,    -1,   152,
      -1,   149,    -1,    62,    -1,    63,    -1,    69,    -1,    70,
      -1,    71,    -1,    72,    -1,    73,    -1,    74,    -1,    77,
      -1,    78,    -1,    79,    -1,    80,    -1,    81,    -1,    75,
      -1,    76,    -1,    82,    -1,    83,    -1,    84,    -1,    85,
      -1,    86,    -1,    87,    -1,    88,    -1,    89,    -1,    90,
      -1,    91,    -1,    92,    -1,    -1,   346,   348,    -1,    -1,
     347,   349,    -1,   134,    -1,   349,    -1,    42,    -1,   350,
      -1,   352,    -1,   351,    -1,    54,    -1,   345,    -1,   141,
      -1,   156,    -1,    94,    -1,     4,    -1,    52,    -1,    38,
      -1,    37,    -1,    98,    -1,    99,    -1,   306,    -1,    13,
      -1,    11,    -1,    12,    -1,    14,    -1,    15,    -1,    10,
      -1,    41,    -1,    43,    -1,    44,    -1,    45,    -1,     3,
      -1,    46,    -1,    58,    -1,    50,    -1,     8,    -1,    39,
      -1,    40,    -1,    53,    -1,    16,    -1,    60,    -1,    93,
      -1,     5,    -1,     7,    -1,     6,    -1,    55,    -1,    56,
      -1,    59,    -1,     9,    -1,    57,    -1,    97,    -1,    67,
      -1,    66,    -1,    65,    -1,    68,    -1,   137,   346,   138,
      -1,   153,   346,   154,    -1,   135,   346,   136,    -1,    95,
     346,   136,    -1,    96,   346,   136,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,  1332,  1332,  1333,  1332,  1337,  1338,  1339,  1340,  1341,
    1342,  1343,  1344,  1345,  1346,  1347,  1348,  1349,  1350,  1351,
    1352,  1353,  1354,  1355,  1356,  1357,  1358,  1364,  1370,  1370,
    1372,  1378,  1378,  1380,  1380,  1382,  1382,  1384,  1384,  1386,
    1387,  1387,  1389,  1389,  1391,  1393,  1395,  1394,  1397,  1400,
    1401,  1402,  1403,  1404,  1405,  1406,  1407,  1408,  1409,  1410,
    1412,  1413,  1414,  1416,  1417,  1418,  1419,  1420,  1422,  1422,
    1424,  1424,  1426,  1427,  1428,  1429,  1436,  1437,  1438,  1448,
    1448,  1450,  1450,  1453,  1453,  1453,  1455,  1456,  1458,  1459,
    1460,  1460,  1462,  1462,  1462,  1462,  1462,  1464,  1465,  1465,
    1469,  1469,  1473,  1473,  1478,  1478,  1479,  1481,  1481,  1482,
    1482,  1483,  1483,  1484,  1484,  1485,  1486,  1492,  1494,  1495,
    1496,  1497,  1498,  1500,  1501,  1502,  1508,  1531,  1532,  1533,
    1535,  1542,  1543,  1544,  1551,  1552,  1552,  1558,  1559,  1559,
    1562,  1572,  1580,  1580,  1592,  1593,  1595,  1595,  1595,  1602,
    1604,  1610,  1612,  1613,  1614,  1615,  1616,  1617,  1618,  1619,
    1620,  1622,  1623,  1625,  1626,  1627,  1632,  1633,  1634,  1641,
    1642,  1650,  1650,  1650,  1652,  1653,  1656,  1657,  1658,  1668,
    1672,  1667,  1684,  1684,  1693,  1694,  1693,  1701,  1701,  1710,
    1711,  1720,  1730,  1736,  1736,  1739,  1738,  1743,  1744,  1743,
    1751,  1751,  1758,  1758,  1760,  1760,  1762,  1762,  1764,  1766,
    1775,  1775,  1781,  1781,  1781,  1784,  1785,  1786,  1786,  1789,
    1791,  1789,  1820,  1844,  1844,  1846,  1846,  1848,  1848,  1855,
    1856,  1857,  1859,  1910,  1911,  1913,  1914,  1914,  1917,  1917,
    1918,  1918,  1922,  1923,  1923,  1937,  1938,  1938,  1951,  1952,
    1954,  1957,  1959,  1962,  1962,  1964,  1965,  1965,  1967,  1970,
    1971,  1975,  1976,  1979,  1979,  1981,  1983,  1983,  1985,  1985,
    1987,  1987,  1989,  1989,  1991,  1992,  1998,  1999,  2000,  2001,
    2002,  2003,  2004,  2005,  2006,  2009,  2010,  2011,  2012,  2013,
    2014,  2015,  2016,  2017,  2018,  2021,  2022,  2029,  2030,  2031,
    2032,  2034,  2035,  2037,  2041,  2042,  2044,  2045,  2047,  2048,
    2049,  2051,  2053,  2054,  2056,  2058,  2058,  2062,  2062,  2065,
    2065,  2068,  2068,  2072,  2073,  2073,  2076,  2076,  2078,  2079,
    2080,  2081,  2082,  2083,  2084,  2085,  2086,  2088,  2093,  2099,
    2099,  2099,  2099,  2099,  2100,  2100,  2116,  2117,  2118,  2123,
    2124,  2136,  2137,  2140,  2141,  2142,  2143,  2144,  2147,  2148,
    2151,  2152,  2153,  2154,  2155,  2156,  2157,  2160,  2161,  2162,
    2163,  2164,  2165,  2166,  2167,  2168,  2169,  2170,  2171,  2172,
    2173,  2174,  2175,  2176,  2177,  2178,  2179,  2180,  2181,  2183,
    2184,  2186,  2187,  2189,  2190,  2192,  2193,  2195,  2196,  2198,
    2199,  2205,  2206,  2206,  2212,  2212,  2214,  2215,  2215,  2220,
    2221,  2221,  2222,  2222,  2226,  2227,  2227,  2228,  2230,  2228,
    2250,  2251,  2252,  2253,  2255,  2256,  2259,  2260,  2261,  2262,
    2263,  2264,  2265,  2275,  2275,  2285,  2286,  2286,  2285,  2295,
    2295,  2305,  2305,  2314,  2314,  2314,  2347,  2346,  2357,  2358,
    2358,  2357,  2367,  2385,  2385,  2390,  2390,  2395,  2395,  2400,
    2400,  2405,  2405,  2410,  2410,  2415,  2415,  2420,  2420,  2425,
    2425,  2442,  2442,  2456,  2493,  2531,  2588,  2589,  2590,  2591,
    2592,  2594,  2595,  2595,  2596,  2596,  2597,  2597,  2598,  2598,
    2599,  2599,  2600,  2600,  2601,  2602,  2603,  2604,  2605,  2606,
    2607,  2608,  2609,  2610,  2611,  2612,  2613,  2614,  2615,  2616,
    2617,  2618,  2619,  2620,  2621,  2622,  2623,  2624,  2625,  2626,
    2627,  2633,  2633,  2634,  2634,  2636,  2636,  2638,  2638,  2638,
    2638,  2638,  2639,  2639,  2639,  2639,  2639,  2639,  2640,  2640,
    2640,  2640,  2640,  2641,  2641,  2641,  2641,  2641,  2642,  2642,
    2642,  2642,  2642,  2642,  2643,  2643,  2643,  2643,  2643,  2643,
    2643,  2644,  2644,  2644,  2644,  2644,  2644,  2645,  2645,  2645,
    2645,  2645,  2645,  2646,  2646,  2646,  2646,  2648,  2649,  2650,
    2650,  2650
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
  "UnicodeString", "IdType", "FloatType", "TypeInt8", "TypeUInt8",
  "TypeInt16", "TypeUInt16", "TypeInt32", "TypeUInt32", "TypeInt64",
  "TypeUInt64", "TypeFloat32", "TypeFloat64", "SetMacro", "GetMacro",
  "SetStringMacro", "GetStringMacro", "SetClampMacro", "SetObjectMacro",
  "GetObjectMacro", "BooleanMacro", "SetVector2Macro", "SetVector3Macro",
  "SetVector4Macro", "SetVector6Macro", "GetVector2Macro",
  "GetVector3Macro", "GetVector4Macro", "GetVector6Macro",
  "SetVectorMacro", "GetVectorMacro", "ViewportCoordinateMacro",
  "WorldCoordinateMacro", "TypeMacro", "VTK_BYTE_SWAP_DECL", "';'", "'('",
  "')'", "'{'", "'}'", "'<'", "'>'", "':'", "','", "'='", "'-'", "'+'",
  "'~'", "'*'", "'/'", "'%'", "'&'", "'|'", "'^'", "'['", "']'", "'!'",
  "'.'", "$accept", "strt", "$@1", "file_item", "extern", "namespace",
  "$@2", "class_def", "$@3", "$@4", "$@5", "$@6", "$@7", "$@8",
  "class_def_body", "$@9", "class_def_item", "optional_scope",
  "scope_list", "scope_list_item", "scope_type", "enum_def", "$@10",
  "$@11", "enum_list", "enum_item", "integer_value", "integer_literal",
  "integer_expression", "$@12", "$@13", "$@14", "math_unary_op",
  "math_binary_op", "template_internal_class", "internal_class",
  "internal_class_body", "type_def", "typedef_start", "using", "template",
  "$@15", "template_args", "$@16", "template_arg", "$@17",
  "class_or_typename", "maybe_template_id", "$@18", "legacy_function",
  "legacy_method", "function", "scoped_method", "scope", "method",
  "explicit_mod", "scoped_operator", "operator", "typecast_op_func",
  "$@19", "$@20", "op_func", "$@21", "op_sig", "$@22", "$@23", "func",
  "$@24", "func_trailer", "func_body", "func_sig", "$@25", "$@26", "@27",
  "constructor", "$@28", "constructor_sig", "$@29", "maybe_initializers",
  "more_initializers", "initializer", "destructor", "destructor_sig",
  "$@30", "args_list", "$@31", "more_args", "$@32", "arg", "$@33", "$@34",
  "maybe_indirect_id", "maybe_var_assign", "var_assign", "$@35", "var",
  "var_id_maybe_assign", "maybe_vars", "maybe_other_vars", "$@36",
  "other_var", "$@37", "$@38", "maybe_complex_var_id", "$@39",
  "complex_var_id", "$@40", "p_or_lp_or_la", "lp_or_la",
  "maybe_func_const", "maybe_array_or_args", "$@41",
  "maybe_indirect_maybe_var_id", "maybe_indirect_var_id", "maybe_var_id",
  "var_id", "maybe_var_array", "var_array", "$@42", "array", "$@43",
  "more_array", "array_size", "any_id", "sized_type_id", "special_type_id",
  "storage_type", "static_mod", "type", "type_red", "const_mod",
  "type_red1", "$@44", "templated_id", "$@45", "$@46", "$@47",
  "template_params", "$@48", "template_param", "maybe_scoped_id",
  "scoped_id", "class_id", "type_indirection", "pointers",
  "pointer_or_const_pointer", "type_red2", "type_simple", "type_id",
  "type_primitive", "value", "$@49", "maybe_comma", "more_values", "$@50",
  "literal", "$@51", "$@52", "$@53", "$@54", "$@55", "any_cast",
  "string_literal", "literal2", "macro", "$@56", "$@57", "$@58", "$@59",
  "$@60", "$@61", "$@62", "$@63", "$@64", "$@65", "$@66", "$@67", "$@68",
  "$@69", "$@70", "$@71", "$@72", "$@73", "$@74", "$@75", "$@76", "$@77",
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
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,    59,    40,    41,   123,   125,    60,
      62,    58,    44,    61,    45,    43,   126,    42,    47,    37,
      38,   124,    94,    91,    93,    33,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   157,   158,   159,   158,   160,   160,   160,   160,   160,
     160,   160,   160,   160,   160,   160,   160,   160,   160,   160,
     160,   160,   160,   160,   160,   160,   160,   161,   163,   162,
     162,   165,   164,   166,   164,   167,   164,   168,   164,   164,
     169,   164,   170,   164,   164,   171,   172,   171,   171,   173,
     173,   173,   173,   173,   173,   173,   173,   173,   173,   173,
     173,   173,   173,   173,   173,   173,   173,   173,   174,   174,
     175,   175,   176,   176,   176,   176,   177,   177,   177,   179,
     178,   180,   178,   181,   181,   181,   182,   182,   183,   183,
     183,   183,   184,   184,   184,   184,   184,   185,   186,   185,
     187,   185,   188,   185,   189,   189,   189,   190,   190,   190,
     190,   190,   190,   190,   190,   190,   190,   191,   192,   192,
     192,   192,   192,   193,   193,   193,   194,   194,   194,   194,
     195,   196,   196,   196,   197,   198,   197,   199,   200,   199,
     201,   201,   202,   201,   203,   203,   204,   205,   204,   206,
     207,   208,   209,   209,   209,   209,   209,   209,   209,   209,
     209,   210,   210,   211,   211,   211,   211,   211,   211,   211,
     211,   212,   212,   212,   213,   213,   214,   214,   214,   216,
     217,   215,   219,   218,   221,   222,   220,   224,   223,   225,
     225,   225,   225,   226,   226,   228,   227,   229,   230,   227,
     232,   231,   234,   233,   235,   235,   236,   236,   237,   238,
     240,   239,   241,   242,   241,   243,   243,   244,   243,   246,
     247,   245,   245,   248,   248,   249,   249,   251,   250,   252,
     252,   252,   253,   254,   254,   255,   256,   255,   258,   257,
     259,   257,   260,   261,   260,   262,   263,   262,   264,   264,
     264,   265,   265,   266,   266,   267,   268,   267,   267,   269,
     269,   270,   270,   271,   271,   272,   273,   273,   275,   274,
     277,   276,   278,   278,   279,   279,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   281,   281,   281,   281,   281,
     281,   281,   281,   281,   281,   282,   282,   283,   283,   283,
     283,   283,   283,   283,   284,   284,   285,   285,   286,   286,
     286,   287,   288,   288,   288,   289,   288,   291,   290,   292,
     290,   293,   290,   294,   295,   294,   296,   296,   297,   297,
     297,   297,   297,   297,   297,   297,   297,   298,   298,   299,
     299,   299,   299,   299,   299,   299,   300,   300,   300,   301,
     301,   302,   302,   303,   303,   303,   303,   303,   304,   304,
     305,   305,   305,   305,   305,   305,   305,   306,   306,   306,
     306,   306,   306,   306,   306,   306,   306,   306,   306,   306,
     306,   306,   306,   306,   306,   306,   306,   306,   306,   306,
     306,   306,   306,   306,   306,   306,   306,   306,   306,   306,
     306,   307,   308,   307,   309,   309,   310,   311,   310,   312,
     313,   312,   314,   312,   312,   315,   312,   316,   317,   312,
     318,   318,   318,   318,   319,   319,   320,   320,   320,   320,
     320,   320,   320,   322,   321,   323,   324,   325,   321,   326,
     321,   327,   321,   328,   329,   321,   330,   321,   331,   332,
     333,   321,   321,   334,   321,   335,   321,   336,   321,   337,
     321,   338,   321,   339,   321,   340,   321,   341,   321,   342,
     321,   343,   321,   321,   321,   321,   344,   344,   344,   344,
     344,   345,   345,   345,   345,   345,   345,   345,   345,   345,
     345,   345,   345,   345,   345,   345,   345,   345,   345,   345,
     345,   345,   345,   345,   345,   345,   345,   345,   345,   345,
     345,   345,   345,   345,   345,   345,   345,   345,   345,   345,
     345,   346,   346,   347,   347,   348,   348,   349,   349,   349,
     349,   349,   349,   349,   349,   349,   349,   349,   349,   349,
     349,   349,   349,   349,   349,   349,   349,   349,   349,   349,
     349,   349,   349,   349,   349,   349,   349,   349,   349,   349,
     349,   349,   349,   349,   349,   349,   349,   349,   349,   349,
     349,   349,   349,   349,   349,   349,   349,   350,   351,   352,
     352,   352
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     0,     3,     1,     3,     1,     1,     1,
       1,     3,     4,     1,     2,     3,     2,     2,     2,     3,
       2,     1,     2,     4,     4,     4,     1,     5,     0,     6,
       4,     0,     7,     0,    10,     0,     7,     0,    10,     4,
       0,     7,     0,    10,     4,     0,     0,     3,     3,     1,
       3,     1,     1,     3,     4,     2,     2,     1,     2,     3,
       3,     2,     3,     3,     2,     5,     1,     1,     0,     2,
       1,     3,     1,     2,     2,     2,     1,     1,     1,     0,
       6,     0,     5,     0,     1,     3,     1,     3,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     0,     3,
       0,     4,     0,     4,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     3,     3,
       2,     3,     2,     1,     5,     3,     4,     4,     4,     3,
       1,     4,     4,     3,     3,     0,     5,     1,     0,     4,
       2,     2,     0,     3,     1,     1,     0,     0,     3,     4,
       4,     2,     3,     4,     5,     3,     4,     6,     7,     5,
       6,     2,     3,     2,     3,     3,     1,     2,     2,     2,
       3,     1,     2,     2,     3,     3,     1,     2,     3,     0,
       0,     8,     0,     3,     0,     0,     7,     0,     3,     0,
       2,     2,     1,     1,     3,     0,     5,     0,     0,     9,
       0,     3,     0,     5,     0,     3,     0,     3,     4,     1,
       0,     5,     0,     0,     2,     1,     1,     0,     4,     0,
       0,     5,     1,     1,     2,     0,     1,     0,     3,     4,
       4,     3,     2,     0,     2,     0,     0,     4,     0,     2,
       0,     3,     2,     0,     5,     2,     0,     5,     1,     1,
       1,     1,     1,     0,     1,     0,     0,     5,     1,     1,
       2,     1,     2,     0,     1,     1,     0,     1,     0,     2,
       0,     5,     0,     1,     0,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     2,
       3,     2,     2,     3,     1,     2,     1,     2,     1,     2,
       2,     1,     1,     1,     1,     0,     3,     0,     5,     0,
       5,     0,     5,     1,     0,     4,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     3,     3,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     1,     1,
       2,     1,     1,     1,     2,     2,     2,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     0,     6,     0,     1,     0,     0,     4,     1,
       0,     3,     0,     3,     1,     0,     4,     0,     0,     9,
       1,     1,     1,     1,     1,     2,     1,     1,     1,     1,
       1,     1,     1,     0,     7,     0,     0,     0,     9,     0,
       5,     0,     5,     0,     0,    10,     0,     7,     0,     0,
       0,     9,     6,     0,     7,     0,     7,     0,     7,     0,
       7,     0,     7,     0,     7,     0,     7,     0,     7,     0,
       9,     0,     9,     4,     4,     7,     2,     2,     3,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     0,     2,     0,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     3,     3,     3,
       3,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       2,     3,     1,     0,     0,     0,     0,   364,   383,   384,
     385,   389,   390,   391,   392,   393,   394,   395,   396,   397,
     398,   386,   387,   388,   367,   368,   369,   370,   362,   363,
      81,     0,    13,   311,     0,   400,   399,     0,     0,     0,
     315,   130,     0,     0,   365,   304,     0,   235,     0,   366,
     360,   361,   381,   382,   371,   372,   373,   374,   375,   376,
     377,   378,   379,   380,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    26,     4,     9,     8,   238,
     238,    10,     0,     7,     0,     0,     0,     0,     0,     0,
     176,     5,     0,     0,   297,   306,     0,   308,   313,   314,
       0,   312,   353,   359,   358,    21,   364,   362,   363,   365,
     366,   360,   361,   295,   296,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   521,    35,   283,   284,   355,
      31,   354,     0,     0,   364,     0,     0,   365,   366,     0,
       0,   521,   319,     0,    79,   357,   521,    40,   356,     0,
     304,     0,     0,     0,   301,     0,   298,   135,     0,   339,
     343,   342,   341,   340,   344,   345,   521,    28,   329,   332,
     331,     0,     0,   328,   330,   333,   334,   335,     0,   336,
     521,   317,   305,   235,     0,   299,     0,     0,   521,   321,
       0,   435,   439,   441,     0,     0,   448,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   352,   351,   346,     0,   235,     0,   240,   348,   349,
       0,     0,     0,     0,     0,   238,     0,     0,     0,   193,
     521,    20,    17,    18,    16,    14,   278,   280,   279,     0,
     276,   251,   252,   277,   281,   282,     0,   177,   182,   151,
     187,   235,   225,     0,   266,   265,     0,   302,   307,   309,
     310,     0,     0,    22,     0,     0,    68,     0,    68,   364,
     362,   363,   365,   366,   360,   361,   178,     0,     0,     0,
      83,     0,     0,     0,    68,     0,   179,   303,     0,   134,
     142,   316,     0,     0,     0,     0,   133,     0,     0,     0,
       2,   300,   231,   236,     0,     0,     0,     0,   278,   280,
     279,   276,   277,   281,   282,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    11,   234,   239,   265,
       0,   347,   350,     6,   129,     0,   223,     0,     0,     0,
       0,    19,    15,     0,     0,   495,   496,   497,   498,   499,
     500,   501,   502,   508,   509,   503,   504,   505,   506,   507,
     510,   511,   512,   513,   514,   515,   516,   517,   518,   519,
     520,     0,   489,   490,   488,   481,   484,   485,   487,   482,
     483,   494,   491,   492,   493,     0,   486,   184,   480,   175,
     152,     0,   189,   189,     0,   227,   232,   226,   261,     0,
       0,   245,   267,   272,   195,   197,   161,   338,   329,   332,
     331,   328,   330,   333,   334,     0,   174,   155,   200,     0,
     337,     0,   553,   536,   564,   566,   565,   557,   570,   548,
     544,   545,   543,   546,   547,   561,   539,   538,   558,   559,
     549,   527,   550,   551,   552,   554,   556,   537,   560,   531,
     567,   568,   571,   555,   569,   562,   495,   496,   575,   574,
     573,   576,   563,   535,   521,   521,   572,   540,   541,   525,
     521,   521,    39,   533,   521,   534,   542,   532,   522,   526,
     528,   530,   529,    93,    95,    94,    96,    92,   327,   326,
       0,   323,     0,     0,     0,     0,    23,     0,     0,    84,
      86,    83,    44,     0,     0,   213,     0,   156,     0,   144,
     145,     0,   137,     0,   146,   146,    30,     2,   132,   131,
      24,     0,   230,     3,   238,   149,    25,     0,   433,     0,
       0,     0,   443,   446,     0,     0,   453,   457,   461,   465,
     455,   459,   463,   467,   469,   471,   473,   474,     0,   241,
     127,   224,   128,   126,    12,   194,     0,     0,   476,   477,
       0,   162,   192,     0,     0,   183,   188,   229,     0,   246,
     262,   269,     0,   213,     0,   153,   209,     0,   204,   202,
       0,     0,     0,     0,     0,     0,    37,   324,     0,     0,
       0,    69,    70,    72,    45,    33,    45,   320,    82,    83,
       0,     0,    42,    45,     0,   219,   154,     0,   136,   138,
     146,   141,   147,   140,     3,   318,    27,   237,   322,     0,
     436,   440,   442,     0,     0,   449,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   404,   478,   479,
     185,   191,   190,   424,   427,   429,   428,   430,   431,   426,
     420,   422,   421,   423,   415,   402,   412,   410,   432,   228,
     401,     0,   414,   409,   255,   270,     0,     0,   210,     0,
     201,   213,     0,   159,   580,   581,   579,   577,   578,    68,
       0,    75,    73,    74,     0,    46,    68,    46,    85,   278,
     276,   277,   102,   104,   105,   106,    97,    88,    87,    98,
      89,    91,    90,    80,    68,    46,   180,   222,   215,   214,
     216,     0,     0,   160,   142,   143,   225,    29,     0,     0,
     444,     0,     0,   452,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   405,     0,   213,     0,     0,     0,
       0,   417,   425,   256,   247,   258,   274,   196,     0,   213,
     206,     0,     0,   157,     0,   325,    71,    76,    77,    78,
      36,     0,     0,     0,    32,     0,   115,   116,   107,   108,
     109,   110,   111,   112,   113,   114,   100,     0,     0,    41,
     189,   217,   263,   158,   139,   148,   434,   437,     0,   447,
     450,   454,   458,   462,   466,   456,   460,   464,   468,     0,
       0,   475,     0,     0,   406,   413,   411,     0,   213,   275,
       0,   198,     0,     0,   205,   521,   203,    45,     0,   364,
     362,   363,    57,     0,     0,   365,     0,   171,   366,   360,
     361,   381,   382,   371,   372,   373,   374,   375,   376,   377,
     378,   379,   380,     0,    67,     0,   238,    47,   238,    52,
      51,     0,     0,     0,     0,     0,   166,    49,     0,    66,
      48,    45,     0,     0,    99,    45,   181,   219,   249,   250,
     248,   220,   263,   266,   264,     0,   523,     0,     0,     0,
     186,   416,   404,     0,     0,   271,   213,   211,   206,     0,
      46,     0,     0,   297,     0,     0,     0,    56,    55,     0,
       0,     0,     0,   172,     0,   167,     0,   173,   521,   163,
       0,     0,   238,     0,     0,    64,    61,   168,    58,   169,
      46,   103,   101,    46,   218,   225,   259,     0,   263,   242,
     438,     0,   451,   470,   472,   407,     0,     0,   253,     0,
     207,   208,    38,   165,   170,   123,   521,   523,   120,     0,
       0,   122,     0,     0,     0,     0,   117,    62,    59,   164,
       0,     0,     0,     0,    53,    50,     0,    63,    60,    34,
      43,   221,   243,   260,   445,   524,     0,   403,   418,   254,
     257,   199,     0,     0,   119,   118,   121,   150,     0,    54,
     255,   408,     0,   523,   125,    65,   244,     0,     0,   419,
     124
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     3,    86,    87,    88,   303,    89,   278,   706,
     276,   699,   294,   724,   705,   781,   867,   513,   611,   612,
     782,    90,   291,   153,   518,   519,   716,   508,   718,   797,
     883,   785,   719,   796,   917,   918,   968,    91,    92,    93,
      94,   300,   531,   734,   532,   533,   534,   631,   736,    95,
     872,    96,    97,   256,   873,   874,    98,    99,   100,   525,
     800,   257,   412,   258,   580,   756,   259,   413,   585,   241,
     260,   593,   594,   906,   876,   598,   438,   691,   690,   834,
     770,   595,   596,   769,   624,   625,   729,   887,   730,   731,
     945,   355,   416,   417,   588,   101,   261,   224,   196,   544,
     225,   226,   350,   891,  1010,   262,   684,   892,   263,  1000,
     764,   828,   947,   419,   893,   264,   421,   422,   423,   591,
     766,   592,   830,   439,   137,   138,   912,   103,   104,   105,
     106,   107,   168,   108,   308,   289,   317,   510,   700,   511,
     678,   109,   150,   227,   228,   229,   111,   112,   113,   114,
     679,   758,   755,   902,   996,   680,   760,   759,   757,   827,
    1012,   681,   682,   683,   115,   639,   326,   739,   895,   327,
     328,   643,   808,   644,   331,   742,   897,   647,   651,   648,
     652,   649,   653,   650,   654,   655,   656,   407,   497,   274,
     951,   498,   499,   500,   501,   502
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -917
static const yytype_int16 yypact[] =
{
    -917,    52,  -917,  5135,  5259,  6825,  6259,    51,  -917,  -917,
    -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,
    -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,   -14,    98,
    6825,  5274,  -917,  -917,  5714,  -917,  -917,  5823,  6259,   -63,
    -917,  -917,    81,   264,   170,    55,  5932,  -917,   -44,   181,
     108,   132,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,
    -917,  -917,  -917,  -917,    38,    49,    53,    54,    64,    86,
     113,   122,   156,   158,   171,   172,   196,   198,   201,   209,
     215,   216,   219,   220,   224,  -917,  -917,  -917,  -917,    10,
      10,  -917,  6041,  -917,  5496,   134,   134,   134,   134,   134,
    -917,  -917,  6615,  6259,  -917,    15,  6368,   151,   188,  -917,
     147,  -917,  -917,  -917,  -917,  -917,   191,   206,   272,   387,
     395,   412,   413,  -917,  -917,  -917,  -917,  -917,  -917,  -917,
    -917,  -917,  -917,  -917,  -917,  -917,    36,  -917,  -917,  -917,
     229,  -917,   343,   343,   -22,   343,   343,     0,    22,   289,
     282,  -917,  -917,   240,  -917,  -917,  -917,   239,  -917,  5823,
     332,  6150,   249,  6259,  -917,   296,  -917,   251,   408,  -917,
    -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,   -22,   300,
     301,   408,   408,     0,    22,   303,   310,   188,   271,  -917,
    -917,  -917,  -917,  -917,  4932,  -917,   138,  5714,  -917,  -917,
    6850,  -917,  -917,  -917,  6850,  6850,  -917,  6850,  6850,  6850,
    6850,  6850,  6850,  6850,  6850,  6850,  6850,  6850,  6850,  6850,
    6850,  -917,  -917,  -917,   278,  -917,  4819,  -917,    19,  -917,
     280,   281,   848,   848,  4819,    10,   134,   134,  6720,  -917,
    -917,  -917,  -917,  -917,  -917,  -917,   325,   300,   301,  6900,
     326,  -917,  -917,   327,   303,   310,  6795,  -917,  -917,  -917,
    -917,  -917,   279,  4456,   277,   -58,   330,  -917,  -917,  -917,
    -917,   408,  4606,  -917,  1000,  5605,   284,  5605,   284,  -917,
    -917,  -917,  -917,  -917,  -917,  -917,  -917,   408,  1154,  5605,
    6850,   295,  1308,  5605,   284,  6259,  -917,  -917,  4588,  -917,
    6585,  -917,  1462,   302,   304,   318,  -917,  1616,  5605,   166,
    -917,  -917,  -917,  -917,   308,  6850,  1770,  5605,  -917,  -917,
    -917,  -917,  -917,  -917,  -917,   311,  6850,  6850,  6850,   314,
     324,  6850,   335,   336,   338,   339,   340,   362,   368,   376,
     377,   378,   381,   391,   397,   393,  -917,   396,  -917,  -917,
    4819,  -917,  -917,  -917,  -917,   405,  -917,  6850,   407,   414,
     422,  -917,  -917,   -58,  1924,   389,   404,  -917,  -917,  -917,
    -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,
    -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,
    -917,   423,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,
    -917,  -917,  -917,  -917,  -917,   283,  -917,  -917,  -917,  -917,
    -917,   464,    40,    40,   184,  -917,  -917,  -917,  -917,   424,
    4819,  -917,  -917,  -917,  -917,  -917,  -917,  -917,    28,   -12,
       5,   139,   175,   123,   125,  6850,  -917,  -917,  -917,   426,
    -917,   469,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,
    -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,
    -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,
    -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,
    -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,
    -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,
    -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,
     425,   427,   351,   429,   428,   433,  -917,   431,   434,   432,
     430,  6850,  -917,   439,   443,   445,  6850,  -917,   488,  -917,
    -917,   444,   447,   533,  6850,  6850,  -917,  -917,  -917,  -917,
    -917,   448,  -917,   453,    15,  -917,  -917,   452,  -917,   451,
     450,   459,  -917,  -917,   454,  6477,  -917,  -917,  -917,  -917,
    -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  6850,  -917,
    -917,  -917,  -917,  -917,  -917,  -917,   449,   455,  -917,  -917,
     462,  -917,  -917,   585,   586,  -917,  -917,  -917,  4768,  -917,
    -917,   458,   461,   445,  5605,  -917,  -917,   470,   463,  -917,
    4663,  2078,  2232,  2386,  2540,  2694,  -917,  -917,   408,   408,
     408,  -917,   465,  -917,  -917,  -917,  -917,  -917,  -917,  6850,
    4531,   468,  -917,  -917,   472,    41,  -917,  4693,  -917,  -917,
    6850,  -917,  -917,  -917,   474,  -917,  -917,  -917,  -917,  6259,
    -917,  -917,  -917,  6477,  6477,  -917,   482,  6477,  6477,  6477,
    6477,  6477,  6477,  6477,  6477,  6477,  6477,   477,  -917,  -917,
    -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,
    -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,
    -917,   481,   611,  -917,   144,  -917,   486,   483,  -917,   408,
    -917,   445,  6850,  -917,  -917,  -917,  -917,  -917,  -917,   284,
    5605,  -917,  -917,  -917,   351,    43,   284,    59,  -917,   -22,
       0,    22,  -917,  -917,  -917,  -917,   364,  -917,  -917,  -917,
    -917,   188,  -917,  -917,   284,    82,  -917,  -917,  -917,  -917,
     485,  6259,  6850,  -917,  6585,  -917,   279,  -917,   489,  6259,
    -917,   493,  6477,  -917,   494,   495,   496,   497,   498,   499,
     500,   501,   502,   503,  -917,   504,   445,  4803,  4768,   446,
     446,  -917,  -917,  -917,  -917,  -917,  4531,  -917,   506,   445,
     505,   507,   510,  -917,   487,  -917,  -917,  -917,  -917,  -917,
    -917,  3772,   508,   511,  -917,  4531,  -917,  -917,  -917,  -917,
    -917,  -917,  -917,  -917,  -917,  -917,  -917,  4531,   513,  -917,
      40,  -917,  5377,  -917,  -917,  -917,  -917,  -917,   509,  -917,
    -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,   627,
     628,  -917,   516,   517,  -917,  -917,  -917,  6259,   445,  -917,
     512,  -917,   518,   408,  -917,  -917,  -917,  -917,  4294,    28,
     -12,     5,  -917,  3904,  4424,   139,   520,   607,   175,   123,
     125,   523,   524,   525,   526,   528,   529,   530,   532,   534,
     535,   538,   539,   540,  -917,  6850,    10,  -917,    10,  -917,
    -917,  4034,   134,   134,  6850,   134,  -917,  -917,  6690,  -917,
    -917,  -917,   542,  4531,  -917,  -917,  -917,    41,  -917,  -917,
    -917,  -917,   390,   277,  -917,   544,  -917,   545,   546,   547,
    -917,  -917,   543,   536,   550,  -917,   445,  -917,   505,  2848,
      97,  6850,  6850,   289,  4987,  6825,  4987,  -917,  -917,   243,
     134,   134,  6720,  -917,  6850,  -917,  4164,  -917,  -917,  -917,
     553,   555,    10,   134,   134,  -917,  -917,  -917,  -917,  -917,
     102,  -917,  -917,   107,  -917,   279,  -917,   554,  5377,  -917,
    -917,  3310,  -917,  -917,  -917,   557,   558,   556,   625,   562,
    -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,   200,
     200,  -917,   200,  5008,  6850,  5008,  -917,  -917,  -917,  -917,
    4294,   563,  6850,  3002,  -917,  -917,   559,  -917,  -917,  -917,
    -917,  -917,  -917,  -917,  -917,  -917,  4768,  -917,  -917,  -917,
    -917,  -917,  3156,  3464,  -917,  -917,  -917,  -917,   566,  -917,
     144,  -917,  4803,  -917,  -917,  -917,  -917,   565,  3618,  -917,
    -917
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -917,  -270,  -917,  -917,  -917,  -917,  -917,   -72,  -917,  -917,
    -917,  -917,  -917,  -917,  -587,  -917,  -917,  -273,    -2,  -917,
    -917,   -84,  -917,  -917,  -476,  -917,  -917,  -541,  -660,  -917,
    -917,  -917,  -917,  -917,  -917,  -227,  -680,   -78,  -917,   -77,
    -505,  -917,   -29,  -917,  -917,  -917,  -917,  -466,  -917,  -917,
    -917,   -27,  -917,  -917,  -750,  -917,  -917,   -70,   435,  -917,
    -917,  -146,  -917,  -917,  -917,  -917,  -250,  -917,  -399,   -81,
    -917,  -917,  -917,  -917,  -261,  -917,  -917,  -917,  -917,  -202,
    -125,  -501,  -917,  -917,  -532,  -917,  -175,  -917,  -917,  -917,
    -917,   480,  -690,  -917,  -917,   -71,  -182,   -83,     2,  -917,
     173,  -917,  -917,  -837,  -917,  -177,  -917,  -917,  -917,  -917,
    -296,  -917,  -917,  -917,  -917,  -761,  -178,  -661,  -917,  -917,
    -917,  -917,  -917,    -4,  -917,  -917,     1,   -35,    90,  -111,
     613,   612,  -917,   -33,  -917,  -917,  -917,  -246,  -917,  -917,
     131,   -10,    31,   -86,  -917,   514,  -180,  -291,     8,  -232,
    -726,  -917,  -181,  -917,  -917,  -727,  -917,  -917,  -917,  -917,
    -917,  -917,  -917,  -630,   -59,  -917,  -917,  -917,  -917,  -917,
    -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,
    -917,  -917,  -917,  -917,  -917,  -917,  -917,  -917,   519,   -98,
    -916,  -917,  -877,  -917,  -917,  -917
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -406
static const yytype_int16 yytable[] =
{
     136,   140,   163,   286,   102,   515,   410,   230,   233,   535,
     187,   437,   139,   141,   586,   242,   243,   244,   245,   268,
     232,   524,   235,   765,   237,   626,   154,   157,   630,   707,
     823,   514,   824,   189,   110,   162,   725,   527,   155,   158,
     543,   894,   496,   517,   348,   621,   805,   523,   777,   778,
     779,  1003,     2,   288,   221,   946,   496,   359,   292,   221,
     496,   686,   541,   221,   777,   778,   779,   236,   165,   633,
     496,   547,  -339,   177,   995,   496,   167,   424,   302,   717,
    -343,   425,  -343,   582,   496,   583,   418,   777,   778,   779,
     169,   197,   307,   920,  -341,   238,   149,  1018,   265,  -342,
     316,   727,   777,   778,   779,   192,   829,   777,   778,   779,
     409,   993,   777,   778,   779,   193,  -340,   152,   170,   171,
    -343,   933,  -339,  -280,   163,   882,   995,   164,   166,   825,
     826,   894,   496,   266,   728,   187,   195,   884,   172,   191,
    -279,   995,   364,   708,  -233,  -339,   357,   357,   187,   187,
     139,   141,   360,   155,   158,   361,   362,   222,   189,   772,
     223,   199,   222,  -278,   735,   223,   222,   152,   569,   351,
     314,   189,   189,   200,   188,   275,   981,   420,   173,   174,
     175,   780,   234,   584,   201,  -339,   151,   894,   202,   203,
     152,   773,  -342,   267,    33,   309,   325,   784,   315,   204,
     329,   330,  -344,   332,   333,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,  -344,   176,  -345,
     799,   205,   349,   942,   822,   717,  -345,   347,   356,   356,
     349,   803,  -342,  -341,   363,   962,   971,   832,   187,   187,
     989,   272,  -344,   590,   717,   990,   973,   974,   206,   164,
     910,   195,   363,   297,   187,   991,   717,   207,  -281,   349,
    -282,   189,   189,   414,  -341,   187,  -345,   634,   239,  -340,
    1011,   240,   312,   178,  -276,  -340,   871,   189,   191,   763,
     313,   273,   271,   975,   311,  1017,   520,   411,   189,  1004,
    1005,   208,  1006,   209,   940,   971,   904,  -268,   943,   301,
     542,   179,   180,   441,  -341,   190,   210,   211,   313,   191,
    -277,   363,   304,   305,   199,  -340,   198,   181,   587,   182,
     199,   183,   549,   550,   551,  -278,   313,   554,  -278,   528,
    -278,   212,  -278,   213,   965,   249,   214,   966,   919,   693,
    -280,   967,   717,  -280,   215,  -280,   349,  -280,   687,   765,
     216,   217,   279,   571,   218,   219,   608,   609,   610,   220,
     178,   184,   185,   186,   929,   509,   733,   509,   277,   496,
     496,   496,   496,   496,   959,   646,   287,   290,   293,   509,
     280,   281,   192,   509,   296,   311,   601,   602,   179,   180,
     298,   299,   603,   604,  -343,  -342,   605,  -344,   509,   318,
     282,   886,   427,   440,  -345,   306,  -279,   509,   183,  -279,
     963,  -279,   346,  -279,   353,   354,   349,   178,   440,  -339,
    -341,  -340,   415,   979,   426,   512,   774,   319,   320,   440,
    -268,   597,   521,   783,   221,   786,   787,   579,   538,   537,
     283,   284,   285,   535,   545,   179,   180,   321,   184,   185,
     186,   798,   539,   548,   775,   178,   552,   664,   665,   666,
     667,   668,   669,   740,   741,   183,   553,   744,   745,   746,
     747,   748,   749,   750,   751,   752,   753,   555,   556,   187,
     557,   558,   559,   179,   180,   888,   889,   322,   323,   324,
     123,   124,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   189,   183,   560,   184,   185,   186,   788,   789,
     561,   790,   791,   792,   793,   794,   795,   520,   562,   563,
     564,  -276,   597,   565,  -276,   890,  -276,   566,  -276,  -277,
     632,   632,  -277,   567,  -277,   568,  -277,   222,   313,   570,
     223,   572,   576,   184,   185,   186,  -281,  -282,   573,  -281,
    -282,  -281,  -282,  -281,  -282,   187,   574,   577,   581,   578,
     589,   599,   810,   600,   657,   606,   614,   187,   615,   607,
     616,   617,   618,   620,   619,   187,   187,   187,   189,   622,
     623,  -212,   627,   925,   628,    39,   641,   721,   635,   629,
     189,   636,   638,   640,   187,   642,   645,   660,   189,   189,
     189,   661,   662,   658,   689,   688,   723,   704,   726,   659,
     722,  -273,   737,   937,   685,   520,   720,   189,   743,   754,
     761,   762,   767,   768,   837,   806,   632,   801,   939,   809,
     811,   812,   813,   814,   815,   816,   817,   818,   898,   899,
     821,   831,   835,   613,   819,   820,   836,   833,   881,   880,
     885,   896,   900,   901,   907,   926,   187,   927,  -295,  -296,
    -285,  -286,   964,  -287,  -288,  -289,   905,  -290,   999,  -291,
    -292,   187,   939,  -293,  -294,   928,   957,   496,   941,   189,
     950,   952,   953,   954,   509,   955,   958,   984,   597,   985,
     992,   998,   976,  1009,   189,  -405,   997,   868,  1001,  1007,
    1015,  1019,   776,   869,   870,   804,   960,   436,   908,   866,
     877,   875,   944,   358,  1016,   949,   903,   637,   269,   496,
     270,   956,   879,     0,   187,   187,   187,   187,   597,   738,
       0,   440,   939,   721,     0,     0,     0,   909,     0,   701,
     702,   703,   352,     0,     0,     0,     0,   189,   189,   189,
     189,   496,   721,     0,     0,     0,   722,     0,   440,     0,
       0,     0,   720,     0,   721,     0,     0,   286,   408,     0,
     496,   496,     0,   921,     0,   722,     0,     0,     0,     0,
       0,   720,   878,   930,     0,   931,   496,   722,     0,     0,
     509,   935,   936,   720,   938,     0,     0,     0,   349,   932,
     187,   934,     0,     0,     0,     0,   948,     0,     0,   163,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     771,   802,     0,   189,     0,     0,     0,     0,     0,   807,
     983,     0,     0,     0,     0,   613,     0,     0,     0,   977,
     978,     0,     0,     0,   922,     0,     0,     0,     0,   986,
     721,     0,   987,   988,     0,     0,     0,   318,     0,     0,
       0,   597,     0,     0,     0,     0,     0,     0,  1002,     0,
       0,     0,   922,   722,   265,     0,     0,     0,     0,   720,
       0,     0,     0,     0,     0,   319,   320,     0,   349,     0,
       0,     0,   221,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   321,     0,   597,   363,     0,
     969,   970,   972,     0,     0,     0,     0,     0,   363,     0,
     597,     0,   139,   141,   158,     0,     0,   982,   913,     0,
       0,     0,     0,     0,   164,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   349,   322,   323,   324,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
       0,     0,     0,   187,   771,     0,     0,     0,     0,   969,
     970,   972,     0,     0,     0,     0,     0,     0,   363,   187,
       0,     0,     0,     0,     0,     0,   189,     0,     0,     0,
       0,     0,     0,     0,     0,   222,     0,     0,   223,     0,
       0,     0,   189,   442,   443,   444,   445,   446,   447,   448,
     449,   450,   451,   452,   453,   454,   455,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   456,   457,   458,
     459,   460,   461,   462,   463,   464,   465,    35,    36,     0,
     466,     0,   467,   468,   469,   470,   471,   472,   473,   474,
     475,     0,   476,   477,     0,   478,   479,   480,   481,   367,
     368,   369,   370,   371,   372,   373,   374,   375,   376,   377,
     378,   379,   380,   381,   382,   383,   384,   385,   386,   387,
     388,   389,   390,   482,   483,   484,   485,   486,   487,   488,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   489,   490,     0,   491,   492,   392,
     393,   493,   394,   395,   396,   397,   398,   399,   400,   401,
     402,   403,   404,   494,     0,   406,   495,   442,   443,   444,
     445,   446,   447,   448,   449,   450,   451,   452,   453,   454,
     455,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   456,   457,   458,   459,   460,   461,   462,   463,   464,
     465,    35,    36,     0,   466,     0,   467,   468,   469,   470,
     471,   472,   473,   474,   475,     0,   476,   477,     0,   478,
     479,   480,   481,   367,   368,   369,   370,   371,   372,   373,
     374,   375,   376,   377,   378,   379,   380,   381,   382,   383,
     384,   385,   386,   387,   388,   389,   390,   482,   483,   484,
     485,   486,   487,   488,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   489,   490,
     516,   491,     0,   392,   393,   493,   394,   395,   396,   397,
     398,   399,   400,   401,   402,   403,   404,   494,     0,   406,
     495,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,   456,   457,   458,   459,   460,
     461,   462,   463,   464,   465,    35,    36,     0,   466,     0,
     467,   468,   469,   470,   471,   472,   473,   474,   475,     0,
     476,   477,     0,   478,   479,   480,   481,   367,   368,   369,
     370,   371,   372,   373,   374,   375,   376,   377,   378,   379,
     380,   381,   382,   383,   384,   385,   386,   387,   388,   389,
     390,   482,   483,   484,   485,   486,   487,   488,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   489,   490,     0,   491,   522,   392,   393,   493,
     394,   395,   396,   397,   398,   399,   400,   401,   402,   403,
     404,   494,     0,   406,   495,   442,   443,   444,   445,   446,
     447,   448,   449,   450,   451,   452,   453,   454,   455,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,   456,
     457,   458,   459,   460,   461,   462,   463,   464,   465,    35,
      36,     0,   466,     0,   467,   468,   469,   470,   471,   472,
     473,   474,   475,     0,   476,   477,     0,   478,   479,   480,
     481,   367,   368,   369,   370,   371,   372,   373,   374,   375,
     376,   377,   378,   379,   380,   381,   382,   383,   384,   385,
     386,   387,   388,   389,   390,   482,   483,   484,   485,   486,
     487,   488,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   489,   490,     0,   491,
     536,   392,   393,   493,   394,   395,   396,   397,   398,   399,
     400,   401,   402,   403,   404,   494,     0,   406,   495,   442,
     443,   444,   445,   446,   447,   448,   449,   450,   451,   452,
     453,   454,   455,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,   456,   457,   458,   459,   460,   461,   462,
     463,   464,   465,    35,    36,     0,   466,     0,   467,   468,
     469,   470,   471,   472,   473,   474,   475,     0,   476,   477,
       0,   478,   479,   480,   481,   367,   368,   369,   370,   371,
     372,   373,   374,   375,   376,   377,   378,   379,   380,   381,
     382,   383,   384,   385,   386,   387,   388,   389,   390,   482,
     483,   484,   485,   486,   487,   488,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     489,   490,   540,   491,     0,   392,   393,   493,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   494,
       0,   406,   495,   442,   443,   444,   445,   446,   447,   448,
     449,   450,   451,   452,   453,   454,   455,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   456,   457,   458,
     459,   460,   461,   462,   463,   464,   465,    35,    36,     0,
     466,     0,   467,   468,   469,   470,   471,   472,   473,   474,
     475,     0,   476,   477,     0,   478,   479,   480,   481,   367,
     368,   369,   370,   371,   372,   373,   374,   375,   376,   377,
     378,   379,   380,   381,   382,   383,   384,   385,   386,   387,
     388,   389,   390,   482,   483,   484,   485,   486,   487,   488,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   489,   490,   546,   491,     0,   392,
     393,   493,   394,   395,   396,   397,   398,   399,   400,   401,
     402,   403,   404,   494,     0,   406,   495,   442,   443,   444,
     445,   446,   447,   448,   449,   450,   451,   452,   453,   454,
     455,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   456,   457,   458,   459,   460,   461,   462,   463,   464,
     465,    35,    36,     0,   466,     0,   467,   468,   469,   470,
     471,   472,   473,   474,   475,     0,   476,   477,     0,   478,
     479,   480,   481,   367,   368,   369,   370,   371,   372,   373,
     374,   375,   376,   377,   378,   379,   380,   381,   382,   383,
     384,   385,   386,   387,   388,   389,   390,   482,   483,   484,
     485,   486,   487,   488,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   489,   490,
       0,   491,   575,   392,   393,   493,   394,   395,   396,   397,
     398,   399,   400,   401,   402,   403,   404,   494,     0,   406,
     495,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,   456,   457,   458,   459,   460,
     461,   462,   463,   464,   465,    35,    36,     0,   466,     0,
     467,   468,   469,   470,   471,   472,   473,   474,   475,     0,
     476,   477,     0,   478,   479,   480,   481,   367,   368,   369,
     370,   371,   372,   373,   374,   375,   376,   377,   378,   379,
     380,   381,   382,   383,   384,   385,   386,   387,   388,   389,
     390,   482,   483,   484,   485,   486,   487,   488,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   489,   490,   694,   491,     0,   392,   393,   493,
     394,   395,   396,   397,   398,   399,   400,   401,   402,   403,
     404,   494,     0,   406,   495,   442,   443,   444,   445,   446,
     447,   448,   449,   450,   451,   452,   453,   454,   455,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,   456,
     457,   458,   459,   460,   461,   462,   463,   464,   465,    35,
      36,     0,   466,     0,   467,   468,   469,   470,   471,   472,
     473,   474,   475,     0,   476,   477,     0,   478,   479,   480,
     481,   367,   368,   369,   370,   371,   372,   373,   374,   375,
     376,   377,   378,   379,   380,   381,   382,   383,   384,   385,
     386,   387,   388,   389,   390,   482,   483,   484,   485,   486,
     487,   488,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   489,   490,   695,   491,
       0,   392,   393,   493,   394,   395,   396,   397,   398,   399,
     400,   401,   402,   403,   404,   494,     0,   406,   495,   442,
     443,   444,   445,   446,   447,   448,   449,   450,   451,   452,
     453,   454,   455,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,   456,   457,   458,   459,   460,   461,   462,
     463,   464,   465,    35,    36,     0,   466,     0,   467,   468,
     469,   470,   471,   472,   473,   474,   475,     0,   476,   477,
       0,   478,   479,   480,   481,   367,   368,   369,   370,   371,
     372,   373,   374,   375,   376,   377,   378,   379,   380,   381,
     382,   383,   384,   385,   386,   387,   388,   389,   390,   482,
     483,   484,   485,   486,   487,   488,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     489,   490,   696,   491,     0,   392,   393,   493,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   494,
       0,   406,   495,   442,   443,   444,   445,   446,   447,   448,
     449,   450,   451,   452,   453,   454,   455,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   456,   457,   458,
     459,   460,   461,   462,   463,   464,   465,    35,    36,     0,
     466,     0,   467,   468,   469,   470,   471,   472,   473,   474,
     475,     0,   476,   477,     0,   478,   479,   480,   481,   367,
     368,   369,   370,   371,   372,   373,   374,   375,   376,   377,
     378,   379,   380,   381,   382,   383,   384,   385,   386,   387,
     388,   389,   390,   482,   483,   484,   485,   486,   487,   488,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   489,   490,     0,   491,   697,   392,
     393,   493,   394,   395,   396,   397,   398,   399,   400,   401,
     402,   403,   404,   494,     0,   406,   495,   442,   443,   444,
     445,   446,   447,   448,   449,   450,   451,   452,   453,   454,
     455,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   456,   457,   458,   459,   460,   461,   462,   463,   464,
     465,    35,    36,     0,   466,     0,   467,   468,   469,   470,
     471,   472,   473,   474,   475,     0,   476,   477,     0,   478,
     479,   480,   481,   367,   368,   369,   370,   371,   372,   373,
     374,   375,   376,   377,   378,   379,   380,   381,   382,   383,
     384,   385,   386,   387,   388,   389,   390,   482,   483,   484,
     485,   486,   487,   488,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   489,   490,
       0,   491,     0,   392,   393,   493,   394,   395,   396,   397,
     398,   399,   400,   401,   402,   403,   404,   494,   698,   406,
     495,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,   456,   457,   458,   459,   460,
     461,   462,   463,   464,   465,    35,    36,     0,   466,     0,
     467,   468,   469,   470,   471,   472,   473,   474,   475,     0,
     476,   477,     0,   478,   479,   480,   481,   367,   368,   369,
     370,   371,   372,   373,   374,   375,   376,   377,   378,   379,
     380,   381,   382,   383,   384,   385,   386,   387,   388,   389,
     390,   482,   483,   484,   485,   486,   487,   488,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   489,   490,   961,   491,     0,   392,   393,   493,
     394,   395,   396,   397,   398,   399,   400,   401,   402,   403,
     404,   494,     0,   406,   495,   442,   443,   444,   445,   446,
     447,   448,   449,   450,   451,   452,   453,   454,   455,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,   456,
     457,   458,   459,   460,   461,   462,   463,   464,   465,    35,
      36,     0,   466,     0,   467,   468,   469,   470,   471,   472,
     473,   474,   475,     0,   476,   477,     0,   478,   479,   480,
     481,   367,   368,   369,   370,   371,   372,   373,   374,   375,
     376,   377,   378,   379,   380,   381,   382,   383,   384,   385,
     386,   387,   388,   389,   390,   482,   483,   484,   485,   486,
     487,   488,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   489,   490,  1008,   491,
       0,   392,   393,   493,   394,   395,   396,   397,   398,   399,
     400,   401,   402,   403,   404,   494,     0,   406,   495,   442,
     443,   444,   445,   446,   447,   448,   449,   450,   451,   452,
     453,   454,   455,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,   456,   457,   458,   459,   460,   461,   462,
     463,   464,   465,    35,    36,     0,   466,     0,   467,   468,
     469,   470,   471,   472,   473,   474,   475,     0,   476,   477,
       0,   478,   479,   480,   481,   367,   368,   369,   370,   371,
     372,   373,   374,   375,   376,   377,   378,   379,   380,   381,
     382,   383,   384,   385,   386,   387,   388,   389,   390,   482,
     483,   484,   485,   486,   487,   488,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     489,   490,     0,   491,  1013,   392,   393,   493,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   494,
       0,   406,   495,   442,   443,   444,   445,   446,   447,   448,
     449,   450,   451,   452,   453,   454,   455,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   456,   457,   458,
     459,   460,   461,   462,   463,   464,   465,    35,    36,     0,
     466,     0,   467,   468,   469,   470,   471,   472,   473,   474,
     475,     0,   476,   477,     0,   478,   479,   480,   481,   367,
     368,   369,   370,   371,   372,   373,   374,   375,   376,   377,
     378,   379,   380,   381,   382,   383,   384,   385,   386,   387,
     388,   389,   390,   482,   483,   484,   485,   486,   487,   488,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   490,   994,   491,     0,   392,
     393,   493,   394,   395,   396,   397,   398,   399,   400,   401,
     402,   403,   404,   494,     0,   406,   495,   442,   443,   444,
     445,   446,   447,   448,   449,   450,   451,   452,   453,   454,
     455,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   456,   457,   458,   459,   460,   461,   462,   463,   464,
     465,    35,    36,     0,   466,     0,   467,   468,   469,   470,
     471,   472,   473,   474,   475,     0,   476,   477,     0,   478,
     479,   480,   481,   367,   368,   369,   370,   371,   372,   373,
     374,   375,   376,   377,   378,   379,   380,   381,   382,   383,
     384,   385,   386,   387,   388,   389,   390,   482,   483,   484,
     485,   486,   487,   488,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1014,   490,
       0,   491,     0,   392,   393,   493,   394,   395,   396,   397,
     398,   399,   400,   401,   402,   403,   404,   494,     0,   406,
     495,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,   456,   457,   458,   459,   460,
     461,   462,   463,   464,   465,    35,    36,     0,   466,     0,
     467,   468,   469,   470,   471,   472,   473,   474,   475,     0,
     476,   477,     0,   478,   479,   480,   481,   367,   368,   369,
     370,   371,   372,   373,   374,   375,   376,   377,   378,   379,
     380,   381,   382,   383,   384,   385,   386,   387,   388,   389,
     390,   482,   483,   484,   485,   486,   487,   488,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  1020,   490,     0,   491,     0,   392,   393,   493,
     394,   395,   396,   397,   398,   399,   400,   401,   402,   403,
     404,   494,     0,   406,   495,     4,     5,     0,     0,     0,
     838,   839,     0,     0,     0,     0,     0,     0,     0,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,   840,
     841,    30,    31,   842,     0,    33,     0,     0,    34,    35,
      36,   843,   844,    38,    39,    40,    41,     0,    43,   845,
      45,   161,    47,   846,     0,     0,   847,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   848,
     849,   850,   851,   852,   853,   854,   855,   856,   857,   858,
     859,   860,   861,   862,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,   863,   864,   914,   915,     0,
       0,     0,   838,   839,     0,     0,     0,     0,   865,     0,
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   840,   841,   145,   916,     0,     0,    33,     0,     0,
      34,    35,    36,     0,   844,    38,    39,    40,     0,     0,
       0,   845,   160,   161,     0,     0,     0,     0,   847,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   848,   849,   850,   851,   852,   853,   854,   855,   856,
     857,   858,   859,   860,   861,   862,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     4,     5,     0,
       0,     0,   838,   839,     0,     0,     0,     0,     0,     0,
     865,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   840,   841,   145,    31,     0,     0,    33,     0,     0,
      34,    35,    36,     0,   844,    38,     0,    40,     0,     0,
       0,   845,   160,   161,     0,     0,     0,     0,   847,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   848,   849,   850,   851,   852,   853,   854,   855,   856,
     857,   858,   859,   860,   861,   862,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   142,   143,     0,
       0,     0,   980,   839,     0,     0,     0,     0,     0,     0,
     865,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   840,   841,   145,   146,     0,     0,    33,     0,     0,
       0,    35,    36,     0,   844,    38,     0,    40,     0,     0,
       0,   845,   160,   161,     0,     0,     0,     0,   847,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   848,   849,   850,   851,   852,   853,   854,   855,   856,
     857,   858,   859,   860,   861,   862,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   142,   143,     0,
       0,     0,     0,   144,     0,     0,     0,     0,     0,     0,
     865,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,   145,   146,     0,     0,    33,     0,     0,
       0,    35,    36,     0,   159,    38,     0,    40,     0,     0,
       0,   147,   160,   161,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   148,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   142,   143,     0,
       0,     0,     0,   839,     0,     0,     0,     0,     0,     0,
     911,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   840,   841,   145,   146,   318,     0,    33,     0,     0,
       0,    35,    36,     0,     0,     0,     0,    40,     0,     0,
       0,   845,   160,     0,     0,     0,     0,     0,   923,     0,
       0,     0,     0,   319,   320,     0,     0,     0,     0,     0,
     221,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   321,     0,     0,     0,     0,     0,     0,
       0,   848,   849,   850,   851,   852,   853,   854,   855,   856,
     857,   858,   859,   860,   861,   862,     0,     0,     0,     0,
     709,     0,   503,   504,   505,     0,   506,   507,     0,     0,
       0,   251,   252,   322,   323,   324,   123,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   247,   248,
     924,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   710,     0,
       0,     0,     0,     0,     0,     0,     0,   428,     0,     0,
       0,     0,     0,   222,     0,     0,   223,     0,     0,     0,
       0,     0,     0,     0,     0,   428,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   429,   430,     0,   711,   254,
     255,   123,   124,   125,   126,   127,   128,   129,   130,   131,
     132,   133,   134,   429,   430,   431,     0,     0,     0,     0,
       0,     0,    34,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   431,     0,     0,   712,     0,     0,     0,
       0,     0,   428,     0,     0,   713,   714,   715,     0,     0,
       0,     0,     0,     0,     0,   432,   433,   434,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     429,   430,   428,   432,   433,   434,   123,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,     0,     0,
     431,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     429,   430,     0,     0,   526,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     431,     0,   435,     0,     0,     0,     0,     0,     0,     0,
     432,   433,   434,   123,   124,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,     0,     0,   178,   663,   664,
     665,   666,   667,   668,   669,     0,     0,     0,     0,     0,
     432,   433,   434,   123,   124,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   179,   180,     0,     0,   692,
       0,     0,   178,   663,   664,   665,   666,   667,   668,   669,
       0,     0,     0,     0,     0,   183,     0,     0,   318,     0,
       0,     0,     0,   670,   671,   672,   673,     0,     0,   732,
     179,   180,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   319,   320,     0,     0,
     183,     0,     0,     0,     0,   184,   185,   186,   670,   671,
     672,   673,     0,     0,     0,     0,   321,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     184,   185,   186,   674,     0,   675,     0,     0,     0,     0,
       0,     0,   676,   677,   251,   252,   322,   323,   324,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,     0,     0,     0,     0,   142,   143,     0,   674,     0,
       0,   144,     0,     0,     0,     0,     0,   676,   677,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,   145,   146,     0,     0,    33,     0,     0,     0,    35,
      36,     0,     0,     0,     0,    40,     0,     0,     0,   147,
       0,     0,     0,     0,     0,     0,   116,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   318,     0,     0,
       0,     0,     0,     0,   117,   118,     0,     0,     0,   148,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,   119,   319,   320,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   321,     0,     0,     0,   310,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   120,   121,   122,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,     0,
       0,     0,     0,     0,     0,   322,   323,   324,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
       0,   965,     0,     0,   966,     0,     0,     0,   967,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     4,     5,
       0,     0,   965,     6,     7,   966,     0,     0,     0,   967,
       0,     0,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,     0,    33,     0,
       0,    34,    35,    36,     0,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,   116,    85,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   116,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   117,   118,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   117,   118,     0,     0,     0,   119,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   119,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   120,   121,   122,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   120,   121,   122,   123,   124,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   318,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   135,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   156,     0,     0,   319,   320,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   321,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   888,   889,   322,   323,   324,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     4,
       5,     0,     0,     0,     6,   144,     0,     0,     0,     0,
       0,     0,   890,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,   145,    31,     0,     0,    33,
       0,     0,    34,    35,    36,     0,   159,    38,     0,    40,
       0,     0,     0,   147,   160,   161,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   148,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,   142,   143,
       0,     0,     0,     0,   144,     0,   503,   504,   505,     0,
     506,   507,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,   145,   146,     0,     0,    33,     0,
       0,     0,    35,    36,     0,     0,     0,     0,    40,     0,
       0,     0,   147,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   148,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,   142,   143,     0,
       0,     0,     0,   144,     0,     0,     0,     0,     0,     0,
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,   145,   146,     0,     0,    33,     0,     0,
       0,    35,    36,     0,   159,    38,     0,    40,     0,     0,
       0,   147,   160,   161,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   148,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,   142,   143,     0,     0,
       0,     0,   144,     0,     0,     0,     0,     0,     0,     0,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,   145,   146,     0,     0,    33,     0,     0,     0,
      35,    36,     0,     0,     0,     0,    40,     0,     0,     0,
     147,   160,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     148,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,   142,   143,     0,     0,     0,
       0,   144,   194,     0,     0,     0,     0,     0,     0,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,   145,   146,     0,     0,    33,     0,     0,     0,    35,
      36,     0,     0,     0,     0,    40,     0,     0,     0,   147,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   148,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,     4,     5,     0,     0,     0,     0,
     144,     0,     0,     0,     0,     0,     0,     0,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,     0,     0,    33,     0,     0,     0,    35,    36,
       0,     0,     0,     0,    40,     0,     0,     0,   147,     0,
       0,   231,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   148,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,   142,   143,     0,     0,     0,     0,   144,
     295,     0,     0,     0,     0,     0,     0,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,   145,
     146,     0,     0,    33,     0,     0,     0,    35,    36,     0,
       0,     0,     0,    40,     0,     0,     0,   147,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   148,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,   142,   143,     0,     0,     0,     0,   144,     0,
       0,     0,     0,     0,     0,     0,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,   145,   146,
       0,     0,    33,     0,     0,     0,    35,    36,     0,     0,
       0,     0,    40,     0,     0,     0,   147,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   148,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,   142,   143,     0,     0,     0,     0,   144,     0,     0,
       0,     0,     0,     0,     0,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,   145,   146,     0,
       0,     0,     0,     0,     0,    35,    36,     0,     0,     0,
       0,    40,     0,     0,     0,   147,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   148,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
     142,   143,     0,     0,     0,     0,   279,     0,     0,     0,
       0,     0,     0,     0,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,   280,   281,   145,   146,     0,     0,
       0,     0,     0,     0,    35,    36,     0,     0,     0,     0,
       0,     0,     0,     0,   282,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   283,   284,   285,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,   529,
       0,     0,     0,     0,   279,     0,     0,     0,     0,     0,
       0,     0,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,   280,   281,   246,     0,     0,     0,     0,     0,
       0,     0,    35,    36,     0,     0,     0,     0,   530,     0,
       0,     0,   282,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   247,   248,     0,     0,     0,     0,     0,     0,
       0,   249,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   250,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   283,   284,   285,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,     0,     0,   318,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     251,   252,   253,   254,   255,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   319,   320,   318,
       0,     0,     0,     0,     0,     0,   249,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   321,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   319,   320,     0,
       0,     0,     0,     0,     0,     0,   249,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   321,     0,     0,
       0,     0,     0,     0,     0,   251,   252,   322,   323,   324,
     123,   124,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,     0,     0,   246,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   322,   323,   324,
     123,   124,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   247,   248,   116,     0,     0,     0,     0,     0,
       0,   249,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   250,     0,     0,     0,     0,     0,     0,   318,
       0,     0,   117,   118,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   119,     0,     0,     0,     0,   319,   320,     0,
       0,     0,   253,   254,   255,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   321,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   322,   323,   324,
     123,   124,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   365,   366,     0,     0,     0,     0,     0,   367,
     368,   369,   370,   371,   372,   373,   374,   375,   376,   377,
     378,   379,   380,   381,   382,   383,   384,   385,   386,   387,
     388,   389,   390,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   391,     0,     0,     0,   392,
     393,     0,   394,   395,   396,   397,   398,   399,   400,   401,
     402,   403,   404,   405,     0,   406
};

static const yytype_int16 yycheck[] =
{
       4,     5,    37,   149,     3,   278,   256,    90,    92,   300,
      43,   272,     4,     5,   413,    96,    97,    98,    99,   105,
      92,   294,    94,   684,    94,   526,    30,    31,   533,   616,
     757,   277,   758,    43,     3,    34,   623,   298,    30,    31,
     310,   802,   274,   289,   226,   521,   736,   293,     5,     6,
       7,   967,     0,   151,    44,   892,   288,   234,   156,    44,
     292,   593,   308,    44,     5,     6,     7,    94,    37,   535,
     302,   317,    94,    42,   951,   307,   139,   135,   176,   620,
      94,   139,    94,    43,   316,    45,   263,     5,     6,     7,
       9,   135,   190,   843,    94,    94,     6,  1013,   102,    94,
     198,    60,     5,     6,     7,    50,   766,     5,     6,     7,
     256,   948,     5,     6,     7,    60,    94,   139,    37,    38,
     134,   871,    94,   135,   159,   785,  1003,    37,    38,   759,
     760,   892,   364,   102,    93,   168,    46,   797,    57,   139,
     135,  1018,   240,   619,   134,    94,   232,   233,   181,   182,
     142,   143,   235,   145,   146,   236,   237,   147,   168,   691,
     150,   139,   147,   135,   630,   150,   147,   139,   350,   150,
     197,   181,   182,   135,    43,   139,   926,   263,    97,    98,
      99,   138,    92,   143,   135,   134,   135,   948,   135,   135,
     139,   692,    94,   103,    43,   193,   200,   138,   197,   135,
     204,   205,    94,   207,   208,   209,   210,   211,   212,   213,
     214,   215,   216,   217,   218,   219,   220,    94,   137,    94,
     138,   135,   226,   883,   756,   766,    94,   225,   232,   233,
     234,   732,   134,    94,   238,   138,   916,   769,   271,   272,
     138,    94,   134,   420,   785,   138,     3,     4,   135,   159,
     837,   161,   256,   163,   287,   945,   797,   135,   135,   263,
     135,   271,   272,   261,    94,   298,   134,   537,   134,    94,
     996,   137,   134,     9,   135,    94,   781,   287,   139,   135,
     142,   134,    94,    40,   194,  1012,   290,   256,   298,   969,
     970,   135,   972,   135,   881,   975,   828,   153,   885,   168,
     134,    37,    38,   272,   134,   135,   135,   135,   142,   139,
     135,   315,   181,   182,   139,   134,   135,    53,   134,    55,
     139,    57,   326,   327,   328,   134,   142,   331,   137,   298,
     139,   135,   141,   135,   134,    46,   135,   137,   843,   600,
     134,   141,   883,   137,   135,   139,   350,   141,   594,  1010,
     135,   135,     9,   357,   135,   135,     5,     6,     7,   135,
       9,    97,    98,    99,   865,   275,   627,   277,   139,   601,
     602,   603,   604,   605,   906,   555,    94,   137,   139,   289,
      37,    38,    50,   293,   135,   295,   484,   485,    37,    38,
      94,   140,   490,   491,    94,    94,   494,    94,   308,     9,
      57,   800,   271,   272,    94,   134,   134,   317,    57,   137,
     911,   139,   134,   141,   134,   134,   420,     9,   287,    94,
      94,    94,   143,   924,    94,   141,   699,    37,    38,   298,
     153,   435,   137,   706,    44,    71,    72,   154,   134,   137,
      97,    98,    99,   734,   136,    37,    38,    57,    97,    98,
      99,   724,   134,   142,   700,     9,   142,    11,    12,    13,
      14,    15,    16,   643,   644,    57,   142,   647,   648,   649,
     650,   651,   652,   653,   654,   655,   656,   142,   142,   512,
     142,   142,   142,    37,    38,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   512,    57,   142,    97,    98,    99,   144,   145,
     142,   147,   148,   149,   150,   151,   152,   521,   142,   142,
     142,   134,   526,   142,   137,   135,   139,   136,   141,   134,
     534,   535,   137,   136,   139,   142,   141,   147,   142,   134,
     150,   134,   153,    97,    98,    99,   134,   134,   134,   137,
     137,   139,   139,   141,   141,   588,   134,   153,    94,   136,
     136,   135,   742,    94,   568,   140,   137,   600,   140,   142,
     137,   140,   138,   143,   142,   608,   609,   610,   588,   140,
     137,   136,    94,   844,   140,    52,   136,   620,   140,   142,
     600,   138,   140,   142,   627,   136,   142,   135,   608,   609,
     610,    16,    16,   154,   141,   135,   138,   142,   136,   154,
     620,   153,   138,   874,   153,   619,   620,   627,   136,   142,
     139,    10,   136,   140,   137,   136,   630,   142,   878,   136,
     136,   136,   136,   136,   136,   136,   136,   136,    11,    11,
     136,   135,   135,   512,   142,   142,   136,   142,   137,   141,
     137,   142,   136,   136,   136,   135,   689,    50,   135,   135,
     135,   135,   912,   135,   135,   135,   154,   135,    43,   135,
     135,   704,   922,   135,   135,   135,   140,   909,   136,   689,
     136,   136,   136,   136,   594,   142,   136,   134,   692,   134,
     136,   135,   919,   134,   704,   138,   138,   781,   136,   136,
     134,   136,   704,   781,   781,   734,   908,   272,   833,   781,
     781,   781,   887,   233,  1010,   893,   827,   544,   106,   951,
     107,   902,   781,    -1,   757,   758,   759,   760,   732,   639,
      -1,   600,   982,   766,    -1,    -1,    -1,   835,    -1,   608,
     609,   610,   228,    -1,    -1,    -1,    -1,   757,   758,   759,
     760,   983,   785,    -1,    -1,    -1,   766,    -1,   627,    -1,
      -1,    -1,   766,    -1,   797,    -1,    -1,   913,   249,    -1,
    1002,  1003,    -1,   843,    -1,   785,    -1,    -1,    -1,    -1,
      -1,   785,   781,   866,    -1,   868,  1018,   797,    -1,    -1,
     700,   872,   873,   797,   875,    -1,    -1,    -1,   802,   871,
     833,   871,    -1,    -1,    -1,    -1,   892,    -1,    -1,   844,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     689,   731,    -1,   833,    -1,    -1,    -1,    -1,    -1,   739,
     928,    -1,    -1,    -1,    -1,   704,    -1,    -1,    -1,   920,
     921,    -1,    -1,    -1,   843,    -1,    -1,    -1,    -1,   932,
     883,    -1,   933,   934,    -1,    -1,    -1,     9,    -1,    -1,
      -1,   865,    -1,    -1,    -1,    -1,    -1,    -1,   966,    -1,
      -1,    -1,   871,   883,   878,    -1,    -1,    -1,    -1,   883,
      -1,    -1,    -1,    -1,    -1,    37,    38,    -1,   892,    -1,
      -1,    -1,    44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    57,    -1,   911,   912,    -1,
     914,   915,   916,    -1,    -1,    -1,    -1,    -1,   922,    -1,
     924,    -1,   914,   915,   916,    -1,    -1,   926,   838,    -1,
      -1,    -1,    -1,    -1,   844,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   948,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
      -1,    -1,    -1,   996,   833,    -1,    -1,    -1,    -1,   973,
     974,   975,    -1,    -1,    -1,    -1,    -1,    -1,   982,  1012,
      -1,    -1,    -1,    -1,    -1,    -1,   996,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   147,    -1,    -1,   150,    -1,
      -1,    -1,  1012,     3,     4,     5,     6,     7,     8,     9,
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
     110,   111,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   134,   135,    -1,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   152,   153,    -1,   155,   156,     3,     4,     5,
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
     106,   107,   108,   109,   110,   111,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   134,   135,
     136,   137,    -1,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   153,    -1,   155,
     156,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    -1,    50,    -1,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    -1,
      62,    63,    -1,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   134,   135,    -1,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   147,   148,   149,   150,   151,
     152,   153,    -1,   155,   156,     3,     4,     5,     6,     7,
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
     108,   109,   110,   111,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   134,   135,    -1,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,   153,    -1,   155,   156,     3,
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
     104,   105,   106,   107,   108,   109,   110,   111,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     134,   135,   136,   137,    -1,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   148,   149,   150,   151,   152,   153,
      -1,   155,   156,     3,     4,     5,     6,     7,     8,     9,
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
     110,   111,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   134,   135,   136,   137,    -1,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   152,   153,    -1,   155,   156,     3,     4,     5,
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
     106,   107,   108,   109,   110,   111,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   134,   135,
      -1,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   153,    -1,   155,
     156,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    -1,    50,    -1,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    -1,
      62,    63,    -1,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   134,   135,   136,   137,    -1,   139,   140,   141,
     142,   143,   144,   145,   146,   147,   148,   149,   150,   151,
     152,   153,    -1,   155,   156,     3,     4,     5,     6,     7,
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
     108,   109,   110,   111,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   134,   135,   136,   137,
      -1,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,   153,    -1,   155,   156,     3,
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
     104,   105,   106,   107,   108,   109,   110,   111,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     134,   135,   136,   137,    -1,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   148,   149,   150,   151,   152,   153,
      -1,   155,   156,     3,     4,     5,     6,     7,     8,     9,
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
     110,   111,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   134,   135,    -1,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   152,   153,    -1,   155,   156,     3,     4,     5,
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
     106,   107,   108,   109,   110,   111,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   134,   135,
      -1,   137,    -1,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    -1,    50,    -1,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    -1,
      62,    63,    -1,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   134,   135,   136,   137,    -1,   139,   140,   141,
     142,   143,   144,   145,   146,   147,   148,   149,   150,   151,
     152,   153,    -1,   155,   156,     3,     4,     5,     6,     7,
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
     108,   109,   110,   111,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   134,   135,   136,   137,
      -1,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,   153,    -1,   155,   156,     3,
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
     104,   105,   106,   107,   108,   109,   110,   111,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     134,   135,    -1,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   148,   149,   150,   151,   152,   153,
      -1,   155,   156,     3,     4,     5,     6,     7,     8,     9,
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
     110,   111,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   135,   136,   137,    -1,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   152,   153,    -1,   155,   156,     3,     4,     5,
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
     106,   107,   108,   109,   110,   111,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   134,   135,
      -1,   137,    -1,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   153,    -1,   155,
     156,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    -1,    50,    -1,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    -1,
      62,    63,    -1,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   134,   135,    -1,   137,    -1,   139,   140,   141,
     142,   143,   144,   145,   146,   147,   148,   149,   150,   151,
     152,   153,    -1,   155,   156,     3,     4,    -1,    -1,    -1,
       8,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    -1,    43,    -1,    -1,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    -1,    56,    57,
      58,    59,    60,    61,    -1,    -1,    64,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,     3,     4,    -1,
      -1,    -1,     8,     9,    -1,    -1,    -1,    -1,   146,    -1,
      -1,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    -1,    -1,    43,    -1,    -1,
      46,    47,    48,    -1,    50,    51,    52,    53,    -1,    -1,
      -1,    57,    58,    59,    -1,    -1,    -1,    -1,    64,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,    -1,
      -1,    -1,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,
     146,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    -1,    -1,    43,    -1,    -1,
      46,    47,    48,    -1,    50,    51,    -1,    53,    -1,    -1,
      -1,    57,    58,    59,    -1,    -1,    -1,    -1,    64,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,    -1,
      -1,    -1,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,
     146,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    -1,    -1,    43,    -1,    -1,
      -1,    47,    48,    -1,    50,    51,    -1,    53,    -1,    -1,
      -1,    57,    58,    59,    -1,    -1,    -1,    -1,    64,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,    -1,
      -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,
     146,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    -1,    -1,    43,    -1,    -1,
      -1,    47,    48,    -1,    50,    51,    -1,    53,    -1,    -1,
      -1,    57,    58,    59,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,    -1,
      -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,
     146,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,     9,    -1,    43,    -1,    -1,
      -1,    47,    48,    -1,    -1,    -1,    -1,    53,    -1,    -1,
      -1,    57,    58,    -1,    -1,    -1,    -1,    -1,    64,    -1,
      -1,    -1,    -1,    37,    38,    -1,    -1,    -1,    -1,    -1,
      44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    57,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,    -1,    -1,    -1,    -1,
       9,    -1,    11,    12,    13,    -1,    15,    16,    -1,    -1,
      -1,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,    37,    38,
     146,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    57,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     9,    -1,    -1,
      -1,    -1,    -1,   147,    -1,    -1,   150,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    37,    38,    -1,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,    37,    38,    57,    -1,    -1,    -1,    -1,
      -1,    -1,    46,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    57,    -1,    -1,   135,    -1,    -1,    -1,
      -1,    -1,     9,    -1,    -1,   144,   145,   146,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
      37,    38,     9,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,    -1,    -1,
      57,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      37,    38,    -1,    -1,   146,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      57,    -1,   146,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,    -1,    -1,     9,    10,    11,
      12,    13,    14,    15,    16,    -1,    -1,    -1,    -1,    -1,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,    37,    38,    -1,    -1,   146,
      -1,    -1,     9,    10,    11,    12,    13,    14,    15,    16,
      -1,    -1,    -1,    -1,    -1,    57,    -1,    -1,     9,    -1,
      -1,    -1,    -1,    65,    66,    67,    68,    -1,    -1,   146,
      37,    38,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    37,    38,    -1,    -1,
      57,    -1,    -1,    -1,    -1,    97,    98,    99,    65,    66,
      67,    68,    -1,    -1,    -1,    -1,    57,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      97,    98,    99,   135,    -1,   137,    -1,    -1,    -1,    -1,
      -1,    -1,   144,   145,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,    -1,    -1,    -1,    -1,     3,     4,    -1,   135,    -1,
      -1,     9,    -1,    -1,    -1,    -1,    -1,   144,   145,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    -1,    -1,    43,    -1,    -1,    -1,    47,
      48,    -1,    -1,    -1,    -1,    53,    -1,    -1,    -1,    57,
      -1,    -1,    -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    37,    38,    -1,    -1,    -1,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,    57,    37,    38,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    57,    -1,    -1,    -1,   137,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,    -1,
      -1,    -1,    -1,    -1,    -1,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
      -1,   134,    -1,    -1,   137,    -1,    -1,    -1,   141,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,
      -1,    -1,   134,     8,     9,   137,    -1,    -1,    -1,   141,
      -1,    -1,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    -1,    43,    -1,
      -1,    46,    47,    48,    -1,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,     9,   134,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    37,    38,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    37,    38,    -1,    -1,    -1,    57,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    57,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   137,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   137,    -1,    -1,    37,    38,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    57,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,
       4,    -1,    -1,    -1,     8,     9,    -1,    -1,    -1,    -1,
      -1,    -1,   135,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    -1,    -1,    43,
      -1,    -1,    46,    47,    48,    -1,    50,    51,    -1,    53,
      -1,    -1,    -1,    57,    58,    59,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,     3,     4,
      -1,    -1,    -1,    -1,     9,    -1,    11,    12,    13,    -1,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    -1,    -1,    43,    -1,
      -1,    -1,    47,    48,    -1,    -1,    -1,    -1,    53,    -1,
      -1,    -1,    57,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,     3,     4,    -1,
      -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    -1,    -1,    43,    -1,    -1,
      -1,    47,    48,    -1,    50,    51,    -1,    53,    -1,    -1,
      -1,    57,    58,    59,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,     3,     4,    -1,    -1,
      -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    -1,    -1,    43,    -1,    -1,    -1,
      47,    48,    -1,    -1,    -1,    -1,    53,    -1,    -1,    -1,
      57,    58,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,     3,     4,    -1,    -1,    -1,
      -1,     9,    10,    -1,    -1,    -1,    -1,    -1,    -1,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    -1,    -1,    43,    -1,    -1,    -1,    47,
      48,    -1,    -1,    -1,    -1,    53,    -1,    -1,    -1,    57,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,     3,     4,    -1,    -1,    -1,    -1,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    -1,    -1,    43,    -1,    -1,    -1,    47,    48,
      -1,    -1,    -1,    -1,    53,    -1,    -1,    -1,    57,    -1,
      -1,    60,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,     3,     4,    -1,    -1,    -1,    -1,     9,
      10,    -1,    -1,    -1,    -1,    -1,    -1,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    -1,    -1,    43,    -1,    -1,    -1,    47,    48,    -1,
      -1,    -1,    -1,    53,    -1,    -1,    -1,    57,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,     3,     4,    -1,    -1,    -1,    -1,     9,    -1,
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
     111,     3,     4,    -1,    -1,    -1,    -1,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    -1,
      -1,    -1,    -1,    -1,    -1,    47,    48,    -1,    -1,    -1,
      -1,    53,    -1,    -1,    -1,    57,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
       3,     4,    -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    -1,    -1,
      -1,    -1,    -1,    -1,    47,    48,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    57,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,     4,
      -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,     9,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    47,    48,    -1,    -1,    -1,    -1,    53,    -1,
      -1,    -1,    57,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    37,    38,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    46,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    57,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,    -1,    -1,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,    37,    38,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    46,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    57,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    37,    38,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    46,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    57,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,    -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,    37,    38,     9,    -1,    -1,    -1,    -1,    -1,
      -1,    46,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    57,    -1,    -1,    -1,    -1,    -1,    -1,     9,
      -1,    -1,    37,    38,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    57,    -1,    -1,    -1,    -1,    37,    38,    -1,
      -1,    -1,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,    57,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,    62,    63,    -1,    -1,    -1,    -1,    -1,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   135,    -1,    -1,    -1,   139,
     140,    -1,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   152,   153,    -1,   155
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   158,     0,   159,     3,     4,     8,     9,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    43,    46,    47,    48,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   134,   160,   161,   162,   164,
     178,   194,   195,   196,   197,   206,   208,   209,   213,   214,
     215,   252,   283,   284,   285,   286,   287,   288,   290,   298,
     299,   303,   304,   305,   306,   321,     9,    37,    38,    57,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   137,   280,   281,   282,   305,
     280,   305,     3,     4,     9,    39,    40,    57,    97,   285,
     299,   135,   139,   180,   280,   305,   137,   280,   305,    50,
      58,    59,   283,   284,   285,   299,   285,   139,   289,     9,
      37,    38,    57,    97,    98,    99,   137,   299,     9,    37,
      38,    53,    55,    57,    97,    98,    99,   290,   297,   298,
     135,   139,    50,    60,    10,   285,   255,   135,   135,   139,
     135,   135,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   135,   135,   135,   135,   135,   135,
     135,    44,   147,   150,   254,   257,   258,   300,   301,   302,
     254,    60,   164,   178,   285,   164,   208,   214,   283,   134,
     137,   226,   226,   226,   226,   226,     9,    37,    38,    46,
      57,    95,    96,    97,    98,    99,   210,   218,   220,   223,
     227,   253,   262,   265,   272,   280,   299,   285,   300,   288,
     287,    94,    94,   134,   346,   139,   167,   139,   165,     9,
      37,    38,    57,    97,    98,    99,   218,    94,   346,   292,
     137,   179,   346,   139,   169,    10,   135,   285,    94,   140,
     198,   297,   346,   163,   297,   297,   134,   346,   291,   255,
     137,   285,   134,   142,   208,   283,   346,   293,     9,    37,
      38,    57,    97,    98,    99,   280,   323,   326,   327,   280,
     280,   331,   280,   280,   280,   280,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   134,   255,   253,   280,
     259,   150,   302,   134,   134,   248,   280,   300,   248,   262,
     254,   226,   226,   280,   346,    62,    63,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,   135,   139,   140,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,   153,   155,   344,   345,   218,
     223,   299,   219,   224,   255,   143,   249,   250,   262,   270,
     300,   273,   274,   275,   135,   139,    94,   297,     9,    37,
      38,    57,    97,    98,    99,   146,   215,   231,   233,   280,
     297,   299,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    50,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    62,    63,    65,    66,
      67,    68,    93,    94,    95,    96,    97,    98,    99,   134,
     135,   137,   138,   141,   153,   156,   306,   345,   348,   349,
     350,   351,   352,    11,    12,    13,    15,    16,   184,   285,
     294,   296,   141,   174,   294,   174,   136,   294,   181,   182,
     280,   137,   138,   294,   174,   216,   146,   231,   299,     4,
      53,   199,   201,   202,   203,   304,   138,   137,   134,   134,
     136,   294,   134,   158,   256,   136,   136,   294,   142,   280,
     280,   280,   142,   142,   280,   142,   142,   142,   142,   142,
     142,   142,   142,   142,   142,   142,   136,   136,   142,   253,
     134,   280,   134,   134,   134,   138,   153,   153,   136,   154,
     221,    94,    43,    45,   143,   225,   225,   134,   251,   136,
     262,   276,   278,   228,   229,   238,   239,   280,   232,   135,
      94,   346,   346,   346,   346,   346,   140,   142,     5,     6,
       7,   175,   176,   297,   137,   140,   137,   140,   138,   142,
     143,   181,   140,   137,   241,   242,   238,    94,   140,   142,
     197,   204,   280,   204,   158,   140,   138,   257,   140,   322,
     142,   136,   136,   328,   330,   142,   303,   334,   336,   338,
     340,   335,   337,   339,   341,   342,   343,   280,   154,   154,
     135,    16,    16,    10,    11,    12,    13,    14,    15,    16,
      65,    66,    67,    68,   135,   137,   144,   145,   297,   307,
     312,   318,   319,   320,   263,   153,   241,   294,   135,   141,
     235,   234,   146,   231,   136,   136,   136,   138,   154,   168,
     295,   297,   297,   297,   142,   171,   166,   171,   181,     9,
      57,    97,   135,   144,   145,   146,   183,   184,   185,   189,
     280,   290,   298,   138,   170,   171,   136,    60,    93,   243,
     245,   246,   146,   231,   200,   204,   205,   138,   285,   324,
     303,   303,   332,   136,   303,   303,   303,   303,   303,   303,
     303,   303,   303,   303,   142,   309,   222,   315,   308,   314,
     313,   139,    10,   135,   267,   274,   277,   136,   140,   240,
     237,   297,   241,   238,   174,   294,   175,     5,     6,     7,
     138,   172,   177,   174,   138,   188,    71,    72,   144,   145,
     147,   148,   149,   150,   151,   152,   190,   186,   174,   138,
     217,   142,   285,   238,   199,   249,   136,   285,   329,   136,
     303,   136,   136,   136,   136,   136,   136,   136,   136,   142,
     142,   136,   241,   312,   307,   320,   320,   316,   268,   185,
     279,   135,   241,   142,   236,   135,   136,   137,     8,     9,
      37,    38,    41,    49,    50,    57,    61,    64,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   133,   134,   146,   164,   173,   178,   194,
     196,   197,   207,   211,   212,   214,   231,   252,   283,   321,
     141,   137,   185,   187,   185,   137,   225,   244,    95,    96,
     135,   260,   264,   271,   272,   325,   142,   333,    11,    11,
     136,   136,   310,   286,   241,   154,   230,   136,   237,   346,
     171,   146,   283,   285,     3,     4,    40,   191,   192,   197,
     211,   214,   283,    64,   146,   231,   135,    50,   135,   238,
     254,   254,   164,   211,   214,   226,   226,   231,   226,   223,
     171,   136,   185,   171,   243,   247,   260,   269,   300,   273,
     136,   347,   136,   136,   136,   142,   309,   140,   136,   241,
     236,   136,   138,   238,   223,   134,   137,   141,   193,   280,
     280,   193,   280,     3,     4,    40,   192,   226,   226,   238,
       8,   211,   283,   346,   134,   134,   254,   226,   226,   138,
     138,   249,   136,   260,   136,   349,   311,   138,   135,    43,
     266,   136,   346,   347,   193,   193,   193,   136,   136,   134,
     261,   307,   317,   138,   134,   134,   267,   312,   347,   136,
     134
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
#line 1333 "vtkParse.y"
    { startSig(); clearTypeId(); clearTemplate(); closeComment(); }
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 1346 "vtkParse.y"
    { output_function(); }
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 1347 "vtkParse.y"
    { output_function(); }
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 1348 "vtkParse.y"
    { reject_function(); }
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 1349 "vtkParse.y"
    { output_function(); }
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 1350 "vtkParse.y"
    { reject_function(); }
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 1351 "vtkParse.y"
    { output_function(); }
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 1352 "vtkParse.y"
    { legacySig(); output_function(); }
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 1370 "vtkParse.y"
    { pushNamespace((yyvsp[(2) - (2)].str)); }
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 1371 "vtkParse.y"
    { popNamespace(); }
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 1378 "vtkParse.y"
    { start_class((yyvsp[(2) - (2)].str), 0); }
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 1379 "vtkParse.y"
    { end_class(); }
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 1380 "vtkParse.y"
    { reject_class((yyvsp[(2) - (5)].str), 0); }
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 1381 "vtkParse.y"
    { end_class(); }
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 1382 "vtkParse.y"
    { start_class((yyvsp[(2) - (2)].str), 1); }
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 1383 "vtkParse.y"
    { end_class(); }
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 1384 "vtkParse.y"
    { reject_class((yyvsp[(2) - (5)].str), 1); }
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 1385 "vtkParse.y"
    { end_class(); }
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 1387 "vtkParse.y"
    { start_class((yyvsp[(2) - (2)].str), 2); }
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 1388 "vtkParse.y"
    { end_class(); }
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 1389 "vtkParse.y"
    { reject_class((yyvsp[(2) - (5)].str), 2); }
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 1390 "vtkParse.y"
    { end_class(); }
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 1395 "vtkParse.y"
    { startSig(); clearTypeId(); clearTemplate();  closeComment(); }
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 1409 "vtkParse.y"
    { output_function(); }
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 1410 "vtkParse.y"
    { ClassInfo *tmpc = currentClass;
     currentClass = NULL; output_function(); currentClass = tmpc; }
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 1412 "vtkParse.y"
    { output_function(); }
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 1413 "vtkParse.y"
    { output_function(); }
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 1414 "vtkParse.y"
    { ClassInfo *tmpc = currentClass;
     currentClass = NULL; output_function(); currentClass = tmpc; }
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 1416 "vtkParse.y"
    { output_function(); }
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 1417 "vtkParse.y"
    { legacySig(); output_function(); }
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 1430 "vtkParse.y"
    {
      vtkParse_AddStringToArray(&currentClass->SuperClasses,
                                &currentClass->NumberOfSuperClasses,
                                vtkstrdup((yyvsp[(2) - (2)].str)));
    }
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 1436 "vtkParse.y"
    {access_level = VTK_ACCESS_PUBLIC;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 1437 "vtkParse.y"
    {access_level = VTK_ACCESS_PRIVATE;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 1438 "vtkParse.y"
    {access_level = VTK_ACCESS_PROTECTED;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 1448 "vtkParse.y"
    {start_enum((yyvsp[(2) - (2)].str));}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 1449 "vtkParse.y"
    {end_enum();}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 1450 "vtkParse.y"
    {start_enum(NULL);}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 1451 "vtkParse.y"
    {end_enum();}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 1455 "vtkParse.y"
    {add_enum((yyvsp[(1) - (1)].str), NULL);}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 1456 "vtkParse.y"
    {add_enum((yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 1458 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 1459 "vtkParse.y"
    {(yyval.str) = vtkstrdup(add_const_scope((yyvsp[(1) - (1)].str)));}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 1464 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 1465 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 1466 "vtkParse.y"
    {
         (yyval.str) = vtkstrcat((yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));
       }
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 1469 "vtkParse.y"
    {postSig((yyvsp[(2) - (2)].str));}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 1470 "vtkParse.y"
    {
         (yyval.str) = vtkstrcat5((yyvsp[(1) - (4)].str), " ", (yyvsp[(2) - (4)].str), " ", (yyvsp[(4) - (4)].str));
       }
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 1473 "vtkParse.y"
    {postSig("(");}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 1474 "vtkParse.y"
    {
         (yyval.str) = vtkstrcat3("(", (yyvsp[(3) - (4)].str), ")");
       }
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 1478 "vtkParse.y"
    { (yyval.str) = "-"; }
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 1478 "vtkParse.y"
    { (yyval.str) = "+"; }
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 1479 "vtkParse.y"
    { (yyval.str) = "~"; }
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 1481 "vtkParse.y"
    { (yyval.str) = "-"; }
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 1481 "vtkParse.y"
    { (yyval.str) = "+"; }
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 1482 "vtkParse.y"
    { (yyval.str) = "*"; }
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 1482 "vtkParse.y"
    { (yyval.str) = "/"; }
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 1483 "vtkParse.y"
    { (yyval.str) = "%"; }
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 1483 "vtkParse.y"
    { (yyval.str) = "&"; }
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 1484 "vtkParse.y"
    { (yyval.str) = "|"; }
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 1484 "vtkParse.y"
    { (yyval.str) = "^"; }
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 1485 "vtkParse.y"
    { (yyval.str) = ">>"; }
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 1486 "vtkParse.y"
    { (yyval.str) = "<<"; }
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1509 "vtkParse.y"
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

  case 127:

/* Line 1455 of yacc.c  */
#line 1531 "vtkParse.y"
    { }
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1532 "vtkParse.y"
    { }
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1533 "vtkParse.y"
    { }
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1535 "vtkParse.y"
    { }
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1542 "vtkParse.y"
    { add_using((yyvsp[(3) - (4)].str), 1); }
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1543 "vtkParse.y"
    { add_using((yyvsp[(3) - (4)].str), 0); }
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1544 "vtkParse.y"
    { add_using((yyvsp[(2) - (3)].str), 0); }
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1551 "vtkParse.y"
    { postSig("template<> "); clearTypeId(); }
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1552 "vtkParse.y"
    { postSig("template<");
          clearTypeId(); startTemplate(); }
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1554 "vtkParse.y"
    { chopSig();
            if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
            postSig("> "); clearTypeId(); }
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1559 "vtkParse.y"
    { chopSig(); postSig(", "); clearTypeId(); }
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1563 "vtkParse.y"
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

  case 141:

/* Line 1455 of yacc.c  */
#line 1573 "vtkParse.y"
    {
               TemplateArg *arg = (TemplateArg *)malloc(sizeof(TemplateArg));
               vtkParse_InitTemplateArg(arg);
               arg->Name = vtkstrdup(getVarName());
               arg->Value = vtkstrdup(getVarValue());
               vtkParse_AddArgumentToTemplate(currentTemplate, arg);
               }
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 1580 "vtkParse.y"
    { pushTemplate(); }
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 1581 "vtkParse.y"
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

  case 144:

/* Line 1455 of yacc.c  */
#line 1592 "vtkParse.y"
    {postSig("class ");}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 1593 "vtkParse.y"
    {postSig("typename ");}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 1595 "vtkParse.y"
    { setVarName((yyvsp[(1) - (1)].str)); }
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1625 "vtkParse.y"
    {openSig(); preSig("~"); closeSig();}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 1626 "vtkParse.y"
    {openSig(); preSig("~"); closeSig();}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 1628 "vtkParse.y"
    {
         openSig(); preSig("virtual ~"); closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1635 "vtkParse.y"
    {
         openSig();
         preSig("explicit ");
         closeSig();
         currentFunction->IsExplicit = 1;
         }
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 1643 "vtkParse.y"
    {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1659 "vtkParse.y"
    {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1668 "vtkParse.y"
    {
      postSig("(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 1672 "vtkParse.y"
    { postSig(")"); }
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 1673 "vtkParse.y"
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

  case 182:

/* Line 1455 of yacc.c  */
#line 1684 "vtkParse.y"
    { postSig(")"); }
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 1685 "vtkParse.y"
    {
      postSig(";");
      closeSig();
      currentFunction->Name = vtkstrdup((yyvsp[(1) - (3)].str));
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed operator", currentFunction->Name);
    }
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 1693 "vtkParse.y"
    {postSig((yyvsp[(2) - (2)].str));}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 1694 "vtkParse.y"
    {
      postSig("(");
      currentFunction->IsOperator = 1;
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 1699 "vtkParse.y"
    { (yyval.str) = (yyvsp[(2) - (7)].str); }
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 1701 "vtkParse.y"
    { postSig(")"); }
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 1702 "vtkParse.y"
    {
      postSig(";");
      closeSig();
      currentFunction->Name = vtkstrdup((yyvsp[(1) - (3)].str));
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 1712 "vtkParse.y"
    {
      postSig(" = 0");
      if (currentClass)
        {
        currentFunction->IsPureVirtual = 1;
        currentClass->IsAbstract = 1;
        }
    }
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 1721 "vtkParse.y"
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

  case 192:

/* Line 1455 of yacc.c  */
#line 1731 "vtkParse.y"
    {
      postSig(" const");
      currentFunction->IsConst = 1;
    }
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 1739 "vtkParse.y"
    {
      postSig("(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 1742 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (5)].str); }
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 1743 "vtkParse.y"
    {markSig(); postSig("<");}
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 1744 "vtkParse.y"
    {
      if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
      (yyval.str) = vtkstrcat((yyvsp[(1) - (6)].str), copySig());
    }
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 1749 "vtkParse.y"
    { (yyval.str) = (yyvsp[(7) - (9)].str); }
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 1751 "vtkParse.y"
    { postSig(");"); closeSig(); }
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 1752 "vtkParse.y"
    {
      currentFunction->Name = vtkstrdup((yyvsp[(1) - (3)].str));
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 1758 "vtkParse.y"
    { postSig("("); }
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 1767 "vtkParse.y"
    {
      postSig(");");
      closeSig();
      currentFunction->Name = vtkstrcat("~", (yyvsp[(1) - (1)].str));
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 1775 "vtkParse.y"
    { postSig("(");}
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 1781 "vtkParse.y"
    {clearTypeId();}
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 1784 "vtkParse.y"
    { currentFunction->IsVariadic = 1; postSig("..."); }
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 1785 "vtkParse.y"
    { clearTypeId(); }
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 1786 "vtkParse.y"
    { clearTypeId(); postSig(", "); }
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 1789 "vtkParse.y"
    { markSig(); }
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 1791 "vtkParse.y"
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

  case 221:

/* Line 1455 of yacc.c  */
#line 1813 "vtkParse.y"
    {
      int i = currentFunction->NumberOfArguments-1;
      if (getVarValue())
        {
        currentFunction->Arguments[i]->Value = vtkstrdup(getVarValue());
        }
    }
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 1821 "vtkParse.y"
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

  case 225:

/* Line 1455 of yacc.c  */
#line 1846 "vtkParse.y"
    {clearVarValue();}
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 1848 "vtkParse.y"
    { postSig("="); clearVarValue();}
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 1849 "vtkParse.y"
    { setVarValue((yyvsp[(3) - (3)].str)); }
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 1860 "vtkParse.y"
    {
       unsigned int type = getStorageType();
       ValueInfo *var = (ValueInfo *)malloc(sizeof(ValueInfo));
       vtkParse_InitValue(var);
       var->ItemType = VTK_VARIABLE_INFO;
       var->Access = access_level;

       handle_complex_type(var, type, (yyvsp[(1) - (2)].integer), getSig());

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
     }
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 1914 "vtkParse.y"
    {postSig(", ");}
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 1917 "vtkParse.y"
    { setStorageTypeIndirection(0); }
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 1918 "vtkParse.y"
    { setStorageTypeIndirection((yyvsp[(1) - (1)].integer)); }
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 1922 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 1923 "vtkParse.y"
    { postSig(")"); }
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 1925 "vtkParse.y"
    {
         const char *scope = getScope();
         unsigned int parens = add_indirection((yyvsp[(1) - (5)].integer), (yyvsp[(2) - (5)].integer));
         if ((yyvsp[(5) - (5)].integer) == VTK_PARSE_FUNCTION) {
           if (scope) { scope = vtkstrndup(scope, strlen(scope) - 2); }
           getFunction()->Class = scope;
           (yyval.integer) = (parens | VTK_PARSE_FUNCTION); }
         else if ((yyvsp[(5) - (5)].integer) == VTK_PARSE_ARRAY) {
           (yyval.integer) = add_indirection_to_array(parens); }
       }
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 1937 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 1938 "vtkParse.y"
    { postSig(")"); }
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 1940 "vtkParse.y"
    {
         const char *scope = getScope();
         unsigned int parens = add_indirection((yyvsp[(1) - (5)].integer), (yyvsp[(2) - (5)].integer));
         if ((yyvsp[(5) - (5)].integer) == VTK_PARSE_FUNCTION) {
           if (scope) { scope = vtkstrndup(scope, strlen(scope) - 2); }
           getFunction()->Class = scope;
           (yyval.integer) = (parens | VTK_PARSE_FUNCTION); }
         else if ((yyvsp[(5) - (5)].integer) == VTK_PARSE_ARRAY) {
           (yyval.integer) = add_indirection_to_array(parens); }
       }
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 1951 "vtkParse.y"
    { postSig("("); scopeSig(""); (yyval.integer) = 0; }
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 1952 "vtkParse.y"
    { postSig("("); scopeSig((yyvsp[(1) - (1)].str)); postSig("*");
               (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 1954 "vtkParse.y"
    { postSig("("); scopeSig((yyvsp[(1) - (1)].str)); postSig("&");
               (yyval.integer) = VTK_PARSE_REF; }
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 1957 "vtkParse.y"
    { postSig("("); scopeSig((yyvsp[(1) - (1)].str)); postSig("*");
               (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 1959 "vtkParse.y"
    { postSig("("); scopeSig((yyvsp[(1) - (1)].str)); postSig("&");
               (yyval.integer) = VTK_PARSE_REF; }
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 1962 "vtkParse.y"
    { currentFunction->IsConst = 1; }
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 1964 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 1965 "vtkParse.y"
    { pushFunction(); postSig("("); }
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 1966 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_FUNCTION; postSig(")"); popFunction(); }
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 1967 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_ARRAY; }
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 1970 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 1972 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(1) - (2)].integer), (yyvsp[(2) - (2)].integer));}
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 1975 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 1977 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(1) - (2)].integer), (yyvsp[(2) - (2)].integer));}
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 1979 "vtkParse.y"
    {clearVarName(); chopSig();}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 1981 "vtkParse.y"
    {setVarName((yyvsp[(1) - (1)].str));}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 1983 "vtkParse.y"
    {clearArray();}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 1985 "vtkParse.y"
    {clearArray();}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 1987 "vtkParse.y"
    {postSig("[");}
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 1987 "vtkParse.y"
    {postSig("]");}
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 1991 "vtkParse.y"
    {pushArraySize("");}
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 1992 "vtkParse.y"
    {pushArraySize((yyvsp[(1) - (1)].str));}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 1998 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 1999 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 2000 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 2001 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 2002 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 2003 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 2004 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 2005 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 2006 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 2009 "vtkParse.y"
    {(yyval.str) = "vtkTypeInt8";}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 2010 "vtkParse.y"
    {(yyval.str) = "vtkTypeUInt8";}
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 2011 "vtkParse.y"
    {(yyval.str) = "vtkTypeInt16";}
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 2012 "vtkParse.y"
    {(yyval.str) = "vtkTypeUInt16";}
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 2013 "vtkParse.y"
    {(yyval.str) = "vtkTypeInt32";}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 2014 "vtkParse.y"
    {(yyval.str) = "vtkTypeUInt32";}
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 2015 "vtkParse.y"
    {(yyval.str) = "vtkTypeInt64";}
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 2016 "vtkParse.y"
    {(yyval.str) = "vtkTypeUInt64";}
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 2017 "vtkParse.y"
    {(yyval.str) = "vtkTypeFloat32";}
    break;

  case 294:

/* Line 1455 of yacc.c  */
#line 2018 "vtkParse.y"
    {(yyval.str) = "vtkTypeFloat64";}
    break;

  case 295:

/* Line 1455 of yacc.c  */
#line 2021 "vtkParse.y"
    {(yyval.str) = "vtkIdType";}
    break;

  case 296:

/* Line 1455 of yacc.c  */
#line 2022 "vtkParse.y"
    {(yyval.str) = "vtkFloatingPointType";}
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 2029 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer); setStorageType((yyval.integer));}
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 2030 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 2031 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 2033 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(3) - (3)].integer); setStorageType((yyval.integer));}
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 2034 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 302:

/* Line 1455 of yacc.c  */
#line 2035 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer));
      setStorageType((yyval.integer));}
    break;

  case 303:

/* Line 1455 of yacc.c  */
#line 2037 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(2) - (3)].integer) | (yyvsp[(3) - (3)].integer));
      setStorageType((yyval.integer));}
    break;

  case 304:

/* Line 1455 of yacc.c  */
#line 2041 "vtkParse.y"
    {postSig("static "); (yyval.integer) = VTK_PARSE_STATIC; }
    break;

  case 305:

/* Line 1455 of yacc.c  */
#line 2042 "vtkParse.y"
    {postSig("static "); (yyval.integer) = VTK_PARSE_STATIC; }
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 2044 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 307:

/* Line 1455 of yacc.c  */
#line 2045 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer));}
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 2047 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 309:

/* Line 1455 of yacc.c  */
#line 2048 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_CONST | (yyvsp[(2) - (2)].integer));}
    break;

  case 310:

/* Line 1455 of yacc.c  */
#line 2049 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_CONST | (yyvsp[(1) - (2)].integer));}
    break;

  case 311:

/* Line 1455 of yacc.c  */
#line 2051 "vtkParse.y"
    {postSig("const ");}
    break;

  case 313:

/* Line 1455 of yacc.c  */
#line 2055 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 314:

/* Line 1455 of yacc.c  */
#line 2057 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 315:

/* Line 1455 of yacc.c  */
#line 2058 "vtkParse.y"
    {postSig("typename ");}
    break;

  case 316:

/* Line 1455 of yacc.c  */
#line 2059 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(3) - (3)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 317:

/* Line 1455 of yacc.c  */
#line 2062 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 318:

/* Line 1455 of yacc.c  */
#line 2063 "vtkParse.y"
    {chopSig(); if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 2065 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 2066 "vtkParse.y"
    {chopSig(); if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 321:

/* Line 1455 of yacc.c  */
#line 2068 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 322:

/* Line 1455 of yacc.c  */
#line 2069 "vtkParse.y"
    {chopSig(); if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 324:

/* Line 1455 of yacc.c  */
#line 2073 "vtkParse.y"
    {chopSig(); postSig(", ");}
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 2076 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 2078 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 329:

/* Line 1455 of yacc.c  */
#line 2079 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 2080 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 331:

/* Line 1455 of yacc.c  */
#line 2081 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 332:

/* Line 1455 of yacc.c  */
#line 2082 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 333:

/* Line 1455 of yacc.c  */
#line 2083 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 2084 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 337:

/* Line 1455 of yacc.c  */
#line 2089 "vtkParse.y"
    {
             (yyval.str) = vtkstrcat3((yyvsp[(1) - (3)].str), "::", (yyvsp[(3) - (3)].str));
             preScopeSig((yyvsp[(1) - (3)].str));
           }
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 2094 "vtkParse.y"
    {
             (yyval.str) = vtkstrcat3((yyvsp[(1) - (3)].str), "::", (yyvsp[(3) - (3)].str));
             preScopeSig("");
           }
    break;

  case 346:

/* Line 1455 of yacc.c  */
#line 2116 "vtkParse.y"
    { postSig("&"); (yyval.integer) = VTK_PARSE_REF;}
    break;

  case 347:

/* Line 1455 of yacc.c  */
#line 2117 "vtkParse.y"
    { postSig("&"); (yyval.integer) = ((yyvsp[(1) - (2)].integer) | VTK_PARSE_REF);}
    break;

  case 348:

/* Line 1455 of yacc.c  */
#line 2118 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 349:

/* Line 1455 of yacc.c  */
#line 2123 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 350:

/* Line 1455 of yacc.c  */
#line 2125 "vtkParse.y"
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

  case 351:

/* Line 1455 of yacc.c  */
#line 2136 "vtkParse.y"
    { postSig("*"); (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 352:

/* Line 1455 of yacc.c  */
#line 2137 "vtkParse.y"
    { postSig("*const "); (yyval.integer) = VTK_PARSE_CONST_POINTER; }
    break;

  case 353:

/* Line 1455 of yacc.c  */
#line 2140 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 354:

/* Line 1455 of yacc.c  */
#line 2141 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); }
    break;

  case 355:

/* Line 1455 of yacc.c  */
#line 2142 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); }
    break;

  case 356:

/* Line 1455 of yacc.c  */
#line 2143 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 357:

/* Line 1455 of yacc.c  */
#line 2144 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 358:

/* Line 1455 of yacc.c  */
#line 2147 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 359:

/* Line 1455 of yacc.c  */
#line 2148 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 360:

/* Line 1455 of yacc.c  */
#line 2151 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_STRING;}
    break;

  case 361:

/* Line 1455 of yacc.c  */
#line 2152 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNICODE_STRING;}
    break;

  case 362:

/* Line 1455 of yacc.c  */
#line 2153 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_OSTREAM; }
    break;

  case 363:

/* Line 1455 of yacc.c  */
#line 2154 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_ISTREAM; }
    break;

  case 364:

/* Line 1455 of yacc.c  */
#line 2155 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 365:

/* Line 1455 of yacc.c  */
#line 2156 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_OBJECT; }
    break;

  case 366:

/* Line 1455 of yacc.c  */
#line 2157 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_QOBJECT; }
    break;

  case 367:

/* Line 1455 of yacc.c  */
#line 2160 "vtkParse.y"
    { typeSig("void"); (yyval.integer) = VTK_PARSE_VOID;}
    break;

  case 368:

/* Line 1455 of yacc.c  */
#line 2161 "vtkParse.y"
    { typeSig("bool"); (yyval.integer) = VTK_PARSE_BOOL;}
    break;

  case 369:

/* Line 1455 of yacc.c  */
#line 2162 "vtkParse.y"
    { typeSig("ssize_t"); (yyval.integer) = VTK_PARSE_SSIZE_T;}
    break;

  case 370:

/* Line 1455 of yacc.c  */
#line 2163 "vtkParse.y"
    { typeSig("size_t"); (yyval.integer) = VTK_PARSE_SIZE_T;}
    break;

  case 371:

/* Line 1455 of yacc.c  */
#line 2164 "vtkParse.y"
    { typeSig("vtkTypeInt8"); (yyval.integer) = VTK_PARSE_INT8; }
    break;

  case 372:

/* Line 1455 of yacc.c  */
#line 2165 "vtkParse.y"
    { typeSig("vtkTypeUInt8"); (yyval.integer) = VTK_PARSE_UINT8; }
    break;

  case 373:

/* Line 1455 of yacc.c  */
#line 2166 "vtkParse.y"
    { typeSig("vtkTypeInt16"); (yyval.integer) = VTK_PARSE_INT16; }
    break;

  case 374:

/* Line 1455 of yacc.c  */
#line 2167 "vtkParse.y"
    { typeSig("vtkTypeUInt16"); (yyval.integer) = VTK_PARSE_UINT16; }
    break;

  case 375:

/* Line 1455 of yacc.c  */
#line 2168 "vtkParse.y"
    { typeSig("vtkTypeInt32"); (yyval.integer) = VTK_PARSE_INT32; }
    break;

  case 376:

/* Line 1455 of yacc.c  */
#line 2169 "vtkParse.y"
    { typeSig("vtkTypeUInt32"); (yyval.integer) = VTK_PARSE_UINT32; }
    break;

  case 377:

/* Line 1455 of yacc.c  */
#line 2170 "vtkParse.y"
    { typeSig("vtkTypeInt64"); (yyval.integer) = VTK_PARSE_INT64; }
    break;

  case 378:

/* Line 1455 of yacc.c  */
#line 2171 "vtkParse.y"
    { typeSig("vtkTypeUInt64"); (yyval.integer) = VTK_PARSE_UINT64; }
    break;

  case 379:

/* Line 1455 of yacc.c  */
#line 2172 "vtkParse.y"
    { typeSig("vtkTypeFloat32"); (yyval.integer) = VTK_PARSE_FLOAT32; }
    break;

  case 380:

/* Line 1455 of yacc.c  */
#line 2173 "vtkParse.y"
    { typeSig("vtkTypeFloat64"); (yyval.integer) = VTK_PARSE_FLOAT64; }
    break;

  case 381:

/* Line 1455 of yacc.c  */
#line 2174 "vtkParse.y"
    { typeSig("vtkIdType"); (yyval.integer) = VTK_PARSE_ID_TYPE;}
    break;

  case 382:

/* Line 1455 of yacc.c  */
#line 2175 "vtkParse.y"
    { typeSig("double"); (yyval.integer) = VTK_PARSE_DOUBLE;}
    break;

  case 383:

/* Line 1455 of yacc.c  */
#line 2176 "vtkParse.y"
    { typeSig("float"); (yyval.integer) = VTK_PARSE_FLOAT;}
    break;

  case 384:

/* Line 1455 of yacc.c  */
#line 2177 "vtkParse.y"
    { typeSig("double"); (yyval.integer) = VTK_PARSE_DOUBLE;}
    break;

  case 385:

/* Line 1455 of yacc.c  */
#line 2178 "vtkParse.y"
    { typeSig("long double"); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 386:

/* Line 1455 of yacc.c  */
#line 2179 "vtkParse.y"
    { typeSig("char"); (yyval.integer) = VTK_PARSE_CHAR;}
    break;

  case 387:

/* Line 1455 of yacc.c  */
#line 2180 "vtkParse.y"
    { typeSig("signed char"); (yyval.integer) = VTK_PARSE_SIGNED_CHAR;}
    break;

  case 388:

/* Line 1455 of yacc.c  */
#line 2182 "vtkParse.y"
    { typeSig("unsigned char"); (yyval.integer) = VTK_PARSE_UNSIGNED_CHAR;}
    break;

  case 389:

/* Line 1455 of yacc.c  */
#line 2183 "vtkParse.y"
    { typeSig("int"); (yyval.integer) = VTK_PARSE_INT;}
    break;

  case 390:

/* Line 1455 of yacc.c  */
#line 2185 "vtkParse.y"
    { typeSig("unsigned int"); (yyval.integer) = VTK_PARSE_UNSIGNED_INT;}
    break;

  case 391:

/* Line 1455 of yacc.c  */
#line 2186 "vtkParse.y"
    { typeSig("short"); (yyval.integer) = VTK_PARSE_SHORT;}
    break;

  case 392:

/* Line 1455 of yacc.c  */
#line 2188 "vtkParse.y"
    { typeSig("unsigned short"); (yyval.integer) = VTK_PARSE_UNSIGNED_SHORT;}
    break;

  case 393:

/* Line 1455 of yacc.c  */
#line 2189 "vtkParse.y"
    { typeSig("long"); (yyval.integer) = VTK_PARSE_LONG;}
    break;

  case 394:

/* Line 1455 of yacc.c  */
#line 2191 "vtkParse.y"
    { typeSig("unsigned long"); (yyval.integer) = VTK_PARSE_UNSIGNED_LONG;}
    break;

  case 395:

/* Line 1455 of yacc.c  */
#line 2192 "vtkParse.y"
    { typeSig("long long"); (yyval.integer) = VTK_PARSE_LONG_LONG;}
    break;

  case 396:

/* Line 1455 of yacc.c  */
#line 2194 "vtkParse.y"
    {typeSig("unsigned long long");(yyval.integer)=VTK_PARSE_UNSIGNED_LONG_LONG;}
    break;

  case 397:

/* Line 1455 of yacc.c  */
#line 2195 "vtkParse.y"
    { typeSig("__int64"); (yyval.integer) = VTK_PARSE___INT64;}
    break;

  case 398:

/* Line 1455 of yacc.c  */
#line 2197 "vtkParse.y"
    { typeSig("unsigned __int64"); (yyval.integer) = VTK_PARSE_UNSIGNED___INT64;}
    break;

  case 399:

/* Line 1455 of yacc.c  */
#line 2198 "vtkParse.y"
    { typeSig("int"); (yyval.integer) = VTK_PARSE_INT; }
    break;

  case 400:

/* Line 1455 of yacc.c  */
#line 2199 "vtkParse.y"
    { typeSig("unsigned int"); (yyval.integer) = VTK_PARSE_UNSIGNED_INT; }
    break;

  case 401:

/* Line 1455 of yacc.c  */
#line 2205 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 402:

/* Line 1455 of yacc.c  */
#line 2206 "vtkParse.y"
    { postSig("{ "); }
    break;

  case 403:

/* Line 1455 of yacc.c  */
#line 2207 "vtkParse.y"
    {
          postSig("}");
          (yyval.str) = vtkstrcat4("{ ", (yyvsp[(3) - (6)].str), (yyvsp[(4) - (6)].str), " }");
        }
    break;

  case 406:

/* Line 1455 of yacc.c  */
#line 2214 "vtkParse.y"
    {(yyval.str) = "";}
    break;

  case 407:

/* Line 1455 of yacc.c  */
#line 2215 "vtkParse.y"
    { postSig(", "); }
    break;

  case 408:

/* Line 1455 of yacc.c  */
#line 2216 "vtkParse.y"
    {
          (yyval.str) = vtkstrcat3((yyvsp[(1) - (4)].str), ", ", (yyvsp[(4) - (4)].str));
        }
    break;

  case 409:

/* Line 1455 of yacc.c  */
#line 2220 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 410:

/* Line 1455 of yacc.c  */
#line 2221 "vtkParse.y"
    {postSig("+");}
    break;

  case 411:

/* Line 1455 of yacc.c  */
#line 2221 "vtkParse.y"
    {(yyval.str) = (yyvsp[(3) - (3)].str);}
    break;

  case 412:

/* Line 1455 of yacc.c  */
#line 2222 "vtkParse.y"
    {postSig("-");}
    break;

  case 413:

/* Line 1455 of yacc.c  */
#line 2223 "vtkParse.y"
    {
             (yyval.str) = vtkstrcat("-", (yyvsp[(3) - (3)].str));
             }
    break;

  case 414:

/* Line 1455 of yacc.c  */
#line 2226 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 415:

/* Line 1455 of yacc.c  */
#line 2227 "vtkParse.y"
    {postSig("(");}
    break;

  case 416:

/* Line 1455 of yacc.c  */
#line 2227 "vtkParse.y"
    {postSig(")"); (yyval.str) = (yyvsp[(3) - (4)].str);}
    break;

  case 417:

/* Line 1455 of yacc.c  */
#line 2228 "vtkParse.y"
    {postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 418:

/* Line 1455 of yacc.c  */
#line 2230 "vtkParse.y"
    {
             chopSig();
             if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
             postSig(">(");
             }
    break;

  case 419:

/* Line 1455 of yacc.c  */
#line 2236 "vtkParse.y"
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

  case 420:

/* Line 1455 of yacc.c  */
#line 2250 "vtkParse.y"
    { (yyval.str) = "static_cast"; }
    break;

  case 421:

/* Line 1455 of yacc.c  */
#line 2251 "vtkParse.y"
    { (yyval.str) = "const_cast"; }
    break;

  case 422:

/* Line 1455 of yacc.c  */
#line 2252 "vtkParse.y"
    { (yyval.str) = "dynamic_cast"; }
    break;

  case 423:

/* Line 1455 of yacc.c  */
#line 2253 "vtkParse.y"
    { (yyval.str) = "reinterpret_cast"; }
    break;

  case 424:

/* Line 1455 of yacc.c  */
#line 2255 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 425:

/* Line 1455 of yacc.c  */
#line 2257 "vtkParse.y"
    { (yyval.str) = vtkstrcat((yyvsp[(1) - (2)].str), (yyvsp[(2) - (2)].str)); }
    break;

  case 426:

/* Line 1455 of yacc.c  */
#line 2259 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 427:

/* Line 1455 of yacc.c  */
#line 2260 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 428:

/* Line 1455 of yacc.c  */
#line 2261 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 429:

/* Line 1455 of yacc.c  */
#line 2262 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 430:

/* Line 1455 of yacc.c  */
#line 2263 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 431:

/* Line 1455 of yacc.c  */
#line 2264 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 432:

/* Line 1455 of yacc.c  */
#line 2266 "vtkParse.y"
    { (yyval.str) = vtkstrdup(add_const_scope((yyvsp[(1) - (1)].str))); }
    break;

  case 433:

/* Line 1455 of yacc.c  */
#line 2275 "vtkParse.y"
    {preSig("void Set"); postSig("(");}
    break;

  case 434:

/* Line 1455 of yacc.c  */
#line 2276 "vtkParse.y"
    {
   postSig("a);");
   currentFunction->Macro = "vtkSetMacro";
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (7)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, (yyvsp[(6) - (7)].integer), getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 435:

/* Line 1455 of yacc.c  */
#line 2285 "vtkParse.y"
    {postSig("Get");}
    break;

  case 436:

/* Line 1455 of yacc.c  */
#line 2286 "vtkParse.y"
    {markSig();}
    break;

  case 437:

/* Line 1455 of yacc.c  */
#line 2286 "vtkParse.y"
    {swapSig();}
    break;

  case 438:

/* Line 1455 of yacc.c  */
#line 2287 "vtkParse.y"
    {
   postSig("();");
   currentFunction->Macro = "vtkGetMacro";
   currentFunction->Name = vtkstrcat("Get", (yyvsp[(4) - (9)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, (yyvsp[(7) - (9)].integer), getTypeId(), 0);
   output_function();
   }
    break;

  case 439:

/* Line 1455 of yacc.c  */
#line 2295 "vtkParse.y"
    {preSig("void Set");}
    break;

  case 440:

/* Line 1455 of yacc.c  */
#line 2296 "vtkParse.y"
    {
   postSig("(char *);");
   currentFunction->Macro = "vtkSetStringMacro";
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(4) - (5)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 441:

/* Line 1455 of yacc.c  */
#line 2305 "vtkParse.y"
    {preSig("char *Get");}
    break;

  case 442:

/* Line 1455 of yacc.c  */
#line 2306 "vtkParse.y"
    {
   postSig("();");
   currentFunction->Macro = "vtkGetStringMacro";
   currentFunction->Name = vtkstrcat("Get", (yyvsp[(4) - (5)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   output_function();
   }
    break;

  case 443:

/* Line 1455 of yacc.c  */
#line 2314 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 444:

/* Line 1455 of yacc.c  */
#line 2314 "vtkParse.y"
    {closeSig();}
    break;

  case 445:

/* Line 1455 of yacc.c  */
#line 2316 "vtkParse.y"
    {
   const char *typeText;
   chopSig();
   typeText = copySig();

   currentFunction->Macro = "vtkSetClampMacro";
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (10)].str));
   currentFunction->Signature =
     vtkstrcat5("void ", currentFunction->Name, "(", typeText, ");");
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, (yyvsp[(6) - (10)].integer), getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();

   currentFunction->Macro = "vtkSetClampMacro";
   currentFunction->Name = vtkstrcat3("Get", (yyvsp[(3) - (10)].str), "MinValue");
   currentFunction->Signature =
     vtkstrcat4(typeText, " ", currentFunction->Name, "();");
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, (yyvsp[(6) - (10)].integer), getTypeId(), 0);
   output_function();

   currentFunction->Macro = "vtkSetClampMacro";
   currentFunction->Name = vtkstrcat3("Get", (yyvsp[(3) - (10)].str), "MaxValue");
   currentFunction->Signature =
     vtkstrcat4(typeText, " ", currentFunction->Name, "();");
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, (yyvsp[(6) - (10)].integer), getTypeId(), 0);
   output_function();
   }
    break;

  case 446:

/* Line 1455 of yacc.c  */
#line 2347 "vtkParse.y"
    {preSig("void Set"); postSig("("); }
    break;

  case 447:

/* Line 1455 of yacc.c  */
#line 2348 "vtkParse.y"
    {
   postSig("*);");
   currentFunction->Macro = "vtkSetObjectMacro";
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (7)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 448:

/* Line 1455 of yacc.c  */
#line 2357 "vtkParse.y"
    {postSig("*Get");}
    break;

  case 449:

/* Line 1455 of yacc.c  */
#line 2358 "vtkParse.y"
    {markSig();}
    break;

  case 450:

/* Line 1455 of yacc.c  */
#line 2358 "vtkParse.y"
    {swapSig();}
    break;

  case 451:

/* Line 1455 of yacc.c  */
#line 2359 "vtkParse.y"
    {
   postSig("();");
   currentFunction->Macro = "vtkGetObjectMacro";
   currentFunction->Name = vtkstrcat("Get", (yyvsp[(4) - (9)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   output_function();
   }
    break;

  case 452:

/* Line 1455 of yacc.c  */
#line 2368 "vtkParse.y"
    {
   currentFunction->Macro = "vtkBooleanMacro";
   currentFunction->Name = vtkstrcat((yyvsp[(3) - (6)].str), "On");
   currentFunction->Comment = vtkstrdup(getComment());
   currentFunction->Signature =
     vtkstrcat3("void ", currentFunction->Name, "();");
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();

   currentFunction->Macro = "vtkBooleanMacro";
   currentFunction->Name = vtkstrcat((yyvsp[(3) - (6)].str), "Off");
   currentFunction->Comment = vtkstrdup(getComment());
   currentFunction->Signature =
     vtkstrcat3("void ", currentFunction->Name, "();");
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 453:

/* Line 1455 of yacc.c  */
#line 2385 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 454:

/* Line 1455 of yacc.c  */
#line 2386 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 2);
   }
    break;

  case 455:

/* Line 1455 of yacc.c  */
#line 2390 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 456:

/* Line 1455 of yacc.c  */
#line 2391 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 2);
   }
    break;

  case 457:

/* Line 1455 of yacc.c  */
#line 2395 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 458:

/* Line 1455 of yacc.c  */
#line 2396 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 3);
   }
    break;

  case 459:

/* Line 1455 of yacc.c  */
#line 2400 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 460:

/* Line 1455 of yacc.c  */
#line 2401 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 3);
   }
    break;

  case 461:

/* Line 1455 of yacc.c  */
#line 2405 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 462:

/* Line 1455 of yacc.c  */
#line 2406 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 4);
   }
    break;

  case 463:

/* Line 1455 of yacc.c  */
#line 2410 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 464:

/* Line 1455 of yacc.c  */
#line 2411 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 4);
   }
    break;

  case 465:

/* Line 1455 of yacc.c  */
#line 2415 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 466:

/* Line 1455 of yacc.c  */
#line 2416 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 6);
   }
    break;

  case 467:

/* Line 1455 of yacc.c  */
#line 2420 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 468:

/* Line 1455 of yacc.c  */
#line 2421 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 6);
   }
    break;

  case 469:

/* Line 1455 of yacc.c  */
#line 2425 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 470:

/* Line 1455 of yacc.c  */
#line 2427 "vtkParse.y"
    {
   const char *typeText;
   chopSig();
   typeText = copySig();
   currentFunction->Macro = "vtkSetVectorMacro";
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

  case 471:

/* Line 1455 of yacc.c  */
#line 2442 "vtkParse.y"
    {startSig();}
    break;

  case 472:

/* Line 1455 of yacc.c  */
#line 2444 "vtkParse.y"
    {
   chopSig();
   currentFunction->Macro = "vtkGetVectorMacro";
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

  case 473:

/* Line 1455 of yacc.c  */
#line 2457 "vtkParse.y"
    {
     currentFunction->Macro = "vtkViewportCoordinateMacro";
     currentFunction->Name = vtkstrcat3("Get", (yyvsp[(3) - (4)].str), "Coordinate");
     currentFunction->Signature =
       vtkstrcat3("vtkCoordinate *", currentFunction->Name, "();");
     currentFunction->Comment = vtkstrdup(getComment());
     set_return(currentFunction, VTK_PARSE_OBJECT_PTR, "vtkCoordinate", 0);
     output_function();

     currentFunction->Macro = "vtkViewportCoordinateMacro";
     currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (4)].str));
     currentFunction->Signature =
       vtkstrcat3("void ", currentFunction->Name, "(double, double);");
     currentFunction->Comment = vtkstrdup(getComment());
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Macro = "vtkViewportCoordinateMacro";
     currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (4)].str));
     currentFunction->Signature =
       vtkstrcat3("void ", currentFunction->Name, "(double a[2]);");
     currentFunction->Comment = vtkstrdup(getComment());
     add_argument(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 2);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Macro = "vtkViewportCoordinateMacro";
     currentFunction->Name = vtkstrcat("Get", (yyvsp[(3) - (4)].str));
     currentFunction->Signature =
       vtkstrcat3("double *", currentFunction->Name, "();");
     currentFunction->Comment = vtkstrdup(getComment());
     set_return(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 2);
     output_function();
   }
    break;

  case 474:

/* Line 1455 of yacc.c  */
#line 2494 "vtkParse.y"
    {
     currentFunction->Macro = "vtkWorldCoordinateMacro";
     currentFunction->Name = vtkstrcat3("Get", (yyvsp[(3) - (4)].str), "Coordinate");
     currentFunction->Signature =
       vtkstrcat3("vtkCoordinate *", currentFunction->Name, "();");
     currentFunction->Comment = vtkstrdup(getComment());
     set_return(currentFunction, VTK_PARSE_OBJECT_PTR, "vtkCoordinate", 0);
     output_function();

     currentFunction->Macro = "vtkWorldCoordinateMacro";
     currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (4)].str));
     currentFunction->Signature =
       vtkstrcat3("void ", currentFunction->Name, "(double, double, double);");
     currentFunction->Comment = vtkstrdup(getComment());
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Macro = "vtkWorldCoordinateMacro";
     currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (4)].str));
     currentFunction->Signature =
       vtkstrcat3("void ", currentFunction->Name, "(double a[3]);");
     currentFunction->Comment = vtkstrdup(getComment());
     add_argument(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 3);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Macro = "vtkWorldCoordinateMacro";
     currentFunction->Name = vtkstrcat("Get", (yyvsp[(3) - (4)].str));
     currentFunction->Signature =
       vtkstrcat3("double *", currentFunction->Name, "();");
     currentFunction->Comment = vtkstrdup(getComment());
     set_return(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 3);
     output_function();
   }
    break;

  case 475:

/* Line 1455 of yacc.c  */
#line 2532 "vtkParse.y"
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
     currentFunction->Macro = "vtkTypeMacro";
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

  case 476:

/* Line 1455 of yacc.c  */
#line 2588 "vtkParse.y"
    { (yyval.str) = "operator()"; }
    break;

  case 477:

/* Line 1455 of yacc.c  */
#line 2589 "vtkParse.y"
    { (yyval.str) = "operator[]"; }
    break;

  case 478:

/* Line 1455 of yacc.c  */
#line 2590 "vtkParse.y"
    { (yyval.str) = "operator new[]"; }
    break;

  case 479:

/* Line 1455 of yacc.c  */
#line 2591 "vtkParse.y"
    { (yyval.str) = "operator delete[]"; }
    break;

  case 481:

/* Line 1455 of yacc.c  */
#line 2594 "vtkParse.y"
    { (yyval.str) = "operator="; }
    break;

  case 482:

/* Line 1455 of yacc.c  */
#line 2595 "vtkParse.y"
    { (yyval.str) = "operator*"; }
    break;

  case 483:

/* Line 1455 of yacc.c  */
#line 2595 "vtkParse.y"
    { (yyval.str) = "operator/"; }
    break;

  case 484:

/* Line 1455 of yacc.c  */
#line 2596 "vtkParse.y"
    { (yyval.str) = "operator-"; }
    break;

  case 485:

/* Line 1455 of yacc.c  */
#line 2596 "vtkParse.y"
    { (yyval.str) = "operator+"; }
    break;

  case 486:

/* Line 1455 of yacc.c  */
#line 2597 "vtkParse.y"
    { (yyval.str) = "operator!"; }
    break;

  case 487:

/* Line 1455 of yacc.c  */
#line 2597 "vtkParse.y"
    { (yyval.str) = "operator~"; }
    break;

  case 488:

/* Line 1455 of yacc.c  */
#line 2598 "vtkParse.y"
    { (yyval.str) = "operator,"; }
    break;

  case 489:

/* Line 1455 of yacc.c  */
#line 2598 "vtkParse.y"
    { (yyval.str) = "operator<"; }
    break;

  case 490:

/* Line 1455 of yacc.c  */
#line 2599 "vtkParse.y"
    { (yyval.str) = "operator>"; }
    break;

  case 491:

/* Line 1455 of yacc.c  */
#line 2599 "vtkParse.y"
    { (yyval.str) = "operator&"; }
    break;

  case 492:

/* Line 1455 of yacc.c  */
#line 2600 "vtkParse.y"
    { (yyval.str) = "operator|"; }
    break;

  case 493:

/* Line 1455 of yacc.c  */
#line 2600 "vtkParse.y"
    { (yyval.str) = "operator^"; }
    break;

  case 494:

/* Line 1455 of yacc.c  */
#line 2601 "vtkParse.y"
    { (yyval.str) = "operator%"; }
    break;

  case 495:

/* Line 1455 of yacc.c  */
#line 2602 "vtkParse.y"
    { (yyval.str) = "operator new"; }
    break;

  case 496:

/* Line 1455 of yacc.c  */
#line 2603 "vtkParse.y"
    { (yyval.str) = "operator delete"; }
    break;

  case 497:

/* Line 1455 of yacc.c  */
#line 2604 "vtkParse.y"
    { (yyval.str) = "operator<<="; }
    break;

  case 498:

/* Line 1455 of yacc.c  */
#line 2605 "vtkParse.y"
    { (yyval.str) = "operator>>="; }
    break;

  case 499:

/* Line 1455 of yacc.c  */
#line 2606 "vtkParse.y"
    { (yyval.str) = "operator<<"; }
    break;

  case 500:

/* Line 1455 of yacc.c  */
#line 2607 "vtkParse.y"
    { (yyval.str) = "operator>>"; }
    break;

  case 501:

/* Line 1455 of yacc.c  */
#line 2608 "vtkParse.y"
    { (yyval.str) = "operator->*"; }
    break;

  case 502:

/* Line 1455 of yacc.c  */
#line 2609 "vtkParse.y"
    { (yyval.str) = "operator->"; }
    break;

  case 503:

/* Line 1455 of yacc.c  */
#line 2610 "vtkParse.y"
    { (yyval.str) = "operator+="; }
    break;

  case 504:

/* Line 1455 of yacc.c  */
#line 2611 "vtkParse.y"
    { (yyval.str) = "operator-="; }
    break;

  case 505:

/* Line 1455 of yacc.c  */
#line 2612 "vtkParse.y"
    { (yyval.str) = "operator*="; }
    break;

  case 506:

/* Line 1455 of yacc.c  */
#line 2613 "vtkParse.y"
    { (yyval.str) = "operator/="; }
    break;

  case 507:

/* Line 1455 of yacc.c  */
#line 2614 "vtkParse.y"
    { (yyval.str) = "operator%="; }
    break;

  case 508:

/* Line 1455 of yacc.c  */
#line 2615 "vtkParse.y"
    { (yyval.str) = "operator++"; }
    break;

  case 509:

/* Line 1455 of yacc.c  */
#line 2616 "vtkParse.y"
    { (yyval.str) = "operator--"; }
    break;

  case 510:

/* Line 1455 of yacc.c  */
#line 2617 "vtkParse.y"
    { (yyval.str) = "operator&="; }
    break;

  case 511:

/* Line 1455 of yacc.c  */
#line 2618 "vtkParse.y"
    { (yyval.str) = "operator|="; }
    break;

  case 512:

/* Line 1455 of yacc.c  */
#line 2619 "vtkParse.y"
    { (yyval.str) = "operator^="; }
    break;

  case 513:

/* Line 1455 of yacc.c  */
#line 2620 "vtkParse.y"
    {(yyval.str) = "operator&&=";}
    break;

  case 514:

/* Line 1455 of yacc.c  */
#line 2621 "vtkParse.y"
    {(yyval.str) = "operator||=";}
    break;

  case 515:

/* Line 1455 of yacc.c  */
#line 2622 "vtkParse.y"
    { (yyval.str) = "operator&&"; }
    break;

  case 516:

/* Line 1455 of yacc.c  */
#line 2623 "vtkParse.y"
    { (yyval.str) = "operator||"; }
    break;

  case 517:

/* Line 1455 of yacc.c  */
#line 2624 "vtkParse.y"
    { (yyval.str) = "operator=="; }
    break;

  case 518:

/* Line 1455 of yacc.c  */
#line 2625 "vtkParse.y"
    { (yyval.str) = "operator!="; }
    break;

  case 519:

/* Line 1455 of yacc.c  */
#line 2626 "vtkParse.y"
    { (yyval.str) = "operator<="; }
    break;

  case 520:

/* Line 1455 of yacc.c  */
#line 2627 "vtkParse.y"
    { (yyval.str) = "operator>="; }
    break;



/* Line 1455 of yacc.c  */
#line 8207 "vtkParse.tab.c"
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
#line 2652 "vtkParse.y"

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
