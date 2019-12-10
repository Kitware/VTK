/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PyVTKEnum.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef PyVTKEnum_h
#define PyVTKEnum_h

#include "vtkPython.h"
#include "vtkSystemIncludes.h"
#include "vtkWrappingPythonCoreModule.h" // For export macro

extern "C"
{
  VTKWRAPPINGPYTHONCORE_EXPORT
  PyTypeObject* PyVTKEnum_Add(PyTypeObject* pytype, const char* name);

  VTKWRAPPINGPYTHONCORE_EXPORT
  PyObject* PyVTKEnum_New(PyTypeObject* pytype, int val);
}

#endif
