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

// define our main check macro VTK_PY3K
#if PY_MAJOR_VERSION >= 3
#define VTK_PY3K
#endif

// ===== Macros needed for Python 3 ====
#ifdef VTK_PY3K

// Buffer compatibility
#if PY_VERSION_HEX < 0x03030000
#define VTK_PYBUFFER_INITIALIZER \
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0 }, 0 }
#else
#define VTK_PYBUFFER_INITIALIZER \
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
#endif

#endif

// ===== Macros needed for Python 2 ====
#ifndef VTK_PY3K

// Required for Python 2.5 compatibility
#ifndef PyVarObject_HEAD_INIT
#define PyVarObject_HEAD_INIT(type, size) \
  PyObject_HEAD_INIT(type) size,
#endif

// Required for Python 2.5 compatibility
#ifndef Py_TYPE
#define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)
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

#endif
