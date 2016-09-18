/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PyVTKNamespace.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PyVTKNamespace was created in Nov 2014 by David Gobbi.

  This is a PyModule subclass for wrapping C++ namespaces.
-----------------------------------------------------------------------*/

#ifndef PyVTKNamespace_h
#define PyVTKNamespace_h

#include "vtkWrappingPythonCoreModule.h" // For export macro
#include "vtkPython.h"
#include "vtkSystemIncludes.h"

extern VTKWRAPPINGPYTHONCORE_EXPORT PyTypeObject PyVTKNamespace_Type;

#define PyVTKNamespace_Check(obj) \
  (Py_TYPE(obj) == &PyVTKNamespace_Type)

extern "C"
{
VTKWRAPPINGPYTHONCORE_EXPORT
PyObject *PyVTKNamespace_New(const char *name);

VTKWRAPPINGPYTHONCORE_EXPORT
PyObject *PyVTKNamespace_GetDict(PyObject *self);

VTKWRAPPINGPYTHONCORE_EXPORT
const char *PyVTKNamespace_GetName(PyObject *self);
}

#endif
