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

/* for VTK_TYPE_USE_LONG_LONG vs VTK_TYPE_USE___INT64 */
#include "vtkConfigure.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


/* -------------------------------------------------------------------- */
/* This method adds constants defined in the file to the module */

void vtkWrapPython_AddConstant(
  FILE *fp, const char *indent, const char *dictvar, const char *objvar,
  ValueInfo *val)
{
  unsigned int valtype;
  const char *valstring;
  int objcreated = 0;

  valtype = (val->Type & VTK_PARSE_UNQUALIFIED_TYPE);
  valstring = val->Value;

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

  switch (valtype)
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
            "%s  PyDict_SetItemString(%s, (char *)\"%s\", %s);\n"
            "%s  Py_DECREF(%s);\n"
            "%s  }\n",
            indent, objvar, indent, indent, dictvar, val->Name, objvar,
            indent, objvar, indent);
    }
}
