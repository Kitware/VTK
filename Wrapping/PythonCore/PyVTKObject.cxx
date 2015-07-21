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
#include <vtksys/ios/sstream>
#include <vtksys/cstddef>

#if defined(VTK_BUILD_SHARED_LIBS)
# define VTK_PYTHON_EXPORT VTK_ABI_EXPORT
# define VTK_PYTHON_IMPORT VTK_ABI_IMPORT
#else
# define VTK_PYTHON_EXPORT VTK_ABI_EXPORT
# define VTK_PYTHON_IMPORT VTK_ABI_EXPORT
#endif

// This will be set to the python type struct for vtkObjectBase
static PyTypeObject *PyVTKObject_Type = 0;

//--------------------------------------------------------------------
PyVTKClass::PyVTKClass(
  PyTypeObject *typeobj, PyMethodDef *methods,
  const char *classname, const char *manglename,
  vtknewfunc constructor)
{
  this->py_type = typeobj;
  this->vtk_methods = methods;
  this->vtk_new = constructor;
  this->vtk_cppname = classname;
  this->vtk_mangle = (manglename ? manglename : classname);
}

//--------------------------------------------------------------------
// C API

//--------------------------------------------------------------------
// Add a class, add methods and members to its type object.  A return
// value of NULL signifies that the class was already added.
PyVTKClass *PyVTKClass_Add(
  PyTypeObject *pytype, PyMethodDef *methods,
  const char *classname, const char *manglename,
  const char *docstring[], vtknewfunc constructor)
{
  // Add this type to the vtk class map
  PyVTKClass *info =
    vtkPythonUtil::AddClassToMap(
      pytype, methods, classname, manglename, constructor);

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

  // Add special attribute __cppname__
  PyObject *s = PyString_FromString(classname);
  PyDict_SetItemString(pytype->tp_dict, "__cppname__", s);
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
  vtksys_ios::ostringstream vtkmsg_with_warning_C4701;
  ((PyVTKObject *)op)->vtk_ptr->Print(vtkmsg_with_warning_C4701);
  vtkmsg_with_warning_C4701.put('\0');
  PyObject *res = PyString_FromString(vtkmsg_with_warning_C4701.str().c_str());
  return res;
}

//--------------------------------------------------------------------
PyObject *PyVTKObject_Repr(PyObject *op)
{
  char buf[255];
  sprintf(buf, "(%.200s)%p", op->ob_type->tp_name, static_cast<void*>(op));

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
    // should use vtk_mangle instead of tp_name, to match __this__
    return vtkPythonUtil::GetObjectFromObject(o, tp->tp_name);
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

  PyObject_GC_Del(op);
}

//--------------------------------------------------------------------
// This defines any special attributes of wrapped VTK objects.

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
  // otherwise, use the mangled form of the class name
  if (*cp != '\0')
    {
    classname = self->vtk_class->vtk_mangle;
    }
  sprintf(buf, "p_%.500s", classname);
  return PyString_FromString(
    vtkPythonUtil::ManglePointer(self->vtk_ptr, buf));
}

PyGetSetDef PyVTKObject_GetSet[] = {
  { (char *)"__this__", PyVTKObject_GetThis, 0,
    (char *)"Pointer to the C++ object.", 0 },
  { 0, 0, 0, 0, 0 }
};

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
PyBufferProcs PyVTKObject_AsBuffer = {
  PyVTKObject_AsBuffer_GetReadBuf,       // bf_getreadbuffer
  PyVTKObject_AsBuffer_GetWriteBuf,      // bf_getwritebuffer
  PyVTKObject_AsBuffer_GetSegCount,      // bf_getsegcount
  0,                                     // bf_getcharbuffer
#if PY_VERSION_HEX >= 0x02060000
  0,                                     // bf_getbuffer
  0                                      // bf_releasebuffer
#endif
};

//--------------------------------------------------------------------
PyObject *PyVTKObject_FromPointer(
  PyTypeObject *pytype, PyObject *pydict, vtkObjectBase *ptr)
{
  // This will be set if we create a new C++ object
  bool created = false;
  const char *classname = pytype->tp_name;
  PyVTKClass *cls = 0;

  if (ptr)
    {
    // If constructing from an existing C++ object, use its actual class
    classname = ptr->GetClassName();
    cls = vtkPythonUtil::FindClass(classname);
    }

  if (cls == 0)
    {
    // Use the cppname of the supplied class type
    PyObject *s = PyObject_GetAttrString((PyObject *)pytype, "__cppname__");
    if (s)
      {
      classname = PyString_AsString(s);
      Py_DECREF(s);
      if (classname == 0)
        {
        return NULL;
        }
      }
    cls = vtkPythonUtil::FindClass(classname);
    if (cls == 0)
      {
      char errortext[280];
      sprintf(errortext, "internal error, unknown VTK class %.200s",
              classname);
      PyErr_SetString(PyExc_ValueError, errortext);
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
      if (strcmp(newclassname, classname) != 0)
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
