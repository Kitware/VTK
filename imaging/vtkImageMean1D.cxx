/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMean1D.cxx
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
#include "vtkImageRegion.h"
#include "vtkImageMean1D.h"





//----------------------------------------------------------------------------
// Description:
// Constructor: Sets default filter to be identity.
vtkImageMean1D::vtkImageMean1D()
{
  this->SetAxes(VTK_IMAGE_X_AXIS);
  this->SetStride(1);
  this->SetKernelSize(1);

  // For better performance, the execute function was written as a 2d.
  this->ExecuteDimensionality = 2;
}



//----------------------------------------------------------------------------
// Description:
// This sets the KernelSize and sets the (default) KernelMiddle to 
// KernelSize / 2;
void vtkImageMean1D::SetKernelSize(int size)
{
  this->KernelSize = size;
  this->KernelMiddle = size / 2;
  this->Modified();
}


//----------------------------------------------------------------------------
// Description:
// This sets the KernelMiddle  if you want to set the KernelMiddle
// explicitly, do it after setting the KernelSize.  The KernelMiddle
// should be in the range [0, Kernelsize).
void vtkImageMean1D::SetKernelMiddle(int middle)
{
  if (middle < 0 || middle >= this->KernelSize)
    {
    vtkWarningMacro(<< "SetKernelMiddle: middle out of range");
    }
  
  this->KernelMiddle = middle;
  this->Modified();
}



//----------------------------------------------------------------------------
// Description:
// This method computes the Region of input necessary to generate outRegion.
void vtkImageMean1D::ComputeRequiredInputRegionExtent(
					      vtkImageRegion *outRegion,
					      vtkImageRegion *inRegion)
{
  int extent[2];
  int *imageExtent;
    
  outRegion->GetExtent(1, extent);
  extent[0] = extent[0] * this->Stride - this->KernelMiddle;
  extent[1] = extent[1] * this->Stride 
    + this->KernelSize - 1 - this->KernelMiddle;
  
  // Clip with input image extent.
  imageExtent = inRegion->GetImageExtent();
  if (extent[0] < imageExtent[0])
    {
    extent[0] = imageExtent[0];
    }
  if (extent[1] > imageExtent[1])
    {
    extent[1] = imageExtent[1];
    }
  
  inRegion->SetExtent(1, extent);
}


//----------------------------------------------------------------------------
// Description:
// Computes any global image information associated with regions.
void vtkImageMean1D::ComputeOutputImageInformation(
		    vtkImageRegion *inRegion, vtkImageRegion *outRegion)
{
  int imageExtent[2];
  float Spacing;

  inRegion->GetImageExtent(1, imageExtent);
  inRegion->GetSpacing(Spacing);

  // Scale the output extent
  imageExtent[0] = (int)(ceil(((float)imageExtent[0]) /((float)this->Stride)));
  imageExtent[1] = (int)(floor(((float)imageExtent[1])/((float)this->Stride)));
  
  // Change the data spacing.
  Spacing *= (float)(this->Stride);

  outRegion->SetImageExtent(1, imageExtent);
  outRegion->SetSpacing(Spacing);
}



//----------------------------------------------------------------------------
// The templated execute function handles all the data types.
// 2d even though operation is 1d.
// Note: Slight misalignment (pixel replication is not nearest neighbor).
template <class T>
static void vtkImageMean1DExecute(vtkImageMean1D *self,
			      vtkImageRegion *inRegion, T *inPtr,
			      vtkImageRegion *outRegion, T *outPtr)
{
  int outMin0, outMax0, outMin1, outMax1;
  int outIdx0, outIdx1; 
  int inInc0, inInc1, outInc0, outInc1;
  int kernelMin, kernelMax, kernelIdx;
  int inImageMin, inImageMax;
  T *inPtr0, *inPtr1, *outPtr0, *outPtr1, *kernelPtr;
  int stride, middle, size;
  float sum;
  
  // Get IVars.
  stride = self->GetStride();
  middle = self->GetKernelMiddle();
  size = self->GetKernelSize();
  inRegion->GetImageExtent(inImageMin, inImageMax);
  inPtr = inPtr;
  
  // Get information to march through data 
  inRegion->GetIncrements(inInc0, inInc1);
  outRegion->GetIncrements(outInc0, outInc1);
  outRegion->GetExtent(outMin0, outMax0, outMin1, outMax1);

  // Loop through output pixels
  inPtr1 = (T *)(inRegion->GetScalarPointer(outMin0 * stride));
  outPtr1 = outPtr;
  for (outIdx1 = outMin1; outIdx1 <= outMax1; ++outIdx1)
    {
    inPtr0 = inPtr1;
    outPtr0 = outPtr1;
    for (outIdx0 = outMin0; outIdx0 <= outMax0; ++outIdx0)
      {
      
      // compute extent of kernel at this position
      kernelMin = outIdx0 * stride - middle;
      kernelMax = kernelMin + size - 1;
      // Note: this pointer may be invalid when out of bounds.
      kernelPtr = inPtr0 - middle * inInc0;
      sum = 0.0;
      for (kernelIdx = kernelMin; kernelIdx <= kernelMax; ++kernelIdx)
	{
	if ((kernelIdx >= inImageMin) && (kernelIdx <= inImageMax))
	  {
	  sum += (float)(*kernelPtr);
	  }
	kernelPtr += inInc0;
	}
      
      *outPtr0 = (T)(sum / (float)(size));

      inPtr0 += inInc0 * stride;
      outPtr0 += outInc0;
      }
    inPtr1 += inInc1;
    outPtr1 += outInc1;
    }
}

    
//----------------------------------------------------------------------------
// Description:
// This method uses the input region to fill the output region.
// It can handle any type data, but the two regions must have the same 
// data type.
void vtkImageMean1D::Execute(vtkImageRegion *inRegion, 
				  vtkImageRegion *outRegion)
{
  void *inPtr = inRegion->GetScalarPointer();
  void *outPtr = outRegion->GetScalarPointer();
  
  // this filter expects that input is the same type as output.
  if (inRegion->GetScalarType() != outRegion->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inRegion->GetScalarType()
         << ", must match out ScalarType " << outRegion->GetScalarType());
    return;
    }
  
  switch (inRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageMean1DExecute(this, 
			  inRegion, (float *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageMean1DExecute(this, 
			  inRegion, (int *)(inPtr), 
			  outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageMean1DExecute(this, 
			  inRegion, (short *)(inPtr), 
			  outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageMean1DExecute(this, 
			  inRegion, (unsigned short *)(inPtr), 
			  outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageMean1DExecute(this, 
			  inRegion, (unsigned char *)(inPtr), 
			  outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}
















