/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonUtil.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPythonUtil.h"

#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkString.h"
#include "vtkHashMap.txx"
#include "vtkHashMapIterator.txx"

#if defined ( _MSC_VER )
#  define vtkConvertPtrToLong(x) ((long)(PtrToUlong(x)))
#else
#  define vtkConvertPtrToLong(x) ((long)(x))
#endif

//#define VTKPYTHONDEBUG

//--------------------------------------------------------------------
// There are two hash tables associated with the Python wrappers
static inline vtkIdType vtkHashMapHashMethod(const vtkObject *const &x)
{
  return reinterpret_cast<vtkIdType>(x);
}

class vtkPythonUtil
{
public:
  vtkPythonUtil();
  ~vtkPythonUtil();

  vtkHashMap<vtkObject *, PyObject *> *ObjectHash;
  vtkHashMap<const char *, PyObject *> *ClassHash;
};

//--------------------------------------------------------------------
vtkPythonUtil *vtkPythonHash = NULL;

//--------------------------------------------------------------------
vtkPythonUtil::vtkPythonUtil()
{
  this->ObjectHash = vtkHashMap<vtkObject *, PyObject *>::New();
  this->ClassHash = vtkHashMap<const char *, PyObject *>::New();
}

//--------------------------------------------------------------------
vtkPythonUtil::~vtkPythonUtil()
{
  this->ObjectHash->Delete();
  this->ClassHash->Delete();
}

//--------------------------------------------------------------------
static PyObject *PyVTKObject_PyString(PyVTKObject *self)
{
  PyObject *func = PyObject_GetAttrString((PyObject *)self, "__str__");

  if (func)
    {
    PyObject *res = PyEval_CallObject(func, (PyObject *)NULL);
    Py_DECREF(func);
    return res;
    }
  PyErr_Clear();

  ostrstream buf;
  self->vtk_ptr->Print(buf);
  buf.put('\0');
  PyObject *res = PyString_FromString(buf.str());
  delete buf.str();
  return res;
}

//--------------------------------------------------------------------
static PyObject *PyVTKObject_PyRepr(PyVTKObject *self)
{
  PyObject *func = PyObject_GetAttrString((PyObject *)self, "__repr__");

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
      PyObject *args = Py_BuildValue("(OOO)", self, attr, value);
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
      PyObject *args = Py_BuildValue("(OO)", self, attr);
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
      sprintf(buf,"%s_p", self->vtk_ptr->GetClassName());
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
    }

  while (pyclass != NULL)
    {
    PyMethodDef *m;

    if (pyclass->vtk_dict)
      {
      value = PyDict_GetItem(pyclass->vtk_dict, attr);
    
      if (value)
        {
        if (PyCallable_Check(value))
          {
          return PyMethod_New(value, (PyObject *)self, (PyObject *)pyclass);
          }
        Py_INCREF(value);
        return value;
        }
      }

    for (m = pyclass->vtk_methods; m && m->ml_name; m++)
      {
      if (name[0] == m->ml_name[0] && strcmp(name+1, m->ml_name+1) == 0)
        {
        return PyCFunction_New(m, (PyObject *)self);
        }
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
    PyObject *args = Py_BuildValue("(OO)", self, attr);
    PyObject *res = PyEval_CallObject(pyclass->vtk_getattr, args);
    Py_DECREF(args);
    return res;
    }

  PyErr_SetString(PyExc_AttributeError, name);
  return NULL;
}

//--------------------------------------------------------------------
class vtkPythonDeleteCommand : public vtkCommand
{
public:
  static vtkPythonDeleteCommand *New(PyVTKObject *obj) {
    return new vtkPythonDeleteCommand(obj); };

  void Execute(vtkObject *caller, unsigned long, void *);

private:
  vtkPythonDeleteCommand(PyVTKObject *obj) : Self(obj) {};

  PyVTKObject *Self;
};

void vtkPythonDeleteCommand::Execute(vtkObject *caller,
                                     unsigned long vtkNotUsed(id),
                                     void *vtkNotUsed(data))
{
  if (this->Self->vtk_ptr != caller)
    {
    vtkGenericWarningMacro("Python vs. VTK mismatch for " << caller);
    return;
    }

  vtkPythonDeleteObjectFromHash((PyObject *)this->Self);
  Py_DECREF((PyObject *)this->Self->vtk_class);
  Py_DECREF(this->Self->vtk_dict);
  PyMem_DEL(this->Self);
}

