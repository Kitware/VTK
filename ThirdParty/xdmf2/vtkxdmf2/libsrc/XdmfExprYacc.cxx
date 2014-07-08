/* A Bison parser, made by GNU Bison 1.875a.  */

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

/* If NAME_PREFIX is specified substitute the variables and functions
   names.  */
#define yyparse dice_yyparse
#define yylex   dice_yylex
#define yyerror dice_yyerror
#define yylval  dice_yylval
#define yychar  dice_yychar
#define yydebug dice_yydebug
#define yynerrs dice_yynerrs


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     lFLOAT = 258,
     tokINTEGER = 259,
     tokARRAY = 260,
     NAME = 261,
     SIN = 262,
     COS = 263,
     TAN = 264,
     ACOS = 265,
     ASIN = 266,
     ATAN = 267,
     LOG = 268,
     EXP = 269,
     ABS_TOKEN = 270,
     SQRT = 271,
     WHERE = 272,
     INDEX = 273,
     EQEQ = 274,
     LT = 275,
     LE = 276,
     GT = 277,
     GE = 278,
     NE = 279,
     LTLT = 280,
     GTGT = 281,
     JOIN = 282
   };
#endif
#define lFLOAT 258
#define tokINTEGER 259
#define tokARRAY 260
#define NAME 261
#define SIN 262
#define COS 263
#define TAN 264
#define ACOS 265
#define ASIN 266
#define ATAN 267
#define LOG 268
#define EXP 269
#define ABS_TOKEN 270
#define SQRT 271
#define WHERE 272
#define INDEX 273
#define EQEQ 274
#define LT 275
#define LE 276
#define GT 277
#define GE 278
#define NE 279
#define LTLT 280
#define GTGT 281
#define JOIN 282




/* Copy the first part of user declarations.  */

/* Force the definition for Linux */
/* Possible bug in older Linux yacc */

#ifndef NOBISON
extern int yylex();
extern "C" {
        void yyerror( const char *);
        int  yyparse( void );
}
#endif
#include <XdmfExpr.h>
#include <XdmfArray.h>
#include <XdmfHDF.h>
#include <math.h>

static xdmf2::XdmfArray *XdmfExprReturnValue;
XdmfExprSymbol *XdmfExprItemsTable = NULL;


namespace xdmf2
{

class XdmfInt64Array : public XdmfArray {
public :
        XdmfInt64Array( XdmfInt64 Length ) {
                this->SetNumberType( XDMF_INT64_TYPE );
                this->SetNumberOfElements( Length );
                }
        XdmfInt64Array() {
                this->SetNumberType( XDMF_INT64_TYPE );
                this->SetNumberOfElements( 10 );
                };
};

}

#define ADD_XDMF_tokARRAY_TO_SYMBOL( a ) \
        { \
        char        name[80]; \
        XdmfExprSymbol *sp; \
        sprintf( name, "XdmfArray_%X", ( XdmfLength)(a) ); \
        sp = XdmfExprSymbolLookup( name ); \
        sp->ClientData = (a); \
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

typedef union YYSTYPE {
        double                DoubleValue;
        long                IntegerValue;
        void                *ArrayPointer;
        XdmfExprSymbol        *Symbol;
} YYSTYPE;
/* Line 191 of yacc.c.  */
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */


#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
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
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
         || (YYSTYPE_IS_TRIVIAL)))

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
     ((N) * (sizeof (short) + sizeof (YYSTYPE))                                \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)                \
      do                                        \
        {                                        \
          register YYSIZE_T yyi;                \
          for (yyi = 0; yyi < (Count); yyi++)        \
            (To)[yyi] = (From)[yyi];                \
        }                                        \
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)                                        \
    do                                                                        \
      {                                                                        \
        YYSIZE_T yynewbytes;                                                \
        YYCOPY (&yyptr->Stack, Stack, yysize);                                \
        Stack = &yyptr->Stack;                                                \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                                \
      }                                                                        \
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  22
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   278

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  40
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  5
/* YYNRULES -- Number of rules. */
#define YYNRULES  46
/* YYNRULES -- Number of states. */
#define YYNSTATES  119

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   282

