/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePermute.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Abdalmajeid M. Alyassin who developed this class.

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
#include "vtkImagePermute.h"

//----------------------------------------------------------------------------
// Description:
// Constructor sets default values
vtkImagePermute::vtkImagePermute()
{
  this->SetExecutionAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS,
			 VTK_IMAGE_Z_AXIS, VTK_IMAGE_COMPONENT_AXIS);
}


//----------------------------------------------------------------------------
void vtkImagePermute::SetFilteredAxes(int num, int *axes)
{
  this->vtkImageFilter::SetFilteredAxes(num, axes);
  this->NumberOfExecutionAxes = 4;
}

//----------------------------------------------------------------------------
void vtkImagePermute::ExecuteImageInformation() 
{
  int min, max;
  float spacing;
  float origin;
  int idx, axis;
  
  for (idx = 0; idx < this->NumberOfFilteredAxes; ++idx)
    {
    axis = this->FilteredAxes[idx];
    this->Input->GetAxisWholeExtent(axis, min, max);
    this->Output->SetAxisWholeExtent(idx, min, max);
    this->Input->GetAxisSpacing(axis, spacing);
    this->Output->SetAxisSpacing(idx, spacing);
    this->Input->GetAxisOrigin(axis, origin);
    this->Output->SetAxisOrigin(idx, origin);
    }
}


//----------------------------------------------------------------------------
void vtkImagePermute::ComputeRequiredInputUpdateExtent()
{
  int min, max;
  int idx, axis;
  
  for (idx = 0; idx < this->NumberOfFilteredAxes; ++idx)
    {
    axis = this->FilteredAxes[idx];
    this->Output->GetAxisUpdateExtent(idx, min, max);
    this->Input->SetAxisUpdateExtent(axis, min, max);
    }
}


//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImagePermuteExecute(vtkImagePermute *self,
				   vtkImageRegion *inRegion, T *inPtr,
				   vtkImageRegion *outRegion, T *outPtr)
{
  int min0, max0, min1, max1, min2, max2, min3, max3;
  int idx0, idx1, idx2, idx3;
  int inInc0, inInc1, inInc2, inInc3;
  int outInc0, outInc1, outInc2, outInc3;
  T  *inPtr0, *inPtr1, *inPtr2, *inPtr3;
  T  *outPtr0, *outPtr1, *outPtr2, *outPtr3;

  self = self;
  outRegion->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS, VTK_IMAGE_Z_AXIS,
		     VTK_IMAGE_TIME_AXIS);
    
  // Get information to march through data 
  inRegion->GetIncrements(inInc0, inInc1, inInc2, inInc3);
  outRegion->GetIncrements(outInc0, outInc1, outInc2, outInc3);
  outRegion->GetExtent(min0, max0, min1, max1, min2, max2, min3, max3);

  // Loop through ouput pixels
  inPtr3 = inPtr;
  outPtr3 = outPtr;
  for (idx3 = min3; idx3 <= max3; ++idx3)
    {
    outPtr2 = outPtr3;
    inPtr2 = inPtr3;
    for (idx2 = min2; idx2 <= max2; ++idx2)
      {
      inPtr1 = inPtr2;
      outPtr1 = outPtr2;
      for (idx1 = min1; idx1 <= max1; ++idx1)
	{
	outPtr0 = outPtr1;
	inPtr0 = inPtr1;
	for (idx0 = min0; idx0 <= max0; ++idx0)
	  {
	  *outPtr0 = *inPtr0;
	  outPtr0 += outInc0;
	  inPtr0  += inInc0;
	  }
	outPtr1 += outInc1;
	inPtr1 += inInc1;
	}
      outPtr2 += outInc2;
      inPtr2  += inInc2;
      }
    outPtr3 += outInc3;
    inPtr3 += inInc3;
    }
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImagePermute::Execute(vtkImageRegion *inRegion, 
			      vtkImageRegion *outRegion)
{
  void *inPtr = inRegion->GetScalarPointer();
  void *outPtr = outRegion->GetScalarPointer();
  
  if (inRegion->GetScalarType() != inRegion->GetScalarType())
    {
    vtkErrorMacro("Input (" 
	  << vtkImageScalarTypeNameMacro(inRegion->GetScalarType()) 
	  << ") has to be the same data type as output"
	  << vtkImageScalarTypeNameMacro(outRegion->GetScalarType()) << ")");
    return;
    }
  
  switch (inRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImagePermuteExecute(this, inRegion, (float *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImagePermuteExecute(this, inRegion, (int *)(inPtr), 
			     outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImagePermuteExecute(this, inRegion, (short *)(inPtr), 
			  outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImagePermuteExecute(this, inRegion, (unsigned short *)(inPtr), 
			  outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImagePermuteExecute(this, inRegion, (unsigned char *)(inPtr), 
			  outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}
















