/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PyVTKReference.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PyVTKReference was created in Sep 2010 by David Gobbi.

  This class is a proxy for immutable python objects like int, float,
  and string.  It allows these objects to be passed to VTK methods that
  require a reference.
-----------------------------------------------------------------------*/

#include "PyVTKReference.h"
#include "vtkPythonUtil.h"

// Silence warning like
// "dereferencing type-punned pointer will break strict-aliasing rules"
// it happens because this kind of expression: (long *)&ptr
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

//--------------------------------------------------------------------

static const char* PyVTKReference_Doc =
  "A simple container that acts as a reference to its contents.\n\n"
  "This wrapper class is needed when a VTK method returns a value\n"
  "in an argument that has been passed by reference.  By calling\n"
  "\"m = vtk.reference(a)\" on a value, you can create a proxy to\n"
  "that value.  The value can be changed by calling \"m.set(b)\".\n";

//--------------------------------------------------------------------
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
  if (self == nullptr || Py_TYPE(self) == &PyVTKStringReference_Type)
  {
    if (
#ifdef Py_USING_UNICODE
      PyUnicode_Check(opn) ||
#endif
      PyBytes_Check(opn))
    {
      Py_INCREF(opn);
      return opn;
    }
  }

  // check if it is a tuple or list
  if (self == nullptr || Py_TYPE(self) == &PyVTKTupleReference_Type)
  {
    if (PyTuple_Check(opn) || PyList_Check(opn))
    {
      Py_INCREF(opn);
      return opn;
    }
  }

  // check if it is a number
  if (self == nullptr || Py_TYPE(self) == &PyVTKNumberReference_Type)
  {
    if (PyFloat_Check(opn) ||
#ifndef VTK_PY3K
      PyInt_Check(opn) ||
#endif
      PyLong_Check(opn))
    {
      Py_INCREF(opn);
      return opn;
    }

    // check if it has number protocol and suitable methods
    PyNumberMethods* nb = Py_TYPE(opn)->tp_as_number;
    if (nb)
    {
      if (nb->nb_index)
      {
        opn = nb->nb_index(opn);
        if (opn == nullptr ||
          (!PyLong_Check(opn)
#ifndef VTK_PY3K
            && !PyInt_Check(opn)
#endif
              ))
        {
          PyErr_SetString(PyExc_TypeError, "nb_index should return integer object");
          return nullptr;
        }
        return opn;
      }
      else if (nb->nb_float)
      {
        opn = nb->nb_float(opn);
        if (opn == nullptr || !PyFloat_Check(opn))
        {
          PyErr_SetString(PyExc_TypeError, "nb_float should return float object");
          return nullptr;
        }
        return opn;
      }
    }
  }

  // set error message according to required type
  const char* errmsg = "bad type";
  if (self == nullptr)
  {
    errmsg = "a numeric, string, or tuple object is required";
  }
  else if (Py_TYPE(self) == &PyVTKStringReference_Type)
  {
    errmsg = "a string object is required";
  }
  else if (Py_TYPE(self) == &PyVTKTupleReference_Type)
  {
    errmsg = "a tuple object is required";
  }
  else if (Py_TYPE(self) == &PyVTKNumberReference_Type)
  {
    errmsg = "a numeric object is required";
  }

  PyErr_SetString(PyExc_TypeError, errmsg);
  return nullptr;
}

//--------------------------------------------------------------------
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

//--------------------------------------------------------------------
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
  PyObject* opn;

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

#ifdef VTK_PY3K
static PyObject* PyVTKReference_Trunc(PyObject* self, PyObject* args)
{
  PyObject* opn;

  if (PyArg_ParseTuple(args, ":__trunc__", &opn))
  {
    PyObject* attr = PyUnicode_InternFromString("__trunc__");
    PyObject* ob = PyVTKReference_GetValue(self);
    PyObject* meth = _PyType_Lookup(Py_TYPE(ob), attr);
    if (meth == nullptr)
    {
      PyErr_Format(
        PyExc_TypeError, "type %.100s doesn't define __trunc__ method", Py_TYPE(ob)->tp_name);
      return nullptr;
    }
#if PY_VERSION_HEX >= 0x03040000
    return PyObject_CallFunction(meth, "O", ob);
#else
    return PyObject_CallFunction(meth, const_cast<char*>("O"), ob);
#endif
  }

  return nullptr;
}

