/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridToPolyDataAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRectilinearGridToPolyDataAlgorithm - subclass of vtkPolyDataAlgorithm that takes vtkDataSet as input
// .SECTION Description
// The only thing overriden from vtkPolyDataAlgorithm in this class is the
// method FillInputPortInformation. It sets the required input data type to
// vtkRectilinearGrid instead of vtkPolyData.

#ifndef __vtkRectilinearGridToPolyDataAlgorithm_h
#define __vtkRectilinearGridToPolyDataAlgorithm_h

#include "vtkPolyDataAlgorithm.h"

class VTK_FILTERING_EXPORT vtkRectilinearGridToPolyDataAlgorithm : public vtkPolyDataAlgorithm
{
public:
  static vtkRectilinearGridToPolyDataAlgorithm* New();
  vtkTypeRevisionMacro(vtkRectilinearGridToPolyDataAlgorithm, vtkPolyDataAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent);

protected:
  vtkRectilinearGridToPolyDataAlgorithm() {}
  ~vtkRectilinearGridToPolyDataAlgorithm() {}

  virtual int FillInputPortInformation(int, vtkInformation *);

private:
  vtkRectilinearGridToPolyDataAlgorithm(const vtkRectilinearGridToPolyDataAlgorithm&);  // Not implemented.
  void operator=(const vtkRectilinearGridToPolyDataAlgorithm&);  // Not implemented.
};

#endif
