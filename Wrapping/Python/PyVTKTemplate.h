/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PyVTKTemplate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PyVTKTemplate was created in May 2011 by David Gobbi.

  This object is a container for instantiations of templated types.
  Essentially, it is a "dict" that accepts template args as keys,
  and provides the corresponding python type.
-----------------------------------------------------------------------*/

#ifndef __PyVTKTemplate_h
#define __PyVTKTemplate_h

#include "vtkPython.h"
#include "vtkSystemIncludes.h"

// The PyVTKTemplate is a wrapper around a dict.
struct PyVTKTemplate {
  PyObject_HEAD
  PyObject *dict;
  PyObject *doc;
  const char *name;
  const char *module;
};

extern VTK_PYTHON_EXPORT PyTypeObject PyVTKTemplate_Type;

#define PyVTKTemplate_Check(obj) ((obj)->ob_type == &PyVTKTemplate_Type)

extern "C"
{
VTK_PYTHON_EXPORT
PyObject *PyVTKTemplate_New(const char *name, const char *modulename,
                            const char *docstring[]);

VTK_PYTHON_EXPORT
int PyVTKTemplate_AddItem(PyObject *self, PyObject *val);
}

#endif
