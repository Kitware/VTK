/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonArgs.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
The vtkPythonArgs class was created in Oct 2010 by David Gobbi for VTK 6.0.

This class provides methods for reading an argument tuple from Python
and converting it to types that can be used by VTK.  It is meant to be
more efficient and flexible that the original PyArg_ParseTuple() code,
resulting in wrapper code that is faster and more compact.
-----------------------------------------------------------------------*/

#include "vtkPythonArgs.h"
#include "vtkPythonUtil.h"

#include "vtkObjectBase.h"

//--------------------------------------------------------------------
// Extract various C++ types from python objects.  The rules are
// identical to PyArg_ParseTuple except that range checking is done
// on unsigned values.

// Macro to mimic a check done in PyArg_ParseTuple
#if PY_VERSION_HEX >= 0x02030000
#define VTK_PYTHON_FLOAT_CHECK()\
  if (PyFloat_Check(o) && \
      PyErr_Warn(PyExc_DeprecationWarning, \
                 "integer argument expected, got float")) \
    { \
    return -1; \
    }
#else
#define VTK_PYTHON_FLOAT_CHECK()
#endif

template <class T> inline
int vtkPythonGetIntValue(PyObject *o, T &a)
{
  VTK_PYTHON_FLOAT_CHECK();

  long i = PyInt_AsLong(o);
  a = static_cast<T>(i);
  if (i != -1 || !PyErr_Occurred())
    {
    i = 0;
    }
  return static_cast<int>(i);
}

template <class T> inline
int vtkPythonGetUnsignedIntValue(PyObject *o, T &a)
{
  VTK_PYTHON_FLOAT_CHECK();

  unsigned long i;
#if PY_VERSION_HEX >= 0x02020000 && PY_VERSION_HEX < 0x2040000
  if (PyInt_Check(o))
    {
#endif
#if PY_VERSION_HEX < 0x2040000
    long l = PyInt_AsLong(o);
    if (l < 0)
      {
      PyErr_SetString(PyExc_OverflowError,
                      "can't convert negative value to unsigned long");
      l = -1;
      }
    i = static_cast<unsigned long>(l);
#endif
#if PY_VERSION_HEX >= 0x02020000 && PY_VERSION_HEX < 0x2040000
    }
  else
    {
#endif
#if PY_VERSION_HEX >= 0x2020000
    i = PyLong_AsUnsignedLong(o);
#endif
#if PY_VERSION_HEX >= 0x02020000 && PY_VERSION_HEX < 0x2040000
    }
#endif
  a = static_cast<T>(i);
  if (static_cast<int>(i) != -1 || !PyErr_Occurred())
    {
    i = 0;
    }
  return static_cast<int>(i);
}

template <class T> inline
int vtkPythonGetLongValue(PyObject *o, T &a)
{
  VTK_PYTHON_FLOAT_CHECK();

#ifdef PY_LONG_LONG
  PY_LONG_LONG i = PyLong_AsLongLong(o);
#else
  long i = PyInt_AsLong(o);
#endif
  a = static_cast<T>(i);
  if (i != -1 || !PyErr_Occurred())
    {
    i = 0;
    }
  return static_cast<int>(i);
}

template <class T> inline
int vtkPythonGetUnsignedLongValue(PyObject *o, T &a)
{
  VTK_PYTHON_FLOAT_CHECK();

#ifdef PY_LONG_LONG
  unsigned PY_LONG_LONG i;
#else
  unsigned long i;
#endif
#if PY_VERSION_HEX >= 0x02020000
  if (PyInt_Check(o))
    {
#endif
    long l = PyInt_AsLong(o);
    if (l < 0)
      {
      PyErr_SetString(PyExc_OverflowError,
                      "can't convert negative value to unsigned long");
      l = -1;
      }
#ifdef PY_LONG_LONG
    i = static_cast<unsigned PY_LONG_LONG>(l);
#else
    i = static_cast<unsigned long>(l);
#endif
#if PY_VERSION_HEX >= 0x02020000
    }
  else
    {
#endif
#if PY_VERSION_HEX >= 0x2020000
#ifdef PY_LONG_LONG
    i = PyLong_AsUnsignedLongLong(o);
#else
    i = PyLong_AsUnsignedLong(o);
#endif
#endif
#if PY_VERSION_HEX >= 0x02020000
    }
#endif
  a = static_cast<T>(i);
  if (static_cast<int>(i) != -1 || !PyErr_Occurred())
    {
    i = 0;
    }
  return static_cast<int>(i);
}


template <class T> inline
int vtkPythonGetFloatValue(PyObject *o, T &a)
{
  double d = PyFloat_AsDouble(o);
  a = static_cast<T>(d);
  int i = -1;
  if (d != -1.0 || !PyErr_Occurred())
    {
    i = 0;
    }
  return i;
}

