/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParse.y
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define yyerror(a) fprintf(stderr,"%s\n",a)
#define yywrap() 1

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
  int sigAllocatedLength;
  
#define YYMAXDEPTH 1000

  void checkSigSize(char *arg)
    {
    if (strlen(currentFunction->Signature) + strlen(arg) + 3 > 
        sigAllocatedLength)
      {
      currentFunction->Signature = (char *)
	realloc(currentFunction->Signature, sigAllocatedLength*2);
      sigAllocatedLength = sigAllocatedLength*2;
      }
    } 
  void preSig(char *arg)
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
      tmp = strdup(currentFunction->Signature);
      sprintf(currentFunction->Signature,"%s%s",arg,tmp);
      free(tmp);
      }
    }
  void postSig(char *arg)
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
      tmp = strdup(currentFunction->Signature);
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
%token <str> STRING
%token <integer> NUM
%token <str> ID
%token INT
%token FLOAT
%token SHORT
%token LONG
%token DOUBLE
%token VOID
%token CHAR
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

/* macro tokens */
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

%%
/*
 * Here is the start of the grammer
 */
strt: maybe_other class_def maybe_other;

class_def : CLASS VTK_ID 
      {
      data.ClassName = strdup($2);
      }
    optional_scope '{' class_def_body '}';

class_def_body: class_def_item | class_def_item class_def_body;

class_def_item: scope_type ':' | var
   | function | FRIEND function | macro ';' | macro;

function: '~' func { preSig("~"); output_function(); } 
      | VIRTUAL '~' func { preSig("virtual ~"); output_function(); }
      | func 
         {
         output_function();
	 }
      | type func 
         {
         currentFunction->ReturnType = $<integer>1;
         output_function();
	 } 
      | VIRTUAL type func 
         {
         preSig("virtual ");
         currentFunction->ReturnType = $<integer>2;
         output_function();
	 }
      | VIRTUAL func
         {
         preSig("virtual ");
         output_function();
	 };

func: func_beg { postSig(")"); } maybe_const { postSig(";"); openSig = 0; } 
      func_end
    {
      openSig = 1;
      currentFunction->Name = $<str>1; 
      fprintf(stderr,"   Parsed func %s\n",$<str>1); 
    }  
  | OPERATOR maybe_other_no_semi ';'
    { 
      currentFunction->IsOperator = 1; 
      fprintf(stderr,"   Converted operator\n"); 
    }
  | func_beg '=' NUM ';'
    { 
      postSig(") = 0;"); 
      currentFunction->Name = $<str>1; 
      fprintf(stderr,"   Parsed func %s\n",$<str>1); 
      currentFunction->IsPureVirtual = 1; 
      data.IsAbstract = 1;
    };

maybe_const: | CONST {postSig(" const");};

func_beg: any_id '('  {postSig(" ("); } args_list ')';

const_mod: CONST {postSig("const ");}

static_mod: STATIC {postSig("static ");}

any_id: VTK_ID {postSig($<str>1);} | ID {postSig($<str>1);};

func_end: ';' 
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
	$<integer>2 / 10000; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
	$<integer>1 + $<integer>2 % 10000;
      /* fail if array is not const */
      if ((($<integer>2 % 10000)/100) % 10 != 0 
	  && ($<integer>1 / 1000) != 1 ) {
	currentFunction->ArrayFailure = 1;
      }
    } opt_var_assign
  | VAR_FUNCTION 
    { 
      postSig("void (*func)(void *) ");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 5000;
    };

opt_var_assign: | '=' float_num;

var:  type var_id ';' {delSig();} | VAR_FUNCTION ';' {delSig();};

var_id: any_id var_array { $<integer>$ = $<integer>2; };

var_array: { $<integer>$ = 0; }
     | ARRAY_NUM { char temp[100]; sprintf(temp,"[%i]",$<integer>1); 
                   postSig(temp); } 
       var_array { $<integer>$ = 300 + 10000 * $<integer>1; } 
     | '[' maybe_other_no_semi ']' var_array 
           { postSig("[]"); $<integer>$ = 300; };


