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
  The methods are stored in the __dict__ of the associated type objects.
  Each PyVTKObject also has a __dict__ of its own that can be used to
  store arbitrary attributes.

  Memory management is done as follows. Each PyVTKObject has
  an entry along with a smart pointer to its vtkObjectBase in
  the vtkPythonUtil::ObjectMap.  When a PyVTKObject is destructed,
  it is removed along with the smart pointer from the ObjectMap.
-----------------------------------------------------------------------*/

#include "PyVTKObject.h"
#include "PyVTKMethodDescriptor.h"
#include "vtkPythonUtil.h"
#include "vtkObjectBase.h"
#include "vtkDataArray.h"
#include "vtkPythonCommand.h"

#include <stddef.h>
#include <sstream>

// This will be set to the python type struct for vtkObjectBase
static PyTypeObject *PyVTKObject_Type = 0;

//--------------------------------------------------------------------
PyVTKClass::PyVTKClass(
  PyTypeObject *typeobj, PyMethodDef *methods,
  const char *classname, vtknewfunc constructor)
{
  this->py_type = typeobj;
  this->py_methods = methods;
  this->vtk_name = classname;
  this->vtk_new = constructor;
}

//--------------------------------------------------------------------
// C API

//--------------------------------------------------------------------
// Add a class, add methods and members to its type object.  A return
// value of NULL signifies that the class was already added.
PyVTKClass *PyVTKClass_Add(
  PyTypeObject *pytype, PyMethodDef *methods,
  const char *classname, const char *docstring[],
  vtknewfunc constructor)
{
  // Add this type to the vtk class map
  PyVTKClass *info =
    vtkPythonUtil::AddClassToMap(
      pytype, methods, classname, constructor);

  if (info == 0)
  {
    // The class was already in the map, so do nothing
    return info;
  }

  // Cache the type object for vtkObjectBase for quick access
  if (PyVTKObject_Type == 0 && strcmp(classname, "vtkObjectBase") == 0)
  {
    PyVTKObject_Type = pytype;
  }

  // Create the dict
  if (pytype->tp_dict == 0)
  {
    pytype->tp_dict = PyDict_New();
  }

  // Add the docstring to the type
  PyObject *doc = vtkPythonUtil::BuildDocString(docstring);
  PyDict_SetItemString(pytype->tp_dict, "__doc__", doc);
  Py_DECREF(doc);

  // Add special attribute __vtkname__
  PyObject *s = PyString_FromString(classname);
  PyDict_SetItemString(pytype->tp_dict, "__vtkname__", s);
  Py_DECREF(s);

  // Add all of the methods
  for (PyMethodDef *meth = methods; meth && meth->ml_name; meth++)
  {
    PyObject *func = PyVTKMethodDescriptor_New(pytype, meth);
    PyDict_SetItemString(pytype->tp_dict, meth->ml_name, func);
    Py_DECREF(func);
  }

  return info;
}

//--------------------------------------------------------------------
int PyVTKObject_Check(PyObject *op)
{
  return PyObject_TypeCheck(op, PyVTKObject_Type);
}

//--------------------------------------------------------------------
// Object protocol

//--------------------------------------------------------------------
PyObject *PyVTKObject_String(PyObject *op)
{
  std::ostringstream vtkmsg_with_warning_C4701;
  ((PyVTKObject *)op)->vtk_ptr->Print(vtkmsg_with_warning_C4701);
  vtkmsg_with_warning_C4701.put('\0');
  PyObject *res = PyString_FromString(vtkmsg_with_warning_C4701.str().c_str());
  return res;
}

//--------------------------------------------------------------------
PyObject *PyVTKObject_Repr(PyObject *op)
{
  char buf[255];
  sprintf(buf, "(%.200s)%p", Py_TYPE(op)->tp_name, static_cast<void*>(op));

  return PyString_FromString(buf);
}

//--------------------------------------------------------------------
int PyVTKObject_Traverse(PyObject *o, visitproc visit, void *arg)
{
  PyVTKObject *self = (PyVTKObject *)o;
  int err = 0;

  if (self->vtk_observers != 0)
  {
    unsigned long *olist = self->vtk_observers;
    while (err == 0 && *olist != 0)
    {
      vtkObject *op = static_cast<vtkObject *>(self->vtk_ptr);
      vtkCommand *c = op->GetCommand(*olist);
      if (c == 0)
      {
        // observer is gone, remove from list
        unsigned long *tmp = olist;
        do { tmp++; } while (*tmp != 0);
        *olist = *--tmp;
        *tmp = 0;
      }
      else
      {
        // visit the observer
        vtkPythonCommand *cbc = static_cast<vtkPythonCommand *>(c);
        err = visit(cbc->obj, arg);
        olist++;
      }
    }
  }

  return err;
}

