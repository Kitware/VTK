%{
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define yyerror(a) fprintf(stderr,"%s\n",a)
#define yywrap() 1
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
char *arg_ids[11];
int arg_failure;
char temps[80];
char *funcNames[1000];
int   funcArgs[1000];
int  numFuncs = 0;
int is_concrete;
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
      fprintf(yyout,"// tcl wrapper for %s object\n//\n",class_name);
      fprintf(yyout,"#ifdef _WIN32\n");
      fprintf(yyout,"#include <strstrea.h>\n");
      fprintf(yyout,"#else\n");
      fprintf(yyout,"#include <strstream.h>\n");
      fprintf(yyout,"#endif\n");
      fprintf(yyout,"#include \"%s.h\"\n\n",class_name);
      fprintf(yyout,"#include \"vtkTclUtil.h\"\n");
      if (is_concrete)
	{
	fprintf(yyout,"\nClientData %sNewCommand()\n{\n",class_name);
	fprintf(yyout,"  %s *temp = new %s;\n",class_name,class_name);
	fprintf(yyout,"  return ((ClientData)temp);\n}\n\n");
	}
      }
    optional_scope 
      {
      int i;
      for (i = 0; i < num_superclasses; i++)
	{
	fprintf(yyout,"int %sCppCommand(%s *op, Tcl_Interp *interp,\n             int argc, char *argv[]);\n",superclasses[i],superclasses[i]);
	}
      fprintf(yyout,"int %sCppCommand(%s *op, Tcl_Interp *interp,\n             int argc, char *argv[]);\n",class_name,class_name);
      fprintf(yyout,"\nint %sCommand(ClientData cd, Tcl_Interp *interp,\n             int argc, char *argv[])\n{\n",class_name);
      fprintf(yyout,"  if ((!strcmp(\"Delete\",argv[1]))&&(argc == 2)&& !vtkTclInDelete())\n    {\n");
      fprintf(yyout,"    Tcl_DeleteCommand(interp,argv[0]);\n");
      fprintf(yyout,"    return TCL_OK;\n    }\n");
      fprintf(yyout,"   return %sCppCommand((%s *)cd,interp, argc, argv);\n}\n",class_name,class_name);

      fprintf(yyout,"\nint %sCppCommand(%s *op, Tcl_Interp *interp,\n             int argc, char *argv[])\n{\n",class_name,class_name);
      fprintf(yyout,"  int    tempi;\n");
      fprintf(yyout,"  double tempd;\n");
      fprintf(yyout,"  static char temps[80];\n");
      fprintf(yyout,"  int    error;\n\n");
      fprintf(yyout,"  tempi = 0;\n");
      fprintf(yyout,"  tempd = 0;\n");
      fprintf(yyout,"  temps[0] = 0;\n\n");

      /* stick in the typecasting and delete functionality here */
      fprintf(yyout,"  if (!interp)\n    {\n");
      fprintf(yyout,"    if (!strcmp(\"DoTypecasting\",argv[0]))\n      {\n");
      fprintf(yyout,"      if (!strcmp(\"%s\",argv[1]))\n        {\n",
	      class_name);
      fprintf(yyout,"        argv[2] = (char *)((void *)op);\n");
      fprintf(yyout,"        return TCL_OK;\n        }\n");

      /* check our superclasses */
      for (i = 0; i < num_superclasses; i++)
	{
	fprintf(yyout,"      if (%sCppCommand((%s *)op,interp,argc,argv) == TCL_OK)\n        {\n",
		superclasses[i],superclasses[i]);
	fprintf(yyout,"        return TCL_OK;\n        }\n");      
	}
      fprintf(yyout,"      }\n    return TCL_ERROR;\n    }\n\n");

      }
    '{' class_def_body '}'
      {
      int i;

      /* handle any special classes */
      handle_special();
      
      /* add the ListInstances method */
      fprintf(yyout,"\n  if (!strcmp(\"ListInstances\",argv[1]))\n    {\n");
      fprintf(yyout,"    vtkTclListInstances(interp,%sCommand);\n",class_name);
      fprintf(yyout,"    return TCL_OK;\n    }\n");

      /* add the ListMethods method */
      fprintf(yyout,"\n  if (!strcmp(\"ListMethods\",argv[1]))\n    {\n");
      /* recurse up the tree */
      for (i = 0; i < num_superclasses; i++)
	{
        fprintf(yyout,"    %sCppCommand(op,interp,argc,argv);\n",
	      superclasses[i]);
	}
      /* now list our methods */
      fprintf(yyout,"    Tcl_AppendResult(interp,\"Methods from %s:\\n\",NULL);\n",class_name);
      for (i = 0; i < numFuncs; i++)
	{
	if (funcArgs[i] > 1)
	  {
	  fprintf(yyout,"    Tcl_AppendResult(interp,\"  %s\\t with %i args\\n\",NULL);\n",
		  funcNames[i],funcArgs[i]);
	  }
	if (funcArgs[i] == 1)
	  {
	  fprintf(yyout,"    Tcl_AppendResult(interp,\"  %s\\t with 1 arg\\n\",NULL);\n",
		  funcNames[i]);
	  }
	if (funcArgs[i] == 0)
	  {
	  fprintf(yyout,"    Tcl_AppendResult(interp,\"  %s\\n\",NULL);\n",funcNames[i]);
	  }
	}
      fprintf(yyout,"    return TCL_OK;\n    }\n");

      /* try superclasses */
      for (i = 0; i < num_superclasses; i++)
	{
	fprintf(yyout,"\n  if (%sCppCommand((%s *)op,interp,argc,argv) == TCL_OK)\n",
		superclasses[i],superclasses[i]);
        fprintf(yyout,"    {\n    return TCL_OK;\n    }\n");
	}

      /* add the default print method to Object */
      if (!strcmp("vtkObject",class_name))
	{
	fprintf(yyout,"  if ((!strcmp(\"Print\",argv[1]))&&(argc == 2))\n    {\n");
	fprintf(yyout,"    ostrstream buf;\n");
	fprintf(yyout,"    op->Print(buf);\n");
	fprintf(yyout,"    buf.put('\\0');\n");
	fprintf(yyout,"    Tcl_SetResult(interp,buf.str(),TCL_VOLATILE);\n");
	fprintf(yyout,"    delete buf.str();\n");
	fprintf(yyout,"    return TCL_OK;\n    }\n");
	}
      fprintf(yyout,"\n  if ((argc >= 2)&&(!strstr(interp->result,\"Object named:\")))\n    {\n");
      fprintf(yyout,"    char temps2[256];\n    sprintf(temps2,\"Object named: %%s, could not find requested method: %%s\\nor the method was called with incorrect arguments.\n\",argv[0],argv[1]);\n    Tcl_AppendResult(interp,temps2,NULL);\n    }\n  if (argc < 2)\n    {\n");
      fprintf(yyout,"    sprintf(interp->result,\"Could not find requested method.\");\n    }\n");
      fprintf(yyout,"  return TCL_ERROR;\n}\n");
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
       fprintf(stderr,"   Converted func %s\n",$<str>1);};

