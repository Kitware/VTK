/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageExtractComponent.cxx
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
#include "vtkImageExtractComponent.h"


//----------------------------------------------------------------------------
vtkImageExtractComponent::vtkImageExtractComponent()
{
  this->SetExecutionAxes(VTK_IMAGE_COMPONENT_AXIS);
  // For better performance, the execute function was written as a 3d.
  this->NumberOfExecutionAxes = 3;
  
  this->Component = 0;
}

//----------------------------------------------------------------------------
// Description:
// This method tells the superclass that only one component will remain.
void vtkImageExtractComponent::ExecuteImageInformation(vtkImageCache *in,
						       vtkImageCache *out)
{
  // Avoid compiler warnings.
  in = in;
  out->SetNumberOfScalarComponents(1);
}

//----------------------------------------------------------------------------
template <class T>
static void vtkImageExtractComponentExecute(vtkImageExtractComponent *self,
				    vtkImageRegion *inRegion, T *inPtr,
				    vtkImageRegion *outRegion, T *outPtr)
{
  int idx1, idx2;
  int min0, max0, min1, max1, min2, max2;
  int inInc0, inInc1, inInc2;
  int outInc0, outInc1, outInc2;
  T *inPtr1, *inPtr2;
  T *outPtr1, *outPtr2;
  int component;
  
  component = self->GetComponent();
  
  // get information to loop through pixels.
  inRegion->GetExtent(min0, max0, min1, max1, min2, max2);
  inRegion->GetIncrements(inInc0, inInc1, inInc2);
  outRegion->GetIncrements(outInc0, outInc1, outInc2);
  
  if (component < min0)
    {
    vtkGenericWarningMacro("Component " << component << " too small");
    component = min0;
    }
  if (component > max0)
    {
    vtkGenericWarningMacro("Component " << component << " too big");
    component = max0;
    }
  
  inPtr2 = inPtr + (inInc0 * component);
  outPtr2 = outPtr;
  for (idx2 = min2; idx2 <= max2; ++idx2)
    {
    inPtr1 = inPtr2;
    outPtr1 = outPtr2;
    for (idx1 = min1; idx1 <= max1; ++idx1)
      {
      *outPtr1 = *inPtr1;
      
      inPtr1 += inInc1;
      outPtr1 += outInc1;
      }
    inPtr2 += inInc2;
    outPtr2 += outInc2;
    }
}

//----------------------------------------------------------------------------
// Description:
// This method is passed input and output regions, and executes the
// ExtractComponent function on each line.  It handles 3 axes for speed.
void vtkImageExtractComponent::Execute(vtkImageRegion *inRegion, 
				       vtkImageRegion *outRegion)
{
  void *outPtr, *inPtr;

  inPtr = inRegion->GetScalarPointer();
  outPtr = outRegion->GetScalarPointer();

  // choose which templated function to call.
  switch (outRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageExtractComponentExecute(this, inRegion, (float *)(inPtr),
				      outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageExtractComponentExecute(this, inRegion, (int *)(inPtr),
				      outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageExtractComponentExecute(this, inRegion, (short *)(inPtr),
				      outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageExtractComponentExecute(this,inRegion,(unsigned short *)(inPtr),
				      outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageExtractComponentExecute(this, inRegion, (unsigned char *)(inPtr),
				      outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}



















