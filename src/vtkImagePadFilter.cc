/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePadFilter.cc
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
#include "vtkImagePadFilter.hh"



//----------------------------------------------------------------------------
// Description:
// Constructor
vtkImagePadFilter::vtkImagePadFilter()
{
  this->PadValue = 0.0;

  this->BoundaryOffset[0] = 0;
  this->BoundaryOffset[1] = 0;
  this->BoundaryOffset[2] = 0;
  
  this->BoundarySize[0] = 512;
  this->BoundarySize[1] = 512;
  this->BoundarySize[2] = 1;
}


//----------------------------------------------------------------------------
// Description:
// Returns the largest region which can be requested.
// Just returns the boundary defined for this object.
void vtkImagePadFilter::GetBoundary(int *offset, int *size)
{
  int idx;
  
  for (idx = 0; idx < 3; ++idx)
    {
    offset[idx] = this->BoundaryOffset[idx];
    size[idx] = this->BoundarySize[idx];
    }
  
  vtkDebugMacro(<< "GetBoundary: returning offset = ("
          << offset[0] << ", " << offset[1] << ", " << offset[2]
          << "), size = (" << size[0] << ", " << size[1] << ", " << size[2]
          << ")");  
}




//----------------------------------------------------------------------------
// Description:
// This method computes the Region of input necessary to generate out Region.
// For this filter the input region is the output region clipped by the real
// boundaries.
void vtkImagePadFilter::RequiredRegion(int *outOffset, int *outSize,
				       int *inOffset, int *inSize)
{
  int idx;
  int boundaryOffset[3], boundarySize[3];
  int tmp1, tmp2;

  // get the Boundary of the input
  this->Input->GetBoundary(boundaryOffset, boundarySize);  

  for (idx = 0; idx < 3; ++idx)
    {
    // Max of left size
    tmp1 = outOffset[idx];
    tmp2 = boundaryOffset[idx];
    inOffset[idx] = (tmp1 > tmp2) ? tmp1 : tmp2;
    // compute the right hand side
    tmp1 = outOffset[idx] + outSize[idx];
    tmp2 = boundaryOffset[idx] + boundarySize[idx];
    tmp1 = (tmp1 < tmp2) ? tmp1 : tmp2;
    inSize[idx] = tmp1 - inOffset[idx];
    if (inSize[idx] < 0)
      inSize[idx] = 0;
    }
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// Pad just copies pixel by pixel and fills the rest with the pad value.
void vtkImagePadFilter::Execute(vtkImageRegion *inRegion, 
				vtkImageRegion *outRegion)
{
  int idx0, idx1, idx2;
  int inSize0, inSize1, inSize2;
  int inInc0, inInc1, inInc2;
  int outInc0, outInc1, outInc2;
  float *inPtr0, *inPtr1, *inPtr2;
  float *outPtr0, *outPtr1, *outPtr2;

  
  vtkDebugMacro(<< "Execute: inRegion = " << inRegion 
		<< ", outRegion = " << outRegion);
  
  // Get information from regions to march through data
  inRegion->GetInc(inInc0, inInc1, inInc2);
  outRegion->GetInc(outInc0, outInc1, outInc2);
  inRegion->GetSize(inSize0, inSize1, inSize2);

  if (inSize0 > 0 && inSize1 > 0 && inSize2 > 0)
    {
    // Copy the input region to the output region (assumed to be contained in).
    inPtr2 = inRegion->GetPointer(inRegion->GetOffset());
    outPtr2 = outRegion->GetPointer(inRegion->GetOffset());
    for (idx2 = 0; idx2 < inSize2; ++idx2)
      {
      outPtr1 = outPtr2;
      inPtr1 = inPtr2;
      for (idx1 = 0; idx1 < inSize1; ++idx1)
	{
	outPtr0 = outPtr1;
	inPtr0 = inPtr1;
	for (idx0 = 0; idx0 < inSize0; ++idx0)
	  {
	  
	  // Copy the pixel
	  *outPtr0 = *inPtr0;
	  
	  outPtr0 += outInc0;
	  inPtr0 += inInc0;
	  }
	outPtr1 += outInc1;
	inPtr1 += inInc1;
	}
      outPtr2 += outInc2;
      inPtr2 += inInc2;
      }

    // Pad the rest of the output
    this->Pad(inRegion, outRegion);
    }
  else
    {
    // Special case: No overlap.  Just fill the entire region with pad value
    this->PadRegion(outRegion, outRegion->GetOffset(), outRegion->GetSize());
    }
}


  
//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output region.
// It fills the output region not covered by input region with the pad value.
// IT ASSUMES INPUT IS CONTAINED IN OUTPUT.
void vtkImagePadFilter::Pad(vtkImageRegion *inRegion, 
			    vtkImageRegion *outRegion)
{
  int padOffset[3], padSize[3];
  int filledOffset[3], filledSize[3];
  int *outOffset, *outSize;
  int idx;
  
  // pad the rest of the output
  inRegion->GetOffset(padOffset);
  inRegion->GetSize(padSize);
  inRegion->GetOffset(filledOffset);
  inRegion->GetSize(filledSize);
  outOffset = outRegion->GetOffset();
  outSize = outRegion->GetSize();

  vtkDebugMacro(<< "Pad: inOffset = (" << padOffset[0] << ", " << padOffset[1] 
                << ", " << padOffset[2] << "), inSize = (" << padSize[0] << ", "
                << padSize[1] << ", " << padSize[2] << ")");
  
  
  // loop through the axes
  for (idx = 0; idx < 3; ++idx) 
    {
    // extend below filled region on this axis
    if (outOffset[idx] < filledOffset[idx])
      {
      // Assumes pad region is the same as filled region
      padOffset[idx] = outOffset[idx];
      padSize[idx] = filledOffset[idx] - outOffset[idx];
      // pad this portion of the region.
      this->PadRegion(outRegion, padOffset, padSize);
      // leave pad equal to filled
      filledOffset[idx] = padOffset[idx];
      padSize[idx] = filledSize[idx] += padSize[idx];
      }
    // extend above filled region for this axis
    if (outSize[idx] > filledSize[idx])
      {
      padOffset[idx] = filledOffset[idx] + filledSize[idx];
      padSize[idx] = outSize[idx] - filledSize[idx];
      // pad this portion of the region.
      this->PadRegion(outRegion, padOffset, padSize);
      // leave pad equal to filled
      padSize[idx] = filledSize[idx] += padSize[idx];
      padOffset[idx] = filledOffset[idx];
      }
    }
}


//----------------------------------------------------------------------------
// Description:
// This method fills a rectangular portion of a region with the pad value.
void vtkImagePadFilter::PadRegion(vtkImageRegion *region,
				  int *offset, int *size)
{
  int idx0, idx1, idx2;
  int size0, size1, size2;
  int inc0, inc1, inc2;
  float *ptr0, *ptr1, *ptr2;

  // if the region is empty return imediately
  if (size[0] <= 0 || size[1] <= 0 || size[1] <= 0)
    return;
  
  vtkDebugMacro(<< "PadRegion: region = (" << region << "), offset = ("
     << offset[0] << ", " << offset[1] << ", " << offset[2]
     << "), size = (" << size[0] << ", " << size[1] << ", " << size[2] << ")");
  
  
  // Get information from regions to march through data
  region->GetInc(inc0, inc1, inc2);
  size0 = size[0];  size1 = size[1];  size2 = size[2];
  
  // Loop through all the pixels in this region.
  ptr2 = region->GetPointer(offset);
  for (idx2 = 0; idx2 < size2; ++idx2)
    {
    ptr1 = ptr2;
    for (idx1 = 0; idx1 < size1; ++idx1)
      {
      ptr0 = ptr1;
      for (idx0 = 0; idx0 < size0; ++idx0)
	{

	// Copy the pixel
	*ptr0 = this->PadValue;
	
	ptr0 += inc0;
	}
      ptr1 += inc1;
      }
    ptr2 += inc2;
    }
}





