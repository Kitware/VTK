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
  This header contains macros to make Python 3 play nice.
  It must be included after vtkPython.h.
-----------------------------------------------------------------------*/
#ifndef vtkPythonCompatibility_h
#define vtkPythonCompatibility_h

// define our main check macro VTK_PY3K
#if PY_MAJOR_VERSION >= 3
#define VTK_PY3K
#endif

// Use this for PyUnicode_EncodeLocale, see PEP 383
#define VTK_PYUNICODE_ENC "surrogateescape"

// Buffer initialization
#define VTK_PYBUFFER_INITIALIZER                                                                   \
  {                                                                                                \
    nullptr, nullptr, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr, nullptr                      \
  }

// PyTypeObject compatibility
#if PY_VERSION_HEX >= 0x03090000
#define VTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED nullptr, 0, nullptr, nullptr,
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
// VTK-HeaderTest-Exclude: vtkPythonCompatibility.h
