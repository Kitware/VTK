/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageContinuousDilate1D.cxx
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
#include "vtkImageCache.h"
#include "vtkImageContinuousDilate1D.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageContinuousDilate1D fitler.
// By default zero values are dilated.
vtkImageContinuousDilate1D::vtkImageContinuousDilate1D()
{
  this->Stride = 1;
  this->KernelSize = 1;
  // Execute method hanldes 1 Axis. Poor performance, but simple implementation
  this->SetFilteredAxis(VTK_IMAGE_X_AXIS);
}


//----------------------------------------------------------------------------
// Description:
// Specify which axis to operate on.
void vtkImageContinuousDilate1D::SetFilteredAxis(int axis)
{
  if (axis < 0 || axis > 3)
    {
    vtkErrorMacro("SetFilteredAxis: Bad axis " << axis);
    return;
    }
  
  this->vtkImageFilter::SetFilteredAxes(1, &axis);
}



//----------------------------------------------------------------------------
void vtkImageContinuousDilate1D::ExecuteImageInformation(vtkImageCache *in, 
							 vtkImageCache *out)
{
  int min, max;
  float spacing;
  
  in->GetAxisWholeExtent(this->FilteredAxes[0], min, max);
  min = (int)(ceil(((float)min) /((float)this->Stride)));
  max = (int)(floor((((float)max+1.0) / ((float)this->Stride))-1.0));
  
  in->GetAxisSpacing(this->FilteredAxes[0], spacing);
  out->SetAxisSpacing(this->FilteredAxes[0], spacing * (float)(this->Stride));
}

  



//----------------------------------------------------------------------------
void vtkImageContinuousDilate1D::ComputeRequiredInputUpdateExtent(
					 vtkImageCache *out, vtkImageCache *in)
{
  int min, max, mid, wholeMin, wholeMax;

  out->GetAxisUpdateExtent(this->FilteredAxes[0], min, max);
  out->GetAxisWholeExtent(this->FilteredAxes[0], wholeMin, wholeMax);

  // Magnify by strides
  min *= this->Stride;
  max = (max+1)*this->Stride - 1;
  // Expand to get inRegion Extent
  mid = (this->KernelSize - 1) / 2;
  min -= mid;
  max += (this->KernelSize - 1) - mid;
  // Clip
  if (min < wholeMin)
    {
    min = wholeMin;
    }
  if (max > wholeMax)
    {
    max = wholeMax;
    }
  
  in->SetAxisUpdateExtent(this->FilteredAxes[0], min, max);
}

			     
			     
//----------------------------------------------------------------------------
// Description:
// This templated function is passed a input and output region, 
// and executes the dilate algorithm to fill the output from the input.
// Note that input pixel is offset from output pixel.
// It also handles WholeExtent by truncating the kernel.  
template <class T>
static void vtkImageContinuousDilate1DExecute(vtkImageContinuousDilate1D *self,
			     vtkImageRegion *inRegion, T *inPtr,
			     vtkImageRegion *outRegion, T *outPtr)
{
  int outIdx, kernelIdx;
  int outMin, outMax;
  int inInc, outInc;
  T *tmpPtr;
  int cut, mid, size;
  int outWholeExtentMin, outWholeExtentMax;
  
  // Get information to march through data 
  inRegion->GetIncrements(inInc);
  outRegion->GetIncrements(outInc);  
  outRegion->GetExtent(outMin, outMax);  
  
  // I do not like this method of boundary cheching !!!!
  size = self->GetKernelSize();
  mid = (size - 1) / 2;

  // Determine the middle portion of the region 
  // that does not need WholeExtent handling.
  outRegion->GetWholeExtent(outWholeExtentMin, outWholeExtentMax);
  outWholeExtentMin += mid;
  outWholeExtentMax -= (size - 1) - mid;

  // Shrink WholeExtent if generated region is smaller
  outWholeExtentMin = outWholeExtentMin > outMin ? outWholeExtentMin : outMin;
  outWholeExtentMax = outWholeExtentMax < outMax ? outWholeExtentMax : outMax;

  
  // loop divided into three pieces, so initialize here.
  outIdx = outMin;

  // loop through the WholeExtent pixels on the left.
  for ( ; outIdx < outWholeExtentMin; ++outIdx)
    {
    *outPtr = *inPtr;
    // The number of pixels cut from the kernel
    cut = (outWholeExtentMin - outIdx);
    // loop over neighborhood pixels
    tmpPtr = inPtr;
    for (kernelIdx = cut; kernelIdx < size; ++kernelIdx)
      {
      if (*tmpPtr > *outPtr)
	{
	*outPtr = *tmpPtr;
	}
      tmpPtr += inInc;
      }
    // increment to next pixel.
    outPtr += outInc;
    // the input pixel is not being incremented because of WholeExtent.
    }
  
  // loop through non WholeExtent pixels
  for ( ; outIdx <= outWholeExtentMax; ++outIdx)
    {
    *outPtr = *inPtr;
    // loop for neighborhood
    tmpPtr = inPtr;
    for (kernelIdx = 0; kernelIdx < size; ++kernelIdx)
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
  
  
  // loop through the WholeExtent pixels on the right.
  for ( ; outIdx <= outMax; ++outIdx)
    {
    *outPtr = *inPtr;
    // The number of pixels cut from the Dilate.
    cut = (outIdx - outWholeExtentMax);
    // loop for Dilate (sum)
    tmpPtr = inPtr;
    for (kernelIdx = cut; kernelIdx < size; ++kernelIdx)
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
void vtkImageContinuousDilate1D::Execute(vtkImageRegion *inRegion, 
					 vtkImageRegion *outRegion)
{
  void *inPtr, *outPtr;

  if (this->Stride != 1)
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
      vtkImageContinuousDilate1DExecute(this, inRegion, (float *)(inPtr), 
				 outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageContinuousDilate1DExecute(this, inRegion, (int *)(inPtr),
				 outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageContinuousDilate1DExecute(this, inRegion, (short *)(inPtr),
				 outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageContinuousDilate1DExecute(this,
				 inRegion, (unsigned short *)(inPtr), 
				 outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageContinuousDilate1DExecute(this,
				 inRegion, (unsigned char *)(inPtr),
				 outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}






