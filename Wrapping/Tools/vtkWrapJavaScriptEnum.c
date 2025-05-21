/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapJavaScriptEnum.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWrapJavaScriptEnum.h"

#include "vtkParseData.h"
#include "vtkWrap.h"
#include "vtkWrapText.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// NOLINTBEGIN(bugprone-unsafe-functions)

/* -------------------------------------------------------------------- */
/* check whether an enum type will be wrapped */
int vtkWrapJavaScript_IsEnumWrapped(HierarchyInfo* hinfo, const char* enumname)
{
  int rval = 0;
  HierarchyEntry* entry;

  if (hinfo && enumname)
  {
    entry = vtkParseHierarchy_FindEntry(hinfo, enumname);
    if (entry && entry->IsEnum && !vtkParseHierarchy_GetProperty(entry, "WRAPEXCLUDE"))
    {
      rval = 1;
    }
  }

  return rval;
}

/* -------------------------------------------------------------------- */
/* find and mark all enum parameters by setting IsEnum=1 */
void vtkWrapJavaScript_MarkAllEnums(NamespaceInfo* contents, HierarchyInfo* hinfo)
{
  FunctionInfo* currentFunction;
  int i, j, n, m, ii, nn;
  ClassInfo* data;
  ValueInfo* val;

  nn = contents->NumberOfClasses;
  for (ii = 0; ii < nn; ii++)
  {
    data = contents->Classes[ii];
    n = data->NumberOfFunctions;
    for (i = 0; i < n; i++)
    {
      currentFunction = data->Functions[i];
      if (!currentFunction->IsExcluded && currentFunction->Access == VTK_ACCESS_PUBLIC)
      {
        /* we start with the return value */
        val = currentFunction->ReturnValue;
        m = vtkWrap_CountWrappedParameters(currentFunction);

        /* the -1 is for the return value */
        for (j = (val ? -1 : 0); j < m; j++)
        {
          if (j >= 0)
          {
            val = currentFunction->Parameters[j];
          }

          if (vtkWrap_IsEnumMember(data, val) || vtkWrapJavaScript_IsEnumWrapped(hinfo, val->Class))
          {
            val->IsEnum = 1;
          }
        }
      }
    }
  }
}

/* -------------------------------------------------------------------- */
void vtkWrapJavaScript_GenerateEnumTypes(
  FILE* fp, const char* modulename, const char* classname, const char* indent, NamespaceInfo* data)
{
  const char* scope = classname;
  // Open an EMSCRIPTEN_BINDINGS block for enums.
  if (data->NumberOfEnums > 0)
  {
    // Add new binding block for anonymous enums in this file under the label
    // 'ModuleName_MainClassName_enums'
    if (classname)
    {
      fprintf(fp, "EMSCRIPTEN_BINDINGS(%s_%s_class_enums) {", modulename, classname);
    }
    else if (data->NumberOfClasses > 0 && data->Classes[0]->Name)
    {
      fprintf(fp, "EMSCRIPTEN_BINDINGS(%s_%s_enums) {", modulename, data->Classes[0]->Name);
    }
    else
    {
      fprintf(fp, "EMSCRIPTEN_BINDINGS(%s_enums) {", modulename);
    }
  }
  for (int i = 0; i < data->NumberOfEnums; ++i)
  {
    if (!data->Enums[i]->IsExcluded && data->Enums[i]->Access == VTK_ACCESS_PUBLIC)
    {
      EnumInfo* enumInfo = data->Enums[i];
      char enumname[512];
      char tpname[512];
      int found = 0;
      if (enumInfo->IsDeprecated)
      {
        // skip deprecated enums
        continue;
      }

      /* check to make sure there won't be a name conflict between an
         enum type and some other class member, it happens specifically
         for vtkImplicitBoolean which has a variable and enum type both
         with the name OperationType */
      for (int j = 0; j < data->NumberOfVariables && !found; j++)
      {
        found = (strcmp(data->Variables[j]->Name, enumInfo->Name) == 0);
      }
      if (found && scope)
      {
        // enums which are members of a class that share a name similar to a protected member of the
        // same class.
        snprintf(enumname, sizeof(enumname), "%.200s", scope);
        snprintf(tpname, sizeof(tpname), "%.200s_%.200s", scope, enumInfo->Name);
      }
      else if (scope)
      {
        // enums which are members of a class.
        snprintf(enumname, sizeof(enumname), "%.200s::%.200s", scope, enumInfo->Name);
        snprintf(tpname, sizeof(tpname), "%.200s_%.200s", scope, enumInfo->Name);
      }
      else // found or not scope
      {
        // anonymous enums
        snprintf(enumname, sizeof(enumname), "%.200s", enumInfo->Name);
        snprintf(tpname, sizeof(tpname), "%.200s", enumInfo->Name);
      }
      if (enumInfo->NumberOfConstants)
      {
        if (!found)
        {
          // C++ style scoped enums are parsed and available on the EnumInfo.
          fprintf(fp, "\n%semscripten::enum_<%s>(\"%s\")", indent, enumname, tpname);
          for (int j = 0; j < enumInfo->NumberOfConstants; ++j)
          {
            fprintf(fp, "\n%s  .value(\"%s\", %s::%s)", indent, enumInfo->Constants[j]->Name,
              enumname, enumInfo->Constants[j]->Name);
          }
          fprintf(fp, ";");
        }
        else
        {
          for (int j = 0; j < enumInfo->NumberOfConstants; ++j)
          {
            fprintf(fp, "\n%semscripten::constant(\"%s_%s\", static_cast<int>(%s::%s));", indent,
              tpname, enumInfo->Constants[j]->Name, enumname, enumInfo->Constants[j]->Name);
          }
        }
      }
    }
  }
  // Close the EMSCRIPTEN_BINDINGS block for enums.
  if (data->NumberOfEnums > 0)
  {
    fprintf(fp, "\n}\n");
  }
}

// NOLINTEND(bugprone-unsafe-functions)
