/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PyVTKMutableObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PyVTKMutableObject was created in Sep 2010 by David Gobbi.

  This class is a proxy for python int and float, it allows these objects
  to be passed to VTK methods that require a ref to a numeric type.
-----------------------------------------------------------------------*/

#include "PyVTKMutableObject.h"
#include "vtkPythonUtil.h"

#include <vtksys/ios/sstream>

// Silence warning like
// "dereferencing type-punned pointer will break strict-aliasing rules"
// it happens because this kind of expression: (long *)&ptr
// pragma GCC diagnostic is available since gcc>=4.2
#if defined(__GNUG__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=2)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

//--------------------------------------------------------------------
// methods for adding this type to a module
#if defined(WIN32)
extern "C" { __declspec( dllexport ) void PyVTKAddFile_mutable(PyObject *, const char *); }
#else
extern "C" { void PyVTKAddFile_mutable(PyObject *, const char *); }
#endif

//--------------------------------------------------------------------
// helper method: make sure than an object is usable
static PyObject *PyVTKMutableObject_CompatibleObject(PyObject *opn)
{
  PyNumberMethods *nb = opn->ob_type->tp_as_number;

  if (PyFloat_Check(opn) ||
      PyLong_Check(opn) ||
      PyInt_Check(opn))
    {
    Py_INCREF(opn);
    }
  else if (PyVTKMutableObject_Check(opn))
    {
    opn = ((PyVTKMutableObject *)opn)->mn_value;
    Py_INCREF(opn);
    }
#if PY_VERSION_HEX >= 0x02050000
  else if (nb && nb->nb_index)
    {
    opn = nb->nb_index(opn);
    if (opn == 0 || !PyLong_Check(opn) && !PyInt_Check(opn))
      {
      PyErr_SetString(PyExc_TypeError,
                      "nb_index should return integer object");
      return NULL;
      }
    }
#endif
  else if (nb && nb->nb_float)
    {
    opn = nb->nb_float(opn);
    if (opn == 0 || !PyFloat_Check(opn))
      {
      PyErr_SetString(PyExc_TypeError,
                      "nb_float should return float object");
      return NULL;
      }
    }
  else
    {
    PyErr_SetString(PyExc_TypeError,
                    "a numeric object is required");
    return NULL;
    }

  return opn;
}

//--------------------------------------------------------------------
// methods from C

PyObject *PyVTKMutableObject_GetValue(PyObject *self)
{
  if (PyVTKMutableObject_Check(self))
    {
    return ((PyVTKMutableObject *)self)->mn_value;
    }
  else
    {
    PyErr_SetString(PyExc_TypeError, "a vtk.mutable() object is required");
    }

  return NULL;
}

int PyVTKMutableObject_SetValue(PyObject *self, PyObject *val)
{
  if (PyVTKMutableObject_Check(self))
    {
    PyObject **op = &((PyVTKMutableObject *)self)->mn_value;

    if (PyFloat_Check(val) ||
        PyLong_Check(val) ||
        PyInt_Check(val))
      {
      Py_DECREF(*op);
      *op = val;
      return 0;
      }
    else
      {
      PyErr_SetString(PyExc_TypeError,
                      "a float, long, or int is required");
      }
    }
  else
    {
    PyErr_SetString(PyExc_TypeError, "a vtk.mutable() object is required");
    }

  return -1;
}

//--------------------------------------------------------------------
// methods from python

static PyObject *PyVTKMutableObject_Get(PyObject *self, PyObject *args)
{
  PyObject *ob = ((PyVTKMutableObject *)self)->mn_value;

  if (PyArg_ParseTuple(args, (char*)":get"))
    {
    Py_INCREF(ob);
    return ob;
    }

  return NULL;
}

