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
pointers, etc.  The parser also creates a typeId string, which is either
a simple id that gives the class name or type name, or is "function" for
function pointer types, or "method" for method pointer types.
*/

/*
Conformance Notes:

This parser was designed empirically and incrementally.  It has been
refactored to make it more similar to the ANSI C++ 1996 BNF grammar,
but there are still many very significant differences.

The most significant difference between this parser and a "standard"
parser is that it only parses declarations in detail.  All other
statements and expressions are parsed as arbitrary sequences of symbols,
without any syntactic analysis.

The "unqualified_id" does not directly include "operator_function_id" or
"conversion_function_id" (e.g. ids like "operator=" or "operator int*").
Instead, these two id types are used to allow operator functions to be
handled by their own rules, rather than by the generic function rules.
These ids can only be used in function declarations and using declarations.

Types are handled quite differently from the ANSI BNF.  These differences
represent a prolonged (and ultimately successful) attempt to empirically
create a yacc parser without any shift/reduce conflicts.  The rules for
types are organized according to the way that types are usually defined
in working code, rather than strictly according to C++ grammar.

The declaration specifiers "friend" and "typedef" can only appear at the
beginning of a declaration sequence.  There are also restrictions on
where class and enum specifiers can be used: you can declare a new struct
within a variable declaration, but not within a parameter declaration.

The lexer returns each of "(scope::*", "(*", "(a::b::*", etc. as single
tokens.  The ANSI BNF, in contrast, would consider these to be a "("
followed by a "ptr_operator".  The lexer concatenates these tokens in
order to eliminate shift/reduce conflicts in the parser.  However, this
means that this parser will only recognize "scope::*" as valid if it is
preceded by "(", e.g. as part of a member function pointer specification.

An odd bit of C++ ambiguity is that y(x); can be interpreted variously
as declaration of variable "x" of type "y", as a function call if "y"
is the name of a function, or as a constructor if "y" is the name of
a class.  This parser always interprets this pattern as a constructor
declaration, because function calls are ignored by the parser, and
variable declarations of the form y(x); are exceedingly rare compared
to the more usual form y x; without parentheses.
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
   the primitive_type production rule code.  */
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
FileInfo      *data = NULL;
int            parseDebug;

/* the "preprocessor" */
PreprocessInfo *preprocessor = NULL;

/* include dirs specified on the command line */
int            NumberOfIncludeDirectories= 0;
const char   **IncludeDirectories;

/* macros specified on the command line */
int            NumberOfDefinitions = 0;
const char   **Definitions;

/* names of classes marked as "concrete" */
int            NumberOfConcreteClasses = 0;
const char   **ConcreteClasses;

/* options that can be set by the programs that use the parser */
int            IgnoreBTX = 0;
int            Recursive = 0;

/* various state variables */
NamespaceInfo *currentNamespace = NULL;
ClassInfo     *currentClass = NULL;
FunctionInfo  *currentFunction = NULL;
TemplateInfo  *currentTemplate = NULL;
const char    *currentEnumName = NULL;
const char    *currentEnumValue = NULL;
parse_access_t access_level = VTK_ACCESS_PUBLIC;

/* functions from vtkParse.l */
void print_parser_error(const char *text, const char *cp, size_t n);

/* helper functions */
const char *type_class(unsigned int type, const char *classname);
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
void add_parameter(FunctionInfo *func, unsigned int type,
                   const char *classname, int count);
