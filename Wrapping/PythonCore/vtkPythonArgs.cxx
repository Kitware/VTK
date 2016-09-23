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

// Keep vtkPythonArgs.h from declaring externs for templates we instantiate
#define vtkPythonArgs_cxx

#include "vtkPythonArgs.h"
#include "vtkPythonUtil.h"

#include "vtkObjectBase.h"

//--------------------------------------------------------------------
// Extract various C++ types from python objects.  The rules are
// identical to PyArg_ParseTuple except that range checking is done
// on unsigned values.

// Macro to mimic a check done in PyArg_ParseTuple
#define VTK_PYTHON_FLOAT_CHECK()\
  if (PyFloat_Check(o)) \
  { \
    PyErr_SetString(PyExc_TypeError, \
                      "integer argument expected, got float"); \
    return false; \
  }

inline
bool vtkPythonGetValue(PyObject *o, long &a)
{
  VTK_PYTHON_FLOAT_CHECK();

  a = PyInt_AsLong(o);
  return (a != static_cast<long>(-1) || !PyErr_Occurred());
}

inline
bool vtkPythonGetValue(PyObject *o, unsigned long &a)
{
  VTK_PYTHON_FLOAT_CHECK();

  a = PyLong_AsUnsignedLong(o);
  return (a != static_cast<unsigned long>(-1) || !PyErr_Occurred());
}

template <class T> inline
bool vtkPythonGetLongLongValue(PyObject *o, T &a)
{
  VTK_PYTHON_FLOAT_CHECK();

  PY_LONG_LONG i = PyLong_AsLongLong(o);
  a = static_cast<T>(i);
  return (i != static_cast<PY_LONG_LONG>(-1) || !PyErr_Occurred());
}

template <class T> inline
bool vtkPythonGetUnsignedLongLongValue(PyObject *o, T &a)
{
  VTK_PYTHON_FLOAT_CHECK();

  // PyLong_AsUnsignedLongLong will fail if "o" is not a PyLong
  if (PyLong_Check(o))
  {
    unsigned PY_LONG_LONG i = PyLong_AsUnsignedLongLong(o);
    a = static_cast<T>(i);
    return (i != static_cast<unsigned PY_LONG_LONG>(-1) || !PyErr_Occurred());
  }

  unsigned long l = PyLong_AsUnsignedLong(o);
  a = static_cast<T>(l);
  return (l != static_cast<unsigned long>(-1) || !PyErr_Occurred());
}


template <class T> inline
bool vtkPythonGetStringValue(PyObject *o, T *&a, const char *exctext)
{
  if (PyBytes_Check(o))
  {
    a = PyBytes_AS_STRING(o);
    return true;
  }
#ifdef Py_USING_UNICODE
  else if (PyUnicode_Check(o))
  {
#if PY_VERSION_HEX >= 0x03030000
    a = PyUnicode_AsUTF8(o);
    return true;
#else
    PyObject *s = _PyUnicode_AsDefaultEncodedString(o, NULL);
    if (s)
    {
      a = PyBytes_AS_STRING(s);
      return true;
    }

    exctext = "(unicode conversion error)";
#endif
  }
#endif

  PyErr_SetString(PyExc_TypeError, exctext);
  return false;
}

inline bool vtkPythonGetStdStringValue(PyObject *o, std::string &a, const char *exctext)
{
  if (PyBytes_Check(o))
  {
    char* val;
    Py_ssize_t len;
    PyBytes_AsStringAndSize(o, &val, &len);
    a = std::string(val, len);
    return true;
  }
#ifdef Py_USING_UNICODE
  else if (PyUnicode_Check(o))
  {
#if PY_VERSION_HEX >= 0x03030000
    Py_ssize_t len;
    const char* val = PyUnicode_AsUTF8AndSize(o, &len);
    a = std::string(val, len);
    return true;
#else
    PyObject *s = _PyUnicode_AsDefaultEncodedString(o, NULL);
    if (s)
    {
      char* val;
      Py_ssize_t len;
      PyBytes_AsStringAndSize(s, &val, &len);
      a = std::string(val, len);
      return true;
    }

    exctext = "(unicode conversion error)";
#endif
  }
#endif

  PyErr_SetString(PyExc_TypeError, exctext);
  return false;
}