template <class T> inline
int vtkPythonGetStringValue(PyObject *o, T *&a)
{
  T *b = 0;
  int r = -1;
  if (PyString_Check(o))
    {
    r = 0;
    b = PyString_AS_STRING(o);
    }
#ifdef Py_USING_UNICODE
  else if (PyUnicode_Check(o))
    {
    r = 1;
#ifdef _PyUnicode_AsDefaultEncodedString
    PyObject *s = _PyUnicode_AsDefaultEncodedString(o, NULL);
#else
    PyObject *s = PyUnicode_AsEncodedString(o, 0, NULL);
#endif
    if (s)
      {
      r = 0;
      b = PyString_AS_STRING(s);
#ifndef _PyUnicode_AsDefaultEncodedString
      Py_DECREF(s);
#endif
      }
    }
#endif
  a = b;
  return r;
}

//--------------------------------------------------------------------
// Overloaded methods, mostly based on the above templates

int vtkPythonGetValue(PyObject *o, const void *&a)
{
  PyBufferProcs *b = o->ob_type->tp_as_buffer;
  if (b && b->bf_getreadbuffer && b->bf_getsegcount)
    {
    if (b->bf_getsegcount(o, NULL) == 1)
      {
      void *p;
      int s = (int)b->bf_getreadbuffer(o, 0, &p);
      if (s >= 0)
        {
        a = vtkPythonUtil::UnmanglePointer((char *)p, &s, "void_p");
        if (s >= 0)
          {
          return 0;
          }
        if (s == -1)
          {
          char buf[128];
          sprintf(buf, "value is %.80s, required type is void_p", (char *)p);
          PyErr_SetString(PyExc_TypeError, buf);
          }
        else
          {
          PyErr_SetString(PyExc_TypeError, "cannot get a void pointer");
          }
        }
      return -1;
      }
    PyErr_SetString(PyExc_TypeError, "buffer must be single-segment");
    return -1;
    }
  PyErr_SetString(PyExc_TypeError, "object does not have a readable buffer");
  return -1;
}

inline
int vtkPythonGetValue(PyObject *o, void *&a)
{
  // should have an alternate form for non-const "void *" that uses
  // writebuffer instead of readbuffer, but that would break existing code
  const void *b;
  int r = vtkPythonGetValue(o, b);
  a = const_cast<void *>(b);
  return r;
}


inline
int vtkPythonGetValue(PyObject *o, const char *&a)
{
  int r = 0;
  a = NULL;

  if (o != Py_None)
    {
    r = vtkPythonGetStringValue(o, a);
    if (r == 1)
      {
      r = -1;
      PyErr_SetString(PyExc_TypeError, "string or None required");
      }
    }

  return r;
}

inline
int vtkPythonGetValue(PyObject *o, char *&a)
{
  int r = 0;
  a = NULL;

  if (o != Py_None)
    {
    r = vtkPythonGetStringValue(o, a);
    if (r == 1)
      {
      r = -1;
      PyErr_SetString(PyExc_TypeError, "string or None required");
      }
    }

  return r;
}

inline
int vtkPythonGetValue(PyObject *o, vtkStdString &a)
{
  const char *b;
  int r = vtkPythonGetStringValue(o, b);
  if (r == 0)
    {
    a = b;
    }
  else if (r == 1)
    {
    r = -1;
    PyErr_SetString(PyExc_TypeError, "a string is required");
    }
  return r;
}

inline
int vtkPythonGetValue(PyObject *o, vtkUnicodeString &a)
{
#ifdef Py_USING_UNICODE
  int r = -1;
  PyObject *s = PyUnicode_AsUTF8String(o);
  if (s)
    {
    a = vtkUnicodeString::from_utf8(PyString_AS_STRING(s));
    Py_DECREF(s);
    r = 0;
    }
  return r;
#else
  a.clear();
  PyErr_SetString(PyExc_TypeError, "python built without unicode support");
  return -1;
#endif
}

inline
int vtkPythonGetValue(PyObject *o, char &a)
{
  const char *b;
  int r = vtkPythonGetStringValue(o, b);
  if (r == 0 && (b[0] == '\0' || b[1] == '\0'))
    {
    a = b[0];
    }
  else if (r != -1)
    {
    r = -1;
    PyErr_SetString(PyExc_TypeError, "a string of length 1 is required");
    }
  return r;
}

inline
int vtkPythonGetValue(PyObject *o, bool &a)
{
  int i = PyObject_IsTrue(o);
  a = (i != 0);
  return i;
}

inline
int vtkPythonGetValue(PyObject *o, float &a)
{
  return vtkPythonGetFloatValue(o, a);
}

inline
int vtkPythonGetValue(PyObject *o, double &a)
{
  return vtkPythonGetFloatValue(o, a);
}

inline
int vtkPythonGetValue(PyObject *o, signed char &a)
{
  long i = 0;
  int r = vtkPythonGetIntValue(o, i);
  a = static_cast<signed char>(i);
  if (r != -1)
    {
    if (i < VTK_SIGNED_CHAR_MIN || i > VTK_SIGNED_CHAR_MAX)
      {
      PyErr_SetString(PyExc_OverflowError,
                      "value is out of range for signed char");
      r = -1;
      }
    }
  return r;
}

