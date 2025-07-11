// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWrapText.h"
#include "vtkWrap.h"

#include "vtkParseExtras.h"
#include "vtkParseMangle.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------- */
/* Convert special characters in a string into their escape codes
 * so that the string can be quoted in a source file.  The specified
 * maxlen must be at least 32 chars, and should not be over 2047 since
 * that is the maximum length of a string literal on some systems */

const char* vtkWrapText_QuoteString(const char* comment, size_t maxlen)
{
  static char* result = 0;
  static size_t oldmaxlen = 0;
  size_t i = 0;
  size_t j = 0;
  size_t k, n, m;
  unsigned short x;

  if (maxlen > oldmaxlen)
  {
    free(result);
    result = (char*)malloc(maxlen + 1);
    oldmaxlen = maxlen;
  }

  if (comment == NULL)
  {
    return "";
  }

  while (comment[i] != '\0')
  {
    n = 1; /* "n" stores number of input bytes consumed */
    m = 1; /* "m" stores number of output bytes written */

    if ((comment[i] & 0x80) != 0)
    {
      /* check for trailing bytes in utf-8 sequence */
      while ((comment[i + n] & 0xC0) == 0x80)
      {
        n++;
      }

      /* the first two bytes will be used to check for validity */
      x = (((unsigned char)(comment[i]) << 8) | (unsigned char)(comment[i + 1]));

      /* check for valid 2, 3, or 4 byte utf-8 sequences */
      if ((n == 2 && x >= 0xC280 && x < 0xE000) ||
        (n == 3 && x >= 0xE0A0 && x < 0xF000 && (x >= 0xEE80 || x < 0xEDA0)) ||
        (n == 4 && x >= 0xF090 && x < 0xF490))
      {
        /* write the valid utf-8 sequence */
        for (k = 0; k < n; k++)
        {
          snprintf(&result[j + 4 * k], maxlen + 1 - j, "\\%3.3o", (unsigned char)(comment[i + k]));
        }
        m = 4 * n;
      }
      else
      {
        /* bad sequence, write the replacement character code U+FFFD */
        snprintf(&result[j], maxlen + 1 - j, "%s", "\\357\\277\\275");
        m = 12;
      }
    }
    else if (comment[i] == '\"' || comment[i] == '\\')
    {
      result[j] = '\\';
      result[j + 1] = comment[i];
      m = 2;
    }
    else if (isprint(comment[i]))
    {
      result[j] = comment[i];
    }
    else if (comment[i] == '\n')
    {
      result[j] = '\\';
      result[j + 1] = 'n';
      m = 2;
    }
    else
    {
      /* use octal escape sequences for other control codes */
      snprintf(&result[j], maxlen + 1 - j, "\\%3.3o", comment[i]);
      m = 4;
    }

    /* check if output limit is reached */
    if (j + m >= maxlen - 20)
    {
      snprintf(&result[j], maxlen + 1 - j, " ...\\n [Truncated]\\n");
      j += strlen(" ...\\n [Truncated]\\n");
      break;
    }

    i += n;
    j += m;
  }

  result[j] = '\0';

  return result;
}

/* -------------------------------------------------------------------- */
/* A simple string that grows as necessary. */

struct vtkWPString
{
  char* str;
  size_t len;
  size_t maxlen;
};

/* -- append ---------- */
static void vtkWPString_Append(struct vtkWPString* str, const char* text)
{
  size_t n = strlen(text);

  if (str->len + n + 1 > str->maxlen)
  {
    str->maxlen = (str->len + n + 1 + 2 * str->maxlen);
    str->str = (char*)realloc(str->str, str->maxlen);
  }

  memcpy(&str->str[str->len], text, n + 1);
  str->len += n;
}

/* -- add a char ---------- */
static void vtkWPString_PushChar(struct vtkWPString* str, char c)
{
  if (str->len + 2 > str->maxlen)
  {
    str->maxlen = (str->len + 2 + 2 * str->maxlen);
    str->str = (char*)realloc(str->str, str->maxlen);
  }

  str->str[str->len++] = c;
  str->str[str->len] = '\0';
}

/* -- strip any of the given chars from the end ---------- */
static void vtkWPString_Strip(struct vtkWPString* str, const char* trailers)
{
  size_t k = str->len;
  const char* cp = str->str;
  size_t j = 0;
  size_t n;

  if (cp)
  {
    n = strlen(trailers);

    while (k > 0 && j < n)
    {
      for (j = 0; j < n; j++)
      {
        if (cp[k - 1] == trailers[j])
        {
          k--;
          break;
        }
      }
    }

    str->len = k;
    str->str[k] = '\0';
  }
}

