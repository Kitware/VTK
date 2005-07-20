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
     SIGNED_CHAR = 275,
     CLASS_REF = 276,
     OTHER = 277,
     CONST = 278,
     OPERATOR = 279,
     UNSIGNED = 280,
     FRIEND = 281,
     VTK_ID = 282,
     STATIC = 283,
     VAR_FUNCTION = 284,
     ARRAY_NUM = 285,
     VTK_LEGACY = 286,
     IdType = 287,
     StdString = 288,
     SetMacro = 289,
     GetMacro = 290,
     SetStringMacro = 291,
     GetStringMacro = 292,
     SetClampMacro = 293,
     SetObjectMacro = 294,
     SetReferenceCountedObjectMacro = 295,
     GetObjectMacro = 296,
     BooleanMacro = 297,
     SetVector2Macro = 298,
     SetVector3Macro = 299,
     SetVector4Macro = 300,
     SetVector6Macro = 301,
     GetVector2Macro = 302,
     GetVector3Macro = 303,
     GetVector4Macro = 304,
     GetVector6Macro = 305,
     SetVectorMacro = 306,
     GetVectorMacro = 307,
     ViewportCoordinateMacro = 308,
     WorldCoordinateMacro = 309,
     TypeMacro = 310
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
#define SIGNED_CHAR 275
#define CLASS_REF 276
#define OTHER 277
#define CONST 278
#define OPERATOR 279
#define UNSIGNED 280
#define FRIEND 281
#define VTK_ID 282
#define STATIC 283
#define VAR_FUNCTION 284
#define ARRAY_NUM 285
#define VTK_LEGACY 286
#define IdType 287
#define StdString 288
#define SetMacro 289
#define GetMacro 290
#define SetStringMacro 291
#define GetStringMacro 292
#define SetClampMacro 293
#define SetObjectMacro 294
#define SetReferenceCountedObjectMacro 295
#define GetObjectMacro 296
#define BooleanMacro 297
#define SetVector2Macro 298
#define SetVector3Macro 299
#define SetVector4Macro 300
#define SetVector6Macro 301
#define GetVector2Macro 302
#define GetVector3Macro 303
#define GetVector4Macro 304
#define GetVector6Macro 305
#define SetVectorMacro 306
#define GetVectorMacro 307
#define ViewportCoordinateMacro 308
#define WorldCoordinateMacro 309
#define TypeMacro 310




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
#line 320 "vtkParse.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 332 "vtkParse.tab.c"

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
#define YYFINAL  50
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   626

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  71
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  74
/* YYNRULES -- Number of rules. */
#define YYNRULES  183
/* YYNRULES -- Number of states. */
#define YYNSTATES  355

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   310

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    67,     2,
      60,    61,    68,     2,    64,    69,    70,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    58,    59,
       2,    63,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    65,     2,    66,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    56,     2,    57,    62,     2,     2,     2,
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
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55
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
     252,   254,   256,   258,   260,   262,   264,   266,   267,   270,
     273,   274,   280,   282,   284,   286,   289,   291,   293,   297,
     299,   300,   308,   309,   310,   319,   320,   326,   327,   333,
     334,   335,   346,   347,   355,   356,   364,   365,   366,   375,
     376,   384,   385,   393,   394,   402,   403,   411,   412,   420,
     421,   429,   430,   438,   439,   447,   448,   456,   457,   467,
     468,   478,   483,   488,   495,   496,   499,   500,   503,   505,
     507,   509,   511,   513,   515,   517,   519,   521,   523,   525,
     527,   529,   531,   533,   535,   537,   539,   541,   543,   545,
     547,   549,   553,   557
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short int yyrhs[] =
{
      72,     0,    -1,   138,    73,   138,    -1,    -1,     3,    27,
      74,   109,    56,    75,    57,    -1,    76,    -1,    76,    75,
      -1,   112,    58,    -1,    97,    -1,    79,    -1,    26,    79,
      -1,    78,    90,    -1,    26,    78,    90,    -1,    77,    90,
      -1,   115,    59,    -1,   115,    -1,    31,    60,    78,    61,
      -1,    62,    81,    -1,     7,    62,    81,    -1,    81,    -1,
     101,    81,    -1,   101,    23,    81,    -1,     7,   101,    23,
      81,    -1,     7,   101,    81,    -1,     7,    81,    -1,    80,
      -1,   101,    80,    -1,   101,    23,    80,    -1,     7,   101,
      23,    80,    -1,     7,   101,    80,    -1,     7,    80,    -1,
      24,   139,    59,    -1,    -1,    -1,    85,    82,    84,    83,
      -1,    85,    63,     9,    -1,    -1,    23,    -1,    -1,    89,
      60,    86,    91,    61,    -1,    23,    -1,    28,    -1,    27,
      -1,    10,    -1,    59,    -1,    56,   138,    57,    59,    -1,
      56,   138,    57,    -1,    58,   139,    59,    -1,    -1,    92,
      -1,    94,    -1,    -1,    94,    93,    64,    92,    -1,   101,
      -1,    -1,   101,    98,    95,    96,    -1,    29,    -1,    -1,
      63,   113,    -1,   101,    98,    59,    -1,    29,    59,    -1,
      89,    99,    -1,    -1,    -1,    30,   100,    99,    -1,    65,
     139,    66,    99,    -1,    87,   102,    -1,   102,    -1,    88,
     102,    -1,    88,    87,   102,    -1,   106,    -1,   106,   105,
      -1,   103,    -1,   104,    -1,   104,    67,    -1,   104,    68,
      -1,    33,    -1,    67,    -1,    68,    -1,    67,   105,    -1,
      68,   105,    -1,    -1,    25,   107,   108,    -1,   108,    -1,
      12,    -1,    18,    -1,    19,    -1,    11,    -1,    13,    -1,
      14,    -1,    17,    -1,    10,    -1,    27,    -1,    32,    -1,
      15,    -1,    16,    -1,    20,    -1,    -1,    58,   110,    -1,
     112,    27,    -1,    -1,   112,    27,   111,    64,   110,    -1,
       4,    -1,     5,    -1,     6,    -1,    69,   114,    -1,   114,
      -1,     9,    -1,     9,    70,     9,    -1,    89,    -1,    -1,
      34,    60,    89,    64,   116,   106,    61,    -1,    -1,    -1,
      35,    60,   117,    89,    64,   118,   106,    61,    -1,    -1,
      36,    60,   119,    89,    61,    -1,    -1,    37,    60,   120,
      89,    61,    -1,    -1,    -1,    38,    60,    89,    64,   121,
     106,   122,    64,   139,    61,    -1,    -1,    39,    60,    89,
      64,   123,   106,    61,    -1,    -1,    40,    60,    89,    64,
     124,   106,    61,    -1,    -1,    -1,    41,    60,   125,    89,
      64,   126,   106,    61,    -1,    -1,    42,    60,    89,   127,
      64,   106,    61,    -1,    -1,    43,    60,    89,    64,   128,
     106,    61,    -1,    -1,    47,    60,    89,    64,   129,   106,
      61,    -1,    -1,    44,    60,    89,    64,   130,   106,    61,
      -1,    -1,    48,    60,    89,    64,   131,   106,    61,    -1,
      -1,    45,    60,    89,    64,   132,   106,    61,    -1,    -1,
      49,    60,    89,    64,   133,   106,    61,    -1,    -1,    46,
      60,    89,    64,   134,   106,    61,    -1,    -1,    50,    60,
      89,    64,   135,   106,    61,    -1,    -1,    51,    60,    89,
      64,   136,   106,    64,   113,    61,    -1,    -1,    52,    60,
      89,    64,   137,   106,    64,   113,    61,    -1,    53,    60,
      89,    61,    -1,    54,    60,    89,    61,    -1,    55,    60,
      89,    64,    89,    61,    -1,    -1,   140,   138,    -1,    -1,
     141,   139,    -1,    59,    -1,   141,    -1,    22,    -1,   142,
      -1,   143,    -1,    68,    -1,    63,    -1,    58,    -1,    64,
      -1,    70,    -1,     8,    -1,   106,    -1,   104,    -1,     9,
      -1,    21,    -1,    67,    -1,   144,    -1,    23,    -1,    24,
      -1,    69,    -1,    62,    -1,    28,    -1,    30,    -1,    56,
     138,    57,    -1,    60,   138,    61,    -1,    65,   138,    66,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   209,   209,   212,   211,   217,   217,   219,   219,   220,
     221,   222,   223,   224,   225,   226,   228,   230,   231,   232,
     233,   237,   241,   246,   251,   257,   261,   266,   271,   277,
     283,   289,   295,   295,   295,   301,   310,   310,   312,   312,
     314,   316,   318,   318,   320,   321,   322,   323,   325,   325,
     327,   328,   328,   330,   336,   335,   342,   349,   349,   351,
     351,   353,   361,   362,   362,   365,   368,   369,   370,   371,
     373,   374,   376,   378,   379,   380,   382,   392,   393,   394,
     395,   397,   397,   399,   402,   403,   404,   405,   406,   407,
     408,   409,   414,   431,   432,   433,   434,   436,   436,   438,
     444,   443,   449,   450,   451,   453,   453,   455,   456,   456,
     460,   459,   471,   471,   471,   480,   480,   491,   491,   501,
     502,   500,   533,   532,   545,   544,   556,   557,   556,   566,
     565,   583,   582,   613,   612,   630,   629,   662,   661,   679,
     678,   713,   712,   730,   729,   768,   767,   785,   784,   803,
     802,   819,   866,   915,   971,   971,   972,   972,   974,   974,
     976,   976,   976,   976,   976,   976,   976,   976,   977,   977,
     977,   977,   977,   977,   977,   978,   978,   978,   978,   978,
     978,   980,   981,   982
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "CLASS", "PUBLIC", "PRIVATE",
  "PROTECTED", "VIRTUAL", "STRING", "NUM", "ID", "INT", "FLOAT", "SHORT",
  "LONG", "LONG_LONG", "INT64__", "DOUBLE", "VOID", "CHAR", "SIGNED_CHAR",
  "CLASS_REF", "OTHER", "CONST", "OPERATOR", "UNSIGNED", "FRIEND",
  "VTK_ID", "STATIC", "VAR_FUNCTION", "ARRAY_NUM", "VTK_LEGACY", "IdType",
  "StdString", "SetMacro", "GetMacro", "SetStringMacro", "GetStringMacro",
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
     305,   306,   307,   308,   309,   310,   123,   125,    58,    59,
      40,    41,   126,    61,    44,    91,    93,    38,    42,    45,
      46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    71,    72,    74,    73,    75,    75,    76,    76,    76,
      76,    76,    76,    76,    76,    76,    77,    78,    78,    78,
      78,    78,    78,    78,    78,    79,    79,    79,    79,    79,
      79,    80,    82,    83,    81,    81,    84,    84,    86,    85,
      87,    88,    89,    89,    90,    90,    90,    90,    91,    91,
      92,    93,    92,    94,    95,    94,    94,    96,    96,    97,
      97,    98,    99,   100,    99,    99,   101,   101,   101,   101,
     102,   102,   102,   103,   103,   103,   104,   105,   105,   105,
     105,   107,   106,   106,   108,   108,   108,   108,   108,   108,
     108,   108,   108,   108,   108,   108,   108,   109,   109,   110,
     111,   110,   112,   112,   112,   113,   113,   114,   114,   114,
     116,   115,   117,   118,   115,   119,   115,   120,   115,   121,
     122,   115,   123,   115,   124,   115,   125,   126,   115,   127,
     115,   128,   115,   129,   115,   130,   115,   131,   115,   132,
     115,   133,   115,   134,   115,   135,   115,   136,   115,   137,
     115,   115,   115,   115,   138,   138,   139,   139,   140,   140,
     141,   141,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   142,   143,   144
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
       1,     1,     1,     1,     1,     1,     1,     0,     2,     2,
       0,     5,     1,     1,     1,     2,     1,     1,     3,     1,
       0,     7,     0,     0,     8,     0,     5,     0,     5,     0,
       0,    10,     0,     7,     0,     7,     0,     0,     8,     0,
       7,     0,     7,     0,     7,     0,     7,     0,     7,     0,
       7,     0,     7,     0,     7,     0,     7,     0,     9,     0,
       9,     4,     4,     6,     0,     2,     0,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     3,     3,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
     154,   168,   171,    91,    87,    84,    88,    89,    94,    95,
      90,    85,    86,    96,   172,   160,   175,   176,    81,    92,
     179,   180,    93,    76,   154,   165,   158,   154,   178,   164,
     166,   154,   173,   163,   177,   167,     0,   170,   169,    83,
       0,   154,   159,   161,   162,   174,     0,     0,     0,     0,
       1,     0,   154,   155,    82,   181,   182,   183,     3,     2,
      97,     0,     0,   102,   103,   104,    98,     0,     0,    99,
       0,    91,    40,   156,     0,    92,    41,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     5,     0,     0,     9,    25,    19,    32,
       0,     0,     0,     8,     0,    67,    72,    73,    70,     0,
      15,     0,     0,    30,    24,     0,     0,   156,     0,    10,
       0,    60,     0,     0,   112,   115,   117,     0,     0,     0,
     126,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    43,    42,    17,     4,     6,
     154,   156,    44,    13,    11,     0,    36,    66,     0,    68,
      38,     0,    26,    20,    62,     0,    74,    75,    77,    78,
      71,     7,    14,     0,    18,     0,    29,    23,    31,   157,
      12,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   129,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    35,    37,
      33,    69,    48,    27,    21,    63,   156,    61,    59,    79,
      80,   101,    28,    22,     0,    16,     0,   110,     0,     0,
       0,   119,   122,   124,     0,     0,   131,   135,   139,   143,
     133,   137,   141,   145,   147,   149,   151,   152,     0,    46,
      47,    34,    56,     0,    49,    50,    53,    62,     0,     0,
       0,   113,   116,   118,     0,     0,     0,   127,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      45,    39,     0,    62,    54,    64,    62,     0,     0,   120,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   153,     0,    57,    65,   111,     0,
       0,   123,   125,     0,   130,   132,   136,   140,   144,   134,
     138,   142,   146,     0,     0,    52,     0,    55,   114,   156,
     128,   107,     0,   109,     0,   106,     0,    58,     0,     0,
     105,   148,   150,   121,   108
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,    36,    52,    60,   102,   103,   104,   105,   106,   107,
     108,   166,   261,   220,   109,   222,   110,   111,   112,   163,
     263,   264,   292,   265,   316,   337,   113,   175,   227,   267,
     114,   115,   116,    37,   180,    38,    46,    39,    62,    66,
     121,    67,   344,   345,   120,   270,   195,   298,   196,   197,
     274,   320,   275,   276,   201,   302,   245,   279,   283,   280,
     284,   281,   285,   282,   286,   287,   288,    40,   126,    41,
      42,    43,    44,    45
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -254
static const short int yypact[] =
{
     276,  -254,  -254,  -254,  -254,  -254,  -254,  -254,  -254,  -254,
    -254,  -254,  -254,  -254,  -254,  -254,  -254,  -254,  -254,  -254,
    -254,  -254,  -254,  -254,   276,  -254,  -254,   276,  -254,  -254,
    -254,   276,  -254,  -254,  -254,  -254,    27,  -254,  -254,  -254,
      37,   276,  -254,  -254,  -254,  -254,   364,    21,    29,    16,
    -254,    59,   276,  -254,  -254,  -254,  -254,  -254,  -254,  -254,
      31,    53,    39,  -254,  -254,  -254,  -254,    69,   406,    34,
     514,    40,  -254,   339,   459,    42,  -254,    44,    47,    48,
      51,    54,    55,    58,    60,    62,    63,    65,    75,    91,
      92,    94,    96,    99,   100,   101,   103,   115,   118,   120,
     123,    28,    49,   406,   -30,   -30,  -254,  -254,  -254,    36,
     593,   154,   125,  -254,    46,  -254,  -254,   -34,    -4,    61,
      71,   124,    28,  -254,  -254,    70,   130,   339,   -30,  -254,
      46,  -254,   483,    28,  -254,  -254,  -254,    28,    28,    28,
    -254,    28,    28,    28,    28,    28,    28,    28,    28,    28,
      28,    28,    28,    28,    28,  -254,  -254,  -254,  -254,  -254,
     276,   339,  -254,  -254,  -254,   184,    89,  -254,   593,  -254,
    -254,    25,  -254,  -254,   -28,   135,  -254,  -254,    -4,    -4,
    -254,  -254,  -254,    53,  -254,    25,  -254,  -254,  -254,  -254,
    -254,   538,    68,    52,   131,    28,    28,    28,   133,   134,
     136,    28,  -254,   137,   138,   140,   141,   142,   146,   158,
     160,   162,   164,   168,   169,   170,   175,   174,  -254,  -254,
    -254,  -254,   569,  -254,  -254,  -254,   339,  -254,  -254,  -254,
    -254,  -254,  -254,  -254,    64,  -254,    28,  -254,   172,   188,
     194,  -254,  -254,  -254,   192,   193,  -254,  -254,  -254,  -254,
    -254,  -254,  -254,  -254,  -254,  -254,  -254,  -254,    28,   199,
    -254,  -254,  -254,   201,  -254,   200,    28,   -15,   197,    28,
     227,  -254,  -254,  -254,   227,   227,   227,  -254,   227,   227,
     227,   227,   227,   227,   227,   227,   227,   227,   227,   204,
    -254,  -254,   203,   -15,  -254,  -254,   -15,   207,   227,  -254,
     208,   209,   227,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   238,  -254,   569,   242,  -254,  -254,   220,
     243,  -254,  -254,   221,  -254,  -254,  -254,  -254,  -254,  -254,
    -254,  -254,  -254,     3,     3,  -254,     3,  -254,  -254,   339,
    -254,   129,    14,  -254,   249,  -254,   250,  -254,   251,   271,
    -254,  -254,  -254,  -254,  -254
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -254,  -254,  -254,  -254,   222,  -254,  -254,   -66,   239,   -54,
      -9,  -254,  -254,  -254,  -254,  -254,   205,  -254,    -5,   -80,
    -254,    -1,  -254,  -254,  -254,  -254,  -254,    56,  -245,  -254,
     -65,   -91,  -254,   -64,   -94,   -67,  -254,   269,  -254,   143,
    -254,   -50,  -253,   -25,  -254,  -254,  -254,  -254,  -254,  -254,
    -254,  -254,  -254,  -254,  -254,  -254,  -254,  -254,  -254,  -254,
    -254,  -254,  -254,  -254,  -254,  -254,  -254,   -10,  -116,  -254,
     -73,  -254,  -254,  -254
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -101
static const short int yytable[] =
{
     127,   118,   225,   118,   117,   125,   117,   118,   128,   130,
     117,   189,   341,   155,    47,   225,   123,    48,   119,   167,
     169,    49,   295,   341,   155,   164,   160,    50,   161,   162,
     156,    53,   170,   176,   177,   155,   118,   226,   155,   117,
      51,   156,    59,   118,   118,   217,   117,   117,   190,    73,
     226,   317,   156,   119,   127,   156,   155,    63,    64,    65,
     172,   124,   155,   178,   179,   118,   192,   193,   117,   171,
      73,   186,   342,   156,   155,   236,   172,   221,    55,   156,
     155,   346,    57,   347,   229,   230,    58,   269,   127,    61,
      56,   156,   157,   185,    73,    68,    69,   156,  -100,   165,
     -43,   118,   -42,   131,   117,   173,   158,   132,   133,   174,
     268,   134,   219,   184,   135,   136,   187,   223,   137,   181,
     138,   173,   139,   140,   118,   141,   234,   117,   194,   235,
     182,   232,   198,   199,   200,   142,   202,   203,   204,   205,
     206,   207,   208,   209,   210,   211,   212,   213,   214,   215,
     216,   143,   144,   127,   145,   118,   146,   266,   117,   147,
     148,   149,   224,   150,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,   151,   233,    72,   152,    18,
     153,    19,   124,   154,   173,   170,    22,    23,   183,   188,
     238,   239,   240,   218,   228,   237,   244,   241,   242,   349,
     243,   246,   247,   297,   248,   249,   250,   299,   300,   301,
     251,   303,   304,   305,   306,   307,   308,   309,   310,   311,
     312,   313,   252,   348,   253,   187,   254,   224,   255,   256,
     257,   319,   259,   260,   258,   323,   271,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,   118,   272,
     266,   117,    18,   289,    19,   273,   277,   278,   290,    22,
     233,   293,   291,   296,   -51,   314,   127,   315,   318,   321,
     322,   324,   325,   326,   327,   328,   329,   330,   331,   332,
     354,   338,   340,   333,     1,     2,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,   334,    19,    20,   336,    21,   339,    22,    23,
     351,   352,   353,   129,   335,    54,   168,   350,     0,     0,
       0,     0,   294,     0,     0,   159,   231,     0,   343,   343,
       0,   343,    24,     0,    25,    26,    27,   343,    28,    29,
      30,    31,     0,    32,    33,    34,    35,     1,     2,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,     0,    19,    20,     0,    21,
       0,    22,    23,     0,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,     0,     0,     0,     0,     0,
       0,    19,     0,     0,     0,    24,    22,    25,     0,    27,
       0,    28,    29,    30,    31,     0,    32,    33,    34,    35,
      63,    64,    65,    70,     0,     0,    71,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,     0,     0,    72,
      73,    18,    74,    75,    76,    77,     0,    78,    22,    23,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,     0,     0,     0,     0,    70,     0,   101,    71,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
       0,     0,    72,    73,    18,     0,    75,    76,     0,     0,
     191,    22,    23,    71,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,     0,     0,    72,     0,    18,     0,
      75,    76,     0,     0,     0,    22,    23,     0,     0,     0,
       0,   101,     0,     0,    71,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,     0,     0,    72,    73,    18,
       0,    75,    76,     0,     0,   101,    22,    23,    71,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,     0,
       0,    72,     0,    18,     0,    75,    76,     0,     0,     0,
      22,    23,     0,     0,     0,     0,   122,     0,     0,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
       0,     0,    72,     0,    18,     0,    19,    76,   262,     0,
     122,    22,    23,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,     0,     0,     0,     0,    18,     0,
      19,     0,     0,     0,     0,    22,    23
};

static const short int yycheck[] =
{
      73,    68,    30,    70,    68,    70,    70,    74,    74,    74,
      74,   127,     9,    10,    24,    30,    70,    27,    68,   110,
     111,    31,   267,     9,    10,   105,    56,     0,    58,    59,
      27,    41,    60,    67,    68,    10,   103,    65,    10,   103,
       3,    27,    52,   110,   111,   161,   110,   111,   128,    24,
      65,   296,    27,   103,   127,    27,    10,     4,     5,     6,
     114,    70,    10,    67,    68,   132,   132,   132,   132,    23,
      24,   125,    69,    27,    10,    23,   130,   168,    57,    27,
      10,   334,    66,   336,   178,   179,    27,    23,   161,    58,
      61,    27,   101,    23,    24,    56,    27,    27,    64,    63,
      60,   168,    60,    59,   168,   114,    57,    60,    60,   114,
     226,    60,    23,   122,    60,    60,   125,   171,    60,    58,
      60,   130,    60,    60,   191,    60,   191,   191,   133,    61,
      59,   185,   137,   138,   139,    60,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     160,    60,    60,   226,    60,   222,    60,   222,   222,    60,
      60,    60,   171,    60,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    60,   185,    23,    60,    25,
      60,    27,   191,    60,   193,    60,    32,    33,    64,    59,
     195,   196,   197,     9,    59,    64,   201,    64,    64,    70,
      64,    64,    64,   270,    64,    64,    64,   274,   275,   276,
      64,   278,   279,   280,   281,   282,   283,   284,   285,   286,
     287,   288,    64,   339,    64,   234,    64,   236,    64,    61,
      61,   298,    57,    59,    64,   302,    64,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,   315,    61,
     315,   315,    25,   258,    27,    61,    64,    64,    59,    32,
     269,   266,    61,    66,    64,    61,   339,    64,    61,    61,
      61,    61,    61,    61,    61,    61,    61,    61,    61,    61,
       9,    61,    61,    64,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    64,    27,    28,    63,    30,    64,    32,    33,
      61,    61,    61,    74,   315,    46,   111,   342,    -1,    -1,
      -1,    -1,   266,    -1,    -1,   103,   183,    -1,   333,   334,
      -1,   336,    56,    -1,    58,    59,    60,   342,    62,    63,
      64,    65,    -1,    67,    68,    69,    70,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    -1,    27,    28,    -1,    30,
      -1,    32,    33,    -1,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    -1,    -1,    -1,    -1,    -1,
      -1,    27,    -1,    -1,    -1,    56,    32,    58,    -1,    60,
      -1,    62,    63,    64,    65,    -1,    67,    68,    69,    70,
       4,     5,     6,     7,    -1,    -1,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    -1,    -1,    23,
      24,    25,    26,    27,    28,    29,    -1,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    -1,    -1,    -1,    -1,     7,    -1,    62,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      -1,    -1,    23,    24,    25,    -1,    27,    28,    -1,    -1,
       7,    32,    33,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    -1,    -1,    23,    -1,    25,    -1,
      27,    28,    -1,    -1,    -1,    32,    33,    -1,    -1,    -1,
      -1,    62,    -1,    -1,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    -1,    -1,    23,    24,    25,
      -1,    27,    28,    -1,    -1,    62,    32,    33,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    -1,
      -1,    23,    -1,    25,    -1,    27,    28,    -1,    -1,    -1,
      32,    33,    -1,    -1,    -1,    -1,    62,    -1,    -1,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      -1,    -1,    23,    -1,    25,    -1,    27,    28,    29,    -1,
      62,    32,    33,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    -1,    -1,    -1,    -1,    25,    -1,
      27,    -1,    -1,    -1,    -1,    32,    33
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    27,
      28,    30,    32,    33,    56,    58,    59,    60,    62,    63,
      64,    65,    67,    68,    69,    70,    72,   104,   106,   108,
     138,   140,   141,   142,   143,   144,   107,   138,   138,   138,
       0,     3,    73,   138,   108,    57,    61,    66,    27,   138,
      74,    58,   109,     4,     5,     6,   110,   112,    56,    27,
       7,    10,    23,    24,    26,    27,    28,    29,    31,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    62,    75,    76,    77,    78,    79,    80,    81,    85,
      87,    88,    89,    97,   101,   102,   103,   104,   106,   112,
     115,   111,    62,    80,    81,   101,   139,   141,    78,    79,
     101,    59,    60,    60,    60,    60,    60,    60,    60,    60,
      60,    60,    60,    60,    60,    60,    60,    60,    60,    60,
      60,    60,    60,    60,    60,    10,    27,    81,    57,    75,
      56,    58,    59,    90,    90,    63,    82,   102,    87,   102,
      60,    23,    80,    81,    89,    98,    67,    68,    67,    68,
     105,    58,    59,    64,    81,    23,    80,    81,    59,   139,
      90,     7,    78,   101,    89,   117,   119,   120,    89,    89,
      89,   125,    89,    89,    89,    89,    89,    89,    89,    89,
      89,    89,    89,    89,    89,    89,   138,   139,     9,    23,
      84,   102,    86,    80,    81,    30,    65,    99,    59,   105,
     105,   110,    80,    81,   101,    61,    23,    64,    89,    89,
      89,    64,    64,    64,    89,   127,    64,    64,    64,    64,
      64,    64,    64,    64,    64,    64,    61,    61,    64,    57,
      59,    83,    29,    91,    92,    94,   101,   100,   139,    23,
     116,    64,    61,    61,   121,   123,   124,    64,    64,   128,
     130,   132,   134,   129,   131,   133,   135,   136,   137,    89,
      59,    61,    93,    89,    98,    99,    66,   106,   118,   106,
     106,   106,   126,   106,   106,   106,   106,   106,   106,   106,
     106,   106,   106,   106,    61,    64,    95,    99,    61,   106,
     122,    61,    61,   106,    61,    61,    61,    61,    61,    61,
      61,    61,    61,    64,    64,    92,    63,    96,    61,    64,
      61,     9,    69,    89,   113,   114,   113,   113,   139,    70,
     114,    61,    61,    61,     9
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
#line 212 "vtkParse.y"
    {
      data.ClassName = vtkstrdup(yyvsp[0].str);
      }
    break;

  case 11:
#line 222 "vtkParse.y"
    { output_function(); }
    break;

  case 12:
#line 223 "vtkParse.y"
    { output_function(); }
    break;

  case 13:
#line 224 "vtkParse.y"
    { legacySig(); output_function(); }
    break;

  case 17:
#line 230 "vtkParse.y"
    { preSig("~"); }
    break;

  case 18:
#line 231 "vtkParse.y"
    { preSig("virtual ~"); }
    break;

  case 20:
#line 234 "vtkParse.y"
    {
         currentFunction->ReturnType = yyvsp[-1].integer;
         }
    break;

  case 21:
#line 238 "vtkParse.y"
    {
         currentFunction->ReturnType = yyvsp[-2].integer;
         }
    break;

  case 22:
#line 242 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-2].integer;
         }
    break;

  case 23:
#line 247 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-1].integer;
         }
    break;

  case 24:
#line 252 "vtkParse.y"
    {
         preSig("virtual ");
         }
    break;

  case 25:
#line 258 "vtkParse.y"
    {
         output_function();
         }
    break;

  case 26:
#line 262 "vtkParse.y"
    {
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
         }
    break;

  case 27:
#line 267 "vtkParse.y"
    {
         currentFunction->ReturnType = yyvsp[-2].integer;
         output_function();
         }
    break;

  case 28:
#line 272 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-2].integer;
         output_function();
         }
    break;

  case 29:
#line 278 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
         }
    break;

  case 30:
#line 284 "vtkParse.y"
    {
         preSig("virtual ");
         output_function();
         }
    break;

  case 31:
#line 290 "vtkParse.y"
    {
      currentFunction->IsOperator = 1;
      vtkParseDebug("Converted operator", 0);
    }
    break;

  case 32:
#line 295 "vtkParse.y"
    { postSig(")"); }
    break;

  case 33:
#line 295 "vtkParse.y"
    { postSig(";"); openSig = 0; }
    break;

  case 34:
#line 296 "vtkParse.y"
    {
      openSig = 1;
      currentFunction->Name = yyvsp[-3].str; 
      vtkParseDebug("Parsed func", yyvsp[-3].str);
    }
    break;

  case 35:
#line 302 "vtkParse.y"
    { 
      postSig(") = 0;"); 
      currentFunction->Name = yyvsp[-2].str; 
      vtkParseDebug("Parsed func", yyvsp[-2].str);
      currentFunction->IsPureVirtual = 1; 
      data.IsAbstract = 1;
    }
    break;

  case 37:
#line 310 "vtkParse.y"
    {postSig(" const");}
    break;

  case 38:
#line 312 "vtkParse.y"
    {postSig(" ("); }
    break;

  case 40:
#line 314 "vtkParse.y"
    {postSig("const ");}
    break;

  case 41:
#line 316 "vtkParse.y"
    {postSig("static ");}
    break;

  case 42:
#line 318 "vtkParse.y"
    {postSig(yyvsp[0].str);}
    break;

  case 43:
#line 318 "vtkParse.y"
    {postSig(yyvsp[0].str);}
    break;

  case 50:
#line 327 "vtkParse.y"
    { currentFunction->NumberOfArguments++;}
    break;

  case 51:
#line 328 "vtkParse.y"
    { currentFunction->NumberOfArguments++; postSig(", ");}
    break;

  case 53:
#line 331 "vtkParse.y"
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
        yyvsp[0].integer;}
    break;

  case 54:
#line 336 "vtkParse.y"
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 
        yyvsp[0].integer / 0x10000; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
        yyvsp[-1].integer + yyvsp[0].integer % 0x10000;
    }
    break;

  case 56:
#line 343 "vtkParse.y"
    { 
      postSig("void (*func)(void *) ");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 0x5000;
    }
    break;

  case 59:
#line 351 "vtkParse.y"
    {delSig();}
    break;

  case 60:
#line 351 "vtkParse.y"
    {delSig();}
    break;

  case 61:
#line 353 "vtkParse.y"
    { yyval.integer = yyvsp[0].integer; }
    break;

  case 62:
#line 361 "vtkParse.y"
    { yyval.integer = 0; }
    break;

  case 63:
#line 362 "vtkParse.y"
    { char temp[100]; sprintf(temp,"[%i]",yyvsp[0].integer); 
                   postSig(temp); }
    break;

  case 64:
#line 364 "vtkParse.y"
    { yyval.integer = 0x300 + 0x10000 * yyvsp[-2].integer + yyvsp[0].integer % 0x1000; }
    break;

  case 65:
#line 366 "vtkParse.y"
    { postSig("[]"); yyval.integer = 0x300 + yyvsp[0].integer % 0x1000; }
    break;

  case 66:
#line 368 "vtkParse.y"
    {yyval.integer = 0x1000 + yyvsp[0].integer;}
    break;

  case 67:
#line 369 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;}
    break;

  case 68:
#line 370 "vtkParse.y"
    {yyval.integer = 0x2000 + yyvsp[0].integer;}
    break;

  case 69:
#line 371 "vtkParse.y"
    {yyval.integer = 0x3000 + yyvsp[0].integer;}
    break;

  case 70:
#line 373 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;}
    break;

  case 71:
#line 375 "vtkParse.y"
    {yyval.integer = yyvsp[-1].integer + yyvsp[0].integer;}
    break;

  case 72:
#line 376 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;}
    break;

  case 73:
#line 378 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;}
    break;

  case 74:
#line 379 "vtkParse.y"
    { postSig("&"); yyval.integer = yyvsp[-1].integer;}
    break;

  case 75:
#line 380 "vtkParse.y"
    { postSig("*"); yyval.integer = 0x400 + yyvsp[-1].integer;}
    break;

  case 76:
#line 382 "vtkParse.y"
    { postSig("vtkStdString "); yyval.integer = 0x1303; }
    break;

  case 77:
#line 392 "vtkParse.y"
    { postSig("&"); yyval.integer = 0x100;}
    break;

  case 78:
#line 393 "vtkParse.y"
    { postSig("*"); yyval.integer = 0x300;}
    break;

  case 79:
#line 394 "vtkParse.y"
    { yyval.integer = 0x100 + yyvsp[0].integer;}
    break;

  case 80:
#line 395 "vtkParse.y"
    { yyval.integer = 0x400 + yyvsp[0].integer;}
    break;

  case 81:
#line 397 "vtkParse.y"
    {postSig("unsigned ");}
    break;

  case 82:
#line 398 "vtkParse.y"
    { yyval.integer = 0x10 + yyvsp[0].integer;}
    break;

  case 83:
#line 399 "vtkParse.y"
    { yyval.integer = yyvsp[0].integer;}
    break;

  case 84:
#line 402 "vtkParse.y"
    { postSig("float "); yyval.integer = 0x1;}
    break;

  case 85:
#line 403 "vtkParse.y"
    { postSig("void "); yyval.integer = 0x2;}
    break;

  case 86:
#line 404 "vtkParse.y"
    { postSig("char "); yyval.integer = 0x3;}
    break;

  case 87:
#line 405 "vtkParse.y"
    { postSig("int "); yyval.integer = 0x4;}
    break;

  case 88:
#line 406 "vtkParse.y"
    { postSig("short "); yyval.integer = 0x5;}
    break;

  case 89:
#line 407 "vtkParse.y"
    { postSig("long "); yyval.integer = 0x6;}
    break;

  case 90:
#line 408 "vtkParse.y"
    { postSig("double "); yyval.integer = 0x7;}
    break;

  case 91:
#line 409 "vtkParse.y"
    {       
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",yyvsp[0].str);
      postSig(ctmpid);
      yyval.integer = 0x8;}
    break;

  case 92:
#line 415 "vtkParse.y"
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
#line 431 "vtkParse.y"
    { postSig("vtkIdType "); yyval.integer = 0xA;}
    break;

  case 94:
#line 432 "vtkParse.y"
    { postSig("long long "); yyval.integer = 0xB;}
    break;

  case 95:
#line 433 "vtkParse.y"
    { postSig("__int64 "); yyval.integer = 0xC;}
    break;

  case 96:
#line 434 "vtkParse.y"
    { postSig("signed char "); yyval.integer = 0xD;}
    break;

  case 99:
#line 439 "vtkParse.y"
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    }
    break;

  case 100:
#line 444 "vtkParse.y"
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    }
    break;

  case 102:
#line 449 "vtkParse.y"
    {in_public = 1; in_protected = 0;}
    break;

  case 103:
#line 450 "vtkParse.y"
    {in_public = 0; in_protected = 0;}
    break;

  case 104:
#line 451 "vtkParse.y"
    {in_public = 0; in_protected = 1;}
    break;

  case 107:
#line 455 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;}
    break;

  case 108:
#line 456 "vtkParse.y"
    {yyval.integer = -1;}
    break;

  case 109:
#line 456 "vtkParse.y"
    {yyval.integer = -1;}
    break;

  case 110:
#line 460 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); }
    break;

  case 111:
#line 461 "vtkParse.y"
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

  case 112:
#line 471 "vtkParse.y"
    {postSig("Get");}
    break;

  case 113:
#line 471 "vtkParse.y"
    {postSig(" ();"); invertSig = 1;}
    break;

  case 114:
#line 473 "vtkParse.y"
    { 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = yyvsp[-1].integer;
   output_function();
   }
    break;

  case 115:
#line 480 "vtkParse.y"
    {preSig("void Set");}
    break;

  case 116:
#line 481 "vtkParse.y"
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

  case 117:
#line 491 "vtkParse.y"
    {preSig("char *Get");}
    break;

  case 118:
#line 492 "vtkParse.y"
    { 
   postSig(" ();");
   sprintf(temps,"Get%s",yyvsp[-1].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x303;
   output_function();
   }
    break;

  case 119:
#line 501 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); }
    break;

  case 120:
#line 502 "vtkParse.y"
    {postSig(");"); openSig = 0;}
    break;

  case 121:
#line 503 "vtkParse.y"
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

  case 122:
#line 533 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); }
    break;

  case 123:
#line 534 "vtkParse.y"
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

  case 124:
#line 545 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); }
    break;

  case 125:
#line 546 "vtkParse.y"
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

  case 126:
#line 556 "vtkParse.y"
    {postSig("*Get");}
    break;

  case 127:
#line 557 "vtkParse.y"
    {postSig(" ();"); invertSig = 1;}
    break;

  case 128:
#line 558 "vtkParse.y"
    { 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x309;
   output_function();
   }
    break;

  case 129:
#line 566 "vtkParse.y"
    {preSig("void "); postSig("On ();"); openSig = 0; }
    break;

  case 130:
#line 568 "vtkParse.y"
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

  case 131:
#line 583 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 132:
#line 588 "vtkParse.y"
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

  case 133:
#line 613 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 134:
#line 618 "vtkParse.y"
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

  case 135:
#line 630 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 136:
#line 635 "vtkParse.y"
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

  case 137:
#line 662 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 138:
#line 667 "vtkParse.y"
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

  case 139:
#line 679 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 140:
#line 684 "vtkParse.y"
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

  case 141:
#line 713 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 142:
#line 718 "vtkParse.y"
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

  case 143:
#line 730 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 144:
#line 735 "vtkParse.y"
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

  case 145:
#line 768 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 146:
#line 773 "vtkParse.y"
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

  case 147:
#line 785 "vtkParse.y"
    {
      free (currentFunction->Signature);
      currentFunction->Signature = NULL;
      }
    break;

  case 148:
#line 790 "vtkParse.y"
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

  case 149:
#line 803 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 150:
#line 808 "vtkParse.y"
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

  case 151:
#line 820 "vtkParse.y"
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

  case 152:
#line 867 "vtkParse.y"
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

  case 153:
#line 916 "vtkParse.y"
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
#line 2741 "vtkParse.tab.c"

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


#line 984 "vtkParse.y"

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
