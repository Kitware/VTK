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

  This class is a proxy for immutable python objects like int, float,
  and string.  It allows these objects to be passed to VTK methods that
  require a ref.
-----------------------------------------------------------------------*/

#include "PyVTKMutableObject.h"
#include "vtkPythonUtil.h"

// Silence warning like
// "dereferencing type-punned pointer will break strict-aliasing rules"
// it happens because this kind of expression: (long *)&ptr
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

//--------------------------------------------------------------------

static const char *PyVTKMutableObject_Doc =
  "A mutable wrapper for immutable objects.\n\n"
  "This wrapper class is needed when a VTK method returns a value\n"
  "in an argument that has been passed by reference.  By calling\n"
  "\"m = vtk.mutable(a)\" on a value, you can create a mutable proxy\n"
  "to that value.  The value can be changed by calling \"m.set(b)\".\n";

//--------------------------------------------------------------------
// helper method: make sure than an object is usable
static PyObject *PyVTKMutableObject_CompatibleObject(PyObject *opn)
{
  PyNumberMethods *nb = Py_TYPE(opn)->tp_as_number;

  if (PyFloat_Check(opn) ||
      PyLong_Check(opn) ||
#ifndef VTK_PY3K
      PyInt_Check(opn) ||
#endif
#ifdef Py_USING_UNICODE
      PyUnicode_Check(opn) ||
#endif
      PyBytes_Check(opn))
  {
    Py_INCREF(opn);
  }
  else if (PyVTKMutableObject_Check(opn))
  {
    opn = ((PyVTKMutableObject *)opn)->value;
    Py_INCREF(opn);
  }
  else if (nb && nb->nb_index)
  {
    opn = nb->nb_index(opn);
    if (opn == 0 || (!PyLong_Check(opn)
#ifndef VTK_PY3K
        && !PyInt_Check(opn)
#endif
        ))
    {
      PyErr_SetString(PyExc_TypeError,
                      "nb_index should return integer object");
      return NULL;
    }
  }
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
                    "a numeric or string object is required");
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
    return ((PyVTKMutableObject *)self)->value;
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
    PyObject **op = &((PyVTKMutableObject *)self)->value;

    if (PyFloat_Check(val) ||
#ifndef VTK_PY3K
        PyInt_Check(val) ||
#endif
        PyLong_Check(val))
    {
      if (PyFloat_Check(*op) ||
#ifndef VTK_PY3K
          PyInt_Check(*op) ||
#endif
          PyLong_Check(*op))
      {
        Py_DECREF(*op);
        *op = val;
        return 0;
      }
      PyErr_SetString(PyExc_TypeError,
                      "cannot set a string mutable to a numeric value");
    }
    else if (
#ifdef Py_USING_UNICODE
        PyUnicode_Check(val) ||
#endif
        PyBytes_Check(val))
    {
      if (
#ifdef Py_USING_UNICODE
          PyUnicode_Check(*op) ||
#endif
          PyBytes_Check(*op))
      {
        Py_DECREF(*op);
        *op = val;
        return 0;
      }
      PyErr_SetString(PyExc_TypeError,
                      "cannot set a numeric mutable to a string value");
    }
    else
    {
      PyErr_SetString(PyExc_TypeError,
                      "a float, long, int, or string is required");
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
  if (PyArg_ParseTuple(args, ":get"))
  {
    PyObject *ob = PyVTKMutableObject_GetValue(self);
    Py_INCREF(ob);
    return ob;
  }

  return NULL;
}

static PyObject *PyVTKMutableObject_Set(PyObject *self, PyObject *args)
{
  PyObject *opn;

  if (PyArg_ParseTuple(args, "O:set", &opn))
  {
    opn = PyVTKMutableObject_CompatibleObject(opn);

    if (opn)
    {
      if (PyVTKMutableObject_SetValue(self, opn) == 0)
      {
        Py_INCREF(Py_None);
        return Py_None;
      }
    }
  }

  return NULL;
}

