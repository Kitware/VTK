/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PyVTKSpecialObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PyVTKSpecialObject was created in Feb 2001 by David Gobbi.
  It was substantially updated in April 2010 by David Gobbi.

  A PyVTKSpecialObject is a python object that represents an object
  that belongs to one of the special classes in VTK, that is, classes
  that are not derived from vtkObjectBase.  Unlike vtkObjects, these
  special objects are not reference counted: a PyVTKSpecialObject
  always contains its own copy of the C++ object.

  The PyVTKSpecialType is a simple structure that contains information
  about the PyVTKSpecialObject type that cannot be stored in python's
  PyTypeObject struct.  Each PyVTKSpecialObject contains a pointer to
  its PyVTKSpecialType. The PyVTKSpecialTypes are also stored in a map
  in vtkPythonUtil.cxx, so that they can be lookup up by name.
-----------------------------------------------------------------------*/

#include "PyVTKSpecialObject.h"
#include "PyVTKMethodDescriptor.h"
#include "vtkPythonUtil.h"

#include <sstream>

// Silence warning like
// "dereferencing type-punned pointer will break strict-aliasing rules"
// it happens because this kind of expression: (long *)&ptr
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

//--------------------------------------------------------------------
PyVTKSpecialType::PyVTKSpecialType(
    PyTypeObject *typeobj, PyMethodDef *cmethods, PyMethodDef *ccons,
    vtkcopyfunc copyfunc)
{
  this->py_type = typeobj;
  this->vtk_methods = cmethods;
  this->vtk_constructors = ccons;
  this->vtk_copy = copyfunc;
}

//--------------------------------------------------------------------
// Object protocol

//--------------------------------------------------------------------
PyObject *PyVTKSpecialObject_Repr(PyObject *self)
{
  PyVTKSpecialObject *obj = (PyVTKSpecialObject *)self;
  PyTypeObject *type = Py_TYPE(self);
  const char *name = Py_TYPE(self)->tp_name;

  while (type->tp_base && !type->tp_str)
  {
    type = type->tp_base;
  }

  // use str() if available
  PyObject *s = NULL;
  if (type->tp_str && type->tp_str != (&PyBaseObject_Type)->tp_str)
  {
    PyObject *t = type->tp_str(self);
    if (t == NULL)
    {
      Py_XDECREF(s);
      s = NULL;
    }
    else
    {
#ifdef VTK_PY3K
      s = PyString_FromFormat("(%.80s)%S", name, t);
#else
      s = PyString_FromFormat("(%.80s)%s", name, PyString_AsString(t));
#endif
    }
  }
  // otherwise just print address of object
  else if (obj->vtk_ptr)
  {
    s = PyString_FromFormat("(%.80s)%p", name, obj->vtk_ptr);
  }

  return s;
}

//--------------------------------------------------------------------
PyObject *PyVTKSpecialObject_SequenceString(PyObject *self)
{
  Py_ssize_t n, i;
  PyObject *s = NULL;
  PyObject *t, *o, *comma;
  const char *bracket = "[...]";

  if (Py_TYPE(self)->tp_as_sequence &&
      Py_TYPE(self)->tp_as_sequence->sq_item != NULL &&
      Py_TYPE(self)->tp_as_sequence->sq_ass_item == NULL)
  {
    bracket = "(...)";
  }

  i = Py_ReprEnter(self);
  if (i < 0)
  {
    return NULL;
  }
  else if (i > 0)
  {
    return PyString_FromString(bracket);
  }

  n = PySequence_Size(self);
  if (n >= 0)
  {
    comma = PyString_FromString(", ");
    s = PyString_FromStringAndSize(bracket, 1);

    for (i = 0; i < n && s != NULL; i++)
    {
      if (i > 0)
      {
#ifdef VTK_PY3K
        PyObject *tmp = PyUnicode_Concat(s, comma);
        Py_DECREF(s);
        s = tmp;
#else
        PyString_Concat(&s, comma);
#endif
      }
      o = PySequence_GetItem(self, i);
      t = NULL;
      if (o)
      {
        t = PyObject_Repr(o);
        Py_DECREF(o);
      }
      if (t)
      {
#ifdef VTK_PY3K
        PyObject *tmp = PyUnicode_Concat(s, t);
        Py_DECREF(s);
        Py_DECREF(t);
        s = tmp;
#else
        PyString_ConcatAndDel(&s, t);
#endif
      }
      else
      {
        Py_DECREF(s);
        s = NULL;
      }
      n = PySequence_Size(self);
    }

    if (s)
    {
#ifdef VTK_PY3K
      PyObject *tmp1 = PyString_FromStringAndSize(&bracket[4], 1);
      PyObject *tmp2 = PyUnicode_Concat(s, tmp1);
      Py_DECREF(s);
      Py_DECREF(tmp1);
      s = tmp2;
#else
      PyString_ConcatAndDel(&s,
        PyString_FromStringAndSize(&bracket[4], 1));
#endif
    }

    Py_DECREF(comma);
  }

  Py_ReprLeave(self);

  return s;
}