static PyObject *PyVTKMutableObject_Set(PyObject *self, PyObject *args)
{
  PyObject **op = &((PyVTKMutableObject *)self)->mn_value;
  PyObject *opn;

  if (PyArg_ParseTuple(args, (char*)"O:set", &opn))
    {
    opn = PyVTKMutableObject_CompatibleObject(opn);

    if (opn)
      {
      Py_DECREF(*op);
      *op = opn;

      Py_INCREF(Py_None);
      return Py_None;
      }
    }

  return NULL;
}

static PyMethodDef PyVTKMutableObject_Methods[] = {
  {(char*)"get", PyVTKMutableObject_Get, 1, (char *)"Get the stored value."},
  {(char*)"set", PyVTKMutableObject_Set, 1, (char *)"Set the stored value."},
  { NULL, NULL, 0, NULL }
};

//--------------------------------------------------------------------
// Number protocol

static int PyVTKMutableObject_NonZero(PyObject *ob)
{
  ob = ((PyVTKMutableObject *)ob)->mn_value;
  return PyObject_IsTrue(ob);
}

static int PyVTKMutableObject_Coerce(PyObject **ob1, PyObject **ob2)
{
  *ob1 = ((PyVTKMutableObject *)*ob1)->mn_value;
  if (PyVTKMutableObject_Check(*ob2))
    {
    *ob2 = ((PyVTKMutableObject *)*ob2)->mn_value;
    }
  return PyNumber_CoerceEx(ob1, ob2);
}

static PyObject *PyVTKMutableObject_Hex(PyObject *ob)
{
  ob = ((PyVTKMutableObject *)ob)->mn_value;
  if (PyInt_Check(ob) || PyLong_Check(ob))
    {
    return ob->ob_type->tp_as_number->nb_hex(ob);
    }
#if PY_VERSION_HEX >= 0x02050000
  else
    {
    ob = PyNumber_Index(ob);
    if (ob)
      {
      PyObject *r = ob->ob_type->tp_as_number->nb_hex(ob);
      Py_DECREF(ob);
      return r;
      }
    else
      {
      PyErr_Clear();
      }
    }
#endif

  PyErr_SetString(PyExc_TypeError,
                  "hex() argument can't be converted to hex");
  return NULL;
}

static PyObject *PyVTKMutableObject_Oct(PyObject *ob)
{
  ob = ((PyVTKMutableObject *)ob)->mn_value;
  if (PyInt_Check(ob) || PyLong_Check(ob))
    {
    return ob->ob_type->tp_as_number->nb_oct(ob);
    }
#if PY_VERSION_HEX >= 0x02050000
  else
    {
    ob = PyNumber_Index(ob);
    if (ob)
      {
      PyObject *r = ob->ob_type->tp_as_number->nb_oct(ob);
      Py_DECREF(ob);
      return r;
      }
    else
      {
      PyErr_Clear();
      }
    }
#endif

  PyErr_SetString(PyExc_TypeError,
                  "oct() argument can't be converted to oct");
  return NULL;
}


#define REFOBJECT_UNARYFUNC(op) \
static PyObject *PyVTKMutableObject_##op(PyObject *ob) \
{ \
  ob = ((PyVTKMutableObject *)ob)->mn_value; \
  return PyNumber_##op(ob); \
}

#define REFOBJECT_BINARYFUNC(op) \
static PyObject *PyVTKMutableObject_##op(PyObject *ob1, PyObject *ob2) \
{ \
  if (PyVTKMutableObject_Check(ob1)) \
    { \
    ob1 = ((PyVTKMutableObject *)ob1)->mn_value; \
    } \
  if (PyVTKMutableObject_Check(ob2)) \
    { \
    ob2 = ((PyVTKMutableObject *)ob2)->mn_value; \
    } \
  return PyNumber_##op(ob1, ob2); \
}

