
# line 44 "vtkParse.y"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define yyerror(a) fprintf(stderr,"%s\n",a)
#define yywrap() 1

#include "vtkParse.h"
    
  FileInfo data;
  static FunctionInfo *currentFunction;

  FILE *fhint;
  char temps[2048];
  int  in_public;
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
      currentFunction->Signature = malloc(2048);
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
      currentFunction->Signature = malloc(2048);
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

# line 120 "vtkParse.y"
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
# define TypeMacro 303

#include <malloc.h>
#include <memory.h>
#include <unistd.h>
#include <values.h>

#ifdef __cplusplus
extern "C" {
#endif
extern char *gettxt(const char *, const char *);
#if !defined(yylex) && !defined(__my_yylex)
	extern int yylex(void);
#endif

#ifdef __cplusplus
}
#endif

#if (defined(__cplusplus) || defined(_XOPEN_SOURCE)) && !defined(yyerror) && !defined(__my_yyerror)
	void yyerror(const char *);
#endif
int yyparse(void);
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

# line 842 "vtkParse.y"

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
 


yytabelem yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 103,
	40, 24,
	-2, 72,
-1, 104,
	40, 25,
	-2, 71,
-1, 105,
	44, 76,
	-2, 75,
-1, 228,
	44, 36,
	-2, 35,
	};
# define YYNPROD 159
# define YYLAST 571
yytabelem yyact[]={

    71,   310,   308,   227,   105,    18,    48,    27,    69,     9,
   116,    12,    22,    13,   321,    41,    38,    34,    31,    35,
    36,    37,    32,    33,   115,    11,     4,    18,    10,    27,
    39,     9,   159,    12,    22,    13,   190,   311,   116,    38,
    34,    31,    35,    36,    37,    32,    33,    11,   154,   146,
    10,    28,   115,    39,     5,   102,   116,   149,    30,    38,
    34,    31,    35,    36,    37,    32,    33,   100,   157,    96,
   115,    28,    57,    39,    98,   230,   110,   309,   113,   280,
    30,   120,    38,    34,    31,    35,    36,    37,    32,    33,
    26,   255,    56,    23,    28,   106,    39,    49,   233,   159,
    59,    60,    61,    52,   304,   148,   301,   281,   256,   225,
   191,    66,    26,   318,   156,    23,   117,   221,   111,   108,
   189,   186,    55,   306,   151,   114,   300,    97,   299,   282,
   242,   241,    59,    60,    61,    72,   235,    62,   104,    34,
    31,    35,    36,    37,    32,    33,    58,   220,    96,   100,
    28,    67,   103,    98,    70,   150,    74,    75,    76,    77,
    78,    79,    80,    81,    82,    83,    85,    87,    89,    84,
    86,    88,    90,    91,    92,    93,    94,    95,   162,   109,
    71,   222,   166,   167,   168,   188,   170,   171,   172,   173,
   174,   175,   176,   177,   178,   179,   180,   181,   182,   183,
   119,   107,   217,    64,   229,   150,   216,   215,   192,   214,
    64,   213,   212,   211,   150,    15,   210,   198,   209,   200,
   201,   202,   158,   208,   144,   206,   145,    29,   205,    14,
    16,    38,    34,    31,    35,    36,    37,    32,    33,    17,
     6,    20,    21,    28,   150,    39,    24,   224,    25,   204,
   203,    14,    16,    38,    34,    31,    35,    36,    37,    32,
    33,    17,     6,    20,    21,    28,   232,    39,    24,   231,
    25,   199,   101,    51,   184,   196,   253,   193,   194,   101,
   152,   320,   319,   101,   153,   260,   302,   316,   101,   158,
   307,   229,   305,   298,   297,   311,   116,    73,   296,   295,
   294,   293,   261,   313,   292,   291,   290,   314,   259,   288,
   115,   317,   101,   287,   101,    72,   284,   279,   104,    34,
    31,    35,    36,    37,    32,    33,   257,   237,    96,   100,
    28,   236,   103,    98,   219,   218,   187,    50,   104,    34,
    31,    35,    36,    37,    32,    33,   154,     2,    96,   100,
    28,    42,   103,    98,   143,   312,   312,   142,   141,   140,
   312,   150,   101,   139,   315,   312,   138,   112,   137,   118,
   121,   136,   135,   134,    43,    44,   133,   132,    46,   131,
   130,   129,   128,   127,   126,   125,   124,   123,    47,   122,
    19,     8,     7,     3,   252,   251,   250,   249,   248,   247,
   246,   245,   244,   243,   207,   267,   169,   240,   112,   239,
   286,   101,   238,   165,   164,   263,   163,   160,   161,   234,
   155,    45,   197,   303,   283,   258,   228,   226,   195,   185,
   147,    99,    68,    65,    63,    54,    53,    40,     1,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
   262,     0,     0,     0,   264,   265,   266,     0,   268,   269,
   270,   271,   272,   273,   274,   275,   276,   277,   278,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,   285,
     0,     0,     0,   289,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,   101,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,   223,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
   254 };
