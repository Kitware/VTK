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
/* generate a wrapped enum type */
void vtkWrapPython_AddEnumType(
  FILE *fp, const char *indent, const char *dictvar, const char *objvar,
  const char *scope, EnumInfo *cls)
{
  /* Don't add anonymous enums */
  fprintf(fp,
          "%sPyType_Ready(&Py%s%s%s_Type);\n"
          "%sPy%s%s%s_Type.tp_new = NULL;\n"
          "\n",
          indent, (scope ? scope : ""), (scope ? "_" : ""), cls->Name,
          indent, (scope ? scope : ""), (scope ? "_" : ""), cls->Name);
  fprintf(fp,
          "%s%s = (PyObject *)&Py%s%s%s_Type;\n"
          "%sif (%s && PyDict_SetItemString(%s, (char *)\"%s\", %s) != 0)\n"
          "%s  {\n"
          "%s  Py_DECREF(%s);\n"
          "%s  }\n",
          indent, objvar,
          (scope ? scope : ""), (scope ? "_" : ""), cls->Name,
          indent, objvar, dictvar, cls->Name, objvar,
          indent,
          indent, objvar,
          indent);
}

/* -------------------------------------------------------------------- */
/* write out an enum type object */
void vtkWrapPython_GenerateEnumType(
  FILE *fp, const char *classname, EnumInfo *data)
{
  char enumname[1024];
  size_t classnamelen = 0;

  if (classname)
    {
    classnamelen = strlen(classname);
    strcpy(enumname, classname);
    enumname[classnamelen] = '_';
    strcpy(enumname+classnamelen+1, data->Name);
    }
  else
    {
    strcpy(enumname, data->Name);
    }

  /* forward declaration of the type object */
  fprintf(fp,
    "#ifndef DECLARED_Py%s_Type\n"
    "extern %s PyTypeObject Py%s_Type;\n"
    "#define DECLARED_Py%s_Type\n"
    "#endif\n"
    "\n",
    enumname, "VTK_PYTHON_EXPORT", enumname, enumname);

  /* generate all functions and protocols needed for the type */

  /* generate the TypeObject */
  fprintf(fp,
    "PyTypeObject Py%s_Type = {\n"
    "  PyObject_HEAD_INIT(&PyType_Type)\n"
    "  0,\n"
    "  (char*)\"%s\", // tp_name\n"
    "  sizeof(PyIntObject), // tp_basicsize\n"
    "  0, // tp_itemsize\n"
    "  0, // tp_dealloc\n"
    "  0, // tp_print\n"
    "  0, // tp_getattr\n"
    "  0, // tp_setattr\n"
    "  0, // tp_compare\n"
    "  0, // tp_repr\n",
    enumname, data->Name);

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
    "  VTK_WRAP_PYTHON_SUPRESS_UNINITIALIZED\n"
    "};\n"
    "\n");

  /* conversion method: construct from enum value */
  fprintf(fp,
    "PyObject *Py%s_FromEnum(int val)\n"
    "{\n"
    "  PyIntObject *self = PyObject_New(PyIntObject,\n"
    "    &Py%s_Type);\n"
    "  self->ob_ival = val;\n"
    "  return (PyObject *)self;\n"
    "}\n"
    "\n",
    enumname,
    enumname);
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
