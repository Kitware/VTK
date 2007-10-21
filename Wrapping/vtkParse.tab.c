/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

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
     BOOL = 276,
     CLASS_REF = 277,
     OTHER = 278,
     CONST = 279,
     OPERATOR = 280,
     UNSIGNED = 281,
     FRIEND = 282,
     VTK_ID = 283,
     STATIC = 284,
     VAR_FUNCTION = 285,
     ARRAY_NUM = 286,
     VTK_LEGACY = 287,
     TypeInt8 = 288,
     TypeUInt8 = 289,
     TypeInt16 = 290,
     TypeUInt16 = 291,
     TypeInt32 = 292,
     TypeUInt32 = 293,
     TypeInt64 = 294,
     TypeUInt64 = 295,
     TypeFloat32 = 296,
     TypeFloat64 = 297,
     IdType = 298,
     StdString = 299,
     SetMacro = 300,
     GetMacro = 301,
     SetStringMacro = 302,
     GetStringMacro = 303,
     SetClampMacro = 304,
     SetObjectMacro = 305,
     SetReferenceCountedObjectMacro = 306,
     GetObjectMacro = 307,
     BooleanMacro = 308,
     SetVector2Macro = 309,
     SetVector3Macro = 310,
     SetVector4Macro = 311,
     SetVector6Macro = 312,
     GetVector2Macro = 313,
     GetVector3Macro = 314,
     GetVector4Macro = 315,
     GetVector6Macro = 316,
     SetVectorMacro = 317,
     GetVectorMacro = 318,
     ViewportCoordinateMacro = 319,
     WorldCoordinateMacro = 320,
     TypeMacro = 321
   };
#endif
/* Tokens.  */
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
#define BOOL 276
#define CLASS_REF 277
#define OTHER 278
#define CONST 279
#define OPERATOR 280
#define UNSIGNED 281
#define FRIEND 282
#define VTK_ID 283
#define STATIC 284
#define VAR_FUNCTION 285
#define ARRAY_NUM 286
#define VTK_LEGACY 287
#define TypeInt8 288
#define TypeUInt8 289
#define TypeInt16 290
#define TypeUInt16 291
#define TypeInt32 292
#define TypeUInt32 293
#define TypeInt64 294
#define TypeUInt64 295
#define TypeFloat32 296
#define TypeFloat64 297
#define IdType 298
#define StdString 299
#define SetMacro 300
#define GetMacro 301
#define SetStringMacro 302
#define GetStringMacro 303
#define SetClampMacro 304
#define SetObjectMacro 305
#define SetReferenceCountedObjectMacro 306
#define GetObjectMacro 307
#define BooleanMacro 308
#define SetVector2Macro 309
#define SetVector3Macro 310
#define SetVector4Macro 311
#define SetVector6Macro 312
#define GetVector2Macro 313
#define GetVector3Macro 314
#define GetVector4Macro 315
#define GetVector6Macro 316
#define SetVectorMacro 317
#define GetVectorMacro 318
#define ViewportCoordinateMacro 319
#define WorldCoordinateMacro 320
#define TypeMacro 321




/* Copy the first part of user declarations.  */
#line 15 "vtkParse.y"


/*

This file must be translated to C and modified to build everywhere.

Run yacc like this:

  yacc -b vtkParse vtkParse.y

Modify vtkParse.tab.c:
  - remove TABs
  - remove yyerrorlab stuff in range ["goto yyerrlab1;", "yyerrstatus = 3;")

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define yyerror(a) fprintf(stderr,"%s\n",a)
#define yywrap() 1

/* Make sure yacc-generated code knows we have included stdlib.h.  */
#ifndef _STDLIB_H
# define _STDLIB_H
#endif
#define YYINCLUDED_STDLIB_H

/* Map from the type enumeration in vtkType.h to the VTK wrapping type
   system number for the type.  Note that the wrapping type system
   does not enumerate its type values by name.  Look in the
   type_primitive production rule in the grammar for the "official"
   enumeration. */
static int vtkParseTypeMap[] =
  {
   0x2,  /* VTK_VOID                0 */
   0,    /* VTK_BIT                 1 */
   0x3,  /* VTK_CHAR                2 */
   0x13, /* VTK_UNSIGNED_CHAR       3 */
   0x5,  /* VTK_SHORT               4 */
   0x15, /* VTK_UNSIGNED_SHORT      5 */
   0x4,  /* VTK_INT                 6 */
   0x14, /* VTK_UNSIGNED_INT        7 */
   0x6,  /* VTK_LONG                8 */
   0x16, /* VTK_UNSIGNED_LONG       9 */
   0x1,  /* VTK_FLOAT              10 */
   0x7,  /* VTK_DOUBLE             11 */
   0xA,  /* VTK_ID_TYPE            12 */
   0,    /* VTK_STRING             13 */
   0,    /* VTK_OPAQUE             14 */
   0xD,  /* VTK_SIGNED_CHAR        15 */
   0xB,  /* VTK_LONG_LONG          16 */
   0x1B, /* VTK_UNSIGNED_LONG_LONG 17 */
   0xC,  /* VTK___INT64            18 */
   0x1C  /* VTK_UNSIGNED___INT64   19 */
  };

/* Define some constants to simplify references to the table lookup in
   the type_primitive production rule code.  */
#include "vtkType.h"
#define VTK_PARSE_INT8 vtkParseTypeMap[VTK_TYPE_INT8]
#define VTK_PARSE_UINT8 vtkParseTypeMap[VTK_TYPE_UINT8]
#define VTK_PARSE_INT16 vtkParseTypeMap[VTK_TYPE_INT16]
#define VTK_PARSE_UINT16 vtkParseTypeMap[VTK_TYPE_UINT16]
#define VTK_PARSE_INT32 vtkParseTypeMap[VTK_TYPE_INT32]
#define VTK_PARSE_UINT32 vtkParseTypeMap[VTK_TYPE_UINT32]
#define VTK_PARSE_INT64 vtkParseTypeMap[VTK_TYPE_INT64]
#define VTK_PARSE_UINT64 vtkParseTypeMap[VTK_TYPE_UINT64]
#define VTK_PARSE_FLOAT32 vtkParseTypeMap[VTK_TYPE_FLOAT32]
#define VTK_PARSE_FLOAT64 vtkParseTypeMap[VTK_TYPE_FLOAT64]

static void vtkParseDebug(const char* s1, const char* s2);

/* Borland and MSVC do not define __STDC__ properly. */
#if !defined(__STDC__)
# if (defined(_MSC_VER) && _MSC_VER >= 1200) || defined(__BORLANDC__)
#  define __STDC__ 1
# endif
#endif

/* Disable warnings in generated code. */
#if defined(_MSC_VER)
# pragma warning (disable: 4127) /* conditional expression is constant */
# pragma warning (disable: 4244) /* conversion to smaller integer type */
#endif
#if defined(__BORLANDC__)
# pragma warn -8004 /* assigned a value that is never used */
# pragma warn -8008 /* conditional is always true */
# pragma warn -8066 /* unreachable code */
#endif

int yylex(void);
void output_function(void);

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

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 200 "vtkParse.y"
{
  char *str;
  int   integer;
  }
/* Line 193 of yacc.c.  */
#line 418 "vtkParse.tab.c"
  YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 431 "vtkParse.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
       && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
   || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)    \
      do          \
  {         \
    YYSIZE_T yyi;       \
    for (yyi = 0; yyi < (Count); yyi++) \
      (To)[yyi] = (From)[yyi];    \
  }         \
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)          \
    do                  \
      {                 \
  YYSIZE_T yynewbytes;            \
  YYCOPY (&yyptr->Stack, Stack, yysize);        \
  Stack = &yyptr->Stack;            \
  yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
  yyptr += yynewbytes / sizeof (*yyptr);        \
      }                 \
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  61
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   826

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  82
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  74
/* YYNRULES -- Number of rules.  */
#define YYNRULES  195
/* YYNRULES -- Number of states.  */
#define YYNSTATES  368

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   321

