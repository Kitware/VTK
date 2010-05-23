
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

/* Map from the type enumeration in vtkType.h to the VTK wrapping type
   system number for the type. */

#include "vtkParse.h"
#include "vtkType.h"

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
  VTK_PARSE_UNSIGNED___INT64,   /* VTK_UNSIGNED___INT64   19 */
  0,                            /* VTK_VARIANT            20 */
  0,                            /* VTK_OBJECT             21 */
  VTK_PARSE_UNICODE_STRING      /* VTK_UNICODE_STRING     22 */
  };

/* Define some constants to simplify references to the table lookup in
   the type_primitive production rule code.  */
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

/* the tokenizer */
int yylex(void);

/* global variables */
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
int sigClosed;
int sigMark[10];
int sigMarkDepth = 0;
unsigned int sigAllocatedLength;
int mainClass;
char *currentId = 0;

void start_class(const char *classname);
void output_function(void);
void reject_function(void);

void outputSetVectorMacro(
  const char *var, int argType, const char *typeText, int n);
void outputGetVectorMacro(
  const char *var, int argType, const char *typeText, int n);

/* duplicate a string */
char *vtkstrdup(const char *in)
{
  char *res = NULL;
  if (in)
    {
    res = (char *)malloc(strlen(in)+1);
    strcpy(res,in);
    }
  return res;
}

/* reallocate Signature if "arg" cannot be appended */
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

/* close the signature, i.e. allow no more additions to it */
void closeSig()
{
  sigClosed = 1;
}

/* re-open the signature */
void openSig()
{
  sigClosed = 0;
}

/* insert text at the beginning of the signature */
void preSig(const char *arg)
{
  if (!currentFunction->Signature)
    {
    currentFunction->Signature = (char*)malloc(2048);
    sigAllocatedLength = 2048;
    strcpy(currentFunction->Signature, arg);
    }
  else if (!sigClosed)
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

/* append text to the end of the signature */
void postSig(const char *arg)
{
  if (!currentFunction->Signature)
    {
    currentFunction->Signature = (char*)malloc(2048);
    sigAllocatedLength = 2048;
    strcpy(currentFunction->Signature, arg);
    }
  else if (!sigClosed)
    {
    int m, n;
    char *cp;
    checkSigSize(arg);
    cp = currentFunction->Signature;
    m = strlen(cp);
    n = strlen(arg);
    strncpy(&cp[m], arg, n+1);
    }
}

/* prepend a scope:: to the ID at the end of the signature */
void preScopeSig(const char *arg)
{
  if (!currentFunction->Signature)
    {
    currentFunction->Signature = (char*)malloc(2048);
    sigAllocatedLength = 2048;
    strcpy(currentFunction->Signature, arg);
    }
  else if (!sigClosed)
    {
    int i, m, n, depth;
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
            cp[i-1] == '_' || cp[i-1] == ':' ||
            cp[i-1] == '>'))
      {
      i--;
      if (cp[i] == '>')
        {
        depth = 1;
        while (i > 0)
          {
          i--;
          if (cp[i] == '<')
            {
            if (--depth == 0)
              {
              break;
              }
            }
          if (cp[i] == '>')
            {
            depth++;
            }
          }
        }
      }

    memmove(&cp[i+n+2], &cp[i], m+1);
    strncpy(&cp[i], arg, n);
    strncpy(&cp[i+n], "::", 2);
    }
}

/* set a mark in the signature for later operations */
void markSig()
{
  sigMark[sigMarkDepth] = 0;
  if (currentFunction->Signature)
    {
    sigMark[sigMarkDepth] = strlen(currentFunction->Signature);
    }
  sigMarkDepth++;
}

/* get the contents of the sig from the mark, and clear the mark */
const char *copySig()
{
  const char *cp = NULL;
  if (sigMarkDepth > 0)
    {
    sigMarkDepth--;
    }
  if (currentFunction->Signature)
    {
    cp = &currentFunction->Signature[sigMark[sigMarkDepth]];
    }
  return cp;
}

/* swap the signature text using the mark as the radix */
void swapSig()
{
  if (sigMarkDepth > 0)
    {
    sigMarkDepth--;
    }
  if (currentFunction->Signature && sigMark[sigMarkDepth] > 0)
    {
    int i, m, n, nn;
    char c;
    char *cp;
    cp = currentFunction->Signature;
    n = strlen(cp);
    m = sigMark[sigMarkDepth];
    nn = m/2;
    for (i = 0; i < nn; i++)
      {
      c = cp[i]; cp[i] = cp[m-i-1]; cp[m-i-1] = c;
      }
    nn = (n-m)/2;
    for (i = 0; i < nn; i++)
      {
      c = cp[i+m]; cp[i+m] = cp[n-i-1]; cp[n-i-1] = c;
      }
    nn = n/2;
    for (i = 0; i < nn; i++)
      {
      c = cp[i]; cp[i] = cp[n-i-1]; cp[n-i-1] = c;
      }
    }
}

/* chop the last char from the signature */
void chopSig(void)
{
  if (currentFunction->Signature)
    {
    int n = strlen(currentFunction->Signature);
    if (n > 0)
      {
      currentFunction->Signature[n-1] = '\0';
      }
    }
}

/* delete the signature */
void delSig(void)
{
  if (currentFunction->Signature)
    {
    free(currentFunction->Signature);
    currentFunction->Signature = NULL;
    }
}

/* mark this signature as legacy */
void legacySig(void)
{
  currentFunction->IsLegacy = 1;
}

/* clear the current Id */
void clearTypeId(void)
{
  currentId = NULL;
}

/* set the current Id, it is sticky until cleared */
void setTypeId(const char *text)
{
  static char static_text[2048];
  if (currentId == NULL)
    {
    currentId = static_text;
    strcpy(static_text, text);
    }
}

/* set the signature and type together */
void typeSig(const char *text)
{
  postSig(text);
  postSig(" ");

  if (currentId == 0)
    {
    setTypeId(text);
    }
  else if (currentId[0] == 'u' && !strcmp(currentId, "unsigned"))
    {
    currentId[8] = ' ';
    strcpy(&currentId[9], text);
    }
}

/* return the current Id and clear it */
const char *getTypeId()
{
  return currentId;
}



/* Line 189 of yacc.c  */
#line 464 "vtkParse.tab.c"

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
     IdType = 335,
     StdString = 336,
     UnicodeString = 337,
     TypeInt8 = 338,
     TypeUInt8 = 339,
     TypeInt16 = 340,
     TypeUInt16 = 341,
     TypeInt32 = 342,
     TypeUInt32 = 343,
     TypeInt64 = 344,
     TypeUInt64 = 345,
     TypeFloat32 = 346,
     TypeFloat64 = 347,
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
#line 405 "vtkParse.y"

  char *str;
  int   integer;



/* Line 214 of yacc.c  */
#line 737 "vtkParse.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 749 "vtkParse.tab.c"

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
#   define YYCOPY(To, From, Count)                \
      do                                        \
        {                                        \
          YYSIZE_T yyi;                                \
          for (yyi = 0; yyi < (Count); yyi++)        \
            (To)[yyi] = (From)[yyi];                \
        }                                        \
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                                \
    do                                                                        \
      {                                                                        \
        YYSIZE_T yynewbytes;                                                \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                        \
        Stack = &yyptr->Stack_alloc;                                        \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                                \
      }                                                                        \
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  133
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2442

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  139
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  153
/* YYNRULES -- Number of rules.  */
#define YYNRULES  452
/* YYNRULES -- Number of states.  */
#define YYNSTATES  816

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   370

