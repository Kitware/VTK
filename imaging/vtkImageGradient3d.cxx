/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGradient3d.cxx
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
#include "vtkImageGradient3d.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageGradient3d fitler.
vtkImageGradient3d::vtkImageGradient3d()
{
  this->KernelSize[0] = 3;
  this->KernelSize[1] = 3;
  this->KernelSize[2] = 3;

  this->KernelMiddle[0] = 1;
  this->KernelMiddle[1] = 1;
  this->KernelMiddle[2] = 1;
  
  this->SetAxes(VTK_IMAGE_X_AXIS,VTK_IMAGE_Y_AXIS,VTK_IMAGE_Z_AXIS);
  this->SetOutputDataType(VTK_FLOAT);
  
  this->UseExecuteCenterOff();
}


//----------------------------------------------------------------------------
void vtkImageGradient3d::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkImageSpatialFilter::PrintSelf(os, indent);
}



//----------------------------------------------------------------------------
// Description:
// Add Component as the fourth axis.
void vtkImageGradient3d::SetAxes(int axis0, int axis1, int axis2)
{
  if (axis0 == VTK_IMAGE_COMPONENT_AXIS || axis1 == VTK_IMAGE_COMPONENT_AXIS ||
      axis2 == VTK_IMAGE_COMPONENT_AXIS)
    {
    vtkErrorMacro(<< "SetAxes: Cannot use Component as an axis");
    return;
    }
  this->vtkImageFilter::SetAxes(axis0, axis1, axis2, VTK_IMAGE_COMPONENT_AXIS);
}


