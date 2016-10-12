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
  It is implemented as a subclass of PyModule.
-----------------------------------------------------------------------*/

#include "PyVTKTemplate.h"
#include "vtkPythonUtil.h"

#include <string>

// Silence warning like
// "dereferencing type-punned pointer will break strict-aliasing rules"
// it happens because this kind of expression: (long *)&ptr
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

//--------------------------------------------------------------------

static const char *PyVTKTemplate_Doc =
  "A container for instantiations of class and function templates.\n\n"
  "This is a dictionary for templates, provide the template args\n"
  "in square brackets to get the desired kind of class.\n";

//--------------------------------------------------------------------
// Methods to help with name mangling and unmangling
PyObject *PyVTKTemplate_KeyFromName(PyObject *self, PyObject *arg);
PyObject *PyVTKTemplate_NameFromKey(PyObject *self, PyObject *key);

//--------------------------------------------------------------------
// Methods for python

static PyObject *PyVTKTemplate_HasKey(PyObject *ob, PyObject *args)
{
  PyObject *key = NULL;
  if (PyArg_ParseTuple(args, "O:has_key", &key))
  {
    PyObject *rval = NULL;
    PyObject *name = PyVTKTemplate_NameFromKey(ob, key);
    if (name)
    {
      PyObject *dict = PyModule_GetDict(ob);
      rval = PyDict_GetItem(dict, name);
      Py_DECREF(name);
    }
    if (rval)
    {
      Py_DECREF(rval);
      Py_INCREF(Py_True);
      return Py_True;
    }
    else if (!PyErr_Occurred())
    {
      Py_INCREF(Py_False);
      return Py_False;
    }
  }
  return NULL;
}

static PyObject *PyVTKTemplate_Keys(PyObject *ob, PyObject *args)
{
  if (PyArg_ParseTuple(args, ":keys"))
  {
    PyObject *dict = PyModule_GetDict(ob);
    PyObject *l = PyList_New(0);
    Py_ssize_t pos = 0;
    PyObject *key, *value;
    while (PyDict_Next(dict, &pos, &key, &value))
    {
      key = PyVTKTemplate_KeyFromName(ob, key);
      if (key)
      {
        PyList_Append(l, key);
        Py_DECREF(key);
      }
    }
    return l;
  }
  return NULL;
}

static PyObject *PyVTKTemplate_Values(PyObject *ob, PyObject *args)
{
  if (PyArg_ParseTuple(args, ":values"))
  {
    PyObject *dict = PyModule_GetDict(ob);
    PyObject *l = PyList_New(0);
    Py_ssize_t pos = 0;
    PyObject *key, *value;
    while (PyDict_Next(dict, &pos, &key, &value))
    {
      key = PyVTKTemplate_KeyFromName(ob, key);
      if (key)
      {
        PyList_Append(l, value);
        Py_DECREF(key);
      }
    }
    return l;
  }
  return NULL;
}

static PyObject *PyVTKTemplate_Items(PyObject *ob, PyObject *args)
{
  if (PyArg_ParseTuple(args, ":items"))
  {
    PyObject *dict = PyModule_GetDict(ob);
    PyObject *l = PyList_New(0);
    Py_ssize_t pos = 0;
    PyObject *key, *value;
    while (PyDict_Next(dict, &pos, &key, &value))
    {
      key = PyVTKTemplate_KeyFromName(ob, key);
      if (key)
      {
        Py_INCREF(value);
        PyObject *t = PyTuple_New(2);
        PyTuple_SET_ITEM(t, 0, key);
        PyTuple_SET_ITEM(t, 1, value);
        PyList_Append(l, t);
        Py_DECREF(t);
      }
    }
    return l;
  }
  return NULL;
}

static PyObject *PyVTKTemplate_Get(PyObject *ob, PyObject *args)
{
  PyObject *key = NULL;
  PyObject *def = Py_None;
  if (PyArg_ParseTuple(args, "O|O:get", &key, &def))
  {
    PyObject *rval = NULL;
    PyObject *dict = PyModule_GetDict(ob);
    key = PyVTKTemplate_NameFromKey(ob, key);
    if (key)
    {
      rval = PyDict_GetItem(dict, key);
      Py_DECREF(key);
    }
    if (rval)
    {
      return rval;
    }
    else if (!PyErr_Occurred())
    {
      Py_INCREF(def);
      return def;
    }
  }
  return NULL;
}


