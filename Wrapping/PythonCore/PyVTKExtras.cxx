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
// A release callback for memory a Python object keeps alive, for C++ code
// that holds the memory past any Python lifetime (vtkMemoryDescriptor and
// what it hands its release to). The context is a callable with a reference
// of its own; it is called with no arguments, then that reference dropped.
//
// A ctypes callback cannot be used there: C++ objects that Python wrapped can
// be destroyed after the interpreter has finalized (vtkPythonUtil deletes the
// ones still alive at exit from a Py_AtExit hook), and a ctypes callback then
// enters a dead interpreter. This checks first, and leaks the hold instead,
// which at exit is harmless.
//
// Py_IsInitialized() is false from the moment finalization starts (CPython
// clears the flag as it marks the runtime finalizing), so it is the whole
// check, and it is in the Limited API, unlike Py_IsFinalizing().
static void PyVTKExtras_ReleaseCallable(void* context)
{
  if (!context || !Py_IsInitialized())
  {
    return;
  }
  PyGILState_STATE state = PyGILState_Ensure();
  PyObject* callable = static_cast<PyObject*>(context);
  PyObject* result = PyObject_CallObject(callable, nullptr);
  if (result)
  {
    Py_DECREF(result);
  }
  else
  {
    PyErr_WriteUnraisable(callable);
  }
  Py_DECREF(callable);
  PyGILState_Release(state);
}

//------------------------------------------------------------------------------
static PyObject* PyVTKExtras_release_callable_address(PyObject*, PyObject*)
{
  return PyLong_FromVoidPtr(reinterpret_cast<void*>(&PyVTKExtras_ReleaseCallable));
}

//------------------------------------------------------------------------------
static PyMethodDef PyVTKExtras_Methods[] = {
  { "buffer_shared", PyVTKExtras_buffer_shared, METH_VARARGS,
    "Check if two objects share the same buffer, meaning that they"
    " point to the same block of memory.  An TypeError exception will"
    " be raised if either of the objects does not provide a buffer." },
  { "release_callable_address", PyVTKExtras_release_callable_address, METH_NOARGS,
    "Address of a C release function for vtkMemoryDescriptor.SetReleaseAddress."
    " Its context is the address of a callable holding a reference of its own"
    " (Py_IncRef it first): the callable is called with no arguments when the"
    " memory is released, and that reference dropped. Unlike a ctypes callback,"
    " it does nothing once the interpreter is finalizing or gone." },
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
