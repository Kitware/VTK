/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageNonMaximumSuppression.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkImageRegion.h"
#include "vtkImageCache.h"
#include "vtkImageNonMaximumSuppression.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageNonMaximumSuppression fitler.
vtkImageNonMaximumSuppression::vtkImageNonMaximumSuppression()
{
  this->NumberOfFilteredAxes = 3;
  this->NumberOfExecutionAxes = 3;
  
  this->SetOutputScalarType(VTK_FLOAT);
}

//----------------------------------------------------------------------------
// Options do not include YZ ...
void vtkImageNonMaximumSuppression::SetNumberOfFilteredAxes(int num)
{
  if (this->NumberOfFilteredAxes != num)
    {
    this->NumberOfFilteredAxes = num;
    this->Modified();
    }
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a region that holds the image extent of this filters
// input, and changes the region to hold the image extent of this filters
// output.
void vtkImageNonMaximumSuppression::ExecuteImageInformation()
{
  int extent[8];
  int idx, axis;

  this->Inputs[0]->GetWholeExtent(extent);
  if ( ! this->HandleBoundaries)
    {
    // shrink output image extent.
    for (idx = 0; idx < this->NumberOfFilteredAxes; ++idx)
      {
      axis = this->FilteredAxes[idx];
      extent[axis*2] += 1;
      extent[axis*2+1] -= 1;
      }
    }
  
  this->Output->SetNumberOfScalarComponents(1);
  this->Output->SetWholeExtent(extent);
}


//----------------------------------------------------------------------------
// Description:
// This method computes the input extent necessary to generate the output.
void vtkImageNonMaximumSuppression::ComputeRequiredInputUpdateExtent(
						     int whichInput)
{
  int extent[8];
  int *wholeExtent;
  int idx, axis;

  wholeExtent = this->Inputs[0]->GetWholeExtent();
  this->Output->GetUpdateExtent(extent);
  
  // grow input image extent.
  for (idx = 0; idx < this->NumberOfFilteredAxes; ++idx)
    {
    axis = this->FilteredAxes[idx];
    extent[axis*2] -= 1;
    extent[axis*2+1] += 1;
    if (this->HandleBoundaries)
      {
      // we must clip extent with whole extent if we hanlde boundaries.
      if (extent[axis*2] < wholeExtent[axis*2])
	{
	extent[axis*2] = wholeExtent[axis*2];
	}
      if (extent[idx*2 + 1] > wholeExtent[idx*2 + 1])
	{
	extent[idx*2 + 1] = wholeExtent[idx*2 + 1];
	}
      }
    }
  
  this->Inputs[whichInput]->SetUpdateExtent(extent);
  // different components
  if (whichInput == 0)
    {
    this->Inputs[0]->SetNumberOfScalarComponents(1);
    }
  else
    {
    this->Inputs[1]->SetNumberOfScalarComponents(this->NumberOfFilteredAxes);
    }
}



//----------------------------------------------------------------------------
// Description:
// This method executes the filter for boundary pixels.
void vtkImageNonMaximumSuppression::Execute(vtkImageRegion *inRegion1, 
					    vtkImageRegion *inRegion2,
					    vtkImageRegion *outRegion)
{
  int idx;
  int *wholeExtent, *incs;
  int outIdxs[VTK_IMAGE_DIMENSIONS], *idxs;
  // For looping though output (and input) pixels.
  int min0, max0, min1, max1, min2, max2, min3, max3;
  int outIdx0, outIdx1, outIdx2, outIdx3;
  int outInc0, outInc1, outInc2, outInc3;
  float *outPtr0, *outPtr1, *outPtr2, *outPtr3;
  int in1Inc0, in1Inc1, in1Inc2, in1Inc3;
  float *in1Ptr0, *in1Ptr1, *in1Ptr2, *in1Ptr3;
  int in2Inc0, in2Inc1, in2Inc2, in2Inc3, in2IncV;
  float *in2Ptr0, *in2Ptr1, *in2Ptr2, *in2Ptr3, *in2PtrV;
  int neighborA, neighborB;
  float d, normalizeFactor, vector[VTK_IMAGE_DIMENSIONS], *ratio;


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

  // Gradient is computed with data spacing (world coordinates)
  ratio = inRegion2->GetSpacing();
  
  // Get information to march through data
  inRegion1->GetIncrements(in1Inc0, in1Inc1, in1Inc2, in1Inc3); 
  inRegion2->GetIncrements(in2Inc0, in2Inc1, in2Inc2, in2Inc3, in2IncV); 
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
	  in2PtrV = in2Ptr0;
	  idxs = outIdxs;
	  incs = inRegion1->GetIncrements();
	  wholeExtent = inRegion1->GetWholeExtent();
	  neighborA = neighborB = 0;
	  // Convert vector to pixel units and normalize.
	  normalizeFactor = 0.0;
	  for (idx = 0; idx < this->NumberOfFilteredAxes; ++idx)
	    {
	    // d units dI/world  -> dI
	    d = vector[idx] = *in2PtrV * ratio[idx];
	    normalizeFactor += d * d;
	    in2PtrV += in2IncV;
	    }
	  normalizeFactor = 1.0 / sqrt(normalizeFactor);
	  for (idx = 0; idx < this->NumberOfFilteredAxes; ++idx)
	    {
	    d = vector[idx] * normalizeFactor;  
	    // Vector points positive along this axis?
	    // (can point along multiple axes)
	    if (d > 0.5)  
	      {
	      if (*idxs < wholeExtent[1])  // max
		{
		neighborA += *incs;
		}
	      if (*idxs > *wholeExtent)  // min
		{
		neighborB -= *incs;
		}
	      }
	    // Vector points negative along this axis?
	    else if (d < -0.5)
	      {
	      if (*idxs < wholeExtent[1])  // max
		{
		neighborB += *incs;
		}
	      if (*idxs > *wholeExtent)  //min
		{
		neighborA -= *incs;
		}
	      }
	    // Increment pointers
	    ++idxs;
	    ++incs;
	    wholeExtent += 2;
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




