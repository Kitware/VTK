/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGradientDerivative2d.cxx
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
#include "vtkImageGradientDerivative2d.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageGradientDerivative2d fitler.
vtkImageGradientDerivative2d::vtkImageGradientDerivative2d()
{
  this->KernelSize[0] = 3;
  this->KernelSize[1] = 3;

  this->KernelMiddle[0] = 1;
  this->KernelMiddle[1] = 1;

  this->LowerThreshold = 0.0;
  
  this->SetAxes(VTK_IMAGE_X_AXIS,VTK_IMAGE_Y_AXIS);
  this->SetOutputDataType(VTK_FLOAT);
}


//----------------------------------------------------------------------------
void 
vtkImageGradientDerivative2d::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkImageSpatialFilter::PrintSelf(os, indent);
}



//----------------------------------------------------------------------------
// Description:
// Add Component as the third axis.
void vtkImageGradientDerivative2d::SetAxes(int axis0, int axis1)
{
  if (axis0 == VTK_IMAGE_COMPONENT_AXIS || axis1 == VTK_IMAGE_COMPONENT_AXIS)
    {
    vtkErrorMacro(<< "SetAxes: Cannot use Component as an axis");
    return;
    }
  this->vtkImageFilter::SetAxes(axis0, axis1, VTK_IMAGE_COMPONENT_AXIS);
}


//----------------------------------------------------------------------------
// Description:
// Both components will always be generated.
void vtkImageGradientDerivative2d::InterceptCacheUpdate(
						 vtkImageRegion *region)
{
  int extent[6];
  
  region->GetExtent(extent, 3);
  extent[4] = 0;
  extent[5] = 1;
  region->SetExtent(extent, 3);
}



//----------------------------------------------------------------------------
// Description:
// This method executes the filter for the pixels of the image which
// are not affected by boundaries.  Component axis is axis2.  
// GradientDerivative is performed over axis0 and axis1.
void vtkImageGradientDerivative2d::ExecuteCenter3d(
						    vtkImageRegion *inRegion,
						    vtkImageRegion *outRegion)
{
  float phase;
  // For looping though output (and input) pixels.
  int outMin0, outMax0, outMin1, outMax1;
  int outIdx0, outIdx1;
  int outInc0, outInc1, outInc2;
  float *outPtr0, *outPtr1;
  int inInc0, inInc1, inInc2;
  float *inPtr0, *inPtr1;
  int neighbor;

  // This filter expects that output and input is type float.
  if (outRegion->GetDataType() != VTK_FLOAT ||
      inRegion->GetDataType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: output DataType, "
                  << vtkImageDataTypeNameMacro(outRegion->GetDataType())
                  << ", must be float");
    return;
    }

  // Get information to march through data
  inRegion->GetIncrements(inInc0, inInc1, inInc2); 
  outRegion->GetIncrements(outInc0, outInc1, outInc2); 
  outRegion->GetExtent(outMin0, outMax0, outMin1, outMax1);
  
  // We want the input pixel to correspond to output
  inPtr1 = (float *)(inRegion->GetScalarPointer(outMin0, outMin1, 0));
  outPtr1 = (float *)(outRegion->GetScalarPointer());

  // loop through pixels of output
  for (outIdx1 = outMin1; outIdx1 <= outMax1; ++outIdx1)
    {
    outPtr0 = outPtr1;
    inPtr0 = inPtr1;
    for (outIdx0 = outMin0; outIdx0 <= outMax0; ++outIdx0)
      {
	
      // Use phase to determine which 2 of 8 pixels are neighbors
      phase = inPtr0[inInc2];
      neighbor = 0;
      // phase is up or down
      if (phase > 0.39269908 && phase < 2.7488936)
	{
	neighbor = +inInc1;
	}
      else if (phase < -0.39269908 && phase > -2.7488936)
	{
	neighbor = -inInc1;
	}
      // phase is left or right
      if (phase > -1.1780972 && phase < 1.1780972)
	{
	neighbor += inInc0;
	}
      else if (phase > 1.9634954 || phase < -1.9634954)
	{
	neighbor -= inInc0;
	}
	
      // Set Magnitude
      if ( *inPtr0 > this->LowerThreshold)
	{
	*outPtr0 = inPtr0[neighbor] - inPtr0[-neighbor];
	}
      else
	{
	*outPtr0 = 0.0;
	}

      // Set Phase
      outPtr0[outInc2] = inPtr0[inInc2];
      
      outPtr0 += outInc0;
      inPtr0 += inInc0;
      }
    outPtr1 += outInc1;
    inPtr1 += inInc1;
    }
}


