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
#include "vtkPythonUtil.h"

#include <cstddef>

//--------------------------------------------------------------------
// C API

//--------------------------------------------------------------------
// Add a wrapped enum type
PyTypeObject* PyVTKEnum_Add(PyTypeObject* pytype, const char* name)
{
  // do not allow direct instantiation
  pytype->tp_new = nullptr;
  vtkPythonUtil::AddEnumToMap(pytype, name);
  return pytype;
}

//--------------------------------------------------------------------
PyObject* PyVTKEnum_New(PyTypeObject* pytype, int val)
{
  // our enums are subtypes of Python's int() type
#ifdef VTK_PY3K
  PyObject* args = Py_BuildValue("(i)", val);
  PyObject* obj = PyLong_Type.tp_new(pytype, args, nullptr);
  Py_DECREF(args);
  return obj;
#else
  PyIntObject* self = PyObject_New(PyIntObject, pytype);
  self->ob_ival = val;
  return (PyObject*)self;
#endif
}
