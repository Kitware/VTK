/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScaleDimension.h
  
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

// .NAME vtkScaleDimension - scales every element in an N-way array along
// one dimension.
//
// .SECTION Description
// Scales every element in an N-way array along one dimension.  The scaling factor
// along this dimension is specified by a scaling vector with the same extents as
// the target dimension.
//
// Inputs:
//   Input port 0: (required) a vtkTypedArray<double> of arbitrary dimension and
//     extents.
//
//   Input port 1: (required) a vtkDenseArray<double> with one dimension.  The extents
//     of the array must match the extents of the port 0 array along the dimension to
//     be scaled.
// 
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkScaleDimension_h
#define __vtkScaleDimension_h

#include <vtkArrayDataAlgorithm.h>

class VTK_TEXT_ANALYSIS_EXPORT vtkScaleDimension :
  public vtkArrayDataAlgorithm
{
public:
  static vtkScaleDimension* New();
  vtkTypeMacro(vtkScaleDimension, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specifies the dimension to be scaled.  Default 0.
  vtkGetMacro(Dimension, int);
  vtkSetMacro(Dimension, int);

  // Description:
  // Controls whether to invert scaling vector values.  Default: false
  vtkSetMacro(Invert, int);
  vtkGetMacro(Invert, int);

//BTX
protected:
  vtkScaleDimension();
  ~vtkScaleDimension();

  virtual int FillInputPortInformation(int, vtkInformation*);

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkScaleDimension(const vtkScaleDimension&); // Not implemented
  void operator=(const vtkScaleDimension&);   // Not implemented

  int Dimension;
  int Invert;
//ETX
};

#endif

