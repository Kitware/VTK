/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGradient.cxx
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
#include "vtkImageGradient.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageGradient fitler.
vtkImageGradient::vtkImageGradient()
{
  this->SetAxes(VTK_IMAGE_X_AXIS,VTK_IMAGE_Y_AXIS);
  this->SetOutputScalarType(VTK_FLOAT);
  this->HandleBoundariesOn();
}


//----------------------------------------------------------------------------
void vtkImageGradient::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkImageFilter::PrintSelf(os, indent);
  if (this->ComputeMagnitude)
    {
    os << indent << "Output magnitude\n";
    }
  else
    {
    os << indent << "Output vector\n";
    }
  os << indent << "HandleBoundaries: " << this->HandleBoundaries << "\n";
}



//----------------------------------------------------------------------------
void vtkImageGradient::SetAxes(int num, int *axes)
{
  int idx;
  int newAxes[VTK_IMAGE_DIMENSIONS];
  
  if (num > 4)
    {
    vtkErrorMacro(<< "SetAxes: too many axes.");
    num = 4;
    }

  // Copy the axes
  for (idx = 0; idx < num; ++idx)
    {
    if (axes[idx] == VTK_IMAGE_COMPONENT_AXIS)
      {
      vtkErrorMacro(<< "SetAxes: Cannot compute gradient on component axis.");
      return;
      }
    newAxes[idx] = axes[idx];
    }
  // Add VTK_IMAGE_COMPONENT_AXIS as the last axis so the supper class
  // will not loop over components.
  newAxes[num] = VTK_IMAGE_COMPONENT_AXIS;
  this->vtkImageFilter::SetAxes(num+1, axes);
}


