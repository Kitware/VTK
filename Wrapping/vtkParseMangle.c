/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParseMangle.c

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

#include "vtkParseMangle.h"
#include "vtkParseExtras.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

/* Generate a mangled name for a type, use gcc ia64 ABI.
 * The result is placed in new_name, which must be large enough
 * to accept the result.  This function is incomplete, it cannot
 * handle function types, or any literals except for integer literals. */
size_t vtkParse_MangledTypeName(const char *name, char *new_name)
{
  int scoped = 0;
  unsigned int ptype = 0;
  size_t j, k, m;
  size_t i = 0;
  const char *cp;
  char basictype;

  m = vtkParse_BasicTypeFromString(name, &ptype, NULL, NULL);

  /* look for pointers */
  cp = &name[m];
  while (*cp == ' ' || *cp == '\t') { cp++; }
  while (*cp == '*')
    {
    do { cp++; } while (*cp == ' ' || *cp == '\t');
    if (*cp == 'c' && strncmp(cp, "const", 5) == 0 &&
        ((cp[5] < 'A' || cp[5] > 'Z') &&
         (cp[5] < 'a' || cp[5] > 'z') &&
         (cp[5] < '0' || cp[5] > '9') &&
         cp[5] != '_'))
      {
      cp += 4;
      do { cp++; } while (*cp == ' ' || *cp == '\t');
      new_name[i++] = 'K';
      }
    new_name[i++] = 'P';
    }

  /* prepend reference if present */
  if (*cp == '&')
    {
    do { cp++; } while (*cp == ' ' || *cp == '\t');
    for (k = i; k > 0; --k)
      {
      new_name[k] = new_name[k-1];
      }
    new_name[0] = 'R';
    i++;
    }

  /* array brackets are not handled */

  /* qualifiers */
  if (ptype & VTK_PARSE_CONST)
    {
    new_name[i++] = 'K';
    }

  /* types: the following are unused
   *  'w' -> wchar_t
   *  'n' -> __int128
   *  'o' -> unsigned __int128
   *  'e' -> __float80
   *  'g' -> __float128
   *  'z' -> ... (varargs)
   */

  basictype = '\0';
  switch (ptype & VTK_PARSE_BASE_TYPE)
    {
    case VTK_PARSE_VOID:
      basictype = 'v';
      break;
    case VTK_PARSE_BOOL:
      basictype = 'b';
      break;
    case VTK_PARSE_CHAR:
      basictype = 'c';
      break;
    case VTK_PARSE_SIGNED_CHAR:
      basictype = 'a';
      break;
    case VTK_PARSE_UNSIGNED_CHAR:
      basictype = 'h';
      break;
    case VTK_PARSE_SHORT:
      basictype = 's';
      break;
    case VTK_PARSE_UNSIGNED_SHORT:
      basictype = 't';
      break;
    case VTK_PARSE_INT:
      basictype = 'i';
      break;
    case VTK_PARSE_UNSIGNED_INT:
      basictype = 'j';
      break;
    case VTK_PARSE_LONG:
      basictype = 'l';
      break;
    case VTK_PARSE_UNSIGNED_LONG:
      basictype = 'm';
      break;
    case VTK_PARSE_LONG_LONG:
    case VTK_PARSE___INT64:
      basictype = 'x';
      break;
    case VTK_PARSE_UNSIGNED_LONG_LONG:
    case VTK_PARSE_UNSIGNED___INT64:
      basictype = 'y';
      break;
    case VTK_PARSE_FLOAT:
      basictype = 'f';
      break;
    case VTK_PARSE_DOUBLE:
      basictype = 'd';
      break;
    }

  if (basictype)
    {
    new_name[i++] = basictype;
    new_name[i] = '\0';
    return (size_t)(cp - name);
    }

  m = 0;
  cp = name;
  do
    {
    cp += m;
    while (*cp == ' ' || *cp == '\t') { cp++; }
    m = vtkParse_UnscopedNameLength(cp);
    }
  while ((m == 5 && strncmp("const", cp, 5) == 0) ||
         (m == 8 && strncmp("volatile", cp, 8) == 0));

  if (cp[m] == ':' && cp[m+1] == ':')
    {
    if (m == 3 && strncmp(cp, "std::", 5) == 0)
      {
      cp += 5;
      m = vtkParse_UnscopedNameLength(cp);
      if (cp[m] == ':' && cp[m+1] == ':')
        {
        new_name[i++] = 'N';
        scoped = 1;
        }
      /* short form for "std::" */
      new_name[i++] = 'S';
      new_name[i++] = 't';
      }
    else
      {
      new_name[i++] = 'N';
      scoped = 1;
      }
    }

  do
    {
    if (cp[0] == ':' && cp[1] == ':')
      {
      cp += 2;
      m = vtkParse_UnscopedNameLength(cp);
      }

    for (j = 0; j < m; j++)
      {
      if (cp[j] == '<')
        {
        break;
        }
      }

    /* write out identifier length */
    if (j >= 100) { new_name[i++] = '0' + (char)(j/100); }
    if (j >= 10) { new_name[i++] = '0' + (char)((j%100)/10); }
    new_name[i++] = '0' + (char)(j%10);

    /* write out the identifier */
    strncpy(&new_name[i], cp, j);
    i += j;
    cp += j;

    /* handle template args */
    if (*cp == '<')
      {
      new_name[i++] = 'I';
      do
        {
        do { cp++; } while (*cp == ' ' || *cp == '\t');
        m = 0;
        if ((*cp >= '0' && *cp <= '9') ||
            (*cp == '.' && cp[1] >= '0' && cp[1] <= '9') ||
            *cp == '\'' || *cp == '\"')
          {
          m = vtkParse_MangledLiteral(cp, &new_name[i]);
          }
        else
          {
          m = vtkParse_MangledTypeName(cp, &new_name[i]);
          }
        if (m == 0)
          {
          return 0;
          }
        cp += m;
        i = strlen(new_name);
        while (*cp == ' ' || *cp == '\t') { cp++; }
        }
      while (*cp == ',');
      new_name[i++] = 'E';
      if (*cp != '>')
        {
        new_name[i] = '\0';
        return 0;
        }
      cp++;
      }
    }
  while (cp[0] == ':' && cp[1] == ':');

  if (scoped)
    {
    new_name[i++] = 'E';
    }

  new_name[i] = '\0';

  return (size_t)(cp - name);
}

