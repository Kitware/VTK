/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageConvolution1d.cxx
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
#include "vtkImageConvolution1d.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageConvolution1d fitler.
vtkImageConvolution1d::vtkImageConvolution1d()
{
  this->Kernel = NULL;
  this->BoundaryFactors = NULL;
  this->SetAxes1d(VTK_IMAGE_X_AXIS);
  this->BoundaryRescaleOn();
  this->UseExecuteCenterOff();
  this->HandleBoundariesOn();
}


//----------------------------------------------------------------------------
// Description:
// Free the kernel before the object is deleted.
vtkImageConvolution1d::~vtkImageConvolution1d()
{
  if (this->Kernel)
    delete [] this->Kernel;
  if (this->BoundaryFactors)
    delete [] this->BoundaryFactors;
}

//----------------------------------------------------------------------------
void vtkImageConvolution1d::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  this->vtkImageSpatialFilter::PrintSelf(os, indent);
  os << indent << "BoundaryRescale: " << this->BoundaryRescale << "\n";
  if (this->Kernel && this->KernelSize[0] > 0)
    {
    os << indent << "Kernel: (" << this->Kernel[0];
    for (idx = 1; idx < this->KernelSize[0]; ++idx)
      {
      os << ", " << this->Kernel[idx];
      }
    os << ")\n";
    }
  
  if (this->BoundaryFactors && this->KernelSize[0] > 0)
    {
    os << indent << "BoundaryFactors: (" << this->BoundaryFactors[0];
    for (idx = 1; idx < this->KernelSize[0]; ++idx)
      {
      os << ", " << this->BoundaryFactors[idx];
      }
    os << ")\n";
    }
  
}

//----------------------------------------------------------------------------
// Description:
// This method copies a kernel into the filter.
void vtkImageConvolution1d::SetKernel(float *kernel, int size)
{
  int idx, mid;
  float temp;

  vtkDebugMacro(<< "SetKernel: kernel = " << kernel 
		<< ", size = " << size);

  // free the old kernel 
  if (this->Kernel)
    delete [] this->Kernel;
  if (this->BoundaryFactors)
    delete [] this->BoundaryFactors;

  // allocate memory for the kernel 
  this->Kernel = new float[size];
  if ( ! this->Kernel)
    {
    vtkWarningMacro(<<"Could not allocate memory for kernel.");
    return;
    }

  // allocate memory for the boundary-rescale factors. 
  this->BoundaryFactors = new float[size];
  if ( ! this->BoundaryFactors)
    {
    vtkWarningMacro(<<"Could not allocate memory for BoundaryFactors array.");
    delete [] this->Kernel;
    this->Kernel = NULL;
    return;
    }

  // copy kernel 
  for (idx = 0; idx < size; ++idx)
    {
    this->Kernel[idx] = kernel[idx];
    }
  this->KernelSize[0] = size;
  mid = this->KernelMiddle[0] = size / 2;
  
  // compute default BoundaryFactors factors
  temp = (float)(size-1)/2.0;
  for (idx = 0; idx < size; ++idx)
    {
    this->BoundaryFactors[idx] = 
      1.0 / (1.0 - fabs((float)(idx) - temp)/(2.0*temp));
    }

  
  this->Modified();
}
  




