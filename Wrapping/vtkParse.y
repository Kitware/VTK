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
  - remove TABs
  - remove yyerrorlab stuff in range ["goto yyerrlab1;", "yyerrstatus = 3;")

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

/* Map from the type enumeration in vtkType.h to the VTK wrapping type
   system number for the type. */

#include "vtkParse.h"
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

/* Define the division between type and array count */
#define VTK_PARSE_COUNT_START 0x10000

static void vtkParseDebug(const char* s1, const char* s2);

/* the tokenizer */
int yylex(void);

/* global variables */
FileInfo data;
FunctionInfo throwAwayFunction;
FunctionInfo *currentFunction;

char *hintFileName;
FILE *fhint;
char temps[2048];
int  in_public;
int  in_protected;
int  HaveComment;
char CommentText[50000];
int CommentState;
int sigClosed;
size_t sigMark[10];
size_t sigMarkDepth = 0;
unsigned int sigAllocatedLength;
int mainClass;
char *currentId = 0;

void start_class(const char *classname);
void output_function(void);
void reject_function(void);

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
    size_t i, m, n, depth;
    char *cp;
    checkSigSize(arg);
    cp = currentFunction->Signature;
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

/* chop the last char from the signature */
void chopSig(void)
{
  if (currentFunction->Signature)
    {
    size_t n = strlen(currentFunction->Signature);
    if (n > 0)
      {
      currentFunction->Signature[n-1] = '\0';
      }
    }
}

/* delete the signature */
void delSig(void)
{
  if (currentFunction->Signature)
    {
    free(currentFunction->Signature);
    currentFunction->Signature = NULL;
    }
}

/* mark this signature as legacy */
void legacySig(void)
{
  currentFunction->IsLegacy = 1;
}

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
  postSig(text);
  postSig(" ");

  if (currentId == 0)
    {
    setTypeId(text);
    }
  else if (currentId[0] == 'u' && !strcmp(currentId, "unsigned"))
    {
    currentId[8] = ' ';
    strcpy(&currentId[9], text);
    }
}

/* return the current Id and clear it */
const char *getTypeId()
{
  return currentId;
}

%}

%union{
  char *str;
  int   integer;
}

%token CLASS
%token PUBLIC
%token PRIVATE
%token PROTECTED
%token VIRTUAL
%token <str> ID
%token <str> STRING_LITERAL
%token <str> INT_LITERAL
%token <str> HEX_LITERAL
%token <str> FLOAT_LITERAL
%token <str> CHAR_LITERAL
%token INT
%token FLOAT
%token SHORT
%token LONG
%token LONG_LONG
%token INT64__
%token DOUBLE
%token VOID
%token CHAR
%token SIGNED_CHAR
%token BOOL
%token OSTREAM
%token ISTREAM
%token ENUM
%token UNION
%token CLASS_REF
%token OTHER
%token CONST
%token CONST_PTR
%token CONST_EQUAL
%token OPERATOR
%token UNSIGNED
%token FRIEND
%token INLINE
%token MUTABLE
%token TEMPLATE
%token TYPENAME
%token TYPEDEF
%token NAMESPACE
%token USING
%token <str> VTK_ID
%token STATIC
%token VAR_FUNCTION
%token ARRAY_NUM
%token VTK_LEGACY
%token NEW
%token DELETE
%token <str> LPAREN_POINTER
%token LPAREN_AMPERSAND
%token OP_LSHIFT_EQ
%token OP_RSHIFT_EQ
%token OP_LSHIFT
%token OP_RSHIFT
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
%token OP_LOGIC_AND_EQ
%token OP_LOGIC_OR_EQ
%token OP_LOGIC_AND
%token OP_LOGIC_OR
%token OP_LOGIC_EQ
%token OP_LOGIC_NEQ
%token OP_LOGIC_LEQ
%token OP_LOGIC_GEQ
%token ELLIPSIS
%token DOUBLE_COLON

/* type tokens */
%token IdType
%token StdString
%token UnicodeString
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

/* macro tokens */
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

/* special tokens */
%token VTK_CONSTANT_DEF
%token VTK_BYTE_SWAP_DECL

%%
/*
 * Here is the start of the grammer
 */
strt: maybe_other maybe_classes;

maybe_classes: | maybe_template_class_def maybe_other maybe_classes;

maybe_template_class_def: template type_red2 | template INLINE type_red2
              | template class_def | class_def;

class_def: CLASS any_id { start_class($<str>2); }
    optional_scope '{' class_def_body '}'
  | CLASS any_id '<' types '>' { start_class($<str>2); }
    optional_scope '{' class_def_body '}'

class_def_body: | { delSig(); clearTypeId(); } class_def_item class_def_body;

class_def_item: scope_type ':'
   | var
   | MUTABLE var
   | enum
   | named_enum
   | union
   | named_union
   | using
   | typedef
   | internal_class
   | FRIEND internal_class
   | template_internal_class
   | CLASS_REF
   | operator func_body { output_function(); }
   | FRIEND operator func_body { reject_function(); }
   | INLINE operator func_body { output_function(); }
   | template_operator func_body { output_function(); }
   | INLINE template_operator func_body { output_function(); }
   | function func_body { output_function(); }
   | FRIEND function func_body { reject_function(); }
   | INLINE function func_body { output_function(); }
   | template_function func_body { output_function(); }
   | INLINE template_function func_body { output_function(); }
   | legacy_function func_body { legacySig(); output_function(); }
   | VTK_BYTE_SWAP_DECL '(' maybe_other ')' ';'
   | macro ';'
   | macro;

/*
 * Enum constants should be handled as strings, so that IDs can be used.
 * The text can be dropped into the generated .cxx file and evaluated there,
 * just make sure that the IDs are properly scoped.
 */

named_enum: ENUM any_id '{' enum_list '}' maybe_other_no_semi ';';

enum: ENUM '{' enum_list '}' maybe_other_no_semi ';';

enum_list: | enum_item | enum_item ',' enum_list;

enum_item: any_id | any_id '=' enum_math;

/* "any_id" -> "vtkClass::any_id" if it's a previously defined enum const */

enum_value: enum_literal | any_id | scoped_id | templated_id;

enum_literal: INT_LITERAL | HEX_LITERAL | CHAR_LITERAL;

enum_math: enum_value { $<str>$ = $<str>1; }
     | math_unary_op enum_math
       {
         $<str>$ = (char *)malloc(strlen($<str>1) + strlen($<str>2) + 1);
         sprintf($<str>$, "%s%s", $<str>1, $<str>2);
       }
     | enum_value math_binary_op enum_math
       {
         $<str>$ = (char *)malloc(strlen($<str>1) + strlen($<str>2) +
                                  strlen($<str>3) + 3);
         sprintf($<str>$, "%s %s %s", $<str>1, $<str>2, $<str>3);
       }
     | '(' enum_math ')'
       {
         $<str>$ = (char *)malloc(strlen($<str>2) + 3);
         sprintf($<str>$, "(%s)", $<str>2);
       };

