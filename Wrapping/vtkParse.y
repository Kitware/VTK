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
   system number for the type.  Note that the wrapping type system
   does not enumerate its type values by name.  Look in the
   type_primitive production rule in the grammar for the "official"
   enumeration. */
static int vtkParseTypeMap[] =
  {
   0x2,  /* VTK_VOID                0 */
   0,    /* VTK_BIT                 1 */
   0x3,  /* VTK_CHAR                2 */
   0x13, /* VTK_UNSIGNED_CHAR       3 */
   0x5,  /* VTK_SHORT               4 */
   0x15, /* VTK_UNSIGNED_SHORT      5 */
   0x4,  /* VTK_INT                 6 */
   0x14, /* VTK_UNSIGNED_INT        7 */
   0x6,  /* VTK_LONG                8 */
   0x16, /* VTK_UNSIGNED_LONG       9 */
   0x1,  /* VTK_FLOAT              10 */
   0x7,  /* VTK_DOUBLE             11 */
   0xA,  /* VTK_ID_TYPE            12 */
   0,    /* VTK_STRING             13 */
   0,    /* VTK_OPAQUE             14 */
   0xD,  /* VTK_SIGNED_CHAR        15 */
   0xB,  /* VTK_LONG_LONG          16 */
   0x1B, /* VTK_UNSIGNED_LONG_LONG 17 */
   0xC,  /* VTK___INT64            18 */
   0x1C  /* VTK_UNSIGNED___INT64   19 */
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
void output_function(void);

/* vtkstrdup is not part of POSIX so we create our own */
char *vtkstrdup(const char *in)
{
  char *res = malloc(strlen(in)+1);
  strcpy(res,in);
  return res;
}

#include "vtkParse.h"
    
  FileInfo data;
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
  
#define YYMAXDEPTH 1000

  void checkSigSize(const char *arg)
    {
    if (strlen(currentFunction->Signature) + strlen(arg) + 3 > 
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
      sprintf(currentFunction->Signature,"%s",arg);
      }    
    else if (openSig)
      {
      char *tmp;
      checkSigSize(arg);
      tmp = vtkstrdup(currentFunction->Signature);
      sprintf(currentFunction->Signature,"%s%s",arg,tmp);
      free(tmp);
      }
    }
  void postSig(const char *arg)
    {
    if (!currentFunction->Signature)
      {
      currentFunction->Signature = (char*)malloc(2048);
      sigAllocatedLength = 2048; 
      sprintf(currentFunction->Signature,"%s",arg);
      }    
    else if (openSig)
      {
      char *tmp;
      checkSigSize(arg);
      tmp = vtkstrdup(currentFunction->Signature);
      if (invertSig)
        {
        sprintf(currentFunction->Signature,"%s%s",arg,tmp);
        }
      else
        {
        sprintf(currentFunction->Signature,"%s%s",tmp,arg);
        }
      free(tmp);
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
  struct {
    char* name;
    int external;
  } vtkid;
}

%token CLASS
%token PUBLIC
%token PRIVATE
%token PROTECTED
%token VIRTUAL
%token <str> STRING
%token <integer> NUM
%token <str> ID
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
%token CLASS_REF
%token OTHER
%token CONST
%token OPERATOR
%token UNSIGNED
%token FRIEND
%token <str> VTK_ID
%token STATIC
%token VAR_FUNCTION
%token ARRAY_NUM
%token VTK_LEGACY
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
%token SetMacro
%token GetMacro
%token SetStringMacro
%token GetStringMacro
%token SetClampMacro
%token SetObjectMacro
%token SetReferenceCountedObjectMacro
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
%token VTK_WRAP_EXTERN

%%
/*
 * Here is the start of the grammer
 */
strt: maybe_other class_def maybe_other;

vtk_id : VTK_ID VTK_WRAP_EXTERN 
         {
           ($<vtkid>$).name = $<str>1;
           ($<vtkid>$).external = 1;
         }
       | VTK_ID 
         {
           ($<vtkid>$).name = $<str>1;
           ($<vtkid>$).external = 0;
         };

class_def : CLASS VTK_ID 
      {
      data.ClassName = vtkstrdup($2);
      }
    optional_scope '{' class_def_body '}';

class_def_body: class_def_item | class_def_item class_def_body;

class_def_item: scope_type ':' | var
   | operator
   | FRIEND operator
   | function func_body { output_function(); }
   | FRIEND function func_body { output_function(); }
   | legacy_function func_body { legacySig(); output_function(); }
   | macro ';'
   | macro;

legacy_function: VTK_LEGACY '(' function ')'

function: '~' func { preSig("~"); } 
      | VIRTUAL '~' func { preSig("virtual ~"); }
      | func
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
         }
      | VIRTUAL func
         {
         preSig("virtual ");
         };

operator:
        operator_sig
         {
         output_function();
         }
      | type operator_sig
         {
         currentFunction->ReturnType = $<integer>1;
         output_function();
         }
      | type CONST operator_sig
         {
         currentFunction->ReturnType = $<integer>1;
         output_function();
         }
      | VIRTUAL type CONST operator_sig
         {
         preSig("virtual ");
         currentFunction->ReturnType = $<integer>2;
         output_function();
         }
      | VIRTUAL type operator_sig
         {
         preSig("virtual ");
         currentFunction->ReturnType = $<integer>2;
         output_function();
         }
      | VIRTUAL operator_sig
         {
         preSig("virtual ");
         output_function();
         };

operator_sig: OPERATOR maybe_other_no_semi ';'
    {
      currentFunction->IsOperator = 1;
      vtkParseDebug("Converted operator", 0);
    }

func: func_sig { postSig(")"); } maybe_const { postSig(";"); openSig = 0; } 
    {
      openSig = 1;
      currentFunction->Name = $<str>1; 
      vtkParseDebug("Parsed func", $<str>1);
    }
  | func_sig '=' NUM
    { 
      postSig(") = 0;"); 
      currentFunction->Name = $<str>1; 
      vtkParseDebug("Parsed func", $<str>1);
      currentFunction->IsPureVirtual = 1; 
      data.IsAbstract = 1;
    };

maybe_const: | CONST {postSig(" const");};

func_sig: any_id '('  {postSig(" ("); } args_list ')';

const_mod: CONST {postSig("const ");};

static_mod: STATIC {postSig("static ");};

any_id: vtk_id {postSig(($<vtkid>1).name);} | ID {postSig($<str>1);};

func_body: ';' 
    | '{' maybe_other '}' ';' 
    | '{' maybe_other '}'  
    | ':' maybe_other_no_semi ';';

args_list: | more_args;

more_args: arg { currentFunction->NumberOfArguments++;} 
  | arg { currentFunction->NumberOfArguments++; postSig(", ");} ',' more_args;

arg: type 
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
        $<integer>1;} 
  | type var_id 
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 
        $<integer>2 / 0x10000; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
        $<integer>1 + $<integer>2 % 0x10000;
    } opt_var_assign
  | VAR_FUNCTION 
    { 
      postSig("void (*func)(void *) ");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 0x5000;
    };

