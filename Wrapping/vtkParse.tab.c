
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
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

  void checkSigSize(const char *arg)
    {
    if (strlen(currentFunction->Signature) + strlen(arg) + 3 > 
        sigAllocatedLength)
      {
      currentFunction->Signature = (char *)
        realloc(currentFunction->Signature, sigAllocatedLength*2);
      sigAllocatedLength = sigAllocatedLength*2;
      }
    } 
  void preSig(const char *arg)
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
  void postSig(const char *arg)
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


/* Line 189 of yacc.c  */
#line 259 "vtkParse.tab.c"

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
     TypeMacro = 321,
     VTK_WRAP_EXTERN = 322
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 200 "vtkParse.y"

  char *str;
  int   integer;
  struct {
    char* name;
    int external;
  } vtkid;



/* Line 214 of yacc.c  */
#line 373 "vtkParse.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 385 "vtkParse.tab.c"

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
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
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
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
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
  {          \
    YYSIZE_T yyi;        \
    for (yyi = 0; yyi < (Count); yyi++)  \
      (To)[yyi] = (From)[yyi];    \
  }          \
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)        \
    do                  \
      {                  \
  YYSIZE_T yynewbytes;            \
  YYCOPY (&yyptr->Stack_alloc, Stack, yysize);      \
  Stack = &yyptr->Stack_alloc;          \
  yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
  yyptr += yynewbytes / sizeof (*yyptr);        \
      }                  \
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  63
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   891

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  83
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  75
/* YYNRULES -- Number of rules.  */
#define YYNRULES  197
/* YYNRULES -- Number of states.  */
#define YYNSTATES  370

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   322

