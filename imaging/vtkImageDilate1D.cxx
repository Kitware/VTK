/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDilate1D.cxx
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
#include "vtkImageDilate1D.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageDilate1D fitler.
// By default zero values are dilated.
vtkImageDilate1D::vtkImageDilate1D()
{
  int idx;
  
  this->SetAxes(VTK_IMAGE_X_AXIS);
  this->UseExecuteCenterOff();
  this->HandleBoundariesOn();

  // Poor performance, but simple implementation.
  this->ExecuteDimensionality = 1;
  this->Dimensionality = 1;

  this->SetStride(1);
  this->SetKernelSize(1);
}


//----------------------------------------------------------------------------
// Description:
// This method sets the size of the neighborhood.
void vtkImageDilate1D::SetKernelSize(int size)
{
  this->KernelSize[0] = size;
  this->KernelMiddle[0] = size / 2;
}
			     
			     
//----------------------------------------------------------------------------
// Description:
// This method Does nothing for now.
void vtkImageDilate1D::SetStride(int stride)
{
  this->Strides[0] = stride;
}
			     
			     
//----------------------------------------------------------------------------
// Description:
// This templated function is passed a input and output region, 
// and executes the dilate algorithm to fill the output from the input.
// Note that input pixel is offset from output pixel.
// It also handles ImageExtent by truncating the kernel.  
template <class T>
void vtkImageDilate1DExecute(vtkImageDilate1D *self,
			     vtkImageRegion *inRegion, T *inPtr,
			     vtkImageRegion *outRegion, T *outPtr)
{
  int outIdx, kernelIdx;
  int outMin, outMax;
  int inInc, outInc;
  T *tmpPtr;
  int cut;
  int outImageExtentMin, outImageExtentMax;
  
  // Get information to march through data 
  inRegion->GetIncrements(inInc);
  outRegion->GetIncrements(outInc);  
  outRegion->GetExtent(outMin, outMax);  

  // Determine the middle portion of the region 
  // that does not need ImageExtent handling.
  outRegion->GetImageExtent(outImageExtentMin, outImageExtentMax);
  if (self->HandleBoundaries)
    {
    outImageExtentMin += self->KernelMiddle[0];
    outImageExtentMax -= (self->KernelSize[0] - 1) - self->KernelMiddle[0];
    }
  else
    {
    // just some error checking
    if (outMin < outImageExtentMin || outMax > outImageExtentMax)
      {
      cerr << "vtkImageDilate1DExecute: Boundaries not handled.";
      return;
      }
    }
  // Shrink ImageExtent if generated region is smaller
  outImageExtentMin = outImageExtentMin > outMin ? outImageExtentMin : outMin;
  outImageExtentMax = outImageExtentMax < outMax ? outImageExtentMax : outMax;

  
  // loop divided into three pieces, so initialize here.
  outIdx = outMin;

  // loop through the ImageExtent pixels on the left.
  for ( ; outIdx < outImageExtentMin; ++outIdx)
    {
    *outPtr = *inPtr;
    // The number of pixels cut from the kernel
    cut = (outImageExtentMin - outIdx);
    // loop over neighborhood pixels
    tmpPtr = inPtr;
    for (kernelIdx = cut; kernelIdx < self->KernelSize[0]; ++kernelIdx)
      {
      if (*tmpPtr > *outPtr)
	{
	*outPtr = *tmpPtr;
	}
      tmpPtr += inInc;
      }
    // increment to next pixel.
    outPtr += outInc;
    // the input pixel is not being incremented because of ImageExtent.
    }
  
  // loop through non ImageExtent pixels
  for ( ; outIdx <= outImageExtentMax; ++outIdx)
    {
    *outPtr = *inPtr;
    // loop for neighborhood
    tmpPtr = inPtr;
    for (kernelIdx = 0; kernelIdx < self->KernelSize[0]; ++kernelIdx)
      {
      if (*tmpPtr > *outPtr)
	{
	*outPtr = *tmpPtr;
	}
      tmpPtr += inInc;
      }
    // Increment to nect pixel
    outPtr += outInc;
    inPtr += inInc;
    }
  
  
  // loop through the ImageExtent pixels on the right.
  for ( ; outIdx <= outMax; ++outIdx)
    {
    *outPtr = *inPtr;
    // The number of pixels cut from the Dilate.
    cut = (outIdx - outImageExtentMax);
    // loop for Dilate (sum)
    tmpPtr = inPtr;
    for (kernelIdx = cut; kernelIdx < self->KernelSize[0]; ++kernelIdx)
      {
      if (*tmpPtr > *outPtr)
	{
	*outPtr = *tmpPtr;
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
void vtkImageDilate1D::Execute(vtkImageRegion *inRegion, 
			       vtkImageRegion *outRegion)
{
  void *inPtr, *outPtr;

  if (this->Strides[0] != 1)
    {
    vtkErrorMacro("Strides not implemented yet.");
    return;
    }
  
  // perform Dilate for each pixel of output.
  // Note that input pixel is offset from output pixel.
  inPtr = inRegion->GetScalarPointer();
  outPtr = outRegion->GetScalarPointer();

  vtkDebugMacro(<< "Execute: inRegion = " << inRegion 
		<< ", outRegion = " << outRegion);
  
  // this filter expects that input is the same type as output.
  if (inRegion->GetScalarType() != outRegion->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inRegion->GetScalarType()
                  << ", must match out ScalarType " << outRegion->GetScalarType());
    return;
    }

  // choose which templated function to call.
  switch (inRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageDilate1DExecute(this, inRegion, (float *)(inPtr), 
				 outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageDilate1DExecute(this, inRegion, (int *)(inPtr),
				 outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageDilate1DExecute(this, inRegion, (short *)(inPtr),
				 outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageDilate1DExecute(this,
				 inRegion, (unsigned short *)(inPtr), 
				 outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageDilate1DExecute(this,
				 inRegion, (unsigned char *)(inPtr),
				 outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}