/* -- Return the last char ---------- */
static char vtkWPString_LastChar(struct vtkWPString* str)
{
  if (str->str && str->len > 0)
  {
    return str->str[str->len - 1];
  }
  return '\0';
}

/* -- do a linebreak on a method declaration ---------- */
static void vtkWPString_BreakSignatureLine(
  struct vtkWPString* str, size_t* linestart, size_t indentation)
{
  size_t i = 0;
  size_t m = 0;
  size_t j = *linestart;
  size_t l = str->len;
  size_t k = str->len;
  char* text = str->str;
  char delim;

  if (!text)
  {
    return;
  }

  while (
    l > j && text[l - 1] != '\n' && text[l - 1] != ',' && text[l - 1] != '(' && text[l - 1] != ')')
  {
    /* treat each string as a unit */
    if (l > 4 && (text[l - 1] == '\'' || text[l - 1] == '\"'))
    {
      delim = text[l - 1];
      l -= 2;
      while (l > 3 && (text[l - 1] != delim || text[l - 3] == '\\'))
      {
        l--;
        if (text[l - 1] == '\\')
        {
          l--;
        }
      }
      l -= 2;
    }
    else
    {
      l--;
    }
  }

  /* if none of these chars was found, split is impossible */
  if (text[l - 1] != ',' && text[l - 1] != '(' && text[l - 1] != ')' && text[l - 1] != '\n')
  {
    j++;
  }

  else
  {
    /* Append some chars to guarantee size */
    vtkWPString_PushChar(str, '\n');
    vtkWPString_PushChar(str, '\n');
    for (i = 0; i < indentation; i++)
    {
      vtkWPString_PushChar(str, ' ');
    }
    /* re-get the char pointer, it may have been reallocated */
    text = str->str;

    if (k > l)
    {
      m = 0;
      while (m < indentation + 2 && text[l + m] == ' ')
      {
        m++;
      }
      memmove(&text[l + indentation + 2 - m], &text[l], k - l);
      k += indentation + 2 - m;
    }
    else
    {
      k += indentation + 2;
    }
    text[l++] = '\\';
    text[l++] = 'n';
    j = l;
    for (i = 0; i < indentation; i++)
    {
      text[l++] = ' ';
    }
  }

  str->len = k;

  /* return the new line start position */
  *linestart = j;
}

/* -- do a linebreak on regular text ---------- */
static void vtkWPString_BreakCommentLine(struct vtkWPString* str, size_t* linestart, size_t indent)
{
  size_t i = 0;
  size_t j = *linestart;
  size_t l = str->len;
  char* text = str->str;

  if (!text)
  {
    return;
  }

  /* try to break the line at a word */
  while (l > 0 && text[l - 1] != ' ' && text[l - 1] != '\n')
  {
    l--;
  }
  if (l > 0 && text[l - 1] != '\n' && l - j > indent)
  {
    /* replace space with newline */
    text[l - 1] = '\n';
    j = l;

    /* Append some chars to guarantee size */
    vtkWPString_PushChar(str, '\n');
    vtkWPString_PushChar(str, '\n');
    for (i = 0; i < indent; i++)
    {
      vtkWPString_PushChar(str, ' ');
    }
    /* re-get the char pointer, it may have been reallocated */
    text = str->str;
    str->len -= indent + 2;

    if (str->len > l && indent > 0)
    {
      memmove(&text[l + indent], &text[l], str->len - l);
      memset(&text[l], ' ', indent);
      str->len += indent;
    }
  }
  else
  {
    /* long word, just split the word */
    vtkWPString_PushChar(str, '\n');
    j = str->len;
    for (i = 0; i < indent; i++)
    {
      vtkWPString_PushChar(str, ' ');
    }
  }

  /* return the new line start position */
  *linestart = j;
}

