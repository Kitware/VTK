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
     SetMacro = 283,
     GetMacro = 284,
     SetStringMacro = 285,
     GetStringMacro = 286,
     SetClampMacro = 287,
     SetObjectMacro = 288,
     SetReferenceCountedObjectMacro = 289,
     GetObjectMacro = 290,
     BooleanMacro = 291,
     SetVector2Macro = 292,
     SetVector3Macro = 293,
     SetVector4Macro = 294,
     SetVector6Macro = 295,
     GetVector2Macro = 296,
     GetVector3Macro = 297,
     GetVector4Macro = 298,
     GetVector6Macro = 299,
     SetVectorMacro = 300,
     GetVectorMacro = 301,
     ViewportCoordinateMacro = 302,
     WorldCoordinateMacro = 303,
     TypeMacro = 304
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
#define SetMacro 283
#define GetMacro 284
#define SetStringMacro 285
#define GetStringMacro 286
#define SetClampMacro 287
#define SetObjectMacro 288
#define SetReferenceCountedObjectMacro 289
#define GetObjectMacro 290
#define BooleanMacro 291
#define SetVector2Macro 292
#define SetVector3Macro 293
#define SetVector4Macro 294
#define SetVector6Macro 295
#define GetVector2Macro 296
#define GetVector3Macro 297
#define GetVector4Macro 298
#define GetVector6Macro 299
#define SetVectorMacro 300
#define GetVectorMacro 301
#define ViewportCoordinateMacro 302
#define WorldCoordinateMacro 303
#define TypeMacro 304




/* Copy the first part of user declarations.  */
#line 28 "vtkParse.y"

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
#line 125 "vtkParse.y"
typedef union YYSTYPE {
  char *str;
  int   integer;
  } YYSTYPE;
