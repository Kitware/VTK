/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCorrelation.cxx
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
#include "vtkImageRegion.h"
#include "vtkImageCache.h"
#include "vtkImageCorrelation.h"



//----------------------------------------------------------------------------
vtkImageCorrelation::vtkImageCorrelation()
{
  this->SetFilteredAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS);
  this->SetOutputScalarType(VTK_FLOAT);
}


//----------------------------------------------------------------------------
void vtkImageCorrelation::SetFilteredAxes(int num, int *axes)
{
  if (num > 2)
    {
    vtkErrorMacro("SetFilteredAxes: Too many axes");
    return;
    }
  
  this->vtkImageTwoInputFilter::SetFilteredAxes(num, axes);
  this->SetExecutionAxes(num, axes);
}


//----------------------------------------------------------------------------
// Description:
// Grow the output image 
void vtkImageCorrelation::ExecuteImageInformation()
{
  int *extent1, extent2[8], idx, axis;
  
  extent1 = this->Inputs[0]->GetWholeExtent();
  this->Inputs[1]->GetWholeExtent(extent2);
  for (idx = 0; idx < this->NumberOfFilteredAxes; ++idx)
    {
    axis = this->FilteredAxes[idx];
    extent2[axis*2] += extent1[axis*2];
    extent2[axis*2+1] += extent1[axis*2+1];
    }
  
  this->Output->SetWholeExtent(extent2);
}

//----------------------------------------------------------------------------
// Description:
// Grow
void vtkImageCorrelation::ComputeRequiredInputUpdateExtent(int whichInput)
{
  int *wholeExtent, updateExtent[8];
  int idx, axis;
  
  // cheat and get the whole image.
  wholeExtent = this->Inputs[whichInput]->GetWholeExtent();
  this->Output->GetUpdateExtent(updateExtent);
  for (idx = 0; idx < this->NumberOfFilteredAxes; ++idx)
    {
    axis = this->FilteredAxes[idx];
    updateExtent[axis*2] = wholeExtent[axis*2];
    updateExtent[axis*2+1] = wholeExtent[axis*2+1];
    }
  
  this->Inputs[whichInput]->SetUpdateExtent(updateExtent);
}


//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output regions, and executes the filter
// It only deals with floats for now
void vtkImageCorrelation::Execute(vtkImageRegion *in1Region, 
				  vtkImageRegion *in2Region, 
				  vtkImageRegion *outRegion)
{
  int temp;
  int outIdx0, outIdx1, in1Idx0, in1Idx1;
  int outMin0, outMax0, outMin1, outMax1;
  int in1Min0, in1Max0, in1Min1, in1Max1;
  int in2Min0, in2Max0, in2Min1, in2Max1;
  int in1Inc0, in1Inc1;
  int in2Inc0, in2Inc1;
  int outInc0, outInc1;
  float *in1Ptr0, *in1Ptr1;
  float *in2Ptr0, *in2Ptr1;
  float *outPtr0, *outPtr1;

  // this filter expects that inputs are the same type as output.
  if (in1Region->GetScalarType() != outRegion->GetScalarType() ||
      in2Region->GetScalarType() != outRegion->GetScalarType() ||
      outRegion->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: input ScalarTypes, " 
        << in1Region->GetScalarType() << " and " << in2Region->GetScalarType()
        << ", and out ScalarType " << outRegion->GetScalarType() 
        << "must be float");
    return;
    }
  
  // Get information to march through data 
  in1Region->GetIncrements(in1Inc0, in1Inc1);
  in2Region->GetIncrements(in2Inc0, in2Inc1);
  in2Region->GetExtent(in2Min0, in2Max0, in2Min1, in2Max1);
  outRegion->GetIncrements(outInc0, outInc1);
  outRegion->GetExtent(outMin0, outMax0, outMin1, outMax1);
  
  // Loop through ouput pixels
  outPtr1 = (float *)(outRegion->GetScalarPointer());
  for (outIdx1 = outMin1; outIdx1 <= outMax1; ++outIdx1)
    {
    outPtr0 = outPtr1;
    for (outIdx0 = outMin0; outIdx0 <= outMax0; ++outIdx0)
      {
      *outPtr0 = 0.0;
      // determine the extent of input 1 that contributes to this pixel
      in1Region->GetExtent(in1Min0, in1Max0, in1Min1, in1Max1);
      temp = outIdx0 - in2Max0;
      if (temp > in1Min0) in1Min0 = temp;
      temp = outIdx0 - in2Min0;
      if (temp < in1Max0) in1Max0 = temp;
      temp = outIdx1 - in2Max1;
      if (temp > in1Min1) in1Min1 = temp;
      temp = outIdx1 - in2Min1;
      if (temp < in1Max1) in1Max1 = temp;
      // Get the pointers
      in1Ptr1 = (float *)(in1Region->GetScalarPointer(in1Min0, in1Min1));
      in2Ptr1 = (float *)(in2Region->GetScalarPointer(outIdx0 - in1Min0, 
						      outIdx1 - in1Min1));
      // sumation
      for (in1Idx1 = in1Min1; in1Idx1 <= in1Max1; ++in1Idx1)
	{
	in1Ptr0 = in1Ptr1;
	in2Ptr0 = in2Ptr1;
	for (in1Idx0 = in1Min0; in1Idx0 <= in1Max0; ++in1Idx0)
	  {
	  *outPtr0 += *in1Ptr0 * *in2Ptr0;
	  in1Ptr0 += in1Inc0;
	  in2Ptr0 -= in2Inc0;
	  }
	in1Ptr1 += in1Inc1;
	in2Ptr1 -= in2Inc1;
	}
      outPtr0 += outInc0;
      }
    outPtr1 += outInc1;
    }
}