//--------------------------------------------------------------------
// Overloaded methods, mostly based on the above templates

static bool vtkPythonGetValue(
  PyObject *o, const void *&a, Py_buffer *view)
{
  void *p = 0;
  Py_ssize_t sz = 0;
#ifndef VTK_PY3K
  const char *format = 0;
  PyBufferProcs *b = Py_TYPE(o)->tp_as_buffer;
#endif

#if PY_VERSION_HEX < 0x02060000
  (void)view;
#else
#ifdef VTK_PY3K
  PyObject *bytes = NULL;
  if (PyUnicode_Check(o))
  {
    bytes = PyUnicode_AsUTF8String(o);
    PyBytes_AsStringAndSize(bytes, reinterpret_cast<char **>(&p), &sz);
  }
  else
#endif
  if (PyObject_CheckBuffer(o))
  {
    // use the new buffer interface
    if (PyObject_GetBuffer(o, view, PyBUF_SIMPLE) == -1)
    {
      return false;
    }
    p = view->buf;
    sz = view->len;
#ifndef VTK_PY3K
    format = view->format;
#endif
  }
#ifndef VTK_PY3K
  else
#endif
#endif
#ifndef VTK_PY3K
  // use the old buffer interface
  if (b && b->bf_getreadbuffer && b->bf_getsegcount)
  {
    if (b->bf_getsegcount(o, NULL) == 1)
    {
      sz = b->bf_getreadbuffer(o, 0, &p);
    }
    else
    {
      PyErr_SetString(PyExc_TypeError, "buffer must be single-segment");
      return false;
    }
  }
#endif

#ifdef VTK_PY3K
  if (bytes)
#else
  if (p && sz >= 0 && sz <= VTK_INT_MAX &&
      (format == 0 || format[0] == 'c' || format[0] == 'B'))
#endif
  {
    // check for pointer mangled as string
    int s = static_cast<int>(sz);
    a = vtkPythonUtil::UnmanglePointer(
      reinterpret_cast<char *>(p), &s, "p_void");
#ifdef VTK_PY3K
    Py_DECREF(bytes);
    if (s != 0)
    {
      PyErr_SetString(PyExc_TypeError, "requires a _addr_p_void string");
      return false;
    }
#else
    if (s == -1)
    {
      // matched _addr_ but not p_void, assume it isn't a swig ptr string:
      // use the buffer's pointer as the argument
      a = p;
    }
#endif
    return true;
  }
  else if (p && sz >= 0)
  {
    // directly use the pointer to the buffer contents
    a = p;
    return true;
  }

  PyErr_SetString(PyExc_TypeError,
    "object does not have a readable buffer");
  return false;
}

