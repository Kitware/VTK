/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParseString.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright (c) 2012 David Gobbi.

  Contributed to the VisualizationToolkit by the author in April 2012
  under the terms of the Visualization Toolkit 2008 copyright.
-------------------------------------------------------------------------*/

/**
  This file provides string handling routines.

  The two important jobs done by these routines are string tokenization
  and string caching.

  Tokenization is done as per the rules of a C++ preprocessor, and
  breaks the strings into ids, literals, and operators.  Any string
  is a valid input for the tokenizer, and it is up to the parser to
  decide if the resulting tokens are valid within the grammar.  The
  two primary tokenization functions are vtkParse_InitTokenizer()
  and vtkParse_NextToken().

  Caching refers to how string memory management is done.  The
  parser uses "const char *" for all strings, and expects all strings
  to be persistent and constant.  These conditions are automatically
  met by static strings, but dynamically-generated strings must be
  cached until the parse is complete.  The primary caching functions
  are vtkParse_CacheString() and vtkParse_FreeStringCache().
*/

#ifndef vtkParseString_h
#define vtkParseString_h

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Various important char types for tokenization
 */
typedef enum _parse_char_type
{
  CPRE_NONDIGIT = 0x01,  /* A-Z a-z and _ */
  CPRE_DIGIT    = 0x02,  /* 0-9 */
  CPRE_XDIGIT   = 0x03,  /* 0-9 A-Z a-z and _ */
  CPRE_EXTEND   = 0x04,  /* non-ascii character */
  CPRE_ID       = 0x05,  /* starting char for identifier */
  CPRE_XID      = 0x07,  /* continuing char for identifier */
  CPRE_HEX      = 0x08,  /* 0-9 A-F a-f hexadecimal digits */
  CPRE_SIGN     = 0x10,  /* +- (sign for floats) */
  CPRE_QUOTE    = 0x20,  /* " and ' */
  CPRE_HSPACE   = 0x40,  /* space, tab, carriage return */
  CPRE_VSPACE   = 0x80,  /* newline, vertical tab, form feed */
  CPRE_WHITE    = 0xC0,  /* all whitespace characters */
} parse_char_type;

/**
 * Character type lookup table
 */
extern unsigned char parse_charbits[256];

/**
 * Macro to check if a char is of a certain type
 */
#define vtkParse_CharType(c, bits) \
  ((parse_charbits[(unsigned char)(c)] & (bits)) != 0)

/**
 * Whitespace types that can be used with the tokenizer.
 * - WS_DEFAULT treats newlines and formfeeds as regular whitespace.
 * - WS_PREPROC treats newline as end-of-line, not as whitespace.
 * - WS_COMMENT treats comments as tokens, not as whitespace.
 */
typedef enum _parse_space_t
{
  WS_DEFAULT = CPRE_WHITE,  /* skip all whitespace */
  WS_PREPROC = CPRE_HSPACE, /* skip horizontal whitespace only */
  WS_COMMENT = (CPRE_WHITE | 0x100), /* comments as tokens */
} parse_space_t;

/**
 * Preprocessor tokens for C++.
 */
typedef enum _preproc_token_t
{
  TOK_OTHER = 257,
  TOK_ID,        /* any id */
  TOK_CHAR,      /* char literal */
  TOK_STRING,    /* string literal */
  TOK_NUMBER,    /* any numeric literal */
  TOK_COMMENT,   /* C or C++ comment */
  TOK_DBLHASH,   /* ## */
  TOK_SCOPE,     /* :: */
  TOK_INCR,      /* ++ */
  TOK_DECR,      /* -- */
  TOK_RSHIFT,    /* >> */
  TOK_LSHIFT,    /* << */
  TOK_AND,       /* && */
  TOK_OR,        /* || */
  TOK_EQ,        /* == */
  TOK_NE,        /* != */
  TOK_GE,        /* >= */
  TOK_LE,        /* <= */
  TOK_ADD_EQ,    /* += */
  TOK_SUB_EQ,    /* -= */
  TOK_MUL_EQ,    /* *= */
  TOK_DIV_EQ,    /* /= */
  TOK_MOD_EQ,    /* %= */
  TOK_AND_EQ,    /* &= */
  TOK_OR_EQ,     /* |= */
  TOK_XOR_EQ,    /* ^= */
  TOK_ARROW,     /* -> */
  TOK_DOT_STAR,  /* .* */
  TOK_ARROW_STAR,/* ->* */
  TOK_RSHIFT_EQ, /* >>= */
  TOK_LSHIFT_EQ, /* <<= */
  TOK_ELLIPSIS,  /* ... */
} preproc_token_t;