#define YYTRANSLATE(YYX)                                                 \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      38,    39,    30,    29,    32,    28,     2,    31,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    37,    33,
       2,    34,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    35,     2,    36,     2,     2,     2,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     5,     9,    13,    20,    27,    36,    45,
      47,    49,    53,    57,    61,    65,    69,    73,    77,    81,
      85,    89,    93,    97,   101,   105,   110,   117,   124,   131,
     138,   145,   152,   159,   166,   173,   178,   182,   187,   189,
     193,   197,   201,   205,   210,   214,   216
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      41,     0,    -1,    42,    -1,     5,    34,    43,    -1,     5,
      34,    44,    -1,     5,    35,    43,    36,    34,    44,    -1,
       5,    35,    43,    36,    34,    43,    -1,     5,    35,     4,
      37,     4,    36,    34,    44,    -1,     5,    35,     4,    37,
       4,    36,    34,    43,    -1,    43,    -1,    44,    -1,    43,
      29,    43,    -1,    43,    32,    43,    -1,    43,    33,    43,
      -1,    43,    28,    43,    -1,    43,    30,    43,    -1,    43,
      31,    43,    -1,    43,    29,    44,    -1,    43,    28,    44,
      -1,    43,    30,    44,    -1,    43,    31,    44,    -1,    44,
      29,    43,    -1,    44,    28,    43,    -1,    44,    30,    43,
      -1,    44,    31,    43,    -1,     5,    35,    43,    36,    -1,
       5,    35,     4,    37,     4,    36,    -1,    18,    38,    43,
      19,    43,    39,    -1,    17,    38,    43,    19,    43,    39,
      -1,    17,    38,    43,    19,    44,    39,    -1,    17,    38,
      43,    20,    44,    39,    -1,    17,    38,    43,    21,    44,
      39,    -1,    17,    38,    43,    22,    44,    39,    -1,    17,
      38,    43,    23,    44,    39,    -1,    17,    38,    43,    24,
      44,    39,    -1,     6,    38,    43,    39,    -1,    38,    43,
      39,    -1,    27,    38,    43,    39,    -1,     5,    -1,    44,
      29,    44,    -1,    44,    28,    44,    -1,    44,    30,    44,
      -1,    44,    31,    44,    -1,     6,    38,    44,    39,    -1,
      38,    44,    39,    -1,     4,    -1,     3,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,    68,    68,    76,    84,    89,   101,   117,   128,   141,
     148,   153,   162,   239,   303,   312,   322,   331,   340,   349,
     358,   367,   376,   385,   394,   403,   413,   424,   469,   502,
     523,   544,   565,   586,   607,   628,   646,   650,   654,   671,
     675,   679,   683,   687,   695,   699,   703
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "lFLOAT", "tokINTEGER", "tokARRAY", "NAME", 
  "SIN", "COS", "TAN", "ACOS", "ASIN", "ATAN", "LOG", "EXP", "ABS_TOKEN", 
  "SQRT", "WHERE", "INDEX", "EQEQ", "LT", "LE", "GT", "GE", "NE", "LTLT", 
  "GTGT", "JOIN", "'-'", "'+'", "'*'", "'/'", "','", "';'", "'='", "'['", 
  "']'", "':'", "'('", "')'", "$accept", "statemant_list", "statement", 
  "ArrayExpression", "ScalarExpression", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,    45,    43,
      42,    47,    44,    59,    61,    91,    93,    58,    40,    41
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    40,    41,    42,    42,    42,    42,    42,    42,    42,
      42,    43,    43,    43,    43,    43,    43,    43,    43,    43,
      43,    43,    43,    43,    43,    43,    43,    43,    43,    43,
      43,    43,    43,    43,    43,    43,    43,    43,    43,    44,
      44,    44,    44,    44,    44,    44,    44
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     3,     3,     6,     6,     8,     8,     1,
       1,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     4,     6,     6,     6,     6,
       6,     6,     6,     6,     6,     4,     3,     4,     1,     3,
       3,     3,     3,     4,     3,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,    46,    45,    38,     0,     0,     0,     0,     0,     0,
       2,     9,    10,     0,     0,     0,     0,     0,     0,    38,
       0,     0,     1,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     3,     4,    45,     0,     0,     0,     0,
       0,     0,     0,     0,    36,    44,    14,    18,    11,    17,
      15,    19,    16,    20,    12,    13,    22,    40,    21,    39,
      23,    41,    24,    42,     0,    25,    35,    43,     0,     0,
       0,     0,     0,     0,     0,    37,    45,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    25,    26,     6,     5,    28,    29,     0,     0,     0,
       0,     0,     0,    30,    31,    32,    33,    34,    27,     0,
       0,     0,    40,    39,    41,    42,    26,     8,     7
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,     9,    10,    11,    37
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -36
static const short yypact[] =
{
       1,   -36,   -36,   -33,   -35,   -27,   -24,   -16,    88,    27,
     -36,   245,    26,    88,   113,    88,    88,    88,    88,     5,
     143,     7,   -36,    88,    88,    88,    88,    88,    88,    88,
      88,    88,    88,   245,    26,     4,   227,    26,   155,   108,
     137,    56,   167,   117,   -36,   -36,   220,   -14,   220,   -14,
     -12,   -36,   -12,   -36,   -36,   -36,   220,   -14,   220,   -14,
     -12,   -36,   -12,   -36,    38,    11,   -36,   -36,    88,     6,
       6,     6,     6,     6,    88,   -36,    10,   236,    40,    88,
     179,   150,    36,     6,   162,   174,   186,   198,   203,   191,
      73,   -36,    46,   245,    26,   -36,   -36,     6,   210,     6,
       6,     6,     6,   -36,   -36,   -36,   -36,   -36,   -36,    45,
      88,   215,     3,     3,   -36,   -36,   -36,   245,    26
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -36,   -36,   -36,    35,     0
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
      12,    13,    14,    15,     1,     2,     3,     4,    21,     1,
       2,    16,    82,    34,    17,    39,    31,    32,     5,     6,
      27,    28,    18,    47,    49,    51,    53,    22,     7,    57,
      59,    61,    63,   101,   102,    29,    30,    31,    32,     8,
      43,    64,    78,    20,    83,    79,    45,    90,    33,    36,
      38,    40,    41,    42,    29,    30,    31,    32,    46,    48,
      50,    52,    54,    55,    56,    58,    60,    62,    81,    84,
      85,    86,    87,    88,    97,    74,    92,   109,    77,    94,
     110,   116,     0,    98,    23,    24,    25,    26,    27,    28,
       0,     1,     2,    19,     4,     0,     0,   111,     0,   112,
     113,   114,   115,    80,     0,     5,     6,     0,     0,    89,
     118,     0,     0,     0,    93,     7,     1,    35,    19,     4,
       1,    76,    19,     4,     0,     0,     8,     0,     0,     0,
       5,     6,     0,     0,     5,     6,    29,    30,    31,    32,
       7,     0,     0,     0,     7,   117,     0,    67,     0,     0,
       0,     8,     0,     0,     0,     8,    68,    69,    70,    71,
      72,    73,     0,     0,     0,    23,    24,    25,    26,    27,
      28,    23,    24,    25,    26,    27,    28,     0,    29,    30,
      31,    32,    44,    23,    24,    25,    26,    27,    28,    96,
      99,   100,   101,   102,    66,    23,    24,    25,    26,    27,
      28,   103,    99,   100,   101,   102,    75,    23,    24,    25,
      26,    27,    28,   104,    99,   100,   101,   102,    95,    23,
      24,    25,    26,    27,    28,   105,    99,   100,   101,   102,
     108,    99,   100,   101,   102,     0,     0,   106,    99,   100,
     101,   102,   107,    99,   100,   101,   102,     0,     0,    45,
      25,    26,    27,    28,    67,    23,    24,    25,    26,    27,
      28,     0,     0,    65,    23,    24,    25,    26,    27,    28,
       0,     0,    91,    23,    24,    25,    26,    27,    28
};

