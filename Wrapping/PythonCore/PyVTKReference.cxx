// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/*-----------------------------------------------------------------------
  The PyVTKReference was created in Sep 2010 by David Gobbi.

  This class is a proxy for immutable python objects like int, float,
  and string.  It allows these objects to be passed to VTK methods that
  require a reference.
-----------------------------------------------------------------------*/

#include "PyVTKReference.h"
#include "vtkABINamespace.h"
#include "vtkPythonUtil.h"

// Silence warning like
// "dereferencing type-punned pointer will break strict-aliasing rules"
// it happens because this kind of expression: (long *)&ptr
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

//------------------------------------------------------------------------------
// The reference types are heap types created via PyType_FromSpec so that the
// wrapping core can be built against the CPython Limited API (stable ABI).
// They are created once by PyVTKReference_ClassNew() and then live for the
// lifetime of the interpreter.
PyTypeObject* PyVTKReference_Type = nullptr;
static PyTypeObject* PyVTKNumberReference_Type = nullptr;
static PyTypeObject* PyVTKStringReference_Type = nullptr;
static PyTypeObject* PyVTKTupleReference_Type = nullptr;

//------------------------------------------------------------------------------

static const char* PyVTKReference_Doc =
  "reference(value:int) -> reference\n"
  "reference(value:float) -> reference\n"
  "reference(value:str) -> reference\n"
  "reference(value:(int, ...)) -> reference\n"
  "\n"
  "A simple container that acts as a reference to its contents.\n\n"
  "This wrapper class is needed when a VTK method returns a value\n"
  "in an argument that has been passed by reference.  By calling\n"
  "\"m = vtk.reference(a)\" on a value, you can create a proxy to\n"
  "that value.  The value can be changed by calling \"m.set(b)\".\n";

//------------------------------------------------------------------------------
// helper method: get a number protocol slot of an object's type.
// PyType_GetSlot only supports static types (e.g. int) from Python 3.10.
static unaryfunc PyVTKReference_NumberSlot(PyObject* obj, int slot)
{
#if PY_VERSION_HEX >= 0x030A0000
  return (unaryfunc)PyType_GetSlot(Py_TYPE(obj), slot);
#else
  PyNumberMethods* nb = Py_TYPE(obj)->tp_as_number;
  return nb ? (slot == Py_nb_index ? nb->nb_index : nb->nb_float) : nullptr;
#endif
}

//------------------------------------------------------------------------------
// helper method: make sure than an object is usable
static PyObject* PyVTKReference_CompatibleObject(PyObject* self, PyObject* opn)
{
  if (PyVTKReference_Check(opn))
  {
    if (self == nullptr || Py_TYPE(opn) == Py_TYPE(self))
    {
      // correct type, so return it
      opn = ((PyVTKReference*)opn)->value;
      Py_INCREF(opn);
      return opn;
    }
    // get contents, do further compatibility checks
    opn = ((PyVTKReference*)opn)->value;
  }

  // check if it is a string
  if (self == nullptr || Py_TYPE(self) == PyVTKStringReference_Type)
  {
    if (PyUnicode_Check(opn) || PyBytes_Check(opn))
    {
      Py_INCREF(opn);
      return opn;
    }
  }

  // check if it is a tuple or list
  if (self == nullptr || Py_TYPE(self) == PyVTKTupleReference_Type)
  {
    if (PyTuple_Check(opn) || PyList_Check(opn))
    {
      Py_INCREF(opn);
      return opn;
    }
  }

  // check if it is a number
  if (self == nullptr || Py_TYPE(self) == PyVTKNumberReference_Type)
  {
    if (PyFloat_Check(opn) || PyLong_Check(opn))
    {
      Py_INCREF(opn);
      return opn;
    }

    // check if it has number protocol and suitable methods
    if (unaryfunc nb_index = PyVTKReference_NumberSlot(opn, Py_nb_index))
    {
      opn = nb_index(opn);
      if (opn == nullptr || !PyLong_Check(opn))
      {
        PyErr_SetString(PyExc_TypeError, "nb_index should return integer object");
        return nullptr;
      }
      return opn;
    }
    else if (unaryfunc nb_float = PyVTKReference_NumberSlot(opn, Py_nb_float))
    {
      opn = nb_float(opn);
      if (opn == nullptr || !PyFloat_Check(opn))
      {
        PyErr_SetString(PyExc_TypeError, "nb_float should return float object");
        return nullptr;
      }
      return opn;
    }
  }

  // set error message according to required type
  const char* errmsg = "bad type";
  if (self == nullptr)
  {
    errmsg = "a numeric, string, or tuple object is required";
  }
  else if (Py_TYPE(self) == PyVTKStringReference_Type)
  {
    errmsg = "a string object is required";
  }
  else if (Py_TYPE(self) == PyVTKTupleReference_Type)
  {
    errmsg = "a tuple object is required";
  }
  else if (Py_TYPE(self) == PyVTKNumberReference_Type)
  {
    errmsg = "a numeric object is required";
  }

  PyErr_SetString(PyExc_TypeError, errmsg);
  return nullptr;
}

