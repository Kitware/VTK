/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PyVTKClass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PyVTKClass was created in Oct 2000 by David Gobbi for VTK 3.2.
-----------------------------------------------------------------------*/

#ifndef __PyVTKClass_h
#define __PyVTKClass_h

#include "vtkPython.h"

// Prototype for static constructor
class vtkObjectBase;
typedef vtkObjectBase *(*vtknewfunc)();

// This is the VTK/Python 'class,' it contains the method list and a pointer
// to the superclass

struct PyVTKClass {
  PyObject_HEAD
  // the first six are common to PyClassObject
  PyObject *vtk_bases;
  PyObject *vtk_dict;
  PyObject *vtk_name;
  PyObject *vtk_getattr;
  PyObject *vtk_setattr;
  PyObject *vtk_delattr;
  // these are unique to the PyVTKClass
  PyObject *vtk_module;
  PyObject *vtk_doc;
  PyMethodDef *vtk_methods;
  vtknewfunc vtk_new;
  const char *vtk_cppname;
  const char *vtk_mangle;
};

extern VTK_PYTHON_EXPORT PyTypeObject PyVTKClass_Type;

#define PyVTKClass_Check(obj) ((obj)->ob_type == &PyVTKClass_Type)

extern "C"
{
VTK_PYTHON_EXPORT
PyObject *PyVTKClass_GetDict(PyObject *obj);

VTK_PYTHON_EXPORT
PyObject *PyVTKClass_New(vtknewfunc constructor, PyMethodDef *methods,
                         const char *classname, const char *modulename,
                         const char *pythonname, const char *manglename,
                         const char *docstring[], PyObject *base);
}

#endif