#define REFOBJECT_INPLACEFUNC(op) \
static PyObject *PyVTKMutableObject_InPlace##op(PyObject *ob1, PyObject *ob2) \
{ \
  PyVTKMutableObject *ob = (PyVTKMutableObject *)ob1; \
  ob1 = ob->mn_value; \
  if (PyVTKMutableObject_Check(ob2)) \
    { \
    ob2 = ((PyVTKMutableObject *)ob2)->mn_value; \
    } \
  ob->mn_value = PyNumber_##op(ob1, ob2); \
  Py_DECREF(ob1); \
  Py_INCREF(ob); \
  return (PyObject *)ob; \
}

#define REFOBJECT_TERNARYFUNC(op) \
static PyObject *PyVTKMutableObject_##op(PyObject *ob1, PyObject *ob2, PyObject *ob3) \
{ \
  if (PyVTKMutableObject_Check(ob1)) \
    { \
    ob1 = ((PyVTKMutableObject *)ob1)->mn_value; \
    } \
  if (PyVTKMutableObject_Check(ob2)) \
    { \
    ob2 = ((PyVTKMutableObject *)ob2)->mn_value; \
    } \
  if (PyVTKMutableObject_Check(ob2)) \
    { \
    ob3 = ((PyVTKMutableObject *)ob3)->mn_value; \
    } \
  return PyNumber_##op(ob1, ob2, ob3); \
}

#define REFOBJECT_INPLACETFUNC(op) \
static PyObject *PyVTKMutableObject_InPlace##op(PyObject *ob1, PyObject *ob2, PyObject *ob3) \
{ \
  PyVTKMutableObject *ob = (PyVTKMutableObject *)ob1; \
  ob1 = ob->mn_value; \
  if (PyVTKMutableObject_Check(ob2)) \
    { \
    ob2 = ((PyVTKMutableObject *)ob2)->mn_value; \
    } \
  if (PyVTKMutableObject_Check(ob3)) \
    { \
    ob3 = ((PyVTKMutableObject *)ob3)->mn_value; \
    } \
  ob->mn_value = PyNumber_##op(ob1, ob2, ob3); \
  Py_DECREF(ob1); \
  Py_INCREF(ob); \
  return (PyObject *)ob; \
}


REFOBJECT_BINARYFUNC(Add)
REFOBJECT_BINARYFUNC(Subtract)
REFOBJECT_BINARYFUNC(Multiply)
REFOBJECT_BINARYFUNC(Divide)
REFOBJECT_BINARYFUNC(Remainder)
REFOBJECT_BINARYFUNC(Divmod)
REFOBJECT_TERNARYFUNC(Power)
REFOBJECT_UNARYFUNC(Negative)
REFOBJECT_UNARYFUNC(Positive)
REFOBJECT_UNARYFUNC(Absolute)
// NonZero
REFOBJECT_UNARYFUNC(Invert)
REFOBJECT_BINARYFUNC(Lshift)
REFOBJECT_BINARYFUNC(Rshift)
REFOBJECT_BINARYFUNC(And)
REFOBJECT_BINARYFUNC(Or)
REFOBJECT_BINARYFUNC(Xor)
// Coerce
REFOBJECT_UNARYFUNC(Int)
REFOBJECT_UNARYFUNC(Long)
REFOBJECT_UNARYFUNC(Float)
// Hex
// Oct

#if PY_VERSION_HEX >= 0x02000000
REFOBJECT_INPLACEFUNC(Add)
REFOBJECT_INPLACEFUNC(Subtract)
REFOBJECT_INPLACEFUNC(Multiply)
REFOBJECT_INPLACEFUNC(Divide)
REFOBJECT_INPLACEFUNC(Remainder)
REFOBJECT_INPLACETFUNC(Power)
REFOBJECT_INPLACEFUNC(Lshift)
REFOBJECT_INPLACEFUNC(Rshift)
REFOBJECT_INPLACEFUNC(And)
REFOBJECT_INPLACEFUNC(Or)
REFOBJECT_INPLACEFUNC(Xor)
#endif

