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

#ifndef __PyVTKObject_h
#define __PyVTKObject_h

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
    py_type(0), vtk_methods(0), vtk_new(0),
    vtk_cppname(0), vtk_mangle(0) {}

  PyVTKClass(
    PyTypeObject *typeobj, PyMethodDef *methods,
    const char *classname, const char *manglename,
    vtknewfunc constructor);

  // general information
  PyTypeObject *py_type;
  PyMethodDef *vtk_methods;
  vtknewfunc vtk_new;       // creates a C++ instance of classtype
  const char *vtk_cppname;  // set to typeid(classtype).name()
  const char *vtk_mangle;   // the classtype, mangled with vtkParseMangle
};

// This is the VTK/Python 'object,' it contains the python object header
// plus a pointer to the associated vtkObjectBase and PyVTKClass.
struct PyVTKObject {
  PyObject_HEAD
  PyObject *vtk_dict;
  PyObject *vtk_weakreflist;
  PyVTKClass *vtk_class;
  vtkObjectBase *vtk_ptr;
  unsigned long *vtk_observers;
  unsigned int vtk_flags;
};

extern VTKWRAPPINGPYTHONCORE_EXPORT PyGetSetDef PyVTKObject_GetSet[];
extern VTKWRAPPINGPYTHONCORE_EXPORT PyBufferProcs PyVTKObject_AsBuffer;

extern "C"
{
VTKWRAPPINGPYTHONCORE_EXPORT
PyVTKClass *PyVTKClass_Add(
  PyTypeObject *pytype, PyMethodDef *methods, const char *classname,
  const char *manglename, const char *docstring[], vtknewfunc constructor);

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
