#if __GNUC__ >= 4 && __GNUC_MINOR__ >=2                           
#pragma GCC diagnostic ignored "-Wconversion"                     
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"  
#pragma GCC diagnostic ignored "-Wlarger-than="                   
#pragma GCC diagnostic ignored "-Wmissing-prototypes"             
#pragma GCC diagnostic ignored "-Wnested-externs"                 
#pragma GCC diagnostic ignored "-Wold-style-definition"           
#pragma GCC diagnostic ignored "-Wredundant-decls"                
#pragma GCC diagnostic ignored "-Wsign-compare"                   
#pragma GCC diagnostic ignored "-Wsign-conversion"                
#pragma GCC diagnostic ignored "-Wstrict-overflow"                
#pragma GCC diagnostic ignored "-Wstrict-prototypes"              
#pragma GCC diagnostic ignored "-Wsuggest-attribute=pure"         
#pragma GCC diagnostic ignored "-Wswitch-default"                 
#pragma GCC diagnostic ignored "-Wunused-function"                
#pragma GCC diagnostic ignored "-Wunused-macros"                  
#pragma GCC diagnostic ignored "-Wunused-parameter"               
#elif defined __SUNPRO_CC                                         
#pragma disable_warn                                              
#elif defined _MSC_VER                                            
#pragma warning(push, 1)                                          
#endif                                                            
/* A Bison parser, made by GNU Bison 3.0.2.  */

/* XXX(kitware): Mangle all HDF5 HL symbols */
#include "vtk_hdf5_hl_mangle.h"

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

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
#define YYBISON_VERSION "3.0.2"

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
#define yydebug         H5LTyydebug
#define yynerrs         H5LTyynerrs

#define yylval          H5LTyylval
#define yychar          H5LTyychar

/* Copy the first part of user declarations.  */
#line 22 "hl/src/H5LTparse.y" /* yacc.c:339  */

#include <stdio.h>
#include <string.h>
#include <hdf5.h>

extern int yylex();
extern int yyerror(const char *);

#define STACK_SIZE      16

/*structure for compound type information*/
struct cmpd_info {
    hid_t       id;             /*type ID*/
    hbool_t     is_field;       /*flag to lexer for compound member*/
    hbool_t     first_memb;     /*flag for first compound member*/
};

/*stack for nested compound type*/
struct cmpd_info cmpd_stack[STACK_SIZE] = {
    {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1},
    {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1},
    {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1},
    {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1} };

int csindex = -1;                /*pointer to the top of compound stack*/

/*structure for array type information*/
struct arr_info {
    hsize_t             dims[H5S_MAX_RANK];     /*size of each dimension, limited to 32 dimensions*/
    unsigned            ndims;                  /*number of dimensions*/
    hbool_t             is_dim;                 /*flag to lexer for dimension*/
};
/*stack for nested array type*/
struct arr_info arr_stack[STACK_SIZE];
int asindex = -1;               /*pointer to the top of array stack*/ 

hbool_t     is_str_size = 0;        /*flag to lexer for string size*/
hbool_t     is_str_pad = 0;         /*flag to lexer for string padding*/
H5T_str_t   str_pad;                /*variable for string padding*/
H5T_cset_t  str_cset;               /*variable for string character set*/
hbool_t     is_variable = 0;        /*variable for variable-length string*/
size_t      str_size;               /*variable for string size*/
   
hid_t       enum_id;                /*type ID*/
hbool_t     is_enum = 0;            /*flag to lexer for enum type*/
hbool_t     is_enum_memb = 0;       /*flag to lexer for enum member*/
char*       enum_memb_symbol;       /*enum member symbol string*/

hbool_t is_opq_size = 0;            /*flag to lexer for opaque type size*/
hbool_t is_opq_tag = 0;             /*flag to lexer for opaque type tag*/


