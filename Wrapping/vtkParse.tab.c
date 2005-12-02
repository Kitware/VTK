/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

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

/* Bison version.  */
#define YYBISON_VERSION "2.1"

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
     TypeInt8 = 287,
     TypeUInt8 = 288,
     TypeInt16 = 289,
     TypeUInt16 = 290,
     TypeInt32 = 291,
     TypeUInt32 = 292,
     TypeInt64 = 293,
     TypeUInt64 = 294,
     TypeFloat32 = 295,
     TypeFloat64 = 296,
     IdType = 297,
     StdString = 298,
     SetMacro = 299,
     GetMacro = 300,
     SetStringMacro = 301,
     GetStringMacro = 302,
     SetClampMacro = 303,
     SetObjectMacro = 304,
     SetReferenceCountedObjectMacro = 305,
     GetObjectMacro = 306,
     BooleanMacro = 307,
     SetVector2Macro = 308,
     SetVector3Macro = 309,
     SetVector4Macro = 310,
     SetVector6Macro = 311,
     GetVector2Macro = 312,
     GetVector3Macro = 313,
     GetVector4Macro = 314,
     GetVector6Macro = 315,
     SetVectorMacro = 316,
     GetVectorMacro = 317,
     ViewportCoordinateMacro = 318,
     WorldCoordinateMacro = 319,
     TypeMacro = 320
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
#define TypeInt8 287
#define TypeUInt8 288
#define TypeInt16 289
#define TypeUInt16 290
#define TypeInt32 291
#define TypeUInt32 292
#define TypeInt64 293
#define TypeUInt64 294
#define TypeFloat32 295
#define TypeFloat64 296
#define IdType 297
#define StdString 298
#define SetMacro 299
#define GetMacro 300
#define SetStringMacro 301
#define GetStringMacro 302
#define SetClampMacro 303
#define SetObjectMacro 304
#define SetReferenceCountedObjectMacro 305
#define GetObjectMacro 306
#define BooleanMacro 307
#define SetVector2Macro 308
#define SetVector3Macro 309
#define SetVector4Macro 310
#define SetVector6Macro 311
#define GetVector2Macro 312
#define GetVector3Macro 313
#define GetVector4Macro 314
#define GetVector6Macro 315
#define SetVectorMacro 316
#define GetVectorMacro 317
#define ViewportCoordinateMacro 318
#define WorldCoordinateMacro 319
#define TypeMacro 320




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

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 187 "vtkParse.y"
typedef union YYSTYPE {
  char *str;
  int   integer;
  } YYSTYPE;
