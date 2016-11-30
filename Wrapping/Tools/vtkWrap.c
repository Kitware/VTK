/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrap.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkWrap.h"
#include "vtkParseData.h"
#include "vtkParseExtras.h"
#include "vtkParseMain.h"
#include "vtkParseMerge.h"
#include "vtkParseString.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* -------------------------------------------------------------------- */
/* Common types. */

int vtkWrap_IsVoid(ValueInfo *val)
{
  if (val == 0)
  {
    return 1;
  }

  return ((val->Type & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VOID);
}

int vtkWrap_IsVoidFunction(ValueInfo *val)
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

int vtkWrap_IsVoidPointer(ValueInfo *val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  return (t == VTK_PARSE_VOID && vtkWrap_IsPointer(val));
}

int vtkWrap_IsCharPointer(ValueInfo *val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  return (t == VTK_PARSE_CHAR && vtkWrap_IsPointer(val));
}

int vtkWrap_IsPODPointer(ValueInfo *val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  return (t != VTK_PARSE_CHAR && vtkWrap_IsNumeric(val) &&
          vtkWrap_IsPointer(val));
}

int vtkWrap_IsVTKObject(ValueInfo *val)
{
  unsigned int t = (val->Type & VTK_PARSE_UNQUALIFIED_TYPE);
  return (t == VTK_PARSE_OBJECT_PTR &&
          val->Class[0] == 'v' && strncmp(val->Class, "vtk", 3) == 0);
}

int vtkWrap_IsSpecialObject(ValueInfo *val)
{
  unsigned int t = (val->Type & VTK_PARSE_UNQUALIFIED_TYPE);
  return ((t == VTK_PARSE_OBJECT ||
           t == VTK_PARSE_OBJECT_REF) &&
          val->Class[0] == 'v' && strncmp(val->Class, "vtk", 3) == 0);
}

int vtkWrap_IsPythonObject(ValueInfo *val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  return (t == VTK_PARSE_UNKNOWN &&
          strncmp(val->Class, "Py", 2) == 0);
}

int vtkWrap_IsQtObject(ValueInfo *val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  if (t == VTK_PARSE_QOBJECT &&
      val->Class[0] == 'Q' && isupper(val->Class[1]))
  {
    return 1;
  }
  return 0;
}

int vtkWrap_IsQtEnum(ValueInfo *val)
{
  unsigned int t = (val->Type & VTK_PARSE_UNQUALIFIED_TYPE);
  if ((t == VTK_PARSE_QOBJECT || t == VTK_PARSE_QOBJECT_REF) &&
      val->Class[0] == 'Q' && strncmp("Qt::", val->Class, 4) == 0)
  {
    return 1;
  }
  return 0;
}


/* -------------------------------------------------------------------- */
/* The base types, all are mutually exclusive. */

int vtkWrap_IsObject(ValueInfo *val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  return (t == VTK_PARSE_OBJECT ||
          t == VTK_PARSE_QOBJECT);
}

int vtkWrap_IsFunction(ValueInfo *val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  return (t == VTK_PARSE_FUNCTION);
}

int vtkWrap_IsStream(ValueInfo *val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  return (t == VTK_PARSE_ISTREAM ||
          t == VTK_PARSE_OSTREAM);
}

int vtkWrap_IsNumeric(ValueInfo *val)
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
    case VTK_PARSE_ID_TYPE:
    case VTK_PARSE_LONG_LONG:
    case VTK_PARSE___INT64:
    case VTK_PARSE_SIGNED_CHAR:
    case VTK_PARSE_SSIZE_T:
    case VTK_PARSE_BOOL:
      return 1;
  }

  return 0;
}

int vtkWrap_IsString(ValueInfo *val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  return (t == VTK_PARSE_STRING ||
          t == VTK_PARSE_UNICODE_STRING);
}

/* -------------------------------------------------------------------- */
/* Subcategories */

int vtkWrap_IsBool(ValueInfo *val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  return (t == VTK_PARSE_BOOL);
}

int vtkWrap_IsChar(ValueInfo *val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  return (t == VTK_PARSE_CHAR);
}

int vtkWrap_IsInteger(ValueInfo *val)
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
    case VTK_PARSE_ID_TYPE:
    case VTK_PARSE_LONG_LONG:
    case VTK_PARSE___INT64:
    case VTK_PARSE_UNSIGNED_CHAR:
    case VTK_PARSE_SIGNED_CHAR:
    case VTK_PARSE_SSIZE_T:
      return 1;
  }

  return 0;
}