//----------------------------------------------------------------------------
// Description:
// This templated function is passed a input and output region, 
// and executes the Conv1d algorithm to fill the output from the input.
// Note that input pixel is offset from output pixel.
// It also handles ImageExtent by truncating the kernel.  
// It renormalizes the truncated kernel if Normalize is on.
template <class T>
void vtkImageConvolution1dExecute1d(vtkImageConvolution1d *self,
					  vtkImageRegion *inRegion, T *inPtr,
					  vtkImageRegion *outRegion, T *outPtr)
{
  int outIdx, kernelIdx;
  int outMin, outMax;
  int inInc, outInc;
  T *tmpPtr;
  float *kernelPtr;
  float sum;
  int cut;
  int outImageExtentMin, outImageExtentMax;
  
  if ( ! self->Kernel)
    {
    cerr << "vtkImageConvolution1dExecute1d: Kernel not set";
    return;
    }

  // Get information to march through data 
  inRegion->GetIncrements1d(inInc);
  outRegion->GetIncrements1d(outInc);  
  outRegion->GetExtent1d(outMin, outMax);  

  // Compute the middle portion of the region 
  // that does not need ImageExtent handling.
  outRegion->GetImageExtent1d(outImageExtentMin, outImageExtentMax);
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
      cerr << "vtkImageConvolution1dExecute1d: Boundaries not handled.";
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
    // The number of pixels cut from the convolution.
    cut = (outImageExtentMin - outIdx);
    // loop for convolution (sum)
    sum = 0.0;
    kernelPtr = self->Kernel + cut;
    tmpPtr = inPtr;
    for (kernelIdx = cut; kernelIdx < self->KernelSize[0]; ++kernelIdx)
      {
      sum += *kernelPtr * (float)(*tmpPtr);
      ++kernelPtr;
      tmpPtr += inInc;
      }
    // Rescale
    if (self->BoundaryRescale)
      {
      sum *= self->BoundaryFactors[self->KernelMiddle[0] - cut];
      }
    // Set output pixel.
    *outPtr = (T)(sum);
    // increment to next pixel.
    outPtr += outInc;
    // the input pixel is not being incremented because of ImageExtent.
    }
  
  // loop through non ImageExtent pixels
  for ( ; outIdx <= outImageExtentMax; ++outIdx)
    {
    // loop for convolution 
    sum = 0.0;
    kernelPtr = self->Kernel;
    tmpPtr = inPtr;
    for (kernelIdx = 0; kernelIdx < self->KernelSize[0]; ++kernelIdx)
      {
      sum += *kernelPtr * (float)(*tmpPtr);
      ++kernelPtr;
      tmpPtr += inInc;
      }
    // Normalization not needed:
    // If Normalize is on, then Normalization[mid] = 1;
    *outPtr = (T)(sum);
    
    outPtr += outInc;
    inPtr += inInc;
    }
  
  
  // loop through the ImageExtent pixels on the right.
  for ( ; outIdx <= outMax; ++outIdx)
    {
    // The number of pixels cut from the convolution.
    cut = (outIdx - outImageExtentMax);
    // loop for convolution (sum)
    sum = 0.0;
    kernelPtr = self->Kernel;
    tmpPtr = inPtr;
    for (kernelIdx = cut; kernelIdx < self->KernelSize[0]; ++kernelIdx)
      {
      sum += *kernelPtr * (float)(*tmpPtr);
      ++kernelPtr;
      tmpPtr += inInc;
      }
    // Normalize sum.
    if (self->BoundaryRescale)
      {
      sum *= self->BoundaryFactors[self->KernelMiddle[0] + cut];
      }
    // Set output pixel.
    *outPtr = (T)(sum);
    // increment to next pixel.
    outPtr += outInc;
    inPtr += inInc;
    }
}




//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output region, and executes the Conv1d
// algorithm to fill the output from the input.
void vtkImageConvolution1d::Execute1d(vtkImageRegion *inRegion, 
					    vtkImageRegion *outRegion)
{
  void *inPtr, *outPtr;

  // perform convolution for each pixel of output.
  // Note that input pixel is offset from output pixel.
  inPtr = inRegion->GetScalarPointer1d();
  outPtr = outRegion->GetScalarPointer1d();

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
      vtkImageConvolution1dExecute1d(this, inRegion, (float *)(inPtr), 
				 outRegion, (float *)(outPtr));
      break;
    case VTK_IMAGE_INT:
      vtkImageConvolution1dExecute1d(this, inRegion, (int *)(inPtr),
				 outRegion, (int *)(outPtr));
      break;
    case VTK_IMAGE_SHORT:
      vtkImageConvolution1dExecute1d(this, inRegion, (short *)(inPtr),
				 outRegion, (short *)(outPtr));
      break;
    case VTK_IMAGE_UNSIGNED_SHORT:
      vtkImageConvolution1dExecute1d(this,
				 inRegion, (unsigned short *)(inPtr), 
				 outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_IMAGE_UNSIGNED_CHAR:
      vtkImageConvolution1dExecute1d(this,
				 inRegion, (unsigned char *)(inPtr),
				 outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown DataType");
      return;
    }
}



















