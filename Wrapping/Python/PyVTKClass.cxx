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
static PyObject *PyVTKClass_String(PyObject *op)
{
  PyVTKClass *self = (PyVTKClass *)op;
  char buf[1024];
  sprintf(buf,"%.500s.%.500s",
          PyString_AsString(self->vtk_module),
          PyString_AsString(self->vtk_name));

  return PyString_FromString(buf);
}

//--------------------------------------------------------------------
static PyObject *PyVTKClass_Repr(PyObject *op)
{
  PyVTKClass *self = (PyVTKClass *)op;
  char buf[512];
  sprintf(buf,"<%.80s %.80s.%.80s>", op->ob_type->tp_name,
          PyString_AsString(self->vtk_module),
          PyString_AsString(self->vtk_name));

  return PyString_FromString(buf);
}

//--------------------------------------------------------------------
static PyObject *PyVTKClass_Call(PyObject *op, PyObject *arg, PyObject *kw)
{
  PyVTKClass *self = (PyVTKClass *)op;
  static PyObject *initstr = 0;

  if (self->vtk_dict)
    {
    if (initstr == 0)
      {
      initstr = PyString_FromString("__init__");
      }

    PyObject *initfunc;
    initfunc = PyDict_GetItem(self->vtk_dict, initstr);

    if (initfunc)
      {
      PyObject *obj = PyVTKObject_New((PyObject *)self, NULL, NULL);
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
    return PyVTKObject_New((PyObject *)self, NULL, NULL);
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
static PyObject *PyVTKClass_GetAttr(PyObject *op, PyObject *attr)
{
  char *name = PyString_AsString(attr);
  PyVTKClass *pyclass = (PyVTKClass *)op;
  PyObject *bases;

  while (pyclass != NULL)
    {
    PyObject *value;
    PyObject *dict;

    dict = PyVTKClass_GetDict((PyObject *)pyclass);
    value = PyDict_GetItem(dict, attr);

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
    pyclass = (PyVTKClass *)op;

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
#if PY_VERSION_HEX >= 0x02020000
static PyObject *PyVTKClass_Dir(PyObject *op, PyObject *arg)
{
  /* depending on where it is bound */
  if (arg && PyVTKClass_Check(arg))
    {
    op = arg;
    }

  PyVTKClass *pyclass = (PyVTKClass *)op;
  PyObject *bases;
  Py_ssize_t n;
  for (n = 0; pyclass != NULL; n++)
    {
    bases = pyclass->vtk_bases;
    pyclass = NULL;
    if (PyTuple_Size(bases))
      {
      pyclass = (PyVTKClass *)PyTuple_GetItem(bases,0);
      }
    }

  PyObject **mro = new PyObject *[n];
  pyclass = (PyVTKClass *)op;
  for (n = 0; pyclass != NULL; n++)
    {
    mro[n] = (PyObject *)pyclass;
    bases = pyclass->vtk_bases;
    pyclass = NULL;
    if (PyTuple_Size(bases))
      {
      pyclass = (PyVTKClass *)PyTuple_GetItem(bases,0);
      }
    }

  PyObject *dict = PyDict_New();
  do
    {
    PyDict_Update(dict, PyVTKClass_GetDict(mro[--n]));
    }
  while (n > 0);
  delete [] mro;

  if (PyVTKObject_Check(arg) && ((PyVTKObject *)arg)->vtk_dict)
    {
    PyDict_Update(dict, ((PyVTKObject *)arg)->vtk_dict);
    }

  PyObject *rval = PyDict_Keys(dict);
  Py_DECREF(dict);

  return rval;
}

static PyMethodDef PyVTKClass_Dir_Method =
{ (char *)"__dir__", PyVTKClass_Dir, METH_O, (char *)"class directory" };
#endif

//--------------------------------------------------------------------
#if PY_MAJOR_VERSION >= 2
static int PyVTKClass_Traverse(PyObject *o, visitproc visit, void *arg)
{
  PyVTKClass *self = (PyVTKClass *)o;
  PyObject *members[8];
  int err = 0;
  int i;

  members[0] = self->vtk_bases;
  members[1] = self->vtk_dict;
  members[2] = self->vtk_name;
  members[3] = self->vtk_getattr;
  members[4] = self->vtk_setattr;
  members[5] = self->vtk_delattr;
  members[6] = self->vtk_module;
  members[7] = self->vtk_doc;

  for (i = 0; i < 8 && err == 0; i++)
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
static void PyVTKClass_Delete(PyObject *op)
{
  PyVTKClass *self = (PyVTKClass *)op;

#if PY_VERSION_HEX >= 0x02020000
  PyObject_GC_UnTrack(op);
#endif
  Py_XDECREF(self->vtk_bases);
  Py_XDECREF(self->vtk_dict);
  Py_XDECREF(self->vtk_name);

  Py_XDECREF(self->vtk_getattr);
  Py_XDECREF(self->vtk_setattr);
  Py_XDECREF(self->vtk_delattr);

  Py_XDECREF(self->vtk_module);
  Py_XDECREF(self->vtk_doc);

#if PY_VERSION_HEX >= 0x02020000
  PyObject_GC_Del(op);
#elif PY_MAJOR_VERSION >= 2
  PyObject_Del(op);
#else
  PyMem_DEL(op);
#endif
}

//--------------------------------------------------------------------
static PyObject *PyVTKClassMetaType_GetAttr(PyObject *op, PyObject *attr)
{
  PyTypeObject *t = (PyTypeObject *)op;
  const char *name = PyString_AsString(attr);

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
#if PY_VERSION_HEX >= 0x02020000
  if (strcmp(name,"__dir__") == 0)
    {
    return PyCFunction_New(&PyVTKClass_Dir_Method, op);
    }
#endif

  PyErr_SetString(PyExc_AttributeError, name);
  return NULL;
}

//--------------------------------------------------------------------
static PyObject *PyVTKClassMetaType_Repr(PyObject *op)
{
  char buf[100];
  sprintf(buf, "<type '%.80s'>", ((PyTypeObject *)op)->tp_name);
  return PyString_FromString(buf);
}

//--------------------------------------------------------------------
static PyObject *PyVTKClass_NewSubclass(PyObject *self, PyObject *args,
                                        PyObject *kw);

//--------------------------------------------------------------------
static PyTypeObject PyVTKClassMetaType = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  (char*)"vtkclass type",                // tp_name
  sizeof(PyTypeObject),                  // tp_basicsize
  0,                                     // tp_itemsize
  0,                                     // tp_dealloc
  0,                                     // tp_print
  0,                                     // tp_getattr
  0,                                     // tp_setattr
  0,                                     // tp_compare
  PyVTKClassMetaType_Repr,               // tp_repr
  0,                                     // tp_as_number
  0,                                     // tp_as_sequence
  0,                                     // tp_as_mapping
  0,                                     // tp_hash
  PyVTKClass_NewSubclass,                // tp_call
  0,                                     // tp_string
  PyVTKClassMetaType_GetAttr,            // tp_getattro
  0,                                     // tp_setattro
  0,                                     // tp_as_buffer
  0,                                     // tp_flags
  (char*)"PyVTKClassMetaType, part of the subclassing mechanism for VTK classes.",
#if PY_MAJOR_VERSION >= 2
  0,                                     // tp_traverse
  0,                                     // tp_clear
  0,                                     // tp_richcompare
  0,                                     // tp_weaklistoffset
#else
  0, 0, 0, 0,                            // reserved
#endif
  VTK_PYTHON_UTIL_SUPRESS_UNINITIALIZED
};

//--------------------------------------------------------------------
PyTypeObject PyVTKClass_Type = {
  PyObject_HEAD_INIT(&PyVTKClassMetaType)
  0,
  (char*)"vtkclass",                     // tp_name
  sizeof(PyVTKClass),                    // tp_basicsize
  0,                                     // tp_itemsize
  PyVTKClass_Delete,                     // tp_dealloc
  0,                                     // tp_print
  0,                                     // tp_getattr
  0,                                     // tp_setattr
  0,                                     // tp_compare
  PyVTKClass_Repr,                       // tp_repr
  0,                                     // tp_as_number
  0,                                     // tp_as_sequence
  0,                                     // tp_as_mapping
  0,                                     // tp_hash
  PyVTKClass_Call,                       // tp_call
  PyVTKClass_String,                     // tp_string
  PyVTKClass_GetAttr,                    // tp_getattro
  0,                                     // tp_setattro
  0,                                     // tp_as_buffer
#if PY_VERSION_HEX >= 0x02020000
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, // tp_flags
#else
  0,                                     // tp_flags
#endif
  (char*)"A generator for VTK objects.  Special attributes are: __bases__ (a tuple of base classes), __dict__ (methods and attributes), __doc__ (the docstring for the class), __name__ (the name of class), and __module__ (module that the class is defined in).", // tp_doc
#if PY_MAJOR_VERSION >= 2
  PyVTKClass_Traverse,                   // tp_traverse
  0,                                     // tp_clear
  0,                                     // tp_richcompare
  0,                                     // tp_weaklistoffset
#else
  0, 0, 0, 0,                            // reserved
#endif
#if PY_VERSION_HEX >= 0x02020000
  0,                                     // tp_iter
  0,                                     // tp_iternext
  0,                                     // tp_methods
  0,                                     // tp_members
  0,                                     // tp_getset
  &PyClass_Type,                         // tp_base
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

PyObject *PyVTKClass_GetDict(PyObject *obj)
{
  PyMethodDef *meth;
  PyVTKClass *pyclass;
  PyObject *func;

  pyclass = (PyVTKClass *)obj;

  if (pyclass->vtk_dict == NULL)
    {
    pyclass->vtk_dict = PyDict_New();

    for (meth = pyclass->vtk_methods; meth && meth->ml_name; meth++)
      {
      func = PyCFunction_New(meth, (PyObject *)pyclass);
      PyDict_SetItemString(pyclass->vtk_dict, meth->ml_name, func);
      Py_DECREF(func);
      }
    }

  return pyclass->vtk_dict;
}

PyObject *PyVTKClass_New(vtknewfunc constructor, PyMethodDef *methods,
                         const char *classname, const char *modulename,
                         const char *pythonname, const char *manglename,
                         const char *docstring[], PyObject *base)
{
  static PyObject *modulestr[32] = {
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
  static int nmodulestr = 32;
  PyObject *moduleobj = 0;
  PyObject *bases = 0;
  PyObject *name = 0;
  PyObject *doc = 0;
  int i;

  PyObject *self = vtkPythonUtil::FindClass(classname);

  if (self)
    {
    Py_INCREF(self);
    }
  else
    {
    if (base)
      {
      Py_INCREF(base);
      bases = PyTuple_New(1);
      PyTuple_SET_ITEM(bases, 0, base);
      }
    else
      {
      bases = PyTuple_New(0);
      }

    if (!pythonname)
      {
      pythonname = classname;
      }
    name = PyString_FromString((char *)pythonname);

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

    doc = vtkPythonUtil::BuildDocString(docstring);

#if PY_VERSION_HEX >= 0x02020000
    PyVTKClass *class_self = PyObject_GC_New(PyVTKClass, &PyVTKClass_Type);
#elif PY_MAJOR_VERSION >= 2
    PyVTKClass *class_self = PyObject_New(PyVTKClass, &PyVTKClass_Type);
#else
    PyVTKClass *class_self = PyObject_NEW(PyVTKClass, &PyVTKClass_Type);
#endif
    self = (PyObject *)class_self;

    class_self->vtk_bases = bases;
    class_self->vtk_dict = NULL;
    class_self->vtk_name = name;

    class_self->vtk_getattr = NULL;
    class_self->vtk_setattr = NULL;
    class_self->vtk_delattr = NULL;

    class_self->vtk_methods = methods;
    class_self->vtk_new = constructor;
    class_self->vtk_doc = doc;
    class_self->vtk_module = moduleobj;
    class_self->vtk_cppname = classname;
    class_self->vtk_mangle = (manglename ? manglename : classname);

#if PY_VERSION_HEX >= 0x02020000
    PyObject_GC_Track(self);
#endif

    vtkPythonUtil::AddClassToMap(self, classname);
    }

  return self;
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
                      "multiple inheritance is not allowed with VTK classes");
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

#if PY_VERSION_HEX >= 0x02020000
    newclass = PyObject_GC_New(PyVTKClass, &PyVTKClass_Type);
#elif PY_MAJOR_VERSION >= 2
    newclass = PyObject_New(PyVTKClass, &PyVTKClass_Type);
#else
    newclass = PyObject_NEW(PyVTKClass, &PyVTKClass_Type);
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
    newclass->vtk_cppname = classname;
    newclass->vtk_mangle = classname;

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

#if PY_VERSION_HEX >= 0x02020000
    PyObject_GC_Track((PyObject *)newclass);
#endif

    return (PyObject *)newclass;
    }
  return NULL;
}