//--------------------------------------------------------------------
PyObject *PyVTKObject_New(PyTypeObject *tp, PyObject *args, PyObject *kwds)
{
  // If type was sublassed within python, then skip arg checks and
  // simply create a new object.
  if ((tp->tp_flags & Py_TPFLAGS_HEAPTYPE) == 0)
  {
    if (kwds != NULL && PyDict_Size(kwds))
    {
      PyErr_SetString(PyExc_TypeError,
                      "this function takes no keyword arguments");
      return NULL;
    }

    PyObject *o = 0;
    if (!PyArg_UnpackTuple(args, tp->tp_name, 0, 1, &o))
    {
      return NULL;
    }

    if (o)
    {
      // used to create a VTK object from a SWIG pointer
      return vtkPythonUtil::GetObjectFromObject(
        o, vtkPythonUtil::StripModule(tp->tp_name));
    }
  }

  // if PyVTKObject_FromPointer gets NULL, it creates a new object.
  return PyVTKObject_FromPointer(tp, NULL, NULL);
}

//--------------------------------------------------------------------
void PyVTKObject_Delete(PyObject *op)
{
  PyVTKObject *self = (PyVTKObject *)op;

  PyObject_GC_UnTrack(op);

  if (self->vtk_weakreflist != NULL)
  {
    PyObject_ClearWeakRefs(op);
  }

  // A python object owning a VTK object reference is getting
  // destroyed.  Remove the python object's VTK object reference.
  vtkPythonUtil::RemoveObjectFromMap(op);

  Py_DECREF(self->vtk_dict);
  delete [] self->vtk_observers;
  delete [] self->vtk_buffer;

  PyObject_GC_Del(op);
}

//--------------------------------------------------------------------
// This defines any special attributes of wrapped VTK objects.

static PyObject *PyVTKObject_GetDict(PyObject *op, void *)
{
  PyVTKObject *self = (PyVTKObject *)op;
  Py_INCREF(self->vtk_dict);
  return self->vtk_dict;
}

static PyObject *PyVTKObject_GetThis(PyObject *op, void *)
{
  PyVTKObject *self = (PyVTKObject *)op;
  const char *classname = self->vtk_ptr->GetClassName();
  const char *cp = classname;
  char buf[1024];
  // check to see if classname is a valid python identifier
  if (isalpha(*cp) || *cp == '_')
  {
    do { cp++; } while (isalnum(*cp) || *cp == '_');
  }
  // otherwise, use the pythonic form of the class name
  if (*cp != '\0')
  {
    classname = vtkPythonUtil::StripModule(Py_TYPE(op)->tp_name);
  }
  sprintf(buf, "p_%.500s", classname);
  return PyString_FromString(
    vtkPythonUtil::ManglePointer(self->vtk_ptr, buf));
}

PyGetSetDef PyVTKObject_GetSet[] = {
  { (char *)"__dict__", PyVTKObject_GetDict, 0,
    (char *)"Dictionary of attributes set by user.", 0 },
  { (char *)"__this__", PyVTKObject_GetThis, 0,
    (char *)"Pointer to the C++ object.", 0 },
  { 0, 0, 0, 0, 0 }
};

//--------------------------------------------------------------------
// The following methods and struct define the "buffer" protocol
// for PyVTKObject, so that python can read from a vtkDataArray.
// This is particularly useful for NumPy.

#ifndef VTK_PY3K
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

#endif

#if PY_VERSION_HEX >= 0x02060000

//--------------------------------------------------------------------
// Convert a VTK type to a python type char (struct module)
static const char *pythonTypeFormat(int t)
{
  const char *b = 0;

  switch (t)
  {
    case VTK_CHAR: b = "c"; break;
    case VTK_SIGNED_CHAR: b = "b"; break;
    case VTK_UNSIGNED_CHAR: b = "B"; break;
    case VTK_SHORT: b = "h"; break;
    case VTK_UNSIGNED_SHORT: b = "H"; break;
    case VTK_INT: b = "i"; break;
    case VTK_UNSIGNED_INT: b = "I"; break;
    case VTK_LONG: b = "l"; break;
    case VTK_UNSIGNED_LONG: b = "L"; break;
    case VTK_LONG_LONG: b = "q"; break;
    case VTK_UNSIGNED_LONG_LONG: b = "Q"; break;
    case VTK___INT64: b = "q"; break;
    case VTK_UNSIGNED___INT64: b = "Q"; break;
    case VTK_FLOAT: b = "f"; break;
    case VTK_DOUBLE: b = "d"; break;
#ifndef VTK_USE_64BIT_IDS
    case VTK_ID_TYPE: b = "i"; break;
#else
    case VTK_ID_TYPE: b = "q"; break;
#endif
  }

  return b;
}

