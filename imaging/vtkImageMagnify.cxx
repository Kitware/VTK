/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMagnify.cxx
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
#include "vtkImageMagnify.h"
#include "vtkImageCache.h"


//----------------------------------------------------------------------------
// Description:
// Constructor: Sets default filter to be identity.
vtkImageMagnify::vtkImageMagnify()
{
  int idx;
  vtkImageMagnify1D *filter;

  this->Interpolate = 1;
  for (idx = 0; idx < 4; ++idx)
    {
    filter = vtkImageMagnify1D::New();
    this->Filters[idx] = filter;
    this->MagnificationFactors[idx] = 1;
    filter->SetFilteredAxis(idx);
    }
  // Let the superclass set some superclass variables of the filters.
  this->InitializeFilters();
  this->InitializeParameters();
}

//----------------------------------------------------------------------------
void vtkImageMagnify::InitializeParameters()
{
  int idx, axis;
  
  vtkImageMagnify1D *filter;
  
  for (idx = 0; idx < this->NumberOfFilteredAxes; ++idx)
    {
    axis = this->FilteredAxes[idx];
    filter = (vtkImageMagnify1D *)(this->Filters[axis]);
    filter->SetMagnificationFactor(this->MagnificationFactors[idx]);
    filter->SetInterpolate(this->Interpolate);
    }
}
      
//----------------------------------------------------------------------------
void vtkImageMagnify::SetMagnificationFactors(int num, int *factors)
{
  int idx;
  
  if (num > 4)
    {
    vtkWarningMacro("Too many factors");
    num = 4;
    }
  for (idx = 0; idx < num; ++idx)
    {
    this->MagnificationFactors[idx] = factors[idx];
    }
  // initialize filters wiht ne parameters
  // Sub Filters handle modifies.
  this->InitializeParameters();
}


//----------------------------------------------------------------------------
void vtkImageMagnify::GetMagnificationFactors(int num, int *factors)
{
  int idx;
  
  if (num > 4)
    {
    vtkWarningMacro(<< "GetMagnificationFactors: Asking for too many " << num);
    num = 4;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    factors[idx] = this->MagnificationFactors[idx];
    }
}



//----------------------------------------------------------------------------
void vtkImageMagnify::SetInterpolate(int interpolate)
{
  this->Interpolate = interpolate;
  this->InitializeParameters();
  // Sub filters tak care of modified.
}


//----------------------------------------------------------------------------
void vtkImageMagnify::SetFilteredAxes(int num, int *axes)
{
  this->vtkImageDecomposedFilter::SetFilteredAxes(num, axes);
  this->InitializeParameters();
}