inline
int vtkPythonGetValue(PyObject *o, unsigned char &a)
{
  long i = 0;
  int r = vtkPythonGetIntValue(o, i);
  a = static_cast<unsigned char>(i);
  if (r != -1)
    {
    if (i < VTK_UNSIGNED_CHAR_MIN || i > VTK_UNSIGNED_CHAR_MAX)
      {
      PyErr_SetString(PyExc_OverflowError,
                      "value is out of range for unsigned char");
      r = -1;
      }
    }
  return r;
}

inline
int vtkPythonGetValue(PyObject *o, short &a)
{
  long i = 0;
  int r = vtkPythonGetIntValue(o, i);
  a = static_cast<short>(i);
  if (r != -1)
    {
    if (i < VTK_SHORT_MIN || i > VTK_SHORT_MAX)
      {
      PyErr_SetString(PyExc_OverflowError,
                      "value is out of range for short");
      r = -1;
      }
    }
  return r;
}

inline
int vtkPythonGetValue(PyObject *o, unsigned short &a)
{
  long i = 0;
  int r = vtkPythonGetIntValue(o, i);
  a = static_cast<unsigned short>(i);
  if (r != -1)
    {
    if (i < VTK_UNSIGNED_SHORT_MIN || i > VTK_UNSIGNED_SHORT_MAX)
      {
      PyErr_SetString(PyExc_OverflowError,
                      "value is out of range for unsigned short");
      r = -1;
      }
    }
  return r;
}

inline
int vtkPythonGetValue(PyObject *o, int &a)
{
  long i = 0;
  int r = vtkPythonGetIntValue(o, i);
  a = static_cast<int>(i);
#if VTK_SIZEOF_INT < VTK_SIZEOF_LONG
  if (r != -1)
    {
    if (i < VTK_INT_MIN || i > VTK_INT_MAX)
      {
      PyErr_SetString(PyExc_OverflowError,
                      "value is out of range for int");
      r = -1;
      }
    }
#endif
  return r;
}

inline
int vtkPythonGetValue(PyObject *o, unsigned int &a)
{
#if VTK_SIZEOF_INT < VTK_SIZEOF_LONG
  long i = 0;
  int r = vtkPythonGetIntValue(o, i);
  a = static_cast<unsigned int>(i);
  if (r != -1)
    {
    if (i < VTK_UNSIGNED_INT_MIN || i > VTK_UNSIGNED_INT_MAX)
      {
      PyErr_SetString(PyExc_OverflowError,
                      "value is out of range for unsigned int");
      r = -1;
      }
    }
#else
  unsigned long i = 0;
  int r = vtkPythonGetUnsignedIntValue(o, i);
  a = static_cast<unsigned int>(i);
#endif
  return r;
}

inline
int vtkPythonGetValue(PyObject *o, long &a)
{
  return vtkPythonGetIntValue(o, a);
}

inline
int vtkPythonGetValue(PyObject *o, unsigned long &a)
{
  return vtkPythonGetUnsignedIntValue(o, a);
}

#ifdef VTK_TYPE_USE_LONG_LONG
inline
int vtkPythonGetValue(PyObject *o, long long &a)
{
  return vtkPythonGetLongValue(o, a);
}

inline
int vtkPythonGetValue(PyObject *o, unsigned long long &a)
{
  return vtkPythonGetUnsignedLongValue(o, a);
}
#endif

#ifdef VTK_TYPE_USE___INT64
inline
int vtkPythonGetValue(PyObject *o, __int64 &a)
{
  return vtkPythonGetLongValue(o, a);
}

inline
int vtkPythonGetValue(PyObject *o, unsigned __int64 &a)
{
  return vtkPythonGetUnsignedLongValue(o, i);
}
#endif


//--------------------------------------------------------------------
// Method for setting a C++ array from a Python sequence.

static
int vtkPythonSequenceError(PyObject *o, Py_ssize_t n, Py_ssize_t m);

template<class T> inline
int vtkPythonGetArray(PyObject *o, T *a, int n)
{
  if (a)
    {
    Py_ssize_t m = n;

    if (PyTuple_Check(o))
      {
      m = PyTuple_GET_SIZE(o);
      if (m == n)
        {
        int r = 0;
        for (int i = 0; i < n && r != -1; i++)
          {
          PyObject *s = PyTuple_GET_ITEM(o, i);
          r = vtkPythonGetValue(s, a[i]);
          }
        return r;
        }
      }
    else if (PyList_Check(o))
      {
      m = PyList_GET_SIZE(o);
      if (m == n)
        {
        int r = 0;
        for (int i = 0; i < n && r != -1; i++)
          {
          PyObject *s = PyList_GET_ITEM(o, i);
          r = vtkPythonGetValue(s, a[i]);
          }
        return r;
        }
      }
    else if (PySequence_Check(o))
      {
#if PY_MAJOR_VERSION >= 2
      m = PySequence_Size(o);
#else
      m = PySequence_Length(o);
#endif
      if (m == n)
        {
        int r = 0;
        for (int i = 0; i < n && r != -1; i++)
          {
          r = -1;
          PyObject *s = PySequence_GetItem(o, i);
          if (s && vtkPythonGetValue(s, a[i]) != -1)
            {
            Py_DECREF(s);
            r = 0;
            }
          }
        return r;
        }
      }

    return vtkPythonSequenceError(o, n, m);
    }

  return 0;
}

