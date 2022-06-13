/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDilateErode3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageDilateErode3D.h"
#include "vtkImageData.h"
#include "vtkImageEllipsoidSource.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>

vtkStandardNewMacro(vtkImageDilateErode3D);

//------------------------------------------------------------------------------
// Construct an instance of vtkImageDilateErode3D filter.
// By default zero values are dilated.
vtkImageDilateErode3D::vtkImageDilateErode3D()
{
  this->HandleBoundaries = 1;

  // Initialize to 0 so that the SetKernelSize() below does its work.
  this->KernelSize[0] = 0;
  this->KernelSize[1] = 0;
  this->KernelSize[2] = 0;

  this->DilateValue = 0.0;
  this->ErodeValue = 255.0;

  this->Ellipse = vtkImageEllipsoidSource::New();
  // Setup the Ellipse to default size
  this->SetKernelSize(1, 1, 1);
}

//------------------------------------------------------------------------------
vtkImageDilateErode3D::~vtkImageDilateErode3D()
{
  if (this->Ellipse)
  {
    this->Ellipse->Delete();
    this->Ellipse = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkImageDilateErode3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DilateValue: " << this->DilateValue << "\n";
  os << indent << "ErodeValue: " << this->ErodeValue << "\n";
}

//------------------------------------------------------------------------------
// This method sets the size of the neighborhood.  It also sets the
// default middle of the neighborhood and computes the elliptical foot print.
void vtkImageDilateErode3D::SetKernelSize(int size0, int size1, int size2)
{
  int modified = 0;

  if (this->KernelSize[0] != size0)
  {
    modified = 1;
    this->KernelSize[0] = size0;
    this->KernelMiddle[0] = size0 / 2;
  }
  if (this->KernelSize[1] != size1)
  {
    modified = 1;
    this->KernelSize[1] = size1;
    this->KernelMiddle[1] = size1 / 2;
  }
  if (this->KernelSize[2] != size2)
  {
    modified = 1;
    this->KernelSize[2] = size2;
    this->KernelMiddle[2] = size2 / 2;
  }

  if (modified)
  {
    this->Modified();
    this->Ellipse->SetWholeExtent(
      0, this->KernelSize[0] - 1, 0, this->KernelSize[1] - 1, 0, this->KernelSize[2] - 1);
    this->Ellipse->SetCenter(static_cast<double>(this->KernelSize[0] - 1) * 0.5,
      static_cast<double>(this->KernelSize[1] - 1) * 0.5,
      static_cast<double>(this->KernelSize[2] - 1) * 0.5);
    this->Ellipse->SetRadius(static_cast<double>(this->KernelSize[0]) * 0.5,
      static_cast<double>(this->KernelSize[1]) * 0.5,
      static_cast<double>(this->KernelSize[2]) * 0.5);

    // make sure scalars have been allocated (needed if multithreaded is used)
    vtkInformation* ellipseOutInfo = this->Ellipse->GetExecutive()->GetOutputInformation(0);
    ellipseOutInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), 0,
      this->KernelSize[0] - 1, 0, this->KernelSize[1] - 1, 0, this->KernelSize[2] - 1);
    this->Ellipse->Update();
  }
}