opt_var_assign: | '=' float_num;

var:  type var_id ';' {delSig();} | VAR_FUNCTION ';' {delSig();};

var_id: any_id var_array { $<integer>$ = $<integer>2; };

/*
 0x300 = [n] 
 0x600 = [n][m]
 0x900 = [n][m][l]
*/

var_array: { $<integer>$ = 0; }
     | ARRAY_NUM { char temp[100]; sprintf(temp,"[%i]",$<integer>1); 
                   postSig(temp); } 
       var_array { $<integer>$ = 0x300 + 0x10000 * $<integer>1 + $<integer>3 % 0x1000; } 
     | '[' maybe_other_no_semi ']' var_array 
           { postSig("[]"); $<integer>$ = 0x300 + $<integer>4 % 0x1000; };

type: const_mod type_red1 {$<integer>$ = 0x1000 + $<integer>2;} 
          | type_red1 {$<integer>$ = $<integer>1;}
          | static_mod type_red1 {$<integer>$ = 0x2000 + $<integer>2;}
          | static_mod const_mod type_red1 {$<integer>$ = 0x3000 + $<integer>3;}; 

type_red1: type_red2 {$<integer>$ = $<integer>1;} 
         | type_red2 type_indirection 
           {$<integer>$ = $<integer>1 + $<integer>2;}
         | type_string1 {$<integer>$ = $<integer>1;};