#define YYTRANSLATE(YYX)                                                \
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
      24,    32,    33,    44,    45,    46,    50,    53,    55,    58,
      60,    62,    64,    66,    68,    70,    72,    75,    77,    79,
      82,    86,    90,    93,    97,   100,   104,   108,   111,   115,
     118,   124,   127,   129,   137,   144,   145,   147,   151,   153,
     157,   159,   161,   163,   165,   167,   169,   171,   173,   176,
     180,   184,   186,   188,   190,   192,   194,   196,   198,   200,
     202,   204,   206,   214,   221,   225,   228,   232,   234,   239,
     243,   248,   257,   265,   275,   285,   288,   291,   294,   297,
     301,   304,   307,   311,   312,   318,   320,   321,   326,   329,
     332,   334,   336,   338,   343,   346,   350,   352,   355,   359,
     364,   368,   370,   373,   377,   382,   386,   387,   388,   397,
     398,   402,   405,   406,   407,   415,   416,   420,   423,   426,
     429,   430,   432,   433,   439,   440,   441,   451,   452,   456,
     457,   463,   464,   468,   469,   473,   478,   480,   481,   487,
     489,   491,   494,   496,   498,   500,   505,   509,   510,   512,
     514,   518,   519,   520,   523,   525,   527,   528,   533,   536,
     537,   542,   544,   545,   551,   552,   553,   563,   564,   566,
     567,   569,   572,   577,   583,   588,   594,   599,   605,   609,
     614,   617,   621,   627,   633,   640,   649,   658,   660,   664,
     666,   670,   673,   678,   681,   684,   685,   687,   688,   692,
     697,   700,   702,   705,   709,   711,   714,   716,   717,   721,
     723,   724,   728,   731,   732,   737,   738,   744,   745,   751,
     753,   754,   759,   761,   763,   765,   767,   771,   775,   779,
     781,   783,   786,   789,   792,   795,   797,   799,   800,   805,
     806,   811,   812,   816,   817,   821,   823,   825,   827,   829,
     831,   833,   835,   837,   839,   841,   843,   845,   847,   849,
     851,   853,   855,   857,   859,   861,   863,   865,   866,   870,
     872,   874,   876,   878,   880,   882,   884,   886,   887,   890,
     893,   894,   900,   901,   903,   905,   907,   909,   912,   915,
     917,   921,   923,   925,   927,   929,   931,   933,   934,   942,
     943,   944,   945,   955,   956,   962,   963,   969,   970,   971,
     982,   983,   991,   992,   993,   994,  1004,  1011,  1012,  1020,
    1021,  1029,  1030,  1038,  1039,  1047,  1048,  1056,  1057,  1065,
    1066,  1074,  1075,  1083,  1084,  1094,  1095,  1105,  1110,  1115,
    1122,  1130,  1133,  1136,  1140,  1144,  1146,  1148,  1150,  1152,
    1154,  1156,  1158,  1160,  1162,  1164,  1166,  1168,  1170,  1172,
    1174,  1176,  1178,  1180,  1182,  1184,  1186,  1188,  1190,  1192,
    1194,  1196,  1198,  1200,  1202,  1204,  1206,  1208,  1210,  1212,
    1214,  1216,  1218,  1220,  1222,  1224,  1226,  1228,  1229,  1232,
    1233,  1236,  1237,  1240,  1242,  1244,  1246,  1248,  1250,  1252,
    1254,  1256,  1258,  1260,  1262,  1264,  1266,  1268,  1270,  1272,
    1274,  1276,  1278,  1280,  1282,  1284,  1286,  1288,  1290,  1292,
    1294,  1296,  1298,  1300,  1302,  1304,  1306,  1308,  1310,  1312,
    1314,  1316,  1318,  1320,  1322,  1326,  1330,  1334,  1338,  1342,
    1346,  1347,  1350
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     140,     0,    -1,   281,   141,    -1,    -1,   142,   281,   141,
      -1,   167,   245,    -1,   167,    38,   245,    -1,   167,   143,
      -1,   143,    -1,    -1,     3,   204,   144,   249,   116,   146,
     117,    -1,    -1,     3,   204,   118,   236,   119,   145,   249,
     116,   146,   117,    -1,    -1,    -1,   147,   148,   146,    -1,
     252,   120,    -1,   220,    -1,    39,   220,    -1,   150,    -1,
     149,    -1,   159,    -1,   158,    -1,   160,    -1,   164,    -1,
     162,    -1,    37,   162,    -1,   161,    -1,    29,    -1,   175,
     205,    -1,    37,   175,   205,    -1,    38,   175,   205,    -1,
     166,   205,    -1,    38,   166,   205,    -1,   174,   205,    -1,
      37,   174,   205,    -1,    38,   174,   205,    -1,   165,   205,
      -1,    38,   165,   205,    -1,   173,   205,    -1,   115,   121,
     281,   122,   123,    -1,   255,   123,    -1,   255,    -1,    27,
     204,   116,   151,   117,   282,   123,    -1,    27,   116,   151,
     117,   282,   123,    -1,    -1,   152,    -1,   152,   124,   151,
      -1,   204,    -1,   204,   125,   155,    -1,   154,    -1,   204,
      -1,   239,    -1,   233,    -1,    10,    -1,    11,    -1,    13,
      -1,   153,    -1,   156,   155,    -1,   153,   157,   155,    -1,
     121,   155,   122,    -1,   126,    -1,   127,    -1,   128,    -1,
     126,    -1,   127,    -1,   129,    -1,   130,    -1,   131,    -1,
     132,    -1,   133,    -1,   134,    -1,    28,   204,   116,   281,
     117,   282,   123,    -1,    28,   116,   281,   117,   282,   123,
      -1,    44,   282,   123,    -1,   167,   162,    -1,     3,   204,
     163,    -1,   123,    -1,   116,   281,   117,   123,    -1,   120,
     282,   123,    -1,    42,   228,   224,   123,    -1,    42,     3,
     204,   116,   281,   117,   204,   123,    -1,    42,     3,   116,
     281,   117,   204,   123,    -1,    42,   228,    52,   204,   122,
     121,   282,   122,   123,    -1,    42,   228,    53,   204,   122,
     121,   282,   122,   123,    -1,    42,   150,    -1,    42,   149,
      -1,    42,   159,    -1,    42,   158,    -1,    42,    47,   123,
      -1,   167,   174,    -1,   167,   175,    -1,    40,   118,   119,
      -1,    -1,    40,   118,   168,   169,   119,    -1,   171,    -1,
      -1,   171,   124,   170,   169,    -1,   172,   217,    -1,   167,
     217,    -1,    41,    -1,     3,    -1,    14,    -1,    49,   121,
     174,   122,    -1,   128,   199,    -1,     7,   128,   199,    -1,
     192,    -1,   228,   184,    -1,   228,    31,   184,    -1,     7,
     228,    31,   184,    -1,     7,   228,   184,    -1,   176,    -1,
     228,   179,    -1,   228,    31,   179,    -1,     7,   228,    31,
     179,    -1,     7,   228,   179,    -1,    -1,    -1,    35,   228,
     121,   177,   208,   122,   178,   187,    -1,    -1,   181,   180,
     187,    -1,   181,   186,    -1,    -1,    -1,    35,   278,   182,
     121,   183,   208,   122,    -1,    -1,   188,   185,   187,    -1,
     188,   186,    -1,   125,    10,    -1,    34,    10,    -1,    -1,
      31,    -1,    -1,   204,   121,   189,   208,   122,    -1,    -1,
      -1,   204,   118,   190,   236,   119,   121,   191,   208,   122,
      -1,    -1,   194,   193,   196,    -1,    -1,   204,   121,   195,
     208,   122,    -1,    -1,   120,   198,   197,    -1,    -1,   124,
     198,   197,    -1,   204,   121,   281,   122,    -1,   200,    -1,
      -1,   204,   121,   201,   208,   122,    -1,    31,    -1,    46,
      -1,    46,    38,    -1,    45,    -1,     8,    -1,   123,    -1,
     116,   281,   117,   123,    -1,   116,   281,   117,    -1,    -1,
     207,    -1,   212,    -1,   212,   124,   207,    -1,    -1,    -1,
     209,   210,    -1,    78,    -1,   212,    -1,    -1,   212,   124,
     211,   210,    -1,   228,   225,    -1,    -1,   228,   224,   213,
     218,    -1,    47,    -1,    -1,   228,    53,   214,   217,   122,
      -1,    -1,    -1,   228,    52,   215,   217,   122,   121,   216,
     206,   122,    -1,    -1,   204,    -1,    -1,   219,    -1,   125,
     253,    -1,     3,   204,   221,   123,    -1,     3,   204,   240,
     221,   123,    -1,    27,   204,   221,   123,    -1,    27,   204,
     240,   221,   123,    -1,    28,   204,   221,   123,    -1,    28,
     204,   240,   221,   123,    -1,   228,   221,   123,    -1,   228,
      31,   221,   123,    -1,    47,   123,    -1,    46,    47,   123,
      -1,   228,    53,   204,   122,   123,    -1,   228,    52,   204,
     122,   123,    -1,   228,    52,   204,   122,    48,   123,    -1,
     228,    52,   204,   122,   135,   281,   136,   123,    -1,   228,
      52,   204,   122,   121,   206,   122,   123,    -1,   223,    -1,
     223,   124,   222,    -1,   223,    -1,   223,   124,   222,    -1,
     240,   223,    -1,   240,   223,   124,   222,    -1,   224,   218,
      -1,   204,   225,    -1,    -1,   226,    -1,    -1,    48,   227,
     225,    -1,   135,   282,   136,   225,    -1,   202,   229,    -1,
     229,    -1,   203,   229,    -1,   203,   202,   229,    -1,   245,
      -1,   245,   240,    -1,   233,    -1,    -1,   233,   230,   240,
      -1,   239,    -1,    -1,   239,   231,   240,    -1,    41,   239,
      -1,    -1,    41,   239,   232,   240,    -1,    -1,    45,   118,
     234,   236,   119,    -1,    -1,     8,   118,   235,   236,   119,
      -1,   228,    -1,    -1,   228,   124,   237,   236,    -1,     8,
      -1,    45,    -1,   233,    -1,   239,    -1,     8,    79,   238,
      -1,    45,    79,   238,    -1,   233,    79,   238,    -1,   132,
      -1,   129,    -1,   129,   132,    -1,   129,   129,    -1,   129,
      33,    -1,   129,    32,    -1,    33,    -1,    32,    -1,    -1,
     129,   129,   241,   240,    -1,    -1,   129,   132,   242,   240,
      -1,    -1,    33,   243,   240,    -1,    -1,    32,   244,   240,
      -1,   246,    -1,    81,    -1,    82,    -1,    25,    -1,    26,
      -1,     8,    -1,    45,    -1,    21,    -1,    15,    -1,    20,
      -1,    24,    -1,    23,    -1,    83,    -1,    84,    -1,    85,
      -1,    86,    -1,    87,    -1,    88,    -1,    89,    -1,    90,
      -1,    91,    -1,    92,    -1,    -1,    36,   247,   248,    -1,
     248,    -1,    22,    -1,    14,    -1,    16,    -1,    17,    -1,
      80,    -1,    18,    -1,    19,    -1,    -1,   120,   250,    -1,
     252,   238,    -1,    -1,   252,   238,   251,   124,   250,    -1,
      -1,     4,    -1,     5,    -1,     6,    -1,   254,    -1,   127,
     254,    -1,   126,   254,    -1,     9,    -1,   121,   253,   122,
      -1,    10,    -1,    11,    -1,    12,    -1,    13,    -1,     8,
      -1,    45,    -1,    -1,    93,   121,   204,   124,   256,   229,
     122,    -1,    -1,    -1,    -1,    94,   121,   257,   204,   124,
     258,   229,   259,   122,    -1,    -1,    95,   121,   260,   204,
     122,    -1,    -1,    96,   121,   261,   204,   122,    -1,    -1,
      -1,    97,   121,   204,   124,   262,   245,   263,   124,   282,
     122,    -1,    -1,    98,   121,   204,   124,   264,   245,   122,
      -1,    -1,    -1,    -1,    99,   121,   265,   204,   124,   266,
     245,   267,   122,    -1,   100,   121,   204,   124,   245,   122,
      -1,    -1,   101,   121,   204,   124,   268,   245,   122,    -1,
      -1,   105,   121,   204,   124,   269,   245,   122,    -1,    -1,
     102,   121,   204,   124,   270,   245,   122,    -1,    -1,   106,
     121,   204,   124,   271,   245,   122,    -1,    -1,   103,   121,
     204,   124,   272,   245,   122,    -1,    -1,   107,   121,   204,
     124,   273,   245,   122,    -1,    -1,   104,   121,   204,   124,
     274,   245,   122,    -1,    -1,   108,   121,   204,   124,   275,
     245,   122,    -1,    -1,   109,   121,   204,   124,   276,   245,
     124,    10,   122,    -1,    -1,   110,   121,   204,   124,   277,
     245,   124,    10,   122,    -1,   111,   121,   204,   122,    -1,
     112,   121,   204,   122,    -1,   113,   121,   204,   124,   204,
     122,    -1,   113,   121,   204,   124,   204,   124,   122,    -1,
     121,   122,    -1,   135,   136,    -1,    50,   135,   136,    -1,
      51,   135,   136,    -1,   279,    -1,   125,    -1,   129,    -1,
     130,    -1,   126,    -1,   127,    -1,   137,    -1,   128,    -1,
     124,    -1,   118,    -1,   119,    -1,   132,    -1,   133,    -1,
     134,    -1,   131,    -1,    50,    -1,    51,    -1,    54,    -1,
      55,    -1,    56,    -1,    57,    -1,    58,    -1,    59,    -1,
      62,    -1,    63,    -1,    64,    -1,    65,    -1,    66,    -1,
      60,    -1,    61,    -1,    67,    -1,    68,    -1,    69,    -1,
      70,    -1,    71,    -1,    72,    -1,    73,    -1,    74,    -1,
      75,    -1,    76,    -1,    77,    -1,   114,    -1,    -1,   285,
     281,    -1,    -1,   286,   282,    -1,    -1,   284,   283,    -1,
       3,    -1,    40,    -1,   285,    -1,   123,    -1,   286,    -1,
     290,    -1,    30,    -1,   287,    -1,   288,    -1,   289,    -1,
     279,    -1,   120,    -1,   138,    -1,   245,    -1,    79,    -1,
      10,    -1,    11,    -1,    12,    -1,    13,    -1,     9,    -1,
      29,    -1,    31,    -1,    32,    -1,    33,    -1,    34,    -1,
      35,    -1,    46,    -1,    38,    -1,     7,    -1,    27,    -1,
      28,    -1,    41,    -1,    48,    -1,    47,    -1,    78,    -1,
       4,    -1,     6,    -1,     5,    -1,    43,    -1,    44,    -1,
     280,    -1,   116,   283,   117,    -1,   121,   281,   122,    -1,
      52,   281,   122,    -1,    53,   281,   122,    -1,   135,   281,
     136,    -1,    42,   291,   123,    -1,    -1,     3,   291,    -1,
     286,   291,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   534,   534,   536,   536,   538,   538,   539,   539,   541,
     541,   543,   543,   546,   546,   546,   548,   549,   550,   551,
     552,   553,   554,   555,   556,   557,   558,   559,   560,   561,
     562,   563,   564,   565,   566,   567,   568,   569,   570,   571,
     572,   573,   574,   582,   584,   586,   586,   586,   588,   588,
     592,   592,   592,   592,   594,   594,   594,   596,   597,   602,
     608,   614,   614,   615,   617,   617,   618,   618,   619,   619,
     620,   620,   622,   624,   626,   628,   630,   632,   633,   634,
     636,   637,   638,   639,   640,   641,   642,   643,   644,   645,
     647,   649,   651,   652,   652,   655,   656,   656,   658,   658,
     660,   661,   662,   664,   666,   667,   668,   669,   673,   677,
     684,   693,   697,   701,   705,   712,   722,   726,   721,   737,
     737,   744,   757,   758,   757,   765,   765,   772,   785,   786,
     788,   788,   791,   790,   795,   796,   795,   805,   805,   813,
     813,   815,   815,   817,   817,   819,   821,   831,   831,   833,
     835,   836,   838,   838,   840,   841,   842,   844,   844,   846,
     846,   848,   848,   848,   850,   851,   854,   853,   857,   867,
     866,   875,   883,   883,   891,   892,   891,   901,   901,   903,
     903,   905,   907,   908,   909,   910,   911,   912,   913,   914,
     915,   916,   917,   918,   919,   920,   921,   923,   924,   926,
     927,   928,   929,   932,   934,   942,   942,   945,   945,   951,
     956,   957,   958,   960,   963,   964,   966,   968,   968,   970,
     972,   972,   974,   977,   976,   981,   981,   983,   983,   986,
     986,   986,   988,   989,   990,   991,   993,   999,  1005,  1021,
    1022,  1023,  1024,  1025,  1027,  1029,  1030,  1031,  1031,  1033,
    1033,  1035,  1035,  1037,  1037,  1040,  1041,  1042,  1044,  1046,
    1047,  1048,  1051,  1052,  1053,  1054,  1055,  1056,  1057,  1058,
    1059,  1060,  1061,  1062,  1063,  1064,  1065,  1066,  1066,  1068,
    1071,  1072,  1073,  1074,  1075,  1076,  1077,  1079,  1079,  1081,
    1090,  1089,  1098,  1099,  1100,  1101,  1103,  1104,  1105,  1107,
    1108,  1110,  1111,  1112,  1113,  1114,  1115,  1118,  1118,  1129,
    1130,  1130,  1129,  1139,  1139,  1150,  1150,  1160,  1160,  1160,
    1195,  1194,  1206,  1207,  1207,  1206,  1216,  1234,  1234,  1239,
    1239,  1244,  1244,  1249,  1249,  1254,  1254,  1259,  1259,  1264,
    1264,  1269,  1269,  1274,  1274,  1291,  1291,  1308,  1358,  1412,
    1463,  1521,  1522,  1523,  1524,  1525,  1527,  1528,  1528,  1529,
    1529,  1530,  1530,  1531,  1531,  1532,  1532,  1533,  1533,  1534,
    1535,  1536,  1537,  1538,  1539,  1540,  1541,  1542,  1543,  1544,
    1545,  1546,  1547,  1548,  1549,  1550,  1551,  1552,  1553,  1554,
    1555,  1556,  1557,  1558,  1559,  1560,  1566,  1571,  1571,  1572,
    1572,  1573,  1573,  1575,  1575,  1575,  1577,  1577,  1577,  1579,
    1579,  1579,  1579,  1580,  1580,  1580,  1580,  1580,  1581,  1581,
    1581,  1581,  1582,  1582,  1582,  1582,  1582,  1582,  1583,  1583,
    1583,  1583,  1583,  1583,  1583,  1584,  1584,  1584,  1584,  1584,
    1584,  1585,  1585,  1585,  1587,  1588,  1589,  1590,  1591,  1592,
    1594,  1594,  1595
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
  "DOUBLE_COLON", "IdType", "StdString", "UnicodeString", "TypeInt8",
  "TypeUInt8", "TypeInt16", "TypeUInt16", "TypeInt32", "TypeUInt32",
  "TypeInt64", "TypeUInt64", "TypeFloat32", "TypeFloat64", "SetMacro",
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
  "$@2", "class_def_body", "$@3", "class_def_item", "named_enum", "enum",
  "enum_list", "enum_item", "enum_value", "enum_literal", "enum_math",
  "math_unary_op", "math_binary_op", "named_union", "union", "using",
  "template_internal_class", "internal_class", "internal_class_body",
  "typedef", "template_function", "template_operator", "template", "$@4",
  "template_args", "$@5", "template_arg", "template_type",
  "legacy_function", "function", "operator", "typecast_op_func", "$@6",
  "$@7", "op_func", "$@8", "op_sig", "$@9", "$@10", "func", "$@11",
  "pure_virtual", "maybe_const", "func_sig", "$@12", "$@13", "@14",
  "constructor", "$@15", "constructor_sig", "$@16", "maybe_initializers",
  "more_initializers", "initializer", "destructor", "destructor_sig",
  "$@17", "const_mod", "static_mod", "any_id", "func_body",
  "ignore_args_list", "ignore_more_args", "args_list", "$@18", "more_args",
  "$@19", "arg", "$@20", "$@21", "$@22", "$@23", "maybe_id",
  "maybe_var_assign", "var_assign", "var", "var_ids",
  "maybe_indirect_var_ids", "var_id_maybe_assign", "var_id",
  "maybe_var_array", "var_array", "$@24", "type", "type_red1", "$@25",
  "$@26", "$@27", "templated_id", "$@28", "$@29", "types", "$@30",
  "maybe_scoped_id", "scoped_id", "type_indirection", "$@31", "$@32",
  "$@33", "$@34", "type_red2", "type_primitive", "$@35", "type_integer",
  "optional_scope", "scope_list", "$@36", "scope_type", "literal",
  "literal2", "macro", "$@37", "$@38", "$@39", "$@40", "$@41", "$@42",
  "$@43", "$@44", "$@45", "$@46", "$@47", "$@48", "$@49", "$@50", "$@51",
  "$@52", "$@53", "$@54", "$@55", "$@56", "$@57", "$@58", "op_token",
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
     143,   145,   143,   146,   147,   146,   148,   148,   148,   148,
     148,   148,   148,   148,   148,   148,   148,   148,   148,   148,
     148,   148,   148,   148,   148,   148,   148,   148,   148,   148,
     148,   148,   148,   149,   150,   151,   151,   151,   152,   152,
     153,   153,   153,   153,   154,   154,   154,   155,   155,   155,
     155,   156,   156,   156,   157,   157,   157,   157,   157,   157,
     157,   157,   158,   159,   160,   161,   162,   163,   163,   163,
     164,   164,   164,   164,   164,   164,   164,   164,   164,   164,
     165,   166,   167,   168,   167,   169,   170,   169,   171,   171,
     172,   172,   172,   173,   174,   174,   174,   174,   174,   174,
     174,   175,   175,   175,   175,   175,   177,   178,   176,   180,
     179,   179,   182,   183,   181,   185,   184,   184,   186,   186,
     187,   187,   189,   188,   190,   191,   188,   193,   192,   195,
     194,   196,   196,   197,   197,   198,   199,   201,   200,   202,
     203,   203,   204,   204,   205,   205,   205,   206,   206,   207,
     207,   208,   209,   208,   210,   210,   211,   210,   212,   213,
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
     258,   259,   255,   260,   255,   261,   255,   262,   263,   255,
     264,   255,   265,   266,   267,   255,   255,   268,   255,   269,
     255,   270,   255,   271,   255,   272,   255,   273,   255,   274,
     255,   275,   255,   276,   255,   277,   255,   255,   255,   255,
     255,   278,   278,   278,   278,   278,   279,   279,   279,   279,
     279,   279,   279,   279,   279,   279,   279,   279,   279,   279,
     279,   279,   279,   279,   279,   279,   279,   279,   279,   279,
     279,   279,   279,   279,   279,   279,   279,   279,   279,   279,
     279,   279,   279,   279,   279,   279,   280,   281,   281,   282,
     282,   283,   283,   284,   284,   284,   285,   285,   285,   286,
     286,   286,   286,   286,   286,   286,   286,   286,   286,   286,
     286,   286,   286,   286,   286,   286,   286,   286,   286,   286,
     286,   286,   286,   286,   286,   286,   286,   286,   286,   286,
     286,   286,   286,   286,   287,   288,   288,   288,   289,   290,
     291,   291,   291
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     0,     3,     2,     3,     2,     1,     0,
       7,     0,    10,     0,     0,     3,     2,     1,     2,     1,
       1,     1,     1,     1,     1,     1,     2,     1,     1,     2,
       3,     3,     2,     3,     2,     3,     3,     2,     3,     2,
       5,     2,     1,     7,     6,     0,     1,     3,     1,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     3,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     7,     6,     3,     2,     3,     1,     4,     3,
       4,     8,     7,     9,     9,     2,     2,     2,     2,     3,
       2,     2,     3,     0,     5,     1,     0,     4,     2,     2,
       1,     1,     1,     4,     2,     3,     1,     2,     3,     4,
       3,     1,     2,     3,     4,     3,     0,     0,     8,     0,
       3,     2,     0,     0,     7,     0,     3,     2,     2,     2,
       0,     1,     0,     5,     0,     0,     9,     0,     3,     0,
       5,     0,     3,     0,     3,     4,     1,     0,     5,     1,
       1,     2,     1,     1,     1,     4,     3,     0,     1,     1,
       3,     0,     0,     2,     1,     1,     0,     4,     2,     0,
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
       0,     0,     9,     0,     5,     0,     5,     0,     0,    10,
       0,     7,     0,     0,     0,     9,     6,     0,     7,     0,
       7,     0,     7,     0,     7,     0,     7,     0,     7,     0,
       7,     0,     7,     0,     9,     0,     9,     4,     4,     6,
       7,     2,     2,     3,     3,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     0,     2,     0,
       2,     0,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     3,     3,     3,     3,     3,
       0,     2,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
     397,   438,   440,   439,   431,   260,   422,   418,   419,   420,
     421,   281,   263,   282,   283,   285,   286,   264,   262,   280,
     266,   265,   258,   259,   432,   433,   423,   409,   424,   425,
     426,   427,   428,   277,   430,   434,   450,   441,   442,   261,
     429,   436,   435,   370,   371,   397,   397,   372,   373,   374,
     375,   376,   377,   383,   384,   378,   379,   380,   381,   382,
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     395,   437,   417,   284,   256,   257,   267,   268,   269,   270,
     271,   272,   273,   274,   275,   276,   396,   401,   364,   365,
     414,   397,   406,   363,   356,   359,   360,   362,   357,   358,
     369,   366,   367,   368,   397,   361,   415,     0,   416,   255,
     279,   413,   443,     3,   397,   407,   410,   411,   412,   408,
       0,   450,   450,     0,     0,     0,   403,   404,     0,   401,
     405,     0,     0,     1,     0,     0,     2,   397,     8,     0,
     398,   278,   451,   452,   449,   446,   447,   444,   402,   445,
     448,   153,   152,     9,    93,     3,     0,     7,     5,     0,
     287,    92,     0,     4,     6,   260,   149,     0,   261,   150,
       0,     0,   229,   211,   216,     0,   219,   214,   292,     0,
     101,   102,   100,   177,     0,    95,   177,     0,   227,     0,
       0,     0,   222,     0,   225,   151,   210,     0,   212,   230,
       0,     0,    11,     0,   246,   245,   240,   239,   215,   293,
     294,   295,   288,     0,    14,   178,    99,    94,    96,    98,
     232,   233,   234,   236,   235,     0,     0,   237,     0,   213,
       0,   238,   218,   287,   221,     0,     0,   244,   243,   242,
     241,   289,     0,   292,     0,     0,   224,     0,   231,     0,
     254,   252,     0,     0,     0,    10,     0,     0,   260,     0,
       0,    28,     0,     0,     0,     0,     0,   399,   261,   150,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    14,    20,    19,    22,    21,
      23,    27,    25,    24,     0,     0,     0,     0,     0,     0,
     111,   106,   137,     0,    17,     0,     0,    42,    97,   228,
     226,    14,   248,   250,   292,     0,     0,     0,    45,     0,
     397,     0,     0,     0,    26,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    18,     0,     0,     0,
       0,     0,    86,    85,    88,    87,     0,     0,   399,     0,
     190,     0,     0,   309,   313,   315,     0,     0,   322,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   397,   104,   146,     0,    15,   397,   154,
      37,    32,    75,    90,    91,    39,    34,    29,   141,   139,
       0,     0,     0,     0,   112,   119,   107,   125,   205,     0,
     197,   179,    16,    41,     0,   291,   397,   399,    77,    76,
     205,     0,     0,   105,     0,   115,   110,     0,     0,    46,
      48,    45,     0,     0,     0,   397,     0,     0,   116,     0,
      35,    30,     0,    38,    33,    36,    31,     0,     0,     0,
       0,   397,     0,     0,     0,    89,     0,     0,     0,    74,
     400,   191,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   147,     0,     0,
     138,   162,   113,   108,     0,   370,   371,     0,     0,   122,
     355,     0,     0,     0,     0,   130,   121,   130,   127,   207,
     134,   132,   399,   204,   206,   188,     0,     0,   203,   180,
      12,     0,     0,   182,     0,   114,   109,   399,    45,     0,
       0,   184,     0,   399,     0,   186,     0,   162,     0,   397,
       0,     0,    80,     0,   103,     0,   307,     0,     0,     0,
     317,   320,     0,     0,   327,   331,   335,   339,   329,   333,
     337,   341,   343,   345,   347,   348,     0,     0,   162,   156,
     143,     0,     0,     0,   189,     0,     0,   351,   352,     0,
       0,     0,   129,   128,   131,   120,   126,   205,     0,   162,
       0,   198,   199,     0,   305,   299,   301,   302,   303,   304,
     306,     0,     0,     0,   181,   296,     0,    79,   183,     0,
      47,   153,    54,    55,    56,   152,     0,    61,    62,    63,
      57,    50,    49,     0,    51,    53,    52,   399,   185,     0,
     399,   187,     0,     0,     0,     0,     0,     0,     0,   310,
     314,   316,     0,     0,   323,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    40,     0,   155,
       0,   142,   397,   140,   171,   164,   163,   165,   205,   353,
     354,   123,     0,   157,   193,   397,   192,   208,     0,     0,
     205,     0,   201,     0,   298,   297,    78,    44,     0,    64,
      65,    66,    67,    68,    69,    70,    71,     0,    58,     0,
      73,     0,   117,     0,     0,   399,   399,     0,     0,   318,
       0,     0,   326,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   349,     0,   148,   143,     0,   166,   174,
     172,   169,   168,   162,   194,     0,   158,   159,     0,     0,
     133,   209,   200,     0,   300,    60,    59,    43,    72,   130,
      82,     0,     0,     0,   308,   311,     0,   321,   324,   328,
     332,   336,   340,   330,   334,   338,   342,     0,     0,   350,
     144,   145,     0,   177,   177,   179,     0,     0,     0,     0,
     135,   202,   118,    81,     0,     0,     0,   399,     0,     0,
       0,   167,     0,     0,   170,   124,   196,   160,   195,   162,
      83,    84,   312,     0,   325,   344,   346,     0,   173,     0,
     319,   175,   136,   157,     0,   176
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,   107,   136,   137,   138,   160,   233,   242,   243,   295,
     296,   297,   428,   429,   620,   621,   622,   623,   697,   298,
     299,   300,   301,   302,   419,   303,   304,   305,   139,   162,
     184,   244,   185,   186,   307,   393,   394,   310,   537,   749,
     404,   505,   405,   579,   733,   406,   507,   506,   585,   407,
     589,   588,   799,   311,   398,   312,   491,   490,   661,   570,
     384,   385,   568,   170,   171,   420,   390,   735,   736,   572,
     573,   666,   772,   737,   775,   774,   773,   813,   216,   518,
     519,   314,   409,   591,   410,   411,   513,   514,   587,   172,
     173,   201,   203,   226,   174,   228,   225,   175,   230,   223,
     176,   593,   252,   253,   236,   235,   108,   109,   120,   110,
     179,   212,   254,   213,   604,   605,   317,   638,   466,   708,
     786,   467,   468,   642,   756,   643,   471,   711,   788,   646,
     650,   647,   651,   648,   652,   649,   653,   654,   655,   499,
     111,   112,   113,   357,   128,   129,   114,   115,   116,   117,
     118,   119,   123
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -656
static const yytype_int16 yypact[] =
{
     973,  -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,
    -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,
    -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,
    -656,  -656,  -656,  -656,  -656,  -656,   838,  -656,  -656,  -656,
    -656,  -656,  -656,  -656,  -656,   973,   973,  -656,  -656,  -656,
    -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,
    -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,
    -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,
    -656,  -656,  -656,  -656,  -656,  -656,  -656,   702,  -656,  -656,
    -656,   973,  -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,
    -656,  -656,  -656,  -656,   973,  -656,  -656,    29,  -656,  -656,
    -656,  -656,  -656,    71,   973,  -656,  -656,  -656,  -656,  -656,
     390,   838,   838,    32,    31,    82,  -656,  -656,    69,   702,
    -656,    85,    76,  -656,   179,   118,  -656,   973,  -656,  1916,
    -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,
    -656,  -656,  -656,   122,   145,    71,  2350,  -656,  -656,  2199,
     151,  -656,   351,  -656,  -656,    -3,  -656,   262,     5,   230,
    2318,  2233,   152,  -656,    63,   160,    19,    84,   461,   178,
    -656,  -656,  -656,   179,   177,   186,   179,   348,  -656,    -3,
       5,   238,   166,   348,  -656,  -656,  -656,  2318,  -656,  -656,
     348,    84,  -656,    84,   234,   269,   301,  -656,  -656,  -656,
    -656,  -656,  -656,   348,   203,  -656,  -656,  -656,  -656,  -656,
      -3,     5,   238,  -656,  -656,  2199,    84,  -656,  2199,  -656,
    2199,  -656,  -656,   151,  -656,    84,    84,  -656,  -656,   305,
     326,   215,   243,  1244,   351,   252,  -656,   255,  -656,   264,
    -656,  -656,    84,    84,   261,  -656,   179,  1702,    51,    16,
      48,  -656,  2199,  1359,  1445,  1792,  1882,  1108,    64,   153,
     266,   274,   276,   282,   284,   290,   293,   296,   298,   304,
     307,   308,   311,   315,   317,   324,   328,   331,   332,   339,
     347,   352,   353,   354,   179,   203,  -656,  -656,  -656,  -656,
    -656,  -656,  -656,  -656,    50,    50,  1359,    50,    50,    50,
    -656,  -656,  -656,   358,  -656,   295,   270,   303,  -656,  -656,
    -656,   203,  -656,  -656,   461,    17,   179,   100,   179,   189,
     973,   200,   359,   179,  -656,    50,    50,   212,    50,    50,
    1531,    50,    50,   179,   179,   179,  -656,   368,    61,    16,
      48,   364,  -656,  -656,  -656,  -656,   291,   371,  1108,   375,
    -656,  1617,   179,  -656,  -656,  -656,   179,   179,  -656,   179,
     179,   179,   179,   179,   179,   179,   179,   179,   179,   179,
     179,   179,   179,   973,  -656,  -656,   361,  -656,   973,  -656,
    -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,   327,  -656,
     217,  1959,   179,   179,  -656,    -7,  -656,    -7,   210,   376,
     377,   363,  -656,  -656,   385,  -656,   973,  1108,  -656,  -656,
      30,   380,   179,  -656,   217,  -656,  -656,   -38,   402,   383,
     395,   179,   398,   179,   405,   973,   400,   179,  -656,   130,
    -656,  -656,   217,  -656,  -656,  -656,  -656,   319,   319,   319,
     179,   973,   408,   420,   421,  -656,   179,   179,   416,  -656,
    -656,  -656,  1702,   418,   341,   417,   179,   179,   179,   419,
     422,   179,   423,   425,   426,   429,   430,   431,   432,   433,
     434,   435,   436,   439,   440,   441,   442,  -656,   446,   179,
    -656,   444,  -656,  -656,   445,   437,   443,   447,   438,  -656,
    -656,   453,   454,   557,   567,   548,  -656,   548,  -656,  -656,
    -656,  -656,  1108,  -656,  -656,  -656,   319,   148,  -656,  -656,
    -656,   463,   458,  -656,   459,  -656,  -656,  1108,   179,    62,
     466,  -656,   462,  1108,   467,  -656,   464,   444,   471,   973,
     470,   474,  -656,   365,  -656,   179,  -656,   469,   475,   476,
    -656,  -656,   477,  2350,  -656,  -656,  -656,  -656,  -656,  -656,
    -656,  -656,  -656,  -656,  -656,  -656,   179,   472,   444,   479,
     480,   478,   481,  2029,  -656,   482,   483,  -656,  -656,   484,
      15,   486,  -656,  -656,  -656,  -656,  -656,    30,  2199,   444,
     485,  -656,   487,   179,  -656,  -656,  -656,  -656,  -656,  -656,
    -656,   148,   473,   473,  -656,  -656,   489,  -656,  -656,   490,
    -656,    -3,  -656,  -656,  -656,     5,    62,  -656,  -656,  -656,
     399,  -656,  -656,    62,  -656,   238,  -656,  1108,  -656,   491,
    1108,  -656,   488,   179,   499,   496,   501,   179,  2318,  -656,
    -656,  -656,  2350,  2350,  -656,   498,  2350,  2350,  2350,  2350,
    2350,  2350,  2350,  2350,  2350,  2350,   -12,  -656,   502,  -656,
     179,  -656,   973,  -656,  -656,  -656,  -656,   503,    12,  -656,
    -656,  -656,   500,  2114,  -656,   973,  -656,  -656,   506,   504,
      30,   319,   507,   508,  -656,  -656,  -656,  -656,   510,  -656,
    -656,  -656,  -656,  -656,  -656,  -656,  -656,    62,  -656,   505,
    -656,   512,  -656,   513,   179,  1108,  1108,   511,  2318,  -656,
     515,  2350,  -656,   516,   517,   519,   521,   522,   523,   524,
     525,   526,   527,  -656,   530,  -656,   480,   531,  -656,  -656,
    -656,  -656,  -656,   444,  -656,   532,  -656,   533,   520,   528,
    -656,  -656,  -656,   319,  -656,  -656,  -656,  -656,  -656,   548,
    -656,   535,   537,   538,  -656,  -656,   539,  -656,  -656,  -656,
    -656,  -656,  -656,  -656,  -656,  -656,  -656,   590,   596,  -656,
    -656,  -656,  2029,   179,   179,   363,   540,   541,  2114,   542,
    -656,  -656,  -656,  -656,   543,   544,   546,  1108,   549,   550,
     551,  -656,   552,   554,  -656,  -656,  -656,  -656,  -656,   444,
    -656,  -656,  -656,   555,  -656,  -656,  -656,   534,  -656,   556,
    -656,  -656,  -656,  2114,   558,  -656
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -656,  -656,   452,  -656,   509,  -656,  -656,  -115,  -656,  -656,
     413,   415,  -414,  -656,  -656,  -656,  -494,  -656,  -656,   424,
     427,  -656,  -656,  -253,  -656,  -656,   397,   428,  -143,  -656,
     450,  -656,  -656,  -656,  -656,  -220,    98,  -656,  -656,  -656,
    -261,  -656,  -656,  -656,  -656,  -280,  -656,   275,  -500,  -656,
    -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,   -43,    26,
     362,  -656,  -656,   518,  -656,   -89,   154,  -126,   -87,  -491,
    -656,   -77,  -656,  -558,  -656,  -656,  -656,  -656,  -184,   -79,
    -656,   536,    -6,  -655,  -502,  -353,  -484,  -656,  -656,  -227,
    -166,  -656,  -656,  -656,  -159,  -656,  -656,  -219,  -656,  -111,
    -145,   -25,  -656,  -656,  -656,  -656,  -138,  -656,  -656,   577,
     465,   378,  -656,   456,    99,  -384,  -656,  -656,  -656,  -656,
    -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,
    -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,  -656,
     300,  -656,   -33,  -318,   574,  -656,   -20,   -36,  -656,  -656,
    -656,  -656,   108
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -291
static const yytype_int16 yytable[] =
{
     122,   158,   219,   458,   196,   198,   245,   586,   191,   247,
     334,   248,   124,   125,   592,   667,   315,   530,   164,   183,
     151,   177,   192,   308,   151,   151,   742,   503,   222,   133,
     327,   229,   177,   177,   222,   332,   337,   337,   347,   356,
     460,   222,   224,   335,   341,   153,   632,   426,   224,   204,
     205,  -220,  -220,   392,   222,   224,   151,   152,   131,   177,
     509,   152,   152,   672,   729,   730,   425,   130,   224,   151,
     611,   132,   612,   613,   134,   614,   187,   658,   509,   337,
     510,   140,   227,   511,   193,   122,   122,   177,   781,   231,
     177,   682,   177,   152,   215,  -217,  -217,   215,   679,   522,
     306,   183,   241,   677,   155,   177,   152,   615,   151,   130,
     723,   135,   724,   337,   610,   188,   204,   205,   504,   177,
     493,   340,   688,   194,   177,   177,   177,   177,   177,   698,
     187,   424,   328,   416,   464,   401,   673,   417,   674,   492,
     418,   463,   200,   193,   526,   152,   206,   512,  -220,   207,
     675,  -220,   208,   145,   313,   144,   594,   595,   596,   597,
     598,   599,   493,   525,   330,   512,   388,   325,   177,   188,
     329,   331,  -153,   389,   313,   313,   232,   451,   234,   592,
     387,   492,   194,   616,   732,  -152,   147,   151,   617,   618,
     619,   195,  -217,   600,   590,  -217,   741,   151,  -223,  -223,
     359,   246,   177,   746,   146,   386,   414,   149,   151,   609,
     250,   251,   150,   206,   667,   629,   207,   313,   684,   685,
     151,   204,   205,   177,   152,   151,   408,   322,   323,   142,
     143,   358,   204,   205,   152,   543,   154,   386,   427,   430,
     159,   592,   776,   442,   439,   152,   416,   401,   427,   782,
     417,   313,   401,   418,   447,   448,   449,   152,   509,   452,
     453,   454,   152,   426,   161,   493,  -253,  -253,   195,   601,
     189,   178,   313,   465,   602,   603,   199,   469,   470,   202,
     472,   473,   474,   475,   476,   477,   478,   479,   480,   481,
     482,   483,   484,   485,   214,  -223,   217,   434,  -223,   151,
     422,  -251,  -251,   151,   433,   431,   437,   190,   809,   699,
     218,   408,   701,   501,   502,   731,   435,   200,   206,   421,
     -13,   207,   358,   432,   177,   436,   400,   151,   510,   206,
     401,   511,   207,   237,   238,   427,   152,  -247,  -247,  -290,
     152,   309,   430,   456,   457,   512,   668,   402,   403,   151,
     486,   204,   205,   427,   180,   488,   220,   526,  -249,  -249,
     255,   336,   342,  -253,   152,   181,  -253,   540,   541,   678,
     625,   319,   545,   151,   320,   427,   151,   547,   548,   549,
     321,   358,   552,   521,   626,   324,   152,   752,   753,   360,
     412,   135,   182,   221,   494,   361,   637,   362,  -251,   450,
     571,  -251,   534,   363,    11,   364,    13,    14,    15,    16,
     152,   365,    19,   152,   366,   645,   524,   367,   538,   368,
     402,   403,   422,   433,   437,   369,   413,   532,   370,   371,
     239,   536,   372,   240,  -247,   177,   373,  -247,   374,   430,
     624,   421,   432,   436,   494,   375,   668,   489,   206,   376,
     177,   207,   377,   378,   427,  -249,   427,   625,  -249,   391,
     379,   395,   396,   397,   625,   209,   210,   211,   380,   803,
      73,   626,   707,   381,   382,   383,   358,   656,   626,   399,
     438,   594,   487,   596,   597,   598,   599,   455,   517,   440,
     441,   358,   443,   444,   459,   445,   446,   358,   461,   515,
     177,   516,   520,   523,   709,   710,   634,   528,   713,   714,
     715,   716,   717,   718,   719,   720,   721,   722,   600,   527,
     529,   531,   533,   535,   539,   689,   690,   624,   691,   692,
     693,   694,   695,   696,   624,   177,   431,   435,   625,   542,
     544,   546,   755,   550,   703,   668,   551,   553,   427,   554,
     555,   668,   626,   556,   557,   558,   559,   560,   561,   562,
     563,   564,   565,   569,   567,   566,  -161,   582,   574,   577,
     177,   571,   575,   758,   578,   580,   581,   583,   576,   584,
     606,   607,   608,   627,   630,   628,   668,   631,   633,   792,
     793,   358,   635,   639,   358,   657,   636,   640,   641,   662,
     789,   644,   659,   663,   660,   671,   790,   163,   624,   676,
     702,   681,   686,   687,   700,   751,   704,   705,   669,   670,
     712,   680,   706,   734,   725,   739,   740,   728,   747,   727,
     744,   743,   745,   754,   177,   748,   750,   757,   759,   760,
     177,   761,   738,   762,   763,   764,   765,   766,   157,   780,
     767,   768,   769,   771,   777,   811,   779,   778,   783,   784,
     785,   338,   795,   787,   796,   798,   800,   801,   802,   358,
     358,   804,   805,   806,   807,   177,   808,   810,   812,   352,
     815,   353,   508,   770,   215,   215,   726,   814,   423,   197,
     354,   797,   339,   355,   318,   791,   794,   141,   249,   316,
     683,   500,   415,   148,     0,   126,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,     0,
      34,     0,   127,    35,    36,    37,    38,    39,    40,    41,
      42,   358,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,     0,     0,     0,     0,     0,
       0,   346,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    86,     0,    87,     0,
      88,    89,    90,    91,     0,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,     0,   105,
     106,   121,     1,     2,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,     0,    34,     0,     0,    35,
       0,    37,    38,    39,    40,    41,    42,     0,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    86,     0,    87,     0,    88,    89,    90,    91,
       0,     0,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,     0,   105,   106,     1,     2,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
       0,    34,     0,     0,    35,    36,    37,    38,    39,    40,
      41,    42,     0,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    86,     0,    87,
       0,    88,    89,    90,    91,     0,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,     0,
     105,   106,     1,     2,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,     0,    34,     0,     0,    35,
       0,    37,    38,    39,    40,    41,    42,     0,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    86,     0,    87,     0,    88,    89,    90,    91,
       0,     0,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,     0,   105,   106,   256,   209,   210,
     211,   257,   258,     0,     0,     0,     0,     0,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,   259,   260,   261,     0,   166,     0,     0,     0,   262,
      33,   263,   264,   265,   135,   167,   266,     0,   267,   268,
     269,   270,     0,   271,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,     0,   293,
       0,     0,   333,     0,     0,     0,   257,   258,     0,     0,
       0,     0,   294,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,     0,     0,     0,     0,
     166,     0,     0,     0,   262,    33,     0,     0,     0,     0,
     167,     0,     0,     0,   268,   169,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,   257,   258,     0,     0,     0,     0,     0,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,     0,     0,     0,     0,   166,     0,     0,     0,
     262,    33,     0,     0,     0,   135,   167,   294,     0,     0,
     268,   169,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,   257,   258,
       0,     0,     0,     0,     0,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,     0,     0,
       0,     0,   166,     0,     0,     0,   262,    33,     0,     0,
       0,     0,   167,   294,     0,     0,   268,   169,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,   462,   258,     0,     0,     0,     0,
       0,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,     0,     0,     0,     0,   166,     0,
       0,     0,     0,    33,     0,     0,     0,     0,   167,   294,
       0,     0,   268,   169,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
     165,     0,     0,     0,     0,     0,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,     0,
       0,     0,     0,   166,     0,     0,     0,     0,    33,     0,
       0,     0,     0,   167,     0,   294,     0,   168,   169,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,   343,     0,     0,     0,     0,
     165,     0,     0,     0,     0,     0,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,   344,
     345,     0,     0,   166,     0,     0,     0,     0,    33,     0,
     326,     0,     0,   167,     0,     0,     0,   168,   269,   270,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,   348,     0,     0,     0,     0,
     165,     0,     0,     0,     0,     0,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,   349,
     350,     0,     0,   166,     0,     0,     0,     0,    33,   134,
       0,     0,     0,   167,     5,     0,     0,   168,   169,   351,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    33,     0,   156,     0,     0,     0,     0,     0,
       0,    39,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,   495,
     496,     0,     0,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,   165,     0,     0,
       0,     0,     0,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,     0,     0,     0,     0,
     166,     0,     0,     0,     0,    33,     0,     0,     0,     0,
     167,     0,     0,     0,   168,   169,   664,    88,    89,     0,
     497,     0,     0,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   498,     0,   105,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   665,     0,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,   165,     0,     0,     0,     0,     0,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,     0,     0,     0,     0,   166,     0,     0,     0,     0,
      33,     0,     0,     0,     0,   167,     0,     0,     0,   168,
     169,   664,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,   165,     0,     0,
       0,     0,     0,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,     0,     0,     0,     0,
     166,     0,     0,     0,     0,    33,     0,     0,     0,     0,
     167,   165,     0,     0,   168,   169,     0,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
       0,     0,     0,     0,   166,     0,     0,     0,     0,    33,
       0,     0,     0,     0,   167,     0,     0,     0,   168,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,   165,     0,     0,     0,
       0,     0,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    33,     0,     0,     0,     5,   167,
       0,     0,     0,   168,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    33,     0,     0,     0,
       0,     0,     0,     0,     0,    39,     0,     0,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85
};

static const yytype_int16 yycheck[] =
{
      36,   139,   186,   356,   170,   171,   225,   507,   167,   228,
     263,   230,    45,    46,   516,   573,   243,   431,   156,   162,
       8,   159,   167,   243,     8,     8,   681,    34,   187,     0,
     257,   197,   170,   171,   193,   262,   263,   264,   265,   266,
     358,   200,   187,   263,   264,   134,   537,   327,   193,    32,
      33,    32,    33,   306,   213,   200,     8,    45,    91,   197,
      48,    45,    45,    48,    52,    53,   327,    87,   213,     8,
       8,   104,    10,    11,     3,    13,    79,   568,    48,   306,
     118,   114,   193,   121,    79,   121,   122,   225,   743,   200,
     228,   593,   230,    45,   183,    32,    33,   186,   589,   417,
     243,   244,   213,   587,   137,   243,    45,    45,     8,   129,
     122,    40,   124,   340,   528,   118,    32,    33,   125,   257,
     400,   264,   616,   118,   262,   263,   264,   265,   266,   623,
      79,    31,   116,   116,   361,    35,   121,   120,   123,   400,
     123,   361,    79,    79,   424,    45,   129,   135,   129,   132,
     135,   132,   177,   122,   243,   123,     8,     9,    10,    11,
      12,    13,   442,   424,   116,   135,   116,   256,   306,   118,
     259,   260,   121,   123,   263,   264,   201,   116,   203,   681,
     295,   442,   118,   121,   668,   121,   117,     8,   126,   127,
     128,    38,   129,    45,   512,   132,   680,     8,    32,    33,
      47,   226,   340,   697,   122,   294,   321,   122,     8,   527,
     235,   236,   136,   129,   772,   533,   132,   306,   602,   603,
       8,    32,    33,   361,    45,     8,   315,   252,   253,   121,
     122,   267,    32,    33,    45,   462,   118,   326,   327,   328,
     118,   743,   733,    31,   333,    45,   116,    35,   337,   749,
     120,   340,    35,   123,   343,   344,   345,    45,    48,   348,
     349,   350,    45,   543,   119,   545,    32,    33,    38,   121,
       8,   120,   361,   362,   126,   127,   124,   366,   367,   119,
     369,   370,   371,   372,   373,   374,   375,   376,   377,   378,
     379,   380,   381,   382,   116,   129,   119,   330,   132,     8,
     325,    32,    33,     8,   329,   116,   331,    45,   799,   627,
     124,   400,   630,   402,   403,   668,   116,    79,   129,   325,
     117,   132,   358,   329,   462,   331,    31,     8,   118,   129,
      35,   121,   132,    32,    33,   424,    45,    32,    33,   124,
      45,   243,   431,    52,    53,   135,   573,    52,    53,     8,
     383,    32,    33,   442,     3,   388,     8,   637,    32,    33,
     117,   263,   264,   129,    45,    14,   132,   456,   457,   588,
     529,   119,    31,     8,   119,   464,     8,   466,   467,   468,
     116,   417,   471,   416,   529,   124,    45,   705,   706,   123,
     120,    40,    41,    45,   400,   121,    31,   121,   129,    31,
     489,   132,   435,   121,    14,   121,    16,    17,    18,    19,
      45,   121,    22,    45,   121,   553,   422,   121,   451,   121,
      52,    53,   447,   448,   449,   121,   123,   433,   121,   121,
     129,   437,   121,   132,   129,   573,   121,   132,   121,   528,
     529,   447,   448,   449,   450,   121,   673,   120,   129,   121,
     588,   132,   121,   121,   543,   129,   545,   616,   132,   305,
     121,   307,   308,   309,   623,     4,     5,     6,   121,   787,
      80,   616,   638,   121,   121,   121,   512,   566,   623,   121,
     121,     8,   121,    10,    11,    12,    13,   123,   125,   335,
     336,   527,   338,   339,   123,   341,   342,   533,   123,   123,
     638,   124,   117,   123,   642,   643,   539,   124,   646,   647,
     648,   649,   650,   651,   652,   653,   654,   655,    45,   117,
     125,   123,   117,   123,   116,   126,   127,   616,   129,   130,
     131,   132,   133,   134,   623,   673,   116,   116,   697,   123,
     122,   124,   708,   124,   633,   772,   124,   124,   637,   124,
     124,   778,   697,   124,   124,   124,   124,   124,   124,   124,
     124,   122,   122,   117,   122,   124,   122,    10,   123,   122,
     708,   660,   135,   711,   136,   122,   122,    10,   135,    31,
     117,   123,   123,   117,   117,   123,   813,   123,   117,   773,
     774,   627,   122,   124,   630,   123,   122,   122,   122,   121,
      10,   124,   123,   122,   124,   121,    10,   155,   697,   123,
     122,   124,   123,   123,   123,   704,   117,   121,   136,   136,
     122,   136,   121,   123,   122,   119,   122,   124,   123,   662,
     122,   124,   122,   122,   772,   123,   123,   122,   122,   122,
     778,   122,   675,   122,   122,   122,   122,   122,   139,   121,
     124,   124,   122,   122,   122,   121,   136,   124,   123,   122,
     122,   264,   122,   124,   123,   123,   123,   123,   122,   705,
     706,   122,   122,   122,   122,   813,   122,   122,   122,   266,
     122,   266,   407,   726,   773,   774,   660,   813,   326,   171,
     266,   778,   264,   266,   244,   772,   775,   120,   233,   243,
     601,   401,   324,   129,    -1,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    -1,
      38,    -1,    40,    41,    42,    43,    44,    45,    46,    47,
      48,   787,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    -1,    -1,    -1,    -1,    -1,
      -1,   265,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   114,    -1,   116,    -1,
     118,   119,   120,   121,    -1,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,    -1,   137,
     138,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    -1,    38,    -1,    -1,    41,
      -1,    43,    44,    45,    46,    47,    48,    -1,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   114,    -1,   116,    -1,   118,   119,   120,   121,
      -1,    -1,   124,   125,   126,   127,   128,   129,   130,   131,
     132,   133,   134,   135,    -1,   137,   138,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      -1,    38,    -1,    -1,    41,    42,    43,    44,    45,    46,
      47,    48,    -1,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,    -1,   116,
      -1,   118,   119,   120,   121,    -1,   123,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,    -1,
     137,   138,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    -1,    38,    -1,    -1,    41,
      -1,    43,    44,    45,    46,    47,    48,    -1,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   114,    -1,   116,    -1,   118,   119,   120,   121,
      -1,    -1,   124,   125,   126,   127,   128,   129,   130,   131,
     132,   133,   134,   135,    -1,   137,   138,     3,     4,     5,
       6,     7,     8,    -1,    -1,    -1,    -1,    -1,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    -1,    31,    -1,    -1,    -1,    35,
      36,    37,    38,    39,    40,    41,    42,    -1,    44,    45,
      46,    47,    -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,    -1,   115,
      -1,    -1,     3,    -1,    -1,    -1,     7,     8,    -1,    -1,
      -1,    -1,   128,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    -1,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    35,    36,    -1,    -1,    -1,    -1,
      41,    -1,    -1,    -1,    45,    46,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,     7,     8,    -1,    -1,    -1,    -1,    -1,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    -1,    -1,    -1,    -1,    31,    -1,    -1,    -1,
      35,    36,    -1,    -1,    -1,    40,    41,   128,    -1,    -1,
      45,    46,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,     7,     8,
      -1,    -1,    -1,    -1,    -1,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    -1,    -1,
      -1,    -1,    31,    -1,    -1,    -1,    35,    36,    -1,    -1,
      -1,    -1,    41,   128,    -1,    -1,    45,    46,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,     7,     8,    -1,    -1,    -1,    -1,
      -1,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    -1,    -1,    31,    -1,
      -1,    -1,    -1,    36,    -1,    -1,    -1,    -1,    41,   128,
      -1,    -1,    45,    46,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
       8,    -1,    -1,    -1,    -1,    -1,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    -1,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    36,    -1,
      -1,    -1,    -1,    41,    -1,   128,    -1,    45,    46,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,     3,    -1,    -1,    -1,    -1,
       8,    -1,    -1,    -1,    -1,    -1,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    -1,    -1,    31,    -1,    -1,    -1,    -1,    36,    -1,
     128,    -1,    -1,    41,    -1,    -1,    -1,    45,    46,    47,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,     3,    -1,    -1,    -1,    -1,
       8,    -1,    -1,    -1,    -1,    -1,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    -1,    -1,    31,    -1,    -1,    -1,    -1,    36,     3,
      -1,    -1,    -1,    41,     8,    -1,    -1,    45,    46,    47,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    36,    -1,    38,    -1,    -1,    -1,    -1,    -1,
      -1,    45,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    50,
      51,    -1,    -1,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,     8,    -1,    -1,
      -1,    -1,    -1,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    -1,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    36,    -1,    -1,    -1,    -1,
      41,    -1,    -1,    -1,    45,    46,    47,   118,   119,    -1,
     121,    -1,    -1,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,    -1,   137,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    78,    -1,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,     8,    -1,    -1,    -1,    -1,    -1,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    -1,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,
      36,    -1,    -1,    -1,    -1,    41,    -1,    -1,    -1,    45,
      46,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,     8,    -1,    -1,
      -1,    -1,    -1,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    -1,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    36,    -1,    -1,    -1,    -1,
      41,     8,    -1,    -1,    45,    46,    -1,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      -1,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    36,
      -1,    -1,    -1,    -1,    41,    -1,    -1,    -1,    45,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,     8,    -1,    -1,    -1,
      -1,    -1,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    36,    -1,    -1,    -1,     8,    41,
      -1,    -1,    -1,    45,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    36,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92
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
     248,   279,   280,   281,   285,   286,   287,   288,   289,   290,
     247,     3,   286,   291,   281,   281,     3,    40,   283,   284,
     285,   281,   281,     0,     3,    40,   141,   142,   143,   167,
     281,   248,   291,   291,   123,   122,   122,   117,   283,   122,
     136,     8,    45,   204,   118,   281,    38,   143,   245,   118,
     144,   119,   168,   141,   245,     8,    31,    41,    45,    46,
     202,   203,   228,   229,   233,   236,   239,   245,   120,   249,
       3,    14,    41,   167,   169,   171,   172,    79,   118,     8,
      45,   233,   239,    79,   118,    38,   229,   202,   229,   124,
      79,   230,   119,   231,    32,    33,   129,   132,   240,     4,
       5,     6,   250,   252,   116,   204,   217,   119,   124,   217,
       8,    45,   233,   238,   239,   235,   232,   238,   234,   229,
     237,   238,   240,   145,   240,   244,   243,    32,    33,   129,
     132,   238,   146,   147,   170,   236,   240,   236,   236,   249,
     240,   240,   241,   242,   251,   117,     3,     7,     8,    27,
      28,    29,    35,    37,    38,    39,    42,    44,    45,    46,
      47,    49,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   115,   128,   148,   149,   150,   158,   159,
     160,   161,   162,   164,   165,   166,   167,   173,   174,   175,
     176,   192,   194,   204,   220,   228,   252,   255,   169,   119,
     119,   116,   240,   240,   124,   204,   128,   228,   116,   204,
     116,   204,   228,     3,   162,   174,   175,   228,   165,   166,
     167,   174,   175,     3,    27,    28,   220,   228,     3,    27,
      28,    47,   149,   150,   158,   159,   228,   282,   286,    47,
     123,   121,   121,   121,   121,   121,   121,   121,   121,   121,
     121,   121,   121,   121,   121,   121,   121,   121,   121,   121,
     121,   121,   121,   121,   199,   200,   204,   146,   116,   123,
     205,   205,   162,   174,   175,   205,   205,   205,   193,   121,
      31,    35,    52,    53,   179,   181,   184,   188,   204,   221,
     223,   224,   120,   123,   146,   250,   116,   120,   123,   163,
     204,   221,   240,   199,    31,   179,   184,   204,   151,   152,
     204,   116,   221,   240,   281,   116,   221,   240,   121,   204,
     205,   205,    31,   205,   205,   205,   205,   204,   204,   204,
      31,   116,   204,   204,   204,   123,    52,    53,   224,   123,
     282,   123,     7,   174,   228,   204,   257,   260,   261,   204,
     204,   265,   204,   204,   204,   204,   204,   204,   204,   204,
     204,   204,   204,   204,   204,   204,   281,   121,   281,   120,
     196,   195,   179,   184,   221,    50,    51,   121,   135,   278,
     279,   204,   204,    34,   125,   180,   186,   185,   186,    48,
     118,   121,   135,   225,   226,   123,   124,   125,   218,   219,
     117,   281,   282,   123,   221,   179,   184,   117,   124,   125,
     151,   123,   221,   117,   281,   123,   221,   177,   281,   116,
     204,   204,   123,   228,   122,    31,   124,   204,   204,   204,
     124,   124,   204,   124,   124,   124,   124,   124,   124,   124,
     124,   124,   124,   124,   122,   122,   124,   122,   201,   117,
     198,   204,   208,   209,   123,   135,   135,   122,   136,   182,
     122,   122,    10,    10,    31,   187,   187,   227,   190,   189,
     282,   222,   223,   240,     8,     9,    10,    11,    12,    13,
      45,   121,   126,   127,   253,   254,   117,   123,   123,   282,
     151,     8,    10,    11,    13,    45,   121,   126,   127,   128,
     153,   154,   155,   156,   204,   233,   239,   117,   123,   282,
     117,   123,   208,   117,   281,   122,   122,    31,   256,   124,
     122,   122,   262,   264,   124,   245,   268,   270,   272,   274,
     269,   271,   273,   275,   276,   277,   204,   123,   208,   123,
     124,   197,   121,   122,    47,    78,   210,   212,   228,   136,
     136,   121,    48,   121,   123,   135,   123,   225,   236,   208,
     136,   124,   223,   253,   254,   254,   123,   123,   155,   126,
     127,   129,   130,   131,   132,   133,   134,   157,   155,   282,
     123,   282,   122,   204,   117,   121,   121,   229,   258,   245,
     245,   266,   122,   245,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   122,   124,   122,   198,   281,   124,    52,
      53,   224,   225,   183,   123,   206,   207,   212,   281,   119,
     122,   225,   222,   124,   122,   122,   155,   123,   123,   178,
     123,   204,   282,   282,   122,   229,   263,   122,   245,   122,
     122,   122,   122,   122,   122,   122,   122,   124,   124,   122,
     197,   122,   211,   215,   214,   213,   208,   122,   124,   136,
     121,   222,   187,   123,   122,   122,   259,   124,   267,    10,
      10,   210,   217,   217,   218,   122,   123,   207,   123,   191,
     123,   123,   122,   282,   122,   122,   122,   122,   122,   208,
     122,   121,   122,   216,   206,   122
};

#define yyerrok                (yyerrstatus = 0)
#define yyclearin        (yychar = YYEMPTY)
#define YYEMPTY                (-2)
#define YYEOF                0

#define YYACCEPT        goto yyacceptlab
#define YYABORT                goto yyabortlab
#define YYERROR                goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL                goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                        \
do                                                                \
  if (yychar == YYEMPTY && yylen == 1)                                \
    {                                                                \
      yychar = (Token);                                                \
      yylval = (Value);                                                \
      yytoken = YYTRANSLATE (yychar);                                \
      YYPOPSTACK (1);                                                \
      goto yybackup;                                                \
    }                                                                \
  else                                                                \
    {                                                                \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                        \
    }                                                                \
while (YYID (0))


#define YYTERROR        1
#define YYERRCODE        256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                        \
      if (YYID (N))                                                    \
        {                                                                \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;        \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;                \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;        \
        }                                                                \
      else                                                                \
        {                                                                \
          (Current).first_line   = (Current).last_line   =                \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =                \
            YYRHSLOC (Rhs, 0).last_column;                                \
        }                                                                \
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)                        \
     fprintf (File, "%d.%d-%d.%d",                        \
              (Loc).first_line, (Loc).first_column,        \
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

# define YYDPRINTF(Args)                        \
do {                                                \
  if (yydebug)                                        \
    YYFPRINTF Args;                                \
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                          \
do {                                                                          \
  if (yydebug)                                                                  \
    {                                                                          \
      YYFPRINTF (stderr, "%s ", Title);                                          \
      yy_symbol_print (stderr,                                                  \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                                  \
    }                                                                          \
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

# define YY_STACK_PRINT(Bottom, Top)                                \
do {                                                                \
  if (yydebug)                                                        \
    yy_stack_print ((Bottom), (Top));                                \
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

# define YY_REDUCE_PRINT(Rule)                \
do {                                        \
  if (yydebug)                                \
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
#ifndef        YYINITDEPTH
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
#line 541 "vtkParse.y"
    { start_class((yyvsp[(2) - (2)].str)); }
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 543 "vtkParse.y"
    { start_class((yyvsp[(2) - (5)].str)); }
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 546 "vtkParse.y"
    { delSig(); clearTypeId(); }
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 561 "vtkParse.y"
    { output_function(); }
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 562 "vtkParse.y"
    { reject_function(); }
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 563 "vtkParse.y"
    { output_function(); }
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 564 "vtkParse.y"
    { output_function(); }
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 565 "vtkParse.y"
    { output_function(); }
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 566 "vtkParse.y"
    { output_function(); }
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 567 "vtkParse.y"
    { reject_function(); }
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 568 "vtkParse.y"
    { output_function(); }
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 569 "vtkParse.y"
    { output_function(); }
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 570 "vtkParse.y"
    { output_function(); }
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 571 "vtkParse.y"
    { legacySig(); output_function(); }
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 596 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 598 "vtkParse.y"
    {
         (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (2)].str)) + strlen((yyvsp[(2) - (2)].str)) + 1);
         sprintf((yyval.str), "%s%s", (yyvsp[(1) - (2)].str), (yyvsp[(2) - (2)].str));
       }
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 603 "vtkParse.y"
    {
         (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (3)].str)) + strlen((yyvsp[(2) - (3)].str)) +
                                  strlen((yyvsp[(3) - (3)].str)) + 3);
         sprintf((yyval.str), "%s %s %s", (yyvsp[(1) - (3)].str), (yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].str));
       }
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 609 "vtkParse.y"
    {
         (yyval.str) = (char *)malloc(strlen((yyvsp[(2) - (3)].str)) + 3);
         sprintf((yyval.str), "(%s)", (yyvsp[(2) - (3)].str));
       }
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 614 "vtkParse.y"
    { (yyval.str) = "-"; }
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 614 "vtkParse.y"
    { (yyval.str) = "+"; }
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 615 "vtkParse.y"
    { (yyval.str) = "~"; }
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 617 "vtkParse.y"
    { (yyval.str) = "-"; }
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 617 "vtkParse.y"
    { (yyval.str) = "+"; }
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 618 "vtkParse.y"
    { (yyval.str) = "*"; }
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 618 "vtkParse.y"
    { (yyval.str) = "/"; }
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 619 "vtkParse.y"
    { (yyval.str) = "%"; }
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 619 "vtkParse.y"
    { (yyval.str) = "&"; }
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 620 "vtkParse.y"
    { (yyval.str) = "|"; }
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 620 "vtkParse.y"
    { (yyval.str) = "^"; }
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 651 "vtkParse.y"
    { postSig("template<> "); clearTypeId(); }
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 652 "vtkParse.y"
    { postSig("template<"); }
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 653 "vtkParse.y"
    { postSig("> "); clearTypeId(); }
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 656 "vtkParse.y"
    { postSig(", "); }
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 660 "vtkParse.y"
    { postSig("typename "); }
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 661 "vtkParse.y"
    { postSig("class "); }
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 662 "vtkParse.y"
    { postSig("int "); }
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 666 "vtkParse.y"
    {openSig(); preSig("~"); closeSig();}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 667 "vtkParse.y"
    {openSig(); preSig("virtual ~"); closeSig();}
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 670 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[(1) - (2)].integer);
         }
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 674 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[(1) - (3)].integer);
         }
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 678 "vtkParse.y"
    {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->ReturnType = (yyvsp[(2) - (4)].integer);
         }
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 685 "vtkParse.y"
    {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->ReturnType = (yyvsp[(2) - (3)].integer);
         }
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 694 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[(1) - (1)].integer);
         }
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 698 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[(1) - (2)].integer);
         }
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 702 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[(1) - (3)].integer);
         }
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 706 "vtkParse.y"
    {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->ReturnType = (yyvsp[(2) - (4)].integer);
         }
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 713 "vtkParse.y"
    {
         openSig();
         preSig("virtual ");
         closeSig();
         currentFunction->ReturnType = (yyvsp[(2) - (3)].integer);
         }
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 722 "vtkParse.y"
    {
      postSig("(");
      currentFunction->ReturnClass = vtkstrdup(getTypeId());
    }
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 726 "vtkParse.y"
    { postSig(")"); }
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 727 "vtkParse.y"
    {
      (yyval.integer) = (yyvsp[(2) - (8)].integer);
      postSig(";");
      preSig("operator ");
      closeSig();
      currentFunction->IsOperator = 1;
      currentFunction->Name = "operator typecast";
      vtkParseDebug("Parsed operator", "operator typecast");
    }
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 737 "vtkParse.y"
    { postSig(")"); }
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 738 "vtkParse.y"
    {
      postSig(";");
      closeSig();
      currentFunction->Name = (yyvsp[(1) - (3)].str);
      vtkParseDebug("Parsed operator", (yyvsp[(1) - (3)].str));
    }
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 745 "vtkParse.y"
    {
      postSig(";");
      closeSig();
      currentFunction->Name = (yyvsp[(1) - (2)].str);
      vtkParseDebug("Parsed operator", (yyvsp[(1) - (2)].str));
      currentFunction->IsPureVirtual = 1;
      if (mainClass)
        {
        data.IsAbstract = 1;
        }
    }
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 757 "vtkParse.y"
    {postSig((yyvsp[(2) - (2)].str));}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 758 "vtkParse.y"
    {
      postSig("(");
      currentFunction->IsOperator = 1;
      currentFunction->ReturnClass = vtkstrdup(getTypeId());
    }
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 763 "vtkParse.y"
    { (yyval.str) = (yyvsp[(2) - (7)].str); }
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 765 "vtkParse.y"
    { postSig(")"); }
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 766 "vtkParse.y"
    {
      postSig(";");
      closeSig();
      currentFunction->Name = (yyvsp[(1) - (3)].str);
      vtkParseDebug("Parsed func", (yyvsp[(1) - (3)].str));
    }
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 773 "vtkParse.y"
    {
      postSig(";");
      closeSig();
      currentFunction->Name = (yyvsp[(1) - (2)].str);
      vtkParseDebug("Parsed func", (yyvsp[(1) - (2)].str));
      currentFunction->IsPureVirtual = 1;
      if (mainClass)
        {
        data.IsAbstract = 1;
        }
    }
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 785 "vtkParse.y"
    {postSig(") = 0");}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 786 "vtkParse.y"
    {postSig(") const = 0");}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 788 "vtkParse.y"
    {postSig(" const");}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 791 "vtkParse.y"
    {
      postSig("(");
      currentFunction->ReturnClass = vtkstrdup(getTypeId());
    }
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 794 "vtkParse.y"
    { (yyval.str) = (yyvsp[(1) - (5)].str); }
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 795 "vtkParse.y"
    {markSig(); postSig("<");}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 796 "vtkParse.y"
    {
      const char *cp;
      postSig(">(");
      currentFunction->ReturnClass = vtkstrdup(getTypeId());
      cp = copySig();
      (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (6)].str)) + strlen(cp) + 1);
      sprintf((yyval.str), "%s%s", (yyvsp[(1) - (6)].str), cp);
    }
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 803 "vtkParse.y"
    { (yyval.str) = (yyvsp[(7) - (9)].str); }
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 805 "vtkParse.y"
    { postSig(")"); }
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 806 "vtkParse.y"
    {
      postSig(";");
      closeSig();
      currentFunction->Name = (yyvsp[(1) - (3)].str);
      vtkParseDebug("Parsed func", (yyvsp[(1) - (3)].str));
    }
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 813 "vtkParse.y"
    { postSig("("); }
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 822 "vtkParse.y"
    {
      postSig(");");
      closeSig();
      currentFunction->Name = (char *)malloc(strlen((yyvsp[(1) - (1)].str)) + 2);
      currentFunction->Name[0] = '~';
      strcpy(&currentFunction->Name[1], (yyvsp[(1) - (1)].str));
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 831 "vtkParse.y"
    { postSig("(");}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 833 "vtkParse.y"
    {postSig("const ");}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 835 "vtkParse.y"
    {postSig("static ");}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 836 "vtkParse.y"
    {postSig("static ");}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 838 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 838 "vtkParse.y"
    {postSig((yyvsp[(1) - (1)].str));}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 848 "vtkParse.y"
    {clearTypeId();}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 850 "vtkParse.y"
    { postSig("...");}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 852 "vtkParse.y"
    { clearTypeId(); currentFunction->NumberOfArguments++; }
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 854 "vtkParse.y"
    { clearTypeId(); currentFunction->NumberOfArguments++; postSig(", "); }
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 858 "vtkParse.y"
    {
      int i = currentFunction->NumberOfArguments;
      int array_type = ((yyvsp[(2) - (2)].integer) % VTK_PARSE_COUNT_START);
      int array_count = ((yyvsp[(2) - (2)].integer) / VTK_PARSE_COUNT_START);
      currentFunction->ArgCounts[i] = array_count;
      currentFunction->ArgTypes[i] = (yyvsp[(1) - (2)].integer) + array_type;
      currentFunction->ArgClasses[i] = vtkstrdup(getTypeId());
    }
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 867 "vtkParse.y"
    {
      int i = currentFunction->NumberOfArguments;
      int array_type = ((yyvsp[(2) - (2)].integer) % VTK_PARSE_COUNT_START);
      int array_count = ((yyvsp[(2) - (2)].integer) / VTK_PARSE_COUNT_START);
      currentFunction->ArgCounts[i] = array_count;
      currentFunction->ArgTypes[i] = (yyvsp[(1) - (2)].integer) + array_type;
      currentFunction->ArgClasses[i] = vtkstrdup(getTypeId());
    }
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 876 "vtkParse.y"
    {
      int i = currentFunction->NumberOfArguments;
      postSig("void (*func)(void *) ");
      currentFunction->ArgCounts[i] = 0;
      currentFunction->ArgTypes[i] = VTK_PARSE_FUNCTION;
      currentFunction->ArgClasses[i] = vtkstrdup("function");
    }
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 883 "vtkParse.y"
    { postSig("(&"); }
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 884 "vtkParse.y"
    {
      int i = currentFunction->NumberOfArguments;
      postSig(") ");
      currentFunction->ArgCounts[i] = 0;
      currentFunction->ArgTypes[i] = ((yyvsp[(1) - (5)].integer) | VTK_PARSE_REF);
      currentFunction->ArgClasses[i] = vtkstrdup(getTypeId());
    }
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 891 "vtkParse.y"
    { postSig("("); postSig((yyvsp[(2) - (2)].str)); postSig("*"); }
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 892 "vtkParse.y"
    { postSig(")("); }
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 893 "vtkParse.y"
    {
      int i = currentFunction->NumberOfArguments;
      postSig(")");
      currentFunction->ArgCounts[i] = 0;
      currentFunction->ArgTypes[i] = VTK_PARSE_UNKNOWN;
      currentFunction->ArgClasses[i] = vtkstrdup("function");
    }
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 905 "vtkParse.y"
    {postSig("="); postSig((yyvsp[(2) - (2)].str));}
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 934 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(2) - (2)].integer);}
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 942 "vtkParse.y"
    {(yyval.integer) = 0;}
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 942 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 945 "vtkParse.y"
    { char temp[100]; sprintf(temp,"[%i]",(yyvsp[(1) - (1)].integer));
                   postSig(temp); }
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 947 "vtkParse.y"
    { (yyval.integer) =
                         ((VTK_PARSE_COUNT_START * (yyvsp[(1) - (3)].integer)) |
                          ((VTK_PARSE_POINTER + (yyvsp[(3) - (3)].integer)) &
                           VTK_PARSE_UNQUALIFIED_TYPE)); }
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 952 "vtkParse.y"
    { postSig("[]");
              (yyval.integer) = ((VTK_PARSE_POINTER + (yyvsp[(4) - (4)].integer)) &
                             VTK_PARSE_UNQUALIFIED_TYPE); }
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 956 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_CONST | (yyvsp[(2) - (2)].integer));}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 957 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 959 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_STATIC | (yyvsp[(2) - (2)].integer));}
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 961 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_CONST|VTK_PARSE_STATIC | (yyvsp[(3) - (3)].integer));}
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 963 "vtkParse.y"
    {(yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 965 "vtkParse.y"
    {(yyval.integer) = ((yyvsp[(1) - (2)].integer) | (yyvsp[(2) - (2)].integer));}
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 967 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 968 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (1)].str));}
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 969 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_UNKNOWN | (yyvsp[(3) - (3)].integer));}
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 971 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 972 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (1)].str));}
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 973 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_UNKNOWN | (yyvsp[(3) - (3)].integer));}
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 975 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (2)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN;}
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 977 "vtkParse.y"
    {postSig(" "); setTypeId((yyvsp[(1) - (2)].str));}
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 978 "vtkParse.y"
    {(yyval.integer) = (VTK_PARSE_UNKNOWN | (yyvsp[(4) - (4)].integer));}
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 981 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 982 "vtkParse.y"
    {chopSig(); postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 983 "vtkParse.y"
    { markSig(); postSig((yyvsp[(1) - (2)].str)); postSig("<");}
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 984 "vtkParse.y"
    {chopSig(); postSig(">"); (yyval.str) = vtkstrdup(copySig()); clearTypeId();}
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 986 "vtkParse.y"
    {postSig(", ");}
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 988 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 989 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str); postSig((yyvsp[(1) - (1)].str));}
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 990 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 991 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 994 "vtkParse.y"
    {
             (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (3)].str))+strlen((yyvsp[(3) - (3)].str))+3);
             sprintf((yyval.str), "%s::%s", (yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));
             preScopeSig((yyvsp[(1) - (3)].str));
           }
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 1000 "vtkParse.y"
    {
             (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (3)].str))+strlen((yyvsp[(3) - (3)].str))+3);
             sprintf((yyval.str), "%s::%s", (yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));
             preScopeSig((yyvsp[(1) - (3)].str));
           }
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 1006 "vtkParse.y"
    {
             (yyval.str) = (char *)malloc(strlen((yyvsp[(1) - (3)].str))+strlen((yyvsp[(3) - (3)].str))+3);
             sprintf((yyval.str), "%s::%s", (yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str));
             preScopeSig("");
           }
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 1021 "vtkParse.y"
    { postSig("&"); (yyval.integer) = VTK_PARSE_REF;}
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 1022 "vtkParse.y"
    { postSig("*"); (yyval.integer) = VTK_PARSE_POINTER;}
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 1023 "vtkParse.y"
    { postSig("*&"); (yyval.integer) = VTK_PARSE_POINTER_REF;}
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 1024 "vtkParse.y"
    { postSig("**"); (yyval.integer) = VTK_PARSE_POINTER_POINTER;}
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 1026 "vtkParse.y"
    { postSig("* const&"); (yyval.integer) = VTK_PARSE_POINTER_CONST_REF;}
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 1028 "vtkParse.y"
    { postSig("* const*"); (yyval.integer) = VTK_PARSE_POINTER_CONST_POINTER;}
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 1029 "vtkParse.y"
    { postSig("const&"); (yyval.integer) = VTK_PARSE_BAD_INDIRECT;}
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 1030 "vtkParse.y"
    { postSig("const*"); (yyval.integer) = VTK_PARSE_BAD_INDIRECT;}
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 1031 "vtkParse.y"
    { postSig("**"); }
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 1032 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_BAD_INDIRECT;}
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 1033 "vtkParse.y"
    { postSig("**"); }
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 1034 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_BAD_INDIRECT;}
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 1035 "vtkParse.y"
    { postSig("const&");}
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 1036 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_BAD_INDIRECT;}
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 1037 "vtkParse.y"
    { postSig("const*");}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 1038 "vtkParse.y"
    { (yyval.integer) = VTK_PARSE_BAD_INDIRECT;}
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 1040 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 1041 "vtkParse.y"
    { typeSig("vtkStdString"); (yyval.integer) = VTK_PARSE_STRING;}
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 1043 "vtkParse.y"
    { typeSig("vtkUnicodeString"); (yyval.integer) = VTK_PARSE_UNICODE_STRING;}
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 1045 "vtkParse.y"
    { typeSig("ostream"); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 1046 "vtkParse.y"
    { typeSig("istream"); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 1047 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_UNKNOWN; }
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 1048 "vtkParse.y"
    { typeSig((yyvsp[(1) - (1)].str)); (yyval.integer) = VTK_PARSE_VTK_OBJECT; }
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 1051 "vtkParse.y"
    { typeSig("void"); (yyval.integer) = VTK_PARSE_VOID;}
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 1052 "vtkParse.y"
    { typeSig("float"); (yyval.integer) = VTK_PARSE_FLOAT;}
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 1053 "vtkParse.y"
    { typeSig("double"); (yyval.integer) = VTK_PARSE_DOUBLE;}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 1054 "vtkParse.y"
    { typeSig("bool"); (yyval.integer) = VTK_PARSE_BOOL;}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 1055 "vtkParse.y"
    {typeSig("signed char"); (yyval.integer) = VTK_PARSE_SIGNED_CHAR;}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 1056 "vtkParse.y"
    { typeSig("vtkTypeInt8"); (yyval.integer) = VTK_PARSE_INT8; }
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 1057 "vtkParse.y"
    { typeSig("vtkTypeUInt8"); (yyval.integer) = VTK_PARSE_UINT8; }
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 1058 "vtkParse.y"
    { typeSig("vtkTypeInt16"); (yyval.integer) = VTK_PARSE_INT16; }
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 1059 "vtkParse.y"
    { typeSig("vtkTypeUInt16"); (yyval.integer) = VTK_PARSE_UINT16; }
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 1060 "vtkParse.y"
    { typeSig("vtkTypeInt32"); (yyval.integer) = VTK_PARSE_INT32; }
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 1061 "vtkParse.y"
    { typeSig("vtkTypeUInt32"); (yyval.integer) = VTK_PARSE_UINT32; }
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 1062 "vtkParse.y"
    { typeSig("vtkTypeInt64"); (yyval.integer) = VTK_PARSE_INT64; }
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 1063 "vtkParse.y"
    { typeSig("vtkTypeUInt64"); (yyval.integer) = VTK_PARSE_UINT64; }
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 1064 "vtkParse.y"
    { typeSig("vtkTypeFloat32"); (yyval.integer) = VTK_PARSE_FLOAT32; }
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 1065 "vtkParse.y"
    { typeSig("vtkTypeFloat64"); (yyval.integer) = VTK_PARSE_FLOAT64; }
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 1066 "vtkParse.y"
    {typeSig("unsigned");}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 1067 "vtkParse.y"
    { (yyval.integer) = (VTK_PARSE_UNSIGNED | (yyvsp[(3) - (3)].integer));}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 1068 "vtkParse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer);}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 1071 "vtkParse.y"
    { typeSig("char"); (yyval.integer) = VTK_PARSE_CHAR;}
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 1072 "vtkParse.y"
    { typeSig("int"); (yyval.integer) = VTK_PARSE_INT;}
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 1073 "vtkParse.y"
    { typeSig("short"); (yyval.integer) = VTK_PARSE_SHORT;}
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 1074 "vtkParse.y"
    { typeSig("long"); (yyval.integer) = VTK_PARSE_LONG;}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 1075 "vtkParse.y"
    { typeSig("vtkIdType"); (yyval.integer) = VTK_PARSE_ID_TYPE;}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 1076 "vtkParse.y"
    { typeSig("long long"); (yyval.integer) = VTK_PARSE_LONG_LONG;}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 1077 "vtkParse.y"
    { typeSig("__int64"); (yyval.integer) = VTK_PARSE___INT64;}
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 1082 "vtkParse.y"
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
#line 1090 "vtkParse.y"
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
#line 1098 "vtkParse.y"
    {in_public = 0; in_protected = 0;}
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 1099 "vtkParse.y"
    {in_public = 1; in_protected = 0;}
    break;

  case 294:

/* Line 1455 of yacc.c  */
#line 1100 "vtkParse.y"
    {in_public = 0; in_protected = 0;}
    break;

  case 295:

/* Line 1455 of yacc.c  */
#line 1101 "vtkParse.y"
    {in_public = 0; in_protected = 1;}
    break;

  case 296:

/* Line 1455 of yacc.c  */
#line 1103 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 1104 "vtkParse.y"
    {(yyval.str) = (yyvsp[(2) - (2)].str);}
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 1105 "vtkParse.y"
    {(yyval.str) = (char *)malloc(strlen((yyvsp[(2) - (2)].str))+2);
                        sprintf((yyval.str), "-%s", (yyvsp[(2) - (2)].str)); }
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 1107 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 1108 "vtkParse.y"
    {(yyval.str) = (yyvsp[(2) - (3)].str);}
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 1110 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 302:

/* Line 1455 of yacc.c  */
#line 1111 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 303:

/* Line 1455 of yacc.c  */
#line 1112 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 304:

/* Line 1455 of yacc.c  */
#line 1113 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 305:

/* Line 1455 of yacc.c  */
#line 1114 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 1115 "vtkParse.y"
    {(yyval.str) = (yyvsp[(1) - (1)].str);}
    break;

  case 307:

/* Line 1455 of yacc.c  */
#line 1118 "vtkParse.y"
    {preSig("void Set"); postSig("(");}
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 1119 "vtkParse.y"
    {
   postSig("a);");
   sprintf(temps,"Set%s",(yyvsp[(3) - (7)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (yyvsp[(6) - (7)].integer);
   currentFunction->ArgClasses[0] = vtkstrdup(getTypeId());
   currentFunction->ArgCounts[0] = 0;
   output_function();
   }
    break;

  case 309:

/* Line 1455 of yacc.c  */
#line 1129 "vtkParse.y"
    {postSig("Get");}
    break;

  case 310:

/* Line 1455 of yacc.c  */
#line 1130 "vtkParse.y"
    {markSig();}
    break;

  case 311:

/* Line 1455 of yacc.c  */
#line 1130 "vtkParse.y"
    {swapSig();}
    break;

  case 312:

/* Line 1455 of yacc.c  */
#line 1131 "vtkParse.y"
    {
   postSig("();");
   sprintf(temps,"Get%s",(yyvsp[(4) - (9)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->ReturnType = (yyvsp[(7) - (9)].integer);
   currentFunction->ReturnClass = vtkstrdup(getTypeId());
   output_function();
   }
    break;

  case 313:

/* Line 1455 of yacc.c  */
#line 1139 "vtkParse.y"
    {preSig("void Set");}
    break;

  case 314:

/* Line 1455 of yacc.c  */
#line 1140 "vtkParse.y"
    {
   postSig("(char *);");
   sprintf(temps,"Set%s",(yyvsp[(4) - (5)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = VTK_PARSE_CHAR_PTR;
   currentFunction->ArgClasses[0] = vtkstrdup("char");
   currentFunction->ArgCounts[0] = 0;
   output_function();
   }
    break;

  case 315:

/* Line 1455 of yacc.c  */
#line 1150 "vtkParse.y"
    {preSig("char *Get");}
    break;

  case 316:

/* Line 1455 of yacc.c  */
#line 1151 "vtkParse.y"
    {
   postSig("();");
   sprintf(temps,"Get%s",(yyvsp[(4) - (5)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = VTK_PARSE_CHAR_PTR;
   currentFunction->ReturnClass = vtkstrdup("char");
   output_function();
   }
    break;

  case 317:

/* Line 1455 of yacc.c  */
#line 1160 "vtkParse.y"
    {delSig(); markSig();}
    break;

  case 318:

/* Line 1455 of yacc.c  */
#line 1160 "vtkParse.y"
    {closeSig();}
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 1162 "vtkParse.y"
    {
   char *local;
   chopSig();
   local = vtkstrdup(copySig());
   sprintf(currentFunction->Signature,"void Set%s(%s);",(yyvsp[(3) - (10)].str),local);
   sprintf(temps,"Set%s",(yyvsp[(3) - (10)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (yyvsp[(6) - (10)].integer);
   currentFunction->ArgClasses[0] = vtkstrdup(getTypeId());
   currentFunction->ArgCounts[0] = 0;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%sGet%sMinValue();",local,(yyvsp[(3) - (10)].str));
   sprintf(temps,"Get%sMinValue",(yyvsp[(3) - (10)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->ReturnType = (yyvsp[(6) - (10)].integer);
   currentFunction->ReturnClass = vtkstrdup(getTypeId());
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%sGet%sMaxValue();",local,(yyvsp[(3) - (10)].str));
   sprintf(temps,"Get%sMaxValue",(yyvsp[(3) - (10)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->ReturnType = (yyvsp[(6) - (10)].integer);
   currentFunction->ReturnClass = vtkstrdup(getTypeId());
   output_function();
   free(local);
   }
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 1195 "vtkParse.y"
    {preSig("void Set"); postSig("("); }
    break;

  case 321:

/* Line 1455 of yacc.c  */
#line 1196 "vtkParse.y"
    {
   postSig("*);");
   sprintf(temps,"Set%s",(yyvsp[(3) - (7)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = VTK_PARSE_VTK_OBJECT_PTR;
   currentFunction->ArgClasses[0] = vtkstrdup(getTypeId());
   currentFunction->ArgCounts[0] = 1;
   output_function();
   }
    break;

  case 322:

/* Line 1455 of yacc.c  */
#line 1206 "vtkParse.y"
    {postSig("*Get");}
    break;

  case 323:

/* Line 1455 of yacc.c  */
#line 1207 "vtkParse.y"
    {markSig();}
    break;

  case 324:

/* Line 1455 of yacc.c  */
#line 1207 "vtkParse.y"
    {swapSig();}
    break;

  case 325:

/* Line 1455 of yacc.c  */
#line 1208 "vtkParse.y"
    {
   postSig("();");
   sprintf(temps,"Get%s",(yyvsp[(4) - (9)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->ReturnType = VTK_PARSE_VTK_OBJECT_PTR;
   currentFunction->ReturnClass = vtkstrdup(getTypeId());
   output_function();
   }
    break;

  case 326:

/* Line 1455 of yacc.c  */
#line 1217 "vtkParse.y"
    {
   sprintf(temps,"%sOn",(yyvsp[(3) - (6)].str));
   currentFunction->Name = vtkstrdup(temps);
   delSig();
   postSig("void ");
   postSig(temps);
   postSig("();");
   output_function();

   sprintf(temps,"%sOff",(yyvsp[(3) - (6)].str));
   currentFunction->Name = vtkstrdup(temps);
   delSig();
   postSig("void ");
   postSig(temps);
   postSig("();");
   output_function();
   }
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 1234 "vtkParse.y"
    {delSig(); markSig();}
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 1235 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 2);
   }
    break;

  case 329:

/* Line 1455 of yacc.c  */
#line 1239 "vtkParse.y"
    {delSig(); markSig();}
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 1240 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 2);
   }
    break;

  case 331:

/* Line 1455 of yacc.c  */
#line 1244 "vtkParse.y"
    {delSig(); markSig();}
    break;

  case 332:

/* Line 1455 of yacc.c  */
#line 1245 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 3);
   }
    break;

  case 333:

/* Line 1455 of yacc.c  */
#line 1249 "vtkParse.y"
    {delSig(); markSig();}
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 1250 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 3);
   }
    break;

  case 335:

/* Line 1455 of yacc.c  */
#line 1254 "vtkParse.y"
    {delSig(); markSig();}
    break;

  case 336:

/* Line 1455 of yacc.c  */
#line 1255 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 4);
   }
    break;

  case 337:

/* Line 1455 of yacc.c  */
#line 1259 "vtkParse.y"
    {delSig(); markSig();}
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 1260 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 4);
   }
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 1264 "vtkParse.y"
    {delSig(); markSig();}
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 1265 "vtkParse.y"
    {
   chopSig();
   outputSetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 6);
   }
    break;

  case 341:

/* Line 1455 of yacc.c  */
#line 1269 "vtkParse.y"
    {delSig(); markSig();}
    break;

  case 342:

/* Line 1455 of yacc.c  */
#line 1270 "vtkParse.y"
    {
   chopSig();
   outputGetVectorMacro((yyvsp[(3) - (7)].str), (yyvsp[(6) - (7)].integer), copySig(), 6);
   }
    break;

  case 343:

/* Line 1455 of yacc.c  */
#line 1274 "vtkParse.y"
    {delSig(); markSig();}
    break;

  case 344:

/* Line 1455 of yacc.c  */
#line 1276 "vtkParse.y"
    {
   char *local;
   chopSig();
   local = vtkstrdup(copySig());
   sprintf(currentFunction->Signature,"void Set%s(%s a[%s]);",
           (yyvsp[(3) - (9)].str), local, (yyvsp[(8) - (9)].str));
   sprintf(temps,"Set%s",(yyvsp[(3) - (9)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (VTK_PARSE_POINTER | (yyvsp[(6) - (9)].integer));
   currentFunction->ArgClasses[0] = vtkstrdup(getTypeId());
   currentFunction->ArgCounts[0] = atol((yyvsp[(8) - (9)].str));
   output_function();
   free(local);
   }
    break;

  case 345:

/* Line 1455 of yacc.c  */
#line 1291 "vtkParse.y"
    {delSig(); markSig();}
    break;

  case 346:

/* Line 1455 of yacc.c  */
#line 1293 "vtkParse.y"
    {
   char *local;
   chopSig();
   local = vtkstrdup(copySig());
   sprintf(currentFunction->Signature,"%s *Get%s();", local, (yyvsp[(3) - (9)].str));
   sprintf(temps,"Get%s",(yyvsp[(3) - (9)].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (VTK_PARSE_POINTER | (yyvsp[(6) - (9)].integer));
   currentFunction->ReturnClass = vtkstrdup(getTypeId());
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = atol((yyvsp[(8) - (9)].str));
   output_function();
   free(local);
   }
    break;

  case 347:

/* Line 1455 of yacc.c  */
#line 1309 "vtkParse.y"
    {
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate();",
             (yyvsp[(3) - (4)].str));

     sprintf(temps,"Get%sCoordinate",(yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = VTK_PARSE_VTK_OBJECT_PTR;
     currentFunction->ReturnClass = "vtkCoordinate";
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s(double, double);",
             (yyvsp[(3) - (4)].str));
     sprintf(temps,"Set%s",(yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 2;
     currentFunction->ArgTypes[0] = VTK_PARSE_DOUBLE;
     currentFunction->ArgClasses[0] = vtkstrdup("double");
     currentFunction->ArgCounts[0] = 0;
     currentFunction->ArgTypes[1] = VTK_PARSE_DOUBLE;
     currentFunction->ArgClasses[1] = vtkstrdup("double");
     currentFunction->ArgCounts[1] = 0;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s(double a[2]);",
             (yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = VTK_PARSE_DOUBLE_PTR;
     currentFunction->ArgClasses[0] = vtkstrdup("double");
     currentFunction->ArgCounts[0] = 2;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s();", (yyvsp[(3) - (4)].str));
     sprintf(temps,"Get%s",(yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = VTK_PARSE_DOUBLE_PTR;
     currentFunction->ReturnClass = vtkstrdup("double");
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 2;
     output_function();
   }
    break;

  case 348:

/* Line 1455 of yacc.c  */
#line 1359 "vtkParse.y"
    {
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate();",
             (yyvsp[(3) - (4)].str));

     sprintf(temps,"Get%sCoordinate",(yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = VTK_PARSE_VTK_OBJECT_PTR;
     currentFunction->ReturnClass = "vtkCoordinate";
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,
             "void Set%s(double, double, double);",
             (yyvsp[(3) - (4)].str));
     sprintf(temps,"Set%s",(yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 3;
     currentFunction->ArgTypes[0] = VTK_PARSE_DOUBLE;
     currentFunction->ArgClasses[0] = vtkstrdup("double");
     currentFunction->ArgCounts[0] = 0;
     currentFunction->ArgTypes[1] = VTK_PARSE_DOUBLE;
     currentFunction->ArgClasses[1] = vtkstrdup("double");
     currentFunction->ArgCounts[1] = 0;
     currentFunction->ArgTypes[2] = VTK_PARSE_DOUBLE;
     currentFunction->ArgClasses[2] = vtkstrdup("double");
     currentFunction->ArgCounts[2] = 0;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s(double a[3]);",
             (yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = VTK_PARSE_DOUBLE_PTR;
     currentFunction->ArgClasses[0] = vtkstrdup("double");
     currentFunction->ArgCounts[0] = 3;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s();", (yyvsp[(3) - (4)].str));
     sprintf(temps,"Get%s",(yyvsp[(3) - (4)].str));
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = VTK_PARSE_DOUBLE_PTR;
     currentFunction->ReturnClass = vtkstrdup("double");
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 3;
     output_function();
   }
    break;

  case 349:

/* Line 1455 of yacc.c  */
#line 1413 "vtkParse.y"
    {
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "const char *GetClassName();");
   sprintf(temps,"GetClassName");
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR);
   currentFunction->ReturnClass = vtkstrdup("char");
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "int IsA(const char *name);");
   sprintf(temps,"IsA");
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR);
   currentFunction->ArgClasses[0] = vtkstrdup("char");
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = VTK_PARSE_INT;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "%s *NewInstance();", (yyvsp[(3) - (6)].str));
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

  case 350:

/* Line 1455 of yacc.c  */
#line 1464 "vtkParse.y"
    {
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "const char *GetClassName();");
   sprintf(temps,"GetClassName");
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR);
   currentFunction->ReturnClass = vtkstrdup("char");
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
   currentFunction->ReturnClass = vtkstrdup("int");
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "%s *NewInstance();", (yyvsp[(3) - (7)].str));
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
     currentFunction->ReturnType = (VTK_PARSE_STATIC|VTK_PARSE_VTK_OBJECT_PTR);
     currentFunction->ReturnClass = vtkstrdup((yyvsp[(3) - (7)].str));
     output_function();
     }
   }
    break;

  case 351:

/* Line 1455 of yacc.c  */
#line 1521 "vtkParse.y"
    { (yyval.str) = "operator()"; }
    break;

  case 352:

/* Line 1455 of yacc.c  */
#line 1522 "vtkParse.y"
    { (yyval.str) = "operator[]"; }
    break;

  case 353:

/* Line 1455 of yacc.c  */
#line 1523 "vtkParse.y"
    { (yyval.str) = "operator new[]"; }
    break;

  case 354:

/* Line 1455 of yacc.c  */
#line 1524 "vtkParse.y"
    { (yyval.str) = "operator delete[]"; }
    break;

  case 356:

/* Line 1455 of yacc.c  */
#line 1527 "vtkParse.y"
    { (yyval.str) = "operator="; }
    break;

  case 357:

/* Line 1455 of yacc.c  */
#line 1528 "vtkParse.y"
    { (yyval.str) = "operator*"; }
    break;

  case 358:

/* Line 1455 of yacc.c  */
#line 1528 "vtkParse.y"
    { (yyval.str) = "operator/"; }
    break;

  case 359:

/* Line 1455 of yacc.c  */
#line 1529 "vtkParse.y"
    { (yyval.str) = "operator-"; }
    break;

  case 360:

/* Line 1455 of yacc.c  */
#line 1529 "vtkParse.y"
    { (yyval.str) = "operator+"; }
    break;

  case 361:

/* Line 1455 of yacc.c  */
#line 1530 "vtkParse.y"
    { (yyval.str) = "operator!"; }
    break;

  case 362:

/* Line 1455 of yacc.c  */
#line 1530 "vtkParse.y"
    { (yyval.str) = "operator~"; }
    break;

  case 363:

/* Line 1455 of yacc.c  */
#line 1531 "vtkParse.y"
    { (yyval.str) = "operator,"; }
    break;

  case 364:

/* Line 1455 of yacc.c  */
#line 1531 "vtkParse.y"
    { (yyval.str) = "operator<"; }
    break;

  case 365:

/* Line 1455 of yacc.c  */
#line 1532 "vtkParse.y"
    { (yyval.str) = "operator>"; }
    break;

  case 366:

/* Line 1455 of yacc.c  */
#line 1532 "vtkParse.y"
    { (yyval.str) = "operator&"; }
    break;

  case 367:

/* Line 1455 of yacc.c  */
#line 1533 "vtkParse.y"
    { (yyval.str) = "operator|"; }
    break;

  case 368:

/* Line 1455 of yacc.c  */
#line 1533 "vtkParse.y"
    { (yyval.str) = "operator^"; }
    break;

  case 369:

/* Line 1455 of yacc.c  */
#line 1534 "vtkParse.y"
    { (yyval.str) = "operator%"; }
    break;

  case 370:

/* Line 1455 of yacc.c  */
#line 1535 "vtkParse.y"
    { (yyval.str) = "operator new"; }
    break;

  case 371:

/* Line 1455 of yacc.c  */
#line 1536 "vtkParse.y"
    { (yyval.str) = "operator delete"; }
    break;

  case 372:

/* Line 1455 of yacc.c  */
#line 1537 "vtkParse.y"
    { (yyval.str) = "operator<<="; }
    break;

  case 373:

/* Line 1455 of yacc.c  */
#line 1538 "vtkParse.y"
    { (yyval.str) = "operator>>="; }
    break;

  case 374:

/* Line 1455 of yacc.c  */
#line 1539 "vtkParse.y"
    { (yyval.str) = "operator<<"; }
    break;

  case 375:

/* Line 1455 of yacc.c  */
#line 1540 "vtkParse.y"
    { (yyval.str) = "operator>>"; }
    break;

  case 376:

/* Line 1455 of yacc.c  */
#line 1541 "vtkParse.y"
    { (yyval.str) = "operator->*"; }
    break;

  case 377:

/* Line 1455 of yacc.c  */
#line 1542 "vtkParse.y"
    { (yyval.str) = "operator->"; }
    break;

  case 378:

/* Line 1455 of yacc.c  */
#line 1543 "vtkParse.y"
    { (yyval.str) = "operator+="; }
    break;

  case 379:

/* Line 1455 of yacc.c  */
#line 1544 "vtkParse.y"
    { (yyval.str) = "operator-="; }
    break;

  case 380:

/* Line 1455 of yacc.c  */
#line 1545 "vtkParse.y"
    { (yyval.str) = "operator*="; }
    break;

  case 381:

/* Line 1455 of yacc.c  */
#line 1546 "vtkParse.y"
    { (yyval.str) = "operator/="; }
    break;

  case 382:

/* Line 1455 of yacc.c  */
#line 1547 "vtkParse.y"
    { (yyval.str) = "operator%="; }
    break;

  case 383:

/* Line 1455 of yacc.c  */
#line 1548 "vtkParse.y"
    { (yyval.str) = "operator++"; }
    break;

  case 384:

/* Line 1455 of yacc.c  */
#line 1549 "vtkParse.y"
    { (yyval.str) = "operator--"; }
    break;

  case 385:

/* Line 1455 of yacc.c  */
#line 1550 "vtkParse.y"
    { (yyval.str) = "operator&="; }
    break;

  case 386:

/* Line 1455 of yacc.c  */
#line 1551 "vtkParse.y"
    { (yyval.str) = "operator|="; }
    break;

  case 387:

/* Line 1455 of yacc.c  */
#line 1552 "vtkParse.y"
    { (yyval.str) = "operator^="; }
    break;

  case 388:

/* Line 1455 of yacc.c  */
#line 1553 "vtkParse.y"
    {(yyval.str) = "operator&&=";}
    break;

  case 389:

/* Line 1455 of yacc.c  */
#line 1554 "vtkParse.y"
    {(yyval.str) = "operator||=";}
    break;

  case 390:

/* Line 1455 of yacc.c  */
#line 1555 "vtkParse.y"
    { (yyval.str) = "operator&&"; }
    break;

  case 391:

/* Line 1455 of yacc.c  */
#line 1556 "vtkParse.y"
    { (yyval.str) = "operator||"; }
    break;

  case 392:

/* Line 1455 of yacc.c  */
#line 1557 "vtkParse.y"
    { (yyval.str) = "operator=="; }
    break;

  case 393:

/* Line 1455 of yacc.c  */
#line 1558 "vtkParse.y"
    { (yyval.str) = "operator!="; }
    break;

  case 394:

/* Line 1455 of yacc.c  */
#line 1559 "vtkParse.y"
    { (yyval.str) = "operator<="; }
    break;

  case 395:

/* Line 1455 of yacc.c  */
#line 1560 "vtkParse.y"
    { (yyval.str) = "operator>="; }
    break;



/* Line 1455 of yacc.c  */
#line 5530 "vtkParse.tab.c"
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
  yyerrstatus = 3;        /* Each real token shifted decrements this.  */

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
#line 1597 "vtkParse.y"

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
  sigClosed = 0;
  sigMarkDepth = 0;
  sigMark[0] = 0;
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

  /* reject template specializations */
  if (currentFunction->Name[strlen(currentFunction->Name)-1] == '>')
    {
    reject_function();
    return;
    }

  /* a void argument is the same as no arguements */
  if ((currentFunction->ArgTypes[0] & VTK_PARSE_UNQUALIFIED_TYPE) ==
      VTK_PARSE_VOID)
    {
    currentFunction->NumberOfArguments = 0;
    }

  /* if return type is void, set return class to void */
  if (currentFunction->ReturnClass == NULL &&
      (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE) ==
       VTK_PARSE_VOID)
    {
    currentFunction->ReturnClass = vtkstrdup("void");
    }

  /* set public, protected */
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

void outputSetVectorMacro(
  const char *var, int argType, const char *typeText, int n)
{
  char *argClass = vtkstrdup(getTypeId());
  char *local = vtkstrdup(typeText);
  char *name;
  int i;

  sprintf(temps,"Set%s", var);
  name = vtkstrdup(temps);

  sprintf(currentFunction->Signature, "void Set%s(%s", var, local);
  for (i = 1; i < n; i++)
    {
    postSig(", ");
    postSig(local);
    }
  postSig(");");
  currentFunction->Name = name;
  currentFunction->NumberOfArguments = n;
  for (i = 0; i < n; i++)
    {
    currentFunction->ArgTypes[i] = argType;
    currentFunction->ArgClasses[i] = argClass;
    currentFunction->ArgCounts[i] = 0;
    }
  output_function();

  currentFunction->Signature = (char *)malloc(2048);
  sigAllocatedLength = 2048;
  sprintf(currentFunction->Signature, "void Set%s(%s a[%i]);",
          var, local, n);
  currentFunction->Name = name;
  currentFunction->NumberOfArguments = 1;
  currentFunction->ArgTypes[0] = (VTK_PARSE_POINTER | argType);
  currentFunction->ArgClasses[0] = vtkstrdup(getTypeId());
  currentFunction->ArgCounts[0] = n;
  output_function();

  free(local);
}

void outputGetVectorMacro(
  const char *var, int argType, const char *typeText, int n)
{
  char *local = vtkstrdup(typeText);

  sprintf(currentFunction->Signature, "%s *Get%s();", local, var);
  sprintf(temps, "Get%s", var);
  currentFunction->Name = vtkstrdup(temps);
  currentFunction->NumberOfArguments = 0;
  currentFunction->ReturnType = (VTK_PARSE_POINTER | argType);
  currentFunction->ReturnClass = vtkstrdup(getTypeId());
  currentFunction->HaveHint = 1;
  currentFunction->HintSize = n;
  output_function();

  free(local);
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

