/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSeparableConvolution.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    

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
#include "vtkImageSeparableConvolution.h"
#include "vtkObjectFactory.h"


// Actually do the convolution
static void ExecuteConvolve ( float* kernel, int kernelSize, float* image, float* outImage, int imageSize )
{

  // Consider the kernel to be centered at (int) ( (kernelSize - 1 ) / 2.0 )
  
  int center = (int) ( (kernelSize - 1 ) / 2.0 );

  int i, j, k, kStart, kEnd, iStart, iEnd, count;
  for ( i = 0; i < imageSize; ++i )
    {
    iStart = i - center;
    if ( iStart < 0 )
      {
      iStart = 0;
      }
    iEnd = i + center;
    if ( iEnd > imageSize - 1 )
      {
      iEnd = imageSize - 1;
      }
    kStart = center + i;
    if ( kStart > kernelSize - 1 )
      {
      kStart = kernelSize - 1;
      }
    count = iEnd - iStart + 1;
    outImage[i] = 0.0;
    for ( j = 0; j < count; ++j )
      {
      outImage[i] += image[j+iStart] * kernel[kStart-j];
      }
    }
}

//------------------------------------------------------------------------------
vtkImageSeparableConvolution* vtkImageSeparableConvolution::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageSeparableConvolution");
  if(ret)
    {
    return (vtkImageSeparableConvolution*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageSeparableConvolution;
}




//----------------------------------------------------------------------------
vtkImageSeparableConvolution::~vtkImageSeparableConvolution()
{
  SetXKernel ( NULL );
  SetYKernel ( NULL );
  SetZKernel ( NULL );
}

//----------------------------------------------------------------------------
vtkImageSeparableConvolution::vtkImageSeparableConvolution()
{
  XKernel = YKernel = ZKernel = NULL;
}

//----------------------------------------------------------------------------
// This extent of the components changes to real and imaginary values.
void vtkImageSeparableConvolution::ExecuteInformation(vtkImageData *input, vtkImageData *output)
{
  output->SetNumberOfScalarComponents(1);
  output->SetScalarType(VTK_FLOAT);
}

//----------------------------------------------------------------------------
// This method tells the superclass that the whole input array is needed
// to compute any output region.
void vtkImageSeparableConvolution::ComputeInputUpdateExtent(int inExt[6],
                                                            int outExt[6])
{
  int *wholeExtent, *extent;

  if ( ! this->GetInput())
    {
    vtkErrorMacro(<< "Input not set.");
    return;
    }

  // Assumes that the input update extent has been initialized to output ...
  extent = this->GetInput()->GetWholeExtent();
  memcpy(inExt, outExt, 6 * sizeof(int));
  inExt[this->Iteration*2] = extent[this->Iteration*2];
  inExt[this->Iteration*2 + 1] = extent[this->Iteration*2 + 1];
  this->GetInput()->GetWholeExtent ( inExt );
}

template <class T>
static void vtkImageSeparableConvolutionExecute ( vtkImageSeparableConvolution* self,
                                                  vtkImageData* inData,
                                                  vtkImageData* outData )
{
  T *inPtr0, *inPtr1, *inPtr2, *inPtrC;
  float *outPtr0, *outPtr1, *outPtr2, *outPtrC;
  int inInc0, inInc1, inInc2;
  int outInc0, outInc1, outInc2;
  int min0, max0, min1, max1, min2, max2, numberOfComponents;
  int idx0, idx1, idx2, idxC;
  int outExt[6];
  int i;
  unsigned long count = 0;
  unsigned long target;
  
  // outData->SetExtent(self->GetOutput()->GetWholeExtent());
  // outData->AllocateScalars();
  self->GetOutput()->GetWholeExtent ( outExt );


  // Reorder axes (the in and out extents are assumed to be the same)
  // (see intercept cache update)
  self->PermuteExtent(outExt, min0, max0, min1, max1, min2, max2);
  self->PermuteIncrements(inData->GetIncrements(), inInc0, inInc1, inInc2);
  self->PermuteIncrements(outData->GetIncrements(), outInc0, outInc1, outInc2);
  numberOfComponents = inData->GetNumberOfScalarComponents();
  
  target = (unsigned long)((max2-min2+1)*(max1-min1+1)/50.0);
  target++;

  vtkFloatArray* KernelArray = NULL;
  switch ( self->GetIteration() )
    {
    case 0:
      KernelArray = self->GetXKernel();
      break;
    case 1:
      KernelArray = self->GetYKernel();
      break;
    case 2:
      KernelArray = self->GetZKernel();
      break;
    }
  int kernelSize = 0;
  float* kernel = NULL;

  if ( KernelArray )
    {
    // Allocate the arrays
    kernelSize = KernelArray->GetNumberOfTuples();
    kernel = new float[kernelSize];
    // Copy the kernel
    for ( i = 0; i < kernelSize; i++ )
      {
      kernel[i] = KernelArray->GetValue ( i );
      }
    }

  int imageSize = (max0 - min0 + 1);
  float* image = new float[imageSize];
  float* outImage = new float[imageSize];
  float* kernelPtr = NULL;
  float* imagePtr = NULL;

  
  // loop over all the extra axes
  inPtr2 = (T *)inData->GetScalarPointerForExtent(outExt);
  outPtr2 = (float *)outData->GetScalarPointerForExtent(outExt);
  for (idx2 = min2; idx2 <= max2; ++idx2)
    {
    inPtr1 = inPtr2;
    outPtr1 = outPtr2;
    for (idx1 = min1; !self->AbortExecute && idx1 <= max1; ++idx1)
      {
      if (!(count%target))
        {
        self->UpdateProgress(count/(50.0*target));
        }
      count++;
      inPtrC = inPtr1;
      outPtrC = outPtr1;
      for (idxC = 0; idxC < numberOfComponents; ++idxC)
        {
        // execute forward pass
        inPtr0 = inPtrC;
        imagePtr = image;
        for (idx0 = min0; idx0 <= max0; ++idx0)
          {
          *imagePtr = (float)(*inPtr0);
          inPtr0 += inInc0;
          ++imagePtr;
          }
        // Call the method that performs the convolution
        if ( kernel )
          {
          ExecuteConvolve ( kernel, kernelSize, image, outImage, imageSize );
          imagePtr = outImage;
          }
        else
          {
          // If we don't have a kernel, just copy to the output
          imagePtr = image;
          }
  
        // Copy to output
        outPtr0 = outPtrC;
        for (idx0 = min0; idx0 <= max0; ++idx0)
          {
          *outPtr0 = (*imagePtr);
          outPtr0 += outInc0;
          ++imagePtr;
          }
        inPtrC += 1;
        outPtrC += 1;
        }
      inPtr1 += inInc1;
      outPtr1 += outInc1;
      }
    inPtr2 += inInc2;
    outPtr2 += outInc2;
    }
  delete [] image;
  delete [] outImage;
  if ( kernel )
    {
    delete [] kernel;
    }

}




//----------------------------------------------------------------------------
// This is writen as a 1D execute method, but is called several times.
void vtkImageSeparableConvolution::IterativeExecuteData(vtkImageData *inData, 
                                                        vtkImageData *outData)
{

  if ( XKernel )
    {
    // Check for a filter of odd length
    if ( 1 - ( XKernel->GetNumberOfTuples() % 2 ) )
      {
      vtkErrorMacro ( << "Execute:  XKernel must have odd length" );
      return;
      }
    }
  if ( YKernel )
    {
    // Check for a filter of odd length
    if ( 1 - ( YKernel->GetNumberOfTuples() % 2 ) )
      {
      vtkErrorMacro ( << "Execute:  YKernel must have odd length" );
      return;
      }
    }
  if ( ZKernel )
    {
    // Check for a filter of odd length
    if ( 1 - ( ZKernel->GetNumberOfTuples() % 2 ) )
      {
      vtkErrorMacro ( << "Execute:  ZKernel must have odd length" );
      return;
      }
    }
  
  // this filter expects that the output be floats.
  if (outData->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: Output must be be type float.");
    return;
    }
  // choose which templated function to call.
  switch (inData->GetScalarType())
    {
    vtkTemplateMacro3(vtkImageSeparableConvolutionExecute<VTK_TT>, this, inData, outData );
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }

}


void vtkImageSeparableConvolution::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageDecomposeFilter::PrintSelf(os,indent);

  if ( this->XKernel )
    {
    os << indent << "XKernel:\n";
    this->XKernel->PrintSelf ( os, indent.GetNextIndent() );
    }
  else
    {
    os << indent << "XKernel: (not defined)\n";
    }
  if ( this->YKernel )
    {
    os << indent << "YKernel:\n";
    this->YKernel->PrintSelf ( os, indent.GetNextIndent() );
    }
  else
    {
    os << indent << "YKernel: (not defined)\n";
    }
  if ( this->ZKernel )
    {
    os << indent << "ZKernel:\n";
    this->ZKernel->PrintSelf ( os, indent.GetNextIndent() );
    }
  else
    {
    os << indent << "ZKernel: (not defined)\n";
    }
}
