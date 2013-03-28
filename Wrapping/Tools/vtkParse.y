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
pointers, etc.  The parser also creates a typeId string, which is either
a simple id that gives the class name or type name, or is "function" for
function pointer types, or "method" for method pointer types.
*/

/*
Conformance Notes:

This parser was designed empirically and incrementally.  It has been
refactored to make it more similar to the C++ 1998 grammar, but there
are still many very significant differences.

The most significant difference between this parser and a "standard"
parser is that it only parses declarations in detail.  All other
statements and expressions are parsed as arbitrary sequences of symbols,
without any syntactic analysis.

The "unqualified_id" does not directly include "operator_function_id" or
"conversion_function_id" (e.g. ids like "operator=" or "operator int*").
Instead, these two id types are used to allow operator functions to be
handled by their own rules, rather than by the generic function rules.
These ids can only be used in function declarations and using declarations.

Types are handled quite differently from the C++ BNF.  These differences
represent a prolonged (and ultimately successful) attempt to empirically
create a yacc parser without any shift/reduce conflicts.  The rules for
types are organized according to the way that types are usually defined
in working code, rather than strictly according to C++ grammar.

The declaration specifiers "friend" and "typedef" can only appear at the
beginning of a declaration sequence.  There are also restrictions on
where class and enum specifiers can be used: you can declare a new struct
within a variable declaration, but not within a parameter declaration.

The lexer returns each of "(scope::*", "(*", "(a::b::*", etc. as single
tokens.  The C++ BNF, in contrast, would consider these to be a "("
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

One ambiguous structure that has been found in some working code, but
is currently not dealt with properly by the parser, is the following:

  enum { x = mytemplate<int,2>::x };

This is interpreted as the following ungrammatical statement:

  enum { x = mytemplate < int ,
         2 > ::x };

This has proven to be very hard to fix in the parser, but it possible
to modify the statement so that it does not confuse the parser:

  enum { x = (mytemplate<int,2>::x) };

The parentheses serve to disambiguate the statement.

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
%token <str> StdString
%token <str> UnicodeString
%token <str> OSTREAM
%token <str> ISTREAM

/* LP = "(*" or "(name::*"    and     LA = "(&" or "(name::&"
   These evaluate to an empty string or to "name::" as a string.
   Without these, the rules for declaring function pointers will
   produce errors because the parser cannot unambiguously choose
   between evaluating ID tokens as names via simple_id, versus
   evaluating ID tokens as types via type_name.  This construct forces
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
%token VOLATILE
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
%token INT
%token SHORT
%token LONG
%token INT64__
%token CHAR
%token SIGNED
%token UNSIGNED

/* typedef types */
%token SSIZE_T
%token SIZE_T

/* VTK typedef types */
%token IdType
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

translation_unit:
    opt_declaration_seq

opt_declaration_seq:
  | opt_declaration_seq
    {
      startSig();
      clearType();
      clearTypeId();
      clearTemplate();
      closeComment();
    }
    declaration

declaration:
    using_directive
  | using_declaration
  | forward_declaration
  | namespace_definition
  | namespace_alias_definition
  | linkage_specification
  | typedef_declaration
  | variable_declaration
  | enum_definition
  | class_definition
  | function_definition
  | template_declaration
  | declaration_macro
  | id_expression ';'
  | ';'

template_declaration:
    template_head class_definition
  | template_head function_definition
  | template_head nested_variable_initialization
  | template_head template_declaration

/*
 * extern section is parsed, but "extern" is ignored
 */

linkage_specification:
    EXTERN STRING_LITERAL '{' opt_declaration_seq '}'

/*
 * Namespace is pushed and its body is parsed
 */

namespace_definition:
    NAMESPACE '{' ignored_items '}'
  | NAMESPACE identifier { pushNamespace($<str>2); }
    '{' opt_declaration_seq '}' { popNamespace(); }

namespace_alias_definition:
    NAMESPACE identifier '=' qualified_id ';'

/*
 * Class definitions
 */

forward_declaration:
    simple_forward_declaration
  | template_head simple_forward_declaration

simple_forward_declaration:
    class_key id_expression ';'
  | decl_specifier_seq class_key id_expression ';'

class_definition:
    class_specifier opt_decl_specifier_seq opt_declarator_list ';'
  | decl_specifier_seq class_specifier opt_decl_specifier_seq opt_declarator_list ';'

class_specifier:
    class_head { pushType(); } '{' member_specification '}'
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

class_head:
    class_key id_expression { start_class($<str>2, $<integer>1); }
    opt_base_clause
  | class_key { start_class(NULL, $<integer>1); }
    opt_base_clause

class_key:
    CLASS { $<integer>$ = 0; }
  | STRUCT { $<integer>$ = 1; }
  | UNION { $<integer>$ = 2; }

member_specification:
  | member_specification
    {
      startSig();
      clearType();
      clearTypeId();
      clearTemplate();
      closeComment();
    }
    member_declaration
  | member_specification
    member_access_specifier ':'

