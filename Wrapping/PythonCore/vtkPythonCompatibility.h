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
#define PyString_AsString PyUnicode_AsUTF8
#define PyString_InternFromString PyUnicode_InternFromString
#define PyString_FromFormat PyUnicode_FromFormat
#define PyString_Check PyUnicode_Check
#define PyString_FromString PyUnicode_FromString
#define PyString_FromStringAndSize PyUnicode_FromStringAndSize

// Use this for PyUnicode_EncodeLocale, see PEP 383
#define VTK_PYUNICODE_ENC "surrogateescape"

// Buffer initialization
#define VTK_PYBUFFER_INITIALIZER                                                                   \
  {                                                                                                \
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0                                                                \
  }

// PyTypeObject compatibility
#if PY_VERSION_HEX >= 0x03090000
#define VTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED 0, 0, 0, 0,
#elif PY_VERSION_HEX >= 0x03080000
#define VTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED 0, 0, 0, 0, 0,
#else
#define VTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED 0, 0, 0,
#endif

// Python 3.8 contains a deprecation marker on the `tp_print` field. Since some
// compilers are very touchy about this situation, just suppress the warning.
#if PY_VERSION_HEX < 0x03090000
// GCC-alike (but not Intel) need this.
#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#define VTK_PYTHON_NEEDS_DEPRECATION_WARNING_SUPPRESSION
#endif
#endif

#endif

// ===== Macros needed for Python 2 ====
#ifndef VTK_PY3K

// Py3k introduced a new type "Py_hash_t"
typedef long Py_hash_t;
typedef unsigned long Py_uhash_t;

// Buffer struct initialization
#define VTK_PYBUFFER_INITIALIZER                                                                   \
  {                                                                                                \
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0 }, 0                                                      \
  }

// PyTypeObject initialization
#define VTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED 0, 0,

#endif
#endif
// VTK-HeaderTest-Exclude: vtkPythonCompatibility.h
