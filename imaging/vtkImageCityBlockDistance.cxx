/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCityBlockDistance.cxx
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
#include "vtkImageCityBlockDistance.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageCityBlockDistance* vtkImageCityBlockDistance::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageCityBlockDistance");
  if(ret)
    {
    return (vtkImageCityBlockDistance*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageCityBlockDistance;
}




//----------------------------------------------------------------------------
vtkImageCityBlockDistance::vtkImageCityBlockDistance()
{
}


//----------------------------------------------------------------------------
void vtkImageCityBlockDistance::AllocateOutputScalars(vtkImageData *outData)
{
  int *wholeExtent, updateExtent[6], idx;
  
  if ( ! this->GetInput())
    {
    vtkErrorMacro(<< "Input not set.");
    return;
    }

  outData->GetUpdateExtent(updateExtent);
  wholeExtent = outData->GetWholeExtent();
  for (idx = 0; idx < this->Dimensionality; ++idx)
    {
    updateExtent[idx*2] = wholeExtent[idx*2];
    updateExtent[idx*2+1] = wholeExtent[idx*2+1];
    }
  outData->SetExtent(updateExtent);
  this->GetOutput()->AllocateScalars();
}


//----------------------------------------------------------------------------
// This method tells the superclass that the whole input array is needed
// to compute any output region.
void vtkImageCityBlockDistance::ComputeInputUpdateExtent(int inExt[6],
							 int outExt[6])
{
  int *wholeExtent;

  if ( ! this->GetInput())
    {
    vtkErrorMacro(<< "Input not set.");
    return;
    }

  memcpy(inExt, outExt, 6 * sizeof(int));
  wholeExtent = this->GetInput()->GetWholeExtent();
  inExt[this->Iteration * 2] = wholeExtent[this->Iteration * 2];
  inExt[this->Iteration * 2 + 1] = wholeExtent[this->Iteration * 2 + 1];
}


//----------------------------------------------------------------------------
// This is writen as a 1D execute method, but is called several times.
void vtkImageCityBlockDistance::IterativeExecuteData(vtkImageData *inData, 
                                                     vtkImageData *outData)
{
  short *inPtr0, *inPtr1, *inPtr2, *inPtrC;
  short *outPtr0, *outPtr1, *outPtr2, *outPtrC;
  int inInc0, inInc1, inInc2;
  int outInc0, outInc1, outInc2;
  int min0, max0, min1, max1, min2, max2, numberOfComponents;
  int idx0, idx1, idx2, idxC;
  short distP, distN;
  short big = 2000;
  int outExt[6];
  unsigned long count = 0;
  unsigned long target;
  
  this->GetOutput()->GetUpdateExtent(outExt);

  // this filter expects that inputand output are short
  if (inData->GetScalarType() != VTK_SHORT ||
      outData->GetScalarType() != VTK_SHORT)
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
                  << ", and out ScalarType " << outData->GetScalarType()
                  << " must be short.");
    return;
    }


  // Reorder axes (the in and out extents are assumed to be the same)
  // (see intercept cache update)
  this->PermuteExtent(outExt, min0, max0, min1, max1, min2, max2);
  this->PermuteIncrements(inData->GetIncrements(), inInc0, inInc1, inInc2);
  this->PermuteIncrements(outData->GetIncrements(), outInc0, outInc1, outInc2);
  numberOfComponents = inData->GetNumberOfScalarComponents();
  
  target = (unsigned long)((max2-min2+1)*(max1-min1+1)/50.0);
  target++;
  
  // loop over all the extra axes
  inPtr2 = (short *)inData->GetScalarPointerForExtent(outExt);
  outPtr2 = (short *)outData->GetScalarPointerForExtent(outExt);
  for (idx2 = min2; idx2 <= max2; ++idx2)
    {
    inPtr1 = inPtr2;
    outPtr1 = outPtr2;
    for (idx1 = min1; !this->AbortExecute && idx1 <= max1; ++idx1)
      {
      if (!(count%target))
	{
	this->UpdateProgress(count/(50.0*target));
	}
      count++;
      inPtrC = inPtr1;
      outPtrC = outPtr1;
      for (idxC = 0; idxC < numberOfComponents; ++idxC)
	{
	// execute forward pass
	distP = big;
	distN = -big;
	inPtr0 = inPtrC;
	outPtr0 = outPtrC;
	for (idx0 = min0; idx0 <= max0; ++idx0)
	  { // preserve sign
	  if (*inPtr0 >= 0)
	    {
	    distN = 0;
	    if (distP > *inPtr0)
	      {
	      distP = *inPtr0;
	      }
	    *outPtr0 = distP;
	    }
	  if (*inPtr0 <= 0)
	    {
	    distP = 0;
	    if (distN < *inPtr0)
	      {
	      distN = *inPtr0;
	      }
	    *outPtr0 = distN;
	    }
	  
	  if (distP < big)
	    {
	    ++distP;
	    }
	  if (distN > -big)
	    {
	    --distN;
	    }
	  
	  inPtr0 += inInc0;
	  outPtr0 += outInc0;
	  }
	
	// backward pass
	distP = big;
	distN = -big;
	// Undo the last increment to put us at the last pixel
	// (input is no longer needed)
	outPtr0 -= outInc0;  
	for (idx0 = max0; idx0 >= min0; --idx0)
	  {
	  if (*outPtr0 >= 0)
	    {
	    if (distP > *outPtr0)
	      {
	      distP = *outPtr0;
	      }
	    *outPtr0 = distP;
	    }
	  if (*outPtr0 <= 0)
	    {
	    if (distN < *outPtr0)
	      {
	      distN = *outPtr0;
	      }
	    *outPtr0 = distN;
	    }
	  
	  if (distP < big)
	    {
	    ++distP;
	    }
	  if (distN > -big)
	    {
	    --distN;
	    }
	  
	  outPtr0 -= outInc0;
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
}


