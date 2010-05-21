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

/* Map from the type enumeration in vtkType.h to the VTK wrapping type
   system number for the type. */

#include "vtkParseType.h"

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
   VTK_PARSE_UNSIGNED___INT64    /* VTK_UNSIGNED___INT64   19 */
  };

/* Define some constants to simplify references to the table lookup in
   the type_primitive production rule code.  */
#include "vtkType.h"
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

int yylex(void);
void start_class(const char *classname);
void output_function(void);
void reject_function(void);

/* vtkstrdup is not part of POSIX so we create our own */
char *vtkstrdup(const char *in)
{
  char *res = malloc(strlen(in)+1);
  strcpy(res,in);
  return res;
}

#include "vtkParse.h"

  FileInfo data;
  FunctionInfo throwAwayFunction;
  FunctionInfo *currentFunction;

  FILE *fhint;
  char temps[2048];
  int  in_public;
  int  in_protected;
  int  HaveComment;
  char CommentText[50000];
  int CommentState;
  int openSig;
  int invertSig;
  unsigned int sigAllocatedLength;
  int mainClass;

#define YYMAXDEPTH 1000

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
  void preSig(const char *arg)
    {
    if (!currentFunction->Signature)
      {
      currentFunction->Signature = (char*)malloc(2048);
      sigAllocatedLength = 2048;
      strcpy(currentFunction->Signature, arg);
      }
    else if (openSig)
      {
      int m, n;
      char *cp;
      checkSigSize(arg);
      cp = currentFunction->Signature;
      m = strlen(cp);
      n = strlen(arg);
      memmove(&cp[n], cp, m+1);
      strncpy(cp, arg, n);
      }
    }
  void postSig(const char *arg)
    {
    if (!currentFunction->Signature)
      {
      currentFunction->Signature = (char*)malloc(2048);
      sigAllocatedLength = 2048;
      strcpy(currentFunction->Signature, arg);
      }
    else if (openSig)
      {
      int m, n;
      char *cp;
      checkSigSize(arg);
      cp = currentFunction->Signature;
      m = strlen(cp);
      n = strlen(arg);
      if (invertSig)
        {
        memmove(&cp[n], cp, m+1);
        strncpy(cp, arg, n);
        }
      else
        {
        strncpy(&cp[m], arg, n+1);
        }
      }
    }
  void preScopeSig(const char *arg)
    {
    if (!currentFunction->Signature)
      {
      currentFunction->Signature = (char*)malloc(2048);
      sigAllocatedLength = 2048;
      strcpy(currentFunction->Signature, arg);
      }
    else if (openSig)
      {
      int i, m, n;
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
             cp[i-1] == '_' || cp[i-1] == ':'))
        {
        i--;
        }
      memmove(&cp[i+n+2], &cp[i], m+1);
      strncpy(&cp[i], arg, n);
      strncpy(&cp[i+n], "::", 2);
      }
    }
  void delSig(void)
    {
    if (currentFunction->Signature)
      {
      free(currentFunction->Signature);
      currentFunction->Signature = NULL;
      }
    }
  void legacySig(void)
    {
    currentFunction->IsLegacy = 1;
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
%token CONST_REF
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
%token IdType
%token StdString
%token UnicodeString
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

class_def: CLASS any_id
      {
        start_class($<str>2);
      }
    optional_scope '{' class_def_body '}'
  | CLASS any_id '<' types '>'
      {
        start_class($<str>2);
      }
    optional_scope '{' class_def_body '}'

class_def_body: | class_def_item class_def_body;

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
   | template_operator func_body { reject_function(); }
   | INLINE template_operator func_body { reject_function(); }
   | function func_body { output_function(); }
   | FRIEND function func_body { reject_function(); }
   | INLINE function func_body { output_function(); }
   | template_function func_body { reject_function(); }
   | INLINE template_function func_body { reject_function(); }
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
 | TYPEDEF CLASS_REF
 | TYPEDEF CLASS any_id '{' maybe_other '}' any_id ';';
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

template: TEMPLATE '<' '>' | TEMPLATE '<' template_args '>';

template_args: template_arg | template_arg ',' template_args;

template_arg: template_type maybe_id | template maybe_id;

template_type: TYPENAME | CLASS | INT;

legacy_function: VTK_LEGACY '(' function ')' ;

function: '~' destructor { preSig("~"); }
      | VIRTUAL '~' destructor { preSig("virtual ~"); }
      | constructor
      | type func
         {
         currentFunction->ReturnType = $<integer>1;
         }
      | type CONST func
         {
         currentFunction->ReturnType = $<integer>1;
         }
      | VIRTUAL type CONST func
         {
         preSig("virtual ");
         currentFunction->ReturnType = $<integer>2;
         }
      | VIRTUAL type func
         {
         preSig("virtual ");
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
      | type CONST op_func
         {
         currentFunction->ReturnType = $<integer>1;
         }
      | VIRTUAL type CONST op_func
         {
         preSig("virtual ");
         currentFunction->ReturnType = $<integer>2;
         }
      | VIRTUAL type op_func
         {
         preSig("virtual ");
         currentFunction->ReturnType = $<integer>2;
         };

typecast_op_func: OPERATOR type '(' { postSig("("); }
                  args_list ')' { postSig(")"); }
                  maybe_const { postSig(";"); openSig = 0; }
    {
      $<integer>$ = $<integer>2;
      openSig = 1;
      currentFunction->IsOperator = 1;
      currentFunction->Name = "operator typecast";
      preSig("operator ");
      vtkParseDebug("Parsed operator", "operator typecast");
    };

op_func: op_sig { postSig(")"); } maybe_const { postSig(";"); openSig = 0; }
    {
      openSig = 1;
      currentFunction->Name = $<str>2;
      vtkParseDebug("Parsed operator", $<str>2);
    }
  | op_sig pure_virtual
    {
      currentFunction->Name = $<str>2;
      vtkParseDebug("Parsed operator", $<str>2);
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
    }
    args_list ')';

func: func_sig { postSig(")"); } maybe_const { postSig(";"); openSig = 0; }
    {
      openSig = 1;
      currentFunction->Name = $<str>1;
      vtkParseDebug("Parsed func", $<str>1);
    }
  | func_sig pure_virtual
    {
      currentFunction->Name = $<str>1;
      vtkParseDebug("Parsed func", $<str>1);
      currentFunction->IsPureVirtual = 1;
      if (mainClass)
        {
        data.IsAbstract = 1;
        }
    };

pure_virtual: '=' INT_LITERAL {postSig(") = 0;");}
            | CONST_EQUAL INT_LITERAL {postSig(") const = 0;");}

maybe_const: | CONST {postSig(" const");};

func_sig: any_id '(' {postSig("("); } args_list ')';

constructor: constructor_sig { postSig(")"); } maybe_initializers
    { postSig(";"); openSig = 0; }
    {
      openSig = 1;
      currentFunction->Name = $<str>1;
      vtkParseDebug("Parsed func", $<str>1);
    }

constructor_sig: any_id '(' { postSig("("); } args_list ')';

maybe_initializers: | ':' initializer more_initializers;

more_initializers: | ',' initializer more_initializers;

initializer: any_id '(' maybe_other ')';

destructor: destructor_sig { postSig(");"); openSig = 0; }
    {
      openSig = 1;
      currentFunction->Name = (char *)malloc(strlen($<str>1) + 2);
      currentFunction->Name[0] = '~';
      strcpy(&currentFunction->Name[1], $<str>1);
      vtkParseDebug("Parsed func", currentFunction->Name);
    }

destructor_sig: any_id '(' { postSig("(");} args_list ')';

const_mod: CONST {postSig("const ");};

static_mod: STATIC {postSig("static ");}
           | STATIC INLINE {postSig("static ");}

any_id: VTK_ID {postSig($<str>1);} | ID {postSig($<str>1);};

func_body: ';'
    | '{' maybe_other '}' ';'
    | '{' maybe_other '}';

ignore_args_list: | ignore_more_args;

ignore_more_args: arg | arg { postSig(", ");} ',' ignore_more_args;

args_list: | more_args;

more_args: ELLIPSIS | arg { currentFunction->NumberOfArguments++;}
  | arg { currentFunction->NumberOfArguments++; postSig(", ");} ',' more_args;

arg: type maybe_var_array
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] =
        $<integer>2 / VTK_PARSE_COUNT_START;
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] =
        $<integer>1 + ($<integer>2 % VTK_PARSE_COUNT_START);
    }
  | type var_id
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] =
        $<integer>2 / VTK_PARSE_COUNT_START;
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] =
        $<integer>1 + ($<integer>2 % VTK_PARSE_COUNT_START);
    } maybe_var_assign
  | VAR_FUNCTION
    {
      postSig("void (*func)(void *) ");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0;
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = VTK_PARSE_FUNCTION;
    }
  | type LPAREN_AMPERSAND { postSig("(&"); } maybe_id ')'
    {
      postSig(") ");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0;
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = VTK_PARSE_UNKNOWN;
    }
  | type LPAREN_POINTER { postSig("("); postSig($<str>2); postSig("*"); }
    maybe_id ')' '(' { postSig(")("); } ignore_args_list ')'
    {
      postSig(")");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0;
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = VTK_PARSE_UNKNOWN;
    };