any_id: VTK_ID | ID;

func_end: ';' 
    | '{' maybe_other '}' ';' 
    | '{' maybe_other '}'  
    | ':' maybe_other_no_semi ';';

args_list: | more_args;

more_args: arg { num_args++;} | arg {num_args++;} ',' more_args;

arg: type {arg_types[num_args] = $<integer>1;} 
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
   output_function();
   }
| SetObjectMacro '(' any_id ',' type_red2 ')'
   { 
   is_virtual = 0;
   sprintf(temps,"Set%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 1;
   arg_types[0] = 309;
   output_function();
   }
| SetRefCountedObjectMacro '(' any_id ',' type_red2 ')'
   { 
   is_virtual = 0;
   sprintf(temps,"Set%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 1;
   arg_types[0] = 309;
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
   arg_types[1] = $<integer>5;
   output_function();
   }
| GetVector2Macro  '(' any_id ',' type_red2 ')'
   { 
   int i;

   is_virtual = 0;
   sprintf(temps,"Get%s",$<str>3); 

   /* check to see if we can handle the args */
   if (($<integer>5 != 2) &&
       (($<integer>5 < 8) ||
        ($<integer>5 == 13) ||
        ($<integer>5 == 14) ||
        ($<integer>5 == 15))) 
      {
      fprintf(yyout,"  if ((!strcmp(\"%s\",argv[1]))&&(argc == 2))\n    {\n",
	      temps);

    /* process the args */
    switch ($<integer>5)
      {
      case 1:   fprintf(yyout,"    float  "); break;
      case 7:   fprintf(yyout,"    double "); break;
      case 4:   fprintf(yyout,"    int    "); break;
      case 5:   fprintf(yyout,"    short  "); break;
      case 6:   fprintf(yyout,"    long   "); break;
      case 3:   fprintf(yyout,"    char   "); break;
      case 13:   fprintf(yyout,"    unsigned char  "); break;
      case 14:   fprintf(yyout,"    unsigned int   "); break;
      case 15:   fprintf(yyout,"    unsigned short "); break;
      }
    fprintf(yyout,"*temp;\n\n");

    /* invoke the function */
    fprintf(yyout,"    temp = op->%s();\n",temps);

    /* now return the args on the stack */
    fprintf(yyout,"    interp->result[0] = '\\0';\n"); 
    for (i = 0; i < 2; i++)
      {
      switch ($<integer>5)
	{
	case 1: case 7:  
	  fprintf(yyout,"    sprintf(temps,\"%%g\",temp[%i]);\n",i); 
	  break;
	case 4: 
	  fprintf(yyout,"    sprintf(temps,\"%%i\",temp[%i]);\n",i); 
	  break;
	case 5: 
	  fprintf(yyout,"    sprintf(temps,\"%%hi\",temp[%i]);\n",i); 
	  break;
	case 6: 
	  fprintf(yyout,"    sprintf(temps,\"%%li\",temp[%i]);\n",i); 
	  break;
	case 3:
	  fprintf(yyout,"    sprintf(temps,\"%%c\",temp[%i]);\n",i); 
	  break;
	case 14: 
	  fprintf(yyout,"    sprintf(temps,\"%%u\",temp[%i]);\n",i); 
	  break;
	case 15: 
	  fprintf(yyout,"    sprintf(temps,\"%%hu\",temp[%i]);\n",i); 
	  break;
	case 13: 
	  fprintf(yyout,"    sprintf(temps,\"%%u\",(int)temp[%i]);\n",i); 
	  break;
	}
      fprintf(yyout,"    Tcl_AppendElement(interp,temps);\n");
      }
      
    fprintf(yyout,"    return TCL_OK;\n    }\n");
    funcNames[numFuncs] = strdup(temps);
    funcArgs[numFuncs] = 0;
    numFuncs++;
    }
   }
| SetVector3Macro '(' any_id ',' type_red2 ')'
   { 
   is_virtual = 0;
   sprintf(temps,"Set%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 3;
   arg_types[0] = $<integer>5;
   arg_types[1] = $<integer>5;
   arg_types[2] = $<integer>5;
   output_function();
   }
| GetVector3Macro  '(' any_id ',' type_red2 ')'
   { 
   int i;

   is_virtual = 0;
   sprintf(temps,"Get%s",$<str>3); 

   /* check to see if we can handle the args */
   if (($<integer>5 != 2) &&
       (($<integer>5 < 8) ||
        ($<integer>5 == 13) ||
        ($<integer>5 == 14) ||
        ($<integer>5 == 15))) 
      {
      fprintf(yyout,"  if ((!strcmp(\"%s\",argv[1]))&&(argc == 2))\n    {\n",
	      temps);

    /* process the args */
    switch ($<integer>5)
      {
      case 1:   fprintf(yyout,"    float  "); break;
      case 7:   fprintf(yyout,"    double "); break;
      case 4:   fprintf(yyout,"    int    "); break;
      case 5:   fprintf(yyout,"    short  "); break;
      case 6:   fprintf(yyout,"    long   "); break;
      case 3:   fprintf(yyout,"    char   "); break;
      case 13:   fprintf(yyout,"    unsigned char  "); break;
      case 14:   fprintf(yyout,"    unsigned int   "); break;
      case 15:   fprintf(yyout,"    unsigned short "); break;
      }
    fprintf(yyout,"*temp;\n\n");

    /* invoke the function */
    fprintf(yyout,"    temp = op->%s();\n",temps);

    /* now return the args on the stack */
    fprintf(yyout,"    interp->result[0] = '\\0';\n"); 
    for (i = 0; i < 3; i++)
      {
      switch ($<integer>5)
	{
	case 1: case 7:  
	  fprintf(yyout,"    sprintf(temps,\"%%g\",temp[%i]);\n",i); 
	  break;
	case 4: 
	  fprintf(yyout,"    sprintf(temps,\"%%i\",temp[%i]);\n",i); 
	  break;
	case 5: 
	  fprintf(yyout,"    sprintf(temps,\"%%hi\",temp[%i]);\n",i); 
	  break;
	case 6: 
	  fprintf(yyout,"    sprintf(temps,\"%%li\",temp[%i]);\n",i); 
	  break;
	case 3:
	  fprintf(yyout,"    sprintf(temps,\"%%c\",temp[%i]);\n",i); 
	  break;
	case 14: 
	  fprintf(yyout,"    sprintf(temps,\"%%u\",temp[%i]);\n",i); 
	  break;
	case 15: 
	  fprintf(yyout,"    sprintf(temps,\"%%hu\",temp[%i]);\n",i); 
	  break;
	case 13: 
	  fprintf(yyout,"    sprintf(temps,\"%%u\",(int)temp[%i]);\n",i); 
	  break;
	}
      fprintf(yyout,"    Tcl_AppendElement(interp,temps);\n");
      }
      
    fprintf(yyout,"    return TCL_OK;\n    }\n");
    funcNames[numFuncs] = strdup(temps);
    funcArgs[numFuncs] = 0;
    numFuncs++;
    }
   }
| SetVector4Macro '(' any_id ',' type_red2 ')'
   { 
   is_virtual = 0;
   sprintf(temps,"Set%s",$<str>3); 
   func_name = strdup(temps);
   num_args = 4;
   arg_types[0] = $<integer>5;
   arg_types[1] = $<integer>5;
   arg_types[2] = $<integer>5;
   arg_types[3] = $<integer>5;
   output_function();
   }
| GetVector4Macro  '(' any_id ',' type_red2 ')'
   { 
   int i;

   is_virtual = 0;
   sprintf(temps,"Get%s",$<str>3); 

   /* check to see if we can handle the args */
   if (($<integer>5 != 2) &&
       (($<integer>5 < 8) ||
        ($<integer>5 == 13) ||
        ($<integer>5 == 14) ||
        ($<integer>5 == 15))) 
      {
      fprintf(yyout,"  if ((!strcmp(\"%s\",argv[1]))&&(argc == 2))\n    {\n",
	      temps);

    /* process the args */
    switch ($<integer>5)
      {
      case 1:   fprintf(yyout,"    float  "); break;
      case 7:   fprintf(yyout,"    double "); break;
      case 4:   fprintf(yyout,"    int    "); break;
      case 5:   fprintf(yyout,"    short  "); break;
      case 6:   fprintf(yyout,"    long   "); break;
      case 3:   fprintf(yyout,"    char   "); break;
      case 13:   fprintf(yyout,"    unsigned char  "); break;
      case 14:   fprintf(yyout,"    unsigned int   "); break;
      case 15:   fprintf(yyout,"    unsigned short "); break;
      }
    fprintf(yyout,"*temp;\n\n");

    /* invoke the function */
    fprintf(yyout,"    temp = op->%s();\n",temps);

    /* now return the args on the stack */
    fprintf(yyout,"    interp->result[0] = '\\0';\n"); 
    for (i = 0; i < 4; i++)
      {
      switch ($<integer>5)
	{
	case 1: case 7:  
	  fprintf(yyout,"    sprintf(temps,\"%%g\",temp[%i]);\n",i); 
	  break;
	case 4: 
	  fprintf(yyout,"    sprintf(temps,\"%%i\",temp[%i]);\n",i); 
	  break;
	case 5: 
	  fprintf(yyout,"    sprintf(temps,\"%%hi\",temp[%i]);\n",i); 
	  break;
	case 6: 
	  fprintf(yyout,"    sprintf(temps,\"%%li\",temp[%i]);\n",i); 
	  break;
	case 3:
	  fprintf(yyout,"    sprintf(temps,\"%%c\",temp[%i]);\n",i); 
	  break;
	case 14: 
	  fprintf(yyout,"    sprintf(temps,\"%%u\",temp[%i]);\n",i); 
	  break;
	case 15: 
	  fprintf(yyout,"    sprintf(temps,\"%%hu\",temp[%i]);\n",i); 
	  break;
	case 13: 
	  fprintf(yyout,"    sprintf(temps,\"%%u\",(int)temp[%i]);\n",i); 
	  break;
	}
      fprintf(yyout,"    Tcl_AppendElement(interp,temps);\n");
      }
      
     fprintf(yyout,"    return TCL_OK;\n    }\n");
     funcNames[numFuncs] = strdup(temps);
     funcArgs[numFuncs] = 0;
     numFuncs++;
     }
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

   sprintf(temps,"Set%s",$<str>3); 
   is_virtual = 0;

  /* check to see if we can handle the args */
  if (($<integer>5 != 2)&&($<integer>5 < 8)&&($<integer>7 >= 0)) 
    {
    fprintf(yyout,"  if ((!strcmp(\"%s\",argv[1]))&&(argc == %i))\n    {\n",
	    temps,$<integer>7 + 2);

    /* process the args */
    switch ($<integer>5)
      {
      case 1:   fprintf(yyout,"    float  "); break;
      case 7:   fprintf(yyout,"    double "); break;
      case 4:   fprintf(yyout,"    int    "); break;
      case 5:   fprintf(yyout,"    short  "); break;
      case 6:   fprintf(yyout,"    long   "); break;
      case 3:   fprintf(yyout,"    char   "); break;
      }
    fprintf(yyout,"temp[%i];\n\n",$<integer>7);
    fprintf(yyout,"    error = 0;\n\n");

    /* now get the required args from the stack */
    for (i = 0; i < $<integer>7; i++)
      {
      switch ($<integer>5)
	{
	case 1: case 7:  
	  fprintf(yyout,
		  "    if (Tcl_GetDouble(interp,argv[%i],&tempd) != TCL_OK) error = 1;\n",
		  i+2); 
	  fprintf(yyout,"    temp[%i] = tempd;\n",i);
	  break;
	case 4: case 5: case 6: 
	  fprintf(yyout,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
		  i+2); 
	  fprintf(yyout,"    temp[%i] = tempi;\n",i);
	  break;
	case 3:
	  fprintf(yyout,"    temp[%i] = *(argv[%i]);\n",i,i+2);
	  break;
	}
      }
      
    fprintf(yyout,"    if (!error)\n      {\n      op->%s(temp);\n",temps);
    fprintf(yyout,"      return TCL_OK;\n      }\n");
    fprintf(yyout,"    }\n");
    funcNames[numFuncs] = strdup(temps);
    funcArgs[numFuncs] = $<integer>7;
    numFuncs++;
    }
   }
| GetVectorMacro  '(' any_id ',' type_red2 ',' float_num ')'
   { 
   int i;

   is_virtual = 0;
   sprintf(temps,"Get%s",$<str>3); 

  /* check to see if we can handle the args */
  if (($<integer>5 != 2) &&
      (($<integer>5 < 8) ||
       ($<integer>5 == 13) ||
       ($<integer>5 == 14) ||
       ($<integer>5 == 15)) &&
      ($<integer>7 >= 0)) 
    {
    fprintf(yyout,"  if ((!strcmp(\"%s\",argv[1]))&&(argc == 2))\n    {\n",
	    temps);

    /* process the args */
    switch ($<integer>5)
      {
      case 1:   fprintf(yyout,"    float  "); break;
      case 7:   fprintf(yyout,"    double "); break;
      case 4:   fprintf(yyout,"    int    "); break;
      case 5:   fprintf(yyout,"    short  "); break;
      case 6:   fprintf(yyout,"    long   "); break;
      case 3:   fprintf(yyout,"    char   "); break;
      case 13:   fprintf(yyout,"    unsigned char  "); break;
      case 14:   fprintf(yyout,"    unsigned int   "); break;
      case 15:   fprintf(yyout,"    unsigned short "); break;
      }
    fprintf(yyout,"*temp;\n\n");

    /* invoke the function */
    fprintf(yyout,"    temp = op->%s();\n",temps);

    /* now return the args on the stack */
    fprintf(yyout,"    interp->result[0] = '\\0';\n"); 
    for (i = 0; i < $<integer>7; i++)
      {
      switch ($<integer>5)
	{
	case 1: case 7:  
	  fprintf(yyout,"    sprintf(temps,\"%%g\",temp[%i]);\n",i); 
	  break;
	case 4: 
	  fprintf(yyout,"    sprintf(temps,\"%%i\",temp[%i]);\n",i); 
	  break;
	case 5: 
	  fprintf(yyout,"    sprintf(temps,\"%%hi\",temp[%i]);\n",i); 
	  break;
	case 6: 
	  fprintf(yyout,"    sprintf(temps,\"%%li\",temp[%i]);\n",i); 
	  break;
	case 3:
	  fprintf(yyout,"    sprintf(temps,\"%%c\",temp[%i]);\n",i); 
	  break;
	case 14: 
	  fprintf(yyout,"    sprintf(temps,\"%%u\",temp[%i]);\n",i); 
	  break;
	case 15: 
	  fprintf(yyout,"    sprintf(temps,\"%%hu\",temp[%i]);\n",i); 
	  break;
	case 13: 
	  fprintf(yyout,"    sprintf(temps,\"%%u\",(int)temp[%i]);\n",i); 
	  break;
	}
      fprintf(yyout,"    Tcl_AppendElement(interp,temps);\n");
      }
      
    fprintf(yyout,"    return TCL_OK;\n    }\n");
    funcNames[numFuncs] = strdup(temps);
    funcArgs[numFuncs] = 0;
    numFuncs++;
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
   | STRING | type_red2 | NUM | CLASS_REF | '&' | brackets | CONST | OPERATOR
   | '-' | '~' | STATIC | ARRAY_NUM;

braces: '{' maybe_other '}';
parens: '(' maybe_other ')';
brackets: '[' maybe_other ']';

%%
#include "lex.yy.c"
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
    fprintf(yyout,"    unsigned ");
    }
  else
    {
    fprintf(yyout,"    ");
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
    case 9:     fprintf(yyout,"%s ",arg_ids[i]); break;
    case 8: return;
    }

  switch ((arg_types[i]%1000)/100)
    {
    case 1: fprintf(yyout, " *"); break; /* act " &" */
    case 2: fprintf(yyout, "&&"); break;
    case 3: fprintf(yyout, " *"); break;
    case 4: fprintf(yyout, "&*"); break;
    case 5: fprintf(yyout, "*&"); break;
    case 7: fprintf(yyout, "**"); break;
    default: fprintf(yyout,"  "); break;
    }
    
  fprintf(yyout,"temp%i",i);

  /* the following is currently not used */
  if (arg_types[i] >= 10000)
    {
    fprintf(yyout,"[%i]",arg_types[i]/10000);
    }
  fprintf(yyout,";\n");
}

/* when the cpp file doesn't have enough info use the hint file */
use_hints()
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
      /* use the hint */
      switch (h_type%1000)
	{
	case 301: case 307:  
	  fprintf(yyout,"    sprintf(interp->result,\"");
	  for (i = 0; i < h_value; i++)
	    {
	    fprintf(yyout,"%%g ");
	    }
	  fprintf(yyout,"\"");
	  for (i = 0; i < h_value; i++)
	    {
	    fprintf(yyout,",temp10[%i]",i);
	    }
	  fprintf(yyout,");\n");
	  break;
	case 304: case 305: case 306: case 313:
	  fprintf(yyout,"    sprintf(interp->result,\"");
	  for (i = 0; i < h_value; i++)
	    {
	    fprintf(yyout,"%%i ");
	    }
	  fprintf(yyout,"\"");
	  for (i = 0; i < h_value; i++)
	    {
	    fprintf(yyout,",temp10[%i]",i);
	    }
	  fprintf(yyout,");\n");
	  break;
	}
      }
    }
}