#define YYTRANSLATE(YYX)            \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    79,     2,
      72,    73,    80,     2,    76,    81,    82,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    70,    71,
       2,    75,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    77,     2,    78,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    68,     2,    69,    74,     2,     2,     2,
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
      65,    66,    67
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     7,    10,    12,    13,    21,    23,    26,
      29,    31,    33,    36,    39,    43,    46,    49,    51,    56,
      59,    63,    65,    68,    72,    77,    81,    84,    86,    89,
      93,    98,   102,   105,   109,   110,   111,   116,   120,   121,
     123,   124,   130,   132,   134,   136,   138,   140,   145,   149,
     153,   154,   156,   158,   159,   164,   166,   167,   172,   174,
     175,   178,   182,   185,   188,   189,   190,   194,   199,   202,
     204,   207,   211,   213,   216,   218,   220,   223,   226,   228,
     230,   232,   235,   238,   239,   243,   245,   247,   249,   251,
     253,   255,   257,   259,   261,   263,   265,   267,   269,   271,
     273,   275,   277,   279,   281,   283,   285,   287,   289,   291,
     293,   294,   297,   300,   301,   307,   309,   311,   313,   316,
     318,   320,   324,   326,   327,   335,   336,   337,   346,   347,
     353,   354,   360,   361,   362,   373,   374,   382,   383,   391,
     392,   393,   402,   403,   411,   412,   420,   421,   429,   430,
     438,   439,   447,   448,   456,   457,   465,   466,   474,   475,
     483,   484,   494,   495,   505,   510,   515,   522,   530,   531,
     534,   535,   538,   540,   542,   544,   546,   548,   550,   552,
     554,   556,   558,   560,   562,   564,   566,   568,   570,   572,
     574,   576,   578,   580,   582,   584,   588,   592
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      84,     0,    -1,   151,    86,   151,    -1,    28,    67,    -1,
      28,    -1,    -1,     3,    28,    87,   122,    68,    88,    69,
      -1,    89,    -1,    89,    88,    -1,   125,    70,    -1,   110,
      -1,    92,    -1,    27,    92,    -1,    91,   103,    -1,    27,
      91,   103,    -1,    90,   103,    -1,   128,    71,    -1,   128,
      -1,    32,    72,    91,    73,    -1,    74,    94,    -1,     7,
      74,    94,    -1,    94,    -1,   114,    94,    -1,   114,    24,
      94,    -1,     7,   114,    24,    94,    -1,     7,   114,    94,
      -1,     7,    94,    -1,    93,    -1,   114,    93,    -1,   114,
      24,    93,    -1,     7,   114,    24,    93,    -1,     7,   114,
      93,    -1,     7,    93,    -1,    25,   152,    71,    -1,    -1,
      -1,    98,    95,    97,    96,    -1,    98,    75,     9,    -1,
      -1,    24,    -1,    -1,   102,    72,    99,   104,    73,    -1,
      24,    -1,    29,    -1,    85,    -1,    10,    -1,    71,    -1,
      68,   151,    69,    71,    -1,    68,   151,    69,    -1,    70,
     152,    71,    -1,    -1,   105,    -1,   107,    -1,    -1,   107,
     106,    76,   105,    -1,   114,    -1,    -1,   114,   111,   108,
     109,    -1,    30,    -1,    -1,    75,   126,    -1,   114,   111,
      71,    -1,    30,    71,    -1,   102,   112,    -1,    -1,    -1,
      31,   113,   112,    -1,    77,   152,    78,   112,    -1,   100,
     115,    -1,   115,    -1,   101,   115,    -1,   101,   100,   115,
      -1,   119,    -1,   119,   118,    -1,   116,    -1,   117,    -1,
     117,    79,    -1,   117,    80,    -1,    44,    -1,    79,    -1,
      80,    -1,    79,   118,    -1,    80,   118,    -1,    -1,    26,
     120,   121,    -1,   121,    -1,    33,    -1,    34,    -1,    35,
      -1,    36,    -1,    37,    -1,    38,    -1,    39,    -1,    40,
      -1,    41,    -1,    42,    -1,    12,    -1,    18,    -1,    19,
      -1,    11,    -1,    13,    -1,    14,    -1,    17,    -1,    10,
      -1,    85,    -1,    43,    -1,    15,    -1,    16,    -1,    20,
      -1,    21,    -1,    -1,    70,   123,    -1,   125,    28,    -1,
      -1,   125,    28,   124,    76,   123,    -1,     4,    -1,     5,
      -1,     6,    -1,    81,   127,    -1,   127,    -1,     9,    -1,
       9,    82,     9,    -1,   102,    -1,    -1,    45,    72,   102,
      76,   129,   119,    73,    -1,    -1,    -1,    46,    72,   130,
     102,    76,   131,   119,    73,    -1,    -1,    47,    72,   132,
     102,    73,    -1,    -1,    48,    72,   133,   102,    73,    -1,
      -1,    -1,    49,    72,   102,    76,   134,   119,   135,    76,
     152,    73,    -1,    -1,    50,    72,   102,    76,   136,   119,
      73,    -1,    -1,    51,    72,   102,    76,   137,   119,    73,
      -1,    -1,    -1,    52,    72,   138,   102,    76,   139,   119,
      73,    -1,    -1,    53,    72,   102,   140,    76,   119,    73,
      -1,    -1,    54,    72,   102,    76,   141,   119,    73,    -1,
      -1,    58,    72,   102,    76,   142,   119,    73,    -1,    -1,
      55,    72,   102,    76,   143,   119,    73,    -1,    -1,    59,
      72,   102,    76,   144,   119,    73,    -1,    -1,    56,    72,
     102,    76,   145,   119,    73,    -1,    -1,    60,    72,   102,
      76,   146,   119,    73,    -1,    -1,    57,    72,   102,    76,
     147,   119,    73,    -1,    -1,    61,    72,   102,    76,   148,
     119,    73,    -1,    -1,    62,    72,   102,    76,   149,   119,
      76,   126,    73,    -1,    -1,    63,    72,   102,    76,   150,
     119,    76,   126,    73,    -1,    64,    72,   102,    73,    -1,
      65,    72,   102,    73,    -1,    66,    72,   102,    76,   102,
      73,    -1,    66,    72,   102,    76,   102,    76,    73,    -1,
      -1,   153,   151,    -1,    -1,   154,   152,    -1,    71,    -1,
     154,    -1,    23,    -1,   155,    -1,   156,    -1,    80,    -1,
      75,    -1,    70,    -1,    76,    -1,    82,    -1,     8,    -1,
     119,    -1,   117,    -1,     9,    -1,    22,    -1,    79,    -1,
     157,    -1,    24,    -1,    25,    -1,    81,    -1,    74,    -1,
      29,    -1,    31,    -1,    68,   151,    69,    -1,    72,   151,
      73,    -1,    77,   151,    78,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   281,   281,   283,   288,   295,   294,   300,   300,   302,
     302,   303,   304,   305,   306,   307,   308,   309,   311,   313,
     314,   315,   316,   320,   324,   329,   334,   340,   344,   349,
     354,   360,   366,   372,   378,   378,   378,   384,   393,   393,
     395,   395,   397,   399,   401,   401,   403,   404,   405,   406,
     408,   408,   410,   411,   411,   413,   419,   418,   425,   432,
     432,   434,   434,   436,   444,   445,   445,   448,   451,   452,
     453,   454,   456,   457,   459,   461,   462,   463,   465,   475,
     476,   477,   478,   480,   480,   482,   485,   486,   487,   488,
     489,   490,   491,   492,   493,   494,   495,   496,   497,   498,
     499,   500,   501,   502,   507,   527,   528,   529,   530,   531,
     533,   533,   535,   541,   540,   546,   547,   548,   550,   550,
     552,   553,   553,   557,   556,   568,   568,   568,   577,   577,
     588,   588,   598,   599,   597,   630,   629,   642,   641,   653,
     654,   653,   663,   662,   680,   679,   710,   709,   727,   726,
     759,   758,   776,   775,   810,   809,   827,   826,   865,   864,
     882,   881,   900,   899,   916,   963,  1012,  1063,  1119,  1119,
    1120,  1120,  1122,  1122,  1124,  1124,  1124,  1124,  1124,  1124,
    1124,  1124,  1125,  1125,  1125,  1125,  1125,  1125,  1125,  1126,
    1126,  1126,  1126,  1126,  1126,  1128,  1129,  1130
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
  "ViewportCoordinateMacro", "WorldCoordinateMacro", "TypeMacro",
  "VTK_WRAP_EXTERN", "'{'", "'}'", "':'", "';'", "'('", "')'", "'~'",
  "'='", "','", "'['", "']'", "'&'", "'*'", "'-'", "'.'", "$accept",
  "strt", "vtk_id", "class_def", "$@1", "class_def_body", "class_def_item",
  "legacy_function", "function", "operator", "operator_sig", "func", "$@2",
  "$@3", "maybe_const", "func_sig", "$@4", "const_mod", "static_mod",
  "any_id", "func_body", "args_list", "more_args", "$@5", "arg", "$@6",
  "opt_var_assign", "var", "var_id", "var_array", "$@7", "type",
  "type_red1", "type_string1", "type_string2", "type_indirection",
  "type_red2", "$@8", "type_primitive", "optional_scope", "scope_list",
  "$@9", "scope_type", "float_num", "float_prim", "macro", "$@10", "$@11",
  "$@12", "$@13", "$@14", "$@15", "$@16", "$@17", "$@18", "$@19", "$@20",
  "$@21", "$@22", "$@23", "$@24", "$@25", "$@26", "$@27", "$@28", "$@29",
  "$@30", "$@31", "maybe_other", "maybe_other_no_semi", "other_stuff",
  "other_stuff_no_semi", "braces", "parens", "brackets", 0
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
     315,   316,   317,   318,   319,   320,   321,   322,   123,   125,
      58,    59,    40,    41,   126,    61,    44,    91,    93,    38,
      42,    45,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    83,    84,    85,    85,    87,    86,    88,    88,    89,
      89,    89,    89,    89,    89,    89,    89,    89,    90,    91,
      91,    91,    91,    91,    91,    91,    91,    92,    92,    92,
      92,    92,    92,    93,    95,    96,    94,    94,    97,    97,
      99,    98,   100,   101,   102,   102,   103,   103,   103,   103,
     104,   104,   105,   106,   105,   107,   108,   107,   107,   109,
     109,   110,   110,   111,   112,   113,   112,   112,   114,   114,
     114,   114,   115,   115,   115,   116,   116,   116,   117,   118,
     118,   118,   118,   120,   119,   119,   121,   121,   121,   121,
     121,   121,   121,   121,   121,   121,   121,   121,   121,   121,
     121,   121,   121,   121,   121,   121,   121,   121,   121,   121,
     122,   122,   123,   124,   123,   125,   125,   125,   126,   126,
     127,   127,   127,   129,   128,   130,   131,   128,   132,   128,
     133,   128,   134,   135,   128,   136,   128,   137,   128,   138,
     139,   128,   140,   128,   141,   128,   142,   128,   143,   128,
     144,   128,   145,   128,   146,   128,   147,   128,   148,   128,
     149,   128,   150,   128,   128,   128,   128,   128,   151,   151,
     152,   152,   153,   153,   154,   154,   154,   154,   154,   154,
     154,   154,   154,   154,   154,   154,   154,   154,   154,   154,
     154,   154,   154,   154,   154,   155,   156,   157
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     3,     2,     1,     0,     7,     1,     2,     2,
       1,     1,     2,     2,     3,     2,     2,     1,     4,     2,
       3,     1,     2,     3,     4,     3,     2,     1,     2,     3,
       4,     3,     2,     3,     0,     0,     4,     3,     0,     1,
       0,     5,     1,     1,     1,     1,     1,     4,     3,     3,
       0,     1,     1,     0,     4,     1,     0,     4,     1,     0,
       2,     3,     2,     2,     0,     0,     3,     4,     2,     1,
       2,     3,     1,     2,     1,     1,     2,     2,     1,     1,
       1,     2,     2,     0,     3,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       0,     2,     2,     0,     5,     1,     1,     1,     2,     1,
       1,     3,     1,     0,     7,     0,     0,     8,     0,     5,
       0,     5,     0,     0,    10,     0,     7,     0,     7,     0,
       0,     8,     0,     7,     0,     7,     0,     7,     0,     7,
       0,     7,     0,     7,     0,     7,     0,     7,     0,     7,
       0,     9,     0,     9,     4,     4,     6,     7,     0,     2,
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     3,     3,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
     168,   182,   185,   103,    99,    96,   100,   101,   106,   107,
     102,    97,    98,   108,   109,   186,   174,   189,   190,    83,
       4,   193,   194,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,   105,    78,   168,   179,   172,   168,   192,
     178,   180,   168,   187,   177,   191,   181,     0,   104,   184,
     183,    85,     0,   168,   173,   175,   176,   188,     0,     3,
       0,     0,     0,     1,     0,   168,   169,    84,   195,   196,
     197,     5,     2,   110,     0,     0,   115,   116,   117,   111,
       0,     0,   112,     0,   103,    42,   170,     0,    43,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   104,     0,     7,     0,     0,    11,
      27,    21,    34,     0,     0,     0,    10,     0,    69,    74,
      75,    72,     0,    17,     0,     0,    32,    26,     0,     0,
     170,     0,    12,     0,    62,     0,     0,   125,   128,   130,
       0,     0,     0,   139,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    45,    44,
      19,     6,     8,   168,   170,    46,    15,    13,     0,    38,
      68,     0,    70,    40,     0,    28,    22,    64,     0,    76,
      77,    79,    80,    73,     9,    16,     0,    20,     0,    31,
      25,    33,   171,    14,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   142,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    37,    39,    35,    71,    50,    29,    23,    65,   170,
      63,    61,    81,    82,   114,    30,    24,     0,    18,     0,
     123,     0,     0,     0,   132,   135,   137,     0,     0,   144,
     148,   152,   156,   146,   150,   154,   158,   160,   162,   164,
     165,     0,    48,    49,    36,    58,     0,    51,    52,    55,
      64,     0,     0,     0,   126,   129,   131,     0,     0,     0,
     140,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    47,    41,     0,    64,    56,    66,    64,
       0,     0,   133,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   166,     0,     0,
      59,    67,   124,     0,     0,   136,   138,     0,   143,   145,
     149,   153,   157,   147,   151,   155,   159,     0,     0,   167,
      54,     0,    57,   127,   170,   141,   120,     0,   122,     0,
     119,     0,    60,     0,     0,   118,   161,   163,   134,   121
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    47,   169,    65,    73,   115,   116,   117,   118,   119,
     120,   121,   179,   274,   233,   122,   235,   123,   124,   125,
     176,   276,   277,   305,   278,   330,   352,   126,   188,   240,
     280,   127,   128,   129,    49,   193,    50,    58,    51,    75,
      79,   134,    80,   359,   360,   133,   283,   208,   311,   209,
     210,   287,   334,   288,   289,   214,   315,   258,   292,   296,
     293,   297,   294,   298,   295,   299,   300,   301,    52,   139,
      53,    54,    55,    56,    57
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -274
static const yytype_int16 yypact[] =
{
     322,  -274,  -274,  -274,  -274,  -274,  -274,  -274,  -274,  -274,
    -274,  -274,  -274,  -274,  -274,  -274,  -274,  -274,  -274,  -274,
     -44,  -274,  -274,  -274,  -274,  -274,  -274,  -274,  -274,  -274,
    -274,  -274,  -274,  -274,  -274,   322,  -274,  -274,   322,  -274,
    -274,  -274,   322,  -274,  -274,  -274,  -274,    37,  -274,  -274,
    -274,  -274,    46,   322,  -274,  -274,  -274,  -274,   848,  -274,
      -3,   -17,     4,  -274,    56,   322,  -274,  -274,  -274,  -274,
    -274,  -274,  -274,    29,   131,    39,  -274,  -274,  -274,  -274,
      77,   476,    32,   625,    38,  -274,   397,   541,  -274,    41,
      45,    47,    49,    50,    58,    59,    61,    67,    69,    71,
      72,    74,    75,    76,    80,    81,    83,    85,    93,    94,
      95,    96,    97,    22,    98,   102,   476,   -46,   -46,  -274,
    -274,  -274,    53,   779,   744,   100,  -274,    66,  -274,  -274,
     -51,    -2,    68,    79,    73,    22,  -274,  -274,    78,    88,
     397,   -46,  -274,    66,  -274,   583,    22,  -274,  -274,  -274,
      22,    22,    22,  -274,    22,    22,    22,    22,    22,    22,
      22,    22,    22,    22,    22,    22,    22,    22,  -274,  -274,
    -274,  -274,  -274,   322,   397,  -274,  -274,  -274,   166,   152,
    -274,   779,  -274,  -274,    44,  -274,  -274,   -13,   106,  -274,
    -274,    -2,    -2,  -274,  -274,  -274,   131,  -274,    44,  -274,
    -274,  -274,  -274,  -274,   667,   105,    87,   103,    22,    22,
      22,   107,   111,   126,    22,  -274,   127,   132,   133,   137,
     149,   150,   151,   153,   154,   155,   109,   159,   157,   167,
     170,  -274,  -274,  -274,  -274,   709,  -274,  -274,  -274,   397,
    -274,  -274,  -274,  -274,  -274,  -274,  -274,    90,  -274,    22,
    -274,   161,   172,   173,  -274,  -274,  -274,   171,   174,  -274,
    -274,  -274,  -274,  -274,  -274,  -274,  -274,  -274,  -274,  -274,
    -274,    22,   178,  -274,  -274,  -274,   180,  -274,   175,    22,
     -10,   177,    22,   814,  -274,  -274,  -274,   814,   814,   814,
    -274,   814,   814,   814,   814,   814,   814,   814,   814,   814,
     814,   814,   -43,  -274,  -274,   182,   -10,  -274,  -274,   -10,
     183,   814,  -274,   186,   187,   814,   188,   189,   190,   191,
     192,   193,   194,   195,   197,   196,   198,  -274,   200,   709,
     201,  -274,  -274,   202,   203,  -274,  -274,   204,  -274,  -274,
    -274,  -274,  -274,  -274,  -274,  -274,  -274,    17,    17,  -274,
    -274,    17,  -274,  -274,   397,  -274,   199,    52,  -274,   205,
    -274,   207,  -274,   209,   262,  -274,  -274,  -274,  -274,  -274
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -274,  -274,     0,  -274,  -274,   168,  -274,  -274,   -70,   215,
     -64,   -42,  -274,  -274,  -274,  -274,  -274,   162,  -274,    34,
     -84,  -274,   -39,  -274,  -274,  -274,  -274,  -274,     6,  -273,
    -274,   -75,  -121,  -274,   -72,   -66,   -77,  -274,   245,  -274,
     108,  -274,   -76,  -256,   -50,  -274,  -274,  -274,  -274,  -274,
    -274,  -274,  -274,  -274,  -274,  -274,  -274,  -274,  -274,  -274,
    -274,  -274,  -274,  -274,  -274,  -274,  -274,  -274,   -22,  -126,
    -274,   -85,  -274,  -274,  -274
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -114
static const yytype_int16 yytable[] =
{
      48,   140,   180,   182,   131,   132,   131,   308,   138,   130,
     131,   130,   143,    60,   202,   130,    61,   141,   238,   136,
      62,   238,   173,    59,   174,   175,   356,   168,   189,   190,
     327,    66,   168,   328,   177,    48,   331,    63,    48,   131,
     132,   137,    48,    72,   130,    20,   131,   131,   230,    64,
      20,   130,   130,    48,   168,   140,    69,   203,    48,   183,
     234,   356,   168,   185,   239,    48,    68,   239,   131,    86,
     206,   170,    20,   130,   199,   205,   168,   191,   192,   185,
      20,   114,    70,   114,    71,   186,    48,   114,   168,   140,
     184,    86,   361,   197,    20,   362,   200,   168,   357,    74,
     168,   186,   198,    86,   131,    82,    20,    81,  -113,   130,
     -45,   249,   144,   281,   282,    20,   114,   145,    20,   146,
     236,   147,   148,    48,    48,   242,   243,   131,   178,   247,
     149,   150,   130,   151,   245,    76,    77,    78,   194,   152,
      48,   153,   237,   154,   155,   114,   156,   157,   158,   196,
     195,   229,   159,   160,   140,   161,   246,   162,   131,   201,
     279,   187,   137,   130,   186,   163,   164,   165,   166,   167,
     -44,   171,   183,    48,    48,   231,   232,   241,   248,   250,
     207,    48,   269,   254,   211,   212,   213,   255,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     227,   228,   256,   259,   114,   200,   310,   237,   260,   261,
     312,   313,   314,   262,   316,   317,   318,   319,   320,   321,
     322,   323,   324,   325,   326,   263,   264,   265,   363,   266,
     267,   268,   270,   271,   333,    48,   272,   284,   337,    48,
     246,   273,   251,   252,   253,   285,   286,   290,   257,   303,
     291,   -53,   131,   304,   279,   309,   332,   130,   329,   335,
     336,   338,   339,   340,   341,   342,   343,   344,   345,   140,
     346,   369,   347,   349,   348,   353,   351,   355,   366,   354,
     367,   364,   368,    48,   172,   307,   181,    48,    48,    48,
     350,    48,    48,    48,    48,    48,    48,    48,    48,    48,
      48,    48,   142,    67,   244,   302,     0,   365,     0,     0,
       0,    48,     0,   306,     0,    48,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    48,
       1,     2,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,     0,
      20,    21,     0,    22,    48,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   358,   358,     0,     0,   358,     0,     0,     0,     0,
      35,   358,    36,    37,    38,     0,    39,    40,    41,    42,
       0,    43,    44,    45,    46,     1,     2,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,     0,    20,    21,     0,    22,     0,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    35,     0,    36,     0,    38,
       0,    39,    40,    41,    42,     0,    43,    44,    45,    46,
      76,    77,    78,    83,     0,     0,    84,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,     0,     0,
      85,    86,    19,    87,    20,    88,    89,     0,    90,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,     0,     0,     0,     0,     0,    83,     0,
     113,    84,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,     0,     0,    85,    86,    19,     0,    20,
      88,     0,     0,     0,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,     0,     0,     0,     0,
     204,     0,     0,    84,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,     0,     0,    85,     0,    19,
       0,    20,    88,     0,     0,   113,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,     0,     0,
       0,     0,     0,     0,     0,    84,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,     0,     0,    85,
      86,    19,     0,    20,    88,     0,     0,   113,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
       0,     0,     0,     0,     0,     0,     0,    84,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,     0,
       0,    85,     0,    19,     0,    20,    88,     0,     0,   135,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,     0,     0,     0,     0,     0,     0,     0,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,     0,     0,    85,     0,    19,     0,    20,    88,   275,
       0,   135,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,     0,     0,    85,     0,
      19,     0,    20,     0,     0,     0,     0,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,     0,     0,     0,     0,    19,     0,    20,     0,     0,
       0,     0,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,     0,     0,     0,     0,
      19,     0,    20,     0,     0,     0,     0,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
       0,     0,     0,     0,     0,     0,    20,     0,     0,     0,
       0,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33
};

static const yytype_int16 yycheck[] =
{
       0,    86,   123,   124,    81,    81,    83,   280,    83,    81,
      87,    83,    87,    35,   140,    87,    38,    87,    31,    83,
      42,    31,    68,    67,    70,    71,     9,    10,    79,    80,
      73,    53,    10,    76,   118,    35,   309,     0,    38,   116,
     116,    83,    42,    65,   116,    28,   123,   124,   174,     3,
      28,   123,   124,    53,    10,   140,    73,   141,    58,    72,
     181,     9,    10,   127,    77,    65,    69,    77,   145,    25,
     145,   113,    28,   145,   138,   145,    10,    79,    80,   143,
      28,    81,    78,    83,    28,   127,    86,    87,    10,   174,
      24,    25,   348,   135,    28,   351,   138,    10,    81,    70,
      10,   143,    24,    25,   181,    28,    28,    68,    76,   181,
      72,    24,    71,   239,    24,    28,   116,    72,    28,    72,
     184,    72,    72,   123,   124,   191,   192,   204,    75,   204,
      72,    72,   204,    72,   198,     4,     5,     6,    70,    72,
     140,    72,   184,    72,    72,   145,    72,    72,    72,    76,
      71,   173,    72,    72,   239,    72,   198,    72,   235,    71,
     235,   127,   204,   235,   206,    72,    72,    72,    72,    72,
      72,    69,    72,   173,   174,     9,    24,    71,    73,    76,
     146,   181,    73,    76,   150,   151,   152,    76,   154,   155,
     156,   157,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   167,    76,    76,   204,   247,   283,   249,    76,    76,
     287,   288,   289,    76,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,    76,    76,    76,   354,    76,
      76,    76,    73,    76,   311,   235,    69,    76,   315,   239,
     282,    71,   208,   209,   210,    73,    73,    76,   214,    71,
      76,    76,   329,    73,   329,    78,    73,   329,    76,    73,
      73,    73,    73,    73,    73,    73,    73,    73,    73,   354,
      73,     9,    76,    73,    76,    73,    75,    73,    73,    76,
      73,    82,    73,   283,   116,   279,   124,   287,   288,   289,
     329,   291,   292,   293,   294,   295,   296,   297,   298,   299,
     300,   301,    87,    58,   196,   271,    -1,   357,    -1,    -1,
      -1,   311,    -1,   279,    -1,   315,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   329,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    -1,
      28,    29,    -1,    31,   354,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   347,   348,    -1,    -1,   351,    -1,    -1,    -1,    -1,
      68,   357,    70,    71,    72,    -1,    74,    75,    76,    77,
      -1,    79,    80,    81,    82,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    28,    29,    -1,    31,    -1,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    68,    -1,    70,    -1,    72,
      -1,    74,    75,    76,    77,    -1,    79,    80,    81,    82,
       4,     5,     6,     7,    -1,    -1,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    -1,    -1,
      24,    25,    26,    27,    28,    29,    30,    -1,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    -1,    -1,    -1,    -1,    -1,     7,    -1,
      74,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    -1,    -1,    24,    25,    26,    -1,    28,
      29,    -1,    -1,    -1,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    -1,    -1,    -1,    -1,
       7,    -1,    -1,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    -1,    -1,    24,    -1,    26,
      -1,    28,    29,    -1,    -1,    74,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    -1,    -1,    24,
      25,    26,    -1,    28,    29,    -1,    -1,    74,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    -1,
      -1,    24,    -1,    26,    -1,    28,    29,    -1,    -1,    74,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    -1,    -1,    24,    -1,    26,    -1,    28,    29,    30,
      -1,    74,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    -1,    -1,    24,    -1,
      26,    -1,    28,    -1,    -1,    -1,    -1,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    -1,    -1,    -1,    -1,    26,    -1,    28,    -1,    -1,
      -1,    -1,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    -1,    -1,    -1,    -1,
      26,    -1,    28,    -1,    -1,    -1,    -1,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      -1,    -1,    -1,    -1,    -1,    -1,    28,    -1,    -1,    -1,
      -1,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      28,    29,    31,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    68,    70,    71,    72,    74,
      75,    76,    77,    79,    80,    81,    82,    84,    85,   117,
     119,   121,   151,   153,   154,   155,   156,   157,   120,    67,
     151,   151,   151,     0,     3,    86,   151,   121,    69,    73,
      78,    28,   151,    87,    70,   122,     4,     5,     6,   123,
     125,    68,    28,     7,    10,    24,    25,    27,    29,    30,
      32,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    74,    85,    88,    89,    90,    91,    92,
      93,    94,    98,   100,   101,   102,   110,   114,   115,   116,
     117,   119,   125,   128,   124,    74,    93,    94,   114,   152,
     154,    91,    92,   114,    71,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    10,    85,
      94,    69,    88,    68,    70,    71,   103,   103,    75,    95,
     115,   100,   115,    72,    24,    93,    94,   102,   111,    79,
      80,    79,    80,   118,    70,    71,    76,    94,    24,    93,
      94,    71,   152,   103,     7,    91,   114,   102,   130,   132,
     133,   102,   102,   102,   138,   102,   102,   102,   102,   102,
     102,   102,   102,   102,   102,   102,   102,   102,   102,   151,
     152,     9,    24,    97,   115,    99,    93,    94,    31,    77,
     112,    71,   118,   118,   123,    93,    94,   114,    73,    24,
      76,   102,   102,   102,    76,    76,    76,   102,   140,    76,
      76,    76,    76,    76,    76,    76,    76,    76,    76,    73,
      73,    76,    69,    71,    96,    30,   104,   105,   107,   114,
     113,   152,    24,   129,    76,    73,    73,   134,   136,   137,
      76,    76,   141,   143,   145,   147,   142,   144,   146,   148,
     149,   150,   102,    71,    73,   106,   102,   111,   112,    78,
     119,   131,   119,   119,   119,   139,   119,   119,   119,   119,
     119,   119,   119,   119,   119,   119,   119,    73,    76,    76,
     108,   112,    73,   119,   135,    73,    73,   119,    73,    73,
      73,    73,    73,    73,    73,    73,    73,    76,    76,    73,
     105,    75,   109,    73,    76,    73,     9,    81,   102,   126,
     127,   126,   126,   152,    82,   127,    73,    73,    73,     9
};

#define yyerrok    (yyerrstatus = 0)
#define yyclearin  (yychar = YYEMPTY)
#define YYEMPTY    (-2)
#define YYEOF    0

#define YYACCEPT  goto yyacceptlab
#define YYABORT    goto yyabortlab
#define YYERROR    goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL    goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)          \
do                \
  if (yychar == YYEMPTY && yylen == 1)        \
    {                \
      yychar = (Token);            \
      yylval = (Value);            \
      yytoken = YYTRANSLATE (yychar);        \
      YYPOPSTACK (1);            \
      goto yybackup;            \
    }                \
  else                \
    {                \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;              \
    }                \
while (YYID (0))


#define YYTERROR  1
#define YYERRCODE  256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)        \
    do                  \
      if (YYID (N))                                                    \
  {                \
    (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;  \
    (Current).first_column = YYRHSLOC (Rhs, 1).first_column;  \
    (Current).last_line    = YYRHSLOC (Rhs, N).last_line;    \
    (Current).last_column  = YYRHSLOC (Rhs, N).last_column;  \
  }                \
      else                \
  {                \
    (Current).first_line   = (Current).last_line   =    \
      YYRHSLOC (Rhs, 0).last_line;        \
    (Current).first_column = (Current).last_column =    \
      YYRHSLOC (Rhs, 0).last_column;        \
  }                \
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)      \
     fprintf (File, "%d.%d-%d.%d",      \
        (Loc).first_line, (Loc).first_column,  \
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
    YYFPRINTF Args;        \
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)        \
do {                    \
  if (yydebug)                  \
    {                    \
      YYFPRINTF (stderr, "%s ", Title);            \
      yy_symbol_print (stderr,              \
      Type, Value); \
      YYFPRINTF (stderr, "\n");              \
    }                    \
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
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)        \
do {                \
  if (yydebug)              \
    yy_stack_print ((Bottom), (Top));        \
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
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
           &(yyvsp[(yyi + 1) - (yynrhs)])
                      );
      YYFPRINTF (stderr, "\n");
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
#ifndef  YYINITDEPTH
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


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

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
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

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
  YYSTACK_RELOCATE (yyss_alloc, yyss);
  YYSTACK_RELOCATE (yyvs_alloc, yyvs);
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

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

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

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
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

/* Line 1455 of yacc.c  */
#line 284 "vtkParse.y"
    {
           ((yyval.vtkid)).name = (yyvsp[(1) - (2)].str);
           ((yyval.vtkid)).external = 1;
         ;}
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 289 "vtkParse.y"
    {
           ((yyval.vtkid)).name = (yyvsp[(1) - (1)].str);
           ((yyval.vtkid)).external = 0;
         ;}
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 295 "vtkParse.y"
    {
      data.ClassName = vtkstrdup((yyvsp[(2) - (2)].str));
      ;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 305 "vtkParse.y"
    { output_function(); ;}
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 306 "vtkParse.y"
    { output_function(); ;}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 307 "vtkParse.y"
    { legacySig(); output_function(); ;}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 313 "vtkParse.y"
    { preSig("~"); ;}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 314 "vtkParse.y"
    { preSig("virtual ~"); ;}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 317 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[(1) - (2)].integer);
         ;}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 321 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[(1) - (3)].integer);
         ;}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 325 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = (yyvsp[(2) - (4)].integer);
         ;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 330 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = (yyvsp[(2) - (3)].integer);
         ;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 335 "vtkParse.y"
    {
         preSig("virtual ");
         ;}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 341 "vtkParse.y"
    {
         output_function();
         ;}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 345 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[(1) - (2)].integer);
         output_function();
         ;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 350 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[(1) - (3)].integer);
         output_function();
         ;}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 355 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = (yyvsp[(2) - (4)].integer);
         output_function();
         ;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 361 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = (yyvsp[(2) - (3)].integer);
         output_function();
         ;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 367 "vtkParse.y"
    {
         preSig("virtual ");
         output_function();
         ;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 373 "vtkParse.y"
    {
      currentFunction->IsOperator = 1;
      vtkParseDebug("Converted operator", 0);
    ;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 378 "vtkParse.y"
    { postSig(")"); ;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 378 "vtkParse.y"
    { postSig(";"); openSig = 0; ;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 379 "vtkParse.y"
    {
      openSig = 1;
      currentFunction->Name = (yyvsp[(1) - (4)].str); 
      vtkParseDebug("Parsed func", (yyvsp[(1) - (4)].str));
    ;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 385 "vtkParse.y"
    { 
      postSig(") = 0;"); 
      currentFunction->Name = (yyvsp[(1) - (3)].str); 
      vtkParseDebug("Parsed func", (yyvsp[(1) - (3)].str));
      currentFunction->IsPureVirtual = 1; 
      data.IsAbstract = 1;
    ;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 393 "vtkParse.y"
    {postSig(" const");;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 395 "vtkParse.y"
    {postSig(" ("); ;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 397 "vtkParse.y"
    {postSig("const ");;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 399 "vtkParse.y"
    {postSig("static ");;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 401 "vtkParse.y"
    {postSig(((yyvsp[(1) - (1)].vtkid)).name);;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 401 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 410 "vtkParse.y"
    { currentFunction->NumberOfArguments++;;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 411 "vtkParse.y"
    { currentFunction->NumberOfArguments++; postSig(", ");;}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 414 "vtkParse.y"
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
        (yyvsp[(1) - (1)].integer);;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 419 "vtkParse.y"
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 
        (yyvsp[(2) - (2)].integer) / 0x10000; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
        (yyvsp[(1) - (2)].integer) + (yyvsp[(2) - (2)].integer) % 0x10000;
    ;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 426 "vtkParse.y"
    { 
      postSig("void (*func)(void *) ");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 0x5000;
    ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 434 "vtkParse.y"
    {delSig();;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 434 "vtkParse.y"
    {delSig();;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 436 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); ;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 444 "vtkParse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 445 "vtkParse.y"
    { char temp[100]; sprintf(temp,"[%i]",(yyvsp[(1) - (1)].integer)); 
                   postSig(temp); ;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 447 "vtkParse.y"
    { (yyval.integer) = 0x300 + 0x10000 * (yyvsp[(1) - (3)].integer) + (yyvsp[(3) - (3)].integer) % 0x1000; ;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 449 "vtkParse.y"
    { postSig("[]"); (yyval.integer) = 0x300 + (yyvsp[(4) - (4)].integer) % 0x1000; ;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 451 "vtkParse.y"
    {(yyval.integer) = 0x1000 + (yyvsp[(2) - (2)].integer);;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 452 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 453 "vtkParse.y"
    {(yyval.integer) = 0x2000 + (yyvsp[(2) - (2)].integer);;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 454 "vtkParse.y"
    {(yyval.integer) = 0x3000 + (yyvsp[(3) - (3)].integer);;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 456 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 458 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (2)].integer) + (yyvsp[(2) - (2)].integer);;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 459 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 461 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 462 "vtkParse.y"
    { postSig("&"); (yyval.integer) = (yyvsp[(1) - (2)].integer);;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 463 "vtkParse.y"
    { postSig("*"); (yyval.integer) = 0x400 + (yyvsp[(1) - (2)].integer);;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 465 "vtkParse.y"
    { postSig("vtkStdString "); (yyval.integer) = 0x1303; ;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 475 "vtkParse.y"
    { postSig("&"); (yyval.integer) = 0x100;;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 476 "vtkParse.y"
    { postSig("*"); (yyval.integer) = 0x300;;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 477 "vtkParse.y"
    { (yyval.integer) = 0x100 + (yyvsp[(2) - (2)].integer);;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 478 "vtkParse.y"
    { (yyval.integer) = 0x400 + (yyvsp[(2) - (2)].integer);;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 480 "vtkParse.y"
    {postSig("unsigned ");;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 481 "vtkParse.y"
    { (yyval.integer) = 0x10 + (yyvsp[(3) - (3)].integer);;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 482 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);;}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 485 "vtkParse.y"
    { postSig("vtkTypeInt8 "); (yyval.integer) = VTK_PARSE_INT8; ;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 486 "vtkParse.y"
    { postSig("vtkTypeUInt8 "); (yyval.integer) = VTK_PARSE_UINT8; ;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 487 "vtkParse.y"
    { postSig("vtkTypeInt16 "); (yyval.integer) = VTK_PARSE_INT16; ;}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 488 "vtkParse.y"
    { postSig("vtkTypeUInt16 "); (yyval.integer) = VTK_PARSE_UINT16; ;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 489 "vtkParse.y"
    { postSig("vtkTypeInt32 "); (yyval.integer) = VTK_PARSE_INT32; ;}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 490 "vtkParse.y"
    { postSig("vtkTypeUInt32 "); (yyval.integer) = VTK_PARSE_UINT32; ;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 491 "vtkParse.y"
    { postSig("vtkTypeInt64 "); (yyval.integer) = VTK_PARSE_INT64; ;}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 492 "vtkParse.y"
    { postSig("vtkTypeUInt64 "); (yyval.integer) = VTK_PARSE_UINT64; ;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 493 "vtkParse.y"
    { postSig("vtkTypeFloat32 "); (yyval.integer) = VTK_PARSE_FLOAT32; ;}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 494 "vtkParse.y"
    { postSig("vtkTypeFloat64 "); (yyval.integer) = VTK_PARSE_FLOAT64; ;}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 495 "vtkParse.y"
    { postSig("float "); (yyval.integer) = 0x1;;}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 496 "vtkParse.y"
    { postSig("void "); (yyval.integer) = 0x2;;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 497 "vtkParse.y"
    { postSig("char "); (yyval.integer) = 0x3;;}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 498 "vtkParse.y"
    { postSig("int "); (yyval.integer) = 0x4;;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 499 "vtkParse.y"
    { postSig("short "); (yyval.integer) = 0x5;;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 500 "vtkParse.y"
    { postSig("long "); (yyval.integer) = 0x6;;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 501 "vtkParse.y"
    { postSig("double "); (yyval.integer) = 0x7;;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 502 "vtkParse.y"
    {       
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",(yyvsp[(1) - (1)].str));
      postSig(ctmpid);
      (yyval.integer) = 0x8;;}
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 508 "vtkParse.y"
    { 
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",((yyvsp[(1) - (1)].vtkid)).name);
      postSig(ctmpid);
      (yyval.integer) = 0x9; 
      currentFunction->ArgClasses[currentFunction->NumberOfArguments] =
        vtkstrdup(((yyvsp[(1) - (1)].vtkid)).name); 
      currentFunction->ArgExternals[currentFunction->NumberOfArguments] =
        ((yyvsp[(1) - (1)].vtkid)).external;
      /* store the string into the return value just in case we need it */
      /* this is a parsing hack because the first "type" parser will */
      /* possibly be ht ereturn type of the first argument */
      if ((!currentFunction->ReturnClass) && 
          (!currentFunction->NumberOfArguments)) 
        { 
        currentFunction->ReturnClass = vtkstrdup(((yyvsp[(1) - (1)].vtkid)).name); 
        currentFunction->ReturnExternal = ((yyvsp[(1) - (1)].vtkid)).external;
        }
    ;}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 527 "vtkParse.y"
    { postSig("vtkIdType "); (yyval.integer) = 0xA;;}
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 528 "vtkParse.y"
    { postSig("long long "); (yyval.integer) = 0xB;;}
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 529 "vtkParse.y"
    { postSig("__int64 "); (yyval.integer) = 0xC;;}
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 530 "vtkParse.y"
    { postSig("signed char "); (yyval.integer) = 0xD;;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 531 "vtkParse.y"
    { postSig("bool "); (yyval.integer) = 0xE;;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 536 "vtkParse.y"
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup((yyvsp[(2) - (2)].str)); 
      data.NumberOfSuperClasses++; 
    ;}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 541 "vtkParse.y"
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup((yyvsp[(2) - (2)].str)); 
      data.NumberOfSuperClasses++; 
    ;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 546 "vtkParse.y"
    {in_public = 1; in_protected = 0;;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 547 "vtkParse.y"
    {in_public = 0; in_protected = 0;;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 548 "vtkParse.y"
    {in_public = 0; in_protected = 1;;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 552 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 553 "vtkParse.y"
    {(yyval.integer) = -1;;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 553 "vtkParse.y"
    {(yyval.integer) = -1;;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 557 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); ;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 558 "vtkParse.y"
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

  case 125:

/* Line 1455 of yacc.c  */
#line 568 "vtkParse.y"
    {postSig("Get");;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 568 "vtkParse.y"
    {postSig(" ();"); invertSig = 1;;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 570 "vtkParse.y"
    { 
   sprintf(temps,"Get%s",(yyvsp[(4) - (8)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (yyvsp[(7) - (8)].integer);
   output_function();
   ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 577 "vtkParse.y"
    {preSig("void Set");;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 578 "vtkParse.y"
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

  case 130:

/* Line 1455 of yacc.c  */
#line 588 "vtkParse.y"
    {preSig("char *Get");;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 589 "vtkParse.y"
    { 
   postSig(" ();");
   sprintf(temps,"Get%s",(yyvsp[(4) - (5)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x303;
   output_function();
   ;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 598 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); ;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 599 "vtkParse.y"
    {postSig(");"); openSig = 0;;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 600 "vtkParse.y"
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

  case 135:

/* Line 1455 of yacc.c  */
#line 630 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); ;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 631 "vtkParse.y"
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

/* Line 1455 of yacc.c  */
#line 642 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); ;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 643 "vtkParse.y"
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

  case 139:

/* Line 1455 of yacc.c  */
#line 653 "vtkParse.y"
    {postSig("*Get");;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 654 "vtkParse.y"
    {postSig(" ();"); invertSig = 1;;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 655 "vtkParse.y"
    { 
   sprintf(temps,"Get%s",(yyvsp[(4) - (8)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x309;
   output_function();
   ;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 663 "vtkParse.y"
    {preSig("void "); postSig("On ();"); openSig = 0; ;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 665 "vtkParse.y"
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

  case 144:

/* Line 1455 of yacc.c  */
#line 680 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 685 "vtkParse.y"
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

  case 146:

/* Line 1455 of yacc.c  */
#line 710 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 715 "vtkParse.y"
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

  case 148:

/* Line 1455 of yacc.c  */
#line 727 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 732 "vtkParse.y"
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

  case 150:

/* Line 1455 of yacc.c  */
#line 759 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 764 "vtkParse.y"
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

  case 152:

/* Line 1455 of yacc.c  */
#line 776 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 781 "vtkParse.y"
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

  case 154:

/* Line 1455 of yacc.c  */
#line 810 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 815 "vtkParse.y"
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

  case 156:

/* Line 1455 of yacc.c  */
#line 827 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 832 "vtkParse.y"
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

  case 158:

/* Line 1455 of yacc.c  */
#line 865 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 870 "vtkParse.y"
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

  case 160:

/* Line 1455 of yacc.c  */
#line 882 "vtkParse.y"
    {
      free (currentFunction->Signature);
      currentFunction->Signature = NULL;
      ;}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 887 "vtkParse.y"
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

  case 162:

/* Line 1455 of yacc.c  */
#line 900 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     ;}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 905 "vtkParse.y"
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

  case 164:

/* Line 1455 of yacc.c  */
#line 917 "vtkParse.y"
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

  case 165:

/* Line 1455 of yacc.c  */
#line 964 "vtkParse.y"
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

  case 166:

/* Line 1455 of yacc.c  */
#line 1013 "vtkParse.y"
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

  case 167:

/* Line 1455 of yacc.c  */
#line 1064 "vtkParse.y"
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



/* Line 1455 of yacc.c  */
#line 3573 "vtkParse.tab.c"
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
      /* If just tried and failed to reuse lookahead token after an
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

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
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

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
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



/* Line 1675 of yacc.c  */
#line 1132 "vtkParse.y"

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



