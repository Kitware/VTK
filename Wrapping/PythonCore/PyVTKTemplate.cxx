/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PyVTKTemplate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PyVTKTemplate was created in May 2011 by David Gobbi.

  This object is a container for instantiations of templated types.
  Essentially, it is a "dict" that accepts template args as keys,
  and provides the corresponding instantiation of the template.
-----------------------------------------------------------------------*/

#include "PyVTKTemplate.h"
#include "PyVTKClass.h"
#include "vtkPythonUtil.h"

// Silence warning like
// "dereferencing type-punned pointer will break strict-aliasing rules"
// it happens because this kind of expression: (long *)&ptr
// pragma GCC diagnostic is available since gcc>=4.2
#if defined(__GNUG__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=2)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

//--------------------------------------------------------------------

const char *PyVTKTemplate_Doc =
  "A container for instantiations of class and function templates.\n\n"
  "This is a dictionary for templates, provide the template args\n"
  "in square brackets to get the desired kind of class.\n";

//--------------------------------------------------------------------
// methods from python

static PyObject *PyVTKTemplate_HasKey(PyObject *ob, PyObject *args)
{
  PyObject *key = NULL;
  if (PyArg_ParseTuple(args, (char *)"O:has_key", &key))
    {
    ob = ((PyVTKTemplate *)ob)->dict;
    return PyObject_CallMethod(ob, (char *)"has_key", (char *)"(O)", key);
    }
  return NULL;
}

static PyObject *PyVTKTemplate_Keys(PyObject *ob, PyObject *args)
{
  if (PyArg_ParseTuple(args, (char *)":keys"))
    {
    ob = ((PyVTKTemplate *)ob)->dict;
    return PyObject_CallMethod(ob, (char *)"keys", (char *)"()");
    }
  return NULL;
}

static PyObject *PyVTKTemplate_Values(PyObject *ob, PyObject *args)
{
  if (PyArg_ParseTuple(args, (char *)":values"))
    {
    ob = ((PyVTKTemplate *)ob)->dict;
    return PyObject_CallMethod(ob, (char *)"values", (char *)"()");
    }
  return NULL;
}

static PyObject *PyVTKTemplate_Items(PyObject *ob, PyObject *args)
{
  if (PyArg_ParseTuple(args, (char *)":items"))
    {
    ob = ((PyVTKTemplate *)ob)->dict;
    return PyObject_CallMethod(ob, (char *)"items", (char *)"()");
    }
  return NULL;
}

static PyObject *PyVTKTemplate_Get(PyObject *ob, PyObject *args)
{
  PyObject *key = NULL;
  PyObject *def = Py_None;
  if (PyArg_ParseTuple(args, (char *)"O|O:get"))
    {
    ob = ((PyVTKTemplate *)ob)->dict;
    return PyObject_CallMethod(ob, (char *)"get", (char *)"(OO)", key, def);
    }
  return NULL;
}

static PyObject *PyVTKTemplate_Copy(PyObject *ob, PyObject *args)
{
  if (PyArg_ParseTuple(args, (char *)":copy"))
    {
    ob = ((PyVTKTemplate *)ob)->dict;
    return PyObject_CallMethod(ob, (char *)"copy", NULL);
    }
  return NULL;
}


static PyMethodDef PyVTKTemplate_Methods[] = {
  {(char*)"has_key", PyVTKTemplate_HasKey, METH_VARARGS,
   (char *)"T.has_key(args) -> True only the template args are allowed."},
  {(char*)"keys", PyVTKTemplate_Keys, METH_VARARGS,
   (char *)"T.keys() -> list of allowed template args."},
  {(char*)"values", PyVTKTemplate_Values, METH_VARARGS,
   (char *)"T.values() -> list of provided template instantiations."},
  {(char*)"items", PyVTKTemplate_Items, METH_VARARGS,
   (char *)"T.items() -> list of (args,types) pairs."},
  {(char*)"get", PyVTKTemplate_Get, METH_VARARGS,
   (char *)"T.get(args) -> get instantiated template type or None."},
  {(char*)"copy", PyVTKTemplate_Copy, METH_VARARGS,
   (char *)"T.copy() -> get a shallow copy of T."},
  { NULL, NULL, 0, NULL }
};

