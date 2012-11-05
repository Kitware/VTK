/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParsePreprocess.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright (c) 2010 David Gobbi.

  Contributed to the VisualizationToolkit by the author in June 2010
  under the terms of the Visualization Toolkit 2008 copyright.
-------------------------------------------------------------------------*/

#include "vtkParsePreprocess.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>

/**
  This file handles preprocessor directives via a simple
  recursive-descent parser that only evaluates integers.
*/

#define PREPROC_DEBUG 0

/** Block size for reading files */
#define FILE_BUFFER_SIZE 8192

/** Size of hash table must be a power of two */
#define PREPROC_HASH_TABLE_SIZE 1024u

/** Hashes for preprocessor keywords */
#define HASH_IFDEF      0x0fa4b283u
#define HASH_IFNDEF     0x04407ab1u
#define HASH_IF         0x00597834u
#define HASH_ELIF       0x7c964b25u
#define HASH_ELSE       0x7c964c6eu
#define HASH_ENDIF      0x0f60b40bu
#define HASH_DEFINED    0x088998d4u
#define HASH_DEFINE     0xf8804a70u
#define HASH_UNDEF      0x10823b97u
#define HASH_INCLUDE    0x9e36af89u
#define HASH_ERROR      0x0f6321efu
#define HASH_LINE       0x7c9a15adu
#define HASH_PRAGMA     0x1566a9fdu

/** Various possible char types */
#define CPRE_ID         0x01  /* A-Z a-z and _ */
#define CPRE_DIGIT      0x02  /* 0-9 */
#define CPRE_IDGIT      0x03  /* 0-9 A-Z a-z and _ */
#define CPRE_HEX        0x04  /* 0-9A-Fa-f */
#define CPRE_EXP        0x08  /* EPep (exponents for floats) */
#define CPRE_SIGN       0x10  /* +- (sign for floats) */
#define CPRE_QUOTE      0x20  /* " and ' */
#define CPRE_HSPACE     0x40  /* space, tab, carriage return */
#define CPRE_VSPACE     0x80  /* newline, vertical tab, form feed */
#define CPRE_WHITE      0xC0  /* all whitespace characters */

/** Whitespace types.
 * WS_NO_EOL treats newline as end-of-line, instead of whitespace.
 * WS_ALL treats newlines as regular whitespace.
 * WS_COMMENT does not treat comments as whitespace, allowing
 * comments blocks to be returned as tokens. */
typedef enum _preproc_space_t
{
  WS_NO_EOL = CPRE_HSPACE, /* skip horizontal whitespace only */
  WS_ALL    = CPRE_WHITE,  /* skip all whitespace */
  WS_COMMENT = (CPRE_WHITE | 0x100), /* comments as tokens */
} preproc_space_t;

/** Preprocessor tokens. */
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

/** A struct for going through the input one token at a time. */
typedef struct _preproc_tokenizer
{
  int tok;
  unsigned int hash;
  const char *text;
  size_t len;
} preproc_tokenizer;

/** Extend dynamic arrays in a progression of powers of two.
 * Whenever "n" reaches a power of two, then the array size is
 * doubled so that "n" can be safely incremented. */
static void *preproc_array_check(
  void *arraymem, size_t size, int n)
{
  /* if empty, alloc for the first time */
  if (n == 0)
    {
    return malloc(size);
    }
  /* if count is power of two, reallocate with double size */
  else if ((n & (n-1)) == 0)
    {
    return realloc(arraymem, (n << 1)*size);
    }

  /* no reallocation, just return the original array */
  return arraymem;
}

/** Convert string to int. */
static preproc_int_t string_to_preproc_int(const char *cp, int base)
{
#if defined(_WIN32) && !defined(__MINGW32__) && !defined(__CYGWIN__)
  return _strtoi64(cp, NULL, base);
#else
  return strtoll(cp, NULL, base);
#endif
}

/** Convert string to unsigned int. */
static preproc_uint_t string_to_preproc_uint(const char *cp, int base)
{
#if defined(_WIN32) && !defined(__MINGW32__) && !defined(__CYGWIN__)
  return _strtoui64(cp, NULL, base);
#else
  return strtoull(cp, NULL, base);
#endif
}

