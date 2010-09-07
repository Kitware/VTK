
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

/* Line 222 of yacc.c  */
#line 1182 "vtkParse.y"

  const char   *str;
  unsigned int  integer;



/* Line 222 of yacc.c  */
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
#define YYLAST   6838

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  157
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  194
/* YYNRULES -- Number of rules.  */
#define YYNRULES  577
/* YYNRULES -- Number of states.  */
#define YYNSTATES  1017

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
     792,   794,   796,   798,   799,   800,   805,   807,   809,   812,
     814,   817,   818,   820,   822,   823,   825,   826,   829,   830,
     836,   837,   839,   840,   842,   844,   846,   848,   850,   852,
     854,   856,   858,   860,   862,   864,   866,   868,   870,   872,
     874,   876,   878,   880,   882,   884,   886,   889,   892,   896,
     899,   902,   906,   908,   911,   913,   916,   918,   921,   924,
     926,   928,   930,   932,   933,   937,   938,   944,   945,   951,
     952,   958,   960,   961,   966,   968,   970,   972,   974,   976,
     978,   980,   982,   984,   988,   992,   994,   996,   998,  1000,
    1002,  1004,  1006,  1008,  1011,  1013,  1015,  1018,  1020,  1022,
    1024,  1027,  1030,  1033,  1036,  1038,  1040,  1042,  1044,  1046,
    1048,  1050,  1052,  1054,  1056,  1058,  1060,  1062,  1064,  1066,
    1068,  1070,  1072,  1074,  1076,  1078,  1080,  1082,  1084,  1086,
    1088,  1090,  1092,  1094,  1096,  1098,  1100,  1102,  1104,  1106,
    1108,  1110,  1112,  1114,  1116,  1118,  1120,  1122,  1124,  1125,
    1132,  1133,  1135,  1136,  1137,  1142,  1144,  1145,  1149,  1150,
    1154,  1156,  1157,  1162,  1163,  1164,  1174,  1176,  1178,  1180,
    1182,  1184,  1187,  1189,  1191,  1193,  1195,  1197,  1199,  1201,
    1202,  1210,  1211,  1212,  1213,  1223,  1224,  1230,  1231,  1237,
    1238,  1239,  1250,  1251,  1259,  1260,  1261,  1262,  1272,  1279,
    1280,  1288,  1289,  1297,  1298,  1306,  1307,  1315,  1316,  1324,
    1325,  1333,  1334,  1342,  1343,  1351,  1352,  1362,  1363,  1373,
    1378,  1383,  1391,  1394,  1397,  1401,  1405,  1407,  1409,  1411,
    1413,  1415,  1417,  1419,  1421,  1423,  1425,  1427,  1429,  1431,
    1433,  1435,  1437,  1439,  1441,  1443,  1445,  1447,  1449,  1451,
    1453,  1455,  1457,  1459,  1461,  1463,  1465,  1467,  1469,  1471,
    1473,  1475,  1477,  1479,  1481,  1483,  1485,  1487,  1488,  1491,
    1492,  1495,  1497,  1499,  1501,  1503,  1505,  1507,  1509,  1511,
    1513,  1515,  1517,  1519,  1521,  1523,  1525,  1527,  1529,  1531,
    1533,  1535,  1537,  1539,  1541,  1543,  1545,  1547,  1549,  1551,
    1553,  1555,  1557,  1559,  1561,  1563,  1565,  1567,  1569,  1571,
    1573,  1575,  1577,  1579,  1581,  1583,  1585,  1587,  1589,  1591,
    1593,  1595,  1597,  1599,  1603,  1607,  1611,  1615
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     158,     0,    -1,    -1,    -1,   158,   159,   160,    -1,   252,
      -1,   178,   254,   134,    -1,   196,    -1,   162,    -1,   161,
      -1,   194,    -1,   164,   254,   134,    -1,   197,   164,   254,
     134,    -1,    41,    -1,   214,   226,    -1,   197,   214,   226,
      -1,   213,   226,    -1,   208,   226,    -1,   209,   226,    -1,
     197,   208,   226,    -1,   206,   226,    -1,   319,    -1,   297,
     134,    -1,     9,   135,   344,   136,    -1,    57,   135,   344,
     136,    -1,    97,   135,   344,   136,    -1,   134,    -1,    59,
      10,   137,   158,   138,    -1,    -1,    55,   297,   163,   137,
     158,   138,    -1,    55,   137,   344,   138,    -1,    -1,     4,
     279,   165,   174,   137,   171,   138,    -1,    -1,     4,   279,
     139,   293,   140,   166,   174,   137,   171,   138,    -1,    -1,
       3,   279,   167,   174,   137,   171,   138,    -1,    -1,     3,
     279,   139,   293,   140,   168,   174,   137,   171,   138,    -1,
       3,   137,   344,   138,    -1,    -1,    40,   279,   169,   174,
     137,   171,   138,    -1,    -1,    40,   279,   139,   293,   140,
     170,   174,   137,   171,   138,    -1,    40,   137,   344,   138,
      -1,    -1,    -1,   171,   172,   173,    -1,   171,   177,   141,
      -1,   252,    -1,   178,   254,   134,    -1,   196,    -1,   194,
      -1,   164,   254,   134,    -1,   197,   164,   254,   134,    -1,
      49,   192,    -1,    49,   191,    -1,    41,    -1,   214,   226,
      -1,    49,   214,   226,    -1,   197,   214,   226,    -1,   211,
     226,    -1,    49,   211,   226,    -1,   197,   211,   226,    -1,
     207,   226,    -1,   133,   135,   344,   136,   134,    -1,   319,
      -1,   134,    -1,    -1,   141,   175,    -1,   176,    -1,   176,
     142,   175,    -1,   295,    -1,     6,   295,    -1,     7,   295,
      -1,     5,   295,    -1,     5,    -1,     6,    -1,     7,    -1,
      -1,    39,   279,   179,   137,   181,   138,    -1,    -1,    39,
     180,   137,   181,   138,    -1,    -1,   182,    -1,   182,   142,
     181,    -1,   279,    -1,   279,   143,   185,    -1,   184,    -1,
     279,    -1,   296,    -1,   289,    -1,    16,    -1,    11,    -1,
      13,    -1,    12,    -1,    15,    -1,   183,    -1,    -1,   189,
     186,   185,    -1,    -1,   183,   190,   187,   185,    -1,    -1,
     135,   188,   185,   136,    -1,   144,    -1,   145,    -1,   146,
      -1,   144,    -1,   145,    -1,   147,    -1,   148,    -1,   149,
      -1,   150,    -1,   151,    -1,   152,    -1,    71,    -1,    72,
      -1,   197,   192,    -1,     4,   279,   193,    -1,     3,   279,
     193,    -1,     3,   193,    -1,    40,   279,   193,    -1,    40,
     193,    -1,   134,    -1,   137,   344,   138,   345,   134,    -1,
     141,   345,   134,    -1,   195,   284,   262,   134,    -1,   195,
     164,   248,   134,    -1,   195,   178,   248,   134,    -1,   195,
      60,   134,    -1,    54,    -1,    56,    55,   295,   134,    -1,
      56,    53,   295,   134,    -1,    56,   295,   134,    -1,    52,
     139,   140,    -1,    -1,    52,   139,   198,   199,   140,    -1,
     201,    -1,    -1,   201,   142,   200,   199,    -1,   302,   204,
      -1,   203,   204,    -1,    -1,   202,   197,   204,    -1,     4,
      -1,    53,    -1,    -1,    -1,   279,   205,   249,    -1,    61,
     135,   208,   136,    -1,    61,   135,   211,   136,    -1,   282,
     223,    -1,   282,   210,   223,    -1,   297,    94,   146,   238,
      -1,    50,   297,    94,   146,   238,    -1,   297,    94,   231,
      -1,    50,   297,    94,   231,    -1,   297,    94,   297,    94,
     146,   238,    -1,    50,   297,    94,   297,    94,   146,   238,
      -1,   297,    94,   297,    94,   231,    -1,    50,   297,    94,
     297,    94,   231,    -1,   297,    94,    -1,   210,   297,    94,
      -1,   146,   238,    -1,    50,   146,   238,    -1,     8,   146,
     238,    -1,   231,    -1,    50,   231,    -1,   212,   231,    -1,
     282,   223,    -1,     8,   282,   223,    -1,    64,    -1,    50,
      64,    -1,    64,    50,    -1,   297,    94,   215,    -1,   282,
     210,   218,    -1,   215,    -1,   282,   218,    -1,     8,   284,
     218,    -1,    -1,    -1,    46,   282,   135,   216,   241,   136,
     217,   225,    -1,    -1,   220,   219,   225,    -1,    -1,    -1,
      46,   342,   221,   135,   222,   241,   136,    -1,    -1,   227,
     224,   225,    -1,    -1,   143,    16,    -1,    45,    16,    -1,
      43,    -1,   134,    -1,   137,   344,   138,    -1,    -1,   279,
     135,   228,   241,   136,    -1,    -1,    -1,   279,   139,   229,
     293,   140,   135,   230,   241,   136,    -1,    -1,   233,   232,
     235,    -1,    -1,   279,   135,   234,   241,   136,    -1,    -1,
     141,   237,   236,    -1,    -1,   142,   237,   236,    -1,   279,
     135,   344,   136,    -1,   239,    -1,    -1,   279,   135,   240,
     241,   136,    -1,    -1,    -1,   242,   243,    -1,    93,    -1,
     245,    -1,    -1,   245,   142,   244,   243,    -1,    -1,    -1,
     246,   284,   260,   247,   249,    -1,    60,    -1,   279,    -1,
     298,   279,    -1,    -1,   250,    -1,    -1,   143,   251,   305,
      -1,   282,   253,   255,   134,    -1,    58,    60,   255,   134,
      -1,    60,   255,   134,    -1,   262,   249,    -1,    -1,   257,
     255,    -1,    -1,    -1,   255,   142,   256,   257,    -1,    -1,
     258,   253,    -1,    -1,   298,   259,   253,    -1,   270,   272,
      -1,    -1,   264,   268,   136,   261,   266,    -1,   271,   272,
      -1,    -1,   265,   269,   136,   263,   266,    -1,   135,    -1,
      95,    -1,    96,    -1,    95,    -1,    96,    -1,    -1,    -1,
     135,   267,   241,   136,    -1,   273,    -1,   260,    -1,   298,
     260,    -1,   262,    -1,   298,   262,    -1,    -1,   271,    -1,
     279,    -1,    -1,   273,    -1,    -1,   274,   275,    -1,    -1,
     277,   153,   276,   278,   154,    -1,    -1,   275,    -1,    -1,
     185,    -1,    57,    -1,    97,    -1,     9,    -1,    38,    -1,
      37,    -1,    98,    -1,    99,    -1,   280,    -1,   281,    -1,
     102,    -1,   103,    -1,   104,    -1,   105,    -1,   106,    -1,
     107,    -1,   108,    -1,   109,    -1,   110,    -1,   111,    -1,
     100,    -1,   101,    -1,   284,    -1,    51,   284,    -1,    59,
     284,    -1,    59,    10,   284,    -1,    50,   284,    -1,   283,
     284,    -1,    50,   283,   284,    -1,    58,    -1,    58,    50,
      -1,   285,    -1,   285,   298,    -1,   287,    -1,   286,   287,
      -1,   287,   286,    -1,    43,    -1,   301,    -1,   289,    -1,
     296,    -1,    -1,    53,   288,   295,    -1,    -1,    57,   139,
     290,   293,   140,    -1,    -1,     9,   139,   291,   293,   140,
      -1,    -1,    97,   139,   292,   293,   140,    -1,   284,    -1,
      -1,   284,   142,   294,   293,    -1,    57,    -1,     9,    -1,
      97,    -1,    38,    -1,    37,    -1,    98,    -1,    99,    -1,
     289,    -1,   296,    -1,   297,    94,   295,    -1,   289,    94,
     295,    -1,     9,    -1,    97,    -1,    57,    -1,    38,    -1,
      37,    -1,    98,    -1,    99,    -1,   150,    -1,   299,   150,
      -1,   299,    -1,   300,    -1,   299,   300,    -1,   147,    -1,
      44,    -1,   302,    -1,     4,   303,    -1,     3,   303,    -1,
      40,   303,    -1,    39,   303,    -1,   304,    -1,   303,    -1,
      98,    -1,    99,    -1,    37,    -1,    38,    -1,     9,    -1,
      57,    -1,    97,    -1,    33,    -1,    34,    -1,    35,    -1,
      36,    -1,   102,    -1,   103,    -1,   104,    -1,   105,    -1,
     106,    -1,   107,    -1,   108,    -1,   109,    -1,   110,    -1,
     111,    -1,   100,    -1,   101,    -1,    17,    -1,    18,    -1,
      19,    -1,    30,    -1,    31,    -1,    32,    -1,    20,    -1,
      21,    -1,    22,    -1,    23,    -1,    24,    -1,    25,    -1,
      26,    -1,    27,    -1,    28,    -1,    29,    -1,    48,    -1,
      47,    -1,   310,    -1,    -1,   137,   306,   305,   308,   307,
     138,    -1,    -1,   142,    -1,    -1,    -1,   308,   142,   309,
     305,    -1,   318,    -1,    -1,   145,   311,   318,    -1,    -1,
     144,   312,   318,    -1,   317,    -1,    -1,   135,   313,   310,
     136,    -1,    -1,    -1,   316,   139,   314,   285,   140,   135,
     315,   310,   136,    -1,    65,    -1,    67,    -1,    66,    -1,
      68,    -1,    10,    -1,   317,    10,    -1,    16,    -1,    11,
      -1,    13,    -1,    12,    -1,    14,    -1,    15,    -1,   295,
      -1,    -1,   112,   135,   279,   142,   320,   284,   136,    -1,
      -1,    -1,    -1,   113,   135,   321,   279,   142,   322,   284,
     323,   136,    -1,    -1,   114,   135,   324,   279,   136,    -1,
      -1,   115,   135,   325,   279,   136,    -1,    -1,    -1,   116,
     135,   279,   142,   326,   301,   327,   142,   345,   136,    -1,
      -1,   117,   135,   279,   142,   328,   301,   136,    -1,    -1,
      -1,    -1,   118,   135,   329,   279,   142,   330,   301,   331,
     136,    -1,   119,   135,   279,   142,   301,   136,    -1,    -1,
     120,   135,   279,   142,   332,   301,   136,    -1,    -1,   124,
     135,   279,   142,   333,   301,   136,    -1,    -1,   121,   135,
     279,   142,   334,   301,   136,    -1,    -1,   125,   135,   279,
     142,   335,   301,   136,    -1,    -1,   122,   135,   279,   142,
     336,   301,   136,    -1,    -1,   126,   135,   279,   142,   337,
     301,   136,    -1,    -1,   123,   135,   279,   142,   338,   301,
     136,    -1,    -1,   127,   135,   279,   142,   339,   301,   136,
      -1,    -1,   128,   135,   279,   142,   340,   301,   142,    11,
     136,    -1,    -1,   129,   135,   279,   142,   341,   301,   142,
      11,   136,    -1,   130,   135,   279,   136,    -1,   131,   135,
     279,   136,    -1,   132,   135,   279,   142,   279,   307,   136,
      -1,   135,   136,    -1,   153,   154,    -1,    62,   153,   154,
      -1,    63,   153,   154,    -1,   343,    -1,   143,    -1,   147,
      -1,   148,    -1,   144,    -1,   145,    -1,   155,    -1,   146,
      -1,   142,    -1,   139,    -1,   140,    -1,   150,    -1,   151,
      -1,   152,    -1,   149,    -1,    62,    -1,    63,    -1,    69,
      -1,    70,    -1,    71,    -1,    72,    -1,    73,    -1,    74,
      -1,    77,    -1,    78,    -1,    79,    -1,    80,    -1,    81,
      -1,    75,    -1,    76,    -1,    82,    -1,    83,    -1,    84,
      -1,    85,    -1,    86,    -1,    87,    -1,    88,    -1,    89,
      -1,    90,    -1,    91,    -1,    92,    -1,    -1,   344,   346,
      -1,    -1,   345,   347,    -1,   134,    -1,   347,    -1,    42,
      -1,   348,    -1,   350,    -1,   349,    -1,    54,    -1,   343,
      -1,   141,    -1,   156,    -1,    94,    -1,     4,    -1,    52,
      -1,    38,    -1,    37,    -1,    98,    -1,    99,    -1,   304,
      -1,    13,    -1,    11,    -1,    12,    -1,    14,    -1,    15,
      -1,    10,    -1,    41,    -1,    43,    -1,    44,    -1,    45,
      -1,     3,    -1,    46,    -1,    58,    -1,    50,    -1,     8,
      -1,    39,    -1,    40,    -1,    53,    -1,    16,    -1,    60,
      -1,    93,    -1,     5,    -1,     7,    -1,     6,    -1,    55,
      -1,    56,    -1,    59,    -1,     9,    -1,    57,    -1,    97,
      -1,    67,    -1,    66,    -1,    65,    -1,    68,    -1,   137,
     344,   138,    -1,   153,   344,   154,    -1,   135,   344,   136,
      -1,    95,   344,   136,    -1,    96,   344,   136,    -1
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
    1954,  1957,  1959,  1962,  1963,  1963,  1965,  1968,  1969,  1973,
    1974,  1977,  1977,  1979,  1981,  1981,  1983,  1983,  1985,  1985,
    1987,  1987,  1989,  1990,  1996,  1997,  1998,  1999,  2000,  2001,
    2002,  2003,  2004,  2007,  2008,  2009,  2010,  2011,  2012,  2013,
    2014,  2015,  2016,  2019,  2020,  2027,  2028,  2029,  2030,  2032,
    2033,  2035,  2039,  2040,  2042,  2043,  2045,  2046,  2047,  2049,
    2051,  2052,  2054,  2056,  2056,  2060,  2060,  2063,  2063,  2066,
    2066,  2070,  2070,  2070,  2072,  2073,  2074,  2075,  2076,  2077,
    2078,  2079,  2080,  2082,  2087,  2093,  2093,  2093,  2093,  2093,
    2094,  2094,  2110,  2111,  2112,  2117,  2118,  2130,  2131,  2134,
    2135,  2136,  2137,  2138,  2141,  2142,  2145,  2146,  2147,  2148,
    2149,  2150,  2151,  2154,  2155,  2156,  2157,  2158,  2159,  2160,
    2161,  2162,  2163,  2164,  2165,  2166,  2167,  2168,  2169,  2170,
    2171,  2172,  2173,  2174,  2175,  2177,  2178,  2180,  2181,  2183,
    2184,  2186,  2187,  2189,  2190,  2192,  2193,  2199,  2200,  2200,
    2206,  2206,  2208,  2209,  2209,  2214,  2215,  2215,  2216,  2216,
    2220,  2221,  2221,  2222,  2224,  2222,  2244,  2245,  2246,  2247,
    2249,  2250,  2253,  2254,  2255,  2256,  2257,  2258,  2259,  2269,
    2269,  2278,  2279,  2279,  2278,  2287,  2287,  2296,  2296,  2304,
    2304,  2304,  2334,  2333,  2343,  2344,  2344,  2343,  2352,  2368,
    2368,  2373,  2373,  2378,  2378,  2383,  2383,  2388,  2388,  2393,
    2393,  2398,  2398,  2403,  2403,  2408,  2408,  2424,  2424,  2437,
    2470,  2504,  2557,  2558,  2559,  2560,  2561,  2563,  2564,  2564,
    2565,  2565,  2566,  2566,  2567,  2567,  2568,  2568,  2569,  2569,
    2570,  2571,  2572,  2573,  2574,  2575,  2576,  2577,  2578,  2579,
    2580,  2581,  2582,  2583,  2584,  2585,  2586,  2587,  2588,  2589,
    2590,  2591,  2592,  2593,  2594,  2595,  2596,  2602,  2602,  2603,
    2603,  2605,  2605,  2607,  2607,  2607,  2607,  2607,  2608,  2608,
    2608,  2608,  2608,  2608,  2609,  2609,  2609,  2609,  2609,  2610,
    2610,  2610,  2610,  2610,  2611,  2611,  2611,  2611,  2611,  2611,
    2612,  2612,  2612,  2612,  2612,  2612,  2612,  2613,  2613,  2613,
    2613,  2613,  2613,  2614,  2614,  2614,  2614,  2614,  2614,  2615,
    2615,  2615,  2615,  2617,  2618,  2619,  2619,  2619
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
  "maybe_array_or_args", "$@41", "maybe_indirect_maybe_var_id",
  "maybe_indirect_var_id", "maybe_var_id", "var_id", "maybe_var_array",
  "var_array", "$@42", "array", "$@43", "more_array", "array_size",
  "any_id", "sized_type_id", "special_type_id", "storage_type",
  "static_mod", "type", "type_red", "const_mod", "type_red1", "$@44",
  "templated_id", "$@45", "$@46", "$@47", "types", "$@48",
  "maybe_scoped_id", "scoped_id", "class_id", "type_indirection",
  "pointers", "pointer_or_const_pointer", "type_red2", "type_simple",
  "type_id", "type_primitive", "value", "$@49", "maybe_comma",
  "more_values", "$@50", "literal", "$@51", "$@52", "$@53", "$@54", "$@55",
  "any_cast", "string_literal", "literal2", "macro", "$@56", "$@57",
  "$@58", "$@59", "$@60", "$@61", "$@62", "$@63", "$@64", "$@65", "$@66",
  "$@67", "$@68", "$@69", "$@70", "$@71", "$@72", "$@73", "$@74", "$@75",
  "$@76", "$@77", "op_token", "op_token_no_delim", "maybe_other",
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
     264,   265,   265,   266,   267,   266,   266,   268,   268,   269,
     269,   270,   270,   271,   272,   272,   274,   273,   276,   275,
     277,   277,   278,   278,   279,   279,   279,   279,   279,   279,
     279,   279,   279,   280,   280,   280,   280,   280,   280,   280,
     280,   280,   280,   281,   281,   282,   282,   282,   282,   282,
     282,   282,   283,   283,   284,   284,   285,   285,   285,   286,
     287,   287,   287,   288,   287,   290,   289,   291,   289,   292,
     289,   293,   294,   293,   295,   295,   295,   295,   295,   295,
     295,   295,   295,   296,   296,   297,   297,   297,   297,   297,
     297,   297,   298,   298,   298,   299,   299,   300,   300,   301,
     301,   301,   301,   301,   302,   302,   303,   303,   303,   303,
     303,   303,   303,   304,   304,   304,   304,   304,   304,   304,
     304,   304,   304,   304,   304,   304,   304,   304,   304,   304,
     304,   304,   304,   304,   304,   304,   304,   304,   304,   304,
     304,   304,   304,   304,   304,   304,   304,   305,   306,   305,
     307,   307,   308,   309,   308,   310,   311,   310,   312,   310,
     310,   313,   310,   314,   315,   310,   316,   316,   316,   316,
     317,   317,   318,   318,   318,   318,   318,   318,   318,   320,
     319,   321,   322,   323,   319,   324,   319,   325,   319,   326,
     327,   319,   328,   319,   329,   330,   331,   319,   319,   332,
     319,   333,   319,   334,   319,   335,   319,   336,   319,   337,
     319,   338,   319,   339,   319,   340,   319,   341,   319,   319,
     319,   319,   342,   342,   342,   342,   342,   343,   343,   343,
     343,   343,   343,   343,   343,   343,   343,   343,   343,   343,
     343,   343,   343,   343,   343,   343,   343,   343,   343,   343,
     343,   343,   343,   343,   343,   343,   343,   343,   343,   343,
     343,   343,   343,   343,   343,   343,   343,   344,   344,   345,
     345,   346,   346,   347,   347,   347,   347,   347,   347,   347,
     347,   347,   347,   347,   347,   347,   347,   347,   347,   347,
     347,   347,   347,   347,   347,   347,   347,   347,   347,   347,
     347,   347,   347,   347,   347,   347,   347,   347,   347,   347,
     347,   347,   347,   347,   347,   347,   347,   347,   347,   347,
     347,   347,   347,   348,   349,   350,   350,   350
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
       1,     1,     1,     0,     0,     4,     1,     1,     2,     1,
       2,     0,     1,     1,     0,     1,     0,     2,     0,     5,
       0,     1,     0,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     2,     3,     2,
       2,     3,     1,     2,     1,     2,     1,     2,     2,     1,
       1,     1,     1,     0,     3,     0,     5,     0,     5,     0,
       5,     1,     0,     4,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     3,     3,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     1,     1,     2,     1,     1,     1,
       2,     2,     2,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     0,     6,
       0,     1,     0,     0,     4,     1,     0,     3,     0,     3,
       1,     0,     4,     0,     0,     9,     1,     1,     1,     1,
       1,     2,     1,     1,     1,     1,     1,     1,     1,     0,
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
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     3,     3,     3,     3,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       2,     3,     1,     0,     0,     0,     0,   360,   379,   380,
     381,   385,   386,   387,   388,   389,   390,   391,   392,   393,
     394,   382,   383,   384,   363,   364,   365,   366,   358,   359,
      81,     0,    13,   309,     0,   396,   395,     0,     0,     0,
     313,   130,     0,     0,   361,   302,     0,   235,     0,   362,
     356,   357,   377,   378,   367,   368,   369,   370,   371,   372,
     373,   374,   375,   376,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    26,     4,     9,     8,   238,
     238,    10,     0,     7,     0,     0,     0,     0,     0,     0,
     176,     5,     0,     0,   295,   304,     0,   306,   311,   312,
       0,   310,   349,   355,   354,    21,   360,   358,   359,   361,
     362,   356,   357,   293,   294,   283,   284,   285,   286,   287,
     288,   289,   290,   291,   292,   517,    35,   281,   282,   351,
      31,   350,     0,     0,   360,     0,     0,   361,   362,     0,
       0,   517,   317,     0,    79,   353,   517,    40,   352,     0,
     302,     0,     0,     0,   299,     0,   296,   135,     0,   335,
     339,   338,   337,   336,   340,   341,   517,    28,   325,   328,
     327,     0,     0,   324,   326,   329,   330,   331,     0,   332,
     517,   315,   303,   235,     0,   297,     0,     0,   517,   319,
       0,   431,   435,   437,     0,     0,   444,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   348,   347,   342,     0,   235,     0,   240,   344,   345,
       0,     0,     0,     0,     0,   238,     0,     0,     0,   193,
     517,    20,    17,    18,    16,    14,   276,   278,   277,     0,
     274,   251,   252,   275,   279,   280,     0,   177,   182,   151,
     187,   235,   225,     0,   264,   263,     0,   300,   305,   307,
     308,     0,     0,    22,     0,     0,    68,     0,    68,   360,
     358,   359,   361,   362,   356,   357,   178,     0,     0,     0,
      83,     0,     0,     0,    68,     0,   179,   301,     0,   134,
     142,   314,     0,     0,     0,     0,   133,     0,     0,     0,
       2,   298,   231,   236,     0,     0,     0,     0,   276,   278,
     277,   274,   275,   279,   280,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    11,   234,   239,   263,
       0,   343,   346,     6,   129,     0,   223,     0,     0,     0,
       0,    19,    15,     0,     0,   491,   492,   493,   494,   495,
     496,   497,   498,   504,   505,   499,   500,   501,   502,   503,
     506,   507,   508,   509,   510,   511,   512,   513,   514,   515,
     516,     0,   485,   486,   484,   477,   480,   481,   483,   478,
     479,   490,   487,   488,   489,     0,   482,   184,   476,   175,
     152,     0,   189,   189,     0,   227,   232,   226,   259,     0,
       0,   245,   265,   270,   195,   197,   161,   334,   325,   328,
     327,   324,   326,   329,   330,     0,   174,   155,   200,     0,
     333,     0,   549,   532,   560,   562,   561,   553,   566,   544,
     540,   541,   539,   542,   543,   557,   535,   534,   554,   555,
     545,   523,   546,   547,   548,   550,   552,   533,   556,   527,
     563,   564,   567,   551,   565,   558,   491,   492,   571,   570,
     569,   572,   559,   531,   517,   517,   568,   536,   537,   521,
     517,   517,    39,   529,   517,   530,   538,   528,   518,   522,
     524,   526,   525,   321,     0,     0,     0,     0,     0,    23,
       0,     0,    84,    86,    83,    44,     0,     0,   213,     0,
     156,     0,   144,   145,     0,   137,     0,   146,   146,    30,
       2,   132,   131,    24,     0,   230,     3,   238,   149,    25,
       0,   429,     0,     0,     0,   439,   442,     0,     0,   449,
     453,   457,   461,   451,   455,   459,   463,   465,   467,   469,
     470,     0,   241,   127,   224,   128,   126,    12,   194,     0,
       0,   472,   473,     0,   162,   192,     0,     0,   183,   188,
     229,     0,   246,   260,   267,     0,   213,     0,   153,   209,
       0,   204,   202,     0,     0,     0,     0,     0,     0,   322,
      37,     0,     0,     0,    69,    70,    72,    45,    33,    45,
     318,    82,    83,     0,     0,    42,    45,     0,   219,   154,
       0,   136,   138,   146,   141,   147,   140,     3,   316,    27,
     237,   320,     0,   432,   436,   438,     0,     0,   445,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     400,   474,   475,   185,   191,   190,   420,   423,   425,   424,
     426,   427,   422,   416,   418,   417,   419,   411,   398,   408,
     406,   428,   228,   397,     0,   410,   405,   253,   268,     0,
       0,   210,     0,   201,   213,     0,   159,   576,   577,   575,
     573,   574,     0,    68,    75,    73,    74,     0,    46,    68,
      46,    85,   276,    93,    95,    94,    96,    92,   274,   275,
     102,   104,   105,   106,    97,    88,    87,    98,    89,    91,
      90,    80,    68,    46,   180,   222,   215,   214,   216,     0,
       0,   160,   142,   143,   225,    29,     0,     0,   440,     0,
       0,   448,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   401,     0,   213,     0,     0,     0,     0,   413,
     421,   254,   247,   256,   272,   196,     0,   213,   206,     0,
       0,   157,   323,     0,    71,    76,    77,    78,    36,     0,
       0,     0,    32,     0,   115,   116,   107,   108,   109,   110,
     111,   112,   113,   114,   100,     0,     0,    41,   189,   217,
     261,   158,   139,   148,   430,   433,     0,   443,   446,   450,
     454,   458,   462,   452,   456,   460,   464,     0,     0,   471,
       0,     0,   402,   409,   407,     0,   213,   273,     0,   198,
       0,     0,   205,   517,   203,    45,     0,   360,   358,   359,
      57,     0,     0,   361,     0,   171,   362,   356,   357,   377,
     378,   367,   368,   369,   370,   371,   372,   373,   374,   375,
     376,     0,    67,     0,   238,    47,   238,    52,    51,     0,
       0,     0,     0,     0,   166,    49,     0,    66,    48,    45,
       0,     0,    99,    45,   181,   219,   249,   250,   248,   220,
     261,   264,   262,     0,   519,     0,     0,     0,   186,   412,
     400,     0,     0,   269,   213,   211,   206,     0,    46,     0,
       0,   295,     0,     0,     0,    56,    55,     0,     0,     0,
       0,   172,     0,   167,     0,   173,   517,   163,     0,     0,
     238,     0,     0,    64,    61,   168,    58,   169,    46,   103,
     101,    46,   218,   225,   257,     0,   261,   242,   434,     0,
     447,   466,   468,   403,     0,     0,   255,     0,   207,   208,
      38,   165,   170,   123,   517,   519,   120,     0,     0,   122,
       0,     0,     0,     0,   117,    62,    59,   164,     0,     0,
       0,     0,    53,    50,     0,    63,    60,    34,    43,   221,
     243,   258,   441,   520,     0,   399,   414,   199,     0,     0,
     119,   118,   121,   150,     0,    54,   253,   404,     0,   519,
     125,    65,   244,     0,     0,   415,   124
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     3,    86,    87,    88,   303,    89,   278,   699,
     276,   693,   294,   722,   698,   779,   865,   506,   604,   605,
     780,    90,   291,   153,   511,   512,   714,   715,   716,   795,
     881,   783,   717,   794,   915,   916,   966,    91,    92,    93,
      94,   300,   524,   732,   525,   526,   527,   624,   734,    95,
     870,    96,    97,   256,   871,   872,    98,    99,   100,   518,
     798,   257,   412,   258,   573,   754,   259,   413,   578,   241,
     260,   586,   587,   904,   874,   591,   438,   684,   683,   832,
     768,   588,   589,   767,   617,   618,   727,   885,   728,   729,
     943,   355,   416,   417,   581,   101,   261,   224,   196,   537,
     225,   226,   350,   889,  1006,   262,   677,   890,   263,   762,
     826,   945,   419,   891,   264,   421,   422,   423,   584,   764,
     585,   828,   439,   137,   138,   910,   103,   104,   105,   106,
     107,   168,   108,   308,   289,   317,   504,   692,   671,   109,
     150,   227,   228,   229,   111,   112,   113,   114,   672,   756,
     753,   900,   994,   673,   758,   757,   755,   825,  1008,   674,
     675,   676,   115,   632,   326,   737,   893,   327,   328,   636,
     806,   637,   331,   740,   895,   640,   644,   641,   645,   642,
     646,   643,   647,   648,   649,   407,   497,   274,   949,   498,
     499,   500,   501,   502
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -875
static const yytype_int16 yypact[] =
{
    -875,   125,  -875,  5027,  5151,  4661,  6042,     2,  -875,  -875,
    -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,
    -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,    45,    68,
    4661,  5166,  -875,  -875,  5497,  -875,  -875,  5606,  6042,   -13,
    -875,  -875,   299,   362,   132,   197,  5715,  -875,    62,   179,
      84,   161,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,
    -875,  -875,  -875,  -875,    64,    96,   114,   139,   140,   147,
     155,   157,   185,   190,   191,   193,   198,   200,   204,   205,
     206,   209,   210,   212,   217,  -875,  -875,  -875,  -875,    30,
      30,  -875,  5824,  -875,  5388,   175,   175,   175,   175,   175,
    -875,  -875,  6398,  6042,  -875,    13,  6151,   254,    74,  -875,
     162,  -875,  -875,  -875,  -875,  -875,   301,   313,   341,   347,
     372,   380,   388,  -875,  -875,  -875,  -875,  -875,  -875,  -875,
    -875,  -875,  -875,  -875,  -875,  -875,   218,  -875,  -875,  -875,
     220,  -875,   232,   232,   -12,   232,   232,    -7,    80,   316,
     270,  -875,  -875,   228,  -875,  -875,  -875,   231,  -875,  5606,
     317,  5933,   244,  6042,  -875,   289,  -875,   269,   323,  -875,
    -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,   -12,   319,
     320,   323,   323,    -7,    80,   324,   332,    74,   276,  -875,
    -875,  -875,  -875,  -875,  4824,  -875,   -37,  5497,  -875,  -875,
    6633,  -875,  -875,  -875,  6633,  6633,  -875,  6633,  6633,  6633,
    6633,  6633,  6633,  6633,  6633,  6633,  6633,  6633,  6633,  6633,
    6633,  -875,  -875,  -875,   293,  -875,  6503,  -875,    23,  -875,
     294,   298,   846,   846,  6503,    30,   175,   175,  6578,  -875,
    -875,  -875,  -875,  -875,  -875,  -875,   349,   319,   320,  6683,
     351,  -875,  -875,   357,   324,   332,  6608,  -875,  -875,  -875,
    -875,  -875,   268,  4452,   302,   167,   359,  -875,  -875,  -875,
    -875,   323,   605,  -875,   996,  6042,   305,  6042,   305,  -875,
    -875,  -875,  -875,  -875,  -875,  -875,  -875,   323,  1150,  6042,
    6633,   321,  1304,  6042,   305,  6042,  -875,  -875,  4582,  -875,
    6368,  -875,  1458,   326,   322,   330,  -875,  1612,  6042,   102,
    -875,  -875,  -875,  -875,   329,  6633,  1766,  6042,  -875,  -875,
    -875,  -875,  -875,  -875,  -875,   315,  6633,  6633,  6633,   325,
     327,  6633,   328,   334,   335,   345,   356,   363,   365,   366,
     370,   374,   376,   338,   358,   378,  -875,   384,  -875,  -875,
    6503,  -875,  -875,  -875,  -875,   394,  -875,  6633,   422,   424,
     426,  -875,  -875,   167,  1920,   340,   408,  -875,  -875,  -875,
    -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,
    -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,
    -875,   427,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,
    -875,  -875,  -875,  -875,  -875,   312,  -875,  -875,  -875,  -875,
    -875,   389,    33,    33,   126,  -875,  -875,  -875,  -875,   428,
    6503,  -875,  -875,  -875,  -875,  -875,  -875,  -875,   -19,     8,
       9,    51,   152,   100,   123,  6633,  -875,  -875,  -875,   430,
    -875,   468,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,
    -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,
    -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,
    -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,
    -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,
    -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,
    -875,  -875,  -875,   431,   432,   186,   429,   434,   438,  -875,
     440,   443,   436,   441,  6633,  -875,   447,   451,   453,  6633,
    -875,   496,  -875,  -875,   454,   455,   543,  6633,  6633,  -875,
    -875,  -875,  -875,  -875,   458,  -875,   461,    13,  -875,  -875,
     471,  -875,   459,   476,   479,  -875,  -875,   474,  6260,  -875,
    -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,
    -875,  6633,  -875,  -875,  -875,  -875,  -875,  -875,  -875,   463,
     464,  -875,  -875,   485,  -875,  -875,   606,   607,  -875,  -875,
    -875,   539,  -875,  -875,   472,   473,   453,  6042,  -875,  -875,
     486,   483,  -875,  4603,  2074,  2228,  2382,  2536,  2690,  -875,
    -875,   323,   323,   323,  -875,   487,  -875,  -875,  -875,  -875,
    -875,  -875,  6633,  4527,   489,  -875,  -875,   492,    19,  -875,
    4637,  -875,  -875,  6633,  -875,  -875,  -875,   493,  -875,  -875,
    -875,  -875,  6042,  -875,  -875,  -875,  6260,  6260,  -875,   498,
    6260,  6260,  6260,  6260,  6260,  6260,  6260,  6260,  6260,  6260,
     490,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,
    -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,
    -875,  -875,  -875,  -875,   500,   625,  -875,    16,  -875,   504,
     501,  -875,  6633,  -875,   453,  6633,  -875,  -875,  -875,  -875,
    -875,  -875,  6042,   305,  -875,  -875,  -875,   186,    43,   305,
      83,  -875,   -12,  -875,  -875,  -875,  -875,  -875,    -7,    80,
    -875,  -875,  -875,  -875,   352,  -875,  -875,  -875,  -875,    74,
    -875,  -875,   305,    94,  -875,  -875,  -875,  -875,   502,  6042,
    6633,  -875,  6368,  -875,   268,  -875,   509,  6042,  -875,   510,
    6260,  -875,   511,   512,   513,   514,   516,   517,   518,   519,
     515,   522,  -875,   520,   453,  4741,   539,   392,   392,  -875,
    -875,  -875,  -875,  -875,  4527,  -875,   524,   453,   523,   525,
     530,  -875,  -875,   531,  -875,  -875,  -875,  -875,  -875,  3768,
     528,   533,  -875,  4527,  -875,  -875,  -875,  -875,  -875,  -875,
    -875,  -875,  -875,  -875,  -875,  4527,   534,  -875,    33,  -875,
    5269,  -875,  -875,  -875,  -875,  -875,   537,  -875,  -875,  -875,
    -875,  -875,  -875,  -875,  -875,  -875,  -875,   650,   661,  -875,
     544,   546,  -875,  -875,  -875,  6042,   453,  -875,   521,  -875,
     550,  6633,  -875,  -875,  -875,  -875,  4290,   -19,     8,     9,
    -875,  3900,  4420,    51,   538,   638,   152,   100,   123,   554,
     558,   559,   560,   561,   584,   585,   590,   593,   594,   596,
     597,   598,  -875,  6633,    30,  -875,    30,  -875,  -875,  4030,
     175,   175,  6633,   175,  -875,  -875,  6473,  -875,  -875,  -875,
     599,  4527,  -875,  -875,  -875,    19,  -875,  -875,  -875,  -875,
     435,   302,  -875,   600,  -875,   602,   603,   604,  -875,  -875,
     592,   601,   608,  -875,   453,  -875,   523,  2844,   101,  6633,
    6633,   316,  4879,  4661,  4879,  -875,  -875,   277,   175,   175,
    6578,  -875,  6633,  -875,  4160,  -875,  -875,  -875,   609,   616,
      30,   175,   175,  -875,  -875,  -875,  -875,  -875,   104,  -875,
    -875,   116,  -875,   268,  -875,   620,  5269,  -875,  -875,  3306,
    -875,  -875,  -875,   614,   619,   623,  -875,   626,  -875,  -875,
    -875,  -875,  -875,  -875,  -875,  -875,  -875,   300,   300,  -875,
     300,  4900,  6633,  4900,  -875,  -875,  -875,  -875,  4290,   628,
    6633,  2998,  -875,  -875,   631,  -875,  -875,  -875,  -875,  -875,
    -875,  -875,  -875,  -875,   539,  -875,  -875,  -875,  3152,  3460,
    -875,  -875,  -875,  -875,   632,  -875,    16,  -875,  4741,  -875,
    -875,  -875,  -875,   633,  3614,  -875,  -875
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -875,  -282,  -875,  -875,  -875,  -875,  -875,   -87,  -875,  -875,
    -875,  -875,  -875,  -875,  -585,  -875,  -875,  -260,    70,  -875,
    -875,   -78,  -875,  -875,  -456,  -875,  -875,  -875,  -722,  -875,
    -875,  -875,  -875,  -875,  -875,  -175,  -663,   -11,  -875,    -8,
    -503,  -875,    40,  -875,  -875,  -875,  -875,  -457,  -875,  -875,
    -875,   -43,  -875,  -875,  -786,  -875,  -875,   -88,   503,  -875,
    -875,  -141,  -875,  -875,  -875,  -875,  -243,  -875,  -396,   -53,
    -875,  -875,  -875,  -875,  -242,  -875,  -875,  -875,  -875,  -132,
     -48,  -497,  -875,  -875,  -527,  -875,  -107,  -875,  -875,  -875,
    -875,   556,  -698,  -875,  -875,    14,  -185,   -80,  -133,  -875,
     250,  -875,  -875,  -813,  -875,  -195,  -875,  -875,  -875,  -211,
    -875,  -875,  -875,  -875,  -771,   -94,  -656,  -875,  -875,  -875,
    -875,  -875,    -4,  -875,  -875,     1,   -35,    26,   -27,   692,
     694,  -875,   -34,  -875,  -875,  -875,  -224,  -875,    97,   -10,
      44,  -102,  -875,   573,  -255,  -284,     7,  -222,  -731,  -875,
     -98,  -875,  -875,  -736,  -875,  -875,  -875,  -875,  -875,  -875,
    -875,  -644,    24,  -875,  -875,  -875,  -875,  -875,  -875,  -875,
    -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,  -875,
    -875,  -875,  -875,  -875,  -875,  -875,   555,  -136,  -874,  -875,
    -832,  -875,  -875,  -875
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -402
static const yytype_int16 yytable[] =
{
     136,   140,   163,   268,   102,   232,   237,   235,   286,   187,
     230,   139,   141,   410,   233,   288,   528,   579,   508,   821,
     292,   763,   619,   623,   700,   822,   154,   157,   536,   892,
     437,   723,   149,   189,   517,   162,   803,   155,   158,   359,
     302,   348,   827,   242,   243,   244,   245,   110,   775,   776,
     777,   236,   496,   507,   307,   918,   520,   221,   614,   679,
     309,   880,   316,   164,   166,   510,   496,   221,   418,   516,
     496,   626,   195,   882,   221,  -335,   575,   944,   576,   725,
     496,   165,  -335,   931,   534,   496,   177,  -337,   775,   776,
     777,   999,   347,   540,   496,   238,  -335,   312,   265,   775,
     776,   777,  -339,  -338,   364,   313,   775,   776,   777,   775,
     776,   777,   726,   823,   824,   409,  -276,   993,   234,   892,
     152,   775,   776,   777,   163,     2,   167,   152,   414,   267,
     357,   357,   191,   991,   187,  1014,  -335,   151,   979,  -339,
     188,   152,   496,  -278,  -277,  -337,   266,   187,   187,   139,
     141,   761,   155,   158,   314,   360,   701,   770,   189,   940,
     222,   420,  -338,   223,  -233,   562,   733,   993,   271,  -266,
     222,   189,   189,   351,  -336,   892,   577,   222,  -340,  -339,
     223,   778,   993,   361,   362,   164,  -274,   195,   771,   297,
     191,   601,   602,   603,  -340,   178,   325,   197,   315,   200,
     329,   330,  -338,   332,   333,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,  -341,  -340,   199,
     311,   782,   349,   179,   180,   583,  -337,   820,   356,   356,
     349,   201,   797,   801,   363,  -279,   535,   187,   187,   960,
     830,   279,   987,   183,   313,   989,  -336,   192,   627,   202,
     908,   969,   363,   187,   988,  -341,   272,   193,  -280,   349,
     580,   189,   189,  1007,   187,   301,  -337,   190,   313,   280,
     281,   191,  1013,  -336,   203,   204,   869,   189,   304,   305,
     971,   972,   205,   184,   185,   186,   513,  -275,   189,   282,
     206,   199,   207,   639,   938,  -341,   273,    33,   941,   902,
     411,   503,   424,   503,  1000,  1001,   425,  1002,   169,   239,
     969,   363,   240,  -336,   198,   503,   441,   973,   199,   503,
     208,   311,   542,   543,   544,   209,   210,   547,   211,   283,
     284,   285,   178,   212,   503,   213,   170,   171,   917,   214,
     215,   216,   521,   503,   217,   218,   349,   219,   594,   595,
     763,   686,   220,   564,   596,   597,   172,   275,   598,   277,
     179,   180,   249,   680,   287,   290,   927,   192,   427,   440,
     293,   178,   496,   496,   496,   496,   496,   957,   731,   296,
     183,   738,   739,   298,   440,   742,   743,   744,   745,   746,
     747,   748,   749,   750,   751,   440,   173,   174,   175,   179,
     180,   178,   884,   657,   658,   659,   660,   661,   662,   299,
     306,   415,   961,  -339,  -338,   181,   349,   182,  -340,   183,
     184,   185,   186,   784,   785,   977,  -341,   346,   353,   179,
     180,   590,   354,   773,   963,  -276,   176,   964,  -276,   781,
    -276,   965,  -276,  -335,   318,  -337,   505,  -278,   528,   183,
    -278,  -336,  -278,   426,  -278,  -266,   531,   541,   514,   184,
     185,   186,   796,   530,   532,   538,   572,   545,   772,   546,
     548,   187,   319,   320,   559,  -277,   549,   550,  -277,   221,
    -277,  -274,  -277,   574,  -274,   808,  -274,   551,  -274,   184,
     185,   186,   321,   569,   560,   189,   786,   787,   552,   788,
     789,   790,   791,   792,   793,   553,  -275,   554,   555,  -275,
     513,  -275,   556,  -275,  -279,   590,   557,  -279,   558,  -279,
     561,  -279,  -280,   625,   625,  -280,   313,  -280,   563,  -280,
     886,   887,   322,   323,   324,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   187,   178,   656,
     657,   658,   659,   660,   661,   662,   565,   650,   566,   187,
     567,   570,   593,   571,   582,   592,   607,   187,   187,   187,
     888,   189,   600,   599,   608,   609,   179,   180,   612,   719,
     610,   611,   222,   189,   613,   223,   187,   615,   616,  -212,
     620,   189,   189,   189,   621,    39,   183,   622,   628,   629,
     923,   633,   606,   720,   663,   664,   665,   666,   513,   718,
     189,   631,   634,   503,   428,   635,   638,   651,   652,   625,
     653,   681,   654,   655,   682,  -271,   678,   721,   724,   697,
     935,   735,   752,   937,   741,   760,   184,   185,   186,   759,
     765,   766,   429,   430,   799,   804,   807,   809,   810,   811,
     812,    34,   813,   814,   815,   816,   819,   817,   736,   829,
     833,   896,   431,   187,   818,   831,   834,   962,   835,   878,
     879,   883,   897,   924,   667,   903,   668,   937,   769,   894,
     898,   590,   899,   669,   670,   496,   905,   189,   925,  -293,
     440,   873,   864,  -294,  -283,  -284,  -285,   907,   694,   695,
     696,   866,   432,   433,   434,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   440,   503,  -286,
    -287,   187,   187,   187,   187,  -288,   590,   496,  -289,  -290,
     719,  -291,  -292,   926,   953,   939,   948,   937,   950,   951,
     952,   955,   974,   982,   956,   189,   189,   189,   189,   719,
     983,   435,  -401,   919,   720,   800,   990,   995,   996,   496,
     718,   719,   997,   805,  1003,  1005,  1011,   774,   867,  1015,
     286,   868,   802,   720,   958,   436,   496,   496,   942,   718,
     876,   932,   930,   906,   928,   720,   929,   630,   946,   358,
     981,   718,   496,   875,   606,  1012,   349,   947,   901,   270,
     269,   352,   954,   877,   408,     0,     0,   163,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   933,   934,     0,
     936,     0,     0,     0,     0,     0,     0,   769,   998,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   920,     0,     0,     0,     0,   719,     0,     0,
     984,     0,     0,     0,     0,   318,     0,     0,     0,   590,
       0,     0,   911,     0,     0,   975,   976,     0,   164,     0,
     920,   720,   265,     0,     0,     0,     0,   718,   985,   986,
       0,     0,     0,   319,   320,     0,   349,     0,     0,     0,
     221,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   321,     0,   590,   363,     0,   967,   968,
     970,     0,     0,     0,     0,     0,   363,     0,   590,   139,
     141,   158,     0,     0,     0,   980,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   349,   322,   323,   324,   123,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,     0,     0,
     187,     0,     0,     0,     0,     0,     0,   967,   968,   970,
       0,     0,     0,     0,   187,     0,   363,     0,     0,     0,
       0,     0,     0,     0,   189,     0,     0,     0,     0,     0,
       0,     0,     0,   222,     0,     0,   223,     0,   189,   442,
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
     489,   490,     0,   491,   492,   392,   393,   493,   394,   395,
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
       0,     0,     0,     0,   489,   490,   509,   491,     0,   392,
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
       0,   491,   515,   392,   393,   493,   394,   395,   396,   397,
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
       0,     0,   489,   490,     0,   491,   529,   392,   393,   493,
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
       0,     0,     0,     0,     0,     0,   489,   490,   533,   491,
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
     489,   490,   539,   491,     0,   392,   393,   493,   394,   395,
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
       0,     0,     0,     0,   489,   490,     0,   491,   568,   392,
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
     687,   491,     0,   392,   393,   493,   394,   395,   396,   397,
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
       0,     0,   489,   490,   688,   491,     0,   392,   393,   493,
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
       0,     0,     0,     0,     0,     0,   489,   490,   689,   491,
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
     489,   490,     0,   491,   690,   392,   393,   493,   394,   395,
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
       0,     0,     0,     0,   489,   490,     0,   491,     0,   392,
     393,   493,   394,   395,   396,   397,   398,   399,   400,   401,
     402,   403,   404,   494,   691,   406,   495,   442,   443,   444,
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
     959,   491,     0,   392,   393,   493,   394,   395,   396,   397,
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
       0,     0,   489,   490,  1004,   491,     0,   392,   393,   493,
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
    1009,   392,   393,   493,   394,   395,   396,   397,   398,   399,
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
       0,   490,   992,   491,     0,   392,   393,   493,   394,   395,
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
       0,     0,     0,     0,  1010,   490,     0,   491,     0,   392,
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
       0,     0,     0,     0,     0,     0,     0,     0,  1016,   490,
       0,   491,     0,   392,   393,   493,   394,   395,   396,   397,
     398,   399,   400,   401,   402,   403,   404,   494,     0,   406,
     495,     4,     5,     0,     0,     0,   836,   837,     0,     0,
       0,     0,     0,     0,     0,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,   838,   839,    30,    31,   840,
       0,    33,     0,     0,    34,    35,    36,   841,   842,    38,
      39,    40,    41,     0,    43,   843,    45,   161,    47,   844,
       0,     0,   845,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   846,   847,   848,   849,   850,
     851,   852,   853,   854,   855,   856,   857,   858,   859,   860,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,   861,   862,   912,   913,     0,     0,     0,   836,   837,
       0,     0,     0,     0,   863,     0,     0,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   838,   839,   145,
     914,     0,     0,    33,     0,     0,    34,    35,    36,     0,
     842,    38,    39,    40,     0,     0,     0,   843,   160,   161,
       0,     0,     0,     0,   845,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   846,   847,   848,
     849,   850,   851,   852,   853,   854,   855,   856,   857,   858,
     859,   860,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     4,     5,     0,     0,     0,   836,   837,
       0,     0,     0,     0,     0,     0,   863,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   838,   839,   145,
      31,     0,     0,    33,     0,     0,    34,    35,    36,     0,
     842,    38,     0,    40,     0,     0,     0,   843,   160,   161,
       0,     0,     0,     0,   845,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   846,   847,   848,
     849,   850,   851,   852,   853,   854,   855,   856,   857,   858,
     859,   860,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   142,   143,     0,     0,     0,   978,   837,
       0,     0,     0,     0,     0,     0,   863,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   838,   839,   145,
     146,     0,     0,    33,     0,     0,     0,    35,    36,     0,
     842,    38,     0,    40,     0,     0,     0,   843,   160,   161,
       0,     0,     0,     0,   845,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   846,   847,   848,
     849,   850,   851,   852,   853,   854,   855,   856,   857,   858,
     859,   860,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   142,   143,     0,     0,     0,     0,   144,
       0,     0,     0,     0,     0,     0,   863,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,   145,
     146,     0,     0,    33,     0,     0,     0,    35,    36,     0,
     159,    38,     0,    40,     0,     0,     0,   147,   160,   161,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   148,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   142,   143,     0,     0,     0,     0,   837,
       0,     0,     0,     0,     0,     0,   909,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   838,   839,   145,
     146,   318,     0,    33,     0,     0,     0,    35,    36,     0,
       0,     0,     0,    40,     0,     0,     0,   843,   160,     0,
       0,     0,     0,     0,   921,     0,     0,     0,     0,   319,
     320,     0,     0,     0,     0,     0,   221,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   321,
       0,     0,     0,     0,     0,     0,     0,   846,   847,   848,
     849,   850,   851,   852,   853,   854,   855,   856,   857,   858,
     859,   860,     0,     0,     0,     0,   702,     0,   703,   704,
     705,     0,   706,   707,     0,     0,     0,   251,   252,   322,
     323,   324,   123,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   247,   248,   922,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   708,     0,     0,     0,     0,     0,
       0,   428,     0,     0,     0,     0,     0,     0,     0,   222,
       0,     0,   223,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   428,     0,     0,     0,     0,     0,     0,   429,
     430,     0,     0,     0,   709,   254,   255,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   431,
     429,   430,     0,     0,     0,     0,   428,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     431,     0,   710,     0,     0,     0,     0,     0,     0,     0,
     116,   711,   712,   713,   429,   430,     0,     0,     0,   432,
     433,   434,   123,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   431,     0,     0,     0,   117,   118,
     432,   433,   434,   123,   124,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,     0,     0,     0,   119,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   519,     0,
       0,     0,     0,     0,   432,   433,   434,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   685,
     178,   656,   657,   658,   659,   660,   661,   662,   120,   121,
     122,   123,   124,   125,   126,   127,   128,   129,   130,   131,
     132,   133,   134,     0,     0,     0,     0,     0,   179,   180,
       0,     0,     0,   730,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   183,     0,
       0,     0,     0,     0,     0,     0,   663,   664,   665,   666,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   142,   143,     0,
       0,     0,     0,   144,     0,     0,     0,     0,   184,   185,
     186,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,   145,   146,     0,     0,    33,     0,     0,
       0,    35,    36,     0,     0,     0,   667,    40,     0,     0,
       0,   147,     0,     0,     0,   669,   670,     0,   116,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   318,
       0,     0,     0,     0,     0,     0,   117,   118,     0,     0,
       0,   148,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,   119,   319,   320,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   321,     0,     0,
       0,   310,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   120,   121,   122,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,     0,     0,     0,     0,     0,     0,   322,   323,   324,
     123,   124,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,     0,   963,     0,     0,   964,     0,     0,     0,
     965,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       4,     5,     0,     0,   963,     6,     7,   964,     0,     0,
       0,   965,     0,     0,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,     0,
      33,     0,     0,    34,    35,    36,     0,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
     116,    85,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   116,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   117,   118,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   117,   118,     0,     0,     0,   119,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   119,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   120,   121,
     122,   123,   124,   125,   126,   127,   128,   129,   130,   131,
     132,   133,   134,   120,   121,   122,   123,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   318,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   135,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   156,     0,     0,   319,   320,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   321,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   886,   887,   322,   323,   324,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     4,     5,     0,     0,     0,     6,   144,     0,     0,
       0,     0,     0,     0,   888,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,   145,    31,     0,
       0,    33,     0,     0,    34,    35,    36,     0,   159,    38,
       0,    40,     0,     0,     0,   147,   160,   161,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   148,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
     142,   143,     0,     0,     0,     0,   144,     0,     0,     0,
       0,     0,     0,     0,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,   145,   146,     0,     0,
      33,     0,     0,     0,    35,    36,     0,   159,    38,     0,
      40,     0,     0,     0,   147,   160,   161,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   148,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,   142,
     143,     0,     0,     0,     0,   144,     0,     0,     0,     0,
       0,     0,     0,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,   145,   146,     0,     0,    33,
       0,     0,     0,    35,    36,     0,     0,     0,     0,    40,
       0,     0,     0,   147,   160,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   148,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,   142,   143,
       0,     0,     0,     0,   144,   194,     0,     0,     0,     0,
       0,     0,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,   145,   146,     0,     0,    33,     0,
       0,     0,    35,    36,     0,     0,     0,     0,    40,     0,
       0,     0,   147,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   148,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,     4,     5,     0,
       0,     0,     0,   144,     0,     0,     0,     0,     0,     0,
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,     0,     0,    33,     0,     0,
       0,    35,    36,     0,     0,     0,     0,    40,     0,     0,
       0,   147,     0,     0,   231,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   148,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,   142,   143,     0,     0,
       0,     0,   144,   295,     0,     0,     0,     0,     0,     0,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,   145,   146,     0,     0,    33,     0,     0,     0,
      35,    36,     0,     0,     0,     0,    40,     0,     0,     0,
     147,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     148,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,   142,   143,     0,     0,     0,
       0,   144,     0,     0,     0,     0,     0,     0,     0,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,   145,   146,     0,     0,    33,     0,     0,     0,    35,
      36,     0,     0,     0,     0,    40,     0,     0,     0,   147,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   148,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,   142,   143,     0,     0,     0,     0,
     144,     0,     0,     0,     0,     0,     0,     0,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
     145,   146,     0,     0,     0,     0,     0,     0,    35,    36,
       0,     0,     0,     0,    40,     0,     0,     0,   147,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   148,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,   142,   143,     0,     0,     0,     0,   279,
       0,     0,     0,     0,     0,     0,     0,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   280,   281,   145,
     146,     0,     0,     0,     0,     0,     0,    35,    36,     0,
       0,     0,     0,     0,     0,     0,     0,   282,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   283,   284,   285,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,   522,     0,     0,     0,     0,   279,     0,     0,
       0,     0,     0,     0,     0,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,   280,   281,   246,     0,     0,
       0,     0,     0,     0,     0,    35,    36,     0,     0,     0,
       0,   523,     0,     0,     0,   282,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   247,   248,     0,     0,     0,
       0,     0,     0,     0,   249,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   250,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   283,   284,   285,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
       0,     0,   318,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   251,   252,   253,   254,   255,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     319,   320,   318,     0,     0,     0,     0,     0,     0,   249,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     321,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     319,   320,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     321,     0,     0,     0,     0,     0,     0,     0,   251,   252,
     322,   323,   324,   123,   124,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,     0,     0,   318,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   251,   252,
     322,   323,   324,   123,   124,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   319,   320,   246,     0,     0,
       0,     0,     0,     0,   249,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   321,     0,     0,     0,     0,
       0,     0,   318,     0,     0,   247,   248,     0,     0,     0,
       0,     0,     0,     0,   249,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   250,     0,     0,     0,     0,
     319,   320,     0,     0,     0,   322,   323,   324,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     321,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   253,   254,   255,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     322,   323,   324,   123,   124,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   365,   366,     0,     0,     0,
       0,     0,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   391,     0,
       0,     0,   392,   393,     0,   394,   395,   396,   397,   398,
     399,   400,   401,   402,   403,   404,   405,     0,   406
};

static const yytype_int16 yycheck[] =
{
       4,     5,    37,   105,     3,    92,    94,    94,   149,    43,
      90,     4,     5,   256,    92,   151,   300,   413,   278,   755,
     156,   677,   519,   526,   609,   756,    30,    31,   310,   800,
     272,   616,     6,    43,   294,    34,   734,    30,    31,   234,
     176,   226,   764,    96,    97,    98,    99,     3,     5,     6,
       7,    94,   274,   277,   190,   841,   298,    44,   514,   586,
     193,   783,   198,    37,    38,   289,   288,    44,   263,   293,
     292,   528,    46,   795,    44,    94,    43,   890,    45,    60,
     302,    37,    94,   869,   308,   307,    42,    94,     5,     6,
       7,   965,   225,   317,   316,    94,    94,   134,   102,     5,
       6,     7,    94,    94,   240,   142,     5,     6,     7,     5,
       6,     7,    93,   757,   758,   256,   135,   949,    92,   890,
     139,     5,     6,     7,   159,     0,   139,   139,   261,   103,
     232,   233,   139,   946,   168,  1009,   134,   135,   924,    94,
      43,   139,   364,   135,   135,    94,   102,   181,   182,   142,
     143,   135,   145,   146,   197,   235,   612,   684,   168,   881,
     147,   263,    94,   150,   134,   350,   623,   999,    94,   153,
     147,   181,   182,   150,    94,   946,   143,   147,    94,   134,
     150,   138,  1014,   236,   237,   159,   135,   161,   685,   163,
     139,     5,     6,     7,    94,     9,   200,   135,   197,   135,
     204,   205,   134,   207,   208,   209,   210,   211,   212,   213,
     214,   215,   216,   217,   218,   219,   220,    94,   134,   139,
     194,   138,   226,    37,    38,   420,    94,   754,   232,   233,
     234,   135,   138,   730,   238,   135,   134,   271,   272,   138,
     767,     9,   138,    57,   142,   943,    94,    50,   530,   135,
     835,   914,   256,   287,   138,    94,    94,    60,   135,   263,
     134,   271,   272,   994,   298,   168,   134,   135,   142,    37,
      38,   139,  1008,    94,   135,   135,   779,   287,   181,   182,
       3,     4,   135,    97,    98,    99,   290,   135,   298,    57,
     135,   139,   135,   548,   879,   134,   134,    43,   883,   826,
     256,   275,   135,   277,   967,   968,   139,   970,     9,   134,
     973,   315,   137,   134,   135,   289,   272,    40,   139,   293,
     135,   295,   326,   327,   328,   135,   135,   331,   135,    97,
      98,    99,     9,   135,   308,   135,    37,    38,   841,   135,
     135,   135,   298,   317,   135,   135,   350,   135,   484,   485,
    1006,   593,   135,   357,   490,   491,    57,   139,   494,   139,
      37,    38,    46,   587,    94,   137,   863,    50,   271,   272,
     139,     9,   594,   595,   596,   597,   598,   904,   620,   135,
      57,   636,   637,    94,   287,   640,   641,   642,   643,   644,
     645,   646,   647,   648,   649,   298,    97,    98,    99,    37,
      38,     9,   798,    11,    12,    13,    14,    15,    16,   140,
     134,   143,   909,    94,    94,    53,   420,    55,    94,    57,
      97,    98,    99,    71,    72,   922,    94,   134,   134,    37,
      38,   435,   134,   693,   134,   134,   137,   137,   137,   699,
     139,   141,   141,    94,     9,    94,   141,   134,   732,    57,
     137,    94,   139,    94,   141,   153,   134,   142,   137,    97,
      98,    99,   722,   137,   134,   136,   154,   142,   692,   142,
     142,   505,    37,    38,   136,   134,   142,   142,   137,    44,
     139,   134,   141,    94,   137,   740,   139,   142,   141,    97,
      98,    99,    57,   153,   136,   505,   144,   145,   142,   147,
     148,   149,   150,   151,   152,   142,   134,   142,   142,   137,
     514,   139,   142,   141,   134,   519,   142,   137,   142,   139,
     142,   141,   134,   527,   528,   137,   142,   139,   134,   141,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   581,     9,    10,
      11,    12,    13,    14,    15,    16,   134,   561,   134,   593,
     134,   153,    94,   136,   136,   135,   137,   601,   602,   603,
     135,   581,   140,   142,   140,   137,    37,    38,   142,   613,
     140,   138,   147,   593,   143,   150,   620,   140,   137,   136,
      94,   601,   602,   603,   140,    52,    57,   142,   140,   138,
     842,   142,   505,   613,    65,    66,    67,    68,   612,   613,
     620,   140,   136,   587,     9,   136,   142,   154,   154,   623,
     135,   135,    16,    16,   141,   153,   153,   138,   136,   142,
     872,   138,   142,   876,   136,    10,    97,    98,    99,   139,
     136,   140,    37,    38,   142,   136,   136,   136,   136,   136,
     136,    46,   136,   136,   136,   136,   136,   142,   632,   135,
     135,    11,    57,   697,   142,   142,   136,   910,   137,   141,
     137,   137,    11,   135,   135,   154,   137,   920,   682,   142,
     136,   685,   136,   144,   145,   907,   136,   697,    50,   135,
     593,   779,   779,   135,   135,   135,   135,   833,   601,   602,
     603,   779,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   620,   692,   135,
     135,   755,   756,   757,   758,   135,   730,   949,   135,   135,
     764,   135,   135,   135,   142,   136,   136,   980,   136,   136,
     136,   140,   917,   134,   136,   755,   756,   757,   758,   783,
     134,   146,   138,   841,   764,   729,   136,   138,   135,   981,
     764,   795,   136,   737,   136,   134,   134,   697,   779,   136,
     911,   779,   732,   783,   906,   272,   998,   999,   885,   783,
     779,   869,   869,   831,   864,   795,   866,   537,   890,   233,
     926,   795,  1014,   779,   697,  1006,   800,   891,   825,   107,
     106,   228,   900,   779,   249,    -1,    -1,   842,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   870,   871,    -1,
     873,    -1,    -1,    -1,    -1,    -1,    -1,   831,   964,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   841,    -1,    -1,    -1,    -1,   881,    -1,    -1,
     930,    -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,   863,
      -1,    -1,   836,    -1,    -1,   918,   919,    -1,   842,    -1,
     869,   881,   876,    -1,    -1,    -1,    -1,   881,   931,   932,
      -1,    -1,    -1,    37,    38,    -1,   890,    -1,    -1,    -1,
      44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    57,    -1,   909,   910,    -1,   912,   913,
     914,    -1,    -1,    -1,    -1,    -1,   920,    -1,   922,   912,
     913,   914,    -1,    -1,    -1,   924,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   946,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,    -1,    -1,
     994,    -1,    -1,    -1,    -1,    -1,    -1,   971,   972,   973,
      -1,    -1,    -1,    -1,  1008,    -1,   980,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   994,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   147,    -1,    -1,   150,    -1,  1008,     3,
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
      -1,    -1,    -1,    -1,   134,   135,    -1,   137,    -1,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   152,   153,   154,   155,   156,     3,     4,     5,
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
      -1,   135,   136,   137,    -1,   139,   140,   141,   142,   143,
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
      -1,    -1,    -1,    -1,   134,   135,    -1,   137,    -1,   139,
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
     156,     3,     4,    -1,    -1,    -1,     8,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      -1,    43,    -1,    -1,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    -1,    56,    57,    58,    59,    60,    61,
      -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,   128,   129,   130,   131,
     132,   133,   134,     3,     4,    -1,    -1,    -1,     8,     9,
      -1,    -1,    -1,    -1,   146,    -1,    -1,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    -1,    -1,    43,    -1,    -1,    46,    47,    48,    -1,
      50,    51,    52,    53,    -1,    -1,    -1,    57,    58,    59,
      -1,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     3,     4,    -1,    -1,    -1,     8,     9,
      -1,    -1,    -1,    -1,    -1,    -1,   146,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    -1,    -1,    43,    -1,    -1,    46,    47,    48,    -1,
      50,    51,    -1,    53,    -1,    -1,    -1,    57,    58,    59,
      -1,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     3,     4,    -1,    -1,    -1,     8,     9,
      -1,    -1,    -1,    -1,    -1,    -1,   146,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    -1,    -1,    43,    -1,    -1,    -1,    47,    48,    -1,
      50,    51,    -1,    53,    -1,    -1,    -1,    57,    58,    59,
      -1,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     3,     4,    -1,    -1,    -1,    -1,     9,
      -1,    -1,    -1,    -1,    -1,    -1,   146,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    -1,    -1,    43,    -1,    -1,    -1,    47,    48,    -1,
      50,    51,    -1,    53,    -1,    -1,    -1,    57,    58,    59,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     3,     4,    -1,    -1,    -1,    -1,     9,
      -1,    -1,    -1,    -1,    -1,    -1,   146,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,     9,    -1,    43,    -1,    -1,    -1,    47,    48,    -1,
      -1,    -1,    -1,    53,    -1,    -1,    -1,    57,    58,    -1,
      -1,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    37,
      38,    -1,    -1,    -1,    -1,    -1,    44,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    57,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,    -1,    -1,    -1,    -1,     9,    -1,    11,    12,
      13,    -1,    15,    16,    -1,    -1,    -1,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,    37,    38,   146,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    57,    -1,    -1,    -1,    -1,    -1,
      -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   147,
      -1,    -1,   150,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    37,
      38,    -1,    -1,    -1,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,    57,
      37,    38,    -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      57,    -1,   135,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       9,   144,   145,   146,    37,    38,    -1,    -1,    -1,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,    57,    -1,    -1,    -1,    37,    38,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,    -1,    -1,    -1,    57,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   146,    -1,
      -1,    -1,    -1,    -1,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   146,
       9,    10,    11,    12,    13,    14,    15,    16,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,    -1,    -1,    -1,    -1,    -1,    37,    38,
      -1,    -1,    -1,   146,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    57,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    65,    66,    67,    68,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,    -1,
      -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,    97,    98,
      99,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    -1,    -1,    43,    -1,    -1,
      -1,    47,    48,    -1,    -1,    -1,   135,    53,    -1,    -1,
      -1,    57,    -1,    -1,    -1,   144,   145,    -1,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    37,    38,    -1,    -1,
      -1,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,    57,    37,    38,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    57,    -1,    -1,
      -1,   137,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,    -1,    -1,    -1,    -1,    -1,    -1,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,    -1,   134,    -1,    -1,   137,    -1,    -1,    -1,
     141,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       3,     4,    -1,    -1,   134,     8,     9,   137,    -1,    -1,
      -1,   141,    -1,    -1,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    -1,
      43,    -1,    -1,    46,    47,    48,    -1,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,   128,   129,   130,   131,   132,
       9,   134,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    37,    38,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    37,    38,    -1,    -1,    -1,    57,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    57,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   137,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   137,    -1,    -1,    37,    38,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    57,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     3,     4,    -1,    -1,    -1,     8,     9,    -1,    -1,
      -1,    -1,    -1,    -1,   135,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    -1,
      -1,    43,    -1,    -1,    46,    47,    48,    -1,    50,    51,
      -1,    53,    -1,    -1,    -1,    57,    58,    59,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
       3,     4,    -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    -1,    -1,
      43,    -1,    -1,    -1,    47,    48,    -1,    50,    51,    -1,
      53,    -1,    -1,    -1,    57,    58,    59,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,     3,
       4,    -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    -1,    -1,    43,
      -1,    -1,    -1,    47,    48,    -1,    -1,    -1,    -1,    53,
      -1,    -1,    -1,    57,    58,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,     3,     4,
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
     105,   106,   107,   108,   109,   110,   111,     3,     4,    -1,
      -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    -1,    -1,    43,    -1,    -1,
      -1,    47,    48,    -1,    -1,    -1,    -1,    53,    -1,    -1,
      -1,    57,    -1,    -1,    60,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,     3,     4,    -1,    -1,
      -1,    -1,     9,    10,    -1,    -1,    -1,    -1,    -1,    -1,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    -1,    -1,    43,    -1,    -1,    -1,
      47,    48,    -1,    -1,    -1,    -1,    53,    -1,    -1,    -1,
      57,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,     3,     4,    -1,    -1,    -1,
      -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,
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
      39,    40,    -1,    -1,    -1,    -1,    -1,    -1,    47,    48,
      -1,    -1,    -1,    -1,    53,    -1,    -1,    -1,    57,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,     3,     4,    -1,    -1,    -1,    -1,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    -1,    -1,    -1,    -1,    -1,    -1,    47,    48,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    57,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,     4,    -1,    -1,    -1,    -1,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    47,    48,    -1,    -1,    -1,
      -1,    53,    -1,    -1,    -1,    57,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    37,    38,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    46,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    57,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
      -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
      37,    38,     9,    -1,    -1,    -1,    -1,    -1,    -1,    46,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      57,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      37,    38,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      57,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,    -1,    -1,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,    37,    38,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    46,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    57,    -1,    -1,    -1,    -1,
      -1,    -1,     9,    -1,    -1,    37,    38,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    46,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    57,    -1,    -1,    -1,    -1,
      37,    38,    -1,    -1,    -1,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
      57,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,    62,    63,    -1,    -1,    -1,
      -1,    -1,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   135,    -1,
      -1,    -1,   139,   140,    -1,   142,   143,   144,   145,   146,
     147,   148,   149,   150,   151,   152,   153,    -1,   155
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
     215,   252,   282,   283,   284,   285,   286,   287,   289,   296,
     297,   301,   302,   303,   304,   319,     9,    37,    38,    57,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   137,   279,   280,   281,   303,
     279,   303,     3,     4,     9,    39,    40,    57,    97,   284,
     297,   135,   139,   180,   279,   303,   137,   279,   303,    50,
      58,    59,   282,   283,   284,   297,   284,   139,   288,     9,
      37,    38,    57,    97,    98,    99,   137,   297,     9,    37,
      38,    53,    55,    57,    97,    98,    99,   289,   295,   296,
     135,   139,    50,    60,    10,   284,   255,   135,   135,   139,
     135,   135,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   135,   135,   135,   135,   135,   135,
     135,    44,   147,   150,   254,   257,   258,   298,   299,   300,
     254,    60,   164,   178,   284,   164,   208,   214,   282,   134,
     137,   226,   226,   226,   226,   226,     9,    37,    38,    46,
      57,    95,    96,    97,    98,    99,   210,   218,   220,   223,
     227,   253,   262,   265,   271,   279,   297,   284,   298,   287,
     286,    94,    94,   134,   344,   139,   167,   139,   165,     9,
      37,    38,    57,    97,    98,    99,   218,    94,   344,   291,
     137,   179,   344,   139,   169,    10,   135,   284,    94,   140,
     198,   295,   344,   163,   295,   295,   134,   344,   290,   255,
     137,   284,   134,   142,   208,   282,   344,   292,     9,    37,
      38,    57,    97,    98,    99,   279,   321,   324,   325,   279,
     279,   329,   279,   279,   279,   279,   279,   279,   279,   279,
     279,   279,   279,   279,   279,   279,   134,   255,   253,   279,
     259,   150,   300,   134,   134,   248,   279,   298,   248,   262,
     254,   226,   226,   279,   344,    62,    63,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,   135,   139,   140,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,   153,   155,   342,   343,   218,
     223,   297,   219,   224,   255,   143,   249,   250,   262,   269,
     298,   272,   273,   274,   135,   139,    94,   295,     9,    37,
      38,    57,    97,    98,    99,   146,   215,   231,   233,   279,
     295,   297,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    50,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    62,    63,    65,    66,
      67,    68,    93,    94,    95,    96,    97,    98,    99,   134,
     135,   137,   138,   141,   153,   156,   304,   343,   346,   347,
     348,   349,   350,   284,   293,   141,   174,   293,   174,   136,
     293,   181,   182,   279,   137,   138,   293,   174,   216,   146,
     231,   297,     4,    53,   199,   201,   202,   203,   302,   138,
     137,   134,   134,   136,   293,   134,   158,   256,   136,   136,
     293,   142,   279,   279,   279,   142,   142,   279,   142,   142,
     142,   142,   142,   142,   142,   142,   142,   142,   142,   136,
     136,   142,   253,   134,   279,   134,   134,   134,   138,   153,
     153,   136,   154,   221,    94,    43,    45,   143,   225,   225,
     134,   251,   136,   262,   275,   277,   228,   229,   238,   239,
     279,   232,   135,    94,   344,   344,   344,   344,   344,   142,
     140,     5,     6,     7,   175,   176,   295,   137,   140,   137,
     140,   138,   142,   143,   181,   140,   137,   241,   242,   238,
      94,   140,   142,   197,   204,   279,   204,   158,   140,   138,
     257,   140,   320,   142,   136,   136,   326,   328,   142,   301,
     332,   334,   336,   338,   333,   335,   337,   339,   340,   341,
     279,   154,   154,   135,    16,    16,    10,    11,    12,    13,
      14,    15,    16,    65,    66,    67,    68,   135,   137,   144,
     145,   295,   305,   310,   316,   317,   318,   263,   153,   241,
     293,   135,   141,   235,   234,   146,   231,   136,   136,   136,
     138,   154,   294,   168,   295,   295,   295,   142,   171,   166,
     171,   181,     9,    11,    12,    13,    15,    16,    57,    97,
     135,   144,   145,   146,   183,   184,   185,   189,   279,   289,
     296,   138,   170,   171,   136,    60,    93,   243,   245,   246,
     146,   231,   200,   204,   205,   138,   284,   322,   301,   301,
     330,   136,   301,   301,   301,   301,   301,   301,   301,   301,
     301,   301,   142,   307,   222,   313,   306,   312,   311,   139,
      10,   135,   266,   273,   276,   136,   140,   240,   237,   279,
     241,   238,   293,   174,   175,     5,     6,     7,   138,   172,
     177,   174,   138,   188,    71,    72,   144,   145,   147,   148,
     149,   150,   151,   152,   190,   186,   174,   138,   217,   142,
     284,   238,   199,   249,   136,   284,   327,   136,   301,   136,
     136,   136,   136,   136,   136,   136,   136,   142,   142,   136,
     241,   310,   305,   318,   318,   314,   267,   185,   278,   135,
     241,   142,   236,   135,   136,   137,     8,     9,    37,    38,
      41,    49,    50,    57,    61,    64,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   133,   134,   146,   164,   173,   178,   194,   196,   197,
     207,   211,   212,   214,   231,   252,   282,   319,   141,   137,
     185,   187,   185,   137,   225,   244,    95,    96,   135,   260,
     264,   270,   271,   323,   142,   331,    11,    11,   136,   136,
     308,   285,   241,   154,   230,   136,   237,   344,   171,   146,
     282,   284,     3,     4,    40,   191,   192,   197,   211,   214,
     282,    64,   146,   231,   135,    50,   135,   238,   254,   254,
     164,   211,   214,   226,   226,   231,   226,   223,   171,   136,
     185,   171,   243,   247,   260,   268,   298,   272,   136,   345,
     136,   136,   136,   142,   307,   140,   136,   241,   236,   136,
     138,   238,   223,   134,   137,   141,   193,   279,   279,   193,
     279,     3,     4,    40,   192,   226,   226,   238,     8,   211,
     282,   344,   134,   134,   254,   226,   226,   138,   138,   249,
     136,   260,   136,   347,   309,   138,   135,   136,   344,   345,
     193,   193,   193,   136,   136,   134,   261,   305,   315,   138,
     134,   134,   266,   310,   345,   136,   134
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
     currentClass = NULL; reject_function(); currentClass = tmpc; }
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
     currentClass = NULL; reject_function(); currentClass = tmpc; }
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
    {access_level = VTK_ACCESS_PROTECTED;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 1438 "vtkParse.y"
    {access_level = VTK_ACCESS_PRIVATE;}
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

  case 253:

/* Line 1455 of yacc.c  */
#line 1962 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 1963 "vtkParse.y"
    { pushFunction(); postSig("("); }
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 1964 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_FUNCTION; postSig(")"); popFunction(); }
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 1965 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_ARRAY; }
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 1968 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 1970 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(1) - (2)].integer), (yyvsp[(2) - (2)].integer));}
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 1973 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 1975 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(1) - (2)].integer), (yyvsp[(2) - (2)].integer));}
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 1977 "vtkParse.y"
    {clearVarName(); chopSig();}
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 1979 "vtkParse.y"
    {setVarName((yyvsp[(1) - (1)].str));}
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 1981 "vtkParse.y"
    {clearArray();}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 1983 "vtkParse.y"
    {clearArray();}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 1985 "vtkParse.y"
    {postSig("[");}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 1985 "vtkParse.y"
    {postSig("]");}
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 1989 "vtkParse.y"
    {pushArraySize("");}
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 1990 "vtkParse.y"
    {pushArraySize((yyvsp[(1) - (1)].str));}
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 1996 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 1997 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
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
#line 2007 "vtkParse.y"
    {(yyval.str) = "vtkTypeInt8";}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 2008 "vtkParse.y"
    {(yyval.str) = "vtkTypeUInt8";}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 2009 "vtkParse.y"
    {(yyval.str) = "vtkTypeInt16";}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 2010 "vtkParse.y"
    {(yyval.str) = "vtkTypeUInt16";}
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 2011 "vtkParse.y"
    {(yyval.str) = "vtkTypeInt32";}
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 2012 "vtkParse.y"
    {(yyval.str) = "vtkTypeUInt32";}
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 2013 "vtkParse.y"
    {(yyval.str) = "vtkTypeInt64";}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 2014 "vtkParse.y"
    {(yyval.str) = "vtkTypeUInt64";}
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 2015 "vtkParse.y"
    {(yyval.str) = "vtkTypeFloat32";}
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 2016 "vtkParse.y"
    {(yyval.str) = "vtkTypeFloat64";}
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 2019 "vtkParse.y"
    {(yyval.str) = "vtkIdType";}
    break;

  case 294:

/* Line 1455 of yacc.c  */
#line 2020 "vtkParse.y"
    {(yyval.str) = "vtkFloatingPointType";}
    break;

  case 295:

/* Line 1455 of yacc.c  */
#line 2027 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer); setStorageType((yyval.integer));}
    break;

  case 296:

/* Line 1455 of yacc.c  */
#line 2028 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 2029 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 2031 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(3) - (3)].integer); setStorageType((yyval.integer));}
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 2032 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 2033 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer));
      setStorageType((yyval.integer));}
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 2035 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(2) - (3)].integer) | (yyvsp[(3) - (3)].integer));
      setStorageType((yyval.integer));}
    break;

  case 302:

/* Line 1455 of yacc.c  */
#line 2039 "vtkParse.y"
    {postSig("static "); (yyval.integer) = VTK_PARSE_STATIC; }
    break;

  case 303:

/* Line 1455 of yacc.c  */
#line 2040 "vtkParse.y"
    {postSig("static "); (yyval.integer) = VTK_PARSE_STATIC; }
    break;

  case 304:

/* Line 1455 of yacc.c  */
#line 2042 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 305:

/* Line 1455 of yacc.c  */
#line 2043 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer));}
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 2045 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 307:

/* Line 1455 of yacc.c  */
#line 2046 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_CONST | (yyvsp[(2) - (2)].integer));}
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 2047 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_CONST | (yyvsp[(1) - (2)].integer));}
    break;

  case 309:

/* Line 1455 of yacc.c  */
#line 2049 "vtkParse.y"
    {postSig("const ");}
    break;

  case 311:

/* Line 1455 of yacc.c  */
#line 2053 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 312:

/* Line 1455 of yacc.c  */
#line 2055 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 313:

/* Line 1455 of yacc.c  */
#line 2056 "vtkParse.y"
    {postSig("typename ");}
    break;

  case 314:

/* Line 1455 of yacc.c  */
#line 2057 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(3) - (3)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 315:

/* Line 1455 of yacc.c  */
#line 2060 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 316:

/* Line 1455 of yacc.c  */
#line 2061 "vtkParse.y"
    {chopSig(); if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 317:

/* Line 1455 of yacc.c  */
#line 2063 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 318:

/* Line 1455 of yacc.c  */
#line 2064 "vtkParse.y"
    {chopSig(); if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 2066 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 2067 "vtkParse.y"
    {chopSig(); if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
      postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 322:

/* Line 1455 of yacc.c  */
#line 2070 "vtkParse.y"
    {postSig(", ");}
    break;

  case 324:

/* Line 1455 of yacc.c  */
#line 2072 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 325:

/* Line 1455 of yacc.c  */
#line 2073 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 326:

/* Line 1455 of yacc.c  */
#line 2074 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 2075 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 2076 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 329:

/* Line 1455 of yacc.c  */
#line 2077 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 2078 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 333:

/* Line 1455 of yacc.c  */
#line 2083 "vtkParse.y"
    {
             (yyval.str) = vtkstrcat3((yyvsp[(1) - (3)].str), "::", (yyvsp[(3) - (3)].str));
             preScopeSig((yyvsp[(1) - (3)].str));
           }
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 2088 "vtkParse.y"
    {
             (yyval.str) = vtkstrcat3((yyvsp[(1) - (3)].str), "::", (yyvsp[(3) - (3)].str));
             preScopeSig("");
           }
    break;

  case 342:

/* Line 1455 of yacc.c  */
#line 2110 "vtkParse.y"
    { postSig("&"); (yyval.integer) = VTK_PARSE_REF;}
    break;

  case 343:

/* Line 1455 of yacc.c  */
#line 2111 "vtkParse.y"
    { postSig("&"); (yyval.integer) = ((yyvsp[(1) - (2)].integer) | VTK_PARSE_REF);}
    break;

  case 344:

/* Line 1455 of yacc.c  */
#line 2112 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 345:

/* Line 1455 of yacc.c  */
#line 2117 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 346:

/* Line 1455 of yacc.c  */
#line 2119 "vtkParse.y"
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

  case 347:

/* Line 1455 of yacc.c  */
#line 2130 "vtkParse.y"
    { postSig("*"); (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 348:

/* Line 1455 of yacc.c  */
#line 2131 "vtkParse.y"
    { postSig("*const "); (yyval.integer) = VTK_PARSE_CONST_POINTER; }
    break;

  case 349:

/* Line 1455 of yacc.c  */
#line 2134 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 350:

/* Line 1455 of yacc.c  */
#line 2135 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); }
    break;

  case 351:

/* Line 1455 of yacc.c  */
#line 2136 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); }
    break;

  case 352:

/* Line 1455 of yacc.c  */
#line 2137 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 353:

/* Line 1455 of yacc.c  */
#line 2138 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 354:

/* Line 1455 of yacc.c  */
#line 2141 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 355:

/* Line 1455 of yacc.c  */
#line 2142 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 356:

/* Line 1455 of yacc.c  */
#line 2145 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_STRING;}
    break;

  case 357:

/* Line 1455 of yacc.c  */
#line 2146 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNICODE_STRING;}
    break;

  case 358:

/* Line 1455 of yacc.c  */
#line 2147 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_OSTREAM; }
    break;

  case 359:

/* Line 1455 of yacc.c  */
#line 2148 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_ISTREAM; }
    break;

  case 360:

/* Line 1455 of yacc.c  */
#line 2149 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 361:

/* Line 1455 of yacc.c  */
#line 2150 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_OBJECT; }
    break;

  case 362:

/* Line 1455 of yacc.c  */
#line 2151 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_QOBJECT; }
    break;

  case 363:

/* Line 1455 of yacc.c  */
#line 2154 "vtkParse.y"
    { typeSig("void"); (yyval.integer) = VTK_PARSE_VOID;}
    break;

  case 364:

/* Line 1455 of yacc.c  */
#line 2155 "vtkParse.y"
    { typeSig("bool"); (yyval.integer) = VTK_PARSE_BOOL;}
    break;

  case 365:

/* Line 1455 of yacc.c  */
#line 2156 "vtkParse.y"
    { typeSig("ssize_t"); (yyval.integer) = VTK_PARSE_SSIZE_T;}
    break;

  case 366:

/* Line 1455 of yacc.c  */
#line 2157 "vtkParse.y"
    { typeSig("size_t"); (yyval.integer) = VTK_PARSE_SIZE_T;}
    break;

  case 367:

/* Line 1455 of yacc.c  */
#line 2158 "vtkParse.y"
    { typeSig("vtkTypeInt8"); (yyval.integer) = VTK_PARSE_INT8; }
    break;

  case 368:

/* Line 1455 of yacc.c  */
#line 2159 "vtkParse.y"
    { typeSig("vtkTypeUInt8"); (yyval.integer) = VTK_PARSE_UINT8; }
    break;

  case 369:

/* Line 1455 of yacc.c  */
#line 2160 "vtkParse.y"
    { typeSig("vtkTypeInt16"); (yyval.integer) = VTK_PARSE_INT16; }
    break;

  case 370:

/* Line 1455 of yacc.c  */
#line 2161 "vtkParse.y"
    { typeSig("vtkTypeUInt16"); (yyval.integer) = VTK_PARSE_UINT16; }
    break;

  case 371:

/* Line 1455 of yacc.c  */
#line 2162 "vtkParse.y"
    { typeSig("vtkTypeInt32"); (yyval.integer) = VTK_PARSE_INT32; }
    break;

  case 372:

/* Line 1455 of yacc.c  */
#line 2163 "vtkParse.y"
    { typeSig("vtkTypeUInt32"); (yyval.integer) = VTK_PARSE_UINT32; }
    break;

  case 373:

/* Line 1455 of yacc.c  */
#line 2164 "vtkParse.y"
    { typeSig("vtkTypeInt64"); (yyval.integer) = VTK_PARSE_INT64; }
    break;

  case 374:

/* Line 1455 of yacc.c  */
#line 2165 "vtkParse.y"
    { typeSig("vtkTypeUInt64"); (yyval.integer) = VTK_PARSE_UINT64; }
    break;

  case 375:

/* Line 1455 of yacc.c  */
#line 2166 "vtkParse.y"
    { typeSig("vtkTypeFloat32"); (yyval.integer) = VTK_PARSE_FLOAT32; }
    break;

  case 376:

/* Line 1455 of yacc.c  */
#line 2167 "vtkParse.y"
    { typeSig("vtkTypeFloat64"); (yyval.integer) = VTK_PARSE_FLOAT64; }
    break;

  case 377:

/* Line 1455 of yacc.c  */
#line 2168 "vtkParse.y"
    { typeSig("vtkIdType"); (yyval.integer) = VTK_PARSE_ID_TYPE;}
    break;

  case 378:

/* Line 1455 of yacc.c  */
#line 2169 "vtkParse.y"
    { typeSig("double"); (yyval.integer) = VTK_PARSE_DOUBLE;}
    break;

  case 379:

/* Line 1455 of yacc.c  */
#line 2170 "vtkParse.y"
    { typeSig("float"); (yyval.integer) = VTK_PARSE_FLOAT;}
    break;

  case 380:

/* Line 1455 of yacc.c  */
#line 2171 "vtkParse.y"
    { typeSig("double"); (yyval.integer) = VTK_PARSE_DOUBLE;}
    break;

  case 381:

/* Line 1455 of yacc.c  */
#line 2172 "vtkParse.y"
    { typeSig("long double"); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 382:

/* Line 1455 of yacc.c  */
#line 2173 "vtkParse.y"
    { typeSig("char"); (yyval.integer) = VTK_PARSE_CHAR;}
    break;

  case 383:

/* Line 1455 of yacc.c  */
#line 2174 "vtkParse.y"
    { typeSig("signed char"); (yyval.integer) = VTK_PARSE_SIGNED_CHAR;}
    break;

  case 384:

/* Line 1455 of yacc.c  */
#line 2176 "vtkParse.y"
    { typeSig("unsigned char"); (yyval.integer) = VTK_PARSE_UNSIGNED_CHAR;}
    break;

  case 385:

/* Line 1455 of yacc.c  */
#line 2177 "vtkParse.y"
    { typeSig("int"); (yyval.integer) = VTK_PARSE_INT;}
    break;

  case 386:

/* Line 1455 of yacc.c  */
#line 2179 "vtkParse.y"
    { typeSig("unsigned int"); (yyval.integer) = VTK_PARSE_UNSIGNED_INT;}
    break;

  case 387:

/* Line 1455 of yacc.c  */
#line 2180 "vtkParse.y"
    { typeSig("short"); (yyval.integer) = VTK_PARSE_SHORT;}
    break;

  case 388:

/* Line 1455 of yacc.c  */
#line 2182 "vtkParse.y"
    { typeSig("unsigned short"); (yyval.integer) = VTK_PARSE_UNSIGNED_SHORT;}
    break;

  case 389:

/* Line 1455 of yacc.c  */
#line 2183 "vtkParse.y"
    { typeSig("long"); (yyval.integer) = VTK_PARSE_LONG;}
    break;

  case 390:

/* Line 1455 of yacc.c  */
#line 2185 "vtkParse.y"
    { typeSig("unsigned long"); (yyval.integer) = VTK_PARSE_UNSIGNED_LONG;}
    break;

  case 391:

/* Line 1455 of yacc.c  */
#line 2186 "vtkParse.y"
    { typeSig("long long"); (yyval.integer) = VTK_PARSE_LONG_LONG;}
    break;

  case 392:

/* Line 1455 of yacc.c  */
#line 2188 "vtkParse.y"
    {typeSig("unsigned long long");(yyval.integer)=VTK_PARSE_UNSIGNED_LONG_LONG;}
    break;

  case 393:

/* Line 1455 of yacc.c  */
#line 2189 "vtkParse.y"
    { typeSig("__int64"); (yyval.integer) = VTK_PARSE___INT64;}
    break;

  case 394:

/* Line 1455 of yacc.c  */
#line 2191 "vtkParse.y"
    { typeSig("unsigned __int64"); (yyval.integer) = VTK_PARSE_UNSIGNED___INT64;}
    break;

  case 395:

/* Line 1455 of yacc.c  */
#line 2192 "vtkParse.y"
    { typeSig("int"); (yyval.integer) = VTK_PARSE_INT; }
    break;

  case 396:

/* Line 1455 of yacc.c  */
#line 2193 "vtkParse.y"
    { typeSig("unsigned int"); (yyval.integer) = VTK_PARSE_UNSIGNED_INT; }
    break;

  case 397:

/* Line 1455 of yacc.c  */
#line 2199 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 398:

/* Line 1455 of yacc.c  */
#line 2200 "vtkParse.y"
    { postSig("{ "); }
    break;

  case 399:

/* Line 1455 of yacc.c  */
#line 2201 "vtkParse.y"
    {
          postSig("}");
          (yyval.str) = vtkstrcat4("{ ", (yyvsp[(3) - (6)].str), (yyvsp[(4) - (6)].str), " }");
        }
    break;

  case 402:

/* Line 1455 of yacc.c  */
#line 2208 "vtkParse.y"
    {(yyval.str) = "";}
    break;

  case 403:

/* Line 1455 of yacc.c  */
#line 2209 "vtkParse.y"
    { postSig(", "); }
    break;

  case 404:

/* Line 1455 of yacc.c  */
#line 2210 "vtkParse.y"
    {
          (yyval.str) = vtkstrcat3((yyvsp[(1) - (4)].str), ", ", (yyvsp[(4) - (4)].str));
        }
    break;

  case 405:

/* Line 1455 of yacc.c  */
#line 2214 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 406:

/* Line 1455 of yacc.c  */
#line 2215 "vtkParse.y"
    {postSig("+");}
    break;

  case 407:

/* Line 1455 of yacc.c  */
#line 2215 "vtkParse.y"
    {(yyval.str) = (yyvsp[(3) - (3)].str);}
    break;

  case 408:

/* Line 1455 of yacc.c  */
#line 2216 "vtkParse.y"
    {postSig("-");}
    break;

  case 409:

/* Line 1455 of yacc.c  */
#line 2217 "vtkParse.y"
    {
             (yyval.str) = vtkstrcat("-", (yyvsp[(3) - (3)].str));
             }
    break;

  case 410:

/* Line 1455 of yacc.c  */
#line 2220 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 411:

/* Line 1455 of yacc.c  */
#line 2221 "vtkParse.y"
    {postSig("(");}
    break;

  case 412:

/* Line 1455 of yacc.c  */
#line 2221 "vtkParse.y"
    {postSig(")"); (yyval.str) = (yyvsp[(3) - (4)].str);}
    break;

  case 413:

/* Line 1455 of yacc.c  */
#line 2222 "vtkParse.y"
    {postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 414:

/* Line 1455 of yacc.c  */
#line 2224 "vtkParse.y"
    {
             chopSig();
             if (getSig()[strlen(getSig())-1] == '>') { postSig(" "); }
             postSig(">(");
             }
    break;

  case 415:

/* Line 1455 of yacc.c  */
#line 2230 "vtkParse.y"
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

  case 416:

/* Line 1455 of yacc.c  */
#line 2244 "vtkParse.y"
    { (yyval.str) = "static_cast"; }
    break;

  case 417:

/* Line 1455 of yacc.c  */
#line 2245 "vtkParse.y"
    { (yyval.str) = "const_cast"; }
    break;

  case 418:

/* Line 1455 of yacc.c  */
#line 2246 "vtkParse.y"
    { (yyval.str) = "dynamic_cast"; }
    break;

  case 419:

/* Line 1455 of yacc.c  */
#line 2247 "vtkParse.y"
    { (yyval.str) = "reinterpret_cast"; }
    break;

  case 420:

/* Line 1455 of yacc.c  */
#line 2249 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 421:

/* Line 1455 of yacc.c  */
#line 2251 "vtkParse.y"
    { (yyval.str) = vtkstrcat((yyvsp[(1) - (2)].str), (yyvsp[(2) - (2)].str)); }
    break;

  case 422:

/* Line 1455 of yacc.c  */
#line 2253 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 423:

/* Line 1455 of yacc.c  */
#line 2254 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 424:

/* Line 1455 of yacc.c  */
#line 2255 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 425:

/* Line 1455 of yacc.c  */
#line 2256 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 426:

/* Line 1455 of yacc.c  */
#line 2257 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 427:

/* Line 1455 of yacc.c  */
#line 2258 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 428:

/* Line 1455 of yacc.c  */
#line 2260 "vtkParse.y"
    { (yyval.str) = vtkstrdup(add_const_scope((yyvsp[(1) - (1)].str))); }
    break;

  case 429:

/* Line 1455 of yacc.c  */
#line 2269 "vtkParse.y"
    {preSig("void Set"); postSig("(");}
    break;

  case 430:

/* Line 1455 of yacc.c  */
#line 2270 "vtkParse.y"
    {
   postSig("a);");
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (7)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, (yyvsp[(6) - (7)].integer), getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 431:

/* Line 1455 of yacc.c  */
#line 2278 "vtkParse.y"
    {postSig("Get");}
    break;

  case 432:

/* Line 1455 of yacc.c  */
#line 2279 "vtkParse.y"
    {markSig();}
    break;

  case 433:

/* Line 1455 of yacc.c  */
#line 2279 "vtkParse.y"
    {swapSig();}
    break;

  case 434:

/* Line 1455 of yacc.c  */
#line 2280 "vtkParse.y"
    {
   postSig("();");
   currentFunction->Name = vtkstrcat("Get", (yyvsp[(4) - (9)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, (yyvsp[(7) - (9)].integer), getTypeId(), 0);
   output_function();
   }
    break;

  case 435:

/* Line 1455 of yacc.c  */
#line 2287 "vtkParse.y"
    {preSig("void Set");}
    break;

  case 436:

/* Line 1455 of yacc.c  */
#line 2288 "vtkParse.y"
    {
   postSig("(char *);");
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(4) - (5)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 437:

/* Line 1455 of yacc.c  */
#line 2296 "vtkParse.y"
    {preSig("char *Get");}
    break;

  case 438:

/* Line 1455 of yacc.c  */
#line 2297 "vtkParse.y"
    {
   postSig("();");
   currentFunction->Name = vtkstrcat("Get", (yyvsp[(4) - (5)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   output_function();
   }
    break;

  case 439:

/* Line 1455 of yacc.c  */
#line 2304 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 440:

/* Line 1455 of yacc.c  */
#line 2304 "vtkParse.y"
    {closeSig();}
    break;

  case 441:

/* Line 1455 of yacc.c  */
#line 2306 "vtkParse.y"
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

  case 442:

/* Line 1455 of yacc.c  */
#line 2334 "vtkParse.y"
    {preSig("void Set"); postSig("("); }
    break;

  case 443:

/* Line 1455 of yacc.c  */
#line 2335 "vtkParse.y"
    {
   postSig("*);");
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (7)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_argument(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 444:

/* Line 1455 of yacc.c  */
#line 2343 "vtkParse.y"
    {postSig("*Get");}
    break;

  case 445:

/* Line 1455 of yacc.c  */
#line 2344 "vtkParse.y"
    {markSig();}
    break;

  case 446:

/* Line 1455 of yacc.c  */
#line 2344 "vtkParse.y"
    {swapSig();}
    break;

  case 447:

/* Line 1455 of yacc.c  */
#line 2345 "vtkParse.y"
    {
   postSig("();");
   currentFunction->Name = vtkstrcat("Get", (yyvsp[(4) - (9)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   output_function();
   }
    break;

  case 448:

/* Line 1455 of yacc.c  */
#line 2353 "vtkParse.y"
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

  case 449:

/* Line 1455 of yacc.c  */
#line 2368 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 450:

/* Line 1455 of yacc.c  */
#line 2369 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 2);
   }
    break;

  case 451:

/* Line 1455 of yacc.c  */
#line 2373 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 452:

/* Line 1455 of yacc.c  */
#line 2374 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 2);
   }
    break;

  case 453:

/* Line 1455 of yacc.c  */
#line 2378 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 454:

/* Line 1455 of yacc.c  */
#line 2379 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 3);
   }
    break;

  case 455:

/* Line 1455 of yacc.c  */
#line 2383 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 456:

/* Line 1455 of yacc.c  */
#line 2384 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 3);
   }
    break;

  case 457:

/* Line 1455 of yacc.c  */
#line 2388 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 458:

/* Line 1455 of yacc.c  */
#line 2389 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 4);
   }
    break;

  case 459:

/* Line 1455 of yacc.c  */
#line 2393 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 460:

/* Line 1455 of yacc.c  */
#line 2394 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 4);
   }
    break;

  case 461:

/* Line 1455 of yacc.c  */
#line 2398 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 462:

/* Line 1455 of yacc.c  */
#line 2399 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 6);
   }
    break;

  case 463:

/* Line 1455 of yacc.c  */
#line 2403 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 464:

/* Line 1455 of yacc.c  */
#line 2404 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 6);
   }
    break;

  case 465:

/* Line 1455 of yacc.c  */
#line 2408 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 466:

/* Line 1455 of yacc.c  */
#line 2410 "vtkParse.y"
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

  case 467:

/* Line 1455 of yacc.c  */
#line 2424 "vtkParse.y"
    {startSig();}
    break;

  case 468:

/* Line 1455 of yacc.c  */
#line 2426 "vtkParse.y"
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

  case 469:

/* Line 1455 of yacc.c  */
#line 2438 "vtkParse.y"
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

  case 470:

/* Line 1455 of yacc.c  */
#line 2471 "vtkParse.y"
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

  case 471:

/* Line 1455 of yacc.c  */
#line 2505 "vtkParse.y"
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

  case 472:

/* Line 1455 of yacc.c  */
#line 2557 "vtkParse.y"
    { (yyval.str) = "operator()"; }
    break;

  case 473:

/* Line 1455 of yacc.c  */
#line 2558 "vtkParse.y"
    { (yyval.str) = "operator[]"; }
    break;

  case 474:

/* Line 1455 of yacc.c  */
#line 2559 "vtkParse.y"
    { (yyval.str) = "operator new[]"; }
    break;

  case 475:

/* Line 1455 of yacc.c  */
#line 2560 "vtkParse.y"
    { (yyval.str) = "operator delete[]"; }
    break;

  case 477:

/* Line 1455 of yacc.c  */
#line 2563 "vtkParse.y"
    { (yyval.str) = "operator="; }
    break;

  case 478:

/* Line 1455 of yacc.c  */
#line 2564 "vtkParse.y"
    { (yyval.str) = "operator*"; }
    break;

  case 479:

/* Line 1455 of yacc.c  */
#line 2564 "vtkParse.y"
    { (yyval.str) = "operator/"; }
    break;

  case 480:

/* Line 1455 of yacc.c  */
#line 2565 "vtkParse.y"
    { (yyval.str) = "operator-"; }
    break;

  case 481:

/* Line 1455 of yacc.c  */
#line 2565 "vtkParse.y"
    { (yyval.str) = "operator+"; }
    break;

  case 482:

/* Line 1455 of yacc.c  */
#line 2566 "vtkParse.y"
    { (yyval.str) = "operator!"; }
    break;

  case 483:

/* Line 1455 of yacc.c  */
#line 2566 "vtkParse.y"
    { (yyval.str) = "operator~"; }
    break;

  case 484:

/* Line 1455 of yacc.c  */
#line 2567 "vtkParse.y"
    { (yyval.str) = "operator,"; }
    break;

  case 485:

/* Line 1455 of yacc.c  */
#line 2567 "vtkParse.y"
    { (yyval.str) = "operator<"; }
    break;

  case 486:

/* Line 1455 of yacc.c  */
#line 2568 "vtkParse.y"
    { (yyval.str) = "operator>"; }
    break;

  case 487:

/* Line 1455 of yacc.c  */
#line 2568 "vtkParse.y"
    { (yyval.str) = "operator&"; }
    break;

  case 488:

/* Line 1455 of yacc.c  */
#line 2569 "vtkParse.y"
    { (yyval.str) = "operator|"; }
    break;

  case 489:

/* Line 1455 of yacc.c  */
#line 2569 "vtkParse.y"
    { (yyval.str) = "operator^"; }
    break;

  case 490:

/* Line 1455 of yacc.c  */
#line 2570 "vtkParse.y"
    { (yyval.str) = "operator%"; }
    break;

  case 491:

/* Line 1455 of yacc.c  */
#line 2571 "vtkParse.y"
    { (yyval.str) = "operator new"; }
    break;

  case 492:

/* Line 1455 of yacc.c  */
#line 2572 "vtkParse.y"
    { (yyval.str) = "operator delete"; }
    break;

  case 493:

/* Line 1455 of yacc.c  */
#line 2573 "vtkParse.y"
    { (yyval.str) = "operator<<="; }
    break;

  case 494:

/* Line 1455 of yacc.c  */
#line 2574 "vtkParse.y"
    { (yyval.str) = "operator>>="; }
    break;

  case 495:

/* Line 1455 of yacc.c  */
#line 2575 "vtkParse.y"
    { (yyval.str) = "operator<<"; }
    break;

  case 496:

/* Line 1455 of yacc.c  */
#line 2576 "vtkParse.y"
    { (yyval.str) = "operator>>"; }
    break;

  case 497:

/* Line 1455 of yacc.c  */
#line 2577 "vtkParse.y"
    { (yyval.str) = "operator->*"; }
    break;

  case 498:

/* Line 1455 of yacc.c  */
#line 2578 "vtkParse.y"
    { (yyval.str) = "operator->"; }
    break;

  case 499:

/* Line 1455 of yacc.c  */
#line 2579 "vtkParse.y"
    { (yyval.str) = "operator+="; }
    break;

  case 500:

/* Line 1455 of yacc.c  */
#line 2580 "vtkParse.y"
    { (yyval.str) = "operator-="; }
    break;

  case 501:

/* Line 1455 of yacc.c  */
#line 2581 "vtkParse.y"
    { (yyval.str) = "operator*="; }
    break;

  case 502:

/* Line 1455 of yacc.c  */
#line 2582 "vtkParse.y"
    { (yyval.str) = "operator/="; }
    break;

  case 503:

/* Line 1455 of yacc.c  */
#line 2583 "vtkParse.y"
    { (yyval.str) = "operator%="; }
    break;

  case 504:

/* Line 1455 of yacc.c  */
#line 2584 "vtkParse.y"
    { (yyval.str) = "operator++"; }
    break;

  case 505:

/* Line 1455 of yacc.c  */
#line 2585 "vtkParse.y"
    { (yyval.str) = "operator--"; }
    break;

  case 506:

/* Line 1455 of yacc.c  */
#line 2586 "vtkParse.y"
    { (yyval.str) = "operator&="; }
    break;

  case 507:

/* Line 1455 of yacc.c  */
#line 2587 "vtkParse.y"
    { (yyval.str) = "operator|="; }
    break;

  case 508:

/* Line 1455 of yacc.c  */
#line 2588 "vtkParse.y"
    { (yyval.str) = "operator^="; }
    break;

  case 509:

/* Line 1455 of yacc.c  */
#line 2589 "vtkParse.y"
    {(yyval.str) = "operator&&=";}
    break;

  case 510:

/* Line 1455 of yacc.c  */
#line 2590 "vtkParse.y"
    {(yyval.str) = "operator||=";}
    break;

  case 511:

/* Line 1455 of yacc.c  */
#line 2591 "vtkParse.y"
    { (yyval.str) = "operator&&"; }
    break;

  case 512:

/* Line 1455 of yacc.c  */
#line 2592 "vtkParse.y"
    { (yyval.str) = "operator||"; }
    break;

  case 513:

/* Line 1455 of yacc.c  */
#line 2593 "vtkParse.y"
    { (yyval.str) = "operator=="; }
    break;

  case 514:

/* Line 1455 of yacc.c  */
#line 2594 "vtkParse.y"
    { (yyval.str) = "operator!="; }
    break;

  case 515:

/* Line 1455 of yacc.c  */
#line 2595 "vtkParse.y"
    { (yyval.str) = "operator<="; }
    break;

  case 516:

/* Line 1455 of yacc.c  */
#line 2596 "vtkParse.y"
    { (yyval.str) = "operator>="; }
    break;



/* Line 1455 of yacc.c  */
#line 8116 "vtkParse.tab.c"
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
#line 2621 "vtkParse.y"

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
  func->Class = NULL;
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

  /* everything below here is legacy information, *
   * maintained only for backwards compatibility  */
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
void vtkParse_InitUsing(UsingInfo *item)
{
  item->ItemType = VTK_USING_INFO;
  item->Access = VTK_ACCESS_PUBLIC;
  item->Name = NULL;
  item->Comment = NULL;
  item->Scope = NULL;
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
      return VTK_DOUBLE;
      }
    else if (result == VTK_PARSE_PREPROC_FLOAT)
      {
      return VTK_FLOAT;
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
