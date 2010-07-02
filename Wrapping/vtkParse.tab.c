
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

static int vtkParseTypeMap[] =
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

NamespaceInfo *currentNamespace = NULL;
ClassInfo *currentClass = NULL;
FunctionInfo *currentFunction = NULL;
TemplateArgs *currentTemplate = NULL;

char *currentEnumName = 0;
char *currentEnumValue = 0;

int parseDebug;
char temps[2048];
parse_access_t access_level = VTK_ACCESS_PUBLIC;
int  is_concrete;
int  HaveComment;
char CommentText[50000];
int CommentState;

/* helper functions */
void start_class(const char *classname, int is_struct);
void reject_class(const char *classname, int is_struct);
void end_class();
void output_function(void);
void reject_function(void);
void set_return(FunctionInfo *func, int type,
                const char *typeclass, int count);
void add_argument(FunctionInfo *func, int type,
                  const char *classname, int count);
void start_enum(const char *enumname);
void add_enum(const char *name, const char *value);
void end_enum();
void add_constant(const char *name, const char *value,
                  int type, const char *typeclass, int global);
const char *add_const_scope(const char *name);
void prepend_scope(char *cp, const char *arg);
int add_indirection(int tval, int ptr);
void handle_complex_type(ValueInfo *val, int datatype,
                         int extra, const char *funcSig);
void handle_function_type(ValueInfo *arg, const char *name,
                          const char *funcSig);

void outputSetVectorMacro(
  const char *var, int argType, const char *typeText, int n);
void outputGetVectorMacro(
  const char *var, int argType, const char *typeText, int n);

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
size_t sigMarkDepth = 0;
unsigned int sigAllocatedLength;

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
int storageType = 0;

/* save the storage type */
void setStorageType(int val)
{
  storageType = val;
}

/* modify the indirection (pointers, refs) in the storage type */
void setStorageTypeIndirection(int ind)
{
  storageType = (storageType & ~VTK_PARSE_INDIRECT);
  ind = (ind & VTK_PARSE_INDIRECT);
  storageType = (storageType | ind);
}