inline
bool vtkPythonGetValue(PyObject *o, void *&a, Py_buffer *buf)
{
  // should have an alternate form for non-const "void *" that uses
  // writebuffer instead of readbuffer, but that would break existing code
  const void *b = NULL;
  bool r = vtkPythonGetValue(o, b, buf);
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
    a = vtkUnicodeString::from_utf8(PyBytes_AS_STRING(s));
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
      m = PySequence_Size(o);
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
      m = PySequence_Size(o);
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
      m = PySequence_Size(o);
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
      m = PySequence_Size(o);
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
VTK_PYTHON_BUILD_TUPLE(long long)
VTK_PYTHON_BUILD_TUPLE(unsigned long long)

//--------------------------------------------------------------------
// If "self" is a class, get real "self" from arg list
PyObject *vtkPythonArgs::GetSelfFromFirstArg(
  PyObject *self, PyObject *args)
{
  if (PyType_Check(self))
  {
    PyTypeObject *pytype = (PyTypeObject *)self;
    if (PyTuple_GET_SIZE(args) > 0)
    {
      self = PyTuple_GET_ITEM(args, 0);
      if (PyObject_TypeCheck(self, pytype))
      {
        return self;
      }
    }

    char buf[256];
    sprintf(buf, "unbound method requires a %.200s as the first argument",
            pytype->tp_name);
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

int vtkPythonArgs::GetArgAsEnum(const char *enumname, bool &valid)
{
  PyObject *o = PyTuple_GET_ITEM(this->Args, this->I++);
  int i = vtkPythonArgs::GetArgAsEnum(o, enumname, valid);
  if (!valid)
  {
    this->RefineArgTypeError(this->I - this->M - 1);
  }
  return i;
}

int vtkPythonArgs::GetArgAsEnum(
  PyObject *o, const char *enumname, bool &valid)
{
  long i = 0;
  PyTypeObject *pytype = vtkPythonUtil::FindEnum(enumname);
  if (pytype && PyObject_TypeCheck(o, pytype))
  {
    i = PyInt_AsLong(o);
    valid = true;
  }
  else
  {
    std::string errstring = "expected enum ";
    errstring += enumname;
    errstring += ", got ";
    errstring += Py_TYPE(o)->tp_name;
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
VTK_PYTHON_GET_ARG(long long)
VTK_PYTHON_GET_ARG(unsigned long long)

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
VTK_PYTHON_GET_ARRAY_ARG(long long)
VTK_PYTHON_GET_ARRAY_ARG(unsigned long long)

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
VTK_PYTHON_GET_NARRAY_ARG(long long)
VTK_PYTHON_GET_NARRAY_ARG(unsigned long long)

//--------------------------------------------------------------------
// Define the special function pointer GetValue method

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
// Define the void pointer GetValue method

#define VTK_PYTHON_GET_BUFFER(T) \
bool vtkPythonArgs::GetBuffer(T &a, Py_buffer *buf) \
{ \
  PyObject *o = PyTuple_GET_ITEM(this->Args, this->I++); \
  if (vtkPythonGetValue(o, a, buf)) \
  { \
    return true; \
  } \
  this->RefineArgTypeError(this->I - this->M - 1); \
  return false; \
} \
 \
bool vtkPythonArgs::GetBuffer(PyObject *o, T &a, Py_buffer *buf) \
{ \
  return vtkPythonGetValue(o, a, buf); \
}

VTK_PYTHON_GET_BUFFER(void *)
VTK_PYTHON_GET_BUFFER(const void *)

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
VTK_PYTHON_SET_ARG(long long)
VTK_PYTHON_SET_ARG(unsigned long long)

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
VTK_PYTHON_SET_ARRAY_ARG(long long)
VTK_PYTHON_SET_ARRAY_ARG(unsigned long long)

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
VTK_PYTHON_SET_NARRAY_ARG(long long)
VTK_PYTHON_SET_NARRAY_ARG(unsigned long long)

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
    PyObject *val, *newval;
    PyObject *frame;

    PyErr_Fetch(&exc, &val, &frame);

#ifdef VTK_PY3K
    const char *cp = "";
    if (val && !PyUnicode_Check(val))
    {
      Py_DECREF(val);
      val = 0;
    }
    newval = PyUnicode_FromFormat("%s argument %d: %V",
      this->MethodName, i+1, val, cp);
#else
    const char *cp = "";
    if (val && PyString_Check(val))
    {
      cp = PyString_AsString(val);
    }
    newval = PyString_FromFormat("%s argument %d: %s",
      this->MethodName, i+1, cp);
#endif

    Py_XDECREF(val);
    PyErr_Restore(exc, newval, frame);
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
            (long)n, ((n == 1) ? "" : "s"), Py_TYPE(o)->tp_name);
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

//--------------------------------------------------------------------
// Check if 'm' equals 'n', and report an error for arg i if not.
bool vtkPythonArgs::CheckSizeHint(int i, Py_ssize_t m, Py_ssize_t n)
{
  if (this->M + i < this->N)
  {
    if (m != n)
    {
      PyObject *o = PyTuple_GET_ITEM(this->Args, this->M + i);
      return vtkPythonSequenceError(o, n, m);
    }
  }
  return true;
}

//--------------------------------------------------------------------
// Use stack space for small arrays, heap for large arrays.
template<class T>
vtkPythonArgs::Array<T>::Array(Py_ssize_t n) : Pointer(0)
{
  if (n > basicsize)
  {
    this->Pointer = new T[n];
  }
  else if (n != 0)
  {
    this->Pointer = this->Storage;
  }
}

// Instantiate the Array class template over all types:
vtkPythonArgsTemplateMacro(
  template class VTKWRAPPINGPYTHONCORE_EXPORT vtkPythonArgs::Array
)
