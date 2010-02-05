/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonUtil.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This include allows VTK to build on some platforms with broken Python
// header files.
#include "vtkPythonUtil.h"

#include "vtkSystemIncludes.h"

#include "vtkDataArray.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointerBase.h"
#include "vtkTimeStamp.h"
#include "vtkWindows.h"


#include <vtksys/ios/sstream>
#include <vtkstd/map>
#include <vtkstd/string>

// Silent warning like
// "dereferencing type-punned pointer will break strict-aliasing rules"
// it happens because this kind of expression: (long *)&ptr
// pragma GCC diagnostic is available since gcc>=4.2
#if defined(__GNUG__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=2)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#if defined ( _MSC_VER )
#  define vtkConvertPtrToLong(x) ((long)(PtrToUlong(x)))
#else
#  define vtkConvertPtrToLong(x) ((long)(x))
#endif

// The following macro is used to supress missing initializer
// warnings.  Python documentation says these should not be necessary.
// We define it as a macro in case the length needs to change across
// python versions.
#if   PY_VERSION_HEX >= 0x02060000 // for tp_version_tag
#define VTK_PYTHON_UTIL_SUPRESS_UNINITIALIZED \
  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0, 0,
#elif   PY_VERSION_HEX >= 0x02030000
#define VTK_PYTHON_UTIL_SUPRESS_UNINITIALIZED \
  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,
#elif PY_VERSION_HEX >= 0x02020000
#define VTK_PYTHON_UTIL_SUPRESS_UNINITIALIZED \
  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
#else
#define VTK_PYTHON_UTIL_SUPRESS_UNINITIALIZED
#endif

#if PY_VERSION_HEX < 0x02050000
  typedef int Py_ssize_t;
#endif

//#define VTKPYTHONDEBUG

//--------------------------------------------------------------------
// There are two hash tables associated with the Python wrappers

class vtkPythonUtil
{
public:
  vtkPythonUtil();
  ~vtkPythonUtil();

  vtkstd::map<vtkSmartPointerBase, PyObject*> *ObjectHash;
  vtkstd::map<vtkstd::string, PyObject*> *ClassHash;
};

//--------------------------------------------------------------------
vtkPythonUtil *vtkPythonHash = NULL;

//--------------------------------------------------------------------
vtkPythonUtil::vtkPythonUtil()
{
  this->ObjectHash = new vtkstd::map<vtkSmartPointerBase, PyObject*>;
  this->ClassHash = new vtkstd::map<vtkstd::string, PyObject*>;;
}

//--------------------------------------------------------------------
vtkPythonUtil::~vtkPythonUtil()
{
  delete this->ObjectHash;
  delete this->ClassHash;
}

//--------------------------------------------------------------------
extern "C" void vtkPythonHashDelete()
{
  delete vtkPythonHash;
  vtkPythonHash = 0;
}

//--------------------------------------------------------------------
static PyObject *PyVTKObject_PyString(PyVTKObject *self)
{
  PyObject *func = PyObject_GetAttrString((PyObject *)self, (char*)"__str__");

  if (func)
    {
    PyObject *res = PyEval_CallObject(func, (PyObject *)NULL);
    Py_DECREF(func);
    return res;
    }
  PyErr_Clear();

  vtksys_ios::ostringstream vtkmsg_with_warning_C4701;
  self->vtk_ptr->Print(vtkmsg_with_warning_C4701);
  vtkmsg_with_warning_C4701.put('\0');
  PyObject *res = PyString_FromString(vtkmsg_with_warning_C4701.str().c_str());
  return res;
}

//--------------------------------------------------------------------
static PyObject *PyVTKObject_PyRepr(PyVTKObject *self)
{
  PyObject *func = PyObject_GetAttrString((PyObject *)self, (char*)"__repr__");

  if (func)
    {
    PyObject *res = PyEval_CallObject(func, (PyObject *)NULL);
    Py_DECREF(func);
    return res;
    }
  PyErr_Clear();

  char buf[255];
  sprintf(buf,"<%s.%s %s at %p>",
          PyString_AsString(self->vtk_class->vtk_module),
          PyString_AsString(self->vtk_class->vtk_name),
          self->ob_type->tp_name,self);
  
  return PyString_FromString(buf);
}

