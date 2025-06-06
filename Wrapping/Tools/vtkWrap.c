// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWrap.h"
#include "vtkParseData.h"
#include "vtkParseExtras.h"
#include "vtkParseMain.h"
#include "vtkParseMerge.h"
#include "vtkParseProperties.h"
#include "vtkParseString.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------- */
/* Common types. */

int vtkWrap_IsVoid(const ValueInfo* val)
{
  if (val == 0)
  {
    return 1;
  }

  return ((val->Type & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VOID);
}

int vtkWrap_IsVoidFunction(const ValueInfo* val)
{
  unsigned int t = (val->Type & VTK_PARSE_UNQUALIFIED_TYPE);

  if (t == VTK_PARSE_FUNCTION_PTR || t == VTK_PARSE_FUNCTION)
  {
    /* check for signature "void (*func)(void *)" */
    if (val->Function->NumberOfParameters == 1 &&
      val->Function->Parameters[0]->Type == VTK_PARSE_VOID_PTR &&
      val->Function->Parameters[0]->NumberOfDimensions == 0 &&
      val->Function->ReturnValue->Type == VTK_PARSE_VOID)
    {
      return 1;
    }
  }

  return 0;
}

int vtkWrap_IsVoidPointer(const ValueInfo* val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  return (t == VTK_PARSE_VOID && vtkWrap_IsPointer(val));
}

int vtkWrap_IsCharPointer(const ValueInfo* val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  return (
    t == VTK_PARSE_CHAR && vtkWrap_IsPointer(val) && (val->Attributes & VTK_PARSE_ZEROCOPY) == 0);
}

int vtkWrap_IsPODPointer(const ValueInfo* val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  return (t != VTK_PARSE_CHAR && vtkWrap_IsNumeric(val) && vtkWrap_IsPointer(val) &&
    (val->Attributes & VTK_PARSE_ZEROCOPY) == 0);
}

int vtkWrap_IsZeroCopyPointer(const ValueInfo* val)
{
  return (vtkWrap_IsPointer(val) && (val->Attributes & VTK_PARSE_ZEROCOPY) != 0);
}

int vtkWrap_IsArrayRef(const ValueInfo* val)
{
  return (vtkWrap_IsRef(val) && val->NumberOfDimensions > 0);
}

int vtkWrap_IsStdVector(const ValueInfo* val)
{
  return ((val->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_UNKNOWN && val->Class &&
    strncmp(val->Class, "std::vector<", 12) == 0);
}

int vtkWrap_IsStdMap(const ValueInfo* val)
{
  return ((val->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_UNKNOWN && val->Class &&
    strncmp(val->Class, "std::map<", 9) == 0);
}

int vtkWrap_IsStdUnorderedMap(const ValueInfo* val)
{
  return ((val->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_UNKNOWN && val->Class &&
    strncmp(val->Class, "std::unordered_map<", 19) == 0);
}

int vtkWrap_IsVTKObject(const ValueInfo* val)
{
  unsigned int t = (val->Type & VTK_PARSE_UNQUALIFIED_TYPE);
  return ((t == VTK_PARSE_UNKNOWN_PTR || t == VTK_PARSE_OBJECT_PTR || t == VTK_PARSE_QOBJECT_PTR) &&
    !val->IsEnum);
}

int vtkWrap_IsVTKSmartPointer(const ValueInfo* val)
{
  return ((val->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_OBJECT && val->Class &&
    strncmp(val->Class, "vtkSmartPointer<", 16) == 0);
}

int vtkWrap_IsSpecialObject(const ValueInfo* val)
{
  /* exclude classes in std:: space, they will have separate handlers */
  unsigned int t = (val->Type & VTK_PARSE_UNQUALIFIED_TYPE);
  return (
    (t == VTK_PARSE_UNKNOWN || t == VTK_PARSE_OBJECT || t == VTK_PARSE_QOBJECT ||
      t == VTK_PARSE_UNKNOWN_REF || t == VTK_PARSE_OBJECT_REF || t == VTK_PARSE_QOBJECT_REF) &&
    !val->IsEnum && val->Class && strncmp(val->Class, "std::", 5) != 0 &&
    strncmp(val->Class, "vtkSmartPointer<", 16) != 0);
}

int vtkWrap_IsPythonObject(const ValueInfo* val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  return (t == VTK_PARSE_UNKNOWN && strncmp(val->Class, "Py", 2) == 0);
}

/* -------------------------------------------------------------------- */
/* The base types, all are mutually exclusive. */

int vtkWrap_IsObject(const ValueInfo* val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  return (t == VTK_PARSE_UNKNOWN || t == VTK_PARSE_OBJECT || t == VTK_PARSE_QOBJECT);
}

int vtkWrap_IsFunction(const ValueInfo* val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  return (t == VTK_PARSE_FUNCTION);
}

int vtkWrap_IsStream(const ValueInfo* val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  return (t == VTK_PARSE_ISTREAM || t == VTK_PARSE_OSTREAM);
}

int vtkWrap_IsNumeric(const ValueInfo* val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);

  t = (t & ~VTK_PARSE_UNSIGNED);
  switch (t)
  {
    case VTK_PARSE_FLOAT:
    case VTK_PARSE_DOUBLE:
    case VTK_PARSE_CHAR:
    case VTK_PARSE_SHORT:
    case VTK_PARSE_INT:
    case VTK_PARSE_LONG:
    case VTK_PARSE_LONG_LONG:
    case VTK_PARSE_SIGNED_CHAR:
    case VTK_PARSE_SSIZE_T:
    case VTK_PARSE_BOOL:
      return 1;
  }

  return 0;
}

int vtkWrap_IsString(const ValueInfo* val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  return (t == VTK_PARSE_STRING);
}

/* -------------------------------------------------------------------- */
/* Subcategories */

int vtkWrap_IsBool(const ValueInfo* val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  return (t == VTK_PARSE_BOOL);
}

int vtkWrap_IsChar(const ValueInfo* val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  return (t == VTK_PARSE_CHAR);
}

int vtkWrap_IsInteger(const ValueInfo* val)
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
  }

  return 0;
}

int vtkWrap_IsRealNumber(const ValueInfo* val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  return (t == VTK_PARSE_FLOAT || t == VTK_PARSE_DOUBLE);
}

/* -------------------------------------------------------------------- */
/* These are mutually exclusive, as well. */

int vtkWrap_IsScalar(const ValueInfo* val)
{
  unsigned int i = (val->Type & VTK_PARSE_POINTER_MASK);
  return (i == 0);
}

int vtkWrap_IsPointer(const ValueInfo* val)
{
  unsigned int i = (val->Type & VTK_PARSE_POINTER_MASK);
  return (i == VTK_PARSE_POINTER && val->Count == 0 && val->CountHint == 0 &&
    val->NumberOfDimensions <= 1);
}

int vtkWrap_IsArray(const ValueInfo* val)
{
  unsigned int i = (val->Type & VTK_PARSE_POINTER_MASK);
  return (i == VTK_PARSE_POINTER && val->NumberOfDimensions <= 1 &&
    (val->Count != 0 || val->CountHint != 0));
}

int vtkWrap_IsNArray(const ValueInfo* val)
{
  int j = 0;
  unsigned int i = (val->Type & VTK_PARSE_POINTER_MASK);
  if (i != VTK_PARSE_ARRAY || val->NumberOfDimensions <= 1)
  {
    return 0;
  }
  for (j = 0; j < val->NumberOfDimensions; j++)
  {
    if (val->Dimensions[j] == NULL || val->Dimensions[j][0] == '\0')
    {
      return 0;
    }
  }
  return 1;
}

/* -------------------------------------------------------------------- */
/* Other type properties, not mutually exclusive. */

int vtkWrap_IsNonConstRef(const ValueInfo* val)
{
  int isconst = ((val->Type & VTK_PARSE_CONST) != 0);
  unsigned int ptrBits = val->Type & VTK_PARSE_POINTER_MASK;

  /* If this is a reference to a pointer, we need to check whether
   * the pointer is const, for example "int *const &arg".  The "const"
   * we need to check is the one that is adjacent to the "&". */
  while (ptrBits != 0)
  {
    isconst = ((ptrBits & VTK_PARSE_POINTER_LOWMASK) == VTK_PARSE_CONST_POINTER);
    ptrBits >>= 2;
  }

  return ((val->Type & VTK_PARSE_REF) != 0 && !isconst);
}

int vtkWrap_IsConstRef(const ValueInfo* val)
{
  return ((val->Type & VTK_PARSE_REF) != 0 && !vtkWrap_IsNonConstRef(val));
}

int vtkWrap_IsRef(const ValueInfo* val)
{
  return ((val->Type & VTK_PARSE_REF) != 0);
}

int vtkWrap_IsConst(const ValueInfo* val)
{
  return ((val->Type & VTK_PARSE_CONST) != 0);
}

/* -------------------------------------------------------------------- */
/* Check if a vtkNew variable has the same name as the property */
int vtkWrap_IsVTKNew(const ClassInfo* data, const PropertyInfo* property)
{
  int i;
  for (i = 0; i < data->NumberOfVariables; ++i)
  {
    const ValueInfo* var = data->Variables[i];
    if (var->Class && !strncmp(var->Class, "vtkNew<", 7) && property->ClassName &&
      strlen(var->Class) - 8 == strlen(property->ClassName) &&
      !strncmp(var->Class + 7, property->ClassName, strlen(property->ClassName)) &&
      !strcmp(var->Name, property->Name))
    {
      return 1;
    }
  }
  return 0;
}

/* -------------------------------------------------------------------- */
/* Check if the arg type is an enum that is a member of the class */
int vtkWrap_IsEnumMember(const ClassInfo* data, const ValueInfo* arg)
{
  int i;

  if (arg->Class)
  {
    /* check if the enum is a member of the class */
    for (i = 0; i < data->NumberOfEnums; i++)
    {
      const EnumInfo* info = data->Enums[i];
      if (info->Name && strcmp(arg->Class, info->Name) == 0)
      {
        return 1;
      }
    }
  }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Hints */

int vtkWrap_IsNewInstance(const ValueInfo* val)
{
  return ((val->Attributes & VTK_PARSE_NEWINSTANCE) != 0);
}

/* -------------------------------------------------------------------- */
/* Constructor/Destructor checks */

int vtkWrap_IsConstructor(const ClassInfo* c, const FunctionInfo* f)

{
  size_t i, m;
  const char* cp = c->Name;

  if (cp && f->Name && !vtkWrap_IsDestructor(c, f))
  {
    /* remove namespaces and template parameters from the name */
    m = vtkParse_UnscopedNameLength(cp);
    while (cp[m] == ':' && cp[m + 1] == ':')
    {
      cp += m + 2;
      m = vtkParse_UnscopedNameLength(cp);
    }
    for (i = 0; i < m; i++)
    {
      if (cp[i] == '<')
      {
        break;
      }
    }

    return (i == strlen(f->Name) && strncmp(cp, f->Name, i) == 0);
  }

  return 0;
}

int vtkWrap_IsDestructor(const ClassInfo* c, const FunctionInfo* f)
{
  size_t i;
  const char* cp;

  if (c->Name && f->Name)
  {
    cp = f->Signature;
    for (i = 0; cp[i] != '\0' && cp[i] != '('; i++)
    {
      if (cp[i] == '~')
      {
        return 1;
      }
    }
  }

  return 0;
}

int vtkWrap_IsInheritedMethod(const ClassInfo* c, const FunctionInfo* f)
{
  size_t l;
  for (l = 0; c->Name[l]; l++)
  {
    /* ignore template args */
    if (c->Name[l] == '<')
    {
      break;
    }
  }

  if (f->Class && (strlen(f->Class) != l || strncmp(f->Class, c->Name, l) != 0))
  {
    return 1;
  }

  return 0;
}

int vtkWrap_IsSetVectorMethod(const FunctionInfo* f)
{
  if (f->Macro && strncmp(f->Macro, "vtkSetVector", 12) == 0)
  {
    return 1;
  }

  return 0;
}

int vtkWrap_IsGetVectorMethod(const FunctionInfo* f)
{
  if (f->Macro && strncmp(f->Macro, "vtkGetVector", 12) == 0)
  {
    return 1;
  }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Argument counting */

int vtkWrap_CountWrappedParameters(const FunctionInfo* f)
{
  int totalArgs = f->NumberOfParameters;

  if (totalArgs > 0 && (f->Parameters[0]->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_FUNCTION)
  {
    totalArgs = 1;
  }
  else if (totalArgs == 1 &&
    (f->Parameters[0]->Type & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VOID)
  {
    totalArgs = 0;
  }

  return totalArgs;
}

int vtkWrap_CountRequiredArguments(const FunctionInfo* f)
{
  int requiredArgs = 0;
  int totalArgs;
  int i;

  totalArgs = vtkWrap_CountWrappedParameters(f);

  for (i = 0; i < totalArgs; i++)
  {
    if (f->Parameters[i]->Value == NULL || vtkWrap_IsNArray(f->Parameters[i]))
    {
      requiredArgs = i + 1;
    }
  }

  return requiredArgs;
}

/* -------------------------------------------------------------------- */
/* Check whether the class is derived from vtkObjectBase. */

int vtkWrap_IsVTKObjectBaseType(const HierarchyInfo* hinfo, const char* classname)
{
  const HierarchyEntry* entry;

  if (hinfo)
  {
    entry = vtkParseHierarchy_FindEntry(hinfo, classname);
    if (entry)
    {
      if (vtkParseHierarchy_IsTypeOf(hinfo, entry, "vtkObjectBase"))
      {
        return 1;
      }
      return 0;
    }
  }

  /* fallback if no HierarchyInfo, but skip smart pointers */
  if (strncmp("vtk", classname, 3) == 0 && strncmp("vtkSmartPointer", classname, 15) != 0)
  {
    return 1;
  }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Check if the class is not derived from vtkObjectBase. */

int vtkWrap_IsSpecialType(const HierarchyInfo* hinfo, const char* classname)
{
  const HierarchyEntry* entry;

  if (hinfo)
  {
    entry = vtkParseHierarchy_FindEntry(hinfo, classname);
    if (entry)
    {
      if (!vtkParseHierarchy_IsTypeOf(hinfo, entry, "vtkObjectBase"))
      {
        return 1;
      }
    }
    return 0;
  }

  /* fallback if no HierarchyInfo */
  if (strncmp("vtk", classname, 3) == 0)
  {
    return -1;
  }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Check if the class is derived from superclass */

int vtkWrap_IsTypeOf(const HierarchyInfo* hinfo, const char* classname, const char* superclass)
{
  const HierarchyEntry* entry;

  if (strcmp(classname, superclass) == 0)
  {
    return 1;
  }

  if (hinfo)
  {
    entry = vtkParseHierarchy_FindEntry(hinfo, classname);
    if (entry && vtkParseHierarchy_IsTypeOf(hinfo, entry, superclass))
    {
      return 1;
    }
  }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Make a guess about whether a class is wrapped */

int vtkWrap_IsClassWrapped(const HierarchyInfo* hinfo, const char* classname)
{
  if (hinfo)
  {
    const HierarchyEntry* entry;
    entry = vtkParseHierarchy_FindEntry(hinfo, classname);

    if (entry && !vtkParseHierarchy_GetProperty(entry, "WRAPEXCLUDE"))
    {
      return 1;
    }
  }
  else if (strncmp("vtk", classname, 3) == 0)
  {
    return 1;
  }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Check whether the destructor is public */
int vtkWrap_HasPublicDestructor(ClassInfo* data)
{
  const FunctionInfo* func;
  int i;

  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    func = data->Functions[i];

    if (vtkWrap_IsDestructor(data, func) && (func->Access != VTK_ACCESS_PUBLIC || func->IsDeleted))
    {
      return 0;
    }
  }

  return 1;
}

/* -------------------------------------------------------------------- */
/* Check whether the copy constructor is public */
int vtkWrap_HasPublicCopyConstructor(ClassInfo* data)
{
  const FunctionInfo* func;
  int i;

  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    func = data->Functions[i];

    if (vtkWrap_IsConstructor(data, func) && func->NumberOfParameters == 1 &&
      func->Parameters[0]->Class && strcmp(func->Parameters[0]->Class, data->Name) == 0 &&
      (func->Access != VTK_ACCESS_PUBLIC || func->IsDeleted))
    {
      return 0;
    }
  }

  return 1;
}

/* -------------------------------------------------------------------- */
/* Get the size for subclasses of vtkTuple */
int vtkWrap_GetTupleSize(const ClassInfo* data, const HierarchyInfo* hinfo)
{
  const HierarchyEntry* entry;
  const char* classname = NULL;
  size_t m;
  int size = 0;

  entry = vtkParseHierarchy_FindEntry(hinfo, data->Name);
  if (entry &&
    vtkParseHierarchy_IsTypeOfTemplated(hinfo, entry, data->Name, "vtkTuple", &classname))
  {
    /* attempt to get count from template parameter */
    if (classname)
    {
      m = strlen(classname);
      if (m > 2 && classname[m - 1] == '>' && isdigit(classname[m - 2]) &&
        (classname[m - 3] == ' ' || classname[m - 3] == ',' || classname[m - 3] == '<'))
      {
        size = classname[m - 2] - '0';
      }
      free((char*)classname);
    }
  }

  return size;
}

/* -------------------------------------------------------------------- */
/* This sets the CountHint for vtkDataArray methods where the
 * tuple size is equal to GetNumberOfComponents. */
void vtkWrap_FindCountHints(ClassInfo* data, FileInfo* finfo, const HierarchyInfo* hinfo)
{
  int i;
  int count;
  const char* countMethod;
  FunctionInfo* theFunc;

  /* add hints for vtkInformation get methods */
  if (vtkWrap_IsTypeOf(hinfo, data->Name, "vtkInformation"))
  {
    countMethod = "Length(temp0)";

    for (i = 0; i < data->NumberOfFunctions; i++)
    {
      theFunc = data->Functions[i];

      if (strcmp(theFunc->Name, "Get") == 0 && theFunc->NumberOfParameters >= 1 &&
        theFunc->Parameters[0]->Type == VTK_PARSE_OBJECT_PTR &&
        (strcmp(theFunc->Parameters[0]->Class, "vtkInformationIntegerVectorKey") == 0 ||
          strcmp(theFunc->Parameters[0]->Class, "vtkInformationDoubleVectorKey") == 0))
      {
        if (theFunc->ReturnValue && theFunc->ReturnValue->Count == 0 &&
          theFunc->NumberOfParameters == 1)
        {
          theFunc->ReturnValue->CountHint = countMethod;
        }
      }
    }
  }

  /* add hints for array GetTuple methods */
  if (vtkWrap_IsTypeOf(hinfo, data->Name, "vtkDataArray") ||
    vtkWrap_IsTypeOf(hinfo, data->Name, "vtkArrayIterator"))
  {
    countMethod = "GetNumberOfComponents()";

    for (i = 0; i < data->NumberOfFunctions; i++)
    {
      theFunc = data->Functions[i];

      if ((strcmp(theFunc->Name, "GetTuple") == 0 || strcmp(theFunc->Name, "GetTypedTuple") == 0) &&
        theFunc->ReturnValue && theFunc->ReturnValue->Count == 0 &&
        theFunc->NumberOfParameters == 1 && vtkWrap_IsScalar(theFunc->Parameters[0]) &&
        vtkWrap_IsInteger(theFunc->Parameters[0]))
      {
        theFunc->ReturnValue->CountHint = countMethod;
      }
      else if ((strcmp(theFunc->Name, "SetTuple") == 0 ||
                 strcmp(theFunc->Name, "SetTypedTuple") == 0 ||
                 strcmp(theFunc->Name, "GetTuple") == 0 ||
                 strcmp(theFunc->Name, "GetTypedTuple") == 0 ||
                 strcmp(theFunc->Name, "InsertTuple") == 0 ||
                 strcmp(theFunc->Name, "InsertTypedTuple") == 0) &&
        theFunc->NumberOfParameters == 2 && vtkWrap_IsScalar(theFunc->Parameters[0]) &&
        vtkWrap_IsInteger(theFunc->Parameters[0]) && theFunc->Parameters[1]->Count == 0)
      {
        theFunc->Parameters[1]->CountHint = countMethod;
      }
      else if ((strcmp(theFunc->Name, "InsertNextTuple") == 0 ||
                 strcmp(theFunc->Name, "InsertNextTypedTuple") == 0) &&
        theFunc->NumberOfParameters == 1 && theFunc->Parameters[0]->Count == 0)
      {
        theFunc->Parameters[0]->CountHint = countMethod;
      }
    }
  }

  /* add hints for interpolator Interpolate methods */
  if (vtkWrap_IsTypeOf(hinfo, data->Name, "vtkAbstractImageInterpolator"))
  {
    countMethod = "GetNumberOfComponents()";

    for (i = 0; i < data->NumberOfFunctions; i++)
    {
      theFunc = data->Functions[i];

      if (strcmp(theFunc->Name, "Interpolate") == 0 && theFunc->NumberOfParameters == 2 &&
        theFunc->Parameters[0]->Type == (VTK_PARSE_DOUBLE_PTR | VTK_PARSE_CONST) &&
        theFunc->Parameters[0]->Count == 3 &&
        theFunc->Parameters[1]->Type == VTK_PARSE_DOUBLE_PTR && theFunc->Parameters[1]->Count == 0)
      {
        theFunc->Parameters[1]->CountHint = countMethod;
      }
    }
  }

  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    theFunc = data->Functions[i];

    /* hints for constructors that take arrays */
    if (vtkWrap_IsConstructor(data, theFunc) && theFunc->NumberOfParameters == 1 &&
      vtkWrap_IsPointer(theFunc->Parameters[0]) && vtkWrap_IsNumeric(theFunc->Parameters[0]) &&
      theFunc->Parameters[0]->Count == 0 && hinfo)
    {
      count = vtkWrap_GetTupleSize(data, hinfo);
      if (count)
      {
        char counttext[24];
        snprintf(counttext, sizeof(counttext), "%d", count);
        theFunc->Parameters[0]->Count = count;
        vtkParse_AddStringToArray(&theFunc->Parameters[0]->Dimensions,
          &theFunc->Parameters[0]->NumberOfDimensions,
          vtkParse_CacheString(finfo->Strings, counttext, strlen(counttext)));
      }
    }

    /* hints for operator[] index range */
    if (theFunc->IsOperator && theFunc->Name && strcmp(theFunc->Name, "operator[]") == 0)
    {
      if (vtkWrap_IsTypeOf(hinfo, data->Name, "vtkTuple"))
      {
        theFunc->SizeHint = "GetSize()";
      }
      else if (vtkWrap_IsTypeOf(hinfo, data->Name, "vtkArrayCoordinates") ||
        vtkWrap_IsTypeOf(hinfo, data->Name, "vtkArrayExtents") ||
        vtkWrap_IsTypeOf(hinfo, data->Name, "vtkArraySort"))
      {
        theFunc->SizeHint = "GetDimensions()";
      }
      else if (vtkWrap_IsTypeOf(hinfo, data->Name, "vtkArrayExtentsList") ||
        vtkWrap_IsTypeOf(hinfo, data->Name, "vtkArrayWeights"))
      {
        theFunc->SizeHint = "GetCount()";
      }
    }
  }
}

/* -------------------------------------------------------------------- */
/* This sets the NewInstance hint for generator methods. */
void vtkWrap_FindNewInstanceMethods(ClassInfo* data, const HierarchyInfo* hinfo)
{
  int i;
  FunctionInfo* theFunc;
  const OptionInfo* options;

  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    theFunc = data->Functions[i];
    if (theFunc->Name && theFunc->ReturnValue && vtkWrap_IsVTKObject(theFunc->ReturnValue) &&
      (theFunc->ReturnValue->Attributes & VTK_PARSE_NEWINSTANCE) == 0 &&
      vtkWrap_IsVTKObjectBaseType(hinfo, theFunc->ReturnValue->Class))
    {
      int needsNewInstance = 0;
      if (strcmp(theFunc->Name, "NewInstance") == 0 || strcmp(theFunc->Name, "CreateInstance") == 0)
      {
        needsNewInstance = 1;
      }
      else if (strcmp(theFunc->Name, "NewIterator") == 0)
      {
        needsNewInstance = 1;
      }
      else if (strcmp(theFunc->Name, "MakeKey") == 0 &&
        vtkWrap_IsTypeOf(hinfo, data->Name, "vtkInformationKey"))
      {
        needsNewInstance = 1;
      }

      if (needsNewInstance)
      {
        /* get the command-line options */
        options = vtkParse_GetCommandLineOptions();
        fprintf(stderr, "Warning: %s without VTK_NEWINSTANCE hint in %s\n", theFunc->Name,
          options->InputFileName);
        theFunc->ReturnValue->Attributes |= VTK_PARSE_NEWINSTANCE;
        /* Do not finalize `options` here; we're just peeking at global state
         * to know when to warn. */
      }
    }
  }
}

/* -------------------------------------------------------------------- */
/* This sets the FilePath hint for method parameters. */
void vtkWrap_FindFilePathMethods(ClassInfo* data)
{
  int i, n;
  size_t l;
  FunctionInfo* theFunc;
  const char* name;
  ValueInfo* arg;

  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    theFunc = data->Functions[i];
    arg = NULL;
    name = theFunc->Name;
    if (name)
    {
      /* check if method ends in "FileName" or "DirectoryName" */
      l = strlen(name);
      if ((l >= 8 && strcmp(&name[l - 8], "FileName") == 0) ||
        (l >= 13 && strcmp(&name[l - 13], "DirectoryName") == 0) ||
        (l == 11 && strcmp(name, "CanReadFile") == 0))
      {
        n = theFunc->NumberOfParameters;
        /* look for Set and Get methods */
        if (n == 0 && strncmp(name, "Get", 3) == 0)
        {
          arg = theFunc->ReturnValue;
        }
        else if (n == 1 && strncmp(name, "Set", 3) == 0)
        {
          arg = theFunc->Parameters[0];
        }
        else if (n == 1 && strncmp(name, "Can", 3) == 0)
        {
          arg = theFunc->Parameters[0];
        }
        /* check the parameter type (must be string) */
        if (arg && (vtkWrap_IsCharPointer(arg) || vtkWrap_IsString(arg)))
        {
          arg->Attributes |= VTK_PARSE_FILEPATH;
        }
      }
    }
  }
}

/* -------------------------------------------------------------------- */
/* Expand all typedef types that are used in function arguments */
void vtkWrap_ExpandTypedefs(ClassInfo* data, FileInfo* finfo, const HierarchyInfo* hinfo)
{
  int i, j, n;
  FunctionInfo* funcInfo;
  const char* newclass;

  n = data->NumberOfSuperClasses;
  for (i = 0; i < n; i++)
  {
    newclass = vtkParseHierarchy_ExpandTypedefsInName(hinfo, data->SuperClasses[i], NULL);
    if (newclass != data->SuperClasses[i])
    {
      data->SuperClasses[i] = vtkParse_CacheString(finfo->Strings, newclass, strlen(newclass));
      free((char*)newclass);
    }
  }

  n = data->NumberOfFunctions;
  for (i = 0; i < n; i++)
  {
    funcInfo = data->Functions[i];
    if (funcInfo->Access == VTK_ACCESS_PUBLIC)
    {
      for (j = 0; j < funcInfo->NumberOfParameters; j++)
      {
        vtkParseHierarchy_ExpandTypedefsInValue(
          hinfo, funcInfo->Parameters[j], finfo->Strings, funcInfo->Class);
#ifndef VTK_PARSE_LEGACY_REMOVE
        if (j < MAX_ARGS)
        {
          if (vtkWrap_IsFunction(funcInfo->Parameters[j]))
          {
            // legacy args only allow "void func(void *)" functions
            if (vtkWrap_IsVoidFunction(funcInfo->Parameters[j]))
            {
              funcInfo->ArgTypes[j] = VTK_PARSE_FUNCTION;
              funcInfo->ArgClasses[j] = funcInfo->Parameters[j]->Class;
            }
          }
          else
          {
            funcInfo->ArgTypes[j] = funcInfo->Parameters[j]->Type;
            funcInfo->ArgClasses[j] = funcInfo->Parameters[j]->Class;
          }
        }
#endif
      }
      if (funcInfo->ReturnValue)
      {
        vtkParseHierarchy_ExpandTypedefsInValue(
          hinfo, funcInfo->ReturnValue, finfo->Strings, funcInfo->Class);
#ifndef VTK_PARSE_LEGACY_REMOVE
        if (!vtkWrap_IsFunction(funcInfo->ReturnValue))
        {
          funcInfo->ReturnType = funcInfo->ReturnValue->Type;
          funcInfo->ReturnClass = funcInfo->ReturnValue->Class;
        }
#endif
      }
    }
  }
}

/* -------------------------------------------------------------------- */
/* Merge superclass methods according to using declarations */
void vtkWrap_ApplyUsingDeclarations(ClassInfo* data, FileInfo* finfo, const HierarchyInfo* hinfo)
{
  int i, n;

  /* first, check if there are any declarations to apply */
  n = data->NumberOfUsings;
  for (i = 0; i < n; i++)
  {
    if (data->Usings[i]->Name)
    {
      break;
    }
  }
  /* if using declarations found, read superclass headers */
  if (i < n)
  {
    n = data->NumberOfSuperClasses;
    for (i = 0; i < n; i++)
    {
      vtkParseMerge_MergeHelper(
        finfo, finfo->Contents, hinfo, data->SuperClasses[i], 0, NULL, NULL, data);
    }
  }
}

/* -------------------------------------------------------------------- */
/* Merge superclass methods */
void vtkWrap_MergeSuperClasses(ClassInfo* data, FileInfo* finfo, const HierarchyInfo* hinfo)
{
  int n = data->NumberOfSuperClasses;
  int i;
  MergeInfo* info;

  if (n == 0)
  {
    return;
  }

  info = vtkParseMerge_CreateMergeInfo(data);

  for (i = 0; i < n; i++)
  {
    vtkParseMerge_MergeHelper(
      finfo, finfo->Contents, hinfo, data->SuperClasses[i], 0, NULL, info, data);
  }

  vtkParseMerge_FreeMergeInfo(info);
}

/* -------------------------------------------------------------------- */
/* get the type name */

const char* vtkWrap_GetTypeName(const ValueInfo* val)
{
  unsigned int aType = val->Type;
  const char* aClass = val->Class;

  /* print the type itself */
  switch (aType & VTK_PARSE_BASE_TYPE)
  {
    case VTK_PARSE_FLOAT:
      return "float";
    case VTK_PARSE_DOUBLE:
      return "double";
    case VTK_PARSE_INT:
      return "int";
    case VTK_PARSE_SHORT:
      return "short";
    case VTK_PARSE_LONG:
      return "long";
    case VTK_PARSE_VOID:
      return "void ";
    case VTK_PARSE_CHAR:
      return "char";
    case VTK_PARSE_UNSIGNED_INT:
      return "unsigned int";
    case VTK_PARSE_UNSIGNED_SHORT:
      return "unsigned short";
    case VTK_PARSE_UNSIGNED_LONG:
      return "unsigned long";
    case VTK_PARSE_UNSIGNED_CHAR:
      return "unsigned char";
    case VTK_PARSE_LONG_LONG:
      return "long long";
    case VTK_PARSE_UNSIGNED_LONG_LONG:
      return "unsigned long long";
    case VTK_PARSE_SIGNED_CHAR:
      return "signed char";
    case VTK_PARSE_BOOL:
      return "bool";
    case VTK_PARSE_SSIZE_T:
      return "ssize_t";
    case VTK_PARSE_SIZE_T:
      return "size_t";
  }

  return aClass;
}

/* -------------------------------------------------------------------- */
/* variable declarations */

void vtkWrap_DeclareVariable(
  FILE* fp, const ClassInfo* data, const ValueInfo* val, const char* name, int i, int flags)
{
  unsigned int aType;
  int j;
  const char* typeName;
  char* newTypeName = NULL;

  if (val == NULL)
  {
    return;
  }

  aType = (val->Type & VTK_PARSE_UNQUALIFIED_TYPE);

  /* do nothing for void */
  if (aType == VTK_PARSE_VOID || (aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_FUNCTION)
  {
    return;
  }

  typeName = vtkWrap_GetTypeName(val);

  if (vtkWrap_IsEnumMember(data, val))
  {
    /* use a typedef to work around compiler issues when someone used
       the same name for the enum type as for a variable or method */
    size_t newTypeNameLen = strlen(name) + 19 + 5 + 1;
    newTypeName = (char*)malloc(newTypeNameLen);
    if (i >= 0)
    {
      snprintf(newTypeName, newTypeNameLen, "%s%i_type", name, i);
    }
    else
    {
      snprintf(newTypeName, newTypeNameLen, "%s_type", name);
    }
    fprintf(fp, "  typedef %s::%s %s;\n", data->Name, typeName, newTypeName);
    typeName = newTypeName;
  }

  /* add a couple spaces for indentation*/
  fprintf(fp, "  ");

  /* for const * return types, prepend with const */
  if ((flags & VTK_WRAP_RETURN) != 0)
  {
    if ((val->Type & VTK_PARSE_CONST) != 0 && (aType & VTK_PARSE_INDIRECT) != 0)
    {
      fprintf(fp, "const ");
    }
  }
  /* do the same for "const char *" arguments */
  else
  {
    if ((val->Type & VTK_PARSE_CONST) != 0 && aType == VTK_PARSE_CHAR_PTR)
    {
      fprintf(fp, "const ");
    }
  }

  /* print the type name */
  fprintf(fp, "%s ", typeName);

  /* indirection */
  if ((flags & VTK_WRAP_RETURN) != 0)
  {
    /* ref and pointer return values are stored as pointers */
    if ((aType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER ||
      (aType & VTK_PARSE_INDIRECT) == VTK_PARSE_REF)
    {
      fprintf(fp, "*");
    }
  }
  else
  {
    /* objects refs and pointers are always handled via pointers,
     * other refs are passed by value */
    if (vtkWrap_IsVTKObject(val) || vtkWrap_IsSpecialObject(val))
    {
      fprintf(fp, "*");
    }
    /* handling of "char *" C strings, "void *" C buffers */
    else if (aType == VTK_PARSE_CHAR_PTR || aType == VTK_PARSE_VOID_PTR)
    {
      fprintf(fp, "*");
    }
    /* arrays of unknown size are handled via pointers */
    else if (val->CountHint || vtkWrap_IsPODPointer(val) || vtkWrap_IsZeroCopyPointer(val) ||
      (vtkWrap_IsArray(val) && val->Value))
    {
      fprintf(fp, "*");
    }
  }

  /* the variable name */
  if (i >= 0)
  {
    fprintf(fp, "%s%i", name, i);
  }
  else
  {
    fprintf(fp, "%s", name);
  }

  if ((flags & VTK_WRAP_ARG) != 0)
  {
    /* print the array decorators */
    if (((aType & VTK_PARSE_POINTER_MASK) != 0) && aType != VTK_PARSE_CHAR_PTR &&
      aType != VTK_PARSE_VOID_PTR && aType != VTK_PARSE_OBJECT_PTR && val->CountHint == NULL &&
      !vtkWrap_IsPODPointer(val) && !(vtkWrap_IsArray(val) && val->Value))
    {
      if (val->NumberOfDimensions <= 1 && val->Count > 0)
      {
        fprintf(fp, "[%d]", val->Count);
      }
      else
      {
        for (j = 0; j < val->NumberOfDimensions; j++)
        {
          fprintf(fp, "[%s]", val->Dimensions[j]);
        }
      }
    }

    /* add a default value */
    else if (val->Value)
    {
      fprintf(fp, " = ");
      vtkWrap_QualifyExpression(fp, data, val->Value);
    }
    else if (aType == VTK_PARSE_CHAR_PTR || aType == VTK_PARSE_VOID_PTR ||
      (!val->IsEnum && !vtkWrap_IsVTKSmartPointer(val) &&
        (aType == VTK_PARSE_OBJECT_PTR || aType == VTK_PARSE_OBJECT_REF ||
          aType == VTK_PARSE_OBJECT)))
    {
      fprintf(fp, " = nullptr");
    }
    else if (val->CountHint || vtkWrap_IsPODPointer(val))
    {
      fprintf(fp, " = nullptr");
    }
    else if (aType == VTK_PARSE_BOOL)
    {
      fprintf(fp, " = false");
    }
  }

  /* finish off with a semicolon */
  if ((flags & VTK_WRAP_NOSEMI) == 0)
  {
    fprintf(fp, ";\n");
  }

  free(newTypeName);
}

void vtkWrap_DeclareVariableSize(FILE* fp, const ValueInfo* val, const char* name, int i)
{
  char idx[32];
  int j;

  idx[0] = '\0';
  if (i >= 0)
  {
    snprintf(idx, sizeof(idx), "%d", i);
  }

  if (val->NumberOfDimensions > 1)
  {
    fprintf(fp, "  static size_t %s%s[%d] = ", name, idx, val->NumberOfDimensions);

    for (j = 0; j < val->NumberOfDimensions; j++)
    {
      fprintf(fp, "%c %s", ((j == 0) ? '{' : ','), val->Dimensions[j]);
    }

    fprintf(fp, " };\n");
  }
  else if (val->Count != 0 || val->CountHint || vtkWrap_IsPODPointer(val))
  {
    fprintf(fp, "  %ssize_t %s%s = %d;\n", ((val->Count == 0 || val->Value != 0) ? "" : "const "),
      name, idx, (val->Count == 0 ? 0 : val->Count));
  }
  else if (val->NumberOfDimensions == 1)
  {
    fprintf(fp, "  const size_t %s%s = %s;\n", name, idx, val->Dimensions[0]);
  }
}

void vtkWrap_QualifyExpression(FILE* fp, const ClassInfo* data, const char* text)
{
  StringTokenizer t;
  int qualified = 0;
  int matched;
  int j;

  /* tokenize the text according to C/C++ rules */
  vtkParse_InitTokenizer(&t, text, WS_DEFAULT);
  do
  {
    /* check whether we have found an unqualified identifier */
    matched = 0;
    if (t.tok == TOK_ID && !qualified)
    {
      /* check for class members */
      for (j = 0; j < data->NumberOfItems; j++)
      {
        const ItemInfo* item = &data->Items[j];
        const char* name = NULL;

        if (item->Type == VTK_CONSTANT_INFO)
        {
          /* enum values and other constants */
          name = data->Constants[item->Index]->Name;
        }
        else if (item->Type == VTK_CLASS_INFO || item->Type == VTK_STRUCT_INFO ||
          item->Type == VTK_UNION_INFO)
        {
          /* embedded classes */
          name = data->Classes[item->Index]->Name;
        }
        else if (item->Type == VTK_ENUM_INFO)
        {
          /* enum type */
          name = data->Enums[item->Index]->Name;
        }
        else if (item->Type == VTK_TYPEDEF_INFO)
        {
          /* typedef'd type */
          name = data->Typedefs[item->Index]->Name;
        }

        if (name && strlen(name) == t.len && strncmp(name, t.text, t.len) == 0)
        {
          fprintf(fp, "%s::%s", data->Name, name);
          matched = 1;
          break;
        }
      }
    }

    if (!matched)
    {
      fprintf(fp, "%*.*s", (int)t.len, (int)t.len, t.text);
    }

    /* if next character is whitespace, add a space */
    if (vtkParse_CharType(t.text[t.len], CPRE_WHITE))
    {
      fprintf(fp, " ");
    }

    /* check whether the next identifier is qualified */
    qualified = (t.tok == TOK_SCOPE || t.tok == TOK_ARROW || t.tok == '.');
  } while (vtkParse_NextToken(&t));
}

char* vtkWrap_SafeSuperclassName(const char* name)
{
  int template_class = 0;
  size_t size = strlen(name);
  char* safe_name = malloc(size + 1);
  size_t i;

  memcpy(safe_name, name, size + 1);

  for (i = 0; i < size; ++i)
  {
    char c = name[i];
    if (c == '<' || c == '>')
    {
      safe_name[i] = '_';
      template_class = 1;
    }
    if (c == ',' || c == ' ')
    {
      safe_name[i] = '_';
    }
  }

  if (!template_class)
  {
    free(safe_name);
    return NULL;
  }
  return safe_name;
}

char* vtkWrap_TemplateArg(const char* name)
{
  /* ignore 2nd arg if present (e.g. std::vector allocator) */
  const char* defaults[2] = { NULL, "" };
  const char** args;
  char* arg;

  vtkParse_DecomposeTemplatedType(name, NULL, 2, &args, defaults);
  arg = strdup(args[0]);
  vtkParse_FreeTemplateDecomposition(NULL, 2, args);

  return arg;
}

void vtkWrap_WarnEmpty(const OptionInfo* options)
{
  if (options->WarningFlags.Empty)
  {
    fprintf(stderr, "warning: did not wrap anything from %s [-Wempty]\n", options->InputFileName);
  }
}