//--------------------------------------------------------------------
// Method for setting an n-dimensional C++ arrays from a Python sequence.

template<class T>
int vtkPythonGetNArray(PyObject *o, T *a, int ndim, const int *dims)
{
  if (a)
    {
    int inc = 1;
    for (int j = 1; j < ndim; j++)
      {
      inc *= dims[j];
      }

    int n = dims[0];
    Py_ssize_t m = n;

    if (PyList_Check(o))
      {
      m = PyList_GET_SIZE(o);
      if (m == n)
        {
        int r = 0;
        if (ndim > 1)
          {
          for (int i = 0; i < n && r != -1; i++)
            {
            PyObject *s = PyList_GET_ITEM(o, i);
            r = vtkPythonGetNArray(s, a, ndim-1, dims+1);
            a += inc;
            }
          }
        else
          {
          for (int i = 0; i < n && r != -1; i++)
            {
            PyObject *s = PyList_GET_ITEM(o, i);
            r = vtkPythonGetValue(s, a[i]);
            }
          }
        return r;
        }
      }
    else if (PySequence_Check(o))
      {
#if PY_MAJOR_VERSION >= 2
      m = PySequence_Size(o);
#else
      m = PySequence_Length(o);
#endif
      if (m == n)
        {
        int r = 0;
        for (int i = 0; i < n && r != -1; i++)
          {
          r = -1;
          PyObject *s = PySequence_GetItem(o, i);
          if (s)
            {
            if (ndim > 1)
              {
              r = vtkPythonGetNArray(s, a, ndim-1, dims+1);
              a += inc;
              }
            else
              {
              r = vtkPythonGetValue(s, a[i]);
              }
            Py_DECREF(s);
            }
          }
        return r;
        }
      }

    return vtkPythonSequenceError(o, n, m);
    }

  return 0;
}

//--------------------------------------------------------------------
// Method for setting a python sequence from a C++ array

template<class T> inline
int vtkPythonSetArray(PyObject *o, const T *a, int n)
{
  if (a)
    {
    Py_ssize_t m = n;

    if (PyList_Check(o))
      {
      m = PyList_GET_SIZE(o);
      if (m == n)
        {
        int r = 0;
        for (int i = 0; i < n && r != -1; i++)
          {
          r = -1;
          PyObject *s = vtkPythonArgs::BuildValue(a[i]);
          if (s)
            {
            PyList_SET_ITEM(o, i, s);
            r = 0;
            }
          }
        return r;
        }
      }
    else if (PySequence_Check(o))
      {
#if PY_MAJOR_VERSION >= 2
      m = PySequence_Size(o);
#else
      m = PySequence_Length(o);
#endif
      if (m == n)
        {
        int r = 0;
        for (int i = 0; i < n && r != -1; i++)
          {
          r = -1;
          PyObject *s = vtkPythonArgs::BuildValue(a[i]);
          if (s)
            {
            r = PySequence_SetItem(o, i, s);
            Py_DECREF(s);
            }
          }
        return r;
        }
      }

    return vtkPythonSequenceError(o, n, m);
    }

  return 0;
}

//--------------------------------------------------------------------
// Method for setting a python array from an n-dimensional C++ array

template<class T>
int vtkPythonSetNArray(
  PyObject *o, const T *a, int ndim, const int *dims)
{
  if (a)
    {
    int inc = 1;
    for (int j = 1; j < ndim; j++)
      {
      inc *= dims[j];
      }

    int n = dims[0];
    Py_ssize_t m = n;

    if (PyList_Check(o))
      {
      m = PyList_GET_SIZE(o);
      if (m == n)
        {
        int r = 0;
        if (ndim > 1)
          {
          for (int i = 0; i < n && r != -1; i++)
            {
            PyObject *s = PyList_GET_ITEM(o, i);
            r = vtkPythonSetNArray(s, a, ndim-1, dims+1);
            a += inc;
            }
          }
        else
          {
          for (int i = 0; i < n && r != -1; i++)
            {
            r = -1;
            PyObject *s = vtkPythonArgs::BuildValue(a[i]);
            if (s)
              {
              PyList_SET_ITEM(o, i, s);
              r = 0;
              }
            }
          }
        return r;
        }
      }
    else if (PySequence_Check(o))
      {
#if PY_MAJOR_VERSION >= 2
      m = PySequence_Size(o);
#else
      m = PySequence_Length(o);
#endif
      if (m == n)
        {
        int r = 0;
        if (ndim > 1)
          {
          for (int i = 0; i < n && r != -1; i++)
            {
            r = -1;
            PyObject *s = PySequence_GetItem(o, i);
            if (s)
              {
              r = vtkPythonSetNArray(s, a, ndim-1, dims+1);
              a += inc;
              Py_DECREF(s);
              }
            }
          }
        else
          {
          for (int i = 0; i < n && r != -1; i++)
            {
            r = -1;
            PyObject *s = vtkPythonArgs::BuildValue(a[i]);
            if (s)
              {
              r = PySequence_SetItem(o, i, s);
              Py_DECREF(s);
              }
            }
          }
        return r;
        }
      }

    return vtkPythonSequenceError(o, n, m);
    }

  return 0;
}

