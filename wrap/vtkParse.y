/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParse.y
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/

%{
#include <stdio.h>
#include <stdlib.h>
#define yyerror(a) fprintf(stderr,"%s\n",a)
#define yywrap() 1

#include "vtkParse.h"
    
  FileInfo data;
  static FunctionInfo *currentFunction;

  FILE *fhint;
  char temps[2048];
  int  in_public;
  
#define YYMAXDEPTH 1000
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

function: '~' func { output_function(); } 
      | VIRTUAL '~' func { output_function(); }
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
         currentFunction->ReturnType = $<integer>2;
         output_function();
	 }
      | VIRTUAL func
         {
         output_function();
	 };

func: any_id '(' args_list ')' func_end
    {
      currentFunction->Name = $<str>1; 
      fprintf(stderr,"   Parsed func %s\n",$<str>1); 
    }  
  | OPERATOR maybe_other_no_semi ';'
    { 
      currentFunction->IsOperator = 1; 
      fprintf(stderr,"   Converted operator\n"); 
    }
  | any_id '(' args_list ')' '=' NUM ';'
    { 
      currentFunction->Name = $<str>1; 
      fprintf(stderr,"   Parsed func %s\n",$<str>1); 
      currentFunction->IsPureVirtual = 1; 
      data.IsAbstract = 1;
    };

any_id: VTK_ID | ID;

func_end: ';' 
    | '{' maybe_other '}' ';' 
    | '{' maybe_other '}'  
    | ':' maybe_other_no_semi ';';

args_list: | more_args;

more_args: arg { currentFunction->NumberOfArguments++;} 
  | arg { currentFunction->NumberOfArguments++;} ',' more_args;

arg: type 
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
	$<integer>1;} 
  | type var_id 
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
	$<integer>1;
    } opt_var_assign
  | VAR_FUNCTION 
    { 
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 5000;
    };

opt_var_assign: | '=' float_num;

var: type var_id ';' | VAR_FUNCTION ';';

var_id: any_id var_array;

var_array: 
  | ARRAY_NUM var_array { currentFunction->ArrayFailure = 1; }
  | '[' maybe_other_no_semi ']' var_array 
    { currentFunction->ArrayFailure = 1; };


type: CONST type_red1 {$<integer>$ = 1000 + $<integer>2;} 
          | type_red1 {$<integer>$ = $<integer>1;}; 
          | STATIC type_red1 {$<integer>$ = 2000 + $<integer>2;}; 
          | STATIC CONST type_red1 {$<integer>$ = 3000 + $<integer>3;}; 

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
type_indirection: '&' { $<integer>$ = 100;} 
                | '*' { $<integer>$ = 300;} 
                | '&' type_indirection { $<integer>$ = 100 + $<integer>2;}
                | '*' type_indirection { $<integer>$ = 400 + $<integer>2;};

type_red2: UNSIGNED type_primitive { $<integer>$ = 10 + $<integer>2;} 
                  | type_primitive { $<integer>$ = $<integer>1;};

