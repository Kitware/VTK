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

#include "vtkMultiGroupDataSetAlgorithm.h"

class vtkMultiBlockDataSet;

class VTK_FILTERING_EXPORT vtkMultiBlockDataSetAlgorithm : public vtkMultiGroupDataSetAlgorithm
{
public:
  static vtkMultiBlockDataSetAlgorithm *New();
  vtkTypeRevisionMacro(vtkMultiBlockDataSetAlgorithm,vtkMultiGroupDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkMultiBlockDataSet* GetOutput();
  vtkMultiBlockDataSet* GetOutput(int);

protected:
  vtkMultiBlockDataSetAlgorithm();
  ~vtkMultiBlockDataSetAlgorithm() {};

  // see algorithm for more info
  virtual int FillOutputPortInformation(int port, vtkInformation* info);
  virtual int FillInputPortInformation(int port, vtkInformation* info);

private:
  vtkMultiBlockDataSetAlgorithm(const vtkMultiBlockDataSetAlgorithm&);  // Not implemented.
  void operator=(const vtkMultiBlockDataSetAlgorithm&);  // Not implemented.
};

#endif


