/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNormalizeMatrixVectors.h
  
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

// .NAME vtkNormalizeMatrixVectors - given a sparse input matrix, produces
// a sparse output matrix with each vector normalized to unit length with respect 
// to a p-norm (default p=2).
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkNormalizeMatrixVectors_h
#define __vtkNormalizeMatrixVectors_h

#include "vtkArrayDataAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkNormalizeMatrixVectors : public vtkArrayDataAlgorithm
{
public:
  static vtkNormalizeMatrixVectors* New();
  vtkTypeMacro(vtkNormalizeMatrixVectors, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Controls whether to normalize row-vectors or column-vectors.  0 = rows, 1 = columns.
  vtkGetMacro(VectorDimension, int);
  vtkSetMacro(VectorDimension, int);

  // Description:
  // Value of p in p-norm normalization, subject to p >= 1.  Default is p=2 (Euclidean norm).
  vtkGetMacro(PValue, double);
  vtkSetMacro(PValue, double);

protected:
  vtkNormalizeMatrixVectors();
  ~vtkNormalizeMatrixVectors();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

  int VectorDimension;
  double PValue;

private:
  vtkNormalizeMatrixVectors(const vtkNormalizeMatrixVectors&); // Not implemented
  void operator=(const vtkNormalizeMatrixVectors&);   // Not implemented
};

#endif

