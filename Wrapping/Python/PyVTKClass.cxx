/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PyVTKClass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PyVTKClass was created in Oct 2000 by David Gobbi for VTK 3.2.
  The PyVTKClassMetaType was created in Jan 2002 by David Gobbi.

  A PyVTKClass is a python object that represents a VTK class.
  It is a callable object, and calling it will result in the
  creation of a PyVTKObject of the class.  All of the class methods
  are stored in the __dict__ of the PyVTKClass.

  The PyVTKClassMetaType allows subclassing of a PyVTKClass within
  python.  Like PyVTKClass, PyVTKClassMetaType is also a callable
  object, and calling it will produce a new PyVTKClass.  This
  underlying mechanism is hidden from the user, who will just use
  the usual "class vtkMyReader(vtk.vtkImageReader):" syntax.
-----------------------------------------------------------------------*/

#include "PyVTKClass.h"
#include "vtkPythonUtil.h"

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
      PyObject *obj = PyVTKObject_New((PyObject *)self, NULL);
      PyObject *cinitfunc = PyObject_GetAttr(obj, initstr);
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
    return vtkPythonUtil::GetObjectFromObject(
      arg, PyString_AsString(self->vtk_name));
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
  (char*)"A generator for VTK objects.  Special attributes are: __bases__ (a tuple of base classes), __dict__ (methods and attributes), __doc__ (the docstring for the class), __name__ (the name of class), and __module__ (module that the class is defined in).", // tp_doc
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

PyObject *PyVTKClass_New(vtknewfunc constructor, PyMethodDef *methods,
                         const char *classname, const char *modulename,
                         const char *docstring[], PyObject *base)
{
  static PyObject *modulestr[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  static int nmodulestr = 10;
  PyObject *moduleobj = 0;
  int i;

  PyObject *self = vtkPythonUtil::FindClass(classname);

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
    class_self->vtk_name = PyString_FromString((char *)classname);

    class_self->vtk_getattr = NULL;
    class_self->vtk_setattr = NULL;
    class_self->vtk_delattr = NULL;

    class_self->vtk_methods = methods;
    class_self->vtk_new = constructor;
    class_self->vtk_doc = vtkPythonUtil::BuildDocString(docstring);

    // intern the module string
    for (i = 0; i < nmodulestr; i++)
      {
      if (modulestr[i] == 0)
        {
        modulestr[i] = PyString_InternFromString((char *)modulename);
        moduleobj = modulestr[i];
        Py_INCREF(moduleobj);
        break;
        }
      else if (strcmp(modulename, PyString_AsString(modulestr[i])) == 0)
        {
        moduleobj = modulestr[i];
        Py_INCREF(moduleobj);
        break;
        }
      }
    if (i == nmodulestr)
      {
      moduleobj = PyString_FromString((char *)modulename);
      }

    class_self->vtk_module = moduleobj;

    vtkPythonUtil::AddClassToMap(self, classname);
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