math_unary_op:   '-' { $<str>$ = "-"; } | '+' { $<str>$ = "+"; }
               | '~' { $<str>$ = "~"; };

math_binary_op:  '-' { $<str>$ = "-"; } | '+' { $<str>$ = "+"; }
               | '*' { $<str>$ = "*"; } | '/' { $<str>$ = "/"; }
               | '%' { $<str>$ = "%"; } | '&' { $<str>$ = "&"; }
               | '|' { $<str>$ = "|"; } | '^' { $<str>$ = "^"; };

named_union: UNION any_id '{' maybe_other '}' maybe_other_no_semi ';';

union: UNION '{' maybe_other '}' maybe_other_no_semi ';';

using: USING maybe_other_no_semi ';';

template_internal_class: template internal_class;

internal_class: CLASS any_id internal_class_body;

internal_class_body: ';'
    | '{' maybe_other '}' ';'
    | ':' maybe_other_no_semi ';';

typedef: TYPEDEF type var_id ';'
 | TYPEDEF CLASS any_id '{' maybe_other '}' any_id ';'
 | TYPEDEF CLASS '{' maybe_other '}' any_id ';';
 | TYPEDEF type LPAREN_POINTER any_id ')' '(' maybe_other_no_semi ')' ';'
 | TYPEDEF type LPAREN_AMPERSAND any_id ')' '(' maybe_other_no_semi ')' ';'
 | TYPEDEF enum
 | TYPEDEF named_enum
 | TYPEDEF union
 | TYPEDEF named_union
 | TYPEDEF VAR_FUNCTION ';';

template_function: template function;

template_operator: template operator;

template: TEMPLATE '<' '>' { postSig("template<> "); clearTypeId(); }
        | TEMPLATE '<' { postSig("template<"); }
          template_args '>' { postSig("> "); clearTypeId(); };

template_args: template_arg
             | template_arg ',' { postSig(", "); } template_args;

template_arg: template_type maybe_id | template maybe_id;

template_type: TYPENAME { postSig("typename "); }
             | CLASS { postSig("class "); }
             | INT { postSig("int "); };

legacy_function: VTK_LEGACY '(' function ')' ;

function: '~' destructor {openSig(); preSig("~"); closeSig();}
      | VIRTUAL '~' destructor {openSig(); preSig("virtual ~"); closeSig();}
      | constructor
      | type func
         {
         currentFunction->ReturnType = $<integer>1;
         }
      | VIRTUAL type func
         {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->ReturnType = $<integer>2;
         };

operator:
        typecast_op_func
         {
         currentFunction->ReturnType = $<integer>1;
         }
      | type op_func
         {
         currentFunction->ReturnType = $<integer>1;
         }
      | VIRTUAL type op_func
         {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->ReturnType = $<integer>2;
         };

typecast_op_func:
  OPERATOR type '('
    {
      postSig("(");
      currentFunction->ReturnClass = vtkstrdup(getTypeId());
    }
  args_list ')' { postSig(")"); } maybe_const
    {
      $<integer>$ = $<integer>2;
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
    };

op_func: op_sig { postSig(")"); } maybe_const
    {
      postSig(";");
      closeSig();
      currentFunction->Name = $<str>1;
      if (HaveComment)
        {
        currentFunction->Comment = vtkstrdup(CommentText);
        }
      vtkParseDebug("Parsed operator", $<str>1);
    }
  | op_sig pure_virtual
    {
      postSig(";");
      closeSig();
      currentFunction->Name = $<str>1;
      if (HaveComment)
        {
        currentFunction->Comment = vtkstrdup(CommentText);
        }
      vtkParseDebug("Parsed operator", $<str>1);
      currentFunction->IsPureVirtual = 1;
      if (mainClass)
        {
        data.IsAbstract = 1;
        }
    };

op_sig: OPERATOR op_token {postSig($<str>2);} '('
    {
      postSig("(");
      currentFunction->IsOperator = 1;
      currentFunction->ReturnClass = vtkstrdup(getTypeId());
    }
    args_list ')' { $<str>$ = $<str>2; };

func: func_sig { postSig(")"); } maybe_const
    {
      postSig(";");
      closeSig();
      currentFunction->Name = $<str>1;
      if (HaveComment)
        {
        currentFunction->Comment = vtkstrdup(CommentText);
        }
      vtkParseDebug("Parsed func", $<str>1);
    }
  | func_sig pure_virtual
    {
      postSig(";");
      closeSig();
      currentFunction->Name = $<str>1;
      if (HaveComment)
        {
        currentFunction->Comment = vtkstrdup(CommentText);
        }
      vtkParseDebug("Parsed func", $<str>1);
      currentFunction->IsPureVirtual = 1;
      if (mainClass)
        {
        data.IsAbstract = 1;
        }
    };

pure_virtual: '=' INT_LITERAL {postSig(") = 0");}
            | CONST_EQUAL INT_LITERAL {postSig(") const = 0");};

maybe_const: | CONST {postSig(" const");};

func_sig: any_id '('
    {
      postSig("(");
      currentFunction->ReturnClass = vtkstrdup(getTypeId());
    } args_list ')' { $<str>$ = $<str>1; }
  | any_id '<' {markSig(); postSig("<");} types '>' '('
    {
      const char *cp;
      postSig(">(");
      currentFunction->ReturnClass = vtkstrdup(getTypeId());
      cp = copySig();
      $<str>$ = (char *)malloc(strlen($<str>1) + strlen(cp) + 1);
      sprintf($<str>$, "%s%s", $<str>1, cp);
    } args_list ')' { $<str>$ = $<str>7; };

constructor: constructor_sig { postSig(")"); } maybe_initializers
    {
      postSig(";");
      closeSig();
      currentFunction->Name = $<str>1;
      if (HaveComment)
        {
        currentFunction->Comment = vtkstrdup(CommentText);
        }
      vtkParseDebug("Parsed func", $<str>1);
    };

constructor_sig: any_id '(' { postSig("("); } args_list ')';

maybe_initializers: | ':' initializer more_initializers;

more_initializers: | ',' initializer more_initializers;

initializer: any_id '(' maybe_other ')';

destructor: destructor_sig
    {
      postSig(");");
      closeSig();
      currentFunction->Name = (char *)malloc(strlen($<str>1) + 2);
      currentFunction->Name[0] = '~';
      strcpy(&currentFunction->Name[1], $<str>1);
      if (HaveComment)
        {
        currentFunction->Comment = vtkstrdup(CommentText);
        }
      vtkParseDebug("Parsed func", currentFunction->Name);
    };

