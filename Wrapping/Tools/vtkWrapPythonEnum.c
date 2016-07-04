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
          "%sPy%s%s%s_Type.tp_new = NULL;\n"
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
    "  0, // tp_dealloc\n"
    "  0, // tp_print\n"
    "  0, // tp_getattr\n"
    "  0, // tp_setattr\n"
    "  0, // tp_compare\n"
    "  0, // tp_repr\n",
    enumname, module, tpname);

  fprintf(fp,
    "  0, // tp_as_number\n"
    "  0, // tp_as_sequence\n"
    "  0, // tp_as_mapping\n"
    "  0, // tp_hash\n"
    "  0, // tp_call\n"
    "  0, // tp_str\n"
    "  0, // tp_getattro\n"
    "  0, // tp_setattro\n"
    "  0, // tp_as_buffer\n"
    "  Py_TPFLAGS_DEFAULT, // tp_flags\n"
    "  0, // tp_doc\n"
    "  0, // tp_traverse\n"
    "  0, // tp_clear\n"
    "  0, // tp_richcompare\n"
    "  0, // tp_weaklistoffset\n");

  fprintf(fp,
    "  0, // tp_iter\n"
    "  0, // tp_iternext\n"
    "  0, // tp_methods\n"
    "  0, // tp_members\n"
    "  0, // tp_getset\n"
    "  &PyInt_Type, // tp_base\n"
    "  0, // tp_dict\n"
    "  0, // tp_descr_get\n"
    "  0, // tp_descr_set\n"
    "  0, // tp_dictoffset\n"
    "  0, // tp_init\n"
    "  0, // tp_alloc\n"
    "  0, // tp_new\n"
    "  PyObject_Del, // tp_free\n"
    "  0, // tp_is_gc\n");

  /* fields set by python itself */
  fprintf(fp,
    "  0, // tp_bases\n"
    "  0, // tp_mro\n"
    "  0, // tp_cache\n"
    "  0, // tp_subclasses\n"
    "  0, // tp_weaklist\n");

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
    "  PyObject *obj = PyLong_Type.tp_new(&Py%s_Type, args, NULL);\n"
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
  int i;

  for (i = 0; i < data->NumberOfEnums; i++)
  {
    if (data->Enums[i]->Access == VTK_ACCESS_PUBLIC)
    {
      vtkWrapPython_AddEnumType(
        fp, indent, dictvar, objvar, data->Name, data->Enums[i]);
      fprintf(fp, "\n");
    }
  }
}
