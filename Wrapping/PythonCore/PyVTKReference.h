/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PyVTKReference.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PyVTKReference was created in Sep 2010 by David Gobbi.

  This class is a proxy for python int and float, it allows these objects
  to be passed to VTK methods that require a ref to a numeric type.
-----------------------------------------------------------------------*/

#ifndef PyVTKReference_h
#define PyVTKReference_h

#include "vtkWrappingPythonCoreModule.h" // For export macro
#include "vtkPython.h"
#include "vtkSystemIncludes.h"

// The PyVTKReference is a wrapper around a PyObject of
// type int or float.
struct PyVTKReference {
  PyObject_HEAD
  PyObject *value;
};

extern VTKWRAPPINGPYTHONCORE_EXPORT PyTypeObject PyVTKReference_Type;
extern VTKWRAPPINGPYTHONCORE_EXPORT PyTypeObject PyVTKNumberReference_Type;
extern VTKWRAPPINGPYTHONCORE_EXPORT PyTypeObject PyVTKStringReference_Type;
extern VTKWRAPPINGPYTHONCORE_EXPORT PyTypeObject PyVTKTupleReference_Type;

#define PyVTKReference_Check(obj) \
  PyObject_TypeCheck(obj, &PyVTKReference_Type)

extern "C"
{
// Set the value held by a mutable object.  It steals the reference
// of the provided value.  Only float, long, and int are allowed.
// A return value of -1 indicates than an error occurred.
VTKWRAPPINGPYTHONCORE_EXPORT
int PyVTKReference_SetValue(PyObject *self, PyObject *val);

// Get the value held by a mutable object.  A borrowed reference
// is returned.
VTKWRAPPINGPYTHONCORE_EXPORT
PyObject *PyVTKReference_GetValue(PyObject *self);
}

#endif
