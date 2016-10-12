/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageWrapPad.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageWrapPad.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageWrapPad);

//----------------------------------------------------------------------------
// Just clip the request.
void vtkImageWrapPad::ComputeInputUpdateExtent (int inExt[6], int outExt[6],
                                                int wholeExtent[6])
{
  int idx;
  int min, max, width, imageMin, imageMax, imageWidth;

  // Clip
  for (idx = 0; idx < 3; ++idx)
  {
    min = outExt[idx * 2];
    max = outExt[idx * 2 + 1];
    imageMin = wholeExtent[idx * 2];
    imageMax = wholeExtent[idx * 2 + 1];
    if (min > max || imageMin > imageMax)
    { // Empty output request.
      inExt[0] = inExt[2] = inExt[4] = 0;
      inExt[1] = inExt[3] = inExt[5] = -1;
      return;
    }
    width = max - min + 1;
    imageWidth = imageMax - imageMin + 1;

    // convert min max to image extent range.
    min = ((min - imageMin) % imageWidth);
    if (min < 0)
    { // Mod does not handle negative numbers as I think it should.
      min += imageWidth;
    }
    min += imageMin;
    max = min + width - 1;
    // if request region wraps, we need the whole input
    // (unless we make multiple requests! Write Update instead??)
    if (max > imageMax)
    {
      max = imageMax;
      min = imageMin;
    }

    inExt[idx * 2] = min;
    inExt[idx * 2 + 1] = max;
  }
}




//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void vtkImageWrapPadExecute(vtkImageWrapPad *self,
                            vtkImageData *inData, T *vtkNotUsed(inPtr),
                            vtkImageData *outData, T *outPtr,
                            int outExt[6], int id,
                            int wholeExt[6])
{
  int min0, max0;
  int imageMin0, imageMax0, imageMin1, imageMax1,
    imageMin2, imageMax2;
  int outIdx0, outIdx1, outIdx2;
  int start0, start1, start2;
  int inIdx0, inIdx1, inIdx2;
  vtkIdType inInc0, inInc1, inInc2;
  vtkIdType outIncX, outIncY, outIncZ;
  T *inPtr0, *inPtr1, *inPtr2;
  unsigned long count = 0;
  unsigned long target;
  int inMaxC, idxC, maxC;

  // Get information to march through data
  inData->GetIncrements(inInc0, inInc1, inInc2);
  imageMin0 = wholeExt[0];
  imageMax0 = wholeExt[1];
  imageMin1 = wholeExt[2];
  imageMax1 = wholeExt[3];
  imageMin2 = wholeExt[4];
  imageMax2 = wholeExt[5];

  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // initialize pointers to coresponding pixels.
  start0 = ((outExt[0] - imageMin0) % (imageMax0-imageMin0+1)) + imageMin0;
  if (start0 < 0)
  {
    start0 += (imageMax0-imageMin0+1);
  }
  start1 = ((outExt[2] - imageMin1) % (imageMax1-imageMin1+1)) + imageMin1;
  if (start1 < 0)
  {
    start1 += (imageMax1-imageMin1+1);
  }
  start2 = ((outExt[4] - imageMin2) % (imageMax2-imageMin2+1)) + imageMin2;
  if (start2 < 0)
  {
    start2 += (imageMax2-imageMin2+1);
  }
  inPtr2 = static_cast<T *>(inData->GetScalarPointer(start0, start1, start2));

  min0 = outExt[0];
  max0 = outExt[1];
  inMaxC = inData->GetNumberOfScalarComponents();
  maxC = outData->GetNumberOfScalarComponents();
  target = static_cast<unsigned long>((outExt[5]-outExt[4]+1)*
                                      (outExt[3]-outExt[2]+1)/50.0);
  target++;

  inIdx2 = start2;
  for (outIdx2 = outExt[4]; outIdx2 <= outExt[5]; ++outIdx2, ++inIdx2)
  {
    if (inIdx2 > imageMax2)
    { // we need to wrap(rewind) the input on this axis
      inIdx2 = imageMin2;
      inPtr2 -= (imageMax2-imageMin2+1)*inInc2;
    }
    inPtr1 = inPtr2;
    inIdx1 = start1;
    for (outIdx1 = outExt[2];
         !self->AbortExecute && outIdx1 <= outExt[3]; ++outIdx1, ++inIdx1)
    {
      if (!id)
      {
        if (!(count%target))
        {
          self->UpdateProgress(count/(50.0*target));
        }
        count++;
      }
      if (inIdx1 > imageMax1)
      { // we need to wrap(rewind) the input on this axis
        inIdx1 = imageMin1;
        inPtr1 -= (imageMax1-imageMin1+1)*inInc1;
      }
      inPtr0 = inPtr1;
      inIdx0 = start0;
      // if components are same much faster
      if ((maxC == inMaxC) && (maxC == 1))
      {
        for (outIdx0 = min0; outIdx0 <= max0; ++outIdx0, ++inIdx0)
        {
          if (inIdx0 > imageMax0)
          { // we need to wrap(rewind) the input on this axis
            inIdx0 = imageMin0;
            inPtr0 -= (imageMax0-imageMin0+1)*inInc0;
          }
          // Copy Pixel
          *outPtr = *inPtr0;
          outPtr++; inPtr0++;
        }
      }
      else
      {
        for (outIdx0 = min0; outIdx0 <= max0; ++outIdx0, ++inIdx0)
        {
          if (inIdx0 > imageMax0)
          { // we need to wrap(rewind) the input on this axis
            inIdx0 = imageMin0;
            inPtr0 -= (imageMax0-imageMin0+1)*inInc0;
          }
          for (idxC = 0; idxC < maxC; idxC++)
          {
            // Copy Pixel
            *outPtr = inPtr0[idxC%inMaxC];
            outPtr++;
          }
          inPtr0 += inInc0;
        }
      }
      outPtr += outIncY;
      inPtr1 += inInc1;
    }
    outPtr += outIncZ;
    inPtr2 += inInc2;
  }
}



//----------------------------------------------------------------------------
// This method is passed a input and output data, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageWrapPad::ThreadedRequestData (
  vtkInformation * vtkNotUsed( request ),
  vtkInformationVector** inputVector,
  vtkInformationVector * vtkNotUsed( outputVector ),
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
  // return if nothing to do
  if (outExt[1] < outExt[0] ||
      outExt[3] < outExt[2] ||
      outExt[5] < outExt[4])
  {
    return;
  }

  int inExt[6];

  // get the whole extent
  int wExt[6];
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),wExt);
  this->ComputeInputUpdateExtent(inExt,outExt,wExt);
  void *inPtr = inData[0][0]->GetScalarPointerForExtent(inExt);
  void *outPtr = outData[0]->GetScalarPointerForExtent(outExt);

  vtkDebugMacro(<< "Execute: inData = " << inData[0][0]
                << ", outData = " << outData[0]);

  // this filter expects that input is the same type as output.
  if (inData[0][0]->GetScalarType() != outData[0]->GetScalarType())
  {
    vtkErrorMacro(<< "Execute: input ScalarType, "
                  << inData[0][0]->GetScalarType()
                  << ", must match out ScalarType "
                  << outData[0]->GetScalarType());
    return;
  }

  switch (inData[0][0]->GetScalarType())
  {
    vtkTemplateMacro(
      vtkImageWrapPadExecute(
        this, inData[0][0],
        static_cast<VTK_TT *>(inPtr), outData[0],
        static_cast<VTK_TT *>(outPtr), outExt, id,
        inputVector[0]->GetInformationObject(0)->Get(
          vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT())));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
  }
}
