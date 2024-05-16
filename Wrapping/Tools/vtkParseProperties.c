// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2010,2015 David Gobbi
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkParseProperties.h"
#include "vtkParseData.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/*-------------------------------------------------------------------
 * A struct that lays out the function information in a way
 * that makes it easy to find methods that act on the same ivars.
 * Only ivar methods will properly fit this struct. */

typedef struct MethodAttributes_
{
  const char* Name;      /* method name */
  unsigned int Type;     /* data type of gettable/settable value */
  int Count;             /* count for gettable/settable value */
  const char* ClassName; /* class name for if the type is a class */
  const char* Comment;   /* documentation for method */
  parse_access_t Access; /* method is private, protected */
  int HasProperty;       /* method accesses a property */
  int IsLegacy;          /* method is marked "legacy" */
  int IsStatic;          /* method is static */
  int IsRepeat;          /* method is a repeat of a similar method */
  int IsHinted;          /* method has a hint */
  int IsMultiValue;      /* method is e.g. SetValue(x0, x1, x2) */
  int IsIndexed;         /* method is e.g. SetValue(i, val) */
  int IsEnumerated;      /* method is e.g. SetValueToSomething() */
  int IsBoolean;         /* method is ValueOn() or ValueOff() */
  int IsRHS;             /* method is GetValue(val), not val = GetValue() */
  int IsNoDiscard;       /* method is int AddValue() or bool RemoveValue() */
} MethodAttributes;

typedef struct ClassPropertyMethods_
{
  int NumberOfMethods;
  MethodAttributes** Methods;
} ClassPropertyMethods;

/*-------------------------------------------------------------------
 * Checks for various common method names for property access */

static int isSetMethod(const char* name)
{
  return name && (!strncmp(name, "Set", 3) && (strnlen(name, 4) == 4) && isupper(name[3]));
}

static int isSetNthMethod(const char* name)
{
  if (isSetMethod(name))
  {
    return (!strncmp(&name[3], "Nth", 3) && (strnlen(name, 7) == 7) && isupper(name[6]));
  }

  return 0;
}

static int isSetNumberOfMethod(const char* name)
{
  size_t n;

  if (isSetMethod(name))
  {
    n = strlen(name);
    return (!strncmp(&name[3], "NumberOf", 8) && n > 11 && isupper(name[11]) && name[n - 1] == 's');
  }

  return 0;
}

static int isGetMethod(const char* name)
{
  return (name && !strncmp(name, "Get", 3) && (strnlen(name, 4) == 4) && isupper(name[3]));
}

static int isGetNthMethod(const char* name)
{
  if (isGetMethod(name))
  {
    return (!strncmp(&name[3], "Nth", 3) && (strnlen(name, 7) == 7) && isupper(name[6]));
  }

  return 0;
}

static int isGetNumberOfMethod(const char* name)
{
  size_t n;

  if (isGetMethod(name))
  {
    n = strlen(name);
    return (!strncmp(&name[3], "NumberOf", 8) && n > 11 && isupper(name[11]) && name[n - 1] == 's');
  }

  return 0;
}

static int isBooleanMethod(const char* name)
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

