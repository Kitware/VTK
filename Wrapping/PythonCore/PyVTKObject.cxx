/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PyVTKObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PyVTKObject was created in Oct 2000 by David Gobbi for VTK 3.2.
  Support for weakref added in July 2005 by Prabhu Ramachandran.
  Buffer interface for vtk arrays added in Feb 2008 by Berk Geveci.

  A PyVTKObject is a python object that represents a VTK object.
  Its methods are stored in the __dict__ of its associated PyVTKClass
  and superclasses.  The PyVTKObject also has a __dict__ of its own
  that can be used to store arbitrary attributes.

  Memory management is done as follows. Each PyVTKObject has
  an entry along with a smart pointer to its vtkObjectBase in
  the vtkPythonUtil::ObjectMap.  When a PyVTKObject is destructed,
  it is removed along with the smart pointer from the ObjectMap.
-----------------------------------------------------------------------*/

#include "PyVTKObject.h"
#include "vtkPythonUtil.h"
#include "vtkObjectBase.h"
#include "vtkDataArray.h"

#include <stddef.h>
#include <vtksys/ios/sstream>
#include <vtksys/cstddef>

//--------------------------------------------------------------------
static PyObject *PyVTKObject_String(PyObject *op)
{
  PyObject *func = PyObject_GetAttrString(op, (char*)"__str__");

  if (func)
    {
    PyObject *res = PyEval_CallObject(func, (PyObject *)NULL);
    Py_DECREF(func);
    return res;
    }
  PyErr_Clear();

  vtksys_ios::ostringstream vtkmsg_with_warning_C4701;
  ((PyVTKObject *)op)->vtk_ptr->Print(vtkmsg_with_warning_C4701);
  vtkmsg_with_warning_C4701.put('\0');
  PyObject *res = PyString_FromString(vtkmsg_with_warning_C4701.str().c_str());
  return res;
}

//--------------------------------------------------------------------
static PyObject *PyVTKObject_Repr(PyObject *op)
{
  PyObject *func = PyObject_GetAttrString(op, (char*)"__repr__");

  if (func)
    {
    PyObject *res = PyEval_CallObject(func, (PyObject *)NULL);
    Py_DECREF(func);
    return res;
    }
  PyErr_Clear();

  char buf[255];
  sprintf(buf,"(%s)%p",
          PyString_AS_STRING(((PyVTKObject *)op)->vtk_class->vtk_name),
          op);

  return PyString_FromString(buf);
}

//--------------------------------------------------------------------
int PyVTKObject_SetAttr(PyObject *op, PyObject *attr, PyObject *value)
{
  PyVTKObject *self = (PyVTKObject *)op;
  char *name = PyString_AsString(attr);

  if (name[0] == '_' && name[1] == '_')
    {
    if (strcmp(name, "__dict__") == 0)
      {
      PyErr_SetString(PyExc_RuntimeError,
                      "__dict__ is a read-only attribute");
      return -1;
      }
    if (strcmp(name, "__class__") == 0)
      {
      PyErr_SetString(PyExc_RuntimeError,
                      "__class__ is a read-only attribute");
      return -1;
      }
    }

  if (value)
    {
    PyObject *func = self->vtk_class->vtk_setattr;
    if (func)
      {
      PyObject *args = Py_BuildValue((char*)"(OOO)", self, attr, value);
      PyObject *res = PyEval_CallObject(func, args);
      Py_DECREF(args);
      if (res)
        {
        Py_DECREF(res);
        return 0;
        }
      return -1;
      }
    return PyDict_SetItem(self->vtk_dict, attr, value);
    }
  else
    {
    PyObject *func = self->vtk_class->vtk_delattr;
    if (func)
      {
      PyObject *args = Py_BuildValue((char*)"(OO)", self, attr);
      PyObject *res = PyEval_CallObject(func, args);
      Py_DECREF(args);
      if (res)
        {
        Py_DECREF(res);
        return 0;
        }
      return -1;
      }
    int rv = PyDict_DelItem(self->vtk_dict, attr);
    if (rv < 0)
      {
      PyErr_SetString(PyExc_AttributeError,
                      "delete non-existing class attribute");
      }
    return rv;
    }
}