static PyObject* PyVTKReference_Round(PyObject* self, PyObject* args)
{
  PyObject* opn;

  if (PyArg_ParseTuple(args, "|O:__round__", &opn))
  {
    PyObject* attr = PyUnicode_InternFromString("__round__");
    PyObject* ob = PyVTKReference_GetValue(self);
    PyObject* meth = _PyType_Lookup(Py_TYPE(ob), attr);
    if (meth == nullptr)
    {
      PyErr_Format(
        PyExc_TypeError, "type %.100s doesn't define __round__ method", Py_TYPE(ob)->tp_name);
      return nullptr;
    }
#if PY_VERSION_HEX >= 0x03040000
    if (opn)
    {
      return PyObject_CallFunction(meth, "OO", ob, opn);
    }
    return PyObject_CallFunction(meth, "O", ob);
#else
    if (opn)
    {
      return PyObject_CallFunction(meth, const_cast<char*>("OO"), ob, opn);
    }
    return PyObject_CallFunction(meth, const_cast<char*>("O"), ob);
#endif
  }

  return nullptr;
}
#endif

static PyMethodDef PyVTKReference_Methods[] = { { "get", PyVTKReference_Get, METH_VARARGS,
                                                  "Get the stored value." },
  { "set", PyVTKReference_Set, METH_VARARGS, "Set the stored value." },
#ifdef VTK_PY3K
  { "__trunc__", PyVTKReference_Trunc, METH_VARARGS,
    "Returns the Integral closest to x between 0 and x." },
  { "__round__", PyVTKReference_Round, METH_VARARGS,
    "Returns the Integral closest to x, rounding half toward even.\n" },
#endif
  { nullptr, nullptr, 0, nullptr } };

//--------------------------------------------------------------------
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
      Py_INCREF(ob);                                                                               \
      return (PyObject*)ob;                                                                        \
    }                                                                                              \
    return 0;                                                                                      \
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
      Py_INCREF(ob);                                                                               \
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
      Py_INCREF(ob);                                                                               \
      return (PyObject*)ob;                                                                        \
    }                                                                                              \
    return 0;                                                                                      \
  }

//--------------------------------------------------------------------
// Number protocol

static int PyVTKReference_NonZero(PyObject* ob)
{
  ob = ((PyVTKReference*)ob)->value;
  return PyObject_IsTrue(ob);
}

#ifndef VTK_PY3K
static int PyVTKReference_Coerce(PyObject** ob1, PyObject** ob2)
{
  *ob1 = ((PyVTKReference*)*ob1)->value;
  if (PyVTKReference_Check(*ob2))
  {
    *ob2 = ((PyVTKReference*)*ob2)->value;
  }
  return PyNumber_CoerceEx(ob1, ob2);
}

static PyObject* PyVTKReference_Hex(PyObject* ob)
{
  ob = ((PyVTKReference*)ob)->value;
#if PY_VERSION_HEX >= 0x02060000
  return PyNumber_ToBase(ob, 16);
#else
  if (Py_TYPE(ob)->tp_as_number && Py_TYPE(ob)->tp_as_number->nb_hex)
  {
    return Py_TYPE(ob)->tp_as_number->nb_hex(ob);
  }

  PyErr_SetString(PyExc_TypeError, "hex() argument can't be converted to hex");
  return nullptr;
#endif
}

static PyObject* PyVTKReference_Oct(PyObject* ob)
{
  ob = ((PyVTKReference*)ob)->value;
#if PY_VERSION_HEX >= 0x02060000
  return PyNumber_ToBase(ob, 8);
#else
  if (Py_TYPE(ob)->tp_as_number && Py_TYPE(ob)->tp_as_number->nb_oct)
  {
    return Py_TYPE(ob)->tp_as_number->nb_oct(ob);
  }

  PyErr_SetString(PyExc_TypeError, "oct() argument can't be converted to oct");
  return nullptr;
#endif
}
#endif