yytabelem yypact[]={

   -33,-10000000,  -242,   -33,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,
-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,
-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,   -33,   -33,-10000000,-10000000,
   -33,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,
   -33,  -272,-10000000,   -28,   296,  -248,    10,-10000000,-10000000,-10000000,
-10000000,-10000000,-10000000,    64,   -31,  -158,  -126,-10000000,  -274,-10000000,
-10000000,-10000000,   -30,  -126,    61,-10000000,-10000000,    54,    59,  -208,
    57,  -208,    74,-10000000,   349,   347,   346,   345,   344,   343,
   342,   341,   340,   339,   337,   336,   333,   332,   331,   328,
   326,   323,   319,   318,   317,   314,  -182,-10000000,  -225,    44,
   -11,   242,   306,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,
  -208,-10000000,-10000000,    55,     8,-10000000,-10000000,-10000000,-10000000,  -208,
  -208,-10000000,  -254,-10000000,-10000000,-10000000,  -254,  -254,  -254,-10000000,
  -254,  -254,  -254,  -254,  -254,  -254,  -254,  -254,  -254,  -254,
  -254,  -254,  -254,  -254,-10000000,-10000000,  -182,    62,  -227,    51,
   -11,-10000000,   242,   242,-10000000,   231,-10000000,-10000000,-10000000,   -11,
-10000000,-10000000,   227,  -254,  -254,  -254,   206,   205,   184,  -254,
-10000000,   179,   174,   172,   169,   168,   167,   165,   163,   162,
   158,   294,   293,   103,-10000000,-10000000,-10000000,    58,   -33,   -11,
    50,-10000000,-10000000,-10000000,-10000000,  -205,  -158,   -59,     5,-10000000,
    92,   290,   286,-10000000,-10000000,-10000000,    87,    86,-10000000,-10000000,
-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,
  -254,-10000000,   -33,   -34,    49,-10000000,   285,-10000000,-10000000,  -254,
-10000000,-10000000,-10000000,   -59,  -182,-10000000,-10000000,-10000000,  -182,  -182,
  -182,-10000000,  -182,  -182,  -182,  -182,  -182,  -182,  -182,  -182,
  -182,  -182,  -182,   276,   -46,    48,-10000000,-10000000,    85,-10000000,
   -59,-10000000,   275,  -182,-10000000,   272,   268,  -182,   265,   264,
   263,   260,   259,   258,   257,   253,   252,    84,    82,-10000000,
    47,-10000000,  -205,    43,-10000000,   251,    79,-10000000,-10000000,   249,
-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,    32,
    32,-10000000,-10000000,-10000000,    32,-10000000,   -11,-10000000,   246,  -226,
-10000000,    67,-10000000,   241,-10000000,   240,-10000000,-10000000,  -249,-10000000,
-10000000,-10000000 };
yytabelem yypgo[]={

     0,   438,   347,   437,   436,   435,   137,   434,   146,   433,
   111,   432,   297,     8,   431,   430,   429,    57,    55,   428,
   427,     3,   426,   425,    78,   424,   423,     2,    68,   422,
   127,   215,   124,   421,   227,    72,   420,     1,   419,   416,
   415,   414,   413,   412,   410,   409,   407,   406,   405,   404,
   403,   402,   401,   400,   399,   398,   397,   396,   395,   394,
   393,    54,   392,   391,   390 };
yytabelem yyr1[]={

     0,     1,     4,     3,     6,     6,     7,     7,     7,     7,
     7,     7,    10,    10,    10,    10,    10,    10,    15,    12,
    12,    12,    19,    14,    18,    18,    16,    16,    16,    16,
    16,    16,    16,    20,    20,    21,    23,    21,    22,    25,
    22,    22,    26,    26,     9,     9,    24,    28,    29,    28,
    28,    13,    13,    13,    13,    30,    30,    32,    32,    32,
    32,    33,    31,    31,    34,    34,    34,    34,    34,    34,
    34,    34,    34,     5,     5,    35,    36,    35,     8,     8,
     8,    27,    27,    37,    37,    37,    38,    11,    39,    40,
    11,    41,    11,    42,    11,    43,    44,    11,    45,    11,
    46,    11,    47,    48,    11,    49,    11,    50,    11,    51,
    11,    52,    11,    53,    11,    54,    11,    55,    11,    56,
    11,    57,    11,    58,    11,    59,    11,    11,    11,    11,
     2,     2,    17,    17,    60,    60,    61,    61,    61,    61,
    61,    61,    61,    61,    61,    61,    61,    61,    61,    61,
    61,    61,    61,    61,    61,    61,    62,    63,    64 };