//--------------------------------------------------------------------
// Define all the "BuildValue" array methods defined in the class.

template<class T> inline
PyObject *vtkPythonBuildTuple(const T *a, int n)
{
  if (a)
    {
    PyObject *t = PyTuple_New(n);
    for (int i = 0; i < n; i++)
      {
      PyObject *o = vtkPythonArgs::BuildValue(a[i]);
      PyTuple_SET_ITEM(t, i, o);
      }
    return t;
    }

  Py_INCREF(Py_None);
  return Py_None;
}

#define VTK_PYTHON_BUILD_TUPLE(T) \
PyObject *vtkPythonArgs::BuildTuple(const T *a, int n) \
{ \
  return vtkPythonBuildTuple(a, n); \
}

VTK_PYTHON_BUILD_TUPLE(bool)
VTK_PYTHON_BUILD_TUPLE(float)
VTK_PYTHON_BUILD_TUPLE(double)
VTK_PYTHON_BUILD_TUPLE(signed char)
VTK_PYTHON_BUILD_TUPLE(unsigned char)
VTK_PYTHON_BUILD_TUPLE(short)
VTK_PYTHON_BUILD_TUPLE(unsigned short)
VTK_PYTHON_BUILD_TUPLE(int)
VTK_PYTHON_BUILD_TUPLE(unsigned int)
VTK_PYTHON_BUILD_TUPLE(long)
VTK_PYTHON_BUILD_TUPLE(unsigned long)
#ifdef VTK_TYPE_USE_LONG_LONG
VTK_PYTHON_BUILD_TUPLE(long long)
VTK_PYTHON_BUILD_TUPLE(unsigned long long)
#endif
#ifdef VTK_TYPE_USE___INT64
VTK_PYTHON_BUILD_TUPLE(__int64)
VTK_PYTHON_BUILD_TUPLE(unsigned __int64)
#endif

//--------------------------------------------------------------------
// If "self" is a class, get real "self" from arg list
vtkObjectBase *vtkPythonArgs::GetSelfFromFirstArg(
  PyObject *self, PyObject *args)
{
 if (PyVTKClass_Check(self))
    {
    PyVTKClass *vtkclass = (PyVTKClass *)self;
    const char *classname = PyString_AS_STRING(vtkclass->vtk_name);

    if (PyTuple_GET_SIZE(args) > 0)
      {
      self = PyTuple_GET_ITEM(args, 0);
      if (PyVTKObject_Check(self))
        {
        vtkObjectBase *vtkself = ((PyVTKObject *)self)->vtk_ptr;
        if (vtkself->IsA(classname))
          {
          return vtkself;
          }
        }
      }

    char buf[256];
    sprintf(buf, "unbound method requires a %.200s as the first argument",
            classname);
    PyErr_SetString(PyExc_TypeError, buf);
    return NULL;
    }

  PyErr_SetString(PyExc_TypeError, "unbound method requires a vtkobject");
  return NULL;
}

//--------------------------------------------------------------------
// Define the GetArg methods for getting objects

vtkObjectBase *vtkPythonArgs::GetArgAsVTKObject(
  const char *classname, bool &valid)
{
  PyObject *o = PyTuple_GET_ITEM(this->Args, this->I++);
  vtkObjectBase *r = vtkPythonUtil::GetPointerFromObject(o, classname);
  if (r || o == Py_None)
    {
    valid = true;
    return r;
    }
  this->RefineArgTypeError(this->I - this->M - 1);
  valid = false;
  return r;
}

void *vtkPythonArgs::GetArgAsSpecialObject(const char *classname, PyObject **p)
{
  PyObject *o = PyTuple_GET_ITEM(this->Args, this->I++);
  void *r = vtkPythonUtil::GetPointerFromSpecialObject(o, classname, p);
  if (r)
    {
    return r;
    }
  this->RefineArgTypeError(this->I - this->M - 1);
  return r;
}

int vtkPythonArgs::GetArgAsEnum(const char *, bool &valid)
{
  PyObject *o = PyTuple_GET_ITEM(this->Args, this->I++);

  // should check enum type for validity
  int i;
  if (vtkPythonGetValue(o, i) != -1)
    {
    valid = true;
    return i;
    }
  this->RefineArgTypeError(this->I - this->M - 1);
  valid = false;
  return 0;
}