REFOBJECT_BINARYFUNC(Number, Add)
REFOBJECT_BINARYFUNC(Number, Subtract)
REFOBJECT_BINARYFUNC(Number, Multiply)
#ifndef VTK_PY3K
REFOBJECT_BINARYFUNC(Number, Divide)
#endif
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
#ifndef VTK_PY3K
REFOBJECT_UNARYFUNC(Number, Int)
#endif
REFOBJECT_UNARYFUNC(Number, Long)
REFOBJECT_UNARYFUNC(Number, Float)

REFOBJECT_INPLACEFUNC(Number, Add)
REFOBJECT_INPLACEFUNC(Number, Subtract)
REFOBJECT_INPLACEFUNC(Number, Multiply)
#ifndef VTK_PY3K
REFOBJECT_INPLACEFUNC(Number, Divide)
#endif
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

//--------------------------------------------------------------------
static PyNumberMethods PyVTKReference_AsNumber = {
  PyVTKReference_Add,      // nb_add
  PyVTKReference_Subtract, // nb_subtract
  PyVTKReference_Multiply, // nb_multiply
#ifndef VTK_PY3K
  PyVTKReference_Divide, // nb_divide
#endif
  PyVTKReference_Remainder, // nb_remainder
  PyVTKReference_Divmod,    // nb_divmod
  PyVTKReference_Power,     // nb_power
  PyVTKReference_Negative,  // nb_negative
  PyVTKReference_Positive,  // nb_positive
  PyVTKReference_Absolute,  // nb_absolute
  PyVTKReference_NonZero,   // nb_nonzero
  PyVTKReference_Invert,    // nb_invert
  PyVTKReference_Lshift,    // nb_lshift
  PyVTKReference_Rshift,    // nb_rshift
  PyVTKReference_And,       // nb_and
  PyVTKReference_Xor,       // nb_xor
  PyVTKReference_Or,        // nb_or
#ifndef VTK_PY3K
  PyVTKReference_Coerce, // nb_coerce
  PyVTKReference_Int,    // nb_int
  PyVTKReference_Long,   // nb_long
#else
  PyVTKReference_Long, // nb_int
  nullptr,             // nb_reserved
#endif
  PyVTKReference_Float, // nb_float
#ifndef VTK_PY3K
  PyVTKReference_Oct, // nb_oct
  PyVTKReference_Hex, // nb_hex
#endif
  PyVTKReference_InPlaceAdd,      // nb_inplace_add
  PyVTKReference_InPlaceSubtract, // nb_inplace_subtract
  PyVTKReference_InPlaceMultiply, // nb_inplace_multiply
#ifndef VTK_PY3K
  PyVTKReference_InPlaceDivide, // nb_inplace_divide
#endif
  PyVTKReference_InPlaceRemainder,   // nb_inplace_remainder
  PyVTKReference_InPlacePower,       // nb_inplace_power
  PyVTKReference_InPlaceLshift,      // nb_inplace_lshift
  PyVTKReference_InPlaceRshift,      // nb_inplace_rshift
  PyVTKReference_InPlaceAnd,         // nb_inplace_and
  PyVTKReference_InPlaceXor,         // nb_inplace_xor
  PyVTKReference_InPlaceOr,          // nb_inplace_or
  PyVTKReference_FloorDivide,        // nb_floor_divide
  PyVTKReference_TrueDivide,         // nb_true_divide
  PyVTKReference_InPlaceFloorDivide, // nb_inplace_floor_divide
  PyVTKReference_InPlaceTrueDivide,  // nb_inplace_true_divide
  PyVTKReference_Index,              // nb_index
#if PY_VERSION_HEX >= 0x03050000
  nullptr, // nb_matrix_multiply
  nullptr, // nb_inplace_matrix_multiply
#endif
};

