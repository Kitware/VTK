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
The vtkPythonArgs class was created in Oct 2010 by David Gobbi.

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
  if (PyFloat_Check(o)) \
    { \
    PyErr_SetString(PyExc_TypeError, \
                      (char *)"integer argument expected, got float"); \
    return false; \
    }
#else
#define VTK_PYTHON_FLOAT_CHECK()
#endif

inline
bool vtkPythonGetValue(PyObject *o, long &a)
{
  VTK_PYTHON_FLOAT_CHECK();

  a = PyInt_AsLong(o);
  return (a != -1 || !PyErr_Occurred());
}

inline
bool vtkPythonGetValue(PyObject *o, unsigned long &a)
{
  VTK_PYTHON_FLOAT_CHECK();

#if PY_VERSION_HEX >= 0x02020000 && PY_VERSION_HEX < 0x2040000
  if (PyInt_Check(o))
    {
#endif
#if PY_VERSION_HEX < 0x2040000
    long l = PyInt_AsLong(o);
    if (l < 0)
      {
      PyErr_SetString(PyExc_OverflowError,
                      "can't convert negative value to unsigned integer");
      return false;
      }
    a = static_cast<unsigned long>(l);
#endif
#if PY_VERSION_HEX >= 0x02020000 && PY_VERSION_HEX < 0x2040000
    }
  else
    {
#endif
#if PY_VERSION_HEX >= 0x2020000
    a = PyLong_AsUnsignedLong(o);
#endif
#if PY_VERSION_HEX >= 0x02020000 && PY_VERSION_HEX < 0x2040000
    }
#endif
  return (static_cast<long>(a) != -1 || !PyErr_Occurred());
}

template <class T> inline
bool vtkPythonGetLongLongValue(PyObject *o, T &a)
{
  VTK_PYTHON_FLOAT_CHECK();

#ifdef PY_LONG_LONG
  PY_LONG_LONG i = PyLong_AsLongLong(o);
#else
  long i = PyInt_AsLong(o);
#endif
  a = static_cast<T>(i);
  return (i != -1 || !PyErr_Occurred());
}

template <class T> inline
bool vtkPythonGetUnsignedLongLongValue(PyObject *o, T &a)
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
      return false;
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
  return (static_cast<int>(i) != -1 || !PyErr_Occurred());
}


template <class T> inline
bool vtkPythonGetStringValue(PyObject *o, T *&a, const char *exctext)
{
  if (PyString_Check(o))
    {
    a = PyString_AS_STRING(o);
    return true;
    }
#ifdef Py_USING_UNICODE
  else if (PyUnicode_Check(o))
    {
#ifdef _PyUnicode_AsDefaultEncodedString
    PyObject *s = _PyUnicode_AsDefaultEncodedString(o, NULL);
#else
    PyObject *s = PyUnicode_AsEncodedString(o, 0, NULL);
#endif
    if (s)
      {
      a = PyString_AS_STRING(s);
#ifndef _PyUnicode_AsDefaultEncodedString
      Py_DECREF(s);
#endif
      return true;
      }

    exctext = "(unicode conversion error)";
    }
#endif

  PyErr_SetString(PyExc_TypeError, exctext);
  return false;
}

inline bool vtkPythonGetStdStringValue(PyObject *o, std::string &a, const char *exctext)
{
  if (PyString_Check(o))
    {
    char* val;
    Py_ssize_t len;
    PyString_AsStringAndSize(o, &val, &len);
    a = std::string(val, len);
    return true;
    }
#ifdef Py_USING_UNICODE
  else if (PyUnicode_Check(o))
    {
#ifdef _PyUnicode_AsDefaultEncodedString
    PyObject *s = _PyUnicode_AsDefaultEncodedString(o, NULL);
#else
    PyObject *s = PyUnicode_AsEncodedString(o, 0, NULL);
#endif
    if (s)
      {
      char* val;
      Py_ssize_t len;
      PyString_AsStringAndSize(s, &val, &len);
      a = std::string(val, len);
#ifndef _PyUnicode_AsDefaultEncodedString
      Py_DECREF(s);
#endif
      return true;
      }

    exctext = "(unicode conversion error)";
    }
#endif

  PyErr_SetString(PyExc_TypeError, exctext);
  return false;
}

