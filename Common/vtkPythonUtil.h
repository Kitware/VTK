/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonUtil.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkObject.h"
#include "vtkTimeStamp.h"
#include "Python.h"
#include "vtkCommand.h"

#if defined(WIN32)
#if defined(vtkCommonPython_EXPORTS)
  #define VTK_PYTHON_EXPORT __declspec( dllexport )
#else
    #define VTK_PYTHON_EXPORT __declspec( dllimport )
#endif
#else
  #define VTK_PYTHON_EXPORT
#endif

// This is the VTK/Python 'class,' it contains the method list and a pointer
// to the superclass
typedef vtkObject *(*vtknewfunc)();

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
// plus a pointer to the associated vtkObject and PyVTKClass.
typedef struct {
  PyObject_HEAD
  // the first two are common with the PyInstanceObject
  PyVTKClass *vtk_class;
  PyObject *vtk_dict;
  // the rest are unique to the PyVTKObject
  vtkObject *vtk_ptr;
} PyVTKObject;

// This for objects not derived from vtkObject
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
PyObject *PyVTKObject_New(PyObject *vtkclass, vtkObject *ptr);
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
vtkObject *PyArg_VTKParseTuple(PyObject *self, PyObject *args, 
                               char *format, ...);
}

// Add a PyVTKClass to the type lookup table, this allows us to later
// create object given only the class name.
extern VTK_PYTHON_EXPORT
void vtkPythonAddClassToHash(PyObject *obj, const char *type); 

// Extract the vtkObject from a PyVTKObject.  If the PyObject is not a 
// PyVTKObject, or is not a PyVTKObject of the specified type, the python
// error indicator will be set.
// Special behaviour: Py_None is converted to NULL without no error.
extern VTK_PYTHON_EXPORT
vtkObject *vtkPythonGetPointerFromObject(PyObject *obj, const char *type);

// Convert a vtkObject to a PyVTKObject.  This will first check to see if
// the PyVTKObject already exists, and create a new PyVTKObject if necessary.
// This function also passes ownership of the reference to the PyObject.
// Special behaviour: NULL is converted to Py_None.
extern VTK_PYTHON_EXPORT
PyObject *vtkPythonGetObjectFromPointer(vtkObject *ptr);

// Try to convert some PyObject into a PyVTKObject, currently conversion
// is supported for SWIG-style mangled pointer strings.
extern VTK_PYTHON_EXPORT
PyObject *vtkPythonGetObjectFromObject(PyObject *arg, const char *type);

// Add and delete PyVTKObject/vtkObject pairs from the wrapper hash table,
// these methods do not change the reference counts of either the vtkObject
// or the PyVTKObject.
extern VTK_PYTHON_EXPORT
void vtkPythonAddObjectToHash(PyObject *obj, vtkObject *anInstance);
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

// For use by SetXXMethod() , SetXXMethodArgDelete()
extern VTK_PYTHON_EXPORT void vtkPythonVoidFunc(void *);
extern VTK_PYTHON_EXPORT void vtkPythonVoidFuncArgDelete(void *);

// To allow Python to use the vtkCommand features
class vtkPythonCommand : public vtkCommand
{
public:
  static vtkPythonCommand *New() { return new vtkPythonCommand; };

  void SetObject(PyObject *o);
  void Execute(vtkObject *ptr, unsigned long eventtype, void *);
 
  PyObject *obj;
protected:
  vtkPythonCommand();
  ~vtkPythonCommand(); 
};