static PyMethodDef PyVTKTemplate_Methods[] = {
  {"has_key", PyVTKTemplate_HasKey, METH_VARARGS,
   "T.has_key(args) -> True only the template args are allowed."},
  {"keys", PyVTKTemplate_Keys, METH_VARARGS,
   "T.keys() -> list of allowed template args."},
  {"values", PyVTKTemplate_Values, METH_VARARGS,
   "T.values() -> list of provided template instantiations."},
  {"items", PyVTKTemplate_Items, METH_VARARGS,
   "T.items() -> list of (args,types) pairs."},
  {"get", PyVTKTemplate_Get, METH_VARARGS,
   "T.get(args) -> get instantiated template type or None."},
  { NULL, NULL, 0, NULL }
};

//--------------------------------------------------------------------
// Mapping protocol

static Py_ssize_t
PyVTKTemplate_Size(PyObject *ob)
{
  Py_ssize_t l = 0;
  PyObject *dict = PyModule_GetDict(ob);
  Py_ssize_t pos = 0;
  PyObject *key, *value;
  while (PyDict_Next(dict, &pos, &key, &value))
  {
    key = PyVTKTemplate_KeyFromName(ob, key);
    if (key)
    {
      Py_DECREF(key);
      l++;
    }
  }
  return l;
}

static PyObject *
PyVTKTemplate_GetItem(PyObject *ob, PyObject *key)
{
  PyObject *r = NULL;
  PyObject *dict = PyModule_GetDict(ob);
  PyObject *name = PyVTKTemplate_NameFromKey(ob, key);
  if (name)
  {
    // see if the named class is present
    r = PyObject_GetItem(dict, name);
    Py_DECREF(name);
    if (r == 0)
    {
      // clear the error (it will be set below)
      PyErr_Clear();
    }
  }
  if (r == 0)
  {
    // set a key error
    PyObject *t = PyTuple_Pack(1, key);
    PyErr_SetObject(PyExc_KeyError, t);
    Py_DECREF(t);
  }

  return r;
}

//--------------------------------------------------------------------
static PyMappingMethods PyVTKTemplate_AsMapping = {
  PyVTKTemplate_Size,                   // mp_length
  PyVTKTemplate_GetItem,                // mp_subscript
  0,                                    // mp_ass_subscript
};

//--------------------------------------------------------------------
static PyObject *PyVTKTemplate_Repr(PyObject *self)
{
  return PyString_FromFormat("<template %s>", PyModule_GetName(self));
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
PyTypeObject PyVTKTemplate_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "vtkCommonCorePython.template",        // tp_name
  0,                                     // tp_basicsize
  0,                                     // tp_itemsize
  0,                                     // tp_dealloc
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
  PyObject_GenericGetAttr,               // tp_getattro
  0,                                     // tp_setattro
  0,                                     // tp_as_buffer
  Py_TPFLAGS_DEFAULT,                    // tp_flags
  PyVTKTemplate_Doc,                     // tp_doc
  0,                                     // tp_traverse
  0,                                     // tp_clear
  0,                                     // tp_richcompare
  0,                                     // tp_weaklistoffset
  0,                                     // tp_iter
  0,                                     // tp_iternext
  PyVTKTemplate_Methods,                 // tp_methods
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
  VTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED
};