member_access_specifier:
    PUBLIC { access_level = VTK_ACCESS_PUBLIC; }
  | PRIVATE { access_level = VTK_ACCESS_PRIVATE; }
  | PROTECTED { access_level = VTK_ACCESS_PROTECTED; }

member_declaration:
    using_declaration
  | forward_declaration
  | friend_declaration
  | typedef_declaration
  | variable_declaration
  | enum_definition
  | class_definition
  | method_definition
  | template_member_declaration
  | declaration_macro
  | VTK_BYTE_SWAP_DECL ignored_parentheses
  | id_expression ';'
  | ';'

template_member_declaration:
    template_head class_definition
  | template_head method_definition

friend_declaration:
    FRIEND ignored_class
  | FRIEND template_head ignored_class
  | FRIEND forward_declaration
  | FRIEND method_declaration function_body { output_friend_function(); }

opt_base_clause:
  | base_clause

base_clause:
    ':' base_specifier_list

base_specifier_list:
    base_specifier
  | base_specifier_list ',' base_specifier

base_specifier:
    id_expression
    { add_base_class(currentClass, $<str>1, access_level, 0); }
  | VIRTUAL opt_access_specifier id_expression
    { add_base_class(currentClass, $<str>3, $<integer>2, 1); }
  | access_specifier opt_virtual id_expression
    { add_base_class(currentClass, $<str>3, $<integer>1, $<integer>2); }

opt_virtual:
    { $<integer>$ = 0; }
  | VIRTUAL { $<integer>$ = 1; }

opt_access_specifier:
    { $<integer>$ = access_level; }
  | access_specifier

access_specifier:
    PUBLIC { $<integer>$ = VTK_ACCESS_PUBLIC; }
  | PRIVATE { $<integer>$ = VTK_ACCESS_PRIVATE; }
  | PROTECTED { $<integer>$ = VTK_ACCESS_PROTECTED; }

/*
 * Enums
 *
 * The values assigned to enum constants are handled as strings.
 * The text can be dropped into the generated .cxx file and evaluated there,
 * as long as all IDs are properly scoped.
 */

enum_definition:
    enum_specifier opt_decl_specifier_seq opt_declarator_list ';'
  | decl_specifier_seq enum_specifier opt_decl_specifier_seq
    opt_declarator_list ';'

enum_specifier:
    enum_head '{' { pushType(); start_enum($<str>1); } opt_enumerator_list '}'
    {
      popType();
      clearTypeId();
      if ($<str>1 != NULL)
        {
        setTypeId($<str>1);
        setTypeBase(guess_id_type($<str>1));
        }
      end_enum();
    }

enum_head:
    ENUM id_expression { $<str>$ = $<str>2; }
  | ENUM { $<str>$ = NULL; }

opt_enumerator_list:
  | enumerator_list

enumerator_list:
    enumerator_definition
  | enumerator_list ','
  | enumerator_list ',' enumerator_definition

enumerator_definition:
    simple_id { add_enum($<str>1, NULL); }
  | simple_id '=' { postSig("="); markSig(); }
    constant_expression { chopSig(); add_enum($<str>1, copySig()); }

/*
 * currently ignored items
 */

nested_variable_initialization:
    store_type nested_name_specifier simple_id '=' ignored_expression ';'

ignored_class:
    class_key id_expression ignored_class_body
  | decl_specifier_seq class_key id_expression ignored_class_body
  | class_key ignored_class_body
  | decl_specifier_seq class_key ignored_class_body

ignored_class_body:
    '{' ignored_items '}' ignored_expression ';'
  | ':' ignored_expression ';'


/*
 * Typedefs
 */

typedef_declaration:
    basic_typedef_declaration
  | decl_specifier_seq basic_typedef_declaration

basic_typedef_declaration:
    TYPEDEF store_type typedef_declarator_id typedef_declarator_list_cont ';'
  | TYPEDEF class_specifier
    opt_decl_specifier_seq typedef_declarator_list ';'
  | TYPEDEF decl_specifier_seq class_specifier
    opt_decl_specifier_seq typedef_declarator_list ';'
  | TYPEDEF enum_specifier
    opt_decl_specifier_seq typedef_declarator_list ';'
  | TYPEDEF decl_specifier_seq enum_specifier
    opt_decl_specifier_seq typedef_declarator_list ';'

typedef_declarator_list:
    typedef_declarator typedef_declarator_list_cont

typedef_declarator_list_cont:
  | typedef_declarator_list_cont ',' typedef_declarator

typedef_declarator:
    opt_ptr_operator_seq typedef_declarator_id

typedef_direct_declarator:
    direct_declarator
  | function_direct_declarator

function_direct_declarator:
    declarator_id '(' { pushFunction(); postSig("("); }
    parameter_declaration_clause ')' { postSig(")"); }
    function_qualifiers { $<integer>$ = VTK_PARSE_FUNCTION; popFunction(); }

