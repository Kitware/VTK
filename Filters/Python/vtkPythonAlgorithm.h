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
/**
 * @class   vtkPythonAlgorithm
 * @brief   algorithm that can be implemented in Python
 *
 * vtkPythonAlgorithm is an algorithm that calls a Python object to do the actual
 * work.
 * It defers the following methods to Python:
 * - ProcessRequest()
 * - FillInputPortInformation()
 * - FillOutputPortInformation()
 *
 * Python signature of these methods is as follows:
 * - ProcessRequest(self, vtkself, request, inInfo, outInfo) : vtkself is the vtk object, inInfo is a tuple of information objects
 * - FillInputPortInformation(self, vtkself, port, info)
 * - FillOutputPortInformation(self, vtkself, port, info)
 * - Initialize(self, vtkself)
 *
 * In addition, it calls an Initialize() method when setting the Python
 * object, which allows the initialization of number of input and output
 * ports etc.
 *
 * @sa
 * vtkProgrammableFilter
*/

#ifndef vtkPythonAlgorithm_h
#define vtkPythonAlgorithm_h

#include "vtkPython.h" // Must be first

#include "vtkFiltersPythonModule.h" // For export macro
#include "vtkAlgorithm.h"

class vtkSmartPyObject;

class VTKFILTERSPYTHON_EXPORT vtkPythonAlgorithm : public vtkAlgorithm
{
public:
  static vtkPythonAlgorithm *New();
  vtkTypeMacro(vtkPythonAlgorithm, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Specify the Python object to use to operate on the data. A reference will
   * be taken on the object. This will also invoke Initialize() on the Python
   * object, which is commonly used to set the number of input and output
   * ports as well as perform tasks commonly performed in the constructor
   * of C++ algorithm subclass.
   */
  void SetPythonObject(PyObject* obj);

  /**
   * Set the number of input ports used by the algorithm.
   * This is made public so that it can be called from Python.
   */
  virtual void SetNumberOfInputPorts(int n);

  /**
   * Set the number of output ports provided by the algorithm.
   * This is made public so that it can be called from Python.
   */
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
  vtkPythonAlgorithm(const vtkPythonAlgorithm&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPythonAlgorithm&) VTK_DELETE_FUNCTION;

  int CheckResult(const char* method, const vtkSmartPyObject& res);

  PyObject* Object;
};

#endif