//--------------------------------------------------------------------
static int
PyVTKObject_AsBuffer_GetBuffer(PyObject *obj, Py_buffer *view, int flags)
{
  PyVTKObject *self = (PyVTKObject*)obj;
  vtkDataArray *da = vtkDataArray::SafeDownCast(self->vtk_ptr);
  if (da)
  {
    void *ptr = da->GetVoidPointer(0);
    Py_ssize_t ntuples = da->GetNumberOfTuples();
    int ncomp = da->GetNumberOfComponents();
    int dsize = da->GetDataTypeSize();
    const char *format = pythonTypeFormat(da->GetDataType());
    Py_ssize_t size = ntuples*ncomp*dsize;

    if (da->GetDataType() == VTK_BIT)
    {
      size = (ntuples*ncomp + 7)/8;
    }

    // start by building a basic "unsigned char" buffer
    if (PyBuffer_FillInfo(view, obj, ptr, size, 0, flags) == -1)
    {
      return -1;
    }
    // check if a dimensioned array was requested
    if (format != 0 && (flags & PyBUF_ND) != 0)
    {
      // first, build a simple 1D array
      view->itemsize = dsize;
      view->ndim = (ncomp > 1 ? 2 : 1);
      view->format = (char *)format;

#if PY_VERSION_HEX >= 0x02070000 && PY_VERSION_HEX < 0x03030000
      // use "smalltable" for 1D arrays, like memoryobject.c
      view->shape = view->smalltable;
      view->strides = &view->smalltable[1];
      if (view->ndim > 1)
#endif
      {
        if (self->vtk_buffer && self->vtk_buffer[0] != view->ndim)
        {
          delete [] self->vtk_buffer;
          self->vtk_buffer = 0;
        }
        if (self->vtk_buffer == 0)
        {
          self->vtk_buffer = new Py_ssize_t[2*view->ndim + 1];
          self->vtk_buffer[0] = view->ndim;
        }
        view->shape = &self->vtk_buffer[1];
        view->strides = &self->vtk_buffer[view->ndim + 1];
      }

      if (view->ndim == 1)
      {
        // simple one-dimensional array
        view->shape[0] = ntuples*ncomp;
        view->strides[0] = view->itemsize;
      }
      else
      {
        // use native C dimension ordering by default
        char order = 'C';
        if ((flags & PyBUF_ANY_CONTIGUOUS) == PyBUF_F_CONTIGUOUS)
        {
          // use fortran ordering only if explicitly requested
          order = 'F';
        }
        // need to allocate space for the strides and shape
        view->shape[0] = ntuples;
        view->shape[1] = ncomp;
        if (order == 'F')
        {
          view->shape[0] = ncomp;
          view->shape[1] = ntuples;
        }
        PyBuffer_FillContiguousStrides(
          view->ndim, view->shape, view->strides, dsize, order);
      }
    }
    return 0;
  }

  PyErr_Format(PyExc_ValueError,
               "Cannot get a buffer from %s.", Py_TYPE(obj)->tp_name);
  return -1;
}

//--------------------------------------------------------------------
static void
PyVTKObject_AsBuffer_ReleaseBuffer(PyObject *obj, Py_buffer *view)
{
  // nothing to do, the caller will decref the obj
  (void)obj;
  (void)view;
}

#endif

//--------------------------------------------------------------------
PyBufferProcs PyVTKObject_AsBuffer = {
#ifndef VTK_PY3K
  PyVTKObject_AsBuffer_GetReadBuf,       // bf_getreadbuffer
  PyVTKObject_AsBuffer_GetWriteBuf,      // bf_getwritebuffer
  PyVTKObject_AsBuffer_GetSegCount,      // bf_getsegcount
  0,                                     // bf_getcharbuffer
#endif
#if PY_VERSION_HEX >= 0x02060000
  PyVTKObject_AsBuffer_GetBuffer,        // bf_getbuffer
  PyVTKObject_AsBuffer_ReleaseBuffer     // bf_releasebuffer
#endif
};

