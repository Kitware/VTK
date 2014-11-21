/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapPythonConstant.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkWrapPythonConstant.h"
#include "vtkWrap.h"

/* for VTK_TYPE_USE_LONG_LONG vs VTK_TYPE_USE___INT64 */
#include "vtkConfigure.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* -------------------------------------------------------------------- */
/* The scope, attrib, and valstring parameters are optional and can be
   set to NULL.

   The "scope" is a namespace to use for enum constants, it is ignored
   if null.

   The "attrib" is the attribute to set in the dictionary, if null then
   val->Name is used as the attribute name.

   The "attribval" is the value to set the attribute to, if null then
   val->Value is used.
*/
void vtkWrapPython_AddConstantHelper(
  FILE *fp, const char *indent, const char *dictvar, const char *objvar,
  const char *scope, const char *attrib, const char *attribval,
  ValueInfo *val)
{
  unsigned int valtype;
  const char *valname;
  const char *valstring;
  int objcreated = 0;

  valtype = (val->Type & VTK_PARSE_UNQUALIFIED_TYPE);
  valname = val->Name;
  valstring = attribval;
  if (valstring == 0)
    {
    valstring = val->Value;
    }

  if (valtype == 0 && (valstring == NULL || valstring[0] == '\0'))
    {
    valtype = VTK_PARSE_VOID;
    }
  else if (strcmp(valstring, "NULL") == 0)
    {
    valtype = VTK_PARSE_VOID;
    }

  if (valtype == 0 || val->Name == NULL)
    {
    return;
    }

  if (val->IsEnum)
    {
    if (val->Class && val->Class[0] != '\0' &&
        strcmp(val->Class, "int") != 0)
      {
      fprintf(fp,
              "%s%s = Py%s%s%s_FromEnum(%s%s%s);\n",
              indent, objvar,
              (scope ? scope : ""), (scope ? "_" : ""), val->Class,
              ((scope && !attribval) ? scope : ""),
              ((scope && !attribval) ? "::" : ""),
              (!attribval ? valname : attribval));
      objcreated = 1;
      }
    else
      {
      fprintf(fp,
              "%s%s = PyInt_FromLong(%s%s%s);\n",
              indent, objvar,
              ((scope && !attribval) ? scope : ""),
              ((scope && !attribval) ? "::" : ""),
              (!attribval ? valname : attribval));
      objcreated = 1;
      }
    }
  else switch (valtype)
    {
    case VTK_PARSE_VOID:
      fprintf(fp,
              "%sPy_INCREF(Py_None);\n"
              "%s%s = Py_None;\n",
              indent, indent, objvar);
      objcreated = 1;
      break;

    case VTK_PARSE_CHAR_PTR:
      fprintf(fp,
              "%s%s = PyString_FromString((char *)(%s));\n",
              indent, objvar, valstring);
      objcreated = 1;
      break;

    case VTK_PARSE_FLOAT:
    case VTK_PARSE_DOUBLE:
      fprintf(fp,
              "%s%s = PyFloat_FromDouble(%s);\n",
              indent, objvar, valstring);
      objcreated = 1;
      break;

    case VTK_PARSE_LONG:
    case VTK_PARSE_INT:
    case VTK_PARSE_SHORT:
    case VTK_PARSE_UNSIGNED_SHORT:
    case VTK_PARSE_CHAR:
    case VTK_PARSE_SIGNED_CHAR:
    case VTK_PARSE_UNSIGNED_CHAR:
      fprintf(fp,
              "%s%s = PyInt_FromLong(%s);\n",
              indent, objvar, valstring);
      objcreated = 1;
      break;

    case VTK_PARSE_UNSIGNED_INT:
      fprintf(fp,
              "#if VTK_SIZEOF_INT < VTK_SIZEOF_LONG\n"
              "%s%s = PyInt_FromLong(%s);\n"
              "#else\n"
              "%s%s = PyLong_FromUnsignedLong(%s);\n"
              "#endif\n",
              indent, objvar, valstring, indent, objvar, valstring);
      objcreated = 1;
      break;

    case VTK_PARSE_UNSIGNED_LONG:
      fprintf(fp,
              "%s%s = PyLong_FromUnsignedLong(%s);\n",
              indent, objvar, valstring);
      objcreated = 1;
      break;

#ifndef VTK_PYTHON_NO_LONG_LONG
#ifdef VTK_TYPE_USE___INT64
    case VTK_PARSE___INT64:
      fprintf(fp,
              "%s%s = PyLong_FromLongLong(%s);\n",
              indent, objvar, valstring);
      objcreated = 1;
      break;

    case VTK_PARSE_UNSIGNED___INT64:
      fprintf(fp,
              "%s%s = PyLong_FromUnsignedLongLong(%s);\n",
              indent, objvar, valstring);
      objcreated = 1;
      break;
#endif

#ifdef VTK_TYPE_USE_LONG_LONG
    case VTK_PARSE_LONG_LONG:
      fprintf(fp,
              "%s%s = PyLong_FromLongLong(%s);\n",
              indent, objvar, valstring);
      objcreated = 1;
      break;

    case VTK_PARSE_UNSIGNED_LONG_LONG:
      fprintf(fp,
              "%s%s = PyLong_FromUnsignedLongLong(%s);\n",
              indent, objvar, valstring);
      objcreated = 1;
      break;
#endif
#endif

    case VTK_PARSE_BOOL:
      fprintf(fp,
              "#if PY_VERSION_HEX >= 0x02030000\n"
              "%s%s = PyBool_FromLong((long)(%s));\n"
              "#else\n"
              "%s%s = PyInt_FromLong((long)(%s));\n"
              "#endif\n",
              indent, objvar, valstring, indent, objvar, valstring);
      objcreated = 1;
      break;
    }

  if (objcreated)
    {
    fprintf(fp,
            "%sif (%s)\n"
            "%s  {\n"
            "%s  PyDict_SetItemString(%s, (char *)%s%s%s, %s);\n"
            "%s  Py_DECREF(%s);\n"
            "%s  }\n",
            indent, objvar, indent, indent, dictvar,
            (attrib ? "" : "\""), (attrib ? attrib : valname),
            (attrib ? "" : "\""), objvar,
            indent, objvar, indent);
    }
}

