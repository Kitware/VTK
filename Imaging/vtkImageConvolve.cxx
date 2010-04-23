/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageConvolve.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageConvolve.h"
#include "vtkImageData.h"
#include "vtkImageEllipsoidSource.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageConvolve);

//----------------------------------------------------------------------------
// Construct an instance of vtkImageConvolve fitler.
// By default zero values are eroded.
vtkImageConvolve::vtkImageConvolve()
{
  int idx;
  for (idx = 0; idx < 343; idx++)
    {
    this->Kernel[idx] = 0.0;
    }

  // Construct a primary id function kernel that does nothing at all
  double kernel[9];
  for (idx = 0; idx < 9; idx++)
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
  this->Superclass::PrintSelf(os, indent);
  
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
void vtkImageConvolve::SetKernel3x3(const double kernel[9])
{  
  // Fill the kernel
  this->SetKernel(kernel, 3, 3, 1);
}

//----------------------------------------------------------------------------
// Set a 5x5 kernel 
void vtkImageConvolve::SetKernel5x5(const double kernel[25])
{
  // Fill the kernel
  this->SetKernel(kernel, 5, 5, 1);
}

//----------------------------------------------------------------------------
// Set a 7x7 kernel 
void vtkImageConvolve::SetKernel7x7(double kernel[49])
{
  // Fill the kernel
  this->SetKernel(kernel, 7, 7, 1);
}

//----------------------------------------------------------------------------
// Set a 3x3x3 kernel
void vtkImageConvolve::SetKernel3x3x3(const double kernel[27])
{
  // Fill the kernel
  this->SetKernel(kernel, 3, 3, 3);
}

//----------------------------------------------------------------------------
// Set a 5x5x5 kernel
void vtkImageConvolve::SetKernel5x5x5(double kernel[125])
{
  // Fill the kernel
  this->SetKernel(kernel, 5, 5, 5);
}

//----------------------------------------------------------------------------
// Set a 7x7x7 kernel
void vtkImageConvolve::SetKernel7x7x7(double kernel[343])
{
  // Fill the kernel
  this->SetKernel(kernel, 7, 7, 7);
}

//----------------------------------------------------------------------------
// Set a kernel, this is an internal method
void vtkImageConvolve::SetKernel(const double* kernel,
                                 int sizeX, int sizeY, int sizeZ)
{
  int modified=0;

  // Set the correct kernel size
  this->KernelSize[0] = sizeX;
  this->KernelSize[1] = sizeY;
  this->KernelSize[2] = sizeZ;

  int kernelLength = sizeX*sizeY*sizeZ;

  for (int idx = 0; idx < kernelLength; idx++)
    {
    if ( this->Kernel[idx] != kernel[idx] )
      {
      modified = 1;
      this->Kernel[idx] = kernel[idx];
      }
    }
  if (modified)
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Get the 3x3 kernel
double* vtkImageConvolve::GetKernel3x3()
{
  return this->GetKernel();
}

//----------------------------------------------------------------------------
// Get the 5x5 kernel
double* vtkImageConvolve::GetKernel5x5()
{
  return this->GetKernel();
}

//----------------------------------------------------------------------------
// Get the 7x7 kernel
double* vtkImageConvolve::GetKernel7x7()
{
  return this->GetKernel();
}

//----------------------------------------------------------------------------
// Get the 3x3x3 kernel
double* vtkImageConvolve::GetKernel3x3x3()
{
  return this->GetKernel();
}

//----------------------------------------------------------------------------
// Get the 5x5x5 kernel
double* vtkImageConvolve::GetKernel5x5x5()
{
  return this->GetKernel();
}

//----------------------------------------------------------------------------
// Get the 7x7x7 kernel
double* vtkImageConvolve::GetKernel7x7x7()
{
  return this->GetKernel();
}

//----------------------------------------------------------------------------
// Get the kernel, this is an internal method
double* vtkImageConvolve::GetKernel()
{
  return this->Kernel;
}

//----------------------------------------------------------------------------
// Get the kernel
void vtkImageConvolve::GetKernel3x3(double kernel[9])
{
  this->GetKernel(kernel);
}

//----------------------------------------------------------------------------
// Get the kernel
void vtkImageConvolve::GetKernel5x5(double kernel[25])
{
  this->GetKernel(kernel);
}

//----------------------------------------------------------------------------
// Get the kernel
void vtkImageConvolve::GetKernel7x7(double kernel[49])
{
  this->GetKernel(kernel);
}

//----------------------------------------------------------------------------
// Get the kernel
void vtkImageConvolve::GetKernel3x3x3(double kernel[27])
{
  this->GetKernel(kernel);
}

//----------------------------------------------------------------------------
// Get the kernel
void vtkImageConvolve::GetKernel5x5x5(double kernel[125])
{
  this->GetKernel(kernel);
}

//----------------------------------------------------------------------------
// Get the kernel
void vtkImageConvolve::GetKernel7x7x7(double kernel[343])
{
  this->GetKernel(kernel);
}

//----------------------------------------------------------------------------
// Get the kernel, this is an internal method
void vtkImageConvolve::GetKernel(double *kernel)
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
void vtkImageConvolveExecute(vtkImageConvolve *self,
                             vtkImageData *inData, T *inPtr, 
                             vtkImageData *outData, T *outPtr,
                             int outExt[6], int id,
                             vtkInformation *inInfo)
{
  int *kernelSize;
  int kernelMiddle[3];

  // For looping though output (and input) pixels.
  int outMin0, outMax0, outMin1, outMax1, outMin2, outMax2;
  int outIdx0, outIdx1, outIdx2;
  vtkIdType inInc0, inInc1, inInc2;
  vtkIdType outInc0, outInc1, outInc2;
  T *inPtr0, *inPtr1, *inPtr2;
  T *outPtr0, *outPtr1, *outPtr2;
  int numComps, outIdxC;

  // For looping through hood pixels
  int hoodMin0, hoodMax0, hoodMin1, hoodMax1, hoodMin2, hoodMax2;
  int hoodIdx0, hoodIdx1, hoodIdx2;
  T *hoodPtr0, *hoodPtr1, *hoodPtr2;

  // For looping through the kernel, and compute the kernel result
  int kernelIdx;
  double sum;

  // The extent of the whole input image
  int inImageExt[6];

  // to compute the range
  unsigned long count = 0;
  unsigned long target;

  // Get information to march through data
  inData->GetIncrements(inInc0, inInc1, inInc2);
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inImageExt);
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
  double kernel[343];
  self->GetKernel7x7x7(kernel);

  // in and out should be marching through corresponding pixels.
  inPtr = static_cast<T *>(
    inData->GetScalarPointer(outMin0, outMin1, outMin2));

  target = static_cast<unsigned long>(numComps*(outMax2 - outMin2 + 1)*
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
                if (outIdx0 + hoodIdx0 >= inImageExt[0] &&
                    outIdx0 + hoodIdx0 <= inImageExt[1] &&
                    outIdx1 + hoodIdx1 >= inImageExt[2] &&
                    outIdx1 + hoodIdx1 <= inImageExt[3] &&
                    outIdx2 + hoodIdx2 >= inImageExt[4] &&
                    outIdx2 + hoodIdx2 <= inImageExt[5])
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
          *outPtr0 = static_cast<T>(sum);

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
void vtkImageConvolve::ThreadedRequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector),
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
  void *inPtr = inData[0][0]->GetScalarPointerForExtent(outExt);
  void *outPtr = outData[0]->GetScalarPointerForExtent(outExt);

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  // this filter expects the output type to be same as input
  if (outData[0]->GetScalarType() != inData[0][0]->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: output ScalarType, "
      << vtkImageScalarTypeNameMacro(outData[0]->GetScalarType())
      << " must match input scalar type");
    return;
    }
  
  switch (inData[0][0]->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageConvolveExecute(this, inData[0][0],
                              static_cast<VTK_TT *>(inPtr), outData[0], 
                              static_cast<VTK_TT *>(outPtr),
                              outExt, id, inInfo));

    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}