typedef_declarator_id:
    typedef_direct_declarator
    {
      ValueInfo *item = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(item);
      item->ItemType = VTK_TYPEDEF_INFO;
      item->Access = access_level;

      handle_complex_type(item, getType(), $<integer>1, getSig());

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


/*
 * The "using" keyword
 */

using_declaration:
    USING  using_id ';' { add_using($<str>2, 0); }

using_id:
    id_expression
  | TYPENAME id_expression { $<str>$ = $<str>2; }
  | nested_name_specifier operator_function_id
    { $<str>$ = vtkstrcat($<str>1, $<str>2); }
  | nested_name_specifier conversion_function_id
    { $<str>$ = vtkstrcat($<str>1, $<str>2); }
  | scope_operator_sig nested_name_specifier operator_function_id
    { $<str>$ = vtkstrcat3($<str>1, $<str>2, $<str>3); }
  | scope_operator_sig nested_name_specifier conversion_function_id
    { $<str>$ = vtkstrcat3($<str>1, $<str>2, $<str>3); }

using_directive:
    USING NAMESPACE id_expression ';' { add_using($<str>3, 1); }

/*
 * Templates
 */

template_head:
    TEMPLATE '<' '>'
    { postSig("template<> "); clearTypeId(); }
  | TEMPLATE '<'
    {
      postSig("template<");
      pushType();
      clearType();
      clearTypeId();
      startTemplate();
    }
    template_parameter_list '>'
    {
      chopSig();
      if (getSig()[getSigLength()-1] == '>') { postSig(" "); }
      postSig("> ");
      clearTypeId();
      popType();
    }

template_parameter_list:
    template_parameter
  | template_parameter_list ','
    { chopSig(); postSig(", "); clearType(); clearTypeId(); }
    template_parameter

template_parameter:
    { markSig(); }
    tparam_type direct_abstract_declarator
    { add_template_parameter(getType(), $<integer>3, copySig()); }
    opt_template_parameter_initializer
  | { markSig(); }
    class_or_typename direct_abstract_declarator
    { add_template_parameter(0, $<integer>3, copySig()); }
    opt_template_parameter_initializer
  | { pushTemplate(); markSig(); }
    template_head CLASS { postSig("class "); }
    direct_abstract_declarator
    {
      int i;
      TemplateInfo *newTemplate = currentTemplate;
      popTemplate();
      add_template_parameter(0, $<integer>5, copySig());
      i = currentTemplate->NumberOfParameters-1;
      currentTemplate->Parameters[i]->Template = newTemplate;
    }
    opt_template_parameter_initializer

class_or_typename:
    CLASS { postSig("class "); }
  | TYPENAME { postSig("typename "); }

opt_template_parameter_initializer:
  | template_parameter_initializer

template_parameter_initializer:
    '=' { postSig("="); markSig(); }
    template_parameter_value
    {
      int i = currentTemplate->NumberOfParameters-1;
      ValueInfo *param = currentTemplate->Parameters[i];
      chopSig();
      param->Value = copySig();
    }

template_parameter_value:
    angle_bracket_pitem
  | template_parameter_value angle_bracket_pitem


/*
 * Functions and Methods
 */

function_definition:
    function_declaration function_body { output_function(); }
  | operator_declaration function_body { output_function(); }
  | nested_method_declaration function_body { reject_function(); }
  | nested_operator_declaration function_body { reject_function(); }

function_declaration:
    store_type function_nr

nested_method_declaration:
    store_type nested_name_specifier function_nr
  | nested_name_specifier structor_declaration
  | decl_specifier_seq nested_name_specifier structor_declaration

nested_operator_declaration:
    nested_name_specifier conversion_function
  | decl_specifier_seq nested_name_specifier conversion_function
  | store_type nested_name_specifier operator_function_nr

method_definition:
    method_declaration function_body { output_function(); }
  | nested_name_specifier operator_function_id ';'
  | decl_specifier_seq nested_name_specifier operator_function_id ';'

method_declaration:
    store_type function_nr
  | operator_declaration
  | structor_declaration
  | decl_specifier_seq structor_declaration

operator_declaration:
    conversion_function
  | decl_specifier_seq conversion_function
  | store_type operator_function_nr

conversion_function:
    conversion_function_id '('
    {
      postSig("(");
      set_return(currentFunction, getType(), getTypeId(), 0);
    }
    parameter_declaration_clause ')' { postSig(")"); }
    function_trailer_clause
    {
      postSig(";");
      closeSig();
      currentFunction->IsOperator = 1;
      currentFunction->Name = "operator typecast";
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed operator", "operator typecast");
    }

conversion_function_id:
    operator_sig store_type
    { $<str>$ = copySig(); }

operator_function_nr:
    operator_function_sig { postSig(")"); } function_trailer_clause
    {
      postSig(";");
      closeSig();
      currentFunction->Name = $<str>1;
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed operator", currentFunction->Name);
    }

operator_function_sig:
    operator_function_id '('
    {
      postSig("(");
      currentFunction->IsOperator = 1;
      set_return(currentFunction, getType(), getTypeId(), 0);
    }
    parameter_declaration_clause ')'

operator_function_id:
    operator_sig operator_id
    { chopSig(); $<str>$ = vtkstrcat(copySig(), $<str>2); postSig($<str>2); }

operator_sig:
    OPERATOR { markSig(); postSig("operator "); }

function_nr:
    function_sig function_trailer_clause
    {
      postSig(";");
      closeSig();
      currentFunction->Name = $<str>1;
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }

function_trailer_clause:
  | function_trailer_clause function_trailer

function_trailer:
    THROW { postSig(" throw "); } parentheses_sig { chopSig(); }
  | CONST { postSig(" const"); currentFunction->IsConst = 1; }
  | '=' ZERO
    {
      postSig(" = 0");
      currentFunction->IsPureVirtual = 1;
      if (currentClass) { currentClass->IsAbstract = 1; }
    }

function_body:
    '{' ignored_items '}'
  | ';'

function_sig:
    function_name '('
    {
      postSig("(");
      set_return(currentFunction, getType(), getTypeId(), 0);
    }
    parameter_declaration_clause ')' { postSig(")"); }

function_name:
    simple_id
  | template_id


/*
 * Constructors and destructors are handled by the same rule
 */

structor_declaration:
    structor_sig { closeSig(); }
    opt_ctor_initializer { openSig(); } function_trailer_clause
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
      currentFunction->Name = $<str>1;
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }

structor_sig:
    function_name '(' { pushType(); postSig("("); }
    parameter_declaration_clause ')' { popType(); postSig(")"); }

opt_ctor_initializer:
  | ':' mem_initializer_list

mem_initializer_list:
    mem_initializer
  | mem_initializer_list ',' mem_initializer

mem_initializer:
    id_expression ignored_parentheses

/*
 * Parameters
 */

parameter_declaration_clause:
  | { clearType(); clearTypeId(); } parameter_list

parameter_list:
    parameter_declaration { clearType(); clearTypeId(); }
  | parameter_list ',' { clearType(); clearTypeId(); postSig(", "); }
    parameter_declaration
  | parameter_list ',' ELLIPSIS
    { currentFunction->IsVariadic = 1; postSig(", ..."); }

parameter_declaration:
    { markSig(); }
    store_type direct_abstract_declarator
    {
      ValueInfo *param = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(param);

      handle_complex_type(param, getType(), $<integer>3, copySig());
      add_legacy_parameter(currentFunction, param);

      if (getVarName())
        {
        param->Name = getVarName();
        }

      vtkParse_AddParameterToFunction(currentFunction, param);
    }
    opt_initializer
    {
      int i = currentFunction->NumberOfParameters-1;
      if (getVarValue())
        {
        currentFunction->Parameters[i]->Value = getVarValue();
        }
    }

opt_initializer:
    { clearVarValue(); }
  | initializer

initializer:
    '=' { postSig("="); clearVarValue(); markSig(); }
    constant_expression { chopSig(); setVarValue(copySig()); }

/*
 * Variables
 */

variable_declaration:
    store_type init_declarator_id declarator_list_cont ';'

init_declarator_id:
    direct_declarator opt_initializer
    {
      unsigned int type = getType();
      ValueInfo *var = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(var);
      var->ItemType = VTK_VARIABLE_INFO;
      var->Access = access_level;

      handle_complex_type(var, type, $<integer>1, getSig());

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

opt_declarator_list:
  | init_declarator declarator_list_cont

declarator_list_cont:
  | declarator_list_cont ',' { postSig(", "); } init_declarator

init_declarator:
    opt_ptr_operator_seq init_declarator_id

opt_ptr_operator_seq:
    { setTypePtr(0); }
  | ptr_operator_seq { setTypePtr($<integer>1); }

/* for parameters, the declarator_id is optional */
direct_abstract_declarator:
    opt_declarator_id opt_array_decorator_seq { $<integer>$ = 0; }
  | p_or_lp_or_la abstract_declarator ')' { postSig(")"); }
    opt_array_or_parameters
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
    }

/* for variables, the declarator_id is mandatory */
direct_declarator:
    declarator_id opt_array_decorator_seq { $<integer>$ = 0; }
  | lp_or_la declarator ')' { postSig(")"); }
    opt_array_or_parameters
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
    }

p_or_lp_or_la:
    '(' { postSig("("); scopeSig(""); $<integer>$ = 0; }
  | LP { postSig("("); scopeSig($<str>1); postSig("*");
         $<integer>$ = VTK_PARSE_POINTER; }
  | LA { postSig("("); scopeSig($<str>1); postSig("&");
         $<integer>$ = VTK_PARSE_REF; }

lp_or_la:
    LP { postSig("("); scopeSig($<str>1); postSig("*");
         $<integer>$ = VTK_PARSE_POINTER; }
  | LA { postSig("("); scopeSig($<str>1); postSig("&");
         $<integer>$ = VTK_PARSE_REF; }

opt_array_or_parameters: { $<integer>$ = 0; }
  | '(' { pushFunction(); postSig("("); } parameter_declaration_clause ')'
    { postSig(")"); } function_qualifiers
    {
      $<integer>$ = VTK_PARSE_FUNCTION;
      popFunction();
    }
  | array_decorator_seq { $<integer>$ = VTK_PARSE_ARRAY; }

