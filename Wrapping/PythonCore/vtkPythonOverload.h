// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Created in June 2010 by David Gobbi, originally in vtkPythonUtil.
 *
 * This file provides methods for calling overloaded functions
 * that are stored in a PyMethodDef table.  The arguments are
 * checked against the format strings that are stored in the
 * documentation fields of the table.  For more information,
 * see vtkWrapPython_ArgCheckString() in vtkWrapPython.c.
 */

/**
 * @class   vtkPythonOverload
 */

#ifndef vtkPythonOverload_h
#define vtkPythonOverload_h

#include "vtkABINamespace.h"
#include "vtkPython.h"
#include "vtkWrappingPythonCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKWRAPPINGPYTHONCORE_EXPORT vtkPythonOverload
{
public:
  /**
   * Check python object against a format character and return a number
   * to indicate how well it matches (lower numbers are better).
   */
  static int CheckArg(PyObject* arg, const char* format, const char* classname, int level = 0);

  /**
   * Call the method that is the best match for the for the provided
   * arguments.  The docstrings in the PyMethodDef must provide info
   * about the argument types for each method.
   */
  static PyObject* CallMethod(PyMethodDef* methods, PyObject* self, PyObject* args);

  ///@{
  /**
   * Find a method that takes the single arg provided, this is used
   * to locate the correct constructor signature for a conversion.
   * The docstrings in the PyMethodDef must provide info about the
   * argument types for each method.
   */
  static PyMethodDef* FindConversionMethod(PyMethodDef* methods, PyObject* arg);
  ///@}
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkPythonOverload.h
