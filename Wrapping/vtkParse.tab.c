
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
     INT = 272,
     FLOAT = 273,
     SHORT = 274,
     LONG = 275,
     LONG_LONG = 276,
     INT64__ = 277,
     DOUBLE = 278,
     VOID = 279,
     CHAR = 280,
     SIGNED_CHAR = 281,
     BOOL = 282,
     SSIZE_T = 283,
     SIZE_T = 284,
     OSTREAM = 285,
     ISTREAM = 286,
     ENUM = 287,
     UNION = 288,
     CLASS_REF = 289,
     OTHER = 290,
     CONST = 291,
     CONST_PTR = 292,
     CONST_EQUAL = 293,
     OPERATOR = 294,
     UNSIGNED = 295,
     SIGNED = 296,
     FRIEND = 297,
     INLINE = 298,
     MUTABLE = 299,
     TEMPLATE = 300,
     TYPENAME = 301,
     TYPEDEF = 302,
     NAMESPACE = 303,
     USING = 304,
     VTK_ID = 305,
     STATIC = 306,
     EXTERN = 307,
     VAR_FUNCTION = 308,
     VTK_LEGACY = 309,
     NEW = 310,
     DELETE = 311,
     OP_LSHIFT_EQ = 312,
     OP_RSHIFT_EQ = 313,
     OP_LSHIFT = 314,
     OP_RSHIFT = 315,
     OP_ARROW_POINTER = 316,
     OP_ARROW = 317,
     OP_INCR = 318,
     OP_DECR = 319,
     OP_PLUS_EQ = 320,
     OP_MINUS_EQ = 321,
     OP_TIMES_EQ = 322,
     OP_DIVIDE_EQ = 323,
     OP_REMAINDER_EQ = 324,
     OP_AND_EQ = 325,
     OP_OR_EQ = 326,
     OP_XOR_EQ = 327,
     OP_LOGIC_AND_EQ = 328,
     OP_LOGIC_OR_EQ = 329,
     OP_LOGIC_AND = 330,
     OP_LOGIC_OR = 331,
     OP_LOGIC_EQ = 332,
     OP_LOGIC_NEQ = 333,
     OP_LOGIC_LEQ = 334,
     OP_LOGIC_GEQ = 335,
     ELLIPSIS = 336,
     DOUBLE_COLON = 337,
     LP = 338,
     LA = 339,
     StdString = 340,
     UnicodeString = 341,
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
     VTK_CONSTANT_DEF = 374,
     VTK_BYTE_SWAP_DECL = 375
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
#line 1148 "vtkParse.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 1160 "vtkParse.tab.c"

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
#define YYLAST   5186

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  144
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  192
/* YYNRULES -- Number of rules.  */
#define YYNRULES  538
/* YYNRULES -- Number of states.  */
#define YYNSTATES  945

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
       2,     2,     2,   142,     2,     2,     2,   136,   137,     2,
     122,   123,   134,   132,   129,   131,   143,   135,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   128,   121,
     126,   130,   127,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   140,     2,   141,   139,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   124,   138,   125,   133,     2,     2,     2,
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
    1003,  1005,  1007,  1008,  1012,  1013,  1017,  1019,  1021,  1023,
    1025,  1027,  1029,  1031,  1033,  1035,  1036,  1043,  1044,  1046,
    1047,  1048,  1053,  1055,  1056,  1060,  1061,  1065,  1067,  1068,
    1073,  1074,  1075,  1085,  1087,  1090,  1092,  1094,  1096,  1098,
    1100,  1102,  1104,  1106,  1107,  1115,  1116,  1117,  1118,  1128,
    1129,  1135,  1136,  1142,  1143,  1144,  1155,  1156,  1164,  1165,
    1166,  1167,  1177,  1184,  1185,  1193,  1194,  1202,  1203,  1211,
    1212,  1220,  1221,  1229,  1230,  1238,  1239,  1247,  1248,  1256,
    1257,  1267,  1268,  1278,  1283,  1288,  1296,  1299,  1302,  1306,
    1310,  1312,  1314,  1316,  1318,  1320,  1322,  1324,  1326,  1328,
    1330,  1332,  1334,  1336,  1338,  1340,  1342,  1344,  1346,  1348,
    1350,  1352,  1354,  1356,  1358,  1360,  1362,  1364,  1366,  1368,
    1370,  1372,  1374,  1376,  1378,  1380,  1382,  1384,  1386,  1388,
    1390,  1392,  1394,  1395,  1398,  1399,  1402,  1404,  1406,  1408,
    1410,  1412,  1414,  1416,  1418,  1420,  1422,  1424,  1426,  1428,
    1430,  1432,  1434,  1436,  1438,  1440,  1442,  1444,  1446,  1448,
    1450,  1452,  1454,  1456,  1458,  1460,  1462,  1464,  1466,  1468,
    1470,  1472,  1474,  1476,  1478,  1480,  1482,  1484,  1486,  1488,
    1490,  1492,  1494,  1496,  1498,  1502,  1506,  1510,  1514
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     145,     0,    -1,    -1,    -1,   145,   146,   147,    -1,   237,
      -1,   163,   239,   121,    -1,   176,   239,   121,    -1,   177,
      -1,   149,    -1,   148,    -1,   181,    -1,   151,   239,   121,
      -1,   183,   151,   239,   121,    -1,    34,    -1,   199,   211,
      -1,   183,   199,   211,    -1,   198,   211,    -1,   194,   211,
      -1,   195,   211,    -1,   183,   194,   211,    -1,   192,   211,
      -1,   303,    -1,   328,    -1,   279,   121,    -1,     9,   122,
     329,   123,    -1,    50,   122,   329,   123,    -1,   121,    -1,
      52,    10,   124,   145,   125,    -1,    -1,    48,   279,   150,
     124,   145,   125,    -1,    48,   124,   329,   125,    -1,    -1,
       4,   264,   152,   159,   124,   156,   125,    -1,    -1,     4,
     264,   126,   275,   127,   153,   159,   124,   156,   125,    -1,
      -1,     3,   264,   154,   159,   124,   156,   125,    -1,    -1,
       3,   264,   126,   275,   127,   155,   159,   124,   156,   125,
      -1,     3,   124,   329,   125,    -1,    -1,    -1,   156,   157,
     158,    -1,   162,   128,    -1,   237,    -1,   163,   239,   121,
      -1,   176,   239,   121,    -1,   177,    -1,   181,    -1,   179,
      -1,    42,   179,    -1,   178,    -1,    34,    -1,   199,   211,
      -1,    42,   199,   211,    -1,   183,   199,   211,    -1,   197,
     211,    -1,    42,   197,   211,    -1,   183,   197,   211,    -1,
     193,   211,    -1,   120,   122,   329,   123,   121,    -1,   303,
      -1,   328,    -1,   121,    -1,    -1,   128,   160,    -1,   161,
      -1,   161,   129,   160,    -1,   277,    -1,     6,   277,    -1,
       7,   277,    -1,     5,   277,    -1,     5,    -1,     6,    -1,
       7,    -1,    -1,    32,   264,   164,   124,   166,   125,    -1,
      -1,    32,   165,   124,   166,   125,    -1,    -1,   167,    -1,
     167,   129,   166,    -1,   264,    -1,   264,   130,   170,    -1,
     169,    -1,   264,    -1,   278,    -1,   272,    -1,    16,    -1,
      11,    -1,    13,    -1,    12,    -1,    15,    -1,   168,    -1,
      -1,   174,   171,   170,    -1,    -1,   168,   175,   172,   170,
      -1,    -1,   122,   173,   170,   123,    -1,   131,    -1,   132,
      -1,   133,    -1,   131,    -1,   132,    -1,   134,    -1,   135,
      -1,   136,    -1,   137,    -1,   138,    -1,   139,    -1,    33,
     264,   124,   329,   125,    -1,    33,   124,   329,   125,    -1,
      49,   330,   121,    -1,   183,   179,    -1,     4,   264,   180,
      -1,     3,   264,   180,    -1,     3,   180,    -1,   121,    -1,
     124,   329,   125,   330,   121,    -1,   128,   330,   121,    -1,
     182,   267,   247,   121,    -1,   182,   151,   233,   121,    -1,
     182,   163,   233,   121,    -1,   182,   176,   233,   121,    -1,
     182,    53,   121,    -1,    47,    -1,    45,   126,   127,    -1,
      -1,    45,   126,   184,   185,   127,    -1,   187,    -1,    -1,
     187,   129,   186,   185,    -1,   284,   190,    -1,   189,   190,
      -1,    -1,   188,   183,   190,    -1,     4,    -1,    46,    -1,
      -1,    -1,   264,   191,   234,    -1,    54,   122,   194,   123,
      -1,    54,   122,   197,   123,    -1,   265,   208,    -1,   265,
     196,   208,    -1,   279,    82,   133,   223,    -1,    43,   279,
      82,   133,   223,    -1,   279,    82,   216,    -1,    43,   279,
      82,   216,    -1,   279,    82,   279,    82,   133,   223,    -1,
      43,   279,    82,   279,    82,   133,   223,    -1,   279,    82,
     279,    82,   216,    -1,    43,   279,    82,   279,    82,   216,
      -1,   279,    82,    -1,   196,   279,    82,    -1,   133,   223,
      -1,    43,   133,   223,    -1,     8,   133,   223,    -1,   216,
      -1,    43,   216,    -1,   265,   208,    -1,     8,   265,   208,
      -1,   279,    82,   200,    -1,   265,   196,   203,    -1,   200,
      -1,   265,   203,    -1,     8,   267,   203,    -1,    -1,    -1,
      39,   265,   122,   201,   226,   123,   202,   210,    -1,    -1,
     205,   204,   210,    -1,    -1,    -1,    39,   326,   206,   122,
     207,   226,   123,    -1,    -1,   212,   209,   210,    -1,    -1,
     130,    16,    -1,    38,    16,    -1,    36,    -1,   121,    -1,
     124,   329,   125,    -1,    -1,   264,   122,   213,   226,   123,
      -1,    -1,    -1,   264,   126,   214,   275,   127,   122,   215,
     226,   123,    -1,    -1,   218,   217,   220,    -1,    -1,   264,
     122,   219,   226,   123,    -1,    -1,   128,   222,   221,    -1,
      -1,   129,   222,   221,    -1,   264,   122,   329,   123,    -1,
     224,    -1,    -1,   264,   122,   225,   226,   123,    -1,    -1,
      -1,   227,   228,    -1,    81,    -1,   230,    -1,    -1,   230,
     129,   229,   228,    -1,    -1,    -1,   231,   267,   245,   232,
     234,    -1,    53,    -1,   264,    -1,   280,   264,    -1,    -1,
     235,    -1,    -1,   130,   236,   290,    -1,   265,   238,   240,
     121,    -1,    51,    53,   240,   121,    -1,    53,   240,   121,
      -1,   247,   234,    -1,    -1,   242,   240,    -1,    -1,    -1,
     240,   129,   241,   242,    -1,    -1,   243,   238,    -1,    -1,
     280,   244,   238,    -1,   255,   257,    -1,    -1,   249,   253,
     123,   246,   251,    -1,   256,   257,    -1,    -1,   250,   254,
     123,   248,   251,    -1,   122,    -1,    83,    -1,    84,    -1,
      83,    -1,    84,    -1,    -1,    -1,   122,   252,   226,   123,
      -1,   258,    -1,   245,    -1,   280,   245,    -1,   247,    -1,
     280,   247,    -1,    -1,   256,    -1,   264,    -1,    -1,   258,
      -1,    -1,   259,   260,    -1,    -1,   262,   140,   261,   263,
     141,    -1,    -1,   260,    -1,    -1,   170,    -1,    50,    -1,
       9,    -1,    31,    -1,    30,    -1,    85,    -1,    86,    -1,
     267,    -1,    44,   267,    -1,    52,   267,    -1,    52,    10,
     267,    -1,    43,   267,    -1,   266,   267,    -1,    43,   266,
     267,    -1,    51,    -1,    51,    43,    -1,   268,    -1,   268,
     280,    -1,   270,    -1,   269,   270,    -1,   270,   269,    -1,
      36,    -1,   283,    -1,   272,    -1,   278,    -1,    -1,    46,
     271,   277,    -1,    -1,    50,   126,   273,   275,   127,    -1,
      -1,     9,   126,   274,   275,   127,    -1,   267,    -1,    -1,
     267,   129,   276,   275,    -1,    50,    -1,     9,    -1,    31,
      -1,    30,    -1,    85,    -1,    86,    -1,   272,    -1,   278,
      -1,   279,    82,   277,    -1,   272,    82,   277,    -1,     9,
      -1,    50,    -1,    31,    -1,    30,    -1,    85,    -1,    86,
      -1,   137,    -1,   281,   137,    -1,   281,    -1,   282,    -1,
     281,   282,    -1,   134,    -1,    37,    -1,   284,    -1,     4,
     285,    -1,     3,   285,    -1,    33,     9,    -1,    33,    50,
      -1,    32,     9,    -1,    32,    50,    -1,   286,    -1,   285,
      -1,    85,    -1,    86,    -1,    30,    -1,    31,    -1,     9,
      -1,    50,    -1,    24,    -1,    18,    -1,    23,    -1,    27,
      -1,    28,    -1,    29,    -1,    26,    -1,    88,    -1,    89,
      -1,    90,    -1,    91,    -1,    92,    -1,    93,    -1,    94,
      -1,    95,    -1,    96,    -1,    97,    -1,    -1,    41,   287,
     289,    -1,    -1,    40,   288,   289,    -1,   289,    -1,    25,
      -1,    17,    -1,    19,    -1,    20,    -1,    87,    -1,    21,
      -1,    22,    -1,   295,    -1,    -1,   124,   291,   290,   293,
     292,   125,    -1,    -1,   129,    -1,    -1,    -1,   293,   129,
     294,   290,    -1,   302,    -1,    -1,   132,   296,   302,    -1,
      -1,   131,   297,   302,    -1,   301,    -1,    -1,   122,   298,
     295,   123,    -1,    -1,    -1,     9,   126,   299,   268,   127,
     122,   300,   302,   123,    -1,    10,    -1,   301,    10,    -1,
      16,    -1,    11,    -1,    13,    -1,    12,    -1,    14,    -1,
      15,    -1,     9,    -1,    50,    -1,    -1,    98,   122,   264,
     129,   304,   267,   123,    -1,    -1,    -1,    -1,    99,   122,
     305,   264,   129,   306,   267,   307,   123,    -1,    -1,   100,
     122,   308,   264,   123,    -1,    -1,   101,   122,   309,   264,
     123,    -1,    -1,    -1,   102,   122,   264,   129,   310,   283,
     311,   129,   330,   123,    -1,    -1,   103,   122,   264,   129,
     312,   283,   123,    -1,    -1,    -1,    -1,   104,   122,   313,
     264,   129,   314,   283,   315,   123,    -1,   105,   122,   264,
     129,   283,   123,    -1,    -1,   106,   122,   264,   129,   316,
     283,   123,    -1,    -1,   110,   122,   264,   129,   317,   283,
     123,    -1,    -1,   107,   122,   264,   129,   318,   283,   123,
      -1,    -1,   111,   122,   264,   129,   319,   283,   123,    -1,
      -1,   108,   122,   264,   129,   320,   283,   123,    -1,    -1,
     112,   122,   264,   129,   321,   283,   123,    -1,    -1,   109,
     122,   264,   129,   322,   283,   123,    -1,    -1,   113,   122,
     264,   129,   323,   283,   123,    -1,    -1,   114,   122,   264,
     129,   324,   283,   129,    11,   123,    -1,    -1,   115,   122,
     264,   129,   325,   283,   129,    11,   123,    -1,   116,   122,
     264,   123,    -1,   117,   122,   264,   123,    -1,   118,   122,
     264,   129,   264,   292,   123,    -1,   122,   123,    -1,   140,
     141,    -1,    55,   140,   141,    -1,    56,   140,   141,    -1,
     327,    -1,   130,    -1,   134,    -1,   135,    -1,   131,    -1,
     132,    -1,   142,    -1,   133,    -1,   129,    -1,   126,    -1,
     127,    -1,   137,    -1,   138,    -1,   139,    -1,   136,    -1,
      55,    -1,    56,    -1,    57,    -1,    58,    -1,    59,    -1,
      60,    -1,    61,    -1,    62,    -1,    65,    -1,    66,    -1,
      67,    -1,    68,    -1,    69,    -1,    63,    -1,    64,    -1,
      70,    -1,    71,    -1,    72,    -1,    73,    -1,    74,    -1,
      75,    -1,    76,    -1,    77,    -1,    78,    -1,    79,    -1,
      80,    -1,   119,    -1,    -1,   329,   331,    -1,    -1,   330,
     332,    -1,   121,    -1,   332,    -1,    35,    -1,   333,    -1,
     335,    -1,   334,    -1,    47,    -1,   327,    -1,   128,    -1,
     143,    -1,    82,    -1,     4,    -1,    45,    -1,    31,    -1,
      30,    -1,    85,    -1,    86,    -1,   286,    -1,    13,    -1,
      11,    -1,    12,    -1,    14,    -1,    15,    -1,    10,    -1,
      34,    -1,    36,    -1,    37,    -1,    38,    -1,     3,    -1,
      39,    -1,    51,    -1,    43,    -1,     8,    -1,    32,    -1,
      33,    -1,    46,    -1,    16,    -1,    53,    -1,    81,    -1,
       5,    -1,     7,    -1,     6,    -1,    48,    -1,    49,    -1,
      52,    -1,     9,    -1,    50,    -1,   328,    -1,   124,   329,
     125,    -1,   140,   329,   141,    -1,   122,   329,   123,    -1,
      83,   329,   123,    -1,    84,   329,   123,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   949,   949,   950,   949,   954,   955,   956,   957,   958,
     959,   960,   961,   962,   963,   964,   965,   966,   967,   968,
     969,   970,   971,   972,   973,   974,   975,   976,   982,   988,
     988,   990,   996,   996,   998,   998,  1000,  1000,  1002,  1002,
    1004,  1007,  1008,  1007,  1011,  1012,  1013,  1014,  1015,  1016,
    1017,  1018,  1019,  1020,  1021,  1022,  1024,  1025,  1026,  1028,
    1029,  1030,  1031,  1032,  1033,  1035,  1035,  1037,  1037,  1039,
    1040,  1041,  1042,  1049,  1050,  1051,  1061,  1061,  1063,  1063,
    1066,  1066,  1066,  1068,  1069,  1071,  1072,  1072,  1072,  1074,
    1074,  1074,  1074,  1074,  1076,  1077,  1077,  1082,  1082,  1088,
    1088,  1095,  1095,  1096,  1098,  1098,  1099,  1099,  1100,  1100,
    1101,  1101,  1107,  1108,  1110,  1112,  1114,  1115,  1116,  1118,
    1119,  1120,  1126,  1149,  1150,  1151,  1152,  1154,  1160,  1161,
    1161,  1165,  1166,  1166,  1169,  1179,  1187,  1187,  1199,  1200,
    1202,  1202,  1202,  1209,  1211,  1217,  1219,  1220,  1221,  1222,
    1223,  1224,  1225,  1226,  1227,  1229,  1230,  1232,  1233,  1234,
    1239,  1240,  1241,  1242,  1250,  1251,  1254,  1255,  1256,  1266,
    1270,  1265,  1285,  1285,  1297,  1298,  1297,  1305,  1305,  1317,
    1318,  1327,  1337,  1343,  1343,  1346,  1345,  1350,  1351,  1350,
    1360,  1360,  1370,  1370,  1372,  1372,  1374,  1374,  1376,  1378,
    1392,  1392,  1398,  1398,  1398,  1401,  1402,  1403,  1403,  1406,
    1408,  1406,  1437,  1461,  1461,  1463,  1463,  1465,  1465,  1472,
    1473,  1474,  1476,  1487,  1488,  1490,  1491,  1491,  1494,  1494,
    1495,  1495,  1499,  1500,  1500,  1511,  1512,  1512,  1522,  1523,
    1525,  1528,  1530,  1533,  1534,  1534,  1536,  1539,  1540,  1544,
    1545,  1548,  1548,  1550,  1552,  1552,  1554,  1554,  1556,  1556,
    1558,  1558,  1560,  1561,  1567,  1568,  1569,  1570,  1571,  1572,
    1579,  1580,  1581,  1582,  1584,  1585,  1587,  1591,  1592,  1594,
    1595,  1597,  1598,  1599,  1601,  1603,  1604,  1606,  1608,  1608,
    1612,  1612,  1614,  1614,  1617,  1617,  1617,  1619,  1620,  1621,
    1622,  1623,  1624,  1625,  1626,  1628,  1634,  1641,  1641,  1641,
    1641,  1641,  1641,  1657,  1658,  1659,  1664,  1665,  1677,  1678,
    1681,  1682,  1683,  1684,  1685,  1686,  1687,  1690,  1691,  1694,
    1695,  1696,  1697,  1698,  1699,  1702,  1703,  1704,  1705,  1706,
    1707,  1708,  1709,  1710,  1711,  1712,  1713,  1714,  1715,  1716,
    1717,  1718,  1719,  1719,  1720,  1720,  1722,  1725,  1726,  1727,
    1728,  1729,  1730,  1731,  1737,  1738,  1738,  1747,  1747,  1749,
    1750,  1750,  1758,  1759,  1759,  1760,  1760,  1763,  1764,  1764,
    1765,  1766,  1765,  1774,  1775,  1782,  1783,  1784,  1785,  1786,
    1787,  1788,  1790,  1800,  1800,  1813,  1814,  1814,  1813,  1826,
    1826,  1839,  1839,  1851,  1851,  1851,  1894,  1893,  1907,  1908,
    1908,  1907,  1920,  1948,  1948,  1953,  1953,  1958,  1958,  1963,
    1963,  1968,  1968,  1973,  1973,  1978,  1978,  1983,  1983,  1988,
    1988,  2008,  2008,  2026,  2080,  2136,  2213,  2214,  2215,  2216,
    2217,  2219,  2220,  2220,  2221,  2221,  2222,  2222,  2223,  2223,
    2224,  2224,  2225,  2225,  2226,  2227,  2228,  2229,  2230,  2231,
    2232,  2233,  2234,  2235,  2236,  2237,  2238,  2239,  2240,  2241,
    2242,  2243,  2244,  2245,  2246,  2247,  2248,  2249,  2250,  2251,
    2252,  2258,  2281,  2281,  2282,  2282,  2284,  2284,  2286,  2286,
    2286,  2286,  2286,  2287,  2287,  2287,  2287,  2287,  2287,  2288,
    2288,  2288,  2288,  2288,  2289,  2289,  2289,  2289,  2289,  2290,
    2290,  2290,  2290,  2290,  2290,  2291,  2291,  2291,  2291,  2291,
    2291,  2291,  2292,  2292,  2292,  2292,  2292,  2292,  2293,  2293,
    2293,  2293,  2293,  2293,  2295,  2296,  2297,  2297,  2297
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
  "INT", "FLOAT", "SHORT", "LONG", "LONG_LONG", "INT64__", "DOUBLE",
  "VOID", "CHAR", "SIGNED_CHAR", "BOOL", "SSIZE_T", "SIZE_T", "OSTREAM",
  "ISTREAM", "ENUM", "UNION", "CLASS_REF", "OTHER", "CONST", "CONST_PTR",
  "CONST_EQUAL", "OPERATOR", "UNSIGNED", "SIGNED", "FRIEND", "INLINE",
  "MUTABLE", "TEMPLATE", "TYPENAME", "TYPEDEF", "NAMESPACE", "USING",
  "VTK_ID", "STATIC", "EXTERN", "VAR_FUNCTION", "VTK_LEGACY", "NEW",
  "DELETE", "OP_LSHIFT_EQ", "OP_RSHIFT_EQ", "OP_LSHIFT", "OP_RSHIFT",
  "OP_ARROW_POINTER", "OP_ARROW", "OP_INCR", "OP_DECR", "OP_PLUS_EQ",
  "OP_MINUS_EQ", "OP_TIMES_EQ", "OP_DIVIDE_EQ", "OP_REMAINDER_EQ",
  "OP_AND_EQ", "OP_OR_EQ", "OP_XOR_EQ", "OP_LOGIC_AND_EQ",
  "OP_LOGIC_OR_EQ", "OP_LOGIC_AND", "OP_LOGIC_OR", "OP_LOGIC_EQ",
  "OP_LOGIC_NEQ", "OP_LOGIC_LEQ", "OP_LOGIC_GEQ", "ELLIPSIS",
  "DOUBLE_COLON", "LP", "LA", "StdString", "UnicodeString", "IdType",
  "TypeInt8", "TypeUInt8", "TypeInt16", "TypeUInt16", "TypeInt32",
  "TypeUInt32", "TypeInt64", "TypeUInt64", "TypeFloat32", "TypeFloat64",
  "SetMacro", "GetMacro", "SetStringMacro", "GetStringMacro",
  "SetClampMacro", "SetObjectMacro", "GetObjectMacro", "BooleanMacro",
  "SetVector2Macro", "SetVector3Macro", "SetVector4Macro",
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
  "type_id", "type_primitive", "$@46", "$@47", "type_integer", "value",
  "$@48", "maybe_comma", "more_values", "$@49", "literal", "$@50", "$@51",
  "$@52", "$@53", "$@54", "string_literal", "literal2", "macro", "$@55",
  "$@56", "$@57", "$@58", "$@59", "$@60", "$@61", "$@62", "$@63", "$@64",
  "$@65", "$@66", "$@67", "$@68", "$@69", "$@70", "$@71", "$@72", "$@73",
  "$@74", "$@75", "$@76", "op_token", "op_token_no_delim",
  "vtk_constant_def", "maybe_other", "maybe_other_no_semi", "other_stuff",
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
     375,    59,    40,    41,   123,   125,    60,    62,    58,    44,
      61,    45,    43,   126,    42,    47,    37,    38,   124,    94,
      91,    93,    33,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   144,   145,   146,   145,   147,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   147,   147,   148,   150,
     149,   149,   152,   151,   153,   151,   154,   151,   155,   151,
     151,   156,   157,   156,   158,   158,   158,   158,   158,   158,
     158,   158,   158,   158,   158,   158,   158,   158,   158,   158,
     158,   158,   158,   158,   158,   159,   159,   160,   160,   161,
     161,   161,   161,   162,   162,   162,   164,   163,   165,   163,
     166,   166,   166,   167,   167,   168,   168,   168,   168,   169,
     169,   169,   169,   169,   170,   171,   170,   172,   170,   173,
     170,   174,   174,   174,   175,   175,   175,   175,   175,   175,
     175,   175,   176,   176,   177,   178,   179,   179,   179,   180,
     180,   180,   181,   181,   181,   181,   181,   182,   183,   184,
     183,   185,   186,   185,   187,   187,   188,   187,   189,   189,
     190,   191,   190,   192,   193,   194,   195,   195,   195,   195,
     195,   195,   195,   195,   195,   196,   196,   197,   197,   197,
     197,   197,   197,   197,   198,   198,   199,   199,   199,   201,
     202,   200,   204,   203,   206,   207,   205,   209,   208,   210,
     210,   210,   210,   211,   211,   213,   212,   214,   215,   212,
     217,   216,   219,   218,   220,   220,   221,   221,   222,   223,
     225,   224,   226,   227,   226,   228,   228,   229,   228,   231,
     232,   230,   230,   233,   233,   234,   234,   236,   235,   237,
     237,   237,   238,   239,   239,   240,   241,   240,   243,   242,
     244,   242,   245,   246,   245,   247,   248,   247,   249,   249,
     249,   250,   250,   251,   252,   251,   251,   253,   253,   254,
     254,   255,   255,   256,   257,   257,   259,   258,   261,   260,
     262,   262,   263,   263,   264,   264,   264,   264,   264,   264,
     265,   265,   265,   265,   265,   265,   265,   266,   266,   267,
     267,   268,   268,   268,   269,   270,   270,   270,   271,   270,
     273,   272,   274,   272,   275,   276,   275,   277,   277,   277,
     277,   277,   277,   277,   277,   278,   278,   279,   279,   279,
     279,   279,   279,   280,   280,   280,   281,   281,   282,   282,
     283,   283,   283,   283,   283,   283,   283,   284,   284,   285,
     285,   285,   285,   285,   285,   286,   286,   286,   286,   286,
     286,   286,   286,   286,   286,   286,   286,   286,   286,   286,
     286,   286,   287,   286,   288,   286,   286,   289,   289,   289,
     289,   289,   289,   289,   290,   291,   290,   292,   292,   293,
     294,   293,   295,   296,   295,   297,   295,   295,   298,   295,
     299,   300,   295,   301,   301,   302,   302,   302,   302,   302,
     302,   302,   302,   304,   303,   305,   306,   307,   303,   308,
     303,   309,   303,   310,   311,   303,   312,   303,   313,   314,
     315,   303,   303,   316,   303,   317,   303,   318,   303,   319,
     303,   320,   303,   321,   303,   322,   303,   323,   303,   324,
     303,   325,   303,   303,   303,   303,   326,   326,   326,   326,
     326,   327,   327,   327,   327,   327,   327,   327,   327,   327,
     327,   327,   327,   327,   327,   327,   327,   327,   327,   327,
     327,   327,   327,   327,   327,   327,   327,   327,   327,   327,
     327,   327,   327,   327,   327,   327,   327,   327,   327,   327,
     327,   328,   329,   329,   330,   330,   331,   331,   332,   332,
     332,   332,   332,   332,   332,   332,   332,   332,   332,   332,
     332,   332,   332,   332,   332,   332,   332,   332,   332,   332,
     332,   332,   332,   332,   332,   332,   332,   332,   332,   332,
     332,   332,   332,   332,   332,   332,   332,   332,   332,   332,
     332,   332,   332,   332,   333,   334,   335,   335,   335
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
       1,     1,     0,     3,     0,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     0,     6,     0,     1,     0,
       0,     4,     1,     0,     3,     0,     3,     1,     0,     4,
       0,     0,     9,     1,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     0,     7,     0,     0,     0,     9,     0,
       5,     0,     5,     0,     0,    10,     0,     7,     0,     0,
       0,     9,     6,     0,     7,     0,     7,     0,     7,     0,
       7,     0,     7,     0,     7,     0,     7,     0,     7,     0,
       9,     0,     9,     4,     4,     7,     2,     2,     3,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     0,     2,     0,     2,     1,     1,     1,     1,
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
       2,     3,     1,     0,     0,     0,     0,   333,   358,   336,
     359,   360,   362,   363,   337,   335,   357,   341,   338,   339,
     340,   331,   332,    78,     0,    14,   284,     0,   354,   352,
       0,     0,     0,   288,   127,     0,   484,   334,   277,     0,
     225,     0,   329,   330,   361,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   481,    27,     4,    10,
       9,   228,   228,   228,     8,    11,     0,     0,     0,     0,
       0,     0,     0,   166,     5,     0,     0,   270,   279,     0,
     281,   286,   287,     0,   285,   320,   328,   327,   356,    22,
      23,   333,   331,   332,   334,   329,   330,   482,    36,   322,
      32,   321,     0,     0,   333,     0,     0,   334,     0,     0,
     482,   292,   325,   267,   266,   326,   268,   269,     0,    76,
     323,   324,   482,     0,     0,   277,     0,     0,     0,     0,
       0,   274,     0,   271,   129,     0,   307,   310,   309,   308,
     311,   312,   482,    29,     0,   482,   290,   278,   225,     0,
     272,     0,     0,     0,   395,   399,   401,     0,     0,   408,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   319,   318,   313,     0,   225,     0,
     230,   315,   316,     0,     0,     0,     0,     0,     0,     0,
     228,     0,     0,     0,   183,   482,    21,    18,    19,    17,
      15,   265,   267,   266,     0,   264,   241,   242,   268,   269,
       0,   167,   172,   145,   177,   225,   215,     0,   254,   253,
       0,   275,   280,   282,   283,     0,     0,    24,     0,     0,
      65,     0,    65,   333,   331,   332,   334,   329,   330,   325,
     326,   323,   324,   168,     0,     0,     0,    80,     0,     0,
     482,     0,   169,   355,   353,   276,     0,   128,   136,   298,
     300,   299,   297,   301,   302,   303,   289,   304,     0,     0,
     514,   497,   525,   527,   526,   518,   531,   509,   505,   506,
     504,   507,   508,   522,   500,   499,   519,   520,   510,   488,
     511,   512,   513,   515,   517,   498,   521,   492,   528,   529,
     532,   516,   530,   523,   455,   456,   457,   458,   459,   460,
     461,   462,   468,   469,   463,   464,   465,   466,   467,   470,
     471,   472,   473,   474,   475,   476,   477,   478,   479,   480,
     524,   496,   482,   482,   501,   502,   114,   482,   482,   449,
     450,   494,   448,   441,   444,   445,   447,   442,   443,   454,
     451,   452,   453,   482,   446,   495,   503,   493,   533,   485,
     489,   491,   490,     0,     0,     0,     2,   273,   221,   226,
       0,     0,   265,   264,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    12,   224,   229,   253,     0,
     314,   317,     6,     7,   126,     0,   213,     0,     0,     0,
       0,     0,    20,    16,     0,     0,   455,   456,     0,     0,
     174,   440,   165,   146,     0,   179,   179,     0,   217,   222,
     216,   249,     0,     0,   235,   255,   260,   185,   187,   155,
     306,   298,   300,   299,   297,   301,   302,     0,   164,   149,
     190,     0,   305,     0,   486,    40,   483,   487,   294,     0,
       0,     0,     0,     0,    25,     0,     0,    81,    83,    80,
     113,     0,   203,     0,   150,     0,   138,   139,     0,   131,
       0,   140,   140,    31,     2,     0,     0,     0,     0,     0,
      26,     0,   220,     3,   228,   143,   393,     0,     0,     0,
     403,   406,     0,     0,   413,   417,   421,   425,   415,   419,
     423,   427,   429,   431,   433,   434,     0,   231,   123,   214,
     124,   125,   122,    13,   184,     0,     0,   436,   437,     0,
     156,   182,     0,     0,   173,   178,   219,     0,   236,   250,
     257,     0,   203,     0,   147,   199,     0,   194,   192,     0,
     295,    38,     0,     0,     0,    66,    67,    69,    41,    34,
      41,   293,    79,    80,     0,     0,   112,     0,   209,   148,
       0,   130,   132,   140,   135,   141,   134,     3,   537,   538,
     536,   534,   535,   291,    28,   227,     0,   396,   400,   402,
       0,     0,   409,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   367,   438,   439,   175,   181,   180,
     391,   383,   386,   388,   387,   389,   390,   385,   392,   378,
     365,   375,   373,   218,   364,   377,   372,   243,   258,     0,
       0,   200,     0,   191,   203,     0,   153,     0,    65,    72,
      70,    71,     0,    42,    65,    42,    82,   265,    90,    92,
      91,    93,    89,   264,    99,   101,   102,   103,    94,    85,
      84,    95,    86,    88,    87,    77,   170,   212,   205,   204,
     206,     0,     0,   154,   136,   137,   215,    30,     0,     0,
     404,     0,     0,   412,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   368,     0,   203,   380,     0,     0,
       0,     0,   384,   244,   237,   246,   262,   186,     0,   203,
     196,     0,     0,   151,   296,     0,    68,    37,     0,     0,
      33,     0,   104,   105,   106,   107,   108,   109,   110,   111,
      97,     0,   179,   207,   251,   152,   133,   142,   394,   397,
       0,   407,   410,   414,   418,   422,   426,   416,   420,   424,
     428,     0,     0,   435,     0,     0,     0,   369,   391,   376,
     374,   203,   263,     0,   188,     0,     0,   195,   482,   193,
      41,     0,     0,    73,    74,    75,     0,   333,   331,   332,
      53,     0,     0,   334,     0,   329,   330,     0,    64,     0,
      43,     0,   228,   228,    48,    52,    50,    49,     0,     0,
       0,     0,   160,    45,     0,    62,    63,    41,     0,     0,
      96,   171,   209,   239,   240,   238,   210,   251,   254,   252,
       0,   484,     0,     0,     0,   176,     0,   379,   367,     0,
     259,   203,   201,   196,     0,    42,   119,   482,   484,   118,
       0,     0,     0,     0,   270,    51,     0,     0,     0,     0,
     161,     0,   482,   157,    44,     0,     0,   115,     0,     0,
      60,    57,    54,   162,    42,   100,    98,   208,   215,   247,
       0,   251,   232,   398,     0,   411,   430,   432,     0,   370,
       0,   245,     0,   197,   198,    39,     0,     0,   117,   116,
     159,   163,    58,    55,   158,     0,     0,     0,     0,    46,
      47,    59,    56,    35,   211,   233,   248,   405,   381,     0,
     366,   189,   484,   121,   144,     0,   243,     0,   371,     0,
      61,   234,     0,   120,   382
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     3,    78,    79,    80,   289,    81,   252,   664,
     250,   658,   663,   738,   810,   481,   575,   576,   811,    82,
     268,   138,   486,   487,   678,   679,   680,   751,   829,   741,
     681,   750,    83,    84,   815,   816,   859,    85,    86,    87,
     278,   498,   694,   499,   500,   501,   594,   696,    88,   819,
      89,    90,   230,   820,    91,    92,    93,   492,   752,   231,
     445,   232,   549,   716,   233,   446,   554,   216,   234,   562,
     563,   851,   822,   567,   470,   654,   653,   787,   730,   564,
     565,   729,   587,   588,   689,   832,   690,   691,   888,   425,
     449,   450,   557,    94,   235,   197,   171,   514,   198,   199,
     419,   836,   936,   236,   647,   837,   237,   724,   781,   890,
     452,   838,   238,   454,   455,   456,   560,   726,   561,   783,
     471,   863,    96,    97,    98,    99,   100,   155,   101,   384,
     266,   479,   657,   472,   102,   129,   200,   201,   202,   104,
     105,   106,   107,   149,   148,   108,   643,   719,   715,   848,
     929,   644,   721,   720,   718,   775,   937,   645,   646,   109,
     606,   395,   699,   840,   396,   397,   610,   760,   611,   400,
     702,   842,   614,   618,   615,   619,   616,   620,   617,   621,
     622,   623,   440,   377,   378,   248,   164,   476,   477,   380,
     381,   382
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -785
static const yytype_int16 yypact[] =
{
    -785,    71,  -785,  4197,   481,   673,  4825,     8,  -785,  -785,
    -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,
    -785,   -32,    86,   800,   509,  -785,  -785,  4411,  -785,  -785,
    4506,  4825,   -48,  -785,  -785,   538,  -785,   108,   122,  4601,
    -785,   -37,   129,   154,  -785,  -785,  -785,  -785,  -785,  -785,
    -785,  -785,  -785,  -785,  -785,   -13,    79,    90,   103,   119,
     121,   126,   133,   144,   148,   165,   167,   170,   185,   188,
     198,   200,   204,   215,   218,   219,  -785,  -785,  -785,  -785,
    -785,    26,    26,    26,  -785,  -785,  4696,  4316,   147,   147,
     147,   147,   147,  -785,  -785,   469,  4825,  -785,    12,  4920,
      85,    58,  -785,   155,  -785,  -785,  -785,  -785,  -785,  -785,
    -785,   245,   253,   262,   288,   296,   304,  -785,   113,  -785,
     187,  -785,   804,   804,   -18,    67,    72,   -15,   306,   264,
    -785,  -785,   225,  -785,  -785,   234,  -785,  -785,   235,  -785,
     225,   234,  -785,   237,  4506,   320,  4791,   246,   600,   600,
    4825,  -785,   290,  -785,   249,   806,  -785,  -785,  -785,  -785,
    -785,  -785,  -785,  -785,  3026,  -785,  -785,  -785,  -785,  4089,
    -785,   117,  4411,   810,  -785,  -785,  -785,   810,   810,  -785,
     810,   810,   810,   810,   810,   810,   810,   810,   810,   810,
     810,   810,   810,   810,  -785,  -785,  -785,   259,  -785,   650,
    -785,    18,  -785,   263,   268,   273,   223,   223,   223,   650,
      26,   147,   147,   767,  -785,  -785,  -785,  -785,  -785,  -785,
    -785,   313,   321,   322,  5044,   331,  -785,  -785,   336,   339,
     777,  -785,  -785,  -785,  -785,  -785,   272,   396,   267,   -24,
     355,  -785,  -785,  -785,  -785,   806,    65,  -785,   911,  4825,
     310,  4825,   310,  -785,  -785,  -785,  -785,  -785,  -785,  -785,
    -785,  -785,  -785,  -785,   806,  1052,  4825,   810,   323,  1193,
    -785,  4825,  -785,  -785,  -785,  -785,   231,  -785,  5048,   -18,
     321,   322,   -15,   336,   339,    58,  -785,  -785,  1334,   324,
    -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,
    -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,
    -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,
    -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,
    -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,
    -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,
    -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,
    -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,
    -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,
    -785,  -785,  -785,  1475,  4825,   123,  -785,  -785,  -785,  -785,
     327,   810,  -785,  -785,   333,   810,   810,   810,   338,   348,
     810,   354,   357,   363,   364,   365,   367,   372,   373,   375,
     376,   380,   342,   345,   384,  -785,   385,  -785,  -785,   650,
    -785,  -785,  -785,  -785,  -785,   335,  -785,   810,   394,   400,
     401,   405,  -785,  -785,   -24,  1616,   344,   387,   411,   397,
    -785,  -785,  -785,  -785,   446,    24,    24,   138,  -785,  -785,
    -785,  -785,   414,   650,  -785,  -785,  -785,  -785,  -785,  -785,
    -785,   -25,   -23,     6,    88,    23,    77,   810,  -785,  -785,
    -785,   422,  -785,   463,  -785,  -785,  -785,  -785,   417,   421,
     622,   426,   424,   432,  -785,   430,   433,   431,   434,   810,
    -785,  1757,   440,   810,  -785,   483,  -785,  -785,   443,   444,
     529,   810,   810,  -785,  -785,  1898,  2039,  2180,  2321,  2462,
    -785,   448,  -785,   451,    12,  -785,  -785,   449,   458,   462,
    -785,  -785,   467,  4954,  -785,  -785,  -785,  -785,  -785,  -785,
    -785,  -785,  -785,  -785,  -785,  -785,   810,  -785,  -785,  -785,
    -785,  -785,  -785,  -785,  -785,   445,   456,  -785,  -785,   465,
    -785,  -785,   582,   583,  -785,  -785,  -785,   207,  -785,  -785,
     461,   466,   440,  4825,  -785,  -785,   478,   484,  -785,   325,
    -785,  -785,   806,   806,   806,  -785,   480,  -785,  -785,  -785,
    -785,  -785,  -785,   810,   312,   490,  -785,   488,   111,  -785,
     457,  -785,  -785,   810,  -785,  -785,  -785,   491,  -785,  -785,
    -785,  -785,  -785,  -785,  -785,  -785,  4825,  -785,  -785,  -785,
    4954,  4954,  -785,   503,  4954,  4954,  4954,  4954,  4954,  4954,
    4954,  4954,  4954,  4954,   501,  -785,  -785,  -785,  -785,  -785,
     506,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,
    -785,  -785,  -785,  -785,  -785,   624,  -785,   102,  -785,   517,
     515,  -785,   810,  -785,   440,   810,  -785,  4825,   310,  -785,
    -785,  -785,   622,   518,   310,   519,  -785,   -18,  -785,  -785,
    -785,  -785,  -785,   -15,  -785,  -785,  -785,  -785,   593,  -785,
    -785,  -785,  -785,    58,  -785,  -785,  -785,  -785,  -785,  -785,
     516,  4825,   810,  -785,  5048,  -785,   272,  -785,   524,  4825,
    -785,   526,  4954,  -785,   527,   531,   535,   537,   543,   544,
     545,   546,   532,   541,  -785,   548,   440,  -785,   460,   207,
     705,   705,  -785,  -785,  -785,  -785,   312,  -785,   551,   440,
     549,   552,   556,  -785,  -785,   559,  -785,  -785,  3590,   560,
    -785,   312,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,
    -785,   312,    24,  -785,   553,  -785,  -785,  -785,  -785,  -785,
     557,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,
    -785,   679,   680,  -785,   570,  4825,   571,  -785,  -785,  -785,
    -785,   440,  -785,   564,  -785,   575,   810,  -785,  -785,  -785,
    -785,   486,   673,  -785,  -785,  -785,  3899,   -25,   -23,     6,
    -785,  3709,  3994,    88,   579,    23,    77,   584,  -785,   810,
    -785,   585,    26,    26,  -785,  -785,  -785,  -785,  3709,   147,
     147,   147,  -785,  -785,   626,  -785,  -785,  -785,   592,   312,
    -785,  -785,   111,  -785,  -785,  -785,  -785,   369,   267,  -785,
     618,  -785,   623,   634,   639,  -785,   637,  -785,   620,   643,
    -785,   440,  -785,   549,  2603,   642,  -785,  -785,  -785,  -785,
     191,   191,   810,   810,   306,  -785,   147,   147,   767,   810,
    -785,  3804,  -785,  -785,  -785,   647,   648,  -785,   147,   147,
    -785,  -785,  -785,  -785,   645,  -785,  -785,  -785,   272,  -785,
     649,   553,  -785,  -785,  3167,  -785,  -785,  -785,   651,   646,
     652,  -785,   655,  -785,  -785,  -785,  2744,  3308,  -785,  -785,
    -785,  -785,  -785,  -785,  -785,  3899,   656,   810,  2885,  -785,
    -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,   207,
    -785,  -785,  -785,  -785,  -785,   654,   102,   705,  -785,  3449,
    -785,  -785,   658,  -785,  -785
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -785,  -342,  -785,  -785,  -785,  -785,  -785,   192,  -785,  -785,
    -785,  -785,  -545,  -785,  -785,  -229,   127,  -785,  -785,   -75,
    -785,  -785,  -442,  -785,  -785,  -785,  -672,  -785,  -785,  -785,
    -785,  -785,   -74,    45,  -785,  -735,  -577,    61,  -785,  -464,
    -785,   110,  -785,  -785,  -785,  -785,  -432,  -785,  -785,  -785,
      -1,  -785,  -785,  -753,  -785,   -53,   550,  -785,  -785,  -110,
    -785,  -785,  -785,  -785,  -222,  -785,  -422,   -76,  -785,  -785,
    -785,  -785,  -225,  -785,  -785,  -785,  -785,   -42,    28,  -461,
    -785,  -785,  -516,  -785,   -20,  -785,  -785,  -785,  -785,    78,
    -653,  -785,  -785,    83,  -161,   -52,  -116,  -785,   308,  -785,
    -785,  -784,  -785,  -176,  -785,  -785,  -785,  -112,  -785,  -785,
    -785,  -785,  -698,   -12,  -622,  -785,  -785,  -785,  -785,  -785,
      -4,     0,   -28,    -2,    48,   728,   730,  -785,  -133,  -785,
    -785,  -193,  -785,  -113,   -49,    42,   -81,  -785,   631,  -317,
    -252,     1,  -155,  -785,  -785,   184,  -678,  -785,   -10,  -785,
    -785,   124,  -785,  -785,  -785,  -785,  -785,  -785,  -681,   101,
    -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,
    -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,  -785,
    -785,  -785,  -785,   619,     4,   -62,  -766,  -785,  -154,  -785,
    -785,  -785
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -369
static const yytype_int16 yytable[] =
{
     118,   120,   150,    95,   128,   119,   121,   110,   443,   376,
     379,   207,   208,   217,   218,   219,   220,   242,   263,   139,
     143,   469,   285,   483,   555,   725,   502,   147,   151,   153,
     203,   204,   589,   430,   212,   665,   593,   170,   417,   779,
     780,   777,   286,   757,   513,   103,   649,   585,   866,   194,
    -310,   494,   385,   889,   782,   194,   839,  -307,   482,  -310,
     551,   451,   552,   194,  -307,   878,   865,  -308,   265,   828,
     596,     2,   152,   485,   461,   894,   259,   163,   154,   830,
     269,   261,   416,   877,   209,   172,   211,   213,  -309,  -310,
    -307,   239,   907,   376,   241,   462,   463,  -265,   457,  -267,
     288,   131,   458,   383,    27,  -311,   287,   926,   131,   173,
     376,   166,   285,   285,   376,   464,   150,   260,   916,   447,
     442,    26,   262,   119,   121,   427,   427,   427,  -266,  -307,
     130,   285,   460,   376,   131,   432,   433,   240,   732,   839,
     245,   666,   151,   285,   170,  -268,   195,  -223,   275,   196,
     465,   466,   195,   435,   553,   420,   453,   886,   431,  -312,
     195,   695,   597,   196,   687,   167,   939,   387,  -309,   394,
    -308,   390,   391,   398,   399,   168,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
    -308,   511,   688,   839,   733,   418,   287,   287,   467,  -269,
     774,   174,   426,   426,   426,   418,   613,  -309,   491,   434,
    -264,  -311,   175,   785,   166,   287,   630,   631,   632,   633,
     634,   635,   636,   637,   723,   176,   434,   287,   376,  -308,
     165,   755,   392,   418,   166,   924,  -312,   246,   388,   249,
     461,   177,  -256,   178,   512,   855,   389,   478,   179,   478,
    -311,   938,   389,   133,   134,   180,   942,   638,   537,   556,
     194,   462,   463,   488,   478,   849,   181,   389,   214,   387,
     182,   215,   444,   393,   818,  -312,   247,   559,   206,   210,
     376,   464,   884,   908,   909,   428,   429,   183,   473,   184,
     505,   506,   185,   700,   701,   507,   508,   704,   705,   706,
     707,   708,   709,   710,   711,   712,   713,   186,   136,   137,
     187,   509,   856,   251,   725,   857,   465,   466,   495,   858,
     188,   667,   189,   668,   669,   670,   190,   671,   672,   639,
     831,   640,   273,   274,   461,   902,   376,   191,   641,   642,
     192,   193,   222,   223,   656,   224,   264,   285,   873,  -265,
     376,   376,   376,   376,   376,   462,   463,   195,  -264,   267,
     196,   270,   673,   167,   493,   693,  -265,   577,   272,  -265,
     650,  -265,   276,  -265,  -267,   464,   277,  -267,   392,  -267,
     415,  -267,   478,  -266,   422,   762,  -266,   434,  -266,   423,
    -266,   517,   518,   519,   424,  -307,   522,   228,   229,   133,
     134,   910,   448,  -310,  -309,   392,   194,  -256,   914,  -264,
     465,   466,  -264,  -308,  -264,   418,  -264,  -268,  -311,   393,
    -268,  -312,  -268,   539,  -268,  -269,   133,   134,  -269,   735,
    -269,   287,  -269,   194,   674,   739,   285,   459,   480,   285,
     285,   285,   502,   675,   676,   677,   393,   489,   504,   418,
     515,   683,   833,   834,   136,   137,   538,   285,   655,   659,
     660,   661,   516,   566,   734,   534,   461,   520,   535,   630,
     631,   632,   633,   634,   635,   636,   637,   521,   221,   226,
     227,   136,   137,   523,   545,   488,   524,   462,   463,   566,
     111,   835,   525,   526,   527,   111,   528,   595,   595,   222,
     223,   529,   530,   195,   531,   532,   196,   464,   224,   533,
     638,   112,   113,   536,   389,   540,   112,   113,   140,   225,
     287,   541,   542,   287,   287,   287,   543,   546,   550,   285,
     195,   114,   624,   196,   547,   684,   114,   558,   548,   133,
     134,   287,   465,   466,   568,   569,   570,   156,   571,   577,
     578,   579,   226,   227,   228,   229,   580,   581,   582,   141,
     583,   478,   392,  -202,   584,   590,   115,   116,   157,   158,
     591,   115,   116,   592,    32,   603,   604,   870,   607,   488,
     682,   608,   639,   133,   134,   609,   625,   627,   159,   595,
     692,   641,   642,   683,   136,   137,   612,   626,   628,   629,
     651,  -261,   883,   393,   698,   117,   648,   856,   683,   662,
     857,   686,   652,   287,   858,   685,   697,     8,   683,    10,
      11,    12,    13,   160,   161,    16,   703,   572,   573,   574,
     714,   279,   717,   142,   722,   392,   833,   834,   136,   137,
     727,   911,   728,   737,   740,   753,   883,   758,   731,   761,
     763,   566,   280,   281,   764,   478,   133,   134,   765,   392,
     766,   771,   162,   812,   813,   224,   767,   768,   769,   770,
     772,   773,   282,   784,   788,   835,   393,   684,   786,   789,
     133,   134,   111,   790,   827,   821,   841,    44,   566,   754,
     843,   844,   684,   845,   847,   883,   683,   759,   852,   376,
     393,   871,   684,   112,   113,   850,   872,   283,   284,   226,
     227,   136,   137,   874,   778,   885,   632,   633,   634,   635,
     636,   637,   682,   114,   742,   743,   854,   744,   745,   746,
     747,   748,   749,   226,   227,   136,   137,   682,   824,   376,
     379,   893,   826,   880,   881,   882,   895,   682,   867,   899,
     418,   376,   376,   379,   263,   638,   891,   896,   115,   116,
     875,   876,   897,   376,   898,   879,   901,   905,   919,   920,
     923,  -368,   925,   928,   150,   940,   392,   930,   931,   934,
     684,   944,   731,   814,   376,   379,   221,   860,   861,   736,
     912,   913,   119,   121,   864,   906,   468,   133,   134,   817,
     151,   868,   921,   922,   756,   566,   224,   222,   223,   132,
     918,   903,   887,   253,   853,   279,   224,   393,   868,   392,
     239,   823,   605,   846,   941,   682,   892,   225,   244,   243,
     133,   134,   421,   418,   254,   255,   280,   281,   900,   825,
     133,   134,   776,   441,     0,     0,     0,     0,     0,     0,
     135,     0,   136,   137,   256,     0,   282,     0,   566,   434,
     393,     0,   228,   229,   434,   566,     0,     0,     0,     0,
       0,   917,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   136,   137,   418,     0,   257,
     258,   283,   284,     0,     0,   136,   137,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   434,   290,   291,   292,   293,   294,   295,
     296,   297,   298,   299,   300,   301,   302,   303,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,   304,   305,   306,   307,   308,   309,   310,   311,   312,
     313,    28,    29,     0,   314,     0,   315,   316,   317,   318,
     319,   320,   321,   322,   323,     0,   324,   325,   326,   327,
     328,   329,   330,   331,   332,   333,   334,   335,   336,   337,
     338,   339,   340,   341,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,   355,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      76,     0,   474,   357,     0,   358,   475,   359,   360,   361,
     362,   363,   364,   365,   366,   367,   368,   369,   370,   371,
     372,   373,     0,   374,   375,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,   304,   305,   306,   307,   308,   309,   310,   311,
     312,   313,    28,    29,     0,   314,     0,   315,   316,   317,
     318,   319,   320,   321,   322,   323,     0,   324,   325,   326,
     327,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    76,     0,   474,   357,   484,   358,     0,   359,   360,
     361,   362,   363,   364,   365,   366,   367,   368,   369,   370,
     371,   372,   373,     0,   374,   375,   290,   291,   292,   293,
     294,   295,   296,   297,   298,   299,   300,   301,   302,   303,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,   304,   305,   306,   307,   308,   309,   310,
     311,   312,   313,    28,    29,     0,   314,     0,   315,   316,
     317,   318,   319,   320,   321,   322,   323,     0,   324,   325,
     326,   327,   328,   329,   330,   331,   332,   333,   334,   335,
     336,   337,   338,   339,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    76,     0,   474,   357,     0,   358,   490,   359,
     360,   361,   362,   363,   364,   365,   366,   367,   368,   369,
     370,   371,   372,   373,     0,   374,   375,   290,   291,   292,
     293,   294,   295,   296,   297,   298,   299,   300,   301,   302,
     303,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,   304,   305,   306,   307,   308,   309,
     310,   311,   312,   313,    28,    29,     0,   314,     0,   315,
     316,   317,   318,   319,   320,   321,   322,   323,     0,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    76,     0,   474,   357,     0,   358,   503,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   368,
     369,   370,   371,   372,   373,     0,   374,   375,   290,   291,
     292,   293,   294,   295,   296,   297,   298,   299,   300,   301,
     302,   303,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,   304,   305,   306,   307,   308,
     309,   310,   311,   312,   313,    28,    29,     0,   314,     0,
     315,   316,   317,   318,   319,   320,   321,   322,   323,     0,
     324,   325,   326,   327,   328,   329,   330,   331,   332,   333,
     334,   335,   336,   337,   338,   339,   340,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    76,     0,   474,   357,   510,   358,
       0,   359,   360,   361,   362,   363,   364,   365,   366,   367,
     368,   369,   370,   371,   372,   373,     0,   374,   375,   290,
     291,   292,   293,   294,   295,   296,   297,   298,   299,   300,
     301,   302,   303,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,   304,   305,   306,   307,
     308,   309,   310,   311,   312,   313,    28,    29,     0,   314,
       0,   315,   316,   317,   318,   319,   320,   321,   322,   323,
       0,   324,   325,   326,   327,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    76,     0,   474,   357,     0,
     358,   544,   359,   360,   361,   362,   363,   364,   365,   366,
     367,   368,   369,   370,   371,   372,   373,     0,   374,   375,
     290,   291,   292,   293,   294,   295,   296,   297,   298,   299,
     300,   301,   302,   303,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,   304,   305,   306,
     307,   308,   309,   310,   311,   312,   313,    28,    29,     0,
     314,     0,   315,   316,   317,   318,   319,   320,   321,   322,
     323,     0,   324,   325,   326,   327,   328,   329,   330,   331,
     332,   333,   334,   335,   336,   337,   338,   339,   340,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    76,     0,   474,   357,
       0,   358,   586,   359,   360,   361,   362,   363,   364,   365,
     366,   367,   368,   369,   370,   371,   372,   373,     0,   374,
     375,   290,   291,   292,   293,   294,   295,   296,   297,   298,
     299,   300,   301,   302,   303,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,   304,   305,
     306,   307,   308,   309,   310,   311,   312,   313,    28,    29,
       0,   314,     0,   315,   316,   317,   318,   319,   320,   321,
     322,   323,     0,   324,   325,   326,   327,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    76,     0,   474,
     357,   598,   358,     0,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,     0,
     374,   375,   290,   291,   292,   293,   294,   295,   296,   297,
     298,   299,   300,   301,   302,   303,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,    28,
      29,     0,   314,     0,   315,   316,   317,   318,   319,   320,
     321,   322,   323,     0,   324,   325,   326,   327,   328,   329,
     330,   331,   332,   333,   334,   335,   336,   337,   338,   339,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,   355,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    76,     0,
     474,   357,   599,   358,     0,   359,   360,   361,   362,   363,
     364,   365,   366,   367,   368,   369,   370,   371,   372,   373,
       0,   374,   375,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
     304,   305,   306,   307,   308,   309,   310,   311,   312,   313,
      28,    29,     0,   314,     0,   315,   316,   317,   318,   319,
     320,   321,   322,   323,     0,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    76,
       0,   474,   357,   600,   358,     0,   359,   360,   361,   362,
     363,   364,   365,   366,   367,   368,   369,   370,   371,   372,
     373,     0,   374,   375,   290,   291,   292,   293,   294,   295,
     296,   297,   298,   299,   300,   301,   302,   303,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,   304,   305,   306,   307,   308,   309,   310,   311,   312,
     313,    28,    29,     0,   314,     0,   315,   316,   317,   318,
     319,   320,   321,   322,   323,     0,   324,   325,   326,   327,
     328,   329,   330,   331,   332,   333,   334,   335,   336,   337,
     338,   339,   340,   341,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,   355,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      76,     0,   474,   357,     0,   358,   601,   359,   360,   361,
     362,   363,   364,   365,   366,   367,   368,   369,   370,   371,
     372,   373,     0,   374,   375,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,   304,   305,   306,   307,   308,   309,   310,   311,
     312,   313,    28,    29,     0,   314,     0,   315,   316,   317,
     318,   319,   320,   321,   322,   323,     0,   324,   325,   326,
     327,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    76,     0,   474,   357,     0,   358,     0,   359,   360,
     361,   362,   363,   364,   365,   366,   367,   368,   369,   370,
     371,   372,   373,   602,   374,   375,   290,   291,   292,   293,
     294,   295,   296,   297,   298,   299,   300,   301,   302,   303,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,   304,   305,   306,   307,   308,   309,   310,
     311,   312,   313,    28,    29,     0,   314,     0,   315,   316,
     317,   318,   319,   320,   321,   322,   323,     0,   324,   325,
     326,   327,   328,   329,   330,   331,   332,   333,   334,   335,
     336,   337,   338,   339,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    76,     0,   474,   357,   904,   358,     0,   359,
     360,   361,   362,   363,   364,   365,   366,   367,   368,   369,
     370,   371,   372,   373,     0,   374,   375,   290,   291,   292,
     293,   294,   295,   296,   297,   298,   299,   300,   301,   302,
     303,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,   304,   305,   306,   307,   308,   309,
     310,   311,   312,   313,    28,    29,     0,   314,     0,   315,
     316,   317,   318,   319,   320,   321,   322,   323,     0,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    76,     0,   474,   357,     0,   358,   932,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   368,
     369,   370,   371,   372,   373,     0,   374,   375,   290,   291,
     292,   293,   294,   295,   296,   297,   298,   299,   300,   301,
     302,   303,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,   304,   305,   306,   307,   308,
     309,   310,   311,   312,   313,    28,    29,     0,   314,     0,
     315,   316,   317,   318,   319,   320,   321,   322,   323,     0,
     324,   325,   326,   327,   328,   329,   330,   331,   332,   333,
     334,   335,   336,   337,   338,   339,   340,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    76,     0,   474,   357,   935,   358,
       0,   359,   360,   361,   362,   363,   364,   365,   366,   367,
     368,   369,   370,   371,   372,   373,     0,   374,   375,   290,
     291,   292,   293,   294,   295,   296,   297,   298,   299,   300,
     301,   302,   303,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,   304,   305,   306,   307,
     308,   309,   310,   311,   312,   313,    28,    29,     0,   314,
       0,   315,   316,   317,   318,   319,   320,   321,   322,   323,
       0,   324,   325,   326,   327,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    76,     0,   356,   357,     0,
     358,     0,   359,   360,   361,   362,   363,   364,   365,   366,
     367,   368,   369,   370,   371,   372,   373,     0,   374,   375,
     290,   291,   292,   293,   294,   295,   296,   297,   298,   299,
     300,   301,   302,   303,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,   304,   305,   306,
     307,   308,   309,   310,   311,   312,   313,    28,    29,     0,
     314,     0,   315,   316,   317,   318,   319,   320,   321,   322,
     323,     0,   324,   325,   326,   327,   328,   329,   330,   331,
     332,   333,   334,   335,   336,   337,   338,   339,   340,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    76,     0,     0,   357,
     927,   358,     0,   359,   360,   361,   362,   363,   364,   365,
     366,   367,   368,   369,   370,   371,   372,   373,     0,   374,
     375,   290,   291,   292,   293,   294,   295,   296,   297,   298,
     299,   300,   301,   302,   303,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,   304,   305,
     306,   307,   308,   309,   310,   311,   312,   313,    28,    29,
       0,   314,     0,   315,   316,   317,   318,   319,   320,   321,
     322,   323,     0,   324,   325,   326,   327,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    76,     0,   933,
     357,     0,   358,     0,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,     0,
     374,   375,   290,   291,   292,   293,   294,   295,   296,   297,
     298,   299,   300,   301,   302,   303,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,    28,
      29,     0,   314,     0,   315,   316,   317,   318,   319,   320,
     321,   322,   323,     0,   324,   325,   326,   327,   328,   329,
     330,   331,   332,   333,   334,   335,   336,   337,   338,   339,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,   355,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    76,     0,
     943,   357,     0,   358,     0,   359,   360,   361,   362,   363,
     364,   365,   366,   367,   368,   369,   370,   371,   372,   373,
       0,   374,   375,   791,   792,   793,   794,   795,   796,   797,
       0,     0,     0,     0,     0,     0,     0,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
     798,   799,    23,    24,   800,     0,    26,     0,     0,    27,
      28,    29,   801,   802,    31,    32,    33,    34,     0,    36,
     803,    38,   146,    40,   804,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   805,   806,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
     807,   808,   791,   792,     0,     0,     0,   796,   797,     0,
       0,     0,     0,   809,     0,     0,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,   798,
     799,   125,   126,     0,     0,    26,     0,     0,    27,    28,
      29,     0,   802,    31,     0,    33,     0,     0,     0,   803,
     145,   146,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   805,   806,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,   122,   123,     0,
       0,     0,   915,   797,     0,     0,     0,     0,     0,     0,
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,   798,   799,   125,   126,     0,     0,
      26,     0,   809,     0,    28,    29,     0,   802,    31,     0,
      33,     0,     0,     0,   803,   145,   146,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   805,
     806,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,   122,   123,     0,     0,     0,     0,   124,     0,
       0,     0,     0,     0,     0,     0,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,   125,   126,     0,     0,    26,     0,   809,     0,    28,
      29,     0,   144,    31,     0,    33,     0,     0,     0,   127,
     145,   146,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,   122,   123,     0,
       0,     0,     0,   797,     0,     0,     0,     0,     0,     0,
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,   798,   799,   125,   126,     0,     0,
      26,     0,   862,     0,    28,    29,     0,     0,     0,     0,
      33,     0,     0,     0,   803,   145,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   805,
     806,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,   122,   123,     0,     0,     0,     0,   124,     0,
       0,     0,     0,     0,     0,     0,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,   125,   126,     0,     0,    26,     0,   869,     0,    28,
      29,     0,     0,     0,     0,    33,     0,     0,     0,   127,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       4,     5,     0,     0,     0,     6,     7,     0,     0,     0,
       0,     0,     0,   386,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,     0,    26,     0,     0,    27,    28,    29,     0,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,     0,    77,     4,
       5,     0,     0,     0,     6,   124,     0,     0,     0,     0,
       0,     0,     0,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,   125,   126,
       0,     0,    26,     0,     0,    27,    28,    29,     0,   144,
      31,     0,    33,     0,     0,     0,   127,   145,   146,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,   122,   123,     0,     0,     0,     0,
     124,     0,     0,     0,     0,     0,     0,     0,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,   125,   126,     0,     0,    26,     0,     0,
       0,    28,    29,     0,   144,    31,     0,    33,     0,     0,
       0,   127,   145,   146,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,   122,
     123,     0,     0,     0,     0,   124,     0,     0,     0,     0,
       0,     0,     0,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,   125,   126,
       0,     0,    26,     0,     0,     0,    28,    29,     0,     0,
       0,     0,    33,     0,     0,     0,   127,   145,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,   122,   123,     0,     0,     0,     0,
     124,   169,     0,     0,     0,     0,     0,     0,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,   125,   126,     0,     0,    26,     0,     0,
       0,    28,    29,     0,     0,     0,     0,    33,     0,     0,
       0,   127,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,     4,
       5,     0,     0,     0,     0,   124,     0,     0,     0,     0,
       0,     0,     0,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
       0,     0,    26,     0,     0,     0,    28,    29,     0,     0,
       0,     0,    33,     0,     0,     0,   127,     0,     0,   205,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,   122,   123,     0,     0,     0,     0,
     124,   271,     0,     0,     0,     0,     0,     0,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,   125,   126,     0,     0,    26,   122,   123,
       0,    28,    29,     0,   124,     0,     0,    33,     0,     0,
       0,   127,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,   125,   126,     0,
       0,    26,     0,     0,     0,    28,    29,     0,     0,     0,
       0,    33,     0,     0,     0,   127,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,   122,   123,     0,     0,     0,     0,   124,
       0,     0,     0,     0,     0,     0,     0,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,   125,   126,     0,     0,     0,   122,   123,     0,
      28,    29,     0,   253,     0,     0,    33,     0,     0,     0,
     127,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,   254,   255,   125,   126,     0,     0,
       0,     0,     0,     0,    28,    29,     0,     0,     0,     0,
       0,     0,     0,     0,   256,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   257,
     258,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,   496,     0,     0,     0,     0,   253,     0,     0,
       0,     0,     0,     0,     0,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,   254,   255,
       0,     0,     0,     0,     0,     0,     0,     0,    28,    29,
       0,     0,     0,     0,   497,     0,     0,     0,   256,   436,
     437,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,     0,     0,     0,     0,     0,
       0,     0,     0,   257,   258,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   438,     0,     0,     0,
     359,   360,     0,   362,   363,   364,   365,   366,   367,   368,
     369,   370,   371,   372,   439,     0,   374
};

static const yytype_int16 yycheck[] =
{
       4,     5,    30,     3,     6,     4,     5,     3,   230,   164,
     164,    86,    86,    89,    90,    91,    92,    98,   128,    23,
      24,   246,   155,   252,   446,   647,   278,    27,    30,    31,
      82,    83,   493,   209,    87,   580,   500,    39,   199,   720,
     721,   719,   155,   696,   386,     3,   562,   489,   801,    37,
      82,   276,   168,   837,   726,    37,   754,    82,   251,    82,
      36,   237,    38,    37,    82,   818,   801,    82,   130,   741,
     502,     0,    30,   266,     9,   841,     9,    35,   126,   751,
     142,     9,   198,   818,    86,   122,    87,    87,    82,   121,
      82,    95,   858,   248,    96,    30,    31,   122,   122,   122,
     162,   126,   126,   165,    39,    82,   155,   891,   126,   122,
     265,   126,   245,   246,   269,    50,   144,    50,   871,   235,
     230,    36,    50,   122,   123,   206,   207,   208,   122,   121,
     122,   264,   245,   288,   126,   211,   212,    95,   654,   837,
      82,   583,   144,   276,   146,   122,   134,   121,   150,   137,
      85,    86,   134,   215,   130,   137,   237,   829,   210,    82,
     134,   593,   504,   137,    53,    43,   932,   169,    82,   173,
      82,   172,   172,   177,   178,    53,   180,   181,   182,   183,
     184,   185,   186,   187,   188,   189,   190,   191,   192,   193,
      82,   384,    81,   891,   655,   199,   245,   246,   133,   122,
     716,   122,   206,   207,   208,   209,   523,   121,   270,   213,
     122,    82,   122,   729,   126,   264,     9,    10,    11,    12,
      13,    14,    15,    16,   122,   122,   230,   276,   383,   121,
     122,   692,     9,   237,   126,   888,    82,    82,   121,   126,
       9,   122,   140,   122,   121,   790,   129,   249,   122,   251,
     121,   929,   129,    30,    31,   122,   937,    50,   419,   121,
      37,    30,    31,   267,   266,   781,   122,   129,   121,   271,
     122,   124,   230,    50,   738,   121,   121,   453,    86,    87,
     435,    50,   827,   860,   861,   207,   208,   122,   246,   122,
     352,   353,   122,   610,   611,   357,   358,   614,   615,   616,
     617,   618,   619,   620,   621,   622,   623,   122,    85,    86,
     122,   373,   121,   126,   936,   124,    85,    86,   276,   128,
     122,     9,   122,    11,    12,    13,   122,    15,    16,   122,
     752,   124,   148,   149,     9,   851,   491,   122,   131,   132,
     122,   122,    30,    31,   569,    39,    82,   480,   809,   124,
     505,   506,   507,   508,   509,    30,    31,   134,   124,   124,
     137,   124,    50,    43,   133,   590,   121,   480,   122,   124,
     563,   126,    82,   128,   121,    50,   127,   124,     9,   126,
     121,   128,   384,   121,   121,   702,   124,   391,   126,   121,
     128,   395,   396,   397,   121,    82,   400,    85,    86,    30,
      31,   862,   130,    82,    82,     9,    37,   140,   869,   121,
      85,    86,   124,    82,   126,   419,   128,   121,    82,    50,
     124,    82,   126,   427,   128,   121,    30,    31,   124,   658,
     126,   480,   128,    37,   122,   664,   569,    82,   128,   572,
     573,   574,   694,   131,   132,   133,    50,   124,   124,   453,
     123,   584,    83,    84,    85,    86,   121,   590,   133,   572,
     573,   574,   129,   467,   657,   123,     9,   129,   123,     9,
      10,    11,    12,    13,    14,    15,    16,   129,     9,    83,
      84,    85,    86,   129,   140,   489,   129,    30,    31,   493,
       9,   122,   129,   129,   129,     9,   129,   501,   502,    30,
      31,   129,   129,   134,   129,   129,   137,    50,    39,   129,
      50,    30,    31,   129,   129,   121,    30,    31,     9,    50,
     569,   121,   121,   572,   573,   574,   121,   140,    82,   662,
     134,    50,   536,   137,   123,   584,    50,   123,   141,    30,
      31,   590,    85,    86,   122,    82,   129,     9,   127,   662,
     124,   127,    83,    84,    85,    86,   124,   127,   125,    50,
     129,   563,     9,   123,   130,    82,    85,    86,    30,    31,
     127,    85,    86,   129,    45,   127,   125,   802,   129,   583,
     584,   123,   122,    30,    31,   123,   141,   122,    50,   593,
     133,   131,   132,   726,    85,    86,   129,   141,    16,    16,
     122,   140,   824,    50,   606,   124,   140,   121,   741,   129,
     124,   123,   128,   662,   128,   125,   125,    17,   751,    19,
      20,    21,    22,    85,    86,    25,   123,     5,     6,     7,
     129,     9,   126,   124,    10,     9,    83,    84,    85,    86,
     123,   863,   127,   125,   125,   129,   868,   123,   652,   123,
     123,   655,    30,    31,   123,   657,    30,    31,   123,     9,
     123,   129,   124,   738,   738,    39,   123,   123,   123,   123,
     129,   123,    50,   122,   122,   122,    50,   726,   129,   123,
      30,    31,     9,   124,   124,   738,   129,    87,   692,   691,
      11,    11,   741,   123,   123,   917,   829,   699,   123,   854,
      50,   122,   751,    30,    31,   141,   122,    85,    86,    83,
      84,    85,    86,   128,     9,   123,    11,    12,    13,    14,
      15,    16,   726,    50,   131,   132,   788,   134,   135,   136,
     137,   138,   139,    83,    84,    85,    86,   741,   738,   894,
     894,   123,   738,   819,   820,   821,   123,   751,   801,   129,
     754,   906,   907,   907,   864,    50,   837,   123,    85,    86,
     812,   813,   123,   918,   127,   818,   123,   125,   121,   121,
     125,   125,   123,   122,   802,   121,     9,   125,   123,   123,
     829,   123,   786,   738,   939,   939,     9,   791,   792,   662,
     866,   867,   791,   792,   796,   857,   246,    30,    31,   738,
     802,   801,   878,   879,   694,   809,    39,    30,    31,     9,
     872,   853,   832,     9,   786,     9,    39,    50,   818,     9,
     824,   738,   514,   775,   936,   829,   838,    50,   100,    99,
      30,    31,   201,   837,    30,    31,    30,    31,   848,   738,
      30,    31,   718,   224,    -1,    -1,    -1,    -1,    -1,    -1,
      50,    -1,    85,    86,    50,    -1,    50,    -1,   862,   863,
      50,    -1,    85,    86,   868,   869,    -1,    -1,    -1,    -1,
      -1,   871,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    85,    86,   891,    -1,    85,
      86,    85,    86,    -1,    -1,    85,    86,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   917,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    -1,    43,    -1,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    -1,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     119,    -1,   121,   122,    -1,   124,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,    -1,   142,   143,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    -1,    43,    -1,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    -1,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   119,    -1,   121,   122,   123,   124,    -1,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,    -1,   142,   143,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    -1,    43,    -1,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    -1,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   119,    -1,   121,   122,    -1,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,    -1,   142,   143,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    -1,    43,    -1,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    -1,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   119,    -1,   121,   122,    -1,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,    -1,   142,   143,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    -1,    43,    -1,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    -1,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   119,    -1,   121,   122,   123,   124,
      -1,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,    -1,   142,   143,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    -1,    43,
      -1,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      -1,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   119,    -1,   121,   122,    -1,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,    -1,   142,   143,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    -1,
      43,    -1,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    -1,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   119,    -1,   121,   122,
      -1,   124,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,    -1,   142,
     143,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      -1,    43,    -1,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    -1,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   119,    -1,   121,
     122,   123,   124,    -1,   126,   127,   128,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,    -1,
     142,   143,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    -1,    43,    -1,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    -1,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   119,    -1,
     121,   122,   123,   124,    -1,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
      -1,   142,   143,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    -1,    43,    -1,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    -1,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   119,
      -1,   121,   122,   123,   124,    -1,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,    -1,   142,   143,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    -1,    43,    -1,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    -1,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     119,    -1,   121,   122,    -1,   124,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,    -1,   142,   143,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    -1,    43,    -1,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    -1,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   119,    -1,   121,   122,    -1,   124,    -1,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    -1,    43,    -1,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    -1,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   119,    -1,   121,   122,   123,   124,    -1,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,    -1,   142,   143,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    -1,    43,    -1,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    -1,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   119,    -1,   121,   122,    -1,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,    -1,   142,   143,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    -1,    43,    -1,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    -1,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   119,    -1,   121,   122,   123,   124,
      -1,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,    -1,   142,   143,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    -1,    43,
      -1,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      -1,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   119,    -1,   121,   122,    -1,
     124,    -1,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,    -1,   142,   143,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    -1,
      43,    -1,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    -1,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   119,    -1,    -1,   122,
     123,   124,    -1,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,    -1,   142,
     143,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      -1,    43,    -1,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    -1,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   119,    -1,   121,
     122,    -1,   124,    -1,   126,   127,   128,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,    -1,
     142,   143,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    -1,    43,    -1,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    -1,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   119,    -1,
     121,   122,    -1,   124,    -1,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
      -1,   142,   143,     3,     4,     5,     6,     7,     8,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    -1,    36,    -1,    -1,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    -1,    49,
      50,    51,    52,    53,    54,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,     3,     4,    -1,    -1,    -1,     8,     9,    -1,
      -1,    -1,    -1,   133,    -1,    -1,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    -1,    -1,    36,    -1,    -1,    39,    40,
      41,    -1,    43,    44,    -1,    46,    -1,    -1,    -1,    50,
      51,    52,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,     3,     4,    -1,
      -1,    -1,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    -1,    -1,
      36,    -1,   133,    -1,    40,    41,    -1,    43,    44,    -1,
      46,    -1,    -1,    -1,    50,    51,    52,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,     3,     4,    -1,    -1,    -1,    -1,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    -1,    -1,    36,    -1,   133,    -1,    40,
      41,    -1,    43,    44,    -1,    46,    -1,    -1,    -1,    50,
      51,    52,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,     3,     4,    -1,
      -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    -1,    -1,
      36,    -1,   133,    -1,    40,    41,    -1,    -1,    -1,    -1,
      46,    -1,    -1,    -1,    50,    51,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,     3,     4,    -1,    -1,    -1,    -1,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    -1,    -1,    36,    -1,   133,    -1,    40,
      41,    -1,    -1,    -1,    -1,    46,    -1,    -1,    -1,    50,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       3,     4,    -1,    -1,    -1,     8,     9,    -1,    -1,    -1,
      -1,    -1,    -1,   124,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    -1,    36,    -1,    -1,    39,    40,    41,    -1,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,    -1,   121,     3,
       4,    -1,    -1,    -1,     8,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      -1,    -1,    36,    -1,    -1,    39,    40,    41,    -1,    43,
      44,    -1,    46,    -1,    -1,    -1,    50,    51,    52,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,     3,     4,    -1,    -1,    -1,    -1,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    -1,    -1,    36,    -1,    -1,
      -1,    40,    41,    -1,    43,    44,    -1,    46,    -1,    -1,
      -1,    50,    51,    52,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,     3,
       4,    -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      -1,    -1,    36,    -1,    -1,    -1,    40,    41,    -1,    -1,
      -1,    -1,    46,    -1,    -1,    -1,    50,    51,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,     3,     4,    -1,    -1,    -1,    -1,
       9,    10,    -1,    -1,    -1,    -1,    -1,    -1,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    -1,    -1,    36,    -1,    -1,
      -1,    40,    41,    -1,    -1,    -1,    -1,    46,    -1,    -1,
      -1,    50,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,     3,
       4,    -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      -1,    -1,    36,    -1,    -1,    -1,    40,    41,    -1,    -1,
      -1,    -1,    46,    -1,    -1,    -1,    50,    -1,    -1,    53,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,     3,     4,    -1,    -1,    -1,    -1,
       9,    10,    -1,    -1,    -1,    -1,    -1,    -1,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    -1,    -1,    36,     3,     4,
      -1,    40,    41,    -1,     9,    -1,    -1,    46,    -1,    -1,
      -1,    50,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    -1,
      -1,    36,    -1,    -1,    -1,    40,    41,    -1,    -1,    -1,
      -1,    46,    -1,    -1,    -1,    50,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,     3,     4,    -1,    -1,    -1,    -1,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    -1,    -1,    -1,     3,     4,    -1,
      40,    41,    -1,     9,    -1,    -1,    46,    -1,    -1,    -1,
      50,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    -1,    -1,
      -1,    -1,    -1,    -1,    40,    41,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    50,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,     4,    -1,    -1,    -1,    -1,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    40,    41,
      -1,    -1,    -1,    -1,    46,    -1,    -1,    -1,    50,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   122,    -1,    -1,    -1,
     126,   127,    -1,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,    -1,   142
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   145,     0,   146,     3,     4,     8,     9,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    36,    39,    40,    41,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   121,   147,   148,
     149,   151,   163,   176,   177,   181,   182,   183,   192,   194,
     195,   198,   199,   200,   237,   265,   266,   267,   268,   269,
     270,   272,   278,   279,   283,   284,   285,   286,   289,   303,
     328,     9,    30,    31,    50,    85,    86,   124,   264,   285,
     264,   285,     3,     4,     9,    32,    33,    50,   267,   279,
     122,   126,     9,    30,    31,    50,    85,    86,   165,   264,
       9,    50,   124,   264,    43,    51,    52,   265,   288,   287,
     266,   267,   279,   267,   126,   271,     9,    30,    31,    50,
      85,    86,   124,   279,   330,   122,   126,    43,    53,    10,
     267,   240,   122,   122,   122,   122,   122,   122,   122,   122,
     122,   122,   122,   122,   122,   122,   122,   122,   122,   122,
     122,   122,   122,   122,    37,   134,   137,   239,   242,   243,
     280,   281,   282,   239,   239,    53,   151,   163,   176,   267,
     151,   194,   199,   265,   121,   124,   211,   211,   211,   211,
     211,     9,    30,    31,    39,    50,    83,    84,    85,    86,
     196,   203,   205,   208,   212,   238,   247,   250,   256,   264,
     279,   267,   280,   270,   269,    82,    82,   121,   329,   126,
     154,   126,   152,     9,    30,    31,    50,    85,    86,     9,
      50,     9,    50,   203,    82,   329,   274,   124,   164,   329,
     124,    10,   122,   289,   289,   267,    82,   127,   184,     9,
      30,    31,    50,    85,    86,   272,   277,   278,   329,   150,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    43,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,   121,   122,   124,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   142,   143,   286,   327,   328,   332,
     333,   334,   335,   329,   273,   240,   124,   267,   121,   129,
     194,   265,     9,    50,   264,   305,   308,   309,   264,   264,
     313,   264,   264,   264,   264,   264,   264,   264,   264,   264,
     264,   264,   264,   264,   264,   121,   240,   238,   264,   244,
     137,   282,   121,   121,   121,   233,   264,   280,   233,   233,
     247,   239,   211,   211,   264,   329,    55,    56,   122,   140,
     326,   327,   203,   208,   279,   204,   209,   240,   130,   234,
     235,   247,   254,   280,   257,   258,   259,   122,   126,    82,
     277,     9,    30,    31,    50,    85,    86,   133,   200,   216,
     218,   264,   277,   279,   121,   125,   331,   332,   267,   275,
     128,   159,   275,   159,   123,   275,   166,   167,   264,   124,
     125,   329,   201,   133,   216,   279,     4,    46,   185,   187,
     188,   189,   284,   125,   124,   329,   329,   329,   329,   329,
     123,   275,   121,   145,   241,   123,   129,   264,   264,   264,
     129,   129,   264,   129,   129,   129,   129,   129,   129,   129,
     129,   129,   129,   129,   123,   123,   129,   238,   121,   264,
     121,   121,   121,   121,   125,   140,   140,   123,   141,   206,
      82,    36,    38,   130,   210,   210,   121,   236,   123,   247,
     260,   262,   213,   214,   223,   224,   264,   217,   122,    82,
     129,   127,     5,     6,     7,   160,   161,   277,   124,   127,
     124,   127,   125,   129,   130,   166,   125,   226,   227,   223,
      82,   127,   129,   183,   190,   264,   190,   145,   123,   123,
     123,   125,   141,   127,   125,   242,   304,   129,   123,   123,
     310,   312,   129,   283,   316,   318,   320,   322,   317,   319,
     321,   323,   324,   325,   264,   141,   141,   122,    16,    16,
       9,    10,    11,    12,    13,    14,    15,    16,    50,   122,
     124,   131,   132,   290,   295,   301,   302,   248,   140,   226,
     275,   122,   128,   220,   219,   133,   216,   276,   155,   277,
     277,   277,   129,   156,   153,   156,   166,     9,    11,    12,
      13,    15,    16,    50,   122,   131,   132,   133,   168,   169,
     170,   174,   264,   272,   278,   125,   123,    53,    81,   228,
     230,   231,   133,   216,   186,   190,   191,   125,   267,   306,
     283,   283,   314,   123,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   129,   292,   207,   126,   298,   291,
     297,   296,    10,   122,   251,   258,   261,   123,   127,   225,
     222,   264,   226,   223,   275,   159,   160,   125,   157,   159,
     125,   173,   131,   132,   134,   135,   136,   137,   138,   139,
     175,   171,   202,   129,   267,   223,   185,   234,   123,   267,
     311,   123,   283,   123,   123,   123,   123,   123,   123,   123,
     123,   129,   129,   123,   226,   299,   295,   290,     9,   302,
     302,   252,   170,   263,   122,   226,   129,   221,   122,   123,
     124,     3,     4,     5,     6,     7,     8,     9,    30,    31,
      34,    42,    43,    50,    54,    85,    86,   120,   121,   133,
     158,   162,   163,   176,   177,   178,   179,   181,   183,   193,
     197,   199,   216,   237,   265,   303,   328,   124,   170,   172,
     170,   210,   229,    83,    84,   122,   245,   249,   255,   256,
     307,   129,   315,    11,    11,   123,   268,   123,   293,   226,
     141,   215,   123,   222,   329,   156,   121,   124,   128,   180,
     264,   264,   133,   265,   267,   179,   197,   199,   265,   133,
     216,   122,   122,   223,   128,   239,   239,   179,   197,   199,
     211,   211,   211,   208,   156,   123,   170,   228,   232,   245,
     253,   280,   257,   123,   330,   123,   123,   123,   127,   129,
     292,   123,   226,   221,   123,   125,   329,   330,   180,   180,
     223,   208,   211,   211,   223,     8,   197,   265,   329,   121,
     121,   211,   211,   125,   234,   123,   245,   123,   122,   294,
     125,   123,   125,   121,   123,   123,   246,   300,   290,   330,
     121,   251,   302,   121,   123
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
#line 950 "vtkParse.y"
    { startSig(); clearTypeId(); clearTemplate(); }
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 964 "vtkParse.y"
    { output_function(); }
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 965 "vtkParse.y"
    { output_function(); }
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 966 "vtkParse.y"
    { reject_function(); }
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 967 "vtkParse.y"
    { output_function(); }
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 968 "vtkParse.y"
    { reject_function(); }
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 969 "vtkParse.y"
    { output_function(); }
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 970 "vtkParse.y"
    { legacySig(); output_function(); }
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 988 "vtkParse.y"
    { pushNamespace((yyvsp[(2) - (2)].str)); }
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 989 "vtkParse.y"
    { popNamespace(); }
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 996 "vtkParse.y"
    { start_class((yyvsp[(2) - (2)].str), 0); }
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 997 "vtkParse.y"
    { end_class(); }
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 998 "vtkParse.y"
    { reject_class((yyvsp[(2) - (5)].str), 0); }
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 999 "vtkParse.y"
    { end_class(); }
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 1000 "vtkParse.y"
    { start_class((yyvsp[(2) - (2)].str), 1); }
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 1001 "vtkParse.y"
    { end_class(); }
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 1002 "vtkParse.y"
    { reject_class((yyvsp[(2) - (5)].str), 1); }
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 1003 "vtkParse.y"
    { end_class(); }
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 1008 "vtkParse.y"
    { startSig(); clearTypeId(); clearTemplate(); }
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 1021 "vtkParse.y"
    { output_function(); }
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 1022 "vtkParse.y"
    { ClassInfo *tmpc = currentClass;
     currentClass = NULL; output_function(); currentClass = tmpc; }
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 1024 "vtkParse.y"
    { output_function(); }
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 1025 "vtkParse.y"
    { output_function(); }
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 1026 "vtkParse.y"
    { ClassInfo *tmpc = currentClass;
     currentClass = NULL; output_function(); currentClass = tmpc; }
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 1028 "vtkParse.y"
    { output_function(); }
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 1029 "vtkParse.y"
    { legacySig(); output_function(); }
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 1043 "vtkParse.y"
    {
      vtkParse_AddPointerToArray(&currentClass->SuperClasses,
                                 &currentClass->NumberOfSuperClasses,
                                 vtkstrdup((yyvsp[(2) - (2)].str)));
    }
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 1049 "vtkParse.y"
    {access_level = VTK_ACCESS_PUBLIC;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 1050 "vtkParse.y"
    {access_level = VTK_ACCESS_PROTECTED;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 1051 "vtkParse.y"
    {access_level = VTK_ACCESS_PRIVATE;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 1061 "vtkParse.y"
    {start_enum((yyvsp[(2) - (2)].str));}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 1062 "vtkParse.y"
    {end_enum();}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 1063 "vtkParse.y"
    {start_enum(NULL);}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 1064 "vtkParse.y"
    {end_enum();}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 1068 "vtkParse.y"
    {add_enum((yyvsp[(1) - (1)].str), NULL);}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 1069 "vtkParse.y"
    {add_enum((yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 1071 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 1076 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 1077 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 1078 "vtkParse.y"
    {
         (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (3)].str)) + strlen((yyvsp[(3) - (3)].str)) + 1);
         sprintf((yyval.str), "%s%s", (yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));
       }
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 1082 "vtkParse.y"
    {postSig((yyvsp[(2) - (2)].str));}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 1083 "vtkParse.y"
    {
         (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (4)].str)) + strlen((yyvsp[(2) - (4)].str)) +
                                  strlen((yyvsp[(4) - (4)].str)) + 3);
         sprintf((yyval.str), "%s %s %s", (yyvsp[(1) - (4)].str), (yyvsp[(2) - (4)].str), (yyvsp[(4) - (4)].str));
       }
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 1088 "vtkParse.y"
    {postSig("(");}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 1089 "vtkParse.y"
    {
         postSig(")");
         (yyval.str) = (char *)malloc(strlen((yyvsp[(3) - (4)].str)) + 3);
         sprintf((yyval.str), "(%s)", (yyvsp[(3) - (4)].str));
       }
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 1095 "vtkParse.y"
    { (yyval.str) = "-"; }
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 1095 "vtkParse.y"
    { (yyval.str) = "+"; }
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 1096 "vtkParse.y"
    { (yyval.str) = "~"; }
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 1098 "vtkParse.y"
    { (yyval.str) = "-"; }
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 1098 "vtkParse.y"
    { (yyval.str) = "+"; }
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 1099 "vtkParse.y"
    { (yyval.str) = "*"; }
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 1099 "vtkParse.y"
    { (yyval.str) = "/"; }
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 1100 "vtkParse.y"
    { (yyval.str) = "%"; }
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 1100 "vtkParse.y"
    { (yyval.str) = "&"; }
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 1101 "vtkParse.y"
    { (yyval.str) = "|"; }
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 1101 "vtkParse.y"
    { (yyval.str) = "^"; }
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 1127 "vtkParse.y"
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
#line 1149 "vtkParse.y"
    { }
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 1150 "vtkParse.y"
    { }
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 1151 "vtkParse.y"
    { }
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1152 "vtkParse.y"
    { }
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1154 "vtkParse.y"
    { }
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1160 "vtkParse.y"
    { postSig("template<> "); clearTypeId(); }
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1161 "vtkParse.y"
    { postSig("template<");
          clearTypeId(); startTemplate(); }
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1163 "vtkParse.y"
    { postSig("> "); clearTypeId(); }
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1166 "vtkParse.y"
    { postSig(", "); clearTypeId(); }
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1170 "vtkParse.y"
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
#line 1180 "vtkParse.y"
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
#line 1187 "vtkParse.y"
    { pushTemplate(); }
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1188 "vtkParse.y"
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
#line 1199 "vtkParse.y"
    {postSig("class ");}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1200 "vtkParse.y"
    {postSig("typename ");}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 1202 "vtkParse.y"
    { setVarName((yyvsp[(1) - (1)].str)); }
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1232 "vtkParse.y"
    {openSig(); preSig("~"); closeSig();}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 1233 "vtkParse.y"
    {openSig(); preSig("~"); closeSig();}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 1235 "vtkParse.y"
    {
         openSig(); preSig("virtual ~"); closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1243 "vtkParse.y"
    {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1257 "vtkParse.y"
    {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 1266 "vtkParse.y"
    {
      postSig("(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 1270 "vtkParse.y"
    { postSig(")"); }
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1271 "vtkParse.y"
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
#line 1285 "vtkParse.y"
    { postSig(")"); }
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1286 "vtkParse.y"
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
#line 1297 "vtkParse.y"
    {postSig((yyvsp[(2) - (2)].str));}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 1298 "vtkParse.y"
    {
      postSig("(");
      currentFunction->IsOperator = 1;
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 1303 "vtkParse.y"
    { (yyval.str) = (yyvsp[(2) - (7)].str); }
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 1305 "vtkParse.y"
    { postSig(")"); }
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1306 "vtkParse.y"
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
#line 1319 "vtkParse.y"
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
#line 1328 "vtkParse.y"
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
#line 1338 "vtkParse.y"
    {
      postSig(" const");
      currentFunction->IsConst = 1;
    }
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 1346 "vtkParse.y"
    {
      postSig("(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 1349 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (5)].str); }
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 1350 "vtkParse.y"
    {markSig(); postSig("<");}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 1351 "vtkParse.y"
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
#line 1358 "vtkParse.y"
    { (yyval.str) = (yyvsp[(7) - (9)].str); }
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 1360 "vtkParse.y"
    { postSig(");"); closeSig(); }
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 1361 "vtkParse.y"
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
#line 1370 "vtkParse.y"
    { postSig("("); }
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 1379 "vtkParse.y"
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
#line 1392 "vtkParse.y"
    { postSig("(");}
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 1398 "vtkParse.y"
    {clearTypeId();}
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 1401 "vtkParse.y"
    { currentFunction->IsVariadic = 1; postSig("..."); }
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 1402 "vtkParse.y"
    { clearTypeId(); }
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 1403 "vtkParse.y"
    { clearTypeId(); postSig(", "); }
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 1406 "vtkParse.y"
    { markSig(); }
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 1408 "vtkParse.y"
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
#line 1430 "vtkParse.y"
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
#line 1438 "vtkParse.y"
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
#line 1463 "vtkParse.y"
    {clearVarValue();}
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 1465 "vtkParse.y"
    { postSig("="); clearVarValue();}
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 1466 "vtkParse.y"
    { setVarValue((yyvsp[(3) - (3)].str)); }
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 1477 "vtkParse.y"
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
#line 1491 "vtkParse.y"
    {postSig(", ");}
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 1494 "vtkParse.y"
    { setStorageTypeIndirection(0); }
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 1495 "vtkParse.y"
    { setStorageTypeIndirection((yyvsp[(1) - (1)].integer)); }
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 1499 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 1500 "vtkParse.y"
    { postSig(")"); }
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 1502 "vtkParse.y"
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
#line 1511 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 1512 "vtkParse.y"
    { postSig(")"); }
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 1514 "vtkParse.y"
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
#line 1522 "vtkParse.y"
    { postSig("("); (yyval.integer) = 0; }
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 1523 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("*");
               (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 1525 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("&");
               (yyval.integer) = VTK_PARSE_REF; }
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 1528 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("*");
               (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 1530 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("&");
               (yyval.integer) = VTK_PARSE_REF; }
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 1533 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 1534 "vtkParse.y"
    { pushFunction(); postSig("("); }
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 1535 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_FUNCTION; postSig(")"); popFunction(); }
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 1536 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_ARRAY; }
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 1539 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 1541 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(1) - (2)].integer), (yyvsp[(2) - (2)].integer));}
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 1544 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 1546 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(1) - (2)].integer), (yyvsp[(2) - (2)].integer));}
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 1548 "vtkParse.y"
    {clearVarName(); chopSig();}
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 1550 "vtkParse.y"
    {setVarName((yyvsp[(1) - (1)].str));}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 1552 "vtkParse.y"
    {clearArray();}
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 1554 "vtkParse.y"
    {clearArray();}
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 1556 "vtkParse.y"
    {postSig("[");}
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 1556 "vtkParse.y"
    {postSig("]");}
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 1560 "vtkParse.y"
    {pushArraySize("");}
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 1561 "vtkParse.y"
    {pushArraySize((yyvsp[(1) - (1)].str));}
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 1567 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 1568 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 1569 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 1570 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 1571 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 1572 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 1579 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer); setStorageType((yyval.integer));}
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 1580 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 1581 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 1583 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(3) - (3)].integer); setStorageType((yyval.integer));}
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 1584 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 1585 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer));
      setStorageType((yyval.integer));}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 1587 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(2) - (3)].integer) | (yyvsp[(3) - (3)].integer));
      setStorageType((yyval.integer));}
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 1591 "vtkParse.y"
    {postSig("static "); (yyval.integer) = VTK_PARSE_STATIC; }
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 1592 "vtkParse.y"
    {postSig("static "); (yyval.integer) = VTK_PARSE_STATIC; }
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 1594 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 1595 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer));}
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 1597 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 1598 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_CONST | (yyvsp[(2) - (2)].integer));}
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 1599 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_CONST | (yyvsp[(1) - (2)].integer));}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 1601 "vtkParse.y"
    {postSig("const ");}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 1605 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 1607 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 1608 "vtkParse.y"
    {postSig("typename ");}
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 1609 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(3) - (3)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 1612 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 1613 "vtkParse.y"
    {chopSig(); postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 1614 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 1615 "vtkParse.y"
    {chopSig(); postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 295:

/* Line 1455 of yacc.c  */
#line 1617 "vtkParse.y"
    {postSig(", ");}
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 1619 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 1620 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 1621 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 1622 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 1623 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 302:

/* Line 1455 of yacc.c  */
#line 1624 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 305:

/* Line 1455 of yacc.c  */
#line 1629 "vtkParse.y"
    {
             (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (3)].str))+strlen((yyvsp[(3) - (3)].str))+3);
             sprintf((yyval.str), "%s::%s", (yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));
             preScopeSig((yyvsp[(1) - (3)].str));
           }
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 1635 "vtkParse.y"
    {
             (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (3)].str))+strlen((yyvsp[(3) - (3)].str))+3);
             sprintf((yyval.str), "%s::%s", (yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));
             preScopeSig("");
           }
    break;

  case 313:

/* Line 1455 of yacc.c  */
#line 1657 "vtkParse.y"
    { postSig("&"); (yyval.integer) = VTK_PARSE_REF;}
    break;

  case 314:

/* Line 1455 of yacc.c  */
#line 1658 "vtkParse.y"
    { postSig("&"); (yyval.integer) = ((yyvsp[(1) - (2)].integer) | VTK_PARSE_REF);}
    break;

  case 315:

/* Line 1455 of yacc.c  */
#line 1659 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 316:

/* Line 1455 of yacc.c  */
#line 1664 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 317:

/* Line 1455 of yacc.c  */
#line 1666 "vtkParse.y"
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
#line 1677 "vtkParse.y"
    { postSig("*"); (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 1678 "vtkParse.y"
    { postSig("*const "); (yyval.integer) = VTK_PARSE_CONST_POINTER; }
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 1681 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 321:

/* Line 1455 of yacc.c  */
#line 1682 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); }
    break;

  case 322:

/* Line 1455 of yacc.c  */
#line 1683 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); }
    break;

  case 323:

/* Line 1455 of yacc.c  */
#line 1684 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 324:

/* Line 1455 of yacc.c  */
#line 1685 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 325:

/* Line 1455 of yacc.c  */
#line 1686 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 326:

/* Line 1455 of yacc.c  */
#line 1687 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 1690 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 1691 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 329:

/* Line 1455 of yacc.c  */
#line 1694 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_STRING;}
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 1695 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNICODE_STRING;}
    break;

  case 331:

/* Line 1455 of yacc.c  */
#line 1696 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_OSTREAM; }
    break;

  case 332:

/* Line 1455 of yacc.c  */
#line 1697 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_ISTREAM; }
    break;

  case 333:

/* Line 1455 of yacc.c  */
#line 1698 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 1699 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_OBJECT; }
    break;

  case 335:

/* Line 1455 of yacc.c  */
#line 1702 "vtkParse.y"
    { typeSig("void"); (yyval.integer) = VTK_PARSE_VOID;}
    break;

  case 336:

/* Line 1455 of yacc.c  */
#line 1703 "vtkParse.y"
    { typeSig("float"); (yyval.integer) = VTK_PARSE_FLOAT;}
    break;

  case 337:

/* Line 1455 of yacc.c  */
#line 1704 "vtkParse.y"
    { typeSig("double"); (yyval.integer) = VTK_PARSE_DOUBLE;}
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 1705 "vtkParse.y"
    { typeSig("bool"); (yyval.integer) = VTK_PARSE_BOOL;}
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 1706 "vtkParse.y"
    { typeSig("ssize_t"); (yyval.integer) = VTK_PARSE_SSIZE_T;}
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 1707 "vtkParse.y"
    { typeSig("size_t"); (yyval.integer) = VTK_PARSE_SIZE_T;}
    break;

  case 341:

/* Line 1455 of yacc.c  */
#line 1708 "vtkParse.y"
    {typeSig("signed char"); (yyval.integer) = VTK_PARSE_SIGNED_CHAR;}
    break;

  case 342:

/* Line 1455 of yacc.c  */
#line 1709 "vtkParse.y"
    { typeSig("vtkTypeInt8"); (yyval.integer) = VTK_PARSE_INT8; }
    break;

  case 343:

/* Line 1455 of yacc.c  */
#line 1710 "vtkParse.y"
    { typeSig("vtkTypeUInt8"); (yyval.integer) = VTK_PARSE_UINT8; }
    break;

  case 344:

/* Line 1455 of yacc.c  */
#line 1711 "vtkParse.y"
    { typeSig("vtkTypeInt16"); (yyval.integer) = VTK_PARSE_INT16; }
    break;

  case 345:

/* Line 1455 of yacc.c  */
#line 1712 "vtkParse.y"
    { typeSig("vtkTypeUInt16"); (yyval.integer) = VTK_PARSE_UINT16; }
    break;

  case 346:

/* Line 1455 of yacc.c  */
#line 1713 "vtkParse.y"
    { typeSig("vtkTypeInt32"); (yyval.integer) = VTK_PARSE_INT32; }
    break;

  case 347:

/* Line 1455 of yacc.c  */
#line 1714 "vtkParse.y"
    { typeSig("vtkTypeUInt32"); (yyval.integer) = VTK_PARSE_UINT32; }
    break;

  case 348:

/* Line 1455 of yacc.c  */
#line 1715 "vtkParse.y"
    { typeSig("vtkTypeInt64"); (yyval.integer) = VTK_PARSE_INT64; }
    break;

  case 349:

/* Line 1455 of yacc.c  */
#line 1716 "vtkParse.y"
    { typeSig("vtkTypeUInt64"); (yyval.integer) = VTK_PARSE_UINT64; }
    break;

  case 350:

/* Line 1455 of yacc.c  */
#line 1717 "vtkParse.y"
    { typeSig("vtkTypeFloat32"); (yyval.integer) = VTK_PARSE_FLOAT32; }
    break;

  case 351:

/* Line 1455 of yacc.c  */
#line 1718 "vtkParse.y"
    { typeSig("vtkTypeFloat64"); (yyval.integer) = VTK_PARSE_FLOAT64; }
    break;

  case 352:

/* Line 1455 of yacc.c  */
#line 1719 "vtkParse.y"
    {typeSig("signed");}
    break;

  case 353:

/* Line 1455 of yacc.c  */
#line 1719 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(3) - (3)].integer);}
    break;

  case 354:

/* Line 1455 of yacc.c  */
#line 1720 "vtkParse.y"
    {typeSig("unsigned");}
    break;

  case 355:

/* Line 1455 of yacc.c  */
#line 1721 "vtkParse.y"
    { (yyval.integer) = (VTK_PARSE_UNSIGNED | (yyvsp[(3) - (3)].integer));}
    break;

  case 356:

/* Line 1455 of yacc.c  */
#line 1722 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 357:

/* Line 1455 of yacc.c  */
#line 1725 "vtkParse.y"
    { typeSig("char"); (yyval.integer) = VTK_PARSE_CHAR;}
    break;

  case 358:

/* Line 1455 of yacc.c  */
#line 1726 "vtkParse.y"
    { typeSig("int"); (yyval.integer) = VTK_PARSE_INT;}
    break;

  case 359:

/* Line 1455 of yacc.c  */
#line 1727 "vtkParse.y"
    { typeSig("short"); (yyval.integer) = VTK_PARSE_SHORT;}
    break;

  case 360:

/* Line 1455 of yacc.c  */
#line 1728 "vtkParse.y"
    { typeSig("long"); (yyval.integer) = VTK_PARSE_LONG;}
    break;

  case 361:

/* Line 1455 of yacc.c  */
#line 1729 "vtkParse.y"
    { typeSig("vtkIdType"); (yyval.integer) = VTK_PARSE_ID_TYPE;}
    break;

  case 362:

/* Line 1455 of yacc.c  */
#line 1730 "vtkParse.y"
    { typeSig("long long"); (yyval.integer) = VTK_PARSE_LONG_LONG;}
    break;

  case 363:

/* Line 1455 of yacc.c  */
#line 1731 "vtkParse.y"
    { typeSig("__int64"); (yyval.integer) = VTK_PARSE___INT64;}
    break;

  case 364:

/* Line 1455 of yacc.c  */
#line 1737 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 365:

/* Line 1455 of yacc.c  */
#line 1738 "vtkParse.y"
    { postSig("{ "); }
    break;

  case 366:

/* Line 1455 of yacc.c  */
#line 1739 "vtkParse.y"
    {
          char *cp;
          postSig("}");
          cp = (char *)malloc(strlen((yyvsp[(3) - (6)].str)) + strlen((yyvsp[(4) - (6)].str)) + 5);
          sprintf(cp, "{ %s%s }", (yyvsp[(3) - (6)].str), (yyvsp[(4) - (6)].str));
          (yyval.str) = cp;
        }
    break;

  case 369:

/* Line 1455 of yacc.c  */
#line 1749 "vtkParse.y"
    {(yyval.str) = vtkstrdup("");}
    break;

  case 370:

/* Line 1455 of yacc.c  */
#line 1750 "vtkParse.y"
    { postSig(", "); }
    break;

  case 371:

/* Line 1455 of yacc.c  */
#line 1751 "vtkParse.y"
    {
          char *cp;
          cp = (char *)malloc(strlen((yyvsp[(1) - (4)].str)) + strlen((yyvsp[(4) - (4)].str)) + 3);
          sprintf(cp, "%s, %s", (yyvsp[(1) - (4)].str), (yyvsp[(4) - (4)].str));
          (yyval.str) = cp;
        }
    break;

  case 372:

/* Line 1455 of yacc.c  */
#line 1758 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 373:

/* Line 1455 of yacc.c  */
#line 1759 "vtkParse.y"
    {postSig("+");}
    break;

  case 374:

/* Line 1455 of yacc.c  */
#line 1759 "vtkParse.y"
    {(yyval.str) = (yyvsp[(3) - (3)].str);}
    break;

  case 375:

/* Line 1455 of yacc.c  */
#line 1760 "vtkParse.y"
    {postSig("-");}
    break;

  case 376:

/* Line 1455 of yacc.c  */
#line 1760 "vtkParse.y"
    {
             (yyval.str) = (char *)malloc(strlen((yyvsp[(3) - (3)].str))+2);
             sprintf((yyval.str), "-%s", (yyvsp[(3) - (3)].str)); }
    break;

  case 377:

/* Line 1455 of yacc.c  */
#line 1763 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 378:

/* Line 1455 of yacc.c  */
#line 1764 "vtkParse.y"
    {postSig("(");}
    break;

  case 379:

/* Line 1455 of yacc.c  */
#line 1764 "vtkParse.y"
    {postSig(")"); (yyval.str) = (yyvsp[(3) - (4)].str);}
    break;

  case 380:

/* Line 1455 of yacc.c  */
#line 1765 "vtkParse.y"
    {postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 381:

/* Line 1455 of yacc.c  */
#line 1766 "vtkParse.y"
    {postSig("<(");}
    break;

  case 382:

/* Line 1455 of yacc.c  */
#line 1767 "vtkParse.y"
    {
            postSig(")");
            (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (9)].str)) + strlen(getTypeId()) +
                                     strlen((yyvsp[(8) - (9)].str)) + 5);
            sprintf((yyval.str), "%s<%s>(%s)", (yyvsp[(1) - (9)].str), getTypeId(), (yyvsp[(8) - (9)].str));
            }
    break;

  case 383:

/* Line 1455 of yacc.c  */
#line 1774 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 384:

/* Line 1455 of yacc.c  */
#line 1775 "vtkParse.y"
    {
                size_t i = strlen((yyvsp[(1) - (2)].str));
                char *cp = (char *)malloc(i + strlen((yyvsp[(2) - (2)].str)) + 1);
                strcpy(cp, (yyvsp[(1) - (2)].str));
                strcpy(&cp[i], (yyvsp[(2) - (2)].str));
                (yyval.str) = cp; }
    break;

  case 385:

/* Line 1455 of yacc.c  */
#line 1782 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 386:

/* Line 1455 of yacc.c  */
#line 1783 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 387:

/* Line 1455 of yacc.c  */
#line 1784 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 388:

/* Line 1455 of yacc.c  */
#line 1785 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 389:

/* Line 1455 of yacc.c  */
#line 1786 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 390:

/* Line 1455 of yacc.c  */
#line 1787 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 391:

/* Line 1455 of yacc.c  */
#line 1788 "vtkParse.y"
    {(yyval.str) = vtkstrdup(add_const_scope((yyvsp[(1) - (1)].str)));
                postSig((yyvsp[(1) - (1)].str));}
    break;

  case 392:

/* Line 1455 of yacc.c  */
#line 1790 "vtkParse.y"
    {(yyval.str) = vtkstrdup(add_const_scope((yyvsp[(1) - (1)].str)));
                postSig((yyvsp[(1) - (1)].str));}
    break;

  case 393:

/* Line 1455 of yacc.c  */
#line 1800 "vtkParse.y"
    {preSig("void Set"); postSig("(");}
    break;

  case 394:

/* Line 1455 of yacc.c  */
#line 1801 "vtkParse.y"
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

  case 395:

/* Line 1455 of yacc.c  */
#line 1813 "vtkParse.y"
    {postSig("Get");}
    break;

  case 396:

/* Line 1455 of yacc.c  */
#line 1814 "vtkParse.y"
    {markSig();}
    break;

  case 397:

/* Line 1455 of yacc.c  */
#line 1814 "vtkParse.y"
    {swapSig();}
    break;

  case 398:

/* Line 1455 of yacc.c  */
#line 1815 "vtkParse.y"
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

  case 399:

/* Line 1455 of yacc.c  */
#line 1826 "vtkParse.y"
    {preSig("void Set");}
    break;

  case 400:

/* Line 1455 of yacc.c  */
#line 1827 "vtkParse.y"
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

  case 401:

/* Line 1455 of yacc.c  */
#line 1839 "vtkParse.y"
    {preSig("char *Get");}
    break;

  case 402:

/* Line 1455 of yacc.c  */
#line 1840 "vtkParse.y"
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

  case 403:

/* Line 1455 of yacc.c  */
#line 1851 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 404:

/* Line 1455 of yacc.c  */
#line 1851 "vtkParse.y"
    {closeSig();}
    break;

  case 405:

/* Line 1455 of yacc.c  */
#line 1853 "vtkParse.y"
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

  case 406:

/* Line 1455 of yacc.c  */
#line 1894 "vtkParse.y"
    {preSig("void Set"); postSig("("); }
    break;

  case 407:

/* Line 1455 of yacc.c  */
#line 1895 "vtkParse.y"
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

  case 408:

/* Line 1455 of yacc.c  */
#line 1907 "vtkParse.y"
    {postSig("*Get");}
    break;

  case 409:

/* Line 1455 of yacc.c  */
#line 1908 "vtkParse.y"
    {markSig();}
    break;

  case 410:

/* Line 1455 of yacc.c  */
#line 1908 "vtkParse.y"
    {swapSig();}
    break;

  case 411:

/* Line 1455 of yacc.c  */
#line 1909 "vtkParse.y"
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

  case 412:

/* Line 1455 of yacc.c  */
#line 1921 "vtkParse.y"
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

  case 413:

/* Line 1455 of yacc.c  */
#line 1948 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 414:

/* Line 1455 of yacc.c  */
#line 1949 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 2);
   }
    break;

  case 415:

/* Line 1455 of yacc.c  */
#line 1953 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 416:

/* Line 1455 of yacc.c  */
#line 1954 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 2);
   }
    break;

  case 417:

/* Line 1455 of yacc.c  */
#line 1958 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 418:

/* Line 1455 of yacc.c  */
#line 1959 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 3);
   }
    break;

  case 419:

/* Line 1455 of yacc.c  */
#line 1963 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 420:

/* Line 1455 of yacc.c  */
#line 1964 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 3);
   }
    break;

  case 421:

/* Line 1455 of yacc.c  */
#line 1968 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 422:

/* Line 1455 of yacc.c  */
#line 1969 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 4);
   }
    break;

  case 423:

/* Line 1455 of yacc.c  */
#line 1973 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 424:

/* Line 1455 of yacc.c  */
#line 1974 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 4);
   }
    break;

  case 425:

/* Line 1455 of yacc.c  */
#line 1978 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 426:

/* Line 1455 of yacc.c  */
#line 1979 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 6);
   }
    break;

  case 427:

/* Line 1455 of yacc.c  */
#line 1983 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 428:

/* Line 1455 of yacc.c  */
#line 1984 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 6);
   }
    break;

  case 429:

/* Line 1455 of yacc.c  */
#line 1988 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 430:

/* Line 1455 of yacc.c  */
#line 1990 "vtkParse.y"
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

  case 431:

/* Line 1455 of yacc.c  */
#line 2008 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 432:

/* Line 1455 of yacc.c  */
#line 2010 "vtkParse.y"
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

  case 433:

/* Line 1455 of yacc.c  */
#line 2027 "vtkParse.y"
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

  case 434:

/* Line 1455 of yacc.c  */
#line 2081 "vtkParse.y"
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

  case 435:

/* Line 1455 of yacc.c  */
#line 2137 "vtkParse.y"
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

  case 436:

/* Line 1455 of yacc.c  */
#line 2213 "vtkParse.y"
    { (yyval.str) = "operator()"; }
    break;

  case 437:

/* Line 1455 of yacc.c  */
#line 2214 "vtkParse.y"
    { (yyval.str) = "operator[]"; }
    break;

  case 438:

/* Line 1455 of yacc.c  */
#line 2215 "vtkParse.y"
    { (yyval.str) = "operator new[]"; }
    break;

  case 439:

/* Line 1455 of yacc.c  */
#line 2216 "vtkParse.y"
    { (yyval.str) = "operator delete[]"; }
    break;

  case 441:

/* Line 1455 of yacc.c  */
#line 2219 "vtkParse.y"
    { (yyval.str) = "operator="; }
    break;

  case 442:

/* Line 1455 of yacc.c  */
#line 2220 "vtkParse.y"
    { (yyval.str) = "operator*"; }
    break;

  case 443:

/* Line 1455 of yacc.c  */
#line 2220 "vtkParse.y"
    { (yyval.str) = "operator/"; }
    break;

  case 444:

/* Line 1455 of yacc.c  */
#line 2221 "vtkParse.y"
    { (yyval.str) = "operator-"; }
    break;

  case 445:

/* Line 1455 of yacc.c  */
#line 2221 "vtkParse.y"
    { (yyval.str) = "operator+"; }
    break;

  case 446:

/* Line 1455 of yacc.c  */
#line 2222 "vtkParse.y"
    { (yyval.str) = "operator!"; }
    break;

  case 447:

/* Line 1455 of yacc.c  */
#line 2222 "vtkParse.y"
    { (yyval.str) = "operator~"; }
    break;

  case 448:

/* Line 1455 of yacc.c  */
#line 2223 "vtkParse.y"
    { (yyval.str) = "operator,"; }
    break;

  case 449:

/* Line 1455 of yacc.c  */
#line 2223 "vtkParse.y"
    { (yyval.str) = "operator<"; }
    break;

  case 450:

/* Line 1455 of yacc.c  */
#line 2224 "vtkParse.y"
    { (yyval.str) = "operator>"; }
    break;

  case 451:

/* Line 1455 of yacc.c  */
#line 2224 "vtkParse.y"
    { (yyval.str) = "operator&"; }
    break;

  case 452:

/* Line 1455 of yacc.c  */
#line 2225 "vtkParse.y"
    { (yyval.str) = "operator|"; }
    break;

  case 453:

/* Line 1455 of yacc.c  */
#line 2225 "vtkParse.y"
    { (yyval.str) = "operator^"; }
    break;

  case 454:

/* Line 1455 of yacc.c  */
#line 2226 "vtkParse.y"
    { (yyval.str) = "operator%"; }
    break;

  case 455:

/* Line 1455 of yacc.c  */
#line 2227 "vtkParse.y"
    { (yyval.str) = "operator new"; }
    break;

  case 456:

/* Line 1455 of yacc.c  */
#line 2228 "vtkParse.y"
    { (yyval.str) = "operator delete"; }
    break;

  case 457:

/* Line 1455 of yacc.c  */
#line 2229 "vtkParse.y"
    { (yyval.str) = "operator<<="; }
    break;

  case 458:

/* Line 1455 of yacc.c  */
#line 2230 "vtkParse.y"
    { (yyval.str) = "operator>>="; }
    break;

  case 459:

/* Line 1455 of yacc.c  */
#line 2231 "vtkParse.y"
    { (yyval.str) = "operator<<"; }
    break;

  case 460:

/* Line 1455 of yacc.c  */
#line 2232 "vtkParse.y"
    { (yyval.str) = "operator>>"; }
    break;

  case 461:

/* Line 1455 of yacc.c  */
#line 2233 "vtkParse.y"
    { (yyval.str) = "operator->*"; }
    break;

  case 462:

/* Line 1455 of yacc.c  */
#line 2234 "vtkParse.y"
    { (yyval.str) = "operator->"; }
    break;

  case 463:

/* Line 1455 of yacc.c  */
#line 2235 "vtkParse.y"
    { (yyval.str) = "operator+="; }
    break;

  case 464:

/* Line 1455 of yacc.c  */
#line 2236 "vtkParse.y"
    { (yyval.str) = "operator-="; }
    break;

  case 465:

/* Line 1455 of yacc.c  */
#line 2237 "vtkParse.y"
    { (yyval.str) = "operator*="; }
    break;

  case 466:

/* Line 1455 of yacc.c  */
#line 2238 "vtkParse.y"
    { (yyval.str) = "operator/="; }
    break;

  case 467:

/* Line 1455 of yacc.c  */
#line 2239 "vtkParse.y"
    { (yyval.str) = "operator%="; }
    break;

  case 468:

/* Line 1455 of yacc.c  */
#line 2240 "vtkParse.y"
    { (yyval.str) = "operator++"; }
    break;

  case 469:

/* Line 1455 of yacc.c  */
#line 2241 "vtkParse.y"
    { (yyval.str) = "operator--"; }
    break;

  case 470:

/* Line 1455 of yacc.c  */
#line 2242 "vtkParse.y"
    { (yyval.str) = "operator&="; }
    break;

  case 471:

/* Line 1455 of yacc.c  */
#line 2243 "vtkParse.y"
    { (yyval.str) = "operator|="; }
    break;

  case 472:

/* Line 1455 of yacc.c  */
#line 2244 "vtkParse.y"
    { (yyval.str) = "operator^="; }
    break;

  case 473:

/* Line 1455 of yacc.c  */
#line 2245 "vtkParse.y"
    {(yyval.str) = "operator&&=";}
    break;

  case 474:

/* Line 1455 of yacc.c  */
#line 2246 "vtkParse.y"
    {(yyval.str) = "operator||=";}
    break;

  case 475:

/* Line 1455 of yacc.c  */
#line 2247 "vtkParse.y"
    { (yyval.str) = "operator&&"; }
    break;

  case 476:

/* Line 1455 of yacc.c  */
#line 2248 "vtkParse.y"
    { (yyval.str) = "operator||"; }
    break;

  case 477:

/* Line 1455 of yacc.c  */
#line 2249 "vtkParse.y"
    { (yyval.str) = "operator=="; }
    break;

  case 478:

/* Line 1455 of yacc.c  */
#line 2250 "vtkParse.y"
    { (yyval.str) = "operator!="; }
    break;

  case 479:

/* Line 1455 of yacc.c  */
#line 2251 "vtkParse.y"
    { (yyval.str) = "operator<="; }
    break;

  case 480:

/* Line 1455 of yacc.c  */
#line 2252 "vtkParse.y"
    { (yyval.str) = "operator>="; }
    break;

  case 481:

/* Line 1455 of yacc.c  */
#line 2259 "vtkParse.y"
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
#line 7212 "vtkParse.tab.c"
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
#line 2299 "vtkParse.y"

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