int have_hints()
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
      /* use the hint */
      return 1;
      }
    }
  return 0;
}

return_result()
{
  switch (arg_types[10]%1000)
    {
    case 2:
      fprintf(yyout,"      interp->result[0] = '\\0';\n"); 
      break;
    case 1: case 7:
      fprintf(yyout,"      sprintf(interp->result,\"%%g\",temp10);\n"); 
      break;
    case 4: 
      fprintf(yyout,"      sprintf(interp->result,\"%%i\",temp10);\n"); 
      break;
    case 5:
      fprintf(yyout,"      sprintf(interp->result,\"%%hi\",temp10);\n"); 
      break;
    case 6:
      fprintf(yyout,"      sprintf(interp->result,\"%%li\",temp10);\n"); 
      break;
    case 14: 
      fprintf(yyout,"      sprintf(interp->result,\"%%u\",temp10);\n"); 
      break;
    case 15:
      fprintf(yyout,"      sprintf(interp->result,\"%%hu\",temp10);\n"); 
      break;
    case 16:
      fprintf(yyout,"      sprintf(interp->result,\"%%lu\",temp10);\n"); 
      break;
    case 13:
      fprintf(yyout,"      sprintf(interp->result,\"%%hu\",temp10);\n"); 
      break;
    case 303:
      fprintf(yyout,"      sprintf(interp->result,\"%%s\",temp10);\n"); 
      break;
    case 3:
      fprintf(yyout,"      sprintf(interp->result,\"%%c\",temp10);\n"); 
      break;
    case 109:
    case 309:  
      fprintf(yyout,"      vtkTclGetObjectFromPointer(interp,(void *)temp10,%sCommand);\n",arg_ids[10]);
      break;

    /* handle functions returning vectors */
    /* this is done by looking them up in a hint file */
    case 301: case 307:
    case 304: case 305: case 306:
      use_hints();
      break;
    default:
      fprintf(yyout,"      sprintf(interp->result,\"unable to return result.\");\n"); 
      break;
    }
}

