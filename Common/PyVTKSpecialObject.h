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

#ifndef __PyVTKSpecialObject_h
#define __PyVTKSpecialObject_h

#include "vtkPython.h"
#include "vtkSystemIncludes.h"

// This for objects not derived from vtkObjectBase

// Prototypes for per-type copy, delete, and print funcs
typedef void *(*PyVTKSpecialCopyFunc)(void *);
typedef void (*PyVTKSpecialDeleteFunc)(void *);
typedef void (*PyVTKSpecialPrintFunc)(ostream& os, void *);

// Unlike PyVTKObject, there is no "meta-type" like PyVTKClass.
// Instead, there is just the following class that contains info
// about each special type.
VTK_PYTHON_EXPORT class PyVTKSpecialType
{
public:
  PyVTKSpecialType() :
    classname(0), docstring(0), methods(0), constructors(0),
    copy_func(0), delete_func(0), print_func(0) {};

  PyVTKSpecialType(
    char *cname, char *cdocs[], PyMethodDef *cmethods, PyMethodDef *ccons,
    PyVTKSpecialCopyFunc copyfunc, PyVTKSpecialDeleteFunc deletefunc,
    PyVTKSpecialPrintFunc printfunc);

  // general information
  PyObject *classname;
  PyObject *docstring;
  PyMethodDef *methods;
  PyMethodDef *constructors;
  // mandatory functions
  PyVTKSpecialCopyFunc copy_func;
  PyVTKSpecialDeleteFunc delete_func;
  PyVTKSpecialPrintFunc print_func;
};

// The PyVTKSpecialObject is very lightweight.
struct PyVTKSpecialObject {
  PyObject_HEAD
  void *vtk_ptr;
  PyVTKSpecialType *vtk_info;
};

extern "C"
{
VTK_PYTHON_EXPORT
PyObject *PyVTKSpecialType_New(
  PyMethodDef *newmethod, PyMethodDef *methods, PyMethodDef *constructors,
  char *classname, char *docstring[],
  PyVTKSpecialCopyFunc copy_func,
  PyVTKSpecialDeleteFunc delete_func,
  PyVTKSpecialPrintFunc print_func);

VTK_PYTHON_EXPORT
int PyVTKSpecialObject_Check(PyObject *obj);

VTK_PYTHON_EXPORT
PyObject *PyVTKSpecialObject_New(char *classname, void *ptr, int copy);
}

#endif