maybe_id: | any_id;

maybe_var_assign: | var_assign;

var_assign: '=' literal {postSig("="); postSig($<str>2);};

var: CLASS any_id var_ids ';' {delSig();}
     | CLASS any_id type_indirection var_ids ';' {delSig();}
     | ENUM any_id var_ids ';' {delSig();}
     | ENUM any_id type_indirection var_ids ';' {delSig();}
     | UNION any_id var_ids ';' {delSig();}
     | UNION any_id type_indirection var_ids ';' {delSig();}
     | type var_ids ';' {delSig();}
     | type CONST var_ids ';' {delSig();}
     | VAR_FUNCTION ';' {delSig();}
     | STATIC VAR_FUNCTION ';' {delSig();}
     | type LPAREN_AMPERSAND any_id ')' ';' {delSig();}
     | type LPAREN_POINTER any_id ')' ';' {delSig();}
     | type LPAREN_POINTER any_id ')' ARRAY_NUM ';' {delSig();}
     | type LPAREN_POINTER any_id ')' '[' maybe_other ']' ';' {delSig();}
     | type LPAREN_POINTER any_id ')' '(' ignore_args_list ')' ';' {delSig();};

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

type: const_mod type_red1 {$<integer>$ = (VTK_PARSE_CONST | $<integer>2);}
          | type_red1 {$<integer>$ = $<integer>1;}
          | static_mod type_red1
            {$<integer>$ = (VTK_PARSE_STATIC | $<integer>2);}
          | static_mod const_mod type_red1
            {$<integer>$ = (VTK_PARSE_CONST|VTK_PARSE_STATIC | $<integer>3);};

