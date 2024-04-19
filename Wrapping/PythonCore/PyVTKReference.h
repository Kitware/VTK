// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/*-----------------------------------------------------------------------
  The PyVTKReference was created in Sep 2010 by David Gobbi.

  This class is a proxy for python int and float, it allows these objects
  to be passed to VTK methods that require a ref to a numeric type.
-----------------------------------------------------------------------*/

#ifndef PyVTKReference_h
#define PyVTKReference_h

#include "vtkABINamespace.h"
#include "vtkPython.h"
#include "vtkSystemIncludes.h"
#include "vtkWrappingPythonCoreModule.h" // For export macro

// The PyVTKReference is a wrapper around a PyObject of
// type int or float.
struct PyVTKReference
{
  PyObject_HEAD
  PyObject* value;
};

extern PyTypeObject PyVTKReference_Type;
extern PyTypeObject PyVTKNumberReference_Type;
extern PyTypeObject PyVTKStringReference_Type;
extern PyTypeObject PyVTKTupleReference_Type;

#define PyVTKReference_Check(obj) PyObject_TypeCheck(obj, &PyVTKReference_Type)

extern "C"
{
  // Set the value held by a mutable object.  It steals the reference
  // of the provided value.  Only float, long, and int are allowed.
  // A return value of -1 indicates than an error occurred.
  int PyVTKReference_SetValue(PyObject* self, PyObject* val);

  // Get the value held by a mutable object.  A borrowed reference
  // is returned.
  PyObject* PyVTKReference_GetValue(PyObject* self);
}

#endif
