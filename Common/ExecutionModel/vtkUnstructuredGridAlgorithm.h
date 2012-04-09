/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkUnstructuredGridAlgorithm - Superclass for algorithms that produce only unstructured grid as output
// .SECTION Description

// vtkUnstructuredGridAlgorithm is a convenience class to make writing algorithms
// easier. It is also designed to help transition old algorithms to the new
// pipeline architecture. There are some assumptions and defaults made by this
// class you should be aware of. This class defaults such that your filter
// will have one input port and one output port. If that is not the case
// simply change it with SetNumberOfInputPorts etc. See this classes
// constructor for the default. This class also provides a FillInputPortInfo
// method that by default says that all inputs will be UnstructuredGrid. If that
// isn't the case then please override this method in your subclass.

#ifndef __vtkUnstructuredGridAlgorithm_h
#define __vtkUnstructuredGridAlgorithm_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkAlgorithm.h"

class vtkDataSet;
class vtkUnstructuredGrid;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkUnstructuredGridAlgorithm : public vtkAlgorithm
{
public:
  static vtkUnstructuredGridAlgorithm *New();
  vtkTypeMacro(vtkUnstructuredGridAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkUnstructuredGrid* GetOutput();
  vtkUnstructuredGrid* GetOutput(int);
  virtual void SetOutput(vtkDataObject* d);

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

  // this method is not recommended for use, but lots of old style filters
  // use it
  vtkDataObject *GetInput(int port);
  vtkDataObject *GetInput() { return this->GetInput(0); };
  vtkUnstructuredGrid *GetUnstructuredGridInput(int port);

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
  vtkUnstructuredGridAlgorithm();
  ~vtkUnstructuredGridAlgorithm();

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
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  // see algorithm for more info
  virtual int FillOutputPortInformation(int port, vtkInformation* info);
  virtual int FillInputPortInformation(int port, vtkInformation* info);

private:
  vtkUnstructuredGridAlgorithm(const vtkUnstructuredGridAlgorithm&);  // Not implemented.
  void operator=(const vtkUnstructuredGridAlgorithm&);  // Not implemented.
};

#endif
