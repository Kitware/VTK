/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFourierCenter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageFourierCenter.h"
#include "vtkObjectFactory.h"

#include <math.h>

vtkCxxRevisionMacro(vtkImageFourierCenter, "1.15");
vtkStandardNewMacro(vtkImageFourierCenter);

//----------------------------------------------------------------------------
// Construct an instance of vtkImageFourierCenter fitler.
vtkImageFourierCenter::vtkImageFourierCenter()
{
}


//----------------------------------------------------------------------------
// This method tells the superclass which input extent is needed.
// This gets the whole input (even though it may not be needed).
void vtkImageFourierCenter::ComputeInputUpdateExtent(int inExt[6], 
                                                     int outExt[6])
{
  int *extent;

  // Assumes that the input update extent has been initialized to output ...
  extent = this->GetInput()->GetWholeExtent();
  memcpy(inExt, outExt, 6 * sizeof(int));
  inExt[this->Iteration*2] = extent[this->Iteration*2];
  inExt[this->Iteration*2 + 1] = extent[this->Iteration*2 + 1];
}

//----------------------------------------------------------------------------
// This method is passed input and output regions, and executes the fft
// algorithm to fill the output from the input.
void vtkImageFourierCenter::ThreadedExecute(vtkImageData *inData, 
                                            vtkImageData *outData,
                                            int outExt[6], int threadId)
{
  float *inPtr0, *inPtr1, *inPtr2;
  float *outPtr0, *outPtr1, *outPtr2;
  int inInc0, inInc1, inInc2;
  int outInc0, outInc1, outInc2;
  int *wholeExtent, wholeMin0, wholeMax0, mid0;
  int inIdx0, outIdx0, idx1, idx2;
  int min0, max0, min1, max1, min2, max2;
  int numberOfComponents;
  int inCoords[3];
  unsigned long count = 0;
  unsigned long target;
  float startProgress;

  startProgress = this->GetIteration()/(float)(this->GetNumberOfIterations());
  
  // this filter expects that the input be floats.
  if (inData->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: Input must be be type float.");
    return;
    }
  // this filter expects that the output be floats.
  if (outData->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: Output must be be type float.");
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
  outPtr0 = (float *)(outData->GetScalarPointerForExtent(outExt));
  wholeExtent = this->GetOutput()->GetWholeExtent();
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
  
  target = (unsigned long)((max2-min2+1)*(max0-min0+1)
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
    inPtr0 = (float *)(inData->GetScalarPointer(inCoords));
    
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


















