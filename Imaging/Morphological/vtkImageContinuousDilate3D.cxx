/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageContinuousDilate3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageContinuousDilate3D.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkImageEllipsoidSource.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageContinuousDilate3D);

//------------------------------------------------------------------------------
// Construct an instance of vtkImageContinuousDilate3D filter.
// By default zero values are dilated.
vtkImageContinuousDilate3D::vtkImageContinuousDilate3D()
{
  this->HandleBoundaries = 1;
  this->KernelSize[0] = 0;
  this->KernelSize[1] = 0;
  this->KernelSize[2] = 0;

  this->Ellipse = vtkImageEllipsoidSource::New();
  // Setup the Ellipse to default size
  this->SetKernelSize(1, 1, 1);
}

//------------------------------------------------------------------------------
vtkImageContinuousDilate3D::~vtkImageContinuousDilate3D()
{
  if (this->Ellipse)
  {
    this->Ellipse->Delete();
    this->Ellipse = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkImageContinuousDilate3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
// This method sets the size of the neighborhood.  It also sets the
// default middle of the neighborhood and computes the elliptical foot print.
void vtkImageContinuousDilate3D::SetKernelSize(int size0, int size1, int size2)
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
    this->Ellipse->SetCenter(static_cast<float>(this->KernelSize[0] - 1) * 0.5,
      static_cast<float>(this->KernelSize[1] - 1) * 0.5,
      static_cast<float>(this->KernelSize[2] - 1) * 0.5);
    this->Ellipse->SetRadius(static_cast<float>(this->KernelSize[0]) * 0.5,
      static_cast<float>(this->KernelSize[1]) * 0.5, static_cast<float>(this->KernelSize[2]) * 0.5);

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
void vtkImageContinuousDilate3DExecute(vtkImageContinuousDilate3D* self, vtkImageData* mask,
  vtkImageData* inData, T* inPtr, vtkImageData* outData, const int* outExt, T* outPtr, int id,
  vtkDataArray* inArray, vtkInformation* inInfo)
{
  // to compute the range
  unsigned long count = 0;

  const int* inExt = inData->GetExtent();

  // Get information to march through data
  vtkIdType inInc0, inInc1, inInc2;
  inData->GetIncrements(inInc0, inInc1, inInc2);
  int inImageExt[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inImageExt);
  int inImageMin0 = inImageExt[0];
  int inImageMax0 = inImageExt[1];
  int inImageMin1 = inImageExt[2];
  int inImageMax1 = inImageExt[3];
  int inImageMin2 = inImageExt[4];
  int inImageMax2 = inImageExt[5];
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
  inPtr = static_cast<T*>(inArray->GetVoidPointer(
    (outMin0 - inExt[0]) * inInc0 + (outMin1 - inExt[2]) * inInc1 + (outMin2 - inExt[4]) * inInc2));

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
          // Find max
          T pixelMax = *inPtr0;
          // loop through neighborhood pixels
          // as sort of a hack to handle boundaries,
          // input pointer will be marching through data that does not exist.
          const T* hoodPtr2 =
            inPtr0 - kernelMiddle[0] * inInc0 - kernelMiddle[1] * inInc1 - kernelMiddle[2] * inInc2;
          const unsigned char* maskPtr2 = maskPtr;
          for (int hoodIdx2 = hoodMin2; hoodIdx2 <= hoodMax2; ++hoodIdx2)
          {
            const T* hoodPtr1 = hoodPtr2;
            const unsigned char* maskPtr1 = maskPtr2;
            for (int hoodIdx1 = hoodMin1; hoodIdx1 <= hoodMax1; ++hoodIdx1)
            {
              const T* hoodPtr0 = hoodPtr1;
              const unsigned char* maskPtr0 = maskPtr1;
              for (int hoodIdx0 = hoodMin0; hoodIdx0 <= hoodMax0; ++hoodIdx0)
              {
                // A quick but rather expensive way to handle boundaries
                if (outIdx0 + hoodIdx0 >= inImageMin0 && outIdx0 + hoodIdx0 <= inImageMax0 &&
                  outIdx1 + hoodIdx1 >= inImageMin1 && outIdx1 + hoodIdx1 <= inImageMax1 &&
                  outIdx2 + hoodIdx2 >= inImageMin2 && outIdx2 + hoodIdx2 <= inImageMax2)
                {
                  if (*maskPtr0 != 0)
                  {
                    if (*hoodPtr0 > pixelMax)
                    {
                      pixelMax = *hoodPtr0;
                    }
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
          *outPtr0 = pixelMax;

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
void vtkImageContinuousDilate3D::ThreadedRequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector),
  vtkImageData*** inData, vtkImageData** outData, int outExt[6], int id)
{
  // return if nothing to do
  if (outExt[1] < outExt[0] || outExt[3] < outExt[2] || outExt[5] < outExt[4])
  {
    return;
  }

  int inExt[6], wholeExt[6];
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExt);
  this->InternalRequestUpdateExtent(inExt, outExt, wholeExt);
  void* inPtr;
  void* outPtr = outData[0]->GetScalarPointerForExtent(outExt);
  vtkImageData* mask;

  vtkDataArray* inArray = this->GetInputArrayToProcess(0, inputVector);

  // The inPtr is reset anyway, so just get the id 0 pointer.
  inPtr = inArray->GetVoidPointer(0);

  // Error checking on mask
  mask = this->Ellipse->GetOutput();
  if (mask->GetScalarType() != VTK_UNSIGNED_CHAR)
  {
    vtkErrorMacro(<< "Execute: mask has wrong scalar type");
    return;
  }

  // this filter expects the output type to be same as input
  if (outData[0]->GetScalarType() != inArray->GetDataType())
  {
    vtkErrorMacro(<< "Execute: output ScalarType, "
                  << vtkImageScalarTypeNameMacro(outData[0]->GetScalarType())
                  << " must match input array data type");
    return;
  }

  switch (inArray->GetDataType())
  {
    vtkTemplateMacro(
      vtkImageContinuousDilate3DExecute(this, mask, inData[0][0], static_cast<VTK_TT*>(inPtr),
        outData[0], outExt, static_cast<VTK_TT*>(outPtr), id, inArray, inInfo));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
  }
}

//------------------------------------------------------------------------------
int vtkImageContinuousDilate3D::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->Ellipse->Update();
  return this->Superclass::RequestData(request, inputVector, outputVector);
}