//--------------------------------------------------------------------
static PyObject *PyVTKObject_GetAttr(PyObject *op, PyObject *attr)
{
  PyVTKObject *self = (PyVTKObject *)op;
  char *name = PyString_AsString(attr);
  PyVTKClass *pyclass = self->vtk_class;
  PyObject *bases;
  PyObject *value;

  if ((value = PyDict_GetItem(self->vtk_dict, attr)))
    {
    Py_INCREF(value);
    return value;
    }

  if (name[0] == '_')
    {
    if (strcmp(name,"__class__") == 0)
      {
      Py_INCREF(self->vtk_class);
      return (PyObject *)self->vtk_class;
      }

    if (strcmp(name,"__this__") == 0)
      {
      const char *classname = self->vtk_ptr->GetClassName();
      const char *cp = classname;
      char buf[1024];
      if (isalpha(*cp) || *cp == '_')
        {
        do { cp++; } while (isalnum(*cp) || *cp == '_');
        }
      if (*cp != '0')
        {
        classname = ((PyVTKClass *)self->vtk_class)->vtk_mangle;
        }
      sprintf(buf, "p_%.500s", classname);
      return PyString_FromString(
        vtkPythonUtil::ManglePointer(self->vtk_ptr, buf));
      }

    if (strcmp(name,"__doc__") == 0)
      {
      Py_INCREF(pyclass->vtk_doc);
      return pyclass->vtk_doc;
      }

    if (strcmp(name,"__dict__") == 0)
      {
      Py_INCREF(self->vtk_dict);
      return self->vtk_dict;
      }
    }

  while (pyclass != NULL)
    {
    value = PyDict_GetItem(PyVTKClass_GetDict((PyObject *)pyclass), attr);

    if (value)
      {
      if (PyCFunction_Check(value))
        {
        return PyCFunction_New(((PyCFunctionObject *)value)->m_ml,
                               (PyObject *)self);
        }
      else if (PyCallable_Check(value))
        {
        return PyMethod_New(value, (PyObject *)self,
                            (PyObject *)self->vtk_class);
        }
      Py_INCREF(value);
      return value;
      }

    bases = pyclass->vtk_bases;
    pyclass = NULL;
    if (PyTuple_Size(bases))
      {
      pyclass = (PyVTKClass *)PyTuple_GetItem(bases,0);
      }
    }

  // try the __getattr__ attribute if set
  pyclass = self->vtk_class;
  if (pyclass->vtk_getattr)
    {
    PyObject *args = Py_BuildValue((char*)"(OO)", self, attr);
    PyObject *res = PyEval_CallObject(pyclass->vtk_getattr, args);
    Py_DECREF(args);
    return res;
    }

  PyErr_SetString(PyExc_AttributeError, name);
  return NULL;
}

//--------------------------------------------------------------------
#if PY_MAJOR_VERSION >= 2
static int PyVTKObject_Traverse(PyObject *o, visitproc visit, void *arg)
{
  PyVTKObject *self = (PyVTKObject *)o;
  PyObject *members[2];
  int err = 0;
  int i;

  members[0] = (PyObject *)self->vtk_class;
  members[1] = self->vtk_dict;

  for (i = 0; i < 2 && err == 0; i++)
    {
    if (members[i])
      {
      err = visit(members[i], arg);
      }
    }

  return err;
}
#endif

//--------------------------------------------------------------------
static void PyVTKObject_Delete(PyObject *op)
{
  PyVTKObject *self = (PyVTKObject *)op;

#if PY_VERSION_HEX >= 0x02020000
  PyObject_GC_UnTrack(op);
#endif

#if PY_VERSION_HEX >= 0x02010000
  if (self->vtk_weakreflist != NULL)
    {
    PyObject_ClearWeakRefs(op);
    }
#endif

  // A python object owning a VTK object reference is getting
  // destroyed.  Remove the python object's VTK object reference.
  vtkPythonUtil::RemoveObjectFromMap(op);

  Py_DECREF((PyObject *)self->vtk_class);
  Py_DECREF(self->vtk_dict);

#if PY_VERSION_HEX >= 0x02020000
  PyObject_GC_Del(op);
#elif PY_MAJOR_VERSION >= 2
  PyObject_Del(op);
#else
  PyMem_DEL(op);
#endif
}