//--------------------------------------------------------------------
// Overloaded methods, mostly based on the above templates

static bool vtkPythonGetValue(PyObject *o, const void *&a)
{
  PyBufferProcs *b = o->ob_type->tp_as_buffer;
  if (b && b->bf_getreadbuffer && b->bf_getsegcount)
    {
    if (b->bf_getsegcount(o, NULL) == 1)
      {
      void *p;
      Py_ssize_t sz = b->bf_getreadbuffer(o, 0, &p);
      if (sz >= 0 && sz <= VTK_INT_MAX)
        {
        // check for pointer mangled as string
        int s = static_cast<int>(sz);
        a = vtkPythonUtil::UnmanglePointer(
          reinterpret_cast<char *>(p), &s, "p_void");
        if (s >= 0)
          {
          return true;
          }
        if (s == -1)
          {
          char buf[128];
          sprintf(buf, "value is %.80s, required type is p_void",
            reinterpret_cast<char *>(p));
          PyErr_SetString(PyExc_TypeError, buf);
          }
        else
          {
          PyErr_SetString(PyExc_TypeError, "cannot get a void pointer");
          }
        }
      else if (sz >= 0)
        {
        // directly use the pointer to the buffer contents
        a = p;
        return true;
        }
      return false;
      }
    PyErr_SetString(PyExc_TypeError, "buffer must be single-segment");
    return false;
    }
  PyErr_SetString(PyExc_TypeError, "object does not have a readable buffer");
  return false;
}

inline
bool vtkPythonGetValue(PyObject *o, void *&a)
{
  // should have an alternate form for non-const "void *" that uses
  // writebuffer instead of readbuffer, but that would break existing code
  const void *b = NULL;
  bool r = vtkPythonGetValue(o, b);
  a = const_cast<void *>(b);
  return r;
}


inline
bool vtkPythonGetValue(PyObject *o, const char *&a)
{
  a = NULL;

  return (o == Py_None ||
          vtkPythonGetStringValue(o, a, "string or None required"));
}

inline
bool vtkPythonGetValue(PyObject *o, char *&a)
{
  a = NULL;

  return (o == Py_None ||
          vtkPythonGetStringValue(o, a, "string or None required"));
}

inline
bool vtkPythonGetValue(PyObject *o, std::string &a)
{
  if (vtkPythonGetStdStringValue(o, a, "string is required"))
    {
    return true;
    }
  return false;
}

inline
bool vtkPythonGetValue(PyObject *o, vtkUnicodeString &a)
{
#ifdef Py_USING_UNICODE
  PyObject *s = PyUnicode_AsUTF8String(o);
  if (s)
    {
    a = vtkUnicodeString::from_utf8(PyString_AS_STRING(s));
    Py_DECREF(s);
    return true;
    }
  return false;
#else
  a.clear();
  PyErr_SetString(PyExc_TypeError, "python built without unicode support");
  return false;
#endif
}

inline
bool vtkPythonGetValue(PyObject *o, char &a)
{
  static const char exctext[] = "a string of length 1 is required";
  const char *b;
  if (vtkPythonGetStringValue(o, b, exctext))
    {
    if (b[0] == '\0' || b[1] == '\0')
      {
      a = b[0];
      return true;
      }
    PyErr_SetString(PyExc_TypeError, exctext);
    }
  return false;
}

inline
bool vtkPythonGetValue(PyObject *o, bool &a)
{
  int i = PyObject_IsTrue(o);
  a = (i != 0);
  return (i != -1);
}

inline
bool vtkPythonGetValue(PyObject *o, float &a)
{
  a = static_cast<float>(PyFloat_AsDouble(o));
  return (a != -1.0f || !PyErr_Occurred());
}

inline
bool vtkPythonGetValue(PyObject *o, double &a)
{
  a = PyFloat_AsDouble(o);
  return (a != -1.0f || !PyErr_Occurred());
}

inline
bool vtkPythonGetValue(PyObject *o, signed char &a)
{
  long i = 0;
  if (vtkPythonGetValue(o, i))
    {
    a = static_cast<signed char>(i);
    if (i >= VTK_SIGNED_CHAR_MIN && i <= VTK_SIGNED_CHAR_MAX)
      {
      return true;
      }
    PyErr_SetString(PyExc_OverflowError,
                    "value is out of range for signed char");
    }
  return false;
}

