/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFourierCenter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

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
#include <math.h>

#include "vtkImageFourierCenter.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageFourierCenter* vtkImageFourierCenter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageFourierCenter");
  if(ret)
    {
    return (vtkImageFourierCenter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageFourierCenter;
}





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


















