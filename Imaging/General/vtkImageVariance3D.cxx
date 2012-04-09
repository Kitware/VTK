/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageVariance3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageVariance3D.h"

#include "vtkImageEllipsoidSource.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageVariance3D);

//----------------------------------------------------------------------------
vtkImageVariance3D::vtkImageVariance3D()
{
  this->HandleBoundaries = 1;
  this->KernelSize[0] = 1;
  this->KernelSize[1] = 1;
  this->KernelSize[2] = 1;

  this->Ellipse = vtkImageEllipsoidSource::New();
  // Setup the Ellipse to default size
  this->SetKernelSize(1, 1, 1);
}

//----------------------------------------------------------------------------
vtkImageVariance3D::~vtkImageVariance3D()
{
  if (this->Ellipse)
    {
    this->Ellipse->Delete();
    this->Ellipse = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkImageVariance3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
// This method sets the size of the neighborhood.  It also sets the
// default middle of the neighborhood and computes the Elliptical foot print.
void vtkImageVariance3D::SetKernelSize(int size0, int size1, int size2)
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
    this->Ellipse->SetWholeExtent(0, this->KernelSize[0]-1,
                                  0, this->KernelSize[1]-1,
                                  0, this->KernelSize[2]-1);
    this->Ellipse->SetCenter(static_cast<float>(this->KernelSize[0]-1)*0.5,
                             static_cast<float>(this->KernelSize[1]-1)*0.5,
                             static_cast<float>(this->KernelSize[2]-1)*0.5);
    this->Ellipse->SetRadius(static_cast<float>(this->KernelSize[0])*0.5,
                             static_cast<float>(this->KernelSize[1])*0.5,
                             static_cast<float>(this->KernelSize[2])*0.5);

    // make sure scalars have been allocated (needed if multithreaded is used)
    vtkInformation *ellipseOutInfo =
      this->Ellipse->GetExecutive()->GetOutputInformation(0);
    ellipseOutInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                        0, this->KernelSize[0]-1,
                        0, this->KernelSize[1]-1,
                        0, this->KernelSize[2]-1);
    this->Ellipse->Update();
    }
}

//----------------------------------------------------------------------------
// Output is always float
int vtkImageVariance3D::RequestInformation (vtkInformation *request,
                                            vtkInformationVector **inputVector,
                                            vtkInformationVector *outputVector)
{
  int retval =
    this->Superclass::RequestInformation(request, inputVector, outputVector);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_FLOAT, -1);
  return retval;
}

