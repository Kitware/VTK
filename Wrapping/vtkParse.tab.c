
/*  A Bison parser, made from vtkParse.y with Bison version GNU Bison version 1.22
  */

#define YYBISON 1  /* Identify Bison output.  */

#define	CLASS	258
#define	PUBLIC	259
#define	PRIVATE	260
#define	PROTECTED	261
#define	VIRTUAL	262
#define	STRING	263
#define	NUM	264
#define	ID	265
#define	INT	266
#define	FLOAT	267
#define	SHORT	268
#define	LONG	269
#define	DOUBLE	270
#define	VOID	271
#define	CHAR	272
#define	CLASS_REF	273
#define	OTHER	274
#define	CONST	275
#define	OPERATOR	276
#define	UNSIGNED	277
#define	FRIEND	278
#define	VTK_ID	279
#define	STATIC	280
#define	VAR_FUNCTION	281
#define	ARRAY_NUM	282
#define	SetMacro	283
#define	GetMacro	284
#define	SetStringMacro	285
#define	GetStringMacro	286
#define	SetClampMacro	287
#define	SetObjectMacro	288
#define	SetReferenceCountedObjectMacro	289
#define	GetObjectMacro	290
#define	BooleanMacro	291
#define	SetVector2Macro	292
#define	SetVector3Macro	293
#define	SetVector4Macro	294
#define	SetVector6Macro	295
#define	GetVector2Macro	296
#define	GetVector3Macro	297
#define	GetVector4Macro	298
#define	GetVector6Macro	299
#define	SetVectorMacro	300
#define	GetVectorMacro	301
#define	ViewportCoordinateMacro	302
#define	WorldCoordinateMacro	303
#define	TypeMacro	304

#line 43 "vtkParse.y"

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

#line 129 "vtkParse.y"
typedef union{
  char *str;
  int   integer;
  } YYSTYPE;

#ifndef YYLTYPE
typedef
  struct yyltype
    {
      int timestamp;
      int first_line;
      int first_column;
      int last_line;
      int last_column;
      char *text;
   }
  yyltype;

#define YYLTYPE yyltype
#endif

#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		322
#define	YYFLAG		-32768
#define	YYNTBASE	65

#define YYTRANSLATE(x) ((unsigned)(x) <= 304 ? yytranslate[x] : 133)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,    61,     2,    56,
    57,    62,     2,    58,    63,    64,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,    52,    53,     2,
    55,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
    59,     2,    60,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    50,     2,    51,    54,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
    16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
    26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
    36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
    46,    47,    48,    49
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     4,     5,    13,    15,    18,    21,    23,    25,    28,
    31,    33,    36,    40,    42,    45,    49,    52,    53,    54,
    60,    64,    69,    70,    72,    73,    79,    81,    83,    85,
    87,    89,    94,    98,   102,   103,   105,   107,   108,   113,
   115,   116,   121,   123,   124,   127,   131,   134,   137,   138,
   139,   143,   148,   151,   153,   156,   160,   162,   165,   167,
   169,   172,   175,   176,   180,   182,   184,   186,   188,   190,
   192,   194,   196,   198,   200,   201,   204,   207,   208,   214,
   216,   218,   220,   223,   225,   227,   231,   233,   234,   242,
   243,   244,   253,   254,   260,   261,   267,   268,   269,   280,
   281,   289,   290,   298,   299,   300,   309,   310,   318,   319,
   327,   328,   336,   337,   345,   346,   354,   355,   363,   364,
   372,   373,   381,   382,   390,   391,   401,   402,   412,   417,
   422,   429,   430,   433,   434,   437,   439,   441,   443,   445,
   447,   449,   451,   453,   455,   457,   459,   461,   463,   465,
   467,   469,   471,   473,   475,   477,   479,   481,   485,   489
};