destructor_sig: any_id '(' { postSig("(");} args_list ')';

const_mod: CONST {postSig("const ");};

static_mod: STATIC {postSig("static ");}
          | STATIC INLINE {postSig("static ");};

any_id: VTK_ID {postSig($<str>1);} | ID {postSig($<str>1);};

func_body: ';'
    | '{' maybe_other '}' ';'
    | '{' maybe_other '}';

ignore_args_list: | ignore_more_args;

ignore_more_args: arg | arg ',' ignore_more_args;

args_list: | {clearTypeId();} more_args;

more_args: ELLIPSIS { postSig("...");}
  | arg
    { clearTypeId(); currentFunction->NumberOfArguments++; }
  | arg ','
    { clearTypeId(); currentFunction->NumberOfArguments++; postSig(", "); }
    more_args;

arg: type maybe_var_array
    {
      int i = currentFunction->NumberOfArguments;
      int array_type = ($<integer>2 % VTK_PARSE_COUNT_START);
      int array_count = ($<integer>2 / VTK_PARSE_COUNT_START);
      currentFunction->ArgCounts[i] = array_count;
      currentFunction->ArgTypes[i] = $<integer>1 + array_type;
      currentFunction->ArgClasses[i] = vtkstrdup(getTypeId());
    }
  | type var_id
    {
      int i = currentFunction->NumberOfArguments;
      int array_type = ($<integer>2 % VTK_PARSE_COUNT_START);
      int array_count = ($<integer>2 / VTK_PARSE_COUNT_START);
      currentFunction->ArgCounts[i] = array_count;
      currentFunction->ArgTypes[i] = $<integer>1 + array_type;
      currentFunction->ArgClasses[i] = vtkstrdup(getTypeId());
    } maybe_var_assign
  | VAR_FUNCTION
    {
      int i = currentFunction->NumberOfArguments;
      postSig("void (*func)(void *) ");
      currentFunction->ArgCounts[i] = 0;
      currentFunction->ArgTypes[i] = VTK_PARSE_FUNCTION;
      currentFunction->ArgClasses[i] = vtkstrdup("function");
    }
  | type LPAREN_AMPERSAND { postSig("(&"); } maybe_id ')'
    {
      int i = currentFunction->NumberOfArguments;
      postSig(") ");
      currentFunction->ArgCounts[i] = 0;
      currentFunction->ArgTypes[i] = ($<integer>1 | VTK_PARSE_REF);
      currentFunction->ArgClasses[i] = vtkstrdup(getTypeId());
    }
  | type LPAREN_POINTER { postSig("("); postSig($<str>2); postSig("*"); }
    maybe_id ')' '(' { postSig(")("); } ignore_args_list ')'
    {
      int i = currentFunction->NumberOfArguments;
      postSig(")");
      currentFunction->ArgCounts[i] = 0;
      currentFunction->ArgTypes[i] = VTK_PARSE_UNKNOWN;
      currentFunction->ArgClasses[i] = vtkstrdup("function");
    };

maybe_id: | any_id;

maybe_var_assign: | var_assign;

var_assign: '=' literal {postSig("="); postSig($<str>2);};

var: CLASS any_id var_ids ';'
     | CLASS any_id type_indirection var_ids ';'
     | ENUM any_id var_ids ';'
     | ENUM any_id type_indirection var_ids ';'
     | UNION any_id var_ids ';'
     | UNION any_id type_indirection var_ids ';'
     | type var_ids ';'
     | VAR_FUNCTION ';'
     | STATIC VAR_FUNCTION ';'
     | type LPAREN_AMPERSAND any_id ')' ';'
     | type LPAREN_POINTER any_id ')' ';'
     | type LPAREN_POINTER any_id ')' ARRAY_NUM ';'
     | type LPAREN_POINTER any_id ')' '[' maybe_other ']' ';'
     | type LPAREN_POINTER any_id ')' '(' ignore_args_list ')' ';'

var_ids: var_id_maybe_assign
       | var_id_maybe_assign ',' maybe_indirect_var_ids;

maybe_indirect_var_ids: var_id_maybe_assign
                | var_id_maybe_assign ',' maybe_indirect_var_ids;
                | type_indirection var_id_maybe_assign
                | type_indirection var_id_maybe_assign ','
                  maybe_indirect_var_ids;

var_id_maybe_assign: var_id maybe_var_assign;

var_id: any_id maybe_var_array {$<integer>$ = $<integer>2;};

/*
 [n]       = VTK_PARSE_POINTER
 [n][m]    = 2*VTK_PARSE_POINTER
 [n][m][l] = 3*VTK_PARSE_POINTER
*/

maybe_var_array: {$<integer>$ = 0;} | var_array {$<integer>$ = $<integer>1;};

var_array:
       ARRAY_NUM { char temp[100]; sprintf(temp,"[%i]",$<integer>1);
                   postSig(temp); }
       maybe_var_array { $<integer>$ =
                         ((VTK_PARSE_COUNT_START * $<integer>1) |
                          ((VTK_PARSE_POINTER + $<integer>3) &
                           VTK_PARSE_UNQUALIFIED_TYPE)); }
     | '[' maybe_other_no_semi ']' maybe_var_array
            { postSig("[]");
              $<integer>$ = ((VTK_PARSE_POINTER + $<integer>4) &
                             VTK_PARSE_UNQUALIFIED_TYPE); };

type: type_red1 {$<integer>$ = $<integer>1;}
    | static_mod type_red1 {$<integer>$ = (VTK_PARSE_STATIC | $<integer>2);};

type_red1: type_red11 {$<integer>$ = $<integer>1;}
    | type_red11 type_indirection {$<integer>$ = ($<integer>1 | $<integer>2);};

type_red11: type_red12 {$<integer>$ = $<integer>1;}
    | const_mod type_red12 {$<integer>$ = (VTK_PARSE_CONST | $<integer>2);}
    | type_red12 const_mod {$<integer>$ = (VTK_PARSE_CONST | $<integer>1);};

type_red12: type_red2
    | templated_id
      {postSig(" "); setTypeId($<str>1); $<integer>$ = VTK_PARSE_UNKNOWN;}
    | scoped_id
      {postSig(" "); setTypeId($<str>1); $<integer>$ = VTK_PARSE_UNKNOWN;}
    | TYPENAME {postSig("typename ");} scoped_id
      {postSig(" "); setTypeId($<str>1); $<integer>$ = VTK_PARSE_UNKNOWN;};

templated_id:
   VTK_ID '<' { markSig(); postSig($<str>1); postSig("<");} types '>'
     {chopSig(); postSig(">"); $<str>$ = vtkstrdup(copySig()); clearTypeId();}
 | ID '<' { markSig(); postSig($<str>1); postSig("<");} types '>'
     {chopSig(); postSig(">"); $<str>$ = vtkstrdup(copySig()); clearTypeId();};

