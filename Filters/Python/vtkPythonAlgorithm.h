/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPythonAlgorithm - a user-programmable filter
// .SECTION Description
// vtkPythonAlgorithm is a filter that calls a Python object to do the actual
// work.

// .SECTION See Also
// vtkProgrammableFilter

#ifndef __vtkPythonAlgorithm_h
#define __vtkPythonAlgorithm_h

#include "vtkPython.h" // Must be first

#include "vtkFiltersPythonModule.h" // For export macro
#include "vtkAlgorithm.h"

class VTKFILTERSPYTHON_EXPORT vtkPythonAlgorithm : public vtkAlgorithm
{
public:
  static vtkPythonAlgorithm *New();
  vtkTypeMacro(vtkPythonAlgorithm, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the Python object to use to operate on the data. A reference will
  // be taken on the object.
  void SetPythonObject(PyObject* obj);

  // Description:
  // Set the number of input ports used by the algorithm.
  virtual void SetNumberOfInputPorts(int n);

  // Description:
  // Set the number of output ports provided by the algorithm.
  virtual void SetNumberOfOutputPorts(int n);

protected:
  vtkPythonAlgorithm();
  ~vtkPythonAlgorithm();

  virtual int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inInfo,
                             vtkInformationVector* outInfo);
  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual int FillOutputPortInformation(int port, vtkInformation* info);

private:
  vtkPythonAlgorithm(const vtkPythonAlgorithm&);  // Not implemented.
  void operator=(const vtkPythonAlgorithm&);  // Not implemented.

  int CheckResult(const char* method, PyObject* res);

  PyObject* Object;
};

#endif