//------------------------------------------------------------------------------
// methods from C

PyObject* PyVTKReference_GetValue(PyObject* self)
{
  if (PyVTKReference_Check(self))
  {
    return ((PyVTKReference*)self)->value;
  }
  else
  {
    PyErr_SetString(PyExc_TypeError, "a vtk.reference() object is required");
  }

  return nullptr;
}

int PyVTKReference_SetValue(PyObject* self, PyObject* val)
{
  if (PyVTKReference_Check(self))
  {
    PyObject** op = &((PyVTKReference*)self)->value;

    PyObject* result = PyVTKReference_CompatibleObject(self, val);
    Py_DECREF(val);
    if (result)
    {
      Py_DECREF(*op);
      *op = result;
      return 0;
    }
  }
  else
  {
    PyErr_SetString(PyExc_TypeError, "a vtk.reference() object is required");
  }

  return -1;
}

//------------------------------------------------------------------------------
// methods from python

static PyObject* PyVTKReference_Get(PyObject* self, PyObject* args)
{
  if (PyArg_ParseTuple(args, ":get"))
  {
    PyObject* ob = PyVTKReference_GetValue(self);
    Py_INCREF(ob);
    return ob;
  }

  return nullptr;
}

static PyObject* PyVTKReference_Set(PyObject* self, PyObject* args)
{
  PyObject* opn = nullptr;

  if (PyArg_ParseTuple(args, "O:set", &opn))
  {
    opn = PyVTKReference_CompatibleObject(self, opn);

    if (opn)
    {
      if (PyVTKReference_SetValue(self, opn) == 0)
      {
        Py_INCREF(Py_None);
        return Py_None;
      }
    }
  }

  return nullptr;
}

static PyObject* PyVTKReference_Trunc(PyObject* self, PyObject* args)
{
  if (PyArg_ParseTuple(args, ":__trunc__"))
  {
    PyObject* ob = PyVTKReference_GetValue(self);
    // Look up the bound method on the wrapped value.  Using attribute access
    // (rather than the private _PyType_Lookup) keeps this Limited-API safe.
    PyObject* meth = PyObject_GetAttrString(ob, "__trunc__");
    if (meth == nullptr)
    {
      PyErr_Clear();
      PyErr_Format(PyExc_TypeError, "type %.100s doesn't define __trunc__ method",
        vtkPythonUtil::GetTypeNameForObject(ob));
      return nullptr;
    }
    PyObject* result = PyObject_CallNoArgs(meth);
    Py_DECREF(meth);
    return result;
  }

  return nullptr;
}

