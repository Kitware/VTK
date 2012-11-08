
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
#line 1354 "vtkParse.tab.c"

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
     TypeInt8 = 343,
     TypeUInt8 = 344,
     TypeInt16 = 345,
     TypeUInt16 = 346,
     TypeInt32 = 347,
     TypeUInt32 = 348,
     TypeInt64 = 349,
     TypeUInt64 = 350,
     TypeFloat32 = 351,
     TypeFloat64 = 352,
     SetMacro = 353,
     GetMacro = 354,
     SetStringMacro = 355,
     GetStringMacro = 356,
     SetClampMacro = 357,
     SetObjectMacro = 358,
     GetObjectMacro = 359,
     BooleanMacro = 360,
     SetVector2Macro = 361,
     SetVector3Macro = 362,
     SetVector4Macro = 363,
     SetVector6Macro = 364,
     GetVector2Macro = 365,
     GetVector3Macro = 366,
     GetVector4Macro = 367,
     GetVector6Macro = 368,
     SetVectorMacro = 369,
     GetVectorMacro = 370,
     ViewportCoordinateMacro = 371,
     WorldCoordinateMacro = 372,
     TypeMacro = 373,
     VTK_BYTE_SWAP_DECL = 374
   };
#endif




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 222 of yacc.c  */
#line 1302 "vtkParse.y"

  const char   *str;
  unsigned int  integer;



