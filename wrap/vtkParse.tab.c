
# line 43 "vtkParse.y"
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

# line 60 "vtkParse.y"
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
# define SetVector6Macro 294
# define GetVector2Macro 295
# define GetVector3Macro 296
# define GetVector4Macro 297
# define GetVector6Macro 298
# define SetVectorMacro 299
# define GetVectorMacro 300
# define ViewportCoordinateMacro 301
# define WorldCoordinateMacro 302

#ifdef __STDC__
#include <stdlib.h>
#include <string.h>
#else
#include <malloc.h>
#include <memory.h>
#endif

#ifndef WIN32
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

# line 599 "vtkParse.y"

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
 


yytabelem yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 100,
	40, 21,
	-2, 64,
-1, 101,
	40, 22,
	-2, 63,
-1, 102,
	44, 68,
	-2, 67,
-1, 180,
	44, 30,
	-2, 29,
	};
# define YYNPROD 128
# define YYLAST 519
yytabelem yyact[]={

    70,    18,   143,    27,   102,     9,   113,    12,    22,    13,
   153,    48,    38,    34,    31,    35,    36,    37,    32,    33,
   112,    11,     4,    18,    10,    27,    39,     9,   268,    12,
    22,    13,   266,   269,   113,    38,    34,    31,    35,    36,
    37,    32,    33,    11,   282,    94,    10,    28,   112,    39,
    96,   182,   259,   153,    30,    41,   273,   267,    97,   179,
    58,    59,    60,    55,    70,    38,    34,    31,    35,    36,
    37,    32,    33,   113,    56,   142,    30,    28,   103,    39,
    49,   216,    51,   264,    98,   281,    26,   112,    65,    23,
    38,    34,    31,    35,    36,    37,    32,    33,   274,   272,
   183,   110,    28,   150,    39,   114,   108,   105,    26,    54,
   279,    23,   151,   258,    61,   257,   244,   239,   208,   207,
   206,   205,   204,   203,   202,   201,   200,   111,   199,   116,
   198,   197,    58,    59,    60,    71,   196,   146,   101,    34,
    31,    35,    36,    37,    32,    33,     5,   144,    94,    98,
    28,    66,   100,    96,    69,   106,    73,    74,    75,    76,
    77,    78,    79,    80,    81,    82,    84,    86,    88,    83,
    85,    87,    89,    90,    91,    92,    93,   104,   156,   157,
   158,   159,   160,   161,   162,   163,   164,   165,   166,   167,
   168,   169,   170,   171,   172,   173,   174,   175,   176,    71,
   152,   195,   101,    34,    31,    35,    36,    37,    32,    33,
   194,   191,    94,    98,    28,    95,   100,    96,   190,   187,
   147,   280,    68,   143,   148,    14,    16,    38,    34,    31,
    35,    36,    37,    32,    33,    17,     6,    20,    21,    28,
   214,    39,    24,   152,    25,   145,    15,    14,    16,    38,
    34,    31,    35,    36,    37,    32,    33,    17,     6,    20,
    21,    28,   215,    39,    24,   188,    25,   101,    34,    31,
    35,    36,    37,    32,    33,   269,   113,    94,    98,    28,
     2,   100,    96,   213,    42,   185,   186,   277,    72,   107,
   112,   271,   145,   184,   117,   276,   278,   275,    57,   262,
   145,   189,    99,   238,   236,   256,   235,    43,    44,    99,
   140,    46,   141,    99,   255,   254,   270,   270,    99,   253,
   252,    47,   251,   270,   250,   249,   270,   248,   247,   241,
   246,   245,   243,   242,   211,   210,   209,   193,   192,    50,
   139,    99,   138,    99,   137,   136,   135,   134,   133,   132,
   131,   130,   129,   128,    63,   127,   126,   109,   177,   115,
   118,    63,   125,   124,   123,   122,   181,   121,   237,   120,
   119,    29,    19,     8,     7,     3,   149,   263,   240,   212,
   180,   234,   178,    67,    64,   145,   261,    62,    53,    99,
    99,   145,   265,    52,    40,     1,   109,     0,     0,     0,
    45,     0,     0,     0,     0,   154,   155,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,   217,   218,     0,
     0,   219,   220,   221,   222,   223,   224,   225,   226,   227,
   228,   229,   230,   231,   232,   233,     0,     0,     0,     0,
     0,     0,   181,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,    99,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,   260 };