types: type_red1 | type_red1 ',' {postSig(", ");} types;

maybe_scoped_id: ID {$<str>$ = $<str>1; postSig($<str>1);}
               | VTK_ID {$<str>$ = $<str>1; postSig($<str>1);}
               | templated_id {$<str>$ = $<str>1;};
               | scoped_id {$<str>$ = $<str>1;};

scoped_id: ID DOUBLE_COLON maybe_scoped_id
           {
             $<str>$ = (char *)malloc(strlen($<str>1)+strlen($<str>3)+3);
             sprintf($<str>$, "%s::%s", $<str>1, $<str>3);
             preScopeSig($<str>1);
           }
         | VTK_ID DOUBLE_COLON maybe_scoped_id
           {
             $<str>$ = (char *)malloc(strlen($<str>1)+strlen($<str>3)+3);
             sprintf($<str>$, "%s::%s", $<str>1, $<str>3);
             preScopeSig($<str>1);
           }
         | templated_id DOUBLE_COLON maybe_scoped_id
           {
             $<str>$ = (char *)malloc(strlen($<str>1)+strlen($<str>3)+3);
             sprintf($<str>$, "%s::%s", $<str>1, $<str>3);
             preScopeSig("");
           };

/* &          is VTK_PARSE_REF
   *          is VTK_PARSE_POINTER
   *&         is VTK_PARSE_POINTER_REF
   **         is VTK_PARSE_POINTER_POINTER
   *const     is VTK_PARSE_CONST_POINTER
   *const&    is VTK_PARSE_CONST_POINTER_REF
   *const*    is VTK_PARSE_POINTER_CONST_POINTER
   everything else is VTK_PARSE_BAD_INDIRECT
   */

type_indirection:
    '&' { postSig("&"); $<integer>$ = VTK_PARSE_REF;}
  | '*' { postSig("*"); $<integer>$ = VTK_PARSE_POINTER;}
  | CONST_PTR { postSig("*const "); $<integer>$ = VTK_PARSE_CONST_POINTER;}
  | '*' '&' { postSig("*&"); $<integer>$ = VTK_PARSE_POINTER_REF;}
  | '*' '*' { postSig("**"); $<integer>$ = VTK_PARSE_POINTER_POINTER;}
  | CONST_PTR '&'
    { postSig("*const &"); $<integer>$ = VTK_PARSE_CONST_POINTER_REF;}
  | CONST_PTR '*'
    { postSig("*const *"); $<integer>$ = VTK_PARSE_POINTER_CONST_POINTER;}
  | CONST_PTR CONST_PTR
    { postSig("*const *"); $<integer>$ = VTK_PARSE_BAD_INDIRECT;}
  | '*' '*' { postSig("**"); } type_indirection
    { $<integer>$ = VTK_PARSE_BAD_INDIRECT;}
  | CONST_PTR '*' { postSig("*const *");} type_indirection
    { $<integer>$ = VTK_PARSE_BAD_INDIRECT;}
  | CONST_PTR CONST_PTR { postSig("*const *const ");} type_indirection
    { $<integer>$ = VTK_PARSE_BAD_INDIRECT;};

type_red2:  type_primitive { $<integer>$ = $<integer>1;}
 | StdString { typeSig("vtkStdString"); $<integer>$ = VTK_PARSE_STRING;}
 | UnicodeString
   { typeSig("vtkUnicodeString"); $<integer>$ = VTK_PARSE_UNICODE_STRING;}
 | OSTREAM
    { typeSig("ostream"); $<integer>$ = VTK_PARSE_UNKNOWN; }
 | ISTREAM { typeSig("istream"); $<integer>$ = VTK_PARSE_UNKNOWN; }
 | ID { typeSig($<str>1); $<integer>$ = VTK_PARSE_UNKNOWN; }
 | VTK_ID { typeSig($<str>1); $<integer>$ = VTK_PARSE_VTK_OBJECT; };

type_primitive:
  VOID   { typeSig("void"); $<integer>$ = VTK_PARSE_VOID;} |
  FLOAT  { typeSig("float"); $<integer>$ = VTK_PARSE_FLOAT;} |
  DOUBLE { typeSig("double"); $<integer>$ = VTK_PARSE_DOUBLE;} |
  BOOL { typeSig("bool"); $<integer>$ = VTK_PARSE_BOOL;} |
  SIGNED_CHAR {typeSig("signed char"); $<integer>$ = VTK_PARSE_SIGNED_CHAR;} |
  TypeInt8 { typeSig("vtkTypeInt8"); $<integer>$ = VTK_PARSE_INT8; } |
  TypeUInt8 { typeSig("vtkTypeUInt8"); $<integer>$ = VTK_PARSE_UINT8; } |
  TypeInt16 { typeSig("vtkTypeInt16"); $<integer>$ = VTK_PARSE_INT16; } |
  TypeUInt16 { typeSig("vtkTypeUInt16"); $<integer>$ = VTK_PARSE_UINT16; } |
  TypeInt32 { typeSig("vtkTypeInt32"); $<integer>$ = VTK_PARSE_INT32; } |
  TypeUInt32 { typeSig("vtkTypeUInt32"); $<integer>$ = VTK_PARSE_UINT32; } |
  TypeInt64 { typeSig("vtkTypeInt64"); $<integer>$ = VTK_PARSE_INT64; } |
  TypeUInt64 { typeSig("vtkTypeUInt64"); $<integer>$ = VTK_PARSE_UINT64; } |
  TypeFloat32 { typeSig("vtkTypeFloat32"); $<integer>$ = VTK_PARSE_FLOAT32; } |
  TypeFloat64 { typeSig("vtkTypeFloat64"); $<integer>$ = VTK_PARSE_FLOAT64; } |
  UNSIGNED {typeSig("unsigned");}
   type_integer { $<integer>$ = (VTK_PARSE_UNSIGNED | $<integer>3);} |
  type_integer { $<integer>$ = $<integer>1;};

type_integer:
  CHAR   { typeSig("char"); $<integer>$ = VTK_PARSE_CHAR;} |
  INT    { typeSig("int"); $<integer>$ = VTK_PARSE_INT;} |
  SHORT  { typeSig("short"); $<integer>$ = VTK_PARSE_SHORT;} |
  LONG   { typeSig("long"); $<integer>$ = VTK_PARSE_LONG;} |
  IdType { typeSig("vtkIdType"); $<integer>$ = VTK_PARSE_ID_TYPE;} |
  LONG_LONG { typeSig("long long"); $<integer>$ = VTK_PARSE_LONG_LONG;} |
  INT64__ { typeSig("__int64"); $<integer>$ = VTK_PARSE___INT64;};