void add_template_parameter(unsigned int datatype,
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
unsigned int guess_id_type(const char *cp);
unsigned int add_indirection(unsigned int tval, unsigned int ptr);
unsigned int add_indirection_to_array(unsigned int ptr);
void handle_complex_type(ValueInfo *val, unsigned int datatype,
                         unsigned int extra, const char *funcSig);
void handle_function_type(ValueInfo *param, const char *name,
                          const char *funcSig);
void add_legacy_parameter(FunctionInfo *func, ValueInfo *param);

void outputSetVectorMacro(const char *var, unsigned int paramType,
                          const char *typeText, int n);
void outputGetVectorMacro(const char *var, unsigned int paramType,
                          const char *typeText, int n);

/*----------------------------------------------------------------
 * String utility methods
 *
 * Strings are centrally allocated and are const, and they are not
 * freed until the program exits.  If they need to be freed before
 * then, vtkParse_FreeStringCache() can be called.
 */

/* duplicate the first n bytes of a string and terminate */
static const char *vtkstrndup(const char *in, size_t n)
{
  return vtkParse_CacheString(data->Strings, in, n);
}

/* duplicate a string */
static const char *vtkstrdup(const char *in)
{
  if (in)
    {
    in = vtkParse_CacheString(data->Strings, in, strlen(in));
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
  cp = vtkParse_NewString(data->Strings, m);
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

static size_t vtkidlen(const char *text)
{
  size_t i = 0;
  char c = text[0];

  if ((c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z') ||
       c == '_')
    {
    do
      {
      c = text[++i];
      }
    while ((c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '_');
    }

  return i;
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
    currentNamespace->Name = name;
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
TemplateInfo *templateStack[10];
int templateDepth = 0;

/* begin a template */
void startTemplate()
{
  currentTemplate = (TemplateInfo *)malloc(sizeof(TemplateInfo));
  vtkParse_InitTemplate(currentTemplate);
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
    signature = vtkParse_NewString(data->Strings, sigAllocatedLength);
    signature[0] = '\0';
    }
  else if (sigLength + n > sigAllocatedLength)
    {
    sigAllocatedLength += sigLength + n;
    ccp = signature;
    signature = vtkParse_NewString(data->Strings, sigAllocatedLength);
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
  return vtkstrdup(cp);
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
 * Subroutines for building a type
 */

/* "private" variables */
unsigned int storedType;
unsigned int typeStack[10];
int typeDepth = 0;

/* save the type on the stack */
void pushType()
{
  typeStack[typeDepth++] = storedType;
}

/* pop the type stack */
void popType()
{
  storedType = typeStack[--typeDepth];
}

/* clear the storage type */
void clearType()
{
  storedType = 0;
}

/* save the type */
void setTypeBase(unsigned int base)
{
  storedType &= ~(unsigned int)(VTK_PARSE_BASE_TYPE);
  storedType |= base;
}

/* set a type modifier bit */
void setTypeMod(unsigned int mod)
{
  storedType |= mod;
}

/* modify the indirection (pointers, refs) in the storage type */
void setTypePtr(unsigned int ind)
{
  storedType &= ~(unsigned int)(VTK_PARSE_INDIRECT);
  ind &= VTK_PARSE_INDIRECT;
  storedType |= ind;
}

/* retrieve the storage type */
unsigned int getType()
{
  return storedType;
}

/* combine two primitive type parts, e.g. "long int" */
unsigned int buildTypeBase(unsigned int a, unsigned int b)
{
  unsigned int base = (a & VTK_PARSE_BASE_TYPE);
  unsigned int basemod = (b & VTK_PARSE_BASE_TYPE);

  switch (base)
    {
    case 0:
      base = basemod;
      break;
    case VTK_PARSE_UNSIGNED_INT:
      base = (basemod | VTK_PARSE_UNSIGNED);
      break;
    case VTK_PARSE_INT:
      base = basemod;
      if (base == VTK_PARSE_CHAR)
        {
        base = VTK_PARSE_SIGNED_CHAR;
        }
      break;
    case VTK_PARSE_CHAR:
      if (basemod == VTK_PARSE_INT)
        {
        base = VTK_PARSE_SIGNED_CHAR;
        }
      else if (basemod == VTK_PARSE_UNSIGNED_INT)
        {
        base = VTK_PARSE_UNSIGNED_CHAR;
        }
      break;
    case VTK_PARSE_SHORT:
      if (basemod == VTK_PARSE_UNSIGNED_INT)
        {
        base = VTK_PARSE_UNSIGNED_SHORT;
        }
      break;
    case VTK_PARSE_LONG:
      if (basemod == VTK_PARSE_UNSIGNED_INT)
        {
        base = VTK_PARSE_UNSIGNED_LONG;
        }
      else if (basemod == VTK_PARSE_LONG)
        {
        base = VTK_PARSE_LONG_LONG;
        }
      else if (basemod == VTK_PARSE_DOUBLE)
        {
        base = VTK_PARSE_LONG_DOUBLE;
        }
      break;
    case VTK_PARSE_UNSIGNED_LONG:
      if (basemod == VTK_PARSE_LONG)
        {
        base = VTK_PARSE_UNSIGNED_LONG_LONG;
        }
      break;
    case VTK_PARSE_LONG_LONG:
      if (basemod == VTK_PARSE_UNSIGNED_INT)
        {
        base = VTK_PARSE_UNSIGNED_LONG_LONG;
        }
      break;
    case VTK_PARSE___INT64:
      if (basemod == VTK_PARSE_UNSIGNED_INT)
        {
        base = VTK_PARSE_UNSIGNED___INT64;
        }
      break;
    case VTK_PARSE_DOUBLE:
      if (basemod == VTK_PARSE_LONG)
        {
        base = VTK_PARSE_LONG_DOUBLE;
        }
      break;
    }

  return ((a & ~(unsigned int)(VTK_PARSE_BASE_TYPE)) | base);
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
                            size);
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

  arrayDimensions[0] = size;
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
 * Variables and Parameters
 */

/* "private" variables */
const char *currentVarName = 0;
const char *currentVarValue = 0;
const char *currentId = 0;

/* clear the var Id */
void clearVarName(void)
{
  currentVarName = NULL;
}

/* set the var Id */
void setVarName(const char *text)
{
  currentVarName = text;
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
  currentVarValue = text;
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
  if (currentId == NULL)
    {
    currentId = text;
    }
}

/* set the signature and type together */
void typeSig(const char *text)
{
  postSig(text);
  postSig(" ");

  if (currentId == 0)
    {
    setTypeId(text);
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
  functionVarNameStack[functionDepth] = getVarName();
  functionTypeIdStack[functionDepth] = getTypeId();
  pushType();
  clearType();
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
  popType();

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
#line 1358 "vtkParse.tab.c"

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
     ID = 258,
     VTK_ID = 259,
     QT_ID = 260,
     StdString = 261,
     UnicodeString = 262,
     OSTREAM = 263,
     ISTREAM = 264,
     LP = 265,
     LA = 266,
     STRING_LITERAL = 267,
     INT_LITERAL = 268,
     HEX_LITERAL = 269,
     OCT_LITERAL = 270,
     FLOAT_LITERAL = 271,
     CHAR_LITERAL = 272,
     ZERO = 273,
     STRUCT = 274,
     CLASS = 275,
     UNION = 276,
     ENUM = 277,
     PUBLIC = 278,
     PRIVATE = 279,
     PROTECTED = 280,
     CONST = 281,
     VOLATILE = 282,
     MUTABLE = 283,
     STATIC = 284,
     VIRTUAL = 285,
     EXPLICIT = 286,
     INLINE = 287,
     FRIEND = 288,
     EXTERN = 289,
     OPERATOR = 290,
     TEMPLATE = 291,
     THROW = 292,
     TYPENAME = 293,
     TYPEDEF = 294,
     NAMESPACE = 295,
     USING = 296,
     NEW = 297,
     DELETE = 298,
     STATIC_CAST = 299,
     DYNAMIC_CAST = 300,
     CONST_CAST = 301,
     REINTERPRET_CAST = 302,
     OP_LSHIFT_EQ = 303,
     OP_RSHIFT_EQ = 304,
     OP_LSHIFT = 305,
     OP_RSHIFT = 306,
     OP_DOT_POINTER = 307,
     OP_ARROW_POINTER = 308,
     OP_ARROW = 309,
     OP_INCR = 310,
     OP_DECR = 311,
     OP_PLUS_EQ = 312,
     OP_MINUS_EQ = 313,
     OP_TIMES_EQ = 314,
     OP_DIVIDE_EQ = 315,
     OP_REMAINDER_EQ = 316,
     OP_AND_EQ = 317,
     OP_OR_EQ = 318,
     OP_XOR_EQ = 319,
     OP_LOGIC_AND = 320,
     OP_LOGIC_OR = 321,
     OP_LOGIC_EQ = 322,
     OP_LOGIC_NEQ = 323,
     OP_LOGIC_LEQ = 324,
     OP_LOGIC_GEQ = 325,
     ELLIPSIS = 326,
     DOUBLE_COLON = 327,
     OTHER = 328,
     VOID = 329,
     BOOL = 330,
     FLOAT = 331,
     DOUBLE = 332,
     INT = 333,
     SHORT = 334,
     LONG = 335,
     INT64__ = 336,
     CHAR = 337,
     SIGNED = 338,
     UNSIGNED = 339,
     SSIZE_T = 340,
     SIZE_T = 341,
     IdType = 342,
     FloatType = 343,
     TypeInt8 = 344,
     TypeUInt8 = 345,
     TypeInt16 = 346,
     TypeUInt16 = 347,
     TypeInt32 = 348,
     TypeUInt32 = 349,
     TypeInt64 = 350,
     TypeUInt64 = 351,
     TypeFloat32 = 352,
     TypeFloat64 = 353,
     SetMacro = 354,
     GetMacro = 355,
     SetStringMacro = 356,
     GetStringMacro = 357,
     SetClampMacro = 358,
     SetObjectMacro = 359,
     GetObjectMacro = 360,
     BooleanMacro = 361,
     SetVector2Macro = 362,
     SetVector3Macro = 363,
     SetVector4Macro = 364,
     SetVector6Macro = 365,
     GetVector2Macro = 366,
     GetVector3Macro = 367,
     GetVector4Macro = 368,
     GetVector6Macro = 369,
     SetVectorMacro = 370,
     GetVectorMacro = 371,
     ViewportCoordinateMacro = 372,
     WorldCoordinateMacro = 373,
     TypeMacro = 374,
     VTK_BYTE_SWAP_DECL = 375
   };
#endif




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 1306 "vtkParse.y"

  const char   *str;
  unsigned int  integer;



/* Line 214 of yacc.c  */
#line 1641 "vtkParse.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 1653 "vtkParse.tab.c"

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
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   5595

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  144
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  239
/* YYNRULES -- Number of rules.  */
#define YYNRULES  603
/* YYNRULES -- Number of states.  */
#define YYNSTATES  966

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   375

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   140,     2,     2,     2,   136,   134,     2,
     127,   128,   135,   139,   126,   138,   143,   137,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   125,   121,
     129,   124,   130,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   131,     2,   132,   142,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   122,   141,   123,   133,     2,     2,     2,
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
     115,   116,   117,   118,   119,   120
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     6,     7,    11,    13,    15,    17,
      19,    21,    23,    25,    27,    29,    31,    33,    35,    37,
      40,    42,    45,    48,    51,    57,    62,    63,    70,    76,
      78,    81,    85,    90,    95,   101,   102,   108,   109,   114,
     115,   119,   121,   123,   125,   126,   127,   131,   135,   137,
     139,   141,   143,   145,   147,   149,   151,   153,   155,   157,
     159,   161,   164,   167,   169,   172,   175,   178,   182,   185,
     189,   190,   192,   195,   197,   201,   203,   207,   211,   212,
     214,   215,   217,   219,   221,   223,   228,   234,   235,   241,
     244,   246,   247,   249,   251,   254,   258,   260,   261,   266,
     273,   277,   282,   285,   289,   295,   299,   301,   304,   310,
     316,   323,   329,   336,   339,   340,   344,   347,   349,   351,
     352,   353,   361,   363,   367,   369,   372,   375,   378,   382,
     386,   391,   395,   396,   402,   404,   405,   410,   411,   412,
     418,   419,   420,   426,   427,   428,   434,   436,   438,   439,
     441,   442,   446,   448,   451,   454,   457,   460,   463,   466,
     470,   473,   477,   480,   484,   488,   491,   495,   500,   503,
     505,   507,   510,   512,   515,   518,   519,   520,   528,   531,
     532,   536,   537,   543,   546,   548,   551,   552,   555,   556,
     560,   562,   565,   569,   571,   572,   578,   580,   582,   583,
     584,   590,   591,   597,   598,   601,   603,   607,   610,   611,
     612,   615,   617,   618,   623,   627,   628,   629,   635,   636,
     638,   639,   643,   648,   651,   652,   655,   656,   657,   662,
     665,   666,   668,   671,   672,   678,   681,   682,   688,   690,
     692,   694,   696,   698,   699,   700,   701,   708,   710,   711,
     714,   717,   721,   723,   726,   728,   731,   732,   734,   736,
     740,   742,   744,   746,   747,   749,   750,   753,   755,   758,
     759,   764,   765,   766,   769,   771,   773,   775,   777,   780,
     783,   786,   789,   792,   796,   800,   801,   807,   809,   811,
     812,   818,   820,   822,   824,   826,   828,   830,   832,   835,
     838,   841,   844,   847,   850,   853,   855,   857,   859,   861,
     863,   865,   867,   869,   871,   873,   875,   877,   879,   881,
     883,   885,   887,   889,   891,   893,   895,   896,   899,   901,
     903,   905,   907,   909,   912,   914,   916,   918,   920,   922,
     925,   927,   929,   931,   933,   935,   937,   939,   942,   945,
     946,   950,   951,   956,   958,   959,   963,   965,   967,   970,
     973,   976,   977,   981,   982,   987,   989,   991,   993,   996,
     999,  1002,  1004,  1006,  1008,  1010,  1012,  1014,  1016,  1018,
    1020,  1022,  1024,  1026,  1028,  1030,  1032,  1034,  1036,  1038,
    1040,  1042,  1044,  1046,  1048,  1050,  1052,  1054,  1056,  1058,
    1060,  1062,  1064,  1066,  1068,  1070,  1072,  1074,  1077,  1079,
    1081,  1082,  1086,  1088,  1091,  1092,  1100,  1101,  1102,  1103,
    1113,  1114,  1120,  1121,  1127,  1128,  1129,  1140,  1141,  1149,
    1150,  1151,  1152,  1162,  1169,  1170,  1178,  1179,  1187,  1188,
    1196,  1197,  1205,  1206,  1214,  1215,  1223,  1224,  1232,  1233,
    1241,  1242,  1252,  1253,  1263,  1268,  1273,  1281,  1282,  1284,
    1287,  1290,  1294,  1298,  1300,  1302,  1304,  1306,  1308,  1310,
    1312,  1314,  1316,  1318,  1320,  1322,  1324,  1326,  1328,  1330,
    1332,  1334,  1336,  1338,  1340,  1342,  1344,  1346,  1348,  1350,
    1352,  1354,  1356,  1358,  1360,  1362,  1364,  1366,  1368,  1370,
    1372,  1374,  1376,  1378,  1380,  1382,  1384,  1386,  1388,  1390,
    1392,  1394,  1396,  1398,  1400,  1402,  1404,  1406,  1408,  1410,
    1412,  1414,  1416,  1418,  1420,  1422,  1424,  1426,  1428,  1430,
    1432,  1434,  1436,  1438,  1441,  1443,  1445,  1447,  1449,  1451,
    1453,  1455,  1457,  1459,  1461,  1463,  1464,  1467,  1469,  1471,
    1473,  1475,  1477,  1479,  1481,  1483,  1484,  1487,  1488,  1491,
    1493,  1495,  1497,  1499,  1501,  1502,  1507,  1508,  1513,  1514,
    1519,  1520,  1525,  1526,  1531,  1532,  1537,  1538,  1541,  1542,
    1545,  1547,  1549,  1551,  1553,  1555,  1557,  1559,  1561,  1563,
    1565,  1567,  1569,  1571,  1573,  1575,  1577,  1579,  1581,  1585,
    1589,  1593,  1595,  1597
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     145,     0,    -1,   146,    -1,    -1,    -1,   146,   147,   148,
      -1,   199,    -1,   197,    -1,   154,    -1,   151,    -1,   153,
      -1,   150,    -1,   187,    -1,   259,    -1,   176,    -1,   156,
      -1,   216,    -1,   149,    -1,   327,    -1,   289,   121,    -1,
     121,    -1,   200,   156,    -1,   200,   216,    -1,   200,   184,
      -1,    34,    12,   122,   146,   123,    -1,    40,   122,   375,
     123,    -1,    -1,    40,   299,   152,   122,   146,   123,    -1,
      40,   299,   124,   291,   121,    -1,   155,    -1,   200,   155,
      -1,   162,   289,   121,    -1,   302,   162,   289,   121,    -1,
     157,   300,   261,   121,    -1,   302,   157,   300,   261,   121,
      -1,    -1,   159,   158,   122,   163,   123,    -1,    -1,   162,
     289,   160,   169,    -1,    -1,   162,   161,   169,    -1,    20,
      -1,    19,    -1,    21,    -1,    -1,    -1,   163,   164,   166,
      -1,   163,   165,   125,    -1,    23,    -1,    24,    -1,    25,
      -1,   197,    -1,   154,    -1,   168,    -1,   187,    -1,   259,
      -1,   176,    -1,   156,    -1,   220,    -1,   167,    -1,   327,
      -1,   120,   381,    -1,   289,   121,    -1,   121,    -1,   200,
     156,    -1,   200,   220,    -1,    33,   185,    -1,    33,   200,
     185,    -1,    33,   154,    -1,    33,   221,   237,    -1,    -1,
     170,    -1,   125,   171,    -1,   172,    -1,   171,   126,   172,
      -1,   289,    -1,    30,   174,   289,    -1,   175,   173,   289,
      -1,    -1,    30,    -1,    -1,   175,    -1,    23,    -1,    24,
      -1,    25,    -1,   177,   300,   261,   121,    -1,   302,   177,
     300,   261,   121,    -1,    -1,   179,   122,   178,   180,   123,
      -1,    22,   289,    -1,    22,    -1,    -1,   181,    -1,   182,
      -1,   181,   126,    -1,   181,   126,   182,    -1,   298,    -1,
      -1,   298,   124,   183,   355,    -1,   308,   292,   298,   124,
     376,   121,    -1,   162,   289,   186,    -1,   302,   162,   289,
     186,    -1,   162,   186,    -1,   302,   162,   186,    -1,   122,
     375,   123,   376,   121,    -1,   125,   376,   121,    -1,   188,
      -1,   302,   188,    -1,    39,   308,   196,   190,   121,    -1,
      39,   157,   300,   189,   121,    -1,    39,   302,   157,   300,
     189,   121,    -1,    39,   177,   300,   189,   121,    -1,    39,
     302,   177,   300,   189,   121,    -1,   191,   190,    -1,    -1,
     190,   126,   191,    -1,   265,   196,    -1,   268,    -1,   193,
      -1,    -1,    -1,   279,   127,   194,   249,   128,   195,   275,
      -1,   192,    -1,    41,   198,   121,    -1,   289,    -1,    38,
     289,    -1,   292,   231,    -1,   292,   226,    -1,   295,   292,
     231,    -1,   295,   292,   226,    -1,    41,    40,   289,   121,
      -1,    36,   129,   130,    -1,    -1,    36,   129,   201,   202,
     130,    -1,   204,    -1,    -1,   202,   126,   203,   204,    -1,
      -1,    -1,   205,   314,   266,   206,   212,    -1,    -1,    -1,
     207,   211,   266,   208,   212,    -1,    -1,    -1,   209,   200,
     266,   210,   212,    -1,    20,    -1,    38,    -1,    -1,   213,
      -1,    -1,   124,   214,   215,    -1,   363,    -1,   215,   363,
      -1,   217,   237,    -1,   222,   237,    -1,   218,   237,    -1,
     219,   237,    -1,   308,   233,    -1,   308,   292,   233,    -1,
     292,   241,    -1,   302,   292,   241,    -1,   292,   223,    -1,
     302,   292,   223,    -1,   308,   292,   227,    -1,   221,   237,
      -1,   292,   231,   121,    -1,   302,   292,   231,   121,    -1,
     308,   233,    -1,   222,    -1,   241,    -1,   302,   241,    -1,
     223,    -1,   302,   223,    -1,   308,   227,    -1,    -1,    -1,
     226,   127,   224,   249,   128,   225,   234,    -1,   232,   308,
      -1,    -1,   229,   228,   234,    -1,    -1,   231,   127,   230,
     249,   128,    -1,   232,   351,    -1,    35,    -1,   238,   234,
      -1,    -1,   234,   235,    -1,    -1,    37,   236,   369,    -1,
      26,    -1,   124,    18,    -1,   122,   375,   123,    -1,   121,
      -1,    -1,   240,   127,   239,   249,   128,    -1,   298,    -1,
     296,    -1,    -1,    -1,   244,   242,   246,   243,   234,    -1,
      -1,   240,   127,   245,   249,   128,    -1,    -1,   125,   247,
      -1,   248,    -1,   247,   126,   248,    -1,   289,   381,    -1,
      -1,    -1,   250,   251,    -1,   253,    -1,    -1,   251,   126,
     252,   253,    -1,   251,   126,    71,    -1,    -1,    -1,   254,
     308,   266,   255,   256,    -1,    -1,   257,    -1,    -1,   124,
     258,   355,    -1,   308,   260,   262,   121,    -1,   268,   256,
      -1,    -1,   264,   262,    -1,    -1,    -1,   262,   126,   263,
     264,    -1,   265,   260,    -1,    -1,   322,    -1,   278,   281,
      -1,    -1,   270,   276,   128,   267,   272,    -1,   279,   281,
      -1,    -1,   271,   277,   128,   269,   272,    -1,   127,    -1,
      10,    -1,    11,    -1,    10,    -1,    11,    -1,    -1,    -1,
      -1,   127,   273,   249,   128,   274,   275,    -1,   282,    -1,
      -1,   275,    28,    -1,   275,    26,    -1,   275,    37,   381,
      -1,   266,    -1,   322,   266,    -1,   268,    -1,   322,   268,
      -1,    -1,   279,    -1,   298,    -1,   298,   125,   280,    -1,
      15,    -1,    13,    -1,    14,    -1,    -1,   282,    -1,    -1,
     283,   284,    -1,   285,    -1,   284,   285,    -1,    -1,   131,
     286,   287,   132,    -1,    -1,    -1,   288,   355,    -1,   290,
      -1,   291,    -1,   298,    -1,   296,    -1,   292,   290,    -1,
     295,   290,    -1,   295,   291,    -1,   294,   295,    -1,   296,
     295,    -1,   292,   294,   295,    -1,   292,   296,   295,    -1,
      -1,   292,    36,   293,   296,   295,    -1,   299,    -1,    72,
      -1,    -1,   299,   129,   297,   361,   130,    -1,     4,    -1,
       5,    -1,     3,    -1,     9,    -1,     8,    -1,     6,    -1,
       7,    -1,   133,     4,    -1,   133,     5,    -1,   133,     3,
      -1,   133,     9,    -1,   133,     8,    -1,   133,     6,    -1,
     133,     7,    -1,    86,    -1,    85,    -1,    89,    -1,    90,
      -1,    91,    -1,    92,    -1,    93,    -1,    94,    -1,    95,
      -1,    96,    -1,    97,    -1,    98,    -1,    87,    -1,    88,
      -1,     3,    -1,     5,    -1,     4,    -1,     9,    -1,     8,
      -1,     6,    -1,     7,    -1,    -1,   300,   301,    -1,   303,
      -1,   321,    -1,    39,    -1,    33,    -1,   303,    -1,   302,
     303,    -1,   304,    -1,   305,    -1,   306,    -1,    28,    -1,
      34,    -1,    34,    12,    -1,    29,    -1,    32,    -1,    30,
      -1,    31,    -1,    26,    -1,    27,    -1,   306,    -1,   307,
     306,    -1,   309,   265,    -1,    -1,   312,   310,   300,    -1,
      -1,   302,   312,   311,   300,    -1,   319,    -1,    -1,    38,
     313,   289,    -1,   296,    -1,   291,    -1,   162,   289,    -1,
      22,   289,    -1,   315,   265,    -1,    -1,   318,   316,   300,
      -1,    -1,   302,   312,   317,   300,    -1,   319,    -1,   296,
      -1,   291,    -1,    19,   289,    -1,    21,   289,    -1,    22,
     289,    -1,   321,    -1,   320,    -1,     6,    -1,     7,    -1,
       8,    -1,     9,    -1,     3,    -1,     4,    -1,     5,    -1,
      85,    -1,    86,    -1,    89,    -1,    90,    -1,    91,    -1,
      92,    -1,    93,    -1,    94,    -1,    95,    -1,    96,    -1,
      97,    -1,    98,    -1,    87,    -1,    88,    -1,    74,    -1,
      75,    -1,    76,    -1,    77,    -1,    82,    -1,    78,    -1,
      79,    -1,    80,    -1,    81,    -1,    83,    -1,    84,    -1,
     323,    -1,   326,    -1,   326,   323,    -1,   134,    -1,   135,
      -1,    -1,   135,   325,   307,    -1,   324,    -1,   326,   324,
      -1,    -1,    99,   127,   298,   126,   328,   308,   128,    -1,
      -1,    -1,    -1,   100,   127,   329,   298,   126,   330,   308,
     331,   128,    -1,    -1,   101,   127,   332,   298,   128,    -1,
      -1,   102,   127,   333,   298,   128,    -1,    -1,    -1,   103,
     127,   298,   126,   334,   308,   335,   126,   376,   128,    -1,
      -1,   104,   127,   298,   126,   336,   308,   128,    -1,    -1,
      -1,    -1,   105,   127,   337,   298,   126,   338,   308,   339,
     128,    -1,   106,   127,   298,   126,   308,   128,    -1,    -1,
     107,   127,   298,   126,   340,   308,   128,    -1,    -1,   111,
     127,   298,   126,   341,   308,   128,    -1,    -1,   108,   127,
     298,   126,   342,   308,   128,    -1,    -1,   112,   127,   298,
     126,   343,   308,   128,    -1,    -1,   109,   127,   298,   126,
     344,   308,   128,    -1,    -1,   113,   127,   298,   126,   345,
     308,   128,    -1,    -1,   110,   127,   298,   126,   346,   308,
     128,    -1,    -1,   114,   127,   298,   126,   347,   308,   128,
      -1,    -1,   115,   127,   298,   126,   348,   308,   126,    13,
     128,    -1,    -1,   116,   127,   298,   126,   349,   308,   126,
      13,   128,    -1,   117,   127,   298,   128,    -1,   118,   127,
     298,   128,    -1,   119,   127,   298,   126,   298,   350,   128,
      -1,    -1,   126,    -1,   127,   128,    -1,   131,   132,    -1,
      42,   131,   132,    -1,    43,   131,   132,    -1,   129,    -1,
     130,    -1,   126,    -1,   124,    -1,   352,    -1,   136,    -1,
     135,    -1,   137,    -1,   138,    -1,   139,    -1,   140,    -1,
     133,    -1,   134,    -1,   141,    -1,   142,    -1,    42,    -1,
      43,    -1,    48,    -1,    49,    -1,    50,    -1,    51,    -1,
      52,    -1,    53,    -1,    54,    -1,    57,    -1,    58,    -1,
      59,    -1,    60,    -1,    61,    -1,    55,    -1,    56,    -1,
      62,    -1,    63,    -1,    64,    -1,    65,    -1,    66,    -1,
      67,    -1,    68,    -1,    69,    -1,    70,    -1,    39,    -1,
      38,    -1,    20,    -1,    19,    -1,    21,    -1,    36,    -1,
      23,    -1,    25,    -1,    24,    -1,    26,    -1,    29,    -1,
      32,    -1,    30,    -1,    34,    -1,    40,    -1,    35,    -1,
      22,    -1,    37,    -1,    46,    -1,    45,    -1,    44,    -1,
      47,    -1,    15,    -1,    13,    -1,    14,    -1,    16,    -1,
      17,    -1,    12,    -1,    18,    -1,   358,    -1,   355,   358,
      -1,   367,    -1,   369,    -1,   373,    -1,   352,    -1,   125,
      -1,   143,    -1,    72,    -1,   353,    -1,   354,    -1,   321,
      -1,   320,    -1,    -1,   357,   359,    -1,   356,    -1,   129,
      -1,   130,    -1,   358,    -1,   124,    -1,   126,    -1,   359,
      -1,   121,    -1,    -1,   361,   364,    -1,    -1,   362,   360,
      -1,   365,    -1,   356,    -1,   363,    -1,   124,    -1,   126,
      -1,    -1,   129,   366,   361,   130,    -1,    -1,   131,   368,
     357,   132,    -1,    -1,   127,   370,   357,   128,    -1,    -1,
      10,   371,   357,   128,    -1,    -1,    11,   372,   357,   128,
      -1,    -1,   122,   374,   362,   123,    -1,    -1,   375,   377,
      -1,    -1,   376,   378,    -1,   378,    -1,   121,    -1,   379,
      -1,   381,    -1,   380,    -1,    72,    -1,    71,    -1,   352,
      -1,   125,    -1,   143,    -1,   129,    -1,   130,    -1,   124,
      -1,   126,    -1,   353,    -1,   354,    -1,   319,    -1,    73,
      -1,   122,   375,   123,    -1,   131,   375,   132,    -1,   382,
     375,   128,    -1,   127,    -1,    10,    -1,    11,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,  1465,  1465,  1467,  1469,  1468,  1479,  1480,  1481,  1482,
    1483,  1484,  1485,  1486,  1487,  1488,  1489,  1490,  1491,  1492,
    1493,  1496,  1497,  1498,  1505,  1512,  1513,  1513,  1517,  1524,
    1525,  1528,  1529,  1532,  1533,  1536,  1536,  1550,  1550,  1552,
    1552,  1556,  1557,  1558,  1560,  1562,  1561,  1570,  1574,  1575,
    1576,  1579,  1580,  1581,  1582,  1583,  1584,  1585,  1586,  1587,
    1588,  1589,  1590,  1591,  1594,  1595,  1598,  1599,  1600,  1601,
    1603,  1604,  1607,  1610,  1611,  1614,  1616,  1618,  1622,  1623,
    1626,  1627,  1630,  1631,  1632,  1643,  1644,  1648,  1648,  1661,
    1662,  1664,  1665,  1668,  1669,  1670,  1673,  1674,  1674,  1682,
    1685,  1686,  1687,  1688,  1691,  1692,  1700,  1701,  1704,  1705,
    1707,  1709,  1711,  1715,  1717,  1718,  1721,  1724,  1725,  1728,
    1729,  1728,  1733,  1767,  1770,  1771,  1772,  1774,  1776,  1778,
    1782,  1789,  1792,  1791,  1809,  1811,  1810,  1815,  1817,  1815,
    1819,  1821,  1819,  1823,  1825,  1823,  1836,  1837,  1839,  1840,
    1843,  1843,  1853,  1854,  1862,  1863,  1864,  1865,  1868,  1871,
    1872,  1873,  1876,  1877,  1878,  1881,  1882,  1883,  1886,  1887,
    1888,  1889,  1892,  1893,  1894,  1898,  1902,  1897,  1914,  1918,
    1918,  1929,  1928,  1937,  1941,  1944,  1953,  1954,  1957,  1957,
    1958,  1959,  1967,  1968,  1972,  1971,  1979,  1980,  1988,  1989,
    1988,  2007,  2007,  2010,  2011,  2014,  2015,  2018,  2024,  2025,
    2025,  2028,  2029,  2029,  2031,  2035,  2037,  2035,  2061,  2062,
    2065,  2065,  2073,  2076,  2135,  2136,  2138,  2139,  2139,  2142,
    2145,  2146,  2150,  2151,  2151,  2170,  2171,  2171,  2189,  2190,
    2192,  2196,  2198,  2201,  2202,  2203,  2202,  2208,  2210,  2211,
    2212,  2213,  2216,  2217,  2221,  2222,  2226,  2227,  2230,  2231,
    2234,  2235,  2236,  2239,  2240,  2243,  2243,  2246,  2247,  2250,
    2250,  2253,  2254,  2254,  2261,  2262,  2265,  2266,  2269,  2271,
    2273,  2277,  2279,  2281,  2283,  2285,  2285,  2290,  2293,  2296,
    2296,  2311,  2312,  2313,  2314,  2315,  2316,  2317,  2318,  2319,
    2320,  2321,  2322,  2323,  2324,  2325,  2326,  2327,  2328,  2329,
    2330,  2331,  2332,  2333,  2334,  2335,  2336,  2337,  2338,  2345,
    2346,  2347,  2348,  2349,  2350,  2351,  2358,  2359,  2362,  2363,
    2365,  2366,  2369,  2370,  2373,  2374,  2375,  2378,  2379,  2380,
    2381,  2384,  2385,  2386,  2389,  2390,  2393,  2394,  2403,  2406,
    2406,  2408,  2408,  2412,  2413,  2413,  2415,  2417,  2419,  2421,
    2425,  2428,  2428,  2430,  2430,  2434,  2435,  2437,  2439,  2441,
    2443,  2447,  2448,  2451,  2452,  2453,  2454,  2455,  2456,  2457,
    2458,  2459,  2460,  2461,  2462,  2463,  2464,  2465,  2466,  2467,
    2468,  2469,  2470,  2471,  2474,  2475,  2476,  2477,  2478,  2479,
    2480,  2481,  2482,  2483,  2484,  2504,  2505,  2506,  2509,  2512,
    2513,  2513,  2528,  2529,  2546,  2546,  2556,  2557,  2557,  2556,
    2566,  2566,  2576,  2576,  2585,  2585,  2585,  2618,  2617,  2628,
    2629,  2629,  2628,  2638,  2656,  2656,  2661,  2661,  2666,  2666,
    2671,  2671,  2676,  2676,  2681,  2681,  2686,  2686,  2691,  2691,
    2696,  2696,  2713,  2713,  2727,  2764,  2802,  2854,  2855,  2862,
    2863,  2864,  2865,  2866,  2867,  2868,  2869,  2870,  2873,  2874,
    2875,  2876,  2877,  2878,  2879,  2880,  2881,  2882,  2883,  2884,
    2885,  2886,  2887,  2888,  2889,  2890,  2891,  2892,  2893,  2894,
    2895,  2896,  2897,  2898,  2899,  2900,  2901,  2902,  2903,  2904,
    2905,  2906,  2907,  2910,  2911,  2912,  2913,  2914,  2915,  2916,
    2917,  2918,  2919,  2920,  2921,  2922,  2923,  2924,  2925,  2926,
    2927,  2928,  2929,  2930,  2931,  2934,  2935,  2936,  2937,  2938,
    2939,  2940,  2947,  2948,  2951,  2952,  2953,  2954,  2985,  2985,
    2986,  2987,  2988,  2989,  2990,  3013,  3014,  3016,  3017,  3018,
    3020,  3021,  3022,  3024,  3025,  3027,  3028,  3030,  3031,  3034,
    3035,  3038,  3039,  3040,  3044,  3043,  3057,  3057,  3061,  3061,
    3063,  3063,  3065,  3065,  3069,  3069,  3074,  3075,  3077,  3078,
    3081,  3082,  3085,  3086,  3087,  3088,  3089,  3090,  3091,  3091,
    3091,  3091,  3091,  3091,  3092,  3092,  3093,  3094,  3097,  3100,
    3103,  3106,  3106,  3106
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ID", "VTK_ID", "QT_ID", "StdString",
  "UnicodeString", "OSTREAM", "ISTREAM", "LP", "LA", "STRING_LITERAL",
  "INT_LITERAL", "HEX_LITERAL", "OCT_LITERAL", "FLOAT_LITERAL",
  "CHAR_LITERAL", "ZERO", "STRUCT", "CLASS", "UNION", "ENUM", "PUBLIC",
  "PRIVATE", "PROTECTED", "CONST", "VOLATILE", "MUTABLE", "STATIC",
  "VIRTUAL", "EXPLICIT", "INLINE", "FRIEND", "EXTERN", "OPERATOR",
  "TEMPLATE", "THROW", "TYPENAME", "TYPEDEF", "NAMESPACE", "USING", "NEW",
  "DELETE", "STATIC_CAST", "DYNAMIC_CAST", "CONST_CAST",
  "REINTERPRET_CAST", "OP_LSHIFT_EQ", "OP_RSHIFT_EQ", "OP_LSHIFT",
  "OP_RSHIFT", "OP_DOT_POINTER", "OP_ARROW_POINTER", "OP_ARROW", "OP_INCR",
  "OP_DECR", "OP_PLUS_EQ", "OP_MINUS_EQ", "OP_TIMES_EQ", "OP_DIVIDE_EQ",
  "OP_REMAINDER_EQ", "OP_AND_EQ", "OP_OR_EQ", "OP_XOR_EQ", "OP_LOGIC_AND",
  "OP_LOGIC_OR", "OP_LOGIC_EQ", "OP_LOGIC_NEQ", "OP_LOGIC_LEQ",
  "OP_LOGIC_GEQ", "ELLIPSIS", "DOUBLE_COLON", "OTHER", "VOID", "BOOL",
  "FLOAT", "DOUBLE", "INT", "SHORT", "LONG", "INT64__", "CHAR", "SIGNED",
  "UNSIGNED", "SSIZE_T", "SIZE_T", "IdType", "FloatType", "TypeInt8",
  "TypeUInt8", "TypeInt16", "TypeUInt16", "TypeInt32", "TypeUInt32",
  "TypeInt64", "TypeUInt64", "TypeFloat32", "TypeFloat64", "SetMacro",
  "GetMacro", "SetStringMacro", "GetStringMacro", "SetClampMacro",
  "SetObjectMacro", "GetObjectMacro", "BooleanMacro", "SetVector2Macro",
  "SetVector3Macro", "SetVector4Macro", "SetVector6Macro",
  "GetVector2Macro", "GetVector3Macro", "GetVector4Macro",
  "GetVector6Macro", "SetVectorMacro", "GetVectorMacro",
  "ViewportCoordinateMacro", "WorldCoordinateMacro", "TypeMacro",
  "VTK_BYTE_SWAP_DECL", "';'", "'{'", "'}'", "'='", "':'", "','", "'('",
  "')'", "'<'", "'>'", "'['", "']'", "'~'", "'&'", "'*'", "'%'", "'/'",
  "'-'", "'+'", "'!'", "'|'", "'^'", "'.'", "$accept", "translation_unit",
  "opt_declaration_seq", "$@1", "declaration", "template_declaration",
  "linkage_specification", "namespace_definition", "$@2",
  "namespace_alias_definition", "forward_declaration",
  "simple_forward_declaration", "class_definition", "class_specifier",
  "$@3", "class_head", "$@4", "$@5", "class_key", "member_specification",
  "$@6", "member_access_specifier", "member_declaration",
  "template_member_declaration", "friend_declaration", "opt_base_clause",
  "base_clause", "base_specifier_list", "base_specifier", "opt_virtual",
  "opt_access_specifier", "access_specifier", "enum_definition",
  "enum_specifier", "$@7", "enum_head", "opt_enumerator_list",
  "enumerator_list", "enumerator_definition", "$@8",
  "nested_variable_initialization", "ignored_class", "ignored_class_body",
  "typedef_declaration", "basic_typedef_declaration",
  "typedef_declarator_list", "typedef_declarator_list_cont",
  "typedef_declarator", "typedef_direct_declarator",
  "function_direct_declarator", "$@9", "$@10", "typedef_declarator_id",
  "using_declaration", "using_id", "using_directive", "template_head",
  "$@11", "template_parameter_list", "$@12", "template_parameter", "$@13",
  "$@14", "$@15", "$@16", "$@17", "$@18", "class_or_typename",
  "opt_template_parameter_initializer", "template_parameter_initializer",
  "$@19", "template_parameter_value", "function_definition",
  "function_declaration", "nested_method_declaration",
  "nested_operator_declaration", "method_definition", "method_declaration",
  "operator_declaration", "conversion_function", "$@20", "$@21",
  "conversion_function_id", "operator_function_nr", "$@22",
  "operator_function_sig", "$@23", "operator_function_id", "operator_sig",
  "function_nr", "function_trailer_clause", "function_trailer", "$@24",
  "function_body", "function_sig", "$@25", "function_name",
  "structor_declaration", "$@26", "$@27", "structor_sig", "$@28",
  "opt_ctor_initializer", "mem_initializer_list", "mem_initializer",
  "parameter_declaration_clause", "$@29", "parameter_list", "$@30",
  "parameter_declaration", "$@31", "$@32", "opt_initializer",
  "initializer", "$@33", "variable_declaration", "init_declarator_id",
  "opt_declarator_list", "declarator_list_cont", "$@34", "init_declarator",
  "opt_ptr_operator_seq", "direct_abstract_declarator", "$@35",
  "direct_declarator", "$@36", "p_or_lp_or_la", "lp_or_la",
  "opt_array_or_parameters", "$@37", "$@38", "function_qualifiers",
  "abstract_declarator", "declarator", "opt_declarator_id",
  "declarator_id", "bitfield_size", "opt_array_decorator_seq",
  "array_decorator_seq", "$@39", "array_decorator_seq_impl",
  "array_decorator", "$@40", "array_size_specifier", "$@41",
  "id_expression", "unqualified_id", "qualified_id",
  "nested_name_specifier", "$@42", "identifier_sig", "scope_operator_sig",
  "template_id", "$@43", "simple_id", "identifier",
  "opt_decl_specifier_seq", "decl_specifier2", "decl_specifier_seq",
  "decl_specifier", "storage_class_specifier", "function_specifier",
  "cv_qualifier", "cv_qualifier_seq", "store_type", "store_type_specifier",
  "$@44", "$@45", "type_specifier", "$@46", "tparam_type",
  "tparam_type_specifier2", "$@47", "$@48", "tparam_type_specifier",
  "simple_type_specifier", "type_name", "primitive_type",
  "ptr_operator_seq", "reference", "pointer", "$@49", "pointer_seq",
  "declaration_macro", "$@50", "$@51", "$@52", "$@53", "$@54", "$@55",
  "$@56", "$@57", "$@58", "$@59", "$@60", "$@61", "$@62", "$@63", "$@64",
  "$@65", "$@66", "$@67", "$@68", "$@69", "$@70", "$@71", "opt_comma",
  "operator_id", "operator_id_no_delim", "keyword", "literal",
  "constant_expression", "common_bracket_item", "any_bracket_contents",
  "bracket_pitem", "any_bracket_item", "braces_item",
  "angle_bracket_contents", "braces_contents", "angle_bracket_pitem",
  "angle_bracket_item", "angle_brackets_sig", "$@72", "brackets_sig",
  "$@73", "parentheses_sig", "$@74", "$@75", "$@76", "braces_sig", "$@77",
  "ignored_items", "ignored_expression", "ignored_item",
  "ignored_item_no_semi", "ignored_braces", "ignored_brackets",
  "ignored_parentheses", "ignored_left_parenthesis", 0
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
     375,    59,   123,   125,    61,    58,    44,    40,    41,    60,
      62,    91,    93,   126,    38,    42,    37,    47,    45,    43,
      33,   124,    94,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   144,   145,   146,   147,   146,   148,   148,   148,   148,
     148,   148,   148,   148,   148,   148,   148,   148,   148,   148,
     148,   149,   149,   149,   150,   151,   152,   151,   153,   154,
     154,   155,   155,   156,   156,   158,   157,   160,   159,   161,
     159,   162,   162,   162,   163,   164,   163,   163,   165,   165,
     165,   166,   166,   166,   166,   166,   166,   166,   166,   166,
     166,   166,   166,   166,   167,   167,   168,   168,   168,   168,
     169,   169,   170,   171,   171,   172,   172,   172,   173,   173,
     174,   174,   175,   175,   175,   176,   176,   178,   177,   179,
     179,   180,   180,   181,   181,   181,   182,   183,   182,   184,
     185,   185,   185,   185,   186,   186,   187,   187,   188,   188,
     188,   188,   188,   189,   190,   190,   191,   192,   192,   194,
     195,   193,   196,   197,   198,   198,   198,   198,   198,   198,
     199,   200,   201,   200,   202,   203,   202,   205,   206,   204,
     207,   208,   204,   209,   210,   204,   211,   211,   212,   212,
     214,   213,   215,   215,   216,   216,   216,   216,   217,   218,
     218,   218,   219,   219,   219,   220,   220,   220,   221,   221,
     221,   221,   222,   222,   222,   224,   225,   223,   226,   228,
     227,   230,   229,   231,   232,   233,   234,   234,   236,   235,
     235,   235,   237,   237,   239,   238,   240,   240,   242,   243,
     241,   245,   244,   246,   246,   247,   247,   248,   249,   250,
     249,   251,   252,   251,   251,   254,   255,   253,   256,   256,
     258,   257,   259,   260,   261,   261,   262,   263,   262,   264,
     265,   265,   266,   267,   266,   268,   269,   268,   270,   270,
     270,   271,   271,   272,   273,   274,   272,   272,   275,   275,
     275,   275,   276,   276,   277,   277,   278,   278,   279,   279,
     280,   280,   280,   281,   281,   283,   282,   284,   284,   286,
     285,   287,   288,   287,   289,   289,   290,   290,   291,   291,
     291,   292,   292,   292,   292,   293,   292,   294,   295,   297,
     296,   298,   298,   298,   298,   298,   298,   298,   298,   298,
     298,   298,   298,   298,   298,   298,   298,   298,   298,   298,
     298,   298,   298,   298,   298,   298,   298,   298,   298,   299,
     299,   299,   299,   299,   299,   299,   300,   300,   301,   301,
     301,   301,   302,   302,   303,   303,   303,   304,   304,   304,
     304,   305,   305,   305,   306,   306,   307,   307,   308,   310,
     309,   311,   309,   312,   313,   312,   312,   312,   312,   312,
     314,   316,   315,   317,   315,   318,   318,   318,   318,   318,
     318,   319,   319,   320,   320,   320,   320,   320,   320,   320,
     320,   320,   320,   320,   320,   320,   320,   320,   320,   320,
     320,   320,   320,   320,   321,   321,   321,   321,   321,   321,
     321,   321,   321,   321,   321,   322,   322,   322,   323,   324,
     325,   324,   326,   326,   328,   327,   329,   330,   331,   327,
     332,   327,   333,   327,   334,   335,   327,   336,   327,   337,
     338,   339,   327,   327,   340,   327,   341,   327,   342,   327,
     343,   327,   344,   327,   345,   327,   346,   327,   347,   327,
     348,   327,   349,   327,   327,   327,   327,   350,   350,   351,
     351,   351,   351,   351,   351,   351,   351,   351,   352,   352,
     352,   352,   352,   352,   352,   352,   352,   352,   352,   352,
     352,   352,   352,   352,   352,   352,   352,   352,   352,   352,
     352,   352,   352,   352,   352,   352,   352,   352,   352,   352,
     352,   352,   352,   353,   353,   353,   353,   353,   353,   353,
     353,   353,   353,   353,   353,   353,   353,   353,   353,   353,
     353,   353,   353,   353,   353,   354,   354,   354,   354,   354,
     354,   354,   355,   355,   356,   356,   356,   356,   356,   356,
     356,   356,   356,   356,   356,   357,   357,   358,   358,   358,
     359,   359,   359,   360,   360,   361,   361,   362,   362,   363,
     363,   364,   364,   364,   366,   365,   368,   367,   370,   369,
     371,   369,   372,   369,   374,   373,   375,   375,   376,   376,
     377,   377,   378,   378,   378,   378,   378,   378,   378,   378,
     378,   378,   378,   378,   378,   378,   378,   378,   379,   380,
     381,   382,   382,   382
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     0,     0,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       1,     2,     2,     2,     5,     4,     0,     6,     5,     1,
       2,     3,     4,     4,     5,     0,     5,     0,     4,     0,
       3,     1,     1,     1,     0,     0,     3,     3,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     2,     2,     1,     2,     2,     2,     3,     2,     3,
       0,     1,     2,     1,     3,     1,     3,     3,     0,     1,
       0,     1,     1,     1,     1,     4,     5,     0,     5,     2,
       1,     0,     1,     1,     2,     3,     1,     0,     4,     6,
       3,     4,     2,     3,     5,     3,     1,     2,     5,     5,
       6,     5,     6,     2,     0,     3,     2,     1,     1,     0,
       0,     7,     1,     3,     1,     2,     2,     2,     3,     3,
       4,     3,     0,     5,     1,     0,     4,     0,     0,     5,
       0,     0,     5,     0,     0,     5,     1,     1,     0,     1,
       0,     3,     1,     2,     2,     2,     2,     2,     2,     3,
       2,     3,     2,     3,     3,     2,     3,     4,     2,     1,
       1,     2,     1,     2,     2,     0,     0,     7,     2,     0,
       3,     0,     5,     2,     1,     2,     0,     2,     0,     3,
       1,     2,     3,     1,     0,     5,     1,     1,     0,     0,
       5,     0,     5,     0,     2,     1,     3,     2,     0,     0,
       2,     1,     0,     4,     3,     0,     0,     5,     0,     1,
       0,     3,     4,     2,     0,     2,     0,     0,     4,     2,
       0,     1,     2,     0,     5,     2,     0,     5,     1,     1,
       1,     1,     1,     0,     0,     0,     6,     1,     0,     2,
       2,     3,     1,     2,     1,     2,     0,     1,     1,     3,
       1,     1,     1,     0,     1,     0,     2,     1,     2,     0,
       4,     0,     0,     2,     1,     1,     1,     1,     2,     2,
       2,     2,     2,     3,     3,     0,     5,     1,     1,     0,
       5,     1,     1,     1,     1,     1,     1,     1,     2,     2,
       2,     2,     2,     2,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     0,     2,     1,     1,
       1,     1,     1,     2,     1,     1,     1,     1,     1,     2,
       1,     1,     1,     1,     1,     1,     1,     2,     2,     0,
       3,     0,     4,     1,     0,     3,     1,     1,     2,     2,
       2,     0,     3,     0,     4,     1,     1,     1,     2,     2,
       2,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     1,     1,
       0,     3,     1,     2,     0,     7,     0,     0,     0,     9,
       0,     5,     0,     5,     0,     0,    10,     0,     7,     0,
       0,     0,     9,     6,     0,     7,     0,     7,     0,     7,
       0,     7,     0,     7,     0,     7,     0,     7,     0,     7,
       0,     9,     0,     9,     4,     4,     7,     0,     1,     2,
       2,     3,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     0,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     0,     2,     0,     2,     1,
       1,     1,     1,     1,     0,     4,     0,     4,     0,     4,
       0,     4,     0,     4,     0,     4,     0,     2,     0,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     3,
       3,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       3,     0,     4,     1,     0,   377,   378,   379,   373,   374,
     375,   376,    42,    41,    43,    90,   344,   345,   337,   340,
     342,   343,   341,   338,   184,     0,   354,     0,     0,     0,
     288,   394,   395,   396,   397,   399,   400,   401,   402,   398,
     403,   404,   380,   381,   392,   393,   382,   383,   384,   385,
     386,   387,   388,   389,   390,   391,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    20,     0,     5,
      17,    11,     9,    10,     8,    29,    15,   326,    35,    39,
      14,   326,     0,    12,   106,     7,     6,     0,    16,     0,
       0,     0,     0,   172,     0,     0,    13,     0,   274,   357,
       0,     0,     0,   356,   276,   287,     0,   332,   334,   335,
     336,     0,   230,   349,   353,   372,   371,    18,   293,   291,
     292,   296,   297,   295,   294,   306,   305,   317,   318,   307,
     308,   309,   310,   311,   312,   313,   314,   315,   316,   359,
     275,     0,   277,   339,   132,     0,   377,   378,   379,   373,
     374,   375,   376,   338,   380,   381,   392,   393,   382,   383,
     384,   385,   386,   387,   388,   389,   390,   391,   326,    39,
     326,   357,   356,     0,     0,   319,   321,   320,   324,   325,
     323,   322,   576,    26,     0,     0,     0,   124,     0,     0,
       0,   416,   420,   422,     0,     0,   429,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   300,   298,   299,   303,   304,   302,   301,   230,     0,
      70,   358,   230,    87,     0,    30,    21,    23,    22,     0,
       0,   193,   576,   154,   156,   157,   155,   175,     0,     0,
     178,    19,   285,   162,     0,   160,   198,   278,     0,   277,
     276,   281,   279,   280,   282,   289,   326,    39,   326,   107,
     173,     0,   333,   351,   241,   242,   174,   179,     0,     0,
     158,   186,     0,   226,   218,     0,   263,     0,   197,   258,
     408,   409,   348,   231,   405,   412,   406,   326,   277,     3,
     131,   137,   355,   339,   230,   358,   230,   326,   326,   293,
     291,   292,   296,   297,   295,   294,   122,   118,   114,   117,
     263,   258,     0,     0,     0,   125,     0,   123,   127,   126,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   331,   330,     0,   226,     0,   327,   328,
     329,    44,     0,    40,    71,    31,    70,     0,    91,   359,
       0,   196,     0,   209,   358,     0,   201,   203,   283,   284,
     555,   230,   358,   230,   163,   161,   326,   186,   181,   478,
     479,   480,   481,   482,   483,   484,   485,   486,   492,   493,
     487,   488,   489,   490,   491,   494,   495,   496,   497,   498,
     499,   500,   501,   502,   466,   465,     0,   463,   464,     0,
     474,   475,   469,   468,   470,   471,   472,   473,   476,   477,
     183,   467,   185,   194,     0,   220,   223,   219,   254,     0,
       0,   235,   264,     0,   164,   159,   197,     0,     0,   407,
     413,   350,     4,     0,   134,     0,     0,     0,     0,   114,
       0,     0,   230,   230,     0,   119,   377,   378,   379,   373,
     374,   375,   376,   602,   603,   530,   526,   527,   525,   528,
     529,   531,   506,   505,   507,   519,   509,   511,   510,   512,
     513,   515,   514,   516,   518,   508,   520,   504,   503,   517,
     478,   479,   523,   522,   521,   524,   586,   585,   597,   581,
     576,    25,   592,   588,   593,   601,   590,   591,   576,   589,
     596,   587,   594,   595,   577,   580,   582,   584,   583,   576,
       0,     0,     3,   130,   129,   128,   414,     0,     0,     0,
     424,   427,     0,     0,   434,   438,   442,   446,   436,   440,
     444,   448,   450,   452,   454,   455,     0,    33,   225,   229,
      45,    82,    83,    84,    80,    72,    73,    78,    75,    38,
      85,     0,    92,    93,    96,   196,   192,     0,   215,     0,
       0,   209,     0,   199,     0,     0,    32,     0,   352,   180,
     209,     0,     0,   459,   460,   190,   188,     0,   187,   209,
     222,   227,     0,   236,   255,   269,   266,   267,   261,   262,
     260,   259,   346,   411,    24,   135,   133,     0,     0,     0,
     367,   366,     0,   256,   230,   361,   365,   146,   147,   256,
     256,   109,   113,   116,   111,     0,     0,   108,   230,   209,
       0,     0,     0,    28,     4,     0,   417,   421,   423,     0,
       0,   430,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   457,    48,    49,    50,    36,     0,     0,
       0,    81,     0,    79,     0,    88,    94,    97,   578,   176,
     210,   211,     0,   286,     0,   204,   205,     0,   186,   570,
     572,   540,   574,   562,   538,   563,   568,   564,   290,   566,
     539,   544,   543,   537,   541,   542,   560,   561,   556,   559,
     534,   535,   536,    34,    86,     0,   461,   462,     0,   191,
       0,   230,   548,   549,   221,   547,   532,   243,   272,   268,
     347,   137,   368,   369,   370,   363,   239,   240,   238,   138,
     256,   263,   257,   360,   326,   141,   144,   110,   112,   115,
       0,   598,   599,   600,    27,     0,     0,   425,     0,     0,
     433,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   458,     0,     0,     0,     0,    63,    52,    57,    46,
      59,    53,    56,    54,    51,     0,    58,     0,   169,   170,
      55,     0,     0,   356,     0,     0,    60,    47,    76,    74,
      77,    95,     0,     0,   186,   212,   256,   202,     0,   207,
     200,   545,   545,   557,   545,   555,   545,   182,   189,   195,
     228,   533,   244,   237,   247,     0,     0,   136,   326,   148,
     252,     0,   256,   232,   362,   148,   148,   120,   415,   418,
       0,   428,   431,   435,   439,   443,   447,   437,   441,   445,
     449,     0,     0,   456,    68,     0,    66,     0,     0,   356,
       0,     0,    61,    64,    65,     0,   165,    62,     0,   171,
       0,   168,   197,    98,    99,   579,   177,   214,   215,   216,
     206,     0,     0,     0,     0,     0,     0,   209,   270,   273,
     364,   150,   139,   149,   233,   253,   142,   145,   248,     0,
     578,     0,     0,     0,   576,   578,   102,   358,     0,    67,
       0,    69,     0,   166,     0,   213,   218,   551,   552,   571,
     550,   546,   573,   554,   575,   553,   558,   569,   565,   567,
       0,     0,   243,   121,   419,     0,   432,   451,   453,     0,
       0,   100,     0,     0,   103,   358,   167,   217,   245,   151,
     152,   234,   250,   249,     0,   426,   578,   105,     0,   101,
     248,   153,   251,     0,   246,   104
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     2,     4,    79,    80,    81,    82,   324,    83,
      84,    85,    86,    87,   229,    88,   366,   230,   248,   560,
     668,   669,   779,   780,   781,   363,   364,   565,   566,   674,
     670,   567,    90,    91,   368,    92,   571,   572,   573,   802,
     237,   856,   906,    93,    94,   458,   464,   459,   316,   317,
     639,   898,   318,    95,   196,    96,    97,   301,   453,   731,
     454,   455,   829,   456,   835,   457,   836,   629,   892,   893,
     931,   949,    98,    99,   100,   101,   786,   787,   788,   103,
     373,   804,   104,   276,   387,   277,   590,   278,   105,   280,
     432,   598,   718,   243,   281,   599,   254,   789,   377,   688,
     256,   581,   583,   685,   686,   577,   578,   680,   878,   681,
     682,   916,   436,   437,   602,   106,   283,   355,   434,   721,
     356,   357,   739,   932,   284,   727,   740,   285,   823,   887,
     960,   933,   831,   439,   741,   742,   611,   441,   442,   443,
     606,   607,   728,   825,   826,   197,   108,   181,   151,   375,
     111,   112,   152,   380,   114,   115,   228,   358,   249,   117,
     118,   119,   120,   613,   250,   122,   297,   386,   123,   155,
     623,   624,   744,   828,   625,   124,   125,   126,   293,   294,
     295,   448,   296,   127,   645,   333,   756,   899,   334,   335,
     649,   840,   650,   338,   759,   901,   653,   657,   654,   658,
     655,   659,   656,   660,   661,   662,   772,   430,   703,   704,
     705,   724,   725,   881,   920,   921,   926,   584,   883,   707,
     708,   709,   815,   710,   816,   711,   814,   811,   812,   712,
     813,   322,   803,   524,   525,   526,   527,   528,   529
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -809
static const yytype_int16 yypact[] =
{
    -809,    39,    69,  -809,  4280,   111,   138,   183,   210,   242,
     258,   279,  -809,  -809,  -809,  4557,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,   125,  -809,    -6,  -809,  5365,   339,  4653,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,   -44,   -18,    86,    97,   118,   153,   154,   166,
     178,   190,   199,   206,   228,   229,    58,    70,    79,    81,
      94,    96,   101,   114,   131,   165,   170,   179,   181,   186,
     208,   227,   235,   239,   248,   253,   256,  -809,   435,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  4557,
    -809,  -809,   283,  -809,  -809,  -809,  -809,  5269,  -809,   -46,
     -46,   -46,   -46,  -809,   290,  5461,  -809,   304,  -809,   305,
    4766,   363,  4557,     0,  -809,   321,  5173,  -809,  -809,  -809,
    -809,  1122,    37,  -809,  -809,  -809,  -809,  -809,    -5,    15,
      23,    40,    45,    60,    93,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,   331,
    -809,  4932,   363,   335,   330,  4557,    -5,    15,    23,    40,
      45,    60,    93,   450,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  4557,
    -809,  -809,   363,  5365,  4800,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,   340,  4557,  4557,   346,  -809,  4766,  4557,
    4966,  -809,  -809,  -809,  4966,  4966,  -809,  4966,  4966,  4966,
    4966,  4966,  4966,  4966,  4966,  4966,  4966,  4966,  4966,  4966,
    4966,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  5006,   366,
     360,   251,  5006,  -809,  4557,  -809,  -809,  -809,  -809,  5269,
    4911,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  4557,  5461,
    -809,  -809,  -809,  -809,   362,  -809,  -809,  -809,   363,   -28,
     365,  -809,  -809,  -809,  -809,  -809,  -809,  4557,  -809,  -809,
    -809,  4766,  -809,  -809,  -809,  -809,  -809,  -809,   380,  5094,
    -809,  -809,   381,  -809,   385,  1010,   379,  4766,   363,    35,
    -809,   371,  -809,  -809,  -809,  -809,    37,  -809,   363,  -809,
    -809,   162,  -809,  -809,  5040,   200,  5040,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
     146,   387,  1342,   386,   389,  -809,   392,  -809,  -809,  -809,
    4021,  4766,   388,  4966,  4966,  4966,   390,   391,  4966,   393,
     394,   401,   402,   411,   413,   414,   415,   416,   418,   419,
     420,   421,   424,  -809,  -809,   400,  -809,  4800,  -809,  -809,
    -809,  -809,  4591,  -809,  -809,  -809,   360,   425,  4966,  -809,
    4766,  -809,  1483,   423,  -809,   468,  -809,   422,  -809,  -809,
    -809,  5006,   311,  5006,  -809,  -809,  -809,  -809,  -809,   384,
     427,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,   426,  -809,  -809,   428,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,    31,  -809,   -48,  -809,  -809,  -809,  -809,   431,
    4800,  -809,  -809,   430,  -809,  -809,   363,   465,   376,  -809,
    -809,  1230,   429,   158,  -809,  5497,    51,   517,   434,  -809,
    4800,   441,  5040,  5040,   -11,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
     442,   363,  -809,  -809,  -809,  -809,  -809,   438,   437,   439,
    -809,  -809,   440,  5461,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  4966,  -809,   443,  -809,
      41,  -809,  -809,  -809,   458,   448,  -809,   526,  -809,  -809,
    -809,   445,   449,  -809,   453,   455,  -809,   452,  -809,   363,
     321,   423,  4557,  -809,  3457,   461,  -809,   462,  1230,    31,
     423,   454,   457,  -809,  -809,  -809,  -809,   567,  -809,   423,
    -809,  -809,  3739,  -809,  -809,  -809,   430,  -809,  -809,  -809,
    -809,  -809,  -809,   376,  -809,  -809,  -809,  4557,  4557,  4557,
    -809,   363,  5461,  4702,    37,  -809,  -809,  -809,  -809,  4702,
    4702,  -809,   464,  -809,  -809,   466,   472,  -809,    37,   423,
    1624,  1765,  1906,  -809,   471,  5461,  -809,  -809,  -809,  5461,
    5461,  -809,   467,  5461,  5461,  5461,  5461,  5461,  5461,  5461,
    5461,  5461,  5461,   470,  -809,  -809,  -809,  -809,  4161,   473,
    4557,  -809,  4591,  -809,  4557,  -809,  4966,  -809,  -809,  -809,
     474,  -809,  5461,  -809,   469,   475,  -809,    43,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,   476,  -809,  -809,    49,  -809,
     477,    37,  -809,  -809,  3739,  -809,  -809,   205,   478,  -809,
    -809,   162,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
     914,   379,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
     479,  -809,  -809,  -809,  -809,   481,  5461,  -809,   483,  5461,
    -809,   494,   498,   500,   501,   502,   503,   508,   509,   480,
     506,  -809,   510,  1156,  4862,    43,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  4495,  -809,   -46,  -809,  -809,
    -809,   518,  4766,   -21,  4399,  1122,  -809,  -809,  -809,  -809,
    -809,  -809,  3739,  2188,  -809,   528,  4702,  -809,  4557,  -809,
      31,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,   512,  3739,  -809,  -809,   516,
    -809,   513,  4702,  -809,  1230,   516,   516,  -809,  -809,  -809,
     519,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,   629,   636,  -809,  -809,  4751,  -809,   504,   -46,   -28,
    4495,  4911,  -809,  -809,  -809,  4495,  -809,  -809,   529,  -809,
    4766,  -809,  -809,  3739,  -809,  -809,    31,  -809,  -809,  -809,
    -809,  2893,  3034,  2752,  3175,  3598,  3316,   423,  -809,  3739,
    1230,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,   524,
    -809,   525,   527,   530,  -809,  -809,  -809,   327,  4751,  -809,
     504,  -809,  4751,  -809,   536,  -809,   385,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
     531,  3880,   205,   110,  -809,  2329,  -809,  -809,  -809,  2047,
    2470,  -809,   327,  4751,  -809,   334,  -809,  -809,  -809,  3880,
    -809,  -809,  -809,  -809,    43,  -809,  -809,  -809,   334,  -809,
    -809,  -809,  -809,  2611,   110,  -809
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -809,  -809,  -275,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -640,   -93,   -92,   -25,  -809,  -809,  -809,  -809,     4,  -809,
    -809,  -809,  -809,  -809,  -809,   294,  -809,  -809,   -10,  -809,
    -809,    99,     7,   -20,  -809,  -809,  -809,  -809,   -15,  -809,
    -809,  -177,  -530,    27,  -100,  -260,   241,    64,  -809,  -809,
    -809,  -809,   238,    38,  -809,  -809,  -439,  -809,  -809,  -809,
     -26,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -426,  -809,
    -809,  -809,   606,  -809,  -809,  -809,   -78,   -63,    17,   -55,
    -809,  -809,  -164,  -240,  -809,  -809,  -809,  -189,   -41,  -277,
    -370,  -809,  -809,   -64,  -809,  -809,   -99,   -52,  -809,  -809,
    -809,  -809,  -809,  -809,   -77,  -549,  -809,  -809,  -809,  -148,
    -809,  -809,  -183,  -809,  -809,    66,   378,  -202,   382,  -809,
      16,   -95,  -581,  -809,  -139,  -809,  -809,  -809,  -185,  -809,
    -809,  -217,  -809,  -809,  -809,   -59,  -809,     9,  -698,  -809,
    -809,   142,  -809,  -809,  -809,    -1,   -70,    -4,    -3,  -809,
     -67,    -9,    29,  -809,   286,   -22,   -12,  -809,     8,     3,
    -809,  -809,  -396,  -809,    59,  -809,  -809,  -809,   -34,  -809,
    -809,  -809,  -809,  -809,  -809,  -195,  -462,   395,  -272,   459,
     460,  -809,  -809,    85,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,
    -809,  -809,  -809,  -809,  -809,  -809,  -809,  -809,    -7,  -211,
       6,  -728,  -561,  -398,  -532,  -134,  -809,   -58,  -809,  -796,
    -809,  -809,  -809,  -809,  -809,    48,  -809,  -809,  -809,  -809,
    -809,  -227,  -808,  -809,  -687,  -809,  -809,  -668,  -809
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -411
static const yytype_int16 yytable[] =
{
     109,   110,   178,   107,   235,   236,   193,   180,    89,   329,
     445,   150,   116,   440,   149,   372,   269,   589,   630,   809,
     199,   102,   282,   706,   452,   150,   198,   292,   777,   824,
     367,   179,   684,   113,   328,   183,   244,   245,   246,     3,
     257,   715,   262,   258,    30,   319,   461,   444,   745,   746,
     720,    30,   612,   473,   474,   253,   182,   595,   255,   689,
     690,   270,   286,   121,   664,   665,   666,  -319,   596,    -2,
     726,   627,    30,   600,   873,   241,   242,  -306,   601,   232,
     279,   257,   273,  -306,   258,   150,   184,  -321,   231,   628,
     750,   266,   935,   445,   110,  -320,   268,   940,   889,  -197,
    -277,    89,   261,  -305,   264,   239,  -197,   862,   263,  -305,
     637,   522,  -324,   271,   102,   638,   875,  -325,   287,   272,
     267,  -277,   701,   154,  -319,   320,   182,   520,   257,   262,
     444,   258,  -323,   854,   182,   950,   952,   153,   953,   259,
     701,   282,   535,   264,  -321,   182,   438,   954,   963,   273,
     288,   150,  -320,   961,   302,   597,   240,   330,   307,   830,
     447,   522,  -196,   308,   667,  -322,   304,   534,   306,  -324,
     515,   290,   291,   264,  -325,   150,   696,   520,   305,   585,
     298,   587,  -140,  -319,   270,   200,   272,   179,   282,  -323,
     150,   150,   821,   325,   326,   263,   331,   201,  -143,   279,
    -140,   257,   635,   636,   258,   273,   202,  -317,   203,   460,
    -321,   460,   182,  -317,   266,   273,   384,   730,  -318,   385,
     258,   204,  -322,   205,  -318,   879,   286,   298,   206,   785,
     150,   359,  -293,   369,   824,   359,   271,   370,  -293,  -307,
    -319,   207,   272,   267,   150,  -307,   279,   374,   875,   378,
     379,   895,   272,   875,   381,  -320,   383,   644,   208,  -291,
     626,   257,   701,   150,   258,  -291,   382,  -321,   182,   288,
     726,   282,   431,   465,  -308,  -309,   875,  -265,   182,   264,
    -308,  -309,  -324,   640,   615,   451,   962,  -310,   616,   379,
     330,   641,   209,  -310,   726,   462,   463,   210,   286,  -311,
     259,   604,   642,   258,  -292,  -311,   211,   359,   212,   359,
    -292,  -312,  -320,   213,  -325,   521,   446,  -312,   810,   530,
    -313,   319,   -37,   431,   706,   -37,  -313,  -314,   523,   279,
    -323,  -296,   822,  -314,   857,   214,  -265,  -296,   930,  -324,
     701,   821,   185,   186,   187,   188,   189,   190,   191,  -315,
    -316,  -322,   531,   580,   215,  -315,  -316,   821,   150,   182,
     298,   568,   216,  -297,   701,   521,   217,   460,   460,  -297,
     706,  -325,   365,   -37,   588,   218,   -37,   941,   523,  -295,
     219,   286,   944,   220,   359,  -295,   359,  -323,   706,   185,
     186,   187,   188,   189,   190,   191,   260,  -410,  -410,   446,
    -294,   320,    16,    17,   579,   233,  -294,   289,  -322,   896,
     897,   701,   941,   944,   882,   959,   884,   247,   886,   701,
     701,   701,   701,   701,   701,   251,  -275,   701,   959,   522,
     522,   522,   586,   -37,   876,    30,   -37,   379,   221,   222,
     223,   224,   225,   226,   227,   520,   520,   520,   365,   904,
     265,   620,   905,   -89,   359,   586,   904,   299,    30,   905,
     300,   192,   303,   622,   323,   359,   359,   327,   832,   701,
     321,   185,   186,   187,   188,   189,   190,   191,   608,   609,
     610,   561,   562,   563,   621,   362,   332,   701,   361,   376,
     336,   337,  -196,   339,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   388,   433,   435,
    -265,   532,   447,   533,   536,   591,   540,   541,   871,   543,
     544,   557,   264,    12,    13,    14,   371,   545,   546,   743,
      16,    17,    18,    19,    20,    21,    22,   547,   163,   548,
     549,   550,   551,   460,   552,   553,   570,   582,   554,   555,
     556,  -208,   614,    25,   593,   631,   673,   260,   592,   603,
     594,   605,   634,   643,   646,   647,   651,   648,   675,   601,
     683,   321,   182,   371,   672,   676,   778,   677,   150,   678,
     679,   687,   713,   714,   871,   719,   716,   747,   735,   717,
     638,   359,   522,   748,   754,   760,   771,   807,   797,   877,
     805,   808,   652,   868,   817,   819,   851,   837,   520,   838,
    -271,   841,   264,   150,   150,   150,   732,   733,   734,   537,
     538,   539,   843,   360,   542,   272,   844,   360,   845,   846,
     847,   848,   852,   521,   521,   521,   849,   850,   853,   867,
     891,   894,   902,   321,   888,   900,   523,   523,   523,   903,
     913,   182,   934,   936,   574,   937,   575,   946,   938,   948,
     569,   801,   799,   671,   109,   792,   150,   791,   150,   798,
     150,   568,    89,   800,   182,   782,   794,   939,   182,   182,
     909,   914,   182,   182,   182,   182,   182,   182,   182,   182,
     182,   182,   235,   863,   269,   783,   282,   793,   633,   360,
     632,   360,   749,   238,   755,   827,   784,   864,   757,   758,
     858,   182,   761,   762,   763,   764,   765,   766,   767,   768,
     769,   770,   257,   866,   522,   258,   321,   795,   522,   522,
     915,   880,   834,   947,   790,   559,   286,   820,   558,   270,
     520,   806,   869,   964,   520,   520,   321,   951,   729,   925,
     833,   279,   522,   796,   279,   449,   450,   885,     0,     0,
     273,     0,   282,     0,   235,   199,   818,     0,   520,   266,
     150,   198,     0,   580,   268,     0,   360,   855,   360,     0,
       0,   860,   792,     0,   264,   182,     0,     0,   182,    89,
       0,   870,     0,   865,   911,     0,   521,   272,   267,     0,
     257,     0,   859,   258,   150,   270,     0,   687,   869,   523,
     270,     0,     0,   869,   859,   839,   890,     0,   842,     0,
     279,   298,     0,   859,   872,     0,   273,     0,     0,   279,
       0,   273,   861,     0,     0,     0,     0,   359,     0,   580,
     266,     0,   663,     0,   861,     0,   360,     0,     0,     0,
     264,   150,     0,     0,   907,     0,     0,   360,   360,     0,
       0,   908,   870,   272,   912,   910,     0,     0,   272,   267,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   859,
     872,     0,     0,   359,   859,     0,     0,     0,     0,   298,
       0,     0,     0,     0,   150,     0,     0,   942,   150,   321,
       0,   945,     0,   272,   943,   321,   321,   309,   310,   311,
     312,   313,   314,   315,   736,   737,     0,     0,   521,     0,
       0,     0,   521,   521,     0,     0,     0,     0,     0,   150,
       0,   523,   958,     0,     0,   523,   523,     0,     0,     0,
       0,     0,     0,     0,   260,     0,   521,     0,     0,     0,
       0,     0,   574,     0,     0,     0,     0,     0,     0,   523,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   702,
       0,     0,     0,   360,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   702,     0,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   148,   309,   310,   311,   312,   313,   314,   315,
     274,   275,     0,     0,     0,     0,   321,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   738,     0,     0,     0,     0,     0,    78,   290,   291,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   371,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   371,     0,     0,     0,     0,     0,     0,     0,     0,
     371,   289,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   321,     0,     0,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   321,   702,
       0,     0,     0,     0,     0,   128,   129,   130,   131,   132,
     133,   134,   274,   275,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    78,   290,   291,   371,   371,     0,     0,
       0,   371,     0,     0,     0,     0,     0,    24,     0,     5,
       6,     7,     8,     9,    10,    11,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    12,    13,    14,   234,     0,
       0,     0,    16,    17,    18,    19,    20,    21,    22,     0,
     163,    24,    25,     0,    26,     0,     0,   702,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,   702,     0,     0,     0,     0,     0,     0,    30,   360,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    78,    16,    17,    18,    19,
      20,    21,    22,   353,   163,     0,     0,     0,   702,   354,
       0,     0,     0,     0,     0,     0,   702,   702,   702,   702,
     702,   702,     0,     0,   702,   360,     0,     0,     0,    78,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   702,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   702,   466,   467,   468,   469,   470,
     471,   472,   473,   474,   475,   476,   477,   478,   479,   480,
     481,   482,   483,   484,   485,   486,   487,   488,   489,     0,
       0,   490,   491,     0,   492,     0,   493,   494,   495,   496,
     497,   498,   499,     0,   500,   501,   502,   503,   504,   505,
     391,   392,   393,   394,   395,   396,   397,   398,   399,   400,
     401,   402,   403,   404,   405,   406,   407,   408,   409,   410,
     411,   412,   413,   506,   507,   508,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   509,   510,   511,   512,   513,   514,   515,
       0,   516,   517,   518,     0,   420,   421,   422,   423,   424,
     425,   426,   427,   428,   429,   519,   466,   467,   468,   469,
     470,   471,   472,   473,   474,   475,   476,   477,   478,   479,
     480,   481,   482,   483,   484,   485,   486,   487,   488,   489,
       0,     0,   490,   491,     0,   492,     0,   493,   494,   495,
     496,   497,   498,   499,     0,   500,   501,   502,   503,   504,
     505,   391,   392,   393,   394,   395,   396,   397,   398,   399,
     400,   401,   402,   403,   404,   405,   406,   407,   408,   409,
     410,   411,   412,   413,   506,   507,   508,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,   164,   165,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,   177,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   509,   510,   576,   512,   513,   514,
     515,     0,   516,   517,   518,     0,   420,   421,   422,   423,
     424,   425,   426,   427,   428,   429,   519,   466,   467,   468,
     469,   470,   471,   472,   473,   474,   475,   476,   477,   478,
     479,   480,   481,   482,   483,   484,   485,   486,   487,   488,
     489,     0,     0,   490,   491,     0,   492,     0,   493,   494,
     495,   496,   497,   498,   499,     0,   500,   501,   502,   503,
     504,   505,   391,   392,   393,   394,   395,   396,   397,   398,
     399,   400,   401,   402,   403,   404,   405,   406,   407,   408,
     409,   410,   411,   412,   413,   506,   507,   508,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   509,   510,   751,   512,   513,
     514,   515,     0,   516,   517,   518,     0,   420,   421,   422,
     423,   424,   425,   426,   427,   428,   429,   519,   466,   467,
     468,   469,   470,   471,   472,   473,   474,   475,   476,   477,
     478,   479,   480,   481,   482,   483,   484,   485,   486,   487,
     488,   489,     0,     0,   490,   491,     0,   492,     0,   493,
     494,   495,   496,   497,   498,   499,     0,   500,   501,   502,
     503,   504,   505,   391,   392,   393,   394,   395,   396,   397,
     398,   399,   400,   401,   402,   403,   404,   405,   406,   407,
     408,   409,   410,   411,   412,   413,   506,   507,   508,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
     164,   165,   166,   167,   168,   169,   170,   171,   172,   173,
     174,   175,   176,   177,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   509,   510,     0,   512,
     513,   514,   515,     0,   516,   517,   518,   752,   420,   421,
     422,   423,   424,   425,   426,   427,   428,   429,   519,   466,
     467,   468,   469,   470,   471,   472,   473,   474,   475,   476,
     477,   478,   479,   480,   481,   482,   483,   484,   485,   486,
     487,   488,   489,     0,     0,   490,   491,     0,   492,     0,
     493,   494,   495,   496,   497,   498,   499,     0,   500,   501,
     502,   503,   504,   505,   391,   392,   393,   394,   395,   396,
     397,   398,   399,   400,   401,   402,   403,   404,   405,   406,
     407,   408,   409,   410,   411,   412,   413,   506,   507,   508,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,   164,   165,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   509,   510,     0,
     512,   513,   514,   515,   753,   516,   517,   518,     0,   420,
     421,   422,   423,   424,   425,   426,   427,   428,   429,   519,
     466,   467,   468,   469,   470,   471,   472,   473,   474,   475,
     476,   477,   478,   479,   480,   481,   482,   483,   484,   485,
     486,   487,   488,   489,     0,     0,   490,   491,     0,   492,
       0,   493,   494,   495,   496,   497,   498,   499,     0,   500,
     501,   502,   503,   504,   505,   391,   392,   393,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   405,
     406,   407,   408,   409,   410,   411,   412,   413,   506,   507,
     508,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,   164,   165,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   509,   510,
     956,   512,   513,   514,   515,     0,   516,   517,   518,     0,
     420,   421,   422,   423,   424,   425,   426,   427,   428,   429,
     519,   466,   467,   468,   469,   470,   471,   472,   473,   474,
     475,   476,   477,   478,   479,   480,   481,   482,   483,   484,
     485,   486,   487,   488,   489,     0,     0,   490,   491,     0,
     492,     0,   493,   494,   495,   496,   497,   498,   499,     0,
     500,   501,   502,   503,   504,   505,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   506,
     507,   508,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,   164,   165,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   874,
     510,     0,   512,   513,   514,   515,     0,   516,   517,   518,
       0,   420,   421,   422,   423,   424,   425,   426,   427,   428,
     429,   519,   466,   467,   468,   469,   470,   471,   472,   473,
     474,   475,   476,   477,   478,   479,   480,   481,   482,   483,
     484,   485,   486,   487,   488,   489,     0,     0,   490,   491,
       0,   492,     0,   493,   494,   495,   496,   497,   498,   499,
       0,   500,   501,   502,   503,   504,   505,   391,   392,   393,
     394,   395,   396,   397,   398,   399,   400,   401,   402,   403,
     404,   405,   406,   407,   408,   409,   410,   411,   412,   413,
     506,   507,   508,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,   175,   176,   177,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   510,     0,   512,   513,   514,   515,   955,   516,   517,
     518,     0,   420,   421,   422,   423,   424,   425,   426,   427,
     428,   429,   519,   466,   467,   468,   469,   470,   471,   472,
     473,   474,   475,   476,   477,   478,   479,   480,   481,   482,
     483,   484,   485,   486,   487,   488,   489,     0,     0,   490,
     491,     0,   492,     0,   493,   494,   495,   496,   497,   498,
     499,     0,   500,   501,   502,   503,   504,   505,   391,   392,
     393,   394,   395,   396,   397,   398,   399,   400,   401,   402,
     403,   404,   405,   406,   407,   408,   409,   410,   411,   412,
     413,   506,   507,   508,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,   164,   165,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   957,   510,     0,   512,   513,   514,   515,     0,   516,
     517,   518,     0,   420,   421,   422,   423,   424,   425,   426,
     427,   428,   429,   519,   466,   467,   468,   469,   470,   471,
     472,   473,   474,   475,   476,   477,   478,   479,   480,   481,
     482,   483,   484,   485,   486,   487,   488,   489,     0,     0,
     490,   491,     0,   492,     0,   493,   494,   495,   496,   497,
     498,   499,     0,   500,   501,   502,   503,   504,   505,   391,
     392,   393,   394,   395,   396,   397,   398,   399,   400,   401,
     402,   403,   404,   405,   406,   407,   408,   409,   410,   411,
     412,   413,   506,   507,   508,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,   164,   165,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,   177,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   965,   510,     0,   512,   513,   514,   515,     0,
     516,   517,   518,     0,   420,   421,   422,   423,   424,   425,
     426,   427,   428,   429,   519,   466,   467,   468,   469,   470,
     471,   472,   689,   690,   475,   476,   477,   478,   479,   480,
     481,   482,   483,   484,   485,   486,   487,   488,   489,     0,
       0,   490,   491,     0,   492,     0,   493,   494,   495,   496,
     497,   498,   499,     0,   500,   501,   502,   503,   504,   505,
     391,   392,   393,   394,   395,   396,   397,   398,   399,   400,
     401,   402,   403,   404,   405,   406,   407,   408,   409,   410,
     411,   412,   413,     0,   691,     0,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   923,   692,   924,   917,   694,   918,   696,
       0,   722,   723,   699,     0,   420,   421,   422,   423,   424,
     425,   426,   427,   428,   429,   700,   466,   467,   468,   469,
     470,   471,   472,   689,   690,   475,   476,   477,   478,   479,
     480,   481,   482,   483,   484,   485,   486,   487,   488,   489,
       0,     0,   490,   491,     0,   492,     0,   493,   494,   495,
     496,   497,   498,   499,     0,   500,   501,   502,   503,   504,
     505,   391,   392,   393,   394,   395,   396,   397,   398,   399,
     400,   401,   402,   403,   404,   405,   406,   407,   408,   409,
     410,   411,   412,   413,     0,   691,     0,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,   164,   165,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,   177,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   692,     0,   917,   694,   918,
     696,   919,   722,   723,   699,     0,   420,   421,   422,   423,
     424,   425,   426,   427,   428,   429,   700,   466,   467,   468,
     469,   470,   471,   472,   689,   690,   475,   476,   477,   478,
     479,   480,   481,   482,   483,   484,   485,   486,   487,   488,
     489,     0,     0,   490,   491,     0,   492,     0,   493,   494,
     495,   496,   497,   498,   499,     0,   500,   501,   502,   503,
     504,   505,   391,   392,   393,   394,   395,   396,   397,   398,
     399,   400,   401,   402,   403,   404,   405,   406,   407,   408,
     409,   410,   411,   412,   413,     0,   691,     0,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   692,     0,   917,   694,
     918,   696,   922,   722,   723,   699,     0,   420,   421,   422,
     423,   424,   425,   426,   427,   428,   429,   700,   466,   467,
     468,   469,   470,   471,   472,   689,   690,   475,   476,   477,
     478,   479,   480,   481,   482,   483,   484,   485,   486,   487,
     488,   489,     0,     0,   490,   491,     0,   492,     0,   493,
     494,   495,   496,   497,   498,   499,     0,   500,   501,   502,
     503,   504,   505,   391,   392,   393,   394,   395,   396,   397,
     398,   399,   400,   401,   402,   403,   404,   405,   406,   407,
     408,   409,   410,   411,   412,   413,     0,   691,     0,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
     164,   165,   166,   167,   168,   169,   170,   171,   172,   173,
     174,   175,   176,   177,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   692,     0,   917,
     694,   918,   696,   927,   722,   723,   699,     0,   420,   421,
     422,   423,   424,   425,   426,   427,   428,   429,   700,   466,
     467,   468,   469,   470,   471,   472,   689,   690,   475,   476,
     477,   478,   479,   480,   481,   482,   483,   484,   485,   486,
     487,   488,   489,     0,     0,   490,   491,     0,   492,     0,
     493,   494,   495,   496,   497,   498,   499,     0,   500,   501,
     502,   503,   504,   505,   391,   392,   393,   394,   395,   396,
     397,   398,   399,   400,   401,   402,   403,   404,   405,   406,
     407,   408,   409,   410,   411,   412,   413,     0,   691,     0,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,   164,   165,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   692,     0,
     917,   694,   918,   696,     0,   722,   723,   699,   929,   420,
     421,   422,   423,   424,   425,   426,   427,   428,   429,   700,
     466,   467,   468,   469,   470,   471,   472,   689,   690,   475,
     476,   477,   478,   479,   480,   481,   482,   483,   484,   485,
     486,   487,   488,   489,     0,     0,   490,   491,     0,   492,
       0,   493,   494,   495,   496,   497,   498,   499,     0,   500,
     501,   502,   503,   504,   505,   391,   392,   393,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   405,
     406,   407,   408,   409,   410,   411,   412,   413,     0,   691,
       0,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,   164,   165,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   692,
       0,   693,   694,   695,   696,     0,   697,   698,   699,     0,
     420,   421,   422,   423,   424,   425,   426,   427,   428,   429,
     700,   466,   467,   468,   469,   470,   471,   472,   689,   690,
     475,   476,   477,   478,   479,   480,   481,   482,   483,   484,
     485,   486,   487,   488,   489,     0,     0,   490,   491,     0,
     492,     0,   493,   494,   495,   496,   497,   498,   499,     0,
     500,   501,   502,   503,   504,   505,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,     0,
     691,     0,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,   164,   165,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     692,     0,   693,   694,   695,   696,     0,   697,   928,   699,
       0,   420,   421,   422,   423,   424,   425,   426,   427,   428,
     429,   700,   466,   467,   468,   469,   470,   471,   472,   689,
     690,   475,   476,   477,   478,   479,   480,   481,   482,   483,
     484,   485,   486,   487,   488,   489,     0,     0,   490,   491,
       0,   492,     0,   493,   494,   495,   496,   497,   498,   499,
       0,   500,   501,   502,   503,   504,   505,   391,   392,   393,
     394,   395,   396,   397,   398,   399,   400,   401,   402,   403,
     404,   405,   406,   407,   408,   409,   410,   411,   412,   413,
       0,   691,     0,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,   175,   176,   177,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   692,     0,     0,   694,     0,   696,     0,   722,   723,
     699,     0,   420,   421,   422,   423,   424,   425,   426,   427,
     428,   429,   700,   466,   467,   468,   469,   470,   471,   472,
     689,   690,   475,   476,   477,   478,   479,   480,   481,   482,
     483,   484,   485,   486,   487,   488,   489,     0,     0,   490,
     491,     0,   492,     0,   493,   494,   495,   496,   497,   498,
     499,     0,   500,   501,   502,   503,   504,   505,   391,   392,
     393,   394,   395,   396,   397,   398,   399,   400,   401,   402,
     403,   404,   405,   406,   407,   408,   409,   410,   411,   412,
     413,     0,   691,     0,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,   164,   165,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   692,     0,     0,   694,     0,   696,     0,   697,
       0,   699,     0,   420,   421,   422,   423,   424,   425,   426,
     427,   428,   429,   700,   156,   157,   158,   159,   160,   161,
     162,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      12,    13,    14,   234,     0,     0,     0,    16,    17,    18,
      19,    20,    21,    22,     0,   163,     0,     0,     0,    26,
       0,     0,     0,   389,   390,     0,     0,     0,     0,   391,
     392,   393,   394,   395,   396,   397,   398,   399,   400,   401,
     402,   403,   404,   405,   406,   407,   408,   409,   410,   411,
     412,   413,     0,    30,     0,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,   164,   165,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,   177,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   414,     0,   415,   416,     0,
     417,   418,   419,     0,   420,   421,   422,   423,   424,   425,
     426,   427,   428,   429,     5,     6,     7,     8,     9,    10,
      11,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      12,    13,    14,    15,     0,     0,     0,    16,    17,    18,
      19,    20,    21,    22,   773,   163,    24,    25,     0,    26,
      27,     0,   774,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    30,     0,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,   775,   776,     5,     6,     7,     8,     9,    10,    11,
       0,     0,     0,     0,    78,     0,     0,     0,     0,    12,
      13,    14,    15,     0,     0,     0,    16,    17,    18,    19,
      20,    21,    22,     0,    23,    24,    25,     0,    26,    27,
      28,    29,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    30,     0,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
       0,    77,     5,     6,     7,     8,     9,    10,    11,     0,
       0,     0,     0,    78,     0,     0,     0,     0,    12,    13,
      14,    15,     0,     0,     0,    16,    17,    18,    19,    20,
      21,    22,     0,   163,    24,     0,     0,    26,    27,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    30,     0,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,     5,     6,
       7,     8,     9,    10,    11,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    12,    13,    14,   234,     0,     0,
       0,    16,    17,    18,    19,    20,    21,    22,     0,   163,
      24,     0,    78,    26,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     128,   129,   130,   131,   132,   133,   134,    30,     0,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,   128,   129,   130,   131,   132,   133,
     134,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   561,   562,   563,     0,     0,     0,
       0,   564,     0,     0,     0,     0,     0,     0,    78,    30,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   147,   148,   128,   129,   130,   131,
     132,   133,   134,    30,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
      78,   194,     0,   195,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   309,   310,   311,   312,   313,
     314,   315,   736,   737,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    78,    30,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     147,   148,     0,     0,   128,   129,   130,   131,   132,   133,
     134,     0,     0,     0,     0,     0,     0,     0,     0,   128,
     129,   130,   131,   132,   133,   134,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    78,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,    24,   252,   309,   310,   311,   312,   313,   314,   315,
     274,   275,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    30,     0,     0,     0,     0,     0,   738,
       0,     0,     0,     0,     0,    78,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
       0,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   148,   128,   129,   130,   131,   132,
     133,   134,     0,   904,     0,     0,   905,     0,     0,     0,
       0,     0,     0,     0,    78,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,    78,
     194,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   128,   129,   130,   131,   132,   133,
     134,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    78,    30,   128,   129,   130,   131,   132,
     133,   134,     0,     0,     0,     0,    24,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,     0,     0,     0,     0,     0,     0,     0,   252,   309,
     310,   311,   312,   313,   314,   315,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    78,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
       0,     0,     0,     0,     0,     0,     0,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,     0,    16,    17,    18,    19,    20,    21,    22,   353,
     163,     0,     0,     0,    78,   354,     0,     0,     0,     0,
       0,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   148,    78,    16,    17,    18,    19,
      20,    21,    22,   353,   163,     0,     0,     0,     0,   354,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,     0,     0,     0,     0,     0,     0,     0,     0,    78,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,     0,     0,  -224,     0,     0,
       0,     0,     0,     0,     0,     0,   389,   390,     0,     0,
     290,   291,   391,   392,   393,   394,   395,   396,   397,   398,
     399,   400,   401,   402,   403,   404,   405,   406,   407,   408,
     409,   410,   411,   412,   413,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   290,   291,   156,   157,   158,   159,
     160,   161,   162,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    12,    13,    14,    15,     0,     0,     0,    16,
      17,    18,    19,    20,    21,    22,     0,   163,    24,     0,
       0,    26,    27,     0,     0,     0,     0,     0,   414,     0,
     415,   416,     0,   417,   418,   419,     0,   420,   421,   422,
     423,   424,   425,   426,   427,   428,   429,     0,     0,     0,
       0,     0,     0,     0,     0,    30,     0,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,   164,   165,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,   177,   156,   157,   158,   159,   160,   161,   162,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    12,    13,
      14,   234,     0,     0,     0,    16,    17,    18,    19,    20,
      21,    22,     0,   163,    24,     0,     0,    26,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    30,     0,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,   175,   176,   177,   156,   157,
     158,   159,   160,   161,   162,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    12,    13,    14,    15,     0,     0,
       0,    16,    17,    18,    19,    20,    21,    22,     0,   163,
       0,     0,     0,    26,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    30,     0,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
     164,   165,   166,   167,   168,   169,   170,   171,   172,   173,
     174,   175,   176,   177,   156,   157,   158,   159,   160,   161,
     162,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      12,    13,    14,   234,     0,     0,     0,    16,    17,    18,
      19,    20,    21,    22,     0,   163,     0,     0,     0,    26,
     156,   157,   158,   159,   160,   161,   162,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   617,     0,   618,   619,
       0,     0,     0,    16,    17,    18,    19,    20,    21,    22,
       0,   163,     0,    30,     0,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,   164,   165,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,   177,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    30,
       0,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,   164,   165,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177
};

static const yytype_int16 yycheck[] =
{
       4,     4,    27,     4,    97,    97,    28,    27,     4,   198,
     287,    15,     4,   285,    15,   242,   116,   387,   457,   687,
      29,     4,   121,   584,   299,    29,    29,   122,   668,   727,
     232,    27,   581,     4,   198,    27,   100,   101,   102,     0,
     110,   590,   112,   110,    72,   184,   306,   287,   629,   630,
     599,    72,   448,    10,    11,   110,    27,    26,   110,    10,
      11,   116,   121,     4,    23,    24,    25,    72,    37,     0,
     602,    20,    72,   121,   802,   121,   122,   121,   126,    91,
     121,   151,   116,   127,   151,    89,    27,    72,    89,    38,
     639,   116,   900,   370,    97,    72,   116,   905,   826,   127,
     121,    97,   111,   121,   113,    97,   127,   775,   112,   127,
     121,   322,    72,   116,    97,   126,   803,    72,   121,   116,
     116,   121,   584,   129,   129,   184,    97,   322,   198,   199,
     370,   198,    72,   773,   105,   931,    26,    12,    28,   110,
     602,   240,   331,   152,   129,   116,   285,    37,   956,   183,
     121,   155,   129,   949,   155,   124,    97,   198,   183,   740,
     125,   372,   127,   183,   123,    72,   178,   331,   180,   129,
     127,   134,   135,   182,   129,   179,   127,   372,   179,   381,
     151,   383,    20,    72,   239,   127,   183,   183,   287,   129,
     194,   195,   724,   194,   195,   199,   199,   127,    36,   240,
      38,   271,   462,   463,   271,   239,   127,   121,   127,   304,
      72,   306,   183,   127,   239,   249,   271,   613,   121,   271,
     287,   127,   129,   127,   127,   806,   285,   198,   127,   668,
     234,   228,   121,   234,   932,   232,   239,   240,   127,   121,
     129,   127,   239,   239,   248,   127,   287,   248,   935,   258,
     259,   832,   249,   940,   266,    72,   268,   532,   127,   121,
     455,   331,   724,   267,   331,   127,   267,   129,   239,   240,
     802,   370,   279,   127,   121,   121,   963,   131,   249,   288,
     127,   127,    72,   510,   126,   297,   954,   121,   130,   298,
     331,   518,   127,   127,   826,   307,   308,   127,   357,   121,
     271,   440,   529,   370,   121,   127,   127,   304,   127,   306,
     127,   121,   129,   127,    72,   322,   287,   127,   688,   323,
     121,   460,   122,   330,   885,   125,   127,   121,   322,   370,
      72,   121,   127,   127,   773,   127,   131,   127,   887,   129,
     802,   873,     3,     4,     5,     6,     7,     8,     9,   121,
     121,    72,   323,   375,   127,   127,   127,   889,   362,   330,
     331,   362,   127,   121,   826,   372,   127,   462,   463,   127,
     931,   129,   121,   122,   386,   127,   125,   907,   372,   121,
     127,   440,   912,   127,   381,   127,   383,   129,   949,     3,
       4,     5,     6,     7,     8,     9,   110,    26,    27,   370,
     121,   460,    26,    27,   375,   122,   127,   121,   129,   835,
     836,   873,   942,   943,   812,   945,   814,   127,   816,   881,
     882,   883,   884,   885,   886,   121,   121,   889,   958,   640,
     641,   642,   121,   122,   804,    72,   125,   446,     3,     4,
       5,     6,     7,     8,     9,   640,   641,   642,   121,   122,
     129,   455,   125,   122,   451,   121,   122,   122,    72,   125,
     130,   122,    12,   455,   124,   462,   463,   121,   740,   931,
     184,     3,     4,     5,     6,     7,     8,     9,    13,    14,
      15,    23,    24,    25,   455,   125,   200,   949,   122,   127,
     204,   205,   127,   207,   208,   209,   210,   211,   212,   213,
     214,   215,   216,   217,   218,   219,   220,   127,   127,   124,
     131,   122,   125,   121,   126,   131,   126,   126,   795,   126,
     126,   121,   531,    19,    20,    21,   240,   126,   126,   624,
      26,    27,    28,    29,    30,    31,    32,   126,    34,   126,
     126,   126,   126,   638,   126,   126,   121,   125,   128,   128,
     126,   128,   123,    36,   128,   121,    30,   271,   131,   128,
     132,   131,   121,   121,   126,   128,   126,   128,   123,   126,
     579,   285,   543,   287,   126,   126,   668,   124,   582,   124,
     128,   582,   121,   121,   861,    18,   132,   121,   622,   132,
     126,   588,   803,   121,   123,   128,   126,   128,   125,    71,
     126,   126,   543,   792,   128,   128,   126,   128,   803,   128,
     132,   128,   621,   617,   618,   619,   617,   618,   619,   333,
     334,   335,   128,   228,   338,   622,   128,   232,   128,   128,
     128,   128,   126,   640,   641,   642,   128,   128,   128,   121,
     124,   128,    13,   357,   132,   126,   640,   641,   642,    13,
     121,   622,   128,   128,   368,   128,   370,   121,   128,   128,
     366,   676,   672,   564,   668,   668,   670,   668,   672,   670,
     674,   672,   668,   674,   645,   668,   668,   904,   649,   650,
     857,   870,   653,   654,   655,   656,   657,   658,   659,   660,
     661,   662,   785,   785,   794,   668,   795,   668,   460,   304,
     459,   306,   638,    97,   645,   731,   668,   785,   649,   650,
     773,   682,   653,   654,   655,   656,   657,   658,   659,   660,
     661,   662,   792,   787,   935,   792,   440,   668,   939,   940,
     878,   808,   744,   916,   668,   357,   795,   721,   356,   794,
     935,   682,   794,   960,   939,   940,   460,   932,   606,   883,
     741,   792,   963,   668,   795,   296,   296,   815,    -1,    -1,
     794,    -1,   861,    -1,   857,   774,   718,    -1,   963,   794,
     774,   774,    -1,   795,   794,    -1,   381,   773,   383,    -1,
      -1,   773,   785,    -1,   793,   756,    -1,    -1,   759,   785,
      -1,   794,    -1,   785,   858,    -1,   803,   794,   794,    -1,
     870,    -1,   773,   870,   808,   860,    -1,   808,   860,   803,
     865,    -1,    -1,   865,   785,   756,   828,    -1,   759,    -1,
     861,   792,    -1,   794,   795,    -1,   860,    -1,    -1,   870,
      -1,   865,   773,    -1,    -1,    -1,    -1,   834,    -1,   861,
     865,    -1,   556,    -1,   785,    -1,   451,    -1,    -1,    -1,
     859,   855,    -1,    -1,   855,    -1,    -1,   462,   463,    -1,
      -1,   857,   865,   860,   860,   857,    -1,    -1,   865,   865,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   860,
     861,    -1,    -1,   890,   865,    -1,    -1,    -1,    -1,   870,
      -1,    -1,    -1,    -1,   908,    -1,    -1,   908,   912,   623,
      -1,   912,    -1,   910,   910,   629,   630,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    -1,    -1,   935,    -1,
      -1,    -1,   939,   940,    -1,    -1,    -1,    -1,    -1,   943,
      -1,   935,   943,    -1,    -1,   939,   940,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   668,    -1,   963,    -1,    -1,    -1,
      -1,    -1,   676,    -1,    -1,    -1,    -1,    -1,    -1,   963,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   584,
      -1,    -1,    -1,   588,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   602,    -1,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    -1,    -1,    -1,    -1,   740,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   127,    -1,    -1,    -1,    -1,    -1,   133,   134,   135,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   773,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   785,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     794,   795,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   806,    -1,    -1,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   832,   724,
      -1,    -1,    -1,    -1,    -1,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   133,   134,   135,   860,   861,    -1,    -1,
      -1,   865,    -1,    -1,    -1,    -1,    -1,    35,    -1,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    19,    20,    21,    22,    -1,
      -1,    -1,    26,    27,    28,    29,    30,    31,    32,    -1,
      34,    35,    36,    -1,    38,    -1,    -1,   802,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,   826,    -1,    -1,    -1,    -1,    -1,    -1,    72,   834,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,   133,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    -1,    -1,    -1,   873,    39,
      -1,    -1,    -1,    -1,    -1,    -1,   881,   882,   883,   884,
     885,   886,    -1,    -1,   889,   890,    -1,    -1,    -1,   133,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   931,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   949,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    -1,
      -1,    29,    30,    -1,    32,    -1,    34,    35,    36,    37,
      38,    39,    40,    -1,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   121,   122,   123,   124,   125,   126,   127,
      -1,   129,   130,   131,    -1,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      -1,    -1,    29,    30,    -1,    32,    -1,    34,    35,    36,
      37,    38,    39,    40,    -1,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   121,   122,   123,   124,   125,   126,
     127,    -1,   129,   130,   131,    -1,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    -1,    -1,    29,    30,    -1,    32,    -1,    34,    35,
      36,    37,    38,    39,    40,    -1,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   121,   122,   123,   124,   125,
     126,   127,    -1,   129,   130,   131,    -1,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    -1,    -1,    29,    30,    -1,    32,    -1,    34,
      35,    36,    37,    38,    39,    40,    -1,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   121,   122,    -1,   124,
     125,   126,   127,    -1,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    -1,    -1,    29,    30,    -1,    32,    -1,
      34,    35,    36,    37,    38,    39,    40,    -1,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,    -1,
     124,   125,   126,   127,   128,   129,   130,   131,    -1,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    29,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    39,    40,    -1,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,
     123,   124,   125,   126,   127,    -1,   129,   130,   131,    -1,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    -1,    -1,    29,    30,    -1,
      32,    -1,    34,    35,    36,    37,    38,    39,    40,    -1,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,
     122,    -1,   124,   125,   126,   127,    -1,   129,   130,   131,
      -1,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    -1,    -1,    29,    30,
      -1,    32,    -1,    34,    35,    36,    37,    38,    39,    40,
      -1,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   122,    -1,   124,   125,   126,   127,   128,   129,   130,
     131,    -1,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    -1,    -1,    29,
      30,    -1,    32,    -1,    34,    35,    36,    37,    38,    39,
      40,    -1,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   121,   122,    -1,   124,   125,   126,   127,    -1,   129,
     130,   131,    -1,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    -1,    -1,
      29,    30,    -1,    32,    -1,    34,    35,    36,    37,    38,
      39,    40,    -1,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   121,   122,    -1,   124,   125,   126,   127,    -1,
     129,   130,   131,    -1,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    -1,
      -1,    29,    30,    -1,    32,    -1,    34,    35,    36,    37,
      38,    39,    40,    -1,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    -1,    72,    -1,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   121,   122,   123,   124,   125,   126,   127,
      -1,   129,   130,   131,    -1,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      -1,    -1,    29,    30,    -1,    32,    -1,    34,    35,    36,
      37,    38,    39,    40,    -1,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    -1,    72,    -1,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   122,    -1,   124,   125,   126,
     127,   128,   129,   130,   131,    -1,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    -1,    -1,    29,    30,    -1,    32,    -1,    34,    35,
      36,    37,    38,    39,    40,    -1,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    -1,    72,    -1,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   122,    -1,   124,   125,
     126,   127,   128,   129,   130,   131,    -1,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    -1,    -1,    29,    30,    -1,    32,    -1,    34,
      35,    36,    37,    38,    39,    40,    -1,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    -1,    72,    -1,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   122,    -1,   124,
     125,   126,   127,   128,   129,   130,   131,    -1,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    -1,    -1,    29,    30,    -1,    32,    -1,
      34,    35,    36,    37,    38,    39,    40,    -1,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    -1,    72,    -1,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   122,    -1,
     124,   125,   126,   127,    -1,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    29,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    39,    40,    -1,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    -1,    72,
      -1,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   122,
      -1,   124,   125,   126,   127,    -1,   129,   130,   131,    -1,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    -1,    -1,    29,    30,    -1,
      32,    -1,    34,    35,    36,    37,    38,    39,    40,    -1,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    -1,
      72,    -1,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     122,    -1,   124,   125,   126,   127,    -1,   129,   130,   131,
      -1,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    -1,    -1,    29,    30,
      -1,    32,    -1,    34,    35,    36,    37,    38,    39,    40,
      -1,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      -1,    72,    -1,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   122,    -1,    -1,   125,    -1,   127,    -1,   129,   130,
     131,    -1,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    -1,    -1,    29,
      30,    -1,    32,    -1,    34,    35,    36,    37,    38,    39,
      40,    -1,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    -1,    72,    -1,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   122,    -1,    -1,   125,    -1,   127,    -1,   129,
      -1,   131,    -1,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      19,    20,    21,    22,    -1,    -1,    -1,    26,    27,    28,
      29,    30,    31,    32,    -1,    34,    -1,    -1,    -1,    38,
      -1,    -1,    -1,    42,    43,    -1,    -1,    -1,    -1,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    -1,    72,    -1,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   124,    -1,   126,   127,    -1,
     129,   130,   131,    -1,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      19,    20,    21,    22,    -1,    -1,    -1,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    -1,    38,
      39,    -1,    41,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    72,    -1,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,     3,     4,     5,     6,     7,     8,     9,
      -1,    -1,    -1,    -1,   133,    -1,    -1,    -1,    -1,    19,
      20,    21,    22,    -1,    -1,    -1,    26,    27,    28,    29,
      30,    31,    32,    -1,    34,    35,    36,    -1,    38,    39,
      40,    41,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    72,    -1,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
      -1,   121,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    -1,    -1,   133,    -1,    -1,    -1,    -1,    19,    20,
      21,    22,    -1,    -1,    -1,    26,    27,    28,    29,    30,
      31,    32,    -1,    34,    35,    -1,    -1,    38,    39,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    72,    -1,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,     3,     4,
       5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    19,    20,    21,    22,    -1,    -1,
      -1,    26,    27,    28,    29,    30,    31,    32,    -1,    34,
      35,    -1,   133,    38,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       3,     4,     5,     6,     7,     8,     9,    72,    -1,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    23,    24,    25,    -1,    -1,    -1,
      -1,    30,    -1,    -1,    -1,    -1,    -1,    -1,   133,    72,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,     3,     4,     5,     6,
       7,     8,     9,    72,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
     133,    38,    -1,    40,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   133,    72,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    -1,    -1,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   133,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    35,    36,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    72,    -1,    -1,    -1,    -1,    -1,   127,
      -1,    -1,    -1,    -1,    -1,   133,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      -1,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,     3,     4,     5,     6,     7,
       8,     9,    -1,   122,    -1,    -1,   125,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   133,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,   133,
      38,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   133,    72,     3,     4,     5,     6,     7,
       8,     9,    -1,    -1,    -1,    -1,    35,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   133,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    -1,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    -1,    -1,    -1,   133,    39,    -1,    -1,    -1,    -1,
      -1,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,   133,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    -1,    -1,    -1,    -1,    39,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   133,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    -1,    -1,   121,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    42,    43,    -1,    -1,
     134,   135,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   134,   135,     3,     4,     5,     6,
       7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    19,    20,    21,    22,    -1,    -1,    -1,    26,
      27,    28,    29,    30,    31,    32,    -1,    34,    35,    -1,
      -1,    38,    39,    -1,    -1,    -1,    -1,    -1,   124,    -1,
     126,   127,    -1,   129,   130,   131,    -1,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    72,    -1,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    20,
      21,    22,    -1,    -1,    -1,    26,    27,    28,    29,    30,
      31,    32,    -1,    34,    35,    -1,    -1,    38,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    72,    -1,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,     3,     4,
       5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    19,    20,    21,    22,    -1,    -1,
      -1,    26,    27,    28,    29,    30,    31,    32,    -1,    34,
      -1,    -1,    -1,    38,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    72,    -1,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      19,    20,    21,    22,    -1,    -1,    -1,    26,    27,    28,
      29,    30,    31,    32,    -1,    34,    -1,    -1,    -1,    38,
       3,     4,     5,     6,     7,     8,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    19,    -1,    21,    22,
      -1,    -1,    -1,    26,    27,    28,    29,    30,    31,    32,
      -1,    34,    -1,    72,    -1,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    72,
      -1,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   145,   146,     0,   147,     3,     4,     5,     6,     7,
       8,     9,    19,    20,    21,    22,    26,    27,    28,    29,
      30,    31,    32,    34,    35,    36,    38,    39,    40,    41,
      72,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   121,   133,   148,
     149,   150,   151,   153,   154,   155,   156,   157,   159,   162,
     176,   177,   179,   187,   188,   197,   199,   200,   216,   217,
     218,   219,   222,   223,   226,   232,   259,   289,   290,   291,
     292,   294,   295,   296,   298,   299,   302,   303,   304,   305,
     306,   308,   309,   312,   319,   320,   321,   327,     3,     4,
       5,     6,     7,     8,     9,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,   289,
     291,   292,   296,    12,   129,   313,     3,     4,     5,     6,
       7,     8,     9,    34,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,   157,   162,
     177,   291,   296,   302,   308,     3,     4,     5,     6,     7,
       8,     9,   122,   299,    38,    40,   198,   289,   292,   295,
     127,   127,   127,   127,   127,   127,   127,   127,   127,   127,
     127,   127,   127,   127,   127,   127,   127,   127,   127,   127,
     127,     3,     4,     5,     6,     7,     8,     9,   300,   158,
     161,   289,   300,   122,    22,   155,   156,   184,   216,   302,
     308,   121,   122,   237,   237,   237,   237,   127,   162,   302,
     308,   121,    36,   223,   240,   241,   244,   290,   294,   296,
     298,   295,   290,   291,   295,   129,   157,   162,   177,   188,
     223,   292,   303,   312,    10,    11,   227,   229,   231,   232,
     233,   238,   240,   260,   268,   271,   279,   292,   296,   298,
     134,   135,   265,   322,   323,   324,   326,   310,   296,   122,
     130,   201,   289,    12,   300,   289,   300,   157,   177,     3,
       4,     5,     6,     7,     8,     9,   192,   193,   196,   268,
     279,   298,   375,   124,   152,   289,   289,   121,   226,   231,
     232,   292,   298,   329,   332,   333,   298,   298,   337,   298,
     298,   298,   298,   298,   298,   298,   298,   298,   298,   298,
     298,   298,   298,    33,    39,   261,   264,   265,   301,   303,
     321,   122,   125,   169,   170,   121,   160,   261,   178,   289,
     292,   298,   375,   224,   289,   293,   127,   242,   295,   295,
     297,   300,   289,   300,   223,   241,   311,   228,   127,    42,
      43,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,   124,   126,   127,   129,   130,   131,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     351,   352,   234,   127,   262,   124,   256,   257,   268,   277,
     322,   281,   282,   283,   227,   233,   296,   125,   325,   323,
     324,   300,   146,   202,   204,   205,   207,   209,   189,   191,
     265,   189,   300,   300,   190,   127,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      29,    30,    32,    34,    35,    36,    37,    38,    39,    40,
      42,    43,    44,    45,    46,    47,    71,    72,    73,   121,
     122,   123,   124,   125,   126,   127,   129,   130,   131,   143,
     319,   352,   353,   354,   377,   378,   379,   380,   381,   382,
     291,   296,   122,   121,   226,   231,   126,   298,   298,   298,
     126,   126,   298,   126,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   128,   128,   126,   121,   262,   260,
     163,    23,    24,    25,    30,   171,   172,   175,   289,   169,
     121,   180,   181,   182,   298,   298,   123,   249,   250,   296,
     299,   245,   125,   246,   361,   261,   121,   261,   300,   234,
     230,   131,   131,   128,   132,    26,    37,   124,   235,   239,
     121,   126,   258,   128,   268,   131,   284,   285,    13,    14,
      15,   280,   306,   307,   123,   126,   130,    19,    21,    22,
     291,   296,   302,   314,   315,   318,   319,    20,    38,   211,
     200,   121,   190,   196,   121,   189,   189,   121,   126,   194,
     375,   375,   375,   121,   146,   328,   126,   128,   128,   334,
     336,   126,   308,   340,   342,   344,   346,   341,   343,   345,
     347,   348,   349,   298,    23,    24,    25,   123,   164,   165,
     174,   175,   126,    30,   173,   123,   126,   124,   124,   128,
     251,   253,   254,   295,   249,   247,   248,   289,   243,    10,
      11,    72,   122,   124,   125,   126,   127,   129,   130,   131,
     143,   320,   321,   352,   353,   354,   356,   363,   364,   365,
     367,   369,   373,   121,   121,   249,   132,   132,   236,    18,
     249,   263,   129,   130,   355,   356,   358,   269,   286,   285,
     306,   203,   289,   289,   289,   312,    10,    11,   127,   266,
     270,   278,   279,   265,   316,   266,   266,   121,   121,   191,
     249,   123,   132,   128,   123,   308,   330,   308,   308,   338,
     128,   308,   308,   308,   308,   308,   308,   308,   308,   308,
     308,   126,   350,    33,    41,   120,   121,   154,   156,   166,
     167,   168,   176,   187,   197,   200,   220,   221,   222,   241,
     259,   289,   292,   296,   302,   308,   327,   125,   289,   172,
     289,   182,   183,   376,   225,   126,   308,   128,   126,   381,
     234,   371,   372,   374,   370,   366,   368,   128,   369,   128,
     264,   358,   127,   272,   282,   287,   288,   204,   317,   206,
     266,   276,   322,   281,   300,   208,   210,   128,   128,   308,
     335,   128,   308,   128,   128,   128,   128,   128,   128,   128,
     128,   126,   126,   128,   154,   162,   185,   200,   221,   296,
     302,   308,   381,   156,   220,   302,   237,   121,   231,   241,
     292,   233,   296,   355,   121,   378,   234,    71,   252,   266,
     248,   357,   357,   362,   357,   361,   357,   273,   132,   355,
     300,   124,   212,   213,   128,   266,   212,   212,   195,   331,
     126,   339,    13,    13,   122,   125,   186,   289,   162,   185,
     302,   237,   162,   121,   231,   253,   255,   124,   126,   128,
     358,   359,   128,   121,   123,   359,   360,   128,   130,   132,
     249,   214,   267,   275,   128,   376,   128,   128,   128,   375,
     376,   186,   289,   162,   186,   289,   121,   256,   128,   215,
     363,   272,    26,    28,    37,   128,   123,   121,   289,   186,
     274,   363,   381,   376,   275,   121
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
        case 4:

/* Line 1455 of yacc.c  */
#line 1469 "vtkParse.y"
    {
      startSig();
      clearType();
      clearTypeId();
      clearTemplate();
      closeComment();
    }
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 1513 "vtkParse.y"
    { pushNamespace((yyvsp[(2) - (2)].str)); }
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 1514 "vtkParse.y"
    { popNamespace(); }
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 1536 "vtkParse.y"
    { pushType(); }
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 1537 "vtkParse.y"
    {
      const char *name = (currentClass ? currentClass->Name : NULL);
      popType();
      clearTypeId();
      if (name)
        {
        setTypeId(name);
        setTypeBase(guess_id_type(name));
        }
      end_class();
    }
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 1550 "vtkParse.y"
    { start_class((yyvsp[(2) - (2)].str), (yyvsp[(1) - (2)].integer)); }
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 1552 "vtkParse.y"
    { start_class(NULL, (yyvsp[(1) - (1)].integer)); }
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 1556 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 1557 "vtkParse.y"
    { (yyval.integer) = 1; }
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 1558 "vtkParse.y"
    { (yyval.integer) = 2; }
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 1562 "vtkParse.y"
    {
      startSig();
      clearType();
      clearTypeId();
      clearTemplate();
      closeComment();
    }
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 1574 "vtkParse.y"
    { access_level = VTK_ACCESS_PUBLIC; }
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 1575 "vtkParse.y"
    { access_level = VTK_ACCESS_PRIVATE; }
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 1576 "vtkParse.y"
    { access_level = VTK_ACCESS_PROTECTED; }
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 1601 "vtkParse.y"
    { output_friend_function(); }
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 1615 "vtkParse.y"
    { add_base_class(currentClass, (yyvsp[(1) - (1)].str), access_level, 0); }
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 1617 "vtkParse.y"
    { add_base_class(currentClass, (yyvsp[(3) - (3)].str), (yyvsp[(2) - (3)].integer), 1); }
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 1619 "vtkParse.y"
    { add_base_class(currentClass, (yyvsp[(3) - (3)].str), (yyvsp[(1) - (3)].integer), (yyvsp[(2) - (3)].integer)); }
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 1622 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 1623 "vtkParse.y"
    { (yyval.integer) = 1; }
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 1626 "vtkParse.y"
    { (yyval.integer) = access_level; }
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 1630 "vtkParse.y"
    { (yyval.integer) = VTK_ACCESS_PUBLIC; }
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 1631 "vtkParse.y"
    { (yyval.integer) = VTK_ACCESS_PRIVATE; }
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 1632 "vtkParse.y"
    { (yyval.integer) = VTK_ACCESS_PROTECTED; }
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 1648 "vtkParse.y"
    { pushType(); start_enum((yyvsp[(1) - (2)].str)); }
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 1649 "vtkParse.y"
    {
      popType();
      clearTypeId();
      if ((yyvsp[(1) - (5)].str) != NULL)
        {
        setTypeId((yyvsp[(1) - (5)].str));
        setTypeBase(guess_id_type((yyvsp[(1) - (5)].str)));
        }
      end_enum();
    }
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 1661 "vtkParse.y"
    { (yyval.str) = (yyvsp[(2) - (2)].str); }
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 1662 "vtkParse.y"
    { (yyval.str) = NULL; }
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 1673 "vtkParse.y"
    { add_enum((yyvsp[(1) - (1)].str), NULL); }
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 1674 "vtkParse.y"
    { postSig("="); markSig(); }
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 1675 "vtkParse.y"
    { chopSig(); add_enum((yyvsp[(1) - (4)].str), copySig()); }
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 1728 "vtkParse.y"
    { pushFunction(); postSig("("); }
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 1729 "vtkParse.y"
    { postSig(")"); }
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 1730 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_FUNCTION; popFunction(); }
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 1734 "vtkParse.y"
    {
      ValueInfo *item = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(item);
      item->ItemType = VTK_TYPEDEF_INFO;
      item->Access = access_level;

      handle_complex_type(item, getType(), (yyvsp[(1) - (1)].integer), getSig());

      if (getVarName())
        {
        item->Name = getVarName();
        }

      if (item->Class == NULL)
        {
        vtkParse_FreeValue(item);
        }
      else if (currentClass)
        {
        vtkParse_AddTypedefToClass(currentClass, item);
        }
      else
        {
        vtkParse_AddTypedefToNamespace(currentNamespace, item);
        }
    }
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 1767 "vtkParse.y"
    { add_using((yyvsp[(2) - (3)].str), 0); }
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 1771 "vtkParse.y"
    { (yyval.str) = (yyvsp[(2) - (2)].str); }
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1773 "vtkParse.y"
    { (yyval.str) = vtkstrcat((yyvsp[(1) - (2)].str), (yyvsp[(2) - (2)].str)); }
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1775 "vtkParse.y"
    { (yyval.str) = vtkstrcat((yyvsp[(1) - (2)].str), (yyvsp[(2) - (2)].str)); }
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1777 "vtkParse.y"
    { (yyval.str) = vtkstrcat3((yyvsp[(1) - (3)].str), (yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].str)); }
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1779 "vtkParse.y"
    { (yyval.str) = vtkstrcat3((yyvsp[(1) - (3)].str), (yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].str)); }
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1782 "vtkParse.y"
    { add_using((yyvsp[(3) - (4)].str), 1); }
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1790 "vtkParse.y"
    { postSig("template<> "); clearTypeId(); }
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1792 "vtkParse.y"
    {
      postSig("template<");
      pushType();
      clearType();
      clearTypeId();
      startTemplate();
    }
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1800 "vtkParse.y"
    {
      chopSig();
      if (getSig()[getSigLength()-1] == '>') { postSig(" "); }
      postSig("> ");
      clearTypeId();
      popType();
    }
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1811 "vtkParse.y"
    { chopSig(); postSig(", "); clearType(); clearTypeId(); }
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1815 "vtkParse.y"
    { markSig(); }
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1817 "vtkParse.y"
    { add_template_parameter(getType(), (yyvsp[(3) - (3)].integer), copySig()); }
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1819 "vtkParse.y"
    { markSig(); }
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 1821 "vtkParse.y"
    { add_template_parameter(0, (yyvsp[(3) - (3)].integer), copySig()); }
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 1823 "vtkParse.y"
    { pushTemplate(); markSig(); }
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 1825 "vtkParse.y"
    {
      int i;
      TemplateInfo *newTemplate = currentTemplate;
      popTemplate();
      add_template_parameter(0, (yyvsp[(3) - (3)].integer), copySig());
      i = currentTemplate->NumberOfParameters-1;
      currentTemplate->Parameters[i]->Template = newTemplate;
    }
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 1836 "vtkParse.y"
    { postSig("class "); }
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 1837 "vtkParse.y"
    { postSig("typename "); }
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 1843 "vtkParse.y"
    { postSig("="); markSig(); }
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 1845 "vtkParse.y"
    {
      int i = currentTemplate->NumberOfParameters-1;
      ValueInfo *param = currentTemplate->Parameters[i];
      chopSig();
      param->Value = copySig();
    }
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 1862 "vtkParse.y"
    { output_function(); }
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1863 "vtkParse.y"
    { output_function(); }
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1864 "vtkParse.y"
    { reject_function(); }
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1865 "vtkParse.y"
    { reject_function(); }
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 1881 "vtkParse.y"
    { output_function(); }
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 1898 "vtkParse.y"
    {
      postSig("(");
      set_return(currentFunction, getType(), getTypeId(), 0);
    }
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 1902 "vtkParse.y"
    { postSig(")"); }
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 1904 "vtkParse.y"
    {
      postSig(";");
      closeSig();
      currentFunction->IsOperator = 1;
      currentFunction->Name = "operator typecast";
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed operator", "operator typecast");
    }
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1915 "vtkParse.y"
    { (yyval.str) = copySig(); }
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1918 "vtkParse.y"
    { postSig(")"); }
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 1919 "vtkParse.y"
    {
      postSig(";");
      closeSig();
      currentFunction->Name = (yyvsp[(1) - (3)].str);
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed operator", currentFunction->Name);
    }
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 1929 "vtkParse.y"
    {
      postSig("(");
      currentFunction->IsOperator = 1;
      set_return(currentFunction, getType(), getTypeId(), 0);
    }
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 1938 "vtkParse.y"
    { chopSig(); (yyval.str) = vtkstrcat(copySig(), (yyvsp[(2) - (2)].str)); postSig((yyvsp[(2) - (2)].str)); }
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 1941 "vtkParse.y"
    { markSig(); postSig("operator "); }
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 1945 "vtkParse.y"
    {
      postSig(";");
      closeSig();
      currentFunction->Name = (yyvsp[(1) - (2)].str);
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 1957 "vtkParse.y"
    { postSig(" throw "); }
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 1957 "vtkParse.y"
    { chopSig(); }
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 1958 "vtkParse.y"
    { postSig(" const"); currentFunction->IsConst = 1; }
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 1960 "vtkParse.y"
    {
      postSig(" = 0");
      currentFunction->IsPureVirtual = 1;
      if (currentClass) { currentClass->IsAbstract = 1; }
    }
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 1972 "vtkParse.y"
    {
      postSig("(");
      set_return(currentFunction, getType(), getTypeId(), 0);
    }
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 1976 "vtkParse.y"
    { postSig(")"); }
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 1988 "vtkParse.y"
    { closeSig(); }
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 1989 "vtkParse.y"
    { openSig(); }
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 1990 "vtkParse.y"
    {
      postSig(";");
      closeSig();
      if (getType() & VTK_PARSE_VIRTUAL)
        {
        currentFunction->IsVirtual = 1;
        }
      if (getType() & VTK_PARSE_EXPLICIT)
        {
        currentFunction->IsExplicit = 1;
        }
      currentFunction->Name = (yyvsp[(1) - (5)].str);
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 2007 "vtkParse.y"
    { pushType(); postSig("("); }
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 2008 "vtkParse.y"
    { popType(); postSig(")"); }
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 2025 "vtkParse.y"
    { clearType(); clearTypeId(); }
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 2028 "vtkParse.y"
    { clearType(); clearTypeId(); }
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 2029 "vtkParse.y"
    { clearType(); clearTypeId(); postSig(", "); }
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 2032 "vtkParse.y"
    { currentFunction->IsVariadic = 1; postSig(", ..."); }
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 2035 "vtkParse.y"
    { markSig(); }
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 2037 "vtkParse.y"
    {
      ValueInfo *param = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(param);

      handle_complex_type(param, getType(), (yyvsp[(3) - (3)].integer), copySig());
      add_legacy_parameter(currentFunction, param);

      if (getVarName())
        {
        param->Name = getVarName();
        }

      vtkParse_AddParameterToFunction(currentFunction, param);
    }
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 2052 "vtkParse.y"
    {
      int i = currentFunction->NumberOfParameters-1;
      if (getVarValue())
        {
        currentFunction->Parameters[i]->Value = getVarValue();
        }
    }
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 2061 "vtkParse.y"
    { clearVarValue(); }
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 2065 "vtkParse.y"
    { postSig("="); clearVarValue(); markSig(); }
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 2066 "vtkParse.y"
    { chopSig(); setVarValue(copySig()); }
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 2077 "vtkParse.y"
    {
      unsigned int type = getType();
      ValueInfo *var = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(var);
      var->ItemType = VTK_VARIABLE_INFO;
      var->Access = access_level;

      handle_complex_type(var, type, (yyvsp[(1) - (2)].integer), getSig());

      var->Name = getVarName();

      if (getVarValue())
        {
        var->Value = getVarValue();
        }

      /* Is this a typedef? */
      if ((type & VTK_PARSE_TYPEDEF) != 0)
        {
        var->ItemType = VTK_TYPEDEF_INFO;
        if (currentClass)
          {
          vtkParse_AddVariableToClass(currentClass, var);
          }
        else
          {
          vtkParse_AddVariableToNamespace(currentNamespace, var);
          }
        }
      /* Is this a constant? */
      else if (((type & VTK_PARSE_CONST) != 0) && var->Value != NULL &&
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

  case 227:

/* Line 1455 of yacc.c  */
#line 2139 "vtkParse.y"
    { postSig(", "); }
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 2145 "vtkParse.y"
    { setTypePtr(0); }
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 2146 "vtkParse.y"
    { setTypePtr((yyvsp[(1) - (1)].integer)); }
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 2150 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 2151 "vtkParse.y"
    { postSig(")"); }
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 2153 "vtkParse.y"
    {
      const char *scope = getScope();
      unsigned int parens = add_indirection((yyvsp[(1) - (5)].integer), (yyvsp[(2) - (5)].integer));
      if ((yyvsp[(5) - (5)].integer) == VTK_PARSE_FUNCTION)
        {
        if (scope) { scope = vtkstrndup(scope, strlen(scope) - 2); }
        getFunction()->Class = scope;
        (yyval.integer) = (parens | VTK_PARSE_FUNCTION);
        }
      else if ((yyvsp[(5) - (5)].integer) == VTK_PARSE_ARRAY)
        {
        (yyval.integer) = add_indirection_to_array(parens);
        }
    }
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 2170 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 2171 "vtkParse.y"
    { postSig(")"); }
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 2173 "vtkParse.y"
    {
      const char *scope = getScope();
      unsigned int parens = add_indirection((yyvsp[(1) - (5)].integer), (yyvsp[(2) - (5)].integer));
      if ((yyvsp[(5) - (5)].integer) == VTK_PARSE_FUNCTION)
        {
        if (scope) { scope = vtkstrndup(scope, strlen(scope) - 2); }
        getFunction()->Class = scope;
        (yyval.integer) = (parens | VTK_PARSE_FUNCTION);
        }
      else if ((yyvsp[(5) - (5)].integer) == VTK_PARSE_ARRAY)
        {
        (yyval.integer) = add_indirection_to_array(parens);
        }
    }
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 2189 "vtkParse.y"
    { postSig("("); scopeSig(""); (yyval.integer) = 0; }
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 2190 "vtkParse.y"
    { postSig("("); scopeSig((yyvsp[(1) - (1)].str)); postSig("*");
         (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 2192 "vtkParse.y"
    { postSig("("); scopeSig((yyvsp[(1) - (1)].str)); postSig("&");
         (yyval.integer) = VTK_PARSE_REF; }
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 2196 "vtkParse.y"
    { postSig("("); scopeSig((yyvsp[(1) - (1)].str)); postSig("*");
         (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 2198 "vtkParse.y"
    { postSig("("); scopeSig((yyvsp[(1) - (1)].str)); postSig("&");
         (yyval.integer) = VTK_PARSE_REF; }
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 2201 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 2202 "vtkParse.y"
    { pushFunction(); postSig("("); }
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 2203 "vtkParse.y"
    { postSig(")"); }
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 2204 "vtkParse.y"
    {
      (yyval.integer) = VTK_PARSE_FUNCTION;
      popFunction();
    }
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 2208 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_ARRAY; }
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 2212 "vtkParse.y"
    { currentFunction->IsConst = 1; }
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 2218 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(1) - (2)].integer), (yyvsp[(2) - (2)].integer)); }
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 2223 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(1) - (2)].integer), (yyvsp[(2) - (2)].integer)); }
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 2226 "vtkParse.y"
    { clearVarName(); chopSig(); }
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 2230 "vtkParse.y"
    { setVarName((yyvsp[(1) - (1)].str)); }
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 2231 "vtkParse.y"
    { setVarName((yyvsp[(1) - (3)].str)); }
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 2239 "vtkParse.y"
    { clearArray(); }
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 2243 "vtkParse.y"
    { clearArray(); }
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 2250 "vtkParse.y"
    { postSig("["); }
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 2250 "vtkParse.y"
    { postSig("]"); }
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 2253 "vtkParse.y"
    { pushArraySize(""); }
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 2254 "vtkParse.y"
    { markSig(); }
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 2254 "vtkParse.y"
    { chopSig(); pushArraySize(copySig()); }
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 2270 "vtkParse.y"
    { (yyval.str) = vtkstrcat((yyvsp[(1) - (2)].str), (yyvsp[(2) - (2)].str)); }
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 2272 "vtkParse.y"
    { (yyval.str) = vtkstrcat((yyvsp[(1) - (2)].str), (yyvsp[(2) - (2)].str)); }
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 2274 "vtkParse.y"
    { (yyval.str) = vtkstrcat((yyvsp[(1) - (2)].str), (yyvsp[(2) - (2)].str)); }
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 2278 "vtkParse.y"
    { (yyval.str) = vtkstrcat((yyvsp[(1) - (2)].str), (yyvsp[(2) - (2)].str)); }
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 2280 "vtkParse.y"
    { (yyval.str) = vtkstrcat((yyvsp[(1) - (2)].str), (yyvsp[(2) - (2)].str)); }
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 2282 "vtkParse.y"
    { (yyval.str) = vtkstrcat3((yyvsp[(1) - (3)].str), (yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].str)); }
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 2284 "vtkParse.y"
    { (yyval.str) = vtkstrcat3((yyvsp[(1) - (3)].str), (yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].str)); }
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 2285 "vtkParse.y"
    { postSig("template "); }
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 2287 "vtkParse.y"
    { (yyval.str) = vtkstrcat4((yyvsp[(1) - (5)].str), "template ", (yyvsp[(4) - (5)].str), (yyvsp[(5) - (5)].str)); }
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 2290 "vtkParse.y"
    { postSig((yyvsp[(1) - (1)].str)); }
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 2293 "vtkParse.y"
    { (yyval.str) = "::"; postSig((yyval.str)); }
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 2296 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<"); }
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 2298 "vtkParse.y"
    {
      chopSig(); if (getSig()[getSigLength()-1] == '>') { postSig(" "); }
      postSig(">"); (yyval.str) = copySig(); clearTypeId();
    }
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 2311 "vtkParse.y"
    { postSig((yyvsp[(1) - (1)].str)); }
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 2312 "vtkParse.y"
    { postSig((yyvsp[(1) - (1)].str)); }
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 2313 "vtkParse.y"
    { postSig((yyvsp[(1) - (1)].str)); }
    break;

  case 294:

/* Line 1455 of yacc.c  */
#line 2314 "vtkParse.y"
    { postSig((yyvsp[(1) - (1)].str)); }
    break;

  case 295:

/* Line 1455 of yacc.c  */
#line 2315 "vtkParse.y"
    { postSig((yyvsp[(1) - (1)].str)); }
    break;

  case 296:

/* Line 1455 of yacc.c  */
#line 2316 "vtkParse.y"
    { postSig((yyvsp[(1) - (1)].str)); }
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 2317 "vtkParse.y"
    { postSig((yyvsp[(1) - (1)].str)); }
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 2318 "vtkParse.y"
    { (yyval.str) = vtkstrcat("~",(yyvsp[(2) - (2)].str)); postSig((yyval.str)); }
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 2319 "vtkParse.y"
    { (yyval.str) = vtkstrcat("~",(yyvsp[(2) - (2)].str)); postSig((yyval.str)); }
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 2320 "vtkParse.y"
    { (yyval.str) = vtkstrcat("~",(yyvsp[(2) - (2)].str)); postSig((yyval.str)); }
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 2321 "vtkParse.y"
    { (yyval.str) = vtkstrcat("~",(yyvsp[(2) - (2)].str)); postSig((yyval.str)); }
    break;

  case 302:

/* Line 1455 of yacc.c  */
#line 2322 "vtkParse.y"
    { (yyval.str) = vtkstrcat("~",(yyvsp[(2) - (2)].str)); postSig((yyval.str)); }
    break;

  case 303:

/* Line 1455 of yacc.c  */
#line 2323 "vtkParse.y"
    { (yyval.str) = vtkstrcat("~",(yyvsp[(2) - (2)].str)); postSig((yyval.str)); }
    break;

  case 304:

/* Line 1455 of yacc.c  */
#line 2324 "vtkParse.y"
    { (yyval.str) = vtkstrcat("~",(yyvsp[(2) - (2)].str)); postSig((yyval.str)); }
    break;

  case 305:

/* Line 1455 of yacc.c  */
#line 2325 "vtkParse.y"
    { (yyval.str) = "size_t"; postSig((yyval.str)); }
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 2326 "vtkParse.y"
    { (yyval.str) = "ssize_t"; postSig((yyval.str)); }
    break;

  case 307:

/* Line 1455 of yacc.c  */
#line 2327 "vtkParse.y"
    { (yyval.str) = "vtkTypeInt8"; postSig((yyval.str)); }
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 2328 "vtkParse.y"
    { (yyval.str) = "vtkTypeUInt8"; postSig((yyval.str)); }
    break;

  case 309:

/* Line 1455 of yacc.c  */
#line 2329 "vtkParse.y"
    { (yyval.str) = "vtkTypeInt16"; postSig((yyval.str)); }
    break;

  case 310:

/* Line 1455 of yacc.c  */
#line 2330 "vtkParse.y"
    { (yyval.str) = "vtkTypeUInt16"; postSig((yyval.str)); }
    break;

  case 311:

/* Line 1455 of yacc.c  */
#line 2331 "vtkParse.y"
    { (yyval.str) = "vtkTypeInt32"; postSig((yyval.str)); }
    break;

  case 312:

/* Line 1455 of yacc.c  */
#line 2332 "vtkParse.y"
    { (yyval.str) = "vtkTypeUInt32"; postSig((yyval.str)); }
    break;

  case 313:

/* Line 1455 of yacc.c  */
#line 2333 "vtkParse.y"
    { (yyval.str) = "vtkTypeInt64"; postSig((yyval.str)); }
    break;

  case 314:

/* Line 1455 of yacc.c  */
#line 2334 "vtkParse.y"
    { (yyval.str) = "vtkTypeUInt64"; postSig((yyval.str)); }
    break;

  case 315:

/* Line 1455 of yacc.c  */
#line 2335 "vtkParse.y"
    { (yyval.str) = "vtkTypeFloat32"; postSig((yyval.str)); }
    break;

  case 316:

/* Line 1455 of yacc.c  */
#line 2336 "vtkParse.y"
    { (yyval.str) = "vtkTypeFloat64"; postSig((yyval.str)); }
    break;

  case 317:

/* Line 1455 of yacc.c  */
#line 2337 "vtkParse.y"
    { (yyval.str) = "vtkIdType"; postSig((yyval.str)); }
    break;

  case 318:

/* Line 1455 of yacc.c  */
#line 2338 "vtkParse.y"
    { (yyval.str) = "vtkFloatingPointType"; postSig((yyval.str)); }
    break;

  case 329:

/* Line 1455 of yacc.c  */
#line 2364 "vtkParse.y"
    { setTypeBase(buildTypeBase(getType(), (yyvsp[(1) - (1)].integer))); }
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 2365 "vtkParse.y"
    { setTypeMod(VTK_PARSE_TYPEDEF); }
    break;

  case 331:

/* Line 1455 of yacc.c  */
#line 2366 "vtkParse.y"
    { setTypeMod(VTK_PARSE_FRIEND); }
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 2373 "vtkParse.y"
    { setTypeMod((yyvsp[(1) - (1)].integer)); }
    break;

  case 335:

/* Line 1455 of yacc.c  */
#line 2374 "vtkParse.y"
    { setTypeMod((yyvsp[(1) - (1)].integer)); }
    break;

  case 336:

/* Line 1455 of yacc.c  */
#line 2375 "vtkParse.y"
    { setTypeMod((yyvsp[(1) - (1)].integer)); }
    break;

  case 337:

/* Line 1455 of yacc.c  */
#line 2378 "vtkParse.y"
    { postSig("mutable "); (yyval.integer) = VTK_PARSE_MUTABLE; }
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 2379 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 2380 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 2381 "vtkParse.y"
    { postSig("static "); (yyval.integer) = VTK_PARSE_STATIC; }
    break;

  case 341:

/* Line 1455 of yacc.c  */
#line 2384 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 342:

/* Line 1455 of yacc.c  */
#line 2385 "vtkParse.y"
    { postSig("virtual "); (yyval.integer) = VTK_PARSE_VIRTUAL; }
    break;

  case 343:

/* Line 1455 of yacc.c  */
#line 2386 "vtkParse.y"
    { postSig("explicit "); (yyval.integer) = VTK_PARSE_EXPLICIT; }
    break;

  case 344:

/* Line 1455 of yacc.c  */
#line 2389 "vtkParse.y"
    { postSig("const "); (yyval.integer) = VTK_PARSE_CONST; }
    break;

  case 345:

/* Line 1455 of yacc.c  */
#line 2390 "vtkParse.y"
    { postSig("volatile "); (yyval.integer) = VTK_PARSE_VOLATILE; }
    break;

  case 347:

/* Line 1455 of yacc.c  */
#line 2395 "vtkParse.y"
    { (yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer)); }
    break;

  case 349:

/* Line 1455 of yacc.c  */
#line 2406 "vtkParse.y"
    { setTypeBase((yyvsp[(1) - (1)].integer)); }
    break;

  case 351:

/* Line 1455 of yacc.c  */
#line 2408 "vtkParse.y"
    { setTypeBase((yyvsp[(2) - (2)].integer)); }
    break;

  case 354:

/* Line 1455 of yacc.c  */
#line 2413 "vtkParse.y"
    { postSig("typename "); }
    break;

  case 355:

/* Line 1455 of yacc.c  */
#line 2414 "vtkParse.y"
    { postSig(" "); setTypeId((yyvsp[(3) - (3)].str)); (yyval.integer) = guess_id_type((yyvsp[(3) - (3)].str)); }
    break;

  case 356:

/* Line 1455 of yacc.c  */
#line 2416 "vtkParse.y"
    { postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = guess_id_type((yyvsp[(1) - (1)].str)); }
    break;

  case 357:

/* Line 1455 of yacc.c  */
#line 2418 "vtkParse.y"
    { postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = guess_id_type((yyvsp[(1) - (1)].str)); }
    break;

  case 358:

/* Line 1455 of yacc.c  */
#line 2420 "vtkParse.y"
    { postSig(" "); setTypeId((yyvsp[(2) - (2)].str)); (yyval.integer) = guess_id_type((yyvsp[(2) - (2)].str)); }
    break;

  case 359:

/* Line 1455 of yacc.c  */
#line 2422 "vtkParse.y"
    { postSig(" "); setTypeId((yyvsp[(2) - (2)].str)); (yyval.integer) = guess_id_type((yyvsp[(2) - (2)].str)); }
    break;

  case 361:

/* Line 1455 of yacc.c  */
#line 2428 "vtkParse.y"
    { setTypeBase((yyvsp[(1) - (1)].integer)); }
    break;

  case 363:

/* Line 1455 of yacc.c  */
#line 2430 "vtkParse.y"
    { setTypeBase((yyvsp[(2) - (2)].integer)); }
    break;

  case 366:

/* Line 1455 of yacc.c  */
#line 2436 "vtkParse.y"
    { postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = guess_id_type((yyvsp[(1) - (1)].str)); }
    break;

  case 367:

/* Line 1455 of yacc.c  */
#line 2438 "vtkParse.y"
    { postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = guess_id_type((yyvsp[(1) - (1)].str)); }
    break;

  case 368:

/* Line 1455 of yacc.c  */
#line 2440 "vtkParse.y"
    { postSig(" "); setTypeId((yyvsp[(2) - (2)].str)); (yyval.integer) = guess_id_type((yyvsp[(2) - (2)].str)); }
    break;

  case 369:

/* Line 1455 of yacc.c  */
#line 2442 "vtkParse.y"
    { postSig(" "); setTypeId((yyvsp[(2) - (2)].str)); (yyval.integer) = guess_id_type((yyvsp[(2) - (2)].str)); }
    break;

  case 370:

/* Line 1455 of yacc.c  */
#line 2444 "vtkParse.y"
    { postSig(" "); setTypeId((yyvsp[(2) - (2)].str)); (yyval.integer) = guess_id_type((yyvsp[(2) - (2)].str)); }
    break;

  case 371:

/* Line 1455 of yacc.c  */
#line 2447 "vtkParse.y"
    { setTypeId(""); }
    break;

  case 373:

/* Line 1455 of yacc.c  */
#line 2451 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_STRING; }
    break;

  case 374:

/* Line 1455 of yacc.c  */
#line 2452 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNICODE_STRING;}
    break;

  case 375:

/* Line 1455 of yacc.c  */
#line 2453 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_OSTREAM; }
    break;

  case 376:

/* Line 1455 of yacc.c  */
#line 2454 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_ISTREAM; }
    break;

  case 377:

/* Line 1455 of yacc.c  */
#line 2455 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 378:

/* Line 1455 of yacc.c  */
#line 2456 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_OBJECT; }
    break;

  case 379:

/* Line 1455 of yacc.c  */
#line 2457 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_QOBJECT; }
    break;

  case 380:

/* Line 1455 of yacc.c  */
#line 2458 "vtkParse.y"
    { typeSig("ssize_t"); (yyval.integer) = VTK_PARSE_SSIZE_T; }
    break;

  case 381:

/* Line 1455 of yacc.c  */
#line 2459 "vtkParse.y"
    { typeSig("size_t"); (yyval.integer) = VTK_PARSE_SIZE_T; }
    break;

  case 382:

/* Line 1455 of yacc.c  */
#line 2460 "vtkParse.y"
    { typeSig("vtkTypeInt8"); (yyval.integer) = VTK_PARSE_INT8; }
    break;

  case 383:

/* Line 1455 of yacc.c  */
#line 2461 "vtkParse.y"
    { typeSig("vtkTypeUInt8"); (yyval.integer) = VTK_PARSE_UINT8; }
    break;

  case 384:

/* Line 1455 of yacc.c  */
#line 2462 "vtkParse.y"
    { typeSig("vtkTypeInt16"); (yyval.integer) = VTK_PARSE_INT16; }
    break;

  case 385:

/* Line 1455 of yacc.c  */
#line 2463 "vtkParse.y"
    { typeSig("vtkTypeUInt16"); (yyval.integer) = VTK_PARSE_UINT16; }
    break;

  case 386:

/* Line 1455 of yacc.c  */
#line 2464 "vtkParse.y"
    { typeSig("vtkTypeInt32"); (yyval.integer) = VTK_PARSE_INT32; }
    break;

  case 387:

/* Line 1455 of yacc.c  */
#line 2465 "vtkParse.y"
    { typeSig("vtkTypeUInt32"); (yyval.integer) = VTK_PARSE_UINT32; }
    break;

  case 388:

/* Line 1455 of yacc.c  */
#line 2466 "vtkParse.y"
    { typeSig("vtkTypeInt64"); (yyval.integer) = VTK_PARSE_INT64; }
    break;

  case 389:

/* Line 1455 of yacc.c  */
#line 2467 "vtkParse.y"
    { typeSig("vtkTypeUInt64"); (yyval.integer) = VTK_PARSE_UINT64; }
    break;

  case 390:

/* Line 1455 of yacc.c  */
#line 2468 "vtkParse.y"
    { typeSig("vtkTypeFloat32"); (yyval.integer) = VTK_PARSE_FLOAT32; }
    break;

  case 391:

/* Line 1455 of yacc.c  */
#line 2469 "vtkParse.y"
    { typeSig("vtkTypeFloat64"); (yyval.integer) = VTK_PARSE_FLOAT64; }
    break;

  case 392:

/* Line 1455 of yacc.c  */
#line 2470 "vtkParse.y"
    { typeSig("vtkIdType"); (yyval.integer) = VTK_PARSE_ID_TYPE; }
    break;

  case 393:

/* Line 1455 of yacc.c  */
#line 2471 "vtkParse.y"
    { typeSig("double"); (yyval.integer) = VTK_PARSE_DOUBLE; }
    break;

  case 394:

/* Line 1455 of yacc.c  */
#line 2474 "vtkParse.y"
    { postSig("void "); (yyval.integer) = VTK_PARSE_VOID; }
    break;

  case 395:

/* Line 1455 of yacc.c  */
#line 2475 "vtkParse.y"
    { postSig("bool "); (yyval.integer) = VTK_PARSE_BOOL; }
    break;

  case 396:

/* Line 1455 of yacc.c  */
#line 2476 "vtkParse.y"
    { postSig("float "); (yyval.integer) = VTK_PARSE_FLOAT; }
    break;

  case 397:

/* Line 1455 of yacc.c  */
#line 2477 "vtkParse.y"
    { postSig("double "); (yyval.integer) = VTK_PARSE_DOUBLE; }
    break;

  case 398:

/* Line 1455 of yacc.c  */
#line 2478 "vtkParse.y"
    { postSig("char "); (yyval.integer) = VTK_PARSE_CHAR; }
    break;

  case 399:

/* Line 1455 of yacc.c  */
#line 2479 "vtkParse.y"
    { postSig("int "); (yyval.integer) = VTK_PARSE_INT; }
    break;

  case 400:

/* Line 1455 of yacc.c  */
#line 2480 "vtkParse.y"
    { postSig("short "); (yyval.integer) = VTK_PARSE_SHORT; }
    break;

  case 401:

/* Line 1455 of yacc.c  */
#line 2481 "vtkParse.y"
    { postSig("long "); (yyval.integer) = VTK_PARSE_LONG; }
    break;

  case 402:

/* Line 1455 of yacc.c  */
#line 2482 "vtkParse.y"
    { postSig("__int64 "); (yyval.integer) = VTK_PARSE___INT64; }
    break;

  case 403:

/* Line 1455 of yacc.c  */
#line 2483 "vtkParse.y"
    { postSig("signed "); (yyval.integer) = VTK_PARSE_INT; }
    break;

  case 404:

/* Line 1455 of yacc.c  */
#line 2484 "vtkParse.y"
    { postSig("unsigned "); (yyval.integer) = VTK_PARSE_UNSIGNED_INT; }
    break;

  case 407:

/* Line 1455 of yacc.c  */
#line 2506 "vtkParse.y"
    { (yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer)); }
    break;

  case 408:

/* Line 1455 of yacc.c  */
#line 2509 "vtkParse.y"
    { postSig("&"); (yyval.integer) = VTK_PARSE_REF; }
    break;

  case 409:

/* Line 1455 of yacc.c  */
#line 2512 "vtkParse.y"
    { postSig("*"); (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 410:

/* Line 1455 of yacc.c  */
#line 2513 "vtkParse.y"
    { postSig("*"); }
    break;

  case 411:

/* Line 1455 of yacc.c  */
#line 2514 "vtkParse.y"
    {
      if (((yyvsp[(3) - (3)].integer) & VTK_PARSE_CONST) != 0)
        {
        (yyval.integer) = VTK_PARSE_CONST_POINTER;
        }
      if (((yyvsp[(3) - (3)].integer) & VTK_PARSE_VOLATILE) != 0)
        {
        (yyval.integer) = VTK_PARSE_BAD_INDIRECT;
        }
    }
    break;

  case 413:

/* Line 1455 of yacc.c  */
#line 2530 "vtkParse.y"
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

  case 414:

/* Line 1455 of yacc.c  */
#line 2546 "vtkParse.y"
    {preSig("void Set"); postSig("(");}
    break;

  case 415:

/* Line 1455 of yacc.c  */
#line 2547 "vtkParse.y"
    {
   postSig("a);");
   currentFunction->Macro = "vtkSetMacro";
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (7)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_parameter(currentFunction, getType(), getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 416:

/* Line 1455 of yacc.c  */
#line 2556 "vtkParse.y"
    {postSig("Get");}
    break;

  case 417:

/* Line 1455 of yacc.c  */
#line 2557 "vtkParse.y"
    {markSig();}
    break;

  case 418:

/* Line 1455 of yacc.c  */
#line 2557 "vtkParse.y"
    {swapSig();}
    break;

  case 419:

/* Line 1455 of yacc.c  */
#line 2558 "vtkParse.y"
    {
   postSig("();");
   currentFunction->Macro = "vtkGetMacro";
   currentFunction->Name = vtkstrcat("Get", (yyvsp[(4) - (9)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, getType(), getTypeId(), 0);
   output_function();
   }
    break;

  case 420:

/* Line 1455 of yacc.c  */
#line 2566 "vtkParse.y"
    {preSig("void Set");}
    break;

  case 421:

/* Line 1455 of yacc.c  */
#line 2567 "vtkParse.y"
    {
   postSig("(char *);");
   currentFunction->Macro = "vtkSetStringMacro";
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(4) - (5)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_parameter(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 422:

/* Line 1455 of yacc.c  */
#line 2576 "vtkParse.y"
    {preSig("char *Get");}
    break;

  case 423:

/* Line 1455 of yacc.c  */
#line 2577 "vtkParse.y"
    {
   postSig("();");
   currentFunction->Macro = "vtkGetStringMacro";
   currentFunction->Name = vtkstrcat("Get", (yyvsp[(4) - (5)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   output_function();
   }
    break;

  case 424:

/* Line 1455 of yacc.c  */
#line 2585 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 425:

/* Line 1455 of yacc.c  */
#line 2585 "vtkParse.y"
    {closeSig();}
    break;

  case 426:

/* Line 1455 of yacc.c  */
#line 2587 "vtkParse.y"
    {
   const char *typeText;
   chopSig();
   typeText = copySig();

   currentFunction->Macro = "vtkSetClampMacro";
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (10)].str));
   currentFunction->Signature =
     vtkstrcat5("void ", currentFunction->Name, "(", typeText, ");");
   currentFunction->Comment = vtkstrdup(getComment());
   add_parameter(currentFunction, getType(), getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();

   currentFunction->Macro = "vtkSetClampMacro";
   currentFunction->Name = vtkstrcat3("Get", (yyvsp[(3) - (10)].str), "MinValue");
   currentFunction->Signature =
     vtkstrcat4(typeText, " ", currentFunction->Name, "();");
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, getType(), getTypeId(), 0);
   output_function();

   currentFunction->Macro = "vtkSetClampMacro";
   currentFunction->Name = vtkstrcat3("Get", (yyvsp[(3) - (10)].str), "MaxValue");
   currentFunction->Signature =
     vtkstrcat4(typeText, " ", currentFunction->Name, "();");
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, getType(), getTypeId(), 0);
   output_function();
   }
    break;

  case 427:

/* Line 1455 of yacc.c  */
#line 2618 "vtkParse.y"
    {preSig("void Set"); postSig("("); }
    break;

  case 428:

/* Line 1455 of yacc.c  */
#line 2619 "vtkParse.y"
    {
   postSig("*);");
   currentFunction->Macro = "vtkSetObjectMacro";
   currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (7)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_parameter(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 429:

/* Line 1455 of yacc.c  */
#line 2628 "vtkParse.y"
    {postSig("*Get");}
    break;

  case 430:

/* Line 1455 of yacc.c  */
#line 2629 "vtkParse.y"
    {markSig();}
    break;

  case 431:

/* Line 1455 of yacc.c  */
#line 2629 "vtkParse.y"
    {swapSig();}
    break;

  case 432:

/* Line 1455 of yacc.c  */
#line 2630 "vtkParse.y"
    {
   postSig("();");
   currentFunction->Macro = "vtkGetObjectMacro";
   currentFunction->Name = vtkstrcat("Get", (yyvsp[(4) - (9)].str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   output_function();
   }
    break;

  case 433:

/* Line 1455 of yacc.c  */
#line 2639 "vtkParse.y"
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

  case 434:

/* Line 1455 of yacc.c  */
#line 2656 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 435:

/* Line 1455 of yacc.c  */
#line 2657 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), getType(), copySig(), 2);
   }
    break;

  case 436:

/* Line 1455 of yacc.c  */
#line 2661 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 437:

/* Line 1455 of yacc.c  */
#line 2662 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), getType(), copySig(), 2);
   }
    break;

  case 438:

/* Line 1455 of yacc.c  */
#line 2666 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 439:

/* Line 1455 of yacc.c  */
#line 2667 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), getType(), copySig(), 3);
   }
    break;

  case 440:

/* Line 1455 of yacc.c  */
#line 2671 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 441:

/* Line 1455 of yacc.c  */
#line 2672 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), getType(), copySig(), 3);
   }
    break;

  case 442:

/* Line 1455 of yacc.c  */
#line 2676 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 443:

/* Line 1455 of yacc.c  */
#line 2677 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), getType(), copySig(), 4);
   }
    break;

  case 444:

/* Line 1455 of yacc.c  */
#line 2681 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 445:

/* Line 1455 of yacc.c  */
#line 2682 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), getType(), copySig(), 4);
   }
    break;

  case 446:

/* Line 1455 of yacc.c  */
#line 2686 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 447:

/* Line 1455 of yacc.c  */
#line 2687 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), getType(), copySig(), 6);
   }
    break;

  case 448:

/* Line 1455 of yacc.c  */
#line 2691 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 449:

/* Line 1455 of yacc.c  */
#line 2692 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), getType(), copySig(), 6);
   }
    break;

  case 450:

/* Line 1455 of yacc.c  */
#line 2696 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 451:

/* Line 1455 of yacc.c  */
#line 2698 "vtkParse.y"
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
   add_parameter(currentFunction, (VTK_PARSE_POINTER | getType()),
                 getTypeId(), (int)strtol((yyvsp[(8) - (9)].str), NULL, 0));
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 452:

/* Line 1455 of yacc.c  */
#line 2713 "vtkParse.y"
    {startSig();}
    break;

  case 453:

/* Line 1455 of yacc.c  */
#line 2715 "vtkParse.y"
    {
   chopSig();
   currentFunction->Macro = "vtkGetVectorMacro";
   currentFunction->Name = vtkstrcat("Get", (yyvsp[(3) - (9)].str));
   postSig(" *");
   postSig(currentFunction->Name);
   postSig("();");
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, (VTK_PARSE_POINTER | getType()),
              getTypeId(), (int)strtol((yyvsp[(8) - (9)].str), NULL, 0));
   output_function();
   }
    break;

  case 454:

/* Line 1455 of yacc.c  */
#line 2728 "vtkParse.y"
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
     add_parameter(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_parameter(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Macro = "vtkViewportCoordinateMacro";
     currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (4)].str));
     currentFunction->Signature =
       vtkstrcat3("void ", currentFunction->Name, "(double a[2]);");
     currentFunction->Comment = vtkstrdup(getComment());
     add_parameter(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 2);
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

  case 455:

/* Line 1455 of yacc.c  */
#line 2765 "vtkParse.y"
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
     add_parameter(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_parameter(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_parameter(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Macro = "vtkWorldCoordinateMacro";
     currentFunction->Name = vtkstrcat("Set", (yyvsp[(3) - (4)].str));
     currentFunction->Signature =
       vtkstrcat3("void ", currentFunction->Name, "(double a[3]);");
     currentFunction->Comment = vtkstrdup(getComment());
     add_parameter(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 3);
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

  case 456:

/* Line 1455 of yacc.c  */
#line 2803 "vtkParse.y"
    {
   int is_concrete = 0;
   int i;

   currentFunction->Macro = "vtkTypeMacro";
   currentFunction->Name = "GetClassName";
   currentFunction->Signature = "const char *GetClassName();";
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR),
              "char", 0);
   output_function();

   currentFunction->Macro = "vtkTypeMacro";
   currentFunction->Name = "IsA";
   currentFunction->Signature = "int IsA(const char *name);";
   currentFunction->Comment = vtkstrdup(getComment());
   add_parameter(currentFunction, (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR),
                "char", 0);
   set_return(currentFunction, VTK_PARSE_INT, "int", 0);
   output_function();

   currentFunction->Macro = "vtkTypeMacro";
   currentFunction->Name = "NewInstance";
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
     currentFunction->Name = "SafeDownCast";
     currentFunction->Signature =
       vtkstrcat((yyvsp[(3) - (7)].str), " *SafeDownCast(vtkObject* o);");
     currentFunction->Comment = vtkstrdup(getComment());
     add_parameter(currentFunction, VTK_PARSE_OBJECT_PTR, "vtkObject", 0);
     set_return(currentFunction, (VTK_PARSE_STATIC | VTK_PARSE_OBJECT_PTR),
                (yyvsp[(3) - (7)].str), 0);
     output_function();
     }
   }
    break;

  case 459:

/* Line 1455 of yacc.c  */
#line 2862 "vtkParse.y"
    { (yyval.str) = "()"; }
    break;

  case 460:

/* Line 1455 of yacc.c  */
#line 2863 "vtkParse.y"
    { (yyval.str) = "[]"; }
    break;

  case 461:

/* Line 1455 of yacc.c  */
#line 2864 "vtkParse.y"
    { (yyval.str) = " new[]"; }
    break;

  case 462:

/* Line 1455 of yacc.c  */
#line 2865 "vtkParse.y"
    { (yyval.str) = " delete[]"; }
    break;

  case 463:

/* Line 1455 of yacc.c  */
#line 2866 "vtkParse.y"
    { (yyval.str) = "<"; }
    break;

  case 464:

/* Line 1455 of yacc.c  */
#line 2867 "vtkParse.y"
    { (yyval.str) = ">"; }
    break;

  case 465:

/* Line 1455 of yacc.c  */
#line 2868 "vtkParse.y"
    { (yyval.str) = ","; }
    break;

  case 466:

/* Line 1455 of yacc.c  */
#line 2869 "vtkParse.y"
    { (yyval.str) = "="; }
    break;

  case 468:

/* Line 1455 of yacc.c  */
#line 2873 "vtkParse.y"
    { (yyval.str) = "%"; }
    break;

  case 469:

/* Line 1455 of yacc.c  */
#line 2874 "vtkParse.y"
    { (yyval.str) = "*"; }
    break;

  case 470:

/* Line 1455 of yacc.c  */
#line 2875 "vtkParse.y"
    { (yyval.str) = "/"; }
    break;

  case 471:

/* Line 1455 of yacc.c  */
#line 2876 "vtkParse.y"
    { (yyval.str) = "-"; }
    break;

  case 472:

/* Line 1455 of yacc.c  */
#line 2877 "vtkParse.y"
    { (yyval.str) = "+"; }
    break;

  case 473:

/* Line 1455 of yacc.c  */
#line 2878 "vtkParse.y"
    { (yyval.str) = "!"; }
    break;

  case 474:

/* Line 1455 of yacc.c  */
#line 2879 "vtkParse.y"
    { (yyval.str) = "~"; }
    break;

  case 475:

/* Line 1455 of yacc.c  */
#line 2880 "vtkParse.y"
    { (yyval.str) = "&"; }
    break;

  case 476:

/* Line 1455 of yacc.c  */
#line 2881 "vtkParse.y"
    { (yyval.str) = "|"; }
    break;

  case 477:

/* Line 1455 of yacc.c  */
#line 2882 "vtkParse.y"
    { (yyval.str) = "^"; }
    break;

  case 478:

/* Line 1455 of yacc.c  */
#line 2883 "vtkParse.y"
    { (yyval.str) = " new"; }
    break;

  case 479:

/* Line 1455 of yacc.c  */
#line 2884 "vtkParse.y"
    { (yyval.str) = " delete"; }
    break;

  case 480:

/* Line 1455 of yacc.c  */
#line 2885 "vtkParse.y"
    { (yyval.str) = "<<="; }
    break;

  case 481:

/* Line 1455 of yacc.c  */
#line 2886 "vtkParse.y"
    { (yyval.str) = ">>="; }
    break;

  case 482:

/* Line 1455 of yacc.c  */
#line 2887 "vtkParse.y"
    { (yyval.str) = "<<"; }
    break;

  case 483:

/* Line 1455 of yacc.c  */
#line 2888 "vtkParse.y"
    { (yyval.str) = ">>"; }
    break;

  case 484:

/* Line 1455 of yacc.c  */
#line 2889 "vtkParse.y"
    { (yyval.str) = ".*"; }
    break;

  case 485:

/* Line 1455 of yacc.c  */
#line 2890 "vtkParse.y"
    { (yyval.str) = "->*"; }
    break;

  case 486:

/* Line 1455 of yacc.c  */
#line 2891 "vtkParse.y"
    { (yyval.str) = "->"; }
    break;

  case 487:

/* Line 1455 of yacc.c  */
#line 2892 "vtkParse.y"
    { (yyval.str) = "+="; }
    break;

  case 488:

/* Line 1455 of yacc.c  */
#line 2893 "vtkParse.y"
    { (yyval.str) = "-="; }
    break;

  case 489:

/* Line 1455 of yacc.c  */
#line 2894 "vtkParse.y"
    { (yyval.str) = "*="; }
    break;

  case 490:

/* Line 1455 of yacc.c  */
#line 2895 "vtkParse.y"
    { (yyval.str) = "/="; }
    break;

  case 491:

/* Line 1455 of yacc.c  */
#line 2896 "vtkParse.y"
    { (yyval.str) = "%="; }
    break;

  case 492:

/* Line 1455 of yacc.c  */
#line 2897 "vtkParse.y"
    { (yyval.str) = "++"; }
    break;

  case 493:

/* Line 1455 of yacc.c  */
#line 2898 "vtkParse.y"
    { (yyval.str) = "--"; }
    break;

  case 494:

/* Line 1455 of yacc.c  */
#line 2899 "vtkParse.y"
    { (yyval.str) = "&="; }
    break;

  case 495:

/* Line 1455 of yacc.c  */
#line 2900 "vtkParse.y"
    { (yyval.str) = "|="; }
    break;

  case 496:

/* Line 1455 of yacc.c  */
#line 2901 "vtkParse.y"
    { (yyval.str) = "^="; }
    break;

  case 497:

/* Line 1455 of yacc.c  */
#line 2902 "vtkParse.y"
    { (yyval.str) = "&&"; }
    break;

  case 498:

/* Line 1455 of yacc.c  */
#line 2903 "vtkParse.y"
    { (yyval.str) = "||"; }
    break;

  case 499:

/* Line 1455 of yacc.c  */
#line 2904 "vtkParse.y"
    { (yyval.str) = "=="; }
    break;

  case 500:

/* Line 1455 of yacc.c  */
#line 2905 "vtkParse.y"
    { (yyval.str) = "!="; }
    break;

  case 501:

/* Line 1455 of yacc.c  */
#line 2906 "vtkParse.y"
    { (yyval.str) = "<="; }
    break;

  case 502:

/* Line 1455 of yacc.c  */
#line 2907 "vtkParse.y"
    { (yyval.str) = ">="; }
    break;

  case 503:

/* Line 1455 of yacc.c  */
#line 2910 "vtkParse.y"
    { (yyval.str) = "typedef"; }
    break;

  case 504:

/* Line 1455 of yacc.c  */
#line 2911 "vtkParse.y"
    { (yyval.str) = "typename"; }
    break;

  case 505:

/* Line 1455 of yacc.c  */
#line 2912 "vtkParse.y"
    { (yyval.str) = "class"; }
    break;

  case 506:

/* Line 1455 of yacc.c  */
#line 2913 "vtkParse.y"
    { (yyval.str) = "struct"; }
    break;

  case 507:

/* Line 1455 of yacc.c  */
#line 2914 "vtkParse.y"
    { (yyval.str) = "union"; }
    break;

  case 508:

/* Line 1455 of yacc.c  */
#line 2915 "vtkParse.y"
    { (yyval.str) = "template"; }
    break;

  case 509:

/* Line 1455 of yacc.c  */
#line 2916 "vtkParse.y"
    { (yyval.str) = "public"; }
    break;

  case 510:

/* Line 1455 of yacc.c  */
#line 2917 "vtkParse.y"
    { (yyval.str) = "protected"; }
    break;

  case 511:

/* Line 1455 of yacc.c  */
#line 2918 "vtkParse.y"
    { (yyval.str) = "private"; }
    break;

  case 512:

/* Line 1455 of yacc.c  */
#line 2919 "vtkParse.y"
    { (yyval.str) = "const"; }
    break;

  case 513:

/* Line 1455 of yacc.c  */
#line 2920 "vtkParse.y"
    { (yyval.str) = "static"; }
    break;

  case 514:

/* Line 1455 of yacc.c  */
#line 2921 "vtkParse.y"
    { (yyval.str) = "inline"; }
    break;

  case 515:

/* Line 1455 of yacc.c  */
#line 2922 "vtkParse.y"
    { (yyval.str) = "virtual"; }
    break;

  case 516:

/* Line 1455 of yacc.c  */
#line 2923 "vtkParse.y"
    { (yyval.str) = "extern"; }
    break;

  case 517:

/* Line 1455 of yacc.c  */
#line 2924 "vtkParse.y"
    { (yyval.str) = "namespace"; }
    break;

  case 518:

/* Line 1455 of yacc.c  */
#line 2925 "vtkParse.y"
    { (yyval.str) = "operator"; }
    break;

  case 519:

/* Line 1455 of yacc.c  */
#line 2926 "vtkParse.y"
    { (yyval.str) = "enum"; }
    break;

  case 520:

/* Line 1455 of yacc.c  */
#line 2927 "vtkParse.y"
    { (yyval.str) = "throw"; }
    break;

  case 521:

/* Line 1455 of yacc.c  */
#line 2928 "vtkParse.y"
    { (yyval.str) = "const_cast"; }
    break;

  case 522:

/* Line 1455 of yacc.c  */
#line 2929 "vtkParse.y"
    { (yyval.str) = "dynamic_cast"; }
    break;

  case 523:

/* Line 1455 of yacc.c  */
#line 2930 "vtkParse.y"
    { (yyval.str) = "static_cast"; }
    break;

  case 524:

/* Line 1455 of yacc.c  */
#line 2931 "vtkParse.y"
    { (yyval.str) = "reinterpret_cast"; }
    break;

  case 537:

/* Line 1455 of yacc.c  */
#line 2955 "vtkParse.y"
    {
      if ((((yyvsp[(1) - (1)].str))[0] == '+' || ((yyvsp[(1) - (1)].str))[0] == '-' ||
           ((yyvsp[(1) - (1)].str))[0] == '*' || ((yyvsp[(1) - (1)].str))[0] == '&') &&
          ((yyvsp[(1) - (1)].str))[1] == '\0')
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
        postSig((yyvsp[(1) - (1)].str));
        if ((c1 >= 'A' && c1 <= 'Z') || (c1 >= 'a' && c1 <= 'z') ||
            (c1 >= '0' && c1 <= '9') || c1 == '_' || c1 == '\'' ||
            c1 == '\"' || c1 == ')' || c1 == ']')
          {
          postSig(" ");
          }
        }
       else
        {
        postSig((yyvsp[(1) - (1)].str));
        postSig(" ");
        }
    }
    break;

  case 538:

/* Line 1455 of yacc.c  */
#line 2985 "vtkParse.y"
    { postSig(":"); postSig(" "); }
    break;

  case 539:

/* Line 1455 of yacc.c  */
#line 2985 "vtkParse.y"
    { postSig("."); }
    break;

  case 540:

/* Line 1455 of yacc.c  */
#line 2986 "vtkParse.y"
    { chopSig(); postSig("::"); }
    break;

  case 541:

/* Line 1455 of yacc.c  */
#line 2987 "vtkParse.y"
    { postSig((yyvsp[(1) - (1)].str)); postSig(" "); }
    break;

  case 542:

/* Line 1455 of yacc.c  */
#line 2988 "vtkParse.y"
    { postSig((yyvsp[(1) - (1)].str)); postSig(" "); }
    break;

  case 544:

/* Line 1455 of yacc.c  */
#line 2991 "vtkParse.y"
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
    }
    break;

  case 548:

/* Line 1455 of yacc.c  */
#line 3017 "vtkParse.y"
    { postSig("< "); }
    break;

  case 549:

/* Line 1455 of yacc.c  */
#line 3018 "vtkParse.y"
    { postSig("> "); }
    break;

  case 551:

/* Line 1455 of yacc.c  */
#line 3021 "vtkParse.y"
    { postSig("= "); }
    break;

  case 552:

/* Line 1455 of yacc.c  */
#line 3022 "vtkParse.y"
    { chopSig(); postSig(", "); }
    break;

  case 554:

/* Line 1455 of yacc.c  */
#line 3025 "vtkParse.y"
    { chopSig(); postSig(";"); }
    break;

  case 562:

/* Line 1455 of yacc.c  */
#line 3039 "vtkParse.y"
    { postSig("= "); }
    break;

  case 563:

/* Line 1455 of yacc.c  */
#line 3040 "vtkParse.y"
    { chopSig(); postSig(", "); }
    break;

  case 564:

/* Line 1455 of yacc.c  */
#line 3044 "vtkParse.y"
    {
      chopSig();
      if (getSig()[getSigLength()-1] == '<') { postSig(" "); }
      postSig("<");
    }
    break;

  case 565:

/* Line 1455 of yacc.c  */
#line 3050 "vtkParse.y"
    {
      chopSig();
      if (getSig()[getSigLength()-1] == '>') { postSig(" "); }
      postSig("> ");
    }
    break;

  case 566:

/* Line 1455 of yacc.c  */
#line 3057 "vtkParse.y"
    { postSig("["); }
    break;

  case 567:

/* Line 1455 of yacc.c  */
#line 3058 "vtkParse.y"
    { chopSig(); postSig("] "); }
    break;

  case 568:

/* Line 1455 of yacc.c  */
#line 3061 "vtkParse.y"
    { postSig("("); }
    break;

  case 569:

/* Line 1455 of yacc.c  */
#line 3062 "vtkParse.y"
    { chopSig(); postSig(") "); }
    break;

  case 570:

/* Line 1455 of yacc.c  */
#line 3063 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("*"); }
    break;

  case 571:

/* Line 1455 of yacc.c  */
#line 3064 "vtkParse.y"
    { chopSig(); postSig(") "); }
    break;

  case 572:

/* Line 1455 of yacc.c  */
#line 3065 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("&"); }
    break;

  case 573:

/* Line 1455 of yacc.c  */
#line 3066 "vtkParse.y"
    { chopSig(); postSig(") "); }
    break;

  case 574:

/* Line 1455 of yacc.c  */
#line 3069 "vtkParse.y"
    { postSig("{ "); }
    break;

  case 575:

/* Line 1455 of yacc.c  */
#line 3069 "vtkParse.y"
    { postSig("} "); }
    break;



/* Line 1455 of yacc.c  */
#line 7956 "vtkParse.tab.c"
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
#line 3108 "vtkParse.y"

#include <string.h>
#include "lex.yy.c"

/* fill in the type name if none given */
const char *type_class(unsigned int type, const char *classname)
{
  if (classname)
    {
    if (classname[0] == '\0')
      {
      switch ((type & VTK_PARSE_BASE_TYPE))
        {
        case VTK_PARSE_VOID:
          classname = "void";
          break;
        case VTK_PARSE_BOOL:
          classname = "bool";
          break;
        case VTK_PARSE_FLOAT:
          classname = "float";
          break;
        case VTK_PARSE_DOUBLE:
          classname = "double";
          break;
        case VTK_PARSE_LONG_DOUBLE:
          classname = "long double";
          break;
        case VTK_PARSE_CHAR:
          classname = "char";
          break;
        case VTK_PARSE_UNSIGNED_CHAR:
          classname = "unsigned char";
          break;
        case VTK_PARSE_SIGNED_CHAR:
          classname = "signed char";
          break;
        case VTK_PARSE_SHORT:
          classname = "short";
          break;
        case VTK_PARSE_UNSIGNED_SHORT:
          classname = "unsigned short";
          break;
        case VTK_PARSE_INT:
          classname = "int";
          break;
        case VTK_PARSE_UNSIGNED_INT:
          classname = "unsigned int";
          break;
        case VTK_PARSE_LONG:
          classname = "long";
          break;
        case VTK_PARSE_UNSIGNED_LONG:
          classname = "unsigned long";
          break;
        case VTK_PARSE_LONG_LONG:
          classname = "long long";
          break;
        case VTK_PARSE_UNSIGNED_LONG_LONG:
          classname = "unsigned long long";
          break;
        case VTK_PARSE___INT64:
          classname = "__int64";
          break;
        case VTK_PARSE_UNSIGNED___INT64:
          classname = "unsigned __int64";
          break;
        }
      }
    }

  return classname;
}

/* check whether this is the class we are looking for */
void start_class(const char *classname, int is_struct_or_union)
{
  ClassInfo *outerClass = currentClass;
  pushClass();
  currentClass = (ClassInfo *)malloc(sizeof(ClassInfo));
  vtkParse_InitClass(currentClass);
  currentClass->Name = classname;
  if (is_struct_or_union == 1)
    {
    currentClass->ItemType = VTK_STRUCT_INFO;
    }
  if (is_struct_or_union == 2)
    {
    currentClass->ItemType = VTK_UNION_INFO;
    }

  if (classname && classname[strlen(classname)-1] != '>')
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
  currentClass->Name = classname;
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
  vtkParse_AddDefaultConstructors(currentClass, data->Strings);

  popClass();
}

/* add a base class to the specified class */
void add_base_class(ClassInfo *cls, const char *name, int al, int virt)
{
  if (cls && al == VTK_ACCESS_PUBLIC && virt == 0)
    {
    vtkParse_AddStringToArray(&cls->SuperClasses,
                              &cls->NumberOfSuperClasses,
                              name);
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
    item->Scope = name;
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
  EnumInfo *item;

  currentEnumName = "int";
  currentEnumValue = NULL;
  if (name)
    {
    currentEnumName = name;
    item = (EnumInfo *)malloc(sizeof(EnumInfo));
    vtkParse_InitEnum(item);
    item->Name = name;
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
    currentEnumValue = value;
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
    currentEnumValue = vtkstrdup(text);
    }
  else
    {
    strcpy(text, "0");
    currentEnumValue = "0";
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
      preprocessor, valstring);

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
      preprocessor, valstring, &val, &is_unsigned);

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
  con->Name = name;
  con->Value = value;
  con->Type = type;
  con->Class = type_class(type, typeclass);

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

/* guess the type from the ID */
unsigned int guess_id_type(const char *cp)
{
  unsigned int t = 0;

  if (cp)
    {
    size_t i;
    const char *dp;

    i = strlen(cp);
    while (i > 0 && cp[i-1] != ':') { i--; }
    dp = &cp[i];

    if (strcmp(dp, "vtkStdString") == 0 ||
        strcmp(cp, "std::string") == 0)
      {
      t = VTK_PARSE_STRING;
      }
    else if (strcmp(dp, "vtkUnicodeString") == 0)
      {
      t = VTK_PARSE_UNICODE_STRING;
      }
    else if (strncmp(dp, "vtk", 3) == 0)
      {
      t = VTK_PARSE_OBJECT;
      }
    else if (strncmp(dp, "Q", 1) == 0 ||
             strncmp(cp, "Qt::", 4) == 0)
      {
      t = VTK_PARSE_QOBJECT;
      }
    else
      {
      t = VTK_PARSE_UNKNOWN;
      }
    }

  return t;
}

/* add a template parameter to the current template */
void add_template_parameter(
  unsigned int datatype, unsigned int extra, const char *funcSig)
{
  ValueInfo *param = (ValueInfo *)malloc(sizeof(ValueInfo));
  vtkParse_InitValue(param);
  handle_complex_type(param, datatype, extra, funcSig);
  param->Name = getVarName();
  vtkParse_AddParameterToTemplate(currentTemplate, param);
}

/* add a parameter to a function */
void add_parameter(FunctionInfo *func, unsigned int type,
                   const char *typeclass, int count)
{
  char text[64];
  ValueInfo *param = (ValueInfo *)malloc(sizeof(ValueInfo));
  vtkParse_InitValue(param);

  param->Type = type;
  param->Class = type_class(type, typeclass);

  if (count)
    {
    param->Count = count;
    sprintf(text, "%i", count);
    vtkParse_AddStringToArray(&param->Dimensions, &param->NumberOfDimensions,
                              vtkstrdup(text));
    }

  add_legacy_parameter(func, param);

  vtkParse_AddParameterToFunction(func, param);
}

/* set the return type for the current function */
void set_return(FunctionInfo *func, unsigned int type,
                const char *typeclass, int count)
{
  char text[64];
  ValueInfo *val = (ValueInfo *)malloc(sizeof(ValueInfo));

  vtkParse_InitValue(val);
  val->Type = type;
  val->Class = type_class(type, typeclass);

  if (count)
    {
    val->Count = count;
    sprintf(text, "%i", count);
    vtkParse_AddStringToArray(&val->Dimensions, &val->NumberOfDimensions,
                              vtkstrdup(text));
    }

  func->ReturnValue = val;

#ifndef VTK_PARSE_LEGACY_REMOVE
  func->ReturnType = val->Type;
  func->ReturnClass = val->Class;
  func->HaveHint = (count > 0);
  func->HintSize = count;
#endif
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

  /* remove specifiers like "friend" and "typedef" */
  datatype &= VTK_PARSE_QUALIFIED_TYPE;

  /* if "extra" was set, parentheses were involved */
  if ((extra & VTK_PARSE_BASE_TYPE) == VTK_PARSE_FUNCTION)
    {
    /* the current type becomes the function return type */
    func = getFunction();
    func->ReturnValue = (ValueInfo *)malloc(sizeof(ValueInfo));
    vtkParse_InitValue(func->ReturnValue);
    func->ReturnValue->Type = datatype;
    func->ReturnValue->Class = type_class(datatype, getTypeId());
    if (funcSig) { func->Signature = vtkstrdup(funcSig); }
    val->Function = func;

#ifndef VTK_PARSE_LEGACY_REMOVE
    func->ReturnType = func->ReturnValue->Type;
    func->ReturnClass = func->ReturnValue->Class;
#endif

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
  val->Class = type_class(datatype, getTypeId());

  /* copy contents of all brackets to the ArgDimensions */
  val->NumberOfDimensions = getArrayNDims();
  val->Dimensions = getArray();
  clearArray();

  /* count is the product of the dimensions */
  val->Count = count_from_dimensions(val);
}

/* add a parameter to the legacy part of the FunctionInfo struct */
void add_legacy_parameter(FunctionInfo *func, ValueInfo *param)
{
#ifndef VTK_PARSE_LEGACY_REMOVE
  int i = func->NumberOfArguments;

  if (i < MAX_ARGS)
    {
    func->NumberOfArguments = i + 1;
    func->ArgTypes[i] = param->Type;
    func->ArgClasses[i] = param->Class;
    func->ArgCounts[i] = param->Count;

    /* legacy wrappers need VTK_PARSE_FUNCTION without POINTER */
    if (param->Type == VTK_PARSE_FUNCTION_PTR)
      {
      /* check for signature "void (*func)(void *)" */
      if (param->Function->NumberOfParameters == 1 &&
          param->Function->Parameters[0]->Type == VTK_PARSE_VOID_PTR &&
          param->Function->Parameters[0]->NumberOfDimensions == 0 &&
          param->Function->ReturnValue->Type == VTK_PARSE_VOID)
        {
        func->ArgTypes[i] = VTK_PARSE_FUNCTION;
        }
      }
    }
  else
    {
    func->ArrayFailure = 1;
    }
#endif
}


/* reject the function, do not output it */
void reject_function()
{
  vtkParse_InitFunction(currentFunction);
  startSig();
  getMacro();
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
  if (currentFunction->ReturnValue &&
      currentFunction->ReturnValue->Type & VTK_PARSE_STATIC)
    {
    currentFunction->IsStatic = 1;
    }

  /* virtual */
  if (currentFunction->ReturnValue &&
      currentFunction->ReturnValue->Type & VTK_PARSE_VIRTUAL)
    {
    currentFunction->IsVirtual = 1;
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

  /* a void argument is the same as no parameters */
  if (currentFunction->NumberOfParameters == 1 &&
      (currentFunction->Parameters[0]->Type & VTK_PARSE_UNQUALIFIED_TYPE) ==
      VTK_PARSE_VOID)
    {
    currentFunction->NumberOfParameters = 0;
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
    }
  else
    {
    currentFunction->Access = VTK_ACCESS_PUBLIC;
    }

#ifndef VTK_PARSE_LEGACY_REMOVE
  /* a void argument is the same as no parameters */
  if (currentFunction->NumberOfArguments == 1 &&
      (currentFunction->ArgTypes[0] & VTK_PARSE_UNQUALIFIED_TYPE) ==
      VTK_PARSE_VOID)
    {
    currentFunction->NumberOfArguments = 0;
    }

  /* if return type is void, set return class to void */
  if (currentFunction->ReturnClass == NULL &&
      (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE) ==
       VTK_PARSE_VOID)
    {
    currentFunction->ReturnClass = "void";
    }

  /* set legacy flags */
  if (currentClass)
    {
    currentFunction->IsPublic = (access_level == VTK_ACCESS_PUBLIC);
    currentFunction->IsProtected = (access_level == VTK_ACCESS_PROTECTED);
    }
  else
    {
    currentFunction->IsPublic = 1;
    currentFunction->IsProtected = 0;
    }

  /* check for too many parameters */
  if (currentFunction->NumberOfParameters > MAX_ARGS)
    {
    currentFunction->ArrayFailure = 1;
    }

  for (i = 0; i < currentFunction->NumberOfParameters; i++)
    {
    ValueInfo *param = currentFunction->Parameters[i];
    /* tell old wrappers that multi-dimensional arrays are bad */
    if ((param->Type & VTK_PARSE_POINTER_MASK) != 0)
      {
      if (((param->Type & VTK_PARSE_INDIRECT) == VTK_PARSE_BAD_INDIRECT) ||
          ((param->Type & VTK_PARSE_POINTER_LOWMASK) != VTK_PARSE_POINTER))
        {
        currentFunction->ArrayFailure = 1;
        }
      }

    /* allow only "void (*func)(void *)" as a valid function pointer */
    if ((param->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_FUNCTION)
      {
      if (i != 0 || param->Type != VTK_PARSE_FUNCTION_PTR ||
          currentFunction->NumberOfParameters != 2 ||
          currentFunction->Parameters[1]->Type != VTK_PARSE_VOID_PTR ||
          param->Function->NumberOfParameters != 1 ||
          param->Function->Parameters[0]->Type != VTK_PARSE_VOID_PTR ||
          param->Function->Parameters[0]->NumberOfDimensions != 0 ||
          param->Function->ReturnValue->Type != VTK_PARSE_VOID)
        {
        currentFunction->ArrayFailure = 1;
        }
      }
    }
#endif /* VTK_PARSE_LEGACY_REMOVE */

  if (currentClass)
    {
    /* is it a delete function */
    if (currentFunction->Name && !strcmp("Delete",currentFunction->Name))
      {
      currentClass->HasDelete = 1;
      }

    currentFunction->Class = currentClass->Name;
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
        if (currentNamespace->Functions[i]->NumberOfParameters ==
            currentFunction->NumberOfParameters)
          {
          for (j = 0; j < currentFunction->NumberOfParameters; j++)
            {
            if (currentNamespace->Functions[i]->Parameters[j]->Type ==
                currentFunction->Parameters[j]->Type)
              {
              if (currentFunction->Parameters[j]->Type == VTK_PARSE_OBJECT &&
                  strcmp(currentNamespace->Functions[i]->Parameters[j]->Class,
                         currentFunction->Parameters[j]->Class) == 0)
                {
                break;
                }
              }
            }
          if (j == currentFunction->NumberOfParameters)
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

void outputSetVectorMacro(const char *var, unsigned int paramType,
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
    add_parameter(currentFunction, paramType, getTypeId(), 0);
    }
  set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
  output_function();

  currentFunction->Macro = mnames[m];
  currentFunction->Name = vtkstrcat("Set", var);
  currentFunction->Signature =
    vtkstrcat7("void ", currentFunction->Name, "(", typeText,
               " a[", ntext, "]);");
  add_parameter(currentFunction, (VTK_PARSE_POINTER | paramType),
                getTypeId(), n);
  set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
  output_function();
}

void outputGetVectorMacro(const char *var, unsigned int paramType,
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
  set_return(currentFunction, (VTK_PARSE_POINTER | paramType), getTypeId(), n);
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

/* Set a flag to recurse into included files */
void vtkParse_SetRecursive(int option)
{
  if (option)
    {
    Recursive = 1;
    }
  else
    {
    Recursive = 0;
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

  /* "data" is a global variable used by the parser */
  data = (FileInfo *)malloc(sizeof(FileInfo));
  vtkParse_InitFile(data);
  data->Strings = (StringCache *)malloc(sizeof(StringCache));
  vtkParse_InitStringCache(data->Strings);

  /* "preprocessor" is a global struct used by the parser */
  preprocessor = (PreprocessInfo *)malloc(sizeof(PreprocessInfo));
  vtkParsePreprocess_Init(preprocessor, filename);
  vtkParsePreprocess_AddStandardMacros(preprocessor, VTK_PARSE_NATIVE);

  /* add include files specified on the command line */
  for (i = 0; i < NumberOfIncludeDirectories; i++)
    {
    vtkParsePreprocess_IncludeDirectory(preprocessor, IncludeDirectories[i]);
    }

  /* add macros specified on the command line */
  for (i = 0; i < NumberOfDefinitions; i++)
    {
    const char *cp = Definitions[i];

    if (*cp == 'U')
      {
      vtkParsePreprocess_RemoveMacro(preprocessor, &cp[1]);
      }
    else if (*cp == 'D')
      {
      const char *definition = &cp[1];
      while (*definition != '=' && *definition != '\0')
        {
        definition++;
        }
      if (*definition == '=')
        {
        definition++;
        }
      else
        {
        definition = NULL;
        }
      vtkParsePreprocess_AddMacro(preprocessor, &cp[1], definition);
      }
    }

  /* should explicitly check for vtkConfigure.h, or even explicitly load it */
#ifdef VTK_USE_64BIT_IDS
  vtkParsePreprocess_AddMacro(preprocessor, "VTK_USE_64BIT_IDS", NULL);
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

  vtkParsePreprocess_Free(preprocessor);
  preprocessor = NULL;
  macroName = NULL;

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

          if ((strcmp(h_func, func_info->Name) == 0) &&
              func_info->ReturnValue &&
              (type == ((func_info->ReturnValue->Type & ~VTK_PARSE_REF) &
                        VTK_PARSE_UNQUALIFIED_TYPE)))
            {
            /* types that hints are accepted for */
            switch (func_info->ReturnValue->Type & VTK_PARSE_UNQUALIFIED_TYPE)
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
                if (func_info->ReturnValue->NumberOfDimensions == 0)
                  {
                  char text[64];
                  sprintf(text, "%i", h_value);
                  func_info->ReturnValue->Count = h_value;
                  vtkParse_AddStringToArray(
                    &func_info->ReturnValue->Dimensions,
                    &func_info->ReturnValue->NumberOfDimensions,
                    vtkParse_CacheString(
                      file_info->Strings, text, strlen(text)));
#ifndef VTK_PARSE_LEGACY_REMOVE
                  func_info->HaveHint = 1;
                  func_info->HintSize = h_value;
#endif
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
  vtkParse_FreeStringCache(file_info->Strings);
  free(file_info->Strings);
  free(file_info);
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
     char *cp = (char *)malloc(strlen(classname) + 1);
     strcpy(cp, classname);

     vtkParse_AddStringToArray(&ConcreteClasses,
                               &NumberOfConcreteClasses,
                               cp);
     }
}

/** Define a preprocessor macro. Function macros are not supported.  */
void vtkParse_DefineMacro(const char *name, const char *definition)
{
  size_t n = vtkidlen(name);
  size_t l;
  char *cp;

  if (definition == NULL)
    {
    definition = "";
    }

  l = n + strlen(definition) + 3;
  cp = (char *)malloc(l);
  cp[0] = 'D';
  strncpy(&cp[1], name, n);
  cp[n+1] = '\0';
  if (definition[0] != '\0')
    {
    cp[n+1] = '=';
    strcpy(&cp[n+2], definition);
    }
  cp[l] = '\0';

  vtkParse_AddStringToArray(&Definitions, &NumberOfDefinitions, cp);
}

/** Undefine a preprocessor macro.  */
void vtkParse_UndefineMacro(const char *name)
{
  size_t n = vtkidlen(name);
  char *cp;

  cp = (char *)malloc(n+2);
  cp[0] = 'U';
  strncpy(&cp[1], name, n);
  cp[n+1] = '\0';

  vtkParse_AddStringToArray(&Definitions, &NumberOfDefinitions, cp);
}

/** Add an include directory, for use with the "-I" option.  */
void vtkParse_IncludeDirectory(const char *dirname)
{
  size_t n = strlen(dirname);
  char *cp;
  int i;

  for (i = 0; i < NumberOfIncludeDirectories; i++)
    {
    if (strncmp(IncludeDirectories[i], dirname, n) == 0 &&
        IncludeDirectories[i][n] == '\0')
      {
      return;
      }
    }

  cp = (char *)malloc(n+1);
  strcpy(cp, dirname);

  vtkParse_AddStringToArray(
    &IncludeDirectories, &NumberOfIncludeDirectories, cp);
}

/** Return the full path to a header file.  */
const char *vtkParse_FindIncludeFile(const char *filename)
{
  static PreprocessInfo info = {0, 0, 0, 0, 0, 0, 0, 0, 0};
  int val;
  int i;

  /* add include files specified on the command line */
  for (i = 0; i < NumberOfIncludeDirectories; i++)
    {
    vtkParsePreprocess_IncludeDirectory(&info, IncludeDirectories[i]);
    }

  return vtkParsePreprocess_FindIncludeFile(&info, filename, 0, &val);
}