/**
 * A struct for going through a string one token at a time.
 * If ws is set to WS_PREPROC, then tokenization stops when a
 * newline or null is encountered.  If ws is set to WS_DEFAULT,
 * then tokenization only stops when a null is encountered.  If
 * ws is set to WS_COMMENT, then tokenization stops only when
 * a null is encountered, and comments are returned as tokens
 * instead of being skipped as whitespace.
 */
typedef struct _StringTokenizer
{
  int tok;           /* the current token */
  unsigned int hash; /* the hash of the current token, if it is an id */
  const char *text;  /* the text for the current token, not null-teminated */
  size_t len;        /* the length of the current token */
  parse_space_t ws;  /* controls what to consider as whitespace */
} StringTokenizer;

/**
 * Initialize the tokenizer and get the first token.
 */
void vtkParse_InitTokenizer(
  StringTokenizer *tokens, const char *text, parse_space_t wstype);

/**
 * Return the next preprocessor token, or '0' if none left.
 */
int vtkParse_NextToken(StringTokenizer *tokens);

/**
 * Skip over whitespace.
 * Return the number of chars until the first non-whitespace token.
 * Set spacetype to WS_DEFAULT, WS_PREPROC, or WS_COMMENT.
 */
size_t vtkParse_SkipWhitespace(
  const char *cp, parse_space_t spacetype);

/**
 * Skip over a comment, C style or C++ style.
 * Return the number of chars until the end of the comment.
 */
size_t vtkParse_SkipComment(const char *cp);

/**
 * Skip over a string in double or single quotes.
 * Return the number of chars until the end of the quotes.
 */
size_t vtkParse_SkipQuotes(const char *cp);

/**
 * Skip over a number.  Uses preprocessor semantics.
 * Return the number of chars until the end of the number.
 */
size_t vtkParse_SkipNumber(const char *cp);

/**
 * Skip over an identifier.
 * Return the number of chars until the end of the identifier.
 */
size_t vtkParse_SkipId(const char *cp);

/**
 * Compute the hash for a id, for use in hash table lookups.
 * This stops at the first non-Id character, so it is safe to use
 * on a string that is not null-terminated as long as there is either
 * whitespace or an operator character before the end of the string.
 * It can be used on null-terminated strings as well, of course.
 */
unsigned int vtkParse_HashId(const char *cp);

/**
 * Decode a single unicode character from utf8, or set error flag to 1.
 * The character pointer will be advanced by one if an error occurred,
 * and the return value will be the value of the first octet.
 */
unsigned int vtkParse_DecodeUtf8(const char **cpp, int *error_flag);

/**
 * StringCache provides a simple way of allocating strings centrally.
 * It eliminates the need to allocate and free each individual string,
 * which makes the code simpler and more efficient.
 */
typedef struct _StringCache
{
  unsigned long  NumberOfChunks;
  char         **Chunks;
  size_t         ChunkSize;
  size_t         Position;
} StringCache;

/**
 * Initialize the string cache.
 */
void vtkParse_InitStringCache(StringCache *cache);

/**
 * Allocate a new string from the cache.
 * A total of n+1 bytes will be allocated, to leave room for null.
 */
char *vtkParse_NewString(StringCache *cache, size_t n);

/**
 * Cache a string so that it can then be used in the vtkParse data
 * structures.  The string will last until the application exits.
 * At most 'n' chars will be copied, and the string will be terminated.
 * If a null pointer is provided, then a null pointer will be returned.
 */
const char *vtkParse_CacheString(
  StringCache *cache, const char *cp, size_t n);

/**
 * Free all strings that were created with vtkParse_NewString() or
 * with vtkParse_CacheString().
 */
void vtkParse_FreeStringCache(StringCache *cache);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
