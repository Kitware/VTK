/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageConvolve.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Z. F. Knops who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkImageData.h"

#include "vtkImageConvolve.h"
#include "vtkImageEllipsoidSource.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkImageConvolve* vtkImageConvolve::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageConvolve");
  if(ret)
    {
    return (vtkImageConvolve*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageConvolve;
}

//----------------------------------------------------------------------------
// Construct an instance of vtkImageConvolve fitler.
// By default zero values are eroded.
vtkImageConvolve::vtkImageConvolve()
{
  // Construct a primary id function kernel that does nothing at all
  float kernel[9];

  for (int idx = 0; idx < 9; idx++)
    {
    kernel[idx] = 0.0;
    }

  kernel[4] = 1.0; 

  this->SetKernel3x3(kernel);
}


//----------------------------------------------------------------------------
// Destructor
vtkImageConvolve::~vtkImageConvolve()
{
}


//----------------------------------------------------------------------------
void vtkImageConvolve::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkImageToImageFilter::PrintSelf(os, indent);
  
  os << indent << "KernelSize: (" <<
    this->KernelSize[0] << ", " <<
    this->KernelSize[1] << ", " <<
    this->KernelSize[2] << ")\n";

  os << indent << "Kernel: (";
  for (int k = 0; k < this->KernelSize[2]; k++)
    {
    for (int j = 0; j < this->KernelSize[1]; j++)
      {
      for (int i = 0; i < this->KernelSize[0]; i++)
        {
        os << this->Kernel[this->KernelSize[1]*this->KernelSize[0]*k +
                           this->KernelSize[0]*j +
                           i];
	
        if (i != this->KernelSize[0] - 1)
          {
          os << ", ";
          }
	}
      if (j != this->KernelSize[1] - 1 || k != this->KernelSize[2] - 1)
        {
        os << ",\n" << indent << "         ";
	}
      }
    }
  os << ")\n";	      
}


//----------------------------------------------------------------------------
// Set a 3x3 kernel 
void vtkImageConvolve::SetKernel3x3(const float kernel[9])
{  
  // Fill the kernel
  this->SetKernel(kernel, 3, 3, 1);
}


//----------------------------------------------------------------------------
// Set a 5x5 kernel 
void vtkImageConvolve::SetKernel5x5(const float kernel[25])
{
  // Fill the kernel
  this->SetKernel(kernel, 5, 5, 1);
}

//----------------------------------------------------------------------------
// Set a 7x7 kernel 
void vtkImageConvolve::SetKernel7x7(float kernel[49])
{
  // Fill the kernel
  this->SetKernel(kernel, 7, 7, 1);
}

//----------------------------------------------------------------------------
// Set a 3x3x3 kernel
void vtkImageConvolve::SetKernel3x3x3(const float kernel[27])
{
  // Fill the kernel
  this->SetKernel(kernel, 3, 3, 3);
}

//----------------------------------------------------------------------------
// Set a 5x5x5 kernel
void vtkImageConvolve::SetKernel5x5x5(float kernel[125])
{
  // Fill the kernel
  this->SetKernel(kernel, 5, 5, 5);
}

//----------------------------------------------------------------------------
// Set a 7x7x7 kernel
void vtkImageConvolve::SetKernel7x7x7(float kernel[343])
{
  // Fill the kernel
  this->SetKernel(kernel, 7, 7, 7);
}

//----------------------------------------------------------------------------
// Set a kernel, this is an internal method
void vtkImageConvolve::SetKernel(const float* kernel,
				 int sizeX, int sizeY, int sizeZ)
{
  // Set the correct kernel size
  this->KernelSize[0] = sizeX;
  this->KernelSize[1] = sizeY;
  this->KernelSize[2] = sizeZ;

  int kernelLength = sizeX*sizeY*sizeZ;

  for (int idx = 0; idx < kernelLength; idx++)
    {
    this->Kernel[idx] = kernel[idx];
    }
}

//----------------------------------------------------------------------------
// Get the 3x3 kernel
float* vtkImageConvolve::GetKernel3x3()
{
  return this->GetKernel();
}

//----------------------------------------------------------------------------
// Get the 5x5 kernel
float* vtkImageConvolve::GetKernel5x5()
{
  return this->GetKernel();
}

//----------------------------------------------------------------------------
// Get the 7x7 kernel
float* vtkImageConvolve::GetKernel7x7()
{
  return this->GetKernel();
}

//----------------------------------------------------------------------------
// Get the 3x3x3 kernel
float* vtkImageConvolve::GetKernel3x3x3()
{
  return this->GetKernel();
}