type_red1: type_red2 {$<integer>$ = $<integer>1;}
         | type_red2 type_indirection
           {$<integer>$ = ($<integer>1 | $<integer>2);}
         | templated_id {postSig(" "); $<integer>$ = VTK_PARSE_UNKNOWN;}
         | templated_id {postSig(" ");} type_indirection
           {$<integer>$ = VTK_PARSE_UNKNOWN;}
         | scoped_id {postSig(" "); $<integer>$ = VTK_PARSE_UNKNOWN;}
         | scoped_id {postSig(" ");} type_indirection
           {$<integer>$ = VTK_PARSE_UNKNOWN;}
         | TYPENAME scoped_id {postSig(" "); $<integer>$ = VTK_PARSE_UNKNOWN;}
         | TYPENAME scoped_id {postSig(" ");} type_indirection
           {$<integer>$ = VTK_PARSE_UNKNOWN;};

templated_id: VTK_ID '<' {postSig($<str>1); postSig("<");} types
              '>' { $<str>$ = $<str>1; postSig(">");}
            | ID '<' {postSig($<str>1); postSig("<");} types
              '>' {$<str>$ = $<str>1; postSig(">");};

types: type | type ',' {postSig(", ");} types;

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

/* &   is VTK_PARSE_REF
   *   is VTK_PARSE_POINTER
   *&  is VTK_PARSE_POINTER_REF
   * const&  is VTK_PARSE_POINTER_CONST_REF
   **  is VTK_PARSE_POINTER_POINTER
   * const*  is VTK_PARSE_POINTER_CONST_POINTER
   everything else is VTK_PARSE_BAD_INDIRECT
   */

