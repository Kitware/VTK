/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImage1dDilateValueFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
#include "vtkImage1dDilateValueFilter.hh"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImage1dDilateValueFilter fitler.
// By default zero values are dilated.
vtkImage1dDilateValueFilter::vtkImage1dDilateValueFilter()
{
  this->Value = 0.0;
  this->HandleBoundariesOn();
}


//----------------------------------------------------------------------------
// Description:
// This templated function is passed a input and output region, 
// and executes the dilate algorithm to fill the output from the input.
// Note that input pixel is offset from output pixel.
// It also handles ImageBounds by truncating the kernel.  
template <class T>
void vtkImage1dDilateValueFilterExecute1d(vtkImage1dDilateValueFilter *self,
					  vtkImageRegion *inRegion, T *inPtr,
					  vtkImageRegion *outRegion, T *outPtr)
{
  int outIdx, kernelIdx;
  int outMin, outMax;
  int inInc, outInc;
  T *tmpPtr;
  T value = (T)(self->Value);
  int cut;
  int outImageBoundsMin, outImageBoundsMax;
  
  // Get information to march through data 
  inRegion->GetIncrements1d(inInc);
  outRegion->GetIncrements1d(outInc);  
  outRegion->GetBounds1d(outMin, outMax);  

  // Determine the middle portion of the region 
  // that does not need ImageBounds handling.
  outRegion->GetImageBounds1d(outImageBoundsMin, outImageBoundsMax);
  if (self->HandleBoundaries)
    {
    outImageBoundsMin += self->KernelMiddle;
    outImageBoundsMax -= (self->KernelSize - 1) - self->KernelMiddle;
    }
  else
    {
    // just some error checking
    if (outMin < outImageBoundsMin || outMax > outImageBoundsMax)
      {
      cerr << "vtkImage1dDilateValueFilterExecute1d: Boundaries not handled.";
      return;
      }
    }
  // Shrink ImageBounds if generated region is smaller
  outImageBoundsMin = outImageBoundsMin > outMin ? outImageBoundsMin : outMin;
  outImageBoundsMax = outImageBoundsMax < outMax ? outImageBoundsMax : outMax;

  
  // loop divided into three pieces, so initialize here.
  outIdx = outMin;

  // loop through the ImageBounds pixels on the left.
  for ( ; outIdx < outImageBoundsMin; ++outIdx)
    {
    // The number of pixels cut from the kernel
    cut = (outImageBoundsMin - outIdx);
    // First do identity (complex indexing saves time?)
    *outPtr = inPtr[self->KernelMiddle - cut];
    // loop over neighborhood pixels
    tmpPtr = inPtr;
    for (kernelIdx = cut; kernelIdx < self->KernelSize; ++kernelIdx)
      {
      if (*tmpPtr == value)
	{
	*outPtr = value;
	}
      tmpPtr += inInc;
      }
    // increment to next pixel.
    outPtr += outInc;
    // the input pixel is not being incremented because of ImageBounds.
    }
  
  // loop through non ImageBounds pixels
  for ( ; outIdx <= outImageBoundsMax; ++outIdx)
    {
    // First do identity (complex indexing saves time?)
    *outPtr = inPtr[self->KernelMiddle];
    // loop for neighborhood
    tmpPtr = inPtr;
    for (kernelIdx = 0; kernelIdx < self->KernelSize; ++kernelIdx)
      {
      if (*tmpPtr == value)
	{
	*outPtr = value;
	}
      tmpPtr += inInc;
      }
    // Increment to nect pixel
    outPtr += outInc;
    inPtr += inInc;
    }
  
  
  // loop through the ImageBounds pixels on the right.
  for ( ; outIdx <= outMax; ++outIdx)
    {
    // The number of pixels cut from the DilateValue.
    cut = (outIdx - outImageBoundsMax);
    // First do identity (complex indexing saves time?)
    *outPtr = inPtr[self->KernelMiddle];
    // loop for DilateValue (sum)
    tmpPtr = inPtr;
    for (kernelIdx = cut; kernelIdx < self->KernelSize; ++kernelIdx)
      {
      if (*tmpPtr == value)
	{
	*outPtr = value;
	}
      tmpPtr += inInc;
      }
    // increment to next pixel.
    outPtr += outInc;
    inPtr += inInc;
    }
}




//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output region, and executes the Conv1d
// algorithm to fill the output from the input.
void vtkImage1dDilateValueFilter::Execute1d(vtkImageRegion *inRegion, 
					    vtkImageRegion *outRegion)
{
  void *inPtr, *outPtr;

  // perform DilateValue for each pixel of output.
  // Note that input pixel is offset from output pixel.
  inPtr = inRegion->GetVoidPointer1d();
  outPtr = outRegion->GetVoidPointer1d();

  vtkDebugMacro(<< "Execute: inRegion = " << inRegion 
		<< ", outRegion = " << outRegion);
  
  // this filter expects that input is the same type as output.
  if (inRegion->GetDataType() != outRegion->GetDataType())
    {
    vtkErrorMacro(<< "Execute: input DataType, " << inRegion->GetDataType()
                  << ", must match out DataType " << outRegion->GetDataType());
    return;
    }

  // choose which templated function to call.
  switch (inRegion->GetDataType())
    {
    case VTK_IMAGE_FLOAT:
      vtkImage1dDilateValueFilterExecute1d(this, inRegion, (float *)(inPtr), 
				 outRegion, (float *)(outPtr));
      break;
    case VTK_IMAGE_INT:
      vtkImage1dDilateValueFilterExecute1d(this, inRegion, (int *)(inPtr),
				 outRegion, (int *)(outPtr));
      break;
    case VTK_IMAGE_SHORT:
      vtkImage1dDilateValueFilterExecute1d(this, inRegion, (short *)(inPtr),
				 outRegion, (short *)(outPtr));
      break;
    case VTK_IMAGE_UNSIGNED_SHORT:
      vtkImage1dDilateValueFilterExecute1d(this,
				 inRegion, (unsigned short *)(inPtr), 
				 outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_IMAGE_UNSIGNED_CHAR:
      vtkImage1dDilateValueFilterExecute1d(this,
				 inRegion, (unsigned char *)(inPtr),
				 outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown DataType");
      return;
    }
}



