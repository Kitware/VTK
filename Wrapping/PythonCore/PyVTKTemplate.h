// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/*-----------------------------------------------------------------------
  The PyVTKTemplate was created in May 2011 by David Gobbi.

  This object is a container for instantiations of templated types.
  Essentially, it is a "dict" that accepts template args as keys,
  and provides the corresponding python type.
-----------------------------------------------------------------------*/

#ifndef PyVTKTemplate_h
#define PyVTKTemplate_h

#include "vtkABINamespace.h"
#include "vtkPython.h"
#include "vtkSystemIncludes.h"
#include "vtkWrappingPythonCoreModule.h" // For export macro

// Heap type created with PyType_FromSpec; null until first PyVTKTemplate_New.
extern VTKWRAPPINGPYTHONCORE_EXPORT PyTypeObject* PyVTKTemplate_Type;

#define PyVTKTemplate_Check(obj) (Py_TYPE((PyObject*)(obj)) == PyVTKTemplate_Type)

extern "C"
{
  VTKWRAPPINGPYTHONCORE_EXPORT
  PyObject* PyVTKTemplate_New(const char* name, const char* docstring);

  VTKWRAPPINGPYTHONCORE_EXPORT
  int PyVTKTemplate_AddItem(PyObject* self, PyObject* val);
}

#endif
/* VTK-HeaderTest-Exclude: PyVTKTemplate.h */
