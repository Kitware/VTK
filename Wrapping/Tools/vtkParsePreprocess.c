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

/** Tokenize and compare two strings */
static int preproc_identical(const char *text1, const char *text2)
{
  int result = 1;

  if (text1 != text2)
  {
    result = 0;

    if (text1 && text2)
    {
      StringTokenizer t1;
      StringTokenizer t2;

      vtkParse_InitTokenizer(&t1, text1, WS_PREPROC);
      vtkParse_InitTokenizer(&t2, text2, WS_PREPROC);

      do
      {
        if (t1.tok != t2.tok ||
            t1.hash != t2.hash ||
            t1.len != t2.len ||
            strncmp(t1.text, t2.text, t1.len) != 0)
        {
          break;
        }
        vtkParse_NextToken(&t1);
        vtkParse_NextToken(&t2);
      }
      while (t1.tok && t2.tok);

      result = (t1.tok == 0 && t2.tok == 0);
    }
  }

  return result;
}

/** Create a new preprocessor macro. */
static MacroInfo *preproc_new_macro(
  PreprocessInfo *info, const char *name, const char *definition)
{
  MacroInfo *macro = (MacroInfo *)malloc(sizeof(MacroInfo));
  vtkParsePreprocess_InitMacro(macro);

  if (name)
  {
    size_t n = vtkParse_SkipId(name);
    macro->Name = vtkParse_CacheString(info->Strings, name, n);
  }

  if (definition)
  {
    size_t n;
    const char *cp = definition;
    StringTokenizer tokens;
    vtkParse_InitTokenizer(&tokens, cp, WS_PREPROC);

    do
    {
      cp = tokens.text + tokens.len;
    }
    while (vtkParse_NextToken(&tokens));

    n = cp - definition;
    macro->Definition = vtkParse_CacheString(info->Strings, definition, n);
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
  PreprocessInfo *info, StringTokenizer *token)
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
  PreprocessInfo *info, StringTokenizer *token, int insert)
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
        MacroInfo **oldhptr = hptr;
        hptr = htable[i];
        hptr = (MacroInfo **)realloc(hptr, (2*(n+1))*sizeof(MacroInfo *));
        if (!hptr)
        {
          free(oldhptr);
          return NULL;
        }
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
  PreprocessInfo *info, StringTokenizer *token)
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
  StringTokenizer token;
  MacroInfo *macro;
  MacroInfo **macro_p;

  vtkParse_InitTokenizer(&token, name, WS_PREPROC);

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
static int preproc_skip_parentheses(StringTokenizer *tokens)
{
  int depth = 0;

  if (tokens->tok == '(')
  {
    depth = 1;

    while (depth > 0 && vtkParse_NextToken(tokens))
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
    vtkParse_NextToken(tokens);
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
  size_t i = 0;
  preproc_int_t code = 0;
  int typecode = 0;

  if (cp[0] == 'u' && cp[1] == '8')
  {
    cp += 2;
  }
  else if (cp[0] == 'u' || cp[0] == 'U' || cp[0] == 'L')
  {
    typecode = cp[0];
    cp++;
  }

  if (*cp == '\'')
  {
    cp++;
    if (*cp != '\\')
    {
      code = vtkParse_DecodeUtf8(&cp, NULL);
    }
    else if (*cp != '\'' && *cp != '\n' && *cp != '\0')
    {
      cp++;
      if (*cp == 'a') { *val = '\a'; cp++; }
      else if (*cp == 'b') { *val = '\b'; cp++; }
      else if (*cp == 'f') { *val = '\f'; cp++; }
      else if (*cp == 'n') { *val = '\n'; cp++; }
      else if (*cp == 'r') { *val = '\r'; cp++; }
      else if (*cp == 't') { *val = '\t'; cp++; }
      else if (*cp == 'v') { *val = '\v'; cp++; }
      else if (*cp == '\'') { *val = '\''; cp++; }
      else if (*cp == '\"') { *val = '\"'; cp++; }
      else if (*cp == '\\') { *val = '\\'; cp++; }
      else if (*cp == '\?') { *val = '\?'; cp++; }
      else if (*cp >= '0' && *cp <= '7')
      {
        code = string_to_preproc_int(cp, 8);
        do { cp++; i++; } while (i < 4 && *cp >= '0' && *cp <= '7');
      }
      else if (*cp == 'x')
      {
        code = string_to_preproc_int(cp+1, 16);
        do { cp++; } while (vtkParse_CharType(*cp, CPRE_HEX));
      }
      else if (*cp == 'u')
      {
        code = string_to_preproc_int(cp+1, 16);
        do { cp++; i++; } while (i < 5 && vtkParse_CharType(*cp, CPRE_HEX));
        if (i != 5) { cp -= i; }
      }
      else if (*cp == 'U')
      {
        code = string_to_preproc_int(cp+1, 16);
        do { cp++; i++; } while (i < 9 && vtkParse_CharType(*cp, CPRE_HEX));
        if (i != 9) { cp -= i; }
      }
    }
    if (*cp != '\'')
    {
#if PREPROC_DEBUG
      fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
      return VTK_PARSE_SYNTAX_ERROR;
    }
    if (typecode == 0)
    {
      *val = (char)code;
    }
    else if (typecode == 'L')
    {
      *val = (wchar_t)code;
    }
    else
    {
      *val = code;
    }
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
  char temp[72];
  const char *ep;
  size_t apos = 0;
  size_t i = 0;
  int base = 0;
  ep = cp;

  if (cp[0] == '0' && (cp[1] == 'x' || cp[1] == 'X'))
  {
    cp += 2;
    base = 16;
    *is_unsigned = 1;
    ep = cp;
    while (vtkParse_CharType(*ep, CPRE_HEX))
    {
      ep++;
      if (*ep == '\'' && vtkParse_CharType(ep[1], CPRE_HEX))
      {
        apos++;
        ep += 2;
      }
    }
  }
  else if (cp[0] == '0' && (cp[1] == 'b' || cp[1] == 'B'))
  {
    cp += 2;
    base = 2;
    *is_unsigned = 1;
    ep = cp;
    while (*ep >= '0' && *ep <= '1')
    {
      ep++;
      if (*ep == '\'' && ep[1] >= '0' && ep[1] <= '1')
      {
        apos++;
        ep += 2;
      }
    }
  }
  else if (cp[0] == '0' && vtkParse_CharType(cp[1], CPRE_DIGIT))
  {
    cp += 1;
    base = 8;
    *is_unsigned = 1;
    ep = cp;
    while (*ep >= '0' && *ep <= '7')
    {
      ep++;
      if (*ep == '\'' && ep[1] >= '0' && ep[1] <= '7')
      {
        apos++;
        ep += 2;
      }
    }
  }
  else
  {
    base = 10;
    *is_unsigned = 0;
    while (vtkParse_CharType(*ep, CPRE_DIGIT))
    {
      ep++;
      if (*ep == '\'' && vtkParse_CharType(ep[1], CPRE_DIGIT))
      {
        apos++;
        ep += 2;
      }
    }
  }

  if (*ep == '.' ||
      ((ep[0] == 'e' || ep[0] == 'E') &&
       (vtkParse_CharType(ep[1], CPRE_SIGN) ||
        vtkParse_CharType(ep[1], CPRE_DIGIT))))
  {
    *val = 0;
    return VTK_PARSE_PREPROC_DOUBLE;
  }

  if (apos > 0 && ep-cp-apos < sizeof(temp))
  {
    while (cp != ep)
    {
      if (*cp != '\'')
      {
        temp[i++] = *cp;
      }
      ++cp;
    }
    temp[i++] = '\0';
    cp = temp;
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

  return VTK_PARSE_OK;
}

/* forward declaration */
static int preproc_evaluate_expression(
  PreprocessInfo *info, StringTokenizer *tokens,
  preproc_int_t *val, int *is_unsigned);

/** Evaluate a single item in an expression. */
static int preproc_evaluate_single(
  PreprocessInfo *info, StringTokenizer *tokens,
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
      vtkParse_NextToken(tokens);

      if (tokens->tok == '(')
      {
        paren = 1;
        vtkParse_NextToken(tokens);
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

      vtkParse_NextToken(tokens);
      if (paren)
      {
        if (tokens->tok != ')')
        {
#if PREPROC_DEBUG
          fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
          return VTK_PARSE_SYNTAX_ERROR;
        }
        vtkParse_NextToken(tokens);
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
      vtkParse_NextToken(tokens);
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
#if PREPROC_DEBUG
        fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
        return (args ? VTK_PARSE_MACRO_NUMARGS : VTK_PARSE_SYNTAX_ERROR);
      }
      cp = expansion;
      cp += vtkParse_SkipWhitespace(cp, WS_PREPROC);
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
    vtkParse_NextToken(tokens);
    result = preproc_evaluate_expression(info, tokens, val, is_unsigned);
    if ((result & VTK_PARSE_FATAL_ERROR) == 0)
    {
      if (tokens->tok == ')')
      {
        vtkParse_NextToken(tokens);
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
    vtkParse_NextToken(tokens);
    return result;
  }
  else if (tokens->tok == TOK_CHAR)
  {
    result = preproc_evaluate_char(tokens->text, val, is_unsigned);
    vtkParse_NextToken(tokens);
    return result;
  }
  else if (tokens->tok == TOK_STRING)
  {
    *val = 0;
    *is_unsigned = 0;
    vtkParse_NextToken(tokens);
    while (tokens->tok == TOK_STRING)
    {
      vtkParse_NextToken(tokens);
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
  PreprocessInfo *info, StringTokenizer *tokens,
  preproc_int_t *val, int *is_unsigned)
{
  int op = tokens->tok;
  int result = VTK_PARSE_OK;

  if (op != '+' && op != '-' && op != '~' && op != '!')
  {
    return preproc_evaluate_single(info, tokens, val, is_unsigned);
  }

  vtkParse_NextToken(tokens);

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
  PreprocessInfo *info, StringTokenizer *tokens,
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

    vtkParse_NextToken(tokens);

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
  PreprocessInfo *info, StringTokenizer *tokens,
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

    vtkParse_NextToken(tokens);

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
  PreprocessInfo *info, StringTokenizer *tokens,
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

    vtkParse_NextToken(tokens);

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
  PreprocessInfo *info, StringTokenizer *tokens,
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

    vtkParse_NextToken(tokens);

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
  PreprocessInfo *info, StringTokenizer *tokens,
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

    vtkParse_NextToken(tokens);

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
  PreprocessInfo *info, StringTokenizer *tokens,
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

    vtkParse_NextToken(tokens);

    result = preproc_evaluate_equal(info, tokens, &rval, &rtype);

    *is_unsigned = (*is_unsigned || rtype);
    *val = (*val & rval);
  }

  return result;
}

static int preproc_evaluate_xor(
  PreprocessInfo *info, StringTokenizer *tokens,
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

    vtkParse_NextToken(tokens);

    result = preproc_evaluate_and(info, tokens, &rval, &rtype);

    *is_unsigned = (*is_unsigned || rtype);
    *val = (*val ^ rval);
  }

  return result;
}

static int preproc_evaluate_or(
  PreprocessInfo *info, StringTokenizer *tokens,
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

    vtkParse_NextToken(tokens);

    result = preproc_evaluate_xor(info, tokens, &rval, &rtype);

    *is_unsigned = (*is_unsigned || rtype);
    *val = (*val | rval);
  }

  return result;
}

static int preproc_evaluate_logic_and(
  PreprocessInfo *info, StringTokenizer *tokens,
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

    vtkParse_NextToken(tokens);

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
          vtkParse_NextToken(tokens);
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
  PreprocessInfo *info, StringTokenizer *tokens,
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

    vtkParse_NextToken(tokens);

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
          vtkParse_NextToken(tokens);
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
  PreprocessInfo *info, StringTokenizer *tokens,
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

    vtkParse_NextToken(tokens);

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

    vtkParse_NextToken(tokens);

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
  PreprocessInfo *info, StringTokenizer *tokens)
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
  PreprocessInfo *info, StringTokenizer *tokens)
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
        vtkParse_NextToken(tokens);
        result = preproc_evaluate_conditional(info, tokens);
      }
      else
      {
        v1 = (tokens->hash != HASH_IFNDEF);
        vtkParse_NextToken(tokens);
        if (tokens->tok != TOK_ID)
        {
#if PREPROC_DEBUG
          fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
          return VTK_PARSE_SYNTAX_ERROR;
        }
        macro = preproc_find_macro(info, tokens);
        v2 = (macro && !macro->IsExcluded);
        vtkParse_NextToken(tokens);
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
        vtkParse_NextToken(tokens);
        result = preproc_evaluate_conditional(info, tokens);
      }
      else
      {
        vtkParse_NextToken(tokens);
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
    vtkParse_NextToken(tokens);
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
  PreprocessInfo *info, StringTokenizer *tokens)
{
  MacroInfo **macro_p;
  MacroInfo *macro;
  int is_function;
  int is_variadic;
  const char *name;
  size_t namelen;
  const char *definition = 0;
  int n = 0;
  const char **params = NULL;
  const char *param;
  size_t l;

  if (tokens->hash == HASH_DEFINE)
  {
    vtkParse_NextToken(tokens);
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
    vtkParse_NextToken(tokens);

    is_function = 0;
    is_variadic = 0;
    if (name[namelen] == '(')
    {
      is_function = 1;
      vtkParse_NextToken(tokens);
      while (tokens->tok != 0 && tokens->tok != ')')
      {
        if (tokens->tok != TOK_ID && tokens->tok != TOK_ELLIPSIS)
        {
          free((char **)params);
#if PREPROC_DEBUG
          fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
          return VTK_PARSE_SYNTAX_ERROR;
        }

        param = tokens->text;
        l = tokens->len;

        if (tokens->tok == TOK_ELLIPSIS)
        {
          is_variadic = 1;
          param = "__VA_ARGS__";
          l = 11;
        }

        /* add to the arg list */
        params = (const char **)preproc_array_check(
          (char **)params, sizeof(char *), n);
        params[n++] = vtkParse_CacheString(info->Strings, param, l);

        vtkParse_NextToken(tokens);

        /* check for gnu cpp "arg..." parameter */
        if (tokens->tok == TOK_ELLIPSIS)
        {
          is_variadic = 1;
          vtkParse_NextToken(tokens);
        }

        if (tokens->tok == ',')
        {
          vtkParse_NextToken(tokens);
        }
        else if (tokens->tok != ')')
        {
          free((char **)params);
#if PREPROC_DEBUG
          fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
          return VTK_PARSE_SYNTAX_ERROR;
        }
      }
      vtkParse_NextToken(tokens);
    }

    if (tokens->tok)
    {
      definition = tokens->text;
    }

    macro = *macro_p;
    if (macro)
    {
      free((char **)params);
      if (preproc_identical(macro->Definition, definition))
      {
        return VTK_PARSE_OK;
      }
#if PREPROC_DEBUG
      fprintf(stderr, "macro redefined %d\n", __LINE__);
#endif
      return VTK_PARSE_MACRO_REDEFINED;
    }

    macro = preproc_new_macro(info, name, definition);
    macro->IsFunction = is_function;
    macro->IsVariadic = is_variadic;
    macro->NumberOfParameters = n;
    macro->Parameters = params;
    *macro_p = macro;

    return VTK_PARSE_OK;
  }
  else if (tokens->hash == HASH_UNDEF)
  {
    vtkParse_NextToken(tokens);
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

  n = info->NumberOfIncludeFiles;
  for (i = 0; i < n; i++)
  {
    if (strcmp(info->IncludeFiles[i], name) == 0)
    {
      return 0;
    }
  }

  info->IncludeFiles = (const char **)preproc_array_check(
    (char **)info->IncludeFiles, sizeof(char *), info->NumberOfIncludeFiles);
  info->IncludeFiles[info->NumberOfIncludeFiles++] =
    vtkParse_CacheString(info->Strings, name, strlen(name));

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
  while (vtkParse_CharType(filename[j], CPRE_XID)) { j++; }

  if (filename[j] == ':' || filename[0] == '/' || filename[0] == '\\')
  {
    if (m+1 > outputsize)
    {
      char *oldoutput = output;
      outputsize += m+1;
      output = (char *)realloc(output, outputsize);
      if (!output)
      {
        free(oldoutput);
        return NULL;
      }
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
            char *oldoutput = output;
            outputsize += m+j+1;
            output = (char *)realloc(output, outputsize);
            if (!output)
            {
              free(oldoutput);
              return NULL;
            }
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
            char *oldoutput = output;
            outputsize += m+1;
            output = (char *)realloc(output, outputsize);
            if (!output)
            {
              free(oldoutput);
              return NULL;
            }
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
          char *oldoutput = output;
          outputsize += j+m+2;
          output = (char *)realloc(output, outputsize);
          if (!output)
          {
            free(oldoutput);
            return NULL;
          }
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
#if defined(_WIN32) && !defined(__CYGWIN__)
      else if (stat(output, &fs) == 0 &&
               (fs.st_mode & _S_IFMT) != _S_IFDIR)
#else
      else if (stat(output, &fs) == 0 &&
               !S_ISDIR(fs.st_mode))
#endif
      {
        nn = info->NumberOfIncludeFiles;
        info->IncludeFiles = (const char **)preproc_array_check(
          (char **)info->IncludeFiles, sizeof(char *), nn);
        info->IncludeFiles[info->NumberOfIncludeFiles++] =
          vtkParse_CacheString(info->Strings, output, strlen(output));
        free(output);
        return info->IncludeFiles[nn];
      }
    }
  }

  free(output);
  return NULL;
}

/**
 * Convert a raw string into a normal string.  This is a helper
 * function for preproc_include_file() to allow raw strings to
 * be used in preprocessor directives.
 */
void preproc_escape_string(
  char **linep, size_t *linelenp, size_t *jp, size_t d, size_t dl)
{
  char *line = *linep;
  char *r = 0;
  size_t linelen = *linelenp;
  size_t l = *jp - d - 2*dl - 2;
  size_t i;
  size_t j = d;

  if (l != 0)
  {
    r = (char *)malloc(l);
    memcpy(r, &line[j+dl+1], l);
  }

  /* remove the "R" prefix */
  if (j >= 2 && line[j-1] == '\"' && line[j-2] == 'R')
  {
    line[j - 2] = '\"';
    j--;
  }

  for (i = 0; i < l; i++)
  {
    /* expand line buffer as necessary */
    while (j+4 > linelen)
    {
      char *oldline = line;
      linelen *= 2;
      line = (char *)realloc(line, linelen);
      if (!line)
      {
        free(r);
        free(oldline);
        *linep = NULL;
        *linelenp = -1;
        *jp = 0; /* XXX: Is this right? */
        return;
      }
    }

    if ((r[i] >= ' ' && r[i] <= '~') || (r[i] & 0x80) != 0)
    {
      line[j++] = r[i];
    }
    else switch (r[i])
    {
      case '\a': line[j++] = '\\'; line[j++] = 'a'; break;
      case '\b': line[j++] = '\\'; line[j++] = 'b'; break;
      case '\f': line[j++] = '\\'; line[j++] = 'f'; break;
      case '\n': line[j++] = '\\'; line[j++] = 'n'; break;
      case '\r': line[j++] = '\\'; line[j++] = 'r'; break;
      case '\t': line[j++] = '\\'; line[j++] = 't'; break;
      case '\v': line[j++] = '\\'; line[j++] = 'v'; break;
      case '\\': line[j++] = '\\'; line[j++] = '\\'; break;
      case '\'': line[j++] = '\\'; line[j++] = '\''; break;
      case '\"': line[j++] = '\\'; line[j++] = '\"'; break;
      default:
        sprintf(&line[j], "\\%3.3o", r[i]);
        j += 4;
        break;
    }
  }

  free(r);
  *linep = line;
  *linelenp = linelen;
  *jp = j;
}

/**
 * Include a file.  All macros defined in the included file
 * will have their IsExternal flag set.
 */
static int preproc_include_file(
  PreprocessInfo *info, const char *filename, int system_first)
{
  const char *switchchars = "\n\r\"\'\?\\/*()";
  char switchchar[256];
  char *tbuf;
  size_t tbuflen = FILE_BUFFER_SIZE;
  char *line;
  size_t linelen = 80;
  size_t i, j, n, r;
  size_t d = 0;
  size_t dn = 0;
  int state = 0;
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

  /* make a table of interesting characters */
  memset(switchchar, '\0', 256);
  n = strlen(switchchars) + 1;
  for (i = 0; i < n; i++)
  {
    switchchar[(unsigned char)(switchchars[i])] = 1;
  }

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
        char *oldline = line;
        linelen *= 2;
        line = (char *)realloc(line, linelen);
        if (!line)
        {
          free(oldline);
          return VTK_PARSE_OUT_OF_MEMORY;
        }
      }

      /* check for uninteresting characters first */
      if (!switchchar[(unsigned char)(tbuf[i])])
      {
        line[j++] = tbuf[i++];
      }
      else if (state == '(')
      {
        /* look for end of raw string delimiter */
        if (tbuf[i] == '(')
        {
          dn = j - d;
          state = ')';
        }
        line[j++] = tbuf[i++];
      }
      else if (state == ')')
      {
        /* look for end of raw string */
        if (tbuf[i] == '\"')
        {
          if ((j - d) > 2*dn+1 && line[j-dn-1] == ')' &&
              strncmp(&line[d], &line[j-dn], dn) == 0)
          {
            preproc_escape_string(&line, &linelen, &j, d, dn);
            state = 0;
          }
        }
        line[j++] = tbuf[i++];
      }
#ifdef PREPROC_TRIGRAPHS
      else if (tbuf[i] == '?' && tbuf[i+1] == '?')
      {
        i += 2;
        switch (tbuf[i])
        {
          case '=': tbuf[i] = '#'; break;
          case '/': tbuf[i] = '\\'; break;
          case '\'': tbuf[i] = '^'; break;
          case '(': tbuf[i] = '['; break;
          case ')': tbuf[i] = ']'; break;
          case '!': tbuf[i] = '|'; break;
          case '<': tbuf[i] = '{'; break;
          case '>': tbuf[i] = '}'; break;
          case '-': tbuf[i] = '~'; break;
          default: line[j++] = tbuf[--i];
        }
      }
#endif
      else if (tbuf[i] == '\\' && tbuf[i+1] == '\n')
      {
        i += 2;
      }
      else if (tbuf[i] == '\\' && tbuf[i+1] == '\r' && tbuf[i+2] == '\n')
      {
        i += 3;
      }
      else if (tbuf[i] == '\r' && tbuf[i+1] == '\n')
      {
        i++;
      }
      else if (state == '*')
      {
        if (tbuf[i] == '*' && tbuf[i+1] == '/')
        {
          line[j++] = tbuf[i++];
          line[j++] = tbuf[i++];
          state = 0;
        }
        else
        {
          line[j++] = tbuf[i++];
        }
      }
      else if (state == '/' && tbuf[i] != '\n')
      {
        line[j++] = tbuf[i++];
      }
      else if (state == '\'' || state == '\"')
      {
        if (tbuf[i] == state)
        {
          line[j++] = tbuf[i++];
          state = 0;
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
      else if (tbuf[i] == '/')
      {
        if (tbuf[i+1] == '*' || tbuf[i+1] == '/')
        {
          state = tbuf[i+1];
          line[j++] = tbuf[i++];
        }
        line[j++] = tbuf[i++];
      }
      else if (tbuf[i] == '\"' || tbuf[i] == '\'')
      {
        state = tbuf[i];
        /* check for raw string prefixes */
        if (state == '\"' && j > 0 && line[j-1] == 'R' &&
            ((j > 2 &&
              (line[j-3] == 'u' || line[j-2] == '8') &&
              (j == 3 ||
               !vtkParse_CharType(line[j-4], CPRE_XID|CPRE_QUOTE))) ||
             (j > 1 &&
              (line[j-2] == 'u' || line[j-2] == 'U' || line[j-2] == 'L') &&
              (j == 2 ||
               !vtkParse_CharType(line[j-3], CPRE_XID|CPRE_QUOTE))) ||
             (j == 1 ||
              !vtkParse_CharType(line[j-2], CPRE_XID|CPRE_QUOTE))))
        {
          state = '(';
          d = j + 1;
        }
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
      cp += vtkParse_SkipWhitespace(cp, WS_PREPROC);
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
  PreprocessInfo *info, StringTokenizer *tokens)
{
  const char *cp;
  const char *filename;

  if (tokens->hash == HASH_INCLUDE)
  {
    vtkParse_NextToken(tokens);

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
      cp += vtkParse_SkipQuotes(cp);
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
  StringTokenizer tokens;

  vtkParse_InitTokenizer(&tokens, directive, WS_PREPROC);

  if (tokens.tok != '#')
  {
    return VTK_PARSE_SYNTAX_ERROR;
  }

  vtkParse_NextToken(&tokens);

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
      while (tokens.tok) { vtkParse_NextToken(&tokens); }
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
  StringTokenizer tokens;
  vtkParse_InitTokenizer(&tokens, text, WS_PREPROC);

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

  /*------------------------------
   * a special macro to indicate that this is the wrapper
   */
  preproc_add_macro_definition(info, "__WRAP__", "1");

  /*------------------------------
   * language macros - assume that we are wrapping C++ code
   */
#ifdef __cplusplus
  PREPROC_ADD_MACRO(info, __cplusplus);
#else
  preproc_add_macro_definition(info, "__cplusplus", "1");
#endif

  /*------------------------------
   * stdc version macros
   */
#ifdef __STDC__
  PREPROC_ADD_MACRO(info, __STDC__);
#endif
#ifdef __STDC_VERSION__
  PREPROC_ADD_MACRO(info, __STDC_VERSION__);
#endif
#ifdef __STDC_HOSTED__
  PREPROC_ADD_MACRO(info, __STDC_HOSTED__);
#endif
#ifdef __STDC_UTF_16__
  PREPROC_ADD_MACRO(info, __STDC_UTF_16__);
#endif
#ifdef __STDC_UTF_32__
  PREPROC_ADD_MACRO(info, __STDC_UTF_32__);
#endif
#ifdef __STDC_ISO_10646__
  PREPROC_ADD_MACRO(info, __STDC_ISO_10646__);
#endif
#ifdef __STDC_MB_MIGHT_NEQ_WC__
  PREPROC_ADD_MACRO(info, __STDC_MB_MIGHT_NEQ_WC__);
#endif

  /* define _DEBUG if it was set */
#ifdef _DEBUG
  PREPROC_ADD_MACRO(info, _DEBUG);
#endif

  /*------------------------------
   * Currently, only the "native" platform is fully supported, which
   * means that cross-compilation of the wrappers might not work.
   * For native, we define the same macros as the native compiler.
   */
  if (platform == VTK_PARSE_NATIVE)
  {
    /* macros that declare the pointer size */
#ifdef _SIZE_PTR
    PREPROC_ADD_MACRO(info, _SIZE_PTR);
#endif
#ifdef __SIZEOF_POINTER__
    PREPROC_ADD_MACRO(info, __SIZEOF_POINTER__);
#endif
#ifdef __POINTER_WIDTH__
    PREPROC_ADD_MACRO(info, __POINTER_WIDTH__);
#endif
#ifdef __BIGGEST_ALIGNMENT__
    PREPROC_ADD_MACRO(info, __BIGGEST_ALIGNMENT__);
#endif
#ifdef _ILP32
    PREPROC_ADD_MACRO(info, _ILP32);
#endif
#ifdef __ILP32__
    PREPROC_ADD_MACRO(info, __ILP32__);
#endif
#ifdef _LP64
    PREPROC_ADD_MACRO(info, _LP64);
#endif
#ifdef __LP64__
    PREPROC_ADD_MACRO(info, __LP64__);
#endif
#ifdef __64BIT__
    PREPROC_ADD_MACRO(info, __64BIT__);
#endif

    /*------------------------------
     * macros for the signedness of the char type
     */
#ifdef _CHAR_IS_UNSIGNED
    PREPROC_ADD_MACRO(info, _CHAR_IS_UNSIGNED);
#endif
#ifdef _CHAR_IS_SIGNED
    PREPROC_ADD_MACRO(info, _CHAR_IS_SIGNED);
#endif
#ifdef _CHAR_UNSIGNED
    PREPROC_ADD_MACRO(info, _CHAR_UNSIGNED);
#endif
#ifdef _CHAR_SIGNED
    PREPROC_ADD_MACRO(info, _CHAR_SIGNED);
#endif
#ifdef __CHAR_UNSIGNED__
    PREPROC_ADD_MACRO(info, __CHAR_UNSIGNED__);
#endif
#ifdef __CHAR_SIGNED__
    PREPROC_ADD_MACRO(info, __CHAR_SIGNED__);
#endif
#ifdef __SIGNED_CHARS__
    PREPROC_ADD_MACRO(info, __SIGNED_CHARS__);
#endif
#ifdef __WCHAR_UNSIGNED__
    PREPROC_ADD_MACRO(info, __WCHAR_UNSIGNED__);
#endif
#ifdef __WCHAR_SIGNED__
    PREPROC_ADD_MACRO(info, __WCHAR_SIGNED__);
#endif

    /*------------------------------
     * macros that declare the endianness
     */
#ifdef _BIG_ENDIAN
    PREPROC_ADD_MACRO(info, _BIG_ENDIAN);
#endif
#ifdef _LITTLE_ENDIAN
    PREPROC_ADD_MACRO(info, _LITTLE_ENDIAN);
#endif
#ifdef __BIG_ENDIAN__
    PREPROC_ADD_MACRO(info, __BIG_ENDIAN__);
#endif
#ifdef __LITTLE_ENDIAN__
    PREPROC_ADD_MACRO(info, __LITTLE_ENDIAN__);
#endif
#ifdef __ORDER_BIG_ENDIAN__
    PREPROC_ADD_MACRO(info, __ORDER_BIG_ENDIAN__);
#endif
#ifdef __ORDER_LITTLE_ENDIAN__
    PREPROC_ADD_MACRO(info, __ORDER_LITTLE_ENDIAN__);
#endif
#ifdef __ORDER_PDP_ENDIAN__
    PREPROC_ADD_MACRO(info, __ORDER_PDP_ENDIAN__);
#endif
#ifdef __BYTE_ORDER__
    PREPROC_ADD_MACRO(info, __BYTE_ORDER__);
#endif
#ifdef __FLOAT_WORD_ORDER__
    PREPROC_ADD_MACRO(info, __FLOAT_WORD_ORDER__);
#endif

    /*------------------------------
     * endianness for specific architectures
     */
#ifdef __AARCH64EB__
    PREPROC_ADD_MACRO(info, __AARCH64EB__);
#endif
#ifdef __AARCH64EL__
    PREPROC_ADD_MACRO(info, __AARCH64EL__);
#endif
#ifdef __XTENSA_EB__
    PREPROC_ADD_MACRO(info, __XTENSA_EB__);
#endif
#ifdef __XTENSA_EL__
    PREPROC_ADD_MACRO(info, __XTENSA_EL__);
#endif
#ifdef __ARMEB__
    PREPROC_ADD_MACRO(info, __ARMEB__);
#endif
#ifdef __ARMEL__
    PREPROC_ADD_MACRO(info, __ARMEL__);
#endif
#ifdef __THUMBEB__
    PREPROC_ADD_MACRO(info, __THUMBEB__);
#endif
#ifdef __THUMBEL__
    PREPROC_ADD_MACRO(info, __THUMBEL__);
#endif
#ifdef __MIPSEB__
    PREPROC_ADD_MACRO(info, __MIPSEB__);
#endif
#ifdef __MIPSEL__
    PREPROC_ADD_MACRO(info, __MIPSEL__);
#endif
#ifdef __MIPSEB
    PREPROC_ADD_MACRO(info, __MIPSEB);
#endif
#ifdef __MIPSEL
    PREPROC_ADD_MACRO(info, __MIPSEL);
#endif
#ifdef _MIPSEB
    PREPROC_ADD_MACRO(info, _MIPSEB);
#endif
#ifdef _MIPSEL
    PREPROC_ADD_MACRO(info, _MIPSEL);
#endif

    /*------------------------------
     * architecture macros
     */
#ifdef _M_ALPHA
    PREPROC_ADD_MACRO(info, _M_ALPHA);
#endif
#ifdef _M_IX86
    PREPROC_ADD_MACRO(info, _M_IX86);
#endif
#ifdef _M_IX86_FP
    PREPROC_ADD_MACRO(info, _M_IX86_FP);
#endif
#ifdef _M_X64
    PREPROC_ADD_MACRO(info, _M_X64);
#endif
#ifdef _M_AMD64
    PREPROC_ADD_MACRO(info, _M_AMD64);
#endif
#ifdef _M_ARM
    PREPROC_ADD_MACRO(info, _M_ARM);
#endif
#ifdef _M_ARM_ARMV7VE
    PREPROC_ADD_MACRO(info, _M_ARM_ARMV7VE);
#endif
#ifdef _M_ARM_FP
    PREPROC_ADD_MACRO(info, _M_ARM_FP);
#endif
#ifdef _M_ARM64
    PREPROC_ADD_MACRO(info, _M_ARM64);
#endif

#ifdef _X86_
    PREPROC_ADD_MACRO(info, _X86_);
#endif
#ifdef __I86__
    PREPROC_ADD_MACRO(info, __I86__);
#endif
#ifdef __i386
    PREPROC_ADD_MACRO(info, __i386);
#endif
#ifdef __i386__
    PREPROC_ADD_MACRO(info, __i386__);
#endif
#ifdef __i686
    PREPROC_ADD_MACRO(info, __i686);
#endif
#ifdef __i686__
    PREPROC_ADD_MACRO(info, __i686__);
#endif
#ifdef __amd64
    PREPROC_ADD_MACRO(info, __amd64);
#endif
#ifdef __amd64__
    PREPROC_ADD_MACRO(info, __amd64__);
#endif
#ifdef __x86_64
    PREPROC_ADD_MACRO(info, __x86_64);
#endif
#ifdef __x86_64__
    PREPROC_ADD_MACRO(info, __x86_64__);
#endif
#ifdef __ia64
    PREPROC_ADD_MACRO(info, __ia64);
#endif
#ifdef __ia64__
    PREPROC_ADD_MACRO(info, __ia64__);
#endif
#ifdef __IA64
    PREPROC_ADD_MACRO(info, __IA64);
#endif
#ifdef __IA64__
    PREPROC_ADD_MACRO(info, __IA64__);
#endif
#ifdef __arm
    PREPROC_ADD_MACRO(info, __arm);
#endif
#ifdef __arm__
    PREPROC_ADD_MACRO(info, __arm__);
#endif
#ifdef __mips
    PREPROC_ADD_MACRO(info, __mips);
#endif
#ifdef __mips__
    PREPROC_ADD_MACRO(info, __mips__);
#endif
#ifdef __MIPS__
    PREPROC_ADD_MACRO(info, __MIPS__);
#endif
#ifdef __sparc
    PREPROC_ADD_MACRO(info, __sparc);
#endif
#ifdef __sparc__
    PREPROC_ADD_MACRO(info, __sparc__);
#endif
#ifdef __sparcv8
    PREPROC_ADD_MACRO(info, __sparcv8);
#endif
#ifdef __sparc_v8__
    PREPROC_ADD_MACRO(info, __sparc_v8__);
#endif
#ifdef __sparcv9
    PREPROC_ADD_MACRO(info, __sparcv9);
#endif
#ifdef __sparc_v9__
    PREPROC_ADD_MACRO(info, __sparc_v9__);
#endif
#ifdef __alpha
    PREPROC_ADD_MACRO(info, __alpha);
#endif
#ifdef __alpha__
    PREPROC_ADD_MACRO(info, __alpha__);
#endif
#ifdef __hppa
    PREPROC_ADD_MACRO(info, __hppa);
#endif
#ifdef __hppa__
    PREPROC_ADD_MACRO(info, __hppa__);
#endif
#ifdef __ppc
    PREPROC_ADD_MACRO(info, __ppc);
#endif
#ifdef __ppc__
    PREPROC_ADD_MACRO(info, __ppc__);
#endif
#ifdef __ppc64__
    PREPROC_ADD_MACRO(info, __ppc64__);
#endif
#ifdef __powerpc
    PREPROC_ADD_MACRO(info, __powerpc);
#endif
#ifdef __powerpc__
    PREPROC_ADD_MACRO(info, __powerpc__);
#endif
#ifdef __powerpc64__
    PREPROC_ADD_MACRO(info, __powerpc64__);
#endif
#ifdef __POWERPC__
    PREPROC_ADD_MACRO(info, __POWERPC__);
#endif
#ifdef __aarch64__
    PREPROC_ADD_MACRO(info, __aarch64__);
#endif
#ifdef __xtensa__
    PREPROC_ADD_MACRO(info, __xtensa__);
#endif

    /*------------------------------
     * compiler macros
     */
#ifdef _MSC_VER
    PREPROC_ADD_MACRO(info, _MSC_VER);
#endif
#ifdef _MSC_FULL_VER
    PREPROC_ADD_MACRO(info, _MSC_FULL_VER);
#endif
#ifdef _MSC_BUILD
    PREPROC_ADD_MACRO(info, _MSC_BUILD);
#endif
#ifdef _MSC_EXTENSIONS
    PREPROC_ADD_MACRO(info, _MSC_EXTENSIONS);
#endif

#ifdef _COMPILER_VERSION
    PREPROC_ADD_MACRO(info, _COMPILER_VERSION);
#endif
#ifdef _SGI_COMPILER_VERSION
    PREPROC_ADD_MACRO(info, _SGI_COMPILER_VERSION);
#endif

#ifdef __llvm__
    PREPROC_ADD_MACRO(info, __llvm__);
#endif
#ifdef __clang__
    PREPROC_ADD_MACRO(info, __clang__);
#endif
#ifdef __clang_major__
    PREPROC_ADD_MACRO(info, __clang_major__);
#endif
#ifdef __clang_minor__
    PREPROC_ADD_MACRO(info, __clang_minor__);
#endif
#ifdef __clang_patchlevel__
    PREPROC_ADD_MACRO(info, __clang_patchlevel__);
#endif
#ifdef __clang_version__
    PREPROC_ADD_MACRO(info, __clang_version__);
#endif

#ifdef __GNUC__
    PREPROC_ADD_MACRO(info, __GNUC__);
#endif
#ifdef __GNUC_MINOR__
    PREPROC_ADD_MACRO(info, __GNUC_MINOR__);
#endif
#ifdef __GNUC_PATCHLEVEL__
    PREPROC_ADD_MACRO(info, __GNUC_PATCHLEVEL__);
#endif
#ifdef __GNUC_STDC_INLINE__
    PREPROC_ADD_MACRO(info, __GNUC_STDC_INLINE__);
#endif
#ifdef __VERSION__
    PREPROC_ADD_MACRO(info, __VERSION__);
#endif

#ifdef __GLIBC__
    PREPROC_ADD_MACRO(info, __GLIBC__);
#endif
#ifdef __GLIBC_MINOR__
    PREPROC_ADD_MACRO(info, __GLIBC_MINOR__);
#endif

#ifdef __MINGW32__
    PREPROC_ADD_MACRO(info, __MINGW32__);
#endif
#ifdef __MINGW32_MAJOR_VERSION
    PREPROC_ADD_MACRO(info, __MINGW32_MAJOR_VERSION);
#endif
#ifdef __MINGW32_MINOR_VERSION
    PREPROC_ADD_MACRO(info, __MINGW32_MINOR_VERSION);
#endif
#ifdef __MINGW64__
    PREPROC_ADD_MACRO(info, __MINGW64__);
#endif
#ifdef __MINGW64_MAJOR_VERSION
    PREPROC_ADD_MACRO(info, __MINGW64_MAJOR_VERSION);
#endif
#ifdef __MINGW64_MINOR_VERSION
    PREPROC_ADD_MACRO(info, __MINGW64_MINOR_VERSION);
#endif

#ifdef __THW_BLUEGENE__
    PREPROC_ADD_MACRO(info, __THW_BLUEGENE__);
#endif
#ifdef __THW_INTEL__
    PREPROC_ADD_MACRO(info, __THW_INTEL__);
#endif
#ifdef __THW_RS6000
    PREPROC_ADD_MACRO(info, __THW_RS6000);
#endif

#ifdef __APPLE_CC__
    PREPROC_ADD_MACRO(info, __APPLE_CC__);
#endif

#ifdef __xlc__
    PREPROC_ADD_MACRO(info, __xlc__);
#endif
#ifdef __xlC__
    PREPROC_ADD_MACRO(info, __xlC__);
#endif
#ifdef __xlC_ver__
    PREPROC_ADD_MACRO(info, __xlC_ver__);
#endif
#ifdef __IBMC__
    PREPROC_ADD_MACRO(info, __IBMC__);
#endif
#ifdef __IBMCPP__
    PREPROC_ADD_MACRO(info, __IBMCPP__);
#endif
#ifdef __HP_cc
    PREPROC_ADD_MACRO(info, __HP_cc);
#endif
#ifdef __HP_aCC
    PREPROC_ADD_MACRO(info, __HP_aCC);
#endif
#ifdef __PGIC__
    PREPROC_ADD_MACRO(info, __PGIC__);
#endif
#ifdef __EDG__
    PREPROC_ADD_MACRO(info, __EDG__);
#endif
#ifdef __EDG_VERSION__
    PREPROC_ADD_MACRO(info, __EDG_VERSION__);
#endif
#ifdef __WATCOMC__
    PREPROC_ADD_MACRO(info, __WATCOMC__);
#endif
#ifdef __BORLANDC__
    PREPROC_ADD_MACRO(info, __BORLANDC__);
#endif
#ifdef __KCC
    PREPROC_ADD_MACRO(info, __KCC);
#endif
#ifdef __KCC_VERSION
    PREPROC_ADD_MACRO(info, __KCC_VERSION);
#endif
#ifdef __SUNPRO_C
    PREPROC_ADD_MACRO(info, __SUNPRO_C);
#endif
#ifdef __SUNPRO_CC
    PREPROC_ADD_MACRO(info, __SUNPRO_CC);
#endif
#ifdef __INTEL_COMPILER
    PREPROC_ADD_MACRO(info, __INTEL_COMPILER);
#endif
#ifdef __INTEL_COMPILER_BUILD_DATE
    PREPROC_ADD_MACRO(info, __INTEL_COMPILER_BUILD_DATE);
#endif
#ifdef __ECC
    PREPROC_ADD_MACRO(info, __ECC);
#endif
#ifdef __ICC
    PREPROC_ADD_MACRO(info, __ICC);
#endif
#ifdef __ICL
    PREPROC_ADD_MACRO(info, __ICL);
#endif

    /*------------------------------
     * platform macros
     */
#ifdef _WIN32
    PREPROC_ADD_MACRO(info, _WIN32);
#endif
#ifdef _WIN64
    PREPROC_ADD_MACRO(info, _WIN64);
#endif

#ifdef _CRAY
    PREPROC_ADD_MACRO(info, _CRAY);
#endif
#ifdef _UNICOS
    PREPROC_ADD_MACRO(info, _UNICOS);
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
#ifdef __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__
    PREPROC_ADD_MACRO(info, __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__);
#endif
#ifdef __apple_build_version__
    PREPROC_ADD_MACRO(info, __apple_build_version__);
#endif

#ifdef __ANDROID__
    PREPROC_ADD_MACRO(info, __ANDROID__);
#endif
#ifdef __ANDROID_API__
    PREPROC_ADD_MACRO(info, __ANDROID_API__);
#endif
#ifdef __bg__
    PREPROC_ADD_MACRO(info, __bg__);
#endif
#ifdef __bgq__
    PREPROC_ADD_MACRO(info, __bgq__);
#endif
#ifdef __TOS_BGQ__
    PREPROC_ADD_MACRO(info, __TOS_BGQ__);
#endif
#ifdef __FreeBSD__
    PREPROC_ADD_MACRO(info, __FreeBSD__);
#endif
#ifdef __NetBSD__
    PREPROC_ADD_MACRO(info, __NetBSD__);
#endif
#ifdef __OpenBSD__
    PREPROC_ADD_MACRO(info, __OpenBSD__);
#endif
#ifdef __bsdi__
    PREPROC_ADD_MACRO(info, __bsdi__);
#endif
#ifdef __DragonFly__
    PREPROC_ADD_MACRO(info, __DragonFly__);
#endif
#ifdef __CYGWIN__
    PREPROC_ADD_MACRO(info, __CYGWIN__);
#endif
#ifdef __GNU__
    PREPROC_ADD_MACRO(info, __GNU__);
#endif
#ifdef __QNX__
    PREPROC_ADD_MACRO(info, __QNX__);
#endif
#ifdef __QNXTO__
    PREPROC_ADD_MACRO(info, __QNXTO__);
#endif
#ifdef __hpux
    PREPROC_ADD_MACRO(info, __hpux);
#endif
#ifdef __sgi
    PREPROC_ADD_MACRO(info, __sgi);
#endif
#ifdef __unix
    PREPROC_ADD_MACRO(info, __unix);
#endif
#ifdef __unix__
    PREPROC_ADD_MACRO(info, __unix__);
#endif
#ifdef __linux
    PREPROC_ADD_MACRO(info, __linux);
#endif
#ifdef __linux__
    PREPROC_ADD_MACRO(info, __linux__);
#endif
#ifdef __gnu_linux__
    PREPROC_ADD_MACRO(info, __gnu_linux__);
#endif

    /*------------------------------
     * miscellaneous type-related macros
     */
#ifdef _WCHAR_T_DEFINED
    preproc_add_macro_definition(info, "_WCHAR_T_DEFINED", NULL);
#endif
#ifdef _NATIVE_WCHAR_T_DEFINED
    PREPROC_ADD_MACRO(info, _NATIVE_WCHAR_T_DEFINED);
#endif
#ifdef _LONG_LONG
    PREPROC_ADD_MACRO(info, _LONG_LONG);
#endif
#ifdef __NO_LONG_LONG
    PREPROC_ADD_MACRO(info, __NO_LONG_LONG);
#endif
#ifdef __CHAR16_TYPE__
    PREPROC_ADD_MACRO(info, __CHAR16_TYPE__);
#endif
#ifdef __CHAR32_TYPE__
    PREPROC_ADD_MACRO(info, __CHAR32_TYPE__);
#endif
#ifdef __CHAR_BIT__
    PREPROC_ADD_MACRO(info, __CHAR_BIT__);
#endif
#ifdef __SCHAR_MAX__
    PREPROC_ADD_MACRO(info, __SCHAR_MAX__);
#endif
#ifdef __SHRT_MAX__
    PREPROC_ADD_MACRO(info, __SHRT_MAX__);
#endif

    /*------------------------------
     * type macros for gcc-compatible compilers
     */
#ifdef __DBL_DENORM_MIN__
    PREPROC_ADD_MACRO(info, __DBL_DENORM_MIN__);
#endif
#ifdef __DBL_DIG__
    PREPROC_ADD_MACRO(info, __DBL_DIG__);
#endif
#ifdef __DBL_DECIMAL_DIG__
    PREPROC_ADD_MACRO(info, __DBL_DECIMAL_DIG__);
#endif
#ifdef __DBL_EPSILON__
    PREPROC_ADD_MACRO(info, __DBL_EPSILON__);
#endif
#ifdef __DBL_HAS_DENORM__
    PREPROC_ADD_MACRO(info, __DBL_HAS_DENORM__);
#endif
#ifdef __DBL_HAS_INFINITY__
    PREPROC_ADD_MACRO(info, __DBL_HAS_INFINITY__);
#endif
#ifdef __DBL_HAS_QUIET_NAN__
    PREPROC_ADD_MACRO(info, __DBL_HAS_QUIET_NAN__);
#endif
#ifdef __DBL_MANT_DIG__
    PREPROC_ADD_MACRO(info, __DBL_MANT_DIG__);
#endif
#ifdef __DBL_MAX_10_EXP__
    PREPROC_ADD_MACRO(info, __DBL_MAX_10_EXP__);
#endif
#ifdef __DBL_MAX_EXP__
    PREPROC_ADD_MACRO(info, __DBL_MAX_EXP__);
#endif
#ifdef __DBL_MAX__
    PREPROC_ADD_MACRO(info, __DBL_MAX__);
#endif
#ifdef __DBL_MIN_10_EXP__
    PREPROC_ADD_MACRO(info, __DBL_MIN_10_EXP__);
#endif
#ifdef __DBL_MIN_EXP__
    PREPROC_ADD_MACRO(info, __DBL_MIN_EXP__);
#endif
#ifdef __DBL_MIN__
    PREPROC_ADD_MACRO(info, __DBL_MIN__);
#endif
#ifdef __DEC128_EPSILON__
    PREPROC_ADD_MACRO(info, __DEC128_EPSILON__);
#endif
#ifdef __DEC128_MANT_DIG__
    PREPROC_ADD_MACRO(info, __DEC128_MANT_DIG__);
#endif
#ifdef __DEC128_MAX_EXP__
    PREPROC_ADD_MACRO(info, __DEC128_MAX_EXP__);
#endif
#ifdef __DEC128_MAX__
    PREPROC_ADD_MACRO(info, __DEC128_MAX__);
#endif
#ifdef __DEC128_MIN_EXP__
    PREPROC_ADD_MACRO(info, __DEC128_MIN_EXP__);
#endif
#ifdef __DEC128_MIN__
    PREPROC_ADD_MACRO(info, __DEC128_MIN__);
#endif
#ifdef __DEC128_SUBNORMAL_MIN__
    PREPROC_ADD_MACRO(info, __DEC128_SUBNORMAL_MIN__);
#endif
#ifdef __DEC32_EPSILON__
    PREPROC_ADD_MACRO(info, __DEC32_EPSILON__);
#endif
#ifdef __DEC32_MANT_DIG__
    PREPROC_ADD_MACRO(info, __DEC32_MANT_DIG__);
#endif
#ifdef __DEC32_MAX_EXP__
    PREPROC_ADD_MACRO(info, __DEC32_MAX_EXP__);
#endif
#ifdef __DEC32_MAX__
    PREPROC_ADD_MACRO(info, __DEC32_MAX__);
#endif
#ifdef __DEC32_MIN_EXP__
    PREPROC_ADD_MACRO(info, __DEC32_MIN_EXP__);
#endif
#ifdef __DEC32_MIN__
    PREPROC_ADD_MACRO(info, __DEC32_MIN__);
#endif
#ifdef __DEC32_SUBNORMAL_MIN__
    PREPROC_ADD_MACRO(info, __DEC32_SUBNORMAL_MIN__);
#endif
#ifdef __DEC64_EPSILON__
    PREPROC_ADD_MACRO(info, __DEC64_EPSILON__);
#endif
#ifdef __DEC64_MANT_DIG__
    PREPROC_ADD_MACRO(info, __DEC64_MANT_DIG__);
#endif
#ifdef __DEC64_MAX_EXP__
    PREPROC_ADD_MACRO(info, __DEC64_MAX_EXP__);
#endif
#ifdef __DEC64_MAX__
    PREPROC_ADD_MACRO(info, __DEC64_MAX__);
#endif
#ifdef __DEC64_MIN_EXP__
    PREPROC_ADD_MACRO(info, __DEC64_MIN_EXP__);
#endif
#ifdef __DEC64_MIN__
    PREPROC_ADD_MACRO(info, __DEC64_MIN__);
#endif
#ifdef __DEC64_SUBNORMAL_MIN__
    PREPROC_ADD_MACRO(info, __DEC64_SUBNORMAL_MIN__);
#endif
#ifdef __DECIMAL_BID_FORMAT__
    PREPROC_ADD_MACRO(info, __DECIMAL_BID_FORMAT__);
#endif
#ifdef __DECIMAL_DIG__
    PREPROC_ADD_MACRO(info, __DECIMAL_DIG__);
#endif
#ifdef __DEC_EVAL_METHOD__
    PREPROC_ADD_MACRO(info, __DEC_EVAL_METHOD__);
#endif
#ifdef __FLT_DECIMAL_DIG__
    PREPROC_ADD_MACRO(info, __FLT_DECIMAL_DIG__);
#endif
#ifdef __FLT_DENORM_MIN__
    PREPROC_ADD_MACRO(info, __FLT_DENORM_MIN__);
#endif
#ifdef __FLT_DIG__
    PREPROC_ADD_MACRO(info, __FLT_DIG__);
#endif
#ifdef __FLT_EPSILON__
    PREPROC_ADD_MACRO(info, __FLT_EPSILON__);
#endif
#ifdef __FLT_EVAL_METHOD__
    PREPROC_ADD_MACRO(info, __FLT_EVAL_METHOD__);
#endif
#ifdef __FLT_HAS_DENORM__
    PREPROC_ADD_MACRO(info, __FLT_HAS_DENORM__);
#endif
#ifdef __FLT_HAS_INFINITY__
    PREPROC_ADD_MACRO(info, __FLT_HAS_INFINITY__);
#endif
#ifdef __FLT_HAS_QUIET_NAN__
    PREPROC_ADD_MACRO(info, __FLT_HAS_QUIET_NAN__);
#endif
#ifdef __FLT_MANT_DIG__
    PREPROC_ADD_MACRO(info, __FLT_MANT_DIG__);
#endif
#ifdef __FLT_MAX_10_EXP__
    PREPROC_ADD_MACRO(info, __FLT_MAX_10_EXP__);
#endif
#ifdef __FLT_MAX_EXP__
    PREPROC_ADD_MACRO(info, __FLT_MAX_EXP__);
#endif
#ifdef __FLT_MAX__
    PREPROC_ADD_MACRO(info, __FLT_MAX__);
#endif
#ifdef __FLT_MIN_10_EXP__
    PREPROC_ADD_MACRO(info, __FLT_MIN_10_EXP__);
#endif
#ifdef __FLT_MIN_EXP__
    PREPROC_ADD_MACRO(info, __FLT_MIN_EXP__);
#endif
#ifdef __FLT_MIN__
    PREPROC_ADD_MACRO(info, __FLT_MIN__);
#endif
#ifdef __FLT_RADIX__
    PREPROC_ADD_MACRO(info, __FLT_RADIX__);
#endif
#ifdef __INT16_C_SUFFIX__
    PREPROC_ADD_MACRO(info, __INT16_C_SUFFIX__);
#endif
#ifdef __INT16_FMTd__
    PREPROC_ADD_MACRO(info, __INT16_FMTd__);
#endif
#ifdef __INT16_FMTi__
    PREPROC_ADD_MACRO(info, __INT16_FMTi__);
#endif
#ifdef __INT16_MAX__
    PREPROC_ADD_MACRO(info, __INT16_MAX__);
#endif
#ifdef __INT16_TYPE__
    PREPROC_ADD_MACRO(info, __INT16_TYPE__);
#endif
#ifdef __INT32_C_SUFFIX__
    PREPROC_ADD_MACRO(info, __INT32_C_SUFFIX__);
#endif
#ifdef __INT32_FMTd__
    PREPROC_ADD_MACRO(info, __INT32_FMTd__);
#endif
#ifdef __INT32_FMTi__
    PREPROC_ADD_MACRO(info, __INT32_FMTi__);
#endif
#ifdef __INT32_MAX__
    PREPROC_ADD_MACRO(info, __INT32_MAX__);
#endif
#ifdef __INT32_MAX__
    PREPROC_ADD_MACRO(info, __INT32_MAX__);
#endif
#ifdef __INT32_TYPE__
    PREPROC_ADD_MACRO(info, __INT32_TYPE__);
#endif
#ifdef __INT64_C_SUFFIX__
    PREPROC_ADD_MACRO(info, __INT64_C_SUFFIX__);
#endif
#ifdef __INT64_FMTd__
    PREPROC_ADD_MACRO(info, __INT64_FMTd__);
#endif
#ifdef __INT64_FMTi__
    PREPROC_ADD_MACRO(info, __INT64_FMTi__);
#endif
#ifdef __INT64_MAX__
    PREPROC_ADD_MACRO(info, __INT64_MAX__);
#endif
#ifdef __INT64_MAX__
    PREPROC_ADD_MACRO(info, __INT64_MAX__);
#endif
#ifdef __INT64_TYPE__
    PREPROC_ADD_MACRO(info, __INT64_TYPE__);
#endif
#ifdef __INT8_C_SUFFIX__
    PREPROC_ADD_MACRO(info, __INT8_C_SUFFIX__);
#endif
#ifdef __INT8_FMTd__
    PREPROC_ADD_MACRO(info, __INT8_FMTd__);
#endif
#ifdef __INT8_FMTi__
    PREPROC_ADD_MACRO(info, __INT8_FMTi__);
#endif
#ifdef __INT8_MAX__
    PREPROC_ADD_MACRO(info, __INT8_MAX__);
#endif
#ifdef __INT8_MAX__
    PREPROC_ADD_MACRO(info, __INT8_MAX__);
#endif
#ifdef __INT8_TYPE__
    PREPROC_ADD_MACRO(info, __INT8_TYPE__);
#endif
#ifdef __INTMAX_C_SUFFIX__
    PREPROC_ADD_MACRO(info, __INTMAX_C_SUFFIX__);
#endif
#ifdef __INTMAX_FMTd__
    PREPROC_ADD_MACRO(info, __INTMAX_FMTd__);
#endif
#ifdef __INTMAX_FMTi__
    PREPROC_ADD_MACRO(info, __INTMAX_FMTi__);
#endif
#ifdef __INTMAX_MAX__
    PREPROC_ADD_MACRO(info, __INTMAX_MAX__);
#endif
#ifdef __INTMAX_TYPE__
    PREPROC_ADD_MACRO(info, __INTMAX_TYPE__);
#endif
#ifdef __INTMAX_WIDTH__
    PREPROC_ADD_MACRO(info, __INTMAX_WIDTH__);
#endif
#ifdef __INTPTR_FMTd__
    PREPROC_ADD_MACRO(info, __INTPTR_FMTd__);
#endif
#ifdef __INTPTR_FMTi__
    PREPROC_ADD_MACRO(info, __INTPTR_FMTi__);
#endif
#ifdef __INTPTR_MAX__
    PREPROC_ADD_MACRO(info, __INTPTR_MAX__);
#endif
#ifdef __INTPTR_TYPE__
    PREPROC_ADD_MACRO(info, __INTPTR_TYPE__);
#endif
#ifdef __INTPTR_WIDTH__
    PREPROC_ADD_MACRO(info, __INTPTR_WIDTH__);
#endif
#ifdef __INT_FAST16_FMTd__
    PREPROC_ADD_MACRO(info, __INT_FAST16_FMTd__);
#endif
#ifdef __INT_FAST16_FMTi__
    PREPROC_ADD_MACRO(info, __INT_FAST16_FMTi__);
#endif
#ifdef __INT_FAST16_MAX__
    PREPROC_ADD_MACRO(info, __INT_FAST16_MAX__);
#endif
#ifdef __INT_FAST16_MAX__
    PREPROC_ADD_MACRO(info, __INT_FAST16_MAX__);
#endif
#ifdef __INT_FAST16_TYPE__
    PREPROC_ADD_MACRO(info, __INT_FAST16_TYPE__);
#endif
#ifdef __INT_FAST16_TYPE__
    PREPROC_ADD_MACRO(info, __INT_FAST16_TYPE__);
#endif
#ifdef __INT_FAST32_FMTd__
    PREPROC_ADD_MACRO(info, __INT_FAST32_FMTd__);
#endif
#ifdef __INT_FAST32_FMTi__
    PREPROC_ADD_MACRO(info, __INT_FAST32_FMTi__);
#endif
#ifdef __INT_FAST32_MAX__
    PREPROC_ADD_MACRO(info, __INT_FAST32_MAX__);
#endif
#ifdef __INT_FAST32_TYPE__
    PREPROC_ADD_MACRO(info, __INT_FAST32_TYPE__);
#endif
#ifdef __INT_FAST64_FMTd__
    PREPROC_ADD_MACRO(info, __INT_FAST64_FMTd__);
#endif
#ifdef __INT_FAST64_FMTi__
    PREPROC_ADD_MACRO(info, __INT_FAST64_FMTi__);
#endif
#ifdef __INT_FAST64_MAX__
    PREPROC_ADD_MACRO(info, __INT_FAST64_MAX__);
#endif
#ifdef __INT_FAST64_TYPE__
    PREPROC_ADD_MACRO(info, __INT_FAST64_TYPE__);
#endif
#ifdef __INT_FAST8_FMTd__
    PREPROC_ADD_MACRO(info, __INT_FAST8_FMTd__);
#endif
#ifdef __INT_FAST8_FMTi__
    PREPROC_ADD_MACRO(info, __INT_FAST8_FMTi__);
#endif
#ifdef __INT_FAST8_MAX__
    PREPROC_ADD_MACRO(info, __INT_FAST8_MAX__);
#endif
#ifdef __INT_FAST8_MAX__
    PREPROC_ADD_MACRO(info, __INT_FAST8_MAX__);
#endif
#ifdef __INT_FAST8_TYPE__
    PREPROC_ADD_MACRO(info, __INT_FAST8_TYPE__);
#endif
#ifdef __INT_LEAST16_FMTd__
    PREPROC_ADD_MACRO(info, __INT_LEAST16_FMTd__);
#endif
#ifdef __INT_LEAST16_FMTi__
    PREPROC_ADD_MACRO(info, __INT_LEAST16_FMTi__);
#endif
#ifdef __INT_LEAST16_MAX__
    PREPROC_ADD_MACRO(info, __INT_LEAST16_MAX__);
#endif
#ifdef __INT_LEAST16_TYPE__
    PREPROC_ADD_MACRO(info, __INT_LEAST16_TYPE__);
#endif
#ifdef __INT_LEAST32_FMTd__
    PREPROC_ADD_MACRO(info, __INT_LEAST32_FMTd__);
#endif
#ifdef __INT_LEAST32_FMTi__
    PREPROC_ADD_MACRO(info, __INT_LEAST32_FMTi__);
#endif
#ifdef __INT_LEAST32_MAX__
    PREPROC_ADD_MACRO(info, __INT_LEAST32_MAX__);
#endif
#ifdef __INT_LEAST32_TYPE__
    PREPROC_ADD_MACRO(info, __INT_LEAST32_TYPE__);
#endif
#ifdef __INT_LEAST64_FMTd__
    PREPROC_ADD_MACRO(info, __INT_LEAST64_FMTd__);
#endif
#ifdef __INT_LEAST64_FMTi__
    PREPROC_ADD_MACRO(info, __INT_LEAST64_FMTi__);
#endif
#ifdef __INT_LEAST64_MAX__
    PREPROC_ADD_MACRO(info, __INT_LEAST64_MAX__);
#endif
#ifdef __INT_LEAST64_TYPE__
    PREPROC_ADD_MACRO(info, __INT_LEAST64_TYPE__);
#endif
#ifdef __INT_LEAST8_FMTd__
    PREPROC_ADD_MACRO(info, __INT_LEAST8_FMTd__);
#endif
#ifdef __INT_LEAST8_FMTi__
    PREPROC_ADD_MACRO(info, __INT_LEAST8_FMTi__);
#endif
#ifdef __INT_LEAST8_MAX__
    PREPROC_ADD_MACRO(info, __INT_LEAST8_MAX__);
#endif
#ifdef __INT_LEAST8_TYPE__
    PREPROC_ADD_MACRO(info, __INT_LEAST8_TYPE__);
#endif
#ifdef __INT_MAX__
    PREPROC_ADD_MACRO(info, __INT_MAX__);
#endif
#ifdef __LDBL_DECIMAL_DIG__
    PREPROC_ADD_MACRO(info, __LDBL_DECIMAL_DIG__);
#endif
#ifdef __LDBL_DENORM_MIN__
    PREPROC_ADD_MACRO(info, __LDBL_DENORM_MIN__);
#endif
#ifdef __LDBL_DIG__
    PREPROC_ADD_MACRO(info, __LDBL_DIG__);
#endif
#ifdef __LDBL_EPSILON__
    PREPROC_ADD_MACRO(info, __LDBL_EPSILON__);
#endif
#ifdef __LDBL_HAS_DENORM__
    PREPROC_ADD_MACRO(info, __LDBL_HAS_DENORM__);
#endif
#ifdef __LDBL_HAS_INFINITY__
    PREPROC_ADD_MACRO(info, __LDBL_HAS_INFINITY__);
#endif
#ifdef __LDBL_HAS_QUIET_NAN__
    PREPROC_ADD_MACRO(info, __LDBL_HAS_QUIET_NAN__);
#endif
#ifdef __LDBL_MANT_DIG__
    PREPROC_ADD_MACRO(info, __LDBL_MANT_DIG__);
#endif
#ifdef __LDBL_MAX_10_EXP__
    PREPROC_ADD_MACRO(info, __LDBL_MAX_10_EXP__);
#endif
#ifdef __LDBL_MAX_EXP__
    PREPROC_ADD_MACRO(info, __LDBL_MAX_EXP__);
#endif
#ifdef __LDBL_MAX__
    PREPROC_ADD_MACRO(info, __LDBL_MAX__);
#endif
#ifdef __LDBL_MIN_10_EXP__
    PREPROC_ADD_MACRO(info, __LDBL_MIN_10_EXP__);
#endif
#ifdef __LDBL_MIN_EXP__
    PREPROC_ADD_MACRO(info, __LDBL_MIN_EXP__);
#endif
#ifdef __LDBL_MIN__
    PREPROC_ADD_MACRO(info, __LDBL_MIN__);
#endif
#ifdef __LONG_LONG_MAX__
    PREPROC_ADD_MACRO(info, __LONG_LONG_MAX__);
#endif
#ifdef __LONG_MAX__
    PREPROC_ADD_MACRO(info, __LONG_MAX__);
#endif
#ifdef __PTRDIFF_FMTd__
    PREPROC_ADD_MACRO(info, __PTRDIFF_FMTd__);
#endif
#ifdef __PTRDIFF_FMTi__
    PREPROC_ADD_MACRO(info, __PTRDIFF_FMTi__);
#endif
#ifdef __PTRDIFF_MAX__
    PREPROC_ADD_MACRO(info, __PTRDIFF_MAX__);
#endif
#ifdef __PTRDIFF_TYPE__
    PREPROC_ADD_MACRO(info, __PTRDIFF_TYPE__);
#endif
#ifdef __PTRDIFF_WIDTH__
    PREPROC_ADD_MACRO(info, __PTRDIFF_WIDTH__);
#endif
#ifdef __SIZEOF_DOUBLE__
    PREPROC_ADD_MACRO(info, __SIZEOF_DOUBLE__);
#endif
#ifdef __SIZEOF_FLOAT128__
    PREPROC_ADD_MACRO(info, __SIZEOF_FLOAT128__);
#endif
#ifdef __SIZEOF_FLOAT80__
    PREPROC_ADD_MACRO(info, __SIZEOF_FLOAT80__);
#endif
#ifdef __SIZEOF_FLOAT__
    PREPROC_ADD_MACRO(info, __SIZEOF_FLOAT__);
#endif
#ifdef __SIZEOF_INT128__
    PREPROC_ADD_MACRO(info, __SIZEOF_INT128__);
#endif
#ifdef __SIZEOF_INT__
    PREPROC_ADD_MACRO(info, __SIZEOF_INT__);
#endif
#ifdef __SIZEOF_LONG_DOUBLE__
    PREPROC_ADD_MACRO(info, __SIZEOF_LONG_DOUBLE__);
#endif
#ifdef __SIZEOF_LONG_LONG__
    PREPROC_ADD_MACRO(info, __SIZEOF_LONG_LONG__);
#endif
#ifdef __SIZEOF_LONG__
    PREPROC_ADD_MACRO(info, __SIZEOF_LONG__);
#endif
#ifdef __SIZEOF_PTRDIFF_T__
    PREPROC_ADD_MACRO(info, __SIZEOF_PTRDIFF_T__);
#endif
#ifdef __SIZEOF_SHORT__
    PREPROC_ADD_MACRO(info, __SIZEOF_SHORT__);
#endif
#ifdef __SIZEOF_SIZE_T__
    PREPROC_ADD_MACRO(info, __SIZEOF_SIZE_T__);
#endif
#ifdef __SIZEOF_WCHAR_T__
    PREPROC_ADD_MACRO(info, __SIZEOF_WCHAR_T__);
#endif
#ifdef __SIZEOF_WINT_T__
    PREPROC_ADD_MACRO(info, __SIZEOF_WINT_T__);
#endif
#ifdef __SIZE_FMTX__
    PREPROC_ADD_MACRO(info, __SIZE_FMTX__);
#endif
#ifdef __SIZE_FMTo__
    PREPROC_ADD_MACRO(info, __SIZE_FMTo__);
#endif
#ifdef __SIZE_FMTu__
    PREPROC_ADD_MACRO(info, __SIZE_FMTu__);
#endif
#ifdef __SIZE_FMTx__
    PREPROC_ADD_MACRO(info, __SIZE_FMTx__);
#endif
#ifdef __SIZE_MAX__
    PREPROC_ADD_MACRO(info, __SIZE_MAX__);
#endif
#ifdef __SIZE_TYPE__
    PREPROC_ADD_MACRO(info, __SIZE_TYPE__);
#endif
#ifdef __SIZE_WIDTH__
    PREPROC_ADD_MACRO(info, __SIZE_WIDTH__);
#endif
#ifdef __UINT16_C_SUFFIX__
    PREPROC_ADD_MACRO(info, __UINT16_C_SUFFIX__);
#endif
#ifdef __UINT16_FMTX__
    PREPROC_ADD_MACRO(info, __UINT16_FMTX__);
#endif
#ifdef __UINT16_FMTo__
    PREPROC_ADD_MACRO(info, __UINT16_FMTo__);
#endif
#ifdef __UINT16_FMTu__
    PREPROC_ADD_MACRO(info, __UINT16_FMTu__);
#endif
#ifdef __UINT16_FMTx__
    PREPROC_ADD_MACRO(info, __UINT16_FMTx__);
#endif
#ifdef __UINT16_MAX__
    PREPROC_ADD_MACRO(info, __UINT16_MAX__);
#endif
#ifdef __UINT16_MAX__
    PREPROC_ADD_MACRO(info, __UINT16_MAX__);
#endif
#ifdef __UINT16_TYPE__
    PREPROC_ADD_MACRO(info, __UINT16_TYPE__);
#endif
#ifdef __UINT32_C_SUFFIX__
    PREPROC_ADD_MACRO(info, __UINT32_C_SUFFIX__);
#endif
#ifdef __UINT32_FMTX__
    PREPROC_ADD_MACRO(info, __UINT32_FMTX__);
#endif
#ifdef __UINT32_FMTo__
    PREPROC_ADD_MACRO(info, __UINT32_FMTo__);
#endif
#ifdef __UINT32_FMTu__
    PREPROC_ADD_MACRO(info, __UINT32_FMTu__);
#endif
#ifdef __UINT32_FMTx__
    PREPROC_ADD_MACRO(info, __UINT32_FMTx__);
#endif
#ifdef __UINT32_MAX__
    PREPROC_ADD_MACRO(info, __UINT32_MAX__);
#endif
#ifdef __UINT32_TYPE__
    PREPROC_ADD_MACRO(info, __UINT32_TYPE__);
#endif
#ifdef __UINT64_C_SUFFIX__
    PREPROC_ADD_MACRO(info, __UINT64_C_SUFFIX__);
#endif
#ifdef __UINT64_FMTX__
    PREPROC_ADD_MACRO(info, __UINT64_FMTX__);
#endif
#ifdef __UINT64_FMTo__
    PREPROC_ADD_MACRO(info, __UINT64_FMTo__);
#endif
#ifdef __UINT64_FMTu__
    PREPROC_ADD_MACRO(info, __UINT64_FMTu__);
#endif
#ifdef __UINT64_FMTx__
    PREPROC_ADD_MACRO(info, __UINT64_FMTx__);
#endif
#ifdef __UINT64_MAX__
    PREPROC_ADD_MACRO(info, __UINT64_MAX__);
#endif
#ifdef __UINT64_TYPE__
    PREPROC_ADD_MACRO(info, __UINT64_TYPE__);
#endif
#ifdef __UINT8_C_SUFFIX__
    PREPROC_ADD_MACRO(info, __UINT8_C_SUFFIX__);
#endif
#ifdef __UINT8_FMTX__
    PREPROC_ADD_MACRO(info, __UINT8_FMTX__);
#endif
#ifdef __UINT8_FMTo__
    PREPROC_ADD_MACRO(info, __UINT8_FMTo__);
#endif
#ifdef __UINT8_FMTu__
    PREPROC_ADD_MACRO(info, __UINT8_FMTu__);
#endif
#ifdef __UINT8_FMTx__
    PREPROC_ADD_MACRO(info, __UINT8_FMTx__);
#endif
#ifdef __UINT8_MAX__
    PREPROC_ADD_MACRO(info, __UINT8_MAX__);
#endif
#ifdef __UINT8_TYPE__
    PREPROC_ADD_MACRO(info, __UINT8_TYPE__);
#endif
#ifdef __UINTMAX_C_SUFFIX__
    PREPROC_ADD_MACRO(info, __UINTMAX_C_SUFFIX__);
#endif
#ifdef __UINTMAX_FMTX__
    PREPROC_ADD_MACRO(info, __UINTMAX_FMTX__);
#endif
#ifdef __UINTMAX_FMTo__
    PREPROC_ADD_MACRO(info, __UINTMAX_FMTo__);
#endif
#ifdef __UINTMAX_FMTu__
    PREPROC_ADD_MACRO(info, __UINTMAX_FMTu__);
#endif
#ifdef __UINTMAX_FMTx__
    PREPROC_ADD_MACRO(info, __UINTMAX_FMTx__);
#endif
#ifdef __UINTMAX_MAX__
    PREPROC_ADD_MACRO(info, __UINTMAX_MAX__);
#endif
#ifdef __UINTMAX_TYPE__
    PREPROC_ADD_MACRO(info, __UINTMAX_TYPE__);
#endif
#ifdef __UINTMAX_WIDTH__
    PREPROC_ADD_MACRO(info, __UINTMAX_WIDTH__);
#endif
#ifdef __UINTPTR_FMTX__
    PREPROC_ADD_MACRO(info, __UINTPTR_FMTX__);
#endif
#ifdef __UINTPTR_FMTo__
    PREPROC_ADD_MACRO(info, __UINTPTR_FMTo__);
#endif
#ifdef __UINTPTR_FMTu__
    PREPROC_ADD_MACRO(info, __UINTPTR_FMTu__);
#endif
#ifdef __UINTPTR_FMTx__
    PREPROC_ADD_MACRO(info, __UINTPTR_FMTx__);
#endif
#ifdef __UINTPTR_MAX__
    PREPROC_ADD_MACRO(info, __UINTPTR_MAX__);
#endif
#ifdef __UINTPTR_TYPE__
    PREPROC_ADD_MACRO(info, __UINTPTR_TYPE__);
#endif
#ifdef __UINTPTR_WIDTH__
    PREPROC_ADD_MACRO(info, __UINTPTR_WIDTH__);
#endif
#ifdef __UINT_FAST16_FMTX__
    PREPROC_ADD_MACRO(info, __UINT_FAST16_FMTX__);
#endif
#ifdef __UINT_FAST16_FMTo__
    PREPROC_ADD_MACRO(info, __UINT_FAST16_FMTo__);
#endif
#ifdef __UINT_FAST16_FMTu__
    PREPROC_ADD_MACRO(info, __UINT_FAST16_FMTu__);
#endif
#ifdef __UINT_FAST16_FMTx__
    PREPROC_ADD_MACRO(info, __UINT_FAST16_FMTx__);
#endif
#ifdef __UINT_FAST16_MAX__
    PREPROC_ADD_MACRO(info, __UINT_FAST16_MAX__);
#endif
#ifdef __UINT_FAST16_TYPE__
    PREPROC_ADD_MACRO(info, __UINT_FAST16_TYPE__);
#endif
#ifdef __UINT_FAST32_FMTX__
    PREPROC_ADD_MACRO(info, __UINT_FAST32_FMTX__);
#endif
#ifdef __UINT_FAST32_FMTo__
    PREPROC_ADD_MACRO(info, __UINT_FAST32_FMTo__);
#endif
#ifdef __UINT_FAST32_FMTu__
    PREPROC_ADD_MACRO(info, __UINT_FAST32_FMTu__);
#endif
#ifdef __UINT_FAST32_FMTx__
    PREPROC_ADD_MACRO(info, __UINT_FAST32_FMTx__);
#endif
#ifdef __UINT_FAST32_MAX__
    PREPROC_ADD_MACRO(info, __UINT_FAST32_MAX__);
#endif
#ifdef __UINT_FAST32_TYPE__
    PREPROC_ADD_MACRO(info, __UINT_FAST32_TYPE__);
#endif
#ifdef __UINT_FAST64_FMTX__
    PREPROC_ADD_MACRO(info, __UINT_FAST64_FMTX__);
#endif
#ifdef __UINT_FAST64_FMTo__
    PREPROC_ADD_MACRO(info, __UINT_FAST64_FMTo__);
#endif
#ifdef __UINT_FAST64_FMTu__
    PREPROC_ADD_MACRO(info, __UINT_FAST64_FMTu__);
#endif
#ifdef __UINT_FAST64_FMTx__
    PREPROC_ADD_MACRO(info, __UINT_FAST64_FMTx__);
#endif
#ifdef __UINT_FAST64_MAX__
    PREPROC_ADD_MACRO(info, __UINT_FAST64_MAX__);
#endif
#ifdef __UINT_FAST64_TYPE__
    PREPROC_ADD_MACRO(info, __UINT_FAST64_TYPE__);
#endif
#ifdef __UINT_FAST8_FMTX__
    PREPROC_ADD_MACRO(info, __UINT_FAST8_FMTX__);
#endif
#ifdef __UINT_FAST8_FMTo__
    PREPROC_ADD_MACRO(info, __UINT_FAST8_FMTo__);
#endif
#ifdef __UINT_FAST8_FMTu__
    PREPROC_ADD_MACRO(info, __UINT_FAST8_FMTu__);
#endif
#ifdef __UINT_FAST8_FMTx__
    PREPROC_ADD_MACRO(info, __UINT_FAST8_FMTx__);
#endif
#ifdef __UINT_FAST8_MAX__
    PREPROC_ADD_MACRO(info, __UINT_FAST8_MAX__);
#endif
#ifdef __UINT_FAST8_TYPE__
    PREPROC_ADD_MACRO(info, __UINT_FAST8_TYPE__);
#endif
#ifdef __UINT_LEAST16_FMTX__
    PREPROC_ADD_MACRO(info, __UINT_LEAST16_FMTX__);
#endif
#ifdef __UINT_LEAST16_FMTo__
    PREPROC_ADD_MACRO(info, __UINT_LEAST16_FMTo__);
#endif
#ifdef __UINT_LEAST16_FMTu__
    PREPROC_ADD_MACRO(info, __UINT_LEAST16_FMTu__);
#endif
#ifdef __UINT_LEAST16_FMTx__
    PREPROC_ADD_MACRO(info, __UINT_LEAST16_FMTx__);
#endif
#ifdef __UINT_LEAST16_MAX__
    PREPROC_ADD_MACRO(info, __UINT_LEAST16_MAX__);
#endif
#ifdef __UINT_LEAST16_TYPE__
    PREPROC_ADD_MACRO(info, __UINT_LEAST16_TYPE__);
#endif
#ifdef __UINT_LEAST32_FMTX__
    PREPROC_ADD_MACRO(info, __UINT_LEAST32_FMTX__);
#endif
#ifdef __UINT_LEAST32_FMTo__
    PREPROC_ADD_MACRO(info, __UINT_LEAST32_FMTo__);
#endif
#ifdef __UINT_LEAST32_FMTu__
    PREPROC_ADD_MACRO(info, __UINT_LEAST32_FMTu__);
#endif
#ifdef __UINT_LEAST32_FMTx__
    PREPROC_ADD_MACRO(info, __UINT_LEAST32_FMTx__);
#endif
#ifdef __UINT_LEAST32_MAX__
    PREPROC_ADD_MACRO(info, __UINT_LEAST32_MAX__);
#endif
#ifdef __UINT_LEAST32_TYPE__
    PREPROC_ADD_MACRO(info, __UINT_LEAST32_TYPE__);
#endif
#ifdef __UINT_LEAST64_FMTX__
    PREPROC_ADD_MACRO(info, __UINT_LEAST64_FMTX__);
#endif
#ifdef __UINT_LEAST64_FMTo__
    PREPROC_ADD_MACRO(info, __UINT_LEAST64_FMTo__);
#endif
#ifdef __UINT_LEAST64_FMTu__
    PREPROC_ADD_MACRO(info, __UINT_LEAST64_FMTu__);
#endif
#ifdef __UINT_LEAST64_FMTx__
    PREPROC_ADD_MACRO(info, __UINT_LEAST64_FMTx__);
#endif
#ifdef __UINT_LEAST64_MAX__
    PREPROC_ADD_MACRO(info, __UINT_LEAST64_MAX__);
#endif
#ifdef __UINT_LEAST64_TYPE__
    PREPROC_ADD_MACRO(info, __UINT_LEAST64_TYPE__);
#endif
#ifdef __UINT_LEAST8_FMTX__
    PREPROC_ADD_MACRO(info, __UINT_LEAST8_FMTX__);
#endif
#ifdef __UINT_LEAST8_FMTo__
    PREPROC_ADD_MACRO(info, __UINT_LEAST8_FMTo__);
#endif
#ifdef __UINT_LEAST8_FMTu__
    PREPROC_ADD_MACRO(info, __UINT_LEAST8_FMTu__);
#endif
#ifdef __UINT_LEAST8_FMTx__
    PREPROC_ADD_MACRO(info, __UINT_LEAST8_FMTx__);
#endif
#ifdef __UINT_LEAST8_MAX__
    PREPROC_ADD_MACRO(info, __UINT_LEAST8_MAX__);
#endif
#ifdef __UINT_LEAST8_TYPE__
    PREPROC_ADD_MACRO(info, __UINT_LEAST8_TYPE__);
#endif
#ifdef __WCHAR_MAX__
    PREPROC_ADD_MACRO(info, __WCHAR_MAX__);
#endif
#ifdef __WCHAR_MIN__
    PREPROC_ADD_MACRO(info, __WCHAR_MIN__);
#endif
#ifdef __WCHAR_TYPE__
    PREPROC_ADD_MACRO(info, __WCHAR_TYPE__);
#endif
#ifdef __WCHAR_WIDTH__
    PREPROC_ADD_MACRO(info, __WCHAR_WIDTH__);
#endif
#ifdef __WINT_MAX__
    PREPROC_ADD_MACRO(info, __WINT_MAX__);
#endif
#ifdef __WINT_MIN__
    PREPROC_ADD_MACRO(info, __WINT_MIN__);
#endif
#ifdef __WINT_TYPE__
    PREPROC_ADD_MACRO(info, __WINT_TYPE__);
#endif
#ifdef __WINT_WIDTH__
    PREPROC_ADD_MACRO(info, __WINT_WIDTH__);
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
  StringTokenizer token;
  MacroInfo **macro_p;
  MacroInfo *macro;

  vtkParse_InitTokenizer(&token, name, WS_PREPROC);
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
  StringTokenizer token;
  MacroInfo *macro;

  vtkParse_InitTokenizer(&token, name, WS_PREPROC);
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
  StringTokenizer token;

  vtkParse_InitTokenizer(&token, name, WS_PREPROC);

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
  int empty_variadic = 0;
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
          cp += vtkParse_SkipQuotes(cp);
        }
        else if (cp[0] == '/' && (cp[1] == '*' || cp[1] == '/'))
        {
          cp += vtkParse_SkipComment(cp);
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

    /* one arg that is only whitespace can also be no args*/
    if (macro->NumberOfParameters == 0 && n == 1)
    {
      const char *tp = values[0];
      tp += vtkParse_SkipWhitespace(tp, WS_PREPROC);
      if (tp + 1 >= values[1])
      {
        n = 0;
      }
    }

    /* allow the variadic arg to be empty */
    if (macro->IsVariadic && n == macro->NumberOfParameters-1)
    {
      empty_variadic = 1;
    }

    /* check for correct number of arguments */
    if (n < (macro->NumberOfParameters - empty_variadic) ||
        (n > macro->NumberOfParameters && !macro->IsVariadic))
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
    while (!vtkParse_CharType(*cp, CPRE_ID) && *cp != '\0')
    {
      dp = cp;
      cp += vtkParse_SkipWhitespace(cp, WS_PREPROC);
      if (cp > dp)
      {
        dp = cp;
      }
      else if (vtkParse_CharType(*cp, CPRE_QUOTE))
      {
        cp += vtkParse_SkipQuotes(cp);
        dp = cp;
        wp = cp;
        noexpand = 0;
      }
      else if (vtkParse_CharType(*cp, CPRE_DIGIT))
      {
        cp += vtkParse_SkipNumber(cp);
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
        cp += vtkParse_SkipWhitespace(cp, WS_PREPROC);
        break;
      }
      else if (*cp == '#')
      {
        stringify = 1;
        dp = cp;
        wp = cp;
        cp++;
        cp += vtkParse_SkipWhitespace(cp, WS_PREPROC);
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
    l = vtkParse_SkipId(cp);
    cp += l;
    if (l > 0)
    {
      for (j = 0; j < macro->NumberOfParameters; j++)
      {
        /* check whether the name matches a parameter */
        if (strncmp(pp, macro->Parameters[j], l) == 0 &&
            macro->Parameters[j][l] == '\0')
        {
          if (macro->IsVariadic && j == macro->NumberOfParameters-1)
          {
            /* if variadic arg, use all remaining args */
            pp = values[j] - empty_variadic;
            l = values[n] - pp - 1;
          }
          else
          {
            /* else just get one arg */
            pp = values[j];
            l = values[j+1] - pp - 1;
          }
          /* remove leading whitespace from argument */
          c = *pp;
          while (vtkParse_CharType(c, CPRE_WHITE))
          {
            c = *(++pp);
            l--;
          }
          /* remove trailing whitespace from argument */
          if (l > 0)
          {
            c = pp[l - 1];
            while (vtkParse_CharType(c, CPRE_WHITE))
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
          wp += vtkParse_SkipWhitespace(wp, WS_PREPROC);
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
        /* convert argument into a string, due to "#" */
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
      else if (empty_variadic && j == macro->NumberOfParameters-1)
      {
        /* remove trailing comma before empty variadic (non-standard) */
        k = i;
        if (k > 0)
        {
          do
          {
            c = rp[--k];
          }
          while (k > 0 && vtkParse_CharType(c, CPRE_WHITE));
          if (rp[k] == ',')
          {
            i = k;
          }
        }
      }
      else if (noexpand)
      {
        /* do not expand args that will be concatenated with "##" */
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
  StringTokenizer tokens;
  vtkParse_InitTokenizer(&tokens, text, WS_PREPROC);

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
      while (*cp != '\"' && l > 1) { cp++; l--; }
      if (*cp == '\"' && l > 1) { cp++; l--; }
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
          vtkParse_NextToken(&tokens);
          if (tokens.tok == '(')
          {
            int depth = 1;
            args = tokens.text;
            while (depth > 0 && vtkParse_NextToken(&tokens))
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
    if (vtkParse_NextToken(&tokens) && tokens.text > cp + l)
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
    vtkParse_CacheString(info->Strings, name, strlen(name));
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
  macro->IsVariadic = 0;
  macro->IsExternal = 0;
  macro->IsExcluded = 0;
}

/**
 * Free a preprocessor macro struct
 */
void vtkParsePreprocess_FreeMacro(MacroInfo *macro)
{
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
  info->Strings = NULL;
  info->IsExternal = 0;
  info->ConditionalDepth = 0;
  info->ConditionalDone = 0;

  if (filename)
  {
    char *cp = (char *)malloc(strlen(filename) + 1);
    strcpy(cp, filename);
    info->FileName = cp;
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

  free((char **)info->IncludeDirectories);
  free((char **)info->IncludeFiles);

  free(info);
}