/* Line 222 of yacc.c  */
#line 1635 "vtkParse.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 1647 "vtkParse.tab.c"

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
#define YYLAST   5657

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  143
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  240
/* YYNRULES -- Number of rules.  */
#define YYNRULES  603
/* YYNRULES -- Number of states.  */
#define YYNSTATES  968

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   374

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   139,     2,     2,     2,   135,   133,     2,
     126,   127,   134,   138,   125,   137,   142,   136,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   124,   120,
     128,   123,   129,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   130,     2,   131,   141,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   121,   140,   122,   132,     2,     2,     2,
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
     115,   116,   117,   118,   119
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     6,     7,    11,    13,    15,    17,
      19,    21,    23,    25,    27,    29,    31,    33,    35,    37,
      40,    42,    45,    48,    51,    54,    60,    65,    66,    73,
      79,    81,    84,    88,    93,    98,   104,   105,   111,   112,
     117,   118,   122,   124,   126,   128,   129,   130,   134,   138,
     140,   142,   144,   146,   148,   150,   152,   154,   156,   158,
     160,   162,   164,   167,   170,   172,   175,   178,   181,   185,
     188,   192,   193,   195,   198,   200,   204,   206,   210,   214,
     215,   217,   218,   220,   222,   224,   226,   231,   237,   238,
     244,   247,   249,   250,   252,   254,   257,   261,   263,   264,
     269,   276,   280,   285,   288,   292,   298,   302,   304,   307,
     313,   319,   326,   332,   339,   342,   343,   347,   350,   352,
     354,   355,   356,   364,   366,   370,   372,   375,   378,   381,
     385,   389,   394,   398,   399,   405,   407,   408,   413,   414,
     415,   421,   422,   423,   429,   430,   431,   432,   440,   442,
     444,   445,   447,   448,   452,   454,   457,   460,   463,   466,
     469,   472,   476,   479,   483,   486,   490,   494,   497,   501,
     506,   509,   511,   513,   516,   518,   521,   524,   525,   526,
     534,   537,   538,   542,   543,   549,   552,   554,   557,   558,
     561,   562,   566,   568,   571,   575,   577,   578,   584,   586,
     588,   589,   590,   596,   597,   603,   604,   607,   609,   613,
     616,   617,   618,   621,   623,   624,   629,   633,   634,   635,
     641,   642,   644,   645,   649,   654,   657,   658,   661,   662,
     663,   668,   671,   672,   674,   677,   678,   684,   687,   688,
     694,   696,   698,   700,   702,   704,   705,   706,   707,   714,
     716,   717,   720,   723,   727,   729,   732,   734,   737,   738,
     740,   742,   746,   748,   750,   752,   753,   755,   756,   759,
     761,   764,   765,   770,   771,   772,   775,   777,   779,   781,
     783,   786,   789,   792,   795,   798,   802,   806,   807,   813,
     815,   817,   818,   824,   826,   828,   830,   832,   834,   836,
     838,   841,   844,   847,   850,   853,   856,   859,   861,   863,
     865,   867,   869,   871,   873,   875,   877,   879,   881,   883,
     885,   887,   889,   891,   893,   895,   897,   899,   900,   903,
     905,   907,   909,   911,   913,   916,   918,   920,   922,   924,
     926,   929,   931,   933,   935,   937,   939,   941,   943,   946,
     949,   950,   954,   955,   960,   962,   963,   967,   969,   971,
     974,   977,   980,   981,   985,   986,   991,   993,   995,   997,
    1000,  1003,  1006,  1008,  1010,  1012,  1014,  1016,  1018,  1020,
    1022,  1024,  1026,  1028,  1030,  1032,  1034,  1036,  1038,  1040,
    1042,  1044,  1046,  1048,  1050,  1052,  1054,  1056,  1058,  1060,
    1062,  1064,  1066,  1068,  1070,  1072,  1074,  1076,  1079,  1081,
    1083,  1084,  1088,  1090,  1093,  1094,  1102,  1103,  1104,  1105,
    1115,  1116,  1122,  1123,  1129,  1130,  1131,  1142,  1143,  1151,
    1152,  1153,  1154,  1164,  1171,  1172,  1180,  1181,  1189,  1190,
    1198,  1199,  1207,  1208,  1216,  1217,  1225,  1226,  1234,  1235,
    1243,  1244,  1254,  1255,  1265,  1270,  1275,  1283,  1284,  1286,
    1289,  1292,  1296,  1300,  1302,  1304,  1306,  1308,  1310,  1312,
    1314,  1316,  1318,  1320,  1322,  1324,  1326,  1328,  1330,  1332,
    1334,  1336,  1338,  1340,  1342,  1344,  1346,  1348,  1350,  1352,
    1354,  1356,  1358,  1360,  1362,  1364,  1366,  1368,  1370,  1372,
    1374,  1376,  1378,  1380,  1382,  1384,  1386,  1388,  1390,  1392,
    1394,  1396,  1398,  1400,  1402,  1404,  1406,  1408,  1410,  1412,
    1414,  1416,  1418,  1420,  1422,  1424,  1426,  1428,  1430,  1432,
    1434,  1436,  1438,  1440,  1443,  1445,  1447,  1449,  1451,  1453,
    1455,  1457,  1459,  1461,  1463,  1465,  1466,  1469,  1471,  1473,
    1475,  1477,  1479,  1481,  1483,  1485,  1486,  1489,  1490,  1493,
    1495,  1497,  1499,  1501,  1503,  1504,  1509,  1510,  1515,  1516,
    1521,  1522,  1527,  1528,  1533,  1534,  1539,  1540,  1543,  1544,
    1547,  1549,  1551,  1553,  1555,  1557,  1559,  1561,  1563,  1565,
    1567,  1569,  1571,  1573,  1575,  1577,  1579,  1581,  1583,  1587,
    1591,  1595,  1597,  1599
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     144,     0,    -1,   145,    -1,    -1,    -1,   145,   146,   147,
      -1,   198,    -1,   196,    -1,   153,    -1,   150,    -1,   152,
      -1,   149,    -1,   186,    -1,   259,    -1,   175,    -1,   155,
      -1,   216,    -1,   148,    -1,   327,    -1,   289,   120,    -1,
     120,    -1,   199,   155,    -1,   199,   216,    -1,   199,   183,
      -1,   199,   148,    -1,    34,    12,   121,   145,   122,    -1,
      40,   121,   375,   122,    -1,    -1,    40,   299,   151,   121,
     145,   122,    -1,    40,   299,   123,   291,   120,    -1,   154,
      -1,   199,   154,    -1,   161,   289,   120,    -1,   302,   161,
     289,   120,    -1,   156,   300,   261,   120,    -1,   302,   156,
     300,   261,   120,    -1,    -1,   158,   157,   121,   162,   122,
      -1,    -1,   161,   289,   159,   168,    -1,    -1,   161,   160,
     168,    -1,    20,    -1,    19,    -1,    21,    -1,    -1,    -1,
     162,   163,   165,    -1,   162,   164,   124,    -1,    23,    -1,
      24,    -1,    25,    -1,   196,    -1,   153,    -1,   167,    -1,
     186,    -1,   259,    -1,   175,    -1,   155,    -1,   220,    -1,
     166,    -1,   327,    -1,   119,   381,    -1,   289,   120,    -1,
     120,    -1,   199,   155,    -1,   199,   220,    -1,    33,   184,
      -1,    33,   199,   184,    -1,    33,   153,    -1,    33,   221,
     237,    -1,    -1,   169,    -1,   124,   170,    -1,   171,    -1,
     170,   125,   171,    -1,   289,    -1,    30,   173,   289,    -1,
     174,   172,   289,    -1,    -1,    30,    -1,    -1,   174,    -1,
      23,    -1,    24,    -1,    25,    -1,   176,   300,   261,   120,
      -1,   302,   176,   300,   261,   120,    -1,    -1,   178,   121,
     177,   179,   122,    -1,    22,   289,    -1,    22,    -1,    -1,
     180,    -1,   181,    -1,   180,   125,    -1,   180,   125,   181,
      -1,   298,    -1,    -1,   298,   123,   182,   355,    -1,   308,
     292,   298,   123,   376,   120,    -1,   161,   289,   185,    -1,
     302,   161,   289,   185,    -1,   161,   185,    -1,   302,   161,
     185,    -1,   121,   375,   122,   376,   120,    -1,   124,   376,
     120,    -1,   187,    -1,   302,   187,    -1,    39,   308,   195,
     189,   120,    -1,    39,   156,   300,   188,   120,    -1,    39,
     302,   156,   300,   188,   120,    -1,    39,   176,   300,   188,
     120,    -1,    39,   302,   176,   300,   188,   120,    -1,   190,
     189,    -1,    -1,   189,   125,   190,    -1,   265,   195,    -1,
     268,    -1,   192,    -1,    -1,    -1,   279,   126,   193,   249,
     127,   194,   275,    -1,   191,    -1,    41,   197,   120,    -1,
     289,    -1,    38,   289,    -1,   292,   231,    -1,   292,   226,
      -1,   295,   292,   231,    -1,   295,   292,   226,    -1,    41,
      40,   289,   120,    -1,    36,   128,   129,    -1,    -1,    36,
     128,   200,   201,   129,    -1,   203,    -1,    -1,   201,   125,
     202,   203,    -1,    -1,    -1,   204,   314,   266,   205,   212,
      -1,    -1,    -1,   206,   211,   266,   207,   212,    -1,    -1,
      -1,    -1,   208,   199,    20,   209,   266,   210,   212,    -1,
      20,    -1,    38,    -1,    -1,   213,    -1,    -1,   123,   214,
     215,    -1,   363,    -1,   215,   363,    -1,   217,   237,    -1,
     222,   237,    -1,   218,   237,    -1,   219,   237,    -1,   308,
     233,    -1,   308,   292,   233,    -1,   292,   241,    -1,   302,
     292,   241,    -1,   292,   223,    -1,   302,   292,   223,    -1,
     308,   292,   227,    -1,   221,   237,    -1,   292,   231,   120,
      -1,   302,   292,   231,   120,    -1,   308,   233,    -1,   222,
      -1,   241,    -1,   302,   241,    -1,   223,    -1,   302,   223,
      -1,   308,   227,    -1,    -1,    -1,   226,   126,   224,   249,
     127,   225,   234,    -1,   232,   308,    -1,    -1,   229,   228,
     234,    -1,    -1,   231,   126,   230,   249,   127,    -1,   232,
     351,    -1,    35,    -1,   238,   234,    -1,    -1,   234,   235,
      -1,    -1,    37,   236,   369,    -1,    26,    -1,   123,    18,
      -1,   121,   375,   122,    -1,   120,    -1,    -1,   240,   126,
     239,   249,   127,    -1,   298,    -1,   296,    -1,    -1,    -1,
     244,   242,   246,   243,   234,    -1,    -1,   240,   126,   245,
     249,   127,    -1,    -1,   124,   247,    -1,   248,    -1,   247,
     125,   248,    -1,   289,   381,    -1,    -1,    -1,   250,   251,
      -1,   253,    -1,    -1,   251,   125,   252,   253,    -1,   251,
     125,    71,    -1,    -1,    -1,   254,   308,   266,   255,   256,
      -1,    -1,   257,    -1,    -1,   123,   258,   355,    -1,   308,
     260,   262,   120,    -1,   268,   256,    -1,    -1,   264,   262,
      -1,    -1,    -1,   262,   125,   263,   264,    -1,   265,   260,
      -1,    -1,   322,    -1,   278,   281,    -1,    -1,   270,   276,
     127,   267,   272,    -1,   279,   281,    -1,    -1,   271,   277,
     127,   269,   272,    -1,   126,    -1,    10,    -1,    11,    -1,
      10,    -1,    11,    -1,    -1,    -1,    -1,   126,   273,   249,
     127,   274,   275,    -1,   282,    -1,    -1,   275,    28,    -1,
     275,    26,    -1,   275,    37,   381,    -1,   266,    -1,   322,
     266,    -1,   268,    -1,   322,   268,    -1,    -1,   279,    -1,
     298,    -1,   298,   124,   280,    -1,    15,    -1,    13,    -1,
      14,    -1,    -1,   282,    -1,    -1,   283,   284,    -1,   285,
      -1,   284,   285,    -1,    -1,   130,   286,   287,   131,    -1,
      -1,    -1,   288,   355,    -1,   290,    -1,   291,    -1,   298,
      -1,   296,    -1,   292,   290,    -1,   295,   290,    -1,   295,
     291,    -1,   294,   295,    -1,   296,   295,    -1,   292,   294,
     295,    -1,   292,   296,   295,    -1,    -1,   292,    36,   293,
     296,   295,    -1,   299,    -1,    72,    -1,    -1,   299,   128,
     297,   361,   129,    -1,     4,    -1,     5,    -1,     3,    -1,
       9,    -1,     8,    -1,     6,    -1,     7,    -1,   132,     4,
      -1,   132,     5,    -1,   132,     3,    -1,   132,     9,    -1,
     132,     8,    -1,   132,     6,    -1,   132,     7,    -1,    86,
      -1,    85,    -1,    88,    -1,    89,    -1,    90,    -1,    91,
      -1,    92,    -1,    93,    -1,    94,    -1,    95,    -1,    96,
      -1,    97,    -1,    87,    -1,     3,    -1,     5,    -1,     4,
      -1,     9,    -1,     8,    -1,     6,    -1,     7,    -1,    -1,
     300,   301,    -1,   303,    -1,   321,    -1,    39,    -1,    33,
      -1,   303,    -1,   302,   303,    -1,   304,    -1,   305,    -1,
     306,    -1,    28,    -1,    34,    -1,    34,    12,    -1,    29,
      -1,    32,    -1,    30,    -1,    31,    -1,    26,    -1,    27,
      -1,   306,    -1,   307,   306,    -1,   309,   265,    -1,    -1,
     312,   310,   300,    -1,    -1,   302,   312,   311,   300,    -1,
     319,    -1,    -1,    38,   313,   289,    -1,   296,    -1,   291,
      -1,   161,   289,    -1,    22,   289,    -1,   315,   265,    -1,
      -1,   318,   316,   300,    -1,    -1,   302,   312,   317,   300,
      -1,   319,    -1,   296,    -1,   291,    -1,    19,   289,    -1,
      21,   289,    -1,    22,   289,    -1,   321,    -1,   320,    -1,
       6,    -1,     7,    -1,     8,    -1,     9,    -1,     3,    -1,
       4,    -1,     5,    -1,    85,    -1,    86,    -1,    88,    -1,
      89,    -1,    90,    -1,    91,    -1,    92,    -1,    93,    -1,
      94,    -1,    95,    -1,    96,    -1,    97,    -1,    87,    -1,
      74,    -1,    75,    -1,    76,    -1,    77,    -1,    82,    -1,
      78,    -1,    79,    -1,    80,    -1,    81,    -1,    83,    -1,
      84,    -1,   323,    -1,   326,    -1,   326,   323,    -1,   133,
      -1,   134,    -1,    -1,   134,   325,   307,    -1,   324,    -1,
     326,   324,    -1,    -1,    98,   126,   298,   125,   328,   308,
     127,    -1,    -1,    -1,    -1,    99,   126,   329,   298,   125,
     330,   308,   331,   127,    -1,    -1,   100,   126,   332,   298,
     127,    -1,    -1,   101,   126,   333,   298,   127,    -1,    -1,
      -1,   102,   126,   298,   125,   334,   308,   335,   125,   376,
     127,    -1,    -1,   103,   126,   298,   125,   336,   308,   127,
      -1,    -1,    -1,    -1,   104,   126,   337,   298,   125,   338,
     308,   339,   127,    -1,   105,   126,   298,   125,   308,   127,
      -1,    -1,   106,   126,   298,   125,   340,   308,   127,    -1,
      -1,   110,   126,   298,   125,   341,   308,   127,    -1,    -1,
     107,   126,   298,   125,   342,   308,   127,    -1,    -1,   111,
     126,   298,   125,   343,   308,   127,    -1,    -1,   108,   126,
     298,   125,   344,   308,   127,    -1,    -1,   112,   126,   298,
     125,   345,   308,   127,    -1,    -1,   109,   126,   298,   125,
     346,   308,   127,    -1,    -1,   113,   126,   298,   125,   347,
     308,   127,    -1,    -1,   114,   126,   298,   125,   348,   308,
     125,    13,   127,    -1,    -1,   115,   126,   298,   125,   349,
     308,   125,    13,   127,    -1,   116,   126,   298,   127,    -1,
     117,   126,   298,   127,    -1,   118,   126,   298,   125,   298,
     350,   127,    -1,    -1,   125,    -1,   126,   127,    -1,   130,
     131,    -1,    42,   130,   131,    -1,    43,   130,   131,    -1,
     128,    -1,   129,    -1,   125,    -1,   123,    -1,   352,    -1,
     135,    -1,   134,    -1,   136,    -1,   137,    -1,   138,    -1,
     139,    -1,   132,    -1,   133,    -1,   140,    -1,   141,    -1,
      42,    -1,    43,    -1,    48,    -1,    49,    -1,    50,    -1,
      51,    -1,    52,    -1,    53,    -1,    54,    -1,    57,    -1,
      58,    -1,    59,    -1,    60,    -1,    61,    -1,    55,    -1,
      56,    -1,    62,    -1,    63,    -1,    64,    -1,    65,    -1,
      66,    -1,    67,    -1,    68,    -1,    69,    -1,    70,    -1,
      39,    -1,    38,    -1,    20,    -1,    19,    -1,    21,    -1,
      36,    -1,    23,    -1,    25,    -1,    24,    -1,    26,    -1,
      29,    -1,    32,    -1,    30,    -1,    34,    -1,    40,    -1,
      35,    -1,    22,    -1,    37,    -1,    46,    -1,    45,    -1,
      44,    -1,    47,    -1,    15,    -1,    13,    -1,    14,    -1,
      16,    -1,    17,    -1,    12,    -1,    18,    -1,   358,    -1,
     355,   358,    -1,   367,    -1,   369,    -1,   373,    -1,   352,
      -1,   124,    -1,   142,    -1,    72,    -1,   353,    -1,   354,
      -1,   321,    -1,   320,    -1,    -1,   357,   359,    -1,   356,
      -1,   128,    -1,   129,    -1,   358,    -1,   123,    -1,   125,
      -1,   359,    -1,   120,    -1,    -1,   361,   364,    -1,    -1,
     362,   360,    -1,   365,    -1,   356,    -1,   363,    -1,   123,
      -1,   125,    -1,    -1,   128,   366,   361,   129,    -1,    -1,
     130,   368,   357,   131,    -1,    -1,   126,   370,   357,   127,
      -1,    -1,    10,   371,   357,   127,    -1,    -1,    11,   372,
     357,   127,    -1,    -1,   121,   374,   362,   122,    -1,    -1,
     375,   377,    -1,    -1,   376,   378,    -1,   378,    -1,   120,
      -1,   379,    -1,   381,    -1,   380,    -1,    72,    -1,    71,
      -1,   352,    -1,   124,    -1,   142,    -1,   128,    -1,   129,
      -1,   123,    -1,   125,    -1,   353,    -1,   354,    -1,   319,
      -1,    73,    -1,   121,   375,   122,    -1,   130,   375,   131,
      -1,   382,   375,   127,    -1,   126,    -1,    10,    -1,    11,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,  1460,  1460,  1462,  1464,  1463,  1474,  1475,  1476,  1477,
    1478,  1479,  1480,  1481,  1482,  1483,  1484,  1485,  1486,  1487,
    1488,  1491,  1492,  1493,  1494,  1501,  1508,  1509,  1509,  1513,
    1520,  1521,  1524,  1525,  1528,  1529,  1532,  1532,  1546,  1546,
    1548,  1548,  1552,  1553,  1554,  1556,  1558,  1557,  1566,  1570,
    1571,  1572,  1575,  1576,  1577,  1578,  1579,  1580,  1581,  1582,
    1583,  1584,  1585,  1586,  1587,  1590,  1591,  1594,  1595,  1596,
    1597,  1599,  1600,  1603,  1606,  1607,  1610,  1612,  1614,  1618,
    1619,  1622,  1623,  1626,  1627,  1628,  1639,  1640,  1644,  1644,
    1657,  1658,  1660,  1661,  1664,  1665,  1666,  1669,  1670,  1670,
    1678,  1681,  1682,  1683,  1684,  1687,  1688,  1696,  1697,  1700,
    1701,  1703,  1705,  1707,  1711,  1713,  1714,  1717,  1720,  1721,
    1724,  1725,  1724,  1729,  1763,  1766,  1767,  1768,  1770,  1772,
    1774,  1778,  1785,  1788,  1787,  1805,  1807,  1806,  1811,  1813,
    1811,  1815,  1817,  1815,  1819,  1820,  1822,  1819,  1833,  1834,
    1836,  1837,  1840,  1840,  1850,  1851,  1859,  1860,  1861,  1862,
    1865,  1868,  1869,  1870,  1873,  1874,  1875,  1878,  1879,  1880,
    1883,  1884,  1885,  1886,  1889,  1890,  1891,  1895,  1899,  1894,
    1911,  1915,  1915,  1926,  1925,  1934,  1938,  1941,  1950,  1951,
    1954,  1954,  1955,  1956,  1964,  1965,  1969,  1968,  1976,  1977,
    1985,  1986,  1985,  2004,  2004,  2007,  2008,  2011,  2012,  2015,
    2021,  2022,  2022,  2025,  2026,  2026,  2028,  2032,  2034,  2032,
    2058,  2059,  2062,  2062,  2070,  2073,  2132,  2133,  2135,  2136,
    2136,  2139,  2142,  2143,  2147,  2148,  2148,  2167,  2168,  2168,
    2186,  2187,  2189,  2193,  2195,  2198,  2199,  2200,  2199,  2205,
    2207,  2208,  2209,  2210,  2213,  2214,  2218,  2219,  2223,  2224,
    2227,  2228,  2231,  2232,  2233,  2236,  2237,  2240,  2240,  2243,
    2244,  2247,  2247,  2250,  2251,  2251,  2258,  2259,  2262,  2263,
    2266,  2268,  2270,  2274,  2276,  2278,  2280,  2282,  2282,  2287,
    2290,  2293,  2293,  2308,  2309,  2310,  2311,  2312,  2313,  2314,
    2315,  2316,  2317,  2318,  2319,  2320,  2321,  2322,  2323,  2324,
    2325,  2326,  2327,  2328,  2329,  2330,  2331,  2332,  2333,  2334,
    2341,  2342,  2343,  2344,  2345,  2346,  2347,  2354,  2355,  2358,
    2359,  2361,  2362,  2365,  2366,  2369,  2370,  2371,  2374,  2375,
    2376,  2377,  2380,  2381,  2382,  2385,  2386,  2389,  2390,  2399,
    2402,  2402,  2404,  2404,  2408,  2409,  2409,  2411,  2413,  2415,
    2417,  2421,  2424,  2424,  2426,  2426,  2430,  2431,  2433,  2435,
    2437,  2439,  2443,  2444,  2447,  2448,  2449,  2450,  2451,  2452,
    2453,  2454,  2455,  2456,  2457,  2458,  2459,  2460,  2461,  2462,
    2463,  2464,  2465,  2466,  2469,  2470,  2471,  2472,  2473,  2474,
    2475,  2476,  2477,  2478,  2479,  2499,  2500,  2501,  2504,  2507,
    2508,  2508,  2523,  2524,  2541,  2541,  2551,  2552,  2552,  2551,
    2561,  2561,  2571,  2571,  2580,  2580,  2580,  2613,  2612,  2623,
    2624,  2624,  2623,  2633,  2651,  2651,  2656,  2656,  2661,  2661,
    2666,  2666,  2671,  2671,  2676,  2676,  2681,  2681,  2686,  2686,
    2691,  2691,  2708,  2708,  2722,  2759,  2797,  2834,  2835,  2842,
    2843,  2844,  2845,  2846,  2847,  2848,  2849,  2850,  2853,  2854,
    2855,  2856,  2857,  2858,  2859,  2860,  2861,  2862,  2863,  2864,
    2865,  2866,  2867,  2868,  2869,  2870,  2871,  2872,  2873,  2874,
    2875,  2876,  2877,  2878,  2879,  2880,  2881,  2882,  2883,  2884,
    2885,  2886,  2887,  2890,  2891,  2892,  2893,  2894,  2895,  2896,
    2897,  2898,  2899,  2900,  2901,  2902,  2903,  2904,  2905,  2906,
    2907,  2908,  2909,  2910,  2911,  2914,  2915,  2916,  2917,  2918,
    2919,  2920,  2927,  2928,  2931,  2932,  2933,  2934,  2965,  2965,
    2966,  2967,  2968,  2969,  2970,  2993,  2994,  2996,  2997,  2998,
    3000,  3001,  3002,  3004,  3005,  3007,  3008,  3010,  3011,  3014,
    3015,  3018,  3019,  3020,  3024,  3023,  3037,  3037,  3041,  3041,
    3043,  3043,  3045,  3045,  3049,  3049,  3054,  3055,  3057,  3058,
    3061,  3062,  3065,  3066,  3067,  3068,  3069,  3070,  3071,  3071,
    3071,  3071,  3071,  3071,  3072,  3072,  3073,  3074,  3077,  3080,
    3083,  3086,  3086,  3086
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
  "UNSIGNED", "SSIZE_T", "SIZE_T", "IdType", "TypeInt8", "TypeUInt8",
  "TypeInt16", "TypeUInt16", "TypeInt32", "TypeUInt32", "TypeInt64",
  "TypeUInt64", "TypeFloat32", "TypeFloat64", "SetMacro", "GetMacro",
  "SetStringMacro", "GetStringMacro", "SetClampMacro", "SetObjectMacro",
  "GetObjectMacro", "BooleanMacro", "SetVector2Macro", "SetVector3Macro",
  "SetVector4Macro", "SetVector6Macro", "GetVector2Macro",
  "GetVector3Macro", "GetVector4Macro", "GetVector6Macro",
  "SetVectorMacro", "GetVectorMacro", "ViewportCoordinateMacro",
  "WorldCoordinateMacro", "TypeMacro", "VTK_BYTE_SWAP_DECL", "';'", "'{'",
  "'}'", "'='", "':'", "','", "'('", "')'", "'<'", "'>'", "'['", "']'",
  "'~'", "'&'", "'*'", "'%'", "'/'", "'-'", "'+'", "'!'", "'|'", "'^'",
  "'.'", "$accept", "translation_unit", "opt_declaration_seq", "$@1",
  "declaration", "template_declaration", "linkage_specification",
  "namespace_definition", "$@2", "namespace_alias_definition",
  "forward_declaration", "simple_forward_declaration", "class_definition",
  "class_specifier", "$@3", "class_head", "$@4", "$@5", "class_key",
  "member_specification", "$@6", "member_access_specifier",
  "member_declaration", "template_member_declaration",
  "friend_declaration", "opt_base_clause", "base_clause",
  "base_specifier_list", "base_specifier", "opt_virtual",
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
  "$@14", "$@15", "$@16", "$@17", "$@18", "$@19", "class_or_typename",
  "opt_template_parameter_initializer", "template_parameter_initializer",
  "$@20", "template_parameter_value", "function_definition",
  "function_declaration", "nested_method_declaration",
  "nested_operator_declaration", "method_definition", "method_declaration",
  "operator_declaration", "conversion_function", "$@21", "$@22",
  "conversion_function_id", "operator_function_nr", "$@23",
  "operator_function_sig", "$@24", "operator_function_id", "operator_sig",
  "function_nr", "function_trailer_clause", "function_trailer", "$@25",
  "function_body", "function_sig", "$@26", "function_name",
  "structor_declaration", "$@27", "$@28", "structor_sig", "$@29",
  "opt_ctor_initializer", "mem_initializer_list", "mem_initializer",
  "parameter_declaration_clause", "$@30", "parameter_list", "$@31",
  "parameter_declaration", "$@32", "$@33", "opt_initializer",
  "initializer", "$@34", "variable_declaration", "init_declarator_id",
  "opt_declarator_list", "declarator_list_cont", "$@35", "init_declarator",
  "opt_ptr_operator_seq", "direct_abstract_declarator", "$@36",
  "direct_declarator", "$@37", "p_or_lp_or_la", "lp_or_la",
  "opt_array_or_parameters", "$@38", "$@39", "function_qualifiers",
  "abstract_declarator", "declarator", "opt_declarator_id",
  "declarator_id", "bitfield_size", "opt_array_decorator_seq",
  "array_decorator_seq", "$@40", "array_decorator_seq_impl",
  "array_decorator", "$@41", "array_size_specifier", "$@42",
  "id_expression", "unqualified_id", "qualified_id",
  "nested_name_specifier", "$@43", "identifier_sig", "scope_operator_sig",
  "template_id", "$@44", "simple_id", "identifier",
  "opt_decl_specifier_seq", "decl_specifier2", "decl_specifier_seq",
  "decl_specifier", "storage_class_specifier", "function_specifier",
  "cv_qualifier", "cv_qualifier_seq", "store_type", "store_type_specifier",
  "$@45", "$@46", "type_specifier", "$@47", "tparam_type",
  "tparam_type_specifier2", "$@48", "$@49", "tparam_type_specifier",
  "simple_type_specifier", "type_name", "primitive_type",
  "ptr_operator_seq", "reference", "pointer", "$@50", "pointer_seq",
  "declaration_macro", "$@51", "$@52", "$@53", "$@54", "$@55", "$@56",
  "$@57", "$@58", "$@59", "$@60", "$@61", "$@62", "$@63", "$@64", "$@65",
  "$@66", "$@67", "$@68", "$@69", "$@70", "$@71", "$@72", "opt_comma",
  "operator_id", "operator_id_no_delim", "keyword", "literal",
  "constant_expression", "common_bracket_item", "any_bracket_contents",
  "bracket_pitem", "any_bracket_item", "braces_item",
  "angle_bracket_contents", "braces_contents", "angle_bracket_pitem",
  "angle_bracket_item", "angle_brackets_sig", "$@73", "brackets_sig",
  "$@74", "parentheses_sig", "$@75", "$@76", "$@77", "braces_sig", "$@78",
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
      59,   123,   125,    61,    58,    44,    40,    41,    60,    62,
      91,    93,   126,    38,    42,    37,    47,    45,    43,    33,
     124,    94,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   143,   144,   145,   146,   145,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   147,   147,   147,   147,
     147,   148,   148,   148,   148,   149,   150,   151,   150,   152,
     153,   153,   154,   154,   155,   155,   157,   156,   159,   158,
     160,   158,   161,   161,   161,   162,   163,   162,   162,   164,
     164,   164,   165,   165,   165,   165,   165,   165,   165,   165,
     165,   165,   165,   165,   165,   166,   166,   167,   167,   167,
     167,   168,   168,   169,   170,   170,   171,   171,   171,   172,
     172,   173,   173,   174,   174,   174,   175,   175,   177,   176,
     178,   178,   179,   179,   180,   180,   180,   181,   182,   181,
     183,   184,   184,   184,   184,   185,   185,   186,   186,   187,
     187,   187,   187,   187,   188,   189,   189,   190,   191,   191,
     193,   194,   192,   195,   196,   197,   197,   197,   197,   197,
     197,   198,   199,   200,   199,   201,   202,   201,   204,   205,
     203,   206,   207,   203,   208,   209,   210,   203,   211,   211,
     212,   212,   214,   213,   215,   215,   216,   216,   216,   216,
     217,   218,   218,   218,   219,   219,   219,   220,   220,   220,
     221,   221,   221,   221,   222,   222,   222,   224,   225,   223,
     226,   228,   227,   230,   229,   231,   232,   233,   234,   234,
     236,   235,   235,   235,   237,   237,   239,   238,   240,   240,
     242,   243,   241,   245,   244,   246,   246,   247,   247,   248,
     249,   250,   249,   251,   252,   251,   251,   254,   255,   253,
     256,   256,   258,   257,   259,   260,   261,   261,   262,   263,
     262,   264,   265,   265,   266,   267,   266,   268,   269,   268,
     270,   270,   270,   271,   271,   272,   273,   274,   272,   272,
     275,   275,   275,   275,   276,   276,   277,   277,   278,   278,
     279,   279,   280,   280,   280,   281,   281,   283,   282,   284,
     284,   286,   285,   287,   288,   287,   289,   289,   290,   290,
     291,   291,   291,   292,   292,   292,   292,   293,   292,   294,
     295,   297,   296,   298,   298,   298,   298,   298,   298,   298,
     298,   298,   298,   298,   298,   298,   298,   298,   298,   298,
     298,   298,   298,   298,   298,   298,   298,   298,   298,   298,
     299,   299,   299,   299,   299,   299,   299,   300,   300,   301,
     301,   301,   301,   302,   302,   303,   303,   303,   304,   304,
     304,   304,   305,   305,   305,   306,   306,   307,   307,   308,
     310,   309,   311,   309,   312,   313,   312,   312,   312,   312,
     312,   314,   316,   315,   317,   315,   318,   318,   318,   318,
     318,   318,   319,   319,   320,   320,   320,   320,   320,   320,
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
       1,     2,     2,     2,     2,     5,     4,     0,     6,     5,
       1,     2,     3,     4,     4,     5,     0,     5,     0,     4,
       0,     3,     1,     1,     1,     0,     0,     3,     3,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     2,     1,     2,     2,     2,     3,     2,
       3,     0,     1,     2,     1,     3,     1,     3,     3,     0,
       1,     0,     1,     1,     1,     1,     4,     5,     0,     5,
       2,     1,     0,     1,     1,     2,     3,     1,     0,     4,
       6,     3,     4,     2,     3,     5,     3,     1,     2,     5,
       5,     6,     5,     6,     2,     0,     3,     2,     1,     1,
       0,     0,     7,     1,     3,     1,     2,     2,     2,     3,
       3,     4,     3,     0,     5,     1,     0,     4,     0,     0,
       5,     0,     0,     5,     0,     0,     0,     7,     1,     1,
       0,     1,     0,     3,     1,     2,     2,     2,     2,     2,
       2,     3,     2,     3,     2,     3,     3,     2,     3,     4,
       2,     1,     1,     2,     1,     2,     2,     0,     0,     7,
       2,     0,     3,     0,     5,     2,     1,     2,     0,     2,
       0,     3,     1,     2,     3,     1,     0,     5,     1,     1,
       0,     0,     5,     0,     5,     0,     2,     1,     3,     2,
       0,     0,     2,     1,     0,     4,     3,     0,     0,     5,
       0,     1,     0,     3,     4,     2,     0,     2,     0,     0,
       4,     2,     0,     1,     2,     0,     5,     2,     0,     5,
       1,     1,     1,     1,     1,     0,     0,     0,     6,     1,
       0,     2,     2,     3,     1,     2,     1,     2,     0,     1,
       1,     3,     1,     1,     1,     0,     1,     0,     2,     1,
       2,     0,     4,     0,     0,     2,     1,     1,     1,     1,
       2,     2,     2,     2,     2,     3,     3,     0,     5,     1,
       1,     0,     5,     1,     1,     1,     1,     1,     1,     1,
       2,     2,     2,     2,     2,     2,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     0,     2,     1,
       1,     1,     1,     1,     2,     1,     1,     1,     1,     1,
       2,     1,     1,     1,     1,     1,     1,     1,     2,     2,
       0,     3,     0,     4,     1,     0,     3,     1,     1,     2,
       2,     2,     0,     3,     0,     4,     1,     1,     1,     2,
       2,     2,     1,     1,     1,     1,     1,     1,     1,     1,
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
       3,     0,     4,     1,     0,   378,   379,   380,   374,   375,
     376,   377,    43,    42,    44,    91,   345,   346,   338,   341,
     343,   344,   342,   339,   186,     0,   355,     0,     0,     0,
     290,   394,   395,   396,   397,   399,   400,   401,   402,   398,
     403,   404,   381,   382,   393,   383,   384,   385,   386,   387,
     388,   389,   390,   391,   392,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    20,     0,     5,    17,
      11,     9,    10,     8,    30,    15,   327,    36,    40,    14,
     327,     0,    12,   107,     7,     6,     0,    16,     0,     0,
       0,     0,   174,     0,     0,    13,     0,   276,   358,     0,
       0,     0,   357,   278,   289,     0,   333,   335,   336,   337,
       0,   232,   350,   354,   373,   372,    18,   295,   293,   294,
     298,   299,   297,   296,   308,   307,   319,   309,   310,   311,
     312,   313,   314,   315,   316,   317,   318,   360,   277,     0,
     279,   340,   133,     0,   378,   379,   380,   374,   375,   376,
     377,   339,   381,   382,   393,   383,   384,   385,   386,   387,
     388,   389,   390,   391,   392,   327,    40,   327,   358,   357,
       0,     0,   320,   322,   321,   325,   326,   324,   323,   576,
      27,     0,     0,     0,   125,     0,     0,     0,   416,   420,
     422,     0,     0,   429,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   302,   300,
     301,   305,   306,   304,   303,   232,     0,    71,   359,   232,
      88,     0,    24,    31,    21,    23,     0,    22,     0,     0,
     195,   576,   156,   158,   159,   157,   177,     0,     0,   180,
      19,   287,   164,     0,   162,   200,   280,     0,   279,   278,
     283,   281,   282,   284,   291,   327,    40,   327,   108,   175,
       0,   334,   352,   243,   244,   176,   181,     0,     0,   160,
     188,     0,   228,   220,     0,   265,     0,   199,   260,   408,
     409,   349,   233,   405,   412,   406,   327,   279,     3,   132,
     138,   356,   340,   232,   359,   232,   327,   327,   295,   293,
     294,   298,   299,   297,   296,   123,   119,   115,   118,   265,
     260,     0,     0,     0,   126,     0,   124,   128,   127,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   332,   331,     0,   228,     0,   328,   329,   330,
      45,     0,    41,    72,    32,    71,     0,    92,   360,     0,
       0,   198,     0,   211,   359,     0,   203,   205,   285,   286,
     555,   232,   359,   232,   165,   163,   327,   188,   183,   478,
     479,   480,   481,   482,   483,   484,   485,   486,   492,   493,
     487,   488,   489,   490,   491,   494,   495,   496,   497,   498,
     499,   500,   501,   502,   466,   465,     0,   463,   464,     0,
     474,   475,   469,   468,   470,   471,   472,   473,   476,   477,
     185,   467,   187,   196,     0,   222,   225,   221,   256,     0,
       0,   237,   266,     0,   166,   161,   199,     0,     0,   407,
     413,   351,     4,     0,   135,     0,     0,     0,     0,   115,
       0,     0,   232,   232,     0,   120,   378,   379,   380,   374,
     375,   376,   377,   602,   603,   530,   526,   527,   525,   528,
     529,   531,   506,   505,   507,   519,   509,   511,   510,   512,
     513,   515,   514,   516,   518,   508,   520,   504,   503,   517,
     478,   479,   523,   522,   521,   524,   586,   585,   597,   581,
     576,    26,   592,   588,   593,   601,   590,   591,   576,   589,
     596,   587,   594,   595,   577,   580,   582,   584,   583,   576,
       0,     0,     3,   131,   130,   129,   414,     0,     0,     0,
     424,   427,     0,     0,   434,   438,   442,   446,   436,   440,
     444,   448,   450,   452,   454,   455,     0,    34,   227,   231,
      46,    83,    84,    85,    81,    73,    74,    79,    76,    39,
      86,     0,    93,    94,    97,   198,   194,     0,   217,     0,
       0,   211,     0,   201,     0,     0,    33,     0,   353,   182,
     211,     0,     0,   459,   460,   192,   190,     0,   189,   211,
     224,   229,     0,   238,   257,   271,   268,   269,   263,   264,
     262,   261,   347,   411,    25,   136,   134,     0,     0,     0,
     368,   367,     0,   258,   232,   362,   366,   148,   149,   258,
       0,   110,   114,   117,   112,     0,     0,   109,   232,   211,
       0,     0,     0,    29,     4,     0,   417,   421,   423,     0,
       0,   430,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   457,    49,    50,    51,    37,     0,     0,
       0,    82,     0,    80,     0,    89,    95,    98,   578,   178,
     212,   213,     0,   288,     0,   206,   207,     0,   188,   570,
     572,   540,   574,   562,   538,   563,   568,   564,   292,   566,
     539,   544,   543,   537,   541,   542,   560,   561,   556,   559,
     534,   535,   536,    35,    87,     0,   461,   462,     0,   193,
       0,   232,   548,   549,   223,   547,   532,   245,   274,   270,
     348,   138,   369,   370,   371,   364,   241,   242,   240,   139,
     258,   265,   259,   361,   327,   142,   145,   111,   113,   116,
       0,   598,   599,   600,    28,     0,     0,   425,     0,     0,
     433,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   458,     0,     0,     0,     0,    64,    53,    58,    47,
      60,    54,    57,    55,    52,     0,    59,     0,   171,   172,
      56,     0,     0,   357,     0,     0,    61,    48,    77,    75,
      78,    96,     0,     0,   188,   214,   258,   204,     0,   209,
     202,   545,   545,   557,   545,   555,   545,   184,   191,   197,
     230,   533,   246,   239,   249,     0,     0,   137,   327,   150,
     254,     0,   258,   234,   363,   150,   258,   121,   415,   418,
       0,   428,   431,   435,   439,   443,   447,   437,   441,   445,
     449,     0,     0,   456,    69,     0,    67,     0,     0,   357,
       0,     0,    62,    65,    66,     0,   167,    63,     0,   173,
       0,   170,   199,    99,   100,   579,   179,   216,   217,   218,
     208,     0,     0,     0,     0,     0,     0,   211,   272,   275,
     365,   152,   140,   151,   235,   255,   143,   146,   250,     0,
     578,     0,     0,     0,   576,   578,   103,   359,     0,    68,
       0,    70,     0,   168,     0,   215,   220,   551,   552,   571,
     550,   546,   573,   554,   575,   553,   558,   569,   565,   567,
       0,     0,   245,   150,   122,   419,     0,   432,   451,   453,
       0,     0,   101,     0,     0,   104,   359,   169,   219,   247,
     153,   154,   236,   147,   252,   251,     0,   426,   578,   106,
       0,   102,   250,   155,   253,     0,   248,   105
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     2,     4,    78,   232,    80,    81,   323,    82,
      83,    84,   234,    86,   226,    87,   365,   227,   247,   560,
     668,   669,   779,   780,   781,   362,   363,   565,   566,   674,
     670,   567,    89,    90,   367,    91,   571,   572,   573,   802,
     235,   856,   906,    92,    93,   458,   464,   459,   315,   316,
     639,   898,   317,    94,   193,    95,   236,   300,   453,   731,
     454,   455,   829,   456,   835,   457,   836,   933,   629,   892,
     893,   931,   950,   237,    98,    99,   100,   786,   787,   101,
     102,   373,   804,   103,   275,   387,   276,   590,   277,   104,
     279,   432,   598,   718,   242,   280,   599,   253,   789,   377,
     688,   255,   581,   583,   685,   686,   577,   578,   680,   878,
     681,   682,   916,   436,   437,   602,   105,   282,   354,   434,
     721,   355,   356,   739,   932,   283,   727,   740,   284,   823,
     887,   962,   934,   831,   439,   741,   742,   611,   441,   442,
     443,   606,   607,   728,   825,   826,   194,   107,   178,   149,
     375,   110,   111,   179,   380,   113,   114,   225,   357,   248,
     116,   117,   118,   119,   613,   239,   121,   296,   386,   122,
     153,   623,   624,   744,   828,   625,   123,   124,   125,   292,
     293,   294,   448,   295,   126,   645,   332,   756,   899,   333,
     334,   649,   840,   650,   337,   759,   901,   653,   657,   654,
     658,   655,   659,   656,   660,   661,   662,   772,   430,   703,
     704,   705,   724,   725,   881,   920,   921,   926,   584,   883,
     707,   708,   709,   815,   710,   816,   711,   814,   811,   812,
     712,   813,   321,   803,   524,   525,   526,   527,   528,   529
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -872
static const yytype_int16 yypact[] =
{
    -872,    77,    87,  -872,  4255,   167,   191,   227,   280,   300,
     301,   304,  -872,  -872,  -872,  4899,  -872,  -872,  -872,  -872,
    -872,  -872,  -872,    98,  -872,    -8,  -872,  5429,   383,  4624,
    -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,
    -872,  -872,   -38,   106,   121,   122,   133,   158,   171,   186,
     196,   203,   210,   228,   243,    -3,    36,    43,    45,    90,
     114,   130,   138,   151,   179,   183,   187,   202,   215,   233,
     235,   249,   258,   267,   269,   271,  -872,   438,  -872,  -872,
    -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  4899,  -872,
    -872,    21,  -872,  -872,  -872,  -872,  5144,  -872,   -71,   -71,
     -71,   -71,  -872,   281,  5524,  -872,    54,  -872,   284,  4742,
     350,  4899,   -25,  -872,   297,  5239,  -872,  -872,  -872,  -872,
    4672,   276,  -872,  -872,  -872,  -872,  -872,   -10,     9,    18,
      50,    52,   148,   166,  -872,  -872,  -872,  -872,  -872,  -872,
    -872,  -872,  -872,  -872,  -872,  -872,  -872,   310,  -872,  4921,
     350,   312,   321,  4899,   -10,     9,    18,    50,    52,   148,
     166,   441,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,
    -872,  -872,  -872,  -872,  -872,  -872,  4899,  -872,  -872,   350,
    5429,  4790,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,
     332,  4899,  4899,   337,  -872,  4742,  4899,  4529,  -872,  -872,
    -872,  4529,  4529,  -872,  4529,  4529,  4529,  4529,  4529,  4529,
    4529,  4529,  4529,  4529,  4529,  4529,  4529,  4529,  -872,  -872,
    -872,  -872,  -872,  -872,  -872,  1286,   343,   348,   259,  1286,
    -872,  4899,  -872,  -872,  -872,  -872,  5144,  -872,  5334,  4947,
    -872,  -872,  -872,  -872,  -872,  -872,  -872,  4899,  5524,  -872,
    -872,  -872,  -872,   360,  -872,  -872,  -872,   350,   -13,   362,
    -872,  -872,  -872,  -872,  -872,  -872,  4899,  -872,  -872,  -872,
    4742,  -872,  -872,  -872,  -872,  -872,  -872,   363,  5071,  -872,
    -872,   370,  -872,   377,  1478,   372,  4742,   350,    15,  -872,
     435,  -872,  -872,  -872,  -872,   276,  -872,   350,  -872,  -872,
      73,  -872,  -872,  5028,   177,  5028,  -872,  -872,  -872,  -872,
    -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,   107,
     379,  1212,   409,   384,  -872,   386,  -872,  -872,  -872,  1383,
    4742,   387,  4529,  4529,  4529,   388,   389,  4529,   390,   391,
     392,   393,   397,   398,   399,   401,   402,   403,   405,   404,
     406,   407,  -872,  -872,   414,  -872,  4790,  -872,  -872,  -872,
    -872,  1111,  -872,  -872,  -872,   348,   415,  4529,  -872,  5334,
    4742,  -872,  1617,   424,  -872,   471,  -872,   412,  -872,  -872,
    -872,  1286,   278,  1286,  -872,  -872,  -872,  -872,  -872,   381,
     423,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,
    -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,
    -872,  -872,  -872,  -872,  -872,  -872,   428,  -872,  -872,   411,
    -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,
    -872,  -872,    41,  -872,   212,  -872,  -872,  -872,  -872,   429,
    4790,  -872,  -872,   427,  -872,  -872,   350,   470,   440,  -872,
    -872,  4121,   436,    11,  -872,  5560,    88,   524,   442,  -872,
    4790,   443,  5028,  5028,   224,  -872,  -872,  -872,  -872,  -872,
    -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,
    -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,
    -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,
    -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,
    -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,
    -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,
     444,   350,  -872,  -872,  -872,  -872,  -872,   445,   434,   439,
    -872,  -872,   446,  5524,  -872,  -872,  -872,  -872,  -872,  -872,
    -872,  -872,  -872,  -872,  -872,  -872,  4529,  -872,   447,  -872,
      48,  -872,  -872,  -872,   485,   448,  -872,   535,  -872,  -872,
    -872,   452,   451,  -872,   456,   457,  -872,   454,  -872,   350,
     297,   424,  4899,  -872,  3577,   449,  -872,   462,  4121,    41,
     424,   437,   453,  -872,  -872,  -872,  -872,   549,  -872,   424,
    -872,  -872,  3857,  -872,  -872,  -872,   427,  -872,  -872,  -872,
    -872,  -872,  -872,   440,  -872,  -872,  -872,  4899,  4899,  4899,
    -872,   350,  5524,  4694,   276,  -872,  -872,  -872,  -872,  4694,
     563,  -872,   461,  -872,  -872,   467,   468,  -872,   276,   424,
    1757,  1897,  2037,  -872,   472,  5524,  -872,  -872,  -872,  5524,
    5524,  -872,   463,  5524,  5524,  5524,  5524,  5524,  5524,  5524,
    5524,  5524,  5524,   464,  -872,  -872,  -872,  -872,  4137,   469,
    4899,  -872,  1111,  -872,  4899,  -872,  4529,  -872,  -872,  -872,
     473,  -872,  5524,  -872,   474,   475,  -872,    58,  -872,  -872,
    -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,
    -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,
    -872,  -872,  -872,  -872,  -872,   477,  -872,  -872,    92,  -872,
     478,   276,  -872,  -872,  3857,  -872,  -872,   160,   465,  -872,
    -872,    73,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,
     969,   372,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,
     479,  -872,  -872,  -872,  -872,   480,  5524,  -872,   481,  5524,
    -872,   482,   483,   484,   489,   490,   491,   492,   496,   487,
     499,  -872,   498,  4373,  4851,    58,  -872,  -872,  -872,  -872,
    -872,  -872,  -872,  -872,  -872,  4563,  -872,   -71,  -872,  -872,
    -872,   506,  4742,   129,  4468,  4672,  -872,  -872,  -872,  -872,
    -872,  -872,  3857,  2317,  -872,   526,  4694,  -872,  4899,  -872,
      41,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,
    -872,  -872,  -872,  -872,  -872,   497,  3857,  -872,  -872,   504,
    -872,   502,  4694,  -872,  4121,   504,  4694,  -872,  -872,  -872,
     508,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,
    -872,   622,   623,  -872,  -872,  4720,  -872,   518,   -71,   -13,
    4563,  4947,  -872,  -872,  -872,  4563,  -872,  -872,   520,  -872,
    4742,  -872,  -872,  3857,  -872,  -872,    41,  -872,  -872,  -872,
    -872,  3017,  3157,  2877,  3297,  3717,  3437,   424,  -872,  3857,
    4121,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,   514,
    -872,   515,   516,   519,  -872,  -872,  -872,   328,  4720,  -872,
     518,  -872,  4720,  -872,   527,  -872,   377,  -872,  -872,  -872,
    -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,
     521,  3997,   160,   504,    93,  -872,  2457,  -872,  -872,  -872,
    2177,  2597,  -872,   328,  4720,  -872,   339,  -872,  -872,  -872,
    3997,  -872,  -872,  -872,  -872,  -872,    58,  -872,  -872,  -872,
     339,  -872,  -872,  -872,  -872,  2737,    93,  -872
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -872,  -872,  -260,  -872,  -872,   645,  -872,  -872,  -872,  -872,
    -613,   -94,     1,   -26,  -872,  -872,  -872,  -872,    16,  -872,
    -872,  -872,  -872,  -872,  -872,   285,  -872,  -872,   -20,  -872,
    -872,    89,   -17,   -21,  -872,  -872,  -872,  -872,   -22,  -872,
    -872,  -202,  -473,    -9,  -102,  -249,   199,    22,  -872,  -872,
    -872,  -872,   201,    -6,  -872,  -872,    -1,  -872,  -872,  -872,
     -66,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -770,
    -872,  -872,  -872,   668,  -872,  -872,  -872,  -111,   -97,  -576,
     -80,  -872,  -872,  -154,  -241,  -872,  -872,  -872,  -155,    35,
    -270,  -364,  -872,  -872,   -91,  -872,  -872,  -105,   -67,  -872,
    -872,  -872,  -872,  -872,  -872,  -129,  -542,  -872,  -872,  -872,
    -198,  -872,  -872,  -230,  -872,  -872,    20,   333,  -166,   338,
    -872,    -5,   -95,  -575,  -872,  -157,  -872,  -872,  -872,  -237,
    -872,  -872,  -261,  -872,  -872,  -872,   -98,  -872,   -29,  -696,
    -872,  -872,   112,  -872,  -872,  -872,    13,   -45,     3,    32,
    -872,   -63,    23,    -4,  -872,   754,   -24,     8,  -872,    10,
     -81,  -872,  -872,  -415,  -872,    49,  -872,  -872,  -872,   -19,
    -872,  -872,  -872,  -872,  -872,  -872,  -121,  -391,   194,  -272,
     425,   430,  -872,  -872,    51,  -872,  -872,  -872,  -872,  -872,
    -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,
    -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -872,  -204,
    -302,    82,  -751,  -554,  -456,  -581,  -153,  -872,   -89,  -872,
    -871,  -872,  -872,  -872,  -872,  -872,    17,  -872,  -872,  -872,
    -872,  -872,  -183,  -820,  -872,  -759,  -872,  -872,  -660,  -872
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -411
static const yytype_int16 yytable[] =
{
     112,   175,   233,    96,   190,    85,   177,   108,   243,   244,
     245,   150,   440,   268,   115,   281,   445,   106,   148,   522,
      88,   726,   285,   589,   318,   150,   291,   809,   147,   252,
     706,   824,   148,   612,   271,   269,   109,   180,   452,   684,
     328,   327,   254,   176,   875,   444,   257,    30,   715,   240,
     241,   873,   196,   120,   745,   777,   461,   720,   372,    30,
     951,   195,  -320,   366,   256,   896,   261,   595,   473,   474,
     522,   664,   665,   666,   431,   889,   181,     3,   596,   963,
     936,  -322,  -308,   319,   150,   941,   257,    -2,  -308,   265,
    -321,   148,   788,  -141,   267,  -279,   272,   750,   229,   271,
     445,   228,   689,   690,   256,   258,   238,   150,   627,  -144,
     151,  -141,    88,  -199,   262,   862,   287,   521,  -320,   954,
     152,   955,  -325,   197,  -326,   431,   628,   438,   109,   444,
     956,   266,   257,   260,   281,   263,   615,  -322,   965,   447,
     616,  -198,   230,   821,   358,   297,  -321,   270,   358,   150,
     256,   261,   286,   249,   306,   278,   148,   271,   269,   307,
     854,   272,   198,   953,   597,   830,   301,   271,   521,   199,
     667,   200,   150,   263,   250,   535,   534,   875,  -325,   148,
    -326,   281,   875,   303,   515,   305,   285,   150,   150,   304,
     384,   297,   150,   701,   148,   148,   176,   788,   730,   262,
     520,    30,   263,   385,   324,   325,   875,   257,   460,   788,
     460,   701,   265,   635,   636,   585,   201,   587,   696,   272,
    -324,   726,   358,   257,   358,   256,  -307,   150,   330,   272,
     329,   879,  -307,   465,   148,   287,   824,  -267,  -323,  -320,
     202,  -319,  -309,   150,   368,   726,   369,  -319,  -309,  -279,
     148,   520,   176,  -310,   266,  -199,   203,   895,   285,  -310,
     374,   897,   150,  -322,   204,   281,   258,   257,   109,   148,
     270,   370,   644,   381,   278,   383,  -324,   205,  -311,   382,
     378,   379,   446,   604,  -311,   256,   822,  -295,   271,   269,
    -267,  -312,   821,  -295,  -323,  -320,   964,  -312,   -38,  -321,
     358,   -38,   358,   318,   451,   206,  -313,   257,   821,   207,
     263,  -293,  -313,   208,   462,   463,  -314,  -293,   531,  -322,
     379,   278,  -314,  -315,   810,   530,   297,   640,   209,  -315,
    -316,   706,   600,   701,   626,   641,  -316,   601,   522,   522,
     522,   210,   285,   265,   637,   930,   642,  -294,  -317,   638,
     272,   580,  -325,  -294,  -317,  -321,   882,   150,   884,   211,
     886,   212,   319,  -318,   148,   329,   446,   460,   460,  -318,
     358,   579,  -326,  -324,   568,   213,  -323,   706,   249,   364,
     -38,   358,   358,   -38,   214,   176,   182,   183,   184,   185,
     186,   187,   188,   215,   588,   216,   706,   217,   586,   -38,
    -298,   270,   -38,   523,  -277,   278,  -298,   246,  -325,   289,
     290,   701,   182,   183,   184,   185,   186,   187,   188,   359,
    -299,  -297,    30,   359,  -296,   264,  -299,  -297,  -326,  -324,
    -296,   -90,  -323,   298,   942,   701,   521,   521,   521,   945,
     876,   218,   219,   220,   221,   222,   223,   224,   364,   904,
     299,   621,   905,   302,   523,   322,   630,   326,   620,   586,
     904,  -410,  -410,   905,   360,   622,    16,    17,   832,   379,
     942,   945,   361,   961,   182,   183,   184,   185,   186,   187,
     188,    30,   701,   608,   609,   610,   376,   961,  -198,   388,
     701,   701,   701,   701,   701,   701,   433,   359,   701,   359,
     435,   522,  -267,   447,   189,   532,   533,   358,   561,   562,
     563,   591,   536,   540,   541,   543,   544,   545,   546,   520,
     520,   520,   547,   548,   549,   871,   550,   551,   552,   743,
     553,   554,   556,   555,   557,   570,   582,    12,    13,    14,
     701,   271,   594,   460,    16,    17,    18,    19,    20,    21,
      22,  -210,   161,   592,   263,   593,   603,   605,   614,   701,
      25,   647,   631,   634,   643,   673,   648,   719,   716,   713,
     646,   651,   601,   672,   675,   359,   676,   359,   150,   677,
     678,   679,   714,   746,   717,   148,   638,   747,   748,   771,
     760,   871,   652,   797,   754,   687,  -273,   877,   805,   521,
     808,   807,   683,   735,   817,   819,   837,   838,   841,   843,
     844,   845,   851,   150,   150,   150,   846,   847,   848,   849,
     148,   148,   148,   850,   852,   853,   867,   891,   888,   894,
     732,   733,   734,   900,   522,   902,   903,   868,   522,   522,
     913,   935,   937,   938,   263,   359,   939,   947,   949,    79,
     569,   782,   799,   671,   801,   909,   359,   359,   632,   783,
     749,   633,   784,   522,   793,   827,   150,   785,   150,   778,
     150,   108,    97,   148,   864,   148,   858,   148,   794,   880,
     915,   791,   520,   798,    88,   568,   948,   800,   790,   559,
     281,   233,   268,   558,   755,   952,   866,   285,   757,   758,
     792,   966,   761,   762,   763,   764,   765,   766,   767,   768,
     769,   770,   833,   271,   269,   914,   820,   795,   729,   796,
     449,   940,   523,   523,   523,   450,   885,   869,     0,   257,
     925,   806,   521,     0,     0,   818,   521,   521,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   256,     0,     0,
       0,     0,   834,   358,     0,     0,   281,     0,     0,     0,
       0,   521,     0,   233,     0,     0,     0,   911,   265,   859,
     150,   580,   857,   267,     0,   272,     0,   148,   702,   271,
     269,   859,   359,   860,   271,   269,   863,     0,   297,   855,
     859,   872,     0,   869,     0,   865,   702,   196,   869,     0,
       0,    88,     0,     0,   150,   839,   195,   257,   842,   358,
     266,   148,     0,     0,     0,   520,   263,   792,     0,   520,
     520,   687,   861,     0,     0,   256,   870,   278,     0,   271,
     278,     0,     0,     0,   861,     0,   890,   580,     0,   265,
       0,   272,     0,     0,   520,     0,   272,     0,     0,     0,
       0,   150,     0,     0,     0,     0,   859,   872,   148,     0,
       0,   859,     0,   259,     0,     0,   297,   910,   907,     0,
       0,     0,     0,   908,   288,     0,   912,     0,     0,     0,
       0,   266,   263,     0,     0,   523,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   278,   870,     0,     0,
       0,     0,     0,     0,   150,   278,     0,     0,   150,     0,
       0,   148,     0,     0,     0,   148,     0,     0,   702,     0,
       0,   943,     0,     0,     0,   946,   944,     0,     0,     0,
       0,     0,     0,     0,     0,   320,     0,     0,     0,     0,
     150,     0,     0,     0,     0,     0,     0,   148,     0,     0,
       0,   331,     0,     0,     0,   335,   336,   960,   338,   339,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   308,   309,   310,   311,   312,   313,   314,   736,
     737,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   371,     0,     0,   702,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   523,     0,
     702,     0,   523,   523,   259,     0,     0,     0,   359,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   320,     0,
     371,     0,     0,     0,     0,     0,     0,   523,     0,     0,
       0,     0,     0,     0,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   702,     0,     0,
       0,     0,     0,     0,     0,   702,   702,   702,   702,   702,
     702,     0,     0,   702,   359,     0,   537,   538,   539,     0,
       0,   542,     0,     0,     0,   738,     0,     0,     0,     0,
       0,    77,   289,   290,     0,     0,     0,     0,     0,     0,
     320,     0,     0,     0,   127,   128,   129,   130,   131,   132,
     133,   574,     0,     0,   575,   702,     0,     0,     0,     0,
       0,     0,     0,     0,   561,   562,   563,     0,     0,     0,
       0,   564,     0,     0,   702,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    30,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   320,     0,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,     0,
       0,     0,     0,     0,   320,   466,   467,   468,   469,   470,
     471,   472,   473,   474,   475,   476,   477,   478,   479,   480,
     481,   482,   483,   484,   485,   486,   487,   488,   489,     0,
       0,   490,   491,    77,   492,     0,   493,   494,   495,   496,
     497,   498,   499,     0,   500,   501,   502,   503,   504,   505,
     391,   392,   393,   394,   395,   396,   397,   398,   399,   400,
     401,   402,   403,   404,   405,   406,   407,   408,   409,   410,
     411,   412,   413,   506,   507,   508,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     663,     0,    16,    17,    18,    19,    20,    21,    22,   352,
     161,     0,     0,     0,     0,   353,     0,     0,     0,     0,
       0,     0,   509,   510,   511,   512,   513,   514,   515,     0,
     516,   517,   518,     0,   420,   421,   422,   423,   424,   425,
     426,   427,   428,   429,   519,     0,     0,     0,     0,     0,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,     0,     0,     0,     0,     0,     0,   320,     0,     0,
       0,     0,     0,   320,     0,     0,   154,   155,   156,   157,
     158,   159,   160,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    12,    13,    14,   231,  -226,     0,     0,    16,
      17,    18,    19,    20,    21,    22,     0,   161,     0,   289,
     290,    26,   259,     0,     0,   389,   390,     0,     0,     0,
     574,   391,   392,   393,   394,   395,   396,   397,   398,   399,
     400,   401,   402,   403,   404,   405,   406,   407,   408,   409,
     410,   411,   412,   413,     0,    30,     0,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,   172,   173,
     174,   308,   309,   310,   311,   312,   313,   314,   273,   274,
       0,     0,     0,     0,   320,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   414,     0,   415,   416,
       0,   417,   418,   419,     0,   420,   421,   422,   423,   424,
     425,   426,   427,   428,   429,     0,     0,   371,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   371,
       0,     0,     0,     0,     0,     0,     0,     0,   371,   288,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     320,     0,     0,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   320,     0,     0,     0,
     320,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      77,   289,   290,     0,   371,   371,     0,     0,     0,   371,
     466,   467,   468,   469,   470,   471,   472,   473,   474,   475,
     476,   477,   478,   479,   480,   481,   482,   483,   484,   485,
     486,   487,   488,   489,     0,     0,   490,   491,     0,   492,
       0,   493,   494,   495,   496,   497,   498,   499,     0,   500,
     501,   502,   503,   504,   505,   391,   392,   393,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   405,
     406,   407,   408,   409,   410,   411,   412,   413,   506,   507,
     508,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   509,   510,   576,
     512,   513,   514,   515,     0,   516,   517,   518,     0,   420,
     421,   422,   423,   424,   425,   426,   427,   428,   429,   519,
     466,   467,   468,   469,   470,   471,   472,   473,   474,   475,
     476,   477,   478,   479,   480,   481,   482,   483,   484,   485,
     486,   487,   488,   489,     0,     0,   490,   491,     0,   492,
       0,   493,   494,   495,   496,   497,   498,   499,     0,   500,
     501,   502,   503,   504,   505,   391,   392,   393,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   405,
     406,   407,   408,   409,   410,   411,   412,   413,   506,   507,
     508,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   509,   510,   751,
     512,   513,   514,   515,     0,   516,   517,   518,     0,   420,
     421,   422,   423,   424,   425,   426,   427,   428,   429,   519,
     466,   467,   468,   469,   470,   471,   472,   473,   474,   475,
     476,   477,   478,   479,   480,   481,   482,   483,   484,   485,
     486,   487,   488,   489,     0,     0,   490,   491,     0,   492,
       0,   493,   494,   495,   496,   497,   498,   499,     0,   500,
     501,   502,   503,   504,   505,   391,   392,   393,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   405,
     406,   407,   408,   409,   410,   411,   412,   413,   506,   507,
     508,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   509,   510,     0,
     512,   513,   514,   515,     0,   516,   517,   518,   752,   420,
     421,   422,   423,   424,   425,   426,   427,   428,   429,   519,
     466,   467,   468,   469,   470,   471,   472,   473,   474,   475,
     476,   477,   478,   479,   480,   481,   482,   483,   484,   485,
     486,   487,   488,   489,     0,     0,   490,   491,     0,   492,
       0,   493,   494,   495,   496,   497,   498,   499,     0,   500,
     501,   502,   503,   504,   505,   391,   392,   393,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   405,
     406,   407,   408,   409,   410,   411,   412,   413,   506,   507,
     508,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,     0,     0,     0,     0,     0,
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
      40,    41,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   509,   510,   958,
     512,   513,   514,   515,     0,   516,   517,   518,     0,   420,
     421,   422,   423,   424,   425,   426,   427,   428,   429,   519,
     466,   467,   468,   469,   470,   471,   472,   473,   474,   475,
     476,   477,   478,   479,   480,   481,   482,   483,   484,   485,
     486,   487,   488,   489,     0,     0,   490,   491,     0,   492,
       0,   493,   494,   495,   496,   497,   498,   499,     0,   500,
     501,   502,   503,   504,   505,   391,   392,   393,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   405,
     406,   407,   408,   409,   410,   411,   412,   413,   506,   507,
     508,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   874,   510,     0,
     512,   513,   514,   515,     0,   516,   517,   518,     0,   420,
     421,   422,   423,   424,   425,   426,   427,   428,   429,   519,
     466,   467,   468,   469,   470,   471,   472,   473,   474,   475,
     476,   477,   478,   479,   480,   481,   482,   483,   484,   485,
     486,   487,   488,   489,     0,     0,   490,   491,     0,   492,
       0,   493,   494,   495,   496,   497,   498,   499,     0,   500,
     501,   502,   503,   504,   505,   391,   392,   393,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   405,
     406,   407,   408,   409,   410,   411,   412,   413,   506,   507,
     508,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   510,     0,
     512,   513,   514,   515,   957,   516,   517,   518,     0,   420,
     421,   422,   423,   424,   425,   426,   427,   428,   429,   519,
     466,   467,   468,   469,   470,   471,   472,   473,   474,   475,
     476,   477,   478,   479,   480,   481,   482,   483,   484,   485,
     486,   487,   488,   489,     0,     0,   490,   491,     0,   492,
       0,   493,   494,   495,   496,   497,   498,   499,     0,   500,
     501,   502,   503,   504,   505,   391,   392,   393,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   405,
     406,   407,   408,   409,   410,   411,   412,   413,   506,   507,
     508,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   959,   510,     0,
     512,   513,   514,   515,     0,   516,   517,   518,     0,   420,
     421,   422,   423,   424,   425,   426,   427,   428,   429,   519,
     466,   467,   468,   469,   470,   471,   472,   473,   474,   475,
     476,   477,   478,   479,   480,   481,   482,   483,   484,   485,
     486,   487,   488,   489,     0,     0,   490,   491,     0,   492,
       0,   493,   494,   495,   496,   497,   498,   499,     0,   500,
     501,   502,   503,   504,   505,   391,   392,   393,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   405,
     406,   407,   408,   409,   410,   411,   412,   413,   506,   507,
     508,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   967,   510,     0,
     512,   513,   514,   515,     0,   516,   517,   518,     0,   420,
     421,   422,   423,   424,   425,   426,   427,   428,   429,   519,
     466,   467,   468,   469,   470,   471,   472,   689,   690,   475,
     476,   477,   478,   479,   480,   481,   482,   483,   484,   485,
     486,   487,   488,   489,     0,     0,   490,   491,     0,   492,
       0,   493,   494,   495,   496,   497,   498,   499,     0,   500,
     501,   502,   503,   504,   505,   391,   392,   393,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   405,
     406,   407,   408,   409,   410,   411,   412,   413,     0,   691,
       0,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   923,   692,   924,
     917,   694,   918,   696,     0,   722,   723,   699,     0,   420,
     421,   422,   423,   424,   425,   426,   427,   428,   429,   700,
     466,   467,   468,   469,   470,   471,   472,   689,   690,   475,
     476,   477,   478,   479,   480,   481,   482,   483,   484,   485,
     486,   487,   488,   489,     0,     0,   490,   491,     0,   492,
       0,   493,   494,   495,   496,   497,   498,   499,     0,   500,
     501,   502,   503,   504,   505,   391,   392,   393,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   405,
     406,   407,   408,   409,   410,   411,   412,   413,     0,   691,
       0,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   692,     0,
     917,   694,   918,   696,   919,   722,   723,   699,     0,   420,
     421,   422,   423,   424,   425,   426,   427,   428,   429,   700,
     466,   467,   468,   469,   470,   471,   472,   689,   690,   475,
     476,   477,   478,   479,   480,   481,   482,   483,   484,   485,
     486,   487,   488,   489,     0,     0,   490,   491,     0,   492,
       0,   493,   494,   495,   496,   497,   498,   499,     0,   500,
     501,   502,   503,   504,   505,   391,   392,   393,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   405,
     406,   407,   408,   409,   410,   411,   412,   413,     0,   691,
       0,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   692,     0,
     917,   694,   918,   696,   922,   722,   723,   699,     0,   420,
     421,   422,   423,   424,   425,   426,   427,   428,   429,   700,
     466,   467,   468,   469,   470,   471,   472,   689,   690,   475,
     476,   477,   478,   479,   480,   481,   482,   483,   484,   485,
     486,   487,   488,   489,     0,     0,   490,   491,     0,   492,
       0,   493,   494,   495,   496,   497,   498,   499,     0,   500,
     501,   502,   503,   504,   505,   391,   392,   393,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   405,
     406,   407,   408,   409,   410,   411,   412,   413,     0,   691,
       0,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   692,     0,
     917,   694,   918,   696,   927,   722,   723,   699,     0,   420,
     421,   422,   423,   424,   425,   426,   427,   428,   429,   700,
     466,   467,   468,   469,   470,   471,   472,   689,   690,   475,
     476,   477,   478,   479,   480,   481,   482,   483,   484,   485,
     486,   487,   488,   489,     0,     0,   490,   491,     0,   492,
       0,   493,   494,   495,   496,   497,   498,   499,     0,   500,
     501,   502,   503,   504,   505,   391,   392,   393,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   405,
     406,   407,   408,   409,   410,   411,   412,   413,     0,   691,
       0,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,     0,     0,     0,     0,     0,
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
      40,    41,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   692,     0,
     693,   694,   695,   696,     0,   697,   698,   699,     0,   420,
     421,   422,   423,   424,   425,   426,   427,   428,   429,   700,
     466,   467,   468,   469,   470,   471,   472,   689,   690,   475,
     476,   477,   478,   479,   480,   481,   482,   483,   484,   485,
     486,   487,   488,   489,     0,     0,   490,   491,     0,   492,
       0,   493,   494,   495,   496,   497,   498,   499,     0,   500,
     501,   502,   503,   504,   505,   391,   392,   393,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   405,
     406,   407,   408,   409,   410,   411,   412,   413,     0,   691,
       0,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   692,     0,
     693,   694,   695,   696,     0,   697,   928,   699,     0,   420,
     421,   422,   423,   424,   425,   426,   427,   428,   429,   700,
     466,   467,   468,   469,   470,   471,   472,   689,   690,   475,
     476,   477,   478,   479,   480,   481,   482,   483,   484,   485,
     486,   487,   488,   489,     0,     0,   490,   491,     0,   492,
       0,   493,   494,   495,   496,   497,   498,   499,     0,   500,
     501,   502,   503,   504,   505,   391,   392,   393,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   405,
     406,   407,   408,   409,   410,   411,   412,   413,     0,   691,
       0,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   692,     0,
       0,   694,     0,   696,     0,   722,   723,   699,     0,   420,
     421,   422,   423,   424,   425,   426,   427,   428,   429,   700,
     466,   467,   468,   469,   470,   471,   472,   689,   690,   475,
     476,   477,   478,   479,   480,   481,   482,   483,   484,   485,
     486,   487,   488,   489,     0,     0,   490,   491,     0,   492,
       0,   493,   494,   495,   496,   497,   498,   499,     0,   500,
     501,   502,   503,   504,   505,   391,   392,   393,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   405,
     406,   407,   408,   409,   410,   411,   412,   413,     0,   691,
       0,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   692,     0,
       0,   694,     0,   696,     0,   697,     0,   699,     0,   420,
     421,   422,   423,   424,   425,   426,   427,   428,   429,   700,
       5,     6,     7,     8,     9,    10,    11,    16,    17,    18,
      19,    20,    21,    22,   352,   161,    12,    13,    14,    15,
     353,     0,     0,    16,    17,    18,    19,    20,    21,    22,
     773,   161,    24,    25,     0,    26,    27,     0,   774,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,     0,     0,     0,    30,
       0,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,   775,   776,     5,     6,
       7,     8,     9,    10,    11,     0,     0,     0,     0,    77,
       0,     0,     0,     0,    12,    13,    14,    15,     0,     0,
       0,    16,    17,    18,    19,    20,    21,    22,     0,    23,
      24,    25,     0,    26,    27,    28,    29,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    30,     0,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,     0,    76,     5,     6,     7,     8,
       9,    10,    11,     0,     0,     0,     0,    77,     0,     0,
       0,     0,    12,    13,    14,   231,     0,     0,     0,    16,
      17,    18,    19,    20,    21,    22,     0,   161,    24,    25,
       0,    26,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    30,     0,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,     5,     6,     7,     8,     9,    10,    11,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    12,    13,    14,
      15,     0,     0,     0,    16,    17,    18,    19,    20,    21,
      22,     0,   161,    24,     0,    77,    26,    27,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   308,   309,   310,   311,   312,   313,   314,     0,
      30,     0,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,     5,     6,     7,     8,
       9,    10,    11,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    12,    13,    14,   231,     0,     0,     0,    16,
      17,    18,    19,    20,    21,    22,     0,   161,    24,     0,
      77,    26,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   127,   128,   129,
     130,   131,   132,   133,     0,    30,     0,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    77,   191,     0,   192,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   127,   128,   129,   130,   131,
     132,   133,   273,   274,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    77,    30,   308,   309,   310,
     311,   312,   313,   314,   736,   737,     0,    24,     0,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,     0,   127,   128,   129,   130,   131,   132,   133,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   127,   128,   129,   130,   131,
     132,   133,     0,     0,     0,     0,    77,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
       0,     0,     0,     0,     0,     0,     0,    24,   251,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,    30,   308,   309,   310,   311,   312,   313,   314,
     273,   274,     0,     0,    77,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,     0,     0,
     738,     0,     0,     0,     0,     0,    77,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
       0,   904,     0,     0,   905,     0,     0,     0,     0,     0,
       0,     0,    77,     0,   127,   128,   129,   130,   131,   132,
     133,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    77,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,     0,   191,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   127,   128,   129,   130,   131,   132,   133,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    77,    30,   127,   128,   129,   130,   131,   132,
     133,     0,     0,     0,     0,     0,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,     0,
     127,   128,   129,   130,   131,   132,   133,   251,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    30,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    24,    77,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    77,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,     0,     0,     0,     0,     0,
       0,     0,     0,    77,    16,    17,    18,    19,    20,    21,
      22,   352,   161,     0,     0,     0,     0,   353,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    77,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,   389,   390,     0,     0,     0,     0,   391,
     392,   393,   394,   395,   396,   397,   398,   399,   400,   401,
     402,   403,   404,   405,   406,   407,   408,   409,   410,   411,
     412,   413,     0,     0,     0,     0,     0,   154,   155,   156,
     157,   158,   159,   160,     0,     0,     0,     0,     0,     0,
       0,   289,   290,    12,    13,    14,   231,     0,     0,     0,
      16,    17,    18,    19,    20,    21,    22,     0,   161,    24,
      25,     0,    26,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   414,     0,   415,   416,     0,   417,
     418,   419,     0,   420,   421,   422,   423,   424,   425,   426,
     427,   428,   429,     0,     0,     0,    30,     0,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,   162,
     163,   164,   165,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   154,   155,   156,   157,   158,   159,   160,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    12,    13,
      14,    15,     0,     0,     0,    16,    17,    18,    19,    20,
      21,    22,     0,   161,    24,     0,     0,    26,    27,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    30,     0,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   154,   155,   156,
     157,   158,   159,   160,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    12,    13,    14,   231,     0,     0,     0,
      16,    17,    18,    19,    20,    21,    22,     0,   161,    24,
       0,     0,    26,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    30,     0,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,   162,
     163,   164,   165,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   154,   155,   156,   157,   158,   159,   160,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    12,    13,
      14,    15,     0,     0,     0,    16,    17,    18,    19,    20,
      21,    22,     0,   161,     0,     0,     0,    26,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    30,     0,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   154,   155,   156,
     157,   158,   159,   160,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    12,    13,    14,   231,     0,     0,     0,
      16,    17,    18,    19,    20,    21,    22,     0,   161,     0,
       0,     0,    26,   154,   155,   156,   157,   158,   159,   160,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   617,
       0,   618,   619,     0,     0,     0,    16,    17,    18,    19,
      20,    21,    22,     0,   161,     0,    30,     0,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,   162,
     163,   164,   165,   166,   167,   168,   169,   170,   171,   172,
     173,   174,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    30,     0,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174
};

static const yytype_int16 yycheck[] =
{
       4,    27,    96,     4,    28,     4,    27,     4,    99,   100,
     101,    15,   284,   115,     4,   120,   286,     4,    15,   321,
       4,   602,   120,   387,   181,    29,   121,   687,    15,   109,
     584,   727,    29,   448,   115,   115,     4,    27,   298,   581,
     195,   195,   109,    27,   803,   286,   109,    72,   590,   120,
     121,   802,    29,     4,   629,   668,   305,   599,   241,    72,
     931,    29,    72,   229,   109,   835,   111,    26,    10,    11,
     372,    23,    24,    25,   278,   826,    27,     0,    37,   950,
     900,    72,   120,   181,    88,   905,   149,     0,   126,   115,
      72,    88,   668,    20,   115,   120,   115,   639,    90,   180,
     370,    88,    10,    11,   149,   109,    96,   111,    20,    36,
      12,    38,    96,   126,   111,   775,   120,   321,   128,    26,
     128,    28,    72,   126,    72,   329,    38,   284,    96,   370,
      37,   115,   195,   110,   239,   112,   125,   128,   958,   124,
     129,   126,   121,   724,   225,   149,   128,   115,   229,   153,
     195,   196,   120,   104,   180,   120,   153,   238,   238,   180,
     773,   180,   126,   933,   123,   740,   153,   248,   372,   126,
     122,   126,   176,   150,   120,   330,   330,   936,   128,   176,
     128,   286,   941,   175,   126,   177,   284,   191,   192,   176,
     270,   195,   196,   584,   191,   192,   180,   773,   613,   196,
     321,    72,   179,   270,   191,   192,   965,   270,   303,   785,
     305,   602,   238,   462,   463,   381,   126,   383,   126,   238,
      72,   802,   303,   286,   305,   270,   120,   231,   196,   248,
     195,   806,   126,   126,   231,   239,   932,   130,    72,    72,
     126,   120,   120,   247,   231,   826,   236,   126,   126,   120,
     247,   372,   236,   120,   238,   126,   126,   832,   356,   126,
     247,   836,   266,    72,   126,   370,   270,   330,   236,   266,
     238,   239,   532,   265,   239,   267,   128,   126,   120,   266,
     257,   258,   286,   440,   126,   330,   126,   120,   369,   369,
     130,   120,   873,   126,   128,   128,   956,   126,   121,    72,
     381,   124,   383,   460,   296,   126,   120,   370,   889,   126,
     287,   120,   126,   126,   306,   307,   120,   126,   322,   128,
     297,   286,   126,   120,   688,   322,   330,   510,   126,   126,
     120,   885,   120,   724,   455,   518,   126,   125,   640,   641,
     642,   126,   440,   369,   120,   887,   529,   120,   120,   125,
     369,   375,    72,   126,   126,   128,   812,   361,   814,   126,
     816,   126,   460,   120,   361,   330,   370,   462,   463,   126,
     451,   375,    72,    72,   361,   126,    72,   931,   329,   120,
     121,   462,   463,   124,   126,   369,     3,     4,     5,     6,
       7,     8,     9,   126,   386,   126,   950,   126,   120,   121,
     120,   369,   124,   321,   120,   370,   126,   126,   128,   133,
     134,   802,     3,     4,     5,     6,     7,     8,     9,   225,
     120,   120,    72,   229,   120,   128,   126,   126,   128,   128,
     126,   121,   128,   121,   907,   826,   640,   641,   642,   912,
     804,     3,     4,     5,     6,     7,     8,     9,   120,   121,
     129,   455,   124,    12,   372,   123,   457,   120,   455,   120,
     121,    26,    27,   124,   121,   455,    26,    27,   740,   446,
     943,   944,   124,   946,     3,     4,     5,     6,     7,     8,
       9,    72,   873,    13,    14,    15,   126,   960,   126,   126,
     881,   882,   883,   884,   885,   886,   126,   303,   889,   305,
     123,   803,   130,   124,   121,   121,   120,   588,    23,    24,
      25,   130,   125,   125,   125,   125,   125,   125,   125,   640,
     641,   642,   125,   125,   125,   795,   125,   125,   125,   624,
     125,   127,   125,   127,   120,   120,   124,    19,    20,    21,
     931,   622,   131,   638,    26,    27,    28,    29,    30,    31,
      32,   127,    34,   130,   531,   127,   127,   130,   122,   950,
      36,   127,   120,   120,   120,    30,   127,    18,   131,   120,
     125,   125,   125,   125,   122,   381,   125,   383,   582,   123,
     123,   127,   120,    20,   131,   582,   125,   120,   120,   125,
     127,   861,   543,   124,   122,   582,   131,    71,   125,   803,
     125,   127,   579,   622,   127,   127,   127,   127,   127,   127,
     127,   127,   125,   617,   618,   619,   127,   127,   127,   127,
     617,   618,   619,   127,   125,   127,   120,   123,   131,   127,
     617,   618,   619,   125,   936,    13,    13,   792,   940,   941,
     120,   127,   127,   127,   621,   451,   127,   120,   127,     4,
     365,   668,   672,   564,   676,   857,   462,   463,   459,   668,
     638,   460,   668,   965,   668,   731,   670,   668,   672,   668,
     674,   668,     4,   670,   785,   672,   773,   674,   668,   808,
     878,   668,   803,   670,   668,   672,   916,   674,   668,   356,
     795,   785,   794,   355,   645,   932,   787,   795,   649,   650,
     668,   962,   653,   654,   655,   656,   657,   658,   659,   660,
     661,   662,   741,   794,   794,   870,   721,   668,   606,   668,
     295,   904,   640,   641,   642,   295,   815,   794,    -1,   792,
     883,   682,   936,    -1,    -1,   718,   940,   941,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   792,    -1,    -1,
      -1,    -1,   744,   834,    -1,    -1,   861,    -1,    -1,    -1,
      -1,   965,    -1,   857,    -1,    -1,    -1,   858,   794,   773,
     774,   795,   773,   794,    -1,   794,    -1,   774,   584,   860,
     860,   785,   588,   773,   865,   865,   785,    -1,   792,   773,
     794,   795,    -1,   860,    -1,   785,   602,   774,   865,    -1,
      -1,   785,    -1,    -1,   808,   756,   774,   870,   759,   890,
     794,   808,    -1,    -1,    -1,   936,   793,   785,    -1,   940,
     941,   808,   773,    -1,    -1,   870,   794,   792,    -1,   910,
     795,    -1,    -1,    -1,   785,    -1,   828,   861,    -1,   865,
      -1,   860,    -1,    -1,   965,    -1,   865,    -1,    -1,    -1,
      -1,   855,    -1,    -1,    -1,    -1,   860,   861,   855,    -1,
      -1,   865,    -1,   109,    -1,    -1,   870,   857,   855,    -1,
      -1,    -1,    -1,   857,   120,    -1,   860,    -1,    -1,    -1,
      -1,   865,   859,    -1,    -1,   803,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   861,   865,    -1,    -1,
      -1,    -1,    -1,    -1,   908,   870,    -1,    -1,   912,    -1,
      -1,   908,    -1,    -1,    -1,   912,    -1,    -1,   724,    -1,
      -1,   908,    -1,    -1,    -1,   912,   910,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   181,    -1,    -1,    -1,    -1,
     944,    -1,    -1,    -1,    -1,    -1,    -1,   944,    -1,    -1,
      -1,   197,    -1,    -1,    -1,   201,   202,   944,   204,   205,
     206,   207,   208,   209,   210,   211,   212,   213,   214,   215,
     216,   217,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   239,    -1,    -1,   802,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   936,    -1,
     826,    -1,   940,   941,   270,    -1,    -1,    -1,   834,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   284,    -1,
     286,    -1,    -1,    -1,    -1,    -1,    -1,   965,    -1,    -1,
      -1,    -1,    -1,    -1,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,   873,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   881,   882,   883,   884,   885,
     886,    -1,    -1,   889,   890,    -1,   332,   333,   334,    -1,
      -1,   337,    -1,    -1,    -1,   126,    -1,    -1,    -1,    -1,
      -1,   132,   133,   134,    -1,    -1,    -1,    -1,    -1,    -1,
     356,    -1,    -1,    -1,     3,     4,     5,     6,     7,     8,
       9,   367,    -1,    -1,   370,   931,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    23,    24,    25,    -1,    -1,    -1,
      -1,    30,    -1,    -1,   950,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    72,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   440,    -1,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    -1,
      -1,    -1,    -1,    -1,   460,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    -1,
      -1,    29,    30,   132,    32,    -1,    34,    35,    36,    37,
      38,    39,    40,    -1,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
     556,    -1,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    -1,   120,   121,   122,   123,   124,   125,   126,    -1,
     128,   129,   130,    -1,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,    -1,    -1,    -1,    -1,    -1,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    -1,    -1,    -1,    -1,    -1,    -1,   623,    -1,    -1,
      -1,    -1,    -1,   629,    -1,    -1,     3,     4,     5,     6,
       7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    19,    20,    21,    22,   120,    -1,    -1,    26,
      27,    28,    29,    30,    31,    32,    -1,    34,    -1,   133,
     134,    38,   668,    -1,    -1,    42,    43,    -1,    -1,    -1,
     676,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    -1,    72,    -1,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      -1,    -1,    -1,    -1,   740,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   123,    -1,   125,   126,
      -1,   128,   129,   130,    -1,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,    -1,    -1,   773,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   785,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   794,   795,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     806,    -1,    -1,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   832,    -1,    -1,    -1,
     836,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     132,   133,   134,    -1,   860,   861,    -1,    -1,    -1,   865,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    29,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    39,    40,    -1,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   120,   121,   122,
     123,   124,   125,   126,    -1,   128,   129,   130,    -1,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    29,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    39,    40,    -1,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   120,   121,   122,
     123,   124,   125,   126,    -1,   128,   129,   130,    -1,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    29,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    39,    40,    -1,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   120,   121,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    29,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    39,    40,    -1,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   120,   121,    -1,
     123,   124,   125,   126,   127,   128,   129,   130,    -1,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    29,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    39,    40,    -1,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   120,   121,   122,
     123,   124,   125,   126,    -1,   128,   129,   130,    -1,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    29,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    39,    40,    -1,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   120,   121,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,    -1,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    29,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    39,    40,    -1,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,    -1,
     123,   124,   125,   126,   127,   128,   129,   130,    -1,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    29,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    39,    40,    -1,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   120,   121,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,    -1,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    29,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    39,    40,    -1,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   120,   121,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,    -1,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    29,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    39,    40,    -1,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    -1,    72,
      -1,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   120,   121,   122,
     123,   124,   125,   126,    -1,   128,   129,   130,    -1,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    29,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    39,    40,    -1,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    -1,    72,
      -1,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,    -1,
     123,   124,   125,   126,   127,   128,   129,   130,    -1,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    29,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    39,    40,    -1,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    -1,    72,
      -1,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,    -1,
     123,   124,   125,   126,   127,   128,   129,   130,    -1,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    29,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    39,    40,    -1,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    -1,    72,
      -1,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,    -1,
     123,   124,   125,   126,   127,   128,   129,   130,    -1,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    29,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    39,    40,    -1,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    -1,    72,
      -1,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    29,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    39,    40,    -1,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    -1,    72,
      -1,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,    -1,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    29,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    39,    40,    -1,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    -1,    72,
      -1,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,    -1,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    29,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    39,    40,    -1,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    -1,    72,
      -1,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,    -1,
      -1,   124,    -1,   126,    -1,   128,   129,   130,    -1,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    29,    30,    -1,    32,
      -1,    34,    35,    36,    37,    38,    39,    40,    -1,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    -1,    72,
      -1,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,    -1,
      -1,   124,    -1,   126,    -1,   128,    -1,   130,    -1,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
       3,     4,     5,     6,     7,     8,     9,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    19,    20,    21,    22,
      39,    -1,    -1,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    -1,    38,    39,    -1,    41,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    -1,    -1,    -1,    72,
      -1,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,     3,     4,
       5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,   132,
      -1,    -1,    -1,    -1,    19,    20,    21,    22,    -1,    -1,
      -1,    26,    27,    28,    29,    30,    31,    32,    -1,    34,
      35,    36,    -1,    38,    39,    40,    41,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    72,    -1,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,    -1,   120,     3,     4,     5,     6,
       7,     8,     9,    -1,    -1,    -1,    -1,   132,    -1,    -1,
      -1,    -1,    19,    20,    21,    22,    -1,    -1,    -1,    26,
      27,    28,    29,    30,    31,    32,    -1,    34,    35,    36,
      -1,    38,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    72,    -1,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,     3,     4,     5,     6,     7,     8,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    20,    21,
      22,    -1,    -1,    -1,    26,    27,    28,    29,    30,    31,
      32,    -1,    34,    35,    -1,   132,    38,    39,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     3,     4,     5,     6,     7,     8,     9,    -1,
      72,    -1,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,     3,     4,     5,     6,
       7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    19,    20,    21,    22,    -1,    -1,    -1,    26,
      27,    28,    29,    30,    31,    32,    -1,    34,    35,    -1,
     132,    38,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,     3,     4,     5,
       6,     7,     8,     9,    -1,    72,    -1,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,   132,    38,    -1,    40,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   132,    72,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    -1,    35,    -1,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    -1,     3,     4,     5,     6,     7,     8,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     3,     4,     5,     6,     7,
       8,     9,    -1,    -1,    -1,    -1,   132,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    35,    36,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    72,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    -1,    -1,   132,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    -1,    -1,
     126,    -1,    -1,    -1,    -1,    -1,   132,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      -1,   121,    -1,    -1,   124,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   132,    -1,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   132,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    -1,    38,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   132,    72,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    -1,    -1,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    -1,
       3,     4,     5,     6,     7,     8,     9,    36,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    72,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    35,   132,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   132,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   132,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    -1,    -1,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   132,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    42,    43,    -1,    -1,    -1,    -1,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,
       6,     7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   133,   134,    19,    20,    21,    22,    -1,    -1,    -1,
      26,    27,    28,    29,    30,    31,    32,    -1,    34,    35,
      36,    -1,    38,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   123,    -1,   125,   126,    -1,   128,
     129,   130,    -1,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,    -1,    -1,    -1,    72,    -1,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    20,
      21,    22,    -1,    -1,    -1,    26,    27,    28,    29,    30,
      31,    32,    -1,    34,    35,    -1,    -1,    38,    39,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    72,    -1,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,     3,     4,     5,
       6,     7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    19,    20,    21,    22,    -1,    -1,    -1,
      26,    27,    28,    29,    30,    31,    32,    -1,    34,    35,
      -1,    -1,    38,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    72,    -1,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    20,
      21,    22,    -1,    -1,    -1,    26,    27,    28,    29,    30,
      31,    32,    -1,    34,    -1,    -1,    -1,    38,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    72,    -1,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,     3,     4,     5,
       6,     7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    19,    20,    21,    22,    -1,    -1,    -1,
      26,    27,    28,    29,    30,    31,    32,    -1,    34,    -1,
      -1,    -1,    38,     3,     4,     5,     6,     7,     8,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,
      -1,    21,    22,    -1,    -1,    -1,    26,    27,    28,    29,
      30,    31,    32,    -1,    34,    -1,    72,    -1,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    72,    -1,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   144,   145,     0,   146,     3,     4,     5,     6,     7,
       8,     9,    19,    20,    21,    22,    26,    27,    28,    29,
      30,    31,    32,    34,    35,    36,    38,    39,    40,    41,
      72,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   120,   132,   147,   148,
     149,   150,   152,   153,   154,   155,   156,   158,   161,   175,
     176,   178,   186,   187,   196,   198,   199,   216,   217,   218,
     219,   222,   223,   226,   232,   259,   289,   290,   291,   292,
     294,   295,   296,   298,   299,   302,   303,   304,   305,   306,
     308,   309,   312,   319,   320,   321,   327,     3,     4,     5,
       6,     7,     8,     9,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,   289,   291,   292,
     296,    12,   128,   313,     3,     4,     5,     6,     7,     8,
       9,    34,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,   156,   161,   176,   291,   296,
     302,   308,     3,     4,     5,     6,     7,     8,     9,   121,
     299,    38,    40,   197,   289,   292,   295,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   126,   126,     3,     4,
       5,     6,     7,     8,     9,   300,   157,   160,   289,   300,
     121,    22,   148,   154,   155,   183,   199,   216,   302,   308,
     120,   121,   237,   237,   237,   237,   126,   161,   302,   308,
     120,    36,   223,   240,   241,   244,   290,   294,   296,   298,
     295,   290,   291,   295,   128,   156,   161,   176,   187,   223,
     292,   303,   312,    10,    11,   227,   229,   231,   232,   233,
     238,   240,   260,   268,   271,   279,   292,   296,   298,   133,
     134,   265,   322,   323,   324,   326,   310,   296,   121,   129,
     200,   289,    12,   300,   289,   300,   156,   176,     3,     4,
       5,     6,     7,     8,     9,   191,   192,   195,   268,   279,
     298,   375,   123,   151,   289,   289,   120,   226,   231,   232,
     292,   298,   329,   332,   333,   298,   298,   337,   298,   298,
     298,   298,   298,   298,   298,   298,   298,   298,   298,   298,
     298,   298,    33,    39,   261,   264,   265,   301,   303,   321,
     121,   124,   168,   169,   120,   159,   261,   177,   289,   302,
     292,   298,   375,   224,   289,   293,   126,   242,   295,   295,
     297,   300,   289,   300,   223,   241,   311,   228,   126,    42,
      43,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,   123,   125,   126,   128,   129,   130,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     351,   352,   234,   126,   262,   123,   256,   257,   268,   277,
     322,   281,   282,   283,   227,   233,   296,   124,   325,   323,
     324,   300,   145,   201,   203,   204,   206,   208,   188,   190,
     265,   188,   300,   300,   189,   126,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      29,    30,    32,    34,    35,    36,    37,    38,    39,    40,
      42,    43,    44,    45,    46,    47,    71,    72,    73,   120,
     121,   122,   123,   124,   125,   126,   128,   129,   130,   142,
     319,   352,   353,   354,   377,   378,   379,   380,   381,   382,
     291,   296,   121,   120,   226,   231,   125,   298,   298,   298,
     125,   125,   298,   125,   125,   125,   125,   125,   125,   125,
     125,   125,   125,   125,   127,   127,   125,   120,   262,   260,
     162,    23,    24,    25,    30,   170,   171,   174,   289,   168,
     120,   179,   180,   181,   298,   298,   122,   249,   250,   296,
     299,   245,   124,   246,   361,   261,   120,   261,   300,   234,
     230,   130,   130,   127,   131,    26,    37,   123,   235,   239,
     120,   125,   258,   127,   268,   130,   284,   285,    13,    14,
      15,   280,   306,   307,   122,   125,   129,    19,    21,    22,
     291,   296,   302,   314,   315,   318,   319,    20,    38,   211,
     199,   120,   189,   195,   120,   188,   188,   120,   125,   193,
     375,   375,   375,   120,   145,   328,   125,   127,   127,   334,
     336,   125,   308,   340,   342,   344,   346,   341,   343,   345,
     347,   348,   349,   298,    23,    24,    25,   122,   163,   164,
     173,   174,   125,    30,   172,   122,   125,   123,   123,   127,
     251,   253,   254,   295,   249,   247,   248,   289,   243,    10,
      11,    72,   121,   123,   124,   125,   126,   128,   129,   130,
     142,   320,   321,   352,   353,   354,   356,   363,   364,   365,
     367,   369,   373,   120,   120,   249,   131,   131,   236,    18,
     249,   263,   128,   129,   355,   356,   358,   269,   286,   285,
     306,   202,   289,   289,   289,   312,    10,    11,   126,   266,
     270,   278,   279,   265,   316,   266,    20,   120,   120,   190,
     249,   122,   131,   127,   122,   308,   330,   308,   308,   338,
     127,   308,   308,   308,   308,   308,   308,   308,   308,   308,
     308,   125,   350,    33,    41,   119,   120,   153,   155,   165,
     166,   167,   175,   186,   196,   199,   220,   221,   222,   241,
     259,   289,   292,   296,   302,   308,   327,   124,   289,   171,
     289,   181,   182,   376,   225,   125,   308,   127,   125,   381,
     234,   371,   372,   374,   370,   366,   368,   127,   369,   127,
     264,   358,   126,   272,   282,   287,   288,   203,   317,   205,
     266,   276,   322,   281,   300,   207,   209,   127,   127,   308,
     335,   127,   308,   127,   127,   127,   127,   127,   127,   127,
     127,   125,   125,   127,   153,   161,   184,   199,   221,   296,
     302,   308,   381,   155,   220,   302,   237,   120,   231,   241,
     292,   233,   296,   355,   120,   378,   234,    71,   252,   266,
     248,   357,   357,   362,   357,   361,   357,   273,   131,   355,
     300,   123,   212,   213,   127,   266,   212,   266,   194,   331,
     125,   339,    13,    13,   121,   124,   185,   289,   161,   184,
     302,   237,   161,   120,   231,   253,   255,   123,   125,   127,
     358,   359,   127,   120,   122,   359,   360,   127,   129,   131,
     249,   214,   267,   210,   275,   127,   376,   127,   127,   127,
     375,   376,   185,   289,   161,   185,   289,   120,   256,   127,
     215,   363,   272,   212,    26,    28,    37,   127,   122,   120,
     289,   185,   274,   363,   381,   376,   275,   120
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
#line 1464 "vtkParse.y"
    {
      startSig();
      clearType();
      clearTypeId();
      clearTemplate();
      closeComment();
    }
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 1509 "vtkParse.y"
    { pushNamespace((yyvsp[(2) - (2)].str)); }
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 1510 "vtkParse.y"
    { popNamespace(); }
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 1532 "vtkParse.y"
    { pushType(); }
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 1533 "vtkParse.y"
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

  case 38:

/* Line 1455 of yacc.c  */
#line 1546 "vtkParse.y"
    { start_class((yyvsp[(2) - (2)].str), (yyvsp[(1) - (2)].integer)); }
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 1548 "vtkParse.y"
    { start_class(NULL, (yyvsp[(1) - (1)].integer)); }
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 1552 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 1553 "vtkParse.y"
    { (yyval.integer) = 1; }
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 1554 "vtkParse.y"
    { (yyval.integer) = 2; }
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 1558 "vtkParse.y"
    {
      startSig();
      clearType();
      clearTypeId();
      clearTemplate();
      closeComment();
    }
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 1570 "vtkParse.y"
    { access_level = VTK_ACCESS_PUBLIC; }
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 1571 "vtkParse.y"
    { access_level = VTK_ACCESS_PRIVATE; }
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 1572 "vtkParse.y"
    { access_level = VTK_ACCESS_PROTECTED; }
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 1597 "vtkParse.y"
    { output_friend_function(); }
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 1611 "vtkParse.y"
    { add_base_class(currentClass, (yyvsp[(1) - (1)].str), access_level, 0); }
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 1613 "vtkParse.y"
    { add_base_class(currentClass, (yyvsp[(3) - (3)].str), (yyvsp[(2) - (3)].integer), 1); }
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 1615 "vtkParse.y"
    { add_base_class(currentClass, (yyvsp[(3) - (3)].str), (yyvsp[(1) - (3)].integer), (yyvsp[(2) - (3)].integer)); }
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 1618 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 1619 "vtkParse.y"
    { (yyval.integer) = 1; }
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 1622 "vtkParse.y"
    { (yyval.integer) = access_level; }
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 1626 "vtkParse.y"
    { (yyval.integer) = VTK_ACCESS_PUBLIC; }
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 1627 "vtkParse.y"
    { (yyval.integer) = VTK_ACCESS_PRIVATE; }
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 1628 "vtkParse.y"
    { (yyval.integer) = VTK_ACCESS_PROTECTED; }
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 1644 "vtkParse.y"
    { pushType(); start_enum((yyvsp[(1) - (2)].str)); }
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 1645 "vtkParse.y"
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

  case 90:

/* Line 1455 of yacc.c  */
#line 1657 "vtkParse.y"
    { (yyval.str) = (yyvsp[(2) - (2)].str); }
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 1658 "vtkParse.y"
    { (yyval.str) = NULL; }
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 1669 "vtkParse.y"
    { add_enum((yyvsp[(1) - (1)].str), NULL); }
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 1670 "vtkParse.y"
    { postSig("="); markSig(); }
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 1671 "vtkParse.y"
    { chopSig(); add_enum((yyvsp[(1) - (4)].str), copySig()); }
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 1724 "vtkParse.y"
    { pushFunction(); postSig("("); }
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 1725 "vtkParse.y"
    { postSig(")"); }
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 1726 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_FUNCTION; popFunction(); }
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 1730 "vtkParse.y"
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

  case 124:

/* Line 1455 of yacc.c  */
#line 1763 "vtkParse.y"
    { add_using((yyvsp[(2) - (3)].str), 0); }
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1767 "vtkParse.y"
    { (yyval.str) = (yyvsp[(2) - (2)].str); }
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1769 "vtkParse.y"
    { (yyval.str) = vtkstrcat((yyvsp[(1) - (2)].str), (yyvsp[(2) - (2)].str)); }
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1771 "vtkParse.y"
    { (yyval.str) = vtkstrcat((yyvsp[(1) - (2)].str), (yyvsp[(2) - (2)].str)); }
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1773 "vtkParse.y"
    { (yyval.str) = vtkstrcat3((yyvsp[(1) - (3)].str), (yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].str)); }
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1775 "vtkParse.y"
    { (yyval.str) = vtkstrcat3((yyvsp[(1) - (3)].str), (yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].str)); }
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1778 "vtkParse.y"
    { add_using((yyvsp[(3) - (4)].str), 1); }
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1786 "vtkParse.y"
    { postSig("template<> "); clearTypeId(); }
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1788 "vtkParse.y"
    {
      postSig("template<");
      pushType();
      clearType();
      clearTypeId();
      startTemplate();
    }
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1796 "vtkParse.y"
    {
      chopSig();
      if (getSig()[getSigLength()-1] == '>') { postSig(" "); }
      postSig("> ");
      clearTypeId();
      popType();
    }
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1807 "vtkParse.y"
    { chopSig(); postSig(", "); clearType(); clearTypeId(); }
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1811 "vtkParse.y"
    { markSig(); }
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1813 "vtkParse.y"
    { add_template_parameter(getType(), (yyvsp[(3) - (3)].integer), copySig()); }
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 1815 "vtkParse.y"
    { markSig(); }
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 1817 "vtkParse.y"
    { add_template_parameter(0, (yyvsp[(3) - (3)].integer), copySig()); }
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 1819 "vtkParse.y"
    { pushTemplate(); markSig(); }
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 1820 "vtkParse.y"
    { postSig("class "); }
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 1822 "vtkParse.y"
    {
      int i;
      TemplateInfo *newTemplate = currentTemplate;
      popTemplate();
      add_template_parameter(0, (yyvsp[(5) - (5)].integer), copySig());
      i = currentTemplate->NumberOfParameters-1;
      currentTemplate->Parameters[i]->Template = newTemplate;
    }
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 1833 "vtkParse.y"
    { postSig("class "); }
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 1834 "vtkParse.y"
    { postSig("typename "); }
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 1840 "vtkParse.y"
    { postSig("="); markSig(); }
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 1842 "vtkParse.y"
    {
      int i = currentTemplate->NumberOfParameters-1;
      ValueInfo *param = currentTemplate->Parameters[i];
      chopSig();
      param->Value = copySig();
    }
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1859 "vtkParse.y"
    { output_function(); }
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1860 "vtkParse.y"
    { output_function(); }
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 1861 "vtkParse.y"
    { reject_function(); }
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 1862 "vtkParse.y"
    { reject_function(); }
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 1878 "vtkParse.y"
    { output_function(); }
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 1895 "vtkParse.y"
    {
      postSig("(");
      set_return(currentFunction, getType(), getTypeId(), 0);
    }
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1899 "vtkParse.y"
    { postSig(")"); }
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1901 "vtkParse.y"
    {
      postSig(";");
      closeSig();
      currentFunction->IsOperator = 1;
      currentFunction->Name = "operator typecast";
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed operator", "operator typecast");
    }
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 1912 "vtkParse.y"
    { (yyval.str) = copySig(); }
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 1915 "vtkParse.y"
    { postSig(")"); }
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 1916 "vtkParse.y"
    {
      postSig(";");
      closeSig();
      currentFunction->Name = (yyvsp[(1) - (3)].str);
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed operator", currentFunction->Name);
    }
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 1926 "vtkParse.y"
    {
      postSig("(");
      currentFunction->IsOperator = 1;
      set_return(currentFunction, getType(), getTypeId(), 0);
    }
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 1935 "vtkParse.y"
    { chopSig(); (yyval.str) = vtkstrcat(copySig(), (yyvsp[(2) - (2)].str)); postSig((yyvsp[(2) - (2)].str)); }
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 1938 "vtkParse.y"
    { markSig(); postSig("operator "); }
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 1942 "vtkParse.y"
    {
      postSig(";");
      closeSig();
      currentFunction->Name = (yyvsp[(1) - (2)].str);
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 1954 "vtkParse.y"
    { postSig(" throw "); }
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 1954 "vtkParse.y"
    { chopSig(); }
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 1955 "vtkParse.y"
    { postSig(" const"); currentFunction->IsConst = 1; }
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 1957 "vtkParse.y"
    {
      postSig(" = 0");
      currentFunction->IsPureVirtual = 1;
      if (currentClass) { currentClass->IsAbstract = 1; }
    }
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 1969 "vtkParse.y"
    {
      postSig("(");
      set_return(currentFunction, getType(), getTypeId(), 0);
    }
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 1973 "vtkParse.y"
    { postSig(")"); }
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 1985 "vtkParse.y"
    { closeSig(); }
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 1986 "vtkParse.y"
    { openSig(); }
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 1987 "vtkParse.y"
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

  case 203:

/* Line 1455 of yacc.c  */
#line 2004 "vtkParse.y"
    { pushType(); postSig("("); }
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 2005 "vtkParse.y"
    { popType(); postSig(")"); }
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 2022 "vtkParse.y"
    { clearType(); clearTypeId(); }
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 2025 "vtkParse.y"
    { clearType(); clearTypeId(); }
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 2026 "vtkParse.y"
    { clearType(); clearTypeId(); postSig(", "); }
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 2029 "vtkParse.y"
    { currentFunction->IsVariadic = 1; postSig(", ..."); }
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 2032 "vtkParse.y"
    { markSig(); }
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 2034 "vtkParse.y"
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

  case 219:

/* Line 1455 of yacc.c  */
#line 2049 "vtkParse.y"
    {
      int i = currentFunction->NumberOfParameters-1;
      if (getVarValue())
        {
        currentFunction->Parameters[i]->Value = getVarValue();
        }
    }
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 2058 "vtkParse.y"
    { clearVarValue(); }
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 2062 "vtkParse.y"
    { postSig("="); clearVarValue(); markSig(); }
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 2063 "vtkParse.y"
    { chopSig(); setVarValue(copySig()); }
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 2074 "vtkParse.y"
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

  case 229:

/* Line 1455 of yacc.c  */
#line 2136 "vtkParse.y"
    { postSig(", "); }
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 2142 "vtkParse.y"
    { setTypePtr(0); }
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 2143 "vtkParse.y"
    { setTypePtr((yyvsp[(1) - (1)].integer)); }
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 2147 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 2148 "vtkParse.y"
    { postSig(")"); }
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 2150 "vtkParse.y"
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

  case 237:

/* Line 1455 of yacc.c  */
#line 2167 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 2168 "vtkParse.y"
    { postSig(")"); }
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 2170 "vtkParse.y"
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

  case 240:

/* Line 1455 of yacc.c  */
#line 2186 "vtkParse.y"
    { postSig("("); scopeSig(""); (yyval.integer) = 0; }
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 2187 "vtkParse.y"
    { postSig("("); scopeSig((yyvsp[(1) - (1)].str)); postSig("*");
         (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 2189 "vtkParse.y"
    { postSig("("); scopeSig((yyvsp[(1) - (1)].str)); postSig("&");
         (yyval.integer) = VTK_PARSE_REF; }
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 2193 "vtkParse.y"
    { postSig("("); scopeSig((yyvsp[(1) - (1)].str)); postSig("*");
         (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 2195 "vtkParse.y"
    { postSig("("); scopeSig((yyvsp[(1) - (1)].str)); postSig("&");
         (yyval.integer) = VTK_PARSE_REF; }
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 2198 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 2199 "vtkParse.y"
    { pushFunction(); postSig("("); }
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 2200 "vtkParse.y"
    { postSig(")"); }
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 2201 "vtkParse.y"
    {
      (yyval.integer) = VTK_PARSE_FUNCTION;
      popFunction();
    }
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 2205 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_ARRAY; }
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 2209 "vtkParse.y"
    { currentFunction->IsConst = 1; }
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 2215 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(1) - (2)].integer), (yyvsp[(2) - (2)].integer)); }
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 2220 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(1) - (2)].integer), (yyvsp[(2) - (2)].integer)); }
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 2223 "vtkParse.y"
    { clearVarName(); chopSig(); }
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 2227 "vtkParse.y"
    { setVarName((yyvsp[(1) - (1)].str)); }
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 2228 "vtkParse.y"
    { setVarName((yyvsp[(1) - (3)].str)); }
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 2236 "vtkParse.y"
    { clearArray(); }
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 2240 "vtkParse.y"
    { clearArray(); }
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 2247 "vtkParse.y"
    { postSig("["); }
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 2247 "vtkParse.y"
    { postSig("]"); }
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 2250 "vtkParse.y"
    { pushArraySize(""); }
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 2251 "vtkParse.y"
    { markSig(); }
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 2251 "vtkParse.y"
    { chopSig(); pushArraySize(copySig()); }
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 2267 "vtkParse.y"
    { (yyval.str) = vtkstrcat((yyvsp[(1) - (2)].str), (yyvsp[(2) - (2)].str)); }
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 2269 "vtkParse.y"
    { (yyval.str) = vtkstrcat((yyvsp[(1) - (2)].str), (yyvsp[(2) - (2)].str)); }
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 2271 "vtkParse.y"
    { (yyval.str) = vtkstrcat((yyvsp[(1) - (2)].str), (yyvsp[(2) - (2)].str)); }
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 2275 "vtkParse.y"
    { (yyval.str) = vtkstrcat((yyvsp[(1) - (2)].str), (yyvsp[(2) - (2)].str)); }
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 2277 "vtkParse.y"
    { (yyval.str) = vtkstrcat((yyvsp[(1) - (2)].str), (yyvsp[(2) - (2)].str)); }
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 2279 "vtkParse.y"
    { (yyval.str) = vtkstrcat3((yyvsp[(1) - (3)].str), (yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].str)); }
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 2281 "vtkParse.y"
    { (yyval.str) = vtkstrcat3((yyvsp[(1) - (3)].str), (yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].str)); }
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 2282 "vtkParse.y"
    { postSig("template "); }
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 2284 "vtkParse.y"
    { (yyval.str) = vtkstrcat4((yyvsp[(1) - (5)].str), "template ", (yyvsp[(4) - (5)].str), (yyvsp[(5) - (5)].str)); }
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 2287 "vtkParse.y"
    { postSig((yyvsp[(1) - (1)].str)); }
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 2290 "vtkParse.y"
    { (yyval.str) = "::"; postSig((yyval.str)); }
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 2293 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<"); }
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 2295 "vtkParse.y"
    {
      chopSig(); if (getSig()[getSigLength()-1] == '>') { postSig(" "); }
      postSig(">"); (yyval.str) = copySig(); clearTypeId();
    }
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 2308 "vtkParse.y"
    { postSig((yyvsp[(1) - (1)].str)); }
    break;

  case 294:

/* Line 1455 of yacc.c  */
#line 2309 "vtkParse.y"
    { postSig((yyvsp[(1) - (1)].str)); }
    break;

  case 295:

/* Line 1455 of yacc.c  */
#line 2310 "vtkParse.y"
    { postSig((yyvsp[(1) - (1)].str)); }
    break;

  case 296:

/* Line 1455 of yacc.c  */
#line 2311 "vtkParse.y"
    { postSig((yyvsp[(1) - (1)].str)); }
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 2312 "vtkParse.y"
    { postSig((yyvsp[(1) - (1)].str)); }
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 2313 "vtkParse.y"
    { postSig((yyvsp[(1) - (1)].str)); }
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 2314 "vtkParse.y"
    { postSig((yyvsp[(1) - (1)].str)); }
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 2315 "vtkParse.y"
    { (yyval.str) = vtkstrcat("~",(yyvsp[(2) - (2)].str)); postSig((yyval.str)); }
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 2316 "vtkParse.y"
    { (yyval.str) = vtkstrcat("~",(yyvsp[(2) - (2)].str)); postSig((yyval.str)); }
    break;

  case 302:

/* Line 1455 of yacc.c  */
#line 2317 "vtkParse.y"
    { (yyval.str) = vtkstrcat("~",(yyvsp[(2) - (2)].str)); postSig((yyval.str)); }
    break;

  case 303:

/* Line 1455 of yacc.c  */
#line 2318 "vtkParse.y"
    { (yyval.str) = vtkstrcat("~",(yyvsp[(2) - (2)].str)); postSig((yyval.str)); }
    break;

  case 304:

/* Line 1455 of yacc.c  */
#line 2319 "vtkParse.y"
    { (yyval.str) = vtkstrcat("~",(yyvsp[(2) - (2)].str)); postSig((yyval.str)); }
    break;

  case 305:

/* Line 1455 of yacc.c  */
#line 2320 "vtkParse.y"
    { (yyval.str) = vtkstrcat("~",(yyvsp[(2) - (2)].str)); postSig((yyval.str)); }
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 2321 "vtkParse.y"
    { (yyval.str) = vtkstrcat("~",(yyvsp[(2) - (2)].str)); postSig((yyval.str)); }
    break;

  case 307:

/* Line 1455 of yacc.c  */
#line 2322 "vtkParse.y"
    { (yyval.str) = "size_t"; postSig((yyval.str)); }
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 2323 "vtkParse.y"
    { (yyval.str) = "ssize_t"; postSig((yyval.str)); }
    break;

  case 309:

/* Line 1455 of yacc.c  */
#line 2324 "vtkParse.y"
    { (yyval.str) = "vtkTypeInt8"; postSig((yyval.str)); }
    break;

  case 310:

/* Line 1455 of yacc.c  */
#line 2325 "vtkParse.y"
    { (yyval.str) = "vtkTypeUInt8"; postSig((yyval.str)); }
    break;

  case 311:

/* Line 1455 of yacc.c  */
#line 2326 "vtkParse.y"
    { (yyval.str) = "vtkTypeInt16"; postSig((yyval.str)); }
    break;

  case 312:

/* Line 1455 of yacc.c  */
#line 2327 "vtkParse.y"
    { (yyval.str) = "vtkTypeUInt16"; postSig((yyval.str)); }
    break;

  case 313:

/* Line 1455 of yacc.c  */
#line 2328 "vtkParse.y"
    { (yyval.str) = "vtkTypeInt32"; postSig((yyval.str)); }
    break;

  case 314:

/* Line 1455 of yacc.c  */
#line 2329 "vtkParse.y"
    { (yyval.str) = "vtkTypeUInt32"; postSig((yyval.str)); }
    break;

  case 315:

/* Line 1455 of yacc.c  */
#line 2330 "vtkParse.y"
    { (yyval.str) = "vtkTypeInt64"; postSig((yyval.str)); }
    break;

  case 316:

/* Line 1455 of yacc.c  */
#line 2331 "vtkParse.y"
    { (yyval.str) = "vtkTypeUInt64"; postSig((yyval.str)); }
    break;

  case 317:

/* Line 1455 of yacc.c  */
#line 2332 "vtkParse.y"
    { (yyval.str) = "vtkTypeFloat32"; postSig((yyval.str)); }
    break;

  case 318:

/* Line 1455 of yacc.c  */
#line 2333 "vtkParse.y"
    { (yyval.str) = "vtkTypeFloat64"; postSig((yyval.str)); }
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 2334 "vtkParse.y"
    { (yyval.str) = "vtkIdType"; postSig((yyval.str)); }
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 2360 "vtkParse.y"
    { setTypeBase(buildTypeBase(getType(), (yyvsp[(1) - (1)].integer))); }
    break;

  case 331:

/* Line 1455 of yacc.c  */
#line 2361 "vtkParse.y"
    { setTypeMod(VTK_PARSE_TYPEDEF); }
    break;

  case 332:

/* Line 1455 of yacc.c  */
#line 2362 "vtkParse.y"
    { setTypeMod(VTK_PARSE_FRIEND); }
    break;

  case 335:

/* Line 1455 of yacc.c  */
#line 2369 "vtkParse.y"
    { setTypeMod((yyvsp[(1) - (1)].integer)); }
    break;

  case 336:

/* Line 1455 of yacc.c  */
#line 2370 "vtkParse.y"
    { setTypeMod((yyvsp[(1) - (1)].integer)); }
    break;

  case 337:

/* Line 1455 of yacc.c  */
#line 2371 "vtkParse.y"
    { setTypeMod((yyvsp[(1) - (1)].integer)); }
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 2374 "vtkParse.y"
    { postSig("mutable "); (yyval.integer) = VTK_PARSE_MUTABLE; }
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 2375 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 2376 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 341:

/* Line 1455 of yacc.c  */
#line 2377 "vtkParse.y"
    { postSig("static "); (yyval.integer) = VTK_PARSE_STATIC; }
    break;

  case 342:

/* Line 1455 of yacc.c  */
#line 2380 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 343:

/* Line 1455 of yacc.c  */
#line 2381 "vtkParse.y"
    { postSig("virtual "); (yyval.integer) = VTK_PARSE_VIRTUAL; }
    break;

  case 344:

/* Line 1455 of yacc.c  */
#line 2382 "vtkParse.y"
    { postSig("explicit "); (yyval.integer) = VTK_PARSE_EXPLICIT; }
    break;

  case 345:

/* Line 1455 of yacc.c  */
#line 2385 "vtkParse.y"
    { postSig("const "); (yyval.integer) = VTK_PARSE_CONST; }
    break;

  case 346:

/* Line 1455 of yacc.c  */
#line 2386 "vtkParse.y"
    { postSig("volatile "); (yyval.integer) = VTK_PARSE_VOLATILE; }
    break;

  case 348:

/* Line 1455 of yacc.c  */
#line 2391 "vtkParse.y"
    { (yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer)); }
    break;

  case 350:

/* Line 1455 of yacc.c  */
#line 2402 "vtkParse.y"
    { setTypeBase((yyvsp[(1) - (1)].integer)); }
    break;

  case 352:

/* Line 1455 of yacc.c  */
#line 2404 "vtkParse.y"
    { setTypeBase((yyvsp[(2) - (2)].integer)); }
    break;

  case 355:

/* Line 1455 of yacc.c  */
#line 2409 "vtkParse.y"
    { postSig("typename "); }
    break;

  case 356:

/* Line 1455 of yacc.c  */
#line 2410 "vtkParse.y"
    { postSig(" "); setTypeId((yyvsp[(3) - (3)].str)); (yyval.integer) = guess_id_type((yyvsp[(3) - (3)].str)); }
    break;

  case 357:

/* Line 1455 of yacc.c  */
#line 2412 "vtkParse.y"
    { postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = guess_id_type((yyvsp[(1) - (1)].str)); }
    break;

  case 358:

/* Line 1455 of yacc.c  */
#line 2414 "vtkParse.y"
    { postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = guess_id_type((yyvsp[(1) - (1)].str)); }
    break;

  case 359:

/* Line 1455 of yacc.c  */
#line 2416 "vtkParse.y"
    { postSig(" "); setTypeId((yyvsp[(2) - (2)].str)); (yyval.integer) = guess_id_type((yyvsp[(2) - (2)].str)); }
    break;

  case 360:

/* Line 1455 of yacc.c  */
#line 2418 "vtkParse.y"
    { postSig(" "); setTypeId((yyvsp[(2) - (2)].str)); (yyval.integer) = guess_id_type((yyvsp[(2) - (2)].str)); }
    break;

  case 362:

/* Line 1455 of yacc.c  */
#line 2424 "vtkParse.y"
    { setTypeBase((yyvsp[(1) - (1)].integer)); }
    break;

  case 364:

/* Line 1455 of yacc.c  */
#line 2426 "vtkParse.y"
    { setTypeBase((yyvsp[(2) - (2)].integer)); }
    break;

  case 367:

/* Line 1455 of yacc.c  */
#line 2432 "vtkParse.y"
    { postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = guess_id_type((yyvsp[(1) - (1)].str)); }
    break;

  case 368:

/* Line 1455 of yacc.c  */
#line 2434 "vtkParse.y"
    { postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = guess_id_type((yyvsp[(1) - (1)].str)); }
    break;

  case 369:

/* Line 1455 of yacc.c  */
#line 2436 "vtkParse.y"
    { postSig(" "); setTypeId((yyvsp[(2) - (2)].str)); (yyval.integer) = guess_id_type((yyvsp[(2) - (2)].str)); }
    break;

  case 370:

/* Line 1455 of yacc.c  */
#line 2438 "vtkParse.y"
    { postSig(" "); setTypeId((yyvsp[(2) - (2)].str)); (yyval.integer) = guess_id_type((yyvsp[(2) - (2)].str)); }
    break;

  case 371:

/* Line 1455 of yacc.c  */
#line 2440 "vtkParse.y"
    { postSig(" "); setTypeId((yyvsp[(2) - (2)].str)); (yyval.integer) = guess_id_type((yyvsp[(2) - (2)].str)); }
    break;

  case 372:

/* Line 1455 of yacc.c  */
#line 2443 "vtkParse.y"
    { setTypeId(""); }
    break;

  case 374:

/* Line 1455 of yacc.c  */
#line 2447 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_STRING; }
    break;

  case 375:

/* Line 1455 of yacc.c  */
#line 2448 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNICODE_STRING;}
    break;

  case 376:

/* Line 1455 of yacc.c  */
#line 2449 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_OSTREAM; }
    break;

  case 377:

/* Line 1455 of yacc.c  */
#line 2450 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_ISTREAM; }
    break;

  case 378:

/* Line 1455 of yacc.c  */
#line 2451 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 379:

/* Line 1455 of yacc.c  */
#line 2452 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_OBJECT; }
    break;

  case 380:

/* Line 1455 of yacc.c  */
#line 2453 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_QOBJECT; }
    break;

  case 381:

/* Line 1455 of yacc.c  */
#line 2454 "vtkParse.y"
    { typeSig("ssize_t"); (yyval.integer) = VTK_PARSE_SSIZE_T; }
    break;

  case 382:

/* Line 1455 of yacc.c  */
#line 2455 "vtkParse.y"
    { typeSig("size_t"); (yyval.integer) = VTK_PARSE_SIZE_T; }
    break;

  case 383:

/* Line 1455 of yacc.c  */
#line 2456 "vtkParse.y"
    { typeSig("vtkTypeInt8"); (yyval.integer) = VTK_PARSE_INT8; }
    break;

  case 384:

/* Line 1455 of yacc.c  */
#line 2457 "vtkParse.y"
    { typeSig("vtkTypeUInt8"); (yyval.integer) = VTK_PARSE_UINT8; }
    break;

  case 385:

/* Line 1455 of yacc.c  */
#line 2458 "vtkParse.y"
    { typeSig("vtkTypeInt16"); (yyval.integer) = VTK_PARSE_INT16; }
    break;

  case 386:

/* Line 1455 of yacc.c  */
#line 2459 "vtkParse.y"
    { typeSig("vtkTypeUInt16"); (yyval.integer) = VTK_PARSE_UINT16; }
    break;

  case 387:

/* Line 1455 of yacc.c  */
#line 2460 "vtkParse.y"
    { typeSig("vtkTypeInt32"); (yyval.integer) = VTK_PARSE_INT32; }
    break;

  case 388:

/* Line 1455 of yacc.c  */
#line 2461 "vtkParse.y"
    { typeSig("vtkTypeUInt32"); (yyval.integer) = VTK_PARSE_UINT32; }
    break;

  case 389:

/* Line 1455 of yacc.c  */
#line 2462 "vtkParse.y"
    { typeSig("vtkTypeInt64"); (yyval.integer) = VTK_PARSE_INT64; }
    break;

  case 390:

/* Line 1455 of yacc.c  */
#line 2463 "vtkParse.y"
    { typeSig("vtkTypeUInt64"); (yyval.integer) = VTK_PARSE_UINT64; }
    break;

  case 391:

/* Line 1455 of yacc.c  */
#line 2464 "vtkParse.y"
    { typeSig("vtkTypeFloat32"); (yyval.integer) = VTK_PARSE_FLOAT32; }
    break;

  case 392:

/* Line 1455 of yacc.c  */
#line 2465 "vtkParse.y"
    { typeSig("vtkTypeFloat64"); (yyval.integer) = VTK_PARSE_FLOAT64; }
    break;

  case 393:

/* Line 1455 of yacc.c  */
#line 2466 "vtkParse.y"
    { typeSig("vtkIdType"); (yyval.integer) = VTK_PARSE_ID_TYPE; }
    break;

  case 394:

/* Line 1455 of yacc.c  */
#line 2469 "vtkParse.y"
    { postSig("void "); (yyval.integer) = VTK_PARSE_VOID; }
    break;

  case 395:

/* Line 1455 of yacc.c  */
#line 2470 "vtkParse.y"
    { postSig("bool "); (yyval.integer) = VTK_PARSE_BOOL; }
    break;

  case 396:

/* Line 1455 of yacc.c  */
#line 2471 "vtkParse.y"
    { postSig("float "); (yyval.integer) = VTK_PARSE_FLOAT; }
    break;

  case 397:

/* Line 1455 of yacc.c  */
#line 2472 "vtkParse.y"
    { postSig("double "); (yyval.integer) = VTK_PARSE_DOUBLE; }
    break;

  case 398:

/* Line 1455 of yacc.c  */
#line 2473 "vtkParse.y"
    { postSig("char "); (yyval.integer) = VTK_PARSE_CHAR; }
    break;

  case 399:

/* Line 1455 of yacc.c  */
#line 2474 "vtkParse.y"
    { postSig("int "); (yyval.integer) = VTK_PARSE_INT; }
    break;

  case 400:

/* Line 1455 of yacc.c  */
#line 2475 "vtkParse.y"
    { postSig("short "); (yyval.integer) = VTK_PARSE_SHORT; }
    break;

  case 401:

/* Line 1455 of yacc.c  */
#line 2476 "vtkParse.y"
    { postSig("long "); (yyval.integer) = VTK_PARSE_LONG; }
    break;

  case 402:

/* Line 1455 of yacc.c  */
#line 2477 "vtkParse.y"
    { postSig("__int64 "); (yyval.integer) = VTK_PARSE___INT64; }
    break;

  case 403:

/* Line 1455 of yacc.c  */
#line 2478 "vtkParse.y"
    { postSig("signed "); (yyval.integer) = VTK_PARSE_INT; }
    break;

  case 404:

/* Line 1455 of yacc.c  */
#line 2479 "vtkParse.y"
    { postSig("unsigned "); (yyval.integer) = VTK_PARSE_UNSIGNED_INT; }
    break;

  case 407:

/* Line 1455 of yacc.c  */
#line 2501 "vtkParse.y"
    { (yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer)); }
    break;

  case 408:

/* Line 1455 of yacc.c  */
#line 2504 "vtkParse.y"
    { postSig("&"); (yyval.integer) = VTK_PARSE_REF; }
    break;

  case 409:

/* Line 1455 of yacc.c  */
#line 2507 "vtkParse.y"
    { postSig("*"); (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 410:

/* Line 1455 of yacc.c  */
#line 2508 "vtkParse.y"
    { postSig("*"); }
    break;

  case 411:

/* Line 1455 of yacc.c  */
#line 2509 "vtkParse.y"
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
#line 2525 "vtkParse.y"
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
#line 2541 "vtkParse.y"
    {preSig("void Set"); postSig("(");}
    break;

  case 415:

/* Line 1455 of yacc.c  */
#line 2542 "vtkParse.y"
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
#line 2551 "vtkParse.y"
    {postSig("Get");}
    break;

  case 417:

/* Line 1455 of yacc.c  */
#line 2552 "vtkParse.y"
    {markSig();}
    break;

  case 418:

/* Line 1455 of yacc.c  */
#line 2552 "vtkParse.y"
    {swapSig();}
    break;

  case 419:

/* Line 1455 of yacc.c  */
#line 2553 "vtkParse.y"
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
#line 2561 "vtkParse.y"
    {preSig("void Set");}
    break;

  case 421:

/* Line 1455 of yacc.c  */
#line 2562 "vtkParse.y"
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
#line 2571 "vtkParse.y"
    {preSig("char *Get");}
    break;

  case 423:

/* Line 1455 of yacc.c  */
#line 2572 "vtkParse.y"
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
#line 2580 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 425:

/* Line 1455 of yacc.c  */
#line 2580 "vtkParse.y"
    {closeSig();}
    break;

  case 426:

/* Line 1455 of yacc.c  */
#line 2582 "vtkParse.y"
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
#line 2613 "vtkParse.y"
    {preSig("void Set"); postSig("("); }
    break;

  case 428:

/* Line 1455 of yacc.c  */
#line 2614 "vtkParse.y"
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
#line 2623 "vtkParse.y"
    {postSig("*Get");}
    break;

  case 430:

/* Line 1455 of yacc.c  */
#line 2624 "vtkParse.y"
    {markSig();}
    break;

  case 431:

/* Line 1455 of yacc.c  */
#line 2624 "vtkParse.y"
    {swapSig();}
    break;

  case 432:

/* Line 1455 of yacc.c  */
#line 2625 "vtkParse.y"
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
#line 2634 "vtkParse.y"
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
#line 2651 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 435:

/* Line 1455 of yacc.c  */
#line 2652 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), getType(), copySig(), 2);
   }
    break;

  case 436:

/* Line 1455 of yacc.c  */
#line 2656 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 437:

/* Line 1455 of yacc.c  */
#line 2657 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), getType(), copySig(), 2);
   }
    break;

  case 438:

/* Line 1455 of yacc.c  */
#line 2661 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 439:

/* Line 1455 of yacc.c  */
#line 2662 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), getType(), copySig(), 3);
   }
    break;

  case 440:

/* Line 1455 of yacc.c  */
#line 2666 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 441:

/* Line 1455 of yacc.c  */
#line 2667 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), getType(), copySig(), 3);
   }
    break;

  case 442:

/* Line 1455 of yacc.c  */
#line 2671 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 443:

/* Line 1455 of yacc.c  */
#line 2672 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), getType(), copySig(), 4);
   }
    break;

  case 444:

/* Line 1455 of yacc.c  */
#line 2676 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 445:

/* Line 1455 of yacc.c  */
#line 2677 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), getType(), copySig(), 4);
   }
    break;

  case 446:

/* Line 1455 of yacc.c  */
#line 2681 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 447:

/* Line 1455 of yacc.c  */
#line 2682 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), getType(), copySig(), 6);
   }
    break;

  case 448:

/* Line 1455 of yacc.c  */
#line 2686 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 449:

/* Line 1455 of yacc.c  */
#line 2687 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), getType(), copySig(), 6);
   }
    break;

  case 450:

/* Line 1455 of yacc.c  */
#line 2691 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 451:

/* Line 1455 of yacc.c  */
#line 2693 "vtkParse.y"
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
#line 2708 "vtkParse.y"
    {startSig();}
    break;

  case 453:

/* Line 1455 of yacc.c  */
#line 2710 "vtkParse.y"
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
#line 2723 "vtkParse.y"
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
#line 2760 "vtkParse.y"
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
#line 2798 "vtkParse.y"
    {
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
    break;

  case 459:

/* Line 1455 of yacc.c  */
#line 2842 "vtkParse.y"
    { (yyval.str) = "()"; }
    break;

  case 460:

/* Line 1455 of yacc.c  */
#line 2843 "vtkParse.y"
    { (yyval.str) = "[]"; }
    break;

  case 461:

/* Line 1455 of yacc.c  */
#line 2844 "vtkParse.y"
    { (yyval.str) = " new[]"; }
    break;

  case 462:

/* Line 1455 of yacc.c  */
#line 2845 "vtkParse.y"
    { (yyval.str) = " delete[]"; }
    break;

  case 463:

/* Line 1455 of yacc.c  */
#line 2846 "vtkParse.y"
    { (yyval.str) = "<"; }
    break;

  case 464:

/* Line 1455 of yacc.c  */
#line 2847 "vtkParse.y"
    { (yyval.str) = ">"; }
    break;

  case 465:

/* Line 1455 of yacc.c  */
#line 2848 "vtkParse.y"
    { (yyval.str) = ","; }
    break;

  case 466:

/* Line 1455 of yacc.c  */
#line 2849 "vtkParse.y"
    { (yyval.str) = "="; }
    break;

  case 468:

/* Line 1455 of yacc.c  */
#line 2853 "vtkParse.y"
    { (yyval.str) = "%"; }
    break;

  case 469:

/* Line 1455 of yacc.c  */
#line 2854 "vtkParse.y"
    { (yyval.str) = "*"; }
    break;

  case 470:

/* Line 1455 of yacc.c  */
#line 2855 "vtkParse.y"
    { (yyval.str) = "/"; }
    break;

  case 471:

/* Line 1455 of yacc.c  */
#line 2856 "vtkParse.y"
    { (yyval.str) = "-"; }
    break;

  case 472:

/* Line 1455 of yacc.c  */
#line 2857 "vtkParse.y"
    { (yyval.str) = "+"; }
    break;

  case 473:

/* Line 1455 of yacc.c  */
#line 2858 "vtkParse.y"
    { (yyval.str) = "!"; }
    break;

  case 474:

/* Line 1455 of yacc.c  */
#line 2859 "vtkParse.y"
    { (yyval.str) = "~"; }
    break;

  case 475:

/* Line 1455 of yacc.c  */
#line 2860 "vtkParse.y"
    { (yyval.str) = "&"; }
    break;

  case 476:

/* Line 1455 of yacc.c  */
#line 2861 "vtkParse.y"
    { (yyval.str) = "|"; }
    break;

  case 477:

/* Line 1455 of yacc.c  */
#line 2862 "vtkParse.y"
    { (yyval.str) = "^"; }
    break;

  case 478:

/* Line 1455 of yacc.c  */
#line 2863 "vtkParse.y"
    { (yyval.str) = " new"; }
    break;

  case 479:

/* Line 1455 of yacc.c  */
#line 2864 "vtkParse.y"
    { (yyval.str) = " delete"; }
    break;

  case 480:

/* Line 1455 of yacc.c  */
#line 2865 "vtkParse.y"
    { (yyval.str) = "<<="; }
    break;

  case 481:

/* Line 1455 of yacc.c  */
#line 2866 "vtkParse.y"
    { (yyval.str) = ">>="; }
    break;

  case 482:

/* Line 1455 of yacc.c  */
#line 2867 "vtkParse.y"
    { (yyval.str) = "<<"; }
    break;

  case 483:

/* Line 1455 of yacc.c  */
#line 2868 "vtkParse.y"
    { (yyval.str) = ">>"; }
    break;

  case 484:

/* Line 1455 of yacc.c  */
#line 2869 "vtkParse.y"
    { (yyval.str) = ".*"; }
    break;

  case 485:

/* Line 1455 of yacc.c  */
#line 2870 "vtkParse.y"
    { (yyval.str) = "->*"; }
    break;

  case 486:

/* Line 1455 of yacc.c  */
#line 2871 "vtkParse.y"
    { (yyval.str) = "->"; }
    break;

  case 487:

/* Line 1455 of yacc.c  */
#line 2872 "vtkParse.y"
    { (yyval.str) = "+="; }
    break;

  case 488:

/* Line 1455 of yacc.c  */
#line 2873 "vtkParse.y"
    { (yyval.str) = "-="; }
    break;

  case 489:

/* Line 1455 of yacc.c  */
#line 2874 "vtkParse.y"
    { (yyval.str) = "*="; }
    break;

  case 490:

/* Line 1455 of yacc.c  */
#line 2875 "vtkParse.y"
    { (yyval.str) = "/="; }
    break;

  case 491:

/* Line 1455 of yacc.c  */
#line 2876 "vtkParse.y"
    { (yyval.str) = "%="; }
    break;

  case 492:

/* Line 1455 of yacc.c  */
#line 2877 "vtkParse.y"
    { (yyval.str) = "++"; }
    break;

  case 493:

/* Line 1455 of yacc.c  */
#line 2878 "vtkParse.y"
    { (yyval.str) = "--"; }
    break;

  case 494:

/* Line 1455 of yacc.c  */
#line 2879 "vtkParse.y"
    { (yyval.str) = "&="; }
    break;

  case 495:

/* Line 1455 of yacc.c  */
#line 2880 "vtkParse.y"
    { (yyval.str) = "|="; }
    break;

  case 496:

/* Line 1455 of yacc.c  */
#line 2881 "vtkParse.y"
    { (yyval.str) = "^="; }
    break;

  case 497:

/* Line 1455 of yacc.c  */
#line 2882 "vtkParse.y"
    { (yyval.str) = "&&"; }
    break;

  case 498:

/* Line 1455 of yacc.c  */
#line 2883 "vtkParse.y"
    { (yyval.str) = "||"; }
    break;

  case 499:

/* Line 1455 of yacc.c  */
#line 2884 "vtkParse.y"
    { (yyval.str) = "=="; }
    break;

  case 500:

/* Line 1455 of yacc.c  */
#line 2885 "vtkParse.y"
    { (yyval.str) = "!="; }
    break;

  case 501:

/* Line 1455 of yacc.c  */
#line 2886 "vtkParse.y"
    { (yyval.str) = "<="; }
    break;

  case 502:

/* Line 1455 of yacc.c  */
#line 2887 "vtkParse.y"
    { (yyval.str) = ">="; }
    break;

  case 503:

/* Line 1455 of yacc.c  */
#line 2890 "vtkParse.y"
    { (yyval.str) = "typedef"; }
    break;

  case 504:

/* Line 1455 of yacc.c  */
#line 2891 "vtkParse.y"
    { (yyval.str) = "typename"; }
    break;

  case 505:

/* Line 1455 of yacc.c  */
#line 2892 "vtkParse.y"
    { (yyval.str) = "class"; }
    break;

  case 506:

/* Line 1455 of yacc.c  */
#line 2893 "vtkParse.y"
    { (yyval.str) = "struct"; }
    break;

  case 507:

/* Line 1455 of yacc.c  */
#line 2894 "vtkParse.y"
    { (yyval.str) = "union"; }
    break;

  case 508:

/* Line 1455 of yacc.c  */
#line 2895 "vtkParse.y"
    { (yyval.str) = "template"; }
    break;

  case 509:

/* Line 1455 of yacc.c  */
#line 2896 "vtkParse.y"
    { (yyval.str) = "public"; }
    break;

  case 510:

/* Line 1455 of yacc.c  */
#line 2897 "vtkParse.y"
    { (yyval.str) = "protected"; }
    break;

  case 511:

/* Line 1455 of yacc.c  */
#line 2898 "vtkParse.y"
    { (yyval.str) = "private"; }
    break;

  case 512:

/* Line 1455 of yacc.c  */
#line 2899 "vtkParse.y"
    { (yyval.str) = "const"; }
    break;

  case 513:

/* Line 1455 of yacc.c  */
#line 2900 "vtkParse.y"
    { (yyval.str) = "static"; }
    break;

  case 514:

/* Line 1455 of yacc.c  */
#line 2901 "vtkParse.y"
    { (yyval.str) = "inline"; }
    break;

  case 515:

/* Line 1455 of yacc.c  */
#line 2902 "vtkParse.y"
    { (yyval.str) = "virtual"; }
    break;

  case 516:

/* Line 1455 of yacc.c  */
#line 2903 "vtkParse.y"
    { (yyval.str) = "extern"; }
    break;

  case 517:

/* Line 1455 of yacc.c  */
#line 2904 "vtkParse.y"
    { (yyval.str) = "namespace"; }
    break;

  case 518:

/* Line 1455 of yacc.c  */
#line 2905 "vtkParse.y"
    { (yyval.str) = "operator"; }
    break;

  case 519:

/* Line 1455 of yacc.c  */
#line 2906 "vtkParse.y"
    { (yyval.str) = "enum"; }
    break;

  case 520:

/* Line 1455 of yacc.c  */
#line 2907 "vtkParse.y"
    { (yyval.str) = "throw"; }
    break;

  case 521:

/* Line 1455 of yacc.c  */
#line 2908 "vtkParse.y"
    { (yyval.str) = "const_cast"; }
    break;

  case 522:

/* Line 1455 of yacc.c  */
#line 2909 "vtkParse.y"
    { (yyval.str) = "dynamic_cast"; }
    break;

  case 523:

/* Line 1455 of yacc.c  */
#line 2910 "vtkParse.y"
    { (yyval.str) = "static_cast"; }
    break;

  case 524:

/* Line 1455 of yacc.c  */
#line 2911 "vtkParse.y"
    { (yyval.str) = "reinterpret_cast"; }
    break;

  case 537:

/* Line 1455 of yacc.c  */
#line 2935 "vtkParse.y"
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
#line 2965 "vtkParse.y"
    { postSig(":"); postSig(" "); }
    break;

  case 539:

/* Line 1455 of yacc.c  */
#line 2965 "vtkParse.y"
    { postSig("."); }
    break;

  case 540:

/* Line 1455 of yacc.c  */
#line 2966 "vtkParse.y"
    { chopSig(); postSig("::"); }
    break;

  case 541:

/* Line 1455 of yacc.c  */
#line 2967 "vtkParse.y"
    { postSig((yyvsp[(1) - (1)].str)); postSig(" "); }
    break;

  case 542:

/* Line 1455 of yacc.c  */
#line 2968 "vtkParse.y"
    { postSig((yyvsp[(1) - (1)].str)); postSig(" "); }
    break;

  case 544:

/* Line 1455 of yacc.c  */
#line 2971 "vtkParse.y"
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
#line 2997 "vtkParse.y"
    { postSig("< "); }
    break;

  case 549:

/* Line 1455 of yacc.c  */
#line 2998 "vtkParse.y"
    { postSig("> "); }
    break;

  case 551:

/* Line 1455 of yacc.c  */
#line 3001 "vtkParse.y"
    { postSig("= "); }
    break;

  case 552:

/* Line 1455 of yacc.c  */
#line 3002 "vtkParse.y"
    { chopSig(); postSig(", "); }
    break;

  case 554:

/* Line 1455 of yacc.c  */
#line 3005 "vtkParse.y"
    { chopSig(); postSig(";"); }
    break;

  case 562:

/* Line 1455 of yacc.c  */
#line 3019 "vtkParse.y"
    { postSig("= "); }
    break;

  case 563:

/* Line 1455 of yacc.c  */
#line 3020 "vtkParse.y"
    { chopSig(); postSig(", "); }
    break;

  case 564:

/* Line 1455 of yacc.c  */
#line 3024 "vtkParse.y"
    {
      chopSig();
      if (getSig()[getSigLength()-1] == '<') { postSig(" "); }
      postSig("<");
    }
    break;

  case 565:

/* Line 1455 of yacc.c  */
#line 3030 "vtkParse.y"
    {
      chopSig();
      if (getSig()[getSigLength()-1] == '>') { postSig(" "); }
      postSig("> ");
    }
    break;

  case 566:

/* Line 1455 of yacc.c  */
#line 3037 "vtkParse.y"
    { postSig("["); }
    break;

  case 567:

/* Line 1455 of yacc.c  */
#line 3038 "vtkParse.y"
    { chopSig(); postSig("] "); }
    break;

  case 568:

/* Line 1455 of yacc.c  */
#line 3041 "vtkParse.y"
    { postSig("("); }
    break;

  case 569:

/* Line 1455 of yacc.c  */
#line 3042 "vtkParse.y"
    { chopSig(); postSig(") "); }
    break;

  case 570:

/* Line 1455 of yacc.c  */
#line 3043 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("*"); }
    break;

  case 571:

/* Line 1455 of yacc.c  */
#line 3044 "vtkParse.y"
    { chopSig(); postSig(") "); }
    break;

  case 572:

/* Line 1455 of yacc.c  */
#line 3045 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("&"); }
    break;

  case 573:

/* Line 1455 of yacc.c  */
#line 3046 "vtkParse.y"
    { chopSig(); postSig(") "); }
    break;

  case 574:

/* Line 1455 of yacc.c  */
#line 3049 "vtkParse.y"
    { postSig("{ "); }
    break;

  case 575:

/* Line 1455 of yacc.c  */
#line 3049 "vtkParse.y"
    { postSig("} "); }
    break;



/* Line 1455 of yacc.c  */
#line 7941 "vtkParse.tab.c"
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
#line 3088 "vtkParse.y"

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