//--------------------------------------------------------------------
static void PyVTKObject_PyDelete(PyVTKObject *self)
{
  self->vtk_ptr->Delete();
  // the rest of the delection is handled when the VTK-level object
  // is destroyed
  vtkPythonDeleteObjectFromHash((PyObject *)self);
  Py_DECREF((PyObject *)self->vtk_class);
  Py_DECREF(self->vtk_dict);
  PyMem_DEL(self);
}

//--------------------------------------------------------------------
static PyTypeObject PyVTKObjectType = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  "vtkobject",                           // tp_name
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
  0,                                     // tp_as_buffer
  0,                                     // tp_flags
  "A VTK object.  Special attributes are:  __class__ (the class that this object belongs to), __dict__ (user-controlled attributes), __doc__ (the docstring for the class), __methods__ (a list of all methods for this object), and __this__ (a string that contains the hexidecimal address of the underlying VTK object)"  // tp_doc
};

int PyVTKObject_Check(PyObject *obj)
{
  return (obj->ob_type == &PyVTKObjectType);
}

PyObject *PyVTKObject_New(PyObject *pyvtkclass, vtkObject *ptr)
{
  PyVTKClass *vtkclass = (PyVTKClass *)pyvtkclass;

  if (ptr)
    {
    ptr->Register(NULL);
    }
  else if (vtkclass->vtk_new != NULL)
    {
    ptr = vtkclass->vtk_new();
    }
  else
    {
    PyErr_SetString(PyExc_TypeError,
                    "this is an abstract class and cannot be instantiated");
    return 0;
    }
  PyVTKObject *self = PyObject_NEW(PyVTKObject, &PyVTKObjectType);
  self->vtk_ptr = ptr;
  PyObject *cls = NULL;
  vtkPythonHash->ClassHash->GetItem(ptr->GetClassName(), cls);
  self->vtk_class = (PyVTKClass *)cls;
  
  // If the class was not in the dictionary (i.e. if there is no 'python'
  // level class to support the VTK level class) we fall back to this.
  if (self->vtk_class == NULL || vtkclass->vtk_methods == NULL)
    {
    self->vtk_class = vtkclass;
    }
  
  Py_INCREF(self->vtk_class);
  
  self->vtk_dict = PyDict_New();

  vtkPythonAddObjectToHash((PyObject *)self, ptr);
  /* I'll reinstate this later
  ptr->AddObserver(vtkCommand::DeleteEvent,
                   vtkPythonDeleteCommand::New(self));
  */
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
      PyObject *initfunc = PyVTKObject_PyGetAttr((PyVTKObject *)obj, initstr);
      PyObject *res = PyEval_CallObjectWithKeywords(initfunc, arg, kw);
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
      Py_DECREF(initfunc);
      return obj;
      }
    }

  if (kw != NULL)
    {
    PyErr_SetString(PyExc_TypeError,
                    "this function takes no keyword arguments");
    return NULL;
    }
  if (PyArg_ParseTuple(arg,""))
    {
    return PyVTKObject_New((PyObject *)self, NULL);
    }
  PyErr_Clear();
  if (PyArg_ParseTuple(arg,"O", &arg))
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

    if (pyclass->vtk_dict)
      {
      PyObject *value;

      value = PyDict_GetItem(pyclass->vtk_dict, attr);
    
      if (value)
        {
        Py_INCREF(value);
        return value;
        }
      }

    for (meth = pyclass->vtk_methods; meth && meth->ml_name; meth++)
      {
      if (name[0] == meth->ml_name[0] && strcmp(name+1, meth->ml_name+1) == 0)
        {
        return PyCFunction_New(meth, (PyObject *)self);
        }
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
    
  PyMem_DEL(self);
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
    char *doc = t->tp_doc;
    if (doc != NULL)
      {
      return PyString_FromString(doc);
      }
    Py_INCREF(Py_None);
    return Py_None;
    }
  if (strcmp(name, "__members__") == 0)
    {
    return Py_BuildValue("[ss]", "__doc__", "__name__");
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
  "vtkclass type",          /* Name of this type */
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
  "Define the behavior of a particular type of object.",
};

