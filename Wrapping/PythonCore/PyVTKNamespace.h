// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/*-----------------------------------------------------------------------
  The PyVTKNamespace was created in Nov 2014 by David Gobbi.

  This is a PyModule subclass for wrapping C++ namespaces.
-----------------------------------------------------------------------*/

#ifndef PyVTKNamespace_h
#define PyVTKNamespace_h

#include "vtkABINamespace.h"
#include "vtkPython.h"
#include "vtkSystemIncludes.h"
#include "vtkWrappingPythonCoreModule.h" // For export macro

extern VTKWRAPPINGPYTHONCORE_EXPORT PyTypeObject PyVTKNamespace_Type;

#define PyVTKNamespace_Check(obj) (Py_TYPE(obj) == &PyVTKNamespace_Type)

extern "C"
{
  VTKWRAPPINGPYTHONCORE_EXPORT
  PyObject* PyVTKNamespace_New(const char* name);

  VTKWRAPPINGPYTHONCORE_EXPORT
  PyObject* PyVTKNamespace_GetDict(PyObject* self);

  VTKWRAPPINGPYTHONCORE_EXPORT
  const char* PyVTKNamespace_GetName(PyObject* self);
}

#endif
