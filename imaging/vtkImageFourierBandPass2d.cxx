/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFourierBandPass2d.cxx
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
#include <math.h>
#include "vtkImageFourierBandPass2d.h"



//----------------------------------------------------------------------------
vtkImageFourierBandPass2d::vtkImageFourierBandPass2d()
{
  this->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS);
  this->SetOutputDataType(VTK_FLOAT);
  this->SetLowPass(0.0);
  this->SetHighPass(1.5);
}


//----------------------------------------------------------------------------
// Description:
// Sets the non component 2d dimensions of this filter.
void vtkImageFourierBandPass2d::SetAxes(int axis0, int axis1)
{
  this->vtkImageFilter::SetAxes(VTK_IMAGE_COMPONENT_AXIS, axis0, axis1);
}


//----------------------------------------------------------------------------
// Description:
// Intercepts the caches UpdateRegion to make the region larger than requested.
// We might as well create both real and imaginary components.
void 
vtkImageFourierBandPass2d::InterceptCacheUpdate(vtkImageRegion *region)
{
  int min, max;
  
  region->GetExtent(min, max);
  if (min < 0 || max > 1)
    {
    vtkErrorMacro(<< "Only two channels to request 0 and 1");
    }
  
  region->SetExtent(0, 1);
}


//----------------------------------------------------------------------------
// Description:
// This function zeros a portion of the image.  Zero is assumed
// to be the origin. (1d easy but slow)
void vtkImageFourierBandPass2d::Execute(vtkImageRegion *inRegion, 
						vtkImageRegion *outRegion)
{
  float *inPtr = (float *)(inRegion->GetScalarPointer());
  float *outPtr = (float *)(outRegion->GetScalarPointer());
  int *extent, *imageExtent;
  int inInc;
  int outInc;
  float temp, mid;
  float freq;  // pseudo frequency.
  
  // Make sure we have real and imaginary components.
  extent = inRegion->GetExtent();
  if (extent[0] != 0 || extent[1] != 1)
    {
    vtkErrorMacro(<< "Execute: Components mismatch");
    return;
    }
  
  // this filter expects that input is the same type as output (float).
  if (inRegion->GetDataType() != VTK_FLOAT ||
      outRegion->GetDataType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: input and output must be floats");
    return;
    }

  imageExtent = inRegion->GetImageExtent();
  freq = 0;
  mid = (float)(imageExtent[3]) / 2.0;
  temp = (float)(imageExtent[2]);
  if (temp > mid)
    {
    temp = (float)(extent[2]) - temp;
    }
  temp = temp / mid;
  freq += temp * temp;

  mid = (float)(imageExtent[5]) / 2.0;
  temp = (float)(extent[4]);
  if (temp > mid)
    {
    temp = (float)(imageExtent[5]) - temp;
    }
  temp = temp / mid;
  freq += temp * temp;

  freq = sqrt(freq);
  
  inRegion->GetIncrements(inInc);
  outRegion->GetIncrements(outInc);
  
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
















