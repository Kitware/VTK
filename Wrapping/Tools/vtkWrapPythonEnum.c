/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapPythonEnum.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkWrapPythonEnum.h"

#include "vtkWrap.h"
#include "vtkWrapText.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* -------------------------------------------------------------------- */
/* check whether an enum type will be wrapped */
int vtkWrapPython_IsEnumWrapped(
  HierarchyInfo *hinfo, const char *enumname)
{
  int rval = 0;
  HierarchyEntry *entry;

  if (hinfo && enumname)
  {
    entry = vtkParseHierarchy_FindEntry(hinfo, enumname);
    if (entry && entry->IsEnum)
    {
      rval = 1;
    }
  }

  return rval;
}

/* -------------------------------------------------------------------- */
/* find and mark all enum parameters by setting IsEnum=1 */
void vtkWrapPython_MarkAllEnums(
  NamespaceInfo *contents, HierarchyInfo *hinfo)
{
  FunctionInfo *currentFunction;
  int i, j, n, m, ii, nn;
  ClassInfo *data;
  ValueInfo *val;

  nn = contents->NumberOfClasses;
  for (ii = 0; ii < nn; ii++)
  {
    data = contents->Classes[ii];
    n = data->NumberOfFunctions;
    for (i = 0; i < n; i++)
    {
      currentFunction = data->Functions[i];
      if (currentFunction->Access == VTK_ACCESS_PUBLIC)
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

          if (vtkWrap_IsEnumMember(data, val) ||
              vtkWrapPython_IsEnumWrapped(hinfo, val->Class))
          {
            val->IsEnum = 1;
          }
        }
      }
    }
  }
}

/* -------------------------------------------------------------------- */
/* generate a wrapped enum type */
void vtkWrapPython_AddEnumType(
  FILE *fp, const char *indent, const char *dictvar, const char *objvar,
  const char *scope, EnumInfo *cls)
{
  /* Don't add anonymous enums */
  fprintf(fp,
          "%sPyType_Ready(&Py%s%s%s_Type);\n"
          "%sPy%s%s%s_Type.tp_new = nullptr;\n"
          "%svtkPythonUtil::AddEnumToMap(&Py%s%s%s_Type);\n"
          "\n",
          indent, (scope ? scope : ""), (scope ? "_" : ""), cls->Name,
          indent, (scope ? scope : ""), (scope ? "_" : ""), cls->Name,
          indent, (scope ? scope : ""), (scope ? "_" : ""), cls->Name);
  fprintf(fp,
          "%s%s = (PyObject *)&Py%s%s%s_Type;\n"
          "%sif (PyDict_SetItemString(%s, \"%s\", %s) != 0)\n"
          "%s{\n"
          "%s  Py_DECREF(%s);\n"
          "%s}\n",
          indent, objvar,
          (scope ? scope : ""), (scope ? "_" : ""), cls->Name,
          indent, dictvar, cls->Name, objvar,
          indent,
          indent, objvar,
          indent);
}