//--------------------------------------------------------------------
// Mapping protocol

static Py_ssize_t
PyVTKTemplate_Size(PyObject *ob)
{
  ob = ((PyVTKTemplate *)ob)->dict;
#if PY_MAJOR_VERSION >= 2
  return PyObject_Size(ob);
#else
  return PyObject_Length(ob);
#endif
}

static PyObject *
PyVTKTemplate_GetItem(PyObject *ob, PyObject *key)
{
  static const char *typenames[] = {
    "bool", "char",
    "int8", "uint8", "int16", "uint16", "int32", "uint32",
    "int", "uint", "int64", "uint64",
    "float32", "float64",
    "str", "unicode",
    NULL };

  static const char *typecodes[] = {
    "?", "c",
    "b", "B", "h", "H", "i", "I",
    "l", "L", "q", "Q",
    "f", "d",
    NULL, NULL,
    NULL };

  Py_ssize_t nargs = 1;
  bool multi = false;

  if (PyTuple_Check(key))
    {
    nargs = PyTuple_GET_SIZE(key);
    multi = true;
    }

  PyObject *o = key;
  PyObject *t = PyTuple_New(nargs);

  for (Py_ssize_t i = 0; i < nargs; i++)
    {
    o = key;
    if (multi)
      {
      o = PyTuple_GET_ITEM(key, i);
      }

    if (PyType_Check(o))
      {
      const char *cp = ((PyTypeObject *)o)->tp_name;
      size_t n = strlen(cp);
      if (n == 5 && strcmp(cp, "float") == 0)
        {
        cp = "float64";
        n += 2;
        }
      while (n && cp[n-1] != '.') { --n; }
      o = PyString_FromString(&cp[n]);
      }
    else if (PyVTKClass_Check(o))
      {
      o = ((PyVTKClass *)o)->vtk_name;
      Py_INCREF(o);
      }
    else if (PyString_Check(o) && PyString_Size(o) == 1)
      {
      char *cp = PyString_AS_STRING(o);
      int j;
      for (j = 0; typecodes[j]; j++)
        {
        if (strcmp(cp, typecodes[j]) == 0)
          {
          o = PyString_FromString(typenames[j]);
          break;
          }
        }
      if (!typecodes[j])
        {
        Py_INCREF(o);
        }
      }
    else
      {
      Py_INCREF(o);
      }

    PyTuple_SET_ITEM(t, i, o);
    }

  PyObject *newkey = t;
  if (!multi)
    {
    newkey = PyTuple_GET_ITEM(t, 0);
    }

  ob = ((PyVTKTemplate *)ob)->dict;
  PyObject *r = PyObject_GetItem(ob, newkey);

  Py_DECREF(t);

  return r;
}

//--------------------------------------------------------------------
static PyMappingMethods PyVTKTemplate_AsMapping = {
  PyVTKTemplate_Size,                   // mp_length
  PyVTKTemplate_GetItem,                // mp_subscript
  0,                                    // mp_ass_subscript
};