//--------------------------------------------------------------------
static PyNumberMethods PyVTKStringReference_AsNumber = {
  nullptr, // nb_add
  nullptr, // nb_subtract
  nullptr, // nb_multiply
#ifndef VTK_PY3K
  nullptr, // nb_divide
#endif
  PyVTKReference_Remainder, // nb_remainder
  nullptr,                  // nb_divmod
  nullptr,                  // nb_power
  nullptr,                  // nb_negative
  nullptr,                  // nb_positive
  nullptr,                  // nb_absolute
  nullptr,                  // nb_nonzero
  nullptr,                  // nb_invert
  nullptr,                  // nb_lshift
  nullptr,                  // nb_rshift
  nullptr,                  // nb_and
  nullptr,                  // nb_xor
  nullptr,                  // nb_or
#ifndef VTK_PY3K
  nullptr, // nb_coerce
  nullptr, // nb_int
  nullptr, // nb_long
#else
  nullptr,             // nb_int
  nullptr,             // nb_reserved
#endif
  nullptr, // nb_float
#ifndef VTK_PY3K
  nullptr, // nb_oct
  nullptr, // nb_hex
#endif
  nullptr, // nb_inplace_add
  nullptr, // nb_inplace_subtract
  nullptr, // nb_inplace_multiply
#ifndef VTK_PY3K
  nullptr, // nb_inplace_divide
#endif
  nullptr, // nb_inplace_remainder
  nullptr, // nb_inplace_power
  nullptr, // nb_inplace_lshift
  nullptr, // nb_inplace_rshift
  nullptr, // nb_inplace_and
  nullptr, // nb_inplace_xor
  nullptr, // nb_inplace_or
  nullptr, // nb_floor_divide
  nullptr, // nb_true_divide
  nullptr, // nb_inplace_floor_divide
  nullptr, // nb_inplace_true_divide
  nullptr, // nb_index
#if PY_VERSION_HEX >= 0x03050000
  nullptr, // nb_matrix_multiply
  nullptr, // nb_inplace_matrix_multiply
#endif
};

//--------------------------------------------------------------------
// Sequence protocol

REFOBJECT_SIZEFUNC(Sequence, Size)
REFOBJECT_BINARYFUNC(Sequence, Concat)
REFOBJECT_INDEXFUNC(Sequence, Repeat)
REFOBJECT_INDEXFUNC(Sequence, GetItem)
#ifndef VTK_PY3K
REFOBJECT_SLICEFUNC(Sequence, GetSlice)
#endif
REFOBJECT_INTFUNC2(Sequence, Contains)

//--------------------------------------------------------------------
static PySequenceMethods PyVTKReference_AsSequence = {
  PyVTKReference_Size,    // sq_length
  PyVTKReference_Concat,  // sq_concat
  PyVTKReference_Repeat,  // sq_repeat
  PyVTKReference_GetItem, // sq_item
#ifndef VTK_PY3K
  PyVTKReference_GetSlice, // sq_slice
#else
  nullptr,             // sq_slice
#endif
  nullptr,                 // sq_ass_item
  nullptr,                 // sq_ass_slice
  PyVTKReference_Contains, // sq_contains
  nullptr,                 // sq_inplace_concat
  nullptr,                 // sq_inplace_repeat
};

//--------------------------------------------------------------------
// Mapping protocol

static PyObject* PyVTKReference_GetMapItem(PyObject* ob, PyObject* key)
{
  ob = ((PyVTKReference*)ob)->value;
  return PyObject_GetItem(ob, key);
}

//--------------------------------------------------------------------
static PyMappingMethods PyVTKReference_AsMapping = {
  PyVTKReference_Size,       // mp_length
  PyVTKReference_GetMapItem, // mp_subscript
  nullptr,                   // mp_ass_subscript
};

//--------------------------------------------------------------------
// Buffer protocol