//----------------------------------------------------------------------------
// Description:
// This method executes the filter for boundary pixels.
void vtkImageGradientDerivative2d::Execute3d(vtkImageRegion *inRegion,
						   vtkImageRegion *outRegion)
{
  float phase;
  int inImageMin0, inImageMax0, inImageMin1, inImageMax1;
  // For looping though output (and input) pixels.
  int outMin0, outMax0, outMin1, outMax1;
  int outIdx0, outIdx1;
  int outInc0, outInc1, outInc2;
  float *outPtr0, *outPtr1;
  int inInc0, inInc1, inInc2;
  float *inPtr0, *inPtr1;
  int neighborA, neighborB;

  // This filter expects that output and input is type float.
  if (outRegion->GetDataType() != VTK_FLOAT ||
      inRegion->GetDataType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: output DataType, "
                  << vtkImageDataTypeNameMacro(outRegion->GetDataType())
                  << ", must be float");
    return;
    }

  // Get information to march through data
  inRegion->GetIncrements(inInc0, inInc1, inInc2); 
  outRegion->GetIncrements(outInc0, outInc1, outInc2); 
  outRegion->GetExtent(outMin0, outMax0, outMin1, outMax1);
  
  // For checking boundary conditions.
  inRegion->GetImageExtent(inImageMin0,inImageMax0, inImageMin1,inImageMax1);
  
  // We want the input pixel to correspond to output
  inPtr1 = (float *)(inRegion->GetScalarPointer(outMin0, outMin1, 0));
  outPtr1 = (float *)(outRegion->GetScalarPointer());
  
  // loop through pixels of output
  for (outIdx1 = outMin1; outIdx1 <= outMax1; ++outIdx1)
    {
    outPtr0 = outPtr1;
    inPtr0 = inPtr1;
    for (outIdx0 = outMin0; outIdx0 <= outMax0; ++outIdx0)
      {
	
      // Use phase to determine which 2 of 8 pixels are neighbors
      phase = inPtr0[inInc2];
      neighborA = neighborB = 0;
      // phase is up or down
      if (phase > 0.39269908 && phase < 2.7488936)
	{
	if (outIdx1 < inImageMax1)
	  {
	  neighborA = +inInc1;
	  }
	if (outIdx0 > inImageMin1)
	  {
	  neighborB = -inInc1;
	  }
	}
      else if (phase < -0.39269908 && phase > -2.7488936)
	{
	if (outIdx1 < inImageMax1)
	  {
	  neighborB = +inInc1;
	  }
	if (outIdx1 > inImageMin1)
	  {
	  neighborA = -inInc1;
	  }
	}
      // phase is left or right
      if (phase > -1.1780972 && phase < 1.1780972)
	{
	if (outIdx0 < inImageMax0)
	  {
	  neighborA += inInc0;
	  }
	if (outIdx0 > inImageMin0)
	  {
	  neighborB -= inInc0;
	  }
	}
      else if (phase > 1.9634954 || phase < -1.9634954)
	{
	if (outIdx0 < inImageMax0)
	  {
	  neighborB += inInc0;
	  }
	if (outIdx0 > inImageMin0)
	  {
	  neighborA -= inInc0;
	  }
	}
	
      // Set Magnitude
      if ( *inPtr0 > this->LowerThreshold)
	{
	*outPtr0 = inPtr0[neighborA] - inPtr0[-neighborB];
	}
      else
	{
	*outPtr0 = 0.0;
	}

      // Set Phase
      outPtr0[outInc2] = inPtr0[inInc2];
      
      outPtr0 += outInc0;
      inPtr0 += inInc0;
      }
    outPtr1 += outInc1;
    inPtr1 += inInc1;
    }
}




