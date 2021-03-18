#if defined (__GNUC__)                                            
#if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 402                    
#pragma GCC diagnostic ignored "-Wconversion"                     
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"  
#pragma GCC diagnostic ignored "-Wmissing-prototypes"             
#pragma GCC diagnostic ignored "-Wnested-externs"                 
#pragma GCC diagnostic ignored "-Wold-style-definition"           
#pragma GCC diagnostic ignored "-Wredundant-decls"                
#pragma GCC diagnostic ignored "-Wsign-compare"                   
#pragma GCC diagnostic ignored "-Wsign-conversion"                
#pragma GCC diagnostic ignored "-Wstrict-overflow"                
#pragma GCC diagnostic ignored "-Wstrict-prototypes"              
#if !defined (__clang__)                                          
#pragma GCC diagnostic ignored "-Wlarger-than="                   
#pragma GCC diagnostic ignored "-Wsuggest-attribute=const"        
#pragma GCC diagnostic ignored "-Wsuggest-attribute=pure"         
#endif                                                            
#pragma GCC diagnostic ignored "-Wswitch-default"                 
#pragma GCC diagnostic ignored "-Wunused-function"                
#pragma GCC diagnostic ignored "-Wunused-macros"                  
#pragma GCC diagnostic ignored "-Wunused-parameter"               
#endif                                                            
#if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 600                    
#pragma GCC diagnostic ignored "-Wnull-dereference"               
#endif                                                            
#elif defined __SUNPRO_CC                                         
#pragma disable_warn                                              
#elif defined _MSC_VER                                            
#pragma warning(push, 1)                                          
#endif                                                            
/* A Bison parser, made by GNU Bison 2.7.  */

/* XXX(kitware): Mangle all HDF5 HL symbols */
#include "vtk_hdf5_hl_mangle.h"

/* Bison implementation for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2012 Free Software Foundation, Inc.
   
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
#define YYBISON_VERSION "2.7"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         H5LTyyparse
#define yylex           H5LTyylex
#define yyerror         H5LTyyerror
#define yylval          H5LTyylval
#define yychar          H5LTyychar
#define yydebug         H5LTyydebug
#define yynerrs         H5LTyynerrs

/* Copy the first part of user declarations.  */
/* Line 371 of yacc.c  */
#line 20 "hl/src//H5LTparse.y"

#include <stdio.h>
#include <string.h>
#include <hdf5.h>

#include "H5private.h"

extern int yylex(void);
extern int yyerror(const char *);

#define STACK_SIZE      16

/*structure for compound type information*/
struct cmpd_info {
    hid_t       id;             /*type ID*/
    hbool_t     is_field;       /*flag to lexer for compound member*/
    hbool_t     first_memb;     /*flag for first compound member*/
};

/*stack for nested compound type*/
static struct cmpd_info cmpd_stack[STACK_SIZE] = {
    {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1},
    {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1},
    {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1},
    {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1} };

static int csindex = -1;                /*pointer to the top of compound stack*/

/*structure for array type information*/
struct arr_info {
    hsize_t             dims[H5S_MAX_RANK];     /*size of each dimension, limited to 32 dimensions*/
    unsigned            ndims;                  /*number of dimensions*/
    hbool_t             is_dim;                 /*flag to lexer for dimension*/
};
/*stack for nested array type*/
static struct arr_info arr_stack[STACK_SIZE];
static int asindex = -1;               /*pointer to the top of array stack*/ 

static H5T_str_t   str_pad;                /*variable for string padding*/
static H5T_cset_t  str_cset;               /*variable for string character set*/
static hbool_t     is_variable = 0;        /*variable for variable-length string*/
static size_t      str_size;               /*variable for string size*/
   
static hid_t       enum_id;                /*type ID*/
static hbool_t     is_enum = 0;            /*flag to lexer for enum type*/
static hbool_t     is_enum_memb = 0;       /*flag to lexer for enum member*/
static char*       enum_memb_symbol;       /*enum member symbol string*/


/* Line 371 of yacc.c  */
#line 125 "hl/src//H5LTparse.c"