type_string1: type_string2 {$<integer>$ = $<integer>1;}
         | type_string2 '&' { postSig("&"); $<integer>$ = $<integer>1;}
         | type_string2 '*' { postSig("*"); $<integer>$ = 0x400 + $<integer>1;}

type_string2: StdString { postSig("vtkStdString "); $<integer>$ = 0x1303; }; 

 
/* 0x100 = &
   0x200 = &&
   0x300 = *
   0x400 = &*
   0x500 = *&
   0x700 = **
   */
type_indirection: '&' { postSig("&"); $<integer>$ = 0x100;} 
                | '*' { postSig("*"); $<integer>$ = 0x300;} 
                | '&' type_indirection { $<integer>$ = 0x100 + $<integer>2;}
                | '*' type_indirection { $<integer>$ = 0x400 + $<integer>2;};

type_red2: UNSIGNED {postSig("unsigned ");} 
             type_primitive { $<integer>$ = 0x10 + $<integer>3;} 
                  | type_primitive { $<integer>$ = $<integer>1;};

type_primitive: 
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
  FLOAT  { postSig("float "); $<integer>$ = 0x1;} | 
  VOID   { postSig("void "); $<integer>$ = 0x2;} | 
  CHAR   { postSig("char "); $<integer>$ = 0x3;} | 
  INT    { postSig("int "); $<integer>$ = 0x4;} | 
  SHORT  { postSig("short "); $<integer>$ = 0x5;} | 
  LONG   { postSig("long "); $<integer>$ = 0x6;} | 
  DOUBLE { postSig("double "); $<integer>$ = 0x7;} | 
  ID     {       
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",$<str>1);
      postSig(ctmpid);
      $<integer>$ = 0x8;} |
  vtk_id  
    { 
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",($<vtkid>1).name);
      postSig(ctmpid);
      $<integer>$ = 0x9; 
      currentFunction->ArgClasses[currentFunction->NumberOfArguments] =
        vtkstrdup(($<vtkid>1).name); 
      currentFunction->ArgExternals[currentFunction->NumberOfArguments] =
        ($<vtkid>1).external;
      /* store the string into the return value just in case we need it */
      /* this is a parsing hack because the first "type" parser will */
      /* possibly be ht ereturn type of the first argument */
      if ((!currentFunction->ReturnClass) && 
          (!currentFunction->NumberOfArguments)) 
        { 
        currentFunction->ReturnClass = vtkstrdup(($<vtkid>1).name); 
        currentFunction->ReturnExternal = ($<vtkid>1).external;
        }
    } |
  IdType { postSig("vtkIdType "); $<integer>$ = 0xA;} |
  LONG_LONG { postSig("long long "); $<integer>$ = 0xB;} |
  INT64__ { postSig("__int64 "); $<integer>$ = 0xC;} |
  SIGNED_CHAR { postSig("signed char "); $<integer>$ = 0xD;} |
  BOOL { postSig("bool "); $<integer>$ = 0xE;};

optional_scope: | ':' scope_list;

scope_list: scope_type VTK_ID 
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup($2); 
      data.NumberOfSuperClasses++; 
    } 
  | scope_type VTK_ID 
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup($2); 
      data.NumberOfSuperClasses++; 
    } ',' scope_list;

scope_type: PUBLIC {in_public = 1; in_protected = 0;} 
          | PRIVATE {in_public = 0; in_protected = 0;} 
          | PROTECTED {in_public = 0; in_protected = 1;};

float_num: '-' float_prim | float_prim;

