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
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

// Required for Python 2.5 through Python 2.7
#ifndef PyDescr_TYPE
#define PyDescr_TYPE(x) (((PyDescrObject*)(x))->d_type)
#define PyDescr_NAME(x) (((PyDescrObject*)(x))->d_name)
#endif

//--------------------------------------------------------------------
// C API

PyObject* PyVTKMethodDescriptor_New(PyTypeObject* pytype, PyMethodDef* meth)
{
  PyMethodDescrObject* descr =
    (PyMethodDescrObject*)PyType_GenericAlloc(&PyVTKMethodDescriptor_Type, 0);

  if (descr)
  {
    Py_XINCREF(pytype);
    PyDescr_TYPE(descr) = pytype;
    PyDescr_NAME(descr) = PyString_InternFromString(meth->ml_name);
    descr->d_method = meth;

    if (!PyDescr_NAME(descr))
    {
      Py_DECREF(descr);
      descr = nullptr;
    }
  }

  return (PyObject*)descr;
}

//--------------------------------------------------------------------
// Object protocol

static void PyVTKMethodDescriptor_Delete(PyObject* ob)
{
  PyMethodDescrObject* descr = (PyMethodDescrObject*)ob;
  PyObject_GC_UnTrack(descr);
  Py_XDECREF(PyDescr_TYPE(descr));
  Py_XDECREF(PyDescr_NAME(descr));
  PyObject_GC_Del(descr);
}

static PyObject* PyVTKMethodDescriptor_Repr(PyObject* ob)
{
  PyMethodDescrObject* descr = (PyMethodDescrObject*)ob;
#ifdef VTK_PY3K
  return PyUnicode_FromFormat(
    "<method \'%U\' of \'%s\' objects>", PyDescr_NAME(descr), PyDescr_TYPE(descr)->tp_name);
#else
  return PyString_FromFormat(
    "<method \'%s\' of \'%s\' objects>", PyString_AS_STRING(descr->d_name), descr->d_type->tp_name);
#endif
}

static int PyVTKMethodDescriptor_Traverse(PyObject* ob, visitproc visit, void* arg)
{
  PyMethodDescrObject* descr = (PyMethodDescrObject*)ob;
  Py_VISIT(PyDescr_TYPE(descr));
  return 0;
}

static PyObject* PyVTKMethodDescriptor_Call(PyObject* ob, PyObject* args, PyObject* kwds)
{
  PyMethodDescrObject* descr = (PyMethodDescrObject*)ob;
  PyObject* result = nullptr;
  PyObject* func = PyCFunction_New(descr->d_method, (PyObject*)PyDescr_TYPE(descr));

  if (func)
  {
    result = PyEval_CallObjectWithKeywords(func, args, kwds);
    Py_DECREF(func);
  }

  return result;
}

static PyObject* PyVTKMethodDescriptor_Get(PyObject* self, PyObject* obj, PyObject*)
{
  PyMethodDescrObject* descr = (PyMethodDescrObject*)self;

  if (obj == nullptr)
  {
    // If no object to bind to, return the descriptor itself
    Py_INCREF(self);
    return self;
  }

  if (PyObject_TypeCheck(obj, PyDescr_TYPE(descr)))
  {
    // Bind the method to the object
    return PyCFunction_New(descr->d_method, obj);
  }

#ifdef VTK_PY3K
  PyErr_Format(PyExc_TypeError, "descriptor '%U' for '%s' objects doesn't apply to '%s' object",
    PyDescr_NAME(descr), PyDescr_TYPE(descr)->tp_name, Py_TYPE(obj)->tp_name);
#else
  PyErr_Format(PyExc_TypeError, "descriptor '%s' for '%s' objects doesn't apply to '%s' object",
    PyString_AS_STRING(PyDescr_NAME(descr)), PyDescr_TYPE(descr)->tp_name, Py_TYPE(obj)->tp_name);
#endif

  return nullptr;
}

static PyObject* PyVTKMethodDescriptor_GetDoc(PyObject* ob, void*)
{
  PyMethodDescrObject* descr = (PyMethodDescrObject*)ob;

  if (descr->d_method->ml_doc == nullptr)
  {
    Py_INCREF(Py_None);
    return Py_None;
  }

  return PyString_FromString(descr->d_method->ml_doc);
}

static PyGetSetDef PyVTKMethodDescriptor_GetSet[] = {
#if PY_VERSION_HEX >= 0x03070000
  { "__doc__", PyVTKMethodDescriptor_GetDoc, nullptr, nullptr, nullptr },
#else
  { const_cast<char*>("__doc__"), PyVTKMethodDescriptor_GetDoc, nullptr, nullptr, nullptr },
#endif
  { nullptr, nullptr, nullptr, nullptr, nullptr }
};

static PyMemberDef PyVTKMethodDescriptor_Members[] = {
#if PY_VERSION_HEX >= 0x03070000
  { "__objclass__", T_OBJECT, offsetof(PyDescrObject, d_type), READONLY, nullptr },
  { "__name__", T_OBJECT, offsetof(PyDescrObject, d_name), READONLY, nullptr },
#else
  { const_cast<char*>("__objclass__"), T_OBJECT, offsetof(PyDescrObject, d_type), READONLY,
    nullptr },
  { const_cast<char*>("__name__"), T_OBJECT, offsetof(PyDescrObject, d_name), READONLY, nullptr },
#endif
  { nullptr, 0, 0, 0, nullptr }
};

//--------------------------------------------------------------------
// clang-format off
PyTypeObject PyVTKMethodDescriptor_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "vtkmodules.vtkCommonCore.method_descriptor", // tp_name
  sizeof(PyMethodDescrObject),             // tp_basicsize
  0,                                       // tp_itemsize
  PyVTKMethodDescriptor_Delete,            // tp_dealloc
  0,                                       // tp_vectorcall_offset
  nullptr,                                 // tp_getattr
  nullptr,                                 // tp_setattr
  nullptr,                                 // tp_compare
  PyVTKMethodDescriptor_Repr,              // tp_repr
  nullptr,                                 // tp_as_number
  nullptr,                                 // tp_as_sequence
  nullptr,                                 // tp_as_mapping
  nullptr,                                 // tp_hash
  PyVTKMethodDescriptor_Call,              // tp_call
  nullptr,                                 // tp_string
  PyObject_GenericGetAttr,                 // tp_getattro
  nullptr,                                 // tp_setattro
  nullptr,                                 // tp_as_buffer
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, // tp_flags
  nullptr,                                 // tp_doc
  PyVTKMethodDescriptor_Traverse,          // tp_traverse
  nullptr,                                 // tp_clear
  nullptr,                                 // tp_richcompare
  0,                                       // tp_weaklistoffset
  nullptr,                                 // tp_iter
  nullptr,                                 // tp_iternext
  nullptr,                                 // tp_methods
  PyVTKMethodDescriptor_Members,           // tp_members
  PyVTKMethodDescriptor_GetSet,            // tp_getset
  nullptr,                                 // tp_base
  nullptr,                                 // tp_dict
  PyVTKMethodDescriptor_Get,               // tp_descr_get
  nullptr,                                 // tp_descr_set
  0,                                       // tp_dictoffset
  nullptr,                                 // tp_init
  nullptr,                                 // tp_alloc
  nullptr,                                 // tp_new
  nullptr,                                 // tp_free
  nullptr,                                 // tp_is_gc
  nullptr,                                 // tp_bases
  nullptr,                                 // tp_mro
  nullptr,                                 // tp_cache
  nullptr,                                 // tp_subclasses
  nullptr,                                 // tp_weaklist
  VTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED };
// clang-format on
