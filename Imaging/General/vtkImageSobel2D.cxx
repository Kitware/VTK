/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSobel2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageSobel2D.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

vtkStandardNewMacro(vtkImageSobel2D);

//----------------------------------------------------------------------------
// Construct an instance of vtkImageSobel2D fitler.
vtkImageSobel2D::vtkImageSobel2D()
{
  this->KernelSize[0] = 3;
  this->KernelSize[1] = 3;
  this->KernelSize[2] = 1;
  this->KernelMiddle[0] = 1;
  this->KernelMiddle[1] = 1;
  this->KernelMiddle[2] = 0;
  this->HandleBoundaries = 1;
}

//----------------------------------------------------------------------------
void vtkImageSobel2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkImageSobel2D::RequestInformation (vtkInformation *request,
                                         vtkInformationVector **inputVector,
                                         vtkInformationVector *outputVector)
{
  int retval =
    this->Superclass::RequestInformation(request, inputVector, outputVector);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_DOUBLE, 2);
  return retval;
}

//----------------------------------------------------------------------------
// This execute method handles boundaries.
// it handles boundaries. Pixels are just replicated to get values
// out of extent.
template <class T>
void vtkImageSobel2DExecute(vtkImageSobel2D *self,
                            vtkImageData *inData, T *inPtr,
                            vtkImageData *outData, int *outExt,
                            double *outPtr, int id,
                            vtkInformation *inInfo)
{
  double r0, r1, *r;
  // For looping though output (and input) pixels.
  int min0, max0, min1, max1, min2, max2;
  int outIdx0, outIdx1, outIdx2;
  vtkIdType outInc0, outInc1, outInc2;
  double *outPtr0, *outPtr1, *outPtr2, *outPtrV;
  vtkIdType inInc0, inInc1, inInc2;
  T *inPtr0, *inPtr1, *inPtr2;
  // For sobel function convolution (Left Right incs for each axis)
  vtkIdType inInc0L, inInc0R, inInc1L, inInc1R;
  T *inPtrL, *inPtrR;
  double sum;
  // Boundary of input image
  int inWholeMin0,inWholeMax0;
  int inWholeMin1,inWholeMax1;
  int inWholeExt[6];
  unsigned long count = 0;
  unsigned long target;

  // Get boundary information
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inWholeExt);
  inWholeMin0 = inWholeExt[0];
  inWholeMax0 = inWholeExt[1];
  inWholeMin1 = inWholeExt[2];
  inWholeMax1 = inWholeExt[3];

  // Get information to march through data
  inData->GetIncrements(inInc0, inInc1, inInc2);
  outData->GetIncrements(outInc0, outInc1, outInc2);
  min0 = outExt[0];   max0 = outExt[1];
  min1 = outExt[2];   max1 = outExt[3];
  min2 = outExt[4];   max2 = outExt[5];

  // We want the input pixel to correspond to output
  inPtr = static_cast<T *>(inData->GetScalarPointer(min0,min1,min2));

  // The data spacing is important for computing the gradient.
  // Scale so it has the same range as gradient.
  r = inData->GetSpacing();
  r0 = 0.125 / r[0];
  r1 = 0.125 / r[1];
  // ignore r2

  target = static_cast<unsigned long>((max2-min2+1)*(max1-min1+1)/50.0);
  target++;

  // loop through pixels of output
  outPtr2 = outPtr;
  inPtr2 = inPtr;
  for (outIdx2 = min2; outIdx2 <= max2; ++outIdx2)
    {
    outPtr1 = outPtr2;
    inPtr1 = inPtr2;
    for (outIdx1 = min1; !self->AbortExecute && outIdx1 <= max1; ++outIdx1)
      {
      if (!id)
        {
        if (!(count%target))
          {
          self->UpdateProgress(count/(50.0*target));
          }
        count++;
        }
      inInc1L = (outIdx1 == inWholeMin1) ? 0 : -inInc1;
      inInc1R = (outIdx1 == inWholeMax1) ? 0 : inInc1;

      outPtr0 = outPtr1;
      inPtr0 = inPtr1;
      for (outIdx0 = min0; outIdx0 <= max0; ++outIdx0)
        {
        inInc0L = (outIdx0 == inWholeMin0) ? 0 : -inInc0;
        inInc0R = (outIdx0 == inWholeMax0) ? 0 : inInc0;

        // compute vector.
        outPtrV = outPtr0;
        // 0 direction
        inPtrL = inPtr0 + inInc0L;
        inPtrR = inPtr0 + inInc0R;
        sum = 2.0 * (*inPtrR - *inPtrL);
        sum += static_cast<double>(inPtrR[inInc1L] + inPtrR[inInc1R]);
        sum -= static_cast<double>(inPtrL[inInc1L] + inPtrL[inInc1R]);

        *outPtrV = sum * r0;
        ++outPtrV;
        // 1 direction
        inPtrL = inPtr0 + inInc1L;
        inPtrR = inPtr0 + inInc1R;
        sum = 2.0 * (*inPtrR - *inPtrL);
        sum += static_cast<double>(inPtrR[inInc0L] + inPtrR[inInc0R]);
        sum -= static_cast<double>(inPtrL[inInc0L] + inPtrL[inInc0R]);
        *outPtrV = static_cast<double>(sum * r1);

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

//----------------------------------------------------------------------------
// This method contains a switch statement that calls the correct
// templated function for the input region type.  The output region
// must be of type double.  This method does handle boundary conditions.
// The third axis is the component axis for the output.
void vtkImageSobel2D::ThreadedRequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector),
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
  void *inPtr, *outPtr;
  int inExt[6], wholeExt[6];

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExt);
  this->InternalRequestUpdateExtent(inExt, outExt, wholeExt);

  inPtr = inData[0][0]->GetScalarPointerForExtent(inExt);
  outPtr = outData[0]->GetScalarPointerForExtent(outExt);

  // this filter expects that output is type double.
  if (outData[0]->GetScalarType() != VTK_DOUBLE)
    {
    vtkErrorMacro(<< "Execute: output ScalarType, "
                  << vtkImageScalarTypeNameMacro(outData[0]->GetScalarType())
                  << ", must be double");
    return;
    }

  // this filter cannot handle multi component input.
  if (inData[0][0]->GetNumberOfScalarComponents() != 1)
    {
    vtkWarningMacro("Expecting input with only one compenent.\n");
    }

  switch (inData[0][0]->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageSobel2DExecute(this, inData[0][0],
                             static_cast<VTK_TT *>(inPtr), outData[0], outExt,
                             static_cast<double *>(outPtr),id, inInfo));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}
