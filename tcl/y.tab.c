
# line 2 "cpp_parse.y"
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

# line 28 "cpp_parse.y"
typedef union
#ifdef __cplusplus
	YYSTYPE
#endif
{
  char *str;
  int   integer;
  } YYSTYPE;
# define CLASS 257
# define PUBLIC 258
# define PRIVATE 259
# define PROTECTED 260
# define VIRTUAL 261
# define STRING 262
# define NUM 263
# define ID 264
# define INT 265
# define FLOAT 266
# define SHORT 267
# define LONG 268
# define DOUBLE 269
# define VOID 270
# define CHAR 271
# define CLASS_REF 272
# define OTHER 273
# define CONST 274
# define OPERATOR 275
# define UNSIGNED 276
# define FRIEND 277
# define VTK_ID 278
# define STATIC 279
# define VAR_FUNCTION 280
# define ARRAY_NUM 281
# define SetMacro 282
# define GetMacro 283
# define SetStringMacro 284
# define GetStringMacro 285
# define SetClampMacro 286
# define SetObjectMacro 287
# define SetReferenceCountedObjectMacro 288
# define GetObjectMacro 289
# define BooleanMacro 290
# define SetVector2Macro 291
# define SetVector3Macro 292
# define SetVector4Macro 293
# define GetVector2Macro 294
# define GetVector3Macro 295
# define GetVector4Macro 296
# define SetVectorMacro 297
# define GetVectorMacro 298
# define ImageSetMacro 299
# define ImageSetExtentMacro 300

#ifdef __STDC__
#include <stdlib.h>
#include <string.h>
#else
#include <malloc.h>
#include <memory.h>
#endif

#ifndef _WIN32
#include <values.h>
#endif

#ifdef __cplusplus

#ifndef yyerror
	void yyerror(const char *);
#endif

#ifndef yylex
#ifdef __EXTERN_C__
	extern "C" { int yylex(void); }
#else
	int yylex(void);
#endif
#endif
	int yyparse(void);

#endif
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern int yyerrflag;
YYSTYPE yylval;
YYSTYPE yyval;
typedef int yytabelem;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
#if YYMAXDEPTH > 0
int yy_yys[YYMAXDEPTH], *yys = yy_yys;
YYSTYPE yy_yyv[YYMAXDEPTH], *yyv = yy_yyv;
#else	/* user does initial allocation */
int *yys;
YYSTYPE *yyv;
#endif
static int yymaxdepth = YYMAXDEPTH;
# define YYERRCODE 256

# line 933 "cpp_parse.y"

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

  /* for const * return types prototype with const */
  if ((i == 10)&&(arg_types[10]%2000 >= 1000))
    {
    fprintf(yyout,"    const ");
    }
  else
    {
    fprintf(yyout,"    ");
    }

  if ((arg_types[i]%100)/10 == 1)
    {
    fprintf(yyout,"unsigned ");
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
  if (num_args && (arg_types[0] == 5000)&&(num_args != 2)) args_ok = 0;

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
      if (num_args && (arg_types[0] == 5000))
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
 


yytabelem yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 62,
	44, 69,
	-2, 68,
-1, 100,
	40, 22,
	-2, 65,
-1, 101,
	40, 23,
	-2, 64,
-1, 176,
	44, 31,
	-2, 30,
	};
# define YYNPROD 127
# define YYLAST 473
yytabelem yyact[]={

    72,    18,   141,    27,   151,     9,   113,    12,    22,    13,
    62,    48,    38,    34,    31,    35,    36,    37,    32,    33,
   112,    11,     4,    18,    10,    27,    39,     9,   113,    12,
    22,    13,   260,   113,   273,   250,    58,    59,    60,    98,
    72,   259,   112,    11,    41,   110,    10,   112,   257,    56,
   264,   103,   144,   151,    30,    38,    34,    31,    35,    36,
    37,    32,    33,   116,   175,    94,    49,    28,   258,    39,
    96,   178,    61,   209,    51,   255,    30,    67,    38,    34,
    31,    35,    36,    37,    32,    33,    26,     2,   140,    23,
    28,    42,    39,    38,    34,    31,    35,    36,    37,    32,
    33,   272,   265,   263,   179,    28,   148,    39,    26,   114,
   108,    23,    63,   105,    43,    44,    54,   270,    46,   249,
   248,   236,   230,   228,    95,   227,   231,   204,    47,   203,
   201,   200,    58,    59,    60,    73,   199,   149,   101,    34,
    31,    35,    36,    37,    32,    33,   106,    97,    94,    98,
    28,    68,   100,    96,    71,   142,    75,    76,    77,    78,
    79,    80,    81,    82,    83,    84,    86,    88,    85,    87,
    89,    92,    93,    90,    91,    73,   198,   104,   101,    34,
    31,    35,    36,    37,    32,    33,   197,   229,    94,    98,
    28,   196,   100,    96,   150,   195,   194,   183,   181,   182,
   193,   101,    34,    31,    35,    36,    37,    32,    33,   192,
   191,    94,    98,    28,   190,   100,    96,     5,   111,   138,
    70,   139,   187,   207,   186,    14,    16,    38,    34,    31,
    35,    36,    37,    32,    33,    17,     6,    20,    21,    28,
    15,    39,    24,   150,    25,   147,    57,    14,    16,    38,
    34,    31,    35,    36,    37,    32,    33,    17,     6,    20,
    21,    28,   271,    39,    24,   173,    25,   154,   155,   156,
   157,   158,   159,   160,   161,   162,   163,   164,   165,   166,
   167,   168,   169,   170,   171,   172,   260,   113,   184,   107,
   268,   145,    74,   267,   117,   146,   253,   247,   262,   180,
   269,   112,    99,   246,   266,    99,   245,   185,    65,    99,
   244,    65,   243,   242,    99,   241,   143,   251,   240,   239,
   238,   237,   235,   234,   205,   208,   202,   189,   188,    50,
   141,   137,   136,   135,   134,    99,   133,    99,   132,   131,
   130,   129,   128,   127,   126,   125,   124,   233,   123,   122,
   121,   120,   119,    19,    29,     8,     7,     3,   102,   254,
   232,   143,   177,   109,   206,   115,   118,   176,   226,   143,
   174,    69,    66,    64,    55,    53,    52,    40,     1,     0,
     0,    99,    99,    45,     0,     0,   252,     0,     0,     0,
     0,     0,   256,     0,     0,     0,   261,   261,     0,     0,
   109,     0,     0,   261,     0,     0,   261,     0,     0,   152,
   153,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,   210,   211,     0,
     0,   212,   213,   214,   215,   216,   217,   218,   219,   220,
   221,   222,   223,     0,   224,   225,     0,     0,   143,     0,
     0,     0,   177,     0,   143,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,    99 };
