
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
char         **ConcreteClasses;

NamespaceInfo *currentNamespace = NULL;
ClassInfo     *currentClass = NULL;
FunctionInfo  *currentFunction = NULL;
TemplateArgs  *currentTemplate = NULL;

char          *currentEnumName = 0;
char          *currentEnumValue = 0;

int            parseDebug;
char           temps[2048];
parse_access_t access_level = VTK_ACCESS_PUBLIC;
int            HaveComment;
char           CommentText[50000];
int            CommentState;
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

/* duplicate a string */
char *vtkstrdup(const char *in)
{
  char *res = NULL;
  if (in)
    {
    res = (char *)malloc(strlen(in)+1);
    strcpy(res,in);
    }
  return res;
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
int sigClosed;
size_t sigMark[10];
size_t sigAllocatedLength;
int sigMarkDepth = 0;

/* start a new signature */
void startSig()
{
  if (currentFunction->Signature)
    {
    free(currentFunction->Signature);
    currentFunction->Signature = NULL;
    }

  sigAllocatedLength = 0;
  sigClosed = 0;
  sigMarkDepth = 0;
  sigMark[0] = 0;
}

/* get the signature */
const char *getSig()
{
  return currentFunction->Signature;
}

/* reallocate Signature if "arg" cannot be appended */
void checkSigSize(const char *arg)
{
  if (strlen(currentFunction->Signature) + strlen(arg) + 5 >
      sigAllocatedLength)
    {
    currentFunction->Signature = (char *)
      realloc(currentFunction->Signature, sigAllocatedLength*2);
    sigAllocatedLength = sigAllocatedLength*2;
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
  if (!currentFunction->Signature)
    {
    currentFunction->Signature = (char*)malloc(2048);
    sigAllocatedLength = 2048;
    strcpy(currentFunction->Signature, arg);
    }
  else if (!sigClosed)
    {
    size_t m, n;
    char *cp;
    checkSigSize(arg);
    cp = currentFunction->Signature;
    m = strlen(cp);
    n = strlen(arg);
    memmove(&cp[n], cp, m+1);
    strncpy(cp, arg, n);
    }
}

/* append text to the end of the signature */
void postSig(const char *arg)
{
  if (!currentFunction->Signature)
    {
    currentFunction->Signature = (char*)malloc(2048);
    sigAllocatedLength = 2048;
    strcpy(currentFunction->Signature, arg);
    }
  else if (!sigClosed)
    {
    size_t m, n;
    char *cp;
    checkSigSize(arg);
    cp = currentFunction->Signature;
    m = strlen(cp);
    n = strlen(arg);
    strncpy(&cp[m], arg, n+1);
    }
}

/* prepend a scope:: to the ID at the end of the signature */
void preScopeSig(const char *arg)
{
  if (!currentFunction->Signature)
    {
    currentFunction->Signature = (char*)malloc(2048);
    sigAllocatedLength = 2048;
    strcpy(currentFunction->Signature, arg);
    }
  else if (!sigClosed)
    {
    checkSigSize(arg);
    prepend_scope(currentFunction->Signature, arg);
    }
}

/* set a mark in the signature for later operations */
void markSig()
{
  sigMark[sigMarkDepth] = 0;
  if (currentFunction->Signature)
    {
    sigMark[sigMarkDepth] = strlen(currentFunction->Signature);
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
  if (currentFunction->Signature)
    {
    cp = &currentFunction->Signature[sigMark[sigMarkDepth]];
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
  if (currentFunction->Signature && sigMark[sigMarkDepth] > 0)
    {
    size_t i, m, n, nn;
    char c;
    char *cp;
    cp = currentFunction->Signature;
    n = strlen(cp);
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
  if (currentFunction->Signature)
    {
    size_t n = strlen(currentFunction->Signature);
    if (n > 0 && currentFunction->Signature[n-1] == ' ')
      {
      currentFunction->Signature[n-1] = '\0';
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
char **arrayDimensions;

/* clear the array counter */
void clearArray(void)
{
  numberOfDimensions = 0;
}

/* add another dimension */
void pushArraySize(const char *size)
{
  vtkParse_AddPointerToArray(&arrayDimensions, &numberOfDimensions,
                             vtkstrdup(size));
}

/* add another dimension to the front */
void pushArrayFront(const char *size)
{
  int i;

  vtkParse_AddPointerToArray(&arrayDimensions, &numberOfDimensions, 0);

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
char **getArray()
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
char *functionVarNameStack[10];
char *functionTypeIdStack[10];
int functionDepth = 0;

void pushFunction()
{
  functionStack[functionDepth] = currentFunction;
  currentFunction = (FunctionInfo *)malloc(sizeof(FunctionInfo));
  vtkParse_InitFunction(currentFunction);
  if (functionStack[functionDepth])
    {
    currentFunction->Signature = functionStack[functionDepth]->Signature;
    }
  else
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
  if (currentFunction)
    {
    currentFunction->Signature = newFunction->Signature;
    newFunction->Signature = NULL;
    }
  clearVarName();
  if (functionVarNameStack[functionDepth])
    {
    setVarName(functionVarNameStack[functionDepth]);
    free(functionVarNameStack[functionDepth]);
    }
  clearTypeId();
  if (functionTypeIdStack[functionDepth])
    {
    setTypeId(functionTypeIdStack[functionDepth]);
    free(functionTypeIdStack[functionDepth]);
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
  size_t i, m, n;
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

  memmove(&cp[i+n+2], &cp[i], m+1);
  strncpy(&cp[i], arg, n);
  strncpy(&cp[i+n], "::", 2);
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
#line 865 "vtkParse.tab.c"

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
     VTK_CONSTANT_DEF = 381,
     VTK_BYTE_SWAP_DECL = 382
   };
#endif




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 222 of yacc.c  */
#line 812 "vtkParse.y"

  char         *str;
  unsigned int  integer;



/* Line 222 of yacc.c  */
#line 1162 "vtkParse.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 1174 "vtkParse.tab.c"

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
#define YYLAST   5650

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  151
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  189
/* YYNRULES -- Number of rules.  */
#define YYNRULES  542
/* YYNRULES -- Number of states.  */
#define YYNSTATES  947

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
      54,    58,    61,    63,    65,    68,    73,    78,    80,    86,
      87,    94,    99,   100,   108,   109,   120,   121,   129,   130,
     141,   146,   147,   148,   152,   155,   157,   161,   165,   167,
     169,   171,   174,   176,   178,   181,   185,   189,   192,   196,
     200,   203,   209,   211,   213,   215,   216,   219,   221,   225,
     227,   230,   233,   236,   238,   240,   242,   243,   250,   251,
     257,   258,   260,   264,   266,   270,   272,   274,   276,   278,
     280,   282,   284,   286,   288,   290,   291,   295,   296,   301,
     302,   307,   309,   311,   313,   315,   317,   319,   321,   323,
     325,   327,   329,   335,   340,   344,   347,   351,   355,   358,
     360,   366,   370,   375,   380,   385,   390,   394,   396,   400,
     401,   407,   409,   410,   415,   418,   421,   422,   426,   428,
     430,   431,   432,   436,   441,   446,   449,   453,   458,   464,
     468,   473,   480,   488,   494,   501,   504,   508,   511,   515,
     519,   521,   524,   527,   531,   535,   539,   541,   544,   548,
     549,   550,   559,   560,   564,   565,   566,   574,   575,   579,
     580,   583,   586,   588,   590,   594,   595,   601,   602,   603,
     613,   614,   618,   619,   625,   626,   630,   631,   635,   640,
     642,   643,   649,   650,   651,   654,   656,   658,   659,   664,
     665,   666,   672,   674,   676,   679,   680,   682,   683,   687,
     692,   697,   701,   704,   705,   708,   709,   710,   715,   716,
     719,   720,   724,   727,   728,   734,   737,   738,   744,   746,
     748,   750,   752,   754,   755,   756,   761,   763,   765,   768,
     770,   773,   774,   776,   778,   779,   781,   782,   785,   786,
     792,   793,   795,   796,   798,   800,   802,   804,   806,   808,
     810,   812,   815,   818,   822,   825,   828,   832,   834,   837,
     839,   842,   844,   847,   850,   852,   854,   856,   858,   859,
     863,   864,   870,   871,   877,   879,   880,   885,   887,   889,
     891,   893,   895,   897,   899,   901,   905,   909,   911,   913,
     915,   917,   919,   921,   923,   926,   928,   930,   933,   935,
     937,   939,   942,   945,   948,   951,   954,   957,   959,   961,
     963,   965,   967,   969,   971,   973,   975,   977,   979,   981,
     983,   985,   987,   989,   991,   993,   995,   997,   999,  1001,
    1003,  1005,  1007,  1009,  1011,  1013,  1015,  1017,  1019,  1021,
    1023,  1025,  1027,  1029,  1031,  1033,  1035,  1037,  1039,  1041,
    1042,  1049,  1050,  1052,  1053,  1054,  1059,  1061,  1062,  1066,
    1067,  1071,  1073,  1074,  1079,  1080,  1081,  1091,  1093,  1096,
    1098,  1100,  1102,  1104,  1106,  1108,  1110,  1112,  1113,  1121,
    1122,  1123,  1124,  1134,  1135,  1141,  1142,  1148,  1149,  1150,
    1161,  1162,  1170,  1171,  1172,  1173,  1183,  1190,  1191,  1199,
    1200,  1208,  1209,  1217,  1218,  1226,  1227,  1235,  1236,  1244,
    1245,  1253,  1254,  1262,  1263,  1273,  1274,  1284,  1289,  1294,
    1302,  1305,  1308,  1312,  1316,  1318,  1320,  1322,  1324,  1326,
    1328,  1330,  1332,  1334,  1336,  1338,  1340,  1342,  1344,  1346,
    1348,  1350,  1352,  1354,  1356,  1358,  1360,  1362,  1364,  1366,
    1368,  1370,  1372,  1374,  1376,  1378,  1380,  1382,  1384,  1386,
    1388,  1390,  1392,  1394,  1396,  1398,  1400,  1401,  1404,  1405,
    1408,  1410,  1412,  1414,  1416,  1418,  1420,  1422,  1424,  1426,
    1428,  1430,  1432,  1434,  1436,  1438,  1440,  1442,  1444,  1446,
    1448,  1450,  1452,  1454,  1456,  1458,  1460,  1462,  1464,  1466,
    1468,  1470,  1472,  1474,  1476,  1478,  1480,  1482,  1484,  1486,
    1488,  1490,  1492,  1494,  1496,  1498,  1500,  1502,  1504,  1508,
    1512,  1516,  1520
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     152,     0,    -1,    -1,    -1,   152,   153,   154,    -1,   244,
      -1,   170,   246,   128,    -1,   183,   246,   128,    -1,   184,
      -1,   156,    -1,   155,    -1,   188,    -1,   158,   246,   128,
      -1,   190,   158,   246,   128,    -1,    41,    -1,   206,   218,
      -1,   190,   206,   218,    -1,   205,   218,    -1,   201,   218,
      -1,   202,   218,    -1,   190,   201,   218,    -1,   199,   218,
      -1,   307,    -1,   332,    -1,   286,   128,    -1,     9,   129,
     333,   130,    -1,    57,   129,   333,   130,    -1,   128,    -1,
      59,    10,   131,   152,   132,    -1,    -1,    55,   286,   157,
     131,   152,   132,    -1,    55,   131,   333,   132,    -1,    -1,
       4,   271,   159,   166,   131,   163,   132,    -1,    -1,     4,
     271,   133,   282,   134,   160,   166,   131,   163,   132,    -1,
      -1,     3,   271,   161,   166,   131,   163,   132,    -1,    -1,
       3,   271,   133,   282,   134,   162,   166,   131,   163,   132,
      -1,     3,   131,   333,   132,    -1,    -1,    -1,   163,   164,
     165,    -1,   169,   135,    -1,   244,    -1,   170,   246,   128,
      -1,   183,   246,   128,    -1,   184,    -1,   188,    -1,   186,
      -1,    49,   186,    -1,   185,    -1,    41,    -1,   206,   218,
      -1,    49,   206,   218,    -1,   190,   206,   218,    -1,   204,
     218,    -1,    49,   204,   218,    -1,   190,   204,   218,    -1,
     200,   218,    -1,   127,   129,   333,   130,   128,    -1,   307,
      -1,   332,    -1,   128,    -1,    -1,   135,   167,    -1,   168,
      -1,   168,   136,   167,    -1,   284,    -1,     6,   284,    -1,
       7,   284,    -1,     5,   284,    -1,     5,    -1,     6,    -1,
       7,    -1,    -1,    39,   271,   171,   131,   173,   132,    -1,
      -1,    39,   172,   131,   173,   132,    -1,    -1,   174,    -1,
     174,   136,   173,    -1,   271,    -1,   271,   137,   177,    -1,
     176,    -1,   271,    -1,   285,    -1,   279,    -1,    16,    -1,
      11,    -1,    13,    -1,    12,    -1,    15,    -1,   175,    -1,
      -1,   181,   178,   177,    -1,    -1,   175,   182,   179,   177,
      -1,    -1,   129,   180,   177,   130,    -1,   138,    -1,   139,
      -1,   140,    -1,   138,    -1,   139,    -1,   141,    -1,   142,
      -1,   143,    -1,   144,    -1,   145,    -1,   146,    -1,    40,
     271,   131,   333,   132,    -1,    40,   131,   333,   132,    -1,
      56,   334,   128,    -1,   190,   186,    -1,     4,   271,   187,
      -1,     3,   271,   187,    -1,     3,   187,    -1,   128,    -1,
     131,   333,   132,   334,   128,    -1,   135,   334,   128,    -1,
     189,   274,   254,   128,    -1,   189,   158,   240,   128,    -1,
     189,   170,   240,   128,    -1,   189,   183,   240,   128,    -1,
     189,    60,   128,    -1,    54,    -1,    52,   133,   134,    -1,
      -1,    52,   133,   191,   192,   134,    -1,   194,    -1,    -1,
     194,   136,   193,   192,    -1,   291,   197,    -1,   196,   197,
      -1,    -1,   195,   190,   197,    -1,     4,    -1,    53,    -1,
      -1,    -1,   271,   198,   241,    -1,    61,   129,   201,   130,
      -1,    61,   129,   204,   130,    -1,   272,   215,    -1,   272,
     203,   215,    -1,   286,    89,   140,   230,    -1,    50,   286,
      89,   140,   230,    -1,   286,    89,   223,    -1,    50,   286,
      89,   223,    -1,   286,    89,   286,    89,   140,   230,    -1,
      50,   286,    89,   286,    89,   140,   230,    -1,   286,    89,
     286,    89,   223,    -1,    50,   286,    89,   286,    89,   223,
      -1,   286,    89,    -1,   203,   286,    89,    -1,   140,   230,
      -1,    50,   140,   230,    -1,     8,   140,   230,    -1,   223,
      -1,    50,   223,    -1,   272,   215,    -1,     8,   272,   215,
      -1,   286,    89,   207,    -1,   272,   203,   210,    -1,   207,
      -1,   272,   210,    -1,     8,   274,   210,    -1,    -1,    -1,
      46,   272,   129,   208,   233,   130,   209,   217,    -1,    -1,
     212,   211,   217,    -1,    -1,    -1,    46,   330,   213,   129,
     214,   233,   130,    -1,    -1,   219,   216,   217,    -1,    -1,
     137,    16,    -1,    45,    16,    -1,    43,    -1,   128,    -1,
     131,   333,   132,    -1,    -1,   271,   129,   220,   233,   130,
      -1,    -1,    -1,   271,   133,   221,   282,   134,   129,   222,
     233,   130,    -1,    -1,   225,   224,   227,    -1,    -1,   271,
     129,   226,   233,   130,    -1,    -1,   135,   229,   228,    -1,
      -1,   136,   229,   228,    -1,   271,   129,   333,   130,    -1,
     231,    -1,    -1,   271,   129,   232,   233,   130,    -1,    -1,
      -1,   234,   235,    -1,    88,    -1,   237,    -1,    -1,   237,
     136,   236,   235,    -1,    -1,    -1,   238,   274,   252,   239,
     241,    -1,    60,    -1,   271,    -1,   287,   271,    -1,    -1,
     242,    -1,    -1,   137,   243,   294,    -1,   272,   245,   247,
     128,    -1,    58,    60,   247,   128,    -1,    60,   247,   128,
      -1,   254,   241,    -1,    -1,   249,   247,    -1,    -1,    -1,
     247,   136,   248,   249,    -1,    -1,   250,   245,    -1,    -1,
     287,   251,   245,    -1,   262,   264,    -1,    -1,   256,   260,
     130,   253,   258,    -1,   263,   264,    -1,    -1,   257,   261,
     130,   255,   258,    -1,   129,    -1,    90,    -1,    91,    -1,
      90,    -1,    91,    -1,    -1,    -1,   129,   259,   233,   130,
      -1,   265,    -1,   252,    -1,   287,   252,    -1,   254,    -1,
     287,   254,    -1,    -1,   263,    -1,   271,    -1,    -1,   265,
      -1,    -1,   266,   267,    -1,    -1,   269,   147,   268,   270,
     148,    -1,    -1,   267,    -1,    -1,   177,    -1,    57,    -1,
       9,    -1,    38,    -1,    37,    -1,    92,    -1,    93,    -1,
     274,    -1,    51,   274,    -1,    59,   274,    -1,    59,    10,
     274,    -1,    50,   274,    -1,   273,   274,    -1,    50,   273,
     274,    -1,    58,    -1,    58,    50,    -1,   275,    -1,   275,
     287,    -1,   277,    -1,   276,   277,    -1,   277,   276,    -1,
      43,    -1,   290,    -1,   279,    -1,   285,    -1,    -1,    53,
     278,   284,    -1,    -1,    57,   133,   280,   282,   134,    -1,
      -1,     9,   133,   281,   282,   134,    -1,   274,    -1,    -1,
     274,   136,   283,   282,    -1,    57,    -1,     9,    -1,    38,
      -1,    37,    -1,    92,    -1,    93,    -1,   279,    -1,   285,
      -1,   286,    89,   284,    -1,   279,    89,   284,    -1,     9,
      -1,    57,    -1,    38,    -1,    37,    -1,    92,    -1,    93,
      -1,   144,    -1,   288,   144,    -1,   288,    -1,   289,    -1,
     288,   289,    -1,   141,    -1,    44,    -1,   291,    -1,     4,
     292,    -1,     3,   292,    -1,    40,     9,    -1,    40,    57,
      -1,    39,     9,    -1,    39,    57,    -1,   293,    -1,   292,
      -1,    92,    -1,    93,    -1,    37,    -1,    38,    -1,     9,
      -1,    57,    -1,    33,    -1,    34,    -1,    35,    -1,    36,
      -1,    95,    -1,    96,    -1,    97,    -1,    98,    -1,    99,
      -1,   100,    -1,   101,    -1,   102,    -1,   103,    -1,   104,
      -1,    94,    -1,    17,    -1,    18,    -1,    19,    -1,    30,
      -1,    31,    -1,    32,    -1,    20,    -1,    21,    -1,    22,
      -1,    23,    -1,    24,    -1,    25,    -1,    26,    -1,    27,
      -1,    28,    -1,    29,    -1,    48,    -1,    47,    -1,   299,
      -1,    -1,   131,   295,   294,   297,   296,   132,    -1,    -1,
     136,    -1,    -1,    -1,   297,   136,   298,   294,    -1,   306,
      -1,    -1,   139,   300,   306,    -1,    -1,   138,   301,   306,
      -1,   305,    -1,    -1,   129,   302,   299,   130,    -1,    -1,
      -1,     9,   133,   303,   275,   134,   129,   304,   306,   130,
      -1,    10,    -1,   305,    10,    -1,    16,    -1,    11,    -1,
      13,    -1,    12,    -1,    14,    -1,    15,    -1,     9,    -1,
      57,    -1,    -1,   105,   129,   271,   136,   308,   274,   130,
      -1,    -1,    -1,    -1,   106,   129,   309,   271,   136,   310,
     274,   311,   130,    -1,    -1,   107,   129,   312,   271,   130,
      -1,    -1,   108,   129,   313,   271,   130,    -1,    -1,    -1,
     109,   129,   271,   136,   314,   290,   315,   136,   334,   130,
      -1,    -1,   110,   129,   271,   136,   316,   290,   130,    -1,
      -1,    -1,    -1,   111,   129,   317,   271,   136,   318,   290,
     319,   130,    -1,   112,   129,   271,   136,   290,   130,    -1,
      -1,   113,   129,   271,   136,   320,   290,   130,    -1,    -1,
     117,   129,   271,   136,   321,   290,   130,    -1,    -1,   114,
     129,   271,   136,   322,   290,   130,    -1,    -1,   118,   129,
     271,   136,   323,   290,   130,    -1,    -1,   115,   129,   271,
     136,   324,   290,   130,    -1,    -1,   119,   129,   271,   136,
     325,   290,   130,    -1,    -1,   116,   129,   271,   136,   326,
     290,   130,    -1,    -1,   120,   129,   271,   136,   327,   290,
     130,    -1,    -1,   121,   129,   271,   136,   328,   290,   136,
      11,   130,    -1,    -1,   122,   129,   271,   136,   329,   290,
     136,    11,   130,    -1,   123,   129,   271,   130,    -1,   124,
     129,   271,   130,    -1,   125,   129,   271,   136,   271,   296,
     130,    -1,   129,   130,    -1,   147,   148,    -1,    62,   147,
     148,    -1,    63,   147,   148,    -1,   331,    -1,   137,    -1,
     141,    -1,   142,    -1,   138,    -1,   139,    -1,   149,    -1,
     140,    -1,   136,    -1,   133,    -1,   134,    -1,   144,    -1,
     145,    -1,   146,    -1,   143,    -1,    62,    -1,    63,    -1,
      64,    -1,    65,    -1,    66,    -1,    67,    -1,    68,    -1,
      69,    -1,    72,    -1,    73,    -1,    74,    -1,    75,    -1,
      76,    -1,    70,    -1,    71,    -1,    77,    -1,    78,    -1,
      79,    -1,    80,    -1,    81,    -1,    82,    -1,    83,    -1,
      84,    -1,    85,    -1,    86,    -1,    87,    -1,   126,    -1,
      -1,   333,   335,    -1,    -1,   334,   336,    -1,   128,    -1,
     336,    -1,    42,    -1,   337,    -1,   339,    -1,   338,    -1,
      54,    -1,   331,    -1,   135,    -1,   150,    -1,    89,    -1,
       4,    -1,    52,    -1,    38,    -1,    37,    -1,    92,    -1,
      93,    -1,   293,    -1,    13,    -1,    11,    -1,    12,    -1,
      14,    -1,    15,    -1,    10,    -1,    41,    -1,    43,    -1,
      44,    -1,    45,    -1,     3,    -1,    46,    -1,    58,    -1,
      50,    -1,     8,    -1,    39,    -1,    40,    -1,    53,    -1,
      16,    -1,    60,    -1,    88,    -1,     5,    -1,     7,    -1,
       6,    -1,    55,    -1,    56,    -1,    59,    -1,     9,    -1,
      57,    -1,   332,    -1,   131,   333,   132,    -1,   147,   333,
     148,    -1,   129,   333,   130,    -1,    90,   333,   130,    -1,
      91,   333,   130,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   956,   956,   957,   956,   961,   962,   963,   964,   965,
     966,   967,   968,   969,   970,   971,   972,   973,   974,   975,
     976,   977,   978,   979,   980,   981,   982,   983,   989,   995,
     995,   997,  1003,  1003,  1005,  1005,  1007,  1007,  1009,  1009,
    1011,  1014,  1015,  1014,  1018,  1019,  1020,  1021,  1022,  1023,
    1024,  1025,  1026,  1027,  1028,  1029,  1031,  1032,  1033,  1035,
    1036,  1037,  1038,  1039,  1040,  1042,  1042,  1044,  1044,  1046,
    1047,  1048,  1049,  1056,  1057,  1058,  1068,  1068,  1070,  1070,
    1073,  1073,  1073,  1075,  1076,  1078,  1079,  1079,  1079,  1081,
    1081,  1081,  1081,  1081,  1083,  1084,  1084,  1089,  1089,  1095,
    1095,  1102,  1102,  1103,  1105,  1105,  1106,  1106,  1107,  1107,
    1108,  1108,  1114,  1115,  1117,  1119,  1121,  1122,  1123,  1125,
    1126,  1127,  1133,  1156,  1157,  1158,  1159,  1161,  1167,  1168,
    1168,  1172,  1173,  1173,  1176,  1186,  1194,  1194,  1206,  1207,
    1209,  1209,  1209,  1216,  1218,  1224,  1226,  1227,  1228,  1229,
    1230,  1231,  1232,  1233,  1234,  1236,  1237,  1239,  1240,  1241,
    1246,  1247,  1248,  1249,  1257,  1258,  1261,  1262,  1263,  1273,
    1277,  1272,  1292,  1292,  1304,  1305,  1304,  1312,  1312,  1324,
    1325,  1334,  1344,  1350,  1350,  1353,  1352,  1357,  1358,  1357,
    1367,  1367,  1377,  1377,  1379,  1379,  1381,  1381,  1383,  1385,
    1399,  1399,  1405,  1405,  1405,  1408,  1409,  1410,  1410,  1413,
    1415,  1413,  1444,  1468,  1468,  1470,  1470,  1472,  1472,  1479,
    1480,  1481,  1483,  1494,  1495,  1497,  1498,  1498,  1501,  1501,
    1502,  1502,  1506,  1507,  1507,  1518,  1519,  1519,  1529,  1530,
    1532,  1535,  1537,  1540,  1541,  1541,  1543,  1546,  1547,  1551,
    1552,  1555,  1555,  1557,  1559,  1559,  1561,  1561,  1563,  1563,
    1565,  1565,  1567,  1568,  1574,  1575,  1576,  1577,  1578,  1579,
    1586,  1587,  1588,  1589,  1591,  1592,  1594,  1598,  1599,  1601,
    1602,  1604,  1605,  1606,  1608,  1610,  1611,  1613,  1615,  1615,
    1619,  1619,  1621,  1621,  1624,  1624,  1624,  1626,  1627,  1628,
    1629,  1630,  1631,  1632,  1633,  1635,  1641,  1648,  1648,  1648,
    1648,  1648,  1648,  1664,  1665,  1666,  1671,  1672,  1684,  1685,
    1688,  1689,  1690,  1691,  1692,  1693,  1694,  1697,  1698,  1701,
    1702,  1703,  1704,  1705,  1706,  1709,  1710,  1711,  1712,  1713,
    1714,  1715,  1716,  1717,  1718,  1719,  1720,  1721,  1722,  1723,
    1724,  1725,  1726,  1727,  1728,  1729,  1731,  1732,  1734,  1735,
    1737,  1738,  1740,  1741,  1743,  1744,  1746,  1747,  1753,  1754,
    1754,  1763,  1763,  1765,  1766,  1766,  1774,  1775,  1775,  1776,
    1776,  1779,  1780,  1780,  1781,  1782,  1781,  1790,  1791,  1798,
    1799,  1800,  1801,  1802,  1803,  1804,  1806,  1816,  1816,  1829,
    1830,  1830,  1829,  1842,  1842,  1855,  1855,  1867,  1867,  1867,
    1910,  1909,  1923,  1924,  1924,  1923,  1936,  1964,  1964,  1969,
    1969,  1974,  1974,  1979,  1979,  1984,  1984,  1989,  1989,  1994,
    1994,  1999,  1999,  2004,  2004,  2024,  2024,  2042,  2096,  2152,
    2229,  2230,  2231,  2232,  2233,  2235,  2236,  2236,  2237,  2237,
    2238,  2238,  2239,  2239,  2240,  2240,  2241,  2241,  2242,  2243,
    2244,  2245,  2246,  2247,  2248,  2249,  2250,  2251,  2252,  2253,
    2254,  2255,  2256,  2257,  2258,  2259,  2260,  2261,  2262,  2263,
    2264,  2265,  2266,  2267,  2268,  2274,  2297,  2297,  2298,  2298,
    2300,  2300,  2302,  2302,  2302,  2302,  2302,  2303,  2303,  2303,
    2303,  2303,  2303,  2304,  2304,  2304,  2304,  2304,  2305,  2305,
    2305,  2305,  2305,  2306,  2306,  2306,  2306,  2306,  2306,  2307,
    2307,  2307,  2307,  2307,  2307,  2307,  2308,  2308,  2308,  2308,
    2308,  2308,  2309,  2309,  2309,  2309,  2309,  2309,  2311,  2312,
    2313,  2313,  2313
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
  "VTK_CONSTANT_DEF", "VTK_BYTE_SWAP_DECL", "';'", "'('", "')'", "'{'",
  "'}'", "'<'", "'>'", "':'", "','", "'='", "'-'", "'+'", "'~'", "'*'",
  "'/'", "'%'", "'&'", "'|'", "'^'", "'['", "']'", "'!'", "'.'", "$accept",
  "strt", "$@1", "file_item", "extern", "namespace", "$@2", "class_def",
  "$@3", "$@4", "$@5", "$@6", "class_def_body", "$@7", "class_def_item",
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
  "op_token", "op_token_no_delim", "vtk_constant_def", "maybe_other",
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
     154,   154,   154,   154,   154,   154,   154,   154,   155,   157,
     156,   156,   159,   158,   160,   158,   161,   158,   162,   158,
     158,   163,   164,   163,   165,   165,   165,   165,   165,   165,
     165,   165,   165,   165,   165,   165,   165,   165,   165,   165,
     165,   165,   165,   165,   165,   166,   166,   167,   167,   168,
     168,   168,   168,   169,   169,   169,   171,   170,   172,   170,
     173,   173,   173,   174,   174,   175,   175,   175,   175,   176,
     176,   176,   176,   176,   177,   178,   177,   179,   177,   180,
     177,   181,   181,   181,   182,   182,   182,   182,   182,   182,
     182,   182,   183,   183,   184,   185,   186,   186,   186,   187,
     187,   187,   188,   188,   188,   188,   188,   189,   190,   191,
     190,   192,   193,   192,   194,   194,   195,   194,   196,   196,
     197,   198,   197,   199,   200,   201,   202,   202,   202,   202,
     202,   202,   202,   202,   202,   203,   203,   204,   204,   204,
     204,   204,   204,   204,   205,   205,   206,   206,   206,   208,
     209,   207,   211,   210,   213,   214,   212,   216,   215,   217,
     217,   217,   217,   218,   218,   220,   219,   221,   222,   219,
     224,   223,   226,   225,   227,   227,   228,   228,   229,   230,
     232,   231,   233,   234,   233,   235,   235,   236,   235,   238,
     239,   237,   237,   240,   240,   241,   241,   243,   242,   244,
     244,   244,   245,   246,   246,   247,   248,   247,   250,   249,
     251,   249,   252,   253,   252,   254,   255,   254,   256,   256,
     256,   257,   257,   258,   259,   258,   258,   260,   260,   261,
     261,   262,   262,   263,   264,   264,   266,   265,   268,   267,
     269,   269,   270,   270,   271,   271,   271,   271,   271,   271,
     272,   272,   272,   272,   272,   272,   272,   273,   273,   274,
     274,   275,   275,   275,   276,   277,   277,   277,   278,   277,
     280,   279,   281,   279,   282,   283,   282,   284,   284,   284,
     284,   284,   284,   284,   284,   285,   285,   286,   286,   286,
     286,   286,   286,   287,   287,   287,   288,   288,   289,   289,
     290,   290,   290,   290,   290,   290,   290,   291,   291,   292,
     292,   292,   292,   292,   292,   293,   293,   293,   293,   293,
     293,   293,   293,   293,   293,   293,   293,   293,   293,   293,
     293,   293,   293,   293,   293,   293,   293,   293,   293,   293,
     293,   293,   293,   293,   293,   293,   293,   293,   294,   295,
     294,   296,   296,   297,   298,   297,   299,   300,   299,   301,
     299,   299,   302,   299,   303,   304,   299,   305,   305,   306,
     306,   306,   306,   306,   306,   306,   306,   308,   307,   309,
     310,   311,   307,   312,   307,   313,   307,   314,   315,   307,
     316,   307,   317,   318,   319,   307,   307,   320,   307,   321,
     307,   322,   307,   323,   307,   324,   307,   325,   307,   326,
     307,   327,   307,   328,   307,   329,   307,   307,   307,   307,
     330,   330,   330,   330,   330,   331,   331,   331,   331,   331,
     331,   331,   331,   331,   331,   331,   331,   331,   331,   331,
     331,   331,   331,   331,   331,   331,   331,   331,   331,   331,
     331,   331,   331,   331,   331,   331,   331,   331,   331,   331,
     331,   331,   331,   331,   331,   332,   333,   333,   334,   334,
     335,   335,   336,   336,   336,   336,   336,   336,   336,   336,
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
       3,     2,     1,     1,     2,     4,     4,     1,     5,     0,
       6,     4,     0,     7,     0,    10,     0,     7,     0,    10,
       4,     0,     0,     3,     2,     1,     3,     3,     1,     1,
       1,     2,     1,     1,     2,     3,     3,     2,     3,     3,
       2,     5,     1,     1,     1,     0,     2,     1,     3,     1,
       2,     2,     2,     1,     1,     1,     0,     6,     0,     5,
       0,     1,     3,     1,     3,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     0,     3,     0,     4,     0,
       4,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     5,     4,     3,     2,     3,     3,     2,     1,
       5,     3,     4,     4,     4,     4,     3,     1,     3,     0,
       5,     1,     0,     4,     2,     2,     0,     3,     1,     1,
       0,     0,     3,     4,     4,     2,     3,     4,     5,     3,
       4,     6,     7,     5,     6,     2,     3,     2,     3,     3,
       1,     2,     2,     3,     3,     3,     1,     2,     3,     0,
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
       1,     2,     2,     3,     2,     2,     3,     1,     2,     1,
       2,     1,     2,     2,     1,     1,     1,     1,     0,     3,
       0,     5,     0,     5,     1,     0,     4,     1,     1,     1,
       1,     1,     1,     1,     1,     3,     3,     1,     1,     1,
       1,     1,     1,     1,     2,     1,     1,     2,     1,     1,
       1,     2,     2,     2,     2,     2,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     0,
       6,     0,     1,     0,     0,     4,     1,     0,     3,     0,
       3,     1,     0,     4,     0,     0,     9,     1,     2,     1,
       1,     1,     1,     1,     1,     1,     1,     0,     7,     0,
       0,     0,     9,     0,     5,     0,     5,     0,     0,    10,
       0,     7,     0,     0,     0,     9,     6,     0,     7,     0,
       7,     0,     7,     0,     7,     0,     7,     0,     7,     0,
       7,     0,     7,     0,     9,     0,     9,     4,     4,     7,
       2,     2,     3,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     0,     2,     0,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
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
       2,     3,     1,     0,     0,     0,     0,   333,   350,   351,
     352,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   353,   354,   355,   335,   336,   337,   338,   331,   332,
      78,     0,    14,   284,     0,   367,   366,     0,     0,     0,
     288,   127,     0,   488,   334,   277,     0,   225,     0,   329,
     330,   349,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   485,    27,     4,    10,     9,   228,   228,
     228,     8,    11,     0,     0,     0,     0,     0,     0,     0,
     166,     5,     0,     0,   270,   279,     0,   281,   286,   287,
       0,   285,   320,   328,   327,    22,    23,   333,   331,   332,
     334,   329,   330,   486,    36,   322,    32,   321,     0,     0,
     333,     0,     0,   334,     0,     0,   486,   292,   325,   267,
     266,   326,   268,   269,     0,    76,   323,   324,   486,     0,
       0,   277,     0,     0,     0,   274,     0,   271,   129,     0,
     307,   310,   309,   308,   311,   312,   486,    29,     0,   486,
     290,   278,   225,     0,   272,     0,     0,     0,   399,   403,
     405,     0,     0,   412,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   319,   318,
     313,     0,   225,     0,   230,   315,   316,     0,     0,     0,
       0,     0,     0,     0,   228,     0,     0,     0,   183,   486,
      21,    18,    19,    17,    15,   265,   267,   266,     0,   264,
     241,   242,   268,   269,     0,   167,   172,   145,   177,   225,
     215,     0,   254,   253,     0,   275,   280,   282,   283,     0,
       0,    24,     0,     0,    65,     0,    65,   333,   331,   332,
     334,   329,   330,   325,   326,   323,   324,   168,     0,     0,
       0,    80,     0,     0,   486,     0,   169,   276,     0,   128,
     136,   298,   300,   299,   297,   301,   302,   303,   289,   304,
       0,     0,   518,   501,   529,   531,   530,   522,   535,   513,
     509,   510,   508,   511,   512,   526,   504,   503,   523,   524,
     514,   492,   515,   516,   517,   519,   521,   502,   525,   496,
     532,   533,   536,   520,   534,   527,   459,   460,   461,   462,
     463,   464,   465,   466,   472,   473,   467,   468,   469,   470,
     471,   474,   475,   476,   477,   478,   479,   480,   481,   482,
     483,   484,   528,   500,   486,   486,   505,   506,   114,   486,
     486,   453,   454,   498,   452,   445,   448,   449,   451,   446,
     447,   458,   455,   456,   457,   486,   450,   499,   507,   497,
     537,   489,   493,   495,   494,     0,     0,     0,     2,   273,
     221,   226,     0,     0,   265,   264,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    12,   224,   229,
     253,     0,   314,   317,     6,     7,   126,     0,   213,     0,
       0,     0,     0,     0,    20,    16,     0,     0,   459,   460,
       0,     0,   174,   444,   165,   146,     0,   179,   179,     0,
     217,   222,   216,   249,     0,     0,   235,   255,   260,   185,
     187,   155,   306,   298,   300,   299,   297,   301,   302,     0,
     164,   149,   190,     0,   305,     0,   490,    40,   487,   491,
     294,     0,     0,     0,     0,     0,    25,     0,     0,    81,
      83,    80,   113,     0,   203,     0,   150,     0,   138,   139,
       0,   131,     0,   140,   140,    31,     2,     0,     0,     0,
       0,     0,    26,     0,   220,     3,   228,   143,   397,     0,
       0,     0,   407,   410,     0,     0,   417,   421,   425,   429,
     419,   423,   427,   431,   433,   435,   437,   438,     0,   231,
     123,   214,   124,   125,   122,    13,   184,     0,     0,   440,
     441,     0,   156,   182,     0,     0,   173,   178,   219,     0,
     236,   250,   257,     0,   203,     0,   147,   199,     0,   194,
     192,     0,   295,    38,     0,     0,     0,    66,    67,    69,
      41,    34,    41,   293,    79,    80,     0,     0,   112,     0,
     209,   148,     0,   130,   132,   140,   135,   141,   134,     3,
     541,   542,   540,   538,   539,   291,    28,   227,     0,   400,
     404,   406,     0,     0,   413,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   371,   442,   443,   175,
     181,   180,   395,   387,   390,   392,   391,   393,   394,   389,
     396,   382,   369,   379,   377,   218,   368,   381,   376,   243,
     258,     0,     0,   200,     0,   191,   203,     0,   153,     0,
      65,    72,    70,    71,     0,    42,    65,    42,    82,   265,
      90,    92,    91,    93,    89,   264,    99,   101,   102,   103,
      94,    85,    84,    95,    86,    88,    87,    77,   170,   212,
     205,   204,   206,     0,     0,   154,   136,   137,   215,    30,
       0,     0,   408,     0,     0,   416,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   372,     0,   203,   384,
       0,     0,     0,     0,   388,   244,   237,   246,   262,   186,
       0,   203,   196,     0,     0,   151,   296,     0,    68,    37,
       0,     0,    33,     0,   104,   105,   106,   107,   108,   109,
     110,   111,    97,     0,   179,   207,   251,   152,   133,   142,
     398,   401,     0,   411,   414,   418,   422,   426,   430,   420,
     424,   428,   432,     0,     0,   439,     0,     0,     0,   373,
     395,   380,   378,   203,   263,     0,   188,     0,     0,   195,
     486,   193,    41,     0,     0,    73,    74,    75,     0,   333,
     331,   332,    53,     0,     0,   334,     0,   329,   330,     0,
      64,     0,    43,     0,   228,   228,    48,    52,    50,    49,
       0,     0,     0,     0,   160,    45,     0,    62,    63,    41,
       0,     0,    96,   171,   209,   239,   240,   238,   210,   251,
     254,   252,     0,   488,     0,     0,     0,   176,     0,   383,
     371,     0,   259,   203,   201,   196,     0,    42,   119,   486,
     488,   118,     0,     0,     0,     0,   270,    51,     0,     0,
       0,     0,   161,     0,   486,   157,    44,     0,     0,   115,
       0,     0,    60,    57,    54,   162,    42,   100,    98,   208,
     215,   247,     0,   251,   232,   402,     0,   415,   434,   436,
       0,   374,     0,   245,     0,   197,   198,    39,     0,     0,
     117,   116,   159,   163,    58,    55,   158,     0,     0,     0,
       0,    46,    47,    59,    56,    35,   211,   233,   248,   409,
     385,     0,   370,   189,   488,   121,   144,     0,   243,     0,
     375,     0,    61,   234,     0,   120,   386
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     3,    85,    86,    87,   291,    88,   256,   666,
     254,   660,   665,   740,   812,   483,   577,   578,   813,    89,
     272,   144,   488,   489,   680,   681,   682,   753,   831,   743,
     683,   752,    90,    91,   817,   818,   861,    92,    93,    94,
     280,   500,   696,   501,   502,   503,   596,   698,    95,   821,
      96,    97,   234,   822,    98,    99,   100,   494,   754,   235,
     447,   236,   551,   718,   237,   448,   556,   220,   238,   564,
     565,   853,   824,   569,   472,   656,   655,   789,   732,   566,
     567,   731,   589,   590,   691,   834,   692,   693,   890,   427,
     451,   452,   559,   101,   239,   201,   175,   516,   202,   203,
     421,   838,   938,   240,   649,   839,   241,   726,   783,   892,
     454,   840,   242,   456,   457,   458,   562,   728,   563,   785,
     473,   865,   103,   104,   105,   106,   107,   159,   108,   386,
     270,   481,   659,   474,   109,   135,   204,   205,   206,   111,
     112,   113,   114,   645,   721,   717,   850,   931,   646,   723,
     722,   720,   777,   939,   647,   648,   115,   608,   397,   701,
     842,   398,   399,   612,   762,   613,   402,   704,   844,   616,
     620,   617,   621,   618,   622,   619,   623,   624,   625,   442,
     379,   380,   252,   168,   478,   479,   382,   383,   384
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -784
static const yytype_int16 yypact[] =
{
    -784,    83,  -784,  4462,   459,   636,  5200,     7,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,   -23,   -15,
     668,   492,  -784,  -784,  4690,  -784,  -784,  4792,  5200,    -8,
    -784,  -784,   539,  -784,   160,    30,  4894,  -784,   -40,    -7,
       3,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,    82,    89,   106,   109,   114,   125,   136,   137,
     147,   152,   162,   166,   167,   168,   173,   177,   179,   180,
     191,   192,   193,  -784,  -784,  -784,  -784,  -784,    28,    28,
      28,  -784,  -784,  4996,  4588,   154,   154,   154,   154,   154,
    -784,  -784,   508,  5200,  -784,    24,  5302,   169,    60,  -784,
      86,  -784,  -784,  -784,  -784,  -784,  -784,   128,   170,   215,
     271,   280,   485,  -784,    46,  -784,   190,  -784,   776,   776,
       6,    50,    61,    11,   279,   235,  -784,  -784,   195,  -784,
    -784,   205,  -784,  -784,   206,  -784,   195,   205,  -784,   213,
    4792,   301,  5098,   224,  5200,  -784,   277,  -784,   234,   799,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  3133,  -784,
    -784,  -784,  -784,  4347,  -784,   119,  4690,   802,  -784,  -784,
    -784,   802,   802,  -784,   802,   802,   802,   802,   802,   802,
     802,   802,   802,   802,   802,   802,   802,   802,  -784,  -784,
    -784,   241,  -784,   772,  -784,    53,  -784,   247,   248,   251,
     363,   363,   363,   772,    28,   154,   154,   625,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,   296,   297,   302,  5501,   303,
    -784,  -784,   307,   308,   788,  -784,  -784,  -784,  -784,  -784,
     253,   290,   256,   -45,   316,  -784,  -784,  -784,  -784,   799,
     237,  -784,   913,  5200,   274,  5200,   274,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,   799,  1061,
    5200,   802,   283,  1209,  -784,  5200,  -784,  -784,   278,  -784,
    5505,     6,   297,   302,    11,   307,   308,    60,  -784,  -784,
    1357,   285,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  1505,  5200,   144,  -784,  -784,
    -784,  -784,   292,   802,  -784,  -784,   287,   802,   802,   802,
     288,   293,   802,   294,   305,   314,   317,   324,   325,   326,
     327,   328,   330,   337,   298,   322,   343,  -784,   347,  -784,
    -784,   772,  -784,  -784,  -784,  -784,  -784,   282,  -784,   802,
     304,   356,   357,   358,  -784,  -784,   -45,  1653,   342,   345,
     364,   350,  -784,  -784,  -784,  -784,   404,    20,    20,   164,
    -784,  -784,  -784,  -784,   375,   772,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,   -27,   -18,   -10,    69,    81,   107,   802,
    -784,  -784,  -784,   377,  -784,   429,  -784,  -784,  -784,  -784,
     383,   386,   678,   390,   391,   395,  -784,   394,   399,   396,
     289,   802,  -784,  1801,   403,   802,  -784,   447,  -784,  -784,
     410,   414,   501,   802,   802,  -784,  -784,  1949,  2097,  2245,
    2393,  2541,  -784,   421,  -784,   434,    24,  -784,  -784,   425,
     437,   438,  -784,  -784,   439,  5404,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,   802,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,   422,   432,  -784,
    -784,   440,  -784,  -784,   557,   570,  -784,  -784,  -784,   433,
    -784,  -784,   442,   445,   403,  5200,  -784,  -784,   464,   460,
    -784,   281,  -784,  -784,   799,   799,   799,  -784,   461,  -784,
    -784,  -784,  -784,  -784,  -784,   802,   465,   470,  -784,   479,
     143,  -784,   295,  -784,  -784,   802,  -784,  -784,  -784,   478,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  5200,  -784,
    -784,  -784,  5404,  5404,  -784,   482,  5404,  5404,  5404,  5404,
    5404,  5404,  5404,  5404,  5404,  5404,   483,  -784,  -784,  -784,
    -784,  -784,   489,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,   605,  -784,   -44,
    -784,   495,   493,  -784,   802,  -784,   403,   802,  -784,  5200,
     274,  -784,  -784,  -784,   678,   497,   274,   498,  -784,     6,
    -784,  -784,  -784,  -784,  -784,    11,  -784,  -784,  -784,  -784,
     705,  -784,  -784,  -784,  -784,    60,  -784,  -784,  -784,  -784,
    -784,  -784,   510,  5200,   802,  -784,  5505,  -784,   253,  -784,
     503,  5200,  -784,   505,  5404,  -784,   514,   518,   521,   524,
     525,   529,   530,   531,   520,   528,  -784,   537,   403,  -784,
     499,   433,   526,   526,  -784,  -784,  -784,  -784,   465,  -784,
     540,   403,   532,   543,   546,  -784,  -784,   547,  -784,  -784,
    3725,   549,  -784,   465,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,   465,    20,  -784,   550,  -784,  -784,  -784,
    -784,  -784,   545,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,   675,   677,  -784,   559,  5200,   562,  -784,
    -784,  -784,  -784,   403,  -784,   548,  -784,   565,   802,  -784,
    -784,  -784,  -784,   486,   636,  -784,  -784,  -784,  4099,   -27,
     -18,   -10,  -784,  3851,  4223,    69,   568,    81,   107,   569,
    -784,   802,  -784,   567,    28,    28,  -784,  -784,  -784,  -784,
    3851,   154,   154,   154,  -784,  -784,   685,  -784,  -784,  -784,
     574,   465,  -784,  -784,   143,  -784,  -784,  -784,  -784,   220,
     256,  -784,   577,  -784,   578,   579,   580,  -784,   566,  -784,
     575,   582,  -784,   403,  -784,   532,  2689,   581,  -784,  -784,
    -784,  -784,   232,   232,   802,   802,   279,  -784,   154,   154,
     625,   802,  -784,  3975,  -784,  -784,  -784,   591,   592,  -784,
     154,   154,  -784,  -784,  -784,  -784,   594,  -784,  -784,  -784,
     253,  -784,   600,   550,  -784,  -784,  3281,  -784,  -784,  -784,
     607,   613,   615,  -784,   620,  -784,  -784,  -784,  2837,  3429,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  4099,   623,   802,
    2985,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,   433,  -784,  -784,  -784,  -784,  -784,   609,   -44,   526,
    -784,  3577,  -784,  -784,   627,  -784,  -784
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -784,  -339,  -784,  -784,  -784,  -784,  -784,    15,  -784,  -784,
    -784,  -784,  -551,  -784,  -784,  -233,    94,  -784,  -784,   -75,
    -784,  -784,  -438,  -784,  -784,  -784,  -676,  -784,  -784,  -784,
    -784,  -784,   -74,    25,  -784,  -727,  -662,    26,  -784,  -461,
    -784,    68,  -784,  -784,  -784,  -784,  -429,  -784,  -784,  -784,
     -33,  -784,  -784,  -739,  -784,   -82,   522,  -784,  -784,  -120,
    -784,  -784,  -784,  -784,  -218,  -784,  -423,   -89,  -784,  -784,
    -784,  -784,  -230,  -784,  -784,  -784,  -784,   -87,   -14,  -452,
    -784,  -784,  -514,  -784,   -61,  -784,  -784,  -784,  -784,    22,
    -638,  -784,  -784,    42,  -173,   -52,   -85,  -784,   267,  -784,
    -784,  -783,  -784,  -184,  -784,  -784,  -784,  -151,  -784,  -784,
    -784,  -784,  -698,   -47,  -621,  -784,  -784,  -784,  -784,  -784,
      -4,     0,   -35,    -2,    21,   692,   694,  -784,  -117,  -784,
    -784,  -223,  -784,  -137,  -104,    36,   -88,  -784,   596,  -397,
    -256,     1,  -153,  -681,  -784,   -46,  -784,  -784,    85,  -784,
    -784,  -784,  -784,  -784,  -784,  -677,    66,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
    -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,  -784,
     584,     8,  -115,  -774,  -784,  -155,  -784,  -784,  -784
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -373
static const yytype_int16 yytable[] =
{
     124,   126,   154,   102,   134,   125,   127,   221,   222,   223,
     224,   116,   216,   381,   267,   378,   445,   246,   211,   212,
     471,   269,   288,   485,   504,   557,   145,   149,   727,   432,
     419,   667,   484,   273,   153,   155,   157,   207,   208,   110,
     779,   595,   287,   591,   174,   781,   782,   487,   496,   515,
     651,   290,   784,   587,   385,   289,   891,   453,   841,   263,
     759,   215,  -307,   553,   868,   554,  -310,   830,   198,   896,
     265,  -310,   198,   156,  -309,   598,   867,   832,   167,  -309,
     171,   880,  -311,     2,   459,   725,   909,   387,   460,   176,
     172,   213,  -312,   879,   217,  -307,  -307,   198,   243,   378,
    -308,   245,  -265,  -256,   437,  -310,   137,   264,   210,   214,
     928,  -267,   462,  -309,   444,   154,   378,   418,   266,  -266,
     378,  -311,   429,   429,   429,   158,   434,   435,   615,   125,
     127,  -312,   287,   287,   918,  -307,   136,   378,   244,   137,
     137,   841,   734,   392,   170,   289,   289,   668,   155,   249,
     174,   287,   277,   455,   449,   888,  -223,   555,  -308,   493,
     941,   287,   433,   513,   289,   199,   697,   599,   200,   199,
    -311,   389,   200,   396,   289,   250,   393,   400,   401,   253,
     403,   404,   405,   406,   407,   408,   409,   410,   411,   412,
     413,   414,   415,   416,   199,   841,  -312,   422,  -264,   420,
     910,   911,   170,   689,   776,   735,   428,   428,   428,   420,
    -268,   177,    33,   436,   251,   702,   703,   787,   178,   706,
     707,   708,   709,   710,   711,   712,   713,   714,   715,   394,
     436,   690,   378,   430,   431,   179,  -269,   420,   180,   507,
     508,   857,   757,   181,   509,   510,   463,   390,   539,  -308,
     940,   480,   926,   480,   182,   391,  -265,   139,   140,  -265,
     511,  -265,   944,  -265,   198,   183,   184,   490,   480,   851,
     446,   561,   514,   389,   464,   465,   185,   395,   886,   820,
     391,   186,   218,    34,   378,   219,   475,   463,  -308,   169,
     463,   187,   558,   170,   466,   188,   189,   190,  -267,   394,
     391,  -267,   191,  -267,   463,  -267,   192,   764,   193,   194,
     835,   836,   142,   143,   497,   464,   465,   727,   464,   465,
     195,   196,   197,   255,   268,   228,  -265,   139,   140,   467,
     468,   833,   464,   465,   198,   466,  -264,   271,   466,   904,
     378,   658,   652,  -266,   274,   579,  -266,   395,  -266,   837,
    -266,   171,   466,   276,   378,   378,   378,   378,   378,   875,
     858,   199,   695,   859,   200,   287,   278,   860,   279,   417,
     467,   468,   394,   467,   468,   424,   425,   469,   289,   426,
     230,   231,   142,   143,   480,  -307,  -310,   467,   468,   436,
     450,  -309,  -308,   519,   520,   521,  -311,  -312,   524,  -264,
     139,   140,  -264,  -256,  -264,   461,  -264,   198,  -268,   482,
     540,  -268,   912,  -268,   491,  -268,   506,   420,   495,   916,
     395,   657,   517,   518,   522,   541,   586,   737,   536,   523,
     525,   199,   542,   741,   200,   694,   736,   661,   662,   663,
     504,   526,   632,   633,   634,   635,   636,   637,   638,   639,
     527,   420,   537,   528,   287,   142,   143,   287,   287,   287,
     529,   530,   531,   532,   533,   568,   534,   289,   117,   685,
     289,   289,   289,   535,   669,   287,   670,   671,   672,   538,
     673,   674,   686,   391,   543,   544,   545,   490,   289,   547,
     640,   568,   548,   552,   549,   117,   118,   119,   550,   597,
     597,   146,   226,   227,   199,   560,   570,   200,   632,   633,
     634,   635,   636,   637,   638,   639,   120,   225,   571,   572,
     573,   580,   675,   118,   119,   581,   582,   579,   583,   139,
     140,   584,   585,  -202,   626,   780,   592,   634,   635,   636,
     637,   638,   639,   120,   593,   226,   227,   287,   160,   147,
     594,   121,   122,    39,   228,   605,   640,   232,   233,   394,
     289,   609,   641,   480,   642,   229,   606,   610,   611,   629,
     627,   643,   644,   630,   872,   614,   161,   162,   121,   122,
     628,   490,   684,   640,   142,   143,   631,   139,   140,  -261,
     123,   597,   650,   653,   676,   654,   163,   664,   230,   231,
     232,   233,   687,   677,   678,   679,   700,   395,   885,   688,
     699,   685,   705,  -269,   858,   724,  -269,   859,  -269,   716,
    -269,   860,   719,   148,   686,   729,   685,   730,   641,   739,
     742,   164,   165,   760,   394,   763,   685,   643,   644,   686,
     835,   836,   142,   143,   765,   117,   755,   913,   766,   686,
     733,   767,   885,   568,   768,   769,   773,   480,   823,   770,
     771,   772,   139,   140,   774,   814,   815,   775,   788,   786,
     166,   228,   790,   118,   119,   856,   791,   138,   792,   837,
     829,   843,   395,   574,   575,   576,   845,   281,   846,   847,
     568,   756,   849,   120,   394,   854,   852,   873,   874,   761,
     900,   885,   876,   378,   887,   139,   140,   895,   897,   898,
     899,   901,   903,   907,   685,   282,   283,   142,   143,   921,
     922,   869,   139,   140,   684,   141,   925,   686,   121,   122,
     927,   228,   882,   883,   884,   284,   930,   942,   881,   684,
     826,   381,   395,   378,   908,  -372,   267,   932,   828,   684,
     933,   893,   420,   936,   381,   378,   378,   946,   738,   920,
     142,   143,   877,   878,   758,   816,   819,   378,   905,   154,
     285,   286,   470,   889,   855,   230,   231,   142,   143,   914,
     915,   394,   825,   607,   733,   257,   381,   943,   378,   862,
     863,   923,   924,   894,   125,   127,   866,   225,   848,   248,
     247,   423,   155,   870,   902,   778,   827,   568,   281,   139,
     140,   394,   443,   258,   259,     0,     0,     0,     0,     0,
     870,     0,   243,     0,     0,   226,   227,   684,     0,   395,
       0,     0,     0,   260,   228,   420,   282,   283,     0,   139,
     140,     0,     0,   744,   745,   229,   746,   747,   748,   749,
     750,   751,     0,     0,     0,     0,   284,     0,     0,   395,
     568,   436,   230,   231,   142,   143,   436,   568,   261,   262,
       0,     0,     0,   919,     0,     0,     0,     0,     0,     0,
     232,   233,     0,     0,     0,     0,     0,     0,     0,   420,
       0,   285,   286,     0,   142,   143,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   436,   292,   293,   294,   295,
     296,   297,   298,   299,   300,   301,   302,   303,   304,   305,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
     306,   307,   308,   309,   310,   311,   312,   313,   314,   315,
      35,    36,     0,   316,     0,   317,   318,   319,   320,   321,
     322,   323,   324,   325,     0,   326,   327,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,   356,   357,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    83,
       0,   476,   359,     0,   360,   477,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,     0,   376,   377,   292,   293,   294,   295,   296,   297,
     298,   299,   300,   301,   302,   303,   304,   305,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,   306,   307,
     308,   309,   310,   311,   312,   313,   314,   315,    35,    36,
       0,   316,     0,   317,   318,   319,   320,   321,   322,   323,
     324,   325,     0,   326,   327,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,   356,   357,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    83,     0,   476,
     359,   486,   360,     0,   361,   362,   363,   364,   365,   366,
     367,   368,   369,   370,   371,   372,   373,   374,   375,     0,
     376,   377,   292,   293,   294,   295,   296,   297,   298,   299,
     300,   301,   302,   303,   304,   305,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,   306,   307,   308,   309,
     310,   311,   312,   313,   314,   315,    35,    36,     0,   316,
       0,   317,   318,   319,   320,   321,   322,   323,   324,   325,
       0,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    83,     0,   476,   359,     0,
     360,   492,   361,   362,   363,   364,   365,   366,   367,   368,
     369,   370,   371,   372,   373,   374,   375,     0,   376,   377,
     292,   293,   294,   295,   296,   297,   298,   299,   300,   301,
     302,   303,   304,   305,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,   306,   307,   308,   309,   310,   311,
     312,   313,   314,   315,    35,    36,     0,   316,     0,   317,
     318,   319,   320,   321,   322,   323,   324,   325,     0,   326,
     327,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,   356,
     357,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    83,     0,   476,   359,     0,   360,   505,
     361,   362,   363,   364,   365,   366,   367,   368,   369,   370,
     371,   372,   373,   374,   375,     0,   376,   377,   292,   293,
     294,   295,   296,   297,   298,   299,   300,   301,   302,   303,
     304,   305,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,   306,   307,   308,   309,   310,   311,   312,   313,
     314,   315,    35,    36,     0,   316,     0,   317,   318,   319,
     320,   321,   322,   323,   324,   325,     0,   326,   327,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    83,     0,   476,   359,   512,   360,     0,   361,   362,
     363,   364,   365,   366,   367,   368,   369,   370,   371,   372,
     373,   374,   375,     0,   376,   377,   292,   293,   294,   295,
     296,   297,   298,   299,   300,   301,   302,   303,   304,   305,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
     306,   307,   308,   309,   310,   311,   312,   313,   314,   315,
      35,    36,     0,   316,     0,   317,   318,   319,   320,   321,
     322,   323,   324,   325,     0,   326,   327,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,   356,   357,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    83,
       0,   476,   359,     0,   360,   546,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,     0,   376,   377,   292,   293,   294,   295,   296,   297,
     298,   299,   300,   301,   302,   303,   304,   305,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,   306,   307,
     308,   309,   310,   311,   312,   313,   314,   315,    35,    36,
       0,   316,     0,   317,   318,   319,   320,   321,   322,   323,
     324,   325,     0,   326,   327,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,   356,   357,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    83,     0,   476,
     359,     0,   360,   588,   361,   362,   363,   364,   365,   366,
     367,   368,   369,   370,   371,   372,   373,   374,   375,     0,
     376,   377,   292,   293,   294,   295,   296,   297,   298,   299,
     300,   301,   302,   303,   304,   305,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,   306,   307,   308,   309,
     310,   311,   312,   313,   314,   315,    35,    36,     0,   316,
       0,   317,   318,   319,   320,   321,   322,   323,   324,   325,
       0,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    83,     0,   476,   359,   600,
     360,     0,   361,   362,   363,   364,   365,   366,   367,   368,
     369,   370,   371,   372,   373,   374,   375,     0,   376,   377,
     292,   293,   294,   295,   296,   297,   298,   299,   300,   301,
     302,   303,   304,   305,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,   306,   307,   308,   309,   310,   311,
     312,   313,   314,   315,    35,    36,     0,   316,     0,   317,
     318,   319,   320,   321,   322,   323,   324,   325,     0,   326,
     327,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,   356,
     357,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    83,     0,   476,   359,   601,   360,     0,
     361,   362,   363,   364,   365,   366,   367,   368,   369,   370,
     371,   372,   373,   374,   375,     0,   376,   377,   292,   293,
     294,   295,   296,   297,   298,   299,   300,   301,   302,   303,
     304,   305,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,   306,   307,   308,   309,   310,   311,   312,   313,
     314,   315,    35,    36,     0,   316,     0,   317,   318,   319,
     320,   321,   322,   323,   324,   325,     0,   326,   327,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    83,     0,   476,   359,   602,   360,     0,   361,   362,
     363,   364,   365,   366,   367,   368,   369,   370,   371,   372,
     373,   374,   375,     0,   376,   377,   292,   293,   294,   295,
     296,   297,   298,   299,   300,   301,   302,   303,   304,   305,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
     306,   307,   308,   309,   310,   311,   312,   313,   314,   315,
      35,    36,     0,   316,     0,   317,   318,   319,   320,   321,
     322,   323,   324,   325,     0,   326,   327,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,   356,   357,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    83,
       0,   476,   359,     0,   360,   603,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,     0,   376,   377,   292,   293,   294,   295,   296,   297,
     298,   299,   300,   301,   302,   303,   304,   305,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,   306,   307,
     308,   309,   310,   311,   312,   313,   314,   315,    35,    36,
       0,   316,     0,   317,   318,   319,   320,   321,   322,   323,
     324,   325,     0,   326,   327,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,   356,   357,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    83,     0,   476,
     359,     0,   360,     0,   361,   362,   363,   364,   365,   366,
     367,   368,   369,   370,   371,   372,   373,   374,   375,   604,
     376,   377,   292,   293,   294,   295,   296,   297,   298,   299,
     300,   301,   302,   303,   304,   305,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,   306,   307,   308,   309,
     310,   311,   312,   313,   314,   315,    35,    36,     0,   316,
       0,   317,   318,   319,   320,   321,   322,   323,   324,   325,
       0,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    83,     0,   476,   359,   906,
     360,     0,   361,   362,   363,   364,   365,   366,   367,   368,
     369,   370,   371,   372,   373,   374,   375,     0,   376,   377,
     292,   293,   294,   295,   296,   297,   298,   299,   300,   301,
     302,   303,   304,   305,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,   306,   307,   308,   309,   310,   311,
     312,   313,   314,   315,    35,    36,     0,   316,     0,   317,
     318,   319,   320,   321,   322,   323,   324,   325,     0,   326,
     327,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,   356,
     357,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    83,     0,   476,   359,     0,   360,   934,
     361,   362,   363,   364,   365,   366,   367,   368,   369,   370,
     371,   372,   373,   374,   375,     0,   376,   377,   292,   293,
     294,   295,   296,   297,   298,   299,   300,   301,   302,   303,
     304,   305,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,   306,   307,   308,   309,   310,   311,   312,   313,
     314,   315,    35,    36,     0,   316,     0,   317,   318,   319,
     320,   321,   322,   323,   324,   325,     0,   326,   327,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    83,     0,   476,   359,   937,   360,     0,   361,   362,
     363,   364,   365,   366,   367,   368,   369,   370,   371,   372,
     373,   374,   375,     0,   376,   377,   292,   293,   294,   295,
     296,   297,   298,   299,   300,   301,   302,   303,   304,   305,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
     306,   307,   308,   309,   310,   311,   312,   313,   314,   315,
      35,    36,     0,   316,     0,   317,   318,   319,   320,   321,
     322,   323,   324,   325,     0,   326,   327,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,   356,   357,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    83,
       0,   358,   359,     0,   360,     0,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,     0,   376,   377,   292,   293,   294,   295,   296,   297,
     298,   299,   300,   301,   302,   303,   304,   305,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,   306,   307,
     308,   309,   310,   311,   312,   313,   314,   315,    35,    36,
       0,   316,     0,   317,   318,   319,   320,   321,   322,   323,
     324,   325,     0,   326,   327,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,   356,   357,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    83,     0,     0,
     359,   929,   360,     0,   361,   362,   363,   364,   365,   366,
     367,   368,   369,   370,   371,   372,   373,   374,   375,     0,
     376,   377,   292,   293,   294,   295,   296,   297,   298,   299,
     300,   301,   302,   303,   304,   305,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,   306,   307,   308,   309,
     310,   311,   312,   313,   314,   315,    35,    36,     0,   316,
       0,   317,   318,   319,   320,   321,   322,   323,   324,   325,
       0,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    83,     0,   935,   359,     0,
     360,     0,   361,   362,   363,   364,   365,   366,   367,   368,
     369,   370,   371,   372,   373,   374,   375,     0,   376,   377,
     292,   293,   294,   295,   296,   297,   298,   299,   300,   301,
     302,   303,   304,   305,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,   306,   307,   308,   309,   310,   311,
     312,   313,   314,   315,    35,    36,     0,   316,     0,   317,
     318,   319,   320,   321,   322,   323,   324,   325,     0,   326,
     327,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,   356,
     357,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    83,     0,   945,   359,     0,   360,     0,
     361,   362,   363,   364,   365,   366,   367,   368,   369,   370,
     371,   372,   373,   374,   375,     0,   376,   377,   793,   794,
     795,   796,   797,   798,   799,     0,     0,     0,     0,     0,
       0,     0,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,   800,   801,    30,    31,   802,     0,    33,     0,
       0,    34,    35,    36,   803,   804,    38,    39,    40,    41,
       0,    43,   805,    45,   152,    47,   806,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   807,   808,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,   809,   810,   793,   794,     0,     0,     0,   798,
     799,     0,     0,     0,     0,   811,     0,     0,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,   800,   801,
     131,   132,     0,     0,    33,     0,     0,    34,    35,    36,
       0,   804,    38,     0,    40,     0,     0,     0,   805,   151,
     152,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   807,   808,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   128,   129,
       0,     0,     0,   917,   799,     0,     0,     0,     0,     0,
       0,   811,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,   800,   801,   131,   132,     0,     0,    33,     0,
       0,     0,    35,    36,     0,   804,    38,     0,    40,     0,
       0,     0,   805,   151,   152,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   807,   808,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   128,   129,     0,     0,     0,     0,   130,     0,
       0,     0,     0,     0,     0,   811,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,   131,   132,
       0,     0,    33,     0,     0,     0,    35,    36,     0,   150,
      38,     0,    40,     0,     0,     0,   133,   151,   152,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   128,   129,     0,     0,
       0,     0,   799,     0,     0,     0,     0,     0,     0,   864,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
     800,   801,   131,   132,     0,     0,    33,     0,     0,     0,
      35,    36,     0,     0,     0,     0,    40,     0,     0,     0,
     805,   151,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   807,   808,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     128,   129,     0,     0,     0,     0,   130,     0,     0,     0,
       0,     0,     0,   871,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,   131,   132,     0,     0,
      33,     0,     0,     0,    35,    36,     0,     0,     0,     0,
      40,     0,     0,     0,   133,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     4,     5,     0,     0,     0,
       6,     7,     0,     0,     0,     0,     0,     0,   388,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,     0,    33,     0,     0,    34,    35,
      36,     0,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,     0,     0,     0,     0,     0,     0,
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
       0,    33,     0,     0,    34,    35,    36,     0,   150,    38,
       0,    40,     0,     0,     0,   133,   151,   152,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,   128,   129,     0,     0,     0,     0,   130,
       0,     0,     0,     0,     0,     0,     0,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,   131,
     132,     0,     0,    33,     0,     0,     0,    35,    36,     0,
     150,    38,     0,    40,     0,     0,     0,   133,   151,   152,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,   128,   129,     0,     0,     0,
       0,   130,     0,     0,     0,     0,     0,     0,     0,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,   131,   132,     0,     0,    33,     0,     0,     0,    35,
      36,     0,     0,     0,     0,    40,     0,     0,     0,   133,
     151,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,   128,   129,     0,
       0,     0,     0,   130,   173,     0,     0,     0,     0,     0,
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,   131,   132,     0,     0,    33,     0,     0,
       0,    35,    36,     0,     0,     0,     0,    40,     0,     0,
       0,   133,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,     4,
       5,     0,     0,     0,     0,   130,     0,     0,     0,     0,
       0,     0,     0,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,     0,     0,    33,
       0,     0,     0,    35,    36,     0,     0,     0,     0,    40,
       0,     0,     0,   133,     0,     0,   209,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,   128,   129,     0,     0,     0,     0,   130,   275,     0,
       0,     0,     0,     0,     0,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,   131,   132,     0,
       0,    33,     0,     0,     0,    35,    36,     0,     0,     0,
       0,    40,     0,     0,     0,   133,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,   128,   129,     0,     0,     0,     0,   130,
       0,     0,     0,     0,     0,     0,     0,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,   131,
     132,     0,     0,    33,     0,     0,     0,    35,    36,     0,
       0,     0,     0,    40,     0,     0,     0,   133,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,   128,   129,     0,     0,     0,
       0,   130,     0,     0,     0,     0,     0,     0,     0,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,   131,   132,     0,     0,     0,     0,     0,     0,    35,
      36,     0,     0,     0,     0,    40,     0,     0,     0,   133,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,   128,   129,     0,
       0,     0,     0,   257,     0,     0,     0,     0,     0,     0,
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   258,   259,   131,   132,     0,     0,     0,     0,     0,
       0,    35,    36,     0,     0,     0,     0,     0,     0,     0,
       0,   260,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   261,   262,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,   498,
       0,     0,     0,     0,   257,     0,     0,     0,     0,     0,
       0,     0,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,   258,   259,     0,     0,     0,     0,     0,     0,
       0,     0,    35,    36,     0,     0,     0,     0,   499,     0,
       0,     0,   260,   438,   439,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,     0,
       0,     0,     0,     0,     0,     0,     0,   261,   262,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     440,     0,     0,     0,   361,   362,     0,   364,   365,   366,
     367,   368,   369,   370,   371,   372,   373,   374,   441,     0,
     376
};

static const yytype_int16 yycheck[] =
{
       4,     5,    37,     3,     6,     4,     5,    96,    97,    98,
      99,     3,    94,   168,   134,   168,   234,   105,    93,    93,
     250,   136,   159,   256,   280,   448,    30,    31,   649,   213,
     203,   582,   255,   148,    34,    37,    38,    89,    90,     3,
     721,   502,   159,   495,    46,   722,   723,   270,   278,   388,
     564,   166,   728,   491,   169,   159,   839,   241,   756,     9,
     698,    94,    89,    43,   803,    45,    89,   743,    44,   843,
       9,    89,    44,    37,    89,   504,   803,   753,    42,    89,
      50,   820,    89,     0,   129,   129,   860,   172,   133,   129,
      60,    93,    89,   820,    94,    89,    89,    44,   102,   252,
      89,   103,   129,   147,   219,   128,   133,    57,    93,    94,
     893,   129,   249,   128,   234,   150,   269,   202,    57,   129,
     273,   128,   210,   211,   212,   133,   215,   216,   525,   128,
     129,   128,   249,   250,   873,   128,   129,   290,   102,   133,
     133,   839,   656,   176,   133,   249,   250,   585,   150,    89,
     152,   268,   154,   241,   239,   831,   128,   137,    89,   274,
     934,   278,   214,   386,   268,   141,   595,   506,   144,   141,
      89,   173,   144,   177,   278,    89,   176,   181,   182,   133,
     184,   185,   186,   187,   188,   189,   190,   191,   192,   193,
     194,   195,   196,   197,   141,   893,    89,   144,   129,   203,
     862,   863,   133,    60,   718,   657,   210,   211,   212,   213,
     129,   129,    43,   217,   128,   612,   613,   731,   129,   616,
     617,   618,   619,   620,   621,   622,   623,   624,   625,     9,
     234,    88,   385,   211,   212,   129,   129,   241,   129,   354,
     355,   792,   694,   129,   359,   360,     9,   128,   421,    89,
     931,   253,   890,   255,   129,   136,   128,    37,    38,   131,
     375,   133,   939,   135,    44,   129,   129,   271,   270,   783,
     234,   455,   128,   275,    37,    38,   129,    57,   829,   740,
     136,   129,   128,    46,   437,   131,   250,     9,   128,   129,
       9,   129,   128,   133,    57,   129,   129,   129,   128,     9,
     136,   131,   129,   133,     9,   135,   129,   704,   129,   129,
      90,    91,    92,    93,   278,    37,    38,   938,    37,    38,
     129,   129,   129,   133,    89,    46,   131,    37,    38,    92,
      93,   754,    37,    38,    44,    57,   131,   131,    57,   853,
     493,   571,   565,   128,   131,   482,   131,    57,   133,   129,
     135,    50,    57,   129,   507,   508,   509,   510,   511,   811,
     128,   141,   592,   131,   144,   482,    89,   135,   134,   128,
      92,    93,     9,    92,    93,   128,   128,   140,   482,   128,
      90,    91,    92,    93,   386,    89,    89,    92,    93,   393,
     137,    89,    89,   397,   398,   399,    89,    89,   402,   128,
      37,    38,   131,   147,   133,    89,   135,    44,   128,   135,
     128,   131,   864,   133,   131,   135,   131,   421,   140,   871,
      57,   140,   130,   136,   136,   429,   137,   660,   130,   136,
     136,   141,   128,   666,   144,   140,   659,   574,   575,   576,
     696,   136,     9,    10,    11,    12,    13,    14,    15,    16,
     136,   455,   130,   136,   571,    92,    93,   574,   575,   576,
     136,   136,   136,   136,   136,   469,   136,   571,     9,   586,
     574,   575,   576,   136,     9,   592,    11,    12,    13,   136,
      15,    16,   586,   136,   128,   128,   128,   491,   592,   147,
      57,   495,   147,    89,   130,     9,    37,    38,   148,   503,
     504,     9,    37,    38,   141,   130,   129,   144,     9,    10,
      11,    12,    13,    14,    15,    16,    57,     9,    89,   136,
     134,   131,    57,    37,    38,   134,   131,   664,   134,    37,
      38,   132,   136,   130,   538,     9,    89,    11,    12,    13,
      14,    15,    16,    57,   134,    37,    38,   664,     9,    57,
     136,    92,    93,    52,    46,   134,    57,    92,    93,     9,
     664,   136,   129,   565,   131,    57,   132,   130,   130,   129,
     148,   138,   139,    16,   804,   136,    37,    38,    92,    93,
     148,   585,   586,    57,    92,    93,    16,    37,    38,   147,
     131,   595,   147,   129,   129,   135,    57,   136,    90,    91,
      92,    93,   132,   138,   139,   140,   608,    57,   826,   130,
     132,   728,   130,   128,   128,    10,   131,   131,   133,   136,
     135,   135,   133,   131,   728,   130,   743,   134,   129,   132,
     132,    92,    93,   130,     9,   130,   753,   138,   139,   743,
      90,    91,    92,    93,   130,     9,   136,   865,   130,   753,
     654,   130,   870,   657,   130,   130,   136,   659,   740,   130,
     130,   130,    37,    38,   136,   740,   740,   130,   136,   129,
     131,    46,   129,    37,    38,   790,   130,     9,   131,   129,
     131,   136,    57,     5,     6,     7,    11,     9,    11,   130,
     694,   693,   130,    57,     9,   130,   148,   129,   129,   701,
     134,   919,   135,   856,   130,    37,    38,   130,   130,   130,
     130,   136,   130,   132,   831,    37,    38,    92,    93,   128,
     128,   803,    37,    38,   728,    57,   132,   831,    92,    93,
     130,    46,   821,   822,   823,    57,   129,   128,   820,   743,
     740,   896,    57,   896,   859,   132,   866,   132,   740,   753,
     130,   839,   756,   130,   909,   908,   909,   130,   664,   874,
      92,    93,   814,   815,   696,   740,   740,   920,   855,   804,
      92,    93,   250,   834,   788,    90,    91,    92,    93,   868,
     869,     9,   740,   516,   788,     9,   941,   938,   941,   793,
     794,   880,   881,   840,   793,   794,   798,     9,   777,   107,
     106,   205,   804,   803,   850,   720,   740,   811,     9,    37,
      38,     9,   228,    37,    38,    -1,    -1,    -1,    -1,    -1,
     820,    -1,   826,    -1,    -1,    37,    38,   831,    -1,    57,
      -1,    -1,    -1,    57,    46,   839,    37,    38,    -1,    37,
      38,    -1,    -1,   138,   139,    57,   141,   142,   143,   144,
     145,   146,    -1,    -1,    -1,    -1,    57,    -1,    -1,    57,
     864,   865,    90,    91,    92,    93,   870,   871,    92,    93,
      -1,    -1,    -1,   873,    -1,    -1,    -1,    -1,    -1,    -1,
      92,    93,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   893,
      -1,    92,    93,    -1,    92,    93,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   919,     3,     4,     5,     6,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   126,
      -1,   128,   129,    -1,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     147,    -1,   149,   150,     3,     4,     5,     6,     7,     8,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   126,    -1,   128,
     129,   130,   131,    -1,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,    -1,
     149,   150,     3,     4,     5,     6,     7,     8,     9,    10,
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
      -1,    -1,    -1,    -1,    -1,   126,    -1,   128,   129,    -1,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,    -1,   149,   150,
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
      -1,    -1,    -1,   126,    -1,   128,   129,    -1,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   147,    -1,   149,   150,     3,     4,
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
      -1,   126,    -1,   128,   129,   130,   131,    -1,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,    -1,   149,   150,     3,     4,     5,     6,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   126,
      -1,   128,   129,    -1,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     147,    -1,   149,   150,     3,     4,     5,     6,     7,     8,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   126,    -1,   128,
     129,    -1,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,    -1,
     149,   150,     3,     4,     5,     6,     7,     8,     9,    10,
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
      -1,    -1,    -1,    -1,    -1,   126,    -1,   128,   129,   130,
     131,    -1,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,    -1,   149,   150,
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
      -1,    -1,    -1,   126,    -1,   128,   129,   130,   131,    -1,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   147,    -1,   149,   150,     3,     4,
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
      -1,   126,    -1,   128,   129,   130,   131,    -1,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,    -1,   149,   150,     3,     4,     5,     6,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   126,
      -1,   128,   129,    -1,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     147,    -1,   149,   150,     3,     4,     5,     6,     7,     8,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   126,    -1,   128,
     129,    -1,   131,    -1,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
     149,   150,     3,     4,     5,     6,     7,     8,     9,    10,
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
      -1,    -1,    -1,    -1,    -1,   126,    -1,   128,   129,   130,
     131,    -1,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,    -1,   149,   150,
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
      -1,    -1,    -1,   126,    -1,   128,   129,    -1,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   147,    -1,   149,   150,     3,     4,
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
      -1,   126,    -1,   128,   129,   130,   131,    -1,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,    -1,   149,   150,     3,     4,     5,     6,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   126,
      -1,   128,   129,    -1,   131,    -1,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     147,    -1,   149,   150,     3,     4,     5,     6,     7,     8,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   126,    -1,    -1,
     129,   130,   131,    -1,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,    -1,
     149,   150,     3,     4,     5,     6,     7,     8,     9,    10,
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
      -1,    -1,    -1,    -1,    -1,   126,    -1,   128,   129,    -1,
     131,    -1,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,    -1,   149,   150,
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
      -1,    -1,    -1,   126,    -1,   128,   129,    -1,   131,    -1,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   147,    -1,   149,   150,     3,     4,
       5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    -1,    43,    -1,
      -1,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      -1,    56,    57,    58,    59,    60,    61,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,     3,     4,    -1,    -1,    -1,     8,
       9,    -1,    -1,    -1,    -1,   140,    -1,    -1,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    -1,    -1,    43,    -1,    -1,    46,    47,    48,
      -1,    50,    51,    -1,    53,    -1,    -1,    -1,    57,    58,
      59,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,
      -1,    -1,    -1,     8,     9,    -1,    -1,    -1,    -1,    -1,
      -1,   140,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    -1,    -1,    43,    -1,
      -1,    -1,    47,    48,    -1,    50,    51,    -1,    53,    -1,
      -1,    -1,    57,    58,    59,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     3,     4,    -1,    -1,    -1,    -1,     9,    -1,
      -1,    -1,    -1,    -1,    -1,   140,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      -1,    -1,    43,    -1,    -1,    -1,    47,    48,    -1,    50,
      51,    -1,    53,    -1,    -1,    -1,    57,    58,    59,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     3,     4,    -1,    -1,
      -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,   140,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    -1,    -1,    43,    -1,    -1,    -1,
      47,    48,    -1,    -1,    -1,    -1,    53,    -1,    -1,    -1,
      57,    58,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       3,     4,    -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,
      -1,    -1,    -1,   140,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    -1,    -1,
      43,    -1,    -1,    -1,    47,    48,    -1,    -1,    -1,    -1,
      53,    -1,    -1,    -1,    57,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     3,     4,    -1,    -1,    -1,
       8,     9,    -1,    -1,    -1,    -1,    -1,    -1,   131,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    -1,    43,    -1,    -1,    46,    47,
      48,    -1,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,    -1,
     128,     3,     4,    -1,    -1,    -1,     8,     9,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     129,    -1,    -1,    -1,   133,   134,    -1,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,    -1,
     149
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   152,     0,   153,     3,     4,     8,     9,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    43,    46,    47,    48,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   128,   154,   155,   156,   158,   170,
     183,   184,   188,   189,   190,   199,   201,   202,   205,   206,
     207,   244,   272,   273,   274,   275,   276,   277,   279,   285,
     286,   290,   291,   292,   293,   307,   332,     9,    37,    38,
      57,    92,    93,   131,   271,   292,   271,   292,     3,     4,
       9,    39,    40,    57,   274,   286,   129,   133,     9,    37,
      38,    57,    92,    93,   172,   271,     9,    57,   131,   271,
      50,    58,    59,   272,   273,   274,   286,   274,   133,   278,
       9,    37,    38,    57,    92,    93,   131,   286,   334,   129,
     133,    50,    60,    10,   274,   247,   129,   129,   129,   129,
     129,   129,   129,   129,   129,   129,   129,   129,   129,   129,
     129,   129,   129,   129,   129,   129,   129,   129,    44,   141,
     144,   246,   249,   250,   287,   288,   289,   246,   246,    60,
     158,   170,   183,   274,   158,   201,   206,   272,   128,   131,
     218,   218,   218,   218,   218,     9,    37,    38,    46,    57,
      90,    91,    92,    93,   203,   210,   212,   215,   219,   245,
     254,   257,   263,   271,   286,   274,   287,   277,   276,    89,
      89,   128,   333,   133,   161,   133,   159,     9,    37,    38,
      57,    92,    93,     9,    57,     9,    57,   210,    89,   333,
     281,   131,   171,   333,   131,    10,   129,   274,    89,   134,
     191,     9,    37,    38,    57,    92,    93,   279,   284,   285,
     333,   157,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    50,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,   128,   129,
     131,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   147,   149,   150,   293,   331,
     332,   336,   337,   338,   339,   333,   280,   247,   131,   274,
     128,   136,   201,   272,     9,    57,   271,   309,   312,   313,
     271,   271,   317,   271,   271,   271,   271,   271,   271,   271,
     271,   271,   271,   271,   271,   271,   271,   128,   247,   245,
     271,   251,   144,   289,   128,   128,   128,   240,   271,   287,
     240,   240,   254,   246,   218,   218,   271,   333,    62,    63,
     129,   147,   330,   331,   210,   215,   286,   211,   216,   247,
     137,   241,   242,   254,   261,   287,   264,   265,   266,   129,
     133,    89,   284,     9,    37,    38,    57,    92,    93,   140,
     207,   223,   225,   271,   284,   286,   128,   132,   335,   336,
     274,   282,   135,   166,   282,   166,   130,   282,   173,   174,
     271,   131,   132,   333,   208,   140,   223,   286,     4,    53,
     192,   194,   195,   196,   291,   132,   131,   333,   333,   333,
     333,   333,   130,   282,   128,   152,   248,   130,   136,   271,
     271,   271,   136,   136,   271,   136,   136,   136,   136,   136,
     136,   136,   136,   136,   136,   136,   130,   130,   136,   245,
     128,   271,   128,   128,   128,   128,   132,   147,   147,   130,
     148,   213,    89,    43,    45,   137,   217,   217,   128,   243,
     130,   254,   267,   269,   220,   221,   230,   231,   271,   224,
     129,    89,   136,   134,     5,     6,     7,   167,   168,   284,
     131,   134,   131,   134,   132,   136,   137,   173,   132,   233,
     234,   230,    89,   134,   136,   190,   197,   271,   197,   152,
     130,   130,   130,   132,   148,   134,   132,   249,   308,   136,
     130,   130,   314,   316,   136,   290,   320,   322,   324,   326,
     321,   323,   325,   327,   328,   329,   271,   148,   148,   129,
      16,    16,     9,    10,    11,    12,    13,    14,    15,    16,
      57,   129,   131,   138,   139,   294,   299,   305,   306,   255,
     147,   233,   282,   129,   135,   227,   226,   140,   223,   283,
     162,   284,   284,   284,   136,   163,   160,   163,   173,     9,
      11,    12,    13,    15,    16,    57,   129,   138,   139,   140,
     175,   176,   177,   181,   271,   279,   285,   132,   130,    60,
      88,   235,   237,   238,   140,   223,   193,   197,   198,   132,
     274,   310,   290,   290,   318,   130,   290,   290,   290,   290,
     290,   290,   290,   290,   290,   290,   136,   296,   214,   133,
     302,   295,   301,   300,    10,   129,   258,   265,   268,   130,
     134,   232,   229,   271,   233,   230,   282,   166,   167,   132,
     164,   166,   132,   180,   138,   139,   141,   142,   143,   144,
     145,   146,   182,   178,   209,   136,   274,   230,   192,   241,
     130,   274,   315,   130,   290,   130,   130,   130,   130,   130,
     130,   130,   130,   136,   136,   130,   233,   303,   299,   294,
       9,   306,   306,   259,   177,   270,   129,   233,   136,   228,
     129,   130,   131,     3,     4,     5,     6,     7,     8,     9,
      37,    38,    41,    49,    50,    57,    61,    92,    93,   127,
     128,   140,   165,   169,   170,   183,   184,   185,   186,   188,
     190,   200,   204,   206,   223,   244,   272,   307,   332,   131,
     177,   179,   177,   217,   236,    90,    91,   129,   252,   256,
     262,   263,   311,   136,   319,    11,    11,   130,   275,   130,
     297,   233,   148,   222,   130,   229,   333,   163,   128,   131,
     135,   187,   271,   271,   140,   272,   274,   186,   204,   206,
     272,   140,   223,   129,   129,   230,   135,   246,   246,   186,
     204,   206,   218,   218,   218,   215,   163,   130,   177,   235,
     239,   252,   260,   287,   264,   130,   334,   130,   130,   130,
     134,   136,   296,   130,   233,   228,   130,   132,   333,   334,
     187,   187,   230,   215,   218,   218,   230,     8,   204,   272,
     333,   128,   128,   218,   218,   132,   241,   130,   252,   130,
     129,   298,   132,   130,   132,   128,   130,   130,   253,   304,
     294,   334,   128,   258,   306,   128,   130
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
#line 957 "vtkParse.y"
    { startSig(); clearTypeId(); clearTemplate(); }
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 971 "vtkParse.y"
    { output_function(); }
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 972 "vtkParse.y"
    { output_function(); }
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 973 "vtkParse.y"
    { reject_function(); }
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 974 "vtkParse.y"
    { output_function(); }
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 975 "vtkParse.y"
    { reject_function(); }
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 976 "vtkParse.y"
    { output_function(); }
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 977 "vtkParse.y"
    { legacySig(); output_function(); }
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 995 "vtkParse.y"
    { pushNamespace((yyvsp[(2) - (2)].str)); }
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 996 "vtkParse.y"
    { popNamespace(); }
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 1003 "vtkParse.y"
    { start_class((yyvsp[(2) - (2)].str), 0); }
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 1004 "vtkParse.y"
    { end_class(); }
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 1005 "vtkParse.y"
    { reject_class((yyvsp[(2) - (5)].str), 0); }
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 1006 "vtkParse.y"
    { end_class(); }
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 1007 "vtkParse.y"
    { start_class((yyvsp[(2) - (2)].str), 1); }
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 1008 "vtkParse.y"
    { end_class(); }
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 1009 "vtkParse.y"
    { reject_class((yyvsp[(2) - (5)].str), 1); }
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 1010 "vtkParse.y"
    { end_class(); }
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 1015 "vtkParse.y"
    { startSig(); clearTypeId(); clearTemplate(); }
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 1028 "vtkParse.y"
    { output_function(); }
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 1029 "vtkParse.y"
    { ClassInfo *tmpc = currentClass;
     currentClass = NULL; output_function(); currentClass = tmpc; }
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 1031 "vtkParse.y"
    { output_function(); }
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 1032 "vtkParse.y"
    { output_function(); }
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 1033 "vtkParse.y"
    { ClassInfo *tmpc = currentClass;
     currentClass = NULL; output_function(); currentClass = tmpc; }
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 1035 "vtkParse.y"
    { output_function(); }
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 1036 "vtkParse.y"
    { legacySig(); output_function(); }
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 1050 "vtkParse.y"
    {
      vtkParse_AddPointerToArray(&currentClass->SuperClasses,
                                 &currentClass->NumberOfSuperClasses,
                                 vtkstrdup((yyvsp[(2) - (2)].str)));
    }
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 1056 "vtkParse.y"
    {access_level = VTK_ACCESS_PUBLIC;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 1057 "vtkParse.y"
    {access_level = VTK_ACCESS_PROTECTED;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 1058 "vtkParse.y"
    {access_level = VTK_ACCESS_PRIVATE;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 1068 "vtkParse.y"
    {start_enum((yyvsp[(2) - (2)].str));}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 1069 "vtkParse.y"
    {end_enum();}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 1070 "vtkParse.y"
    {start_enum(NULL);}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 1071 "vtkParse.y"
    {end_enum();}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 1075 "vtkParse.y"
    {add_enum((yyvsp[(1) - (1)].str), NULL);}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 1076 "vtkParse.y"
    {add_enum((yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 1078 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 1083 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 1084 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 1085 "vtkParse.y"
    {
         (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (3)].str)) + strlen((yyvsp[(3) - (3)].str)) + 1);
         sprintf((yyval.str), "%s%s", (yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));
       }
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 1089 "vtkParse.y"
    {postSig((yyvsp[(2) - (2)].str));}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 1090 "vtkParse.y"
    {
         (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (4)].str)) + strlen((yyvsp[(2) - (4)].str)) +
                                  strlen((yyvsp[(4) - (4)].str)) + 3);
         sprintf((yyval.str), "%s %s %s", (yyvsp[(1) - (4)].str), (yyvsp[(2) - (4)].str), (yyvsp[(4) - (4)].str));
       }
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 1095 "vtkParse.y"
    {postSig("(");}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 1096 "vtkParse.y"
    {
         postSig(")");
         (yyval.str) = (char *)malloc(strlen((yyvsp[(3) - (4)].str)) + 3);
         sprintf((yyval.str), "(%s)", (yyvsp[(3) - (4)].str));
       }
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 1102 "vtkParse.y"
    { (yyval.str) = "-"; }
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 1102 "vtkParse.y"
    { (yyval.str) = "+"; }
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 1103 "vtkParse.y"
    { (yyval.str) = "~"; }
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 1105 "vtkParse.y"
    { (yyval.str) = "-"; }
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 1105 "vtkParse.y"
    { (yyval.str) = "+"; }
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 1106 "vtkParse.y"
    { (yyval.str) = "*"; }
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 1106 "vtkParse.y"
    { (yyval.str) = "/"; }
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 1107 "vtkParse.y"
    { (yyval.str) = "%"; }
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 1107 "vtkParse.y"
    { (yyval.str) = "&"; }
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 1108 "vtkParse.y"
    { (yyval.str) = "|"; }
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 1108 "vtkParse.y"
    { (yyval.str) = "^"; }
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 1134 "vtkParse.y"
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

  case 123:

/* Line 1455 of yacc.c  */
#line 1156 "vtkParse.y"
    { }
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 1157 "vtkParse.y"
    { }
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 1158 "vtkParse.y"
    { }
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1159 "vtkParse.y"
    { }
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1161 "vtkParse.y"
    { }
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1167 "vtkParse.y"
    { postSig("template<> "); clearTypeId(); }
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1168 "vtkParse.y"
    { postSig("template<");
          clearTypeId(); startTemplate(); }
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1170 "vtkParse.y"
    { postSig("> "); clearTypeId(); }
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1173 "vtkParse.y"
    { postSig(", "); clearTypeId(); }
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1177 "vtkParse.y"
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

  case 135:

/* Line 1455 of yacc.c  */
#line 1187 "vtkParse.y"
    {
               TemplateArg *arg = (TemplateArg *)malloc(sizeof(TemplateArg));
               vtkParse_InitTemplateArg(arg);
               arg->Name = vtkstrdup(getVarName());
               arg->Value = vtkstrdup(getVarValue());
               vtkParse_AddItemMacro2(currentTemplate, Arguments, arg);
               }
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1194 "vtkParse.y"
    { pushTemplate(); }
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1195 "vtkParse.y"
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

  case 138:

/* Line 1455 of yacc.c  */
#line 1206 "vtkParse.y"
    {postSig("class ");}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1207 "vtkParse.y"
    {postSig("typename ");}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 1209 "vtkParse.y"
    { setVarName((yyvsp[(1) - (1)].str)); }
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1239 "vtkParse.y"
    {openSig(); preSig("~"); closeSig();}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 1240 "vtkParse.y"
    {openSig(); preSig("~"); closeSig();}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 1242 "vtkParse.y"
    {
         openSig(); preSig("virtual ~"); closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1250 "vtkParse.y"
    {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1264 "vtkParse.y"
    {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 1273 "vtkParse.y"
    {
      postSig("(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 1277 "vtkParse.y"
    { postSig(")"); }
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1278 "vtkParse.y"
    {
      (yyval.integer) = (yyvsp[(2) - (8)].integer);
      postSig(";");
      preSig("operator ");
      closeSig();
      currentFunction->IsOperator = 1;
      currentFunction->Name = "operator typecast";
      if (HaveComment)
        {
        currentFunction->Comment = vtkstrdup(CommentText);
        }
      vtkParseDebug("Parsed operator", "operator typecast");
    }
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1292 "vtkParse.y"
    { postSig(")"); }
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1293 "vtkParse.y"
    {
      postSig(";");
      closeSig();
      currentFunction->Name = (yyvsp[(1) - (3)].str);
      if (HaveComment)
        {
        currentFunction->Comment = vtkstrdup(CommentText);
        }
      vtkParseDebug("Parsed operator", currentFunction->Name);
    }
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 1304 "vtkParse.y"
    {postSig((yyvsp[(2) - (2)].str));}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 1305 "vtkParse.y"
    {
      postSig("(");
      currentFunction->IsOperator = 1;
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 1310 "vtkParse.y"
    { (yyval.str) = (yyvsp[(2) - (7)].str); }
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 1312 "vtkParse.y"
    { postSig(")"); }
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1313 "vtkParse.y"
    {
      postSig(";");
      closeSig();
      currentFunction->Name = (yyvsp[(1) - (3)].str);
      if (HaveComment)
        {
        currentFunction->Comment = vtkstrdup(CommentText);
        }
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 1326 "vtkParse.y"
    {
      postSig(" = 0");
      if (currentClass)
        {
        currentFunction->IsPureVirtual = 1;
        currentClass->IsAbstract = 1;
        }
    }
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 1335 "vtkParse.y"
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

  case 182:

/* Line 1455 of yacc.c  */
#line 1345 "vtkParse.y"
    {
      postSig(" const");
      currentFunction->IsConst = 1;
    }
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 1353 "vtkParse.y"
    {
      postSig("(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 1356 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (5)].str); }
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 1357 "vtkParse.y"
    {markSig(); postSig("<");}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 1358 "vtkParse.y"
    {
      const char *cp;
      postSig(">(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
      cp = copySig();
      (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (6)].str)) + strlen(cp) + 1);
      sprintf((yyval.str), "%s%s", (yyvsp[(1) - (6)].str), cp);
    }
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 1365 "vtkParse.y"
    { (yyval.str) = (yyvsp[(7) - (9)].str); }
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 1367 "vtkParse.y"
    { postSig(");"); closeSig(); }
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 1368 "vtkParse.y"
    {
      currentFunction->Name = (yyvsp[(1) - (3)].str);
      if (HaveComment)
        {
        currentFunction->Comment = vtkstrdup(CommentText);
        }
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 1377 "vtkParse.y"
    { postSig("("); }
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 1386 "vtkParse.y"
    {
      postSig(");");
      closeSig();
      currentFunction->Name = (char *)malloc(strlen((yyvsp[(1) - (1)].str)) + 2);
      currentFunction->Name[0] = '~';
      strcpy(&currentFunction->Name[1], (yyvsp[(1) - (1)].str));
      if (HaveComment)
        {
        currentFunction->Comment = vtkstrdup(CommentText);
        }
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 1399 "vtkParse.y"
    { postSig("(");}
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 1405 "vtkParse.y"
    {clearTypeId();}
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 1408 "vtkParse.y"
    { currentFunction->IsVariadic = 1; postSig("..."); }
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 1409 "vtkParse.y"
    { clearTypeId(); }
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 1410 "vtkParse.y"
    { clearTypeId(); postSig(", "); }
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 1413 "vtkParse.y"
    { markSig(); }
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 1415 "vtkParse.y"
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

  case 211:

/* Line 1455 of yacc.c  */
#line 1437 "vtkParse.y"
    {
      int i = currentFunction->NumberOfArguments-1;
      if (getVarValue())
        {
        currentFunction->Arguments[i]->Value = vtkstrdup(getVarValue());
        }
    }
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 1445 "vtkParse.y"
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

  case 215:

/* Line 1455 of yacc.c  */
#line 1470 "vtkParse.y"
    {clearVarValue();}
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 1472 "vtkParse.y"
    { postSig("="); clearVarValue();}
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 1473 "vtkParse.y"
    { setVarValue((yyvsp[(3) - (3)].str)); }
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 1484 "vtkParse.y"
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

  case 226:

/* Line 1455 of yacc.c  */
#line 1498 "vtkParse.y"
    {postSig(", ");}
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 1501 "vtkParse.y"
    { setStorageTypeIndirection(0); }
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 1502 "vtkParse.y"
    { setStorageTypeIndirection((yyvsp[(1) - (1)].integer)); }
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 1506 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 1507 "vtkParse.y"
    { postSig(")"); }
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 1509 "vtkParse.y"
    {
         unsigned int parens = add_indirection((yyvsp[(1) - (5)].integer), (yyvsp[(2) - (5)].integer));
         if ((yyvsp[(5) - (5)].integer) == VTK_PARSE_FUNCTION) {
           (yyval.integer) = (parens | VTK_PARSE_FUNCTION); }
         else if ((yyvsp[(5) - (5)].integer) == VTK_PARSE_ARRAY) {
           (yyval.integer) = add_indirection_to_array(parens); }
       }
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 1518 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 1519 "vtkParse.y"
    { postSig(")"); }
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 1521 "vtkParse.y"
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
#line 1529 "vtkParse.y"
    { postSig("("); (yyval.integer) = 0; }
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 1530 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("*");
               (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 1532 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("&");
               (yyval.integer) = VTK_PARSE_REF; }
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 1535 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("*");
               (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 1537 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("&");
               (yyval.integer) = VTK_PARSE_REF; }
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 1540 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 1541 "vtkParse.y"
    { pushFunction(); postSig("("); }
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 1542 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_FUNCTION; postSig(")"); popFunction(); }
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 1543 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_ARRAY; }
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 1546 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 1548 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(1) - (2)].integer), (yyvsp[(2) - (2)].integer));}
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 1551 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 1553 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(1) - (2)].integer), (yyvsp[(2) - (2)].integer));}
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 1555 "vtkParse.y"
    {clearVarName(); chopSig();}
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 1557 "vtkParse.y"
    {setVarName((yyvsp[(1) - (1)].str));}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 1559 "vtkParse.y"
    {clearArray();}
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 1561 "vtkParse.y"
    {clearArray();}
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 1563 "vtkParse.y"
    {postSig("[");}
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 1563 "vtkParse.y"
    {postSig("]");}
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 1567 "vtkParse.y"
    {pushArraySize("");}
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 1568 "vtkParse.y"
    {pushArraySize((yyvsp[(1) - (1)].str));}
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 1574 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 1575 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 1576 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 1577 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 1578 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 1579 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 1586 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer); setStorageType((yyval.integer));}
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 1587 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 1588 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 1590 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(3) - (3)].integer); setStorageType((yyval.integer));}
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 1591 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 1592 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer));
      setStorageType((yyval.integer));}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 1594 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(2) - (3)].integer) | (yyvsp[(3) - (3)].integer));
      setStorageType((yyval.integer));}
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 1598 "vtkParse.y"
    {postSig("static "); (yyval.integer) = VTK_PARSE_STATIC; }
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 1599 "vtkParse.y"
    {postSig("static "); (yyval.integer) = VTK_PARSE_STATIC; }
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 1601 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 1602 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer));}
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 1604 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 1605 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_CONST | (yyvsp[(2) - (2)].integer));}
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 1606 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_CONST | (yyvsp[(1) - (2)].integer));}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 1608 "vtkParse.y"
    {postSig("const ");}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 1612 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 1614 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 1615 "vtkParse.y"
    {postSig("typename ");}
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 1616 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(3) - (3)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 1619 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 1620 "vtkParse.y"
    {chopSig(); postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 1621 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 1622 "vtkParse.y"
    {chopSig(); postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 295:

/* Line 1455 of yacc.c  */
#line 1624 "vtkParse.y"
    {postSig(", ");}
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 1626 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 1627 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 1628 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 1629 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 1630 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 302:

/* Line 1455 of yacc.c  */
#line 1631 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 305:

/* Line 1455 of yacc.c  */
#line 1636 "vtkParse.y"
    {
             (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (3)].str))+strlen((yyvsp[(3) - (3)].str))+3);
             sprintf((yyval.str), "%s::%s", (yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));
             preScopeSig((yyvsp[(1) - (3)].str));
           }
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 1642 "vtkParse.y"
    {
             (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (3)].str))+strlen((yyvsp[(3) - (3)].str))+3);
             sprintf((yyval.str), "%s::%s", (yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));
             preScopeSig("");
           }
    break;

  case 313:

/* Line 1455 of yacc.c  */
#line 1664 "vtkParse.y"
    { postSig("&"); (yyval.integer) = VTK_PARSE_REF;}
    break;

  case 314:

/* Line 1455 of yacc.c  */
#line 1665 "vtkParse.y"
    { postSig("&"); (yyval.integer) = ((yyvsp[(1) - (2)].integer) | VTK_PARSE_REF);}
    break;

  case 315:

/* Line 1455 of yacc.c  */
#line 1666 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 316:

/* Line 1455 of yacc.c  */
#line 1671 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 317:

/* Line 1455 of yacc.c  */
#line 1673 "vtkParse.y"
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

  case 318:

/* Line 1455 of yacc.c  */
#line 1684 "vtkParse.y"
    { postSig("*"); (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 1685 "vtkParse.y"
    { postSig("*const "); (yyval.integer) = VTK_PARSE_CONST_POINTER; }
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 1688 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 321:

/* Line 1455 of yacc.c  */
#line 1689 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); }
    break;

  case 322:

/* Line 1455 of yacc.c  */
#line 1690 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); }
    break;

  case 323:

/* Line 1455 of yacc.c  */
#line 1691 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 324:

/* Line 1455 of yacc.c  */
#line 1692 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 325:

/* Line 1455 of yacc.c  */
#line 1693 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 326:

/* Line 1455 of yacc.c  */
#line 1694 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 1697 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 1698 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 329:

/* Line 1455 of yacc.c  */
#line 1701 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_STRING;}
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 1702 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNICODE_STRING;}
    break;

  case 331:

/* Line 1455 of yacc.c  */
#line 1703 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_OSTREAM; }
    break;

  case 332:

/* Line 1455 of yacc.c  */
#line 1704 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_ISTREAM; }
    break;

  case 333:

/* Line 1455 of yacc.c  */
#line 1705 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 1706 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_OBJECT; }
    break;

  case 335:

/* Line 1455 of yacc.c  */
#line 1709 "vtkParse.y"
    { typeSig("void"); (yyval.integer) = VTK_PARSE_VOID;}
    break;

  case 336:

/* Line 1455 of yacc.c  */
#line 1710 "vtkParse.y"
    { typeSig("bool"); (yyval.integer) = VTK_PARSE_BOOL;}
    break;

  case 337:

/* Line 1455 of yacc.c  */
#line 1711 "vtkParse.y"
    { typeSig("ssize_t"); (yyval.integer) = VTK_PARSE_SSIZE_T;}
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 1712 "vtkParse.y"
    { typeSig("size_t"); (yyval.integer) = VTK_PARSE_SIZE_T;}
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 1713 "vtkParse.y"
    { typeSig("vtkTypeInt8"); (yyval.integer) = VTK_PARSE_INT8; }
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 1714 "vtkParse.y"
    { typeSig("vtkTypeUInt8"); (yyval.integer) = VTK_PARSE_UINT8; }
    break;

  case 341:

/* Line 1455 of yacc.c  */
#line 1715 "vtkParse.y"
    { typeSig("vtkTypeInt16"); (yyval.integer) = VTK_PARSE_INT16; }
    break;

  case 342:

/* Line 1455 of yacc.c  */
#line 1716 "vtkParse.y"
    { typeSig("vtkTypeUInt16"); (yyval.integer) = VTK_PARSE_UINT16; }
    break;

  case 343:

/* Line 1455 of yacc.c  */
#line 1717 "vtkParse.y"
    { typeSig("vtkTypeInt32"); (yyval.integer) = VTK_PARSE_INT32; }
    break;

  case 344:

/* Line 1455 of yacc.c  */
#line 1718 "vtkParse.y"
    { typeSig("vtkTypeUInt32"); (yyval.integer) = VTK_PARSE_UINT32; }
    break;

  case 345:

/* Line 1455 of yacc.c  */
#line 1719 "vtkParse.y"
    { typeSig("vtkTypeInt64"); (yyval.integer) = VTK_PARSE_INT64; }
    break;

  case 346:

/* Line 1455 of yacc.c  */
#line 1720 "vtkParse.y"
    { typeSig("vtkTypeUInt64"); (yyval.integer) = VTK_PARSE_UINT64; }
    break;

  case 347:

/* Line 1455 of yacc.c  */
#line 1721 "vtkParse.y"
    { typeSig("vtkTypeFloat32"); (yyval.integer) = VTK_PARSE_FLOAT32; }
    break;

  case 348:

/* Line 1455 of yacc.c  */
#line 1722 "vtkParse.y"
    { typeSig("vtkTypeFloat64"); (yyval.integer) = VTK_PARSE_FLOAT64; }
    break;

  case 349:

/* Line 1455 of yacc.c  */
#line 1723 "vtkParse.y"
    { typeSig("vtkIdType"); (yyval.integer) = VTK_PARSE_ID_TYPE;}
    break;

  case 350:

/* Line 1455 of yacc.c  */
#line 1724 "vtkParse.y"
    { typeSig("float"); (yyval.integer) = VTK_PARSE_FLOAT;}
    break;

  case 351:

/* Line 1455 of yacc.c  */
#line 1725 "vtkParse.y"
    { typeSig("double"); (yyval.integer) = VTK_PARSE_DOUBLE;}
    break;

  case 352:

/* Line 1455 of yacc.c  */
#line 1726 "vtkParse.y"
    { typeSig("long double"); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 353:

/* Line 1455 of yacc.c  */
#line 1727 "vtkParse.y"
    { typeSig("char"); (yyval.integer) = VTK_PARSE_CHAR;}
    break;

  case 354:

/* Line 1455 of yacc.c  */
#line 1728 "vtkParse.y"
    { typeSig("signed char"); (yyval.integer) = VTK_PARSE_SIGNED_CHAR;}
    break;

  case 355:

/* Line 1455 of yacc.c  */
#line 1730 "vtkParse.y"
    { typeSig("unsigned char"); (yyval.integer) = VTK_PARSE_UNSIGNED_CHAR;}
    break;

  case 356:

/* Line 1455 of yacc.c  */
#line 1731 "vtkParse.y"
    { typeSig("int"); (yyval.integer) = VTK_PARSE_INT;}
    break;

  case 357:

/* Line 1455 of yacc.c  */
#line 1733 "vtkParse.y"
    { typeSig("unsigned int"); (yyval.integer) = VTK_PARSE_UNSIGNED_INT;}
    break;

  case 358:

/* Line 1455 of yacc.c  */
#line 1734 "vtkParse.y"
    { typeSig("short"); (yyval.integer) = VTK_PARSE_SHORT;}
    break;

  case 359:

/* Line 1455 of yacc.c  */
#line 1736 "vtkParse.y"
    { typeSig("unsigned short"); (yyval.integer) = VTK_PARSE_UNSIGNED_SHORT;}
    break;

  case 360:

/* Line 1455 of yacc.c  */
#line 1737 "vtkParse.y"
    { typeSig("long"); (yyval.integer) = VTK_PARSE_LONG;}
    break;

  case 361:

/* Line 1455 of yacc.c  */
#line 1739 "vtkParse.y"
    { typeSig("unsigned long"); (yyval.integer) = VTK_PARSE_UNSIGNED_LONG;}
    break;

  case 362:

/* Line 1455 of yacc.c  */
#line 1740 "vtkParse.y"
    { typeSig("long long"); (yyval.integer) = VTK_PARSE_LONG_LONG;}
    break;

  case 363:

/* Line 1455 of yacc.c  */
#line 1742 "vtkParse.y"
    {typeSig("unsigned long long");(yyval.integer)=VTK_PARSE_UNSIGNED_LONG_LONG;}
    break;

  case 364:

/* Line 1455 of yacc.c  */
#line 1743 "vtkParse.y"
    { typeSig("__int64"); (yyval.integer) = VTK_PARSE___INT64;}
    break;

  case 365:

/* Line 1455 of yacc.c  */
#line 1745 "vtkParse.y"
    { typeSig("unsigned __int64"); (yyval.integer) = VTK_PARSE_UNSIGNED___INT64;}
    break;

  case 366:

/* Line 1455 of yacc.c  */
#line 1746 "vtkParse.y"
    { typeSig("int"); (yyval.integer) = VTK_PARSE_INT; }
    break;

  case 367:

/* Line 1455 of yacc.c  */
#line 1747 "vtkParse.y"
    { typeSig("unsigned int"); (yyval.integer) = VTK_PARSE_UNSIGNED_INT; }
    break;

  case 368:

/* Line 1455 of yacc.c  */
#line 1753 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 369:

/* Line 1455 of yacc.c  */
#line 1754 "vtkParse.y"
    { postSig("{ "); }
    break;

  case 370:

/* Line 1455 of yacc.c  */
#line 1755 "vtkParse.y"
    {
          char *cp;
          postSig("}");
          cp = (char *)malloc(strlen((yyvsp[(3) - (6)].str)) + strlen((yyvsp[(4) - (6)].str)) + 5);
          sprintf(cp, "{ %s%s }", (yyvsp[(3) - (6)].str), (yyvsp[(4) - (6)].str));
          (yyval.str) = cp;
        }
    break;

  case 373:

/* Line 1455 of yacc.c  */
#line 1765 "vtkParse.y"
    {(yyval.str) = vtkstrdup("");}
    break;

  case 374:

/* Line 1455 of yacc.c  */
#line 1766 "vtkParse.y"
    { postSig(", "); }
    break;

  case 375:

/* Line 1455 of yacc.c  */
#line 1767 "vtkParse.y"
    {
          char *cp;
          cp = (char *)malloc(strlen((yyvsp[(1) - (4)].str)) + strlen((yyvsp[(4) - (4)].str)) + 3);
          sprintf(cp, "%s, %s", (yyvsp[(1) - (4)].str), (yyvsp[(4) - (4)].str));
          (yyval.str) = cp;
        }
    break;

  case 376:

/* Line 1455 of yacc.c  */
#line 1774 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 377:

/* Line 1455 of yacc.c  */
#line 1775 "vtkParse.y"
    {postSig("+");}
    break;

  case 378:

/* Line 1455 of yacc.c  */
#line 1775 "vtkParse.y"
    {(yyval.str) = (yyvsp[(3) - (3)].str);}
    break;

  case 379:

/* Line 1455 of yacc.c  */
#line 1776 "vtkParse.y"
    {postSig("-");}
    break;

  case 380:

/* Line 1455 of yacc.c  */
#line 1776 "vtkParse.y"
    {
             (yyval.str) = (char *)malloc(strlen((yyvsp[(3) - (3)].str))+2);
             sprintf((yyval.str), "-%s", (yyvsp[(3) - (3)].str)); }
    break;

  case 381:

/* Line 1455 of yacc.c  */
#line 1779 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 382:

/* Line 1455 of yacc.c  */
#line 1780 "vtkParse.y"
    {postSig("(");}
    break;

  case 383:

/* Line 1455 of yacc.c  */
#line 1780 "vtkParse.y"
    {postSig(")"); (yyval.str) = (yyvsp[(3) - (4)].str);}
    break;

  case 384:

/* Line 1455 of yacc.c  */
#line 1781 "vtkParse.y"
    {postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 385:

/* Line 1455 of yacc.c  */
#line 1782 "vtkParse.y"
    {postSig("<(");}
    break;

  case 386:

/* Line 1455 of yacc.c  */
#line 1783 "vtkParse.y"
    {
            postSig(")");
            (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (9)].str)) + strlen(getTypeId()) +
                                     strlen((yyvsp[(8) - (9)].str)) + 5);
            sprintf((yyval.str), "%s<%s>(%s)", (yyvsp[(1) - (9)].str), getTypeId(), (yyvsp[(8) - (9)].str));
            }
    break;

  case 387:

/* Line 1455 of yacc.c  */
#line 1790 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 388:

/* Line 1455 of yacc.c  */
#line 1791 "vtkParse.y"
    {
                size_t i = strlen((yyvsp[(1) - (2)].str));
                char *cp = (char *)malloc(i + strlen((yyvsp[(2) - (2)].str)) + 1);
                strcpy(cp, (yyvsp[(1) - (2)].str));
                strcpy(&cp[i], (yyvsp[(2) - (2)].str));
                (yyval.str) = cp; }
    break;

  case 389:

/* Line 1455 of yacc.c  */
#line 1798 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 390:

/* Line 1455 of yacc.c  */
#line 1799 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 391:

/* Line 1455 of yacc.c  */
#line 1800 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 392:

/* Line 1455 of yacc.c  */
#line 1801 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 393:

/* Line 1455 of yacc.c  */
#line 1802 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 394:

/* Line 1455 of yacc.c  */
#line 1803 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 395:

/* Line 1455 of yacc.c  */
#line 1804 "vtkParse.y"
    {(yyval.str) = vtkstrdup(add_const_scope((yyvsp[(1) - (1)].str)));
                postSig((yyvsp[(1) - (1)].str));}
    break;

  case 396:

/* Line 1455 of yacc.c  */
#line 1806 "vtkParse.y"
    {(yyval.str) = vtkstrdup(add_const_scope((yyvsp[(1) - (1)].str)));
                postSig((yyvsp[(1) - (1)].str));}
    break;

  case 397:

/* Line 1455 of yacc.c  */
#line 1816 "vtkParse.y"
    {preSig("void Set"); postSig("(");}
    break;

  case 398:

/* Line 1455 of yacc.c  */
#line 1817 "vtkParse.y"
    {
   postSig("a);");
   sprintf(temps,"Set%s",(yyvsp[(3) - (7)].str));
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   add_argument(currentFunction, (yyvsp[(6) - (7)].integer), getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 399:

/* Line 1455 of yacc.c  */
#line 1829 "vtkParse.y"
    {postSig("Get");}
    break;

  case 400:

/* Line 1455 of yacc.c  */
#line 1830 "vtkParse.y"
    {markSig();}
    break;

  case 401:

/* Line 1455 of yacc.c  */
#line 1830 "vtkParse.y"
    {swapSig();}
    break;

  case 402:

/* Line 1455 of yacc.c  */
#line 1831 "vtkParse.y"
    {
   postSig("();");
   sprintf(temps,"Get%s",(yyvsp[(4) - (9)].str));
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   set_return(currentFunction, (yyvsp[(7) - (9)].integer), getTypeId(), 0);
   output_function();
   }
    break;

  case 403:

/* Line 1455 of yacc.c  */
#line 1842 "vtkParse.y"
    {preSig("void Set");}
    break;

  case 404:

/* Line 1455 of yacc.c  */
#line 1843 "vtkParse.y"
    {
   postSig("(char *);");
   sprintf(temps,"Set%s",(yyvsp[(4) - (5)].str));
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   add_argument(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 405:

/* Line 1455 of yacc.c  */
#line 1855 "vtkParse.y"
    {preSig("char *Get");}
    break;

  case 406:

/* Line 1455 of yacc.c  */
#line 1856 "vtkParse.y"
    {
   postSig("();");
   sprintf(temps,"Get%s",(yyvsp[(4) - (5)].str));
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   set_return(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   output_function();
   }
    break;

  case 407:

/* Line 1455 of yacc.c  */
#line 1867 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 408:

/* Line 1455 of yacc.c  */
#line 1867 "vtkParse.y"
    {closeSig();}
    break;

  case 409:

/* Line 1455 of yacc.c  */
#line 1869 "vtkParse.y"
    {
   char *local;
   chopSig();
   local = vtkstrdup(copySig());
   sprintf(currentFunction->Signature,"void Set%s(%s);",(yyvsp[(3) - (10)].str),local);
   sprintf(temps,"Set%s",(yyvsp[(3) - (10)].str));
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   add_argument(currentFunction, (yyvsp[(6) - (10)].integer), getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%sGet%sMinValue();",local,(yyvsp[(3) - (10)].str));
   sprintf(temps,"Get%sMinValue",(yyvsp[(3) - (10)].str));
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   set_return(currentFunction, (yyvsp[(6) - (10)].integer), getTypeId(), 0);
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%sGet%sMaxValue();",local,(yyvsp[(3) - (10)].str));
   sprintf(temps,"Get%sMaxValue",(yyvsp[(3) - (10)].str));
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   set_return(currentFunction, (yyvsp[(6) - (10)].integer), getTypeId(), 0);
   output_function();
   free(local);
   }
    break;

  case 410:

/* Line 1455 of yacc.c  */
#line 1910 "vtkParse.y"
    {preSig("void Set"); postSig("("); }
    break;

  case 411:

/* Line 1455 of yacc.c  */
#line 1911 "vtkParse.y"
    {
   postSig("*);");
   sprintf(temps,"Set%s",(yyvsp[(3) - (7)].str));
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   add_argument(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 412:

/* Line 1455 of yacc.c  */
#line 1923 "vtkParse.y"
    {postSig("*Get");}
    break;

  case 413:

/* Line 1455 of yacc.c  */
#line 1924 "vtkParse.y"
    {markSig();}
    break;

  case 414:

/* Line 1455 of yacc.c  */
#line 1924 "vtkParse.y"
    {swapSig();}
    break;

  case 415:

/* Line 1455 of yacc.c  */
#line 1925 "vtkParse.y"
    {
   postSig("();");
   sprintf(temps,"Get%s",(yyvsp[(4) - (9)].str));
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   set_return(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   output_function();
   }
    break;

  case 416:

/* Line 1455 of yacc.c  */
#line 1937 "vtkParse.y"
    {
   sprintf(temps,"%sOn",(yyvsp[(3) - (6)].str));
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   startSig();
   postSig("void ");
   postSig(temps);
   postSig("();");
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();

   sprintf(temps,"%sOff",(yyvsp[(3) - (6)].str));
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   startSig();
   postSig("void ");
   postSig(temps);
   postSig("();");
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }
    break;

  case 417:

/* Line 1455 of yacc.c  */
#line 1964 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 418:

/* Line 1455 of yacc.c  */
#line 1965 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 2);
   }
    break;

  case 419:

/* Line 1455 of yacc.c  */
#line 1969 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 420:

/* Line 1455 of yacc.c  */
#line 1970 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 2);
   }
    break;

  case 421:

/* Line 1455 of yacc.c  */
#line 1974 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 422:

/* Line 1455 of yacc.c  */
#line 1975 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 3);
   }
    break;

  case 423:

/* Line 1455 of yacc.c  */
#line 1979 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 424:

/* Line 1455 of yacc.c  */
#line 1980 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 3);
   }
    break;

  case 425:

/* Line 1455 of yacc.c  */
#line 1984 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 426:

/* Line 1455 of yacc.c  */
#line 1985 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 4);
   }
    break;

  case 427:

/* Line 1455 of yacc.c  */
#line 1989 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 428:

/* Line 1455 of yacc.c  */
#line 1990 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 4);
   }
    break;

  case 429:

/* Line 1455 of yacc.c  */
#line 1994 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 430:

/* Line 1455 of yacc.c  */
#line 1995 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 6);
   }
    break;

  case 431:

/* Line 1455 of yacc.c  */
#line 1999 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 432:

/* Line 1455 of yacc.c  */
#line 2000 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 6);
   }
    break;

  case 433:

/* Line 1455 of yacc.c  */
#line 2004 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 434:

/* Line 1455 of yacc.c  */
#line 2006 "vtkParse.y"
    {
   char *local;
   chopSig();
   local = vtkstrdup(copySig());
   sprintf(currentFunction->Signature,"void Set%s(%s a[%s]);",
           (yyvsp[(3) - (9)].str), local, (yyvsp[(8) - (9)].str));
   sprintf(temps,"Set%s",(yyvsp[(3) - (9)].str));
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   add_argument(currentFunction, (VTK_PARSE_POINTER | (yyvsp[(6) - (9)].integer)),
                getTypeId(), (int)strtol((yyvsp[(8) - (9)].str), NULL, 0));
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   free(local);
   }
    break;

  case 435:

/* Line 1455 of yacc.c  */
#line 2024 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 436:

/* Line 1455 of yacc.c  */
#line 2026 "vtkParse.y"
    {
   char *local;
   chopSig();
   local = vtkstrdup(copySig());
   sprintf(currentFunction->Signature,"%s *Get%s();", local, (yyvsp[(3) - (9)].str));
   sprintf(temps,"Get%s",(yyvsp[(3) - (9)].str));
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   set_return(currentFunction, (VTK_PARSE_POINTER | (yyvsp[(6) - (9)].integer)),
              getTypeId(), (int)strtol((yyvsp[(8) - (9)].str), NULL, 0));
   output_function();
   free(local);
   }
    break;

  case 437:

/* Line 1455 of yacc.c  */
#line 2043 "vtkParse.y"
    {
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate();",
             (yyvsp[(3) - (4)].str));

     sprintf(temps,"Get%sCoordinate",(yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     set_return(currentFunction, VTK_PARSE_OBJECT_PTR, "vtkCoordinate", 0);
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s(double, double);",
             (yyvsp[(3) - (4)].str));
     sprintf(temps,"Set%s",(yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s(double a[2]);",
             (yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     add_argument(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 2);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s();", (yyvsp[(3) - (4)].str));
     sprintf(temps,"Get%s",(yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     set_return(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 2);
     output_function();
   }
    break;

  case 438:

/* Line 1455 of yacc.c  */
#line 2097 "vtkParse.y"
    {
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate();",
             (yyvsp[(3) - (4)].str));

     sprintf(temps,"Get%sCoordinate",(yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     set_return(currentFunction, VTK_PARSE_OBJECT_PTR, "vtkCoordinate", 0);
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,
             "void Set%s(double, double, double);",
             (yyvsp[(3) - (4)].str));
     sprintf(temps,"Set%s",(yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_argument(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s(double a[3]);",
             (yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     add_argument(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 3);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s();", (yyvsp[(3) - (4)].str));
     sprintf(temps,"Get%s",(yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     set_return(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 3);
     output_function();
   }
    break;

  case 439:

/* Line 1455 of yacc.c  */
#line 2153 "vtkParse.y"
    {
   int is_concrete = 0;
   int i;

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "const char *GetClassName();");
   sprintf(temps,"GetClassName");
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   set_return(currentFunction, (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR),
              "char", 0);
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "int IsA(const char *name);");
   sprintf(temps,"IsA");
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   add_argument(currentFunction, (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR),
                "char", 0);
   set_return(currentFunction, VTK_PARSE_INT, "int", 0);
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "%s *NewInstance();", (yyvsp[(3) - (7)].str));
   sprintf(temps,"NewInstance");
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
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
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature, "%s *SafeDownCast(vtkObject* o);",
             (yyvsp[(3) - (7)].str));
     sprintf(temps,"SafeDownCast");
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     add_argument(currentFunction, VTK_PARSE_OBJECT_PTR, "vtkObject", 0);
     set_return(currentFunction, (VTK_PARSE_STATIC | VTK_PARSE_OBJECT_PTR),
                (yyvsp[(3) - (7)].str), 0);
     output_function();
     }
   }
    break;

  case 440:

/* Line 1455 of yacc.c  */
#line 2229 "vtkParse.y"
    { (yyval.str) = "operator()"; }
    break;

  case 441:

/* Line 1455 of yacc.c  */
#line 2230 "vtkParse.y"
    { (yyval.str) = "operator[]"; }
    break;

  case 442:

/* Line 1455 of yacc.c  */
#line 2231 "vtkParse.y"
    { (yyval.str) = "operator new[]"; }
    break;

  case 443:

/* Line 1455 of yacc.c  */
#line 2232 "vtkParse.y"
    { (yyval.str) = "operator delete[]"; }
    break;

  case 445:

/* Line 1455 of yacc.c  */
#line 2235 "vtkParse.y"
    { (yyval.str) = "operator="; }
    break;

  case 446:

/* Line 1455 of yacc.c  */
#line 2236 "vtkParse.y"
    { (yyval.str) = "operator*"; }
    break;

  case 447:

/* Line 1455 of yacc.c  */
#line 2236 "vtkParse.y"
    { (yyval.str) = "operator/"; }
    break;

  case 448:

/* Line 1455 of yacc.c  */
#line 2237 "vtkParse.y"
    { (yyval.str) = "operator-"; }
    break;

  case 449:

/* Line 1455 of yacc.c  */
#line 2237 "vtkParse.y"
    { (yyval.str) = "operator+"; }
    break;

  case 450:

/* Line 1455 of yacc.c  */
#line 2238 "vtkParse.y"
    { (yyval.str) = "operator!"; }
    break;

  case 451:

/* Line 1455 of yacc.c  */
#line 2238 "vtkParse.y"
    { (yyval.str) = "operator~"; }
    break;

  case 452:

/* Line 1455 of yacc.c  */
#line 2239 "vtkParse.y"
    { (yyval.str) = "operator,"; }
    break;

  case 453:

/* Line 1455 of yacc.c  */
#line 2239 "vtkParse.y"
    { (yyval.str) = "operator<"; }
    break;

  case 454:

/* Line 1455 of yacc.c  */
#line 2240 "vtkParse.y"
    { (yyval.str) = "operator>"; }
    break;

  case 455:

/* Line 1455 of yacc.c  */
#line 2240 "vtkParse.y"
    { (yyval.str) = "operator&"; }
    break;

  case 456:

/* Line 1455 of yacc.c  */
#line 2241 "vtkParse.y"
    { (yyval.str) = "operator|"; }
    break;

  case 457:

/* Line 1455 of yacc.c  */
#line 2241 "vtkParse.y"
    { (yyval.str) = "operator^"; }
    break;

  case 458:

/* Line 1455 of yacc.c  */
#line 2242 "vtkParse.y"
    { (yyval.str) = "operator%"; }
    break;

  case 459:

/* Line 1455 of yacc.c  */
#line 2243 "vtkParse.y"
    { (yyval.str) = "operator new"; }
    break;

  case 460:

/* Line 1455 of yacc.c  */
#line 2244 "vtkParse.y"
    { (yyval.str) = "operator delete"; }
    break;

  case 461:

/* Line 1455 of yacc.c  */
#line 2245 "vtkParse.y"
    { (yyval.str) = "operator<<="; }
    break;

  case 462:

/* Line 1455 of yacc.c  */
#line 2246 "vtkParse.y"
    { (yyval.str) = "operator>>="; }
    break;

  case 463:

/* Line 1455 of yacc.c  */
#line 2247 "vtkParse.y"
    { (yyval.str) = "operator<<"; }
    break;

  case 464:

/* Line 1455 of yacc.c  */
#line 2248 "vtkParse.y"
    { (yyval.str) = "operator>>"; }
    break;

  case 465:

/* Line 1455 of yacc.c  */
#line 2249 "vtkParse.y"
    { (yyval.str) = "operator->*"; }
    break;

  case 466:

/* Line 1455 of yacc.c  */
#line 2250 "vtkParse.y"
    { (yyval.str) = "operator->"; }
    break;

  case 467:

/* Line 1455 of yacc.c  */
#line 2251 "vtkParse.y"
    { (yyval.str) = "operator+="; }
    break;

  case 468:

/* Line 1455 of yacc.c  */
#line 2252 "vtkParse.y"
    { (yyval.str) = "operator-="; }
    break;

  case 469:

/* Line 1455 of yacc.c  */
#line 2253 "vtkParse.y"
    { (yyval.str) = "operator*="; }
    break;

  case 470:

/* Line 1455 of yacc.c  */
#line 2254 "vtkParse.y"
    { (yyval.str) = "operator/="; }
    break;

  case 471:

/* Line 1455 of yacc.c  */
#line 2255 "vtkParse.y"
    { (yyval.str) = "operator%="; }
    break;

  case 472:

/* Line 1455 of yacc.c  */
#line 2256 "vtkParse.y"
    { (yyval.str) = "operator++"; }
    break;

  case 473:

/* Line 1455 of yacc.c  */
#line 2257 "vtkParse.y"
    { (yyval.str) = "operator--"; }
    break;

  case 474:

/* Line 1455 of yacc.c  */
#line 2258 "vtkParse.y"
    { (yyval.str) = "operator&="; }
    break;

  case 475:

/* Line 1455 of yacc.c  */
#line 2259 "vtkParse.y"
    { (yyval.str) = "operator|="; }
    break;

  case 476:

/* Line 1455 of yacc.c  */
#line 2260 "vtkParse.y"
    { (yyval.str) = "operator^="; }
    break;

  case 477:

/* Line 1455 of yacc.c  */
#line 2261 "vtkParse.y"
    {(yyval.str) = "operator&&=";}
    break;

  case 478:

/* Line 1455 of yacc.c  */
#line 2262 "vtkParse.y"
    {(yyval.str) = "operator||=";}
    break;

  case 479:

/* Line 1455 of yacc.c  */
#line 2263 "vtkParse.y"
    { (yyval.str) = "operator&&"; }
    break;

  case 480:

/* Line 1455 of yacc.c  */
#line 2264 "vtkParse.y"
    { (yyval.str) = "operator||"; }
    break;

  case 481:

/* Line 1455 of yacc.c  */
#line 2265 "vtkParse.y"
    { (yyval.str) = "operator=="; }
    break;

  case 482:

/* Line 1455 of yacc.c  */
#line 2266 "vtkParse.y"
    { (yyval.str) = "operator!="; }
    break;

  case 483:

/* Line 1455 of yacc.c  */
#line 2267 "vtkParse.y"
    { (yyval.str) = "operator<="; }
    break;

  case 484:

/* Line 1455 of yacc.c  */
#line 2268 "vtkParse.y"
    { (yyval.str) = "operator>="; }
    break;

  case 485:

/* Line 1455 of yacc.c  */
#line 2275 "vtkParse.y"
    {
  static char name[256];
  static char value[256];
  size_t i = 0;
  char *cp = (yyvsp[(1) - (1)].str);
  while ((*cp >= 'a' && *cp <= 'z') ||
         (*cp >= 'A' && *cp <= 'Z') ||
         (*cp >= '0' && *cp <= '9') ||
         *cp == '_') { name[i++] = *cp++; }
  name[i] = '\0';
  while (*cp == ' ' || *cp == '\t') { cp++; }
  strcpy(value, cp);
  i = strlen(value);
  while (i > 0 && (value[i-1] == '\n' || value[i-1] == '\r' ||
                   value[i-1] == '\t' || value[i-1] == ' ')) { i--; }
  value[i] = '\0';
  add_constant(name, value, 0, NULL, 1);
  }
    break;



/* Line 1455 of yacc.c  */
#line 7355 "vtkParse.tab.c"
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
#line 2315 "vtkParse.y"

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
      free(class_info->SuperClasses);
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

  access_level = VTK_ACCESS_PRIVATE;
  if (is_struct)
    {
    access_level = VTK_ACCESS_PUBLIC;
    }

  vtkParse_InitFunction(currentFunction);
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
    currentEnumName = text;
    strcpy(currentEnumName, name);
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
  static char text[256];
  int i;
  long j;

  if (value)
    {
    currentEnumValue = text;
    strcpy(currentEnumValue, value);
    }
  else if (currentEnumValue)
    {
    i = strlen(currentEnumValue);
    while (i > 0 && currentEnumValue[i-1] >= '0' &&
           currentEnumValue[i-1] <= '9') { i--; }

    if (i == 0 || currentEnumValue[i-1] == ' ' ||
        (i > 1 && currentEnumValue[i-2] == ' ' &&
         (currentEnumValue[i-1] == '-' || currentEnumValue[i-1] == '+')))
      {
      if (i > 0 && currentEnumValue[i-1] != ' ')
        {
        i--;
        }
      j = (int)strtol(&currentEnumValue[i], NULL, 10);
      sprintf(&currentEnumValue[i], "%li", j+1);
      }
    else
      {
      i = strlen(currentEnumValue);
      strcpy(&currentEnumValue[i], " + 1");
      }
    }
  else
    {
    currentEnumValue = text;
    strcpy(currentEnumValue, "0");
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
    vtkParse_AddItemMacro2(arg, Dimensions, vtkstrdup(text));
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
    vtkParse_AddItemMacro2(val, Dimensions, vtkstrdup(text));
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
  char *cp;

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
  func->Signature = vtkstrdup(funcSig);
  j = strlen(func->Signature);
  while (j > 0 && func->Signature[j-1] == ' ')
    {
    func->Signature[j-1] = '\0';
    }

  arg->Function = func;
}


/* reject the function, do not output it */
void reject_function()
{
  vtkParse_InitFunction(currentFunction);
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
}

void outputSetVectorMacro(const char *var, unsigned int argType,
                          const char *typeText, int n)
{
  char *local = vtkstrdup(typeText);
  char *name;
  int i;

  sprintf(temps,"Set%s", var);
  name = vtkstrdup(temps);

  sprintf(currentFunction->Signature, "void Set%s(%s", var, local);
  for (i = 1; i < n; i++)
    {
    postSig(", ");
    postSig(local);
    }
  postSig(");");
  currentFunction->Name = name;
  for (i = 0; i < n; i++)
    {
    add_argument(currentFunction, argType, getTypeId(), 0);
    }
  set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
  output_function();

  currentFunction->Signature = (char *)malloc(2048);
  sigAllocatedLength = 2048;
  sprintf(currentFunction->Signature, "void Set%s(%s a[%i]);",
          var, local, n);
  currentFunction->Name = name;
  add_argument(currentFunction, (VTK_PARSE_POINTER | argType),
               getTypeId(), n);
  set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
  output_function();

  free(local);
}

void outputGetVectorMacro(const char *var, unsigned int argType,
                          const char *typeText, int n)
{
  char *local = vtkstrdup(typeText);

  sprintf(currentFunction->Signature, "%s *Get%s();", local, var);
  sprintf(temps, "Get%s", var);
  currentFunction->Name = vtkstrdup(temps);
  set_return(currentFunction, (VTK_PARSE_POINTER | argType), getTypeId(), n);
  output_function();

  free(local);
}

/* Parse a header file and return a FileInfo struct */
void vtkParse_AddPointerToArray(
  void *valueArray, int *count, void *value)
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

  values[n++] = value;
  *count = n;
  *(void ***)valueArray = values;
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

  CommentState = 0;

  namespaceDepth = 0;
  currentNamespace = (NamespaceInfo *)malloc(sizeof(NamespaceInfo));
  vtkParse_InitNamespace(currentNamespace);
  data.Contents = currentNamespace;

  templateDepth = 0;
  currentTemplate = NULL;

  currentFunction = (FunctionInfo *)malloc(sizeof(FunctionInfo));
  vtkParse_InitFunction(currentFunction);

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
                  vtkParse_AddItemMacro2(func_info->ReturnValue,
                                         Dimensions, vtkstrdup(text));
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
     vtkParse_AddPointerToArray(&ConcreteClasses,
                                &NumberOfConcreteClasses,
                                vtkstrdup(classname));
     }
}