//----------------------------------------------------------------------------
// Description:
// All components will be generated.
void vtkImageGradient3d::InterceptCacheUpdate(vtkImageRegion *region)
{
  int extent[8];
  
  region->GetExtent(extent, 4);
  extent[6] = 0;
  extent[7] = 3;
  region->SetExtent(extent, 4);
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a region that holds the image extent of this filters
// input, and changes the region to hold the image extent of this filters
// output.
void vtkImageGradient3d::ComputeOutputImageInformation(
		    vtkImageRegion *inRegion, vtkImageRegion *outRegion)
{
  int extent[8];
  int idx;

  inRegion->GetImageExtent(extent, 4);
  if ( ! this->HandleBoundaries)
    {
    // shrink output image extent.
    for (idx = 0; idx < 4; ++idx)
      {
      extent[idx*2] += this->KernelMiddle[idx];
      extent[idx*2+1] -= (this->KernelSize[idx] - 1) - this->KernelMiddle[idx];
      }
    }
  
  // from 0 to 3 components
  extent[6] = 0;
  extent[7] = 3;

  outRegion->SetImageExtent(extent, 4);
}



//----------------------------------------------------------------------------
// Description:
// This execute method handles boundaries.
// it handles boundaries. Pixels are just replicated to get values 
// out of extent.
template <class T>
void vtkImageGradient4dExecute(vtkImageGradient3d *self,
				     vtkImageRegion *inRegion, T *inPtr, 
				     vtkImageRegion *outRegion, float *outPtr)
{
  float d0, d1, d2;
  float r0, r1, r2;
  float temp;
  // For looping though output (and input) pixels.
  int outMin0, outMax0, outMin1, outMax1, outMin2, outMax2;
  int outIdx0, outIdx1, outIdx2;
  int outInc0, outInc1, outInc2, outInc3;
  float *outPtr0, *outPtr1, *outPtr2, *outPtr3;
  int inInc0, inInc1, inInc2;
  T *inPtr0, *inPtr1, *inPtr2;
  // Boundary of input image
  int inImageMin0,inImageMax0,inImageMin1,inImageMax1,inImageMin2,inImageMax2;
  
  self = self;
  // Get boundary information
  inRegion->GetImageExtent(inImageMin0,inImageMax0, inImageMin1,inImageMax1,
			   inImageMin2,inImageMax2);
  
  // Get information to march through data
  inRegion->GetIncrements(inInc0, inInc1, inInc2); 
  outRegion->GetIncrements(outInc0, outInc1, outInc2, outInc3); 
  outRegion->GetExtent(outMin0, outMax0, outMin1, outMax1, outMin2, outMax2);
  
  // We want the input pixel to correspond to output
  inPtr = (T *)(inRegion->GetScalarPointer(outMin0, outMin1, outMin2));

  // The aspect ratio is important for computing the gradient.
  inRegion->GetAspectRatio(r0, r1, r2);
  r0 = 1.0 / r0;
  r1 = 1.0 / r1;
  r2 = 1.0 / r2;
  
  // loop through pixels of output
  outPtr2 = outPtr;
  inPtr2 = inPtr;
  for (outIdx2 = outMin2; outIdx2 <= outMax2; ++outIdx2)
    {
    outPtr1 = outPtr2;
    inPtr1 = inPtr2;
    for (outIdx1 = outMin1; outIdx1 <= outMax1; ++outIdx1)
      {
      outPtr0 = outPtr1;
      inPtr0 = inPtr1;
      for (outIdx0 = outMin0; outIdx0 <= outMax0; ++outIdx0)
	{
	
	// Compute gradient using central differences (if in extent).
	d0 = ((outIdx0 + 1) > inImageMax0) ? *inPtr0 : inPtr0[inInc0];
	d1 = ((outIdx1 + 1) > inImageMax1) ? *inPtr0 : inPtr0[inInc1];
	d2 = ((outIdx2 + 1) > inImageMax1) ? *inPtr0 : inPtr0[inInc2];
	d0 -= ((outIdx0 - 1) < inImageMin0) ? *inPtr0 : inPtr0[-inInc0];
	d1 -= ((outIdx1 - 1) < inImageMin1) ? *inPtr0 : inPtr0[-inInc1];
	d2 -= ((outIdx2 - 1) < inImageMin2) ? *inPtr0 : inPtr0[-inInc2];
	d0 *= r0;
	d1 *= r1;
	d2 *= r2;
      
	// Set the magnitude
	outPtr3 = outPtr0;
	*outPtr3 = (float)(sqrt(d0*d0 + d1*d1 + d2*d2));
	temp = 1.0 / *outPtr3;
	// Set the vector
	outPtr3 += outInc3;
	*outPtr3 = d0 * temp;
	outPtr3 += outInc3;
	*outPtr3 = d1 * temp;
	outPtr3 += outInc3;
	*outPtr3 = d2 * temp;
	
	outPtr0 += outInc0;
	inPtr0 += inInc0;
	}
      outPtr1 += outInc1;
      inPtr1 += inInc1;
      }
    outPtr2 += outInc2;
    inPtr2 += inInc2;
    }
}


//----------------------------------------------------------------------------
// Description:
// This method contains a switch statement that calls the correct
// templated function for the input region type.  The output region
// must be of type float.  This method does handle boundary conditions.
// The third axis is the component axis for the output.
void vtkImageGradient3d::Execute4d(vtkImageRegion *inRegion, 
					 vtkImageRegion *outRegion)
{
  void *inPtr = inRegion->GetScalarPointer();
  void *outPtr = outRegion->GetScalarPointer();
  
  // this filter expects that output is type float.
  if (outRegion->GetDataType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute4d: output DataType, "
                  << vtkImageDataTypeNameMacro(outRegion->GetDataType())
                  << ", must be float");
    return;
    }
  
  switch (inRegion->GetDataType())
    {
    case VTK_FLOAT:
      vtkImageGradient4dExecute(this, 
			  inRegion, (float *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageGradient4dExecute(this, 
			  inRegion, (int *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageGradient4dExecute(this, 
			  inRegion, (short *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageGradient4dExecute(this, 
			  inRegion, (unsigned short *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageGradient4dExecute(this, 
			  inRegion, (unsigned char *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute4d: Unknown DataType");
      return;
    }
}