inline
bool vtkPythonGetValue(PyObject *o, unsigned char &a)
{
  long i = 0;
  if (vtkPythonGetValue(o, i))
    {
    a = static_cast<unsigned char>(i);
    if (i >= VTK_UNSIGNED_CHAR_MIN && i <= VTK_UNSIGNED_CHAR_MAX)
      {
      return true;
      }
    PyErr_SetString(PyExc_OverflowError,
                    "value is out of range for unsigned char");
    }
  return false;
}

inline
bool vtkPythonGetValue(PyObject *o, short &a)
{
  long i = 0;
  if (vtkPythonGetValue(o, i))
    {
    a = static_cast<short>(i);
    if (i >= VTK_SHORT_MIN && i <= VTK_SHORT_MAX)
      {
      return true;
      }
    PyErr_SetString(PyExc_OverflowError,
                    "value is out of range for short");
    }
  return false;
}

inline
bool vtkPythonGetValue(PyObject *o, unsigned short &a)
{
  long i = 0;
  if (vtkPythonGetValue(o, i))
    {
    a = static_cast<unsigned short>(i);
    if (i >= VTK_UNSIGNED_SHORT_MIN && i <= VTK_UNSIGNED_SHORT_MAX)
      {
      return true;
      }
    PyErr_SetString(PyExc_OverflowError,
                    "value is out of range for unsigned short");
    }
  return false;
}


inline
bool vtkPythonGetValue(PyObject *o, int &a)
{
  long i = 0;
  if (vtkPythonGetValue(o, i))
    {
    a = static_cast<int>(i);
#if VTK_SIZEOF_INT < VTK_SIZEOF_LONG
    if (i >= VTK_INT_MIN && i <= VTK_INT_MAX)
      {
      return true;
      }
    PyErr_SetString(PyExc_OverflowError,
                    "value is out of range for int");
#else
    return true;
#endif
    }
  return false;
}

inline
bool vtkPythonGetValue(PyObject *o, unsigned int &a)
{
#if VTK_SIZEOF_INT < VTK_SIZEOF_LONG
  long i = 0;
  if (vtkPythonGetValue(o, i))
    {
    a = static_cast<unsigned int>(i);
    if (i >= VTK_UNSIGNED_INT_MIN && i <= VTK_UNSIGNED_INT_MAX)
      {
      return true;
      }
    PyErr_SetString(PyExc_OverflowError,
                    "value is out of range for unsigned int");
    }
  return false;
#else
  unsigned long i = 0;
  if (vtkPythonGetValue(o, i))
    {
    a = static_cast<unsigned int>(i);
    return true;
    }
  return false;
#endif
}

#ifdef VTK_TYPE_USE_LONG_LONG
inline
bool vtkPythonGetValue(PyObject *o, long long &a)
{
  return vtkPythonGetLongLongValue(o, a);
}

inline
bool vtkPythonGetValue(PyObject *o, unsigned long long &a)
{
  return vtkPythonGetUnsignedLongLongValue(o, a);
}
#endif

#ifdef VTK_TYPE_USE___INT64
inline
bool vtkPythonGetValue(PyObject *o, __int64 &a)
{
  return vtkPythonGetLongLongValue(o, a);
}

inline
bool vtkPythonGetValue(PyObject *o, unsigned __int64 &a)
{
  return vtkPythonGetUnsignedLongLongValue(o, i);
}
#endif


//--------------------------------------------------------------------
// Method for setting a C++ array from a Python sequence.

static
bool vtkPythonSequenceError(PyObject *o, Py_ssize_t n, Py_ssize_t m);

template<class T> inline
bool vtkPythonGetArray(PyObject *o, T *a, int n)
{
  if (a)
    {
    Py_ssize_t m = n;

    if (PyTuple_Check(o))
      {
      m = PyTuple_GET_SIZE(o);
      if (m == n)
        {
        bool r = true;
        for (int i = 0; i < n && r; i++)
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
        bool r = true;
        for (int i = 0; i < n && r; i++)
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
        bool r = true;
        for (int i = 0; i < n && r; i++)
          {
          r = false;
          PyObject *s = PySequence_GetItem(o, i);
          if (s && vtkPythonGetValue(s, a[i]))
            {
            Py_DECREF(s);
            r = true;
            }
          }
        return r;
        }
      }

    return vtkPythonSequenceError(o, n, m);
    }

  return true;
}