yytabelem yypact[]={

   -37,-10000000,  -202,   -37,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,
-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,
-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,   -37,   -37,  -252,-10000000,
   -37,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,
   -37,  -267,-10000000,   -45,   298,-10000000,   -11,-10000000,-10000000,-10000000,
-10000000,-10000000,    51,   -60,  -198,  -126,-10000000,  -274,-10000000,-10000000,
-10000000,   -47,  -126,    49,-10000000,-10000000,   -62,    47,  -191,    46,
  -191,     3,-10000000,   330,   329,   327,   325,   324,   323,   322,
   316,   315,   313,   312,   311,   310,   309,   308,   307,   306,
   305,   304,   302,   300,  -174,-10000000,  -199,   183,   -15,   182,
-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,  -191,-10000000,-10000000,
    44,   -38,-10000000,-10000000,-10000000,-10000000,  -191,  -191,-10000000,  -258,
  -258,  -258,  -258,  -258,  -258,  -258,  -258,  -258,  -258,  -258,
  -258,  -258,  -258,  -258,  -258,  -258,  -258,  -258,  -258,  -258,
-10000000,-10000000,  -174,  -229,    41,   -15,-10000000,   182,   182,   175,
-10000000,-10000000,   -81,   -15,-10000000,-10000000,   174,   167,   297,   296,
   166,   157,    92,    87,    86,    84,    82,    81,    80,    79,
    78,    77,    76,    75,    74,   295,   294,-10000000,   293,-10000000,
-10000000,  -258,-10000000,-10000000,-10000000,-10000000,-10000000,  -198,-10000000,   -12,
  -174,  -174,-10000000,-10000000,  -174,  -174,  -174,  -174,  -174,  -174,
  -174,  -174,  -174,  -174,  -174,  -174,  -174,  -174,  -174,-10000000,
-10000000,   245,    73,-10000000,   -81,-10000000,   -81,   292,   291,    72,
   290,   289,   287,   286,   284,   283,   281,   279,   278,   274,
   273,   264,    71,    69,-10000000,  -211,-10000000,   -37,   -15,  -229,
    22,-10000000,-10000000,-10000000,   -15,-10000000,-10000000,-10000000,-10000000,-10000000,
-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,    12,    12,    40,
   -69,    39,-10000000,-10000000,    12,   254,   246,  -230,-10000000,    64,
-10000000,   180,-10000000,    26,-10000000,-10000000,-10000000,-10000000,-10000000,  -219,
-10000000,-10000000,-10000000 };
yytabelem yypgo[]={

     0,   395,   280,   394,   393,   388,   114,   387,   298,   384,
    88,   383,   288,   222,    58,   382,   381,   147,    59,   380,
   379,   101,   378,   377,    32,   112,   215,   246,   137,   371,
    74,   376,    28,   375,   146,   374,   373,   372 };
yytabelem yyr1[]={

     0,     1,     4,     3,     6,     6,     7,     7,     7,     7,
     7,     7,    10,    10,    10,    10,    10,    10,    12,    12,
    12,    14,    14,    16,    16,    16,    16,    15,    15,    18,
    20,    18,    19,    22,    19,    19,    23,    23,     9,     9,
    21,    25,    25,    25,    13,    13,    13,    13,    26,    26,
    28,    28,    28,    28,    27,    27,    29,    29,    29,    29,
    29,    29,    29,    29,    29,     5,     5,    30,    31,    30,
     8,     8,     8,    24,    24,    32,    32,    32,    11,    11,
    11,    11,    11,    11,    11,    11,    11,    11,    11,    11,
    11,    11,    11,    11,    11,    11,    11,    11,    11,     2,
     2,    17,    17,    33,    33,    34,    34,    34,    34,    34,
    34,    34,    34,    34,    34,    34,    34,    34,    34,    34,
    34,    34,    34,    34,    34,    35,    36,    37 };
