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
  this->SetAxes(VTK_IMAGE_X_AXIS);
  this->BoundaryRescale = 1;
  this->HandleBoundariesOn();

  // Poor performance, but simple implementation.
  this->ExecuteDimensionality = 1;
  this->Dimensionality = 1;
}


//----------------------------------------------------------------------------
// Description:
// Free the kernel before the object is deleted.
vtkImageConvolution1D::~vtkImageConvolution1D()
{
  if (this->Kernel)
    delete [] this->Kernel;
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
}

//----------------------------------------------------------------------------
// Description:
// This method copies a kernel into the filter.
void vtkImageConvolution1D::SetKernel(float *kernel, int size)
{
  int idx;
  float sum;

  vtkDebugMacro(<< "SetKernel: kernel = " << kernel 
		<< ", size = " << size);

  // free the old kernel 
  if (this->Kernel)
    delete [] this->Kernel;

  // allocate memory for the kernel 
  this->Kernel = new float[size];
  if ( ! this->Kernel)
    {
    vtkWarningMacro(<<"Could not allocate memory for kernel.");
    return;
    }

  // copy kernel 
  sum = 0.0;
  for (idx = 0; idx < size; ++idx)
    {
    this->Kernel[idx] = kernel[idx];
    sum += kernel[idx];
    }
  this->KernelSize[0] = size;
  this->KernelMiddle[0] = size / 2;
  this->KernelArea = sum;
  
  this->Modified();
}
  

