/*=========================================================================

  Program:   Java Wrapper for VTK
  Module:    java_wrap.y
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
int have_delete = 0;
int have_hint = 0;
int hint_size = 0;
int   is_abstract = 0;
FILE *fhint;
char *class_name;
char *file_name;
char *superclasses[5];
int num_superclasses = 0;
int in_public = 0;
char *func_name;
int is_virtual;
int num_args = 0;
/* the last entry is for the return type */
int arg_types[11];
int arg_counts[11];
char *arg_ids[11];
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
%token SetReferenceCountedObjectMacro
%token GetObjectMacro
%token BooleanMacro
%token SetVector2Macro
%token SetVector3Macro
%token SetVector4Macro
%token GetVector2Macro
%token GetVector3Macro
%token GetVector4Macro
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
      }
    optional_scope 
      {
      int i;
      fprintf(yyout,"#include \"%s.h\"\n",class_name);
      fprintf(yyout,"#include \"vtkJavaUtil.h\"\n\n",class_name);

      for (i = 0; i < num_superclasses; i++)
	{
	fprintf(yyout,"extern void *%s_Typecast(void *op,char *dType);\n",
		superclasses[i]);
	}
      
      fprintf(yyout,"\nvoid *%s_Typecast(void *me,char *dType)\n",class_name);
      fprintf(yyout,"{\n",class_name);
      fprintf(yyout,"  if (!strcmp(\"%s\",dType))\n    {\n", class_name);
      fprintf(yyout,"    return me;\n    }\n  else\n    {\n");

      /* check our superclasses */
      for (i = 0; i < num_superclasses; i++)
	{
	fprintf(yyout,"    if (%s_Typecast(((void *)((%s *)me)),dType) != NULL)\n",
		superclasses[i],superclasses[i]);
	fprintf(yyout,"      {\n");
	fprintf(yyout,"      return %s_Typecast(((void *)((%s *)me)),dType);\n      }\n",superclasses[i],superclasses[i]);
	
	}
      fprintf(yyout,"    }\n  return NULL;\n}\n\n");
      }
    '{' class_def_body '}'
      {
	if ((!num_superclasses)&&(have_delete))
	  {
	  fprintf(yyout,"\nextern \"C\" JNIEXPORT void JNICALL Java_vtk_%s_VTKDelete(JNIEnv *env,jobject obj)\n",
		  class_name);
	  fprintf(yyout,"{\n  %s *op;\n",class_name);
	  fprintf(yyout,"  op = (%s *)vtkJavaGetPointerFromObject(env,obj,\"%s\");\n",
		  class_name,class_name);
	  fprintf(yyout,"  if (vtkJavaShouldIDeleteObject(env,obj))\n");
	  fprintf(yyout,"    {\n    op->Delete();\n    }\n");
	  
	  fprintf(yyout,"}\n");
	  }
	if ((!is_abstract)&&
	    strcmp(class_name,"vtkDataWriter") &&
	    strcmp(class_name,"vtkPointSet") &&
	    strcmp(class_name,"vtkDataSetSource") &&
	    ((num_superclasses && 
	      strcmp(superclasses[0],"vtkGeometryPrimitive")) ||
	     !num_superclasses))
	  {
	  fprintf(yyout,"static int vtk_%s_NoCreate = 0;\n",class_name);
	  fprintf(yyout,"void vtk_%s_NoCPP()\n",class_name);
	  fprintf(yyout,"{\n  vtk_%s_NoCreate = 1;\n}\n\n",class_name);
	  fprintf(yyout,"\nextern \"C\" JNIEXPORT void JNICALL Java_vtk_%s_VTKInit(JNIEnv *env, jobject obj)\n",
		  class_name);
	  fprintf(yyout,"{\n  if (!vtk_%s_NoCreate)\n",class_name);
	  fprintf(yyout,"    {\n    %s *aNewOne = %s::New();\n",class_name,
		  class_name);
	  fprintf(yyout,"    vtkJavaAddObjectToHash(env,obj,(void *)aNewOne,(void *)%s_Typecast,1);\n",class_name);
	  fprintf(yyout,"    }\n  vtk_%s_NoCreate = 0;\n}\n",class_name);
	  }
	else
	  {
	  if (num_superclasses)
	    {
	    fprintf(yyout,"extern void vtk_%s_NoCPP();\n",superclasses[0]);
	    fprintf(yyout,"void vtk_%s_NoCPP()\n",class_name);
	    fprintf(yyout,"{\n  vtk_%s_NoCPP();\n}\n\n",superclasses[0]);
	    }
	  }
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
| SetReferenceCountedObjectMacro '(' any_id ',' type_red2 ')'
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
| GetVector2Macro  '(' any_id ',' type_red2 ')'
   { 
   is_virtual = 0;
   sprintf(temps,"Get%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 0;
   arg_types[10] = 300 + $<integer>5;
   have_hint = 1;
   hint_size = 2;
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
| GetVector3Macro  '(' any_id ',' type_red2 ')'
   { 
   is_virtual = 0;
   sprintf(temps,"Get%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 0;
   arg_types[10] = 300 + $<integer>5;
   have_hint = 1;
   hint_size = 3;
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
| GetVector4Macro  '(' any_id ',' type_red2 ')'
   { 
   is_virtual = 0;
   sprintf(temps,"Get%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 0;
   arg_types[10] = 300 + $<integer>5;
   have_hint = 1;
   hint_size = 4;
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
   
   if (!done_one())
     {
     fprintf(yyout,"extern \"C\" JNIEXPORT void JNICALL Java_vtk_%s_%s_1%i(JNIEnv *env,jobject obj",
	     class_name,func_name,numFuncs);
     
     for (i = 0; i < num_args; i++)
       {
       fprintf(yyout,",");
       output_proto_vars(i);
       }
     fprintf(yyout,")\n{\n");

     /* get the object pointer */
     fprintf(yyout,"  %s *op;\n",class_name);
     
     switch (arg_types[0]%10)
       {
       case 1:   fprintf(yyout,"  float  "); break;
       case 7:   fprintf(yyout,"  double "); break;
       case 4:   fprintf(yyout,"  int    "); break;
       case 5:   fprintf(yyout,"  short  "); break;
       case 6:   fprintf(yyout,"  long   "); break;
       case 2:   fprintf(yyout,"  void   "); break;
       case 3:   fprintf(yyout,"  char   "); break;
       }
     
     fprintf(yyout," temp[%i];\n",num_args);
     for (i = 0; i < num_args; i++)
       {
       fprintf(yyout,"  temp[%i] = id%i;\n",i,i);
       }

     fprintf(yyout,"\n  op = (%s *)vtkJavaGetPointerFromObject(env,obj,\"%s\");\n",
	     class_name,class_name);
     fprintf(yyout,"  op->%s(temp);\n",func_name);
     fprintf(yyout,"}\n");
     
     funcNames[numFuncs] = strdup(func_name);
     funcArgs[numFuncs] = num_args;
     for (i = 0; i < num_args; i++)
       {
       funcArgTypes[numFuncs][i] = arg_types[i];
       }
     funcArgTypes[numFuncs][10] = 2;
     numFuncs++;
     }

   num_args = 1;
   arg_types[0] = 300 + $<integer>5;
   arg_counts[0] = $<integer>7;
   
   if (!done_one())
     {
     fprintf(yyout,"extern \"C\" JNIEXPORT void JNICALL Java_vtk_%s_%s_1%i(JNIEnv *env,jobject obj",
	     class_name,func_name,numFuncs);
     fprintf(yyout,",");
     output_proto_vars(0);
     fprintf(yyout,")\n{\n");

     /* get the object pointer */
     fprintf(yyout,"  %s *op;\n",class_name);
     
     switch (arg_types[0]%10)
       {
       case 1:   fprintf(yyout,"  float  "); break;
       case 7:   fprintf(yyout,"  double "); break;
       case 4:   fprintf(yyout,"  int    "); break;
       case 5:   fprintf(yyout,"  short  "); break;
       case 6:   fprintf(yyout,"  long   "); break;
       case 2:   fprintf(yyout,"  void   "); break;
       case 3:   fprintf(yyout,"  char   "); break;
       }
     
     fprintf(yyout," temp[%i];\n",arg_counts[0]);
     fprintf(yyout,"  void *tempArray;\n");
     switch (arg_types[0]%1000)
       {
       case 301:
       case 307:
	 fprintf(yyout,"  tempArray = (void *)(env->GetDoubleArrayElements(id0,NULL));\n");
	 for (i = 0; i < arg_counts[0]; i++)
	   {
	   fprintf(yyout,"  temp[%i] = ((jdouble *)tempArray)[%i];\n",i,i);
	   }
	 fprintf(yyout,"  env->ReleaseDoubleArrayElements(id0,(jdouble *)tempArray,0);\n");      
	 break;
       case 304:
       case 306:
	 fprintf(yyout,"  tempArray = (void *)(env->GetLongArrayElements(id0,NULL));\n");
	 for (i = 0; i < arg_counts[0]; i++)
	   {
	   fprintf(yyout,"  temp[%i] = ((jlong *)tempArray)[%i];\n",i,i);
	   }
	 fprintf(yyout,"  env->ReleaseLongArrayElements(id0,(jlong *)tempArray,0);\n");      
	 break;
       }

     fprintf(yyout,"\n  op = (%s *)vtkJavaGetPointerFromObject(env,obj,\"%s\");\n",
	     class_name,class_name);
     fprintf(yyout,"  op->%s(temp);\n",func_name);
     fprintf(yyout,"}\n");
     
     funcNames[numFuncs] = strdup(func_name);
     funcArgs[numFuncs] = num_args;
     for (i = 0; i < num_args; i++)
       {
       funcArgTypes[numFuncs][i] = arg_types[i];
       }
     funcArgTypes[numFuncs][10] = 2;
     numFuncs++;
     }
   }
| GetVectorMacro  '(' any_id ',' type_red2 ',' float_num ')'
   { 
   is_virtual = 0;
   sprintf(temps,"Get%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 0;
   arg_types[10] = 300 + $<integer>5;
   have_hint = 1;
   hint_size = $<integer>7;
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

output_proto_vars(int i)
{
  /* ignore void */
  if (((arg_types[i] % 10) == 2)&&(!((arg_types[i]%1000)/100)))
    {
    return;
    }
  
  if (arg_types[i] == 303)
    {
    fprintf(yyout,"jstring ");
    fprintf(yyout,"id%i",i);
    return;
    }

  if ((arg_types[i] == 301)||(arg_types[i] == 307))
    {
    fprintf(yyout,"jdoubleArray ");
    fprintf(yyout,"id%i",i);
    return;
    }

  if ((arg_types[i] == 304)||(arg_types[i] == 306))
    {
    fprintf(yyout,"jlongArray ");
    fprintf(yyout,"id%i",i);
    return;
    }


  switch (arg_types[i]%10)
    {
    case 1:   fprintf(yyout,"jdouble "); break;
    case 7:   fprintf(yyout,"jdouble "); break;
    case 4:   fprintf(yyout,"jint "); break;
    case 5:   fprintf(yyout,"jint "); break;
    case 6:   fprintf(yyout,"jint "); break;
    case 2:     fprintf(yyout,"void "); break;
    case 3:     fprintf(yyout,"jchar "); break;
    case 9:     fprintf(yyout,"jref ",arg_ids[i]); break;
    case 8: return;
    }

  fprintf(yyout,"id%i",i);
}

/* when the cpp file doesn't have enough info use the hint file */
use_hints()
{
  char h_cls[80];
  char h_func[80];
  int  h_type;
  int  i;

  if (have_hint)
    {
    /* use the hint */
    switch (arg_types[10])
      {
      case 301:
	fprintf(yyout,"    return vtkJavaMakeJArrayOfDoubleFromFloat(env,temp10,%i);\n",hint_size);
	break;
      case 307:  
	fprintf(yyout,"    return vtkJavaMakeJArrayOfDoubleFromDouble(env,temp10,%i);\n",hint_size);
	break;
      case 304: 
	fprintf(yyout,"    return vtkJavaMakeJArrayOfIntFromInt(env,temp10,%i);\n",hint_size);
	break;

      case 305: case 306: case 313: case 314: case 315: case 316:
	  break;
      }
    return;
    }

  /* reset the position */
  rewind(fhint);

  /* first find a hint */
  while (fscanf(fhint,"%s %s %i %i",h_cls,h_func,&h_type,&hint_size) != EOF)
    {
    if ((!strcmp(h_cls,class_name))&&(!strcmp(h_func,func_name))&&
	(h_type == arg_types[10]))
      {
      /* use the hint */
      switch (h_type%1000)
	{
	case 301:
	  fprintf(yyout,"    return vtkJavaMakeJArrayOfDoubleFromFloat(env,temp10,%i);\n",hint_size);
	  break;
	case 307:  
	  fprintf(yyout,"    return vtkJavaMakeJArrayOfDoubleFromDouble(env,temp10,%i);\n",hint_size);
	  break;
	case 304: 
	  fprintf(yyout,"    return vtkJavaMakeJArrayOfIntFromInt(env,temp10,%i);\n",hint_size);
	  break;
	  
	case 305: case 306: case 313: case 314: case 315: case 316:
	  break;
	}
      }
    }
}

return_result()
{
  switch (arg_types[10]%1000)
    {
    case 1: fprintf(yyout,"jdouble "); break;
    case 2: fprintf(yyout,"void "); break;
    case 3: fprintf(yyout,"jchar "); break;
    case 7: fprintf(yyout,"jdouble "); break;
    case 4: case 5: case 6: case 13: case 14: case 15: case 16:
      fprintf(yyout,"jint "); 
      break;
    case 303: fprintf(yyout,"jstring "); break;
    case 109:
    case 309:  
      fprintf(yyout,"jobject ",arg_ids[10]);
      break;

    case 301: case 307:
    case 304: case 305: case 306:
      fprintf(yyout,"jarray "); break;
    }
}


output_temp(int i)
{
  /* handle VAR FUNCTIONS */
  if (arg_types[i] == 5000)
    {
    fprintf(yyout,"    vtkTclVoidFuncArg *temp%i = new vtkTclVoidFuncArg;\n",i);
    return;
    }
  
  /* ignore void */
  if (((arg_types[i] % 10) == 2)&&(!((arg_types[i]%1000)/100)))
    {
    return;
    }

  if ((arg_types[i]%100)/10 == 1)
    {
    fprintf(yyout,"  unsigned ");
    }
  else
    {
    fprintf(yyout,"  ");
    }

  switch (arg_types[i]%10)
    {
    case 1:   fprintf(yyout,"float  "); break;
    case 7:   fprintf(yyout,"double "); break;
    case 4:   fprintf(yyout,"int    "); break;
    case 5:   fprintf(yyout,"short  "); break;
    case 6:   fprintf(yyout,"long   "); break;
    case 2:     fprintf(yyout,"void   "); break;
    case 3:     fprintf(yyout,"char   "); break;
    case 9:     
      fprintf(yyout,"%s ",arg_ids[i]); break;
    case 8: return;
    }
  
  switch ((arg_types[i]%1000)/100)
    {
    case 1: fprintf(yyout, " *"); break; /* act " &" */
    case 2: fprintf(yyout, "&&"); break;
    case 3: 
      if ((i == 10)||
	  (arg_types[i]%10 == 9)||
	  (arg_types[i] == 303)) 
	{
	fprintf(yyout, " *"); 
	}
      break;
    case 4: fprintf(yyout, "&*"); break;
    case 5: fprintf(yyout, "*&"); break;
    case 7: fprintf(yyout, "**"); break;
    default: fprintf(yyout,"  "); break;
    }
  fprintf(yyout,"temp%i",i);
  
  /* handle arrays */
  if ((arg_types[i]%1000/100 == 3)&&
      (i != 10)&&
      (arg_types[i]%10 != 9)&&
      (arg_types[i] != 303))
    {
    fprintf(yyout,"[%i]",arg_counts[i]);
    fprintf(yyout,";\n  void *tempArray");
    }

  fprintf(yyout,";\n");
  if ((i == 10) && ((arg_types[i]%1000 == 309)||(arg_types[i]%1000 == 109)))
    {
    fprintf(yyout,"  jobject tempH;\n");
    }
}

get_args(int i)
{
  int j;

  /* handle VAR FUNCTIONS */
  if (arg_types[i] == 5000)
    {
    fprintf(yyout,"    temp%i->interp = interp;\n",i);
    fprintf(yyout,"    temp%i->command = strcpy(new char [strlen(argv[2])+1],argv[2]);\n",i);
    return;
    }

  /* ignore void */
  if (((arg_types[i] % 10) == 2)&&(!((arg_types[i]%1000)/100)))
    {
    return;
    }

  switch (arg_types[i]%1000)
    {
    case 3:
      fprintf(yyout,"  temp%i = (char)(0xff & id%i);\n",i,i);
      break;
    case 303:
      fprintf(yyout,"  temp%i = vtkJavaUTFToChar(env,id%i);\n",i,i);
      break;
    case 109:
    case 309:
      fprintf(yyout,"  temp%i = (%s *)(vtkJavaGetPointerFromObject(env,id%i,\"%s\"));\n",i,arg_ids[i],i,arg_ids[i]);
      break;
    case 301:
    case 307:
      fprintf(yyout,"  tempArray = (void *)(env->GetDoubleArrayElements(id%i,NULL));\n",i);
      for (j = 0; j < arg_counts[i]; j++)
	{
	fprintf(yyout,"  temp%i[%i] = ((jdouble *)tempArray)[%i];\n",i,j,j);
	}
      fprintf(yyout,"  env->ReleaseDoubleArrayElements(id%i,(jdouble *)tempArray,0);\n",i);      
      break;
    case 304:
    case 306:
      fprintf(yyout,"  tempArray = (void *)(env->GetLongArrayElements(id%i,NULL));\n",i);
      for (j = 0; j < arg_counts[i]; j++)
	{
	fprintf(yyout,"  temp%i[%i] = ((jlong *)tempArray)[%i];\n",i,j,j);
	}
      fprintf(yyout,"  env->ReleaseLongArrayElements(id%i,(jlong *)tempArray,0);\n",i);      
      break;
    case 2:    
    case 9: break;
    default: fprintf(yyout,"  temp%i = id%i;\n",i,i); break;
    }
}


do_return()
{
  /* ignore void */
  if (((arg_types[10] % 10) == 2)&&(!((arg_types[10]%1000)/100)))
    {
    return;
    }

  switch (arg_types[10]%1000)
    {
    case 303: fprintf(yyout,
		      "  return vtkJavaMakeJavaString(env,temp10);\n"); 
    break;
    case 109:
    case 309:  
      {
      fprintf(yyout,"  tempH = vtkJavaGetObjectFromPointer((void *)temp10);\n");
      fprintf(yyout,"  if (!tempH)\n    {\n");
      fprintf(yyout,"    vtk_%s_NoCPP();\n",arg_ids[10]);
      fprintf(yyout,"    tempH = env->NewObject(env->FindClass(\"vtk/%s\"),env->GetMethodID(env->FindClass(\"vtk/%s\"),\"<init>\",\"()V\"));\n",arg_ids[10],arg_ids[10]);
      fprintf(yyout,"    vtkJavaAddObjectToHash(env, tempH,(void *)temp10,(void *)%s_Typecast,0);\n    }\n",arg_ids[10]);
      fprintf(yyout,"  return tempH;\n",arg_ids[10]);
      break;
      }
      
    /* handle functions returning vectors */
    /* this is done by looking them up in a hint file */
    case 301: case 307:
    case 304: case 305: case 306:
      use_hints();
      break;
    default: fprintf(yyout,"  return temp10;\n"); break;
    }
}

void handle_vtkobj_return()
{
  fprintf(yyout,"extern void *%s_Typecast(void *,char *);\n",arg_ids[10]);
  fprintf(yyout,"extern void vtk_%s_NoCPP();\n",arg_ids[10]);
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
	  if (((arg_types[j] != 309)&&(funcArgTypes[i][j] != 109))&&
	      ((arg_types[j] != 109)&&(funcArgTypes[i][j] != 309)))
	    {
	    match = 0;
	    }
	  }
	}
      if (arg_types[10] != funcArgTypes[i][10])
	{
	if (((arg_types[10] != 309)&&(funcArgTypes[i][10] != 109))&&
	    ((arg_types[10] != 109)&&(funcArgTypes[i][10] != 309)))
	  {
	  match = 0;
	  }
	}
      if (match) return 1;
      }
    }
  return 0;
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

  /* eliminate unsigned char * */
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
	fprintf(yyout,"\n");

	/* does this return a vtkObject if so must do special stuff */
	if ((arg_types[10]%1000 == 309)||(arg_types[10]%1000 == 109))
	  {
	  handle_vtkobj_return();
	  }
	fprintf(yyout,"extern \"C\" JNIEXPORT ");
	return_result();
	fprintf(yyout," JNICALL Java_vtk_%s_%s_1%i(JNIEnv *env, jobject obj",
		class_name,func_name, numFuncs);
	
	for (i = 0; i < num_args; i++)
	  {
	  fprintf(yyout,",");
	  output_proto_vars(i);
	  }
	fprintf(yyout,")\n{\n");
	
	/* get the object pointer */
	fprintf(yyout,"  %s *op;\n",class_name);
	/* process the args */
	for (i = 0; i < num_args; i++)
	  {
	  output_temp(i);
	  }
	output_temp(10);
	
	/* now get the required args from the stack */
	for (i = 0; i < num_args; i++)
	  {
	  get_args(i);
	  }
	
	fprintf(yyout,"\n  op = (%s *)vtkJavaGetPointerFromObject(env,obj,\"%s\");\n",
		class_name,class_name);
	
	
	switch (arg_types[10]%1000)
	  {
	  case 2:
	    fprintf(yyout,"  op->%s(",func_name);
	    break;
	  case 109:
	    fprintf(yyout,"  temp10 = &(op)->%s(",func_name);
	    break;
	  default:
	    fprintf(yyout,"  temp10 = (op)->%s(",func_name);
	  }
	for (i = 0; i < num_args; i++)
	  {
	  if (i)
	    {
	    fprintf(yyout,",");
	    }
	  if (arg_types[i] == 109)
	    {
	    fprintf(yyout,"*(temp%i)",i);
	    }
	  else if (arg_types[i] == 5000)
	    {
	    fprintf(yyout,"vtkTclVoidFunc,(void *)temp%i",i);
	    }
	  else
	    {
	    fprintf(yyout,"temp%i",i);
	    }
	  }
	fprintf(yyout,");\n");
	if (arg_types[0] == 5000)
	  {
	  fprintf(yyout,"      op->%sArgDelete(vtkTclVoidFuncArgDelete);\n",
		  func_name);
	  }
	
	do_return();
	fprintf(yyout,"}\n");
	
	funcNames[numFuncs] = strdup(func_name);
	funcArgs[numFuncs] = num_args;
	for (i = 0; i < num_args; i++)
	  {
	  funcArgTypes[numFuncs][i] = arg_types[i];
	  if (funcArgTypes[numFuncs][i] == 109)
	    {
	    funcArgTypes[numFuncs][i] = 309;
	    }
	  }
	funcArgTypes[numFuncs][10] = arg_types[10];
	if (funcArgTypes[numFuncs][10] == 109)
	  {
	  funcArgTypes[numFuncs][10] = 309;
	  }
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
 