yytabelem yypact[]={

   -37,-10000000,  -213,   -37,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,
-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,
-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,   -37,   -37,  -252,-10000000,
   -37,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,
   -37,  -267,-10000000,   -59,   288,-10000000,   -19,-10000000,-10000000,-10000000,
-10000000,-10000000,    58,-10000000,  -222,   -51,-10000000,  -268,-10000000,-10000000,
-10000000,  -126,-10000000,   -74,  -126,    55,-10000000,-10000000,   -86,    51,
  -236,    50,  -236,   -63,-10000000,   312,   311,   310,   309,   308,
   306,   305,   304,   303,   302,   301,   300,   299,   298,   296,
   294,   293,   292,   291,  -171,-10000000,  -186,   290,   -15,   253,
-10000000,-10000000,   201,-10000000,-10000000,-10000000,-10000000,  -236,-10000000,-10000000,
    47,   -38,-10000000,-10000000,-10000000,-10000000,  -236,  -236,-10000000,  -258,
  -258,  -258,  -258,  -258,  -258,  -258,  -258,  -258,  -258,  -258,
  -258,  -258,  -258,  -258,  -258,  -258,  -258,  -258,-10000000,-10000000,
  -171,  -209,    45,   -15,-10000000,   253,   253,  -222,-10000000,-10000000,
   -87,   -15,-10000000,-10000000,   180,   178,   287,   286,   170,   166,
   165,   156,   152,   151,   147,   142,   132,    92,    87,    86,
   285,    85,    83,-10000000,   283,-10000000,-10000000,  -258,-10000000,-10000000,
-10000000,-10000000,-10000000,-10000000,-10000000,   -20,  -171,  -171,-10000000,-10000000,
  -171,  -171,  -171,  -171,  -171,  -171,  -171,  -171,  -171,  -171,
  -171,  -171,-10000000,  -171,  -171,    64,    82,-10000000,   -87,   -87,
   282,   281,    77,   280,   279,   278,   277,   274,   272,   271,
   269,   265,   262,   256,    76,    75,-10000000,  -228,-10000000,   -37,
   -15,  -209,    14,-10000000,-10000000,-10000000,   -15,-10000000,-10000000,-10000000,
-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,    23,    23,
    44,   -75,    43,-10000000,-10000000,    23,   252,   249,  -231,-10000000,
    71,-10000000,   221,-10000000,    42,-10000000,-10000000,-10000000,-10000000,-10000000,
  -229,-10000000,-10000000,-10000000 };
yytabelem yypgo[]={

     0,   378,    87,   377,   376,   375,   374,   112,   373,   246,
   372,    77,   371,   292,   220,   147,   370,   368,   155,    64,
   367,   364,    45,   360,   359,    48,   137,   124,   240,    52,
   354,    49,   358,    41,   357,   217,   356,   355,   353 };
yytabelem yyr1[]={

     0,     1,     4,     6,     3,     7,     7,     8,     8,     8,
     8,     8,     8,    11,    11,    11,    11,    11,    11,    13,
    13,    13,    15,    15,    17,    17,    17,    17,    16,    16,
    19,    21,    19,    20,    23,    20,    20,    24,    24,    10,
    10,    22,    26,    26,    26,    14,    14,    14,    14,    27,
    27,    29,    29,    29,    29,    28,    28,    30,    30,    30,
    30,    30,    30,    30,    30,    30,     5,     5,    31,    32,
    31,     9,     9,     9,    25,    25,    33,    33,    33,    12,
    12,    12,    12,    12,    12,    12,    12,    12,    12,    12,
    12,    12,    12,    12,    12,    12,    12,    12,     2,     2,
    18,    18,    34,    34,    35,    35,    35,    35,    35,    35,
    35,    35,    35,    35,    35,    35,    35,    35,    35,    35,
    35,    35,    35,    35,    36,    37,    38 };
yytabelem yyr2[]={

     0,     6,     1,     1,    17,     2,     4,     4,     2,     3,
     5,     5,     3,     4,     6,     3,     5,     7,     5,    11,
     7,    15,     2,     2,     2,     8,     6,     6,     0,     2,
     3,     1,     8,     3,     1,     8,     3,     0,     4,     6,
     4,     4,     0,     5,     9,     5,     3,     5,     7,     3,
     5,     3,     3,     5,     5,     5,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     0,     4,     5,     1,
    10,     3,     3,     3,     4,     2,     3,     7,     3,    13,
    13,     9,     9,    17,    13,    13,    13,    13,    13,    13,
    13,    13,    13,    13,    13,     9,    17,    17,     0,     4,
     0,     4,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     6,     6,     6 };
yytabelem yychk[]={

-10000000,    -1,    -2,   -34,    59,   -35,   273,   -36,   -37,    42,
    61,    58,    44,    46,   262,   -28,   263,   272,    38,   -38,
   274,   275,    45,   126,   279,   281,   123,    40,   276,   -30,
    91,   266,   270,   271,   265,   267,   268,   269,   264,   278,
    -3,   257,    -2,    -2,    -2,   -30,    -2,    -2,   278,   125,
    41,    93,    -4,    -5,    58,    -6,   -31,    -9,   258,   259,
   260,   123,   278,    -7,    -8,    -9,   -10,   -11,   277,   -12,
   -14,   280,   126,   261,   -13,   282,   283,   284,   285,   286,
   287,   288,   289,   290,   291,   294,   292,   295,   293,   296,
   299,   300,   297,   298,   274,   -27,   279,   -15,   275,   -28,
   278,   264,   -32,   125,    -7,    58,   -11,   -14,    59,   -13,
   -22,   -15,   278,   264,    59,   -13,   126,   -14,   -13,    40,
    40,    40,    40,    40,    40,    40,    40,    40,    40,    40,
    40,    40,    40,    40,    40,    40,    40,    40,   -27,   -27,
   274,    40,   -18,   -35,   -29,    38,    42,    44,    59,   -26,
   281,    91,   -13,   -13,   -15,   -15,   -15,   -15,   -15,   -15,
   -15,   -15,   -15,   -15,   -15,   -15,   -15,   -15,   -15,   -15,
   -15,   -15,   -15,   -27,   -16,   -19,   -20,   -14,   280,    59,
   -18,   -29,   -29,   -31,   -26,   -18,    44,    44,    41,    41,
    44,    44,    44,    44,    44,    44,    44,    44,    44,    44,
    44,    44,    41,    44,    44,    41,   -21,   -22,   -15,    93,
   -28,   -28,   -28,   -28,   -28,   -28,   -28,   -28,   -28,   -28,
   -28,   -28,   -28,   -28,   -28,   -28,   -17,    61,    59,   123,
    58,    44,   -23,   -26,    41,    41,    44,    41,    41,    41,
    41,    41,    41,    41,    41,    41,    41,    41,    44,    44,
   263,    -2,   -18,   -19,   -24,    61,   -18,   -25,    45,   -33,
   263,   -15,   -25,    59,   125,    59,   -25,    41,    41,   -33,
    46,    41,    59,   263 };
