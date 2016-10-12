/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDecomposeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageDecomposeFilter.h"

#include <cmath>


//----------------------------------------------------------------------------
// Construct an instance of vtkImageDecomposeFilter fitler.
vtkImageDecomposeFilter::vtkImageDecomposeFilter()
{
  this->Dimensionality = 3;
  this->SetNumberOfIterations(3);
}


//----------------------------------------------------------------------------
void vtkImageDecomposeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Dimensionality: " << this->Dimensionality << "\n";
}

//----------------------------------------------------------------------------
void vtkImageDecomposeFilter::SetDimensionality(int dim)
{
  if (this->Dimensionality == dim)
  {
    return;
  }

  if (dim < 1 || dim > 3)
  {
    vtkErrorMacro("SetDimensionality: Bad dim: " << dim);
    return;
  }

  this->Dimensionality = dim;
  this->SetNumberOfIterations(dim);
  this->Modified();
}


//----------------------------------------------------------------------------
void vtkImageDecomposeFilter::PermuteIncrements(vtkIdType *increments,
                                                vtkIdType &inc0,
                                                vtkIdType &inc1,
                                                vtkIdType &inc2)
{
  switch (this->Iteration)
  {
    case 0:
      inc0 = increments[0];
      inc1 = increments[1];
      inc2 = increments[2];
      break;
    case 1:
      inc1 = increments[0];
      inc0 = increments[1];
      inc2 = increments[2];
      break;
    case 2:
      inc1 = increments[0];
      inc2 = increments[1];
      inc0 = increments[2];
      break;
  }
}


//----------------------------------------------------------------------------
void vtkImageDecomposeFilter::PermuteExtent(int *extent, int &min0, int &max0,
                                            int &min1, int &max1,
                                            int &min2, int &max2)
{
  switch (this->Iteration)
  {
    case 0:
      min0 = extent[0];       max0 = extent[1];
      min1 = extent[2];       max1 = extent[3];
      min2 = extent[4];       max2 = extent[5];
      break;
    case 1:
      min1 = extent[0];       max1 = extent[1];
      min0 = extent[2];       max0 = extent[3];
      min2 = extent[4];       max2 = extent[5];
      break;
    case 2:
      min1 = extent[0];       max1 = extent[1];
      min2 = extent[2];       max2 = extent[3];
      min0 = extent[4];       max0 = extent[5];
      break;
  }
}


