/* -------------------------------------------------------------------- */
/* Format a signature to a 70 char linewidth and char limit */
const char* vtkWrapText_FormatSignature(const char* signature, size_t width, size_t maxlen)
{
  static struct vtkWPString staticString = { NULL, 0, 0 };
  struct vtkWPString* text;
  size_t i, j, n;
  const char* cp = signature;
  char delim;
  size_t lastSigStart = 0;
  size_t sigCount = 0;

  text = &staticString;
  text->len = 0;

  if (signature == 0)
  {
    return "";
  }

  i = 0;
  j = 0;

  while (cp[i] != '\0')
  {
    while (text->len - j < width && cp[i] != '\n' && cp[i] != '\0')
    {
      /* escape quotes */
      if (cp[i] == '\"' || cp[i] == '\'')
      {
        delim = cp[i];
        vtkWPString_PushChar(text, '\\');
        vtkWPString_PushChar(text, cp[i++]);
        while (cp[i] != delim && cp[i] != '\0')
        {
          if (cp[i] == '\\')
          {
            vtkWPString_PushChar(text, '\\');
          }
          vtkWPString_PushChar(text, cp[i++]);
        }
        if (cp[i] == delim)
        {
          vtkWPString_PushChar(text, '\\');
          vtkWPString_PushChar(text, cp[i++]);
        }
      }
      /* remove items that trail the closing parenthesis */
      else if (cp[i] == ')')
      {
        vtkWPString_PushChar(text, cp[i++]);
        if (strncmp(&cp[i], " const", 6) == 0)
        {
          i += 6;
        }
        if (strncmp(&cp[i], " = 0", 4) == 0)
        {
          i += 4;
        }
        if (cp[i] == ';')
        {
          i++;
        }
      }
      /* anything else */
      else
      {
        vtkWPString_PushChar(text, cp[i++]);
      }
    }

    /* break the line (try to break after a comma) */
    if (cp[i] != '\n' && cp[i] != '\0')
    {
      vtkWPString_BreakSignatureLine(text, &j, 4);
    }
    /* reached end of line: do next signature */
    else
    {
      vtkWPString_Strip(text, " \r\t");
      if (cp[i] != '\0')
      {
        sigCount++;
        /* if sig count is even, check length against maxlen */
        if ((sigCount & 1) == 0)
        {
          n = strlen(text->str);
          if (n >= maxlen)
          {
            break;
          }
          lastSigStart = n;
        }

        i++;
        vtkWPString_PushChar(text, '\\');
        vtkWPString_PushChar(text, 'n');
      }
      /* mark the position of the start of the line */
      j = text->len;
    }
  }

  vtkWPString_Strip(text, " \r\t");

  if (strlen(text->str) >= maxlen)
  {
    /* terminate before the current signature */
    text->str[lastSigStart] = '\0';
  }

  return text->str;
}

/* -------------------------------------------------------------------- */
/* Format a comment to a 70 char linewidth, in several steps:
 * 1) remove html tags, convert <p> and <br> into breaks
 * 2) remove doxygen tags like \em
 * 3) remove extra whitespace (except paragraph breaks)
 * 4) re-break the lines
 */

