/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFlip.cxx
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
#include "vtkImageFlip.h"



//----------------------------------------------------------------------------
vtkImageFlip::vtkImageFlip()
{
  this->NumberOfExecutionAxes = 4;
  this->PreserveImageExtent = 1;
}

//----------------------------------------------------------------------------
void vtkImageFlip::SetFilteredAxes(int num, int *axes)
{
  this->vtkImageFilter::SetFilteredAxes(num, axes);
  this->NumberOfExecutionAxes = 4;
}
						 
//----------------------------------------------------------------------------
// Description:
// Image extent is modified by this filter.
void vtkImageFlip::ExecuteImageInformation()
{
  int idx, axis, extent[8], temp;

  if ( ! this->PreserveImageExtent)
    {
    this->Input->GetWholeExtent(extent);
    for (idx = 0; idx < this->NumberOfFilteredAxes; ++idx)
      {
      axis = this->FilteredAxes[idx];
      temp = extent[axis*2];
      extent[axis*2] = -extent[axis*2+1];
      extent[axis*2+1] = -temp;
      }
    this->Output->SetWholeExtent(extent);
    }
}

//----------------------------------------------------------------------------
// Description:
// What input should be requested.
void vtkImageFlip::ComputeRequiredInputUpdateExtent()
{
  int idx, axis, extent[8], temp, sum;
  int *wholeExtent;
  
  this->Output->GetUpdateExtent(extent);
  wholeExtent = this->Output->GetWholeExtent();
  for (idx = 0; idx < this->NumberOfFilteredAxes; ++idx)
    {
    axis = this->FilteredAxes[idx];
    if (this->PreserveImageExtent)
      {
      temp = extent[axis*2];
      sum = wholeExtent[axis*2] + wholeExtent[axis*2+1];
      extent[axis*2] = -extent[axis*2+1]+sum;
      extent[axis*2+1] = -temp+sum;
      }
    else
      {
      temp = extent[axis*2];
      extent[axis*2] = -extent[axis*2+1];
      extent[axis*2+1] = -temp;
      }
    }
  
}


//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
template <class IT, class OT>
static void vtkImageFlipExecute(vtkImageFlip *self,
			 vtkImageRegion *inRegion, IT *inPtr,
			 vtkImageRegion *outRegion, OT *outPtr){
  int min0, max0, min1, max1, min2, max2, min3, max3;
  int idx0, idx1, idx2, idx3;
  int inInc0, inInc1, inInc2, inInc3;
  int outInc0, outInc1, outInc2, outInc3;
  IT  *inPtr0, *inPtr1, *inPtr2, *inPtr3;
  OT  *outPtr0, *outPtr1, *outPtr2, *outPtr3;

  self = self;
  outPtr = outPtr;
  
  // Get information to march through data 
  inRegion->GetIncrements(inInc0, inInc1, inInc2, inInc3);
  outRegion->GetIncrements(outInc0, outInc1, outInc2, outInc3);
  outRegion->GetExtent(min0, max0, min1, max1, min2, max2, min3, max3);

  if (self->GetNumberOfFilteredAxes() > 0)
    {
    outPtr += (max0 - min0) * outInc0;
    outInc0 = -outInc0;
    }
  if (self->GetNumberOfFilteredAxes() > 1)
    {
    outPtr += (max1 - min1) * outInc1;
    outInc1 = -outInc1;
    }
  if (self->GetNumberOfFilteredAxes() > 2)
    {
    outPtr += (max2 - min2) * outInc2;
    outInc2 = -outInc2;
    }
  if (self->GetNumberOfFilteredAxes() > 3)
    {
    outPtr += (max3 - min3) * outInc3;
    outInc3 = -outInc3;
    }
  
  // Loop through ouput pixels
  inPtr3 = inPtr;
  outPtr3 = outPtr;
  for (idx3 = min3; idx3 <= max3; ++idx3)
    {
    outPtr2 = outPtr3;
    inPtr2 = inPtr3;
    for (idx2 = min2; idx2 <= max2; ++idx2)
      {
      outPtr1 = outPtr0;
      inPtr1 = inPtr0;
      for (idx1 = min1; idx1 <= max1; ++idx1)
	{
	outPtr0 = outPtr1;
	inPtr0 = inPtr1;
	for (idx0 = min0; idx0 <= max0; ++idx0)
	  {
	  *outPtr0 = (OT)(*inPtr0);
	  outPtr0 += outInc0;
	  inPtr0  += inInc0;
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
template <class T>
static void vtkImageFlipExecute(vtkImageFlip *self,
			 vtkImageRegion *inRegion, T *inPtr,
			 vtkImageRegion *outRegion)
{
  void *outPtr = outRegion->GetScalarPointer();
  switch (outRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageFlipExecute(self, inRegion, (T *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageFlipExecute(self, inRegion, (T *)(inPtr), 
			  outRegion, (int *)(outPtr)); 
      break;
    case VTK_SHORT:
      vtkImageFlipExecute(self, inRegion, (T *)(inPtr), 
			  outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageFlipExecute(self, inRegion, (T *)(inPtr), 
			  outRegion, (unsigned short *)(outPtr)); 
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageFlipExecute(self, inRegion, (T *)(inPtr), 
			  outRegion, (unsigned char *)(outPtr)); 
      break;
    default:
      vtkGenericWarningMacro("Execute: Unknown output ScalarType");
      return;
    }
}

//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageFlip::Execute(vtkImageRegion *inRegion, 
				vtkImageRegion *outRegion) {
  void *inPtr = inRegion->GetScalarPointer();
  
  vtkDebugMacro(<< "Execute: inRegion = " << inRegion 
		<< ", outRegion = " << outRegion);

  switch (inRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageFlipExecute(this, inRegion, (float *)(inPtr), outRegion);
      break;
    case VTK_INT:
      vtkImageFlipExecute(this, inRegion, (int *)(inPtr), outRegion);
      break;
    case VTK_SHORT:
      vtkImageFlipExecute(this, inRegion, (short *)(inPtr), outRegion);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageFlipExecute(this, inRegion, (unsigned short *)(inPtr), 
			  outRegion);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageFlipExecute(this, inRegion, (unsigned char *)(inPtr), outRegion);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}
















