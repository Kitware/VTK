/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParseExtras.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright (c) 2011 David Gobbi.

  Contributed to the VisualizationToolkit by the author in May 2011
  under the terms of the Visualization Toolkit 2008 copyright.
-------------------------------------------------------------------------*/

#include "vtkParseExtras.h"
#include "vtkParseString.h"
#include "vtkType.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

/* skip over an identifier */
static size_t vtkparse_id_len(const char *text)
{
  size_t i = 0;
  char c = text[0];

  if ((c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z') ||
       c == '_')
    {
    do
      {
      c = text[++i];
      }
    while ((c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '_');
    }

  return i;
}

/* skip over numbers, int or float, including suffixes */
static size_t vtkparse_number_len(const char *text)
{
  size_t i = 0;
  char c = text[0];

  if (c == '.')
    {
    c = text[1];
    }

  if (c >= '0' && c <= '9')
    {
    do
      {
      do
        {
        c = text[++i];
        }
      while ((c >= '0' && c <= '9') ||
             (c >= 'a' && c <= 'z') ||
             (c >= 'A' && c <= 'Z') ||
             c == '_' || c == '.');
      }
    while ((c == '-' || c == '+') &&
           (text[i-1] == 'e' || text[i-1] == 'E'));
    }

  return i;
}

/* skip over string and char literals. */
static size_t vtkparse_quote_len(const char *text)
{
  size_t i = 0;
  const char qc = text[0];
  char c = text[0];

  if (c == '\'' || c == '\"')
    {
    do
      {
      do
        {
        c = text[++i];
        }
      while (c != qc && c != '\n' && c != '\0');
      }
    while (c == qc && text[i-1] == '\\');

    if (c == qc)
      {
      ++i;
      }
    }

  return i;
}

/* skip over an expression in brackets */
static size_t vtkparse_bracket_len(const char *text)
{
  size_t i = 0;
  size_t j = 1;
  char bc = text[0];
  char tc = 0;
  char semi = ';';
  char c;

  if (bc == '(') { tc = ')'; }
  else if (bc == '[') { tc = ']'; }
  else if (bc == '{') { tc = '}'; semi = '\0'; }
  else if (bc == '<') { tc = '>'; }
  else { return 0; }

  do
    {
    i += j;
    j = 1;
    c = text[i];
    if (c == '\'' || c == '\"')
      {
      j = vtkparse_quote_len(&text[i]);
      }
    else if (c == bc || c == '(' || c == '[' || c == '{')
      {
      j = vtkparse_bracket_len(&text[i]);
      }
    }
  while (c != tc && c != ')' && c != ']' && c != '}' &&
         c != '\0' && c != '\n' && c != semi && j != 0);

  if (c == tc)
    {
    i++;
    }

  return i;
}

/* skip over a name that is neither scoped or templated, return the
 * total number of characters in the name */
size_t vtkParse_IdentifierLength(const char *text)
{
  return vtkparse_id_len(text);
}

/* skip over a name that might be templated, return the
 * total number of characters in the name */
size_t vtkParse_UnscopedNameLength(const char *text)
{
  size_t i = 0;

  i += vtkparse_id_len(text);
  if (text[i] == '<')
    {
    i += vtkparse_bracket_len(&text[i]);
    if (text[i-1] != '>')
      {
      fprintf(stderr, "Bad template args %*.*s\n", (int)i, (int)i, text);
      assert(text[i-1] == '>');
      return 0;
      }
    }

  return i;
}

/* skip over a name that might be scoped or templated, return the
 * total number of characters in the name */
size_t vtkParse_NameLength(const char *text)
{
  size_t i = 0;
  do
    {
    if (text[i] == ':' && text[i+1] == ':') { i += 2; }
    i += vtkParse_UnscopedNameLength(&text[i]);
    }
  while (text[i] == ':' && text[i+1] == ':');
  return i;
}

/* Search and replace, return the initial string if no replacements
 * occurred, otherwise return a new string. */
static const char *vtkparse_string_replace(
  StringCache *cache, const char *str1,
  int n, const char *name[], const char *val[])
{
  const char *cp = str1;
  char result_store[1024];
  size_t resultMaxLen = 1024;
  char *result, *tmp;
  int k;
  size_t i, j, l, m;
  size_t lastPos, nameBegin, nameEnd;
  int replaced = 0;
  int any_replaced = 0;

  result = result_store;

  if (n == 0)
    {
    return str1;
    }

  i = 0;
  j = 0;
  result[j] = '\0';

  while (cp[i] != '\0')
    {
    lastPos = i;

    /* skip all chars that aren't part of a name */
    while ((cp[i] < 'a' || cp[i] > 'z') &&
           (cp[i] < 'A' || cp[i] > 'Z') &&
           cp[i] != '_' && cp[i] != '\0')
      {
      if (cp[i] == '\'' || cp[i] == '\"')
        {
        i += vtkparse_quote_len(&cp[i]);
        }
      else if (cp[i] >= '0' && cp[i] <= '9')
        {
        i += vtkparse_number_len(&cp[i]);
        }
      else
        {
        i++;
        }
      }
    nameBegin = i;

    /* skip all chars that are part of a name */
    i += vtkparse_id_len(&cp[i]);
    nameEnd = i;

    /* search through the list of names to replace */
    replaced = 0;
    m = nameEnd - nameBegin;
    for (k = 0; k < n; k++)
      {
      l = strlen(name[k]);
      if (l > 0 && l == m && strncmp(&cp[nameBegin], name[k], l) == 0)
        {
        m = strlen(val[k]);
        replaced = 1;
        any_replaced = 1;
        break;
        }
      }

    /* expand the storage space if needed */
    if (j + m + (nameBegin - lastPos) + 1 >= resultMaxLen)
      {
      resultMaxLen *= 2;
      tmp = (char *)malloc(resultMaxLen);
      strcpy(tmp, result);
      if (result != result_store)
         {
         free(result);
         }
       result = tmp;
       }

    /* copy the old bits */
    if (nameBegin > lastPos)
      {
      strncpy(&result[j], &cp[lastPos], nameBegin - lastPos);
      j += (nameBegin - lastPos);
      }

    /* do the replacement */
    if (replaced)
      {
      strncpy(&result[j], val[k], m);
      j += m;
      /* guard against creating double ">>" */
      if (val[k][m-1] == '>' && cp[nameEnd] == '>')
        {
        result[j++] = ' ';
        }
      }
    else if (nameEnd > nameBegin)
      {
      strncpy(&result[j], &cp[nameBegin], nameEnd - nameBegin);
      j += (nameEnd - nameBegin);
      }

    result[j] = '\0';
    }

  if (cache)
    {
    if (any_replaced)
      {
      /* use the efficient CacheString method */
      cp = vtkParse_CacheString(cache, result, j);
      if (result != result_store)
        {
        free(result);
        }
      }
    }
  else
    {
    if (any_replaced)
      {
      /* return a string that was allocated with malloc */
      if (result == result_store)
        {
        tmp = (char *)malloc(strlen(result) + 1);
        strcpy(tmp, result);
        result = tmp;
        }
      cp = result;
      }
    }

  return cp;
}

/* Wherever one of the specified names exists inside a Value or inside
 * a Dimension size, replace it with the corresponding val string. */
void vtkParse_ExpandValues(
  ValueInfo *valinfo, StringCache *cache,
  int n, const char *name[], const char *val[])
{
  int j, m, dim, count;
  const char *cp;

  if (valinfo->Value)
    {
    valinfo->Value = vtkparse_string_replace(
      cache, valinfo->Value, n, name, val);
    }

  m = valinfo->NumberOfDimensions;
  if (m)
    {
    count = 1;
    for (j = 0; j < m; j++)
      {
      cp = valinfo->Dimensions[j];
      if (cp)
        {
        cp = vtkparse_string_replace(cache, cp, n, name, val);
        valinfo->Dimensions[j] = cp;

        /* check whether dimension has become an integer literal */
        if (cp[0] == '0' && (cp[1] == 'x' || cp[1] == 'X')) { cp += 2; }
        while (*cp >= '0' && *cp <= '9') { cp++; }
        while (*cp == 'u' || *cp == 'l' || *cp == 'U' || *cp == 'L') { cp++; }
        dim = 0;
        if (*cp == '\0')
          {
          dim = (int)strtol(valinfo->Dimensions[j], NULL, 0);
          }
        count *= dim;
        }
      }

    /* update count if all values are integer literals */
    if (count)
      {
      valinfo->Count = count;
      }
    }
}

/* Expand a typedef within a type declaration. */
void vtkParse_ExpandTypedef(
  ValueInfo *valinfo, ValueInfo *typedefinfo)
{
  const char *classname;
  unsigned int baseType;
  unsigned int pointers;
  unsigned int refbit;
  unsigned int qualifiers;
  unsigned int tmp1, tmp2;
  int i;

  classname = typedefinfo->Class;
  baseType = (typedefinfo->Type & VTK_PARSE_BASE_TYPE);
  pointers = (typedefinfo->Type & VTK_PARSE_POINTER_MASK);
  refbit = (valinfo->Type & VTK_PARSE_REF);
  qualifiers = (typedefinfo->Type & VTK_PARSE_CONST);

  /* handle const */
  if ((valinfo->Type & VTK_PARSE_CONST) != 0)
    {
    if ((pointers & VTK_PARSE_POINTER_LOWMASK) != 0)
      {
      if ((pointers & VTK_PARSE_POINTER_LOWMASK) != VTK_PARSE_ARRAY)
        {
        /* const turns into const pointer */
        pointers = (pointers & ~VTK_PARSE_POINTER_LOWMASK);
        pointers = (pointers | VTK_PARSE_CONST_POINTER);
        }
      }
    else
      {
      /* const remains as const value */
      qualifiers = (qualifiers | VTK_PARSE_CONST);
      }
    }

  /* make a reversed copy of the pointer bitfield */
  tmp1 = (valinfo->Type & VTK_PARSE_POINTER_MASK);
  tmp2 = 0;
  while (tmp1)
    {
    tmp2 = ((tmp2 << 2) | (tmp1 & VTK_PARSE_POINTER_LOWMASK));
    tmp1 = ((tmp1 >> 2) & VTK_PARSE_POINTER_MASK);
    }

  /* turn pointers into zero-element arrays where necessary */
  if ((pointers & VTK_PARSE_POINTER_LOWMASK) == VTK_PARSE_ARRAY)
    {
    tmp2 = ((tmp2 >> 2) & VTK_PARSE_POINTER_MASK);
    while (tmp2)
      {
      vtkParse_AddStringToArray(
        &valinfo->Dimensions, &valinfo->NumberOfDimensions, "");
      tmp2 = ((tmp2 >> 2) & VTK_PARSE_POINTER_MASK);
      }
    }
  else
    {
    /* combine the pointers */
    while (tmp2)
      {
      pointers = ((pointers << 2) | (tmp2 & VTK_PARSE_POINTER_LOWMASK));
      tmp2 = ((tmp2 >> 2) & VTK_PARSE_POINTER_MASK);
      }
    }

  /* combine the arrays */
  for (i = 0; i < typedefinfo->NumberOfDimensions; i++)
    {
    vtkParse_AddStringToArray(
      &valinfo->Dimensions, &valinfo->NumberOfDimensions,
      typedefinfo->Dimensions[i]);
    }
  if (valinfo->NumberOfDimensions > 1)
    {
    pointers = ((pointers & ~VTK_PARSE_POINTER_LOWMASK) | VTK_PARSE_ARRAY);
    }

  /* put everything together */
  valinfo->Type = (baseType | pointers | refbit | qualifiers);
  valinfo->Class = classname;
  valinfo->Function = typedefinfo->Function;
  valinfo->Count *= typedefinfo->Count;
}

/* Expand any unrecognized types within a variable, parameter, or typedef
 * that match any of the supplied typedefs. The expansion is done in-place. */
void vtkParse_ExpandTypedefs(
  ValueInfo *val, StringCache *cache,
  int n, const char *names[], const char *values[],
  ValueInfo *typedefinfo[])
{
  int i;

  if (((val->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_OBJECT ||
       (val->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_UNKNOWN) &&
      val->Class != 0)
   {
   for (i = 0; i < n; i++)
     {
     if (typedefinfo[i] && strcmp(val->Class, typedefinfo[i]->Name) == 0)
       {
       vtkParse_ExpandTypedef(val, typedefinfo[i]);
       break;
       }
     }
   if (i == n)
     {
     /* in case type appears as a template arg of another type */
     val->Class = vtkparse_string_replace(
       cache, val->Class, n, names, values);
     }
   }
}

/* Helper struct for VTK-specific types */
struct vtk_type_struct
{
  size_t len;
  const char *name;
  unsigned int type;
};

/* Simple utility for mapping VTK types to VTK_PARSE types */
unsigned int vtkParse_MapType(int vtktype)
{
  static unsigned int typemap[] =
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

  if (vtktype > 0 && vtktype <= VTK_UNICODE_STRING)
    {
    return typemap[vtktype];
    }
  return 0;
}

/* Get a type from a type name, and return the number of characters used.
 * If the "classname" argument is not NULL, then it is used to return
 * the short name for the type, e.g. "long int" becomes "long", while
 * typedef names and class names are returned unchanged.  If "const"
 * appears in the type name, then the const bit flag is set for the
 * type, but "const" will not appear in the returned classname. */
size_t vtkParse_BasicTypeFromString(
  const char *text, unsigned int *type_ptr,
  const char **classname_ptr, size_t *len_ptr)
{
  /* The various typedefs and types specific to VTK */
  static struct vtk_type_struct vtktypes[] = {
    { 9,  "vtkIdType", VTK_ID_TYPE },
    { 12, "vtkStdString", VTK_STRING },
    { 16, "vtkUnicodeString", VTK_UNICODE_STRING },
    { 11, "vtkTypeInt8", VTK_TYPE_INT8 },
    { 12, "vtkTypeUInt8", VTK_TYPE_UINT8 },
    { 12, "vtkTypeInt16", VTK_TYPE_INT16 },
    { 13, "vtkTypeUInt16", VTK_TYPE_UINT16 },
    { 12, "vtkTypeInt32", VTK_TYPE_INT32 },
    { 13, "vtkTypeUInt32", VTK_TYPE_UINT32 },
    { 12, "vtkTypeInt64", VTK_TYPE_INT64 },
    { 13, "vtkTypeUInt64", VTK_TYPE_UINT64 },
    { 14, "vtkTypeFloat32", VTK_TYPE_FLOAT32 },
    { 14, "vtkTypeFloat64", VTK_TYPE_FLOAT64 },
    { 0, 0, 0 } };

  /* Other typedefs and types */
  static struct vtk_type_struct stdtypes[] = {
    { 6,  "size_t", VTK_PARSE_SIZE_T },
    { 7,  "ssize_t", VTK_PARSE_SSIZE_T },
    { 7,  "ostream", VTK_PARSE_OSTREAM },
    { 7,  "istream", VTK_PARSE_ISTREAM },
    { 8,  "string", VTK_PARSE_STRING },
    { 0, 0, 0 } };

  const char *cp = text;
  const char *tmpcp;
  size_t k, n, m;
  int i;
  unsigned int const_bits = 0;
  unsigned int static_bits = 0;
  unsigned int unsigned_bits = 0;
  unsigned int base_bits = 0;
  const char *classname = NULL;
  size_t len = 0;

  while (*cp == ' ' || *cp == '\t') { cp++; }

  while ((*cp >= 'a' && *cp <= 'z') ||
         (*cp >= 'A' && *cp <= 'Z') ||
         (*cp == '_') || (cp[0] == ':' && cp[1] == ':'))
    {
    /* skip all chars that are part of a name */
    n = vtkParse_NameLength(cp);

    if ((n == 6 && strncmp("static", cp, n) == 0) ||
        (n == 4 && strncmp("auto", cp, n) == 0) ||
        (n == 8 && strncmp("register", cp, n) == 0) ||
        (n == 8 && strncmp("volatile", cp, n) == 0))
      {
      if (strncmp("static", cp, n) == 0)
        {
        static_bits = VTK_PARSE_STATIC;
        }
      }
    else if (n == 5 && strncmp(cp, "const", n) == 0)
      {
      const_bits |= VTK_PARSE_CONST;
      }
    else if (n == 8 && strncmp(cp, "unsigned", n) == 0)
      {
      unsigned_bits |= VTK_PARSE_UNSIGNED;
      if (base_bits == 0)
        {
        classname = "int";
        base_bits = VTK_PARSE_INT;
        }
      }
    else if (n == 6 && strncmp(cp, "signed", n) == 0)
      {
      if (base_bits == VTK_PARSE_CHAR)
        {
        classname = "signed char";
        base_bits = VTK_PARSE_SIGNED_CHAR;
        }
      else
        {
        classname = "int";
        base_bits = VTK_PARSE_INT;
        }
      }
    else if (n == 3 && strncmp(cp, "int", n) == 0)
      {
      if (base_bits == 0)
        {
        classname = "int";
        base_bits = VTK_PARSE_INT;
        }
      }
    else if (n == 4 && strncmp(cp, "long", n) == 0)
      {
      if (base_bits == VTK_PARSE_DOUBLE)
        {
        classname = "long double";
        base_bits = VTK_PARSE_LONG_DOUBLE;
        }
      else if (base_bits == VTK_PARSE_LONG)
        {
        classname = "long long";
        base_bits = VTK_PARSE_LONG_LONG;
        }
      else
        {
        classname = "long";
        base_bits = VTK_PARSE_LONG;
        }
      }
    else if (n == 5 && strncmp(cp, "short", n) == 0)
      {
      classname = "short";
      base_bits = VTK_PARSE_SHORT;
      }
    else if (n == 4 && strncmp(cp, "char", n) == 0)
      {
      if (base_bits == VTK_PARSE_INT && unsigned_bits != VTK_PARSE_UNSIGNED)
        {
        classname = "signed char";
        base_bits = VTK_PARSE_SIGNED_CHAR;
        }
      else
        {
        classname = "char";
        base_bits = VTK_PARSE_CHAR;
        }
      }
    else if (n == 5 && strncmp(cp, "float", n) == 0)
      {
      classname = "float";
      base_bits = VTK_PARSE_FLOAT;
      }
    else if (n == 6 && strncmp(cp, "double", n) == 0)
      {
      if (base_bits == VTK_PARSE_LONG)
        {
        classname = "long double";
        base_bits = VTK_PARSE_LONG_DOUBLE;
        }
      else
        {
        classname = "double";
        base_bits = VTK_PARSE_DOUBLE;
        }
      }
    else if (n == 4 && strncmp(cp, "bool", n) == 0)
      {
      classname = "bool";
      base_bits = VTK_PARSE_BOOL;
      }
    else if (n == 4 && strncmp(cp, "void", n) == 0)
      {
      classname = "void";
      base_bits = VTK_PARSE_VOID;
      }
    else if (n == 7 && strncmp(cp, "__int64", n) == 0)
      {
      classname = "__int64";
      base_bits = VTK_PARSE___INT64;
      }
    else
      {
      /* if type already found, break */
      if (base_bits != 0)
        {
        break;
        }

      /* check vtk typedefs */
      if (strncmp(cp, "vtk", 3) == 0)
        {
        for (i = 0; vtktypes[i].len != 0; i++)
          {
          if (n == vtktypes[i].len && strncmp(cp, vtktypes[i].name, n) == 0)
            {
            classname = vtktypes[i].name;
            base_bits = vtkParse_MapType((int)vtktypes[i].type);
            }
          }
        }

      /* check standard typedefs */
      if (base_bits == 0)
        {
        m = 0;
        if (strncmp(cp, "::", 2) == 0) { m = 2; }
        else if (strncmp(cp, "std::", 5) == 0) { m = 5; }
        else if (strncmp(cp, "vtkstd::", 8) == 0) { m = 8; }

        /* advance past the namespace */
        tmpcp = cp + m;

        for (i = 0; stdtypes[i].len != 0; i++)
          {
          if (n == stdtypes[i].len && strncmp(tmpcp, stdtypes[i].name, n) == 0)
            {
            classname = stdtypes[i].name;
            base_bits = stdtypes[i].type;
            }
          }

        /* include the namespace if present */
        if (base_bits != 0 && m > 0)
          {
          classname = cp;
          len = n;
          }
        }

      /* anything else is assumed to be a class, enum, or who knows */
      if (base_bits == 0)
        {
        base_bits = VTK_PARSE_UNKNOWN;
        classname = cp;
        len = n;

        /* VTK classes all start with vtk */
        if (strncmp(classname, "vtk", 3) == 0)
          {
          base_bits = VTK_PARSE_OBJECT;
          /* make sure the "vtk" isn't just part of the namespace */
          for (k = 0; k < n; k++)
            {
            if (cp[k] == ':')
              {
              base_bits = VTK_PARSE_UNKNOWN;
              break;
              }
            }
          }
        /* Qt objects and enums */
        else if (classname[0] == 'Q' &&
                 ((classname[1] >= 'A' && classname[2] <= 'Z') ||
                  strncmp(classname, "Qt::", 4) == 0))
          {
          base_bits = VTK_PARSE_QOBJECT;
          }
        }
      }

    cp += n;
    while (*cp == ' ' || *cp == '\t') { cp++; }
    }

  if ((unsigned_bits & VTK_PARSE_UNSIGNED) != 0)
    {
    switch (base_bits)
      {
      case VTK_PARSE_CHAR:
        classname = "unsigned char";
        break;
      case VTK_PARSE_SHORT:
        classname = "unsigned short";
        break;
      case VTK_PARSE_INT:
        classname = "unsigned int";
        break;
      case VTK_PARSE_LONG:
        classname = "unsigned long";
        break;
      case VTK_PARSE_LONG_LONG:
        classname = "unsigned long long";
        break;
      case VTK_PARSE___INT64:
        classname = "unsigned __int64";
        break;
      }
    }

  *type_ptr = (static_bits | const_bits | unsigned_bits | base_bits);

  if (classname_ptr)
    {
    *classname_ptr = classname;
    if (len == 0)
      {
      len = strlen(classname);
      }
    *len_ptr = len;
    }

  return (size_t)(cp - text);
}

/* Parse a type description in "text" and generate a typedef named "name" */
size_t vtkParse_ValueInfoFromString(
  ValueInfo *data, StringCache *cache, const char *text)
{
  const char *cp = text;
  size_t n;
  int m, count;
  unsigned int base_bits = 0;
  unsigned int pointer_bits = 0;
  unsigned int ref_bits = 0;
  const char *classname = NULL;

  /* get the basic type with qualifiers */
  cp += vtkParse_BasicTypeFromString(cp, &base_bits, &classname, &n);

  data->Class = vtkParse_CacheString(cache, classname, n);

  if ((base_bits & VTK_PARSE_STATIC) != 0)
    {
    data->IsStatic = 1;
    }

  /* look for pointers (and const pointers) */
  while (*cp == '*')
    {
    cp++;
    pointer_bits = (pointer_bits << 2);
    while (*cp == ' ' || *cp == '\t') { cp++; }
    if (strncmp(cp, "const", 5) == 0 &&
        (cp[5] < 'a' || cp[5] > 'z') &&
        (cp[5] < 'A' || cp[5] > 'Z') &&
        (cp[5] < '0' || cp[5] > '9') &&
        cp[5] != '_')
      {
      cp += 5;
      while (*cp == ' ' || *cp == '\t') { cp++; }
      pointer_bits = (pointer_bits | VTK_PARSE_CONST_POINTER);
      }
    else
      {
      pointer_bits = (pointer_bits | VTK_PARSE_POINTER);
      }
    pointer_bits = (pointer_bits & VTK_PARSE_POINTER_MASK);
    }

  /* look for ref */
  if (*cp == '&')
    {
    cp++;
    while (*cp == ' ' || *cp == '\t') { cp++; }
    ref_bits = VTK_PARSE_REF;
    }

  /* look for the variable name */
  if ((*cp >= 'a' && *cp <= 'z') ||
      (*cp >= 'A' && *cp <= 'Z') ||
      (*cp == '_'))
    {
    /* skip all chars that are part of a name */
    n = vtkparse_id_len(cp);
    data->Name = vtkParse_CacheString(cache, cp, n);
    cp += n;
    while (*cp == ' ' || *cp == '\t') { cp++; }
    }

  /* look for array brackets */
  if (*cp == '[')
    {
    count = 1;

    while (*cp == '[')
      {
      n = vtkparse_bracket_len(cp);
      if (n > 1)
        {
        cp++;
        n -= 2;
        }
      while (*cp == ' ' || *cp == '\t') { cp++; n--; }
      while (n > 0 && (cp[n-1] == ' ' || cp[n-1] == '\t')) { n--; }
      vtkParse_AddStringToArray(
        &data->Dimensions,
        &data->NumberOfDimensions,
        vtkParse_CacheString(cache, cp, n));
      m = 0;
      if (*cp >= '0' && *cp <= '9' && vtkparse_number_len(cp) == n)
        {
        m = (int)strtol(cp, NULL, 0);
        }
      count *= m;

      cp += n;
      while (*cp == ' ' || *cp == '\t') { cp++; }
      if (*cp == ']') { cp++; }
      while (*cp == ' ' || *cp == '\t') { cp++; }
      }
    }

  /* add pointer indirection to correspond to first array dimension */
  if (data->NumberOfDimensions > 1)
    {
    pointer_bits = ((pointer_bits << 2) | VTK_PARSE_ARRAY);
    }
  else if (data->NumberOfDimensions == 1)
    {
    pointer_bits = ((pointer_bits << 2) | VTK_PARSE_POINTER);
    }
  pointer_bits = (pointer_bits & VTK_PARSE_POINTER_MASK);

  /* (Add code here to look for "=" followed by a value ) */

  data->Type = (pointer_bits | ref_bits | base_bits);

  return (cp - text);
}

/* Generate a C++ declaration string from a ValueInfo struct */
const char *vtkParse_ValueInfoToString(
  ValueInfo *data, int *needs_free)
{
  unsigned int pointer_bits = (data->Type & VTK_PARSE_POINTER_MASK);
  unsigned int ref_bits = (data->Type & VTK_PARSE_REF);
  unsigned int qualifier_bits = (data->Type & VTK_PARSE_CONST);
  unsigned int reverse_bits = 0;
  unsigned int pointer_type = 0;
  const char *classname = data->Class;
  const char *name = data->Name;
  char *text = NULL;
  size_t i = 0;
  size_t l;
  int j = 0;

  if (pointer_bits == 0 && ref_bits == 0 && qualifier_bits == 0 &&
      name == NULL)
    {
    if (needs_free)
      {
      *needs_free = 0;
      }
    return classname;
    }

  /* compute the length of string to allocate */
  l = 6; /* for const */
  l += 4*7; /* for pointers */
  l += 1; /* for ref */
  l += strlen(classname) + 1; /* for type */
  for (j = 0; j < data->NumberOfDimensions; j++)
    {
    l += 2 + strlen(data->Dimensions[j]);
    }
  l++; /* for NULL */
  l += 4; /* for safety */

  text = (char *)malloc(l);

  if ((qualifier_bits & VTK_PARSE_CONST) != 0)
    {
    strcpy(&text[i], "const ");
    i += 6;
    }

  strcpy(&text[i], classname);
  i += strlen(classname);
  text[i++] = ' ';

  while (pointer_bits != 0)
    {
    reverse_bits <<= 2;
    reverse_bits |= (pointer_bits & VTK_PARSE_POINTER_LOWMASK);
    pointer_bits = ((pointer_bits >> 2) & VTK_PARSE_POINTER_MASK);
    }

  while (reverse_bits != 0)
    {
    pointer_type = (reverse_bits & VTK_PARSE_POINTER_LOWMASK);
    if (pointer_type == VTK_PARSE_ARRAY ||
        (reverse_bits == VTK_PARSE_POINTER &&
         data->NumberOfDimensions > 0))
      {
      break;
      }
    else if (pointer_type == VTK_PARSE_POINTER)
      {
      text[i++] = '*';
      }
    else if (pointer_type == VTK_PARSE_CONST_POINTER)
      {
      strcpy(&text[i], "*const ");
      i += 7;
      }

    reverse_bits = ((reverse_bits >> 2) & VTK_PARSE_POINTER_MASK);
    }

  if (ref_bits)
    {
    text[i++] = '&';
    }

  if (name)
    {
    strcpy(&text[i], name);
    i += strlen(name);
    }

  for (j = 0; j < data->NumberOfDimensions; j++)
    {
    text[i++] = '[';
    if (data->Dimensions[j])
      {
      strcpy(&text[i], data->Dimensions[j]);
      i += strlen(data->Dimensions[j]);
      }
    text[i++] = ']';
    }

  text[i] = '\0';

  /* make sure enough space was allocated */
  assert(i < l);

  if (needs_free)
    {
    *needs_free = 1;
    }

  return text;
}

/* Search and replace, return the initial string if no replacements
 * occurred, otherwise return a new string allocated with malloc. */
const char *vtkParse_StringReplace(
  const char *str1, int n, const char *name[], const char *val[])
{
  return vtkparse_string_replace(NULL, str1, n, name, val);
}

/* substitute generic types and values with actual types and values */
static void func_substitution(
  FunctionInfo *data, StringCache *cache,
  int m, const char *arg_names[],
  const char *arg_values[], ValueInfo *arg_types[]);

static void value_substitution(
  ValueInfo *data, StringCache *cache,
  int m, const char *arg_names[],
  const char *arg_values[], ValueInfo *arg_types[])
{
  vtkParse_ExpandTypedefs(data, cache, m, arg_names, arg_values, arg_types);
  vtkParse_ExpandValues(data, cache, m, arg_names, arg_values);

  if (data->Function)
    {
    func_substitution(
      data->Function, cache, m, arg_names, arg_values, arg_types);
    }
}

static void func_substitution(
  FunctionInfo *data, StringCache *cache,
  int m, const char *arg_names[],
  const char *arg_values[], ValueInfo *arg_types[])
{
  int i, n;

  n = data->NumberOfParameters;
  for (i = 0; i < n; i++)
    {
    value_substitution(
      data->Parameters[i], cache, m, arg_names, arg_values, arg_types);
    }

  if (data->ReturnValue)
    {
    value_substitution(
      data->ReturnValue, cache, m, arg_names, arg_values, arg_types);
    }

  if (data->Signature)
    {
    data->Signature =
      vtkparse_string_replace(
        cache, data->Signature, m, arg_names, arg_values);
    }

  /* legacy information for old wrappers */
#ifndef VTK_PARSE_LEGACY_REMOVE
  n = data->NumberOfArguments;
  for (i = 0; i < n; i++)
    {
    data->ArgTypes[i] = data->Parameters[i]->Type;
    data->ArgClasses[i] = data->Parameters[i]->Class;
    if (data->Parameters[i]->NumberOfDimensions == 1 &&
        data->Parameters[i]->Count > 0)
      {
      data->ArgCounts[i] = data->Parameters[i]->Count;
      }
    }

  if (data->ReturnValue)
    {
    data->ReturnType = data->ReturnValue->Type;
    data->ReturnClass = data->ReturnValue->Class;
    if (data->ReturnValue->NumberOfDimensions == 1 &&
        data->ReturnValue->Count > 0)
      {
      data->HintSize = data->ReturnValue->Count;
      data->HaveHint = 1;
      }
    }
#endif /* VTK_PARSE_LEGACY_REMOVE */
}

static void class_substitution(
  ClassInfo *data, StringCache *cache,
  int m, const char *arg_names[],
  const char *arg_values[], ValueInfo *arg_types[])
{
  int i, n;

  /* superclasses may be templated */
  n = data->NumberOfSuperClasses;
  for (i = 0; i < n; i++)
    {
    data->SuperClasses[i] = vtkparse_string_replace(
      cache, data->SuperClasses[i], m, arg_names, arg_values);
    }

  n = data->NumberOfClasses;
  for (i = 0; i < n; i++)
    {
    class_substitution(
      data->Classes[i], cache, m, arg_names, arg_values, arg_types);
    }

  n = data->NumberOfFunctions;
  for (i = 0; i < n; i++)
    {
    func_substitution(
      data->Functions[i], cache, m, arg_names, arg_values, arg_types);
    }

  n = data->NumberOfConstants;
  for (i = 0; i < n; i++)
    {
    value_substitution(
      data->Constants[i], cache, m, arg_names, arg_values, arg_types);
    }

  n = data->NumberOfVariables;
  for (i = 0; i < n; i++)
    {
    value_substitution(
      data->Variables[i], cache, m, arg_names, arg_values, arg_types);
    }

  n = data->NumberOfTypedefs;
  for (i = 0; i < n; i++)
    {
    value_substitution(
      data->Typedefs[i], cache, m, arg_names, arg_values, arg_types);
    }
}

/* Extract template args from a comma-separated list enclosed
 * in angle brackets.  Returns zero if no angle brackets found. */
size_t vtkParse_DecomposeTemplatedType(
  const char *text, const char **classname,
  int nargs, const char ***argp, const char *defaults[])
{
  size_t i, j, k, n;
  const char *arg;
  char *new_text;
  const char **template_args = NULL;
  int template_arg_count = 0;

  n = vtkParse_NameLength(text);

  /* is the class templated? */
  for (i = 0; i < n; i++)
    {
    if (text[i] == '<')
      {
      break;
      }
    }

  if (classname)
    {
    new_text = (char *)malloc(i + 1);
    strncpy(new_text, text, i);
    new_text[i] = '\0';
    *classname = new_text;
    }

  if (text[i] == '<')
    {
    i++;
    /* extract the template arguments */
    for (;;)
      {
      while (text[i] == ' ' || text[i] == '\t') { i++; }
      j = i;
      while (text[j] != ',' && text[j] != '>' &&
             text[j] != '\n' && text[j] != '\0')
        {
        if (text[j] == '<' || text[j] == '(' ||
            text[j] == '[' || text[j] == '{')
          {
          j += vtkparse_bracket_len(&text[j]);
          }
        else if (text[j] == '\'' || text[j] == '\"')
          {
          j += vtkparse_quote_len(&text[j]);
          }
        else
          {
          j++;
          }
        }

      k = j;
      while (text[k-1] == ' ' || text[k-1] == '\t') { --k; }

      new_text = (char *)malloc(k-i + 1);
      strncpy(new_text, &text[i], k-i);
      new_text[k-i] = '\0';
      vtkParse_AddStringToArray(&template_args, &template_arg_count,
                                new_text);

      assert(template_arg_count <= nargs);

      i = j + 1;

      if (text[j] != ',')
        {
        break;
        }
      }
    }

  while (template_arg_count < nargs)
    {
    assert(defaults != NULL);
    arg = defaults[template_arg_count];
    assert(arg != NULL);
    new_text = (char *)malloc(strlen(arg + 1));
    strcpy(new_text, arg);
    vtkParse_AddStringToArray(&template_args, &template_arg_count, new_text);
    }

  *argp = template_args;

  return i;
}

/* Free the list of strings returned by ExtractTemplateArgs.  */
void vtkParse_FreeTemplateDecomposition(
  const char *name, int n, const char **args)
{
  int i;

  if (name)
    {
    free((char *)name);
    }

  if (n > 0)
    {
    for (i = 0; i < n; i++)
      {
      free((char *)args[i]);
      }

    free((char **)args);
    }
}

/* Instantiate a class template by substituting the provided arguments. */
void vtkParse_InstantiateClassTemplate(
  ClassInfo *data, StringCache *cache, int n, const char *args[])
{
  TemplateInfo *t = data->Template;
  const char **new_args = NULL;
  const char **arg_names = NULL;
  ValueInfo **arg_types = NULL;
  int i, m;
  char *new_name;
  size_t k;

  if (t == NULL)
    {
    fprintf(stderr, "vtkParse_InstantiateClassTemplate: "
            "this class is not templated.\n");
    return;
    }

  m = t->NumberOfParameters;
  if (n > m)
    {
    fprintf(stderr, "vtkParse_InstantiateClassTemplate: "
            "too many template args.\n");
    return;
    }

  for (i = n; i < m; i++)
    {
    if (t->Parameters[i]->Value == NULL ||
        t->Parameters[i]->Value[0] == '\0')
      {
      fprintf(stderr, "vtkParse_InstantiateClassTemplate: "
              "too few template args.\n");
      return;
      }
    }

  new_args = (const char **)malloc(m*sizeof(char **));
  for (i = 0; i < n; i++)
    {
    new_args[i] = args[i];
    }
  for (i = n; i < m; i++)
    {
    new_args[i] = t->Parameters[i]->Value;
    }
  args = new_args;

  arg_names = (const char **)malloc(m*sizeof(char **));
  arg_types = (ValueInfo **)malloc(m*sizeof(ValueInfo *));
  for (i = 0; i < m; i++)
    {
    arg_names[i] = t->Parameters[i]->Name;
    arg_types[i] = NULL;
    if (t->Parameters[i]->Type == 0)
      {
      arg_types[i] = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(arg_types[i]);
      vtkParse_ValueInfoFromString(arg_types[i], cache, args[i]);
      arg_types[i]->ItemType = VTK_TYPEDEF_INFO;
      arg_types[i]->Name = arg_names[i];
      }
    }

  /* no longer a template (has been instantiated) */
  if (data->Template)
    {
    vtkParse_FreeTemplate(data->Template);
    }
  data->Template = NULL;

  /* append template args to class name */
  k = strlen(data->Name) + 2;
  for (i = 0; i < m; i++)
    {
    k += strlen(args[i]) + 2;
    }
  new_name = (char *)malloc(k);
  strcpy(new_name, data->Name);
  k = strlen(new_name);
  new_name[k++] = '<';
  for (i = 0; i < m; i++)
    {
    strcpy(&new_name[k], args[i]);
    k += strlen(args[i]);
    if (i+1 < m)
      {
      new_name[k++] = ',';
      new_name[k++] = ' ';
      }
    }
  if (new_name[k-1] == '>')
    {
    new_name[k++] = ' ';
    }
  new_name[k++] = '>';
  new_name[k] = '\0';

  data->Name = vtkParse_CacheString(cache, new_name, k);
  free(new_name);

  /* do the template arg substitution */
  class_substitution(data, cache, m, arg_names, args, arg_types);

  /* free all allocated arrays */
  free((char **)new_args);
  free((char **)arg_names);

  for (i = 0; i < m; i++)
    {
    if (arg_types[i])
      {
      vtkParse_FreeValue(arg_types[i]);
      }
    }
  free(arg_types);
}

/* Get a zero-terminated array of the types in vtkTemplateMacro. */
const char **vtkParse_GetTemplateMacroTypes()
{
  static const char *types[] = {
    "char", "signed char", "unsigned char", "short", "unsigned short",
    "int", "unsigned int", "long", "unsigned long",
#ifdef VTK_TYPE_USE_LONG_LONG
    "long long", "unsigned long long",
#endif
#ifdef VTK_TYPE_USE___INT64
    "__int64", "unsigned __int64",
#endif
    "float", "double", NULL };

  return types;
}

/* Get a zero-terminated array of the types in vtkArray. */
const char **vtkParse_GetArrayTypes()
{
  static const char *types[] = {
    "char", "signed char", "unsigned char", "short", "unsigned short",
    "int", "unsigned int", "long", "unsigned long",
#ifdef VTK_TYPE_USE_LONG_LONG
    "long long", "unsigned long long",
#endif
#ifdef VTK_TYPE_USE___INT64
    "__int64", "unsigned __int64",
#endif
    "float", "double",
    "vtkStdString", "vtkUnicodeString", "vtkVariant", NULL };

  return types;
}
