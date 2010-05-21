
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
   VTK_PARSE_STRING,             /* VTK_STRING             13 */
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
void start_class(const char *classname);
void output_function(void);
void reject_function(void);

/* vtkstrdup is not part of POSIX so we create our own */
char *vtkstrdup(const char *in)
{
  char *res = malloc(strlen(in)+1);
  strcpy(res,in);
  return res;
}

#include "vtkParse.h"

  FileInfo data;
  FunctionInfo throwAwayFunction;
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
  int mainClass;

#define YYMAXDEPTH 1000

  void checkSigSize(const char *arg)
    {
    if (strlen(currentFunction->Signature) + strlen(arg) + 5 >
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
      strcpy(currentFunction->Signature, arg);
      }
    else if (openSig)
      {
      int m, n;
      char *cp;
      checkSigSize(arg);
      cp = currentFunction->Signature;
      m = strlen(cp);
      n = strlen(arg);
      memmove(&cp[n], cp, m+1);
      strncpy(cp, arg, n);
      }
    }
  void postSig(const char *arg)
    {
    if (!currentFunction->Signature)
      {
      currentFunction->Signature = (char*)malloc(2048);
      sigAllocatedLength = 2048;
      strcpy(currentFunction->Signature, arg);
      }
    else if (openSig)
      {
      int m, n;
      char *cp;
      checkSigSize(arg);
      cp = currentFunction->Signature;
      m = strlen(cp);
      n = strlen(arg);
      if (invertSig)
        {
        memmove(&cp[n], cp, m+1);
        strncpy(cp, arg, n);
        }
      else
        {
        strncpy(&cp[m], arg, n+1);
        }
      }
    }
  void preScopeSig(const char *arg)
    {
    if (!currentFunction->Signature)
      {
      currentFunction->Signature = (char*)malloc(2048);
      sigAllocatedLength = 2048;
      strcpy(currentFunction->Signature, arg);
      }
    else if (openSig)
      {
      int i, m, n;
      char *cp;
      checkSigSize(arg);
      cp = currentFunction->Signature;
      m = strlen(cp);
      n = strlen(arg);
      i = m;
      while (i > 0 &&
            ((cp[i-1] >= 'a' && cp[i-1] <= 'z') ||
             (cp[i-1] >= 'A' && cp[i-1] <= 'Z') ||
             (cp[i-1] >= '0' && cp[i-1] <= '9') ||
             cp[i-1] == '_' || cp[i-1] == ':'))
        {
        i--;
        }
      memmove(&cp[i+n+2], &cp[i], m+1);
      strncpy(&cp[i], arg, n);
      strncpy(&cp[i+n], "::", 2);
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
#line 302 "vtkParse.tab.c"

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
     ID = 263,
     STRING_LITERAL = 264,
     INT_LITERAL = 265,
     HEX_LITERAL = 266,
     FLOAT_LITERAL = 267,
     CHAR_LITERAL = 268,
     INT = 269,
     FLOAT = 270,
     SHORT = 271,
     LONG = 272,
     LONG_LONG = 273,
     INT64__ = 274,
     DOUBLE = 275,
     VOID = 276,
     CHAR = 277,
     SIGNED_CHAR = 278,
     BOOL = 279,
     OSTREAM = 280,
     ISTREAM = 281,
     ENUM = 282,
     UNION = 283,
     CLASS_REF = 284,
     OTHER = 285,
     CONST = 286,
     CONST_PTR = 287,
     CONST_REF = 288,
     CONST_EQUAL = 289,
     OPERATOR = 290,
     UNSIGNED = 291,
     FRIEND = 292,
     INLINE = 293,
     MUTABLE = 294,
     TEMPLATE = 295,
     TYPENAME = 296,
     TYPEDEF = 297,
     NAMESPACE = 298,
     USING = 299,
     VTK_ID = 300,
     STATIC = 301,
     VAR_FUNCTION = 302,
     ARRAY_NUM = 303,
     VTK_LEGACY = 304,
     NEW = 305,
     DELETE = 306,
     LPAREN_POINTER = 307,
     LPAREN_AMPERSAND = 308,
     OP_LSHIFT_EQ = 309,
     OP_RSHIFT_EQ = 310,
     OP_LSHIFT = 311,
     OP_RSHIFT = 312,
     OP_ARROW_POINTER = 313,
     OP_ARROW = 314,
     OP_INCR = 315,
     OP_DECR = 316,
     OP_PLUS_EQ = 317,
     OP_MINUS_EQ = 318,
     OP_TIMES_EQ = 319,
     OP_DIVIDE_EQ = 320,
     OP_REMAINDER_EQ = 321,
     OP_AND_EQ = 322,
     OP_OR_EQ = 323,
     OP_XOR_EQ = 324,
     OP_LOGIC_AND_EQ = 325,
     OP_LOGIC_OR_EQ = 326,
     OP_LOGIC_AND = 327,
     OP_LOGIC_OR = 328,
     OP_LOGIC_EQ = 329,
     OP_LOGIC_NEQ = 330,
     OP_LOGIC_LEQ = 331,
     OP_LOGIC_GEQ = 332,
     ELLIPSIS = 333,
     DOUBLE_COLON = 334,
     TypeInt8 = 335,
     TypeUInt8 = 336,
     TypeInt16 = 337,
     TypeUInt16 = 338,
     TypeInt32 = 339,
     TypeUInt32 = 340,
     TypeInt64 = 341,
     TypeUInt64 = 342,
     TypeFloat32 = 343,
     TypeFloat64 = 344,
     IdType = 345,
     StdString = 346,
     UnicodeString = 347,
     SetMacro = 348,
     GetMacro = 349,
     SetStringMacro = 350,
     GetStringMacro = 351,
     SetClampMacro = 352,
     SetObjectMacro = 353,
     GetObjectMacro = 354,
     BooleanMacro = 355,
     SetVector2Macro = 356,
     SetVector3Macro = 357,
     SetVector4Macro = 358,
     SetVector6Macro = 359,
     GetVector2Macro = 360,
     GetVector3Macro = 361,
     GetVector4Macro = 362,
     GetVector6Macro = 363,
     SetVectorMacro = 364,
     GetVectorMacro = 365,
     ViewportCoordinateMacro = 366,
     WorldCoordinateMacro = 367,
     TypeMacro = 368,
     VTK_CONSTANT_DEF = 369,
     VTK_BYTE_SWAP_DECL = 370
   };
#endif




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 243 "vtkParse.y"

  char *str;
  int   integer;



/* Line 214 of yacc.c  */
#line 575 "vtkParse.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 587 "vtkParse.tab.c"

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
#   define YYCOPY(To, From, Count)        \
      do                    \
    {                    \
      YYSIZE_T yyi;                \
      for (yyi = 0; yyi < (Count); yyi++)    \
        (To)[yyi] = (From)[yyi];        \
    }                    \
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                \
    do                                    \
      {                                    \
    YYSIZE_T yynewbytes;                        \
    YYCOPY (&yyptr->Stack_alloc, Stack, yysize);            \
    Stack = &yyptr->Stack_alloc;                    \
    yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
    yyptr += yynewbytes / sizeof (*yyptr);                \
      }                                    \
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  133
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2499

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  139
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  152
/* YYNRULES -- Number of rules.  */
#define YYNRULES  451
/* YYNRULES -- Number of states.  */
#define YYNSTATES  810

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   370

#define YYTRANSLATE(YYX)                        \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   137,     2,     2,     2,   131,   132,     2,
     121,   122,   129,   127,   124,   126,   138,   130,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   120,   123,
     118,   125,   119,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   135,     2,   136,   134,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   116,   133,   117,   128,     2,     2,     2,
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
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     7,    11,    14,    18,    21,    23,
      24,    32,    33,    44,    45,    48,    51,    53,    56,    58,
      60,    62,    64,    66,    68,    70,    73,    75,    77,    80,
      84,    88,    91,    95,    98,   102,   106,   109,   113,   116,
     122,   125,   127,   135,   142,   143,   145,   149,   151,   155,
     157,   159,   161,   163,   165,   167,   169,   171,   174,   178,
     182,   184,   186,   188,   190,   192,   194,   196,   198,   200,
     202,   204,   212,   219,   223,   226,   230,   232,   237,   241,
     246,   249,   258,   266,   276,   286,   289,   292,   295,   298,
     302,   305,   308,   312,   317,   319,   323,   326,   329,   331,
     333,   335,   340,   343,   347,   349,   352,   356,   361,   365,
     367,   370,   374,   379,   383,   384,   385,   386,   396,   397,
     398,   403,   406,   407,   408,   416,   417,   418,   423,   426,
     429,   432,   433,   435,   436,   442,   443,   444,   449,   450,
     456,   457,   461,   462,   466,   471,   472,   475,   476,   482,
     484,   486,   489,   491,   493,   495,   500,   504,   505,   507,
     509,   510,   515,   516,   518,   520,   522,   523,   528,   531,
     532,   537,   539,   540,   546,   547,   548,   558,   559,   561,
     562,   564,   567,   572,   578,   583,   589,   594,   600,   604,
     609,   612,   616,   622,   628,   635,   644,   653,   655,   659,
     661,   665,   668,   673,   676,   679,   680,   682,   683,   687,
     692,   695,   697,   700,   704,   706,   709,   711,   712,   716,
     718,   719,   723,   726,   727,   732,   733,   739,   740,   746,
     748,   749,   754,   756,   758,   760,   762,   766,   770,   774,
     776,   778,   781,   784,   787,   790,   792,   794,   795,   800,
     801,   806,   807,   811,   812,   816,   818,   820,   822,   824,
     826,   828,   830,   832,   834,   836,   838,   840,   842,   844,
     846,   848,   850,   852,   854,   856,   858,   860,   861,   865,
     867,   869,   871,   873,   875,   877,   879,   881,   882,   885,
     888,   889,   895,   896,   898,   900,   902,   904,   907,   910,
     912,   916,   918,   920,   922,   924,   926,   928,   929,   937,
     938,   939,   948,   949,   955,   956,   962,   963,   964,   975,
     976,   984,   985,   986,   995,   996,  1004,  1005,  1013,  1014,
    1022,  1023,  1031,  1032,  1040,  1041,  1049,  1050,  1058,  1059,
    1067,  1068,  1076,  1077,  1087,  1088,  1098,  1103,  1108,  1115,
    1123,  1126,  1129,  1133,  1137,  1139,  1141,  1143,  1145,  1147,
    1149,  1151,  1153,  1155,  1157,  1159,  1161,  1163,  1165,  1167,
    1169,  1171,  1173,  1175,  1177,  1179,  1181,  1183,  1185,  1187,
    1189,  1191,  1193,  1195,  1197,  1199,  1201,  1203,  1205,  1207,
    1209,  1211,  1213,  1215,  1217,  1219,  1221,  1222,  1225,  1226,
    1229,  1230,  1233,  1235,  1237,  1239,  1241,  1243,  1245,  1247,
    1249,  1251,  1253,  1255,  1257,  1259,  1261,  1263,  1265,  1267,
    1269,  1271,  1273,  1275,  1277,  1279,  1281,  1283,  1285,  1287,
    1289,  1291,  1293,  1295,  1297,  1299,  1301,  1303,  1305,  1307,
    1309,  1311,  1313,  1315,  1319,  1323,  1327,  1331,  1335,  1339,
    1340,  1343
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     140,     0,    -1,   280,   141,    -1,    -1,   142,   280,   141,
      -1,   166,   245,    -1,   166,    38,   245,    -1,   166,   143,
      -1,   143,    -1,    -1,     3,   204,   144,   249,   116,   146,
     117,    -1,    -1,     3,   204,   118,   236,   119,   145,   249,
     116,   146,   117,    -1,    -1,   147,   146,    -1,   252,   120,
      -1,   220,    -1,    39,   220,    -1,   149,    -1,   148,    -1,
     158,    -1,   157,    -1,   159,    -1,   163,    -1,   161,    -1,
      37,   161,    -1,   160,    -1,    29,    -1,   172,   205,    -1,
      37,   172,   205,    -1,    38,   172,   205,    -1,   165,   205,
      -1,    38,   165,   205,    -1,   171,   205,    -1,    37,   171,
     205,    -1,    38,   171,   205,    -1,   164,   205,    -1,    38,
     164,   205,    -1,   170,   205,    -1,   115,   121,   280,   122,
     123,    -1,   255,   123,    -1,   255,    -1,    27,   204,   116,
     150,   117,   281,   123,    -1,    27,   116,   150,   117,   281,
     123,    -1,    -1,   151,    -1,   151,   124,   150,    -1,   204,
      -1,   204,   125,   154,    -1,   153,    -1,   204,    -1,   239,
      -1,   233,    -1,    10,    -1,    11,    -1,    13,    -1,   152,
      -1,   155,   154,    -1,   152,   156,   154,    -1,   121,   154,
     122,    -1,   126,    -1,   127,    -1,   128,    -1,   126,    -1,
     127,    -1,   129,    -1,   130,    -1,   131,    -1,   132,    -1,
     133,    -1,   134,    -1,    28,   204,   116,   280,   117,   281,
     123,    -1,    28,   116,   280,   117,   281,   123,    -1,    44,
     281,   123,    -1,   166,   161,    -1,     3,   204,   162,    -1,
     123,    -1,   116,   280,   117,   123,    -1,   120,   281,   123,
      -1,    42,   228,   224,   123,    -1,    42,    29,    -1,    42,
       3,   204,   116,   280,   117,   204,   123,    -1,    42,     3,
     116,   280,   117,   204,   123,    -1,    42,   228,    52,   204,
     122,   121,   281,   122,   123,    -1,    42,   228,    53,   204,
     122,   121,   281,   122,   123,    -1,    42,   149,    -1,    42,
     148,    -1,    42,   158,    -1,    42,   157,    -1,    42,    47,
     123,    -1,   166,   171,    -1,   166,   172,    -1,    40,   118,
     119,    -1,    40,   118,   167,   119,    -1,   168,    -1,   168,
     124,   167,    -1,   169,   217,    -1,   166,   217,    -1,    41,
      -1,     3,    -1,    14,    -1,    49,   121,   171,   122,    -1,
     128,   198,    -1,     7,   128,   198,    -1,   190,    -1,   228,
     183,    -1,   228,    31,   183,    -1,     7,   228,    31,   183,
      -1,     7,   228,   183,    -1,   173,    -1,   228,   177,    -1,
     228,    31,   177,    -1,     7,   228,    31,   177,    -1,     7,
     228,   177,    -1,    -1,    -1,    -1,    35,   228,   121,   174,
     209,   122,   175,   187,   176,    -1,    -1,    -1,   180,   178,
     187,   179,    -1,   180,   186,    -1,    -1,    -1,    35,   277,
     181,   121,   182,   209,   122,    -1,    -1,    -1,   188,   184,
     187,   185,    -1,   188,   186,    -1,   125,    10,    -1,    34,
      10,    -1,    -1,    31,    -1,    -1,   204,   121,   189,   209,
     122,    -1,    -1,    -1,   193,   191,   195,   192,    -1,    -1,
     204,   121,   194,   209,   122,    -1,    -1,   120,   197,   196,
      -1,    -1,   124,   197,   196,    -1,   204,   121,   280,   122,
      -1,    -1,   200,   199,    -1,    -1,   204,   121,   201,   209,
     122,    -1,    31,    -1,    46,    -1,    46,    38,    -1,    45,
      -1,     8,    -1,   123,    -1,   116,   280,   117,   123,    -1,
     116,   280,   117,    -1,    -1,   207,    -1,   212,    -1,    -1,
     212,   208,   124,   207,    -1,    -1,   210,    -1,    78,    -1,
     212,    -1,    -1,   212,   211,   124,   210,    -1,   228,   225,
      -1,    -1,   228,   224,   213,   218,    -1,    47,    -1,    -1,
     228,    53,   214,   217,   122,    -1,    -1,    -1,   228,    52,
     215,   217,   122,   121,   216,   206,   122,    -1,    -1,   204,
      -1,    -1,   219,    -1,   125,   253,    -1,     3,   204,   221,
     123,    -1,     3,   204,   240,   221,   123,    -1,    27,   204,
     221,   123,    -1,    27,   204,   240,   221,   123,    -1,    28,
     204,   221,   123,    -1,    28,   204,   240,   221,   123,    -1,
     228,   221,   123,    -1,   228,    31,   221,   123,    -1,    47,
     123,    -1,    46,    47,   123,    -1,   228,    53,   204,   122,
     123,    -1,   228,    52,   204,   122,   123,    -1,   228,    52,
     204,   122,    48,   123,    -1,   228,    52,   204,   122,   135,
     280,   136,   123,    -1,   228,    52,   204,   122,   121,   206,
     122,   123,    -1,   223,    -1,   223,   124,   222,    -1,   223,
      -1,   223,   124,   222,    -1,   240,   223,    -1,   240,   223,
     124,   222,    -1,   224,   218,    -1,   204,   225,    -1,    -1,
     226,    -1,    -1,    48,   227,   225,    -1,   135,   281,   136,
     225,    -1,   202,   229,    -1,   229,    -1,   203,   229,    -1,
     203,   202,   229,    -1,   245,    -1,   245,   240,    -1,   233,
      -1,    -1,   233,   230,   240,    -1,   239,    -1,    -1,   239,
     231,   240,    -1,    41,   239,    -1,    -1,    41,   239,   232,
     240,    -1,    -1,    45,   118,   234,   236,   119,    -1,    -1,
       8,   118,   235,   236,   119,    -1,   228,    -1,    -1,   228,
     124,   237,   236,    -1,     8,    -1,    45,    -1,   233,    -1,
     239,    -1,     8,    79,   238,    -1,    45,    79,   238,    -1,
     233,    79,   238,    -1,   132,    -1,   129,    -1,   129,   132,
      -1,   129,   129,    -1,   129,    33,    -1,   129,    32,    -1,
      33,    -1,    32,    -1,    -1,   129,   129,   241,   240,    -1,
      -1,   129,   132,   242,   240,    -1,    -1,    33,   243,   240,
      -1,    -1,    32,   244,   240,    -1,   246,    -1,    91,    -1,
      92,    -1,    25,    -1,    26,    -1,     8,    -1,    45,    -1,
      21,    -1,    15,    -1,    20,    -1,    24,    -1,    23,    -1,
      80,    -1,    81,    -1,    82,    -1,    83,    -1,    84,    -1,
      85,    -1,    86,    -1,    87,    -1,    88,    -1,    89,    -1,
      -1,    36,   247,   248,    -1,   248,    -1,    22,    -1,    14,
      -1,    16,    -1,    17,    -1,    90,    -1,    18,    -1,    19,
      -1,    -1,   120,   250,    -1,   252,   238,    -1,    -1,   252,
     238,   251,   124,   250,    -1,    -1,     4,    -1,     5,    -1,
       6,    -1,   254,    -1,   127,   254,    -1,   126,   254,    -1,
       9,    -1,   121,   253,   122,    -1,    10,    -1,    11,    -1,
      12,    -1,    13,    -1,     8,    -1,    45,    -1,    -1,    93,
     121,   204,   124,   256,   229,   122,    -1,    -1,    -1,    94,
     121,   257,   204,   124,   258,   229,   122,    -1,    -1,    95,
     121,   259,   204,   122,    -1,    -1,    96,   121,   260,   204,
     122,    -1,    -1,    -1,    97,   121,   204,   124,   261,   245,
     262,   124,   281,   122,    -1,    -1,    98,   121,   204,   124,
     263,   245,   122,    -1,    -1,    -1,    99,   121,   264,   204,
     124,   265,   245,   122,    -1,    -1,   100,   121,   204,   266,
     124,   245,   122,    -1,    -1,   101,   121,   204,   124,   267,
     245,   122,    -1,    -1,   105,   121,   204,   124,   268,   245,
     122,    -1,    -1,   102,   121,   204,   124,   269,   245,   122,
      -1,    -1,   106,   121,   204,   124,   270,   245,   122,    -1,
      -1,   103,   121,   204,   124,   271,   245,   122,    -1,    -1,
     107,   121,   204,   124,   272,   245,   122,    -1,    -1,   104,
     121,   204,   124,   273,   245,   122,    -1,    -1,   108,   121,
     204,   124,   274,   245,   122,    -1,    -1,   109,   121,   204,
     124,   275,   245,   124,    10,   122,    -1,    -1,   110,   121,
     204,   124,   276,   245,   124,    10,   122,    -1,   111,   121,
     204,   122,    -1,   112,   121,   204,   122,    -1,   113,   121,
     204,   124,   204,   122,    -1,   113,   121,   204,   124,   204,
     124,   122,    -1,   121,   122,    -1,   135,   136,    -1,    50,
     135,   136,    -1,    51,   135,   136,    -1,   278,    -1,   125,
      -1,   129,    -1,   130,    -1,   126,    -1,   127,    -1,   137,
      -1,   128,    -1,   124,    -1,   118,    -1,   119,    -1,   132,
      -1,   133,    -1,   134,    -1,   131,    -1,    50,    -1,    51,
      -1,    54,    -1,    55,    -1,    56,    -1,    57,    -1,    58,
      -1,    59,    -1,    62,    -1,    63,    -1,    64,    -1,    65,
      -1,    66,    -1,    60,    -1,    61,    -1,    67,    -1,    68,
      -1,    69,    -1,    70,    -1,    71,    -1,    72,    -1,    73,
      -1,    74,    -1,    75,    -1,    76,    -1,    77,    -1,   114,
      -1,    -1,   284,   280,    -1,    -1,   285,   281,    -1,    -1,
     283,   282,    -1,     3,    -1,    40,    -1,   284,    -1,   123,
      -1,   285,    -1,   289,    -1,    30,    -1,   286,    -1,   287,
      -1,   288,    -1,   278,    -1,   120,    -1,   138,    -1,   245,
      -1,    79,    -1,    10,    -1,    11,    -1,    12,    -1,    13,
      -1,     9,    -1,    29,    -1,    31,    -1,    32,    -1,    33,
      -1,    34,    -1,    35,    -1,    46,    -1,    38,    -1,     7,
      -1,    27,    -1,    28,    -1,    41,    -1,    48,    -1,    47,
      -1,    78,    -1,     4,    -1,     6,    -1,     5,    -1,    43,
      -1,    44,    -1,   279,    -1,   116,   282,   117,    -1,   121,
     280,   122,    -1,    52,   280,   122,    -1,    53,   280,   122,
      -1,   135,   280,   136,    -1,    42,   290,   123,    -1,    -1,
       3,   290,    -1,   285,   290,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   369,   369,   371,   371,   373,   373,   374,   374,   377,
     376,   382,   381,   387,   387,   389,   390,   391,   392,   393,
     394,   395,   396,   397,   398,   399,   400,   401,   402,   403,
     404,   405,   406,   407,   408,   409,   410,   411,   412,   413,
     414,   415,   423,   425,   427,   427,   427,   429,   429,   433,
     433,   433,   433,   435,   435,   435,   437,   438,   443,   449,
     455,   455,   456,   458,   458,   459,   459,   460,   460,   461,
     461,   463,   465,   467,   469,   471,   473,   474,   475,   477,
     478,   479,   480,   481,   482,   483,   484,   485,   486,   487,
     489,   491,   493,   493,   495,   495,   497,   497,   499,   499,
     499,   501,   503,   504,   505,   506,   510,   514,   519,   526,
     530,   534,   538,   543,   549,   550,   551,   549,   561,   561,
     561,   567,   578,   579,   578,   585,   585,   585,   591,   602,
     603,   605,   605,   607,   607,   609,   610,   609,   617,   617,
     619,   619,   621,   621,   623,   625,   625,   634,   634,   636,
     638,   639,   641,   641,   643,   644,   645,   647,   647,   649,
     649,   649,   651,   651,   653,   653,   654,   654,   656,   664,
     663,   670,   676,   676,   682,   683,   682,   690,   690,   692,
     692,   694,   696,   697,   698,   699,   700,   701,   702,   703,
     704,   705,   706,   707,   708,   709,   710,   712,   713,   715,
     716,   717,   718,   721,   723,   731,   731,   734,   734,   740,
     745,   746,   747,   749,   752,   753,   755,   756,   756,   758,
     759,   759,   761,   762,   762,   765,   765,   767,   767,   770,
     770,   770,   772,   773,   774,   775,   777,   783,   789,   805,
     806,   807,   808,   809,   811,   813,   814,   815,   815,   817,
     817,   819,   819,   821,   821,   824,   825,   826,   828,   829,
     830,   836,   851,   852,   853,   854,   855,   856,   857,   858,
     859,   860,   861,   862,   863,   864,   865,   866,   866,   868,
     871,   872,   873,   874,   875,   876,   877,   879,   879,   881,
     890,   889,   898,   899,   900,   901,   903,   904,   905,   907,
     908,   910,   911,   912,   913,   914,   915,   919,   918,   930,
     930,   930,   939,   939,   950,   950,   960,   961,   959,   992,
     991,  1003,  1004,  1003,  1013,  1012,  1030,  1029,  1060,  1059,
    1077,  1076,  1109,  1108,  1126,  1125,  1160,  1159,  1177,  1176,
    1215,  1214,  1232,  1231,  1250,  1249,  1266,  1313,  1362,  1413,
    1470,  1471,  1472,  1473,  1474,  1476,  1477,  1477,  1478,  1478,
    1479,  1479,  1480,  1480,  1481,  1481,  1482,  1482,  1483,  1484,
    1485,  1486,  1487,  1488,  1489,  1490,  1491,  1492,  1493,  1494,
    1495,  1496,  1497,  1498,  1499,  1500,  1501,  1502,  1503,  1504,
    1505,  1506,  1507,  1508,  1509,  1515,  1520,  1520,  1521,  1521,
    1522,  1522,  1524,  1524,  1524,  1526,  1526,  1526,  1528,  1528,
    1528,  1528,  1529,  1529,  1529,  1529,  1529,  1530,  1530,  1530,
    1530,  1531,  1531,  1531,  1531,  1531,  1531,  1532,  1532,  1532,
    1532,  1532,  1532,  1532,  1533,  1533,  1533,  1533,  1533,  1533,
    1534,  1534,  1534,  1536,  1537,  1538,  1539,  1540,  1541,  1543,
    1543,  1544
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "CLASS", "PUBLIC", "PRIVATE",
  "PROTECTED", "VIRTUAL", "ID", "STRING_LITERAL", "INT_LITERAL",
  "HEX_LITERAL", "FLOAT_LITERAL", "CHAR_LITERAL", "INT", "FLOAT", "SHORT",
  "LONG", "LONG_LONG", "INT64__", "DOUBLE", "VOID", "CHAR", "SIGNED_CHAR",
  "BOOL", "OSTREAM", "ISTREAM", "ENUM", "UNION", "CLASS_REF", "OTHER",
  "CONST", "CONST_PTR", "CONST_REF", "CONST_EQUAL", "OPERATOR", "UNSIGNED",
  "FRIEND", "INLINE", "MUTABLE", "TEMPLATE", "TYPENAME", "TYPEDEF",
  "NAMESPACE", "USING", "VTK_ID", "STATIC", "VAR_FUNCTION", "ARRAY_NUM",
  "VTK_LEGACY", "NEW", "DELETE", "LPAREN_POINTER", "LPAREN_AMPERSAND",
  "OP_LSHIFT_EQ", "OP_RSHIFT_EQ", "OP_LSHIFT", "OP_RSHIFT",
  "OP_ARROW_POINTER", "OP_ARROW", "OP_INCR", "OP_DECR", "OP_PLUS_EQ",
  "OP_MINUS_EQ", "OP_TIMES_EQ", "OP_DIVIDE_EQ", "OP_REMAINDER_EQ",
  "OP_AND_EQ", "OP_OR_EQ", "OP_XOR_EQ", "OP_LOGIC_AND_EQ",
  "OP_LOGIC_OR_EQ", "OP_LOGIC_AND", "OP_LOGIC_OR", "OP_LOGIC_EQ",
  "OP_LOGIC_NEQ", "OP_LOGIC_LEQ", "OP_LOGIC_GEQ", "ELLIPSIS",
  "DOUBLE_COLON", "TypeInt8", "TypeUInt8", "TypeInt16", "TypeUInt16",
  "TypeInt32", "TypeUInt32", "TypeInt64", "TypeUInt64", "TypeFloat32",
  "TypeFloat64", "IdType", "StdString", "UnicodeString", "SetMacro",
  "GetMacro", "SetStringMacro", "GetStringMacro", "SetClampMacro",
  "SetObjectMacro", "GetObjectMacro", "BooleanMacro", "SetVector2Macro",
  "SetVector3Macro", "SetVector4Macro", "SetVector6Macro",
  "GetVector2Macro", "GetVector3Macro", "GetVector4Macro",
  "GetVector6Macro", "SetVectorMacro", "GetVectorMacro",
  "ViewportCoordinateMacro", "WorldCoordinateMacro", "TypeMacro",
  "VTK_CONSTANT_DEF", "VTK_BYTE_SWAP_DECL", "'{'", "'}'", "'<'", "'>'",
  "':'", "'('", "')'", "';'", "','", "'='", "'-'", "'+'", "'~'", "'*'",
  "'/'", "'%'", "'&'", "'|'", "'^'", "'['", "']'", "'!'", "'.'", "$accept",
  "strt", "maybe_classes", "maybe_template_class_def", "class_def", "$@1",
  "$@2", "class_def_body", "class_def_item", "named_enum", "enum",
  "enum_list", "enum_item", "enum_value", "enum_literal", "enum_math",
  "math_unary_op", "math_binary_op", "named_union", "union", "using",
  "template_internal_class", "internal_class", "internal_class_body",
  "typedef", "template_function", "template_operator", "template",
  "template_args", "template_arg", "template_type", "legacy_function",
  "function", "operator", "typecast_op_func", "$@3", "$@4", "$@5",
  "op_func", "@6", "$@7", "op_sig", "$@8", "$@9", "func", "$@10", "$@11",
  "pure_virtual", "maybe_const", "func_sig", "$@12", "constructor", "$@13",
  "$@14", "constructor_sig", "$@15", "maybe_initializers",
  "more_initializers", "initializer", "destructor", "$@16",
  "destructor_sig", "$@17", "const_mod", "static_mod", "any_id",
  "func_body", "ignore_args_list", "ignore_more_args", "$@18", "args_list",
  "more_args", "$@19", "arg", "$@20", "$@21", "$@22", "$@23", "maybe_id",
  "maybe_var_assign", "var_assign", "var", "var_ids",
  "maybe_indirect_var_ids", "var_id_maybe_assign", "var_id",
  "maybe_var_array", "var_array", "$@24", "type", "type_red1", "$@25",
  "$@26", "$@27", "templated_id", "$@28", "$@29", "types", "$@30",
  "maybe_scoped_id", "scoped_id", "type_indirection", "$@31", "$@32",
  "$@33", "$@34", "type_red2", "type_primitive", "$@35", "type_integer",
  "optional_scope", "scope_list", "$@36", "scope_type", "literal",
  "literal2", "macro", "$@37", "$@38", "$@39", "$@40", "$@41", "$@42",
  "$@43", "$@44", "$@45", "$@46", "$@47", "$@48", "$@49", "$@50", "$@51",
  "$@52", "$@53", "$@54", "$@55", "$@56", "$@57", "op_token",
  "op_token_no_delim", "vtk_constant_def", "maybe_other",
  "maybe_other_no_semi", "maybe_other_class", "other_stuff_or_class",
  "other_stuff", "other_stuff_no_semi", "braces", "parens", "brackets",
  "typedef_ignore", "typedef_ignore_body", 0
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
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   123,   125,    60,    62,
      58,    40,    41,    59,    44,    61,    45,    43,   126,    42,
      47,    37,    38,   124,    94,    91,    93,    33,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   139,   140,   141,   141,   142,   142,   142,   142,   144,
     143,   145,   143,   146,   146,   147,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   147,   147,   147,   147,
     147,   147,   148,   149,   150,   150,   150,   151,   151,   152,
     152,   152,   152,   153,   153,   153,   154,   154,   154,   154,
     155,   155,   155,   156,   156,   156,   156,   156,   156,   156,
     156,   157,   158,   159,   160,   161,   162,   162,   162,   163,
     163,   163,   163,   163,   163,   163,   163,   163,   163,   163,
     164,   165,   166,   166,   167,   167,   168,   168,   169,   169,
     169,   170,   171,   171,   171,   171,   171,   171,   171,   172,
     172,   172,   172,   172,   174,   175,   176,   173,   178,   179,
     177,   177,   181,   182,   180,   184,   185,   183,   183,   186,
     186,   187,   187,   189,   188,   191,   192,   190,   194,   193,
     195,   195,   196,   196,   197,   199,   198,   201,   200,   202,
     203,   203,   204,   204,   205,   205,   205,   206,   206,   207,
     208,   207,   209,   209,   210,   210,   211,   210,   212,   213,
     212,   212,   214,   212,   215,   216,   212,   217,   217,   218,
     218,   219,   220,   220,   220,   220,   220,   220,   220,   220,
     220,   220,   220,   220,   220,   220,   220,   221,   221,   222,
     222,   222,   222,   223,   224,   225,   225,   227,   226,   226,
     228,   228,   228,   228,   229,   229,   229,   230,   229,   229,
     231,   229,   229,   232,   229,   234,   233,   235,   233,   236,
     237,   236,   238,   238,   238,   238,   239,   239,   239,   240,
     240,   240,   240,   240,   240,   240,   240,   241,   240,   242,
     240,   243,   240,   244,   240,   245,   245,   245,   245,   245,
     245,   245,   246,   246,   246,   246,   246,   246,   246,   246,
     246,   246,   246,   246,   246,   246,   246,   247,   246,   246,
     248,   248,   248,   248,   248,   248,   248,   249,   249,   250,
     251,   250,   252,   252,   252,   252,   253,   253,   253,   253,
     253,   254,   254,   254,   254,   254,   254,   256,   255,   257,
     258,   255,   259,   255,   260,   255,   261,   262,   255,   263,
     255,   264,   265,   255,   266,   255,   267,   255,   268,   255,
     269,   255,   270,   255,   271,   255,   272,   255,   273,   255,
     274,   255,   275,   255,   276,   255,   255,   255,   255,   255,
     277,   277,   277,   277,   277,   278,   278,   278,   278,   278,
     278,   278,   278,   278,   278,   278,   278,   278,   278,   278,
     278,   278,   278,   278,   278,   278,   278,   278,   278,   278,
     278,   278,   278,   278,   278,   278,   278,   278,   278,   278,
     278,   278,   278,   278,   278,   279,   280,   280,   281,   281,
     282,   282,   283,   283,   283,   284,   284,   284,   285,   285,
     285,   285,   285,   285,   285,   285,   285,   285,   285,   285,
     285,   285,   285,   285,   285,   285,   285,   285,   285,   285,
     285,   285,   285,   285,   285,   285,   285,   285,   285,   285,
     285,   285,   285,   286,   287,   287,   287,   288,   289,   290,
     290,   290
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     0,     3,     2,     3,     2,     1,     0,
       7,     0,    10,     0,     2,     2,     1,     2,     1,     1,
       1,     1,     1,     1,     1,     2,     1,     1,     2,     3,
       3,     2,     3,     2,     3,     3,     2,     3,     2,     5,
       2,     1,     7,     6,     0,     1,     3,     1,     3,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     3,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     7,     6,     3,     2,     3,     1,     4,     3,     4,
       2,     8,     7,     9,     9,     2,     2,     2,     2,     3,
       2,     2,     3,     4,     1,     3,     2,     2,     1,     1,
       1,     4,     2,     3,     1,     2,     3,     4,     3,     1,
       2,     3,     4,     3,     0,     0,     0,     9,     0,     0,
       4,     2,     0,     0,     7,     0,     0,     4,     2,     2,
       2,     0,     1,     0,     5,     0,     0,     4,     0,     5,
       0,     3,     0,     3,     4,     0,     2,     0,     5,     1,
       1,     2,     1,     1,     1,     4,     3,     0,     1,     1,
       0,     4,     0,     1,     1,     1,     0,     4,     2,     0,
       4,     1,     0,     5,     0,     0,     9,     0,     1,     0,
       1,     2,     4,     5,     4,     5,     4,     5,     3,     4,
       2,     3,     5,     5,     6,     8,     8,     1,     3,     1,
       3,     2,     4,     2,     2,     0,     1,     0,     3,     4,
       2,     1,     2,     3,     1,     2,     1,     0,     3,     1,
       0,     3,     2,     0,     4,     0,     5,     0,     5,     1,
       0,     4,     1,     1,     1,     1,     3,     3,     3,     1,
       1,     2,     2,     2,     2,     1,     1,     0,     4,     0,
       4,     0,     3,     0,     3,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     0,     3,     1,
       1,     1,     1,     1,     1,     1,     1,     0,     2,     2,
       0,     5,     0,     1,     1,     1,     1,     2,     2,     1,
       3,     1,     1,     1,     1,     1,     1,     0,     7,     0,
       0,     8,     0,     5,     0,     5,     0,     0,    10,     0,
       7,     0,     0,     8,     0,     7,     0,     7,     0,     7,
       0,     7,     0,     7,     0,     7,     0,     7,     0,     7,
       0,     7,     0,     9,     0,     9,     4,     4,     6,     7,
       2,     2,     3,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     0,     2,     0,     2,
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     3,     3,     3,     3,     3,     3,     0,
       2,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
     396,   437,   439,   438,   430,   260,   421,   417,   418,   419,
     420,   281,   263,   282,   283,   285,   286,   264,   262,   280,
     266,   265,   258,   259,   431,   432,   422,   408,   423,   424,
     425,   426,   427,   277,   429,   433,   449,   440,   441,   261,
     428,   435,   434,   369,   370,   396,   396,   371,   372,   373,
     374,   375,   376,   382,   383,   377,   378,   379,   380,   381,
     384,   385,   386,   387,   388,   389,   390,   391,   392,   393,
     394,   436,   416,   267,   268,   269,   270,   271,   272,   273,
     274,   275,   276,   284,   256,   257,   395,   400,   363,   364,
     413,   396,   405,   362,   355,   358,   359,   361,   356,   357,
     368,   365,   366,   367,   396,   360,   414,     0,   415,   255,
     279,   412,   442,     3,   396,   406,   409,   410,   411,   407,
       0,   449,   449,     0,     0,     0,   402,   403,     0,   400,
     404,     0,     0,     1,     0,     0,     2,   396,     8,     0,
     397,   278,   450,   451,   448,   445,   446,   443,   401,   444,
     447,   153,   152,     9,     0,     3,     0,     7,     5,     0,
     287,    99,   100,    98,    92,   177,     0,    94,   177,     4,
       6,   260,   149,     0,   261,   150,     0,     0,   229,   211,
     216,     0,   219,   214,   292,     0,   178,    97,    93,     0,
      96,     0,   227,     0,     0,     0,   222,     0,   225,   151,
     210,     0,   212,   230,     0,     0,    11,     0,   246,   245,
     240,   239,   215,   293,   294,   295,   288,     0,    13,    95,
     232,   233,   234,   236,   235,     0,     0,   237,     0,   213,
       0,   238,   218,   287,   221,     0,     0,   244,   243,   242,
     241,   289,     0,     0,   260,     0,     0,    27,     0,     0,
       0,     0,     0,   398,   261,   150,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    13,    19,    18,    21,    20,    22,    26,    24,
      23,     0,     0,     0,     0,     0,     0,   109,   104,   135,
       0,    16,     0,     0,    41,     0,   224,     0,   231,     0,
     254,   252,     0,     0,     0,     0,     0,     0,    44,     0,
     396,     0,     0,     0,    25,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    17,     0,     0,     0,
       0,    80,     0,    86,    85,    88,    87,     0,     0,   398,
       0,   190,     0,     0,   309,   312,   314,     0,     0,   321,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   396,   102,   145,     0,    10,    14,
     396,   154,    36,    31,    74,    90,    91,    38,    33,    28,
     140,   138,     0,     0,     0,     0,   110,   118,   105,   125,
     205,     0,   197,   179,    15,    40,   228,   226,    13,   248,
     250,   292,   396,   398,    76,    75,   205,     0,     0,   103,
       0,   113,   108,     0,     0,    45,    47,    44,     0,     0,
       0,   396,     0,     0,   114,     0,    34,    29,     0,    37,
      32,    35,    30,     0,     0,     0,     0,   396,     0,     0,
       0,    89,     0,     0,     0,    73,   399,   191,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   324,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   146,   147,     0,     0,   136,   162,   111,
     106,     0,   369,   370,     0,     0,   122,   354,     0,     0,
       0,     0,   131,   121,   131,   128,   207,   133,   398,   204,
     206,   188,     0,     0,   203,   180,     0,   291,     0,     0,
     182,     0,   112,   107,   398,    44,     0,     0,   184,     0,
     398,     0,   186,     0,   162,     0,   396,     0,     0,    79,
       0,   101,     0,   307,     0,     0,     0,   316,   319,     0,
       0,   326,   330,   334,   338,   328,   332,   336,   340,   342,
     344,   346,   347,     0,     0,   162,   156,   142,     0,   137,
     171,   164,     0,   163,   165,   205,   189,     0,     0,   350,
     351,     0,     0,     0,   130,   129,   132,   119,   126,   205,
     162,     0,   198,   199,     0,   305,   299,   301,   302,   303,
     304,   306,     0,     0,     0,   181,   296,    12,     0,    78,
     183,     0,    46,   153,    53,    54,    55,   152,     0,    60,
      61,    62,    56,    49,    48,     0,    50,    52,    51,   398,
     185,     0,   398,   187,     0,     0,     0,     0,     0,     0,
       0,   310,   313,   315,     0,     0,   322,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    39,
       0,   155,     0,   141,   396,   139,     0,   174,   172,   169,
     168,   352,   353,   123,     0,   157,   193,   396,   192,   120,
     127,   208,     0,   205,     0,   201,     0,   298,   297,    77,
      43,     0,    63,    64,    65,    66,    67,    68,    69,    70,
       0,    57,     0,    72,     0,   115,     0,     0,   398,   398,
       0,     0,   317,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   348,     0,   148,   142,
       0,     0,   177,   177,   179,   162,   194,     0,   158,   159,
       0,   134,   209,   200,     0,   300,    59,    58,    42,    71,
     131,    82,     0,     0,     0,   308,     0,     0,   320,     0,
     325,   327,   331,   335,   339,   329,   333,   337,   341,     0,
       0,   349,   143,   144,   167,     0,     0,   170,     0,     0,
       0,     0,   202,   116,    81,     0,     0,   311,   398,   323,
       0,     0,     0,   173,   124,   196,     0,   195,   117,    83,
      84,     0,   343,   345,   175,   161,   318,   157,     0,   176
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,   107,   136,   137,   138,   160,   233,   281,   282,   283,
     284,   424,   425,   622,   623,   624,   625,   700,   285,   286,
     287,   288,   289,   415,   290,   291,   292,   293,   166,   167,
     168,   294,   295,   296,   297,   534,   750,   798,   396,   502,
     679,   397,   581,   735,   398,   504,   680,   503,   587,   399,
     590,   298,   390,   569,   299,   488,   487,   663,   567,   375,
     483,   376,   565,   176,   177,   416,   382,   737,   738,   780,
     572,   573,   666,   574,   734,   733,   732,   807,   187,   514,
     515,   301,   401,   592,   402,   403,   509,   510,   589,   575,
     179,   205,   207,   226,   180,   228,   225,   181,   230,   223,
     182,   594,   312,   313,   236,   235,   108,   109,   120,   110,
     185,   216,   314,   303,   605,   606,   304,   640,   462,   711,
     463,   464,   644,   757,   645,   467,   714,   550,   648,   652,
     649,   653,   650,   654,   651,   655,   656,   657,   496,   111,
     112,   113,   348,   128,   129,   114,   115,   116,   117,   118,
     119,   123
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -659
static const yytype_int16 yypact[] =
{
    1324,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,
    -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,
    -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,
    -659,  -659,  -659,  -659,  -659,  -659,  1189,  -659,  -659,  -659,
    -659,  -659,  -659,  -659,  -659,  1324,  1324,  -659,  -659,  -659,
    -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,
    -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,
    -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,
    -659,  -659,  -659,  -659,  -659,  -659,  -659,  1053,  -659,  -659,
    -659,  1324,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,
    -659,  -659,  -659,  -659,  1324,  -659,  -659,    44,  -659,  -659,
    -659,  -659,  -659,    50,  1324,  -659,  -659,  -659,  -659,  -659,
     377,  1189,  1189,   -74,   -70,   -67,  -659,  -659,   -40,  1053,
    -659,   -35,   -31,  -659,   128,     7,  -659,  1324,  -659,  1973,
    -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,
    -659,  -659,  -659,    25,   166,    50,  2407,  -659,  -659,  2256,
      37,  -659,  -659,  -659,  -659,   128,    46,    57,   128,  -659,
    -659,   -59,  -659,   182,   -42,   172,  2375,  2290,    98,  -659,
     240,   106,    -6,    66,    96,   130,  -659,  -659,  -659,   345,
    -659,   236,  -659,   -59,   -42,   177,   109,   236,  -659,  -659,
    -659,  2375,  -659,  -659,   236,    66,  -659,    66,   168,   186,
     188,  -659,  -659,  -659,  -659,  -659,  -659,   236,   814,  -659,
     -59,   -42,   177,  -659,  -659,  2256,    66,  -659,  2256,  -659,
    2256,  -659,  -659,    37,  -659,    66,    66,  -659,  -659,   196,
     220,   133,   128,  1849,   171,    16,    39,  -659,  2256,  1592,
     394,  1939,   950,  1459,   205,    13,   141,   153,   155,   159,
     169,   180,   195,   211,   213,   214,   219,   237,   241,   255,
     260,   261,   268,   271,   277,   282,   283,   284,   286,   303,
     128,   176,   814,  -659,  -659,  -659,  -659,  -659,  -659,  -659,
    -659,    52,    52,  1592,    52,    52,    52,  -659,  -659,  -659,
     305,  -659,   335,   189,   218,   193,  -659,   238,  -659,   245,
    -659,  -659,    66,    66,   250,   131,   128,   392,   128,   126,
    1324,   146,   310,   128,  -659,    52,    52,   413,    52,    52,
    1678,    52,    52,   128,   128,   128,  -659,   251,    58,    16,
      39,  -659,   309,  -659,  -659,  -659,  -659,   253,   318,  1459,
     320,  -659,  1764,   128,  -659,  -659,  -659,   128,   128,  -659,
     128,   128,   128,   128,   128,   128,   128,   128,   128,   128,
     128,   128,   128,   128,  1324,  -659,  -659,   321,  -659,  -659,
    1324,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,
     326,  -659,   125,  2016,   128,   128,  -659,    -1,  -659,    -1,
      73,   327,   325,   329,  -659,  -659,  -659,  -659,   814,  -659,
    -659,    96,  1324,  1459,  -659,  -659,   -25,   328,   128,  -659,
     125,  -659,  -659,   331,   343,   337,   338,   128,   342,   128,
     349,  1324,   346,   128,  -659,   191,  -659,  -659,   125,  -659,
    -659,  -659,  -659,   154,   154,   154,   128,  1324,   352,   355,
     357,  -659,   128,   128,   366,  -659,  -659,  -659,  1849,   369,
     319,   368,   128,   128,   128,   373,   378,   128,  -659,   380,
     381,   382,   385,   397,   399,   400,   401,   402,   403,   371,
     379,   404,   409,  -659,  -659,   417,   128,  -659,  2086,  -659,
    -659,   421,   418,   419,   429,   420,  -659,  -659,   430,   433,
     547,   548,   528,  -659,   528,  -659,  -659,  -659,  1459,  -659,
    -659,  -659,   154,   257,  -659,  -659,   443,  -659,   444,   439,
    -659,   440,  -659,  -659,  1459,   128,   203,   452,  -659,   447,
    1459,   456,  -659,   453,  2086,   458,  1324,   457,   459,  -659,
     414,  -659,   128,  -659,   454,   460,   461,  -659,  -659,   462,
     463,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,
    -659,  -659,  -659,   128,   465,  2086,   466,   467,   464,  -659,
    -659,  -659,   468,  -659,   471,    17,  -659,   448,   469,  -659,
    -659,   476,   187,   477,  -659,  -659,  -659,  -659,  -659,   -25,
    2086,   470,  -659,   475,   128,  -659,  -659,  -659,  -659,  -659,
    -659,  -659,   257,   334,   334,  -659,  -659,  -659,   478,  -659,
    -659,   480,  -659,   -59,  -659,  -659,  -659,   -42,   203,  -659,
    -659,  -659,   416,  -659,  -659,   203,  -659,   177,  -659,  1459,
    -659,   484,  1459,  -659,   486,   128,   492,   489,   490,   128,
    2375,  -659,  -659,  -659,  2407,  2407,  -659,  2407,  2407,  2407,
    2407,  2407,  2407,  2407,  2407,  2407,  2407,  2407,    60,  -659,
     491,  -659,   128,  -659,  1324,  -659,   488,  -659,  -659,  -659,
    -659,  -659,  -659,  -659,   493,  2171,  -659,  1324,  -659,  -659,
    -659,  -659,   495,   -25,   154,   494,   497,  -659,  -659,  -659,
    -659,   498,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,
     203,  -659,   499,  -659,   502,  -659,   503,   128,  1459,  1459,
     505,  2375,  -659,   506,  2407,   507,   508,   509,   510,   511,
     512,   513,   515,   519,   518,   520,  -659,   521,  -659,   467,
     524,  2086,   128,   128,   329,  2086,  -659,   525,  -659,   526,
     485,  -659,  -659,  -659,   154,  -659,  -659,  -659,  -659,  -659,
     528,  -659,   530,   529,   532,  -659,   536,   539,  -659,   542,
    -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,   570,
     582,  -659,  -659,  -659,  -659,   543,   544,  -659,   545,   546,
     552,   554,  -659,  -659,  -659,   555,   558,  -659,  1459,  -659,
     549,   574,   527,  -659,  -659,  -659,  2171,  -659,  -659,  -659,
    -659,   575,  -659,  -659,  -659,  -659,  -659,  2171,   576,  -659
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -659,  -659,   550,  -659,   560,  -659,  -659,  -264,  -659,   362,
     449,  -406,  -659,  -659,  -659,  -550,  -659,  -659,   450,   451,
    -659,  -659,  -214,  -659,  -659,   472,   473,   -97,   479,  -659,
    -659,  -659,   -53,    45,  -659,  -659,  -659,  -659,  -272,  -659,
    -659,  -659,  -659,  -659,  -303,  -659,  -659,   301,  -501,  -659,
    -659,  -659,  -659,  -659,  -659,  -659,  -659,   -23,    42,   391,
    -659,  -659,  -659,   531,  -659,   322,   204,   -98,   -86,  -659,
    -523,   -20,  -659,  -658,  -659,  -659,  -659,  -659,  -167,   -22,
    -659,   474,  -241,  -636,  -499,  -332,  -543,  -659,  -659,  -121,
    -170,  -659,  -659,  -659,  -163,  -659,  -659,   208,  -659,   156,
    -161,   -89,  -659,  -659,  -659,  -659,  -137,  -659,  -659,   593,
     482,   307,  -659,  -175,   117,  -521,  -659,  -659,  -659,  -659,
    -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,
    -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,  -659,   333,
    -659,   -41,  -341,   591,  -659,   -58,   -36,  -659,  -659,  -659,
    -659,   112
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -293
static const yytype_int16 yytable[] =
{
     122,   190,   158,   588,   124,   125,   200,   202,   456,   217,
     195,   634,   196,   593,   422,   454,   139,   739,   379,   170,
     191,   527,   183,   506,   151,   151,  -220,  -220,   222,   130,
     224,   229,   670,   500,   222,   324,   224,   197,   178,   183,
     183,   222,   660,   224,   133,   421,   681,   151,   743,   144,
     131,   199,   145,   134,   222,   146,   224,   165,   139,   192,
     350,   152,   152,   132,   183,   506,   151,   682,   691,   667,
     668,   130,   519,   140,   417,   701,   198,   147,   428,   384,
     432,   183,   687,   688,   152,   122,   122,   149,   183,   490,
     135,   183,   165,   183,   212,   685,   155,   302,   208,   209,
     213,   214,   215,   152,   178,   150,   183,   178,   782,   178,
     508,   183,   183,   183,   183,   183,   232,   523,   234,   612,
     489,   506,   317,  -220,   501,   154,  -220,   322,   327,   327,
     337,   347,   318,   151,   151,   490,   151,   306,   739,   151,
     742,  -223,  -223,   159,   516,   183,   310,   311,   522,   739,
     747,   491,   508,   330,   151,   320,   183,   184,   208,   209,
     393,   302,   151,   208,   209,   188,   489,   591,   380,   161,
     152,   152,   327,   152,   447,   381,   152,   521,   208,   209,
     162,   189,   726,   611,   727,   593,   208,   209,   529,   631,
     193,   152,   533,   183,   507,   210,   325,   331,   211,   152,
    -253,  -253,   417,   428,   432,   491,   135,   163,   508,   327,
     199,   613,   778,   614,   615,   183,   616,   349,  -251,  -251,
     237,   238,   203,   409,   410,   206,   418,   194,  -247,  -247,
     429,   460,   433,   142,   143,   674,   217,   422,  -223,   490,
     385,  -223,   427,   669,   220,   593,   218,   412,   617,   783,
     191,   413,  -249,  -249,   414,   210,   204,  -290,   211,   151,
     210,   151,   431,   211,   351,   595,   596,   597,   598,   599,
     600,   183,  -217,  -217,   352,   210,   353,   385,   211,   430,
     354,   221,   446,   210,   197,   164,   211,   302,   702,   192,
     355,   704,  -153,   378,   326,   332,   152,  -253,   152,   459,
    -253,   356,   601,   394,   395,   452,   453,   412,   675,   404,
     676,   413,   406,   349,   414,  -251,   357,   239,  -251,   204,
     240,   183,   677,   198,   618,  -247,  -152,   151,  -247,   619,
     620,   621,   358,   482,   359,   360,   523,   540,   386,   485,
     361,   405,   595,   151,   597,   598,   599,   600,   161,  -249,
     542,   183,  -249,   227,   418,   429,   433,   407,   362,   162,
     231,   408,   363,   627,   152,   628,   392,   753,   754,  -217,
     393,   518,  -217,   241,   411,   386,   364,   349,   602,   601,
     152,   365,   366,   603,   604,   135,   163,   394,   395,   367,
     531,    11,   368,    13,    14,    15,    16,   183,   369,    19,
     151,   243,   244,   370,   371,   372,   535,   373,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,   151,   151,   420,   374,   172,   391,   393,   183,   248,
      33,   434,   451,   305,   135,   173,   307,   152,   308,   254,
     175,   455,   484,   457,   438,   639,   486,   801,   393,   512,
     511,   520,   507,   183,   513,   627,   153,   628,   152,   152,
     524,   525,   627,   526,   628,   528,   530,    83,   536,   532,
     710,   427,   349,   431,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,   186,   349,   539,
     186,   541,   543,   561,   349,   636,   383,   547,   387,   388,
     389,   562,   548,   183,   551,   552,   553,   712,   713,   554,
     715,   716,   717,   718,   719,   720,   721,   722,   723,   724,
     725,   555,   280,   556,   557,   558,   559,   560,   563,   436,
     437,   564,   439,   440,   566,   441,   442,   627,   183,   628,
     300,   756,   692,   693,   576,   694,   695,   696,   697,   698,
     699,   579,   582,   577,   578,   583,   580,   584,   585,   586,
     607,   608,   609,   610,   315,   775,   776,   319,   321,   629,
     630,   300,   300,   632,   183,   635,   633,   759,   641,   637,
     790,   638,   642,   643,   671,   664,   646,   647,   659,   661,
     665,   662,   791,   349,   183,  -166,   349,   673,   183,   684,
     678,   689,   377,   690,   300,   672,   683,   703,   705,   707,
     708,   709,   731,   728,   343,   300,   736,   741,   744,   745,
     746,   781,   748,   730,   400,   749,   751,   755,   758,   760,
     761,   762,   763,   764,   765,   766,   740,   767,   377,   423,
     426,   768,   769,   771,   770,   435,   773,   779,   804,   423,
    -160,   785,   300,   784,   786,   443,   444,   445,   787,   183,
     448,   449,   450,   788,   789,   792,   793,   794,   219,   795,
     183,   802,   349,   349,   300,   461,   796,   797,   799,   465,
     466,   800,   468,   469,   470,   471,   472,   473,   474,   475,
     476,   477,   478,   479,   480,   481,   803,   806,   809,   157,
     505,   344,   345,   346,   729,   169,   772,   419,   201,   808,
     805,   774,   777,   141,   400,   309,   498,   499,   517,   686,
     148,     0,   328,   329,     0,   336,   497,     0,     0,     0,
     300,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   423,     0,     0,     0,     0,     0,     0,   426,
       0,     0,   349,     0,     0,     0,     0,     0,     0,     0,
     423,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   537,   538,     0,     0,     0,     0,
       0,     0,   423,     0,   544,   545,   546,     0,     0,   549,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   568,     0,
       0,     0,     0,     0,     0,     0,     0,   242,   213,   214,
     215,   243,   244,     0,     0,     0,     0,     0,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,   245,   246,   247,     0,   172,     0,   426,   626,   248,
      33,   249,   250,   251,   135,   173,   252,     0,   253,   254,
     255,   256,   423,   257,   423,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   658,     0,     0,     0,     0,
       0,     0,     0,     0,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271,   272,   273,   274,   275,   276,   277,   278,     0,   279,
       0,     0,     0,     0,  -292,     0,     0,     0,     0,     0,
     626,     0,   280,     0,     0,     0,     0,   626,     0,     0,
       0,     0,     0,   338,     0,     0,     0,   706,   171,     0,
       0,   423,     0,     0,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,   339,   340,   341,
       0,   172,     0,     0,   568,     0,    33,     0,     0,     0,
       0,   173,     0,     0,     0,   174,   175,   342,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   626,     0,     0,     0,     0,     0,     0,   752,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   186,   186,   126,     1,     2,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
       0,    34,     0,   127,    35,    36,    37,    38,    39,    40,
      41,    42,     0,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    86,     0,    87,
       0,    88,    89,    90,    91,     0,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,     0,
     105,   106,   121,     1,     2,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,     0,    34,     0,     0,
      35,     0,    37,    38,    39,    40,    41,    42,     0,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    86,     0,    87,     0,    88,    89,    90,
      91,     0,     0,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,     0,   105,   106,     1,     2,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,     0,    34,     0,     0,    35,    36,    37,    38,    39,
      40,    41,    42,     0,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    86,     0,
      87,     0,    88,    89,    90,    91,     0,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
       0,   105,   106,     1,     2,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,     0,    34,     0,     0,
      35,     0,    37,    38,    39,    40,    41,    42,     0,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    86,     0,    87,     0,    88,    89,    90,
      91,     0,     0,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   323,   105,   106,     0,   243,
     244,     0,     0,     0,     0,     0,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,     0,
       0,     0,     0,   172,     0,     0,     0,   248,    33,     0,
       0,     0,     0,   173,     0,     0,     0,   254,   175,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,   243,   244,     0,     0,     0,
       0,     0,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,     0,     0,     0,     0,   172,
       0,     0,     0,   248,    33,     0,     0,     0,     0,   173,
     280,     0,     0,   254,   175,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,   458,   244,     0,     0,     0,     0,     0,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,     0,     0,     0,     0,   172,     0,     0,     0,     0,
      33,     0,     0,     0,     0,   173,   280,     0,     0,   254,
     175,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,   171,     0,     0,
       0,     0,     0,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,     0,     0,     0,     0,
     172,     0,     0,     0,     0,    33,     0,     0,     0,     0,
     173,     0,   280,     0,   174,   175,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,   333,     0,     0,     0,     0,   171,     0,     0,
       0,     0,     0,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,   334,   335,     0,     0,
     172,     0,     0,     0,     0,    33,   134,   316,     0,     0,
     173,     5,     0,     0,   174,   255,   256,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    33,
       0,   156,     0,     0,     0,     0,     0,     0,    39,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,   492,   493,     0,     0,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,   171,     0,     0,     0,     0,     0,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,     0,     0,     0,     0,   172,     0,     0,
       0,     0,    33,     0,     0,     0,     0,   173,     0,     0,
       0,   174,   175,   570,    88,    89,     0,   494,     0,     0,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   495,     0,   105,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   571,     0,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,   171,
       0,     0,     0,     0,     0,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,     0,     0,
       0,     0,   172,     0,     0,     0,     0,    33,     0,     0,
       0,     0,   173,     0,     0,     0,   174,   175,   570,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,   171,     0,     0,     0,     0,     0,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,     0,     0,     0,     0,   172,     0,     0,
       0,     0,    33,     0,     0,     0,     0,   173,   171,     0,
       0,   174,   175,     0,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,     0,     0,     0,
       0,   172,     0,     0,     0,     0,    33,     0,     0,     0,
       0,   173,     0,     0,     0,   174,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,   171,     0,     0,     0,     0,     0,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    33,     0,     0,     0,     5,   173,     0,     0,     0,
     174,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    33,     0,     0,     0,     0,     0,     0,
       0,     0,    39,     0,     0,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85
};

static const yytype_int16 yycheck[] =
{
      36,   168,   139,   504,    45,    46,   176,   177,   349,   184,
     173,   534,   173,   512,   317,   347,   113,   675,   282,   156,
      79,   427,   159,    48,     8,     8,    32,    33,   191,    87,
     191,   201,   575,    34,   197,   249,   197,    79,   159,   176,
     177,   204,   565,   204,     0,   317,   589,     8,   684,   123,
      91,    38,   122,     3,   217,   122,   217,   154,   155,   118,
      47,    45,    45,   104,   201,    48,     8,   590,   618,    52,
      53,   129,   413,   114,   315,   625,   118,   117,   319,   293,
     321,   218,   603,   604,    45,   121,   122,   122,   225,   392,
      40,   228,   189,   230,   183,   594,   137,   218,    32,    33,
       4,     5,     6,    45,   225,   136,   243,   228,   744,   230,
     135,   248,   249,   250,   251,   252,   205,   420,   207,   525,
     392,    48,   243,   129,   125,   118,   132,   248,   249,   250,
     251,   252,   116,     8,     8,   438,     8,   226,   796,     8,
     683,    32,    33,   118,   408,   282,   235,   236,   420,   807,
     700,   392,   135,   250,     8,   116,   293,   120,    32,    33,
      35,   282,     8,    32,    33,   119,   438,   508,   116,     3,
      45,    45,   293,    45,   116,   123,    45,   418,    32,    33,
      14,   124,   122,   524,   124,   684,    32,    33,   429,   530,
       8,    45,   433,   330,   121,   129,   249,   250,   132,    45,
      32,    33,   443,   444,   445,   446,    40,    41,   135,   330,
      38,     8,   735,    10,    11,   352,    13,   253,    32,    33,
      32,    33,   124,   312,   313,   119,   315,    45,    32,    33,
     319,   352,   321,   121,   122,    48,   411,   540,   129,   542,
     293,   132,   116,   575,     8,   744,   116,   116,    45,   750,
      79,   120,    32,    33,   123,   129,    79,   124,   132,     8,
     129,     8,   116,   132,   123,     8,     9,    10,    11,    12,
      13,   408,    32,    33,   121,   129,   121,   330,   132,   320,
     121,    45,    31,   129,    79,   119,   132,   408,   629,   118,
     121,   632,   121,   117,   249,   250,    45,   129,    45,   352,
     132,   121,    45,    52,    53,    52,    53,   116,   121,   120,
     123,   120,   119,   349,   123,   129,   121,   129,   132,    79,
     132,   458,   135,   118,   121,   129,   121,     8,   132,   126,
     127,   128,   121,   374,   121,   121,   639,   458,   293,   380,
     121,   123,     8,     8,    10,    11,    12,    13,     3,   129,
      31,   488,   132,   197,   443,   444,   445,   119,   121,    14,
     204,   116,   121,   526,    45,   526,    31,   708,   709,   129,
      35,   412,   132,   217,   124,   330,   121,   413,   121,    45,
      45,   121,   121,   126,   127,    40,    41,    52,    53,   121,
     431,    14,   121,    16,    17,    18,    19,   534,   121,    22,
       8,     7,     8,   121,   121,   121,   447,   121,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,     8,     8,    31,   121,    31,   121,    35,   565,    35,
      36,   121,   123,   225,    40,    41,   228,    45,   230,    45,
      46,   123,   121,   123,    31,    31,   120,   788,    35,   124,
     123,   123,   121,   590,   125,   618,   134,   618,    45,    45,
     117,   124,   625,   125,   625,   123,   117,    90,   116,   123,
     640,   116,   508,   116,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,   165,   524,   123,
     168,   122,   124,   122,   530,   536,   292,   124,   294,   295,
     296,   122,   124,   640,   124,   124,   124,   644,   645,   124,
     647,   648,   649,   650,   651,   652,   653,   654,   655,   656,
     657,   124,   128,   124,   124,   124,   124,   124,   124,   325,
     326,   122,   328,   329,   117,   331,   332,   700,   675,   700,
     218,   711,   126,   127,   123,   129,   130,   131,   132,   133,
     134,   122,   122,   135,   135,   122,   136,    10,    10,    31,
     117,   117,   123,   123,   242,   732,   733,   245,   246,   117,
     123,   249,   250,   117,   711,   117,   123,   714,   124,   122,
      10,   122,   122,   122,   136,   121,   124,   124,   123,   123,
     122,   124,    10,   629,   731,   124,   632,   121,   735,   124,
     123,   123,   280,   123,   282,   136,   136,   123,   122,   117,
     121,   121,   124,   122,   252,   293,   123,   122,   124,   122,
     122,   136,   123,   664,   302,   123,   123,   122,   122,   122,
     122,   122,   122,   122,   122,   122,   677,   122,   316,   317,
     318,   122,   124,   122,   124,   323,   122,   122,   121,   327,
     124,   122,   330,   123,   122,   333,   334,   335,   122,   796,
     338,   339,   340,   124,   122,   122,   122,   122,   189,   123,
     807,   122,   708,   709,   352,   353,   124,   123,   123,   357,
     358,   123,   360,   361,   362,   363,   364,   365,   366,   367,
     368,   369,   370,   371,   372,   373,   122,   122,   122,   139,
     399,   252,   252,   252,   662,   155,   729,   316,   177,   807,
     796,   731,   734,   120,   392,   233,   394,   395,   411,   602,
     129,    -1,   250,   250,    -1,   251,   393,    -1,    -1,    -1,
     408,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   420,    -1,    -1,    -1,    -1,    -1,    -1,   427,
      -1,    -1,   788,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     438,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   452,   453,    -1,    -1,    -1,    -1,
      -1,    -1,   460,    -1,   462,   463,   464,    -1,    -1,   467,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   486,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,
       6,     7,     8,    -1,    -1,    -1,    -1,    -1,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    -1,    31,    -1,   525,   526,    35,
      36,    37,    38,    39,    40,    41,    42,    -1,    44,    45,
      46,    47,   540,    49,   542,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   563,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,    -1,   115,
      -1,    -1,    -1,    -1,   120,    -1,    -1,    -1,    -1,    -1,
     618,    -1,   128,    -1,    -1,    -1,    -1,   625,    -1,    -1,
      -1,    -1,    -1,     3,    -1,    -1,    -1,   635,     8,    -1,
      -1,   639,    -1,    -1,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      -1,    31,    -1,    -1,   662,    -1,    36,    -1,    -1,    -1,
      -1,    41,    -1,    -1,    -1,    45,    46,    47,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   700,    -1,    -1,    -1,    -1,    -1,    -1,   707,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   732,   733,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      -1,    38,    -1,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    -1,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,    -1,   116,
      -1,   118,   119,   120,   121,    -1,   123,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,    -1,
     137,   138,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    -1,    38,    -1,    -1,
      41,    -1,    43,    44,    45,    46,    47,    48,    -1,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   114,    -1,   116,    -1,   118,   119,   120,
     121,    -1,    -1,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,    -1,   137,   138,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    -1,    38,    -1,    -1,    41,    42,    43,    44,    45,
      46,    47,    48,    -1,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,    -1,
     116,    -1,   118,   119,   120,   121,    -1,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
      -1,   137,   138,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    -1,    38,    -1,    -1,
      41,    -1,    43,    44,    45,    46,    47,    48,    -1,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   114,    -1,   116,    -1,   118,   119,   120,
     121,    -1,    -1,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,     3,   137,   138,    -1,     7,
       8,    -1,    -1,    -1,    -1,    -1,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    -1,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    35,    36,    -1,
      -1,    -1,    -1,    41,    -1,    -1,    -1,    45,    46,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,     7,     8,    -1,    -1,    -1,
      -1,    -1,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    -1,    -1,    -1,    -1,    31,
      -1,    -1,    -1,    35,    36,    -1,    -1,    -1,    -1,    41,
     128,    -1,    -1,    45,    46,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,     7,     8,    -1,    -1,    -1,    -1,    -1,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    -1,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,
      36,    -1,    -1,    -1,    -1,    41,   128,    -1,    -1,    45,
      46,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,     8,    -1,    -1,
      -1,    -1,    -1,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    -1,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    36,    -1,    -1,    -1,    -1,
      41,    -1,   128,    -1,    45,    46,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,     3,    -1,    -1,    -1,    -1,     8,    -1,    -1,
      -1,    -1,    -1,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    36,     3,   128,    -1,    -1,
      41,     8,    -1,    -1,    45,    46,    47,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,
      -1,    38,    -1,    -1,    -1,    -1,    -1,    -1,    45,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    50,    51,    -1,    -1,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,     8,    -1,    -1,    -1,    -1,    -1,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    -1,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    36,    -1,    -1,    -1,    -1,    41,    -1,    -1,
      -1,    45,    46,    47,   118,   119,    -1,   121,    -1,    -1,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,    -1,   137,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    78,    -1,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,     8,
      -1,    -1,    -1,    -1,    -1,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    -1,    -1,
      -1,    -1,    31,    -1,    -1,    -1,    -1,    36,    -1,    -1,
      -1,    -1,    41,    -1,    -1,    -1,    45,    46,    47,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,     8,    -1,    -1,    -1,    -1,    -1,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    -1,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    36,    -1,    -1,    -1,    -1,    41,     8,    -1,
      -1,    45,    46,    -1,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    -1,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    36,    -1,    -1,    -1,
      -1,    41,    -1,    -1,    -1,    45,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,     8,    -1,    -1,    -1,    -1,    -1,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    36,    -1,    -1,    -1,     8,    41,    -1,    -1,    -1,
      45,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    36,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    38,    41,    42,    43,    44,    45,
      46,    47,    48,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,   114,   116,   118,   119,
     120,   121,   123,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   137,   138,   140,   245,   246,
     248,   278,   279,   280,   284,   285,   286,   287,   288,   289,
     247,     3,   285,   290,   280,   280,     3,    40,   282,   283,
     284,   280,   280,     0,     3,    40,   141,   142,   143,   166,
     280,   248,   290,   290,   123,   122,   122,   117,   282,   122,
     136,     8,    45,   204,   118,   280,    38,   143,   245,   118,
     144,     3,    14,    41,   119,   166,   167,   168,   169,   141,
     245,     8,    31,    41,    45,    46,   202,   203,   228,   229,
     233,   236,   239,   245,   120,   249,   204,   217,   119,   124,
     217,    79,   118,     8,    45,   233,   239,    79,   118,    38,
     229,   202,   229,   124,    79,   230,   119,   231,    32,    33,
     129,   132,   240,     4,     5,     6,   250,   252,   116,   167,
       8,    45,   233,   238,   239,   235,   232,   238,   234,   229,
     237,   238,   240,   145,   240,   244,   243,    32,    33,   129,
     132,   238,     3,     7,     8,    27,    28,    29,    35,    37,
      38,    39,    42,    44,    45,    46,    47,    49,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   115,
     128,   146,   147,   148,   149,   157,   158,   159,   160,   161,
     163,   164,   165,   166,   170,   171,   172,   173,   190,   193,
     204,   220,   228,   252,   255,   236,   240,   236,   236,   249,
     240,   240,   241,   242,   251,   204,   128,   228,   116,   204,
     116,   204,   228,     3,   161,   171,   172,   228,   164,   165,
     166,   171,   172,     3,    27,    28,   220,   228,     3,    27,
      28,    29,    47,   148,   149,   157,   158,   228,   281,   285,
      47,   123,   121,   121,   121,   121,   121,   121,   121,   121,
     121,   121,   121,   121,   121,   121,   121,   121,   121,   121,
     121,   121,   121,   121,   121,   198,   200,   204,   117,   146,
     116,   123,   205,   205,   161,   171,   172,   205,   205,   205,
     191,   121,    31,    35,    52,    53,   177,   180,   183,   188,
     204,   221,   223,   224,   120,   123,   119,   119,   116,   240,
     240,   124,   116,   120,   123,   162,   204,   221,   240,   198,
      31,   177,   183,   204,   150,   151,   204,   116,   221,   240,
     280,   116,   221,   240,   121,   204,   205,   205,    31,   205,
     205,   205,   205,   204,   204,   204,    31,   116,   204,   204,
     204,   123,    52,    53,   224,   123,   281,   123,     7,   171,
     228,   204,   257,   259,   260,   204,   204,   264,   204,   204,
     204,   204,   204,   204,   204,   204,   204,   204,   204,   204,
     204,   204,   280,   199,   121,   280,   120,   195,   194,   177,
     183,   221,    50,    51,   121,   135,   277,   278,   204,   204,
      34,   125,   178,   186,   184,   186,    48,   121,   135,   225,
     226,   123,   124,   125,   218,   219,   146,   250,   280,   281,
     123,   221,   177,   183,   117,   124,   125,   150,   123,   221,
     117,   280,   123,   221,   174,   280,   116,   204,   204,   123,
     228,   122,    31,   124,   204,   204,   204,   124,   124,   204,
     266,   124,   124,   124,   124,   124,   124,   124,   124,   124,
     124,   122,   122,   124,   122,   201,   117,   197,   204,   192,
      47,    78,   209,   210,   212,   228,   123,   135,   135,   122,
     136,   181,   122,   122,    10,    10,    31,   187,   187,   227,
     189,   281,   222,   223,   240,     8,     9,    10,    11,    12,
      13,    45,   121,   126,   127,   253,   254,   117,   117,   123,
     123,   281,   150,     8,    10,    11,    13,    45,   121,   126,
     127,   128,   152,   153,   154,   155,   204,   233,   239,   117,
     123,   281,   117,   123,   209,   117,   280,   122,   122,    31,
     256,   124,   122,   122,   261,   263,   124,   124,   267,   269,
     271,   273,   268,   270,   272,   274,   275,   276,   204,   123,
     209,   123,   124,   196,   121,   122,   211,    52,    53,   224,
     225,   136,   136,   121,    48,   121,   123,   135,   123,   179,
     185,   225,   209,   136,   124,   223,   253,   254,   254,   123,
     123,   154,   126,   127,   129,   130,   131,   132,   133,   134,
     156,   154,   281,   123,   281,   122,   204,   117,   121,   121,
     229,   258,   245,   245,   265,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   122,   124,   122,   197,
     280,   124,   215,   214,   213,   182,   123,   206,   207,   212,
     280,   122,   225,   222,   124,   122,   122,   154,   123,   123,
     175,   123,   204,   281,   281,   122,   229,   262,   122,   245,
     122,   122,   122,   122,   122,   122,   122,   122,   122,   124,
     124,   122,   196,   122,   210,   217,   217,   218,   209,   122,
     208,   136,   222,   187,   123,   122,   122,   122,   124,   122,
      10,    10,   122,   122,   122,   123,   124,   123,   176,   123,
     123,   281,   122,   122,   121,   207,   122,   216,   206,   122
};

#define yyerrok        (yyerrstatus = 0)
#define yyclearin    (yychar = YYEMPTY)
#define YYEMPTY        (-2)
#define YYEOF        0

#define YYACCEPT    goto yyacceptlab
#define YYABORT        goto yyabortlab
#define YYERROR        goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL        goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                    \
do                                \
  if (yychar == YYEMPTY && yylen == 1)                \
    {                                \
      yychar = (Token);                        \
      yylval = (Value);                        \
      yytoken = YYTRANSLATE (yychar);                \
      YYPOPSTACK (1);                        \
      goto yybackup;                        \
    }                                \
  else                                \
    {                                \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                            \
    }                                \
while (YYID (0))


#define YYTERROR    1
#define YYERRCODE    256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                \
    do                                    \
      if (YYID (N))                                                    \
    {                                \
      (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;    \
      (Current).first_column = YYRHSLOC (Rhs, 1).first_column;    \
      (Current).last_line    = YYRHSLOC (Rhs, N).last_line;        \
      (Current).last_column  = YYRHSLOC (Rhs, N).last_column;    \
    }                                \
      else                                \
    {                                \
      (Current).first_line   = (Current).last_line   =        \
        YYRHSLOC (Rhs, 0).last_line;                \
      (Current).first_column = (Current).last_column =        \
        YYRHSLOC (Rhs, 0).last_column;                \
    }                                \
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)            \
     fprintf (File, "%d.%d-%d.%d",            \
          (Loc).first_line, (Loc).first_column,    \
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

# define YYDPRINTF(Args)            \
do {                        \
  if (yydebug)                    \
    YYFPRINTF Args;                \
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)              \
do {                                      \
  if (yydebug)                                  \
    {                                      \
      YYFPRINTF (stderr, "%s ", Title);                      \
      yy_symbol_print (stderr,                          \
          Type, Value); \
      YYFPRINTF (stderr, "\n");                          \
    }                                      \
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

# define YY_STACK_PRINT(Bottom, Top)                \
do {                                \
  if (yydebug)                            \
    yy_stack_print ((Bottom), (Top));                \
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

# define YY_REDUCE_PRINT(Rule)        \
do {                    \
  if (yydebug)                \
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
#ifndef    YYINITDEPTH
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
        case 9:

/* Line 1455 of yacc.c  */
#line 377 "vtkParse.y"
    {
        start_class((yyvsp[(2) - (2)].str));
      }
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 382 "vtkParse.y"
    {
        start_class((yyvsp[(2) - (5)].str));
      }
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 402 "vtkParse.y"
    { output_function(); }
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 403 "vtkParse.y"
    { reject_function(); }
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 404 "vtkParse.y"
    { output_function(); }
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 405 "vtkParse.y"
    { reject_function(); }
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 406 "vtkParse.y"
    { reject_function(); }
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 407 "vtkParse.y"
    { output_function(); }
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 408 "vtkParse.y"
    { reject_function(); }
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 409 "vtkParse.y"
    { output_function(); }
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 410 "vtkParse.y"
    { reject_function(); }
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 411 "vtkParse.y"
    { reject_function(); }
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 412 "vtkParse.y"
    { legacySig(); output_function(); }
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 437 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 439 "vtkParse.y"
    {
         (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (2)].str)) + strlen((yyvsp[(2) - (2)].str)) + 1);
         sprintf((yyval.str), "%s%s", (yyvsp[(1) - (2)].str), (yyvsp[(2) - (2)].str));
       }
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 444 "vtkParse.y"
    {
         (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (3)].str)) + strlen((yyvsp[(2) - (3)].str)) +
                                  strlen((yyvsp[(3) - (3)].str)) + 3);
         sprintf((yyval.str), "%s %s %s", (yyvsp[(1) - (3)].str), (yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].str));
       }
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 450 "vtkParse.y"
    {
         (yyval.str) = (char *)malloc(strlen((yyvsp[(2) - (3)].str)) + 3);
         sprintf((yyval.str), "(%s)", (yyvsp[(2) - (3)].str));
       }
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 455 "vtkParse.y"
    { (yyval.str) = "-"; }
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 455 "vtkParse.y"
    { (yyval.str) = "+"; }
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 456 "vtkParse.y"
    { (yyval.str) = "~"; }
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 458 "vtkParse.y"
    { (yyval.str) = "-"; }
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 458 "vtkParse.y"
    { (yyval.str) = "+"; }
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 459 "vtkParse.y"
    { (yyval.str) = "*"; }
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 459 "vtkParse.y"
    { (yyval.str) = "/"; }
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 460 "vtkParse.y"
    { (yyval.str) = "%"; }
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 460 "vtkParse.y"
    { (yyval.str) = "&"; }
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 461 "vtkParse.y"
    { (yyval.str) = "|"; }
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 461 "vtkParse.y"
    { (yyval.str) = "^"; }
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 503 "vtkParse.y"
    { preSig("~"); }
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 504 "vtkParse.y"
    { preSig("virtual ~"); }
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 507 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[(1) - (2)].integer);
         }
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 511 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[(1) - (3)].integer);
         }
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 515 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = (yyvsp[(2) - (4)].integer);
         }
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 520 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = (yyvsp[(2) - (3)].integer);
         }
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 527 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[(1) - (1)].integer);
         }
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 531 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[(1) - (2)].integer);
         }
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 535 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[(1) - (3)].integer);
         }
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 539 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = (yyvsp[(2) - (4)].integer);
         }
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 544 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = (yyvsp[(2) - (3)].integer);
         }
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 549 "vtkParse.y"
    { postSig("("); }
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 550 "vtkParse.y"
    { postSig(")"); }
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 551 "vtkParse.y"
    { postSig(";"); openSig = 0; }
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 552 "vtkParse.y"
    {
      (yyval.integer) = (yyvsp[(2) - (9)].integer);
      openSig = 1;
      currentFunction->IsOperator = 1;
      currentFunction->Name = "operator typecast";
      preSig("operator ");
      vtkParseDebug("Parsed operator", "operator typecast");
    }
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 561 "vtkParse.y"
    { postSig(")"); }
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 561 "vtkParse.y"
    { postSig(";"); openSig = 0; }
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 562 "vtkParse.y"
    {
      openSig = 1;
      currentFunction->Name = (yyvsp[(2) - (4)].str);
      vtkParseDebug("Parsed operator", (yyvsp[(2) - (4)].str));
    }
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 568 "vtkParse.y"
    {
      currentFunction->Name = (yyvsp[(2) - (2)].str);
      vtkParseDebug("Parsed operator", (yyvsp[(2) - (2)].str));
      currentFunction->IsPureVirtual = 1;
      if (mainClass)
        {
        data.IsAbstract = 1;
        }
    }
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 578 "vtkParse.y"
    {postSig((yyvsp[(2) - (2)].str));}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 579 "vtkParse.y"
    {
      postSig("(");
      currentFunction->IsOperator = 1;
    }
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 585 "vtkParse.y"
    { postSig(")"); }
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 585 "vtkParse.y"
    { postSig(";"); openSig = 0; }
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 586 "vtkParse.y"
    {
      openSig = 1;
      currentFunction->Name = (yyvsp[(1) - (4)].str);
      vtkParseDebug("Parsed func", (yyvsp[(1) - (4)].str));
    }
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 592 "vtkParse.y"
    {
      currentFunction->Name = (yyvsp[(1) - (2)].str);
      vtkParseDebug("Parsed func", (yyvsp[(1) - (2)].str));
      currentFunction->IsPureVirtual = 1;
      if (mainClass)
        {
        data.IsAbstract = 1;
        }
    }
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 602 "vtkParse.y"
    {postSig(") = 0;");}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 603 "vtkParse.y"
    {postSig(") const = 0;");}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 605 "vtkParse.y"
    {postSig(" const");}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 607 "vtkParse.y"
    {postSig("("); }
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 609 "vtkParse.y"
    { postSig(")"); }
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 610 "vtkParse.y"
    { postSig(";"); openSig = 0; }
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 611 "vtkParse.y"
    {
      openSig = 1;
      currentFunction->Name = (yyvsp[(1) - (4)].str);
      vtkParseDebug("Parsed func", (yyvsp[(1) - (4)].str));
    }
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 617 "vtkParse.y"
    { postSig("("); }
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 625 "vtkParse.y"
    { postSig(");"); openSig = 0; }
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 626 "vtkParse.y"
    {
      openSig = 1;
      currentFunction->Name = (char *)malloc(strlen((yyvsp[(1) - (2)].str)) + 2);
      currentFunction->Name[0] = '~';
      strcpy(&currentFunction->Name[1], (yyvsp[(1) - (2)].str));
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 634 "vtkParse.y"
    { postSig("(");}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 636 "vtkParse.y"
    {postSig("const ");}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 638 "vtkParse.y"
    {postSig("static ");}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 639 "vtkParse.y"
    {postSig("static ");}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 641 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 641 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 649 "vtkParse.y"
    { postSig(", ");}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 653 "vtkParse.y"
    { currentFunction->NumberOfArguments++;}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 654 "vtkParse.y"
    { currentFunction->NumberOfArguments++; postSig(", ");}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 657 "vtkParse.y"
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] =
        (yyvsp[(2) - (2)].integer) / VTK_PARSE_COUNT_START;
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] =
        (yyvsp[(1) - (2)].integer) + ((yyvsp[(2) - (2)].integer) % VTK_PARSE_COUNT_START);
    }
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 664 "vtkParse.y"
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] =
        (yyvsp[(2) - (2)].integer) / VTK_PARSE_COUNT_START;
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] =
        (yyvsp[(1) - (2)].integer) + ((yyvsp[(2) - (2)].integer) % VTK_PARSE_COUNT_START);
    }
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 671 "vtkParse.y"
    {
      postSig("void (*func)(void *) ");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0;
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = VTK_PARSE_FUNCTION;
    }
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 676 "vtkParse.y"
    { postSig("(&"); }
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 677 "vtkParse.y"
    {
      postSig(") ");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0;
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = VTK_PARSE_UNKNOWN;
    }
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 682 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(2) - (2)].str)); postSig("*"); }
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 683 "vtkParse.y"
    { postSig(")("); }
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 684 "vtkParse.y"
    {
      postSig(")");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0;
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = VTK_PARSE_UNKNOWN;
    }
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 694 "vtkParse.y"
    {postSig("="); postSig((yyvsp[(2) - (2)].str));}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 696 "vtkParse.y"
    {delSig();}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 697 "vtkParse.y"
    {delSig();}
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 698 "vtkParse.y"
    {delSig();}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 699 "vtkParse.y"
    {delSig();}
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 700 "vtkParse.y"
    {delSig();}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 701 "vtkParse.y"
    {delSig();}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 702 "vtkParse.y"
    {delSig();}
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 703 "vtkParse.y"
    {delSig();}
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 704 "vtkParse.y"
    {delSig();}
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 705 "vtkParse.y"
    {delSig();}
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 706 "vtkParse.y"
    {delSig();}
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 707 "vtkParse.y"
    {delSig();}
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 708 "vtkParse.y"
    {delSig();}
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 709 "vtkParse.y"
    {delSig();}
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 710 "vtkParse.y"
    {delSig();}
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 723 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer);}
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 731 "vtkParse.y"
    {(yyval.integer) = 0;}
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 731 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 734 "vtkParse.y"
    { char temp[100]; sprintf(temp,"[%i]",(yyvsp[(1) - (1)].integer));
                   postSig(temp); }
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 736 "vtkParse.y"
    { (yyval.integer) =
                         ((VTK_PARSE_COUNT_START * (yyvsp[(1) - (3)].integer)) |
                          ((VTK_PARSE_POINTER + (yyvsp[(3) - (3)].integer)) &
                           VTK_PARSE_UNQUALIFIED_TYPE)); }
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 741 "vtkParse.y"
    { postSig("[]");
              (yyval.integer) = ((VTK_PARSE_POINTER + (yyvsp[(4) - (4)].integer)) &
                             VTK_PARSE_UNQUALIFIED_TYPE); }
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 745 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_CONST | (yyvsp[(2) - (2)].integer));}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 746 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 748 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_STATIC | (yyvsp[(2) - (2)].integer));}
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 750 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_CONST|VTK_PARSE_STATIC | (yyvsp[(3) - (3)].integer));}
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 752 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 754 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer));}
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 755 "vtkParse.y"
    {postSig(" "); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 756 "vtkParse.y"
    {postSig(" ");}
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 757 "vtkParse.y"
    {(yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 758 "vtkParse.y"
    {postSig(" "); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 759 "vtkParse.y"
    {postSig(" ");}
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 760 "vtkParse.y"
    {(yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 761 "vtkParse.y"
    {postSig(" "); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 762 "vtkParse.y"
    {postSig(" ");}
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 763 "vtkParse.y"
    {(yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 765 "vtkParse.y"
    {postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 766 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (5)].str); postSig(">");}
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 767 "vtkParse.y"
    {postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 768 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (5)].str); postSig(">");}
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 770 "vtkParse.y"
    {postSig(", ");}
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 772 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 773 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 774 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 775 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 778 "vtkParse.y"
    {
             (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (3)].str))+strlen((yyvsp[(3) - (3)].str))+3);
             sprintf((yyval.str), "%s::%s", (yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));
             preScopeSig((yyvsp[(1) - (3)].str));
           }
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 784 "vtkParse.y"
    {
             (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (3)].str))+strlen((yyvsp[(3) - (3)].str))+3);
             sprintf((yyval.str), "%s::%s", (yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));
             preScopeSig((yyvsp[(1) - (3)].str));
           }
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 790 "vtkParse.y"
    {
             (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (3)].str))+strlen((yyvsp[(3) - (3)].str))+3);
             sprintf((yyval.str), "%s::%s", (yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));
             preScopeSig("");
           }
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 805 "vtkParse.y"
    { postSig("&"); (yyval.integer) = VTK_PARSE_REF;}
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 806 "vtkParse.y"
    { postSig("*"); (yyval.integer) = VTK_PARSE_POINTER;}
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 807 "vtkParse.y"
    { postSig("*&"); (yyval.integer) = VTK_PARSE_POINTER_REF;}
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 808 "vtkParse.y"
    { postSig("**"); (yyval.integer) = VTK_PARSE_POINTER_POINTER;}
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 810 "vtkParse.y"
    { postSig("* const&"); (yyval.integer) = VTK_PARSE_POINTER_CONST_REF;}
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 812 "vtkParse.y"
    { postSig("* const*"); (yyval.integer) = VTK_PARSE_POINTER_CONST_POINTER;}
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 813 "vtkParse.y"
    { postSig("const&"); (yyval.integer) = VTK_PARSE_BAD_INDIRECT;}
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 814 "vtkParse.y"
    { postSig("const*"); (yyval.integer) = VTK_PARSE_BAD_INDIRECT;}
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 815 "vtkParse.y"
    { postSig("**"); }
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 816 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_BAD_INDIRECT;}
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 817 "vtkParse.y"
    { postSig("**"); }
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 818 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_BAD_INDIRECT;}
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 819 "vtkParse.y"
    { postSig("const&");}
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 820 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_BAD_INDIRECT;}
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 821 "vtkParse.y"
    { postSig("const*");}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 822 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_BAD_INDIRECT;}
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 824 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 825 "vtkParse.y"
    { postSig("vtkStdString "); (yyval.integer) = VTK_PARSE_STRING;}
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 827 "vtkParse.y"
    { postSig("vtkUnicodeString "); (yyval.integer) = VTK_PARSE_UNICODE_STRING;}
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 828 "vtkParse.y"
    { postSig("ostream "); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 829 "vtkParse.y"
    { postSig("istream "); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 831 "vtkParse.y"
    {
      postSig((yyvsp[(1) - (1)].str));
      postSig(" ");
      (yyval.integer) = VTK_PARSE_UNKNOWN;
    }
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 837 "vtkParse.y"
    {
      postSig((yyvsp[(1) - (1)].str));
      postSig(" ");
      currentFunction->ArgClasses[currentFunction->NumberOfArguments] =
        vtkstrdup((yyvsp[(1) - (1)].str));
      if ((!currentFunction->ReturnClass) &&
          (!currentFunction->NumberOfArguments))
        {
        currentFunction->ReturnClass = vtkstrdup((yyvsp[(1) - (1)].str));
        }
      (yyval.integer) = VTK_PARSE_VTK_OBJECT;
    }
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 851 "vtkParse.y"
    { postSig("void "); (yyval.integer) = VTK_PARSE_VOID;}
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 852 "vtkParse.y"
    { postSig("float "); (yyval.integer) = VTK_PARSE_FLOAT;}
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 853 "vtkParse.y"
    { postSig("double "); (yyval.integer) = VTK_PARSE_DOUBLE;}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 854 "vtkParse.y"
    { postSig("bool "); (yyval.integer) = VTK_PARSE_BOOL;}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 855 "vtkParse.y"
    {postSig("signed char "); (yyval.integer) = VTK_PARSE_SIGNED_CHAR;}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 856 "vtkParse.y"
    { postSig("vtkTypeInt8 "); (yyval.integer) = VTK_PARSE_INT8; }
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 857 "vtkParse.y"
    { postSig("vtkTypeUInt8 "); (yyval.integer) = VTK_PARSE_UINT8; }
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 858 "vtkParse.y"
    { postSig("vtkTypeInt16 "); (yyval.integer) = VTK_PARSE_INT16; }
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 859 "vtkParse.y"
    { postSig("vtkTypeUInt16 "); (yyval.integer) = VTK_PARSE_UINT16; }
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 860 "vtkParse.y"
    { postSig("vtkTypeInt32 "); (yyval.integer) = VTK_PARSE_INT32; }
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 861 "vtkParse.y"
    { postSig("vtkTypeUInt32 "); (yyval.integer) = VTK_PARSE_UINT32; }
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 862 "vtkParse.y"
    { postSig("vtkTypeInt64 "); (yyval.integer) = VTK_PARSE_INT64; }
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 863 "vtkParse.y"
    { postSig("vtkTypeUInt64 "); (yyval.integer) = VTK_PARSE_UINT64; }
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 864 "vtkParse.y"
    { postSig("vtkTypeFloat32 "); (yyval.integer) = VTK_PARSE_FLOAT32; }
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 865 "vtkParse.y"
    { postSig("vtkTypeFloat64 "); (yyval.integer) = VTK_PARSE_FLOAT64; }
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 866 "vtkParse.y"
    {postSig("unsigned ");}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 867 "vtkParse.y"
    { (yyval.integer) = (VTK_PARSE_UNSIGNED | (yyvsp[(3) - (3)].integer));}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 868 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 871 "vtkParse.y"
    { postSig("char "); (yyval.integer) = VTK_PARSE_CHAR;}
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 872 "vtkParse.y"
    { postSig("int "); (yyval.integer) = VTK_PARSE_INT;}
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 873 "vtkParse.y"
    { postSig("short "); (yyval.integer) = VTK_PARSE_SHORT;}
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 874 "vtkParse.y"
    { postSig("long "); (yyval.integer) = VTK_PARSE_LONG;}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 875 "vtkParse.y"
    { postSig("vtkIdType "); (yyval.integer) = VTK_PARSE_ID_TYPE;}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 876 "vtkParse.y"
    { postSig("long long "); (yyval.integer) = VTK_PARSE_LONG_LONG;}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 877 "vtkParse.y"
    { postSig("__int64 "); (yyval.integer) = VTK_PARSE___INT64;}
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 882 "vtkParse.y"
    {
      if (mainClass)
        {
        data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup((yyvsp[(2) - (2)].str));
        data.NumberOfSuperClasses++;
        }
    }
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 890 "vtkParse.y"
    {
      if (mainClass)
        {
        data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup((yyvsp[(2) - (2)].str));
        data.NumberOfSuperClasses++;
        }
    }
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 898 "vtkParse.y"
    {in_public = 0; in_protected = 0;}
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 899 "vtkParse.y"
    {in_public = 1; in_protected = 0;}
    break;

  case 294:

/* Line 1455 of yacc.c  */
#line 900 "vtkParse.y"
    {in_public = 0; in_protected = 0;}
    break;

  case 295:

/* Line 1455 of yacc.c  */
#line 901 "vtkParse.y"
    {in_public = 0; in_protected = 1;}
    break;

  case 296:

/* Line 1455 of yacc.c  */
#line 903 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 904 "vtkParse.y"
    {(yyval.str) = (yyvsp[(2) - (2)].str);}
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 905 "vtkParse.y"
    {(yyval.str) = (char *)malloc(strlen((yyvsp[(2) - (2)].str))+2);
                        sprintf((yyval.str), "-%s", (yyvsp[(2) - (2)].str)); }
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 907 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 908 "vtkParse.y"
    {(yyval.str) = (yyvsp[(2) - (3)].str);}
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 910 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 302:

/* Line 1455 of yacc.c  */
#line 911 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 303:

/* Line 1455 of yacc.c  */
#line 912 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 304:

/* Line 1455 of yacc.c  */
#line 913 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 305:

/* Line 1455 of yacc.c  */
#line 914 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 915 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 307:

/* Line 1455 of yacc.c  */
#line 919 "vtkParse.y"
    {preSig("void Set"); postSig("("); }
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 920 "vtkParse.y"
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

  case 309:

/* Line 1455 of yacc.c  */
#line 930 "vtkParse.y"
    {postSig("Get");}
    break;

  case 310:

/* Line 1455 of yacc.c  */
#line 930 "vtkParse.y"
    {postSig("();"); invertSig = 1;}
    break;

  case 311:

/* Line 1455 of yacc.c  */
#line 932 "vtkParse.y"
    {
   sprintf(temps,"Get%s",(yyvsp[(4) - (8)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (yyvsp[(7) - (8)].integer);
   output_function();
   }
    break;

  case 312:

/* Line 1455 of yacc.c  */
#line 939 "vtkParse.y"
    {preSig("void Set");}
    break;

  case 313:

/* Line 1455 of yacc.c  */
#line 940 "vtkParse.y"
    {
   postSig("(char *);");
   sprintf(temps,"Set%s",(yyvsp[(4) - (5)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = VTK_PARSE_CHAR_PTR;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = VTK_PARSE_VOID;
   output_function();
   }
    break;

  case 314:

/* Line 1455 of yacc.c  */
#line 950 "vtkParse.y"
    {preSig("char *Get");}
    break;

  case 315:

/* Line 1455 of yacc.c  */
#line 951 "vtkParse.y"
    {
   postSig("();");
   sprintf(temps,"Get%s",(yyvsp[(4) - (5)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = VTK_PARSE_CHAR_PTR;
   output_function();
   }
    break;

  case 316:

/* Line 1455 of yacc.c  */
#line 960 "vtkParse.y"
    {preSig("void Set"); postSig("("); }
    break;

  case 317:

/* Line 1455 of yacc.c  */
#line 961 "vtkParse.y"
    {postSig(");"); openSig = 0;}
    break;

  case 318:

/* Line 1455 of yacc.c  */
#line 962 "vtkParse.y"
    {
   char *local = vtkstrdup(currentFunction->Signature);
   sscanf(currentFunction->Signature, "%*s %*s(%s);", local);
   sprintf(temps,"Set%s",(yyvsp[(3) - (10)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (yyvsp[(6) - (10)].integer);
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = VTK_PARSE_VOID;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%s Get%sMinValue();",local,(yyvsp[(3) - (10)].str));
   sprintf(temps,"Get%sMinValue",(yyvsp[(3) - (10)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (yyvsp[(6) - (10)].integer);
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%s Get%sMaxValue();",local,(yyvsp[(3) - (10)].str));
   sprintf(temps,"Get%sMaxValue",(yyvsp[(3) - (10)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (yyvsp[(6) - (10)].integer);
   output_function();
   }
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 992 "vtkParse.y"
    {preSig("void Set"); postSig("("); }
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 993 "vtkParse.y"
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

  case 321:

/* Line 1455 of yacc.c  */
#line 1003 "vtkParse.y"
    {postSig("*Get");}
    break;

  case 322:

/* Line 1455 of yacc.c  */
#line 1004 "vtkParse.y"
    {postSig("();"); invertSig = 1;}
    break;

  case 323:

/* Line 1455 of yacc.c  */
#line 1005 "vtkParse.y"
    {
   sprintf(temps,"Get%s",(yyvsp[(4) - (8)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = VTK_PARSE_VTK_OBJECT_PTR;
   output_function();
   }
    break;

  case 324:

/* Line 1455 of yacc.c  */
#line 1013 "vtkParse.y"
    {preSig("void "); postSig("On();"); openSig = 0; }
    break;

  case 325:

/* Line 1455 of yacc.c  */
#line 1015 "vtkParse.y"
    {
   sprintf(temps,"%sOn",(yyvsp[(3) - (7)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = VTK_PARSE_VOID;
   output_function();
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void %sOff();",(yyvsp[(3) - (7)].str));
   sprintf(temps,"%sOff",(yyvsp[(3) - (7)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   output_function();
   }
    break;

  case 326:

/* Line 1455 of yacc.c  */
#line 1030 "vtkParse.y"
    {
     free(currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 1035 "vtkParse.y"
    {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s(%s, %s);",(yyvsp[(3) - (7)].str),
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
   sprintf(currentFunction->Signature,"void Set%s(%s a[2]);",(yyvsp[(3) - (7)].str),
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (VTK_PARSE_POINTER | (yyvsp[(6) - (7)].integer));
   currentFunction->ArgCounts[0] = 2;
   output_function();
   }
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 1060 "vtkParse.y"
    {
     free(currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 329:

/* Line 1455 of yacc.c  */
#line 1065 "vtkParse.y"
    {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s();",local, (yyvsp[(3) - (7)].str));
   sprintf(temps,"Get%s",(yyvsp[(3) - (7)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (VTK_PARSE_POINTER | (yyvsp[(6) - (7)].integer));
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 2;
   output_function();
   }
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 1077 "vtkParse.y"
    {
     free(currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 331:

/* Line 1455 of yacc.c  */
#line 1082 "vtkParse.y"
    {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s(%s, %s, %s);",
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
   sprintf(currentFunction->Signature,"void Set%s(%s a[3]);",(yyvsp[(3) - (7)].str),
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (VTK_PARSE_POINTER | (yyvsp[(6) - (7)].integer));
   currentFunction->ArgCounts[0] = 3;
   output_function();
   }
    break;

  case 332:

/* Line 1455 of yacc.c  */
#line 1109 "vtkParse.y"
    {
     free(currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 333:

/* Line 1455 of yacc.c  */
#line 1114 "vtkParse.y"
    {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s();",local, (yyvsp[(3) - (7)].str));
   sprintf(temps,"Get%s",(yyvsp[(3) - (7)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (VTK_PARSE_POINTER | (yyvsp[(6) - (7)].integer));
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 3;
   output_function();
   }
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 1126 "vtkParse.y"
    {
     free(currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 335:

/* Line 1455 of yacc.c  */
#line 1131 "vtkParse.y"
    {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s(%s, %s, %s, %s);",
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
   sprintf(currentFunction->Signature,"void Set%s(%s a[4]);",(yyvsp[(3) - (7)].str),
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (VTK_PARSE_POINTER | (yyvsp[(6) - (7)].integer));
   currentFunction->ArgCounts[0] = 4;
   output_function();
   }
    break;

  case 336:

/* Line 1455 of yacc.c  */
#line 1160 "vtkParse.y"
    {
     free(currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 337:

/* Line 1455 of yacc.c  */
#line 1165 "vtkParse.y"
    {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s();",local, (yyvsp[(3) - (7)].str));
   sprintf(temps,"Get%s",(yyvsp[(3) - (7)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (VTK_PARSE_POINTER | (yyvsp[(6) - (7)].integer));
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 4;
   output_function();
   }
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 1177 "vtkParse.y"
    {
     free(currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 1182 "vtkParse.y"
    {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s(%s, %s, %s, %s, %s, %s);",
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
   sprintf(currentFunction->Signature,"void Set%s(%s a[6]);",(yyvsp[(3) - (7)].str),
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (VTK_PARSE_POINTER | (yyvsp[(6) - (7)].integer));
   currentFunction->ArgCounts[0] = 6;
   output_function();
   }
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 1215 "vtkParse.y"
    {
     free(currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 341:

/* Line 1455 of yacc.c  */
#line 1220 "vtkParse.y"
    {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s();",local, (yyvsp[(3) - (7)].str));
   sprintf(temps,"Get%s",(yyvsp[(3) - (7)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (VTK_PARSE_POINTER | (yyvsp[(6) - (7)].integer));
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 6;
   output_function();
   }
    break;

  case 342:

/* Line 1455 of yacc.c  */
#line 1232 "vtkParse.y"
    {
      free(currentFunction->Signature);
      currentFunction->Signature = NULL;
      }
    break;

  case 343:

/* Line 1455 of yacc.c  */
#line 1237 "vtkParse.y"
    {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s(%s [%s]);",(yyvsp[(3) - (9)].str),
      local, (yyvsp[(8) - (9)].str));
     sprintf(temps,"Set%s",(yyvsp[(3) - (9)].str));
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->ReturnType = VTK_PARSE_VOID;
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = (VTK_PARSE_POINTER | (yyvsp[(6) - (9)].integer));
     currentFunction->ArgCounts[0] = atol((yyvsp[(8) - (9)].str));
     output_function();
   }
    break;

  case 344:

/* Line 1455 of yacc.c  */
#line 1250 "vtkParse.y"
    {
     free(currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 345:

/* Line 1455 of yacc.c  */
#line 1255 "vtkParse.y"
    {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s();",local, (yyvsp[(3) - (9)].str));
   sprintf(temps,"Get%s",(yyvsp[(3) - (9)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (VTK_PARSE_POINTER | (yyvsp[(6) - (9)].integer));
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = atol((yyvsp[(8) - (9)].str));
   output_function();
   }
    break;

  case 346:

/* Line 1455 of yacc.c  */
#line 1267 "vtkParse.y"
    {
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate();",
       (yyvsp[(3) - (4)].str));

     sprintf(temps,"Get%sCoordinate",(yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = VTK_PARSE_VTK_OBJECT_PTR;
     currentFunction->ReturnClass = vtkstrdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s(double, double);",
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
     sprintf(currentFunction->Signature,"void Set%s(double a[2]);",
       (yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = VTK_PARSE_DOUBLE_PTR;
     currentFunction->ArgCounts[0] = 2;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s();", (yyvsp[(3) - (4)].str));
     sprintf(temps,"Get%s",(yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = VTK_PARSE_DOUBLE_PTR;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 2;
     output_function();
   }
    break;

  case 347:

/* Line 1455 of yacc.c  */
#line 1314 "vtkParse.y"
    {
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate();",
       (yyvsp[(3) - (4)].str));

     sprintf(temps,"Get%sCoordinate",(yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = VTK_PARSE_VTK_OBJECT_PTR;
     currentFunction->ReturnClass = vtkstrdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s(double, double, double);",
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
     sprintf(currentFunction->Signature,"void Set%s(double a[3]);",
       (yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = VTK_PARSE_DOUBLE_PTR;
     currentFunction->ArgCounts[0] = 3;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s();", (yyvsp[(3) - (4)].str));
     sprintf(temps,"Get%s",(yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = VTK_PARSE_DOUBLE_PTR;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 3;
     output_function();
   }
    break;

  case 348:

/* Line 1455 of yacc.c  */
#line 1363 "vtkParse.y"
    {
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "const char *GetClassName();");
   sprintf(temps,"GetClassName");
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR);
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,
           "int IsA(const char *name);");
   sprintf(temps,"IsA");
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR);
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = VTK_PARSE_INT;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "%s *NewInstance();",
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
     sprintf(currentFunction->Signature, "%s *SafeDownCast(vtkObject* o);",
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

  case 349:

/* Line 1455 of yacc.c  */
#line 1414 "vtkParse.y"
    {
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "const char *GetClassName();");
   sprintf(temps,"GetClassName");
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR);
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,
           "int IsA(const char *name);");
   sprintf(temps,"IsA");
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR);
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = VTK_PARSE_INT;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "%s *NewInstance();",
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
     sprintf(currentFunction->Signature, "%s *SafeDownCast(vtkObject* o);",
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

  case 350:

/* Line 1455 of yacc.c  */
#line 1470 "vtkParse.y"
    { (yyval.str) = "operator()"; }
    break;

  case 351:

/* Line 1455 of yacc.c  */
#line 1471 "vtkParse.y"
    { (yyval.str) = "operator[]"; }
    break;

  case 352:

/* Line 1455 of yacc.c  */
#line 1472 "vtkParse.y"
    { (yyval.str) = "operator new[]"; }
    break;

  case 353:

/* Line 1455 of yacc.c  */
#line 1473 "vtkParse.y"
    { (yyval.str) = "operator delete[]"; }
    break;

  case 355:

/* Line 1455 of yacc.c  */
#line 1476 "vtkParse.y"
    { (yyval.str) = "operator="; }
    break;

  case 356:

/* Line 1455 of yacc.c  */
#line 1477 "vtkParse.y"
    { (yyval.str) = "operator*"; }
    break;

  case 357:

/* Line 1455 of yacc.c  */
#line 1477 "vtkParse.y"
    { (yyval.str) = "operator/"; }
    break;

  case 358:

/* Line 1455 of yacc.c  */
#line 1478 "vtkParse.y"
    { (yyval.str) = "operator-"; }
    break;

  case 359:

/* Line 1455 of yacc.c  */
#line 1478 "vtkParse.y"
    { (yyval.str) = "operator+"; }
    break;

  case 360:

/* Line 1455 of yacc.c  */
#line 1479 "vtkParse.y"
    { (yyval.str) = "operator!"; }
    break;

  case 361:

/* Line 1455 of yacc.c  */
#line 1479 "vtkParse.y"
    { (yyval.str) = "operator~"; }
    break;

  case 362:

/* Line 1455 of yacc.c  */
#line 1480 "vtkParse.y"
    { (yyval.str) = "operator,"; }
    break;

  case 363:

/* Line 1455 of yacc.c  */
#line 1480 "vtkParse.y"
    { (yyval.str) = "operator<"; }
    break;

  case 364:

/* Line 1455 of yacc.c  */
#line 1481 "vtkParse.y"
    { (yyval.str) = "operator>"; }
    break;

  case 365:

/* Line 1455 of yacc.c  */
#line 1481 "vtkParse.y"
    { (yyval.str) = "operator&"; }
    break;

  case 366:

/* Line 1455 of yacc.c  */
#line 1482 "vtkParse.y"
    { (yyval.str) = "operator|"; }
    break;

  case 367:

/* Line 1455 of yacc.c  */
#line 1482 "vtkParse.y"
    { (yyval.str) = "operator^"; }
    break;

  case 368:

/* Line 1455 of yacc.c  */
#line 1483 "vtkParse.y"
    { (yyval.str) = "operator%"; }
    break;

  case 369:

/* Line 1455 of yacc.c  */
#line 1484 "vtkParse.y"
    { (yyval.str) = "operator new"; }
    break;

  case 370:

/* Line 1455 of yacc.c  */
#line 1485 "vtkParse.y"
    { (yyval.str) = "operator delete"; }
    break;

  case 371:

/* Line 1455 of yacc.c  */
#line 1486 "vtkParse.y"
    { (yyval.str) = "operator<<="; }
    break;

  case 372:

/* Line 1455 of yacc.c  */
#line 1487 "vtkParse.y"
    { (yyval.str) = "operator>>="; }
    break;

  case 373:

/* Line 1455 of yacc.c  */
#line 1488 "vtkParse.y"
    { (yyval.str) = "operator<<"; }
    break;

  case 374:

/* Line 1455 of yacc.c  */
#line 1489 "vtkParse.y"
    { (yyval.str) = "operator>>"; }
    break;

  case 375:

/* Line 1455 of yacc.c  */
#line 1490 "vtkParse.y"
    { (yyval.str) = "operator->*"; }
    break;

  case 376:

/* Line 1455 of yacc.c  */
#line 1491 "vtkParse.y"
    { (yyval.str) = "operator->"; }
    break;

  case 377:

/* Line 1455 of yacc.c  */
#line 1492 "vtkParse.y"
    { (yyval.str) = "operator+="; }
    break;

  case 378:

/* Line 1455 of yacc.c  */
#line 1493 "vtkParse.y"
    { (yyval.str) = "operator-="; }
    break;

  case 379:

/* Line 1455 of yacc.c  */
#line 1494 "vtkParse.y"
    { (yyval.str) = "operator*="; }
    break;

  case 380:

/* Line 1455 of yacc.c  */
#line 1495 "vtkParse.y"
    { (yyval.str) = "operator/="; }
    break;

  case 381:

/* Line 1455 of yacc.c  */
#line 1496 "vtkParse.y"
    { (yyval.str) = "operator%="; }
    break;

  case 382:

/* Line 1455 of yacc.c  */
#line 1497 "vtkParse.y"
    { (yyval.str) = "operator++"; }
    break;

  case 383:

/* Line 1455 of yacc.c  */
#line 1498 "vtkParse.y"
    { (yyval.str) = "operator--"; }
    break;

  case 384:

/* Line 1455 of yacc.c  */
#line 1499 "vtkParse.y"
    { (yyval.str) = "operator&="; }
    break;

  case 385:

/* Line 1455 of yacc.c  */
#line 1500 "vtkParse.y"
    { (yyval.str) = "operator|="; }
    break;

  case 386:

/* Line 1455 of yacc.c  */
#line 1501 "vtkParse.y"
    { (yyval.str) = "operator^="; }
    break;

  case 387:

/* Line 1455 of yacc.c  */
#line 1502 "vtkParse.y"
    {(yyval.str) = "operator&&=";}
    break;

  case 388:

/* Line 1455 of yacc.c  */
#line 1503 "vtkParse.y"
    {(yyval.str) = "operator||=";}
    break;

  case 389:

/* Line 1455 of yacc.c  */
#line 1504 "vtkParse.y"
    { (yyval.str) = "operator&&"; }
    break;

  case 390:

/* Line 1455 of yacc.c  */
#line 1505 "vtkParse.y"
    { (yyval.str) = "operator||"; }
    break;

  case 391:

/* Line 1455 of yacc.c  */
#line 1506 "vtkParse.y"
    { (yyval.str) = "operator=="; }
    break;

  case 392:

/* Line 1455 of yacc.c  */
#line 1507 "vtkParse.y"
    { (yyval.str) = "operator!="; }
    break;

  case 393:

/* Line 1455 of yacc.c  */
#line 1508 "vtkParse.y"
    { (yyval.str) = "operator<="; }
    break;

  case 394:

/* Line 1455 of yacc.c  */
#line 1509 "vtkParse.y"
    { (yyval.str) = "operator>="; }
    break;



/* Line 1455 of yacc.c  */
#line 5514 "vtkParse.tab.c"
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


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;    /* Each real token shifted decrements this.  */

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
#line 1546 "vtkParse.y"

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

/* check whether this is the class we are looking for */
void start_class(const char *classname)
{
  if (!strcmp(data.ClassName, classname))
    {
    mainClass = 1;
    currentFunction = data.Functions;
    data.NumberOfFunctions = 0;
    }
  else
    {
    mainClass = 0;
    currentFunction = &throwAwayFunction;
    }
  InitFunction(currentFunction);
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

/* reject the function, do not output it */
void reject_function()
{
  InitFunction(currentFunction);
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
    if (mainClass)
      {
      data.HasDelete = 1;
      }
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
    switch (currentFunction->ArgTypes[i] & VTK_PARSE_INDIRECT)
      {
      case VTK_PARSE_ARRAY_2D:
      case VTK_PARSE_ARRAY_3D:
      case VTK_PARSE_BAD_INDIRECT:
        currentFunction->ArrayFailure = 1;
        break;
      }
    }

  if (HaveComment)
    {
    currentFunction->Comment = vtkstrdup(CommentText);
    }

  if (mainClass)
    {
    data.NumberOfFunctions++;
    currentFunction = data.Functions + data.NumberOfFunctions;
    }

  InitFunction(currentFunction);
}

extern void vtkParseOutput(FILE *,FileInfo *);

int main(int argc,char *argv[])
{
  int i, j;
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

  /* The class name should be the file name */
  i = strlen(data.FileName);
  j = i;
  while (i > 0)
    {
    --i;
    if (data.FileName[i] == '.')
      {
      j = i;
      }
    if (data.FileName[i] == '/' || data.FileName[i] == '\\')
      {
      i++;
      break;
      }
    }
  data.ClassName = (char *)malloc(j-i + 1);
  strncpy(data.ClassName, &data.FileName[i], j-i);
  data.ClassName[j-i] = '\0';

  /* This will be set to 1 while parsing the main class */
  mainClass = 0;

  currentFunction = &throwAwayFunction;

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

