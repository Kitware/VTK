// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "PyVTKMethodDescriptor.h"
#include "vtkABINamespace.h"
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

//------------------------------------------------------------------------------
// C API

PyObject* PyVTKMethodDescriptor_New(PyTypeObject* pytype, PyMethodDef* meth)
{
  PyMethodDescrObject* descr =
    (PyMethodDescrObject*)PyType_GenericAlloc(&PyVTKMethodDescriptor_Type, 0);

  if (descr)
  {
    Py_XINCREF(pytype);
    PyDescr_TYPE(descr) = pytype;
    PyDescr_NAME(descr) = PyUnicode_InternFromString(meth->ml_name);
    descr->d_method = meth;

    if (!PyDescr_NAME(descr))
    {
      Py_DECREF(descr);
      descr = nullptr;
    }
  }

  return (PyObject*)descr;
}

//------------------------------------------------------------------------------
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
  return PyUnicode_FromFormat("<method \'%U\' of \'%s\' objects>", PyDescr_NAME(descr),
    vtkPythonUtil::GetTypeName(PyDescr_TYPE(descr)));
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
    result = PyObject_Call(func, args, kwds);
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

  PyErr_Format(PyExc_TypeError, "descriptor '%U' for '%s' objects doesn't apply to '%s' object",
    PyDescr_NAME(descr), vtkPythonUtil::GetTypeName(PyDescr_TYPE(descr)),
    vtkPythonUtil::GetTypeNameForObject(obj));

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

  return PyUnicode_FromString(descr->d_method->ml_doc);
}

#if PY_VERSION_HEX >= 0x03070000
#define pystr(x) x
#else
#define pystr(x) const_cast<char*>(x)
#endif

static PyGetSetDef PyVTKMethodDescriptor_GetSet[] = {
  { pystr("__doc__"), PyVTKMethodDescriptor_GetDoc, nullptr, nullptr, nullptr },
  { nullptr, nullptr, nullptr, nullptr, nullptr }
};

static PyMemberDef PyVTKMethodDescriptor_Members[] = {
  { pystr("__objclass__"), T_OBJECT, offsetof(PyDescrObject, d_type), READONLY, nullptr },
  { pystr("__name__"), T_OBJECT, offsetof(PyDescrObject, d_name), READONLY, nullptr },
  { nullptr, 0, 0, 0, nullptr }
};

#ifdef VTK_PYTHON_NEEDS_DEPRECATION_WARNING_SUPPRESSION
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

//------------------------------------------------------------------------------
// clang-format off
PyTypeObject PyVTKMethodDescriptor_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "vtkmodules.vtkCommonCore.method_descriptor", // tp_name
  sizeof(PyMethodDescrObject),             // tp_basicsize
  0,                                       // tp_itemsize
  PyVTKMethodDescriptor_Delete,            // tp_dealloc
#if PY_VERSION_HEX >= 0x03080000
  //Prior to Py3.8, this member was a function pointer,
  //but as of Py3.8 it is an integer
  //(and therefore incompatible with nullptr).
  0,                                       // tp_vectorcall_offset
#else
  nullptr,                                 // tp_print
#endif
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
