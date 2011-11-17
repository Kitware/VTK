/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonOverload.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Created in June 2010 by David Gobbi, originally in vtkPythonUtil.
 *
 * This file provides methods for calling overloaded functions
 * that are stored in a PyMethodDef table.  The arguments are
 * checked against the format strings that are stored in the
 * documentation fields of the table.  For more information,
 * see vtkWrapPython_ArgCheckString() in vtkWrapPython.c.
 */

// .NAME vtkPythonOverload

#ifndef __vtkPythonOverload_h
#define __vtkPythonOverload_h

#include "vtkPython.h"

class VTK_PYTHON_EXPORT vtkPythonOverload
{
public:

  // Description:
  // Check python object against a format character and return a number
  // to indicate how well it matches (lower numbers are better).
  static int CheckArg(PyObject *arg, const char *format,
                      const char *classname, int level=0);

  // Description:
  // Call the method that is the best match for the for the provided
  // arguments.  The docstrings in the PyMethodDef must provide info
  // about the argument types for each method.
  static PyObject *CallMethod(PyMethodDef *methods,
                              PyObject *self, PyObject *args);

  // Description:
  // Find a method that takes the single arg provided, this is used
  // to locate the correct constructor signature for a conversion.
  // The docstrings in the PyMethodDef must provide info about the
  // argument types for each method.
  static PyMethodDef *FindConversionMethod(PyMethodDef *methods,
                                           PyObject *arg);
};

#endif