static PyObject* PyVTKReference_Round(PyObject* self, PyObject* args)
{
  PyObject* opn = nullptr;

  if (PyArg_ParseTuple(args, "|O:__round__", &opn))
  {
    PyObject* ob = PyVTKReference_GetValue(self);
    PyObject* meth = PyObject_GetAttrString(ob, "__round__");
    if (meth == nullptr)
    {
      PyErr_Clear();
      PyErr_Format(PyExc_TypeError, "type %.100s doesn't define __round__ method",
        vtkPythonUtil::GetTypeNameForObject(ob));
      return nullptr;
    }
    PyObject* result;
    if (opn)
    {
      result = PyObject_CallFunctionObjArgs(meth, opn, nullptr);
    }
    else
    {
      result = PyObject_CallNoArgs(meth);
    }
    Py_DECREF(meth);
    return result;
  }

  return nullptr;
}

static PyMethodDef PyVTKReference_Methods[] = { { "get", PyVTKReference_Get, METH_VARARGS,
                                                  "get() -> object\n\nGet the stored value." },
  { "set", PyVTKReference_Set, METH_VARARGS, "set(value:object) -> None\n\nSet the stored value." },
  { "__trunc__", PyVTKReference_Trunc, METH_VARARGS,
    "__trunc__() -> int\n\nReturns the Integral closest to x between 0 and x." },
  { "__round__", PyVTKReference_Round, METH_VARARGS,
    "__round__() -> int\n\nReturns the Integral closest to x, rounding half toward even.\n" },
  { nullptr, nullptr, 0, nullptr } };

//------------------------------------------------------------------------------
// Macros used for defining protocol methods

#define REFOBJECT_INTFUNC(prot, op)                                                                \
  static int PyVTKReference_##op(PyObject* ob)                                                     \
  {                                                                                                \
    ob = ((PyVTKReference*)ob)->value;                                                             \
    return Py##prot##_##op(ob);                                                                    \
  }

#define REFOBJECT_INTFUNC2(prot, op)                                                               \
  static int PyVTKReference_##op(PyObject* ob, PyObject* o)                                        \
  {                                                                                                \
    ob = ((PyVTKReference*)ob)->value;                                                             \
    return Py##prot##_##op(ob, o);                                                                 \
  }

#define REFOBJECT_SIZEFUNC(prot, op)                                                               \
  static Py_ssize_t PyVTKReference_##op(PyObject* ob)                                              \
  {                                                                                                \
    ob = ((PyVTKReference*)ob)->value;                                                             \
    return Py##prot##_##op(ob);                                                                    \
  }

#define REFOBJECT_INDEXFUNC(prot, op)                                                              \
  static PyObject* PyVTKReference_##op(PyObject* ob, Py_ssize_t i)                                 \
  {                                                                                                \
    ob = ((PyVTKReference*)ob)->value;                                                             \
    return Py##prot##_##op(ob, i);                                                                 \
  }

#define REFOBJECT_INDEXSETFUNC(prot, op)                                                           \
  static int PyVTKReference_##op(PyObject* ob, Py_ssize_t i, PyObject* o)                          \
  {                                                                                                \
    ob = ((PyVTKReference*)ob)->value;                                                             \
    return Py##prot##_##op(ob, i, o);                                                              \
  }

#define REFOBJECT_SLICEFUNC(prot, op)                                                              \
  static PyObject* PyVTKReference_##op(PyObject* ob, Py_ssize_t i, Py_ssize_t j)                   \
  {                                                                                                \
    ob = ((PyVTKReference*)ob)->value;                                                             \
    return Py##prot##_##op(ob, i, j);                                                              \
  }

#define REFOBJECT_SLICESETFUNC(prot, op)                                                           \
  static int PyVTKReference_##op(PyObject* ob, Py_ssize_t i, Py_ssize_t j, PyObject* o)            \
  {                                                                                                \
    ob = ((PyVTKReference*)ob)->value;                                                             \
    return Py##prot##_##op(ob, i, j, o);                                                           \
  }

#define REFOBJECT_UNARYFUNC(prot, op)                                                              \
  static PyObject* PyVTKReference_##op(PyObject* ob)                                               \
  {                                                                                                \
    ob = ((PyVTKReference*)ob)->value;                                                             \
    return Py##prot##_##op(ob);                                                                    \
  }