optional_scope: | ':' scope_list;

scope_list: scope_type maybe_scoped_id
    {
      if (mainClass)
        {
        data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup($<str>2);
        data.NumberOfSuperClasses++;
        }
    }
  | scope_type maybe_scoped_id
    {
      if (mainClass)
        {
        data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup($<str>2);
        data.NumberOfSuperClasses++;
        }
    } ',' scope_list;

scope_type: {in_public = 0; in_protected = 0;}
          | PUBLIC {in_public = 1; in_protected = 0;}
          | PRIVATE {in_public = 0; in_protected = 0;}
          | PROTECTED {in_public = 0; in_protected = 1;};

literal:  literal2 {$<str>$ = $<str>1;}
          | '+' literal2 {$<str>$ = $<str>2;}
          | '-' literal2 {$<str>$ = (char *)malloc(strlen($<str>2)+2);
                        sprintf($<str>$, "-%s", $<str>2); }
          | STRING_LITERAL {$<str>$ = $<str>1;}
          | '(' literal ')' {$<str>$ = $<str>2;};

literal2: INT_LITERAL {$<str>$ = $<str>1;}
          | HEX_LITERAL {$<str>$ = $<str>1;}
          | FLOAT_LITERAL {$<str>$ = $<str>1;}
          | CHAR_LITERAL {$<str>$ = $<str>1;}
          | ID {$<str>$ = $<str>1;}
          | VTK_ID {$<str>$ = $<str>1;};

macro:
  SetMacro '(' any_id ',' {preSig("void Set"); postSig("(");} type_red1 ')'
   {
   postSig("a);");
   sprintf(temps,"Set%s",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = $<integer>6;
   currentFunction->ArgClasses[0] = vtkstrdup(getTypeId());
   currentFunction->ArgCounts[0] = 0;
   output_function();
   }
| GetMacro '(' {postSig("Get");} any_id ','
   {markSig();} type_red1 {swapSig();} ')'
   {
   postSig("();");
   sprintf(temps,"Get%s",$<str>4);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   currentFunction->ReturnType = $<integer>7;
   currentFunction->ReturnClass = vtkstrdup(getTypeId());
   output_function();
   }
| SetStringMacro '(' {preSig("void Set");} any_id ')'
   {
   postSig("(char *);");
   sprintf(temps,"Set%s",$<str>4);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = VTK_PARSE_CHAR_PTR;
   currentFunction->ArgClasses[0] = vtkstrdup("char");
   currentFunction->ArgCounts[0] = 0;
   output_function();
   }
| GetStringMacro '(' {preSig("char *Get");} any_id ')'
   {
   postSig("();");
   sprintf(temps,"Get%s",$<str>4);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = VTK_PARSE_CHAR_PTR;
   currentFunction->ReturnClass = vtkstrdup("char");
   output_function();
   }
| SetClampMacro '(' any_id ',' {delSig(); markSig();} type_red2 {closeSig();}
     ',' maybe_other_no_semi ')'
   {
   char *local;
   chopSig();
   local = vtkstrdup(copySig());
   sprintf(currentFunction->Signature,"void Set%s(%s);",$<str>3,local);
   sprintf(temps,"Set%s",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = $<integer>6;
   currentFunction->ArgClasses[0] = vtkstrdup(getTypeId());
   currentFunction->ArgCounts[0] = 0;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%sGet%sMinValue();",local,$<str>3);
   sprintf(temps,"Get%sMinValue",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   currentFunction->ReturnType = $<integer>6;
   currentFunction->ReturnClass = vtkstrdup(getTypeId());
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%sGet%sMaxValue();",local,$<str>3);
   sprintf(temps,"Get%sMaxValue",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   currentFunction->ReturnType = $<integer>6;
   currentFunction->ReturnClass = vtkstrdup(getTypeId());
   output_function();
   free(local);
   }
| SetObjectMacro '(' any_id ','
  {preSig("void Set"); postSig("("); } type_red2 ')'
   {
   postSig("*);");
   sprintf(temps,"Set%s",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = VTK_PARSE_VTK_OBJECT_PTR;
   currentFunction->ArgClasses[0] = vtkstrdup(getTypeId());
   currentFunction->ArgCounts[0] = 1;
   output_function();
   }
| GetObjectMacro '(' {postSig("*Get");} any_id ','
   {markSig();} type_red2 {swapSig();} ')'
   {
   postSig("();");
   sprintf(temps,"Get%s",$<str>4);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   currentFunction->ReturnType = VTK_PARSE_VTK_OBJECT_PTR;
   currentFunction->ReturnClass = vtkstrdup(getTypeId());
   output_function();
   }
| BooleanMacro '(' any_id ',' type_red2 ')'
   {
   sprintf(temps,"%sOn",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   delSig();
   postSig("void ");
   postSig(temps);
   postSig("();");
   output_function();

   sprintf(temps,"%sOff",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   delSig();
   postSig("void ");
   postSig(temps);
   postSig("();");
   output_function();
   }
| SetVector2Macro '(' any_id ',' {delSig(); markSig();} type_red2 ')'
   {
   chopSig();
   outputSetVectorMacro($<str>3, $<integer>6, copySig(), 2);
   }
| GetVector2Macro '(' any_id ',' {delSig(); markSig();} type_red2 ')'
   {
   chopSig();
   outputGetVectorMacro($<str>3, $<integer>6, copySig(), 2);
   }
| SetVector3Macro '(' any_id ',' {delSig(); markSig();} type_red2 ')'
   {
   chopSig();
   outputSetVectorMacro($<str>3, $<integer>6, copySig(), 3);
   }
| GetVector3Macro  '(' any_id ',' {delSig(); markSig();} type_red2 ')'
   {
   chopSig();
   outputGetVectorMacro($<str>3, $<integer>6, copySig(), 3);
   }
| SetVector4Macro '(' any_id ',' {delSig(); markSig();} type_red2 ')'
   {
   chopSig();
   outputSetVectorMacro($<str>3, $<integer>6, copySig(), 4);
   }
| GetVector4Macro  '(' any_id ',' {delSig(); markSig();} type_red2 ')'
   {
   chopSig();
   outputGetVectorMacro($<str>3, $<integer>6, copySig(), 4);
   }
| SetVector6Macro '(' any_id ',' {delSig(); markSig();} type_red2 ')'
   {
   chopSig();
   outputSetVectorMacro($<str>3, $<integer>6, copySig(), 6);
   }
| GetVector6Macro  '(' any_id ',' {delSig(); markSig();} type_red2 ')'
   {
   chopSig();
   outputGetVectorMacro($<str>3, $<integer>6, copySig(), 6);
   }
| SetVectorMacro  '(' any_id ',' {delSig(); markSig();}
     type_red2 ',' INT_LITERAL ')'
   {
   char *local;
   chopSig();
   local = vtkstrdup(copySig());
   sprintf(currentFunction->Signature,"void Set%s(%s a[%s]);",
           $<str>3, local, $<str>8);
   sprintf(temps,"Set%s",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (VTK_PARSE_POINTER | $<integer>6);
   currentFunction->ArgClasses[0] = vtkstrdup(getTypeId());
   currentFunction->ArgCounts[0] = atol($<str>8);
   output_function();
   free(local);
   }
| GetVectorMacro  '(' any_id ',' {delSig(); markSig();}
     type_red2 ',' INT_LITERAL ')'
   {
   char *local;
   chopSig();
   local = vtkstrdup(copySig());
   sprintf(currentFunction->Signature,"%s *Get%s();", local, $<str>3);
   sprintf(temps,"Get%s",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (VTK_PARSE_POINTER | $<integer>6);
   currentFunction->ReturnClass = vtkstrdup(getTypeId());
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = atol($<str>8);
   output_function();
   free(local);
   }
| ViewportCoordinateMacro '(' any_id ')'
   {
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate();",
             $<str>3);

     sprintf(temps,"Get%sCoordinate",$<str>3);
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = VTK_PARSE_VTK_OBJECT_PTR;
     currentFunction->ReturnClass = "vtkCoordinate";
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s(double, double);",
             $<str>3);
     sprintf(temps,"Set%s",$<str>3);
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     currentFunction->NumberOfArguments = 2;
     currentFunction->ArgTypes[0] = VTK_PARSE_DOUBLE;
     currentFunction->ArgClasses[0] = vtkstrdup("double");
     currentFunction->ArgCounts[0] = 0;
     currentFunction->ArgTypes[1] = VTK_PARSE_DOUBLE;
     currentFunction->ArgClasses[1] = vtkstrdup("double");
     currentFunction->ArgCounts[1] = 0;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s(double a[2]);",
             $<str>3);
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = VTK_PARSE_DOUBLE_PTR;
     currentFunction->ArgClasses[0] = vtkstrdup("double");
     currentFunction->ArgCounts[0] = 2;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s();", $<str>3);
     sprintf(temps,"Get%s",$<str>3);
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = VTK_PARSE_DOUBLE_PTR;
     currentFunction->ReturnClass = vtkstrdup("double");
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 2;
     output_function();
   }
| WorldCoordinateMacro '(' any_id ')'
   {
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate();",
             $<str>3);

     sprintf(temps,"Get%sCoordinate",$<str>3);
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = VTK_PARSE_VTK_OBJECT_PTR;
     currentFunction->ReturnClass = "vtkCoordinate";
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,
             "void Set%s(double, double, double);",
             $<str>3);
     sprintf(temps,"Set%s",$<str>3);
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     currentFunction->NumberOfArguments = 3;
     currentFunction->ArgTypes[0] = VTK_PARSE_DOUBLE;
     currentFunction->ArgClasses[0] = vtkstrdup("double");
     currentFunction->ArgCounts[0] = 0;
     currentFunction->ArgTypes[1] = VTK_PARSE_DOUBLE;
     currentFunction->ArgClasses[1] = vtkstrdup("double");
     currentFunction->ArgCounts[1] = 0;
     currentFunction->ArgTypes[2] = VTK_PARSE_DOUBLE;
     currentFunction->ArgClasses[2] = vtkstrdup("double");
     currentFunction->ArgCounts[2] = 0;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s(double a[3]);",
             $<str>3);
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = VTK_PARSE_DOUBLE_PTR;
     currentFunction->ArgClasses[0] = vtkstrdup("double");
     currentFunction->ArgCounts[0] = 3;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s();", $<str>3);
     sprintf(temps,"Get%s",$<str>3);
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = VTK_PARSE_DOUBLE_PTR;
     currentFunction->ReturnClass = vtkstrdup("double");
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 3;
     output_function();
   }
| TypeMacro '(' any_id ',' any_id ')'
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
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR);
   currentFunction->ReturnClass = vtkstrdup("char");
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
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR);
   currentFunction->ArgClasses[0] = vtkstrdup("char");
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = VTK_PARSE_INT;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "%s *NewInstance();", $<str>3);
   sprintf(temps,"NewInstance");
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = VTK_PARSE_VTK_OBJECT_PTR;
   currentFunction->ReturnClass = vtkstrdup($<str>3);
   output_function();

   if ( data.IsConcrete )
     {
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature, "%s *SafeDownCast(vtkObject* o);",
             $<str>3);
     sprintf(temps,"SafeDownCast");
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = VTK_PARSE_VTK_OBJECT_PTR;
     currentFunction->ArgCounts[0] = 1;
     currentFunction->ArgClasses[0] = vtkstrdup("vtkObject");
     currentFunction->ReturnType = (VTK_PARSE_STATIC | VTK_PARSE_VTK_OBJECT_PTR);
     currentFunction->ReturnClass = vtkstrdup($<str>3);
     output_function();
     }
   }