type_indirection: '&' { postSig("&"); $<integer>$ = VTK_PARSE_REF;}
  | '*' { postSig("*"); $<integer>$ = VTK_PARSE_POINTER;}
  | '*' '&' { postSig("*&"); $<integer>$ = VTK_PARSE_POINTER_REF;}
  | '*' '*' { postSig("**"); $<integer>$ = VTK_PARSE_POINTER_POINTER;}
  | '*' CONST_REF
    { postSig("* const&"); $<integer>$ = VTK_PARSE_POINTER_CONST_REF;}
  | '*' CONST_PTR
    { postSig("* const*"); $<integer>$ = VTK_PARSE_POINTER_CONST_POINTER;}
  | CONST_REF { postSig("const&"); $<integer>$ = VTK_PARSE_BAD_INDIRECT;}
  | CONST_PTR { postSig("const*"); $<integer>$ = VTK_PARSE_BAD_INDIRECT;}
  | '*' '*' { postSig("**"); } type_indirection
    { $<integer>$ = VTK_PARSE_BAD_INDIRECT;}
  | '*' '&' { postSig("**"); } type_indirection
    { $<integer>$ = VTK_PARSE_BAD_INDIRECT;}
  | CONST_REF { postSig("const&");} type_indirection
    { $<integer>$ = VTK_PARSE_BAD_INDIRECT;}
  | CONST_PTR { postSig("const*");} type_indirection
    { $<integer>$ = VTK_PARSE_BAD_INDIRECT;};

type_red2:  type_primitive { $<integer>$ = $<integer>1;}
 | StdString { postSig("vtkStdString "); $<integer>$ = VTK_PARSE_STRING;}
 | UnicodeString
   { postSig("vtkUnicodeString "); $<integer>$ = VTK_PARSE_UNICODE_STRING;}
 | OSTREAM { postSig("ostream "); $<integer>$ = VTK_PARSE_UNKNOWN;}
 | ISTREAM { postSig("istream "); $<integer>$ = VTK_PARSE_UNKNOWN;}
 | ID
    {
      postSig($<str>1);
      postSig(" ");
      $<integer>$ = VTK_PARSE_UNKNOWN;
    }
 | VTK_ID
    {
      postSig($<str>1);
      postSig(" ");
      currentFunction->ArgClasses[currentFunction->NumberOfArguments] =
        vtkstrdup($<str>1);
      if ((!currentFunction->ReturnClass) &&
          (!currentFunction->NumberOfArguments))
        {
        currentFunction->ReturnClass = vtkstrdup($<str>1);
        }
      $<integer>$ = VTK_PARSE_VTK_OBJECT;
    };

type_primitive:
  VOID   { postSig("void "); $<integer>$ = VTK_PARSE_VOID;} |
  FLOAT  { postSig("float "); $<integer>$ = VTK_PARSE_FLOAT;} |
  DOUBLE { postSig("double "); $<integer>$ = VTK_PARSE_DOUBLE;} |
  BOOL { postSig("bool "); $<integer>$ = VTK_PARSE_BOOL;} |
  SIGNED_CHAR {postSig("signed char "); $<integer>$ = VTK_PARSE_SIGNED_CHAR;} |
  TypeInt8 { postSig("vtkTypeInt8 "); $<integer>$ = VTK_PARSE_INT8; } |
  TypeUInt8 { postSig("vtkTypeUInt8 "); $<integer>$ = VTK_PARSE_UINT8; } |
  TypeInt16 { postSig("vtkTypeInt16 "); $<integer>$ = VTK_PARSE_INT16; } |
  TypeUInt16 { postSig("vtkTypeUInt16 "); $<integer>$ = VTK_PARSE_UINT16; } |
  TypeInt32 { postSig("vtkTypeInt32 "); $<integer>$ = VTK_PARSE_INT32; } |
  TypeUInt32 { postSig("vtkTypeUInt32 "); $<integer>$ = VTK_PARSE_UINT32; } |
  TypeInt64 { postSig("vtkTypeInt64 "); $<integer>$ = VTK_PARSE_INT64; } |
  TypeUInt64 { postSig("vtkTypeUInt64 "); $<integer>$ = VTK_PARSE_UINT64; } |
  TypeFloat32 { postSig("vtkTypeFloat32 "); $<integer>$ = VTK_PARSE_FLOAT32; } |
  TypeFloat64 { postSig("vtkTypeFloat64 "); $<integer>$ = VTK_PARSE_FLOAT64; } |
  UNSIGNED {postSig("unsigned ");}
   type_integer { $<integer>$ = (VTK_PARSE_UNSIGNED | $<integer>3);} |
  type_integer { $<integer>$ = $<integer>1;};