//--------------------------------------------------------------------
static PyTypeObject PyVTKClassType = {
  PyObject_HEAD_INIT(&PyVTKClassMetaType)
  0,
  "vtkclass",                            // tp_name
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
  "A generator for VTK objects.  Special attributes are: __bases__ (a tuple of base classes), __dict__ (user-defined methods and attributes), __doc__ (the docstring for the class), __name__ (the name of class), __methods__ (methods for this class, not including inherited methods or user-defined methods), and __module__ (module that the class is defined in)." // tp_doc
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

  for (n = 0; docstring[n] != NULL; n++);

  m = new int[n];

  for (i = 0; i < n; i++)
    {
    m[i] = vtkString::Length(docstring[i]);
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
    vtkPythonHash->ClassHash->GetItem(classname, self);
    }
  if (self)
    {
    Py_INCREF(self);
    }
  else
    {
    PyVTKClass *class_self = PyObject_NEW(PyVTKClass, &PyVTKClassType);
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
  static char *kwlist[] = {"name", "bases", "dict", NULL};

  PyVTKClass *newclass;
  char *classname;
  PyObject *globals;
  PyObject *bases;
  PyVTKClass *base;
  PyObject *attributes;

  if ((PyArg_ParseTupleAndKeywords(args, kw, "sOO", kwlist, 
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

    if (PyDict_GetItemString(attributes, "__del__"))
      {
      PyErr_SetString(PyExc_ValueError, "__del__ attribute is not supported");
      return NULL;
      }

    newclass = PyObject_NEW(PyVTKClass, &PyVTKClassType);

    Py_INCREF(bases);
    Py_INCREF(attributes);

    newclass->vtk_bases = bases;
    newclass->vtk_dict = attributes;
    newclass->vtk_name = PyString_FromString(classname);

    newclass->vtk_getattr = PyDict_GetItemString(attributes, "__getattr__");
    if (newclass->vtk_getattr == 0)
      {
      newclass->vtk_getattr = base->vtk_getattr;
      }
    Py_XINCREF(newclass->vtk_getattr);
    newclass->vtk_setattr = PyDict_GetItemString(attributes, "__setattr__");
    if (newclass->vtk_setattr == 0)
      {
      newclass->vtk_setattr = base->vtk_setattr;
      }
    Py_XINCREF(newclass->vtk_setattr);
    newclass->vtk_delattr = PyDict_GetItemString(attributes, "__delattr__");
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
      PyObject *modname = PyDict_GetItemString(globals, "__name__");
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

    newclass->vtk_doc = PyDict_GetItemString(attributes, "__doc__");
    if (newclass->vtk_doc)
      {
      Py_INCREF(newclass->vtk_doc);
      PyDict_DelItemString(attributes, "__doc__");
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
      PyMethodDef *meth = self->vtk_methods;
      PyObject *lst;
      int i, n;

      for (n = 0; meth && meth[n].ml_name; n++);

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
  PyMem_DEL(self);
}

//--------------------------------------------------------------------
static PyTypeObject PyVTKSpecialObjectType = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  "vtkspecialobject",                    // tp_name
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
  "vtkspecialobject - a vtk object not derived from vtkObject." // tp_doc
};

int PyVTKSpecialObject_Check(PyObject *obj)
{
  return (obj->ob_type == &PyVTKSpecialObjectType);
}

PyObject *PyVTKSpecialObject_New(void *ptr, PyMethodDef *methods,
                                 char *classname, char *docstring[])
{
  PyVTKSpecialObject *self = PyObject_NEW(PyVTKSpecialObject, 
                                          &PyVTKSpecialObjectType);
  self->vtk_ptr = ptr;
  self->vtk_methods = methods;
  self->vtk_name = PyString_FromString(classname);
  self->vtk_doc = vtkBuildDocString(docstring);
  
  return (PyObject *)self;
}

//--------------------------------------------------------------------
vtkObject *PyArg_VTKParseTuple(PyObject *pself, PyObject *args, 
                               char *format, ...)
{
  PyVTKObject *self = (PyVTKObject *)pself;
  vtkObject *obj = NULL;
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
    }

#ifdef VTKPYTHONDEBUG
  //  vtkGenericWarningMacro("Adding an type " << type << " to hash ptr");
#endif  

  // lets make sure it isn't already there
  PyObject *tmp = NULL;
  if (vtkPythonHash->ClassHash->GetItem(classname, tmp) == VTK_OK)
    {
#ifdef VTKPYTHONDEBUG
    vtkGenericWarningMacro("Attempt to add type to the hash when already there!!!");
#endif
    return;
    }

  vtkPythonHash->ClassHash->SetItem(classname, vtkclass);

#ifdef VTKPYTHONDEBUG
  //  vtkGenericWarningMacro("Added type to hash type = " << typeObject);
#endif  
}  

//--------------------------------------------------------------------
void vtkPythonAddObjectToHash(PyObject *obj, vtkObject *ptr)
{
  if (vtkPythonHash == NULL)
    {
    vtkPythonHash = new vtkPythonUtil();
    }

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Adding an object to hash ptr = " << ptr);
#endif  

  ((PyVTKObject *)obj)->vtk_ptr = ptr;
  vtkPythonHash->ObjectHash->SetItem(ptr, obj);

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Added object to hash obj= " << obj << " " 
                         << ptr);
#endif  
}  

//--------------------------------------------------------------------
void vtkPythonDeleteObjectFromHash(PyObject *obj)
{
  vtkObject *ptr = ((PyVTKObject *)obj)->vtk_ptr;

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Deleting an object from hash obj = " << obj << " "
                         << obj->vtk_ptr);
#endif  

  vtkPythonHash->ObjectHash->RemoveItem(ptr);
}

//--------------------------------------------------------------------
static PyObject *vtkFindNearestBase(vtkObject *ptr);

PyObject *vtkPythonGetObjectFromPointer(vtkObject *ptr)
{
  PyObject *obj = NULL;

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Checking into pointer " << ptr);
#endif
  
  if (ptr)
    {
    vtkPythonHash->ObjectHash->GetItem(ptr, obj);

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
    vtkPythonHash->ClassHash->GetItem(ptr->GetClassName(),vtkclass);

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
static PyObject *vtkFindNearestBase(vtkObject *ptr)
{
  vtkHashMap<const char *, PyObject *>::IteratorType *classes = 
    vtkPythonHash->ClassHash->NewIterator();

  PyObject *nearestbase = NULL;
  int maxdepth = 0;
  int depth;

  for(classes->InitTraversal();
      !classes->IsDoneWithTraversal();
      classes->GoToNextItem())
    {
    PyObject *pyclass = NULL;
    classes->GetData(pyclass);

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
      // we want the class that is furthest from vtkObject
      if (depth > maxdepth)
        {
        maxdepth = depth;
        nearestbase = pyclass;
        }
      }
    }
  
  classes->Delete();

  return nearestbase;
}

//--------------------------------------------------------------------
vtkObject *vtkPythonGetPointerFromObject(PyObject *obj,
                                         const char *result_type)
{ 
  vtkObject *ptr;

  // convert Py_None to NULL every time
  if (obj == Py_None)
    {
      return NULL;
    }

  // check to ensure it is a vtk object
  if (obj->ob_type != &PyVTKObjectType)
    {
    obj = PyObject_GetAttrString(obj,"__vtk__");
    if (obj)
      {
      PyObject *arglist = Py_BuildValue("()");
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
            result_type,((vtkObject *)ptr)->GetClassName());
    PyErr_SetString(PyExc_ValueError,error_string);
    return NULL;
    }
}

PyObject *vtkPythonGetObjectFromObject(PyObject *arg, const char *type)
{
  if (PyString_Check(arg))
    {
    char *ptrText = PyString_AsString(arg);

    vtkObject *ptr;
    char typeCheck[256];  // typeCheck is currently not used
    int i = sscanf(ptrText,"_%lx_%s",(long *)&ptr,typeCheck);
      
    if (i <= 0)
      {
      i = sscanf(ptrText,"Addr=0x%lx",(long *)&ptr);
      }      
    if (i <= 0)
      {
      i = sscanf(ptrText,"%lx",(long *)&ptr);
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
              type,((vtkObject *)ptr)->GetClassName());
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
  char typeCheck[128];
  if (*len < 128)
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
static inline
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
static inline
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

int vtkPythonCheckArray(PyObject *args, int i, char *a, int n)
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

//--------------------------------------------------------------------
void vtkPythonVoidFunc(void *arg)
{
  PyObject *arglist, *result;
  PyObject *func = (PyObject *)arg;

  arglist = Py_BuildValue("()");

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
}

//--------------------------------------------------------------------
void vtkPythonVoidFuncArgDelete(void *arg)
{
  PyObject *func = (PyObject *)arg;
  if (func)
    {
    Py_DECREF(func);
    }
}
  
//--------------------------------------------------------------------
vtkPythonCommand::vtkPythonCommand()
{ 
  this->obj = NULL;
}

vtkPythonCommand::~vtkPythonCommand()
{ 
  if (this->obj)
    {
    Py_DECREF(this->obj);
    }
  this->obj = NULL;
}

void vtkPythonCommand::SetObject(PyObject *o)
{ 
  this->obj = o; 
}

void vtkPythonCommand::Execute(vtkObject *ptr, unsigned long eventtype, 
                               void *)
{
  PyObject *arglist, *result, *obj2;
  const char *eventname;

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
  
  arglist = Py_BuildValue("(Ns)",obj2,eventname);
  
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
}
//--------------------------------------------------------------------



