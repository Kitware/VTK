/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridToPolyDataAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStructuredGridToPolyDataAlgorithm - subclass of vtkPolyDataAlgorithm that takes vtkDataSet as input
// .SECTION Description
// The only thing overriden from vtkPolyDataAlgorithm in this class is the
// method FillInputPortInformation. It sets the required input data type to
// vtkStructuredGrid instead of vtkPolyData.

#ifndef __vtkStructuredGridToPolyDataAlgorithm_h
#define __vtkStructuredGridToPolyDataAlgorithm_h

#include "vtkPolyDataAlgorithm.h"

class VTK_FILTERING_EXPORT vtkStructuredGridToPolyDataAlgorithm : public vtkPolyDataAlgorithm
{
public:
  static vtkStructuredGridToPolyDataAlgorithm* New();
  vtkTypeRevisionMacro(vtkStructuredGridToPolyDataAlgorithm, vtkPolyDataAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent);

protected:
  vtkStructuredGridToPolyDataAlgorithm() {}
  ~vtkStructuredGridToPolyDataAlgorithm() {}

  virtual int FillInputPortInformation(int, vtkInformation *);

private:
  vtkStructuredGridToPolyDataAlgorithm(const vtkStructuredGridToPolyDataAlgorithm&);  // Not implemented.
  void operator=(const vtkStructuredGridToPolyDataAlgorithm&);  // Not implemented.
};

#endif