//--------------------------------------------------------------------
PyObject *PyVTKObject_FromPointer(
  PyTypeObject *pytype, PyObject *pydict, vtkObjectBase *ptr)
{
  // This will be set if we create a new C++ object
  bool created = false;
  std::string classname = vtkPythonUtil::StripModule(pytype->tp_name);
  PyVTKClass *cls = 0;

  if (ptr)
  {
    // If constructing from an existing C++ object, use its actual class
    classname = ptr->GetClassName();
    cls = vtkPythonUtil::FindClass(classname.c_str());
  }

  if (cls == 0)
  {
    // Use the vtkname of the supplied class type
    PyObject *s = PyObject_GetAttrString((PyObject *)pytype, "__vtkname__");
    if (s)
    {
#ifdef VTK_PY3K
      PyObject *tmp = PyUnicode_AsUTF8String(s);
      if (tmp)
      {
        Py_DECREF(s);
        s = tmp;
      }
#endif
      const char *vtkname_classname = PyBytes_AsString(s);
      if (vtkname_classname == 0)
      {
        Py_DECREF(s);
        return NULL;
      }
      classname = vtkname_classname;
      Py_DECREF(s);
    }
    cls = vtkPythonUtil::FindClass(classname.c_str());
    if (cls == 0)
    {
      PyErr_Format(PyExc_ValueError,
                   "internal error, unknown VTK class %.200s",
                   classname.c_str());
      return NULL;
    }
  }

  if (!ptr)
  {
    // Create a new instance of this class since we were not given one.
    if (cls->vtk_new)
    {
      ptr = cls->vtk_new();
      if (!ptr)
      {
        // The vtk_new() method returns null when a factory class has no
        // implementation (i.e. cannot provide a concrete class instance.)
        // NotImplementedError indicates a pure virtual method call.
        PyErr_SetString(
          PyExc_NotImplementedError,
          "no concrete implementation exists for this class");
        return 0;
      }
      created = true;

      // Check the type of the newly-created object
      const char *newclassname = ptr->GetClassName();
      if (std::string(newclassname) != classname)
      {
        PyVTKClass *newclass = vtkPythonUtil::FindClass(newclassname);
        if (newclass)
        {
          classname = newclassname;
          cls = newclass;
        }
      }
    }
    else
    {
      PyErr_SetString(
        PyExc_TypeError,
        "this is an abstract class and cannot be instantiated");
      return 0;
    }
  }

  if ((pytype->tp_flags & Py_TPFLAGS_HEAPTYPE) != 0)
  {
    // Incref if class was declared in python (see PyType_GenericAlloc).
    Py_INCREF(pytype);
  }
  else
  {
    // To support factory New methods, use the object's actual class
    pytype = cls->py_type;
  }

  // Create a new dict unless one was provided
  if (pydict)
  {
    Py_INCREF(pydict);
  }
  else
  {
    pydict = PyDict_New();
  }

  PyVTKObject *self = PyObject_GC_New(PyVTKObject, pytype);

  self->vtk_ptr = ptr;
  self->vtk_flags = 0;
  self->vtk_class = cls;
  self->vtk_dict = pydict;
  self->vtk_buffer = 0;
  self->vtk_observers = 0;
  self->vtk_weakreflist = NULL;

  PyObject_GC_Track((PyObject *)self);

  // A python object owning a VTK object reference is getting
  // created.  Add the python object's VTK object reference.
  vtkPythonUtil::AddObjectToMap((PyObject *)self, ptr);

  // The hash now owns a reference so we can free ours.
  if (created)
  {
    ptr->Delete();
  }

  return (PyObject *)self;
}

vtkObjectBase *PyVTKObject_GetObject(PyObject *obj)
{
  return ((PyVTKObject *)obj)->vtk_ptr;
}

void PyVTKObject_AddObserver(PyObject *obj, unsigned long id)
{
  unsigned long *olist = ((PyVTKObject *)obj)->vtk_observers;
  unsigned long n = 0;
  if (olist == 0)
  {
    olist = new unsigned long[8];
    ((PyVTKObject *)obj)->vtk_observers = olist;
  }
  else
  {
    // count the number of items
    while (olist[n] != 0) { n++; }
    // check if n+1 is a power of two (base allocation is 8)
    unsigned long m = n+1;
    if (m >= 8 && (n & m) == 0)
    {
      unsigned long *tmp = olist;
      olist = new unsigned long[2*m];
      for (unsigned long i = 0; i < n; i++)
      {
        olist[i] = tmp[i];
      }
      delete [] tmp;
      ((PyVTKObject *)obj)->vtk_observers = olist;
    }
  }
  olist[n++] = id;
  olist[n] = 0;
}

unsigned int PyVTKObject_GetFlags(PyObject *obj)
{
  return ((PyVTKObject *)obj)->vtk_flags;
}

void PyVTKObject_SetFlag(PyObject *obj, unsigned int flag, int val)
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