function_qualifiers:
  | function_qualifiers MUTABLE
  | function_qualifiers CONST { currentFunction->IsConst = 1; }
  | function_qualifiers THROW ignored_parentheses

abstract_declarator:
    direct_abstract_declarator
  | ptr_operator_seq direct_abstract_declarator
    { $<integer>$ = add_indirection($<integer>1, $<integer>2); }

declarator:
    direct_declarator
  | ptr_operator_seq direct_declarator
    { $<integer>$ = add_indirection($<integer>1, $<integer>2); }

opt_declarator_id:
    { clearVarName(); chopSig(); }
  | declarator_id

declarator_id:
    simple_id { setVarName($<str>1); }
  | simple_id ':' bitfield_size { setVarName($<str>1); }

bitfield_size:
    OCT_LITERAL
  | INT_LITERAL
  | HEX_LITERAL

opt_array_decorator_seq:
    { clearArray(); }
  | array_decorator_seq

array_decorator_seq:
    { clearArray(); } array_decorator_seq_impl

array_decorator_seq_impl:
    array_decorator
  | array_decorator_seq_impl array_decorator

array_decorator:
    '[' { postSig("["); } array_size_specifier ']' { postSig("]"); }

array_size_specifier:
    { pushArraySize(""); }
  | { markSig(); } constant_expression { chopSig(); pushArraySize(copySig()); }

/*
 * Identifiers
 */

id_expression:
    unqualified_id
  | qualified_id

unqualified_id:
    simple_id
  | template_id

qualified_id:
    nested_name_specifier unqualified_id
    { $<str>$ = vtkstrcat($<str>1, $<str>2); }
  | scope_operator_sig unqualified_id
    { $<str>$ = vtkstrcat($<str>1, $<str>2); }
  | scope_operator_sig qualified_id
    { $<str>$ = vtkstrcat($<str>1, $<str>2); }

nested_name_specifier:
    identifier_sig scope_operator_sig
    { $<str>$ = vtkstrcat($<str>1, $<str>2); }
  | template_id scope_operator_sig
    { $<str>$ = vtkstrcat($<str>1, $<str>2); }
  | nested_name_specifier identifier_sig scope_operator_sig
    { $<str>$ = vtkstrcat3($<str>1, $<str>2, $<str>3); }
  | nested_name_specifier template_id scope_operator_sig
    { $<str>$ = vtkstrcat3($<str>1, $<str>2, $<str>3); }
  | nested_name_specifier TEMPLATE { postSig("template "); }
    template_id scope_operator_sig
    { $<str>$ = vtkstrcat4($<str>1, "template ", $<str>4, $<str>5); }

identifier_sig:
    identifier { postSig($<str>1); }

scope_operator_sig:
    DOUBLE_COLON { $<str>$ = "::"; postSig($<str>$); }

template_id:
    identifier '<' { markSig(); postSig($<str>1); postSig("<"); }
    angle_bracket_contents '>'
    {
      chopSig(); if (getSig()[getSigLength()-1] == '>') { postSig(" "); }
      postSig(">"); $<str>$ = copySig(); clearTypeId();
    }

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

/*
 * An identifier with no side-effects.
 */

identifier:
    ID
  | QT_ID
  | VTK_ID
  | ISTREAM
  | OSTREAM
  | StdString
  | UnicodeString


/*
 * Declaration specifiers
 */

opt_decl_specifier_seq:
  | opt_decl_specifier_seq decl_specifier2

decl_specifier2:
    decl_specifier
  | primitive_type
    { setTypeBase(buildTypeBase(getType(), $<integer>1)); }
  | TYPEDEF { setTypeMod(VTK_PARSE_TYPEDEF); }
  | FRIEND { setTypeMod(VTK_PARSE_FRIEND); }

decl_specifier_seq:
    decl_specifier
  | decl_specifier_seq decl_specifier

decl_specifier:
    storage_class_specifier { setTypeMod($<integer>1); }
  | function_specifier { setTypeMod($<integer>1); }
  | cv_qualifier { setTypeMod($<integer>1); }

storage_class_specifier:
    MUTABLE { postSig("mutable "); $<integer>$ = VTK_PARSE_MUTABLE; }
  | EXTERN { $<integer>$ = 0; }
  | EXTERN STRING_LITERAL { $<integer>$ = 0; }
  | STATIC { postSig("static "); $<integer>$ = VTK_PARSE_STATIC; }

function_specifier:
    INLINE { $<integer>$ = 0; }
  | VIRTUAL { postSig("virtual "); $<integer>$ = VTK_PARSE_VIRTUAL; }
  | EXPLICIT { postSig("explicit "); $<integer>$ = VTK_PARSE_EXPLICIT; }

cv_qualifier:
    CONST { postSig("const "); $<integer>$ = VTK_PARSE_CONST; }
  | VOLATILE { postSig("volatile "); $<integer>$ = VTK_PARSE_VOLATILE; }

cv_qualifier_seq:
    cv_qualifier
  | cv_qualifier_seq cv_qualifier
    { $<integer>$ = ($<integer>1 | $<integer>2); }


/*
 * Types
 */