int vtkWrap_IsRealNumber(ValueInfo *val)
{
  unsigned int t = (val->Type & VTK_PARSE_BASE_TYPE);
  return (t == VTK_PARSE_FLOAT || t == VTK_PARSE_DOUBLE);
}

/* -------------------------------------------------------------------- */
/* These are mutually exclusive, as well. */

int vtkWrap_IsScalar(ValueInfo *val)
{
  unsigned int i = (val->Type & VTK_PARSE_POINTER_MASK);
  return (i == 0);
}

int vtkWrap_IsPointer(ValueInfo *val)
{
  unsigned int i = (val->Type & VTK_PARSE_POINTER_MASK);
  return (i == VTK_PARSE_POINTER && val->Count == 0 &&
          val->CountHint == 0 && val->NumberOfDimensions <= 1);
}

int vtkWrap_IsArray(ValueInfo *val)
{
  unsigned int i = (val->Type & VTK_PARSE_POINTER_MASK);
  return (i == VTK_PARSE_POINTER && val->NumberOfDimensions <= 1 &&
          (val->Count != 0 || val->CountHint != 0));
}

int vtkWrap_IsNArray(ValueInfo *val)
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

int vtkWrap_IsNonConstRef(ValueInfo *val)
{
  return ((val->Type & VTK_PARSE_REF) != 0 &&
          (val->Type & VTK_PARSE_CONST) == 0);
}

int vtkWrap_IsConstRef(ValueInfo *val)
{
  return ((val->Type & VTK_PARSE_REF) != 0 &&
          (val->Type & VTK_PARSE_CONST) != 0);
}

int vtkWrap_IsRef(ValueInfo *val)
{
  return ((val->Type & VTK_PARSE_REF) != 0);
}

int vtkWrap_IsConst(ValueInfo *val)
{
  return ((val->Type & VTK_PARSE_CONST) != 0);
}