#if PY_VERSION_HEX >= 0x02020000
REFOBJECT_BINARYFUNC(FloorDivide)
REFOBJECT_BINARYFUNC(TrueDivide)
REFOBJECT_INPLACEFUNC(FloorDivide)
REFOBJECT_INPLACEFUNC(TrueDivide)
#endif

#if PY_VERSION_HEX >= 0x02050000
REFOBJECT_UNARYFUNC(Index)
#endif

//--------------------------------------------------------------------
static PyNumberMethods PyVTKMutableObject_AsNumber = {
  PyVTKMutableObject_Add,                    // nb_add
  PyVTKMutableObject_Subtract,               // nb_subtract
  PyVTKMutableObject_Multiply,               // nb_multiply
  PyVTKMutableObject_Divide,                 // nb_divide
  PyVTKMutableObject_Remainder,              // nb_remainder
  PyVTKMutableObject_Divmod,                 // nb_divmod
  PyVTKMutableObject_Power,                  // nb_power
  PyVTKMutableObject_Negative,               // nb_negative
  PyVTKMutableObject_Positive,               // nb_positive
  PyVTKMutableObject_Absolute,               // nb_absolute
  PyVTKMutableObject_NonZero,                // nb_nonzero
  PyVTKMutableObject_Invert,                 // nb_invert
  PyVTKMutableObject_Lshift,                 // nb_lshift
  PyVTKMutableObject_Rshift,                 // nb_rshift
  PyVTKMutableObject_And,                    // nb_and
  PyVTKMutableObject_Xor,                    // nb_xor
  PyVTKMutableObject_Or,                     // nb_or
  PyVTKMutableObject_Coerce,                 // nb_coerce
  PyVTKMutableObject_Int,                    // nb_int
  PyVTKMutableObject_Long,                   // nb_long
  PyVTKMutableObject_Float,                  // nb_float
  PyVTKMutableObject_Oct,                    // nb_oct
  PyVTKMutableObject_Hex,                    // nb_hex
#if PY_VERSION_HEX >= 0x02000000
  PyVTKMutableObject_InPlaceAdd,             // nb_inplace_add
  PyVTKMutableObject_InPlaceSubtract,        // nb_inplace_subtract
  PyVTKMutableObject_InPlaceMultiply,        // nb_inplace_multiply
  PyVTKMutableObject_InPlaceDivide,          // nb_inplace_divide
  PyVTKMutableObject_InPlaceRemainder,       // nb_inplace_remainder
  PyVTKMutableObject_InPlacePower,           // nb_inplace_power
  PyVTKMutableObject_InPlaceLshift,          // nb_inplace_lshift
  PyVTKMutableObject_InPlaceRshift,          // nb_inplace_rshift
  PyVTKMutableObject_InPlaceAnd,             // nb_inplace_and
  PyVTKMutableObject_InPlaceXor,             // nb_inplace_xor
  PyVTKMutableObject_InPlaceOr,              // nb_inplace_or
#endif
#if PY_VERSION_HEX >= 0x02020000
  PyVTKMutableObject_FloorDivide,            // nb_floor_divide
  PyVTKMutableObject_TrueDivide,             // nb_true_divide
  PyVTKMutableObject_InPlaceFloorDivide,     // nb_inplace_floor_divide
  PyVTKMutableObject_InPlaceTrueDivide,      // nb_inplace_true_divide
#endif
#if PY_VERSION_HEX >= 0x02050000
  PyVTKMutableObject_Index,                  // nb_index
#endif
};

//--------------------------------------------------------------------
// Object protocol

static void PyVTKMutableObject_Delete(PyObject *ob)
{
  Py_DECREF(((PyVTKMutableObject *)ob)->mn_value);
#if PY_MAJOR_VERSION >= 2
  PyObject_Del(ob);
#else
  PyMem_DEL(ob);
#endif
}