static const short yyrhs[] = {   126,
    66,   126,     0,     0,     3,    24,    67,    97,    50,    68,
    51,     0,    69,     0,    69,    68,     0,   100,    52,     0,
    87,     0,    70,     0,    23,    70,     0,   103,    53,     0,
   103,     0,    54,    71,     0,     7,    54,    71,     0,    71,
     0,    91,    71,     0,     7,    91,    71,     0,     7,    71,
     0,     0,     0,    75,    72,    74,    73,    80,     0,    21,
   127,    53,     0,    75,    55,     9,    53,     0,     0,    20,
     0,     0,    79,    56,    76,    81,    57,     0,    20,     0,
    25,     0,    24,     0,    10,     0,    53,     0,    50,   126,
    51,    53,     0,    50,   126,    51,     0,    52,   127,    53,
     0,     0,    82,     0,    84,     0,     0,    84,    83,    58,
    82,     0,    91,     0,     0,    91,    88,    85,    86,     0,
    26,     0,     0,    55,   101,     0,    91,    88,    53,     0,
    26,    53,     0,    79,    89,     0,     0,     0,    27,    90,
    89,     0,    59,   127,    60,    89,     0,    77,    92,     0,
    92,     0,    78,    92,     0,    78,    77,    92,     0,    94,
     0,    94,    93,     0,    61,     0,    62,     0,    61,    93,
     0,    62,    93,     0,     0,    22,    95,    96,     0,    96,
     0,    12,     0,    16,     0,    17,     0,    11,     0,    13,
     0,    14,     0,    15,     0,    10,     0,    24,     0,     0,
    52,    98,     0,   100,    24,     0,     0,   100,    24,    99,
    58,    98,     0,     4,     0,     5,     0,     6,     0,    63,
   102,     0,   102,     0,     9,     0,     9,    64,     9,     0,
    79,     0,     0,    28,    56,    79,    58,   104,    94,    57,
     0,     0,     0,    29,    56,   105,    79,    58,   106,    94,
    57,     0,     0,    30,    56,   107,    79,    57,     0,     0,
    31,    56,   108,    79,    57,     0,     0,     0,    32,    56,
    79,    58,   109,    94,   110,    58,   127,    57,     0,     0,
    33,    56,    79,    58,   111,    94,    57,     0,     0,    34,
    56,    79,    58,   112,    94,    57,     0,     0,     0,    35,
    56,   113,    79,    58,   114,    94,    57,     0,     0,    36,
    56,    79,   115,    58,    94,    57,     0,     0,    37,    56,
    79,    58,   116,    94,    57,     0,     0,    41,    56,    79,
    58,   117,    94,    57,     0,     0,    38,    56,    79,    58,
   118,    94,    57,     0,     0,    42,    56,    79,    58,   119,
    94,    57,     0,     0,    39,    56,    79,    58,   120,    94,
    57,     0,     0,    43,    56,    79,    58,   121,    94,    57,
     0,     0,    40,    56,    79,    58,   122,    94,    57,     0,
     0,    44,    56,    79,    58,   123,    94,    57,     0,     0,
    45,    56,    79,    58,   124,    94,    58,   101,    57,     0,
     0,    46,    56,    79,    58,   125,    94,    58,   101,    57,
     0,    47,    56,    79,    57,     0,    48,    56,    79,    57,
     0,    49,    56,    79,    58,    79,    57,     0,     0,   128,
   126,     0,     0,   129,   127,     0,    53,     0,   129,     0,
    19,     0,   130,     0,   131,     0,    62,     0,    55,     0,
    52,     0,    58,     0,    64,     0,     8,     0,    94,     0,
     9,     0,    18,     0,    61,     0,   132,     0,    20,     0,
    21,     0,    63,     0,    54,     0,    25,     0,    27,     0,
    50,   126,    51,     0,    56,   126,    57,     0,    59,   126,
    60,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   188,   190,   194,   196,   196,   198,   198,   199,   199,   199,
   199,   201,   202,   203,   207,   212,   218,   224,   224,   226,
   231,   236,   245,   245,   247,   247,   249,   251,   253,   253,
   255,   256,   257,   258,   260,   260,   262,   263,   263,   265,
   270,   282,   282,   289,   289,   291,   291,   293,   295,   296,
   298,   299,   303,   304,   305,   306,   308,   309,   319,   320,
   321,   322,   324,   325,   326,   328,   329,   330,   331,   332,
   333,   334,   335,   340,   359,   359,   361,   366,   370,   372,
   373,   374,   376,   376,   378,   379,   379,   381,   383,   394,
   394,   395,   403,   403,   414,   414,   423,   425,   425,   455,
   456,   467,   468,   479,   479,   480,   488,   490,   505,   510,
   535,   540,   552,   557,   584,   589,   601,   606,   635,   640,
   652,   657,   690,   695,   707,   712,   725,   730,   742,   790,
   840,   868,   868,   869,   869,   871,   871,   873,   873,   873,
   873,   873,   873,   873,   873,   874,   874,   874,   874,   874,
   874,   874,   875,   875,   875,   875,   875,   877,   878,   879
};

static const char * const yytname[] = {   "$","error","$illegal.","CLASS","PUBLIC",
"PRIVATE","PROTECTED","VIRTUAL","STRING","NUM","ID","INT","FLOAT","SHORT","LONG",
"DOUBLE","VOID","CHAR","CLASS_REF","OTHER","CONST","OPERATOR","UNSIGNED","FRIEND",
"VTK_ID","STATIC","VAR_FUNCTION","ARRAY_NUM","SetMacro","GetMacro","SetStringMacro",
"GetStringMacro","SetClampMacro","SetObjectMacro","SetReferenceCountedObjectMacro",
"GetObjectMacro","BooleanMacro","SetVector2Macro","SetVector3Macro","SetVector4Macro",
"SetVector6Macro","GetVector2Macro","GetVector3Macro","GetVector4Macro","GetVector6Macro",
"SetVectorMacro","GetVectorMacro","ViewportCoordinateMacro","WorldCoordinateMacro",
"TypeMacro","'{'","'}'","':'","';'","'~'","'='","'('","')'","','","'['","']'",
"'&'","'*'","'-'","'.'","strt","class_def","@1","class_def_body","class_def_item",
"function","func","@2","@3","maybe_const","func_beg","@4","const_mod","static_mod",
"any_id","func_end","args_list","more_args","@5","arg","@6","opt_var_assign",
"var","var_id","var_array","@7","type","type_red1","type_indirection","type_red2",
"@8","type_primitive","optional_scope","scope_list","@9","scope_type","float_num",
"float_prim","macro","@10","@11","@12","@13","@14","@15","@16","@17","@18","@19",
"@20","@21","@22","@23","@24","@25","@26","@27","@28","@29","@30","@31","maybe_other",
"maybe_other_no_semi","other_stuff","other_stuff_no_semi","braces","parens",
"brackets",""
};
#endif

