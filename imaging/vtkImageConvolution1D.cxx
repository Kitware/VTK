/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageConvolution1D.cxx
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
#include "vtkImageConvolution1D.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageConvolution1D fitler.
vtkImageConvolution1D::vtkImageConvolution1D()
{
  this->Kernel = NULL;
  this->BoundaryFactors = NULL;
  this->SetAxes(VTK_IMAGE_X_AXIS);
  this->BoundaryRescaleOn();
  this->UseExecuteCenterOff();
  this->HandleBoundariesOn();
}


//----------------------------------------------------------------------------
// Description:
// Free the kernel before the object is deleted.
vtkImageConvolution1D::~vtkImageConvolution1D()
{
  if (this->Kernel)
    delete [] this->Kernel;
  if (this->BoundaryFactors)
    delete [] this->BoundaryFactors;
}

//----------------------------------------------------------------------------
void vtkImageConvolution1D::PrintSelf(ostream& os, vtkIndent indent)
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
void vtkImageConvolution1D::SetKernel(float *kernel, int size)
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
  

//****************************************************************************
// Note backward coding BoundaryFactors[0] has the most pixels cut,
// BoundaryFactors[center] has no pixels cut.
void vtkImageConvolution1D::ComputeBoundaryFactors()
{
  int idx;
  int size = this->KernelSize[0];
  int center = this->KernelMiddle[0];
  float totalArea, partialArea;

  if ( ! this->BoundaryFactors)
    {
    vtkErrorMacro(<< "ComputeBoundaryFactors: Kernel not set.");
    return;
    }
  
  // Compute the total area of the kernel
  totalArea = 0.0;
  for (idx = 0; idx < size; ++idx)
    {
    totalArea += this->Kernel[idx];
    }
  
  // middle is always one (no pixels excluded
  this->BoundaryFactors[center] = 1;
  
  // left boundary factors
  partialArea = 0.0;
  for (idx = 0; idx < center; ++idx)
    {
    partialArea += this->Kernel[idx];
    this->BoundaryFactors[center - idx - 1]
      = totalArea / (totalArea - partialArea);
    }

  
  // right boundary factors
  partialArea = 0.0;
  for (idx = size - 1; idx > center; --idx)
    {
    partialArea += this->Kernel[idx];
    this->BoundaryFactors[size-idx+center]
      = totalArea / (totalArea - partialArea);
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
void vtkImageConvolution1DExecute(vtkImageConvolution1D *self,
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
    cerr << "vtkImageConvolution1DExecute: Kernel not set";
    return;
    }

  // Get information to march through data 
  inRegion->GetIncrements(inInc);
  outRegion->GetIncrements(outInc);  
  outRegion->GetExtent(outMin, outMax);  

  // Compute the middle portion of the region 
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
      cerr << "vtkImageConvolution1DExecute: Boundaries not handled.";
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
void vtkImageConvolution1D::Execute(vtkImageRegion *inRegion, 
				    vtkImageRegion *outRegion)
{
  void *inPtr, *outPtr;

  // perform convolution for each pixel of output.
  // Note that input pixel is offset from output pixel.
  inPtr = inRegion->GetScalarPointer();
  outPtr = outRegion->GetScalarWritePointer();

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
      vtkImageConvolution1DExecute(this, inRegion, (float *)(inPtr), 
				 outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageConvolution1DExecute(this, inRegion, (int *)(inPtr),
				 outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageConvolution1DExecute(this, inRegion, (short *)(inPtr),
				 outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageConvolution1DExecute(this,
				 inRegion, (unsigned short *)(inPtr), 
				 outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageConvolution1DExecute(this,
				 inRegion, (unsigned char *)(inPtr),
				 outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}



