const char* vtkWrapText_FormatComment(const char* comment, size_t width)
{
  static struct vtkWPString staticString = { NULL, 0, 0 };
  struct vtkWPString* text;
  const char* cp;
  size_t i, j, l;
  size_t indent = 0;
  int nojoin = 0;
  int start;

  text = &staticString;
  text->len = 0;

  if (comment == 0)
  {
    return "";
  }

  i = 0;
  j = 0;
  l = 0;
  start = 1;
  cp = comment;

  /* skip any leading whitespace */
  while (cp[i] == '\n' || cp[i] == '\r' || cp[i] == '\t' || cp[i] == ' ')
  {
    i++;
  }

  while (cp[i] != '\0')
  {
    /* Add characters until the output line is complete */
    while (cp[i] != '\0' && text->len - j < width)
    {
      /* if the end of the line was found, see how next line begins */
      if (start)
      {
        /* eat the leading space */
        if (cp[i] == ' ')
        {
          i++;
        }

        /* skip ahead to find any interesting first characters */
        l = i;
        while (cp[l] == ' ' || cp[l] == '\t' || cp[l] == '\r')
        {
          l++;
        }

        /* check for new section */
        if (cp[l] == '.' && strncmp(&cp[l], ".SECTION", 8) == 0)
        {
          vtkWPString_Strip(text, "\n");
          if (text->len > 0)
          {
            vtkWPString_PushChar(text, '\n');
            vtkWPString_PushChar(text, '\n');
          }
          i = l + 8;
          while (cp[i] == '\r' || cp[i] == '\t' || cp[i] == ' ')
          {
            i++;
          }
          while (cp[i] != '\n' && cp[i] != '\0')
          {
            vtkWPString_PushChar(text, cp[i++]);
          }
          vtkWPString_Strip(text, " \t\r");

          if (vtkWPString_LastChar(text) != ':')
          {
            vtkWPString_PushChar(text, ':');
          }
          vtkWPString_PushChar(text, '\n');
          vtkWPString_PushChar(text, '\n');
          j = text->len;
          indent = 0;
          if (cp[i] == '\n')
          {
            i++;
          }
          start = 1;
          continue;
        }

        /* handle doxygen tags that appear at start of line */
        if (cp[l] == '\\' || cp[l] == '@')
        {
          if (strncmp(&cp[l + 1], "brief", 5) == 0 || strncmp(&cp[l + 1], "short", 5) == 0 ||
            strncmp(&cp[l + 1], "pre", 3) == 0 || strncmp(&cp[l + 1], "post", 4) == 0 ||
            strncmp(&cp[l + 1], "param", 5) == 0 || strncmp(&cp[l + 1], "tparam", 6) == 0 ||
            strncmp(&cp[l + 1], "cmdparam", 8) == 0 || strncmp(&cp[l + 1], "exception", 9) == 0 ||
            strncmp(&cp[l + 1], "return", 6) == 0 || strncmp(&cp[l + 1], "warning", 7) == 0 ||
            strncmp(&cp[l + 1], "sa", 2) == 0 || strncmp(&cp[l + 1], "li", 2) == 0)
          {
            nojoin = 2;
            indent = 4;
            if (text->len > 0 && vtkWPString_LastChar(text) != '\n')
            {
              vtkWPString_PushChar(text, '\n');
            }
            j = text->len;
            i = l;

            /* remove these two tags from the output text */
            if (strncmp(&cp[l + 1], "brief", 5) == 0 || strncmp(&cp[l + 1], "short", 5) == 0)
            {
              i = l + 6;
              while (cp[i] == ' ')
              {
                i++;
              }
            }
          }
        }

        /* handle bullets and numbering */
        else if (cp[l] == '-' || cp[l] == '*' || cp[l] == '#' ||
          (cp[l] >= '0' && cp[l] <= '9' && (cp[l + 1] == ')' || cp[l + 1] == '.') &&
            cp[l + 2] == ' '))
        {
          indent = 0;
          while (indent < 3 && cp[l + indent] != ' ')
          {
            indent++;
          }
          indent++;
          if (text->len > 0 && vtkWPString_LastChar(text) != '\n')
          {
            vtkWPString_PushChar(text, '\n');
          }
          j = text->len;
          i = l;
        }

        /* keep paragraph breaks */
        else if (cp[l] == '\n')
        {
          i = l + 1;
          vtkWPString_Strip(text, "\n");
          if (text->len > 0)
          {
            vtkWPString_PushChar(text, '\n');
            vtkWPString_PushChar(text, '\n');
          }
          nojoin = 0;
          indent = 0;
          j = text->len;
          start = 1;
          continue;
        }

        /* add newline if nojoin is not set */
        else if (nojoin || (cp[i] == ' ' && !indent))
        {
          if (nojoin == 2)
          {
            nojoin = 0;
            indent = 0;
          }
          vtkWPString_PushChar(text, '\n');
          j = text->len;
        }

        /* do line joining */
        else if (text->len > 0 && vtkWPString_LastChar(text) != '\n')
        {
          i = l;
          vtkWPString_PushChar(text, ' ');
        }
      }

      /* handle quotes */
      if (cp[i] == '\"')
      {
        size_t q = i;
        size_t r = text->len;

        /* try to keep the quote whole */
        vtkWPString_PushChar(text, cp[i++]);
        while (cp[i] != '\"' && cp[i] != '\r' && cp[i] != '\n' && cp[i] != '\0')
        {
          vtkWPString_PushChar(text, cp[i++]);
        }
        /* if line ended before quote did, then reset */
        if (cp[i] != '\"')
        {
          i = q;
          text->len = r;
        }
      }
      else if (cp[i] == '\'')
      {
        size_t q = i;
        size_t r = text->len;

        /* try to keep the quote whole */
        vtkWPString_PushChar(text, cp[i++]);
        while (cp[i] != '\'' && cp[i] != '\r' && cp[i] != '\n' && cp[i] != '\0')
        {
          vtkWPString_PushChar(text, cp[i++]);
        }
        /* if line ended before quote did, then reset */
        if (cp[i] != '\'')
        {
          i = q;
          text->len = r;
        }
      }

      /* handle simple html tags */
      else if (cp[i] == '<')
      {
        l = i + 1;
        if (cp[l] == '/')
        {
          l++;
        }
        while ((cp[l] >= 'a' && cp[l] <= 'z') || (cp[l] >= 'A' && cp[l] <= 'Z'))
        {
          l++;
        }
        if (cp[l] == '>')
        {
          if (cp[i + 1] == 'p' || cp[i + 1] == 'P' || (cp[i + 1] == 'b' && cp[i + 2] == 'r') ||
            (cp[i + 1] == 'B' && cp[i + 2] == 'R'))
          {
            vtkWPString_Strip(text, " \n");
            vtkWPString_PushChar(text, '\n');
            vtkWPString_PushChar(text, '\n');
            j = text->len;
            indent = 0;
          }
          i = l + 1;
          while (cp[i] == '\r' || cp[i] == '\t' || cp[i] == ' ')
          {
            i++;
          }
        }
      }
      else if (cp[i] == '\\' || cp[i] == '@')
      {
        /* handle simple doxygen tags */
        if (strncmp(&cp[i + 1], "em ", 3) == 0)
        {
          i += 4;
        }
        else if (strncmp(&cp[i + 1], "a ", 2) == 0 || strncmp(&cp[i + 1], "e ", 2) == 0 ||
          strncmp(&cp[i + 1], "c ", 2) == 0 || strncmp(&cp[i + 1], "b ", 2) == 0 ||
          strncmp(&cp[i + 1], "p ", 2) == 0 || strncmp(&cp[i + 1], "f$", 2) == 0 ||
          strncmp(&cp[i + 1], "f[", 2) == 0 || strncmp(&cp[i + 1], "f]", 2) == 0)
        {
          if (i > 0 && cp[i - 1] != ' ')
          {
            vtkWPString_PushChar(text, ' ');
          }
          if (cp[i + 1] == 'f')
          {
            if (cp[i + 2] == '$')
            {
              vtkWPString_PushChar(text, '$');
            }
            else
            {
              vtkWPString_PushChar(text, '\\');
              vtkWPString_PushChar(text, cp[i + 2]);
            }
          }
          i += 3;
        }
        else if (cp[i + 1] == '&' || cp[i + 1] == '$' || cp[i + 1] == '#' || cp[i + 1] == '<' ||
          cp[i + 1] == '>' || cp[i + 1] == '%' || cp[i + 1] == '@' || cp[i + 1] == '\\' ||
          cp[i + 1] == '\"')
        {
          i++;
        }
        else if (cp[i + 1] == 'n')
        {
          vtkWPString_Strip(text, " \n");
          vtkWPString_PushChar(text, '\n');
          vtkWPString_PushChar(text, '\n');
          indent = 0;
          i += 2;
          j = text->len;
        }
        else if (strncmp(&cp[i + 1], "brief", 5) == 0)
        {
          i += 6;
          while (cp[i] == ' ' || cp[i] == '\r' || cp[i] == '\t')
          {
            i++;
          }
        }
        else if (strncmp(&cp[i + 1], "code", 4) == 0)
        {
          nojoin = 1;
          i += 5;
          while (cp[i] == ' ' || cp[i] == '\r' || cp[i] == '\t' || cp[i] == '\n')
          {
            i++;
          }
        }
        else if (strncmp(&cp[i + 1], "endcode", 7) == 0)
        {
          nojoin = 0;
          i += 8;
          l = i;
          while (cp[l] == ' ' || cp[l] == '\t' || cp[l] == '\r')
          {
            l++;
          }
          if (cp[l] == '\n')
          {
            i = l;
            vtkWPString_PushChar(text, '\n');
            j = text->len;
          }
        }
        else if (strncmp(&cp[i + 1], "verbatim", 8) == 0)
        {
          i += 9;
          while (cp[i] != '\0' &&
            ((cp[i] != '@' && cp[i] != '\\') || strncmp(&cp[i + 1], "endverbatim", 11) != 0))
          {
            if (cp[i] != '\r')
            {
              vtkWPString_PushChar(text, cp[i]);
            }
            if (cp[i] == '\n')
            {
              j = text->len;
            }
            i++;
          }
          if (cp[i] != '\0')
          {
            i += 12;
          }
        }
      }

      /* search for newline */
      start = 0;
      l = i;
      while (cp[l] == ' ' || cp[l] == '\t' || cp[l] == '\r')
      {
        l++;
      }
      if (cp[l] == '\n')
      {
        i = l + 1;
        start = 1;
      }

      /* append */
      else if (cp[i] != '\0')
      {
        vtkWPString_PushChar(text, cp[i++]);
      }

    } /* while (cp[i] != '\0' && text->len-j < width) */

    if (cp[i] == '\0')
    {
      break;
    }

    vtkWPString_BreakCommentLine(text, &j, indent);
  }

  /* remove any trailing blank lines */
  vtkWPString_Strip(text, "\n");
  vtkWPString_PushChar(text, '\n');

  return text->str;
}