#ifndef VTK_PY3K
// old buffer protocol
static Py_ssize_t PyVTKReference_GetReadBuf(PyObject* op, Py_ssize_t segment, void** ptrptr)
{
  char text[80];
  PyBufferProcs* pb;
  op = ((PyVTKReference*)op)->value;
  pb = Py_TYPE(op)->tp_as_buffer;

  if (pb && pb->bf_getreadbuffer)
  {
    return Py_TYPE(op)->tp_as_buffer->bf_getreadbuffer(op, segment, ptrptr);
  }

  snprintf(text, sizeof(text), "type \'%.20s\' does not support readable buffer access",
    Py_TYPE(op)->tp_name);
  PyErr_SetString(PyExc_TypeError, text);

  return -1;
}

static Py_ssize_t PyVTKReference_GetWriteBuf(PyObject* op, Py_ssize_t segment, void** ptrptr)
{
  char text[80];
  PyBufferProcs* pb;
  op = ((PyVTKReference*)op)->value;
  pb = Py_TYPE(op)->tp_as_buffer;

  if (pb && pb->bf_getwritebuffer)
  {
    return Py_TYPE(op)->tp_as_buffer->bf_getwritebuffer(op, segment, ptrptr);
  }

  snprintf(text, sizeof(text), "type \'%.20s\' does not support writeable buffer access",
    Py_TYPE(op)->tp_name);
  PyErr_SetString(PyExc_TypeError, text);

  return -1;
}

static Py_ssize_t PyVTKReference_GetSegCount(PyObject* op, Py_ssize_t* lenp)
{
  char text[80];
  PyBufferProcs* pb;
  op = ((PyVTKReference*)op)->value;
  pb = Py_TYPE(op)->tp_as_buffer;

  if (pb && pb->bf_getsegcount)
  {
    return Py_TYPE(op)->tp_as_buffer->bf_getsegcount(op, lenp);
  }

  snprintf(
    text, sizeof(text), "type \'%.20s\' does not support buffer access", Py_TYPE(op)->tp_name);
  PyErr_SetString(PyExc_TypeError, text);

  return -1;
}

static Py_ssize_t PyVTKReference_GetCharBuf(PyObject* op, Py_ssize_t segment, char** ptrptr)
{
  char text[80];
  PyBufferProcs* pb;
  op = ((PyVTKReference*)op)->value;
  pb = Py_TYPE(op)->tp_as_buffer;

  if (pb && pb->bf_getcharbuffer)
  {
    return Py_TYPE(op)->tp_as_buffer->bf_getcharbuffer(op, segment, ptrptr);
  }

  snprintf(text, sizeof(text), "type \'%.20s\' does not support character buffer access",
    Py_TYPE(op)->tp_name);
  PyErr_SetString(PyExc_TypeError, text);

  return -1;
}
#endif

#if PY_VERSION_HEX >= 0x02060000
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
#endif

static PyBufferProcs PyVTKReference_AsBuffer = {
#ifndef VTK_PY3K
  PyVTKReference_GetReadBuf,  // bf_getreadbuffer
  PyVTKReference_GetWriteBuf, // bf_getwritebuffer
  PyVTKReference_GetSegCount, // bf_getsegcount
  PyVTKReference_GetCharBuf,  // bf_getcharbuffer
#endif
#if PY_VERSION_HEX >= 0x02060000
  PyVTKReference_GetBuffer,    // bf_getbuffer
  PyVTKReference_ReleaseBuffer // bf_releasebuffer
#endif
};

//--------------------------------------------------------------------
// Object protocol

static void PyVTKReference_Delete(PyObject* ob)
{
  Py_DECREF(((PyVTKReference*)ob)->value);
  PyObject_Del(ob);
}

