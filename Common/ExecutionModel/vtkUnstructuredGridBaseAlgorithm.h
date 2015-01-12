/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridBaseAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkUnstructuredGridBaseAlgorithm - Superclass for algorithms that
// produce only vtkUnstructureGridBase subclasses as output
// .SECTION Description
// vtkUnstructuredGridBaseAlgorithm is a convenience class to make writing
// algorithms easier. There are some assumptions and defaults made by this
// class you should be aware of. This class defaults such that your filter
// will have one input port and one output port. If that is not the case
// simply change it with SetNumberOfInputPorts etc. See this classes
// constructor for the default. This class also provides a FillInputPortInfo
// method that by default says that all inputs will be UnstructuredGridBase. If
// that isn't the case then please override this method in your subclass.

#ifndef vtkUnstructuredGridBaseAlgorithm_h
#define vtkUnstructuredGridBaseAlgorithm_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkAlgorithm.h"

class vtkDataSet;
class vtkUnstructuredGridBase;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkUnstructuredGridBaseAlgorithm
    : public vtkAlgorithm
{
public:
  static vtkUnstructuredGridBaseAlgorithm *New();
  vtkTypeMacro(vtkUnstructuredGridBaseAlgorithm, vtkAlgorithm)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkUnstructuredGridBase* GetOutput();
  vtkUnstructuredGridBase* GetOutput(int);
  virtual void SetOutput(vtkDataObject* d);

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

  // Description:
  // Assign a data object as input. Note that this method does not
  // establish a pipeline connection. Use SetInputConnection() to
  // setup a pipeline connection.
  void SetInputData(vtkDataObject *);
  void SetInputData(int, vtkDataObject*);

  // Description:
  // Assign a data object as input. Note that this method does not
  // establish a pipeline connection. Use SetInputConnection() to
  // setup a pipeline connection.
  void AddInputData(vtkDataObject *);
  void AddInputData(int, vtkDataObject*);

protected:
  vtkUnstructuredGridBaseAlgorithm();
  ~vtkUnstructuredGridBaseAlgorithm();

  // convenience method
  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  // see algorithm for more info
  virtual int FillOutputPortInformation(int port, vtkInformation* info);
  virtual int FillInputPortInformation(int port, vtkInformation* info);

private:
  vtkUnstructuredGridBaseAlgorithm(const vtkUnstructuredGridBaseAlgorithm&); // Not implemented.
  void operator=(const vtkUnstructuredGridBaseAlgorithm&); // Not implemented.
};

#endif