#line 127 "hl/src/H5LTparse.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
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
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int H5LTyydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
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

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 74 "hl/src/H5LTparse.y" /* yacc.c:355  */

    int     ival;         /*for integer token*/
    char    *sval;        /*for name string*/
    hid_t   hid;          /*for hid_t token*/

#line 232 "hl/src/H5LTparse.c" /* yacc.c:355  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE H5LTyylval;

hid_t H5LTyyparse (void);

#endif /* !YY_H5LTYY_HL_SRC_H5LTPARSE_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 247 "hl/src/H5LTparse.c" /* yacc.c:358  */

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
#else
typedef signed char yytype_int8;
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
# elif ! defined YYSIZE_T
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

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
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
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
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
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
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
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

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
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  58
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   203

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  66
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  46
/* YYNRULES -- Number of rules.  */
#define YYNRULES  95
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  143

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   313

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    63,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    64,    65,
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
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   107,   107,   108,   110,   111,   112,   113,   115,   116,
     117,   118,   119,   122,   123,   124,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
     151,   152,   153,   154,   155,   156,   157,   161,   160,   169,
     170,   172,   172,   209,   217,   218,   221,   223,   223,   232,
     233,   235,   236,   235,   243,   246,   252,   253,   258,   259,
     250,   267,   269,   273,   274,   282,   291,   298,   271,   322,
     323,   325,   326,   327,   329,   330,   332,   333,   337,   336,
     341,   342,   344,   344,   398,   400
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
  "NUMBER", "'{'", "'}'", "'['", "']'", "'\"'", "':'", "';'", "$accept",
  "start", "ddl_type", "atomic_type", "integer_type", "fp_type",
  "compound_type", "$@1", "memb_list", "memb_def", "$@2", "field_name",
  "field_offset", "offset", "array_type", "$@3", "dim_list", "dim", "$@4",
  "$@5", "dimsize", "vlen_type", "opaque_type", "$@6", "@7", "$@8", "$@9",
  "opaque_size", "opaque_tag", "string_type", "$@10", "$@11", "$@12",
  "$@13", "@14", "strsize", "strpad", "cset", "ctype", "enum_type", "$@15",
  "enum_list", "enum_def", "$@16", "enum_symbol", "enum_val", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   123,
     125,    91,    93,    34,    58,    59
};
# endif

#define YYPACT_NINF -25

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-25)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     114,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,
     -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,
     -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,
     -25,   -25,   -25,   -25,   -25,   -24,   -20,   -25,   -15,   -25,
     -14,    49,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,
     -25,   -25,    19,    45,    38,   168,    39,   114,   -25,   -25,
     -25,   -25,    34,   -25,    40,    -4,    43,    56,   -25,    -3,
     -25,   -25,   -25,    37,   -25,    42,   -25,   -25,   -25,   -25,
     -25,    44,   -25,   -25,   -25,    50,   -23,    47,   -25,    64,
      62,    51,   -25,    58,   -25,   -25,   -25,    -2,   -25,   -25,
      89,   -25,    90,    92,   -25,   -25,   -25,    91,    94,    95,
     -25,   -25,   -25,    98,   100,    96,   102,   122,   -25,   103,
     -25,   -25,   -25,   -25,   133,     9,   134,   -25,   -25,   -25,
     135,   -25,   -25,   105,   160,   -25,    46,   -25,   -25,   137,
     -25,   143,   -25
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,     0,     0,    47,     0,    57,
       0,     0,     3,     4,     8,     9,     5,     6,     7,    12,
      10,    11,     0,     0,     0,     0,     0,     0,     1,    73,
      66,    49,     0,    59,     0,     0,     0,     0,    88,     0,
      65,    79,    80,     0,    71,     0,    48,    51,    50,    90,
      61,     0,    60,    74,    67,     0,     0,     0,    58,     0,
       0,     0,    89,     0,    91,    64,    62,     0,    68,    53,
       0,    94,     0,     0,    81,    82,    83,     0,     0,    54,
      92,    63,    75,     0,     0,     0,     0,     0,    72,     0,
      56,    55,    52,    95,     0,     0,     0,    93,    84,    85,
       0,    69,    76,     0,     0,    70,     0,    86,    87,     0,
      77,     0,    78
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -25,   -25,   -21,   -25,   108,   -25,   -25,   -25,   -25,   -25,
     -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,
     -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,
     -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,   -25,
     -25,   -25,   -25,   -25,   -25,   -25
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    41,    42,    43,    44,    45,    46,    54,    67,    78,
      85,   100,   115,   121,    47,    56,    69,    82,    87,   103,
      96,    48,    49,    66,    90,   108,   133,    75,   119,    50,
      65,    89,   117,   134,   141,    73,   107,   130,   139,    51,
      79,    86,    94,   116,   102,   124
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
       1,     2,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    52,    64,    92,    71,    53,
      93,   104,   105,   106,    55,    57,    77,    36,    81,    58,
      37,    38,    39,    40,    72,   128,   129,    59,    80,     1,
       2,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,   137,   138,    60,    61,    63,    68,
      70,    74,    83,    97,    88,    95,    36,    84,    99,    37,
      38,    39,    40,    91,    98,   101,    76,     1,     2,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,   109,   110,   111,   118,   112,   113,   120,   114,
     123,   122,   125,    62,    36,   135,   126,    37,    38,    39,
      40,     1,     2,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,   127,   131,
     132,   136,   140,   142
};