#define REFOBJECT_BINARYFUNC(prot, op)                                                             \
  static PyObject* PyVTKReference_##op(PyObject* ob1, PyObject* ob2)                               \
  {                                                                                                \
    if (PyVTKReference_Check(ob1))                                                                 \
    {                                                                                              \
      ob1 = ((PyVTKReference*)ob1)->value;                                                         \
    }                                                                                              \
    if (PyVTKReference_Check(ob2))                                                                 \
    {                                                                                              \
      ob2 = ((PyVTKReference*)ob2)->value;                                                         \
    }                                                                                              \
    return Py##prot##_##op(ob1, ob2);                                                              \
  }

#define REFOBJECT_INPLACEFUNC(prot, op)                                                            \
  static PyObject* PyVTKReference_InPlace##op(PyObject* ob1, PyObject* ob2)                        \
  {                                                                                                \
    PyVTKReference* ob = (PyVTKReference*)ob1;                                                     \
    PyObject* obn;                                                                                 \
    ob1 = ob->value;                                                                               \
    if (PyVTKReference_Check(ob2))                                                                 \
    {                                                                                              \
      ob2 = ((PyVTKReference*)ob2)->value;                                                         \
    }                                                                                              \
    obn = Py##prot##_##op(ob1, ob2);                                                               \
    if (obn)                                                                                       \
    {                                                                                              \
      ob->value = obn;                                                                             \
      Py_DECREF(ob1);                                                                              \
      Py_INCREF((PyObject*)ob);                                                                    \
      return (PyObject*)ob;                                                                        \
    }                                                                                              \
    return nullptr;                                                                                \
  }

#define REFOBJECT_INPLACEIFUNC(prot, op)                                                           \
  static PyObject* PyVTKReference_InPlace##op(PyObject* ob1, Py_ssize_t i)                         \
  {                                                                                                \
    PyVTKReference* ob = (PyVTKReference*)ob1;                                                     \
    PyObject* obn;                                                                                 \
    ob1 = ob->value;                                                                               \
    obn = Py##prot##_##op(ob1, i);                                                                 \
    if (obn)                                                                                       \
    {                                                                                              \
      ob->value = obn;                                                                             \
      Py_DECREF(ob1);                                                                              \
      Py_INCREF((PyObject*)ob);                                                                    \
      return (PyObject*)ob;                                                                        \
    }                                                                                              \
    return 0;                                                                                      \
  }

#define REFOBJECT_TERNARYFUNC(prot, op)                                                            \
  static PyObject* PyVTKReference_##op(PyObject* ob1, PyObject* ob2, PyObject* ob3)                \
  {                                                                                                \
    if (PyVTKReference_Check(ob1))                                                                 \
    {                                                                                              \
      ob1 = ((PyVTKReference*)ob1)->value;                                                         \
    }                                                                                              \
    if (PyVTKReference_Check(ob2))                                                                 \
    {                                                                                              \
      ob2 = ((PyVTKReference*)ob2)->value;                                                         \
    }                                                                                              \
    if (PyVTKReference_Check(ob2))                                                                 \
    {                                                                                              \
      ob3 = ((PyVTKReference*)ob3)->value;                                                         \
    }                                                                                              \
    return Py##prot##_##op(ob1, ob2, ob3);                                                         \
  }

#define REFOBJECT_INPLACETFUNC(prot, op)                                                           \
  static PyObject* PyVTKReference_InPlace##op(PyObject* ob1, PyObject* ob2, PyObject* ob3)         \
  {                                                                                                \
    PyVTKReference* ob = (PyVTKReference*)ob1;                                                     \
    PyObject* obn;                                                                                 \
    ob1 = ob->value;                                                                               \
    if (PyVTKReference_Check(ob2))                                                                 \
    {                                                                                              \
      ob2 = ((PyVTKReference*)ob2)->value;                                                         \
    }                                                                                              \
    if (PyVTKReference_Check(ob3))                                                                 \
    {                                                                                              \
      ob3 = ((PyVTKReference*)ob3)->value;                                                         \
    }                                                                                              \
    obn = Py##prot##_##op(ob1, ob2, ob3);                                                          \
    if (obn)                                                                                       \
    {                                                                                              \
      ob->value = obn;                                                                             \
      Py_DECREF(ob1);                                                                              \
      Py_INCREF((PyObject*)ob);                                                                    \
      return (PyObject*)ob;                                                                        \
    }                                                                                              \
    return nullptr;                                                                                \
  }