type_integer:
  CHAR   { postSig("char "); $<integer>$ = VTK_PARSE_CHAR;} |
  INT    { postSig("int "); $<integer>$ = VTK_PARSE_INT;} |
  SHORT  { postSig("short "); $<integer>$ = VTK_PARSE_SHORT;} |
  LONG   { postSig("long "); $<integer>$ = VTK_PARSE_LONG;} |
  IdType { postSig("vtkIdType "); $<integer>$ = VTK_PARSE_ID_TYPE;} |
  LONG_LONG { postSig("long long "); $<integer>$ = VTK_PARSE_LONG_LONG;} |
  INT64__ { postSig("__int64 "); $<integer>$ = VTK_PARSE___INT64;};

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
  SetMacro '(' any_id ','
           {preSig("void Set"); postSig("("); } type_red1 ')'
   {
   postSig(");");
   sprintf(temps,"Set%s",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = $<integer>6;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = VTK_PARSE_VOID;
   output_function();
   }
| GetMacro '('{postSig("Get");} any_id ',' {postSig("();"); invertSig = 1;}
    type_red1 ')'
   {
   sprintf(temps,"Get%s",$<str>4);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = $<integer>7;
   output_function();
   }
| SetStringMacro '(' {preSig("void Set");} any_id ')'
   {
   postSig("(char *);");
   sprintf(temps,"Set%s",$<str>4);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = VTK_PARSE_CHAR_PTR;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = VTK_PARSE_VOID;
   output_function();
   }
| GetStringMacro '(' {preSig("char *Get");} any_id ')'
   {
   postSig("();");
   sprintf(temps,"Get%s",$<str>4);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = VTK_PARSE_CHAR_PTR;
   output_function();
   }
| SetClampMacro  '(' any_id ','
    {preSig("void Set"); postSig("("); } type_red2
    {postSig(");"); openSig = 0;} ',' maybe_other_no_semi ')'
   {
   char *local = vtkstrdup(currentFunction->Signature);
   sscanf(currentFunction->Signature, "%*s %*s(%s);", local);
   sprintf(temps,"Set%s",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = $<integer>6;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = VTK_PARSE_VOID;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%s Get%sMinValue();",local,$<str>3);
   sprintf(temps,"Get%sMinValue",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = $<integer>6;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%s Get%sMaxValue();",local,$<str>3);
   sprintf(temps,"Get%sMaxValue",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = $<integer>6;
   output_function();
   }
| SetObjectMacro '(' any_id ','
  {preSig("void Set"); postSig("("); } type_red2 ')'
   {
   postSig("*);");
   sprintf(temps,"Set%s",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = VTK_PARSE_VTK_OBJECT_PTR;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = VTK_PARSE_VOID;
   output_function();
   }
| GetObjectMacro '(' {postSig("*Get");} any_id ','
   {postSig("();"); invertSig = 1;} type_red2 ')'
   {
   sprintf(temps,"Get%s",$<str>4);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = VTK_PARSE_VTK_OBJECT_PTR;
   output_function();
   }
| BooleanMacro '(' any_id
    {preSig("void "); postSig("On();"); openSig = 0; }
        ',' type_red2 ')'
   {
   sprintf(temps,"%sOn",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = VTK_PARSE_VOID;
   output_function();
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void %sOff();",$<str>3);
   sprintf(temps,"%sOff",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   output_function();
   }
| SetVector2Macro '(' any_id ','
     {
     free(currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
      type_red2 ')'
   {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s(%s, %s);",$<str>3,
     local, local);
   sprintf(temps,"Set%s",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 2;
   currentFunction->ArgTypes[0] = $<integer>6;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = $<integer>6;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ReturnType = VTK_PARSE_VOID;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s(%s a[2]);",$<str>3,
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (VTK_PARSE_POINTER | $<integer>6);
   currentFunction->ArgCounts[0] = 2;
   output_function();
   }
| GetVector2Macro  '(' any_id ','
     {
     free(currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
      type_red2 ')'
   {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s();",local, $<str>3);
   sprintf(temps,"Get%s",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (VTK_PARSE_POINTER | $<integer>6);
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 2;
   output_function();
   }
| SetVector3Macro '(' any_id ','
     {
     free(currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
      type_red2 ')'
   {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s(%s, %s, %s);",
     $<str>3, local, local, local);
   sprintf(temps,"Set%s",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 3;
   currentFunction->ArgTypes[0] = $<integer>6;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = $<integer>6;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = $<integer>6;
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ReturnType = VTK_PARSE_VOID;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s(%s a[3]);",$<str>3,
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (VTK_PARSE_POINTER | $<integer>6);
   currentFunction->ArgCounts[0] = 3;
   output_function();
   }
| GetVector3Macro  '(' any_id ','
     {
     free(currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
      type_red2 ')'
   {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s();",local, $<str>3);
   sprintf(temps,"Get%s",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (VTK_PARSE_POINTER | $<integer>6);
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 3;
   output_function();
   }
| SetVector4Macro '(' any_id ','
     {
     free(currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
      type_red2 ')'
   {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s(%s, %s, %s, %s);",
     $<str>3, local, local, local, local);
   sprintf(temps,"Set%s",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 4;
   currentFunction->ArgTypes[0] = $<integer>6;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = $<integer>6;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = $<integer>6;
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ArgTypes[3] = $<integer>6;
   currentFunction->ArgCounts[3] = 0;
   currentFunction->ReturnType = VTK_PARSE_VOID;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s(%s a[4]);",$<str>3,
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (VTK_PARSE_POINTER | $<integer>6);
   currentFunction->ArgCounts[0] = 4;
   output_function();
   }
| GetVector4Macro  '(' any_id ','
     {
     free(currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
      type_red2 ')'
   {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s();",local, $<str>3);
   sprintf(temps,"Get%s",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (VTK_PARSE_POINTER | $<integer>6);
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 4;
   output_function();
   }
| SetVector6Macro '(' any_id ','
     {
     free(currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
      type_red2 ')'
   {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s(%s, %s, %s, %s, %s, %s);",
     $<str>3, local, local, local, local, local, local);
   sprintf(temps,"Set%s",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 6;
   currentFunction->ArgTypes[0] = $<integer>6;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = $<integer>6;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = $<integer>6;
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ArgTypes[3] = $<integer>6;
   currentFunction->ArgCounts[3] = 0;
   currentFunction->ArgTypes[4] = $<integer>6;
   currentFunction->ArgCounts[4] = 0;
   currentFunction->ArgTypes[5] = $<integer>6;
   currentFunction->ArgCounts[5] = 0;
   currentFunction->ReturnType = VTK_PARSE_VOID;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s(%s a[6]);",$<str>3,
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (VTK_PARSE_POINTER | $<integer>6);
   currentFunction->ArgCounts[0] = 6;
   output_function();
   }
| GetVector6Macro  '(' any_id ','
     {
     free(currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
      type_red2 ')'
   {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s();",local, $<str>3);
   sprintf(temps,"Get%s",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (VTK_PARSE_POINTER | $<integer>6);
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 6;
   output_function();
   }
| SetVectorMacro  '(' any_id ','
      {
      free(currentFunction->Signature);
      currentFunction->Signature = NULL;
      }
     type_red2 ',' INT_LITERAL ')'
   {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s(%s [%s]);",$<str>3,
      local, $<str>8);
     sprintf(temps,"Set%s",$<str>3);
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->ReturnType = VTK_PARSE_VOID;
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = (VTK_PARSE_POINTER | $<integer>6);
     currentFunction->ArgCounts[0] = atol($<str>8);
     output_function();
   }
| GetVectorMacro  '(' any_id ','
     {
     free(currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
     type_red2 ',' INT_LITERAL ')'
   {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s();",local, $<str>3);
   sprintf(temps,"Get%s",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (VTK_PARSE_POINTER | $<integer>6);
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = atol($<str>8);
   output_function();
   }
| ViewportCoordinateMacro '(' any_id ')'
   {
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate();",
       $<str>3);

     sprintf(temps,"Get%sCoordinate",$<str>3);
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = VTK_PARSE_VTK_OBJECT_PTR;
     currentFunction->ReturnClass = vtkstrdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s(double, double);",
       $<str>3);
     sprintf(temps,"Set%s",$<str>3);
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 2;
     currentFunction->ArgTypes[0] = VTK_PARSE_DOUBLE;
     currentFunction->ArgCounts[0] = 0;
     currentFunction->ArgTypes[1] = VTK_PARSE_DOUBLE;
     currentFunction->ArgCounts[1] = 0;
     currentFunction->ReturnType = VTK_PARSE_VOID;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s(double a[2]);",
       $<str>3);
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = VTK_PARSE_DOUBLE_PTR;
     currentFunction->ArgCounts[0] = 2;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s();", $<str>3);
     sprintf(temps,"Get%s",$<str>3);
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = VTK_PARSE_DOUBLE_PTR;
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
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = VTK_PARSE_VTK_OBJECT_PTR;
     currentFunction->ReturnClass = vtkstrdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s(double, double, double);",
       $<str>3);
     sprintf(temps,"Set%s",$<str>3);
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 3;
     currentFunction->ArgTypes[0] = VTK_PARSE_DOUBLE;
     currentFunction->ArgCounts[0] = 0;
     currentFunction->ArgTypes[1] = VTK_PARSE_DOUBLE;
     currentFunction->ArgCounts[1] = 0;
     currentFunction->ArgTypes[2] = VTK_PARSE_DOUBLE;
     currentFunction->ArgCounts[2] = 0;
     currentFunction->ReturnType = VTK_PARSE_VOID;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s(double a[3]);",
       $<str>3);
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = VTK_PARSE_DOUBLE_PTR;
     currentFunction->ArgCounts[0] = 3;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s();", $<str>3);
     sprintf(temps,"Get%s",$<str>3);
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = VTK_PARSE_DOUBLE_PTR;
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
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR);
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,
           "int IsA(const char *name);");
   sprintf(temps,"IsA");
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR);
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = VTK_PARSE_INT;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "%s *NewInstance();",
           $<str>3);
   sprintf(temps,"NewInstance");
   currentFunction->Name = vtkstrdup(temps);
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
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR);
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,
           "int IsA(const char *name);");
   sprintf(temps,"IsA");
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR);
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = VTK_PARSE_INT;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "%s *NewInstance();",
           $<str>3);
   sprintf(temps,"NewInstance");
   currentFunction->Name = vtkstrdup(temps);
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
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = VTK_PARSE_VTK_OBJECT_PTR;
     currentFunction->ArgCounts[0] = 1;
     currentFunction->ArgClasses[0] = vtkstrdup("vtkObject");
     currentFunction->ReturnType = (VTK_PARSE_STATIC | VTK_PARSE_VTK_OBJECT_PTR);
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
   | STRING_LITERAL | CLASS_REF | CONST | CONST_PTR | CONST_REF | CONST_EQUAL
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
  openSig = 1;
  invertSig = 0;
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

  /* a void argument is the same as no arguements */
  if ((currentFunction->ArgTypes[0] & VTK_PARSE_UNQUALIFIED_TYPE) ==
      VTK_PARSE_VOID)
    {
    currentFunction->NumberOfArguments = 0;
    }

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

  if (HaveComment)
    {
    currentFunction->Comment = vtkstrdup(CommentText);
    }

  if (mainClass)
    {
    data.NumberOfFunctions++;
    currentFunction = data.Functions + data.NumberOfFunctions;
    }

  InitFunction(currentFunction);
}

extern void vtkParseOutput(FILE *,FileInfo *);

int main(int argc,char *argv[])
{
  int i, j;
  FILE *fin;
  int ret;
  FILE *fout;

  if (argc < 4 || argc > 5)
    {
    fprintf(stderr,
            "Usage: %s input_file <hint_file> is_concrete output_file\n",argv[0]);
    exit(1);
    }

  if (!(fin = fopen(argv[1],"r")))
    {
    fprintf(stderr,"Error opening input file %s\n",argv[1]);
    exit(1);
    }

  fhint = 0;
  data.FileName = argv[1];
  data.NameComment = NULL;
  data.Description = NULL;
  data.Caveats = NULL;
  data.SeeAlso = NULL;
  CommentState = 0;

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

  if (argc == 5)
    {
    if (!(fhint = fopen(argv[2],"r")))
      {
      fprintf(stderr,"Error opening hint file %s\n",argv[2]);
      exit(1);
      }
    data.IsConcrete = atoi(argv[3]);
    }
  else
    {
    data.IsConcrete = atoi(argv[2]);
    }

  yyin = fin;
  yyout = stdout;
  ret = yyparse();
  if (ret)
    {
    fprintf(stdout,
            "*** SYNTAX ERROR found in parsing the header file %s before line %d ***\n",
            argv[1], yylineno);
    return ret;
    }

  if (argc == 5)
    {
    fout = fopen(argv[4],"w");
    data.OutputFileName = argv[4];
    }
  else
    {
    fout = fopen(argv[3],"w");
    data.OutputFileName = argv[3];
    }

  if (!fout)
    {
    fprintf(stderr,"Error opening output file %s\n",argv[3]);
    exit(1);
    }
  vtkParseOutput(fout,&data);
  fclose (fout);

  return 0;
}