/* retrieve the storage type */
int getStorageType()
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
  int n;

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
    }
  newFunction->Signature = NULL;
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
  size_t i, m, n, depth;

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
int add_indirection(int type1, int type2)
{
  int ptr1 = (type1 & VTK_PARSE_POINTER_MASK);
  int ptr2 = (type2 & VTK_PARSE_POINTER_MASK);
  int reverse = 0;
  int result;

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



/* Line 189 of yacc.c  */
#line 825 "vtkParse.tab.c"

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

/* Line 214 of yacc.c  */
#line 772 "vtkParse.y"

  char *str;
  int   integer;



/* Line 214 of yacc.c  */
#line 1108 "vtkParse.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 1120 "vtkParse.tab.c"

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
#define YYLAST   6068

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  144
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  199
/* YYNRULES -- Number of rules.  */
#define YYNRULES  559
/* YYNRULES -- Number of states.  */
#define YYNSTATES  987

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
       0,     0,     3,     4,     5,     9,    11,    13,    15,    17,
      19,    21,    23,    25,    27,    29,    31,    34,    36,    39,
      43,    46,    49,    52,    56,    59,    61,    63,    66,    71,
      76,    78,    84,    85,    92,    97,    98,   106,   107,   118,
     119,   127,   128,   139,   140,   141,   145,   148,   150,   152,
     154,   156,   158,   160,   162,   164,   166,   169,   171,   173,
     176,   180,   184,   187,   191,   195,   198,   204,   206,   208,
     210,   211,   214,   216,   220,   222,   225,   228,   231,   233,
     235,   237,   238,   239,   249,   250,   251,   260,   261,   263,
     267,   269,   273,   275,   277,   279,   281,   283,   285,   287,
     289,   291,   293,   294,   298,   299,   304,   305,   310,   312,
     314,   316,   318,   320,   322,   324,   326,   328,   330,   332,
     339,   347,   354,   358,   361,   365,   369,   371,   376,   380,
     386,   395,   404,   412,   420,   430,   440,   443,   446,   449,
     452,   455,   459,   463,   464,   470,   472,   473,   478,   481,
     484,   485,   489,   491,   493,   494,   495,   499,   504,   509,
     512,   516,   521,   527,   531,   536,   543,   551,   557,   564,
     567,   571,   574,   578,   582,   584,   587,   590,   594,   598,
     602,   604,   607,   611,   612,   613,   622,   623,   627,   628,
     629,   637,   638,   642,   643,   646,   649,   651,   653,   657,
     658,   664,   665,   666,   676,   677,   681,   682,   688,   689,
     693,   694,   698,   703,   705,   706,   712,   713,   715,   717,
     719,   720,   725,   729,   731,   732,   733,   736,   738,   740,
     741,   746,   747,   748,   754,   756,   758,   761,   762,   764,
     765,   769,   774,   779,   783,   786,   787,   788,   793,   794,
     797,   798,   802,   805,   806,   812,   815,   816,   822,   824,
     826,   828,   830,   832,   833,   834,   839,   841,   843,   846,
     848,   851,   852,   854,   856,   857,   859,   860,   863,   864,
     870,   871,   873,   874,   876,   878,   880,   882,   884,   886,
     888,   890,   893,   896,   900,   903,   906,   910,   912,   915,
     917,   920,   922,   925,   928,   930,   932,   934,   936,   937,
     941,   942,   948,   949,   955,   957,   958,   963,   965,   967,
     969,   971,   973,   975,   977,   979,   983,   987,   989,   991,
     993,   995,   997,   999,  1001,  1004,  1006,  1008,  1011,  1013,
    1015,  1017,  1020,  1023,  1026,  1029,  1032,  1035,  1037,  1039,
    1041,  1043,  1045,  1047,  1049,  1051,  1053,  1055,  1057,  1059,
    1061,  1063,  1065,  1067,  1069,  1071,  1073,  1075,  1077,  1079,
    1081,  1083,  1085,  1086,  1090,  1091,  1095,  1097,  1099,  1101,
    1103,  1105,  1107,  1109,  1111,  1113,  1114,  1121,  1122,  1124,
    1125,  1126,  1131,  1133,  1134,  1138,  1139,  1143,  1145,  1146,
    1151,  1152,  1153,  1163,  1165,  1168,  1170,  1172,  1174,  1176,
    1178,  1180,  1182,  1184,  1185,  1193,  1194,  1195,  1196,  1206,
    1207,  1213,  1214,  1220,  1221,  1222,  1233,  1234,  1242,  1243,
    1244,  1245,  1255,  1262,  1263,  1271,  1272,  1280,  1281,  1289,
    1290,  1298,  1299,  1307,  1308,  1316,  1317,  1325,  1326,  1334,
    1335,  1345,  1346,  1356,  1361,  1366,  1373,  1381,  1384,  1387,
    1391,  1395,  1397,  1399,  1401,  1403,  1405,  1407,  1409,  1411,
    1413,  1415,  1417,  1419,  1421,  1423,  1425,  1427,  1429,  1431,
    1433,  1435,  1437,  1439,  1441,  1443,  1445,  1447,  1449,  1451,
    1453,  1455,  1457,  1459,  1461,  1463,  1465,  1467,  1469,  1471,
    1473,  1475,  1477,  1479,  1480,  1483,  1484,  1487,  1489,  1491,
    1493,  1495,  1497,  1499,  1501,  1503,  1505,  1507,  1509,  1511,
    1513,  1515,  1517,  1519,  1521,  1523,  1525,  1527,  1529,  1531,
    1533,  1535,  1537,  1539,  1541,  1543,  1545,  1547,  1549,  1551,
    1553,  1555,  1557,  1559,  1561,  1563,  1565,  1567,  1569,  1571,
    1573,  1575,  1577,  1579,  1581,  1583,  1587,  1591,  1595,  1599
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     145,     0,    -1,    -1,    -1,   145,   146,   147,    -1,   245,
      -1,   166,    -1,   163,    -1,   181,    -1,   180,    -1,   179,
      -1,   182,    -1,   149,    -1,   148,    -1,   186,    -1,   151,
      -1,   187,   151,    -1,    34,    -1,   203,   215,    -1,   187,
     203,   215,    -1,   202,   215,    -1,   198,   215,    -1,   199,
     215,    -1,   187,   198,   215,    -1,   196,   215,    -1,   310,
      -1,   335,    -1,   286,   121,    -1,     9,   122,   336,   123,
      -1,    50,   122,   336,   123,    -1,   121,    -1,    52,    10,
     124,   145,   125,    -1,    -1,    48,   286,   150,   124,   145,
     125,    -1,    48,   124,   336,   125,    -1,    -1,     4,   271,
     152,   159,   124,   156,   125,    -1,    -1,     4,   271,   126,
     282,   127,   153,   159,   124,   156,   125,    -1,    -1,     3,
     271,   154,   159,   124,   156,   125,    -1,    -1,     3,   271,
     126,   282,   127,   155,   159,   124,   156,   125,    -1,    -1,
      -1,   156,   157,   158,    -1,   162,   128,    -1,   245,    -1,
     166,    -1,   163,    -1,   181,    -1,   180,    -1,   179,    -1,
     182,    -1,   186,    -1,   184,    -1,    42,   184,    -1,   183,
      -1,    34,    -1,   203,   215,    -1,    42,   203,   215,    -1,
     187,   203,   215,    -1,   201,   215,    -1,    42,   201,   215,
      -1,   187,   201,   215,    -1,   197,   215,    -1,   120,   122,
     336,   123,   121,    -1,   310,    -1,   335,    -1,   121,    -1,
      -1,   128,   160,    -1,   161,    -1,   161,   129,   160,    -1,
     284,    -1,     6,   284,    -1,     7,   284,    -1,     5,   284,
      -1,     5,    -1,     6,    -1,     7,    -1,    -1,    -1,    32,
     271,   164,   124,   169,   125,   165,   337,   121,    -1,    -1,
      -1,    32,   167,   124,   169,   125,   168,   337,   121,    -1,
      -1,   170,    -1,   170,   129,   169,    -1,   271,    -1,   271,
     130,   173,    -1,   172,    -1,   271,    -1,   285,    -1,   279,
      -1,    16,    -1,    11,    -1,    13,    -1,    12,    -1,    15,
      -1,   171,    -1,    -1,   177,   174,   173,    -1,    -1,   171,
     178,   175,   173,    -1,    -1,   122,   176,   173,   123,    -1,
     131,    -1,   132,    -1,   133,    -1,   131,    -1,   132,    -1,
     134,    -1,   135,    -1,   136,    -1,   137,    -1,   138,    -1,
     139,    -1,     3,   124,   336,   125,   337,   121,    -1,    33,
     271,   124,   336,   125,   337,   121,    -1,    33,   124,   336,
     125,   337,   121,    -1,    49,   337,   121,    -1,   187,   184,
      -1,     4,   271,   185,    -1,     3,   271,   185,    -1,   121,
      -1,   124,   336,   125,   121,    -1,   128,   337,   121,    -1,
      47,   274,   263,   264,   121,    -1,    47,     4,   271,   124,
     336,   125,   241,   121,    -1,    47,     3,   271,   124,   336,
     125,   241,   121,    -1,    47,   274,    84,   241,   123,   264,
     121,    -1,    47,   274,    83,   241,   123,   264,   121,    -1,
      47,   274,    84,   241,   123,   122,   230,   123,   121,    -1,
      47,   274,    83,   241,   123,   122,   230,   123,   121,    -1,
      47,   166,    -1,    47,   163,    -1,    47,   181,    -1,    47,
     180,    -1,    47,   179,    -1,    47,    53,   121,    -1,    45,
     126,   127,    -1,    -1,    45,   126,   188,   189,   127,    -1,
     191,    -1,    -1,   191,   129,   190,   189,    -1,   291,   194,
      -1,   193,   194,    -1,    -1,   192,   187,   194,    -1,     4,
      -1,    46,    -1,    -1,    -1,   271,   195,   242,    -1,    54,
     122,   198,   123,    -1,    54,   122,   201,   123,    -1,   272,
     212,    -1,   272,   200,   212,    -1,   286,    82,   133,   227,
      -1,    43,   286,    82,   133,   227,    -1,   286,    82,   220,
      -1,    43,   286,    82,   220,    -1,   286,    82,   286,    82,
     133,   227,    -1,    43,   286,    82,   286,    82,   133,   227,
      -1,   286,    82,   286,    82,   220,    -1,    43,   286,    82,
     286,    82,   220,    -1,   286,    82,    -1,   200,   286,    82,
      -1,   133,   227,    -1,    43,   133,   227,    -1,     8,   133,
     227,    -1,   220,    -1,    43,   220,    -1,   272,   212,    -1,
       8,   272,   212,    -1,   286,    82,   204,    -1,   272,   200,
     207,    -1,   204,    -1,   272,   207,    -1,     8,   274,   207,
      -1,    -1,    -1,    39,   272,   122,   205,   234,   123,   206,
     214,    -1,    -1,   209,   208,   214,    -1,    -1,    -1,    39,
     333,   210,   122,   211,   234,   123,    -1,    -1,   216,   213,
     214,    -1,    -1,   130,    16,    -1,    38,    16,    -1,    36,
      -1,   121,    -1,   124,   336,   125,    -1,    -1,   271,   122,
     217,   234,   123,    -1,    -1,    -1,   271,   126,   218,   282,
     127,   122,   219,   234,   123,    -1,    -1,   222,   221,   224,
      -1,    -1,   271,   122,   223,   234,   123,    -1,    -1,   128,
     226,   225,    -1,    -1,   129,   226,   225,    -1,   271,   122,
     336,   123,    -1,   228,    -1,    -1,   271,   122,   229,   234,
     123,    -1,    -1,   231,    -1,    81,    -1,   233,    -1,    -1,
     233,   129,   232,   231,    -1,   274,   252,   242,    -1,    53,
      -1,    -1,    -1,   235,   236,    -1,    81,    -1,   238,    -1,
      -1,   238,   129,   237,   236,    -1,    -1,    -1,   239,   274,
     252,   240,   242,    -1,    53,    -1,   271,    -1,   287,   271,
      -1,    -1,   243,    -1,    -1,   130,   244,   297,    -1,   272,
     246,   247,   121,    -1,    51,    53,   247,   121,    -1,    53,
     247,   121,    -1,   254,   242,    -1,    -1,    -1,   247,   129,
     248,   249,    -1,    -1,   250,   246,    -1,    -1,   287,   251,
     246,    -1,   262,   264,    -1,    -1,   256,   260,   123,   253,
     258,    -1,   263,   264,    -1,    -1,   257,   261,   123,   255,
     258,    -1,   122,    -1,    83,    -1,    84,    -1,    83,    -1,
      84,    -1,    -1,    -1,   122,   259,   234,   123,    -1,   265,
      -1,   252,    -1,   287,   252,    -1,   254,    -1,   287,   254,
      -1,    -1,   263,    -1,   271,    -1,    -1,   265,    -1,    -1,
     266,   267,    -1,    -1,   269,   140,   268,   270,   141,    -1,
      -1,   267,    -1,    -1,   173,    -1,    50,    -1,     9,    -1,
      31,    -1,    30,    -1,    85,    -1,    86,    -1,   274,    -1,
      44,   274,    -1,    52,   274,    -1,    52,    10,   274,    -1,
      43,   274,    -1,   273,   274,    -1,    43,   273,   274,    -1,
      51,    -1,    51,    43,    -1,   275,    -1,   275,   287,    -1,
     277,    -1,   276,   277,    -1,   277,   276,    -1,    36,    -1,
     290,    -1,   279,    -1,   285,    -1,    -1,    46,   278,   284,
      -1,    -1,    50,   126,   280,   282,   127,    -1,    -1,     9,
     126,   281,   282,   127,    -1,   274,    -1,    -1,   274,   129,
     283,   282,    -1,    50,    -1,     9,    -1,    31,    -1,    30,
      -1,    85,    -1,    86,    -1,   279,    -1,   285,    -1,   286,
      82,   284,    -1,   279,    82,   284,    -1,     9,    -1,    50,
      -1,    31,    -1,    30,    -1,    85,    -1,    86,    -1,   137,
      -1,   288,   137,    -1,   288,    -1,   289,    -1,   288,   289,
      -1,   134,    -1,    37,    -1,   291,    -1,     4,   292,    -1,
       3,   292,    -1,    33,     9,    -1,    33,    50,    -1,    32,
       9,    -1,    32,    50,    -1,   293,    -1,   292,    -1,    85,
      -1,    86,    -1,    30,    -1,    31,    -1,     9,    -1,    50,
      -1,    24,    -1,    18,    -1,    23,    -1,    27,    -1,    28,
      -1,    29,    -1,    26,    -1,    88,    -1,    89,    -1,    90,
      -1,    91,    -1,    92,    -1,    93,    -1,    94,    -1,    95,
      -1,    96,    -1,    97,    -1,    -1,    41,   294,   296,    -1,
      -1,    40,   295,   296,    -1,   296,    -1,    25,    -1,    17,
      -1,    19,    -1,    20,    -1,    87,    -1,    21,    -1,    22,
      -1,   302,    -1,    -1,   124,   298,   297,   300,   299,   125,
      -1,    -1,   129,    -1,    -1,    -1,   300,   129,   301,   297,
      -1,   309,    -1,    -1,   132,   303,   309,    -1,    -1,   131,
     304,   309,    -1,   308,    -1,    -1,   122,   305,   302,   123,
      -1,    -1,    -1,     9,   126,   306,   275,   127,   122,   307,
     309,   123,    -1,    10,    -1,   308,    10,    -1,    16,    -1,
      11,    -1,    13,    -1,    12,    -1,    14,    -1,    15,    -1,
       9,    -1,    50,    -1,    -1,    98,   122,   271,   129,   311,
     274,   123,    -1,    -1,    -1,    -1,    99,   122,   312,   271,
     129,   313,   274,   314,   123,    -1,    -1,   100,   122,   315,
     271,   123,    -1,    -1,   101,   122,   316,   271,   123,    -1,
      -1,    -1,   102,   122,   271,   129,   317,   290,   318,   129,
     337,   123,    -1,    -1,   103,   122,   271,   129,   319,   290,
     123,    -1,    -1,    -1,    -1,   104,   122,   320,   271,   129,
     321,   290,   322,   123,    -1,   105,   122,   271,   129,   290,
     123,    -1,    -1,   106,   122,   271,   129,   323,   290,   123,
      -1,    -1,   110,   122,   271,   129,   324,   290,   123,    -1,
      -1,   107,   122,   271,   129,   325,   290,   123,    -1,    -1,
     111,   122,   271,   129,   326,   290,   123,    -1,    -1,   108,
     122,   271,   129,   327,   290,   123,    -1,    -1,   112,   122,
     271,   129,   328,   290,   123,    -1,    -1,   109,   122,   271,
     129,   329,   290,   123,    -1,    -1,   113,   122,   271,   129,
     330,   290,   123,    -1,    -1,   114,   122,   271,   129,   331,
     290,   129,    11,   123,    -1,    -1,   115,   122,   271,   129,
     332,   290,   129,    11,   123,    -1,   116,   122,   271,   123,
      -1,   117,   122,   271,   123,    -1,   118,   122,   271,   129,
     271,   123,    -1,   118,   122,   271,   129,   271,   129,   123,
      -1,   122,   123,    -1,   140,   141,    -1,    55,   140,   141,
      -1,    56,   140,   141,    -1,   334,    -1,   130,    -1,   134,
      -1,   135,    -1,   131,    -1,   132,    -1,   142,    -1,   133,
      -1,   129,    -1,   126,    -1,   127,    -1,   137,    -1,   138,
      -1,   139,    -1,   136,    -1,    55,    -1,    56,    -1,    57,
      -1,    58,    -1,    59,    -1,    60,    -1,    61,    -1,    62,
      -1,    65,    -1,    66,    -1,    67,    -1,    68,    -1,    69,
      -1,    63,    -1,    64,    -1,    70,    -1,    71,    -1,    72,
      -1,    73,    -1,    74,    -1,    75,    -1,    76,    -1,    77,
      -1,    78,    -1,    79,    -1,    80,    -1,   119,    -1,    -1,
     336,   338,    -1,    -1,   337,   339,    -1,   121,    -1,   339,
      -1,    35,    -1,   340,    -1,   342,    -1,   341,    -1,    47,
      -1,   334,    -1,   128,    -1,   143,    -1,    82,    -1,     4,
      -1,    45,    -1,    31,    -1,    30,    -1,    85,    -1,    86,
      -1,   293,    -1,    13,    -1,    11,    -1,    12,    -1,    14,
      -1,    15,    -1,    10,    -1,    34,    -1,    36,    -1,    37,
      -1,    38,    -1,     3,    -1,    39,    -1,    51,    -1,    43,
      -1,     8,    -1,    32,    -1,    33,    -1,    46,    -1,    16,
      -1,    53,    -1,    81,    -1,     5,    -1,     7,    -1,     6,
      -1,    48,    -1,    49,    -1,    52,    -1,     9,    -1,    50,
      -1,   335,    -1,   124,   336,   125,    -1,   140,   336,   141,
      -1,   122,   336,   123,    -1,    83,   336,   123,    -1,    84,
     336,   123,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   909,   909,   910,   909,   914,   915,   916,   917,   918,
     919,   920,   921,   922,   923,   924,   925,   926,   927,   928,
     929,   930,   931,   932,   933,   934,   935,   936,   937,   938,
     939,   945,   951,   951,   953,   959,   959,   961,   961,   963,
     963,   965,   965,   968,   969,   968,   972,   973,   974,   975,
     976,   977,   978,   979,   980,   981,   982,   983,   984,   985,
     986,   988,   989,   990,   992,   993,   994,   995,   996,   997,
     999,   999,  1001,  1001,  1003,  1004,  1005,  1006,  1013,  1014,
    1015,  1025,  1026,  1025,  1028,  1029,  1028,  1031,  1031,  1031,
    1033,  1034,  1036,  1037,  1037,  1037,  1039,  1039,  1039,  1039,
    1039,  1041,  1042,  1042,  1047,  1047,  1053,  1053,  1060,  1060,
    1061,  1063,  1063,  1064,  1064,  1065,  1065,  1066,  1066,  1072,
    1074,  1076,  1078,  1080,  1082,  1083,  1085,  1086,  1087,  1089,
    1090,  1091,  1092,  1093,  1094,  1095,  1096,  1097,  1098,  1099,
    1100,  1101,  1107,  1108,  1108,  1112,  1113,  1113,  1116,  1126,
    1134,  1134,  1146,  1147,  1149,  1149,  1149,  1156,  1158,  1164,
    1166,  1167,  1168,  1169,  1170,  1171,  1172,  1173,  1174,  1176,
    1177,  1179,  1180,  1181,  1186,  1187,  1188,  1189,  1197,  1198,
    1201,  1202,  1203,  1213,  1217,  1212,  1232,  1232,  1244,  1245,
    1244,  1252,  1252,  1264,  1265,  1274,  1284,  1290,  1290,  1293,
    1292,  1297,  1298,  1297,  1307,  1307,  1317,  1317,  1319,  1319,
    1321,  1321,  1323,  1325,  1339,  1339,  1345,  1345,  1347,  1348,
    1348,  1348,  1351,  1352,  1356,  1356,  1356,  1359,  1360,  1361,
    1361,  1364,  1366,  1364,  1395,  1419,  1419,  1421,  1421,  1423,
    1423,  1430,  1431,  1432,  1434,  1445,  1446,  1446,  1449,  1449,
    1450,  1450,  1454,  1455,  1455,  1461,  1462,  1462,  1467,  1468,
    1470,  1473,  1475,  1478,  1479,  1479,  1481,  1484,  1485,  1489,
    1490,  1493,  1493,  1495,  1497,  1497,  1499,  1499,  1501,  1501,
    1503,  1503,  1505,  1506,  1512,  1513,  1514,  1515,  1516,  1517,
    1524,  1525,  1526,  1527,  1529,  1530,  1532,  1536,  1537,  1539,
    1540,  1542,  1543,  1544,  1546,  1548,  1549,  1551,  1553,  1553,
    1557,  1557,  1559,  1559,  1562,  1562,  1562,  1564,  1565,  1566,
    1567,  1568,  1569,  1570,  1571,  1573,  1579,  1586,  1586,  1586,
    1586,  1586,  1586,  1602,  1603,  1604,  1609,  1610,  1622,  1623,
    1626,  1627,  1628,  1629,  1630,  1631,  1632,  1635,  1636,  1639,
    1640,  1641,  1642,  1643,  1644,  1647,  1648,  1649,  1650,  1651,
    1652,  1653,  1654,  1655,  1656,  1657,  1658,  1659,  1660,  1661,
    1662,  1663,  1664,  1664,  1665,  1665,  1667,  1670,  1671,  1672,
    1673,  1674,  1675,  1676,  1682,  1683,  1683,  1692,  1692,  1694,
    1695,  1695,  1703,  1704,  1704,  1705,  1705,  1708,  1709,  1709,
    1710,  1711,  1710,  1719,  1720,  1727,  1728,  1729,  1730,  1731,
    1732,  1733,  1735,  1745,  1745,  1758,  1759,  1759,  1758,  1771,
    1771,  1784,  1784,  1796,  1796,  1796,  1839,  1838,  1852,  1853,
    1853,  1852,  1865,  1893,  1893,  1898,  1898,  1903,  1903,  1908,
    1908,  1913,  1913,  1918,  1918,  1923,  1923,  1928,  1928,  1933,
    1933,  1953,  1953,  1971,  2025,  2081,  2140,  2206,  2207,  2208,
    2209,  2210,  2212,  2213,  2213,  2214,  2214,  2215,  2215,  2216,
    2216,  2217,  2217,  2218,  2218,  2219,  2220,  2221,  2222,  2223,
    2224,  2225,  2226,  2227,  2228,  2229,  2230,  2231,  2232,  2233,
    2234,  2235,  2236,  2237,  2238,  2239,  2240,  2241,  2242,  2243,
    2244,  2245,  2251,  2274,  2274,  2275,  2275,  2277,  2277,  2279,
    2279,  2279,  2279,  2279,  2280,  2280,  2280,  2280,  2280,  2280,
    2281,  2281,  2281,  2281,  2281,  2282,  2282,  2282,  2282,  2282,
    2283,  2283,  2283,  2283,  2283,  2283,  2284,  2284,  2284,  2284,
    2284,  2284,  2284,  2285,  2285,  2285,  2285,  2285,  2285,  2286,
    2286,  2286,  2286,  2286,  2286,  2288,  2289,  2290,  2290,  2290
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
  "named_enum", "$@8", "$@9", "anonymous_enum", "$@10", "$@11",
  "enum_list", "enum_item", "integer_value", "integer_literal",
  "integer_expression", "$@12", "$@13", "$@14", "math_unary_op",
  "math_binary_op", "anonymous_struct", "named_union", "anonymous_union",
  "using", "template_internal_class", "internal_class",
  "internal_class_body", "typedef", "template", "$@15", "template_args",
  "$@16", "template_arg", "$@17", "class_or_typename", "maybe_template_id",
  "$@18", "legacy_function", "legacy_method", "function", "scoped_method",
  "scope", "method", "scoped_operator", "operator", "typecast_op_func",
  "$@19", "$@20", "op_func", "$@21", "op_sig", "$@22", "$@23", "func",
  "$@24", "func_trailer", "func_body", "func_sig", "$@25", "$@26", "@27",
  "constructor", "$@28", "constructor_sig", "$@29", "maybe_initializers",
  "more_initializers", "initializer", "destructor", "destructor_sig",
  "$@30", "ignore_args_list", "ignore_more_args", "$@31", "ignore_arg",
  "args_list", "$@32", "more_args", "$@33", "arg", "$@34", "$@35",
  "maybe_indirect_id", "maybe_var_assign", "var_assign", "$@36", "var",
  "var_id_maybe_assign", "maybe_other_vars", "$@37", "other_var", "$@38",
  "$@39", "maybe_complex_var_id", "$@40", "complex_var_id", "$@41",
  "p_or_lp_or_la", "lp_or_la", "maybe_array_or_args", "$@42",
  "maybe_indirect_maybe_var_id", "maybe_indirect_var_id", "maybe_var_id",
  "var_id", "maybe_var_array", "var_array", "$@43", "array", "$@44",
  "more_array", "array_size", "any_id", "storage_type", "static_mod",
  "type", "type_red", "const_mod", "type_red1", "$@45", "templated_id",
  "$@46", "$@47", "types", "$@48", "maybe_scoped_id", "scoped_id",
  "class_id", "type_indirection", "pointers", "pointer_or_const_pointer",
  "type_red2", "type_simple", "type_id", "type_primitive", "$@49", "$@50",
  "type_integer", "value", "$@51", "maybe_comma", "more_values", "$@52",
  "literal", "$@53", "$@54", "$@55", "$@56", "$@57", "string_literal",
  "literal2", "macro", "$@58", "$@59", "$@60", "$@61", "$@62", "$@63",
  "$@64", "$@65", "$@66", "$@67", "$@68", "$@69", "$@70", "$@71", "$@72",
  "$@73", "$@74", "$@75", "$@76", "$@77", "$@78", "$@79", "op_token",
  "op_token_no_delim", "vtk_constant_def", "maybe_other",
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
     147,   147,   147,   147,   147,   147,   147,   147,   147,   147,
     147,   148,   150,   149,   149,   152,   151,   153,   151,   154,
     151,   155,   151,   156,   157,   156,   158,   158,   158,   158,
     158,   158,   158,   158,   158,   158,   158,   158,   158,   158,
     158,   158,   158,   158,   158,   158,   158,   158,   158,   158,
     159,   159,   160,   160,   161,   161,   161,   161,   162,   162,
     162,   164,   165,   163,   167,   168,   166,   169,   169,   169,
     170,   170,   171,   171,   171,   171,   172,   172,   172,   172,
     172,   173,   174,   173,   175,   173,   176,   173,   177,   177,
     177,   178,   178,   178,   178,   178,   178,   178,   178,   179,
     180,   181,   182,   183,   184,   184,   185,   185,   185,   186,
     186,   186,   186,   186,   186,   186,   186,   186,   186,   186,
     186,   186,   187,   188,   187,   189,   190,   189,   191,   191,
     192,   191,   193,   193,   194,   195,   194,   196,   197,   198,
     199,   199,   199,   199,   199,   199,   199,   199,   199,   200,
     200,   201,   201,   201,   201,   201,   201,   201,   202,   202,
     203,   203,   203,   205,   206,   204,   208,   207,   210,   211,
     209,   213,   212,   214,   214,   214,   214,   215,   215,   217,
     216,   218,   219,   216,   221,   220,   223,   222,   224,   224,
     225,   225,   226,   227,   229,   228,   230,   230,   231,   231,
     232,   231,   233,   233,   234,   235,   234,   236,   236,   237,
     236,   239,   240,   238,   238,   241,   241,   242,   242,   244,
     243,   245,   245,   245,   246,   247,   248,   247,   250,   249,
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
     293,   293,   294,   293,   295,   293,   293,   296,   296,   296,
     296,   296,   296,   296,   297,   298,   297,   299,   299,   300,
     301,   300,   302,   303,   302,   304,   302,   302,   305,   302,
     306,   307,   302,   308,   308,   309,   309,   309,   309,   309,
     309,   309,   309,   311,   310,   312,   313,   314,   310,   315,
     310,   316,   310,   317,   318,   310,   319,   310,   320,   321,
     322,   310,   310,   323,   310,   324,   310,   325,   310,   326,
     310,   327,   310,   328,   310,   329,   310,   330,   310,   331,
     310,   332,   310,   310,   310,   310,   310,   333,   333,   333,
     333,   333,   334,   334,   334,   334,   334,   334,   334,   334,
     334,   334,   334,   334,   334,   334,   334,   334,   334,   334,
     334,   334,   334,   334,   334,   334,   334,   334,   334,   334,
     334,   334,   334,   334,   334,   334,   334,   334,   334,   334,
     334,   334,   335,   336,   336,   337,   337,   338,   338,   339,
     339,   339,   339,   339,   339,   339,   339,   339,   339,   339,
     339,   339,   339,   339,   339,   339,   339,   339,   339,   339,
     339,   339,   339,   339,   339,   339,   339,   339,   339,   339,
     339,   339,   339,   339,   339,   339,   339,   339,   339,   339,
     339,   339,   339,   339,   339,   340,   341,   342,   342,   342
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     0,     3,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     1,     2,     3,
       2,     2,     2,     3,     2,     1,     1,     2,     4,     4,
       1,     5,     0,     6,     4,     0,     7,     0,    10,     0,
       7,     0,    10,     0,     0,     3,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     1,     1,     2,
       3,     3,     2,     3,     3,     2,     5,     1,     1,     1,
       0,     2,     1,     3,     1,     2,     2,     2,     1,     1,
       1,     0,     0,     9,     0,     0,     8,     0,     1,     3,
       1,     3,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     0,     3,     0,     4,     0,     4,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     6,
       7,     6,     3,     2,     3,     3,     1,     4,     3,     5,
       8,     8,     7,     7,     9,     9,     2,     2,     2,     2,
       2,     3,     3,     0,     5,     1,     0,     4,     2,     2,
       0,     3,     1,     1,     0,     0,     3,     4,     4,     2,
       3,     4,     5,     3,     4,     6,     7,     5,     6,     2,
       3,     2,     3,     3,     1,     2,     2,     3,     3,     3,
       1,     2,     3,     0,     0,     8,     0,     3,     0,     0,
       7,     0,     3,     0,     2,     2,     1,     1,     3,     0,
       5,     0,     0,     9,     0,     3,     0,     5,     0,     3,
       0,     3,     4,     1,     0,     5,     0,     1,     1,     1,
       0,     4,     3,     1,     0,     0,     2,     1,     1,     0,
       4,     0,     0,     5,     1,     1,     2,     0,     1,     0,
       3,     4,     4,     3,     2,     0,     0,     4,     0,     2,
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
       9,     0,     9,     4,     4,     6,     7,     2,     2,     3,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     0,     2,     0,     2,     1,     1,     1,
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
       2,     3,     1,     0,     0,     0,     0,   353,   378,   356,
     379,   380,   382,   383,   357,   355,   377,   361,   358,   359,
     360,   351,   352,    84,     0,    17,   304,     0,   374,   372,
       0,     0,     0,   308,     0,     0,   505,   354,   297,     0,
     245,     0,   349,   350,   381,   362,   363,   364,   365,   366,
     367,   368,   369,   370,   371,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   502,    30,     4,    13,
      12,    15,     7,     6,    10,     9,     8,    11,    14,     0,
       0,     0,     0,     0,     0,   180,     5,     0,     0,   290,
     299,     0,   301,   306,   307,     0,   305,   340,   348,   347,
     376,    25,    26,   353,   351,   352,   354,   349,   350,   503,
      39,   342,    35,   341,     0,     0,   353,     0,     0,   354,
       0,     0,   503,   312,   345,   287,   286,   346,   288,   289,
       0,    81,   343,   344,   503,     0,     0,   297,     0,     0,
       0,     0,     0,   294,     0,   291,   143,     0,     0,     0,
       0,   137,   136,   140,   139,   138,     0,   327,   330,   329,
     328,   331,   332,   503,    32,     0,   503,   310,   298,   245,
       0,   292,     0,     0,     0,   415,   419,   421,     0,     0,
     428,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    16,     0,     0,     0,
     197,   503,    24,    21,    22,    20,    18,   285,   287,   286,
       0,   284,   261,   262,   288,   289,     0,   181,   186,   159,
     191,   245,   237,     0,   274,   273,     0,   295,   339,   338,
     333,   300,   335,   336,   302,   303,     0,     0,    27,     0,
       0,    70,     0,    70,   353,   351,   352,   354,   349,   350,
     345,   346,   343,   344,   182,     0,     0,     0,    87,     0,
       0,   503,     0,   183,   375,   373,   296,     0,   142,   150,
     318,   320,   319,   317,   321,   322,   323,   309,   324,     0,
       0,   141,   285,   284,     0,     0,   274,   273,     0,     0,
     535,   518,   546,   548,   547,   539,   552,   530,   526,   527,
     525,   528,   529,   543,   521,   520,   540,   541,   531,   509,
     532,   533,   534,   536,   538,   519,   542,   513,   549,   550,
     553,   537,   551,   544,   476,   477,   478,   479,   480,   481,
     482,   483,   489,   490,   484,   485,   486,   487,   488,   491,
     492,   493,   494,   495,   496,   497,   498,   499,   500,   501,
     545,   517,   503,   503,   522,   523,   122,   503,   503,   470,
     471,   515,   469,   462,   465,   466,   468,   463,   464,   475,
     472,   473,   474,   503,   467,   516,   524,   514,   554,   506,
     510,   512,   511,     0,     0,     0,     2,   293,   243,   246,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    23,    19,     0,     0,   476,   477,     0,
       0,   188,   461,   179,   160,     0,   193,   193,     0,   239,
     244,   238,   269,     0,     0,   255,   275,   280,   199,   201,
     169,   334,   337,   326,   318,   320,   319,   317,   321,   322,
       0,   178,   163,   204,     0,   325,     0,   507,   505,   504,
     508,   314,     0,     0,     0,     0,     0,    28,     0,     0,
      88,    90,    87,   505,     0,   225,     0,   164,     0,   152,
     153,     0,   145,     0,   154,   154,   503,   503,     0,   235,
       0,     0,     0,    34,     2,     0,     0,     0,     0,     0,
      29,     0,   242,     3,   248,   157,   413,     0,     0,     0,
     423,   426,     0,     0,   433,   437,   441,   445,   435,   439,
     443,   447,   449,   451,   453,   454,     0,   198,     0,     0,
     457,   458,     0,   170,   196,     0,     0,   187,   192,   241,
       0,   256,   270,   277,     0,   225,     0,   161,   213,     0,
     208,   206,     0,     0,   315,    41,     0,     0,     0,    71,
      72,    74,    43,    37,    43,   313,    85,    87,     0,     0,
       0,   505,     0,   231,   162,     0,   144,   146,   154,   149,
     155,   148,     0,     0,   274,   236,   274,   129,     3,   558,
     559,   557,   555,   556,   311,    31,   247,     0,   250,     0,
     416,   420,   422,     0,     0,   429,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   459,   460,
     189,   195,   194,   411,   403,   406,   408,   407,   409,   410,
     405,   412,   398,   385,   395,   393,   240,   384,   397,   392,
     263,   278,     0,     0,   214,     0,   205,   225,     0,   167,
     119,     0,    70,    77,    75,    76,     0,    44,    70,    44,
     505,    89,   285,    97,    99,    98,   100,    96,   284,   106,
     108,   109,   110,   101,    92,    91,   102,    93,    95,    94,
      82,   121,     0,   184,   234,   227,   226,   228,     0,     0,
     168,   150,   151,   237,     0,     0,   216,     0,   216,     0,
      33,   249,     0,     0,     0,   424,     0,     0,   432,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   455,
       0,   225,   400,     0,     0,     0,     0,   404,   264,   257,
     266,   282,   200,     0,   225,   210,     0,     0,   165,   316,
       0,    73,    40,     0,     0,    36,     0,     0,   111,   112,
     113,   114,   115,   116,   117,   118,   104,     0,   505,   120,
     193,   229,   271,   166,   147,   156,     0,     0,   223,   218,
       0,   217,   219,   271,   133,     0,   132,   251,   414,   417,
       0,   427,   430,   434,   438,   442,   446,   436,   440,   444,
     448,     0,     0,   456,     0,     0,     0,   389,   411,   396,
     394,   225,   283,     0,   202,     0,     0,   209,   503,   207,
      43,     0,     0,    78,    79,    80,     0,   353,   351,   352,
      58,     0,     0,   354,     0,   349,   350,     0,    69,     0,
      45,     0,    49,    48,    52,    51,    50,    53,    57,    55,
      54,     0,     0,     0,     0,   174,    47,     0,    67,    68,
      43,    86,     0,     0,   103,     0,   185,   231,   259,   260,
     258,   232,   271,   274,   272,   131,   130,     0,   220,   237,
       0,     0,   505,     0,     0,     0,   190,     0,   399,   387,
       0,   279,   225,   215,   210,     0,    44,     0,     0,     0,
       0,   290,     0,    56,     0,     0,     0,     0,   175,     0,
     503,   171,    46,   123,     0,     0,    65,    62,    59,   176,
      44,   107,   105,    83,   230,   237,   267,     0,   271,   252,
     135,     0,   222,   134,   418,     0,   431,   450,   452,     0,
     390,     0,   265,     0,   211,   212,    42,   126,   503,   505,
     125,   124,   173,   177,    63,    60,   172,     0,     0,     0,
       0,    64,    61,    38,   233,   253,   268,   221,   425,   401,
       0,   386,   203,     0,     0,   158,     0,   263,     0,   391,
       0,   128,    66,   254,     0,   127,   402
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     3,    78,    79,    80,   299,    81,   253,   668,
     251,   662,   667,   753,   840,   474,   569,   570,   841,    82,
     269,   768,    83,   140,   670,   479,   480,   683,   684,   685,
     767,   863,   757,   686,   766,    84,    85,    86,    87,   848,
     849,   950,    88,    89,   279,   491,   701,   492,   493,   494,
     589,   703,    90,   852,    91,    92,   226,   853,    93,    94,
      95,   485,   770,   227,   436,   228,   542,   731,   229,   437,
     547,   212,   230,   555,   556,   892,   855,   560,   463,   657,
     656,   817,   745,   557,   558,   744,   780,   781,   931,   782,
     582,   583,   696,   867,   697,   698,   925,   498,   440,   441,
     550,    96,   231,   182,   514,   606,   607,   712,   871,   977,
     232,   650,   872,   233,   739,   811,   927,   443,   873,   234,
     445,   446,   447,   553,   741,   554,   813,   297,   900,    98,
      99,   100,   101,   102,   157,   103,   394,   267,   472,   661,
     465,   104,   131,   500,   242,   243,   106,   107,   108,   109,
     151,   150,   110,   646,   734,   941,   889,   970,   647,   736,
     735,   733,   805,   978,   648,   649,   111,   609,   403,   714,
     881,   404,   405,   613,   790,   614,   408,   717,   883,   617,
     621,   618,   622,   619,   623,   620,   624,   625,   626,   431,
     387,   388,   249,   175,   469,   470,   390,   391,   392
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -761
static const yytype_int16 yypact[] =
{
    -761,    69,  -761,  5085,    55,   513,  5808,    95,  -761,  -761,
    -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,
    -761,     2,    12,   672,   363,  -761,  -761,  5299,  -761,  -761,
    5489,  5808,   -46,  -761,  5584,   442,  -761,   145,    56,  5679,
    -761,    23,    99,   125,  -761,  -761,  -761,  -761,  -761,  -761,
    -761,  -761,  -761,  -761,  -761,    54,    91,   116,   122,   127,
     132,   147,   157,   164,   166,   175,   180,   191,   200,   205,
     206,   213,   215,   216,   219,   226,  -761,  -761,  -761,  -761,
    -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  5204,
     156,   156,   156,   156,   156,  -761,  -761,   583,  5808,  -761,
      19,  5842,    68,    38,  -761,   137,  -761,  -761,  -761,  -761,
    -761,  -761,  -761,   161,   221,   295,   454,   522,   619,  -761,
     152,  -761,   227,  -761,   746,   746,    -1,    74,    80,     0,
     313,   277,  -761,  -761,   236,  -761,  -761,   237,  -761,  -761,
     238,  -761,   236,   237,  -761,   243,  5489,   327,  5713,   254,
     214,   214,  5808,  -761,   299,  -761,   255,   758,    55,   513,
     262,  -761,  -761,  -761,  -761,  -761,   599,  -761,  -761,  -761,
    -761,  -761,  -761,  -761,  -761,  3350,  -761,  -761,  -761,  -761,
    4977,  -761,   103,  5299,  4622,  -761,  -761,  -761,  4622,  4622,
    -761,  4622,  4622,  4622,  4622,  4622,  4622,  4622,  4622,  4622,
    4622,  4622,  4622,  4622,  4622,   513,  -761,   156,   156,   445,
    -761,  -761,  -761,  -761,  -761,  -761,  -761,   306,   309,   314,
     812,   320,  -761,  -761,   326,   332,   538,  -761,  -761,  -761,
    -761,  -761,   279,   294,   275,   133,   336,  -761,  -761,  -761,
    -761,  -761,    77,  -761,  -761,  -761,   758,   265,  -761,   953,
    5808,   267,  5808,   267,  -761,  -761,  -761,  -761,  -761,  -761,
    -761,  -761,  -761,  -761,  -761,   758,  1094,  5808,  4622,   300,
    1235,  -761,  5808,  -761,  -761,  -761,  -761,   289,  -761,  5971,
      -1,   309,   314,     0,   326,   332,    38,  -761,  -761,   302,
     308,  -761,  -761,  -761,   231,   231,   275,  -761,  1376,   310,
    -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,
    -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,
    -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,
    -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,
    -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,
    -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,
    -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,
    -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,
    -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,
    -761,  -761,  -761,  1517,  5808,   108,  -761,  -761,  -761,  -761,
     297,  4622,   317,  4622,  4622,  4622,   321,   323,  4622,   328,
     330,   338,   339,   341,   342,   348,   350,   351,   354,   356,
     312,   316,   357,  -761,  -761,   133,  1658,   303,   349,   365,
     368,  -761,  -761,  -761,  -761,   428,    41,    41,   124,  -761,
    -761,  -761,  -761,   388,   756,  -761,  -761,  -761,  -761,  -761,
    -761,  -761,  -761,  -761,   -16,     5,    13,    -9,    15,    50,
    4622,  -761,  -761,  -761,   390,  -761,   435,  -761,  -761,  -761,
    -761,   391,   394,   509,   399,   397,   401,  -761,   402,   408,
     405,   407,  4622,  -761,  1799,   412,  4622,  -761,   456,  -761,
    -761,   415,   416,   501,  4622,  4622,  -761,  -761,   425,  -761,
    4622,   427,   436,  -761,  -761,  1940,  2081,  2222,  2363,  2504,
    -761,   433,  -761,   439,    19,  -761,  -761,   432,   444,   449,
    -761,  -761,   447,  5937,  -761,  -761,  -761,  -761,  -761,  -761,
    -761,  -761,  -761,  -761,  -761,  -761,  4622,  -761,   438,   448,
    -761,  -761,   463,  -761,  -761,   575,   577,  -761,  -761,  -761,
     714,  -761,  -761,   461,   462,   412,  5808,  -761,  -761,   474,
     475,  -761,   375,  3491,  -761,  -761,   758,   758,   758,  -761,
     480,  -761,  -761,  -761,  -761,  -761,  -761,  4622,   540,   479,
    3632,  -761,   488,    21,  -761,   380,  -761,  -761,  4622,  -761,
    -761,  -761,  2645,  2786,    35,  -761,    86,  -761,   487,  -761,
    -761,  -761,  -761,  -761,  -761,  -761,  -761,   756,  -761,  5808,
    -761,  -761,  -761,  5937,  5937,  -761,   494,  5937,  5937,  5937,
    5937,  5937,  5937,  5937,  5937,  5937,  5937,   -22,  -761,  -761,
    -761,  -761,  -761,   492,  -761,  -761,  -761,  -761,  -761,  -761,
    -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,   618,  -761,
     101,  -761,   512,   505,  -761,  4622,  -761,   412,  4622,  -761,
    -761,  5808,   267,  -761,  -761,  -761,   509,   511,   267,   520,
    -761,  -761,    -1,  -761,  -761,  -761,  -761,  -761,     0,  -761,
    -761,  -761,  -761,   783,  -761,  -761,  -761,  -761,    38,  -761,
    -761,  -761,  3773,  -761,  -761,  -761,  -761,   518,  5808,  4622,
    -761,  5971,  -761,   279,   231,   231,  5394,   531,  5394,   536,
    -761,  -761,   756,   535,  5808,  -761,   537,  5937,  -761,   542,
     547,   551,   554,   557,   563,   565,   566,   532,   550,  -761,
     567,   412,  -761,  4428,   714,   296,   296,  -761,  -761,  -761,
    -761,   540,  -761,   569,   412,   568,   572,   576,  -761,  -761,
     574,  -761,  -761,  4478,   581,  -761,  3914,   540,  -761,  -761,
    -761,  -761,  -761,  -761,  -761,  -761,  -761,   540,  -761,  -761,
      41,  -761,   556,  -761,  -761,  -761,   592,   594,  -761,  -761,
     595,  -761,   588,   556,  -761,   597,  -761,  -761,  -761,  -761,
     602,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,
    -761,   710,   721,  -761,   615,  5808,   616,  -761,  -761,  -761,
    -761,   412,  -761,   601,  -761,   627,  4622,  -761,  -761,  -761,
    -761,    55,   513,  -761,  -761,  -761,  4787,   -16,     5,    13,
    -761,  4597,  4882,    -9,   626,    15,    50,   629,  -761,  4622,
    -761,   624,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,
    -761,  4597,   156,   156,   156,  -761,  -761,   625,  -761,  -761,
    -761,  -761,   637,   540,  -761,  4055,  -761,    21,  -761,  -761,
    -761,  -761,    81,   275,  -761,  -761,  -761,   645,  -761,   279,
     647,   646,  -761,   648,   650,   656,  -761,   643,  -761,   651,
     658,  -761,   412,  -761,   568,  2927,   657,   208,   208,  4622,
    4622,   313,   513,  -761,   156,   156,   445,  4622,  -761,  4692,
    -761,  -761,  -761,  -761,   156,   156,  -761,  -761,  -761,  -761,
     660,  -761,  -761,  -761,  -761,   279,  -761,   667,   556,  -761,
    -761,  5394,  -761,  -761,  -761,  4196,  -761,  -761,  -761,   661,
     666,   668,  -761,   669,  -761,  -761,  -761,  -761,  -761,  -761,
    -761,  -761,  -761,  -761,  -761,  -761,  -761,  4787,   674,  4622,
    3068,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,
     714,  -761,  -761,  3209,  4337,  -761,   677,   101,   296,  -761,
     679,  -761,  -761,  -761,   688,  -761,  -761
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -761,  -335,  -761,  -761,  -761,  -761,  -761,   706,  -761,  -761,
    -761,  -761,  -526,  -761,  -761,  -235,   136,  -761,  -761,   -20,
    -761,  -761,   -19,  -761,  -761,  -425,  -761,  -761,  -761,  -689,
    -761,  -761,  -761,  -761,  -761,   -18,   -17,   -12,    60,  -761,
    -743,   -84,    62,  -447,  -761,   115,  -761,  -761,  -761,  -761,
    -437,  -761,  -761,  -761,   -13,  -761,  -761,  -760,  -761,   -77,
     582,  -761,  -761,  -123,  -761,  -761,  -761,  -761,  -213,  -761,
    -399,   -49,  -761,  -761,  -761,  -761,  -222,  -761,  -761,  -761,
    -761,   -75,    17,  -452,  -761,  -761,   113,   -83,  -761,  -761,
    -519,  -761,   -15,  -761,  -761,  -761,  -761,  -260,  -649,  -761,
    -761,    96,  -544,  -109,  -761,  -761,  -761,  -761,  -724,  -761,
    -184,  -761,  -761,  -761,  -127,  -761,  -761,  -761,  -761,  -156,
    -273,  -611,  -761,  -761,  -761,  -761,  -761,    -4,     3,   -25,
      -2,    46,   755,   757,  -761,  -104,  -761,  -761,  -192,  -761,
    -130,    53,    37,   -97,  -761,   620,  -120,  -248,     4,  -151,
    -761,  -761,    32,  -687,  -761,  -761,  -761,  -761,   128,  -761,
    -761,  -761,  -761,  -761,  -761,  -685,   107,  -761,  -761,  -761,
    -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,
    -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,  -761,
     644,     8,  -111,  -442,  -761,  -173,  -761,  -761,  -761
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -389
static const yytype_int16 yytable[] =
{
     120,   122,   389,   241,   130,   152,    97,   264,   121,   123,
     296,   112,   208,   434,   161,   162,   163,   164,   476,   141,
     145,   266,   165,   502,   386,   462,   563,   287,   153,   155,
     149,   495,   166,   270,   584,   501,   652,   181,   548,   740,
     105,   580,   213,   214,   215,   216,   588,   807,   669,   442,
     809,   810,   812,   286,   775,   487,   238,   579,   591,   879,
     475,   513,   298,   711,   113,   393,  -327,   154,   862,     2,
     395,   904,   174,  -328,   694,   478,   207,   544,   864,   545,
     156,  -327,  -328,   260,  -330,   114,   115,  -330,   903,   262,
     292,   914,   209,   235,  -329,  -329,   237,  -331,   386,   178,
     426,   729,   695,   433,    26,   116,  -285,   730,   913,   179,
     133,   135,   136,  -284,   238,   386,   453,   177,   238,   386,
     246,   152,   438,  -330,   261,   133,   177,  -287,   121,   123,
     263,   293,  -332,  -329,   236,  -286,   444,  -288,   747,   692,
     117,   118,   286,   286,   153,   183,   181,   386,   926,   958,
     276,   702,   671,   239,   289,   290,   240,   706,   423,   424,
     484,   286,   121,   123,   868,   869,   138,   139,   787,   598,
     400,   546,  -289,   286,   922,  -276,   184,  -327,   397,   119,
     402,  -331,   274,   275,   406,   407,   401,   409,   410,   411,
     412,   413,   414,   415,   416,   417,   418,   419,   420,   421,
     422,   120,   511,   870,   966,   425,   748,  -332,   708,   121,
     288,   239,   804,   185,   451,   239,  -327,   132,   240,   247,
    -331,   133,   425,   738,   398,   815,  -276,  -328,   756,   512,
     932,     8,   399,    10,    11,    12,    13,   399,   186,    16,
     292,  -276,   386,   464,   187,   549,  -332,   773,   471,   188,
     471,   505,   506,   399,   189,   448,   507,   508,   248,   449,
     552,   135,   136,   435,   481,   471,  -328,   176,   238,   190,
     397,   177,   509,   464,   454,   386,   964,   210,   250,   191,
     211,   293,  -285,   979,   466,  -285,   192,  -285,   193,  -285,
     499,   499,   890,   984,   896,   455,   456,   194,   454,   288,
     288,    44,   195,   292,    27,   808,   851,   635,   636,   637,
     638,   639,   640,   196,   488,   457,   138,   139,   288,   455,
     456,   707,   197,   709,   135,   136,   865,   198,   199,   947,
     288,   238,   948,   386,   920,   200,   949,   201,   202,   457,
     659,   203,  -287,   571,   293,  -287,   641,  -287,   204,  -287,
     458,   459,   220,   252,   386,   386,   386,   386,   386,   265,
    -285,  -284,   268,   700,   653,   239,   740,   271,   240,   286,
     178,   866,   142,   943,   458,   459,   273,   222,   223,   138,
     139,   277,   278,   291,   454,   592,   593,   911,  -327,   454,
     389,  -330,   471,   135,   136,   473,  -329,   425,   460,   517,
     518,   519,  -328,   616,   522,   455,   456,   389,  -331,   439,
     455,   456,   386,   143,  -332,  -276,  -286,   608,   450,  -286,
     515,  -286,   486,  -286,   482,   457,   496,   750,   239,   386,
     457,   240,   497,   754,   504,   534,   663,   664,   665,   535,
     935,   386,   386,   538,   776,   777,   516,   952,   138,   139,
     520,   167,   521,   495,   292,   956,   559,   523,   286,   524,
     458,   459,   286,   286,   286,   458,   459,   525,   526,   749,
     527,   528,   168,   169,   688,   135,   136,   529,   481,   530,
     531,   286,   559,   532,   220,   533,   536,   144,   540,   539,
     590,   590,   170,   715,   716,   293,   595,   719,   720,   721,
     722,   723,   724,   725,   726,   727,   728,   974,   658,   541,
     543,   551,   561,   699,   566,   567,   568,   562,   280,   389,
     564,   565,   113,   572,   573,   574,   288,   171,   172,   575,
     138,   139,   627,   576,   577,  -224,   571,   578,   585,   281,
     282,   386,   586,   114,   115,   587,    32,   217,   594,   672,
     596,   673,   674,   675,   471,   676,   677,   597,   464,   283,
     604,   610,   286,   116,   605,   292,   173,   611,   218,   219,
     218,   219,   612,   481,   687,  -284,   615,   220,  -284,   628,
    -284,   464,  -284,   389,   590,   630,   135,   136,   221,   629,
     678,   631,   217,   632,   284,   285,   654,   792,   117,   118,
     929,  -281,   651,   655,   690,   386,   293,   713,   292,   666,
     908,   693,   710,   218,   219,   288,   874,   718,   732,   288,
     288,   288,   220,   224,   225,   224,   225,   874,   737,   135,
     136,   689,   743,   221,   292,   742,   752,   688,   288,   868,
     869,   138,   139,  -288,   919,   755,  -288,   771,  -288,   293,
    -288,   746,   784,   688,   559,   135,   136,   786,   788,   471,
     791,   801,   679,   688,   220,   793,   222,   223,   224,   225,
     794,   680,   681,   682,   795,   293,   854,   796,   870,   802,
     797,   134,   294,   295,   138,   139,   798,   953,   799,   800,
     803,   814,   389,   919,   818,   559,   772,   816,   820,   819,
     499,   499,   135,   136,   783,   860,   783,   895,   222,   223,
     138,   139,   789,   875,   386,   876,   874,   878,   877,   288,
     880,   884,   137,   633,   634,   635,   636,   637,   638,   639,
     640,   882,   885,   842,   843,   844,   845,   687,   886,   888,
    -289,   846,   891,  -289,   386,  -289,   919,  -289,   909,   464,
     893,   910,   912,   687,   905,   254,   857,   138,   139,   688,
     921,   859,   389,   687,   641,   292,   930,   280,   933,   934,
     939,   936,   874,   937,   915,   928,   255,   256,   264,   938,
     940,   942,   946,   969,   386,   963,   135,   136,   281,   282,
     965,  -388,   972,   971,   689,   206,   257,   975,   982,   960,
     985,   389,   751,   916,   917,   918,   293,   152,   283,   386,
     689,   986,   746,   847,   951,   850,   774,   897,   898,   944,
     689,   785,   386,   386,   901,   121,   123,   464,   464,   461,
     153,   258,   259,   894,   906,   559,   642,   973,   643,   222,
     223,   138,   139,   284,   285,   644,   645,   464,   967,   856,
     983,   887,   924,   235,   906,   954,   955,   245,   244,   687,
     858,   806,   452,     0,   432,   961,   962,   427,   428,   336,
     337,   338,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,   356,
     357,   358,   359,     0,     0,   559,   425,     0,   897,     0,
       0,     0,   425,   559,     0,   464,   121,     0,     0,     0,
       0,     0,   959,     0,   758,   759,   689,   760,   761,   762,
     763,   764,   765,     0,     0,     0,     0,     0,     0,   783,
       0,     0,     0,     0,   429,     0,     0,     0,   369,   370,
       0,   372,   373,   374,   375,   376,   377,   378,   379,   380,
     381,   382,   430,     0,   384,   425,   300,   301,   302,   303,
     304,   305,   306,   307,   308,   309,   310,   311,   312,   313,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,   314,   315,   316,   317,   318,   319,   320,
     321,   322,   323,    28,    29,     0,   324,     0,   325,   326,
     327,   328,   329,   330,   331,   332,   333,     0,   334,   335,
     336,   337,   338,   339,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,   362,   363,   364,   365,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    76,     0,   467,   367,     0,   368,   468,   369,
     370,   371,   372,   373,   374,   375,   376,   377,   378,   379,
     380,   381,   382,   383,     0,   384,   385,   300,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,   311,   312,
     313,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,   314,   315,   316,   317,   318,   319,
     320,   321,   322,   323,    28,    29,     0,   324,     0,   325,
     326,   327,   328,   329,   330,   331,   332,   333,     0,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    76,     0,   467,   367,   477,   368,     0,
     369,   370,   371,   372,   373,   374,   375,   376,   377,   378,
     379,   380,   381,   382,   383,     0,   384,   385,   300,   301,
     302,   303,   304,   305,   306,   307,   308,   309,   310,   311,
     312,   313,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,   314,   315,   316,   317,   318,
     319,   320,   321,   322,   323,    28,    29,     0,   324,     0,
     325,   326,   327,   328,   329,   330,   331,   332,   333,     0,
     334,   335,   336,   337,   338,   339,   340,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,   356,   357,   358,   359,   360,   361,   362,   363,
     364,   365,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    76,     0,   467,   367,     0,   368,
     483,   369,   370,   371,   372,   373,   374,   375,   376,   377,
     378,   379,   380,   381,   382,   383,     0,   384,   385,   300,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
     311,   312,   313,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,   314,   315,   316,   317,
     318,   319,   320,   321,   322,   323,    28,    29,     0,   324,
       0,   325,   326,   327,   328,   329,   330,   331,   332,   333,
       0,   334,   335,   336,   337,   338,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,   356,   357,   358,   359,   360,   361,   362,
     363,   364,   365,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    76,     0,   467,   367,     0,
     368,   503,   369,   370,   371,   372,   373,   374,   375,   376,
     377,   378,   379,   380,   381,   382,   383,     0,   384,   385,
     300,   301,   302,   303,   304,   305,   306,   307,   308,   309,
     310,   311,   312,   313,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,   314,   315,   316,
     317,   318,   319,   320,   321,   322,   323,    28,    29,     0,
     324,     0,   325,   326,   327,   328,   329,   330,   331,   332,
     333,     0,   334,   335,   336,   337,   338,   339,   340,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,   356,   357,   358,   359,   360,   361,
     362,   363,   364,   365,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    76,     0,   467,   367,
     510,   368,     0,   369,   370,   371,   372,   373,   374,   375,
     376,   377,   378,   379,   380,   381,   382,   383,     0,   384,
     385,   300,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,   311,   312,   313,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,   314,   315,
     316,   317,   318,   319,   320,   321,   322,   323,    28,    29,
       0,   324,     0,   325,   326,   327,   328,   329,   330,   331,
     332,   333,     0,   334,   335,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,   356,   357,   358,   359,   360,
     361,   362,   363,   364,   365,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    76,     0,   467,
     367,     0,   368,   537,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,     0,
     384,   385,   300,   301,   302,   303,   304,   305,   306,   307,
     308,   309,   310,   311,   312,   313,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,    28,
      29,     0,   324,     0,   325,   326,   327,   328,   329,   330,
     331,   332,   333,     0,   334,   335,   336,   337,   338,   339,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,   355,   356,   357,   358,   359,
     360,   361,   362,   363,   364,   365,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    76,     0,
     467,   367,     0,   368,   581,   369,   370,   371,   372,   373,
     374,   375,   376,   377,   378,   379,   380,   381,   382,   383,
       0,   384,   385,   300,   301,   302,   303,   304,   305,   306,
     307,   308,   309,   310,   311,   312,   313,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
     314,   315,   316,   317,   318,   319,   320,   321,   322,   323,
      28,    29,     0,   324,     0,   325,   326,   327,   328,   329,
     330,   331,   332,   333,     0,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    76,
       0,   467,   367,   599,   368,     0,   369,   370,   371,   372,
     373,   374,   375,   376,   377,   378,   379,   380,   381,   382,
     383,     0,   384,   385,   300,   301,   302,   303,   304,   305,
     306,   307,   308,   309,   310,   311,   312,   313,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,   314,   315,   316,   317,   318,   319,   320,   321,   322,
     323,    28,    29,     0,   324,     0,   325,   326,   327,   328,
     329,   330,   331,   332,   333,     0,   334,   335,   336,   337,
     338,   339,   340,   341,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,   355,   356,   357,
     358,   359,   360,   361,   362,   363,   364,   365,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      76,     0,   467,   367,   600,   368,     0,   369,   370,   371,
     372,   373,   374,   375,   376,   377,   378,   379,   380,   381,
     382,   383,     0,   384,   385,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,   314,   315,   316,   317,   318,   319,   320,   321,
     322,   323,    28,    29,     0,   324,     0,   325,   326,   327,
     328,   329,   330,   331,   332,   333,     0,   334,   335,   336,
     337,   338,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,   356,
     357,   358,   359,   360,   361,   362,   363,   364,   365,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    76,     0,   467,   367,   601,   368,     0,   369,   370,
     371,   372,   373,   374,   375,   376,   377,   378,   379,   380,
     381,   382,   383,     0,   384,   385,   300,   301,   302,   303,
     304,   305,   306,   307,   308,   309,   310,   311,   312,   313,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,   314,   315,   316,   317,   318,   319,   320,
     321,   322,   323,    28,    29,     0,   324,     0,   325,   326,
     327,   328,   329,   330,   331,   332,   333,     0,   334,   335,
     336,   337,   338,   339,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,   362,   363,   364,   365,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    76,     0,   467,   367,     0,   368,   602,   369,
     370,   371,   372,   373,   374,   375,   376,   377,   378,   379,
     380,   381,   382,   383,     0,   384,   385,   300,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,   311,   312,
     313,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,   314,   315,   316,   317,   318,   319,
     320,   321,   322,   323,    28,    29,     0,   324,     0,   325,
     326,   327,   328,   329,   330,   331,   332,   333,     0,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    76,     0,   467,   367,     0,   368,     0,
     369,   370,   371,   372,   373,   374,   375,   376,   377,   378,
     379,   380,   381,   382,   383,   603,   384,   385,   300,   301,
     302,   303,   304,   305,   306,   307,   308,   309,   310,   311,
     312,   313,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,   314,   315,   316,   317,   318,
     319,   320,   321,   322,   323,    28,    29,     0,   324,     0,
     325,   326,   327,   328,   329,   330,   331,   332,   333,     0,
     334,   335,   336,   337,   338,   339,   340,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,   356,   357,   358,   359,   360,   361,   362,   363,
     364,   365,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    76,     0,   467,   367,     0,   368,
     704,   369,   370,   371,   372,   373,   374,   375,   376,   377,
     378,   379,   380,   381,   382,   383,     0,   384,   385,   300,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
     311,   312,   313,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,   314,   315,   316,   317,
     318,   319,   320,   321,   322,   323,    28,    29,     0,   324,
       0,   325,   326,   327,   328,   329,   330,   331,   332,   333,
       0,   334,   335,   336,   337,   338,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,   356,   357,   358,   359,   360,   361,   362,
     363,   364,   365,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    76,     0,   467,   367,     0,
     368,   705,   369,   370,   371,   372,   373,   374,   375,   376,
     377,   378,   379,   380,   381,   382,   383,     0,   384,   385,
     300,   301,   302,   303,   304,   305,   306,   307,   308,   309,
     310,   311,   312,   313,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,   314,   315,   316,
     317,   318,   319,   320,   321,   322,   323,    28,    29,     0,
     324,     0,   325,   326,   327,   328,   329,   330,   331,   332,
     333,     0,   334,   335,   336,   337,   338,   339,   340,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,   356,   357,   358,   359,   360,   361,
     362,   363,   364,   365,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    76,     0,   467,   367,
     945,   368,     0,   369,   370,   371,   372,   373,   374,   375,
     376,   377,   378,   379,   380,   381,   382,   383,     0,   384,
     385,   300,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,   311,   312,   313,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,   314,   315,
     316,   317,   318,   319,   320,   321,   322,   323,    28,    29,
       0,   324,     0,   325,   326,   327,   328,   329,   330,   331,
     332,   333,     0,   334,   335,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,   356,   357,   358,   359,   360,
     361,   362,   363,   364,   365,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    76,     0,   467,
     367,   976,   368,     0,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,     0,
     384,   385,   300,   301,   302,   303,   304,   305,   306,   307,
     308,   309,   310,   311,   312,   313,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,    28,
      29,     0,   324,     0,   325,   326,   327,   328,   329,   330,
     331,   332,   333,     0,   334,   335,   336,   337,   338,   339,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,   355,   356,   357,   358,   359,
     360,   361,   362,   363,   364,   365,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    76,     0,
     467,   367,     0,   368,   980,   369,   370,   371,   372,   373,
     374,   375,   376,   377,   378,   379,   380,   381,   382,   383,
       0,   384,   385,   300,   301,   302,   303,   304,   305,   306,
     307,   308,   309,   310,   311,   312,   313,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
     314,   315,   316,   317,   318,   319,   320,   321,   322,   323,
      28,    29,     0,   324,     0,   325,   326,   327,   328,   329,
     330,   331,   332,   333,     0,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    76,
       0,   366,   367,     0,   368,     0,   369,   370,   371,   372,
     373,   374,   375,   376,   377,   378,   379,   380,   381,   382,
     383,     0,   384,   385,   300,   301,   302,   303,   304,   305,
     306,   307,   308,   309,   310,   311,   312,   313,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,   314,   315,   316,   317,   318,   319,   320,   321,   322,
     323,    28,    29,     0,   324,     0,   325,   326,   327,   328,
     329,   330,   331,   332,   333,     0,   334,   335,   336,   337,
     338,   339,   340,   341,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,   355,   356,   357,
     358,   359,   360,   361,   362,   363,   364,   365,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      76,     0,   660,   367,     0,   368,     0,   369,   370,   371,
     372,   373,   374,   375,   376,   377,   378,   379,   380,   381,
     382,   383,     0,   384,   385,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,   314,   315,   316,   317,   318,   319,   320,   321,
     322,   323,    28,    29,     0,   324,     0,   325,   326,   327,
     328,   329,   330,   331,   332,   333,     0,   334,   335,   336,
     337,   338,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,   356,
     357,   358,   359,   360,   361,   362,   363,   364,   365,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    76,     0,   691,   367,     0,   368,     0,   369,   370,
     371,   372,   373,   374,   375,   376,   377,   378,   379,   380,
     381,   382,   383,     0,   384,   385,   300,   301,   302,   303,
     304,   305,   306,   307,   308,   309,   310,   311,   312,   313,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,   314,   315,   316,   317,   318,   319,   320,
     321,   322,   323,    28,    29,     0,   324,     0,   325,   326,
     327,   328,   329,   330,   331,   332,   333,     0,   334,   335,
     336,   337,   338,   339,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,   362,   363,   364,   365,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    76,     0,   769,   367,     0,   368,     0,   369,
     370,   371,   372,   373,   374,   375,   376,   377,   378,   379,
     380,   381,   382,   383,     0,   384,   385,   300,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,   311,   312,
     313,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,   314,   315,   316,   317,   318,   319,
     320,   321,   322,   323,    28,    29,     0,   324,     0,   325,
     326,   327,   328,   329,   330,   331,   332,   333,     0,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    76,     0,   861,   367,     0,   368,     0,
     369,   370,   371,   372,   373,   374,   375,   376,   377,   378,
     379,   380,   381,   382,   383,     0,   384,   385,   300,   301,
     302,   303,   304,   305,   306,   307,   308,   309,   310,   311,
     312,   313,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,   314,   315,   316,   317,   318,
     319,   320,   321,   322,   323,    28,    29,     0,   324,     0,
     325,   326,   327,   328,   329,   330,   331,   332,   333,     0,
     334,   335,   336,   337,   338,   339,   340,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,   356,   357,   358,   359,   360,   361,   362,   363,
     364,   365,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    76,     0,   923,   367,     0,   368,
       0,   369,   370,   371,   372,   373,   374,   375,   376,   377,
     378,   379,   380,   381,   382,   383,     0,   384,   385,   300,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
     311,   312,   313,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,   314,   315,   316,   317,
     318,   319,   320,   321,   322,   323,    28,    29,     0,   324,
       0,   325,   326,   327,   328,   329,   330,   331,   332,   333,
       0,   334,   335,   336,   337,   338,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,   356,   357,   358,   359,   360,   361,   362,
     363,   364,   365,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    76,     0,     0,   367,   968,
     368,     0,   369,   370,   371,   372,   373,   374,   375,   376,
     377,   378,   379,   380,   381,   382,   383,     0,   384,   385,
     300,   301,   302,   303,   304,   305,   306,   307,   308,   309,
     310,   311,   312,   313,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,   314,   315,   316,
     317,   318,   319,   320,   321,   322,   323,    28,    29,     0,
     324,     0,   325,   326,   327,   328,   329,   330,   331,   332,
     333,     0,   334,   335,   336,   337,   338,   339,   340,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,   356,   357,   358,   359,   360,   361,
     362,   363,   364,   365,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,     0,     0,   633,   634,   635,
     636,   637,   638,   639,   640,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    76,     0,   981,   367,
       0,   368,     0,   369,   370,   371,   372,   373,   374,   375,
     376,   377,   378,   379,   380,   381,   382,   383,   641,   384,
     385,   821,   822,   823,   824,   825,   826,   827,     0,     0,
       0,     0,     0,     0,     0,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,   828,   829,
      23,    24,   830,     0,    26,     0,     0,    27,    28,    29,
     831,   832,    31,    32,    33,    34,     0,    36,   833,    38,
     148,    40,   834,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     642,     0,     0,     0,     0,     0,     0,     0,     0,   644,
     645,     0,     0,   835,   836,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,   837,   838,
     902,   822,     0,     0,     0,   826,   827,     0,     0,     0,
       0,   839,     0,     0,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,   828,   829,   127,
     128,   292,     0,    26,     0,     0,    27,    28,    29,     0,
     832,    31,     0,    33,     0,     0,     0,   833,   147,   148,
       0,     0,   135,   136,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   293,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   835,   836,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,   124,   125,     0,     0,     0,
     957,   827,     0,     0,     0,     0,     0,   138,   139,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,   828,   829,   127,   128,     0,     0,    26,     0,
     839,     0,    28,    29,     0,   832,    31,     0,    33,     0,
       0,     0,   833,   147,   148,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   835,   836,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
     124,   125,     0,     0,     0,     0,   126,     0,     0,     0,
       0,     0,     0,     0,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,   127,
     128,     0,     0,    26,     0,   839,     0,    28,    29,     0,
     146,    31,     0,    33,     0,     0,     0,   129,   147,   148,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,   124,   125,     0,     0,     0,
       0,   827,     0,     0,     0,     0,     0,     0,     0,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,   828,   829,   127,   128,     0,     0,    26,     0,
     899,     0,    28,    29,     0,     0,     0,     0,    33,     0,
       0,     0,   833,   147,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   835,   836,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
     124,   125,     0,     0,     0,     0,   126,     0,     0,     0,
       0,     0,     0,     0,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,   127,
     128,     0,     0,    26,     0,   907,     0,    28,    29,     0,
       0,     0,     0,    33,     0,     0,     0,   129,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     4,     5,
       0,     0,     0,     6,     7,     0,     0,     0,     0,     0,
       0,   396,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
       0,    26,     0,     0,    27,    28,    29,     0,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,     0,    77,   205,     5,     0,
       0,     0,     6,   126,     0,     0,     0,     0,     0,     0,
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,   127,   128,     0,     0,
      26,     0,     0,    27,    28,    29,     0,   146,    31,     0,
      33,     0,     0,     0,   129,   147,   148,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,   124,   125,     0,     0,     0,     0,   126,     0,
       0,     0,     0,     0,     0,     0,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,   127,   128,     0,     0,    26,     0,     0,     0,    28,
      29,     0,   146,    31,     0,    33,     0,     0,     0,   129,
     147,   148,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,   124,   125,     0,
       0,     0,     0,   126,     0,     0,     0,     0,     0,     0,
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,   127,   128,     0,     0,
      26,     0,     0,     0,    28,    29,     0,     0,     0,     0,
      33,     0,     0,     0,   129,     0,     0,   778,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   779,     0,     0,     0,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,   124,   125,     0,     0,     0,     0,   126,     0,
       0,     0,     0,     0,     0,     0,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,   127,   128,     0,     0,    26,     0,     0,     0,    28,
      29,     0,     0,     0,     0,    33,     0,     0,     0,   129,
     147,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,   158,   159,     0,
       0,     0,     0,   126,     0,     0,     0,     0,     0,     0,
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,     0,     0,
      26,     0,     0,     0,    28,    29,     0,     0,     0,     0,
      33,     0,     0,     0,   129,     0,     0,   160,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,   124,   125,     0,     0,     0,     0,   126,   180,
       0,     0,     0,     0,     0,     0,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,   127,   128,     0,     0,    26,   124,   125,     0,    28,
      29,     0,   126,   272,     0,    33,     0,     0,     0,   129,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,   127,   128,     0,     0,    26,
       0,     0,     0,    28,    29,     0,     0,     0,     0,    33,
       0,     0,     0,   129,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,   124,   125,     0,     0,     0,     0,   126,     0,     0,
       0,     0,     0,     0,     0,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
     127,   128,     0,     0,    26,   124,   125,     0,    28,    29,
       0,   126,     0,     0,    33,     0,     0,     0,   129,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,   127,   128,     0,     0,     0,     0,
       0,     0,    28,    29,     0,     0,     0,     0,    33,     0,
       0,     0,   129,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
     124,   125,     0,     0,     0,     0,   254,     0,     0,     0,
       0,     0,     0,     0,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,   255,   256,   127,
     128,     0,     0,     0,     0,   489,     0,    28,    29,     0,
     254,     0,     0,     0,     0,     0,     0,   257,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,   255,   256,     0,     0,     0,     0,     0,     0,     0,
       0,    28,    29,     0,     0,     0,     0,   490,     0,     0,
       0,   257,   258,   259,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   258,   259,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54
};

static const yytype_int16 yycheck[] =
{
       4,     5,   175,   100,     6,    30,     3,   130,     4,     5,
     166,     3,    89,   226,    34,    34,    34,    34,   253,    23,
      24,   132,    34,   296,   175,   247,   468,   157,    30,    31,
      27,   279,    34,   144,   486,   295,   555,    39,   437,   650,
       3,   483,    91,    92,    93,    94,   493,   734,   574,   233,
     735,   736,   741,   157,   703,   277,    37,   482,   495,   783,
     252,   396,   173,   607,     9,   176,    82,    30,   757,     0,
     179,   831,    35,    82,    53,   267,    89,    36,   767,    38,
     126,    82,    82,     9,    82,    30,    31,    82,   831,     9,
       9,   851,    89,    97,    82,    82,    98,    82,   249,    43,
     211,   123,    81,   226,    36,    50,   122,   129,   851,    53,
     126,    30,    31,   122,    37,   266,   246,   126,    37,   270,
      82,   146,   231,   121,    50,   126,   126,   122,   124,   125,
      50,    50,    82,   121,    97,   122,   233,   122,   657,   581,
      85,    86,   246,   247,   146,   122,   148,   298,   872,   909,
     152,   588,   577,   134,   158,   159,   137,   122,   207,   208,
     271,   265,   158,   159,    83,    84,    85,    86,   712,   504,
     183,   130,   122,   277,   863,   140,   122,    82,   180,   124,
     184,    82,   150,   151,   188,   189,   183,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,   201,   202,   203,
     204,   205,   394,   122,   928,   209,   658,    82,   122,   205,
     157,   134,   731,   122,   137,   134,   121,   122,   137,    82,
     121,   126,   226,   122,   121,   744,   140,    82,   670,   121,
     879,    17,   129,    19,    20,    21,    22,   129,   122,    25,
       9,   140,   393,   247,   122,   121,   121,   699,   250,   122,
     252,   362,   363,   129,   122,   122,   367,   368,   121,   126,
     444,    30,    31,   226,   268,   267,   121,   122,    37,   122,
     272,   126,   383,   277,     9,   426,   925,   121,   126,   122,
     124,    50,   121,   970,   247,   124,   122,   126,   122,   128,
     294,   295,   811,   978,   820,    30,    31,   122,     9,   246,
     247,    87,   122,     9,    39,     9,   753,    11,    12,    13,
      14,    15,    16,   122,   277,    50,    85,    86,   265,    30,
      31,   594,   122,   596,    30,    31,   768,   122,   122,   121,
     277,    37,   124,   484,   860,   122,   128,   122,   122,    50,
     562,   122,   121,   473,    50,   124,    50,   126,   122,   128,
      85,    86,    39,   126,   505,   506,   507,   508,   509,    82,
     124,   124,   124,   585,   556,   134,   977,   124,   137,   473,
      43,   770,     9,   892,    85,    86,   122,    83,    84,    85,
      86,    82,   127,   121,     9,   496,   497,   839,    82,     9,
     563,    82,   394,    30,    31,   128,    82,   401,   133,   403,
     404,   405,    82,   523,   408,    30,    31,   580,    82,   130,
      30,    31,   563,    50,    82,   140,   121,   514,    82,   124,
     123,   126,   133,   128,   124,    50,   124,   662,   134,   580,
      50,   137,   124,   668,   124,   123,   566,   567,   568,   123,
     882,   592,   593,   140,   704,   705,   129,   899,    85,    86,
     129,     9,   129,   701,     9,   907,   460,   129,   562,   129,
      85,    86,   566,   567,   568,    85,    86,   129,   129,   661,
     129,   129,    30,    31,   578,    30,    31,   129,   482,   129,
     129,   585,   486,   129,    39,   129,   129,   124,   123,   140,
     494,   495,    50,   613,   614,    50,   500,   617,   618,   619,
     620,   621,   622,   623,   624,   625,   626,   949,   133,   141,
      82,   123,   122,   133,     5,     6,     7,    82,     9,   692,
     129,   127,     9,   124,   127,   124,   473,    85,    86,   127,
      85,    86,   536,   125,   129,   123,   666,   130,    82,    30,
      31,   692,   127,    30,    31,   129,    45,     9,   123,     9,
     123,    11,    12,    13,   556,    15,    16,   121,   562,    50,
     127,   129,   666,    50,   125,     9,   124,   123,    30,    31,
      30,    31,   123,   577,   578,   121,   129,    39,   124,   141,
     126,   585,   128,   756,   588,   122,    30,    31,    50,   141,
      50,    16,     9,    16,    85,    86,   122,   717,    85,    86,
     873,   140,   140,   128,   125,   756,    50,   609,     9,   129,
     832,   123,   125,    30,    31,   562,   772,   123,   126,   566,
     567,   568,    39,    85,    86,    85,    86,   783,    10,    30,
      31,   578,   127,    50,     9,   123,   125,   741,   585,    83,
      84,    85,    86,   121,   857,   125,   124,   129,   126,    50,
     128,   655,   121,   757,   658,    30,    31,   121,   123,   661,
     123,   129,   122,   767,    39,   123,    83,    84,    85,    86,
     123,   131,   132,   133,   123,    50,   753,   123,   122,   129,
     123,     9,    83,    84,    85,    86,   123,   900,   123,   123,
     123,   122,   865,   906,   122,   699,   698,   129,   124,   123,
     704,   705,    30,    31,   706,   124,   708,   818,    83,    84,
      85,    86,   714,   121,   865,   121,   872,   129,   123,   666,
     123,    11,    50,     9,    10,    11,    12,    13,    14,    15,
      16,   129,    11,   753,   753,   753,   753,   741,   123,   123,
     121,   753,   141,   124,   895,   126,   959,   128,   122,   753,
     123,   122,   128,   757,   831,     9,   753,    85,    86,   863,
     123,   753,   935,   767,    50,     9,   121,     9,   121,   123,
     127,   123,   928,   123,   851,   872,    30,    31,   901,   123,
     129,   123,   125,   122,   935,   125,    30,    31,    30,    31,
     123,   125,   123,   125,   741,    89,    50,   123,   121,   910,
     121,   974,   666,   852,   853,   854,    50,   832,    50,   960,
     757,   123,   816,   753,   898,   753,   701,   821,   822,   894,
     767,   708,   973,   974,   826,   821,   822,   831,   832,   247,
     832,    85,    86,   816,   831,   839,   122,   948,   124,    83,
      84,    85,    86,    85,    86,   131,   132,   851,   931,   753,
     977,   805,   867,   857,   851,   904,   905,   102,   101,   863,
     753,   733,   242,    -1,   220,   914,   915,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    -1,    -1,   899,   900,    -1,   902,    -1,
      -1,    -1,   906,   907,    -1,   909,   902,    -1,    -1,    -1,
      -1,    -1,   909,    -1,   131,   132,   863,   134,   135,   136,
     137,   138,   139,    -1,    -1,    -1,    -1,    -1,    -1,   931,
      -1,    -1,    -1,    -1,   122,    -1,    -1,    -1,   126,   127,
      -1,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,    -1,   142,   959,     3,     4,     5,     6,
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
      -1,    -1,    -1,   119,    -1,   121,   122,   123,   124,    -1,
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
      -1,    -1,    -1,    -1,   119,    -1,   121,   122,    -1,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
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
     122,    -1,   124,   125,   126,   127,   128,   129,   130,   131,
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
     121,   122,    -1,   124,   125,   126,   127,   128,   129,   130,
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
     119,    -1,   121,   122,   123,   124,    -1,   126,   127,   128,
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
      -1,    -1,    -1,   119,    -1,   121,   122,    -1,   124,    -1,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,     3,     4,
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
      -1,    -1,    -1,    -1,   119,    -1,   121,   122,    -1,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
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
     121,   122,    -1,   124,   125,   126,   127,   128,   129,   130,
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
      -1,   121,   122,    -1,   124,    -1,   126,   127,   128,   129,
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
     119,    -1,   121,   122,    -1,   124,    -1,   126,   127,   128,
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
      -1,    -1,   119,    -1,   121,   122,    -1,   124,    -1,   126,
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
      -1,    -1,    -1,   119,    -1,   121,   122,    -1,   124,    -1,
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
      -1,    -1,    -1,    -1,   119,    -1,   121,   122,    -1,   124,
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
      -1,    -1,    -1,    -1,    -1,   119,    -1,    -1,   122,   123,
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
      93,    94,    95,    96,    97,    -1,    -1,     9,    10,    11,
      12,    13,    14,    15,    16,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   119,    -1,   121,   122,
      -1,   124,    -1,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,    50,   142,
     143,     3,     4,     5,     6,     7,     8,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    -1,    36,    -1,    -1,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    -1,    49,    50,    51,
      52,    53,    54,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     122,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   131,
     132,    -1,    -1,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
       3,     4,    -1,    -1,    -1,     8,     9,    -1,    -1,    -1,
      -1,   133,    -1,    -1,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,     9,    -1,    36,    -1,    -1,    39,    40,    41,    -1,
      43,    44,    -1,    46,    -1,    -1,    -1,    50,    51,    52,
      -1,    -1,    30,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    50,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,     3,     4,    -1,    -1,    -1,
       8,     9,    -1,    -1,    -1,    -1,    -1,    85,    86,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    -1,    -1,    36,    -1,
     133,    -1,    40,    41,    -1,    43,    44,    -1,    46,    -1,
      -1,    -1,    50,    51,    52,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       3,     4,    -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    -1,    -1,    36,    -1,   133,    -1,    40,    41,    -1,
      43,    44,    -1,    46,    -1,    -1,    -1,    50,    51,    52,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,     3,     4,    -1,    -1,    -1,
      -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    -1,    -1,    36,    -1,
     133,    -1,    40,    41,    -1,    -1,    -1,    -1,    46,    -1,
      -1,    -1,    50,    51,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       3,     4,    -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    -1,    -1,    36,    -1,   133,    -1,    40,    41,    -1,
      -1,    -1,    -1,    46,    -1,    -1,    -1,    50,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,
      -1,    -1,    -1,     8,     9,    -1,    -1,    -1,    -1,    -1,
      -1,   124,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      -1,    36,    -1,    -1,    39,    40,    41,    -1,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,    -1,   121,     3,     4,    -1,
      -1,    -1,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    -1,    -1,
      36,    -1,    -1,    39,    40,    41,    -1,    43,    44,    -1,
      46,    -1,    -1,    -1,    50,    51,    52,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,     3,     4,    -1,    -1,    -1,    -1,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    -1,    -1,    36,    -1,    -1,    -1,    40,
      41,    -1,    43,    44,    -1,    46,    -1,    -1,    -1,    50,
      51,    52,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,     3,     4,    -1,
      -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    -1,    -1,
      36,    -1,    -1,    -1,    40,    41,    -1,    -1,    -1,    -1,
      46,    -1,    -1,    -1,    50,    -1,    -1,    53,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    81,    -1,    -1,    -1,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,     3,     4,    -1,    -1,    -1,    -1,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    -1,    -1,    36,    -1,    -1,    -1,    40,
      41,    -1,    -1,    -1,    -1,    46,    -1,    -1,    -1,    50,
      51,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,     3,     4,    -1,
      -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    -1,    -1,
      36,    -1,    -1,    -1,    40,    41,    -1,    -1,    -1,    -1,
      46,    -1,    -1,    -1,    50,    -1,    -1,    53,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,     3,     4,    -1,    -1,    -1,    -1,     9,    10,
      -1,    -1,    -1,    -1,    -1,    -1,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    -1,    -1,    36,     3,     4,    -1,    40,
      41,    -1,     9,    10,    -1,    46,    -1,    -1,    -1,    50,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    -1,    -1,    36,
      -1,    -1,    -1,    40,    41,    -1,    -1,    -1,    -1,    46,
      -1,    -1,    -1,    50,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,     3,     4,    -1,    -1,    -1,    -1,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    -1,    -1,    36,     3,     4,    -1,    40,    41,
      -1,     9,    -1,    -1,    46,    -1,    -1,    -1,    50,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    -1,    -1,    -1,    -1,
      -1,    -1,    40,    41,    -1,    -1,    -1,    -1,    46,    -1,
      -1,    -1,    50,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       3,     4,    -1,    -1,    -1,    -1,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    -1,    -1,    -1,    -1,     4,    -1,    40,    41,    -1,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    50,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    40,    41,    -1,    -1,    -1,    -1,    46,    -1,    -1,
      -1,    50,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97
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
     149,   151,   163,   166,   179,   180,   181,   182,   186,   187,
     196,   198,   199,   202,   203,   204,   245,   272,   273,   274,
     275,   276,   277,   279,   285,   286,   290,   291,   292,   293,
     296,   310,   335,     9,    30,    31,    50,    85,    86,   124,
     271,   292,   271,   292,     3,     4,     9,    32,    33,    50,
     274,   286,   122,   126,     9,    30,    31,    50,    85,    86,
     167,   271,     9,    50,   124,   271,    43,    51,    52,   272,
     295,   294,   273,   274,   286,   274,   126,   278,     3,     4,
      53,   163,   166,   179,   180,   181,   274,     9,    30,    31,
      50,    85,    86,   124,   286,   337,   122,   126,    43,    53,
      10,   274,   247,   122,   122,   122,   122,   122,   122,   122,
     122,   122,   122,   122,   122,   122,   122,   122,   122,   122,
     122,   122,   122,   122,   122,     3,   151,   198,   203,   272,
     121,   124,   215,   215,   215,   215,   215,     9,    30,    31,
      39,    50,    83,    84,    85,    86,   200,   207,   209,   212,
     216,   246,   254,   257,   263,   271,   286,   274,    37,   134,
     137,   287,   288,   289,   277,   276,    82,    82,   121,   336,
     126,   154,   126,   152,     9,    30,    31,    50,    85,    86,
       9,    50,     9,    50,   207,    82,   336,   281,   124,   164,
     336,   124,    10,   122,   296,   296,   274,    82,   127,   188,
       9,    30,    31,    50,    85,    86,   279,   284,   285,   271,
     271,   121,     9,    50,    83,    84,   263,   271,   336,   150,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    43,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,   121,   122,   124,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   142,   143,   293,   334,   335,   339,
     340,   341,   342,   336,   280,   247,   124,   274,   121,   129,
     198,   272,   271,   312,   315,   316,   271,   271,   320,   271,
     271,   271,   271,   271,   271,   271,   271,   271,   271,   271,
     271,   271,   271,   215,   215,   271,   336,    55,    56,   122,
     140,   333,   334,   207,   212,   286,   208,   213,   247,   130,
     242,   243,   254,   261,   287,   264,   265,   266,   122,   126,
      82,   137,   289,   284,     9,    30,    31,    50,    85,    86,
     133,   204,   220,   222,   271,   284,   286,   121,   125,   338,
     339,   274,   282,   128,   159,   282,   159,   123,   282,   169,
     170,   271,   124,   125,   336,   205,   133,   220,   286,     4,
      46,   189,   191,   192,   193,   291,   124,   124,   241,   271,
     287,   241,   264,   125,   124,   336,   336,   336,   336,   336,
     123,   282,   121,   145,   248,   123,   129,   271,   271,   271,
     129,   129,   271,   129,   129,   129,   129,   129,   129,   129,
     129,   129,   129,   129,   123,   123,   129,   125,   140,   140,
     123,   141,   210,    82,    36,    38,   130,   214,   214,   121,
     244,   123,   254,   267,   269,   217,   218,   227,   228,   271,
     221,   122,    82,   337,   129,   127,     5,     6,     7,   160,
     161,   284,   124,   127,   124,   127,   125,   129,   130,   169,
     337,   125,   234,   235,   227,    82,   127,   129,   187,   194,
     271,   194,   336,   336,   123,   271,   123,   121,   145,   123,
     123,   123,   125,   141,   127,   125,   249,   250,   287,   311,
     129,   123,   123,   317,   319,   129,   290,   323,   325,   327,
     329,   324,   326,   328,   330,   331,   332,   271,   141,   141,
     122,    16,    16,     9,    10,    11,    12,    13,    14,    15,
      16,    50,   122,   124,   131,   132,   297,   302,   308,   309,
     255,   140,   234,   282,   122,   128,   224,   223,   133,   220,
     121,   283,   155,   284,   284,   284,   129,   156,   153,   156,
     168,   169,     9,    11,    12,    13,    15,    16,    50,   122,
     131,   132,   133,   171,   172,   173,   177,   271,   279,   285,
     125,   121,   337,   123,    53,    81,   236,   238,   239,   133,
     220,   190,   194,   195,   125,   125,   122,   264,   122,   264,
     125,   246,   251,   274,   313,   290,   290,   321,   123,   290,
     290,   290,   290,   290,   290,   290,   290,   290,   290,   123,
     129,   211,   126,   305,   298,   304,   303,    10,   122,   258,
     265,   268,   123,   127,   229,   226,   271,   234,   227,   282,
     159,   160,   125,   157,   159,   125,   337,   176,   131,   132,
     134,   135,   136,   137,   138,   139,   178,   174,   165,   121,
     206,   129,   274,   227,   189,   242,   241,   241,    53,    81,
     230,   231,   233,   274,   121,   230,   121,   246,   123,   274,
     318,   123,   290,   123,   123,   123,   123,   123,   123,   123,
     123,   129,   129,   123,   234,   306,   302,   297,     9,   309,
     309,   259,   173,   270,   122,   234,   129,   225,   122,   123,
     124,     3,     4,     5,     6,     7,     8,     9,    30,    31,
      34,    42,    43,    50,    54,    85,    86,   120,   121,   133,
     158,   162,   163,   166,   179,   180,   181,   182,   183,   184,
     186,   187,   197,   201,   203,   220,   245,   272,   310,   335,
     124,   121,   173,   175,   173,   337,   214,   237,    83,    84,
     122,   252,   256,   262,   263,   121,   121,   123,   129,   252,
     123,   314,   129,   322,    11,    11,   123,   275,   123,   300,
     234,   141,   219,   123,   226,   336,   156,   271,   271,   133,
     272,   274,     3,   184,   201,   203,   272,   133,   220,   122,
     122,   227,   128,   184,   201,   203,   215,   215,   215,   212,
     156,   123,   173,   121,   236,   240,   252,   260,   287,   264,
     121,   232,   242,   121,   123,   337,   123,   123,   123,   127,
     129,   299,   123,   234,   225,   123,   125,   121,   124,   128,
     185,   185,   227,   212,   215,   215,   227,     8,   201,   272,
     336,   215,   215,   125,   242,   123,   252,   231,   123,   122,
     301,   125,   123,   336,   337,   123,   123,   253,   307,   297,
     125,   121,   121,   258,   309,   121,   123
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
#line 910 "vtkParse.y"
    { startSig(); clearTypeId(); clearTemplate(); }
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 927 "vtkParse.y"
    { output_function(); }
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 928 "vtkParse.y"
    { output_function(); }
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 929 "vtkParse.y"
    { reject_function(); }
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 930 "vtkParse.y"
    { output_function(); }
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 931 "vtkParse.y"
    { reject_function(); }
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 932 "vtkParse.y"
    { output_function(); }
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 933 "vtkParse.y"
    { legacySig(); output_function(); }
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 951 "vtkParse.y"
    { pushNamespace((yyvsp[(2) - (2)].str)); }
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 952 "vtkParse.y"
    { popNamespace(); }
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 959 "vtkParse.y"
    { start_class((yyvsp[(2) - (2)].str), 0); }
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 960 "vtkParse.y"
    { end_class(); }
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 961 "vtkParse.y"
    { reject_class((yyvsp[(2) - (5)].str), 0); }
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 962 "vtkParse.y"
    { end_class(); }
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 963 "vtkParse.y"
    { start_class((yyvsp[(2) - (2)].str), 1); }
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 964 "vtkParse.y"
    { end_class(); }
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 965 "vtkParse.y"
    { reject_class((yyvsp[(2) - (5)].str), 1); }
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 966 "vtkParse.y"
    { end_class(); }
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 969 "vtkParse.y"
    { startSig(); clearTypeId(); clearTemplate(); }
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 985 "vtkParse.y"
    { output_function(); }
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 986 "vtkParse.y"
    { ClassInfo *tmpc = currentClass;
     currentClass = NULL; output_function(); currentClass = tmpc; }
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 988 "vtkParse.y"
    { output_function(); }
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 989 "vtkParse.y"
    { output_function(); }
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 990 "vtkParse.y"
    { ClassInfo *tmpc = currentClass;
     currentClass = NULL; output_function(); currentClass = tmpc; }
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 992 "vtkParse.y"
    { output_function(); }
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 993 "vtkParse.y"
    { legacySig(); output_function(); }
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 1007 "vtkParse.y"
    {
      vtkParse_AddPointerToArray(&currentClass->SuperClasses,
                                 &currentClass->NumberOfSuperClasses,
                                 vtkstrdup((yyvsp[(2) - (2)].str)));
    }
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 1013 "vtkParse.y"
    {access_level = VTK_ACCESS_PUBLIC;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 1014 "vtkParse.y"
    {access_level = VTK_ACCESS_PROTECTED;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 1015 "vtkParse.y"
    {access_level = VTK_ACCESS_PRIVATE;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 1025 "vtkParse.y"
    {start_enum((yyvsp[(2) - (2)].str));}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 1026 "vtkParse.y"
    {end_enum();}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 1028 "vtkParse.y"
    {start_enum(NULL);}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 1029 "vtkParse.y"
    {end_enum();}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 1033 "vtkParse.y"
    {add_enum((yyvsp[(1) - (1)].str), NULL);}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 1034 "vtkParse.y"
    {add_enum((yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 1036 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 1041 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 1042 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 1043 "vtkParse.y"
    {
         (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (3)].str)) + strlen((yyvsp[(3) - (3)].str)) + 1);
         sprintf((yyval.str), "%s%s", (yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));
       }
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 1047 "vtkParse.y"
    {postSig((yyvsp[(2) - (2)].str));}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 1048 "vtkParse.y"
    {
         (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (4)].str)) + strlen((yyvsp[(2) - (4)].str)) +
                                  strlen((yyvsp[(4) - (4)].str)) + 3);
         sprintf((yyval.str), "%s %s %s", (yyvsp[(1) - (4)].str), (yyvsp[(2) - (4)].str), (yyvsp[(4) - (4)].str));
       }
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 1053 "vtkParse.y"
    {postSig("(");}
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 1054 "vtkParse.y"
    {
         postSig(")");
         (yyval.str) = (char *)malloc(strlen((yyvsp[(3) - (4)].str)) + 3);
         sprintf((yyval.str), "(%s)", (yyvsp[(3) - (4)].str));
       }
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 1060 "vtkParse.y"
    { (yyval.str) = "-"; }
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 1060 "vtkParse.y"
    { (yyval.str) = "+"; }
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 1061 "vtkParse.y"
    { (yyval.str) = "~"; }
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 1063 "vtkParse.y"
    { (yyval.str) = "-"; }
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 1063 "vtkParse.y"
    { (yyval.str) = "+"; }
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 1064 "vtkParse.y"
    { (yyval.str) = "*"; }
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 1064 "vtkParse.y"
    { (yyval.str) = "/"; }
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 1065 "vtkParse.y"
    { (yyval.str) = "%"; }
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 1065 "vtkParse.y"
    { (yyval.str) = "&"; }
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 1066 "vtkParse.y"
    { (yyval.str) = "|"; }
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 1066 "vtkParse.y"
    { (yyval.str) = "^"; }
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 1107 "vtkParse.y"
    { postSig("template<> "); clearTypeId(); }
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 1108 "vtkParse.y"
    { postSig("template<");
          clearTypeId(); startTemplate(); }
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 1110 "vtkParse.y"
    { postSig("> "); clearTypeId(); }
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 1113 "vtkParse.y"
    { postSig(", "); clearTypeId(); }
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 1117 "vtkParse.y"
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

  case 149:

/* Line 1455 of yacc.c  */
#line 1127 "vtkParse.y"
    {
               TemplateArg *arg = (TemplateArg *)malloc(sizeof(TemplateArg));
               vtkParse_InitTemplateArg(arg);
               arg->Name = vtkstrdup(getVarName());
               arg->Value = vtkstrdup(getVarValue());
               vtkParse_AddItemMacro2(currentTemplate, Arguments, arg);
               }
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 1134 "vtkParse.y"
    { pushTemplate(); }
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 1135 "vtkParse.y"
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

  case 152:

/* Line 1455 of yacc.c  */
#line 1146 "vtkParse.y"
    {postSig("class ");}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 1147 "vtkParse.y"
    {postSig("typename ");}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1149 "vtkParse.y"
    { setVarName((yyvsp[(1) - (1)].str)); }
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1179 "vtkParse.y"
    {openSig(); preSig("~"); closeSig();}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1180 "vtkParse.y"
    {openSig(); preSig("~"); closeSig();}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1182 "vtkParse.y"
    {
         openSig(); preSig("virtual ~"); closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 1190 "vtkParse.y"
    {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 1204 "vtkParse.y"
    {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->IsVirtual = 1;
         }
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 1213 "vtkParse.y"
    {
      postSig("(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 1217 "vtkParse.y"
    { postSig(")"); }
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 1218 "vtkParse.y"
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

  case 186:

/* Line 1455 of yacc.c  */
#line 1232 "vtkParse.y"
    { postSig(")"); }
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 1233 "vtkParse.y"
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

  case 188:

/* Line 1455 of yacc.c  */
#line 1244 "vtkParse.y"
    {postSig((yyvsp[(2) - (2)].str));}
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 1245 "vtkParse.y"
    {
      postSig("(");
      currentFunction->IsOperator = 1;
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 1250 "vtkParse.y"
    { (yyval.str) = (yyvsp[(2) - (7)].str); }
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 1252 "vtkParse.y"
    { postSig(")"); }
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 1253 "vtkParse.y"
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

  case 194:

/* Line 1455 of yacc.c  */
#line 1266 "vtkParse.y"
    {
      postSig(" = 0");
      if (currentClass)
        {
        currentFunction->IsPureVirtual = 1;
        currentClass->IsAbstract = 1;
        }
    }
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 1275 "vtkParse.y"
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

  case 196:

/* Line 1455 of yacc.c  */
#line 1285 "vtkParse.y"
    {
      postSig(" const");
      currentFunction->IsConst = 1;
    }
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 1293 "vtkParse.y"
    {
      postSig("(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
    }
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 1296 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (5)].str); }
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 1297 "vtkParse.y"
    {markSig(); postSig("<");}
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 1298 "vtkParse.y"
    {
      const char *cp;
      postSig(">(");
      set_return(currentFunction, getStorageType(), getTypeId(), 0);
      cp = copySig();
      (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (6)].str)) + strlen(cp) + 1);
      sprintf((yyval.str), "%s%s", (yyvsp[(1) - (6)].str), cp);
    }
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 1305 "vtkParse.y"
    { (yyval.str) = (yyvsp[(7) - (9)].str); }
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 1307 "vtkParse.y"
    { postSig(");"); closeSig(); }
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 1308 "vtkParse.y"
    {
      currentFunction->Name = (yyvsp[(1) - (3)].str);
      if (HaveComment)
        {
        currentFunction->Comment = vtkstrdup(CommentText);
        }
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 1317 "vtkParse.y"
    { postSig("("); }
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 1326 "vtkParse.y"
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

  case 214:

/* Line 1455 of yacc.c  */
#line 1339 "vtkParse.y"
    { postSig("(");}
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 1347 "vtkParse.y"
    { postSig("..."); }
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 1348 "vtkParse.y"
    { postSig(", "); }
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 1353 "vtkParse.y"
    { postSig("void (*"); postSig((yyvsp[(1) - (1)].str)); postSig(")(void *) "); }
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 1356 "vtkParse.y"
    {clearTypeId();}
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 1359 "vtkParse.y"
    { currentFunction->IsVariadic = 1; postSig("..."); }
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 1360 "vtkParse.y"
    { clearTypeId(); }
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 1361 "vtkParse.y"
    { clearTypeId(); postSig(", "); }
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 1364 "vtkParse.y"
    { markSig(); }
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 1366 "vtkParse.y"
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

  case 233:

/* Line 1455 of yacc.c  */
#line 1388 "vtkParse.y"
    {
      int i = currentFunction->NumberOfArguments-1;
      if (getVarValue())
        {
        currentFunction->Arguments[i]->Value = vtkstrdup(getVarValue());
        }
    }
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 1396 "vtkParse.y"
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

  case 237:

/* Line 1455 of yacc.c  */
#line 1421 "vtkParse.y"
    {clearVarValue();}
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 1423 "vtkParse.y"
    { postSig("="); clearVarValue();}
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 1424 "vtkParse.y"
    { setVarValue((yyvsp[(3) - (3)].str)); }
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 1435 "vtkParse.y"
    {
       int type = getStorageType();
       if (getVarValue() && ((type & VTK_PARSE_CONST) != 0) &&
           ((type & VTK_PARSE_INDIRECT) == 0) && getArrayNDims() == 0)
         {
         add_constant(getVarName(), getVarValue(),
                       (type & VTK_PARSE_UNQUALIFIED_TYPE), getTypeId(), 0);
         }
     }
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 1446 "vtkParse.y"
    {postSig(", ");}
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 1449 "vtkParse.y"
    { setStorageTypeIndirection(0); }
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 1450 "vtkParse.y"
    { setStorageTypeIndirection((yyvsp[(1) - (1)].integer)); }
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 1454 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 1455 "vtkParse.y"
    { postSig(")"); }
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 1457 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(5) - (5)].integer),
                       add_indirection((yyvsp[(1) - (5)].integer), (yyvsp[(2) - (5)].integer))); }
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 1461 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 1462 "vtkParse.y"
    { postSig(")"); }
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 1464 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(5) - (5)].integer),
                       add_indirection((yyvsp[(1) - (5)].integer), (yyvsp[(2) - (5)].integer))); }
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 1467 "vtkParse.y"
    { postSig("("); (yyval.integer) = 0; }
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 1468 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("*");
               (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 1470 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("&");
               (yyval.integer) = VTK_PARSE_REF; }
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 1473 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("*");
               (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 1475 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(1) - (1)].str)); postSig("&");
               (yyval.integer) = VTK_PARSE_REF; }
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 1478 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 1479 "vtkParse.y"
    { pushFunction(); postSig("("); }
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 1480 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_FUNCTION; postSig(")"); popFunction(); }
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 1481 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 1484 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 1486 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(1) - (2)].integer), (yyvsp[(2) - (2)].integer));}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 1489 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 1491 "vtkParse.y"
    { (yyval.integer) = add_indirection((yyvsp[(1) - (2)].integer), (yyvsp[(2) - (2)].integer));}
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 1493 "vtkParse.y"
    {clearVarName(); chopSig();}
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 1495 "vtkParse.y"
    {setVarName((yyvsp[(1) - (1)].str));}
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 1497 "vtkParse.y"
    {clearArray();}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 1499 "vtkParse.y"
    {clearArray();}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 1501 "vtkParse.y"
    {postSig("[");}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 1501 "vtkParse.y"
    {postSig("]");}
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 1505 "vtkParse.y"
    {pushArraySize("");}
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 1506 "vtkParse.y"
    {pushArraySize((yyvsp[(1) - (1)].str));}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 1512 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 1513 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 1514 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 1515 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 1516 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 1517 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 1524 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer); setStorageType((yyval.integer));}
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 1525 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 1526 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 1528 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(3) - (3)].integer); setStorageType((yyval.integer));}
    break;

  case 294:

/* Line 1455 of yacc.c  */
#line 1529 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer); setStorageType((yyval.integer));}
    break;

  case 295:

/* Line 1455 of yacc.c  */
#line 1530 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer));
      setStorageType((yyval.integer));}
    break;

  case 296:

/* Line 1455 of yacc.c  */
#line 1532 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(2) - (3)].integer) | (yyvsp[(3) - (3)].integer));
      setStorageType((yyval.integer));}
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 1536 "vtkParse.y"
    {postSig("static "); (yyval.integer) = VTK_PARSE_STATIC; }
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 1537 "vtkParse.y"
    {postSig("static "); (yyval.integer) = VTK_PARSE_STATIC; }
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 1539 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 1540 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer));}
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 1542 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 302:

/* Line 1455 of yacc.c  */
#line 1543 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_CONST | (yyvsp[(2) - (2)].integer));}
    break;

  case 303:

/* Line 1455 of yacc.c  */
#line 1544 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_CONST | (yyvsp[(1) - (2)].integer));}
    break;

  case 304:

/* Line 1455 of yacc.c  */
#line 1546 "vtkParse.y"
    {postSig("const ");}
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 1550 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 307:

/* Line 1455 of yacc.c  */
#line 1552 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 1553 "vtkParse.y"
    {postSig("typename ");}
    break;

  case 309:

/* Line 1455 of yacc.c  */
#line 1554 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(3) - (3)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 310:

/* Line 1455 of yacc.c  */
#line 1557 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 311:

/* Line 1455 of yacc.c  */
#line 1558 "vtkParse.y"
    {chopSig(); postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 312:

/* Line 1455 of yacc.c  */
#line 1559 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 313:

/* Line 1455 of yacc.c  */
#line 1560 "vtkParse.y"
    {chopSig(); postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 315:

/* Line 1455 of yacc.c  */
#line 1562 "vtkParse.y"
    {postSig(", ");}
    break;

  case 317:

/* Line 1455 of yacc.c  */
#line 1564 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 318:

/* Line 1455 of yacc.c  */
#line 1565 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 1566 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 1567 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 321:

/* Line 1455 of yacc.c  */
#line 1568 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 322:

/* Line 1455 of yacc.c  */
#line 1569 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 325:

/* Line 1455 of yacc.c  */
#line 1574 "vtkParse.y"
    {
             (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (3)].str))+strlen((yyvsp[(3) - (3)].str))+3);
             sprintf((yyval.str), "%s::%s", (yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));
             preScopeSig((yyvsp[(1) - (3)].str));
           }
    break;

  case 326:

/* Line 1455 of yacc.c  */
#line 1580 "vtkParse.y"
    {
             (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (3)].str))+strlen((yyvsp[(3) - (3)].str))+3);
             sprintf((yyval.str), "%s::%s", (yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));
             preScopeSig("");
           }
    break;

  case 333:

/* Line 1455 of yacc.c  */
#line 1602 "vtkParse.y"
    { postSig("&"); (yyval.integer) = VTK_PARSE_REF;}
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 1603 "vtkParse.y"
    { postSig("&"); (yyval.integer) = ((yyvsp[(1) - (2)].integer) | VTK_PARSE_REF);}
    break;

  case 335:

/* Line 1455 of yacc.c  */
#line 1604 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 336:

/* Line 1455 of yacc.c  */
#line 1609 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); }
    break;

  case 337:

/* Line 1455 of yacc.c  */
#line 1611 "vtkParse.y"
    {
       int n;
       n = (((yyvsp[(1) - (2)].integer) << 2) | (yyvsp[(2) - (2)].integer));
       if ((n & VTK_PARSE_INDIRECT) != n)
         {
         n = VTK_PARSE_BAD_INDIRECT;
         }
      (yyval.integer) = n;
    }
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 1622 "vtkParse.y"
    { postSig("*"); (yyval.integer) = VTK_PARSE_POINTER; }
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 1623 "vtkParse.y"
    { postSig("*const "); (yyval.integer) = VTK_PARSE_CONST_POINTER; }
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 1626 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 341:

/* Line 1455 of yacc.c  */
#line 1627 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); }
    break;

  case 342:

/* Line 1455 of yacc.c  */
#line 1628 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); }
    break;

  case 343:

/* Line 1455 of yacc.c  */
#line 1629 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 344:

/* Line 1455 of yacc.c  */
#line 1630 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 345:

/* Line 1455 of yacc.c  */
#line 1631 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 346:

/* Line 1455 of yacc.c  */
#line 1632 "vtkParse.y"
    { typeSig((yyvsp[(2) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 347:

/* Line 1455 of yacc.c  */
#line 1635 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 348:

/* Line 1455 of yacc.c  */
#line 1636 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 349:

/* Line 1455 of yacc.c  */
#line 1639 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_STRING;}
    break;

  case 350:

/* Line 1455 of yacc.c  */
#line 1640 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNICODE_STRING;}
    break;

  case 351:

/* Line 1455 of yacc.c  */
#line 1641 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_OSTREAM; }
    break;

  case 352:

/* Line 1455 of yacc.c  */
#line 1642 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_ISTREAM; }
    break;

  case 353:

/* Line 1455 of yacc.c  */
#line 1643 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 354:

/* Line 1455 of yacc.c  */
#line 1644 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_OBJECT; }
    break;

  case 355:

/* Line 1455 of yacc.c  */
#line 1647 "vtkParse.y"
    { typeSig("void"); (yyval.integer) = VTK_PARSE_VOID;}
    break;

  case 356:

/* Line 1455 of yacc.c  */
#line 1648 "vtkParse.y"
    { typeSig("float"); (yyval.integer) = VTK_PARSE_FLOAT;}
    break;

  case 357:

/* Line 1455 of yacc.c  */
#line 1649 "vtkParse.y"
    { typeSig("double"); (yyval.integer) = VTK_PARSE_DOUBLE;}
    break;

  case 358:

/* Line 1455 of yacc.c  */
#line 1650 "vtkParse.y"
    { typeSig("bool"); (yyval.integer) = VTK_PARSE_BOOL;}
    break;

  case 359:

/* Line 1455 of yacc.c  */
#line 1651 "vtkParse.y"
    { typeSig("ssize_t"); (yyval.integer) = VTK_PARSE_SSIZE_T;}
    break;

  case 360:

/* Line 1455 of yacc.c  */
#line 1652 "vtkParse.y"
    { typeSig("size_t"); (yyval.integer) = VTK_PARSE_SIZE_T;}
    break;

  case 361:

/* Line 1455 of yacc.c  */
#line 1653 "vtkParse.y"
    {typeSig("signed char"); (yyval.integer) = VTK_PARSE_SIGNED_CHAR;}
    break;

  case 362:

/* Line 1455 of yacc.c  */
#line 1654 "vtkParse.y"
    { typeSig("vtkTypeInt8"); (yyval.integer) = VTK_PARSE_INT8; }
    break;

  case 363:

/* Line 1455 of yacc.c  */
#line 1655 "vtkParse.y"
    { typeSig("vtkTypeUInt8"); (yyval.integer) = VTK_PARSE_UINT8; }
    break;

  case 364:

/* Line 1455 of yacc.c  */
#line 1656 "vtkParse.y"
    { typeSig("vtkTypeInt16"); (yyval.integer) = VTK_PARSE_INT16; }
    break;

  case 365:

/* Line 1455 of yacc.c  */
#line 1657 "vtkParse.y"
    { typeSig("vtkTypeUInt16"); (yyval.integer) = VTK_PARSE_UINT16; }
    break;

  case 366:

/* Line 1455 of yacc.c  */
#line 1658 "vtkParse.y"
    { typeSig("vtkTypeInt32"); (yyval.integer) = VTK_PARSE_INT32; }
    break;

  case 367:

/* Line 1455 of yacc.c  */
#line 1659 "vtkParse.y"
    { typeSig("vtkTypeUInt32"); (yyval.integer) = VTK_PARSE_UINT32; }
    break;

  case 368:

/* Line 1455 of yacc.c  */
#line 1660 "vtkParse.y"
    { typeSig("vtkTypeInt64"); (yyval.integer) = VTK_PARSE_INT64; }
    break;

  case 369:

/* Line 1455 of yacc.c  */
#line 1661 "vtkParse.y"
    { typeSig("vtkTypeUInt64"); (yyval.integer) = VTK_PARSE_UINT64; }
    break;

  case 370:

/* Line 1455 of yacc.c  */
#line 1662 "vtkParse.y"
    { typeSig("vtkTypeFloat32"); (yyval.integer) = VTK_PARSE_FLOAT32; }
    break;

  case 371:

/* Line 1455 of yacc.c  */
#line 1663 "vtkParse.y"
    { typeSig("vtkTypeFloat64"); (yyval.integer) = VTK_PARSE_FLOAT64; }
    break;

  case 372:

/* Line 1455 of yacc.c  */
#line 1664 "vtkParse.y"
    {typeSig("signed");}
    break;

  case 373:

/* Line 1455 of yacc.c  */
#line 1664 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(3) - (3)].integer);}
    break;

  case 374:

/* Line 1455 of yacc.c  */
#line 1665 "vtkParse.y"
    {typeSig("unsigned");}
    break;

  case 375:

/* Line 1455 of yacc.c  */
#line 1666 "vtkParse.y"
    { (yyval.integer) = (VTK_PARSE_UNSIGNED | (yyvsp[(3) - (3)].integer));}
    break;

  case 376:

/* Line 1455 of yacc.c  */
#line 1667 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 377:

/* Line 1455 of yacc.c  */
#line 1670 "vtkParse.y"
    { typeSig("char"); (yyval.integer) = VTK_PARSE_CHAR;}
    break;

  case 378:

/* Line 1455 of yacc.c  */
#line 1671 "vtkParse.y"
    { typeSig("int"); (yyval.integer) = VTK_PARSE_INT;}
    break;

  case 379:

/* Line 1455 of yacc.c  */
#line 1672 "vtkParse.y"
    { typeSig("short"); (yyval.integer) = VTK_PARSE_SHORT;}
    break;

  case 380:

/* Line 1455 of yacc.c  */
#line 1673 "vtkParse.y"
    { typeSig("long"); (yyval.integer) = VTK_PARSE_LONG;}
    break;

  case 381:

/* Line 1455 of yacc.c  */
#line 1674 "vtkParse.y"
    { typeSig("vtkIdType"); (yyval.integer) = VTK_PARSE_ID_TYPE;}
    break;

  case 382:

/* Line 1455 of yacc.c  */
#line 1675 "vtkParse.y"
    { typeSig("long long"); (yyval.integer) = VTK_PARSE_LONG_LONG;}
    break;

  case 383:

/* Line 1455 of yacc.c  */
#line 1676 "vtkParse.y"
    { typeSig("__int64"); (yyval.integer) = VTK_PARSE___INT64;}
    break;

  case 384:

/* Line 1455 of yacc.c  */
#line 1682 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 385:

/* Line 1455 of yacc.c  */
#line 1683 "vtkParse.y"
    { postSig("{ "); }
    break;

  case 386:

/* Line 1455 of yacc.c  */
#line 1684 "vtkParse.y"
    {
          char *cp;
          postSig("}");
          cp = (char *)malloc(strlen((yyvsp[(3) - (6)].str)) + strlen((yyvsp[(4) - (6)].str)) + 5);
          sprintf(cp, "{ %s%s }", (yyvsp[(3) - (6)].str), (yyvsp[(4) - (6)].str));
          (yyval.str) = cp;
        }
    break;

  case 389:

/* Line 1455 of yacc.c  */
#line 1694 "vtkParse.y"
    {(yyval.str) = vtkstrdup("");}
    break;

  case 390:

/* Line 1455 of yacc.c  */
#line 1695 "vtkParse.y"
    { postSig(", "); }
    break;

  case 391:

/* Line 1455 of yacc.c  */
#line 1696 "vtkParse.y"
    {
          char *cp;
          cp = (char *)malloc(strlen((yyvsp[(1) - (4)].str)) + strlen((yyvsp[(4) - (4)].str)) + 3);
          sprintf(cp, "%s, %s", (yyvsp[(1) - (4)].str), (yyvsp[(4) - (4)].str));
          (yyval.str) = cp;
        }
    break;

  case 392:

/* Line 1455 of yacc.c  */
#line 1703 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 393:

/* Line 1455 of yacc.c  */
#line 1704 "vtkParse.y"
    {postSig("+");}
    break;

  case 394:

/* Line 1455 of yacc.c  */
#line 1704 "vtkParse.y"
    {(yyval.str) = (yyvsp[(3) - (3)].str);}
    break;

  case 395:

/* Line 1455 of yacc.c  */
#line 1705 "vtkParse.y"
    {postSig("-");}
    break;

  case 396:

/* Line 1455 of yacc.c  */
#line 1705 "vtkParse.y"
    {
             (yyval.str) = (char *)malloc(strlen((yyvsp[(3) - (3)].str))+2);
             sprintf((yyval.str), "-%s", (yyvsp[(3) - (3)].str)); }
    break;

  case 397:

/* Line 1455 of yacc.c  */
#line 1708 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 398:

/* Line 1455 of yacc.c  */
#line 1709 "vtkParse.y"
    {postSig("(");}
    break;

  case 399:

/* Line 1455 of yacc.c  */
#line 1709 "vtkParse.y"
    {postSig(")"); (yyval.str) = (yyvsp[(3) - (4)].str);}
    break;

  case 400:

/* Line 1455 of yacc.c  */
#line 1710 "vtkParse.y"
    {postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 401:

/* Line 1455 of yacc.c  */
#line 1711 "vtkParse.y"
    {postSig("<(");}
    break;

  case 402:

/* Line 1455 of yacc.c  */
#line 1712 "vtkParse.y"
    {
            postSig(")");
            (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (9)].str)) + strlen(getTypeId()) +
                                     strlen((yyvsp[(8) - (9)].str)) + 5);
            sprintf((yyval.str), "%s<%s>(%s)", (yyvsp[(1) - (9)].str), getTypeId(), (yyvsp[(8) - (9)].str));
            }
    break;

  case 403:

/* Line 1455 of yacc.c  */
#line 1719 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 404:

/* Line 1455 of yacc.c  */
#line 1720 "vtkParse.y"
    {
                int i = strlen((yyvsp[(1) - (2)].str));
                char *cp = (char *)malloc(i + strlen((yyvsp[(2) - (2)].str)) + 1);
                strcpy(cp, (yyvsp[(1) - (2)].str));
                strcpy(&cp[i], (yyvsp[(2) - (2)].str));
                (yyval.str) = cp; }
    break;

  case 405:

/* Line 1455 of yacc.c  */
#line 1727 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 406:

/* Line 1455 of yacc.c  */
#line 1728 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 407:

/* Line 1455 of yacc.c  */
#line 1729 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 408:

/* Line 1455 of yacc.c  */
#line 1730 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 409:

/* Line 1455 of yacc.c  */
#line 1731 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 410:

/* Line 1455 of yacc.c  */
#line 1732 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 411:

/* Line 1455 of yacc.c  */
#line 1733 "vtkParse.y"
    {(yyval.str) = vtkstrdup(add_const_scope((yyvsp[(1) - (1)].str)));
                postSig((yyvsp[(1) - (1)].str));}
    break;

  case 412:

/* Line 1455 of yacc.c  */
#line 1735 "vtkParse.y"
    {(yyval.str) = vtkstrdup(add_const_scope((yyvsp[(1) - (1)].str)));
                postSig((yyvsp[(1) - (1)].str));}
    break;

  case 413:

/* Line 1455 of yacc.c  */
#line 1745 "vtkParse.y"
    {preSig("void Set"); postSig("(");}
    break;

  case 414:

/* Line 1455 of yacc.c  */
#line 1746 "vtkParse.y"
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

  case 415:

/* Line 1455 of yacc.c  */
#line 1758 "vtkParse.y"
    {postSig("Get");}
    break;

  case 416:

/* Line 1455 of yacc.c  */
#line 1759 "vtkParse.y"
    {markSig();}
    break;

  case 417:

/* Line 1455 of yacc.c  */
#line 1759 "vtkParse.y"
    {swapSig();}
    break;

  case 418:

/* Line 1455 of yacc.c  */
#line 1760 "vtkParse.y"
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

  case 419:

/* Line 1455 of yacc.c  */
#line 1771 "vtkParse.y"
    {preSig("void Set");}
    break;

  case 420:

/* Line 1455 of yacc.c  */
#line 1772 "vtkParse.y"
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

  case 421:

/* Line 1455 of yacc.c  */
#line 1784 "vtkParse.y"
    {preSig("char *Get");}
    break;

  case 422:

/* Line 1455 of yacc.c  */
#line 1785 "vtkParse.y"
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

  case 423:

/* Line 1455 of yacc.c  */
#line 1796 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 424:

/* Line 1455 of yacc.c  */
#line 1796 "vtkParse.y"
    {closeSig();}
    break;

  case 425:

/* Line 1455 of yacc.c  */
#line 1798 "vtkParse.y"
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

  case 426:

/* Line 1455 of yacc.c  */
#line 1839 "vtkParse.y"
    {preSig("void Set"); postSig("("); }
    break;

  case 427:

/* Line 1455 of yacc.c  */
#line 1840 "vtkParse.y"
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

  case 428:

/* Line 1455 of yacc.c  */
#line 1852 "vtkParse.y"
    {postSig("*Get");}
    break;

  case 429:

/* Line 1455 of yacc.c  */
#line 1853 "vtkParse.y"
    {markSig();}
    break;

  case 430:

/* Line 1455 of yacc.c  */
#line 1853 "vtkParse.y"
    {swapSig();}
    break;

  case 431:

/* Line 1455 of yacc.c  */
#line 1854 "vtkParse.y"
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

  case 432:

/* Line 1455 of yacc.c  */
#line 1866 "vtkParse.y"
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

  case 433:

/* Line 1455 of yacc.c  */
#line 1893 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 434:

/* Line 1455 of yacc.c  */
#line 1894 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 2);
   }
    break;

  case 435:

/* Line 1455 of yacc.c  */
#line 1898 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 436:

/* Line 1455 of yacc.c  */
#line 1899 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 2);
   }
    break;

  case 437:

/* Line 1455 of yacc.c  */
#line 1903 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 438:

/* Line 1455 of yacc.c  */
#line 1904 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 3);
   }
    break;

  case 439:

/* Line 1455 of yacc.c  */
#line 1908 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 440:

/* Line 1455 of yacc.c  */
#line 1909 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 3);
   }
    break;

  case 441:

/* Line 1455 of yacc.c  */
#line 1913 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 442:

/* Line 1455 of yacc.c  */
#line 1914 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 4);
   }
    break;

  case 443:

/* Line 1455 of yacc.c  */
#line 1918 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 444:

/* Line 1455 of yacc.c  */
#line 1919 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 4);
   }
    break;

  case 445:

/* Line 1455 of yacc.c  */
#line 1923 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 446:

/* Line 1455 of yacc.c  */
#line 1924 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 6);
   }
    break;

  case 447:

/* Line 1455 of yacc.c  */
#line 1928 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 448:

/* Line 1455 of yacc.c  */
#line 1929 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 6);
   }
    break;

  case 449:

/* Line 1455 of yacc.c  */
#line 1933 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 450:

/* Line 1455 of yacc.c  */
#line 1935 "vtkParse.y"
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
                getTypeId(), atol((yyvsp[(8) - (9)].str)));
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   free(local);
   }
    break;

  case 451:

/* Line 1455 of yacc.c  */
#line 1953 "vtkParse.y"
    {startSig(); markSig();}
    break;

  case 452:

/* Line 1455 of yacc.c  */
#line 1955 "vtkParse.y"
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
              getTypeId(), atol((yyvsp[(8) - (9)].str)));
   output_function();
   free(local);
   }
    break;

  case 453:

/* Line 1455 of yacc.c  */
#line 1972 "vtkParse.y"
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

  case 454:

/* Line 1455 of yacc.c  */
#line 2026 "vtkParse.y"
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

  case 455:

/* Line 1455 of yacc.c  */
#line 2082 "vtkParse.y"
    {
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
   sprintf(currentFunction->Signature, "%s *NewInstance();", (yyvsp[(3) - (6)].str));
   sprintf(temps,"NewInstance");
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   set_return(currentFunction, VTK_PARSE_OBJECT_PTR, (yyvsp[(3) - (6)].str), 0);
   output_function();

   if ( is_concrete )
     {
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature, "%s *SafeDownCast(vtkObject* o);",
             (yyvsp[(3) - (6)].str));
     sprintf(temps,"SafeDownCast");
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     add_argument(currentFunction, VTK_PARSE_OBJECT_PTR, "vtkObject", 0);
     set_return(currentFunction, (VTK_PARSE_STATIC | VTK_PARSE_OBJECT_PTR),
                (yyvsp[(3) - (6)].str), 0);
     output_function();
     }
   }
    break;

  case 456:

/* Line 1455 of yacc.c  */
#line 2141 "vtkParse.y"
    {
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
   sprintf(currentFunction->Signature,
           "int IsA(const char *name);");
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

  case 457:

/* Line 1455 of yacc.c  */
#line 2206 "vtkParse.y"
    { (yyval.str) = "operator()"; }
    break;

  case 458:

/* Line 1455 of yacc.c  */
#line 2207 "vtkParse.y"
    { (yyval.str) = "operator[]"; }
    break;

  case 459:

/* Line 1455 of yacc.c  */
#line 2208 "vtkParse.y"
    { (yyval.str) = "operator new[]"; }
    break;

  case 460:

/* Line 1455 of yacc.c  */
#line 2209 "vtkParse.y"
    { (yyval.str) = "operator delete[]"; }
    break;

  case 462:

/* Line 1455 of yacc.c  */
#line 2212 "vtkParse.y"
    { (yyval.str) = "operator="; }
    break;

  case 463:

/* Line 1455 of yacc.c  */
#line 2213 "vtkParse.y"
    { (yyval.str) = "operator*"; }
    break;

  case 464:

/* Line 1455 of yacc.c  */
#line 2213 "vtkParse.y"
    { (yyval.str) = "operator/"; }
    break;

  case 465:

/* Line 1455 of yacc.c  */
#line 2214 "vtkParse.y"
    { (yyval.str) = "operator-"; }
    break;

  case 466:

/* Line 1455 of yacc.c  */
#line 2214 "vtkParse.y"
    { (yyval.str) = "operator+"; }
    break;

  case 467:

/* Line 1455 of yacc.c  */
#line 2215 "vtkParse.y"
    { (yyval.str) = "operator!"; }
    break;

  case 468:

/* Line 1455 of yacc.c  */
#line 2215 "vtkParse.y"
    { (yyval.str) = "operator~"; }
    break;

  case 469:

/* Line 1455 of yacc.c  */
#line 2216 "vtkParse.y"
    { (yyval.str) = "operator,"; }
    break;

  case 470:

/* Line 1455 of yacc.c  */
#line 2216 "vtkParse.y"
    { (yyval.str) = "operator<"; }
    break;

  case 471:

/* Line 1455 of yacc.c  */
#line 2217 "vtkParse.y"
    { (yyval.str) = "operator>"; }
    break;

  case 472:

/* Line 1455 of yacc.c  */
#line 2217 "vtkParse.y"
    { (yyval.str) = "operator&"; }
    break;

  case 473:

/* Line 1455 of yacc.c  */
#line 2218 "vtkParse.y"
    { (yyval.str) = "operator|"; }
    break;

  case 474:

/* Line 1455 of yacc.c  */
#line 2218 "vtkParse.y"
    { (yyval.str) = "operator^"; }
    break;

  case 475:

/* Line 1455 of yacc.c  */
#line 2219 "vtkParse.y"
    { (yyval.str) = "operator%"; }
    break;

  case 476:

/* Line 1455 of yacc.c  */
#line 2220 "vtkParse.y"
    { (yyval.str) = "operator new"; }
    break;

  case 477:

/* Line 1455 of yacc.c  */
#line 2221 "vtkParse.y"
    { (yyval.str) = "operator delete"; }
    break;

  case 478:

/* Line 1455 of yacc.c  */
#line 2222 "vtkParse.y"
    { (yyval.str) = "operator<<="; }
    break;

  case 479:

/* Line 1455 of yacc.c  */
#line 2223 "vtkParse.y"
    { (yyval.str) = "operator>>="; }
    break;

  case 480:

/* Line 1455 of yacc.c  */
#line 2224 "vtkParse.y"
    { (yyval.str) = "operator<<"; }
    break;

  case 481:

/* Line 1455 of yacc.c  */
#line 2225 "vtkParse.y"
    { (yyval.str) = "operator>>"; }
    break;

  case 482:

/* Line 1455 of yacc.c  */
#line 2226 "vtkParse.y"
    { (yyval.str) = "operator->*"; }
    break;

  case 483:

/* Line 1455 of yacc.c  */
#line 2227 "vtkParse.y"
    { (yyval.str) = "operator->"; }
    break;

  case 484:

/* Line 1455 of yacc.c  */
#line 2228 "vtkParse.y"
    { (yyval.str) = "operator+="; }
    break;

  case 485:

/* Line 1455 of yacc.c  */
#line 2229 "vtkParse.y"
    { (yyval.str) = "operator-="; }
    break;

  case 486:

/* Line 1455 of yacc.c  */
#line 2230 "vtkParse.y"
    { (yyval.str) = "operator*="; }
    break;

  case 487:

/* Line 1455 of yacc.c  */
#line 2231 "vtkParse.y"
    { (yyval.str) = "operator/="; }
    break;

  case 488:

/* Line 1455 of yacc.c  */
#line 2232 "vtkParse.y"
    { (yyval.str) = "operator%="; }
    break;

  case 489:

/* Line 1455 of yacc.c  */
#line 2233 "vtkParse.y"
    { (yyval.str) = "operator++"; }
    break;

  case 490:

/* Line 1455 of yacc.c  */
#line 2234 "vtkParse.y"
    { (yyval.str) = "operator--"; }
    break;

  case 491:

/* Line 1455 of yacc.c  */
#line 2235 "vtkParse.y"
    { (yyval.str) = "operator&="; }
    break;

  case 492:

/* Line 1455 of yacc.c  */
#line 2236 "vtkParse.y"
    { (yyval.str) = "operator|="; }
    break;

  case 493:

/* Line 1455 of yacc.c  */
#line 2237 "vtkParse.y"
    { (yyval.str) = "operator^="; }
    break;

  case 494:

/* Line 1455 of yacc.c  */
#line 2238 "vtkParse.y"
    {(yyval.str) = "operator&&=";}
    break;

  case 495:

/* Line 1455 of yacc.c  */
#line 2239 "vtkParse.y"
    {(yyval.str) = "operator||=";}
    break;

  case 496:

/* Line 1455 of yacc.c  */
#line 2240 "vtkParse.y"
    { (yyval.str) = "operator&&"; }
    break;

  case 497:

/* Line 1455 of yacc.c  */
#line 2241 "vtkParse.y"
    { (yyval.str) = "operator||"; }
    break;

  case 498:

/* Line 1455 of yacc.c  */
#line 2242 "vtkParse.y"
    { (yyval.str) = "operator=="; }
    break;

  case 499:

/* Line 1455 of yacc.c  */
#line 2243 "vtkParse.y"
    { (yyval.str) = "operator!="; }
    break;

  case 500:

/* Line 1455 of yacc.c  */
#line 2244 "vtkParse.y"
    { (yyval.str) = "operator<="; }
    break;

  case 501:

/* Line 1455 of yacc.c  */
#line 2245 "vtkParse.y"
    { (yyval.str) = "operator>="; }
    break;

  case 502:

/* Line 1455 of yacc.c  */
#line 2252 "vtkParse.y"
    {
  static char name[256];
  static char value[256];
  int i = 0;
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
#line 7380 "vtkParse.tab.c"
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
#line 2292 "vtkParse.y"

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

  currentEnumName = NULL;
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
  int i, j;

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
      j = atol(&currentEnumValue[i]);
      sprintf(&currentEnumValue[i], "%i", j+1);
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
                  int type, const char *typeclass, int flag)
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
void add_argument(FunctionInfo *func, int type,
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
void set_return(FunctionInfo *func, int type,
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
  ValueInfo *val, int datatype, int extra, const char *funcSig)
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
    func->Signature = vtkstrdup(funcSig);
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
      if (getArrayNDims() == 1)
        {
        datatype = add_indirection(datatype, VTK_PARSE_POINTER);
        }
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
          n = (int)atol(val->Dimensions[i]);
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
  int j;

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
  func->Signature = vtkstrdup(copySig());
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
  int i, j, match;

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

void outputSetVectorMacro(
  const char *var, int argType, const char *typeText, int n)
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

void outputGetVectorMacro(
  const char *var, int argType, const char *typeText, int n)
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
void vtkParse_AddPointerToArray(void *valueArray, int *count, void *value)
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
  const char *filename, int concrete, FILE *ifile, FILE *errfile)
{
  int i, j, lineno;
  int ret;
  FileInfo *file_info;
  char *main_class;

  vtkParse_InitFile(&data);

  data.FileName = vtkstrdup(filename);
  is_concrete = concrete;

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
  char h_cls[80];
  char h_func[80];
  unsigned int h_type;
  int  h_value;
  FunctionInfo *func_info;
  ClassInfo *class_info;
  NamespaceInfo *contents;
  int i, j;

  contents = file_info->Contents;

  /* read each hint line in succession */
  while (fscanf(hfile,"%s %s %x %i", h_cls, h_func, &h_type, &h_value) != EOF)
    {
    /* erase "ref" and qualifiers from hint type */
    h_type = ((h_type & VTK_PARSE_BASE_TYPE) |
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
              ((int)h_type == ((func_info->ReturnType & ~VTK_PARSE_REF) &
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