//--------------------------------------------------------------------
// Generate mangled name from the given template args
PyObject *PyVTKTemplate_NameFromKey(PyObject *self, PyObject *key)
{
  // python type names
  static const char *typenames[] = {
    "bool", "char",
    "int8", "uint8", "int16", "uint16", "int32", "uint32",
    "int", "uint", "int64", "uint64",
    "float32", "float64", "float",
    "str", "unicode",
    NULL };

  // python type codes
  static const char typecodes[] = {
    '?', 'c',
    'b', 'B', 'h', 'H', 'i', 'I',
    'l', 'L', 'q', 'Q',
    'f', 'd', 'd',
    '\0', '\0',
    '\0' };

  // ia64 ABI type codes
  static const char typechars[] = {
    'b', 'c',
    'a', 'h', 's', 't', 'i', 'j',
    'l', 'm', 'x', 'y',
    'f', 'd', 'd',
    '\0', '\0',
    '\0' };

  // get name of the template (skip any namespaces)
  const char *tname = PyModule_GetName(self);
  for (const char *cp = tname; *cp != '\0'; cp++)
  {
    if (*cp == '.')
    {
      tname = cp + 1;
    }
  }

  // begin constructing the mangled name
  std::string name = tname;
  name.push_back('_');
  name.push_back('I');

  // mangle the key using ia64 ABI for template args
  Py_ssize_t nargs = 1;
  bool multi = false;

  if (PyTuple_Check(key))
  {
    nargs = PyTuple_GET_SIZE(key);
    multi = true;
  }

  PyObject *o = key;

  for (Py_ssize_t i = 0; i < nargs; i++)
  {
    o = key;
    if (multi)
    {
      o = PyTuple_GET_ITEM(key, i);
    }

    tname = NULL;
    if (PyType_Check(o))
    {
      // if type object, get the name of the type
      Py_INCREF(o);
      tname = ((PyTypeObject *)o)->tp_name;
      for (const char *cp = tname; *cp != '\0'; cp++)
      {
        if (*cp == '.')
        {
          tname = cp + 1;
        }
      }
    }
    else
    {
      // else convert into an ASCII string
      o = PyObject_Str(o);
      if (PyBytes_Check(o))
      {
        tname = PyBytes_AS_STRING(o);
      }
      else if (PyUnicode_Check(o))
      {
#if PY_VERSION_HEX >= 0x03030000
        tname = PyUnicode_AsUTF8(o);
#else
        PyObject *s = _PyUnicode_AsDefaultEncodedString(o, NULL);
        tname = PyBytes_AS_STRING(s);
#endif
      }
    }

    if ((*tname >= '0' && *tname <= '9') ||
        (*tname == '-' && tname[1] >= '0' && tname[1] <= '9'))
    {
      // integer literal
      name.push_back('L');
      // guess the type based on available template instantiations
      const char *trylist = (*tname == '-' ? "lxisa" : "lmxyijstah");
      int bestfit = (*tname == '-' ? 5 : 10);
      char typechar = 'l'; // C++ long is best fit for python int
      PyObject *dict = PyModule_GetDict(self);
      Py_ssize_t pos = 0;
      PyObject *okey, *value;
      // loop through all the wrapped template instances
      while (PyDict_Next(dict, &pos, &okey, &value))
      {
        if (PyType_Check(value))
        {
          const char *cname = ((PyTypeObject *)value)->tp_name;
          for (const char *cp = cname; *cp != '\0'; cp++)
          {
            if (*cp == '.')
            {
              cname = cp + 1;
            }
          }
          if (strncmp(cname, name.data(), name.length()) == 0)
          {
            // compare this template instance against the typecode
            char c = cname[name.length()];
            for (int k = 0; k < bestfit; k++)
            {
              if (c == trylist[k])
              {
                typechar = c;
                bestfit = k;
                break;
              }
            }
          }
        }
      }
      // push the char that identifies the literal type
      name.push_back(typechar);
      if (*tname == '-')
      {
        name.push_back('n');
        tname++;
      }
      while (*tname >= '0' && *tname <= '9')
      {
        name.push_back(*tname++);
      }
      name.push_back('E');
    }
    else
    {
      // check against known types
      size_t n = strlen(tname);
      char typechar = '\0';
      for (int j = 0; typenames[j]; j++)
      {
        if (strcmp(typenames[j], tname) == 0)
        {
          typechar = typechars[j];
          if (typechar == '\0')
          {
            if (n == 3 && strcmp(tname, "str") == 0)
            {
              tname = "vtkStdString";
              n = 12;
            }
            else if (n == 7 && strcmp(tname, "unicode") == 0)
            {
              tname = "vtkUnicodeString";
              n = 16;
            }
          }
          break;
        }
      }
      if (typechar == '\0' && n == 1)
      {
        for (int j = 0; typecodes[j]; j++)
        {
          if (tname[0] == typecodes[j])
          {
            typechar = typechars[j];
            break;
          }
        }
      }
      if (typechar == 'l' || typechar == 'm')
      {
        // special compatibility code for 'long' (python 'int') to allow
        // it to match either a 32-bit or a 64-bit integer
        const char *trylist = (typechar == 'l' ? "lxi" : "myj");
        int bestfit = 3;
        PyObject *dict = PyModule_GetDict(self);
        Py_ssize_t pos = 0;
        PyObject *okey, *value;
        // loop through all the wrapped template instances
        while (PyDict_Next(dict, &pos, &okey, &value))
        {
          if (PyType_Check(value))
          {
            const char *cname = ((PyTypeObject *)value)->tp_name;
            for (const char *cp = cname; *cp != '\0'; cp++)
            {
              if (*cp == '.')
              {
                cname = cp + 1;
              }
            }
            if (strncmp(cname, name.data(), name.length()) == 0)
            {
              // compare this template instance against the typecode
              char c = cname[name.length()];
              for (int k = 0; k < bestfit; k++)
              {
                if (c == trylist[k])
                {
                  typechar = c;
                  bestfit = k;
                  break;
                }
              }
            }
          }
        }
      }
      if (typechar)
      {
        // for fundamental types, directly use the character code
        name.push_back(typechar);
      }
      else if (n < 256)
      {
        // for all other types, write the type in full
        if (n >= 100)
        {
          name.push_back('0' + static_cast<char>(n/100));
        }
        if (n >= 10)
        {
          name.push_back('0' + static_cast<char>((n % 100)/10));
        }
        name.push_back('0' + static_cast<char>(n % 10));
        name += tname;
      }
    }
    // free the python arg
    Py_DECREF(o);
  }

  // close the list of template arguments
  name.push_back('E');

#ifdef VTK_PY3K
  return PyUnicode_FromStringAndSize(name.data(), name.length());
#else
  return PyBytes_FromStringAndSize(name.data(), name.length());
#endif
}

