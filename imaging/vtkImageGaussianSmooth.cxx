/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGaussianSmooth.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkImageCache.h"
#include "vtkImageGaussianSmooth.h"

//----------------------------------------------------------------------------
vtkImageGaussianSmooth::vtkImageGaussianSmooth()
{
  int idx;
  vtkImageGaussianSmooth1D *filter;

  for (idx = 0; idx < 4; ++idx)
    {
    filter = vtkImageGaussianSmooth1D::New();
    this->Filters[idx] = filter;
    this->Strides[idx] = 1;
    this->StandardDeviations[idx] = 1;
    this->RadiusFactors[idx] = 2.0;
    filter->SetFilteredAxis(idx);
    }
  // Let the superclass set some superclass variables of the filters.
  this->InitializeFilters();
  this->InitializeParameters();
}

//----------------------------------------------------------------------------
void vtkImageGaussianSmooth::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  this->vtkImageDecomposedFilter::PrintSelf(os, indent);
  
  os << indent << "StandardDeviations: ";
  for (idx = 0; idx < 4; ++idx)
    {
    os << this->StandardDeviations[idx] << " ";
    }
  os << "\n";

  os << indent << "RadiusFactors: ";
  for (idx = 0; idx < 4; ++idx)
    {
    os << this->RadiusFactors[idx] << " ";
    }
  os << "\n";

  os << indent << "Strides: ";
  for (idx = 0; idx < 4; ++idx)
    {
    os << this->Strides[idx] << " ";
    }
  os << "\n";
}


//----------------------------------------------------------------------------
void vtkImageGaussianSmooth::SetFilteredAxes(int num, int *axes)
{
  this->vtkImageDecomposedFilter::SetFilteredAxes(num, axes);
  this->InitializeParameters();
}


//----------------------------------------------------------------------------
void vtkImageGaussianSmooth::InitializeParameters()
{
  int idx, axis;
  vtkImageGaussianSmooth1D *filter;
  
  for (idx = 0; idx < this->NumberOfFilteredAxes; ++idx)
    {
    axis = this->FilteredAxes[idx];
    filter = (vtkImageGaussianSmooth1D *)(this->Filters[axis]);
    filter->SetStride(this->Strides[idx]);
    filter->SetStandardDeviation(this->StandardDeviations[idx]);
    filter->SetRadiusFactor(this->RadiusFactors[idx]);
    }
}
      

//----------------------------------------------------------------------------
void vtkImageGaussianSmooth::SetStandardDeviations(int num, float *stds)
{
  int idx;

  if (num > 4)
    {
    vtkWarningMacro("SetStandardDeviations: " << num << " is too many");
    num = 4;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    this->StandardDeviations[idx] = stds[idx];
    }
  this->InitializeParameters();
  // Modified handled by sub calls
}
//----------------------------------------------------------------------------
void vtkImageGaussianSmooth::SetStandardDeviation(int num, float *stds)
{
  float newStds[4];
  int idx;

  for (idx = 0; idx < 4; ++idx)
    {
    newStds[idx] = stds[idx%num];
    }
  this->SetStandardDeviations(4, newStds);
}
//----------------------------------------------------------------------------
void vtkImageGaussianSmooth::SetRadiusFactors(int num, float *factors)
{
  int idx;

  if (num > 4)
    {
    vtkWarningMacro("SetRadiusFactors: " << num << " is too many");
    num = 4;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    this->RadiusFactors[idx] = factors[idx];
    }
  this->InitializeParameters();
  // Modified handled by sub calls
}
//----------------------------------------------------------------------------
void vtkImageGaussianSmooth::SetStrides(int num, int *strides)
{
  int idx;

  if (num > 4)
    {
    vtkWarningMacro("SetStrides: " << num << " is too many");
    num = 4;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    this->Strides[idx] = strides[idx];
    }
  this->InitializeParameters();
  // Modified handled by sub calls
}