/* Line 196 of yacc.c.  */
#line 392 "vtkParse.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
#line 404 "vtkParse.tab.c"

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T) && (defined (__STDC__) || defined (__cplusplus))
# include <stddef.h> /* INFRINGES ON USER NAME SPACE */
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

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

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if defined (__STDC__) || defined (__cplusplus)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     define YYINCLUDED_STDLIB_H
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2005 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM ((YYSIZE_T) -1)
#  endif
#  ifdef __cplusplus
extern "C" {
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if (! defined (malloc) && ! defined (YYINCLUDED_STDLIB_H) \
        && (defined (__STDC__) || defined (__cplusplus)))
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if (! defined (free) && ! defined (YYINCLUDED_STDLIB_H) \
        && (defined (__STDC__) || defined (__cplusplus)))
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifdef __cplusplus
}
#  endif
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
          YYSIZE_T yyi;                         \
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
#define YYFINAL  60
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   820

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  81
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  74
/* YYNRULES -- Number of rules. */
#define YYNRULES  193
/* YYNRULES -- Number of states. */
#define YYNSTATES  365

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   320

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    77,     2,
      70,    71,    78,     2,    74,    79,    80,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    68,    69,
       2,    73,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    75,     2,    76,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    66,     2,    67,    72,     2,     2,     2,
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
      65
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
     252,   254,   256,   258,   260,   262,   264,   266,   268,   270,
     272,   274,   276,   278,   280,   282,   284,   286,   287,   290,
     293,   294,   300,   302,   304,   306,   309,   311,   313,   317,
     319,   320,   328,   329,   330,   339,   340,   346,   347,   353,
     354,   355,   366,   367,   375,   376,   384,   385,   386,   395,
     396,   404,   405,   413,   414,   422,   423,   431,   432,   440,
     441,   449,   450,   458,   459,   467,   468,   476,   477,   487,
     488,   498,   503,   508,   515,   516,   519,   520,   523,   525,
     527,   529,   531,   533,   535,   537,   539,   541,   543,   545,
     547,   549,   551,   553,   555,   557,   559,   561,   563,   565,
     567,   569,   573,   577
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short int yyrhs[] =
{
      82,     0,    -1,   148,    83,   148,    -1,    -1,     3,    27,
      84,   119,    66,    85,    67,    -1,    86,    -1,    86,    85,
      -1,   122,    68,    -1,   107,    -1,    89,    -1,    26,    89,
      -1,    88,   100,    -1,    26,    88,   100,    -1,    87,   100,
      -1,   125,    69,    -1,   125,    -1,    31,    70,    88,    71,
      -1,    72,    91,    -1,     7,    72,    91,    -1,    91,    -1,
     111,    91,    -1,   111,    23,    91,    -1,     7,   111,    23,
      91,    -1,     7,   111,    91,    -1,     7,    91,    -1,    90,
      -1,   111,    90,    -1,   111,    23,    90,    -1,     7,   111,
      23,    90,    -1,     7,   111,    90,    -1,     7,    90,    -1,
      24,   149,    69,    -1,    -1,    -1,    95,    92,    94,    93,
      -1,    95,    73,     9,    -1,    -1,    23,    -1,    -1,    99,
      70,    96,   101,    71,    -1,    23,    -1,    28,    -1,    27,
      -1,    10,    -1,    69,    -1,    66,   148,    67,    69,    -1,
      66,   148,    67,    -1,    68,   149,    69,    -1,    -1,   102,
      -1,   104,    -1,    -1,   104,   103,    74,   102,    -1,   111,
      -1,    -1,   111,   108,   105,   106,    -1,    29,    -1,    -1,
      73,   123,    -1,   111,   108,    69,    -1,    29,    69,    -1,
      99,   109,    -1,    -1,    -1,    30,   110,   109,    -1,    75,
     149,    76,   109,    -1,    97,   112,    -1,   112,    -1,    98,
     112,    -1,    98,    97,   112,    -1,   116,    -1,   116,   115,
      -1,   113,    -1,   114,    -1,   114,    77,    -1,   114,    78,
      -1,    43,    -1,    77,    -1,    78,    -1,    77,   115,    -1,
      78,   115,    -1,    -1,    25,   117,   118,    -1,   118,    -1,
      32,    -1,    33,    -1,    34,    -1,    35,    -1,    36,    -1,
      37,    -1,    38,    -1,    39,    -1,    40,    -1,    41,    -1,
      12,    -1,    18,    -1,    19,    -1,    11,    -1,    13,    -1,
      14,    -1,    17,    -1,    10,    -1,    27,    -1,    42,    -1,
      15,    -1,    16,    -1,    20,    -1,    -1,    68,   120,    -1,
     122,    27,    -1,    -1,   122,    27,   121,    74,   120,    -1,
       4,    -1,     5,    -1,     6,    -1,    79,   124,    -1,   124,
      -1,     9,    -1,     9,    80,     9,    -1,    99,    -1,    -1,
      44,    70,    99,    74,   126,   116,    71,    -1,    -1,    -1,
      45,    70,   127,    99,    74,   128,   116,    71,    -1,    -1,
      46,    70,   129,    99,    71,    -1,    -1,    47,    70,   130,
      99,    71,    -1,    -1,    -1,    48,    70,    99,    74,   131,
     116,   132,    74,   149,    71,    -1,    -1,    49,    70,    99,
      74,   133,   116,    71,    -1,    -1,    50,    70,    99,    74,
     134,   116,    71,    -1,    -1,    -1,    51,    70,   135,    99,
      74,   136,   116,    71,    -1,    -1,    52,    70,    99,   137,
      74,   116,    71,    -1,    -1,    53,    70,    99,    74,   138,
     116,    71,    -1,    -1,    57,    70,    99,    74,   139,   116,
      71,    -1,    -1,    54,    70,    99,    74,   140,   116,    71,
      -1,    -1,    58,    70,    99,    74,   141,   116,    71,    -1,
      -1,    55,    70,    99,    74,   142,   116,    71,    -1,    -1,
      59,    70,    99,    74,   143,   116,    71,    -1,    -1,    56,
      70,    99,    74,   144,   116,    71,    -1,    -1,    60,    70,
      99,    74,   145,   116,    71,    -1,    -1,    61,    70,    99,
      74,   146,   116,    74,   123,    71,    -1,    -1,    62,    70,
      99,    74,   147,   116,    74,   123,    71,    -1,    63,    70,
      99,    71,    -1,    64,    70,    99,    71,    -1,    65,    70,
      99,    74,    99,    71,    -1,    -1,   150,   148,    -1,    -1,
     151,   149,    -1,    69,    -1,   151,    -1,    22,    -1,   152,
      -1,   153,    -1,    78,    -1,    73,    -1,    68,    -1,    74,
      -1,    80,    -1,     8,    -1,   116,    -1,   114,    -1,     9,
      -1,    21,    -1,    77,    -1,   154,    -1,    23,    -1,    24,
      -1,    79,    -1,    72,    -1,    28,    -1,    30,    -1,    66,
     148,    67,    -1,    70,   148,    71,    -1,    75,   148,    76,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   262,   262,   265,   264,   270,   270,   272,   272,   273,
     274,   275,   276,   277,   278,   279,   281,   283,   284,   285,
     286,   290,   294,   299,   304,   310,   314,   319,   324,   330,
     336,   342,   348,   348,   348,   354,   363,   363,   365,   365,
     367,   369,   371,   371,   373,   374,   375,   376,   378,   378,
     380,   381,   381,   383,   389,   388,   395,   402,   402,   404,
     404,   406,   414,   415,   415,   418,   421,   422,   423,   424,
     426,   427,   429,   431,   432,   433,   435,   445,   446,   447,
     448,   450,   450,   452,   455,   456,   457,   458,   459,   460,
     461,   462,   463,   464,   465,   466,   467,   468,   469,   470,
     471,   472,   477,   494,   495,   496,   497,   499,   499,   501,
     507,   506,   512,   513,   514,   516,   516,   518,   519,   519,
     523,   522,   534,   534,   534,   543,   543,   554,   554,   564,
     565,   563,   596,   595,   608,   607,   619,   620,   619,   629,
     628,   646,   645,   676,   675,   693,   692,   725,   724,   742,
     741,   776,   775,   793,   792,   831,   830,   848,   847,   866,
     865,   882,   929,   978,  1034,  1034,  1035,  1035,  1037,  1037,
    1039,  1039,  1039,  1039,  1039,  1039,  1039,  1039,  1040,  1040,
    1040,  1040,  1040,  1040,  1040,  1041,  1041,  1041,  1041,  1041,
    1041,  1043,  1044,  1045
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "CLASS", "PUBLIC", "PRIVATE",
  "PROTECTED", "VIRTUAL", "STRING", "NUM", "ID", "INT", "FLOAT", "SHORT",
  "LONG", "LONG_LONG", "INT64__", "DOUBLE", "VOID", "CHAR", "SIGNED_CHAR",
  "CLASS_REF", "OTHER", "CONST", "OPERATOR", "UNSIGNED", "FRIEND",
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
static const unsigned short int yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   123,   125,    58,    59,
      40,    41,   126,    61,    44,    91,    93,    38,    42,    45,
      46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    81,    82,    84,    83,    85,    85,    86,    86,    86,
      86,    86,    86,    86,    86,    86,    87,    88,    88,    88,
      88,    88,    88,    88,    88,    89,    89,    89,    89,    89,
      89,    90,    92,    93,    91,    91,    94,    94,    96,    95,
      97,    98,    99,    99,   100,   100,   100,   100,   101,   101,
     102,   103,   102,   104,   105,   104,   104,   106,   106,   107,
     107,   108,   109,   110,   109,   109,   111,   111,   111,   111,
     112,   112,   112,   113,   113,   113,   114,   115,   115,   115,
     115,   117,   116,   116,   118,   118,   118,   118,   118,   118,
     118,   118,   118,   118,   118,   118,   118,   118,   118,   118,
     118,   118,   118,   118,   118,   118,   118,   119,   119,   120,
     121,   120,   122,   122,   122,   123,   123,   124,   124,   124,
     126,   125,   127,   128,   125,   129,   125,   130,   125,   131,
     132,   125,   133,   125,   134,   125,   135,   136,   125,   137,
     125,   138,   125,   139,   125,   140,   125,   141,   125,   142,
     125,   143,   125,   144,   125,   145,   125,   146,   125,   147,
     125,   125,   125,   125,   148,   148,   149,   149,   150,   150,
     151,   151,   151,   151,   151,   151,   151,   151,   151,   151,
     151,   151,   151,   151,   151,   151,   151,   151,   151,   151,
     151,   152,   153,   154
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
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
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
     164,   178,   181,   101,    97,    94,    98,    99,   104,   105,
     100,    95,    96,   106,   182,   170,   185,   186,    81,   102,
     189,   190,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,   103,    76,   164,   175,   168,   164,   188,   174,
     176,   164,   183,   173,   187,   177,     0,   180,   179,    83,
       0,   164,   169,   171,   172,   184,     0,     0,     0,     0,
       1,     0,   164,   165,    82,   191,   192,   193,     3,     2,
     107,     0,     0,   112,   113,   114,   108,     0,     0,   109,
       0,   101,    40,   166,     0,   102,    41,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     5,     0,     0,     9,    25,    19,    32,
       0,     0,     0,     8,     0,    67,    72,    73,    70,     0,
      15,     0,     0,    30,    24,     0,     0,   166,     0,    10,
       0,    60,     0,     0,   122,   125,   127,     0,     0,     0,
     136,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    43,    42,    17,     4,     6,
     164,   166,    44,    13,    11,     0,    36,    66,     0,    68,
      38,     0,    26,    20,    62,     0,    74,    75,    77,    78,
      71,     7,    14,     0,    18,     0,    29,    23,    31,   167,
      12,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   139,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    35,    37,
      33,    69,    48,    27,    21,    63,   166,    61,    59,    79,
      80,   111,    28,    22,     0,    16,     0,   120,     0,     0,
       0,   129,   132,   134,     0,     0,   141,   145,   149,   153,
     143,   147,   151,   155,   157,   159,   161,   162,     0,    46,
      47,    34,    56,     0,    49,    50,    53,    62,     0,     0,
       0,   123,   126,   128,     0,     0,     0,   137,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      45,    39,     0,    62,    54,    64,    62,     0,     0,   130,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   163,     0,    57,    65,   121,     0,
       0,   133,   135,     0,   140,   142,   146,   150,   154,   144,
     148,   152,   156,     0,     0,    52,     0,    55,   124,   166,
     138,   117,     0,   119,     0,   116,     0,    58,     0,     0,
     115,   158,   160,   131,   118
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,    46,    62,    70,   112,   113,   114,   115,   116,   117,
     118,   176,   271,   230,   119,   232,   120,   121,   122,   173,
     273,   274,   302,   275,   326,   347,   123,   185,   237,   277,
     124,   125,   126,    47,   190,    48,    56,    49,    72,    76,
     131,    77,   354,   355,   130,   280,   205,   308,   206,   207,
     284,   330,   285,   286,   211,   312,   255,   289,   293,   290,
     294,   291,   295,   292,   296,   297,   298,    50,   136,    51,
      52,    53,    54,    55
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -256
static const short int yypact[] =
{
     266,  -256,  -256,  -256,  -256,  -256,  -256,  -256,  -256,  -256,
    -256,  -256,  -256,  -256,  -256,  -256,  -256,  -256,  -256,  -256,
    -256,  -256,  -256,  -256,  -256,  -256,  -256,  -256,  -256,  -256,
    -256,  -256,  -256,  -256,   266,  -256,  -256,   266,  -256,  -256,
    -256,   266,  -256,  -256,  -256,  -256,    27,  -256,  -256,  -256,
      30,   266,  -256,  -256,  -256,  -256,   778,   -29,   -30,   -28,
    -256,    26,   266,  -256,  -256,  -256,  -256,  -256,  -256,  -256,
      -9,    51,    -2,  -256,  -256,  -256,  -256,    63,   416,    19,
     561,    29,  -256,   339,   479,    32,  -256,    31,    33,    36,
      37,    38,    41,    42,    44,    45,    48,    49,    50,    53,
      55,    59,    65,    81,    82,    84,    86,    89,    90,    91,
      93,    22,    97,   416,   -40,   -40,  -256,  -256,  -256,    57,
     711,   677,    95,  -256,    60,  -256,  -256,    -4,    20,    54,
      98,    92,    22,  -256,  -256,    62,    99,   339,   -40,  -256,
      60,  -256,   520,    22,  -256,  -256,  -256,    22,    22,    22,
    -256,    22,    22,    22,    22,    22,    22,    22,    22,    22,
      22,    22,    22,    22,    22,  -256,  -256,  -256,  -256,  -256,
     266,   339,  -256,  -256,  -256,   160,   147,  -256,   711,  -256,
    -256,    13,  -256,  -256,   -12,   102,  -256,  -256,    20,    20,
    -256,  -256,  -256,    51,  -256,    13,  -256,  -256,  -256,  -256,
    -256,   602,   101,    52,   100,    22,    22,    22,   103,   104,
     105,    22,  -256,   106,   107,   109,   111,   112,   113,   114,
     115,   119,   120,   124,   126,   125,   108,   129,  -256,  -256,
    -256,  -256,   643,  -256,  -256,  -256,   339,  -256,  -256,  -256,
    -256,  -256,  -256,  -256,    68,  -256,    22,  -256,   127,   131,
     133,  -256,  -256,  -256,   132,   136,  -256,  -256,  -256,  -256,
    -256,  -256,  -256,  -256,  -256,  -256,  -256,  -256,    22,   153,
    -256,  -256,  -256,   134,  -256,   150,    22,    -6,   152,    22,
     745,  -256,  -256,  -256,   745,   745,   745,  -256,   745,   745,
     745,   745,   745,   745,   745,   745,   745,   745,   745,   155,
    -256,  -256,   156,    -6,  -256,  -256,    -6,   158,   745,  -256,
     161,   162,   745,   163,   165,   166,   167,   168,   169,   170,
     171,   172,   173,   175,  -256,   643,   179,  -256,  -256,   174,
     180,  -256,  -256,   184,  -256,  -256,  -256,  -256,  -256,  -256,
    -256,  -256,  -256,     3,     3,  -256,     3,  -256,  -256,   339,
    -256,   164,    25,  -256,   185,  -256,   186,  -256,   187,   191,
    -256,  -256,  -256,  -256,  -256
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -256,  -256,  -256,  -256,   146,  -256,  -256,   -76,   178,   -64,
     -19,  -256,  -256,  -256,  -256,  -256,   142,  -256,   -15,  -113,
    -256,  -152,  -256,  -256,  -256,  -256,  -256,   -11,  -255,  -256,
     -75,  -101,  -256,   -74,  -108,   -77,  -256,   190,  -256,    71,
    -256,   -63,  -250,   -85,  -256,  -256,  -256,  -256,  -256,  -256,
    -256,  -256,  -256,  -256,  -256,  -256,  -256,  -256,  -256,  -256,
    -256,  -256,  -256,  -256,  -256,  -256,  -256,   -20,  -126,  -256,
     -83,  -256,  -256,  -256
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -111
static const short int yytable[] =
{
     137,   128,   174,   128,   127,   135,   127,   128,   138,   140,
     127,   199,   351,   165,    57,   129,   133,    58,   235,   177,
     179,    59,   305,   165,   235,   200,   170,    60,   171,   172,
     166,    63,   165,    61,   351,   165,   128,    83,    65,   127,
     166,    66,    69,   128,   128,   227,   127,   127,    67,   166,
     129,   327,   166,    68,   137,    73,    74,    75,   180,    71,
     182,   134,   165,   236,    78,   128,   202,   203,   127,   236,
     165,   196,   165,   186,   187,   246,   182,   231,   165,   166,
     239,   240,   352,   181,    83,   195,    83,   166,   137,   166,
      79,   279,   167,  -110,   356,   166,   357,   188,   189,   -43,
     141,   128,   -42,   142,   127,   183,   143,   144,   145,   184,
     278,   146,   147,   194,   148,   149,   197,   233,   150,   151,
     152,   183,   191,   153,   128,   154,   244,   127,   204,   155,
     175,   242,   208,   209,   210,   156,   212,   213,   214,   215,
     216,   217,   218,   219,   220,   221,   222,   223,   224,   225,
     226,   157,   158,   137,   159,   128,   160,   276,   127,   161,
     162,   163,   234,   164,   168,   180,   193,   192,   198,   228,
     229,   238,   245,   345,   247,   269,   243,   251,   252,   253,
     256,   257,   134,   258,   183,   259,   260,   261,   262,   263,
     248,   249,   250,   264,   265,   266,   254,   267,   270,   268,
     364,   281,   282,   307,   283,   301,   287,   309,   310,   311,
     288,   313,   314,   315,   316,   317,   318,   319,   320,   321,
     322,   323,   300,   358,   -51,   197,   324,   234,   306,   328,
     325,   329,   331,   332,   334,   333,   335,   336,   337,   338,
     339,   340,   341,   342,   359,   348,    64,   343,   128,   344,
     276,   127,   346,   299,   349,   350,   361,   362,   363,   169,
     243,   303,   139,   178,   241,   304,   137,   360,     0,     0,
       0,     0,     0,     0,     1,     2,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,     0,    19,    20,     0,    21,     0,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   353,   353,
       0,   353,    34,     0,    35,    36,    37,   353,    38,    39,
      40,    41,     0,    42,    43,    44,    45,     1,     2,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,     0,    19,    20,     0,    21,
       0,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    34,     0,    35,     0,    37,
       0,    38,    39,    40,    41,     0,    42,    43,    44,    45,
      73,    74,    75,    80,     0,     0,    81,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,     0,     0,    82,
      83,    18,    84,    85,    86,    87,     0,    88,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,     0,     0,     0,     0,    80,     0,   111,    81,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
       0,     0,    82,    83,    18,     0,    85,    86,     0,     0,
       0,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,     0,     0,     0,     0,   201,     0,     0,
      81,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,     0,     0,    82,     0,    18,     0,    85,    86,     0,
       0,   111,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,     0,     0,     0,     0,     0,     0,
       0,    81,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,     0,     0,    82,    83,    18,     0,    85,    86,
       0,     0,   111,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,     0,     0,     0,     0,     0,
       0,     0,    81,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,     0,     0,    82,     0,    18,     0,    85,
      86,     0,     0,   132,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,     0,     0,     0,     0,
       0,     0,     0,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,     0,     0,    82,     0,    18,     0,
      19,    86,   272,     0,   132,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,     0,     0,
      82,     0,    18,     0,    19,     0,     0,     0,     0,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,     0,     0,     0,     0,    18,     0,    19,     0,
       0,     0,     0,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,     0,     0,     0,     0,
      18,     0,    19,     0,     0,     0,     0,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,     0,
       0,     0,     0,     0,     0,    19,     0,     0,     0,     0,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32
};

static const short int yycheck[] =
{
      83,    78,   115,    80,    78,    80,    80,    84,    84,    84,
      84,   137,     9,    10,    34,    78,    80,    37,    30,   120,
     121,    41,   277,    10,    30,   138,    66,     0,    68,    69,
      27,    51,    10,     3,     9,    10,   113,    24,    67,   113,
      27,    71,    62,   120,   121,   171,   120,   121,    76,    27,
     113,   306,    27,    27,   137,     4,     5,     6,    70,    68,
     124,    80,    10,    75,    66,   142,   142,   142,   142,    75,
      10,   135,    10,    77,    78,    23,   140,   178,    10,    27,
     188,   189,    79,    23,    24,    23,    24,    27,   171,    27,
      27,    23,   111,    74,   344,    27,   346,    77,    78,    70,
      69,   178,    70,    70,   178,   124,    70,    70,    70,   124,
     236,    70,    70,   132,    70,    70,   135,   181,    70,    70,
      70,   140,    68,    70,   201,    70,   201,   201,   143,    70,
      73,   195,   147,   148,   149,    70,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     170,    70,    70,   236,    70,   232,    70,   232,   232,    70,
      70,    70,   181,    70,    67,    70,    74,    69,    69,     9,
      23,    69,    71,   325,    74,    67,   195,    74,    74,    74,
      74,    74,   201,    74,   203,    74,    74,    74,    74,    74,
     205,   206,   207,    74,    74,    71,   211,    71,    69,    74,
       9,    74,    71,   280,    71,    71,    74,   284,   285,   286,
      74,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     297,   298,    69,   349,    74,   244,    71,   246,    76,    71,
      74,   308,    71,    71,    71,   312,    71,    71,    71,    71,
      71,    71,    71,    71,    80,    71,    56,    74,   325,    74,
     325,   325,    73,   268,    74,    71,    71,    71,    71,   113,
     279,   276,    84,   121,   193,   276,   349,   352,    -1,    -1,
      -1,    -1,    -1,    -1,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    -1,    27,    28,    -1,    30,    -1,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   343,   344,
      -1,   346,    66,    -1,    68,    69,    70,   352,    72,    73,
      74,    75,    -1,    77,    78,    79,    80,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    -1,    27,    28,    -1,    30,
      -1,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    66,    -1,    68,    -1,    70,
      -1,    72,    73,    74,    75,    -1,    77,    78,    79,    80,
       4,     5,     6,     7,    -1,    -1,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    -1,    -1,    23,
      24,    25,    26,    27,    28,    29,    -1,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    -1,    -1,    -1,    -1,     7,    -1,    72,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      -1,    -1,    23,    24,    25,    -1,    27,    28,    -1,    -1,
      -1,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    -1,    -1,    -1,    -1,     7,    -1,    -1,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    -1,    -1,    23,    -1,    25,    -1,    27,    28,    -1,
      -1,    72,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    -1,    -1,    23,    24,    25,    -1,    27,    28,
      -1,    -1,    72,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    -1,    -1,    23,    -1,    25,    -1,    27,
      28,    -1,    -1,    72,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    -1,    -1,    23,    -1,    25,    -1,
      27,    28,    29,    -1,    72,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    -1,    -1,
      23,    -1,    25,    -1,    27,    -1,    -1,    -1,    -1,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,
      -1,    -1,    -1,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    -1,    -1,    -1,    -1,
      25,    -1,    27,    -1,    -1,    -1,    -1,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    -1,
      -1,    -1,    -1,    -1,    -1,    27,    -1,    -1,    -1,    -1,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    27,
      28,    30,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    66,    68,    69,    70,    72,    73,
      74,    75,    77,    78,    79,    80,    82,   114,   116,   118,
     148,   150,   151,   152,   153,   154,   117,   148,   148,   148,
       0,     3,    83,   148,   118,    67,    71,    76,    27,   148,
      84,    68,   119,     4,     5,     6,   120,   122,    66,    27,
       7,    10,    23,    24,    26,    27,    28,    29,    31,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    72,    85,    86,    87,    88,    89,    90,    91,    95,
      97,    98,    99,   107,   111,   112,   113,   114,   116,   122,
     125,   121,    72,    90,    91,   111,   149,   151,    88,    89,
     111,    69,    70,    70,    70,    70,    70,    70,    70,    70,
      70,    70,    70,    70,    70,    70,    70,    70,    70,    70,
      70,    70,    70,    70,    70,    10,    27,    91,    67,    85,
      66,    68,    69,   100,   100,    73,    92,   112,    97,   112,
      70,    23,    90,    91,    99,   108,    77,    78,    77,    78,
     115,    68,    69,    74,    91,    23,    90,    91,    69,   149,
     100,     7,    88,   111,    99,   127,   129,   130,    99,    99,
      99,   135,    99,    99,    99,    99,    99,    99,    99,    99,
      99,    99,    99,    99,    99,    99,   148,   149,     9,    23,
      94,   112,    96,    90,    91,    30,    75,   109,    69,   115,
     115,   120,    90,    91,   111,    71,    23,    74,    99,    99,
      99,    74,    74,    74,    99,   137,    74,    74,    74,    74,
      74,    74,    74,    74,    74,    74,    71,    71,    74,    67,
      69,    93,    29,   101,   102,   104,   111,   110,   149,    23,
     126,    74,    71,    71,   131,   133,   134,    74,    74,   138,
     140,   142,   144,   139,   141,   143,   145,   146,   147,    99,
      69,    71,   103,    99,   108,   109,    76,   116,   128,   116,
     116,   116,   136,   116,   116,   116,   116,   116,   116,   116,
     116,   116,   116,   116,    71,    74,   105,   109,    71,   116,
     132,    71,    71,   116,    71,    71,    71,    71,    71,    71,
      71,    71,    71,    74,    74,   102,    73,   106,    71,    74,
      71,     9,    79,    99,   123,   124,   123,   123,   149,    80,
     124,    71,    71,    71,     9
};

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
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)


#define YYTERROR        1
#define YYERRCODE       256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)                  \
     fprintf (File, "%d.%d-%d.%d",                      \
              (Loc).first_line, (Loc).first_column,     \
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
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)          \
do {                                                            \
  if (yydebug)                                                  \
    {                                                           \
      YYFPRINTF (stderr, "%s ", Title);                         \
      yysymprint (stderr,                                       \
                  Type, Value); \
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
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname[yyr1[yyrule]]);
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
  const char *yys = yystr;

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
      size_t yyn = 0;
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

#endif /* YYERROR_VERBOSE */



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
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);


# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
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
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

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
    ;
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

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  short int *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



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
        short int *yyss1 = yyss;
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

/* Do appropriate processing given the current state.  */
/* Read a look-ahead token if we need one and don't already have one.  */
/* yyresume: */

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

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

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
#line 265 "vtkParse.y"
    {
      data.ClassName = vtkstrdup((yyvsp[0].str));
      }
    break;

  case 11:
#line 275 "vtkParse.y"
    { output_function(); }
    break;

  case 12:
#line 276 "vtkParse.y"
    { output_function(); }
    break;

  case 13:
#line 277 "vtkParse.y"
    { legacySig(); output_function(); }
    break;

  case 17:
#line 283 "vtkParse.y"
    { preSig("~"); }
    break;

  case 18:
#line 284 "vtkParse.y"
    { preSig("virtual ~"); }
    break;

  case 20:
#line 287 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[-1].integer);
         }
    break;

  case 21:
#line 291 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[-2].integer);
         }
    break;

  case 22:
#line 295 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = (yyvsp[-2].integer);
         }
    break;

  case 23:
#line 300 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = (yyvsp[-1].integer);
         }
    break;

  case 24:
#line 305 "vtkParse.y"
    {
         preSig("virtual ");
         }
    break;

  case 25:
#line 311 "vtkParse.y"
    {
         output_function();
         }
    break;

  case 26:
#line 315 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[-1].integer);
         output_function();
         }
    break;

  case 27:
#line 320 "vtkParse.y"
    {
         currentFunction->ReturnType = (yyvsp[-2].integer);
         output_function();
         }
    break;

  case 28:
#line 325 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = (yyvsp[-2].integer);
         output_function();
         }
    break;

  case 29:
#line 331 "vtkParse.y"
    {
         preSig("virtual ");
         currentFunction->ReturnType = (yyvsp[-1].integer);
         output_function();
         }
    break;

  case 30:
#line 337 "vtkParse.y"
    {
         preSig("virtual ");
         output_function();
         }
    break;

  case 31:
#line 343 "vtkParse.y"
    {
      currentFunction->IsOperator = 1;
      vtkParseDebug("Converted operator", 0);
    }
    break;

  case 32:
#line 348 "vtkParse.y"
    { postSig(")"); }
    break;

  case 33:
#line 348 "vtkParse.y"
    { postSig(";"); openSig = 0; }
    break;

  case 34:
#line 349 "vtkParse.y"
    {
      openSig = 1;
      currentFunction->Name = (yyvsp[-3].str); 
      vtkParseDebug("Parsed func", (yyvsp[-3].str));
    }
    break;

  case 35:
#line 355 "vtkParse.y"
    { 
      postSig(") = 0;"); 
      currentFunction->Name = (yyvsp[-2].str); 
      vtkParseDebug("Parsed func", (yyvsp[-2].str));
      currentFunction->IsPureVirtual = 1; 
      data.IsAbstract = 1;
    }
    break;

  case 37:
#line 363 "vtkParse.y"
    {postSig(" const");}
    break;

  case 38:
#line 365 "vtkParse.y"
    {postSig(" ("); }
    break;

  case 40:
#line 367 "vtkParse.y"
    {postSig("const ");}
    break;

  case 41:
#line 369 "vtkParse.y"
    {postSig("static ");}
    break;

  case 42:
#line 371 "vtkParse.y"
    {postSig((yyvsp[0].str));}
    break;

  case 43:
#line 371 "vtkParse.y"
    {postSig((yyvsp[0].str));}
    break;

  case 50:
#line 380 "vtkParse.y"
    { currentFunction->NumberOfArguments++;}
    break;

  case 51:
#line 381 "vtkParse.y"
    { currentFunction->NumberOfArguments++; postSig(", ");}
    break;

  case 53:
#line 384 "vtkParse.y"
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
        (yyvsp[0].integer);}
    break;

  case 54:
#line 389 "vtkParse.y"
    {
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 
        (yyvsp[0].integer) / 0x10000; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
        (yyvsp[-1].integer) + (yyvsp[0].integer) % 0x10000;
    }
    break;

  case 56:
#line 396 "vtkParse.y"
    { 
      postSig("void (*func)(void *) ");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 0x5000;
    }
    break;

  case 59:
#line 404 "vtkParse.y"
    {delSig();}
    break;

  case 60:
#line 404 "vtkParse.y"
    {delSig();}
    break;

  case 61:
#line 406 "vtkParse.y"
    { (yyval.integer) = (yyvsp[0].integer); }
    break;

  case 62:
#line 414 "vtkParse.y"
    { (yyval.integer) = 0; }
    break;

  case 63:
#line 415 "vtkParse.y"
    { char temp[100]; sprintf(temp,"[%i]",(yyvsp[0].integer)); 
                   postSig(temp); }
    break;

  case 64:
#line 417 "vtkParse.y"
    { (yyval.integer) = 0x300 + 0x10000 * (yyvsp[-2].integer) + (yyvsp[0].integer) % 0x1000; }
    break;

  case 65:
#line 419 "vtkParse.y"
    { postSig("[]"); (yyval.integer) = 0x300 + (yyvsp[0].integer) % 0x1000; }
    break;

  case 66:
#line 421 "vtkParse.y"
    {(yyval.integer) = 0x1000 + (yyvsp[0].integer);}
    break;

  case 67:
#line 422 "vtkParse.y"
    {(yyval.integer) = (yyvsp[0].integer);}
    break;

  case 68:
#line 423 "vtkParse.y"
    {(yyval.integer) = 0x2000 + (yyvsp[0].integer);}
    break;

  case 69:
#line 424 "vtkParse.y"
    {(yyval.integer) = 0x3000 + (yyvsp[0].integer);}
    break;

  case 70:
#line 426 "vtkParse.y"
    {(yyval.integer) = (yyvsp[0].integer);}
    break;

  case 71:
#line 428 "vtkParse.y"
    {(yyval.integer) = (yyvsp[-1].integer) + (yyvsp[0].integer);}
    break;

  case 72:
#line 429 "vtkParse.y"
    {(yyval.integer) = (yyvsp[0].integer);}
    break;

  case 73:
#line 431 "vtkParse.y"
    {(yyval.integer) = (yyvsp[0].integer);}
    break;

  case 74:
#line 432 "vtkParse.y"
    { postSig("&"); (yyval.integer) = (yyvsp[-1].integer);}
    break;

  case 75:
#line 433 "vtkParse.y"
    { postSig("*"); (yyval.integer) = 0x400 + (yyvsp[-1].integer);}
    break;

  case 76:
#line 435 "vtkParse.y"
    { postSig("vtkStdString "); (yyval.integer) = 0x1303; }
    break;

  case 77:
#line 445 "vtkParse.y"
    { postSig("&"); (yyval.integer) = 0x100;}
    break;

  case 78:
#line 446 "vtkParse.y"
    { postSig("*"); (yyval.integer) = 0x300;}
    break;

  case 79:
#line 447 "vtkParse.y"
    { (yyval.integer) = 0x100 + (yyvsp[0].integer);}
    break;

  case 80:
#line 448 "vtkParse.y"
    { (yyval.integer) = 0x400 + (yyvsp[0].integer);}
    break;

  case 81:
#line 450 "vtkParse.y"
    {postSig("unsigned ");}
    break;

  case 82:
#line 451 "vtkParse.y"
    { (yyval.integer) = 0x10 + (yyvsp[0].integer);}
    break;

  case 83:
#line 452 "vtkParse.y"
    { (yyval.integer) = (yyvsp[0].integer);}
    break;

  case 84:
#line 455 "vtkParse.y"
    { postSig("vtkTypeInt8 "); (yyval.integer) = VTK_PARSE_INT8; }
    break;

  case 85:
#line 456 "vtkParse.y"
    { postSig("vtkTypeUInt8 "); (yyval.integer) = VTK_PARSE_UINT8; }
    break;

  case 86:
#line 457 "vtkParse.y"
    { postSig("vtkTypeInt16 "); (yyval.integer) = VTK_PARSE_INT16; }
    break;

  case 87:
#line 458 "vtkParse.y"
    { postSig("vtkTypeUInt16 "); (yyval.integer) = VTK_PARSE_UINT16; }
    break;

  case 88:
#line 459 "vtkParse.y"
    { postSig("vtkTypeInt32 "); (yyval.integer) = VTK_PARSE_INT32; }
    break;

  case 89:
#line 460 "vtkParse.y"
    { postSig("vtkTypeUInt32 "); (yyval.integer) = VTK_PARSE_UINT32; }
    break;

  case 90:
#line 461 "vtkParse.y"
    { postSig("vtkTypeInt64 "); (yyval.integer) = VTK_PARSE_INT64; }
    break;

  case 91:
#line 462 "vtkParse.y"
    { postSig("vtkTypeUInt64 "); (yyval.integer) = VTK_PARSE_UINT64; }
    break;

  case 92:
#line 463 "vtkParse.y"
    { postSig("vtkTypeFloat32 "); (yyval.integer) = VTK_PARSE_FLOAT32; }
    break;

  case 93:
#line 464 "vtkParse.y"
    { postSig("vtkTypeFloat64 "); (yyval.integer) = VTK_PARSE_FLOAT64; }
    break;

  case 94:
#line 465 "vtkParse.y"
    { postSig("float "); (yyval.integer) = 0x1;}
    break;

  case 95:
#line 466 "vtkParse.y"
    { postSig("void "); (yyval.integer) = 0x2;}
    break;

  case 96:
#line 467 "vtkParse.y"
    { postSig("char "); (yyval.integer) = 0x3;}
    break;

  case 97:
#line 468 "vtkParse.y"
    { postSig("int "); (yyval.integer) = 0x4;}
    break;

  case 98:
#line 469 "vtkParse.y"
    { postSig("short "); (yyval.integer) = 0x5;}
    break;

  case 99:
#line 470 "vtkParse.y"
    { postSig("long "); (yyval.integer) = 0x6;}
    break;

  case 100:
#line 471 "vtkParse.y"
    { postSig("double "); (yyval.integer) = 0x7;}
    break;

  case 101:
#line 472 "vtkParse.y"
    {       
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",(yyvsp[0].str));
      postSig(ctmpid);
      (yyval.integer) = 0x8;}
    break;

  case 102:
#line 478 "vtkParse.y"
    { 
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",(yyvsp[0].str));
      postSig(ctmpid);
      (yyval.integer) = 0x9; 
      currentFunction->ArgClasses[currentFunction->NumberOfArguments] =
        vtkstrdup((yyvsp[0].str)); 
      /* store the string into the return value just in case we need it */
      /* this is a parsing hack because the first "type" parser will */
      /* possibly be ht ereturn type of the first argument */
      if ((!currentFunction->ReturnClass) && 
          (!currentFunction->NumberOfArguments)) 
        { 
        currentFunction->ReturnClass = vtkstrdup((yyvsp[0].str)); 
        }
    }
    break;

  case 103:
#line 494 "vtkParse.y"
    { postSig("vtkIdType "); (yyval.integer) = 0xA;}
    break;

  case 104:
#line 495 "vtkParse.y"
    { postSig("long long "); (yyval.integer) = 0xB;}
    break;

  case 105:
#line 496 "vtkParse.y"
    { postSig("__int64 "); (yyval.integer) = 0xC;}
    break;

  case 106:
#line 497 "vtkParse.y"
    { postSig("signed char "); (yyval.integer) = 0xD;}
    break;

  case 109:
#line 502 "vtkParse.y"
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup((yyvsp[0].str)); 
      data.NumberOfSuperClasses++; 
    }
    break;

  case 110:
#line 507 "vtkParse.y"
    { 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup((yyvsp[0].str)); 
      data.NumberOfSuperClasses++; 
    }
    break;

  case 112:
#line 512 "vtkParse.y"
    {in_public = 1; in_protected = 0;}
    break;

  case 113:
#line 513 "vtkParse.y"
    {in_public = 0; in_protected = 0;}
    break;

  case 114:
#line 514 "vtkParse.y"
    {in_public = 0; in_protected = 1;}
    break;

  case 117:
#line 518 "vtkParse.y"
    {(yyval.integer) = (yyvsp[0].integer);}
    break;

  case 118:
#line 519 "vtkParse.y"
    {(yyval.integer) = -1;}
    break;

  case 119:
#line 519 "vtkParse.y"
    {(yyval.integer) = -1;}
    break;

  case 120:
#line 523 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); }
    break;

  case 121:
#line 524 "vtkParse.y"
    {
   postSig(");");
   sprintf(temps,"Set%s",(yyvsp[-4].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (yyvsp[-1].integer);
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();
   }
    break;

  case 122:
#line 534 "vtkParse.y"
    {postSig("Get");}
    break;

  case 123:
#line 534 "vtkParse.y"
    {postSig(" ();"); invertSig = 1;}
    break;

  case 124:
#line 536 "vtkParse.y"
    { 
   sprintf(temps,"Get%s",(yyvsp[-4].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (yyvsp[-1].integer);
   output_function();
   }
    break;

  case 125:
#line 543 "vtkParse.y"
    {preSig("void Set");}
    break;

  case 126:
#line 544 "vtkParse.y"
    {
   postSig(" (char *);"); 
   sprintf(temps,"Set%s",(yyvsp[-1].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x303;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();
   }
    break;

  case 127:
#line 554 "vtkParse.y"
    {preSig("char *Get");}
    break;

  case 128:
#line 555 "vtkParse.y"
    { 
   postSig(" ();");
   sprintf(temps,"Get%s",(yyvsp[-1].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x303;
   output_function();
   }
    break;

  case 129:
#line 564 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); }
    break;

  case 130:
#line 565 "vtkParse.y"
    {postSig(");"); openSig = 0;}
    break;

  case 131:
#line 566 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sscanf (currentFunction->Signature, "%*s %*s (%s);", local);
   sprintf(temps,"Set%s",(yyvsp[-7].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = (yyvsp[-4].integer);
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%s Get%sMinValue ();",local,(yyvsp[-7].str));
   sprintf(temps,"Get%sMinValue",(yyvsp[-7].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (yyvsp[-4].integer);
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%s Get%sMaxValue ();",local,(yyvsp[-7].str));
   sprintf(temps,"Get%sMaxValue",(yyvsp[-7].str));
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = (yyvsp[-4].integer);
   output_function();
   }
    break;

  case 132:
#line 596 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); }
    break;

  case 133:
#line 597 "vtkParse.y"
    { 
   postSig("*);");
   sprintf(temps,"Set%s",(yyvsp[-4].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 0x2;
   output_function();
   }
    break;

  case 134:
#line 608 "vtkParse.y"
    {preSig("void Set"); postSig(" ("); }
    break;

  case 135:
#line 609 "vtkParse.y"
    { 
   postSig("*);");
   sprintf(temps,"Set%s",(yyvsp[-4].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 0x2;
   output_function();
   }
    break;

  case 136:
#line 619 "vtkParse.y"
    {postSig("*Get");}
    break;

  case 137:
#line 620 "vtkParse.y"
    {postSig(" ();"); invertSig = 1;}
    break;

  case 138:
#line 621 "vtkParse.y"
    { 
   sprintf(temps,"Get%s",(yyvsp[-4].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x309;
   output_function();
   }
    break;

  case 139:
#line 629 "vtkParse.y"
    {preSig("void "); postSig("On ();"); openSig = 0; }
    break;

  case 140:
#line 631 "vtkParse.y"
    { 
   sprintf(temps,"%sOn",(yyvsp[-4].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x2;
   output_function();
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void %sOff ();",(yyvsp[-4].str)); 
   sprintf(temps,"%sOff",(yyvsp[-4].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   output_function();
   }
    break;

  case 141:
#line 646 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 142:
#line 651 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s);",(yyvsp[-4].str),
     local, local);
   sprintf(temps,"Set%s",(yyvsp[-4].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 2;
   currentFunction->ArgTypes[0] = (yyvsp[-1].integer);
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = (yyvsp[-1].integer);
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[2]);",(yyvsp[-4].str),
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x300 + (yyvsp[-1].integer);
   currentFunction->ArgCounts[0] = 0x2;
   output_function();
   }
    break;

  case 143:
#line 676 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 144:
#line 681 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, (yyvsp[-4].str));
   sprintf(temps,"Get%s",(yyvsp[-4].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + (yyvsp[-1].integer);
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 2;
   output_function();
   }
    break;

  case 145:
#line 693 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 146:
#line 698 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s);",
     (yyvsp[-4].str), local, local, local);
   sprintf(temps,"Set%s",(yyvsp[-4].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 3;
   currentFunction->ArgTypes[0] = (yyvsp[-1].integer);
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = (yyvsp[-1].integer);
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = (yyvsp[-1].integer);
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[3]);",(yyvsp[-4].str),
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x300 + (yyvsp[-1].integer);
   currentFunction->ArgCounts[0] = 3;
   output_function();
   }
    break;

  case 147:
#line 725 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 148:
#line 730 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, (yyvsp[-4].str));
   sprintf(temps,"Get%s",(yyvsp[-4].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + (yyvsp[-1].integer);
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 3;
   output_function();
   }
    break;

  case 149:
#line 742 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 150:
#line 747 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s, %s);",
     (yyvsp[-4].str), local, local, local, local);
   sprintf(temps,"Set%s",(yyvsp[-4].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 4;
   currentFunction->ArgTypes[0] = (yyvsp[-1].integer);
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = (yyvsp[-1].integer);
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = (yyvsp[-1].integer);
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ArgTypes[3] = (yyvsp[-1].integer);
   currentFunction->ArgCounts[3] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[4]);",(yyvsp[-4].str),
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x300 + (yyvsp[-1].integer);
   currentFunction->ArgCounts[0] = 4;
   output_function();
   }
    break;

  case 151:
#line 776 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 152:
#line 781 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, (yyvsp[-4].str));
   sprintf(temps,"Get%s",(yyvsp[-4].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + (yyvsp[-1].integer);
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 4;
   output_function();
   }
    break;

  case 153:
#line 793 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 154:
#line 798 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s, %s, %s, %s);",
     (yyvsp[-4].str), local, local, local, local, local, local);
   sprintf(temps,"Set%s",(yyvsp[-4].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 6;
   currentFunction->ArgTypes[0] = (yyvsp[-1].integer);
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = (yyvsp[-1].integer);
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = (yyvsp[-1].integer);
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ArgTypes[3] = (yyvsp[-1].integer);
   currentFunction->ArgCounts[3] = 0;
   currentFunction->ArgTypes[4] = (yyvsp[-1].integer);
   currentFunction->ArgCounts[4] = 0;
   currentFunction->ArgTypes[5] = (yyvsp[-1].integer);
   currentFunction->ArgCounts[5] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[6]);",(yyvsp[-4].str),
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x300 + (yyvsp[-1].integer);
   currentFunction->ArgCounts[0] = 6;
   output_function();
   }
    break;

  case 155:
#line 831 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 156:
#line 836 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, (yyvsp[-4].str));
   sprintf(temps,"Get%s",(yyvsp[-4].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + (yyvsp[-1].integer);
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 6;
   output_function();
   }
    break;

  case 157:
#line 848 "vtkParse.y"
    {
      free (currentFunction->Signature);
      currentFunction->Signature = NULL;
      }
    break;

  case 158:
#line 853 "vtkParse.y"
    {
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s [%i]);",(yyvsp[-6].str),
      local, (yyvsp[-1].integer));
     sprintf(temps,"Set%s",(yyvsp[-6].str)); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->ReturnType = 0x2;
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 0x300 + (yyvsp[-3].integer);
     currentFunction->ArgCounts[0] = (yyvsp[-1].integer);
     output_function();
   }
    break;

  case 159:
#line 866 "vtkParse.y"
    {
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
    break;

  case 160:
#line 871 "vtkParse.y"
    { 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, (yyvsp[-6].str));
   sprintf(temps,"Get%s",(yyvsp[-6].str)); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + (yyvsp[-3].integer);
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = (yyvsp[-1].integer);
   output_function();
   }
    break;

  case 161:
#line 883 "vtkParse.y"
    { 
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       (yyvsp[-1].str));

     sprintf(temps,"Get%sCoordinate",(yyvsp[-1].str)); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 0x309;
     currentFunction->ReturnClass = vtkstrdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double, double);",
       (yyvsp[-1].str));
     sprintf(temps,"Set%s",(yyvsp[-1].str)); 
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
       (yyvsp[-1].str));
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 0x307;
     currentFunction->ArgCounts[0] = 2;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s ();", (yyvsp[-1].str));
     sprintf(temps,"Get%s",(yyvsp[-1].str)); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 0x307;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 2;
     output_function();
   }
    break;

  case 162:
#line 930 "vtkParse.y"
    { 
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       (yyvsp[-1].str));

     sprintf(temps,"Get%sCoordinate",(yyvsp[-1].str)); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 0x309;
     currentFunction->ReturnClass = vtkstrdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double, double, double);",
       (yyvsp[-1].str));
     sprintf(temps,"Set%s",(yyvsp[-1].str)); 
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
       (yyvsp[-1].str));
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 0x307;
     currentFunction->ArgCounts[0] = 3;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s ();", (yyvsp[-1].str));
     sprintf(temps,"Get%s",(yyvsp[-1].str)); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 0x307;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 3;
     output_function();
   }
    break;

  case 163:
#line 979 "vtkParse.y"
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
           (yyvsp[-3].str));
   sprintf(temps,"NewInstance"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x309;
   currentFunction->ReturnClass = vtkstrdup((yyvsp[-3].str));
   output_function();

   if ( data.IsConcrete )
     {
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature, "%s *SafeDownCast (vtkObject* o);",
             (yyvsp[-3].str));
     sprintf(temps,"SafeDownCast"); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 0x309;
     currentFunction->ArgCounts[0] = 1;
     currentFunction->ArgClasses[0] = vtkstrdup("vtkObject");
     currentFunction->ReturnType = 0x2309;
     currentFunction->ReturnClass = vtkstrdup((yyvsp[-3].str));
     output_function();
     }
   }
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 3020 "vtkParse.tab.c"

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
          int yytype = YYTRANSLATE (yychar);
          YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
          YYSIZE_T yysize = yysize0;
          YYSIZE_T yysize1;
          int yysize_overflow = 0;
          char *yymsg = 0;
#         define YYERROR_VERBOSE_ARGS_MAXIMUM 5
          char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
          int yyx;

#if 0
          /* This is so xgettext sees the translatable formats that are
             constructed on the fly.  */
          YY_("syntax error, unexpected %s");
          YY_("syntax error, unexpected %s, expecting %s");
          YY_("syntax error, unexpected %s, expecting %s or %s");
          YY_("syntax error, unexpected %s, expecting %s or %s or %s");
          YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
#endif
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
          int yychecklim = YYLAST - yyn;
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
                yysize_overflow |= yysize1 < yysize;
                yysize = yysize1;
                yyfmt = yystpcpy (yyfmt, yyprefix);
                yyprefix = yyor;
              }

          yyf = YY_(yyformat);
          yysize1 = yysize + yystrlen (yyf);
          yysize_overflow |= yysize1 < yysize;
          yysize = yysize1;

          if (!yysize_overflow && yysize <= YYSTACK_ALLOC_MAXIMUM)
            yymsg = (char *) YYSTACK_ALLOC (yysize);
          if (yymsg)
            {
              /* Avoid sprintf, as that infringes on the user's name space.
                 Don't have undefined behavior even if the translation
                 produced a string with the wrong number of "%s"s.  */
              char *yyp = yymsg;
              int yyi = 0;
              while ((*yyp = *yyf))
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
              yyerror (yymsg);
              YYSTACK_FREE (yymsg);
            }
          else
            {
              yyerror (YY_("syntax error"));
              goto yyexhaustedlab;
            }
        }
      else
#endif /* YYERROR_VERBOSE */
        yyerror (YY_("syntax error"));
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
          yydestruct ("Error: discarding", yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse look-ahead token after shifting the error
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


      yydestruct ("Error: popping", yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token. */
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
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp);
      YYPOPSTACK;
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 1047 "vtkParse.y"

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