static const yytype_uint8 yycheck[] =
{
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    59,    57,    60,    42,    59,
      63,    43,    44,    45,    59,    59,    67,    50,    69,     0,
      53,    54,    55,    56,    58,    46,    47,    38,    61,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    48,    49,    51,    59,    59,    65,
      60,    58,    65,    39,    60,    58,    50,    65,    57,    53,
      54,    55,    56,    63,    52,    57,    60,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    63,    63,    62,    57,    65,    63,    58,    64,
      58,    65,    40,    55,    50,    60,    63,    53,    54,    55,
      56,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    65,    65,
      65,    41,    65,    60
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    50,    53,    54,    55,
      56,    67,    68,    69,    70,    71,    72,    80,    87,    88,
      95,   105,    59,    59,    73,    59,    81,    59,     0,    38,
      51,    59,    70,    59,    68,    96,    89,    74,    65,    82,
      60,    42,    58,   101,    58,    93,    60,    68,    75,   106,
      61,    68,    83,    65,    65,    76,   107,    84,    60,    97,
      90,    63,    60,    63,   108,    58,    86,    39,    52,    57,
      77,    57,   110,    85,    43,    44,    45,   102,    91,    63,
      63,    62,    65,    63,    64,    78,   109,    98,    57,    94,
      58,    79,    65,    58,   111,    40,    63,    65,    46,    47,
     103,    65,    65,    92,    99,    60,    41,    48,    49,   104,
      65,   100,    60
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    66,    67,    67,    68,    68,    68,    68,    69,    69,
      69,    69,    69,    70,    70,    70,    70,    70,    70,    70,
      70,    70,    70,    70,    70,    70,    70,    70,    70,    70,
      70,    70,    70,    70,    70,    70,    70,    70,    70,    70,
      71,    71,    71,    71,    71,    71,    71,    73,    72,    74,
      74,    76,    75,    77,    78,    78,    79,    81,    80,    82,
      82,    84,    85,    83,    86,    87,    89,    90,    91,    92,
      88,    93,    94,    96,    97,    98,    99,   100,    95,   101,
     101,   102,   102,   102,   103,   103,   104,   104,   106,   105,
     107,   107,   109,   108,   110,   111
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     0,     5,     0,
       2,     0,     7,     1,     0,     2,     1,     0,     6,     0,
       2,     0,     0,     5,     1,     4,     0,     0,     0,     0,
      15,     1,     1,     0,     0,     0,     0,     0,    20,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     0,     7,
       0,     2,     0,     6,     1,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


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
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



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

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
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

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
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
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
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
static char *
yystpcpy (char *yydest, const char *yysrc)
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
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
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
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
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

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

hid_t
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

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
      yychar = yylex ();
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
     '$$ = $1'.

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
#line 107 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { memset(arr_stack, 0, STACK_SIZE*sizeof(struct arr_info)); /*initialize here?*/ }
#line 1468 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 3:
#line 108 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { return (yyval.hid);}
#line 1474 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 13:
#line 122 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_STD_I8BE); }
#line 1480 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 14:
#line 123 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_STD_I8LE); }
#line 1486 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 15:
#line 124 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_STD_I16BE); }
#line 1492 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 16:
#line 125 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_STD_I16LE); }
#line 1498 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 17:
#line 126 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_STD_I32BE); }
#line 1504 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 18:
#line 127 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_STD_I32LE); }
#line 1510 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 19:
#line 128 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_STD_I64BE); }
#line 1516 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 20:
#line 129 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_STD_I64LE); }
#line 1522 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 21:
#line 130 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_STD_U8BE); }
#line 1528 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 22:
#line 131 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_STD_U8LE); }
#line 1534 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 23:
#line 132 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_STD_U16BE); }
#line 1540 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 24:
#line 133 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_STD_U16LE); }
#line 1546 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 25:
#line 134 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_STD_U32BE); }
#line 1552 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 26:
#line 135 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_STD_U32LE); }
#line 1558 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 27:
#line 136 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_STD_U64BE); }
#line 1564 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 28:
#line 137 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_STD_U64LE); }
#line 1570 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 29:
#line 138 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_CHAR); }
#line 1576 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 30:
#line 139 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_SCHAR); }
#line 1582 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 31:
#line 140 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_UCHAR); }
#line 1588 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 32:
#line 141 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_SHORT); }
#line 1594 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 33:
#line 142 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_USHORT); }
#line 1600 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 34:
#line 143 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_INT); }
#line 1606 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 35:
#line 144 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_UINT); }
#line 1612 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 36:
#line 145 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_LONG); }
#line 1618 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 37:
#line 146 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_ULONG); }
#line 1624 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 38:
#line 147 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_LLONG); }
#line 1630 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 39:
#line 148 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_ULLONG); }
#line 1636 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 40:
#line 151 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_IEEE_F32BE); }
#line 1642 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 41:
#line 152 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_IEEE_F32LE); }
#line 1648 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 42:
#line 153 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_IEEE_F64BE); }
#line 1654 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 43:
#line 154 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_IEEE_F64LE); }
#line 1660 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 44:
#line 155 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_FLOAT); }
#line 1666 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 45:
#line 156 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_DOUBLE); }
#line 1672 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 46:
#line 157 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tcopy(H5T_NATIVE_LDOUBLE); }
#line 1678 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 47:
#line 161 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { csindex++; cmpd_stack[csindex].id = H5Tcreate(H5T_COMPOUND, 1); /*temporarily set size to 1*/ }
#line 1684 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 48:
#line 163 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = cmpd_stack[csindex].id; 
                              cmpd_stack[csindex].id = 0;
                              cmpd_stack[csindex].first_memb = 1; 
                              csindex--;
                            }