//--------------------------------------------------------------------
#if PY_MAJOR_VERSION >= 2
static int PyVTKTemplate_Traverse(PyObject *o, visitproc visit, void *arg)
{
  PyVTKTemplate *self = (PyVTKTemplate *)o;
  PyObject *members[2];
  int err = 0;
  int i;

  members[0] = self->dict;
  members[1] = self->doc;

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
static void PyVTKTemplate_Delete(PyObject *op)
{
#if PY_VERSION_HEX >= 0x02020000
  PyObject_GC_UnTrack(op);
#endif

  Py_DECREF(((PyVTKTemplate *)op)->dict);
  Py_DECREF(((PyVTKTemplate *)op)->doc);

#if PY_VERSION_HEX >= 0x02020000
  PyObject_GC_Del(op);
#elif PY_MAJOR_VERSION >= 2
  PyObject_Del(op);
#else
  PyMem_DEL(op);
#endif
}

//--------------------------------------------------------------------
static PyObject *PyVTKTemplate_Repr(PyObject *op)
{
  PyVTKTemplate *self = (PyVTKTemplate *)op;
  char buf[512];
  sprintf(buf,"<%.80s %.80s.%.80s>", op->ob_type->tp_name,
          self->module, self->name);

  return PyString_FromString(buf);
}

//--------------------------------------------------------------------
static PyObject *PyVTKTemplate_Call(PyObject *, PyObject *, PyObject *)
{
  PyErr_SetString(PyExc_TypeError,
                  "this is a template, provide template args in brackets "
                  "before the ().");

  return NULL;
}

//--------------------------------------------------------------------
static PyObject *PyVTKTemplate_GetAttr(PyObject *self, PyObject *attr)
{
  char text[256];
  char *name = PyString_AsString(attr);
  PyVTKTemplate *op = (PyVTKTemplate *)self;
  PyMethodDef *meth;

  if (name[0] == '_')
    {
    if (strcmp(name, "__name__") == 0)
      {
      return PyString_FromString(op->name);
      }
    if (strcmp(name, "__module__") == 0)
      {
      return PyString_FromString(op->module);
      }
    if (strcmp(name, "__doc__") == 0)
      {
      Py_INCREF(op->doc);
      return op->doc;
      }
    if (strcmp(name, "__bases__") == 0)
      {
      return Py_BuildValue((char *)"()");
      }
    }

  for (meth = PyVTKTemplate_Methods; meth && meth->ml_name; meth++)
    {
    if (strcmp(name, meth->ml_name) == 0)
      {
      return PyCFunction_New(meth, self);
      }
    }

  sprintf(text, "'%.80s' object has no attribute '%.80s'",
          self->ob_type->tp_name, name);
  PyErr_SetString(PyExc_AttributeError, text);
  return NULL;
}

//--------------------------------------------------------------------
PyTypeObject PyVTKTemplate_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  (char*)"vtk.template",                 // tp_name
  sizeof(PyVTKTemplate),                 // tp_basicsize
  0,                                     // tp_itemsize
  PyVTKTemplate_Delete,                  // tp_dealloc
  0,                                     // tp_print
  0,                                     // tp_getattr
  0,                                     // tp_setattr
  0,                                     // tp_compare
  PyVTKTemplate_Repr,                    // tp_repr
  0,                                     // tp_as_number
  0,                                     // tp_as_sequence
  &PyVTKTemplate_AsMapping,              // tp_as_mapping
  0,                                     // tp_hash
  PyVTKTemplate_Call,                    // tp_call
  0,                                     // tp_string
  PyVTKTemplate_GetAttr,                 // tp_getattro
  0,                                     // tp_setattro
  0,                                     // tp_as_buffer
#if PY_VERSION_HEX >= 0x02020000
  Py_TPFLAGS_CHECKTYPES | Py_TPFLAGS_HAVE_GC |
#endif
  Py_TPFLAGS_DEFAULT,                    // tp_flags
  (char*)PyVTKTemplate_Doc,              // tp_doc
  PyVTKTemplate_Traverse,                // tp_traverse
  0,                                     // tp_clear
  0,                                     // tp_richcompare
  0,                                     // tp_weaklistoffset
#if PY_VERSION_HEX >= 0x02020000
  0,                                     // tp_iter
  0,                                     // tp_iternext
  PyVTKTemplate_Methods,                 // tp_methods
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
#if PY_VERSION_HEX >= 0x02030000
  PyObject_Del,                          // tp_free
#else
  _PyObject_Del,                         // tp_free
#endif
  0,                                     // tp_is_gc
  0,                                     // tp_bases
  0,                                     // tp_mro
  0,                                     // tp_cache
  0,                                     // tp_subclasses
  0,                                     // tp_weaklist
#endif
  VTK_WRAP_PYTHON_SUPRESS_UNINITIALIZED
};

//--------------------------------------------------------------------
PyObject *PyVTKTemplate_New(const char *name, const char *modulename,
                            const char *docstring[])
{
  PyObject *doc = vtkPythonUtil::BuildDocString(docstring);
  PyObject *dict = PyDict_New();

#if PY_VERSION_HEX >= 0x02020000
  PyVTKTemplate *op = PyObject_GC_New(PyVTKTemplate, &PyVTKTemplate_Type);
#elif PY_MAJOR_VERSION >= 2
  PyVTKTemplate *op = PyObject_New(PyVTKTemplate, &PyVTKTemplate_Type);
#else
  PyVTKTemplate *op = PyObject_NEW(PyVTKTemplate, &PyVTKTemplate_Type);
#endif
  PyObject *self = (PyObject *)op;

  op->dict = dict;
  op->doc = doc;
  op->name = name;
  op->module = modulename;

#if PY_VERSION_HEX >= 0x02020000
  PyObject_GC_Track(self);
#endif

  return self;
}