//----------------------------------------------------------------------------
// Description:
// All components will be generated.
void vtkImageGradient::InterceptCacheUpdate(vtkImageRegion *region)
{
  int extent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int componentAxisIdx;
  
  region->GetExtent(VTK_IMAGE_DIMENSIONS, extent);
  // Component axis is the last axis.
  componentAxisIdx = this->Dimensionality - 1;
  
  extent[componentAxisIdx * 2] = 0;
  if (this->ComputeMagnitude)
    {
    extent[componentAxisIdx * 2 + 1] = 0;
    }
  else
    {
    extent[componentAxisIdx * 2 + 1] = componentAxisIdx;
    }

  region->SetExtent(VTK_IMAGE_DIMENSIONS, extent);
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a region that holds the image extent of this filters
// input, and changes the region to hold the image extent of this filters
// output.
void vtkImageGradient::ComputeOutputImageInformation(vtkImageRegion *inRegion,
						     vtkImageRegion *outRegion)
{
  int extent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int componentAxisIdx;
  int idx;

  inRegion->GetImageExtent(VTK_IMAGE_DIMENSIONS, extent);
  // Component axis is the last axis.
  componentAxisIdx = this->Dimensionality - 1;
  if ( ! this->HandleBoundaries)
    {
    // shrink output image extent.
    for (idx = 0; idx < componentAxisIdx; ++idx)
      {
      extent[idx*2] += 1;
      extent[idx*2+1] -= 1;
      }
    }
  
  extent[componentAxisIdx * 2] = 0;
  if (this->ComputeMagnitude)
    {
    extent[componentAxisIdx * 2 + 1] = 0;
    }
  else
    {
    extent[componentAxisIdx * 2 + 1] = componentAxisIdx;
    }

  outRegion->SetImageExtent(VTK_IMAGE_DIMENSIONS, extent);
}


//----------------------------------------------------------------------------
// Description:
// This method computes the input extent necessary to generate the output.
void vtkImageGradient::ComputeRequiredInputRegionExtent(
			vtkImageRegion *outRegion, vtkImageRegion *inRegion)
{
  int extent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int *imageExtent;
  int componentAxisIdx;
  int idx;

  imageExtent = inRegion->GetImageExtent();
  outRegion->GetExtent(VTK_IMAGE_DIMENSIONS, extent);
  // Component axis is the last axis.
  componentAxisIdx = this->Dimensionality - 1;
  extent[componentAxisIdx * 2] = 0;
  extent[componentAxisIdx * 2 + 1] = 0;
  
  // grow input image extent.
  for (idx = 0; idx < componentAxisIdx; ++idx)
    {
    extent[idx*2] -= 1;
    extent[idx*2+1] += 1;
    if (this->HandleBoundaries)
      {
      // we must clip extent with image extent is we hanlde boundaries.
      if (extent[idx*2] < imageExtent[idx*2])
	{
	extent[idx*2] = imageExtent[idx*2];
	}
      if (extent[idx*2 + 1] > imageExtent[idx*2 + 1])
	{
	extent[idx*2 + 1] = imageExtent[idx*2 + 1];
	}
      }
    }
  
  inRegion->SetExtent(VTK_IMAGE_DIMENSIONS, extent);
}





//----------------------------------------------------------------------------
// Description:
// This execute method handles boundaries.
// it handles boundaries. Pixels are just replicated to get values 
// out of extent.
template <class T>
void vtkImageGradientExecute(vtkImageGradient *self,
			     vtkImageRegion *inRegion, T *inPtr, 
			     vtkImageRegion *outRegion, float *outPtr)
{
  int axisIdx, axesNum, magnitudeFlag;
  float d, sum;
  float r[4];
  // For looping though output (and input) pixels.
  int outMin0, outMax0, outMin1, outMax1, outMin2, outMax2, outMin3, outMax3;
  int outIdx0, outIdx1, outIdx2, outIdx3;
  int outInc0, outInc1, outInc2, outInc3, outInc4;
  float *outPtr0, *outPtr1, *outPtr2, *outPtr3, *outPtr4;
  int inInc0, inInc1, inInc2, inInc3;
  T *inPtr0, *inPtr1, *inPtr2, *inPtr3;
  // Boundary of input image
  int inImageMin0, inImageMax0, inImageMin1, inImageMax1;
  int inImageMin2, inImageMax2, inImageMin3, inImageMax3;
  
  // Get the dimensionality of the gradient.
  axesNum = self->GetDimensionality() - 1;
  
  // Are we computing magnitude or vectors?
  magnitudeFlag = self->GetComputeMagnitude();
  
  // Get boundary information
  inRegion->GetImageExtent(inImageMin0,inImageMax0, inImageMin1,inImageMax1,
			   inImageMin2,inImageMax2, inImageMin3,inImageMax3);
  
  // Get information to march through data
  inRegion->GetIncrements(inInc0, inInc1, inInc2, inInc3); 
  outRegion->GetIncrements(outInc0, outInc1, outInc2, outInc3, outInc4); 
  outRegion->GetExtent(outMin0, outMax0, outMin1, outMax1,
		       outMin1, outMax1, outMin2, outMax2);
    
  // We want the input pixel to correspond to output
  inPtr = (T *)(inRegion->GetScalarPointer(outMin0,outMin1,outMin2,outMin3));

  // The aspect ratio is important for computing the gradient.
  inRegion->GetAspectRatio(4, r);
  r[0] = 1.0 / r[0];
  r[1] = 1.0 / r[1];
  r[2] = 1.0 / r[2];
  r[3] = 1.0 / r[3];
  
  // loop through pixels of output
  outPtr3 = outPtr;
  inPtr3 = inPtr;
  for (outIdx3 = outMin3; outIdx3 <= outMax3; ++outIdx3)
    {
    outPtr2 = outPtr3;
    inPtr2 = inPtr3;
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

	  // compute magnitude or vector.
	  sum = 0.0;
	  outPtr4 = outPtr0;
	  for(axisIdx = 0; axisIdx < axesNum; ++axisIdx)
	    {
	    // Compute difference using central differences (if in extent).
	    d = ((outIdx0 + 1) > inImageMax0) ? *inPtr0 : inPtr0[inInc0];
	    d -= ((outIdx0 - 1) < inImageMin0) ? *inPtr0 : inPtr0[-inInc0];
	    d *= r[axisIdx];
	    if (magnitudeFlag)
	      {
	      sum += d * d;
	      }
	    else
	      {
	      *outPtr4 = d;
	      outPtr4 += outInc4;
	      }
	    }
	    if (magnitudeFlag)
	      {
	      *outPtr0 = sqrt(sum);
	      }
	    
	    outPtr0 += outInc0;
	    inPtr0 += inInc0;
	  }
	outPtr1 += outInc1;
	inPtr1 += inInc1;
	}
      outPtr2 += outInc2;
      inPtr2 += inInc2;
      }
    outPtr3 += outInc3;
    inPtr3 += inInc3;
    }
}


//----------------------------------------------------------------------------
// Description:
// This method contains a switch statement that calls the correct
// templated function for the input region type.  The output region
// must be of type float.  This method does handle boundary conditions.
// The third axis is the component axis for the output.
void vtkImageGradient::Execute(vtkImageRegion *inRegion, 
			       vtkImageRegion *outRegion)
{
  void *inPtr = inRegion->GetScalarPointer();
  void *outPtr = outRegion->GetScalarPointer();
  
  // this filter expects that output is type float.
  if (outRegion->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: output ScalarType, "
                  << vtkImageScalarTypeNameMacro(outRegion->GetScalarType())
                  << ", must be float");
    return;
    }
  
  switch (inRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageGradientExecute(this, 
			  inRegion, (float *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageGradientExecute(this, 
			  inRegion, (int *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageGradientExecute(this, 
			  inRegion, (short *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageGradientExecute(this, 
			  inRegion, (unsigned short *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageGradientExecute(this, 
			  inRegion, (unsigned char *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}






