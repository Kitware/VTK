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

/** Preprocessor tokens. */
enum _preproc_token_t
{
  TOK_ID = 258,
  TOK_CHAR,
  TOK_STRING,
  TOK_NUMBER,
  TOK_AND,
  TOK_OR,
  TOK_NE,
  TOK_EQ,
  TOK_GE,
  TOK_LE,
  TOK_LSHIFT,
  TOK_RSHIFT,
  TOK_DBLHASH,
  TOK_ELLIPSIS,
  TOK_OTHER
};

/** A struct for going through the input one token at a time. */
typedef struct _preproc_tokenizer
{
  int tok;
  size_t len;
  const char *text;
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

/** Skip over whitespace, but not newlines unless preceeded by backlash. */
static void preproc_skip_whitespace(const char **cpp)
{
  const char *cp = *cpp;

  for (;;)
    {
    while (*cp == ' ' || *cp == '\t' || *cp == '\r') { cp++; }

    if (cp[0] == '\\' && cp[1] == '\n')
      {
      cp += 2;
      }
    else if (cp[0] == '\\' && cp[1] == '\r' && cp[2] == '\n')
      {
      cp += 3;
      }
    else if (cp[0] == '/' && (cp[1] == '/' || cp[1] == '*'))
      {
      preproc_skip_comment(&cp);
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

  if (*cp == '\'' || *cp == '\"')
    {
    cp++;
    while (*cp != qc && *cp != '\n' && *cp != '\0')
      {
      if (cp[0] == '\\' && cp[1] == qc) { cp++; }
      cp++;
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

  if ((*cp >= 'a' && *cp <= 'z') ||
      (*cp >= 'A' && *cp <= 'Z') ||
      (*cp == '_'))
    {
    cp++;
    while ((*cp >= '0' && *cp <= '9') ||
           (*cp >= 'a' && *cp <= 'z') ||
           (*cp >= 'A' && *cp <= 'Z') ||
           (*cp == '_'))
      {
      cp++;
      }
    }

  *cpp = cp;
}

/** Skip over a number. */
static void preproc_skip_number(const char **cpp)
{
  const char *cp = *cpp;

  if ((cp[0] >= '0' && cp[0] <= '9') ||
      (cp[0] == '.' && (cp[1] >= '0' && cp[1] <= '9')))
    {
    cp++;
    while ((*cp >= '0' && *cp <= '9') ||
           (*cp >= 'a' && *cp <= 'z') ||
           (*cp >= 'A' && *cp <= 'Z') ||
           *cp == '_' || *cp == '.')
      {
      char c = *cp++;
      if (c == 'e' || c == 'E' ||
          c == 'p' || c == 'P')
        {
        if (*cp == '-' || *cp == '+') { cp++; }
        }
      }
    }

  *cpp = cp;
}

/** Return the next preprocessor token, or '0' if none left. */
static int preproc_next(preproc_tokenizer *tokens)
{
  const char *cp = tokens->text + tokens->len;
  preproc_skip_whitespace(&cp);
  tokens->text = cp;

  if (cp[0] == '_' ||
      (cp[0] >= 'a' && cp[0] <= 'z') ||
      (cp[0] >= 'A' && cp[0] <= 'Z'))
    {
    const char *ep = cp;
    preproc_skip_name(&ep);
    tokens->len = ep - cp;
    tokens->tok = TOK_ID;
    }
  else if ((cp[0] >= '0' && cp[0] <= '9') ||
           (cp[0] == '.' && (cp[1] >= '0' && cp[1] <= '9')))
    {
    const char *ep = cp;
    preproc_skip_number(&ep);
    tokens->len = ep - cp;
    tokens->tok = TOK_NUMBER;
    }
  else if (cp[0] == '\'')
    {
    const char *ep = cp;
    preproc_skip_quotes(&ep);
    tokens->len = ep - cp;
    tokens->tok = TOK_CHAR;
    }
  else if (cp[0] == '\"')
    {
    const char *ep = cp;
    preproc_skip_quotes(&ep);
    tokens->len = ep - cp;
    tokens->tok = TOK_STRING;
    }
  else
    {
    switch (cp[0])
      {
      case ':':
        if (cp[1] == ':') { tokens->len = 2; tokens->tok = TOK_OTHER; }
        else { tokens->len = 1; tokens->tok = cp[0]; }
        break;
      case '.':
        if (cp[1] == '.' && cp[2] == '.')
          { tokens->len = 3; tokens->tok = TOK_ELLIPSIS; }
        else if (cp[1] == '*') { tokens->len = 2; tokens->tok = TOK_OTHER; }
        else { tokens->len = 1; tokens->tok = cp[0]; }
        break;
      case '=':
        if (cp[1] == '=') { tokens->len = 2; tokens->tok = TOK_EQ; }
        else { tokens->len = 1; tokens->tok = cp[0]; }
        break;
      case '!':
        if (cp[1] == '=') { tokens->len = 2; tokens->tok = TOK_NE; }
        else { tokens->len = 1; tokens->tok = cp[0]; }
        break;
      case '<':
        if (cp[1] == '<' && cp[2] == '=')
          { tokens->len = 3; tokens->tok = TOK_OTHER; }
        else if (cp[1] == '<') { tokens->len = 2; tokens->tok = TOK_RSHIFT; }
        else if (cp[1] == '=') { tokens->len = 2; tokens->tok = TOK_LE; }
        else { tokens->len = 1; tokens->tok = cp[0]; }
        break;
      case '>':
        if (cp[1] == '>' && cp[2] == '=')
          { tokens->len = 3; tokens->tok = TOK_OTHER; }
        else if (cp[1] == '>') { tokens->len = 2; tokens->tok = TOK_LSHIFT; }
        else if (cp[1] == '=') { tokens->len = 2; tokens->tok = TOK_GE; }
        else { tokens->len = 1; tokens->tok = cp[0]; }
        break;
      case '&':
        if (cp[1] == '&' && cp[2] == '=')
          { tokens->len = 3; tokens->tok = TOK_OTHER; }
        else if (cp[1] == '=') { tokens->len = 2; tokens->tok = TOK_OTHER; }
        else if (cp[1] == '&') { tokens->len = 2; tokens->tok = TOK_AND; }
        else { tokens->len = 1; tokens->tok = cp[0]; }
        break;
      case '|':
        if (cp[1] == '|' && cp[2] == '=')
          { tokens->len = 3; tokens->tok = TOK_OTHER; }
        else if (cp[1] == '=') { tokens->len = 2; tokens->tok = TOK_OTHER; }
        else if (cp[1] == '|') { tokens->len = 2; tokens->tok = TOK_OR; }
        else { tokens->len = 1; tokens->tok = cp[0]; }
        break;
      case '^': case '*': case '/': case '%':
        if (cp[1] == '=') { tokens->len = 2; tokens->tok = TOK_OTHER; }
        else { tokens->len = 1; tokens->tok = cp[0]; }
        break;
      case '+':
        if (cp[1] == '+') { tokens->len = 2; tokens->tok = TOK_OTHER; }
        else if (cp[1] == '=') { tokens->len = 2; tokens->tok = TOK_OTHER; }
        else { tokens->len = 1; tokens->tok = cp[0]; }
        break;
      case '-':
        if (cp[1] == '>' && cp[2] == '*')
          { tokens->len = 3; tokens->tok = TOK_OTHER; }
        else if (cp[1] == '>') { tokens->len = 2; tokens->tok = TOK_OTHER; }
        else if (cp[1] == '-') { tokens->len = 2; tokens->tok = TOK_OTHER; }
        else if (cp[1] == '=') { tokens->len = 2; tokens->tok = TOK_OTHER; }
        else { tokens->len = 1; tokens->tok = cp[0]; }
        break;
      case '#':
        if (cp[1] == '#') { tokens->len = 2; tokens->tok = TOK_DBLHASH; }
        else { tokens->len = 1; tokens->tok = cp[0]; }
        break;
      case '\n':
      case '\0':
        { tokens->len = 0; tokens->tok = 0; }
        break;
      default:
        { tokens->len = 1; tokens->tok = cp[0]; }
        break;
      }
    }

  return tokens->tok;
}

/** Initialize the tokenizer. */
static void preproc_init(preproc_tokenizer *tokens, const char *text)
{
  tokens->tok = 0;
  tokens->len = 0;
  tokens->text = text;
  preproc_next(tokens);
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

/** Free a preprocessor macro struct. */
static void preproc_free_macro(MacroInfo *info)
{
  free(info);
}

/** Add a preprocessor macro to the PreprocessInfo. */
static void preproc_add_macro(
  PreprocessInfo *info, MacroInfo *macro)
{
  info->Macros = (MacroInfo **)preproc_array_check(
    info->Macros, sizeof(MacroInfo *), info->NumberOfMacros);
  info->Macros[info->NumberOfMacros++] = macro;
}

/** A simple way to add a preprocessor macro definition. */
static MacroInfo *preproc_add_macro_definition(
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
  preproc_add_macro(info, macro);

  return macro;
}

/** Find a preprocessor macro, return 0 if not found. */
static int preproc_find_macro(
  PreprocessInfo *info, const char *name, int *idx)
{
  int i, n;
  size_t m;
  const char *cp = name;

  preproc_skip_name(&cp);
  m = cp - name;

  n = info->NumberOfMacros;
  for (i = 0; i < n; i++)
    {
    if (strncmp(name, info->Macros[i]->Name, m) == 0 &&
        info->Macros[i]->Name[m] == '\0')
      {
      *idx = i;
      return 1;
      }
    }

  *idx = 0;
  return 0;
}

/** Remove a preprocessor macro.  Returns 1 if macro not found. */
static int preproc_remove_macro(
  PreprocessInfo *info, const char *name)
{
  int i, n;

  if (preproc_find_macro(info, name, &i))
    {
    preproc_free_macro(info->Macros[i]);
    n = info->NumberOfMacros-1;
    for (; i < n; i++)
      {
      info->Macros[i] = info->Macros[i+1];
      }
    info->NumberOfMacros = n;
    return 1;
    }

  return 0;
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
        while (*cp >= '0' && *cp <= '7') { cp++; }
        }
      else if (*cp == 'x')
        {
        cp++;
        *val = string_to_preproc_int(cp, 16);
        while ((*cp >= '0' && *cp <= '9') ||
               (*cp >= 'a' && *cp <= 'z') ||
               (*cp >= 'A' && *cp <= 'Z')) { cp++; }
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
    while ((*ep >= '0' && *ep <= '9') ||
           (*ep >= 'a' && *ep <= 'f') ||
           (*ep >= 'A' && *ep <= 'F'))
      {
      ep++;
      }
    }
  else if (cp[0] == '0' && (cp[1] >= '0' && cp[1] <= '9'))
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
    while (*ep >= '0' && *ep <= '9')
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
  else if (tokens->tok == TOK_ID)
    {
    if (strncmp("defined", tokens->text, tokens->len) == 0)
      {
      const char *name;
      int paren = 0;
      preproc_next(tokens);

      if (tokens->tok == '(')
        {
        paren = 1;
        preproc_next(tokens);
        }
      if (tokens->tok != TOK_ID)
        {
#if PREPROC_DEBUG
        fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
        return VTK_PARSE_SYNTAX_ERROR;
        }
      name = tokens->text;
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

      /* do the name lookup */
      *is_unsigned = 0;
      *val = (vtkParsePreprocess_GetMacro(info, name) != NULL);

      return result;
      }
    else
      {
      /* look up and evaluate the macro */
      const char *name = tokens->text;
      MacroInfo *macro = vtkParsePreprocess_GetMacro(info, name);
      preproc_next(tokens);

      if (macro == NULL)
        {
        *val = 0;
        *is_unsigned = 0;
        return VTK_PARSE_MACRO_UNDEFINED;
        }
      else if (macro->IsFunction)
        {
        /* expand function macros using the arguments */
        if (tokens->tok == '(')
          {
          const char *args = tokens->text;
          *val = 0;
          *is_unsigned = 0;
          if (preproc_skip_parentheses(tokens) == VTK_PARSE_OK)
            {
            const char *expansion;
            expansion = vtkParsePreprocess_ExpandMacro(macro, args);
            if (expansion)
              {
              result = vtkParsePreprocess_EvaluateExpression(
                info, expansion, val, is_unsigned);
              vtkParsePreprocess_FreeExpandedMacro(expansion);
              return result;
              }
#if PREPROC_DEBUG
            fprintf(stderr, "wrong number of macro args %d\n", __LINE__);
#endif
            }
#if PREPROC_DEBUG
          fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
          return VTK_PARSE_SYNTAX_ERROR;
          }
        else
          {
#if PREPROC_DEBUG
          fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
          return VTK_PARSE_SYNTAX_ERROR;
          }
        }

      return vtkParsePreprocess_EvaluateExpression(
        info, macro->Definition, val, is_unsigned);
      }
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
    return VTK_PARSE_PREPROC_STRING;
    }

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

    if (op != TOK_RSHIFT && op != TOK_LSHIFT)
      {
      return result;
      }

    preproc_next(tokens);

    result = preproc_evaluate_add(info, tokens, &rval, &rtype);

    if (*is_unsigned)
      {
      if (op == TOK_RSHIFT)
        {
        *val = (preproc_int_t)((preproc_uint_t)*val << rval);
        }
      else if (op == TOK_LSHIFT)
        {
        *val = (preproc_int_t)((preproc_uint_t)*val >> rval);
        }
      }
    else
      {
      if (op == TOK_RSHIFT)
        {
        *val = *val << rval;
        }
      else if (op == TOK_LSHIFT)
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
             tokens->tok != TOK_OR && tokens->tok != TOK_OTHER)
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
             tokens->tok != TOK_OTHER)
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
  int v1, v2;
  int result = VTK_PARSE_OK;

  if (strncmp("if", tokens->text, tokens->len) == 0 ||
      strncmp("ifdef", tokens->text, tokens->len) == 0 ||
      strncmp("ifndef", tokens->text, tokens->len) == 0)
    {
    if (info->ConditionalDepth == 0)
      {
      if (strncmp("if", tokens->text, tokens->len) == 0)
        {
        preproc_next(tokens);
        result = preproc_evaluate_conditional(info, tokens);
        }
      else
        {
        v1 = (strncmp("ifndef", tokens->text, tokens->len) != 0);
        preproc_next(tokens);
        if (tokens->tok != TOK_ID)
          {
#if PREPROC_DEBUG
          fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
          return VTK_PARSE_SYNTAX_ERROR;
          }
        v2 = (vtkParsePreprocess_GetMacro(info, tokens->text) != 0);
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
  else if (strncmp("elif", tokens->text, tokens->len) == 0 ||
           strncmp("else", tokens->text, tokens->len) == 0)
    {
    if (info->ConditionalDepth == 0)
      {
      /* preceeding clause was not skipped, so must skip this one */
      info->ConditionalDepth = 1;
      }
    else if (info->ConditionalDepth == 1 &&
             info->ConditionalDone == 0)
      {
      if (strncmp("elif", tokens->text, tokens->len) == 0)
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
  else if (strncmp("endif", tokens->text, tokens->len) == 0)
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
  MacroInfo *macro;
  int is_function;
  const char *name;
  size_t namelen;
  const char *definition = "";
  int i;
  int n = 0;
  const char **args = NULL;

  if (strncmp("define", tokens->text, tokens->len) == 0)
    {
    preproc_next(tokens);
    if (tokens->tok != TOK_ID)
      {
#if PREPROC_DEBUG
      fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
      return VTK_PARSE_SYNTAX_ERROR;
      }

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
          if (args) { free((char **)args); }
#if PREPROC_DEBUG
          fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
          return VTK_PARSE_SYNTAX_ERROR;
          }

        /* add to the arg list */
        args = (const char **)preproc_array_check(
          (char **)args, sizeof(char *), n);
        args[n++] = preproc_strndup(tokens->text, tokens->len);

        preproc_next(tokens);
        if (tokens->tok == ',')
          {
          preproc_next(tokens);
          }
        else if (tokens->tok != ')')
          {
          if (args) { free((char **)args); }
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
    if (preproc_find_macro(info, name, &i))
      {
      if (args) { free((char **)args); }
#if PREPROC_DEBUG
      fprintf(stderr, "macro redefined %d\n", __LINE__);
#endif
      return VTK_PARSE_MACRO_REDEFINED;
      }

    macro = preproc_add_macro_definition(info, name, definition);
    macro->IsFunction = is_function;
    macro->NumberOfArguments = n;
    macro->Arguments = args;
    return VTK_PARSE_OK;
    }
  else if (strncmp("undef", tokens->text, tokens->len) == 0)
    {
    preproc_next(tokens);
    if (tokens->tok != TOK_ID)
      {
#if PREPROC_DEBUG
      fprintf(stderr, "syntax error %d\n", __LINE__);
#endif
      return VTK_PARSE_SYNTAX_ERROR;
      }
    name = tokens->text;
    preproc_remove_macro(info, name);
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
  info->IncludeFiles[info->NumberOfIncludeFiles++] = name;

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
  while (filename[j] == '_' ||
         (filename[j] >= '0' && filename[j] <= '9') ||
         (filename[j] >= 'a' && filename[j] <= 'z') ||
         (filename[j] >= 'Z' && filename[j] <= 'Z')) { j++; }

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
        else if (tbuf[i] == '\\' && tbuf[i+1] == '\"')
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
      preproc_skip_whitespace(&cp);
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

  if (strncmp("include", tokens->text, tokens->len) == 0)
    {
    preproc_next(tokens);

    cp = tokens->text;

    if (tokens->tok == TOK_ID)
      {
      MacroInfo *macro;
      macro = vtkParsePreprocess_GetMacro(info, cp);
      if (macro && macro->Definition)
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
    if (strncmp("ifdef", tokens.text, tokens.len) == 0 ||
        strncmp("ifndef", tokens.text, tokens.len) == 0 ||
        strncmp("if", tokens.text, tokens.len) == 0 ||
        strncmp("elif", tokens.text, tokens.len) == 0 ||
        strncmp("else", tokens.text, tokens.len) == 0 ||
        strncmp("endif", tokens.text, tokens.len) == 0)
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
      if (strncmp("define", tokens.text, tokens.len) == 0 ||
          strncmp("undef", tokens.text, tokens.len) == 0)
        {
        result = preproc_evaluate_define(info, &tokens);
        }
      else if (strncmp("include", tokens.text, tokens.len) == 0)
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
  int i;
  MacroInfo *macro;

  if (preproc_find_macro(info, name, &i))
    {
    return VTK_PARSE_MACRO_REDEFINED;
    }

  macro = preproc_add_macro_definition(info, name, definition);
  macro->IsExternal = 1;

  return VTK_PARSE_OK;
}

/**
 * Return a preprocessor macro struct, or NULL if not found.
 */
MacroInfo *vtkParsePreprocess_GetMacro(
  PreprocessInfo *info, const char *name)
{
  int i = 0;

  if (preproc_find_macro(info, name, &i))
    {
    return info->Macros[i];
    }

  return NULL;
}

/**
 * Remove a preprocessor macro.
 */
int vtkParsePreprocess_RemoveMacro(
 PreprocessInfo *info, const char *name)
{
  if (preproc_remove_macro(info, name))
    {
    return VTK_PARSE_OK;
    }

  return VTK_PARSE_MACRO_UNDEFINED;
}

/**
 * Expand a function macro
 */
const char *vtkParsePreprocess_ExpandMacro(
  MacroInfo *macro, const char *argstring)
{
  const char *cp = argstring;
  int n = 0;
  int j = 0;
  const char **values = NULL;
  const char *pp = NULL;
  const char *dp = NULL;
  char *rp = NULL;
  size_t rs = 0;
  size_t i = 0;
  size_t l = 0;
  size_t k = 0;
  int stringify = 0;
  int depth = 1;
  int c;

  if (argstring == NULL || *cp != '(')
    {
    return NULL;
    }

  /* break the string into individual argument values */
  values = (const char **)malloc(4*sizeof(const char **));

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
    if (n >= 4 && (n & (n-1)) == 0)
      {
      values = (const char **)realloc(
        (char **)values, 2*n*sizeof(const char **));
      }

    values[n++] = cp;
    }
  --n;

  /* diagnostic: print out the values */
#if PREPROC_DEBUG
  for (j = 0; j < n; j++)
    {
    size_t m = values[j+1] - values[j] - 1;
    fprintf(stderr, "arg %i: %*.*s\n", (int)j, (int)m, (int)m, values[j]);
    }
#endif

  /* allow whitespace as "no argument" */
  if (macro->NumberOfArguments == 0 && n == 1)
    {
    cp = values[0];
    c = *cp;
    while (c == ' ' || c == '\n' || c == '\t' || c == '\r')
      {
      c = *(++cp);
      }
    if (cp + 1 == values[1])
      {
      n = 0;
      }
    }

  if (n != macro->NumberOfArguments)
    {
    free((char **)values);
#if PREPROC_DEBUG
    fprintf(stderr, "wrong number of macro args to %s, %d != %d\n",
            macro->Name, n, macro->NumberOfArguments);
#endif
    return NULL;
    }

  cp = macro->Definition;
  dp = cp;
  if (cp == NULL)
    {
    free((char **)values);
    return NULL;
    }

  rp = (char *)malloc(128);
  rp[0] = '\0';
  rs = 128;

  while (*cp != '\0')
    {
    pp = cp;
    stringify = 0;
    /* skip all chars that aren't part of a name */
    while ((*cp < 'a' || *cp > 'z') &&
           (*cp < 'A' || *cp > 'Z') &&
           *cp != '_' && *cp != '\0')
      {
      if (*cp == '\'' || *cp == '\"')
        {
        preproc_skip_quotes(&cp);
        dp = cp;
        }
      else if (*cp >= '0' && *cp <= '9')
        {
        preproc_skip_number(&cp);
        dp = cp;
        }
      else if (*cp == '/' && (cp[1] == '/' || cp[1] == '*'))
        {
        preproc_skip_comment(&cp);
        dp = cp;
        }
      else if (cp[0] == '#' && cp[1] == '#')
        {
        dp = cp;
        while (dp > pp && (dp[-1] == ' ' || dp[-1] == '\t' ||
                           dp[-1] == '\r' || dp[-1] == '\n'))
          {
          --dp;
          }
        cp += 2;
        while (*cp == ' ' || *cp == '\t' || *cp == '\r' || *cp == '\n')
          {
          cp++;
          }
        break;
        }
      else if (*cp == '#')
        {
        stringify = 1;
        dp = cp;
        cp++;
        while (*cp == ' ' || *cp == '\t' || *cp == '\r' || *cp == '\n')
          {
          cp++;
          }
        break;
        }
      else
        {
        cp++;
        dp = cp;
        }
      }
    l = dp - pp;
    if (l > 0)
      {
      if (i + l + 1 >= rs)
        {
        rs += rs + i + l + 1;
        rp = (char *)realloc(rp, rs);
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
        /* check whether the name matches an argument */
        if (strncmp(pp, macro->Arguments[j], l) == 0 &&
            macro->Arguments[j][l] == '\0')
          {
          /* substitute the argument value */
          l = values[j+1] - values[j] - 1;
          pp = values[j];
          /* remove leading whitespace from argument */
          c = *pp;
          while (c == ' ' || c == '\n' || c == '\t' || c == '\r')
            {
            c = *(++pp);
            l--;
            }
          /* remove trailing whitespace from argument */
          if (l > 0)
            {
            c = pp[l - 1];
            while (c == ' ' || c == '\n' || c == '\t' || c == '\r')
              {
              if (--l == 0)
                {
                break;
                }
              c = pp[l-1];
              }
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
        rp = (char *)realloc(rp, rs);
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
      else
        {
        strncpy(&rp[i], pp, l);
        i += l;
        }
      rp[i] = '\0';
      }
    }

  free((char **)values);

  return rp;
}

/**
 * Free an expanded function macro
 */
void vtkParsePreprocess_FreeExpandedMacro(const char *emacro)
{
  free((char *)emacro);
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
  macro->NumberOfArguments = 0;
  macro->Arguments = NULL;
  macro->IsFunction = 0;
  macro->IsExternal = 0;
}

/**
 * Initialize a preprocessor struct
 */
void vtkParsePreprocess_InitPreprocess(PreprocessInfo *info)
{
  info->FileName = NULL;
  info->NumberOfMacros = 0;
  info->Macros = NULL;
  info->NumberOfIncludeDirectories = 0;
  info->IncludeDirectories = NULL;
  info->NumberOfIncludeFiles = 0;
  info->IncludeFiles = NULL;
  info->IsExternal = 0;
  info->ConditionalDepth = 0;
  info->ConditionalDone = 0;
}