static int isEnumeratedMethod(const char* name)
{
  size_t i, n;

  if (isSetMethod(name))
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

static int isAsStringMethod(const char* name)
{
  size_t n;

  if (isGetMethod(name))
  {
    n = strlen(name);
    if (!strncmp(&name[n - 8], "AsString", 8))
    {
      return 1;
    }
  }

  return 0;
}

static int isAddMethod(const char* name)
{
  return (name && !strncmp(name, "Add", 3) && (strnlen(name, 4) == 4) && isupper(name[3]) &&
    !isBooleanMethod(name));
}

static int isRemoveMethod(const char* name)
{
  return (name && !strncmp(name, "Remove", 6) && (strnlen(name, 7) == 7) && isupper(name[6]) &&
    !isBooleanMethod(name));
}

static int isRemoveAllMethod(const char* name)
{
  size_t n;

  if (isRemoveMethod(name))
  {
    n = strlen(name);
    return (!strncmp(&name[6], "All", 3) && (n > 9) && isupper(name[9]));
  }

  return 0;
}

static int isGetMinValueMethod(const char* name)
{
  size_t n;

  if (isGetMethod(name))
  {
    n = strlen(name);
    if (n > 11 && strncmp("MinValue", &name[n - 8], 8) == 0)
    {
      return 1;
    }
  }

  return 0;
}

static int isGetMaxValueMethod(const char* name)
{
  size_t n;

  if (isGetMethod(name))
  {
    n = strlen(name);
    if (n > 11 && strncmp("MaxValue", &name[n - 8], 8) == 0)
    {
      return 1;
    }
  }

  return 0;
}

/*-------------------------------------------------------------------
 * Return the method category bit for the given method, based on the
 * method name and other information in the MethodAttributes struct.
 * If shortForm in on, then suffixes such as On, Off, AsString,
 * and ToSomething are considered while doing the categorization */

static unsigned int methodCategory(MethodAttributes* meth, int shortForm)
{
  size_t n;
  const char* name;
  name = meth->Name;

  if (isSetMethod(name))
  {
    if (meth->IsEnumerated)
    {
      return VTK_METHOD_SET_VALUE_TO;
    }
    else if (meth->IsIndexed)
    {
      if (isSetNthMethod(name))
      {
        return VTK_METHOD_SET_NTH;
      }
      else
      {
        return VTK_METHOD_SET_IDX;
      }
    }
    else if (meth->IsMultiValue)
    {
      return VTK_METHOD_SET_MULTI;
    }
    else if (shortForm && isSetNumberOfMethod(name))
    {
      return VTK_METHOD_SET_NUMBER_OF;
    }
    else
    {
      return VTK_METHOD_SET;
    }
  }
  else if (meth->IsBoolean)
  {
    n = strlen(name);
    if (name[n - 1] == 'n')
    {
      return VTK_METHOD_BOOL_ON;
    }
    else
    {
      return VTK_METHOD_BOOL_OFF;
    }
  }
  else if (isGetMethod(name))
  {
    if (shortForm && isGetMinValueMethod(name))
    {
      return VTK_METHOD_GET_MIN_VALUE;
    }
    else if (shortForm && isGetMaxValueMethod(name))
    {
      return VTK_METHOD_GET_MAX_VALUE;
    }
    else if (shortForm && isAsStringMethod(name))
    {
      return VTK_METHOD_GET_AS_STRING;
    }
    else if (meth->IsIndexed && meth->IsRHS)
    {
      if (isGetNthMethod(name))
      {
        return VTK_METHOD_GET_NTH_RHS;
      }
      else
      {
        return VTK_METHOD_GET_IDX_RHS;
      }
    }
    else if (meth->IsIndexed)
    {
      if (isGetNthMethod(name))
      {
        return VTK_METHOD_GET_NTH;
      }
      else
      {
        return VTK_METHOD_GET_IDX;
      }
    }
    else if (meth->IsMultiValue)
    {
      return VTK_METHOD_GET_MULTI;
    }
    else if (meth->IsRHS)
    {
      return VTK_METHOD_GET_RHS;
    }
    else if (shortForm && isGetNumberOfMethod(name))
    {
      return VTK_METHOD_GET_NUMBER_OF;
    }
    else
    {
      return VTK_METHOD_GET;
    }
  }
  else if (isRemoveMethod(name))
  {
    if (isRemoveAllMethod(name))
    {
      return VTK_METHOD_REMOVE_ALL;
    }
    else if (meth->IsIndexed)
    {
      return VTK_METHOD_REMOVE_IDX;
    }
    else if (meth->IsNoDiscard)
    {
      return VTK_METHOD_REMOVE_NODISCARD;
    }
    else
    {
      return VTK_METHOD_REMOVE;
    }
  }
  else if (isAddMethod(name))
  {
    if (meth->IsIndexed)
    {
      return VTK_METHOD_ADD_IDX;
    }
    else if (meth->IsMultiValue)
    {
      return VTK_METHOD_ADD_MULTI;
    }
    else if (meth->IsNoDiscard)
    {
      return VTK_METHOD_ADD_NODISCARD;
    }
    else
    {
      return VTK_METHOD_ADD;
    }
  }

  return 0;
}

/*-------------------------------------------------------------------
 * remove the following prefixes from a method name:
 * Set, Get, Add, Remove */

static const char* nameWithoutPrefix(const char* name)
{
  if (isGetNthMethod(name) || isSetNthMethod(name))
  {
    return &name[6];
  }
  else if (isGetMethod(name) || isSetMethod(name) || isAddMethod(name))
  {
    return &name[3];
  }
  else if (isRemoveAllMethod(name))
  {
    return &name[9];
  }
  else if (isRemoveMethod(name))
  {
    return &name[6];
  }

  return name;
}

/*-------------------------------------------------------------------
 * check for a valid suffix, i.e. "On" or "Off" or "ToSomething" */

static int isValidSuffix(const char* methName, const char* propertyName, const char* suffix)
{
  if (!strncmp(suffix, "On", 2) || !strncmp(suffix, "Off", 3))
  {
    return 1;
  }

  else if (isSetMethod(methName) && !strncmp(suffix, "To", 2) &&
    (isupper(suffix[2]) || isdigit(suffix[2])))
  {
    return 1;
  }

  /* AsString or MaxValue or MinValue or AsN or */
  else if (isGetMethod(methName) &&
    ((!strncmp(suffix, "As", 2) && (strnlen(suffix, 3) == 3) &&
       ((suffix[2] == 'S') || isdigit(suffix[2]))) ||
      ((!strncmp(suffix, "Max", 3) || !strncmp(suffix, "Min", 3)) &&
        !strncmp(&suffix[3], "Value", 5))))
  {
    return 1;
  }

  else if (isRemoveAllMethod(methName))
  {
    return (suffix[0] == 's' && suffix[1] == '\0');
  }

  else if (isGetNumberOfMethod(methName) || isSetNumberOfMethod(methName))
  {
    if (strncmp(propertyName, "NumberOf", 8) == 0)
    {
      return (suffix[0] == '\0');
    }
    else
    {
      return (suffix[0] == 's' && suffix[1] == '\0');
    }
  }
  else if (suffix[0] == '\0')
  {
    return 1;
  }

  return 0;
}

static int isIntegral(const ValueInfo* val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);

  if (t != VTK_PARSE_UNSIGNED_CHAR)
  {
    t = (t & ~VTK_PARSE_UNSIGNED);
  }
  switch (t)
  {
    case VTK_PARSE_SHORT:
    case VTK_PARSE_INT:
    case VTK_PARSE_LONG:
    case VTK_PARSE_LONG_LONG:
    case VTK_PARSE_UNSIGNED_CHAR:
    case VTK_PARSE_SIGNED_CHAR:
    case VTK_PARSE_SSIZE_T:
      return 1;
    default:
      break;
  }

  return 0;
}

/*-------------------------------------------------------------------
 * Convert the FunctionInfo into a MethodAttributes, which will make
 * it easier to find matched Set/Get methods.  A return value of zero
 * means the conversion failed, i.e. the method signature is too complex
 * for the MethodAttributes struct */

