/* A Bison parser, made by GNU Bison 1.875c.  */

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
     IdType = 284,
     SetMacro = 285,
     GetMacro = 286,
     SetStringMacro = 287,
     GetStringMacro = 288,
     SetClampMacro = 289,
     SetObjectMacro = 290,
     SetReferenceCountedObjectMacro = 291,
     GetObjectMacro = 292,
     BooleanMacro = 293,
     SetVector2Macro = 294,
     SetVector3Macro = 295,
     SetVector4Macro = 296,
     SetVector6Macro = 297,
     GetVector2Macro = 298,
     GetVector3Macro = 299,
     GetVector4Macro = 300,
     GetVector6Macro = 301,
     SetVectorMacro = 302,
     GetVectorMacro = 303,
     ViewportCoordinateMacro = 304,
     WorldCoordinateMacro = 305,
     TypeMacro = 306
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
#define IdType 284
#define SetMacro 285
#define GetMacro 286
#define SetStringMacro 287
#define GetStringMacro 288
#define SetClampMacro 289
#define SetObjectMacro 290
#define SetReferenceCountedObjectMacro 291
#define GetObjectMacro 292
#define BooleanMacro 293
#define SetVector2Macro 294
#define SetVector3Macro 295
#define SetVector4Macro 296
#define SetVector6Macro 297
#define GetVector2Macro 298
#define GetVector3Macro 299
#define GetVector4Macro 300
#define GetVector6Macro 301
#define SetVectorMacro 302
#define GetVectorMacro 303
#define ViewportCoordinateMacro 304
#define WorldCoordinateMacro 305
#define TypeMacro 306




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
#line 312 "vtkParse.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 324 "vtkParse.tab.c"

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
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))            \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)      \
      do               \
   {               \
     register YYSIZE_T yyi;      \
     for (yyi = 0; yyi < (Count); yyi++)   \
       (To)[yyi] = (From)[yyi];      \
   }               \
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)               \
    do                           \
      {                           \
   YYSIZE_T yynewbytes;                  \
   YYCOPY (&yyptr->Stack, Stack, yysize);            \
   Stack = &yyptr->Stack;                  \
   yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
   yyptr += yynewbytes / sizeof (*yyptr);            \
      }                           \
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  45
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   495

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  67
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  72
/* YYNRULES -- Number of rules. */
#define YYNRULES  174
/* YYNRULES -- Number of states. */
#define YYNSTATES  346

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   306