/** Array for quick lookup of char types */
static unsigned char preproc_charbits[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0,
  CPRE_HSPACE, /* tab */
  CPRE_VSPACE, CPRE_VSPACE, CPRE_VSPACE, /* newline, vtab, form feed */
  CPRE_HSPACE, /* carriage return */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  CPRE_HSPACE, /* ' ' */
  0, CPRE_QUOTE, 0, 0, 0, 0, CPRE_QUOTE, 0, 0, /* !"#$%&'() */
  0, CPRE_SIGN, 0, CPRE_SIGN, 0, 0, /* *+,-./ */
  CPRE_DIGIT|CPRE_HEX, /* 0 */
  CPRE_DIGIT|CPRE_HEX, CPRE_DIGIT|CPRE_HEX,
  CPRE_DIGIT|CPRE_HEX, CPRE_DIGIT|CPRE_HEX,
  CPRE_DIGIT|CPRE_HEX, CPRE_DIGIT|CPRE_HEX,
  CPRE_DIGIT|CPRE_HEX, CPRE_DIGIT|CPRE_HEX,
  CPRE_DIGIT|CPRE_HEX, /* 9 */
  0, 0, 0, 0, 0, 0, 0, /* :;<=>?@ */
  CPRE_ID|CPRE_HEX, /* A */
  CPRE_ID|CPRE_HEX, CPRE_ID|CPRE_HEX, CPRE_ID|CPRE_HEX, /* BCD */
  CPRE_ID|CPRE_HEX|CPRE_EXP, /* E */
  CPRE_ID|CPRE_HEX, CPRE_ID, CPRE_ID, CPRE_ID, /* FGHI */
  CPRE_ID, CPRE_ID, CPRE_ID, CPRE_ID, /* JKLM */
  CPRE_ID, CPRE_ID, CPRE_ID|CPRE_EXP, CPRE_ID, /* NOPQ */
  CPRE_ID, CPRE_ID, CPRE_ID, CPRE_ID, /* RSTU */
  CPRE_ID, CPRE_ID, CPRE_ID, CPRE_ID, /* VWXY */
  CPRE_ID, /* Z */
  0, 0, 0, 0, /* [\\]^ */
  CPRE_ID, /* _ */
  0, /* ` */
  CPRE_ID|CPRE_HEX, /* a */
  CPRE_ID|CPRE_HEX, CPRE_ID|CPRE_HEX, CPRE_ID|CPRE_HEX, /* bcd */
  CPRE_ID|CPRE_HEX|CPRE_EXP, /* e */
  CPRE_ID|CPRE_HEX, CPRE_ID, CPRE_ID, CPRE_ID, /* fghi */
  CPRE_ID, CPRE_ID, CPRE_ID, CPRE_ID, /* jklm */
  CPRE_ID, CPRE_ID, CPRE_ID|CPRE_EXP, CPRE_ID, /* nopq */
  CPRE_ID, CPRE_ID, CPRE_ID, CPRE_ID, /* rstu */
  CPRE_ID, CPRE_ID, CPRE_ID, CPRE_ID, /* vwxy */
  CPRE_ID, /* z */
  0, 0, 0, 0, /* {|}~ */
  0, /* '\x7f' */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/** Macro to get char type */
#define preproc_chartype(c, bits) \
  ((preproc_charbits[(unsigned char)(c)] & bits) != 0)

/** Skip over a comment. */
static void preproc_skip_comment(const char **cpp)
{
  const char *cp = *cpp;

  if (cp[0] == '/')
    {
    if (cp[1] == '/')
      {
      cp += 2;
      while (*cp != '\n' && *cp != '\0')
        {
        if (cp[0] == '\\')
          {
          if (cp[1] == '\n') { cp++; }
          else if (cp[1] == '\r' && cp[2] == '\n') { cp += 2; }
          }
        cp++;
        }
      }
    else if (cp[1] == '*')
      {
      cp += 2;
      while (*cp != '\0')
        {
        if (cp[0] == '*' && cp[1] == '/') { cp += 2; break; }
        cp++;
        }
      }
    }

  *cpp = cp;
}

/** Skip over whitespace, but not newlines unless preceded by backlash. */
static void preproc_skip_whitespace(
  const char **cpp, preproc_space_t spacetype)
{
  const char *cp = *cpp;

  for (;;)
    {
    if (preproc_chartype(*cp, spacetype))
      {
      do
        {
        cp++;
        }
      while (preproc_chartype(*cp, spacetype));
      }
    if (cp[0] == '\\')
      {
      if (cp[1] == '\n')
        {
        cp += 2;
        }
      else if (cp[1] == '\r' && cp[2] == '\n')
        {
        cp += 3;
        }
      else
        {
        break;
        }
      }
    else if (cp[0] == '/' && (spacetype & WS_COMMENT) != WS_COMMENT)
      {
      if (cp[1] == '/' || cp[1] == '*')
        {
        preproc_skip_comment(&cp);
        }
      else
        {
        break;
        }
      }
    else
      {
      break;
      }
    }

  *cpp = cp;
}

/** Skip over string and char literals. */
static void preproc_skip_quotes(const char **cpp)
{
  const char *cp = *cpp;
  const char qc = *cp;

  if (preproc_chartype(*cp, CPRE_QUOTE))
    {
    cp++;
    while (*cp != qc && *cp != '\n' && *cp != '\0')
      {
      if (*cp++ == '\\')
        {
        if (cp[0] == '\r' && cp[1] == '\n') { cp += 2; }
        else if (*cp != '\0') { cp++; }
        }
      }
    }
  if (*cp == qc)
    {
    cp++;
    }

  *cpp = cp;
}

/** Skip over a name. */
static void preproc_skip_name(const char **cpp)
{
  const char *cp = *cpp;

  if (preproc_chartype(*cp, CPRE_ID))
    {
    do
      {
      cp++;
      }
    while (preproc_chartype(*cp, CPRE_IDGIT));
    }

  *cpp = cp;
}

/** A simple 32-bit hash function based on "djb2". */
static unsigned int preproc_hash_name(const char **cpp)
{
  const char *cp = (*cpp);
  int h = 5381;

  if (preproc_chartype(*cp, CPRE_ID))
    {
    do { h = (h << 5) + h + (unsigned char)*cp++; }
    while (preproc_chartype(*cp, CPRE_IDGIT));
    }

  *cpp = cp;
  return h;
}

/** Skip over a number. */
static void preproc_skip_number(const char **cpp)
{
  const char *cp = *cpp;

  if (preproc_chartype(cp[0], CPRE_DIGIT) ||
      (cp[0] == '.' && preproc_chartype(cp[1], CPRE_DIGIT)))
    {
    do
      {
      char c = *cp++;
      if (preproc_chartype(c, CPRE_EXP) &&
          preproc_chartype(*cp, CPRE_SIGN))
        {
        cp++;
        }
      }
    while (preproc_chartype(*cp, CPRE_IDGIT) || *cp == '.');
    }

  *cpp = cp;
}

/** Return the next preprocessor token, or '0' if none left. */
static int preproc_next(preproc_tokenizer *tokens)
{
  const char *cp = tokens->text + tokens->len;
  preproc_skip_whitespace(&cp, WS_NO_EOL);

  if (preproc_chartype(*cp, CPRE_ID))
    {
    const char *ep = cp;
    unsigned int h = preproc_hash_name(&ep);
    tokens->tok = TOK_ID;
    tokens->hash = h;
    tokens->text = cp;
    tokens->len = ep - cp;
    }
  else if (preproc_chartype(*cp, CPRE_QUOTE))
    {
    const char *ep = cp;
    preproc_skip_quotes(&ep);
    tokens->tok = (*cp == '\"' ? TOK_STRING : TOK_CHAR);
    tokens->hash = 0;
    tokens->text = cp;
    tokens->len = ep - cp;
    }
  else if (preproc_chartype(*cp, CPRE_DIGIT) ||
           (cp[0] == '.' && preproc_chartype(cp[1], CPRE_DIGIT)))
    {
    const char *ep = cp;
    preproc_skip_number(&ep);
    tokens->tok = TOK_NUMBER;
    tokens->hash = 0;
    tokens->text = cp;
    tokens->len = ep - cp;
    }
  else if (cp[0] == '/' && (cp[1] == '/' || cp[1] == '*'))
    {
    const char *ep = cp;
    preproc_skip_comment(&ep);
    tokens->tok = TOK_COMMENT;
    tokens->hash = 0;
    tokens->text = cp;
    tokens->len = ep - cp;
    }
  else
    {
    int t = cp[0];
    size_t l = 1;

    switch (cp[0])
      {
      case ':':
        if (cp[1] == ':') { l = 2; t = TOK_SCOPE; }
        break;
      case '.':
        if (cp[1] == '.' && cp[2] == '.') { l = 3; t = TOK_ELLIPSIS; }
        else if (cp[1] == '*') { l = 2; t = TOK_DOT_STAR; }
        break;
      case '=':
        if (cp[1] == '=') { l = 2; t = TOK_EQ; }
        break;
      case '!':
        if (cp[1] == '=') { l = 2; t = TOK_NE; }
        break;
      case '<':
        if (cp[1] == '<' && cp[2] == '=') { l = 3; t = TOK_LSHIFT_EQ; }
        else if (cp[1] == '<') { l = 2; t = TOK_LSHIFT; }
        else if (cp[1] == '=') { l = 2; t = TOK_LE; }
        break;
      case '>':
        if (cp[1] == '>' && cp[2] == '=') { l = 3; t = TOK_RSHIFT_EQ; }
        else if (cp[1] == '>') { l = 2; t = TOK_RSHIFT; }
        else if (cp[1] == '=') { l = 2; t = TOK_GE; }
        break;
      case '&':
        if (cp[1] == '=') { l = 2; t = TOK_AND_EQ; }
        else if (cp[1] == '&') { l = 2; t = TOK_AND; }
        break;
      case '|':
        if (cp[1] == '=') { l = 2; t = TOK_OR_EQ; }
        else if (cp[1] == '|') { l = 2; t = TOK_OR; }
        break;
      case '^':
        if (cp[1] == '=') { l = 2; t = TOK_XOR_EQ; }
        break;
      case '*':
        if (cp[1] == '=') { l = 2; t = TOK_MUL_EQ; }
        break;
      case '/':
        if (cp[1] == '=') { l = 2; t = TOK_DIV_EQ; }
        break;
      case '%':
        if (cp[1] == '=') { l = 2; t = TOK_MOD_EQ; }
        break;
      case '+':
        if (cp[1] == '+') { l = 2; t = TOK_INCR; }
        else if (cp[1] == '=') { l = 2; t = TOK_ADD_EQ; }
        break;
      case '-':
        if (cp[1] == '>' && cp[2] == '*') { l = 3; t = TOK_ARROW_STAR; }
        else if (cp[1] == '>') { l = 2; t = TOK_ARROW; }
        else if (cp[1] == '-') { l = 2; t = TOK_DECR; }
        else if (cp[1] == '=') { l = 2; t = TOK_SUB_EQ; }
        break;
      case '#':
        if (cp[1] == '#') { l = 2; t = TOK_DBLHASH; }
        break;
      case '\n':
      case '\0':
        { l = 0; t = 0; }
        break;
      }

    tokens->tok = t;
    tokens->hash = 0;
    tokens->text = cp;
    tokens->len = l;
    }

  return tokens->tok;
}

/** Initialize the tokenizer. */
static void preproc_init(preproc_tokenizer *tokens, const char *text)
{
  tokens->tok = 0;
  tokens->hash = 0;
  tokens->text = text;
  tokens->len = 0;
  preproc_next(tokens);
}

/** Tokenize and compare two strings */
static int preproc_identical(const char *text1, const char *text2)
{
  int result = 1;

  if (text1 != text2)
    {
    result = 0;

    if (text1 && text2)
      {
      preproc_tokenizer t1;
      preproc_tokenizer t2;

      preproc_init(&t1, text1);
      preproc_init(&t2, text2);

      do
        {
        if (t1.tok != t2.tok ||
            t1.hash != t2.hash ||
            t1.len != t2.len ||
            strncmp(t1.text, t2.text, t1.len) != 0)
          {
          break;
          }
        preproc_next(&t1);
        preproc_next(&t2);
        }
      while (t1.tok && t2.tok);

      result = (t1.tok == 0 && t2.tok == 0);
      }
    }

  return result;
}

/** Duplicate the first n bytes of a string. */
static const char *preproc_strndup(const char *in, size_t n)
{
  char *res = NULL;

  res = (char *)malloc(n+1);
  strncpy(res, in, n);
  res[n] = '\0';

  return res;
}

/** Create a new preprocessor macro. */
static MacroInfo *preproc_new_macro(
  PreprocessInfo *info, const char *name, const char *definition)
{
  MacroInfo *macro = (MacroInfo *)malloc(sizeof(MacroInfo));
  vtkParsePreprocess_InitMacro(macro);

  if (name)
    {
    size_t n;
    const char *cp = name;
    preproc_skip_name(&cp);
    n = cp - name;
    macro->Name = preproc_strndup(name, n);
    }

  if (definition)
    {
    size_t n;
    const char *cp = definition;
    preproc_tokenizer tokens;
    preproc_init(&tokens, cp);

    do
      {
      cp = tokens.text + tokens.len;
      }
    while (preproc_next(&tokens));

    n = cp - definition;
    macro->Definition = preproc_strndup(definition, n);
    }

  macro->IsExternal = info->IsExternal;

  return macro;
}

/** Free a preprocessor macro struct. */
static void preproc_free_macro(MacroInfo *info)
{
  free(info);
}

/** Find a preprocessor macro, return 0 if not found. */
static MacroInfo *preproc_find_macro(
  PreprocessInfo *info, preproc_tokenizer *token)
{
  unsigned int m = PREPROC_HASH_TABLE_SIZE - 1;
  unsigned int i = (token->hash & m);
  const char *name = token->text;
  size_t l = token->len;
  MacroInfo ***htable = info->MacroHashTable;
  MacroInfo **hptr;
  const char *mname;

  if (htable && ((hptr = htable[i]) != NULL) && *hptr)
    {
    do
      {
      mname = (*hptr)->Name;
      if (mname[0] == name[0] &&
          strncmp(mname, name, l) == 0 &&
          mname[l] == '\0')
        {
        return *hptr;
        }
      hptr++;
      }
    while (*hptr);
    }

  return NULL;
}

/** Return the address of the macro within the hash table.
  * If "insert" is nonzero, add a new location if macro not found. */
static MacroInfo **preproc_macro_location(
  PreprocessInfo *info, preproc_tokenizer *token, int insert)
{
  MacroInfo ***htable = info->MacroHashTable;
  unsigned int m = PREPROC_HASH_TABLE_SIZE - 1;
  unsigned int i = (token->hash & m);
  const char *name = token->text;
  size_t l = token->len;
  size_t n;
  MacroInfo **hptr;
  const char *mname;

  if (htable == NULL)
    {
    if (!insert)
      {
      return NULL;
      }

    m = PREPROC_HASH_TABLE_SIZE;
    htable = (MacroInfo ***)malloc(m*sizeof(MacroInfo **));
    info->MacroHashTable = htable;
    do { *htable++ = NULL; } while (--m);
    htable = info->MacroHashTable;
    }

  hptr = htable[i];

  if (hptr == NULL)
    {
    if (!insert)
      {
      return NULL;
      }

    hptr = (MacroInfo **)malloc(2*sizeof(MacroInfo *));
    hptr[0] = NULL;
    hptr[1] = NULL;
    htable[i] = hptr;
    }
  else if (*hptr)
    {
    /* see if macro is already there */
    n = 0;
    do
      {
      mname = (*hptr)->Name;
      if (mname[0] == name[0] &&
          strncmp(mname, name, l) == 0 &&
          mname[l] == '\0')
        {
        break;
        }
      n++;
      hptr++;
      }
    while (*hptr);

    if (*hptr == NULL)
      {
      if (!insert)
        {
        return NULL;
        }

      /* if n+1 is a power of two, double allocated space */
      if (n > 0 && (n & (n+1)) == 0)
        {
        hptr = htable[i];
        hptr = (MacroInfo **)realloc(hptr, (2*(n+1))*sizeof(MacroInfo *));
        htable[i] = hptr;
        hptr += n;
        }

      /* add a terminating null */
      hptr[1] = NULL;
      }
    }

  return hptr;
}

/** Remove a preprocessor macro.  Returns 0 if macro not found. */
static int preproc_remove_macro(
  PreprocessInfo *info, preproc_tokenizer *token)
{
  MacroInfo **hptr;

  hptr = preproc_macro_location(info, token, 0);

  if (hptr && *hptr)
    {
    preproc_free_macro(*hptr);

    do
      {
      hptr[0] = hptr[1];
      hptr++;
      }
    while (*hptr);

    return 1;
    }

  return 0;
}

/** A simple way to add a preprocessor macro definition. */
static MacroInfo *preproc_add_macro_definition(
  PreprocessInfo *info, const char *name, const char *definition)
{
  preproc_tokenizer token;
  MacroInfo *macro;
  MacroInfo **macro_p;

  preproc_init(&token, name);

  macro = preproc_new_macro(info, name, definition);
  macro_p = preproc_macro_location(info, &token, 1);
#if PREPROC_DEBUG
  if (*macro_p)
    {
    fprintf(stderr, "duplicate macro definition %s\n", name);
    }
#endif
  *macro_p = macro;

  return macro;
}

/** Skip over parentheses, return nonzero if not closed. */
static int preproc_skip_parentheses(preproc_tokenizer *tokens)
{
  int depth = 0;

  if (tokens->tok == '(')
    {
    depth = 1;

    while (depth > 0 && preproc_next(tokens))
      {
      if (tokens->tok == '(')
        {
        depth++;
        }
      else if (tokens->tok == ')')
        {
        depth--;
        }
      }
    }

  if (tokens->tok == ')')
    {
    preproc_next(tokens);
    return VTK_PARSE_OK;
    }

#if PREPROC_DEBUG
  fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
  return VTK_PARSE_SYNTAX_ERROR;
}


/** Evaluate a char literal to an integer value. */
static int preproc_evaluate_char(
  const char *cp, preproc_int_t *val, int *is_unsigned)
{
  if (cp[0] == '\'')
    {
    cp++;
    if (*cp != '\\')
      {
      *val = *cp;
      }
    else if (*cp != '\'' && *cp != '\n' && *cp != '\0')
      {
      cp++;
      if (*cp == 'a') { *val = '\a'; }
      else if (*cp == 'b') { *val = '\b'; }
      else if (*cp == 'f') { *val = '\f'; }
      else if (*cp == 'n') { *val = '\n'; }
      else if (*cp == 'r') { *val = '\r'; }
      else if (*cp == 'b') { *val = '\b'; }
      else if (*cp == 't') { *val = '\t'; }
      else if (*cp == 'v') { *val = '\v'; }
      else if (*cp == '\'') { *val = '\''; }
      else if (*cp == '\"') { *val = '\"'; }
      else if (*cp == '\\') { *val = '\\'; }
      else if (*cp == '\?') { *val = '\?'; }
      else if (*cp == '0')
        {
        *val = string_to_preproc_int(cp, 8);
        do { cp++; } while (*cp >= '0' && *cp <= '7');
        }
      else if (*cp == 'x')
        {
        *val = string_to_preproc_int(cp+1, 16);
        do { cp++; } while (preproc_chartype(*cp, CPRE_HEX));
        }
      }
    if (*cp != '\'')
      {
#if PREPROC_DEBUG
      fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
      return VTK_PARSE_SYNTAX_ERROR;
      }
    cp++;
    *is_unsigned = 0;
    return VTK_PARSE_OK;
    }

#if PREPROC_DEBUG
  fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
  return VTK_PARSE_SYNTAX_ERROR;
}

/* Evaluate an integer, ignoring any suffixes except 'u'. */
static int preproc_evaluate_integer(
  const char *cp, preproc_int_t *val, int *is_unsigned)
{
  const char *ep;
  int base = 0;
  ep = cp;

  if (cp[0] == '0' && (cp[1] == 'x' || cp[1] == 'X'))
    {
    cp += 2;
    base = 16;
    *is_unsigned = 1;
    ep = cp;
    while (preproc_chartype(*ep, CPRE_HEX))
      {
      ep++;
      }
    }
  else if (cp[0] == '0' && preproc_chartype(cp[1], CPRE_DIGIT))
    {
    cp += 1;
    base = 8;
    *is_unsigned = 1;
    ep = cp;
    while (*ep >= '0' && *ep <= '7')
      {
      ep++;
      }
    }
  else
    {
    base = 10;
    *is_unsigned = 0;
    while (preproc_chartype(*ep, CPRE_DIGIT))
      {
      ep++;
      }
    }

  for (;;)
    {
    if (ep[0] == 'i' && ep[1] == '6' && ep[2] == '4') { ep += 3; }
    else if (*ep == 'u') { *is_unsigned = 1; ep++; }
    else if (*ep == 'l' || *ep == 'L') { ep++; }
    else { break; }
    }

  if (*is_unsigned)
    {
    *val = (preproc_int_t)string_to_preproc_uint(cp, base);
    }
  else
    {
    *val = string_to_preproc_int(cp, base);
    }

  if (*ep == '.' || *ep == 'e' || *ep == 'E')
    {
    return VTK_PARSE_PREPROC_DOUBLE;
    }

  return VTK_PARSE_OK;
}

/* forward declaration */
static int preproc_evaluate_expression(
  PreprocessInfo *info, preproc_tokenizer *tokens,
  preproc_int_t *val, int *is_unsigned);

/** Evaluate a single item in an expression. */
static int preproc_evaluate_single(
  PreprocessInfo *info, preproc_tokenizer *tokens,
  preproc_int_t *val, int *is_unsigned)
{
  int result = VTK_PARSE_OK;

  while (tokens->tok == TOK_ID)
    {
    /* handle the "defined" keyword */
    if (tokens->hash == HASH_DEFINED && tokens->len == 7 &&
        strncmp("defined", tokens->text, tokens->len) == 0)
      {
      int paren = 0;
      preproc_next(tokens);

      if (tokens->tok == '(')
        {
        paren = 1;
        preproc_next(tokens);
        }
      if (tokens->tok != TOK_ID)
        {
        *val = 0;
        *is_unsigned = 0;
#if PREPROC_DEBUG
        fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
        return VTK_PARSE_SYNTAX_ERROR;
        }

      /* do the name lookup */
      *is_unsigned = 0;
      *val = (preproc_find_macro(info, tokens) != 0);

      preproc_next(tokens);
      if (paren)
        {
        if (tokens->tok != ')')
          {
#if PREPROC_DEBUG
          fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
          return VTK_PARSE_SYNTAX_ERROR;
          }
        preproc_next(tokens);
        }

      return result;
      }
    else
      {
      /* look up and evaluate the macro */
      MacroInfo *macro = preproc_find_macro(info, tokens);
      const char *args = NULL;
      const char *expansion = NULL;
      const char *cp;
      preproc_next(tokens);
      *val = 0;
      *is_unsigned = 0;

      if (macro == NULL || macro->IsExcluded)
        {
        return VTK_PARSE_MACRO_UNDEFINED;
        }
      else if (macro->IsFunction)
        {
        /* expand function macros using the arguments */
        args = tokens->text;
        if (tokens->tok != '(' ||
            preproc_skip_parentheses(tokens) != VTK_PARSE_OK)
          {
#if PREPROC_DEBUG
          fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
          return VTK_PARSE_SYNTAX_ERROR;
          }
        }
      expansion = vtkParsePreprocess_ExpandMacro(info, macro, args);
      if (expansion == NULL)
        {
        free((char *)args);
#if PREPROC_DEBUG
        fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
        return (args ? VTK_PARSE_MACRO_NUMARGS : VTK_PARSE_SYNTAX_ERROR);
        }
      cp = expansion;
      preproc_skip_whitespace(&cp, WS_NO_EOL);
      if (*cp != '\0')
        {
        macro->IsExcluded = 1;
        result = vtkParsePreprocess_EvaluateExpression(
          info, expansion, val, is_unsigned);
        macro->IsExcluded = 0;
        vtkParsePreprocess_FreeMacroExpansion(
          info, macro, expansion);
        return result;
        }
      vtkParsePreprocess_FreeMacroExpansion(info, macro, expansion);
      }
    /* if macro expansion was empty, continue */
    }

  if (tokens->tok == '(')
    {
    preproc_next(tokens);
    result = preproc_evaluate_expression(info, tokens, val, is_unsigned);
    if ((result & VTK_PARSE_FATAL_ERROR) == 0)
      {
      if (tokens->tok == ')')
        {
        preproc_next(tokens);
        return result;
        }
#if PREPROC_DEBUG
      fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
      return VTK_PARSE_SYNTAX_ERROR;
      }
    return result;
    }
  else if (tokens->tok == TOK_NUMBER)
    {
    result = preproc_evaluate_integer(tokens->text, val, is_unsigned);
    if (tokens->text[tokens->len-1] == 'f' ||
        tokens->text[tokens->len-1] == 'F')
      {
      result = VTK_PARSE_PREPROC_FLOAT;
      }
    preproc_next(tokens);
    return result;
    }
  else if (tokens->tok == TOK_CHAR)
    {
    result = preproc_evaluate_char(tokens->text, val, is_unsigned);
    preproc_next(tokens);
    return result;
    }
  else if (tokens->tok == TOK_STRING)
    {
    *val = 0;
    *is_unsigned = 0;
    preproc_next(tokens);
    while (tokens->tok == TOK_STRING)
      {
      preproc_next(tokens);
      }
    return VTK_PARSE_PREPROC_STRING;
    }

  *val = 0;
  *is_unsigned = 0;
#if PREPROC_DEBUG
  fprintf(stderr, "syntax error %d \"%*.*s\"\n", __LINE__,
          (int)tokens->len, (int)tokens->len, tokens->text);
#endif
  return VTK_PARSE_SYNTAX_ERROR;
}

static int preproc_evaluate_unary(
  PreprocessInfo *info, preproc_tokenizer *tokens,
  preproc_int_t *val, int *is_unsigned)
{
  int op = tokens->tok;
  int result = VTK_PARSE_OK;

  if (op != '+' && op != '-' && op != '~' && op != '!')
    {
    return preproc_evaluate_single(info, tokens, val, is_unsigned);
    }

  preproc_next(tokens);

  result = preproc_evaluate_unary(info, tokens, val, is_unsigned);
  if ((result & VTK_PARSE_FATAL_ERROR) == 0)
    {
    if (op == '~') { *val = ~(*val); }
    else if (op == '!') { *val = !(*val); *is_unsigned = 0; }
    else if (op == '-') { *val = -(*val); }
    return result;
    }

  return result;
}

static int preproc_evaluate_multiply(
  PreprocessInfo *info, preproc_tokenizer *tokens,
  preproc_int_t *val, int *is_unsigned)
{
  int op;
  preproc_int_t rval;
  int rtype;
  int result;

  result = preproc_evaluate_unary(info, tokens, val, is_unsigned);
  while ((result & VTK_PARSE_FATAL_ERROR) == 0)
    {
    op = tokens->tok;
    if (op != '*' && op != '/' && op != '%')
      {
      return result;
      }

    preproc_next(tokens);

    result = preproc_evaluate_unary(info, tokens, &rval, &rtype);

    *is_unsigned = (*is_unsigned || rtype);

    if (*is_unsigned)
      {
      if (op == '*')
        {
        *val = (preproc_int_t)((preproc_uint_t)*val *
                                (preproc_uint_t)rval);
        }
      else if (op == '/')
        {
        if (rval != 0)
          {
          *val = (preproc_int_t)((preproc_uint_t)*val /
                                 (preproc_uint_t)rval);
          }
        else
          {
          *val = 2147483647;
          }
        }
      else if (op == '%')
        {
        if (rval != 0)
          {
          *val = (preproc_int_t)((preproc_uint_t)*val %
                                  (preproc_uint_t)rval);
          }
        else
          {
          *val = 2147483647;
          }
        }
      }
    else
      {
      if (op == '*')
        {
        *val = *val * rval;
        }
      else if (op == '/')
        {
        if (rval != 0)
          {
          *val = *val / rval;
          }
        else if (*val < 0)
          {
          *val = -2147483647;
          }
        else
          {
          *val = 2147483647;
          }
        }
      else if (op == '%')
        {
        if (rval != 0)
          {
          *val = *val % rval;
          }
        else if (*val < 0)
          {
          *val = -2147483647;
          }
        else
          {
          *val = 2147483647;
          }
        }
      }
    }

  return result;
}

static int preproc_evaluate_add(
  PreprocessInfo *info, preproc_tokenizer *tokens,
  preproc_int_t *val, int *is_unsigned)
{
  int op;
  preproc_int_t rval;
  int rtype;
  int result;

  result = preproc_evaluate_multiply(info, tokens, val, is_unsigned);
  while ((result & VTK_PARSE_FATAL_ERROR) == 0)
    {
    op = tokens->tok;
    if (op != '+' && op != '-')
      {
      return result;
      }

    preproc_next(tokens);

    result = preproc_evaluate_multiply(info, tokens, &rval, &rtype);

    *is_unsigned = (*is_unsigned || rtype);

    if (op == '+')
      {
      *val = *val + rval;
      }
    else if (op == '-')
      {
      *val = *val - rval;
      }
    }

  return result;
}

static int preproc_evaluate_bitshift(
  PreprocessInfo *info, preproc_tokenizer *tokens,
  preproc_int_t *val, int *is_unsigned)
{
  int op;
  preproc_int_t rval;
  int rtype;
  int result;

  result = preproc_evaluate_add(info, tokens, val, is_unsigned);
  while ((result & VTK_PARSE_FATAL_ERROR) == 0)
    {
    op = tokens->tok;

    if (op != TOK_LSHIFT && op != TOK_RSHIFT)
      {
      return result;
      }

    preproc_next(tokens);

    result = preproc_evaluate_add(info, tokens, &rval, &rtype);

    if (*is_unsigned)
      {
      if (op == TOK_LSHIFT)
        {
        *val = (preproc_int_t)((preproc_uint_t)*val << rval);
        }
      else if (op == TOK_RSHIFT)
        {
        *val = (preproc_int_t)((preproc_uint_t)*val >> rval);
        }
      }
    else
      {
      if (op == TOK_LSHIFT)
        {
        *val = *val << rval;
        }
      else if (op == TOK_RSHIFT)
        {
        *val = *val >> rval;
        }
      }
    }

  return result;
}

static int preproc_evaluate_compare(
  PreprocessInfo *info, preproc_tokenizer *tokens,
  preproc_int_t *val, int *is_unsigned)
{
  int op;
  preproc_int_t rval;
  int rtype;
  int result;

  result = preproc_evaluate_bitshift(info, tokens, val, is_unsigned);
  while ((result & VTK_PARSE_FATAL_ERROR) == 0)
    {
    op = tokens->tok;
    if (op != '<' && op != '>' && op != TOK_LE && op != TOK_GE)
      {
      return result;
      }

    preproc_next(tokens);

    result = preproc_evaluate_bitshift(info, tokens, &rval, &rtype);

    *is_unsigned = (*is_unsigned || rtype);

    if (*is_unsigned)
      {
      if (op == TOK_LE)
        {
        *val = ((preproc_uint_t)*val <= (preproc_uint_t)rval);
        }
      else if (op == '<')
        {
        *val = ((preproc_uint_t)*val < (preproc_uint_t)rval);
        }
      else if (op == TOK_GE)
        {
        *val = ((preproc_uint_t)*val >= (preproc_uint_t)rval);
        }
      else if (op == '>')
        {
        *val = ((preproc_uint_t)*val > (preproc_uint_t)rval);
        }
      }
    else
      {
      if (op == TOK_LE)
        {
        *val = (*val <= rval);
        }
      else if (op == '<')
        {
        *val = (*val < rval);
        }
      else if (op == TOK_GE)
        {
        *val = (*val >= rval);
        }
      else if (op == '>')
        {
        *val = (*val > rval);
        }
      }
    *is_unsigned = 0;
    }

  return result;
}

static int preproc_evaluate_equal(
  PreprocessInfo *info, preproc_tokenizer *tokens,
  preproc_int_t *val, int *is_unsigned)
{
  int op;
  preproc_int_t rval;
  int rtype;
  int result;

  result = preproc_evaluate_compare(info, tokens, val, is_unsigned);
  while ((result & VTK_PARSE_FATAL_ERROR) == 0)
    {
    op = tokens->tok;
    if (op != TOK_EQ && op != TOK_NE)
      {
      return result;
      }

    preproc_next(tokens);

    result = preproc_evaluate_compare(info, tokens, &rval, &rtype);

    if (op == TOK_EQ)
      {
      *val = (*val == rval);
      }
    else if (op == TOK_NE)
      {
      *val = (*val != rval);
      }
    *is_unsigned = 0;
    }

  return result;
}

static int preproc_evaluate_and(
  PreprocessInfo *info, preproc_tokenizer *tokens,
  preproc_int_t *val, int *is_unsigned)
{
  preproc_int_t rval;
  int rtype;
  int result;

  result = preproc_evaluate_equal(info, tokens, val, is_unsigned);
  while ((result & VTK_PARSE_FATAL_ERROR) == 0)
    {
    if (tokens->tok != '&')
      {
      return result;
      }

    preproc_next(tokens);

    result = preproc_evaluate_equal(info, tokens, &rval, &rtype);

    *is_unsigned = (*is_unsigned || rtype);
    *val = (*val & rval);
    }

  return result;
}

static int preproc_evaluate_xor(
  PreprocessInfo *info, preproc_tokenizer *tokens,
  preproc_int_t *val, int *is_unsigned)
{
  preproc_int_t rval;
  int rtype;
  int result;

  result = preproc_evaluate_and(info, tokens, val, is_unsigned);
  while ((result & VTK_PARSE_FATAL_ERROR) == 0)
    {
    if (tokens->tok != '^')
      {
      return result;
      }

    preproc_next(tokens);

    result = preproc_evaluate_and(info, tokens, &rval, &rtype);

    *is_unsigned = (*is_unsigned || rtype);
    *val = (*val ^ rval);
    }

  return result;
}

static int preproc_evaluate_or(
  PreprocessInfo *info, preproc_tokenizer *tokens,
  preproc_int_t *val, int *is_unsigned)
{
  preproc_int_t rval;
  int rtype;
  int result;

  result = preproc_evaluate_xor(info, tokens, val, is_unsigned);
  while ((result & VTK_PARSE_FATAL_ERROR) == 0)
    {
    if (tokens->tok != '|')
      {
      return result;
      }

    preproc_next(tokens);

    result = preproc_evaluate_xor(info, tokens, &rval, &rtype);

    *is_unsigned = (*is_unsigned || rtype);
    *val = (*val | rval);
    }

  return result;
}

static int preproc_evaluate_logic_and(
  PreprocessInfo *info, preproc_tokenizer *tokens,
  preproc_int_t *val, int *is_unsigned)
{
  preproc_int_t rval;
  int rtype;
  int result;

  result = preproc_evaluate_or(info, tokens, val, is_unsigned);
  while ((result & VTK_PARSE_FATAL_ERROR) == 0)
    {
    if (tokens->tok != TOK_AND)
      {
      return result;
      }

    preproc_next(tokens);

    if (*val == 0)
      {
      /* short circuit */
      while (tokens->tok != 0 && tokens->tok != ')' &&
             tokens->tok != ':' && tokens->tok != '?' &&
             tokens->tok != ',' && tokens->tok != TOK_OR)
        {
        if (tokens->tok == '(')
          {
          if (preproc_skip_parentheses(tokens) != VTK_PARSE_OK)
            {
#if PREPROC_DEBUG
            fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
            result = VTK_PARSE_SYNTAX_ERROR;
            }
          }
        else
          {
          preproc_next(tokens);
          }
        }

      *is_unsigned = 0;

      return result;
      }

    result = preproc_evaluate_or(info, tokens, &rval, &rtype);

    *is_unsigned = 0;
    *val = (rval != 0);
    }

  return result;
}

static int preproc_evaluate_logic_or(
  PreprocessInfo *info, preproc_tokenizer *tokens,
  preproc_int_t *val, int *is_unsigned)
{
  preproc_int_t rval;
  int rtype;
  int result;

  result = preproc_evaluate_logic_and(info, tokens, val, is_unsigned);
  while ((result & VTK_PARSE_FATAL_ERROR) == 0)
    {
    if (tokens->tok != TOK_OR)
      {
      return result;
      }

    preproc_next(tokens);

    if (*val != 0)
      {
      /* short circuit */
      while (tokens->tok != 0 && tokens->tok != ')' &&
             tokens->tok != ':' && tokens->tok != '?' &&
             tokens->tok != ',')
        {
        if (tokens->tok == '(')
          {
          if (preproc_skip_parentheses(tokens) != VTK_PARSE_OK)
            {
#if PREPROC_DEBUG
            fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
            result = VTK_PARSE_SYNTAX_ERROR;
            }
          }
        else
          {
          preproc_next(tokens);
          }
        }

      *is_unsigned = 0;

      return result;
      }

    result = preproc_evaluate_logic_and(info, tokens, &rval, &rtype);

    *is_unsigned = 0;
    *val = (rval != 0);
    }

  return result;
}

/** Evaluate an arimetic *expression.  */
int preproc_evaluate_expression(
  PreprocessInfo *info, preproc_tokenizer *tokens,
  preproc_int_t *val, int *is_unsigned)
{
  preproc_int_t rval, sval;
  int rtype, stype;
  int result;

  result = preproc_evaluate_logic_or(info, tokens, val, is_unsigned);
  if ((result & VTK_PARSE_FATAL_ERROR) == 0)
    {
    if (tokens->tok != '?')
      {
      return result;
      }

    preproc_next(tokens);

    result = preproc_evaluate_expression(info, tokens, &rval, &rtype);
    if ((result & VTK_PARSE_FATAL_ERROR) != 0)
      {
      return result;
      }

    if (tokens->tok != ':')
      {
#if PREPROC_DEBUG
      fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
      return VTK_PARSE_SYNTAX_ERROR;
      }

    preproc_next(tokens);

    result = preproc_evaluate_expression(info, tokens, &sval, &stype);
    if ((result & VTK_PARSE_FATAL_ERROR) != 0)
      {
      return result;
      }

    if (*val != 0)
      {
      *val = rval;
      *is_unsigned = rtype;
      }
    else
      {
      *val = sval;
      *is_unsigned = stype;
      }
    }

  return result;
}

/** Evaluate a conditional *expression.
 * Returns VTK_PARSE_OK if the expression is true,
 * or VTK_PARSE_SKIP of the expression is false. */
int preproc_evaluate_conditional(
  PreprocessInfo *info, preproc_tokenizer *tokens)
{
  preproc_int_t rval;
  int rtype;
  int result;

  result = preproc_evaluate_expression(info, tokens, &rval, &rtype);
  if ((result & VTK_PARSE_FATAL_ERROR) == 0)
    {
    if (tokens->tok != 0)
      {
#if PREPROC_DEBUG
      fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
      return VTK_PARSE_SYNTAX_ERROR;
      }
    return (rval == 0 ? VTK_PARSE_SKIP : VTK_PARSE_OK);
    }

  return result;
}

/**
 * Handle any of the following directives:
 * #if, #ifdef, #ifndef, #elif, #else, #endif
 * A return value of VTK_PARSE_SKIP means that
 * the following code block should be skipped.
 */
static int preproc_evaluate_if(
  PreprocessInfo *info, preproc_tokenizer *tokens)
{
  MacroInfo *macro;
  int v1, v2;
  int result = VTK_PARSE_OK;

  if (tokens->hash == HASH_IF ||
      tokens->hash == HASH_IFDEF ||
      tokens->hash == HASH_IFNDEF)
    {
    if (info->ConditionalDepth == 0)
      {
      if (tokens->hash == HASH_IF)
        {
        preproc_next(tokens);
        result = preproc_evaluate_conditional(info, tokens);
        }
      else
        {
        v1 = (tokens->hash != HASH_IFNDEF);
        preproc_next(tokens);
        if (tokens->tok != TOK_ID)
          {
#if PREPROC_DEBUG
          fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
          return VTK_PARSE_SYNTAX_ERROR;
          }
        macro = preproc_find_macro(info, tokens);
        v2 = (macro && !macro->IsExcluded);
        preproc_next(tokens);
        result = ( (v1 ^ v2) ? VTK_PARSE_SKIP : VTK_PARSE_OK);
        }

      if (result != VTK_PARSE_SKIP)
        {
        /* mark as done, so that the "else" clause is skipped */
        info->ConditionalDone = 1;
        }
      else
        {
        /* mark as not done, so that "else" clause is not skipped */
        info->ConditionalDone = 0;
        /* skip the "if" clause */
        info->ConditionalDepth = 1;
        }
      }
    else
      {
      /* increase the skip depth */
      info->ConditionalDepth++;
      }
    }
  else if (tokens->hash == HASH_ELIF ||
           tokens->hash == HASH_ELSE)
    {
    if (info->ConditionalDepth == 0)
      {
      /* preceding clause was not skipped, so must skip this one */
      info->ConditionalDepth = 1;
      }
    else if (info->ConditionalDepth == 1 &&
             info->ConditionalDone == 0)
      {
      if (tokens->hash == HASH_ELIF)
        {
        preproc_next(tokens);
        result = preproc_evaluate_conditional(info, tokens);
        }
      else
        {
        preproc_next(tokens);
        }
      if (result != VTK_PARSE_SKIP)
        {
        /* do not skip this clause */
        info->ConditionalDepth = 0;
        /* make sure remaining else/elif clauses are skipped */
        info->ConditionalDone = 1;
        }
      }
    }
  else if (tokens->hash == HASH_ENDIF)
    {
    preproc_next(tokens);
    if (info->ConditionalDepth > 0)
      {
      /* decrease the skip depth */
      info->ConditionalDepth--;
      }
    if (info->ConditionalDepth == 0)
      {
      /* set "done" flag for the context that is being returned to */
      info->ConditionalDone = 1;
      }
    }

  return result;
}

/**
 * Handle the #define and #undef directives.
 */
static int preproc_evaluate_define(
  PreprocessInfo *info, preproc_tokenizer *tokens)
{
  MacroInfo **macro_p;
  MacroInfo *macro;
  int is_function;
  const char *name;
  size_t namelen;
  const char *definition = 0;
  int n = 0;
  const char **params = NULL;

  if (tokens->hash == HASH_DEFINE)
    {
    preproc_next(tokens);
    if (tokens->tok != TOK_ID)
      {
#if PREPROC_DEBUG
      fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
      return VTK_PARSE_SYNTAX_ERROR;
      }

    macro_p = preproc_macro_location(info, tokens, 1);
    name = tokens->text;
    namelen = tokens->len;
    preproc_next(tokens);

    is_function = 0;
    if (name[namelen] == '(')
      {
      is_function = 1;
      preproc_next(tokens);
      while (tokens->tok != 0 && tokens->tok != ')')
        {
        if (tokens->tok != TOK_ID && tokens->tok != TOK_ELLIPSIS)
          {
          if (params) { free((char **)params); }
#if PREPROC_DEBUG
          fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
          return VTK_PARSE_SYNTAX_ERROR;
          }

        /* add to the arg list */
        params = (const char **)preproc_array_check(
          (char **)params, sizeof(char *), n);
        params[n++] = preproc_strndup(tokens->text, tokens->len);

        preproc_next(tokens);
        if (tokens->tok == ',')
          {
          preproc_next(tokens);
          }
        else if (tokens->tok != ')')
          {
          if (params) { free((char **)params); }
#if PREPROC_DEBUG
          fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
          return VTK_PARSE_SYNTAX_ERROR;
          }
        }
      preproc_next(tokens);
      }

    if (tokens->tok)
      {
      definition = tokens->text;
      }

    macro = *macro_p;
    if (macro)
      {
      if (preproc_identical(macro->Definition, definition))
        {
        return VTK_PARSE_OK;
        }
      if (params) { free((char **)params); }
#if PREPROC_DEBUG
      fprintf(stderr, "macro redefined %d\n", __LINE__);
#endif
      return VTK_PARSE_MACRO_REDEFINED;
      }

    macro = preproc_new_macro(info, name, definition);
    macro->IsFunction = is_function;
    macro->NumberOfParameters = n;
    macro->Parameters = params;
    *macro_p = macro;

    return VTK_PARSE_OK;
    }
  else if (tokens->hash == HASH_UNDEF)
    {
    preproc_next(tokens);
    if (tokens->tok != TOK_ID)
      {
#if PREPROC_DEBUG
      fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
      return VTK_PARSE_SYNTAX_ERROR;
      }
    preproc_remove_macro(info, tokens);
    return VTK_PARSE_OK;
    }

  return VTK_PARSE_OK;
}

/**
 * Add an include file to the list.  Return 0 if it is already there.
 */
static int preproc_add_include_file(PreprocessInfo *info, const char *name)
{
  int i, n;
  char *dp;

  n = info->NumberOfIncludeFiles;
  for (i = 0; i < n; i++)
    {
    if (strcmp(info->IncludeFiles[i], name) == 0)
      {
      return 0;
      }
    }

  dp = (char *)malloc(strlen(name)+1);
  strcpy(dp, name);

  info->IncludeFiles = (const char **)preproc_array_check(
    (char **)info->IncludeFiles, sizeof(char *), info->NumberOfIncludeFiles);
  info->IncludeFiles[info->NumberOfIncludeFiles++] = dp;

  return 1;
}

/**
 * Find an include file.  If "cache_only" is set, then do a check to
 * see if the file was previously found without going to the filesystem.
 */
const char *preproc_find_include_file(
  PreprocessInfo *info, const char *filename, int system_first,
  int cache_only)
{
  int i, n, ii, nn;
  size_t j, m;
  struct stat fs;
  const char *directory;
  char *output;
  size_t outputsize = 16;
  int count;

  /* allow filename to be terminated by quote or bracket */
  m = 0;
  while (filename[m] != '\"' && filename[m] != '>' &&
         filename[m] != '\n' && filename[m] != '\0') { m++; }

  /* search file system for the file */
  output = (char *)malloc(outputsize);

  if (system_first != 0)
    {
    system_first = 1;
    }

  if (cache_only != 0)
    {
    cache_only = 1;
    }

  /* check for absolute path of form DRIVE: or /path/to/file */
  j = 0;
  while (preproc_chartype(filename[j], CPRE_IDGIT)) { j++; }

  if (filename[j] == ':' || filename[0] == '/' || filename[0] == '\\')
    {
    if (m+1 > outputsize)
      {
      outputsize += m+1;
      output = (char *)realloc(output, outputsize);
      }
    strncpy(output, filename, m);
    output[m] = '\0';

    nn = info->NumberOfIncludeFiles;
    for (ii = 0; ii < nn; ii++)
      {
      if (strcmp(output, info->IncludeFiles[ii]) == 0)
        {
        free(output);
        return info->IncludeFiles[ii];
        }
      }

    if (cache_only)
      {
      free(output);
      return NULL;
      }

    info->IncludeFiles = (const char **)preproc_array_check(
      (char **)info->IncludeFiles, sizeof(char *),
      info->NumberOfIncludeFiles);
    info->IncludeFiles[info->NumberOfIncludeFiles++] = output;

    return output;
    }

  /* Make sure the current filename is already added */
  if (info->FileName)
    {
    preproc_add_include_file(info, info->FileName);
    }

  /* Check twice. First check the cache, then stat the files. */
  for (count = 0; count < (2-cache_only); count++)
    {
    n = info->NumberOfIncludeDirectories;
    for (i = 0; i < (n+1-system_first); i++)
      {
      /* search the directory of the file being processed */
      if (i == 0 && system_first == 0)
        {
        if (info->FileName)
          {
          j = strlen(info->FileName);
          while (j > 0)
            {
            if (info->FileName[j-1] == '/') { break; }
            j--;
            }
          if (m+j+1 > outputsize)
            {
            outputsize += m+j+1;
            output = (char *)realloc(output, outputsize);
            }
          if (j > 0)
            {
            strncpy(output, info->FileName, j);
            }
          strncpy(&output[j], filename, m);
          output[j+m] = '\0';
          }
        else
          {
          if (m+1 > outputsize)
            {
            outputsize += m+1;
            output = (char *)realloc(output, outputsize);
            }
          strncpy(output, filename, m);
          output[m] = '\0';
          }
        }
      /* check all the search paths */
      else
        {
        directory = info->IncludeDirectories[i-1+system_first];
        j = strlen(directory);
        if (j + m + 2 > outputsize)
          {
          outputsize += j+m+2;
          output = (char *)realloc(output, outputsize);
          }

        strncpy(output, directory, j);
        if (directory[j-1] != '/') { output[j++] = '/'; }
        strncpy(&output[j], filename, m);
        output[j+m] = '\0';
        }

      if (count == 0)
        {
        nn = info->NumberOfIncludeFiles;
        for (ii = 0; ii < nn; ii++)
          {
          if (strcmp(output, info->IncludeFiles[ii]) == 0)
            {
            free(output);
            return info->IncludeFiles[ii];
            }
          }
        }
      else if (stat(output, &fs) == 0)
        {
        info->IncludeFiles = (const char **)preproc_array_check(
          (char **)info->IncludeFiles, sizeof(char *),
          info->NumberOfIncludeFiles);
        info->IncludeFiles[info->NumberOfIncludeFiles++] = output;

        return output;
        }
      }
    }

  free(output);
  return NULL;
}

/**
 * Include a file.  All macros defined in the included file
 * will have their IsExternal flag set.
 */
static int preproc_include_file(
  PreprocessInfo *info, const char *filename, int system_first)
{
  char *tbuf;
  size_t tbuflen = FILE_BUFFER_SIZE;
  char *line;
  size_t linelen = 80;
  size_t i, j, n, r;
  int in_comment = 0;
  int in_quote = 0;
  int result = VTK_PARSE_OK;
  FILE *fp = NULL;
  const char *path = NULL;
  const char *save_filename;
  int save_external;

  /* check to see if the file has aleady been included */
  path = preproc_find_include_file(info, filename, system_first, 1);
  if (path != 0)
    {
#if PREPROC_DEBUG
    int k = 0;
    while (filename[k] != '>' && filename[k] != '\"' &&
           filename[k] != '\n' && filename[k] != '\0') { k++; }
    if (filename[k] == '>')
      fprintf(stderr, "already loaded file <%*.*s>\n", k, k, filename);
    else
      fprintf(stderr, "already loaded file \"%*.*s\"\n", k, k, filename);
#endif

    return VTK_PARSE_OK;
    }
  /* go to the filesystem */
  path = preproc_find_include_file(info, filename, system_first, 0);
  if (path == NULL)
    {
#if PREPROC_DEBUG
    int k = 0;
    while (filename[k] != '>' && filename[k] != '\"' &&
           filename[k] != '\n' && filename[k] != '\0') { k++; }
    if (filename[k] == '>')
      fprintf(stderr, "couldn't find file <%*.*s>\n", k, k, filename);
    else
      fprintf(stderr, "couldn't find file \"%*.*s\"\n", k, k, filename);
#endif
    return VTK_PARSE_FILE_NOT_FOUND;
    }

#if PREPROC_DEBUG
  fprintf(stderr, "including file %s\n", path);
#endif
  fp = fopen(path, "r");

  if (fp == NULL)
    {
#if PREPROC_DEBUG
    fprintf(stderr, "couldn't open file %s\n", path);
#endif
    return VTK_PARSE_FILE_OPEN_ERROR;
    }

  save_external = info->IsExternal;
  save_filename = info->FileName;
  info->IsExternal = 1;
  info->FileName = path;

  tbuf = (char *)malloc(tbuflen+4);
  line = (char *)malloc(linelen);

  /* the buffer must hold a whole line for it to be processed */
  j = 0;
  i = 0;
  n = 0;
  r = 0;

  do
    {
    if (i >= n)
      {
      /* recycle unused lookahead chars */
      if (r)
        {
        r = n + 2 - i;
        if (r == 2)
          {
          tbuf[0] = tbuf[tbuflen-2];
          tbuf[1] = tbuf[tbuflen-1];
          }
        else if (r == 1)
          {
          tbuf[0] = tbuf[tbuflen-1];
          }
        }

      /* read the next chunk of the file */
      i = 0;
      if (feof(fp))
        {
        /* still have the lookahead chars left */
        n = r;
        r = 0;
        }
      else
        {
        /* fill the remainder of the buffer */
        errno = 0;
        tbuflen = r + FILE_BUFFER_SIZE;
        while ((n = fread(&tbuf[r], 1, tbuflen-r, fp)) == 0 && ferror(fp))
          {
          if (errno != EINTR)
            {
            fclose(fp);
            free(tbuf);
            free(line);
            info->IsExternal = save_external;
            return VTK_PARSE_FILE_READ_ERROR;
            }
          errno = 0;
          clearerr(fp);
          }

        if (n + r < tbuflen)
          {
          /* this only occurs if the final fread does not fill the buffer */
          n += r;
          r = 0;
          }
        else
          {
          /* set a lookahead reserve of two chars */
          n -= (2 - r);
          r = 2;
          }

        /* guard against lookahead past last char in file */
        tbuf[n + r] = '\0';
        }
      }

    /* copy the characters until end of line is found */
    while (i < n)
      {
      /* expand line buffer as necessary */
      while (j+4 > linelen)
        {
        linelen *= 2;
        line = (char *)realloc(line, linelen);
        }

      if (in_comment)
        {
        if (tbuf[i] == '*' && tbuf[i+1] == '/')
          {
          line[j++] = tbuf[i++];
          line[j++] = tbuf[i++];
          in_comment = 0;
          }
        else
          {
          line[j++] = tbuf[i++];
          }
        }
      else if (in_quote)
        {
        if (tbuf[i] == '\"')
          {
          line[j++] = tbuf[i++];
          in_quote = 0;
          }
        else if (tbuf[i] == '\\' && tbuf[i+1] != '\0')
          {
          line[j++] = tbuf[i++];
          line[j++] = tbuf[i++];
          }
        else
          {
          line[j++] = tbuf[i++];
          }
        }
      else if (tbuf[i] == '/' && tbuf[i+1] == '*')
        {
        line[j++] = tbuf[i++];
        line[j++] = tbuf[i++];
        in_comment = 1;
        }
      else if (tbuf[i] == '\"')
        {
        line[j++] = tbuf[i++];
        in_quote = 1;
        }
      else if (tbuf[i] == '\\' && tbuf[i+1] == '\n')
        {
        line[j++] = tbuf[i++];
        line[j++] = tbuf[i++];
        }
      else if (tbuf[i] == '\\' && tbuf[i+1] == '\r' && tbuf[i+2] == '\n')
        {
        line[j++] = tbuf[i++];
        line[j++] = tbuf[i++];
        line[j++] = tbuf[i++];
        }
      else if (tbuf[i] != '\n' && tbuf[i] != '\0')
        {
        line[j++] = tbuf[i++];
        }
      else
        {
        line[j++] = tbuf[i++];
        break;
        }
      }

    if (i < n || n == 0)
      {
      const char *cp = line;
      line[j] = '\0';
      j = 0;
      preproc_skip_whitespace(&cp, WS_NO_EOL);
      if (*cp == '#')
        {
        vtkParsePreprocess_HandleDirective(info, line);
        }
      }
    }
  while (n > 0);

  free(tbuf);
  free(line);
  fclose(fp);

  info->IsExternal = save_external;
  info->FileName = save_filename;

  return result;
}

/**
 * Handle the #include directive.  The header file will
 * only go through the preprocessor.
 */
static int preproc_evaluate_include(
  PreprocessInfo *info, preproc_tokenizer *tokens)
{
  const char *cp;
  const char *filename;

  if (tokens->hash == HASH_INCLUDE)
    {
    preproc_next(tokens);

    cp = tokens->text;

    if (tokens->tok == TOK_ID)
      {
      MacroInfo *macro = preproc_find_macro(info, tokens);
      if (macro && !macro->IsExcluded && macro->Definition)
        {
        cp = macro->Definition;
        }
      else
        {
#if PREPROC_DEBUG
        fprintf(stderr, "couldn't find macro %*.*s.\n",
                (int)tokens->len, (int)tokens->len, tokens->text);
#endif
        return VTK_PARSE_MACRO_UNDEFINED;
        }
      }

    if (*cp == '\"')
      {
      filename = cp + 1;
      preproc_skip_quotes(&cp);
      if (cp <= filename + 1 || *(cp-1) != '\"')
        {
        return VTK_PARSE_SYNTAX_ERROR;
        }

      return preproc_include_file(info, filename, 0);
      }
    else if (*cp == '<')
      {
      cp++;
      filename = cp;
      while (*cp != '>' && *cp != '\n' && *cp != '\0') { cp++; }
      if (*cp != '>')
        {
        return VTK_PARSE_SYNTAX_ERROR;
        }

      return preproc_include_file(info, filename, 1);
      }
    }

  return VTK_PARSE_OK;
}

/**
 * Handle any recognized directive.
 * Unrecognized directives are ignored.
 */
int vtkParsePreprocess_HandleDirective(
  PreprocessInfo *info, const char *directive)
{
  int result = VTK_PARSE_OK;
  preproc_tokenizer tokens;

  preproc_init(&tokens, directive);

  if (tokens.tok != '#')
    {
    return VTK_PARSE_SYNTAX_ERROR;
    }

  preproc_next(&tokens);

  if (tokens.tok == TOK_ID)
    {
    if ((tokens.hash == HASH_IFDEF && tokens.len == 5 &&
         strncmp("ifdef", tokens.text, tokens.len) == 0) ||
        (tokens.hash == HASH_IFNDEF && tokens.len == 6 &&
         strncmp("ifndef", tokens.text, tokens.len) == 0) ||
        (tokens.hash == HASH_IF && tokens.len == 2 &&
         strncmp("if", tokens.text, tokens.len) == 0) ||
        (tokens.hash == HASH_ELIF && tokens.len == 4 &&
         strncmp("elif", tokens.text, tokens.len) == 0) ||
        (tokens.hash == HASH_ELSE && tokens.len == 4 &&
         strncmp("else", tokens.text, tokens.len) == 0) ||
        (tokens.hash == HASH_ENDIF && tokens.len == 5 &&
         strncmp("endif", tokens.text, tokens.len) == 0))
      {
      result = preproc_evaluate_if(info, &tokens);
      while (tokens.tok) { preproc_next(&tokens); }
#if PREPROC_DEBUG
        {
        size_t n = tokens.text - directive;

        if (result == VTK_PARSE_SKIP)
          {
          fprintf(stderr, "SKIP: ");
          }
        else if (result == VTK_PARSE_OK)
          {
          fprintf(stderr, "READ: ");
          }
        else
          {
          fprintf(stderr, "ERR%-2.2d ", result);
          }
        fprintf(stderr, "%*.*s\n", (int)n, (int)n, directive);
        }
#endif
      }
    else if (info->ConditionalDepth == 0)
      {
      if ((tokens.hash == HASH_DEFINE && tokens.len == 6 &&
           strncmp("define", tokens.text, tokens.len) == 0) ||
          (tokens.hash == HASH_UNDEF && tokens.len == 5 &&
           strncmp("undef", tokens.text, tokens.len) == 0))
        {
        result = preproc_evaluate_define(info, &tokens);
        }
      else if (tokens.hash == HASH_INCLUDE && tokens.len == 7 &&
               strncmp("include", tokens.text, tokens.len) == 0)
        {
        result = preproc_evaluate_include(info, &tokens);
        }
      }
    }

  if (info->ConditionalDepth > 0)
    {
    return VTK_PARSE_SKIP;
    }

  return result;
}

/**
 * Evaluate a preprocessor expression.
 * If no errors occurred, the result will be VTK_PARSE_OK.
 */
int vtkParsePreprocess_EvaluateExpression(
  PreprocessInfo *info, const char *text,
  preproc_int_t *val, int *is_unsigned)
{
  preproc_tokenizer tokens;
  preproc_init(&tokens, text);

  return preproc_evaluate_expression(info, &tokens, val, is_unsigned);
}

/** Add a macro for defining a macro */
#define PREPROC_MACRO_TO_STRING2(x) #x
#define PREPROC_MACRO_TO_STRING(x) PREPROC_MACRO_TO_STRING2(x)
#define PREPROC_ADD_MACRO(info, x) \
preproc_add_macro_definition(info, #x, PREPROC_MACRO_TO_STRING2(x))

/**
 * Add all standard preprocessory macros.  Specify the platform.
 */
void vtkParsePreprocess_AddStandardMacros(
  PreprocessInfo *info, int platform)
{
  int save_external = info->IsExternal;
  info->IsExternal = 1;

  /* a special macro to indicate that this is the wrapper */
  preproc_add_macro_definition(info, "__WRAP__", "1");

  /* language macros - assume that we are wrapping C++ code */
  preproc_add_macro_definition(info, "__cplusplus", "1");

  /* stdc version macros */
#ifdef __STDC__
  PREPROC_ADD_MACRO(info, __STDC__);
#endif
#ifdef __STDC_VERSION__
  PREPROC_ADD_MACRO(info, __STDC_VERSION__);
#endif
#ifdef __STDC_HOSTED__
  PREPROC_ADD_MACRO(info, __STDC_HOSTED__);
#endif

  if (platform == VTK_PARSE_NATIVE)
    {
#ifdef WIN32
    PREPROC_ADD_MACRO(info, WIN32);
#endif
#ifdef _WIN32
    PREPROC_ADD_MACRO(info, _WIN32);
#endif
#ifdef _MSC_VER
    PREPROC_ADD_MACRO(info, _MSC_VER);
#endif

#ifdef __BORLAND__
    PREPROC_ADD_MACRO(info, __BORLAND__);
#endif

#ifdef __CYGWIN__
    PREPROC_ADD_MACRO(info, __CYGWIN__);
#endif
#ifdef MINGW
    PREPROC_ADD_MACRO(info, MINGW);
#endif
#ifdef __MINGW32__
    PREPROC_ADD_MACRO(info, __MINGW32__);
#endif

#ifdef __linux__
    PREPROC_ADD_MACRO(info, __linux__);
#endif
#ifdef __LINUX__
    PREPROC_ADD_MACRO(info, __LINUX__);
#endif

#ifdef __APPLE__
    PREPROC_ADD_MACRO(info, __APPLE__);
#endif
#ifdef __MACH__
    PREPROC_ADD_MACRO(info, __MACH__);
#endif
#ifdef __DARWIN__
    PREPROC_ADD_MACRO(info, __DARWIN__);
#endif

#ifdef __GNUC__
    PREPROC_ADD_MACRO(info, __GNUC__);
#endif
#ifdef __LP64__
    PREPROC_ADD_MACRO(info, __LP64__);
#endif
#ifdef __BIG_ENDIAN__
    PREPROC_ADD_MACRO(info, __BIG_ENDIAN__);
#endif
#ifdef __LITTLE_ENDIAN__
    PREPROC_ADD_MACRO(info, __LITTLE_ENDIAN__);
#endif
    }

  info->IsExternal = save_external;
}

/**
 * Add a preprocessor macro, including a definition.
 */
int vtkParsePreprocess_AddMacro(
  PreprocessInfo *info, const char *name, const char *definition)
{
  preproc_tokenizer token;
  MacroInfo **macro_p;
  MacroInfo *macro;

  preproc_init(&token, name);
  macro_p = preproc_macro_location(info, &token, 1);
  if (*macro_p)
    {
    macro = *macro_p;
    if (preproc_identical(macro->Definition, definition))
      {
      return VTK_PARSE_OK;
      }
    else
      {
      return VTK_PARSE_MACRO_REDEFINED;
      }
    }

  macro = preproc_new_macro(info, name, definition);
  macro->IsExternal = 1;
  *macro_p = macro;

  return VTK_PARSE_OK;
}

/**
 * Return a preprocessor macro struct, or NULL if not found.
 */
MacroInfo *vtkParsePreprocess_GetMacro(
  PreprocessInfo *info, const char *name)
{
  preproc_tokenizer token;
  MacroInfo *macro;

  preproc_init(&token, name);
  macro = preproc_find_macro(info, &token);

  if (macro && !macro->IsExcluded)
    {
    return macro;
    }

  return NULL;
}

/**
 * Remove a preprocessor macro.
 */
int vtkParsePreprocess_RemoveMacro(
  PreprocessInfo *info, const char *name)
{
  preproc_tokenizer token;

  preproc_init(&token, name);

  if (preproc_remove_macro(info, &token))
    {
    return VTK_PARSE_OK;
    }

  return VTK_PARSE_MACRO_UNDEFINED;
}

/**
 * Expand a macro, argstring is ignored if not a function macro
 */
const char *vtkParsePreprocess_ExpandMacro(
  PreprocessInfo *info, MacroInfo *macro, const char *argstring)
{
  const char *cp = argstring;
  int n = 0;
  int j = 0;
  const char *stack_values[8];
  const char **values = NULL;
  const char *pp = NULL;
  const char *dp = NULL;
  const char *wp = NULL;
  char stack_rp[128];
  char *rp = NULL;
  size_t rs = 0;
  size_t i = 0;
  size_t l = 0;
  size_t k = 0;
  int stringify = 0;
  int noexpand = 0;
  int depth = 1;
  int c;

  if (macro->IsFunction)
    {
    if (argstring == NULL || *cp != '(')
      {
      return NULL;
      }

    /* break the string into individual argument values */
    values = stack_values;

    cp++;
    values[n++] = cp;
    while (depth > 0 && *cp != '\0')
      {
      while (*cp != '\0')
        {
        if (*cp == '\"' || *cp == '\'')
          {
          preproc_skip_quotes(&cp);
          }
        else if (cp[0] == '/' && (cp[1] == '*' || cp[1] == '/'))
          {
          preproc_skip_comment(&cp);
          }
        else if (*cp == '(')
          {
          cp++;
          depth++;
          }
        else if (*cp == ')')
          {
          cp++;
          if (--depth == 0)
            {
            break;
            }
          }
        else if (*cp == ',')
          {
          cp++;
          if (depth == 1)
            {
            break;
            }
          }
        else if (*cp != '\0')
          {
          cp++;
          }
        }
      if (n >= 8 && (n & (n-1)) == 0)
        {
        if (values != stack_values)
          {
          values = (const char **)realloc(
            (char **)values, 2*n*sizeof(const char **));
          }
        else
          {
          values = (const char **)malloc(2*n*sizeof(const char **));
          memcpy((char **)values, stack_values, 8*sizeof(const char **));
          }
        }

      values[n++] = cp;
      }
    --n;

    /* diagnostic: print out the values */
#if PREPROC_DEBUG
    for (j = 0; j < n; j++)
      {
      size_t m = values[j+1] - values[j] - 1;
      fprintf(stderr, "arg %i: %*.*s\n",
              (int)j, (int)m, (int)m, values[j]);
      }
#endif

    if (macro->NumberOfParameters == 0 && n == 1)
      {
      const char *tp = values[0];
      preproc_skip_whitespace(&tp, WS_NO_EOL);
      if (tp + 1 >= values[1])
        {
        n = 0;
        }
      }

    if (n != macro->NumberOfParameters)
      {
      if (values != stack_values) { free((char **)values); }
#if PREPROC_DEBUG
      fprintf(stderr, "wrong number of macro args to %s, %lu != %lu\n",
              macro->Name, n, macro->NumberOfParameters);
#endif
      return NULL;
      }
    }

  cp = macro->Definition;
  cp = (cp ? cp : "");
  dp = cp;
  rp = stack_rp;
  rp[0] = '\0';
  rs = 128;

  while (*cp != '\0')
    {
    pp = cp;
    wp = cp;
    stringify = 0;
    noexpand = 0;
    /* skip all chars that aren't part of a name */
    while (!preproc_chartype(*cp, CPRE_ID) && *cp != '\0')
      {
      dp = cp;
      preproc_skip_whitespace(&cp, WS_NO_EOL);
      if (cp > dp)
        {
        dp = cp;
        }
      else if (preproc_chartype(*cp, CPRE_QUOTE))
        {
        preproc_skip_quotes(&cp);
        dp = cp;
        wp = cp;
        noexpand = 0;
        }
      else if (preproc_chartype(*cp, CPRE_DIGIT))
        {
        preproc_skip_number(&cp);
        dp = cp;
        wp = cp;
        noexpand = 0;
        }
      else if (cp[0] == '#' && cp[1] == '#')
        {
        noexpand = 1;
        dp = wp;
        cp += 2;
        wp = cp;
        preproc_skip_whitespace(&cp, WS_NO_EOL);
        break;
        }
      else if (*cp == '#')
        {
        stringify = 1;
        dp = cp;
        wp = cp;
        cp++;
        preproc_skip_whitespace(&cp, WS_NO_EOL);
        break;
        }
      else
        {
        cp++;
        dp = cp;
        wp = cp;
        }
      }
    l = dp - pp;
    if (l > 0)
      {
      if (i + l + 1 >= rs)
        {
        rs += rs + i + l + 1;
        if (rp != stack_rp)
          {
          rp = (char *)realloc(rp, rs);
          }
        else
          {
          rp = (char *)malloc(rs);
          memcpy(rp, stack_rp, i);
          }
        }
      strncpy(&rp[i], pp, l);
      i += l;
      rp[i] = '\0';
      }

    /* get the name */
    pp = cp;
    preproc_skip_name(&cp);
    l = cp - pp;
    if (l > 0)
      {
      for (j = 0; j < n; j++)
        {
        /* check whether the name matches a parameter */
        if (strncmp(pp, macro->Parameters[j], l) == 0 &&
            macro->Parameters[j][l] == '\0')
          {
          /* substitute the argument value */
          l = values[j+1] - values[j] - 1;
          pp = values[j];
          /* remove leading whitespace from argument */
          c = *pp;
          while (preproc_chartype(c, CPRE_WHITE))
            {
            c = *(++pp);
            l--;
            }
          /* remove trailing whitespace from argument */
          if (l > 0)
            {
            c = pp[l - 1];
            while (preproc_chartype(c, CPRE_WHITE))
              {
              if (--l == 0)
                {
                break;
                }
              c = pp[l-1];
              }
            }
          /* check if followed by "##" */
          wp = cp;
          preproc_skip_whitespace(&wp, WS_NO_EOL);
          if (wp[0] == '#' && wp[1] == '#')
            {
            noexpand = 1;
            }
          break;
          }
        }
      if (stringify)
        {
        /* compute number of chars that will be added */
        stringify = 2;
        for (k = 0; k < l; k++)
          {
          c = pp[k];
          if (c == '\\' || c == '\"')
            {
            stringify++;
            }
          }
        }
      if (i + l + stringify + 1 >= rs)
        {
        rs += rs + i + l + 1;
        if (rp != stack_rp)
          {
          rp = (char *)realloc(rp, rs);
          }
        else
          {
          rp = (char *)malloc(rs);
          memcpy(rp, stack_rp, i);
          }
        }
      if (stringify)
        {
        rp[i++] = '\"';
        for (k = 0; k < l; k++)
          {
          c = pp[k];
          if (c == '\\' || c == '\"')
            {
            rp[i++] = '\\';
            }
          rp[i++] = c;
          }
        rp[i++] = '\"';
        }
      else if (noexpand)
        {
        strncpy(&rp[i], pp, l);
        i += l;
        }
      else
        {
        /* process the arguments before substituting them */
        const char *text;
        int is_excluded = macro->IsExcluded;
        macro->IsExcluded = 1;
        strncpy(&rp[i], pp, l);
        rp[i + l] = '\0';
        text = vtkParsePreprocess_ProcessString(info, &rp[i]);
        if (text)
          {
          l = strlen(text);
          if (text != &rp[i])
            {
            char *tp = NULL;
            if (i + l + 1 >= rs)
              {
              rs += rs + i + l + 1;
              tp = rp;
              rp = (char *)malloc(rs);
              memcpy(rp, tp, i);
              }
            strncpy(&rp[i], text, l);
            vtkParsePreprocess_FreeProcessedString(info, text);
            if (tp && tp != stack_rp)
              {
              free(tp);
              }
            }
          }
        macro->IsExcluded = is_excluded;
        i += l;
        }
      rp[i] = '\0';
      }
    }

  if (values != stack_values) { free((char **)values); }

  if (!macro->IsFunction && macro->Definition &&
      strcmp(rp, macro->Definition) == 0)
    {
    if (rp != stack_rp) { free(rp); }
    return macro->Definition;
    }

  if (rp == stack_rp)
    {
    rp = (char *)malloc(strlen(stack_rp) + 1);
    strcpy(rp, stack_rp);
    }

  return rp;
}

/**
 * Process a string
 */
const char *vtkParsePreprocess_ProcessString(
  PreprocessInfo *info, const char *text)
{
  char stack_rp[128];
  char *rp;
  char *ep;
  size_t i = 0;
  size_t rs = 128;
  int last_tok = 0;
  preproc_tokenizer tokens;
  preproc_init(&tokens, text);

  rp = stack_rp;
  rp[0] = '\0';

  while (tokens.tok)
    {
    size_t l = tokens.len;
    size_t j;
    const char *cp = tokens.text;
    const char *dp;

    if (tokens.tok == TOK_STRING && last_tok == TOK_STRING)
      {
      if (i > 0)
        {
        do { --i; } while (i > 0 && rp[i] != '\"');
        }
      cp++;
      }

    if (i + l + 2 >= rs)
      {
      rs += rs + i + l + 2;
      if (rp == stack_rp)
        {
        rp = (char *)malloc(rs);
        memcpy(rp, stack_rp, i);
        }
      else
        {
        rp = (char *)realloc(rp, rs);
        }
      }

    /* copy the token, removing backslash-newline */
    dp = cp;
    ep = &rp[i];
    for (j = 0; j < l; j++)
      {
      if (*dp == '\\')
        {
        if (dp[1] == '\n') { dp += 2; }
        else if (dp[1] == '\r' && dp[2] == '\n') { dp += 3; }
        else { *ep++ = *dp++; }
        }
      else
        {
        *ep++ = *dp++;
        }
      }
    l = ep - &rp[i];

    if (tokens.tok == TOK_ID)
      {
      MacroInfo *macro = preproc_find_macro(info, &tokens);
      if (macro && !macro->IsExcluded)
        {
        const char *args = NULL;
        int expand = 1;

        if (macro->IsFunction)
          {
          /* expand function macros using the arguments */
          preproc_next(&tokens);
          if (tokens.tok == '(')
            {
            int depth = 1;
            args = tokens.text;
            while (depth > 0 && preproc_next(&tokens))
              {
              if (tokens.tok == '(')
                {
                depth++;
                }
              else if (tokens.tok == ')')
                {
                depth--;
                }
              }
            if (tokens.tok != ')')
              {
              if (rp != stack_rp) { free(rp); }
              return NULL;
              }
            }
          else
            {
            /* unput the last token if it isn't "(" */
            tokens.len = l;
            tokens.text = cp;
            expand = 0;
            }
          }
        if (expand)
          {
          const char *expansion;
          const char *processed;
          expansion = vtkParsePreprocess_ExpandMacro(info, macro, args);
          if (expansion == NULL)
            {
            if (rp != stack_rp) { free(rp); }
            return NULL;
            }
          macro->IsExcluded = 1;
          processed = vtkParsePreprocess_ProcessString(info, expansion);
          macro->IsExcluded = 0;
          if (processed == NULL)
            {
            vtkParsePreprocess_FreeMacroExpansion(info, macro, expansion);
            if (rp != stack_rp) { free(rp); }
            return NULL;
            }
          l = strlen(processed);
          if (l > 0)
            {
            if (i + l + 2 >= rs)
              {
              rs += rs + i + l + 2;
              if (rp == stack_rp)
                {
                rp = (char *)malloc(rs);
                memcpy(rp, stack_rp, i);
                }
              else
                {
                rp = (char *)realloc(rp, rs);
                }
              }
            strncpy(&rp[i], processed, l);
            }
          if (processed != expansion)
            {
            vtkParsePreprocess_FreeProcessedString(info, processed);
            }
          vtkParsePreprocess_FreeMacroExpansion(info, macro, expansion);
          }
        }
      }

    i += l;

    last_tok = tokens.tok;
    l = tokens.len;
    cp = tokens.text;
    if (preproc_next(&tokens) && tokens.text > cp + l)
      {
      rp[i++] = ' ';
      }
    }
  rp[i] = '\0';

  if (strcmp(rp, text) == 0)
    {
    /* no change, return */
    if (rp != stack_rp) { free(rp); }
    return text;
    }
  else
    {
    /* string changed, recursively reprocess */
    const char *tp = vtkParsePreprocess_ProcessString(info, rp);
    if (rp != tp)
      {
      if (rp != stack_rp) { free(rp); }
      return tp;
      }
    if (rp == stack_rp)
      {
      rp = (char *)malloc(strlen(stack_rp) + 1);
      strcpy(rp, stack_rp);
      }
    }

  return rp;
}

/**
 * Free a string returned by ExpandMacro
 */
void vtkParsePreprocess_FreeMacroExpansion(
  PreprocessInfo *info, MacroInfo *macro, const char *text)
{
  /* only free expansion if it is different from definition */
  if (info && text != macro->Definition)
    {
    free((char *)text);
    }
}

/**
 * Free a string returned by ProcessString
 */
void vtkParsePreprocess_FreeProcessedString(
  PreprocessInfo *info, const char *text)
{
  if (info)
    {
    free((char *)text);
    }
}

/**
 * Add an include directory.
 */
void vtkParsePreprocess_IncludeDirectory(
  PreprocessInfo *info, const char *name)
{
  int i, n;

  n = info->NumberOfIncludeDirectories;
  for (i = 0; i < n; i++)
    {
    if (strcmp(name, info->IncludeDirectories[i]) == 0)
      {
      return;
      }
    }

  info->IncludeDirectories = (const char **)preproc_array_check(
    (char **)info->IncludeDirectories, sizeof(char *),
    info->NumberOfIncludeDirectories);
  info->IncludeDirectories[info->NumberOfIncludeDirectories++] =
    preproc_strndup(name, strlen(name));
}

/**
 * Find an include file in the path.  If system_first is set,
 * then the current directory is not searched.
 */
const char *vtkParsePreprocess_FindIncludeFile(
  PreprocessInfo *info, const char *filename, int system_first,
  int *already_loaded)
{
  const char *cp;
  cp = preproc_find_include_file(info, filename, system_first, 1);
  if (cp)
    {
    *already_loaded = 1;
    return cp;
    }

  *already_loaded = 0;
  return preproc_find_include_file(info, filename, system_first, 0);
}

/**
 * Initialize a preprocessor macro struct
 */
void vtkParsePreprocess_InitMacro(MacroInfo *macro)
{
  macro->Name = NULL;
  macro->Definition = NULL;
  macro->Comment = NULL;
  macro->NumberOfParameters = 0;
  macro->Parameters = NULL;
  macro->IsFunction = 0;
  macro->IsExternal = 0;
  macro->IsExcluded = 0;
}

/**
 * Free a preprocessor macro struct
 */
void vtkParsePreprocess_FreeMacro(MacroInfo *macro)
{
  int i, n;

  free((char *)macro->Name);
  free((char *)macro->Definition);
  free((char *)macro->Comment);

  n = macro->NumberOfParameters;
  for (i = 0; i < n; i++)
    {
    free((char *)macro->Parameters[i]);
    }
  free((char **)macro->Parameters);

  free(macro);
}

/**
 * Initialize a preprocessor struct
 */
void vtkParsePreprocess_Init(
  PreprocessInfo *info, const char *filename)
{
  info->FileName = NULL;
  info->MacroHashTable = NULL;
  info->NumberOfIncludeDirectories = 0;
  info->IncludeDirectories = NULL;
  info->NumberOfIncludeFiles = 0;
  info->IncludeFiles = NULL;
  info->IsExternal = 0;
  info->ConditionalDepth = 0;
  info->ConditionalDone = 0;

  if (filename)
    {
    info->FileName = preproc_strndup(filename, strlen(filename));
    }
}

/**
 * Free a preprocessor struct and its contents
 */
void vtkParsePreprocess_Free(PreprocessInfo *info)
{
  int i, n;
  MacroInfo **mptr;

  free((char *)info->FileName);

  if (info->MacroHashTable)
    {
    n = PREPROC_HASH_TABLE_SIZE;
    for (i = 0; i < n; i++)
      {
      mptr = info->MacroHashTable[i];
      if (mptr)
        {
        while (*mptr)
          {
          vtkParsePreprocess_FreeMacro(*mptr++);
          }
        }
      free(info->MacroHashTable[i]);
      }
    free(info->MacroHashTable);
    }

  n = info->NumberOfIncludeDirectories;
  for (i = 0; i < n; i++)
    {
    free((char *)info->IncludeDirectories[i]);
    }
  free((char **)info->IncludeDirectories);

  n = info->NumberOfIncludeFiles;
  for (i = 0; i < n; i++)
    {
    free((char *)info->IncludeFiles[i]);
    }
  free((char **)info->IncludeFiles);

  free(info);
}
