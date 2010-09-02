/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointwiseMutualInformation.h
  
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

// .NAME vtkPointwiseMutualInformation - Computes pointwise mutual information.
//
// .SECTION Description
// Given an arbitrary-dimension array of doubles, computes the pointwise mutual
// information log(p(i,j,...) / p(i)p(j)p(...)) for each value in the array.
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkPointwiseMutualInformation_h
#define __vtkPointwiseMutualInformation_h

#include "vtkArrayDataAlgorithm.h"

class VTK_TEXT_ANALYSIS_EXPORT vtkPointwiseMutualInformation : public vtkArrayDataAlgorithm
{
public:
  static vtkPointwiseMutualInformation* New();
  vtkTypeMacro(vtkPointwiseMutualInformation, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPointwiseMutualInformation();
  ~vtkPointwiseMutualInformation();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkPointwiseMutualInformation(const vtkPointwiseMutualInformation&); // Not implemented
  void operator=(const vtkPointwiseMutualInformation&);   // Not implemented
//ETX
};

#endif

