/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageNonMaximalSuppression3d.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkImageNonMaximalSuppression3d.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageNonMaximalSuppression3d fitler.
vtkImageNonMaximalSuppression3d::vtkImageNonMaximalSuppression3d()
{
  this->KernelSize[0] = 3;
  this->KernelSize[1] = 3;
  this->KernelSize[2] = 3;

  this->KernelMiddle[0] = 1;
  this->KernelMiddle[1] = 1;
  this->KernelMiddle[2] = 1;
  
  this->SetAxes(VTK_IMAGE_X_AXIS,VTK_IMAGE_Y_AXIS,VTK_IMAGE_Z_AXIS);
  
  this->SetOutputDataType(VTK_FLOAT);
}


//----------------------------------------------------------------------------
// Description:
// Add Component as the third axis.
void 
vtkImageNonMaximalSuppression3d::SetAxes(int axis0,int axis1,int axis2)
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
void vtkImageNonMaximalSuppression3d::InterceptCacheUpdate(
						 vtkImageRegion *region)
{
  int extent[8];
  
  region->GetExtent(extent, 4);
  extent[6] = 0;
  extent[7] = 3;
  region->SetExtent(extent, 4);
}



//----------------------------------------------------------------------------
// Description:
// This method executes the filter for the pixels of the image which
// are not affected by boundaries.  Component axis is axis3,  
// NonMaximalSuppression is performed over axis0, axis1 and axis2.
void vtkImageNonMaximalSuppression3d::ExecuteCenter4d(
						    vtkImageRegion *inRegion,
						    vtkImageRegion *outRegion)
{
  float d0, d1, d2;
  // For looping though output (and input) pixels.
  int outMin0, outMax0, outMin1, outMax1, outMin2, outMax2;
  int outIdx0, outIdx1, outIdx2;
  int outInc0, outInc1, outInc2, outInc3;
  float *outPtr0, *outPtr1, *outPtr2, *outPtr3;
  int inInc0, inInc1, inInc2, inInc3;
  float *inPtr0, *inPtr1, *inPtr2, *inPtr3;
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
  inRegion->GetIncrements(inInc0, inInc1, inInc2, inInc3); 
  outRegion->GetIncrements(outInc0, outInc1, outInc2, outInc3); 
  outRegion->GetExtent(outMin0, outMax0, outMin1, outMax1, outMin2, outMax2);
  
  // We want the input pixel to correspond to output
  inPtr2 = (float *)(inRegion->GetScalarPointer(outMin0, outMin1, outMin2, 0));
  outPtr2 = (float *)(outRegion->GetScalarPointer());

  // loop through pixels of output
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
	
	// Use phase to determine which 2 of 26 pixels are neighbors
	inPtr3 = inPtr0 + inInc3;
	d0 = *inPtr3;
	inPtr3 += inInc3;
	d1 = *inPtr3;
	inPtr3 += inInc3;
	d2 = *inPtr3;
      
	neighbor = 0;
	// phase is in or out
	if (d2 > 0.38268343)  // sin 22.5 degrees
	  {
	  neighbor = +inInc2;
	  }
	else if (d2 < -0.38268343)
	  {
	  neighbor = -inInc2;
	  }
	// phase is up or down
	if (d1 > 0.38268343)  // sin 22.5 degrees
	  {
	  neighbor += inInc1;
	  }
	else if (d1 < -0.38268343)
	  {
	  neighbor -= inInc1;
	  }
	// phase is left or right
	if (d0 > 0.38268343)  
	  {
	  neighbor += inInc0;
	  }
	else if (d0 < -0.38268343)  
	  {
	  neighbor -= inInc0;
	  }
	
	// Set Magnitude
	if (inPtr0[neighbor] > *inPtr0 || inPtr0[-neighbor] > *inPtr0)
	  {
	  *outPtr0 = 0.0;
	  }
	else
	  {
	  *outPtr0 = *inPtr0;
	  // also check for them being equal is neighbor with larger ptr
	  if ((neighbor > -neighbor)&&(inPtr0[neighbor] == *inPtr0))
	    {
	    *outPtr0 = 0.0;
	    }
	  if ((-neighbor > neighbor)&&(inPtr0[-neighbor] == *inPtr0))
	    {
	    *outPtr0 = 0.0;
	    }
	  }
	
	// Set Direction
	outPtr3 = outPtr0 + outInc3;
	*outPtr3 = d0;
	outPtr3 += outInc3;
	*outPtr3 = d1;
	outPtr3 += outInc3;
	*outPtr3 = d2;
	
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
// This method executes the filter for boundary pixels.
void vtkImageNonMaximalSuppression3d::Execute4d(vtkImageRegion *inRegion,
						     vtkImageRegion *outRegion)
{
  float d0, d1, d2;
  int inImageMin0,inImageMax0,inImageMin1,inImageMax1,inImageMin2,inImageMax2;
  // For looping though output (and input) pixels.
  int outMin0, outMax0, outMin1, outMax1, outMin2, outMax2;
  int outIdx0, outIdx1, outIdx2;
  int outInc0, outInc1, outInc2, outInc3;
  float *outPtr0, *outPtr1, *outPtr2, *outPtr3;
  int inInc0, inInc1, inInc2, inInc3;
  float *inPtr0, *inPtr1, *inPtr2, *inPtr3;
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
  inRegion->GetIncrements(inInc0, inInc1, inInc2, inInc3); 
  outRegion->GetIncrements(outInc0, outInc1, outInc2, outInc3); 
  outRegion->GetExtent(outMin0, outMax0, outMin1, outMax1, outMin2, outMax2);
  
  // For checking boundary conditions.
  inRegion->GetImageExtent(inImageMin0,inImageMax0, inImageMin1,inImageMax1,
			   inImageMin2,inImageMax2);
  
  // We want the input pixel to correspond to output
  inPtr2 = (float *)(inRegion->GetScalarPointer(outMin0, outMin1, outMin2, 0));
  outPtr2 = (float *)(outRegion->GetScalarPointer());
  
  // loop through pixels of output
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
	
	// Use phase to determine which 2 of 26 pixels are neighbors
	inPtr3 = inPtr0 + inInc3;
	d0 = *inPtr3;
	inPtr3 += inInc3;
	d1 = *inPtr3;
	inPtr3 += inInc3;
	d2 = *inPtr3;
	
	neighborA = neighborB = 0;
	// phase is in or out
	if (d2 > 0.38268343)  // sin(22.5 degrees)
	  {
	  if (outIdx2 < inImageMax2)
	    {
	    neighborA = +inInc2;
	    }
	  if (outIdx2 > inImageMin2)
	    {
	    neighborB = -inInc2;
	    }
	  }
	else if (d2 < -0.38268343)
	  {
	  if (outIdx2 < inImageMax2)
	    {
	    neighborB = +inInc2;
	    }
	if (outIdx2 > inImageMin2)
	  {
	  neighborA = -inInc2;
	  }
	  }
	// phase is up or down
	if (d1 > 0.38268343)  // sin(22.5 degrees)
	  {
	  if (outIdx1 < inImageMax1)
	    {
	    neighborA = +inInc1;
	    }
	  if (outIdx1 > inImageMin1)
	    {
	    neighborB = -inInc1;
	    }
	  }
	else if (d1 < -0.38268343)
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
	if (d0 > 0.38268343)
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
	else if (d0 < -0.38268343)
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
	if (inPtr0[neighborA] > *inPtr0 || inPtr0[neighborB] > *inPtr0)
	  {
	  *outPtr0 = 0.0;
	  }
	else
	  {
	  *outPtr0 = *inPtr0;
	  // also check for them being equal is neighbor with larger ptr
	  if ((neighborA > neighborB)&&(inPtr0[neighborA] == *inPtr0))
	    {
	    *outPtr0 = 0.0;
	    }
	  if ((neighborB > neighborA)&&(inPtr0[neighborB] == *inPtr0))
	    {
	    *outPtr0 = 0.0;
	    }
	  }
	// Set Direction
	outPtr3 = outPtr0 + outInc3;
	*outPtr3 = d0;
	outPtr3 += outInc3;
	*outPtr3 = d1;
	outPtr3 += outInc3;
	*outPtr3 = d2;
	
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




