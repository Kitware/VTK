/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PyVTKObject.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PyVTKObject was created in Oct 2000 by David Gobbi for VTK 3.2.
-----------------------------------------------------------------------*/

#ifndef __PyVTKObject_h
#define __PyVTKObject_h

#include "vtkWrappingPythonModule.h" // For export macro
#include "vtkPython.h"

class vtkObjectBase;
struct PyVTKClass;

// Flags for special properties or features
#define VTK_PYTHON_IGNORE_UNREGISTER 1 // block Register/UnRegister calls

// This is the VTK/Python 'object,' it contains the python object header
// plus a pointer to the associated vtkObjectBase and PyVTKClass.
struct PyVTKObject {
  PyObject_HEAD
  PyVTKClass *vtk_class;
  PyObject *vtk_dict;
  vtkObjectBase *vtk_ptr;
  unsigned long *vtk_observers;
  unsigned int vtk_flags;
#if PY_VERSION_HEX >= 0x02010000
  PyObject *vtk_weakreflist;
#endif
};

extern VTKWRAPPINGPYTHON_EXPORT PyTypeObject PyVTKObject_Type;

#define PyVTKObject_Check(obj) ((obj)->ob_type == &PyVTKObject_Type)

extern "C"
{
VTKWRAPPINGPYTHON_EXPORT
PyObject *PyVTKObject_New(
  PyObject *vtkclass, PyObject *pydict, vtkObjectBase *ptr);

VTKWRAPPINGPYTHON_EXPORT
vtkObjectBase *PyVTKObject_GetObject(PyObject *obj);

VTKWRAPPINGPYTHON_EXPORT
void PyVTKObject_AddObserver(PyObject *obj, unsigned long id);

VTKWRAPPINGPYTHON_EXPORT
void PyVTKObject_SetFlag(PyObject *obj, unsigned int flag, int val);

VTKWRAPPINGPYTHON_EXPORT
unsigned int PyVTKObject_GetFlags(PyObject *obj);
}

#endif
