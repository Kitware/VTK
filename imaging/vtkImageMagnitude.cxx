/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMagnitude.cxx
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
#include <math.h>
#include "vtkImageRegion.h"
#include "vtkImageCache.h"
#include "vtkImageMagnitude.h"


//----------------------------------------------------------------------------
vtkImageMagnitude::vtkImageMagnitude()
{
  this->SetExecutionAxes(VTK_IMAGE_COMPONENT_AXIS);
  // For better performance, the execute function was written as a 3d.
  this->NumberOfExecutionAxes = 3;
}

//----------------------------------------------------------------------------
// Description:
// This method tells the superclass that the first axis will collapse.
void vtkImageMagnitude::ExecuteImageInformation(vtkImageCache *in,
						vtkImageCache *out)
{
  // Avoid compiler warnings.
  in = in;
  out->SetNumberOfScalarComponents(1);
}

//----------------------------------------------------------------------------
// Description:
// This templated execute method handles any type input, and output.
// Axis 0 should be components.
template <class T1, class T2>
static void vtkImageMagnitudeExecute(vtkImageMagnitude *self,
			      vtkImageRegion *inRegion, T1 *inPtr,
			      vtkImageRegion *outRegion, T2 *outPtr)
{
  int idx0, idx1, idx2;
  int min0, max0, min1, max1, min2, max2;
  int inInc0, inInc1, inInc2;
  int outInc0, outInc1, outInc2;
  T1 *inPtr0, *inPtr1, *inPtr2;
  T2 *outPtr1, *outPtr2;
  float sum;
  
  // Avoid complier warnings
  self = self;
  
  // get information to loop through pixels.
  inRegion->GetExtent(min0, max0, min1, max1, min2, max2);
  inRegion->GetIncrements(inInc0, inInc1, inInc2);
  outRegion->GetIncrements(outInc0, outInc1, outInc2);
  
  inPtr2 = inPtr;
  outPtr2 = outPtr;
  for (idx2 = min2; idx2 <= max2; ++idx2)
    {
    inPtr1 = inPtr2;
    outPtr1 = outPtr2;
    for (idx1 = min1; idx1 <= max1; ++idx1)
      {
      // Compute magnitude along last axis.
      inPtr0 = inPtr1;
      sum = 0.0;
      for (idx0 = min0; idx0 <= max0; ++idx0)
	{
	sum += (float)(*inPtr0 * *inPtr0);
	inPtr0 += inInc0;
	}
      *outPtr1 = (T2)(sqrt(sum));
      inPtr1 += inInc1;
      outPtr1 += outInc1;
      }
    inPtr2 += inInc2;
    outPtr2 += outInc2;
    }
}

//----------------------------------------------------------------------------
template <class T>
static void vtkImageMagnitudeExecute(vtkImageMagnitude *self, 
			      vtkImageRegion *inRegion, 
			      vtkImageRegion *outRegion, T *outPtr)
{
  void *inPtr;

  inPtr = inRegion->GetScalarPointer();

  // choose which templated function to call.
  switch (inRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageMagnitudeExecute(self, inRegion, (float *)(inPtr), 
			       outRegion, outPtr);
      break;
    case VTK_INT:
      vtkImageMagnitudeExecute(self, inRegion, (int *)(inPtr),
			       outRegion, outPtr);
      break;
    case VTK_SHORT:
      vtkImageMagnitudeExecute(self, inRegion, (short *)(inPtr),
			       outRegion, outPtr);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageMagnitudeExecute(self, inRegion, (unsigned short *)(inPtr),
			       outRegion, outPtr);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageMagnitudeExecute(self, inRegion, (unsigned char *)(inPtr),
			       outRegion, outPtr);
      break;
    default:
      vtkGenericWarningMacro("Execute: Unknown ScalarType");
      return;
    }
}

//----------------------------------------------------------------------------
// Description:
// This method is passed input and output regions, and executes the
// magnitude function on each line.  It handles 3 axes for speed.
void vtkImageMagnitude::Execute(vtkImageRegion *inRegion, 
				vtkImageRegion *outRegion)
{
  void *outPtr;

  outPtr = outRegion->GetScalarPointer();

  // choose which templated function to call.
  switch (outRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageMagnitudeExecute(this, inRegion, outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageMagnitudeExecute(this, inRegion, outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageMagnitudeExecute(this, inRegion, outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageMagnitudeExecute(this, inRegion,outRegion, 
			       (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageMagnitudeExecute(this, inRegion, outRegion, 
			       (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}



















