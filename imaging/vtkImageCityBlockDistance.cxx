/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCityBlockDistance.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkImageCityBlockDistance.h"

//----------------------------------------------------------------------------
vtkImageCityBlockDistance::vtkImageCityBlockDistance()
{
}


//----------------------------------------------------------------------------
// Description:
// Intercepts the caches Update to make the region larger than requested.
// Create the whole output array.
void vtkImageCityBlockDistance::InterceptCacheUpdate()
{
  int *wholeExtent, updateExtent[6], idx;
  
  // Filter superclass has no control of intercept cache update.
  // a work around
  if (this->Bypass)
    {
    return;
    }
  
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "Input not set.");
    return;
    }

  this->Output->GetUpdateExtent(updateExtent);
  wholeExtent = this->Output->GetWholeExtent();
  for (idx = 0; idx < this->Dimensionality; ++idx)
    {
    updateExtent[idx*2] = wholeExtent[idx*2];
    updateExtent[idx*2+1] = wholeExtent[idx*2+1];
    }
  this->Output->SetUpdateExtent(updateExtent);
}


//----------------------------------------------------------------------------
// Description:
// This method tells the superclass that the whole input array is needed
// to compute any output region.
void vtkImageCityBlockDistance::ComputeRequiredInputUpdateExtent(int inExt[6],
								 int outExt[6])
{
  int *wholeExtent;

  memcpy(inExt, outExt, 6 * sizeof(int));
  wholeExtent = this->Input->GetWholeExtent();
  inExt[this->Iteration * 2] = wholeExtent[this->Iteration * 2];
  inExt[this->Iteration * 2 + 1] = wholeExtent[this->Iteration * 2 + 1];
}


//----------------------------------------------------------------------------
// Description:
// This is writen as a 1D execute method, but is called several times.
void vtkImageCityBlockDistance::Execute(vtkImageData *inData, 
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
  
  this->Output->GetUpdateExtent(outExt);

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