#ifdef VTK_PY3K
static PyObject *PyVTKMutableObject_Trunc(PyObject *self, PyObject *args)
{
  PyObject *opn;

  if (PyArg_ParseTuple(args, ":__trunc__", &opn))
  {
    PyObject *attr = PyUnicode_InternFromString("__trunc__");
    PyObject *ob = PyVTKMutableObject_GetValue(self);
    PyObject *meth = _PyType_Lookup(Py_TYPE(ob), attr);
    if (meth == NULL)
    {
      PyErr_Format(PyExc_TypeError,
                   "type %.100s doesn't define __trunc__ method",
                   Py_TYPE(ob)->tp_name);
      return NULL;
    }
    return PyObject_CallFunction(meth, (char *)"O", ob);
  }

  return NULL;
}

static PyObject *PyVTKMutableObject_Round(PyObject *self, PyObject *args)
{
  PyObject *opn;

  if (PyArg_ParseTuple(args, "|O:__round__", &opn))
  {
    PyObject *attr = PyUnicode_InternFromString("__round__");
    PyObject *ob = PyVTKMutableObject_GetValue(self);
    PyObject *meth = _PyType_Lookup(Py_TYPE(ob), attr);
    if (meth == NULL)
    {
      PyErr_Format(PyExc_TypeError,
                   "type %.100s doesn't define __round__ method",
                   Py_TYPE(ob)->tp_name);
      return NULL;
    }
    if (opn)
    {
      return PyObject_CallFunction(meth, (char *)"OO", ob, opn);
    }
    return PyObject_CallFunction(meth, (char *)"O", ob);
  }

  return NULL;
}
#endif

static PyMethodDef PyVTKMutableObject_Methods[] = {
  {"get", PyVTKMutableObject_Get, METH_VARARGS, "Get the stored value."},
  {"set", PyVTKMutableObject_Set, METH_VARARGS, "Set the stored value."},
#ifdef VTK_PY3K
  {"__trunc__", PyVTKMutableObject_Trunc, METH_VARARGS,
   "Returns the Integral closest to x between 0 and x."},
  {"__round__", PyVTKMutableObject_Round, METH_VARARGS,
   "Returns the Integral closest to x, rounding half toward even.\n"},
#endif
  { NULL, NULL, 0, NULL }
};


//--------------------------------------------------------------------
// Macros used for defining protocol methods

#define REFOBJECT_INTFUNC(prot, op) \
static int PyVTKMutableObject_##op(PyObject *ob) \
{ \
  ob = ((PyVTKMutableObject *)ob)->value; \
  return Py##prot##_##op(ob); \
}

#define REFOBJECT_SIZEFUNC(prot, op) \
static Py_ssize_t PyVTKMutableObject_##op(PyObject *ob) \
{ \
  ob = ((PyVTKMutableObject *)ob)->value; \
  return Py##prot##_##op(ob); \
}

#define REFOBJECT_INDEXFUNC(prot, op) \
static PyObject *PyVTKMutableObject_##op(PyObject *ob, Py_ssize_t i) \
{ \
  ob = ((PyVTKMutableObject *)ob)->value; \
  return Py##prot##_##op(ob, i); \
}

#define REFOBJECT_INDEXSETFUNC(prot, op) \
static int PyVTKMutableObject_##op(PyObject *ob, Py_ssize_t i, PyObject *o) \
{ \
  ob = ((PyVTKMutableObject *)ob)->value; \
  return Py##prot##_##op(ob, i, o); \
}

#define REFOBJECT_SLICEFUNC(prot, op) \
static PyObject *PyVTKMutableObject_##op(PyObject *ob, Py_ssize_t i, Py_ssize_t j) \
{ \
  ob = ((PyVTKMutableObject *)ob)->value; \
  return Py##prot##_##op(ob, i, j); \
}

#define REFOBJECT_SLICESETFUNC(prot, op) \
static int PyVTKMutableObject_##op(PyObject *ob, Py_ssize_t i, Py_ssize_t j, PyObject *o) \
{ \
  ob = ((PyVTKMutableObject *)ob)->value; \
  return Py##prot##_##op(ob, i, j, o); \
}

#define REFOBJECT_UNARYFUNC(prot, op) \
static PyObject *PyVTKMutableObject_##op(PyObject *ob) \
{ \
  ob = ((PyVTKMutableObject *)ob)->value; \
  return Py##prot##_##op(ob); \
}