//----------------------------------------------------------------------------
// Get the 5x5x5 kernel
float* vtkImageConvolve::GetKernel5x5x5()
{
  return this->GetKernel();
}

//----------------------------------------------------------------------------
// Get the 7x7x7 kernel
float* vtkImageConvolve::GetKernel7x7x7()
{
  return this->GetKernel();
}

//----------------------------------------------------------------------------
// Get the kernel, this is an internal method
float* vtkImageConvolve::GetKernel()
{
  return this->Kernel;
}

//----------------------------------------------------------------------------
// Get the kernel
void vtkImageConvolve::GetKernel3x3(float kernel[9])
{
  this->GetKernel(kernel);
}

//----------------------------------------------------------------------------
// Get the kernel
void vtkImageConvolve::GetKernel5x5(float kernel[25])
{
  this->GetKernel(kernel);
}

//----------------------------------------------------------------------------
// Get the kernel
void vtkImageConvolve::GetKernel7x7(float kernel[49])
{
  this->GetKernel(kernel);
}

//----------------------------------------------------------------------------
// Get the kernel
void vtkImageConvolve::GetKernel3x3x3(float kernel[27])
{
  this->GetKernel(kernel);
}

//----------------------------------------------------------------------------
// Get the kernel
void vtkImageConvolve::GetKernel5x5x5(float kernel[125])
{
  this->GetKernel(kernel);
}

//----------------------------------------------------------------------------
// Get the kernel
void vtkImageConvolve::GetKernel7x7x7(float kernel[343])
{
  this->GetKernel(kernel);
}

//----------------------------------------------------------------------------
// Get the kernel, this is an internal method
void vtkImageConvolve::GetKernel(float *kernel)
{
  int kernelLength = this->KernelSize[0]*
    this->KernelSize[1]*this->KernelSize[2];

  for (int idx = 0; idx < kernelLength; idx++)
    {
    kernel[idx] = this->Kernel[idx];
    }
}

