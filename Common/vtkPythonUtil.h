/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonUtil.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkPythonUtil_h
#define __vtkPythonUtil_h

#include "vtkPython.h"
#include "vtkCommand.h"

#if defined(WIN32)
# if defined(vtkCommonPythonD_EXPORTS)
#  define VTK_PYTHON_EXPORT __declspec(dllexport)
# else
#  define VTK_PYTHON_EXPORT __declspec(dllimport)
# endif
#else
# define VTK_PYTHON_EXPORT
#endif

// This is the VTK/Python 'class,' it contains the method list and a pointer
// to the superclass
typedef vtkObjectBase *(*vtknewfunc)();

typedef struct {
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
} PyVTKClass;

// This is the VTK/Python 'object,' it contains the python object header
// plus a pointer to the associated vtkObjectBase and PyVTKClass.
typedef struct {
  PyObject_HEAD
  // the first two are common with the PyInstanceObject
  PyVTKClass *vtk_class;
  PyObject *vtk_dict;
  // the rest are unique to the PyVTKObject
  vtkObjectBase *vtk_ptr;
#if PY_VERSION_HEX >= 0x02010000
  PyObject *vtk_weakreflist;
#endif
} PyVTKObject;

// This for objects not derived from vtkObjectBase
typedef struct {
  PyObject_HEAD
  void *vtk_ptr;
  PyMethodDef *vtk_methods;
  PyObject *vtk_name;
  PyObject *vtk_doc;
} PyVTKSpecialObject;

// Standard methods for all vtk/python objects
extern "C" 
{
VTK_PYTHON_EXPORT int PyVTKObject_Check(PyObject *obj);
VTK_PYTHON_EXPORT int PyVTKClass_Check(PyObject *obj);
VTK_PYTHON_EXPORT int PyVTKSpecialObjectCheck(PyObject *obj);
VTK_PYTHON_EXPORT
PyObject *PyVTKObject_New(PyObject *vtkclass, vtkObjectBase *ptr);
VTK_PYTHON_EXPORT
PyObject *PyVTKClass_New(vtknewfunc constructor, PyMethodDef *methods,
                         char *classname, char *modulename, char *docstring[],
                         PyObject *base);
VTK_PYTHON_EXPORT
PyObject *PyVTKSpecialObject_New(void *ptr, PyMethodDef *methods,
                                 char *classname, char *docstring[]);

// this is a special version of ParseTuple that handles both bound
// and unbound method calls for VTK objects
VTK_PYTHON_EXPORT
vtkObjectBase *PyArg_VTKParseTuple(PyObject *self, PyObject *args, 
                                   char *format, ...);
}

// Add a PyVTKClass to the type lookup table, this allows us to later
// create object given only the class name.
extern VTK_PYTHON_EXPORT
void vtkPythonAddClassToHash(PyObject *obj, const char *type); 

// Extract the vtkObjectBase from a PyVTKObject.  If the PyObject is not a 
// PyVTKObject, or is not a PyVTKObject of the specified type, the python
// error indicator will be set.
// Special behaviour: Py_None is converted to NULL without no error.
extern VTK_PYTHON_EXPORT
vtkObjectBase *vtkPythonGetPointerFromObject(PyObject *obj, const char *type);

// Convert a vtkObjectBase to a PyVTKObject.  This will first check to see if
// the PyVTKObject already exists, and create a new PyVTKObject if necessary.
// This function also passes ownership of the reference to the PyObject.
// Special behaviour: NULL is converted to Py_None.
extern VTK_PYTHON_EXPORT
PyObject *vtkPythonGetObjectFromPointer(vtkObjectBase *ptr);

// Try to convert some PyObject into a PyVTKObject, currently conversion
// is supported for SWIG-style mangled pointer strings.
extern VTK_PYTHON_EXPORT
PyObject *vtkPythonGetObjectFromObject(PyObject *arg, const char *type);

// Add and delete PyVTKObject/vtkObjectBase pairs from the wrapper hash table,
// these methods do not change the reference counts of either the vtkObjectBase
// or the PyVTKObject.
extern VTK_PYTHON_EXPORT
void vtkPythonAddObjectToHash(PyObject *obj, vtkObjectBase *anInstance);
extern VTK_PYTHON_EXPORT
void vtkPythonDeleteObjectFromHash(PyObject *obj);

// Utility functions for creating/usinge SWIG-style mangled pointer strings.
extern VTK_PYTHON_EXPORT
char *vtkPythonManglePointer(void *ptr, const char *type);
extern VTK_PYTHON_EXPORT
void *vtkPythonUnmanglePointer(char *ptrText, int *len, const char *type);

// check array arguments sent through the wrappers to see if the underlying
// C++ method changed the values, and attempt to modify the original python
// sequence (list or tuple) if so.
extern VTK_PYTHON_EXPORT
int vtkPythonCheckArray(PyObject *args, int i, char *a, int n);
extern VTK_PYTHON_EXPORT
int vtkPythonCheckArray(PyObject *args, int i, signed char *a, int n);
extern VTK_PYTHON_EXPORT
int vtkPythonCheckArray(PyObject *args, int i, unsigned char *a, int n);
extern VTK_PYTHON_EXPORT
int vtkPythonCheckArray(PyObject *args, int i, short *a, int n);
extern VTK_PYTHON_EXPORT
int vtkPythonCheckArray(PyObject *args, int i, unsigned short *a, int n);
extern VTK_PYTHON_EXPORT
int vtkPythonCheckArray(PyObject *args, int i, int *a, int n);
extern VTK_PYTHON_EXPORT
int vtkPythonCheckArray(PyObject *args, int i, unsigned int *a, int n);
extern VTK_PYTHON_EXPORT
int vtkPythonCheckArray(PyObject *args, int i, long *a, int n);
extern VTK_PYTHON_EXPORT
int vtkPythonCheckArray(PyObject *args, int i, unsigned long *a, int n);
extern VTK_PYTHON_EXPORT
int vtkPythonCheckArray(PyObject *args, int i, float *a, int n);
extern VTK_PYTHON_EXPORT
int vtkPythonCheckArray(PyObject *args, int i, double *a, int n);
#if defined(VTK_TYPE_USE_LONG_LONG)
extern VTK_PYTHON_EXPORT
int vtkPythonCheckArray(PyObject *args, int i, long long *a, int n);
extern VTK_PYTHON_EXPORT
int vtkPythonCheckArray(PyObject *args, int i, unsigned long long *a, int n);
#endif
#if defined(VTK_TYPE_USE___INT64)
extern VTK_PYTHON_EXPORT
int vtkPythonCheckArray(PyObject *args, int i, __int64 *a, int n);
extern VTK_PYTHON_EXPORT
int vtkPythonCheckArray(PyObject *args, int i, unsigned __int64 *a, int n);
#endif

// For use by SetXXMethod() , SetXXMethodArgDelete()
extern VTK_PYTHON_EXPORT void vtkPythonVoidFunc(void *);
extern VTK_PYTHON_EXPORT void vtkPythonVoidFuncArgDelete(void *);

// To allow Python to use the vtkCommand features
class vtkPythonCommand : public vtkCommand
{
public:
  static vtkPythonCommand *New() { return new vtkPythonCommand; };

  void SetObject(PyObject *o);
  void SetThreadState(PyThreadState *ts);
  void Execute(vtkObject *ptr, unsigned long eventtype, void *CallData);
 
  PyObject *obj;
  PyThreadState *ThreadState;
protected:
  vtkPythonCommand();
  ~vtkPythonCommand(); 
};

#endif
