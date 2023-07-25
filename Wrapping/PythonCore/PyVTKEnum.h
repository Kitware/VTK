// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef PyVTKEnum_h
#define PyVTKEnum_h

#include "vtkABINamespace.h"
#include "vtkPython.h"
#include "vtkSystemIncludes.h"
#include "vtkWrappingPythonCoreModule.h" // For export macro

extern "C"
{
  VTKWRAPPINGPYTHONCORE_EXPORT
  PyTypeObject* PyVTKEnum_Add(PyTypeObject* pytype, const char* name);

  VTKWRAPPINGPYTHONCORE_EXPORT
  PyObject* PyVTKEnum_New(PyTypeObject* pytype, int val);
}

#endif