#define REFOBJECT_BINARYFUNC(prot, op) \
static PyObject *PyVTKMutableObject_##op(PyObject *ob1, PyObject *ob2) \
{ \
  if (PyVTKMutableObject_Check(ob1)) \
  { \
    ob1 = ((PyVTKMutableObject *)ob1)->value; \
  } \
  if (PyVTKMutableObject_Check(ob2)) \
  { \
    ob2 = ((PyVTKMutableObject *)ob2)->value; \
  } \
  return Py##prot##_##op(ob1, ob2); \
}

#define REFOBJECT_INPLACEFUNC(prot, op) \
static PyObject *PyVTKMutableObject_InPlace##op(PyObject *ob1, PyObject *ob2) \
{ \
  PyVTKMutableObject *ob = (PyVTKMutableObject *)ob1; \
  PyObject *obn;\
  ob1 = ob->value; \
  if (PyVTKMutableObject_Check(ob2)) \
  { \
    ob2 = ((PyVTKMutableObject *)ob2)->value; \
  } \
  obn = Py##prot##_##op(ob1, ob2); \
  if (obn) \
  { \
    ob->value = obn; \
    Py_DECREF(ob1); \
    Py_INCREF(ob); \
    return (PyObject *)ob; \
  } \
  return 0; \
}

#define REFOBJECT_INPLACEIFUNC(prot, op) \
static PyObject *PyVTKMutableObject_InPlace##op(PyObject *ob1, Py_ssize_t i) \
{ \
  PyVTKMutableObject *ob = (PyVTKMutableObject *)ob1; \
  PyObject *obn;\
  ob1 = ob->value; \
  obn = Py##prot##_##op(ob1, i); \
  if (obn) \
  { \
    ob->value = obn; \
    Py_DECREF(ob1); \
    Py_INCREF(ob); \
    return (PyObject *)ob; \
  } \
  return 0; \
}


#define REFOBJECT_TERNARYFUNC(prot, op) \
static PyObject *PyVTKMutableObject_##op(PyObject *ob1, PyObject *ob2, PyObject *ob3) \
{ \
  if (PyVTKMutableObject_Check(ob1)) \
  { \
    ob1 = ((PyVTKMutableObject *)ob1)->value; \
  } \
  if (PyVTKMutableObject_Check(ob2)) \
  { \
    ob2 = ((PyVTKMutableObject *)ob2)->value; \
  } \
  if (PyVTKMutableObject_Check(ob2)) \
  { \
    ob3 = ((PyVTKMutableObject *)ob3)->value; \
  } \
  return Py##prot##_##op(ob1, ob2, ob3); \
}

#define REFOBJECT_INPLACETFUNC(prot, op) \
static PyObject *PyVTKMutableObject_InPlace##op(PyObject *ob1, PyObject *ob2, PyObject *ob3) \
{ \
  PyVTKMutableObject *ob = (PyVTKMutableObject *)ob1; \
  PyObject *obn; \
  ob1 = ob->value; \
  if (PyVTKMutableObject_Check(ob2)) \
  { \
    ob2 = ((PyVTKMutableObject *)ob2)->value; \
  } \
  if (PyVTKMutableObject_Check(ob3)) \
  { \
    ob3 = ((PyVTKMutableObject *)ob3)->value; \
  } \
  obn = Py##prot##_##op(ob1, ob2, ob3); \
  if (obn) \
  { \
    ob->value = obn; \
    Py_DECREF(ob1); \
    Py_INCREF(ob); \
    return (PyObject *)ob; \
  } \
  return 0; \
}

//--------------------------------------------------------------------
// Number protocol

static int PyVTKMutableObject_NonZero(PyObject *ob)
{
  ob = ((PyVTKMutableObject *)ob)->value;
  return PyObject_IsTrue(ob);
}

#ifndef VTK_PY3K
static int PyVTKMutableObject_Coerce(PyObject **ob1, PyObject **ob2)
{
  *ob1 = ((PyVTKMutableObject *)*ob1)->value;
  if (PyVTKMutableObject_Check(*ob2))
  {
    *ob2 = ((PyVTKMutableObject *)*ob2)->value;
  }
  return PyNumber_CoerceEx(ob1, ob2);
}

