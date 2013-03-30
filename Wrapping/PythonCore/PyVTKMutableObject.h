/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PyVTKMutableObject.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PyVTKMutableObject was created in Sep 2010 by David Gobbi.

  This class is a proxy for python int and float, it allows these objects
  to be passed to VTK methods that require a ref to a numeric type.
-----------------------------------------------------------------------*/

#ifndef __PyVTKMutableObject_h
#define __PyVTKMutableObject_h

#include "vtkWrappingPythonCoreModule.h" // For export macro
#include "vtkPython.h"
#include "vtkSystemIncludes.h"

// The PyVTKMutableObject is a wrapper around a PyObject of
// type int or float.
struct PyVTKMutableObject {
  PyObject_HEAD
  PyObject *value;
};

extern VTKWRAPPINGPYTHONCORE_EXPORT PyTypeObject PyVTKMutableObject_Type;

#define PyVTKMutableObject_Check(obj) ((obj)->ob_type == &PyVTKMutableObject_Type)

extern "C"
{
// Set the value held by a mutable object.  It steals the reference
// of the provided value.  Only float, long, and int are allowed.
// A return value of -1 indicates than an error occurred.
VTKWRAPPINGPYTHONCORE_EXPORT
int PyVTKMutableObject_SetValue(PyObject *self, PyObject *val);

// Get the value held by a mutable object.  A borrowed reference
// is returned.
VTKWRAPPINGPYTHONCORE_EXPORT
PyObject *PyVTKMutableObject_GetValue(PyObject *self);
}

#endif
