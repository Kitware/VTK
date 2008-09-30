/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatrixColumnNorm.h
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkMatrixColumnNorm - given a sparse input matrix (vtkSparseArray<double>),
// computes the L-norm for each column, storing the results in a dense output vector
// (vtkDenseArray<double>).

// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkMatrixColumnNorm_h
#define __vtkMatrixColumnNorm_h

#include "vtkArrayDataAlgorithm.h"
#include "vtkSetGet.h"

class VTK_INFOVIS_EXPORT vtkMatrixColumnNorm : public vtkArrayDataAlgorithm
{
public:
  static vtkMatrixColumnNorm* New();
  vtkTypeRevisionMacro(vtkMatrixColumnNorm, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Controls the L-value.  Default: 2
  vtkGetMacro(L, int);
  void SetL(int value);

protected:
  vtkMatrixColumnNorm();
  ~vtkMatrixColumnNorm();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkMatrixColumnNorm(const vtkMatrixColumnNorm&); // Not implemented
  void operator=(const vtkMatrixColumnNorm&);   // Not implemented

  int L;
};

#endif

