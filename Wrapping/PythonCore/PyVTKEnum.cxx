/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PyVTKEnum.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "PyVTKEnum.h"
#include "vtkABINamespace.h"
#include "vtkPythonUtil.h"

#include <cstddef>

//------------------------------------------------------------------------------
// C API

//------------------------------------------------------------------------------
// Add a wrapped enum type
PyTypeObject* PyVTKEnum_Add(PyTypeObject* pytype, const char* name)
{
#if PY_VERSION_HEX < 0x030A0000
  // do not allow direct instantiation
  pytype->tp_new = nullptr;
#endif
  vtkPythonUtil::AddEnumToMap(pytype, name);
  return pytype;
}

//------------------------------------------------------------------------------
PyObject* PyVTKEnum_New(PyTypeObject* pytype, int val)
{
  // our enums are subtypes of Python's int() type
  PyObject* args = Py_BuildValue("(i)", val);
  PyObject* obj = PyLong_Type.tp_new(pytype, args, nullptr);
  Py_DECREF(args);
  return obj;
}
