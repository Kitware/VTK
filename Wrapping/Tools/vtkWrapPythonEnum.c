// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWrapPythonEnum.h"

#include "vtkWrap.h"
#include "vtkWrapText.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// NOLINTBEGIN(bugprone-unsafe-functions)

/* -------------------------------------------------------------------- */
/* check whether an enum type will be wrapped */
int vtkWrapPython_IsEnumWrapped(const HierarchyInfo* hinfo, const char* enumname)
{
  int rval = 0;
  const HierarchyEntry* entry;

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
void vtkWrapPython_MarkAllEnums(NamespaceInfo* contents, const HierarchyInfo* hinfo)
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

          if (vtkWrap_IsEnumMember(data, val) || vtkWrapPython_IsEnumWrapped(hinfo, val->Class))
          {
            val->IsEnum = 1;
          }
        }
      }
    }
  }
}

/* -------------------------------------------------------------------- */
/* generate a wrapped enum type (no anonymous enums, only named enums) */
void vtkWrapPython_AddEnumType(FILE* fp, const char* indent, const char* dictvar,
  const char* objvar, const char* scope, EnumInfo* cls)
{
  const ValueInfo* val;
  int j;

  if (cls->IsDeprecated)
  {
    fprintf(fp, "  /* Deprecated %s */\n", (cls->DeprecatedReason ? cls->DeprecatedReason : ""));
  }

  /* create the enum type (a heap type); TypeNew also registers it */
  fprintf(fp, "%s%s = (PyObject *)Py%s%s%s_TypeNew();\n", indent, objvar, (scope ? scope : ""),
    (scope ? "_" : ""), cls->Name);
  fprintf(fp,
    "%sif (%s)\n"
    "%s{\n",
    indent, objvar, indent);

  if (cls->NumberOfConstants)
  {
    fprintf(fp,
      "%s  // members of %s%s%s\n"
      "%s  PyObject *enumval;\n"
      "%s  typedef %s%s%s cxx_enum_type;\n"
      "%s  static const struct {\n"
      "%s    const char *name; cxx_enum_type value;\n"
      "%s  } constants[%d] = {\n",
      indent, (scope ? scope : ""), (scope ? "::" : ""), cls->Name, indent, indent,
      (scope ? scope : ""), (scope ? "::" : ""), cls->Name, indent, indent, indent,
      cls->NumberOfConstants);

    for (j = 0; j < cls->NumberOfConstants; j++)
    {
      val = cls->Constants[j];
      fprintf(fp, "%s    { \"%s%s\", cxx_enum_type::%s },%s\n", indent, val->Name,
        (vtkWrapText_IsPythonKeyword(val->Name) ? "_" : ""), val->Name,
        ((val->Attributes & VTK_PARSE_DEPRECATED) ? " /* deprecated */" : ""));
    }

    fprintf(fp,
      "%s  };\n"
      "\n",
      indent);

    fprintf(fp,
      "%s  for (int c = 0; c < %d; c++)\n"
      "%s  {\n"
      "%s    enumval = Py%s%s%s_FromEnum(constants[c].value);\n"
      "%s    if (enumval)\n"
      "%s    {\n"
      "%s      PyObject_SetAttrString(%s, constants[c].name, enumval);\n"
      "%s      Py_DECREF(enumval);\n"
      "%s    }\n"
      "%s  }\n",
      indent, cls->NumberOfConstants, indent, indent, (scope ? scope : ""), (scope ? "_" : ""),
      cls->Name, indent, indent, indent, objvar, indent, indent, indent);
  }

  /* add the enum type to the enclosing scope (a module/namespace dict or a
   * wrapped type); the scope holds the reference, ours is borrowed */
  fprintf(fp,
    "%s  vtkPythonUtil::AddToScope(%s, \"%s\", %s);\n"
    "%s}\n",
    indent, dictvar, cls->Name, objvar, indent);
}