handle_return_prototype()
{
  switch (arg_types[10]%1000)
    {
    case 109:
    case 309:  
      fprintf(yyout,"    int %sCommand(ClientData, Tcl_Interp *, int, char *[]);\n",arg_ids[10]);
      break;
    }
}

get_args(int i)
{
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
    case 1: case 7:  
      fprintf(yyout,
	      "    if (Tcl_GetDouble(interp,argv[%i],&tempd) != TCL_OK) error = 1;\n",
	      i+2); 
      fprintf(yyout,"    temp%i = tempd;\n",i);
      break;
    case 4: case 5: case 6: 
      fprintf(yyout,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
	      i+2); 
      fprintf(yyout,"    temp%i = tempi;\n",i);
      break;
    case 3:
      fprintf(yyout,"    temp%i = *(argv[%i]);\n",i,i+2);
      break;
    case 13:
      fprintf(yyout,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
	      i+2); 
      fprintf(yyout,"    temp%i = (unsigned char)tempi;\n",i);
      break;
    case 14:
      fprintf(yyout,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
	      i+2); 
      fprintf(yyout,"    temp%i = (unsigned int)tempi;\n",i);
      break;
    case 15:
      fprintf(yyout,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
	      i+2); 
      fprintf(yyout,"    temp%i = (unsigned short)tempi;\n",i);
      break;
    case 303:
      fprintf(yyout,"    temp%i = argv[%i];\n",i,i+2);
      break;
    case 109:
    case 309:
      fprintf(yyout,"    temp%i = (%s *)(vtkTclGetPointerFromObject(argv[%i],\"%s\",interp));\n",i,arg_ids[i],i+2,arg_ids[i]);
      fprintf(yyout,"    if (temp%i == NULL)\n      {  error = 1;  }\n",i);
      break;
    case 2:    
    case 9:
      break;
    }
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
    if ((arg_types[i]%10) == 8) args_ok = 0;
    if ((arg_types[i]%1000 >= 100)&&(arg_types[i]%1000 != 303)&&
	(arg_types[i]%1000 != 309)&&(arg_types[i]%1000 != 109)) args_ok = 0;
    if ((arg_types[i]%100 >= 10)&&(arg_types[i] != 13)&&
	(arg_types[i] != 14)&&(arg_types[i] != 15)) args_ok = 0;
    }
  if ((arg_types[10]%10) == 8) args_ok = 0;
  if (((arg_types[10]%1000)/100 != 3)&&
      ((arg_types[10]%1000)/100 != 1)&&
      ((arg_types[10]%1000)/100)) args_ok = 0;
  if ((arg_types[0] == 5000)&&(num_args != 2)) args_ok = 0;

  /* look for VAR FUNCTIONS */
  if ((arg_types[0] == 5000)&&(num_args == 2)) 
    {
    args_ok = 1;
    num_args = 1;
    }
  
  /* watch out for functions that dont have enough info */
  switch (arg_types[10]%1000)
    {
    case 301: case 307:
    case 304: case 305: case 306:
      args_ok = have_hints();
      break;
    }
  
  if (in_public && args_ok)
    {
    /* make sure it's not a constructor */
    if (strcmp(class_name,func_name))
      {
      fprintf(yyout,"  if ((!strcmp(\"%s\",argv[1]))&&(argc == %i))\n    {\n",
	      func_name,num_args + 2);

      /* process the args */
      for (i = 0; i < num_args; i++)
	{
	output_temp(i);
	}
      output_temp(10);
      handle_return_prototype();
      fprintf(yyout,"    error = 0;\n\n");

      /* now get the required args from the stack */
      for (i = 0; i < num_args; i++)
	{
	get_args(i);
	}
      
      fprintf(yyout,"    if (!error)\n      {\n");
      switch (arg_types[10]%1000)
	{
	case 2:
	  fprintf(yyout,"      op->%s(",func_name);
	  break;
	case 109:
	  fprintf(yyout,"      temp10 = &(op)->%s(",func_name);
	  break;
	default:
	  fprintf(yyout,"      temp10 = (op)->%s(",func_name);
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
      return_result();
      fprintf(yyout,"      return TCL_OK;\n      }\n");
      fprintf(yyout,"    }\n");
      funcNames[numFuncs] = strdup(func_name);
      funcArgs[numFuncs] = num_args;
      numFuncs++;
      }
    }
}