yytabelem yyr2[]={

     0,     6,     1,    14,     2,     4,     4,     2,     2,     4,
     4,     2,     5,     7,     3,     5,     7,     5,     1,     7,
     7,     9,     1,    10,     3,     3,     2,     4,    10,     8,
     8,     6,     6,     0,     2,     3,     1,     8,     3,     1,
     8,     3,     0,     4,     6,     4,     4,     0,     1,     7,
     9,     5,     3,     5,     7,     3,     5,     3,     3,     5,
     5,     1,     7,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     0,     4,     5,     1,    10,     3,     3,
     3,     4,     2,     3,     7,     3,     1,    15,     1,     1,
    17,     1,    11,     1,    11,     1,     1,    21,     1,    15,
     1,    15,     1,     1,    17,     1,    15,     1,    15,     1,
    15,     1,    15,     1,    15,     1,    15,     1,    15,     1,
    15,     1,    15,     1,    19,     1,    19,     9,     9,    13,
     0,     4,     0,     4,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     6,     6,     6 };
yytabelem yychk[]={

-10000000,    -1,    -2,   -60,    59,   -61,   273,   -62,   -63,    42,
    61,    58,    44,    46,   262,   -31,   263,   272,    38,   -64,
   274,   275,    45,   126,   279,   281,   123,    40,   276,   -34,
    91,   266,   270,   271,   265,   267,   268,   269,   264,   278,
    -3,   257,    -2,    -2,    -2,   -33,    -2,    -2,   278,   125,
    41,   -34,    93,    -4,    -5,    58,   123,   -35,    -8,   258,
   259,   260,    -6,    -7,    -8,    -9,   -10,   277,   -11,   -13,
   280,   126,   261,   -12,   282,   283,   284,   285,   286,   287,
   288,   289,   290,   291,   295,   292,   296,   293,   297,   294,
   298,   299,   300,   301,   302,   303,   274,   -30,   279,   -14,
   275,   -31,   -18,   278,   264,   278,   125,    -6,    58,   -10,
   -13,    59,   -12,   -24,   -18,   278,   264,    59,   -12,   126,
   -13,   -12,    40,    40,    40,    40,    40,    40,    40,    40,
    40,    40,    40,    40,    40,    40,    40,    40,    40,    40,
    40,    40,    40,    40,   -30,   -30,   274,   -15,    61,   -17,
   -61,   -32,    38,    42,    40,   -36,    59,   -28,   281,    91,
   -12,   -12,   -18,   -39,   -41,   -42,   -18,   -18,   -18,   -47,
   -18,   -18,   -18,   -18,   -18,   -18,   -18,   -18,   -18,   -18,
   -18,   -18,   -18,   -18,   -30,   -16,    59,   274,   123,    58,
   263,    59,   -17,   -32,   -32,   -19,    44,   -29,   -17,    44,
   -18,   -18,   -18,    44,    44,    44,   -18,   -49,    44,    44,
    44,    44,    44,    44,    44,    44,    44,    44,    41,    41,
    44,    59,   123,    -2,   -17,    59,   -20,   -21,   -22,   -13,
   280,   -35,   -28,    93,   -38,    44,    41,    41,   -43,   -45,
   -46,    44,    44,   -50,   -51,   -52,   -53,   -54,   -55,   -56,
   -57,   -58,   -59,   -18,    -2,   125,    59,    41,   -23,   -24,
   -18,   -28,   -31,   -40,   -31,   -31,   -31,   -48,   -31,   -31,
   -31,   -31,   -31,   -31,   -31,   -31,   -31,   -31,   -31,    41,
   125,    59,    44,   -25,    41,   -31,   -44,    41,    41,   -31,
    41,    41,    41,    41,    41,    41,    41,    41,    41,    44,
    44,    59,   -21,   -26,    61,    41,    44,    41,   -27,    45,
   -37,   263,   -18,   -27,   -27,   -17,    41,   -37,    46,    41,
    41,   263 };