static const short yyr1[] = {     0,
    65,    67,    66,    68,    68,    69,    69,    69,    69,    69,
    69,    70,    70,    70,    70,    70,    70,    72,    73,    71,
    71,    71,    74,    74,    76,    75,    77,    78,    79,    79,
    80,    80,    80,    80,    81,    81,    82,    83,    82,    84,
    85,    84,    84,    86,    86,    87,    87,    88,    89,    90,
    89,    89,    91,    91,    91,    91,    92,    92,    93,    93,
    93,    93,    95,    94,    94,    96,    96,    96,    96,    96,
    96,    96,    96,    96,    97,    97,    98,    99,    98,   100,
   100,   100,   101,   101,   102,   102,   102,   104,   103,   105,
   106,   103,   107,   103,   108,   103,   109,   110,   103,   111,
   103,   112,   103,   113,   114,   103,   115,   103,   116,   103,
   117,   103,   118,   103,   119,   103,   120,   103,   121,   103,
   122,   103,   123,   103,   124,   103,   125,   103,   103,   103,
   103,   126,   126,   127,   127,   128,   128,   129,   129,   129,
   129,   129,   129,   129,   129,   129,   129,   129,   129,   129,
   129,   129,   129,   129,   129,   129,   129,   130,   131,   132
};

static const short yyr2[] = {     0,
     3,     0,     7,     1,     2,     2,     1,     1,     2,     2,
     1,     2,     3,     1,     2,     3,     2,     0,     0,     5,
     3,     4,     0,     1,     0,     5,     1,     1,     1,     1,
     1,     4,     3,     3,     0,     1,     1,     0,     4,     1,
     0,     4,     1,     0,     2,     3,     2,     2,     0,     0,
     3,     4,     2,     1,     2,     3,     1,     2,     1,     1,
     2,     2,     0,     3,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     0,     2,     2,     0,     5,     1,
     1,     1,     2,     1,     1,     3,     1,     0,     7,     0,
     0,     8,     0,     5,     0,     5,     0,     0,    10,     0,
     7,     0,     7,     0,     0,     8,     0,     7,     0,     7,
     0,     7,     0,     7,     0,     7,     0,     7,     0,     7,
     0,     7,     0,     7,     0,     9,     0,     9,     4,     4,
     6,     0,     2,     0,     2,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     3,     3,     3
};

static const short yydefact[] = {   132,
   146,   148,    73,    69,    66,    70,    71,    72,    67,    68,
   149,   138,   152,   153,    63,    74,   156,   157,   132,   143,
   136,   155,   142,   132,   144,   132,   150,   141,   154,   145,
   147,    65,     0,   132,   137,   139,   140,   151,     0,     0,
     0,     0,     0,   132,   133,    64,   158,   159,   160,     2,
     1,    75,     0,     0,    80,    81,    82,    76,     0,     0,
    77,     0,    73,    27,   134,     0,    74,    28,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     4,     8,    14,    18,     0,     0,     0,
     7,     0,    54,    57,     0,    11,     0,     0,    17,     0,
     0,   134,     9,     0,    47,     0,    90,    93,    95,     0,
     0,     0,   104,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,    30,    29,    12,
     3,     5,     0,    23,    53,     0,    55,    25,    15,    49,
     0,    59,    60,    58,     6,    10,     0,    13,    16,    21,
   135,     0,     0,     0,     0,     0,     0,     0,     0,   107,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,    24,    19,    56,    35,    50,   134,
    48,    46,    61,    62,    79,    88,     0,     0,     0,    97,
   100,   102,     0,     0,   109,   113,   117,   121,   111,   115,
   119,   123,   125,   127,   129,   130,     0,    22,     0,    43,
     0,    36,    37,    40,    49,     0,     0,    91,    94,    96,
     0,     0,     0,   105,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,   132,   134,    31,    20,
    26,     0,    49,    41,    51,    49,     0,     0,    98,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,   131,     0,     0,     0,    44,    52,    89,
     0,     0,   101,   103,     0,   108,   110,   114,   118,   122,
   112,   116,   120,   124,     0,     0,    33,    34,    39,     0,
    42,    92,   134,   106,    85,     0,    87,     0,    84,     0,
    32,    45,     0,     0,    83,   126,   128,    99,    86,     0,
     0,     0
};

static const short yydefgoto[] = {   320,
    44,    52,    93,    94,    95,    96,   144,   219,   186,    97,
   188,    98,    99,   100,   250,   221,   222,   252,   223,   278,
   301,   101,   151,   191,   225,   102,   103,   154,    31,    39,
    32,    54,    58,   107,    59,   308,   309,   106,   227,   163,
   258,   164,   165,   231,   282,   232,   233,   169,   262,   204,
   236,   240,   237,   241,   238,   242,   239,   243,   244,   245,
    33,   111,    34,    35,    36,    37,    38
};

