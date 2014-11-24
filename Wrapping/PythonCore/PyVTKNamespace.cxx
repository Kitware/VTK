/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PyVTKNamespace.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PyVTKNamespace was created in Nov 2014 by David Gobbi.

  This is a PyModule subclass for wrapping C++ namespaces.
-----------------------------------------------------------------------*/

#include "PyVTKNamespace.h"
#include "vtkPythonUtil.h"

// Silence warning like
// "dereferencing type-punned pointer will break strict-aliasing rules"
// it happens because this kind of expression: (long *)&ptr
// pragma GCC diagnostic is available since gcc>=4.2
#if defined(__GNUC__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=2)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

//--------------------------------------------------------------------

const char *PyVTKNamespace_Doc =
  "A python module that wraps a C++ namespace.\n";

//--------------------------------------------------------------------
static void PyVTKNamespace_Delete(PyObject *op)
{
  // remove from the map so that there is no dangling reference
  vtkPythonUtil::RemoveNamespaceFromMap(op);
  // call the superclass destructor
  PyVTKNamespace_Type.tp_base->tp_dealloc(op);
}

//--------------------------------------------------------------------
PyTypeObject PyVTKNamespace_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  (char*)"vtk.namespace",                // tp_name
  0,                                     // tp_basicsize
  0,                                     // tp_itemsize
  PyVTKNamespace_Delete,                 // tp_dealloc
  0,                                     // tp_print
  0,                                     // tp_getattr
  0,                                     // tp_setattr
  0,                                     // tp_compare
  0,                                     // tp_repr
  0,                                     // tp_as_number
  0,                                     // tp_as_sequence
  0,                                     // tp_as_mapping
  0,                                     // tp_hash
  0,                                     // tp_call
  0,                                     // tp_string
  0,                                     // tp_getattro
  0,                                     // tp_setattro
  0,                                     // tp_as_buffer
  Py_TPFLAGS_DEFAULT,                    // tp_flags
  (char*)PyVTKNamespace_Doc,             // tp_doc
  0,                                     // tp_traverse
  0,                                     // tp_clear
  0,                                     // tp_richcompare
  0,                                     // tp_weaklistoffset
  0,                                     // tp_iter
  0,                                     // tp_iternext
  0,                                     // tp_methods
  0,                                     // tp_members
  0,                                     // tp_getset
  &PyModule_Type,                        // tp_base
  0,                                     // tp_dict
  0,                                     // tp_descr_get
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

//--------------------------------------------------------------------
PyObject *PyVTKNamespace_New(const char *name)
{
  // first check to see if this namespace exists
  PyObject *self = vtkPythonUtil::FindNamespace(name);
  if (self)
    {
    Py_INCREF(self);
    }
  else
    {
    // make sure python has readied the type object
    PyType_Ready(&PyVTKNamespace_Type);
    // call the allocator provided by python for this type
    self = PyVTKNamespace_Type.tp_alloc(&PyVTKNamespace_Type, 0);
    // call the superclass init function
    PyObject *args = PyTuple_New(1);
    PyTuple_SET_ITEM(args, 0, PyString_FromString(name));
    PyVTKNamespace_Type.tp_base->tp_init(self, args, 0);
    Py_DECREF(args);
    // remember the object for later reference
    vtkPythonUtil::AddNamespaceToMap(self);
    }
  return self;
}

//--------------------------------------------------------------------
PyObject *PyVTKNamespace_GetDict(PyObject *self)
{
  return PyModule_GetDict(self);
}

//--------------------------------------------------------------------
const char *PyVTKNamespace_GetName(PyObject *self)
{
  return PyModule_GetName(self);
}