yytabelem yydef[]={

   130,    -2,     0,   130,   134,   135,   136,   137,   138,   139,
   140,   141,   142,   143,   144,   145,   146,   147,   148,   149,
   150,   151,   152,   153,   154,   155,   130,   130,    61,    63,
   130,    64,    65,    66,    67,    68,    69,    70,    71,    72,
   130,     0,   131,     0,     0,     0,     0,     1,     2,   156,
   157,    62,   158,    73,     0,     0,     0,    74,     0,    78,
    79,    80,     0,     4,     0,     7,     8,     0,    11,     0,
     0,     0,     0,    14,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,    52,     0,    18,
   132,    55,     0,    -2,    -2,    -2,     3,     5,     6,     9,
     0,    10,    15,     0,    47,    24,    25,    45,    12,     0,
     0,    17,     0,    88,    91,    93,     0,     0,     0,   102,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,    51,    53,     0,     0,     0,     0,
   132,    56,    57,    58,    22,     0,    44,    46,    48,   132,
    13,    16,     0,     0,     0,     0,     0,     0,     0,     0,
   105,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,    54,    19,    26,     0,   130,   132,
     0,    20,   133,    59,    60,    33,     0,    47,     0,    86,
     0,     0,     0,    95,    98,   100,     0,     0,   107,   109,
   111,   113,   115,   117,   119,   121,   123,   125,   127,   128,
     0,    27,   130,     0,     0,    21,     0,    34,    -2,    38,
    41,    77,    49,    47,     0,    89,    92,    94,     0,     0,
     0,   103,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    31,    32,    23,     0,    39,
    47,    50,     0,     0,    96,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,   129,
    30,    29,     0,    42,    87,     0,     0,    99,   101,     0,
   106,   108,   110,   112,   114,   116,   118,   120,   122,     0,
     0,    28,    37,    40,     0,    90,   132,   104,     0,     0,
    82,    83,    85,     0,    43,     0,   124,    81,     0,   126,
    97,    84 };
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
	"TypeMacro",	303,
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
	"func : func_beg",
	"func : func_beg func_end",
	"func : OPERATOR maybe_other_no_semi ';'",
	"func : func_beg '=' NUM ';'",
	"func_beg : any_id '('",
	"func_beg : any_id '(' args_list ')'",
	"any_id : VTK_ID",
	"any_id : ID",
	"func_end : ';'",
	"func_end : CONST ';'",
	"func_end : CONST '{' maybe_other '}' ';'",
	"func_end : '{' maybe_other '}' ';'",
	"func_end : CONST '{' maybe_other '}'",
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
	"var_array : ARRAY_NUM",
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
	"type_red2 : UNSIGNED",
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
	"macro : SetMacro '(' any_id ','",
	"macro : SetMacro '(' any_id ',' type_red2 ')'",
	"macro : GetMacro '('",
	"macro : GetMacro '(' any_id ','",
	"macro : GetMacro '(' any_id ',' type_red2 ')'",
	"macro : SetStringMacro '('",
	"macro : SetStringMacro '(' any_id ')'",
	"macro : GetStringMacro '('",
	"macro : GetStringMacro '(' any_id ')'",
	"macro : SetClampMacro '(' any_id ','",
	"macro : SetClampMacro '(' any_id ',' type_red2",
	"macro : SetClampMacro '(' any_id ',' type_red2 ',' maybe_other_no_semi ')'",
	"macro : SetObjectMacro '(' any_id ','",
	"macro : SetObjectMacro '(' any_id ',' type_red2 ')'",
	"macro : SetReferenceCountedObjectMacro '(' any_id ','",
	"macro : SetReferenceCountedObjectMacro '(' any_id ',' type_red2 ')'",
	"macro : GetObjectMacro '('",
	"macro : GetObjectMacro '(' any_id ','",
	"macro : GetObjectMacro '(' any_id ',' type_red2 ')'",
	"macro : BooleanMacro '(' any_id",
	"macro : BooleanMacro '(' any_id ',' type_red2 ')'",
	"macro : SetVector2Macro '(' any_id ','",
	"macro : SetVector2Macro '(' any_id ',' type_red2 ')'",
	"macro : GetVector2Macro '(' any_id ','",
	"macro : GetVector2Macro '(' any_id ',' type_red2 ')'",
	"macro : SetVector3Macro '(' any_id ','",
	"macro : SetVector3Macro '(' any_id ',' type_red2 ')'",
	"macro : GetVector3Macro '(' any_id ','",
	"macro : GetVector3Macro '(' any_id ',' type_red2 ')'",
	"macro : SetVector4Macro '(' any_id ','",
	"macro : SetVector4Macro '(' any_id ',' type_red2 ')'",
	"macro : GetVector4Macro '(' any_id ','",
	"macro : GetVector4Macro '(' any_id ',' type_red2 ')'",
	"macro : SetVector6Macro '(' any_id ','",
	"macro : SetVector6Macro '(' any_id ',' type_red2 ')'",
	"macro : GetVector6Macro '(' any_id ','",
	"macro : GetVector6Macro '(' any_id ',' type_red2 ')'",
	"macro : SetVectorMacro '(' any_id ','",
	"macro : SetVectorMacro '(' any_id ',' type_red2 ',' float_num ')'",
	"macro : GetVectorMacro '(' any_id ','",
	"macro : GetVectorMacro '(' any_id ',' type_red2 ',' float_num ')'",
	"macro : ViewportCoordinateMacro '(' any_id ')'",
	"macro : WorldCoordinateMacro '(' any_id ')'",
	"macro : TypeMacro '(' any_id ',' any_id ')'",
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
/* 
 *	Copyright 1987 Silicon Graphics, Inc. - All Rights Reserved
 */