static const short yypact[] = {    95,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,    95,-32768,
-32768,-32768,-32768,    95,-32768,    95,-32768,-32768,-32768,-32768,
-32768,-32768,    -1,    95,-32768,-32768,-32768,-32768,   353,   -42,
   -35,    21,    33,    95,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,     7,    87,    39,-32768,-32768,-32768,-32768,    73,   292,
    41,   337,    44,-32768,   231,   153,    62,-32768,    68,    67,
    71,    72,    74,    75,    76,    78,    79,    80,    81,    82,
    83,    84,    85,    86,    88,    96,    99,   105,   106,   115,
   116,    61,   125,   292,-32768,-32768,    69,    53,   249,   123,
-32768,    61,-32768,   -34,   128,   131,   127,    61,-32768,    61,
   136,   231,-32768,    61,-32768,     8,-32768,-32768,-32768,     8,
     8,     8,-32768,     8,     8,     8,     8,     8,     8,     8,
     8,     8,     8,     8,     8,     8,     8,-32768,-32768,-32768,
-32768,-32768,   172,   182,-32768,    53,-32768,-32768,-32768,    -4,
   150,   -34,   -34,-32768,-32768,-32768,    87,-32768,-32768,-32768,
-32768,   148,     8,     8,     8,   152,   154,   156,     8,-32768,
   158,   161,   162,   176,   196,   199,   209,   210,   212,   214,
   151,   173,   216,   222,-32768,-32768,-32768,   211,-32768,   231,
-32768,-32768,-32768,-32768,-32768,-32768,   218,   220,   221,-32768,
-32768,-32768,   224,   226,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,     8,-32768,    34,-32768,
   223,-32768,   230,     8,    -3,   219,    53,-32768,-32768,-32768,
    53,    53,    53,-32768,    53,    53,    53,    53,    53,    53,
    53,    53,    53,    53,    53,   234,    95,   231,-32768,-32768,
-32768,   242,    -3,-32768,-32768,    -3,   244,    53,-32768,   253,
   254,    53,   262,   285,   286,   287,   288,   298,   299,   303,
   314,   315,   316,-32768,   160,   319,   211,   320,-32768,-32768,
   321,   318,-32768,-32768,   322,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,    -5,    -5,   327,-32768,-32768,    -5,
-32768,-32768,   231,-32768,   317,    52,-32768,   325,-32768,   326,
-32768,-32768,   328,   375,-32768,-32768,-32768,-32768,-32768,   386,
   387,-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,   294,-32768,   323,   -12,-32768,-32768,-32768,-32768,
-32768,   291,-32768,   -91,-32768,-32768,   117,-32768,-32768,-32768,
-32768,-32768,   168,  -205,-32768,   -59,   -86,   -58,   -45,-32768,
   354,-32768,   238,-32768,   -46,  -217,    90,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
   -18,  -102,-32768,   -65,-32768,-32768,-32768
};


#define	YYLAST		396


static const short yytable[] = {   112,
    40,    43,   110,   305,   138,    41,   114,    42,    47,   161,
   150,   145,   147,   105,   104,    45,   104,   138,   139,   255,
   104,    48,   189,   189,   162,    51,   152,   153,   166,   167,
   168,   139,   170,   171,   172,   173,   174,   175,   176,   177,
   178,   179,   180,   181,   182,   183,   112,   105,   104,   109,
   279,   148,   104,   104,   190,   190,    50,   306,    53,   187,
   305,   138,     3,     4,     5,     6,     7,     8,     9,    10,
   138,   197,   198,   199,    15,   139,    16,   203,   310,   140,
    49,    65,   312,   247,   139,   248,   249,   226,    60,   149,
    55,    56,    57,   193,   194,   158,    61,   159,   -78,   -30,
   104,   149,     1,     2,     3,     4,     5,     6,     7,     8,
     9,    10,    11,    12,    13,    14,    15,   -29,    16,    17,
   115,    18,   116,   143,   112,   246,   117,   118,   224,   119,
   120,   121,   253,   122,   123,   124,   125,   126,   127,   128,
   129,   130,   104,   131,    19,   276,    20,    21,    22,    23,
    24,   132,    25,    26,   133,    27,    28,    29,    30,    62,
   134,   135,    63,     4,     5,     6,     7,     8,     9,    10,
   136,   137,    64,    65,    15,   141,    67,    68,   148,   155,
   184,   257,   112,   156,   157,   259,   260,   261,   160,   263,
   264,   265,   266,   267,   268,   269,   270,   271,   272,   273,
   313,   185,   192,   307,   307,   196,    92,   215,   307,   200,
   297,   201,   281,   202,   307,   205,   285,   224,   206,   207,
     3,     4,     5,     6,     7,     8,     9,    10,   275,   216,
    64,   104,    15,   208,    16,    68,   220,   112,     1,     2,
     3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
    13,    14,    15,   209,    16,    17,   210,    18,     3,     4,
     5,     6,     7,     8,     9,    10,   211,   212,    64,   213,
    15,   214,    16,   217,   218,   228,   229,   230,   256,   251,
    19,   234,    20,   235,    22,    23,    24,   -38,    25,    26,
   274,    27,    28,    29,    30,    55,    56,    57,    62,   277,
   280,    63,     4,     5,     6,     7,     8,     9,    10,   283,
   284,    64,    65,    15,    66,    67,    68,    69,   286,    70,
    71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
    81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
    91,   287,   288,   289,   290,    92,    63,     4,     5,     6,
     7,     8,     9,    10,   291,   292,    64,    65,    15,   293,
    67,    68,     3,     4,     5,     6,     7,     8,     9,    10,
   294,   298,   295,   296,   300,   303,    16,   302,   304,   311,
   314,   316,   317,   319,   318,   321,   322,   142,   113,   146,
   108,   254,    46,   299,   195,   315
};

