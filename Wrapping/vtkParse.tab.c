/* A Bison parser, made by GNU Bison 1.875a.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     CLASS = 258,
     PUBLIC = 259,
     PRIVATE = 260,
     PROTECTED = 261,
     VIRTUAL = 262,
     STRING = 263,
     NUM = 264,
     ID = 265,
     INT = 266,
     FLOAT = 267,
     SHORT = 268,
     LONG = 269,
     DOUBLE = 270,
     VOID = 271,
     CHAR = 272,
     CLASS_REF = 273,
     OTHER = 274,
     CONST = 275,
     OPERATOR = 276,
     UNSIGNED = 277,
     FRIEND = 278,
     VTK_ID = 279,
     STATIC = 280,
     VAR_FUNCTION = 281,
     ARRAY_NUM = 282,
     VTK_LEGACY = 283,
     SetMacro = 284,
     GetMacro = 285,
     SetStringMacro = 286,
     GetStringMacro = 287,
     SetClampMacro = 288,
     SetObjectMacro = 289,
     SetReferenceCountedObjectMacro = 290,
     GetObjectMacro = 291,
     BooleanMacro = 292,
     SetVector2Macro = 293,
     SetVector3Macro = 294,
     SetVector4Macro = 295,
     SetVector6Macro = 296,
     GetVector2Macro = 297,
     GetVector3Macro = 298,
     GetVector4Macro = 299,
     GetVector6Macro = 300,
     SetVectorMacro = 301,
     GetVectorMacro = 302,
     ViewportCoordinateMacro = 303,
     WorldCoordinateMacro = 304,
     TypeMacro = 305
   };
#endif
#define CLASS 258
#define PUBLIC 259
#define PRIVATE 260
#define PROTECTED 261
#define VIRTUAL 262
#define STRING 263
#define NUM 264
#define ID 265
#define INT 266
#define FLOAT 267
#define SHORT 268
#define LONG 269
#define DOUBLE 270
#define VOID 271
#define CHAR 272
#define CLASS_REF 273
#define OTHER 274
#define CONST 275
#define OPERATOR 276
#define UNSIGNED 277
#define FRIEND 278
#define VTK_ID 279
#define STATIC 280
#define VAR_FUNCTION 281
#define ARRAY_NUM 282
#define VTK_LEGACY 283
#define SetMacro 284
#define GetMacro 285
#define SetStringMacro 286
#define GetStringMacro 287
#define SetClampMacro 288
#define SetObjectMacro 289
#define SetReferenceCountedObjectMacro 290
#define GetObjectMacro 291
#define BooleanMacro 292
#define SetVector2Macro 293
#define SetVector3Macro 294
#define SetVector4Macro 295
#define SetVector6Macro 296
#define GetVector2Macro 297
#define GetVector3Macro 298
#define GetVector4Macro 299
#define GetVector6Macro 300
#define SetVectorMacro 301
#define GetVectorMacro 302
#define ViewportCoordinateMacro 303
#define WorldCoordinateMacro 304
#define TypeMacro 305




/* Copy the first part of user declarations.  */
#line 15 "vtkParse.y"


/*

This file must be translated to C and modified to build everywhere.

Run yacc like this:

  yacc -b vtkParse vtkParse.y

Modify vtkParse.tab.c:
  - remove TABs

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define yyerror(a) fprintf(stderr,"%s\n",a)
#define yywrap() 1

/* MSVC Does not define __STDC__ properly. */
#if defined(_MSC_VER) && _MSC_VER >= 1200 && !defined(__STDC__)
# define __STDC__ 1
#endif

/* Disable warnings in generated code. */
#if defined(_MSC_VER)
# pragma warning (disable: 4127) /* conditional expression is constant */
# pragma warning (disable: 4244) /* conversion to smaller integer type */
#endif

int yylex(void);
void output_function();

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
      tmp = vtkstrdup(currentFunction->Signature);
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


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 141 "vtkParse.y"
typedef union YYSTYPE {
  char *str;
  int   integer;
  } YYSTYPE;