//--------------------------------------------------------------------
// Define the methods for SIP objects

void *vtkPythonArgs::GetArgAsSIPObject(const char *classname)
{
  PyObject *o = PyTuple_GET_ITEM(this->Args, this->I++);
  void *r = vtkPythonUtil::SIPGetPointerFromObject(o, classname);
  if (r)
    {
    return r;
    }
  if (PyErr_Occurred())
    {
    this->RefineArgTypeError(this->I - this->M - 1);
    }
  return r;
}

int vtkPythonArgs::GetArgAsSIPEnum(const char *, bool &valid)
{
  PyObject *o = PyTuple_GET_ITEM(this->Args, this->I++);

  // should check enum type for validity
  int i;
  if (vtkPythonGetValue(o, i) != -1)
    {
    valid = true;
    return i;
    }
  this->RefineArgTypeError(this->I - this->M - 1);
  valid = false;
  return 0;
}

//--------------------------------------------------------------------
// Define all the "GetValue" methods in the class.

#define VTK_PYTHON_GET_ARG(T) \
bool vtkPythonArgs::GetValue(T &a) \
{ \
  PyObject *o = PyTuple_GET_ITEM(this->Args, this->I++); \
  if (vtkPythonGetValue(o, a) != -1) \
    { \
    return true; \
    } \
  this->RefineArgTypeError(this->I - this->M - 1); \
  return false; \
}

VTK_PYTHON_GET_ARG(void *)
VTK_PYTHON_GET_ARG(const void *)
VTK_PYTHON_GET_ARG(char *)
VTK_PYTHON_GET_ARG(const char *)
VTK_PYTHON_GET_ARG(vtkStdString)
VTK_PYTHON_GET_ARG(vtkUnicodeString)
VTK_PYTHON_GET_ARG(char)
VTK_PYTHON_GET_ARG(bool)
VTK_PYTHON_GET_ARG(float)
VTK_PYTHON_GET_ARG(double)
VTK_PYTHON_GET_ARG(signed char)
VTK_PYTHON_GET_ARG(unsigned char)
VTK_PYTHON_GET_ARG(short)
VTK_PYTHON_GET_ARG(unsigned short)
VTK_PYTHON_GET_ARG(int)
VTK_PYTHON_GET_ARG(unsigned int)
VTK_PYTHON_GET_ARG(long)
VTK_PYTHON_GET_ARG(unsigned long)
#ifdef VTK_TYPE_USE_LONG_LONG
VTK_PYTHON_GET_ARG(long long)
VTK_PYTHON_GET_ARG(unsigned long long)
#endif
#ifdef VTK_TYPE_USE___INT64
VTK_PYTHON_GET_ARG(__int64)
VTK_PYTHON_GET_ARG(unsigned __int64)
#endif

//--------------------------------------------------------------------
// Define all the GetArray methods in the class.

#define VTK_PYTHON_GET_ARRAY_ARG(T) \
bool vtkPythonArgs::GetArray(T *a, int n) \
{ \
  PyObject *o = PyTuple_GET_ITEM(this->Args, this->I++); \
  if (vtkPythonGetArray(o, a, n) != -1) \
    { \
    return true; \
    } \
  this->RefineArgTypeError(this->I - this->M - 1); \
  return false; \
}

VTK_PYTHON_GET_ARRAY_ARG(bool)
VTK_PYTHON_GET_ARRAY_ARG(float)
VTK_PYTHON_GET_ARRAY_ARG(double)
VTK_PYTHON_GET_ARRAY_ARG(signed char)
VTK_PYTHON_GET_ARRAY_ARG(unsigned char)
VTK_PYTHON_GET_ARRAY_ARG(short)
VTK_PYTHON_GET_ARRAY_ARG(unsigned short)
VTK_PYTHON_GET_ARRAY_ARG(int)
VTK_PYTHON_GET_ARRAY_ARG(unsigned int)
VTK_PYTHON_GET_ARRAY_ARG(long)
VTK_PYTHON_GET_ARRAY_ARG(unsigned long)
#ifdef VTK_TYPE_USE_LONG_LONG
VTK_PYTHON_GET_ARRAY_ARG(long long)
VTK_PYTHON_GET_ARRAY_ARG(unsigned long long)
#endif
#ifdef VTK_TYPE_USE___INT64
VTK_PYTHON_GET_ARRAY_ARG(__int64)
VTK_PYTHON_GET_ARRAY_ARG(unsigned __int64)
#endif

//--------------------------------------------------------------------
// Define all the GetNArray methods in the class.

#define VTK_PYTHON_GET_NARRAY_ARG(T) \
bool vtkPythonArgs::GetNArray(T *a, int ndim, const int *dims) \
{ \
  PyObject *o = PyTuple_GET_ITEM(this->Args, this->I++); \
  if (vtkPythonGetNArray(o, a, ndim, dims) != -1) \
    { \
    return true; \
    } \
  this->RefineArgTypeError(this->I - this->M - 1); \
  return false; \
}

