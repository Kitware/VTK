/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageWrapPad.cxx
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
#include "vtkImageRegion.h"
#include "vtkImageWrapPad.h"



//----------------------------------------------------------------------------
// Description:
// Constructor sets default values
vtkImageWrapPad::vtkImageWrapPad()
{
  // execute function handles four axes.
  this->ExecuteDimensionality = 4;
}



//----------------------------------------------------------------------------
// Just clip the request.
void 
vtkImageWrapPad::ComputeRequiredInputRegionExtent(vtkImageRegion *outRegion,
						  vtkImageRegion *inRegion)
{
  int idx;
  int extent[8];
  int min, max, width, imageMin, imageMax, imageWidth;
  int *imageExtent;
  
  outRegion->GetExtent(VTK_IMAGE_DIMENSIONS, extent);
  imageExtent = inRegion->GetImageExtent();

  // Clip
  for (idx = 0; idx < 4; ++idx)
    {
    min = extent[idx * 2];
    max = extent[idx * 2 + 1];
    width = max - min + 1;
    imageMin = imageExtent[idx * 2];
    imageMax = imageExtent[idx * 2 + 1];
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
    
    extent[idx * 2] = min;
    extent[idx * 2 + 1] = max;
    }
  
  inRegion->SetExtent(4, extent);
}




//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageWrapPadExecute(vtkImageWrapPad *self,
			    vtkImageRegion *inRegion, T *inPtr,
			    vtkImageRegion *outRegion, T *outPtr)
{
  int min0, max0, min1, max1, min2, max2, min3, max3;
  int imageMin0, imageMax0, imageMin1, imageMax1, 
    imageMin2, imageMax2, imageMin3, imageMax3;
  int outIdx0, outIdx1, outIdx2, outIdx3;
  int start0, start1, start2, start3;
  int inIdx0, inIdx1, inIdx2, inIdx3;
  int inInc0, inInc1, inInc2, inInc3;
  int outInc0, outInc1, outInc2, outInc3;
  T *inPtr0, *inPtr1, *inPtr2, *inPtr3;
  T *outPtr0, *outPtr1, *outPtr2, *outPtr3;

  self = self;
  inPtr = inPtr;
  
  // Get information to march through data 
  inRegion->GetIncrements(inInc0, inInc1, inInc2, inInc3);
  inRegion->GetImageExtent(imageMin0, imageMax0, imageMin1, imageMax1, 
			   imageMin2, imageMax2, imageMin3, imageMax3);
  outRegion->GetIncrements(outInc0, outInc1, outInc2, outInc3);
  outRegion->GetExtent(min0, max0, min1, max1, min2, max2, min3, max3);

  // initialize pointers to coresponding pixels.
  start0 = ((min0 - imageMin0) % (imageMax0-imageMin0+1)) + imageMin0;
  if (start0 < 0) start0 += (imageMax0-imageMin0+1);
  start1 = ((min1 - imageMin1) % (imageMax1-imageMin1+1)) + imageMin1;
  if (start1 < 0) start1 += (imageMax1-imageMin1+1);
  start2 = ((min2 - imageMin2) % (imageMax2-imageMin2+1)) + imageMin2;
  if (start2 < 0) start2 += (imageMax2-imageMin2+1);
  start3 = ((min3 - imageMin3) % (imageMax3-imageMin3+1)) + imageMin3;
  if (start3 < 0) start3 += (imageMax3-imageMin3+1);
  inPtr3 = (T *)(inRegion->GetScalarPointer(start0, start1, start2, start3));
  outPtr3 = outPtr; // (min0, min1, min2, min3)
  
  // Loop through ouput pixels
  inIdx3 = start3;
  for (outIdx3 = min3; outIdx3 <= max3; ++outIdx3, ++inIdx3)
    {
    if (inIdx3 > imageMax3) 
      { // we need to wrap(rewind) the input on this axis
      inIdx3 = imageMin3;
      inPtr3 -= (imageMax3-imageMin3+1)*inInc3;
      }
    outPtr2 = outPtr3;
    inPtr2 = inPtr3;
    inIdx2 = start2;
    for (outIdx2 = min2; outIdx2 <= max2; ++outIdx2, ++inIdx2)
      {
      if (inIdx2 > imageMax2) 
	{ // we need to wrap(rewind) the input on this axis
	inIdx2 = imageMin2;
	inPtr2 -= (imageMax2-imageMin2+1)*inInc2;
	}
      outPtr1 = outPtr2;
      inPtr1 = inPtr2;
      inIdx1 = start1;
      for (outIdx1 = min1; outIdx1 <= max1; ++outIdx1, ++inIdx1)
	{
	if (inIdx1 > imageMax1) 
	  { // we need to wrap(rewind) the input on this axis
	  inIdx1 = imageMin1;
	  inPtr1 -= (imageMax1-imageMin1+1)*inInc1;
	  }
	outPtr0 = outPtr1;
	inPtr0 = inPtr1;
	inIdx0 = start0;
	for (outIdx0 = min0; outIdx0 <= max0; ++outIdx0, ++inIdx0)
	  {
	  if (inIdx0 > imageMax0) 
	    { // we need to wrap(rewind) the input on this axis
	    inIdx0 = imageMin0;
	    inPtr0 -= (imageMax0-imageMin0+1)*inInc0;
	    }
	  
	  // Copy Pixel
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
    outPtr3 += outInc3;
    inPtr3 += inInc3;
    }
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageWrapPad::Execute(vtkImageRegion *inRegion, 
			      vtkImageRegion *outRegion)
{
  void *inPtr = inRegion->GetScalarPointer();
  void *outPtr = outRegion->GetScalarPointer();
  
  vtkDebugMacro(<< "Execute: inRegion = " << inRegion 
		<< ", outRegion = " << outRegion);
  
  // this filter expects that input is the same type as output.
  if (inRegion->GetScalarType() != outRegion->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inRegion->GetScalarType()
          << ", must match out ScalarType " << outRegion->GetScalarType());
    return;
    }
  
  switch (inRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageWrapPadExecute(this, inRegion, (float *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageWrapPadExecute(this, inRegion, (int *)(inPtr), 
			  outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageWrapPadExecute(this, inRegion, (short *)(inPtr), 
			  outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageWrapPadExecute(this, inRegion, (unsigned short *)(inPtr), 
			  outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageWrapPadExecute(this, inRegion, (unsigned char *)(inPtr), 
			  outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}
