type: const_mod type_red1 {$<integer>$ = 1000 + $<integer>2;} 
          | type_red1 {$<integer>$ = $<integer>1;}; 
          | static_mod type_red1 {$<integer>$ = 2000 + $<integer>2;}; 
          | static_mod const_mod type_red1 {$<integer>$ = 3000 + $<integer>3;}; 

type_red1: type_red2 {$<integer>$ = $<integer>1;} 
         | type_red2 type_indirection 
             {$<integer>$ = $<integer>1 + $<integer>2;};

/* 100 = &
   200 = &&
   300 = *
   400 = &*
   500 = *&
   700 = **
   */
type_indirection: '&' { postSig("&"); $<integer>$ = 100;} 
                | '*' { postSig("*"); $<integer>$ = 300;} 
                | '&' type_indirection { $<integer>$ = 100 + $<integer>2;}
                | '*' type_indirection { $<integer>$ = 400 + $<integer>2;};

type_red2: UNSIGNED {postSig("unsigned ");} 
             type_primitive { $<integer>$ = 10 + $<integer>3;} 
                  | type_primitive { $<integer>$ = $<integer>1;};

type_primitive: 
  FLOAT  { postSig("float "); $<integer>$ = 1;} | 
  VOID   { postSig("void "); $<integer>$ = 2;} | 
  CHAR   { postSig("char "); $<integer>$ = 3;} | 
  INT    { postSig("int "); $<integer>$ = 4;} | 
  SHORT  { postSig("short "); $<integer>$ = 5;} | 
  LONG   { postSig("long "); $<integer>$ = 6;} | 
  DOUBLE { postSig("double "); $<integer>$ = 7;} | 
  ID     {       
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",$<str>1);
      postSig(ctmpid);
      $<integer>$ = 8;} |
  VTK_ID  
    { 
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",$<str>1);
      postSig(ctmpid);
      $<integer>$ = 9; 
      currentFunction->ArgClasses[currentFunction->NumberOfArguments] =
        strdup($1); 
      /* store the string into the return value just in case we need it */
      /* this is a parsing hack because the first "type" parser will */
      /* possibly be ht ereturn type of the first argument */
      if ((!currentFunction->ReturnClass) && 
          (!currentFunction->NumberOfArguments)) 
        { 
        currentFunction->ReturnClass = strdup($1); 
        }
    };

optional_scope: | ':' scope_list;

scope_list: scope_type VTK_ID 
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = strdup($2); 
      data.NumberOfSuperClasses++; 
    } 
  | scope_type VTK_ID 
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = strdup($2); 
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
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = $<integer>6;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
   output_function();
   }
| GetMacro '('{postSig("Get");} any_id ',' {postSig(" ();"); invertSig = 1;} 
    type_red2 ')'
   { 
   sprintf(temps,"Get%s",$<str>4); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = $<integer>7;
   output_function();
   }
| SetStringMacro '(' {preSig("void Set");} any_id ')'
   {
   postSig(" (char *);"); 
   sprintf(temps,"Set%s",$<str>4); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 303;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
   output_function();
   }
| GetStringMacro '(' {preSig("char *Get");} any_id ')'
   { 
   postSig(" ();");
   sprintf(temps,"Get%s",$<str>4); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 303;
   output_function();
   }
| SetClampMacro  '(' any_id ',' 
    {preSig("void Set"); postSig(" ("); } type_red2 
    {postSig(");"); openSig = 0;} ',' maybe_other_no_semi ')'
   { 
   char *local = strdup(currentFunction->Signature);
   sscanf (currentFunction->Signature, "%*s %*s (%s);", local);
   sprintf(temps,"Set%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = $<integer>6;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%s Get%sMinValue ();",local,$<str>3);
   sprintf(temps,"Get%sMinValue",$<str>3);
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = $<integer>6;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%s Get%sMaxValue ();",local,$<str>3);
   sprintf(temps,"Get%sMaxValue",$<str>3);
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = $<integer>6;
   output_function();
   }
