/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToPolyDataAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataSetToPolyDataAlgorithm - subclass of vtkPolyDataAlgorithm that takes vtkDataSet as input
// .SECTION Description
// The only thing overriden from vtkPolyDataAlgorithm in this class is the
// method FillInputPortInformation. It sets the required input data type to
// vtkDataSet instead of vtkPolyData.

#ifndef __vtkDataSetToPolyDataAlgorithm_h
#define __vtkDataSetToPolyDataAlgorithm_h

#include "vtkPolyDataAlgorithm.h"

class VTK_FILTERING_EXPORT vtkDataSetToPolyDataAlgorithm : public vtkPolyDataAlgorithm
{
public:
  static vtkDataSetToPolyDataAlgorithm* New();
  vtkTypeRevisionMacro(vtkDataSetToPolyDataAlgorithm, vtkPolyDataAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent);

protected:
  vtkDataSetToPolyDataAlgorithm() {}
  ~vtkDataSetToPolyDataAlgorithm() {}

  virtual int FillInputPortInformation(int, vtkInformation *);

private:
  vtkDataSetToPolyDataAlgorithm(const vtkDataSetToPolyDataAlgorithm&);  // Not implemented.
  void operator=(const vtkDataSetToPolyDataAlgorithm&);  // Not implemented.
};

#endif