float_prim: NUM {$<integer>$ = $1;} 
         | NUM '.' NUM {$<integer>$ = -1;} | any_id {$<integer>$ = -1;};

macro:
  SetMacro '(' any_id ',' 
           {preSig("void Set"); postSig(" ("); } type_red2 ')'
   {
   postSig(");");
   sprintf(temps,"Set%s",$<str>3); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = $<integer>6;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();
   }
| GetMacro '('{postSig("Get");} any_id ',' {postSig(" ();"); invertSig = 1;} 
    type_red2 ')'
   { 
   sprintf(temps,"Get%s",$<str>4); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = $<integer>7;
   output_function();
   }
| SetStringMacro '(' {preSig("void Set");} any_id ')'
   {
   postSig(" (char *);"); 
   sprintf(temps,"Set%s",$<str>4); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x303;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();
   }
| GetStringMacro '(' {preSig("char *Get");} any_id ')'
   { 
   postSig(" ();");
   sprintf(temps,"Get%s",$<str>4); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x303;
   output_function();
   }
| SetClampMacro  '(' any_id ',' 
    {preSig("void Set"); postSig(" ("); } type_red2 
    {postSig(");"); openSig = 0;} ',' maybe_other_no_semi ')'
   { 
   char *local = vtkstrdup(currentFunction->Signature);
   sscanf (currentFunction->Signature, "%*s %*s (%s);", local);
   sprintf(temps,"Set%s",$<str>3); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = $<integer>6;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%s Get%sMinValue ();",local,$<str>3);
   sprintf(temps,"Get%sMinValue",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = $<integer>6;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%s Get%sMaxValue ();",local,$<str>3);
   sprintf(temps,"Get%sMaxValue",$<str>3);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = $<integer>6;
   output_function();
   }
| SetObjectMacro '(' any_id ',' 
  {preSig("void Set"); postSig(" ("); } type_red2 ')'
   { 
   postSig("*);");
   sprintf(temps,"Set%s",$<str>3); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 0x2;
   output_function();
   }
| SetReferenceCountedObjectMacro '(' any_id ','   
   {preSig("void Set"); postSig(" ("); } type_red2 ')'
   { 
   postSig("*);");
   sprintf(temps,"Set%s",$<str>3); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 0x2;
   output_function();
   }
| GetObjectMacro '(' {postSig("*Get");} any_id ',' 
   {postSig(" ();"); invertSig = 1;} type_red2 ')'
   { 
   sprintf(temps,"Get%s",$<str>4); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x309;
   output_function();
   }
| BooleanMacro '(' any_id 
    {preSig("void "); postSig("On ();"); openSig = 0; } 
        ',' type_red2 ')'
   { 
   sprintf(temps,"%sOn",$<str>3); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x2;
   output_function();
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void %sOff ();",$<str>3); 
   sprintf(temps,"%sOff",$<str>3); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   output_function();
   }
| SetVector2Macro '(' any_id ',' 
     {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     } 
      type_red2 ')'
   { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s);",$<str>3,
     local, local);
   sprintf(temps,"Set%s",$<str>3); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 2;
   currentFunction->ArgTypes[0] = $<integer>6;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = $<integer>6;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[2]);",$<str>3,
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x300 + $<integer>6;
   currentFunction->ArgCounts[0] = 0x2;
   output_function();
   }
| GetVector2Macro  '(' any_id ',' 
     {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     } 
      type_red2 ')'
   { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, $<str>3);
   sprintf(temps,"Get%s",$<str>3); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + $<integer>6;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 2;
   output_function();
   }
| SetVector3Macro '(' any_id ',' 
     {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     } 
      type_red2 ')'
   { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s);",
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
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[3]);",$<str>3,
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x300 + $<integer>6;
   currentFunction->ArgCounts[0] = 3;
   output_function();
   }
| GetVector3Macro  '(' any_id ','
     {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     } 
      type_red2 ')'
   { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, $<str>3);
   sprintf(temps,"Get%s",$<str>3); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + $<integer>6;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 3;
   output_function();
   }
