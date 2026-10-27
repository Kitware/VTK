// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "PyVTKMethodDescriptor.h"
#include "vtkABINamespace.h"
#include "vtkPythonUtil.h"

#include <structmember.h> // a python header

//------------------------------------------------------------------------------
PyTypeObject* PyVTKMethodDescriptor_Type = nullptr;

//------------------------------------------------------------------------------
// C API

PyObject* PyVTKMethodDescriptor_New(PyTypeObject* pytype, PyMethodDef* meth)
{
  PyVTKMethodDescriptor* descr =
    (PyVTKMethodDescriptor*)PyType_GenericAlloc(PyVTKMethodDescriptor_Type, 0);

  if (descr)
  {
    Py_XINCREF((PyObject*)pytype);
    descr->meth_class = pytype;
    descr->meth_name = PyUnicode_InternFromString(meth->ml_name);
    descr->meth_def = meth;

    if (!descr->meth_name)
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
  PyVTKMethodDescriptor* descr = (PyVTKMethodDescriptor*)ob;
  // Instances hold a reference to their (heap) type, taken by PyType_GenericAlloc.
  PyTypeObject* tp = Py_TYPE(ob);
  PyObject_GC_UnTrack(descr);
  Py_XDECREF((PyObject*)descr->meth_class);
  Py_XDECREF(descr->meth_name);
  PyObject_GC_Del(descr);
  Py_DECREF((PyObject*)tp);
}

static PyObject* PyVTKMethodDescriptor_Repr(PyObject* ob)
{
  PyVTKMethodDescriptor* descr = (PyVTKMethodDescriptor*)ob;
  return PyUnicode_FromFormat("<method \'%U\' of \'%s\' objects>", descr->meth_name,
    vtkPythonUtil::GetTypeName(descr->meth_class));
}

static int PyVTKMethodDescriptor_Traverse(PyObject* ob, visitproc visit, void* arg)
{
  PyVTKMethodDescriptor* descr = (PyVTKMethodDescriptor*)ob;
  Py_VISIT(Py_TYPE(ob));
  Py_VISIT((PyObject*)descr->meth_class);
  return 0;
}

static PyObject* PyVTKMethodDescriptor_Call(PyObject* ob, PyObject* args, PyObject* kwds)
{
  PyVTKMethodDescriptor* descr = (PyVTKMethodDescriptor*)ob;
  PyObject* result = nullptr;
  PyObject* func = PyCFunction_New(descr->meth_def, (PyObject*)descr->meth_class);

  if (func)
  {
    result = PyObject_Call(func, args, kwds);
    Py_DECREF(func);
  }

  return result;
}

static PyObject* PyVTKMethodDescriptor_Get(PyObject* self, PyObject* obj, PyObject*)
{
  PyVTKMethodDescriptor* descr = (PyVTKMethodDescriptor*)self;

  if (obj == nullptr)
  {
    // If no object to bind to, return the descriptor itself
    Py_INCREF(self);
    return self;
  }

  if (PyObject_TypeCheck(obj, descr->meth_class))
  {
    // Bind the method to the object
    return PyCFunction_New(descr->meth_def, obj);
  }

  PyErr_Format(PyExc_TypeError, "descriptor '%U' for '%s' objects doesn't apply to '%s' object",
    descr->meth_name, vtkPythonUtil::GetTypeName(descr->meth_class),
    vtkPythonUtil::GetTypeNameForObject(obj));

  return nullptr;
}

static PyObject* PyVTKMethodDescriptor_GetDoc(PyObject* ob, void*)
{
  PyVTKMethodDescriptor* descr = (PyVTKMethodDescriptor*)ob;

  if (descr->meth_def->ml_doc == nullptr)
  {
    Py_INCREF(Py_None);
    return Py_None;
  }

  return PyUnicode_FromString(descr->meth_def->ml_doc);
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
  // T_OBJECT_EX (not T_OBJECT) is the member type that is part of the stable
  // ABI; both fields are always set after a successful PyVTKMethodDescriptor_New
  // so the only difference (raising AttributeError vs returning None when null)
  // never applies here.
  { pystr("__objclass__"), T_OBJECT_EX, offsetof(PyVTKMethodDescriptor, meth_class), READONLY,
    nullptr },
  { pystr("__name__"), T_OBJECT_EX, offsetof(PyVTKMethodDescriptor, meth_name), READONLY, nullptr },
  { nullptr, 0, 0, 0, nullptr }
};

//------------------------------------------------------------------------------
// Type definition

static PyType_Slot PyVTKMethodDescriptor_Slots[] = {
  { Py_tp_dealloc, (void*)PyVTKMethodDescriptor_Delete },
  { Py_tp_repr, (void*)PyVTKMethodDescriptor_Repr },
  { Py_tp_call, (void*)PyVTKMethodDescriptor_Call },
  { Py_tp_getattro, (void*)PyObject_GenericGetAttr },
  { Py_tp_traverse, (void*)PyVTKMethodDescriptor_Traverse },
  { Py_tp_members, (void*)PyVTKMethodDescriptor_Members },
  { Py_tp_getset, (void*)PyVTKMethodDescriptor_GetSet },
  { Py_tp_descr_get, (void*)PyVTKMethodDescriptor_Get },
  { 0, nullptr },
};

static PyType_Spec PyVTKMethodDescriptor_Spec = {
  "vtkmodules.vtkCommonCore.method_descriptor", // tp_name
  sizeof(PyVTKMethodDescriptor),                // tp_basicsize
  0,                                            // tp_itemsize
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, PyVTKMethodDescriptor_Slots
};

//------------------------------------------------------------------------------
bool PyVTKMethodDescriptor_ClassNew()
{
  if (PyVTKMethodDescriptor_Type != nullptr)
  {
    return true;
  }
  PyVTKMethodDescriptor_Type = (PyTypeObject*)PyType_FromSpec(&PyVTKMethodDescriptor_Spec);
  return PyVTKMethodDescriptor_Type != nullptr;
}