/* -------------------------------------------------------------------- */
/* Add all constants defined in the namespace to the module */

void vtkWrapPython_AddPublicConstants(
  FILE *fp, const char *indent, const char *dictvar, const char *objvar,
  NamespaceInfo *data)
{
  const char *nextindent = "        ";
  ValueInfo *val;
  ValueInfo *firstval;
  const char *scope;
  int scopeType, scopeValue;
  unsigned int valtype;
  const char *typeName;
  const char *tname;
  int j = 0;
  int count, k, i;
  size_t l, m;

  /* get the next indent to use */
  l = strlen(indent);
  m = strlen(nextindent);
  if (m > l + 2)
    {
    nextindent += m - l - 2;
    }

  /* get the name of the namespace, or NULL if global */
  scope = data->Name;
  if (scope && scope[0] == '\0')
    {
    scope = 0;
    }

  /* go through the constants, collecting them by type */
  while (j < data->NumberOfConstants)
    {
    val = data->Constants[j];
    if (val->Access != VTK_ACCESS_PUBLIC)
      {
      j++;
      continue;
      }

    /* write a single constant if not numerical */
    if (j+1 == data->NumberOfConstants ||
        val->Type != data->Constants[j+1]->Type ||
        !vtkWrap_IsScalar(val) ||
        (!val->IsEnum && !vtkWrap_IsNumeric(val)))
      {
      vtkWrapPython_AddConstant(
        fp, indent, dictvar, objvar, scope, val);
      j++;
      continue;
      }

    /* get important information about the value */
    valtype = val->Type;
    typeName = (val->IsEnum ? val->Class : vtkWrap_GetTypeName(val));
    scopeType = (scope && val->IsEnum && strcmp(typeName, "int") != 0);
    scopeValue = (scope && val->IsEnum);

    /* count a series of constants of the same type */
    firstval = val;
    count = 0;
    for (k = j; k < data->NumberOfConstants; k++)
      {
      val = data->Constants[k];
      if (val->Access == VTK_ACCESS_PUBLIC)
        {
        tname = (val->IsEnum ? val->Class : vtkWrap_GetTypeName(val));
        if (val->Type != valtype || strcmp(tname, typeName) != 0)
          {
          break;
          }
        count++;
        }
      }

    /* if no constants to generate, then continue */
    if (count == 0)
      {
      j = k;
      continue;
      }

    /* check to make sure there won't be a name conflict between an
       enum type and some other class member, it happens specifically
       for vtkImplicitBoolean which has a variable and enum type both
       with the name OperationType */
    if (scopeType)
      {
      int conflict = 0;
      for (i = 0; i < data->NumberOfVariables && !conflict; i++)
        {
        conflict = (strcmp(data->Variables[i]->Name, typeName) == 0);
        }
      if (conflict)
        {
        valtype = VTK_PARSE_INT;
        typeName = "int";
        scopeType = 0;
        }
      }

    /* generate the code */
    fprintf(fp,
      "%sfor (int c = 0; c < %d; c++)\n"
      "%s  {\n",
      indent, count, indent);

    if (scopeType)
      {
      fprintf(fp,
        "%s  typedef %s::%s cxx_enum_type;\n\n",
        indent, scope, typeName);
      }

    fprintf(fp,
      "%s  static const struct { const char *name; %s value; }\n"
      "%s    constants[%d] = {\n",
      indent, (scopeType ? "cxx_enum_type" : typeName),
      indent, count);

    while (j < k)
      {
      val = data->Constants[j++];
      if (val->Access == VTK_ACCESS_PUBLIC)
        {
        fprintf(fp,
          "%s      { \"%s\", %s%s%s },\n",
          indent, val->Name,
          (scopeValue ? scope : ""), (scopeValue ? "::" : ""),
          (val->IsEnum ? val->Name : val->Value));
        }
      }

    fprintf(fp,
      "%s    };\n"
      "\n",
      indent);

    vtkWrapPython_AddConstantHelper(
      fp, nextindent, dictvar, objvar, scope,
      "constants[c].name", "constants[c].value", firstval);

    fprintf(fp,
      "%s  }\n\n",
      indent);
    }
}

/* -------------------------------------------------------------------- */
/* This method adds one constant defined in the file to the module */

void vtkWrapPython_AddConstant(
  FILE *fp, const char *indent, const char *dictvar, const char *objvar,
  const char *scope, ValueInfo *val)
{
  vtkWrapPython_AddConstantHelper(
    fp, indent, dictvar, objvar, scope, NULL, NULL, val);
}