static const short yycheck[] = {    65,
    19,     3,    62,     9,    10,    24,    66,    26,    51,   112,
   102,    98,    99,    60,    60,    34,    62,    10,    24,   225,
    66,    57,    27,    27,   116,    44,    61,    62,   120,   121,
   122,    24,   124,   125,   126,   127,   128,   129,   130,   131,
   132,   133,   134,   135,   136,   137,   112,    94,    94,    62,
   256,    56,    98,    99,    59,    59,    24,    63,    52,   146,
     9,    10,    10,    11,    12,    13,    14,    15,    16,    17,
    10,   163,   164,   165,    22,    24,    24,   169,   296,    92,
    60,    21,   300,    50,    24,    52,    53,   190,    50,   102,
     4,     5,     6,   152,   153,   108,    24,   110,    58,    56,
   146,   114,     8,     9,    10,    11,    12,    13,    14,    15,
    16,    17,    18,    19,    20,    21,    22,    56,    24,    25,
    53,    27,    56,    55,   190,   217,    56,    56,   188,    56,
    56,    56,   224,    56,    56,    56,    56,    56,    56,    56,
    56,    56,   188,    56,    50,   248,    52,    53,    54,    55,
    56,    56,    58,    59,    56,    61,    62,    63,    64,     7,
    56,    56,    10,    11,    12,    13,    14,    15,    16,    17,
    56,    56,    20,    21,    22,    51,    24,    25,    56,    52,
     9,   227,   248,    53,    58,   231,   232,   233,    53,   235,
   236,   237,   238,   239,   240,   241,   242,   243,   244,   245,
   303,    20,    53,   295,   296,    58,    54,    57,   300,    58,
    51,    58,   258,    58,   306,    58,   262,   277,    58,    58,
    10,    11,    12,    13,    14,    15,    16,    17,   247,    57,
    20,   277,    22,    58,    24,    25,    26,   303,     8,     9,
    10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
    20,    21,    22,    58,    24,    25,    58,    27,    10,    11,
    12,    13,    14,    15,    16,    17,    58,    58,    20,    58,
    22,    58,    24,    58,    53,    58,    57,    57,    60,    57,
    50,    58,    52,    58,    54,    55,    56,    58,    58,    59,
    57,    61,    62,    63,    64,     4,     5,     6,     7,    58,
    57,    10,    11,    12,    13,    14,    15,    16,    17,    57,
    57,    20,    21,    22,    23,    24,    25,    26,    57,    28,
    29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
    39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
    49,    57,    57,    57,    57,    54,    10,    11,    12,    13,
    14,    15,    16,    17,    57,    57,    20,    21,    22,    57,
    24,    25,    10,    11,    12,    13,    14,    15,    16,    17,
    57,    53,    58,    58,    55,    58,    24,    57,    57,    53,
    64,    57,    57,     9,    57,     0,     0,    94,    66,    99,
    54,   224,    39,   277,   157,   306
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "bison.simple"

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Bob Corbett and Richard Stallman

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */


#ifndef alloca
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not GNU C.  */
#ifdef __DECC
#include <builtins.h>
#ifndef C_ALLOCA
#define alloca __ALLOCA
#else
void *alloca ();
#endif
#else /* not DEC C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi)
#include <alloca.h>
#else /* not sparc */
#if defined (MSDOS) && !defined (__TURBOC__)
#include <malloc.h>
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
#include <malloc.h>
 #pragma alloca
#else /* not MSDOS, __TURBOC__, or _AIX */
#ifdef __hpux
#ifdef __cplusplus
extern "C" {
void *alloca (unsigned int);
};
#else /* not __cplusplus */
void *alloca ();
#endif /* not __cplusplus */
#endif /* __hpux */
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc.  */
#endif /* not DEC C.  */
#endif /* not GNU C.  */
#endif /* alloca not defined.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	return(0)
#define YYABORT 	return(1)
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#define YYLEX		yylex(&yylval, &yylloc)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
int yyparse (void);
#endif

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_bcopy(FROM,TO,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_bcopy (from, to, count)
     char *from;
     char *to;
     int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_bcopy (char *from, char *to, int count)
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 184 "bison.simple"
int
yyparse()
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
      yyss = (short *) alloca (yystacksize * sizeof (*yyssp));
      __yy_bcopy ((char *)yyss1, (char *)yyss, size * sizeof (*yyssp));
      yyvs = (YYSTYPE *) alloca (yystacksize * sizeof (*yyvsp));
      __yy_bcopy ((char *)yyvs1, (char *)yyvs, size * sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) alloca (yystacksize * sizeof (*yylsp));
      __yy_bcopy ((char *)yyls1, (char *)yyls, size * sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 2:
#line 191 "vtkParse.y"
{
      data.ClassName = strdup(yyvsp[0].str);
      ;
    break;}
case 12:
#line 201 "vtkParse.y"
{ preSig("~"); output_function(); ;
    break;}
case 13:
#line 202 "vtkParse.y"
{ preSig("virtual ~"); output_function(); ;
    break;}
case 14:
#line 204 "vtkParse.y"
{
         output_function();
	 ;
    break;}
case 15:
#line 208 "vtkParse.y"
{
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
	 ;
    break;}
case 16:
#line 213 "vtkParse.y"
{
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
	 ;
    break;}
case 17:
#line 219 "vtkParse.y"
{
         preSig("virtual ");
         output_function();
	 ;
    break;}
case 18:
#line 224 "vtkParse.y"
{ postSig(")"); ;
    break;}
case 19:
#line 224 "vtkParse.y"
{ postSig(";"); openSig = 0; ;
    break;}
case 20:
#line 226 "vtkParse.y"
{
      openSig = 1;
      currentFunction->Name = yyvsp[-4].str; 
      fprintf(stderr,"   Parsed func %s\n",yyvsp[-4].str); 
    ;
    break;}
case 21:
#line 232 "vtkParse.y"
{ 
      currentFunction->IsOperator = 1; 
      fprintf(stderr,"   Converted operator\n"); 
    ;
    break;}
case 22:
#line 237 "vtkParse.y"
{ 
      postSig(") = 0;"); 
      currentFunction->Name = yyvsp[-3].str; 
      fprintf(stderr,"   Parsed func %s\n",yyvsp[-3].str); 
      currentFunction->IsPureVirtual = 1; 
      data.IsAbstract = 1;
    ;
    break;}
case 24:
#line 245 "vtkParse.y"
{postSig(" const");;
    break;}
case 25:
#line 247 "vtkParse.y"
{postSig(" ("); ;
    break;}
case 27:
#line 249 "vtkParse.y"
{postSig("const ");;
    break;}
case 28:
#line 251 "vtkParse.y"
{postSig("static ");;
    break;}
case 29:
#line 253 "vtkParse.y"
{postSig(yyvsp[0].str);;
    break;}
case 30:
#line 253 "vtkParse.y"
{postSig(yyvsp[0].str);;
    break;}
case 37:
#line 262 "vtkParse.y"
{ currentFunction->NumberOfArguments++;;
    break;}
case 38:
#line 263 "vtkParse.y"
{ currentFunction->NumberOfArguments++; postSig(", ");;
    break;}
case 40:
#line 266 "vtkParse.y"
{
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
	yyvsp[0].integer;;
    break;}
case 41:
#line 271 "vtkParse.y"
{
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 
	yyvsp[0].integer / 10000; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
	yyvsp[-1].integer + yyvsp[0].integer % 10000;
      /* fail if array is not const */
      if (((yyvsp[0].integer % 10000)/100) % 10 != 0 
	  && (yyvsp[-1].integer / 1000) != 1 ) {
	currentFunction->ArrayFailure = 1;
      }
    ;
    break;}
case 43:
#line 283 "vtkParse.y"
{ 
      postSig("void (*func)(void *) ");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 5000;
    ;
    break;}
case 46:
#line 291 "vtkParse.y"
{delSig();;
    break;}
case 47:
#line 291 "vtkParse.y"
{delSig();;
    break;}
case 48:
#line 293 "vtkParse.y"
{ yyval.integer = yyvsp[0].integer; ;
    break;}
case 49:
#line 295 "vtkParse.y"
{ yyval.integer = 0; ;
    break;}
case 50:
#line 296 "vtkParse.y"
{ char temp[100]; sprintf(temp,"[%i]",yyvsp[0].integer); 
                   postSig(temp); ;
    break;}
case 51:
#line 298 "vtkParse.y"
{ yyval.integer = 300 + 10000 * yyvsp[-2].integer; ;
    break;}
case 52:
#line 300 "vtkParse.y"
{ postSig("[]"); yyval.integer = 300; ;
    break;}
case 53:
#line 303 "vtkParse.y"
{yyval.integer = 1000 + yyvsp[0].integer;;
    break;}
case 54:
#line 304 "vtkParse.y"
{yyval.integer = yyvsp[0].integer;;
    break;}
case 55:
#line 305 "vtkParse.y"
{yyval.integer = 2000 + yyvsp[0].integer;;
    break;}
case 56:
#line 306 "vtkParse.y"
{yyval.integer = 3000 + yyvsp[0].integer;;
    break;}
case 57:
#line 308 "vtkParse.y"
{yyval.integer = yyvsp[0].integer;;
    break;}
case 58:
#line 310 "vtkParse.y"
{yyval.integer = yyvsp[-1].integer + yyvsp[0].integer;;
    break;}
case 59:
#line 319 "vtkParse.y"
{ postSig("&"); yyval.integer = 100;;
    break;}
case 60:
#line 320 "vtkParse.y"
{ postSig("*"); yyval.integer = 300;;
    break;}
case 61:
#line 321 "vtkParse.y"
{ yyval.integer = 100 + yyvsp[0].integer;;
    break;}
case 62:
#line 322 "vtkParse.y"
{ yyval.integer = 400 + yyvsp[0].integer;;
    break;}
case 63:
#line 324 "vtkParse.y"
{postSig("unsigned ");;
    break;}
case 64:
#line 325 "vtkParse.y"
{ yyval.integer = 10 + yyvsp[0].integer;;
    break;}
case 65:
#line 326 "vtkParse.y"
{ yyval.integer = yyvsp[0].integer;;
    break;}
case 66:
#line 329 "vtkParse.y"
{ postSig("float "); yyval.integer = 1;;
    break;}
case 67:
#line 330 "vtkParse.y"
{ postSig("void "); yyval.integer = 2;;
    break;}
case 68:
#line 331 "vtkParse.y"
{ postSig("char "); yyval.integer = 3;;
    break;}
case 69:
#line 332 "vtkParse.y"
{ postSig("int "); yyval.integer = 4;;
    break;}
case 70:
#line 333 "vtkParse.y"
{ postSig("short "); yyval.integer = 5;;
    break;}
case 71:
#line 334 "vtkParse.y"
{ postSig("long "); yyval.integer = 6;;
    break;}
case 72:
#line 335 "vtkParse.y"
{ postSig("double "); yyval.integer = 7;;
    break;}
case 73:
#line 336 "vtkParse.y"
{       
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",yyvsp[0].str);
      postSig(ctmpid);
      yyval.integer = 8;;
    break;}
case 74:
#line 342 "vtkParse.y"
{ 
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",yyvsp[0].str);
      postSig(ctmpid);
      yyval.integer = 9; 
      currentFunction->ArgClasses[currentFunction->NumberOfArguments] =
        strdup(yyvsp[0].str); 
      /* store the string into the return value just in case we need it */
      /* this is a parsing hack because the first "type" parser will */
      /* possibly be ht ereturn type of the first argument */
      if ((!currentFunction->ReturnClass) && 
          (!currentFunction->NumberOfArguments)) 
        { 
        currentFunction->ReturnClass = strdup(yyvsp[0].str); 
        }
    ;
    break;}
case 77:
#line 362 "vtkParse.y"
{ 
      data.SuperClasses[data.NumberOfSuperClasses] = strdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    ;
    break;}
case 78:
#line 367 "vtkParse.y"
{ 
      data.SuperClasses[data.NumberOfSuperClasses] = strdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    ;
    break;}
case 80:
#line 372 "vtkParse.y"
{in_public = 1; in_protected = 0;;
    break;}
case 81:
#line 373 "vtkParse.y"
{in_public = 0; in_protected = 0;;
    break;}
case 82:
#line 374 "vtkParse.y"
{in_public = 0; in_protected = 1;;
    break;}
case 85:
#line 378 "vtkParse.y"
{yyval.integer = yyvsp[0].integer;;
    break;}
case 86:
#line 379 "vtkParse.y"
{yyval.integer = -1;;
    break;}
case 87:
#line 379 "vtkParse.y"
{yyval.integer = -1;;
    break;}
case 88:
#line 383 "vtkParse.y"
{preSig("void Set"); postSig(" ("); ;
    break;}
case 89:
#line 384 "vtkParse.y"
{
   postSig(");");
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
   output_function();
   ;
    break;}
case 90:
#line 394 "vtkParse.y"
{postSig("Get");;
    break;}
case 91:
#line 394 "vtkParse.y"
{postSig(" ();"); invertSig = 1;;
    break;}
case 92:
#line 396 "vtkParse.y"
{ 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = yyvsp[-1].integer;
   output_function();
   ;
    break;}
case 93:
#line 403 "vtkParse.y"
{preSig("void Set");;
    break;}
case 94:
#line 404 "vtkParse.y"
{
   postSig(" (char *);"); 
   sprintf(temps,"Set%s",yyvsp[-1].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 303;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
   output_function();
   ;
    break;}
case 95:
#line 414 "vtkParse.y"
{preSig("char *Get");;
    break;}
case 96:
#line 415 "vtkParse.y"
{ 
   postSig(" ();");
   sprintf(temps,"Get%s",yyvsp[-1].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 303;
   output_function();
   ;
    break;}
case 97:
#line 424 "vtkParse.y"
{preSig("void Set"); postSig(" ("); ;
    break;}
case 98:
#line 425 "vtkParse.y"
{postSig(");"); openSig = 0;;
    break;}
case 99:
#line 426 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sscanf (currentFunction->Signature, "%*s %*s (%s);", local);
   sprintf(temps,"Set%s",yyvsp[-7].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = yyvsp[-4].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%s Get%sMinValue ();",local,yyvsp[-7].str);
   sprintf(temps,"Get%sMinValue",yyvsp[-7].str);
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = yyvsp[-4].integer;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%s Get%sMaxValue ();",local,yyvsp[-7].str);
   sprintf(temps,"Get%sMaxValue",yyvsp[-7].str);
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = yyvsp[-4].integer;
   output_function();
   ;
    break;}
case 100:
#line 456 "vtkParse.y"
{preSig("void Set"); postSig(" ("); ;
    break;}
case 101:
#line 457 "vtkParse.y"
{ 
   postSig("*);");
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 2;
   output_function();
   ;
    break;}
case 102:
#line 468 "vtkParse.y"
{preSig("void Set"); postSig(" ("); ;
    break;}
case 103:
#line 469 "vtkParse.y"
{ 
   postSig("*);");
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 2;
   output_function();
   ;
    break;}
case 104:
#line 479 "vtkParse.y"
{postSig("*Get");;
    break;}
case 105:
#line 480 "vtkParse.y"
{postSig(" ();"); invertSig = 1;;
    break;}
case 106:
#line 481 "vtkParse.y"
{ 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 309;
   output_function();
   ;
    break;}
case 107:
#line 489 "vtkParse.y"
{preSig("void "); postSig("On ();"); openSig = 0; ;
    break;}
case 108:
#line 491 "vtkParse.y"
{ 
   sprintf(temps,"%sOn",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 2;
   output_function();
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void %sOff ();",yyvsp[-4].str); 
   sprintf(temps,"%sOff",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   output_function();
   ;
    break;}
case 109:
#line 506 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;
    break;}
case 110:
#line 511 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s);",yyvsp[-4].str,
     local, local);
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 2;
   currentFunction->ArgTypes[0] = yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = yyvsp[-1].integer;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[2]);",yyvsp[-4].str,
     local);
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 2;
   output_function();
   ;
    break;}
case 111:
#line 536 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;
    break;}
case 112:
#line 541 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 2;
   output_function();
   ;
    break;}
