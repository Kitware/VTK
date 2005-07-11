/* A Bison parser, made by GNU Bison 1.875d.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004 Free Software Foundation, Inc.

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
     LONG_LONG = 270,
     INT64__ = 271,
     DOUBLE = 272,
     VOID = 273,
     CHAR = 274,
     CLASS_REF = 275,
     OTHER = 276,
     CONST = 277,
     OPERATOR = 278,
     UNSIGNED = 279,
     FRIEND = 280,
     VTK_ID = 281,
     STATIC = 282,
     VAR_FUNCTION = 283,
     ARRAY_NUM = 284,
     VTK_LEGACY = 285,
     IdType = 286,
     StdString = 287,
     SetMacro = 288,
     GetMacro = 289,
     SetStringMacro = 290,
     GetStringMacro = 291,
     SetClampMacro = 292,
     SetObjectMacro = 293,
     SetReferenceCountedObjectMacro = 294,
     GetObjectMacro = 295,
     BooleanMacro = 296,
     SetVector2Macro = 297,
     SetVector3Macro = 298,
     SetVector4Macro = 299,
     SetVector6Macro = 300,
     GetVector2Macro = 301,
     GetVector3Macro = 302,
     GetVector4Macro = 303,
     GetVector6Macro = 304,
     SetVectorMacro = 305,
     GetVectorMacro = 306,
     ViewportCoordinateMacro = 307,
     WorldCoordinateMacro = 308,
     TypeMacro = 309
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
#define LONG_LONG 270
#define INT64__ 271
#define DOUBLE 272
#define VOID 273
#define CHAR 274
#define CLASS_REF 275
#define OTHER 276
#define CONST 277
#define OPERATOR 278
#define UNSIGNED 279
#define FRIEND 280
#define VTK_ID 281
#define STATIC 282
#define VAR_FUNCTION 283
#define ARRAY_NUM 284
#define VTK_LEGACY 285
#define IdType 286
#define StdString 287
#define SetMacro 288
#define GetMacro 289
#define SetStringMacro 290
#define GetStringMacro 291
#define SetClampMacro 292
#define SetObjectMacro 293
#define SetReferenceCountedObjectMacro 294
#define GetObjectMacro 295
#define BooleanMacro 296
#define SetVector2Macro 297
#define SetVector3Macro 298
#define SetVector4Macro 299
#define SetVector6Macro 300
#define GetVector2Macro 301
#define GetVector3Macro 302
#define GetVector4Macro 303
#define GetVector6Macro 304
#define SetVectorMacro 305
#define GetVectorMacro 306
#define ViewportCoordinateMacro 307
#define WorldCoordinateMacro 308
#define TypeMacro 309




/* Copy the first part of user declarations.  */
#line 15 "vtkParse.y"


/*

This file must be translated to C and modified to build everywhere.

Run yacc like this:

  yacc -b vtkParse vtkParse.y

Modify vtkParse.tab.c:
  - remove TABs
  - comment out yyerrorlab stuff

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define yyerror(a) fprintf(stderr,"%s\n",a)
#define yywrap() 1

static void vtkParseDebug(const char* s1, const char* s2);

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
#line 144 "vtkParse.y"
typedef union YYSTYPE {
  char *str;
  int   integer;
  } YYSTYPE;
/* Line 191 of yacc.c.  */
#line 318 "vtkParse.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 330 "vtkParse.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