//--------------------------------------------------------------------
int PyVTKObject_PySetAttr(PyVTKObject *self, PyObject *attr,
                          PyObject *value)
{
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
static PyObject *PyVTKObject_PyGetAttr(PyVTKObject *self, PyObject *attr)
{
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
      char buf[256];
      sprintf(buf,"p_%s", self->vtk_ptr->GetClassName());
      return PyString_FromString(vtkPythonManglePointer(self->vtk_ptr,buf));
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
      
    /* __methods__ is no longer applicable, methods are in __dict__
    if (strcmp(name,"__methods__") == 0)
      {
      PyMethodDef *meth = pyclass->vtk_methods;
      PyObject *lst;
      int i, n, m;

      n = 0;
      if ((lst = PyList_New(0)) == NULL)
        {
        return NULL;
        }
          
      bases = NULL;
      while (pyclass != NULL)
        {
        m = 0;
        for (meth = pyclass->vtk_methods; meth && meth->ml_name; meth++)
          {
          for (i = 0; i < n; i++)
            {
            if (strcmp(PyString_AsString(PyList_GetItem(lst,i)),
                       meth->ml_name) == 0)
              {
              break;
              }
            }
          if (i == n &&
              PyDict_GetItemString(self->vtk_dict, meth->ml_name) == 0)
            {
            if (PyList_Append(lst,PyString_FromString(meth->ml_name)) == -1)
              {
              Py_DECREF(lst);
              return NULL;
              }
            m++;
            }
          }
        n += m;
        bases = pyclass->vtk_bases;
        pyclass = NULL;
        if (PyTuple_Size(bases))
          {
          pyclass = (PyVTKClass *)PyTuple_GetItem(bases,0);
          }
        }
      PyList_Sort(lst);
      return lst;
      }

    if (strcmp(name,"__members__") == 0)
      {
      PyObject *lst;
      if ((lst = PyList_New(6)) != NULL)
        {
        PyList_SetItem(lst,0,PyString_FromString("__class__"));
        PyList_SetItem(lst,1,PyString_FromString("__dict__"));
        PyList_SetItem(lst,2,PyString_FromString("__doc__"));
        PyList_SetItem(lst,3,PyString_FromString("__members__"));
        PyList_SetItem(lst,4,PyString_FromString("__methods__"));
        PyList_SetItem(lst,5,PyString_FromString("__this__"));
        }
      return lst;
      }
    */
    }

  while (pyclass != NULL)
    {
    PyMethodDef *meth;

    if (pyclass->vtk_dict == NULL)
      {
      pyclass->vtk_dict = PyDict_New();

      for (meth = pyclass->vtk_methods; meth && meth->ml_name; meth++)
        {
        PyDict_SetItemString(pyclass->vtk_dict,meth->ml_name,
                             PyCFunction_New(meth, (PyObject *)pyclass));
        }
      }

    value = PyDict_GetItem(pyclass->vtk_dict, attr);
    
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

    bases = ((PyVTKClass *)pyclass)->vtk_bases;
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
static void PyVTKObject_PyDelete(PyVTKObject *self)
{
#if PY_VERSION_HEX >= 0x02010000
  if (self->vtk_weakreflist != NULL)
    {
    PyObject_ClearWeakRefs((PyObject *) self);
    }
#endif

  // A python object owning a VTK object reference is getting
  // destroyed.  Remove the python object's VTK object reference.
  vtkPythonDeleteObjectFromHash((PyObject *)self);

  Py_DECREF((PyObject *)self->vtk_class);
  Py_DECREF(self->vtk_dict);
#if (PY_MAJOR_VERSION >= 2)
  PyObject_Del(self);
#else
  PyMem_DEL(self);
#endif  
}

static Py_ssize_t
array_getsegcount(PyObject *pself, Py_ssize_t *lenp)
{
  PyVTKObject *self = (PyVTKObject*)pself;
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

static Py_ssize_t
array_getreadbuf(PyObject *pself, Py_ssize_t segment, void **ptrptr)
{
  if (segment != 0) 
    {
    PyErr_SetString(PyExc_ValueError,
                    "accessing non-existing array segment");
    return -1;
    }

  PyVTKObject *self = (PyVTKObject*)pself;
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


static Py_ssize_t
array_getwritebuf(PyObject *pself, Py_ssize_t segment, void **ptrptr)
{
  return array_getreadbuf(pself, segment, ptrptr);
}

static Py_ssize_t
array_getcharbuf(PyObject *, Py_ssize_t , const char **)
{
  return -1;
}

static PyBufferProcs array_as_buffer = {
#if PY_VERSION_HEX >= 0x02050000
  (readbufferproc)array_getreadbuf,    /*bf_getreadbuffer*/
  (writebufferproc)array_getwritebuf,  /*bf_getwritebuffer*/
  (segcountproc)array_getsegcount,            /*bf_getsegcount*/
  (charbufferproc)array_getcharbuf,    /*bf_getcharbuffer*/
  #if PY_VERSION_HEX >= 0x02060000
   (getbufferproc)0, /* bf_getbuffer */
   (releasebufferproc)0 /* bf_releasebuffer */
  #endif
#else
  (getreadbufferproc)array_getreadbuf,    /*bf_getreadbuffer*/
  (getwritebufferproc)array_getwritebuf,  /*bf_getwritebuffer*/
  (getsegcountproc)array_getsegcount,         /*bf_getsegcount*/
  (getcharbufferproc)array_getcharbuf,    /*bf_getcharbuffer*/
#endif
};

//--------------------------------------------------------------------
static PyTypeObject PyVTKObjectType = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  (char*)"vtkobject",                    // tp_name
  sizeof(PyVTKObject),                   // tp_basicsize
  0,                                     // tp_itemsize
  (destructor)PyVTKObject_PyDelete,      // tp_dealloc
  (printfunc)0,                          // tp_print
  (getattrfunc)0,                        // tp_getattr
  (setattrfunc)0,                        // tp_setattr
  (cmpfunc)0,                            // tp_compare
  (reprfunc)PyVTKObject_PyRepr,          // tp_repr
  0,                                     // tp_as_number 
  0,                                     // tp_as_sequence
  0,                                     // tp_as_mapping
  (hashfunc)0,                           // tp_hash
  (ternaryfunc)0,                        // tp_call
  (reprfunc)PyVTKObject_PyString,        // tp_string
  (getattrofunc)PyVTKObject_PyGetAttr,   // tp_getattro
  (setattrofunc)PyVTKObject_PySetAttr,   // tp_setattro
  &array_as_buffer,                      // tp_as_buffer
#if PY_VERSION_HEX >= 0x02010000
  Py_TPFLAGS_HAVE_WEAKREFS,              // tp_flags
#else
  0,                                     // tp_flags
#endif 
  (char*)"A VTK object.  Special attributes are:  __class__ (the class that this object belongs to), __dict__ (user-controlled attributes), __doc__ (the docstring for the class), __methods__ (a list of all methods for this object), and __this__ (a string that contains the hexidecimal address of the underlying VTK object)",  // tp_doc
  0,                                     // tp_traverse
  0,                                     // tp_clear
  0,                                     // tp_richcompare
#if PY_VERSION_HEX >= 0x02010000
  offsetof(PyVTKObject, vtk_weakreflist),// tp_weaklistoffset
#else
  0,                                     // tp_weaklistoffset
#endif
  VTK_PYTHON_UTIL_SUPRESS_UNINITIALIZED
};

int PyVTKObject_Check(PyObject *obj)
{
  return (obj->ob_type == &PyVTKObjectType);
}

PyObject *PyVTKObject_New(PyObject *pyvtkclass, vtkObjectBase *ptr)
{
  PyVTKClass *vtkclass = (PyVTKClass *)pyvtkclass;
  bool haveRef = false;
  if(!ptr)
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
#if (PY_MAJOR_VERSION >= 2)
  PyVTKObject *self = PyObject_New(PyVTKObject, &PyVTKObjectType);
#else
  PyVTKObject *self = PyObject_NEW(PyVTKObject, &PyVTKObjectType);
#endif  
  self->vtk_ptr = ptr;
  PyObject *cls = NULL;
  vtkstd::map<vtkstd::string, PyObject*>::iterator i =
    vtkPythonHash->ClassHash->find(ptr->GetClassName());
  if(i != vtkPythonHash->ClassHash->end())
    {
    cls = i->second;
    }
  self->vtk_class = (PyVTKClass *)cls;
  
  // If the class was not in the dictionary (i.e. if there is no 'python'
  // level class to support the VTK level class) we fall back to this.
  if (self->vtk_class == NULL || vtkclass->vtk_methods == NULL)
    {
    self->vtk_class = vtkclass;
    }
  
  Py_INCREF(self->vtk_class);
  
  self->vtk_dict = PyDict_New();

#if PY_VERSION_HEX >= 0x02010000
  self->vtk_weakreflist = NULL;
#endif

  // A python object owning a VTK object reference is getting
  // created.  Add the python object's VTK object reference.
  vtkPythonAddObjectToHash((PyObject *)self, ptr);

  // The hash now owns a reference so we can free ours.
  if(haveRef)
    {
    ptr->Delete();
    }

  return (PyObject *)self;
}

//--------------------------------------------------------------------
static PyObject *PyVTKClass_PyString(PyVTKClass *self)
{
  char buf[255];
  sprintf(buf,"%s.%s",
          PyString_AsString(self->vtk_module),
          PyString_AsString(self->vtk_name));

  return PyString_FromString(buf);
}

//--------------------------------------------------------------------
static PyObject *PyVTKClass_PyRepr(PyVTKClass *self)
{
  char buf[255];
  sprintf(buf,"<%s %s.%s at %p>",self->ob_type->tp_name,
          PyString_AsString(self->vtk_module),
          PyString_AsString(self->vtk_name),
          self);
  
  return PyString_FromString(buf);
}

//--------------------------------------------------------------------
static PyObject *PyVTKClass_PyCall(PyVTKClass *self, PyObject *arg,
                                   PyObject *kw)
{
  static PyObject *initstr = 0;

  if (((PyVTKClass *)self)->vtk_dict)
    {
    if (initstr == 0)
      {
      initstr = PyString_FromString("__init__");
      }

    PyObject *initfunc;
    initfunc = PyDict_GetItem(self->vtk_dict, initstr);

    if (initfunc)
      {      
      PyObject *obj = PyVTKObject_New((PyObject *)self,NULL);
      PyObject *cinitfunc = PyVTKObject_PyGetAttr((PyVTKObject *)obj, initstr);
      PyObject *res = PyEval_CallObjectWithKeywords(cinitfunc, arg, kw);
      if (res == NULL)
        {
        Py_DECREF(obj);
        obj = NULL;
        }
      else if (res != Py_None)
        {
        PyErr_SetString(PyExc_TypeError, "__init__() should return None");
        Py_DECREF(obj);
        obj = NULL;
        }
      Py_DECREF(cinitfunc);
      return obj;
      }
    }

  if (kw != NULL)
    {
    PyErr_SetString(PyExc_TypeError,
                    "this function takes no keyword arguments");
    return NULL;
    }
  if (PyArg_ParseTuple(arg,(char*)""))
    {
    return PyVTKObject_New((PyObject *)self, NULL);
    }
  PyErr_Clear();
  if (PyArg_ParseTuple(arg,(char*)"O", &arg))
    {
    return vtkPythonGetObjectFromObject(arg,
                                        PyString_AsString(self->vtk_name));
    }
  PyErr_Clear();
  PyErr_SetString(PyExc_TypeError,
                  "function requires 0 or 1 arguments");

  return NULL;
}

//--------------------------------------------------------------------
static PyObject *PyVTKClass_PyGetAttr(PyVTKClass *self, PyObject *attr)
{
  char *name = PyString_AsString(attr);
  PyVTKClass *pyclass = self;
  PyObject *bases;

  while (pyclass != NULL)
    {
    PyMethodDef *meth;
    PyObject *value;

    if (pyclass->vtk_dict == NULL)
      {
      pyclass->vtk_dict = PyDict_New();

      for (meth = pyclass->vtk_methods; meth && meth->ml_name; meth++)
        {
        PyDict_SetItemString(pyclass->vtk_dict,meth->ml_name,
                             PyCFunction_New(meth, (PyObject *)pyclass));
        }
      }

    value = PyDict_GetItem(pyclass->vtk_dict, attr);
    
    if (value)
      {
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

  if (name[0] == '_')
    {
    pyclass = (PyVTKClass *)self;

    if (strcmp(name,"__bases__") == 0)
      {
      Py_INCREF(pyclass->vtk_bases);
      return pyclass->vtk_bases;
      }

    if (strcmp(name,"__name__") == 0)
      {
      Py_INCREF(pyclass->vtk_name);
      return pyclass->vtk_name;
      }

    if (strcmp(name,"__module__") == 0)
      {
      Py_INCREF(pyclass->vtk_module);
      return pyclass->vtk_module;
      }

    if (strcmp(name,"__dict__") == 0 && pyclass->vtk_dict)
      {
      Py_INCREF(pyclass->vtk_dict);
      return pyclass->vtk_dict;
      }
      
    if (strcmp(name,"__doc__") == 0)
      {
      Py_INCREF(pyclass->vtk_doc);
      return pyclass->vtk_doc;
      }

    /* methods are stored in __dict__, so this is irrelevant
    if (strcmp(name,"__methods__") == 0)
      {
      PyMethodDef *meth = pyclass->vtk_methods;
      PyObject *lst;
      int i, n;

      for (n = 0; meth && meth[n].ml_name; n++);

      if ((lst = PyList_New(0)) != NULL)
        {
        meth = pyclass->vtk_methods;
        for (i = 0; i < n; i++)
          {
          if (pyclass->vtk_dict == NULL ||
              PyDict_GetItemString(pyclass->vtk_dict, meth[i].ml_name) == 0)
            {
            PyList_Append(lst, PyString_FromString(meth[i].ml_name));
            }
          }
        PyList_Sort(lst);
        }
      return lst;
      }
      
    if (strcmp(name,"__members__") == 0)
      {
      int n = 6;
      int i = 0;
      PyObject *lst;

      if (pyclass->vtk_dict)
        {
        n++;
        }

      if ((lst = PyList_New(n)) != NULL)
        {
        PyList_SetItem(lst,i++,PyString_FromString("__bases__"));
        if (pyclass->vtk_dict)
          {
          PyList_SetItem(lst,i++,PyString_FromString("__dict__"));
          }
        PyList_SetItem(lst,i++,PyString_FromString("__doc__"));
        PyList_SetItem(lst,i++,PyString_FromString("__members__"));
        PyList_SetItem(lst,i++,PyString_FromString("__methods__"));
        PyList_SetItem(lst,i++,PyString_FromString("__module__"));
        PyList_SetItem(lst,i++,PyString_FromString("__name__"));
        }
      return lst;
      }
    */
    }

  PyErr_SetString(PyExc_AttributeError, name);
  return NULL;
}

//--------------------------------------------------------------------
static void PyVTKClass_PyDelete(PyVTKClass *self)
{
  Py_XDECREF(self->vtk_bases);
  Py_XDECREF(self->vtk_dict);
  Py_XDECREF(self->vtk_name);

  Py_XDECREF(self->vtk_getattr);
  Py_XDECREF(self->vtk_setattr);
  Py_XDECREF(self->vtk_delattr);

  Py_XDECREF(self->vtk_module);
  Py_XDECREF(self->vtk_doc);
    
#if (PY_MAJOR_VERSION >= 2)
  PyObject_Del(self);
#else
  PyMem_DEL(self);
#endif  
}

//--------------------------------------------------------------------
static PyObject *PyVTKClassMetaType_GetAttr(PyTypeObject *t, char *name)
{
  if (strcmp(name, "__name__") == 0)
    {
    return PyString_FromString(t->tp_name);
    }
  if (strcmp(name, "__doc__") == 0)
    {
    const char *doc = t->tp_doc;
    if (doc != NULL)
      {
      return PyString_FromString(doc);
      }
    Py_INCREF(Py_None);
    return Py_None;
    }
  if (strcmp(name, "__members__") == 0)
    {
    return Py_BuildValue((char*)"[ss]", "__doc__", "__name__");
    }
  PyErr_SetString(PyExc_AttributeError, name);
  return NULL;
}

//--------------------------------------------------------------------
static PyObject *PyVTKClassMetaType_Repr(PyTypeObject *v)
{
  char buf[100];
  sprintf(buf, "<type '%.80s'>", v->tp_name);
  return PyString_FromString(buf);
}

//--------------------------------------------------------------------
static PyObject *PyVTKClass_NewSubclass(PyObject *self, PyObject *args,
                                        PyObject *kw);

//--------------------------------------------------------------------
PyTypeObject PyVTKClassMetaType = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                        /* Number of items for varobject */
  (char*)"vtkclass type",   /* Name of this type */
  sizeof(PyTypeObject),     /* Basic object size */
  0,                        /* Item size for varobject */
  0,                        /*tp_dealloc*/
  0,                        /*tp_print*/
  (getattrfunc)PyVTKClassMetaType_GetAttr, /*tp_getattr*/
  0,                        /*tp_setattr*/
  0,                        /*tp_compare*/
  (reprfunc)PyVTKClassMetaType_Repr,        /*tp_repr*/
  0,                        /*tp_as_number*/
  0,                        /*tp_as_sequence*/
  0,                        /*tp_as_mapping*/
  0,                        /*tp_hash*/
  (ternaryfunc)PyVTKClass_NewSubclass, /*tp_call*/
  0,                        /*tp_str*/
  0,                        /*tp_xxx1*/
  0,                        /*tp_xxx2*/
  0,                        /*tp_xxx3*/
  0,                        /*tp_xxx4*/
  (char*)"Define the behavior of a particular type of object.",
  0,                        // tp_traverse
  0,                        // tp_clear
  0,                        // tp_richcompare
  0,                        // tp_weaklistoffset
  VTK_PYTHON_UTIL_SUPRESS_UNINITIALIZED
};

//--------------------------------------------------------------------
static PyTypeObject PyVTKClassType = {
  PyObject_HEAD_INIT(&PyVTKClassMetaType)
  0,
  (char*)"vtkclass",                     // tp_name
  sizeof(PyVTKClass),                    // tp_basicsize
  0,                                     // tp_itemsize
  (destructor)PyVTKClass_PyDelete,       // tp_dealloc
  (printfunc)0,                          // tp_print
  (getattrfunc)0,                        // tp_getattr
  (setattrfunc)0,                        // tp_setattr
  (cmpfunc)0,                            // tp_compare
  (reprfunc)PyVTKClass_PyRepr,           // tp_repr
  0,                                     // tp_as_number 
  0,                                     // tp_as_sequence
  0,                                     // tp_as_mapping
  (hashfunc)0,                           // tp_hash
  (ternaryfunc)PyVTKClass_PyCall,        // tp_call
  (reprfunc)PyVTKClass_PyString,         // tp_string
  (getattrofunc)PyVTKClass_PyGetAttr,    // tp_getattro
  (setattrofunc)0,                       // tp_setattro
  0,                                     // tp_as_buffer
  0,                                     // tp_flags
  (char*)"A generator for VTK objects.  Special attributes are: __bases__ (a tuple of base classes), __dict__ (user-defined methods and attributes), __doc__ (the docstring for the class), __name__ (the name of class), __methods__ (methods for this class, not including inherited methods or user-defined methods), and __module__ (module that the class is defined in).", // tp_doc
  0,                                     // tp_traverse
  0,                                     // tp_clear
  0,                                     // tp_richcompare
  0,                                     // tp_weaklistoffset
  VTK_PYTHON_UTIL_SUPRESS_UNINITIALIZED
};

int PyVTKClass_Check(PyObject *obj)
{
  return (obj->ob_type == &PyVTKClassType);
}

//--------------------------------------------------------------------
// Concatenate an array of strings into a single string.  The resulting
// string is allocated via new.  The array of strings must be null-terminated,
// e.g. static char *strings[] = {"string1", "string2", NULL};
static PyObject *vtkBuildDocString(char *docstring[])
{
  PyObject *result;
  char *data;
  int i, j, n;
  int *m;
  int total = 0;

  for (n = 0; docstring[n] != NULL; n++)
    {
    ;
    }

  m = new int[n];

  for (i = 0; i < n; i++)
    {
    m[i] = static_cast<int>(strlen(docstring[i]));
    total += m[i];
    }

  result = PyString_FromStringAndSize(docstring[0], m[0]);
  
  if (n > 1)
    {
    _PyString_Resize(&result, total);
    }

  data = PyString_AsString(result);
  
  j = m[0];
  for (i = 1; i < n; i++)
    {
    strcpy(&data[j], docstring[i]);
    j += m[i];
    }
  
  delete [] m;

  return result;
}

PyObject *PyVTKClass_New(vtknewfunc constructor,
                         PyMethodDef *methods,
                         char *classname, char *modulename, char *docstring[],
                         PyObject *base)
{
  static PyObject *modulestr[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; 
  static int nmodulestr = 10;
  PyObject *moduleobj = 0;
  PyObject *self = NULL;
  int i;

  if (vtkPythonHash)
    {
    vtkstd::map<vtkstd::string, PyObject*>::iterator it =
      vtkPythonHash->ClassHash->find(classname);
    if(it != vtkPythonHash->ClassHash->end())
      {
      self = it->second;
      }
    }
  if (self)
    {
    Py_INCREF(self);
    }
  else
    {
#if (PY_MAJOR_VERSION >= 2)
    PyVTKClass *class_self = PyObject_New(PyVTKClass, &PyVTKClassType);
#else
    PyVTKClass *class_self = PyObject_NEW(PyVTKClass, &PyVTKClassType);
#endif  
    self = (PyObject *)class_self;
    
    if (base)
      {
      class_self->vtk_bases = PyTuple_New(1);
      PyTuple_SET_ITEM(class_self->vtk_bases, 0, base);
      }
    else
      {
      class_self->vtk_bases = PyTuple_New(0);
      }
    class_self->vtk_dict = NULL;
    class_self->vtk_name = PyString_FromString(classname);

    class_self->vtk_getattr = NULL;
    class_self->vtk_setattr = NULL;
    class_self->vtk_delattr = NULL;
    
    class_self->vtk_methods = methods;
    class_self->vtk_new = constructor;
    class_self->vtk_doc = vtkBuildDocString(docstring);

    // intern the module string
    for (i = 0; i < nmodulestr; i++)
      {
      if (modulestr[i] == 0)
        {
        modulestr[i] = PyString_InternFromString(modulename);
        moduleobj = modulestr[i];
        Py_INCREF(moduleobj);
        break;
        }
      else if (strcmp(modulename,PyString_AsString(modulestr[i])) == 0)
        {
        moduleobj = modulestr[i];
        Py_INCREF(moduleobj);
        break;
        }
      }
    if (i == nmodulestr)
      {
      moduleobj = PyString_FromString(modulename);
      }

    class_self->vtk_module = moduleobj;

    vtkPythonAddClassToHash(self,classname);
    }

  return (PyObject *)self;
}

//--------------------------------------------------------------------
static PyObject *PyVTKClass_NewSubclass(PyObject *, PyObject *args,
                                        PyObject *kw)
{
  static const char *kwlist[] = {"name", "bases", "dict", NULL};

  PyVTKClass *newclass;
  char *classname;
  PyObject *globals;
  PyObject *bases;
  PyVTKClass *base;
  PyObject *attributes;

  if ((PyArg_ParseTupleAndKeywords(args, kw, (char*)"sOO", (char**)kwlist, 
                                   &classname, &bases, &attributes)))
    {
    if (!PyTuple_Check(bases) || PyTuple_Size(bases) != 1)
      {
      PyErr_SetString(PyExc_ValueError,
                      "multiple inheritence is not allowed with VTK classes");
      return NULL;
      }

    base = (PyVTKClass *)PyTuple_GetItem(bases,0);
    if (base == 0)
      {
      PyErr_SetString(PyExc_ValueError,"bases must be a tuple");
      return NULL;
      }

    if (!PyVTKClass_Check((PyObject *)base))
      {
      PyErr_SetString(PyExc_ValueError,"base class is not a VTK class");
      return NULL;
      }

    if (!PyDict_Check(attributes))
      {
      PyErr_SetString(PyExc_ValueError,"namespace not provided");
      return NULL;
      }

    if (PyDict_GetItemString(attributes, (char*)"__del__"))
      {
      PyErr_SetString(PyExc_ValueError, "__del__ attribute is not supported");
      return NULL;
      }

#if (PY_MAJOR_VERSION >= 2)
    newclass = PyObject_New(PyVTKClass, &PyVTKClassType);
#else
    newclass = PyObject_NEW(PyVTKClass, &PyVTKClassType);
#endif  

    Py_INCREF(bases);
    Py_INCREF(attributes);

    newclass->vtk_bases = bases;
    newclass->vtk_dict = attributes;
    newclass->vtk_name = PyString_FromString(classname);

    newclass->vtk_getattr = PyDict_GetItemString(attributes, (char*)"__getattr__");
    if (newclass->vtk_getattr == 0)
      {
      newclass->vtk_getattr = base->vtk_getattr;
      }
    Py_XINCREF(newclass->vtk_getattr);
    newclass->vtk_setattr = PyDict_GetItemString(attributes, (char*)"__setattr__");
    if (newclass->vtk_setattr == 0)
      {
      newclass->vtk_setattr = base->vtk_setattr;
      }
    Py_XINCREF(newclass->vtk_setattr);
    newclass->vtk_delattr = PyDict_GetItemString(attributes, (char*)"__delattr__");
    if (newclass->vtk_delattr == 0)
      {
      newclass->vtk_delattr = base->vtk_delattr;
      }
    Py_XINCREF(newclass->vtk_delattr);

    newclass->vtk_methods = NULL;
    newclass->vtk_new = base->vtk_new;
    newclass->vtk_module = NULL;
    newclass->vtk_doc = NULL;

    globals = PyEval_GetGlobals();
    if (globals != NULL)
      {
      PyObject *modname = PyDict_GetItemString(globals, (char*)"__name__");
      if (modname != NULL)
        {
        Py_INCREF(modname);
        newclass->vtk_module = modname;
        }
      }
    if (newclass->vtk_module == NULL)
      {
      newclass->vtk_module = PyString_FromString("__main__");
      }

    newclass->vtk_doc = PyDict_GetItemString(attributes, (char*)"__doc__");
    if (newclass->vtk_doc)
      {
      Py_INCREF(newclass->vtk_doc);
      PyDict_DelItemString(attributes, (char*)"__doc__");
      }
    else
      {
      newclass->vtk_doc = PyString_FromString("");
      }

    return (PyObject *)newclass;    
    }
  return NULL;
}

//--------------------------------------------------------------------
static PyObject *PyVTKSpecialObject_PyString(PyVTKSpecialObject *self)
{
  Py_INCREF(self->vtk_name);
  return self->vtk_name;
}

//--------------------------------------------------------------------
static PyObject *PyVTKSpecialObject_PyRepr(PyVTKSpecialObject *self)
{
  char buf[255];
  sprintf(buf,"<%s %s at %p>", self->ob_type->tp_name, 
          PyString_AsString(self->vtk_name), self);
  return PyString_FromString(buf);
}

//--------------------------------------------------------------------
static PyObject *PyVTKSpecialObject_PyGetAttr(PyVTKSpecialObject *self,
                                              PyObject *attr)
{
  char *name = PyString_AsString(attr);
  PyMethodDef *meth;

  if (name[0] == '_')
    {
    if (strcmp(name,"__name__") == 0)
      {
      Py_INCREF(self->vtk_name);
      return self->vtk_name;
      }
    if (strcmp(name,"__doc__") == 0)
      {
      Py_INCREF(self->vtk_doc);
      return self->vtk_doc;
      }
    if (strcmp(name,"__methods__") == 0)
      {
      meth = self->vtk_methods;
      PyObject *lst;
      int i, n;

      for (n = 0; meth && meth[n].ml_name; n++)
        {
        ;
        }

      if ((lst = PyList_New(n)) != NULL)
        {
        meth = self->vtk_methods;
        for (i = 0; i < n; i++)
          {
          PyList_SetItem(lst, i, PyString_FromString(meth[i].ml_name));
          }
        PyList_Sort(lst);
        }
      return lst;
      }

    if (strcmp(name,"__members__") == 0)
      {
      PyObject *lst;
      if ((lst = PyList_New(4)) != NULL)
        {
        PyList_SetItem(lst,0,PyString_FromString("__doc__"));
        PyList_SetItem(lst,1,PyString_FromString("__members__"));
        PyList_SetItem(lst,2,PyString_FromString("__methods__"));
        PyList_SetItem(lst,3,PyString_FromString("__name__"));
        }
      return lst;
      }
    } 

  for (meth = self->vtk_methods; meth && meth->ml_name; meth++)
    {
    if (name[0] == meth->ml_name[0] && strcmp(name+1, meth->ml_name+1) == 0)
      {
      return PyCFunction_New(meth, (PyObject *)self);
      }
    }
  
  PyErr_SetString(PyExc_AttributeError, name);
  return NULL;
}

//--------------------------------------------------------------------
static void PyVTKSpecialObject_PyDelete(PyVTKSpecialObject *self)
{
  self->vtk_ptr = NULL;
  Py_XDECREF(self->vtk_name);
  Py_XDECREF(self->vtk_doc);
#if (PY_MAJOR_VERSION >= 2)
  PyObject_Del(self);
#else
  PyMem_DEL(self);
#endif  
}

//--------------------------------------------------------------------
static PyTypeObject PyVTKSpecialObjectType = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  (char*)"vtkspecialobject",             // tp_name
  sizeof(PyVTKSpecialObject),            // tp_basicsize
  0,                                     // tp_itemsize
  (destructor)PyVTKSpecialObject_PyDelete, // tp_dealloc
  (printfunc)0,                          // tp_print
  (getattrfunc)0,                        // tp_getattr
  (setattrfunc)0,                        // tp_setattr
  (cmpfunc)0,                            // tp_compare
  (reprfunc)PyVTKSpecialObject_PyRepr,   // tp_repr
  0,                                     // tp_as_number 
  0,                                     // tp_as_sequence
  0,                                     // tp_as_mapping
  (hashfunc)0,                           // tp_hash
  (ternaryfunc)0,                        // tp_call
  (reprfunc)PyVTKSpecialObject_PyString, // tp_string
  (getattrofunc)PyVTKSpecialObject_PyGetAttr, // tp_getattro
  (setattrofunc)0,                       // tp_setattro
  0,                                     // tp_as_buffer
  0,                                     // tp_flags
  (char*)"vtkspecialobject - a vtk object not derived from vtkObjectBase.", // tp_doc
  0,                                     // tp_traverse
  0,                                     // tp_clear
  0,                                     // tp_richcompare
  0,                                     // tp_weaklistoffset
  VTK_PYTHON_UTIL_SUPRESS_UNINITIALIZED
};

int PyVTKSpecialObject_Check(PyObject *obj)
{
  return (obj->ob_type == &PyVTKSpecialObjectType);
}

PyObject *PyVTKSpecialObject_New(void *ptr, PyMethodDef *methods,
                                 char *classname, char *docstring[])
{
#if (PY_MAJOR_VERSION >= 2)
  PyVTKSpecialObject *self = PyObject_New(PyVTKSpecialObject, 
                                          &PyVTKSpecialObjectType);
#else
  PyVTKSpecialObject *self = PyObject_NEW(PyVTKSpecialObject, 
                                          &PyVTKSpecialObjectType);
#endif  
  self->vtk_ptr = ptr;
  self->vtk_methods = methods;
  self->vtk_name = PyString_FromString(classname);
  self->vtk_doc = vtkBuildDocString(docstring);
  
  return (PyObject *)self;
}

//--------------------------------------------------------------------
vtkObjectBase *PyArg_VTKParseTuple(PyObject *pself, PyObject *args, 
                                   char *format, ...)
{
  PyVTKObject *self = (PyVTKObject *)pself;
  vtkObjectBase *obj = NULL;
  va_list va;
  va_start(va, format);

  /* check if this was called as an unbound method */
  if (self->ob_type == &PyVTKClassType)
    {
    int n = PyTuple_Size(args);
    PyVTKClass *vtkclass = (PyVTKClass *)self;

    if (n == 0 || (self = (PyVTKObject *)PyTuple_GetItem(args, 0)) == NULL ||
        self->ob_type != &PyVTKObjectType ||
        !self->vtk_ptr->IsA(PyString_AsString(vtkclass->vtk_name)))
      {
      char buf[256];
      sprintf(buf,"unbound method requires a %s as the first argument",
              PyString_AsString(vtkclass->vtk_name));
      PyErr_SetString(PyExc_ValueError,buf);
      return NULL;
      }
    // re-slice the args to remove 'self'
    args = PyTuple_GetSlice(args,1,n);
    if (PyArg_VaParse(args,format,va))
      {
      obj = self->vtk_ptr;
      }
    Py_DECREF(args);
    }
  /* it was called as a bound method */
  else
    {
    if (PyArg_VaParse(args,format,va))
      {
      obj = self->vtk_ptr;
      }
    }   
  return obj;
}

//--------------------------------------------------------------------
void vtkPythonAddClassToHash(PyObject *vtkclass, const char *classname)
{
  if (vtkPythonHash == NULL)
    {
    vtkPythonHash = new vtkPythonUtil();
    Py_AtExit(vtkPythonHashDelete);
    }

#ifdef VTKPYTHONDEBUG
  //  vtkGenericWarningMacro("Adding an type " << type << " to hash ptr");
#endif  

  // lets make sure it isn't already there
  vtkstd::map<vtkstd::string, PyObject*>::iterator i =
    vtkPythonHash->ClassHash->find(classname);
  if(i != vtkPythonHash->ClassHash->end())
    {
#ifdef VTKPYTHONDEBUG
    vtkGenericWarningMacro("Attempt to add type to the hash when already there!!!");
#endif
    return;
    }

  (*vtkPythonHash->ClassHash)[classname] = vtkclass;

#ifdef VTKPYTHONDEBUG
  //  vtkGenericWarningMacro("Added type to hash type = " << typeObject);
#endif  
}  

//--------------------------------------------------------------------
void vtkPythonAddObjectToHash(PyObject *obj, vtkObjectBase *ptr)
{
  if (vtkPythonHash == NULL)
    {
    vtkPythonHash = new vtkPythonUtil();
    Py_AtExit(vtkPythonHashDelete);
    }

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Adding an object to hash ptr = " << ptr);
#endif  

  ((PyVTKObject *)obj)->vtk_ptr = ptr;
  (*vtkPythonHash->ObjectHash)[ptr] = obj;

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Added object to hash obj= " << obj << " " 
                         << ptr);
#endif  
}  

//--------------------------------------------------------------------
void vtkPythonDeleteObjectFromHash(PyObject *obj)
{
  vtkObjectBase *ptr = ((PyVTKObject *)obj)->vtk_ptr;

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Deleting an object from hash obj = " << obj << " "
                         << obj->vtk_ptr);
#endif  

  if (vtkPythonHash)
    {
    vtkPythonHash->ObjectHash->erase(ptr);
    }
}

//--------------------------------------------------------------------
static PyObject *vtkFindNearestBase(vtkObjectBase *ptr);

PyObject *vtkPythonGetObjectFromPointer(vtkObjectBase *ptr)
{
  PyObject *obj = NULL;

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Checking into pointer " << ptr);
#endif
  
  if (ptr)
    {
    vtkstd::map<vtkSmartPointerBase, PyObject*>::iterator i =
      vtkPythonHash->ObjectHash->find(ptr);
    if(i != vtkPythonHash->ObjectHash->end())
      {
      obj = i->second;
      }
    if (obj)
      {
      Py_INCREF(obj);
      }
    }
  else
    {
    Py_INCREF(Py_None);
    obj = Py_None;
    }
#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Checking into pointer " << ptr << " obj = " << obj);
#endif
  
  if (obj == NULL)
    {
    PyObject *vtkclass = NULL;
    vtkstd::map<vtkstd::string, PyObject*>::iterator i =
      vtkPythonHash->ClassHash->find(ptr->GetClassName());
    if(i != vtkPythonHash->ClassHash->end())
      {
      vtkclass = i->second;
      }

      // if the class was not in the hash, then find the nearest base class
      // that is and associate ptr->GetClassName() with that base class
    if (vtkclass == NULL)
      {
      vtkclass = vtkFindNearestBase(ptr);
      vtkPythonAddClassToHash(vtkclass, ptr->GetClassName());
      }

    obj = PyVTKObject_New(vtkclass, ptr);
    }

  return obj;
}

// this is a helper function to find the nearest base class for an
// object whose class is not in the ClassDict
static PyObject *vtkFindNearestBase(vtkObjectBase *ptr)
{
  PyObject *nearestbase = NULL;
  int maxdepth = 0;
  int depth;

  for(vtkstd::map<vtkstd::string, PyObject*>::iterator classes =
        vtkPythonHash->ClassHash->begin();
      classes != vtkPythonHash->ClassHash->end(); ++classes)
    {
    PyObject *pyclass = classes->second;

    if (ptr->IsA(PyString_AsString(((PyVTKClass *)pyclass)->vtk_name)))
      { 
      PyObject *cls = pyclass;
      PyObject *bases = ((PyVTKClass *)pyclass)->vtk_bases;
      // count the heirarchy depth for this class
      for (depth = 0; PyTuple_Size(bases) != 0; depth++)
        {
        cls = PyTuple_GetItem(bases,0);
        bases = ((PyVTKClass *)cls)->vtk_bases;
        }
      // we want the class that is furthest from vtkObjectBase
      if (depth > maxdepth)
        {
        maxdepth = depth;
        nearestbase = pyclass;
        }
      }
    }
  
  return nearestbase;
}

//--------------------------------------------------------------------
vtkObjectBase *vtkPythonGetPointerFromObject(PyObject *obj,
                                             const char *result_type)
{ 
  vtkObjectBase *ptr;

  // convert Py_None to NULL every time
  if (obj == Py_None)
    {
      return NULL;
    }

  // check to ensure it is a vtk object
  if (obj->ob_type != &PyVTKObjectType)
    {
    obj = PyObject_GetAttrString(obj,(char*)"__vtk__");
    if (obj)
      {
      PyObject *arglist = Py_BuildValue((char*)"()");
      PyObject *result = PyEval_CallObject(obj, arglist);
      Py_DECREF(arglist);
      Py_DECREF(obj);
      if (result == NULL)
        {
        return NULL;
        }
      if (result->ob_type != &PyVTKObjectType)
        {
        PyErr_SetString(PyExc_ValueError,"__vtk__() doesn't return a VTK object");
        Py_DECREF(result);
        return NULL;
        }
      else
        {
        ptr = ((PyVTKObject *)result)->vtk_ptr;
        Py_DECREF(result);
        }
      }
    else
      {
#ifdef VTKPYTHONDEBUG
      vtkGenericWarningMacro("Object " << obj << " is not a VTK object!!");
#endif  
      PyErr_SetString(PyExc_ValueError,"method requires a VTK object");
      return NULL;
      }
    }
  else
    {
    ptr = ((PyVTKObject *)obj)->vtk_ptr;
    }
  
#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Checking into obj " << obj << " ptr = " << ptr);
#endif  

  if (ptr->IsA(result_type))
    {
#ifdef VTKPYTHONDEBUG
      vtkGenericWarningMacro("Got obj= " << obj << " ptr= " << ptr << " " << result_type);
#endif  
      return ptr;
    }
  else
    {
    char error_string[256];
#ifdef VTKPYTHONDEBUG
    vtkGenericWarningMacro("vtk bad argument, type conversion failed.");
#endif
    sprintf(error_string,"method requires a %s, a %s was provided.",
            result_type,((vtkObjectBase *)ptr)->GetClassName());
    PyErr_SetString(PyExc_ValueError,error_string);
    return NULL;
    }
}

PyObject *vtkPythonGetObjectFromObject(PyObject *arg, const char *type)
{
  if (PyString_Check(arg))
    {
    char *ptrText = PyString_AsString(arg);

    vtkObjectBase *ptr;
    char typeCheck[256];  // typeCheck is currently not used
    int i = sscanf(ptrText,"_%lx_%s",(long *)&ptr,typeCheck);
      
    if (i <= 0)
      {
      i = sscanf(ptrText,"Addr=0x%lx",(long *)&ptr);
      }      
    if (i <= 0)
      {
      i = sscanf(ptrText,"%p",&ptr);
      }
    if (i <= 0)
      {
      PyErr_SetString(PyExc_ValueError,"could not extract hexidecimal address from argument string");
      return NULL;
      }

    if (!ptr->IsA(type))
      {
      char error_string[256];
      sprintf(error_string,"method requires a %s address, a %s address was provided.",
              type,((vtkObjectBase *)ptr)->GetClassName());
      PyErr_SetString(PyExc_TypeError,error_string);
      return NULL;
      }
      
    return vtkPythonGetObjectFromPointer(ptr);
    }
  
  PyErr_SetString(PyExc_TypeError,"method requires a string argument");
  return NULL;
}

//--------------------------------------------------------------------
// mangle a void pointer into a SWIG-style string
char *vtkPythonManglePointer(void *ptr, const char *type)
{
  static char ptrText[128];
  sprintf(ptrText,"_%*.*lx_%s",2*(int)sizeof(void *),2*(int)sizeof(void *),
          vtkConvertPtrToLong(ptr),type);
  return ptrText;
}

//--------------------------------------------------------------------
// unmangle a void pointer from a SWIG-style string
void *vtkPythonUnmanglePointer(char *ptrText, int *len, const char *type)
{
  int i; 
  void *ptr;
  char typeCheck[256];
  typeCheck[0] = '\0';

  // Do some minimal checks that it might be a swig pointer.
  if (*len < 256 && *len > 4 && ptrText[0] == '_')
    {
    // Verify that this is a terminated string
    for (i = *len-1; i >= 0; --i)
      {
      if (ptrText[i] == '\0')
        {
        break;
        }
      }

    // If the string is terminated, do a full check for a swig pointer
    if (i >= 0)
      {
      i = sscanf(ptrText,"_%lx_%s",(long *)&ptr,typeCheck);
      if (strcmp(type,typeCheck) == 0)
        { // sucessfully unmangle
        *len = 0;
        return ptr;
        }
      else if (i == 2)
        { // mangled pointer of wrong type
        *len = -1;
        return NULL;
        }
      }
    }

  // couldn't unmangle: return string as void pointer if it didn't look
  // like a SWIG mangled pointer
  return (void *)ptrText;
}

//--------------------------------------------------------------------
// These functions check an array that was sent to a method to see if
// any of the values were changed by the method.
// If a value was changed, then the corresponding value in the python
// list is modified.

template<class T>
inline
int vtkPythonCheckFloatArray(PyObject *args, int i, T *a, int n)
{
  int changed = 0;

  PyObject *seq = PyTuple_GET_ITEM(args, i);
  for (i = 0; i < n; i++)
    {
    PyObject *oldobj = PySequence_GetItem(seq, i);
    T oldval = (T)PyFloat_AsDouble(oldobj);
    Py_DECREF(oldobj);
    changed |= (a[i] != oldval);
    }

  if (changed)
    {
    for (i = 0; i < n; i++)
      {
      PyObject *newobj = PyFloat_FromDouble(a[i]);
      int rval = PySequence_SetItem(seq, i, newobj);
      Py_DECREF(newobj);
      if (rval == -1)
        {
        return -1;
        }
      }
    }

  return 0;
}

template<class T>
inline
int vtkPythonCheckIntArray(PyObject *args, int i, T *a, int n)
{
  int changed = 0;

  PyObject *seq = PyTuple_GET_ITEM(args, i);
  for (i = 0; i < n; i++)
    {
    PyObject *oldobj = PySequence_GetItem(seq, i);
    T oldval = (T)PyInt_AsLong(oldobj);
    Py_DECREF(oldobj);
    changed |= (a[i] != oldval);
    }

  if (changed)
    {
    for (i = 0; i < n; i++)
      {
      PyObject *newobj = PyInt_FromLong(a[i]);
      int rval = PySequence_SetItem(seq, i, newobj);
      Py_DECREF(newobj);
      if (rval == -1)
        {
        return -1;
        }
      }
    }

  return 0;
}

#if defined(VTK_TYPE_USE_LONG_LONG) || defined(VTK_TYPE_USE___INT64)
template<class T>
inline
int vtkPythonCheckLongArray(PyObject *args, int i, T *a, int n)
{
  int changed = 0;

  PyObject *seq = PyTuple_GET_ITEM(args, i);
  for (i = 0; i < n; i++)
    {
    PyObject *oldobj = PySequence_GetItem(seq, i);
    T oldval;
    if (PyLong_Check(oldobj))
      {
#ifdef PY_LONG_LONG
      oldval = PyLong_AsLongLong(oldobj);
#else
      oldval = PyLong_AsLong(oldobj);
#endif
      }
    else
      {
      oldval = PyInt_AsLong(oldobj);
      }
    Py_DECREF(oldobj);
    changed |= (a[i] != oldval);
    }

  if (changed)
    {
    for (i = 0; i < n; i++)
      {
#if defined(VTK_TYPE_USE_LONG_LONG)
# if defined(PY_LONG_LONG) && (VTK_SIZEOF_LONG != VTK_SIZEOF_LONG_LONG)
      PyObject *newobj = PyLong_FromLongLong(a[i]);
# else
      PyObject *newobj = PyInt_FromLong((long)a[i]);
# endif
#else
# if defined(PY_LONG_LONG) && (VTK_SIZEOF_LONG != VTK_SIZEOF___INT64)
      PyObject *newobj = PyLong_FromLongLong(a[i]);
# else
      PyObject *newobj = PyInt_FromLong((long)a[i]);
# endif
#endif
      int rval = PySequence_SetItem(seq, i, newobj);
      Py_DECREF(newobj);
      if (rval == -1)
        {
        return -1;
        }
      }
    }

  return 0;
}
#endif

int vtkPythonCheckArray(PyObject *args, int i, char *a, int n)
{
  return vtkPythonCheckIntArray(args, i, a, n);
}

int vtkPythonCheckArray(PyObject *args, int i, signed char *a, int n)
{
  return vtkPythonCheckIntArray(args, i, a, n);
}

int vtkPythonCheckArray(PyObject *args, int i, unsigned char *a, int n)
{
  return vtkPythonCheckIntArray(args, i, a, n);
}

int vtkPythonCheckArray(PyObject *args, int i, short *a, int n)
{
  return vtkPythonCheckIntArray(args, i, a, n);
}

int vtkPythonCheckArray(PyObject *args, int i, unsigned short *a, int n)
{
  return vtkPythonCheckIntArray(args, i, a, n);
}

int vtkPythonCheckArray(PyObject *args, int i, int *a, int n)
{
  return vtkPythonCheckIntArray(args, i, a, n);
}

int vtkPythonCheckArray(PyObject *args, int i, unsigned int *a, int n)
{
  return vtkPythonCheckIntArray(args, i, a, n);
}

int vtkPythonCheckArray(PyObject *args, int i, long *a, int n)
{
  return vtkPythonCheckIntArray(args, i, a, n);
}

int vtkPythonCheckArray(PyObject *args, int i, unsigned long *a, int n)
{
  return vtkPythonCheckIntArray(args, i, a, n);
}

int vtkPythonCheckArray(PyObject *args, int i, float *a, int n)
{
  return vtkPythonCheckFloatArray(args, i, a, n);
}

int vtkPythonCheckArray(PyObject *args, int i, double *a, int n)
{
  return vtkPythonCheckFloatArray(args, i, a, n);
}

#if defined(VTK_TYPE_USE_LONG_LONG)
int vtkPythonCheckArray(PyObject *args, int i, long long *a, int n)
{
  return vtkPythonCheckLongArray(args, i, a, n);
}
int vtkPythonCheckArray(PyObject *args, int i, unsigned long long *a, int n)
{
  return vtkPythonCheckLongArray(args, i, a, n);
}
#endif

#if defined(VTK_TYPE_USE___INT64)
int vtkPythonCheckArray(PyObject *args, int i, __int64 *a, int n)
{
  return vtkPythonCheckLongArray(args, i, a, n);
}
int vtkPythonCheckArray(PyObject *args, int i, unsigned __int64 *a, int n)
{
  return vtkPythonCheckLongArray(args, i, a, n);
}
#endif

//--------------------------------------------------------------------
void vtkPythonVoidFunc(void *arg)
{
  PyObject *arglist, *result;
  PyObject *func = (PyObject *)arg;

  // Sometimes it is possible for the function to be invoked after
  // Py_Finalize is called, this will cause nasty errors so we return if
  // the interpreter is not initialized.
  if (Py_IsInitialized() == 0)
    {
    return;
    }

#ifndef VTK_NO_PYTHON_THREADS
#if (PY_MAJOR_VERSION > 2) || \
((PY_MAJOR_VERSION == 2) && (PY_MINOR_VERSION >= 3))
  PyGILState_STATE state = PyGILState_Ensure();
#endif
#endif
  
  arglist = Py_BuildValue((char*)"()");

  result = PyEval_CallObject(func, arglist);
  Py_DECREF(arglist);

  if (result)
    {
    Py_XDECREF(result);
    }
  else
    {
    if (PyErr_ExceptionMatches(PyExc_KeyboardInterrupt))
      {
      cerr << "Caught a Ctrl-C within python, exiting program.\n";
      Py_Exit(1);
      }
    PyErr_Print();
    }

#ifndef VTK_NO_PYTHON_THREADS
#if (PY_MAJOR_VERSION > 2) || \
((PY_MAJOR_VERSION == 2) && (PY_MINOR_VERSION >= 3))
  PyGILState_Release(state);
#endif
#endif
}

//--------------------------------------------------------------------
void vtkPythonVoidFuncArgDelete(void *arg)
{
  PyObject *func = (PyObject *)arg;

  // Sometimes it is possible for the function to be invoked after
  // Py_Finalize is called, this will cause nasty errors so we return if
  // the interpreter is not initialized.
  if (Py_IsInitialized() == 0)
    {
    return;
    }

#ifndef VTK_NO_PYTHON_THREADS
#if (PY_MAJOR_VERSION > 2) || \
((PY_MAJOR_VERSION == 2) && (PY_MINOR_VERSION >= 3))
  PyGILState_STATE state = PyGILState_Ensure();
#endif
#endif
  
  if (func)
    {
    Py_DECREF(func);
    }

#ifndef VTK_NO_PYTHON_THREADS
#if (PY_MAJOR_VERSION > 2) || \
((PY_MAJOR_VERSION == 2) && (PY_MINOR_VERSION >= 3))
  PyGILState_Release(state);
#endif
#endif
}
  
//--------------------------------------------------------------------
vtkPythonCommand::vtkPythonCommand()
{ 
  this->obj = NULL;
  this->ThreadState = NULL;
}

vtkPythonCommand::~vtkPythonCommand()
{
  if (this->obj && Py_IsInitialized())
    {
    Py_DECREF(this->obj);
    }
  this->obj = NULL;
}

void vtkPythonCommand::SetObject(PyObject *o)
{ 
  this->obj = o; 
}

void vtkPythonCommand::SetThreadState(PyThreadState *ts)
{ 
  this->ThreadState = ts; 
}

void vtkPythonCommand::Execute(vtkObject *ptr, unsigned long eventtype,
                               void *CallData)
{
  PyObject *arglist, *result, *obj2;
  const char *eventname;

  // Sometimes it is possible for the command to be invoked after
  // Py_Finalize is called, this will cause nasty errors so we return if
  // the interpreter is not initialized.
  if (Py_IsInitialized() == 0)
    {
    return;
    }

#ifndef VTK_NO_PYTHON_THREADS
#if (PY_MAJOR_VERSION > 2) || \
((PY_MAJOR_VERSION == 2) && (PY_MINOR_VERSION >= 3))
  PyGILState_STATE state = PyGILState_Ensure();
#endif
#endif
  
  // If a threadstate has been set using vtkPythonCommand::SetThreadState,
  // then swap it in here.  See the email to vtk-developers@vtk.org from
  // June 18, 2009 with subject "Py_NewInterpreter and vtkPythonCallback issue"
  PyThreadState* prevThreadState = NULL;
  if (this->ThreadState)
    {
    prevThreadState = PyThreadState_Swap(this->ThreadState);
    }

  if (ptr && ptr->GetReferenceCount() > 0)
    {
    obj2 = vtkPythonGetObjectFromPointer(ptr);
    }
  else
    {
    Py_INCREF(Py_None);
    obj2 = Py_None;
    }

  eventname = this->GetStringFromEventId(eventtype);
  
  // extension by Charl P. Botha so that CallData is available from Python:
  // * CallData used to be ignored completely: this is not entirely desirable,
  //   e.g. with catching ErrorEvent
  // * I have extended this code so that CallData can be caught whilst not
  //   affecting any existing VTK Python code
  // * make sure your observer python function has a CallDataType string
  //   attribute that describes how CallData should be passed through, e.g.:
  //   def handler(theObject, eventType, message):
  //      print "Error: %s" % (message)
  //   # we know that ErrorEvent passes a null-terminated string
  //   handler.CallDataType = "string0" 
  //   someObject.AddObserver('ErrorEvent', handler)
  // * At the moment, only string0 is supported as that is what ErrorEvent
  //   generates.
  //                                    
  char CallDataTypeLiteral[] = "CallDataType"; // Need char*, not const char*.
  PyObject *CallDataTypeObj = PyObject_GetAttrString(this->obj,
                                                     CallDataTypeLiteral);
  char *CallDataTypeString = NULL;
  if (CallDataTypeObj)
    {
    CallDataTypeString = PyString_AsString(CallDataTypeObj);
    if (CallDataTypeString)
        {
        if (strcmp(CallDataTypeString, "string0") == 0)
            {
            // this means the user wants the CallData cast as a string
            PyObject* CallDataAsString = PyString_FromString((char*)CallData);
            if (CallDataAsString)
                {
                arglist = Py_BuildValue((char*)"(NsN)", obj2, eventname, CallDataAsString);
                }
            else
                {
                PyErr_Clear();
                // we couldn't create a string, so we pass in None
                Py_INCREF(Py_None);
                arglist = Py_BuildValue((char*)"(NsN)", obj2, eventname, Py_None);
                }
            }
        else
            {
            // we don't handle this, so we pass in a None as the third parameter
            Py_INCREF(Py_None);
            arglist = Py_BuildValue((char*)"(NsN)", obj2, eventname, Py_None);
            }
        }
    else
        {
        // the handler object has a CallDataType attribute, but it's not a
        // string -- then we do traditional arguments
        arglist = Py_BuildValue((char*)"(Ns)",obj2,eventname);
        }
        
    // we have to do this
    Py_DECREF(CallDataTypeObj);
    }
  else
    {
    // this means there was no CallDataType attribute, so we do the
    // traditional obj(object, eventname) call
    PyErr_Clear();
    arglist = Py_BuildValue((char*)"(Ns)",obj2,eventname);
    }
  
  result = PyEval_CallObject(this->obj, arglist);
  Py_DECREF(arglist);
  
  if (result)
    {
    Py_DECREF(result);
    }
  else
    {
    if (PyErr_ExceptionMatches(PyExc_KeyboardInterrupt))
      {
      cerr << "Caught a Ctrl-C within python, exiting program.\n";
      Py_Exit(1);
      }
    PyErr_Print();
    }

  // If we did the swap near the top of this function then swap back now.
  if (this->ThreadState)
    {
    PyThreadState_Swap(prevThreadState);
    }

#ifndef VTK_NO_PYTHON_THREADS
#if (PY_MAJOR_VERSION > 2) || \
((PY_MAJOR_VERSION == 2) && (PY_MINOR_VERSION >= 3))
  PyGILState_Release(state);
#endif
#endif
}
//--------------------------------------------------------------------