//--------------------------------------------------------------------
// C API

//--------------------------------------------------------------------
// Create a new python object from the pointer to a C++ object
PyObject *PyVTKSpecialObject_New(const char *classname, void *ptr)
{
  // would be nice if "info" could be passed instead if "classname",
  // but this way of doing things is more dynamic if less efficient
  PyVTKSpecialType *info = vtkPythonUtil::FindSpecialType(classname);

  PyVTKSpecialObject *self = PyObject_New(PyVTKSpecialObject, info->py_type);

  self->vtk_info = info;
  self->vtk_ptr = ptr;
  self->vtk_hash = -1;

  return (PyObject *)self;
}

//--------------------------------------------------------------------
// Create a new python object via the copy constructor of the C++ object
PyObject *PyVTKSpecialObject_CopyNew(const char *classname, const void *ptr)
{
  PyVTKSpecialType *info = vtkPythonUtil::FindSpecialType(classname);

  if (info == 0)
  {
    return PyErr_Format(PyExc_ValueError,
                        "cannot create object of unknown type \"%s\"",
                        classname);
  }
  else if (info->vtk_copy == 0)
  {
    return PyErr_Format(PyExc_ValueError,
                        "no copy constructor for object of type \"%s\"",
                        classname);
  }

  PyVTKSpecialObject *self = PyObject_New(PyVTKSpecialObject, info->py_type);

  self->vtk_info = info;
  self->vtk_ptr = info->vtk_copy(ptr);
  self->vtk_hash = -1;

  return (PyObject *)self;
}

//--------------------------------------------------------------------
// Add a special type, add methods and members to its type object.
// A return value of NULL signifies that it was already added.
PyVTKSpecialType *PyVTKSpecialType_Add(PyTypeObject *pytype,
  PyMethodDef *methods, PyMethodDef *constructors,
  const char *docstring[], vtkcopyfunc copyfunc)
{
  // Add this type to the special type map
  PyVTKSpecialType *info =
    vtkPythonUtil::AddSpecialTypeToMap(
      pytype, methods, constructors, copyfunc);

  if (info == 0)
  {
    // The type was already in the map, so do nothing
    return info;
  }

  // Create the dict
  if (pytype->tp_dict == 0)
  {
    pytype->tp_dict = PyDict_New();
  }

  // Add the docstring to the type
  PyObject *doc = vtkPythonUtil::BuildDocString(docstring);
  PyDict_SetItemString(pytype->tp_dict, "__doc__", doc);
  Py_DECREF(doc);

  // Add all of the methods
  for (PyMethodDef *meth = methods; meth && meth->ml_name; meth++)
  {
    PyObject *func = PyVTKMethodDescriptor_New(pytype, meth);
    PyDict_SetItemString(pytype->tp_dict, meth->ml_name, func);
    Py_DECREF(func);
  }

  return info;
}
