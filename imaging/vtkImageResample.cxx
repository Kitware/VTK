/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageResample.cxx
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
#include "vtkImageResample.h"
#include "vtkImageCache.h"


//----------------------------------------------------------------------------
// Description:
// Constructor: Sets default filter to be identity.
vtkImageResample::vtkImageResample()
{
  int idx;
  
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->MagnificationFactors[idx] = 1.0;
    this->OutputSpacing[idx] = 0.0; // Not Set
    }
}

//----------------------------------------------------------------------------
// Description:
// This method sets up multiple magnify filters (one for each axis).
void vtkImageResample::SetDimensionality(int num)
{
  int idx;
  
  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkErrorMacro(<< "SetDimensionality: " << num << " is too many fitlers.");
    return;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    // Get rid of old filters
    if (this->Filters[idx])
      {
      this->Filters[idx]->Delete();
      }
    // Create new filters
    this->Filters[idx] = vtkImageResample1D::New();
    this->Filters[idx]->SetAxes(this->Axes[idx]);
    // Set ivars
    if (this->MagnificationFactors[idx] != 0.0)
      {
      ((vtkImageResample1D *)
       this->Filters[idx])->SetMagnificationFactor(
	   this->MagnificationFactors[idx]);
      }
    if (this->OutputSpacing[idx] != 0.0)
      {
      ((vtkImageResample1D *)
       this->Filters[idx])->SetOutputSpacing(this->OutputSpacing[idx]);
      }
    }
  
  this->Dimensionality = num;
  this->Modified();
  
  // If the input has already been set, set the pipelines input.
  if (this->Input)
    {
    this->SetInternalInput(this->Input);
    }
}



//----------------------------------------------------------------------------
void vtkImageResample::SetMagnificationFactors(int num, float *factors)
{
  int idx;
  
  // If dimensionality has already been set
  if (this->Dimensionality != num && this->Dimensionality != 0)
    {
    vtkWarningMacro(<< "SetMagnificationFactors: number of axes " << num 
        << " does not match dimensionality " << this->Dimensionality);
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    this->MagnificationFactors[idx] = factors[idx];
    // Invalidate OutputSpacing
    this->OutputSpacing[idx] = 0.0;
    
    if (this->Filters[idx])
      {
      ((vtkImageResample1D *)
       (this->Filters[idx]))->SetMagnificationFactor(factors[idx]);
      }
    }

  this->Modified();
}


//----------------------------------------------------------------------------
void vtkImageResample::SetOutputSpacing(int num, float *spacing)
{
  int idx;
  
  // If dimensionality has already been set
  if (this->Dimensionality != num && this->Dimensionality != 0)
    {
    vtkWarningMacro(<< "SetOutputSpacing: number of axes " << num 
        << " does not match dimensionality " << this->Dimensionality);
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    this->OutputSpacing[idx] = spacing[idx];
    // Invalidate Magnification factors
    this->MagnificationFactors[idx] = 0.0;
    
    if (this->Filters[idx])
      {
      ((vtkImageResample1D *)
       (this->Filters[idx]))->SetOutputSpacing(spacing[idx]);
      }
    }

  this->Modified();
}


//----------------------------------------------------------------------------
void vtkImageResample::GetMagnificationFactors(int num, float *factors)
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