| TypeMacro '(' any_id ',' any_id ',' ')'
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
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR);
   currentFunction->ReturnClass = vtkstrdup("char");
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
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR);
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = VTK_PARSE_INT;
   currentFunction->ReturnClass = vtkstrdup("int");
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "%s *NewInstance();", $<str>3);
   sprintf(temps,"NewInstance");
   currentFunction->Name = vtkstrdup(temps);
   if (HaveComment)
     {
     currentFunction->Comment = vtkstrdup(CommentText);
     }
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = VTK_PARSE_VTK_OBJECT_PTR;
   currentFunction->ReturnClass = vtkstrdup($<str>3);
   output_function();

   if ( data.IsConcrete )
     {
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature, "%s *SafeDownCast(vtkObject* o);",
             $<str>3);
     sprintf(temps,"SafeDownCast");
     currentFunction->Name = vtkstrdup(temps);
     if (HaveComment)
       {
       currentFunction->Comment = vtkstrdup(CommentText);
       }
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = VTK_PARSE_VTK_OBJECT_PTR;
     currentFunction->ArgCounts[0] = 1;
     currentFunction->ArgClasses[0] = vtkstrdup("vtkObject");
     currentFunction->ReturnType = (VTK_PARSE_STATIC|VTK_PARSE_VTK_OBJECT_PTR);
     currentFunction->ReturnClass = vtkstrdup($<str>3);
     output_function();
     }
   }
;

/*
 * Operators
 */

op_token: '(' ')' { $<str>$ = "operator()"; }
        | '[' ']' { $<str>$ = "operator[]"; }
        | NEW '[' ']' { $<str>$ = "operator new[]"; }
        | DELETE '[' ']' { $<str>$ = "operator delete[]"; }
        | op_token_no_delim;