yytabelem yydef[]={

    98,    -2,     0,    98,   102,   103,   104,   105,   106,   107,
   108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
   118,   119,   120,   121,   122,   123,    98,    98,     0,    56,
    98,    57,    58,    59,    60,    61,    62,    63,    64,    65,
    98,     0,    99,     0,     0,    55,     0,     1,     2,   124,
   125,   126,    66,     3,     0,     0,    67,     0,    71,    72,
    73,     0,    -2,     0,     5,     0,     8,     9,     0,    12,
     0,     0,     0,     0,    15,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    46,     0,     0,   100,    49,
    -2,    -2,     0,     4,     6,     7,    10,     0,    11,    16,
     0,    42,    22,    23,    40,    13,     0,     0,    18,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,    45,    47,
     0,    28,     0,   100,    50,    51,    52,     0,    39,    41,
    42,   100,    14,    17,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,    48,     0,    29,    -2,    33,    36,    20,
   101,    53,    54,    70,    43,     0,     0,     0,    81,    82,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,    95,     0,     0,     0,     0,    34,    42,    42,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,    19,     0,    24,    98,
   100,     0,    37,    44,    79,    80,   100,    84,    85,    86,
    87,    88,    89,    90,    91,    92,    93,    94,     0,     0,
     0,     0,     0,    32,    35,     0,     0,     0,     0,    75,
    76,    78,     0,    21,    26,    27,    38,    83,    96,    74,
     0,    97,    25,    77 };
typedef struct
#ifdef __cplusplus
	yytoktype
#endif
{ char *t_name; int t_val; } yytoktype;
#ifndef YYDEBUG
#	define YYDEBUG	0	/* don't allow debugging */
#endif

#if YYDEBUG

yytoktype yytoks[] =
{
	"CLASS",	257,
	"PUBLIC",	258,
	"PRIVATE",	259,
	"PROTECTED",	260,
	"VIRTUAL",	261,
	"STRING",	262,
	"NUM",	263,
	"ID",	264,
	"INT",	265,
	"FLOAT",	266,
	"SHORT",	267,
	"LONG",	268,
	"DOUBLE",	269,
	"VOID",	270,
	"CHAR",	271,
	"CLASS_REF",	272,
	"OTHER",	273,
	"CONST",	274,
	"OPERATOR",	275,
	"UNSIGNED",	276,
	"FRIEND",	277,
	"VTK_ID",	278,
	"STATIC",	279,
	"VAR_FUNCTION",	280,
	"ARRAY_NUM",	281,
	"SetMacro",	282,
	"GetMacro",	283,
	"SetStringMacro",	284,
	"GetStringMacro",	285,
	"SetClampMacro",	286,
	"SetObjectMacro",	287,
	"SetReferenceCountedObjectMacro",	288,
	"GetObjectMacro",	289,
	"BooleanMacro",	290,
	"SetVector2Macro",	291,
	"SetVector3Macro",	292,
	"SetVector4Macro",	293,
	"GetVector2Macro",	294,
	"GetVector3Macro",	295,
	"GetVector4Macro",	296,
	"SetVectorMacro",	297,
	"GetVectorMacro",	298,
	"ImageSetMacro",	299,
	"ImageSetExtentMacro",	300,
	"-unknown-",	-1	/* ends search */
};

char * yyreds[] =
{
	"-no such reduction-",
	"strt : maybe_other class_def maybe_other",
	"class_def : CLASS VTK_ID",
	"class_def : CLASS VTK_ID optional_scope",
	"class_def : CLASS VTK_ID optional_scope '{' class_def_body '}'",
	"class_def_body : class_def_item",
	"class_def_body : class_def_item class_def_body",
	"class_def_item : scope_type ':'",
	"class_def_item : var",
	"class_def_item : function",
	"class_def_item : FRIEND function",
	"class_def_item : macro ';'",
	"class_def_item : macro",
	"function : '~' func",
	"function : VIRTUAL '~' func",
	"function : func",
	"function : type func",
	"function : VIRTUAL type func",
	"function : VIRTUAL func",
	"func : any_id '(' args_list ')' func_end",
	"func : OPERATOR maybe_other_no_semi ';'",
	"func : any_id '(' args_list ')' '=' NUM ';'",
	"any_id : VTK_ID",
	"any_id : ID",
	"func_end : ';'",
	"func_end : '{' maybe_other '}' ';'",
	"func_end : '{' maybe_other '}'",
	"func_end : ':' maybe_other_no_semi ';'",
	"args_list : /* empty */",
	"args_list : more_args",
	"more_args : arg",
	"more_args : arg",
	"more_args : arg ',' more_args",
	"arg : type",
	"arg : type var_id",
	"arg : type var_id opt_var_assign",
	"arg : VAR_FUNCTION",
	"opt_var_assign : /* empty */",
	"opt_var_assign : '=' float_num",
	"var : type var_id ';'",
	"var : VAR_FUNCTION ';'",
	"var_id : any_id var_array",
	"var_array : /* empty */",
	"var_array : ARRAY_NUM var_array",
	"var_array : '[' maybe_other_no_semi ']' var_array",
	"type : CONST type_red1",
	"type : type_red1",
	"type : STATIC type_red1",
	"type : STATIC CONST type_red1",
	"type_red1 : type_red2",
	"type_red1 : type_red2 type_indirection",
	"type_indirection : '&'",
	"type_indirection : '*'",
	"type_indirection : '&' type_indirection",
	"type_indirection : '*' type_indirection",
	"type_red2 : UNSIGNED type_primitive",
	"type_red2 : type_primitive",
	"type_primitive : FLOAT",
	"type_primitive : VOID",
	"type_primitive : CHAR",
	"type_primitive : INT",
	"type_primitive : SHORT",
	"type_primitive : LONG",
	"type_primitive : DOUBLE",
	"type_primitive : ID",
	"type_primitive : VTK_ID",
	"optional_scope : /* empty */",
	"optional_scope : ':' scope_list",
	"scope_list : scope_type VTK_ID",
	"scope_list : scope_type VTK_ID",
	"scope_list : scope_type VTK_ID ',' scope_list",
	"scope_type : PUBLIC",
	"scope_type : PRIVATE",
	"scope_type : PROTECTED",
	"float_num : '-' float_prim",
	"float_num : float_prim",
	"float_prim : NUM",
	"float_prim : NUM '.' NUM",
	"float_prim : any_id",
	"macro : SetMacro '(' any_id ',' type_red2 ')'",
	"macro : GetMacro '(' any_id ',' type_red2 ')'",
	"macro : SetStringMacro '(' any_id ')'",
	"macro : GetStringMacro '(' any_id ')'",
	"macro : SetClampMacro '(' any_id ',' type_red2 ',' maybe_other_no_semi ')'",
	"macro : SetObjectMacro '(' any_id ',' type_red2 ')'",
	"macro : SetReferenceCountedObjectMacro '(' any_id ',' type_red2 ')'",
	"macro : GetObjectMacro '(' any_id ',' type_red2 ')'",
	"macro : BooleanMacro '(' any_id ',' type_red2 ')'",
	"macro : SetVector2Macro '(' any_id ',' type_red2 ')'",
	"macro : GetVector2Macro '(' any_id ',' type_red2 ')'",
	"macro : SetVector3Macro '(' any_id ',' type_red2 ')'",
	"macro : GetVector3Macro '(' any_id ',' type_red2 ')'",
	"macro : SetVector4Macro '(' any_id ',' type_red2 ')'",
	"macro : GetVector4Macro '(' any_id ',' type_red2 ')'",
	"macro : ImageSetMacro '(' any_id ',' type_red2 ')'",
	"macro : ImageSetExtentMacro '(' any_id ')'",
	"macro : SetVectorMacro '(' any_id ',' type_red2 ',' float_num ')'",
	"macro : GetVectorMacro '(' any_id ',' type_red2 ',' float_num ')'",
	"maybe_other : /* empty */",
	"maybe_other : other_stuff maybe_other",
	"maybe_other_no_semi : /* empty */",
	"maybe_other_no_semi : other_stuff_no_semi maybe_other_no_semi",
	"other_stuff : ';'",
	"other_stuff : other_stuff_no_semi",
	"other_stuff_no_semi : OTHER",
	"other_stuff_no_semi : braces",
	"other_stuff_no_semi : parens",
	"other_stuff_no_semi : '*'",
	"other_stuff_no_semi : '='",
	"other_stuff_no_semi : ':'",
	"other_stuff_no_semi : ','",
	"other_stuff_no_semi : '.'",
	"other_stuff_no_semi : STRING",
	"other_stuff_no_semi : type_red2",
	"other_stuff_no_semi : NUM",
	"other_stuff_no_semi : CLASS_REF",
	"other_stuff_no_semi : '&'",
	"other_stuff_no_semi : brackets",
	"other_stuff_no_semi : CONST",
	"other_stuff_no_semi : OPERATOR",
	"other_stuff_no_semi : '-'",
	"other_stuff_no_semi : '~'",
	"other_stuff_no_semi : STATIC",
	"other_stuff_no_semi : ARRAY_NUM",
	"braces : '{' maybe_other '}'",
	"parens : '(' maybe_other ')'",
	"brackets : '[' maybe_other ']'",
};
#endif /* YYDEBUG */
# line	1 "/usr/ccs/bin/yaccpar"
/*
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)yaccpar	6.12	93/06/07 SMI"

/*
** Skeleton parser driver for yacc output
*/

