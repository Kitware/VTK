/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataToPolyDataAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageDataToPolyDataAlgorithm - subclass of vtkPolyDataAlgorithm that takes vtkDataSet as input
// .SECTION Description
// The only thing overriden from vtkPolyDataAlgorithm in this class is the
// method FillInputPortInformation. It sets the required input data type to
// vtkImageData instead of vtkPolyData.

#ifndef __vtkImageDataToPolyDataAlgorithm_h
#define __vtkImageDataToPolyDataAlgorithm_h

#include "vtkPolyDataAlgorithm.h"

class VTK_FILTERING_EXPORT vtkImageDataToPolyDataAlgorithm : public vtkPolyDataAlgorithm
{
public:
  static vtkImageDataToPolyDataAlgorithm* New();
  vtkTypeRevisionMacro(vtkImageDataToPolyDataAlgorithm, vtkPolyDataAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent);

protected:
  vtkImageDataToPolyDataAlgorithm() {}
  ~vtkImageDataToPolyDataAlgorithm() {}

  virtual int FillInputPortInformation(int, vtkInformation *);

private:
  vtkImageDataToPolyDataAlgorithm(const vtkImageDataToPolyDataAlgorithm&);  // Not implemented.
  void operator=(const vtkImageDataToPolyDataAlgorithm&);  // Not implemented.
};

#endif
