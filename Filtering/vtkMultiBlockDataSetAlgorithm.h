/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockDataSetAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiBlockDataSetAlgorithm - Superclass for algorithms that produce only vtkMultiBlockDataSet as output
// .SECTION Description
// Algorithms that take any type of data object (including composite dataset)
// and produce a vtkMultiBlockDataSet in the output can subclass from this
// class.


#ifndef __vtkMultiBlockDataSetAlgorithm_h
#define __vtkMultiBlockDataSetAlgorithm_h

#include "vtkAlgorithm.h"

class vtkMultiBlockDataSet;

class VTK_FILTERING_EXPORT vtkMultiBlockDataSetAlgorithm : public vtkAlgorithm
{
public:
  static vtkMultiBlockDataSetAlgorithm *New();
  vtkTypeMacro(vtkMultiBlockDataSetAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkMultiBlockDataSet* GetOutput();
  vtkMultiBlockDataSet* GetOutput(int);

  // Description:
  // Set an input of this algorithm. You should not override these
  // methods because they are not the only way to connect a pipeline.
  // Note that these methods support old-style pipeline connections.
  // When writing new code you should use the more general
  // vtkAlgorithm::SetInputConnection().  These methods transform the
  // input index to the input port index, not an index of a connection
  // within a single port.
  void SetInput(vtkDataObject*);
  void SetInput(int, vtkDataObject*);

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation* request, 
                             vtkInformationVector** inputVector, 
                             vtkInformationVector* outputVector);

protected:
  vtkMultiBlockDataSetAlgorithm();
  ~vtkMultiBlockDataSetAlgorithm() {};

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestDataObject(vtkInformation*, 
                                vtkInformationVector**, 
                                vtkInformationVector*) {return 1;};

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestInformation(vtkInformation*, 
                                 vtkInformationVector**, 
                                 vtkInformationVector*) {return 1;};

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestData(vtkInformation*, 
                          vtkInformationVector**, 
                          vtkInformationVector*) {return 1;};
  
  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*)
    {
      return 1;
    };

  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive();

  // see algorithm for more info
  virtual int FillOutputPortInformation(int port, vtkInformation* info);
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  vtkDataObject *GetInput(int port);

private:
  vtkMultiBlockDataSetAlgorithm(const vtkMultiBlockDataSetAlgorithm&);  // Not implemented.
  void operator=(const vtkMultiBlockDataSetAlgorithm&);  // Not implemented.
};

#endif


