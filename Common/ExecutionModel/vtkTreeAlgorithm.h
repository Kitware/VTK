/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkTreeAlgorithm - Superclass for algorithms that produce only Tree as output
//
// .SECTION Description
// vtkTreeAlgorithm is a convenience class to make writing algorithms
// easier. It is also designed to help transition old algorithms to the new
// pipeline edgehitecture. There are some assumptions and defaults made by this
// class you should be aware of. This class defaults such that your filter
// will have one input port and one output port. If that is not the case
// simply change it with SetNumberOfInputPorts etc. See this class
// constructor for the default. This class also provides a FillInputPortInfo
// method that by default says that all inputs will be Tree. If that
// isn't the case then please override this method in your subclass.

#ifndef vtkTreeAlgorithm_h
#define vtkTreeAlgorithm_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkAlgorithm.h"
#include "vtkTree.h" // makes things a bit easier

class vtkDataSet;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkTreeAlgorithm : public vtkAlgorithm
{
public:
  static vtkTreeAlgorithm *New();
  vtkTypeMacro(vtkTreeAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkTree* GetOutput() { return this->GetOutput(0); }
  vtkTree* GetOutput(int index);

  // Description:
  // Assign a data object as input. Note that this method does not
  // establish a pipeline connection. Use SetInputConnection() to
  // setup a pipeline connection.
  void SetInputData(vtkDataObject * obj) { this->SetInputData(0, obj); }
  void SetInputData(int index, vtkDataObject* obj);

protected:
  vtkTreeAlgorithm();
  ~vtkTreeAlgorithm();

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
  vtkTreeAlgorithm(const vtkTreeAlgorithm&);  // Not implemented.
  void operator=(const vtkTreeAlgorithm&);  // Not implemented.
};

#endif