//----------------------------------------------------------------------------
// This templated function executes the filter on any region,
// whether it needs boundary checking or not.
// If the filter needs to be faster, the function could be duplicated
// for strictly center (no boundary) processing.
template <class T>
static void vtkImageConvolveExecute(vtkImageConvolve *self,
                                    vtkImageData *inData, T *inPtr, 
                                    vtkImageData *outData, T *outPtr,
                                    int outExt[6], int id)
{
  int *kernelSize;
  int kernelMiddle[3];

  // For looping though output (and input) pixels.
  int outMin0, outMax0, outMin1, outMax1, outMin2, outMax2;
  int outIdx0, outIdx1, outIdx2;
  int inInc0, inInc1, inInc2;
  int outInc0, outInc1, outInc2;
  T *inPtr0, *inPtr1, *inPtr2;
  T *outPtr0, *outPtr1, *outPtr2;
  int numComps, outIdxC;

  // For looping through hood pixels
  int hoodMin0, hoodMax0, hoodMin1, hoodMax1, hoodMin2, hoodMax2;
  int hoodIdx0, hoodIdx1, hoodIdx2;
  T *hoodPtr0, *hoodPtr1, *hoodPtr2;

  // For looping through the kernel, and compute the kernel result
  int kernelIdx;
  float sum;

  // The extent of the whole input image
  int inImageMin0, inImageMin1, inImageMin2;
  int inImageMax0, inImageMax1, inImageMax2;

  // to compute the range
  unsigned long count = 0;
  unsigned long target;

  // Get information to march through data
  inData->GetIncrements(inInc0, inInc1, inInc2); 
  self->GetInput()->GetWholeExtent(inImageMin0, inImageMax0, inImageMin1,
                                   inImageMax1, inImageMin2, inImageMax2);
  outData->GetIncrements(outInc0, outInc1, outInc2); 
  outMin0 = outExt[0];   outMax0 = outExt[1];
  outMin1 = outExt[2];   outMax1 = outExt[3];
  outMin2 = outExt[4];   outMax2 = outExt[5];
  numComps = outData->GetNumberOfScalarComponents();
   
  // Get ivars of this object (easier than making friends)
  kernelSize = self->GetKernelSize();

  kernelMiddle[0] = kernelSize[0] / 2;
  kernelMiddle[1] = kernelSize[1] / 2;
  kernelMiddle[2] = kernelSize[2] / 2;

  hoodMin0 = -kernelMiddle[0];
  hoodMin1 = -kernelMiddle[1];
  hoodMin2 = -kernelMiddle[2];

  hoodMax0 = hoodMin0 + kernelSize[0] - 1;
  hoodMax1 = hoodMin1 + kernelSize[1] - 1;
  hoodMax2 = hoodMin2 + kernelSize[2] - 1;

  // Get the kernel, just use GetKernel7x7x7(kernel) if the kernel is smaller
  // it still works :)
  float kernel[343];
  self->GetKernel7x7x7(kernel);

  // in and out should be marching through corresponding pixels.
  inPtr = (T *)(inData->GetScalarPointer(outMin0, outMin1, outMin2));

  target = (unsigned long)(numComps*(outMax2 - outMin2 + 1)*
                                    (outMax1 - outMin1 + 1)/50.0);
  target++;
  
  // loop through components
  for (outIdxC = 0; outIdxC < numComps; ++outIdxC)
    {
    // loop through pixels of output
    outPtr2 = outPtr;
    inPtr2 = inPtr;
    for (outIdx2 = outMin2; outIdx2 <= outMax2; ++outIdx2)
      {
      outPtr1 = outPtr2;
      inPtr1 = inPtr2;
      for (outIdx1 = outMin1; 
           outIdx1 <= outMax1 && !self->AbortExecute; 
           ++outIdx1)
        {
        if (!id)
          {
          if (!(count%target))
            {
            self->UpdateProgress(count/(50.0*target));
            }
          count++;
          }

        outPtr0 = outPtr1;
        inPtr0 = inPtr1;

        for (outIdx0 = outMin0; outIdx0 <= outMax0; ++outIdx0)
          {
          // Inner loop where we compute the kernel

          // Set the sum to zero
          sum = 0;

          // loop through neighborhood pixels
          // as sort of a hack to handle boundaries, 
          // input pointer will be marching through data that does not exist.
          hoodPtr2 = inPtr0 - kernelMiddle[0] * inInc0 
                            - kernelMiddle[1] * inInc1 
                            - kernelMiddle[2] * inInc2;

          // Set the kernel index to the starting position
          kernelIdx = 0;

          for (hoodIdx2 = hoodMin2; hoodIdx2 <= hoodMax2; ++hoodIdx2)
	    {
            hoodPtr1 = hoodPtr2;

            for (hoodIdx1 = hoodMin1; hoodIdx1 <= hoodMax1; ++hoodIdx1)
	      {
              hoodPtr0 = hoodPtr1;

              for (hoodIdx0 = hoodMin0; hoodIdx0 <= hoodMax0; ++hoodIdx0)
                {
                // A quick but rather expensive way to handle boundaries
                // This assumes the boundary values are zero
                if (outIdx0 + hoodIdx0 >= inImageMin0 &&
                    outIdx0 + hoodIdx0 <= inImageMax0 &&
                    outIdx1 + hoodIdx1 >= inImageMin1 &&
                    outIdx1 + hoodIdx1 <= inImageMax1 &&
                    outIdx2 + hoodIdx2 >= inImageMin2 &&
                    outIdx2 + hoodIdx2 <= inImageMax2)
                  {
                  sum += *hoodPtr0 * kernel[kernelIdx];

                  // Take the next postion in the kernel
                  kernelIdx++;
                  }

                hoodPtr0 += inInc0;
                }

              hoodPtr1 += inInc1;
              }

            hoodPtr2 += inInc2;
            }

          // Set the output pixel to the correct value
          *outPtr0 = (T)sum;

          inPtr0 += inInc0;
          outPtr0 += outInc0;
          }

        inPtr1 += inInc1;
        outPtr1 += outInc1;
        }

      inPtr2 += inInc2;
      outPtr2 += outInc2;
      }

    ++inPtr;
    ++outPtr;
    }
}


    

//----------------------------------------------------------------------------
// This method contains the first switch statement that calls the correct
// templated function for the input and output Data types.
// It hanldes image boundaries, so the image does not shrink.
void vtkImageConvolve::ThreadedExecute(vtkImageData *inData, 
                                       vtkImageData *outData, 
                                       int outExt[6], int id)
{
  int inExt[6];
  this->ComputeInputUpdateExtent(inExt,outExt);
  void *inPtr = inData->GetScalarPointerForExtent(inExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);

  // this filter expects the output type to be same as input
  if (outData->GetScalarType() != inData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: output ScalarType, "
      << vtkImageScalarTypeNameMacro(outData->GetScalarType())
      << " must match input scalar type");
    return;
    }
  
  switch (inData->GetScalarType())
    {
    vtkTemplateMacro7(vtkImageConvolveExecute, this, inData, 
                      (VTK_TT *)(inPtr), outData, (VTK_TT *)(outPtr),
                      outExt, id);

    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}