//----------------------------------------------------------------------------
// This templated function executes the filter on any region,
// whether it needs boundary checking or not.
// If the filter needs to be faster, the function could be duplicated
// for strictly center (no boundary ) processing.
template <class T>
void vtkImageVariance3DExecute(vtkImageVariance3D *self,
                               vtkImageData *mask,
                               vtkImageData *inData, T *inPtr,
                               vtkImageData *outData, int *outExt,
                               float *outPtr, int id,
                               vtkInformation *inInfo)
{
  int *kernelMiddle, *kernelSize;
  // For looping though output (and input) pixels.
  int outMin0, outMax0, outMin1, outMax1, outMin2, outMax2;
  int outIdx0, outIdx1, outIdx2;
  vtkIdType inInc0, inInc1, inInc2;
  vtkIdType outInc0, outInc1, outInc2;
  T *inPtr0, *inPtr1, *inPtr2;
  float *outPtr0, *outPtr1, *outPtr2;
  int numComps, outIdxC;
  // For looping through hood pixels
  int hoodMin0, hoodMax0, hoodMin1, hoodMax1, hoodMin2, hoodMax2;
  int hoodIdx0, hoodIdx1, hoodIdx2;
  T *hoodPtr0, *hoodPtr1, *hoodPtr2;
  // For looping through the mask.
  unsigned char *maskPtr, *maskPtr0, *maskPtr1, *maskPtr2;
  vtkIdType maskInc0, maskInc1, maskInc2;
  // The extent of the whole input image
  int inImageMin0, inImageMin1, inImageMin2;
  int inImageMax0, inImageMax1, inImageMax2;
  int inImageExt[6];
  // To compute the variance
  float diff, sum;
  int icount;
  unsigned long count = 0;
  unsigned long target;

  // Get information to march through data
  inData->GetIncrements(inInc0, inInc1, inInc2);
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inImageExt);
  inImageMin0 = inImageExt[0];
  inImageMax0 = inImageExt[1];
  inImageMin1 = inImageExt[2];
  inImageMax1 = inImageExt[3];
  inImageMin2 = inImageExt[4];
  inImageMax2 = inImageExt[5];
  outData->GetIncrements(outInc0, outInc1, outInc2);
  outMin0 = outExt[0];   outMax0 = outExt[1];
  outMin1 = outExt[2];   outMax1 = outExt[3];
  outMin2 = outExt[4];   outMax2 = outExt[5];
  numComps = outData->GetNumberOfScalarComponents();

  // Get ivars of this object (easier than making friends)
  kernelSize = self->GetKernelSize();;
  kernelMiddle = self->GetKernelMiddle();
  hoodMin0 = - kernelMiddle[0];
  hoodMin1 = - kernelMiddle[1];
  hoodMin2 = - kernelMiddle[2];
  hoodMax0 = hoodMin0 + kernelSize[0] - 1;
  hoodMax1 = hoodMin1 + kernelSize[1] - 1;
  hoodMax2 = hoodMin2 + kernelSize[2] - 1;

  // Setup mask info
  maskPtr = static_cast<unsigned char *>(mask->GetScalarPointer());
  mask->GetIncrements(maskInc0, maskInc1, maskInc2);

  // in and out should be marching through corresponding pixels.
  inPtr = static_cast<T *>(
    inData->GetScalarPointer(outMin0, outMin1, outMin2));

  target = static_cast<unsigned long>(numComps*(outMax2-outMin2+1)*
                                      (outMax1-outMin1+1)/50.0);
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
      for (outIdx1 = outMin1; !self->AbortExecute && outIdx1 <= outMax1;
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

          // Find variance
          sum = 0.0;
          icount = 0;
          // loop through neighborhood pixels
          // as sort of a hack to handle boundaries,
          // input pointer will be marching through data that does not exist.
          hoodPtr2 = inPtr0 - kernelMiddle[0] * inInc0
            - kernelMiddle[1] * inInc1 - kernelMiddle[2] * inInc2;
          maskPtr2 = maskPtr;
          for (hoodIdx2 = hoodMin2; hoodIdx2 <= hoodMax2; ++hoodIdx2)
            {
            hoodPtr1 = hoodPtr2;
            maskPtr1 = maskPtr2;
            for (hoodIdx1 = hoodMin1; hoodIdx1 <= hoodMax1; ++hoodIdx1)
              {
              hoodPtr0 = hoodPtr1;
              maskPtr0 = maskPtr1;
              for (hoodIdx0 = hoodMin0; hoodIdx0 <= hoodMax0; ++hoodIdx0)
                {
                // A quick but rather expensive way to handle boundaries
                if ( outIdx0 + hoodIdx0 >= inImageMin0 &&
                     outIdx0 + hoodIdx0 <= inImageMax0 &&
                     outIdx1 + hoodIdx1 >= inImageMin1 &&
                     outIdx1 + hoodIdx1 <= inImageMax1 &&
                     outIdx2 + hoodIdx2 >= inImageMin2 &&
                     outIdx2 + hoodIdx2 <= inImageMax2)
                  {
                  if (*maskPtr0)
                    {
                    diff = static_cast<float>(*hoodPtr0)
                      - static_cast<float>(*inPtr0);
                    sum += diff * diff;
                    ++icount;
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
          *outPtr0 = sum / static_cast<float>(icount);

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
void vtkImageVariance3D::ThreadedRequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector),
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
  int inExt[6], wholeExt[6];
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExt);
  this->InternalRequestUpdateExtent(inExt,outExt,wholeExt);
  void *inPtr = inData[0][0]->GetScalarPointerForExtent(inExt);
  void *outPtr = outData[0]->GetScalarPointerForExtent(outExt);
  vtkImageData *mask;

  // Error checking on mask
  mask = this->Ellipse->GetOutput();
  if (mask->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
    vtkErrorMacro(<< "Execute: mask has wrong scalar type");
    return;
    }

  // this filter expects the output to be float
  if (outData[0]->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: output ScalarType, "
      << vtkImageScalarTypeNameMacro(outData[0]->GetScalarType())
      << " must be float");
    return;
    }

  switch (inData[0][0]->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageVariance3DExecute(this, mask, inData[0][0],
                                static_cast<VTK_TT *>(inPtr), outData[0],
                                outExt,
                                static_cast<float *>(outPtr),id, inInfo));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}

//----------------------------------------------------------------------------
int vtkImageVariance3D::RequestData(vtkInformation *request,
                                     vtkInformationVector **inputVector,
                                     vtkInformationVector *outputVector)
{
  this->Ellipse->Update();
  return this->Superclass::RequestData(request, inputVector, outputVector);
}
