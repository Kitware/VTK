/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToUnstructuredGridAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataSetToUnstructuredGridAlgorithm - subclass of vtkPolyDataAlgorithm that takes vtkDataSet as input
// .SECTION Description
// The only thing overriden from vtkUnstructuredGridAlgorithm in this class is the
// method FillInputPortInformation. It sets the required input data type to
// vtkDataSet instead of vtkUnstructuredGrid.

#ifndef __vtkDataSetToUnstructuredGridAlgorithm_h
#define __vtkDataSetToUnstructuredGridAlgorithm_h

#include "vtkUnstructuredGridAlgorithm.h"

class VTK_FILTERING_EXPORT vtkDataSetToUnstructuredGridAlgorithm : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkDataSetToUnstructuredGridAlgorithm* New();
  vtkTypeRevisionMacro(vtkDataSetToUnstructuredGridAlgorithm, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent);

protected:
  vtkDataSetToUnstructuredGridAlgorithm() {}
  ~vtkDataSetToUnstructuredGridAlgorithm() {}

  virtual int FillInputPortInformation(int, vtkInformation *);

private:
  vtkDataSetToUnstructuredGridAlgorithm(const vtkDataSetToUnstructuredGridAlgorithm&);  // Not implemented.
  void operator=(const vtkDataSetToUnstructuredGridAlgorithm&);  // Not implemented.
};

#endif