static PyObject *PyVTKMutableObject_Hex(PyObject *ob)
{
  ob = ((PyVTKMutableObject *)ob)->value;
#if PY_VERSION_HEX >= 0x02060000
  return PyNumber_ToBase(ob, 16);
#else
  if (Py_TYPE(ob)->tp_as_number &&
      Py_TYPE(ob)->tp_as_number->nb_hex)
  {
    return Py_TYPE(ob)->tp_as_number->nb_hex(ob);
  }

  PyErr_SetString(PyExc_TypeError,
                  "hex() argument can't be converted to hex");
  return NULL;
#endif
}

static PyObject *PyVTKMutableObject_Oct(PyObject *ob)
{
  ob = ((PyVTKMutableObject *)ob)->value;
#if PY_VERSION_HEX >= 0x02060000
  return PyNumber_ToBase(ob, 8);
#else
  if (Py_TYPE(ob)->tp_as_number &&
      Py_TYPE(ob)->tp_as_number->nb_oct)
  {
    return Py_TYPE(ob)->tp_as_number->nb_oct(ob);
  }

  PyErr_SetString(PyExc_TypeError,
                  "oct() argument can't be converted to oct");
  return NULL;
#endif
}
#endif


REFOBJECT_BINARYFUNC(Number,Add)
REFOBJECT_BINARYFUNC(Number,Subtract)
REFOBJECT_BINARYFUNC(Number,Multiply)
#ifndef VTK_PY3K
REFOBJECT_BINARYFUNC(Number,Divide)
#endif
REFOBJECT_BINARYFUNC(Number,Remainder)
REFOBJECT_BINARYFUNC(Number,Divmod)
REFOBJECT_TERNARYFUNC(Number,Power)
REFOBJECT_UNARYFUNC(Number,Negative)
REFOBJECT_UNARYFUNC(Number,Positive)
REFOBJECT_UNARYFUNC(Number,Absolute)
// NonZero
REFOBJECT_UNARYFUNC(Number,Invert)
REFOBJECT_BINARYFUNC(Number,Lshift)
REFOBJECT_BINARYFUNC(Number,Rshift)
REFOBJECT_BINARYFUNC(Number,And)
REFOBJECT_BINARYFUNC(Number,Or)
REFOBJECT_BINARYFUNC(Number,Xor)
// Coerce
#ifndef VTK_PY3K
REFOBJECT_UNARYFUNC(Number,Int)
#endif
REFOBJECT_UNARYFUNC(Number,Long)
REFOBJECT_UNARYFUNC(Number,Float)

REFOBJECT_INPLACEFUNC(Number,Add)
REFOBJECT_INPLACEFUNC(Number,Subtract)
REFOBJECT_INPLACEFUNC(Number,Multiply)
#ifndef VTK_PY3K
REFOBJECT_INPLACEFUNC(Number,Divide)
#endif
REFOBJECT_INPLACEFUNC(Number,Remainder)
REFOBJECT_INPLACETFUNC(Number,Power)
REFOBJECT_INPLACEFUNC(Number,Lshift)
REFOBJECT_INPLACEFUNC(Number,Rshift)
REFOBJECT_INPLACEFUNC(Number,And)
REFOBJECT_INPLACEFUNC(Number,Or)
REFOBJECT_INPLACEFUNC(Number,Xor)

REFOBJECT_BINARYFUNC(Number,FloorDivide)
REFOBJECT_BINARYFUNC(Number,TrueDivide)
REFOBJECT_INPLACEFUNC(Number,FloorDivide)
REFOBJECT_INPLACEFUNC(Number,TrueDivide)

REFOBJECT_UNARYFUNC(Number,Index)