static PyObject *PyVTKMutableObject_Repr(PyObject *ob)
{
  char textspace[128];
  PyObject *r = 0;
  const char *name = ob->ob_type->tp_name;
  PyObject *s = PyObject_Repr(((PyVTKMutableObject *)ob)->mn_value);
  if (s)
    {
    const char *text = PyString_AsString(s);
    size_t n = strlen(name) + strlen(text) + 3;
    char *cp = textspace;
    if (n > 128) { cp = (char *)malloc(n); }
    sprintf(cp, "%s(%s)", name, text);
    r = PyString_FromString(cp);
    if (n > 128) { free(cp); }
    Py_DECREF(s);
    }
  return r;
}

static PyObject *PyVTKMutableObject_Str(PyObject *ob)
{
  return PyObject_Str(((PyVTKMutableObject *)ob)->mn_value);
}

#if PY_VERSION_HEX >= 0x02010000
static PyObject *PyVTKMutableObject_RichCompare(
  PyObject *ob1, PyObject *ob2, int opid)
{
  if (PyVTKMutableObject_Check(ob1))
    {
    ob1 = ((PyVTKMutableObject *)ob1)->mn_value;
    }
  if (PyVTKMutableObject_Check(ob2))
    {
    ob2 = ((PyVTKMutableObject *)ob2)->mn_value;
    }
  return PyObject_RichCompare(ob1, ob2, opid);
}
#else
static int PyVTKMutableObject_Compare(PyObject *ob1, PyObject *ob2)
{
  if (PyVTKMutableObject_Check(ob1))
    {
    ob1 = ((PyVTKMutableObject *)ob1)->mn_value;
    }
  if (PyVTKMutableObject_Check(ob2))
    {
    ob2 = ((PyVTKMutableObject *)ob2)->mn_value;
    }
  return PyObject_Compare(ob1, ob2);
}
#endif

#if PY_VERSION_HEX < 0x02020000
static PyObject *PyVTKMutableObject_GetAttr(PyObject *self, PyObject *attr)
{
  PyVTKMutableObject *obj = (PyVTKMutableObject *)self;
  char *name = PyString_AsString(attr);
  PyMethodDef *meth;

  if (name[0] == '_')
    {
    if (strcmp(name, "__name__") == 0)
      {
      return PyString_FromString(self->ob_type->tp_name);
      }
    if (strcmp(name, "__doc__") == 0)
      {
      return PyString_FromString(self->ob_type->tp_doc);
      }
    if (strcmp(name,"__methods__") == 0)
      {
      meth = PyVTKMutableObject_Methods;
      PyObject *lst;
      int i, n;

      for (n = 0; meth && meth[n].ml_name; n++)
        {
        ;
        }

      if ((lst = PyList_New(n)) != NULL)
        {
        meth = obj->vtk_info->methods;
        for (i = 0; i < n; i++)
          {
          PyList_SetItem(lst, i, PyString_FromString(meth[i].ml_name));
          }
        PyList_Sort(lst);
        }
      return lst;
      }

    if (strcmp(name, "__members__") == 0)
      {
      PyObject *lst;
      if ((lst = PyList_New(4)) != NULL)
        {
        PyList_SetItem(lst, 0, PyString_FromString("__doc__"));
        PyList_SetItem(lst, 1, PyString_FromString("__members__"));
        PyList_SetItem(lst, 2, PyString_FromString("__methods__"));
        PyList_SetItem(lst, 3, PyString_FromString("__name__"));
        }
      return lst;
      }
    }

  for (meth = obj->vtk_info->methods; meth && meth->ml_name; meth++)
    {
    if (strcmp(name, meth->ml_name) == 0)
      {
      return PyCFunction_New(meth, self);
      }
    }

  PyErr_SetString(PyExc_AttributeError, name);
  return NULL;
}
#endif