/* -------------------------------------------------------------------- */
/* Create a signature for the python version of a method. */

static void vtkWrapText_PythonTypeSignature(
  struct vtkWPString* result, const char* braces[2], ValueInfo* arg);

static void vtkWrapText_PythonArraySignature(struct vtkWPString* result, const char* classname,
  const char* braces[2], int ndim, const char** dims);

static void vtkWrapText_PythonPODSignature(
  struct vtkWPString* result, const char* classname, const char* braces[2]);

static void vtkWrapText_PythonStdVectorSignature(
  struct vtkWPString* result, const ValueInfo* arg, const char* braces[2]);

static void vtkWrapText_PythonValueSignature(struct vtkWPString* result, ValueInfo* arg);

const char* vtkWrapText_PythonSignature(FunctionInfo* currentFunction)
{
  /* string is intentionally not freed until the program exits */
  static struct vtkWPString staticString = { NULL, 0, 0 };
  struct vtkWPString* result;
  ValueInfo *arg, *ret;
  const char* parens[2] = { "(", ")" };
  const char* braces[2] = { "[", "]" };
  const char** delims;
  int i, n;
  int isConstructor = 0;
  int needsSelf = 1;

  if (currentFunction->Class && strcmp(currentFunction->Class, currentFunction->Name) == 0)
  {
    isConstructor = 1;
  }

  if (isConstructor || currentFunction->IsStatic || !currentFunction->Class)
  {
    needsSelf = 0;
  }

  n = vtkWrap_CountWrappedParameters(currentFunction);

  result = &staticString;
  result->len = 0;

  /* print out the name of the method */
  vtkWPString_Append(result, currentFunction->Name);

  /* print the arg list */
  if (needsSelf)
  {
    vtkWPString_Append(result, "(self");
  }
  else
  {
    vtkWPString_Append(result, "(");
  }

  for (i = 0; i < n; i++)
  {
    arg = currentFunction->Parameters[i];

    if (i != 0 || needsSelf)
    {
      vtkWPString_Append(result, ", ");
    }

    delims = parens;
    if (!vtkWrap_IsConst(arg) && !vtkWrap_IsSetVectorMethod(currentFunction))
    {
      delims = braces;
    }

    if (arg->Name)
    {
      vtkWPString_Append(result, arg->Name);
      /* add underscore to keywords and other specials */
      if (vtkWrapText_IsPythonKeyword(arg->Name) || strcmp(arg->Name, "self") == 0)
      {
        vtkWPString_Append(result, "_");
      }
    }
    else
    {
      /* PEP 484 recommends underscores for position-only arguments */
      char argname[4];
      snprintf(argname, sizeof(argname), "__%c", 'a' + (i % 26));
      vtkWPString_Append(result, argname);
    }

    vtkWPString_Append(result, ":");

    vtkWrapText_PythonTypeSignature(result, delims, arg);
    if (arg->Name && arg->Value)
    {
      vtkWPString_Append(result, "=");
      vtkWrapText_PythonValueSignature(result, arg);
    }
  }

  vtkWPString_Append(result, ")");

  /* print "->" and the return type */
  ret = currentFunction->ReturnValue;
  if (ret && !vtkWrap_IsVoid(ret))
  {
    vtkWPString_Append(result, " -> ");
    if (vtkWrap_IsPODPointer(ret))
    {
      /* can't return POD as tuple, since size is unknown */
      vtkWPString_Append(result, "Pointer");
    }
    else
    {
      vtkWrapText_PythonTypeSignature(result, parens, ret);
    }
  }
  else if (isConstructor)
  {
    vtkWPString_Append(result, " -> ");
    vtkWPString_Append(result, currentFunction->Name);
  }
  else
  {
    vtkWPString_Append(result, " -> None");
  }

  if (currentFunction->Signature)
  {
    vtkWPString_Append(result, "\nC++: ");
    vtkWPString_Append(result, currentFunction->Signature);
  }

  return result->str;
}

