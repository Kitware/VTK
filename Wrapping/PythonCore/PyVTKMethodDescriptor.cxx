/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PyVTKMethodDescriptor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "PyVTKMethodDescriptor.h"
#include "vtkPythonUtil.h"

#include <structmember.h> // a python header

// Silence warning like
// "dereferencing type-punned pointer will break strict-aliasing rules"
// it happens because this kind of expression: (long *)&ptr
// pragma GCC diagnostic is available since gcc>=4.2
#if defined(__GNUC__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=2)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

//--------------------------------------------------------------------
// C API

PyObject *PyVTKMethodDescriptor_New(PyTypeObject *pytype, PyMethodDef *meth)
{
  PyMethodDescrObject *descr = (PyMethodDescrObject *)
    PyType_GenericAlloc(&PyVTKMethodDescriptor_Type, 0);

  if (descr)
    {
    Py_XINCREF(pytype);
    descr->d_type = pytype;
    descr->d_name = PyString_InternFromString(meth->ml_name);
    descr->d_method = meth;

    if (!descr->d_name)
      {
      Py_DECREF(descr);
      descr = 0;
      }
    }

  return (PyObject *)descr;
}

//--------------------------------------------------------------------
// Object protocol

static void PyVTKMethodDescriptor_Delete(PyObject *ob)
{
  PyMethodDescrObject *descr = (PyMethodDescrObject *)ob;
  PyObject_GC_UnTrack(descr);
  Py_XDECREF(descr->d_type);
  Py_XDECREF(descr->d_name);
  PyObject_GC_Del(descr);
}

static PyObject *PyVTKMethodDescriptor_Repr(PyObject *ob)
{
  PyMethodDescrObject *descr = (PyMethodDescrObject *)ob;
  return PyString_FromFormat("<method \'%s\' of \'%s\' objects>",
    PyString_AS_STRING(descr->d_name), descr->d_type->tp_name);
}

static int PyVTKMethodDescriptor_Traverse(
  PyObject *ob, visitproc visit, void *arg)
{
  PyMethodDescrObject *descr = (PyMethodDescrObject *)ob;
  Py_VISIT(descr->d_type);
  return 0;
}

static PyObject *PyVTKMethodDescriptor_Call(
  PyObject *ob, PyObject *args, PyObject *kwds)
{
  PyMethodDescrObject *descr = (PyMethodDescrObject *)ob;
  PyObject *result = 0;
  PyObject *func = PyCFunction_New(
    descr->d_method, (PyObject *)descr->d_type);

  if (func)
    {
    result = PyEval_CallObjectWithKeywords(func, args, kwds);
    Py_DECREF(func);
    }

  return result;
}

static PyObject *PyVTKMethodDescriptor_Get(
  PyObject *self, PyObject *obj, PyObject *)
{
  PyMethodDescrObject *descr = (PyMethodDescrObject *)self;

  if (obj == NULL)
    {
    // If no object to bind to, return the descriptor itself
    Py_INCREF(self);
    return self;
    }

  if (PyObject_TypeCheck(obj, descr->d_type))
    {
    // Bind the method to the object
    return PyCFunction_New(descr->d_method, obj);
    }

  PyErr_Format(
    PyExc_TypeError,
    "descriptor '%s' for '%s' objects doesn't apply to '%s' object",
    PyString_AS_STRING(descr->d_name), descr->d_type->tp_name,
    obj->ob_type->tp_name);

  return NULL;
}

static PyObject *PyVTKMethodDescriptor_GetDoc(PyObject *ob, void *)
{
  PyMethodDescrObject *descr = (PyMethodDescrObject *)ob;

  if (descr->d_method->ml_doc == NULL)
    {
    Py_INCREF(Py_None);
    return Py_None;
    }

  return PyString_FromString(descr->d_method->ml_doc);
}

static PyGetSetDef PyVTKMethodDescriptor_GetSet[] = {
  { (char *)"__doc__", PyVTKMethodDescriptor_GetDoc, 0, 0, 0 },
  { 0, 0, 0, 0, 0 }
};

static PyMemberDef PyVTKMethodDescriptor_Members[] = {
  { (char *)"__objclass__", T_OBJECT, offsetof(PyDescrObject, d_type),
    READONLY, 0 },
  { (char *)"__name__", T_OBJECT, offsetof(PyDescrObject, d_name),
    READONLY, 0 },
  { 0, 0, 0, 0, 0 }
};

//--------------------------------------------------------------------
PyTypeObject PyVTKMethodDescriptor_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  "vtk_method_descriptor",               // tp_name
  sizeof(PyMethodDescrObject),           // tp_basicsize
  0,                                     // tp_itemsize
  PyVTKMethodDescriptor_Delete,          // tp_dealloc
  0,                                     // tp_print
  0,                                     // tp_getattr
  0,                                     // tp_setattr
  0,                                     // tp_compare
  PyVTKMethodDescriptor_Repr,            // tp_repr
  0,                                     // tp_as_number
  0,                                     // tp_as_sequence
  0,                                     // tp_as_mapping
  0,                                     // tp_hash
  PyVTKMethodDescriptor_Call,            // tp_call
  0,                                     // tp_string
  PyObject_GenericGetAttr,               // tp_getattro
  0,                                     // tp_setattro
  0,                                     // tp_as_buffer
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, // tp_flags
  0,                                     // tp_doc
  PyVTKMethodDescriptor_Traverse,        // tp_traverse
  0,                                     // tp_clear
  0,                                     // tp_richcompare
  0,                                     // tp_weaklistoffset
  0,                                     // tp_iter
  0,                                     // tp_iternext
  0,                                     // tp_methods
  PyVTKMethodDescriptor_Members,         // tp_members
  PyVTKMethodDescriptor_GetSet,          // tp_getset
  0,                                     // tp_base
  0,                                     // tp_dict
  PyVTKMethodDescriptor_Get,             // tp_descr_get
  0,                                     // tp_descr_set
  0,                                     // tp_dictoffset
  0,                                     // tp_init
  0,                                     // tp_alloc
  0,                                     // tp_new
  0,                                     // tp_free
  0,                                     // tp_is_gc
  0,                                     // tp_bases
  0,                                     // tp_mro
  0,                                     // tp_cache
  0,                                     // tp_subclasses
  0,                                     // tp_weaklist
  VTK_WRAP_PYTHON_SUPRESS_UNINITIALIZED
};