yytabelem yyr2[]={

     0,     6,     1,    14,     2,     4,     4,     2,     2,     4,
     4,     2,     5,     7,     3,     5,     7,     5,    11,     7,
    15,     2,     2,     2,     8,     6,     6,     0,     2,     3,
     1,     8,     3,     1,     8,     3,     0,     4,     6,     4,
     4,     0,     5,     9,     5,     3,     5,     7,     3,     5,
     3,     3,     5,     5,     5,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     0,     4,     5,     1,    10,
     3,     3,     3,     4,     2,     3,     7,     3,    13,    13,
     9,     9,    17,    13,    13,    13,    13,    13,    13,    13,
    13,    13,    13,    13,    13,    17,    17,     9,     9,     0,
     4,     0,     4,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     6,     6,     6 };
yytabelem yychk[]={

-10000000,    -1,    -2,   -33,    59,   -34,   273,   -35,   -36,    42,
    61,    58,    44,    46,   262,   -27,   263,   272,    38,   -37,
   274,   275,    45,   126,   279,   281,   123,    40,   276,   -29,
    91,   266,   270,   271,   265,   267,   268,   269,   264,   278,
    -3,   257,    -2,    -2,    -2,   -29,    -2,    -2,   278,   125,
    41,    93,    -4,    -5,    58,   123,   -30,    -8,   258,   259,
   260,    -6,    -7,    -8,    -9,   -10,   277,   -11,   -13,   280,
   126,   261,   -12,   282,   283,   284,   285,   286,   287,   288,
   289,   290,   291,   295,   292,   296,   293,   297,   294,   298,
   299,   300,   301,   302,   274,   -26,   279,   -14,   275,   -27,
   278,   264,   278,   125,    -6,    58,   -10,   -13,    59,   -12,
   -21,   -14,   278,   264,    59,   -12,   126,   -13,   -12,    40,
    40,    40,    40,    40,    40,    40,    40,    40,    40,    40,
    40,    40,    40,    40,    40,    40,    40,    40,    40,    40,
   -26,   -26,   274,    40,   -17,   -34,   -28,    38,    42,   -31,
    59,   -25,   281,    91,   -12,   -12,   -14,   -14,   -14,   -14,
   -14,   -14,   -14,   -14,   -14,   -14,   -14,   -14,   -14,   -14,
   -14,   -14,   -14,   -14,   -14,   -14,   -14,   -26,   -15,   -18,
   -19,   -13,   280,    59,   -17,   -28,   -28,    44,   -25,   -17,
    44,    44,    41,    41,    44,    44,    44,    44,    44,    44,
    44,    44,    44,    44,    44,    44,    44,    44,    44,    41,
    41,    41,   -20,   -21,   -14,   -30,    93,   -27,   -27,   -27,
   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,
   -27,   -27,   -27,   -27,   -16,    61,    59,   123,    58,    44,
   -22,   -25,    41,    41,    44,    41,    41,    41,    41,    41,
    41,    41,    41,    41,    41,    41,    41,    44,    44,   263,
    -2,   -17,   -18,   -23,    61,   -17,   -24,    45,   -32,   263,
   -14,   -24,    59,   125,    59,   -24,    41,    41,   -32,    46,
    41,    59,   263 };
yytabelem yydef[]={

    99,    -2,     0,    99,   103,   104,   105,   106,   107,   108,
   109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
   119,   120,   121,   122,   123,   124,    99,    99,     0,    55,
    99,    56,    57,    58,    59,    60,    61,    62,    63,    64,
    99,     0,   100,     0,     0,    54,     0,     1,     2,   125,
   126,   127,    65,     0,     0,     0,    66,     0,    70,    71,
    72,     0,     4,     0,     7,     8,     0,    11,     0,     0,
     0,     0,    14,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    45,     0,     0,   101,    48,
    -2,    -2,    -2,     3,     5,     6,     9,     0,    10,    15,
     0,    41,    21,    22,    39,    12,     0,     0,    17,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    44,    46,     0,    27,     0,   101,    49,    50,    51,     0,
    38,    40,    41,   101,    13,    16,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,    47,     0,    28,
    -2,    32,    35,    19,   102,    52,    53,     0,    42,     0,
     0,     0,    80,    81,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,    97,
    98,     0,     0,    33,    41,    69,    41,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,    18,     0,    23,    99,   101,     0,
    36,    43,    78,    79,   101,    83,    84,    85,    86,    87,
    88,    89,    90,    91,    92,    93,    94,     0,     0,     0,
     0,     0,    31,    34,     0,     0,     0,     0,    74,    75,
    77,     0,    20,    25,    26,    37,    82,    95,    73,     0,
    96,    24,    76 };
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
	"SetVector6Macro",	294,
	"GetVector2Macro",	295,
	"GetVector3Macro",	296,
	"GetVector4Macro",	297,
	"GetVector6Macro",	298,
	"SetVectorMacro",	299,
	"GetVectorMacro",	300,
	"ViewportCoordinateMacro",	301,
	"WorldCoordinateMacro",	302,
	"-unknown-",	-1	/* ends search */
};

