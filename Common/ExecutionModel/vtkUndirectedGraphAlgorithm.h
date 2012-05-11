/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUndirectedGraphAlgorithm.h

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
// .NAME vtkUndirectedGraphAlgorithm - Superclass for algorithms that produce undirected graph as output
//
// .SECTION Description
// vtkUndirectedGraphAlgorithm is a convenience class to make writing algorithms
// easier. It is also designed to help transition old algorithms to the new
// pipeline edgehitecture. There are some assumptions and defaults made by this
// class you should be aware of. This class defaults such that your filter
// will have one input port and one output port. If that is not the case
// simply change it with SetNumberOfInputPorts etc. See this class
// constructor for the default. This class also provides a FillInputPortInfo
// method that by default says that all inputs will be Graph. If that
// isn't the case then please override this method in your subclass.
//
// .SECTION Thanks
// Thanks to Patricia Crossno, Ken Moreland, Andrew Wilson and Brian Wylie from
// Sandia National Laboratories for their help in developing this class.

#ifndef __vtkUndirectedGraphAlgorithm_h
#define __vtkUndirectedGraphAlgorithm_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkAlgorithm.h"
#include "vtkUndirectedGraph.h" // makes things a bit easier

class vtkDataSet;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkUndirectedGraphAlgorithm : public vtkAlgorithm
{
public:
  static vtkUndirectedGraphAlgorithm *New();
  vtkTypeMacro(vtkUndirectedGraphAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkUndirectedGraph* GetOutput() { return this->GetOutput(0); }
  vtkUndirectedGraph* GetOutput(int index);

  // Description:
  // Assign a data object as input. Note that this method does not
  // establish a pipeline connection. Use SetInputConnection() to
  // setup a pipeline connection.
  void SetInputData(vtkDataObject * obj) { this->SetInputData(0, obj); }
  void SetInputData(int index, vtkDataObject* obj);

protected:
  vtkUndirectedGraphAlgorithm();
  ~vtkUndirectedGraphAlgorithm();

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
  vtkUndirectedGraphAlgorithm(const vtkUndirectedGraphAlgorithm&);  // Not implemented.
  void operator=(const vtkUndirectedGraphAlgorithm&);  // Not implemented.
};

#endif