/* Line 191 of yacc.c.  */
#line 276 "vtkParse.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 288 "vtkParse.tab.c"

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
#define YYLAST   407

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  65
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  69
/* YYNRULES -- Number of rules. */
#define YYNRULES  163
/* YYNRULES -- Number of states. */
#define YYNSTATES  326

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   304

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    61,     2,
      56,    57,    62,     2,    58,    63,    64,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    52,    53,
       2,    55,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    59,     2,    60,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    50,     2,    51,    54,     2,     2,     2,
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
      45,    46,    47,    48,    49
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short yyprhs[] =
{
       0,     0,     3,     7,     8,    16,    18,    21,    24,    26,
      28,    31,    34,    36,    39,    43,    45,    48,    52,    57,
      61,    64,    65,    66,    72,    76,    81,    82,    84,    85,
      91,    93,    95,    97,    99,   101,   106,   110,   114,   115,
     117,   119,   120,   125,   127,   128,   133,   135,   136,   139,
     143,   146,   149,   150,   151,   155,   160,   163,   165,   168,
     172,   174,   177,   179,   181,   184,   187,   188,   192,   194,
     196,   198,   200,   202,   204,   206,   208,   210,   212,   213,
     216,   219,   220,   226,   228,   230,   232,   235,   237,   239,
     243,   245,   246,   254,   255,   256,   265,   266,   272,   273,
     279,   280,   281,   292,   293,   301,   302,   310,   311,   312,
     321,   322,   330,   331,   339,   340,   348,   349,   357,   358,
     366,   367,   375,   376,   384,   385,   393,   394,   402,   403,
     413,   414,   424,   429,   434,   441,   442,   445,   446,   449,
     451,   453,   455,   457,   459,   461,   463,   465,   467,   469,
     471,   473,   475,   477,   479,   481,   483,   485,   487,   489,
     491,   493,   497,   501
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short yyrhs[] =
{
      66,     0,    -1,   127,    67,   127,    -1,    -1,     3,    24,
      68,    98,    50,    69,    51,    -1,    70,    -1,    70,    69,
      -1,   101,    52,    -1,    88,    -1,    71,    -1,    23,    71,
      -1,   104,    53,    -1,   104,    -1,    54,    72,    -1,     7,
      54,    72,    -1,    72,    -1,    92,    72,    -1,    92,    20,
      72,    -1,     7,    92,    20,    72,    -1,     7,    92,    72,
      -1,     7,    72,    -1,    -1,    -1,    76,    73,    75,    74,
      81,    -1,    21,   128,    53,    -1,    76,    55,     9,    53,
      -1,    -1,    20,    -1,    -1,    80,    56,    77,    82,    57,
      -1,    20,    -1,    25,    -1,    24,    -1,    10,    -1,    53,
      -1,    50,   127,    51,    53,    -1,    50,   127,    51,    -1,
      52,   128,    53,    -1,    -1,    83,    -1,    85,    -1,    -1,
      85,    84,    58,    83,    -1,    92,    -1,    -1,    92,    89,
      86,    87,    -1,    26,    -1,    -1,    55,   102,    -1,    92,
      89,    53,    -1,    26,    53,    -1,    80,    90,    -1,    -1,
      -1,    27,    91,    90,    -1,    59,   128,    60,    90,    -1,
      78,    93,    -1,    93,    -1,    79,    93,    -1,    79,    78,
      93,    -1,    95,    -1,    95,    94,    -1,    61,    -1,    62,
      -1,    61,    94,    -1,    62,    94,    -1,    -1,    22,    96,
      97,    -1,    97,    -1,    12,    -1,    16,    -1,    17,    -1,
      11,    -1,    13,    -1,    14,    -1,    15,    -1,    10,    -1,
      24,    -1,    -1,    52,    99,    -1,   101,    24,    -1,    -1,
     101,    24,   100,    58,    99,    -1,     4,    -1,     5,    -1,
       6,    -1,    63,   103,    -1,   103,    -1,     9,    -1,     9,
      64,     9,    -1,    80,    -1,    -1,    28,    56,    80,    58,
     105,    95,    57,    -1,    -1,    -1,    29,    56,   106,    80,
      58,   107,    95,    57,    -1,    -1,    30,    56,   108,    80,
      57,    -1,    -1,    31,    56,   109,    80,    57,    -1,    -1,
      -1,    32,    56,    80,    58,   110,    95,   111,    58,   128,
      57,    -1,    -1,    33,    56,    80,    58,   112,    95,    57,
      -1,    -1,    34,    56,    80,    58,   113,    95,    57,    -1,
      -1,    -1,    35,    56,   114,    80,    58,   115,    95,    57,
      -1,    -1,    36,    56,    80,   116,    58,    95,    57,    -1,
      -1,    37,    56,    80,    58,   117,    95,    57,    -1,    -1,
      41,    56,    80,    58,   118,    95,    57,    -1,    -1,    38,
      56,    80,    58,   119,    95,    57,    -1,    -1,    42,    56,
      80,    58,   120,    95,    57,    -1,    -1,    39,    56,    80,
      58,   121,    95,    57,    -1,    -1,    43,    56,    80,    58,
     122,    95,    57,    -1,    -1,    40,    56,    80,    58,   123,
      95,    57,    -1,    -1,    44,    56,    80,    58,   124,    95,
      57,    -1,    -1,    45,    56,    80,    58,   125,    95,    58,
     102,    57,    -1,    -1,    46,    56,    80,    58,   126,    95,
      58,   102,    57,    -1,    47,    56,    80,    57,    -1,    48,
      56,    80,    57,    -1,    49,    56,    80,    58,    80,    57,
      -1,    -1,   129,   127,    -1,    -1,   130,   128,    -1,    53,
      -1,   130,    -1,    19,    -1,   131,    -1,   132,    -1,    62,
      -1,    55,    -1,    52,    -1,    58,    -1,    64,    -1,     8,
      -1,    95,    -1,     9,    -1,    18,    -1,    61,    -1,   133,
      -1,    20,    -1,    21,    -1,    63,    -1,    54,    -1,    25,
      -1,    27,    -1,    50,   127,    51,    -1,    56,   127,    57,
      -1,    59,   127,    60,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   184,   184,   187,   186,   192,   192,   194,   194,   195,
     195,   195,   195,   197,   198,   199,   203,   208,   213,   219,
     225,   231,   231,   231,   238,   243,   252,   252,   254,   254,
     256,   258,   260,   260,   262,   263,   264,   265,   267,   267,
     269,   270,   270,   272,   278,   277,   284,   291,   291,   293,
     293,   295,   303,   304,   304,   307,   310,   311,   312,   313,
     315,   316,   326,   327,   328,   329,   331,   331,   333,   336,
     337,   338,   339,   340,   341,   342,   343,   348,   366,   366,
     368,   374,   373,   379,   380,   381,   383,   383,   385,   386,
     386,   390,   389,   401,   401,   401,   410,   410,   421,   421,
     431,   432,   430,   463,   462,   475,   474,   486,   487,   486,
     496,   495,   513,   512,   543,   542,   560,   559,   592,   591,
     609,   608,   643,   642,   660,   659,   698,   697,   715,   714,
     733,   732,   749,   796,   845,   900,   900,   901,   901,   903,
     903,   905,   905,   905,   905,   905,   905,   905,   905,   906,
     906,   906,   906,   906,   906,   906,   907,   907,   907,   907,
     907,   909,   910,   911
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
  "SetMacro", "GetMacro", "SetStringMacro", "GetStringMacro", 
  "SetClampMacro", "SetObjectMacro", "SetReferenceCountedObjectMacro", 
  "GetObjectMacro", "BooleanMacro", "SetVector2Macro", "SetVector3Macro", 
  "SetVector4Macro", "SetVector6Macro", "GetVector2Macro", 
  "GetVector3Macro", "GetVector4Macro", "GetVector6Macro", 
  "SetVectorMacro", "GetVectorMacro", "ViewportCoordinateMacro", 
  "WorldCoordinateMacro", "TypeMacro", "'{'", "'}'", "':'", "';'", "'~'", 
  "'='", "'('", "')'", "','", "'['", "']'", "'&'", "'*'", "'-'", "'.'", 
  "$accept", "strt", "class_def", "@1", "class_def_body", 
  "class_def_item", "function", "func", "@2", "@3", "maybe_const", 
  "func_beg", "@4", "const_mod", "static_mod", "any_id", "func_end", 
  "args_list", "more_args", "@5", "arg", "@6", "opt_var_assign", "var", 
  "var_id", "var_array", "@7", "type", "type_red1", "type_indirection", 
  "type_red2", "@8", "type_primitive", "optional_scope", "scope_list", 
  "@9", "scope_type", "float_num", "float_prim", "macro", "@10", "@11", 
  "@12", "@13", "@14", "@15", "@16", "@17", "@18", "@19", "@20", "@21", 
  "@22", "@23", "@24", "@25", "@26", "@27", "@28", "@29", "@30", "@31", 
  "maybe_other", "maybe_other_no_semi", "other_stuff", 
  "other_stuff_no_semi", "braces", "parens", "brackets", 0
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
     123,   125,    58,    59,   126,    61,    40,    41,    44,    91,
      93,    38,    42,    45,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    65,    66,    68,    67,    69,    69,    70,    70,    70,
      70,    70,    70,    71,    71,    71,    71,    71,    71,    71,
      71,    73,    74,    72,    72,    72,    75,    75,    77,    76,
      78,    79,    80,    80,    81,    81,    81,    81,    82,    82,
      83,    84,    83,    85,    86,    85,    85,    87,    87,    88,
      88,    89,    90,    91,    90,    90,    92,    92,    92,    92,
      93,    93,    94,    94,    94,    94,    96,    95,    95,    97,
      97,    97,    97,    97,    97,    97,    97,    97,    98,    98,
      99,   100,    99,   101,   101,   101,   102,   102,   103,   103,
     103,   105,   104,   106,   107,   104,   108,   104,   109,   104,
     110,   111,   104,   112,   104,   113,   104,   114,   115,   104,
     116,   104,   117,   104,   118,   104,   119,   104,   120,   104,
     121,   104,   122,   104,   123,   104,   124,   104,   125,   104,
     126,   104,   104,   104,   104,   127,   127,   128,   128,   129,
     129,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   131,   132,   133
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     3,     0,     7,     1,     2,     2,     1,     1,
       2,     2,     1,     2,     3,     1,     2,     3,     4,     3,
       2,     0,     0,     5,     3,     4,     0,     1,     0,     5,
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
     135,   149,   151,    76,    72,    69,    73,    74,    75,    70,
      71,   152,   141,   155,   156,    66,    77,   159,   160,   135,
     146,   139,   158,   145,   135,   147,   135,   153,   144,   157,
     148,     0,   150,    68,     0,   135,   140,   142,   143,   154,
       0,     0,     0,     0,     1,     0,   135,   136,    67,   161,
     162,   163,     3,     2,    78,     0,     0,    83,    84,    85,
      79,     0,     0,    80,     0,    76,    30,   137,     0,    77,
      31,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     5,     9,    15,    21,
       0,     0,     0,     8,     0,    57,    60,     0,    12,     0,
       0,    20,     0,     0,   137,    10,     0,    50,     0,    93,
      96,    98,     0,     0,     0,   107,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      33,    32,    13,     4,     6,     0,    26,    56,     0,    58,
      28,     0,    16,    52,     0,    62,    63,    61,     7,    11,
       0,    14,     0,    19,    24,   138,     0,     0,     0,     0,
       0,     0,     0,     0,   110,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    27,
      22,    59,    38,    17,    53,   137,    51,    49,    64,    65,
      82,    18,    91,     0,     0,     0,   100,   103,   105,     0,
       0,   112,   116,   120,   124,   114,   118,   122,   126,   128,
     130,   132,   133,     0,    25,     0,    46,     0,    39,    40,
      43,    52,     0,     0,    94,    97,    99,     0,     0,     0,
     108,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   135,   137,    34,    23,    29,     0,    52,
      44,    54,    52,     0,     0,   101,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     134,     0,     0,     0,    47,    55,    92,     0,     0,   104,
     106,     0,   111,   113,   117,   121,   125,   115,   119,   123,
     127,     0,     0,    36,    37,    42,     0,    45,    95,   137,
     109,    88,     0,    90,     0,    87,     0,    35,    48,     0,
       0,    86,   129,   131,   102,    89
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,    31,    46,    54,    95,    96,    97,    98,   146,   225,
     190,    99,   192,   100,   101,   102,   256,   227,   228,   258,
     229,   284,   307,   103,   154,   196,   231,   104,   105,   157,
      32,    40,    33,    56,    60,   109,    61,   314,   315,   108,
     233,   167,   264,   168,   169,   237,   288,   238,   239,   173,
     268,   210,   242,   246,   243,   247,   244,   248,   245,   249,
     250,   251,    34,   113,    35,    36,    37,    38,    39
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -298
static const short yypact[] =
{
     235,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,
    -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,   235,
    -298,  -298,  -298,  -298,   235,  -298,   235,  -298,  -298,  -298,
    -298,     4,  -298,  -298,     7,   235,  -298,  -298,  -298,  -298,
     216,     2,    13,    12,  -298,    57,   235,  -298,  -298,  -298,
    -298,  -298,  -298,  -298,    34,   121,    47,  -298,  -298,  -298,
    -298,    77,   316,    49,    98,    60,  -298,   255,    78,    65,
    -298,    52,    73,    80,    91,    94,   108,   109,   111,   113,
     117,   127,   134,   135,   136,   140,   155,   156,   158,   159,
     160,   161,   162,   165,   114,    82,   316,  -298,  -298,    79,
     164,   129,   167,  -298,    58,  -298,   -11,   102,   172,   176,
     114,  -298,    63,   183,   255,  -298,    58,  -298,    -8,  -298,
    -298,  -298,    -8,    -8,    -8,  -298,    -8,    -8,    -8,    -8,
      -8,    -8,    -8,    -8,    -8,    -8,    -8,    -8,    -8,    -8,
    -298,  -298,  -298,  -298,  -298,   228,   218,  -298,   164,  -298,
    -298,   114,  -298,    -1,   188,   -11,   -11,  -298,  -298,  -298,
     121,  -298,   114,  -298,  -298,  -298,   200,    -8,    -8,    -8,
     203,   220,   223,    -8,  -298,   225,   226,   234,   237,   242,
     243,   244,   245,   246,   248,   229,   251,   254,   262,  -298,
    -298,  -298,   146,  -298,  -298,   255,  -298,  -298,  -298,  -298,
    -298,  -298,  -298,   266,   268,   277,  -298,  -298,  -298,   285,
     308,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,
    -298,  -298,  -298,    -8,  -298,   132,  -298,   278,  -298,   309,
      -8,     5,   311,   164,  -298,  -298,  -298,   164,   164,   164,
    -298,   164,   164,   164,   164,   164,   164,   164,   164,   164,
     164,   164,   312,   235,   255,  -298,  -298,  -298,   310,     5,
    -298,  -298,     5,   315,   164,  -298,   317,   318,   164,   319,
     320,   321,   322,   323,   324,   325,   326,   327,   328,   329,
    -298,   334,   335,   146,   336,  -298,  -298,   332,   337,  -298,
    -298,   333,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,
    -298,     3,     3,   339,  -298,  -298,     3,  -298,  -298,   255,
    -298,   330,    53,  -298,   340,  -298,   341,  -298,  -298,   342,
     364,  -298,  -298,  -298,  -298,  -298
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
    -298,  -298,  -298,  -298,   297,  -298,   338,   -45,  -298,  -298,
    -298,  -298,  -298,   295,  -298,   -93,  -298,  -298,   118,  -298,
    -298,  -298,  -298,  -298,   170,  -208,  -298,   -61,   -79,   -95,
     -44,  -298,   362,  -298,   247,  -298,   -48,  -297,    92,  -298,
    -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,
    -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,
    -298,  -298,   -18,   -99,  -298,   -67,  -298,  -298,  -298
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -82
static const short yytable[] =
{
     114,    41,   140,   112,    44,   316,    42,   116,    43,   318,
      45,   153,   311,   140,   107,   165,   141,    47,   106,   111,
     106,   147,   149,   261,   106,   166,   194,   141,    53,   170,
     171,   172,   194,   174,   175,   176,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,   114,   107,   142,
     155,   156,   106,    49,   285,   150,   106,   106,   195,   152,
     198,   199,   311,   140,   195,   161,   312,   163,   140,   191,
      50,   152,    51,   140,   203,   204,   205,   141,   151,    67,
     209,    52,   141,   162,    67,    64,    55,   141,    65,     4,
       5,     6,     7,     8,     9,    10,   232,    62,    66,    67,
      15,    63,    69,    70,   106,   117,   193,   -81,    65,     4,
       5,     6,     7,     8,     9,    10,   -33,   201,    66,    67,
      15,   -32,    69,    70,   140,    57,    58,    59,   114,   118,
     252,   230,    94,   143,   145,    67,   119,   259,   141,     3,
       4,     5,     6,     7,     8,     9,    10,   120,   106,    66,
     121,    15,   110,    16,   158,   282,     3,     4,     5,     6,
       7,     8,     9,    10,   122,   123,    66,   124,    15,   125,
      16,    70,   226,   126,     3,     4,     5,     6,     7,     8,
       9,    10,   253,   127,   254,   255,    15,   114,    16,   263,
     128,   129,   130,   265,   266,   267,   131,   269,   270,   271,
     272,   273,   274,   275,   276,   277,   278,   279,   313,   313,
     319,   132,   133,   313,   134,   135,   136,   137,   138,   313,
     287,   139,   230,   150,   291,   159,     3,     4,     5,     6,
       7,     8,     9,    10,   160,   281,   164,   188,   189,   106,
      16,   197,   114,     1,     2,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,   202,    16,
      17,   206,    18,     1,     2,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,   207,    16,
      17,   208,    18,   211,   212,    19,   221,    20,    21,    22,
      23,    24,   213,    25,    26,   214,    27,    28,    29,    30,
     215,   216,   217,   218,   219,    19,   220,    20,   222,    22,
      23,    24,   223,    25,    26,   224,    27,    28,    29,    30,
      57,    58,    59,    64,   234,   235,    65,     4,     5,     6,
       7,     8,     9,    10,   236,   257,    66,    67,    15,    68,
      69,    70,    71,   240,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,   241,   -41,   283,   280,
      94,   262,   286,   325,   289,   290,   292,   293,   294,   295,
     296,   297,   298,   299,   300,   303,   301,   302,   304,   308,
     310,   306,   317,   144,   320,   309,   148,   322,   323,   324,
     260,   305,    48,     0,   321,     0,   115,   200
};

static const short yycheck[] =
{
      67,    19,    10,    64,     0,   302,    24,    68,    26,   306,
       3,   104,     9,    10,    62,   114,    24,    35,    62,    64,
      64,   100,   101,   231,    68,   118,    27,    24,    46,   122,
     123,   124,    27,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   114,    96,    94,
      61,    62,    96,    51,   262,    56,   100,   101,    59,   104,
     155,   156,     9,    10,    59,   110,    63,   112,    10,   148,
      57,   116,    60,    10,   167,   168,   169,    24,    20,    21,
     173,    24,    24,    20,    21,     7,    52,    24,    10,    11,
      12,    13,    14,    15,    16,    17,   195,    50,    20,    21,
      22,    24,    24,    25,   148,    53,   151,    58,    10,    11,
      12,    13,    14,    15,    16,    17,    56,   162,    20,    21,
      22,    56,    24,    25,    10,     4,     5,     6,   195,    56,
     223,   192,    54,    51,    55,    21,    56,   230,    24,    10,
      11,    12,    13,    14,    15,    16,    17,    56,   192,    20,
      56,    22,    54,    24,    52,   254,    10,    11,    12,    13,
      14,    15,    16,    17,    56,    56,    20,    56,    22,    56,
      24,    25,    26,    56,    10,    11,    12,    13,    14,    15,
      16,    17,    50,    56,    52,    53,    22,   254,    24,   233,
      56,    56,    56,   237,   238,   239,    56,   241,   242,   243,
     244,   245,   246,   247,   248,   249,   250,   251,   301,   302,
     309,    56,    56,   306,    56,    56,    56,    56,    56,   312,
     264,    56,   283,    56,   268,    53,    10,    11,    12,    13,
      14,    15,    16,    17,    58,   253,    53,     9,    20,   283,
      24,    53,   309,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    58,    24,
      25,    58,    27,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    58,    24,
      25,    58,    27,    58,    58,    50,    57,    52,    53,    54,
      55,    56,    58,    58,    59,    58,    61,    62,    63,    64,
      58,    58,    58,    58,    58,    50,    58,    52,    57,    54,
      55,    56,    58,    58,    59,    53,    61,    62,    63,    64,
       4,     5,     6,     7,    58,    57,    10,    11,    12,    13,
      14,    15,    16,    17,    57,    57,    20,    21,    22,    23,
      24,    25,    26,    58,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    58,    58,    58,    57,
      54,    60,    57,     9,    57,    57,    57,    57,    57,    57,
      57,    57,    57,    57,    57,    51,    58,    58,    53,    57,
      57,    55,    53,    96,    64,    58,   101,    57,    57,    57,
     230,   283,    40,    -1,   312,    -1,    68,   160
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    24,    25,    27,    50,
      52,    53,    54,    55,    56,    58,    59,    61,    62,    63,
      64,    66,    95,    97,   127,   129,   130,   131,   132,   133,
      96,   127,   127,   127,     0,     3,    67,   127,    97,    51,
      57,    60,    24,   127,    68,    52,    98,     4,     5,     6,
      99,   101,    50,    24,     7,    10,    20,    21,    23,    24,
      25,    26,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    54,    69,    70,    71,    72,    76,
      78,    79,    80,    88,    92,    93,    95,   101,   104,   100,
      54,    72,    92,   128,   130,    71,    92,    53,    56,    56,
      56,    56,    56,    56,    56,    56,    56,    56,    56,    56,
      56,    56,    56,    56,    56,    56,    56,    56,    56,    56,
      10,    24,    72,    51,    69,    55,    73,    93,    78,    93,
      56,    20,    72,    80,    89,    61,    62,    94,    52,    53,
      58,    72,    20,    72,    53,   128,    80,   106,   108,   109,
      80,    80,    80,   114,    80,    80,    80,    80,    80,    80,
      80,    80,    80,    80,    80,    80,    80,    80,     9,    20,
      75,    93,    77,    72,    27,    59,    90,    53,    94,    94,
      99,    72,    58,    80,    80,    80,    58,    58,    58,    80,
     116,    58,    58,    58,    58,    58,    58,    58,    58,    58,
      58,    57,    57,    58,    53,    74,    26,    82,    83,    85,
      92,    91,   128,   105,    58,    57,    57,   110,   112,   113,
      58,    58,   117,   119,   121,   123,   118,   120,   122,   124,
     125,   126,    80,    50,    52,    53,    81,    57,    84,    80,
      89,    90,    60,    95,   107,    95,    95,    95,   115,    95,
      95,    95,    95,    95,    95,    95,    95,    95,    95,    95,
      57,   127,   128,    58,    86,    90,    57,    95,   111,    57,
      57,    95,    57,    57,    57,    57,    57,    57,    57,    57,
      57,    58,    58,    51,    53,    83,    55,    87,    57,    58,
      57,     9,    63,    80,   102,   103,   102,    53,   102,   128,
      64,   103,    57,    57,    57,     9
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
  
  register short yystate;
  register short yyn;
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
#line 187 "vtkParse.y"
    {
      data.ClassName = vtkstrdup(yyvsp[0].str);
      }
    break;

  case 13:
#line 197 "vtkParse.y"
    { preSig("~"); output_function(); }
    break;

  case 14:
#line 198 "vtkParse.y"
    { preSig("virtual ~"); output_function(); }
    break;

  case 15:
#line 200 "vtkParse.y"
    {
         output_function();
         }
    break;

  case 16:
#line 204 "vtkParse.y"
    {
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
         }
    break;

  case 17:
#line 209 "vtkParse.y"
    {
         currentFunction->ReturnType = yyvsp[-2].integer;
         output_function();
         }
    break;

  case 18:
#line 214 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-2].integer;
         output_function();
         }
    break;

  case 19:
#line 220 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
         }
    break;

  case 20:
#line 226 "vtkParse.y"
    {
         preSig("virtual ");
         output_function();
         }
    break;

  case 21:
#line 231 "vtkParse.y"
    { postSig(")"); }
    break;

  case 22:
#line 231 "vtkParse.y"
    { postSig(";"); openSig = 0; }
    break;

  case 23:
#line 233 "vtkParse.y"
    {
      openSig = 1;
      currentFunction->Name = yyvsp[-4].str; 
      fprintf(stderr,"   Parsed func %s\n",yyvsp[-4].str); 
    }
    break;

  case 24:
#line 239 "vtkParse.y"
    { 
      currentFunction->IsOperator = 1; 
      fprintf(stderr,"   Converted operator\n"); 
    }
    break;

  case 25:
#line 244 "vtkParse.y"
    { 
      postSig(") = 0;"); 
      currentFunction->Name = yyvsp[-3].str; 
      fprintf(stderr,"   Parsed func %s\n",yyvsp[-3].str); 
      currentFunction->IsPureVirtual = 1; 
      data.IsAbstract = 1;
    }
    break;

  case 27:
#line 252 "vtkParse.y"
    {postSig(" const");}
    break;

  case 28:
#line 254 "vtkParse.y"
    {postSig(" ("); }
    break;

  case 30:
#line 256 "vtkParse.y"
    {postSig("const ");}
    break;

  case 31:
#line 258 "vtkParse.y"
    {postSig("static ");}
    break;

  case 32:
#line 260 "vtkParse.y"
    {postSig(yyvsp[0].str);}
    break;

  case 33:
#line 260 "vtkParse.y"
    {postSig(yyvsp[0].str);}
    break;

  case 40:
#line 269 "vtkParse.y"
    { currentFunction->NumberOfArguments++;}
    break;

  case 41:
#line 270 "vtkParse.y"
    { currentFunction->NumberOfArguments++; postSig(", ");}
    break;

  case 43:
#line 273 "vtkParse.y"
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
        yyvsp[0].integer;}
    break;

  case 44:
#line 278 "vtkParse.y"
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 
        yyvsp[0].integer / 10000; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
        yyvsp[-1].integer + yyvsp[0].integer % 10000;
    }
    break;

  case 46:
#line 285 "vtkParse.y"
    { 
      postSig("void (*func)(void *) ");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 5000;
    }
    break;

  case 49:
#line 293 "vtkParse.y"
    {delSig();}
    break;

  case 50:
#line 293 "vtkParse.y"
    {delSig();}
    break;

  case 51:
#line 295 "vtkParse.y"
    { yyval.integer = yyvsp[0].integer; }
    break;

  case 52:
#line 303 "vtkParse.y"
    { yyval.integer = 0; }
    break;

  case 53:
#line 304 "vtkParse.y"
    { char temp[100]; sprintf(temp,"[%i]",yyvsp[0].integer); 
                   postSig(temp); }
    break;

  case 54:
#line 306 "vtkParse.y"
    { yyval.integer = 300 + 10000 * yyvsp[-2].integer + yyvsp[0].integer % 1000; }
    break;

  case 55:
#line 308 "vtkParse.y"
    { postSig("[]"); yyval.integer = 300 + yyvsp[0].integer % 1000; }
    break;

  case 56:
#line 310 "vtkParse.y"
    {yyval.integer = 1000 + yyvsp[0].integer;}
    break;

  case 57:
#line 311 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;}
    break;

  case 58:
#line 312 "vtkParse.y"
    {yyval.integer = 2000 + yyvsp[0].integer;}
    break;

  case 59:
#line 313 "vtkParse.y"
    {yyval.integer = 3000 + yyvsp[0].integer;}
    break;

  case 60:
#line 315 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;}
    break;

  case 61:
#line 317 "vtkParse.y"
    {yyval.integer = yyvsp[-1].integer + yyvsp[0].integer;}
    break;

  case 62:
#line 326 "vtkParse.y"
    { postSig("&"); yyval.integer = 100;}
    break;

  case 63:
#line 327 "vtkParse.y"
    { postSig("*"); yyval.integer = 300;}
    break;

  case 64:
#line 328 "vtkParse.y"
    { yyval.integer = 100 + yyvsp[0].integer;}
    break;

  case 65:
#line 329 "vtkParse.y"
    { yyval.integer = 400 + yyvsp[0].integer;}
    break;

  case 66:
#line 331 "vtkParse.y"
    {postSig("unsigned ");}
    break;

  case 67:
#line 332 "vtkParse.y"
    { yyval.integer = 10 + yyvsp[0].integer;}
    break;

  case 68:
#line 333 "vtkParse.y"
    { yyval.integer = yyvsp[0].integer;}
    break;

  case 69:
#line 336 "vtkParse.y"
    { postSig("float "); yyval.integer = 1;}
    break;

  case 70:
#line 337 "vtkParse.y"
    { postSig("void "); yyval.integer = 2;}
    break;

  case 71:
#line 338 "vtkParse.y"
    { postSig("char "); yyval.integer = 3;}
    break;

  case 72:
#line 339 "vtkParse.y"
    { postSig("int "); yyval.integer = 4;}
    break;

  case 73:
#line 340 "vtkParse.y"
    { postSig("short "); yyval.integer = 5;}
    break;

  case 74:
#line 341 "vtkParse.y"
    { postSig("long "); yyval.integer = 6;}
    break;

  case 75:
#line 342 "vtkParse.y"
    { postSig("double "); yyval.integer = 7;}
    break;

  case 76:
#line 343 "vtkParse.y"
    {       
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",yyvsp[0].str);
      postSig(ctmpid);
      yyval.integer = 8;}
    break;

  case 77:
#line 349 "vtkParse.y"
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

  case 80:
#line 369 "vtkParse.y"
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    }
    break;

  case 81:
#line 374 "vtkParse.y"
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    }
    break;

  case 83:
#line 379 "vtkParse.y"
    {in_public = 1; in_protected = 0;}
    break;

  case 84:
#line 380 "vtkParse.y"
    {in_public = 0; in_protected = 0;}
    break;

  case 85:
#line 381 "vtkParse.y"
    {in_public = 0; in_protected = 1;}
    break;

  case 88:
#line 385 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;}
    break;

  case 89:
#line 386 "vtkParse.y"
    {yyval.integer = -1;}
    break;

  case 90:
#line 386 "vtkParse.y"
    {yyval.integer = -1;}
    break;

  case 91:
#line 390 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); }
    break;

  case 92:
#line 391 "vtkParse.y"
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

  case 93:
#line 401 "vtkParse.y"
    {postSig("Get");}
    break;

  case 94:
#line 401 "vtkParse.y"
    {postSig(" ();"); invertSig = 1;}
    break;

  case 95:
#line 403 "vtkParse.y"
    { 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = yyvsp[-1].integer;
   output_function();
   }
    break;

  case 96:
#line 410 "vtkParse.y"
    {preSig("void Set");}
    break;

  case 97:
#line 411 "vtkParse.y"
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

  case 98:
#line 421 "vtkParse.y"
    {preSig("char *Get");}
    break;

  case 99:
#line 422 "vtkParse.y"
    { 
   postSig(" ();");
   sprintf(temps,"Get%s",yyvsp[-1].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 303;
   output_function();
   }
    break;

  case 100:
#line 431 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); }
    break;

  case 101:
#line 432 "vtkParse.y"
    {postSig(");"); openSig = 0;}
    break;

  case 102:
#line 433 "vtkParse.y"
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

  case 103:
#line 463 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); }
    break;

  case 104:
#line 464 "vtkParse.y"
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

  case 105:
#line 475 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); }
    break;

  case 106:
#line 476 "vtkParse.y"
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

  case 107:
#line 486 "vtkParse.y"
    {postSig("*Get");}
    break;

  case 108:
#line 487 "vtkParse.y"
    {postSig(" ();"); invertSig = 1;}
    break;

  case 109:
#line 488 "vtkParse.y"
    { 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 309;
   output_function();
   }
    break;

  case 110:
#line 496 "vtkParse.y"
    {preSig("void "); postSig("On ();"); openSig = 0; }
    break;

  case 111:
#line 498 "vtkParse.y"
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

  case 112:
#line 513 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 113:
#line 518 "vtkParse.y"
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

  case 114:
#line 543 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 115:
#line 548 "vtkParse.y"
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

  case 116:
#line 560 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 117:
#line 565 "vtkParse.y"
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

  case 118:
#line 592 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 119:
#line 597 "vtkParse.y"
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

  case 120:
#line 609 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 121:
#line 614 "vtkParse.y"
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

  case 122:
#line 643 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 123:
#line 648 "vtkParse.y"
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

  case 124:
#line 660 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 125:
#line 665 "vtkParse.y"
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

  case 126:
#line 698 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 127:
#line 703 "vtkParse.y"
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

  case 128:
#line 715 "vtkParse.y"
    {
      free (currentFunction->Signature);
      currentFunction->Signature = NULL;
      }
    break;

  case 129:
#line 720 "vtkParse.y"
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

  case 130:
#line 733 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 131:
#line 738 "vtkParse.y"
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

  case 132:
#line 750 "vtkParse.y"
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

  case 133:
#line 797 "vtkParse.y"
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

  case 134:
#line 846 "vtkParse.y"
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
#line 2518 "vtkParse.tab.c"

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


#line 913 "vtkParse.y"

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
