/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PyVTKExtras.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "PyVTKExtras.h"
#include "vtkPythonCompatibility.h"
#include "PyVTKMutableObject.h"

// Silence warning like
// "dereferencing type-punned pointer will break strict-aliasing rules"
// it happens because this kind of expression: (long *)&ptr
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

//--------------------------------------------------------------------
// Helper function for the buffer_shared() check: get the pointer and
// size (in bytes) of the buffer of the provided object.  A return
// value of zero indicates that an exception was raised.
static void *buffer_pointer_and_size(PyObject *o, Py_ssize_t *size)
{
  void *ptr = NULL;

#if PY_VERSION_HEX >= 0x02060000
  // New buffer protocol
  Py_buffer view = VTK_PYBUFFER_INITIALIZER;
  if (PyObject_CheckBuffer(o))
  {
    // Check for a simple buffer
    if (PyObject_GetBuffer(o, &view, PyBUF_SIMPLE) == -1)
    {
      // Check for a C or Fortran contiguous buffer
      PyErr_Clear();
      if (PyObject_GetBuffer(o, &view, PyBUF_STRIDES) == -1)
      {
        return NULL;
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
#endif

#ifndef VTK_PY3K
  // Old buffer protocol
  PyBufferProcs *b = Py_TYPE(o)->tp_as_buffer;
  if (b && b->bf_getreadbuffer && b->bf_getsegcount)
  {
    if (b->bf_getsegcount(o, NULL) == 1)
    {
      *size = b->bf_getreadbuffer(o, 0, &ptr);
      if (ptr)
      {
        return ptr;
      }
    }
    else
    {
      PyErr_SetString(PyExc_TypeError, "buffer must be single-segment");
      return NULL;
    }
  }
#endif

  PyErr_SetString(PyExc_TypeError,
    "object does not have a readable buffer");

  return NULL;
}

//--------------------------------------------------------------------
static PyObject *PyVTKExtras_buffer_shared(PyObject *, PyObject *args)
{
  PyObject *ob[2] = { NULL, NULL };
  if (PyArg_UnpackTuple(args, "buffer_shared", 2, 2, &ob[0], &ob[1]))
  {
    void *ptr[2] = { NULL, NULL };
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
  return NULL;
}

//--------------------------------------------------------------------
static PyMethodDef PyVTKExtras_Methods[] = {
  {"buffer_shared", PyVTKExtras_buffer_shared, METH_VARARGS,
   "Check if two objects share the same buffer, meaning that they"
   " point to the same block of memory.  An TypeError exception will"
   " be raised if either of the objects does not provide a buffer."},
  {NULL, NULL, 0, NULL}
};

//--------------------------------------------------------------------
// Exported method called by vtkCommonCorePythonInit
void PyVTKAddFile_PyVTKExtras(PyObject *dict)
{
  PyObject *o = (PyObject *)&PyVTKMutableObject_Type;

  if (o && PyDict_SetItemString(dict, "mutable", o) != 0)
  {
    Py_DECREF(o);
  }

  for (PyMethodDef *meth = PyVTKExtras_Methods;
       meth->ml_name != NULL;
       meth++)
  {
    // Third argument would be the module object, but all we have is
    // the module's dict, and it's safe to set it to NULL.
    o = PyCFunction_NewEx(meth, NULL, NULL);
    if (o && PyDict_SetItemString(dict, meth->ml_name, o) != 0)
    {
      Py_DECREF(o);
    }
  }
}