case 113:
#line 553 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;
    break;}
case 114:
#line 558 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s);",
     yyvsp[-4].str, local, local, local);
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 3;
   currentFunction->ArgTypes[0] = yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = yyvsp[-1].integer;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = yyvsp[-1].integer;
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[3]);",yyvsp[-4].str,
     local);
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 3;
   output_function();
   ;
    break;}
case 115:
#line 585 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;
    break;}
case 116:
#line 590 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 3;
   output_function();
   ;
    break;}
case 117:
#line 602 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;
    break;}
case 118:
#line 607 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s, %s);",
     yyvsp[-4].str, local, local, local, local);
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 4;
   currentFunction->ArgTypes[0] = yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = yyvsp[-1].integer;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = yyvsp[-1].integer;
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ArgTypes[3] = yyvsp[-1].integer;
   currentFunction->ArgCounts[3] = 0;
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[4]);",yyvsp[-4].str,
     local);
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 4;
   output_function();
   ;
    break;}
case 119:
#line 636 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;
    break;}
case 120:
#line 641 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 4;
   output_function();
   ;
    break;}
case 121:
#line 653 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;
    break;}
case 122:
#line 658 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s, %s, %s, %s);",
     yyvsp[-4].str, local, local, local, local, local, local);
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 6;
   currentFunction->ArgTypes[0] = yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = yyvsp[-1].integer;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = yyvsp[-1].integer;
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ArgTypes[3] = yyvsp[-1].integer;
   currentFunction->ArgCounts[3] = 0;
   currentFunction->ArgTypes[4] = yyvsp[-1].integer;
   currentFunction->ArgCounts[4] = 0;
   currentFunction->ArgTypes[5] = yyvsp[-1].integer;
   currentFunction->ArgCounts[5] = 0;
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[6]);",yyvsp[-4].str,
     local);
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 6;
   output_function();
   ;
    break;}