//--------------------------------------------------------------------
static PyNumberMethods PyVTKMutableObject_AsNumber = {
  PyVTKMutableObject_Add,                    // nb_add
  PyVTKMutableObject_Subtract,               // nb_subtract
  PyVTKMutableObject_Multiply,               // nb_multiply
#ifndef VTK_PY3K
  PyVTKMutableObject_Divide,                 // nb_divide
#endif
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
#ifndef VTK_PY3K
  PyVTKMutableObject_Coerce,                 // nb_coerce
  PyVTKMutableObject_Int,                    // nb_int
  PyVTKMutableObject_Long,                   // nb_long
#else
  PyVTKMutableObject_Long,                   // nb_int
  NULL,                                      // nb_reserved
#endif
  PyVTKMutableObject_Float,                  // nb_float
#ifndef VTK_PY3K
  PyVTKMutableObject_Oct,                    // nb_oct
  PyVTKMutableObject_Hex,                    // nb_hex
#endif
  PyVTKMutableObject_InPlaceAdd,             // nb_inplace_add
  PyVTKMutableObject_InPlaceSubtract,        // nb_inplace_subtract
  PyVTKMutableObject_InPlaceMultiply,        // nb_inplace_multiply
#ifndef VTK_PY3K
  PyVTKMutableObject_InPlaceDivide,          // nb_inplace_divide
#endif
  PyVTKMutableObject_InPlaceRemainder,       // nb_inplace_remainder
  PyVTKMutableObject_InPlacePower,           // nb_inplace_power
  PyVTKMutableObject_InPlaceLshift,          // nb_inplace_lshift
  PyVTKMutableObject_InPlaceRshift,          // nb_inplace_rshift
  PyVTKMutableObject_InPlaceAnd,             // nb_inplace_and
  PyVTKMutableObject_InPlaceXor,             // nb_inplace_xor
  PyVTKMutableObject_InPlaceOr,              // nb_inplace_or
  PyVTKMutableObject_FloorDivide,            // nb_floor_divide
  PyVTKMutableObject_TrueDivide,             // nb_true_divide
  PyVTKMutableObject_InPlaceFloorDivide,     // nb_inplace_floor_divide
  PyVTKMutableObject_InPlaceTrueDivide,      // nb_inplace_true_divide
  PyVTKMutableObject_Index,                  // nb_index
#if PY_VERSION_HEX >= 0x03050000
  0,                                         // nb_matrix_multiply
  0,                                         // nb_inplace_matrix_multiply
#endif
};


// Disable sequence and mapping protocols until a subtype is made
#if 0
//--------------------------------------------------------------------
// Sequence protocol

REFOBJECT_SIZEFUNC(Sequence,Size)
REFOBJECT_BINARYFUNC(Sequence,Concat)
REFOBJECT_INDEXFUNC(Sequence,Repeat)
REFOBJECT_INDEXFUNC(Sequence,GetItem)
REFOBJECT_SLICEFUNC(Sequence,GetSlice)
REFOBJECT_INDEXSETFUNC(Sequence,SetItem)
REFOBJECT_SLICESETFUNC(Sequence,SetSlice)

REFOBJECT_INPLACEFUNC(Sequence,Concat)
REFOBJECT_INPLACEIFUNC(Sequence,Repeat)

//--------------------------------------------------------------------
static PySequenceMethods PyVTKMutableObject_AsSequence = {
  PyVTKMutableObject_Size,                   // sq_length
  PyVTKMutableObject_Concat,                 // sq_concat
  PyVTKMutableObject_Repeat,                 // sq_repeat
  PyVTKMutableObject_GetItem,                // sq_item
  PyVTKMutableObject_GetSlice,               // sq_slice
  PyVTKMutableObject_SetItem,                // sq_ass_item
  PyVTKMutableObject_SetSlice,               // sq_ass_slice
  0,                                         // sq_contains
  PyVTKMutableObject_InPlaceConcat,          // sq_inplace_concat
  PyVTKMutableObject_InPlaceRepeat,          // sq_inplace_repeat
};

//--------------------------------------------------------------------
// Mapping protocol

static PyObject *
PyVTKMutableObject_GetMapItem(PyObject *ob, PyObject *key)
{
  ob = ((PyVTKMutableObject *)ob)->value;
  return PyObject_GetItem(ob, key);
}

static int
PyVTKMutableObject_SetMapItem(PyObject *ob, PyObject *key, PyObject *o)
{
  ob = ((PyVTKMutableObject *)ob)->value;
  return PyObject_SetItem(ob, key, o);
}

//--------------------------------------------------------------------
static PyMappingMethods PyVTKMutableObject_AsMapping = {
  PyVTKMutableObject_Size,                   // mp_length
  PyVTKMutableObject_GetMapItem,             // mp_subscript
  PyVTKMutableObject_SetMapItem,             // mp_ass_subscript
};
#endif

//--------------------------------------------------------------------
// Buffer protocol