static PyObject *PyVTKMutableObject_New(
  PyTypeObject *, PyObject *args, PyObject *kwds)
{
  PyObject *o;

  if (kwds && PyDict_Size(kwds))
    {
    PyErr_SetString(PyExc_TypeError,
                    "mutable() does not take keyword arguments");
    return NULL;
    }

  if (PyArg_ParseTuple(args, "O:mutable", &o))
    {
    o = PyVTKMutableObject_CompatibleObject(o);

    if (o)
      {
#if PY_MAJOR_VERSION >= 2
      PyVTKMutableObject *self = PyObject_New(PyVTKMutableObject, &PyVTKMutableObject_Type);
#else
      PyVTKMutableObject *self = PyObject_NEW(PyVTKMutableObject, &PyVTKMutableObject_Type);
#endif
      self->mn_value = o;
      return (PyObject *)self;
      }
    }

  return NULL;
}

//--------------------------------------------------------------------
PyTypeObject PyVTKMutableObject_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  (char*)"mutable",                      // tp_name
  sizeof(PyVTKMutableObject),            // tp_basicsize
  0,                                     // tp_itemsize
  PyVTKMutableObject_Delete,             // tp_dealloc
  0,                                     // tp_print
  0,                                     // tp_getattr
  0,                                     // tp_setattr
#if PY_VERSION_HEX >= 0x02010000
  0,                                     // tp_compare
#else
  PyVTKMutableObject_Compare,            // tp_compare
#endif
  PyVTKMutableObject_Repr,               // tp_repr
  &PyVTKMutableObject_AsNumber,          // tp_as_number
  0,                                     // tp_as_sequence
  0,                                     // tp_as_mapping
  0,                                     // tp_hash
  0,                                     // tp_call
  PyVTKMutableObject_Str,                // tp_string
#if PY_VERSION_HEX >= 0x02020000
  PyObject_GenericGetAttr,               // tp_getattro
#else
  PyVTKMutableObject_GetAttr,            // tp_getattro
#endif
  0,                                     // tp_setattro
  0,                                     // tp_as_buffer
#if PY_VERSION_HEX >= 0x02020000
  Py_TPFLAGS_CHECKTYPES |
#endif
  Py_TPFLAGS_DEFAULT,                    // tp_flags
  (char*)"A mutable numeric object.",    // tp_doc
  0,                                     // tp_traverse
  0,                                     // tp_clear
#if PY_VERSION_HEX >= 0x02010000
  PyVTKMutableObject_RichCompare,        // tp_richcompare
#else
  0,                                     // tp_richcompare
#endif
  0,                                     // tp_weaklistoffset
#if PY_VERSION_HEX >= 0x02020000
  0,                                     // tp_iter
  0,                                     // tp_iternext
  PyVTKMutableObject_Methods,            // tp_methods
  0,                                     // tp_members
  0,                                     // tp_getset
  0,                                     // tp_base
  0,                                     // tp_dict
  0,                                     // tp_descr_get
  0,                                     // tp_descr_set
  0,                                     // tp_dictoffset
  0,                                     // tp_init
  0,                                     // tp_alloc
  PyVTKMutableObject_New,                // tp_new
  _PyObject_Del,                         // tp_free
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
// Classic new method

#if PY_VERSION_HEX < 0x02020000
static PyObject *PyVTKMutableObject_ClassicNew(PyObject *, PyObject *args)
{
  return PyVTKMutableObject_New(PyVTKMutableObject_Type, args, 0);
}

static PyMethodDef PyVTKMutableObject_NewMethod =
  {(char*)"mutable", PyVTKMutableObject_ClassicNew, 1,
   (char *)"A mutable numeric object."}
};
#endif

//--------------------------------------------------------------------
// Exported method for adding this type to a module's dict
void PyVTKAddFile_mutable(
  PyObject *dict, const char *)
{
#if PY_VERSION_HEX < 0x2020000
  PyObject *o = PyCFunction_New(&PyVTKMutableObject_NewMethod, Py_None);
#else
  PyObject *o = (PyObject *)&PyVTKMutableObject_Type;
#endif

  if (o && PyDict_SetItemString(dict, (char *)"mutable", o) != 0)
    {
    Py_DECREF(o);
    }
}
