/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDilateFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
#include "vtkImageDilateFilter.h"


//----------------------------------------------------------------------------
// Description:
// Constructor: Sets default filter to be identity.
vtkImageDilateFilter::vtkImageDilateFilter()
{
  this->SetRadius(1, 1, 0);
}


//----------------------------------------------------------------------------
// Description:
// This method computes the Region of the input necessary to generate outRegion.
void vtkImageDilateFilter::RequiredRegion(int *outOffset, int *outSize,
				int *inOffset, int *inSize)
{
  int idx;

  // ignoring boundaries for now
  for (idx = 0; idx < 3; ++idx)
    {
    inOffset[idx] = outOffset[idx] - this->Radius[idx];
    inSize[idx] = outSize[idx] + 2 * this->Radius[idx];
    }
}


//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output tile, and executes the Median
// algorithm to fill the output from the input.
// As a place holder, an identity Median is implemented.
void vtkImageDilateFilter::Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion)
{
  int size0, size1, size2;
  int idx0, idx1, idx2;
  int inInc0, inInc1, inInc2;
  int outInc0, outInc1, outInc2;
  float *inPtr0, *inPtr1, *inPtr2;
  float *outPtr0, *outPtr1, *outPtr2;

  // Get information to march through data
  inPtr2 = inRegion->GetPointer(inRegion->GetOffset());
  inRegion->GetInc(inInc0, inInc1, inInc2);  
  outPtr2 = outRegion->GetPointer(outRegion->GetOffset());
  outRegion->GetInc(outInc0, outInc1, outInc2);  
  outRegion->GetSize(size0, size1, size2);  

  vtkDebugMacro(<< "Execute: inRegion = (" << inRegion 
		<< "), outRegion = (" << outRegion << ")");
    
  // perform filter for each pixel of output
  for (idx2 = 0; idx2 < size2; ++idx2){
    outPtr1 = outPtr2;
    inPtr1 = inPtr2;
    for (idx1 = 0; idx1 < size1; ++idx1){
      outPtr0 = outPtr1;
      inPtr0 = inPtr1;
      for (idx0 = 0; idx0 < size0; ++idx0){

        // Replace this pixel with the neighborhood maximum
        *outPtr0 = this->NeighborhoodMax(inPtr0, inInc0, inInc1, inInc2);

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
// Description:
// This private method calculates and returns the median of a neighborhood
// around a pixel.  Is this approach faster than a sort?
float vtkImageDilateFilter::NeighborhoodMax(float *inPtr, 
				     int inc0, int inc1, int inc2)
{
  int idx0, idx1, idx2;
  float *ptr0, *ptr1, *ptr2;
  int diam0, diam1, diam2;
  float max;

  // determine size of neighborhood. (should be pre computed)
  diam0 = 1 + 2 * this->Radius[0];
  diam1 = 1 + 2 * this->Radius[1];
  diam2 = 1 + 2 * this->Radius[2];


  // find the corner of the neighborhood (already at corner)
  ptr2 = inPtr;
  // loop over neighborhood pixels finding maximum
  max = *inPtr;
  for (idx2 = 0; idx2 < diam2; ++idx2)
    {
    ptr1 = ptr2;
    for (idx1 = 0; idx1 < diam1; ++idx1)
      {
      ptr0 = ptr1;
      for (idx0 = 0; idx0 < diam0; ++idx0)
	{

	// keep maximum
	if (max < *ptr0)
	  max = *ptr0;

	ptr0 += inc0;
	}
      ptr1 += inc1;
      }
    ptr2 += inc2;
    }

  return max;
}