#ifndef VTK_PY3K
// old buffer protocol
static Py_ssize_t PyVTKMutableObject_GetReadBuf(
  PyObject *op, Py_ssize_t segment, void **ptrptr)
{
  char text[80];
  PyBufferProcs *pb;
  op = ((PyVTKMutableObject *)op)->value;
  pb = Py_TYPE(op)->tp_as_buffer;

  if (pb && pb->bf_getreadbuffer)
  {
    return Py_TYPE(op)->tp_as_buffer->bf_getreadbuffer(
      op, segment, ptrptr);
  }

  sprintf(text, "type \'%.20s\' does not support readable buffer access",
          Py_TYPE(op)->tp_name);
  PyErr_SetString(PyExc_TypeError, text);

  return -1;
}

static Py_ssize_t PyVTKMutableObject_GetWriteBuf(
  PyObject *op, Py_ssize_t segment, void **ptrptr)
{
  char text[80];
  PyBufferProcs *pb;
  op = ((PyVTKMutableObject *)op)->value;
  pb = Py_TYPE(op)->tp_as_buffer;

  if (pb && pb->bf_getwritebuffer)
  {
    return Py_TYPE(op)->tp_as_buffer->bf_getwritebuffer(
      op, segment, ptrptr);
  }

  sprintf(text, "type \'%.20s\' does not support writeable buffer access",
          Py_TYPE(op)->tp_name);
  PyErr_SetString(PyExc_TypeError, text);

  return -1;
}

static Py_ssize_t
PyVTKMutableObject_GetSegCount(PyObject *op, Py_ssize_t *lenp)
{
  char text[80];
  PyBufferProcs *pb;
  op = ((PyVTKMutableObject *)op)->value;
  pb = Py_TYPE(op)->tp_as_buffer;

  if (pb && pb->bf_getsegcount)
  {
    return Py_TYPE(op)->tp_as_buffer->bf_getsegcount(op, lenp);
  }

  sprintf(text, "type \'%.20s\' does not support buffer access",
          Py_TYPE(op)->tp_name);
  PyErr_SetString(PyExc_TypeError, text);

  return -1;
}

static Py_ssize_t PyVTKMutableObject_GetCharBuf(
  PyObject *op, Py_ssize_t segment, char **ptrptr)
{
  char text[80];
  PyBufferProcs *pb;
  op = ((PyVTKMutableObject *)op)->value;
  pb = Py_TYPE(op)->tp_as_buffer;

  if (pb && pb->bf_getcharbuffer)
  {
    return Py_TYPE(op)->tp_as_buffer->bf_getcharbuffer(
      op, segment, ptrptr);
  }

  sprintf(text, "type \'%.20s\' does not support character buffer access",
          Py_TYPE(op)->tp_name);
  PyErr_SetString(PyExc_TypeError, text);

  return -1;
}
#endif

#if PY_VERSION_HEX >= 0x02060000
// new buffer protocol
static int PyVTKMutableObject_GetBuffer(
  PyObject *self, Py_buffer *view, int flags)
{
  PyObject *obj = ((PyVTKMutableObject *)self)->value;
  return PyObject_GetBuffer(obj, view, flags);
}

static void PyVTKMutableObject_ReleaseBuffer(
  PyObject *, Py_buffer *view)
{
  PyBuffer_Release(view);
}
#endif

static PyBufferProcs PyVTKMutableObject_AsBuffer = {
#ifndef VTK_PY3K
  PyVTKMutableObject_GetReadBuf,       // bf_getreadbuffer
  PyVTKMutableObject_GetWriteBuf,      // bf_getwritebuffer
  PyVTKMutableObject_GetSegCount,      // bf_getsegcount
  PyVTKMutableObject_GetCharBuf,       // bf_getcharbuffer
#endif
#if PY_VERSION_HEX >= 0x02060000
  PyVTKMutableObject_GetBuffer,        // bf_getbuffer
  PyVTKMutableObject_ReleaseBuffer     // bf_releasebuffer
#endif
};

//--------------------------------------------------------------------
// Object protocol

static void PyVTKMutableObject_Delete(PyObject *ob)
{
  Py_DECREF(((PyVTKMutableObject *)ob)->value);
  PyObject_Del(ob);
}