#define YYTRANSLATE(YYX)                   \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    63,     2,
      56,    57,    64,     2,    60,    65,    66,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    54,    55,
       2,    59,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    61,     2,    62,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    52,     2,    53,    58,     2,     2,     2,
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
      45,    46,    47,    48,    49,    50,    51
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
     230,   232,   234,   236,   238,   240,   242,   244,   246,   248,
     249,   252,   255,   256,   262,   264,   266,   268,   271,   273,
     275,   279,   281,   282,   290,   291,   292,   301,   302,   308,
     309,   315,   316,   317,   328,   329,   337,   338,   346,   347,
     348,   357,   358,   366,   367,   375,   376,   384,   385,   393,
     394,   402,   403,   411,   412,   420,   421,   429,   430,   438,
     439,   449,   450,   460,   465,   470,   477,   478,   481,   482,
     485,   487,   489,   491,   493,   495,   497,   499,   501,   503,
     505,   507,   509,   511,   513,   515,   517,   519,   521,   523,
     525,   527,   529,   533,   537
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short yyrhs[] =
{
      68,     0,    -1,   132,    69,   132,    -1,    -1,     3,    24,
      70,   103,    52,    71,    53,    -1,    72,    -1,    72,    71,
      -1,   106,    54,    -1,    93,    -1,    75,    -1,    23,    75,
      -1,    74,    86,    -1,    23,    74,    86,    -1,    73,    86,
      -1,   109,    55,    -1,   109,    -1,    28,    56,    74,    57,
      -1,    58,    77,    -1,     7,    58,    77,    -1,    77,    -1,
      97,    77,    -1,    97,    20,    77,    -1,     7,    97,    20,
      77,    -1,     7,    97,    77,    -1,     7,    77,    -1,    76,
      -1,    97,    76,    -1,    97,    20,    76,    -1,     7,    97,
      20,    76,    -1,     7,    97,    76,    -1,     7,    76,    -1,
      21,   133,    55,    -1,    -1,    -1,    81,    78,    80,    79,
      -1,    81,    59,     9,    -1,    -1,    20,    -1,    -1,    85,
      56,    82,    87,    57,    -1,    20,    -1,    25,    -1,    24,
      -1,    10,    -1,    55,    -1,    52,   132,    53,    55,    -1,
      52,   132,    53,    -1,    54,   133,    55,    -1,    -1,    88,
      -1,    90,    -1,    -1,    90,    89,    60,    88,    -1,    97,
      -1,    -1,    97,    94,    91,    92,    -1,    26,    -1,    -1,
      59,   107,    -1,    97,    94,    55,    -1,    26,    55,    -1,
      85,    95,    -1,    -1,    -1,    27,    96,    95,    -1,    61,
     133,    62,    95,    -1,    83,    98,    -1,    98,    -1,    84,
      98,    -1,    84,    83,    98,    -1,   100,    -1,   100,    99,
      -1,    63,    -1,    64,    -1,    63,    99,    -1,    64,    99,
      -1,    -1,    22,   101,   102,    -1,   102,    -1,    12,    -1,
      16,    -1,    17,    -1,    11,    -1,    13,    -1,    14,    -1,
      15,    -1,    10,    -1,    24,    -1,    29,    -1,    -1,    54,
     104,    -1,   106,    24,    -1,    -1,   106,    24,   105,    60,
     104,    -1,     4,    -1,     5,    -1,     6,    -1,    65,   108,
      -1,   108,    -1,     9,    -1,     9,    66,     9,    -1,    85,
      -1,    -1,    30,    56,    85,    60,   110,   100,    57,    -1,
      -1,    -1,    31,    56,   111,    85,    60,   112,   100,    57,
      -1,    -1,    32,    56,   113,    85,    57,    -1,    -1,    33,
      56,   114,    85,    57,    -1,    -1,    -1,    34,    56,    85,
      60,   115,   100,   116,    60,   133,    57,    -1,    -1,    35,
      56,    85,    60,   117,   100,    57,    -1,    -1,    36,    56,
      85,    60,   118,   100,    57,    -1,    -1,    -1,    37,    56,
     119,    85,    60,   120,   100,    57,    -1,    -1,    38,    56,
      85,   121,    60,   100,    57,    -1,    -1,    39,    56,    85,
      60,   122,   100,    57,    -1,    -1,    43,    56,    85,    60,
     123,   100,    57,    -1,    -1,    40,    56,    85,    60,   124,
     100,    57,    -1,    -1,    44,    56,    85,    60,   125,   100,
      57,    -1,    -1,    41,    56,    85,    60,   126,   100,    57,
      -1,    -1,    45,    56,    85,    60,   127,   100,    57,    -1,
      -1,    42,    56,    85,    60,   128,   100,    57,    -1,    -1,
      46,    56,    85,    60,   129,   100,    57,    -1,    -1,    47,
      56,    85,    60,   130,   100,    60,   107,    57,    -1,    -1,
      48,    56,    85,    60,   131,   100,    60,   107,    57,    -1,
      49,    56,    85,    57,    -1,    50,    56,    85,    57,    -1,
      51,    56,    85,    60,    85,    57,    -1,    -1,   134,   132,
      -1,    -1,   135,   133,    -1,    55,    -1,   135,    -1,    19,
      -1,   136,    -1,   137,    -1,    64,    -1,    59,    -1,    54,
      -1,    60,    -1,    66,    -1,     8,    -1,   100,    -1,     9,
      -1,    18,    -1,    63,    -1,   138,    -1,    20,    -1,    21,
      -1,    65,    -1,    58,    -1,    25,    -1,    27,    -1,    52,
     132,    53,    -1,    56,   132,    57,    -1,    61,   132,    62,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   205,   205,   208,   207,   213,   213,   215,   215,   216,
     217,   218,   219,   220,   221,   222,   224,   226,   227,   228,
     229,   233,   237,   242,   247,   253,   257,   262,   267,   273,
     279,   285,   291,   291,   291,   297,   306,   306,   308,   308,
     310,   312,   314,   314,   316,   317,   318,   319,   321,   321,
     323,   324,   324,   326,   332,   331,   338,   345,   345,   347,
     347,   349,   357,   358,   358,   361,   364,   365,   366,   367,
     369,   370,   380,   381,   382,   383,   385,   385,   387,   390,
     391,   392,   393,   394,   395,   396,   397,   402,   419,   421,
     421,   423,   429,   428,   434,   435,   436,   438,   438,   440,
     441,   441,   445,   444,   456,   456,   456,   465,   465,   476,
     476,   486,   487,   485,   518,   517,   530,   529,   541,   542,
     541,   551,   550,   568,   567,   598,   597,   615,   614,   647,
     646,   664,   663,   698,   697,   715,   714,   753,   752,   770,
     769,   788,   787,   804,   851,   900,   956,   956,   957,   957,
     959,   959,   961,   961,   961,   961,   961,   961,   961,   961,
     962,   962,   962,   962,   962,   962,   962,   963,   963,   963,
     963,   963,   965,   966,   967
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "CLASS", "PUBLIC", "PRIVATE",
  "PROTECTED", "VIRTUAL", "STRING", "NUM", "ID", "INT", "FLOAT", "SHORT",
  "LONG", "DOUBLE", "VOID", "CHAR", "CLASS_REF", "OTHER", "CONST",
  "OPERATOR", "UNSIGNED", "FRIEND", "VTK_ID", "STATIC", "VAR_FUNCTION",
  "ARRAY_NUM", "VTK_LEGACY", "IdType", "SetMacro", "GetMacro",
  "SetStringMacro", "GetStringMacro", "SetClampMacro", "SetObjectMacro",
  "SetReferenceCountedObjectMacro", "GetObjectMacro", "BooleanMacro",
  "SetVector2Macro", "SetVector3Macro", "SetVector4Macro",
  "SetVector6Macro", "GetVector2Macro", "GetVector3Macro",
  "GetVector4Macro", "GetVector6Macro", "SetVectorMacro", "GetVectorMacro",
  "ViewportCoordinateMacro", "WorldCoordinateMacro", "TypeMacro", "'{'",
  "'}'", "':'", "';'", "'('", "')'", "'~'", "'='", "','", "'['", "']'",
  "'&'", "'*'", "'-'", "'.'", "$accept", "strt", "class_def", "@1",
  "class_def_body", "class_def_item", "legacy_function", "function",
  "operator", "operator_sig", "func", "@2", "@3", "maybe_const",
  "func_sig", "@4", "const_mod", "static_mod", "any_id", "func_body",
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
     305,   306,   123,   125,    58,    59,    40,    41,   126,    61,
      44,    91,    93,    38,    42,    45,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    67,    68,    70,    69,    71,    71,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    73,    74,    74,    74,
      74,    74,    74,    74,    74,    75,    75,    75,    75,    75,
      75,    76,    78,    79,    77,    77,    80,    80,    82,    81,
      83,    84,    85,    85,    86,    86,    86,    86,    87,    87,
      88,    89,    88,    90,    91,    90,    90,    92,    92,    93,
      93,    94,    95,    96,    95,    95,    97,    97,    97,    97,
      98,    98,    99,    99,    99,    99,   101,   100,   100,   102,
     102,   102,   102,   102,   102,   102,   102,   102,   102,   103,
     103,   104,   105,   104,   106,   106,   106,   107,   107,   108,
     108,   108,   110,   109,   111,   112,   109,   113,   109,   114,
     109,   115,   116,   109,   117,   109,   118,   109,   119,   120,
     109,   121,   109,   122,   109,   123,   109,   124,   109,   125,
     109,   126,   109,   127,   109,   128,   109,   129,   109,   130,
     109,   131,   109,   109,   109,   109,   132,   132,   133,   133,
     134,   134,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   136,   137,   138
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
       1,     1,     1,     1,     1,     1,     1,     1,     1,     0,
       2,     2,     0,     5,     1,     1,     1,     2,     1,     1,
       3,     1,     0,     7,     0,     0,     8,     0,     5,     0,
       5,     0,     0,    10,     0,     7,     0,     7,     0,     0,
       8,     0,     7,     0,     7,     0,     7,     0,     7,     0,
       7,     0,     7,     0,     7,     0,     7,     0,     7,     0,
       9,     0,     9,     4,     4,     6,     0,     2,     0,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     3,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
     146,   160,   162,    86,    82,    79,    83,    84,    85,    80,
      81,   163,   152,   166,   167,    76,    87,   170,   171,    88,
     146,   157,   150,   146,   169,   156,   158,   146,   164,   155,
     168,   159,     0,   161,    78,     0,   146,   151,   153,   154,
     165,     0,     0,     0,     0,     1,     0,   146,   147,    77,
     172,   173,   174,     3,     2,    89,     0,     0,    94,    95,
      96,    90,     0,     0,    91,     0,    86,    40,   148,     0,
      87,    41,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     5,     0,
       0,     9,    25,    19,    32,     0,     0,     0,     8,     0,
      67,    70,     0,    15,     0,     0,    30,    24,     0,     0,
     148,     0,    10,     0,    60,     0,     0,   104,   107,   109,
       0,     0,     0,   118,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    43,    42,
      17,     4,     6,   146,   148,    44,    13,    11,     0,    36,
      66,     0,    68,    38,     0,    26,    20,    62,     0,    72,
      73,    71,     7,    14,     0,    18,     0,    29,    23,    31,
     149,    12,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   121,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    35,
      37,    33,    69,    48,    27,    21,    63,   148,    61,    59,
      74,    75,    93,    28,    22,     0,    16,     0,   102,     0,
       0,     0,   111,   114,   116,     0,     0,   123,   127,   131,
     135,   125,   129,   133,   137,   139,   141,   143,   144,     0,
      46,    47,    34,    56,     0,    49,    50,    53,    62,     0,
       0,     0,   105,   108,   110,     0,     0,     0,   119,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    45,    39,     0,    62,    54,    64,    62,     0,     0,
     112,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   145,     0,    57,    65,   103,
       0,     0,   115,   117,     0,   122,   124,   128,   132,   136,
     126,   130,   134,   138,     0,     0,    52,     0,    55,   106,
     148,   120,    99,     0,   101,     0,    98,     0,    58,     0,
       0,    97,   140,   142,   113,   100
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,    32,    47,    55,    97,    98,    99,   100,   101,   102,
     103,   159,   252,   211,   104,   213,   105,   106,   107,   156,
     254,   255,   283,   256,   307,   328,   108,   168,   218,   258,
     109,   110,   171,    33,    41,    34,    57,    61,   114,    62,
     335,   336,   113,   261,   186,   289,   187,   188,   265,   311,
     266,   267,   192,   293,   236,   270,   274,   271,   275,   272,
     276,   273,   277,   278,   279,    35,   119,    36,    37,    38,
      39,    40
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -293
static const short yypact[] =
{
     255,  -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,
    -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,
     255,  -293,  -293,   255,  -293,  -293,  -293,   255,  -293,  -293,
    -293,  -293,    22,  -293,  -293,    39,   255,  -293,  -293,  -293,
    -293,   466,    -4,     0,    -9,  -293,    35,   255,  -293,  -293,
    -293,  -293,  -293,  -293,  -293,    11,    90,    12,  -293,  -293,
    -293,  -293,    44,   377,    30,   338,    26,  -293,   314,   225,
      36,  -293,    43,    48,    77,    85,    87,    89,    91,    92,
      96,    97,    98,    99,   100,   104,   105,   106,   108,   109,
     110,   111,   112,   113,   114,   116,    17,    47,   377,   -24,
     -24,  -293,  -293,  -293,   123,   446,   115,   117,  -293,    51,
    -293,   -17,   133,    68,    86,    17,  -293,  -293,    64,   135,
     314,   -24,  -293,    51,  -293,   164,    17,  -293,  -293,  -293,
      17,    17,    17,  -293,    17,    17,    17,    17,    17,    17,
      17,    17,    17,    17,    17,    17,    17,    17,  -293,  -293,
    -293,  -293,  -293,   255,   314,  -293,  -293,  -293,   182,   172,
    -293,   446,  -293,  -293,    16,  -293,  -293,   -11,   139,   -17,
     -17,  -293,  -293,  -293,    90,  -293,    16,  -293,  -293,  -293,
    -293,  -293,   426,   138,    67,   136,    17,    17,    17,   137,
     140,   141,    17,  -293,   142,   146,   159,   161,   163,   165,
     166,   169,   170,   173,   167,   191,   192,   145,   196,  -293,
    -293,  -293,  -293,   277,  -293,  -293,  -293,   314,  -293,  -293,
    -293,  -293,  -293,  -293,  -293,    69,  -293,    17,  -293,   193,
     198,   199,  -293,  -293,  -293,   197,   200,  -293,  -293,  -293,
    -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,    17,
     203,  -293,  -293,  -293,   202,  -293,   201,    17,   -10,   216,
      17,   446,  -293,  -293,  -293,   446,   446,   446,  -293,   446,
     446,   446,   446,   446,   446,   446,   446,   446,   446,   446,
     224,  -293,  -293,   226,   -10,  -293,  -293,   -10,   228,   446,
    -293,   243,   248,   446,   251,   260,   280,   283,   285,   287,
     288,   289,   290,   252,   296,  -293,   277,   298,  -293,  -293,
     304,   305,  -293,  -293,   307,  -293,  -293,  -293,  -293,  -293,
    -293,  -293,  -293,  -293,     5,     5,  -293,     5,  -293,  -293,
     314,  -293,   303,    10,  -293,   319,  -293,   328,  -293,   329,
     362,  -293,  -293,  -293,  -293,  -293
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
    -293,  -293,  -293,  -293,   297,  -293,  -293,   -65,   335,   -40,
     -42,  -293,  -293,  -293,  -293,  -293,   323,  -293,   -29,   -82,
    -293,   124,  -293,  -293,  -293,  -293,  -293,   174,  -249,  -293,
     -63,   -95,  -114,   -62,  -293,   391,  -293,   259,  -293,   -50,
    -292,   101,  -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,
    -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,
    -293,  -293,  -293,  -293,  -293,   -15,   -96,  -293,   -68,  -293,
    -293,  -293
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -93
static const short yytable[] =
{
     120,   111,   118,   111,   121,    42,   123,   111,    43,   286,
     160,   162,    44,   112,   332,   148,   216,   216,   157,   332,
     148,    48,    45,   117,   180,   116,   148,   148,   153,   149,
     154,   155,    54,   337,   149,   338,   111,    68,   308,   181,
     149,   149,    46,   111,   111,   163,   169,   170,   112,    50,
     217,   217,   120,    52,   150,   220,   221,    51,   208,    53,
     183,   148,   184,   111,    63,    56,   212,   166,    64,   165,
     333,   164,    68,   175,   148,   149,   178,   148,   177,   148,
     167,   166,   -43,   165,   176,    68,   120,   227,   149,   260,
     -92,   149,   -42,   149,    58,    59,    60,   185,   124,   111,
     151,   189,   190,   191,   125,   193,   194,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   204,   205,   206,   225,
     111,   259,   215,   173,   214,     3,     4,     5,     6,     7,
       8,     9,    10,   126,   224,    67,   223,    15,   207,    16,
     117,   127,   166,   128,    19,   129,   174,   130,   131,   120,
     257,   111,   132,   133,   134,   135,   136,   229,   230,   231,
     137,   138,   139,   235,   140,   141,   142,   143,   144,   145,
     146,   182,   147,   163,    66,     4,     5,     6,     7,     8,
       9,    10,   158,   178,    67,   215,    15,   172,    70,    71,
     179,   209,   210,    19,   219,   226,   228,   232,   250,   288,
     233,   234,   237,   290,   291,   292,   238,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,   304,   224,   239,
     280,   240,    96,   241,   247,   242,   243,   310,   284,   244,
     245,   314,    65,   246,   339,    66,     4,     5,     6,     7,
       8,     9,    10,   257,   111,    67,    68,    15,   248,    70,
      71,   251,   249,   262,    19,   263,   264,   268,   281,   282,
     269,   -51,   120,     1,     2,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,   287,    16,
      17,   305,    18,    96,    19,   309,   306,     3,     4,     5,
       6,     7,     8,     9,    10,   334,   334,    67,   334,    15,
     312,    16,    71,   253,   334,   313,    19,    20,   315,    21,
      22,    23,   324,    24,    25,    26,    27,   316,    28,    29,
      30,    31,     1,     2,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,   317,    16,    17,
     318,    18,   319,    19,   320,   321,   322,   323,    66,     4,
       5,     6,     7,     8,     9,    10,   325,   327,    67,    68,
      15,   329,    70,    71,   331,   330,    20,    19,    21,   340,
      23,   345,    24,    25,    26,    27,   342,    28,    29,    30,
      31,    58,    59,    60,    65,   343,   344,    66,     4,     5,
       6,     7,     8,     9,    10,   152,   115,    67,    68,    15,
      69,    70,    71,    72,   122,    73,    19,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,   161,
     326,   285,    49,   222,   341,    96,    66,     4,     5,     6,
       7,     8,     9,    10,     0,     0,    67,     0,    15,     0,
      70,    71,     0,     0,     0,    19,     3,     4,     5,     6,
       7,     8,     9,    10,     0,     0,     0,     0,    15,     0,
      16,     0,     0,     0,     0,    19,     3,     4,     5,     6,
       7,     8,     9,    10,   115,     0,     0,     0,     0,     0,
      16,     0,     0,     0,     0,    19
};

static const short yycheck[] =
{
      68,    63,    65,    65,    69,    20,    69,    69,    23,   258,
     105,   106,    27,    63,     9,    10,    27,    27,   100,     9,
      10,    36,     0,    65,   120,    65,    10,    10,    52,    24,
      54,    55,    47,   325,    24,   327,    98,    21,   287,   121,
      24,    24,     3,   105,   106,    56,    63,    64,    98,    53,
      61,    61,   120,    62,    96,   169,   170,    57,   154,    24,
     125,    10,   125,   125,    52,    54,   161,   109,    24,   109,
      65,    20,    21,   115,    10,    24,   118,    10,   118,    10,
     109,   123,    56,   123,    20,    21,   154,    20,    24,    20,
      60,    24,    56,    24,     4,     5,     6,   126,    55,   161,
      53,   130,   131,   132,    56,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   182,
     182,   217,   164,    55,   164,    10,    11,    12,    13,    14,
      15,    16,    17,    56,   176,    20,   176,    22,   153,    24,
     182,    56,   184,    56,    29,    56,    60,    56,    56,   217,
     213,   213,    56,    56,    56,    56,    56,   186,   187,   188,
      56,    56,    56,   192,    56,    56,    56,    56,    56,    56,
      56,     7,    56,    56,    10,    11,    12,    13,    14,    15,
      16,    17,    59,   225,    20,   227,    22,    54,    24,    25,
      55,     9,    20,    29,    55,    57,    60,    60,    53,   261,
      60,    60,    60,   265,   266,   267,    60,   269,   270,   271,
     272,   273,   274,   275,   276,   277,   278,   279,   260,    60,
     249,    60,    58,    60,    57,    60,    60,   289,   257,    60,
      60,   293,     7,    60,   330,    10,    11,    12,    13,    14,
      15,    16,    17,   306,   306,    20,    21,    22,    57,    24,
      25,    55,    60,    60,    29,    57,    57,    60,    55,    57,
      60,    60,   330,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    62,    24,
      25,    57,    27,    58,    29,    57,    60,    10,    11,    12,
      13,    14,    15,    16,    17,   324,   325,    20,   327,    22,
      57,    24,    25,    26,   333,    57,    29,    52,    57,    54,
      55,    56,    60,    58,    59,    60,    61,    57,    63,    64,
      65,    66,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    57,    24,    25,
      57,    27,    57,    29,    57,    57,    57,    57,    10,    11,
      12,    13,    14,    15,    16,    17,    60,    59,    20,    21,
      22,    57,    24,    25,    57,    60,    52,    29,    54,    66,
      56,     9,    58,    59,    60,    61,    57,    63,    64,    65,
      66,     4,     5,     6,     7,    57,    57,    10,    11,    12,
      13,    14,    15,    16,    17,    98,    58,    20,    21,    22,
      23,    24,    25,    26,    69,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,   106,
     306,   257,    41,   174,   333,    58,    10,    11,    12,    13,
      14,    15,    16,    17,    -1,    -1,    20,    -1,    22,    -1,
      24,    25,    -1,    -1,    -1,    29,    10,    11,    12,    13,
      14,    15,    16,    17,    -1,    -1,    -1,    -1,    22,    -1,
      24,    -1,    -1,    -1,    -1,    29,    10,    11,    12,    13,
      14,    15,    16,    17,    58,    -1,    -1,    -1,    -1,    -1,
      24,    -1,    -1,    -1,    -1,    29
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    24,    25,    27,    29,
      52,    54,    55,    56,    58,    59,    60,    61,    63,    64,
      65,    66,    68,   100,   102,   132,   134,   135,   136,   137,
     138,   101,   132,   132,   132,     0,     3,    69,   132,   102,
      53,    57,    62,    24,   132,    70,    54,   103,     4,     5,
       6,   104,   106,    52,    24,     7,    10,    20,    21,    23,
      24,    25,    26,    28,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    58,    71,    72,    73,
      74,    75,    76,    77,    81,    83,    84,    85,    93,    97,
      98,   100,   106,   109,   105,    58,    76,    77,    97,   133,
     135,    74,    75,    97,    55,    56,    56,    56,    56,    56,
      56,    56,    56,    56,    56,    56,    56,    56,    56,    56,
      56,    56,    56,    56,    56,    56,    56,    56,    10,    24,
      77,    53,    71,    52,    54,    55,    86,    86,    59,    78,
      98,    83,    98,    56,    20,    76,    77,    85,    94,    63,
      64,    99,    54,    55,    60,    77,    20,    76,    77,    55,
     133,    86,     7,    74,    97,    85,   111,   113,   114,    85,
      85,    85,   119,    85,    85,    85,    85,    85,    85,    85,
      85,    85,    85,    85,    85,    85,    85,   132,   133,     9,
      20,    80,    98,    82,    76,    77,    27,    61,    95,    55,
      99,    99,   104,    76,    77,    97,    57,    20,    60,    85,
      85,    85,    60,    60,    60,    85,   121,    60,    60,    60,
      60,    60,    60,    60,    60,    60,    60,    57,    57,    60,
      53,    55,    79,    26,    87,    88,    90,    97,    96,   133,
      20,   110,    60,    57,    57,   115,   117,   118,    60,    60,
     122,   124,   126,   128,   123,   125,   127,   129,   130,   131,
      85,    55,    57,    89,    85,    94,    95,    62,   100,   112,
     100,   100,   100,   120,   100,   100,   100,   100,   100,   100,
     100,   100,   100,   100,   100,    57,    60,    91,    95,    57,
     100,   116,    57,    57,   100,    57,    57,    57,    57,    57,
      57,    57,    57,    57,    60,    60,    88,    59,    92,    57,
      60,    57,     9,    65,    85,   107,   108,   107,   107,   133,
      66,   108,    57,    57,    57,     9
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

#define yyerrok      (yyerrstatus = 0)
#define yyclearin   (yychar = YYEMPTY)
#define YYEMPTY      (-2)
#define YYEOF      0

#define YYACCEPT   goto yyacceptlab
#define YYABORT      goto yyabortlab
#define YYERROR      goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL      goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)               \
do                        \
  if (yychar == YYEMPTY && yylen == 1)            \
    {                        \
      yychar = (Token);                  \
      yylval = (Value);                  \
      yytoken = YYTRANSLATE (yychar);            \
      YYPOPSTACK;                  \
      goto yybackup;                  \
    }                        \
  else                        \
    {                         \
      yyerror ("syntax error: cannot back up");\
      YYERROR;                     \
    }                        \
while (0)

#define YYTERROR   1
#define YYERRCODE   256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)      \
   ((Current).first_line   = (Rhs)[1].first_line,   \
    (Current).first_column = (Rhs)[1].first_column,   \
    (Current).last_line    = (Rhs)[N].last_line,   \
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

# define YYDPRINTF(Args)         \
do {                  \
  if (yydebug)               \
    YYFPRINTF Args;            \
} while (0)

# define YYDSYMPRINT(Args)         \
do {                  \
  if (yydebug)               \
    yysymprint Args;            \
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)      \
do {                        \
  if (yydebug)                     \
    {                        \
      YYFPRINTF (stderr, "%s ", Title);            \
      yysymprint (stderr,                \
                  Token, Value);   \
      YYFPRINTF (stderr, "\n");               \
    }                        \
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
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

# define YY_STACK_PRINT(Bottom, Top)            \
do {                        \
  if (yydebug)                     \
    yy_stack_print ((Bottom), (Top));            \
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

# define YY_REDUCE_PRINT(Rule)      \
do {               \
  if (yydebug)            \
    yy_reduce_print (Rule);      \
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
#ifndef   YYINITDEPTH
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
  short   yyssa[YYINITDEPTH];
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
  yychar = YYEMPTY;      /* Cause a token to be read.  */

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
#line 208 "vtkParse.y"
    {
      data.ClassName = vtkstrdup(yyvsp[0].str);
      ;}
    break;

  case 11:
#line 218 "vtkParse.y"
    { output_function(); ;}
    break;

  case 12:
#line 219 "vtkParse.y"
    { output_function(); ;}
    break;

  case 13:
#line 220 "vtkParse.y"
    { legacySig(); output_function(); ;}
    break;

  case 17:
#line 226 "vtkParse.y"
    { preSig("~"); ;}
    break;

  case 18:
#line 227 "vtkParse.y"
    { preSig("virtual ~"); ;}
    break;

  case 20:
#line 230 "vtkParse.y"
    {
         currentFunction->ReturnType = yyvsp[-1].integer;
    ;}
    break;

  case 21:
#line 234 "vtkParse.y"
    {
         currentFunction->ReturnType = yyvsp[-2].integer;
    ;}
    break;

  case 22:
#line 238 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-2].integer;
    ;}
    break;

  case 23:
#line 243 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-1].integer;
    ;}
    break;

  case 24:
#line 248 "vtkParse.y"
    {
         preSig("virtual ");
    ;}
    break;

  case 25:
#line 254 "vtkParse.y"
    {
         output_function();
    ;}
    break;

  case 26:
#line 258 "vtkParse.y"
    {
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
    ;}
    break;

  case 27:
#line 263 "vtkParse.y"
    {
         currentFunction->ReturnType = yyvsp[-2].integer;
         output_function();
    ;}
    break;

  case 28:
#line 268 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-2].integer;
         output_function();
    ;}
    break;

  case 29:
#line 274 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
    ;}
    break;

  case 30:
#line 280 "vtkParse.y"
    {
         preSig("virtual ");
         output_function();
    ;}
    break;

  case 31:
#line 286 "vtkParse.y"
    {
      currentFunction->IsOperator = 1;
      vtkParseDebug("Converted operator", 0);
    ;}
    break;

  case 32:
#line 291 "vtkParse.y"
    { postSig(")"); ;}
    break;

  case 33:
#line 291 "vtkParse.y"
    { postSig(";"); openSig = 0; ;}
    break;

  case 34:
#line 292 "vtkParse.y"
    {
      openSig = 1;
      currentFunction->Name = yyvsp[-3].str; 
      vtkParseDebug("Parsed func", yyvsp[-3].str);
    ;}
    break;

  case 35:
#line 298 "vtkParse.y"
    { 
      postSig(") = 0;"); 
      currentFunction->Name = yyvsp[-2].str; 
      vtkParseDebug("Parsed func", yyvsp[-2].str);
      currentFunction->IsPureVirtual = 1; 
      data.IsAbstract = 1;
    ;}
    break;

  case 37:
#line 306 "vtkParse.y"
    {postSig(" const");;}
    break;

  case 38:
#line 308 "vtkParse.y"
    {postSig(" ("); ;}
    break;

  case 40:
#line 310 "vtkParse.y"
    {postSig("const ");;}
    break;

  case 41:
#line 312 "vtkParse.y"
    {postSig("static ");;}
    break;

  case 42:
#line 314 "vtkParse.y"
    {postSig(yyvsp[0].str);;}
    break;

  case 43:
#line 314 "vtkParse.y"
    {postSig(yyvsp[0].str);;}
    break;

  case 50:
#line 323 "vtkParse.y"
    { currentFunction->NumberOfArguments++;;}
    break;

  case 51:
#line 324 "vtkParse.y"
    { currentFunction->NumberOfArguments++; postSig(", ");;}
    break;

  case 53:
#line 327 "vtkParse.y"
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
   yyvsp[0].integer;;}
    break;

  case 54:
#line 332 "vtkParse.y"
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 
   yyvsp[0].integer / 0x10000; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
   yyvsp[-1].integer + yyvsp[0].integer % 0x10000;
    ;}
    break;

  case 56:
#line 339 "vtkParse.y"
    { 
      postSig("void (*func)(void *) ");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 0x5000;
    ;}
    break;

  case 59:
#line 347 "vtkParse.y"
    {delSig();;}
    break;

  case 60:
#line 347 "vtkParse.y"
    {delSig();;}
    break;

  case 61:
#line 349 "vtkParse.y"
    { yyval.integer = yyvsp[0].integer; ;}
    break;

  case 62:
#line 357 "vtkParse.y"
    { yyval.integer = 0; ;}
    break;

  case 63:
#line 358 "vtkParse.y"
    { char temp[100]; sprintf(temp,"[%i]",yyvsp[0].integer); 
                   postSig(temp); ;}
    break;

  case 64:
#line 360 "vtkParse.y"
    { yyval.integer = 0x300 + 0x10000 * yyvsp[-2].integer + yyvsp[0].integer % 0x1000; ;}
    break;

  case 65:
#line 362 "vtkParse.y"
    { postSig("[]"); yyval.integer = 0x300 + yyvsp[0].integer % 0x1000; ;}
    break;

  case 66:
#line 364 "vtkParse.y"
    {yyval.integer = 0x1000 + yyvsp[0].integer;;}
    break;

  case 67:
#line 365 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;;}
    break;

  case 68:
#line 366 "vtkParse.y"
    {yyval.integer = 0x2000 + yyvsp[0].integer;;}
    break;

  case 69:
#line 367 "vtkParse.y"
    {yyval.integer = 0x3000 + yyvsp[0].integer;;}
    break;

  case 70:
#line 369 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;;}
    break;

  case 71:
#line 371 "vtkParse.y"
    {yyval.integer = yyvsp[-1].integer + yyvsp[0].integer;;}
    break;

  case 72:
#line 380 "vtkParse.y"
    { postSig("&"); yyval.integer = 0x100;;}
    break;

  case 73:
#line 381 "vtkParse.y"
    { postSig("*"); yyval.integer = 0x300;;}
    break;

  case 74:
#line 382 "vtkParse.y"
    { yyval.integer = 0x100 + yyvsp[0].integer;;}
    break;

  case 75:
#line 383 "vtkParse.y"
    { yyval.integer = 0x400 + yyvsp[0].integer;;}
    break;

  case 76:
#line 385 "vtkParse.y"
    {postSig("unsigned ");;}
    break;

  case 77:
#line 386 "vtkParse.y"
    { yyval.integer = 0x10 + yyvsp[0].integer;;}
    break;

  case 78:
#line 387 "vtkParse.y"
    { yyval.integer = yyvsp[0].integer;;}
    break;

  case 79:
#line 390 "vtkParse.y"
    { postSig("float "); yyval.integer = 0x1;;}
    break;

  case 80:
#line 391 "vtkParse.y"
    { postSig("void "); yyval.integer = 0x2;;}
    break;

  case 81:
#line 392 "vtkParse.y"
    { postSig("char "); yyval.integer = 0x3;;}
    break;

  case 82:
#line 393 "vtkParse.y"
    { postSig("int "); yyval.integer = 0x4;;}
    break;

  case 83:
#line 394 "vtkParse.y"
    { postSig("short "); yyval.integer = 0x5;;}
    break;

  case 84:
#line 395 "vtkParse.y"
    { postSig("long "); yyval.integer = 0x6;;}
    break;

  case 85:
#line 396 "vtkParse.y"
    { postSig("double "); yyval.integer = 0x7;;}
    break;

  case 86:
#line 397 "vtkParse.y"
    {       
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",yyvsp[0].str);
      postSig(ctmpid);
      yyval.integer = 0x8;;}
    break;

  case 87:
#line 403 "vtkParse.y"
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
    ;}
    break;

  case 88:
#line 419 "vtkParse.y"
    { postSig("vtkIdType "); yyval.integer = 0xA;;}
    break;

  case 91:
#line 424 "vtkParse.y"
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    ;}
    break;

  case 92:
#line 429 "vtkParse.y"
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    ;}
    break;

  case 94:
#line 434 "vtkParse.y"
    {in_public = 1; in_protected = 0;;}
    break;

  case 95:
#line 435 "vtkParse.y"
    {in_public = 0; in_protected = 0;;}
    break;

  case 96:
#line 436 "vtkParse.y"
    {in_public = 0; in_protected = 1;;}
    break;

  case 99:
#line 440 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;;}
    break;

  case 100:
#line 441 "vtkParse.y"
    {yyval.integer = -1;;}
    break;

  case 101:
#line 441 "vtkParse.y"
    {yyval.integer = -1;;}
    break;

  case 102:
#line 445 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); ;}
    break;

  case 103:
#line 446 "vtkParse.y"
    {
   postSig(");");
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();
   ;}
    break;

  case 104:
#line 456 "vtkParse.y"
    {postSig("Get");;}
    break;

  case 105:
#line 456 "vtkParse.y"
    {postSig(" ();"); invertSig = 1;;}
    break;

  case 106:
#line 458 "vtkParse.y"
    { 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = yyvsp[-1].integer;
   output_function();
   ;}
    break;

  case 107:
#line 465 "vtkParse.y"
    {preSig("void Set");;}
    break;

  case 108:
#line 466 "vtkParse.y"
    {
   postSig(" (char *);"); 
   sprintf(temps,"Set%s",yyvsp[-1].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x303;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();
   ;}
    break;

  case 109:
#line 476 "vtkParse.y"
    {preSig("char *Get");;}
    break;

  case 110:
#line 477 "vtkParse.y"
    { 
   postSig(" ();");
   sprintf(temps,"Get%s",yyvsp[-1].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x303;
   output_function();
   ;}
    break;

  case 111:
#line 486 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); ;}
    break;

  case 112:
#line 487 "vtkParse.y"
    {postSig(");"); openSig = 0;;}
    break;

  case 113:
#line 488 "vtkParse.y"
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
   ;}
    break;

  case 114:
#line 518 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); ;}
    break;

  case 115:
#line 519 "vtkParse.y"
    { 
   postSig("*);");
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 0x2;
   output_function();
   ;}
    break;

  case 116:
#line 530 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); ;}
    break;

  case 117:
#line 531 "vtkParse.y"
    { 
   postSig("*);");
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 0x2;
   output_function();
   ;}
    break;

  case 118:
#line 541 "vtkParse.y"
    {postSig("*Get");;}
    break;

  case 119:
#line 542 "vtkParse.y"
    {postSig(" ();"); invertSig = 1;;}
    break;

  case 120:
#line 543 "vtkParse.y"
    { 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x309;
   output_function();
   ;}
    break;

  case 121:
#line 551 "vtkParse.y"
    {preSig("void "); postSig("On ();"); openSig = 0; ;}
    break;

  case 122:
#line 553 "vtkParse.y"
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
   ;}
    break;

  case 123:
#line 568 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 124:
#line 573 "vtkParse.y"
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
   ;}
    break;

  case 125:
#line 598 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 126:
#line 603 "vtkParse.y"
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
   ;}
    break;

  case 127:
#line 615 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 128:
#line 620 "vtkParse.y"
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
   ;}
    break;

  case 129:
#line 647 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 130:
#line 652 "vtkParse.y"
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
   ;}
    break;

  case 131:
#line 664 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 132:
#line 669 "vtkParse.y"
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
   ;}
    break;

  case 133:
#line 698 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 134:
#line 703 "vtkParse.y"
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
   ;}
    break;

  case 135:
#line 715 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 136:
#line 720 "vtkParse.y"
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
   ;}
    break;

  case 137:
#line 753 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 138:
#line 758 "vtkParse.y"
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
   ;}
    break;

  case 139:
#line 770 "vtkParse.y"
    {
      free (currentFunction->Signature);
      currentFunction->Signature = NULL;
      ;}
    break;

  case 140:
#line 775 "vtkParse.y"
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
   ;}
    break;

  case 141:
#line 788 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 142:
#line 793 "vtkParse.y"
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
   ;}
    break;

  case 143:
#line 805 "vtkParse.y"
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
   ;}
    break;

  case 144:
#line 852 "vtkParse.y"
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
   ;}
    break;

  case 145:
#line 901 "vtkParse.y"
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
   ;}
    break;


    }

/* Line 1000 of yacc.c.  */
#line 2654 "vtkParse.tab.c"

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
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

#ifdef __GNUC__
  /* Pacify GCC when the user code never invokes YYERROR and the label
     yyerrorlab therefore never appears in user code.  */
  if (0)
     goto yyerrorlab;
#endif

  yyvsp -= yylen;
  yyssp -= yylen;
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;   /* Each real token shifted decrements this.  */

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


#line 969 "vtkParse.y"

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
      case 0x301: case 0x302: case 0x307: case 0x30A:
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