//--------------------------------------------------------------------
// Method for setting an n-dimensional C++ arrays from a Python sequence.

template<class T>
bool vtkPythonGetNArray(PyObject *o, T *a, int ndim, const int *dims)
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
        bool r = true;
        if (ndim > 1)
          {
          for (int i = 0; i < n && r; i++)
            {
            PyObject *s = PyList_GET_ITEM(o, i);
            r = vtkPythonGetNArray(s, a, ndim-1, dims+1);
            a += inc;
            }
          }
        else
          {
          for (int i = 0; i < n && r; i++)
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
        bool r = true;
        for (int i = 0; i < n && r; i++)
          {
          r = false;
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

  return true;
}

//--------------------------------------------------------------------
// Method for setting a python sequence from a C++ array

template<class T> inline
bool vtkPythonSetArray(PyObject *o, const T *a, int n)
{
  if (a)
    {
    Py_ssize_t m = n;

    if (PyList_Check(o))
      {
      m = PyList_GET_SIZE(o);
      if (m == n)
        {
        bool r = true;
        for (int i = 0; i < n && r; i++)
          {
          r = false;
          PyObject *s = vtkPythonArgs::BuildValue(a[i]);
          if (s)
            {
            Py_DECREF(PyList_GET_ITEM(o, i));
            PyList_SET_ITEM(o, i, s);
            r = true;
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
        bool r = true;
        for (int i = 0; i < n && r; i++)
          {
          r = false;
          PyObject *s = vtkPythonArgs::BuildValue(a[i]);
          if (s)
            {
            r = (PySequence_SetItem(o, i, s) != -1);
            Py_DECREF(s);
            }
          }
        return r;
        }
      }

    return vtkPythonSequenceError(o, n, m);
    }

  return true;
}

//--------------------------------------------------------------------
// Method for setting a python array from an n-dimensional C++ array

template<class T>
bool vtkPythonSetNArray(
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
        bool r = true;
        if (ndim > 1)
          {
          for (int i = 0; i < n && r; i++)
            {
            PyObject *s = PyList_GET_ITEM(o, i);
            r = vtkPythonSetNArray(s, a, ndim-1, dims+1);
            a += inc;
            }
          }
        else
          {
          for (int i = 0; i < n && r; i++)
            {
            r = false;
            PyObject *s = vtkPythonArgs::BuildValue(a[i]);
            if (s)
              {
              Py_DECREF(PyList_GET_ITEM(o, i));
              PyList_SET_ITEM(o, i, s);
              r = true;
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
        bool r = true;
        if (ndim > 1)
          {
          for (int i = 0; i < n && r; i++)
            {
            r = false;
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
          for (int i = 0; i < n && r; i++)
            {
            r = false;
            PyObject *s = vtkPythonArgs::BuildValue(a[i]);
            if (s)
              {
              r = (PySequence_SetItem(o, i, s) != -1);
              Py_DECREF(s);
              }
            }
          }
        return r;
        }
      }

    return vtkPythonSequenceError(o, n, m);
    }

  return true;
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
        if (vtkself->IsA(vtkclass->vtk_cppname))
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

PyObject *vtkPythonArgs::GetArgAsPythonObject(
  bool &valid)
{
  PyObject *o = PyTuple_GET_ITEM(this->Args, this->I++);
  valid = true;
  return o;
}

PyObject *vtkPythonArgs::GetArgAsPythonObject(
  PyObject *o, bool &valid)
{
  valid = true;
  return o;
}

vtkObjectBase *vtkPythonArgs::GetArgAsVTKObject(
  const char *classname, bool &valid)
{
  PyObject *o = PyTuple_GET_ITEM(this->Args, this->I++);
  vtkObjectBase *r = vtkPythonArgs::GetArgAsVTKObject(o, classname, valid);
  if (!valid)
    {
    this->RefineArgTypeError(this->I - this->M - 1);
    }
  return r;
}

vtkObjectBase *vtkPythonArgs::GetArgAsVTKObject(
  PyObject *o, const char *classname, bool &valid)
{
  vtkObjectBase *r = vtkPythonUtil::GetPointerFromObject(o, classname);
  valid = (r || o == Py_None);
  return r;
}

void *vtkPythonArgs::GetArgAsSpecialObject(
  const char *classname, PyObject **p)
{
  PyObject *o = PyTuple_GET_ITEM(this->Args, this->I++);
  void *r = vtkPythonArgs::GetArgAsSpecialObject(o, classname, p);
  if (r == NULL)
    {
    this->RefineArgTypeError(this->I - this->M - 1);
    }
  return r;
}

void *vtkPythonArgs::GetArgAsSpecialObject(
  PyObject *o, const char *classname, PyObject **p)
{
  void *r = vtkPythonUtil::GetPointerFromSpecialObject(o, classname, p);
  return r;
}

int vtkPythonArgs::GetArgAsEnum(PyTypeObject *enumtype, bool &valid)
{
  PyObject *o = PyTuple_GET_ITEM(this->Args, this->I++);
  int i = vtkPythonArgs::GetArgAsEnum(o, enumtype, valid);
  if (!valid)
    {
    this->RefineArgTypeError(this->I - this->M - 1);
    }
  return i;
}

int vtkPythonArgs::GetArgAsEnum(
  PyObject *o, PyTypeObject *enumtype, bool &valid)
{
  long i = 0;
  if (o->ob_type == enumtype)
    {
    i = PyInt_AsLong(o);
    valid = true;
    }
  else
    {
    std::string errstring = "expected enum ";
    errstring += enumtype->tp_name;
    errstring += ", got ";
    errstring += o->ob_type->tp_name;
    PyErr_SetString(PyExc_TypeError, errstring.c_str());
    valid = false;
    }
  return i;
}


//--------------------------------------------------------------------
// Define the methods for SIP objects

void *vtkPythonArgs::GetArgAsSIPObject(const char *classname, bool &valid)
{
  PyObject *o = PyTuple_GET_ITEM(this->Args, this->I++);
  void *r = vtkPythonArgs::GetArgAsSIPObject(o, classname, valid);
  if (!valid)
    {
    this->RefineArgTypeError(this->I - this->M - 1);
    }
  return r;
}

void *vtkPythonArgs::GetArgAsSIPObject(
  PyObject *o, const char *classname, bool &valid)
{
  void *r = vtkPythonUtil::SIPGetPointerFromObject(o, classname);
  valid = (r || !PyErr_Occurred());
  return (valid ? r : NULL);
}

int vtkPythonArgs::GetArgAsSIPEnum(const char *classname, bool &valid)
{
  PyObject *o = PyTuple_GET_ITEM(this->Args, this->I++);
  int i = vtkPythonArgs::GetArgAsSIPEnum(o, classname, valid);
  if (!valid)
    {
    this->RefineArgTypeError(this->I - this->M - 1);
    }
  return i;
}

int vtkPythonArgs::GetArgAsSIPEnum(
  PyObject *o, const char *classname, bool &valid)
{
  int i = 0;
  valid = (vtkPythonUtil::SIPGetPointerFromObject(o, classname) &&
           vtkPythonGetValue(o, i));
  return (valid ? i : 0);
}

//--------------------------------------------------------------------
// Define all the "GetValue" methods in the class.

#define VTK_PYTHON_GET_ARG(T) \
bool vtkPythonArgs::GetValue(T &a) \
{ \
  PyObject *o = PyTuple_GET_ITEM(this->Args, this->I++); \
  if (PyVTKMutableObject_Check(o)) \
    { \
    o = PyVTKMutableObject_GetValue(o); \
    } \
  if (vtkPythonGetValue(o, a)) \
    { \
    return true; \
    } \
  this->RefineArgTypeError(this->I - this->M - 1); \
  return false; \
} \
 \
bool vtkPythonArgs::GetValue(PyObject *o, T &a) \
{ \
  return vtkPythonGetValue(o, a); \
}

VTK_PYTHON_GET_ARG(void *)
VTK_PYTHON_GET_ARG(const void *)
VTK_PYTHON_GET_ARG(char *)
VTK_PYTHON_GET_ARG(const char *)
VTK_PYTHON_GET_ARG(std::string)
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
  if (vtkPythonGetArray(o, a, n)) \
    { \
    return true; \
    } \
  this->RefineArgTypeError(this->I - this->M - 1); \
  return false; \
}

VTK_PYTHON_GET_ARRAY_ARG(bool)
VTK_PYTHON_GET_ARRAY_ARG(float)
VTK_PYTHON_GET_ARRAY_ARG(double)
VTK_PYTHON_GET_ARRAY_ARG(char)
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
  if (vtkPythonGetNArray(o, a, ndim, dims)) \
    { \
    return true; \
    } \
  this->RefineArgTypeError(this->I - this->M - 1); \
  return false; \
}

VTK_PYTHON_GET_NARRAY_ARG(bool)
VTK_PYTHON_GET_NARRAY_ARG(float)
VTK_PYTHON_GET_NARRAY_ARG(double)
VTK_PYTHON_GET_NARRAY_ARG(char)
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

bool vtkPythonArgs::GetFunction(PyObject *arg, PyObject *&o)
{
  o = arg;
  if (o == Py_None || PyCallable_Check(o))
    {
    return true;
    }
  PyErr_SetString(PyExc_TypeError, "a callable object is required");
  return false;
}

bool vtkPythonArgs::GetFunction(PyObject *&o)
{
  PyObject *arg = PyTuple_GET_ITEM(this->Args, this->I++);
  return vtkPythonArgs::GetFunction(arg, o);
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

VTK_PYTHON_SET_ARG(const std::string &)
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
    if (vtkPythonSetArray(o, a, n)) \
      { \
      return true; \
      } \
    this->RefineArgTypeError(i); \
    return false; \
    } \
  return true; \
}

VTK_PYTHON_SET_ARRAY_ARG(bool)
VTK_PYTHON_SET_ARRAY_ARG(float)
VTK_PYTHON_SET_ARRAY_ARG(double)
VTK_PYTHON_SET_ARRAY_ARG(char)
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
    if (vtkPythonSetNArray(o, a, ndim, dims)) \
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
VTK_PYTHON_SET_NARRAY_ARG(char)
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
bool vtkPythonArgs::ArgCountError(int m, int n)
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
  return false;
}

//--------------------------------------------------------------------
// Static method to write an arg count error.
bool vtkPythonArgs::ArgCountError(int n, const char *name)
{
  char text[256];

  sprintf(text, "no overloads of %.200s%s take %d argument%s",
          (name ? name : "function"), (name ? "()" : ""),
          n, (n == 1 ? "" : "s"));
  PyErr_SetString(PyExc_TypeError, text);
  return false;
}

//--------------------------------------------------------------------
// Raise an exception about pure virtual method call
bool vtkPythonArgs::PureVirtualError()
{
  char text[256];

  sprintf(text, "pure virtual method %.200s() was called",
          this->MethodName);
  PyErr_SetString(PyExc_TypeError, text);
  return false;
}

//--------------------------------------------------------------------
// Refine an error by saying what argument it is for
bool vtkPythonArgs::RefineArgTypeError(int i)
{
  if (PyErr_ExceptionMatches(PyExc_TypeError) ||
      PyErr_ExceptionMatches(PyExc_ValueError) ||
      PyErr_ExceptionMatches(PyExc_OverflowError))
    {
    PyObject *exc;
    PyObject *val;
    PyObject *frame;
    char text[480];
    const char *cp = "";

    PyErr_Fetch(&exc, &val, &frame);
    if (val && PyString_Check(val))
      {
      cp = PyString_AsString(val);
      }
    sprintf(text, "%.200s argument %d: %.200s",
            this->MethodName, i+1, cp);
    Py_XDECREF(val);
    val = PyString_FromString(text);
    PyErr_Restore(exc, val, frame);
    }
  return false;
}

//--------------------------------------------------------------------
// Raise a type error for a sequence arg of wrong type or size.
bool vtkPythonSequenceError(PyObject *o, Py_ssize_t n, Py_ssize_t m)
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
  return false;
}

//--------------------------------------------------------------------
// Checking size of array arg.
int vtkPythonArgs::GetArgSize(int i)
{
  int size = 0;
  if (this->M + i < this->N)
    {
    PyObject *o = PyTuple_GET_ITEM(this->Args, this->M + i);
    if (PySequence_Check(o))
      {
      size = static_cast<int>(PySequence_Size(o));
      }
    }
  return size;
}