//----------------------------------------------------------------------------
// This templated function is passed a input and output region, 
// and executes the Conv1d algorithm to fill the output from the input.
// Note that input pixel is offset from output pixel.
template <class T>
void vtkImageConvolution1DExecuteCenter(vtkImageConvolution1D *self,
					vtkImageRegion *inRegion, T *inPtr,
					vtkImageRegion *outRegion, T *outPtr)
{
  int outIdx, kernelIdx;
  int outMin, outMax;
  int inInc, outInc;
  T *tmpPtr;
  float *kernelPtr;
  float sum;
  int stride, size;
  
  if ( ! self->Kernel)
    {
    cerr << "vtkImageConvolution1DExecuteCenter: Kernel not set";
    return;
    }

  // Get information to march through data 
  inRegion->GetIncrements(inInc);
  outRegion->GetIncrements(outInc);  
  outRegion->GetExtent(outMin, outMax);  
  stride = self->Strides[0];
  size = self->KernelSize[0];

  // loop through output pixels
  for (outIdx = outMin ; outIdx <= outMax; ++outIdx)
    {
    // loop for convolution 
    sum = 0.0;
    kernelPtr = self->Kernel;
    tmpPtr = inPtr;
    for (kernelIdx = 0; kernelIdx < size; ++kernelIdx)
      {
      sum += *kernelPtr * (float)(*tmpPtr);
      ++kernelPtr;
      tmpPtr += inInc;
      }
    *outPtr = (T)(sum);
    
    outPtr += outInc;
    inPtr += inInc * stride;
    }
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output region, and executes the Conv1d
// algorithm to fill the output from the input.  The regions do not have any
// boundary conditions.
void vtkImageConvolution1D::ExecuteCenter(vtkImageRegion *inRegion, 
					  vtkImageRegion *outRegion)
{
  void *inPtr, *outPtr;

  // perform convolution for each pixel of output.
  // Note that input pixel is offset from output pixel.
  inPtr = inRegion->GetScalarPointer();
  outPtr = outRegion->GetScalarPointer();

  // this filter expects that input is the same type as output.
  if (inRegion->GetScalarType() != outRegion->GetScalarType())
    {
    vtkErrorMacro(<< "ExecuteCenter: input ScalarType, " 
        << inRegion->GetScalarType() << ", must match out ScalarType " 
        << outRegion->GetScalarType());
    return;
    }

  // choose which templated function to call.
  switch (inRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageConvolution1DExecuteCenter(this, inRegion, (float *)(inPtr), 
				 outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageConvolution1DExecuteCenter(this, inRegion, (int *)(inPtr),
				 outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageConvolution1DExecuteCenter(this, inRegion, (short *)(inPtr),
				 outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageConvolution1DExecuteCenter(this,
				 inRegion, (unsigned short *)(inPtr), 
				 outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageConvolution1DExecuteCenter(this,
				 inRegion, (unsigned char *)(inPtr),
				 outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "ExecuteCenter: Unknown ScalarType");
      return;
    }
}


//****************************************************************************
// Some duplication of code, but it makes the boundary 
// conditions much easier to handle.
//****************************************************************************


//----------------------------------------------------------------------------
// This templated function is passed a input and output region, 
// and executes the Conv1d algorithm to fill the output from the input.
// Note that input pixel is offset from output pixel.
// It is only call with boundary regions.
template <class T>
void vtkImageConvolution1DExecute(vtkImageConvolution1D *self,
				  vtkImageRegion *inRegion, T *inPtr,
				  vtkImageRegion *outRegion, T *outPtr)
{
  int outIdx, kernelIdx;
  int inMin, inMax, outMin, outMax;
  int inInc, outInc;
  int kernelMin, kernelMax;
  float *kernelPtr;
  T *tmpPtr;
  float sum, area;
  int stride, kernelMiddle, kernelSize;
  
  if ( ! self->Kernel)
    {
    cerr << "vtkImageConvolution1DExecuteCenter: Kernel not set";
    return;
    }
  
  // Get information about kernel.
  kernelMiddle = self->KernelMiddle[0];
  kernelSize = self->KernelSize[0];
  
  // Get information to march through data 
  inRegion->GetIncrements(inInc);
  inRegion->GetExtent(inMin, inMax);  
  outRegion->GetIncrements(outInc);  
  outRegion->GetExtent(outMin, outMax);  
  stride = self->Strides[0];

  // Input pointer should correspond to the output pixel.
  inPtr = (T *)(inRegion->GetScalarPointer(outMin*stride));

  // loop through output pixels
  for (outIdx = outMin; outIdx <= outMax; ++outIdx)
    {
    // Compute the start and end of kernel (because of boundary conditions)
    kernelMin = inMin - (outIdx*stride - kernelMiddle);
    if (kernelMin < 0) kernelMin = 0;
    kernelMax = inMax - (outIdx*stride - kernelMiddle);
    if (kernelMax >= kernelSize) kernelMax = kernelSize - 1;
     
    // shift input pointer from middle to start of kernel
    tmpPtr = inPtr + (kernelMin-kernelMiddle)*inInc;
    // loop for convolution 
    sum = area = 0.0;
    kernelPtr = self->Kernel + kernelMin;
    for (kernelIdx = kernelMin; kernelIdx <= kernelMax; ++kernelIdx)
      {
      // Sum for convolution.
      sum += *kernelPtr * (float)(*tmpPtr);
      // Keep track of partial area for convolution.
      area += *kernelPtr;
      ++kernelPtr;
      tmpPtr += inInc;
      }
    *outPtr = (T)(sum);
    // Rescale if flag is on
    if (self->BoundaryRescale)
      {
      *outPtr = (T) ((float) *outPtr * (self->KernelArea / area));
      }
    
    outPtr += outInc;
    inPtr += inInc * stride;
    }
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output region, and executes the Conv1d
// algorithm to fill the output from the input.  The regions have only
// pixels affected by boundary conditions.
void vtkImageConvolution1D::Execute(vtkImageRegion *inRegion, 
				    vtkImageRegion *outRegion)
{
  void *inPtr, *outPtr;

  // perform convolution for each pixel of output.
  // Note: Temporary place holder for input pointer.
  inPtr = NULL;
  outPtr = outRegion->GetScalarPointer();

  // this filter expects that input is the same type as output.
  if (inRegion->GetScalarType() != outRegion->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " 
        << inRegion->GetScalarType() << ", must match out ScalarType " 
        << outRegion->GetScalarType());
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




