static const yysigned_char yycheck[] =
{
       0,    34,    35,    38,     3,     4,     5,     6,     8,     3,
       4,    38,     6,    13,    38,    15,    30,    31,    17,    18,
      32,    33,    38,    23,    24,    25,    26,     0,    27,    29,
      30,    31,    32,    30,    31,    28,    29,    30,    31,    38,
      35,    37,     4,     8,    38,    34,    39,    37,    13,    14,
      15,    16,    17,    18,    28,    29,    30,    31,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    68,    69,
      70,    71,    72,    73,    38,    19,    36,     4,    43,    79,
      34,    36,    -1,    83,    28,    29,    30,    31,    32,    33,
      -1,     3,     4,     5,     6,    -1,    -1,    97,    -1,    99,
     100,   101,   102,    68,    -1,    17,    18,    -1,    -1,    74,
     110,    -1,    -1,    -1,    79,    27,     3,     4,     5,     6,
       3,     4,     5,     6,    -1,    -1,    38,    -1,    -1,    -1,
      17,    18,    -1,    -1,    17,    18,    28,    29,    30,    31,
      27,    -1,    -1,    -1,    27,   110,    -1,    39,    -1,    -1,
      -1,    38,    -1,    -1,    -1,    38,    19,    20,    21,    22,
      23,    24,    -1,    -1,    -1,    28,    29,    30,    31,    32,
      33,    28,    29,    30,    31,    32,    33,    -1,    28,    29,
      30,    31,    39,    28,    29,    30,    31,    32,    33,    39,
      28,    29,    30,    31,    39,    28,    29,    30,    31,    32,
      33,    39,    28,    29,    30,    31,    39,    28,    29,    30,
      31,    32,    33,    39,    28,    29,    30,    31,    39,    28,
      29,    30,    31,    32,    33,    39,    28,    29,    30,    31,
      39,    28,    29,    30,    31,    -1,    -1,    39,    28,    29,
      30,    31,    39,    28,    29,    30,    31,    -1,    -1,    39,
      30,    31,    32,    33,    39,    28,    29,    30,    31,    32,
      33,    -1,    -1,    36,    28,    29,    30,    31,    32,    33,
      -1,    -1,    36,    28,    29,    30,    31,    32,    33
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     3,     4,     5,     6,    17,    18,    27,    38,    41,
      42,    43,    44,    34,    35,    38,    38,    38,    38,     5,
      43,    44,     0,    28,    29,    30,    31,    32,    33,    28,
      29,    30,    31,    43,    44,     4,    43,    44,    43,    44,
      43,    43,    43,    35,    39,    39,    43,    44,    43,    44,
      43,    44,    43,    44,    43,    43,    43,    44,    43,    44,
      43,    44,    43,    44,    37,    36,    39,    39,    19,    20,
      21,    22,    23,    24,    19,    39,     4,    43,     4,    34,
      43,    44,     6,    38,    44,    44,    44,    44,    44,    43,
      37,    36,    36,    43,    44,    39,    39,    38,    44,    28,
      29,    30,    31,    39,    39,    39,    39,    39,    39,     4,
      34,    44,    44,    44,    44,    44,    36,    43,    44
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

#define yyerrok                (yyerrstatus = 0)
#define yyclearin        (yychar = YYEMPTY)
#define YYEMPTY                (-2)
#define YYEOF                0

#define YYACCEPT        goto yyacceptlab
#define YYABORT                goto yyabortlab
#define YYERROR                goto yyerrlab1


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
      YYPOPSTACK;                                                \
      goto yybackup;                                                \
    }                                                                \
  else                                                                \
    {                                                                 \
      yyerror ("syntax error: cannot back up");\
      YYERROR;                                                        \
    }                                                                \
while (0)

#define YYTERROR        1
#define YYERRCODE        256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
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
} while (0)