static void vtkWrapText_PythonTypeSignature(
  struct vtkWPString* result, const char* braces[2], ValueInfo* arg)
{
  char text[256];
  const char* dimension;
  const char* classname = "";

  if (vtkWrap_IsVoid(arg))
  {
    classname = "Any";
  }
  else if (vtkWrap_IsFunction(arg))
  {
    classname = "Callback";
  }
  else if (vtkWrap_IsZeroCopyPointer(arg))
  {
    classname = "Buffer";
  }
  else if (vtkWrap_IsVoidPointer(arg))
  {
    classname = "Pointer";
  }
  else if (vtkWrap_IsString(arg) || vtkWrap_IsCharPointer(arg))
  {
    classname = "str";
  }
  else if (vtkWrap_IsChar(arg))
  {
    classname = "str";
  }
  else if (vtkWrap_IsBool(arg))
  {
    classname = "bool";
  }
  else if (vtkWrap_IsRealNumber(arg))
  {
    classname = "float";
  }
  else if (vtkWrap_IsInteger(arg))
  {
    classname = "int";
  }
  else
  {
    vtkWrapText_PythonName(arg->Class, text);
    classname = text;
  }

  if ((vtkWrap_IsArray(arg) && arg->CountHint) || vtkWrap_IsPODPointer(arg))
  {
    vtkWrapText_PythonPODSignature(result, classname, braces);
  }
  else if (vtkWrap_IsArray(arg))
  {
    snprintf(text, sizeof(text), "%d", arg->Count);
    dimension = text;
    vtkWrapText_PythonArraySignature(result, classname, braces, 1, &dimension);
  }
  else if (vtkWrap_IsNArray(arg))
  {
    vtkWrapText_PythonArraySignature(
      result, classname, braces, arg->NumberOfDimensions, arg->Dimensions);
  }
  else if (vtkWrap_IsStdVector(arg))
  {
    vtkWrapText_PythonStdVectorSignature(result, arg, braces);
  }
  else if (vtkWrap_IsVTKSmartPointer(arg))
  {
    char* templateArg = vtkWrap_TemplateArg(arg->Class);
    vtkWPString_Append(result, templateArg);
    free(templateArg);
  }
  else
  {
    vtkWPString_Append(result, classname);
  }
}

