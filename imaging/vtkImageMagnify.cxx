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
}

//----------------------------------------------------------------------------
// Description:
// This method sets up multiple magnify filters (one for each axis).
void vtkImageMagnify::SetDimensionality(int num)
{
  int idx;
  
  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkErrorMacro(<< "SetDimensionality: " << num << " is too many fitlers.");
    return;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    if (this->Filters[idx])
      {
      this->Filters[idx]->Delete();
      }
    this->Filters[idx] = new vtkImageMagnify1D;
    this->Filters[idx]->SetAxes(this->Axes[idx]);
    // Get the default values for the MagnificationFactors.
    this->MagnificationFactors[idx] 
      = ((vtkImageMagnify1D *)this->Filters[idx])->GetMagnificationFactor();
    }
  
  this->Dimensionality = num;
  this->Modified();
}



//----------------------------------------------------------------------------
void vtkImageMagnify::SetMagnificationFactors(int num, int *factors)
{
  int idx;
  
  for (idx = 0; idx < num; ++idx)
    {
    // Having my own copy simplifies the Get methods.
    this->MagnificationFactors[idx] = factors[idx];
    if (this->Filters[idx])
      {
      ((vtkImageMagnify1D *)
       (this->Filters[idx]))->SetMagnificationFactor(factors[idx]);
      }
    else
      {
      vtkErrorMacro(<< "SetMagnificationFactors: No filter. " 
                    << "Did you SetDimensionality correctly?");
      }
    }
  this->Modified();
}


//----------------------------------------------------------------------------
void vtkImageMagnify::GetMagnificationFactors(int num, int *factors)
{
  int idx;
  
  if (num > this->Dimensionality)
    {
    vtkWarningMacro(<< "GetMagnificationFactors: Asking for too many " << num);
    num = this->Dimensionality;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    factors[idx] = this->MagnificationFactors[idx];
    }
}



//----------------------------------------------------------------------------
void vtkImageMagnify::SetInterpolate(int interpolate)
{
  int idx;
  
  if (this->Dimensionality == 0)
    {
    vtkWarningMacro(<< "SetInterpolate: No Filters. "
                    << "Try SetDimensionality first.");
    }
  
  for (idx = 0; idx < this->Dimensionality; ++idx)
    {
    if ( ! this->Filters[idx])
      {
      vtkWarningMacro(<< "SetStandardDeviation: Filter " << idx << " not set");
      }
    else
      {
      ((vtkImageMagnify1D *)
       (this->Filters[idx]))->SetInterpolate(interpolate);
      }
    }

  this->Modified();
}



//----------------------------------------------------------------------------
int vtkImageMagnify::GetInterpolate()
{
  if ( ! this->Filters[0])
    {
    vtkErrorMacro(<< "GetInterpolate: Filter not set.");
    return 0;
    }
  
  return ((vtkImageMagnify1D *)(this->Filters[0]))->GetInterpolate();
}












