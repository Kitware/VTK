/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapJavaScriptClass.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkWrapJavaScriptClass.h"
#include "vtkParseData.h"
#include "vtkWrap.h"
#include "vtkWrapJavaScriptConstant.h"
#include "vtkWrapJavaScriptEnum.h"
#include "vtkWrapJavaScriptMethod.h"

#include <string.h>

// NOLINTBEGIN(bugprone-unsafe-functions)

#ifdef NDEBUG
#define DLOG(...)
#else
#define DLOG(...) printf(__VA_ARGS__);
#endif

static int vtkWrapJavaScript_IsSpecialTypeWrappable(ClassInfo* data);

/* -------------------------------------------------------------------- */
/* For classes that aren't derived from vtkObjectBase, check to see if
 * they are wrappable */
int vtkWrapJavaScript_IsSpecialTypeWrappable(ClassInfo* data)
{
  /* wrapping templates is only possible after template instantiation */
  if (data->Template)
  {
    return 0;
  }

  /* restrict wrapping to classes that have a "vtk" prefix */
  if (strncmp(data->Name, "vtk", 3) != 0)
  {
    return 0;
  }

  return 1;
}

/* -------------------------------------------------------------------- */
/* get the true superclass */
const char* vtkWrapJavaScript_GetSuperClass(
  ClassInfo* data, HierarchyInfo* hinfo, const char** supermodule)
{
  const char* supername = NULL;
  const char* module = NULL;
  HierarchyEntry* entry;
  int i;

  /* if there are multiple superclasses, we just need the relevant one */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
  {
    supername = data->SuperClasses[i];
    if (vtkWrap_IsClassWrapped(hinfo, supername))
    {
      if (vtkWrap_IsVTKObjectBaseType(hinfo, data->Name))
      {
        /* if class derived from vtkObjectBase, then only accept a
           superclass that is also a vtkObjectBase */
        if (vtkWrap_IsVTKObjectBaseType(hinfo, supername))
        {
          break;
        }
      }
      else
      {
        break;
      }
    }

    supername = NULL;
  }

  if (supermodule)
  {
    *supermodule = NULL;

    if (hinfo && supername)
    {
      /* get superclass module and check against our own */
      entry = vtkParseHierarchy_FindEntry(hinfo, data->Name);
      if (entry)
      {
        module = entry->Module;
      }
      entry = vtkParseHierarchy_FindEntry(hinfo, supername);
      if (entry && (!module || strcmp(entry->Module, module) != 0))
      {
        *supermodule = entry->Module;
      }
    }
  }

  return supername;
}

/* -------------------------------------------------------------------- */
/* Create the docstring for a class, and print it to fp */
void vtkWrapJavaScript_ClassDoc(
  FILE* fp, FileInfo* file_info, ClassInfo* data, HierarchyInfo* hinfo, int is_vtkobject)
{
  (void)fp;
  (void)file_info;
  (void)data;
  (void)hinfo;
  (void)is_vtkobject;
}

