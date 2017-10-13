/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PyVTKSpecialObject.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PyVTKSpecialObject was created in Feb 2001 by David Gobbi.
  The PyVTKSpecialType class was created in April 2010 by David Gobbi.
-----------------------------------------------------------------------*/

#ifndef PyVTKSpecialObject_h
#define PyVTKSpecialObject_h

#include "vtkWrappingPythonCoreModule.h" // For export macro
#include "vtkPython.h"
#include "vtkSystemIncludes.h"

// This for objects not derived from vtkObjectBase

// Prototypes for per-type copy, delete, and print funcs

// copy the object and return the copy
typedef void *(*vtkcopyfunc)(const void *);

// Because the PyTypeObject can't hold all the typing information that we
// need, we use this PyVTKSpecialType class to hold a bit of extra info.
class VTKWRAPPINGPYTHONCORE_EXPORT PyVTKSpecialType
{
public:
  PyVTKSpecialType() :
    py_type(nullptr), vtk_methods(nullptr), vtk_constructors(nullptr), vtk_copy(nullptr) {}

  PyVTKSpecialType(
    PyTypeObject *typeobj, PyMethodDef *cmethods, PyMethodDef *ccons,
    vtkcopyfunc copyfunc);

  // general information
  PyTypeObject *py_type;
  PyMethodDef *vtk_methods;
  PyMethodDef *vtk_constructors;
  // copy an object
  vtkcopyfunc vtk_copy;
};

// The PyVTKSpecialObject is very lightweight.  All special VTK types
// that are wrapped in VTK use this struct, they do not define their
// own structs.
struct PyVTKSpecialObject {
  PyObject_HEAD
  PyVTKSpecialType *vtk_info;
  void *vtk_ptr;
  long vtk_hash;
};

extern "C"
{
VTKWRAPPINGPYTHONCORE_EXPORT
PyVTKSpecialType *PyVTKSpecialType_Add(PyTypeObject *pytype,
  PyMethodDef *methods, PyMethodDef *constructors, vtkcopyfunc copyfunc);

VTKWRAPPINGPYTHONCORE_EXPORT
PyObject *PyVTKSpecialObject_New(const char *classname, void *ptr);

VTKWRAPPINGPYTHONCORE_EXPORT
PyObject *PyVTKSpecialObject_CopyNew(const char *classname, const void *ptr);

VTKWRAPPINGPYTHONCORE_EXPORT
PyObject *PyVTKSpecialObject_Repr(PyObject *self);

VTKWRAPPINGPYTHONCORE_EXPORT
PyObject *PyVTKSpecialObject_SequenceString(PyObject *self);
}

#endif