//--------------------------------------------------------------------
int PyVTKTemplate_AddItem(PyObject *self, PyObject *val)
{
  const char *name = NULL;
  const char *cp;
  char *dp;
  const char *ptype;
  PyObject *keys[16];
  PyObject *key;
  int i, n;
  size_t j = 0;

  if (PyVTKClass_Check(val))
    {
    name = PyString_AsString(((PyVTKClass *)val)->vtk_name);
    }
  else if (PyType_Check(val))
    {
    name = ((PyTypeObject *)val)->tp_name;
    }
  else
    {
    PyErr_SetString(PyExc_TypeError, "value must be a class or type");
    return -1;
    }

  /* find the underscore that precedes the template args */
  cp = name;
  while (*cp != '_' && *cp != '\0') { cp++; }

  if (*cp++ != '_')
    {
    PyErr_SetString(PyExc_TypeError, "name has no underscore");
    return -1;
    }

  /* go through the mangled template arg list */
  if (*cp++ != 'I')
    {
    PyErr_SetString(PyExc_TypeError, "badly formed mangled name");
    return -1;
    }

  for (i = 0; *cp != 'E' && *cp != '\0' && i < 16; i++)
    {
    /* check for literal */
    if (*cp == 'L')
      {
      cp++;
      if (*cp != 'i' && *cp != 'j' && *cp != 'l' && *cp != 'm')
        {
        PyErr_SetString(PyExc_TypeError, "non-integer template arg constant.");
        return -1;
        }
      cp++;
      keys[i] = PyInt_FromLong(strtol(cp, NULL, 0));
      while (*cp != 'E' && *cp != '\0') { cp++; }
      }
    else
      {
      ptype = NULL;
      switch (*cp)
        {
        case 'b':
          ptype = "bool";
          break;
        case 'c':
          ptype = "char";
          break;
        case 'a':
          ptype = "int8";
          break;
        case 'h':
          ptype = "uint8";
          break;
        case 's':
          ptype = "int16";
          break;
        case 't':
          ptype = "uint16";
          break;
        case 'i':
          ptype = "int32";
          break;
        case 'j':
          ptype = "uint32";
          break;
        case 'l':
          ptype = "int"; /* python int is C long */
          break;
        case 'm':
          ptype = "uint";
          break;
        case 'x':
          ptype = "int64";
          break;
        case 'y':
          ptype = "uint64";
          break;
        case 'f':
          ptype = "float32";
          break;
        case 'd':
          ptype = "float64";
          break;
        }

      if (ptype)
        {
        j = strlen(ptype);
        cp++;
        }
      else if (*cp >= '1' && *cp <= '9')
        {
        j = strtol(cp, &dp, 10);
        cp = dp;
        for (size_t k = 0; k < j; k++)
          {
          if (*dp++ == '\0')
            {
            PyErr_SetString(PyExc_TypeError, "badly formed mangled name");
            return -1;
            }
          }
        if (j == 16 && strncmp(cp, "vtkUnicodeString", 16) == 0)
          {
          ptype = "unicode";
          j = 7;
          }
        else if (j == 12 && strncmp(cp, "vtkStdString", 12) == 0)
          {
          ptype = "str";
          j = 3;
          }
        else
          {
          ptype = cp;
          }
        cp = dp;
        }

      if (ptype == NULL)
        {
        PyErr_SetString(PyExc_TypeError, "unrecognized mangled type.");
        return -1;
        }
      keys[i] = PyString_FromStringAndSize((char *)ptype, (Py_ssize_t)j);
      }
    }

  n = i;
  if (n == 1)
    {
    key = keys[0];
    }
  else
    {
    key = PyTuple_New(n);
    for (i = 0; i < n; i++)
      {
      PyTuple_SET_ITEM(key, i, keys[i]);
      }
    }

  PyDict_SetItem(((PyVTKTemplate *)self)->dict, key, val);
  Py_DECREF(key);

  return 0;
}