#line 1694 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 51:
#line 172 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { cmpd_stack[csindex].is_field = 1; /*notify lexer a compound member is parsed*/ }
#line 1700 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 52:
#line 174 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    {   
                            size_t origin_size, new_size;
                            hid_t dtype_id = cmpd_stack[csindex].id;

                            /*Adjust size and insert member, consider both member size and offset.*/
                            if(cmpd_stack[csindex].first_memb) { /*reclaim the size 1 temporarily set*/
                                new_size = H5Tget_size((yyvsp[-6].hid)) + (yyvsp[-1].ival);
                                H5Tset_size(dtype_id, new_size);
                                /*member name is saved in yylval.sval by lexer*/
                                H5Tinsert(dtype_id, (yyvsp[-3].sval), (yyvsp[-1].ival), (yyvsp[-6].hid));

                                cmpd_stack[csindex].first_memb = 0;
                            } else {
                                origin_size = H5Tget_size(dtype_id);
                                
                                if((yyvsp[-1].ival) == 0) {
                                    new_size = origin_size + H5Tget_size((yyvsp[-6].hid));
                                    H5Tset_size(dtype_id, new_size);
                                    H5Tinsert(dtype_id, (yyvsp[-3].sval), origin_size, (yyvsp[-6].hid));
                                } else {
                                    new_size = (yyvsp[-1].ival) + H5Tget_size((yyvsp[-6].hid));
                                    H5Tset_size(dtype_id, new_size);
                                    H5Tinsert(dtype_id, (yyvsp[-3].sval), (yyvsp[-1].ival), (yyvsp[-6].hid));
                                }
                            }
                            if((yyvsp[-3].sval)) {
                                free((yyvsp[-3].sval));
                                (yyvsp[-3].sval) = NULL;
                            }
                            cmpd_stack[csindex].is_field = 0;
                            H5Tclose((yyvsp[-6].hid));
                             
                            new_size = H5Tget_size(dtype_id);
                        }
