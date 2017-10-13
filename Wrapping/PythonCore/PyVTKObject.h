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
  It was rewritten in Jul 2015 to wrap VTK classes as python type objects.
-----------------------------------------------------------------------*/

#ifndef PyVTKObject_h
#define PyVTKObject_h

#include "vtkWrappingPythonCoreModule.h" // For export macro
#include "vtkPython.h"
#include "vtkSystemIncludes.h"

class vtkObjectBase;
typedef vtkObjectBase *(*vtknewfunc)();

// Flags for special properties or features
#define VTK_PYTHON_IGNORE_UNREGISTER 1 // block Register/UnRegister calls

// This class is used for defining new VTK wrapped classes.
// It contains information such as the methods and docstring, as well
// as extra info that can't easily be stored in the PyTypeObject.
class VTKWRAPPINGPYTHONCORE_EXPORT PyVTKClass
{
public:
  PyVTKClass() :
    py_type(nullptr), py_methods(nullptr), vtk_name(nullptr), vtk_new(nullptr) {}

  PyVTKClass(
    PyTypeObject *typeobj, PyMethodDef *methods,
    const char *classname, vtknewfunc constructor);

  PyTypeObject *py_type;
  PyMethodDef *py_methods;
  const char *vtk_name;     // the name returned by GetClassName()
  vtknewfunc vtk_new;       // creates a C++ instance of classtype
};

// This is the VTK/Python 'object,' it contains the python object header
// plus a pointer to the associated vtkObjectBase and PyVTKClass.
struct PyVTKObject {
  PyObject_HEAD
  PyObject *vtk_dict;        // each object has its own dict
  PyObject *vtk_weakreflist; // list of weak references via python
  PyVTKClass *vtk_class;     // information about the class
  vtkObjectBase *vtk_ptr;    // pointer to the C++ object
  Py_ssize_t *vtk_buffer;    // ndims, shape, strides for Py_buffer
  unsigned long *vtk_observers; // used to find our observers
  unsigned int vtk_flags;    // flags (see list above)
};

extern VTKWRAPPINGPYTHONCORE_EXPORT PyGetSetDef PyVTKObject_GetSet[];
extern VTKWRAPPINGPYTHONCORE_EXPORT PyBufferProcs PyVTKObject_AsBuffer;

extern "C"
{
VTKWRAPPINGPYTHONCORE_EXPORT
PyVTKClass *PyVTKClass_Add(
  PyTypeObject *pytype, PyMethodDef *methods, const char *classname,
  vtknewfunc constructor);

VTKWRAPPINGPYTHONCORE_EXPORT
int PyVTKObject_Check(PyObject *obj);

VTKWRAPPINGPYTHONCORE_EXPORT
PyObject *PyVTKObject_FromPointer(
  PyTypeObject *vtkclass, PyObject *pydict, vtkObjectBase *ptr);

VTKWRAPPINGPYTHONCORE_EXPORT
vtkObjectBase *PyVTKObject_GetObject(PyObject *obj);

VTKWRAPPINGPYTHONCORE_EXPORT
void PyVTKObject_AddObserver(PyObject *obj, unsigned long id);

VTKWRAPPINGPYTHONCORE_EXPORT
void PyVTKObject_SetFlag(PyObject *obj, unsigned int flag, int val);

VTKWRAPPINGPYTHONCORE_EXPORT
unsigned int PyVTKObject_GetFlags(PyObject *obj);

VTKWRAPPINGPYTHONCORE_EXPORT
PyObject *PyVTKObject_Repr(PyObject *op);

VTKWRAPPINGPYTHONCORE_EXPORT
PyObject *PyVTKObject_String(PyObject *op);

VTKWRAPPINGPYTHONCORE_EXPORT
int PyVTKObject_Traverse(PyObject *o, visitproc visit, void *arg);

VTKWRAPPINGPYTHONCORE_EXPORT
PyObject *PyVTKObject_New(PyTypeObject *, PyObject *args, PyObject *kwds);

VTKWRAPPINGPYTHONCORE_EXPORT
void PyVTKObject_Delete(PyObject *op);
}

#endif
