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
// .NAME vtkHierarchicalDataSetAlgorithm -
// .SECTION Description


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

  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestCompositeInformation(vtkInformation*, 
                                          vtkInformationVector**, 
                                          vtkInformationVector*) {return 1;};

  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestCompositeData(vtkInformation*, 
                                   vtkInformationVector**, 
                                   vtkInformationVector*) {return 1;};

  // This is called by the superclass.
  // This is the method you should override.
  virtual int ComputeCompositeInputUpdateExtent(vtkInformation*,
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