/* Generate a mangled name for a literal, use gcc ia64 ABI. */
size_t vtkParse_MangledLiteral(const char *name, char *new_name)
{
  const char *cp = name;
  size_t i = 0;
  char *tmp;

  /* only decimal integers are supported for now */
  if (*cp >= '0' && *cp <= '9')
    {
    /* reject octal and hexadecimal */
    if (cp[0] == '0' && (cp[1] == 'x' || cp[1] == 'X' ||
                         (cp[1] >= '0' && cp[1] <= '9')))
      {
      new_name[0] = '\0';
      return 0;
      }

    new_name[i++] = 'L';
    tmp = &new_name[i];
    new_name[i++] = 'i';
    do { new_name[i++] = *cp++; }
    while (*cp >= '0' && *cp <= '9');

    /* reject floats */
    if (*cp == '.' || *cp == 'f' || *cp == 'e' || *cp == 'E')
      {
      new_name[0] = '\0';
      return 0;
      }

    for (;;)
      {
      if (*cp == 'u' || *cp == 'U')
        {
        if (*tmp == 'i') { *tmp = 'j'; }
        else if (*tmp == 'l') { *tmp = 'm'; }
        else if (*tmp == 'x') { *tmp = 'y'; }
        cp++;
        }
      else if (*cp == 'l' || *cp == 'L')
        {
        if (*tmp == 'i') { *tmp = 'l'; }
        else if (*tmp == 'j') { *tmp = 'm'; }
        else if (*tmp == 'l') { *tmp = 'x'; }
        else if (*tmp == 'm') { *tmp = 'y'; }
        cp++;
        }
      else
        {
        break;
        }
      }
    new_name[i++] = 'E';
    }
  new_name[i] = '\0';

  return (size_t)(cp - name);
}