handle_special()
{
  if (!strcmp(class_name,"vtkRenderWindow"))
    {
    fprintf(yyout,"  if ((!strcmp(\"SetTkWindow\",argv[1]))&&(argc == 3))\n");
    fprintf(yyout,"    {\n");
    fprintf(yyout,"    error = 0;\n\n");
    fprintf(yyout,"    if (!error)\n");
    fprintf(yyout,"      {\n");
    fprintf(yyout,"      Tk_Window awin;\n\n");
    fprintf(yyout,"      awin = Tk_NameToWindow(interp,argv[2],Tk_MainWindow(interp));\n");
    fprintf(yyout,"      Tk_MakeWindowExist(awin);\n");
    fprintf(yyout,"      op->SetWindowId((void *)Tk_WindowId(awin));\n");
    fprintf(yyout,"      op->SetDisplayId((void *)Tk_Display(awin));\n");
    fprintf(yyout,"      interp->result[0] = '\\0';\n");
    fprintf(yyout,"      return TCL_OK;\n");
    fprintf(yyout,"      }\n");
    fprintf(yyout,"    }\n");
    }
  if (!strcmp(class_name,"vtkImageViewer"))
    {
    fprintf(yyout,"  if ((!strcmp(\"SetTkWindow\",argv[1]))&&(argc == 3))\n");
    fprintf(yyout,"    {\n");
    fprintf(yyout,"    error = 0;\n\n");
    fprintf(yyout,"    if (!error)\n");
    fprintf(yyout,"      {\n");
    fprintf(yyout,"      Tk_Window awin;\n\n");
    fprintf(yyout,"      awin = Tk_NameToWindow(interp,argv[2],Tk_MainWindow(interp));\n");
    fprintf(yyout,"      Tk_MakeWindowExist(awin);\n");
    fprintf(yyout,"      op->SetDisplayId((void *)Tk_Display(awin));\n");
    fprintf(yyout,"      op->SetWindowId((void *)Tk_WindowId(awin));\n");
    fprintf(yyout,"      interp->result[0] = '\\0';\n");
    fprintf(yyout,"      return TCL_OK;\n");
    fprintf(yyout,"      }\n");
    fprintf(yyout,"    }\n");
    }
}

main(int argc,char *argv[])
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

  file_name = argv[1];

  is_concrete = atoi(argv[3]);
  
  yyin = fin;
  yyout = stdout;
  yyparse();
  return 0;
}
 