op_token_no_delim: '=' { $<str>$ = "operator="; }
   | '*' { $<str>$ = "operator*"; } | '/' { $<str>$ = "operator/"; }
   | '-' { $<str>$ = "operator-"; } | '+' { $<str>$ = "operator+"; }
   | '!' { $<str>$ = "operator!"; } | '~' { $<str>$ = "operator~"; }
   | ',' { $<str>$ = "operator,"; } | '<' { $<str>$ = "operator<"; }
   | '>' { $<str>$ = "operator>"; } | '&' { $<str>$ = "operator&"; }
   | '|' { $<str>$ = "operator|"; } | '^' { $<str>$ = "operator^"; }
   | '%' { $<str>$ = "operator%"; }
   | NEW { $<str>$ = "operator new"; }
   | DELETE { $<str>$ = "operator delete"; }
   | OP_LSHIFT_EQ { $<str>$ = "operator<<="; }
   | OP_RSHIFT_EQ { $<str>$ = "operator>>="; }
   | OP_LSHIFT { $<str>$ = "operator<<"; }
   | OP_RSHIFT { $<str>$ = "operator>>"; }
   | OP_ARROW_POINTER { $<str>$ = "operator->*"; }
   | OP_ARROW { $<str>$ = "operator->"; }
   | OP_PLUS_EQ { $<str>$ = "operator+="; }
   | OP_MINUS_EQ { $<str>$ = "operator-="; }
   | OP_TIMES_EQ { $<str>$ = "operator*="; }
   | OP_DIVIDE_EQ { $<str>$ = "operator/="; }
   | OP_REMAINDER_EQ { $<str>$ = "operator%="; }
   | OP_INCR { $<str>$ = "operator++"; }
   | OP_DECR { $<str>$ = "operator--"; }
   | OP_AND_EQ { $<str>$ = "operator&="; }
   | OP_OR_EQ { $<str>$ = "operator|="; }
   | OP_XOR_EQ { $<str>$ = "operator^="; }
   | OP_LOGIC_AND_EQ {$<str>$ = "operator&&=";}
   | OP_LOGIC_OR_EQ {$<str>$ = "operator||=";}
   | OP_LOGIC_AND { $<str>$ = "operator&&"; }
   | OP_LOGIC_OR { $<str>$ = "operator||"; }
   | OP_LOGIC_EQ { $<str>$ = "operator=="; }
   | OP_LOGIC_NEQ { $<str>$ = "operator!="; }
   | OP_LOGIC_LEQ { $<str>$ = "operator<="; }
   | OP_LOGIC_GEQ { $<str>$ = "operator>="; };

/*
 * "VTK_CONSTANT some_value"
 */

vtk_constant_def: VTK_CONSTANT_DEF;

/*
 * These just eat up misc garbage
 */
maybe_other : | other_stuff maybe_other;
maybe_other_no_semi : | other_stuff_no_semi maybe_other_no_semi;
maybe_other_class : | other_stuff_or_class maybe_other_class;

other_stuff_or_class : CLASS | TEMPLATE | other_stuff;

other_stuff : ';' | other_stuff_no_semi | typedef_ignore;

other_stuff_no_semi : OTHER | braces | parens | brackets
   | op_token_no_delim | ':' | '.' | type_red2 | DOUBLE_COLON
   | INT_LITERAL | HEX_LITERAL | FLOAT_LITERAL | CHAR_LITERAL
   | STRING_LITERAL | CLASS_REF | CONST | CONST_PTR | CONST_EQUAL
   | OPERATOR | STATIC | INLINE | VIRTUAL | ENUM | UNION | TYPENAME
   | ARRAY_NUM | VAR_FUNCTION | ELLIPSIS | PUBLIC | PROTECTED | PRIVATE
   | NAMESPACE | USING | vtk_constant_def;

braces: '{' maybe_other_class '}';
parens: '(' maybe_other ')'
       | LPAREN_POINTER maybe_other ')'
       | LPAREN_AMPERSAND maybe_other ')';
brackets: '[' maybe_other ']';
typedef_ignore: TYPEDEF typedef_ignore_body ';';

typedef_ignore_body : | CLASS typedef_ignore_body
                      | other_stuff_no_semi typedef_ignore_body;

%%
#include <string.h>
#include "lex.yy.c"

static void vtkParseDebug(const char* s1, const char* s2)
{
  if ( getenv("DEBUG") )
    {
    fprintf(stderr, "   %s", s1);
    if ( s2 )
      {
      fprintf(stderr, " %s", s2);
      }
    fprintf(stderr, "\n");
    }
}

/* initialize the structure */
void InitFunction(FunctionInfo *func)
{
  func->Name = NULL;
  func->NumberOfArguments = 0;
  func->ArrayFailure = 0;
  func->IsPureVirtual = 0;
  func->IsPublic = 0;
  func->IsOperator = 0;
  func->HaveHint = 0;
  func->HintSize = 0;
  func->ReturnType = VTK_PARSE_VOID;
  func->ReturnClass = NULL;
  func->Comment = NULL;
  func->Signature = NULL;
  func->IsLegacy = 0;
  sigAllocatedLength = 0;
  sigClosed = 0;
  sigMarkDepth = 0;
  sigMark[0] = 0;
}

/* check whether this is the class we are looking for */
void start_class(const char *classname)
{
  if (!strcmp(data.ClassName, classname))
    {
    mainClass = 1;
    currentFunction = data.Functions;
    data.NumberOfFunctions = 0;
    }
  else
    {
    mainClass = 0;
    currentFunction = &throwAwayFunction;
    }
  InitFunction(currentFunction);
}

/* when the cpp file doesn't have enough info use the hint file */
void look_for_hint(void)
{
  char h_cls[80];
  char h_func[80];
  unsigned int  h_type;
  int  h_value;

  /* reset the position */
  if (!fhint)
    {
    return;
    }
  rewind(fhint);

  /* first find a hint */
  while (fscanf(fhint,"%s %s %x %i",h_cls,h_func,&h_type,&h_value) != EOF)
    {
    if ((!strcmp(h_cls,data.ClassName))&&
        currentFunction->Name &&
        (!strcmp(h_func,currentFunction->Name))&&
        ((int)h_type == currentFunction->ReturnType))
      {
      currentFunction->HaveHint = 1;
      currentFunction->HintSize = h_value;
      }
    }
}

/* reject the function, do not output it */
void reject_function()
{
  InitFunction(currentFunction);
}