#line 1739 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 53:
#line 210 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    {
                            (yyval.sval) = strdup(yylval.sval);
                            free(yylval.sval);
                            yylval.sval = NULL;
                        }
#line 1749 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 54:
#line 217 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.ival) = 0; }
#line 1755 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 55:
#line 219 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.ival) = yylval.ival; }
#line 1761 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 57:
#line 223 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { asindex++; /*pushd onto the stack*/ }
#line 1767 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 58:
#line 225 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { 
                          (yyval.hid) = H5Tarray_create2((yyvsp[-1].hid), arr_stack[asindex].ndims, arr_stack[asindex].dims);
                          arr_stack[asindex].ndims = 0;
                          asindex--;
                          H5Tclose((yyvsp[-1].hid));
                        }
#line 1778 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 61:
#line 235 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { arr_stack[asindex].is_dim = 1; /*notice lexer of dimension size*/ }
#line 1784 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 62:
#line 236 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { unsigned ndims = arr_stack[asindex].ndims;
                                  arr_stack[asindex].dims[ndims] = (hsize_t)yylval.ival; 
                                  arr_stack[asindex].ndims++;
                                  arr_stack[asindex].is_dim = 0; 
                                }
#line 1794 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 65:
#line 247 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = H5Tvlen_create((yyvsp[-1].hid)); H5Tclose((yyvsp[-1].hid)); }
#line 1800 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 66:
#line 252 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { is_opq_size = 1; }
#line 1806 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 67:
#line 253 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    {   
                                size_t size = (size_t)yylval.ival;
                                (yyval.hid) = H5Tcreate(H5T_OPAQUE, size);
                                is_opq_size = 0;    
                            }
#line 1816 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 68:
#line 258 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { is_opq_tag = 1; }
#line 1822 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 69:
#line 259 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    {  
                                H5Tset_tag((yyvsp[-6].hid), yylval.sval);
                                free(yylval.sval);
                                yylval.sval = NULL;
                                is_opq_tag = 0;
                            }
#line 1833 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 70:
#line 265 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { (yyval.hid) = (yyvsp[-8].hid); }
#line 1839 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 73:
#line 273 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { is_str_size = 1; }
#line 1845 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 74:
#line 274 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    {  
                                if((yyvsp[-1].ival) == H5T_VARIABLE_TOKEN)
                                    is_variable = 1;
                                else 
                                    str_size = yylval.ival;
                                is_str_size = 0; 
                            }
