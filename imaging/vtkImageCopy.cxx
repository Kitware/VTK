/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCopy.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Abdalmajeid M. Alyassin who developed this class.

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
#include "vtkImageCache.h"
#include "vtkImageCopy.h"

//----------------------------------------------------------------------------
// Description:
// Constructor sets default values
vtkImageCopy::vtkImageCopy()
{
  this->SetExecutionAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS);
}


//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageCopyExecute(vtkImageCopy *self,
				vtkImageRegion *inRegion, T *inPtr,
				vtkImageRegion *outRegion, T *outPtr)
{
  int min0, max0, min1, max1;
  int idx0, idx1;
  int inInc0, inInc1;
  int outInc0, outInc1;
  T  *inPtr0, *inPtr1;
  T  *outPtr0, *outPtr1;

  self = self;
  
  // Get information to march through data 
  inRegion->GetIncrements(inInc0, inInc1);
  outRegion->GetIncrements(outInc0, outInc1);
  outRegion->GetExtent(min0, max0, min1, max1);

  // Loop through ouput pixels
  inPtr1 = inPtr;
  outPtr1 = outPtr;
  for (idx1 = min1; idx1 <= max1; ++idx1)
    {
    outPtr0 = outPtr1;
    inPtr0 = inPtr1;
    for (idx0 = min0; idx0 <= max0; ++idx0)
      {
      *outPtr0 = *inPtr0;
      outPtr0 += outInc0;
      inPtr0  += inInc0;
      }
    outPtr1 += outInc1;
    inPtr1 += inInc1;
  }
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageCopy::Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion)
{
  void *inPtr = inRegion->GetScalarPointer();
  void *outPtr = outRegion->GetScalarPointer();
  
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
      vtkImageCopyExecute(this, 
			  inRegion, (float *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageCopyExecute(this, 
			  inRegion, (int *)(inPtr), 
			  outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageCopyExecute(this, 
			  inRegion, (short *)(inPtr), 
			  outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageCopyExecute(this, 
			  inRegion, (unsigned short *)(inPtr), 
			  outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageCopyExecute(this, 
			  inRegion, (unsigned char *)(inPtr), 
			  outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}
