# ifndef YY_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULL nullptr
#  else
#   define YY_NULL 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "H5LTparse.h".  */
#ifndef YY_H5LTYY_HL_SRC_H5LTPARSE_H_INCLUDED
# define YY_H5LTYY_HL_SRC_H5LTPARSE_H_INCLUDED
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int H5LTyydebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     H5T_STD_I8BE_TOKEN = 258,
     H5T_STD_I8LE_TOKEN = 259,
     H5T_STD_I16BE_TOKEN = 260,
     H5T_STD_I16LE_TOKEN = 261,
     H5T_STD_I32BE_TOKEN = 262,
     H5T_STD_I32LE_TOKEN = 263,
     H5T_STD_I64BE_TOKEN = 264,
     H5T_STD_I64LE_TOKEN = 265,
     H5T_STD_U8BE_TOKEN = 266,
     H5T_STD_U8LE_TOKEN = 267,
     H5T_STD_U16BE_TOKEN = 268,
     H5T_STD_U16LE_TOKEN = 269,
     H5T_STD_U32BE_TOKEN = 270,
     H5T_STD_U32LE_TOKEN = 271,
     H5T_STD_U64BE_TOKEN = 272,
     H5T_STD_U64LE_TOKEN = 273,
     H5T_NATIVE_CHAR_TOKEN = 274,
     H5T_NATIVE_SCHAR_TOKEN = 275,
     H5T_NATIVE_UCHAR_TOKEN = 276,
     H5T_NATIVE_SHORT_TOKEN = 277,
     H5T_NATIVE_USHORT_TOKEN = 278,
     H5T_NATIVE_INT_TOKEN = 279,
     H5T_NATIVE_UINT_TOKEN = 280,
     H5T_NATIVE_LONG_TOKEN = 281,
     H5T_NATIVE_ULONG_TOKEN = 282,
     H5T_NATIVE_LLONG_TOKEN = 283,
     H5T_NATIVE_ULLONG_TOKEN = 284,
     H5T_IEEE_F32BE_TOKEN = 285,
     H5T_IEEE_F32LE_TOKEN = 286,
     H5T_IEEE_F64BE_TOKEN = 287,
     H5T_IEEE_F64LE_TOKEN = 288,
     H5T_NATIVE_FLOAT_TOKEN = 289,
     H5T_NATIVE_DOUBLE_TOKEN = 290,
     H5T_NATIVE_LDOUBLE_TOKEN = 291,
     H5T_STRING_TOKEN = 292,
     STRSIZE_TOKEN = 293,
     STRPAD_TOKEN = 294,
     CSET_TOKEN = 295,
     CTYPE_TOKEN = 296,
     H5T_VARIABLE_TOKEN = 297,
     H5T_STR_NULLTERM_TOKEN = 298,
     H5T_STR_NULLPAD_TOKEN = 299,
     H5T_STR_SPACEPAD_TOKEN = 300,
     H5T_CSET_ASCII_TOKEN = 301,
     H5T_CSET_UTF8_TOKEN = 302,
     H5T_C_S1_TOKEN = 303,
     H5T_FORTRAN_S1_TOKEN = 304,
     H5T_OPAQUE_TOKEN = 305,
     OPQ_SIZE_TOKEN = 306,
     OPQ_TAG_TOKEN = 307,
     H5T_COMPOUND_TOKEN = 308,
     H5T_ENUM_TOKEN = 309,
     H5T_ARRAY_TOKEN = 310,
     H5T_VLEN_TOKEN = 311,
     STRING = 312,
     NUMBER = 313
   };
#endif


#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 387 of yacc.c  */
#line 69 "hl/src//H5LTparse.y"

    int     ival;         /*for integer token*/
    char    *sval;        /*for name string*/
    hid_t   hid;          /*for hid_t token*/


/* Line 387 of yacc.c  */
#line 233 "hl/src//H5LTparse.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE H5LTyylval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
hid_t H5LTyyparse (void *YYPARSE_PARAM);
#else
hid_t H5LTyyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
hid_t H5LTyyparse (void);
#else
hid_t H5LTyyparse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_H5LTYY_HL_SRC_H5LTPARSE_H_INCLUDED  */

/* Copy the second part of user declarations.  */

/* Line 390 of yacc.c  */
#line 261 "hl/src//H5LTparse.c"

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
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(N) (N)
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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
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
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
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

