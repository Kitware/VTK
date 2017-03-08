/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonCompatibility.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  This header contains macros to make Python 2 and Python 3 play nice.
  It must be included after vtkPython.h.
-----------------------------------------------------------------------*/
#ifndef vtkPythonCompatibility_h
#define vtkPythonCompatibility_h

// define our main check macro VTK_PY3K
#if PY_MAJOR_VERSION >= 3
#define VTK_PY3K
#endif

// ===== Macros needed for Python 3 ====
#ifdef VTK_PY3K

// Int/Long compatibility
#define PyIntObject PyLongObject
#define PyInt_Type PyLong_Type
#define PyInt_Check PyLong_Check
#define PyInt_FromLong PyLong_FromLong
#define PyInt_AsLong PyLong_AsLong

// Unicode/String compatibility
#define PyString_InternFromString PyUnicode_InternFromString
#define PyString_FromFormat PyUnicode_FromFormat
#define PyString_Check PyUnicode_Check
#define PyString_FromString PyUnicode_FromString
#define PyString_FromStringAndSize PyUnicode_FromStringAndSize

// Use this for PyUnicode_EncodeLocale, see PEP 383
#define VTK_PYUNICODE_ENC "surrogateescape"

// Required for Python 3.2 compatibility
#if PY_VERSION_HEX < 0x03030000
#define PyUnicode_DecodeLocaleAndSize PyUnicode_DecodeFSDefaultAndSize
#define PyUnicode_DecodeLocale PyUnicode_DecodeFSDefault
#define PyUnicode_EncodeLocale(o,e) PyUnicode_EncodeFSDefault(o)
#define PyString_AsString _PyUnicode_AsString
#else
#define PyString_AsString PyUnicode_AsUTF8
#endif

// Buffer compatibility
#if PY_VERSION_HEX < 0x03030000
#define VTK_PYBUFFER_INITIALIZER \
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0 }, 0 }
#else
#define VTK_PYBUFFER_INITIALIZER \
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
#endif

// PyTypeObject compatibility
#if PY_VERSION_HEX >= 0x03040000
#define VTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED \
  0, 0, 0,
#else
#define VTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED \
  0, 0,
#endif

#endif

// ===== Macros needed for Python 2 ====
#ifndef VTK_PY3K

// Py3k introduced a new type "Py_hash_t"
typedef long Py_hash_t;
typedef unsigned long Py_uhash_t;

// Required for Python 2.5 compatibility
#ifndef PyVarObject_HEAD_INIT
#define PyVarObject_HEAD_INIT(type, size) \
  PyObject_HEAD_INIT(type) size,
#endif

// Required for Python 2.5 compatibility
#ifndef Py_TYPE
#define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)
#endif

// Required for Python 2.5 compatibility
#ifndef PyBytes_Check
#define PyBytesObject PyStringObject
#define PyBytes_Type PyString_Type
#define PyBytes_Check PyString_Check
#define PyBytes_CheckExact PyString_CheckExact
#define PyBytes_AS_STRING PyString_AS_STRING
#define PyBytes_GET_SIZE PyString_GET_SIZE
#define PyBytes_FromStringAndSize PyString_FromStringAndSize
#define PyBytes_FromString PyString_FromString
#define PyBytes_FromFormat PyString_FromFormat
#define PyBytes_Size PyString_Size
#define PyBytes_AsString PyString_AsString
#define PyBytes_Concat PyString_Concat
#define PyBytes_ConcatAndDel PyString_ConcatAndDel
#define _PyBytes_Resize _PyString_Resize
#define PyBytes_Format PyString_Format
#define PyBytes_AsStringAndSize PyString_AsStringAndSize
#endif

// Buffer struct initialization is different for every version
#if PY_VERSION_HEX < 0x02060000
typedef struct bufferinfo { PyObject *obj; } Py_buffer;
#define VTK_PYBUFFER_INITIALIZER \
  { 0 }
#elif PY_VERSION_HEX < 0x02070000
#define VTK_PYBUFFER_INITIALIZER \
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
#else
#define VTK_PYBUFFER_INITIALIZER \
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0 }, 0 }
#endif

// PyTypeObject compatibility
#if PY_VERSION_HEX >= 0x02060000
#define VTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED 0, 0,
#else
#define VTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED 0,
#endif

#endif
#endif