| SetVector4Macro '(' any_id ',' 
     {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     } 
      type_red2 ')'
   { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s, %s);",
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
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[4]);",$<str>3,
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x300 + $<integer>6;
   currentFunction->ArgCounts[0] = 4;
   output_function();
   }
| GetVector4Macro  '(' any_id ','
     {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     } 
      type_red2 ')'
   { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, $<str>3);
   sprintf(temps,"Get%s",$<str>3); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + $<integer>6;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 4;
   output_function();
   }
| SetVector6Macro '(' any_id ','
     {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     } 
      type_red2 ')'
   { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s, %s, %s, %s);",
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
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[6]);",$<str>3,
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x300 + $<integer>6;
   currentFunction->ArgCounts[0] = 6;
   output_function();
   }
| GetVector6Macro  '(' any_id ','
     {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     } 
      type_red2 ')'
   { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, $<str>3);
   sprintf(temps,"Get%s",$<str>3); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + $<integer>6;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 6;
   output_function();
   }
| SetVectorMacro  '(' any_id ',' 
      {
      free (currentFunction->Signature);
      currentFunction->Signature = NULL;
      } 
     type_red2 ',' float_num ')'
   {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s [%i]);",$<str>3,
      local, $<integer>8);
     sprintf(temps,"Set%s",$<str>3); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->ReturnType = 0x2;
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 0x300 + $<integer>6;
     currentFunction->ArgCounts[0] = $<integer>8;
     output_function();
   }
| GetVectorMacro  '(' any_id ',' 
     {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     } 
     type_red2 ',' float_num ')'
   { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, $<str>3);
   sprintf(temps,"Get%s",$<str>3); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + $<integer>6;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = $<integer>8;
   output_function();
   }
| ViewportCoordinateMacro '(' any_id ')'
   { 
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       $<str>3);

     sprintf(temps,"Get%sCoordinate",$<str>3); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 0x309;
     currentFunction->ReturnClass = vtkstrdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double, double);",
       $<str>3);
     sprintf(temps,"Set%s",$<str>3); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 2;
     currentFunction->ArgTypes[0] = 0x7;
     currentFunction->ArgCounts[0] = 0;
     currentFunction->ArgTypes[1] = 0x7;
     currentFunction->ArgCounts[1] = 0;
     currentFunction->ReturnType = 0x2;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double a[2]);",
       $<str>3);
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 0x307;
     currentFunction->ArgCounts[0] = 2;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s ();", $<str>3);
     sprintf(temps,"Get%s",$<str>3); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 0x307;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 2;
     output_function();
   }
| WorldCoordinateMacro '(' any_id ')'
   { 
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       $<str>3);

     sprintf(temps,"Get%sCoordinate",$<str>3); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 0x309;
     currentFunction->ReturnClass = vtkstrdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double, double, double);",
       $<str>3);
     sprintf(temps,"Set%s",$<str>3); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 3;
     currentFunction->ArgTypes[0] = 0x7;
     currentFunction->ArgCounts[0] = 0;
     currentFunction->ArgTypes[1] = 0x7;
     currentFunction->ArgCounts[1] = 0;
     currentFunction->ArgTypes[2] = 0x7;
     currentFunction->ArgCounts[2] = 0;
     currentFunction->ReturnType = 0x2;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double a[3]);",
       $<str>3);
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 0x307;
     currentFunction->ArgCounts[0] = 3;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s ();", $<str>3);
     sprintf(temps,"Get%s",$<str>3); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 0x307;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 3;
     output_function();
   }
| TypeMacro '(' any_id ',' any_id ')'
   { 
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "const char *GetClassName ();");
   sprintf(temps,"GetClassName"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x1303;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,
           "int IsA (const char *name);");
   sprintf(temps,"IsA"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x1303;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 0x4;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "%s *NewInstance ();",
           $<str>3);
   sprintf(temps,"NewInstance"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x309;
   currentFunction->ReturnClass = vtkstrdup($<str>3);
   output_function();

   if ( data.IsConcrete )
     {
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature, "%s *SafeDownCast (vtkObject* o);",
             $<str>3);
     sprintf(temps,"SafeDownCast"); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 0x309;
     currentFunction->ArgCounts[0] = 1;
     currentFunction->ArgClasses[0] = vtkstrdup("vtkObject");
     currentFunction->ReturnType = 0x2309;
     currentFunction->ReturnClass = vtkstrdup($<str>3);
     output_function();
     }
   }
