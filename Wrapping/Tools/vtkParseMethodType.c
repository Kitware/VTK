// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkParseMethodType.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>

int vtkParseMethodType_IsSetMethod(const char* name)
{
  return name && (!strncmp(name, "Set", 3) && (strnlen(name, 4) == 4) && isupper(name[3]));
}

int vtkParseMethodType_IsSetNthMethod(const char* name)
{
  if (vtkParseMethodType_IsSetMethod(name))
  {
    return (!strncmp(&name[3], "Nth", 3) && (strnlen(name, 7) == 7) && isupper(name[6]));
  }

  return 0;
}

int vtkParseMethodType_IsSetNumberOfMethod(const char* name)
{
  size_t n;

  if (vtkParseMethodType_IsSetMethod(name))
  {
    n = strlen(name);
    return (!strncmp(&name[3], "NumberOf", 8) && n > 11 && isupper(name[11]) && name[n - 1] == 's');
  }

  return 0;
}

int vtkParseMethodType_IsGetMethod(const char* name)
{
  return (name && !strncmp(name, "Get", 3) && (strnlen(name, 4) == 4) && isupper(name[3]));
}

int vtkParseMethodType_IsGetNthMethod(const char* name)
{
  if (vtkParseMethodType_IsGetMethod(name))
  {
    return (!strncmp(&name[3], "Nth", 3) && (strnlen(name, 7) == 7) && isupper(name[6]));
  }

  return 0;
}

int vtkParseMethodType_IsGetNumberOfMethod(const char* name)
{
  size_t n;

  if (vtkParseMethodType_IsGetMethod(name))
  {
    n = strlen(name);
    return (!strncmp(&name[3], "NumberOf", 8) && n > 11 && isupper(name[11]) && name[n - 1] == 's');
  }

  return 0;
}

int vtkParseMethodType_IsBooleanMethod(const char* name)
{
  size_t n;

  if (name)
  {
    n = strlen(name);
    if (((n > 2) && !strncmp(&name[n - 2], "On", 2)) ||
      ((n > 3) && !strncmp(&name[n - 3], "Off", 3)))
    {
      return 1;
    }
  }

  return 0;
}

int vtkParseMethodType_IsEnumeratedMethod(const char* name)
{
  size_t i, n;

  if (vtkParseMethodType_IsSetMethod(name))
  {
    n = strlen(name) - 3;
    for (i = 3; i < n; i++)
    {
      if (!strncmp(&name[i], "To", 2) && (isupper(name[i + 2]) || isdigit(name[i + 2])))
      {
        return 1;
      }
    }
  }

  return 0;
}

int vtkParseMethodType_IsAsStringMethod(const char* name)
{
  size_t n;

  if (vtkParseMethodType_IsGetMethod(name))
  {
    n = strlen(name);
    if (!strncmp(&name[n - 8], "AsString", 8))
    {
      return 1;
    }
  }

  return 0;
}

int vtkParseMethodType_IsAddMethod(const char* name)
{
  return (name && !strncmp(name, "Add", 3) && (strnlen(name, 4) == 4) && isupper(name[3]) &&
    !vtkParseMethodType_IsBooleanMethod(name));
}

int vtkParseMethodType_IsRemoveMethod(const char* name)
{
  return (name && !strncmp(name, "Remove", 6) && (strnlen(name, 7) == 7) && isupper(name[6]) &&
    !vtkParseMethodType_IsBooleanMethod(name));
}

int vtkParseMethodType_IsRemoveAllMethod(const char* name)
{
  size_t n;

  if (vtkParseMethodType_IsRemoveMethod(name))
  {
    n = strlen(name);
    return (!strncmp(&name[6], "All", 3) && (n > 9) && isupper(name[9]));
  }

  return 0;
}

int vtkParseMethodType_IsGetMinValueMethod(const char* name)
{
  size_t n;

  if (vtkParseMethodType_IsGetMethod(name))
  {
    n = strlen(name);
    if (n > 11 && strncmp("MinValue", &name[n - 8], 8) == 0)
    {
      return 1;
    }
  }

  return 0;
}

int vtkParseMethodType_IsGetMaxValueMethod(const char* name)
{
  size_t n;

  if (vtkParseMethodType_IsGetMethod(name))
  {
    n = strlen(name);
    if (n > 11 && strncmp("MaxValue", &name[n - 8], 8) == 0)
    {
      return 1;
    }
  }

  return 0;
}