case 123:
#line 691 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;
    break;}
case 124:
#line 696 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 6;
   output_function();
   ;
    break;}
case 125:
#line 708 "vtkParse.y"
{
      free (currentFunction->Signature);
      currentFunction->Signature = NULL;
      ;
    break;}
case 126:
#line 713 "vtkParse.y"
{
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s [%i]);",yyvsp[-6].str,
      local, yyvsp[-1].integer);
     sprintf(temps,"Set%s",yyvsp[-6].str); 
     currentFunction->Name = strdup(temps);
     currentFunction->ReturnType = 2;
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 300 + yyvsp[-3].integer;
     currentFunction->ArgCounts[0] = yyvsp[-1].integer;
     output_function();
   ;
    break;}
case 127:
#line 726 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;
    break;}
case 128:
#line 731 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-6].str);
   sprintf(temps,"Get%s",yyvsp[-6].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yyvsp[-3].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = yyvsp[-1].integer;
   output_function();
   ;
    break;}
case 129:
#line 743 "vtkParse.y"
{ 
     char *local = strdup(currentFunction->Signature);
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       yyvsp[-1].str);

     sprintf(temps,"Get%sCoordinate",yyvsp[-1].str); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 309;
     currentFunction->ReturnClass = strdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (float, float);",
       yyvsp[-1].str);
     sprintf(temps,"Set%s",yyvsp[-1].str); 
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
       yyvsp[-1].str);
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 301;
     currentFunction->ArgCounts[0] = 2;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"float *Get%s ();", yyvsp[-1].str);
     sprintf(temps,"Get%s",yyvsp[-1].str); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 301;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 2;
     output_function();
   ;
    break;}
case 130:
#line 791 "vtkParse.y"
{ 
     char *local = strdup(currentFunction->Signature);
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       yyvsp[-1].str);

     sprintf(temps,"Get%sCoordinate",yyvsp[-1].str); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 309;
     currentFunction->ReturnClass = strdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (float, float, float);",
       yyvsp[-1].str);
     sprintf(temps,"Set%s",yyvsp[-1].str); 
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
       yyvsp[-1].str);
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 301;
     currentFunction->ArgCounts[0] = 3;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"float *Get%s ();", yyvsp[-1].str);
     sprintf(temps,"Get%s",yyvsp[-1].str); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 301;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 3;
     output_function();
   ;
    break;}
case 131:
#line 841 "vtkParse.y"
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
   ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 465 "bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;
}
#line 881 "vtkParse.y"

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
 