static PyObject* PyVTKReference_Repr(PyObject* ob)
{
  PyObject* r = nullptr;
  const char* name = Py_TYPE(ob)->tp_name;
  PyObject* s = PyObject_Repr(((PyVTKReference*)ob)->value);
  if (s)
  {
#ifdef VTK_PY3K
    r = PyUnicode_FromFormat("%s(%U)", name, s);
#else
    const char* text = PyString_AsString(s);
    size_t n = strlen(name) + strlen(text) + 3; // for '(', ')', null
    if (n > 128)
    {
      char* cp = (char*)malloc(n);
      snprintf(cp, n, "%s(%s)", name, text);
      r = PyString_FromString(cp);
      free(cp);
    }
    else
    {
      char textspace[128];
      snprintf(textspace, sizeof(textspace), "%s(%s)", name, text);
      r = PyString_FromString(textspace);
    }
#endif
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

#ifndef VTK_PY3K
  char* name = PyString_AsString(attr);
  int firstchar = name[0];
#elif PY_VERSION_HEX >= 0x03030000
  int firstchar = '\0';
  if (PyUnicode_GetLength(attr) > 0)
  {
    firstchar = PyUnicode_ReadChar(attr, 0);
  }
#else
  int firstchar = '\0';
  if (PyUnicode_Check(attr) && PyUnicode_GetSize(attr) > 0)
  {
    firstchar = PyUnicode_AS_UNICODE(attr)[0];
  }
#endif
  if (firstchar != '_')
  {
    a = PyObject_GetAttr(((PyVTKReference*)self)->value, attr);

    if (a || !PyErr_ExceptionMatches(PyExc_AttributeError))
    {
      return a;
    }
    PyErr_Clear();
  }

#ifdef VTK_PY3K
  PyErr_Format(
    PyExc_AttributeError, "'%.50s' object has no attribute '%U'", Py_TYPE(self)->tp_name, attr);
#else
  PyErr_Format(
    PyExc_AttributeError, "'%.50s' object has no attribute '%.80s'", Py_TYPE(self)->tp_name, name);
#endif
  return nullptr;
}

static PyObject* PyVTKReference_New(PyTypeObject*, PyObject* args, PyObject* kwds)
{
  PyObject* o;

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
      PyVTKReference* self;
      if (
#ifdef Py_USING_UNICODE
        PyUnicode_Check(o) ||
#endif
        PyBytes_Check(o))
      {
        self = PyObject_New(PyVTKReference, &PyVTKStringReference_Type);
      }
      else if (PyTuple_Check(o) || PyList_Check(o))
      {
        self = PyObject_New(PyVTKReference, &PyVTKTupleReference_Type);
      }
      else
      {
        self = PyObject_New(PyVTKReference, &PyVTKNumberReference_Type);
      }
      self->value = o;
      return (PyObject*)self;
    }
  }

  return nullptr;
}

// clang-format off
//--------------------------------------------------------------------
PyTypeObject PyVTKReference_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "vtkmodules.vtkCommonCore.reference", // tp_name
  sizeof(PyVTKReference), // tp_basicsize
  0,                      // tp_itemsize
  PyVTKReference_Delete,  // tp_dealloc
#if PY_VERSION_HEX >= 0x03080000
  0,                      // tp_vectorcall_offset
#else
  nullptr,                // tp_print
#endif
  nullptr,                // tp_getattr
  nullptr,                // tp_setattr
  nullptr,                // tp_compare
  PyVTKReference_Repr,    // tp_repr
  nullptr,                // tp_as_number
  nullptr,                // tp_as_sequence
  nullptr,                // tp_as_mapping
#if PY_VERSION_HEX >= 0x02060000
  PyObject_HashNotImplemented, // tp_hash
#else
  nullptr,                // tp_hash
#endif
  nullptr,                // tp_call
  PyVTKReference_Str,     // tp_string
  PyVTKReference_GetAttr, // tp_getattro
  nullptr,                // tp_setattro
  nullptr,                // tp_as_buffer
#ifndef VTK_PY3K
  Py_TPFLAGS_CHECKTYPES |
#endif
    Py_TPFLAGS_DEFAULT,       // tp_flags
  PyVTKReference_Doc,         // tp_doc
  nullptr,                    // tp_traverse
  nullptr,                    // tp_clear
  PyVTKReference_RichCompare, // tp_richcompare
  0,                          // tp_weaklistoffset
  nullptr,                    // tp_iter
  nullptr,                    // tp_iternext
  PyVTKReference_Methods,     // tp_methods
  nullptr,                    // tp_members
  nullptr,                    // tp_getset
  nullptr,                    // tp_base
  nullptr,                    // tp_dict
  nullptr,                    // tp_descr_get
  nullptr,                    // tp_descr_set
  0,                          // tp_dictoffset
  nullptr,                    // tp_init
  nullptr,                    // tp_alloc
  PyVTKReference_New,         // tp_new
  PyObject_Del,               // tp_free
  nullptr,                    // tp_is_gc
  nullptr,                    // tp_bases
  nullptr,                    // tp_mro
  nullptr,                    // tp_cache
  nullptr,                    // tp_subclasses
  nullptr,                    // tp_weaklist
  VTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED };

//--------------------------------------------------------------------
PyTypeObject PyVTKNumberReference_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "vtkmodules.vtkCommonCore.number_reference", // tp_name
  sizeof(PyVTKReference),   // tp_basicsize
  0,                        // tp_itemsize
  PyVTKReference_Delete,    // tp_dealloc
#if PY_VERSION_HEX >= 0x03080000
  0,                        // tp_vectorcall_offset
#else
  nullptr,                  // tp_print
#endif
  nullptr,                  // tp_getattr
  nullptr,                  // tp_setattr
  nullptr,                  // tp_compare
  PyVTKReference_Repr,      // tp_repr
  &PyVTKReference_AsNumber, // tp_as_number
  nullptr,                  // tp_as_sequence
  nullptr,                  // tp_as_mapping
#if PY_VERSION_HEX >= 0x02060000
  PyObject_HashNotImplemented, // tp_hash
#else
  nullptr,                  // tp_hash
#endif
  nullptr,                  // tp_call
  PyVTKReference_Str,       // tp_string
  PyVTKReference_GetAttr,   // tp_getattro
  nullptr,                  // tp_setattro
  nullptr,                  // tp_as_buffer
#ifndef VTK_PY3K
  Py_TPFLAGS_CHECKTYPES |
#endif
    Py_TPFLAGS_DEFAULT,                // tp_flags
  PyVTKReference_Doc,                  // tp_doc
  nullptr,                             // tp_traverse
  nullptr,                             // tp_clear
  PyVTKReference_RichCompare,          // tp_richcompare
  0,                                   // tp_weaklistoffset
  nullptr,                             // tp_iter
  nullptr,                             // tp_iternext
  PyVTKReference_Methods,              // tp_methods
  nullptr,                             // tp_members
  nullptr,                             // tp_getset
  (PyTypeObject*)&PyVTKReference_Type, // tp_base
  nullptr,                             // tp_dict
  nullptr,                             // tp_descr_get
  nullptr,                             // tp_descr_set
  0,                                   // tp_dictoffset
  nullptr,                             // tp_init
  nullptr,                             // tp_alloc
  PyVTKReference_New,                  // tp_new
  PyObject_Del,                        // tp_free
  nullptr,                             // tp_is_gc
  nullptr,                             // tp_bases
  nullptr,                             // tp_mro
  nullptr,                             // tp_cache
  nullptr,                             // tp_subclasses
  nullptr,                             // tp_weaklist
  VTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED };

//--------------------------------------------------------------------
PyTypeObject PyVTKStringReference_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "vtkmodules.vtkCommonCore.string_reference", // tp_name
  sizeof(PyVTKReference),         // tp_basicsize
  0,                              // tp_itemsize
  PyVTKReference_Delete,          // tp_dealloc
#if PY_VERSION_HEX >= 0x03080000
  0,                              // tp_vectorcall_offset
#else
  nullptr,                        // tp_print
#endif
  nullptr,                        // tp_getattr
  nullptr,                        // tp_setattr
  nullptr,                        // tp_compare
  PyVTKReference_Repr,            // tp_repr
  &PyVTKStringReference_AsNumber, // tp_as_number
  &PyVTKReference_AsSequence,     // tp_as_sequence
  &PyVTKReference_AsMapping,      // tp_as_mapping
#if PY_VERSION_HEX >= 0x02060000
  PyObject_HashNotImplemented,    // tp_hash
#else
  nullptr,                        // tp_hash