char * yyreds[] =
{
	"-no such reduction-",
	"strt : maybe_other class_def maybe_other",
	"class_def : CLASS VTK_ID",
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
	"macro : SetVector6Macro '(' any_id ',' type_red2 ')'",
	"macro : GetVector6Macro '(' any_id ',' type_red2 ')'",
	"macro : SetVectorMacro '(' any_id ',' type_red2 ',' float_num ')'",
	"macro : GetVectorMacro '(' any_id ',' type_red2 ',' float_num ')'",
	"macro : ViewportCoordinateMacro '(' any_id ')'",
	"macro : WorldCoordinateMacro '(' any_id ')'",
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
# line 121 "vtkParse.y"
{
      data.ClassName = strdup(yypvt[-0].str);
      } break;
case 12:
# line 131 "vtkParse.y"
{ output_function(); } break;
case 13:
# line 132 "vtkParse.y"
{ output_function(); } break;
case 14:
# line 134 "vtkParse.y"
{
         output_function();
	 } break;
case 15:
# line 138 "vtkParse.y"
{
         currentFunction->ReturnType = yypvt[-1].integer;
         output_function();
	 } break;
case 16:
# line 143 "vtkParse.y"
{
         currentFunction->ReturnType = yypvt[-1].integer;
         output_function();
	 } break;
case 17:
# line 148 "vtkParse.y"
{
         output_function();
	 } break;
case 18:
# line 153 "vtkParse.y"
{
      currentFunction->Name = yypvt[-4].str; 
      fprintf(stderr,"   Parsed func %s\n",yypvt[-4].str); 
    } break;
case 19:
# line 158 "vtkParse.y"
{ 
      currentFunction->IsOperator = 1; 
      fprintf(stderr,"   Converted operator\n"); 
    } break;
case 20:
# line 163 "vtkParse.y"
{ 
      currentFunction->Name = yypvt[-6].str; 
      fprintf(stderr,"   Parsed func %s\n",yypvt[-6].str); 
      currentFunction->IsPureVirtual = 1; 
      data.IsAbstract = 1;
    } break;
case 29:
# line 179 "vtkParse.y"
{ currentFunction->NumberOfArguments++;} break;
case 30:
# line 180 "vtkParse.y"
{ currentFunction->NumberOfArguments++;} break;
case 32:
# line 183 "vtkParse.y"
{
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
	yypvt[-0].integer;} break;
case 33:
# line 188 "vtkParse.y"
{
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
	yypvt[-1].integer;
    } break;
case 35:
# line 194 "vtkParse.y"
{ 
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 5000;
    } break;
case 42:
# line 206 "vtkParse.y"
{ currentFunction->ArrayFailure = 1; } break;
case 43:
# line 208 "vtkParse.y"
{ currentFunction->ArrayFailure = 1; } break;
case 44:
# line 211 "vtkParse.y"
{yyval.integer = 1000 + yypvt[-0].integer;} break;
case 45:
# line 212 "vtkParse.y"
{yyval.integer = yypvt[-0].integer;} break;
case 46:
# line 213 "vtkParse.y"
{yyval.integer = 2000 + yypvt[-0].integer;} break;
case 47:
# line 214 "vtkParse.y"
{yyval.integer = 3000 + yypvt[-0].integer;} break;
case 48:
# line 216 "vtkParse.y"
{yyval.integer = yypvt[-0].integer;} break;
case 49:
# line 218 "vtkParse.y"
{yyval.integer = yypvt[-1].integer + yypvt[-0].integer;} break;
case 50:
# line 227 "vtkParse.y"
{ yyval.integer = 100;} break;
case 51:
# line 228 "vtkParse.y"
{ yyval.integer = 300;} break;
case 52:
# line 229 "vtkParse.y"
{ yyval.integer = 100 + yypvt[-0].integer;} break;
case 53:
# line 230 "vtkParse.y"
{ yyval.integer = 400 + yypvt[-0].integer;} break;
case 54:
# line 232 "vtkParse.y"
{ yyval.integer = 10 + yypvt[-0].integer;} break;
case 55:
# line 233 "vtkParse.y"
{ yyval.integer = yypvt[-0].integer;} break;
case 56:
# line 236 "vtkParse.y"
{ yyval.integer = 1;} break;
case 57:
# line 237 "vtkParse.y"
{ yyval.integer = 2;} break;
case 58:
# line 238 "vtkParse.y"
{ yyval.integer = 3;} break;
case 59:
# line 239 "vtkParse.y"
{ yyval.integer = 4;} break;
case 60:
# line 240 "vtkParse.y"
{ yyval.integer = 5;} break;
case 61:
# line 241 "vtkParse.y"
{ yyval.integer = 6;} break;
case 62:
# line 242 "vtkParse.y"
{ yyval.integer = 7;} break;
case 63:
# line 243 "vtkParse.y"
{ yyval.integer = 8;} break;
case 64:
# line 245 "vtkParse.y"
{ 
      yyval.integer = 9; 
      currentFunction->ArgClasses[currentFunction->NumberOfArguments] =
        strdup(yypvt[-0].str); 
      /* store the string into the return value just in case we need it */
      /* this is a parsing hack because the first "type" parser will */
      /* possibly be ht ereturn type of the first argument */
      if ((!currentFunction->ReturnClass) && 
          (!currentFunction->NumberOfArguments)) 
        { 
        currentFunction->ReturnClass = strdup(yypvt[-0].str); 
        }
    } break;
case 67:
# line 262 "vtkParse.y"
{ 
      data.SuperClasses[data.NumberOfSuperClasses] = strdup(yypvt[-0].str); 
      data.NumberOfSuperClasses++; 
    } break;
case 68:
# line 267 "vtkParse.y"
{ 
      data.SuperClasses[data.NumberOfSuperClasses] = strdup(yypvt[-0].str); 
      data.NumberOfSuperClasses++; 
    } break;
case 70:
# line 272 "vtkParse.y"
{in_public = 1;} break;
case 71:
# line 272 "vtkParse.y"
{in_public = 0;} break;
case 72:
# line 273 "vtkParse.y"
{in_public = 0;} break;
case 75:
# line 277 "vtkParse.y"
{yyval.integer = yypvt[-0].integer;} break;
case 76:
# line 278 "vtkParse.y"
{yyval.integer = -1;} break;
case 77:
# line 278 "vtkParse.y"
{yyval.integer = -1;} break;
case 78:
# line 282 "vtkParse.y"
{
   sprintf(temps,"Set%s",yypvt[-3].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = yypvt[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
   output_function();
   } break;
case 79:
# line 292 "vtkParse.y"
{ 
   sprintf(temps,"Get%s",yypvt[-3].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = yypvt[-1].integer;
   output_function();
   } break;
case 80:
# line 300 "vtkParse.y"
{ 
   sprintf(temps,"Set%s",yypvt[-1].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 303;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
   output_function();
   } break;
case 81:
# line 310 "vtkParse.y"
{ 
   sprintf(temps,"Get%s",yypvt[-1].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 303;
   output_function();
   } break;
case 82:
# line 318 "vtkParse.y"
{ 
   sprintf(temps,"Set%s",yypvt[-5].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = yypvt[-3].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
   output_function();
   } break;
case 83:
# line 328 "vtkParse.y"
{ 
   sprintf(temps,"Set%s",yypvt[-3].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 2;
   output_function();
   } break;
case 84:
# line 338 "vtkParse.y"
{ 
   sprintf(temps,"Set%s",yypvt[-3].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 2;
   output_function();
   } break;
case 85:
# line 348 "vtkParse.y"
{ 
   sprintf(temps,"Get%s",yypvt[-3].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 309;
   output_function();
   } break;
case 86:
# line 356 "vtkParse.y"
{ 
   sprintf(temps,"%sOn",yypvt[-3].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 2;
   output_function();
   sprintf(temps,"%sOff",yypvt[-3].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   output_function();
   } break;
case 87:
# line 368 "vtkParse.y"
{ 
   sprintf(temps,"Set%s",yypvt[-3].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 2;
   currentFunction->ArgTypes[0] = yypvt[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = yypvt[-1].integer;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yypvt[-1].integer;
   currentFunction->ArgCounts[0] = 2;
   output_function();
   } break;
case 88:
# line 386 "vtkParse.y"
{ 
   sprintf(temps,"Get%s",yypvt[-3].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yypvt[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 2;
   output_function();
   } break;
case 89:
# line 396 "vtkParse.y"
{ 
   sprintf(temps,"Set%s",yypvt[-3].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 3;
   currentFunction->ArgTypes[0] = yypvt[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = yypvt[-1].integer;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = yypvt[-1].integer;
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yypvt[-1].integer;
   currentFunction->ArgCounts[0] = 3;
   output_function();
   } break;
case 90:
# line 416 "vtkParse.y"
{ 
   sprintf(temps,"Get%s",yypvt[-3].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yypvt[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 3;
   output_function();
   } break;
case 91:
# line 426 "vtkParse.y"
{ 
   sprintf(temps,"Set%s",yypvt[-3].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 4;
   currentFunction->ArgTypes[0] = yypvt[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = yypvt[-1].integer;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = yypvt[-1].integer;
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ArgTypes[3] = yypvt[-1].integer;
   currentFunction->ArgCounts[3] = 0;
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yypvt[-1].integer;
   currentFunction->ArgCounts[0] = 4;
   output_function();
   } break;
case 92:
# line 448 "vtkParse.y"
{ 
   sprintf(temps,"Get%s",yypvt[-3].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yypvt[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 4;
   output_function();
   } break;
case 93:
# line 458 "vtkParse.y"
{ 
   sprintf(temps,"Set%s",yypvt[-3].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 6;
   currentFunction->ArgTypes[0] = yypvt[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = yypvt[-1].integer;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = yypvt[-1].integer;
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ArgTypes[3] = yypvt[-1].integer;
   currentFunction->ArgCounts[3] = 0;
   currentFunction->ArgTypes[4] = yypvt[-1].integer;
   currentFunction->ArgCounts[4] = 0;
   currentFunction->ArgTypes[5] = yypvt[-1].integer;
   currentFunction->ArgCounts[5] = 0;
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yypvt[-1].integer;
   currentFunction->ArgCounts[0] = 6;
   output_function();
   } break;
case 94:
# line 484 "vtkParse.y"
{ 
   sprintf(temps,"Get%s",yypvt[-3].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yypvt[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 6;
   output_function();
   } break;
case 95:
# line 494 "vtkParse.y"
{
     sprintf(temps,"Set%s",yypvt[-5].str); 
     currentFunction->Name = strdup(temps);
     currentFunction->ReturnType = 2;
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 300 + yypvt[-3].integer;
     currentFunction->ArgCounts[0] = yypvt[-1].integer;
     output_function();
   } break;
case 96:
# line 504 "vtkParse.y"
{ 
   sprintf(temps,"Get%s",yypvt[-5].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yypvt[-3].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = yypvt[-1].integer;
   output_function();
   } break;
case 97:
# line 514 "vtkParse.y"
{ 
     sprintf(temps,"Get%sCoordinate",yypvt[-1].str); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 309;
     currentFunction->ReturnClass = strdup("vtkCoordinate");
     output_function();

     sprintf(temps,"Set%s",yypvt[-1].str); 
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
     
     sprintf(temps,"Get%s",yypvt[-1].str); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 301;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 2;
     output_function();
   } break;
case 98:
# line 547 "vtkParse.y"
{ 
     sprintf(temps,"Get%sCoordinate",yypvt[-1].str); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 309;
     currentFunction->ReturnClass = strdup("vtkCoordinate");
     output_function();

     sprintf(temps,"Set%s",yypvt[-1].str); 
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
     
     sprintf(temps,"Get%s",yypvt[-1].str); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 301;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 3;
     output_function();
   } break;
# line	532 "/usr/ccs/bin/yaccpar"
	}
	goto yystack;		/* reset registers in driver code */
}