/* -------------------------------------------------------------------- */
/* Check if the arg type is an enum that is a member of the class */
int vtkWrap_IsEnumMember(ClassInfo *data, ValueInfo *arg)
{
  int i;

  if (arg->Class)
  {
    /* check if the enum is a member of the class */
    for (i = 0; i < data->NumberOfEnums; i++)
    {
      EnumInfo *info = data->Enums[i];
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

int vtkWrap_IsNewInstance(ValueInfo *val)
{
  return ((val->Type & VTK_PARSE_NEWINSTANCE) != 0);
}

/* -------------------------------------------------------------------- */
/* Constructor/Destructor checks */

int vtkWrap_IsConstructor(ClassInfo *c, FunctionInfo *f)

{
  size_t i, m;
  const char *cp = c->Name;

  if (cp && f->Name && !vtkWrap_IsDestructor(c, f))
  {
    /* remove namespaces and template parameters from the name */
    m = vtkParse_UnscopedNameLength(cp);
    while (cp[m] == ':' && cp[m+1] == ':')
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

int vtkWrap_IsDestructor(ClassInfo *c, FunctionInfo *f)
{
  size_t i;
  const char *cp;

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

int vtkWrap_IsSetVectorMethod(FunctionInfo *f)
{
  if (f->Macro && strncmp(f->Macro, "vtkSetVector", 12) == 0)
  {
    return 1;
  }

  return 0;
}

int vtkWrap_IsGetVectorMethod(FunctionInfo *f)
{
  if (f->Macro && strncmp(f->Macro, "vtkGetVector", 12) == 0)
  {
    return 1;
  }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Argument counting */

int vtkWrap_CountWrappedParameters(FunctionInfo *f)
{
  int totalArgs = f->NumberOfParameters;

  if (totalArgs > 0 &&
      (f->Parameters[0]->Type & VTK_PARSE_BASE_TYPE)
       == VTK_PARSE_FUNCTION)
  {
    totalArgs = 1;
  }
  else if (totalArgs == 1 &&
           (f->Parameters[0]->Type & VTK_PARSE_UNQUALIFIED_TYPE)
            == VTK_PARSE_VOID)
  {
    totalArgs = 0;
  }

  return totalArgs;
}

int vtkWrap_CountRequiredArguments(FunctionInfo *f)
{
  int requiredArgs = 0;
  int totalArgs;
  int i;

  totalArgs = vtkWrap_CountWrappedParameters(f);

  for (i = 0; i < totalArgs; i++)
  {
    if (f->Parameters[i]->Value == NULL ||
        vtkWrap_IsNArray(f->Parameters[i]))
    {
      requiredArgs = i+1;
    }
  }

  return requiredArgs;
}

/* -------------------------------------------------------------------- */
/* Check whether the class is derived from vtkObjectBase. */

int vtkWrap_IsVTKObjectBaseType(
  HierarchyInfo *hinfo, const char *classname)
{
  HierarchyEntry *entry;

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
  if (strncmp("vtk", classname, 3) == 0 &&
      strncmp("vtkSmartPointer", classname, 15) != 0)
  {
    return 1;
  }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Check if the class is not derived from vtkObjectBase. */

int vtkWrap_IsSpecialType(
  HierarchyInfo *hinfo, const char *classname)
{
  HierarchyEntry *entry;

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

int vtkWrap_IsTypeOf(
  HierarchyInfo *hinfo, const char *classname, const char *superclass)
{
  HierarchyEntry *entry;

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

int vtkWrap_IsClassWrapped(
  HierarchyInfo *hinfo, const char *classname)
{
  if (hinfo)
  {
    HierarchyEntry *entry;
    entry = vtkParseHierarchy_FindEntry(hinfo, classname);

    if (entry)
    {
      if (!vtkParseHierarchy_GetProperty(entry, "WRAP_EXCLUDE_PYTHON"))
      {
        return 1;
      }
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
int vtkWrap_HasPublicDestructor(ClassInfo *data)
{
  FunctionInfo *func;
  int i;

  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    func = data->Functions[i];

    if (vtkWrap_IsDestructor(data, func) &&
        func->Access != VTK_ACCESS_PUBLIC)
    {
      return 0;
    }
  }

  return 1;
}

/* -------------------------------------------------------------------- */
/* Check whether the copy constructor is public */
int vtkWrap_HasPublicCopyConstructor(ClassInfo *data)
{
  FunctionInfo *func;
  int i;

  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    func = data->Functions[i];

    if (vtkWrap_IsConstructor(data, func) &&
        func->NumberOfParameters == 1 &&
        func->Parameters[0]->Class &&
        strcmp(func->Parameters[0]->Class, data->Name) == 0 &&
        func->Access != VTK_ACCESS_PUBLIC)
    {
      return 0;
    }
  }

  return 1;
}

/* -------------------------------------------------------------------- */
/* Get the size for subclasses of vtkTuple */
int vtkWrap_GetTupleSize(ClassInfo *data, HierarchyInfo *hinfo)
{
  HierarchyEntry *entry;
  const char *classname = NULL;
  size_t m;
  int size = 0;

  entry = vtkParseHierarchy_FindEntry(hinfo, data->Name);
  if (entry && vtkParseHierarchy_IsTypeOfTemplated(
        hinfo, entry, data->Name, "vtkTuple", &classname))
  {
    /* attempt to get count from template parameter */
    if (classname)
    {
      m = strlen(classname);
      if (m > 2 && classname[m - 1] == '>' &&
          isdigit(classname[m-2]) && (classname[m-3] == ' ' ||
          classname[m-3] == ',' || classname[m-3] == '<'))
      {
        size = classname[m-2] - '0';
      }
      free((char *)classname);
    }
  }

  return size;
}

/* -------------------------------------------------------------------- */
/* This sets the CountHint for vtkDataArray methods where the
 * tuple size is equal to GetNumberOfComponents. */
void vtkWrap_FindCountHints(
  ClassInfo *data, FileInfo *finfo, HierarchyInfo *hinfo)
{
  int i;
  int count;
  const char *countMethod;
  FunctionInfo *theFunc;

  /* add hints for vtkInformation get methods */
  if (vtkWrap_IsTypeOf(hinfo, data->Name, "vtkInformation"))
  {
    countMethod = "Length(temp0)";

    for (i = 0; i < data->NumberOfFunctions; i++)
    {
      theFunc = data->Functions[i];

      if (strcmp(theFunc->Name, "Get") == 0 &&
          theFunc->NumberOfParameters >= 1 &&
          theFunc->Parameters[0]->Type == VTK_PARSE_OBJECT_PTR &&
          (strcmp(theFunc->Parameters[0]->Class,
                  "vtkInformationIntegerVectorKey") == 0 ||
           strcmp(theFunc->Parameters[0]->Class,
                  "vtkInformationDoubleVectorKey") == 0))
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

      if ((strcmp(theFunc->Name, "GetTuple") == 0 ||
           strcmp(theFunc->Name, "GetTypedTuple") == 0) &&
          theFunc->ReturnValue && theFunc->ReturnValue->Count == 0 &&
          theFunc->NumberOfParameters == 1 &&
          theFunc->Parameters[0]->Type == VTK_PARSE_ID_TYPE)
      {
        theFunc->ReturnValue->CountHint = countMethod;
      }
      else if ((strcmp(theFunc->Name, "SetTuple") == 0 ||
                strcmp(theFunc->Name, "SetTypedTuple") == 0 ||
                strcmp(theFunc->Name, "GetTuple") == 0 ||
                strcmp(theFunc->Name, "GetTypedTuple") == 0 ||
                strcmp(theFunc->Name, "InsertTuple") == 0 ||
                strcmp(theFunc->Name, "InsertTypedTuple") == 0) &&
               theFunc->NumberOfParameters == 2 &&
               theFunc->Parameters[0]->Type == VTK_PARSE_ID_TYPE &&
               theFunc->Parameters[1]->Count == 0)
      {
        theFunc->Parameters[1]->CountHint = countMethod;
      }
      else if ((strcmp(theFunc->Name, "InsertNextTuple") == 0 ||
                strcmp(theFunc->Name, "InsertNextTypedTuple") == 0) &&
               theFunc->NumberOfParameters == 1 &&
               theFunc->Parameters[0]->Count == 0)
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

      if (strcmp(theFunc->Name, "Interpolate") == 0 &&
           theFunc->NumberOfParameters == 2 &&
           theFunc->Parameters[0]->Type == (VTK_PARSE_DOUBLE_PTR|VTK_PARSE_CONST) &&
           theFunc->Parameters[0]->Count == 3 &&
           theFunc->Parameters[1]->Type == VTK_PARSE_DOUBLE_PTR &&
           theFunc->Parameters[1]->Count == 0)
      {
        theFunc->Parameters[1]->CountHint = countMethod;
      }
    }
  }

  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    theFunc = data->Functions[i];

    /* hints for constructors that take arrays */
    if (vtkWrap_IsConstructor(data, theFunc) &&
        theFunc->NumberOfParameters == 1 &&
        vtkWrap_IsPointer(theFunc->Parameters[0]) &&
        vtkWrap_IsNumeric(theFunc->Parameters[0]) &&
        theFunc->Parameters[0]->Count == 0 &&
        hinfo)
    {
      count = vtkWrap_GetTupleSize(data, hinfo);
      if (count)
      {
        char counttext[24];
        sprintf(counttext, "%d", count);
        theFunc->Parameters[0]->Count = count;
        vtkParse_AddStringToArray(
          &theFunc->Parameters[0]->Dimensions,
          &theFunc->Parameters[0]->NumberOfDimensions,
          vtkParse_CacheString(finfo->Strings, counttext, strlen(counttext)));
      }
    }

    /* hints for operator[] index range */
    if (theFunc->IsOperator && theFunc->Name &&
        strcmp(theFunc->Name, "operator[]") == 0)
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
void vtkWrap_FindNewInstanceMethods(
  ClassInfo *data, HierarchyInfo *hinfo)
{
  int i;
  FunctionInfo *theFunc;
  OptionInfo *options;

  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    theFunc = data->Functions[i];
    if (theFunc->Name && theFunc->ReturnValue &&
        vtkWrap_IsVTKObject(theFunc->ReturnValue) &&
        vtkWrap_IsVTKObjectBaseType(hinfo, theFunc->ReturnValue->Class))
    {
      if (strcmp(theFunc->Name, "NewInstance") == 0 ||
          strcmp(theFunc->Name, "NewIterator") == 0 ||
          strcmp(theFunc->Name, "CreateInstance") == 0)
      {
        if ((theFunc->ReturnValue->Type & VTK_PARSE_NEWINSTANCE) == 0)
        {
          /* get the command-line options */
          options = vtkParse_GetCommandLineOptions();
          fprintf(stderr, "Warning: %s without VTK_NEWINSTANCE hint in %s\n",
            theFunc->Name, options->InputFileName);
          theFunc->ReturnValue->Type |= VTK_PARSE_NEWINSTANCE;
        }
      }
    }
  }
}


/* -------------------------------------------------------------------- */
/* Expand all typedef types that are used in function arguments */
void vtkWrap_ExpandTypedefs(
  ClassInfo *data, FileInfo *finfo, HierarchyInfo *hinfo)
{
  int i, j, n;
  FunctionInfo *funcInfo;
  const char *newclass;

  n = data->NumberOfSuperClasses;
  for (i = 0; i < n; i++)
  {
    newclass = vtkParseHierarchy_ExpandTypedefsInName(
      hinfo, data->SuperClasses[i], NULL);
    if (newclass != data->SuperClasses[i])
    {
      data->SuperClasses[i] =
        vtkParse_CacheString(finfo->Strings, newclass, strlen(newclass));
      free((char *)newclass);
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
          hinfo, funcInfo->Parameters[j], finfo->Strings, data->Name);
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
          hinfo, funcInfo->ReturnValue, finfo->Strings, data->Name);
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
void vtkWrap_ApplyUsingDeclarations(
  ClassInfo *data, FileInfo *finfo, HierarchyInfo *hinfo)
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
        finfo, finfo->Contents, hinfo, data->SuperClasses[i],
        0, NULL, NULL, data);
    }
  }
}

/* -------------------------------------------------------------------- */
/* get the type name */

const char *vtkWrap_GetTypeName(ValueInfo *val)
{
  unsigned int aType = val->Type;
  const char *aClass = val->Class;

  /* print the type itself */
  switch (aType & VTK_PARSE_BASE_TYPE)
  {
    case VTK_PARSE_FLOAT:          return "float";
    case VTK_PARSE_DOUBLE:         return "double";
    case VTK_PARSE_INT:            return "int";
    case VTK_PARSE_SHORT:          return "short";
    case VTK_PARSE_LONG:           return "long";
    case VTK_PARSE_VOID:           return "void ";
    case VTK_PARSE_CHAR:           return "char";
    case VTK_PARSE_UNSIGNED_INT:   return "unsigned int";
    case VTK_PARSE_UNSIGNED_SHORT: return "unsigned short";
    case VTK_PARSE_UNSIGNED_LONG:  return "unsigned long";
    case VTK_PARSE_UNSIGNED_CHAR:  return "unsigned char";
    case VTK_PARSE_ID_TYPE:        return "vtkIdType";
    case VTK_PARSE_LONG_LONG:      return "long long";
    case VTK_PARSE___INT64:        return "__int64";
    case VTK_PARSE_UNSIGNED_LONG_LONG: return "unsigned long long";
    case VTK_PARSE_UNSIGNED___INT64:   return "unsigned __int64";
    case VTK_PARSE_SIGNED_CHAR:    return "signed char";
    case VTK_PARSE_BOOL:           return "bool";
    case VTK_PARSE_UNICODE_STRING: return "vtkUnicodeString";
    case VTK_PARSE_SSIZE_T:        return "ssize_t";
    case VTK_PARSE_SIZE_T:         return "size_t";
  }

  return aClass;
}

/* -------------------------------------------------------------------- */
/* variable declarations */

void vtkWrap_DeclareVariable(
  FILE *fp, ClassInfo *data, ValueInfo *val, const char *name,
  int i, int flags)
{
  unsigned int aType;
  int j;
  const char *typeName;
  char *newTypeName = NULL;

  if (val == NULL)
  {
    return;
  }

  aType = (val->Type & VTK_PARSE_UNQUALIFIED_TYPE);

  /* do nothing for void */
  if (aType == VTK_PARSE_VOID ||
      (aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_FUNCTION)
  {
    return;
  }

  typeName = vtkWrap_GetTypeName(val);

  if (vtkWrap_IsEnumMember(data, val))
  {
    /* use a typedef to work around compiler issues when someone used
       the same name for the enum type as for a variable or method */
    newTypeName = (char *)malloc(strlen(name) + 16);
    if (i >= 0)
    {
      sprintf(newTypeName, "%s%i_type", name, i);
    }
    else
    {
      sprintf(newTypeName, "%s_type", name);
    }
    fprintf(fp, "  typedef %s::%s %s;\n",
            data->Name, typeName, newTypeName);
    typeName = newTypeName;
  }

  /* add a couple spaces for indentation*/
  fprintf(fp,"  ");

  /* for const * return types, prepend with const */
  if ((flags & VTK_WRAP_RETURN) != 0)
  {
    if ((val->Type & VTK_PARSE_CONST) != 0 &&
        (aType & VTK_PARSE_INDIRECT) != 0)
    {
      fprintf(fp,"const ");
    }
  }
  /* do the same for "const char *" with initializer */
  else
  {
    if ((val->Type & VTK_PARSE_CONST) != 0 &&
        aType == VTK_PARSE_CHAR_PTR &&
        val->Value &&
        strcmp(val->Value, "0") != 0 &&
        strcmp(val->Value, "NULL") != 0)
    {
      fprintf(fp,"const ");
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
    if (aType == VTK_PARSE_CHAR_PTR ||
        aType == VTK_PARSE_VOID_PTR ||
        aType == VTK_PARSE_OBJECT_PTR ||
        aType == VTK_PARSE_OBJECT_REF ||
        aType == VTK_PARSE_OBJECT ||
        vtkWrap_IsQtObject(val))
    {
      fprintf(fp, "*");
    }
    /* arrays of unknown size are handled via pointers */
    else if (val->CountHint || vtkWrap_IsPODPointer(val) ||
             (vtkWrap_IsArray(val) && val->Value))
    {
      fprintf(fp, "*");
    }
  }

  /* the variable name */
  if (i >= 0)
  {
    fprintf(fp,"%s%i", name, i);
  }
  else
  {
    fprintf(fp,"%s", name);
  }

  if ((flags & VTK_WRAP_ARG) != 0)
  {
    /* print the array decorators */
    if (((aType & VTK_PARSE_POINTER_MASK) != 0) &&
        aType != VTK_PARSE_CHAR_PTR &&
        aType != VTK_PARSE_VOID_PTR &&
        aType != VTK_PARSE_OBJECT_PTR &&
        !vtkWrap_IsQtObject(val) &&
        val->CountHint == NULL &&
        !vtkWrap_IsPODPointer(val) &&
        !(vtkWrap_IsArray(val) && val->Value))
    {
      if (val->NumberOfDimensions == 1 && val->Count > 0)
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
      fprintf(fp, " = %s", val->Value);
    }
    else if (aType == VTK_PARSE_CHAR_PTR ||
             aType == VTK_PARSE_VOID_PTR ||
             aType == VTK_PARSE_OBJECT_PTR ||
             aType == VTK_PARSE_OBJECT_REF ||
             aType == VTK_PARSE_OBJECT ||
             vtkWrap_IsQtObject(val))
    {
      fprintf(fp, " = NULL");
    }
    else if (val->CountHint || vtkWrap_IsPODPointer(val))
    {
      fprintf(fp, " = NULL");
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

void vtkWrap_DeclareVariableSize(
  FILE *fp, ValueInfo *val, const char *name, int i)
{
  char idx[32];
  int j;

  idx[0] = '\0';
  if (i >= 0)
  {
    sprintf(idx, "%d", i);
  }

  if (val->NumberOfDimensions > 1)
  {
    fprintf(fp,
            "  static int %s%s[%d] = ",
            name, idx, val->NumberOfDimensions);

    for (j = 0; j < val->NumberOfDimensions; j++)
    {
      fprintf(fp, "%c %s", ((j == 0) ? '{' : ','), val->Dimensions[j]);
    }

    fprintf(fp, " };\n");
  }
  else if (val->Count != 0 || val->CountHint || vtkWrap_IsPODPointer(val))
  {
    fprintf(fp,
            "  %sint %s%s = %d;\n",
            ((val->Count == 0 || val->Value != 0) ? "" : "const "),
            name, idx, (val->Count == 0 ? 0 : val->Count));
  }
  else if (val->NumberOfDimensions == 1)
  {
    fprintf(fp,
            "  const int %s%s = %s;\n",
            name, idx, val->Dimensions[0]);
  }
}

char *vtkWrap_SafeSuperclassName(const char *name)
{
  int template_class = 0;
  size_t size = strlen(name);
  char *safe_name = malloc(size + 1);
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
    if  (c == ',' || c == ' ')
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