# define YYCOPY_NEEDED 1

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

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  58
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   197

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  65
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  43
/* YYNRULES -- Number of rules.  */
#define YYNRULES  92
/* YYNRULES -- Number of states.  */
#define YYNSTATES  134

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   313

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    63,    64,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    61,     2,    62,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    59,     2,    60,     2,     2,     2,     2,
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
      55,    56,    57,    58
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     4,     6,     8,    10,    12,    14,    16,
      18,    20,    22,    24,    26,    28,    30,    32,    34,    36,
      38,    40,    42,    44,    46,    48,    50,    52,    54,    56,
      58,    60,    62,    64,    66,    68,    70,    72,    74,    76,
      78,    80,    82,    84,    86,    88,    90,    92,    93,    99,
     100,   103,   104,   110,   112,   113,   116,   118,   119,   126,
     127,   130,   131,   132,   138,   140,   145,   146,   147,   159,
     161,   163,   164,   165,   166,   167,   187,   189,   191,   193,
     195,   197,   199,   201,   203,   205,   206,   214,   215,   218,
     219,   224,   226
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      66,     0,    -1,    -1,    67,    -1,    68,    -1,    71,    -1,
      79,    -1,    86,    -1,    69,    -1,    70,    -1,    92,    -1,
     101,    -1,    87,    -1,     3,    -1,     4,    -1,     5,    -1,
       6,    -1,     7,    -1,     8,    -1,     9,    -1,    10,    -1,
      11,    -1,    12,    -1,    13,    -1,    14,    -1,    15,    -1,
      16,    -1,    17,    -1,    18,    -1,    19,    -1,    20,    -1,
      21,    -1,    22,    -1,    23,    -1,    24,    -1,    25,    -1,
      26,    -1,    27,    -1,    28,    -1,    29,    -1,    30,    -1,
      31,    -1,    32,    -1,    33,    -1,    34,    -1,    35,    -1,
      36,    -1,    -1,    53,    72,    59,    73,    60,    -1,    -1,
      73,    74,    -1,    -1,    67,    75,    76,    77,    64,    -1,
      57,    -1,    -1,    63,    78,    -1,    58,    -1,    -1,    55,
      80,    59,    81,    67,    60,    -1,    -1,    81,    82,    -1,
      -1,    -1,    61,    83,    85,    84,    62,    -1,    58,    -1,
      56,    59,    67,    60,    -1,    -1,    -1,    50,    59,    51,
      90,    64,    88,    52,    91,    64,    89,    60,    -1,    58,
      -1,    57,    -1,    -1,    -1,    -1,    -1,    37,    59,    38,
      97,    64,    93,    39,    98,    64,    94,    40,    99,    64,
      95,    41,   100,    64,    96,    60,    -1,    42,    -1,    58,
      -1,    43,    -1,    44,    -1,    45,    -1,    46,    -1,    47,
      -1,    48,    -1,    49,    -1,    -1,    54,    59,    69,    64,
     102,   103,    60,    -1,    -1,   103,   104,    -1,    -1,   106,
     105,   107,    64,    -1,    57,    -1,    58,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   102,   102,   103,   105,   106,   107,   108,   110,   111,
     112,   113,   114,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     146,   147,   148,   149,   150,   151,   152,   156,   155,   164,
     165,   167,   167,   204,   212,   213,   216,   218,   218,   227,
     228,   230,   231,   230,   238,   241,   248,   253,   245,   260,
     262,   267,   274,   283,   290,   264,   314,   315,   317,   318,
     319,   321,   322,   324,   325,   329,   328,   333,   334,   336,
     336,   386,   388
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "H5T_STD_I8BE_TOKEN",
  "H5T_STD_I8LE_TOKEN", "H5T_STD_I16BE_TOKEN", "H5T_STD_I16LE_TOKEN",
  "H5T_STD_I32BE_TOKEN", "H5T_STD_I32LE_TOKEN", "H5T_STD_I64BE_TOKEN",
  "H5T_STD_I64LE_TOKEN", "H5T_STD_U8BE_TOKEN", "H5T_STD_U8LE_TOKEN",
  "H5T_STD_U16BE_TOKEN", "H5T_STD_U16LE_TOKEN", "H5T_STD_U32BE_TOKEN",
  "H5T_STD_U32LE_TOKEN", "H5T_STD_U64BE_TOKEN", "H5T_STD_U64LE_TOKEN",
  "H5T_NATIVE_CHAR_TOKEN", "H5T_NATIVE_SCHAR_TOKEN",
  "H5T_NATIVE_UCHAR_TOKEN", "H5T_NATIVE_SHORT_TOKEN",
  "H5T_NATIVE_USHORT_TOKEN", "H5T_NATIVE_INT_TOKEN",
  "H5T_NATIVE_UINT_TOKEN", "H5T_NATIVE_LONG_TOKEN",
  "H5T_NATIVE_ULONG_TOKEN", "H5T_NATIVE_LLONG_TOKEN",
  "H5T_NATIVE_ULLONG_TOKEN", "H5T_IEEE_F32BE_TOKEN",
  "H5T_IEEE_F32LE_TOKEN", "H5T_IEEE_F64BE_TOKEN", "H5T_IEEE_F64LE_TOKEN",
  "H5T_NATIVE_FLOAT_TOKEN", "H5T_NATIVE_DOUBLE_TOKEN",
  "H5T_NATIVE_LDOUBLE_TOKEN", "H5T_STRING_TOKEN", "STRSIZE_TOKEN",
  "STRPAD_TOKEN", "CSET_TOKEN", "CTYPE_TOKEN", "H5T_VARIABLE_TOKEN",
  "H5T_STR_NULLTERM_TOKEN", "H5T_STR_NULLPAD_TOKEN",
  "H5T_STR_SPACEPAD_TOKEN", "H5T_CSET_ASCII_TOKEN", "H5T_CSET_UTF8_TOKEN",
  "H5T_C_S1_TOKEN", "H5T_FORTRAN_S1_TOKEN", "H5T_OPAQUE_TOKEN",
  "OPQ_SIZE_TOKEN", "OPQ_TAG_TOKEN", "H5T_COMPOUND_TOKEN",
  "H5T_ENUM_TOKEN", "H5T_ARRAY_TOKEN", "H5T_VLEN_TOKEN", "STRING",
  "NUMBER", "'{'", "'}'", "'['", "']'", "':'", "';'", "$accept", "start",
  "ddl_type", "atomic_type", "integer_type", "fp_type", "compound_type",
  "$@1", "memb_list", "memb_def", "$@2", "field_name", "field_offset",
  "offset", "array_type", "$@3", "dim_list", "dim", "$@4", "$@5",
  "dimsize", "vlen_type", "opaque_type", "@6", "$@7", "opaque_size",
  "opaque_tag", "string_type", "$@8", "$@9", "$@10", "@11", "strsize",
  "strpad", "cset", "ctype", "enum_type", "$@12", "enum_list", "enum_def",
  "$@13", "enum_symbol", "enum_val", YY_NULL
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
     305,   306,   307,   308,   309,   310,   311,   312,   313,   123,
     125,    91,    93,    58,    59
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    65,    66,    66,    67,    67,    67,    67,    68,    68,
      68,    68,    68,    69,    69,    69,    69,    69,    69,    69,
      69,    69,    69,    69,    69,    69,    69,    69,    69,    69,
      69,    69,    69,    69,    69,    69,    69,    69,    69,    69,
      70,    70,    70,    70,    70,    70,    70,    72,    71,    73,
      73,    75,    74,    76,    77,    77,    78,    80,    79,    81,
      81,    83,    84,    82,    85,    86,    88,    89,    87,    90,
      91,    93,    94,    95,    96,    92,    97,    97,    98,    98,
      98,    99,    99,   100,   100,   102,   101,   103,   103,   105,
     104,   106,   107
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     0,     5,     0,
       2,     0,     5,     1,     0,     2,     1,     0,     6,     0,
       2,     0,     0,     5,     1,     4,     0,     0,    11,     1,
       1,     0,     0,     0,     0,    19,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     0,     7,     0,     2,     0,
       4,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,     0,     0,    47,     0,    57,
       0,     0,     3,     4,     8,     9,     5,     6,     7,    12,
      10,    11,     0,     0,     0,     0,     0,     0,     1,     0,
       0,    49,     0,    59,     0,    76,    77,     0,    69,     0,
       0,    85,     0,    65,    71,    66,    48,    51,    50,    87,
      61,     0,    60,     0,     0,     0,     0,     0,    58,     0,
       0,    53,    54,    91,    86,    88,    89,    64,    62,    78,
      79,    80,     0,    70,     0,     0,     0,     0,     0,    72,
      67,    56,    55,    52,    92,     0,    63,     0,     0,    90,
       0,    68,    81,    82,     0,    73,     0,     0,    83,    84,
       0,    74,     0,    75
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    41,    42,    43,    44,    45,    46,    54,    70,    78,
      85,    92,   106,   112,    47,    56,    72,    82,    87,   108,
      98,    48,    49,    84,   118,    69,   104,    50,    83,   117,
     126,   132,    67,   102,   124,   130,    51,    79,    86,    95,
     107,    96,   115
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -25
static const yytype_int16 yypact[] =
{
     114,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,
     -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,
     -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,
     -25,   -25,   -25,   -25,   -25,   -24,   -22,   -25,   -13,   -25,
     -11,    49,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,
     -25,   -25,    18,    45,    38,   168,    39,   114,   -25,    -4,
      41,   -25,    36,   -25,    42,   -25,   -25,    37,   -25,    40,
      56,   -25,    -3,   -25,   -25,   -25,   -25,   -25,   -25,   -25,
     -25,    43,   -25,    66,    55,    51,   -21,    57,   -25,     0,
      95,   -25,    50,   -25,   -25,   -25,   -25,   -25,   -25,   -25,
     -25,   -25,    89,   -25,    90,    97,    92,    99,    52,   -25,
     -25,   -25,   -25,   -25,   -25,    94,   -25,   119,   100,   -25,
      -6,   -25,   -25,   -25,    98,   -25,   120,    46,   -25,   -25,
     101,   -25,   103,   -25
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -25,   -25,   -15,   -25,   111,   -25,   -25,   -25,   -25,   -25,
     -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,
     -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,
     -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,
     -25,   -25,   -25
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
       1,     2,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    52,    93,    53,    65,    94,
     122,   123,    64,    99,   100,   101,    55,    36,    57,    58,
      37,    38,    39,    40,    66,    77,    59,    81,    80,     1,
       2,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,   128,   129,    60,    61,    63,    68,
      71,    74,    73,    88,    75,    89,    36,    90,    91,    37,
      38,    39,    40,   105,   116,    97,    76,     1,     2,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,   103,   109,   110,   111,   113,   114,   119,   120,
     121,   127,   125,   133,    36,   131,    62,    37,    38,    39,
      40,     1,     2,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27
};

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-25)))