static void vtkWrapText_PythonArraySignature(struct vtkWPString* result, const char* classname,
  const char* braces[2], int ndim, const char** dims)
{
  int j, n;

  vtkWPString_Append(result, braces[0]);
  n = (int)strtoul(dims[0], 0, 0);
  if (ndim > 1)
  {
    for (j = 0; j < n; j++)
    {
      if (j != 0)
      {
        vtkWPString_Append(result, ", ");
      }
      vtkWrapText_PythonArraySignature(result, classname, braces, ndim - 1, dims + 1);
    }
  }
  else
  {
    for (j = 0; j < n; j++)
    {
      if (j != 0)
      {
        vtkWPString_Append(result, ", ");
      }
      vtkWPString_Append(result, classname);
    }
  }
  vtkWPString_Append(result, braces[1]);
}

static void vtkWrapText_PythonPODSignature(
  struct vtkWPString* result, const char* classname, const char* braces[2])
{
  vtkWPString_Append(result, braces[0]);
  vtkWPString_Append(result, classname);
  vtkWPString_Append(result, ", ...");
  vtkWPString_Append(result, braces[1]);
}

static void vtkWrapText_PythonStdVectorSignature(
  struct vtkWPString* result, const ValueInfo* arg, const char* braces[2])
{
  StringCache cache = { 0, 0, 0, 0 };
  ValueInfo val;
  size_t n;
  const char* classname;
  const char* temp;
  const char** args;
  const char* defaults[2] = { NULL, "" };
  unsigned int basetype;
  /* decompose template to get the first arg */
  vtkParse_DecomposeTemplatedType(arg->Class, &temp, 2, &args, defaults);
  vtkParse_BasicTypeFromString(args[0], &basetype, &classname, &n);
  classname = vtkParse_CacheString(&cache, classname, n);
  vtkParse_FreeTemplateDecomposition(temp, 2, args);
  /* create a ValueInfo that describes the type */
  vtkParse_InitValue(&val);
  val.Class = classname;
  val.Type = basetype;
  /* write out as a list of unknown size */
  vtkWPString_Append(result, braces[0]);
  vtkWrapText_PythonTypeSignature(result, braces, &val);
  vtkWPString_Append(result, ", ...");
  vtkWPString_Append(result, braces[1]);
  /* cleanup */
  vtkParse_FreeStringCache(&cache);
}

