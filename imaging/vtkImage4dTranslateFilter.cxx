/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImage4dTranslateFilter.cxx
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
#include "vtkImage4dTranslateFilter.h"



//----------------------------------------------------------------------------
vtkImage4dTranslateFilter::vtkImage4dTranslateFilter()
{
  this->SetTranslation(0, 0, 0, 0);
  this->SetAxes4d(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS,
		  VTK_IMAGE_Z_AXIS, VTK_IMAGE_TIME_AXIS);
}



//----------------------------------------------------------------------------
void vtkImage4dTranslateFilter::ComputeOutputImageInformation(
		      vtkImageRegion *inRegion, vtkImageRegion *outRegion)
{
  int idx;
  int bounds[8];
  
  inRegion->GetImageBounds4d(bounds);
  for(idx = 0; idx < 4; ++idx)
    {
    bounds[idx*2] += this->Translation[idx];
    bounds[idx*2+1] += this->Translation[idx];    
    }
  
  outRegion->SetImageBounds4d(bounds);
}

  



//----------------------------------------------------------------------------
void vtkImage4dTranslateFilter::ComputeRequiredInputRegionBounds(
			 vtkImageRegion *outRegion, vtkImageRegion *inRegion)
{
  int idx;
  int bounds[8];
  
  outRegion->GetBounds4d(bounds);
  for(idx = 0; idx < 4; ++idx)
    {
    bounds[idx*2] -= this->Translation[idx];
    bounds[idx*2+1] -= this->Translation[idx];    
    }
  
  inRegion->SetBounds4d(bounds);
}

  








//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
template <class T>
void vtkImage4dTranslateFilterExecute4d(vtkImage4dTranslateFilter *self,
					vtkImageRegion *inRegion, T *inPtr,
					vtkImageRegion *outRegion, T *outPtr)
{
  int min0, max0, min1, max1, min2, max2, min3, max3;
  int idx0, idx1, idx2, idx3;
  int inInc0, inInc1, inInc2, inInc3;
  int outInc0, outInc1, outInc2, outInc3;
  T *inPtr0, *inPtr1, *inPtr2, *inPtr3;
  T *outPtr0, *outPtr1, *outPtr2, *outPtr3;

  self = self;
  
  // Get information to march through data 
  inRegion->GetIncrements4d(inInc0, inInc1, inInc2, inInc3);
  outRegion->GetIncrements4d(outInc0, outInc1, outInc2, outInc3);
  outRegion->GetBounds4d(min0, max0, min1, max1,
			 min2, max2, min3, max3);

  // Loop through ouput pixels
  inPtr3 = inPtr;
  outPtr3 = outPtr;
  for (idx3 = min3; idx3 <= max3; ++idx3)
    {
    outPtr2 = outPtr3;
    inPtr2 = inPtr3;
    for (idx2 = min2; idx2 <= max2; ++idx2)
      {
      inPtr1 = inPtr2;
      outPtr1 = outPtr2;
      for (idx1 = min1; idx1 <= max1; ++idx1)
	{
	outPtr0 = outPtr1;
	inPtr0 = inPtr1;
	for (idx0 = min0; idx0 <= max0; ++idx0)
	  {
	  
	  // Pixel operation
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
void vtkImage4dTranslateFilter::Execute4d(vtkImageRegion *inRegion, 
					  vtkImageRegion *outRegion)
{
  void *inPtr = inRegion->GetVoidPointer4d();
  void *outPtr = outRegion->GetVoidPointer4d();
  
  vtkDebugMacro(<< "Execute: inRegion = " << inRegion 
		<< ", outRegion = " << outRegion);
  
  // this filter expects that input is the same type as output.
  if (inRegion->GetDataType() != outRegion->GetDataType())
    {
    vtkErrorMacro(<< "Execute: input DataType, " << inRegion->GetDataType()
                  << ", must match out DataType " << outRegion->GetDataType());
    return;
    }
  
  switch (inRegion->GetDataType())
    {
    case VTK_IMAGE_FLOAT:
      vtkImage4dTranslateFilterExecute4d(this, 
			  inRegion, (float *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_IMAGE_INT:
      vtkImage4dTranslateFilterExecute4d(this, 
			  inRegion, (int *)(inPtr), 
			  outRegion, (int *)(outPtr));
      break;
    case VTK_IMAGE_SHORT:
      vtkImage4dTranslateFilterExecute4d(this, 
			  inRegion, (short *)(inPtr), 
			  outRegion, (short *)(outPtr));
      break;
    case VTK_IMAGE_UNSIGNED_SHORT:
      vtkImage4dTranslateFilterExecute4d(this, 
			  inRegion, (unsigned short *)(inPtr), 
			  outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_IMAGE_UNSIGNED_CHAR:
      vtkImage4dTranslateFilterExecute4d(this, 
			  inRegion, (unsigned char *)(inPtr), 
			  outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown DataType");
      return;
    }
}
