/*
** yacc user known macros and defines
*/
#define YYERROR		goto yyerrlab
#define YYACCEPT	return(0)
#define YYABORT		return(1)
#define YYBACKUP( newtoken, newvalue )\
{\
	if ( yychar >= 0 || ( yyr2[ yytmp ] >> 1 ) != 1 )\
	{\
		yyerror( "syntax error - cannot backup" );\
		goto yyerrlab;\
	}\
	yychar = newtoken;\
	yystate = *yyps;\
	yylval = newvalue;\
	goto yynewstate;\
}
#define YYRECOVERING()	(!!yyerrflag)
#define YYNEW(type)	malloc(sizeof(type) * yynewmax)
#define YYCOPY(to, from, type) \
	(type *) memcpy(to, (char *) from, yynewmax * sizeof(type))
#define YYENLARGE( from, type) \
	(type *) realloc((char *) from, yynewmax * sizeof(type))
#ifndef YYDEBUG
#	define YYDEBUG	1	/* make debugging available */
#endif

/*
** user known globals
*/
int yydebug;			/* set to 1 to get debugging */

/*
** driver internal defines
*/
#define YYFLAG		(-10000000)

/*
** global variables used by the parser
*/
YYSTYPE *yypv;			/* top of value stack */
int *yyps;			/* top of state stack */

int yystate;			/* current state */
int yytmp;			/* extra var (lasts between blocks) */

int yynerrs;			/* number of errors */
int yyerrflag;			/* error recovery flag */
int yychar;			/* current input token number */



#ifdef YYNMBCHARS
#define YYLEX()		yycvtok(yylex())
/*
** yycvtok - return a token if i is a wchar_t value that exceeds 255.
**	If i<255, i itself is the token.  If i>255 but the neither 
**	of the 30th or 31st bit is on, i is already a token.
*/
#if defined(__STDC__) || defined(__cplusplus)
int yycvtok(int i)
#else
int yycvtok(i) int i;
#endif
{
	int first = 0;
	int last = YYNMBCHARS - 1;
	int mid;
	wchar_t j;

	if(i&0x60000000){/*Must convert to a token. */
		if( yymbchars[last].character < i ){
			return i;/*Giving up*/
		}
		while ((last>=first)&&(first>=0)) {/*Binary search loop*/
			mid = (first+last)/2;
			j = yymbchars[mid].character;
			if( j==i ){/*Found*/ 
				return yymbchars[mid].tvalue;
			}else if( j<i ){
				first = mid + 1;
			}else{
				last = mid -1;
			}
		}
		/*No entry in the table.*/
		return i;/* Giving up.*/
	}else{/* i is already a token. */
		return i;
	}
}
#else/*!YYNMBCHARS*/
#define YYLEX()		yylex()
#endif/*!YYNMBCHARS*/

