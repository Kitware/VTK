/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImage2dFourierBandPassFilter.cxx
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
#include <math.h>
#include "vtkImage2dFourierBandPassFilter.h"



//----------------------------------------------------------------------------
vtkImage2dFourierBandPassFilter::vtkImage2dFourierBandPassFilter()
{
  this->SetAxes3d(VTK_IMAGE_COMPONENT_AXIS, 
		  VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS);
  this->SetOutputDataType(VTK_IMAGE_FLOAT);
  this->SetLowPass(0.0);
  this->SetHighPass(1.5);
}


//----------------------------------------------------------------------------
// Description:
// Sets the non component 2d dimensions of this filter.
void vtkImage2dFourierBandPassFilter::SetAxes2d(int axis0, int axis1)
{
  this->SetAxes3d(VTK_IMAGE_COMPONENT_AXIS, axis0, axis1);
}


//----------------------------------------------------------------------------
// Description:
// Intercepts the caches UpdateRegion to make the region larger than requested.
// We might as well create both real and imaginary components.
void 
vtkImage2dFourierBandPassFilter::InterceptCacheUpdate(vtkImageRegion *region)
{
  int min, max;
  
  region->GetBounds1d(min, max);
  if (min < 0 || max > 1)
    {
    vtkErrorMacro(<< "Only two channels to request 0 and 1");
    }
  
  region->SetBounds1d(0, 1);
}


//----------------------------------------------------------------------------
// Description:
// This function zeros a portion of the image.  Zero is assumed
// to be the origin. (1d easy but slow)
void vtkImage2dFourierBandPassFilter::Execute1d(vtkImageRegion *inRegion, 
						vtkImageRegion *outRegion)
{
  float *inPtr = (float *)(inRegion->GetVoidPointer1d());
  float *outPtr = (float *)(outRegion->GetVoidPointer1d());
  int *bounds, *imageBounds;
  int inInc;
  int outInc;
  float temp, mid;
  float freq;  // pseudo frequency.
  
  // Make sure we have real and imaginary components.
  bounds = inRegion->GetBounds();
  if (bounds[0] != 0 || bounds[1] != 1)
    {
    vtkErrorMacro(<< "Execute1d: Components mismatch");
    return;
    }
  
  // this filter expects that input is the same type as output (float).
  if (inRegion->GetDataType() != VTK_IMAGE_FLOAT ||
      outRegion->GetDataType() != VTK_IMAGE_FLOAT)
    {
    vtkErrorMacro(<< "Execute: input and output must be floats");
    return;
    }

  imageBounds = inRegion->GetImageBounds();
  freq = 0;
  mid = (float)(imageBounds[3]) / 2.0;
  temp = (float)(imageBounds[2]);
  if (temp > mid)
    {
    temp = (float)(bounds[2]) - temp;
    }
  temp = temp / mid;
  freq += temp * temp;

  mid = (float)(imageBounds[5]) / 2.0;
  temp = (float)(bounds[4]);
  if (temp > mid)
    {
    temp = (float)(imageBounds[5]) - temp;
    }
  temp = temp / mid;
  freq += temp * temp;

  freq = sqrt(freq);
  
  inRegion->GetIncrements1d(inInc);
  outRegion->GetIncrements1d(outInc);
  
  if (freq > this->LowPass && freq < this->HighPass)
    {
    *outPtr = *inPtr;
    outPtr[outInc] = inPtr[inInc];
    }
  else
    {
    *outPtr = 0.0;
    outPtr[outInc] = 0.0;
    }
}
















