/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageNonMaximumSuppression.cxx
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
#include "vtkImageNonMaximumSuppression.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageNonMaximumSuppression fitler.
vtkImageNonMaximumSuppression::vtkImageNonMaximumSuppression()
{
  this->SetAxes(VTK_IMAGE_X_AXIS,VTK_IMAGE_Y_AXIS);
  
  this->SetOutputScalarType(VTK_FLOAT);
}

//----------------------------------------------------------------------------
void vtkImageNonMaximumSuppression::SetAxes(int num, int *axes)
{
  int idx, count;
  int newAxes[VTK_IMAGE_DIMENSIONS];
  
  if (num > 4)
    {
    vtkErrorMacro(<< "SetAxes: too many axes.");
    num = 4;
    }
  
  // Save the actual number of axes for execute method.
  this->NumberOfAxes = num;

  // First set the axes to fill in all axes.
  this->vtkImageDyadicFilter::SetAxes(num, axes);
  
  // Copy the first four (non component) axes.
  count = 0;
  idx = 0;
  while (count < 4)
    {
    if (this->Axes[idx] != VTK_IMAGE_COMPONENT_AXIS)
      {
      newAxes[count] = this->Axes[idx];
      ++count;
      }
    ++idx;
    if (idx >= VTK_IMAGE_DIMENSIONS)
      {
      vtkErrorMacro(<< "SetAxes: Could not find axes");
      return;
      }
    }
  // Last axis is component
  newAxes[4] = VTK_IMAGE_COMPONENT_AXIS;

  this->vtkImageDyadicFilter::SetAxes(5, newAxes);
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a region that holds the image extent of this filters
// input, and changes the region to hold the image extent of this filters
// output.
void vtkImageNonMaximumSuppression::ComputeOutputImageInformation(
			  vtkImageRegion *inRegion1, vtkImageRegion *inRegion2,
			  vtkImageRegion *outRegion)
{
  int extent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int idx;

  inRegion1->GetImageExtent(VTK_IMAGE_DIMENSIONS, extent);
  // To avoid compiler warnings
  inRegion2 = inRegion2;
  if ( ! this->HandleBoundaries)
    {
    // shrink output image extent.
    for (idx = 0; idx < this->NumberOfAxes; ++idx)
      {
      extent[idx*2] += 1;
      extent[idx*2+1] -= 1;
      }
    }
  
  extent[8] = 0;
  extent[9] = 0;

  outRegion->SetImageExtent(VTK_IMAGE_DIMENSIONS, extent);
}


//----------------------------------------------------------------------------
// Description:
// This method computes the input extent necessary to generate the output.
void vtkImageNonMaximumSuppression::ComputeRequiredInputRegionExtent(
			vtkImageRegion *outRegion, 
			vtkImageRegion *inRegion1, vtkImageRegion *inRegion2)
{
  int extent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int *imageExtent;
  int idx;

  imageExtent = inRegion1->GetImageExtent();
  outRegion->GetExtent(VTK_IMAGE_DIMENSIONS, extent);
  extent[8] = 0;
  extent[9] = 0;
  
  // grow input image extent.
  for (idx = 0; idx < this->NumberOfAxes; ++idx)
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
  
  inRegion1->SetExtent(VTK_IMAGE_DIMENSIONS, extent);
  extent[9] = this->NumberOfAxes - 1;
  inRegion2->SetExtent(VTK_IMAGE_DIMENSIONS, extent);
}



//----------------------------------------------------------------------------
// Description:
// This method executes the filter for boundary pixels.
void vtkImageNonMaximumSuppression::Execute(vtkImageRegion *inRegion1, 
					    vtkImageRegion *inRegion2,
					    vtkImageRegion *outRegion)
{
  int idx;
  float d;
  int *imageExtent, *incs;
  int outIdxs[VTK_IMAGE_DIMENSIONS], *idxs;
  // For looping though output (and input) pixels.
  int min0, max0, min1, max1, min2, max2, min3, max3;
  int outIdx0, outIdx1, outIdx2, outIdx3;
  int outInc0, outInc1, outInc2, outInc3;
  float *outPtr0, *outPtr1, *outPtr2, *outPtr3;
  int in1Inc0, in1Inc1, in1Inc2, in1Inc3;
  float *in1Ptr0, *in1Ptr1, *in1Ptr2, *in1Ptr3;
  int in2Inc0, in2Inc1, in2Inc2, in2Inc3, in2Inc4;
  float *in2Ptr0, *in2Ptr1, *in2Ptr2, *in2Ptr3, *in2Ptr4;
  int neighborA, neighborB;

  // This filter expects that output and input are type float.
  if (outRegion->GetScalarType() != VTK_FLOAT ||
      inRegion1->GetScalarType() != VTK_FLOAT ||
      inRegion2->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: output ScalarType, "
                  << vtkImageScalarTypeNameMacro(outRegion->GetScalarType())
                  << ", must be float");
    return;
    }

  // Get information to march through data
  inRegion1->GetIncrements(in1Inc0, in1Inc1, in1Inc2, in1Inc3); 
  inRegion2->GetIncrements(in2Inc0, in2Inc1, in2Inc2, in2Inc3, in2Inc4); 
  outRegion->GetIncrements(outInc0, outInc1, outInc2, outInc3); 
  outRegion->GetExtent(min0, max0, min1, max1, min2, max2, min3, max3);
  
  // We want the input pixel to correspond to output
  in1Ptr3 = (float *)(inRegion1->GetScalarPointer(min0, min1, min2, min3));
  in2Ptr3 = (float *)(inRegion2->GetScalarPointer(min0, min1, min2, min3));
  outPtr3 = (float *)(outRegion->GetScalarPointer());
  
  // loop through pixels of output
  for (outIdx3 = min3; outIdx3 <= max3; ++outIdx3)
    {
    outIdxs[3] = outIdx3;
    outPtr2 = outPtr3;
    in1Ptr2 = in1Ptr3;
    in2Ptr2 = in2Ptr3;
    for (outIdx2 = min2; outIdx2 <= max2; ++outIdx2)
      {
      outIdxs[2] = outIdx2;
      outPtr1 = outPtr2;
      in1Ptr1 = in1Ptr2;
      in2Ptr1 = in2Ptr2;
      for (outIdx1 = min1; outIdx1 <= max1; ++outIdx1)
	{
	outIdxs[1] = outIdx1;
	outPtr0 = outPtr1;
	in1Ptr0 = in1Ptr1;
	in2Ptr0 = in2Ptr1;
	for (outIdx0 = min0; outIdx0 <= max0; ++outIdx0)
	  {
	  outIdxs[0] = outIdx0;
	  
	  // Use vector (in2) to determine which neighbors to use.
	  in2Ptr4 = in2Ptr0;
	  idxs = outIdxs;
	  incs = inRegion1->GetIncrements();
	  imageExtent = inRegion1->GetImageExtent();
	  neighborA = neighborB = 0;
	  for (idx = 0; idx < this->NumberOfAxes; ++idx)
	    {
	    // Normalize the vector.
	    d = *in2Ptr4 / *in1Ptr0;  
	    // Vector points positive along this axis?
	    // (can point along multiple axes)
	    if (d > 0.38268343)  // sin(22.5 degrees)
	      {
	      if (*idxs < imageExtent[1])  // max
		{
		neighborA += *incs;
		}
	      if (outIdx2 > *imageExtent)  // min
		{
		neighborB -= *incs;
		}
	      }
	    // Vector points negative along this axis?
	    else if (d < -0.38268343)
	      {
	      if (*idxs < imageExtent[1])  // max
		{
		neighborB += *incs;
		}
	      if (outIdx2 > *imageExtent)  //min
		{
		neighborA -= *incs;
		}
	      }
	    // Increment pointers
	    in2Ptr4 += in2Inc4;
	    ++idxs;
	    ++incs;
	    imageExtent += 2;
	    }
	  
	  // Set Output Magnitude
	  if (in1Ptr0[neighborA] > *in1Ptr0 || in1Ptr0[neighborB] > *in1Ptr0)
	    {
	    *outPtr0 = 0.0;
	    }
	  else
	    {
	    *outPtr0 = *in1Ptr0;
	    // also check for them being equal is neighbor with larger ptr
	    if ((neighborA > neighborB)&&(in1Ptr0[neighborA] == *in1Ptr0))
	      {
	      *outPtr0 = 0.0;
	      }
	    if ((neighborB > neighborA)&&(in1Ptr0[neighborB] == *in1Ptr0))
	      {
	      *outPtr0 = 0.0;
	      }
	    }
	  
	  outPtr0 += outInc0;
	  in1Ptr0 += in1Inc0;
	  in2Ptr0 += in2Inc0;
	  }
	outPtr1 += outInc1;
	in1Ptr1 += in1Inc1;
	in2Ptr1 += in2Inc1;
	}
      outPtr2 += outInc2;
      in1Ptr2 += in1Inc2;
      in2Ptr2 += in2Inc2;
      }
    outPtr3 += outInc3;
    in1Ptr3 += in1Inc3;
    in2Ptr3 += in2Inc3;
    }
}




