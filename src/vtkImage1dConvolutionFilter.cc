/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImage1dConvolutionFilter.cc
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
#include "vtkImage1dConvolutionFilter.hh"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImage1dConvolutionFilter fitler.
vtkImage1dConvolutionFilter::vtkImage1dConvolutionFilter()
{
  this->Kernel = NULL;
  this->KernelSize = 0;
  this->Axis = 0;
}


//----------------------------------------------------------------------------
// Description:
// Free the kernel before the object is deleted.
vtkImage1dConvolutionFilter::~vtkImage1dConvolutionFilter()
{
  if (this->Kernel)
    delete [] this->Kernel;
}


//----------------------------------------------------------------------------
// Description:
// Returns the largest region which can be requested.
// Since borders are not handled yet, the valid image shrinks.
void vtkImage1dConvolutionFilter::GetBoundary(int *offset, int *size)
{
  // get the Boundary of the input
  this->Input->GetBoundary(offset, size);
  
  // modify the axis of the smoothing
  offset[this->Axis] -= this->KernelOffset;
  size[this->Axis] -= (this->KernelSize - 1);
  
  vtkDebugMacro(<< "GetBoundary: returning offset = ("
          << offset[0] << ", " << offset[1] << ", " << offset[2]
          << "), size = (" << size[0] << ", " << size[1] << ", " << size[2]
          << ")");  
}



//----------------------------------------------------------------------------
// Description:
// This method copies a kernel into the filter.
void vtkImage1dConvolutionFilter::SetKernel(float *kernel, int kernelSize)
{
  int idx;

  vtkDebugMacro(<< "SetKernel: kernel = " << kernel 
		<< ", kernelSize = " << kernelSize);

  // free the old kernel 
  if (this->Kernel)
    delete [] this->Kernel;

  // allocate memory for the kernel 
  this->Kernel = new float[kernelSize];
  if ( ! this->Kernel)
    {
    vtkWarningMacro(<<"Could not allocate memory for kernel.");
    this->KernelSize = 0;
    return;
    }

  this->KernelSize = kernelSize;
  this->KernelOffset = -kernelSize / 2;
  
  // copy kernel 
  for (idx = 0; idx < kernelSize; ++idx)
    this->Kernel[idx] = kernel[idx];

  this->Modified();
}
  



//----------------------------------------------------------------------------
// Description:
// This method computes the Region of the input necessary to generate out Region
void vtkImage1dConvolutionFilter::RequiredRegion(int *outOffset, int *outSize,
				int *inOffset, int *inSize)
{
  int idx;

  // ignoring boundaries for now 
  for (idx = 0; idx < 3; ++idx)
    {
    inOffset[idx] = outOffset[idx];
    inSize[idx] = outSize[idx];
    }
  if ( ! this->Kernel)
    {
    vtkWarningMacro(<<"Kernel not set.");
    return;
    }

  inOffset[this->Axis] += this->KernelOffset;
  inSize[this->Axis] += this->KernelSize - 1;
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output tile, and executes the Conv1d
// algorithm to fill the output from the input.
// As a place holder, an identity Conv1d is implemented.
void vtkImage1dConvolutionFilter::Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion)
{
  int size0, size1, size2;
  int idx0, idx1, idx2;
  int inInc0, inInc1, inInc2;
  int outInc0, outInc1, outInc2;
  float *inPtr0, *inPtr1, *inPtr2;
  float *outPtr0, *outPtr1, *outPtr2;
  int kernelIdx, tmpInc, *tmpIncAxes;
  float *kernelPtr, *tmpPtr;

  if ( ! this->Kernel)
    {
    vtkWarningMacro(<<"Kernel not set");
    return;
    }

  // determine which axis will be convolved 
  tmpIncAxes = inRegion->GetInc();
  tmpInc = tmpIncAxes[this->Axis];
  
  // Get information to march through data 
  inPtr2 = inRegion->GetPointer(inRegion->GetOffset());
  inRegion->GetInc(inInc0, inInc1, inInc2);  
  outPtr2 = outRegion->GetPointer(outRegion->GetOffset());
  outRegion->GetInc(outInc0, outInc1, outInc2);  
  outRegion->GetSize(size0, size1, size2);  

  vtkDebugMacro(<< "Execute: inRegion = (" << inRegion 
		<< "), outRegion = (" << outRegion << ")");
    
  // perform convolution for each pixel of output 
  for (idx2 = 0; idx2 < size2; ++idx2)
    {
    outPtr1 = outPtr2;
    inPtr1 = inPtr2;
    for (idx1 = 0; idx1 < size1; ++idx1)
      {
      outPtr0 = outPtr1;
      inPtr0 = inPtr1;
      for (idx0 = 0; idx0 < size0; ++idx0)
	{

	// loop for comvolution 
	*outPtr0 = 0.0;
	kernelPtr = this->Kernel;
	tmpPtr = inPtr0;
	for (kernelIdx = 0; kernelIdx < this->KernelSize; ++kernelIdx)
	  {
	  *outPtr0 += *kernelPtr * *tmpPtr;
	  ++kernelPtr;
	  tmpPtr += tmpInc;
	  }

	outPtr0 += outInc0;
	inPtr0 += inInc0;
	}
      outPtr1 += outInc1;
      inPtr1 += inInc1;
      }
    outPtr2 += outInc2;
    inPtr2 += inInc2;
    }
}
