//------------------------------------------------------------------------------
// Number protocol

static int PyVTKReference_NonZero(PyObject* ob)
{
  ob = ((PyVTKReference*)ob)->value;
  return PyObject_IsTrue(ob);
}

REFOBJECT_BINARYFUNC(Number, Add)
REFOBJECT_BINARYFUNC(Number, Subtract)
REFOBJECT_BINARYFUNC(Number, Multiply)
REFOBJECT_BINARYFUNC(Number, Remainder)
REFOBJECT_BINARYFUNC(Number, Divmod)
REFOBJECT_TERNARYFUNC(Number, Power)
REFOBJECT_UNARYFUNC(Number, Negative)
REFOBJECT_UNARYFUNC(Number, Positive)
REFOBJECT_UNARYFUNC(Number, Absolute)
// NonZero
REFOBJECT_UNARYFUNC(Number, Invert)
REFOBJECT_BINARYFUNC(Number, Lshift)
REFOBJECT_BINARYFUNC(Number, Rshift)
REFOBJECT_BINARYFUNC(Number, And)
REFOBJECT_BINARYFUNC(Number, Or)
REFOBJECT_BINARYFUNC(Number, Xor)
// Coerce
REFOBJECT_UNARYFUNC(Number, Long)
REFOBJECT_UNARYFUNC(Number, Float)

REFOBJECT_INPLACEFUNC(Number, Add)
REFOBJECT_INPLACEFUNC(Number, Subtract)
REFOBJECT_INPLACEFUNC(Number, Multiply)
REFOBJECT_INPLACEFUNC(Number, Remainder)
REFOBJECT_INPLACETFUNC(Number, Power)
REFOBJECT_INPLACEFUNC(Number, Lshift)
REFOBJECT_INPLACEFUNC(Number, Rshift)
REFOBJECT_INPLACEFUNC(Number, And)
REFOBJECT_INPLACEFUNC(Number, Or)
REFOBJECT_INPLACEFUNC(Number, Xor)

REFOBJECT_BINARYFUNC(Number, FloorDivide)
REFOBJECT_BINARYFUNC(Number, TrueDivide)
REFOBJECT_INPLACEFUNC(Number, FloorDivide)
REFOBJECT_INPLACEFUNC(Number, TrueDivide)

REFOBJECT_UNARYFUNC(Number, Index)

//------------------------------------------------------------------------------
// Sequence protocol

REFOBJECT_SIZEFUNC(Sequence, Size)
REFOBJECT_BINARYFUNC(Sequence, Concat)
REFOBJECT_INDEXFUNC(Sequence, Repeat)
REFOBJECT_INDEXFUNC(Sequence, GetItem)
REFOBJECT_INTFUNC2(Sequence, Contains)

//------------------------------------------------------------------------------
// Mapping protocol

static PyObject* PyVTKReference_GetMapItem(PyObject* ob, PyObject* key)
{
  ob = ((PyVTKReference*)ob)->value;
  return PyObject_GetItem(ob, key);
}

//------------------------------------------------------------------------------
// Buffer protocol

// new buffer protocol
static int PyVTKReference_GetBuffer(PyObject* self, Py_buffer* view, int flags)
{
  PyObject* obj = ((PyVTKReference*)self)->value;
  return PyObject_GetBuffer(obj, view, flags);
}

static void PyVTKReference_ReleaseBuffer(PyObject*, Py_buffer* view)
{
  PyBuffer_Release(view);
}

//------------------------------------------------------------------------------
// Object protocol