#define yytable_value_is_error(Yytable_value) \
  YYID (0)

static const yytype_uint8 yycheck[] =
{
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    59,    57,    59,    42,    60,
      46,    47,    57,    43,    44,    45,    59,    50,    59,     0,
      53,    54,    55,    56,    58,    70,    38,    72,    61,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    48,    49,    51,    59,    59,    58,
      64,    64,    60,    60,    64,    39,    50,    52,    57,    53,
      54,    55,    56,    63,    62,    58,    60,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    57,    64,    64,    58,    64,    58,    64,    40,
      60,    41,    64,    60,    50,    64,    55,    53,    54,    55,
      56,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    50,    53,    54,    55,
      56,    66,    67,    68,    69,    70,    71,    79,    86,    87,
      92,   101,    59,    59,    72,    59,    80,    59,     0,    38,
      51,    59,    69,    59,    67,    42,    58,    97,    58,    90,
      73,    64,    81,    60,    64,    64,    60,    67,    74,   102,
      61,    67,    82,    93,    88,    75,   103,    83,    60,    39,
      52,    57,    76,    57,    60,   104,   106,    58,    85,    43,
      44,    45,    98,    57,    91,    63,    77,   105,    84,    64,
      64,    58,    78,    64,    58,   107,    62,    94,    89,    64,
      40,    60,    46,    47,    99,    64,    95,    41,    48,    49,
     100,    64,    96,    60
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
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))

