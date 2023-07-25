// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/*-----------------------------------------------------------------------
  The PyVTKExtras was created in Aug 2015 by David Gobbi.

  This file provides extra classes and functions for the vtk module.
  Unlike the contents of vtk.util, the classes and functions provided
  here are ones that must be written in C++ instead of pure python.
-----------------------------------------------------------------------*/

#ifndef PyVTKExtras_h
#define PyVTKExtras_h

#include "vtkABINamespace.h"
#include "vtkPython.h"
#include "vtkWrappingPythonCoreModule.h" // For export macro

//--------------------------------------------------------------------
// This will add extras to the provided dict.  It is called during the
// initialization of the vtkCommonCore python module.
extern "C"
{
  VTKWRAPPINGPYTHONCORE_EXPORT void PyVTKAddFile_PyVTKExtras(PyObject* dict);
}

#endif