# define YYDSYMPRINT(Args)                        \
do {                                                \
  if (yydebug)                                        \
    yysymprint Args;                                \
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)                \
do {                                                                \
  if (yydebug)                                                        \
    {                                                                \
      YYFPRINTF (stderr, "%s ", Title);                                \
      yysymprint (stderr,                                         \
                  Token, Value);        \
      YYFPRINTF (stderr, "\n");                                        \
    }                                                                \
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
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

# define YY_STACK_PRINT(Bottom, Top)                                \
do {                                                                \
  if (yydebug)                                                        \
    yy_stack_print ((Bottom), (Top));                                \
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
  unsigned int yylineno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylineno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)                \
do {                                        \
  if (yydebug)                                \
    yy_reduce_print (Rule);                \
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
#ifndef        YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
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
  (void) yytype;
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
  short        yyssa[YYINITDEPTH];
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
  yychar = YYEMPTY;                /* Cause a token to be read.  */

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
        case 2:

    {
                /* 
                printf("Complete\n");
                printf("XdmfExprReturnValue Nelms = %d\n", XdmfExprReturnValue->GetNumberOfElements());
                */
                }
    break;

  case 3:

    {
                xdmf2::XdmfArray *TempArray = ( xdmf2::XdmfArray *)yyvsp[0].ArrayPointer;

                /* printf("Setting %s from ArrayExpression\n", $1); */
                XdmfExprReturnValue = (xdmf2::XdmfArray *)yyvsp[-2].ArrayPointer;
                *XdmfExprReturnValue = *TempArray;
                delete TempArray;
                }
    break;

  case 4:

    {
                /* printf("Setting %s from ScalarExpression\n", $1); */
                XdmfExprReturnValue = (xdmf2::XdmfArray *)yyvsp[-2].ArrayPointer;
                *XdmfExprReturnValue = yyvsp[0].DoubleValue;
                }
    break;

  case 5:

    {
                        xdmf2::XdmfArray        *Array1 = ( xdmf2::XdmfArray *)yyvsp[-3].ArrayPointer;
                        xdmf2::XdmfArray        *Result = ( xdmf2::XdmfArray *)yyvsp[-5].ArrayPointer;
                        XdmfLength        i, index, Length = Array1->GetNumberOfElements();

                        for( i = 0 ; i < Length ; i++ ){
                                index = (XdmfLength)Array1->GetValueAsFloat64( i );
                                Result->SetValueFromFloat64( index, yyvsp[0].DoubleValue );
                                }
                        delete Array1;
                        XdmfExprReturnValue = Result;
                }
    break;

  case 6:

    {
                        xdmf2::XdmfArray        *Array1 = ( xdmf2::XdmfArray *)yyvsp[-3].ArrayPointer;
                        xdmf2::XdmfArray        *Array2 = ( xdmf2::XdmfArray *)yyvsp[0].ArrayPointer;
                        xdmf2::XdmfArray        *Result = ( xdmf2::XdmfArray *)yyvsp[-5].ArrayPointer;
                        XdmfFloat64        Value;
                        XdmfLength        i, index, Length = Array1->GetNumberOfElements();

                        for( i = 0 ; i < Length ; i++ ){
                                index = (XdmfLength)Array1->GetValueAsFloat64( i );
                                Value = Array2->GetValueAsFloat64( i );
                                Result->SetValueFromFloat64( index, Value );
                                }
                        delete Array1;
                        delete Array2;
                        XdmfExprReturnValue = Result;
                }
    break;

  case 7:

    {
                        xdmf2::XdmfArray *Range;

                        /* printf("Array Range %d:%d = ScalarExpression \n", $3, $5);         */
                        Range = (xdmf2::XdmfArray *)yyvsp[-7].ArrayPointer;
                        XdmfExprReturnValue = Range->Reference( yyvsp[-5].IntegerValue, yyvsp[-3].IntegerValue ); /* This is a Reference */
                        *XdmfExprReturnValue = yyvsp[0].DoubleValue;

                        /* Now Point to the Entire Array */
                        XdmfExprReturnValue = (xdmf2::XdmfArray *)yyvsp[-7].ArrayPointer;
                        }
    break;

  case 8:

    {
                        xdmf2::XdmfArray *TempArray = ( xdmf2::XdmfArray *)yyvsp[0].ArrayPointer;
                        xdmf2::XdmfArray *Range;

                        /* printf("Array Range %d:%d = ArrayExpression \n", $3, $5);         */
                        Range = (xdmf2::XdmfArray *)yyvsp[-7].ArrayPointer;
                        XdmfExprReturnValue = Range->Reference( yyvsp[-5].IntegerValue, yyvsp[-3].IntegerValue ); /* This is a Reference */
                        *XdmfExprReturnValue = *TempArray;

                        /* Now Point to the Entire Array */
                        XdmfExprReturnValue = (xdmf2::XdmfArray *)yyvsp[-7].ArrayPointer;
                        delete TempArray;
                        }
    break;

  case 9:

    {
                xdmf2::XdmfArray *TempArray = ( xdmf2::XdmfArray *)yyvsp[0].ArrayPointer;

                /* printf("Clone from ArrayExpression\n"); */
                XdmfExprReturnValue = TempArray;        
                /* printf("XdmfExprReturnValue Nelms = %d\n", XdmfExprReturnValue->GetNumberOfElements()); */
                }
    break;

  case 10:

    {
                printf("Pointless !! Scalar = %g\n", yyvsp[0].DoubleValue);
                }
    break;

  case 11:

    {
                        xdmf2::XdmfArray *Array1 = ( xdmf2::XdmfArray *)yyvsp[-2].ArrayPointer;
                        xdmf2::XdmfArray *Array2 = ( xdmf2::XdmfArray *)yyvsp[0].ArrayPointer;

                        /* printf("Array 0x%X + 0x%X\n", Array1, Array2); */
                        *Array1 += *Array2;
                        yyval.ArrayPointer = Array1;
                        delete Array2;
                        }
    break;

  case 12:

    {
                        /* Interlace */
                        xdmf2::XdmfArray *Array1 = ( xdmf2::XdmfArray *)yyvsp[-2].ArrayPointer;
                        xdmf2::XdmfArray *Array2 = ( xdmf2::XdmfArray *)yyvsp[0].ArrayPointer;
                        xdmf2::XdmfArray *NewArray = new xdmf2::XdmfArray();
                        XdmfInt32 i, Rank1, Rank2;
                        XdmfInt64 NewLength, Length1, Length2, IFactor, Lcd;
                        XdmfInt64 Dimension1[ XDMF_MAX_DIMENSION ];
                        XdmfInt64 Dimension2[ XDMF_MAX_DIMENSION ];
                        XdmfInt64 Start[ XDMF_MAX_DIMENSION ];
                        XdmfInt64 Stride[ XDMF_MAX_DIMENSION ];
                        XdmfInt64 Count[ XDMF_MAX_DIMENSION ];
                        XdmfInt64 NewDimension[ XDMF_MAX_DIMENSION ];

                        /* printf("Array 0x%X , 0x%X\n", Array1, Array2); */
                        
                        Rank1 = Array1->GetShape( Dimension1 );
                        Rank2 = Array2->GetShape( Dimension2 );
                        if( Rank1 != Rank2 ){
                                printf(" Interlace : Rank Mismatch !!\n");
                                }
                        NewArray->CopyType( Array1 );

                        Length1 = Array1->GetNumberOfElements();
                        Length2 = Array2->GetNumberOfElements();
                        NewLength = Length1 + Length2;
                        IFactor = Length1 / Length2;
                        Lcd = Length1;
                        if( Length2 < Length1 ){
                                Lcd = Length2;
                                }
                        NewDimension[0] = Lcd;
                        NewDimension[1] = NewLength / Lcd;
                        NewArray->SetShape( 2, NewDimension );
                        /*
                        printf("Rank1 = %d Rank2 = %d\n", Rank1, Rank2 );
                        printf("Array1 Size = %d\n", Array1->GetNumberOfElements() );
                        printf("Array2 Size = %d\n", Array2->GetNumberOfElements() );
                        printf("NewLength = %d\n", NewLength );
                        printf("Lcd = %d\n", Lcd );
                        printf("IFactor = %d\n", IFactor );
                        printf("New Dims = %s\n", NewArray->GetShapeAsString() );
                        */
                        /* NewArray->Generate( -55.0,  -55.0 ); */
                        /* Copy in Array 1 */
                        Start[0] = 0; Start[1] = 0;
                        Stride[0] = 1; Stride[1] = 1;
                        Count[0] = Lcd; Count[1] = Length1 / Lcd;
                        NewArray->SelectHyperSlab( Start, Stride, Count );
                        Array1->SelectAll();
                        /*
                        printf("Copy in Array1 = %s\n", NewArray->GetHyperSlabAsString() );
                        */
                        CopyArray( Array1, NewArray );
                        /* Copy in Array 2 */
                        Start[0] = 0; Start[1] = Length1 / Lcd;
                        Stride[0] = 1; Stride[1] = 1;
                        Count[0] = Lcd; Count[1] = Length2 / Lcd;
                        NewArray->SelectHyperSlab( Start, Stride, Count );
                        Array2->SelectAll();
                        /*
                        printf("Copy in Array2 = %s\n", NewArray->GetHyperSlabAsString() );
                        */
                        CopyArray( Array2, NewArray );
                        NewDimension[0] = Dimension1[0] + Dimension2[0];
                        for( i = 1 ; i < Rank1 ; i++ ){
                                NewDimension[i] = Dimension1[i];
                                } 
                        NewArray->Reform( Rank1, NewDimension );
                        /*        
                        printf("Result(%s) = %s\n", NewArray->GetShapeAsString(), NewArray->GetValues() );
                        */
                        yyval.ArrayPointer = NewArray;
                        delete Array1;
                        delete Array2;
                        }
    break;

  case 13:

    {
                        /* Interlace */
                        xdmf2::XdmfArray *Array1 = ( xdmf2::XdmfArray *)yyvsp[-2].ArrayPointer;
                        xdmf2::XdmfArray *Array2 = ( xdmf2::XdmfArray *)yyvsp[0].ArrayPointer;
                        xdmf2::XdmfArray *NewArray = new xdmf2::XdmfArray();
                        XdmfInt32 i, Rank1, Rank2;
                        XdmfInt64 Dimension1[ XDMF_MAX_DIMENSION ];
                        XdmfInt64 Dimension2[ XDMF_MAX_DIMENSION ];
                        XdmfInt64 Start[ XDMF_MAX_DIMENSION ];
                        XdmfInt64 Stride[ XDMF_MAX_DIMENSION ];
                        XdmfInt64 Count[ XDMF_MAX_DIMENSION ];
                        XdmfInt64 NewDimension[ XDMF_MAX_DIMENSION ];

                         /* printf("Array 0x%X  << 0x%X\n", Array1, Array2); */
                        
                        Rank1 = Array1->GetShape( Dimension1 );
                        Rank2 = Array2->GetShape( Dimension2 );
                        if( Rank1 != Rank2 ){
                                printf(" Cat : Rank Mismatch !!\n");
                                }
                        NewDimension[0] = Dimension1[0] + Dimension2[0];
                        for( i = 1 ; i < Rank1 ; i++ ){
                                NewDimension[i] = Dimension1[i];
                                } 
                        NewArray->CopyType( Array1 );
                        NewArray->SetShape( Rank1, NewDimension );

                        /*
                        NewArray->Generate( -55.0,  -55.0 );
                        */
                        /* Copy in Array 1 */
                        for( i = 0 ; i < Rank1 ; i++ ){
                                Start[i] = 0;
                                Stride[i] = 1;
                                Count[i] = Dimension1[i];
                                }
                        NewArray->SelectHyperSlab( Start, Stride, Count );
                        Array1->SelectAll();
                        /*
                        printf("Copy in Array1 = %s\n", NewArray->GetHyperSlabAsString() );
                        */
                        CopyArray( Array1, NewArray );
                        /* Copy in Array 2 */
                        Start[0] = Dimension1[0];
                        Stride[0] = 1;
                        Count[0] = Dimension2[0];
                        for( i = 1 ; i < Rank1 ; i++ ){
                                Start[i] = 0;
                                Stride[i] = 1;
                                Count[i] = Dimension1[i];
                                }
                        NewArray->SelectHyperSlab( Start, Stride, Count );
                        Array2->SelectAll();
                        /*
                        printf("Copy in Array2 = %s\n", NewArray->GetHyperSlabAsString() );
                        */
                        CopyArray( Array2, NewArray );
                        /*
                        printf("Result(%s) = %s\n", NewArray->GetShapeAsString(), NewArray->GetValues() );
                        */
                        yyval.ArrayPointer = NewArray;
                        delete Array1;
                        delete Array2;
                        }
    break;

  case 14:

    {
                        xdmf2::XdmfArray *Array1 = ( xdmf2::XdmfArray *)yyvsp[-2].ArrayPointer;
                        xdmf2::XdmfArray *Array2 = ( xdmf2::XdmfArray *)yyvsp[0].ArrayPointer;

                        /* printf("Array 0x%X + 0x%X\n", Array1, Array2); */
                        *Array1 -= *Array2;
                        yyval.ArrayPointer = Array1;
                        delete Array2;
                        }
    break;

  case 15:

    {
                        xdmf2::XdmfArray *Array1 = ( xdmf2::XdmfArray *)yyvsp[-2].ArrayPointer;
                        xdmf2::XdmfArray *Array2 = ( xdmf2::XdmfArray *)yyvsp[0].ArrayPointer;

                        /* printf("Array 0x%X * 0x%X\n", Array1, Array2); */
                        *Array1 *= *Array2;
                        yyval.ArrayPointer = Array1;
                        delete Array2;
                        /* printf("Array1 Nelms = %d\n", Array1->GetNumberOfElements()); */
                        }
    break;

  case 16:

    {
                        xdmf2::XdmfArray *Array1 = ( xdmf2::XdmfArray *)yyvsp[-2].ArrayPointer;
                        xdmf2::XdmfArray *Array2 = ( xdmf2::XdmfArray *)yyvsp[0].ArrayPointer;

                        /* printf("Array 0x%X + 0x%X\n", Array1, Array2); */
                        *Array1 /= *Array2;
                        yyval.ArrayPointer = Array1;
                        delete Array2;
                        }
    break;

  case 17:

    {
                        xdmf2::XdmfArray *Array1 = ( xdmf2::XdmfArray *)yyvsp[-2].ArrayPointer;
                        xdmf2::XdmfArray *Result;

                        /* printf("Array + %g\n", $3); */
                        Result  = Array1;
                        *Result += yyvsp[0].DoubleValue;
                        yyval.ArrayPointer = Result;
                        }
    break;

  case 18:

    {
                        xdmf2::XdmfArray *Array1 = ( xdmf2::XdmfArray *)yyvsp[-2].ArrayPointer;
                        xdmf2::XdmfArray *Result;

                        /* printf("Array - %g\n", $3); */
                        Result  = Array1;
                        *Result -= yyvsp[0].DoubleValue;
                        yyval.ArrayPointer = Result;
                        }
    break;

  case 19:

    {
                        xdmf2::XdmfArray *Array1 = ( xdmf2::XdmfArray *)yyvsp[-2].ArrayPointer;
                        xdmf2::XdmfArray *Result;

                        /* printf("Array * %g\n", $3); */
                        Result  = Array1;
                        *Result *= yyvsp[0].DoubleValue;
                        yyval.ArrayPointer = Result;
                        }
    break;

  case 20:

    {
                        xdmf2::XdmfArray *Array1 = ( xdmf2::XdmfArray *)yyvsp[-2].ArrayPointer;
                        xdmf2::XdmfArray *Result;

                        /* printf("Array / %g\n", $3); */
                        Result  = Array1;
                        *Result /= yyvsp[0].DoubleValue;
                        yyval.ArrayPointer = Result;
                        }
    break;

  case 21:

    {
                        xdmf2::XdmfArray *Array1 = ( xdmf2::XdmfArray *)yyvsp[0].ArrayPointer;
                        xdmf2::XdmfArray *Result;

                        /* printf("Array + %g\n", $1); */
                        Result  = Array1;
                        *Result += yyvsp[-2].DoubleValue;
                        yyval.ArrayPointer = Result;
                        }
    break;

  case 22:

    {
                        xdmf2::XdmfArray *Array1 = ( xdmf2::XdmfArray *)yyvsp[0].ArrayPointer;
                        xdmf2::XdmfArray *Result;

                        /* printf("Array - %g\n", $1); */
                        Result  = Array1;
                        *Result -= yyvsp[-2].DoubleValue;
                        yyval.ArrayPointer = Result;
                        }
    break;

  case 23:

    {
                        xdmf2::XdmfArray *Array1 = ( xdmf2::XdmfArray *)yyvsp[0].ArrayPointer;
                        xdmf2::XdmfArray *Result;

                        /* printf("Array * %g\n", $1); */
                        Result  = Array1;
                        *Result *= yyvsp[-2].DoubleValue;
                        yyval.ArrayPointer = Result;
                        }
    break;

  case 24:

    {
                        xdmf2::XdmfArray *Array1 = ( xdmf2::XdmfArray *)yyvsp[0].ArrayPointer;
                        xdmf2::XdmfArray *Result;

                        /* printf("Array / %g\n", $1); */
                        Result  = Array1;
                        *Result /= yyvsp[-2].DoubleValue;
                        yyval.ArrayPointer = Result;
                        }
    break;

  case 25:

    {
                        xdmf2::XdmfArray        *Array1 = ( xdmf2::XdmfArray *)yyvsp[-3].ArrayPointer;
                        xdmf2::XdmfArray        *Array2 = ( xdmf2::XdmfArray *)yyvsp[-1].ArrayPointer;
                        xdmf2::XdmfArray        *Result;

                        /* printf("ArrayExpression From Indexes\n"); */
                        Result = Array1->Clone( Array2 );
                        delete Array2;
                        yyval.ArrayPointer = Result;
                }
    break;

  case 26:

    {
                        xdmf2::XdmfArray *Array1 = ( xdmf2::XdmfArray *)yyvsp[-5].ArrayPointer;
                        xdmf2::XdmfArray *Range, *Result;

                        /* printf("ArrayExpression From Array Range %d:%d\n", $3, $5);         */
                        Range = Array1->Reference( yyvsp[-3].IntegerValue, yyvsp[-1].IntegerValue ); /* This not a copy  */
        
                        Result  = Range->Clone(); /* So Copy It */
                        delete Array1;
                        yyval.ArrayPointer = Result;
                        }
    break;

  case 27:

    {
                        xdmf2::XdmfArray        *Array1 = ( xdmf2::XdmfArray *)yyvsp[-3].ArrayPointer;
                        xdmf2::XdmfArray        *Array2 = ( xdmf2::XdmfArray *)yyvsp[-1].ArrayPointer;
                        XdmfLength        i, howmany = 0, cntr = 0;
                        XdmfLength        Length1 = Array1->GetNumberOfElements(), Length2;
                        xdmf2::XdmfInt64Array        *Index = new xdmf2::XdmfInt64Array( Length1 );
                        XdmfInt64        A1Value, A2Value;
                        XdmfInt64        *A1Values, *A2Values;
                        float                Percent;

                        if(Array1->GetNumberType() != XDMF_INT64_TYPE){
                                yyerror("INDEX operator only uses XdmfInt64 Arrays");
                                return( 0 );
                                }
                        if(Array2->GetNumberType() != XDMF_INT64_TYPE){
                                yyerror("INDEX operator only uses XdmfInt64 Arrays");
                                return( 0 );
                                }
                        Length2 = Array2->GetNumberOfElements();
                        A1Values = (XdmfInt64 *)Array1->GetDataPointer();
                        A2Values = (XdmfInt64 *)Array2->GetDataPointer();
                        for( i = 0 ; i < Length1 ; i++ ){
                                /* A1Value = Array1->GetValueAsFloat64( i ); */
                                A1Value = *A1Values++;
                                cntr = 0;
                                A2Value = A1Value + 1;
                                while((cntr < Length2) && (A2Value != A1Value)) {
                                        /* A2Value = Array2->GetValueAsFloat64(cntr); */
                                        A2Value = A2Values[cntr];
                                        cntr++;
                                        }
                                howmany++;
                                if(howmany > 5000){
                                        Percent = 100.0 * i / Length1;
                                        printf("%5.2f %% Done\n", Percent);
                                        howmany = 0;
                                        }
                                if( A1Value == A2Value ) {
                                        Index->SetValue( i, cntr - 1 );
                                }else{
                                        Index->SetValue( i, -1);
                                        }
                                }        
                        yyval.ArrayPointer = ( xdmf2::XdmfArray *)Index;
                        }
    break;

  case 28:

    {
                        xdmf2::XdmfArray        *Array1 = ( xdmf2::XdmfArray *)yyvsp[-3].ArrayPointer;
                        /* XdmfLength        howmany = 0; */
                        XdmfLength        i, cntr = 0;
                        XdmfLength        Length1 = Array1->GetNumberOfElements(), Length2;
                        xdmf2::XdmfInt64Array        *Index = new xdmf2::XdmfInt64Array( Length1 );
                        xdmf2::XdmfArray        *Array2 = ( xdmf2::XdmfArray *)yyvsp[-1].ArrayPointer;
                        XdmfFloat64        A1Value, A2Value;

                        Length2 = Array2->GetNumberOfElements();
                        for( i = 0 ; i < Length1 ; i++ ){
                                A1Value = Array1->GetValueAsFloat64( i );
                                cntr = 0;
                                A2Value = A1Value + 1;
                                while((cntr < Length2) && (A2Value != A1Value)) {
                                        A2Value = Array2->GetValueAsFloat64(cntr);
                                        cntr++;
                                        }
/*
                                howmany++;
                                if(howmany > 1000){
                                        cout << "Checked " << i << " of " << Length1 << endl;
                                        howmany = 0;
                                        }
*/
                                if( A1Value == A2Value ) {
                                        Index->SetValue( i, cntr - 1 );
                                }else{
                                        Index->SetValue( i, -1);
                                        }
                                }        
                        yyval.ArrayPointer = ( xdmf2::XdmfArray *)Index;
                        }
    break;

  case 29:

    {
                        xdmf2::XdmfArray        *Array1 = ( xdmf2::XdmfArray *)yyvsp[-3].ArrayPointer;
                        XdmfLength        i, cntr = 0;
                        XdmfLength        Length = Array1->GetNumberOfElements();
                        xdmf2::XdmfInt64Array        *Index = new xdmf2::XdmfInt64Array( Length );
                        XdmfFloat64        Value, SValue = yyvsp[-1].DoubleValue;

                        for( i = 0 ; i < Length ; i++ ){
                                Value = Array1->GetValueAsFloat64( i );
                                if( Value == SValue ) {
                                        Index->SetValue( cntr++, i );
                                        }
                                }        
                        /* printf("Found %d Wheres\n", cntr ); */
                        if( cntr == 0 ){
                                yyerror("WHERE Function Length == 0");
                                return( 0 );
                                }
                        Index->SetNumberOfElements( cntr );
                        yyval.ArrayPointer = ( xdmf2::XdmfArray *)Index;
                        }
    break;

  case 30:

    {
                        xdmf2::XdmfArray        *Array1 = ( xdmf2::XdmfArray *)yyvsp[-3].ArrayPointer;
                        XdmfLength        i, cntr = 0;
                        XdmfLength        Length = Array1->GetNumberOfElements();
                        xdmf2::XdmfInt64Array        *Index = new xdmf2::XdmfInt64Array( Length );
                        XdmfFloat64        Value, SValue = yyvsp[-1].DoubleValue;

                        for( i = 0 ; i < Length ; i++ ){
                                Value = Array1->GetValueAsFloat64( i );
                                if( Value < SValue ) {
                                        Index->SetValue( cntr++, i );
                                        }
                                }        
                        /* printf("Found %d Wheres\n", cntr ); */
                        if( cntr == 0 ){
                                yyerror("WHERE Function Length == 0");
                                return( 0 );
                                }
                        Index->SetNumberOfElements( cntr );
                        yyval.ArrayPointer = ( xdmf2::XdmfArray *)Index;
                        }
    break;

  case 31:

    {
                        xdmf2::XdmfArray        *Array1 = ( xdmf2::XdmfArray *)yyvsp[-3].ArrayPointer;
                        XdmfLength        i, cntr = 0;
                        XdmfLength        Length = Array1->GetNumberOfElements();
                        xdmf2::XdmfInt64Array        *Index = new xdmf2::XdmfInt64Array( Length );
                        XdmfFloat64        Value, SValue = yyvsp[-1].DoubleValue;

                        for( i = 0 ; i < Length ; i++ ){
                                Value = Array1->GetValueAsFloat64( i );
                                if( Value <= SValue ) {
                                        Index->SetValue( cntr++, i );
                                        }
                                }        
                        /* printf("Found %d Wheres\n", cntr ); */
                        if( cntr == 0 ){
                                yyerror("WHERE Function Length == 0");
                                return( 0 );
                                }
                        Index->SetNumberOfElements( cntr );
                        yyval.ArrayPointer = ( xdmf2::XdmfArray *)Index;
                        }
    break;

  case 32:

    {
                        xdmf2::XdmfArray        *Array1 = ( xdmf2::XdmfArray *)yyvsp[-3].ArrayPointer;
                        XdmfLength        i, cntr = 0;
                        XdmfLength        Length = Array1->GetNumberOfElements();
                        xdmf2::XdmfInt64Array        *Index = new xdmf2::XdmfInt64Array( Length );
                        XdmfFloat64        Value, SValue = yyvsp[-1].DoubleValue;

                        for( i = 0 ; i < Length ; i++ ){
                                Value = Array1->GetValueAsFloat64( i );
                                if( Value > SValue ) {
                                        Index->SetValue( cntr++, i );
                                        }
                                }        
                        /* printf("Found %d Wheres\n", cntr ); */
                        if( cntr == 0 ){
                                yyerror("WHERE Function Length == 0");
                                return( 0 );
                                }
                        Index->SetNumberOfElements( cntr );
                        yyval.ArrayPointer = ( xdmf2::XdmfArray *)Index;
                        }
    break;

  case 33:

    {
                        xdmf2::XdmfArray        *Array1 = ( xdmf2::XdmfArray *)yyvsp[-3].ArrayPointer;
                        XdmfLength        i, cntr = 0;
                        XdmfLength        Length = Array1->GetNumberOfElements();
                        xdmf2::XdmfInt64Array        *Index = new xdmf2::XdmfInt64Array( Length );
                        XdmfFloat64        Value, SValue = yyvsp[-1].DoubleValue;

                        for( i = 0 ; i < Length ; i++ ){
                                Value = Array1->GetValueAsFloat64( i );
                                if( Value >= SValue ) {
                                        Index->SetValue( cntr++, i );
                                        }
                                }        
                        /* printf("Found %d Wheres\n", cntr ); */
                        if( cntr == 0 ){
                                yyerror("WHERE Function Length == 0");
                                return( 0 );
                                }
                        Index->SetNumberOfElements( cntr );
                        yyval.ArrayPointer = ( xdmf2::XdmfArray *)Index;
                        }
    break;

  case 34:

    {
                        xdmf2::XdmfArray        *Array1 = ( xdmf2::XdmfArray *)yyvsp[-3].ArrayPointer;
                        XdmfLength        i, cntr = 0;
                        XdmfLength        Length = Array1->GetNumberOfElements();
                        xdmf2::XdmfInt64Array        *Index = new xdmf2::XdmfInt64Array( Length );
                        XdmfFloat64        Value, SValue = yyvsp[-1].DoubleValue;

                        for( i = 0 ; i < Length ; i++ ){
                                Value = Array1->GetValueAsFloat64( i );
                                if( Value != SValue ) {
                                        Index->SetValue( cntr++, i );
                                        }
                                }        
                        /* printf("Found %d Wheres\n", cntr ); */
                        if( cntr == 0 ){
                                yyerror("WHERE Function Length == 0");
                                return( 0 );
                                }
                        Index->SetNumberOfElements( cntr );
                        yyval.ArrayPointer = ( xdmf2::XdmfArray *)Index;
                        }
    break;

  case 35:

    {

                        if( yyvsp[-3].Symbol->DoubleFunctionPtr == NULL ){
                                /* printf("Bad Function Ptr for %s\n", $1->Name ); */
                                yyval.ArrayPointer = yyvsp[-1].ArrayPointer;
                        } else {
                                xdmf2::XdmfArray *Array1 = ( xdmf2::XdmfArray *)yyvsp[-1].ArrayPointer;
                                XdmfFloat64        Value;
                                XdmfLength        i, Length = Array1->GetNumberOfElements();

                                /* printf("Function Call %s\n", $1->Name ); */
                                for( i = 0 ; i < Length ; i++ ){
                                        Value = Array1->GetValueAsFloat64( i );
                                        Array1->SetValueFromFloat64( i, (yyvsp[-3].Symbol->DoubleFunctionPtr)( Value ) );
                                        }        
                                yyval.ArrayPointer = Array1;
                        }
                        }
    break;

  case 36:

    {
                        /* printf("( ArrayExpression )\n"); */
                        yyval.ArrayPointer = yyvsp[-1].ArrayPointer;
                        }
    break;

  case 37:

    {
                        /* printf("( ArrayExpression )\n"); */
                        yyval.ArrayPointer = yyvsp[-1].ArrayPointer;
                        }
    break;

  case 38:

    {
                        xdmf2::XdmfArray *Array1 = ( xdmf2::XdmfArray *)yyvsp[0].ArrayPointer;
                        xdmf2::XdmfArray *Result;

                        /* printf("ArrayExpression From Array\n"); */

                        if ( Array1 == NULL ){
                                /* Bomb */
                                yyerror("NULL Array Pointer");
                                return( 0 );
                        } else {
                                Result  = Array1->Clone();
                                yyval.ArrayPointer = Result;
                                }
                        }
    break;

  case 39:

    {
                        /* printf("Scalar +\n"); */
                        yyval.DoubleValue = yyvsp[-2].DoubleValue + yyvsp[0].DoubleValue;
                        }
    break;

  case 40:

    {
                        /* printf("Scalar -\n"); */
                        yyval.DoubleValue = yyvsp[-2].DoubleValue - yyvsp[0].DoubleValue;
                        }
    break;

  case 41:

    {
                        /* printf("Scalar *\n"); */
                        yyval.DoubleValue = yyvsp[-2].DoubleValue * yyvsp[0].DoubleValue;
                        }
    break;

  case 42:

    {
                        /* printf("Scalar /\n"); */
                        yyval.DoubleValue = yyvsp[-2].DoubleValue / yyvsp[0].DoubleValue;
                        }
    break;

  case 43:

    {
                        if( yyvsp[-3].Symbol->DoubleFunctionPtr == NULL ){
                                /* printf("Bad Function Ptr for %s\n", $1->Name ); */
                                yyval.DoubleValue = 0.0;
                        } else {
                                yyval.DoubleValue = (yyvsp[-3].Symbol->DoubleFunctionPtr)( yyvsp[-1].DoubleValue );
                        }
                        }
    break;

  case 44:

    {
                        /* printf ("( ScalarExpression )\n"); */
                        yyval.DoubleValue = yyvsp[-1].DoubleValue;
                        }
    break;

  case 45:

    {
                        /* printf ("ScalarExpression from tokINTEGER\n"); */
                        yyval.DoubleValue = yyvsp[0].IntegerValue;
                        }
    break;

  case 46:

    {
                        /* printf ("ScalarExpression from FLOAT\n"); */
                        yyval.DoubleValue = yyvsp[0].DoubleValue;
                        }
    break;


    }

/* Line 999 of yacc.c.  */


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
          char *yymsg;
          int yyx, yycount;

          yycount = 0;
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  */
          for (yyx = yyn < 0 ? -yyn : 0;
               yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
              yysize += yystrlen (yytname[yyx]) + 15, yycount++;
          yysize += yystrlen ("syntax error, unexpected ") + 1;
          yysize += yystrlen (yytname[yytype]);
          yymsg = (char *) YYSTACK_ALLOC (yysize);
          if (yymsg != 0)
            {
              char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
              yyp = yystpcpy (yyp, yytname[yytype]);

              if (yycount < 5)
                {
                  yycount = 0;
                  for (yyx = yyn < 0 ? -yyn : 0;
                       yyx < (int) (sizeof (yytname) / sizeof (char *));
                       yyx++)
                    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
                      {
                        const char *yyq = ! yycount ? ", expecting " : " or ";
                        yyp = yystpcpy (yyp, yyq);
                        yyp = yystpcpy (yyp, yytname[yyx]);
                        yycount++;
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

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
          /* Pop the error token.  */
          YYPOPSTACK;
          /* Pop the rest of the stack.  */
          while (yyss < yyssp)
            {
              YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
              yydestruct (yystos[*yyssp], yyvsp);
              YYPOPSTACK;
            }
          YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
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

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

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





/* extern        FILE        *yyin, *yyout; */

#ifdef __cplusplus
/**/
extern "C" {
/**/
#endif

char        InputBuffer[ 512 ];
int         InputBufferPtr = 0, InputBufferEnd = 0;
char        OutputBuffer[ 512 ];
int         OutputBufferPtr = 0;
/* static int OutputBufferEnd = 511; */

int
dice_yywrap( void ) {

return 1;
}

void
dice_yyerror( const char *string ) {
fprintf(stderr, "XdmfExpr : %s \n", string);
}

int
XdmfExprFlexInput( char *buf, int maxlen ) {
(void)maxlen;
if ( InputBufferPtr < InputBufferEnd ){
        buf[0] = InputBuffer[ InputBufferPtr++ ];
        return(1);
} else {
        buf[0] = '\n';
        return( 0 );
        }
}

int
XdmfExprInput( void ){

if ( InputBufferPtr < InputBufferEnd ){
        return( InputBuffer[ InputBufferPtr++ ] );
} else {
        return '\n';
        }
}

void
XdmfExprUnput( int c ) {
if( InputBufferPtr > 0 ){
        InputBufferPtr--;
        InputBuffer[ InputBufferPtr ] = c;
        }
}

void
XdmfExprOutput( int c ) {
        /* printf("XdmfExprOutput Called\n"); */
        OutputBuffer[ OutputBufferPtr++ ] = c;
        OutputBuffer[ OutputBufferPtr ] = '\0';
        }

XdmfExprSymbol
*XdmfExprSymbolLookup( const char *Name ){

XdmfExprSymbol        *Last = NULL, *Item = XdmfExprItemsTable;

if( Name == NULL ) {
        /* Table Check  */
        return( XdmfExprItemsTable );
        }

while( Item != NULL ) {
        if( strcmp( Item->Name, Name ) == 0 ) {
                /* printf("Found Symbol %s\n", Name ); */
                return( Item );
                }
        Last = Item;
        Item = Item->Next;
}
/* Not Found : Create New One */
Item = ( XdmfExprSymbol *)calloc( 1, sizeof( XdmfExprSymbol ));
Item->Next = NULL;
Item->Name = strdup( Name );
Item->ClientData = NULL;
Item->DoubleValue = 0;
Item->DoubleFunctionPtr = NULL;
if( XdmfExprItemsTable == NULL ) {
        XdmfExprItemsTable = Item;
        }
if( Last != NULL ){
        Last->Next = Item;
        }
/* printf("New Symbol for %s\n", Name ); */
return( Item );
}

#ifdef __cplusplus
/**/
}
/**/
#endif

xdmf2::XdmfArray *
XdmfExprParse( char *string ){

XdmfExprSymbol        *Item;
XdmfLength        CurrentTime;
XdmfLength        TimeOfCreation;
xdmf2::XdmfArray        *ap;

/* Build the Symbol Table if Necessary */
Item = XdmfExprSymbolLookup( NULL );
if( Item == NULL ){
        /* printf("Creating Symbol Table\n"); */
        Item = XdmfExprSymbolLookup( "cos" );
        Item->DoubleFunctionPtr = cos;
        Item = XdmfExprSymbolLookup( "sin" );
        Item->DoubleFunctionPtr = sin;
        Item = XdmfExprSymbolLookup( "exp" );
        Item->DoubleFunctionPtr = exp;
        Item = XdmfExprSymbolLookup( "tan" );
        Item->DoubleFunctionPtr = tan;
        Item = XdmfExprSymbolLookup( "acos" );
        Item->DoubleFunctionPtr = acos;
        Item = XdmfExprSymbolLookup( "asin" );
        Item->DoubleFunctionPtr = asin;
        Item = XdmfExprSymbolLookup( "atan" );
        Item->DoubleFunctionPtr = atan;
        Item = XdmfExprSymbolLookup( "log" );
        Item->DoubleFunctionPtr = log;
        Item = XdmfExprSymbolLookup( "sqrt" );
        Item->DoubleFunctionPtr = sqrt;
        }
/* Print Symbol Table */
Item = XdmfExprSymbolLookup( NULL );
while( Item != NULL ) {
        if( Item->ClientData != NULL ){
                /* printf("Found Symbol %s\n", Item->Name ); */
                }
        Item = Item->Next;
        }
strcpy( InputBuffer, string );
InputBufferEnd = strlen( InputBuffer );
InputBufferPtr = OutputBufferPtr = 0;
XdmfExprReturnValue = NULL;
/* printf("XdmfExprParse Scanning <%s>\n", InputBuffer); */
 CurrentTime = xdmf2::GetCurrentArrayTime();
if ( yyparse() != 0 ){
        /* Error */
        XdmfExprReturnValue = NULL;
        }
Item = XdmfExprSymbolLookup( NULL );
while( Item != NULL ) {
  XdmfExprSymbol *next = Item->Next;
  if ( Item->Name )
    {
    free(Item->Name);
    }
  free(Item);
        Item = next;
        }

XdmfExprItemsTable = NULL;

/* Remove All Arrays Older than when we started */
/* printf("Cleaning up Temparary Arrays\n"); */
 while( ( ap = xdmf2::GetNextOlderArray( CurrentTime, &TimeOfCreation ) ) != NULL ){
        /* Don't remove the return value */
        if( ap != XdmfExprReturnValue ){
                /* printf("Removing Temporary Array\n"); */
                delete ap;
                }
        CurrentTime = TimeOfCreation;
        }
return( XdmfExprReturnValue );
}