VTK_PYTHON_GET_NARRAY_ARG(bool)
VTK_PYTHON_GET_NARRAY_ARG(float)
VTK_PYTHON_GET_NARRAY_ARG(double)
VTK_PYTHON_GET_NARRAY_ARG(signed char)
VTK_PYTHON_GET_NARRAY_ARG(unsigned char)
VTK_PYTHON_GET_NARRAY_ARG(short)
VTK_PYTHON_GET_NARRAY_ARG(unsigned short)
VTK_PYTHON_GET_NARRAY_ARG(int)
VTK_PYTHON_GET_NARRAY_ARG(unsigned int)
VTK_PYTHON_GET_NARRAY_ARG(long)
VTK_PYTHON_GET_NARRAY_ARG(unsigned long)
#ifdef VTK_TYPE_USE_LONG_LONG
VTK_PYTHON_GET_NARRAY_ARG(long long)
VTK_PYTHON_GET_NARRAY_ARG(unsigned long long)
#endif
#ifdef VTK_TYPE_USE___INT64
VTK_PYTHON_GET_NARRAY_ARG(__int64)
VTK_PYTHON_GET_NARRAY_ARG(unsigned __int64)
#endif

//--------------------------------------------------------------------
// Define the special function pointer GetNextArg method

bool vtkPythonArgs::GetFunction(PyObject *&o)
{
  o = PyTuple_GET_ITEM(this->Args, this->I++);
  if (o == Py_None || PyCallable_Check(o))
    {
    return true;
    }
  PyErr_SetString(PyExc_TypeError, "a callable object is required");
  return false;
}

//--------------------------------------------------------------------
// Define all the SetArgValue methods for setting reference args

#define VTK_PYTHON_SET_ARG(T) \
bool vtkPythonArgs::SetArgValue(int i, T a) \
{ \
  if (this->M + i < this->N) \
    { \
    PyObject *m = PyTuple_GET_ITEM(this->Args, this->M + i); \
    PyObject *o = vtkPythonArgs::BuildValue(a); \
    int r = PyVTKMutableObject_SetValue(m, o); \
    if (r == 0) \
      { \
      return true; \
      } \
    this->RefineArgTypeError(i); \
    return false; \
    } \
  return true; \
}

VTK_PYTHON_SET_ARG(const vtkStdString &)
VTK_PYTHON_SET_ARG(const vtkUnicodeString &)
VTK_PYTHON_SET_ARG(char)
VTK_PYTHON_SET_ARG(bool)
VTK_PYTHON_SET_ARG(float)
VTK_PYTHON_SET_ARG(double)
VTK_PYTHON_SET_ARG(signed char)
VTK_PYTHON_SET_ARG(unsigned char)
VTK_PYTHON_SET_ARG(short)
VTK_PYTHON_SET_ARG(unsigned short)
VTK_PYTHON_SET_ARG(int)
VTK_PYTHON_SET_ARG(unsigned int)
VTK_PYTHON_SET_ARG(long)
VTK_PYTHON_SET_ARG(unsigned long)
#ifdef VTK_TYPE_USE_LONG_LONG
VTK_PYTHON_SET_ARG(long long)
VTK_PYTHON_SET_ARG(unsigned long long)
#endif
#ifdef VTK_TYPE_USE___INT64
VTK_PYTHON_SET_ARG(__int64)
VTK_PYTHON_SET_ARG(unsigned __int64)
#endif

//--------------------------------------------------------------------
// Define all the SetArgValue methods for setting array args

#define VTK_PYTHON_SET_ARRAY_ARG(T) \
bool vtkPythonArgs::SetArray(int i, const T *a, int n) \
{ \
  if (this->M + i < this->N) \
    { \
    PyObject *o = PyTuple_GET_ITEM(this->Args, this->M + i); \
    int r = vtkPythonSetArray(o, a, n); \
    if (r == -1) \
      { \
      this->RefineArgTypeError(i); \
      return false; \
      } \
    } \
  return true; \
}

VTK_PYTHON_SET_ARRAY_ARG(bool)
VTK_PYTHON_SET_ARRAY_ARG(float)
VTK_PYTHON_SET_ARRAY_ARG(double)
VTK_PYTHON_SET_ARRAY_ARG(signed char)
VTK_PYTHON_SET_ARRAY_ARG(unsigned char)
VTK_PYTHON_SET_ARRAY_ARG(short)
VTK_PYTHON_SET_ARRAY_ARG(unsigned short)
VTK_PYTHON_SET_ARRAY_ARG(int)
VTK_PYTHON_SET_ARRAY_ARG(unsigned int)
VTK_PYTHON_SET_ARRAY_ARG(long)
VTK_PYTHON_SET_ARRAY_ARG(unsigned long)
#ifdef VTK_TYPE_USE_LONG_LONG
VTK_PYTHON_SET_ARRAY_ARG(long long)
VTK_PYTHON_SET_ARRAY_ARG(unsigned long long)
#endif
#ifdef VTK_TYPE_USE___INT64
VTK_PYTHON_SET_ARRAY_ARG(__int64)
VTK_PYTHON_SET_ARRAY_ARG(unsigned __int64)
#endif