#define YYTRANSLATE(YYX)            \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    78,     2,
      71,    72,    79,     2,    75,    80,    81,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    69,    70,
       2,    74,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    76,     2,    77,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    67,     2,    68,    73,     2,     2,     2,
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
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
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
     252,   254,   256,   258,   260,   262,   264,   266,   268,   270,
     272,   274,   276,   278,   280,   282,   284,   286,   288,   289,
     292,   295,   296,   302,   304,   306,   308,   311,   313,   315,
     319,   321,   322,   330,   331,   332,   341,   342,   348,   349,
     355,   356,   357,   368,   369,   377,   378,   386,   387,   388,
     397,   398,   406,   407,   415,   416,   424,   425,   433,   434,
     442,   443,   451,   452,   460,   461,   469,   470,   478,   479,
     489,   490,   500,   505,   510,   517,   525,   526,   529,   530,
     533,   535,   537,   539,   541,   543,   545,   547,   549,   551,
     553,   555,   557,   559,   561,   563,   565,   567,   569,   571,
     573,   575,   577,   579,   583,   587
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      83,     0,    -1,   149,    84,   149,    -1,    -1,     3,    28,
      85,   120,    67,    86,    68,    -1,    87,    -1,    87,    86,
      -1,   123,    69,    -1,   108,    -1,    90,    -1,    27,    90,
      -1,    89,   101,    -1,    27,    89,   101,    -1,    88,   101,
      -1,   126,    70,    -1,   126,    -1,    32,    71,    89,    72,
      -1,    73,    92,    -1,     7,    73,    92,    -1,    92,    -1,
     112,    92,    -1,   112,    24,    92,    -1,     7,   112,    24,
      92,    -1,     7,   112,    92,    -1,     7,    92,    -1,    91,
      -1,   112,    91,    -1,   112,    24,    91,    -1,     7,   112,
      24,    91,    -1,     7,   112,    91,    -1,     7,    91,    -1,
      25,   150,    70,    -1,    -1,    -1,    96,    93,    95,    94,
      -1,    96,    74,     9,    -1,    -1,    24,    -1,    -1,   100,
      71,    97,   102,    72,    -1,    24,    -1,    29,    -1,    28,
      -1,    10,    -1,    70,    -1,    67,   149,    68,    70,    -1,
      67,   149,    68,    -1,    69,   150,    70,    -1,    -1,   103,
      -1,   105,    -1,    -1,   105,   104,    75,   103,    -1,   112,
      -1,    -1,   112,   109,   106,   107,    -1,    30,    -1,    -1,
      74,   124,    -1,   112,   109,    70,    -1,    30,    70,    -1,
     100,   110,    -1,    -1,    -1,    31,   111,   110,    -1,    76,
     150,    77,   110,    -1,    98,   113,    -1,   113,    -1,    99,
     113,    -1,    99,    98,   113,    -1,   117,    -1,   117,   116,
      -1,   114,    -1,   115,    -1,   115,    78,    -1,   115,    79,
      -1,    44,    -1,    78,    -1,    79,    -1,    78,   116,    -1,
      79,   116,    -1,    -1,    26,   118,   119,    -1,   119,    -1,
      33,    -1,    34,    -1,    35,    -1,    36,    -1,    37,    -1,
      38,    -1,    39,    -1,    40,    -1,    41,    -1,    42,    -1,
      12,    -1,    18,    -1,    19,    -1,    11,    -1,    13,    -1,
      14,    -1,    17,    -1,    10,    -1,    28,    -1,    43,    -1,
      15,    -1,    16,    -1,    20,    -1,    21,    -1,    -1,    69,
     121,    -1,   123,    28,    -1,    -1,   123,    28,   122,    75,
     121,    -1,     4,    -1,     5,    -1,     6,    -1,    80,   125,
      -1,   125,    -1,     9,    -1,     9,    81,     9,    -1,   100,
      -1,    -1,    45,    71,   100,    75,   127,   117,    72,    -1,
      -1,    -1,    46,    71,   128,   100,    75,   129,   117,    72,
      -1,    -1,    47,    71,   130,   100,    72,    -1,    -1,    48,
      71,   131,   100,    72,    -1,    -1,    -1,    49,    71,   100,
      75,   132,   117,   133,    75,   150,    72,    -1,    -1,    50,
      71,   100,    75,   134,   117,    72,    -1,    -1,    51,    71,
     100,    75,   135,   117,    72,    -1,    -1,    -1,    52,    71,
     136,   100,    75,   137,   117,    72,    -1,    -1,    53,    71,
     100,   138,    75,   117,    72,    -1,    -1,    54,    71,   100,
      75,   139,   117,    72,    -1,    -1,    58,    71,   100,    75,
     140,   117,    72,    -1,    -1,    55,    71,   100,    75,   141,
     117,    72,    -1,    -1,    59,    71,   100,    75,   142,   117,
      72,    -1,    -1,    56,    71,   100,    75,   143,   117,    72,
      -1,    -1,    60,    71,   100,    75,   144,   117,    72,    -1,
      -1,    57,    71,   100,    75,   145,   117,    72,    -1,    -1,
      61,    71,   100,    75,   146,   117,    72,    -1,    -1,    62,
      71,   100,    75,   147,   117,    75,   124,    72,    -1,    -1,
      63,    71,   100,    75,   148,   117,    75,   124,    72,    -1,
      64,    71,   100,    72,    -1,    65,    71,   100,    72,    -1,
      66,    71,   100,    75,   100,    72,    -1,    66,    71,   100,
      75,   100,    75,    72,    -1,    -1,   151,   149,    -1,    -1,
     152,   150,    -1,    70,    -1,   152,    -1,    23,    -1,   153,
      -1,   154,    -1,    79,    -1,    74,    -1,    69,    -1,    75,
      -1,    81,    -1,     8,    -1,   117,    -1,   115,    -1,     9,
      -1,    22,    -1,    78,    -1,   155,    -1,    24,    -1,    25,
      -1,    80,    -1,    73,    -1,    29,    -1,    31,    -1,    67,
     149,    68,    -1,    71,   149,    72,    -1,    76,   149,    77,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   276,   276,   279,   278,   284,   284,   286,   286,   287,
     288,   289,   290,   291,   292,   293,   295,   297,   298,   299,
     300,   304,   308,   313,   318,   324,   328,   333,   338,   344,
     350,   356,   362,   362,   362,   368,   377,   377,   379,   379,
     381,   383,   385,   385,   387,   388,   389,   390,   392,   392,
     394,   395,   395,   397,   403,   402,   409,   416,   416,   418,
     418,   420,   428,   429,   429,   432,   435,   436,   437,   438,
     440,   441,   443,   445,   446,   447,   449,   459,   460,   461,
     462,   464,   464,   466,   469,   470,   471,   472,   473,   474,
     475,   476,   477,   478,   479,   480,   481,   482,   483,   484,
     485,   486,   491,   508,   509,   510,   511,   512,   514,   514,
     516,   522,   521,   527,   528,   529,   531,   531,   533,   534,
     534,   538,   537,   549,   549,   549,   558,   558,   569,   569,
     579,   580,   578,   611,   610,   623,   622,   634,   635,   634,
     644,   643,   661,   660,   691,   690,   708,   707,   740,   739,
     757,   756,   791,   790,   808,   807,   846,   845,   863,   862,
     881,   880,   897,   944,   993,  1044,  1100,  1100,  1101,  1101,
    1103,  1103,  1105,  1105,  1105,  1105,  1105,  1105,  1105,  1105,
    1106,  1106,  1106,  1106,  1106,  1106,  1106,  1107,  1107,  1107,
    1107,  1107,  1107,  1109,  1110,  1111
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "CLASS", "PUBLIC", "PRIVATE",
  "PROTECTED", "VIRTUAL", "STRING", "NUM", "ID", "INT", "FLOAT", "SHORT",
  "LONG", "LONG_LONG", "INT64__", "DOUBLE", "VOID", "CHAR", "SIGNED_CHAR",
  "BOOL", "CLASS_REF", "OTHER", "CONST", "OPERATOR", "UNSIGNED", "FRIEND",
  "VTK_ID", "STATIC", "VAR_FUNCTION", "ARRAY_NUM", "VTK_LEGACY",
  "TypeInt8", "TypeUInt8", "TypeInt16", "TypeUInt16", "TypeInt32",
  "TypeUInt32", "TypeInt64", "TypeUInt64", "TypeFloat32", "TypeFloat64",
  "IdType", "StdString", "SetMacro", "GetMacro", "SetStringMacro",
  "GetStringMacro", "SetClampMacro", "SetObjectMacro",
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
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   123,   125,    58,
      59,    40,    41,   126,    61,    44,    91,    93,    38,    42,
      45,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    82,    83,    85,    84,    86,    86,    87,    87,    87,
      87,    87,    87,    87,    87,    87,    88,    89,    89,    89,
      89,    89,    89,    89,    89,    90,    90,    90,    90,    90,
      90,    91,    93,    94,    92,    92,    95,    95,    97,    96,
      98,    99,   100,   100,   101,   101,   101,   101,   102,   102,
     103,   104,   103,   105,   106,   105,   105,   107,   107,   108,
     108,   109,   110,   111,   110,   110,   112,   112,   112,   112,
     113,   113,   113,   114,   114,   114,   115,   116,   116,   116,
     116,   118,   117,   117,   119,   119,   119,   119,   119,   119,
     119,   119,   119,   119,   119,   119,   119,   119,   119,   119,
     119,   119,   119,   119,   119,   119,   119,   119,   120,   120,
     121,   122,   121,   123,   123,   123,   124,   124,   125,   125,
     125,   127,   126,   128,   129,   126,   130,   126,   131,   126,
     132,   133,   126,   134,   126,   135,   126,   136,   137,   126,
     138,   126,   139,   126,   140,   126,   141,   126,   142,   126,
     143,   126,   144,   126,   145,   126,   146,   126,   147,   126,
     148,   126,   126,   126,   126,   126,   149,   149,   150,   150,
     151,   151,   152,   152,   152,   152,   152,   152,   152,   152,
     152,   152,   152,   152,   152,   152,   152,   152,   152,   152,
     152,   152,   152,   153,   154,   155
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
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
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     0,     2,
       2,     0,     5,     1,     1,     1,     2,     1,     1,     3,
       1,     0,     7,     0,     0,     8,     0,     5,     0,     5,
       0,     0,    10,     0,     7,     0,     7,     0,     0,     8,
       0,     7,     0,     7,     0,     7,     0,     7,     0,     7,
       0,     7,     0,     7,     0,     7,     0,     7,     0,     9,
       0,     9,     4,     4,     6,     7,     0,     2,     0,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     3,     3,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
     166,   180,   183,   101,    97,    94,    98,    99,   104,   105,
     100,    95,    96,   106,   107,   184,   172,   187,   188,    81,
     102,   191,   192,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,   103,    76,   166,   177,   170,   166,   190,
     176,   178,   166,   185,   175,   189,   179,     0,   182,   181,
      83,     0,   166,   171,   173,   174,   186,     0,     0,     0,
       0,     1,     0,   166,   167,    82,   193,   194,   195,     3,
       2,   108,     0,     0,   113,   114,   115,   109,     0,     0,
     110,     0,   101,    40,   168,     0,   102,    41,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     5,     0,     0,     9,    25,    19,
      32,     0,     0,     0,     8,     0,    67,    72,    73,    70,
       0,    15,     0,     0,    30,    24,     0,     0,   168,     0,
      10,     0,    60,     0,     0,   123,   126,   128,     0,     0,
       0,   137,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    43,    42,    17,     4,
       6,   166,   168,    44,    13,    11,     0,    36,    66,     0,
      68,    38,     0,    26,    20,    62,     0,    74,    75,    77,
      78,    71,     7,    14,     0,    18,     0,    29,    23,    31,
     169,    12,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   140,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    35,
      37,    33,    69,    48,    27,    21,    63,   168,    61,    59,
      79,    80,   112,    28,    22,     0,    16,     0,   121,     0,
       0,     0,   130,   133,   135,     0,     0,   142,   146,   150,
     154,   144,   148,   152,   156,   158,   160,   162,   163,     0,
      46,    47,    34,    56,     0,    49,    50,    53,    62,     0,
       0,     0,   124,   127,   129,     0,     0,     0,   138,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    45,    39,     0,    62,    54,    64,    62,     0,     0,
     131,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   164,     0,     0,    57,    65,
     122,     0,     0,   134,   136,     0,   141,   143,   147,   151,
     155,   145,   149,   153,   157,     0,     0,   165,    52,     0,
      55,   125,   168,   139,   118,     0,   120,     0,   117,     0,
      58,     0,     0,   116,   159,   161,   132,   119
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    47,    63,    71,   113,   114,   115,   116,   117,   118,
     119,   177,   272,   231,   120,   233,   121,   122,   123,   174,
     274,   275,   303,   276,   328,   350,   124,   186,   238,   278,
     125,   126,   127,    48,   191,    49,    57,    50,    73,    77,
     132,    78,   357,   358,   131,   281,   206,   309,   207,   208,
     285,   332,   286,   287,   212,   313,   256,   290,   294,   291,
     295,   292,   296,   293,   297,   298,   299,    51,   137,    52,
      53,    54,    55,    56
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -277
static const yytype_int16 yypact[] =
{
     264,  -277,  -277,  -277,  -277,  -277,  -277,  -277,  -277,  -277,
    -277,  -277,  -277,  -277,  -277,  -277,  -277,  -277,  -277,  -277,
    -277,  -277,  -277,  -277,  -277,  -277,  -277,  -277,  -277,  -277,
    -277,  -277,  -277,  -277,  -277,   264,  -277,  -277,   264,  -277,
    -277,  -277,   264,  -277,  -277,  -277,  -277,    11,  -277,  -277,
    -277,    21,   264,  -277,  -277,  -277,  -277,   783,   -34,     3,
     -25,  -277,    51,   264,  -277,  -277,  -277,  -277,  -277,  -277,
    -277,    12,    85,    28,  -277,  -277,  -277,  -277,    68,   416,
      22,   562,    27,  -277,   338,   480,    29,  -277,    32,    37,
      38,    40,    41,    43,    46,    47,    48,    49,    57,    61,
      76,    78,    79,    80,    81,    83,    88,    89,    90,    92,
      93,    94,    23,    35,   416,   -39,   -39,  -277,  -277,  -277,
      95,   714,   679,    96,  -277,    13,  -277,  -277,    -7,     7,
      53,    98,    91,    23,  -277,  -277,    52,   101,   338,   -39,
    -277,    13,  -277,   521,    23,  -277,  -277,  -277,    23,    23,
      23,  -277,    23,    23,    23,    23,    23,    23,    23,    23,
      23,    23,    23,    23,    23,    23,  -277,  -277,  -277,  -277,
    -277,   264,   338,  -277,  -277,  -277,   114,   148,  -277,   714,
    -277,  -277,    25,  -277,  -277,   -13,   103,  -277,  -277,     7,
       7,  -277,  -277,  -277,    85,  -277,    25,  -277,  -277,  -277,
    -277,  -277,   603,   102,    54,   100,    23,    23,    23,   104,
     105,   106,    23,  -277,   108,   110,   111,   115,   116,   117,
     119,   120,   121,   122,   126,   127,   125,   109,   131,  -277,
    -277,  -277,  -277,   644,  -277,  -277,  -277,   338,  -277,  -277,
    -277,  -277,  -277,  -277,  -277,    59,  -277,    23,  -277,   129,
     130,   133,  -277,  -277,  -277,   135,   149,  -277,  -277,  -277,
    -277,  -277,  -277,  -277,  -277,  -277,  -277,  -277,  -277,    23,
     136,  -277,  -277,  -277,   151,  -277,   153,    23,   -10,   152,
      23,   749,  -277,  -277,  -277,   749,   749,   749,  -277,   749,
     749,   749,   749,   749,   749,   749,   749,   749,   749,   749,
     -16,  -277,  -277,   155,   -10,  -277,  -277,   -10,   154,   749,
    -277,   160,   161,   749,   162,   164,   165,   166,   167,   168,
     169,   170,   171,   172,   173,  -277,   174,   644,   179,  -277,
    -277,   182,   180,  -277,  -277,   184,  -277,  -277,  -277,  -277,
    -277,  -277,  -277,  -277,  -277,     4,     4,  -277,  -277,     4,
    -277,  -277,   338,  -277,    97,    17,  -277,   185,  -277,   187,
    -277,   189,   235,  -277,  -277,  -277,  -277,  -277
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -277,  -277,  -277,  -277,   150,  -277,  -277,   -83,   177,   -26,
     -20,  -277,  -277,  -277,  -277,  -277,   123,  -277,   -19,   -91,
    -277,   -64,  -277,  -277,  -277,  -277,  -277,   -12,  -258,  -277,
     -76,  -105,  -277,   -75,   -96,   -78,  -277,   209,  -277,    73,
    -277,   -57,  -276,   -86,  -277,  -277,  -277,  -277,  -277,  -277,
    -277,  -277,  -277,  -277,  -277,  -277,  -277,  -277,  -277,  -277,
    -277,  -277,  -277,  -277,  -277,  -277,  -277,   -23,  -130,  -277,
     -84,  -277,  -277,  -277
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -112
static const yytype_int16 yytable[] =
{
     138,   129,   139,   129,   128,   136,   128,   129,   200,   141,
     128,    61,    58,   354,   166,    59,   178,   180,   236,    60,
     306,   236,   130,   166,    62,   175,   354,   166,   171,    64,
     172,   173,   167,   166,    66,   166,   129,   182,    84,   128,
      70,   167,   228,   129,   129,   167,   128,   128,   201,   329,
      84,   167,    68,   167,   138,   134,   325,   130,   181,   326,
     203,   135,   166,   237,   166,   129,   237,   204,   128,   166,
     359,   187,   188,   360,   232,    67,   196,    84,   247,    69,
     167,    72,   167,   280,   355,   189,   190,   167,   138,    74,
      75,    76,   168,   240,   241,    79,    80,  -111,   -43,   183,
     -42,   129,   142,   169,   128,   184,   185,   279,   143,   144,
     197,   145,   146,   195,   147,   183,   198,   148,   149,   150,
     151,   184,   192,   229,   129,   205,   245,   128,   152,   209,
     210,   211,   153,   213,   214,   215,   216,   217,   218,   219,
     220,   221,   222,   223,   224,   225,   226,   154,   227,   155,
     156,   157,   158,   138,   159,   129,   234,   277,   128,   160,
     161,   162,   235,   163,   164,   165,   194,   181,   193,   176,
     243,   199,   230,   239,   246,   248,   244,   270,   362,   252,
     253,   254,   135,   257,   184,   258,   259,   249,   250,   251,
     260,   261,   262,   255,   263,   264,   265,   266,   267,   268,
     269,   271,   283,   308,   282,   284,   301,   310,   311,   312,
     288,   314,   315,   316,   317,   318,   319,   320,   321,   322,
     323,   324,   361,   302,   289,   198,   330,   235,   -51,   307,
     327,   331,   333,   334,   336,   335,   337,   338,   339,   340,
     341,   342,   343,   344,   367,   179,   347,   345,   346,   129,
     300,   277,   128,   349,   351,   352,   353,   364,   304,   365,
     244,   366,   140,   348,   170,   305,    65,   242,   138,   363,
       0,     0,     1,     2,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,     0,    20,    21,     0,    22,     0,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   356,   356,     0,     0,
     356,    35,     0,    36,    37,    38,   356,    39,    40,    41,
      42,     0,    43,    44,    45,    46,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,     0,    20,    21,     0,    22,
       0,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    35,     0,    36,     0,    38,
       0,    39,    40,    41,    42,     0,    43,    44,    45,    46,
      74,    75,    76,    81,     0,     0,    82,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,     0,     0,
      83,    84,    19,    85,    86,    87,    88,     0,    89,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,     0,     0,     0,     0,    81,     0,   112,
      82,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,     0,     0,    83,    84,    19,     0,    86,    87,
       0,     0,     0,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,     0,     0,     0,   202,     0,
       0,    82,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,     0,     0,    83,     0,    19,     0,    86,
      87,     0,     0,   112,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,     0,     0,     0,     0,
       0,     0,    82,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,     0,     0,    83,    84,    19,     0,
      86,    87,     0,     0,   112,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,     0,     0,     0,
       0,     0,     0,    82,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,     0,     0,    83,     0,    19,
       0,    86,    87,     0,     0,   133,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,     0,     0,
       0,     0,     0,     0,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,     0,     0,    83,     0,
      19,     0,    20,    87,   273,     0,   133,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,     0,     0,    83,     0,    19,     0,    20,     0,     0,
       0,     0,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,     0,     0,     0,     0,
      19,     0,    20,     0,     0,     0,     0,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,     0,     0,     0,     0,    19,     0,    20,     0,     0,
       0,     0,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,     0,     0,     0,     0,     0,
       0,    20,     0,     0,     0,     0,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33
};

static const yytype_int16 yycheck[] =
{
      84,    79,    85,    81,    79,    81,    81,    85,   138,    85,
      85,     0,    35,     9,    10,    38,   121,   122,    31,    42,
     278,    31,    79,    10,     3,   116,     9,    10,    67,    52,
      69,    70,    28,    10,    68,    10,   114,    24,    25,   114,
      63,    28,   172,   121,   122,    28,   121,   122,   139,   307,
      25,    28,    77,    28,   138,    81,    72,   114,    71,    75,
     143,    81,    10,    76,    10,   143,    76,   143,   143,    10,
     346,    78,    79,   349,   179,    72,    24,    25,    24,    28,
      28,    69,    28,    24,    80,    78,    79,    28,   172,     4,
       5,     6,   112,   189,   190,    67,    28,    75,    71,   125,
      71,   179,    70,    68,   179,   125,   125,   237,    71,    71,
     136,    71,    71,   133,    71,   141,   136,    71,    71,    71,
      71,   141,    69,     9,   202,   144,   202,   202,    71,   148,
     149,   150,    71,   152,   153,   154,   155,   156,   157,   158,
     159,   160,   161,   162,   163,   164,   165,    71,   171,    71,
      71,    71,    71,   237,    71,   233,   182,   233,   233,    71,
      71,    71,   182,    71,    71,    71,    75,    71,    70,    74,
     196,    70,    24,    70,    72,    75,   196,    68,    81,    75,
      75,    75,   202,    75,   204,    75,    75,   206,   207,   208,
      75,    75,    75,   212,    75,    75,    75,    75,    72,    72,
      75,    70,    72,   281,    75,    72,    70,   285,   286,   287,
      75,   289,   290,   291,   292,   293,   294,   295,   296,   297,
     298,   299,   352,    72,    75,   245,    72,   247,    75,    77,
      75,   309,    72,    72,    72,   313,    72,    72,    72,    72,
      72,    72,    72,    72,     9,   122,    72,    75,    75,   327,
     269,   327,   327,    74,    72,    75,    72,    72,   277,    72,
     280,    72,    85,   327,   114,   277,    57,   194,   352,   355,
      -1,    -1,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    -1,    28,    29,    -1,    31,    -1,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   345,   346,    -1,    -1,
     349,    67,    -1,    69,    70,    71,   355,    73,    74,    75,
      76,    -1,    78,    79,    80,    81,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    -1,    28,    29,    -1,    31,
      -1,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    67,    -1,    69,    -1,    71,
      -1,    73,    74,    75,    76,    -1,    78,    79,    80,    81,
       4,     5,     6,     7,    -1,    -1,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    -1,    -1,
      24,    25,    26,    27,    28,    29,    30,    -1,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    -1,    -1,    -1,    -1,     7,    -1,    73,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    -1,    -1,    24,    25,    26,    -1,    28,    29,
      -1,    -1,    -1,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    -1,    -1,    -1,     7,    -1,
      -1,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    -1,    -1,    24,    -1,    26,    -1,    28,
      29,    -1,    -1,    73,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    -1,    -1,    -1,    -1,
      -1,    -1,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    -1,    -1,    24,    25,    26,    -1,
      28,    29,    -1,    -1,    73,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    -1,    -1,    -1,
      -1,    -1,    -1,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    -1,    -1,    24,    -1,    26,
      -1,    28,    29,    -1,    -1,    73,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    -1,    -1,
      -1,    -1,    -1,    -1,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    -1,    -1,    24,    -1,
      26,    -1,    28,    29,    30,    -1,    73,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    -1,    -1,    24,    -1,    26,    -1,    28,    -1,    -1,
      -1,    -1,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    -1,    -1,    -1,    -1,
      26,    -1,    28,    -1,    -1,    -1,    -1,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    -1,    -1,    -1,    -1,    26,    -1,    28,    -1,    -1,
      -1,    -1,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    -1,    -1,    -1,    -1,    -1,
      -1,    28,    -1,    -1,    -1,    -1,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      28,    29,    31,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    67,    69,    70,    71,    73,
      74,    75,    76,    78,    79,    80,    81,    83,   115,   117,
     119,   149,   151,   152,   153,   154,   155,   118,   149,   149,
     149,     0,     3,    84,   149,   119,    68,    72,    77,    28,
     149,    85,    69,   120,     4,     5,     6,   121,   123,    67,
      28,     7,    10,    24,    25,    27,    28,    29,    30,    32,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    73,    86,    87,    88,    89,    90,    91,    92,
      96,    98,    99,   100,   108,   112,   113,   114,   115,   117,
     123,   126,   122,    73,    91,    92,   112,   150,   152,    89,
      90,   112,    70,    71,    71,    71,    71,    71,    71,    71,
      71,    71,    71,    71,    71,    71,    71,    71,    71,    71,
      71,    71,    71,    71,    71,    71,    10,    28,    92,    68,
      86,    67,    69,    70,   101,   101,    74,    93,   113,    98,
     113,    71,    24,    91,    92,   100,   109,    78,    79,    78,
      79,   116,    69,    70,    75,    92,    24,    91,    92,    70,
     150,   101,     7,    89,   112,   100,   128,   130,   131,   100,
     100,   100,   136,   100,   100,   100,   100,   100,   100,   100,
     100,   100,   100,   100,   100,   100,   100,   149,   150,     9,
      24,    95,   113,    97,    91,    92,    31,    76,   110,    70,
     116,   116,   121,    91,    92,   112,    72,    24,    75,   100,
     100,   100,    75,    75,    75,   100,   138,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    75,    72,    72,    75,
      68,    70,    94,    30,   102,   103,   105,   112,   111,   150,
      24,   127,    75,    72,    72,   132,   134,   135,    75,    75,
     139,   141,   143,   145,   140,   142,   144,   146,   147,   148,
     100,    70,    72,   104,   100,   109,   110,    77,   117,   129,
     117,   117,   117,   137,   117,   117,   117,   117,   117,   117,
     117,   117,   117,   117,   117,    72,    75,    75,   106,   110,
      72,   117,   133,    72,    72,   117,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    75,    75,    72,   103,    74,
     107,    72,    75,    72,     9,    80,   100,   124,   125,   124,
     124,   150,    81,   125,    72,    72,    72,     9
};

#define yyerrok   (yyerrstatus = 0)
#define yyclearin (yychar = YYEMPTY)
#define YYEMPTY   (-2)
#define YYEOF   0

#define YYACCEPT  goto yyacceptlab
#define YYABORT   goto yyabortlab
#define YYERROR   goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL    goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)          \
do                \
  if (yychar == YYEMPTY && yylen == 1)        \
    {               \
      yychar = (Token);           \
      yylval = (Value);           \
      yytoken = YYTRANSLATE (yychar);       \
      YYPOPSTACK (1);           \
      goto yybackup;            \
    }               \
  else                \
    {               \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;              \
    }               \
while (YYID (0))


#define YYTERROR  1
#define YYERRCODE 256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)        \
    do                  \
      if (YYID (N))                                                    \
  {               \
    (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;  \
    (Current).first_column = YYRHSLOC (Rhs, 1).first_column;  \
    (Current).last_line    = YYRHSLOC (Rhs, N).last_line;   \
    (Current).last_column  = YYRHSLOC (Rhs, N).last_column; \
  }               \
      else                \
  {               \
    (Current).first_line   = (Current).last_line   =    \
      YYRHSLOC (Rhs, 0).last_line;        \
    (Current).first_column = (Current).last_column =    \
      YYRHSLOC (Rhs, 0).last_column;        \
  }               \
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)      \
     fprintf (File, "%d.%d-%d.%d",      \
        (Loc).first_line, (Loc).first_column, \
        (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
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

# define YYDPRINTF(Args)      \
do {            \
  if (yydebug)          \
    YYFPRINTF Args;       \
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)        \
do {                    \
  if (yydebug)                  \
    {                   \
      YYFPRINTF (stderr, "%s ", Title);           \
      yy_symbol_print (stderr,              \
      Type, Value); \
      YYFPRINTF (stderr, "\n");             \
    }                   \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
  break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)        \
do {                \
  if (yydebug)              \
    yy_stack_print ((Bottom), (Top));       \
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
       yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
           &(yyvsp[(yyi + 1) - (yynrhs)])
                     );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)    \
do {          \
  if (yydebug)        \
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
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
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
  switch (*++yyp)
    {
    case '\'':
    case ',':
      goto do_not_strip_quotes;

    case '\\':
      if (*++yyp != '\\')
        goto do_not_strip_quotes;
      /* Fall through.  */
    default:
      if (yyres)
        yyres[yyn] = *yyp;
      yyn++;
      break;

    case '"':
      if (yyres)
        yyres[yyn] = '\0';
      return yyn;
    }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
   constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
        + sizeof yyexpecting - 1
        + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
           * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
   YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
  if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
    {
      if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
        {
    yycount = 1;
    yysize = yysize0;
    yyformat[sizeof yyunexpected - 1] = '\0';
    break;
        }
      yyarg[yycount++] = yytname[yyx];
      yysize1 = yysize + yytnamerr (0, yytname[yyx]);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;
      yyfmt = yystpcpy (yyfmt, yyprefix);
      yyprefix = yyor;
    }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
  return YYSIZE_MAXIMUM;

      if (yyresult)
  {
    /* Avoid sprintf, as that infringes on the user's name space.
       Don't have undefined behavior even if the translation
       produced a string with the wrong number of "%s"s.  */
    char *yyp = yyresult;
    int yyi = 0;
    while ((*yyp = *yyf) != '\0')
      {
        if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
    {
      yyp += yytnamerr (yyp, yyarg[yyi++]);
      yyf += 2;
    }
        else
    {
      yyp++;
      yyf++;
    }
      }
  }
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
  break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;   /* Cause a token to be read.  */

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
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
  /* Give user a chance to reallocate the stack.  Use copies of
     these so that the &'s don't force the real ones into
     memory.  */
  YYSTYPE *yyvs1 = yyvs;
  yytype_int16 *yyss1 = yyss;


  /* Each stack pointer address is followed by the size of the
     data in use in that stack, in bytes.  This used to be a
     conditional around just the two extra args, but that might
     be undefined if yyoverflow is a macro.  */
  yyoverflow (YY_("memory exhausted"),
        &yyss1, yysize * sizeof (*yyssp),
        &yyvs1, yysize * sizeof (*yyvsp),

        &yystacksize);

  yyss = yyss1;
  yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
  goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
  yystacksize = YYMAXDEPTH;

      {
  yytype_int16 *yyss1 = yyss;
  union yyalloc *yyptr =
    (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
  if (! yyptr)
    goto yyexhaustedlab;
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

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
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
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
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

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

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
#line 279 "vtkParse.y"
    {
      data.ClassName = vtkstrdup((yyvsp[(2) - (2)].str));
      ;}
    break;

  case 11:
#line 289 "vtkParse.y"
    { output_function(); ;}
    break;

  case 12:
#line 290 "vtkParse.y"
    { output_function(); ;}
    break;

  case 13:
#line 291 "vtkParse.y"
    { legacySig(); output_function(); ;}
    break;

  case 17:
#line 297 "vtkParse.y"
    { preSig("~"); ;}
    break;

  case 18:
#line 298 "vtkParse.y"
    { preSig("virtual ~"); ;}
    break;

  case 20:
#line 301 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[(1) - (2)].integer);
         ;}
    break;

  case 21:
#line 305 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[(1) - (3)].integer);
         ;}
    break;

  case 22:
#line 309 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = (yyvsp[(2) - (4)].integer);
         ;}
    break;

  case 23:
#line 314 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = (yyvsp[(2) - (3)].integer);
         ;}
    break;

  case 24:
#line 319 "vtkParse.y"
    {
         preSig("virtual ");
         ;}
    break;

  case 25:
#line 325 "vtkParse.y"
    {
         output_function();
         ;}
    break;

  case 26:
#line 329 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[(1) - (2)].integer);
         output_function();
         ;}
    break;

  case 27:
#line 334 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[(1) - (3)].integer);
         output_function();
         ;}
    break;

  case 28:
#line 339 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = (yyvsp[(2) - (4)].integer);
         output_function();
         ;}
    break;

  case 29:
#line 345 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = (yyvsp[(2) - (3)].integer);
         output_function();
         ;}
    break;

  case 30:
#line 351 "vtkParse.y"
    {
         preSig("virtual ");
         output_function();
         ;}
    break;

  case 31:
#line 357 "vtkParse.y"
    {
      currentFunction->IsOperator = 1;
      vtkParseDebug("Converted operator", 0);
    ;}
    break;

  case 32:
#line 362 "vtkParse.y"
    { postSig(")"); ;}
    break;

  case 33:
#line 362 "vtkParse.y"
    { postSig(";"); openSig = 0; ;}
    break;

  case 34:
#line 363 "vtkParse.y"
    {
      openSig = 1;
      currentFunction->Name = (yyvsp[(1) - (4)].str); 
      vtkParseDebug("Parsed func", (yyvsp[(1) - (4)].str));
    ;}
    break;

  case 35:
#line 369 "vtkParse.y"
    { 
      postSig(") = 0;"); 
      currentFunction->Name = (yyvsp[(1) - (3)].str); 
      vtkParseDebug("Parsed func", (yyvsp[(1) - (3)].str));
      currentFunction->IsPureVirtual = 1; 
      data.IsAbstract = 1;
    ;}
    break;

  case 37:
#line 377 "vtkParse.y"
    {postSig(" const");;}
    break;

  case 38:
#line 379 "vtkParse.y"
    {postSig(" ("); ;}
    break;

  case 40:
#line 381 "vtkParse.y"
    {postSig("const ");;}
    break;

  case 41:
#line 383 "vtkParse.y"
    {postSig("static ");;}
    break;

  case 42:
#line 385 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));;}
    break;

  case 43:
#line 385 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));;}
    break;

  case 50:
#line 394 "vtkParse.y"
    { currentFunction->NumberOfArguments++;;}
    break;

  case 51:
#line 395 "vtkParse.y"
    { currentFunction->NumberOfArguments++; postSig(", ");;}
    break;

  case 53:
#line 398 "vtkParse.y"
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
        (yyvsp[(1) - (1)].integer);;}
    break;

  case 54:
#line 403 "vtkParse.y"
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 
        (yyvsp[(2) - (2)].integer) / 0x10000; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
        (yyvsp[(1) - (2)].integer) + (yyvsp[(2) - (2)].integer) % 0x10000;
    ;}
    break;

  case 56:
#line 410 "vtkParse.y"
    { 
      postSig("void (*func)(void *) ");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 0x5000;
    ;}
    break;

  case 59:
#line 418 "vtkParse.y"
    {delSig();;}
    break;

  case 60:
#line 418 "vtkParse.y"
    {delSig();;}
    break;

  case 61:
#line 420 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); ;}
    break;

  case 62:
#line 428 "vtkParse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 63:
#line 429 "vtkParse.y"
    { char temp[100]; sprintf(temp,"[%i]",(yyvsp[(1) - (1)].integer)); 
                   postSig(temp); ;}
    break;

  case 64:
#line 431 "vtkParse.y"
    { (yyval.integer) = 0x300 + 0x10000 * (yyvsp[(1) - (3)].integer) + (yyvsp[(3) - (3)].integer) % 0x1000; ;}
    break;

  case 65:
#line 433 "vtkParse.y"
    { postSig("[]"); (yyval.integer) = 0x300 + (yyvsp[(4) - (4)].integer) % 0x1000; ;}
    break;

  case 66:
#line 435 "vtkParse.y"
    {(yyval.integer) = 0x1000 + (yyvsp[(2) - (2)].integer);;}
    break;

  case 67:
#line 436 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);;}
    break;

  case 68:
#line 437 "vtkParse.y"
    {(yyval.integer) = 0x2000 + (yyvsp[(2) - (2)].integer);;}
    break;

  case 69:
#line 438 "vtkParse.y"
    {(yyval.integer) = 0x3000 + (yyvsp[(3) - (3)].integer);;}
    break;

  case 70:
#line 440 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);;}
    break;

  case 71:
#line 442 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (2)].integer) + (yyvsp[(2) - (2)].integer);;}
    break;

  case 72:
#line 443 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);;}
    break;

  case 73:
#line 445 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);;}
    break;

  case 74:
#line 446 "vtkParse.y"
    { postSig("&"); (yyval.integer) = (yyvsp[(1) - (2)].integer);;}
    break;

  case 75:
#line 447 "vtkParse.y"
    { postSig("*"); (yyval.integer) = 0x400 + (yyvsp[(1) - (2)].integer);;}
    break;

  case 76:
#line 449 "vtkParse.y"
    { postSig("vtkStdString "); (yyval.integer) = 0x1303; ;}
    break;

  case 77:
#line 459 "vtkParse.y"
    { postSig("&"); (yyval.integer) = 0x100;;}
    break;

  case 78:
#line 460 "vtkParse.y"
    { postSig("*"); (yyval.integer) = 0x300;;}
    break;

  case 79:
#line 461 "vtkParse.y"
    { (yyval.integer) = 0x100 + (yyvsp[(2) - (2)].integer);;}
    break;

  case 80:
#line 462 "vtkParse.y"
    { (yyval.integer) = 0x400 + (yyvsp[(2) - (2)].integer);;}
    break;

  case 81:
#line 464 "vtkParse.y"
    {postSig("unsigned ");;}
    break;

  case 82:
#line 465 "vtkParse.y"
    { (yyval.integer) = 0x10 + (yyvsp[(3) - (3)].integer);;}
    break;

  case 83:
#line 466 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);;}
    break;

  case 84:
#line 469 "vtkParse.y"
    { postSig("vtkTypeInt8 "); (yyval.integer) = VTK_PARSE_INT8; ;}
    break;

  case 85:
#line 470 "vtkParse.y"
    { postSig("vtkTypeUInt8 "); (yyval.integer) = VTK_PARSE_UINT8; ;}
    break;

  case 86:
#line 471 "vtkParse.y"
    { postSig("vtkTypeInt16 "); (yyval.integer) = VTK_PARSE_INT16; ;}
    break;

  case 87:
#line 472 "vtkParse.y"
    { postSig("vtkTypeUInt16 "); (yyval.integer) = VTK_PARSE_UINT16; ;}
    break;

  case 88:
#line 473 "vtkParse.y"
    { postSig("vtkTypeInt32 "); (yyval.integer) = VTK_PARSE_INT32; ;}
    break;

  case 89:
#line 474 "vtkParse.y"
    { postSig("vtkTypeUInt32 "); (yyval.integer) = VTK_PARSE_UINT32; ;}
    break;

  case 90:
#line 475 "vtkParse.y"
    { postSig("vtkTypeInt64 "); (yyval.integer) = VTK_PARSE_INT64; ;}
    break;

  case 91:
#line 476 "vtkParse.y"
    { postSig("vtkTypeUInt64 "); (yyval.integer) = VTK_PARSE_UINT64; ;}
    break;

  case 92:
#line 477 "vtkParse.y"
    { postSig("vtkTypeFloat32 "); (yyval.integer) = VTK_PARSE_FLOAT32; ;}
    break;

  case 93:
#line 478 "vtkParse.y"
    { postSig("vtkTypeFloat64 "); (yyval.integer) = VTK_PARSE_FLOAT64; ;}
    break;

  case 94:
#line 479 "vtkParse.y"
    { postSig("float "); (yyval.integer) = 0x1;;}
    break;

  case 95:
#line 480 "vtkParse.y"
    { postSig("void "); (yyval.integer) = 0x2;;}
    break;

  case 96:
#line 481 "vtkParse.y"
    { postSig("char "); (yyval.integer) = 0x3;;}
    break;

  case 97:
#line 482 "vtkParse.y"
    { postSig("int "); (yyval.integer) = 0x4;;}
    break;

  case 98:
#line 483 "vtkParse.y"
    { postSig("short "); (yyval.integer) = 0x5;;}
    break;

  case 99:
#line 484 "vtkParse.y"
    { postSig("long "); (yyval.integer) = 0x6;;}
    break;

  case 100:
#line 485 "vtkParse.y"
    { postSig("double "); (yyval.integer) = 0x7;;}
    break;

  case 101:
#line 486 "vtkParse.y"
    {       
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",(yyvsp[(1) - (1)].str));
      postSig(ctmpid);
      (yyval.integer) = 0x8;;}
    break;

  case 102:
#line 492 "vtkParse.y"
    { 
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",(yyvsp[(1) - (1)].str));
      postSig(ctmpid);
      (yyval.integer) = 0x9; 
      currentFunction->ArgClasses[currentFunction->NumberOfArguments] =
        vtkstrdup((yyvsp[(1) - (1)].str)); 
      /* store the string into the return value just in case we need it */
      /* this is a parsing hack because the first "type" parser will */
      /* possibly be ht ereturn type of the first argument */
      if ((!currentFunction->ReturnClass) && 
          (!currentFunction->NumberOfArguments)) 
        { 
        currentFunction->ReturnClass = vtkstrdup((yyvsp[(1) - (1)].str)); 
        }
    ;}
    break;

  case 103:
#line 508 "vtkParse.y"
    { postSig("vtkIdType "); (yyval.integer) = 0xA;;}
    break;

  case 104:
#line 509 "vtkParse.y"
    { postSig("long long "); (yyval.integer) = 0xB;;}
    break;

  case 105:
#line 510 "vtkParse.y"
    { postSig("__int64 "); (yyval.integer) = 0xC;;}
    break;

  case 106:
#line 511 "vtkParse.y"
    { postSig("signed char "); (yyval.integer) = 0xD;;}
    break;

  case 107:
#line 512 "vtkParse.y"
    { postSig("bool "); (yyval.integer) = 0xE;;}
    break;

  case 110:
#line 517 "vtkParse.y"
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup((yyvsp[(2) - (2)].str)); 
      data.NumberOfSuperClasses++; 
    ;}
    break;

  case 111:
#line 522 "vtkParse.y"
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup((yyvsp[(2) - (2)].str)); 
      data.NumberOfSuperClasses++; 
    ;}
    break;

  case 113:
#line 527 "vtkParse.y"
    {in_public = 1; in_protected = 0;;}
    break;

  case 114:
#line 528 "vtkParse.y"
    {in_public = 0; in_protected = 0;;}
    break;

  case 115:
#line 529 "vtkParse.y"
    {in_public = 0; in_protected = 1;;}
    break;

  case 118:
#line 533 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);;}
    break;

  case 119:
#line 534 "vtkParse.y"
    {(yyval.integer) = -1;;}
    break;

  case 120:
#line 534 "vtkParse.y"
    {(yyval.integer) = -1;;}
    break;

  case 121:
#line 538 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); ;}
    break;

  case 122:
#line 539 "vtkParse.y"
    {
   postSig(");");
   sprintf(temps,"Set%s",(yyvsp[(3) - (7)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();
   ;}
    break;

  case 123:
#line 549 "vtkParse.y"
    {postSig("Get");;}
    break;

  case 124:
#line 549 "vtkParse.y"
    {postSig(" ();"); invertSig = 1;;}
    break;

  case 125:
#line 551 "vtkParse.y"
    { 
   sprintf(temps,"Get%s",(yyvsp[(4) - (8)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (yyvsp[(7) - (8)].integer);
   output_function();
   ;}
    break;

  case 126:
#line 558 "vtkParse.y"
    {preSig("void Set");;}
    break;

  case 127:
#line 559 "vtkParse.y"
    {
   postSig(" (char *);"); 
   sprintf(temps,"Set%s",(yyvsp[(4) - (5)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x303;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();
   ;}
    break;

  case 128:
#line 569 "vtkParse.y"
    {preSig("char *Get");;}
    break;

  case 129:
#line 570 "vtkParse.y"
    { 
   postSig(" ();");
   sprintf(temps,"Get%s",(yyvsp[(4) - (5)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x303;
   output_function();
   ;}
    break;

  case 130:
#line 579 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); ;}
    break;

  case 131:
#line 580 "vtkParse.y"
    {postSig(");"); openSig = 0;;}
    break;

  case 132:
#line 581 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sscanf (currentFunction->Signature, "%*s %*s (%s);", local);
   sprintf(temps,"Set%s",(yyvsp[(3) - (10)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (yyvsp[(6) - (10)].integer);
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%s Get%sMinValue ();",local,(yyvsp[(3) - (10)].str));
   sprintf(temps,"Get%sMinValue",(yyvsp[(3) - (10)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (yyvsp[(6) - (10)].integer);
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%s Get%sMaxValue ();",local,(yyvsp[(3) - (10)].str));
   sprintf(temps,"Get%sMaxValue",(yyvsp[(3) - (10)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (yyvsp[(6) - (10)].integer);
   output_function();
   ;}
    break;

  case 133:
#line 611 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); ;}
    break;

  case 134:
#line 612 "vtkParse.y"
    { 
   postSig("*);");
   sprintf(temps,"Set%s",(yyvsp[(3) - (7)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 0x2;
   output_function();
   ;}
    break;

  case 135:
#line 623 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); ;}
    break;

  case 136:
#line 624 "vtkParse.y"
    { 
   postSig("*);");
   sprintf(temps,"Set%s",(yyvsp[(3) - (7)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 0x2;
   output_function();
   ;}
    break;

  case 137:
#line 634 "vtkParse.y"
    {postSig("*Get");;}
    break;

  case 138:
#line 635 "vtkParse.y"
    {postSig(" ();"); invertSig = 1;;}
    break;

  case 139:
#line 636 "vtkParse.y"
    { 
   sprintf(temps,"Get%s",(yyvsp[(4) - (8)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x309;
   output_function();
   ;}
    break;

  case 140:
#line 644 "vtkParse.y"
    {preSig("void "); postSig("On ();"); openSig = 0; ;}
    break;

  case 141:
#line 646 "vtkParse.y"
    { 
   sprintf(temps,"%sOn",(yyvsp[(3) - (7)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x2;
   output_function();
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void %sOff ();",(yyvsp[(3) - (7)].str)); 
   sprintf(temps,"%sOff",(yyvsp[(3) - (7)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   output_function();
   ;}
    break;

  case 142:
#line 661 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 143:
#line 666 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s);",(yyvsp[(3) - (7)].str),
     local, local);
   sprintf(temps,"Set%s",(yyvsp[(3) - (7)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 2;
   currentFunction->ArgTypes[0] = (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[2]);",(yyvsp[(3) - (7)].str),
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x300 + (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[0] = 0x2;
   output_function();
   ;}
    break;

  case 144:
#line 691 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 145:
#line 696 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, (yyvsp[(3) - (7)].str));
   sprintf(temps,"Get%s",(yyvsp[(3) - (7)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + (yyvsp[(6) - (7)].integer);
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 2;
   output_function();
   ;}
    break;

  case 146:
#line 708 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 147:
#line 713 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s);",
     (yyvsp[(3) - (7)].str), local, local, local);
   sprintf(temps,"Set%s",(yyvsp[(3) - (7)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 3;
   currentFunction->ArgTypes[0] = (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[3]);",(yyvsp[(3) - (7)].str),
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x300 + (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[0] = 3;
   output_function();
   ;}
    break;

  case 148:
#line 740 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 149:
#line 745 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, (yyvsp[(3) - (7)].str));
   sprintf(temps,"Get%s",(yyvsp[(3) - (7)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + (yyvsp[(6) - (7)].integer);
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 3;
   output_function();
   ;}
    break;

  case 150:
#line 757 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 151:
#line 762 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s, %s);",
     (yyvsp[(3) - (7)].str), local, local, local, local);
   sprintf(temps,"Set%s",(yyvsp[(3) - (7)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 4;
   currentFunction->ArgTypes[0] = (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ArgTypes[3] = (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[3] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[4]);",(yyvsp[(3) - (7)].str),
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x300 + (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[0] = 4;
   output_function();
   ;}
    break;

  case 152:
#line 791 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 153:
#line 796 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, (yyvsp[(3) - (7)].str));
   sprintf(temps,"Get%s",(yyvsp[(3) - (7)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + (yyvsp[(6) - (7)].integer);
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 4;
   output_function();
   ;}
    break;

  case 154:
#line 808 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 155:
#line 813 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s, %s, %s, %s);",
     (yyvsp[(3) - (7)].str), local, local, local, local, local, local);
   sprintf(temps,"Set%s",(yyvsp[(3) - (7)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 6;
   currentFunction->ArgTypes[0] = (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ArgTypes[3] = (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[3] = 0;
   currentFunction->ArgTypes[4] = (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[4] = 0;
   currentFunction->ArgTypes[5] = (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[5] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[6]);",(yyvsp[(3) - (7)].str),
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x300 + (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[0] = 6;
   output_function();
   ;}
    break;

  case 156:
#line 846 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 157:
#line 851 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, (yyvsp[(3) - (7)].str));
   sprintf(temps,"Get%s",(yyvsp[(3) - (7)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + (yyvsp[(6) - (7)].integer);
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 6;
   output_function();
   ;}
    break;

  case 158:
#line 863 "vtkParse.y"
    {
      free (currentFunction->Signature);
      currentFunction->Signature = NULL;
      ;}
    break;

  case 159:
#line 868 "vtkParse.y"
    {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s [%i]);",(yyvsp[(3) - (9)].str),
      local, (yyvsp[(8) - (9)].integer));
     sprintf(temps,"Set%s",(yyvsp[(3) - (9)].str)); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->ReturnType = 0x2;
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 0x300 + (yyvsp[(6) - (9)].integer);
     currentFunction->ArgCounts[0] = (yyvsp[(8) - (9)].integer);
     output_function();
   ;}
    break;

  case 160:
#line 881 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 161:
#line 886 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, (yyvsp[(3) - (9)].str));
   sprintf(temps,"Get%s",(yyvsp[(3) - (9)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + (yyvsp[(6) - (9)].integer);
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = (yyvsp[(8) - (9)].integer);
   output_function();
   ;}
    break;

  case 162:
#line 898 "vtkParse.y"
    { 
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       (yyvsp[(3) - (4)].str));

     sprintf(temps,"Get%sCoordinate",(yyvsp[(3) - (4)].str)); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 0x309;
     currentFunction->ReturnClass = vtkstrdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double, double);",
       (yyvsp[(3) - (4)].str));
     sprintf(temps,"Set%s",(yyvsp[(3) - (4)].str)); 
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
       (yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 0x307;
     currentFunction->ArgCounts[0] = 2;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s ();", (yyvsp[(3) - (4)].str));
     sprintf(temps,"Get%s",(yyvsp[(3) - (4)].str)); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 0x307;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 2;
     output_function();
   ;}
    break;

  case 163:
#line 945 "vtkParse.y"
    { 
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       (yyvsp[(3) - (4)].str));

     sprintf(temps,"Get%sCoordinate",(yyvsp[(3) - (4)].str)); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 0x309;
     currentFunction->ReturnClass = vtkstrdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double, double, double);",
       (yyvsp[(3) - (4)].str));
     sprintf(temps,"Set%s",(yyvsp[(3) - (4)].str)); 
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
       (yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 0x307;
     currentFunction->ArgCounts[0] = 3;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s ();", (yyvsp[(3) - (4)].str));
     sprintf(temps,"Get%s",(yyvsp[(3) - (4)].str)); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 0x307;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 3;
     output_function();
   ;}
    break;

  case 164:
#line 994 "vtkParse.y"
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
           (yyvsp[(3) - (6)].str));
   sprintf(temps,"NewInstance"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x309;
   currentFunction->ReturnClass = vtkstrdup((yyvsp[(3) - (6)].str));
   output_function();

   if ( data.IsConcrete )
     {
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature, "%s *SafeDownCast (vtkObject* o);",
             (yyvsp[(3) - (6)].str));
     sprintf(temps,"SafeDownCast"); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 0x309;
     currentFunction->ArgCounts[0] = 1;
     currentFunction->ArgClasses[0] = vtkstrdup("vtkObject");
     currentFunction->ReturnType = 0x2309;
     currentFunction->ReturnClass = vtkstrdup((yyvsp[(3) - (6)].str));
     output_function();
     }
   ;}
    break;

  case 165:
#line 1045 "vtkParse.y"
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
           (yyvsp[(3) - (7)].str));
   sprintf(temps,"NewInstance"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x309;
   currentFunction->ReturnClass = vtkstrdup((yyvsp[(3) - (7)].str));
   output_function();

   if ( data.IsConcrete )
     {
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature, "%s *SafeDownCast (vtkObject* o);",
             (yyvsp[(3) - (7)].str));
     sprintf(temps,"SafeDownCast"); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 0x309;
     currentFunction->ArgCounts[0] = 1;
     currentFunction->ArgClasses[0] = vtkstrdup("vtkObject");
     currentFunction->ReturnType = 0x2309;
     currentFunction->ReturnClass = vtkstrdup((yyvsp[(3) - (7)].str));
     output_function();
     }
   ;}
    break;


/* Line 1267 of yacc.c.  */
#line 3313 "vtkParse.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
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
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
  YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
  if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
    {
      YYSIZE_T yyalloc = 2 * yysize;
      if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
        yyalloc = YYSTACK_ALLOC_MAXIMUM;
      if (yymsg != yymsgbuf)
        YYSTACK_FREE (yymsg);
      yymsg = (char *) YYSTACK_ALLOC (yyalloc);
      if (yymsg)
        yymsg_alloc = yyalloc;
      else
        {
    yymsg = yymsgbuf;
    yymsg_alloc = sizeof yymsgbuf;
        }
    }

  if (0 < yysize && yysize <= yymsg_alloc)
    {
      (void) yysyntax_error (yymsg, yystate, yychar);
      yyerror (yymsg);
    }
  else
    {
      yyerror (YY_("syntax error"));
      if (yysize != 0)
        goto yyexhaustedlab;
    }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
   error, discard it.  */

      if (yychar <= YYEOF)
  {
    /* Return failure if at end of input.  */
    if (yychar == YYEOF)
      YYABORT;
  }
      else
  {
    yydestruct ("Error: discarding",
          yytoken, &yylval);
    yychar = YYEMPTY;
  }
    }

  yyerrstatus = 3;  /* Each real token shifted decrements this.  */

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


      yydestruct ("Error: popping",
      yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

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
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
     yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
      yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 1113 "vtkParse.y"

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
void look_for_hint(void)
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