static void PyVTKReference_Delete(PyObject* ob)
{
  // Instances of a heap type hold a reference to their type (taken by
  // _PyObject_New); release it after freeing the instance.
  PyTypeObject* tp = Py_TYPE(ob);
  Py_DECREF(((PyVTKReference*)ob)->value);
  PyObject_Del(ob);
  Py_DECREF((PyObject*)tp);
}

static PyObject* PyVTKReference_Repr(PyObject* ob)
{
  PyObject* r = nullptr;
  const char* name = vtkPythonUtil::GetTypeNameForObject(ob);
  PyObject* s = PyObject_Repr(((PyVTKReference*)ob)->value);
  if (s)
  {
    r = PyUnicode_FromFormat("%s(%U)", name, s);
    Py_DECREF(s);
  }
  return r;
}

static PyObject* PyVTKReference_Str(PyObject* ob)
{
  return PyObject_Str(((PyVTKReference*)ob)->value);
}

static PyObject* PyVTKReference_RichCompare(PyObject* ob1, PyObject* ob2, int opid)
{
  if (PyVTKReference_Check(ob1))
  {
    ob1 = ((PyVTKReference*)ob1)->value;
  }
  if (PyVTKReference_Check(ob2))
  {
    ob2 = ((PyVTKReference*)ob2)->value;
  }
  return PyObject_RichCompare(ob1, ob2, opid);
}

static PyObject* PyVTKReference_GetIter(PyObject* ob)
{
  return PyObject_GetIter(((PyVTKReference*)ob)->value);
}

static PyObject* PyVTKReference_GetAttr(PyObject* self, PyObject* attr)
{
  PyObject* a = PyObject_GenericGetAttr(self, attr);
  if (a || !PyErr_ExceptionMatches(PyExc_AttributeError))
  {
    return a;
  }
  PyErr_Clear();

  int firstchar = '\0';
  if (PyUnicode_GetLength(attr) > 0)
  {
    firstchar = PyUnicode_ReadChar(attr, 0);
  }
  if (firstchar != '_')
  {
    a = PyObject_GetAttr(((PyVTKReference*)self)->value, attr);

    if (a || !PyErr_ExceptionMatches(PyExc_AttributeError))
    {
      return a;
    }
    PyErr_Clear();
  }

  PyErr_Format(PyExc_AttributeError, "'%.50s' object has no attribute '%U'",
    vtkPythonUtil::GetTypeNameForObject(self), attr);
  return nullptr;
}

static PyObject* PyVTKReference_New(PyTypeObject*, PyObject* args, PyObject* kwds)
{
  PyObject* o = nullptr;

  if (kwds && PyDict_Size(kwds))
  {
    PyErr_SetString(PyExc_TypeError, "reference() does not take keyword arguments");
    return nullptr;
  }

  if (PyArg_ParseTuple(args, "O:reference", &o))
  {
    o = PyVTKReference_CompatibleObject(nullptr, o);

    if (o)
    {
      // _PyObject_New (via PyObject_New) takes the reference to the heap type
      // that the matching PyVTKReference_Delete releases.
      PyVTKReference* self;
      if (PyUnicode_Check(o) || PyBytes_Check(o))
      {
        self = PyObject_New(PyVTKReference, PyVTKStringReference_Type);
      }
      else if (PyTuple_Check(o) || PyList_Check(o))
      {
        self = PyObject_New(PyVTKReference, PyVTKTupleReference_Type);
      }
      else
      {
        self = PyObject_New(PyVTKReference, PyVTKNumberReference_Type);
      }
      self->value = o;
      return (PyObject*)self;
    }
  }

  return nullptr;
}

//------------------------------------------------------------------------------
// Type specifications
//
// The reference type and its three subtypes are PyType_Spec slot tables
// instantiated as heap types by PyType_FromSpec.

