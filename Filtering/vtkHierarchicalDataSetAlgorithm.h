/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataSetAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalDataSetAlgorithm - Superclass for algorithms that produce only vtkHierarchicalDataSet as output
// .SECTION Description
// Algorithms that take any type of data object (including composite dataset)
// and produce a vtkHierarchicalDataSet in the output can subclass from this
// class.


#ifndef __vtkHierarchicalDataSetAlgorithm_h
#define __vtkHierarchicalDataSetAlgorithm_h

#include "vtkAlgorithm.h"

class vtkHierarchicalDataSet;

class VTK_FILTERING_EXPORT vtkHierarchicalDataSetAlgorithm : public vtkAlgorithm
{
public:
  static vtkHierarchicalDataSetAlgorithm *New();
  vtkTypeRevisionMacro(vtkHierarchicalDataSetAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkHierarchicalDataSet* GetOutput();
  vtkHierarchicalDataSet* GetOutput(int);

  // Description:
  // Set an input of this algorithm.
  void SetInput(vtkDataObject*);
  void SetInput(int, vtkDataObject*);

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation* request, 
                             vtkInformationVector** inputVector, 
                             vtkInformationVector* outputVector);

protected:
  vtkHierarchicalDataSetAlgorithm();
  ~vtkHierarchicalDataSetAlgorithm() {};

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
  vtkHierarchicalDataSetAlgorithm(const vtkHierarchicalDataSetAlgorithm&);  // Not implemented.
  void operator=(const vtkHierarchicalDataSetAlgorithm&);  // Not implemented.
};

#endif