/* #ident	"@(#)yacc:yaccpar	1.10" */
#ident	"$Revision$"

/*
** Skeleton parser driver for yacc output
*/
#include "stddef.h"

/*
** yacc user known macros and defines
*/
#define YYERROR		goto yyerrlab
#define YYACCEPT	return(0)
#define YYABORT		return(1)
#ifdef __cplusplus
#define YYBACKUP( newtoken, newvalue )\
{\
	if ( yychar >= 0 || ( yyr2[ yytmp ] >> 1 ) != 1 )\
	{\
		yyerror( gettxt("uxlibc:78", "syntax error - cannot backup") );\
		goto yyerrlab;\
	}\
	yychar = newtoken;\
	yystate = *yyps;\
	yylval = newvalue;\
	goto yynewstate;\
}
#else
#define YYBACKUP( newtoken, newvalue )\
{\
	if ( yychar >= 0 || ( yyr2[ yytmp ] >> 1 ) != 1 )\
	{\
		yyerror( gettxt("uxlibc:78", "Syntax error - cannot backup") );\
		goto yyerrlab;\
	}\
	yychar = newtoken;\
	yystate = *yyps;\
	yylval = newvalue;\
	goto yynewstate;\
}
#endif
#define YYRECOVERING()	(!!yyerrflag)
#define YYNEW(type)	malloc((size_t)(sizeof(type) * yynewmax))
#define YYCOPY(to, from, type) \
	(type *) memcpy(to, (char *) from, (size_t)(yynewmax * sizeof(type)))
#define YYENLARGE( from, type) \
	(type *) realloc((char *) from, (size_t)(yynewmax * sizeof(type)))
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
#ifdef __cplusplus
			yyerror(gettxt("uxlibc:79", "yacc initialization error"));
#else
			yyerror(gettxt("uxlibc:79", "Yacc initialization error"));
#endif
			YYABORT;
		}
	}
