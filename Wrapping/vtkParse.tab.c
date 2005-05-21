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
     StdString = 285,
     SetMacro = 286,
     GetMacro = 287,
     SetStringMacro = 288,
     GetStringMacro = 289,
     SetClampMacro = 290,
     SetObjectMacro = 291,
     SetReferenceCountedObjectMacro = 292,
     GetObjectMacro = 293,
     BooleanMacro = 294,
     SetVector2Macro = 295,
     SetVector3Macro = 296,
     SetVector4Macro = 297,
     SetVector6Macro = 298,
     GetVector2Macro = 299,
     GetVector3Macro = 300,
     GetVector4Macro = 301,
     GetVector6Macro = 302,
     SetVectorMacro = 303,
     GetVectorMacro = 304,
     ViewportCoordinateMacro = 305,
     WorldCoordinateMacro = 306,
     TypeMacro = 307
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
#define StdString 285
#define SetMacro 286
#define GetMacro 287
#define SetStringMacro 288
#define GetStringMacro 289
#define SetClampMacro 290
#define SetObjectMacro 291
#define SetReferenceCountedObjectMacro 292
#define GetObjectMacro 293
#define BooleanMacro 294
#define SetVector2Macro 295
#define SetVector3Macro 296
#define SetVector4Macro 297
#define SetVector6Macro 298
#define GetVector2Macro 299
#define GetVector3Macro 300
#define GetVector4Macro 301
#define GetVector6Macro 302
#define SetVectorMacro 303
#define GetVectorMacro 304
#define ViewportCoordinateMacro 305
#define WorldCoordinateMacro 306
#define TypeMacro 307




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
#line 314 "vtkParse.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 326 "vtkParse.tab.c"

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
#define YYFINAL  47
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   576

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  68
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  74
/* YYNRULES -- Number of rules. */
#define YYNRULES  180
/* YYNRULES -- Number of states. */
#define YYNSTATES  352

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   307