//------------------------------------------------------------------------------
// This templated function executes the filter on any region,
// whether it needs boundary checking or not.
// If the filter needs to be faster, the function could be duplicated
// for strictly center (no boundary) processing.
template <class T>
void vtkImageDilateErode3DExecute(vtkImageDilateErode3D* self, vtkImageData* mask,
  vtkImageData* inData, T* inPtr, vtkImageData* outData, const int* outExt, T* outPtr, int id)
{
  // to compute the range
  unsigned long count = 0;

  // Get information to march through data
  vtkIdType inInc0, inInc1, inInc2;
  inData->GetIncrements(inInc0, inInc1, inInc2);
  const int* inExt = inData->GetExtent();
  int inImageMin0 = inExt[0];
  int inImageMax0 = inExt[1];
  int inImageMin1 = inExt[2];
  int inImageMax1 = inExt[3];
  int inImageMin2 = inExt[4];
  int inImageMax2 = inExt[5];
  vtkIdType outInc0, outInc1, outInc2;
  outData->GetIncrements(outInc0, outInc1, outInc2);
  int outMin0 = outExt[0];
  int outMax0 = outExt[1];
  int outMin1 = outExt[2];
  int outMax1 = outExt[3];
  int outMin2 = outExt[4];
  int outMax2 = outExt[5];
  int numComps = outData->GetNumberOfScalarComponents();

  // Get ivars of this object (easier than making friends)
  T erodeValue = static_cast<T>(self->GetErodeValue());
  T dilateValue = static_cast<T>(self->GetDilateValue());
  const int* kernelSize = self->GetKernelSize();
  const int* kernelMiddle = self->GetKernelMiddle();
  int hoodMin0 = -kernelMiddle[0];
  int hoodMin1 = -kernelMiddle[1];
  int hoodMin2 = -kernelMiddle[2];
  int hoodMax0 = hoodMin0 + kernelSize[0] - 1;
  int hoodMax1 = hoodMin1 + kernelSize[1] - 1;
  int hoodMax2 = hoodMin2 + kernelSize[2] - 1;

  // Setup mask info
  const unsigned char* maskPtr = static_cast<unsigned char*>(mask->GetScalarPointer());
  vtkIdType maskInc0, maskInc1, maskInc2;
  mask->GetIncrements(maskInc0, maskInc1, maskInc2);

  // in and out should be marching through corresponding pixels.
  inPtr = static_cast<T*>(inData->GetScalarPointer(outMin0, outMin1, outMin2));

  unsigned long target =
    static_cast<unsigned long>(numComps * (outMax2 - outMin2 + 1) * (outMax1 - outMin1 + 1) / 50.0);
  target++;

  // loop through components
  for (int outIdxC = 0; outIdxC < numComps; ++outIdxC)
  {
    // loop through pixels of output
    T* outPtr2 = outPtr;
    const T* inPtr2 = inPtr;
    for (int outIdx2 = outMin2; outIdx2 <= outMax2; ++outIdx2)
    {
      T* outPtr1 = outPtr2;
      const T* inPtr1 = inPtr2;
      for (int outIdx1 = outMin1; !self->AbortExecute && outIdx1 <= outMax1; ++outIdx1)
      {
        if (!id)
        {
          if (!(count % target))
          {
            self->UpdateProgress(count / (50.0 * target));
          }
          count++;
        }

        T* outPtr0 = outPtr1;
        const T* inPtr0 = inPtr1;
        for (int outIdx0 = outMin0; outIdx0 <= outMax0; ++outIdx0)
        {
          // Default behavior (copy input pixel)
          *outPtr0 = *inPtr0;
          if (*inPtr0 == erodeValue)
          {
            // loop through neighborhood pixels
            // as sort of a hack to handle boundaries,
            // input pointer will be marching through data that does not exist.
            int hoodMinClamp2 = std::max(hoodMin2, inImageMin2 - outIdx2);
            int hoodMaxClamp2 = std::min(hoodMax2, inImageMax2 - outIdx2);
            const T* hoodPtr2 = inPtr0 - kernelMiddle[0] * inInc0 - kernelMiddle[1] * inInc1 -
              kernelMiddle[2] * inInc2 + (hoodMinClamp2 - hoodMin2) * inInc2;
            const unsigned char* maskPtr2 = maskPtr + (hoodMinClamp2 - hoodMin2) * maskInc2;
            for (int hoodIdx2 = hoodMinClamp2; hoodIdx2 <= hoodMaxClamp2; ++hoodIdx2)
            {
              int hoodMinClamp1 = std::max(hoodMin1, inImageMin1 - outIdx1);
              int hoodMaxClamp1 = std::min(hoodMax1, inImageMax1 - outIdx1);
              const T* hoodPtr1 = hoodPtr2 + (hoodMinClamp1 - hoodMin1) * inInc1;
              const unsigned char* maskPtr1 = maskPtr2 + (hoodMinClamp1 - hoodMin1) * maskInc1;
              for (int hoodIdx1 = hoodMinClamp1; hoodIdx1 <= hoodMaxClamp1; ++hoodIdx1)
              {
                int hoodMinClamp0 = std::max(hoodMin0, inImageMin0 - outIdx0);
                int hoodMaxClamp0 = std::min(hoodMax0, inImageMax0 - outIdx0);
                const T* hoodPtr0 = hoodPtr1 + (hoodMinClamp0 - hoodMin0) * inInc0;
                const unsigned char* maskPtr0 = maskPtr1 + (hoodMinClamp0 - hoodMin0) * maskInc0;
                for (int hoodIdx0 = hoodMinClamp0; hoodIdx0 <= hoodMaxClamp0; ++hoodIdx0)
                {
                  if (*maskPtr0 != 0)
                  {
                    if (*hoodPtr0 == dilateValue)
                    {
                      *outPtr0 = dilateValue;
                    }
                  }

                  hoodPtr0 += inInc0;
                  maskPtr0 += maskInc0;
                }
                hoodPtr1 += inInc1;
                maskPtr1 += maskInc1;
              }
              hoodPtr2 += inInc2;
              maskPtr2 += maskInc2;
            }
          }

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

//------------------------------------------------------------------------------
// This method contains the first switch statement that calls the correct
// templated function for the input and output Data types.
// It handles image boundaries, so the image does not shrink.
void vtkImageDilateErode3D::ThreadedRequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector),
  vtkImageData*** inData, vtkImageData** outData, int outExt[6], int id)
{
  int inExt[6], wholeExt[6];
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExt);
  this->InternalRequestUpdateExtent(inExt, outExt, wholeExt);
  void* inPtr = inData[0][0]->GetScalarPointerForExtent(inExt);
  void* outPtr = outData[0]->GetScalarPointerForExtent(outExt);
  vtkImageData* mask;

  // Error checking on mask
  mask = this->Ellipse->GetOutput();
  if (mask->GetScalarType() != VTK_UNSIGNED_CHAR)
  {
    vtkErrorMacro(<< "Execute: mask has wrong scalar type");
    return;
  }

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
    vtkTemplateMacro(vtkImageDilateErode3DExecute(this, mask, inData[0][0],
      static_cast<VTK_TT*>(inPtr), outData[0], outExt, static_cast<VTK_TT*>(outPtr), id));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
  }
}

//------------------------------------------------------------------------------
int vtkImageDilateErode3D::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->Ellipse->Update();
  return this->Superclass::RequestData(request, inputVector, outputVector);
}