# ifndef YYFREE
#  define YYFREE free
# endif
# ifndef YYMALLOC
#  define YYMALLOC malloc
# endif

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   define YYSTACK_ALLOC alloca
#  endif
# else
#  if defined (alloca) || defined (_ALLOCA_H)
#   define YYSTACK_ALLOC alloca
#  else
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
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
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
         || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE))                     \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
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
   typedef short int yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  49
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   620

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  70
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  74
/* YYNRULES -- Number of rules. */
#define YYNRULES  182
/* YYNRULES -- Number of states. */
#define YYNSTATES  354

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   309

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    66,     2,
      59,    60,    67,     2,    63,    68,    69,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    57,    58,
       2,    62,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    64,     2,    65,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    55,     2,    56,    61,     2,     2,     2,
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
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,     7,     8,    16,    18,    21,    24,    26,
      28,    31,    34,    38,    41,    44,    46,    51,    54,    58,
      60,    63,    67,    72,    76,    79,    81,    84,    88,    93,
      97,   100,   104,   105,   106,   111,   115,   116,   118,   119,
     125,   127,   129,   131,   133,   135,   140,   144,   148,   149,
     151,   153,   154,   159,   161,   162,   167,   169,   170,   173,
     177,   180,   183,   184,   185,   189,   194,   197,   199,   202,
     206,   208,   211,   213,   215,   218,   221,   223,   225,   227,
     230,   233,   234,   238,   240,   242,   244,   246,   248,   250,
     252,   254,   256,   258,   260,   262,   264,   265,   268,   271,
     272,   278,   280,   282,   284,   287,   289,   291,   295,   297,
     298,   306,   307,   308,   317,   318,   324,   325,   331,   332,
     333,   344,   345,   353,   354,   362,   363,   364,   373,   374,
     382,   383,   391,   392,   400,   401,   409,   410,   418,   419,
     427,   428,   436,   437,   445,   446,   454,   455,   465,   466,
     476,   481,   486,   493,   494,   497,   498,   501,   503,   505,
     507,   509,   511,   513,   515,   517,   519,   521,   523,   525,
     527,   529,   531,   533,   535,   537,   539,   541,   543,   545,
     547,   551,   555
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short int yyrhs[] =
{
      71,     0,    -1,   137,    72,   137,    -1,    -1,     3,    26,
      73,   108,    55,    74,    56,    -1,    75,    -1,    75,    74,
      -1,   111,    57,    -1,    96,    -1,    78,    -1,    25,    78,
      -1,    77,    89,    -1,    25,    77,    89,    -1,    76,    89,
      -1,   114,    58,    -1,   114,    -1,    30,    59,    77,    60,
      -1,    61,    80,    -1,     7,    61,    80,    -1,    80,    -1,
     100,    80,    -1,   100,    22,    80,    -1,     7,   100,    22,
      80,    -1,     7,   100,    80,    -1,     7,    80,    -1,    79,
      -1,   100,    79,    -1,   100,    22,    79,    -1,     7,   100,
      22,    79,    -1,     7,   100,    79,    -1,     7,    79,    -1,
      23,   138,    58,    -1,    -1,    -1,    84,    81,    83,    82,
      -1,    84,    62,     9,    -1,    -1,    22,    -1,    -1,    88,
      59,    85,    90,    60,    -1,    22,    -1,    27,    -1,    26,
      -1,    10,    -1,    58,    -1,    55,   137,    56,    58,    -1,
      55,   137,    56,    -1,    57,   138,    58,    -1,    -1,    91,
      -1,    93,    -1,    -1,    93,    92,    63,    91,    -1,   100,
      -1,    -1,   100,    97,    94,    95,    -1,    28,    -1,    -1,
      62,   112,    -1,   100,    97,    58,    -1,    28,    58,    -1,
      88,    98,    -1,    -1,    -1,    29,    99,    98,    -1,    64,
     138,    65,    98,    -1,    86,   101,    -1,   101,    -1,    87,
     101,    -1,    87,    86,   101,    -1,   105,    -1,   105,   104,
      -1,   102,    -1,   103,    -1,   103,    66,    -1,   103,    67,
      -1,    32,    -1,    66,    -1,    67,    -1,    66,   104,    -1,
      67,   104,    -1,    -1,    24,   106,   107,    -1,   107,    -1,
      12,    -1,    18,    -1,    19,    -1,    11,    -1,    13,    -1,
      14,    -1,    17,    -1,    10,    -1,    26,    -1,    31,    -1,
      15,    -1,    16,    -1,    -1,    57,   109,    -1,   111,    26,
      -1,    -1,   111,    26,   110,    63,   109,    -1,     4,    -1,
       5,    -1,     6,    -1,    68,   113,    -1,   113,    -1,     9,
      -1,     9,    69,     9,    -1,    88,    -1,    -1,    33,    59,
      88,    63,   115,   105,    60,    -1,    -1,    -1,    34,    59,
     116,    88,    63,   117,   105,    60,    -1,    -1,    35,    59,
     118,    88,    60,    -1,    -1,    36,    59,   119,    88,    60,
      -1,    -1,    -1,    37,    59,    88,    63,   120,   105,   121,
      63,   138,    60,    -1,    -1,    38,    59,    88,    63,   122,
     105,    60,    -1,    -1,    39,    59,    88,    63,   123,   105,
      60,    -1,    -1,    -1,    40,    59,   124,    88,    63,   125,
     105,    60,    -1,    -1,    41,    59,    88,   126,    63,   105,
      60,    -1,    -1,    42,    59,    88,    63,   127,   105,    60,
      -1,    -1,    46,    59,    88,    63,   128,   105,    60,    -1,
      -1,    43,    59,    88,    63,   129,   105,    60,    -1,    -1,
      47,    59,    88,    63,   130,   105,    60,    -1,    -1,    44,
      59,    88,    63,   131,   105,    60,    -1,    -1,    48,    59,
      88,    63,   132,   105,    60,    -1,    -1,    45,    59,    88,
      63,   133,   105,    60,    -1,    -1,    49,    59,    88,    63,
     134,   105,    60,    -1,    -1,    50,    59,    88,    63,   135,
     105,    63,   112,    60,    -1,    -1,    51,    59,    88,    63,
     136,   105,    63,   112,    60,    -1,    52,    59,    88,    60,
      -1,    53,    59,    88,    60,    -1,    54,    59,    88,    63,
      88,    60,    -1,    -1,   139,   137,    -1,    -1,   140,   138,
      -1,    58,    -1,   140,    -1,    21,    -1,   141,    -1,   142,
      -1,    67,    -1,    62,    -1,    57,    -1,    63,    -1,    69,
      -1,     8,    -1,   105,    -1,   103,    -1,     9,    -1,    20,
      -1,    66,    -1,   143,    -1,    22,    -1,    23,    -1,    68,
      -1,    61,    -1,    27,    -1,    29,    -1,    55,   137,    56,
      -1,    59,   137,    60,    -1,    64,   137,    65,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   208,   208,   211,   210,   216,   216,   218,   218,   219,
     220,   221,   222,   223,   224,   225,   227,   229,   230,   231,
     232,   236,   240,   245,   250,   256,   260,   265,   270,   276,
     282,   288,   294,   294,   294,   300,   309,   309,   311,   311,
     313,   315,   317,   317,   319,   320,   321,   322,   324,   324,
     326,   327,   327,   329,   335,   334,   341,   348,   348,   350,
     350,   352,   360,   361,   361,   364,   367,   368,   369,   370,
     372,   373,   375,   377,   378,   379,   381,   391,   392,   393,
     394,   396,   396,   398,   401,   402,   403,   404,   405,   406,
     407,   408,   413,   430,   431,   432,   434,   434,   436,   442,
     441,   447,   448,   449,   451,   451,   453,   454,   454,   458,
     457,   469,   469,   469,   478,   478,   489,   489,   499,   500,
     498,   531,   530,   543,   542,   554,   555,   554,   564,   563,
     581,   580,   611,   610,   628,   627,   660,   659,   677,   676,
     711,   710,   728,   727,   766,   765,   783,   782,   801,   800,
     817,   864,   913,   969,   969,   970,   970,   972,   972,   974,
     974,   974,   974,   974,   974,   974,   974,   975,   975,   975,
     975,   975,   975,   975,   976,   976,   976,   976,   976,   976,
     978,   979,   980
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "CLASS", "PUBLIC", "PRIVATE",
  "PROTECTED", "VIRTUAL", "STRING", "NUM", "ID", "INT", "FLOAT", "SHORT",
  "LONG", "LONG_LONG", "INT64__", "DOUBLE", "VOID", "CHAR", "CLASS_REF",
  "OTHER", "CONST", "OPERATOR", "UNSIGNED", "FRIEND", "VTK_ID", "STATIC",
  "VAR_FUNCTION", "ARRAY_NUM", "VTK_LEGACY", "IdType", "StdString",
  "SetMacro", "GetMacro", "SetStringMacro", "GetStringMacro",
  "SetClampMacro", "SetObjectMacro", "SetReferenceCountedObjectMacro",
  "GetObjectMacro", "BooleanMacro", "SetVector2Macro", "SetVector3Macro",
  "SetVector4Macro", "SetVector6Macro", "GetVector2Macro",
  "GetVector3Macro", "GetVector4Macro", "GetVector6Macro",
  "SetVectorMacro", "GetVectorMacro", "ViewportCoordinateMacro",
  "WorldCoordinateMacro", "TypeMacro", "'{'", "'}'", "':'", "';'", "'('",
  "')'", "'~'", "'='", "','", "'['", "']'", "'&'", "'*'", "'-'", "'.'",
  "$accept", "strt", "class_def", "@1", "class_def_body", "class_def_item",
  "legacy_function", "function", "operator", "operator_sig", "func", "@2",
  "@3", "maybe_const", "func_sig", "@4", "const_mod", "static_mod",
  "any_id", "func_body", "args_list", "more_args", "@5", "arg", "@6",
  "opt_var_assign", "var", "var_id", "var_array", "@7", "type",
  "type_red1", "type_string1", "type_string2", "type_indirection",
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
static const unsigned short int yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   123,   125,    58,    59,    40,
      41,   126,    61,    44,    91,    93,    38,    42,    45,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    70,    71,    73,    72,    74,    74,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    76,    77,    77,    77,
      77,    77,    77,    77,    77,    78,    78,    78,    78,    78,
      78,    79,    81,    82,    80,    80,    83,    83,    85,    84,
      86,    87,    88,    88,    89,    89,    89,    89,    90,    90,
      91,    92,    91,    93,    94,    93,    93,    95,    95,    96,
      96,    97,    98,    99,    98,    98,   100,   100,   100,   100,
     101,   101,   101,   102,   102,   102,   103,   104,   104,   104,
     104,   106,   105,   105,   107,   107,   107,   107,   107,   107,
     107,   107,   107,   107,   107,   107,   108,   108,   109,   110,
     109,   111,   111,   111,   112,   112,   113,   113,   113,   115,
     114,   116,   117,   114,   118,   114,   119,   114,   120,   121,
     114,   122,   114,   123,   114,   124,   125,   114,   126,   114,
     127,   114,   128,   114,   129,   114,   130,   114,   131,   114,
     132,   114,   133,   114,   134,   114,   135,   114,   136,   114,
     114,   114,   114,   137,   137,   138,   138,   139,   139,   140,
     140,   140,   140,   140,   140,   140,   140,   140,   140,   140,
     140,   140,   140,   140,   140,   140,   140,   140,   140,   140,
     141,   142,   143
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
       1,     2,     1,     1,     2,     2,     1,     1,     1,     2,
       2,     0,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     0,     2,     2,     0,
       5,     1,     1,     1,     2,     1,     1,     3,     1,     0,
       7,     0,     0,     8,     0,     5,     0,     5,     0,     0,
      10,     0,     7,     0,     7,     0,     0,     8,     0,     7,
       0,     7,     0,     7,     0,     7,     0,     7,     0,     7,
       0,     7,     0,     7,     0,     7,     0,     9,     0,     9,
       4,     4,     6,     0,     2,     0,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     3,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
     153,   167,   170,    91,    87,    84,    88,    89,    94,    95,
      90,    85,    86,   171,   159,   174,   175,    81,    92,   178,
     179,    93,    76,   153,   164,   157,   153,   177,   163,   165,
     153,   172,   162,   176,   166,     0,   169,   168,    83,     0,
     153,   158,   160,   161,   173,     0,     0,     0,     0,     1,
       0,   153,   154,    82,   180,   181,   182,     3,     2,    96,
       0,     0,   101,   102,   103,    97,     0,     0,    98,     0,
      91,    40,   155,     0,    92,    41,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     5,     0,     0,     9,    25,    19,    32,     0,
       0,     0,     8,     0,    67,    72,    73,    70,     0,    15,
       0,     0,    30,    24,     0,     0,   155,     0,    10,     0,
      60,     0,     0,   111,   114,   116,     0,     0,     0,   125,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    43,    42,    17,     4,     6,   153,
     155,    44,    13,    11,     0,    36,    66,     0,    68,    38,
       0,    26,    20,    62,     0,    74,    75,    77,    78,    71,
       7,    14,     0,    18,     0,    29,    23,    31,   156,    12,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   128,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    35,    37,    33,
      69,    48,    27,    21,    63,   155,    61,    59,    79,    80,
     100,    28,    22,     0,    16,     0,   109,     0,     0,     0,
     118,   121,   123,     0,     0,   130,   134,   138,   142,   132,
     136,   140,   144,   146,   148,   150,   151,     0,    46,    47,
      34,    56,     0,    49,    50,    53,    62,     0,     0,     0,
     112,   115,   117,     0,     0,     0,   126,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    45,
      39,     0,    62,    54,    64,    62,     0,     0,   119,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   152,     0,    57,    65,   110,     0,     0,
     122,   124,     0,   129,   131,   135,   139,   143,   133,   137,
     141,   145,     0,     0,    52,     0,    55,   113,   155,   127,
     106,     0,   108,     0,   105,     0,    58,     0,     0,   104,
     147,   149,   120,   107
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,    35,    51,    59,   101,   102,   103,   104,   105,   106,
     107,   165,   260,   219,   108,   221,   109,   110,   111,   162,
     262,   263,   291,   264,   315,   336,   112,   174,   226,   266,
     113,   114,   115,    36,   179,    37,    45,    38,    61,    65,
     120,    66,   343,   344,   119,   269,   194,   297,   195,   196,
     273,   319,   274,   275,   200,   301,   244,   278,   282,   279,
     283,   280,   284,   281,   285,   286,   287,    39,   125,    40,
      41,    42,    43,    44
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -261
static const short int yypact[] =
{
     277,  -261,  -261,  -261,  -261,  -261,  -261,  -261,  -261,  -261,
    -261,  -261,  -261,  -261,  -261,  -261,  -261,  -261,  -261,  -261,
    -261,  -261,  -261,   277,  -261,  -261,   277,  -261,  -261,  -261,
     277,  -261,  -261,  -261,  -261,    27,  -261,  -261,  -261,    35,
     277,  -261,  -261,  -261,  -261,    68,   -26,    29,   -31,  -261,
      26,   277,  -261,  -261,  -261,  -261,  -261,  -261,  -261,    17,
      58,    36,  -261,  -261,  -261,  -261,    67,   405,    37,   511,
      39,  -261,   339,   457,    53,  -261,    60,    61,    63,    64,
      66,    70,    71,    76,    92,    93,    95,    97,   100,   101,
     102,   104,   105,   116,   119,   121,   126,   129,   130,   134,
      23,   138,   405,   -29,   -29,  -261,  -261,  -261,    57,   588,
     155,   139,  -261,    80,  -261,  -261,    30,    48,   140,   142,
     141,    23,  -261,  -261,    85,   143,   339,   -29,  -261,    80,
    -261,   480,    23,  -261,  -261,  -261,    23,    23,    23,  -261,
      23,    23,    23,    23,    23,    23,    23,    23,    23,    23,
      23,    23,    23,    23,  -261,  -261,  -261,  -261,  -261,   277,
     339,  -261,  -261,  -261,   193,   183,  -261,   588,  -261,  -261,
      46,  -261,  -261,   -27,   148,  -261,  -261,    48,    48,  -261,
    -261,  -261,    58,  -261,    46,  -261,  -261,  -261,  -261,  -261,
     534,   150,    31,   159,    23,    23,    23,   161,   163,   165,
      23,  -261,   166,   167,   169,   170,   171,   174,   186,   192,
     194,   195,   176,   196,   199,   207,   206,  -261,  -261,  -261,
    -261,   565,  -261,  -261,  -261,   339,  -261,  -261,  -261,  -261,
    -261,  -261,  -261,   173,  -261,    23,  -261,   202,   208,   209,
    -261,  -261,  -261,   204,   210,  -261,  -261,  -261,  -261,  -261,
    -261,  -261,  -261,  -261,  -261,  -261,  -261,    23,   212,  -261,
    -261,  -261,   211,  -261,   213,    23,   -14,   214,    23,   228,
    -261,  -261,  -261,   228,   228,   228,  -261,   228,   228,   228,
     228,   228,   228,   228,   228,   228,   228,   228,   215,  -261,
    -261,   217,   -14,  -261,  -261,   -14,   218,   228,  -261,   221,
     222,   228,   223,   224,   242,   245,   247,   250,   251,   252,
     253,   254,   255,  -261,   565,   257,  -261,  -261,   256,   258,
    -261,  -261,   260,  -261,  -261,  -261,  -261,  -261,  -261,  -261,
    -261,  -261,     9,     9,  -261,     9,  -261,  -261,   339,  -261,
     203,    14,  -261,   262,  -261,   263,  -261,   264,   265,  -261,
    -261,  -261,  -261,  -261
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -261,  -261,  -261,  -261,   175,  -261,  -261,   -65,   241,   -53,
      -8,  -261,  -261,  -261,  -261,  -261,   205,  -261,    -4,   -79,
    -261,    11,  -261,  -261,  -261,  -261,  -261,    62,  -244,  -261,
     -64,   -97,  -261,   -63,  -119,   -66,  -261,   281,  -261,   151,
    -261,   -47,  -260,   -11,  -261,  -261,  -261,  -261,  -261,  -261,
    -261,  -261,  -261,  -261,  -261,  -261,  -261,  -261,  -261,  -261,
    -261,  -261,  -261,  -261,  -261,  -261,  -261,    -9,  -115,  -261,
     -72,  -261,  -261,  -261
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -100
static const short int yytable[] =
{
     126,   117,   224,   117,   116,   124,   116,   117,   127,   129,
     116,   188,   166,   168,    46,   224,   122,    47,   340,   154,
     118,    48,   294,   340,   154,   163,   159,    49,   160,   161,
      54,    52,   169,   154,    56,   155,   117,   225,    50,   116,
     155,   154,    58,   117,   117,   216,   116,   116,   189,   155,
     225,   316,    57,   235,   126,   118,   154,   155,   228,   229,
     171,   123,    62,    63,    64,   117,   191,   192,   116,    72,
     220,   185,   155,   345,    60,   346,   171,   341,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,   126,    55,
     154,    67,   156,    68,    18,   154,   175,   176,   -43,    21,
     -99,   117,   170,    72,   116,   172,   155,   184,    72,   173,
     267,   155,   -42,   183,   177,   178,   186,   222,   130,   164,
     131,   172,   132,   133,   117,   134,   233,   116,   193,   135,
     136,   231,   197,   198,   199,   137,   201,   202,   203,   204,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   214,
     215,   138,   139,   126,   140,   117,   141,   265,   116,   142,
     143,   144,   223,   145,   146,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,   147,   232,    71,   148,    17,
     149,    18,   123,   154,   172,   150,    21,    22,   151,   152,
     237,   238,   239,   153,   157,   268,   243,   180,   169,   155,
     181,   187,   217,   296,   182,   218,   227,   298,   299,   300,
     234,   302,   303,   304,   305,   306,   307,   308,   309,   310,
     311,   312,   236,   347,   240,   186,   241,   223,   242,   245,
     246,   318,   247,   248,   249,   322,   255,   250,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,   117,   251,
     265,   116,    17,   288,    18,   252,   256,   253,   254,    21,
     232,   292,   257,   258,   259,   270,   126,   276,   271,   272,
     289,   290,   348,   277,   353,   313,   -51,   158,   317,   295,
     314,   320,   321,   323,   324,     1,     2,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,   325,    18,    19,   326,    20,   327,    21,    22,
     328,   329,   330,   331,   128,   167,   337,   332,   333,   335,
     339,   338,   350,   351,   352,   334,    53,   293,   342,   342,
     349,   342,    23,   230,    24,    25,    26,   342,    27,    28,
      29,    30,     0,    31,    32,    33,    34,     1,     2,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,     0,    18,    19,     0,    20,     0,
      21,    22,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    23,     0,    24,     0,    26,     0,
      27,    28,    29,    30,     0,    31,    32,    33,    34,    62,
      63,    64,    69,     0,     0,    70,     4,     5,     6,     7,
       8,     9,    10,    11,    12,     0,     0,    71,    72,    17,
      73,    74,    75,    76,     0,    77,    21,    22,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
       0,     0,     0,     0,    69,     0,   100,    70,     4,     5,
       6,     7,     8,     9,    10,    11,    12,     0,     0,    71,
      72,    17,     0,    74,    75,     0,     0,   190,    21,    22,
      70,     4,     5,     6,     7,     8,     9,    10,    11,    12,
       0,     0,    71,     0,    17,     0,    74,    75,     0,     0,
       0,    21,    22,     0,     0,     0,     0,     0,   100,     0,
       0,    70,     4,     5,     6,     7,     8,     9,    10,    11,
      12,     0,     0,    71,    72,    17,     0,    74,    75,     0,
       0,   100,    21,    22,    70,     4,     5,     6,     7,     8,
       9,    10,    11,    12,     0,     0,    71,     0,    17,     0,
      74,    75,     0,     0,     0,    21,    22,     0,     0,     0,
       0,     0,   121,     0,     0,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,     0,     0,    71,     0,    17,
       0,    18,    75,   261,     0,   121,    21,    22,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,     0,     0,
       0,     0,    17,     0,    18,     0,     0,     0,     0,    21,
      22
};

static const short int yycheck[] =
{
      72,    67,    29,    69,    67,    69,    69,    73,    73,    73,
      73,   126,   109,   110,    23,    29,    69,    26,     9,    10,
      67,    30,   266,     9,    10,   104,    55,     0,    57,    58,
      56,    40,    59,    10,    65,    26,   102,    64,     3,   102,
      26,    10,    51,   109,   110,   160,   109,   110,   127,    26,
      64,   295,    26,    22,   126,   102,    10,    26,   177,   178,
     113,    69,     4,     5,     6,   131,   131,   131,   131,    23,
     167,   124,    26,   333,    57,   335,   129,    68,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,   160,    60,
      10,    55,   100,    26,    26,    10,    66,    67,    59,    31,
      63,   167,    22,    23,   167,   113,    26,    22,    23,   113,
     225,    26,    59,   121,    66,    67,   124,   170,    58,    62,
      59,   129,    59,    59,   190,    59,   190,   190,   132,    59,
      59,   184,   136,   137,   138,    59,   140,   141,   142,   143,
     144,   145,   146,   147,   148,   149,   150,   151,   152,   153,
     159,    59,    59,   225,    59,   221,    59,   221,   221,    59,
      59,    59,   170,    59,    59,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    59,   184,    22,    59,    24,
      59,    26,   190,    10,   192,    59,    31,    32,    59,    59,
     194,   195,   196,    59,    56,    22,   200,    57,    59,    26,
      58,    58,     9,   269,    63,    22,    58,   273,   274,   275,
      60,   277,   278,   279,   280,   281,   282,   283,   284,   285,
     286,   287,    63,   338,    63,   233,    63,   235,    63,    63,
      63,   297,    63,    63,    63,   301,    60,    63,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,   314,    63,
     314,   314,    24,   257,    26,    63,    60,    63,    63,    31,
     268,   265,    63,    56,    58,    63,   338,    63,    60,    60,
      58,    60,    69,    63,     9,    60,    63,   102,    60,    65,
      63,    60,    60,    60,    60,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    60,    26,    27,    60,    29,    60,    31,    32,
      60,    60,    60,    60,    73,   110,    60,    63,    63,    62,
      60,    63,    60,    60,    60,   314,    45,   265,   332,   333,
     341,   335,    55,   182,    57,    58,    59,   341,    61,    62,
      63,    64,    -1,    66,    67,    68,    69,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    -1,    26,    27,    -1,    29,    -1,
      31,    32,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    55,    -1,    57,    -1,    59,    -1,
      61,    62,    63,    64,    -1,    66,    67,    68,    69,     4,
       5,     6,     7,    -1,    -1,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    -1,    -1,    22,    23,    24,
      25,    26,    27,    28,    -1,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      -1,    -1,    -1,    -1,     7,    -1,    61,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    -1,    -1,    22,
      23,    24,    -1,    26,    27,    -1,    -1,     7,    31,    32,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      -1,    -1,    22,    -1,    24,    -1,    26,    27,    -1,    -1,
      -1,    31,    32,    -1,    -1,    -1,    -1,    -1,    61,    -1,
      -1,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    -1,    -1,    22,    23,    24,    -1,    26,    27,    -1,
      -1,    61,    31,    32,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    -1,    -1,    22,    -1,    24,    -1,
      26,    27,    -1,    -1,    -1,    31,    32,    -1,    -1,    -1,
      -1,    -1,    61,    -1,    -1,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    -1,    -1,    22,    -1,    24,
      -1,    26,    27,    28,    -1,    61,    31,    32,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    24,    -1,    26,    -1,    -1,    -1,    -1,    31,
      32
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    26,    27,
      29,    31,    32,    55,    57,    58,    59,    61,    62,    63,
      64,    66,    67,    68,    69,    71,   103,   105,   107,   137,
     139,   140,   141,   142,   143,   106,   137,   137,   137,     0,
       3,    72,   137,   107,    56,    60,    65,    26,   137,    73,
      57,   108,     4,     5,     6,   109,   111,    55,    26,     7,
      10,    22,    23,    25,    26,    27,    28,    30,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      61,    74,    75,    76,    77,    78,    79,    80,    84,    86,
      87,    88,    96,   100,   101,   102,   103,   105,   111,   114,
     110,    61,    79,    80,   100,   138,   140,    77,    78,   100,
      58,    59,    59,    59,    59,    59,    59,    59,    59,    59,
      59,    59,    59,    59,    59,    59,    59,    59,    59,    59,
      59,    59,    59,    59,    10,    26,    80,    56,    74,    55,
      57,    58,    89,    89,    62,    81,   101,    86,   101,    59,
      22,    79,    80,    88,    97,    66,    67,    66,    67,   104,
      57,    58,    63,    80,    22,    79,    80,    58,   138,    89,
       7,    77,   100,    88,   116,   118,   119,    88,    88,    88,
     124,    88,    88,    88,    88,    88,    88,    88,    88,    88,
      88,    88,    88,    88,    88,   137,   138,     9,    22,    83,
     101,    85,    79,    80,    29,    64,    98,    58,   104,   104,
     109,    79,    80,   100,    60,    22,    63,    88,    88,    88,
      63,    63,    63,    88,   126,    63,    63,    63,    63,    63,
      63,    63,    63,    63,    63,    60,    60,    63,    56,    58,
      82,    28,    90,    91,    93,   100,    99,   138,    22,   115,
      63,    60,    60,   120,   122,   123,    63,    63,   127,   129,
     131,   133,   128,   130,   132,   134,   135,   136,    88,    58,
      60,    92,    88,    97,    98,    65,   105,   117,   105,   105,
     105,   125,   105,   105,   105,   105,   105,   105,   105,   105,
     105,   105,   105,    60,    63,    94,    98,    60,   105,   121,
      60,    60,   105,    60,    60,    60,    60,    60,    60,    60,
      60,    60,    63,    63,    91,    62,    95,    60,    63,    60,
       9,    68,    88,   112,   113,   112,   112,   138,    69,   113,
      60,    60,    60,     9
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
#define YYERROR         goto yyerrorlab


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
# define YYLLOC_DEFAULT(Current, Rhs, N)                \
   ((Current).first_line   = (Rhs)[1].first_line,       \
    (Current).first_column = (Rhs)[1].first_column,     \
    (Current).last_line    = (Rhs)[N].last_line,        \
    (Current).last_column  = (Rhs)[N].last_column)
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
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short int *bottom, short int *top)
#else
static void
yy_stack_print (bottom, top)
    short int *bottom;
    short int *top;
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
  unsigned int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylno);
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

#if defined (YYMAXDEPTH) && YYMAXDEPTH == 0
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
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  register short int *yyssp;

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
        short int *yyss1 = yyss;


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
        short int *yyss1 = yyss;
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
#line 211 "vtkParse.y"
    {
      data.ClassName = vtkstrdup(yyvsp[0].str);
      }
    break;

  case 11:
#line 221 "vtkParse.y"
    { output_function(); }
    break;

  case 12:
#line 222 "vtkParse.y"
    { output_function(); }
    break;

  case 13:
#line 223 "vtkParse.y"
    { legacySig(); output_function(); }
    break;

  case 17:
#line 229 "vtkParse.y"
    { preSig("~"); }
    break;

  case 18:
#line 230 "vtkParse.y"
    { preSig("virtual ~"); }
    break;

  case 20:
#line 233 "vtkParse.y"
    {
         currentFunction->ReturnType = yyvsp[-1].integer;
         }
    break;

  case 21:
#line 237 "vtkParse.y"
    {
         currentFunction->ReturnType = yyvsp[-2].integer;
         }
    break;

  case 22:
#line 241 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-2].integer;
         }
    break;

  case 23:
#line 246 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-1].integer;
         }
    break;

  case 24:
#line 251 "vtkParse.y"
    {
         preSig("virtual ");
         }
    break;

  case 25:
#line 257 "vtkParse.y"
    {
         output_function();
         }
    break;

  case 26:
#line 261 "vtkParse.y"
    {
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
         }
    break;

  case 27:
#line 266 "vtkParse.y"
    {
         currentFunction->ReturnType = yyvsp[-2].integer;
         output_function();
         }
    break;

  case 28:
#line 271 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-2].integer;
         output_function();
         }
    break;

  case 29:
#line 277 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
         }
    break;

  case 30:
#line 283 "vtkParse.y"
    {
         preSig("virtual ");
         output_function();
         }
    break;

  case 31:
#line 289 "vtkParse.y"
    {
      currentFunction->IsOperator = 1;
      vtkParseDebug("Converted operator", 0);
    }
    break;

  case 32:
#line 294 "vtkParse.y"
    { postSig(")"); }
    break;

  case 33:
#line 294 "vtkParse.y"
    { postSig(";"); openSig = 0; }
    break;

  case 34:
#line 295 "vtkParse.y"
    {
      openSig = 1;
      currentFunction->Name = yyvsp[-3].str; 
      vtkParseDebug("Parsed func", yyvsp[-3].str);
    }
    break;

  case 35:
#line 301 "vtkParse.y"
    { 
      postSig(") = 0;"); 
      currentFunction->Name = yyvsp[-2].str; 
      vtkParseDebug("Parsed func", yyvsp[-2].str);
      currentFunction->IsPureVirtual = 1; 
      data.IsAbstract = 1;
    }
    break;

  case 37:
#line 309 "vtkParse.y"
    {postSig(" const");}
    break;

  case 38:
#line 311 "vtkParse.y"
    {postSig(" ("); }
    break;

  case 40:
#line 313 "vtkParse.y"
    {postSig("const ");}
    break;

  case 41:
#line 315 "vtkParse.y"
    {postSig("static ");}
    break;

  case 42:
#line 317 "vtkParse.y"
    {postSig(yyvsp[0].str);}
    break;

  case 43:
#line 317 "vtkParse.y"
    {postSig(yyvsp[0].str);}
    break;

  case 50:
#line 326 "vtkParse.y"
    { currentFunction->NumberOfArguments++;}
    break;

  case 51:
#line 327 "vtkParse.y"
    { currentFunction->NumberOfArguments++; postSig(", ");}
    break;

  case 53:
#line 330 "vtkParse.y"
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
        yyvsp[0].integer;}
    break;

  case 54:
#line 335 "vtkParse.y"
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 
        yyvsp[0].integer / 0x10000; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
        yyvsp[-1].integer + yyvsp[0].integer % 0x10000;
    }
    break;

  case 56:
#line 342 "vtkParse.y"
    { 
      postSig("void (*func)(void *) ");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 0x5000;
    }
    break;

  case 59:
#line 350 "vtkParse.y"
    {delSig();}
    break;

  case 60:
#line 350 "vtkParse.y"
    {delSig();}
    break;

  case 61:
#line 352 "vtkParse.y"
    { yyval.integer = yyvsp[0].integer; }
    break;

  case 62:
#line 360 "vtkParse.y"
    { yyval.integer = 0; }
    break;

  case 63:
#line 361 "vtkParse.y"
    { char temp[100]; sprintf(temp,"[%i]",yyvsp[0].integer); 
                   postSig(temp); }
    break;

  case 64:
#line 363 "vtkParse.y"
    { yyval.integer = 0x300 + 0x10000 * yyvsp[-2].integer + yyvsp[0].integer % 0x1000; }
    break;

  case 65:
#line 365 "vtkParse.y"
    { postSig("[]"); yyval.integer = 0x300 + yyvsp[0].integer % 0x1000; }
    break;

  case 66:
#line 367 "vtkParse.y"
    {yyval.integer = 0x1000 + yyvsp[0].integer;}
    break;

  case 67:
#line 368 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;}
    break;

  case 68:
#line 369 "vtkParse.y"
    {yyval.integer = 0x2000 + yyvsp[0].integer;}
    break;

  case 69:
#line 370 "vtkParse.y"
    {yyval.integer = 0x3000 + yyvsp[0].integer;}
    break;

  case 70:
#line 372 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;}
    break;

  case 71:
#line 374 "vtkParse.y"
    {yyval.integer = yyvsp[-1].integer + yyvsp[0].integer;}
    break;

  case 72:
#line 375 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;}
    break;

  case 73:
#line 377 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;}
    break;

  case 74:
#line 378 "vtkParse.y"
    { postSig("&"); yyval.integer = yyvsp[-1].integer;}
    break;

  case 75:
#line 379 "vtkParse.y"
    { postSig("*"); yyval.integer = 0x400 + yyvsp[-1].integer;}
    break;

  case 76:
#line 381 "vtkParse.y"
    { postSig("vtkStdString "); yyval.integer = 0x1303; }
    break;

  case 77:
#line 391 "vtkParse.y"
    { postSig("&"); yyval.integer = 0x100;}
    break;

  case 78:
#line 392 "vtkParse.y"
    { postSig("*"); yyval.integer = 0x300;}
    break;

  case 79:
#line 393 "vtkParse.y"
    { yyval.integer = 0x100 + yyvsp[0].integer;}
    break;

  case 80:
#line 394 "vtkParse.y"
    { yyval.integer = 0x400 + yyvsp[0].integer;}
    break;

  case 81:
#line 396 "vtkParse.y"
    {postSig("unsigned ");}
    break;

  case 82:
#line 397 "vtkParse.y"
    { yyval.integer = 0x10 + yyvsp[0].integer;}
    break;

  case 83:
#line 398 "vtkParse.y"
    { yyval.integer = yyvsp[0].integer;}
    break;

  case 84:
#line 401 "vtkParse.y"
    { postSig("float "); yyval.integer = 0x1;}
    break;

  case 85:
#line 402 "vtkParse.y"
    { postSig("void "); yyval.integer = 0x2;}
    break;

  case 86:
#line 403 "vtkParse.y"
    { postSig("char "); yyval.integer = 0x3;}
    break;

  case 87:
#line 404 "vtkParse.y"
    { postSig("int "); yyval.integer = 0x4;}
    break;

  case 88:
#line 405 "vtkParse.y"
    { postSig("short "); yyval.integer = 0x5;}
    break;

  case 89:
#line 406 "vtkParse.y"
    { postSig("long "); yyval.integer = 0x6;}
    break;

  case 90:
#line 407 "vtkParse.y"
    { postSig("double "); yyval.integer = 0x7;}
    break;

  case 91:
#line 408 "vtkParse.y"
    {       
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",yyvsp[0].str);
      postSig(ctmpid);
      yyval.integer = 0x8;}
    break;

  case 92:
#line 414 "vtkParse.y"
    { 
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",yyvsp[0].str);
      postSig(ctmpid);
      yyval.integer = 0x9; 
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

  case 93:
#line 430 "vtkParse.y"
    { postSig("vtkIdType "); yyval.integer = 0xA;}
    break;

  case 94:
#line 431 "vtkParse.y"
    { postSig("long long "); yyval.integer = 0xB;}
    break;

  case 95:
#line 432 "vtkParse.y"
    { postSig("__int64 "); yyval.integer = 0xC;}
    break;

  case 98:
#line 437 "vtkParse.y"
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    }
    break;

  case 99:
#line 442 "vtkParse.y"
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    }
    break;

  case 101:
#line 447 "vtkParse.y"
    {in_public = 1; in_protected = 0;}
    break;

  case 102:
#line 448 "vtkParse.y"
    {in_public = 0; in_protected = 0;}
    break;

  case 103:
#line 449 "vtkParse.y"
    {in_public = 0; in_protected = 1;}
    break;

  case 106:
#line 453 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;}
    break;

  case 107:
#line 454 "vtkParse.y"
    {yyval.integer = -1;}
    break;

  case 108:
#line 454 "vtkParse.y"
    {yyval.integer = -1;}
    break;

  case 109:
#line 458 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); }
    break;

  case 110:
#line 459 "vtkParse.y"
    {
   postSig(");");
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();
   }
    break;

  case 111:
#line 469 "vtkParse.y"
    {postSig("Get");}
    break;

  case 112:
#line 469 "vtkParse.y"
    {postSig(" ();"); invertSig = 1;}
    break;

  case 113:
#line 471 "vtkParse.y"
    { 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = yyvsp[-1].integer;
   output_function();
   }
    break;

  case 114:
#line 478 "vtkParse.y"
    {preSig("void Set");}
    break;

  case 115:
#line 479 "vtkParse.y"
    {
   postSig(" (char *);"); 
   sprintf(temps,"Set%s",yyvsp[-1].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x303;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();
   }
    break;

  case 116:
#line 489 "vtkParse.y"
    {preSig("char *Get");}
    break;

  case 117:
#line 490 "vtkParse.y"
    { 
   postSig(" ();");
   sprintf(temps,"Get%s",yyvsp[-1].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x303;
   output_function();
   }
    break;

  case 118:
#line 499 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); }
    break;

  case 119:
#line 500 "vtkParse.y"
    {postSig(");"); openSig = 0;}
    break;

  case 120:
#line 501 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sscanf (currentFunction->Signature, "%*s %*s (%s);", local);
   sprintf(temps,"Set%s",yyvsp[-7].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = yyvsp[-4].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 0x2;
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

  case 121:
#line 531 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); }
    break;

  case 122:
#line 532 "vtkParse.y"
    { 
   postSig("*);");
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 0x2;
   output_function();
   }
    break;

  case 123:
#line 543 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); }
    break;

  case 124:
#line 544 "vtkParse.y"
    { 
   postSig("*);");
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 0x2;
   output_function();
   }
    break;

  case 125:
#line 554 "vtkParse.y"
    {postSig("*Get");}
    break;

  case 126:
#line 555 "vtkParse.y"
    {postSig(" ();"); invertSig = 1;}
    break;

  case 127:
#line 556 "vtkParse.y"
    { 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x309;
   output_function();
   }
    break;

  case 128:
#line 564 "vtkParse.y"
    {preSig("void "); postSig("On ();"); openSig = 0; }
    break;

  case 129:
#line 566 "vtkParse.y"
    { 
   sprintf(temps,"%sOn",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x2;
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

  case 130:
#line 581 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 131:
#line 586 "vtkParse.y"
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
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[2]);",yyvsp[-4].str,
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 0x2;
   output_function();
   }
    break;

  case 132:
#line 611 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 133:
#line 616 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 2;
   output_function();
   }
    break;

  case 134:
#line 628 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 135:
#line 633 "vtkParse.y"
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
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[3]);",yyvsp[-4].str,
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 3;
   output_function();
   }
    break;

  case 136:
#line 660 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 137:
#line 665 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 3;
   output_function();
   }
    break;

  case 138:
#line 677 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 139:
#line 682 "vtkParse.y"
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
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[4]);",yyvsp[-4].str,
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 4;
   output_function();
   }
    break;

  case 140:
#line 711 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 141:
#line 716 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 4;
   output_function();
   }
    break;

  case 142:
#line 728 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 143:
#line 733 "vtkParse.y"
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
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[6]);",yyvsp[-4].str,
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 6;
   output_function();
   }
    break;

  case 144:
#line 766 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 145:
#line 771 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 6;
   output_function();
   }
    break;

  case 146:
#line 783 "vtkParse.y"
    {
      free (currentFunction->Signature);
      currentFunction->Signature = NULL;
      }
    break;

  case 147:
#line 788 "vtkParse.y"
    {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s [%i]);",yyvsp[-6].str,
      local, yyvsp[-1].integer);
     sprintf(temps,"Set%s",yyvsp[-6].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->ReturnType = 0x2;
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 0x300 + yyvsp[-3].integer;
     currentFunction->ArgCounts[0] = yyvsp[-1].integer;
     output_function();
   }
    break;

  case 148:
#line 801 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 149:
#line 806 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-6].str);
   sprintf(temps,"Get%s",yyvsp[-6].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + yyvsp[-3].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = yyvsp[-1].integer;
   output_function();
   }
    break;

  case 150:
#line 818 "vtkParse.y"
    { 
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       yyvsp[-1].str);

     sprintf(temps,"Get%sCoordinate",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 0x309;
     currentFunction->ReturnClass = vtkstrdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double, double);",
       yyvsp[-1].str);
     sprintf(temps,"Set%s",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 2;
     currentFunction->ArgTypes[0] = 0x7;
     currentFunction->ArgCounts[0] = 0;
     currentFunction->ArgTypes[1] = 0x7;
     currentFunction->ArgCounts[1] = 0;
     currentFunction->ReturnType = 0x2;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double a[2]);",
       yyvsp[-1].str);
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 0x307;
     currentFunction->ArgCounts[0] = 2;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s ();", yyvsp[-1].str);
     sprintf(temps,"Get%s",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 0x307;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 2;
     output_function();
   }
    break;

  case 151:
#line 865 "vtkParse.y"
    { 
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       yyvsp[-1].str);

     sprintf(temps,"Get%sCoordinate",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 0x309;
     currentFunction->ReturnClass = vtkstrdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double, double, double);",
       yyvsp[-1].str);
     sprintf(temps,"Set%s",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 3;
     currentFunction->ArgTypes[0] = 0x7;
     currentFunction->ArgCounts[0] = 0;
     currentFunction->ArgTypes[1] = 0x7;
     currentFunction->ArgCounts[1] = 0;
     currentFunction->ArgTypes[2] = 0x7;
     currentFunction->ArgCounts[2] = 0;
     currentFunction->ReturnType = 0x2;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double a[3]);",
       yyvsp[-1].str);
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 0x307;
     currentFunction->ArgCounts[0] = 3;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s ();", yyvsp[-1].str);
     sprintf(temps,"Get%s",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 0x307;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 3;
     output_function();
   }
    break;

  case 152:
#line 914 "vtkParse.y"
    { 
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "const char *GetClassName ();");
   sprintf(temps,"GetClassName"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x1303;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,
           "int IsA (const char *name);");
   sprintf(temps,"IsA"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x1303;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 0x4;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "%s *NewInstance ();",
           yyvsp[-3].str);
   sprintf(temps,"NewInstance"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x309;
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
     currentFunction->ArgTypes[0] = 0x309;
     currentFunction->ArgCounts[0] = 1;
     currentFunction->ArgClasses[0] = vtkstrdup("vtkObject");
     currentFunction->ReturnType = 0x2309;
     currentFunction->ReturnClass = vtkstrdup(yyvsp[-3].str);
     output_function();
     }
   }
    break;


    }

/* Line 1010 of yacc.c.  */
#line 2731 "vtkParse.tab.c"

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
          const char* yyprefix;
          char *yymsg;
          int yyx;

          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;

          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yycount = 0;

          yyprefix = ", expecting ";
          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
              {
                yysize += yystrlen (yyprefix) + yystrlen (yytname [yyx]);
                yycount += 1;
                if (yycount == 5)
                  {
                    yysize = 0;
                    break;
                  }
              }
          yysize += (sizeof ("syntax error, unexpected ")
                     + yystrlen (yytname[yytype]));
          yymsg = (char *) YYSTACK_ALLOC (yysize);
          if (yymsg != 0)
            {
              char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
              yyp = yystpcpy (yyp, yytname[yytype]);

              if (yycount < 5)
                {
                  yyprefix = ", expecting ";
                  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
                    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
                      {
                        yyp = yystpcpy (yyp, yyprefix);
                        yyp = yystpcpy (yyp, yytname[yyx]);
                        yyprefix = " or ";
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

      if (yychar <= YYEOF)
        {
          /* If at end of input, pop the error token,
             then the rest of the stack, then return failure.  */
          if (yychar == YYEOF)
             for (;;)
               {
                 YYPOPSTACK;
                 if (yyssp == yyss)
                   YYABORT;
                 YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
                 yydestruct (yystos[*yyssp], yyvsp);
               }
        }
      else
        {
          YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
          yydestruct (yytoken, &yylval);
          yychar = YYEMPTY;

        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
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
      YYPOPSTACK;
      yystate = *yyssp;
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


#line 982 "vtkParse.y"

#include <string.h>
#include "lex.yy.c"

static void vtkParseDebug(const char* s1, const char* s2)
{
  if ( getenv("DEBUG") )
    {
    fprintf(stderr, "   %s", s1);
    if ( s2 )
      {
      fprintf(stderr, " %s", s2);
      }
    fprintf(stderr, "\n");
    }
}

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
  func->ReturnType = 0x2;
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
  unsigned int  h_type;
  int  h_value;

  /* reset the position */
  if (!fhint) 
    {
    return;
    }
  rewind(fhint);

  /* first find a hint */
  while (fscanf(fhint,"%s %s %x %i",h_cls,h_func,&h_type,&h_value) != EOF)
    {
    if ((!strcmp(h_cls,data.ClassName))&&
        currentFunction->Name &&
        (!strcmp(h_func,currentFunction->Name))&&
        ((int)h_type == currentFunction->ReturnType))
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
  if (currentFunction->ArgTypes[0] % 0x1000 == 0x2) 
    {
    currentFunction->NumberOfArguments = 0;
    }

  currentFunction->IsPublic = in_public;
  currentFunction->IsProtected = in_protected;
  
  /* look for VAR FUNCTIONS */
  if (currentFunction->NumberOfArguments
      && (currentFunction->ArgTypes[0] == 0x5000))
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
    switch (currentFunction->ReturnType % 0x1000)
      {
      case 0x301: case 0x302: case 0x307: case 0x30A: case 0x30B: case 0x30C:
      case 0x304: case 0x305: case 0x306: case 0x313:
        look_for_hint();
        break;
      }
    }

  /* reject multi-dimensional arrays from wrappers */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    if ((currentFunction->ArgTypes[i] % 0x1000)/0x100 == 0x6 ||
        (currentFunction->ArgTypes[i] % 0x1000)/0x100 == 0x9)
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