#line 1857 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 75:
#line 282 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    {
                                if((yyvsp[-1].ival) == H5T_STR_NULLTERM_TOKEN)
                                    str_pad = H5T_STR_NULLTERM;
                                else if((yyvsp[-1].ival) == H5T_STR_NULLPAD_TOKEN)
                                    str_pad = H5T_STR_NULLPAD;
                                else if((yyvsp[-1].ival) == H5T_STR_SPACEPAD_TOKEN)
                                    str_pad = H5T_STR_SPACEPAD;
                            }
#line 1870 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 76:
#line 291 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    {  
                                if((yyvsp[-1].ival) == H5T_CSET_ASCII_TOKEN)
                                    str_cset = H5T_CSET_ASCII;
                                else if((yyvsp[-1].ival) == H5T_CSET_UTF8_TOKEN)
                                    str_cset = H5T_CSET_UTF8;
                            }
#line 1881 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 77:
#line 298 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    {
                                if((yyvsp[-1].hid) == H5T_C_S1_TOKEN)
                                    (yyval.hid) = H5Tcopy(H5T_C_S1);
                                else if((yyvsp[-1].hid) == H5T_FORTRAN_S1_TOKEN)
                                    (yyval.hid) = H5Tcopy(H5T_FORTRAN_S1);
                            }
#line 1892 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 78:
#line 305 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    {   
                                hid_t str_id = (yyvsp[-1].hid);

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
#line 1913 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 79:
#line 322 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    {(yyval.ival) = H5T_VARIABLE_TOKEN;}
#line 1919 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 81:
#line 325 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    {(yyval.ival) = H5T_STR_NULLTERM_TOKEN;}
#line 1925 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 82:
#line 326 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    {(yyval.ival) = H5T_STR_NULLPAD_TOKEN;}
#line 1931 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 83:
#line 327 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    {(yyval.ival) = H5T_STR_SPACEPAD_TOKEN;}
#line 1937 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 84:
#line 329 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    {(yyval.ival) = H5T_CSET_ASCII_TOKEN;}
#line 1943 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 85:
#line 330 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    {(yyval.ival) = H5T_CSET_UTF8_TOKEN;}
#line 1949 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 86:
#line 332 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    {(yyval.hid) = H5T_C_S1_TOKEN;}
#line 1955 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 87:
#line 333 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    {(yyval.hid) = H5T_FORTRAN_S1_TOKEN;}
#line 1961 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 88:
#line 337 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { is_enum = 1; enum_id = H5Tenum_create((yyvsp[-1].hid)); H5Tclose((yyvsp[-1].hid)); }
#line 1967 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 89:
#line 339 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    { is_enum = 0; /*reset*/ (yyval.hid) = enum_id; }
#line 1973 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 92:
#line 344 "hl/src/H5LTparse.y" /* yacc.c:1646  */
    {
                                                is_enum_memb = 1; /*indicate member of enum*/
#ifdef H5_HAVE_WIN32_API
                                                enum_memb_symbol = _strdup(yylval.sval); 
#else /* H5_HAVE_WIN32_API */
                                                enum_memb_symbol = strdup(yylval.sval); 
#endif  /* H5_HAVE_WIN32_API */
                                                free(yylval.sval);
                                                yylval.sval = NULL;
                                            }
#line 1988 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;

  case 93:
#line 355 "hl/src/H5LTparse.y" /* yacc.c:1646  */
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
                                    if(enum_memb_symbol) free(enum_memb_symbol);
                                }

                                H5Tclose(super);
                                H5Tclose(native);
                            }
#line 2035 "hl/src/H5LTparse.c" /* yacc.c:1646  */
    break;


#line 2039 "hl/src/H5LTparse.c" /* yacc.c:1646  */
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

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
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

  /* Do not reclaim the symbols of the rule whose action triggered
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
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

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
  /* Do not reclaim the symbols of the rule whose action triggered
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
  return yyresult;
}