/*
** yyparse - return 0 if worked, 1 if syntax error not recovered from
*/
#if defined(__STDC__) || defined(__cplusplus)
int yyparse(void)
#else
int yyparse()
#endif
{
	register YYSTYPE *yypvt;	/* top of value stack for $vars */

#if defined(__cplusplus) || defined(lint)
/*
	hacks to please C++ and lint - goto's inside switch should never be
	executed; yypvt is set to 0 to avoid "used before set" warning.
*/
	static int __yaccpar_lint_hack__ = 0;
	switch (__yaccpar_lint_hack__)
	{
		case 1: goto yyerrlab;
		case 2: goto yynewstate;
	}
	yypvt = 0;
#endif

	/*
	** Initialize externals - yyparse may be called more than once
	*/
	yypv = &yyv[-1];
	yyps = &yys[-1];
	yystate = 0;
	yytmp = 0;
	yynerrs = 0;
	yyerrflag = 0;
	yychar = -1;

#if YYMAXDEPTH <= 0
	if (yymaxdepth <= 0)
	{
		if ((yymaxdepth = YYEXPAND(0)) <= 0)
		{
			yyerror("yacc initialization error");
			YYABORT;
		}
	}
#endif

	{
		register YYSTYPE *yy_pv;	/* top of value stack */
		register int *yy_ps;		/* top of state stack */
		register int yy_state;		/* current state */
		register int  yy_n;		/* internal state number info */
	goto yystack;	/* moved from 6 lines above to here to please C++ */

		/*
		** get globals into registers.
		** branch to here only if YYBACKUP was called.
		*/
	yynewstate:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;
		goto yy_newstate;

		/*
		** get globals into registers.
		** either we just started, or we just finished a reduction
		*/
	yystack:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;

		/*
		** top of for (;;) loop while no reductions done
		*/
	yy_stack:
		/*
		** put a state and value onto the stacks
		*/
#if YYDEBUG
		/*
		** if debugging, look up token value in list of value vs.
		** name pairs.  0 and negative (-1) are special values.
		** Note: linear search is used since time is not a real
		** consideration while debugging.
		*/
		if ( yydebug )
		{
			register int yy_i;

			printf( "State %d, token ", yy_state );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ++yy_ps >= &yys[ yymaxdepth ] )	/* room on stack? */
		{
			/*
			** reallocate and recover.  Note that pointers
			** have to be reset, or bad things will happen
			*/
			int yyps_index = (yy_ps - yys);
			int yypv_index = (yy_pv - yyv);
			int yypvt_index = (yypvt - yyv);
			int yynewmax;
#ifdef YYEXPAND
			yynewmax = YYEXPAND(yymaxdepth);
#else
			yynewmax = 2 * yymaxdepth;	/* double table size */
			if (yymaxdepth == YYMAXDEPTH)	/* first time growth */
			{
				char *newyys = (char *)YYNEW(int);
				char *newyyv = (char *)YYNEW(YYSTYPE);
				if (newyys != 0 && newyyv != 0)
				{
					yys = YYCOPY(newyys, yys, int);
					yyv = YYCOPY(newyyv, yyv, YYSTYPE);
				}
				else
					yynewmax = 0;	/* failed */
			}
			else				/* not first time */
			{
				yys = YYENLARGE(yys, int);
				yyv = YYENLARGE(yyv, YYSTYPE);
				if (yys == 0 || yyv == 0)
					yynewmax = 0;	/* failed */
			}
#endif
			if (yynewmax <= yymaxdepth)	/* tables not expanded */
			{
				yyerror( "yacc stack overflow" );
				YYABORT;
			}
			yymaxdepth = yynewmax;

			yy_ps = yys + yyps_index;
			yy_pv = yyv + yypv_index;
			yypvt = yyv + yypvt_index;
		}
		*yy_ps = yy_state;
		*++yy_pv = yyval;

		/*
		** we have a new state - find out what to do
		*/
	yy_newstate:
		if ( ( yy_n = yypact[ yy_state ] ) <= YYFLAG )
			goto yydefault;		/* simple state */
#if YYDEBUG
		/*
		** if debugging, need to mark whether new token grabbed
		*/
		yytmp = yychar < 0;
#endif
		if ( ( yychar < 0 ) && ( ( yychar = YYLEX() ) < 0 ) )
			yychar = 0;		/* reached EOF */
#if YYDEBUG
		if ( yydebug && yytmp )
		{
			register int yy_i;

			printf( "Received token " );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ( ( yy_n += yychar ) < 0 ) || ( yy_n >= YYLAST ) )
			goto yydefault;
		if ( yychk[ yy_n = yyact[ yy_n ] ] == yychar )	/*valid shift*/
		{
			yychar = -1;
			yyval = yylval;
			yy_state = yy_n;
			if ( yyerrflag > 0 )
				yyerrflag--;
			goto yy_stack;
		}

	yydefault:
		if ( ( yy_n = yydef[ yy_state ] ) == -2 )
		{
#if YYDEBUG
			yytmp = yychar < 0;
#endif
			if ( ( yychar < 0 ) && ( ( yychar = YYLEX() ) < 0 ) )
				yychar = 0;		/* reached EOF */
#if YYDEBUG
			if ( yydebug && yytmp )
			{
				register int yy_i;

				printf( "Received token " );
				if ( yychar == 0 )
					printf( "end-of-file\n" );
				else if ( yychar < 0 )
					printf( "-none-\n" );
				else
				{
					for ( yy_i = 0;
						yytoks[yy_i].t_val >= 0;
						yy_i++ )
					{
						if ( yytoks[yy_i].t_val
							== yychar )
						{
							break;
						}
					}
					printf( "%s\n", yytoks[yy_i].t_name );
				}
			}
#endif /* YYDEBUG */
			/*
			** look through exception table
			*/
			{
				register int *yyxi = yyexca;

				while ( ( *yyxi != -1 ) ||
					( yyxi[1] != yy_state ) )
				{
					yyxi += 2;
				}
				while ( ( *(yyxi += 2) >= 0 ) &&
					( *yyxi != yychar ) )
					;
				if ( ( yy_n = yyxi[1] ) < 0 )
					YYACCEPT;
			}
		}

		/*
		** check for syntax error
		*/
		if ( yy_n == 0 )	/* have an error */
		{
			/* no worry about speed here! */
			switch ( yyerrflag )
			{
			case 0:		/* new error */
				yyerror( "syntax error" );
				goto skip_init;
			yyerrlab:
				/*
				** get globals into registers.
				** we have a user generated syntax type error
				*/
				yy_pv = yypv;
				yy_ps = yyps;
				yy_state = yystate;
			skip_init:
				yynerrs++;
				/* FALLTHRU */
			case 1:
			case 2:		/* incompletely recovered error */
					/* try again... */
				yyerrflag = 3;
				/*
				** find state where "error" is a legal
				** shift action
				*/
				while ( yy_ps >= yys )
				{
					yy_n = yypact[ *yy_ps ] + YYERRCODE;
					if ( yy_n >= 0 && yy_n < YYLAST &&
						yychk[yyact[yy_n]] == YYERRCODE)					{
						/*
						** simulate shift of "error"
						*/
						yy_state = yyact[ yy_n ];
						goto yy_stack;
					}
					/*
					** current state has no shift on
					** "error", pop stack
					*/
#if YYDEBUG
#	define _POP_ "Error recovery pops state %d, uncovers state %d\n"
					if ( yydebug )
						printf( _POP_, *yy_ps,
							yy_ps[-1] );
#	undef _POP_
#endif
					yy_ps--;
					yy_pv--;
				}
				/*
				** there is no state on stack with "error" as
				** a valid shift.  give up.
				*/
				YYABORT;
			case 3:		/* no shift yet; eat a token */
#if YYDEBUG
				/*
				** if debugging, look up token in list of
				** pairs.  0 and negative shouldn't occur,
				** but since timing doesn't matter when
				** debugging, it doesn't hurt to leave the
				** tests here.
				*/
				if ( yydebug )
				{
					register int yy_i;

					printf( "Error recovery discards " );
					if ( yychar == 0 )
						printf( "token end-of-file\n" );
					else if ( yychar < 0 )
						printf( "token -none-\n" );
					else
					{
						for ( yy_i = 0;
							yytoks[yy_i].t_val >= 0;
							yy_i++ )
						{
							if ( yytoks[yy_i].t_val
								== yychar )
							{
								break;
							}
						}
						printf( "token %s\n",
							yytoks[yy_i].t_name );
					}
				}
#endif /* YYDEBUG */
				if ( yychar == 0 )	/* reached EOF. quit */
					YYABORT;
				yychar = -1;
				goto yy_newstate;
			}
		}/* end if ( yy_n == 0 ) */
		/*
		** reduction by production yy_n
		** put stack tops, etc. so things right after switch
		*/
#if YYDEBUG
		/*
		** if debugging, print the string that is the user's
		** specification of the reduction which is just about
		** to be done.
		*/
		if ( yydebug )
			printf( "Reduce by (%d) \"%s\"\n",
				yy_n, yyreds[ yy_n ] );
#endif
		yytmp = yy_n;			/* value to switch over */
		yypvt = yy_pv;			/* $vars top of value stack */
		/*
		** Look in goto table for next state
		** Sorry about using yy_state here as temporary
		** register variable, but why not, if it works...
		** If yyr2[ yy_n ] doesn't have the low order bit
		** set, then there is no action to be done for
		** this reduction.  So, no saving & unsaving of
		** registers done.  The only difference between the
		** code just after the if and the body of the if is
		** the goto yy_stack in the body.  This way the test
		** can be made before the choice of what to do is needed.
		*/
		{
			/* length of production doubled with extra bit */
			register int yy_len = yyr2[ yy_n ];

			if ( !( yy_len & 01 ) )
			{
				yy_len >>= 1;
				yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
				yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
					*( yy_ps -= yy_len ) + 1;
				if ( yy_state >= YYLAST ||
					yychk[ yy_state =
					yyact[ yy_state ] ] != -yy_n )
				{
					yy_state = yyact[ yypgo[ yy_n ] ];
				}
				goto yy_stack;
			}
			yy_len >>= 1;
			yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
			yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
				*( yy_ps -= yy_len ) + 1;
			if ( yy_state >= YYLAST ||
				yychk[ yy_state = yyact[ yy_state ] ] != -yy_n )
			{
				yy_state = yyact[ yypgo[ yy_n ] ];
			}
		}
					/* save until reenter driver code */
		yystate = yy_state;
		yyps = yy_ps;
		yypv = yy_pv;
	}
	/*
	** code supplied by user is placed in this switch
	*/
	switch( yytmp )
	{
		
case 2:
# line 89 "cpp_parse.y"
{
      class_name = strdup(yypvt[-0].str);
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
	fprintf(yyout,"  %s *temp = %s::New();\n",class_name,class_name);
	fprintf(yyout,"  return ((ClientData)temp);\n}\n\n");
	}
      } break;
case 3:
# line 108 "cpp_parse.y"
{
      int i;
      for (i = 0; i < num_superclasses; i++)
	{
	fprintf(yyout,"int %sCppCommand(%s *op, Tcl_Interp *interp,\n             int argc, char *argv[]);\n",superclasses[i],superclasses[i]);
	}
      fprintf(yyout,"int %sCppCommand(%s *op, Tcl_Interp *interp,\n             int argc, char *argv[]);\n",class_name,class_name);
      fprintf(yyout,"\nint %sCommand(ClientData cd, Tcl_Interp *interp,\n             int argc, char *argv[])\n{\n",class_name);
      fprintf(yyout,"  if ((argc == 2)&&(!strcmp(\"Delete\",argv[1]))&& !vtkTclInDelete())\n    {\n");
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

      fprintf(yyout,"  if (argc < 2)\n    {\n    sprintf(interp->result,\"Could not find requested method.\");\n    return TCL_ERROR;\n    }\n");

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

      } break;
case 4:
# line 151 "cpp_parse.y"
{
      int i;

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
      fprintf(yyout,"    char temps2[256];\n    sprintf(temps2,\"Object named: %%s, could not find requested method: %%s\\nor the method was called with incorrect arguments.\\n\",argv[0],argv[1]);\n    Tcl_AppendResult(interp,temps2,NULL);\n    }\n");
      fprintf(yyout,"  return TCL_ERROR;\n}\n");
      } break;
case 9:
# line 216 "cpp_parse.y"
{ arg_failure = 0; num_args = 0; arg_types[10] = 2; arg_ids[10] = NULL;} break;
case 10:
# line 218 "cpp_parse.y"
{ arg_failure = 0; num_args = 0; arg_types[10] = 2; arg_ids[10] = NULL;} break;
case 11:
# line 220 "cpp_parse.y"
{ arg_failure = 0; num_args = 0; arg_types[10] = 2; arg_ids[10] = NULL;} break;
case 12:
# line 222 "cpp_parse.y"
{ arg_failure = 0; num_args = 0; arg_types[10] = 2; arg_ids[10] = NULL;} break;
case 15:
# line 226 "cpp_parse.y"
{
         output_function();
	 } break;
case 16:
# line 230 "cpp_parse.y"
{
         arg_types[10] = yypvt[-1].integer;
         output_function();
	 } break;
case 17:
# line 235 "cpp_parse.y"
{
         arg_types[10] = yypvt[-1].integer;
         output_function();
	 } break;
case 18:
# line 240 "cpp_parse.y"
{
         output_function();
	 } break;
case 19:
# line 245 "cpp_parse.y"
{ is_virtual = 0; func_name = yypvt[-4].str; 
       fprintf(stderr,"   Converted func %s\n",yypvt[-4].str); } break;
case 20:
# line 248 "cpp_parse.y"
{ is_virtual = 1; fprintf(stderr,"   Converted operator\n"); } break;
case 21:
# line 250 "cpp_parse.y"
{ is_virtual = 0; func_name = yypvt[-6].str;
       fprintf(stderr,"   Converted func %s\n",yypvt[-6].str);} break;
case 30:
# line 262 "cpp_parse.y"
{ num_args++;} break;
case 31:
# line 262 "cpp_parse.y"
{num_args++;} break;
case 33:
# line 264 "cpp_parse.y"
{arg_types[num_args] = yypvt[-0].integer;} break;
case 34:
# line 265 "cpp_parse.y"
{arg_types[num_args] = yypvt[-1].integer; } break;
case 36:
# line 266 "cpp_parse.y"
{arg_types[num_args] = 5000;} break;
case 43:
# line 275 "cpp_parse.y"
{ arg_failure = 1; } break;
case 44:
# line 276 "cpp_parse.y"
{ arg_failure = 1; } break;
case 45:
# line 279 "cpp_parse.y"
{yyval.integer = 1000 + yypvt[-0].integer;} break;
case 46:
# line 280 "cpp_parse.y"
{yyval.integer = yypvt[-0].integer;} break;
case 47:
# line 281 "cpp_parse.y"
{yyval.integer = 2000 + yypvt[-0].integer;} break;
case 48:
# line 282 "cpp_parse.y"
{yyval.integer = 3000 + yypvt[-0].integer;} break;
case 49:
# line 284 "cpp_parse.y"
{yyval.integer = yypvt[-0].integer;} break;
case 50:
# line 286 "cpp_parse.y"
{yyval.integer = yypvt[-1].integer + yypvt[-0].integer;} break;
case 51:
# line 295 "cpp_parse.y"
{ yyval.integer = 100;} break;
case 52:
# line 296 "cpp_parse.y"
{ yyval.integer = 300;} break;
case 53:
# line 297 "cpp_parse.y"
{ yyval.integer = 100 + yypvt[-0].integer;} break;
case 54:
# line 298 "cpp_parse.y"
{ yyval.integer = 400 + yypvt[-0].integer;} break;
case 55:
# line 300 "cpp_parse.y"
{ yyval.integer = 10 + yypvt[-0].integer;} break;
case 56:
# line 301 "cpp_parse.y"
{ yyval.integer = yypvt[-0].integer;} break;
case 57:
# line 304 "cpp_parse.y"
{ yyval.integer = 1;} break;
case 58:
# line 305 "cpp_parse.y"
{ yyval.integer = 2;} break;
case 59:
# line 306 "cpp_parse.y"
{ yyval.integer = 3;} break;
case 60:
# line 307 "cpp_parse.y"
{ yyval.integer = 4;} break;
case 61:
# line 308 "cpp_parse.y"
{ yyval.integer = 5;} break;
case 62:
# line 309 "cpp_parse.y"
{ yyval.integer = 6;} break;
case 63:
# line 310 "cpp_parse.y"
{ yyval.integer = 7;} break;
case 64:
# line 311 "cpp_parse.y"
{ yyval.integer = 8;} break;
case 65:
# line 312 "cpp_parse.y"
{ yyval.integer = 9; 
           arg_ids[num_args] = strdup(yypvt[-0].str); 
           if ((!arg_ids[10])&&(!num_args))
             { 
             arg_ids[10] = arg_ids[0];
             }
         } break;
case 68:
# line 323 "cpp_parse.y"
{ superclasses[num_superclasses] = strdup(yypvt[-0].str); num_superclasses++; } break;
case 69:
# line 325 "cpp_parse.y"
{ superclasses[num_superclasses] = strdup(yypvt[-0].str); num_superclasses++; } break;
case 71:
# line 328 "cpp_parse.y"
{in_public = 1;} break;
case 72:
# line 328 "cpp_parse.y"
{in_public = 0;} break;
case 73:
# line 329 "cpp_parse.y"
{in_public = 0;} break;
case 76:
# line 333 "cpp_parse.y"
{yyval.integer = yypvt[-0].integer;} break;
case 77:
# line 334 "cpp_parse.y"
{yyval.integer = -1;} break;
case 78:
# line 334 "cpp_parse.y"
{yyval.integer = -1;} break;
case 79:
# line 338 "cpp_parse.y"
{ 
   is_virtual = 0;
   sprintf(temps,"Set%s",yypvt[-3].str); 
   func_name = strdup(temps);
   num_args = 1;
   arg_types[0] = yypvt[-1].integer;
   output_function();
   } break;
case 80:
# line 347 "cpp_parse.y"
{ 
   is_virtual = 0;
   sprintf(temps,"Get%s",yypvt[-3].str); 
   func_name = strdup(temps);
   num_args = 0;
   arg_types[10] = yypvt[-1].integer;
   output_function();
   } break;
case 81:
# line 356 "cpp_parse.y"
{ 
   is_virtual = 0;
   sprintf(temps,"Set%s",yypvt[-1].str); 
   func_name = strdup(temps);
   num_args = 1;
   arg_types[0] = 303;
   output_function();
   } break;
case 82:
# line 365 "cpp_parse.y"
{ 
   is_virtual = 0;
   sprintf(temps,"Get%s",yypvt[-1].str); 
   func_name = strdup(temps);
   num_args = 0;
   arg_types[10] = 303;
   output_function();
   } break;
case 83:
# line 374 "cpp_parse.y"
{ 
   is_virtual = 0;
   sprintf(temps,"Set%s",yypvt[-5].str); 
   func_name = strdup(temps);
   num_args = 1;
   arg_types[0] = yypvt[-3].integer;
   output_function();
   } break;
case 84:
# line 383 "cpp_parse.y"
{ 
   is_virtual = 0;
   sprintf(temps,"Set%s",yypvt[-3].str); 
   func_name = strdup(temps);
   num_args = 1;
   arg_types[0] = 309;
   output_function();
   } break;
case 85:
# line 392 "cpp_parse.y"
{ 
   is_virtual = 0;
   sprintf(temps,"Set%s",yypvt[-3].str); 
   func_name = strdup(temps);
   num_args = 1;
   arg_types[0] = 309;
   output_function();
   } break;
case 86:
# line 401 "cpp_parse.y"
{ 
   is_virtual = 0;
   sprintf(temps,"Get%s",yypvt[-3].str); 
   func_name = strdup(temps);
   num_args = 0;
   arg_types[10] = 309;
   output_function();
   } break;
case 87:
# line 410 "cpp_parse.y"
{ 
   is_virtual = 0;
   sprintf(temps,"%sOn",yypvt[-3].str); 
   func_name = strdup(temps);
   num_args = 0;
   output_function();
   free(func_name);
   sprintf(temps,"%sOff",yypvt[-3].str); 
   func_name = strdup(temps);
   num_args = 0;
   output_function();
   } break;
case 88:
# line 423 "cpp_parse.y"
{ 
   is_virtual = 0;
   sprintf(temps,"Set%s",yypvt[-3].str); 
   func_name = strdup(temps);
   num_args = 2;
   arg_types[0] = yypvt[-1].integer;
   arg_types[1] = yypvt[-1].integer;
   output_function();
   } break;
case 89:
# line 433 "cpp_parse.y"
{ 
   int i;

   is_virtual = 0;
   sprintf(temps,"Get%s",yypvt[-3].str); 

   /* check to see if we can handle the args */
   if ((yypvt[-1].integer != 2) &&
       ((yypvt[-1].integer < 8) ||
        (yypvt[-1].integer == 13) ||
        (yypvt[-1].integer == 14) ||
        (yypvt[-1].integer == 15))) 
      {
      fprintf(yyout,"  if ((!strcmp(\"%s\",argv[1]))&&(argc == 2))\n    {\n",
	      temps);

    /* process the args */
    switch (yypvt[-1].integer)
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
      switch (yypvt[-1].integer)
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
   } break;
case 90:
# line 508 "cpp_parse.y"
{ 
   is_virtual = 0;
   sprintf(temps,"Set%s",yypvt[-3].str); 
   func_name = strdup(temps);
   num_args = 3;
   arg_types[0] = yypvt[-1].integer;
   arg_types[1] = yypvt[-1].integer;
   arg_types[2] = yypvt[-1].integer;
   output_function();
   } break;
case 91:
# line 519 "cpp_parse.y"
{ 
   int i;

   is_virtual = 0;
   sprintf(temps,"Get%s",yypvt[-3].str); 

   /* check to see if we can handle the args */
   if ((yypvt[-1].integer != 2) &&
       ((yypvt[-1].integer < 8) ||
        (yypvt[-1].integer == 13) ||
        (yypvt[-1].integer == 14) ||
        (yypvt[-1].integer == 15))) 
      {
      fprintf(yyout,"  if ((!strcmp(\"%s\",argv[1]))&&(argc == 2))\n    {\n",
	      temps);

    /* process the args */
    switch (yypvt[-1].integer)
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
      switch (yypvt[-1].integer)
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
   } break;
case 92:
# line 594 "cpp_parse.y"
{ 
   is_virtual = 0;
   sprintf(temps,"Set%s",yypvt[-3].str); 
   func_name = strdup(temps);
   num_args = 4;
   arg_types[0] = yypvt[-1].integer;
   arg_types[1] = yypvt[-1].integer;
   arg_types[2] = yypvt[-1].integer;
   arg_types[3] = yypvt[-1].integer;
   output_function();
   } break;
case 93:
# line 606 "cpp_parse.y"
{ 
   int i;

   is_virtual = 0;
   sprintf(temps,"Get%s",yypvt[-3].str); 

   /* check to see if we can handle the args */
   if ((yypvt[-1].integer != 2) &&
       ((yypvt[-1].integer < 8) ||
        (yypvt[-1].integer == 13) ||
        (yypvt[-1].integer == 14) ||
        (yypvt[-1].integer == 15))) 
      {
      fprintf(yyout,"  if ((!strcmp(\"%s\",argv[1]))&&(argc == 2))\n    {\n",
	      temps);

    /* process the args */
    switch (yypvt[-1].integer)
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
      switch (yypvt[-1].integer)
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
   } break;
case 94:
# line 681 "cpp_parse.y"
{ 
   is_virtual = 0;
   sprintf(temps,"Set%s",yypvt[-3].str); 
   func_name = strdup(temps);
   num_args = 5;
   arg_types[0] = yypvt[-1].integer;
   arg_types[1] = yypvt[-1].integer;
   arg_types[2] = yypvt[-1].integer;
   arg_types[3] = yypvt[-1].integer;
   arg_types[4] = yypvt[-1].integer;
   output_function();
   free(func_name);
   sprintf(temps,"Set%s",yypvt[-3].str); 
   func_name = strdup(temps);
   num_args = 4;
   arg_types[0] = yypvt[-1].integer;
   arg_types[1] = yypvt[-1].integer;
   arg_types[2] = yypvt[-1].integer;
   arg_types[3] = yypvt[-1].integer;
   output_function();
   free(func_name);
   sprintf(temps,"Set%s",yypvt[-3].str); 
   func_name = strdup(temps);
   num_args = 3;
   arg_types[0] = yypvt[-1].integer;
   arg_types[1] = yypvt[-1].integer;
   arg_types[2] = yypvt[-1].integer;
   output_function();
   free(func_name);
   sprintf(temps,"Set%s",yypvt[-3].str); 
   func_name = strdup(temps);
   num_args = 2;
   arg_types[0] = yypvt[-1].integer;
   arg_types[1] = yypvt[-1].integer;
   output_function();
   free(func_name);
   sprintf(temps,"Set%s",yypvt[-3].str); 
   func_name = strdup(temps);
   num_args = 1;
   arg_types[0] = yypvt[-1].integer;
   output_function();
   free(func_name);
   } break;
case 95:
# line 725 "cpp_parse.y"
{ 
   is_virtual = 0;
   sprintf(temps,"Set%s",yypvt[-1].str); 
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
   sprintf(temps,"Set%s",yypvt[-1].str); 
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
   sprintf(temps,"Set%s",yypvt[-1].str); 
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
   sprintf(temps,"Set%s",yypvt[-1].str); 
   func_name = strdup(temps);
   num_args = 4;
   arg_types[0] = 4;
   arg_types[1] = 4;
   arg_types[2] = 4;
   arg_types[3] = 4;
   output_function();
   free(func_name);
   sprintf(temps,"Set%s",yypvt[-1].str); 
   func_name = strdup(temps);
   num_args = 2;
   arg_types[0] = 4;
   arg_types[1] = 4;
   output_function();
   free(func_name);
   } break;
case 96:
# line 784 "cpp_parse.y"
{ 
   int i;

   sprintf(temps,"Set%s",yypvt[-5].str); 
   is_virtual = 0;

  /* check to see if we can handle the args */
  if ((yypvt[-3].integer != 2)&&(yypvt[-3].integer < 8)&&(yypvt[-1].integer >= 0)) 
    {
    fprintf(yyout,"  if ((!strcmp(\"%s\",argv[1]))&&(argc == %i))\n    {\n",
	    temps,yypvt[-1].integer + 2);

    /* process the args */
    switch (yypvt[-3].integer)
      {
      case 1:   fprintf(yyout,"    float  "); break;
      case 7:   fprintf(yyout,"    double "); break;
      case 4:   fprintf(yyout,"    int    "); break;
      case 5:   fprintf(yyout,"    short  "); break;
      case 6:   fprintf(yyout,"    long   "); break;
      case 3:   fprintf(yyout,"    char   "); break;
      }
    fprintf(yyout,"temp[%i];\n\n",yypvt[-1].integer);
    fprintf(yyout,"    error = 0;\n\n");

    /* now get the required args from the stack */
    for (i = 0; i < yypvt[-1].integer; i++)
      {
      switch (yypvt[-3].integer)
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
    funcArgs[numFuncs] = yypvt[-1].integer;
    numFuncs++;
    }
   } break;
case 97:
# line 840 "cpp_parse.y"
{ 
   int i;

   is_virtual = 0;
   sprintf(temps,"Get%s",yypvt[-5].str); 

  /* check to see if we can handle the args */
  if ((yypvt[-3].integer != 2) &&
      ((yypvt[-3].integer < 8) ||
       (yypvt[-3].integer == 13) ||
       (yypvt[-3].integer == 14) ||
       (yypvt[-3].integer == 15)) &&
      (yypvt[-1].integer >= 0)) 
    {
    fprintf(yyout,"  if ((!strcmp(\"%s\",argv[1]))&&(argc == 2))\n    {\n",
	    temps);

    /* process the args */
    switch (yypvt[-3].integer)
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
    for (i = 0; i < yypvt[-1].integer; i++)
      {
      switch (yypvt[-3].integer)
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
   } break;
# line	532 "/usr/ccs/bin/yaccpar"
	}
	goto yystack;		/* reset registers in driver code */
}

