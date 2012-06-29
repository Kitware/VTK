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

#ifndef __PyVTKSpecialObject_h
#define __PyVTKSpecialObject_h

#include "vtkWrappingPythonModule.h" // For export macro
#include "vtkPython.h"
#include "vtkSystemIncludes.h"

// This for objects not derived from vtkObjectBase

// Prototypes for per-type copy, delete, and print funcs

// copy the object and return the copy
typedef void *(*PyVTKSpecialCopyFunc)(const void *);

// Unlike PyVTKObject, for special VTK types there is no "meta-type"
// like PyVTKClass.  Special VTK types use PyTypeObject instead, in a
// manner more in line with most python extension packages.  However,
// since PyTypeObject can't hold all the typing information that we
// need, we use this PyVTKSpecialType class to hold a bit of extra info.
class VTKWRAPPINGPYTHON_EXPORT PyVTKSpecialType
{
public:
  PyVTKSpecialType() :
    py_type(0), methods(0), constructors(0), docstring(0), copy_func(0) {};

  PyVTKSpecialType(
    PyTypeObject *typeobj, PyMethodDef *cmethods, PyMethodDef *ccons,
    const char *cdocs[], PyVTKSpecialCopyFunc copyfunc);

  // general information
  PyTypeObject *py_type;
  PyMethodDef *methods;
  PyMethodDef *constructors;
  PyObject *docstring;
  // copy an object
  PyVTKSpecialCopyFunc copy_func;
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
VTKWRAPPINGPYTHON_EXPORT
PyObject *PyVTKSpecialType_New(PyTypeObject *pytype,
  PyMethodDef *methods, PyMethodDef *constructors, PyMethodDef *newmethod,
  const char *docstring[], PyVTKSpecialCopyFunc copyfunc);

VTKWRAPPINGPYTHON_EXPORT
PyObject *PyVTKSpecialObject_New(const char *classname, void *ptr);

VTKWRAPPINGPYTHON_EXPORT
PyObject *PyVTKSpecialObject_CopyNew(const char *classname, const void *ptr);

VTKWRAPPINGPYTHON_EXPORT
PyObject *PyVTKSpecialObject_Repr(PyObject *self);

VTKWRAPPINGPYTHON_EXPORT
PyObject *PyVTKSpecialObject_SequenceString(PyObject *self);

#if PY_VERSION_HEX < 0x02020000
VTKWRAPPINGPYTHON_EXPORT
PyObject *PyVTKSpecialObject_GetAttr(PyObject *self, PyObject *attr);
#endif
}

#endif