//--------------------------------------------------------------------
// The following methods and struct define the "buffer" protocol
// for PyVTKObject, so that python can read from a vtkDataArray.
// This is particularly useful for NumPy.

//--------------------------------------------------------------------
static Py_ssize_t
PyVTKObject_AsBuffer_GetSegCount(PyObject *op, Py_ssize_t *lenp)
{
  PyVTKObject *self = (PyVTKObject*)op;
  vtkDataArray *da = vtkDataArray::SafeDownCast(self->vtk_ptr);
  if (da)
    {
    if (lenp)
      {
      *lenp = da->GetNumberOfTuples()*
        da->GetNumberOfComponents()*
        da->GetDataTypeSize();
      }

    return 1;
    }

  if (lenp)
    {
    *lenp = 0;
    }
  return 0;
}

//--------------------------------------------------------------------
static Py_ssize_t
PyVTKObject_AsBuffer_GetReadBuf(
  PyObject *op, Py_ssize_t segment, void **ptrptr)
{
  if (segment != 0)
    {
    PyErr_SetString(PyExc_ValueError,
                    "accessing non-existing array segment");
    return -1;
    }

  PyVTKObject *self = (PyVTKObject*)op;
  vtkDataArray *da = vtkDataArray::SafeDownCast(self->vtk_ptr);
  if (da)
    {
    *ptrptr = da->GetVoidPointer(0);
    return da->GetNumberOfTuples()*
      da->GetNumberOfComponents()*
      da->GetDataTypeSize();
    }
  return -1;
}

//--------------------------------------------------------------------
static Py_ssize_t
PyVTKObject_AsBuffer_GetWriteBuf(
  PyObject *op, Py_ssize_t segment, void **ptrptr)
{
  return PyVTKObject_AsBuffer_GetReadBuf(op, segment, ptrptr);
}

//--------------------------------------------------------------------
static PyBufferProcs PyVTKObject_AsBuffer = {
#if PY_VERSION_HEX >= 0x02050000
  PyVTKObject_AsBuffer_GetReadBuf,       // bf_getreadbuffer
  PyVTKObject_AsBuffer_GetWriteBuf,      // bf_getwritebuffer
  PyVTKObject_AsBuffer_GetSegCount,      // bf_getsegcount
  0,                                     // bf_getcharbuffer
 #if PY_VERSION_HEX >= 0x02060000
  0,                                     // bf_getbuffer
  0                                      // bf_releasebuffer
 #endif
#else
  PyVTKObject_AsBuffer_GetReadBuf,       // bf_getreadbuffer
  PyVTKObject_AsBuffer_GetWriteBuf,      // bf_getwritebuffer
  PyVTKObject_AsBuffer_GetSegCount,      // bf_getsegcount
  0,                                     // bf_getcharbuffer
#endif
};

//--------------------------------------------------------------------
PyTypeObject PyVTKObject_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  (char*)"vtkobject",                    // tp_name
  sizeof(PyVTKObject),                   // tp_basicsize
  0,                                     // tp_itemsize
  PyVTKObject_Delete,                    // tp_dealloc
  0,                                     // tp_print
  0,                                     // tp_getattr
  0,                                     // tp_setattr
  0,                                     // tp_compare
  PyVTKObject_Repr,                      // tp_repr
  0,                                     // tp_as_number
  0,                                     // tp_as_sequence
  0,                                     // tp_as_mapping
  0,                                     // tp_hash
  0,                                     // tp_call
  PyVTKObject_String,                    // tp_string
  PyVTKObject_GetAttr,                   // tp_getattro
  PyVTKObject_SetAttr,                   // tp_setattro
  &PyVTKObject_AsBuffer,                 // tp_as_buffer
#if PY_VERSION_HEX >= 0x02020000
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, // tp_flags
#elif PY_VERSION_HEX >= 0x02010000
  Py_TPFLAGS_HAVE_WEAKREFS,              // tp_flags
#else
  0,                                     // tp_flags
