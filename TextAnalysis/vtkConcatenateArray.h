/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConcatenateArray.h
  
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

// .NAME vtkConcatenateArray - Merge two arrays into one.
//
// .SECTION Description
// Given two input arrays of arbitrary dimension, creates a single, larger output
// array that contains both.  The input arrays will be adjacent to each other within
// the combined output array.
//
// The adjacent dimension can be specified so that e.g: you can control whether two
// matrices are combined row-wise or column-wise.
//
// Both arrays must have the same number of dimensions.  The array extents along the
// adjacent dimension may be different, but all other dimensions must have identical
// extents.
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkConcatenateArray_h
#define __vtkConcatenateArray_h

#include <vtkArrayDataAlgorithm.h>

class VTK_TEXT_ANALYSIS_EXPORT vtkConcatenateArray : public vtkArrayDataAlgorithm
{
public:
  static vtkConcatenateArray* New();
  vtkTypeMacro(vtkConcatenateArray, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Controls the 0-numbered dimension along-which the arrays will be concatenated.
  // Default: 0
  vtkGetMacro(AdjacentDimension, vtkIdType);
  vtkSetMacro(AdjacentDimension, vtkIdType);
  
protected:
  vtkConcatenateArray();
  ~vtkConcatenateArray();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkConcatenateArray(const vtkConcatenateArray&); // Not implemented
  void operator=(const vtkConcatenateArray&);   // Not implemented

  vtkIdType AdjacentDimension;
};

#endif