/* Line 191 of yacc.c.  */
#line 307 "vtkParse.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 319 "vtkParse.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
         || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))                         \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)              \
      do                                        \
        {                                       \
          register YYSIZE_T yyi;                \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (To)[yyi] = (From)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)                                        \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack, Stack, yysize);                          \
        Stack = &yyptr->Stack;                                          \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  44
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   480

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  66
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  72
/* YYNRULES -- Number of rules. */
#define YYNRULES  173
/* YYNRULES -- Number of states. */
#define YYNSTATES  345

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   305

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    62,     2,
      55,    56,    63,     2,    59,    64,    65,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    53,    54,
       2,    58,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    60,     2,    61,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    51,     2,    52,    57,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short yyprhs[] =
{
       0,     0,     3,     7,     8,    16,    18,    21,    24,    26,
      28,    31,    34,    38,    41,    44,    46,    51,    54,    58,
      60,    63,    67,    72,    76,    79,    81,    84,    88,    93,
      97,   100,   104,   105,   106,   111,   115,   116,   118,   119,
     125,   127,   129,   131,   133,   135,   140,   144,   148,   149,
     151,   153,   154,   159,   161,   162,   167,   169,   170,   173,
     177,   180,   183,   184,   185,   189,   194,   197,   199,   202,
     206,   208,   211,   213,   215,   218,   221,   222,   226,   228,
     230,   232,   234,   236,   238,   240,   242,   244,   246,   247,
     250,   253,   254,   260,   262,   264,   266,   269,   271,   273,
     277,   279,   280,   288,   289,   290,   299,   300,   306,   307,
     313,   314,   315,   326,   327,   335,   336,   344,   345,   346,
     355,   356,   364,   365,   373,   374,   382,   383,   391,   392,
     400,   401,   409,   410,   418,   419,   427,   428,   436,   437,
     447,   448,   458,   463,   468,   475,   476,   479,   480,   483,
     485,   487,   489,   491,   493,   495,   497,   499,   501,   503,
     505,   507,   509,   511,   513,   515,   517,   519,   521,   523,
     525,   527,   531,   535
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short yyrhs[] =
{
      67,     0,    -1,   131,    68,   131,    -1,    -1,     3,    24,
      69,   102,    51,    70,    52,    -1,    71,    -1,    71,    70,
      -1,   105,    53,    -1,    92,    -1,    74,    -1,    23,    74,
      -1,    73,    85,    -1,    23,    73,    85,    -1,    72,    85,
      -1,   108,    54,    -1,   108,    -1,    28,    55,    73,    56,
      -1,    57,    76,    -1,     7,    57,    76,    -1,    76,    -1,
      96,    76,    -1,    96,    20,    76,    -1,     7,    96,    20,
      76,    -1,     7,    96,    76,    -1,     7,    76,    -1,    75,
      -1,    96,    75,    -1,    96,    20,    75,    -1,     7,    96,
      20,    75,    -1,     7,    96,    75,    -1,     7,    75,    -1,
      21,   132,    54,    -1,    -1,    -1,    80,    77,    79,    78,
      -1,    80,    58,     9,    -1,    -1,    20,    -1,    -1,    84,
      55,    81,    86,    56,    -1,    20,    -1,    25,    -1,    24,
      -1,    10,    -1,    54,    -1,    51,   131,    52,    54,    -1,
      51,   131,    52,    -1,    53,   132,    54,    -1,    -1,    87,
      -1,    89,    -1,    -1,    89,    88,    59,    87,    -1,    96,
      -1,    -1,    96,    93,    90,    91,    -1,    26,    -1,    -1,
      58,   106,    -1,    96,    93,    54,    -1,    26,    54,    -1,
      84,    94,    -1,    -1,    -1,    27,    95,    94,    -1,    60,
     132,    61,    94,    -1,    82,    97,    -1,    97,    -1,    83,
      97,    -1,    83,    82,    97,    -1,    99,    -1,    99,    98,
      -1,    62,    -1,    63,    -1,    62,    98,    -1,    63,    98,
      -1,    -1,    22,   100,   101,    -1,   101,    -1,    12,    -1,
      16,    -1,    17,    -1,    11,    -1,    13,    -1,    14,    -1,
      15,    -1,    10,    -1,    24,    -1,    -1,    53,   103,    -1,
     105,    24,    -1,    -1,   105,    24,   104,    59,   103,    -1,
       4,    -1,     5,    -1,     6,    -1,    64,   107,    -1,   107,
      -1,     9,    -1,     9,    65,     9,    -1,    84,    -1,    -1,
      29,    55,    84,    59,   109,    99,    56,    -1,    -1,    -1,
      30,    55,   110,    84,    59,   111,    99,    56,    -1,    -1,
      31,    55,   112,    84,    56,    -1,    -1,    32,    55,   113,
      84,    56,    -1,    -1,    -1,    33,    55,    84,    59,   114,
      99,   115,    59,   132,    56,    -1,    -1,    34,    55,    84,
      59,   116,    99,    56,    -1,    -1,    35,    55,    84,    59,
     117,    99,    56,    -1,    -1,    -1,    36,    55,   118,    84,
      59,   119,    99,    56,    -1,    -1,    37,    55,    84,   120,
      59,    99,    56,    -1,    -1,    38,    55,    84,    59,   121,
      99,    56,    -1,    -1,    42,    55,    84,    59,   122,    99,
      56,    -1,    -1,    39,    55,    84,    59,   123,    99,    56,
      -1,    -1,    43,    55,    84,    59,   124,    99,    56,    -1,
      -1,    40,    55,    84,    59,   125,    99,    56,    -1,    -1,
      44,    55,    84,    59,   126,    99,    56,    -1,    -1,    41,
      55,    84,    59,   127,    99,    56,    -1,    -1,    45,    55,
      84,    59,   128,    99,    56,    -1,    -1,    46,    55,    84,
      59,   129,    99,    59,   106,    56,    -1,    -1,    47,    55,
      84,    59,   130,    99,    59,   106,    56,    -1,    48,    55,
      84,    56,    -1,    49,    55,    84,    56,    -1,    50,    55,
      84,    59,    84,    56,    -1,    -1,   133,   131,    -1,    -1,
     134,   132,    -1,    54,    -1,   134,    -1,    19,    -1,   135,
      -1,   136,    -1,    63,    -1,    58,    -1,    53,    -1,    59,
      -1,    65,    -1,     8,    -1,    99,    -1,     9,    -1,    18,
      -1,    62,    -1,   137,    -1,    20,    -1,    21,    -1,    64,
      -1,    57,    -1,    25,    -1,    27,    -1,    51,   131,    52,
      -1,    55,   131,    56,    -1,    60,   131,    61,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   201,   201,   204,   203,   209,   209,   211,   211,   212,
     213,   214,   215,   216,   217,   218,   220,   222,   223,   224,
     225,   229,   233,   238,   243,   249,   253,   258,   263,   269,
     275,   281,   287,   287,   287,   293,   302,   302,   304,   304,
     306,   308,   310,   310,   312,   313,   314,   315,   317,   317,
     319,   320,   320,   322,   328,   327,   334,   341,   341,   343,
     343,   345,   353,   354,   354,   357,   360,   361,   362,   363,
     365,   366,   376,   377,   378,   379,   381,   381,   383,   386,
     387,   388,   389,   390,   391,   392,   393,   398,   416,   416,
     418,   424,   423,   429,   430,   431,   433,   433,   435,   436,
     436,   440,   439,   451,   451,   451,   460,   460,   471,   471,
     481,   482,   480,   513,   512,   525,   524,   536,   537,   536,
     546,   545,   563,   562,   593,   592,   610,   609,   642,   641,
     659,   658,   693,   692,   710,   709,   748,   747,   765,   764,
     783,   782,   799,   846,   895,   951,   951,   952,   952,   954,
     954,   956,   956,   956,   956,   956,   956,   956,   956,   957,
     957,   957,   957,   957,   957,   957,   958,   958,   958,   958,
     958,   960,   961,   962
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "CLASS", "PUBLIC", "PRIVATE", "PROTECTED", 
  "VIRTUAL", "STRING", "NUM", "ID", "INT", "FLOAT", "SHORT", "LONG", 
  "DOUBLE", "VOID", "CHAR", "CLASS_REF", "OTHER", "CONST", "OPERATOR", 
  "UNSIGNED", "FRIEND", "VTK_ID", "STATIC", "VAR_FUNCTION", "ARRAY_NUM", 
  "VTK_LEGACY", "SetMacro", "GetMacro", "SetStringMacro", 
  "GetStringMacro", "SetClampMacro", "SetObjectMacro", 
  "SetReferenceCountedObjectMacro", "GetObjectMacro", "BooleanMacro", 
  "SetVector2Macro", "SetVector3Macro", "SetVector4Macro", 
  "SetVector6Macro", "GetVector2Macro", "GetVector3Macro", 
  "GetVector4Macro", "GetVector6Macro", "SetVectorMacro", 
  "GetVectorMacro", "ViewportCoordinateMacro", "WorldCoordinateMacro", 
  "TypeMacro", "'{'", "'}'", "':'", "';'", "'('", "')'", "'~'", "'='", 
  "','", "'['", "']'", "'&'", "'*'", "'-'", "'.'", "$accept", "strt", 
  "class_def", "@1", "class_def_body", "class_def_item", 
  "legacy_function", "function", "operator", "operator_sig", "func", "@2", 
  "@3", "maybe_const", "func_sig", "@4", "const_mod", "static_mod", 
  "any_id", "func_body", "args_list", "more_args", "@5", "arg", "@6", 
  "opt_var_assign", "var", "var_id", "var_array", "@7", "type", 
  "type_red1", "type_indirection", "type_red2", "@8", "type_primitive", 
  "optional_scope", "scope_list", "@9", "scope_type", "float_num", 
  "float_prim", "macro", "@10", "@11", "@12", "@13", "@14", "@15", "@16", 
  "@17", "@18", "@19", "@20", "@21", "@22", "@23", "@24", "@25", "@26", 
  "@27", "@28", "@29", "@30", "@31", "maybe_other", "maybe_other_no_semi", 
  "other_stuff", "other_stuff_no_semi", "braces", "parens", "brackets", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   123,   125,    58,    59,    40,    41,   126,    61,    44,
      91,    93,    38,    42,    45,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    66,    67,    69,    68,    70,    70,    71,    71,    71,
      71,    71,    71,    71,    71,    71,    72,    73,    73,    73,
      73,    73,    73,    73,    73,    74,    74,    74,    74,    74,
      74,    75,    77,    78,    76,    76,    79,    79,    81,    80,
      82,    83,    84,    84,    85,    85,    85,    85,    86,    86,
      87,    88,    87,    89,    90,    89,    89,    91,    91,    92,
      92,    93,    94,    95,    94,    94,    96,    96,    96,    96,
      97,    97,    98,    98,    98,    98,   100,    99,    99,   101,
     101,   101,   101,   101,   101,   101,   101,   101,   102,   102,
     103,   104,   103,   105,   105,   105,   106,   106,   107,   107,
     107,   109,   108,   110,   111,   108,   112,   108,   113,   108,
     114,   115,   108,   116,   108,   117,   108,   118,   119,   108,
     120,   108,   121,   108,   122,   108,   123,   108,   124,   108,
     125,   108,   126,   108,   127,   108,   128,   108,   129,   108,
     130,   108,   108,   108,   108,   131,   131,   132,   132,   133,
     133,   134,   134,   134,   134,   134,   134,   134,   134,   134,
     134,   134,   134,   134,   134,   134,   134,   134,   134,   134,
     134,   135,   136,   137
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     3,     0,     7,     1,     2,     2,     1,     1,
       2,     2,     3,     2,     2,     1,     4,     2,     3,     1,
       2,     3,     4,     3,     2,     1,     2,     3,     4,     3,
       2,     3,     0,     0,     4,     3,     0,     1,     0,     5,
       1,     1,     1,     1,     1,     4,     3,     3,     0,     1,
       1,     0,     4,     1,     0,     4,     1,     0,     2,     3,
       2,     2,     0,     0,     3,     4,     2,     1,     2,     3,
       1,     2,     1,     1,     2,     2,     0,     3,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     0,     2,
       2,     0,     5,     1,     1,     1,     2,     1,     1,     3,
       1,     0,     7,     0,     0,     8,     0,     5,     0,     5,
       0,     0,    10,     0,     7,     0,     7,     0,     0,     8,
       0,     7,     0,     7,     0,     7,     0,     7,     0,     7,
       0,     7,     0,     7,     0,     7,     0,     7,     0,     9,
       0,     9,     4,     4,     6,     0,     2,     0,     2,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     3,     3,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
     145,   159,   161,    86,    82,    79,    83,    84,    85,    80,
      81,   162,   151,   165,   166,    76,    87,   169,   170,   145,
     156,   149,   145,   168,   155,   157,   145,   163,   154,   167,
     158,     0,   160,    78,     0,   145,   150,   152,   153,   164,
       0,     0,     0,     0,     1,     0,   145,   146,    77,   171,
     172,   173,     3,     2,    88,     0,     0,    93,    94,    95,
      89,     0,     0,    90,     0,    86,    40,   147,     0,    87,
      41,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     5,     0,     0,
       9,    25,    19,    32,     0,     0,     0,     8,     0,    67,
      70,     0,    15,     0,     0,    30,    24,     0,     0,   147,
       0,    10,     0,    60,     0,     0,   103,   106,   108,     0,
       0,     0,   117,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    43,    42,    17,
       4,     6,   145,   147,    44,    13,    11,     0,    36,    66,
       0,    68,    38,     0,    26,    20,    62,     0,    72,    73,
      71,     7,    14,     0,    18,     0,    29,    23,    31,   148,
      12,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   120,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    35,    37,
      33,    69,    48,    27,    21,    63,   147,    61,    59,    74,
      75,    92,    28,    22,     0,    16,     0,   101,     0,     0,
       0,   110,   113,   115,     0,     0,   122,   126,   130,   134,
     124,   128,   132,   136,   138,   140,   142,   143,     0,    46,
      47,    34,    56,     0,    49,    50,    53,    62,     0,     0,
       0,   104,   107,   109,     0,     0,     0,   118,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      45,    39,     0,    62,    54,    64,    62,     0,     0,   111,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   144,     0,    57,    65,   102,     0,
       0,   114,   116,     0,   121,   123,   127,   131,   135,   125,
     129,   133,   137,     0,     0,    52,     0,    55,   105,   147,
     119,    98,     0,   100,     0,    97,     0,    58,     0,     0,
      96,   139,   141,   112,    99
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,    31,    46,    54,    96,    97,    98,    99,   100,   101,
     102,   158,   251,   210,   103,   212,   104,   105,   106,   155,
     253,   254,   282,   255,   306,   327,   107,   167,   217,   257,
     108,   109,   170,    32,    40,    33,    56,    60,   113,    61,
     334,   335,   112,   260,   185,   288,   186,   187,   264,   310,
     265,   266,   191,   292,   235,   269,   273,   270,   274,   271,
     275,   272,   276,   277,   278,    34,   118,    35,    36,    37,
      38,    39
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -292
static const short yypact[] =
{
     255,  -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,
    -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,   255,
    -292,  -292,   255,  -292,  -292,  -292,   255,  -292,  -292,  -292,
    -292,    22,  -292,  -292,    48,   255,  -292,  -292,  -292,  -292,
     273,     1,    14,    -4,  -292,    55,   255,  -292,  -292,  -292,
    -292,  -292,  -292,  -292,    29,    25,    34,  -292,  -292,  -292,
    -292,    65,   375,    36,   336,    41,  -292,   313,   235,    43,
    -292,    40,    45,    49,    68,    78,    86,    88,    89,    90,
      91,    92,    93,    97,    98,    99,   100,   101,   105,   106,
     107,   109,   110,   111,   112,    16,   116,   375,    39,    39,
    -292,  -292,  -292,   113,   178,   115,   125,  -292,    35,  -292,
     -15,   117,   127,   137,    16,  -292,  -292,    51,   143,   313,
      39,  -292,    35,  -292,   162,    16,  -292,  -292,  -292,    16,
      16,    16,  -292,    16,    16,    16,    16,    16,    16,    16,
      16,    16,    16,    16,    16,    16,    16,  -292,  -292,  -292,
    -292,  -292,   255,   313,  -292,  -292,  -292,   189,   181,  -292,
     178,  -292,  -292,    17,  -292,  -292,   -18,   152,   -15,   -15,
    -292,  -292,  -292,    25,  -292,    17,  -292,  -292,  -292,  -292,
    -292,   423,   165,    64,   163,    16,    16,    16,   164,   166,
     167,    16,  -292,   170,   171,   173,   174,   176,   177,   179,
     180,   182,   194,   168,   184,   195,   185,   204,  -292,  -292,
    -292,  -292,   439,  -292,  -292,  -292,   313,  -292,  -292,  -292,
    -292,  -292,  -292,  -292,    67,  -292,    16,  -292,   202,   222,
     225,  -292,  -292,  -292,   232,   234,  -292,  -292,  -292,  -292,
    -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,    16,   240,
    -292,  -292,  -292,   243,  -292,   241,    16,   -11,   242,    16,
     178,  -292,  -292,  -292,   178,   178,   178,  -292,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   245,
    -292,  -292,   246,   -11,  -292,  -292,   -11,   251,   178,  -292,
     260,   280,   178,   283,   285,   286,   287,   288,   289,   298,
     299,   303,   252,   304,  -292,   439,   244,  -292,  -292,   306,
     308,  -292,  -292,   309,  -292,  -292,  -292,  -292,  -292,  -292,
    -292,  -292,  -292,     4,     4,  -292,     4,  -292,  -292,   313,
    -292,   318,    10,  -292,   328,  -292,   338,  -292,   346,   360,
    -292,  -292,  -292,  -292,  -292
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
    -292,  -292,  -292,  -292,   277,  -292,  -292,   -64,   358,   -39,
     -41,  -292,  -292,  -292,  -292,  -292,   322,  -292,   -28,   -81,
    -292,   123,  -292,  -292,  -292,  -292,  -292,   175,  -240,  -292,
     -62,   -94,  -104,   -61,  -292,   389,  -292,   257,  -292,   -47,
    -291,   114,  -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,
    -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,
    -292,  -292,  -292,  -292,  -292,   -14,   -95,  -292,   -67,  -292,
    -292,  -292
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -92
static const short yytable[] =
{
     119,   110,   117,   110,   120,    41,   122,   110,    42,   215,
     159,   161,    43,   331,   147,   111,   215,   285,   156,   331,
     147,    47,    44,   116,   179,   115,   147,   147,   148,    57,
      58,    59,    53,   336,   148,   337,   110,   162,    67,   180,
     148,   148,   216,   110,   110,   147,   307,   168,   169,   216,
     111,    45,   119,    49,   149,   163,    67,    51,   207,   148,
     182,   147,   183,   110,   219,   220,   211,   165,   332,   164,
      50,   175,    67,   174,   147,   148,   177,   147,   176,    52,
     166,   165,    55,   164,   226,    62,   119,   259,   148,    63,
     152,   148,   153,   154,   123,   -91,   -43,   184,   -42,   110,
     124,   188,   189,   190,   125,   192,   193,   194,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,   205,   224,
     110,   258,   214,   126,   213,     3,     4,     5,     6,     7,
       8,     9,    10,   127,   223,    66,   222,    15,   206,    16,
     116,   128,   165,   129,   130,   131,   132,   133,   134,   119,
     256,   110,   135,   136,   137,   138,   139,   228,   229,   230,
     140,   141,   142,   234,   143,   144,   145,   146,   150,   181,
     171,   157,    65,     4,     5,     6,     7,     8,     9,    10,
     162,   172,    66,   177,    15,   214,    69,    70,     3,     4,
       5,     6,     7,     8,     9,    10,   173,   178,   208,   287,
      15,   209,    16,   289,   290,   291,   218,   293,   294,   295,
     296,   297,   298,   299,   300,   301,   302,   303,   223,    95,
     279,   225,   227,   231,   246,   232,   233,   309,   283,   236,
     237,   313,   238,   239,   338,   240,   241,   249,   242,   243,
     247,   244,    64,   256,   110,    65,     4,     5,     6,     7,
       8,     9,    10,   245,   248,    66,    67,    15,   250,    69,
      70,   261,   119,     1,     2,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,   262,    16,
      17,   263,    18,     3,     4,     5,     6,     7,     8,     9,
      10,   267,    95,   268,   280,   333,   333,    16,   333,   281,
     -51,   304,   326,   286,   333,   305,    19,   308,    20,    21,
      22,   323,    23,    24,    25,    26,   311,    27,    28,    29,
      30,     1,     2,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,   312,    16,    17,   314,
      18,   315,   316,   317,   318,   319,    65,     4,     5,     6,
       7,     8,     9,    10,   320,   321,    66,    67,    15,   322,
      69,    70,   328,   324,    19,   330,    20,   329,    22,   344,
      23,    24,    25,    26,   151,    27,    28,    29,    30,    57,
      58,    59,    64,   339,   341,    65,     4,     5,     6,     7,
       8,     9,    10,   114,   342,    66,    67,    15,    68,    69,
      70,    71,   343,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,   121,   160,   325,    48,
     221,   284,    95,    65,     4,     5,     6,     7,     8,     9,
      10,     0,     0,    66,     0,    15,   340,    69,    70,     3,
       4,     5,     6,     7,     8,     9,    10,     0,     0,    66,
       0,    15,     0,    16,    70,   252,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     114
};

static const short yycheck[] =
{
      67,    62,    64,    64,    68,    19,    68,    68,    22,    27,
     104,   105,    26,     9,    10,    62,    27,   257,    99,     9,
      10,    35,     0,    64,   119,    64,    10,    10,    24,     4,
       5,     6,    46,   324,    24,   326,    97,    55,    21,   120,
      24,    24,    60,   104,   105,    10,   286,    62,    63,    60,
      97,     3,   119,    52,    95,    20,    21,    61,   153,    24,
     124,    10,   124,   124,   168,   169,   160,   108,    64,   108,
      56,    20,    21,   114,    10,    24,   117,    10,   117,    24,
     108,   122,    53,   122,    20,    51,   153,    20,    24,    24,
      51,    24,    53,    54,    54,    59,    55,   125,    55,   160,
      55,   129,   130,   131,    55,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   181,
     181,   216,   163,    55,   163,    10,    11,    12,    13,    14,
      15,    16,    17,    55,   175,    20,   175,    22,   152,    24,
     181,    55,   183,    55,    55,    55,    55,    55,    55,   216,
     212,   212,    55,    55,    55,    55,    55,   185,   186,   187,
      55,    55,    55,   191,    55,    55,    55,    55,    52,     7,
      53,    58,    10,    11,    12,    13,    14,    15,    16,    17,
      55,    54,    20,   224,    22,   226,    24,    25,    10,    11,
      12,    13,    14,    15,    16,    17,    59,    54,     9,   260,
      22,    20,    24,   264,   265,   266,    54,   268,   269,   270,
     271,   272,   273,   274,   275,   276,   277,   278,   259,    57,
     248,    56,    59,    59,    56,    59,    59,   288,   256,    59,
      59,   292,    59,    59,   329,    59,    59,    52,    59,    59,
      56,    59,     7,   305,   305,    10,    11,    12,    13,    14,
      15,    16,    17,    59,    59,    20,    21,    22,    54,    24,
      25,    59,   329,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    56,    24,
      25,    56,    27,    10,    11,    12,    13,    14,    15,    16,
      17,    59,    57,    59,    54,   323,   324,    24,   326,    56,
      59,    56,    58,    61,   332,    59,    51,    56,    53,    54,
      55,    59,    57,    58,    59,    60,    56,    62,    63,    64,
      65,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    56,    24,    25,    56,
      27,    56,    56,    56,    56,    56,    10,    11,    12,    13,
      14,    15,    16,    17,    56,    56,    20,    21,    22,    56,
      24,    25,    56,    59,    51,    56,    53,    59,    55,     9,
      57,    58,    59,    60,    97,    62,    63,    64,    65,     4,
       5,     6,     7,    65,    56,    10,    11,    12,    13,    14,
      15,    16,    17,    57,    56,    20,    21,    22,    23,    24,
      25,    26,    56,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    68,   105,   305,    40,
     173,   256,    57,    10,    11,    12,    13,    14,    15,    16,
      17,    -1,    -1,    20,    -1,    22,   332,    24,    25,    10,
      11,    12,    13,    14,    15,    16,    17,    -1,    -1,    20,
      -1,    22,    -1,    24,    25,    26,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      57
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    24,    25,    27,    51,
      53,    54,    55,    57,    58,    59,    60,    62,    63,    64,
      65,    67,    99,   101,   131,   133,   134,   135,   136,   137,
     100,   131,   131,   131,     0,     3,    68,   131,   101,    52,
      56,    61,    24,   131,    69,    53,   102,     4,     5,     6,
     103,   105,    51,    24,     7,    10,    20,    21,    23,    24,
      25,    26,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    57,    70,    71,    72,    73,
      74,    75,    76,    80,    82,    83,    84,    92,    96,    97,
      99,   105,   108,   104,    57,    75,    76,    96,   132,   134,
      73,    74,    96,    54,    55,    55,    55,    55,    55,    55,
      55,    55,    55,    55,    55,    55,    55,    55,    55,    55,
      55,    55,    55,    55,    55,    55,    55,    10,    24,    76,
      52,    70,    51,    53,    54,    85,    85,    58,    77,    97,
      82,    97,    55,    20,    75,    76,    84,    93,    62,    63,
      98,    53,    54,    59,    76,    20,    75,    76,    54,   132,
      85,     7,    73,    96,    84,   110,   112,   113,    84,    84,
      84,   118,    84,    84,    84,    84,    84,    84,    84,    84,
      84,    84,    84,    84,    84,    84,   131,   132,     9,    20,
      79,    97,    81,    75,    76,    27,    60,    94,    54,    98,
      98,   103,    75,    76,    96,    56,    20,    59,    84,    84,
      84,    59,    59,    59,    84,   120,    59,    59,    59,    59,
      59,    59,    59,    59,    59,    59,    56,    56,    59,    52,
      54,    78,    26,    86,    87,    89,    96,    95,   132,    20,
     109,    59,    56,    56,   114,   116,   117,    59,    59,   121,
     123,   125,   127,   122,   124,   126,   128,   129,   130,    84,
      54,    56,    88,    84,    93,    94,    61,    99,   111,    99,
      99,    99,   119,    99,    99,    99,    99,    99,    99,    99,
      99,    99,    99,    99,    56,    59,    90,    94,    56,    99,
     115,    56,    56,    99,    56,    56,    56,    56,    56,    56,
      56,    56,    56,    59,    59,    87,    58,    91,    56,    59,
      56,     9,    64,    84,   106,   107,   106,   106,   132,    65,
     107,    56,    56,    56,     9
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrlab1


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL          goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY && yylen == 1)                          \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      yytoken = YYTRANSLATE (yychar);                           \
      YYPOPSTACK;                                               \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror ("syntax error: cannot back up");\
      YYERROR;                                                  \
    }                                                           \
while (0)

#define YYTERROR        1
#define YYERRCODE       256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

# define YYDSYMPRINT(Args)                      \
do {                                            \
  if (yydebug)                                  \
    yysymprint Args;                            \
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)            \
do {                                                            \
  if (yydebug)                                                  \
    {                                                           \
      YYFPRINTF (stderr, "%s ", Title);                         \
      yysymprint (stderr,                                       \
                  Token, Value);        \
      YYFPRINTF (stderr, "\n");                                 \
    }                                                           \
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylineno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylineno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (Rule);             \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;             /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack. Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        short *yyss1 = yyss;


        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow ("parser stack overflow",
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),

                    &yystacksize);

        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        short *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyoverflowlab;
        YYSTACK_RELOCATE (yyss);
        YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 3:
#line 204 "vtkParse.y"
    {
      data.ClassName = vtkstrdup(yyvsp[0].str);
      }
    break;

  case 11:
#line 214 "vtkParse.y"
    { output_function(); }
    break;

  case 12:
#line 215 "vtkParse.y"
    { output_function(); }
    break;

  case 13:
#line 216 "vtkParse.y"
    { legacySig(); output_function(); }
    break;

  case 17:
#line 222 "vtkParse.y"
    { preSig("~"); }
    break;

  case 18:
#line 223 "vtkParse.y"
    { preSig("virtual ~"); }
    break;

  case 20:
#line 226 "vtkParse.y"
    {
         currentFunction->ReturnType = yyvsp[-1].integer;
         }
    break;

  case 21:
#line 230 "vtkParse.y"
    {
         currentFunction->ReturnType = yyvsp[-2].integer;
         }
    break;

  case 22:
#line 234 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-2].integer;
         }
    break;

  case 23:
#line 239 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-1].integer;
         }
    break;

  case 24:
#line 244 "vtkParse.y"
    {
         preSig("virtual ");
         }
    break;

  case 25:
#line 250 "vtkParse.y"
    {
         output_function();
         }
    break;

  case 26:
#line 254 "vtkParse.y"
    {
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
         }
    break;

  case 27:
#line 259 "vtkParse.y"
    {
         currentFunction->ReturnType = yyvsp[-2].integer;
         output_function();
         }
    break;

  case 28:
#line 264 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-2].integer;
         output_function();
         }
    break;

  case 29:
#line 270 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
         }
    break;

  case 30:
#line 276 "vtkParse.y"
    {
         preSig("virtual ");
         output_function();
         }
    break;

  case 31:
#line 282 "vtkParse.y"
    {
      currentFunction->IsOperator = 1;
      fprintf(stderr,"   Converted operator\n");
    }
    break;

  case 32:
#line 287 "vtkParse.y"
    { postSig(")"); }
    break;

  case 33:
#line 287 "vtkParse.y"
    { postSig(";"); openSig = 0; }
    break;

  case 34:
#line 288 "vtkParse.y"
    {
      openSig = 1;
      currentFunction->Name = yyvsp[-3].str; 
      fprintf(stderr,"   Parsed func %s\n",yyvsp[-3].str); 
    }
    break;

  case 35:
#line 294 "vtkParse.y"
    { 
      postSig(") = 0;"); 
      currentFunction->Name = yyvsp[-2].str; 
      fprintf(stderr,"   Parsed func %s\n",yyvsp[-2].str); 
      currentFunction->IsPureVirtual = 1; 
      data.IsAbstract = 1;
    }
    break;

  case 37:
#line 302 "vtkParse.y"
    {postSig(" const");}
    break;

  case 38:
#line 304 "vtkParse.y"
    {postSig(" ("); }
    break;

  case 40:
#line 306 "vtkParse.y"
    {postSig("const ");}
    break;

  case 41:
#line 308 "vtkParse.y"
    {postSig("static ");}
    break;

  case 42:
#line 310 "vtkParse.y"
    {postSig(yyvsp[0].str);}
    break;

  case 43:
#line 310 "vtkParse.y"
    {postSig(yyvsp[0].str);}
    break;

  case 50:
#line 319 "vtkParse.y"
    { currentFunction->NumberOfArguments++;}
    break;

  case 51:
#line 320 "vtkParse.y"
    { currentFunction->NumberOfArguments++; postSig(", ");}
    break;

  case 53:
#line 323 "vtkParse.y"
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
        yyvsp[0].integer;}
    break;

  case 54:
#line 328 "vtkParse.y"
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 
        yyvsp[0].integer / 10000; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
        yyvsp[-1].integer + yyvsp[0].integer % 10000;
    }
    break;

  case 56:
#line 335 "vtkParse.y"
    { 
      postSig("void (*func)(void *) ");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 5000;
    }
    break;

  case 59:
#line 343 "vtkParse.y"
    {delSig();}
    break;

  case 60:
#line 343 "vtkParse.y"
    {delSig();}
    break;

  case 61:
#line 345 "vtkParse.y"
    { yyval.integer = yyvsp[0].integer; }
    break;

  case 62:
#line 353 "vtkParse.y"
    { yyval.integer = 0; }
    break;

  case 63:
#line 354 "vtkParse.y"
    { char temp[100]; sprintf(temp,"[%i]",yyvsp[0].integer); 
                   postSig(temp); }
    break;

  case 64:
#line 356 "vtkParse.y"
    { yyval.integer = 300 + 10000 * yyvsp[-2].integer + yyvsp[0].integer % 1000; }
    break;

  case 65:
#line 358 "vtkParse.y"
    { postSig("[]"); yyval.integer = 300 + yyvsp[0].integer % 1000; }
    break;

  case 66:
#line 360 "vtkParse.y"
    {yyval.integer = 1000 + yyvsp[0].integer;}
    break;

  case 67:
#line 361 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;}
    break;

  case 68:
#line 362 "vtkParse.y"
    {yyval.integer = 2000 + yyvsp[0].integer;}
    break;

  case 69:
#line 363 "vtkParse.y"
    {yyval.integer = 3000 + yyvsp[0].integer;}
    break;

  case 70:
#line 365 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;}
    break;

  case 71:
#line 367 "vtkParse.y"
    {yyval.integer = yyvsp[-1].integer + yyvsp[0].integer;}
    break;

  case 72:
#line 376 "vtkParse.y"
    { postSig("&"); yyval.integer = 100;}
    break;

  case 73:
#line 377 "vtkParse.y"
    { postSig("*"); yyval.integer = 300;}
    break;

  case 74:
#line 378 "vtkParse.y"
    { yyval.integer = 100 + yyvsp[0].integer;}
    break;

  case 75:
#line 379 "vtkParse.y"
    { yyval.integer = 400 + yyvsp[0].integer;}
    break;

  case 76:
#line 381 "vtkParse.y"
    {postSig("unsigned ");}
    break;

  case 77:
#line 382 "vtkParse.y"
    { yyval.integer = 10 + yyvsp[0].integer;}
    break;

  case 78:
#line 383 "vtkParse.y"
    { yyval.integer = yyvsp[0].integer;}
    break;

  case 79:
#line 386 "vtkParse.y"
    { postSig("float "); yyval.integer = 1;}
    break;

  case 80:
#line 387 "vtkParse.y"
    { postSig("void "); yyval.integer = 2;}
    break;

  case 81:
#line 388 "vtkParse.y"
    { postSig("char "); yyval.integer = 3;}
    break;

  case 82:
#line 389 "vtkParse.y"
    { postSig("int "); yyval.integer = 4;}
    break;

  case 83:
#line 390 "vtkParse.y"
    { postSig("short "); yyval.integer = 5;}
    break;

  case 84:
#line 391 "vtkParse.y"
    { postSig("long "); yyval.integer = 6;}
    break;

  case 85:
#line 392 "vtkParse.y"
    { postSig("double "); yyval.integer = 7;}
    break;

  case 86:
#line 393 "vtkParse.y"
    {       
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",yyvsp[0].str);
      postSig(ctmpid);
      yyval.integer = 8;}
    break;

  case 87:
#line 399 "vtkParse.y"
    { 
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",yyvsp[0].str);
      postSig(ctmpid);
      yyval.integer = 9; 
      currentFunction->ArgClasses[currentFunction->NumberOfArguments] =
        vtkstrdup(yyvsp[0].str); 
      /* store the string into the return value just in case we need it */
      /* this is a parsing hack because the first "type" parser will */
      /* possibly be ht ereturn type of the first argument */
      if ((!currentFunction->ReturnClass) && 
          (!currentFunction->NumberOfArguments)) 
        { 
        currentFunction->ReturnClass = vtkstrdup(yyvsp[0].str); 
        }
    }
    break;

  case 90:
#line 419 "vtkParse.y"
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    }
    break;

  case 91:
#line 424 "vtkParse.y"
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    }
    break;

  case 93:
#line 429 "vtkParse.y"
    {in_public = 1; in_protected = 0;}
    break;

  case 94:
#line 430 "vtkParse.y"
    {in_public = 0; in_protected = 0;}
    break;

  case 95:
#line 431 "vtkParse.y"
    {in_public = 0; in_protected = 1;}
    break;

  case 98:
#line 435 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;}
    break;

  case 99:
#line 436 "vtkParse.y"
    {yyval.integer = -1;}
    break;

  case 100:
#line 436 "vtkParse.y"
    {yyval.integer = -1;}
    break;

  case 101:
#line 440 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); }
    break;

  case 102:
#line 441 "vtkParse.y"
    {
   postSig(");");
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
   output_function();
   }
    break;

  case 103:
#line 451 "vtkParse.y"
    {postSig("Get");}
    break;

  case 104:
#line 451 "vtkParse.y"
    {postSig(" ();"); invertSig = 1;}
    break;

  case 105:
#line 453 "vtkParse.y"
    { 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = yyvsp[-1].integer;
   output_function();
   }
    break;

  case 106:
#line 460 "vtkParse.y"
    {preSig("void Set");}
    break;

  case 107:
#line 461 "vtkParse.y"
    {
   postSig(" (char *);"); 
   sprintf(temps,"Set%s",yyvsp[-1].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 303;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
   output_function();
   }
    break;

  case 108:
#line 471 "vtkParse.y"
    {preSig("char *Get");}
    break;

  case 109:
#line 472 "vtkParse.y"
    { 
   postSig(" ();");
   sprintf(temps,"Get%s",yyvsp[-1].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 303;
   output_function();
   }
    break;

  case 110:
#line 481 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); }
    break;

  case 111:
#line 482 "vtkParse.y"
    {postSig(");"); openSig = 0;}
    break;

  case 112:
#line 483 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sscanf (currentFunction->Signature, "%*s %*s (%s);", local);
   sprintf(temps,"Set%s",yyvsp[-7].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = yyvsp[-4].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%s Get%sMinValue ();",local,yyvsp[-7].str);
   sprintf(temps,"Get%sMinValue",yyvsp[-7].str);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = yyvsp[-4].integer;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%s Get%sMaxValue ();",local,yyvsp[-7].str);
   sprintf(temps,"Get%sMaxValue",yyvsp[-7].str);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = yyvsp[-4].integer;
   output_function();
   }
    break;

  case 113:
#line 513 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); }
    break;

  case 114:
#line 514 "vtkParse.y"
    { 
   postSig("*);");
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 2;
   output_function();
   }
    break;

  case 115:
#line 525 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); }
    break;

  case 116:
#line 526 "vtkParse.y"
    { 
   postSig("*);");
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 2;
   output_function();
   }
    break;

  case 117:
#line 536 "vtkParse.y"
    {postSig("*Get");}
    break;

  case 118:
#line 537 "vtkParse.y"
    {postSig(" ();"); invertSig = 1;}
    break;

  case 119:
#line 538 "vtkParse.y"
    { 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 309;
   output_function();
   }
    break;

  case 120:
#line 546 "vtkParse.y"
    {preSig("void "); postSig("On ();"); openSig = 0; }
    break;

  case 121:
#line 548 "vtkParse.y"
    { 
   sprintf(temps,"%sOn",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 2;
   output_function();
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void %sOff ();",yyvsp[-4].str); 
   sprintf(temps,"%sOff",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   output_function();
   }
    break;

  case 122:
#line 563 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 123:
#line 568 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s);",yyvsp[-4].str,
     local, local);
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
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
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 2;
   output_function();
   }
    break;

  case 124:
#line 593 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 125:
#line 598 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 2;
   output_function();
   }
    break;

  case 126:
#line 610 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 127:
#line 615 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s);",
     yyvsp[-4].str, local, local, local);
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
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
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 3;
   output_function();
   }
    break;

  case 128:
#line 642 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 129:
#line 647 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 3;
   output_function();
   }
    break;

  case 130:
#line 659 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 131:
#line 664 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s, %s);",
     yyvsp[-4].str, local, local, local, local);
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
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
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 4;
   output_function();
   }
    break;

  case 132:
#line 693 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 133:
#line 698 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 4;
   output_function();
   }
    break;

  case 134:
#line 710 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 135:
#line 715 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s, %s, %s, %s);",
     yyvsp[-4].str, local, local, local, local, local, local);
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
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
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 6;
   output_function();
   }
    break;

  case 136:
#line 748 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 137:
#line 753 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 6;
   output_function();
   }
    break;

  case 138:
#line 765 "vtkParse.y"
    {
      free (currentFunction->Signature);
      currentFunction->Signature = NULL;
      }
    break;

  case 139:
#line 770 "vtkParse.y"
    {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s [%i]);",yyvsp[-6].str,
      local, yyvsp[-1].integer);
     sprintf(temps,"Set%s",yyvsp[-6].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->ReturnType = 2;
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 300 + yyvsp[-3].integer;
     currentFunction->ArgCounts[0] = yyvsp[-1].integer;
     output_function();
   }
    break;

  case 140:
#line 783 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 141:
#line 788 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-6].str);
   sprintf(temps,"Get%s",yyvsp[-6].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yyvsp[-3].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = yyvsp[-1].integer;
   output_function();
   }
    break;

  case 142:
#line 800 "vtkParse.y"
    { 
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       yyvsp[-1].str);

     sprintf(temps,"Get%sCoordinate",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 309;
     currentFunction->ReturnClass = vtkstrdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double, double);",
       yyvsp[-1].str);
     sprintf(temps,"Set%s",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 2;
     currentFunction->ArgTypes[0] = 7;
     currentFunction->ArgCounts[0] = 0;
     currentFunction->ArgTypes[1] = 7;
     currentFunction->ArgCounts[1] = 0;
     currentFunction->ReturnType = 2;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double a[2]);",
       yyvsp[-1].str);
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 307;
     currentFunction->ArgCounts[0] = 2;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s ();", yyvsp[-1].str);
     sprintf(temps,"Get%s",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 307;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 2;
     output_function();
   }
    break;

  case 143:
#line 847 "vtkParse.y"
    { 
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       yyvsp[-1].str);

     sprintf(temps,"Get%sCoordinate",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 309;
     currentFunction->ReturnClass = vtkstrdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double, double, double);",
       yyvsp[-1].str);
     sprintf(temps,"Set%s",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 3;
     currentFunction->ArgTypes[0] = 7;
     currentFunction->ArgCounts[0] = 0;
     currentFunction->ArgTypes[1] = 7;
     currentFunction->ArgCounts[1] = 0;
     currentFunction->ArgTypes[2] = 7;
     currentFunction->ArgCounts[2] = 0;
     currentFunction->ReturnType = 2;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double a[3]);",
       yyvsp[-1].str);
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 307;
     currentFunction->ArgCounts[0] = 3;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s ();", yyvsp[-1].str);
     sprintf(temps,"Get%s",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 307;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 3;
     output_function();
   }
    break;

  case 144:
#line 896 "vtkParse.y"
    { 
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "const char *GetClassName ();");
   sprintf(temps,"GetClassName"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 1303;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,
           "int IsA (const char *name);");
   sprintf(temps,"IsA"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 1303;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 4;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "%s *NewInstance ();",
           yyvsp[-3].str);
   sprintf(temps,"NewInstance"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 309;
   currentFunction->ReturnClass = vtkstrdup(yyvsp[-3].str);
   output_function();

   if ( data.IsConcrete )
     {
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature, "%s *SafeDownCast (vtkObject* o);",
             yyvsp[-3].str);
     sprintf(temps,"SafeDownCast"); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 309;
     currentFunction->ArgCounts[0] = 1;
     currentFunction->ArgClasses[0] = vtkstrdup("vtkObject");
     currentFunction->ReturnType = 2309;
     currentFunction->ReturnClass = vtkstrdup(yyvsp[-3].str);
     output_function();
     }
   }
    break;


    }

/* Line 999 of yacc.c.  */
#line 2634 "vtkParse.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
        {
          YYSIZE_T yysize = 0;
          int yytype = YYTRANSLATE (yychar);
          char *yymsg;
          int yyx, yycount;

          yycount = 0;
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  */
          for (yyx = yyn < 0 ? -yyn : 0;
               yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
              yysize += yystrlen (yytname[yyx]) + 15, yycount++;
          yysize += yystrlen ("syntax error, unexpected ") + 1;
          yysize += yystrlen (yytname[yytype]);
          yymsg = (char *) YYSTACK_ALLOC (yysize);
          if (yymsg != 0)
            {
              char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
              yyp = yystpcpy (yyp, yytname[yytype]);

              if (yycount < 5)
                {
                  yycount = 0;
                  for (yyx = yyn < 0 ? -yyn : 0;
                       yyx < (int) (sizeof (yytname) / sizeof (char *));
                       yyx++)
                    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
                      {
                        const char *yyq = ! yycount ? ", expecting " : " or ";
                        yyp = yystpcpy (yyp, yyq);
                        yyp = yystpcpy (yyp, yytname[yyx]);
                        yycount++;
                      }
                }
              yyerror (yymsg);
              YYSTACK_FREE (yymsg);
            }
          else
            yyerror ("syntax error; also virtual memory exhausted");
        }
      else
#endif /* YYERROR_VERBOSE */
        yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
          /* Pop the error token.  */
          YYPOPSTACK;
          /* Pop the rest of the stack.  */
          while (yyss < yyssp)
            {
              YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
              yydestruct (yystos[*yyssp], yyvsp);
              YYPOPSTACK;
            }
          YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 964 "vtkParse.y"

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
  func->IsLegacy = 0;
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
  if (!fhint) 
    {
    return;
    }
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
  int i;

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

  /* reject multi-dimensional arrays from wrappers */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    if ((currentFunction->ArgTypes[i]%1000)/100 == 6 ||
        (currentFunction->ArgTypes[i]%1000)/100 == 9)
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
