// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "PyVTKExtras.h"
#include "PyVTKReference.h"
#include "vtkABINamespace.h"
#include "vtkPythonCompatibility.h"

// Silence warning like
// "dereferencing type-punned pointer will break strict-aliasing rules"
// it happens because this kind of expression: (long *)&ptr
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

//------------------------------------------------------------------------------
// Helper function for the buffer_shared() check: get the pointer and
// size (in bytes) of the buffer of the provided object.  A return
// value of zero indicates that an exception was raised.
static void* buffer_pointer_and_size(PyObject* o, Py_ssize_t* size)
{
  void* ptr = nullptr;

  // New buffer protocol
  Py_buffer view = { nullptr, nullptr, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr, nullptr };
  if (PyObject_CheckBuffer(o))
  {
    // Check for a simple buffer
    if (PyObject_GetBuffer(o, &view, PyBUF_SIMPLE) == -1)
    {
      // Check for a C or Fortran contiguous buffer
      PyErr_Clear();
      if (PyObject_GetBuffer(o, &view, PyBUF_STRIDES) == -1)
      {
        return nullptr;
      }
    }

    ptr = view.buf;
    *size = view.len;

    PyBuffer_Release(&view);

    if (ptr)
    {
      return ptr;
    }
  }

  PyErr_SetString(PyExc_TypeError, "object does not have a readable buffer");

  return nullptr;
}

//------------------------------------------------------------------------------
static PyObject* PyVTKExtras_buffer_shared(PyObject*, PyObject* args)
{
  PyObject* ob[2] = { nullptr, nullptr };
  if (PyArg_UnpackTuple(args, "buffer_shared", 2, 2, &ob[0], &ob[1]))
  {
    void* ptr[2] = { nullptr, nullptr };
    Py_ssize_t size[2] = { 0, 0 };
    for (int i = 0; i < 2; i++)
    {
      ptr[i] = buffer_pointer_and_size(ob[i], &size[i]);
      if (!ptr[i])
      {
        break;
      }
    }
    // check if the pointers and memory size are equal
    if (ptr[0] && ptr[1])
    {
      if (ptr[0] == ptr[1] && size[0] == size[1])
      {
        Py_RETURN_TRUE;
      }
      else
      {
        Py_RETURN_FALSE;
      }
    }
  }
  return nullptr;
}

//------------------------------------------------------------------------------
static PyMethodDef PyVTKExtras_Methods[] = {
  { "buffer_shared", PyVTKExtras_buffer_shared, METH_VARARGS,
    "Check if two objects share the same buffer, meaning that they"
    " point to the same block of memory.  An TypeError exception will"
    " be raised if either of the objects does not provide a buffer." },
  { nullptr, nullptr, 0, nullptr }
};

//------------------------------------------------------------------------------
// Exported method called by vtkCommonCorePythonInit
void PyVTKAddFile_PyVTKExtras(PyObject* dict)
{
  // It is necessary to call PyType_Ready() on all subclasses
  PyType_Ready(&PyVTKNumberReference_Type);
  PyType_Ready(&PyVTKStringReference_Type);
  PyType_Ready(&PyVTKTupleReference_Type);

  // Add the "mutable" object (used for C++ pass-by-reference)
  PyObject* o = (PyObject*)&PyVTKReference_Type;
  PyDict_SetItemString(dict, "reference", o); // new name (as of VTK 8.1)
  PyDict_SetItemString(dict, "mutable", o);   // old name

  for (PyMethodDef* meth = PyVTKExtras_Methods; meth->ml_name != nullptr; meth++)
  {
    // Third argument would be the module object, but all we have is
    // the module's dict, and it's safe to set it to nullptr.
    o = PyCFunction_NewEx(meth, nullptr, nullptr);
    if (o && PyDict_SetItemString(dict, meth->ml_name, o) != 0)
    {
      Py_DECREF(o);
    }
  }
}