//--------------------------------------------------------------------
// Define all the SetArgValue methods for setting multi-dim array args

#define VTK_PYTHON_SET_NARRAY_ARG(T) \
bool vtkPythonArgs::SetNArray( \
  int i, const T *a, int ndim, const int *dims) \
{ \
  if (this->M + i < this->N) \
    { \
    PyObject *o = PyTuple_GET_ITEM(this->Args, this->M + i); \
    int r = vtkPythonSetNArray(o, a, ndim, dims); \
    if (r != -1) \
      { \
      return true; \
      } \
    this->RefineArgTypeError(i); \
    return false; \
    } \
  return true; \
}

VTK_PYTHON_SET_NARRAY_ARG(bool)
VTK_PYTHON_SET_NARRAY_ARG(float)
VTK_PYTHON_SET_NARRAY_ARG(double)
VTK_PYTHON_SET_NARRAY_ARG(signed char)
VTK_PYTHON_SET_NARRAY_ARG(unsigned char)
VTK_PYTHON_SET_NARRAY_ARG(short)
VTK_PYTHON_SET_NARRAY_ARG(unsigned short)
VTK_PYTHON_SET_NARRAY_ARG(int)
VTK_PYTHON_SET_NARRAY_ARG(unsigned int)
VTK_PYTHON_SET_NARRAY_ARG(long)
VTK_PYTHON_SET_NARRAY_ARG(unsigned long)
#ifdef VTK_TYPE_USE_LONG_LONG
VTK_PYTHON_SET_NARRAY_ARG(long long)
VTK_PYTHON_SET_NARRAY_ARG(unsigned long long)
#endif
#ifdef VTK_TYPE_USE___INT64
VTK_PYTHON_SET_NARRAY_ARG(__int64)
VTK_PYTHON_SET_NARRAY_ARG(unsigned __int64)
#endif

//--------------------------------------------------------------------
// Raise an exception about incorrect arg count.
void vtkPythonArgs::ArgCountError(int m, int n)
{
  char text[256];
  const char *name = this->MethodName;
  int nargs = this->N;

  sprintf(text, "%.200s%s takes %s %d argument%s (%d given)",
          (name ? name : "function"), (name ? "()" : ""),
          ((m == n) ? "exactly" : ((nargs < m) ? "at least" : "at most")),
          ((nargs < m) ? m : n),
          ((((nargs < m) ? m : n)) == 1 ? "" : "s"),
          nargs);
  PyErr_SetString(PyExc_TypeError, text);
}

//--------------------------------------------------------------------
// Static method to write an arg count error.
void vtkPythonArgs::ArgCountError(int n, const char *name)
{
  char text[256];

  sprintf(text, "no overloads of %.200s%s take %d argument%s",
          (name ? name : "function"), (name ? "()" : ""),
          n, (n == 1 ? "" : "s"));
  PyErr_SetString(PyExc_TypeError, text);
}

//--------------------------------------------------------------------
// Raise an exception about pure virtual method call
void vtkPythonArgs::PureVirtualError()
{
  char text[256];

  sprintf(text, "pure virtual method %.200s() was called",
          this->MethodName);
  PyErr_SetString(PyExc_TypeError, text);
}

//--------------------------------------------------------------------
// Refine an error by saying what argument it is for
void vtkPythonArgs::RefineArgTypeError(int i)
{
  if (PyErr_ExceptionMatches(PyExc_TypeError) ||
      PyErr_ExceptionMatches(PyExc_ValueError) ||
      PyErr_ExceptionMatches(PyExc_OverflowError))
    {
    PyObject *exc;
    PyObject *val;
    PyObject *frame;
    char text[256];
    const char *cp = "";

    PyErr_Fetch(&exc, &val, &frame);
    if (val && PyString_Check(val))
      {
      cp = PyString_AsString(val);
      }
    sprintf(text, "argument %d: %.200s", i+1, cp);
    Py_XDECREF(val);
    val = PyString_FromString(text);
    PyErr_Restore(exc, val, frame);
    }
}

//--------------------------------------------------------------------
// Raise a type error for a sequence arg of wrong type or size.
int vtkPythonSequenceError(PyObject *o, Py_ssize_t n, Py_ssize_t m)
{
  char text[80];
  if (m == n)
    {
    sprintf(text, "expected a sequence of %ld value%s, got %s",
            (long)n, ((n == 1) ? "" : "s"), o->ob_type->tp_name);
    }
  else
    {
    sprintf(text, "expected a sequence of %ld value%s, got %ld values",
            (long)n, ((n == 1) ? "" : "s"), (long)m);
    }
  PyErr_SetString(PyExc_TypeError, text);
  return -1;
}