static PyObject *PyVTKMutableObject_Repr(PyObject *ob)
{
  PyObject *r = 0;
  const char *name = Py_TYPE(ob)->tp_name;
  PyObject *s = PyObject_Repr(((PyVTKMutableObject *)ob)->value);
  if (s)
  {
#ifdef VTK_PY3K
    r = PyUnicode_FromFormat("%s(%U)", name, s);
#else
    char textspace[128];
    const char *text = PyString_AsString(s);
    size_t n = strlen(name) + strlen(text) + 3;
    char *cp = textspace;
    if (n > 128) { cp = (char *)malloc(n); }
    sprintf(cp, "%s(%s)", name, text);
    r = PyString_FromString(cp);
    if (n > 128) { free(cp); }
#endif
    Py_DECREF(s);
  }
  return r;
}

static PyObject *PyVTKMutableObject_Str(PyObject *ob)
{
  return PyObject_Str(((PyVTKMutableObject *)ob)->value);
}

static PyObject *PyVTKMutableObject_RichCompare(
  PyObject *ob1, PyObject *ob2, int opid)
{
  if (PyVTKMutableObject_Check(ob1))
  {
    ob1 = ((PyVTKMutableObject *)ob1)->value;
  }
  if (PyVTKMutableObject_Check(ob2))
  {
    ob2 = ((PyVTKMutableObject *)ob2)->value;
  }
  return PyObject_RichCompare(ob1, ob2, opid);
}

static PyObject *PyVTKMutableObject_GetAttr(PyObject *self, PyObject *attr)
{
  PyObject *a = PyObject_GenericGetAttr(self, attr);
  if (a || !PyErr_ExceptionMatches(PyExc_AttributeError))
  {
    return a;
  }
  PyErr_Clear();

#ifndef VTK_PY3K
  char *name = PyString_AsString(attr);
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
    a = PyObject_GetAttr(((PyVTKMutableObject *)self)->value, attr);

    if (a || !PyErr_ExceptionMatches(PyExc_AttributeError))
    {
      return a;
    }
    PyErr_Clear();
  }

#ifdef VTK_PY3K
  PyErr_Format(PyExc_AttributeError,
               "'%.50s' object has no attribute '%U'",
               Py_TYPE(self)->tp_name, attr);
#else
  PyErr_Format(PyExc_AttributeError,
               "'%.50s' object has no attribute '%.80s'",
               Py_TYPE(self)->tp_name, name);
#endif
  return NULL;
}

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
      PyVTKMutableObject *self = PyObject_New(PyVTKMutableObject, &PyVTKMutableObject_Type);
      self->value = o;
      return (PyObject *)self;
    }
  }

  return NULL;
}

//--------------------------------------------------------------------
PyTypeObject PyVTKMutableObject_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "vtkCommonCorePython.mutable",         // tp_name
  sizeof(PyVTKMutableObject),            // tp_basicsize
  0,                                     // tp_itemsize
  PyVTKMutableObject_Delete,             // tp_dealloc
  0,                                     // tp_print
  0,                                     // tp_getattr
  0,                                     // tp_setattr
  0,                                     // tp_compare
  PyVTKMutableObject_Repr,               // tp_repr
  &PyVTKMutableObject_AsNumber,          // tp_as_number
  0,                                     // tp_as_sequence
  0,                                     // tp_as_mapping
#if PY_VERSION_HEX >= 0x02060000
  PyObject_HashNotImplemented,           // tp_hash
#else
  0,                                     // tp_hash
#endif
  0,                                     // tp_call
  PyVTKMutableObject_Str,                // tp_string
  PyVTKMutableObject_GetAttr,            // tp_getattro
  0,                                     // tp_setattro
  &PyVTKMutableObject_AsBuffer,          // tp_as_buffer
#ifndef VTK_PY3K
  Py_TPFLAGS_CHECKTYPES |
#endif
  Py_TPFLAGS_DEFAULT,                    // tp_flags
  PyVTKMutableObject_Doc,                // tp_doc
  0,                                     // tp_traverse
  0,                                     // tp_clear
  PyVTKMutableObject_RichCompare,        // tp_richcompare
  0,                                     // tp_weaklistoffset
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
  PyObject_Del,                          // tp_free
  0,                                     // tp_is_gc
  0,                                     // tp_bases
  0,                                     // tp_mro
  0,                                     // tp_cache
  0,                                     // tp_subclasses
  0,                                     // tp_weaklist
  VTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED
};