type_primitive: 
  FLOAT  { $<integer>$ = 1;} | 
  VOID   { $<integer>$ = 2;} | 
  CHAR   { $<integer>$ = 3;} | 
  INT    { $<integer>$ = 4;} | 
  SHORT  { $<integer>$ = 5;} | 
  LONG   { $<integer>$ = 6;} | 
  DOUBLE { $<integer>$ = 7;} | 
  ID     { $<integer>$ = 8;} |
  VTK_ID  
    { 
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

scope_type: PUBLIC {in_public = 1;} | PRIVATE {in_public = 0;} 
          | PROTECTED {in_public = 0;};

float_num: '-' float_prim | float_prim;

float_prim: NUM {$<integer>$ = $1;} 
         | NUM '.' NUM {$<integer>$ = -1;} | any_id {$<integer>$ = -1;};

macro:
  SetMacro '(' any_id ',' type_red2 ')'
   {
   sprintf(temps,"Set%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = $<integer>5;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
   output_function();
   }
| GetMacro '(' any_id ',' type_red2 ')'
   { 
   sprintf(temps,"Get%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = $<integer>5;
   output_function();
   }
| SetStringMacro '(' any_id ')'
   { 
   sprintf(temps,"Set%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 303;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
   output_function();
   }
| GetStringMacro '(' any_id ')'
   { 
   sprintf(temps,"Get%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 303;
   output_function();
   }
| SetClampMacro  '(' any_id ',' type_red2 ',' maybe_other_no_semi ')'
   { 
   sprintf(temps,"Set%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = $<integer>5;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
   output_function();
   }
| SetObjectMacro '(' any_id ',' type_red2 ')'
   { 
   sprintf(temps,"Set%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 2;
   output_function();
   }
| SetReferenceCountedObjectMacro '(' any_id ',' type_red2 ')'
   { 
   sprintf(temps,"Set%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 2;
   output_function();
   }
| GetObjectMacro '(' any_id ',' type_red2 ')'
   { 
   sprintf(temps,"Get%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 309;
   output_function();
   }
| BooleanMacro   '(' any_id ',' type_red2 ')'
   { 
   sprintf(temps,"%sOn",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 2;
   output_function();
   sprintf(temps,"%sOff",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   output_function();
   }
| SetVector2Macro '(' any_id ',' type_red2 ')'
   { 
   sprintf(temps,"Set%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 2;
   currentFunction->ArgTypes[0] = $<integer>5;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = $<integer>5;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + $<integer>5;
   currentFunction->ArgCounts[0] = 2;
   output_function();
   }
| GetVector2Macro  '(' any_id ',' type_red2 ')'
   { 
   sprintf(temps,"Get%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + $<integer>5;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 2;
   output_function();
   }
| SetVector3Macro '(' any_id ',' type_red2 ')'
   { 
   sprintf(temps,"Set%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 3;
   currentFunction->ArgTypes[0] = $<integer>5;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = $<integer>5;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = $<integer>5;
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + $<integer>5;
   currentFunction->ArgCounts[0] = 3;
   output_function();
   }
| GetVector3Macro  '(' any_id ',' type_red2 ')'
   { 
   sprintf(temps,"Get%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + $<integer>5;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 3;
   output_function();
   }
| SetVector4Macro '(' any_id ',' type_red2 ')'
   { 
   sprintf(temps,"Set%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 4;
   currentFunction->ArgTypes[0] = $<integer>5;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = $<integer>5;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = $<integer>5;
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ArgTypes[3] = $<integer>5;
   currentFunction->ArgCounts[3] = 0;
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + $<integer>5;
   currentFunction->ArgCounts[0] = 4;
   output_function();
   }
| GetVector4Macro  '(' any_id ',' type_red2 ')'
   { 
   sprintf(temps,"Get%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + $<integer>5;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 4;
   output_function();
   }
| SetVector6Macro '(' any_id ',' type_red2 ')'
   { 
   sprintf(temps,"Set%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 6;
   currentFunction->ArgTypes[0] = $<integer>5;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = $<integer>5;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = $<integer>5;
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ArgTypes[3] = $<integer>5;
   currentFunction->ArgCounts[3] = 0;
   currentFunction->ArgTypes[4] = $<integer>5;
   currentFunction->ArgCounts[4] = 0;
   currentFunction->ArgTypes[5] = $<integer>5;
   currentFunction->ArgCounts[5] = 0;
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + $<integer>5;
   currentFunction->ArgCounts[0] = 6;
   output_function();
   }
| GetVector6Macro  '(' any_id ',' type_red2 ')'
   { 
   sprintf(temps,"Get%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + $<integer>5;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 6;
   output_function();
   }
| SetVectorMacro  '(' any_id ',' type_red2 ',' float_num ')'
   {
     sprintf(temps,"Set%s",$<str>3); 
     currentFunction->Name = strdup(temps);
     currentFunction->ReturnType = 2;
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 300 + $<integer>5;
     currentFunction->ArgCounts[0] = $<integer>7;
     output_function();
   }
| GetVectorMacro  '(' any_id ',' type_red2 ',' float_num ')'
   { 
   sprintf(temps,"Get%s",$<str>3); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + $<integer>5;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = $<integer>7;
   output_function();
   }
| ViewportCoordinateMacro '(' any_id ')'
   { 
     sprintf(temps,"Get%sCoordinate",$<str>3); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 309;
     currentFunction->ReturnClass = strdup("vtkCoordinate");
     output_function();

     sprintf(temps,"Set%s",$<str>3); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 2;
     currentFunction->ArgTypes[0] = 1;
     currentFunction->ArgCounts[0] = 0;
     currentFunction->ArgTypes[1] = 1;
     currentFunction->ArgCounts[1] = 0;
     currentFunction->ReturnType = 2;
     output_function();

     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 301;
     currentFunction->ArgCounts[0] = 2;
     output_function();
     
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
     sprintf(temps,"Get%sCoordinate",$<str>3); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 309;
     currentFunction->ReturnClass = strdup("vtkCoordinate");
     output_function();

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

     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 301;
     currentFunction->ArgCounts[0] = 3;
     output_function();
     
     sprintf(temps,"Get%s",$<str>3); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 301;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 3;
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
   | STRING | type_red2 | NUM | CLASS_REF | '&' | brackets | CONST | OPERATOR
   | '-' | '~' | STATIC | ARRAY_NUM;

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
      case 304: case 305: case 306:
        look_for_hint();
	break;
      }
    }

  data.NumberOfFunctions++;
  currentFunction = data.Functions + data.NumberOfFunctions;
  InitFunction(currentFunction);
}

extern void vtkParseOutput(FILE *,FileInfo *);

int main(int argc,char *argv[])
{
  FILE *fin;

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

  data.IsConcrete = atoi(argv[3]);

  currentFunction = data.Functions;
  InitFunction(currentFunction);
  
  yyin = fin;
  yyout = stdout;
  yyparse();
  vtkParseOutput(stdout,&data);
  return 0;
}
 