// Slots shared by the base reference type; subtypes inherit these.
static PyType_Slot PyVTKReference_Slots[] = {
  { Py_tp_dealloc, (void*)PyVTKReference_Delete },
  { Py_tp_repr, (void*)PyVTKReference_Repr },
  { Py_tp_hash, (void*)PyObject_HashNotImplemented },
  { Py_tp_str, (void*)PyVTKReference_Str },
  { Py_tp_getattro, (void*)PyVTKReference_GetAttr },
  { Py_tp_doc, (void*)PyVTKReference_Doc },
  { Py_tp_richcompare, (void*)PyVTKReference_RichCompare },
  { Py_tp_methods, (void*)PyVTKReference_Methods },
  { Py_tp_new, (void*)PyVTKReference_New },
  { 0, nullptr },
};

static PyType_Spec PyVTKReference_Spec = { "vtkmodules.vtkCommonCore.reference", // tp_name
  sizeof(PyVTKReference),                                                        // tp_basicsize
  0,                                                                             // tp_itemsize
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, PyVTKReference_Slots };

// The number reference adds the number protocol on top of the base.
static PyType_Slot PyVTKNumberReference_Slots[] = {
  { Py_nb_add, (void*)PyVTKReference_Add },
  { Py_nb_subtract, (void*)PyVTKReference_Subtract },
  { Py_nb_multiply, (void*)PyVTKReference_Multiply },
  { Py_nb_remainder, (void*)PyVTKReference_Remainder },
  { Py_nb_divmod, (void*)PyVTKReference_Divmod },
  { Py_nb_power, (void*)PyVTKReference_Power },
  { Py_nb_negative, (void*)PyVTKReference_Negative },
  { Py_nb_positive, (void*)PyVTKReference_Positive },
  { Py_nb_absolute, (void*)PyVTKReference_Absolute },
  { Py_nb_bool, (void*)PyVTKReference_NonZero },
  { Py_nb_invert, (void*)PyVTKReference_Invert },
  { Py_nb_lshift, (void*)PyVTKReference_Lshift },
  { Py_nb_rshift, (void*)PyVTKReference_Rshift },
  { Py_nb_and, (void*)PyVTKReference_And },
  { Py_nb_xor, (void*)PyVTKReference_Xor },
  { Py_nb_or, (void*)PyVTKReference_Or },
  { Py_nb_int, (void*)PyVTKReference_Long },
  { Py_nb_float, (void*)PyVTKReference_Float },
  { Py_nb_inplace_add, (void*)PyVTKReference_InPlaceAdd },
  { Py_nb_inplace_subtract, (void*)PyVTKReference_InPlaceSubtract },
  { Py_nb_inplace_multiply, (void*)PyVTKReference_InPlaceMultiply },
  { Py_nb_inplace_remainder, (void*)PyVTKReference_InPlaceRemainder },
  { Py_nb_inplace_power, (void*)PyVTKReference_InPlacePower },
  { Py_nb_inplace_lshift, (void*)PyVTKReference_InPlaceLshift },
  { Py_nb_inplace_rshift, (void*)PyVTKReference_InPlaceRshift },
  { Py_nb_inplace_and, (void*)PyVTKReference_InPlaceAnd },
  { Py_nb_inplace_xor, (void*)PyVTKReference_InPlaceXor },
  { Py_nb_inplace_or, (void*)PyVTKReference_InPlaceOr },
  { Py_nb_floor_divide, (void*)PyVTKReference_FloorDivide },
  { Py_nb_true_divide, (void*)PyVTKReference_TrueDivide },
  { Py_nb_inplace_floor_divide, (void*)PyVTKReference_InPlaceFloorDivide },
  { Py_nb_inplace_true_divide, (void*)PyVTKReference_InPlaceTrueDivide },
  { Py_nb_index, (void*)PyVTKReference_Index },
  { 0, nullptr },
};

static PyType_Spec PyVTKNumberReference_Spec = {
  "vtkmodules.vtkCommonCore.number_reference", // tp_name
  sizeof(PyVTKReference),                      // tp_basicsize
  0,                                           // tp_itemsize
  Py_TPFLAGS_DEFAULT, PyVTKNumberReference_Slots
};

