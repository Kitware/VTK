/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridToPolyDataAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkUnstructuredGridToPolyDataAlgorithm - subclass of vtkPolyDataAlgorithm that takes vtkDataSet as input
// .SECTION Description
// The only thing overriden from vtkPolyDataAlgorithm in this class is the
// method FillInputPortInformation. It sets the required input data type to
// vtkUnstructuredGrid instead of vtkPolyData.

#ifndef __vtkUnstructuredGridToPolyDataAlgorithm_h
#define __vtkUnstructuredGridToPolyDataAlgorithm_h

#include "vtkPolyDataAlgorithm.h"

class VTK_FILTERING_EXPORT vtkUnstructuredGridToPolyDataAlgorithm : public vtkPolyDataAlgorithm
{
public:
  static vtkUnstructuredGridToPolyDataAlgorithm* New();
  vtkTypeRevisionMacro(vtkUnstructuredGridToPolyDataAlgorithm, vtkPolyDataAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent);

protected:
  vtkUnstructuredGridToPolyDataAlgorithm() {}
  ~vtkUnstructuredGridToPolyDataAlgorithm() {}

  virtual int FillInputPortInformation(int, vtkInformation *);

private:
  vtkUnstructuredGridToPolyDataAlgorithm(const vtkUnstructuredGridToPolyDataAlgorithm&);  // Not implemented.
  void operator=(const vtkUnstructuredGridToPolyDataAlgorithm&);  // Not implemented.
};

#endif
