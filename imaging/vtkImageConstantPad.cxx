/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageConstantPad.cxx
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
#include "vtkImageConstantPad.h"



//----------------------------------------------------------------------------
// Description:
// Constructor sets default values
vtkImageConstantPad::vtkImageConstantPad()
{
  // execute function handles four axes.
  this->ExecuteDimensionality = 4;
  // Not used
  this->Dimensionality = 4;
}



//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
template <class T>
void vtkImageConstantPadExecute(vtkImageConstantPad *self,
				vtkImageRegion *inRegion, T *inPtr,
				vtkImageRegion *outRegion, T *outPtr)
{
  int min0, max0, min1, max1, min2, max2, min3, max3;
  int imageMin0, imageMax0, imageMin1, imageMax1, 
    imageMin2, imageMax2, imageMin3, imageMax3;
  int outIdx0, outIdx1, outIdx2, outIdx3;
  int inInc0, inInc1, inInc2, inInc3;
  int outInc0, outInc1, outInc2, outInc3;
  T *inPtr0, *inPtr1, *inPtr2, *inPtr3;
  T *outPtr0, *outPtr1, *outPtr2, *outPtr3;
  int state0, state1, state2, state3;
  T constant;


  constant = (T)(self->GetConstant());
  // Get information to march through data 
  inRegion->GetIncrements(inInc0, inInc1, inInc2, inInc3);
  inRegion->GetImageExtent(imageMin0, imageMax0, imageMin1, imageMax1, 
			   imageMin2, imageMax2, imageMin3, imageMax3);
  outRegion->GetIncrements(outInc0, outInc1, outInc2, outInc3);
  outRegion->GetExtent(min0, max0, min1, max1, min2, max2, min3, max3);

  // Loop through ouput pixels
  inPtr3 = inPtr;
  outPtr3 = outPtr;
  for (outIdx3 = min3; outIdx3 <= max3; ++outIdx3)
    {
    state3 = (outIdx3 < imageMin3 || outIdx3 > imageMax3);
    outPtr2 = outPtr3;
    inPtr2 = inPtr3;
    for (outIdx2 = min2; outIdx2 <= max2; ++outIdx2)
      {
      state2 = (state3 || outIdx2 < imageMin2 || outIdx2 > imageMax2);
      outPtr1 = outPtr2;
      inPtr1 = inPtr2;
      for (outIdx1 = min1; outIdx1 <= max1; ++outIdx1)
	{
	state1 = (state2 || outIdx1 < imageMin1 || outIdx1 > imageMax1);
	outPtr0 = outPtr1;
	inPtr0 = inPtr1;
	for (outIdx0 = min0; outIdx0 <= max0; ++outIdx0)
	  {
	  state0 = (state1 || outIdx0 < imageMin0 || outIdx0 > imageMax0);
	  
	  // Copy Pixel
	  if (state0)
	    {
	    *outPtr0 = constant;
	    }
	  else
	    {
	    *outPtr0 = *inPtr0;
	    inPtr0 += inInc0;
	    }
	  outPtr0 += outInc0;
	  }
	if ( ! state1)
	  {
	  inPtr1 += inInc1;
	  }
	outPtr1 += outInc1;
	}
      if ( ! state2)
	{
	inPtr2 += inInc2;
	}
      outPtr2 += outInc2;
      }
    if ( ! state3)
      {
      inPtr3 += inInc3;
      }
    outPtr3 += outInc3;
    }
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageConstantPad::Execute(vtkImageRegion *inRegion, 
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
      vtkImageConstantPadExecute(this, inRegion, (float *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageConstantPadExecute(this, inRegion, (int *)(inPtr), 
			  outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageConstantPadExecute(this, inRegion, (short *)(inPtr), 
			  outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageConstantPadExecute(this, inRegion, (unsigned short *)(inPtr), 
			  outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageConstantPadExecute(this, inRegion, (unsigned char *)(inPtr), 
			  outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}
















