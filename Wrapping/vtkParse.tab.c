
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
   system number for the type. */

#include "vtkParseType.h"

static int vtkParseTypeMap[] =
  {
   VTK_PARSE_VOID,               /* VTK_VOID                0 */
   0,                            /* VTK_BIT                 1 */
   VTK_PARSE_CHAR,               /* VTK_CHAR                2 */
   VTK_PARSE_UNSIGNED_CHAR,      /* VTK_UNSIGNED_CHAR       3 */
   VTK_PARSE_SHORT,              /* VTK_SHORT               4 */
   VTK_PARSE_UNSIGNED_SHORT,     /* VTK_UNSIGNED_SHORT      5 */
   VTK_PARSE_INT,                /* VTK_INT                 6 */
   VTK_PARSE_UNSIGNED_INT,       /* VTK_UNSIGNED_INT        7 */
   VTK_PARSE_LONG,               /* VTK_LONG                8 */
   VTK_PARSE_UNSIGNED_LONG,      /* VTK_UNSIGNED_LONG       9 */
   VTK_PARSE_FLOAT,              /* VTK_FLOAT              10 */
   VTK_PARSE_DOUBLE,             /* VTK_DOUBLE             11 */
   VTK_PARSE_ID_TYPE,            /* VTK_ID_TYPE            12 */
   0,                            /* VTK_STRING             13 */
   0,                            /* VTK_OPAQUE             14 */
   VTK_PARSE_SIGNED_CHAR,        /* VTK_SIGNED_CHAR        15 */
   VTK_PARSE_LONG_LONG,          /* VTK_LONG_LONG          16 */
   VTK_PARSE_UNSIGNED_LONG_LONG, /* VTK_UNSIGNED_LONG_LONG 17 */
   VTK_PARSE___INT64,            /* VTK___INT64            18 */
   VTK_PARSE_UNSIGNED___INT64    /* VTK_UNSIGNED___INT64   19 */
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

/* Define the division between type and array count */
#define VTK_PARSE_COUNT_START 0x10000

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
#line 262 "vtkParse.tab.c"

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
     UnicodeString = 300,
     SetMacro = 301,
     GetMacro = 302,
     SetStringMacro = 303,
     GetStringMacro = 304,
     SetClampMacro = 305,
     SetObjectMacro = 306,
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
#line 203 "vtkParse.y"

  char *str;
  int   integer;
  struct {
    char* name;
    int external;
  } vtkid;



/* Line 214 of yacc.c  */
#line 443 "vtkParse.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 455 "vtkParse.tab.c"

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
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  66
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   927

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  83
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  76
/* YYNRULES -- Number of rules.  */
#define YYNRULES  198
/* YYNRULES -- Number of states.  */
#define YYNSTATES  366

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   322

#define YYTRANSLATE(YYX)						\
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
     230,   232,   234,   236,   238,   241,   244,   245,   249,   251,
     253,   255,   257,   259,   261,   263,   265,   267,   269,   271,
     273,   275,   277,   279,   281,   283,   285,   287,   289,   291,
     293,   295,   297,   299,   300,   303,   306,   307,   313,   315,
     317,   319,   322,   324,   326,   330,   332,   333,   341,   342,
     343,   352,   353,   359,   360,   366,   367,   368,   379,   380,
     388,   389,   390,   399,   400,   408,   409,   417,   418,   426,
     427,   435,   436,   444,   445,   453,   454,   462,   463,   471,
     472,   480,   481,   491,   492,   502,   507,   512,   519,   527,
     528,   531,   532,   535,   537,   539,   541,   543,   545,   547,
     549,   551,   553,   555,   557,   559,   561,   563,   565,   567,
     569,   571,   573,   575,   577,   579,   581,   585,   589
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      84,     0,    -1,   152,    86,   152,    -1,    28,    67,    -1,
      28,    -1,    -1,     3,    28,    87,   124,    68,    88,    69,
      -1,    89,    -1,    89,    88,    -1,   127,    70,    -1,   110,
      -1,    92,    -1,    27,    92,    -1,    91,   103,    -1,    27,
      91,   103,    -1,    90,   103,    -1,   130,    71,    -1,   130,
      -1,    32,    72,    91,    73,    -1,    74,    94,    -1,     7,
      74,    94,    -1,    94,    -1,   114,    94,    -1,   114,    24,
      94,    -1,     7,   114,    24,    94,    -1,     7,   114,    94,
      -1,     7,    94,    -1,    93,    -1,   114,    93,    -1,   114,
      24,    93,    -1,     7,   114,    24,    93,    -1,     7,   114,
      93,    -1,     7,    93,    -1,    25,   153,    71,    -1,    -1,
      -1,    98,    95,    97,    96,    -1,    98,    75,     9,    -1,
      -1,    24,    -1,    -1,   102,    72,    99,   104,    73,    -1,
      24,    -1,    29,    -1,    85,    -1,    10,    -1,    71,    -1,
      68,   152,    69,    71,    -1,    68,   152,    69,    -1,    70,
     153,    71,    -1,    -1,   105,    -1,   107,    -1,    -1,   107,
     106,    76,   105,    -1,   114,    -1,    -1,   114,   111,   108,
     109,    -1,    30,    -1,    -1,    75,   128,    -1,   114,   111,
      71,    -1,    30,    71,    -1,   102,   112,    -1,    -1,    -1,
      31,   113,   112,    -1,    77,   153,    78,   112,    -1,   100,
     115,    -1,   115,    -1,   101,   115,    -1,   101,   100,   115,
      -1,   121,    -1,   121,   120,    -1,   116,    -1,   117,    -1,
     117,    79,    -1,   117,    80,    -1,   118,    -1,   119,    -1,
      44,    -1,    45,    -1,    79,    -1,    80,    -1,    79,   120,
      -1,    80,   120,    -1,    -1,    26,   122,   123,    -1,   123,
      -1,    33,    -1,    34,    -1,    35,    -1,    36,    -1,    37,
      -1,    38,    -1,    39,    -1,    40,    -1,    41,    -1,    42,
      -1,    12,    -1,    18,    -1,    19,    -1,    11,    -1,    13,
      -1,    14,    -1,    17,    -1,    10,    -1,    85,    -1,    43,
      -1,    15,    -1,    16,    -1,    20,    -1,    21,    -1,    -1,
      70,   125,    -1,   127,    28,    -1,    -1,   127,    28,   126,
      76,   125,    -1,     4,    -1,     5,    -1,     6,    -1,    81,
     129,    -1,   129,    -1,     9,    -1,     9,    82,     9,    -1,
     102,    -1,    -1,    46,    72,   102,    76,   131,   121,    73,
      -1,    -1,    -1,    47,    72,   132,   102,    76,   133,   121,
      73,    -1,    -1,    48,    72,   134,   102,    73,    -1,    -1,
      49,    72,   135,   102,    73,    -1,    -1,    -1,    50,    72,
     102,    76,   136,   121,   137,    76,   153,    73,    -1,    -1,
      51,    72,   102,    76,   138,   121,    73,    -1,    -1,    -1,
      52,    72,   139,   102,    76,   140,   121,    73,    -1,    -1,
      53,    72,   102,   141,    76,   121,    73,    -1,    -1,    54,
      72,   102,    76,   142,   121,    73,    -1,    -1,    58,    72,
     102,    76,   143,   121,    73,    -1,    -1,    55,    72,   102,
      76,   144,   121,    73,    -1,    -1,    59,    72,   102,    76,
     145,   121,    73,    -1,    -1,    56,    72,   102,    76,   146,
     121,    73,    -1,    -1,    60,    72,   102,    76,   147,   121,
      73,    -1,    -1,    57,    72,   102,    76,   148,   121,    73,
      -1,    -1,    61,    72,   102,    76,   149,   121,    73,    -1,
      -1,    62,    72,   102,    76,   150,   121,    76,   128,    73,
      -1,    -1,    63,    72,   102,    76,   151,   121,    76,   128,
      73,    -1,    64,    72,   102,    73,    -1,    65,    72,   102,
      73,    -1,    66,    72,   102,    76,   102,    73,    -1,    66,
      72,   102,    76,   102,    76,    73,    -1,    -1,   154,   152,
      -1,    -1,   155,   153,    -1,    71,    -1,   155,    -1,    23,
      -1,   156,    -1,   157,    -1,    80,    -1,    75,    -1,    70,
      -1,    76,    -1,    82,    -1,     8,    -1,   121,    -1,   117,
      -1,     9,    -1,    22,    -1,    79,    -1,   158,    -1,    24,
      -1,    25,    -1,    81,    -1,    74,    -1,    29,    -1,    31,
      -1,    68,   152,    69,    -1,    72,   152,    73,    -1,    77,
     152,    78,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   284,   284,   286,   291,   298,   297,   303,   303,   305,
     305,   306,   307,   308,   309,   310,   311,   312,   314,   316,
     317,   318,   319,   323,   327,   332,   337,   343,   347,   352,
     357,   363,   369,   375,   381,   381,   381,   387,   396,   396,
     398,   398,   400,   402,   404,   404,   406,   407,   408,   409,
     411,   411,   413,   414,   414,   416,   422,   421,   428,   435,
     435,   437,   437,   439,   447,   448,   448,   451,   454,   455,
     456,   457,   459,   460,   462,   464,   465,   466,   468,   469,
     471,   473,   482,   483,   484,   485,   487,   487,   489,   492,
     493,   494,   495,   496,   497,   498,   499,   500,   501,   502,
     503,   504,   505,   506,   507,   508,   509,   514,   534,   535,
     536,   537,   538,   540,   540,   542,   548,   547,   553,   554,
     555,   557,   557,   559,   560,   560,   564,   563,   575,   575,
     575,   584,   584,   595,   595,   605,   606,   604,   637,   636,
     648,   649,   648,   658,   657,   675,   674,   705,   704,   722,
     721,   754,   753,   771,   770,   805,   804,   822,   821,   860,
     859,   877,   876,   895,   894,   911,   958,  1007,  1058,  1114,
    1114,  1115,  1115,  1117,  1117,  1119,  1119,  1119,  1119,  1119,
    1119,  1119,  1119,  1120,  1120,  1120,  1120,  1120,  1120,  1120,
    1121,  1121,  1121,  1121,  1121,  1121,  1123,  1124,  1125
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
  "IdType", "StdString", "UnicodeString", "SetMacro", "GetMacro",
  "SetStringMacro", "GetStringMacro", "SetClampMacro", "SetObjectMacro",
  "GetObjectMacro", "BooleanMacro", "SetVector2Macro", "SetVector3Macro",
  "SetVector4Macro", "SetVector6Macro", "GetVector2Macro",
  "GetVector3Macro", "GetVector4Macro", "GetVector6Macro",
  "SetVectorMacro", "GetVectorMacro", "ViewportCoordinateMacro",
  "WorldCoordinateMacro", "TypeMacro", "VTK_WRAP_EXTERN", "'{'", "'}'",
  "':'", "';'", "'('", "')'", "'~'", "'='", "','", "'['", "']'", "'&'",
  "'*'", "'-'", "'.'", "$accept", "strt", "vtk_id", "class_def", "$@1",
  "class_def_body", "class_def_item", "legacy_function", "function",
  "operator", "operator_sig", "func", "$@2", "$@3", "maybe_const",
  "func_sig", "$@4", "const_mod", "static_mod", "any_id", "func_body",
  "args_list", "more_args", "$@5", "arg", "$@6", "opt_var_assign", "var",
  "var_id", "var_array", "$@7", "type", "type_red1", "type_string1",
  "type_string2", "type_string3", "type_string4", "type_indirection",
  "type_red2", "$@8", "type_primitive", "optional_scope", "scope_list",
  "$@9", "scope_type", "float_num", "float_prim", "macro", "$@10", "$@11",
  "$@12", "$@13", "$@14", "$@15", "$@16", "$@17", "$@18", "$@19", "$@20",
  "$@21", "$@22", "$@23", "$@24", "$@25", "$@26", "$@27", "$@28", "$@29",
  "$@30", "maybe_other", "maybe_other_no_semi", "other_stuff",
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
     114,   114,   115,   115,   115,   116,   116,   116,   117,   117,
     118,   119,   120,   120,   120,   120,   122,   121,   121,   123,
     123,   123,   123,   123,   123,   123,   123,   123,   123,   123,
     123,   123,   123,   123,   123,   123,   123,   123,   123,   123,
     123,   123,   123,   124,   124,   125,   126,   125,   127,   127,
     127,   128,   128,   129,   129,   129,   131,   130,   132,   133,
     130,   134,   130,   135,   130,   136,   137,   130,   138,   130,
     139,   140,   130,   141,   130,   142,   130,   143,   130,   144,
     130,   145,   130,   146,   130,   147,   130,   148,   130,   149,
     130,   150,   130,   151,   130,   130,   130,   130,   130,   152,
     152,   153,   153,   154,   154,   155,   155,   155,   155,   155,
     155,   155,   155,   155,   155,   155,   155,   155,   155,   155,
     155,   155,   155,   155,   155,   155,   156,   157,   158
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
       1,     1,     1,     1,     2,     2,     0,     3,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     0,     2,     2,     0,     5,     1,     1,
       1,     2,     1,     1,     3,     1,     0,     7,     0,     0,
       8,     0,     5,     0,     5,     0,     0,    10,     0,     7,
       0,     0,     8,     0,     7,     0,     7,     0,     7,     0,
       7,     0,     7,     0,     7,     0,     7,     0,     7,     0,
       7,     0,     9,     0,     9,     4,     4,     6,     7,     0,
       2,     0,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     3,     3,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
     169,   183,   186,   106,   102,    99,   103,   104,   109,   110,
     105,   100,   101,   111,   112,   187,   175,   190,   191,    86,
       4,   194,   195,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,   108,    80,    81,   169,   180,   173,   169,
     193,   179,   181,   169,   188,   178,   192,   182,     0,   107,
     185,    78,    79,   184,    88,     0,   169,   174,   176,   177,
     189,     0,     3,     0,     0,     0,     1,     0,   169,   170,
      87,   196,   197,   198,     5,     2,   113,     0,     0,   118,
     119,   120,   114,     0,     0,   115,     0,   106,    42,   171,
       0,    43,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   107,     0,     7,     0,
       0,    11,    27,    21,    34,     0,     0,     0,    10,     0,
      69,    74,    75,    72,     0,    17,     0,     0,    32,    26,
       0,     0,   171,     0,    12,     0,    62,     0,     0,   128,
     131,   133,     0,     0,   140,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    45,
      44,    19,     6,     8,   169,   171,    46,    15,    13,     0,
      38,    68,     0,    70,    40,     0,    28,    22,    64,     0,
      76,    77,    82,    83,    73,     9,    16,     0,    20,     0,
      31,    25,    33,   172,    14,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   143,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    37,    39,    35,    71,    50,    29,    23,    65,   171,
      63,    61,    84,    85,   117,    30,    24,     0,    18,     0,
     126,     0,     0,     0,   135,   138,     0,     0,   145,   149,
     153,   157,   147,   151,   155,   159,   161,   163,   165,   166,
       0,    48,    49,    36,    58,     0,    51,    52,    55,    64,
       0,     0,     0,   129,   132,   134,     0,     0,   141,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    47,    41,     0,    64,    56,    66,    64,     0,     0,
     136,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   167,     0,     0,    59,    67,   127,
       0,     0,   139,     0,   144,   146,   150,   154,   158,   148,
     152,   156,   160,     0,     0,   168,    54,     0,    57,   130,
     171,   142,   123,     0,   125,     0,   122,     0,    60,     0,
       0,   121,   162,   164,   137,   124
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    48,   170,    68,    76,   117,   118,   119,   120,   121,
     122,   123,   180,   273,   233,   124,   235,   125,   126,   127,
     177,   275,   276,   303,   277,   327,   348,   128,   189,   240,
     279,   129,   130,   131,    50,    51,    52,   194,    53,    61,
      54,    78,    82,   136,    83,   355,   356,   135,   282,   209,
     309,   210,   211,   286,   331,   287,   214,   312,   257,   290,
     294,   291,   295,   292,   296,   293,   297,   298,   299,    55,
     141,    56,    57,    58,    59,    60
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -276
static const yytype_int16 yypact[] =
{
     389,  -276,  -276,  -276,  -276,  -276,  -276,  -276,  -276,  -276,
    -276,  -276,  -276,  -276,  -276,  -276,  -276,  -276,  -276,  -276,
     -38,  -276,  -276,  -276,  -276,  -276,  -276,  -276,  -276,  -276,
    -276,  -276,  -276,  -276,  -276,  -276,   389,  -276,  -276,   389,
    -276,  -276,  -276,   389,  -276,  -276,  -276,  -276,    24,  -276,
    -276,  -276,  -276,  -276,  -276,    31,   389,  -276,  -276,  -276,
    -276,   884,  -276,   -27,     2,   -25,  -276,    55,   389,  -276,
    -276,  -276,  -276,  -276,  -276,  -276,    21,    54,    25,  -276,
    -276,  -276,  -276,    77,   543,    35,   692,    42,  -276,   464,
     608,  -276,    44,    48,    49,    50,    51,    56,    58,    59,
      60,    62,    63,    64,    65,    66,    67,    69,    71,    72,
      73,    74,    76,    80,    81,     3,    83,    47,   543,    27,
      27,  -276,  -276,  -276,    75,   848,   812,    86,  -276,    52,
    -276,  -276,    28,    30,    70,    46,    84,     3,  -276,  -276,
      57,    91,   464,    27,  -276,    52,  -276,   650,     3,  -276,
    -276,  -276,     3,     3,  -276,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,  -276,
    -276,  -276,  -276,  -276,   389,   464,  -276,  -276,  -276,   140,
     139,  -276,   848,  -276,  -276,    45,  -276,  -276,   -20,    93,
    -276,  -276,    30,    30,  -276,  -276,  -276,    54,  -276,    45,
    -276,  -276,  -276,  -276,  -276,   734,    92,    68,    90,     3,
       3,     3,    94,    95,     3,  -276,    96,    97,   100,   101,
     102,   103,   105,   107,   110,   126,   130,   133,   131,    98,
     151,  -276,  -276,  -276,  -276,   776,  -276,  -276,  -276,   464,
    -276,  -276,  -276,  -276,  -276,  -276,  -276,    78,  -276,     3,
    -276,   147,   152,   153,  -276,  -276,   148,   156,  -276,  -276,
    -276,  -276,  -276,  -276,  -276,  -276,  -276,  -276,  -276,  -276,
       3,   157,  -276,  -276,  -276,   154,  -276,   160,     3,   -12,
     155,     3,   318,  -276,  -276,  -276,   318,   318,  -276,   318,
     318,   318,   318,   318,   318,   318,   318,   318,   318,   318,
     -43,  -276,  -276,   161,   -12,  -276,  -276,   -12,   165,   318,
    -276,   167,   318,   174,   176,   178,   180,   181,   183,   184,
     185,   186,   187,   188,  -276,   192,   776,   191,  -276,  -276,
     194,   193,  -276,   195,  -276,  -276,  -276,  -276,  -276,  -276,
    -276,  -276,  -276,    13,    13,  -276,  -276,    13,  -276,  -276,
     464,  -276,    87,    18,  -276,   197,  -276,   198,  -276,   199,
     159,  -276,  -276,  -276,  -276,  -276
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -276,  -276,     0,  -276,  -276,   111,  -276,  -276,   -83,   171,
     -66,   115,  -276,  -276,  -276,  -276,  -276,   149,  -276,    32,
     -94,  -276,   -53,  -276,  -276,  -276,  -276,  -276,    -4,  -275,
    -276,   -76,  -110,  -276,   -81,  -276,  -276,   -80,   -78,  -276,
     215,  -276,    82,  -276,   -67,  -244,   -75,  -276,  -276,  -276,
    -276,  -276,  -276,  -276,  -276,  -276,  -276,  -276,  -276,  -276,
    -276,  -276,  -276,  -276,  -276,  -276,  -276,  -276,  -276,   -18,
    -140,  -276,   -88,  -276,  -276,  -276
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -117
static const yytype_int16 yytable[] =
{
      49,   142,   203,   132,   306,   132,   133,   143,   133,   132,
     140,   238,   133,   169,   145,   181,   183,   134,    63,   238,
     138,    64,   352,   169,    66,    65,   178,   352,   169,    62,
     324,    20,   328,   325,    67,   230,    49,   132,    69,    49,
     133,    20,    71,    49,   132,   132,    20,   133,   133,   204,
      75,   134,   184,    73,   142,   169,    49,   239,    79,    80,
      81,    49,   169,   186,   206,   239,   132,   169,    49,   133,
      89,   207,   234,    20,   200,    72,   185,    89,   169,   186,
      20,   199,    89,    74,   116,    20,   116,   142,   169,    49,
     116,    77,   249,    84,   353,   174,    20,   175,   176,   280,
     357,   132,   281,   358,   133,    85,    20,   190,   191,   192,
     193,  -116,   242,   243,   -45,   146,   172,   196,   116,   236,
     147,   148,   149,   150,   132,    49,    49,   133,   151,   247,
     152,   153,   154,   245,   155,   156,   157,   158,   159,   160,
     195,   161,    49,   162,   163,   164,   165,   116,   166,   231,
     179,   142,   167,   168,   132,   -44,   229,   133,   184,   278,
     197,   188,   202,   232,   241,   248,   250,   271,   365,   360,
     254,   255,   258,   259,    49,    49,   260,   261,   262,   263,
     208,   264,    49,   265,   212,   213,   266,   215,   216,   217,
     218,   219,   220,   221,   222,   223,   224,   225,   226,   227,
     228,   139,   267,   268,   308,   116,   269,   270,   310,   311,
     359,   313,   314,   315,   316,   317,   318,   319,   320,   321,
     322,   323,   272,   283,   288,   284,   285,   302,   301,   173,
     171,   330,   289,   307,   333,    49,   -53,   326,   329,    49,
     332,   251,   252,   253,   187,   132,   256,   334,   133,   335,
     278,   336,   198,   337,   338,   201,   339,   340,   341,   342,
     187,   144,   142,   343,   344,   345,   347,   349,   351,   350,
     362,   363,   364,   346,   305,   182,    70,     0,   361,   244,
       0,     0,    49,     0,     0,     0,    49,    49,     0,    49,
      49,    49,    49,    49,    49,    49,    49,    49,    49,    49,
     237,     0,   300,     0,     0,     0,     0,     0,     0,    49,
     304,     0,    49,     0,   246,     0,     0,     0,     0,     0,
     139,     0,   187,     0,     0,     0,    49,     0,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
       0,     0,     0,     0,    19,     0,    20,     0,     0,     0,
      49,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,   201,     0,   237,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   354,   354,     0,     0,   354,
       0,     0,     0,     0,     0,   354,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   246,     1,     2,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,     0,    20,    21,     0,
      22,     0,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    36,     0,    37,
      38,    39,     0,    40,    41,    42,    43,     0,    44,    45,
      46,    47,     1,     2,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,     0,    20,    21,     0,    22,     0,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    36,     0,    37,     0,    39,     0,    40,    41,
      42,    43,     0,    44,    45,    46,    47,    79,    80,    81,
      86,     0,     0,    87,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,     0,     0,    88,    89,    19,
      90,    20,    91,    92,     0,    93,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
       0,     0,     0,     0,     0,    86,     0,   115,    87,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
       0,     0,    88,    89,    19,     0,    20,    91,     0,     0,
       0,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,     0,     0,     0,   205,     0,     0,
      87,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,     0,     0,    88,     0,    19,     0,    20,    91,
       0,     0,   115,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,     0,     0,     0,     0,
       0,     0,    87,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,     0,     0,    88,    89,    19,     0,
      20,    91,     0,     0,   115,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,     0,     0,
       0,     0,     0,     0,    87,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,     0,     0,    88,     0,
      19,     0,    20,    91,     0,     0,   137,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
       0,     0,     0,     0,     0,     0,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,     0,     0,
      88,     0,    19,     0,    20,    91,   274,     0,   137,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,     0,     0,    88,     0,    19,     0,
      20,     0,     0,     0,     0,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
       0,     0,     0,     0,    19,     0,    20,     0,     0,     0,
       0,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,     0,     0,     0,     0,
       0,     0,    20,     0,     0,     0,     0,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33
};

static const yytype_int16 yycheck[] =
{
       0,    89,   142,    84,   279,    86,    84,    90,    86,    90,
      86,    31,    90,    10,    90,   125,   126,    84,    36,    31,
      86,    39,     9,    10,     0,    43,   120,     9,    10,    67,
      73,    28,   307,    76,     3,   175,    36,   118,    56,    39,
     118,    28,    69,    43,   125,   126,    28,   125,   126,   143,
      68,   118,    72,    78,   142,    10,    56,    77,     4,     5,
       6,    61,    10,   129,   147,    77,   147,    10,    68,   147,
      25,   147,   182,    28,   140,    73,    24,    25,    10,   145,
      28,    24,    25,    28,    84,    28,    86,   175,    10,    89,
      90,    70,    24,    68,    81,    68,    28,    70,    71,   239,
     344,   182,    24,   347,   182,    28,    28,    79,    80,    79,
      80,    76,   192,   193,    72,    71,    69,    71,   118,   185,
      72,    72,    72,    72,   205,   125,   126,   205,    72,   205,
      72,    72,    72,   199,    72,    72,    72,    72,    72,    72,
      70,    72,   142,    72,    72,    72,    72,   147,    72,     9,
      75,   239,    72,    72,   235,    72,   174,   235,    72,   235,
      76,   129,    71,    24,    71,    73,    76,    69,     9,    82,
      76,    76,    76,    76,   174,   175,    76,    76,    76,    76,
     148,    76,   182,    76,   152,   153,    76,   155,   156,   157,
     158,   159,   160,   161,   162,   163,   164,   165,   166,   167,
     168,    86,    76,    73,   282,   205,    73,    76,   286,   287,
     350,   289,   290,   291,   292,   293,   294,   295,   296,   297,
     298,   299,    71,    76,    76,    73,    73,    73,    71,   118,
     115,   309,    76,    78,   312,   235,    76,    76,    73,   239,
      73,   209,   210,   211,   129,   326,   214,    73,   326,    73,
     326,    73,   137,    73,    73,   140,    73,    73,    73,    73,
     145,    90,   350,    76,    76,    73,    75,    73,    73,    76,
      73,    73,    73,   326,   278,   126,    61,    -1,   353,   197,
      -1,    -1,   282,    -1,    -1,    -1,   286,   287,    -1,   289,
     290,   291,   292,   293,   294,   295,   296,   297,   298,   299,
     185,    -1,   270,    -1,    -1,    -1,    -1,    -1,    -1,   309,
     278,    -1,   312,    -1,   199,    -1,    -1,    -1,    -1,    -1,
     205,    -1,   207,    -1,    -1,    -1,   326,    -1,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      -1,    -1,    -1,    -1,    26,    -1,    28,    -1,    -1,    -1,
     350,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,   247,    -1,   249,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   343,   344,    -1,    -1,   347,
      -1,    -1,    -1,    -1,    -1,   353,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   281,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    -1,    28,    29,    -1,
      31,    -1,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    68,    -1,    70,
      71,    72,    -1,    74,    75,    76,    77,    -1,    79,    80,
      81,    82,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    -1,    28,    29,    -1,    31,    -1,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    68,    -1,    70,    -1,    72,    -1,    74,    75,
      76,    77,    -1,    79,    80,    81,    82,     4,     5,     6,
       7,    -1,    -1,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    -1,    -1,    24,    25,    26,
      27,    28,    29,    30,    -1,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      -1,    -1,    -1,    -1,    -1,     7,    -1,    74,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      -1,    -1,    24,    25,    26,    -1,    28,    29,    -1,    -1,
      -1,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    -1,    -1,    -1,     7,    -1,    -1,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    -1,    -1,    24,    -1,    26,    -1,    28,    29,
      -1,    -1,    74,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    -1,    -1,    -1,    -1,
      -1,    -1,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    -1,    -1,    24,    25,    26,    -1,
      28,    29,    -1,    -1,    74,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    -1,    -1,
      -1,    -1,    -1,    -1,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    -1,    -1,    24,    -1,
      26,    -1,    28,    29,    -1,    -1,    74,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      -1,    -1,    -1,    -1,    -1,    -1,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    -1,    -1,
      24,    -1,    26,    -1,    28,    29,    30,    -1,    74,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    -1,    -1,    24,    -1,    26,    -1,
      28,    -1,    -1,    -1,    -1,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      -1,    -1,    -1,    -1,    26,    -1,    28,    -1,    -1,    -1,
      -1,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    -1,    -1,    -1,    -1,
      -1,    -1,    28,    -1,    -1,    -1,    -1,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      28,    29,    31,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    68,    70,    71,    72,
      74,    75,    76,    77,    79,    80,    81,    82,    84,    85,
     117,   118,   119,   121,   123,   152,   154,   155,   156,   157,
     158,   122,    67,   152,   152,   152,     0,     3,    86,   152,
     123,    69,    73,    78,    28,   152,    87,    70,   124,     4,
       5,     6,   125,   127,    68,    28,     7,    10,    24,    25,
      27,    29,    30,    32,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    74,    85,    88,    89,    90,
      91,    92,    93,    94,    98,   100,   101,   102,   110,   114,
     115,   116,   117,   121,   127,   130,   126,    74,    93,    94,
     114,   153,   155,    91,    92,   114,    71,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    10,
      85,    94,    69,    88,    68,    70,    71,   103,   103,    75,
      95,   115,   100,   115,    72,    24,    93,    94,   102,   111,
      79,    80,    79,    80,   120,    70,    71,    76,    94,    24,
      93,    94,    71,   153,   103,     7,    91,   114,   102,   132,
     134,   135,   102,   102,   139,   102,   102,   102,   102,   102,
     102,   102,   102,   102,   102,   102,   102,   102,   102,   152,
     153,     9,    24,    97,   115,    99,    93,    94,    31,    77,
     112,    71,   120,   120,   125,    93,    94,   114,    73,    24,
      76,   102,   102,   102,    76,    76,   102,   141,    76,    76,
      76,    76,    76,    76,    76,    76,    76,    76,    73,    73,
      76,    69,    71,    96,    30,   104,   105,   107,   114,   113,
     153,    24,   131,    76,    73,    73,   136,   138,    76,    76,
     142,   144,   146,   148,   143,   145,   147,   149,   150,   151,
     102,    71,    73,   106,   102,   111,   112,    78,   121,   133,
     121,   121,   140,   121,   121,   121,   121,   121,   121,   121,
     121,   121,   121,   121,    73,    76,    76,   108,   112,    73,
     121,   137,    73,   121,    73,    73,    73,    73,    73,    73,
      73,    73,    73,    76,    76,    73,   105,    75,   109,    73,
      76,    73,     9,    81,   102,   128,   129,   128,   128,   153,
      82,   129,    73,    73,    73,     9
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
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

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
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

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
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

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
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
#ifndef	YYINITDEPTH
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
#line 287 "vtkParse.y"
    {
           ((yyval.vtkid)).name = (yyvsp[(1) - (2)].str);
           ((yyval.vtkid)).external = 1;
         }
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 292 "vtkParse.y"
    {
           ((yyval.vtkid)).name = (yyvsp[(1) - (1)].str);
           ((yyval.vtkid)).external = 0;
         }
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 298 "vtkParse.y"
    {
      data.ClassName = vtkstrdup((yyvsp[(2) - (2)].str));
      }
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 308 "vtkParse.y"
    { output_function(); }
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 309 "vtkParse.y"
    { output_function(); }
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 310 "vtkParse.y"
    { legacySig(); output_function(); }
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 316 "vtkParse.y"
    { preSig("~"); }
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 317 "vtkParse.y"
    { preSig("virtual ~"); }
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 320 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[(1) - (2)].integer);
         }
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 324 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[(1) - (3)].integer);
         }
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 328 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = (yyvsp[(2) - (4)].integer);
         }
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 333 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = (yyvsp[(2) - (3)].integer);
         }
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 338 "vtkParse.y"
    {
         preSig("virtual ");
         }
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 344 "vtkParse.y"
    {
         output_function();
         }
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 348 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[(1) - (2)].integer);
         output_function();
         }
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 353 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[(1) - (3)].integer);
         output_function();
         }
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 358 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = (yyvsp[(2) - (4)].integer);
         output_function();
         }
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 364 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = (yyvsp[(2) - (3)].integer);
         output_function();
         }
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 370 "vtkParse.y"
    {
         preSig("virtual ");
         output_function();
         }
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 376 "vtkParse.y"
    {
      currentFunction->IsOperator = 1;
      vtkParseDebug("Converted operator", 0);
    }
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 381 "vtkParse.y"
    { postSig(")"); }
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 381 "vtkParse.y"
    { postSig(";"); openSig = 0; }
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 382 "vtkParse.y"
    {
      openSig = 1;
      currentFunction->Name = (yyvsp[(1) - (4)].str); 
      vtkParseDebug("Parsed func", (yyvsp[(1) - (4)].str));
    }
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 388 "vtkParse.y"
    { 
      postSig(") = 0;"); 
      currentFunction->Name = (yyvsp[(1) - (3)].str); 
      vtkParseDebug("Parsed func", (yyvsp[(1) - (3)].str));
      currentFunction->IsPureVirtual = 1; 
      data.IsAbstract = 1;
    }
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 396 "vtkParse.y"
    {postSig(" const");}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 398 "vtkParse.y"
    {postSig(" ("); }
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 400 "vtkParse.y"
    {postSig("const ");}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 402 "vtkParse.y"
    {postSig("static ");}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 404 "vtkParse.y"
    {postSig(((yyvsp[(1) - (1)].vtkid)).name);}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 404 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 413 "vtkParse.y"
    { currentFunction->NumberOfArguments++;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 414 "vtkParse.y"
    { currentFunction->NumberOfArguments++; postSig(", ");}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 417 "vtkParse.y"
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
        (yyvsp[(1) - (1)].integer);}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 422 "vtkParse.y"
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 
        (yyvsp[(2) - (2)].integer) / VTK_PARSE_COUNT_START; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
        (yyvsp[(1) - (2)].integer) + (yyvsp[(2) - (2)].integer) % VTK_PARSE_COUNT_START;
    }
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 429 "vtkParse.y"
    { 
      postSig("void (*func)(void *) ");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = VTK_PARSE_FUNCTION;
    }
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 437 "vtkParse.y"
    {delSig();}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 437 "vtkParse.y"
    {delSig();}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 439 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); }
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 447 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 448 "vtkParse.y"
    { char temp[100]; sprintf(temp,"[%i]",(yyvsp[(1) - (1)].integer)); 
                   postSig(temp); }
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 450 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_POINTER + (VTK_PARSE_COUNT_START * (yyvsp[(1) - (3)].integer)) + ((yyvsp[(3) - (3)].integer) & VTK_PARSE_UNQUALIFIED_TYPE); }
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 452 "vtkParse.y"
    { postSig("[]"); (yyval.integer) = VTK_PARSE_POINTER + ((yyvsp[(4) - (4)].integer) & VTK_PARSE_UNQUALIFIED_TYPE); }
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 454 "vtkParse.y"
    {(yyval.integer) = VTK_PARSE_CONST + (yyvsp[(2) - (2)].integer);}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 455 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 456 "vtkParse.y"
    {(yyval.integer) = VTK_PARSE_STATIC + (yyvsp[(2) - (2)].integer);}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 457 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_CONST | VTK_PARSE_STATIC) + (yyvsp[(3) - (3)].integer);}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 459 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 461 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (2)].integer) + (yyvsp[(2) - (2)].integer);}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 462 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 464 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 465 "vtkParse.y"
    { postSig("&"); (yyval.integer) = (yyvsp[(1) - (2)].integer);}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 466 "vtkParse.y"
    { postSig("*"); (yyval.integer) = (VTK_PARSE_REF + VTK_PARSE_POINTER) + (yyvsp[(1) - (2)].integer);}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 468 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 469 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 471 "vtkParse.y"
    { postSig("vtkStdString "); (yyval.integer) = (VTK_PARSE_STRING); }
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 473 "vtkParse.y"
    { postSig("vtkUnicodeString "); (yyval.integer) = (VTK_PARSE_UNICODE_STRING); }
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 482 "vtkParse.y"
    { postSig("&"); (yyval.integer) = VTK_PARSE_REF;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 483 "vtkParse.y"
    { postSig("*"); (yyval.integer) = VTK_PARSE_POINTER;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 484 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_REF + (yyvsp[(2) - (2)].integer);}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 485 "vtkParse.y"
    { (yyval.integer) = (VTK_PARSE_REF + VTK_PARSE_POINTER) + (yyvsp[(2) - (2)].integer);}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 487 "vtkParse.y"
    {postSig("unsigned ");}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 488 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_UNSIGNED + (yyvsp[(3) - (3)].integer);}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 489 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 492 "vtkParse.y"
    { postSig("vtkTypeInt8 "); (yyval.integer) = VTK_PARSE_INT8; }
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 493 "vtkParse.y"
    { postSig("vtkTypeUInt8 "); (yyval.integer) = VTK_PARSE_UINT8; }
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 494 "vtkParse.y"
    { postSig("vtkTypeInt16 "); (yyval.integer) = VTK_PARSE_INT16; }
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 495 "vtkParse.y"
    { postSig("vtkTypeUInt16 "); (yyval.integer) = VTK_PARSE_UINT16; }
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 496 "vtkParse.y"
    { postSig("vtkTypeInt32 "); (yyval.integer) = VTK_PARSE_INT32; }
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 497 "vtkParse.y"
    { postSig("vtkTypeUInt32 "); (yyval.integer) = VTK_PARSE_UINT32; }
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 498 "vtkParse.y"
    { postSig("vtkTypeInt64 "); (yyval.integer) = VTK_PARSE_INT64; }
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 499 "vtkParse.y"
    { postSig("vtkTypeUInt64 "); (yyval.integer) = VTK_PARSE_UINT64; }
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 500 "vtkParse.y"
    { postSig("vtkTypeFloat32 "); (yyval.integer) = VTK_PARSE_FLOAT32; }
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 501 "vtkParse.y"
    { postSig("vtkTypeFloat64 "); (yyval.integer) = VTK_PARSE_FLOAT64; }
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 502 "vtkParse.y"
    { postSig("float "); (yyval.integer) = VTK_PARSE_FLOAT;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 503 "vtkParse.y"
    { postSig("void "); (yyval.integer) = VTK_PARSE_VOID;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 504 "vtkParse.y"
    { postSig("char "); (yyval.integer) = VTK_PARSE_CHAR;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 505 "vtkParse.y"
    { postSig("int "); (yyval.integer) = VTK_PARSE_INT;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 506 "vtkParse.y"
    { postSig("short "); (yyval.integer) = VTK_PARSE_SHORT;}
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 507 "vtkParse.y"
    { postSig("long "); (yyval.integer) = VTK_PARSE_LONG;}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 508 "vtkParse.y"
    { postSig("double "); (yyval.integer) = VTK_PARSE_DOUBLE;}
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 509 "vtkParse.y"
    {       
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",(yyvsp[(1) - (1)].str));
      postSig(ctmpid);
      (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 515 "vtkParse.y"
    { 
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",((yyvsp[(1) - (1)].vtkid)).name);
      postSig(ctmpid);
      (yyval.integer) = VTK_PARSE_VTK_OBJECT; 
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
    }
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 534 "vtkParse.y"
    { postSig("vtkIdType "); (yyval.integer) = VTK_PARSE_ID_TYPE;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 535 "vtkParse.y"
    { postSig("long long "); (yyval.integer) = VTK_PARSE_LONG_LONG;}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 536 "vtkParse.y"
    { postSig("__int64 "); (yyval.integer) = VTK_PARSE___INT64;}
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 537 "vtkParse.y"
    { postSig("signed char "); (yyval.integer) = VTK_PARSE_SIGNED_CHAR;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 538 "vtkParse.y"
    { postSig("bool "); (yyval.integer) = VTK_PARSE_BOOL;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 543 "vtkParse.y"
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup((yyvsp[(2) - (2)].str)); 
      data.NumberOfSuperClasses++; 
    }
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 548 "vtkParse.y"
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup((yyvsp[(2) - (2)].str)); 
      data.NumberOfSuperClasses++; 
    }
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 553 "vtkParse.y"
    {in_public = 1; in_protected = 0;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 554 "vtkParse.y"
    {in_public = 0; in_protected = 0;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 555 "vtkParse.y"
    {in_public = 0; in_protected = 1;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 559 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 560 "vtkParse.y"
    {(yyval.integer) = -1;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 560 "vtkParse.y"
    {(yyval.integer) = -1;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 564 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); }
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 565 "vtkParse.y"
    {
   postSig(");");
   sprintf(temps,"Set%s",(yyvsp[(3) - (7)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = VTK_PARSE_VOID;
   output_function();
   }
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 575 "vtkParse.y"
    {postSig("Get");}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 575 "vtkParse.y"
    {postSig(" ();"); invertSig = 1;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 577 "vtkParse.y"
    { 
   sprintf(temps,"Get%s",(yyvsp[(4) - (8)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (yyvsp[(7) - (8)].integer);
   output_function();
   }
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 584 "vtkParse.y"
    {preSig("void Set");}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 585 "vtkParse.y"
    {
   postSig(" (char *);"); 
   sprintf(temps,"Set%s",(yyvsp[(4) - (5)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = VTK_PARSE_CHAR_PTR;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = VTK_PARSE_VOID;
   output_function();
   }
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 595 "vtkParse.y"
    {preSig("char *Get");}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 596 "vtkParse.y"
    { 
   postSig(" ();");
   sprintf(temps,"Get%s",(yyvsp[(4) - (5)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = VTK_PARSE_CHAR_PTR;
   output_function();
   }
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 605 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); }
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 606 "vtkParse.y"
    {postSig(");"); openSig = 0;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 607 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sscanf (currentFunction->Signature, "%*s %*s (%s);", local);
   sprintf(temps,"Set%s",(yyvsp[(3) - (10)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (yyvsp[(6) - (10)].integer);
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = VTK_PARSE_VOID;
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
   }
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 637 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); }
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 638 "vtkParse.y"
    { 
   postSig("*);");
   sprintf(temps,"Set%s",(yyvsp[(3) - (7)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = VTK_PARSE_VTK_OBJECT_PTR;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = VTK_PARSE_VOID;
   output_function();
   }
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 648 "vtkParse.y"
    {postSig("*Get");}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 649 "vtkParse.y"
    {postSig(" ();"); invertSig = 1;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 650 "vtkParse.y"
    { 
   sprintf(temps,"Get%s",(yyvsp[(4) - (8)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = VTK_PARSE_VTK_OBJECT_PTR;
   output_function();
   }
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 658 "vtkParse.y"
    {preSig("void "); postSig("On ();"); openSig = 0; }
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 660 "vtkParse.y"
    { 
   sprintf(temps,"%sOn",(yyvsp[(3) - (7)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = VTK_PARSE_VOID;
   output_function();
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void %sOff ();",(yyvsp[(3) - (7)].str)); 
   sprintf(temps,"%sOff",(yyvsp[(3) - (7)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   output_function();
   }
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 675 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 680 "vtkParse.y"
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
   currentFunction->ReturnType = VTK_PARSE_VOID;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[2]);",(yyvsp[(3) - (7)].str),
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = VTK_PARSE_POINTER + (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[0] = 2;
   output_function();
   }
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 705 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 710 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, (yyvsp[(3) - (7)].str));
   sprintf(temps,"Get%s",(yyvsp[(3) - (7)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = VTK_PARSE_POINTER + (yyvsp[(6) - (7)].integer);
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 2;
   output_function();
   }
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 722 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 727 "vtkParse.y"
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
   currentFunction->ReturnType = VTK_PARSE_VOID;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[3]);",(yyvsp[(3) - (7)].str),
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = VTK_PARSE_POINTER + (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[0] = 3;
   output_function();
   }
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 754 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 759 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, (yyvsp[(3) - (7)].str));
   sprintf(temps,"Get%s",(yyvsp[(3) - (7)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = VTK_PARSE_POINTER + (yyvsp[(6) - (7)].integer);
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 3;
   output_function();
   }
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 771 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 776 "vtkParse.y"
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
   currentFunction->ReturnType = VTK_PARSE_VOID;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[4]);",(yyvsp[(3) - (7)].str),
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = VTK_PARSE_POINTER + (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[0] = 4;
   output_function();
   }
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 805 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 810 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, (yyvsp[(3) - (7)].str));
   sprintf(temps,"Get%s",(yyvsp[(3) - (7)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = VTK_PARSE_POINTER + (yyvsp[(6) - (7)].integer);
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 4;
   output_function();
   }
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 822 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 827 "vtkParse.y"
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
   currentFunction->ReturnType = VTK_PARSE_VOID;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[6]);",(yyvsp[(3) - (7)].str),
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = VTK_PARSE_POINTER + (yyvsp[(6) - (7)].integer);
   currentFunction->ArgCounts[0] = 6;
   output_function();
   }
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 860 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 865 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, (yyvsp[(3) - (7)].str));
   sprintf(temps,"Get%s",(yyvsp[(3) - (7)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = VTK_PARSE_POINTER + (yyvsp[(6) - (7)].integer);
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 6;
   output_function();
   }
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 877 "vtkParse.y"
    {
      free (currentFunction->Signature);
      currentFunction->Signature = NULL;
      }
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 882 "vtkParse.y"
    {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s [%i]);",(yyvsp[(3) - (9)].str),
      local, (yyvsp[(8) - (9)].integer));
     sprintf(temps,"Set%s",(yyvsp[(3) - (9)].str)); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->ReturnType = VTK_PARSE_VOID;
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = VTK_PARSE_POINTER + (yyvsp[(6) - (9)].integer);
     currentFunction->ArgCounts[0] = (yyvsp[(8) - (9)].integer);
     output_function();
   }
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 895 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 900 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, (yyvsp[(3) - (9)].str));
   sprintf(temps,"Get%s",(yyvsp[(3) - (9)].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = VTK_PARSE_POINTER + (yyvsp[(6) - (9)].integer);
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = (yyvsp[(8) - (9)].integer);
   output_function();
   }
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 912 "vtkParse.y"
    { 
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       (yyvsp[(3) - (4)].str));

     sprintf(temps,"Get%sCoordinate",(yyvsp[(3) - (4)].str)); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = VTK_PARSE_VTK_OBJECT_PTR;
     currentFunction->ReturnClass = vtkstrdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double, double);",
       (yyvsp[(3) - (4)].str));
     sprintf(temps,"Set%s",(yyvsp[(3) - (4)].str)); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 2;
     currentFunction->ArgTypes[0] = VTK_PARSE_DOUBLE;
     currentFunction->ArgCounts[0] = 0;
     currentFunction->ArgTypes[1] = VTK_PARSE_DOUBLE;
     currentFunction->ArgCounts[1] = 0;
     currentFunction->ReturnType = VTK_PARSE_VOID;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double a[2]);",
       (yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = VTK_PARSE_DOUBLE_PTR;
     currentFunction->ArgCounts[0] = 2;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s ();", (yyvsp[(3) - (4)].str));
     sprintf(temps,"Get%s",(yyvsp[(3) - (4)].str)); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = VTK_PARSE_DOUBLE_PTR;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 2;
     output_function();
   }
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 959 "vtkParse.y"
    { 
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       (yyvsp[(3) - (4)].str));

     sprintf(temps,"Get%sCoordinate",(yyvsp[(3) - (4)].str)); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = VTK_PARSE_VTK_OBJECT_PTR;
     currentFunction->ReturnClass = vtkstrdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double, double, double);",
       (yyvsp[(3) - (4)].str));
     sprintf(temps,"Set%s",(yyvsp[(3) - (4)].str)); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 3;
     currentFunction->ArgTypes[0] = VTK_PARSE_DOUBLE;
     currentFunction->ArgCounts[0] = 0;
     currentFunction->ArgTypes[1] = VTK_PARSE_DOUBLE;
     currentFunction->ArgCounts[1] = 0;
     currentFunction->ArgTypes[2] = VTK_PARSE_DOUBLE;
     currentFunction->ArgCounts[2] = 0;
     currentFunction->ReturnType = VTK_PARSE_VOID;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double a[3]);",
       (yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = VTK_PARSE_DOUBLE_PTR;
     currentFunction->ArgCounts[0] = 3;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s ();", (yyvsp[(3) - (4)].str));
     sprintf(temps,"Get%s",(yyvsp[(3) - (4)].str)); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = VTK_PARSE_DOUBLE_PTR;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 3;
     output_function();
   }
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 1008 "vtkParse.y"
    { 
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "const char *GetClassName ();");
   sprintf(temps,"GetClassName"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR);
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,
           "int IsA (const char *name);");
   sprintf(temps,"IsA"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR);
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = VTK_PARSE_INT;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "%s *NewInstance ();",
           (yyvsp[(3) - (6)].str));
   sprintf(temps,"NewInstance"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = VTK_PARSE_VTK_OBJECT_PTR;
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
     currentFunction->ArgTypes[0] = VTK_PARSE_VTK_OBJECT_PTR;
     currentFunction->ArgCounts[0] = 1;
     currentFunction->ArgClasses[0] = vtkstrdup("vtkObject");
     currentFunction->ReturnType = (VTK_PARSE_STATIC | VTK_PARSE_VTK_OBJECT_PTR);
     currentFunction->ReturnClass = vtkstrdup((yyvsp[(3) - (6)].str));
     output_function();
     }
   }
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1059 "vtkParse.y"
    { 
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "const char *GetClassName ();");
   sprintf(temps,"GetClassName"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR);
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,
           "int IsA (const char *name);");
   sprintf(temps,"IsA"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR);
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = VTK_PARSE_INT;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "%s *NewInstance ();",
           (yyvsp[(3) - (7)].str));
   sprintf(temps,"NewInstance"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = VTK_PARSE_VTK_OBJECT_PTR;
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
     currentFunction->ArgTypes[0] = VTK_PARSE_VTK_OBJECT_PTR;
     currentFunction->ArgCounts[0] = 1;
     currentFunction->ArgClasses[0] = vtkstrdup("vtkObject");
     currentFunction->ReturnType = (VTK_PARSE_STATIC | VTK_PARSE_VTK_OBJECT_PTR);
     currentFunction->ReturnClass = vtkstrdup((yyvsp[(3) - (7)].str));
     output_function();
     }
   }
    break;



/* Line 1455 of yacc.c  */
#line 3647 "vtkParse.tab.c"
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
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

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
#line 1127 "vtkParse.y"

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
  func->ReturnType = VTK_PARSE_VOID;
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
  if ((currentFunction->ArgTypes[0] & VTK_PARSE_UNQUALIFIED_TYPE) ==
      VTK_PARSE_VOID) 
    {
    currentFunction->NumberOfArguments = 0;
    }

  currentFunction->IsPublic = in_public;
  currentFunction->IsProtected = in_protected;
  
  /* look for VAR FUNCTIONS */
  if (currentFunction->NumberOfArguments
      && (currentFunction->ArgTypes[0] == VTK_PARSE_FUNCTION))
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
    switch (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE)
      {
      case VTK_PARSE_FLOAT_PTR:
      case VTK_PARSE_VOID_PTR:
      case VTK_PARSE_DOUBLE_PTR:
      case VTK_PARSE_ID_TYPE_PTR:
      case VTK_PARSE_LONG_LONG_PTR:
      case VTK_PARSE___INT64_PTR:
      case VTK_PARSE_INT_PTR:
      case VTK_PARSE_SHORT_PTR:
      case VTK_PARSE_LONG_PTR:
      case VTK_PARSE_UNSIGNED_CHAR_PTR:
        look_for_hint();
        break;
      }
    }

  /* reject multi-dimensional arrays from wrappers */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    if ((currentFunction->ArgTypes[i] & VTK_PARSE_INDIRECT) == 2*VTK_PARSE_POINTER ||
        (currentFunction->ArgTypes[i] & VTK_PARSE_INDIRECT) == 3*VTK_PARSE_POINTER)
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