#endif

	goto yystack;
	{
		register YYSTYPE *yy_pv;	/* top of value stack */
		register int *yy_ps;		/* top of state stack */
		register int yy_state;		/* current state */
		register int  yy_n;		/* internal state number info */

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
			int yynewmax;
			ptrdiff_t yys_off;

			/* The following pointer-differences are safe, since
			 * yypvt, yy_pv, and yypv all are a multiple of
			 * sizeof(YYSTYPE) bytes from yyv.
			 */
			ptrdiff_t yypvt_off = yypvt - yyv;
			ptrdiff_t yy_pv_off = yy_pv - yyv;
			ptrdiff_t yypv_off = yypv - yyv;

			int *yys_base = yys;
#ifdef YYEXPAND
			yynewmax = YYEXPAND(yymaxdepth);
#else
			yynewmax = 2 * yymaxdepth;	/* double table size */
			if (yymaxdepth == YYMAXDEPTH)	/* first time growth */
			{
				void *newyys = YYNEW(int);
				void *newyyv = YYNEW(YYSTYPE);
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
#ifdef __cplusplus
				yyerror( gettxt("uxlibc:80", "yacc stack overflow") );
#else
				yyerror( gettxt("uxlibc:80", "Yacc stack overflow") );
#endif
				YYABORT;
			}
			yymaxdepth = yynewmax;

			/* reset pointers into yys */
			yys_off = yys - yys_base;
			yy_ps = yy_ps + yys_off;
			yyps = yyps + yys_off;

			/* reset pointers into yyv */
			yypvt = yyv + yypvt_off;
			yy_pv = yyv + yy_pv_off;
			yypv = yyv + yypv_off;
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
		if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
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
			if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
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
#ifdef __cplusplus
				yyerror( gettxt("uxlibc:81", "syntax error") );
#else
				yyerror( gettxt("uxlibc:81", "Syntax error") );
#endif
				goto skip_init;
			yyerrlab:
				/*
				** get globals into registers.
				** we have a user generated syntax type error
				*/
				yy_pv = yypv;
				yy_ps = yyps;
				yy_state = yystate;
				yynerrs++;
				/* FALLTHRU */
			skip_init:
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
# line 182 "vtkParse.y"
{
      data.ClassName = strdup(yypvt[-0].str);
      } break;
case 12:
# line 192 "vtkParse.y"
{ preSig("~"); output_function(); } break;
case 13:
# line 193 "vtkParse.y"
{ preSig("virtual ~"); output_function(); } break;
case 14:
# line 195 "vtkParse.y"
{
         output_function();
	 } break;
case 15:
# line 199 "vtkParse.y"
{
         currentFunction->ReturnType = yypvt[-1].integer;
         output_function();
	 } break;
case 16:
# line 204 "vtkParse.y"
{
         preSig("virtual ");
         currentFunction->ReturnType = yypvt[-1].integer;
         output_function();
	 } break;
case 17:
# line 210 "vtkParse.y"
{
         preSig("virtual ");
         output_function();
	 } break;
case 18:
# line 215 "vtkParse.y"
{ postSig(");"); openSig = 0; } break;
case 19:
# line 216 "vtkParse.y"
{
      openSig = 1;
      currentFunction->Name = yypvt[-2].str; 
      fprintf(stderr,"   Parsed func %s\n",yypvt[-2].str); 
    } break;
case 20:
# line 222 "vtkParse.y"
{ 
      currentFunction->IsOperator = 1; 
      fprintf(stderr,"   Converted operator\n"); 
    } break;
case 21:
# line 227 "vtkParse.y"
{ 
      postSig(") = 0;"); 
      currentFunction->Name = yypvt[-3].str; 
      fprintf(stderr,"   Parsed func %s\n",yypvt[-3].str); 
      currentFunction->IsPureVirtual = 1; 
      data.IsAbstract = 1;
    } break;
case 22:
# line 235 "vtkParse.y"
{postSig(" ("); } break;
case 24:
# line 238 "vtkParse.y"
{postSig(yypvt[-0].str);} break;
case 25:
# line 238 "vtkParse.y"
{postSig(yypvt[-0].str);} break;
case 35:
# line 250 "vtkParse.y"
{ currentFunction->NumberOfArguments++;} break;
case 36:
# line 251 "vtkParse.y"
{ currentFunction->NumberOfArguments++; postSig(", ");} break;
case 38:
# line 254 "vtkParse.y"
{
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
	yypvt[-0].integer;} break;
case 39:
# line 259 "vtkParse.y"
{
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
	yypvt[-1].integer;
    } break;
case 41:
# line 265 "vtkParse.y"
{ 
      postSig("void (*func)(void *) ");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 5000;
    } break;
case 48:
# line 279 "vtkParse.y"
{char temp[100]; sprintf(temp,"[%i]",yypvt[-0].integer); postSig(temp);} break;
case 49:
# line 280 "vtkParse.y"
{ currentFunction->ArrayFailure = 1; } break;
case 50:
# line 282 "vtkParse.y"
{ postSig("[]"); currentFunction->ArrayFailure = 1; } break;
case 51:
# line 285 "vtkParse.y"
{yyval.integer = 1000 + yypvt[-0].integer;} break;
case 52:
# line 286 "vtkParse.y"
{yyval.integer = yypvt[-0].integer;} break;
case 53:
# line 287 "vtkParse.y"
{yyval.integer = 2000 + yypvt[-0].integer;} break;
case 54:
# line 288 "vtkParse.y"
{yyval.integer = 3000 + yypvt[-0].integer;} break;
case 55:
# line 290 "vtkParse.y"
{yyval.integer = yypvt[-0].integer;} break;
case 56:
# line 292 "vtkParse.y"
{yyval.integer = yypvt[-1].integer + yypvt[-0].integer;} break;
case 57:
# line 301 "vtkParse.y"
{ postSig("&"); yyval.integer = 100;} break;
case 58:
# line 302 "vtkParse.y"
{ postSig("*"); yyval.integer = 300;} break;
case 59:
# line 303 "vtkParse.y"
{ yyval.integer = 100 + yypvt[-0].integer;} break;
case 60:
# line 304 "vtkParse.y"
{ yyval.integer = 400 + yypvt[-0].integer;} break;
case 61:
# line 306 "vtkParse.y"
{postSig("unsigned ");} break;
case 62:
# line 307 "vtkParse.y"
{ yyval.integer = 10 + yypvt[-0].integer;} break;
case 63:
# line 308 "vtkParse.y"
{ yyval.integer = yypvt[-0].integer;} break;
case 64:
# line 311 "vtkParse.y"
{ postSig("float "); yyval.integer = 1;} break;
case 65:
# line 312 "vtkParse.y"
{ postSig("void "); yyval.integer = 2;} break;
case 66:
# line 313 "vtkParse.y"
{ postSig("char "); yyval.integer = 3;} break;
case 67:
# line 314 "vtkParse.y"
{ postSig("int "); yyval.integer = 4;} break;
case 68:
# line 315 "vtkParse.y"
{ postSig("short "); yyval.integer = 5;} break;
case 69:
# line 316 "vtkParse.y"
{ postSig("long "); yyval.integer = 6;} break;
case 70:
# line 317 "vtkParse.y"
{ postSig("double "); yyval.integer = 7;} break;
case 71:
# line 318 "vtkParse.y"
{       
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",yypvt[-0].str);
      postSig(ctmpid);
      yyval.integer = 8;} break;
case 72:
# line 324 "vtkParse.y"
{ 
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",yypvt[-0].str);
      postSig(ctmpid);
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
case 75:
# line 344 "vtkParse.y"
{ 
      data.SuperClasses[data.NumberOfSuperClasses] = strdup(yypvt[-0].str); 
      data.NumberOfSuperClasses++; 
    } break;
case 76:
# line 349 "vtkParse.y"
{ 
      data.SuperClasses[data.NumberOfSuperClasses] = strdup(yypvt[-0].str); 
      data.NumberOfSuperClasses++; 
    } break;
case 78:
# line 354 "vtkParse.y"
{in_public = 1;} break;
case 79:
# line 354 "vtkParse.y"
{in_public = 0;} break;
case 80:
# line 355 "vtkParse.y"
{in_public = 0;} break;
case 83:
# line 359 "vtkParse.y"
{yyval.integer = yypvt[-0].integer;} break;
case 84:
# line 360 "vtkParse.y"
{yyval.integer = -1;} break;
case 85:
# line 360 "vtkParse.y"
{yyval.integer = -1;} break;
case 86:
# line 364 "vtkParse.y"
{preSig("void Set"); postSig(" ("); } break;
case 87:
# line 365 "vtkParse.y"
{
   postSig(");");
   sprintf(temps,"Set%s",yypvt[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = yypvt[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
   output_function();
   } break;
case 88:
# line 375 "vtkParse.y"
{postSig("Get");} break;
case 89:
# line 375 "vtkParse.y"
{postSig(" ();"); invertSig = 1;} break;
case 90:
# line 377 "vtkParse.y"
{ 
   sprintf(temps,"Get%s",yypvt[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = yypvt[-1].integer;
   output_function();
   } break;
case 91:
# line 384 "vtkParse.y"
{preSig("void Set");} break;
case 92:
# line 385 "vtkParse.y"
{
   postSig(" (char *);"); 
   sprintf(temps,"Set%s",yypvt[-1].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 303;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
   output_function();
   } break;
case 93:
# line 395 "vtkParse.y"
{preSig("char *Get");} break;
case 94:
# line 396 "vtkParse.y"
{ 
   postSig(" ();");
   sprintf(temps,"Get%s",yypvt[-1].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 303;
   output_function();
   } break;
case 95:
# line 405 "vtkParse.y"
{preSig("void Set"); postSig(" ("); } break;
case 96:
# line 406 "vtkParse.y"
{postSig(");"); openSig = 0;} break;
case 97:
# line 407 "vtkParse.y"
{ 
   sprintf(temps,"Set%s",yypvt[-7].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = yypvt[-4].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
   output_function();
   } break;
case 98:
# line 417 "vtkParse.y"
{preSig("void Set"); postSig(" ("); } break;
case 99:
# line 418 "vtkParse.y"
{ 
   postSig("*);");
   sprintf(temps,"Set%s",yypvt[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 2;
   output_function();
   } break;
case 100:
# line 429 "vtkParse.y"
{preSig("void Set"); postSig(" ("); } break;
case 101:
# line 430 "vtkParse.y"
{ 
   postSig("*);");
   sprintf(temps,"Set%s",yypvt[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 2;
   output_function();
   } break;
case 102:
# line 440 "vtkParse.y"
{postSig("*Get");} break;
case 103:
# line 441 "vtkParse.y"
{postSig(" ();"); invertSig = 1;} break;
case 104:
# line 442 "vtkParse.y"
{ 
   sprintf(temps,"Get%s",yypvt[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 309;
   output_function();
   } break;
case 105:
# line 450 "vtkParse.y"
{preSig("void "); postSig("On ();"); openSig = 0; } break;
case 106:
# line 452 "vtkParse.y"
{ 
   sprintf(temps,"%sOn",yypvt[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 2;
   output_function();
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void %sOff ();",yypvt[-4].str); 
   sprintf(temps,"%sOff",yypvt[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   output_function();
   } break;
case 107:
# line 467 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     } break;
case 108:
# line 472 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s , %s);",yypvt[-4].str,
     local, local);
   sprintf(temps,"Set%s",yypvt[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 2;
   currentFunction->ArgTypes[0] = yypvt[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = yypvt[-1].integer;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[2]);",yypvt[-4].str,
     local);
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yypvt[-1].integer;
   currentFunction->ArgCounts[0] = 2;
   output_function();
   } break;
case 109:
# line 497 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     } break;
case 110:
# line 502 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yypvt[-4].str);
   sprintf(temps,"Get%s",yypvt[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yypvt[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 2;
   output_function();
   } break;
case 111:
# line 514 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     } break;
case 112:
# line 519 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s , %s, %s);",
     yypvt[-4].str, local, local, local);
   sprintf(temps,"Set%s",yypvt[-4].str); 
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

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[3]);",yypvt[-4].str,
     local);
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yypvt[-1].integer;
   currentFunction->ArgCounts[0] = 3;
   output_function();
   } break;
case 113:
# line 546 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     } break;
case 114:
# line 551 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yypvt[-4].str);
   sprintf(temps,"Get%s",yypvt[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yypvt[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 3;
   output_function();
   } break;
case 115:
# line 563 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     } break;
case 116:
# line 568 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s , %s, %s, %s);",
     yypvt[-4].str, local, local, local, local);
   sprintf(temps,"Set%s",yypvt[-4].str); 
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

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[4]);",yypvt[-4].str,
     local);
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yypvt[-1].integer;
   currentFunction->ArgCounts[0] = 4;
   output_function();
   } break;
case 117:
# line 597 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     } break;
case 118:
# line 602 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yypvt[-4].str);
   sprintf(temps,"Get%s",yypvt[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yypvt[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 4;
   output_function();
   } break;
case 119:
# line 614 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     } break;
case 120:
# line 619 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s , %s, %s, %s, %s, %s);",
     yypvt[-4].str, local, local, local, local, local, local);
   sprintf(temps,"Set%s",yypvt[-4].str); 
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

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[6]);",yypvt[-4].str,
     local);
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yypvt[-1].integer;
   currentFunction->ArgCounts[0] = 6;
   output_function();
   } break;
case 121:
# line 652 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     } break;
case 122:
# line 657 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yypvt[-4].str);
   sprintf(temps,"Get%s",yypvt[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yypvt[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 6;
   output_function();
   } break;
case 123:
# line 669 "vtkParse.y"
{
      free (currentFunction->Signature);
      currentFunction->Signature = NULL;
      } break;
case 124:
# line 674 "vtkParse.y"
{
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s [%i]);",yypvt[-6].str,
      local, yypvt[-1].integer);
     sprintf(temps,"Set%s",yypvt[-6].str); 
     currentFunction->Name = strdup(temps);
     currentFunction->ReturnType = 2;
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 300 + yypvt[-3].integer;
     currentFunction->ArgCounts[0] = yypvt[-1].integer;
     output_function();
   } break;
case 125:
# line 687 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     } break;
case 126:
# line 692 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yypvt[-6].str);
   sprintf(temps,"Get%s",yypvt[-6].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yypvt[-3].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = yypvt[-1].integer;
   output_function();
   } break;
case 127:
# line 704 "vtkParse.y"
{ 
     char *local = strdup(currentFunction->Signature);
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       yypvt[-1].str);

     sprintf(temps,"Get%sCoordinate",yypvt[-1].str); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 309;
     currentFunction->ReturnClass = strdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (float , float);",
       yypvt[-1].str);
     sprintf(temps,"Set%s",yypvt[-1].str); 
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
       yypvt[-1].str);
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 301;
     currentFunction->ArgCounts[0] = 2;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"float *Get%s ();", yypvt[-1].str);
     sprintf(temps,"Get%s",yypvt[-1].str); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 301;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 2;
     output_function();
   } break;
case 128:
# line 752 "vtkParse.y"
{ 
     char *local = strdup(currentFunction->Signature);
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       yypvt[-1].str);

     sprintf(temps,"Get%sCoordinate",yypvt[-1].str); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 309;
     currentFunction->ReturnClass = strdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (float , float, float);",
       yypvt[-1].str);
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

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (float a[3]);",
       yypvt[-1].str);
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 301;
     currentFunction->ArgCounts[0] = 3;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"float *Get%s ();", yypvt[-1].str);
     sprintf(temps,"Get%s",yypvt[-1].str); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 301;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 3;
     output_function();
   } break;
case 129:
# line 802 "vtkParse.y"
{ 
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "const char *GetClassName();");
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
   } break;
	}
	goto yystack;		/* reset registers in driver code */
}