#endif
  (char*)"Use help(x.__class__) to get full documentation.",  // tp_doc
#if PY_MAJOR_VERSION >= 2
  PyVTKObject_Traverse,                  // tp_traverse
  0,                                     // tp_clear
  0,                                     // tp_richcompare
#if PY_VERSION_HEX >= 0x02010000
  offsetof(PyVTKObject, vtk_weakreflist),// tp_weaklistoffset
#else
  0,                                     // tp_weaklistoffset
#endif
#else
  0, 0, 0, 0,                            // reserved
#endif
#if PY_VERSION_HEX >= 0x02020000
  0,                                     // tp_iter
  0,                                     // tp_iternext
  0,                                     // tp_methods
  0,                                     // tp_members
  0,                                     // tp_getset
  0,                                     // tp_base
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
#endif
  VTK_WRAP_PYTHON_SUPRESS_UNINITIALIZED
};

PyObject *PyVTKObject_New(
  PyObject *pyvtkclass, PyObject *pydict, vtkObjectBase *ptr)
{
  PyVTKClass *vtkclass = (PyVTKClass *)pyvtkclass;
  PyObject *dict = 0;
  PyObject *cls = 0;

  bool haveRef = false;
  if (!ptr)
    {
    // Create a new instance of this class since we were not given one.
    if(vtkclass->vtk_new)
      {
      ptr = vtkclass->vtk_new();
      haveRef = true;
      }
    else
      {
      PyErr_SetString(
        PyExc_TypeError,
        (char*)"this is an abstract class and cannot be instantiated");
      return 0;
      }
    }

  // We want to find the class that best matches GetClassName(), since
  // the object might have been created by a factory and might therefore
  // not be of the type of the class that New() was called on.
  // There are two situations where we don't want to do this, though.
  // If pydict is set, then the object is being recreated from a ghost
  // and we must keep the original class. Also, if vtk_methods is NULL
  // then the class is a special custom class and must be kept.
  if (pydict == NULL && vtkclass->vtk_methods != NULL)
    {
    cls = vtkPythonUtil::FindClass(ptr->GetClassName());
    }

  // Use the class that was passed as an argument
  if (cls == NULL)
    {
    cls = (PyObject *)vtkclass;
    }

  Py_INCREF(cls);

  if (pydict)
    {
    Py_INCREF(pydict);
    dict = pydict;
    }
  else
    {
    dict = PyDict_New();
    }

#if PY_VERSION_HEX >= 0x02020000
  PyVTKObject *self = PyObject_GC_New(PyVTKObject, &PyVTKObject_Type);
#elif PY_MAJOR_VERSION >= 2
  PyVTKObject *self = PyObject_New(PyVTKObject, &PyVTKObject_Type);
#else
  PyVTKObject *self = PyObject_NEW(PyVTKObject, &PyVTKObject_Type);
#endif

  self->vtk_ptr = ptr;
  self->vtk_flags = 0;
  self->vtk_class = (PyVTKClass *)cls;
  self->vtk_dict = dict;

#if PY_VERSION_HEX >= 0x02010000
  self->vtk_weakreflist = NULL;
#endif

#if PY_VERSION_HEX >= 0x02020000
  PyObject_GC_Track((PyObject *)self);
#endif

  // A python object owning a VTK object reference is getting
  // created.  Add the python object's VTK object reference.
  vtkPythonUtil::AddObjectToMap((PyObject *)self, ptr);

  // The hash now owns a reference so we can free ours.
  if (haveRef)
    {
    ptr->Delete();
    }

  return (PyObject *)self;
}

vtkObjectBase *PyVTKObject_GetObject(PyObject *obj)
{
  return ((PyVTKObject *)obj)->vtk_ptr;
}

long PyVTKObject_GetFlags(PyObject *obj)
{
  return ((PyVTKObject *)obj)->vtk_flags;
}

void PyVTKObject_SetFlag(PyObject *obj, long flag, int val)
{
  if (val)
    {
    ((PyVTKObject *)obj)->vtk_flags |= flag;
    }
  else
    {
    ((PyVTKObject *)obj)->vtk_flags &= ~flag;
    }
}
