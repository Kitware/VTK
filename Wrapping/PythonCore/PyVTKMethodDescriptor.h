// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/*-----------------------------------------------------------------------
  The PyVTKMethodDescriptor was created in July 2015 by David Gobbi.

  Python's built-in method descriptor can only be used for non-static
  method calls.  VTK, however, has many methods where one signature of
  the method is static and another signature of the method is not. In
  order to wrap these methods, a custom method descriptor is needed.
-----------------------------------------------------------------------*/

#ifndef PyVTKMethodDescriptor_h
#define PyVTKMethodDescriptor_h

#include "vtkABINamespace.h"
#include "vtkPython.h"
#include "vtkSystemIncludes.h"
#include "vtkWrappingPythonCoreModule.h" // For export macro

// Our own descriptor object.  CPython's PyMethodDescrObject struct (accessed
// via the PyDescr_TYPE/PyDescr_NAME macros) is not available under the
// Limited API, so the data is stored in our own fields instead.
struct PyVTKMethodDescriptor
{
  PyObject_HEAD
  PyTypeObject* meth_class; // the type the method belongs to (exposed as __objclass__)
  PyObject* meth_name;      // interned method name (exposed as __name__)
  PyMethodDef* meth_def;    // the method definition
};

// The descriptor type is a heap type created with PyType_FromSpec; this pointer
// is null until PyVTKMethodDescriptor_ClassNew() has been called.
extern PyTypeObject* PyVTKMethodDescriptor_Type;

#define PyVTKMethodDescriptor_Check(obj) (Py_TYPE((PyObject*)(obj)) == PyVTKMethodDescriptor_Type)

// Create the method descriptor type object.  Returns false (with a Python
// exception set) on failure.  Safe to call more than once.
bool PyVTKMethodDescriptor_ClassNew();

extern "C"
{
  // Create a new method descriptor from a PyMethodDef.
  PyObject* PyVTKMethodDescriptor_New(PyTypeObject* cls, PyMethodDef* meth);
}

#endif