#define YYTRANSLATE(YYX)                   \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    64,     2,
      57,    58,    65,     2,    61,    66,    67,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    55,    56,
       2,    60,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    62,     2,    63,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    53,     2,    54,    59,     2,     2,     2,
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
      45,    46,    47,    48,    49,    50,    51,    52
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
     206,   208,   211,   213,   215,   218,   221,   223,   225,   227,
     230,   233,   234,   238,   240,   242,   244,   246,   248,   250,
     252,   254,   256,   258,   260,   261,   264,   267,   268,   274,
     276,   278,   280,   283,   285,   287,   291,   293,   294,   302,
     303,   304,   313,   314,   320,   321,   327,   328,   329,   340,
     341,   349,   350,   358,   359,   360,   369,   370,   378,   379,
     387,   388,   396,   397,   405,   406,   414,   415,   423,   424,
     432,   433,   441,   442,   450,   451,   461,   462,   472,   477,
     482,   489,   490,   493,   494,   497,   499,   501,   503,   505,
     507,   509,   511,   513,   515,   517,   519,   521,   523,   525,
     527,   529,   531,   533,   535,   537,   539,   541,   543,   547,
     551
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short yyrhs[] =
{
      69,     0,    -1,   135,    70,   135,    -1,    -1,     3,    24,
      71,   106,    53,    72,    54,    -1,    73,    -1,    73,    72,
      -1,   109,    55,    -1,    94,    -1,    76,    -1,    23,    76,
      -1,    75,    87,    -1,    23,    75,    87,    -1,    74,    87,
      -1,   112,    56,    -1,   112,    -1,    28,    57,    75,    58,
      -1,    59,    78,    -1,     7,    59,    78,    -1,    78,    -1,
      98,    78,    -1,    98,    20,    78,    -1,     7,    98,    20,
      78,    -1,     7,    98,    78,    -1,     7,    78,    -1,    77,
      -1,    98,    77,    -1,    98,    20,    77,    -1,     7,    98,
      20,    77,    -1,     7,    98,    77,    -1,     7,    77,    -1,
      21,   136,    56,    -1,    -1,    -1,    82,    79,    81,    80,
      -1,    82,    60,     9,    -1,    -1,    20,    -1,    -1,    86,
      57,    83,    88,    58,    -1,    20,    -1,    25,    -1,    24,
      -1,    10,    -1,    56,    -1,    53,   135,    54,    56,    -1,
      53,   135,    54,    -1,    55,   136,    56,    -1,    -1,    89,
      -1,    91,    -1,    -1,    91,    90,    61,    89,    -1,    98,
      -1,    -1,    98,    95,    92,    93,    -1,    26,    -1,    -1,
      60,   110,    -1,    98,    95,    56,    -1,    26,    56,    -1,
      86,    96,    -1,    -1,    -1,    27,    97,    96,    -1,    62,
     136,    63,    96,    -1,    84,    99,    -1,    99,    -1,    85,
      99,    -1,    85,    84,    99,    -1,   103,    -1,   103,   102,
      -1,   100,    -1,   101,    -1,   101,    64,    -1,   101,    65,
      -1,    30,    -1,    64,    -1,    65,    -1,    64,   102,    -1,
      65,   102,    -1,    -1,    22,   104,   105,    -1,   105,    -1,
      12,    -1,    16,    -1,    17,    -1,    11,    -1,    13,    -1,
      14,    -1,    15,    -1,    10,    -1,    24,    -1,    29,    -1,
      -1,    55,   107,    -1,   109,    24,    -1,    -1,   109,    24,
     108,    61,   107,    -1,     4,    -1,     5,    -1,     6,    -1,
      66,   111,    -1,   111,    -1,     9,    -1,     9,    67,     9,
      -1,    86,    -1,    -1,    31,    57,    86,    61,   113,   103,
      58,    -1,    -1,    -1,    32,    57,   114,    86,    61,   115,
     103,    58,    -1,    -1,    33,    57,   116,    86,    58,    -1,
      -1,    34,    57,   117,    86,    58,    -1,    -1,    -1,    35,
      57,    86,    61,   118,   103,   119,    61,   136,    58,    -1,
      -1,    36,    57,    86,    61,   120,   103,    58,    -1,    -1,
      37,    57,    86,    61,   121,   103,    58,    -1,    -1,    -1,
      38,    57,   122,    86,    61,   123,   103,    58,    -1,    -1,
      39,    57,    86,   124,    61,   103,    58,    -1,    -1,    40,
      57,    86,    61,   125,   103,    58,    -1,    -1,    44,    57,
      86,    61,   126,   103,    58,    -1,    -1,    41,    57,    86,
      61,   127,   103,    58,    -1,    -1,    45,    57,    86,    61,
     128,   103,    58,    -1,    -1,    42,    57,    86,    61,   129,
     103,    58,    -1,    -1,    46,    57,    86,    61,   130,   103,
      58,    -1,    -1,    43,    57,    86,    61,   131,   103,    58,
      -1,    -1,    47,    57,    86,    61,   132,   103,    58,    -1,
      -1,    48,    57,    86,    61,   133,   103,    61,   110,    58,
      -1,    -1,    49,    57,    86,    61,   134,   103,    61,   110,
      58,    -1,    50,    57,    86,    58,    -1,    51,    57,    86,
      58,    -1,    52,    57,    86,    61,    86,    58,    -1,    -1,
     137,   135,    -1,    -1,   138,   136,    -1,    56,    -1,   138,
      -1,    19,    -1,   139,    -1,   140,    -1,    65,    -1,    60,
      -1,    55,    -1,    61,    -1,    67,    -1,     8,    -1,   103,
      -1,   101,    -1,     9,    -1,    18,    -1,    64,    -1,   141,
      -1,    20,    -1,    21,    -1,    66,    -1,    59,    -1,    25,
      -1,    27,    -1,    53,   135,    54,    -1,    57,   135,    58,
      -1,    62,   135,    63,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   206,   206,   209,   208,   214,   214,   216,   216,   217,
     218,   219,   220,   221,   222,   223,   225,   227,   228,   229,
     230,   234,   238,   243,   248,   254,   258,   263,   268,   274,
     280,   286,   292,   292,   292,   298,   307,   307,   309,   309,
     311,   313,   315,   315,   317,   318,   319,   320,   322,   322,
     324,   325,   325,   327,   333,   332,   339,   346,   346,   348,
     348,   350,   358,   359,   359,   362,   365,   366,   367,   368,
     370,   371,   373,   375,   376,   377,   379,   389,   390,   391,
     392,   394,   394,   396,   399,   400,   401,   402,   403,   404,
     405,   406,   411,   428,   430,   430,   432,   438,   437,   443,
     444,   445,   447,   447,   449,   450,   450,   454,   453,   465,
     465,   465,   474,   474,   485,   485,   495,   496,   494,   527,
     526,   539,   538,   550,   551,   550,   560,   559,   577,   576,
     607,   606,   624,   623,   656,   655,   673,   672,   707,   706,
     724,   723,   762,   761,   779,   778,   797,   796,   813,   860,
     909,   965,   965,   966,   966,   968,   968,   970,   970,   970,
     970,   970,   970,   970,   970,   971,   971,   971,   971,   971,
     971,   971,   972,   972,   972,   972,   972,   972,   974,   975,
     976
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
  "ARRAY_NUM", "VTK_LEGACY", "IdType", "StdString", "SetMacro", "GetMacro",
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
  "var_id", "var_array", "@7", "type", "type_red1", "type_string1",
  "type_string2", "type_indirection", "type_red2", "@8", "type_primitive",
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
     305,   306,   307,   123,   125,    58,    59,    40,    41,   126,
      61,    44,    91,    93,    38,    42,    45,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    68,    69,    71,    70,    72,    72,    73,    73,    73,
      73,    73,    73,    73,    73,    73,    74,    75,    75,    75,
      75,    75,    75,    75,    75,    76,    76,    76,    76,    76,
      76,    77,    79,    80,    78,    78,    81,    81,    83,    82,
      84,    85,    86,    86,    87,    87,    87,    87,    88,    88,
      89,    90,    89,    91,    92,    91,    91,    93,    93,    94,
      94,    95,    96,    97,    96,    96,    98,    98,    98,    98,
      99,    99,    99,   100,   100,   100,   101,   102,   102,   102,
     102,   104,   103,   103,   105,   105,   105,   105,   105,   105,
     105,   105,   105,   105,   106,   106,   107,   108,   107,   109,
     109,   109,   110,   110,   111,   111,   111,   113,   112,   114,
     115,   112,   116,   112,   117,   112,   118,   119,   112,   120,
     112,   121,   112,   122,   123,   112,   124,   112,   125,   112,
     126,   112,   127,   112,   128,   112,   129,   112,   130,   112,
     131,   112,   132,   112,   133,   112,   134,   112,   112,   112,
     112,   135,   135,   136,   136,   137,   137,   138,   138,   138,
     138,   138,   138,   138,   138,   138,   138,   138,   138,   138,
     138,   138,   138,   138,   138,   138,   138,   138,   139,   140,
     141
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
       1,     1,     1,     1,     0,     2,     2,     0,     5,     1,
       1,     1,     2,     1,     1,     3,     1,     0,     7,     0,
       0,     8,     0,     5,     0,     5,     0,     0,    10,     0,
       7,     0,     7,     0,     0,     8,     0,     7,     0,     7,
       0,     7,     0,     7,     0,     7,     0,     7,     0,     7,
       0,     7,     0,     7,     0,     9,     0,     9,     4,     4,
       6,     0,     2,     0,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     3,
       3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
     151,   165,   168,    91,    87,    84,    88,    89,    90,    85,
      86,   169,   157,   172,   173,    81,    92,   176,   177,    93,
      76,   151,   162,   155,   151,   175,   161,   163,   151,   170,
     160,   174,   164,     0,   167,   166,    83,     0,   151,   156,
     158,   159,   171,     0,     0,     0,     0,     1,     0,   151,
     152,    82,   178,   179,   180,     3,     2,    94,     0,     0,
      99,   100,   101,    95,     0,     0,    96,     0,    91,    40,
     153,     0,    92,    41,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       5,     0,     0,     9,    25,    19,    32,     0,     0,     0,
       8,     0,    67,    72,    73,    70,     0,    15,     0,     0,
      30,    24,     0,     0,   153,     0,    10,     0,    60,     0,
       0,   109,   112,   114,     0,     0,     0,   123,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    43,    42,    17,     4,     6,   151,   153,    44,
      13,    11,     0,    36,    66,     0,    68,    38,     0,    26,
      20,    62,     0,    74,    75,    77,    78,    71,     7,    14,
       0,    18,     0,    29,    23,    31,   154,    12,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   126,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    35,    37,    33,    69,    48,
      27,    21,    63,   153,    61,    59,    79,    80,    98,    28,
      22,     0,    16,     0,   107,     0,     0,     0,   116,   119,
     121,     0,     0,   128,   132,   136,   140,   130,   134,   138,
     142,   144,   146,   148,   149,     0,    46,    47,    34,    56,
       0,    49,    50,    53,    62,     0,     0,     0,   110,   113,
     115,     0,     0,     0,   124,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    45,    39,     0,
      62,    54,    64,    62,     0,     0,   117,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   150,     0,    57,    65,   108,     0,     0,   120,   122,
       0,   127,   129,   133,   137,   141,   131,   135,   139,   143,
       0,     0,    52,     0,    55,   111,   153,   125,   104,     0,
     106,     0,   103,     0,    58,     0,     0,   102,   145,   147,
     118,   105
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,    33,    49,    57,    99,   100,   101,   102,   103,   104,
     105,   163,   258,   217,   106,   219,   107,   108,   109,   160,
     260,   261,   289,   262,   313,   334,   110,   172,   224,   264,
     111,   112,   113,    34,   177,    35,    43,    36,    59,    63,
     118,    64,   341,   342,   117,   267,   192,   295,   193,   194,
     271,   317,   272,   273,   198,   299,   242,   276,   280,   277,
     281,   278,   282,   279,   283,   284,   285,    37,   123,    38,
      39,    40,    41,    42
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -275
static const short yypact[] =
{
     259,  -275,  -275,  -275,  -275,  -275,  -275,  -275,  -275,  -275,
    -275,  -275,  -275,  -275,  -275,  -275,  -275,  -275,  -275,  -275,
    -275,   259,  -275,  -275,   259,  -275,  -275,  -275,   259,  -275,
    -275,  -275,  -275,    28,  -275,  -275,  -275,    46,   259,  -275,
    -275,  -275,  -275,   351,   -13,     4,     1,  -275,    32,   259,
    -275,  -275,  -275,  -275,  -275,  -275,  -275,    23,   114,    34,
    -275,  -275,  -275,  -275,    66,   394,    35,   496,    43,  -275,
     330,   444,    49,  -275,    56,    57,    65,    73,    78,    95,
      97,    99,   102,   103,   106,   107,   109,   118,   121,   123,
     126,   128,   131,   132,   136,   137,   138,   140,    16,   144,
     394,   -26,   -26,  -275,  -275,  -275,    63,   280,   157,   142,
    -275,    53,  -275,  -275,   -30,    38,   145,   146,   143,    16,
    -275,  -275,    87,   149,   330,   -26,  -275,    53,  -275,   465,
      16,  -275,  -275,  -275,    16,    16,    16,  -275,    16,    16,
      16,    16,    16,    16,    16,    16,    16,    16,    16,    16,
      16,    16,  -275,  -275,  -275,  -275,  -275,   259,   330,  -275,
    -275,  -275,   192,   186,  -275,   280,  -275,  -275,    48,  -275,
    -275,   -25,   154,  -275,  -275,    38,    38,  -275,  -275,  -275,
     114,  -275,    48,  -275,  -275,  -275,  -275,  -275,   517,   164,
     105,   163,    16,    16,    16,   165,   167,   168,    16,  -275,
     169,   171,   172,   173,   175,   176,   177,   178,   188,   194,
     198,   199,   197,   208,   207,  -275,  -275,  -275,  -275,    69,
    -275,  -275,  -275,   330,  -275,  -275,  -275,  -275,  -275,  -275,
    -275,   141,  -275,    16,  -275,   203,   224,   227,  -275,  -275,
    -275,   204,   226,  -275,  -275,  -275,  -275,  -275,  -275,  -275,
    -275,  -275,  -275,  -275,  -275,    16,   242,  -275,  -275,  -275,
     241,  -275,   239,    16,   -12,   238,    16,   230,  -275,  -275,
    -275,   230,   230,   230,  -275,   230,   230,   230,   230,   230,
     230,   230,   230,   230,   230,   230,   245,  -275,  -275,   244,
     -12,  -275,  -275,   -12,   248,   230,  -275,   249,   250,   230,
     253,   255,   264,   269,   272,   274,   275,   276,   277,   256,
     292,  -275,    69,   296,  -275,  -275,   278,   297,  -275,  -275,
     311,  -275,  -275,  -275,  -275,  -275,  -275,  -275,  -275,  -275,
       9,     9,  -275,     9,  -275,  -275,   330,  -275,   303,    14,
    -275,   313,  -275,   314,  -275,   315,   365,  -275,  -275,  -275,
    -275,  -275
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
    -275,  -275,  -275,  -275,   279,  -275,  -275,   -63,   305,   -51,
      -6,  -275,  -275,  -275,  -275,  -275,   270,  -275,    -2,   -77,
    -275,    70,  -275,  -275,  -275,  -275,  -275,   125,  -242,  -275,
     -62,   -95,  -275,   -61,  -123,   -64,  -275,   334,  -275,   201,
    -275,   -45,  -274,    45,  -275,  -275,  -275,  -275,  -275,  -275,
    -275,  -275,  -275,  -275,  -275,  -275,  -275,  -275,  -275,  -275,
    -275,  -275,  -275,  -275,  -275,  -275,  -275,    -7,  -113,  -275,
     -70,  -275,  -275,  -275
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -98
static const short yytable[] =
{
     124,   115,   222,   115,   114,   122,   114,   115,   125,   127,
     114,   186,   164,   166,    44,   222,   120,    45,   338,   152,
     116,    46,   292,   338,   152,   161,   152,   157,    47,   158,
     159,    50,   167,   153,   173,   174,   115,   223,   153,   114,
     153,    52,    56,   115,   115,   214,   114,   114,   187,    48,
     223,   314,   226,   227,   124,   116,    55,   343,   152,   344,
     169,   121,    53,   152,    54,   115,   189,   190,   114,    70,
     218,   183,   153,   168,    70,   339,   169,   153,    58,     3,
       4,     5,     6,     7,     8,     9,    10,    65,   124,    69,
      66,    15,   154,    16,    73,   259,   -97,   152,    19,    20,
     -43,   115,   175,   176,   114,   170,   -42,   182,    70,   171,
     265,   153,   128,   181,   129,   152,   184,   220,    60,    61,
      62,   170,   130,   162,   115,   233,   231,   114,   191,   153,
     131,   229,   195,   196,   197,   132,   199,   200,   201,   202,
     203,   204,   205,   206,   207,   208,   209,   210,   211,   212,
     213,   152,   133,   124,   134,   115,   135,   263,   114,   136,
     137,   266,   221,   138,   139,   153,   140,     3,     4,     5,
       6,     7,     8,     9,    10,   141,   230,    69,   142,    15,
     143,    16,   121,   144,   170,   145,    19,    20,   146,   147,
     235,   236,   237,   148,   149,   150,   241,   151,   155,   167,
     178,   215,   179,   294,   180,   185,   216,   296,   297,   298,
     225,   300,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,   232,   345,   234,   184,   238,   221,   239,   240,
     243,   316,   244,   245,   246,   320,   247,   248,   249,   250,
       3,     4,     5,     6,     7,     8,     9,    10,   115,   251,
     263,   114,    15,   286,    16,   252,   253,   254,   255,    19,
     230,   290,   256,   257,   268,   274,   124,     1,     2,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,   269,    16,    17,   270,    18,   275,    19,    20,
       3,     4,     5,     6,     7,     8,     9,    10,   287,   288,
     -51,   293,    15,   311,    16,   312,   315,   318,   319,    19,
      20,   321,    21,   322,    22,    23,    24,   330,    25,    26,
      27,    28,   323,    29,    30,    31,    32,   324,   340,   340,
     325,   340,   326,   327,   328,   329,   335,   340,     1,     2,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,   331,    16,    17,   333,    18,   336,    19,
      20,     3,     4,     5,     6,     7,     8,     9,    10,   337,
     346,   348,   349,   350,   351,    16,   126,    51,   165,   156,
      19,   228,   332,    21,   347,    22,     0,    24,   291,    25,
      26,    27,    28,     0,    29,    30,    31,    32,    60,    61,
      62,    67,     0,     0,    68,     4,     5,     6,     7,     8,
       9,    10,     0,     0,    69,    70,    15,    71,    72,    73,
      74,     0,    75,    19,    20,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,     0,     0,     0,
       0,    67,     0,    98,    68,     4,     5,     6,     7,     8,
       9,    10,     0,     0,    69,    70,    15,     0,    72,    73,
       0,     0,   188,    19,    20,    68,     4,     5,     6,     7,
       8,     9,    10,     0,     0,    69,     0,    15,     0,    72,
      73,     0,     0,     0,    19,    20,     0,     0,     0,     0,
       0,     0,     0,    98,     0,     0,    68,     4,     5,     6,
       7,     8,     9,    10,     0,     0,    69,    70,    15,     0,
      72,    73,     0,     0,    98,    19,    20,    68,     4,     5,
       6,     7,     8,     9,    10,     0,     0,    69,     0,    15,
       0,    72,    73,     0,     0,     0,    19,    20,     0,     0,
       0,     0,     0,     0,     0,   119,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   119
};

static const short yycheck[] =
{
      70,    65,    27,    67,    65,    67,    67,    71,    71,    71,
      71,   124,   107,   108,    21,    27,    67,    24,     9,    10,
      65,    28,   264,     9,    10,   102,    10,    53,     0,    55,
      56,    38,    57,    24,    64,    65,   100,    62,    24,   100,
      24,    54,    49,   107,   108,   158,   107,   108,   125,     3,
      62,   293,   175,   176,   124,   100,    24,   331,    10,   333,
     111,    67,    58,    10,    63,   129,   129,   129,   129,    21,
     165,   122,    24,    20,    21,    66,   127,    24,    55,    10,
      11,    12,    13,    14,    15,    16,    17,    53,   158,    20,
      24,    22,    98,    24,    25,    26,    61,    10,    29,    30,
      57,   165,    64,    65,   165,   111,    57,    20,    21,   111,
     223,    24,    56,   119,    57,    10,   122,   168,     4,     5,
       6,   127,    57,    60,   188,    20,   188,   188,   130,    24,
      57,   182,   134,   135,   136,    57,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   147,   148,   149,   150,   151,
     157,    10,    57,   223,    57,   219,    57,   219,   219,    57,
      57,    20,   168,    57,    57,    24,    57,    10,    11,    12,
      13,    14,    15,    16,    17,    57,   182,    20,    57,    22,
      57,    24,   188,    57,   190,    57,    29,    30,    57,    57,
     192,   193,   194,    57,    57,    57,   198,    57,    54,    57,
      55,     9,    56,   267,    61,    56,    20,   271,   272,   273,
      56,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,    58,   336,    61,   231,    61,   233,    61,    61,
      61,   295,    61,    61,    61,   299,    61,    61,    61,    61,
      10,    11,    12,    13,    14,    15,    16,    17,   312,    61,
     312,   312,    22,   255,    24,    61,    58,    58,    61,    29,
     266,   263,    54,    56,    61,    61,   336,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    58,    24,    25,    58,    27,    61,    29,    30,
      10,    11,    12,    13,    14,    15,    16,    17,    56,    58,
      61,    63,    22,    58,    24,    61,    58,    58,    58,    29,
      30,    58,    53,    58,    55,    56,    57,    61,    59,    60,
      61,    62,    58,    64,    65,    66,    67,    58,   330,   331,
      58,   333,    58,    58,    58,    58,    58,   339,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    61,    24,    25,    60,    27,    61,    29,
      30,    10,    11,    12,    13,    14,    15,    16,    17,    58,
      67,    58,    58,    58,     9,    24,    71,    43,   108,   100,
      29,   180,   312,    53,   339,    55,    -1,    57,   263,    59,
      60,    61,    62,    -1,    64,    65,    66,    67,     4,     5,
       6,     7,    -1,    -1,    10,    11,    12,    13,    14,    15,
      16,    17,    -1,    -1,    20,    21,    22,    23,    24,    25,
      26,    -1,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    -1,    -1,    -1,
      -1,     7,    -1,    59,    10,    11,    12,    13,    14,    15,
      16,    17,    -1,    -1,    20,    21,    22,    -1,    24,    25,
      -1,    -1,     7,    29,    30,    10,    11,    12,    13,    14,
      15,    16,    17,    -1,    -1,    20,    -1,    22,    -1,    24,
      25,    -1,    -1,    -1,    29,    30,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    59,    -1,    -1,    10,    11,    12,    13,
      14,    15,    16,    17,    -1,    -1,    20,    21,    22,    -1,
      24,    25,    -1,    -1,    59,    29,    30,    10,    11,    12,
      13,    14,    15,    16,    17,    -1,    -1,    20,    -1,    22,
      -1,    24,    25,    -1,    -1,    -1,    29,    30,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    59,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    59
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    24,    25,    27,    29,
      30,    53,    55,    56,    57,    59,    60,    61,    62,    64,
      65,    66,    67,    69,   101,   103,   105,   135,   137,   138,
     139,   140,   141,   104,   135,   135,   135,     0,     3,    70,
     135,   105,    54,    58,    63,    24,   135,    71,    55,   106,
       4,     5,     6,   107,   109,    53,    24,     7,    10,    20,
      21,    23,    24,    25,    26,    28,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    59,    72,
      73,    74,    75,    76,    77,    78,    82,    84,    85,    86,
      94,    98,    99,   100,   101,   103,   109,   112,   108,    59,
      77,    78,    98,   136,   138,    75,    76,    98,    56,    57,
      57,    57,    57,    57,    57,    57,    57,    57,    57,    57,
      57,    57,    57,    57,    57,    57,    57,    57,    57,    57,
      57,    57,    10,    24,    78,    54,    72,    53,    55,    56,
      87,    87,    60,    79,    99,    84,    99,    57,    20,    77,
      78,    86,    95,    64,    65,    64,    65,   102,    55,    56,
      61,    78,    20,    77,    78,    56,   136,    87,     7,    75,
      98,    86,   114,   116,   117,    86,    86,    86,   122,    86,
      86,    86,    86,    86,    86,    86,    86,    86,    86,    86,
      86,    86,    86,   135,   136,     9,    20,    81,    99,    83,
      77,    78,    27,    62,    96,    56,   102,   102,   107,    77,
      78,    98,    58,    20,    61,    86,    86,    86,    61,    61,
      61,    86,   124,    61,    61,    61,    61,    61,    61,    61,
      61,    61,    61,    58,    58,    61,    54,    56,    80,    26,
      88,    89,    91,    98,    97,   136,    20,   113,    61,    58,
      58,   118,   120,   121,    61,    61,   125,   127,   129,   131,
     126,   128,   130,   132,   133,   134,    86,    56,    58,    90,
      86,    95,    96,    63,   103,   115,   103,   103,   103,   123,
     103,   103,   103,   103,   103,   103,   103,   103,   103,   103,
     103,    58,    61,    92,    96,    58,   103,   119,    58,    58,
     103,    58,    58,    58,    58,    58,    58,    58,    58,    58,
      61,    61,    89,    60,    93,    58,    61,    58,     9,    66,
      86,   110,   111,   110,   110,   136,    67,   111,    58,    58,
      58,     9
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
#line 209 "vtkParse.y"
    {
      data.ClassName = vtkstrdup(yyvsp[0].str);
      ;}
    break;

  case 11:
#line 219 "vtkParse.y"
    { output_function(); ;}
    break;

  case 12:
#line 220 "vtkParse.y"
    { output_function(); ;}
    break;

  case 13:
#line 221 "vtkParse.y"
    { legacySig(); output_function(); ;}
    break;

  case 17:
#line 227 "vtkParse.y"
    { preSig("~"); ;}
    break;

  case 18:
#line 228 "vtkParse.y"
    { preSig("virtual ~"); ;}
    break;

  case 20:
#line 231 "vtkParse.y"
    {
         currentFunction->ReturnType = yyvsp[-1].integer;
    ;}
    break;

  case 21:
#line 235 "vtkParse.y"
    {
         currentFunction->ReturnType = yyvsp[-2].integer;
    ;}
    break;

  case 22:
#line 239 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-2].integer;
    ;}
    break;

  case 23:
#line 244 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-1].integer;
    ;}
    break;

  case 24:
#line 249 "vtkParse.y"
    {
         preSig("virtual ");
    ;}
    break;

  case 25:
#line 255 "vtkParse.y"
    {
         output_function();
    ;}
    break;

  case 26:
#line 259 "vtkParse.y"
    {
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
    ;}
    break;

  case 27:
#line 264 "vtkParse.y"
    {
         currentFunction->ReturnType = yyvsp[-2].integer;
         output_function();
    ;}
    break;

  case 28:
#line 269 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-2].integer;
         output_function();
    ;}
    break;

  case 29:
#line 275 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
    ;}
    break;

  case 30:
#line 281 "vtkParse.y"
    {
         preSig("virtual ");
         output_function();
    ;}
    break;

  case 31:
#line 287 "vtkParse.y"
    {
      currentFunction->IsOperator = 1;
      vtkParseDebug("Converted operator", 0);
    ;}
    break;

  case 32:
#line 292 "vtkParse.y"
    { postSig(")"); ;}
    break;

  case 33:
#line 292 "vtkParse.y"
    { postSig(";"); openSig = 0; ;}
    break;

  case 34:
#line 293 "vtkParse.y"
    {
      openSig = 1;
      currentFunction->Name = yyvsp[-3].str; 
      vtkParseDebug("Parsed func", yyvsp[-3].str);
    ;}
    break;

  case 35:
#line 299 "vtkParse.y"
    { 
      postSig(") = 0;"); 
      currentFunction->Name = yyvsp[-2].str; 
      vtkParseDebug("Parsed func", yyvsp[-2].str);
      currentFunction->IsPureVirtual = 1; 
      data.IsAbstract = 1;
    ;}
    break;

  case 37:
#line 307 "vtkParse.y"
    {postSig(" const");;}
    break;

  case 38:
#line 309 "vtkParse.y"
    {postSig(" ("); ;}
    break;

  case 40:
#line 311 "vtkParse.y"
    {postSig("const ");;}
    break;

  case 41:
#line 313 "vtkParse.y"
    {postSig("static ");;}
    break;

  case 42:
#line 315 "vtkParse.y"
    {postSig(yyvsp[0].str);;}
    break;

  case 43:
#line 315 "vtkParse.y"
    {postSig(yyvsp[0].str);;}
    break;

  case 50:
#line 324 "vtkParse.y"
    { currentFunction->NumberOfArguments++;;}
    break;

  case 51:
#line 325 "vtkParse.y"
    { currentFunction->NumberOfArguments++; postSig(", ");;}
    break;

  case 53:
#line 328 "vtkParse.y"
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
   yyvsp[0].integer;;}
    break;

  case 54:
#line 333 "vtkParse.y"
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 
   yyvsp[0].integer / 0x10000; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
   yyvsp[-1].integer + yyvsp[0].integer % 0x10000;
    ;}
    break;

  case 56:
#line 340 "vtkParse.y"
    { 
      postSig("void (*func)(void *) ");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 0x5000;
    ;}
    break;

  case 59:
#line 348 "vtkParse.y"
    {delSig();;}
    break;

  case 60:
#line 348 "vtkParse.y"
    {delSig();;}
    break;

  case 61:
#line 350 "vtkParse.y"
    { yyval.integer = yyvsp[0].integer; ;}
    break;

  case 62:
#line 358 "vtkParse.y"
    { yyval.integer = 0; ;}
    break;

  case 63:
#line 359 "vtkParse.y"
    { char temp[100]; sprintf(temp,"[%i]",yyvsp[0].integer); 
                   postSig(temp); ;}
    break;

  case 64:
#line 361 "vtkParse.y"
    { yyval.integer = 0x300 + 0x10000 * yyvsp[-2].integer + yyvsp[0].integer % 0x1000; ;}
    break;

  case 65:
#line 363 "vtkParse.y"
    { postSig("[]"); yyval.integer = 0x300 + yyvsp[0].integer % 0x1000; ;}
    break;

  case 66:
#line 365 "vtkParse.y"
    {yyval.integer = 0x1000 + yyvsp[0].integer;;}
    break;

  case 67:
#line 366 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;;}
    break;

  case 68:
#line 367 "vtkParse.y"
    {yyval.integer = 0x2000 + yyvsp[0].integer;;}
    break;

  case 69:
#line 368 "vtkParse.y"
    {yyval.integer = 0x3000 + yyvsp[0].integer;;}
    break;

  case 70:
#line 370 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;;}
    break;

  case 71:
#line 372 "vtkParse.y"
    {yyval.integer = yyvsp[-1].integer + yyvsp[0].integer;;}
    break;

  case 72:
#line 373 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;;}
    break;

  case 73:
#line 375 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;;}
    break;

  case 74:
#line 376 "vtkParse.y"
    { postSig("&"); yyval.integer = yyvsp[-1].integer;;}
    break;

  case 75:
#line 377 "vtkParse.y"
    { postSig("*"); yyval.integer = 0x400 + yyvsp[-1].integer;;}
    break;

  case 76:
#line 379 "vtkParse.y"
    { postSig("vtkStdString "); yyval.integer = 0x1303; ;}
    break;

  case 77:
#line 389 "vtkParse.y"
    { postSig("&"); yyval.integer = 0x100;;}
    break;

  case 78:
#line 390 "vtkParse.y"
    { postSig("*"); yyval.integer = 0x300;;}
    break;

  case 79:
#line 391 "vtkParse.y"
    { yyval.integer = 0x100 + yyvsp[0].integer;;}
    break;

  case 80:
#line 392 "vtkParse.y"
    { yyval.integer = 0x400 + yyvsp[0].integer;;}
    break;

  case 81:
#line 394 "vtkParse.y"
    {postSig("unsigned ");;}
    break;

  case 82:
#line 395 "vtkParse.y"
    { yyval.integer = 0x10 + yyvsp[0].integer;;}
    break;

  case 83:
#line 396 "vtkParse.y"
    { yyval.integer = yyvsp[0].integer;;}
    break;

  case 84:
#line 399 "vtkParse.y"
    { postSig("float "); yyval.integer = 0x1;;}
    break;

  case 85:
#line 400 "vtkParse.y"
    { postSig("void "); yyval.integer = 0x2;;}
    break;

  case 86:
#line 401 "vtkParse.y"
    { postSig("char "); yyval.integer = 0x3;;}
    break;

  case 87:
#line 402 "vtkParse.y"
    { postSig("int "); yyval.integer = 0x4;;}
    break;

  case 88:
#line 403 "vtkParse.y"
    { postSig("short "); yyval.integer = 0x5;;}
    break;

  case 89:
#line 404 "vtkParse.y"
    { postSig("long "); yyval.integer = 0x6;;}
    break;

  case 90:
#line 405 "vtkParse.y"
    { postSig("double "); yyval.integer = 0x7;;}
    break;

  case 91:
#line 406 "vtkParse.y"
    {       
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",yyvsp[0].str);
      postSig(ctmpid);
      yyval.integer = 0x8;;}
    break;

  case 92:
#line 412 "vtkParse.y"
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

  case 93:
#line 428 "vtkParse.y"
    { postSig("vtkIdType "); yyval.integer = 0xA;;}
    break;

  case 96:
#line 433 "vtkParse.y"
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    ;}
    break;

  case 97:
#line 438 "vtkParse.y"
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    ;}
    break;

  case 99:
#line 443 "vtkParse.y"
    {in_public = 1; in_protected = 0;;}
    break;

  case 100:
#line 444 "vtkParse.y"
    {in_public = 0; in_protected = 0;;}
    break;

  case 101:
#line 445 "vtkParse.y"
    {in_public = 0; in_protected = 1;;}
    break;

  case 104:
#line 449 "vtkParse.y"
    {yyval.integer = yyvsp[0].integer;;}
    break;

  case 105:
#line 450 "vtkParse.y"
    {yyval.integer = -1;;}
    break;

  case 106:
#line 450 "vtkParse.y"
    {yyval.integer = -1;;}
    break;

  case 107:
#line 454 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); ;}
    break;

  case 108:
#line 455 "vtkParse.y"
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

  case 109:
#line 465 "vtkParse.y"
    {postSig("Get");;}
    break;

  case 110:
#line 465 "vtkParse.y"
    {postSig(" ();"); invertSig = 1;;}
    break;

  case 111:
#line 467 "vtkParse.y"
    { 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = yyvsp[-1].integer;
   output_function();
   ;}
    break;

  case 112:
#line 474 "vtkParse.y"
    {preSig("void Set");;}
    break;

  case 113:
#line 475 "vtkParse.y"
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

  case 114:
#line 485 "vtkParse.y"
    {preSig("char *Get");;}
    break;

  case 115:
#line 486 "vtkParse.y"
    { 
   postSig(" ();");
   sprintf(temps,"Get%s",yyvsp[-1].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x303;
   output_function();
   ;}
    break;

  case 116:
#line 495 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); ;}
    break;

  case 117:
#line 496 "vtkParse.y"
    {postSig(");"); openSig = 0;;}
    break;

  case 118:
#line 497 "vtkParse.y"
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

  case 119:
#line 527 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); ;}
    break;

  case 120:
#line 528 "vtkParse.y"
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

  case 121:
#line 539 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); ;}
    break;

  case 122:
#line 540 "vtkParse.y"
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

  case 123:
#line 550 "vtkParse.y"
    {postSig("*Get");;}
    break;

  case 124:
#line 551 "vtkParse.y"
    {postSig(" ();"); invertSig = 1;;}
    break;

  case 125:
#line 552 "vtkParse.y"
    { 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x309;
   output_function();
   ;}
    break;

  case 126:
#line 560 "vtkParse.y"
    {preSig("void "); postSig("On ();"); openSig = 0; ;}
    break;

  case 127:
#line 562 "vtkParse.y"
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

  case 128:
#line 577 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 129:
#line 582 "vtkParse.y"
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

  case 130:
#line 607 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 131:
#line 612 "vtkParse.y"
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

  case 132:
#line 624 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 133:
#line 629 "vtkParse.y"
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

  case 134:
#line 656 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 135:
#line 661 "vtkParse.y"
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

  case 136:
#line 673 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 137:
#line 678 "vtkParse.y"
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

  case 138:
#line 707 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 139:
#line 712 "vtkParse.y"
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

  case 140:
#line 724 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 141:
#line 729 "vtkParse.y"
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

  case 142:
#line 762 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 143:
#line 767 "vtkParse.y"
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

  case 144:
#line 779 "vtkParse.y"
    {
      free (currentFunction->Signature);
      currentFunction->Signature = NULL;
      ;}
    break;

  case 145:
#line 784 "vtkParse.y"
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

  case 146:
#line 797 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 147:
#line 802 "vtkParse.y"
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

  case 148:
#line 814 "vtkParse.y"
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

  case 149:
#line 861 "vtkParse.y"
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

  case 150:
#line 910 "vtkParse.y"
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
#line 2705 "vtkParse.tab.c"

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


#line 978 "vtkParse.y"

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