/* Error token number */
#define YYTERROR	1
#define YYERRCODE	256


/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
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
  FILE *yyo = yyoutput;
  YYUSE (yyo);
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

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULL, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULL;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULL, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
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




/* The lookahead symbol.  */
int yychar;


#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval YY_INITIAL_VALUE(yyval_default);

/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
hid_t
yyparse (void *YYPARSE_PARAM)
#else
hid_t
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
hid_t
yyparse (void)
#else
hid_t
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

       Refer to the stacks through separate pointers, to allow yyoverflow
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
  int yytoken = 0;
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

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
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
  if (yypact_value_is_default (yyn))
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
      if (yytable_value_is_error (yyn))
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
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
/* Line 1792 of yacc.c  */
#line 102 "hl/src//H5LTparse.y"
    { memset(arr_stack, 0, STACK_SIZE*sizeof(struct arr_info)); /*initialize here?*/ }
    break;

  case 3:
/* Line 1792 of yacc.c  */
#line 103 "hl/src//H5LTparse.y"
    { return (yyval.hid);}
    break;

  case 13:
/* Line 1792 of yacc.c  */
#line 117 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_STD_I8BE); }
    break;

  case 14:
/* Line 1792 of yacc.c  */
#line 118 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_STD_I8LE); }
    break;

  case 15:
/* Line 1792 of yacc.c  */
#line 119 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_STD_I16BE); }
    break;

  case 16:
/* Line 1792 of yacc.c  */
#line 120 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_STD_I16LE); }
    break;

  case 17:
/* Line 1792 of yacc.c  */
#line 121 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_STD_I32BE); }
    break;

  case 18:
/* Line 1792 of yacc.c  */
#line 122 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_STD_I32LE); }
    break;

  case 19:
/* Line 1792 of yacc.c  */
#line 123 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_STD_I64BE); }
    break;

  case 20:
/* Line 1792 of yacc.c  */
#line 124 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_STD_I64LE); }
    break;

  case 21:
/* Line 1792 of yacc.c  */
#line 125 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_STD_U8BE); }
    break;

  case 22:
/* Line 1792 of yacc.c  */
#line 126 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_STD_U8LE); }
    break;

  case 23:
/* Line 1792 of yacc.c  */
#line 127 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_STD_U16BE); }
    break;

  case 24:
/* Line 1792 of yacc.c  */
#line 128 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_STD_U16LE); }
    break;

  case 25:
/* Line 1792 of yacc.c  */
#line 129 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_STD_U32BE); }
    break;

  case 26:
/* Line 1792 of yacc.c  */
#line 130 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_STD_U32LE); }
    break;

  case 27:
/* Line 1792 of yacc.c  */
#line 131 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_STD_U64BE); }
    break;

  case 28:
/* Line 1792 of yacc.c  */
#line 132 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_STD_U64LE); }
    break;

  case 29:
/* Line 1792 of yacc.c  */
#line 133 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_CHAR); }
    break;

  case 30:
/* Line 1792 of yacc.c  */
#line 134 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_SCHAR); }
    break;

  case 31:
/* Line 1792 of yacc.c  */
#line 135 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_UCHAR); }
    break;

  case 32:
/* Line 1792 of yacc.c  */
#line 136 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_SHORT); }
    break;

  case 33:
/* Line 1792 of yacc.c  */
#line 137 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_USHORT); }
    break;

  case 34:
/* Line 1792 of yacc.c  */
#line 138 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_INT); }
    break;

  case 35:
/* Line 1792 of yacc.c  */
#line 139 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_UINT); }
    break;

  case 36:
/* Line 1792 of yacc.c  */
#line 140 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_LONG); }
    break;

  case 37:
/* Line 1792 of yacc.c  */
#line 141 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_ULONG); }
    break;

  case 38:
/* Line 1792 of yacc.c  */
#line 142 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_LLONG); }
    break;

  case 39:
/* Line 1792 of yacc.c  */
#line 143 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_ULLONG); }
    break;

  case 40:
/* Line 1792 of yacc.c  */
#line 146 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_IEEE_F32BE); }
    break;

  case 41:
/* Line 1792 of yacc.c  */
#line 147 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_IEEE_F32LE); }
    break;

  case 42:
/* Line 1792 of yacc.c  */
#line 148 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_IEEE_F64BE); }
    break;

  case 43:
/* Line 1792 of yacc.c  */
#line 149 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_IEEE_F64LE); }
    break;

  case 44:
/* Line 1792 of yacc.c  */
#line 150 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_FLOAT); }
    break;

  case 45:
/* Line 1792 of yacc.c  */
#line 151 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_DOUBLE); }
    break;

  case 46:
/* Line 1792 of yacc.c  */
#line 152 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_LDOUBLE); }
    break;

  case 47:
/* Line 1792 of yacc.c  */
#line 156 "hl/src//H5LTparse.y"
    { csindex++; cmpd_stack[csindex].id = H5Tcreate(H5T_COMPOUND, 1); /*temporarily set size to 1*/ }
    break;

  case 48:
/* Line 1792 of yacc.c  */
#line 158 "hl/src//H5LTparse.y"
    { (yyval.hid) = cmpd_stack[csindex].id; 
                              cmpd_stack[csindex].id = 0;
                              cmpd_stack[csindex].first_memb = 1; 
                              csindex--;
                            }
    break;

  case 51:
/* Line 1792 of yacc.c  */
#line 167 "hl/src//H5LTparse.y"
    { cmpd_stack[csindex].is_field = 1; /*notify lexer a compound member is parsed*/ }
    break;

  case 52:
/* Line 1792 of yacc.c  */
#line 169 "hl/src//H5LTparse.y"
    {   
                            size_t origin_size, new_size;
                            hid_t dtype_id = cmpd_stack[csindex].id;

                            /*Adjust size and insert member, consider both member size and offset.*/
                            if(cmpd_stack[csindex].first_memb) { /*reclaim the size 1 temporarily set*/
                                new_size = H5Tget_size((yyvsp[(1) - (5)].hid)) + (yyvsp[(4) - (5)].ival);
                                H5Tset_size(dtype_id, new_size);
                                /*member name is saved in yylval.sval by lexer*/
                                H5Tinsert(dtype_id, (yyvsp[(3) - (5)].sval), (yyvsp[(4) - (5)].ival), (yyvsp[(1) - (5)].hid));

                                cmpd_stack[csindex].first_memb = 0;
                            } else {
                                origin_size = H5Tget_size(dtype_id);
                                
                                if((yyvsp[(4) - (5)].ival) == 0) {
                                    new_size = origin_size + H5Tget_size((yyvsp[(1) - (5)].hid));
                                    H5Tset_size(dtype_id, new_size);
                                    H5Tinsert(dtype_id, (yyvsp[(3) - (5)].sval), origin_size, (yyvsp[(1) - (5)].hid));
                                } else {
                                    new_size = (yyvsp[(4) - (5)].ival) + H5Tget_size((yyvsp[(1) - (5)].hid));
                                    H5Tset_size(dtype_id, new_size);
                                    H5Tinsert(dtype_id, (yyvsp[(3) - (5)].sval), (yyvsp[(4) - (5)].ival), (yyvsp[(1) - (5)].hid));
                                }
                            }
                            if((yyvsp[(3) - (5)].sval)) {
                                HDfree((yyvsp[(3) - (5)].sval));
                                (yyvsp[(3) - (5)].sval) = NULL;
                            }
                            cmpd_stack[csindex].is_field = 0;
                            H5Tclose((yyvsp[(1) - (5)].hid));
                             
                            new_size = H5Tget_size(dtype_id);
                        }
    break;

  case 53:
/* Line 1792 of yacc.c  */
#line 205 "hl/src//H5LTparse.y"
    {
                            (yyval.sval) = HDstrdup(yylval.sval);
                            HDfree(yylval.sval);
                            yylval.sval = NULL;
                        }
    break;

  case 54:
/* Line 1792 of yacc.c  */
#line 212 "hl/src//H5LTparse.y"
    { (yyval.ival) = 0; }
    break;

  case 55:
/* Line 1792 of yacc.c  */
#line 214 "hl/src//H5LTparse.y"
    { (yyval.ival) = yylval.ival; }
    break;

  case 57:
/* Line 1792 of yacc.c  */
#line 218 "hl/src//H5LTparse.y"
    { asindex++; /*pushd onto the stack*/ }
    break;

  case 58:
/* Line 1792 of yacc.c  */
#line 220 "hl/src//H5LTparse.y"
    { 
                          (yyval.hid) = H5Tarray_create2((yyvsp[(5) - (6)].hid), arr_stack[asindex].ndims, arr_stack[asindex].dims);
                          arr_stack[asindex].ndims = 0;
                          asindex--;
                          H5Tclose((yyvsp[(5) - (6)].hid));
                        }
    break;

  case 61:
/* Line 1792 of yacc.c  */
#line 230 "hl/src//H5LTparse.y"
    { arr_stack[asindex].is_dim = 1; /*notice lexer of dimension size*/ }
    break;

  case 62:
/* Line 1792 of yacc.c  */
#line 231 "hl/src//H5LTparse.y"
    { unsigned ndims = arr_stack[asindex].ndims;
                                  arr_stack[asindex].dims[ndims] = (hsize_t)yylval.ival; 
                                  arr_stack[asindex].ndims++;
                                  arr_stack[asindex].is_dim = 0; 
                                }
    break;

  case 65:
/* Line 1792 of yacc.c  */
#line 242 "hl/src//H5LTparse.y"
    { (yyval.hid) = H5Tvlen_create((yyvsp[(3) - (4)].hid)); H5Tclose((yyvsp[(3) - (4)].hid)); }
    break;

  case 66:
/* Line 1792 of yacc.c  */
#line 248 "hl/src//H5LTparse.y"
    {   
                                size_t size = (size_t)yylval.ival;
                                (yyval.hid) = H5Tcreate(H5T_OPAQUE, size);
                            }
    break;

  case 67:
/* Line 1792 of yacc.c  */
#line 253 "hl/src//H5LTparse.y"
    {  
                                H5Tset_tag((yyvsp[(6) - (9)].hid), yylval.sval);
                                HDfree(yylval.sval);
                                yylval.sval = NULL;
                            }
    break;

  case 68:
/* Line 1792 of yacc.c  */
#line 258 "hl/src//H5LTparse.y"
    { (yyval.hid) = (yyvsp[(6) - (11)].hid); }
    break;

  case 71:
/* Line 1792 of yacc.c  */
#line 267 "hl/src//H5LTparse.y"
    {  
                                if((yyvsp[(4) - (5)].ival) == H5T_VARIABLE_TOKEN)
                                    is_variable = 1;
                                else 
                                    str_size = yylval.ival;
                            }
    break;

  case 72:
/* Line 1792 of yacc.c  */
#line 274 "hl/src//H5LTparse.y"
    {
                                if((yyvsp[(8) - (9)].ival) == H5T_STR_NULLTERM_TOKEN)
                                    str_pad = H5T_STR_NULLTERM;
                                else if((yyvsp[(8) - (9)].ival) == H5T_STR_NULLPAD_TOKEN)
                                    str_pad = H5T_STR_NULLPAD;
                                else if((yyvsp[(8) - (9)].ival) == H5T_STR_SPACEPAD_TOKEN)
                                    str_pad = H5T_STR_SPACEPAD;
                            }
    break;

  case 73:
/* Line 1792 of yacc.c  */
#line 283 "hl/src//H5LTparse.y"
    {  
                                if((yyvsp[(12) - (13)].ival) == H5T_CSET_ASCII_TOKEN)
                                    str_cset = H5T_CSET_ASCII;
                                else if((yyvsp[(12) - (13)].ival) == H5T_CSET_UTF8_TOKEN)
                                    str_cset = H5T_CSET_UTF8;
                            }
    break;

  case 74:
/* Line 1792 of yacc.c  */
#line 290 "hl/src//H5LTparse.y"
    {
                                if((yyvsp[(16) - (17)].hid) == H5T_C_S1_TOKEN)
                                    (yyval.hid) = H5Tcopy(H5T_C_S1);
                                else if((yyvsp[(16) - (17)].hid) == H5T_FORTRAN_S1_TOKEN)
                                    (yyval.hid) = H5Tcopy(H5T_FORTRAN_S1);
                            }
    break;

  case 75:
/* Line 1792 of yacc.c  */
#line 297 "hl/src//H5LTparse.y"
    {   
                                hid_t str_id = (yyvsp[(18) - (19)].hid);

                                /*set string size*/
                                if(is_variable) {
                                    H5Tset_size(str_id, H5T_VARIABLE);
                                    is_variable = 0;
                                } else
                                    H5Tset_size(str_id, str_size);
                                
                                /*set string padding and character set*/
                                H5Tset_strpad(str_id, str_pad);
                                H5Tset_cset(str_id, str_cset);

                                (yyval.hid) = str_id; 
                            }
    break;

  case 76:
/* Line 1792 of yacc.c  */
#line 314 "hl/src//H5LTparse.y"
    {(yyval.ival) = H5T_VARIABLE_TOKEN;}
    break;

  case 78:
/* Line 1792 of yacc.c  */
#line 317 "hl/src//H5LTparse.y"
    {(yyval.ival) = H5T_STR_NULLTERM_TOKEN;}
    break;

  case 79:
/* Line 1792 of yacc.c  */
#line 318 "hl/src//H5LTparse.y"
    {(yyval.ival) = H5T_STR_NULLPAD_TOKEN;}
    break;

  case 80:
/* Line 1792 of yacc.c  */
#line 319 "hl/src//H5LTparse.y"
    {(yyval.ival) = H5T_STR_SPACEPAD_TOKEN;}
    break;

  case 81:
/* Line 1792 of yacc.c  */
#line 321 "hl/src//H5LTparse.y"
    {(yyval.ival) = H5T_CSET_ASCII_TOKEN;}
    break;

  case 82:
/* Line 1792 of yacc.c  */
#line 322 "hl/src//H5LTparse.y"
    {(yyval.ival) = H5T_CSET_UTF8_TOKEN;}
    break;

  case 83:
/* Line 1792 of yacc.c  */
#line 324 "hl/src//H5LTparse.y"
    {(yyval.hid) = H5T_C_S1_TOKEN;}
    break;

  case 84:
/* Line 1792 of yacc.c  */
#line 325 "hl/src//H5LTparse.y"
    {(yyval.hid) = H5T_FORTRAN_S1_TOKEN;}
    break;

  case 85:
/* Line 1792 of yacc.c  */
#line 329 "hl/src//H5LTparse.y"
    { is_enum = 1; enum_id = H5Tenum_create((yyvsp[(3) - (4)].hid)); H5Tclose((yyvsp[(3) - (4)].hid)); }
    break;

  case 86:
/* Line 1792 of yacc.c  */
#line 331 "hl/src//H5LTparse.y"
    { is_enum = 0; /*reset*/ (yyval.hid) = enum_id; }
    break;

  case 89:
/* Line 1792 of yacc.c  */
#line 336 "hl/src//H5LTparse.y"
    {
                                                is_enum_memb = 1; /*indicate member of enum*/
                                                enum_memb_symbol = strdup(yylval.sval); 
                                                HDfree(yylval.sval);
                                                yylval.sval = NULL;
                                            }
    break;

  case 90:
/* Line 1792 of yacc.c  */
#line 343 "hl/src//H5LTparse.y"
    {
                                char char_val=(char)yylval.ival;
                                short short_val=(short)yylval.ival;
                                int int_val=(int)yylval.ival;
                                long long_val=(long)yylval.ival;
                                long long llong_val=(long long)yylval.ival;
                                hid_t super = H5Tget_super(enum_id);
                                hid_t native = H5Tget_native_type(super, H5T_DIR_ASCEND);
                                H5T_order_t super_order = H5Tget_order(super);
                                H5T_order_t native_order = H5Tget_order(native);
 
                                if(is_enum && is_enum_memb) { /*if it's an enum member*/
                                    /*To handle machines of different endianness*/
                                    if(H5Tequal(native, H5T_NATIVE_SCHAR) || H5Tequal(native, H5T_NATIVE_UCHAR)) {
                                        if(super_order != native_order)
                                            H5Tconvert(native, super, 1, &char_val, NULL, H5P_DEFAULT); 
                                        H5Tenum_insert(enum_id, enum_memb_symbol, &char_val);
                                    } else if(H5Tequal(native, H5T_NATIVE_SHORT) || H5Tequal(native, H5T_NATIVE_USHORT)) {
                                        if(super_order != native_order)
                                            H5Tconvert(native, super, 1, &short_val, NULL, H5P_DEFAULT); 
                                        H5Tenum_insert(enum_id, enum_memb_symbol, &short_val);
                                    } else if(H5Tequal(native, H5T_NATIVE_INT) || H5Tequal(native, H5T_NATIVE_UINT)) {
                                        if(super_order != native_order)
                                            H5Tconvert(native, super, 1, &int_val, NULL, H5P_DEFAULT); 
                                        H5Tenum_insert(enum_id, enum_memb_symbol, &int_val);
                                    } else if(H5Tequal(native, H5T_NATIVE_LONG) || H5Tequal(native, H5T_NATIVE_ULONG)) {
                                        if(super_order != native_order)
                                            H5Tconvert(native, super, 1, &long_val, NULL, H5P_DEFAULT); 
                                        H5Tenum_insert(enum_id, enum_memb_symbol, &long_val);
                                    } else if(H5Tequal(native, H5T_NATIVE_LLONG) || H5Tequal(native, H5T_NATIVE_ULLONG)) {
                                        if(super_order != native_order)
                                            H5Tconvert(native, super, 1, &llong_val, NULL, H5P_DEFAULT); 
                                        H5Tenum_insert(enum_id, enum_memb_symbol, &llong_val);
                                    }

                                    is_enum_memb = 0; 
                                    if(enum_memb_symbol) HDfree(enum_memb_symbol);
                                }

                                H5Tclose(super);
                                H5Tclose(native);
                            }
    break;


/* Line 1792 of yacc.c  */
#line 2166 "hl/src//H5LTparse.c"
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
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
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
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
      if (!yypact_value_is_default (yyn))
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

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


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

#if !defined yyoverflow || YYERROR_VERBOSE
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
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
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


