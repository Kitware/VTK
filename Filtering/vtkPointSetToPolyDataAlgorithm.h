/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSetToPolyDataAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPointSetToPolyDataAlgorithm - subclass of vtkPolyDataAlgorithm that takes vtkDataSet as input
// .SECTION Description
// The only thing overriden from vtkPolyDataAlgorithm in this class is the
// method FillInputPortInformation. It sets the required input data type to
// vtkPointSet instead of vtkPolyData.

#ifndef __vtkPointSetToPolyDataAlgorithm_h
#define __vtkPointSetToPolyDataAlgorithm_h

#include "vtkPolyDataAlgorithm.h"

class VTK_FILTERING_EXPORT vtkPointSetToPolyDataAlgorithm : public vtkPolyDataAlgorithm
{
public:
  static vtkPointSetToPolyDataAlgorithm* New();
  vtkTypeRevisionMacro(vtkPointSetToPolyDataAlgorithm, vtkPolyDataAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent);

protected:
  vtkPointSetToPolyDataAlgorithm() {}
  ~vtkPointSetToPolyDataAlgorithm() {}

  virtual int FillInputPortInformation(int, vtkInformation *);

private:
  vtkPointSetToPolyDataAlgorithm(const vtkPointSetToPolyDataAlgorithm&);  // Not implemented.
  void operator=(const vtkPointSetToPolyDataAlgorithm&);  // Not implemented.
};

#endif