store_type:
    store_type_specifier opt_ptr_operator_seq

store_type_specifier:
    type_specifier { setTypeBase($<integer>1); }
    opt_decl_specifier_seq
  | decl_specifier_seq type_specifier { setTypeBase($<integer>2); }
    opt_decl_specifier_seq

type_specifier:
    simple_type_specifier
  | TYPENAME { postSig("typename "); } id_expression
    { postSig(" "); setTypeId($<str>3); $<integer>$ = guess_id_type($<str>3); }
  | template_id
    { postSig(" "); setTypeId($<str>1); $<integer>$ = guess_id_type($<str>1); }
  | qualified_id
    { postSig(" "); setTypeId($<str>1); $<integer>$ = guess_id_type($<str>1); }
  | class_key id_expression
    { postSig(" "); setTypeId($<str>2); $<integer>$ = guess_id_type($<str>2); }
  | ENUM id_expression
    { postSig(" "); setTypeId($<str>2); $<integer>$ = guess_id_type($<str>2); }

tparam_type:
    tparam_type_specifier2 opt_ptr_operator_seq

tparam_type_specifier2:
    tparam_type_specifier { setTypeBase($<integer>1); }
    opt_decl_specifier_seq
  | decl_specifier_seq type_specifier { setTypeBase($<integer>2); }
    opt_decl_specifier_seq

tparam_type_specifier:
    simple_type_specifier
  | template_id
    { postSig(" "); setTypeId($<str>1); $<integer>$ = guess_id_type($<str>1); }
  | qualified_id
    { postSig(" "); setTypeId($<str>1); $<integer>$ = guess_id_type($<str>1); }
  | STRUCT id_expression
    { postSig(" "); setTypeId($<str>2); $<integer>$ = guess_id_type($<str>2); }
  | UNION id_expression
    { postSig(" "); setTypeId($<str>2); $<integer>$ = guess_id_type($<str>2); }
  | ENUM id_expression
    { postSig(" "); setTypeId($<str>2); $<integer>$ = guess_id_type($<str>2); }

simple_type_specifier:
    primitive_type { setTypeId(""); }
  | type_name

type_name:
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

primitive_type:
    VOID   { postSig("void "); $<integer>$ = VTK_PARSE_VOID; }
  | BOOL { postSig("bool "); $<integer>$ = VTK_PARSE_BOOL; }
  | FLOAT  { postSig("float "); $<integer>$ = VTK_PARSE_FLOAT; }
  | DOUBLE { postSig("double "); $<integer>$ = VTK_PARSE_DOUBLE; }
  | CHAR   { postSig("char "); $<integer>$ = VTK_PARSE_CHAR; }
  | INT    { postSig("int "); $<integer>$ = VTK_PARSE_INT; }
  | SHORT  { postSig("short "); $<integer>$ = VTK_PARSE_SHORT; }
  | LONG   { postSig("long "); $<integer>$ = VTK_PARSE_LONG; }
  | INT64__ { postSig("__int64 "); $<integer>$ = VTK_PARSE___INT64; }
  | SIGNED { postSig("signed "); $<integer>$ = VTK_PARSE_INT; }
  | UNSIGNED { postSig("unsigned "); $<integer>$ = VTK_PARSE_UNSIGNED_INT; }


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

ptr_operator_seq:
    reference
  | pointer_seq
  | pointer_seq reference { $<integer>$ = ($<integer>1 | $<integer>2); }

reference:
    '&' { postSig("&"); $<integer>$ = VTK_PARSE_REF; }

pointer:
    '*' { postSig("*"); $<integer>$ = VTK_PARSE_POINTER; }
  | '*' { postSig("*"); } cv_qualifier_seq
    {
      if (($<integer>3 & VTK_PARSE_CONST) != 0)
        {
        $<integer>$ = VTK_PARSE_CONST_POINTER;
        }
      if (($<integer>3 & VTK_PARSE_VOLATILE) != 0)
        {
        $<integer>$ = VTK_PARSE_BAD_INDIRECT;
        }
    }

/* "VTK_BAD_INDIRECT" occurs when the bitfield fills up */

pointer_seq:
    pointer
  | pointer_seq pointer
    {
      unsigned int n;
      n = (($<integer>1 << 2) | $<integer>2);
      if ((n & VTK_PARSE_INDIRECT) != n)
        {
        n = VTK_PARSE_BAD_INDIRECT;
        }
      $<integer>$ = n;
    }


/*
 * VTK Macros
 */