/* -------------------------------------------------------------------- */
/* Wrap one class */
int vtkWrapJavaScript_WrapOneClass(FILE* fp, const char* module, const char* classname,
  ClassInfo* data, FileInfo* file_info, HierarchyInfo* hinfo, int is_vtkobject)
{
  int class_has_new = 0;

  /* recursive handling of templated classes */
  if (data->Template)
  {
    // return vtkWrapJavaScript_WrapTemplatedClass()
    return 0;
  }

  /* verify wrappability */
  if (!is_vtkobject && !vtkWrapJavaScript_IsSpecialTypeWrappable(data))
  {
    return 0;
  }

  /* check for New() function */
  for (int i = 0; i < data->NumberOfFunctions; i++)
  {
    FunctionInfo* func = data->Functions[i];
    if (func->IsDeprecated)
    {
      // skip deprecated member functions.
      continue;
    }

    if (func->Name && !func->IsExcluded && func->Access == VTK_ACCESS_PUBLIC &&
      strncmp("New", func->Name, 3) == 0 && func->NumberOfParameters == 0 &&
      !vtkWrap_IsInheritedMethod(data, func))
    {
      class_has_new = 1;
      if (func->IsDeprecated)
      {
        // skip class with deprecated ::New method
        return 0;
      }
    }
  }

  if (data->IsAbstract)
  {
    DLOG("%s abstract class is not fully supported.\n", classname);
  }

  /* create any enum types defined in the class */
  vtkWrapJavaScript_GenerateEnumTypes(fp, module, classname, "  ", data);

  if (is_vtkobject || class_has_new)
  {
    // add destructor
    fprintf(fp,
      "template<> void emscripten::internal::raw_destructor<%s>(%s * ptr){ ptr->Delete(); }",
      classname, classname);
  }
  fprintf(fp, "\nEMSCRIPTEN_BINDINGS(%s_class) {", classname);

  for (int i = 0; i < data->NumberOfEnums; ++i)
  {
    if (!data->Enums[i]->IsExcluded && data->Enums[i]->Access == VTK_ACCESS_PUBLIC)
    {
      EnumInfo* enumInfo = data->Enums[i];
      int found = 0;
      /* check to make sure there won't be a name conflict between an
         enum type and some other class member, it happens specifically
         for vtkImplicitBoolean which has a variable and enum type both
         with the name OperationType */
      for (int j = 0; j < data->NumberOfVariables && !found; j++)
      {
        found = (strcmp(data->Variables[j]->Name, enumInfo->Name) == 0);
      }
      if (!found)
      {
        // Scoped C++/C style enums need to be `using`'d.
        fprintf(fp, "\n  using %s=%s::%s;", enumInfo->Name, classname, enumInfo->Name);
      }
    }
  }
  const char* supermodule = NULL;
  const char* supername = vtkWrapJavaScript_GetSuperClass(data, hinfo, &supermodule);
  const char* indent = "  ";
  if (supername == NULL)
  {
    fprintf(fp, "\n%semscripten::class_<%s>(\"%s\")", indent, classname, classname);
  }
  else
  {
    fprintf(fp, "\n%semscripten::class_<%s, emscripten::base<%s>>(\"%s\")", indent, classname,
      supername, classname);
  }
  // no constructors for abstract classes.
  if (!data->IsAbstract)
  {
    if (is_vtkobject && class_has_new)
    {
      fprintf(fp, "\n%s%s.smart_ptr<vtkSmartPointer<%s>>(\"vtkSmartPointer<%s>\")", indent, indent,
        classname, classname);
      fprintf(fp, "\n%s%s.constructor(&vtk::MakeAvtkSmartPointer<%s>)", indent, indent, classname);
    }
    else
    {
      // check if class has constructors and destructors
      int hasPublicConstructor = 0;
      int hasPublicDestructor = 0;
      for (int i = 0; i < data->NumberOfFunctions; ++i)
      {
        FunctionInfo* theFunc = data->Functions[i];
        if (theFunc->IsDeprecated)
        {
          // skip deprecated member functions.
          continue;
        }
        if (!theFunc->IsPublic)
        {
          continue;
        }
        // TODO: handle constructors with arguments.
        hasPublicConstructor |=
          (vtkWrap_IsConstructor(data, theFunc) && theFunc->NumberOfParameters == 0);
        hasPublicDestructor |= vtkWrap_IsDestructor(data, theFunc);
      }
      if (hasPublicConstructor && hasPublicDestructor)
      {
        // use std::shared_ptr
        fprintf(fp, "\n%s%s.smart_ptr<std::shared_ptr<%s>>(\"std::shared_ptr<%s>\")", indent,
          indent, classname, classname);
        fprintf(fp, "\n%s%s.constructor(&std::make_shared<%s>)", indent, indent, classname);
      }
    }
  }
  /* now output all the methods which are wrappable */
  vtkWrapJavaScript_GenerateMethods(fp, classname, data, file_info, hinfo, indent);
  fprintf(fp, ";\n}\n");

  /* create any constant types defined in the class */
  vtkWrapJavaScript_GenerateConstants(fp, module, classname, "  ", data);

  return 1;
}

// NOLINTEND(bugprone-unsafe-functions)
