// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/*-----------------------------------------------------------------------
  The PyVTKNamespace was created in Nov 2014 by David Gobbi.

  This is a PyModule subclass for wrapping C++ namespaces.
-----------------------------------------------------------------------*/

#include "PyVTKNamespace.h"
#include "vtkABINamespace.h"
#include "vtkPythonUtil.h"

// Silence warning like
// "dereferencing type-punned pointer will break strict-aliasing rules"
// it happens because this kind of expression: (long *)&ptr
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

//------------------------------------------------------------------------------

static const char* PyVTKNamespace_Doc = "A python module that wraps a C++ namespace.\n";

//------------------------------------------------------------------------------
static void PyVTKNamespace_Delete(PyObject* op)
{
  // remove from the map so that there is no dangling reference
  vtkPythonUtil::RemoveNamespaceFromMap(op);
  // An instance of a heap type holds a reference to its type; calling the base
  // (module) destructor directly does not release it the way subtype_dealloc
  // would, so release it here.
  PyTypeObject* type = Py_TYPE(op);
  // call the superclass destructor
#if PY_VERSION_HEX >= 0x030A0000
  PyTypeObject* base = (PyTypeObject*)PyType_GetSlot(type, Py_tp_base);
  if (base)
  {
    destructor dtor = (destructor)PyType_GetSlot(base, Py_tp_dealloc);
    dtor(op);
  }
#else
  PyVTKNamespace_Type->tp_base->tp_dealloc(op);
#endif
  Py_DECREF((PyObject*)type);
}

//------------------------------------------------------------------------------
// Heap type created with PyType_FromSpec, as a subclass of the module type.
PyTypeObject* PyVTKNamespace_Type = nullptr;

static PyType_Slot PyVTKNamespace_Slots[] = {
  { Py_tp_dealloc, (void*)PyVTKNamespace_Delete },
  { Py_tp_doc, (void*)PyVTKNamespace_Doc },
  { 0, nullptr },
};

static PyType_Spec PyVTKNamespace_Spec = { "vtkmodules.vtkCommonCore.namespace", // tp_name
  0, // tp_basicsize (inherited from PyModule_Type)
  0, // tp_itemsize
  Py_TPFLAGS_DEFAULT, PyVTKNamespace_Slots };

static PyTypeObject* PyVTKNamespace_GetType()
{
  if (PyVTKNamespace_Type == nullptr)
  {
    PyObject* bases = PyTuple_Pack(1, (PyObject*)&PyModule_Type);
    if (bases)
    {
      PyVTKNamespace_Type = (PyTypeObject*)PyType_FromSpecWithBases(&PyVTKNamespace_Spec, bases);
      Py_DECREF(bases);
    }
  }
  return PyVTKNamespace_Type;
}

//------------------------------------------------------------------------------
PyObject* PyVTKNamespace_New(const char* name)
{
  // first check to see if this namespace exists
  PyObject* self = vtkPythonUtil::FindNamespace(name);
  if (self)
  {
    Py_INCREF(self);
  }
  else
  {
    PyTypeObject* pytype = PyVTKNamespace_GetType();
    if (pytype == nullptr)
    {
      return nullptr;
    }
    // call the superclass (module type) new and init functions
    newfunc moduleNew = (newfunc)PyType_GetSlot(&PyModule_Type, Py_tp_new);
    initproc moduleInit = (initproc)PyType_GetSlot(&PyModule_Type, Py_tp_init);
    PyObject* empty = PyTuple_New(0);
    self = moduleNew(pytype, empty, nullptr);
    Py_DECREF(empty);
    // call the superclass init function
    PyObject* pyname = PyUnicode_FromString(name);
    PyObject* args = PyTuple_Pack(1, pyname);
    Py_DECREF(pyname);
    moduleInit(self, args, nullptr);
    Py_DECREF(args);
    // remember the object for later reference
    vtkPythonUtil::AddNamespaceToMap(self);
  }
  return self;
}

//------------------------------------------------------------------------------
PyObject* PyVTKNamespace_GetDict(PyObject* self)
{
  return PyModule_GetDict(self);
}

//------------------------------------------------------------------------------
const char* PyVTKNamespace_GetName(PyObject* self)
{
  return PyModule_GetName(self);
}
