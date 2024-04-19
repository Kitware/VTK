// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkParse.h"
#include "vtkParseHierarchy.h"
#include "vtkParseMain.h"
#include "vtkParseSystem.h"
#include "vtkWrap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static HierarchyInfo* hierarchyInfo = NULL;
static StringCache* stringCache = NULL;
static int numberOfWrappedFunctions = 0;
static FunctionInfo* wrappedFunctions[1000];
static FunctionInfo* thisFunction;

void outputScalarParamDeclarations(FILE* fp, int i, unsigned int aType)
{
  /* ignore void */
  if (aType == VTK_PARSE_VOID)
  {
    return;
  }

  switch (aType & VTK_PARSE_BASE_TYPE)
  {
    case VTK_PARSE_SIGNED_CHAR:
    case VTK_PARSE_UNSIGNED_CHAR:
      fprintf(fp, "byte ");
      break;
    case VTK_PARSE_CHAR:
      fprintf(fp, "char ");
      break;
  }

  switch ((aType & VTK_PARSE_BASE_TYPE) & ~VTK_PARSE_UNSIGNED)
  {
    case VTK_PARSE_FLOAT:
      fprintf(fp, "float ");
      break;
    case VTK_PARSE_DOUBLE:
      fprintf(fp, "double ");
      break;
    case VTK_PARSE_INT:
      fprintf(fp, "int ");
      break;
    case VTK_PARSE_SHORT:
      fprintf(fp, "short ");
      break;
    case VTK_PARSE_LONG:
    case VTK_PARSE_LONG_LONG:
      fprintf(fp, "long ");
      break;
    case VTK_PARSE_BOOL:
      fprintf(fp, "boolean ");
      break;
    case VTK_PARSE_VOID:
      fprintf(fp, "void ");
      break;
    case VTK_PARSE_OBJECT:
      fprintf(fp, "%s ", thisFunction->ArgClasses[i]);
      break;
    case VTK_PARSE_UNKNOWN:
      fprintf(fp, "int ");
      break;
  }

  fprintf(fp, "id%i", i);
  if (((aType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER) && (aType != VTK_PARSE_CHAR_PTR) &&
    (aType != VTK_PARSE_OBJECT_PTR))
  {
    fprintf(fp, "[]");
  }
}

void return_result(FILE* fp)
{
  unsigned int rType = (thisFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);

  switch (rType)
  {
    case VTK_PARSE_FLOAT:
      fprintf(fp, "float ");
      break;
    case VTK_PARSE_VOID:
      fprintf(fp, "void ");
      break;
    case VTK_PARSE_CHAR:
      fprintf(fp, "char ");
      break;
    case VTK_PARSE_DOUBLE:
      fprintf(fp, "double ");
      break;
    case VTK_PARSE_SIGNED_CHAR:
    case VTK_PARSE_UNSIGNED_CHAR:
      fprintf(fp, "byte ");
      break;
    case VTK_PARSE_SHORT:
    case VTK_PARSE_UNSIGNED_SHORT:
      fprintf(fp, "short ");
      break;
    case VTK_PARSE_INT:
    case VTK_PARSE_UNSIGNED_INT:
      fprintf(fp, "int ");
      break;
    case VTK_PARSE_UNKNOWN:
      fprintf(fp, "int ");
      break;
    case VTK_PARSE_LONG:
    case VTK_PARSE_LONG_LONG:
    case VTK_PARSE_UNSIGNED_LONG:
    case VTK_PARSE_UNSIGNED_LONG_LONG:
      fprintf(fp, "long ");
      break;
    case VTK_PARSE_BOOL:
      fprintf(fp, "boolean ");
      break;
    case VTK_PARSE_CHAR_PTR:
    case VTK_PARSE_STRING:
    case VTK_PARSE_STRING_REF:
      fprintf(fp, "String ");
      break;
    case VTK_PARSE_OBJECT_PTR:
      fprintf(fp, "%s ", thisFunction->ReturnClass);
      break;

      /* handle functions returning vectors */
      /* this is done by looking them up in a hint file */
    case VTK_PARSE_FLOAT_PTR:
      fprintf(fp, "float[] ");
      break;
    case VTK_PARSE_DOUBLE_PTR:
      fprintf(fp, "double[] ");
      break;
    case VTK_PARSE_UNSIGNED_CHAR_PTR:
    case VTK_PARSE_SIGNED_CHAR_PTR:
      fprintf(fp, "byte[]  ");
      break;
    case VTK_PARSE_SHORT_PTR:
    case VTK_PARSE_UNSIGNED_SHORT_PTR:
      fprintf(fp, "short[] ");
      break;
    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_UNSIGNED_INT_PTR:
      fprintf(fp, "int[] ");
      break;
    case VTK_PARSE_LONG_PTR:
    case VTK_PARSE_LONG_LONG_PTR:
    case VTK_PARSE_UNSIGNED_LONG_PTR:
    case VTK_PARSE_UNSIGNED_LONG_LONG_PTR:
      fprintf(fp, "long[]  ");
      break;
    case VTK_PARSE_BOOL_PTR:
      fprintf(fp, "boolean[]  ");
      break;
  }
}

/* same as return_result except we return a long (the c++ pointer) rather than an object */
void return_result_native(FILE* fp)
{
  unsigned int rType = (thisFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);

  switch (rType)
  {
    case VTK_PARSE_FLOAT:
      fprintf(fp, "float ");
      break;
    case VTK_PARSE_VOID:
      fprintf(fp, "void ");
      break;
    case VTK_PARSE_CHAR:
      fprintf(fp, "char ");
      break;
    case VTK_PARSE_DOUBLE:
      fprintf(fp, "double ");
      break;
    case VTK_PARSE_SIGNED_CHAR:
    case VTK_PARSE_UNSIGNED_CHAR:
      fprintf(fp, "byte ");
      break;
    case VTK_PARSE_SHORT:
    case VTK_PARSE_UNSIGNED_SHORT:
      fprintf(fp, "short ");
      break;
    case VTK_PARSE_INT:
    case VTK_PARSE_UNSIGNED_INT:
      fprintf(fp, "int ");
      break;
    case VTK_PARSE_UNKNOWN:
      fprintf(fp, "int ");
      break;
    case VTK_PARSE_LONG:
    case VTK_PARSE_LONG_LONG:
    case VTK_PARSE_UNSIGNED_LONG:
    case VTK_PARSE_UNSIGNED_LONG_LONG:
      fprintf(fp, "long ");
      break;
    case VTK_PARSE_BOOL:
      fprintf(fp, "boolean ");
      break;
    case VTK_PARSE_CHAR_PTR:
    case VTK_PARSE_STRING:
    case VTK_PARSE_STRING_REF:
      fprintf(fp, "byte[] ");
      break;
    case VTK_PARSE_OBJECT_PTR:
      fprintf(fp, "long ");
      break;

      /* handle functions returning vectors */
      /* this is done by looking them up in a hint file */
    case VTK_PARSE_BOOL_PTR:
      fprintf(fp, "boolean[]  ");
      break;
    case VTK_PARSE_FLOAT_PTR:
      fprintf(fp, "float[] ");
      break;
    case VTK_PARSE_DOUBLE_PTR:
      fprintf(fp, "double[] ");
      break;
    case VTK_PARSE_SIGNED_CHAR_PTR:
    case VTK_PARSE_UNSIGNED_CHAR_PTR:
      fprintf(fp, "byte[]  ");
      break;
    case VTK_PARSE_SHORT_PTR:
    case VTK_PARSE_UNSIGNED_SHORT_PTR:
      fprintf(fp, "short[]  ");
      break;
    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_UNSIGNED_INT_PTR:
      fprintf(fp, "int[] ");
      break;
    case VTK_PARSE_LONG_PTR:
    case VTK_PARSE_LONG_LONG_PTR:
    case VTK_PARSE_UNSIGNED_LONG_PTR:
    case VTK_PARSE_UNSIGNED_LONG_LONG_PTR:
      fprintf(fp, "long[]  ");
      break;
  }
}

/* Check to see if two types will map to the same Java type,
 * return 1 if type1 should take precedence,
 * return 2 if type2 should take precedence,
 * return 0 if the types do not map to the same type */
static int CheckMatch(unsigned int type1, unsigned int type2, const char* c1, const char* c2)
{
  /* VTK_PARSE_UNKNOWN is used for enum types, which are mapped to java's int type */
  static unsigned int byteTypes[] = { VTK_PARSE_UNSIGNED_CHAR, VTK_PARSE_SIGNED_CHAR, 0 };
  static unsigned int shortTypes[] = { VTK_PARSE_UNSIGNED_SHORT, VTK_PARSE_SHORT, 0 };
  static unsigned int intTypes[] = { VTK_PARSE_UNKNOWN, VTK_PARSE_UNSIGNED_INT, VTK_PARSE_INT, 0 };
  static unsigned int longTypes[] = { VTK_PARSE_UNSIGNED_LONG, VTK_PARSE_UNSIGNED_LONG_LONG,
    VTK_PARSE_LONG, VTK_PARSE_LONG_LONG, 0 };

  static unsigned int stringTypes[] = { VTK_PARSE_CHAR_PTR, VTK_PARSE_STRING_REF, VTK_PARSE_STRING,
    0 };

  static unsigned int* numericTypes[] = { byteTypes, shortTypes, intTypes, longTypes, 0 };

  int i, j;
  int hit1, hit2;

  if ((type1 & VTK_PARSE_UNQUALIFIED_TYPE) == (type2 & VTK_PARSE_UNQUALIFIED_TYPE))
  {
    if ((type1 & VTK_PARSE_BASE_TYPE) == VTK_PARSE_OBJECT)
    {
      if (strcmp(c1, c2) == 0)
      {
        return 1;
      }
      return 0;
    }
    else
    {
      return 1;
    }
  }

  for (i = 0; numericTypes[i]; i++)
  {
    hit1 = 0;
    hit2 = 0;
    for (j = 0; numericTypes[i][j]; j++)
    {
      if ((type1 & VTK_PARSE_BASE_TYPE) == numericTypes[i][j])
      {
        hit1 = j + 1;
      }
      if ((type2 & VTK_PARSE_BASE_TYPE) == numericTypes[i][j])
      {
        hit2 = j + 1;
      }
    }
    if (hit1 && hit2 && (type1 & VTK_PARSE_INDIRECT) == (type2 & VTK_PARSE_INDIRECT))
    {
      if (hit1 < hit2)
      {
        return 1;
      }
      else
      {
        return 2;
      }
    }
  }

  hit1 = 0;
  hit2 = 0;
  for (j = 0; stringTypes[j]; j++)
  {
    if ((type1 & VTK_PARSE_UNQUALIFIED_TYPE) == stringTypes[j])
    {
      hit1 = j + 1;
    }
    if ((type2 & VTK_PARSE_UNQUALIFIED_TYPE) == stringTypes[j])
    {
      hit2 = j + 1;
    }
  }
  if (hit1 && hit2)
  {
    if (hit1 < hit2)
    {
      return 1;
    }
    else
    {
      return 2;
    }
  }

  return 0;
}

/* have we done one of these yet */
int DoneOne(void)
{
  int i, j;
  int match;
  const FunctionInfo* fi;

  for (i = 0; i < numberOfWrappedFunctions; i++)
  {
    fi = wrappedFunctions[i];

    if ((!strcmp(fi->Name, thisFunction->Name)) &&
      (fi->NumberOfArguments == thisFunction->NumberOfArguments))
    {
      match = 1;
      for (j = 0; j < fi->NumberOfArguments; j++)
      {
        if (!CheckMatch(thisFunction->ArgTypes[j], fi->ArgTypes[j], thisFunction->ArgClasses[j],
              fi->ArgClasses[j]))
        {
          match = 0;
        }
      }
      if (!CheckMatch(
            thisFunction->ReturnType, fi->ReturnType, thisFunction->ReturnClass, fi->ReturnClass))
      {
        match = 0;
      }
      if (match)
        return 1;
    }
  }
  return 0;
}

void HandleDataReader(FILE* fp)
{
  fprintf(fp, "\n  private native void ");
  fprintf(fp, "%s_%i(byte id0[],int id1);\n", thisFunction->Name, numberOfWrappedFunctions);
  fprintf(fp, "\n  public void ");
  fprintf(fp, "%s(byte id0[],int id1)\n", thisFunction->Name);
  fprintf(fp, "    { %s_%i(id0,id1); }\n", thisFunction->Name, numberOfWrappedFunctions);
}

void HandleDataArray(FILE* fp, const ClassInfo* data)
{
  const char* type = 0;

  if (!strcmp("vtkCharArray", data->Name))
  {
    type = "char";
  }
  else if (!strcmp("vtkDoubleArray", data->Name))
  {
    type = "double";
  }
  else if (!strcmp("vtkFloatArray", data->Name))
  {
    type = "float";
  }
  else if (!strcmp("vtkIntArray", data->Name))
  {
    type = "int";
  }
  else if (!strcmp("vtkLongArray", data->Name))
  {
    type = "long";
  }
  else if (!strcmp("vtkShortArray", data->Name))
  {
    type = "short";
  }
  else if (!strcmp("vtkUnsignedCharArray", data->Name))
  {
    type = "byte";
  }
  else if (!strcmp("vtkUnsignedIntArray", data->Name))
  {
    type = "int";
  }
  else if (!strcmp("vtkUnsignedLongArray", data->Name))
  {
    type = "long";
  }
  else if (!strcmp("vtkUnsignedShortArray", data->Name))
  {
    type = "short";
  }
  else
  {
    return;
  }

  fprintf(fp, "\n");
  fprintf(fp, "  private native %s[] GetJavaArray_0();\n", type);
  fprintf(fp, "  public %s[] GetJavaArray()\n", type);
  fprintf(fp, "  {\n");
  fprintf(fp, "    return GetJavaArray_0();\n");
  fprintf(fp, "  }\n\n");
  fprintf(fp, "  private native void SetJavaArray_0(%s[] arr, int length);\n", type);
  fprintf(fp, "  public void SetJavaArray(%s[] arr)\n", type);
  fprintf(fp, "  {\n");
  fprintf(fp, "    SetJavaArray_0(arr,arr.length);\n");
  fprintf(fp, "  }\n");
}

static int isClassWrapped(const char* classname)
{
  const HierarchyEntry* entry;

  if (hierarchyInfo)
  {
    entry = vtkParseHierarchy_FindEntry(hierarchyInfo, classname);

    if (entry == 0 || vtkParseHierarchy_GetProperty(entry, "WRAPEXCLUDE") ||
      !vtkParseHierarchy_IsTypeOf(hierarchyInfo, entry, "vtkObjectBase"))
    {
      return 0;
    }

    /* Templated classes are not wrapped in Java */
    if (strchr(classname, '<'))
    {
      return 0;
    }

    /* Only the primary class in the header is wrapped in Java */
    return vtkParseHierarchy_IsPrimary(entry);
  }

  return 1;
}

int checkFunctionSignature(ClassInfo* data)
{
  static const unsigned int supported_types[] = { VTK_PARSE_VOID, VTK_PARSE_BOOL, VTK_PARSE_FLOAT,
    VTK_PARSE_DOUBLE, VTK_PARSE_CHAR, VTK_PARSE_UNSIGNED_CHAR, VTK_PARSE_SIGNED_CHAR, VTK_PARSE_INT,
    VTK_PARSE_UNSIGNED_INT, VTK_PARSE_SHORT, VTK_PARSE_UNSIGNED_SHORT, VTK_PARSE_LONG,
    VTK_PARSE_UNSIGNED_LONG, VTK_PARSE_LONG_LONG, VTK_PARSE_UNSIGNED_LONG_LONG, VTK_PARSE_OBJECT,
    VTK_PARSE_STRING, VTK_PARSE_UNKNOWN, 0 };

  int i, j;
  int args_ok = 1;
  unsigned int rType = (thisFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);
  unsigned int aType = 0;
  unsigned int baseType = 0;

  /* some functions will not get wrapped no matter what else */
  if (thisFunction->IsOperator || thisFunction->ArrayFailure || thisFunction->Template ||
    thisFunction->IsExcluded || thisFunction->IsDeleted || !thisFunction->IsPublic ||
    !thisFunction->Name)
  {
    return 0;
  }

  /* NewInstance and SafeDownCast can not be wrapped because it is a
     (non-virtual) method which returns a pointer of the same type as
     the current pointer. Since all methods are virtual in Java, this
     looks like polymorphic return type.  */
  if (!strcmp("NewInstance", thisFunction->Name))
  {
    return 0;
  }

  if (!strcmp("SafeDownCast", thisFunction->Name))
  {
    return 0;
  }

  /* The GetInput() in vtkMapper cannot be overridden with a
   * different return type, Java doesn't allow this */
  if (strcmp(data->Name, "vtkMapper") == 0 && strcmp(thisFunction->Name, "GetInput") == 0)
  {
    return 0;
  }

  /* function pointer arguments for callbacks */
  if (thisFunction->NumberOfArguments == 2 && thisFunction->ArgTypes[0] == VTK_PARSE_FUNCTION &&
    thisFunction->ArgTypes[1] == VTK_PARSE_VOID_PTR && rType == VTK_PARSE_VOID)
  {
    return 1;
  }

  /* check to see if we can handle the args */
  for (i = 0; i < thisFunction->NumberOfArguments; i++)
  {
    aType = (thisFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);
    baseType = (aType & VTK_PARSE_BASE_TYPE);

    for (j = 0; supported_types[j] != 0; j++)
    {
      if (baseType == supported_types[j])
      {
        break;
      }
    }
    if (supported_types[j] == 0)
    {
      args_ok = 0;
    }

    if (baseType == VTK_PARSE_UNKNOWN)
    {
      const char* qualified_name = 0;
      if ((aType & VTK_PARSE_INDIRECT) == 0)
      {
        qualified_name = vtkParseHierarchy_QualifiedEnumName(
          hierarchyInfo, data, stringCache, thisFunction->ArgClasses[i]);
      }
      if (qualified_name)
      {
        thisFunction->ArgClasses[i] = qualified_name;
      }
      else
      {
        args_ok = 0;
      }
    }

    if (baseType == VTK_PARSE_OBJECT)
    {
      if ((aType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER)
      {
        args_ok = 0;
      }
      else if (!isClassWrapped(thisFunction->ArgClasses[i]))
      {
        args_ok = 0;
      }
    }

    if (aType == VTK_PARSE_OBJECT)
      args_ok = 0;
    if (((aType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) &&
      ((aType & VTK_PARSE_INDIRECT) != 0) && (aType != VTK_PARSE_STRING_REF))
      args_ok = 0;
    if (aType == VTK_PARSE_STRING_PTR)
      args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_CHAR_PTR)
      args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_INT_PTR)
      args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_SHORT_PTR)
      args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_LONG_PTR)
      args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_LONG_LONG_PTR)
      args_ok = 0;
  }

  baseType = (rType & VTK_PARSE_BASE_TYPE);

  for (j = 0; supported_types[j] != 0; j++)
  {
    if (baseType == supported_types[j])
    {
      break;
    }
  }
  if (supported_types[j] == 0)
  {
    args_ok = 0;
  }

  if (baseType == VTK_PARSE_UNKNOWN)
  {
    const char* qualified_name = 0;
    if ((rType & VTK_PARSE_INDIRECT) == 0)
    {
      qualified_name = vtkParseHierarchy_QualifiedEnumName(
        hierarchyInfo, data, stringCache, thisFunction->ReturnClass);
    }
    if (qualified_name)
    {
      thisFunction->ReturnClass = qualified_name;
    }
    else
    {
      args_ok = 0;
    }
  }

  if (baseType == VTK_PARSE_OBJECT)
  {
    if ((rType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER)
    {
      args_ok = 0;
    }
    else if (!isClassWrapped(thisFunction->ReturnClass))
    {
      args_ok = 0;
    }
  }

  if (((rType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) && ((rType & VTK_PARSE_INDIRECT) != 0) &&
    (rType != VTK_PARSE_STRING_REF))
    args_ok = 0;
  if (rType == VTK_PARSE_STRING_PTR)
    args_ok = 0;

  /* eliminate unsigned char/short/int/long/int64 pointers */
  if (rType == VTK_PARSE_UNSIGNED_CHAR_PTR)
    args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_INT_PTR)
    args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_SHORT_PTR)
    args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_LONG_PTR)
    args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_LONG_LONG_PTR)
    args_ok = 0;

  /* make sure we have all the info we need for array arguments in */
  for (i = 0; i < thisFunction->NumberOfArguments; i++)
  {
    aType = (thisFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

    if (((aType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER) && (thisFunction->ArgCounts[i] <= 0) &&
      (aType != VTK_PARSE_OBJECT_PTR) && (aType != VTK_PARSE_CHAR_PTR))
      args_ok = 0;
  }

  /* if we need a return type hint make sure we have one */
  switch (rType)
  {
    case VTK_PARSE_FLOAT_PTR:
    case VTK_PARSE_VOID_PTR:
    case VTK_PARSE_DOUBLE_PTR:
    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_SHORT_PTR:
    case VTK_PARSE_LONG_PTR:
    case VTK_PARSE_LONG_LONG_PTR:
    case VTK_PARSE_SIGNED_CHAR_PTR:
    case VTK_PARSE_BOOL_PTR:
    case VTK_PARSE_UNSIGNED_CHAR_PTR:
      args_ok = thisFunction->HaveHint;
      break;
  }

  /* make sure there isn't a Java-specific override */
  if (!strcmp("vtkObject", data->Name))
  {
    /* remove the original vtkCommand observer methods */
    if (!strcmp(thisFunction->Name, "AddObserver") || !strcmp(thisFunction->Name, "GetCommand") ||
      (!strcmp(thisFunction->Name, "RemoveObserver") &&
        (thisFunction->ArgTypes[0] != VTK_PARSE_UNSIGNED_LONG)) ||
      ((!strcmp(thisFunction->Name, "RemoveObservers") ||
         !strcmp(thisFunction->Name, "HasObserver")) &&
        (((thisFunction->ArgTypes[0] != VTK_PARSE_UNSIGNED_LONG) &&
           (thisFunction->ArgTypes[0] != (VTK_PARSE_CHAR_PTR | VTK_PARSE_CONST))) ||
          (thisFunction->NumberOfArguments > 1))) ||
      (!strcmp(thisFunction->Name, "RemoveAllObservers") && (thisFunction->NumberOfArguments > 0)))
    {
      args_ok = 0;
    }
  }
  else if (!strcmp("vtkObjectBase", data->Name))
  {
    /* remove the special vtkObjectBase methods */
    if (!strcmp(thisFunction->Name, "Print"))
    {
      args_ok = 0;
    }
  }

  /* make sure it isn't a Delete or New function */
  if (!strcmp("Delete", thisFunction->Name) || !strcmp("New", thisFunction->Name))
  {
    args_ok = 0;
  }

  return args_ok;
}

void outputParamDeclarationsNative(FILE* fp)
{
  int i;
  unsigned int type;

  for (i = 0; i < thisFunction->NumberOfArguments; i++)
  {
    if (thisFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
    {
      fprintf(fp, " Object id0, byte[] id1, int len1");
      /* ignore args after function pointer */
      break;
    }
    else
    {
      if (i)
      {
        fprintf(fp, ",");
      }

      type = (thisFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);
      switch (type)
      {
        case VTK_PARSE_CHAR_PTR:
        case VTK_PARSE_STRING:
        case VTK_PARSE_STRING_REF:
          fprintf(fp, "byte[] id%i, int len%i", i, i);
          break;
        default:
          outputScalarParamDeclarations(fp, i, type);
          break;
      }
    }
  }
}

void outputParamDeclarations(FILE* fp)
{
  int i;
  unsigned int type;

  for (i = 0; i < thisFunction->NumberOfArguments; i++)
  {
    if (thisFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
    {
      fprintf(fp, " Object id0, String id1");
      /* ignore args after function pointer */
      break;
    }
    else
    {
      if (i)
      {
        fprintf(fp, ",");
      }

      type = (thisFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);
      switch (type)
      {
        case VTK_PARSE_CHAR_PTR:
        case VTK_PARSE_STRING:
        case VTK_PARSE_STRING_REF:
          fprintf(fp, "String id%i", i);
          break;
        default:
          outputScalarParamDeclarations(fp, i, type);
          break;
      }
    }
  }
}

void outputFunctionParams(FILE* fp)
{
  int i;
  unsigned int type;

  for (i = 0; i < thisFunction->NumberOfArguments; i++)
  {
    if (thisFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
    {
      fprintf(fp, "id0, bytes1, bytes1.length");
      /* ignore args after function pointer */
      break;
    }

    if (i)
    {
      fprintf(fp, ",");
    }

    type = (thisFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);
    switch (type)
    {
      case VTK_PARSE_CHAR_PTR:
      case VTK_PARSE_STRING:
      case VTK_PARSE_STRING_REF:
        fprintf(fp, "bytes%i, bytes%i.length", i, i);
        break;
      default:
        fprintf(fp, "id%i", i);
        break;
    }
  }
}

void outputStringConversionVariables(FILE* fp)
{
  int i;
  unsigned int type;

  /* output local variables which convert string args to byte arrays */
  for (i = 0; i < thisFunction->NumberOfArguments; i++)
  {
    if (thisFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
    {
      fprintf(fp, "    byte[] bytes1 = id1.getBytes(StandardCharsets.UTF_8);\n");
      /* ignore args after function pointer */
      break;
    }

    type = (thisFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);
    switch (type)
    {
      case VTK_PARSE_CHAR_PTR:
      case VTK_PARSE_STRING:
      case VTK_PARSE_STRING_REF:
        fprintf(fp, "    byte[] bytes%i = id%i.getBytes(StandardCharsets.UTF_8);\n", i, i);
        break;
    }
  }
}

void outputFunction(FILE* fp, ClassInfo* data)
{
  unsigned int rType = (thisFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);
  int args_ok = checkFunctionSignature(data);

  /* handle DataReader SetBinaryInputString as a special case */
  if (!strcmp("SetBinaryInputString", thisFunction->Name) &&
    (!strcmp("vtkDataReader", data->Name) || !strcmp("vtkStructuredGridReader", data->Name) ||
      !strcmp("vtkRectilinearGridReader", data->Name) ||
      !strcmp("vtkUnstructuredGridReader", data->Name) ||
      !strcmp("vtkStructuredPointsReader", data->Name) || !strcmp("vtkPolyDataReader", data->Name)))
  {
    HandleDataReader(fp);
    wrappedFunctions[numberOfWrappedFunctions] = thisFunction;
    numberOfWrappedFunctions++;
  }

  if (!thisFunction->IsExcluded && thisFunction->IsPublic && args_ok &&
    strcmp(data->Name, thisFunction->Name) != 0 && strcmp(data->Name, thisFunction->Name + 1) != 0)
  {
    /* make sure we haven't already done one of these */
    if (!DoneOne())
    {
      fprintf(fp, "\n  private native ");
      return_result_native(fp);
      fprintf(fp, "%s_%i(", thisFunction->Name, numberOfWrappedFunctions);
      outputParamDeclarationsNative(fp);
      fprintf(fp, ");\n");
      fprintf(fp, "  public ");
      return_result(fp);
      fprintf(fp, "%s(", thisFunction->Name);
      outputParamDeclarations(fp);
      fprintf(fp, ")\n  {\n");
      outputStringConversionVariables(fp);
      /* if returning object, lookup in global hash */
      if (rType == VTK_PARSE_OBJECT_PTR)
      {
        fprintf(fp, "    long temp = %s_%i(", thisFunction->Name, numberOfWrappedFunctions);
        outputFunctionParams(fp);
        fprintf(fp, ");\n");
        fprintf(fp, "\n    if (temp == 0) return null;");
        fprintf(fp, "\n    return (%s)vtkObjectBase.JAVA_OBJECT_MANAGER.getJavaObject(temp);",
          thisFunction->ReturnClass);
      }
      else
      {
        /* if not void then need return otherwise none */
        fprintf(fp, "    ");
        if (rType != VTK_PARSE_VOID)
        {
          fprintf(fp, "return ");
          /* convert byte array result into utf8 string */
          switch (rType)
          {
            case VTK_PARSE_CHAR_PTR:
            case VTK_PARSE_STRING:
            case VTK_PARSE_STRING_REF:
              fprintf(fp, "new String(");
              break;
          }
        }
        fprintf(fp, "%s_%i(", thisFunction->Name, numberOfWrappedFunctions);
        outputFunctionParams(fp);

        if (rType != VTK_PARSE_VOID)
        {
          /* convert byte array result into utf8 string */
          switch (rType)
          {
            case VTK_PARSE_CHAR_PTR:
            case VTK_PARSE_STRING:
            case VTK_PARSE_STRING_REF:
              fprintf(fp, "), StandardCharsets.UTF_8");
              break;
          }
        }

        fprintf(fp, ");");
      }
      fprintf(fp, "\n  }\n");

      wrappedFunctions[numberOfWrappedFunctions] = thisFunction;
      numberOfWrappedFunctions++;
    }
  }
}

static void WriteDummyClass(FILE* fp, const ClassInfo* data, const char* filename)
{
  char* class_name = NULL;
  if (data == NULL)
  {
    const char* last_slash = strrchr(filename, '/');
    const char* first_dot = strchr(last_slash, '.');
    size_t size = first_dot - last_slash;
    class_name = malloc(size * sizeof(char));
    strncpy(class_name, last_slash + 1, size);
    class_name[size - 1] = '\0';
  }
  else
  {
    class_name = strdup(data->Name);
  }
  fprintf(fp, "package vtk;\n\nclass %s {\n}\n", class_name);
  free(class_name);
}

/* print the parsed structures */
int VTK_PARSE_MAIN(int argc, char* argv[])
{
  const OptionInfo* options;
  FileInfo* file_info;
  ClassInfo* data;
  FILE* fp;
  int i;

  /* pre-define a macro to identify the language */
  vtkParse_DefineMacro("__VTK_WRAP_JAVA__", 0);

  /* get command-line args and parse the header file */
  file_info = vtkParse_Main(argc, argv);

  /* some utility functions require the string cache */
  stringCache = file_info->Strings;

  /* get the command-line options */
  options = vtkParse_GetCommandLineOptions();

  /* get the hierarchy info for accurate typing */
  if (options->HierarchyFileNames)
  {
    hierarchyInfo =
      vtkParseHierarchy_ReadFiles(options->NumberOfHierarchyFileNames, options->HierarchyFileNames);
  }

  /* get the output file */
  fp = vtkParse_FileOpen(options->OutputFileName, "w");

  if (!fp)
  {
    fprintf(stderr, "Error opening output file %s\n", options->OutputFileName);
    return vtkParse_FinalizeMain(1);
  }

  /* get the main class */
  data = file_info->MainClass;
  if (data == NULL || data->IsExcluded)
  {
    WriteDummyClass(fp, data, options->OutputFileName);
    fclose(fp);
    vtkWrap_WarnEmpty(options);
    return vtkParse_FinalizeMain(0);
  }

  if (data->Template)
  {
    WriteDummyClass(fp, data, options->OutputFileName);
    fclose(fp);
    vtkWrap_WarnEmpty(options);
    return vtkParse_FinalizeMain(0);
  }

  for (i = 0; i < data->NumberOfSuperClasses; ++i)
  {
    if (strchr(data->SuperClasses[i], '<'))
    {
      WriteDummyClass(fp, data, options->OutputFileName);
      fclose(fp);
      vtkWrap_WarnEmpty(options);
      return vtkParse_FinalizeMain(0);
    }
  }

  if (hierarchyInfo)
  {
    if (!vtkWrap_IsTypeOf(hierarchyInfo, data->Name, "vtkObjectBase"))
    {
      WriteDummyClass(fp, data, options->OutputFileName);
      fclose(fp);
      vtkWrap_WarnEmpty(options);
      return vtkParse_FinalizeMain(0);
    }

    /* resolve using declarations within the header files */
    vtkWrap_ApplyUsingDeclarations(data, file_info, hierarchyInfo);

    /* expand typedefs */
    vtkWrap_ExpandTypedefs(data, file_info, hierarchyInfo);
  }

  fprintf(fp, "// java wrapper for %s object\n//\n\n", data->Name);
  fprintf(fp, "package vtk;\n");

  if (strcmp("vtkObjectBase", data->Name) != 0)
  {
    fprintf(fp, "import vtk.*;\n");
  }
  fprintf(fp, "import java.nio.charset.*;\n\n");
  fprintf(fp, "\npublic class %s", data->Name);
  if (strcmp("vtkObjectBase", data->Name) != 0)
  {
    if (data->NumberOfSuperClasses)
    {
      fprintf(fp, " extends %s", data->SuperClasses[0]);
    }
  }
  fprintf(fp, "\n{\n");

  /* insert function handling code here */
  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    thisFunction = data->Functions[i];
    outputFunction(fp, data);
  }

  HandleDataArray(fp, data);

  if (!data->NumberOfSuperClasses)
  {
    if (strcmp("vtkObjectBase", data->Name) == 0)
    {
      fprintf(fp,
        "\n  public static vtk.vtkJavaMemoryManager JAVA_OBJECT_MANAGER = new "
        "vtk.vtkJavaMemoryManagerImpl();");
    }
    if (!data->IsAbstract)
    {
      fprintf(fp, "\n  public %s() {", data->Name);
      fprintf(fp, "\n    this.vtkId = this.VTKInit();");
      fprintf(fp, "\n    vtkObjectBase.JAVA_OBJECT_MANAGER.registerJavaObject(this.vtkId, this);");
      fprintf(fp, "\n}\n");
    }
    else
    {
      fprintf(fp, "\n  public %s() { super(); }\n", data->Name);
    }
    fprintf(fp, "\n  public %s(long id) {", data->Name);
    fprintf(fp, "\n    super();");
    fprintf(fp, "\n    this.vtkId = id;");
    fprintf(fp, "\n    this.VTKRegister();");
    fprintf(fp, "\n    vtkObjectBase.JAVA_OBJECT_MANAGER.registerJavaObject(this.vtkId, this);");
    fprintf(fp, "\n}\n");
    fprintf(fp, "\n  protected long vtkId;\n");
    fprintf(fp, "\n  public long GetVTKId() { return this.vtkId; }");

    /* if we are a base class and have a delete method */
    if (data->HasDelete)
    {
      fprintf(fp, "\n");
      fprintf(fp, "  public static native void VTKDeleteReference(long id);\n");
      fprintf(fp, "  private static native byte[] VTKGetClassNameBytesFromReference(long id);\n");
      fprintf(fp, "  public static String VTKGetClassNameFromReference(long id)\n");
      fprintf(fp, "  {\n");
      fprintf(fp,
        "    return new String(VTKGetClassNameBytesFromReference(id),StandardCharsets.UTF_8);\n");
      fprintf(fp, "  }\n");
      fprintf(fp, "  protected native void VTKDelete();\n");
      fprintf(fp, "  protected native void VTKRegister();\n");
      fprintf(fp, "  public void Delete()\n");
      fprintf(fp, "  {\n");
      fprintf(fp, "    vtkObjectBase.JAVA_OBJECT_MANAGER.unRegisterJavaObject(this.vtkId);\n");
      fprintf(fp, "    this.vtkId = 0;\n");
      fprintf(fp, "  }\n");
    }
  }
  else
  {
    fprintf(fp, "\n  public %s() { super(); }\n", data->Name);
    fprintf(fp, "\n  public %s(long id) { super(id); }\n", data->Name);
  }

  if (!data->IsAbstract)
  {
    fprintf(fp, "  public native long   VTKInit();\n");
  }

  /* fprintf(fp,"  protected native void   VTKCastInit();\n"); */

  if (!strcmp("vtkObjectBase", data->Name))
  {
    /* Add the Print method to vtkObjectBase. */
    fprintf(fp, "\n");
    fprintf(fp, "  private native byte[] PrintBytes();\n");
    fprintf(fp, "  public String Print()\n");
    fprintf(fp, "  {\n");
    fprintf(fp, "    return new String(PrintBytes(),StandardCharsets.UTF_8);\n");
    fprintf(fp, "  }\n");
    /* Add the default toString from java object */
    fprintf(fp, "  public String toString() { return Print(); }\n");
  }

  if (!strcmp("vtkObject", data->Name))
  {
    fprintf(fp, "\n");
    fprintf(fp,
      "  private native int AddObserver(byte[] id0, int len0, Object id1, byte[] id2, int "
      "len2);\n");
    fprintf(fp, "  public int AddObserver(String id0, Object id1, String id2)\n");
    fprintf(fp, "  {\n");
    fprintf(fp, "    byte[] bytes0 = id0.getBytes(StandardCharsets.UTF_8);\n");
    fprintf(fp, "    byte[] bytes2 = id2.getBytes(StandardCharsets.UTF_8);\n");
    fprintf(fp, "    return AddObserver(bytes0, bytes0.length, id1, bytes2, bytes2.length);\n");
    fprintf(fp, "  }\n");
  }
  fprintf(fp, "\n}\n");
  fclose(fp);
  {
    size_t cc;
    size_t len;
    char* dir;
    const char* fname;
    const char javaDone[] = "VTKJavaWrapped";
    FILE* tfp;
    fname = options->OutputFileName;
    size_t dirlen = strlen(fname) + strlen(javaDone) + 2;
    dir = (char*)malloc(dirlen);
    snprintf(dir, dirlen, "%s", fname);
    len = strlen(dir);
    for (cc = len - 1; cc > 0; cc--)
    {
      if (dir[cc] == '/' || dir[cc] == '\\')
      {
        dir[cc + 1] = 0;
        break;
      }
    }
    strcat(dir, javaDone);
    tfp = vtkParse_FileOpen(dir, "w");
    if (tfp)
    {
      fprintf(tfp, "File: %s\n", fname);
      fclose(tfp);
    }
    free(dir);
  }

  if (hierarchyInfo)
  {
    vtkParseHierarchy_Free(hierarchyInfo);
  }

  vtkParse_Free(file_info);

  return vtkParse_FinalizeMain(0);
}