/* a simple routine that updates a few variables */
void output_function()
{
  int i;

  /* reject template specializations */
  if (currentFunction->Name[strlen(currentFunction->Name)-1] == '>')
    {
    reject_function();
    return;
    }

  /* a void argument is the same as no arguements */
  if ((currentFunction->ArgTypes[0] & VTK_PARSE_UNQUALIFIED_TYPE) ==
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
  currentFunction->IsPublic = in_public;
  currentFunction->IsProtected = in_protected;

  /* look for VAR FUNCTIONS */
  if (currentFunction->NumberOfArguments
      && (currentFunction->ArgTypes[0] == VTK_PARSE_FUNCTION))
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

  /* is it a delete function */
  if (currentFunction->Name && !strcmp("Delete",currentFunction->Name))
    {
    if (mainClass)
      {
      data.HasDelete = 1;
      }
    }


  /* if we need a return type hint and dont currently have one */
  /* then try to find one */
  if (!currentFunction->HaveHint)
    {
    switch (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE)
      {
      case VTK_PARSE_FLOAT_PTR:
      case VTK_PARSE_VOID_PTR:
      case VTK_PARSE_DOUBLE_PTR:
      case VTK_PARSE_ID_TYPE_PTR:
      case VTK_PARSE_LONG_LONG_PTR:
      case VTK_PARSE___INT64_PTR:
      case VTK_PARSE_INT_PTR:
      case VTK_PARSE_SHORT_PTR:
      case VTK_PARSE_LONG_PTR:
      case VTK_PARSE_UNSIGNED_CHAR_PTR:
        look_for_hint();
        break;
      }
    }

  /* reject multi-dimensional arrays from wrappers */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    switch (currentFunction->ArgTypes[i] & VTK_PARSE_INDIRECT)
      {
      case VTK_PARSE_ARRAY_2D:
      case VTK_PARSE_ARRAY_3D:
      case VTK_PARSE_BAD_INDIRECT:
        currentFunction->ArrayFailure = 1;
        break;
      }
    }

  if (mainClass)
    {
    data.NumberOfFunctions++;
    currentFunction = data.Functions + data.NumberOfFunctions;
    }

  InitFunction(currentFunction);
}

void outputSetVectorMacro(
  const char *var, int argType, const char *typeText, int n)
{
  char *argClass = vtkstrdup(getTypeId());
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
  currentFunction->NumberOfArguments = n;
  for (i = 0; i < n; i++)
    {
    currentFunction->ArgTypes[i] = argType;
    currentFunction->ArgClasses[i] = argClass;
    currentFunction->ArgCounts[i] = 0;
    }
  output_function();

  currentFunction->Signature = (char *)malloc(2048);
  sigAllocatedLength = 2048;
  sprintf(currentFunction->Signature, "void Set%s(%s a[%i]);",
          var, local, n);
  currentFunction->Name = name;
  currentFunction->NumberOfArguments = 1;
  currentFunction->ArgTypes[0] = (VTK_PARSE_POINTER | argType);
  currentFunction->ArgClasses[0] = vtkstrdup(getTypeId());
  currentFunction->ArgCounts[0] = n;
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
  currentFunction->NumberOfArguments = 0;
  currentFunction->ReturnType = (VTK_PARSE_POINTER | argType);
  currentFunction->ReturnClass = vtkstrdup(getTypeId());
  currentFunction->HaveHint = 1;
  currentFunction->HintSize = n;
  output_function();

  free(local);
}

extern void vtkParseOutput(FILE *,FileInfo *);

int check_options(int argc, char *argv[])
{
  int i;

  data.IsConcrete = 1;
  data.IsVTKObject = 1;
  hintFileName = 0;

  for (i = 1; i < argc && argv[i][0] == '-'; i++)
    {
    if (strcmp(argv[i], "--concrete") == 0)
      {
      data.IsConcrete = 1;
      }
    else if (strcmp(argv[i], "--abstract") == 0)
      {
      data.IsConcrete = 0;
      }
    else if (strcmp(argv[i], "--vtkobject") == 0)
      {
      data.IsVTKObject = 1;
      }
    else if (strcmp(argv[i], "--special") == 0)
      {
      data.IsVTKObject = 0;
      }
    else if (strcmp(argv[i], "--hints") == 0)
      {
      i++;
      if (i >= argc || argv[i][0] == '-')
        {
        return -1;
        }
      hintFileName = argv[i];
      }
    }

  return i;
}

int main(int argc, char *argv[])
{
  int i, j;
  int argi;
  int has_options = 0;
  FILE *fin;
  int ret;
  FILE *fout;

  argi = check_options(argc, argv);
  if (argi > 1 && argc - argi == 2)
    {
    has_options = 1;
    }
  else if (argi < 0 || argc < 4 || argc > 5)
    {
    fprintf(stderr,
            "Usage: %s [options] input_file output_file\n"
            "  --concrete      concrete class (default)\n"
            "  --abstract      abstract class\n"
            "  --vtkobject     vtkObjectBase-derived class (default)\n"
            "  --special       non-vtkObjectBase class\n"
            "  --hints <file>  hints file\n",
            argv[0]);
    exit(1);
    }

  data.FileName = argv[argi++];
  data.NameComment = NULL;
  data.Description = NULL;
  data.Caveats = NULL;
  data.SeeAlso = NULL;
  CommentState = 0;

  if (!(fin = fopen(data.FileName, "r")))
    {
    fprintf(stderr,"Error opening input file %s\n", data.FileName);
    exit(1);
    }

  /* The class name should be the file name */
  i = strlen(data.FileName);
  j = i;
  while (i > 0)
    {
    --i;
    if (data.FileName[i] == '.')
      {
      j = i;
      }
    if (data.FileName[i] == '/' || data.FileName[i] == '\\')
      {
      i++;
      break;
      }
    }
  data.ClassName = (char *)malloc(j-i + 1);
  strncpy(data.ClassName, &data.FileName[i], j-i);
  data.ClassName[j-i] = '\0';

  /* This will be set to 1 while parsing the main class */
  mainClass = 0;

  currentFunction = &throwAwayFunction;

  if (!has_options)
    {
    if (argc == 5)
      {
      hintFileName = argv[argi++];
      }
    data.IsConcrete = atoi(argv[argi++]);
    }

  if (hintFileName && hintFileName[0] != '\0')
    {
    if (!(fhint = fopen(hintFileName, "r")))
      {
      fprintf(stderr, "Error opening hint file %s\n", hintFileName);
      exit(1);
      }
    }

  yyin = fin;
  yyout = stdout;
  ret = yyparse();
  if (ret)
    {
    fprintf(stdout,
            "*** SYNTAX ERROR found in parsing the header file %s "
            "before line %d ***\n",
            data.FileName, yylineno);
    return ret;
    }

  data.OutputFileName = argv[argi++];
  fout = fopen(data.OutputFileName, "w");

  if (!fout)
    {
    fprintf(stderr, "Error opening output file %s\n", data.OutputFileName);
    exit(1);
    }
  vtkParseOutput(fout, &data);
  fclose(fout);

  return 0;
}