static int getMethodAttributes(FunctionInfo* func, MethodAttributes* attrs)
{
  int i, n;
  unsigned int tmptype = 0;
  const char* tmpclass = 0;
  int allSame = 0;
  int indexed = 0;

  attrs->Name = func->Name;
  attrs->HasProperty = 0;
  attrs->Type = 0;
  attrs->Count = 0;
  attrs->ClassName = 0;
  attrs->Comment = func->Comment;
  attrs->Access = func->Access;
  attrs->IsLegacy = func->IsLegacy;
  attrs->IsStatic = func->IsStatic;
  attrs->IsRepeat = 0;
  attrs->IsMultiValue = 0;
  attrs->IsHinted = 0;
  attrs->IsIndexed = 0;
  attrs->IsEnumerated = 0;
  attrs->IsBoolean = 0;
  attrs->IsRHS = 0;
  attrs->IsNoDiscard = 0;

  /* check for major issues with the function */
  if (!func->Name || func->IsOperator || (func->ReturnValue && func->ReturnValue->Function) ||
    (func->ReturnValue && func->ReturnValue->NumberOfDimensions > 1))
  {
    return 0;
  }
  for (i = 0; i < func->NumberOfParameters; i++)
  {
    if (func->Parameters[i]->Function || func->Parameters[i]->NumberOfDimensions > 1)
    {
      return 0;
    }
  }

  /* check for indexed methods: the first argument will be an integer */
  if (func->NumberOfParameters > 0 && isIntegral(func->Parameters[0]))
  {
    /* methods of the form "void SetValue(int i, type value)" */
    if ((!func->ReturnValue ||
          (func->ReturnValue->Type & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VOID) &&
      func->NumberOfParameters == 2)
    {
      indexed = 1;

      if (!isSetNumberOfMethod(func->Name))
      {
        /* make sure this isn't a multi-value int method */
        tmptype = func->Parameters[0]->Type;
        tmpclass = func->Parameters[0]->Class;
        allSame = 1;

        n = func->NumberOfParameters;
        for (i = 0; i < n; i++)
        {
          if (func->Parameters[i]->Type != tmptype)
          {
            allSame = 0;
          }
        }
        indexed = !allSame;
      }
    }
    /* methods of the form "type GetValue(int i)" */
    if ((!func->ReturnValue ||
          (func->ReturnValue->Type & VTK_PARSE_UNQUALIFIED_TYPE) != VTK_PARSE_VOID) &&
      func->NumberOfParameters == 1)
    {
      indexed = 1;
    }

    attrs->IsIndexed = indexed;
  }

  /* if return type is not void and no parameters or 1 index */
  if ((func->ReturnValue &&
        (func->ReturnValue->Type & VTK_PARSE_UNQUALIFIED_TYPE) != VTK_PARSE_VOID) &&
    func->NumberOfParameters == indexed)
  {
    /* methods of the form "type GetValue()" or "type GetValue(i)" */
    if (isGetMethod(func->Name))
    {
      attrs->HasProperty = 1;
      attrs->Type = func->ReturnValue->Type;
      attrs->Count = func->ReturnValue->Count;
      attrs->IsHinted = (attrs->Count > 0);
      attrs->ClassName = func->ReturnValue->Class;

      return 1;
    }
  }

  /* if return type is void and 1 arg or 1 index and 1 arg */
  if ((!func->ReturnValue ||
        (func->ReturnValue->Type & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VOID) &&
    func->NumberOfParameters == (1 + indexed))
  {
    /* "void SetValue(type)" or "void SetValue(int, type)" */
    if (isSetMethod(func->Name))
    {
      attrs->HasProperty = 1;
      attrs->IsRHS = 1;
      attrs->Type = func->Parameters[indexed]->Type;
      attrs->Count = func->Parameters[indexed]->Count;
      attrs->ClassName = func->Parameters[indexed]->Class;

      return 1;
    }
    /* "void GetValue(type *)" or "void GetValue(int, type *)" */
    else if (isGetMethod(func->Name) &&
      /* func->Parameters[indexed]->Count > 0 && */
      (func->Parameters[indexed]->Type & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER &&
      (func->Parameters[indexed]->Type & VTK_PARSE_CONST) == 0)
    {
      attrs->HasProperty = 1;
      attrs->IsRHS = 1;
      attrs->Type = func->Parameters[indexed]->Type;
      attrs->Count = func->Parameters[indexed]->Count;
      attrs->ClassName = func->Parameters[indexed]->Class;

      return 1;
    }
    /* "void AddValue(vtkObject *)" or "void RemoveValue(vtkObject *)" */
    else if ((isAddMethod(func->Name) || isRemoveMethod(func->Name)) &&
      (func->Parameters[indexed]->Type & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_OBJECT_PTR)
    {
      attrs->HasProperty = 1;
      attrs->IsRHS = 1;
      attrs->Type = func->Parameters[indexed]->Type;
      attrs->Count = func->Parameters[indexed]->Count;
      attrs->ClassName = func->Parameters[indexed]->Class;

      return 1;
    }
  }

  /* if return type is void and 1 arg or 1 index and 1 arg */
  if ((!func->ReturnValue ||
        (func->ReturnValue->Type & VTK_PARSE_UNQUALIFIED_TYPE) != VTK_PARSE_VOID) &&
    func->NumberOfParameters == (1 + indexed))
  {
    /* "int AddValue(vtkObject *)" or "bool RemoveValue(vtkObject *)" */
    if ((isAddMethod(func->Name) || isRemoveMethod(func->Name)) &&
      (func->Parameters[indexed]->Type & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_OBJECT_PTR)
    {
      attrs->HasProperty = 1;
      attrs->IsRHS = 1;
      attrs->IsNoDiscard = 1;
      attrs->Type = func->Parameters[indexed]->Type;
      attrs->Count = func->Parameters[indexed]->Count;
      attrs->ClassName = func->Parameters[indexed]->Class;

      return 1;
    }
  }

  /* check for multiple arguments of the same type */
  if (func->NumberOfParameters > 1 && !indexed)
  {
    tmptype = func->Parameters[0]->Type;
    tmpclass = func->Parameters[0]->Class;
    allSame = 1;

    n = func->NumberOfParameters;
    for (i = 0; i < n; i++)
    {
      if (func->Parameters[i]->Type != tmptype)
      {
        allSame = 0;
      }
    }

    if (allSame)
    {
      /* "void SetValue(type x, type y, type z)" */
      if (isSetMethod(func->Name) && (tmptype & VTK_PARSE_INDIRECT) == 0 &&
        (!func->ReturnValue ||
          (func->ReturnValue->Type & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VOID))
      {
        attrs->HasProperty = 1;
        attrs->Type = tmptype;
        attrs->ClassName = tmpclass;
        attrs->Count = n;
        attrs->IsMultiValue = 1;

        return 1;
      }
      /* "void GetValue(type& x, type& x, type& x)" */
      else if (isGetMethod(func->Name) && (tmptype & VTK_PARSE_REF) != 0 &&
        (tmptype & VTK_PARSE_CONST) == 0 &&
        (!func->ReturnValue ||
          (func->ReturnValue->Type & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VOID))
      {
        attrs->HasProperty = 1;
        attrs->Type = tmptype;
        attrs->ClassName = tmpclass;
        attrs->Count = n;
        attrs->IsMultiValue = 1;

        return 1;
      }
      /* "void AddValue(type x, type y, type z)" */
      else if (isAddMethod(func->Name) && (tmptype & VTK_PARSE_INDIRECT) == 0 &&
        (!func->ReturnValue ||
          (func->ReturnValue->Type & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VOID ||
          (func->ReturnValue->Type & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_INT ||
          (func->ReturnValue->Type & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_SIZE_T ||
          (func->ReturnValue->Type & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_ID_TYPE))
      {
        attrs->HasProperty = 1;
        attrs->Type = tmptype;
        attrs->ClassName = tmpclass;
        attrs->Count = n;
        attrs->IsMultiValue = 1;

        return 1;
      }
    }
  }

  /* if return type is void, and there are no arguments */
  if ((!func->ReturnValue ||
        (func->ReturnValue->Type & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VOID) &&
    func->NumberOfParameters == 0)
  {
    attrs->Type = VTK_PARSE_VOID;
    attrs->ClassName = "void";

    /* "void ValueOn()" or "void ValueOff()" */
    if (isBooleanMethod(func->Name))
    {
      attrs->HasProperty = 1;
      attrs->IsBoolean = 1;
      return 1;
    }
    /* "void SetValueToEnum()" */
    else if (isEnumeratedMethod(func->Name))
    {
      attrs->HasProperty = 1;
      attrs->IsEnumerated = 1;
      return 1;
    }
    /* "void RemoveAllValues()" */
    else if (isRemoveAllMethod(func->Name))
    {
      attrs->HasProperty = 1;
      return 1;
    }
  }

  return 0;
}

/*-------------------------------------------------------------------
 * Check to see if the specified method is a match with the specified
 * property, i.e the name, type, and array count of the property
 * must match.  The longMatch value is set to '1' if the prefix/suffix
 * was part of the name match. */

static int methodMatchesProperty(
  const HierarchyInfo* hinfo, PropertyInfo* property, MethodAttributes* meth, int* longMatch)
{
  size_t n;
  int propertyType, methType;
  const char* propertyName;
  const char* name;
  const char* methSuffix;
  int methodBitfield = 0;

  /* get the bitfield containing all found methods for this property */
  if (meth->Access == VTK_ACCESS_PUBLIC)
  {
    methodBitfield = property->PublicMethods;
  }
  else if (meth->Access == VTK_ACCESS_PROTECTED)
  {
    methodBitfield = property->ProtectedMethods;
  }
  else
  {
    methodBitfield = property->PrivateMethods;
  }

  /* get the property name and compare it to the method name */
  propertyName = property->Name;
  name = nameWithoutPrefix(meth->Name);

  if (name == 0 || propertyName == 0)
  {
    return 0;
  }

  /* longMatch is only set for full matches of GetNumberOf(),
   * SetNumberOf(), GetVarMinValue(), GetVarMaxValue() methods */
  *longMatch = 0;
  n = strlen(propertyName);
  if (isGetNumberOfMethod(meth->Name) || isSetNumberOfMethod(meth->Name))
  {
    if (strncmp(propertyName, "NumberOf", 8) == 0 && isupper(propertyName[8]))
    {
      /* longer match */
      *longMatch = 1;
    }
    else
    {
      /* longer prefix */
      name = &meth->Name[11];
    }
  }
  else if (isGetMinValueMethod(meth->Name))
  {
    if (n >= 8 && strncmp(&propertyName[n - 8], "MinValue", 8) == 0)
    {
      *longMatch = 1;
    }
  }
  else if (isGetMaxValueMethod(meth->Name))
  {
    if (n >= 8 && strncmp(&propertyName[n - 8], "MaxValue", 8) == 0)
    {
      *longMatch = 1;
    }
  }
  else if (isAsStringMethod(meth->Name))
  {
    if (n >= 8 && strncmp(&propertyName[n - 8], "AsString", 8) == 0)
    {
      *longMatch = 1;
    }
  }

  /* make sure the method name contains the property name */
  if (strncmp(name, propertyName, n) != 0)
  {
    return 0;
  }

  /* make sure that any non-matching bits are valid suffixes */
  methSuffix = &name[n];
  if (!isValidSuffix(meth->Name, propertyName, methSuffix))
  {
    return 0;
  }

  /* check for type match */
  methType = meth->Type;
  propertyType = property->Type;

  /* remove "const" and "static" */
  methType = (methType & VTK_PARSE_UNQUALIFIED_TYPE);

  /* check for RemoveAll method matching an Add method*/
  if (isRemoveAllMethod(meth->Name) && methType == VTK_PARSE_VOID &&
    (methType & VTK_PARSE_INDIRECT) == 0 &&
    ((methodBitfield & (VTK_METHOD_ADD | VTK_METHOD_ADD_MULTI | VTK_METHOD_ADD_NODISCARD)) != 0))
  {
    return 1;
  }

  /* check for GetNumberOf and SetNumberOf for indexed properties */
  if (isGetNumberOfMethod(meth->Name) &&
    (methType == VTK_PARSE_INT || methType == VTK_PARSE_SIZE_T || methType == VTK_PARSE_ID_TYPE) &&
    (methType & VTK_PARSE_INDIRECT) == 0 &&
    ((methodBitfield & (VTK_METHOD_GET_IDX | VTK_METHOD_GET_NTH)) != 0))
  {
    return 1;
  }

  if (isSetNumberOfMethod(meth->Name) &&
    (methType == VTK_PARSE_INT || methType == VTK_PARSE_SIZE_T || methType == VTK_PARSE_ID_TYPE) &&
    (methType & VTK_PARSE_INDIRECT) == 0 &&
    ((methodBitfield & (VTK_METHOD_SET_IDX | VTK_METHOD_SET_NTH)) != 0))
  {
    return 1;
  }

  /* remove ampersands i.e. "ref" */
  methType = (methType & ~VTK_PARSE_REF);

  /* if method is multivalue, e.g. SetColor(r,g,b), then the
   * referenced property is a pointer */
  if (meth->IsMultiValue)
  {
    if ((methType & VTK_PARSE_POINTER_MASK) == VTK_PARSE_POINTER)
    {
      methType = (methType & ~VTK_PARSE_INDIRECT);
      methType = (methType | VTK_PARSE_POINTER_POINTER);
    }
    else if ((methType & VTK_PARSE_POINTER_MASK) == 0)
    {
      methType = (methType | VTK_PARSE_POINTER);
    }
    else
    {
      return 0;
    }
  }

  /* promote "void" to enumerated type for e.g. boolean methods, and */
  /* check for GetValueAsString method, assume it has matching enum */
  if (meth->IsBoolean || meth->IsEnumerated ||
    (isAsStringMethod(meth->Name) && (methType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_CHAR_PTR))
  {
    if ((propertyType & VTK_PARSE_INDIRECT) == 0 &&
      (propertyType == VTK_PARSE_INT || propertyType == VTK_PARSE_UNSIGNED_INT ||
        propertyType == VTK_PARSE_UNSIGNED_CHAR ||
        (meth->IsBoolean && propertyType == VTK_PARSE_BOOL)))
    {
      methType = propertyType;
    }
  }

  /* signed integer promotion */
  int methTypeInt = (methType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_INT;
  int methTypeLong = (methType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_LONG;
  int methTypeLongLong = (methType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_LONG_LONG;
  int propertyTypeInt = (propertyType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_INT;
  int propertyTypeLong = (propertyType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_LONG;
  int propertyTypeLongLong = (propertyType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_LONG_LONG;
  if ((methTypeInt && propertyTypeLong) || (methTypeInt && propertyTypeLongLong) ||
    (methTypeLong && propertyTypeInt) || (methTypeLong && propertyTypeLongLong) ||
    (methTypeLongLong && propertyTypeInt) || (methTypeLongLong && propertyTypeLong) ||
    (propertyTypeInt && methTypeLong) || (propertyTypeInt && methTypeLongLong) ||
    (propertyTypeLong && methTypeInt) || (propertyTypeLong && methTypeLongLong) ||
    (propertyTypeLongLong && methTypeInt) || (propertyTypeLongLong && methTypeLong))
  {
    methType = propertyType;
  }

  /* check for matched type and count */
  if (methType != propertyType || meth->Count != property->Count)
  {
    return 0;
  }

  /* if vtkObject, check that classes match or atleast one is derived from the other */
  if ((methType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_OBJECT)
  {
    HierarchyEntry* methEntry = vtkParseHierarchy_FindEntry(hinfo, meth->ClassName);
    if (meth->IsMultiValue || (methType & VTK_PARSE_POINTER_MASK) == 0 || meth->Count != 0 ||
      meth->ClassName == 0 || property->ClassName == 0 ||
      (strcmp(meth->ClassName, property->ClassName) != 0 &&
        !vtkParseHierarchy_IsTypeOf(hinfo, methEntry, property->ClassName)))
    {
      return 0;
    }
  }

  /* done, match was found! */
  return 1;
}

/*-------------------------------------------------------------------
 * initialize a PropertyInfo struct from a MethodAttributes
 * struct, only valid if the method name has no suffixes such as
 * On/Off, AsString, ToSomething, RemoveAllSomethings, etc. */

static void initializePropertyInfo(
  PropertyInfo* property, MethodAttributes* meth, unsigned int methodBit)
{
  unsigned int type;
  const char* typeClass;
  type = meth->Type;
  typeClass = meth->ClassName;

  /* for ValueOn()/Off() or SetValueToEnum() methods, set type to int */
  if (meth->IsBoolean || meth->IsEnumerated)
  {
    type = VTK_PARSE_INT;
    typeClass = "int";
  }

  property->Name = nameWithoutPrefix(meth->Name);

  /* get property type, but don't include "ref" as part of type,
   * and use a pointer if the method is multi-valued */
  property->Type = (type & VTK_PARSE_BASE_TYPE);
  property->ClassName = typeClass;
  if ((!meth->IsMultiValue && (type & VTK_PARSE_POINTER_MASK) == VTK_PARSE_POINTER) ||
    (meth->IsMultiValue && (type & VTK_PARSE_POINTER_MASK) == 0))
  {
    property->Type = (property->Type | VTK_PARSE_POINTER);
  }
  else if ((!meth->IsMultiValue && (type & VTK_PARSE_POINTER_MASK) == VTK_PARSE_POINTER))
  {
    property->Type = (property->Type | VTK_PARSE_POINTER);
    property->Type = (property->Type | (type & VTK_PARSE_CONST));
  }
  else if (((type & VTK_PARSE_POINTER_MASK) == VTK_PARSE_POINTER && meth->IsMultiValue) ||
    (type & VTK_PARSE_POINTER_MASK) == VTK_PARSE_POINTER_POINTER)
  {
    property->Type = (property->Type | VTK_PARSE_POINTER_POINTER);
    property->Type = (property->Type | (type & VTK_PARSE_CONST));
  }

  property->ClassName = meth->ClassName;
  property->Count = meth->Count;
  property->IsStatic = meth->IsStatic;
  property->EnumConstantNames = 0;
  property->PublicMethods = 0;
  property->ProtectedMethods = 0;
  property->PrivateMethods = 0;
  property->LegacyMethods = 0;
  property->Comment = meth->Comment;

  if (meth->Access == VTK_ACCESS_PUBLIC)
  {
    property->PublicMethods = methodBit;
  }
  else if (meth->Access == VTK_ACCESS_PROTECTED)
  {
    property->ProtectedMethods = methodBit;
  }
  else
  {
    property->PrivateMethods = methodBit;
  }

  if (meth->IsLegacy)
  {
    property->LegacyMethods = methodBit;
  }
}

/*-------------------------------------------------------------------
 * Find all the methods that match the specified method, and add
 * flags to the PropertyInfo struct */

static void findAllMatches(const HierarchyInfo* hinfo, PropertyInfo* property, int propertyId,
  ClassPropertyMethods* methods, int matchedMethods[], unsigned int methodCategories[],
  int methodHasProperty[], int methodProperties[])
{
  int i, j, k, n;
  size_t m;
  MethodAttributes* meth;
  unsigned int methodBit;
  int longMatch;
  int foundNoMatches = 0;

  n = methods->NumberOfMethods;

  /* loop repeatedly until no more matches are found */
  while (!foundNoMatches)
  {
    foundNoMatches = 1;

    for (i = 0; i < n; i++)
    {
      if (matchedMethods[i])
      {
        continue;
      }

      meth = methods->Methods[i];
      if (methodMatchesProperty(hinfo, property, meth, &longMatch))
      {
        matchedMethods[i] = 1;
        foundNoMatches = 0;

        /* if any method is static, the property is static */
        if (meth->IsStatic)
        {
          property->IsStatic = 1;
        }

        /* add this as a member of the method bitfield, and consider method
         * suffixes like On, MaxValue, etc. while doing the categorization */
        methodBit = methodCategory(meth, !longMatch);
        methodCategories[i] = methodBit;
        methodHasProperty[i] = 1;
        methodProperties[i] = propertyId;

        if (meth->Access == VTK_ACCESS_PUBLIC)
        {
          property->PublicMethods |= methodBit;
        }
        else if (meth->Access == VTK_ACCESS_PROTECTED)
        {
          property->ProtectedMethods |= methodBit;
        }
        else
        {
          property->PrivateMethods |= methodBit;
        }

        if (meth->IsLegacy)
        {
          property->LegacyMethods |= methodBit;
        }

        if (meth->IsEnumerated)
        {
          m = strlen(property->Name);
          if (!strncmp(&meth->Name[3 + m], "To", 2) &&
            (isdigit(meth->Name[5 + m]) || isupper(meth->Name[5 + m])))
          {
            if (property->EnumConstantNames == 0)
            {
              property->EnumConstantNames = (const char**)malloc(sizeof(char*) * 8);
              property->EnumConstantNames[0] = 0;
            }

            j = 0;
            while (property->EnumConstantNames[j] != 0)
            {
              j++;
            }
            property->EnumConstantNames[j++] = &meth->Name[5 + m];
            if (j % 8 == 0)
            {
              const char** savenames = property->EnumConstantNames;
              property->EnumConstantNames = (const char**)malloc(sizeof(char*) * (j + 8));
              for (k = 0; k < j; k++)
              {
                property->EnumConstantNames[k] = savenames[k];
              }
              free((void*)savenames);
            }
            property->EnumConstantNames[j] = 0;
          }
        }
      }
    }
  }
}

/*-------------------------------------------------------------------
 * search for methods that are repeated with minor variations */
static int searchForRepeatedMethods(
  ClassProperties* properties, ClassPropertyMethods* methods, int j)
{
  int i, n;
  MethodAttributes* attrs;
  MethodAttributes* meth;
  n = methods->NumberOfMethods;

  attrs = methods->Methods[j];

  for (i = 0; i < n; i++)
  {
    meth = methods->Methods[i];

    /* check whether the function name and basic structure are matched */
    if (meth->Name && strcmp(attrs->Name, meth->Name) == 0 &&
      ((attrs->Type & VTK_PARSE_POINTER_MASK) == (meth->Type & VTK_PARSE_POINTER_MASK)) &&
      attrs->Access == meth->Access && attrs->IsHinted == meth->IsHinted &&
      attrs->IsMultiValue == meth->IsMultiValue && attrs->IsIndexed == meth->IsIndexed &&
      attrs->IsEnumerated == meth->IsEnumerated && attrs->IsBoolean == meth->IsBoolean)
    {
      /* check to see if the types are compatible:
       * prefer "double" over "float",
       * prefer higher-counted arrays,
       * prefer non-legacy methods */

      if (((attrs->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_FLOAT &&
            (meth->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_DOUBLE) ||
        ((attrs->Type & VTK_PARSE_BASE_TYPE) == (meth->Type & VTK_PARSE_BASE_TYPE) &&
          attrs->Count < meth->Count) ||
        (attrs->IsLegacy && !meth->IsLegacy))
      {
        /* keep existing method */
        attrs->IsRepeat = 1;
        if (properties)
        {
          properties->MethodTypes[j] = properties->MethodTypes[i];
          properties->MethodHasProperty[j] = properties->MethodHasProperty[i];
          properties->MethodProperties[j] = properties->MethodProperties[i];
        }
        return 0;
      }

      if (((attrs->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_DOUBLE &&
            (meth->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_FLOAT) ||
        ((attrs->Type & VTK_PARSE_BASE_TYPE) == (meth->Type & VTK_PARSE_BASE_TYPE) &&
          attrs->Count > meth->Count) ||
        (!attrs->IsLegacy && meth->IsLegacy))
      {
        /* keep this method */
        meth->IsRepeat = 1;
        if (properties)
        {
          properties->MethodTypes[i] = properties->MethodTypes[j];
          properties->MethodHasProperty[i] = properties->MethodHasProperty[j];
          properties->MethodProperties[i] = properties->MethodProperties[j];
        }
        return 0;
      }
    }
  }

  /* no matches */
  return 1;
}

/*-------------------------------------------------------------------
 * Add a property, using method at index i as a template */

static void addProperty(const HierarchyInfo* hinfo, ClassProperties* properties,
  ClassPropertyMethods* methods, int i, int matchedMethods[])
{
  MethodAttributes* meth = methods->Methods[i];
  PropertyInfo* property;
  unsigned int category;

  /* save the info about the method used to discover the property */
  matchedMethods[i] = 1;
  category = methodCategory(meth, 0);
  properties->MethodTypes[i] = category;
  properties->MethodHasProperty[i] = 1;
  properties->MethodProperties[i] = properties->NumberOfProperties;
  /* duplicate the info for all "repeat" methods */
  searchForRepeatedMethods(properties, methods, i);

  /* create the property */
  property = (PropertyInfo*)malloc(sizeof(PropertyInfo));
  initializePropertyInfo(property, meth, category);
  findAllMatches(hinfo, property, properties->NumberOfProperties, methods, matchedMethods,
    properties->MethodTypes, properties->MethodHasProperty, properties->MethodProperties);

  properties->Properties[properties->NumberOfProperties++] = property;
}

/*-------------------------------------------------------------------
 * This is the method that finds out everything that it can about
 * all properties that can be accessed by the methods of a class */

static void categorizeProperties(
  const HierarchyInfo* hinfo, ClassPropertyMethods* methods, ClassProperties* properties)
{
  int i, n;
  int* matchedMethods;

  properties->NumberOfProperties = 0;

  n = methods->NumberOfMethods;
  matchedMethods = (int*)malloc(sizeof(int) * n);
  for (i = 0; i < n; i++)
  {
    /* "matchedMethods" are methods removed from consideration */
    matchedMethods[i] = 0;
    if (!methods->Methods[i]->HasProperty || methods->Methods[i]->IsRepeat)
    {
      matchedMethods[i] = 1;
    }
  }

  /* start with the set methods */
  for (i = 0; i < n; i++)
  {
    /* all set methods except for SetValueToEnum() methods
     * and SetNumberOf() methods */
    if (!matchedMethods[i] && isSetMethod(methods->Methods[i]->Name) &&
      !methods->Methods[i]->IsEnumerated && !isSetNumberOfMethod(methods->Methods[i]->Name))
    {
      addProperty(hinfo, properties, methods, i, matchedMethods);
    }
  }

  /* next do the get methods that didn't have matching set methods */
  for (i = 0; i < n; i++)
  {
    /* all get methods except for GetValueAs() methods
     * and GetNumberOf() methods */
    if (!matchedMethods[i] && isGetMethod(methods->Methods[i]->Name) &&
      !isAsStringMethod(methods->Methods[i]->Name) &&
      !isGetNumberOfMethod(methods->Methods[i]->Name))
    {
      addProperty(hinfo, properties, methods, i, matchedMethods);
    }
  }

  /* sweep SetNumberOf() methods that didn't have
   * matching indexed Set methods */
  for (i = 0; i < n; i++)
  {
    if (!matchedMethods[i] && isSetNumberOfMethod(methods->Methods[i]->Name))
    {
      addProperty(hinfo, properties, methods, i, matchedMethods);
    }
  }

  /* sweep the GetNumberOf() methods that didn't have
   * matching indexed Get methods */
  for (i = 0; i < n; i++)
  {
    if (!matchedMethods[i] && isGetNumberOfMethod(methods->Methods[i]->Name))
    {
      addProperty(hinfo, properties, methods, i, matchedMethods);
    }
  }

  /* finally do the add methods */
  for (i = 0; i < n; i++)
  {
    /* all add methods */
    if (!matchedMethods[i] && isAddMethod(methods->Methods[i]->Name))
    {
      addProperty(hinfo, properties, methods, i, matchedMethods);
    }
  }

  free(matchedMethods);
}

/*-------------------------------------------------------------------
 * categorize methods that get/set/add/remove values */

static void categorizePropertyMethods(ClassInfo* data, ClassPropertyMethods* methods)
{
  int i, n;
  FunctionInfo* func;
  MethodAttributes* attrs;

  methods->NumberOfMethods = 0;

  /* build up the ClassPropertyMethods struct */
  n = data->NumberOfFunctions;
  for (i = 0; i < n; i++)
  {
    func = data->Functions[i];
    attrs = (MethodAttributes*)malloc(sizeof(MethodAttributes));
    methods->Methods[methods->NumberOfMethods++] = attrs;

    /* copy the func into a MethodAttributes struct if possible */
    if (getMethodAttributes(func, attrs))
    {
      /* check for repeats e.g. SetPoint(float *), SetPoint(double *) */
      searchForRepeatedMethods(0, methods, i);
    }
  }
}

/*-------------------------------------------------------------------
 * build a ClassProperties struct from the info in a FileInfo struct */

ClassProperties* vtkParseProperties_Create(ClassInfo* data, const HierarchyInfo* hinfo)
{
  int i;
  ClassProperties* properties;
  ClassPropertyMethods* methods;

  methods = (ClassPropertyMethods*)malloc(sizeof(ClassPropertyMethods));
  methods->Methods =
    (MethodAttributes**)malloc(sizeof(MethodAttributes*) * data->NumberOfFunctions);

  /* categorize the methods according to what properties they reference
   * and what they do to that property */
  categorizePropertyMethods(data, methods);

  properties = (ClassProperties*)malloc(sizeof(ClassProperties));
  properties->NumberOfProperties = 0;
  properties->NumberOfMethods = methods->NumberOfMethods;
  properties->Properties = (PropertyInfo**)malloc(sizeof(PropertyInfo*) * methods->NumberOfMethods);
  properties->MethodTypes = (unsigned int*)malloc(sizeof(unsigned int) * methods->NumberOfMethods);
  properties->MethodHasProperty = (int*)malloc(sizeof(int) * methods->NumberOfMethods);
  properties->MethodProperties = (int*)malloc(sizeof(int) * methods->NumberOfMethods);

  for (i = 0; i < methods->NumberOfMethods; i++)
  {
    properties->MethodTypes[i] = 0;
    properties->MethodHasProperty[i] = 0;
    properties->MethodProperties[i] = 0;
  }

  /* synthesize a list of properties from the list of methods */
  categorizeProperties(hinfo, methods, properties);

  for (i = 0; i < methods->NumberOfMethods; i++)
  {
    free(methods->Methods[i]);
  }

  free(methods->Methods);
  free(methods);

  return properties;
}

/*-------------------------------------------------------------------
 * free a class properties struct */

void vtkParseProperties_Free(ClassProperties* properties)
{
  int i, n;

  n = properties->NumberOfProperties;
  for (i = 0; i < n; i++)
  {
    free(properties->Properties[i]);
  }

  free(properties->Properties);
  free(properties->MethodTypes);
  free(properties->MethodHasProperty);
  free(properties->MethodProperties);
  free(properties);
}

/*-------------------------------------------------------------------
 * get a string representation of method bitfield value */

const char* vtkParseProperties_MethodTypeAsString(unsigned int methodType)
{
  switch (methodType)
  {
    case VTK_METHOD_SET_CLAMP:
      return "SET_CLAMP";
    case VTK_METHOD_SET_BOOL:
      return "SET_BOOL";
    case VTK_METHOD_GET:
      return "GET";
    case VTK_METHOD_SET:
      return "SET";
    case VTK_METHOD_GET_MULTI:
      return "GET_MULTI";
    case VTK_METHOD_SET_MULTI:
      return "SET_MULTI";
    case VTK_METHOD_GET_IDX:
      return "GET_IDX";
    case VTK_METHOD_SET_IDX:
      return "SET_IDX";
    case VTK_METHOD_GET_NTH:
      return "GET_NTH";
    case VTK_METHOD_SET_NTH:
      return "SET_NTH";
    case VTK_METHOD_GET_RHS:
      return "GET_RHS";
    case VTK_METHOD_GET_IDX_RHS:
      return "GET_IDX_RHS";
    case VTK_METHOD_GET_NTH_RHS:
      return "GET_NTH_RHS";
    case VTK_METHOD_GET_AS_STRING:
      return "GET_AS_STRING";
    case VTK_METHOD_SET_VALUE_TO:
      return "SET_VALUE_TO";
    case VTK_METHOD_BOOL_ON:
      return "BOOL_ON";
    case VTK_METHOD_BOOL_OFF:
      return "BOOL_OFF";
    case VTK_METHOD_GET_MIN_VALUE:
      return "GET_MIN_VALUE";
    case VTK_METHOD_GET_MAX_VALUE:
      return "GET_MAX_VALUE";
    case VTK_METHOD_GET_NUMBER_OF:
      return "GET_NUMBER_OF";
    case VTK_METHOD_SET_NUMBER_OF:
      return "SET_NUMBER_OF";
    case VTK_METHOD_ADD:
      return "ADD";
    case VTK_METHOD_ADD_MULTI:
      return "ADD_MULTI";
    case VTK_METHOD_ADD_IDX:
      return "ADD_IDX";
    case VTK_METHOD_REMOVE:
      return "REMOVE";
    case VTK_METHOD_REMOVE_IDX:
      return "REMOVE_IDX";
    case VTK_METHOD_REMOVE_ALL:
      return "REMOVE_ALL";
    case VTK_METHOD_ADD_NODISCARD:
      return "ADD_NODISCARD";
    case VTK_METHOD_REMOVE_NODISCARD:
      return "REMOVE_NODISCARD";
    default:
      return "UNKNOWN";
  }
}