| SetObjectMacro '(' any_id ',' 
  {preSig("void Set"); postSig(" ("); } type_red2 ')'
   { 
   postSig("*);");
   sprintf(temps,"Set%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 2;
   output_function();
   }
| SetReferenceCountedObjectMacro '(' any_id ','   
   {preSig("void Set"); postSig(" ("); } type_red2 ')'
   { 
   postSig("*);");
   sprintf(temps,"Set%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 2;
   output_function();
   }
| GetObjectMacro '(' {postSig("*Get");} any_id ',' 
   {postSig(" ();"); invertSig = 1;} type_red2 ')'
   { 
   sprintf(temps,"Get%s",$<str>4); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 309;
   output_function();
   }
| BooleanMacro '(' any_id 
    {preSig("void "); postSig("On ();"); openSig = 0; } 
        ',' type_red2 ')'
   { 
   sprintf(temps,"%sOn",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 2;
   output_function();
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void %sOff ();",$<str>3); 
   sprintf(temps,"%sOff",$<str>3); 
   currentFunction->Name = strdup(temps);
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
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s);",$<str>3,
     local, local);
   sprintf(temps,"Set%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 2;
   currentFunction->ArgTypes[0] = $<integer>6;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = $<integer>6;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[2]);",$<str>3,
     local);
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + $<integer>6;
   currentFunction->ArgCounts[0] = 2;
   output_function();
   }
| GetVector2Macro  '(' any_id ',' 
     {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     } 
      type_red2 ')'
   { 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, $<str>3);
   sprintf(temps,"Get%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + $<integer>6;
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
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s);",
     $<str>3, local, local, local);
   sprintf(temps,"Set%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 3;
   currentFunction->ArgTypes[0] = $<integer>6;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = $<integer>6;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = $<integer>6;
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[3]);",$<str>3,
     local);
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + $<integer>6;
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
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, $<str>3);
   sprintf(temps,"Get%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + $<integer>6;
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
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s, %s);",
     $<str>3, local, local, local, local);
   sprintf(temps,"Set%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 4;
   currentFunction->ArgTypes[0] = $<integer>6;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = $<integer>6;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = $<integer>6;
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ArgTypes[3] = $<integer>6;
   currentFunction->ArgCounts[3] = 0;
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[4]);",$<str>3,
     local);
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + $<integer>6;
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
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, $<str>3);
   sprintf(temps,"Get%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + $<integer>6;
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
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s, %s, %s, %s);",
     $<str>3, local, local, local, local, local, local);
   sprintf(temps,"Set%s",$<str>3); 
   currentFunction->Name = strdup(temps);
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
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[6]);",$<str>3,
     local);
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + $<integer>6;
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
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, $<str>3);
   sprintf(temps,"Get%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + $<integer>6;
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
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s [%i]);",$<str>3,
      local, $<integer>8);
     sprintf(temps,"Set%s",$<str>3); 
     currentFunction->Name = strdup(temps);
     currentFunction->ReturnType = 2;
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 300 + $<integer>6;
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
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, $<str>3);
   sprintf(temps,"Get%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + $<integer>6;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = $<integer>8;
   output_function();
   }
| ViewportCoordinateMacro '(' any_id ')'
   { 
     char *local = strdup(currentFunction->Signature);
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       $<str>3);

     sprintf(temps,"Get%sCoordinate",$<str>3); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 309;
     currentFunction->ReturnClass = strdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (float, float);",
       $<str>3);
     sprintf(temps,"Set%s",$<str>3); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 2;
     currentFunction->ArgTypes[0] = 1;
     currentFunction->ArgCounts[0] = 0;
     currentFunction->ArgTypes[1] = 1;
     currentFunction->ArgCounts[1] = 0;
     currentFunction->ReturnType = 2;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (float a[2]);",
       $<str>3);
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 301;
     currentFunction->ArgCounts[0] = 2;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"float *Get%s ();", $<str>3);
     sprintf(temps,"Get%s",$<str>3); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 301;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 2;
     output_function();
   }