#endif
  nullptr,                        // tp_call
  PyVTKReference_Str,             // tp_string
  PyVTKReference_GetAttr,         // tp_getattro
  nullptr,                        // tp_setattro
  &PyVTKReference_AsBuffer,       // tp_as_buffer
#ifndef VTK_PY3K
  Py_TPFLAGS_CHECKTYPES |
#endif
    Py_TPFLAGS_DEFAULT,                // tp_flags
  PyVTKReference_Doc,                  // tp_doc
  nullptr,                             // tp_traverse
  nullptr,                             // tp_clear
  PyVTKReference_RichCompare,          // tp_richcompare
  0,                                   // tp_weaklistoffset
  PyVTKReference_GetIter,              // tp_iter
  nullptr,                             // tp_iternext
  PyVTKReference_Methods,              // tp_methods
  nullptr,                             // tp_members
  nullptr,                             // tp_getset
  (PyTypeObject*)&PyVTKReference_Type, // tp_base
  nullptr,                             // tp_dict
  nullptr,                             // tp_descr_get
  nullptr,                             // tp_descr_set
  0,                                   // tp_dictoffset
  nullptr,                             // tp_init
  nullptr,                             // tp_alloc
  PyVTKReference_New,                  // tp_new
  PyObject_Del,                        // tp_free
  nullptr,                             // tp_is_gc
  nullptr,                             // tp_bases
  nullptr,                             // tp_mro
  nullptr,                             // tp_cache
  nullptr,                             // tp_subclasses
  nullptr,                             // tp_weaklist
  VTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED };

//--------------------------------------------------------------------
PyTypeObject PyVTKTupleReference_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "vtkmodules.vtkCommonCore.tuple_reference", // tp_name
  sizeof(PyVTKReference),     // tp_basicsize
  0,                          // tp_itemsize
  PyVTKReference_Delete,      // tp_dealloc
#if PY_VERSION_HEX >= 0x03080000
  0,                          // tp_vectorcall_offset
#else
  nullptr,                    // tp_print
#endif
  nullptr,                    // tp_getattr
  nullptr,                    // tp_setattr
  nullptr,                    // tp_compare
  PyVTKReference_Repr,        // tp_repr
  nullptr,                    // tp_as_number
  &PyVTKReference_AsSequence, // tp_as_sequence
  &PyVTKReference_AsMapping,  // tp_as_mapping
#if PY_VERSION_HEX >= 0x02060000
  PyObject_HashNotImplemented, // tp_hash
#else
  nullptr,                    // tp_hash
#endif
  nullptr,                    // tp_call
  PyVTKReference_Str,         // tp_string
  PyVTKReference_GetAttr,     // tp_getattro
  nullptr,                    // tp_setattro
  nullptr,                    // tp_as_buffer
#ifndef VTK_PY3K
  Py_TPFLAGS_CHECKTYPES |
#endif
    Py_TPFLAGS_DEFAULT,                // tp_flags
  PyVTKReference_Doc,                  // tp_doc
  nullptr,                             // tp_traverse
  nullptr,                             // tp_clear
  PyVTKReference_RichCompare,          // tp_richcompare
  0,                                   // tp_weaklistoffset
  PyVTKReference_GetIter,              // tp_iter
  nullptr,                             // tp_iternext
  PyVTKReference_Methods,              // tp_methods
  nullptr,                             // tp_members
  nullptr,                             // tp_getset
  (PyTypeObject*)&PyVTKReference_Type, // tp_base
  nullptr,                             // tp_dict
  nullptr,                             // tp_descr_get
  nullptr,                             // tp_descr_set
  0,                                   // tp_dictoffset
  nullptr,                             // tp_init
  nullptr,                             // tp_alloc
  PyVTKReference_New,                  // tp_new
  PyObject_Del,                        // tp_free
  nullptr,                             // tp_is_gc
  nullptr,                             // tp_bases
  nullptr,                             // tp_mro
  nullptr,                             // tp_cache
  nullptr,                             // tp_subclasses
  nullptr,                             // tp_weaklist
  VTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED };
// clang-format on
