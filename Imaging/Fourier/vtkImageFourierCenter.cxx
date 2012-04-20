/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFourierCenter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageFourierCenter.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

vtkStandardNewMacro(vtkImageFourierCenter);

//----------------------------------------------------------------------------
// Construct an instance of vtkImageFourierCenter fitler.
vtkImageFourierCenter::vtkImageFourierCenter()
{
}


//----------------------------------------------------------------------------
// This method tells the superclass which input extent is needed.
// This gets the whole input (even though it may not be needed).
int vtkImageFourierCenter::IterativeRequestUpdateExtent(
  vtkInformation* input, vtkInformation* output)
{
  int *outExt = output->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  int *wExt = input->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  int inExt[6];
  memcpy(inExt, outExt, 6 * sizeof(int));
  inExt[this->Iteration*2] = wExt[this->Iteration*2];
  inExt[this->Iteration*2 + 1] = wExt[this->Iteration*2 + 1];
  input->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),inExt,6);

  return 1;
}

//----------------------------------------------------------------------------
// This method is passed input and output regions, and executes the fft
// algorithm to fill the output from the input.
void vtkImageFourierCenter::ThreadedRequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector* outputVector,
  vtkImageData ***inDataVec,
  vtkImageData **outDataVec,
  int outExt[6],
  int threadId)
{
  vtkImageData* inData = inDataVec[0][0];
  vtkImageData* outData = outDataVec[0];
  double *inPtr0, *inPtr1, *inPtr2;
  double *outPtr0, *outPtr1, *outPtr2;
  vtkIdType inInc0, inInc1, inInc2;
  vtkIdType outInc0, outInc1, outInc2;
  int *wholeExtent, wholeMin0, wholeMax0, mid0;
  int inIdx0, outIdx0, idx1, idx2;
  int min0, max0, min1, max1, min2, max2;
  int numberOfComponents;
  int inCoords[3];
  unsigned long count = 0;
  unsigned long target;
  double startProgress;

  startProgress = this->GetIteration()/
    static_cast<double>(this->GetNumberOfIterations());

  // this filter expects that the input be doubles.
  if (inData->GetScalarType() != VTK_DOUBLE)
    {
    vtkErrorMacro(<< "Execute: Input must be be type double.");
    return;
    }
  // this filter expects that the output be doubles.
  if (outData->GetScalarType() != VTK_DOUBLE)
    {
    vtkErrorMacro(<< "Execute: Output must be be type double.");
    return;
    }
  // this filter expects input to have 1 or two components
  if (outData->GetNumberOfScalarComponents() != 1 &&
      outData->GetNumberOfScalarComponents() != 2)
    {
    vtkErrorMacro(<< "Execute: Cannot handle more than 2 components");
    return;
    }

  // Get stuff needed to loop through the pixel
  numberOfComponents = outData->GetNumberOfScalarComponents();
  outPtr0 = static_cast<double *>(outData->GetScalarPointerForExtent(outExt));
  wholeExtent = outputVector->GetInformationObject(0)->Get(
    vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  // permute to make the filtered axis come first
  this->PermuteExtent(outExt, min0, max0, min1, max1, min2, max2);
  this->PermuteIncrements(inData->GetIncrements(), inInc0, inInc1, inInc2);
  this->PermuteIncrements(outData->GetIncrements(), outInc0, outInc1, outInc2);

  // Determine the mid for the filtered axis
  wholeMin0 = wholeExtent[this->Iteration * 2];
  wholeMax0 = wholeExtent[this->Iteration * 2 + 1];
  mid0 = (wholeMin0 + wholeMax0) / 2;

  // initialize input coordinates
  inCoords[0] = outExt[0];
  inCoords[1] = outExt[2];
  inCoords[2] = outExt[4];

  target = static_cast<unsigned long>((max2-min2+1)*(max0-min0+1)
                                      * this->GetNumberOfIterations() / 50.0);
  target++;

  // loop over the filtered axis first
  for (outIdx0 = min0; outIdx0 <= max0; ++outIdx0)
    {
    // get the correct input pointer
    inIdx0 = outIdx0 + mid0;
    if (inIdx0 > wholeMax0)
      {
      inIdx0 -= (wholeMax0 - wholeMin0 + 1);
      }
    inCoords[this->Iteration] = inIdx0;
    inPtr0 = static_cast<double *>(inData->GetScalarPointer(inCoords));

    // loop over other axes
    inPtr2 = inPtr0;
    outPtr2 = outPtr0;
    for (idx2 = min2; !this->AbortExecute && idx2 <= max2; ++idx2)
      {
      if (!threadId)
        {
        if (!(count%target))
          {
          this->UpdateProgress(count/(50.0*target) + startProgress);
          }
        count++;
        }
      inPtr1 = inPtr2;
      outPtr1 = outPtr2;
      for (idx1 = min1; idx1 <= max1; ++idx1)
        {
        // handle components (up to 2) explicitly
        *outPtr1 = *inPtr1;
        if (numberOfComponents == 2)
          {
          outPtr1[1] = inPtr1[1];
          }

        inPtr1 += inInc1;
        outPtr1 += outInc1;
        }
      inPtr2 += inInc2;
      outPtr2 += outInc2;
      }
    outPtr0 += outInc0;
    }
}


