| WorldCoordinateMacro '(' any_id ')'
   { 
     char *local = strdup(currentFunction->Signature);
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       $<str>3);

     sprintf(temps,"Get%sCoordinate",$<str>3); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 309;
     currentFunction->ReturnClass = strdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (float, float, float);",
       $<str>3);
     sprintf(temps,"Set%s",$<str>3); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 3;
     currentFunction->ArgTypes[0] = 1;
     currentFunction->ArgCounts[0] = 0;
     currentFunction->ArgTypes[1] = 1;
     currentFunction->ArgCounts[1] = 0;
     currentFunction->ArgTypes[2] = 1;
     currentFunction->ArgCounts[2] = 0;
     currentFunction->ReturnType = 2;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (float a[3]);",
       $<str>3);
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 301;
     currentFunction->ArgCounts[0] = 3;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"float *Get%s ();", $<str>3);
     sprintf(temps,"Get%s",$<str>3); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 301;
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
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 1303;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,
           "int IsA (const char *name);");
   sprintf(temps,"IsA"); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 1303;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 4;
   output_function();
   }
;

/*
 * These just eat up misc garbage
 */
maybe_other : | other_stuff maybe_other;
maybe_other_no_semi : | other_stuff_no_semi maybe_other_no_semi;

other_stuff : ';' | other_stuff_no_semi;

other_stuff_no_semi : OTHER | braces | parens | '*' | '=' | ':' | ',' | '.'
   | STRING | type_red2 | NUM | CLASS_REF | '&' | brackets | CONST 
   | OPERATOR | '-' | '~' | STATIC | ARRAY_NUM;

braces: '{' maybe_other '}';
parens: '(' maybe_other ')';
brackets: '[' maybe_other ']';

%%
#include <string.h>
#include "lex.yy.c"

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
  func->ReturnType = 2;
  func->ReturnClass = NULL;
  func->Comment = NULL;
  func->Signature = NULL;
  sigAllocatedLength = 0;
  openSig = 1;
  invertSig = 0;
}

/* when the cpp file doesn't have enough info use the hint file */
void look_for_hint()
{
  char h_cls[80];
  char h_func[80];
  int  h_type;
  int  h_value;

  /* reset the position */
  rewind(fhint);

  /* first find a hint */
  while (fscanf(fhint,"%s %s %i %i",h_cls,h_func,&h_type,&h_value) != EOF)
    {
    if ((!strcmp(h_cls,data.ClassName))&&
	currentFunction->Name &&
	(!strcmp(h_func,currentFunction->Name))&&
	(h_type == currentFunction->ReturnType))
      {
      currentFunction->HaveHint = 1;
      currentFunction->HintSize = h_value;
      }
    }
}

/* a simple routine that updates a few variables */
void output_function()
{
  /* a void argument is the same as no arguements */
  if (currentFunction->ArgTypes[0]%1000 == 2) 
    {
    currentFunction->NumberOfArguments = 0;
    }

  currentFunction->IsPublic = in_public;
  currentFunction->IsProtected = in_protected;
  
  /* look for VAR FUNCTIONS */
  if (currentFunction->NumberOfArguments
      && (currentFunction->ArgTypes[0] == 5000))
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
    switch (currentFunction->ReturnType%1000)
      {
      case 301: case 302: case 307:
      case 304: case 305: case 306: case 313:
        look_for_hint();
	break;
      }
    }

  if (HaveComment)
    {
    currentFunction->Comment = strdup(CommentText);
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
  
  if (argc != 4)
    {
    fprintf(stderr,"Usage: %s input_file hint_file is_concrete\n",argv[0]);
    exit(1);
    }
  
  if (!(fin = fopen(argv[1],"r")))
    {
    fprintf(stderr,"Error opening input file %s\n",argv[1]);
    exit(1);
    }

  if (!(fhint = fopen(argv[2],"r")))
    {
    fprintf(stderr,"Error opening hint file %s\n",argv[2]);
    exit(1);
    }

  data.FileName = argv[1];
  data.NameComment = NULL;
  data.Description = NULL;
  data.Caveats = NULL;
  data.SeeAlso = NULL;
  CommentState = 0;
  data.IsConcrete = atoi(argv[3]);

  currentFunction = data.Functions;
  InitFunction(currentFunction);
  
  yyin = fin;
  yyout = stdout;
  ret = yyparse();
  if (ret)
    {
    fprintf(stdout,
            "*** SYNTAX ERROR found in parsing the header file %s ***\n", 
            argv[1]);
    return ret;
    }
  vtkParseOutput(stdout,&data);
  return 0;
}
 


