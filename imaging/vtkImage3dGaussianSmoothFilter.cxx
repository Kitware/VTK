/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImage3dGaussianSmoothFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
#include "vtkImage3dGaussianSmoothFilter.h"

//----------------------------------------------------------------------------
// Description:
// This method sets up the 2 1d filters that perform the convolution.
vtkImage3dGaussianSmoothFilter::vtkImage3dGaussianSmoothFilter()
{
  // create the filter chain 
  this->Filter0 = new vtkImage1dGaussianSmoothFilter;
  this->Filter1 = new vtkImage1dGaussianSmoothFilter;
  this->Filter2 = new vtkImage1dGaussianSmoothFilter;

  this->SetAxes3d(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS, VTK_IMAGE_Z_AXIS);
}


//----------------------------------------------------------------------------
// Description:
// This method sets the kernal. Both axes are the same.  
// A future simple extension could make the kernel eliptical.
void vtkImage3dGaussianSmoothFilter::SetGaussianStdRadius(float std, int rad)
{
  vtkDebugMacro(<< "SetGauss: Std = " << std << ", Radius = " << rad);

  ((vtkImage1dGaussianSmoothFilter *)
   (this->Filter0))->SetGaussianStdRadius(std, rad);
  ((vtkImage1dGaussianSmoothFilter *)
   (this->Filter1))->SetGaussianStdRadius(std, rad);
  ((vtkImage1dGaussianSmoothFilter *)
   (this->Filter2))->SetGaussianStdRadius(std, rad);

  this->Modified();
}