//--------------------------------------------------------------------
// Generate template args by unmangling the class name
PyObject *PyVTKTemplate_KeyFromName(PyObject *self, PyObject *o)
{
  // convert arg to a C string
  const char *name = NULL;
  if (PyBytes_Check(o))
  {
    name = PyBytes_AS_STRING(o);
  }
  else if (PyUnicode_Check(o))
  {
#if PY_VERSION_HEX >= 0x03030000
    name = PyUnicode_AsUTF8(o);
#else
    PyObject *s = _PyUnicode_AsDefaultEncodedString(o, NULL);
    name = PyBytes_AS_STRING(s);
#endif
  }

  if (!name)
  {
    // name must be a string
    return NULL;
  }

  // get name of the template (skip any namespaces)
  const char *tname = PyModule_GetName(self);
  for (const char *cp = tname; *cp != '\0'; cp++)
  {
    if (*cp == '.')
    {
      tname = cp + 1;
    }
  }

  // match against template name
  const char *cp = name;
  while (*tname != '\0' && *cp == *tname)
  {
    cp++;
    tname++;
  }

  if (*cp != '_' || *tname != '\0')
  {
    // name does not match template
    return NULL;
  }
  cp++;

  // go through the mangled template arg list
  if (*cp++ != 'I')
  {
    // badly formed mangled name
    return NULL;
  }

  PyObject *keys[16];
  int i;
  for (i = 0; *cp != 'E' && *cp != '\0' && i < 16; i++)
  {
    // check for literal
    if (*cp == 'L')
    {
      cp++;
      if (*cp != 'i' && *cp != 'j' && *cp != 'l' && *cp != 'm')
      {
        // non-integer template arg constant
        return NULL;
      }
      cp++;
      long sign = 1;
      if (*cp == 'n')
      {
        sign = -1;
        cp++;
      }
      keys[i] = PyInt_FromLong(sign*strtol(cp, NULL, 0));
      while (*cp != 'E' && *cp != '\0')
      {
        cp++;
      }
    }
    else
    {
      const char *ptype = NULL;
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
          ptype = "int"; // python int is C long
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

      size_t j = 0;
      if (ptype)
      {
        j = strlen(ptype);
        cp++;
      }
      else if (*cp >= '1' && *cp <= '9')
      {
        char *dp;
        j = strtol(cp, &dp, 10);
        cp = dp;
        for (size_t k = 0; k < j; k++)
        {
          if (*dp++ == '\0')
          {
            // badly formed mangled name
            return NULL;
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
        // unrecognized mangled type.
        return NULL;
      }
      keys[i] = PyString_FromStringAndSize(ptype, (Py_ssize_t)j);
    }
  }

  int n = i;
  PyObject *key;
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

  return key;
}

//--------------------------------------------------------------------
// C API

//--------------------------------------------------------------------
PyObject *PyVTKTemplate_New(const char *name, const char *docstring[])
{
  // make sure python has readied the type object
  PyType_Ready(&PyVTKTemplate_Type);
  // call the allocator provided by python for this type
  PyObject *self = PyVTKTemplate_Type.tp_alloc(&PyVTKTemplate_Type, 0);
  // call the superclass init function
  PyObject *args = PyTuple_New(2);
  PyTuple_SET_ITEM(args, 0, PyString_FromString(name));
  PyTuple_SET_ITEM(args, 1, vtkPythonUtil::BuildDocString(docstring));
  PyVTKTemplate_Type.tp_base->tp_init(self, args, 0);
  Py_DECREF(args);

  return self;
}

//--------------------------------------------------------------------
int PyVTKTemplate_AddItem(PyObject *self, PyObject *val)
{
  if (!PyType_Check(val))
  {
    PyErr_SetString(PyExc_TypeError, "value must be a class or type");
    return -1;
  }

  // get the name, but strip the namespace
  const char *name = ((PyTypeObject *)val)->tp_name;
  for (const char *cp = name; *cp != '\0'; cp++)
  {
    if (*cp == '.')
    {
      name = cp+1;
    }
  }
  PyObject *dict = PyModule_GetDict(self);
  PyDict_SetItemString(dict, name, val);

  return 0;
}