// The string reference adds %-formatting, the sequence and mapping protocols,
// the buffer protocol, and iteration.
static PyType_Slot PyVTKStringReference_Slots[] = {
  { Py_nb_remainder, (void*)PyVTKReference_Remainder },
  { Py_sq_length, (void*)PyVTKReference_Size },
  { Py_sq_concat, (void*)PyVTKReference_Concat },
  { Py_sq_repeat, (void*)PyVTKReference_Repeat },
  { Py_sq_item, (void*)PyVTKReference_GetItem },
  { Py_sq_contains, (void*)PyVTKReference_Contains },
  { Py_mp_length, (void*)PyVTKReference_Size },
  { Py_mp_subscript, (void*)PyVTKReference_GetMapItem },
  { Py_bf_getbuffer, (void*)PyVTKReference_GetBuffer },
  { Py_bf_releasebuffer, (void*)PyVTKReference_ReleaseBuffer },
  { Py_tp_iter, (void*)PyVTKReference_GetIter },
  { 0, nullptr },
};

static PyType_Spec PyVTKStringReference_Spec = {
  "vtkmodules.vtkCommonCore.string_reference", // tp_name
  sizeof(PyVTKReference),                      // tp_basicsize
  0,                                           // tp_itemsize
  Py_TPFLAGS_DEFAULT, PyVTKStringReference_Slots
};

// The tuple reference adds the sequence and mapping protocols and iteration.
static PyType_Slot PyVTKTupleReference_Slots[] = {
  { Py_sq_length, (void*)PyVTKReference_Size },
  { Py_sq_concat, (void*)PyVTKReference_Concat },
  { Py_sq_repeat, (void*)PyVTKReference_Repeat },
  { Py_sq_item, (void*)PyVTKReference_GetItem },
  { Py_sq_contains, (void*)PyVTKReference_Contains },
  { Py_mp_length, (void*)PyVTKReference_Size },
  { Py_mp_subscript, (void*)PyVTKReference_GetMapItem },
  { Py_tp_iter, (void*)PyVTKReference_GetIter },
  { 0, nullptr },
};

static PyType_Spec PyVTKTupleReference_Spec = {
  "vtkmodules.vtkCommonCore.tuple_reference", // tp_name
  sizeof(PyVTKReference),                     // tp_basicsize
  0,                                          // tp_itemsize
  Py_TPFLAGS_DEFAULT, PyVTKTupleReference_Slots
};

//------------------------------------------------------------------------------
// Create a heap type from a spec, optionally deriving from a base type.
static PyTypeObject* PyVTKReference_CreateType(PyType_Spec* spec, PyObject* base)
{
  PyObject* bases = nullptr;
  if (base)
  {
    bases = PyTuple_Pack(1, base);
    if (!bases)
    {
      return nullptr;
    }
  }
  PyObject* type = PyType_FromSpecWithBases(spec, bases);
  Py_XDECREF(bases);
  return (PyTypeObject*)type;
}

//------------------------------------------------------------------------------
// Create the four reference type objects.  Safe to call more than once.
bool PyVTKReference_ClassNew()
{
  if (PyVTKReference_Type != nullptr)
  {
    return true;
  }

  PyVTKReference_Type = PyVTKReference_CreateType(&PyVTKReference_Spec, nullptr);
  if (PyVTKReference_Type == nullptr)
  {
    return false;
  }

  auto* base = (PyObject*)PyVTKReference_Type;
  PyVTKNumberReference_Type = PyVTKReference_CreateType(&PyVTKNumberReference_Spec, base);
  PyVTKStringReference_Type = PyVTKReference_CreateType(&PyVTKStringReference_Spec, base);
  PyVTKTupleReference_Type = PyVTKReference_CreateType(&PyVTKTupleReference_Spec, base);

  return PyVTKNumberReference_Type != nullptr && PyVTKStringReference_Type != nullptr &&
    PyVTKTupleReference_Type != nullptr;
}
