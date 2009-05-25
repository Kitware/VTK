/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayNorm.h
  
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

// .NAME vtkArrayNorm - Computes L-norms along one dimension of an array.
//
// .SECTION Description
// Given an input array (vtkTypedArray<double>), computes the L-norm for each
// slice along a user-specified dimension, storing the results in a dense output
// vector (1D vtkDenseArray<double>).  Most useful for matrices.
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkArrayNorm_h
#define __vtkArrayNorm_h

#include "vtkArrayDataAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkArrayNorm : public vtkArrayDataAlgorithm
{
public:
  static vtkArrayNorm* New();
  vtkTypeRevisionMacro(vtkArrayNorm, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Controls the dimension along which norms will be computed.  For input matrices,
  // For input matrices, use "0" (rows) or "1" (columns). Default: 0
  vtkGetMacro(Dimension, int);
  vtkSetMacro(Dimension, int);

  // Description:
  // Controls the L-value.  Default: 2
  vtkGetMacro(L, int);
  void SetL(int value);

protected:
  vtkArrayNorm();
  ~vtkArrayNorm();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkArrayNorm(const vtkArrayNorm&); // Not implemented
  void operator=(const vtkArrayNorm&);   // Not implemented

  int Dimension;
  int L;
};

#endif

