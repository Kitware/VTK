/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphAlgorithm.h

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
// .NAME vtkGraphAlgorithm - Superclass for algorithms that produce only graph as output
//
// .SECTION Description
// vtkGraphAlgorithm is a convenience class to make writing algorithms
// easier. It is also designed to help transition old algorithms to the new
// pipeline architecture. There are some assumptions and defaults made by this
// class you should be aware of. This class defaults such that your filter
// will have one input port and one output port. If that is not the case
// simply change it with SetNumberOfInputPorts etc. See this class
// constructor for the default. This class also provides a FillInputPortInfo
// method that by default says that all inputs will be Graph. If that
// isn't the case then please override this method in your subclass.
// You should implement the subclass's algorithm into
// RequestData( request, inputVec, outputVec).
//
// .SECTION Thanks
// Thanks to Patricia Crossno, Ken Moreland, Andrew Wilson and Brian Wylie from
// Sandia National Laboratories for their help in developing this class.

#ifndef vtkGraphAlgorithm_h
#define vtkGraphAlgorithm_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkAlgorithm.h"
#include "vtkGraph.h" // makes things a bit easier

class vtkDataSet;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkGraphAlgorithm : public vtkAlgorithm
{
public:
  static vtkGraphAlgorithm *New();
  vtkTypeMacro(vtkGraphAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkGraph* GetOutput() { return this->GetOutput(0); }
  vtkGraph* GetOutput(int index);

  // Description:
  // Assign a data object as input. Note that this method does not
  // establish a pipeline connection. Use SetInputConnection() to
  // setup a pipeline connection.
  void SetInputData(vtkDataObject * obj) { this->SetInputData(0, obj); }
  void SetInputData(int index, vtkDataObject* obj);

protected:
  vtkGraphAlgorithm();
  ~vtkGraphAlgorithm();

  // convenience method
  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);

  virtual int RequestDataObject(vtkInformation* request,
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
  vtkGraphAlgorithm(const vtkGraphAlgorithm&);  // Not implemented.
  void operator=(const vtkGraphAlgorithm&);  // Not implemented.
};

#endif