declaration_macro:
  SetMacro '(' simple_id ',' {preSig("void Set"); postSig("(");} store_type ')'
   {
   postSig("a);");
   currentFunction->Macro = "vtkSetMacro";
   currentFunction->Name = vtkstrcat("Set", $<str>3);
   currentFunction->Comment = vtkstrdup(getComment());
   add_parameter(currentFunction, getType(), getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
| GetMacro '(' {postSig("Get");} simple_id ','
   {markSig();} store_type {swapSig();} ')'
   {
   postSig("();");
   currentFunction->Macro = "vtkGetMacro";
   currentFunction->Name = vtkstrcat("Get", $<str>4);
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, getType(), getTypeId(), 0);
   output_function();
   }
| SetStringMacro '(' {preSig("void Set");} simple_id ')'
   {
   postSig("(char *);");
   currentFunction->Macro = "vtkSetStringMacro";
   currentFunction->Name = vtkstrcat("Set", $<str>4);
   currentFunction->Comment = vtkstrdup(getComment());
   add_parameter(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
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
| SetClampMacro '(' simple_id ',' {startSig(); markSig();} store_type {closeSig();}
     ',' ignored_expression ')'
   {
   const char *typeText;
   chopSig();
   typeText = copySig();

   currentFunction->Macro = "vtkSetClampMacro";
   currentFunction->Name = vtkstrcat("Set", $<str>3);
   currentFunction->Signature =
     vtkstrcat5("void ", currentFunction->Name, "(", typeText, ");");
   currentFunction->Comment = vtkstrdup(getComment());
   add_parameter(currentFunction, getType(), getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();

   currentFunction->Macro = "vtkSetClampMacro";
   currentFunction->Name = vtkstrcat3("Get", $<str>3, "MinValue");
   currentFunction->Signature =
     vtkstrcat4(typeText, " ", currentFunction->Name, "();");
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, getType(), getTypeId(), 0);
   output_function();

   currentFunction->Macro = "vtkSetClampMacro";
   currentFunction->Name = vtkstrcat3("Get", $<str>3, "MaxValue");
   currentFunction->Signature =
     vtkstrcat4(typeText, " ", currentFunction->Name, "();");
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, getType(), getTypeId(), 0);
   output_function();
   }
| SetObjectMacro '(' simple_id ','
  {preSig("void Set"); postSig("("); } store_type ')'
   {
   postSig("*);");
   currentFunction->Macro = "vtkSetObjectMacro";
   currentFunction->Name = vtkstrcat("Set", $<str>3);
   currentFunction->Comment = vtkstrdup(getComment());
   add_parameter(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
| GetObjectMacro '(' {postSig("*Get");} simple_id ','
   {markSig();} store_type {swapSig();} ')'
   {
   postSig("();");
   currentFunction->Macro = "vtkGetObjectMacro";
   currentFunction->Name = vtkstrcat("Get", $<str>4);
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   output_function();
   }
| BooleanMacro '(' simple_id ',' store_type ')'
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
| SetVector2Macro '(' simple_id ',' {startSig(); markSig();} store_type ')'
   {
   chopSig();
   outputSetVectorMacro($<str>3, getType(), copySig(), 2);
   }
| GetVector2Macro '(' simple_id ',' {startSig(); markSig();} store_type ')'
   {
   chopSig();
   outputGetVectorMacro($<str>3, getType(), copySig(), 2);
   }
| SetVector3Macro '(' simple_id ',' {startSig(); markSig();} store_type ')'
   {
   chopSig();
   outputSetVectorMacro($<str>3, getType(), copySig(), 3);
   }
| GetVector3Macro  '(' simple_id ',' {startSig(); markSig();} store_type ')'
   {
   chopSig();
   outputGetVectorMacro($<str>3, getType(), copySig(), 3);
   }
| SetVector4Macro '(' simple_id ',' {startSig(); markSig();} store_type ')'
   {
   chopSig();
   outputSetVectorMacro($<str>3, getType(), copySig(), 4);
   }
| GetVector4Macro  '(' simple_id ',' {startSig(); markSig();} store_type ')'
   {
   chopSig();
   outputGetVectorMacro($<str>3, getType(), copySig(), 4);
   }
| SetVector6Macro '(' simple_id ',' {startSig(); markSig();} store_type ')'
   {
   chopSig();
   outputSetVectorMacro($<str>3, getType(), copySig(), 6);
   }
| GetVector6Macro  '(' simple_id ',' {startSig(); markSig();} store_type ')'
   {
   chopSig();
   outputGetVectorMacro($<str>3, getType(), copySig(), 6);
   }
| SetVectorMacro  '(' simple_id ',' {startSig(); markSig();}
     store_type ',' INT_LITERAL ')'
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
   add_parameter(currentFunction, (VTK_PARSE_POINTER | getType()),
                 getTypeId(), (int)strtol($<str>8, NULL, 0));
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
| GetVectorMacro  '(' simple_id ',' {startSig();}
     store_type ',' INT_LITERAL ')'
   {
   chopSig();
   currentFunction->Macro = "vtkGetVectorMacro";
   currentFunction->Name = vtkstrcat("Get", $<str>3);
   postSig(" *");
   postSig(currentFunction->Name);
   postSig("();");
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, (VTK_PARSE_POINTER | getType()),
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
     add_parameter(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_parameter(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Macro = "vtkViewportCoordinateMacro";
     currentFunction->Name = vtkstrcat("Set", $<str>3);
     currentFunction->Signature =
       vtkstrcat3("void ", currentFunction->Name, "(double a[2]);");
     currentFunction->Comment = vtkstrdup(getComment());
     add_parameter(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 2);
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
     add_parameter(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_parameter(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_parameter(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Macro = "vtkWorldCoordinateMacro";
     currentFunction->Name = vtkstrcat("Set", $<str>3);
     currentFunction->Signature =
       vtkstrcat3("void ", currentFunction->Name, "(double a[3]);");
     currentFunction->Comment = vtkstrdup(getComment());
     add_parameter(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 3);
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
| TypeMacro '(' simple_id ',' id_expression opt_comma ')'
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
   currentFunction->Signature = vtkstrcat($<str>3, " *NewInstance();");
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_OBJECT_PTR, $<str>3, 0);
   output_function();

   currentFunction->Macro = "vtkTypeMacro";
   currentFunction->Name = "SafeDownCast";
   currentFunction->Signature =
     vtkstrcat($<str>3, " *SafeDownCast(vtkObject* o);");
   currentFunction->Comment = vtkstrdup(getComment());
   add_parameter(currentFunction, VTK_PARSE_OBJECT_PTR, "vtkObject", 0);
   set_return(currentFunction, (VTK_PARSE_STATIC | VTK_PARSE_OBJECT_PTR),
              $<str>3, 0);
   output_function();
   }

opt_comma:
  | ','

/*
 * Operators
 */

operator_id:
    '(' ')' { $<str>$ = "()"; }
  | '[' ']' { $<str>$ = "[]"; }
  | NEW '[' ']' { $<str>$ = " new[]"; }
  | DELETE '[' ']' { $<str>$ = " delete[]"; }
  | '<' { $<str>$ = "<"; }
  | '>' { $<str>$ = ">"; }
  | ',' { $<str>$ = ","; }
  | '=' { $<str>$ = "="; }
  | operator_id_no_delim

operator_id_no_delim:
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
  | OP_LOGIC_GEQ { $<str>$ = ">="; }

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
  | USING { $<str>$ = "using"; }
  | NAMESPACE { $<str>$ = "namespace"; }
  | OPERATOR { $<str>$ = "operator"; }
  | ENUM { $<str>$ = "enum"; }
  | THROW { $<str>$ = "throw"; }
  | CONST_CAST { $<str>$ = "const_cast"; }
  | DYNAMIC_CAST { $<str>$ = "dynamic_cast"; }
  | STATIC_CAST { $<str>$ = "static_cast"; }
  | REINTERPRET_CAST { $<str>$ = "reinterpret_cast"; }

literal:
    OCT_LITERAL
  | INT_LITERAL
  | HEX_LITERAL
  | FLOAT_LITERAL
  | CHAR_LITERAL
  | STRING_LITERAL
  | ZERO

/*
 * Constant expressions that evaluate to one or more values
 */

constant_expression:
    bracket_pitem
  | constant_expression bracket_pitem;

common_bracket_item:
    brackets_sig
  | parentheses_sig
  | braces_sig
  | operator_id_no_delim
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
  | primitive_type
  | type_name
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

any_bracket_contents:
  | any_bracket_contents any_bracket_item

bracket_pitem: common_bracket_item
  | '<' { postSig("< "); }
  | '>' { postSig("> "); }

any_bracket_item: bracket_pitem
  | '=' { postSig("= "); }
  | ',' { chopSig(); postSig(", "); }

braces_item: any_bracket_item
  | ';' { chopSig(); postSig(";"); }

angle_bracket_contents:
  | angle_bracket_contents angle_bracket_item

braces_contents:
  | braces_contents braces_item

angle_bracket_pitem:
    angle_brackets_sig
  | common_bracket_item

angle_bracket_item:
    angle_bracket_pitem
  | '=' { postSig("= "); }
  | ',' { chopSig(); postSig(", "); }

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
    }

brackets_sig:
    '[' { postSig("["); } any_bracket_contents ']'
    { chopSig(); postSig("] "); }

parentheses_sig:
    '(' { postSig("("); } any_bracket_contents ')'
    { chopSig(); postSig(") "); }
  | LP { postSig("("); postSig($<str>1); postSig("*"); }
    any_bracket_contents ')' { chopSig(); postSig(") "); }
  | LA { postSig("("); postSig($<str>1); postSig("&"); }
    any_bracket_contents ')' { chopSig(); postSig(") "); }

braces_sig:
    '{' { postSig("{ "); } braces_contents '}' { postSig("} "); }

/*
 * These just eat up stuff we don't care about, like function bodies
 */
ignored_items:
  | ignored_items ignored_item

ignored_expression:
  | ignored_expression ignored_item_no_semi

ignored_item:
    ignored_item_no_semi
  | ';'

ignored_item_no_semi:
    ignored_braces
  | ignored_parentheses
  | ignored_brackets
  | DOUBLE_COLON
  | ELLIPSIS
  | operator_id_no_delim
  | ':' | '.' | '<' | '>' | '=' | ','
  | keyword | literal
  | simple_type_specifier
  | OTHER

ignored_braces:
  '{' ignored_items '}'

ignored_brackets:
  '[' ignored_items ']'

ignored_parentheses:
  ignored_left_parenthesis ignored_items ')'

ignored_left_parenthesis:
  '(' | LP | LA

%%
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