static void vtkWrapText_PythonValueSignature(struct vtkWPString* result, ValueInfo* arg)
{
  const char* valstring = "...";
  size_t l;

  if (vtkWrap_IsScalar(arg))
  {
    if (vtkWrap_IsBool(arg) || vtkWrap_IsInteger(arg) || vtkWrap_IsRealNumber(arg))
    {
      if (strcmp(arg->Value, "true") == 0)
      {
        valstring = "True";
      }
      else if (strcmp(arg->Value, "false") == 0)
      {
        valstring = "False";
      }
      else
      {
        const char* tryval = arg->Value;
        if (tryval[0] == '-' || tryval[0] == '+' || tryval[0] == '~')
        {
          tryval++;
        }
        l = vtkParse_SkipNumber(tryval);
        if (tryval[l] == '\0')
        {
          valstring = arg->Value;
        }
      }
    }
  }
  else if (vtkWrap_IsPointer(arg))
  {
    if (vtkWrap_IsCharPointer(arg))
    {
      l = vtkParse_SkipQuotes(arg->Value);
      if (arg->Value[l] == '\0')
      {
        valstring = arg->Value;
      }
    }
  }

  vtkWPString_Append(result, valstring);
}

/* convert C++ identifier to a valid python identifier by mangling */
void vtkWrapText_PythonName(const char* name, char* pname)
{
  size_t j = 0;
  size_t i;
  size_t l;
  char* cp;
  int scoped = 0;

  /* look for first char that is not alphanumeric or underscore */
  l = vtkParse_IdentifierLength(name);

  if (name[l] != '\0')
  {
    /* get the mangled name */
    vtkParse_MangledTypeName(name, pname);

    /* put dots after namespaces */
    i = 0;
    cp = pname;
    if (cp[0] == 'S' && cp[1] >= 'a' && cp[1] <= 'z')
    {
      /* keep std:: namespace abbreviations */
      pname[j++] = *cp++;
      pname[j++] = *cp++;
    }
    while (*cp == 'N')
    {
      scoped++;
      cp++;
      while (*cp >= '0' && *cp <= '9')
      {
        i = i * 10 + (*cp++ - '0');
      }
      i += j;
      while (j < i)
      {
        pname[j++] = *cp++;
      }
      pname[j++] = '.';
    }

    /* remove mangling from first identifier and add an underscore */
    i = 0;
    while (*cp >= '0' && *cp <= '9')
    {
      i = i * 10 + (*cp++ - '0');
    }
    i += j;
    while (j < i)
    {
      pname[j++] = *cp++;
    }
    pname[j++] = '_';
    while (*cp != '\0')
    {
      pname[j++] = *cp++;
    }
    pname[j] = '\0';
  }
  else
  {
    strcpy(pname, name);
  }

  /* remove the "_E" that is added to mangled scoped names */
  if (scoped)
  {
    j = strlen(pname);
    if (j > 2 && pname[j - 2] == '_' && pname[j - 1] == 'E')
    {
      pname[j - 2] = '\0';
    }
  }
}

/* -------------------------------------------------------------------- */
/* Check if an identifier is a Python keyword */
static int stringcomp(const void* a, const void* b)
{
  return strcmp(*((const char**)a), *((const char**)b));
}

int vtkWrapText_IsPythonKeyword(const char* name)
{
  const char* text = name;
  /* The keywords must be lexically sorted in order for bsearch to work */
  const char* specials[] = { "False", "None", "True", "and", "as", "assert", "async", "await",
    "break", "class", "continue", "def", "del", "elif", "else", "except", "finally", "for", "from",
    "global", "if", "import", "in", "is", "lambda", "nonlocal", "not", "or", "pass", "raise",
    "return", "try", "while", "with", "yield" };

  if (bsearch(&text, specials, sizeof(specials) / sizeof(char*), sizeof(char*), stringcomp))
  {
    return 1;
  }
  return 0;
}

int vtkWrapText_IsJavaScriptKeyword(const char* name)
{
  const char* text = name;
  /* The keywords must be lexically sorted in order for bsearch to work */
  const char* specials[] = { "break", "case", "catch", "class", "const", "continue", "debugger",
    "default", "delete", "do", "else", "export", "extends", "false", "finally", "for", "function",
    "if", "import", "in", "instanceof", "new", "null", "return", "super", "switch", "this", "throw",
    "true", "try", "typeof", "var", "void", "while", "with", "let ", "static", "yield", "await" };

  if (bsearch(&text, specials, sizeof(specials) / sizeof(char*), sizeof(char*), stringcomp))
  {
    return 1;
  }
  return 0;
}
