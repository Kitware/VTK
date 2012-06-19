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
// pragma GCC diagnostic is available since gcc>=4.2
#if defined(__GNUG__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=2)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

//--------------------------------------------------------------------
// methods for adding this type to a module
extern "C" { VTKWRAPPINGPYTHON_EXPORT void PyVTKAddFile_mutable(PyObject *, const char *); }

//--------------------------------------------------------------------

const char *PyVTKMutableObject_Doc =
  "A mutable wrapper for immutable objects.\n\n"
  "This wrapper class is needed when a VTK method returns a value\n"
  "in an argument that has been passed by reference.  By calling\n"
  "\"m = vtk.mutable(a)\" on a value, you can create a mutable proxy\n"
  "to that value.  The value can be changed by calling \"m.set(b)\".\n";

//--------------------------------------------------------------------
// helper method: make sure than an object is usable
static PyObject *PyVTKMutableObject_CompatibleObject(PyObject *opn)
{
  PyNumberMethods *nb = opn->ob_type->tp_as_number;

  if (PyFloat_Check(opn) ||
      PyLong_Check(opn) ||
      PyInt_Check(opn) ||
#ifdef Py_USING_UNICODE
      PyUnicode_Check(opn) ||
#endif
      PyString_Check(opn))
    {
    Py_INCREF(opn);
    }
  else if (PyVTKMutableObject_Check(opn))
    {
    opn = ((PyVTKMutableObject *)opn)->value;
    Py_INCREF(opn);
    }
#if PY_VERSION_HEX >= 0x02050000
  else if (nb && nb->nb_index)
    {
    opn = nb->nb_index(opn);
    if (opn == 0 || (!PyLong_Check(opn) && !PyInt_Check(opn)))
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
        PyLong_Check(val) ||
        PyInt_Check(val))
      {
      if (PyFloat_Check(*op) ||
          PyLong_Check(*op) ||
          PyInt_Check(*op))
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
        PyString_Check(val))
      {
      if (
#ifdef Py_USING_UNICODE
          PyUnicode_Check(*op) ||
#endif
          PyString_Check(*op))
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
  if (PyArg_ParseTuple(args, (char*)":get"))
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

  if (PyArg_ParseTuple(args, (char*)"O:set", &opn))
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

static PyMethodDef PyVTKMutableObject_Methods[] = {
  {(char*)"get", PyVTKMutableObject_Get, 1, (char *)"Get the stored value."},
  {(char*)"set", PyVTKMutableObject_Set, 1, (char *)"Set the stored value."},
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
  if (ob->ob_type->tp_as_number &&
      ob->ob_type->tp_as_number->nb_hex)
    {
    return ob->ob_type->tp_as_number->nb_hex(ob);
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
  if (ob->ob_type->tp_as_number &&
      ob->ob_type->tp_as_number->nb_oct)
    {
    return ob->ob_type->tp_as_number->nb_oct(ob);
    }

  PyErr_SetString(PyExc_TypeError,
                  "oct() argument can't be converted to oct");
  return NULL;
#endif
}


REFOBJECT_BINARYFUNC(Number,Add)
REFOBJECT_BINARYFUNC(Number,Subtract)
REFOBJECT_BINARYFUNC(Number,Multiply)
REFOBJECT_BINARYFUNC(Number,Divide)
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
REFOBJECT_UNARYFUNC(Number,Int)
REFOBJECT_UNARYFUNC(Number,Long)
REFOBJECT_UNARYFUNC(Number,Float)
// Hex
// Oct

#if PY_VERSION_HEX >= 0x02000000
REFOBJECT_INPLACEFUNC(Number,Add)
REFOBJECT_INPLACEFUNC(Number,Subtract)
REFOBJECT_INPLACEFUNC(Number,Multiply)
REFOBJECT_INPLACEFUNC(Number,Divide)
REFOBJECT_INPLACEFUNC(Number,Remainder)
REFOBJECT_INPLACETFUNC(Number,Power)
REFOBJECT_INPLACEFUNC(Number,Lshift)
REFOBJECT_INPLACEFUNC(Number,Rshift)
REFOBJECT_INPLACEFUNC(Number,And)
REFOBJECT_INPLACEFUNC(Number,Or)
REFOBJECT_INPLACEFUNC(Number,Xor)
#endif

#if PY_VERSION_HEX >= 0x02020000
REFOBJECT_BINARYFUNC(Number,FloorDivide)
REFOBJECT_BINARYFUNC(Number,TrueDivide)
REFOBJECT_INPLACEFUNC(Number,FloorDivide)
REFOBJECT_INPLACEFUNC(Number,TrueDivide)
#endif

#if PY_VERSION_HEX >= 0x02050000
REFOBJECT_UNARYFUNC(Number,Index)
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


// Disable sequence and mapping protocols until a subtype is made
#if 0
//--------------------------------------------------------------------
// Sequence protocol

#if PY_MAJOR_VERSION >= 2
REFOBJECT_SIZEFUNC(Sequence,Size)
#else
REFOBJECT_SIZEFUNC(Sequence,Length)
#endif
REFOBJECT_BINARYFUNC(Sequence,Concat)
REFOBJECT_INDEXFUNC(Sequence,Repeat)
REFOBJECT_INDEXFUNC(Sequence,GetItem)
REFOBJECT_SLICEFUNC(Sequence,GetSlice)
REFOBJECT_INDEXSETFUNC(Sequence,SetItem)
REFOBJECT_SLICESETFUNC(Sequence,SetSlice)

#if PY_VERSION_HEX >= 0x02000000
REFOBJECT_INPLACEFUNC(Sequence,Concat)
REFOBJECT_INPLACEIFUNC(Sequence,Repeat)
#endif

//--------------------------------------------------------------------
static PySequenceMethods PyVTKMutableObject_AsSequence = {
#if PY_MAJOR_VERSION >= 2
  PyVTKMutableObject_Size,                   // sq_length
#else
  PyVTKMutableObject_Length,                 // sq_length
#endif
  PyVTKMutableObject_Concat,                 // sq_concat
  PyVTKMutableObject_Repeat,                 // sq_repeat
  PyVTKMutableObject_GetItem,                // sq_item
  PyVTKMutableObject_GetSlice,               // sq_slice
  PyVTKMutableObject_SetItem,                // sq_ass_item
  PyVTKMutableObject_SetSlice,               // sq_ass_slice
#if PY_VERSION_HEX >= 0x02000000
  0,                                         // sq_contains
  PyVTKMutableObject_InPlaceConcat,          // sq_inplace_concat
  PyVTKMutableObject_InPlaceRepeat,          // sq_inplace_repeat
#endif
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
#if PY_MAJOR_VERSION >= 2
  PyVTKMutableObject_Size,                   // mp_length
#else
  PyVTKMutableObject_Length,                 // mp_length
#endif
  PyVTKMutableObject_GetMapItem,             // mp_subscript
  PyVTKMutableObject_SetMapItem,             // mp_ass_subscript
};
#endif

//--------------------------------------------------------------------
// Buffer protocol

static Py_ssize_t PyVTKMutableObject_GetReadBuf(
  PyObject *op, Py_ssize_t segment, void **ptrptr)
{
  char text[80];
  PyBufferProcs *pb;
  op = ((PyVTKMutableObject *)op)->value;
  pb = op->ob_type->tp_as_buffer;

  if (pb && pb->bf_getreadbuffer)
    {
    return op->ob_type->tp_as_buffer->bf_getreadbuffer(
      op, segment, ptrptr);
    }

  sprintf(text, "type \'%.20s\' does not support readable buffer access",
          op->ob_type->tp_name);
  PyErr_SetString(PyExc_TypeError, text);

  return -1;
}

static Py_ssize_t PyVTKMutableObject_GetWriteBuf(
  PyObject *op, Py_ssize_t segment, void **ptrptr)
{
  char text[80];
  PyBufferProcs *pb;
  op = ((PyVTKMutableObject *)op)->value;
  pb = op->ob_type->tp_as_buffer;

  if (pb && pb->bf_getwritebuffer)
    {
    return op->ob_type->tp_as_buffer->bf_getwritebuffer(
      op, segment, ptrptr);
    }

  sprintf(text, "type \'%.20s\' does not support writeable buffer access",
          op->ob_type->tp_name);
  PyErr_SetString(PyExc_TypeError, text);

  return -1;
}

static Py_ssize_t
PyVTKMutableObject_GetSegCount(PyObject *op, Py_ssize_t *lenp)
{
  char text[80];
  PyBufferProcs *pb;
  op = ((PyVTKMutableObject *)op)->value;
  pb = op->ob_type->tp_as_buffer;

  if (pb && pb->bf_getsegcount)
    {
    return op->ob_type->tp_as_buffer->bf_getsegcount(op, lenp);
    }

  sprintf(text, "type \'%.20s\' does not support buffer access",
          op->ob_type->tp_name);
  PyErr_SetString(PyExc_TypeError, text);

  return -1;
}

#if PY_VERSION_HEX >= 0x02050000
static Py_ssize_t PyVTKMutableObject_GetCharBuf(
  PyObject *op, Py_ssize_t segment, char **ptrptr)
#else
static Py_ssize_t PyVTKMutableObject_GetCharBuf(
  PyObject *op, Py_ssize_t segment, const char **ptrptr)
#endif
{
  char text[80];
  PyBufferProcs *pb;
  op = ((PyVTKMutableObject *)op)->value;
  pb = op->ob_type->tp_as_buffer;

  if (pb && pb->bf_getcharbuffer)
    {
    return op->ob_type->tp_as_buffer->bf_getcharbuffer(
      op, segment, ptrptr);
    }

  sprintf(text, "type \'%.20s\' does not support character buffer access",
          op->ob_type->tp_name);
  PyErr_SetString(PyExc_TypeError, text);

  return -1;
}

static PyBufferProcs PyVTKMutableObject_AsBuffer = {
  PyVTKMutableObject_GetReadBuf,       // bf_getreadbuffer
  PyVTKMutableObject_GetWriteBuf,      // bf_getwritebuffer
  PyVTKMutableObject_GetSegCount,      // bf_getsegcount
  PyVTKMutableObject_GetCharBuf,       // bf_getcharbuffer
#if PY_VERSION_HEX >= 0x02060000
  0,                                   // bf_getbuffer
  0                                    // bf_releasebuffer
#endif
};

//--------------------------------------------------------------------
// Object protocol

static void PyVTKMutableObject_Delete(PyObject *ob)
{
  Py_DECREF(((PyVTKMutableObject *)ob)->value);
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
  PyObject *s = PyObject_Repr(((PyVTKMutableObject *)ob)->value);
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
  return PyObject_Str(((PyVTKMutableObject *)ob)->value);
}

#if PY_VERSION_HEX >= 0x02010000
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
#else
static int PyVTKMutableObject_Compare(PyObject *ob1, PyObject *ob2)
{
  if (PyVTKMutableObject_Check(ob1))
    {
    ob1 = ((PyVTKMutableObject *)ob1)->value;
    }
  if (PyVTKMutableObject_Check(ob2))
    {
    ob2 = ((PyVTKMutableObject *)ob2)->value;
    }
  return PyObject_Compare(ob1, ob2);
}
#endif

static PyObject *PyVTKMutableObject_GetAttr(PyObject *self, PyObject *attr)
{
  char text[128];
  char *name = PyString_AsString(attr);
  PyObject *a;
#if PY_VERSION_HEX < 0x02020000
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
        meth = PyVTKMutableObject_Methods;
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

  for (meth = PyVTKMutableObject_Methods; meth && meth->ml_name; meth++)
    {
    if (strcmp(name, meth->ml_name) == 0)
      {
      return PyCFunction_New(meth, self);
      }
    }
#else
  a = PyObject_GenericGetAttr(self, attr);
  if (a || !PyErr_ExceptionMatches(PyExc_AttributeError))
    {
    return a;
    }
  PyErr_Clear();
#endif

  if (name[0] != '_')
    {
    a = PyObject_GetAttr(((PyVTKMutableObject *)self)->value, attr);

    if (a || !PyErr_ExceptionMatches(PyExc_AttributeError))
      {
      return a;
      }
    PyErr_Clear();
    }

  sprintf(text, "'%.20s' object has no attribute '%.80s'",
          self->ob_type->tp_name, name);
  PyErr_SetString(PyExc_AttributeError, text);
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

  if (PyArg_ParseTuple(args, (char *)"O:mutable", &o))
    {
    o = PyVTKMutableObject_CompatibleObject(o);

    if (o)
      {
#if PY_MAJOR_VERSION >= 2
      PyVTKMutableObject *self = PyObject_New(PyVTKMutableObject, &PyVTKMutableObject_Type);
#else
      PyVTKMutableObject *self = PyObject_NEW(PyVTKMutableObject, &PyVTKMutableObject_Type);
#endif
      self->value = o;
      return (PyObject *)self;
      }
    }

  return NULL;
}

//--------------------------------------------------------------------
PyTypeObject PyVTKMutableObject_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  (char*)"vtk.mutable",                  // tp_name
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
#if PY_VERSION_HEX >= 0x02020000
  Py_TPFLAGS_CHECKTYPES |
#endif
  Py_TPFLAGS_DEFAULT,                    // tp_flags
  (char*)PyVTKMutableObject_Doc,         // tp_doc
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
// Classic new method

#if PY_VERSION_HEX < 0x02020000
static PyObject *PyVTKMutableObject_ClassicNew(PyObject *, PyObject *args)
{
  return PyVTKMutableObject_New(PyVTKMutableObject_Type, args, 0);
}

static PyMethodDef PyVTKMutableObject_NewMethod =
  {(char*)"mutable", PyVTKMutableObject_ClassicNew, 1,
   (char*)PyVTKMutableObject_Doc }
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
