/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFourierBandPass.cxx
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
#include "vtkImageFourierBandPass.h"



//----------------------------------------------------------------------------
vtkImageFourierBandPass::vtkImageFourierBandPass()
{
  int idx;
  
  this->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS);
  this->SetOutputScalarType(VTK_FLOAT);
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->LowPass[idx] = this->HighPass[idx] = VTK_LARGE_FLOAT;
    }
}


//----------------------------------------------------------------------------
void vtkImageFourierBandPass::SetHighPass(int num, float *highPass)
{
  int idx;
  
  if (num >= VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro(<< "SetHighPass: Too many elements " << num);
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    this->HighPass[idx] = highPass[idx];
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkImageFourierBandPass::GetHighPass(int num, float *highPass)
{
  int idx;
  
  if ( num >= VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro(<< "SetHighPass: Too many elements " << num);
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    highPass[idx] = this->HighPass[idx];
    }
}

//----------------------------------------------------------------------------
void vtkImageFourierBandPass::SetLowPass(int num, float *lowPass)
{
  int idx;
  
  if ( num >= VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro(<< "SetLowPass: Too many elements " << num);
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    this->LowPass[idx] = lowPass[idx];
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkImageFourierBandPass::GetLowPass(int num, float *lowPass)
{
  int idx;
  
  if ( num >= VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro(<< "SetLowPass: Too many elements " << num);
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    lowPass[idx] = this->LowPass[idx];
    }
}





//----------------------------------------------------------------------------
// Description:
// Sets the axes, but puts the component axis first.
// Note that GetAxes will not return the same set of Axes.
void vtkImageFourierBandPass::SetAxes(int num, int *axes)
{
  int newAxes[VTK_IMAGE_DIMENSIONS];
  int idx;
  
  if (num >= VTK_IMAGE_DIMENSIONS)
    {
    vtkErrorMacro(<< "SetAxes: Too many axes");
    }

  // Add the component axis
  newAxes[0] = VTK_IMAGE_COMPONENT_AXIS;

  // Copy the additional axes
  for (idx = 0; idx < num; ++idx)
    {
    if (axes[idx] == VTK_IMAGE_COMPONENT_AXIS)
      {
      vtkErrorMacro(<< "SetAxes: You can's specify component axis.");
      }
    newAxes[idx + 1] = axes[idx];
    }

  // Call the superclass
  this->vtkImageFilter::SetAxes(num + 1, newAxes);

  // over ride the nuber of axes so execute will only get
  // one complex pixel oer region.
  this->NumberOfAxes = 1;
}


//----------------------------------------------------------------------------
// Description:
// Intercepts the caches UpdateRegion to make the region larger than requested.
// We might as well create both real and imaginary components.
void 
vtkImageFourierBandPass::InterceptCacheUpdate(vtkImageRegion *region)
{
  int min, max;
  
  region->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, min, max);
  if (min < 0 || max > 1)
    {
    vtkErrorMacro(<< "Only two channels to request 0 and 1");
    }
  
  region->SetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, 0, 1);
}


//----------------------------------------------------------------------------
// Description:
// This function zeros a portion of the image.  Zero is assumed
// to be the origin. (1d easy but slow)
void vtkImageFourierBandPass::Execute(vtkImageRegion *inRegion, 
				      vtkImageRegion *outRegion)
{
  int idx;
  float *inPtr = (float *)(inRegion->GetScalarPointer());
  float *outPtr = (float *)(outRegion->GetScalarPointer());
  int *extent, *imageExtent;
  float *aspectRatio;
  int inInc;
  int outInc;
  float temp, freq, mid;
  float sumLow, sumHigh;
  
  // Make sure we have real and imaginary components.
  extent = inRegion->GetExtent();
  if (extent[0] != 0 || extent[1] != 1)
    {
    vtkErrorMacro(<< "Execute: Components mismatch");
    return;
    }
  
  // this filter expects that input is the same type as output (float).
  if (inRegion->GetScalarType() != VTK_FLOAT ||
      outRegion->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: input and output must be floats");
    return;
    }

  imageExtent = inRegion->GetImageExtent();
  aspectRatio = inRegion->GetAspectRatio();
  sumLow = sumHigh = 0.0;
  // Sum up distance squared for each axis (except for component)
  for (idx = 1; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    temp = (float)(extent[2*idx]);
    // Assumes image min is 0.
    mid = (float)(imageExtent[2*idx + 1] + 1) / 2.0;
    // Wrap back to 0.
    if (temp > mid)
      {
      temp = mid + mid - temp;
      }
    // Aspect ratio == 0 implies there is no spatial meaning for this axis
    if (aspectRatio[idx] > 0.0)
      {
      // Convert location into cycles / world unit
      freq = temp / (aspectRatio[idx] * 2.0 * mid);
      // Scale to unit circle (Pass band does not include Component Axis)
      temp = this->LowPass[idx - 1];
      if (temp > 0)
	{
	temp = freq / temp;
	}
      else
	{
	temp = VTK_LARGE_FLOAT;
	}
      sumLow += temp * temp;
      // Scale to unit circle (Pass band does not include Component Axis)
      temp = this->HighPass[idx - 1];
      if (temp > 0)
	{
	temp = freq / temp;
	}
      else
	{
	temp = VTK_LARGE_FLOAT;
	}
      sumHigh += temp * temp;
      }
    }
  
  sumLow = sqrt(sumLow);
  sumHigh = sqrt(sumHigh);
  
  inRegion->GetIncrements(inInc);
  outRegion->GetIncrements(outInc);
  
  if (sumLow > 1.0 && sumHigh < 1.0)
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
