/* -------------------------------------------------------------------- */
/* write out an enum type object */
void vtkWrapPython_GenerateEnumType(
  FILE* fp, const char* module, const char* classname, const EnumInfo* data)
{
  char enumname[512];
  char tpname[512];

  if (classname)
  {
    /* join with "_" for identifier, and with "." for type name */
    snprintf(enumname, sizeof(enumname), "%.200s_%.200s", classname, data->Name);
    snprintf(tpname, sizeof(tpname), "%.200s.%.200s", classname, data->Name);
  }
  else
  {
    snprintf(enumname, sizeof(enumname), "%.200s", data->Name);
    snprintf(tpname, sizeof(tpname), "%.200s", data->Name);
  }

  /* generate all functions and protocols needed for the type */

  /* generate the type as a heap type created with PyType_FromSpec (a subclass
   * of int), so that the wrapping can be built against the Limited API */
  fprintf(fp, "static PyTypeObject *Py%s_Type = nullptr;\n\n", enumname);

  fprintf(fp,
    "static PyType_Slot Py%s_Slots[] = {\n"
    "  { 0, nullptr }\n"
    "};\n"
    "\n"
    "static PyType_Spec Py%s_Spec = {\n"
    "  PYTHON_PACKAGE_SCOPE \"%s.%s\", // tp_name\n"
    "  0, // tp_basicsize (inherited from PyLong_Type)\n"
    "  0, // tp_itemsize\n"
    "  Py_TPFLAGS_DEFAULT\n"
    "#if PY_VERSION_HEX >= 0x030A0000\n"
    "    | Py_TPFLAGS_DISALLOW_INSTANTIATION\n"
    "#endif\n"
    "  , // tp_flags\n"
    "  Py%s_Slots\n"
    "};\n"
    "\n",
    enumname, enumname, module, tpname, enumname);

  /* create the enum type as a subclass of int (idempotent) */
  fprintf(fp,
    "static PyTypeObject *Py%s_TypeNew()\n"
    "{\n"
    "  if (Py%s_Type == nullptr)\n"
    "  {\n"
    "    PyObject *bases = PyTuple_Pack(1, (PyObject *)&PyLong_Type);\n"
    "    if (bases)\n"
    "    {\n"
    "      Py%s_Type = (PyTypeObject *)PyType_FromSpecWithBases(&Py%s_Spec, bases);\n"
    "      Py_DECREF(bases);\n"
    "      if (Py%s_Type)\n"
    "      {\n"
    "        PyVTKEnum_Add(Py%s_Type, \"%s\");\n"
    "      }\n"
    "    }\n"
    "  }\n"
    "  return Py%s_Type;\n"
    "}\n"
    "\n",
    enumname, enumname, enumname, enumname, enumname, enumname, tpname, enumname);

  /* conversion method: construct from enum value */
  fprintf(fp,
    "template<class T>\n"
    "static PyObject *Py%s_FromEnum(T val)\n"
    "{\n"
    "  return PyVTKEnum_New(Py%s_TypeNew(), static_cast<int>(val));\n"
    "}\n"
    "\n",
    enumname, enumname);
}

/* generate code that adds all public enum types to a python dict */
void vtkWrapPython_AddPublicEnumTypes(
  FILE* fp, const char* indent, const char* dictvar, const char* objvar, NamespaceInfo* data)
{
  char text[1024];
  const char* pythonname = data->Name;
  int i;

  if (data->Name)
  {
    /* convert C++ class names to a python-friendly format */
    vtkWrapText_PythonName(data->Name, text);
    pythonname = text;
  }

  for (i = 0; i < data->NumberOfEnums; i++)
  {
    if (!data->Enums[i]->IsExcluded && data->Enums[i]->Access == VTK_ACCESS_PUBLIC)
    {
      vtkWrapPython_AddEnumType(fp, indent, dictvar, objvar, pythonname, data->Enums[i]);
      fprintf(fp, "\n");
    }
  }
}

// NOLINTEND(bugprone-unsafe-functions)
