/*=========================================================================

  Program:   Java Wrapper for VTK
  Module:    java_parse.y
  Language:  Yacc
  Date:      $Date$
  Version:   $Revision$

This file's contents may be copied, reproduced or altered in any way 
without the express written consent of the author.

Copyright (c) Ken Martin 1995

=========================================================================*/
%{
#include <stdio.h>
#include <stdlib.h>
#define yyerror(a) fprintf(stderr,"%s\n",a)
#define yywrap() 1
int   is_abstract = 0;
int have_delete = 0;
FILE *fhint;
char *class_name;
char *file_name;
char *superclasses[5];
int have_hint = 0;
int num_superclasses = 0;
int in_public = 0;
char *func_name;
int is_virtual;
int num_args = 0;
/* the last entry is for the return type */
int arg_types[11];
char *arg_ids[11];
int arg_counts[11];
int arg_failure;
char temps[80];
char *funcNames[1000];
int   funcArgs[1000];
int   funcArgTypes[1000][11];
int  numFuncs = 0;
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
%token SetRefCountedObjectMacro
%token GetObjectMacro
%token BooleanMacro
%token SetVector2Macro
%token SetVector3Macro
%token SetVector4Macro
%token SetVectorMacro
%token GetVectorMacro
%token ImageSetMacro
%token ImageSetExtentMacro

%%
/*
 * Here is the start of the grammer
 */
strt: maybe_other class_def maybe_other;

class_def : CLASS VTK_ID 
      {
      class_name = strdup($2);
      fprintf(stderr,"Working on %s\n",class_name);
      fprintf(yyout,"// java wrapper for %s object\n//\n",class_name);
      fprintf(yyout,"\npackage vtk;\n");
      }
    optional_scope 
      {
      int i;

      if (strcmp("vtkObject",class_name))
	{
	fprintf(yyout,"import vtk.*;\n");
	}
      fprintf(yyout,"\npublic class %s",class_name);
      if (strcmp("vtkObject",class_name))
	{
	  if (num_superclasses) fprintf(yyout," extends %s",superclasses[0]);
	}
      fprintf(yyout,"\n{\n");
      }
    '{' class_def_body '}'
      {
	if (!num_superclasses)
	  {
	  fprintf(yyout,"\n  public %s() { this.VTKInit();};\n",class_name);
          fprintf(yyout,"  protected int vtkId = 0;\n");
	  
	  /* if we are a base class and have a delete method */
	  if (have_delete)
	    {
	    fprintf(yyout,"\n  public native void VTKDelete();\n");
	    fprintf(yyout,"  protected void finalize() { this.VTKDelete();};\n");
	    }
	  }
	if ((!is_abstract)&&
	    strcmp(class_name,"vtkDataWriter") &&
	    strcmp(class_name,"vtkPointSet") &&
	    strcmp(class_name,"vtkDataSetSource") 
	    )
	  {
	  if ((num_superclasses && 
	       strcmp(superclasses[0],"vtkGeometryPrimitive")) ||
	      !num_superclasses)
	    {
	    fprintf(yyout,"  public native void   VTKInit();\n");
	    }
	  }
	fprintf(yyout,"}\n");
      };

class_def_body: class_def_item | class_def_item class_def_body;

class_def_item: scope_type ':' | var 
   | function 
     { arg_failure = 0; num_args = 0; arg_types[10] = 2; arg_ids[10] = NULL;} 
   | FRIEND function
     { arg_failure = 0; num_args = 0; arg_types[10] = 2; arg_ids[10] = NULL;} 
   | macro ';' 
     { arg_failure = 0; num_args = 0; arg_types[10] = 2; arg_ids[10] = NULL;} 
   | macro  
     { arg_failure = 0; num_args = 0; arg_types[10] = 2; arg_ids[10] = NULL;};

function: '~' func | VIRTUAL '~' func 
      | func 
         {
         output_function();
	 }
      | type func 
         {
         arg_types[10] = $<integer>1;
         output_function();
	 } 
      | VIRTUAL type func 
         {
         arg_types[10] = $<integer>2;
         output_function();
	 }
      | VIRTUAL func
         {
         output_function();
	 };

func: any_id '(' args_list ')' func_end
     { is_virtual = 0; func_name = $<str>1; 
       fprintf(stderr,"   Converted func %s\n",$<str>1); }  
  | OPERATOR maybe_other_no_semi ';'
     { is_virtual = 1; fprintf(stderr,"   Converted operator\n"); }
  | any_id '(' args_list ')' '=' NUM ';' 
     { is_virtual = 0; func_name = $<str>1;
       fprintf(stderr,"   Converted func %s\n",$<str>1); is_abstract = 1;};

any_id: VTK_ID | ID;

func_end: ';' 
    | '{' maybe_other '}' ';' 
    | '{' maybe_other '}'  
    | ':' maybe_other_no_semi ';';

args_list: | more_args;

more_args: arg { num_args++;} | arg {num_args++;} ',' more_args;

arg: type {arg_counts[num_args] = 0; arg_types[num_args] = $<integer>1;} 
   | type var_id {arg_types[num_args] = $<integer>1; } opt_var_assign
   | VAR_FUNCTION {arg_types[num_args] = 5000;};

opt_var_assign: | '=' float_num;

var: type var_id ';' | VAR_FUNCTION ';';

var_id: any_id var_array;

var_array: 
  | ARRAY_NUM var_array { arg_failure = 1; }
  | '[' maybe_other_no_semi ']' var_array { arg_failure = 1; };


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
  VTK_ID  { $<integer>$ = 9; 
           arg_ids[num_args] = strdup($1); 
           if ((!arg_ids[10])&&(!num_args))
             { 
             arg_ids[10] = arg_ids[0];
             }
         };

optional_scope: | ':' scope_list;

scope_list: scope_type VTK_ID 
    { superclasses[num_superclasses] = strdup($2); num_superclasses++; } 
  | scope_type VTK_ID 
    { superclasses[num_superclasses] = strdup($2); num_superclasses++; } 
    ',' scope_list;

scope_type: PUBLIC {in_public = 1;} | PRIVATE {in_public = 0;} 
          | PROTECTED {in_public = 0;};

float_num: '-' float_prim | float_prim;

float_prim: NUM {$<integer>$ = $1;} 
         | NUM '.' NUM {$<integer>$ = -1;} | any_id {$<integer>$ = -1;};

macro:
  SetMacro '(' any_id ',' type_red2 ')'
   { 
   is_virtual = 0;
   sprintf(temps,"Set%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 1;
   arg_types[0] = $<integer>5;
   arg_counts[0] = 0;
   arg_types[10] = 2;
   output_function();
   }
| GetMacro '(' any_id ',' type_red2 ')'
   { 
   is_virtual = 0;
   sprintf(temps,"Get%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 0;
   arg_types[10] = $<integer>5;
   output_function();
   }
| SetStringMacro '(' any_id ')'
   { 
   is_virtual = 0;
   sprintf(temps,"Set%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 1;
   arg_types[0] = 303;
   arg_counts[0] = 0;
   arg_types[10] = 2;
   output_function();
   }
| GetStringMacro '(' any_id ')'
   { 
   is_virtual = 0;
   sprintf(temps,"Get%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 0;
   arg_types[10] = 303;
   output_function();
   }
| SetClampMacro  '(' any_id ',' type_red2 ',' maybe_other_no_semi ')'
   { 
   is_virtual = 0;
   sprintf(temps,"Set%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 1;
   arg_types[0] = $<integer>5;
   arg_counts[0] = 0;
   arg_types[10] = 2;
   output_function();
   }
| SetObjectMacro '(' any_id ',' type_red2 ')'
   { 
   is_virtual = 0;
   sprintf(temps,"Set%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 1;
   arg_types[0] = 309;
   arg_counts[0] = 1;
   arg_types[10] = 2;
   output_function();
   }
| SetRefCountedObjectMacro '(' any_id ',' type_red2 ')'
   { 
   is_virtual = 0;
   sprintf(temps,"Set%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 1;
   arg_types[0] = 309;
   arg_counts[0] = 1;
   arg_types[10] = 2;
   output_function();
   }
| GetObjectMacro '(' any_id ',' type_red2 ')'
   { 
   is_virtual = 0;
   sprintf(temps,"Get%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 0;
   arg_types[10] = 309;
   output_function();
   }
| BooleanMacro   '(' any_id ',' type_red2 ')'
   { 
   is_virtual = 0;
   sprintf(temps,"%sOn",$<str>3); 
   func_name = strdup(temps);
   num_args = 0;
   arg_types[10] = 2;
   output_function();
   free(func_name);
   sprintf(temps,"%sOff",$<str>3); 
   func_name = strdup(temps);
   num_args = 0;
   output_function();
   }
| SetVector2Macro '(' any_id ',' type_red2 ')'
   { 
   is_virtual = 0;
   sprintf(temps,"Set%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 2;
   arg_types[0] = $<integer>5;
   arg_counts[0] = 0;
   arg_types[1] = $<integer>5;
   arg_counts[1] = 0;
   arg_types[10] = 2;
   output_function();

   num_args = 1;
   arg_types[0] = 300 + $<integer>5;
   arg_counts[0] = 2;
   output_function();
   }
| SetVector3Macro '(' any_id ',' type_red2 ')'
   { 
   is_virtual = 0;
   sprintf(temps,"Set%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 3;
   arg_types[0] = $<integer>5;
   arg_counts[0] = 0;
   arg_types[1] = $<integer>5;
   arg_counts[1] = 0;
   arg_types[2] = $<integer>5;
   arg_counts[2] = 0;
   arg_types[10] = 2;
   output_function();

   num_args = 1;
   arg_types[0] = 300 + $<integer>5;
   arg_counts[0] = 3;
   output_function();
   }
| SetVector4Macro '(' any_id ',' type_red2 ')'
   { 
   is_virtual = 0;
   sprintf(temps,"Set%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 4;
   arg_types[0] = $<integer>5;
   arg_counts[0] = 0;
   arg_types[1] = $<integer>5;
   arg_counts[1] = 0;
   arg_types[2] = $<integer>5;
   arg_counts[2] = 0;
   arg_types[3] = $<integer>5;
   arg_counts[3] = 0;
   arg_types[10] = 2;
   output_function();

   num_args = 1;
   arg_types[0] = 300 + $<integer>5;
   arg_counts[0] = 4;
   output_function();
   }
| SetVectorMacro  '(' any_id ',' type_red2 ',' float_num ')'
   { 
   int i;

   is_virtual = 0;
   sprintf(temps,"Set%s",$<str>3); 
   func_name = strdup(temps);
   
   num_args = $<integer>7;
   for (i = 0; i < $<integer>7; i++)
     {
     arg_types[i] = $<integer>5;
     arg_counts[i] = 0;
     }
   arg_types[10] = 2;
   
   output_function();

   num_args = 1;
   arg_types[0] = 300 + $<integer>5;
   arg_counts[0] = $<integer>7;
   output_function();
   }
| GetVectorMacro  '(' any_id ',' type_red2 ',' float_num ')'
   { 
   is_virtual = 0;
   sprintf(temps,"Get%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 0;
   arg_types[10] = 300 + $<integer>5;
   have_hint = 1;
   output_function();
   }
| ImageSetMacro '(' any_id ',' type_red2 ')'
   { 
   is_virtual = 0;
   sprintf(temps,"Set%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 5;
   arg_types[0] = $<integer>5;
   arg_types[1] = $<integer>5;
   arg_types[2] = $<integer>5;
   arg_types[3] = $<integer>5;
   arg_types[4] = $<integer>5;
   output_function();
   free(func_name);
   sprintf(temps,"Set%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 4;
   arg_types[0] = $<integer>5;
   arg_types[1] = $<integer>5;
   arg_types[2] = $<integer>5;
   arg_types[3] = $<integer>5;
   output_function();
   free(func_name);
   sprintf(temps,"Set%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 3;
   arg_types[0] = $<integer>5;
   arg_types[1] = $<integer>5;
   arg_types[2] = $<integer>5;
   output_function();
   free(func_name);
   sprintf(temps,"Set%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 2;
   arg_types[0] = $<integer>5;
   arg_types[1] = $<integer>5;
   output_function();
   free(func_name);
   sprintf(temps,"Set%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 1;
   arg_types[0] = $<integer>5;
   output_function();
   free(func_name);
   }
| ImageSetExtentMacro '(' any_id ')'
   { 
   is_virtual = 0;
   sprintf(temps,"Set%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 10;
   arg_types[0] = 4;
   arg_types[1] = 4;
   arg_types[2] = 4;
   arg_types[3] = 4;
   arg_types[4] = 4;
   arg_types[5] = 4;
   arg_types[6] = 4;
   arg_types[7] = 4;
   arg_types[8] = 4;
   arg_types[9] = 4;
   output_function();
   free(func_name);
   sprintf(temps,"Set%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 8;
   arg_types[0] = 4;
   arg_types[1] = 4;
   arg_types[2] = 4;
   arg_types[3] = 4;
   arg_types[4] = 4;
   arg_types[5] = 4;
   arg_types[6] = 4;
   arg_types[7] = 4;
   output_function();
   free(func_name);
   sprintf(temps,"Set%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 6;
   arg_types[0] = 4;
   arg_types[1] = 4;
   arg_types[2] = 4;
   arg_types[3] = 4;
   arg_types[4] = 4;
   arg_types[5] = 4;
   output_function();
   free(func_name);
   sprintf(temps,"Set%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 4;
   arg_types[0] = 4;
   arg_types[1] = 4;
   arg_types[2] = 4;
   arg_types[3] = 4;
   output_function();
   free(func_name);
   sprintf(temps,"Set%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 2;
   arg_types[0] = 4;
   arg_types[1] = 4;
   output_function();
   free(func_name);
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

output_temp(int i)
{
  /* ignore void */
  if (((arg_types[i] % 10) == 2)&&(!((arg_types[i]%1000)/100)))
    {
    return;
    }
  
  if (arg_types[i] == 303)
    {
    fprintf(yyout,"String ");
    }
  else
    {
    switch (arg_types[i]%10)
      {
      case 1:   fprintf(yyout,"double "); break;
      case 7:   fprintf(yyout,"double "); break;
      case 4:   fprintf(yyout,"int "); break;
      case 5:   fprintf(yyout,"int "); break;
      case 6:   fprintf(yyout,"int "); break;
      case 2:     fprintf(yyout,"void "); break;
      case 3:     fprintf(yyout,"char "); break;
      case 9:     fprintf(yyout,"%s ",arg_ids[i]); break;
      case 8: return;
      }
    }

  fprintf(yyout,"id%i",i);
  if (((arg_types[i]%1000)/100 == 3)&&
      (arg_types[i] != 303)&&
      (arg_types[i] != 309))
    {
    fprintf(yyout,"[]");
    }
}

/* when the cpp file doesn't have enough info use the hint file */
use_hints()
{
  char h_cls[80];
  char h_func[80];
  int  h_type;
  int  h_value;
  int  i;

  if (have_hint)
    {
    /* use the hint */
    switch (arg_types[10])
      {
      case 301: case 307:  
	fprintf(yyout,"double[] "); break;
      case 304: case 305: case 306: case 313: case 314: case 315: case 316:
	fprintf(yyout,"int[]  "); break;
      }
    return;
    }
  
  /* reset the position */
  rewind(fhint);

  /* first find a hint */
  while (fscanf(fhint,"%s %s %i %i",h_cls,h_func,&h_type,&h_value) != EOF)
    {
    if ((!strcmp(h_cls,class_name))&&(!strcmp(h_func,func_name))&&
	(h_type == arg_types[10]))
      {
      /* use the hint */
      switch (h_type%1000)
	{
	case 301: case 307:  
	  fprintf(yyout,"double[] "); break;
	case 304: case 305: case 306: case 313: case 314: case 315: case 316:
	  fprintf(yyout,"int[]  "); break;
	}
      }
    }
}

/* when the cpp file doesn't have enough info use the hint file */
int hint_in_file()
{
  char h_cls[80];
  char h_func[80];
  int  h_type;
  int  h_value;
  int  i;

  /* reset the position */
  rewind(fhint);

  /* first find a hint */
  while (fscanf(fhint,"%s %s %i %i",h_cls,h_func,&h_type,&h_value) != EOF)
    {
    if ((!strcmp(h_cls,class_name))&&(!strcmp(h_func,func_name))&&
	(h_type == arg_types[10]))
      {
      return 1;
      }
    }
  return 0;
}

return_result()
{
  /* fprintf(stderr,"ret res %i\n",arg_types[10]);*/
  
  switch (arg_types[10]%1000)
    {
    case 1: fprintf(yyout,"double "); break;
    case 2: fprintf(yyout,"void "); break;
    case 3: fprintf(yyout,"char "); break;
    case 7: fprintf(yyout,"double "); break;
    case 4: case 5: case 6: case 13: case 14: case 15: case 16:
      fprintf(yyout,"int "); 
      break;
    case 303: fprintf(yyout,"String "); break;
    case 309:  
      fprintf(yyout,"%s ",arg_ids[10]);
      break;

    /* handle functions returning vectors */
    /* this is done by looking them up in a hint file */
    case 301: case 307:
    case 304: case 305: case 306:
      use_hints();
      break;
    }
}

/* have we done one of these yet */
int done_one()
{
  int i,j;
  int match;

  for (i = 0; i < numFuncs; i++)
    {
    if ((!strcmp(func_name,funcNames[i]))&&(num_args == funcArgs[i]))
      {
      match = 1;
      for (j = 0; j < num_args; j++)
	{
	if (arg_types[j] != funcArgTypes[i][j])
	  {
	  match = 0;
          }
	}
      if (arg_types[10] != funcArgTypes[i][10])
	{
	match = 0;
	}
      if (match) return 1;
      }
    }

  return 0;
}

output_function()
{
  int i;
  int args_ok = 1;
 
  if (is_virtual) return;
  if (arg_failure) return;

  /* check to see if we can handle the args */
  if (arg_types[0]%1000 == 2) 
    {
    num_args = 0;
    }
  for (i = 0; i < num_args; i++)
    {
    if (arg_types[i] == 9) args_ok = 0;
    if ((arg_types[i]%10) == 8) args_ok = 0;
    if (((arg_types[i]%1000)/100 != 3)&&
	(arg_types[i]%1000 != 109)&&
	((arg_types[i]%1000)/100)) args_ok = 0;
    if (arg_types[i] == 313) args_ok = 0;
    if (arg_types[i] == 315) args_ok = 0;
    }
  if ((arg_types[10]%10) == 8) args_ok = 0;
  if (arg_types[10] == 9) args_ok = 0;
  if (((arg_types[10]%1000)/100 != 3)&&
      (arg_types[10]%1000 != 109)&&
      ((arg_types[10]%1000)/100)) args_ok = 0;
  if ((arg_types[0] == 5000)&&(num_args != 2)) args_ok = 0;

  /* eliminate unsigned char * and unsigned short * */
  if (arg_types[10] == 313) args_ok = 0;
  if (arg_types[10] == 315) args_ok = 0;

  /* look for VAR FUNCTIONS */
  if ((arg_types[0] == 5000)&&(num_args == 2)) 
    {
    /*    args_ok = 1; */
    /* right now punt on var functions */
    args_ok = 0;
    num_args = 1;
    }

  /* make sure we have all the info we need for array arguments in */
  for (i = 0; i < num_args; i++)
    {
    if (((arg_types[i]%1000)/100 == 3)&&
	(arg_counts[i] <= 0)&&
	(arg_types[i] != 309)&&
	(arg_types[i] != 303)) args_ok = 0;
    }

  /* if we need a return type hint make sure we have one */
  if (!have_hint)
    {
    switch (arg_types[10]%1000)
      {
      case 301: case 302: case 307:
      case 304: case 305: case 306:
	if (!hint_in_file()) args_ok = 0;
	break;
      }
    }

  /* treat any 109 as if they were 309 */
  for (i = 0; i < num_args; i++)
    {
    if (arg_types[i] == 109)
      {
      arg_types[i] = 309;
      }
    }
  if (arg_types[10] == 109)
    {
    arg_types[10] = 309;
    }

  /* make sure it isn't a Delete function */
  if (!strcmp("Delete",func_name))
    {
    have_delete = 1;
    args_ok = 0;
    }

  if (in_public && args_ok)
    {
    /* make sure it's not a constructor */
    if (strcmp(class_name,func_name))
      {
      /* make sure we haven't already done one of these */
      if (!done_one())
	{
	fprintf(yyout,"\n  public native ");
	return_result();
	fprintf(yyout,"%s_%i(",func_name,numFuncs);
	
	for (i = 0; i < num_args; i++)
	  {
	  if (i)
	    {
	    fprintf(yyout,",");
	    }
	  output_temp(i);
	  }
	fprintf(yyout,");\n");
	fprintf(yyout,"  public ");
	return_result();
	fprintf(yyout,"%s(",func_name);
	
	for (i = 0; i < num_args; i++)
	  {
	  if (i)
	    {
	    fprintf(yyout,",");
	    }
	  output_temp(i);
	  }
	/* if not void then need return otherwise none */
	if (arg_types[10]%1000 == 2)
	  {
	  fprintf(yyout,")\n    { %s_%i(",func_name,numFuncs);
	  }
	else
	  {
	  fprintf(yyout,")\n    { return %s_%i(",func_name,numFuncs);
	  }
	for (i = 0; i < num_args; i++)
	  {
	  if (i)
	    {
	    fprintf(yyout,",");
	    }
	  fprintf(yyout,"id%i",i);
	  }
	fprintf(yyout,"); }\n");

	funcNames[numFuncs] = strdup(func_name);
	funcArgs[numFuncs] = num_args;
	for (i = 0; i < num_args; i++)
	  {
	  funcArgTypes[numFuncs][i] = arg_types[i];
	  }
	funcArgTypes[numFuncs][10] = arg_types[10];
	numFuncs++;
	}
      }
    }
  have_hint = 0;
}

main(int argc,char *argv[])
{
  FILE *fin;

  if (argc != 3)
    {
    fprintf(stderr,"Usage: %s input_file hint_file\n",argv[0]);
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

  file_name = argv[1];
  
  yyin = fin;
  yyout = stdout;
  yyparse();
  return 0;
}
 