/* -------------------------------------------------------------------- */
/* write out an enum type object */
void vtkWrapPython_GenerateEnumType(
  FILE *fp, const char *module, const char *classname, EnumInfo *data)
{
  char enumname[512];
  char tpname[512];

  if (classname)
  {
    /* join with "_" for identifier, and with "." for type name */
    sprintf(enumname, "%.200s_%.200s", classname, data->Name);
    sprintf(tpname, "%.200s.%.200s", classname, data->Name);
  }
  else
  {
    sprintf(enumname, "%.200s", data->Name);
    sprintf(tpname, "%.200s", data->Name);
  }

  /* generate all functions and protocols needed for the type */

  /* generate the TypeObject */
  fprintf(fp,
    "static PyTypeObject Py%s_Type = {\n"
    "  PyVarObject_HEAD_INIT(&PyType_Type, 0)\n"
    "  \"%sPython.%s\", // tp_name\n"
    "  sizeof(PyIntObject), // tp_basicsize\n"
    "  0, // tp_itemsize\n"
    "  nullptr, // tp_dealloc\n"
    "  nullptr, // tp_print\n"
    "  nullptr, // tp_getattr\n"
    "  nullptr, // tp_setattr\n"
    "  nullptr, // tp_compare\n"
    "  nullptr, // tp_repr\n",
    enumname, module, tpname);

  fprintf(fp,
    "  nullptr, // tp_as_number\n"
    "  nullptr, // tp_as_sequence\n"
    "  nullptr, // tp_as_mapping\n"
    "  nullptr, // tp_hash\n"
    "  nullptr, // tp_call\n"
    "  nullptr, // tp_str\n"
    "  nullptr, // tp_getattro\n"
    "  nullptr, // tp_setattro\n"
    "  nullptr, // tp_as_buffer\n"
    "  Py_TPFLAGS_DEFAULT, // tp_flags\n"
    "  nullptr, // tp_doc\n"
    "  nullptr, // tp_traverse\n"
    "  nullptr, // tp_clear\n"
    "  nullptr, // tp_richcompare\n"
    "  0, // tp_weaklistoffset\n");

  fprintf(fp,
    "  nullptr, // tp_iter\n"
    "  nullptr, // tp_iternext\n"
    "  nullptr, // tp_methods\n"
    "  nullptr, // tp_members\n"
    "  nullptr, // tp_getset\n"
    "  &PyInt_Type, // tp_base\n"
    "  nullptr, // tp_dict\n"
    "  nullptr, // tp_descr_get\n"
    "  nullptr, // tp_descr_set\n"
    "  0, // tp_dictoffset\n"
    "  nullptr, // tp_init\n"
    "  nullptr, // tp_alloc\n"
    "  nullptr, // tp_new\n"
    "  PyObject_Del, // tp_free\n"
    "  nullptr, // tp_is_gc\n");

  /* fields set by python itself */
  fprintf(fp,
    "  nullptr, // tp_bases\n"
    "  nullptr, // tp_mro\n"
    "  nullptr, // tp_cache\n"
    "  nullptr, // tp_subclasses\n"
    "  nullptr, // tp_weaklist\n");

  /* internal struct members */
  fprintf(fp,
    "  VTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED\n"
    "};\n"
    "\n");

  /* conversion method: construct from enum value */
  /* XXX the PY3K implementation is horridly inefficient */
  fprintf(fp,
    "PyObject *Py%s_FromEnum(int val)\n"
    "{\n"
    "#ifdef VTK_PY3K\n"
    "  PyObject *args = Py_BuildValue(\"(i)\", val);\n"
    "  PyObject *obj = PyLong_Type.tp_new(&Py%s_Type, args, nullptr);\n"
    "  Py_DECREF(args);\n"
    "  return obj;\n"
    "#else\n"
    "  PyIntObject *self = PyObject_New(PyIntObject,\n"
    "    &Py%s_Type);\n"
    "  self->ob_ival = val;\n"
    "  return (PyObject *)self;\n"
    "#endif\n"
    "}\n"
    "\n",
    enumname, enumname, enumname);
}

/* generate code that adds all public enum types to a python dict */
void vtkWrapPython_AddPublicEnumTypes(
  FILE *fp, const char *indent, const char *dictvar, const char *objvar,
  NamespaceInfo *data)
{
  char text[1024];
  const char *pythonname = data->Name;
  int i;

  if (data->Name)
  {
    /* convert C++ class names to a python-friendly format */
    vtkWrapText_PythonName(data->Name, text);
    pythonname = text;
  }

  for (i = 0; i < data->NumberOfEnums; i++)
  {
    if (data->Enums[i]->Access == VTK_ACCESS_PUBLIC)
    {
      vtkWrapPython_AddEnumType(
        fp, indent, dictvar, objvar, pythonname, data->Enums[i]);
      fprintf(fp, "\n");
    }
  }
}