| TypeMacro '(' any_id ',' any_id ',' ')'
   { 
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "const char *GetClassName ();");
   sprintf(temps,"GetClassName"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x1303;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,
           "int IsA (const char *name);");
   sprintf(temps,"IsA"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x1303;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 0x4;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "%s *NewInstance ();",
           $<str>3);
   sprintf(temps,"NewInstance"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x309;
   currentFunction->ReturnClass = vtkstrdup($<str>3);
   output_function();

   if ( data.IsConcrete )
     {
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature, "%s *SafeDownCast (vtkObject* o);",
             $<str>3);
     sprintf(temps,"SafeDownCast"); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 0x309;
     currentFunction->ArgCounts[0] = 1;
     currentFunction->ArgClasses[0] = vtkstrdup("vtkObject");
     currentFunction->ReturnType = 0x2309;
     currentFunction->ReturnClass = vtkstrdup($<str>3);
     output_function();
     }
   }
;

/*
 * These just eat up misc garbage
 */
maybe_other : | other_stuff maybe_other;
maybe_other_no_semi : | other_stuff_no_semi maybe_other_no_semi;

other_stuff : ';' | other_stuff_no_semi;

other_stuff_no_semi : OTHER | braces | parens | '*' | '=' | ':' | ',' | '.'
   | STRING | type_red2 | type_string2 | NUM | CLASS_REF | '&' | brackets
   | CONST | OPERATOR | '-' | '~' | STATIC | ARRAY_NUM;

braces: '{' maybe_other '}';
parens: '(' maybe_other ')';
brackets: '[' maybe_other ']';

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
  func->ReturnType = 0x2;
  func->ReturnClass = NULL;
  func->Comment = NULL;
  func->Signature = NULL;
  func->IsLegacy = 0;
  sigAllocatedLength = 0;
  openSig = 1;
  invertSig = 0;
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

/* a simple routine that updates a few variables */
void output_function()
{
  int i;

  /* a void argument is the same as no arguements */
  if (currentFunction->ArgTypes[0] % 0x1000 == 0x2) 
    {
    currentFunction->NumberOfArguments = 0;
    }

  currentFunction->IsPublic = in_public;
  currentFunction->IsProtected = in_protected;
  
  /* look for VAR FUNCTIONS */
  if (currentFunction->NumberOfArguments
      && (currentFunction->ArgTypes[0] == 0x5000))
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
    data.HasDelete = 1;
    }


  /* if we need a return type hint and dont currently have one */
  /* then try to find one */
  if (!currentFunction->HaveHint)
    {
    switch (currentFunction->ReturnType % 0x1000)
      {
      case 0x301: case 0x302: case 0x307: case 0x30A: case 0x30B: case 0x30C:
      case 0x304: case 0x305: case 0x306: case 0x313:
        look_for_hint();
        break;
      }
    }

  /* reject multi-dimensional arrays from wrappers */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    if ((currentFunction->ArgTypes[i] % 0x1000)/0x100 == 0x6 ||
        (currentFunction->ArgTypes[i] % 0x1000)/0x100 == 0x9)
      {
      currentFunction->ArrayFailure = 1;
      }
    }

  if (HaveComment)
    {
    currentFunction->Comment = vtkstrdup(CommentText);
    }
  
  data.NumberOfFunctions++;
  currentFunction = data.Functions + data.NumberOfFunctions;
  InitFunction(currentFunction);
}

extern void vtkParseOutput(FILE *,FileInfo *);

int main(int argc,char *argv[])
{
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
  
  currentFunction = data.Functions;
  InitFunction(currentFunction);
  
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


